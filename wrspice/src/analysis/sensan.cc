
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: UCB CAD Group
         1993 Stephen R. Whiteley
****************************************************************************/

#include "sensdefs.h"
#include "sensgen.h"
#include "device.h"
#include "output.h"
#include "sparse/spmatrix.h"


#ifndef M_LOG2E
#define M_LOG2E     1.4426950408889634074
#endif

double SENSanalysis::Sens_Delta = 0.000001;
double SENSanalysis::Sens_Abs_Delta = 0.000001;

// DC sweep assumed to be swept V/I source.
#define SRC(x) ((sGENSRCinstance*)x)


//    Procedure:
//
//        Determine operating point (call ckt->op)
//
//        For each frequency point:
//            (for AC) call NIacIter to get base node voltages
//            For each element/parameter in the test list:
//                construct the perturbation matrix
//                Solve for the sensitivities:
//                    delta_E = Y^-1 (delta_I - delta_Y E)
//                save results


int
SENSanalysis::anFunc(sCKT *ckt, int restart)
{
    sSENSAN *job = static_cast<sSENSAN*>(ckt->CKTcurJob);
    sSENSint *st = &job->ST;

    int flg;
    if (job->JOBac.stepType() == DCSTEP) {
        sCKTmodGen mgen(ckt->CKTmodels);
        for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
            if (DEV.device(m->GENmodType)->flags() & DV_NODCT) {
                OP.error(ERR_FATAL,
                    "DC sensitivity analysis not possible with device %s.",
                    DEV.device(m->GENmodType)->name());
                return (OK);
            }
        }
        flg = DOING_TRCV | DOING_SENS;
    }
    else {
        sCKTmodGen mgen(ckt->CKTmodels);
        for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
            if (DEV.device(m->GENmodType)->flags() & DV_NOAC) {
                OP.error(ERR_FATAL,
                    "AC sensitivity analysis not possible with device %s.",
                    DEV.device(m->GENmodType)->name());
                return (OK);
            }
        }
        flg = DOING_AC | DOING_SENS;
    }
    ckt->CKTcurrentAnalysis |= flg;

    sOUTdata *outd;
    if (restart) {

        // make sure sources are present
        int code = ckt->typelook("Source");

        if (job->SENSoutSrc) {
            sGENinstance *inst = job->SENSoutSrcDev;
            int error = ckt->findInst(&code, &inst, job->SENSoutSrc, 0, 0);
            if (error != 0) {
                OP.error(ERR_FATAL,
                    "Sensitivity Analysis source %s not in circuit",
                    job->SENSoutSrc);
                ckt->CKTcurrentAnalysis &= ~flg;
                return (E_NOTFOUND);
            }
            job->SENSoutSrcDev = (sGENSRCinstance*)inst;
        }

        int error = job->JOBdc.init(ckt);
        if (error != OK) {
            ckt->CKTcurrentAnalysis &= ~flg;
            return (error);
        }

        bool is_dc = (job->JOBac.stepType() == DCSTEP);
        error = st->setup(ckt->CKTmatrix->spGetSize(1), is_dc);
        if (error) {
            ckt->CKTcurrentAnalysis &= ~flg;
            return (error);
        }

        if (!job->JOBoutdata) {
            outd = new sOUTdata;
            job->JOBoutdata = outd;
        }
        else
            outd = job->JOBoutdata;
        outd->numNames = 0;

        sgen *sg = new sgen(ckt, is_dc);
        for (sg = sg->next(); sg; sg = sg->next())
            outd->numNames++;

        if (!outd->numNames) {
            delete outd;
            job->JOBoutdata = 0;
            st->clear();
            ckt->CKTcurrentAnalysis &= ~flg;
            return (E_NOTFOUND);
        }

        outd->dataNames = new IFuid[outd->numNames];
        char namebuf[513];
        int i;
        sg = new sgen(ckt, is_dc);
        for (i = 0, sg = sg->next(); sg; i++, sg = sg->next()) {
            if (!sg->sg_is_instparam) {
                sprintf(namebuf, "%s:%s",
                    (const char*)sg->sg_instance->GENname,
                    sg->sg_ptable[sg->sg_param].keyword);
            }
            else if ((sg->sg_ptable[sg->sg_param].dataType & IF_PRINCIPAL) &&
                    sg->sg_is_principle) {
                sprintf(namebuf, "%s", (char*)sg->sg_instance->GENname);
            }
            else {
                sprintf(namebuf, "%s.%s",
                    (const char*)sg->sg_instance->GENname,
                    sg->sg_ptable[sg->sg_param].keyword);
            }
            ckt->newUid(outd->dataNames + i, 0, namebuf, UID_OTHER);
        }

        if (is_dc) {
            outd->dataType = IF_REAL;
            ckt->newUid(&outd->refName, 0, "sweep", UID_OTHER);
        }
        else {
            outd->dataType = IF_COMPLEX;
            ckt->newUid(&outd->refName, 0, "frequency", UID_OTHER);
        }

        outd->circuitPtr = ckt;
        outd->analysisPtr = ckt->CKTcurJob;
        outd->analName = ckt->CKTcurJob->JOBname;
        outd->refType = IF_REAL;
        outd->initValue = 0;
        outd->finalValue = 0;
        outd->step = 0;
        outd->numPts = 1;
        outd->count = 0;
        job->JOBrun = OP.beginPlot(outd);

        delete [] outd->dataNames;
        outd->dataNames = 0;
        job->JOBoutdata = outd;
        if (!job->JOBrun) {
            ckt->CKTcurrentAnalysis &= ~flg;
            return (E_TOOMUCH);
        }

        if (is_dc) {
            st->o_values = new double[outd->numNames];
            st->o_cvalues = 0;
        }
        else {
            st->o_values = 0;
            st->o_cvalues = new IFcomplex[outd->numNames];
        }
    }
    else
        outd = job->JOBoutdata;

    int error = job->JOBdc.loop(sens_dcoperation, ckt, restart);
    if (error < 0) {
        // pause
        ckt->CKTcurrentAnalysis &= ~flg;
        return (error);
    }

    OP.endPlot(job->JOBrun, false);
    st->clear();

    ckt->CKTcurrentAnalysis &= ~flg;
    return (error);
}


// Static private function.
//
int
SENSanalysis::sens_dcoperation(sCKT *ckt, int restart)
{
    sSENSAN *job = static_cast<sSENSAN*>(ckt->CKTcurJob);
    struct sSENSint *st = &job->ST;

    ckt->CKTpreload = 1; // do preload
    // Do the DC analysis at this point.  It appears to be necessary
    // to start from scratch.
    //
    int error = ckt->setup();
    if (error)
        return (error);
    error = ckt->temp();
    if (error)
        return (error);

    // If using KLU, the KLUmatrix is created, the original struct
    // cleared, and all further matrix operations will be done using
    // KLU.
    //
    ckt->CKTmatrix->spSwitchMatrix();

    // This caches the real part for reinitializing the matrix with
    // spLoadInitialization.
    //
    ckt->CKTmatrix->spSaveForInitialization();  // cache real part
    if (ckt->CKTmatrix->spDataAddressChange()) {
        // We're using KLU, so all of the pointers into the
        // original matrix are bogus.  Call resetup to
        // get the new pointers.

        error = ckt->resetup();
        if (error)
            return (error);
    }

    error = 1;
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
    if (job->JOBdc.elt(0))
        ckt->CKTtime = SRC(job->JOBdc.elt(0))->SRCdcValue;

    // Save the DC sweep values, these will have to be restored after
    // the sensitivity calculation or the sweep status is lost.
    //
    double tmpVal0 = 0.0;
    double tmpVal1 = 0.0;
    if (job->JOBdc.elt(0))
        tmpVal0 = SRC(job->JOBdc.elt(0))->SRCdcValue;
    if (job->JOBdc.elt(1))
        tmpVal1 = SRC(job->JOBdc.elt(1))->SRCdcValue;

    if (job->JOBac.stepType() != DCSTEP) {
        error = job->JOBac.loop(sens_acoperation, ckt, restart);
        if (job->JOBdc.elt(0))
            SRC(job->JOBdc.elt(0))->SRCdcValue = tmpVal0;
        if (job->JOBdc.elt(1))
            SRC(job->JOBdc.elt(1))->SRCdcValue = tmpVal1;
        return (error);
    }

    // Save some other things that are affected by sensitivity
    // calculation.
    //
    int tmpNst = ckt->CKTnumStates;
    int tmpBypass = ckt->CKTcurTask->TSKbypass;
    ckt->CKTcurTask->TSKbypass = 0;
    double *tmpRhs = ckt->CKTrhs;
    double *tmpRhsOld = ckt->CKTrhsOld;
    spMatrixFrame *tmpMat = ckt->CKTmatrix;

    // Swap in a different vector and matrix.
    // The st->dY matrix does not use KLU or sorting.
    ckt->CKTrhs = st->dIr;
    ckt->CKTmatrix = st->dY;

    // Calculate effect of each parameter.
    int i;
    IFdata data;
    IFdata ndata;
    sgen *sg = new sgen(ckt, true);
    for (i = 0, sg = sg->next(); sg; i++, sg = sg->next()) {

        // clear CKTmatrix, CKTrhs
        st->dY->spSetReal();
        st->dY->spClear();
        int j;
        for (j = 0; j <= st->size; j++)
            st->dIr[j] = 0.0;
        error = sg->load_new(true);
        if (error)
            goto done;

        // Alter the parameter.
        double delta_var;
        if (sg->sg_value != 0.0)
            delta_var = sg->sg_value * Sens_Delta;
        else
            delta_var = Sens_Abs_Delta;

        ndata.v.rValue = sg->sg_value + delta_var;
        ndata.type = IF_REAL;
        error = sg->set_param(&ndata);
        if (error)
            goto done;

        // Change the sign of CKTmatrix, CKTrhs.
        st->dY->spConstMult(-1.0);
        for (j = 0; j <= st->size; j++)
            st->dIr[j] = -st->dIr[j];

        error = sg->load_new(true);
        if (error)
            goto done;

        // Now have delta_Y = CKTmatrix, delta_I = CKTrhs.

        // Reset parameter.
        data.v.rValue = sg->sg_value;
        data.type = IF_REAL;
        sg->set_param(&data);

        // delta_Y E
        st->dY->spMultiply(st->dIdYr, tmpRhsOld, 0, 0);

        // delta_I - delta_Y E
        for (j = 0; j <= st->size; j++)
            st->dIr[j] -= st->dIdYr[j];

        // Solve; Y already factored.
        tmpMat->spSolve(st->dIr, st->dIr, 0, 0);

        // delta_I is now equal to delta_E

        st->dIr[0] = 0.0;
        if (job->SENSoutName)
            st->o_values[i] =
                st->dIr[job->SENSoutPos->number()] -
                st->dIr[job->SENSoutNeg->number()];
        else {
            st->o_values[i] = st->dIr[job->SENSoutSrcDev->SRCbranch];
        }
        st->o_values[i] /= delta_var;
    }

    ndata.v.v.vec.rVec = st->o_values;
    if (job->JOBdc.elt(0))
        data.v.rValue = SRC(job->JOBdc.elt(0))->SRCdcValue;
    else
        data.v.rValue = 0;
    OP.appendData(job->JOBrun, &data.v, &ndata.v);
    job->JOBoutdata->count++;

done:
    // Put things back to original.
    if (job->JOBdc.elt(0))
        SRC(job->JOBdc.elt(0))->SRCdcValue = tmpVal0;
    if (job->JOBdc.elt(1))
        SRC(job->JOBdc.elt(1))->SRCdcValue = tmpVal1;
    ckt->CKTnumStates = tmpNst;
    ckt->CKTrhs = tmpRhs;
    ckt->CKTrhsOld = tmpRhsOld;
    ckt->CKTmatrix = tmpMat;

    ckt->CKTcurTask->TSKbypass = tmpBypass;

    return (error);
}


// Static private function.
//
int
SENSanalysis::sens_acoperation(sCKT *ckt, int restart)
{
    (void)restart;
    sSENSAN *job = static_cast<sSENSAN*>(ckt->CKTcurJob);
    sSENSint *st = &job->ST;

    ckt->CKTpreload = 1; // do preload
    // Do the AC analysis at this point.  It appears to be necessary
    // to start from scratch.
    //
    int error = ckt->setup();
    if (error)
        return (error);
    error = ckt->temp();
    if (error)
        return (error);

    // If using KLU, the KLUmatrix is created, the original struct
    // cleared, and all further matrix operations will be done using
    // KLU.
    //
    ckt->CKTmatrix->spSwitchMatrix();

    // This caches the real part for reinitializing the matrix with
    // spLoadInitialization.
    //
    ckt->CKTmatrix->spSaveForInitialization();  // cache real part
    if (ckt->CKTmatrix->spDataAddressChange()) {
        // We're using KLU, so all of the pointers into the
        // original matrix are bogus.  Call resetup to
        // get the new pointers.

        error = ckt->resetup();
        if (error)
            return (error);
    }

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

    ckt->CKTniState |= NIACSHOULDREORDER;  // KLU requires this.
    ckt->CKTmode = MODEAC;
    error = ckt->NIacIter();
    if (error)
        return (error);

    // Save some other things that are affected by sensitivity
    // calculation.
    //
    int tmpBypass = ckt->CKTcurTask->TSKbypass;
    ckt->CKTcurTask->TSKbypass = 0;
    double *tmpRhs = ckt->CKTrhs;
    double *tmpIRhs = ckt->CKTirhs;
    double *tmpRhsOld = ckt->CKTrhsOld;
    double *tmpIRhsOld = ckt->CKTirhsOld;
    spMatrixFrame *tmpMat = ckt->CKTmatrix;

    // Swap in a different vector & matrix.
    // The st->dY matrix does not use KLU or sorting.
    ckt->CKTrhs = st->dIr;
    ckt->CKTirhs = st->dIi;
    ckt->CKTmatrix = st->dY;

    // Calculate effect of each parameter.
    int i;
    IFdata data;
    IFdata ndata;
    sgen *sg = new sgen(ckt, false);
    for (i = 0, sg = sg->next(); sg; i++, sg = sg->next()) {

        // Clear CKTmatrix, CKTrhs.
        st->dY->spSetComplex();
        st->dY->spClear();
        int j;
        for (j = 0; j <= st->size; j++) {
            st->dIr[j] = 0.0;
            st->dIi[j] = 0.0;
        }
        error = sg->load_new(false);
        if (error)
            return (error);

        // Alter the parameter.
        double delta_var;
        if (sg->sg_value != 0.0)
            delta_var = sg->sg_value * Sens_Delta;
        else
            delta_var = Sens_Abs_Delta;

        ndata.v.rValue = sg->sg_value + delta_var;
        ndata.type = IF_REAL;
        error = sg->set_param(&ndata);
        if (error)
            return (error);

        // Change sign of CKTmatrix, CKTrhs.
        st->dY->spConstMult(-1.0);
        for (j = 0; j <= st->size; j++) {
            st->dIr[j] = -st->dIr[j];
            st->dIi[j] = -st->dIi[j];
        }

        error = sg->load_new(false);
        if (error)
            return (error);

        // Now have delta_Y = CKTmatrix, delta_I = CKTrhs.

        // Reset parameter.
        data.v.rValue = sg->sg_value;
        data.type = IF_REAL;
        sg->set_param(&data);

        // delta_Y E
        st->dY->spMultiply(st->dIdYr, tmpRhsOld, st->dIdYi, tmpIRhsOld);

        // delta_I - delta_Y E
        for (j = 0; j <= st->size; j++) {
            st->dIr[j] -= st->dIdYr[j];
            st->dIi[j] -= st->dIdYi[j];
        }

        // Solve; Y already factored.
        tmpMat->spSolve(st->dIr, st->dIr, st->dIi, st->dIi);

        // delta_I is now equal to delta_E

        st->dIr[0] = 0.0;
        st->dIi[0] = 0.0;
        if (job->SENSoutName) {
            st->o_cvalues[i].real =
                st->dIr[job->SENSoutPos->number()] -
                st->dIr[job->SENSoutNeg->number()];
            st->o_cvalues[i].imag =
                st->dIi[job->SENSoutPos->number()] -
                st->dIi[job->SENSoutNeg->number()];
        }
        else {
            st->o_cvalues[i].real =
                st->dIr[job->SENSoutSrcDev->SRCbranch];
            st->o_cvalues[i].imag =
                st->dIi[job->SENSoutSrcDev->SRCbranch];
        }
        st->o_cvalues[i].real /= delta_var;
        st->o_cvalues[i].imag /= delta_var;
    }

    ndata.v.v.vec.cVec = st->o_cvalues;
    data.v.rValue = ckt->CKTomega/(2*M_PI);
    OP.appendData(job->JOBrun, &data.v, &ndata.v);
    job->JOBoutdata->count++;

    ckt->CKTrhs = tmpRhs;
    ckt->CKTirhs = tmpIRhs;
    ckt->CKTrhsOld = tmpRhsOld;
    ckt->CKTirhsOld = tmpIRhsOld;
    ckt->CKTmatrix = tmpMat;
    ckt->CKTcurTask->TSKbypass = tmpBypass;

    return (OK);
}

