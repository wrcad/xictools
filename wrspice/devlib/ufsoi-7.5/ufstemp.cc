
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
 $Id: ufstemp.cc,v 2.8 2002/09/30 13:01:48 stevew Exp $
 *========================================================================*/

/***********
Copyright 1997 University of Florida.  All rights reserved.
Author: Min-Chie Jeng (For SPICE3E2)
File: ufstemp.c
**********/

#include "ufsdefs.h"

// This function modifies instance and model parameters according to
// the temperature


int
UFSdev::temperature(sGENmodel *genmod, sCKT *ckt)
{
sUFSmodel *model = (sUFSmodel*)genmod;
sUFSinstance *here;
struct ufsAPI_InstData *pInst;
struct ufsAPI_ModelData *pModel;
struct ufsTDModelData *pTempModel;
struct ufsAPI_EnvData Env;
double T;
int SH;                                                                          /* 4.5 */

    /*  loop through all the UFS device models */
    Env.Temperature = ckt->CKTtemp;
    Env.Tnom = ckt->CKTnomTemp;
    for (; model != NULL; model = model->UFSnextModel)
    {    pModel = model->pModel;
	 model->UFSvcrit = CONSTvt0 * log(CONSTvt0 / (CONSTroot2 * 1.0e-14));

         /* loop through all the instances of the model */
         for (here = model->UFSinstances; here != NULL;
              here = here->UFSnextInstance) 
	 {    pInst = here->pInst;
	      pInst->pDebug = NULL;
	      pTempModel = pInst->pTempModel;
	      T = ckt->CKTtemp;
	      SH = 1;                                                            /* 4.5 */
	      ufsTempEffect(pModel, pInst, &Env, T, SH);                         /* 4.5 */
         }
    }
    return(OK);
}

