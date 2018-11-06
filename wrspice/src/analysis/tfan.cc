
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
Authors: 1988 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/


// subroutine to do Transfer Function analysis

#include "tfdefs.h"
#include "device.h"
#include "outdata.h"
#include "misc.h"
#include "sparse/spmatrix.h"


// DC sweep assumed to be swept V/I source.
#define SRC(x) ((sGENSRCinstance*)x)

int
TFanalysis::anFunc(sCKT *ckt, int restart)
{
    sTFAN *job = static_cast<sTFAN*>(ckt->CKTcurJob);

    int flg;
    if (job->JOBac.stepType() == DCSTEP) {
        sCKTmodGen mgen(ckt->CKTmodels);
        for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
            if (DEV.device(m->GENmodType)->flags() & DV_NODCT) {
                OP.error(ERR_FATAL,
                    "DC transfer analysis not possible with device %s.",
                    DEV.device(m->GENmodType)->name());
                return (OK);
            }
        }
        flg = DOING_TRCV | DOING_TF;
    }
    else {
        sCKTmodGen mgen(ckt->CKTmodels);
        for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
            if (DEV.device(m->GENmodType)->flags() & DV_NOAC) {
                OP.error(ERR_FATAL,
                    "AC transfer analysis not possible with device %s.",
                    DEV.device(m->GENmodType)->name());
                return (OK);
            }
        }
        flg = DOING_AC | DOING_TF;
    }
    ckt->CKTcurrentAnalysis |= flg;
    sOUTdata *outd;
    if (restart) {

        int error = job->init(ckt);
        if (error != OK) {
            ckt->CKTcurrentAnalysis &= ~flg;
            return (error);
        }
        int multip = job->JOBdc.points(ckt);

        // make a UID for the transfer function output
        IFuid uids[3];
        ckt->newUid(&uids[0], 0, "tranfunc", UID_OTHER);

        // make a UID for the input impedance
        ckt->newUid(&uids[1], job->TFinSrc, "Zi", UID_OTHER);

        // make a UID for the output impedance
        if (job->TFoutIsI) {
            ckt->newUid(&uids[2], job->TFoutSrc, "Zo", UID_OTHER);
        }
        else {
            char *nm = new char[strlen(job->TFoutName) + 22];
            const char *s = strchr(job->TFoutName, '(');
            if (s)
                sprintf(nm, "Zo%s", s);
            else
                sprintf(nm, "Zo(%s)", job->TFoutName);
            ckt->newUid(&uids[2], 0, nm, UID_OTHER);
        }

        if (!job->JOBoutdata) {
            outd = new sOUTdata;
            job->JOBoutdata = outd;
        }
        else
            outd = job->JOBoutdata;
        if (job->JOBac.stepType() == DCSTEP) {
            outd->dataType = IF_REAL;
            if (job->JOBdc.elt(0)) {
                const char *swp;
                if (SRC(job->JOBdc.elt(0))->SRCtype == GENSRC_V)
                    swp = "vsweep";
                else
                    swp = "isweep";
                ckt->newUid(&outd->refName, 0, swp, UID_OTHER);
                outd->refType = IF_REAL;
            }
            else {
                outd->refName = 0;
                outd->refType = 0;
            }
        }
        else {
            outd->dataType = IF_COMPLEX;
            ckt->newUid(&outd->refName, 0, "frequency", UID_OTHER);
            outd->refType = IF_REAL;
        }

        outd->numPts = job->points(ckt);

        outd->circuitPtr = ckt;
        outd->analysisPtr = ckt->CKTcurJob;
        outd->analName = ckt->CKTcurJob->JOBname;
        outd->numNames = 3;
        outd->dataNames = uids;
        outd->initValue = 0;
        outd->finalValue = 0;
        outd->step = 0;
        outd->count = 0;
        job->JOBrun = OP.beginPlot(outd, multip);
        if (!job->JOBrun) {
            ckt->CKTcurrentAnalysis &= ~flg;
            return (E_TOOMUCH);
        }
    }
    else
        outd = (struct sOUTdata*)job->JOBoutdata;

    int error = job->JOBdc.loop(tf_dcoperation, ckt, restart);
    if (error < 0) {
        // pause
        ckt->CKTcurrentAnalysis &= ~flg;
        return (error);
    }

    OP.endPlot(job->JOBrun, false);
    ckt->CKTcurrentAnalysis &= ~flg;
    return (OK);
}


// Static private function.
//
int
TFanalysis::tf_dcoperation(sCKT *ckt, int restart)
{
    sTFAN *job = static_cast<sTFAN*>(ckt->CKTcurJob);

    // find the operating point
    int error = ckt->ic();
    if (error)
        return (error);

    error = ckt->op(MODEDCOP | MODEINITJCT,
            MODEDCOP | MODEINITFLOAT, ckt->CKTcurTask->TSKdcMaxIter);
    if (error)
        return (error);

    if (job->JOBac.stepType() != DCSTEP) {
        ckt->CKTmode = MODEDCOP | MODEINITSMSIG;
        error = ckt->load();
        if (error)
            return (error);

        ckt->CKTniState |= NIACSHOULDREORDER;  // KLU requires this.
        error = job->JOBac.loop(tf_acoperation, ckt, restart);
        return (error);
    }

    int size = ckt->CKTmatrix->spGetSize(1);
    int i;
    for (i = 0; i <= size; i++)
        ckt->CKTrhs[i] = 0;

    if (job->TFinIsI) {
        ckt->CKTrhs[job->TFinSrcDev->SRCposNode] -= 1;
        ckt->CKTrhs[job->TFinSrcDev->SRCnegNode] += 1;
    }
    else
        ckt->CKTrhs[job->TFinSrcDev->SRCbranch] += 1;

    ckt->CKTmatrix->spSolve(ckt->CKTrhs, ckt->CKTrhs, 0, 0);
    ckt->CKTrhs[0] = 0;

    // find transfer function
    double outputs[3];
    if (job->TFoutIsV) {
        outputs[0] =
            ckt->CKTrhs[job->TFoutPos->number()] -
                ckt->CKTrhs[job->TFoutNeg->number()] ;
    }
    else
        outputs[0] = ckt->CKTrhs[job->TFoutSrcDev->SRCbranch];

    // now for input impedance
    if (job->TFinIsI) {
        outputs[1] = ckt->CKTrhs[job->TFinSrcDev->SRCnegNode] -
            ckt->CKTrhs[job->TFinSrcDev->SRCposNode];
    }
    else {
        double A = ckt->CKTrhs[job->TFinSrcDev->SRCbranch];
        if (fabs(A) < 1e-20)
            outputs[1] = 1e20;
        else
            outputs[1] = -1/A;
    }

    if (job->TFoutIsI && (job->TFoutSrc == job->TFinSrc)) {
        outputs[2] = outputs[1];
        // No need to compute output impedance when it is the same as 
        // the input.
        //
    }
    else {
        // now for output resistance
        for (i = 0; i <= size; i++)
            ckt->CKTrhs[i] = 0;
        if (job->TFoutIsV) {
            ckt->CKTrhs[job->TFoutPos->number()] -= 1;
            ckt->CKTrhs[job->TFoutNeg->number()] += 1;
        }
        else
            ckt->CKTrhs[job->TFoutSrcDev->SRCbranch] += 1;
        ckt->CKTmatrix->spSolve(ckt->CKTrhs, ckt->CKTrhs,
            ckt->CKTirhs, ckt->CKTirhs);
        ckt->CKTrhs[0] = 0;

        if (job->TFoutIsV) {
            outputs[2] =
                ckt->CKTrhs[job->TFoutNeg->number()] -
                ckt->CKTrhs[job->TFoutPos->number()];
        }
        else {
            double A = ckt->CKTrhs[job->TFoutSrcDev->SRCbranch];
            if (fabs(A) < 1e-20)
                outputs[2] = 1e20;
            else
                outputs[2] = 1/A;
        }
    }

    IFvalue outdata;
    outdata.v.numValue = 3;
    outdata.v.vec.rVec = outputs;
    sOUTdata *outd = job->JOBoutdata;
    if (outd->cycle > 0) {
        // Analysis is multi-threaded, we're computing one cycle. 
        // Compute the actual offset and insert data at that location.

        unsigned int os = (outd->cycle - 1)*outd->numPts + outd->count;
        if (job->JOBdc.elt(0)) {
            IFvalue refval;
            refval.rValue = SRC(job->JOBdc.elt(0))->SRCdcValue;
            OP.insertData(ckt, job->JOBrun, &refval, &outdata, os);
        }
        else
            OP.insertData(ckt, job->JOBrun, 0, &outdata, os);
    }
    else {
        if (job->JOBdc.elt(0)) {
            IFvalue refval;
            refval.rValue = SRC(job->JOBdc.elt(0))->SRCdcValue;
            OP.appendData(job->JOBrun, &refval, &outdata);
            OP.checkRunops(job->JOBrun, refval.rValue);
        }
        else
            OP.appendData(job->JOBrun, 0, &outdata);
    }
    outd->count++;
    return (OK);
}


// Static private function.
//
int
TFanalysis::tf_acoperation(sCKT *ckt, int restart)
{
    (void)restart;
    sTFAN *job = static_cast<sTFAN*>(ckt->CKTcurJob);

    ckt->CKTmode = MODEAC;
    int error = ckt->NIacIter();
    if (error)
        return (error);

    int size = ckt->CKTmatrix->spGetSize(1);
    int i;
    for (i = 0; i <= size; i++) {
        ckt->CKTrhs[i] = 0;
        ckt->CKTirhs[i] = 0;
    }

    if (job->TFinIsI) {
        ckt->CKTrhs[job->TFinSrcDev->SRCposNode] -= 1;
        ckt->CKTrhs[job->TFinSrcDev->SRCnegNode] += 1;
    }
    else
        ckt->CKTrhs[job->TFinSrcDev->SRCbranch] += 1;

    ckt->CKTmatrix->spSolve(ckt->CKTrhs, ckt->CKTrhs,
        ckt->CKTirhs, ckt->CKTirhs);
    ckt->CKTrhs[0] = 0;
    ckt->CKTirhs[0] = 0;

    // find transfer function
    IFcomplex coutputs[3];
    if (job->TFoutIsV) {
        coutputs[0].real =
            ckt->CKTrhs[job->TFoutPos->number()] -
                ckt->CKTrhs[job->TFoutNeg->number()] ;
        coutputs[0].imag =
            ckt->CKTirhs[job->TFoutPos->number()] -
                ckt->CKTirhs[job->TFoutNeg->number()] ;
    }
    else {
        coutputs[0].real = ckt->CKTrhs[job->TFoutSrcDev->SRCbranch];
        coutputs[0].imag = ckt->CKTirhs[job->TFoutSrcDev->SRCbranch];
    }

    // now for input impedance
    if (job->TFinIsI) {
        coutputs[1].real =
            ckt->CKTrhs[job->TFinSrcDev->SRCnegNode] -
                ckt->CKTrhs[job->TFinSrcDev->SRCposNode];
        coutputs[1].imag =
            ckt->CKTirhs[job->TFinSrcDev->SRCnegNode] -
                ckt->CKTirhs[job->TFinSrcDev->SRCposNode];
    }
    else {
        double A = ckt->CKTrhs[job->TFinSrcDev->SRCbranch];
        double B = ckt->CKTirhs[job->TFinSrcDev->SRCbranch];
        double MAG = A*A + B*B;
        if (MAG < 1e-40)
            MAG = 1e-40;
        coutputs[1].real = -A/MAG;
        coutputs[1].imag = B/MAG;
    }

    if (job->TFoutIsI && (job->TFoutSrc == job->TFinSrc)) {
        coutputs[2].real = coutputs[1].real;
        coutputs[2].imag = coutputs[1].imag;
        // No need to compute output impedance when it is the same as 
        // the input.
    }
    else {
        // now for output resistance
        for (i = 0; i <= size; i++) {
            ckt->CKTrhs[i] = 0;
            ckt->CKTirhs[i] = 0;
        }
        if (job->TFoutIsV) {
            ckt->CKTrhs[job->TFoutPos->number()] -= 1;
            ckt->CKTrhs[job->TFoutNeg->number()] += 1;
        }
        else
            ckt->CKTrhs[job->TFoutSrcDev->SRCbranch] += 1;
        ckt->CKTmatrix->spSolve(ckt->CKTrhs, ckt->CKTrhs,
            ckt->CKTirhs, ckt->CKTirhs);
        ckt->CKTrhs[0] = 0;
        ckt->CKTirhs[0] = 0;

        if (job->TFoutIsV) {
            coutputs[2].real =
                ckt->CKTrhs[job->TFoutNeg->number()] -
                ckt->CKTrhs[job->TFoutPos->number()];
            coutputs[2].imag =
                ckt->CKTirhs[job->TFoutNeg->number()] -
                ckt->CKTirhs[job->TFoutPos->number()];
        }
        else {
            double A = ckt->CKTrhs[job->TFoutSrcDev->SRCbranch];
            double B = ckt->CKTirhs[job->TFoutSrcDev->SRCbranch];
            double MAG = A*A + B*B;
            if (MAG < 1e-40)
                MAG = 1e-40;
            coutputs[2].real = A/MAG;
            coutputs[2].imag = -B/MAG;
        }
    }

    IFvalue outdata;
    outdata.v.numValue = 3;
    outdata.v.vec.cVec = coutputs;
    IFvalue refval;
    refval.rValue = ckt->CKTomega/(2*M_PI);
    sOUTdata *outd = job->JOBoutdata;
    if (outd->cycle > 0) {
        // Analysis is multi-threaded, we're computing one cycle. 
        // Compute the actual offset and insert data at that location.

        unsigned int os = (outd->cycle - 1)*outd->numPts + outd->count;
        OP.insertData(ckt, job->JOBrun, &refval, &outdata, os);
    }
    else {
        OP.appendData(job->JOBrun, &refval, &outdata);
        OP.checkRunops(job->JOBrun, refval.rValue);
    }
    outd->count++;
    return (OK);
}
// End of TFanalysis functions.


int
sTFAN::init(sCKT *ckt)
{
    // make sure sources are present
    int code = ckt->typelook("Source");
    sGENinstance *inst = TFinSrcDev;
    int error = ckt->findInst(&code, &inst, TFinSrc, 0, 0);
    if (error != OK) {
        OP.error(ERR_FATAL, "Transfer function source %s not in circuit",
            TFinSrc);
        return (E_NOTFOUND);
    }
    TFinSrcDev = (sGENSRCinstance*)inst;
    if (TFinSrcDev->SRCtype == GENSRC_V) {
        TFinIsV = 1;
        TFinIsI = 0;
    }
    else {
        TFinIsI = 1;
        TFinIsV = 0;
    }

    if (TFoutIsI) {
        inst = TFoutSrcDev;
        error = ckt->findInst(&code, &inst, TFoutSrc, 0, 0);
        if (error != 0) {
            OP.error(ERR_FATAL, "Transfer function source %s not in circuit",
                TFoutSrc);
            return (E_NOTFOUND);
        }
        TFoutSrcDev = (sGENSRCinstance*)inst;
        if (TFoutSrcDev->SRCtype != GENSRC_V) {
            OP.error(ERR_FATAL,
                "Transfer function source %s not a voltage source", TFoutSrc);
            return (E_NOTFOUND);
        }
    }

    error = JOBdc.init(ckt);
    return (error);
}

