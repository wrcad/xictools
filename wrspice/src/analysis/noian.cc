
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
Authors: 1987 Gary W. Ng
         1992 Stephen R. Whiteley
****************************************************************************/

#include "noisdefs.h"
#include "device.h"
#include "outdata.h"
#include "miscutil/lstring.h"


int
NOISEanalysis::anFunc(sCKT *ckt, int restart)
{
    sNdata *ndata = 0;
    sNdata *idata = 0;
    sNOISEAN *job = static_cast<sNOISEAN*>(ckt->CKTcurJob);
    static const char *nodundef = "noise output node %s is not defined";
    static const char *noacinput = "no AC input source %s for noise analysis";

    sCKTmodGen mgen(ckt->CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        if (DEV.device(m->GENmodType)->flags() & (DV_NOAC | DV_NONOIS)) {
            OP.error(ERR_FATAL,
                "Noise analysis not possible with device %s.",
                DEV.device(m->GENmodType)->name());
            return (OK);
        }
    }
    ckt->CKTcurrentAnalysis |= (DOING_AC | DOING_NOISE);

    // Make sure all the nodes/sources referenced are actually
    // in the circuit.
    //
    int error;
    sOUTdata *outdN;
    sOUTdata *outdI;
    if (restart) {
        job->NposOutNode = -1;
        if (job->Noutput) {
            char *tmp = lstring::copy(job->Noutput);
            sCKTnode *node;
            if (ckt->findTerm(&tmp, &node) == OK)
                job->NposOutNode = node->number();
        }
        if (job->NposOutNode == -1) {
            OP.error(ERR_FATAL, nodundef, job->Noutput);
            ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_NOISE);
            return (E_NODUNDEF);
        }
        if (job->NoutputRef) {     // Was an output reference specified?
            job->NnegOutNode = -1;
            char *tmp = lstring::copy(job->NoutputRef);
            sCKTnode *node;
            if (ckt->findTerm(&tmp, &node) == OK)
                job->NnegOutNode = node->number();
            if (job->NnegOutNode == -1) {
                OP.error(ERR_FATAL, nodundef, job->NoutputRef);
                ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_NOISE);
                return (E_NODUNDEF);
            }
        }
        else
            job->NnegOutNode = 0;
            // If no output reference node was specified, we assume ground.

        // See if the source specified is AC.

        int code = ckt->typelook("Source");
        sGENinstance *here = 0;
        error = ckt->findInst(&code, &here, job->Ninput, 0, 0);

        if (error) {
            OP.error(ERR_FATAL, noacinput, job->Ninput);
            ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_NOISE);
            return (E_NOACINPUT);
        }

        error = job->JOBdc.init(ckt);
        if (error) {
            ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_NOISE);
            return (error);
        }

        if (job->JOBac.fstart() != job->JOBac.fstop()) {
            idata = new sNdata;

            error = idata->noise(ckt, INT_NOIZ, N_OPEN);
            if (error) {
                ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_NOISE);
                return (error);
            }
            job->IdataPtr = idata;

            if (!job->Ioutdata) {
                outdI = new sOUTdata;
                job->Ioutdata = outdI;
            }
            else
                outdI = job->Ioutdata;

            outdI->circuitPtr = ckt;
            outdI->analysisPtr = ckt->CKTcurJob;
            outdI->analName = (char*)"Noise, Integrated";
            outdI->refName = 0;
            outdI->refType = 0;
            outdI->numNames = idata->numPlots;
            outdI->dataNames = idata->namelist;
            outdI->dataType = IF_REAL;
            outdI->numPts = 1;
            outdI->initValue = 0;
            outdI->finalValue = 0;
            outdI->step = 0;
            outdI->count = 0;
            job->Irun = OP.beginPlot(outdI);
            if (!job->Irun) {
                ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_NOISE);
                return (E_TOOMUCH);
            }
        }
        else {
            if (job->IdataPtr) {
                delete [] job->IdataPtr->namelist;
                delete [] job->IdataPtr->outpVector;
                delete job->IdataPtr;
                job->IdataPtr = 0;
            }
            delete job->Ioutdata;
            job->Ioutdata = 0;
        }

        // The current front-end needs the namelist to be fully
        // declared before beginPlot().
        //
        if (!job->JOBoutdata) {
            outdN = new sOUTdata;
            job->JOBoutdata = outdN;
        }
        else
            outdN = job->JOBoutdata;

        ckt->newUid(&outdN->refName, 0, "frequency", UID_OTHER);

        ndata = new sNdata;
        ndata->numPlots = 0;    // we don't have any plots yet
        error = ndata->noise(ckt, N_DENS, N_OPEN);
        if (error) {
            ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_NOISE);
            return (error);
        }
        job->NdataPtr = ndata;

        // This will be the "current plot"
        outdN->circuitPtr = ckt;
        outdN->analysisPtr = ckt->CKTcurJob;
        outdN->analName = (char*)"Noise, Spectrum";
        outdN->refType = IF_REAL;
        outdN->numNames = ndata->numPlots;
        outdN->dataNames = ndata->namelist;
        outdN->dataType = IF_REAL;
        outdN->numPts = 1;
        outdN->initValue = 0;
        outdN->finalValue = 0;
        outdN->step = 0;
        outdN->count = 0;
        job->JOBrun = OP.beginPlot(outdN);
        if (!job->JOBrun) {
            ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_NOISE);
            return (E_TOOMUCH);
        }
    }
    else {
        outdI = (struct sOUTdata*)job->Ioutdata;
        outdN = (struct sOUTdata*)job->JOBoutdata;
        ndata = job->NdataPtr;
        idata = job->IdataPtr;
    }

    // Do the noise analysis over all frequencies.
    error = job->JOBdc.loop(noi_dcoperation, ckt, restart);
    if (error < 0) {
        // pause
        ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_NOISE);
        return (error);
    }
    ndata->noise(ckt, N_DENS, N_CLOSE);
    if (idata)
        idata->noise(ckt, INT_NOIZ, N_CLOSE);

    ckt->CKTcurrentAnalysis &= ~(DOING_AC | DOING_NOISE);
    return (error);
}


// Static private function.
//
int
NOISEanalysis::noi_dcoperation(sCKT *ckt, int restart)
{
    sNOISEAN *job = static_cast<sNOISEAN*>(ckt->CKTcurJob);
    sNdata *data = job->NdataPtr;
    sNdata *idata = job->IdataPtr;

    if (restart) {
        data->freq = job->JOBac.fstart();
        data->lstFreq = data->freq;
        data->outNoiz = 0.0;
        data->inNoise = 0.0;
        job->Nstep = 0;
    }
    else {
        // We must have paused before, pick up where we left off.
        data->outNoiz = job->NsavOnoise;
        data->inNoise = job->NsavInoise;
    }

    int error = ckt->ic();
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
    error = job->JOBac.loop(noi_operation, ckt, restart);
    if (error < 0) {
        // pause
        job->NsavOnoise = data->outNoiz; // up until now
        job->NsavInoise = data->inNoise;
        return (error);
    }
    if (idata) {
        idata->outNoiz = data->outNoiz;
        idata->inNoise = data->inNoise;
        idata->outNumber = 0;
        error = idata->noise(ckt, INT_NOIZ, N_CALC);
        if (error)
            return (error);
        int dims[3];
        job->JOBdc.get_dims(dims);
        if (dims[1] > 1) {
            // hack for dimensions
            dims[2] = 1;
            OP.setDims(job->Irun, dims+1, 2);
        }
    }
    return (OK);
}


// Static private function.
//
int
NOISEanalysis::noi_operation(sCKT *ckt, int restart)
{
    (void)restart;
    sNOISEAN *job = static_cast<sNOISEAN*>(ckt->CKTcurJob);
    sNdata *data = job->NdataPtr;
    ckt->NIacIter();
    data->freq = ckt->CKTomega/(2*M_PI);
    double realVal = *((ckt->CKTrhsOld) + job->NposOutNode) -
        *((ckt->CKTrhsOld) + job->NnegOutNode);
    double imagVal = *((ckt->CKTirhsOld) + job->NposOutNode) -
        *((ckt->CKTirhsOld) + job->NnegOutNode);
    double dtmp = realVal*realVal + imagVal*imagVal;
    if (dtmp < N_MINGAIN) {
        // The transfer gain is too small, assume a limit and
        // set a flag.  The @vin output will be crud.
        data->gainLimit = true;
        dtmp = N_MINGAIN;
    }
    data->GainSqInv = 1.0/dtmp;
    data->lnGainInv = log(data->GainSqInv);

    // Set up a block of "common" data so we don't have to
    // recalculate it for every device.

    data->delFreq = job->Nstep ? data->freq - data->lstFreq : 0.0;
    data->lnFreq = log(SPMAX(data->freq, N_MINLOG));
    data->lnLastFreq = log(SPMAX(data->lstFreq, N_MINLOG));
    data->delLnFreq = data->lnFreq - data->lnLastFreq;

    if ((job->NStpsSm != 0) && ((job->Nstep % (job->NStpsSm)) == 0))
        data->prtSummary = true;
    else
        data->prtSummary = false;

    data->outNumber = 0;
    // The frequency will NOT be stored in array[0]  as before; instead,
    // it will be given in refVal.rValue (see later).

    // Solve the adjoint system.
    ckt->NInzIter(job->NposOutNode, job->NnegOutNode);

    // Now we use the adjoint system to calculate the noise
    // contributions of each generator in the circuit.

    int error = data->noise(ckt, N_DENS, N_CALC);
    if (error)
        return (error);
    job->JOBoutdata->count++;

    data->lstFreq = data->freq;
    job->Nstep++;

    return (OK);
}
// End of NOISEanalysis functions.


// This method is responsible for naming and evaluating all of the
// noise sources in the circuit.  It uses a series of subroutines to
// name and evaluate the sources associated with each model, and then
// it evaluates the noise for the entire circuit.
//
int
sNdata::noise (sCKT *ckt, int mode, int operation)
{
    sNOISEAN *job = static_cast<sNOISEAN*>(ckt->CKTcurJob);
    // let each device decide how many and what type of noise sources
    // it has
    //
    double outNdens = 0.0;
    sCKTmodGen mgen(ckt->CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        int error = DEV.device(m->GENmodType)->noise(mode, operation, m,
            ckt, this, &outNdens);
        if (error)
            return (error);
    }

    if (operation == N_OPEN) {
        // Take care of the noise for the circuit as a whole.
        char buf[128];
        char *s = buf;
        if (*job->Noutput >= '0' && *job->Noutput <= '9')
            *s++ = 'n';
        strcpy(s, job->Noutput);
        while (*s)
            s++;
        if (job->NoutputRef) {
            *s++ = '_';
            strcpy(s, job->NoutputRef);
            while (*s)
                s++;
        }
        *s++ = ':';

        Realloc(&namelist, numPlots+2, numPlots);
        if (mode == N_DENS) {
            strcpy(s, "dens");
            ckt->newUid(&namelist[numPlots++], 0, buf, UID_OTHER);
            sprintf(s, "dens@%s", job->Ninput);
            ckt->newUid(&namelist[numPlots++], 0, buf, UID_OTHER);

            // we've added two more plots
            outpVector = new double[numPlots];
            return (OK);
        }
        if (mode == INT_NOIZ) {
            strcpy(s, "tot");
            ckt->newUid(&namelist[numPlots++], 0, buf, UID_OTHER);
            sprintf(s, "tot@%s", job->Ninput);
            ckt->newUid(&namelist[numPlots++], 0, buf, UID_OTHER);

            // we've added two more plots
            outpVector = new double[numPlots];
            return (OK);
        }
        return (E_INTERN);
    }

    if (operation == N_CALC) {
        IFvalue outData;
        IFvalue refVal;
        if (mode == N_DENS) {
            if ((((sNOISEAN*)ckt->CKTcurJob)->NStpsSm == 0) || prtSummary) {
                outpVector[outNumber++] = outNdens;
                outpVector[outNumber++] = (outNdens * GainSqInv);
                refVal.rValue = freq; // the reference is the freq
                outData.v.numValue = outNumber; // vector number
                outData.v.vec.rVec = outpVector; // vector of outputs
                OP.appendData(job->JOBrun, &refVal, &outData);
                OP.checkRunops(job->JOBrun, freq);
            }
            return (OK);
        }
        if (mode == INT_NOIZ) {
            outpVector[outNumber++] =  outNoiz; 
            outpVector[outNumber++] =  inNoise;
            outData.v.vec.rVec = outpVector; // vector of outputs
            outData.v.numValue = outNumber;  // vector number
            OP.appendData(job->Irun, &refVal, &outData);
            return (OK);
        }
        return (E_INTERN);
    }

    if (operation == N_CLOSE) {
        const char *nmsg = "Gain is less than the minimum %g, input "
            "noise values aren't good.";
        if (mode == N_DENS) {
            if (job->NdataPtr && job->NdataPtr->gainLimit) {
                char *tmp = new char[strlen(nmsg) + 20];
                sprintf(tmp, nmsg, N_MINGAIN);
                OP.addPlotNote(job->JOBrun, tmp);
                delete [] tmp;
            }
            OP.endPlot(job->JOBrun, false);
            delete [] namelist;
            namelist = 0;
            delete [] outpVector;
            outpVector = 0;
            return (OK);
        }
        if (mode == INT_NOIZ) {
            if (job->IdataPtr && job->IdataPtr->gainLimit) {
                char *tmp = new char[strlen(nmsg) + 20];
                sprintf(tmp, nmsg, N_MINGAIN);
                OP.addPlotNote(job->Irun, tmp);
                delete [] tmp;
            }
            OP.endPlot(job->Irun, false);
            delete [] namelist;
            namelist = 0;
            delete [] outpVector;
            outpVector = 0;
            return (OK);
        }
    }
    return (E_INTERN);
}


//    This functions evaluates the integral of the function
//
//                                             EXPONENT
//                      NOISE = a * (FREQUENCY)
//
//   given two points from the curve.  If EXPONENT is relatively close
//   to 0, the noise is simply multiplied by the change in frequency.
//   If it isn't, a more complicated expression must be used.  Note that
//   EXPONENT = -1 gives a different equation than EXPONENT <> -1.
//   Hence, the reason for the constant 'N_INTUSELOG'.
//
double
sNdata::integrate(double noizDens, double lnNdens, double lnNlstDens)
{
    double exponent = (lnNdens - lnNlstDens) / delLnFreq;
    if (fabs(exponent) < N_INTFTHRESH)
        return (noizDens * delFreq);
    else {
        double a = exp(lnNdens - exponent*lnFreq);
        exponent += 1.0;

        if (fabs(exponent) < N_INTUSELOG)
            return (a * (lnFreq - lnLastFreq));
        else
            return (a * ((exp(exponent * lnFreq) -
                exp(exponent * lnLastFreq)) / exponent));
    }
}


//   This functions evaluates the noise due to different physical
//   phenomena.  This includes the "shot" noise associated with dc
//   currents in semiconductors and the "thermal" noise associated with
//   resistance.  Although semiconductors also display "flicker" (1/f)
//   noise, the lack of a unified model requires us to handle it on a
//   "case by case" basis.  What we CAN provide, though, is the noise
//   gain associated with the 1/f source.
//
void
NevalSrc(double *noise, double *lnNoise, sCKT *ckt, int type, int node1,
    int node2, double param, double mult)
{
    double realVal = *(ckt->CKTrhs + node1) - *(ckt->CKTrhs + node2);
    double imagVal = *(ckt->CKTirhs + node1) - *(ckt->CKTirhs + node2);
    double gain = (realVal*realVal + imagVal*imagVal)*mult;

    switch (type) {
    case SHOTNOISE:
        // param is the dc current in a semiconductor
        *noise = gain * 2 * wrsCHARGE * fabs(param);
        *lnNoise = log(SPMAX(*noise,N_MINLOG) ); 
        break;

    case THERMNOISE:
        // param is the conductance of a resistor
        *noise = gain * 4 * wrsCONSTboltz * ckt->CKTcurTask->TSKtemp * param;
        *lnNoise = log(SPMAX(*noise, N_MINLOG));
        break;

    case N_GAIN:
        *noise = gain;
        break;
    }
}

