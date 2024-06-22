
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
Copyright 1997 University of Florida.  All rights reserved.
Author: Min-Chie Jeng (For SPICE3E2)
File: ufsld.c
**********/

#include "ufsdefs.h"
#include "stdio.h"

inline int
NIintegrate(sCKT *ckt, double *g, double *c, double gm, int st)
{
    ckt->integrate(g, c, gm, st);
    return (OK);
}
       

// This function is the main workhorse for transient and dc analysis.
// The instance internal code is executed here, and the values for
// the matrix and RHS are computed and loaded.


int
// SRW UFSdev::load(sGENmodel *genmod, sCKT *ckt)
UFSdev::load(sGENinstance *in_inst, sCKT *ckt)
{
// SRW sUFSmodel *model = (sUFSmodel*)genmod;
// SRW sUFSinstance *here;
sUFSinstance *here = (sUFSinstance*)in_inst;
sUFSmodel *model = (sUFSmodel*)here->GENmodPtr;;
struct ufsAPI_ModelData *pModel;
struct ufsAPI_InstData *pInst;
struct ufsAPI_OPData *pOpInfo;
struct ufsTDModelData *pTempModel;

double vbs=0.0, vgfs, vds, vbd, vgfd, vgfdo, delvgfs, delvds, delvbd, delvbs;
double delvgfd, cdhat, cbhat, /*cghat,*/ T, delt, xfact, Tprev, vgbs, Vgbs, gdpr, gspr;    /* 7.0Y */
#ifndef NEWCONV
double cghat;
#endif
double gcggb, gcgdb, gcgsb, gcgbb, gcdgb, gcddb, gcdsb, gcdbb, tempv;
double gcsgb, gcsdb, gcssb, gcsbb, gcbgb, gcbdb, gcbsb, gcbbb;
double qgate, qdrn, qsrc, qbulk, /*vgbso,*/ Cth, vgbd, delvgbd, delvgbs;
double gcgtb, gcdtb, gcstb, gcbtb, gcth, /*grth, Vbd,*/ ceqqs, vgbdo;
double geq, ceq, ceqqg, ceqqd, ceqqb, cqgate, cqdrn, cqbulk/*, cqsrc*/;
double dId_dVg, dId_dVd, dId_dVs, dId_dVb, dId_dT, dP_dT, dP_dVg;
double dIs_dVg, dIs_dVd, dIs_dVs, dIs_dVb, dIs_dT, dP_dVd, dP_dVs;
double dIgf_dVg, dIgf_dVd, dIgf_dVs, dIgf_dVb, dIgf_dT;                                /* 7.0Y */
double dIb_dVg, dIb_dVd, dIb_dVs, dIb_dVb, dIb_dT, dP_dVb, gbpr, Qt, cqt, ceqqt;
double /*vgb,*/ Vds, Vgfs, Vbs, ag0, ceqg, ceqd, ceqs, ceqb, ceqp/*, ceqgb*/;
double cqback, T0, /*T1, T2,*/ Cgfdo, Cgfso, Cgfbo, ceqqgb;                     
double Cgbbo, Qgfdo, Qgfso, Qgfbo, Qgbbo, Csg, Csd, Csb, Csgb, qback;
double gcggbb, gcdgbb, gcbgbb, gcgbgbb, gcgbgb, gcgbdb, gcgbbb;
double gcgbtb, gcsgbb, gcgbsb, dId_dVgb, dIb_dVgb, dIs_dVgb, dIgf_dVgb, dP_dVgb;       /* 7.0Y */
double Vthf, Vthb, Idtot, Ibtot, Igtot, Weff, Leff, Vtemp, Qnqff, Qnqfb;               /* 7.0Y */
int SH;                                                                                /* 4.5 */
struct ufsAPI_EnvData Env;
char *DevName;
//double Rhs[6], Gmat[6][6]; 
//int NodeNum[6];

int ByPass, Check, DynamicNeeded, /*J,*/ error, /*I,*/ ACNeeded;                         /* 4.5d */
Env.Temperature = ckt->CKTtemp;
Env.Tnom = ckt->CKTnomTemp;
Env.Gmin = ckt->CKTgmin;

/* SRW
DynamicNeeded =  ((ckt->CKTmode & (MODEAC | MODETRAN | MODEINITSMSIG)) ||
                 ((ckt->CKTmode & MODETRANOP) && (ckt->CKTmode & MODEUIC)))
                 ? 1 : 0;
*/
DynamicNeeded = ckt->CKTchargeCompNeeded;
ACNeeded = (ckt->CKTmode & (MODEDCOP)) ? 1: 0;                                   /* 5.0 */

/* SRW
for (; model != NULL; model = model->UFSnextModel)
{    pModel = model->pModel;
     for (here = model->UFSinstances; here != NULL; 
          here = here->UFSnextInstance)
     {
*/
     pModel = model->pModel;
          Check = 1;
          ByPass = 0;

	  pInst = here->pInst;
	  pOpInfo = here->pOpInfo;
	  pTempModel = pInst->pTempModel;
	  Weff = pInst->Weff;
	  Leff = pInst->Leff;
          Vthf = pInst->Vthf;                                                    /* 4.51 */
          Vthb = pInst->Vthb;                                                    /* 4.51 */

          if ((ckt->CKTmode & MODEINITSMSIG))
	  {   vbs = *(ckt->CKTstate0 + here->UFSvbs);
              vgfs = *(ckt->CKTstate0 + here->UFSvgfs);
              vds = *(ckt->CKTstate0 + here->UFSvds);
              vgbs = *(ckt->CKTstate0 + here->UFSvgbs);
              T = *(ckt->CKTstate0 + here->UFStemp);
	      if (pModel->Selft == 1)
	      {   Vtemp = T + ckt->CKTtemp;
		  SH = 1;                                                        /* 4.5 */
	          ufsTempEffect(pModel, pInst, &Env, Vtemp, SH);                 /* 4.5 */
	      }
          }
	  else if ((ckt->CKTmode & MODEINITTRAN))
	  {   vbs = *(ckt->CKTstate1 + here->UFSvbs);
              vgfs = *(ckt->CKTstate1 + here->UFSvgfs);
              vds = *(ckt->CKTstate1 + here->UFSvds);
              vgbs = *(ckt->CKTstate1 + here->UFSvgbs);
              T = *(ckt->CKTstate1 + here->UFStemp);
	      if (pModel->Selft == 1)
	      {   Vtemp = T + ckt->CKTtemp;
		  SH = 1;                                                        /* 4.5 */
	          ufsTempEffect(pModel, pInst, &Env, Vtemp, SH);                 /* 4.5 */
	      }
          }
	  else if ((ckt->CKTmode & MODEINITJCT) && !here->UFSoff)
	  {   vds = pModel->Type * here->UFSicVDS;
              vgfs = pModel->Type * here->UFSicVGFS;
              vbs = pModel->Type * here->UFSicVBS;
              vgbs = pModel->Type * here->UFSicVGBS;
              T = 0.0;

              if ((vds == 0.0) && (vgfs == 0.0) && (vbs == 0.0) && (vgbs == 0.0)
                  && ((ckt->CKTmode & (MODETRAN | MODEAC|MODEDCOP |
                   MODEDCTRANCURVE)) || (!(ckt->CKTmode & MODEUIC))))
	      {   vbs = 0.0;
                  vgfs = Vthf + 0.5;
                  vgbs = 0.0;
                  vds = 0.1;
              }
	      pOpInfo->Vbs = vbs;
          }
	  else if ((ckt->CKTmode & (MODEINITJCT | MODEINITFIX)) && 
                  (here->UFSoff)) 
          {    T = vgfs = vgbs = vds = 0.0;
	       pOpInfo->Vbs = vbs;
	  }
          else
	  {
#ifndef PREDICTOR
               if ((ckt->CKTmode & MODEINITPRED))
	       {   xfact = ckt->CKTdelta / ckt->CKTdeltaOld[1];
                   *(ckt->CKTstate0 + here->UFSvbs) = 
                         *(ckt->CKTstate1 + here->UFSvbs);
                   vbs = (1.0 + xfact)* (*(ckt->CKTstate1 + here->UFSvbs))
                       - (xfact * (*(ckt->CKTstate2 + here->UFSvbs)));
                   *(ckt->CKTstate0 + here->UFSvgfs) = 
                         *(ckt->CKTstate1 + here->UFSvgfs);
                   vgfs = (1.0 + xfact)* (*(ckt->CKTstate1 + here->UFSvgfs))
                         - (xfact * (*(ckt->CKTstate2 + here->UFSvgfs)));
                   *(ckt->CKTstate0 + here->UFSvds) = 
                         *(ckt->CKTstate1 + here->UFSvds);
                   vds = (1.0 + xfact)* (*(ckt->CKTstate1 + here->UFSvds))
                         - (xfact * (*(ckt->CKTstate2 + here->UFSvds)));
                   *(ckt->CKTstate0 + here->UFSvgbs) = 
                         *(ckt->CKTstate1 + here->UFSvgbs);
                   vgbs = (1.0 + xfact)* (*(ckt->CKTstate1 + here->UFSvgbs))
                         - (xfact * (*(ckt->CKTstate2 + here->UFSvgbs)));
	           if (pModel->Selft == 1)
	           {   T = *(ckt->CKTstate1 + here->UFStemp);
		       Vtemp = T + ckt->CKTtemp;
		       SH = 1;                                                   /* 4.5 */
                       ufsTempEffect(pModel, pInst, &Env, Vtemp, SH);            /* 4.5 */
	           }

                   *(ckt->CKTstate0 + here->UFStemp) = 
                         *(ckt->CKTstate1 + here->UFStemp);
                   T = (1.0 + xfact)* (*(ckt->CKTstate1 + here->UFStemp))
                     -(xfact * (*(ckt->CKTstate2 + here->UFStemp)));
		   /* MCJ */
                   *(ckt->CKTstate0 + here->UFSvbd) = 
                         *(ckt->CKTstate0 + here->UFSvbs)
                         - *(ckt->CKTstate0 + here->UFSvds);
               }
	       else
	       {
#endif /* PREDICTOR */
                   vbs = pModel->Type
                       * (*(ckt->CKTrhsOld + here->UFSbNodePrime)
                       - *(ckt->CKTrhsOld + here->UFSsNodePrime));
                   vgfs = pModel->Type
                       * (*(ckt->CKTrhsOld + here->UFSgNode)               
                       - *(ckt->CKTrhsOld + here->UFSsNodePrime));
                   vds = pModel->Type
                       * (*(ckt->CKTrhsOld + here->UFSdNodePrime)
                       - *(ckt->CKTrhsOld + here->UFSsNodePrime));
                   vgbs = pModel->Type
                       * (*(ckt->CKTrhsOld + here->UFSbgNode)
                       - *(ckt->CKTrhsOld + here->UFSsNodePrime));
                   T = pModel->Type * (*(ckt->CKTrhsOld + here->UFStNode));
#ifndef PREDICTOR
               }
#endif /* PREDICTOR */

               vbd = vbs - vds;
               vgfd = vgfs - vds;
               vgbd = vgbs - vds;

               delvbs = vbs - *(ckt->CKTstate0 + here->UFSvbs);
               delvbd = vbd - *(ckt->CKTstate0 + here->UFSvbd);
               delvgfs = vgfs - *(ckt->CKTstate0 + here->UFSvgfs);
               delvgbs = vgbs - *(ckt->CKTstate0 + here->UFSvgbs);
               delvds = vds - *(ckt->CKTstate0 + here->UFSvds);

               vgfdo = *(ckt->CKTstate0 + here->UFSvgfs)
                     - *(ckt->CKTstate0 + here->UFSvds);
               delvgfd = vgfd - vgfdo;

               vgbdo = *(ckt->CKTstate0 + here->UFSvgbs)
                     - *(ckt->CKTstate0 + here->UFSvds);
               delvgbd = vgbd - vgbdo;

	       Idtot = pOpInfo->Ich + pOpInfo->Ibjt + pOpInfo->Igi
		     - pOpInfo->Igt;
               Ibtot = pOpInfo->Igt + pOpInfo->Ir - pOpInfo->Igi
                     - pOpInfo->Igb;                                                     /* 7.0Y */
               Igtot = pOpInfo->Igb;                                                     /* 7.0Y */

               if (pInst->Mode >= 0) 
	       {   cdhat = Idtot + (pOpInfo->dId_dVgf + pOpInfo->dIgi_dVgf
			 - pOpInfo->dIgt_dVgf) * delvgfs
			 + (pOpInfo->dId_dVd + pOpInfo->dIgi_dVd 
			 - pOpInfo->dIgt_dVd) * delvds
			 + (pOpInfo->dId_dVb + pOpInfo->dIgi_dVb 
			 - pOpInfo->dIgt_dVb) * delvbs
			 + (pOpInfo->dId_dVgb + pOpInfo->dIgi_dVgb 
			 - pOpInfo->dIgt_dVgb) * delvgbs;
                   cbhat = Ibtot
                         + (pOpInfo->dIgt_dVgf - pOpInfo->dIgi_dVgf
                         - pOpInfo->dIgb_dVgf) * delvgfs
                         + (pOpInfo->dIgt_dVd - pOpInfo->dIgi_dVd
                         - pOpInfo->dIgb_dVd) * delvds
                         + (pOpInfo->dIgt_dVb + pOpInfo->dIr_dVb
                         - pOpInfo->dIgi_dVb - pOpInfo->dIgb_dVb) * delvbs
                         + (pOpInfo->dIgt_dVgb - pOpInfo->dIgi_dVgb
                         - pOpInfo->dIgb_dVgb) * delvgbs;                               /* 7.0Y */
#ifndef NEWCONV
                   cghat = Igtot                                                         /* 7.0Y */
                         + (pOpInfo->dIgb_dVgf) * delvgfs                               /* 7.0Y */
                         + (pOpInfo->dIgb_dVd) * delvds                                 /* 7.0Y */
                         + (pOpInfo->dIgb_dVb) * delvbs                                 /* 7.0Y */
                         + (pOpInfo->dIgb_dVgb) * delvgbs;                              /* 7.0Y */
#endif
	       }
               else
	       {   cdhat = Idtot + (pOpInfo->dId_dVgf + pOpInfo->dIgi_dVgf
			 - pOpInfo->dIgt_dVgf) * delvgfd
			 - (pOpInfo->dId_dVd + pOpInfo->dIgi_dVd 
			 - pOpInfo->dIgt_dVd) * delvds
			 + (pOpInfo->dId_dVb + pOpInfo->dIgi_dVb 
			 - pOpInfo->dIgt_dVb) * delvbd
			 + (pOpInfo->dId_dVgb + pOpInfo->dIgi_dVgb 
			 - pOpInfo->dIgt_dVgb) * delvgbd;
                   cbhat = Ibtot
                         + (pOpInfo->dIgt_dVgf - pOpInfo->dIgi_dVgf
                         - pOpInfo->dIgb_dVgf) * delvgfd
                         - (pOpInfo->dIgt_dVd - pOpInfo->dIgi_dVd 
                         - pOpInfo->dIgb_dVd) * delvds
                         + (pOpInfo->dIgt_dVb + pOpInfo->dIr_dVb
                         - pOpInfo->dIgi_dVb - pOpInfo->dIgb_dVb) * delvbd
                         + (pOpInfo->dIgt_dVgb - pOpInfo->dIgi_dVgb
                         - pOpInfo->dIgb_dVgb) * delvgbd;                               /* 7.0Y */
#ifndef NEWCONV
                   cghat = Igtot                                                         /* 7.0Y */
                         + (pOpInfo->dIgb_dVgf) * delvgfd                               /* 7.0Y */
                         - (pOpInfo->dIgb_dVd) * delvds                                 /* 7.0Y */
                         + (pOpInfo->dIgb_dVb) * delvbd                                 /* 7.0Y */
                         + (pOpInfo->dIgb_dVgb) * delvgbd;                              /* 7.0Y */
#endif

	       }
	       if (pModel->Selft > 1)
	       {   delt = T - *(ckt->CKTstate0 + here->UFStemp);
	           cdhat += (pOpInfo->dId_dT + pOpInfo->dIgi_dT
			 - pOpInfo->dIgt_dT) * delt; 
                   cbhat += (pOpInfo->dIgt_dT + pOpInfo->dIr_dT
                         - pOpInfo->dIgi_dT - pOpInfo->dIgb_dT) * delt;                 /* 7.0Y */
#ifndef NEWCONV
                   cghat += (pOpInfo->dIgb_dT) * delt;                                  /* 7.0Y */
#endif
	       }
	       else
	       {   delt = 0.0;
	       }

#ifndef NOBYPASS
           /* following should be one big if connected by && all over
            * the place, but some C compilers can't handle that, so
            * we split it up here to let them digest it in stages
            */

               if ((!(ckt->CKTmode & MODEINITPRED)) && (ckt->CKTbypass))
               if ((FABS(delvbs) < (ckt->CKTreltol * MAX(FABS(vbs),
                   FABS(*(ckt->CKTstate0+here->UFSvbs))) + ckt->CKTvoltTol)))
               if ((FABS(delvbd) < (ckt->CKTreltol * MAX(FABS(vbd),
                   FABS(*(ckt->CKTstate0+here->UFSvbd))) + ckt->CKTvoltTol)))
               if ((FABS(delvgfs) < (ckt->CKTreltol * MAX(FABS(vgfs),
                   FABS(*(ckt->CKTstate0+here->UFSvgfs))) + ckt->CKTvoltTol)))
               if ((FABS(delvgbs) < (ckt->CKTreltol * MAX(FABS(vgbs),
                   FABS(*(ckt->CKTstate0+here->UFSvgbs))) + ckt->CKTvoltTol)))
               if ((FABS(delvds) < (ckt->CKTreltol * MAX(FABS(vds),
                   FABS(*(ckt->CKTstate0+here->UFSvds))) + ckt->CKTvoltTol)))
               if ((FABS(cdhat - Idtot) < ckt->CKTreltol 
                   * MAX(FABS(cdhat),FABS(Idtot)) + ckt->CKTabstol))
	       {   tempv = MAX(FABS(cbhat),FABS(Ibtot)) + ckt->CKTabstol;
                   T0 = ckt->CKTreltol * MAX(FABS(T), FABS(*(ckt->CKTstate0+here->UFStemp))) + 1.0e4 * ckt->CKTvoltTol;
                   if ((FABS(cbhat - Ibtot) < ckt->CKTreltol * tempv) &&
                       (FABS(delt) < T0))
		   {   /* bypass code */
                       vbs = *(ckt->CKTstate0 + here->UFSvbs);
                       vbd = *(ckt->CKTstate0 + here->UFSvbd);
                       vgfs = *(ckt->CKTstate0 + here->UFSvgfs);
                       vds = *(ckt->CKTstate0 + here->UFSvds);
                       vgbs = *(ckt->CKTstate0 + here->UFSvgbs);
                       T = *(ckt->CKTstate0 + here->UFStemp);

                       vgfd = vgfs - vds;
                       vgbd = vgbs - vds;

		       Vds = pOpInfo->Vds;
		       Vgfs = pOpInfo->Vgfs;
		       Vgbs = pOpInfo->Vgbs;
		       Vbs = pOpInfo->Vbs;
                       if ((ckt->CKTmode & (MODETRAN | MODEAC)) || 
                           ((ckt->CKTmode & MODETRANOP) && 
                           (ckt->CKTmode & MODEUIC)))
		       {   ByPass = 1;
                           goto line755;
                       }
		       else
		       {   goto line850;
		       }
                   }
               }

#endif /*NOBYPASS*/
               if (*(ckt->CKTstate0 + here->UFSvds) >= 0.0)
	       {   vgfs = DEV.fetlim(vgfs, *(ckt->CKTstate0+here->UFSvgfs), Vthf);
		   if (vgfs > pOpInfo->Vgfs + 0.5)
		       vgfs = pOpInfo->Vgfs + 0.5;
		   else if (vgfs < pOpInfo->Vgfs - 0.5)
		       vgfs = pOpInfo->Vgfs - 0.5;

	           vgbs = DEV.fetlim(vgbs, *(ckt->CKTstate0+here->UFSvgbs), Vthb);
		   if (vgbs > pOpInfo->Vgbs + 0.5)
		       vgbs = pOpInfo->Vgbs + 0.5;
		   else if (vgbs < pOpInfo->Vgbs - 0.5)
		       vgbs = pOpInfo->Vgbs - 0.5;

		   vds = vgfs - vgfd;
                   vds = DEV.limvds(vds, *(ckt->CKTstate0 + here->UFSvds));
		   if (vds > pOpInfo->Vds + 0.5)
		       vds = pOpInfo->Vds + 0.5;
		   else if (vds < pOpInfo->Vds - 0.5)
		       vds = pOpInfo->Vds - 0.5;
                   vgfd = vgfs - vds;
                   vgbd = vgbs - vds;
               }
	       else
	       {   vgfd = DEV.fetlim(vgfd, vgfdo, Vthf);
		   if (vgfd > pOpInfo->Vgfs + 0.5)
		       vgfd = pOpInfo->Vgfs + 0.5;
		   else if (vgfd < pOpInfo->Vgfs - 0.5)
		       vgfd = pOpInfo->Vgfs - 0.5;

	           vgbd = DEV.fetlim(vgbd, vgbdo, Vthb);
		   if (vgbd > pOpInfo->Vgbs + 0.5)
		       vgbd = pOpInfo->Vgbs + 0.5;
		   else if (vgbd < pOpInfo->Vgbs - 0.5)
		       vgbd = pOpInfo->Vgbs - 0.5;

		   vds = vgfs - vgfd;
                   vds = -DEV.limvds(-vds, -(*(ckt->CKTstate0+here->UFSvds)));
		   if (-vds > pOpInfo->Vds + 0.5)
		       vds = -(pOpInfo->Vds + 0.5);
		   else if (-vds < pOpInfo->Vds - 0.5)
		       vds = -(pOpInfo->Vds - 0.5);
                   vgfs = vgfd + vds;
                   vgbs = vgbd + vds;
               }

               if (vds >= 0.0)
	       {   
		   vbs = DEV.pnjlim(vbs, *(ckt->CKTstate0 + here->UFSvbs),
                                   CONSTvt0, model->UFSvcrit, &Check);

		   vbs = ufsLimiting(pInst, pOpInfo->Vbs, pOpInfo->dIr_dVb,
				     ckt->CKTabstol, DynamicNeeded, Ibtot, vbs);
                   vbd = vbs - vds;
               }
	       else
	       {   
		   vbd = DEV.pnjlim(vbd, *(ckt->CKTstate0 + here->UFSvbd),
                                   CONSTvt0, model->UFSvcrit, &Check); 
		   vbd = ufsLimiting(pInst, pOpInfo->Vbs, pOpInfo->dIr_dVb,
				     ckt->CKTabstol, DynamicNeeded, Ibtot, vbd);
                   vbs = vbd + vds;
               }
	       if (pModel->Selft > 0)
	       {   Tprev = *(ckt->CKTstate0 + here->UFStemp);
		   if (!DynamicNeeded)
		   {   T = pOpInfo->Power / pInst->MFinger * pInst->Rth;
		   }
		   if (T > Tprev + 10.0)
		       T = Tprev + 10.0;
		   else if (T < Tprev - 10.0)
		       T = Tprev - 10.0;
		   if (T < 0.0)
		       T = 0.0;
	       }
          }

          /* determine DC current and derivatives */
          vbd = vbs - vds;
          vgfd = vgfs - vds;
          vgbd = vgbs - vds;
//          vgb = vgfs - vbs;

          if (vds >= 0.0)
	  {   /* normal mode */
              pInst->Mode = 1;
              Vds = vds;
              Vgfs = vgfs;
              Vgbs = vgbs;
              Vbs = vbs;
          }
	  else
	  {   /* inverse mode */
              pInst->Mode = -1;
              Vds = -vds;
              Vgfs = vgfd;
              Vgbs = vgbd;
              Vbs = vbd;
          }
	  pOpInfo->Vgfs = Vgfs;
	  pOpInfo->Vgfd = Vgfs - Vds;
	  pOpInfo->Vds = Vds;
	  pOpInfo->Vgbs = Vgbs;
	  pOpInfo->Vbs = Vbs;
	  pOpInfo->Vbd = Vbs - Vds;

	  if (pModel->Selft == 2)
	  {   Vtemp = T + ckt->CKTtemp;
	      SH = 1;                                                            /* 4.5 */
	      ufsTempEffect(pModel, pInst, &Env, Vtemp, SH);                     /* 4.5 */
	  }
	  else
	  {   pInst->Temperature = T + ckt->CKTtemp;                             /* 5.0 */
	  }

	  if (pModel->NfdMod == 0)
	  {   fdEvalDeriv(Vds, Vgfs, Vbs, Vgbs, pOpInfo, pInst, pModel, &Env,
		          DynamicNeeded, ACNeeded);                              /* 5.0 */
	  }
	  else
	  {   nfdEvalDeriv(Vds, Vgfs, Vbs, Vgbs, pOpInfo, pInst, pModel, &Env,
		           DynamicNeeded, ACNeeded);                             /* 4.5d */
	  }

//finished: /* returning Values to Calling Routine */
          /*
           *  COMPUTE EQUIVALENT DRAIN CURRENT SOURCE
           */

          /*
           *  check convergence
           */
          if ((here->UFSoff == 0) || (!(ckt->CKTmode & MODEINITFIX)))
	  {   if (Check == 1)
	      {   ckt->incNoncon();  // SRW
#ifndef NEWCONV
              } 
	      else
	      {   tol = ckt->CKTreltol * MAX(FABS(cdhat), FABS(Idtot))
		      + ckt->CKTabstol;
                  if (FABS(cdhat - Idtot) >= tol)
		  {   ckt->incNoncon();  // SRW
                  }
		  else
		  {   if (pInst->BulkContact)
		      {   tol = ckt->CKTreltol * MAX(FABS(cbhat), 
			        FABS(Ibtot)) + ckt->CKTabstol;
                          if (FABS(cbhat - Ibtot) > tol)
		          {   ckt->incNoncon();  // SRW
                          }
		      }
		      else
		      {   if (FABS(Ibtot) > ckt->CKTabstol)
		          {   ckt->incNoncon();  // SRW
                          }
		      }
                  }
                  tol = ckt->CKTreltol * MAX(FABS(cghat), FABS(Igtot))                 /* 7.0Y */
                      + ckt->CKTabstol;                                                /* 7.0Y */
                  if (FABS(cghat - Igtot) >= tol)                                      /* 7.0Y */
                  {   ckt->incNoncon();  // SRW
                  }                                                                    /* 7.0Y */

#endif /* NEWCONV */
              }
          }
          *(ckt->CKTstate0 + here->UFSvbs) = vbs;
          *(ckt->CKTstate0 + here->UFSvbd) = vbd;
          *(ckt->CKTstate0 + here->UFSvgfs) = vgfs;
          *(ckt->CKTstate0 + here->UFSvds) = vds;
          *(ckt->CKTstate0 + here->UFSvgbs) = vgbs;
          *(ckt->CKTstate0 + here->UFStemp) = T;

          if (!DynamicNeeded)
              goto line850; 
         
line755:
          ag0 = ckt->CKTag[0];

	  Cgfdo = Weff * pModel->Cgfdo * pInst->MFinger;
	  Cgfso = Weff * pModel->Cgfso * pInst->MFinger;
	  Cgfbo = 2.0 * Leff * pModel->Cgfbo * pInst->MFinger;
	  Cgbbo = pModel->Coxb * pInst->BodyArea * pInst->MFinger;
	  Qgfdo = Cgfdo * (vgfs - vds);
	  Qgfso = Cgfso * vgfs;
	  Qgfbo = Cgfbo * (vgfs - vbs);
	  Qgbbo = Cgbbo * (vgbs - vbs);
	  Csg = -(pOpInfo->dQgf_dVgf + pOpInfo->dQd_dVgf
	      + pOpInfo->dQb_dVgf + pOpInfo->dQgb_dVgf);
	  Csd = -(pOpInfo->dQgf_dVd + pOpInfo->dQd_dVd 
	      + pOpInfo->dQb_dVd + pOpInfo->dQgb_dVd);
	  Csb = -(pOpInfo->dQgf_dVb + pOpInfo->dQd_dVb 
	      + pOpInfo->dQb_dVb + pOpInfo->dQgb_dVb);
	  Csgb = -(pOpInfo->dQgf_dVgb + pOpInfo->dQd_dVgb 
	      + pOpInfo->dQb_dVgb + pOpInfo->dQgb_dVgb);
	  T0 = pModel->Type * ELECTRON_CHARGE * Weff * Leff * pInst->MFinger;    /* 4.5 */
	  Qnqff= T0 * (pModel->Nqff + 2.0 * pModel->Nqfsw * pModel->Tb / Weff);  /* 4.5r */
	  Qnqfb= T0 * pModel->Nqfb;                                              /* 4.5 */
          if (pInst->Mode > 0)
	  {   
	      gcggb = (pOpInfo->dQgf_dVgf + Cgfdo + Cgfso + Cgfbo) * ag0;
              gcgdb = (pOpInfo->dQgf_dVd - Cgfdo) * ag0;
              gcgbb = (pOpInfo->dQgf_dVb - Cgfbo) * ag0;
              gcggbb = pOpInfo->dQgf_dVgb * ag0;
              gcgsb = -(gcggb + gcgdb + gcgbb + gcggbb);

	      gcdgb = (pOpInfo->dQd_dVgf - Cgfdo) * ag0;
              gcddb = (pOpInfo->dQd_dVd + Cgfdo) * ag0;
              gcdbb = pOpInfo->dQd_dVb * ag0;
              gcdgbb = pOpInfo->dQd_dVgb * ag0;
              gcdsb = -(gcdgb + gcddb + gcdbb + gcdgbb);

	      gcsgb = (Csg - Cgfso) * ag0;
              gcsdb = Csd * ag0;
              gcsbb = Csb * ag0;
              gcsgbb = Csgb * ag0;
              gcssb = -(gcsgb + gcsdb + gcsbb + gcsgbb);

	      gcbgb = (pOpInfo->dQb_dVgf - Cgfbo) * ag0;
              gcbdb = pOpInfo->dQb_dVd * ag0;
              gcbbb = (pOpInfo->dQb_dVb + Cgfbo + Cgbbo) * ag0;
              gcbgbb = (pOpInfo->dQb_dVgb - Cgbbo) * ag0;
              gcbsb = -(gcbgb + gcbdb + gcbbb + gcbgbb);
 
	      gcgbgb = pOpInfo->dQgb_dVgf * ag0;
              gcgbdb = pOpInfo->dQgb_dVd * ag0;
              gcgbbb = (pOpInfo->dQgb_dVb - Cgbbo) * ag0;
              gcgbgbb = (pOpInfo->dQgb_dVgb + Cgbbo) * ag0;
              gcgbsb = -(gcgbgb + gcgbdb + gcgbbb + gcgbgbb);
 
              qgate = pOpInfo->Qgf + Qgfdo + Qgfso + Qgfbo;
              qdrn = pOpInfo->Qd - Qgfdo;
              qbulk = pOpInfo->Qb - Qgfbo - Qgbbo;
              qback = pOpInfo->Qgb + Qgbbo;
              qsrc = -(qgate + qdrn + qbulk + qback + Qnqff + Qnqfb);            /* 4.5 */

	      if (pModel->Selft > 0)
	      {   Cth = pInst->Cth * pInst->MFinger;
	          gcth = Cth * ag0;
	          if (pModel->Selft > 1)
	          {   gcgtb = pOpInfo->dQgf_dT * ag0;
                      gcdtb = pOpInfo->dQd_dT * ag0;
                      gcbtb = pOpInfo->dQb_dT * ag0;
                      gcgbtb = pOpInfo->dQgb_dT * ag0;
                      gcstb = -(gcgtb + gcdtb + gcbtb + gcgbtb);
		  }
	      }
	  }
	  else
	  {   gcggb = (pOpInfo->dQgf_dVgf + Cgfdo + Cgfso + Cgfbo) * ag0;
              gcgsb = (pOpInfo->dQgf_dVd - Cgfso) * ag0;
              gcgbb = (pOpInfo->dQgf_dVb - Cgfbo) * ag0;
              gcggbb = pOpInfo->dQgf_dVgb * ag0;
              gcgdb = -(gcggb + gcgsb + gcgbb + gcggbb);

	      gcdgb = (Csg - Cgfdo) * ag0;
              gcdsb = Csd * ag0;
              gcdbb = Csb * ag0;
              gcdgbb = Csgb * ag0;
              gcddb = -(gcdgb + gcdsb + gcdbb + gcdgbb);

	      gcsgb = (pOpInfo->dQd_dVgf - Cgfso) * ag0;
              gcssb = (pOpInfo->dQd_dVd + Cgfso) * ag0;
              gcsbb = pOpInfo->dQd_dVb * ag0;
              gcsgbb = pOpInfo->dQd_dVgb * ag0;
              gcsdb = -(gcsgb + gcssb + gcsbb + gcsgbb);

	      gcbgb = (pOpInfo->dQb_dVgf - Cgfbo) * ag0;
              gcbsb = pOpInfo->dQb_dVd * ag0;
              gcbbb = (pOpInfo->dQb_dVb + Cgfbo + Cgbbo) * ag0;
              gcbgbb = (pOpInfo->dQb_dVgb - Cgbbo) * ag0;
              gcbdb = -(gcbgb + gcbsb + gcbbb + gcbgbb);
 
	      gcgbgb = pOpInfo->dQgb_dVgf * ag0;
              gcgbsb = pOpInfo->dQgb_dVd * ag0;
              gcgbbb = (pOpInfo->dQgb_dVb - Cgbbo) * ag0;
              gcgbgbb = (pOpInfo->dQgb_dVgb + Cgbbo) * ag0;
              gcgbdb = -(gcgbgb + gcgbsb + gcgbbb + gcgbgbb);
 
              qgate = pOpInfo->Qgf + Qgfdo + Qgfso + Qgfbo;
              qsrc = pOpInfo->Qd - Qgfso;
              qbulk = pOpInfo->Qb - Qgfbo - Qgbbo;
              qback = pOpInfo->Qgb + Qgbbo;
              qdrn = -(qgate + qsrc + qbulk + qback + Qnqff + Qnqfb);            /* 4.5 */

	      if (pModel->Selft > 0)
	      {   Cth = pInst->Cth * pInst->MFinger;
	          gcth = Cth * ag0;
	          if (pModel->Selft > 1)
	          {   gcgtb = pOpInfo->dQgf_dT * ag0;
                      gcstb = pOpInfo->dQd_dT * ag0;
                      gcbtb = pOpInfo->dQb_dT * ag0;
                      gcgbtb = pOpInfo->dQgb_dT * ag0;
                      gcdtb = -(gcgtb + gcstb + gcbtb + gcgbtb);
	          }
	      }
          }

          if (ByPass) goto line860;

          *(ckt->CKTstate0 + here->UFSqg) = qgate;
          *(ckt->CKTstate0 + here->UFSqd) = qdrn;
          *(ckt->CKTstate0 + here->UFSqb) = qbulk;
          *(ckt->CKTstate0 + here->UFSqgb) = qback;
	  if (pModel->Selft > 0)
	  {   Qt = Cth * T;
	      *(ckt->CKTstate0 + here->UFSqt) = Qt;
	  }

          /* store small signal parameters */
          if (ckt->CKTmode & MODEINITSMSIG)
	  {   goto line1000;
          }
          if (!DynamicNeeded)
              goto line850;
       
          if (ckt->CKTmode & MODEINITTRAN)
	  {   *(ckt->CKTstate1 + here->UFSqb) =
                    *(ckt->CKTstate0 + here->UFSqb);
              *(ckt->CKTstate1 + here->UFSqg) =
                    *(ckt->CKTstate0 + here->UFSqg);
              *(ckt->CKTstate1 + here->UFSqd) =
                    *(ckt->CKTstate0 + here->UFSqd);
              *(ckt->CKTstate1 + here->UFSqgb) =
                    *(ckt->CKTstate0 + here->UFSqgb);
	      if (pModel->Selft > 0)
	      {   *(ckt->CKTstate1 + here->UFSqt) =
                        *(ckt->CKTstate0 + here->UFSqt);
	      }
          }
       
          error = NIintegrate(ckt, &geq, &ceq, 0.0, here->UFSqb);
          if (error)
	      return(error);
          error = NIintegrate(ckt, &geq, &ceq, 0.0, here->UFSqg);
          if (error)
	      return(error);
          error = NIintegrate(ckt, &geq, &ceq, 0.0, here->UFSqd);
          if (error)
	      return(error);
          error = NIintegrate(ckt, &geq, &ceq, 0.0, here->UFSqgb);
          if (error)
	      return(error);
	  if (pModel->Selft > 0)
	  {   error = NIintegrate(ckt, &geq, &ceq, 0.0, here->UFSqt);
              if (error)
	          return(error);
	  }
          goto line860;

line850:
          /* initialize to zero charge conductance and current */
          ceqqg = ceqqb = ceqqd = ceqqgb = ceqqt = 0.0;
          gcdgb = gcddb = gcdsb = gcdbb = gcdgbb = 0.0;
          gcsgb = gcsdb = gcssb = gcsbb = gcsgbb = 0.0;
          gcggb = gcgdb = gcgsb = gcgbb = gcggbb = 0.0;
          gcbgb = gcbdb = gcbsb = gcbbb = gcbgbb = 0.0;
          gcgbgb = gcgbdb = gcgbsb = gcgbbb = gcgbgbb = 0.0;
	  gcgtb = gcstb = gcbtb = gcdtb = gcgbtb = gcth = 0.0;
          goto line900;
            
line860:
          /* evaluate equivalent charge current */

          cqgate = *(ckt->CKTstate0 + here->UFScqg);
          cqbulk = *(ckt->CKTstate0 + here->UFScqb);
          cqdrn = *(ckt->CKTstate0 + here->UFScqd);
          cqback = *(ckt->CKTstate0 + here->UFScqgb);

          ceqqg = cqgate - gcggb * vgfs - gcgdb * vds - gcgbb * vbs
	        - gcggbb * vgbs;
          ceqqb = cqbulk - gcbgb * vgfs - gcbdb * vds - gcbbb * vbs
	        - gcbgbb * vgbs;
          ceqqd = cqdrn - gcdgb * vgfs - gcddb * vds - gcdbb * vbs
	        - gcdgbb * vgbs;
          ceqqgb = cqback - gcgbgb * vgfs - gcgbdb * vds - gcgbbb * vbs
	        - gcgbgbb * vgbs;
	  if (pModel->Selft > 0)
	  {   if (pModel->Selft > 1)
	      {   ceqqg -= gcgtb * T;
                  ceqqb -= gcbtb * T;
                  ceqqd -= gcdtb * T;
                  ceqqgb -= gcgbtb * T;
	      }
              cqt = *(ckt->CKTstate0 + here->UFScqt);
	      ceqqt = cqt - gcth * T;
	  }
 
          if (ckt->CKTmode & MODEINITTRAN)
	  {   *(ckt->CKTstate1 + here->UFScqb) =  
                    *(ckt->CKTstate0 + here->UFScqb);
              *(ckt->CKTstate1 + here->UFScqg) =  
                    *(ckt->CKTstate0 + here->UFScqg);
              *(ckt->CKTstate1 + here->UFScqd) =  
                    *(ckt->CKTstate0 + here->UFScqd);
              *(ckt->CKTstate1 + here->UFScqgb) =  
                    *(ckt->CKTstate0 + here->UFScqgb);
	      if (pModel->Selft > 0)
	      {   *(ckt->CKTstate1 + here->UFScqt) =  
                    *(ckt->CKTstate0 + here->UFScqt);
	      }
          }

          /*
           *  load current vector
           */
line900:

          if (pInst->Mode >= 0)
	  {   Idtot = pOpInfo->Ich + pOpInfo->Ibjt + pOpInfo->Igi
		    - pOpInfo->Igt;
              Ibtot = pOpInfo->Igt + pOpInfo->Ir - pOpInfo->Igi
                    - pOpInfo->Igb;                                                   /* 7.0Y */
              Igtot = pOpInfo->Igb;                                                   /* 7.0Y */
	      dId_dVg = (pOpInfo->dId_dVgf + pOpInfo->dIgi_dVgf
		      - pOpInfo->dIgt_dVgf);
	      dId_dVd = (pOpInfo->dId_dVd + pOpInfo->dIgi_dVd 
		      - pOpInfo->dIgt_dVd);
	      dId_dVb = (pOpInfo->dId_dVb + pOpInfo->dIgi_dVb 
		      - pOpInfo->dIgt_dVb);
	      dId_dVgb = (pOpInfo->dId_dVgb + pOpInfo->dIgi_dVgb 
		      - pOpInfo->dIgt_dVgb);
	      dId_dVs = -(dId_dVg + dId_dVd + dId_dVb + dId_dVgb);

              dIb_dVg = (pOpInfo->dIgt_dVgf - pOpInfo->dIgi_dVgf
                      - pOpInfo->dIgb_dVgf);                                          /* 7.0Y */
              dIb_dVd = (pOpInfo->dIgt_dVd - pOpInfo->dIgi_dVd
                      - pOpInfo->dIgb_dVd);                                           /* 7.0Y */
              dIb_dVb = (pOpInfo->dIgt_dVb + pOpInfo->dIr_dVb
                      - pOpInfo->dIgi_dVb - pOpInfo->dIgb_dVb);                       /* 7.0Y */
              dIb_dVgb = (pOpInfo->dIgt_dVgb - pOpInfo->dIgi_dVgb
                       - pOpInfo->dIgb_dVgb);                                         /* 7.0Y */
              dIb_dVs = -(dIb_dVg + dIb_dVd + dIb_dVb + dIb_dVgb);                     /* 7.0Y */

              dIgf_dVg = (pOpInfo->dIgb_dVgf);                                        /* 7.0Y */
              dIgf_dVd = (pOpInfo->dIgb_dVd);                                         /* 7.0Y */
              dIgf_dVb = (pOpInfo->dIgb_dVb);                                         /* 7.0Y */
              dIgf_dVgb = (pOpInfo->dIgb_dVgb);                                       /* 7.0Y */
              dIgf_dVs = -(dIgf_dVg + dIgf_dVd + dIgf_dVb + dIgf_dVgb);                /* 7.0Y */

              dIs_dVg = -(dId_dVg + dIb_dVg + dIgf_dVg);                               /* 7.0Y */
              dIs_dVd = -(dId_dVd + dIb_dVd + dIgf_dVd);                               /* 7.0Y */
              dIs_dVb = -(dId_dVb + dIb_dVb + dIgf_dVb);                               /* 7.0Y */
              dIs_dVgb = -(dId_dVgb + dIb_dVgb + dIgf_dVgb);                           /* 7.0Y */
              dIs_dVs = -(dId_dVs + dIb_dVs + dIgf_dVs);                               /* 7.0Y */


              ceqd = pModel->Type * (Idtot - dId_dVd * Vds - dId_dVg * Vgfs
		   - dId_dVb * Vbs - dId_dVgb *  Vgbs);
              ceqb = pModel->Type * (Ibtot - dIb_dVg * Vgfs - dIb_dVd * Vds
		   - dIb_dVb * Vbs - dIb_dVgb * Vgbs);
              ceqg = pModel->Type * (Igtot - dIgf_dVg * Vgfs - dIgf_dVd * Vds
                   - dIgf_dVb * Vbs - dIgf_dVgb * Vgbs);                               /* 7.0Y */

	      if (pModel->Selft > 0)
	      {   dP_dVg = pOpInfo->dP_dVgf;
		  dP_dVd = pOpInfo->dP_dVd;
		  dP_dVb = pOpInfo->dP_dVb;
		  dP_dVgb = pOpInfo->dP_dVgb;
	          dP_dVs = -(dP_dVg + dP_dVd + dP_dVb + dP_dVgb);
                  ceqp = pModel->Type * (pOpInfo->Power - dP_dVd * Vds
                           - dP_dVg * Vgfs - dP_dVb * Vbs - dP_dVgb * Vgbs);
	          if (pModel->Selft > 1)
		  {   dP_dT = pOpInfo->dP_dT;
		      dId_dT = (pOpInfo->dId_dT + pOpInfo->dIgi_dT
			     - pOpInfo->dIgt_dT);
                      dIb_dT = (pOpInfo->dIgt_dT + pOpInfo->dIr_dT
                             - pOpInfo->dIgi_dT - pOpInfo->dIgb_dT);                  /* 7.0Y */
                      dIgf_dT = (pOpInfo->dIgb_dT);                                   /* 7.0Y */
                      dIs_dT = -(dId_dT + dIb_dT + dIgf_dT);                           /* 7.0Y */
                      ceqp -= pModel->Type * dP_dT * T;
		      ceqd -= pModel->Type * dId_dT * T;
		      ceqb -= pModel->Type * dIb_dT * T;
                      ceqg -= pModel->Type * dIgf_dT * T;                              /* 7.0Y */
		  }
	      }
              ceqs = -(ceqd + ceqb + ceqg);                                            /* 7.0Y */

          }
	  else
	  {   Idtot = pOpInfo->Ich + pOpInfo->Ibjt + pOpInfo->Igi
		    - pOpInfo->Igt;
              Ibtot = pOpInfo->Igt + pOpInfo->Ir - pOpInfo->Igi
                    - pOpInfo->Igb;                                                   /* 7.0Y */
              Igtot = pOpInfo->Igb;                                                   /* 7.0Y */

	      dIs_dVg = (pOpInfo->dId_dVgf + pOpInfo->dIgi_dVgf 
		      - pOpInfo->dIgt_dVgf);
	      dIs_dVs = (pOpInfo->dId_dVd + pOpInfo->dIgi_dVd 
		      - pOpInfo->dIgt_dVd);
	      dIs_dVb = (pOpInfo->dId_dVb + pOpInfo->dIgi_dVb 
		      - pOpInfo->dIgt_dVb);
	      dIs_dVgb = (pOpInfo->dId_dVgb + pOpInfo->dIgi_dVgb 
		      - pOpInfo->dIgt_dVgb);
	      dIs_dVd = -(dIs_dVg + dIs_dVs + dIs_dVb + dIs_dVgb);


              dIb_dVg = (pOpInfo->dIgt_dVgf - pOpInfo->dIgi_dVgf
                      - pOpInfo->dIgb_dVgf);                                          /* 7.0Y */
              dIb_dVs = (pOpInfo->dIgt_dVd - pOpInfo->dIgi_dVd
                      - pOpInfo->dIgb_dVd);                                           /* 7.0Y */
              dIb_dVb = (pOpInfo->dIgt_dVb + pOpInfo->dIr_dVb
                      - pOpInfo->dIgi_dVb - pOpInfo->dIgb_dVb);                       /* 7.0Y */
              dIb_dVgb = (pOpInfo->dIgt_dVgb - pOpInfo->dIgi_dVgb
                       - pOpInfo->dIgb_dVgb);                                         /* 7.0Y */
              dIb_dVd = -(dIb_dVg + dIb_dVs + dIb_dVb + dIb_dVgb);                     /* 7.0Y */

              dIgf_dVg = (pOpInfo->dIgb_dVgf);                                        /* 7.0Y */
              dIgf_dVs = (pOpInfo->dIgb_dVd);                                         /* 7.0Y */
              dIgf_dVb = (pOpInfo->dIgb_dVb);                                         /* 7.0Y */
              dIgf_dVgb = (pOpInfo->dIgb_dVgb);                                       /* 7.0Y */
              dIgf_dVd = -(dIgf_dVg + dIgf_dVs + dIgf_dVb + dIgf_dVgb);                /* 7.0Y */

              dId_dVg = -(dIs_dVg + dIb_dVg + dIgf_dVg);                               /* 7.0Y */
              dId_dVs = -(dIs_dVs + dIb_dVs + dIgf_dVs);                               /* 7.0Y */
              dId_dVb = -(dIs_dVb + dIb_dVb + dIgf_dVb);                               /* 7.0Y */
              dId_dVgb = -(dIs_dVgb + dIb_dVgb + dIgf_dVgb);                           /* 7.0Y */
              dId_dVd = -(dIs_dVd + dIb_dVd + dIgf_dVd);                               /* 7.0Y */

              ceqs = pModel->Type * (Idtot - dIs_dVs * Vds - dIs_dVg * Vgfs
		   - dIs_dVb * Vbs - dIs_dVgb *  Vgbs);
              ceqb = pModel->Type * (Ibtot - dIb_dVg * Vgfs - dIb_dVs * Vds
		   - dIb_dVb * Vbs - dIb_dVgb * Vgbs);
              ceqg = pModel->Type * (Igtot - dIgf_dVg * Vgfs - dIgf_dVs * Vds
                   - dIgf_dVb * Vbs - dIgf_dVgb * Vgbs);                               /* 7.0Y */

	      if (pModel->Selft > 0)
	      {   dP_dVg = pOpInfo->dP_dVgf;
		  dP_dVs = pOpInfo->dP_dVd;
		  dP_dVb = pOpInfo->dP_dVb;
		  dP_dVgb = pOpInfo->dP_dVgb;
	          dP_dVd = -(dP_dVg + dP_dVs + dP_dVb + dP_dVgb);
                  ceqp = pModel->Type * (pOpInfo->Power - dP_dVs * Vds
                       - dP_dVg * Vgfs - dP_dVb * Vbs - dP_dVgb * Vgbs);
	          if (pModel->Selft > 1)
		  {   dP_dT = pOpInfo->dP_dT;
		      dIs_dT = (pOpInfo->dId_dT + pOpInfo->dIgi_dT 
			     - pOpInfo->dIgt_dT);
                      dIb_dT = (pOpInfo->dIgt_dT + pOpInfo->dIr_dT
                             - pOpInfo->dIgi_dT - pOpInfo->dIgb_dT);                  /* 7.0Y */
                      dIgf_dT = (pOpInfo->dIgb_dT);                                   /* 7.0Y */
                      dId_dT = -(dIs_dT + dIb_dT + dIgf_dT);                           /* 7.0Y */
		      ceqp -= pModel->Type * dP_dT * T;
		      ceqs -= pModel->Type * dIs_dT * T;
		      ceqb -= pModel->Type * dIb_dT * T;
                      ceqg -= pModel->Type * dIgf_dT * T;                              /* 7.0Y */
		  }
	      }
              ceqd = -(ceqs + ceqb + ceqg);                                            /* 7.0Y */
          }

	  if (pModel->Type < 0)
	  {   ceqqg = -ceqqg;
              ceqqb = -ceqqb;
              ceqqd = -ceqqd;
              ceqqgb = -ceqqgb;
              ceqqt = -ceqqt;
	  }
	  ceqqs = -(ceqqg + ceqqd + ceqqb + ceqqgb);
	  /*
	  if (pInst->Mode < 1)
	       fprintf(stderr, "Reversed, Vds = %g\n", vds);
	       */

          ckt->rhsadd(here->UFSgNode, -(ceqg + ceqqg));                          /* 7.0Y */
          ckt->rhsadd(here->UFSbNodePrime, -(ceqb + ceqqb));
          ckt->rhsadd(here->UFSdNodePrime, -(ceqd + ceqqd));
          ckt->rhsadd(here->UFSsNodePrime, -(ceqs + ceqqs));
          ckt->rhsadd(here->UFSbgNode, -ceqqgb);
	  if (pModel->Selft > 0)
	  {   ckt->rhsadd(here->UFStNode, ceqp - ceqqt);
	  }

          /*
           *  load y matrix
           */

	  if (pInst->DrainConductance > 0.0)
	  {   gdpr = pInst->MFinger * pInst->DrainConductance
		   * pTempModel->GdsTempFac;
	      ckt->ldadd(here->UFSDdPtr, gdpr);
              ckt->ldadd(here->UFSDdpPtr, -gdpr);
              ckt->ldadd(here->UFSDPdPtr, -gdpr);
	  }
	  else
	  {   gdpr = 0.0;
	  }
	  if (pInst->SourceConductance > 0.0)
	  {   gspr = pInst->MFinger * pInst->SourceConductance
		   * pTempModel->GdsTempFac;
	      ckt->ldadd(here->UFSSsPtr, gspr);
              ckt->ldadd(here->UFSSspPtr, -gspr);
              ckt->ldadd(here->UFSSPsPtr, -gspr);
	  }
	  else
	  {   gspr = 0.0;
	  }
	  if (pInst->BodyConductance > 0.0)
	  {   gbpr = pInst->MFinger * pInst->BodyConductance
		   * pTempModel->GbTempFac;
              ckt->ldadd(here->UFSBbPtr, gbpr);
              ckt->ldadd(here->UFSBbpPtr, -gbpr);
              ckt->ldadd(here->UFSBPbPtr, -gbpr);
	  }
	  else
	  {   gbpr = 0.0;
	  }

          ckt->ldadd(here->UFSGgPtr, dIgf_dVg + gcggb);                 /* 7.0Y */
          ckt->ldadd(here->UFSGdpPtr, dIgf_dVd + gcgdb);                /* 7.0Y */
          ckt->ldadd(here->UFSGspPtr, dIgf_dVs + gcgsb);                /* 7.0Y */
          ckt->ldadd(here->UFSGbpPtr, dIgf_dVb + gcgbb);                /* 7.0Y */
          ckt->ldadd(here->UFSGgbPtr, dIgf_dVgb + gcggbb);              /* 7.0Y */

	  ckt->ldadd(here->UFSDPgPtr, dId_dVg + gcdgb);              
          ckt->ldadd(here->UFSDPdpPtr, gdpr + dId_dVd + gcddb);
          ckt->ldadd(here->UFSDPspPtr, dId_dVs + gcdsb);
          ckt->ldadd(here->UFSDPbpPtr, dId_dVb + gcdbb);
          ckt->ldadd(here->UFSDPgbPtr, dId_dVgb + gcdgbb);

          ckt->ldadd(here->UFSSPgPtr, gcsgb + dIs_dVg);              
          ckt->ldadd(here->UFSSPdpPtr, dIs_dVd + gcsdb);
          ckt->ldadd(here->UFSSPspPtr, gspr + dIs_dVs + gcssb);
          ckt->ldadd(here->UFSSPbpPtr, dIs_dVb + gcsbb);
          ckt->ldadd(here->UFSSPgbPtr, dIs_dVgb + gcsgbb);

          ckt->ldadd(here->UFSBPgPtr, gcbgb + dIb_dVg);              
          ckt->ldadd(here->UFSBPdpPtr, gcbdb + dIb_dVd);
          ckt->ldadd(here->UFSBPspPtr, gcbsb + dIb_dVs);
          ckt->ldadd(here->UFSBPbpPtr, dIb_dVb + gcbbb + gbpr);
          ckt->ldadd(here->UFSBPgbPtr, dIb_dVgb + gcbgbb);

          ckt->ldadd(here->UFSGBgPtr, gcgbgb);                       
          ckt->ldadd(here->UFSGBdpPtr, gcgbdb);
          ckt->ldadd(here->UFSGBspPtr, gcgbsb);
          ckt->ldadd(here->UFSGBbpPtr, gcgbbb);
          ckt->ldadd(here->UFSGBgbPtr, gcgbgbb);

	  if (pModel->Selft > 0)
	  {   ckt->ldadd(here->UFSTtPtr, pInst->MFinger / pInst->Rth + gcth);
	      ckt->ldadd(here->UFSTgPtr, -dP_dVg);                    
	      ckt->ldadd(here->UFSTdpPtr, -dP_dVd);
	      ckt->ldadd(here->UFSTspPtr, -dP_dVs);
	      ckt->ldadd(here->UFSTbpPtr, -dP_dVb);
	      ckt->ldadd(here->UFSTgbPtr, -dP_dVgb);
	      if (pModel->Selft > 1)
	      {   ckt->ldadd(here->UFSTtPtr, -dP_dT);
                  ckt->ldadd(here->UFSGtPtr, dIgf_dT + gcgtb);           /* 7.0Y */
	          ckt->ldadd(here->UFSDPtPtr, dId_dT + gcdtb);
	          ckt->ldadd(here->UFSSPtPtr, dIs_dT + gcstb);
	          ckt->ldadd(here->UFSBPtPtr, dIb_dT + gcbtb);
	          ckt->ldadd(here->UFSGBtPtr, gcgbtb);
	      }
	  }


	  if (model->UFSdebug == 2)
	  {
#ifdef notdef
// Removed by SRW, this is all debugging stuff.  The Gmat will be
// broken with long doubles.
              NodeNum[0] = here->UFSdNodePrime;
	      NodeNum[1] = here->UFSgNode;
	      NodeNum[2] = here->UFSsNodePrime;
	      NodeNum[3] = here->UFSbNodePrime;
	      NodeNum[5] = here->UFSbgNode;
	      NodeNum[6] = here->UFStNode;
              for (I = 0; I < 6; I++)
	      {    Rhs[I] = 0.0;
		   for (J = 0; J < 6; J++)
		   {    Gmat[I][J] = 0.0;
		   }
	      }
	      Rhs[0] = *(ckt->CKTrhs + here->UFSdNodePrime);
              Rhs[1] = *(ckt->CKTrhs + here->UFSgNode);
              Rhs[2] = *(ckt->CKTrhs + here->UFSsNodePrime);
              Rhs[3] = *(ckt->CKTrhs + here->UFSbNodePrime);
              Rhs[4] = *(ckt->CKTrhs + here->UFSbgNode);
	      if (pModel->Selft > 0)
	      {   Rhs[5] = *(ckt->CKTrhs + here->UFStNode);
	      }

              Gmat[0][0] = *(here->UFSDPdpPtr);
              Gmat[0][1] = *(here->UFSDPgPtr);                  
              Gmat[0][2] = *(here->UFSDPspPtr);
              Gmat[0][3] = *(here->UFSDPbpPtr);
              Gmat[0][4] = *(here->UFSDPgbPtr);

              Gmat[1][0] = *(here->UFSGdpPtr);
              Gmat[1][1] = *(here->UFSGgPtr);                  
              Gmat[1][2] = *(here->UFSGspPtr);
              Gmat[1][3] = *(here->UFSGbpPtr);
              Gmat[1][4] = *(here->UFSGgbPtr);

              Gmat[2][0] = *(here->UFSSPdpPtr);
              Gmat[2][1] = *(here->UFSSPgPtr);                  
              Gmat[2][2] = *(here->UFSSPspPtr);
              Gmat[2][3] = *(here->UFSSPbpPtr);
              Gmat[2][4] = *(here->UFSSPgbPtr);

              Gmat[3][0] = *(here->UFSBPdpPtr);
              Gmat[3][1] = *(here->UFSBPgPtr);                  
              Gmat[3][2] = *(here->UFSBPspPtr);
              Gmat[3][3] = *(here->UFSBPbpPtr);
              Gmat[3][4] = *(here->UFSBPgbPtr);

              Gmat[4][0] = *(here->UFSGBdpPtr);
              Gmat[4][1] = *(here->UFSGBgPtr);                  
              Gmat[4][2] = *(here->UFSGBspPtr);
              Gmat[4][3] = *(here->UFSGBbpPtr);
              Gmat[4][4] = *(here->UFSGBgbPtr);

	      if (pModel->Selft > 0)
	      {   Gmat[5][0] = *(here->UFSTdpPtr);
	          Gmat[5][1] = *(here->UFSTgPtr);               
	          Gmat[5][2] = *(here->UFSTspPtr);
	          Gmat[5][3] = *(here->UFSTbpPtr);
	          Gmat[5][4] = *(here->UFSTgbPtr);
                  Gmat[5][5] = *(here->UFSTtPtr);
	          if (pModel->Selft > 1)
	          {   Gmat[0][5] = *(here->UFSDPtPtr);
		      Gmat[1][5] = *(here->UFSGtPtr);           
	              Gmat[2][5] = *(here->UFSSPtPtr);
	              Gmat[3][5] = *(here->UFSBPtPtr);
	              Gmat[4][5] = *(here->UFSGBtPtr);
	          }
	      }
	      /*
	      for (I = 0; I < 6; I++)
	      {    
                   if ((Gmat[I][I] < 0.0) && (NodeNum[I] != 0))
		       fprintf(stderr, "After load: G[%d][%d] = %g\n", I, I, Gmat[I][I]);
	      }
	      */
	      /*
	      for (I = 0; I < 6; I++)
	      {    if (NodeNum[I] != 0)
		       fprintf(stderr, "After load: G[%d][5] = %g\n", I, Gmat[I][5]);
	      }
	      */
	      /*
	      if ((Vds >= 0.8) && (Vgfs < 0.9))
	      {   fprintf(stderr, "Ids=%g, Ib=%g, Vds=%g, Vgfs=%g, Vbs=%g\n",
			  pOpInfo->Ids, pOpInfo->Ibtot, Vds, Vgfs, Vbs);
	      }

	      */
	      /*
	      fprintf(stderr, "%s Vds = %g Vgfs = %g Vbs = %g Isub = %g time=%g\n",
		      DevName, Vds, Vgfs, Vbs, pOpInfo->Igi, ckt->CKTtime);
	      */
#endif
	      DevName = (char *)here->UFSname;
	      fprintf(stderr, "%s T=%g, delt=%g, Vds=%g, delvds=%g\n",
			  DevName, T, delt, Vds, delvds);
	  }

 
line1000:  ;

// SRW     }  /* End of Mosfet Instance */
// SRW}   /* End of Model Instance */

return(OK);
}
