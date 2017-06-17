
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
 $Id: ufssetm.cc,v 2.9 2015/07/26 01:09:14 stevew Exp $
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

