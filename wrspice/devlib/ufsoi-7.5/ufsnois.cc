
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Device Library                                                         *
 *                                                                        *
 *========================================================================*
 $Id: ufsnois.cc,v 2.12 2015/08/08 01:51:14 stevew Exp $
 *========================================================================*/

/**********
Copyright 1997 University of Florida.  All rights reserved.
Author: Min-Chie Jeng (For SPICE3E2)
File:  ufsnoi.c
**********/

#include <stdio.h>
#include "ufsdefs.h"
#include "noisdefs.h"


int
UFSdev::noise (int mode, int operation, sGENmodel *genmod, sCKT *ckt,
    sNdata *data, double *OnDens)
{
sUFSmodel *model = (sUFSmodel*)genmod;
sUFSinstance *here;
struct ufsTDModelData *pTempModel;                                                          /* 5.0 */
struct ufsAPI_ModelData *pModel;
struct ufsAPI_InstData *pInst;
struct ufsAPI_OPData *pOpInfo;
char ufsname[N_MXVLNTH];
double tempOutNoise;                                                                                 /* 4.5n */
double tempInNoise;                                                                                  /* 4.5n */
double noizDens[UFSNSRCS];
double lnNdens[UFSNSRCS];
double T0, q, esi, Sich, Iso, Ido, Psj, Pdj, Xnin, Weff, Leff, toc, Eg, Egbgn, Btrev, T2, Eo;        /* 5.0 */
double Tratio, Is, Id;                                                                               /* 5.0 */
/*
int error, i;
*/
int i;

    /* define the names of the noise sources */
    static const char *UFSnNames[UFSNSRCS] =
    {   /* Note that we have to keep the order */
	/* consistent with the index definitions */
	/* in UFSdef.h */
	".rd",              /* noise due to rd */
	".rs",              /* noise due to rs */
	".rb",              /* noise due to rb */
	".id",              /* thermal noise due to ich */
	".irgts",           /* shot noise at source junction */                                      /* 4.5n */
	".irgtd",           /* shot noise at drain junction */                                       /* 4.5n */
	".ibjt",            /* shot noise of transport current (Ibjt) */                             /* 4.5n */
	".1overf",          /* flicker (1/f) noise */
	""                  /* total transistor noise */
    };

    for (; model != NULL; model = model->UFSnextModel)
    {    pModel = model->pModel;
	 for (here = model->UFSinstances; here != NULL;
	      here = here->UFSnextInstance)
	 {    pOpInfo = here->pOpInfo;
	      pInst = here->pInst;
	      switch (operation)
	      {  case N_OPEN:
		     /* see if we have to to produce a summary report */
		     /* if so, name all the noise generators */
/*
		      if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0)
*/
		      if (((sNOISEAN*)ckt->CKTcurJob)->NStpsSm != 0)
		      {   switch (mode)
			  {  case N_DENS:
			          for (i = 0; i < UFSNSRCS; i++)
				  {    (void) sprintf(ufsname, "onoise.%s%s",
					              (char*)here->UFSname,
						      UFSnNames[i]);
/*
                                       data->namelist = (IFuid *) trealloc(
					     (char *) data->namelist,
					     (data->numPlots + 1)
					     * sizeof(IFuid));
                                       if (!data->namelist)
					   return(E_NOMEM);
		                       (*(SPfrontEnd->IFnewUid)) (ckt,
			                  &(data->namelist[data->numPlots++]),
			                  (IFuid) NULL, ufsname, UID_OTHER,
					  (GENERIC **) NULL);
*/

                            Realloc(&data->namelist, data->numPlots+1,
                                data->numPlots);
                            ckt->newUid(&data->namelist[data->numPlots++],
                                0, ufsname, UID_OTHER);

				       /* we've added one more plot */
			          }
			          break;
		             case INT_NOIZ:
			          for (i = 0; i < UFSNSRCS; i++)
				  {    (void) sprintf(ufsname, "onoise_total.%s%s",
						      (char*)here->UFSname,
						      UFSnNames[i]);
/*
                                       data->namelist = (IFuid *) trealloc(
					     (char *) data->namelist,
					     (data->numPlots + 1)
					     * sizeof(IFuid));
                                       if (!data->namelist)
					   return(E_NOMEM);
		                       (*(SPfrontEnd->IFnewUid)) (ckt,
			                  &(data->namelist[data->numPlots++]),
			                  (IFuid) NULL, ufsname, UID_OTHER,
					  (GENERIC **) NULL);
*/

                            Realloc(&data->namelist, data->numPlots+2,
                                data->numPlots);
                            ckt->newUid(&data->namelist[data->numPlots++],
                                0, ufsname, UID_OTHER);

				       /* we've added one more plot */

			               (void) sprintf(ufsname, "inoise_total.%s%s",
						      (char*)here->UFSname,
						      UFSnNames[i]);
/*
                                       data->namelist = (IFuid *) trealloc(
					     (char *) data->namelist,
					     (data->numPlots + 1)
					     * sizeof(IFuid));
                                       if (!data->namelist)
					   return(E_NOMEM);
		                       (*(SPfrontEnd->IFnewUid)) (ckt,
			                  &(data->namelist[data->numPlots++]),
			                  (IFuid) NULL, ufsname, UID_OTHER,
					  (GENERIC **)NULL);
*/

                            ckt->newUid(&data->namelist[data->numPlots++],
                                0, ufsname, UID_OTHER);

				       /* we've added one more plot */
			          }
			          break;
		          }
		      }
		      break;
	         case N_CALC:
		      switch (mode)
		      {  case N_DENS:
			   /* To look at calculated values with a debugger go to nevalsrc.c */

			      if(pModel->Selft > 0)                                                  /* 5.0 */
			      { Tratio = pInst->Temperature / ckt->CKTtemp;                          /* 5.0 */
			      }                                                                      /* 5.0 */
			      else                                                                   /* 5.0 */
			      { Tratio = 1.0;                                                        /* 5.0 */
			      }                                                                      /* 5.0 */
			      Sich = pOpInfo->Sich;                                                  /* 5.0 */
			      pTempModel = pInst->pTempModel;                                        /* 5.0 */
			      q = ELECTRON_CHARGE;                                                   /* 5.0 */
			      esi = SILICON_PERMITTIVITY;                                            /* 5.0 */
			      Weff = pInst->Weff;                                                    /* 5.0 */
			      Leff = pInst->Leff;                                                    /* 5.0 */
			      Psj = pInst->SourceJunctionPerimeter;                                  /* 5.0 */
			      Pdj = pInst->DrainJunctionPerimeter;                                   /* 5.0 */
			      Xnin = pTempModel->Xnin;                                               /* 5.0 */
			      Eg = pTempModel->Eg;                                                   /* 5.0 */
			      if (pTempModel->Tauo == 0.0)                                           /* 5.0 */
			      { Iso = pTempModel->Jro * Psj;                                         /* 5.0 */
			        Ido = pTempModel->Jro * Pdj;                                         /* 5.0 */
			      }                                                                      /* 5.0 */
			      else                                                                   /* 5.0 */
			      { Iso = q * Xnin * Psj *                                               /* 5.0 */
                                     (pModel->Tb * pTempModel->D0 / pTempModel->Taug +               /* 5.0 */
                                      pModel->Teff * pTempModel->D0h / pTempModel->Taugh);           /* 5.0 */
			        Ido = Iso / Psj * Pdj;                                               /* 5.0 */
			        Ido += q * Xnin * Weff * Leff * pModel->Tb / pTempModel->Taug;       /* 5.0 */
			      }                                                                      /* 5.0 */
			      if(pModel->Ntr > 0.0)                                                  /* 5.0 */
			      { toc = 1.0e-12;                                                       /* 5.0 */
				if((pModel->Lldd > 0.0) && (pModel->Thalo == 0.0))                   /* 5.0 */
				{ Egbgn = Eg - 0.0225 * sqrt(pModel->Nldd / 1.0e24);}                /* 5.0 */
				else                                                                 /* 5.0 */
				{ Egbgn = Eg - 0.0225 * sqrt(pModel->Nds  / 1.0e24);}                /* 5.0 */
				Btrev = 1.27e9 * pow((Egbgn / Eg),1.5);                              /* 5.0 */
				Eo = -2.0 * pTempModel->Vbih/ pTempModel->D0h;                       /* 5.0 */
				T0 = 5.0 * pModel->Teff * pModel->Ntr * esi /                        /* 5.0 */
				    (pModel->Nbheff * toc * Btrev);	                             /* 5.0 */
				T2 = Eo * Eo * exp(Btrev / Eo);                                      /* 5.0 */
				Iso += T0 * T2 * Psj;                                                /* 5.0 */
				Ido += T0 * T2 * Pdj;                                                /* 5.0 */
			      }                                                                      /* 5.0 */
			      Is = FABS(pOpInfo->Ir) + 2.0 * Iso;                                    /* 5.0 */
			      Id = FABS(pOpInfo->Igt) + 2.0 * Ido + FABS(pOpInfo->Igi);              /* 5.0 */
			      if(Is < 1.0e-20) Is = 1.0e-20;                                         /* 5.0 */
			      if(Id < 1.0e-20) Id = 1.0e-20;                                         /* 5.0 */

		              NevalSrc(&noizDens[UFSRDNOIZ],&lnNdens[UFSRDNOIZ],ckt,THERMNOISE,
				here->UFSdNodePrime,here->UFSdNode,pInst->DrainConductance*Tratio);  /* 5.0 */

		              NevalSrc(&noizDens[UFSRSNOIZ],&lnNdens[UFSRSNOIZ],ckt,THERMNOISE,
				here->UFSsNodePrime,here->UFSsNode,pInst->SourceConductance*Tratio); /* 5.0 */

			      if (pInst->BulkContact)
			      { NevalSrc(&noizDens[UFSRBNOIZ],&lnNdens[UFSRBNOIZ],ckt,THERMNOISE,
				  here->UFSbNodePrime,here->UFSbNode,pInst->BodyConductance*Tratio); /* 5.0 */
			      }
			      else
			      { NevalSrc(&noizDens[UFSRBNOIZ],&lnNdens[UFSRBNOIZ],ckt,THERMNOISE,    /* 5.0 */
				         here->UFSbNodePrime,here->UFSbNode,0.0);
			      }

		              NevalSrc(&noizDens[UFSIDNOIZ],&lnNdens[UFSIDNOIZ],ckt,SHOTNOISE,       /* 5.0 */
				       here->UFSdNodePrime,here->UFSsNodePrime,Sich/2.0/q);          /* 5.0 */

		              NevalSrc(&noizDens[UFSFLNOIZ],(double*) NULL,ckt,N_GAIN,
                                  here->UFSdNodePrime,here->UFSsNodePrime,(double) 0.0);
			      noizDens[UFSFLNOIZ] *= pModel->Fnk * exp(pModel->Fna *
				  log(MAX(FABS(pOpInfo->Ich),N_MINLOG))) /
				  (data->freq*pInst->Leff*pInst->Leff*pModel->Coxf);
		              lnNdens[UFSFLNOIZ] = log(MAX(noizDens[UFSFLNOIZ], N_MINLOG));

			      NevalSrc(&noizDens[UFSSJNOIZ],&lnNdens[UFSSJNOIZ],ckt,SHOTNOISE,       /* 5.0 */
				  here->UFSbNodePrime,here->UFSsNodePrime,Is);                       /* 5.0 */

			      NevalSrc(&noizDens[UFSDJNOIZ],&lnNdens[UFSDJNOIZ],ckt,SHOTNOISE,       /* 5.0 */
				  here->UFSbNodePrime,here->UFSdNodePrime,Id);                       /* 5.0 */

			      NevalSrc(&noizDens[UFSBJTNOIZ],&lnNdens[UFSBJTNOIZ],ckt,SHOTNOISE,     /* 5.0 */
				  here->UFSdNodePrime,here->UFSsNodePrime,FABS(pOpInfo->Ibjt));      /* 5.0 */

		              noizDens[UFSTOTNOIZ] = noizDens[UFSRDNOIZ]
						   + noizDens[UFSRSNOIZ]
						   + noizDens[UFSRBNOIZ]                             /* 4.5n */
						   + noizDens[UFSIDNOIZ]
					           + noizDens[UFSSJNOIZ]                             /* 4.5n */
					           + noizDens[UFSDJNOIZ]                             /* 4.5n */
					           + noizDens[UFSBJTNOIZ]                            /* 4.5n */
					           + noizDens[UFSFLNOIZ];
		              lnNdens[UFSTOTNOIZ] = log(MAX(noizDens[UFSTOTNOIZ], N_MINLOG));
		              *OnDens += noizDens[UFSTOTNOIZ];

		              if (data->delFreq == 0.0)
			      {   /* if we haven't done any previous
				     integration, we need to initialize our
				     "history" variables.
				    */

			          for (i = 0; i < UFSNSRCS; i++)
				  {    here->UFSnVar[LNLSTDENS][i] =
					     lnNdens[i];
			          }

			          /* clear out our integration variables
				     if it's the first pass
				   */
			          if (data->freq ==
/*
				      ((NOISEAN*) ckt->CKTcurJob)->NstartFreq)
*/
				      ((sNOISEAN*) ckt->CKTcurJob)->JOBac.fstart())
				  {   for (i = 0; i < UFSNSRCS; i++)
				      {    here->UFSnVar[OUTNOIZ][i] = 0.0;
				           here->UFSnVar[INNOIZ][i] = 0.0;
			              }
			          }
		              }
			      else
			      {   /* data->delFreq != 0.0,
				     we have to integrate.
				   */
			          for (i = 0; i < UFSNSRCS; i++)
				  {    if (i != UFSTOTNOIZ)
/*
				       {   tempOutNoise = Nintegrate(noizDens[i], */                    /* 4.5n */
/*
						lnNdens[i],
				                here->UFSnVar[LNLSTDENS][i],
						data);
*/

                       {   tempOutNoise = data->integrate(noizDens[i],
                        lnNdens[i], here->UFSnVar[LNLSTDENS][i]);

/*
				           tempInNoise = Nintegrate(noizDens[i] */                      /* 4.5n */
/*
						* data->GainSqInv, lnNdens[i]
						+ data->lnGainInv,
				                here->UFSnVar[LNLSTDENS][i]
						+ data->lnGainInv, data);
*/

                           tempInNoise = data->integrate(noizDens[i]
                        * data->GainSqInv, lnNdens[i]
                        + data->lnGainInv,
                                here->UFSnVar[LNLSTDENS][i]
                        + data->lnGainInv);

				           here->UFSnVar[LNLSTDENS][i] =
						lnNdens[i];
				           data->outNoiz += tempOutNoise;                            /* 4.5n */
				           data->inNoise += tempInNoise;                             /* 4.5n */
/*
				           if (((NOISEAN*)
*/
				           if (((sNOISEAN*)

					       ckt->CKTcurJob)->NStpsSm != 0)
					   {   here->UFSnVar[OUTNOIZ][i]
						     += tempOutNoise;                                /* 4.5n */
				               here->UFSnVar[OUTNOIZ][UFSTOTNOIZ]
						     += tempOutNoise;                                /* 4.5n */
				               here->UFSnVar[INNOIZ][i]
						     += tempInNoise;                                 /* 4.5n */
				               here->UFSnVar[INNOIZ][UFSTOTNOIZ]
						     += tempInNoise;                                 /* 4.5n */
                                           }
			               }
			          }
		              }
		              if (data->prtSummary)
			      {   for (i = 0; i < UFSNSRCS; i++)
				  {    /* print a summary report */
			               data->outpVector[data->outNumber++]
					     = noizDens[i];
			          }
		              }
		              break;
		         case INT_NOIZ:
			      /* already calculated, just output */
/*
		              if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0)
*/
		              if (((sNOISEAN*)ckt->CKTcurJob)->NStpsSm != 0)

			      {   for (i = 0; i < UFSNSRCS; i++)
				  {    data->outpVector[data->outNumber++]
					     = here->UFSnVar[OUTNOIZ][i];
			               data->outpVector[data->outNumber++]
					     = here->UFSnVar[INNOIZ][i];
			          }
		              }
		              break;
		      }
		      break;
	         case N_CLOSE:
		      /* do nothing, the main calling routine will close */
		      return (OK);
		      break;   /* the plots */
	      }       /* switch (operation) */
	 }    /* for here */
    }    /* for model */

    return(OK);
}
