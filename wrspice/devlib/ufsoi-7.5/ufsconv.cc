
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
File: ufscvtst.c
**********/

#include "ufsdefs.h"

// This function performs a convergence test for devices


int
UFSdev::convTest(sGENmodel *genmod, sCKT *ckt)
{
sUFSmodel *model = (sUFSmodel*)genmod;
sUFSinstance *here;
struct ufsAPI_OPData *pOpInfo;
struct ufsAPI_InstData *pInst;
double delvbd, delvbs, delvds, delvgfd, delvgfs, vbd, vbs, vds, vgbs, delvgbs;
double T, delt, cbhat, cdhat, cghat, tol, vgfd, vgfdo, vgfs, vgbdo, vgbd, delvgbd;     /* 7.0Y */
double Idtot, Ibtot, Igtot;                                                            /* 7.0Y */


    /*  loop through all the UFS device models */
    for (; model != NULL; model = model->UFSnextModel)
    {    /* loop through all the instances of the model */
         for (here = model->UFSinstances; here != NULL ;
              here=here->UFSnextInstance) 
	 {    pInst = here->pInst;
	      vbs = model->pModel->Type * (*(ckt->CKTrhsOld+here->UFSbNodePrime)
		  - *(ckt->CKTrhsOld+here->UFSsNodePrime));
              vgfs = model->pModel->Type * (*(ckt->CKTrhsOld+here->UFSgNode)
		   - *(ckt->CKTrhsOld+here->UFSsNodePrime));
              vds = model->pModel->Type * (*(ckt->CKTrhsOld+here->UFSdNodePrime) 
		  - *(ckt->CKTrhsOld+here->UFSsNodePrime));
              vgbs = model->pModel->Type * (*(ckt->CKTrhsOld+here->UFSbgNode) 
		   - *(ckt->CKTrhsOld+here->UFSsNodePrime));
              vbd = vbs - vds;
              vgfd = vgfs - vds;
              vgfdo = *(ckt->CKTstate0 + here->UFSvgfs) 
		    - *(ckt->CKTstate0 + here->UFSvds);
              vgbd = vgbs - vds;
              vgbdo = *(ckt->CKTstate0 + here->UFSvgbs) 
		    - *(ckt->CKTstate0 + here->UFSvds);

              delvbs = vbs - *(ckt->CKTstate0 + here->UFSvbs);
              delvbd = vbd - *(ckt->CKTstate0 + here->UFSvbd);
              delvgfs = vgfs - *(ckt->CKTstate0 + here->UFSvgfs);
              delvds = vds - *(ckt->CKTstate0 + here->UFSvds);
              delvgbs = vgbs - *(ckt->CKTstate0 + here->UFSvgbs);
              delvgfd = vgfd - vgfdo;
              delvgbd = vgbd - vgbdo;

	      pOpInfo = here->pOpInfo;
	      Idtot = pOpInfo->Ich + pOpInfo->Ibjt + pOpInfo->Igi
		    - pOpInfo->Igt;
              Ibtot = pOpInfo->Igt + pOpInfo->Ir - pOpInfo->Igi - pOpInfo->Igb;       /* 7.0Y */
              Igtot = pOpInfo->Igb;                                                   /* 7.0Y */

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
                        - pOpInfo->dIgb_dVgf) * delvgfs                               /* 7.0Y */
                        + (pOpInfo->dIgt_dVd - pOpInfo->dIgi_dVd
                        - pOpInfo->dIgb_dVd) * delvds                                 /* 7.0Y */
                        + (pOpInfo->dIgt_dVb + pOpInfo->dIr_dVb
                        - pOpInfo->dIgi_dVb - pOpInfo->dIgb_dVb) * delvbs             /* 7.0Y */
                        + (pOpInfo->dIgt_dVgb - pOpInfo->dIgi_dVgb
                        - pOpInfo->dIgb_dVgb) * delvgbs;                              /* 7.0Y */

                  cghat = Igtot                                                        /* 7.0Y */
                        + (pOpInfo->dIgb_dVgf) * delvgfs                              /* 7.0Y */
                        + (pOpInfo->dIgb_dVd) * delvds                                /* 7.0Y */
                        + (pOpInfo->dIgb_dVb) * delvbs                                /* 7.0Y */
                        + (pOpInfo->dIgb_dVgb) * delvgbs;                             /* 7.0Y */

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
                        - pOpInfo->dIgb_dVgb) * delvgbd;                              /* 7.0Y */

                  cghat = Igtot
                        + (pOpInfo->dIgb_dVgf) * delvgfd
                        - (pOpInfo->dIgb_dVd) * delvds
                        + (pOpInfo->dIgb_dVb) * delvbd
                        + (pOpInfo->dIgb_dVgb) * delvgbd;                             /* 7.0Y */

	      }
	      if (model->pModel->Selft > 1)
	      {   T = *(ckt->CKTrhsOld+here->UFStNode);
                  delt = T - *(ckt->CKTstate0 + here->UFStemp);
	          cdhat += (pOpInfo->dId_dT + pOpInfo->dIgi_dT
			- pOpInfo->dIgt_dT) * delt;
                  cbhat += (pOpInfo->dIgt_dT + pOpInfo->dIr_dT
                        - pOpInfo->dIgi_dT - pOpInfo->dIgb_dT) * delt;                /* 7.0Y */
                  cghat += (pOpInfo->dIgb_dT) * delt;                                 /* 7.0Y */

	      }

              /*
               *  check convergence
               */
              if ((here->UFSoff == 0)  || (!(ckt->CKTmode & MODEINITFIX)))
	      {   tol = ckt->CKTreltol * MAX(FABS(cdhat), FABS(Idtot))
		      + ckt->CKTabstol;
                  if (FABS(cdhat - Idtot) >= tol)
		  {   ckt->CKTnoncon++;
                      return(OK);
                  } 
                  tol = ckt->CKTreltol * MAX(FABS(cghat), FABS(Igtot))                 /* 7.0Y */
                      + ckt->CKTabstol;                                                /* 7.0Y */
                  if (FABS(cghat - Igtot) >= tol)                                      /* 7.0Y */
                  {   ckt->CKTnoncon++;                                                /* 7.0Y */
                      return(OK);                                                      /* 7.0Y */
                  }                                                                    /* 7.0Y */
		  if (pInst->BulkContact)
		  {   tol = ckt->CKTreltol * MAX(FABS(cbhat), FABS(Ibtot))
		      + ckt->CKTabstol;
                      if (FABS(cbhat - Ibtot) > tol)
		      {   ckt->CKTnoncon++;
                          return(OK);
                      }
		  }
		  else
		  {   if (FABS(Ibtot) > ckt->CKTabstol)
		      {   ckt->CKTnoncon++;
                          return(OK);
                      }
		  }
              }
         }
    }
    return(OK);
}
