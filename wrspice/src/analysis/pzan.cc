
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: UCB CAD Group
         1992 Stephen R. Whiteley
****************************************************************************/

#include "pzdefs.h"
#include "device.h"
#include "input.h"
#include "output.h"
#include "misc.h"
#include "sparse/spmatrix.h"


#define DEBUG if (0)
#define ERROR(CODE, MESSAGE) {  \
  IP.logError(0, MESSAGE); \
  return (CODE); }


int
PZanalysis::anFunc(sCKT *ckt, int restart)
{
    sPZAN *pzan = static_cast<sPZAN*>(ckt->CKTcurJob);
    sCKTmodGen mgen(ckt->CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        if (DEV.device(m->GENmodType)->flags() & (DV_NOAC | DV_NOPZ)) {
            OP.error(ERR_FATAL,
                "PZ analysis not possible with device %s.",
                DEV.device(m->GENmodType)->name());
            return (OK);
        }
    }
    ckt->CKTcurrentAnalysis |= (DOING_AC | DOING_PZ);

    struct sOUTdata *outd;
    if (restart) {
        int error = pzInit(ckt);
        if (error != OK) {
            ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_PZ);
            return error;
        }

        error = pzan->JOBdc.init(ckt);
        if (error != OK) {
            ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_PZ);
            return error;
        }

        if (!pzan->JOBoutdata) {
            outd = new sOUTdata;
            pzan->JOBoutdata = outd;
        }
        else
            outd = pzan->JOBoutdata;

        outd->circuitPtr = ckt;
        outd->analysisPtr = pzan;
        outd->analName = pzan->JOBname;
        outd->refName = 0;
        outd->refType = 0;
        outd->dataType = IF_COMPLEX;
        outd->numPts = 1;
        outd->count = 0;
        outd->initValue = 0;
        outd->finalValue = 0;
        outd->step = 0;
    }
    else
        outd = (struct sOUTdata*)pzan->JOBoutdata;

    int error = pzan->JOBdc.loop(pz_dcoperation, ckt, restart);
    if (error < 0) {
        // pause
        ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_PZ);
        return (error);
    }

    OP.endPlot(pzan->JOBrun, false);
    ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_PZ);
    return (OK);
}


// Static provate function.
//
int
PZanalysis::pz_dcoperation(sCKT *ckt, int restart)
{
    (void)restart;
    sPZAN *pzan = static_cast<sPZAN*>(ckt->CKTcurJob);
    ckt->CKTpreload = 1; // do preload
    // normal reset
    int error = ckt->setup();
    if (error)
        return (error);
    error = ckt->temp();
    if (error)
        return (error);

    ckt->CKTmatrix->spSaveForInitialization();

    // Calculate small signal parameters at the operating point.
    error = ckt->ic();
    if (error)
        return (error);
    error = ckt->op(MODEDCOP | MODEINITJCT,
        MODEDCOP | MODEINITFLOAT, ckt->CKTcurTask->TSKdcMaxIter);
    if (error)
        return (error);

    ckt->CKTmode = MODEDCOP | MODEINITSMSIG;
    error = ckt->load();
    if (error)
        return (error);

    if (pzan->PZwhich & PZ_DO_POLES) {
        error = pzan->PZsetup(ckt, PZ_DO_POLES);
        if (error != OK)
            return (error);
        error = pzan->PZfindZeros(ckt, &pzan->PZpoleList, &pzan->PZnPoles);
        if (error != OK)
            return (error);
    }

    if (pzan->PZwhich & PZ_DO_ZEROS) {
        error = pzan->PZsetup(ckt, PZ_DO_ZEROS);
        if (error != OK)
            return (error);
        error = pzan->PZfindZeros(ckt, &pzan->PZzeroList, &pzan->PZnZeros);
        if (error != OK)
            return (error);
    }

    sOUTdata *outd = pzan->JOBoutdata;
    if (!outd->numNames) {
        IFuid *namelist = new IFuid[pzan->PZnPoles + pzan->PZnZeros];

        int i, j = 0;
        char name[50];
        for (i = 0; i < pzan->PZnPoles; i++) {
            snprintf(name, sizeof(name), "pole(%-u)", i+1);
            ckt->newUid(&namelist[j++], 0, name, UID_OTHER);
        }
        for (i = 0; i < pzan->PZnZeros; i++) {
            snprintf(name, sizeof(name), "zero(%-u)", i+1);
            ckt->newUid(&namelist[j++], 0, name, UID_OTHER);
        }
        outd->numNames = pzan->PZnPoles + pzan->PZnZeros;
        outd->dataNames = namelist;
        pzan->JOBrun = OP.beginPlot(outd);
        delete [] outd->dataNames;
        outd->dataNames = 0;
        if (!pzan->JOBrun)
            return (E_TOOMUCH);
    }

    int j = 0;
    IFcomplex *out_list = new IFcomplex[pzan->PZnPoles + pzan->PZnZeros];
    if (pzan->PZnPoles > 0) {
        for (PZtrial *root = pzan->PZpoleList; root != 0;
                root = root->next) {
            for (int i = 0; i < root->multiplicity; i++) {
                out_list[j].real = root->s.real;
                out_list[j].imag = root->s.imag;
                j += 1;
                if (root->s.imag != 0.0) {
                    out_list[j].real = root->s.real;
                    out_list[j].imag = -root->s.imag;
                    j += 1;
                }
            }
            DEBUG printf("LIST pole: (%g,%g) x %d\n",
                root->s.real, root->s.imag, root->multiplicity);
        }
    }

    if (pzan->PZnZeros > 0) {
        for (PZtrial *root = pzan->PZzeroList; root != 0;
                root = root->next) {
            for (int i = 0; i < root->multiplicity; i++) {
                out_list[j].real = root->s.real;
                out_list[j].imag = root->s.imag;
                j += 1;
                if (root->s.imag != 0.0) {
                    out_list[j].real = root->s.real;
                    out_list[j].imag = -root->s.imag;
                    j += 1;
                }
            }
            DEBUG printf("LIST zero: (%g,%g) x %d\n",
                root->s.real, root->s.imag, root->multiplicity);
        }
    }

    IFvalue outData;
    outData.v.numValue = pzan->PZnPoles + pzan->PZnZeros;
    outData.v.vec.cVec = out_list;

    OP.appendData(pzan->JOBrun, (IFvalue *) 0, &outData);
    delete [] out_list;
    outd->count++;
    return (OK);
}


// Static provate function.
// Perform error checking.
//
int
PZanalysis::pzInit(sCKT *ckt)
{
    sPZAN *pzan = static_cast<sPZAN*>(ckt->CKTcurJob);
    int i;
    if ((i = ckt->typelook("transmission line")) != -1 &&
            ckt->typelook(i) >= 0)
        ERROR(E_XMISSIONLINE, "Transmission lines not supported")

    pzan->PZpoleList = 0;
    pzan->PZzeroList = 0;
    pzan->PZnPoles = 0;
    pzan->PZnZeros = 0;

    if (pzan->PZin_pos == pzan->PZin_neg)
        ERROR(E_SHORT, "Input is shorted")

    if (pzan->PZout_pos == pzan->PZout_neg)
        ERROR(E_SHORT, "Output is shorted")

    if (pzan->PZin_pos == pzan->PZout_pos
            && pzan->PZin_neg == pzan->PZout_neg
            && pzan->PZinput_type == PZ_IN_VOL)
        ERROR(E_INISOUT, "Transfer function is unity")
    else
        if (pzan->PZin_pos == pzan->PZout_neg
                && pzan->PZin_neg == pzan->PZout_pos
                && pzan->PZinput_type == PZ_IN_VOL)
            ERROR(E_INISOUT, "Transfer function is -1")

    return (OK);
}
// End of PZanalysis functions.


// Iterate through all the various pzSetup functions provided for the
// circuit elements in the given circuit, setup ...
//
int
sPZAN::PZsetup(sCKT *ckt, int typ)
{
    sPZAN *pzan = static_cast<sPZAN*>(ckt->CKTcurJob);
    ckt->NIdestroy();
    int error = ckt->NIinit();
    if (error)
        return (error);

    ckt->CKTnumStates = 0;
    error = ckt->setup();
    if (error)
        return (error);

    ckt->CKTmatrix->spSetComplex();
    if (ckt->CKTmatrix->spDataAddressChange()) {
        error = ckt->resetup();
        if (error)
            return (error);
    }

    int solution_col = 0;
    int balance_col = 0;
    int input_pos = pzan->PZin_pos;
    int input_neg = pzan->PZin_neg;

    int output_pos, output_neg;
    if (typ == PZ_DO_ZEROS) {
        // Vo/Ii in Y
        output_pos = pzan->PZout_pos;
        output_neg = pzan->PZout_neg;
    }
    else if (pzan->PZinput_type == PZ_IN_VOL) {
        // Vi/Ii in Y
        output_pos = pzan->PZin_pos;
        output_neg = pzan->PZin_neg;
    }
    else {
        // Denominator
        output_pos = 0;
        output_neg = 0;
        input_pos = 0;
        input_neg = 0;
    }

    if (output_pos) {
        solution_col = output_pos;
        if (output_neg)
            balance_col = output_neg;
    }
    else {
        solution_col = output_neg;
        int temp = input_pos;
        input_pos = input_neg;
        input_neg = temp;
    }

    if (input_pos)
        pzan->PZdrive_pptr =
            ckt->CKTmatrix->spGetElement(input_pos, solution_col);
    else
        pzan->PZdrive_pptr = 0;

    if (input_neg)
        pzan->PZdrive_nptr =
            ckt->CKTmatrix->spGetElement(input_neg, solution_col);
    else
        pzan->PZdrive_nptr = 0;

    pzan->PZsolution_col = solution_col;
    pzan->PZbalance_col = balance_col;

    pzan->PZnumswaps = 1;

    error = ckt->NIreinit();
    if (error)
        return (error);

    return (OK);
}

