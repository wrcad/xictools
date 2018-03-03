
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
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "dctdefs.h"
#include "device.h"
#include "outdata.h"


int
DCTanalysis::anFunc(sCKT *ckt, int restart)
{
    sDCTAN *job = static_cast<sDCTAN*>(ckt->CKTcurJob);
    sCKTmodGen mgen(ckt->CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        if (DEV.device(m->GENmodType)->flags() & DV_NODCT) {
            OP.error(ERR_FATAL,
                "DC analysis not possible with device %s.",
                DEV.device(m->GENmodType)->name());
            return (OK);
        }
        if (DEV.device(m->GENmodType)->flags() & DV_JJSTEP)
            ckt->CKTjjPresent = true;
    }
    ckt->CKTcurrentAnalysis |= DOING_TRCV;

    if (restart) {
        if (!job->JOBoutdata)
            job->JOBoutdata = new sOUTdata;
        int error = dct_init(ckt);
        if (error) {
            ckt->CKTcurrentAnalysis &= ~DOING_TRCV;
            return (error);
        }
    }

    int error = job->JOBdc.loop(dct_operation, ckt, restart);
    if (error < 0) {
        // pause
        ckt->CKTcurrentAnalysis &= ~DOING_TRCV;
        return (error);
    }

    OP.endPlot(job->JOBrun, false);
    ckt->CKTcurrentAnalysis &= ~DOING_TRCV;
    return (error);
}


// Static private function.
//
int
DCTanalysis::dct_init(sCKT *ckt)
{
    sDCTAN *job = static_cast<sDCTAN*>(ckt->CKTcurJob);
    sOUTdata *outd = job->JOBoutdata;
    ckt->CKTtime = 0;
    ckt->CKTdelta = job->JOBdc.vstep(0);
    ckt->CKTmode = MODEDCTRANCURVE | MODEINITJCT;
    ckt->CKTorder = 1;

    int i;
    for (i = 0; i < 7; i++)
        ckt->CKTdeltaOld[i] = ckt->CKTdelta;

    int error = job->JOBdc.init(ckt);
    if (error)
        return (error);

    error = ckt->names(&outd->numNames, &outd->dataNames);
    if (error)
        return (error);

    const char *swp;
    sGENinstance *inst = job->JOBdc.elt(0);
    if (inst->GENmodPtr->GENmodType == ckt->typelook("Source")) {
        if (((sGENSRCinstance*)inst)->SRCtype == GENSRC_V)
            swp = "vsweep";
        else
            swp = "isweep";
    }
    else
        swp = "sweep";
    ckt->newUid(&outd->refName, 0, swp, UID_OTHER);

    outd->circuitPtr = ckt;
    outd->analysisPtr = ckt->CKTcurJob;
    outd->analName = ckt->CKTcurJob->JOBname;
    outd->refType = IF_REAL;
    outd->dataType = IF_REAL;
    outd->numPts = 1;
    outd->initValue = job->JOBdc.vstart(0);
    outd->finalValue = job->JOBdc.vstop(0);
    outd->step = job->JOBdc.vstep(0);
    outd->count = 0;
    job->JOBrun = OP.beginPlot(outd);
    if (!job->JOBrun)
        return (E_TOOMUCH);
    return (OK);
}


// Static private function.
//
int
DCTanalysis::dct_operation(sCKT *ckt, int restart)
{
    (void)restart;
    sDCTAN *job = static_cast<sDCTAN*>(ckt->CKTcurJob);

#ifdef ALLPRMS
    IFdevice *dev = DEV.device(job->JOBdc.elt(0)->GENmodPtr->GENmodType);
    IFdata data;
    int error = dev->askInst(ckt, job->JOBdc.elt(0), job->JOBdc.param(0),
        &data);
    if (error)
        return (error);
    if ((data.type & IF_VARTYPES) == IF_REAL)
        ckt->CKTtime = data.v.rValue;
    else
        return (E_BADPARM);
    error = 1;
#else
    ckt->CKTtime = job->JOBdc.elt(0)->SRCdcValue;
    int error = 1;
#endif

    if (ckt->CKTmode & MODEINITPRED) {
        double *temp = ckt->CKTstates[ckt->CKTcurTask->TSKmaxOrder+1];
        for (int i = ckt->CKTcurTask->TSKmaxOrder; i >= 0; i--)
            ckt->CKTstates[i+1] = ckt->CKTstates[i];
        ckt->CKTstate0 = temp;
        error = ckt->NIiter(ckt->CKTcurTask->TSKdcTrcvMaxIter);
    }
    if (error) {

        error = ckt->ic();
        if (error)
            return (error);

        error = ckt->op(MODEDCOP | MODEINITJCT, 
            MODEDCOP | MODEINITFLOAT, ckt->CKTcurTask->TSKdcMaxIter);
        if (error)
            return (error);
        memcpy(ckt->CKTstate1, ckt->CKTstate0, 
            ckt->CKTnumStates*sizeof(double));
    }
    ckt->CKTmode = MODEDCTRANCURVE | MODEINITPRED;

    ckt->dump(ckt->CKTtime, job->JOBrun);

    return (OK);
}

