
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
Model Author: 1995 Colin McAndrew Motorola
Spice3 Implementation: 2003 Dietmar Warning DAnalyse GmbH
**********/

#include <stdio.h>
#include "vbicdefs.h"
#include "noisdefs.h"

#define VBICnextModel      next()
#define VBICnextInstance   next()
#define VBICinstances      inst()
#define VBICname GENname
#define MAX SPMAX
#define CKTtemp CKTcurTask->TSKtemp
#define NOISEAN sNOISEAN
#define Nintegrate(a, b, c, d) (d)->integrate(a, b, c)
#define NstartFreq JOBac.fstart()


int
VBICdev::noise (int mode, int operation, sGENmodel *genmod, sCKT *ckt,
    sNdata *data, double *OnDens)
{
    sVBICmodel *firstModel = static_cast<sVBICmodel*>(genmod);
    sVBICmodel *model;
    sVBICinstance *inst;

    char vbicname[N_MXVLNTH];
    double tempOnoise;
    double tempInoise;
    double noizDens[VBICNSRCS];
    double lnNdens[VBICNSRCS];
    int i;

    /* define the names of the noise sources */

    static const char *VBICnNames[VBICNSRCS] = {
        /* Note that we have to keep the order consistent with the
          strchr definitions in VBICdefs.h */
        "_rc",              /* noise due to rc */
        "_rci",             /* noise due to rci */
        "_rb",              /* noise due to rb */
        "_rbi",             /* noise due to rbi */
        "_re",              /* noise due to re */
        "_rbp",             /* noise due to rbp */
        "_ic",              /* noise due to ic */
        "_ib",              /* noise due to ib */
        "_ibep",            /* noise due to ibep */
        "_1overfbe",        /* flicker (1/f) noise ibe */
        "_1overfbep",       /* flicker (1/f) noise ibep */
        "_rs",              /* noise due to rs */
        "_iccp",            /* noise due to iccp */
        ""                  /* total transistor noise */
    };

    for (model=firstModel; model != NULL; model=model->VBICnextModel) {
        for (inst=model->VBICinstances; inst != NULL;
                inst=inst->VBICnextInstance) {

//            if (inst->VBICowner != ARCHme) continue;

            switch (operation) {

            case N_OPEN:

                /* see if we have to to produce a summary report */
                /* if so, name all the noise generators */

                if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
                    switch (mode) {

                    case N_DENS:
                        for (i=0; i < VBICNSRCS; i++) {
                            (void)sprintf(vbicname,"onoise_%s%s",
                                (char*)inst->VBICname,VBICnNames[i]);


/* SRW
                        data->namelist = (IFuid *)
                                trealloc((char *)data->namelist,
                                (data->numPlots + 1)*sizeof(IFuid));
                        if (!data->namelist) return(E_NOMEM);
                        (*(SPfrontEnd->IFnewUid))(ckt,
                            &(data->namelist[data->numPlots++]),
                            (IFuid)NULL,vbicname,UID_OTHER,(void **)NULL);
*/
                            Realloc(&data->namelist, data->numPlots+1,
                                data->numPlots);
                            ckt->newUid(&data->namelist[data->numPlots++],
                                0, vbicname, UID_OTHER);

                                /* we've added one more plot */
                        }
                        break;

                    case INT_NOIZ:
                        for (i=0; i < VBICNSRCS; i++) {
                            (void)sprintf(vbicname,"onoise_total_%s%s",
                                (char*)inst->VBICname,VBICnNames[i]);

/* SRW
                        data->namelist = (IFuid *)
                                trealloc((char *)data->namelist,
                                (data->numPlots + 1)*sizeof(IFuid));
                        if (!data->namelist) return(E_NOMEM);
                        (*(SPfrontEnd->IFnewUid))(ckt,
                            &(data->namelist[data->numPlots++]),
                            (IFuid)NULL,vbicname,UID_OTHER,(void **)NULL);
*/
                            Realloc(&data->namelist, data->numPlots+2,
                                data->numPlots);
                            ckt->newUid(&data->namelist[data->numPlots++],
                                0, vbicname, UID_OTHER);

                                /* we've added one more plot */

                            (void)sprintf(vbicname,"inoise_total_%s%s",
                                (char*)inst->VBICname,VBICnNames[i]);

/* SRW
data->namelist = (IFuid *)trealloc((char *)data->namelist,(data->numPlots + 1)*sizeof(IFuid));
if (!data->namelist) return(E_NOMEM);
                (*(SPfrontEnd->IFnewUid))(ckt,
                        &(data->namelist[data->numPlots++]),
                        (IFuid)NULL,vbicname,UID_OTHER,(void **)NULL);
*/
                            ckt->newUid(&data->namelist[data->numPlots++],
                                0, vbicname, UID_OTHER);

                                /* we've added one more plot */
                        }
                        break;
                    }
                }
                break;

            case N_CALC:
                switch (mode) {

                case N_DENS:
                    NevalSrc(&noizDens[VBICRCNOIZ],&lnNdens[VBICRCNOIZ],
                                 ckt,THERMNOISE,inst->VBICcollCXNode,inst->VBICcollNode,
                                 model->VBICcollectorConduct * inst->VBICarea * inst->VBICm);

                    NevalSrc(&noizDens[VBICRCINOIZ],&lnNdens[VBICRCINOIZ],
                                 ckt,THERMNOISE,inst->VBICcollCXNode,inst->VBICcollCINode,
                                 *(ckt->CKTstate0 + inst->VBICirci_Vrci));

                    NevalSrc(&noizDens[VBICRBNOIZ],&lnNdens[VBICRBNOIZ],
                                 ckt,THERMNOISE,inst->VBICbaseBXNode,inst->VBICbaseNode,
                                 model->VBICbaseConduct * inst->VBICarea * inst->VBICm);

                    NevalSrc(&noizDens[VBICRBINOIZ],&lnNdens[VBICRBINOIZ],
                                 ckt,THERMNOISE,inst->VBICbaseBXNode,inst->VBICbaseBINode,
                                 *(ckt->CKTstate0 + inst->VBICirbi_Vrbi));

                    NevalSrc(&noizDens[VBICRENOIZ],&lnNdens[VBICRENOIZ],
                                 ckt,THERMNOISE,inst->VBICemitEINode,inst->VBICemitNode,
                                 model->VBICemitterConduct * inst->VBICarea * inst->VBICm);

                    NevalSrc(&noizDens[VBICRBPNOIZ],&lnNdens[VBICRBPNOIZ],
                                 ckt,THERMNOISE,inst->VBICemitEINode,inst->VBICemitNode,
                                 *(ckt->CKTstate0 + inst->VBICirbp_Vrbp));

                    NevalSrc(&noizDens[VBICRSNOIZ],&lnNdens[VBICRSNOIZ],
                                 ckt,THERMNOISE,inst->VBICsubsSINode,inst->VBICsubsNode,
                                 model->VBICsubstrateConduct * inst->VBICarea * inst->VBICm);


                    NevalSrc(&noizDens[VBICICNOIZ],&lnNdens[VBICICNOIZ],
                                 ckt,SHOTNOISE,inst->VBICcollCINode, inst->VBICemitEINode,
                                 *(ckt->CKTstate0 + inst->VBICitzf));

                    NevalSrc(&noizDens[VBICIBNOIZ],&lnNdens[VBICIBNOIZ],
                                 ckt,SHOTNOISE,inst->VBICbaseBINode, inst->VBICemitEINode,
                                 *(ckt->CKTstate0 + inst->VBICibe));

                    NevalSrc(&noizDens[VBICIBEPNOIZ],&lnNdens[VBICIBEPNOIZ],
                                 ckt,SHOTNOISE,inst->VBICbaseBXNode, inst->VBICbaseBPNode,
                                 *(ckt->CKTstate0 + inst->VBICibep));

                    NevalSrc(&noizDens[VBICICCPNOIZ],&lnNdens[VBICICCPNOIZ],
                                 ckt,SHOTNOISE,inst->VBICbaseBXNode, inst->VBICsubsSINode,
                                 *(ckt->CKTstate0 + inst->VBICiccp));


                    NevalSrc(&noizDens[VBICFLBENOIZ],(double*)NULL,ckt,
                                 N_GAIN,inst->VBICbaseBINode, inst->VBICemitEINode,
                                 (double)0.0);
                    noizDens[VBICFLBENOIZ] *= inst->VBICm * model->VBICfNcoef * 
                                 exp(model->VBICfNexpA *
                                 log(MAX(fabs(*(ckt->CKTstate0 + inst->VBICibe)/inst->VBICm),N_MINLOG))) /
                                 pow(data->freq, model->VBICfNexpB);
                    lnNdens[VBICFLBENOIZ] = 
                                 log(MAX(noizDens[VBICFLBENOIZ],N_MINLOG));

                    NevalSrc(&noizDens[VBICFLBEPNOIZ],(double*)NULL,ckt,
                                 N_GAIN,inst->VBICbaseBXNode, inst->VBICbaseBPNode,
                                 (double)0.0);
                    noizDens[VBICFLBEPNOIZ] *= inst->VBICm * model->VBICfNcoef * 
                                 exp(model->VBICfNexpA *
                                 log(MAX(fabs(*(ckt->CKTstate0 + inst->VBICibep)/inst->VBICm),N_MINLOG))) /
                                 pow(data->freq, model->VBICfNexpB);
                    lnNdens[VBICFLBEPNOIZ] = 
                                 log(MAX(noizDens[VBICFLBEPNOIZ],N_MINLOG));


                    noizDens[VBICTOTNOIZ] = noizDens[VBICRCNOIZ] +
                                            noizDens[VBICRCINOIZ] +
                                            noizDens[VBICRBNOIZ] +
                                            noizDens[VBICRBINOIZ] +
                                            noizDens[VBICRENOIZ] +
                                            noizDens[VBICRBPNOIZ] +
                                            noizDens[VBICICNOIZ] +
                                            noizDens[VBICIBNOIZ] +
                                            noizDens[VBICIBEPNOIZ] +
                                            noizDens[VBICFLBENOIZ] +
                                            noizDens[VBICFLBEPNOIZ];

                    lnNdens[VBICTOTNOIZ] = 
                                 log(noizDens[VBICTOTNOIZ]);

                    *OnDens += noizDens[VBICTOTNOIZ];

                    if (data->delFreq == 0.0) { 

                        /* if we haven't done any previous integration, we need to */
                        /* initialize our "history" variables                      */

                        for (i=0; i < VBICNSRCS; i++) {
                            inst->VBICnVar[LNLSTDENS][i] = lnNdens[i];
                        }

                        /* clear out our integration variables if it's the first pass */

                        if (data->freq == ((NOISEAN*)ckt->CKTcurJob)->NstartFreq) {
                            for (i=0; i < VBICNSRCS; i++) {
                                inst->VBICnVar[OUTNOIZ][i] = 0.0;
                                inst->VBICnVar[INNOIZ][i] = 0.0;
                            }
                        }
                    } else {   /* data->delFreq != 0.0 (we have to integrate) */

/* In order to get the best curve fit, we have to integrate each component separately */

                        for (i=0; i < VBICNSRCS; i++) {
                            if (i != VBICTOTNOIZ) {
                                tempOnoise = Nintegrate(noizDens[i], lnNdens[i],
                                      inst->VBICnVar[LNLSTDENS][i], data);
                                tempInoise = Nintegrate(noizDens[i] * data->GainSqInv ,
                                      lnNdens[i] + data->lnGainInv,
                                      inst->VBICnVar[LNLSTDENS][i] + data->lnGainInv,
                                      data);
                                inst->VBICnVar[LNLSTDENS][i] = lnNdens[i];
                                data->outNoiz += tempOnoise;
                                data->inNoise += tempInoise;
                                if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
                                    inst->VBICnVar[OUTNOIZ][i] += tempOnoise;
                                    inst->VBICnVar[OUTNOIZ][VBICTOTNOIZ] += tempOnoise;
                                    inst->VBICnVar[INNOIZ][i] += tempInoise;
                                    inst->VBICnVar[INNOIZ][VBICTOTNOIZ] += tempInoise;
                                }
                            }
                        }
                    }
                    if (data->prtSummary) {
                        for (i=0; i < VBICNSRCS; i++) {     /* print a summary report */
                            data->outpVector[data->outNumber++] = noizDens[i];
                        }
                    }
                    break;

                case INT_NOIZ:        /* already calculated, just output */
                    if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
                        for (i=0; i < VBICNSRCS; i++) {
                            data->outpVector[data->outNumber++] = inst->VBICnVar[OUTNOIZ][i];
                            data->outpVector[data->outNumber++] = inst->VBICnVar[INNOIZ][i];
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

