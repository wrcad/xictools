
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
File: ufspzld.c
**********/

#include "ufsdefs.h"

// This function loads the matrix for pole-zero analysis


int
UFSdev::pzLoad(sGENmodel *genmod, sCKT *ckt, IFcomplex *s)
{
(void)ckt;
sUFSmodel *model = (sUFSmodel*)genmod;
sUFSinstance *here;
struct ufsAPI_OPData *pOpInfo;
struct ufsAPI_ModelData *pModel;
struct ufsAPI_InstData *pInst;
struct ufsTDModelData *pTempModel;
double xcggb, xcgdb, xcgsb, xcgbb, xcbgb, xcbdb, xcbsb, xcbbb;
double xcdgb, xcddb, xcdsb, xcdbb, xcsgb, xcsdb, xcssb, xcsbb;
double gdpr, gspr, gbpr, dId_dVg, dId_dVd, dId_dVs, dId_dVb;                
double dIs_dVg, dIs_dVd, dIs_dVs, dIs_dVb, dIb_dVg, dIb_dVd, dIb_dVs, dIb_dVb;
double dIgf_dVg, dIgf_dVd, dIgf_dVs, dIgf_dVb, dIgf_dVgb;                        /* 7.0Y */
double xcstb, xcgtb, xcdtb, xcbtb, xcth, MFinger, Cgfdo, Cgfso, Cgfbo, Cgbbo;
double dP_dT, dP_dVg, dP_dVd, dP_dVs, dP_dVb, dId_dT, dIb_dT, dIs_dT, dIgf_dT;   /* 7.0Y */
double xcggbb, xcdgbb, xcsgbb, xcbgbb;
double xcgbsb, xcgbgb, xcgbdb, xcgbbb, xcgbgbb, xcgbtb;
double Csg, Csd, Csb, Csgb, dId_dVgb, dIs_dVgb, dIb_dVgb, dP_dVgb, Weff, Leff;

    for (; model != NULL; model = model->UFSnextModel) 
    {    pModel = model->pModel;
         for (here = model->UFSinstances; here!= NULL;
              here = here->UFSnextInstance) 
	 {    pOpInfo = here->pOpInfo;
	      pInst = here->pInst;
	      pTempModel = pInst->pTempModel;
	      MFinger = here->pInst->MFinger;
	      Weff = pInst->Weff;
	      Leff = pInst->Leff;
	      Cgfdo = Weff * pModel->Cgfdo * MFinger;
	      Cgfso = Weff * pModel->Cgfso * MFinger;
	      Cgfbo = 2.0 * Leff * pModel->Cgfbo * MFinger;
	      Cgbbo = pModel->Coxb * pInst->BodyArea * MFinger;
	      Csg = -(pOpInfo->dQgf_dVgf + pOpInfo->dQd_dVgf
		  + pOpInfo->dQb_dVgf + pOpInfo->dQgb_dVgf);
	      Csd = -(pOpInfo->dQgf_dVd + pOpInfo->dQd_dVd 
		  + pOpInfo->dQb_dVd + pOpInfo->dQgb_dVd);
	      Csb = -(pOpInfo->dQgf_dVb + pOpInfo->dQd_dVb 
		  + pOpInfo->dQb_dVb + pOpInfo->dQgb_dVb);
	      Csgb = -(pOpInfo->dQgf_dVgb + pOpInfo->dQd_dVgb 
		  + pOpInfo->dQb_dVgb + pOpInfo->dQgb_dVgb);

	      if (pInst->Mode >= 0) 
	      {   
/*	          xcggb = (pOpInfo->dQgf_dVgf + Cgfso + Cgfbo);                  */
	          xcggb = (pOpInfo->dQgf_dVgf + Cgfdo + Cgfso + Cgfbo);           /* 6.0 */
                  xcgdb = (pOpInfo->dQgf_dVd - Cgfdo);
                  xcgbb = (pOpInfo->dQgf_dVb - Cgfbo);
                  xcggbb = pOpInfo->dQgf_dVgb;
                  xcgsb = -(xcggb + xcgdb + xcgbb + xcggbb);

	          xcdgb = (pOpInfo->dQd_dVgf - Cgfdo);
                  xcddb = (pOpInfo->dQd_dVd + Cgfdo);
                  xcdbb = pOpInfo->dQd_dVb;
                  xcdgbb = pOpInfo->dQd_dVgb;
                  xcdsb = -(xcdgb + xcddb + xcdbb + xcdgbb);

	          xcsgb = (Csg - Cgfso);
                  xcsdb = Csd;
                  xcsbb = Csb;
                  xcsgbb = Csgb;
                  xcssb = -(xcsgb + xcsdb + xcsbb + xcsgbb);

	          xcbgb = (pOpInfo->dQb_dVgf - Cgfbo);
                  xcbdb = pOpInfo->dQb_dVd;
                  xcbbb = (pOpInfo->dQb_dVb + Cgfbo + Cgbbo);
                  xcbgbb = (pOpInfo->dQb_dVgb - Cgbbo);
                  xcbsb = -(xcbgb + xcbdb + xcbbb + xcbgbb);

	          xcgbgb = pOpInfo->dQgb_dVgf;
                  xcgbdb = pOpInfo->dQgb_dVd;
                  xcgbbb = (pOpInfo->dQgb_dVb - Cgbbo);
                  xcgbgbb = (pOpInfo->dQgb_dVgb + Cgbbo);
                  xcgbsb = -(xcgbgb + xcgbdb + xcgbbb + xcgbgbb);

		  if (pModel->Selft > 0)
		  {   xcth = pInst->Cth * MFinger;
		      if (pModel->Selft > 1)
		      {   xcgtb = pOpInfo->dQgf_dT;
		          xcdtb = pOpInfo->dQd_dT;
		          xcbtb = pOpInfo->dQb_dT;
		          xcgbtb = pOpInfo->dQgb_dT;
		          xcstb = -(xcgtb + xcdtb + xcbtb + xcgbtb);
		      }
		  }

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
                            - pOpInfo->dIgb_dVgf);                                     /* 7.0Y */
                  dIb_dVd = (pOpInfo->dIgt_dVd - pOpInfo->dIgi_dVd
                            - pOpInfo->dIgb_dVd);                                      /* 7.0Y */
                  dIb_dVb = (pOpInfo->dIgt_dVb + pOpInfo->dIr_dVb
                            - pOpInfo->dIgi_dVb - pOpInfo->dIgb_dVb);                  /* 7.0Y */
                  dIb_dVgb = (pOpInfo->dIgt_dVgb - pOpInfo->dIgi_dVgb
                            - pOpInfo->dIgb_dVgb);                                     /* 7.0Y */
                  dIb_dVs = -(dIb_dVg + dIb_dVd + dIb_dVb + dIb_dVgb);

                  dIgf_dVg = (pOpInfo->dIgb_dVgf);                                     /* 7.0Y */
                  dIgf_dVd = (pOpInfo->dIgb_dVd);                                      /* 7.0Y */
                  dIgf_dVb = (pOpInfo->dIgb_dVb);                                      /* 7.0Y */
                  dIgf_dVgb = (pOpInfo->dIgb_dVgb);                                    /* 7.0Y */
                  dIgf_dVs = -(dIgf_dVg + dIgf_dVd + dIgf_dVb + dIgf_dVgb);             /* 7.0Y */

                  dIs_dVg = -(dId_dVg + dIb_dVg + dIgf_dVg);                            /* 7.0Y */
                  dIs_dVd = -(dId_dVd + dIb_dVd + dIgf_dVd);                            /* 7.0Y */
                  dIs_dVb = -(dId_dVb + dIb_dVb + dIgf_dVb);                            /* 7.0Y */
                  dIs_dVgb = -(dId_dVgb + dIb_dVgb + dIgf_dVgb);                        /* 7.0Y */
                  dIs_dVs = -(dId_dVs + dIb_dVs + dIgf_dVs);                            /* 7.0Y */



	          if (pModel->Selft > 0)
	          {   dP_dVg = pOpInfo->dP_dVgf;
		      dP_dVd = pOpInfo->dP_dVd;
		      dP_dVb = pOpInfo->dP_dVb;
		      dP_dVgb = pOpInfo->dP_dVgb;
	              dP_dVs = -(dP_dVg + dP_dVd + dP_dVb + dP_dVgb);
	              if (pModel->Selft > 1)
		      {   dP_dT = pOpInfo->dP_dT;
		          dId_dT = (pOpInfo->dId_dT + pOpInfo->dIgi_dT
				 - pOpInfo->dIgt_dT);
                          dIb_dT = (pOpInfo->dIgt_dT + pOpInfo->dIr_dT
                                 - pOpInfo->dIgi_dT - pOpInfo->dIgb_dT);               /* 7.0Y */
                          dIgf_dT = (pOpInfo->dIgb_dT);                                /* 7.0Y */
                          dIs_dT = -(dId_dT + dIb_dT + dIgf_dT);                        /* 7.0Y */
		      }
	          }
              } 
	      else
	      {   xcggb = (pOpInfo->dQgf_dVgf + Cgfdo + Cgfso + Cgfbo);
/*                  xcgsb = (pOpInfo->dQgf_dVd - Cgfdo);                         */
                  xcgsb = (pOpInfo->dQgf_dVd - Cgfso);                            /* 6.0 */
                  xcgbb = (pOpInfo->dQgf_dVb - Cgfbo);
                  xcggbb = pOpInfo->dQgf_dVgb;
                  xcgdb = -(xcggb + xcgsb + xcgbb + xcggbb);

	          xcdgb = (Csg - Cgfdo);
                  xcdsb = Csd;
                  xcdbb = Csb;
                  xcdgbb = Csgb;
                  xcddb = -(xcdgb + xcdsb + xcdbb + xcdgbb);

	          xcsgb = (pOpInfo->dQd_dVgf - Cgfso);
                  xcssb = (pOpInfo->dQd_dVd + Cgfso);
                  xcsbb = pOpInfo->dQd_dVb;
                  xcsgbb = pOpInfo->dQd_dVgb;
                  xcsdb = -(xcsgb + xcssb + xcsbb + xcsgbb);

	          xcbgb = (pOpInfo->dQb_dVgf - Cgfbo);
                  xcbsb = pOpInfo->dQb_dVd;
                  xcbbb = (pOpInfo->dQb_dVb + Cgfbo + Cgbbo);
                  xcbgbb = (pOpInfo->dQb_dVgb - Cgbbo);
                  xcbdb = -(xcbgb + xcbsb + xcbbb + xcbgbb);

	          xcgbgb = pOpInfo->dQgb_dVgf;
                  xcgbsb = pOpInfo->dQgb_dVd;
                  xcgbbb = (pOpInfo->dQgb_dVb - Cgbbo);
                  xcgbgbb = (pOpInfo->dQgb_dVgb + Cgbbo);
                  xcgbdb = -(xcgbgb + xcgbsb + xcgbbb + xcgbgbb);

		  if (pModel->Selft > 0)
		  {   xcth = pInst->Cth * MFinger;
		      if (pModel->Selft > 1)
		      {   xcgtb = pOpInfo->dQgf_dT;
		          xcstb = pOpInfo->dQd_dT;
		          xcbtb = pOpInfo->dQb_dT;
		          xcgbtb = pOpInfo->dQgb_dT;
		          xcdtb = -(xcgtb + xcstb + xcbtb + xcgbtb);
		      }
		  }

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
                          - pOpInfo->dIgb_dVgf);                                        /* 7.0Y */
                  dIb_dVs = (pOpInfo->dIgt_dVd - pOpInfo->dIgi_dVd
                          - pOpInfo->dIgb_dVd);                                         /* 7.0Y */
                  dIb_dVb = (pOpInfo->dIgt_dVb + pOpInfo->dIr_dVb
                          - pOpInfo->dIgi_dVb - pOpInfo->dIgb_dVb);
                  dIb_dVgb = (pOpInfo->dIgt_dVgb - pOpInfo->dIgi_dVgb
                           - pOpInfo->dIgb_dVgb);                                       /* 7.0Y */
                  dIb_dVd = -(dIb_dVg + dIb_dVs + dIb_dVb + dIb_dVgb);                   /* 7.0Y */

                  dIgf_dVg = (pOpInfo->dIgb_dVgf);                                      /* 7.0Y */
                  dIgf_dVs = (pOpInfo->dIgb_dVd);                                       /* 7.0Y */
                  dIgf_dVb = (pOpInfo->dIgb_dVb);                                       /* 7.0Y */
                  dIgf_dVgb = (pOpInfo->dIgb_dVgb);                                     /* 7.0Y */
                  dIgf_dVd = -(dIgf_dVg + dIgf_dVs + dIgf_dVb + dIgf_dVgb);              /* 7.0Y */

                  dId_dVg = -(dIs_dVg + dIb_dVg + dIgf_dVg);                             /* 7.0Y */
                  dId_dVs = -(dIs_dVs + dIb_dVs + dIgf_dVs);                             /* 7.0Y */
                  dId_dVb = -(dIs_dVb + dIb_dVb + dIgf_dVb);                             /* 7.0Y */
                  dId_dVgb = -(dIs_dVgb + dIb_dVgb + dIgf_dVgb);                         /* 7.0Y */
                  dId_dVd = -(dIs_dVd + dIb_dVd + dIgf_dVd);                             /* 7.0Y */

	          if (pModel->Selft > 0)
	          {   dP_dVg = pOpInfo->dP_dVgf;
		      dP_dVs = pOpInfo->dP_dVd;
		      dP_dVb = pOpInfo->dP_dVb;
		      dP_dVgb = pOpInfo->dP_dVgb;
	              dP_dVd = -(dP_dVg + dP_dVs + dP_dVb + dP_dVgb);
	              if (pModel->Selft > 1)
		      {   dP_dT = pOpInfo->dP_dT;
		          dIs_dT = (pOpInfo->dId_dT + pOpInfo->dIgi_dT 
				 - pOpInfo->dIgt_dT);
                          dIb_dT = (pOpInfo->dIgt_dT + pOpInfo->dIr_dT
                                 - pOpInfo->dIgi_dT - pOpInfo->dIgb_dT);               /* 7.0Y */
                          dIgf_dT = (pOpInfo->dIgb_dT);                                /* 7.0Y */
                          dId_dT = -(dIs_dT + dIb_dT + dIgf_dT);                        /* 7.0Y */
		      }
	          }
              }

              *(here->UFSGgPtr ) += xcggb * s->real;            
              *(here->UFSGgPtr +1) += xcggb * s->imag;          
              *(here->UFSGdpPtr ) += xcgdb * s->real;            
              *(here->UFSGdpPtr +1) += xcgdb * s->imag;          
              *(here->UFSGspPtr ) += xcgsb * s->real;            
              *(here->UFSGspPtr +1) += xcgsb * s->imag;          
              *(here->UFSGbpPtr ) += xcgbb * s->real;            
              *(here->UFSGbpPtr +1) += xcgbb * s->imag;          
              *(here->UFSGgbPtr ) += xcggbb * s->real;           
              *(here->UFSGgbPtr +1) += xcggbb * s->imag;         

              *(here->UFSDPgPtr ) += xcdgb * s->real;            
              *(here->UFSDPgPtr +1) += xcdgb * s->imag;          
              *(here->UFSDPdpPtr ) += xcddb * s->real;
              *(here->UFSDPdpPtr +1) += xcddb * s->imag;
              *(here->UFSDPspPtr ) += xcdsb * s->real;
              *(here->UFSDPspPtr +1) += xcdsb * s->imag;
              *(here->UFSDPbpPtr ) += xcdbb * s->real;
              *(here->UFSDPbpPtr +1) += xcdbb * s->imag;
              *(here->UFSDPgbPtr ) += xcdgbb * s->real;
              *(here->UFSDPgbPtr +1) += xcdgbb * s->imag;

              *(here->UFSSPgPtr ) += xcsgb * s->real;            
              *(here->UFSSPgPtr +1) += xcsgb * s->imag;          
              *(here->UFSSPdpPtr ) += xcsdb * s->real;
              *(here->UFSSPdpPtr +1) += xcsdb * s->imag;
              *(here->UFSSPspPtr ) += xcssb * s->real;
              *(here->UFSSPspPtr +1) += xcssb * s->imag;
              *(here->UFSSPbpPtr ) += xcsbb * s->real;
              *(here->UFSSPbpPtr +1) += xcsbb * s->imag;
              *(here->UFSSPgbPtr ) += xcsgbb * s->real;
              *(here->UFSSPgbPtr +1) += xcsgbb * s->imag;

              *(here->UFSBPgPtr ) += xcbgb * s->real;            
              *(here->UFSBPgPtr +1) += xcbgb * s->imag;          
              *(here->UFSBPdpPtr ) += xcbdb * s->real;
              *(here->UFSBPdpPtr +1) += xcbdb * s->imag;
              *(here->UFSBPspPtr ) += xcbsb * s->real;
              *(here->UFSBPspPtr +1) += xcbsb * s->imag;
              *(here->UFSBPbpPtr ) += xcbbb * s->real;
              *(here->UFSBPbpPtr +1) += xcbbb * s->imag;
              *(here->UFSBPgbPtr ) += xcbgbb * s->real;
              *(here->UFSBPgbPtr +1) += xcbgbb * s->imag;

              *(here->UFSGBgPtr ) += xcgbgb * s->real;            
              *(here->UFSGBgPtr +1) += xcgbgb * s->imag;          
              *(here->UFSGBdpPtr ) += xcgbdb * s->real;
              *(here->UFSGBdpPtr +1) += xcgbdb * s->imag;
              *(here->UFSGBspPtr ) += xcgbsb * s->real;
              *(here->UFSGBspPtr +1) += xcgbsb * s->imag;
              *(here->UFSGBbpPtr ) += xcgbbb * s->real;
              *(here->UFSGBbpPtr +1) += xcgbbb * s->imag;
              *(here->UFSGBgbPtr ) += xcgbgbb * s->real;
              *(here->UFSGBgbPtr +1) += xcgbgbb * s->imag;

	      gdpr = MFinger * pInst->DrainConductance * pTempModel->GdsTempFac;
	      if (gdpr > 0.0)
	      {   *(here->UFSDdPtr) += gdpr;
                  *(here->UFSDdpPtr) -= gdpr;
                  *(here->UFSDPdPtr) -= gdpr;
	      }
	      gspr = MFinger * pInst->SourceConductance * pTempModel->GdsTempFac;
	      if (gspr > 0.0)
	      {   *(here->UFSSsPtr) += gspr;
                  *(here->UFSSspPtr) -= gspr;
                  *(here->UFSSPsPtr) -= gspr;
	      }
	      gbpr = pInst->BodyConductance * MFinger * pTempModel->GbTempFac;
	      if (gbpr > 0.0)
	      {   gbpr = MFinger / gbpr;
		  *(here->UFSBbPtr) += gbpr;
		  *(here->UFSBbpPtr) -= gbpr;
		  *(here->UFSBPbPtr) -= gbpr;
	      }

              *(here->UFSDPgPtr) += dId_dVg;                      
              *(here->UFSDPdpPtr) += gdpr + dId_dVd;
              *(here->UFSDPspPtr) += dId_dVs;
              *(here->UFSDPbpPtr) += dId_dVb;
              *(here->UFSDPgbPtr) += dId_dVgb;

              *(here->UFSSPgPtr) += dIs_dVg;                      
              *(here->UFSSPdpPtr) += dIs_dVd;
              *(here->UFSSPspPtr) += gspr + dIs_dVs;
              *(here->UFSSPbpPtr) += dIs_dVb;
              *(here->UFSSPgbPtr) += dIs_dVgb;

              *(here->UFSBPgPtr) += dIb_dVg;                      
              *(here->UFSBPdpPtr) += dIb_dVd;
              *(here->UFSBPspPtr) += dIb_dVs;
              *(here->UFSBPbpPtr) += dIb_dVb + gbpr;
              *(here->UFSBPgbPtr) += dIb_dVgb;

	      if (pModel->Selft > 0)
	      {   *(here->UFSTtPtr) += MFinger / pInst->Rth;
	          *(here->UFSTtPtr) += xcth * s->real;
	          *(here->UFSTtPtr +1) += xcth * s->imag;
	          *(here->UFSTgPtr) += dP_dVg;                    
	          *(here->UFSTdpPtr) += dP_dVd;
	          *(here->UFSTspPtr) += dP_dVs;
	          *(here->UFSTbpPtr) += dP_dVb;
	          *(here->UFSTgbPtr) += dP_dVgb;
	          if (pModel->Selft > 1)
	          {   *(here->UFSTtPtr) += dP_dT;
	              *(here->UFSDPtPtr) += dId_dT;
	              *(here->UFSSPtPtr) += dIs_dT;
	              *(here->UFSBPtPtr) += dIb_dT;
                      *(here->UFSGtPtr) += dIgf_dT;                                     /* 7.0Y */

	              *(here->UFSGtPtr) += xcgtb * s->real;       
	              *(here->UFSDPtPtr) += xcdtb * s->real;
	              *(here->UFSSPtPtr) += xcstb * s->real;
	              *(here->UFSBPtPtr) += xcbtb * s->real;
	              *(here->UFSGBtPtr) += xcgbtb * s->real;
	              *(here->UFSGtPtr +1) += xcgtb * s->imag;    
	              *(here->UFSDPtPtr +1) += xcdtb * s->imag;
	              *(here->UFSSPtPtr +1) += xcstb * s->imag;
	              *(here->UFSBPtPtr +1) += xcbtb * s->imag;
	              *(here->UFSGBtPtr +1) += xcgbtb * s->imag;
	          }
	      }
         }
    }
    return(OK);
}
