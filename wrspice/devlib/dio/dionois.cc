
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
 * WRspice Circuit Simulation and Analysis Tool:  Device Library          *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1987 Gary W. Ng
Modified by Dietmar Warning 2003
**********/

#include <stdio.h>
#include "diodefs.h"
#include "noisdefs.h"

#define DIOnextModel      next()
#define DIOnextInstance   next()
#define DIOinstances      inst()
#define DIOname GENname
#define MAX SPMAX
#define CKTtemp CKTcurTask->TSKtemp
#define NOISEAN sNOISEAN
#define Nintegrate(a, b, c, d) (d)->integrate(a, b, c)
#define NstartFreq JOBac.fstart()

// define the names of the noise sources
static const char *DIOnNames[DIONSRCS] = {
    // Note that we have to keep the order consistent with the index
    // definitions in DIOdefs.h
    //
    "_rs",      // noise due to rs
    "_id",      // noise due to id
    "_1overf",  // flicker (1/f) noise
    ""          // total diode noise
};


int
DIOdev::noise (int mode, int operation, sGENmodel *genmod, sCKT *ckt,
    sNdata *data, double *OnDens)
{
    char dioname[N_MXVLNTH];
    sDIOmodel *model = static_cast<sDIOmodel*>(genmod);
    if (operation == N_OPEN) {
        // see if we have to to produce a summary report
        // if so, name all the noise generators

        if (static_cast<NOISEAN*>(ckt->CKTcurJob)->NStpsSm == 0)
            return (OK);

        if (mode == N_DENS) {
            for ( ; model; model = model->next()) {
                sDIOinstance *inst;
                for (inst = model->inst(); inst; inst = inst->next()) {

                    for (int i = 0; i < DIONSRCS; i++) {
                        (void)snprintf(dioname, sizeof(dioname),
                            "onoise_%s%s", (char*)inst->DIOname,
                            DIOnNames[i]);

                        Realloc(&data->namelist, data->numPlots+1,
                            data->numPlots);
                        ckt->newUid(&data->namelist[data->numPlots++], 0,
                            dioname, UID_OTHER);
                        // we've added one more plot
                    }
                }
            }
            return (OK);
        }

        if (mode == INT_NOIZ) {
            for ( ; model; model = model->next()) {
                sDIOinstance *inst;
                for (inst = model->inst(); inst; inst = inst->next()) {

                    for (int i = 0; i < DIONSRCS; i++) {
                        (void)snprintf(dioname, sizeof(dioname),
                            "onoise_total_%s%s", (char*)inst->DIOname,
                            DIOnNames[i]);
                        Realloc(&data->namelist, data->numPlots+2,
                            data->numPlots);
                        ckt->newUid(&data->namelist[data->numPlots++], 0,
                            dioname, UID_OTHER);
                        // we've added one more plot
                        (void)snprintf(dioname, sizeof(dioname),
                            "inoise_total_%s%s", (char*)inst->DIOname,
                            DIOnNames[i]);
                        ckt->newUid(&data->namelist[data->numPlots++], 0,
                            dioname, UID_OTHER);
                        // we've added one more plot
                    }
                }
            }
        }
        return (OK);
    }

    if (operation == N_CALC) {
        if (mode == N_DENS) {
            for ( ; model; model = model->next()) {
                sDIOinstance *inst;
                for (inst = model->inst(); inst; inst = inst->next()) {

                    double noizDens[DIONSRCS];
                    double lnNdens[DIONSRCS];
                    NevalSrc(&noizDens[DIORSNOIZ], &lnNdens[DIORSNOIZ],
                        ckt, THERMNOISE, inst->DIOposPrimeNode,
                        inst->DIOposNode,
                        inst->DIOtConductance * inst->DIOarea * inst->DIOm);

                    NevalSrc(&noizDens[DIOIDNOIZ], &lnNdens[DIOIDNOIZ],
                        ckt, SHOTNOISE, inst->DIOposPrimeNode,
                        inst->DIOnegNode,
                        *(ckt->CKTstate0 + inst->DIOcurrent));

                    NevalSrc(&noizDens[DIOFLNOIZ], (double*)NULL, ckt,
                        N_GAIN, inst->DIOposPrimeNode, inst->DIOnegNode, 0.0);

                    noizDens[DIOFLNOIZ] *= model->DIOfNcoef * 
                                 exp(model->DIOfNexp *
                                 log(MAX(fabs(*(ckt->CKTstate0 +
                                 inst->DIOcurrent)/inst->DIOm),N_MINLOG))) /
                                 data->freq * inst->DIOm;

                    lnNdens[DIOFLNOIZ] = 
                                 log(MAX(noizDens[DIOFLNOIZ],N_MINLOG));

                    noizDens[DIOTOTNOIZ] = noizDens[DIORSNOIZ] +
                        noizDens[DIOIDNOIZ] + noizDens[DIOFLNOIZ];

                    lnNdens[DIOTOTNOIZ] = 
                                 log(MAX(noizDens[DIOTOTNOIZ], N_MINLOG));

                    *OnDens += noizDens[DIOTOTNOIZ];

                    if (data->delFreq == 0.0) { 
                        // if we haven't done any previousintegration,
                        // we need to initialize our "history" variables

                        for (int i = 0; i < DIONSRCS; i++)
                            inst->DIOnVar[LNLSTDENS][i] = lnNdens[i];

                        // clear out our integration variables if it's
                        // the first pass

                        if (data->freq == 
                                ((NOISEAN*)ckt->CKTcurJob)->NstartFreq) {
                            for (int i = 0; i < DIONSRCS; i++) {
                                inst->DIOnVar[OUTNOIZ][i] = 0.0;
                                inst->DIOnVar[INNOIZ][i] = 0.0;
                            }
                        }
                    }
                    else {
                        // data->delFreq != 0.0 (we have to integrate)

                        // To insure accurracy, we have to integrate
                        // each component separately

                        for (int i = 0; i < DIONSRCS; i++) {
                            if (i != DIOTOTNOIZ) {
                                double tempOnoise;
                                double tempInoise;

                                tempOnoise = data->integrate(noizDens[i],
                                    lnNdens[i], inst->DIOnVar[LNLSTDENS][i]);
                                tempInoise = data->integrate(noizDens[i]*
                                    data->GainSqInv ,
                                    lnNdens[i] + data->lnGainInv,
                                    inst->DIOnVar[LNLSTDENS][i] +
                                    data->lnGainInv);

                                inst->DIOnVar[LNLSTDENS][i] = lnNdens[i];
                                data->outNoiz += tempOnoise;
                                data->inNoise += tempInoise;

                                if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
                                    inst->DIOnVar[OUTNOIZ][i] += tempOnoise;
                                    inst->DIOnVar[OUTNOIZ][DIOTOTNOIZ] +=
                                        tempOnoise;
                                    inst->DIOnVar[INNOIZ][i] += tempInoise;
                                    inst->DIOnVar[INNOIZ][DIOTOTNOIZ] +=
                                        tempInoise;
                                }
                            }
                        }
                    }
                    if (data->prtSummary) {
                        for (int i = 0; i < DIONSRCS; i++) {
                            // print a summary report
                            data->outpVector[data->outNumber++] = noizDens[i];
                        }
                    }
                }
            }
            return (OK);
        }

        if (mode == INT_NOIZ) {
            // already calculated, just output
            if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm == 0)
                return (OK);
            for ( ; model; model = model->next()) {
                sDIOinstance *inst;
                for (inst = model->inst(); inst; inst = inst->next()) {
                    for (int i = 0; i < DIONSRCS; i++) {
                        data->outpVector[data->outNumber++] =
                            inst->DIOnVar[OUTNOIZ][i];
                        data->outpVector[data->outNumber++] =
                            inst->DIOnVar[INNOIZ][i];
                    }
                }
            }
        }
    }
    return(OK);
}

