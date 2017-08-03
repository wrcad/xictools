
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
Copyright 1997 University of Florida.  All rights reserved.
Author: Min-Chie Jeng (For SPICE3E2)
File: ufsmpar.c
**********/

#define TRUE 1

#include "ufsdefs.h"

// This is the interface used to set model parameters


int
UFSdev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sUFSmodel *model = (sUFSmodel*)genmod;
    IFvalue *value = &data->v;
    struct ufsAPI_ModelData *pModel;
    int Error;
    double ParamValue;

    if (model->ModelInitialized == 0)
    {   model->pModel = ALLOC(struct ufsAPI_ModelData, 1);
	if (model->pModel == NULL)
            return(E_NOMEM);
	ufsInitModelFlag(model->pModel);
	model->ModelInitialized = 1;
    }
    pModel = model->pModel;
    switch(param)
    {   
        case  UFS_MOD_PARAMCHK:
            model->UFSparamChk = value->iValue;
            model->UFSparamChkGiven = TRUE;
            break;
        case  UFS_MOD_DEBUG:
            model->UFSdebug = value->iValue;
            model->UFSdebugGiven = TRUE;
            break;

	/* Integer Type parameters */
        case UFS_MOD_NMOS:
        case UFS_MOD_PMOS:
	case UFS_MOD_SELFT:
	case UFS_MOD_BODY:
	case UFS_MOD_TPG:
	case UFS_MOD_TPS:
	case UFS_MOD_BJT:                                  /* 4.5 */
	    ParamValue = (double) value->iValue;
	    Error = ufsSetModelParam(pModel, param, ParamValue);
	    if (Error)
                return(E_BADPARM);
	    return(0);

	/* double type parameters */
        default:
	    ParamValue = value->rValue;
	    Error = ufsSetModelParam(pModel, param, ParamValue);
	    if (Error)
                return(E_BADPARM);
    }
    return(OK);
}

