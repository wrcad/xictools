
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
//struct ufsTDModelData *pTempModel;
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
//	      pTempModel = pInst->pTempModel;
	      T = ckt->CKTtemp;
	      SH = 1;                                                            /* 4.5 */
	      ufsTempEffect(pModel, pInst, &Env, T, SH);                         /* 4.5 */
         }
    }
    return(OK);
}

