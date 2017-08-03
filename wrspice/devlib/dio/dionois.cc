
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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


int
DIOdev::noise (int mode, int operation, sGENmodel *genmod, sCKT *ckt,
    sNdata *data, double *OnDens)
{
    sDIOmodel *firstModel = static_cast<sDIOmodel*>(genmod);
    sDIOmodel *model;
    sDIOinstance *inst;

    char dioname[N_MXVLNTH];
    double tempOnoise;
    double tempInoise;
    double noizDens[DIONSRCS];
    double lnNdens[DIONSRCS];
    int i;

    /* define the names of the noise sources */

    static const char *DIOnNames[DIONSRCS] = {       /* Note that we have to keep the order */
        "_rs",              /* noise due to rs */        /* consistent with thestrchr definitions */
        "_id",              /* noise due to id */        /* in DIOdefs.h */
        "_1overf",          /* flicker (1/f) noise */
        ""                  /* total diode noise */
    };

    for (model=firstModel; model != NULL; model=model->DIOnextModel) {
        for (inst=model->DIOinstances; inst != NULL; inst=inst->DIOnextInstance) {
//            if (inst->DIOowner != ARCHme) continue;

            switch (operation) {

            case N_OPEN:

                /* see if we have to to produce a summary report */
                /* if so, name all the noise generators */

                if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
                    switch (mode) {

                    case N_DENS:
                        for (i=0; i < DIONSRCS; i++) {
                            (void)sprintf(dioname,"onoise_%s%s",(char*)inst->DIOname,DIOnNames[i]);

/* SRW
                            data->namelist = (IFuid *)trealloc((char *)data->namelist,(data->numPlots + 1)*sizeof(IFuid));
                            if (!data->namelist) return(E_NOMEM);
                            (*(SPfrontEnd->IFnewUid))(ckt, &(data->namelist[data->numPlots++]),
                                                      (IFuid)NULL,dioname,UID_OTHER,(void **)NULL);
*/
                            Realloc(&data->namelist, data->numPlots+1,
                                data->numPlots);
                            ckt->newUid(&data->namelist[data->numPlots++],
                                0, dioname, UID_OTHER);

                                /* we've added one more plot */

                        }
                        break;

                    case INT_NOIZ:
                        for (i=0; i < DIONSRCS; i++) {
                            (void)sprintf(dioname,"onoise_total_%s%s",(char*)inst->DIOname,DIOnNames[i]);

/* SRW
                            data->namelist = (IFuid *)trealloc((char *)data->namelist,(data->numPlots + 1)*sizeof(IFuid));
                            if (!data->namelist) return(E_NOMEM);
                            (*(SPfrontEnd->IFnewUid))(ckt, &(data->namelist[data->numPlots++]),
                                                      (IFuid)NULL,dioname,UID_OTHER,(void **)NULL);
*/

                            Realloc(&data->namelist, data->numPlots+2,
                                data->numPlots);
                            ckt->newUid(&data->namelist[data->numPlots++],
                                0, dioname, UID_OTHER);

                                /* we've added one more plot */

                            (void)sprintf(dioname,"inoise_total_%s%s",(char*)inst->DIOname,DIOnNames[i]);

/* SRW
                            data->namelist = (IFuid *)trealloc((char *)data->namelist,(data->numPlots + 1)*sizeof(IFuid));
                            if (!data->namelist) return(E_NOMEM);
                            (*(SPfrontEnd->IFnewUid))(ckt, &(data->namelist[data->numPlots++]),
                                                      (IFuid)NULL,dioname,UID_OTHER,(void **)NULL);
*/
                            ckt->newUid(&data->namelist[data->numPlots++],
                                0, dioname, UID_OTHER);

                                /* we've added one more plot */

                        }
                        break;
                    }
                }
                break;

            case N_CALC:
                switch (mode) {

                case N_DENS:
                    NevalSrc(&noizDens[DIORSNOIZ],&lnNdens[DIORSNOIZ],
                                 ckt,THERMNOISE,inst->DIOposPrimeNode,inst->DIOposNode,
                                 inst->DIOtConductance * inst->DIOarea * inst->DIOm);
                    NevalSrc(&noizDens[DIOIDNOIZ],&lnNdens[DIOIDNOIZ],
                                 ckt,SHOTNOISE,inst->DIOposPrimeNode, inst->DIOnegNode,
                                 *(ckt->CKTstate0 + inst->DIOcurrent));

                    NevalSrc(&noizDens[DIOFLNOIZ],(double*)NULL,ckt,
                                 N_GAIN,inst->DIOposPrimeNode, inst->DIOnegNode,
                                 (double)0.0);
                    noizDens[DIOFLNOIZ] *= model->DIOfNcoef * 
                                 exp(model->DIOfNexp *
                                 log(MAX(fabs(*(ckt->CKTstate0 + inst->DIOcurrent)/inst->DIOm),N_MINLOG))) /
                                 data->freq * inst->DIOm;
                    lnNdens[DIOFLNOIZ] = 
                                 log(MAX(noizDens[DIOFLNOIZ],N_MINLOG));

                    noizDens[DIOTOTNOIZ] = noizDens[DIORSNOIZ] +
                                                    noizDens[DIOIDNOIZ] +
                                                    noizDens[DIOFLNOIZ];
                    lnNdens[DIOTOTNOIZ] = 
                                 log(MAX(noizDens[DIOTOTNOIZ], N_MINLOG));

                    *OnDens += noizDens[DIOTOTNOIZ];

                    if (data->delFreq == 0.0) { 

                        /* if we haven't done any previous integration, we need to */
                        /* initialize our "history" variables                      */

                        for (i=0; i < DIONSRCS; i++) {
                            inst->DIOnVar[LNLSTDENS][i] = lnNdens[i];
                        }

                        /* clear out our integration variables if it's the first pass */

                        if (data->freq == ((NOISEAN*)ckt->CKTcurJob)->NstartFreq) {
                            for (i=0; i < DIONSRCS; i++) {
                                inst->DIOnVar[OUTNOIZ][i] = 0.0;
                                inst->DIOnVar[INNOIZ][i] = 0.0;
                            }
                        }
                    } else {   /* data->delFreq != 0.0 (we have to integrate) */

/* To insure accurracy, we have to integrate each component separately */

                        for (i=0; i < DIONSRCS; i++) {
                            if (i != DIOTOTNOIZ) {
                                tempOnoise = Nintegrate(noizDens[i], lnNdens[i],
                                      inst->DIOnVar[LNLSTDENS][i], data);
                                tempInoise = Nintegrate(noizDens[i] * data->GainSqInv ,
                                      lnNdens[i] + data->lnGainInv,
                                      inst->DIOnVar[LNLSTDENS][i] + data->lnGainInv,
                                      data);
                                inst->DIOnVar[LNLSTDENS][i] = lnNdens[i];
                                data->outNoiz += tempOnoise;
                                data->inNoise += tempInoise;
                                if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
                                    inst->DIOnVar[OUTNOIZ][i] += tempOnoise;
                                    inst->DIOnVar[OUTNOIZ][DIOTOTNOIZ] += tempOnoise;
                                    inst->DIOnVar[INNOIZ][i] += tempInoise;
                                    inst->DIOnVar[INNOIZ][DIOTOTNOIZ] += tempInoise;
                                }
                            }
                        }
                    }
                    if (data->prtSummary) {
                        for (i=0; i < DIONSRCS; i++) {     /* print a summary report */
                            data->outpVector[data->outNumber++] = noizDens[i];
                        }
                    }
                    break;

                case INT_NOIZ:        /* already calculated, just output */
                    if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
                        for (i=0; i < DIONSRCS; i++) {
                            data->outpVector[data->outNumber++] = inst->DIOnVar[OUTNOIZ][i];
                            data->outpVector[data->outNumber++] = inst->DIOnVar[INNOIZ][i];
                        }
                    }    /* if */
                    break;
                }    /* switch (mode) */
                break;

            case N_CLOSE:
                return (OK);         /* do nothing, the main calling routine will close */
                break;               /* the plots */
            }    /* switch (operation) */
        }    /* for inst */
    }    /* for model */

return(OK);
}
            
