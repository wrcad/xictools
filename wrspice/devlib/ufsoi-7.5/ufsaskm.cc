
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
File: ufsmask.c
**********/

#include "ufsdefs.h"

// This function is the interface used to obtain parameters from the
// device model.  The model pointer and parameter number are
// passed, and the result is returned in the IFvalue struct.
// Only parameters with the IFask flag set get here.


int
UFSdev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sUFSmodel *model = (const sUFSmodel *)genmod;
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    struct ufsAPI_ModelData *pModel;
    int Error;
    double ParamValue;

    pModel = model->pModel;
    if (pModel == NULL)
        return(E_BADPARM);

    switch(which) 
    {   case UFS_MOD_PARAMCHK:
            value->iValue = model->UFSparamChk; 
            data->type = IF_INTEGER;
            return(OK);
        case UFS_MOD_DEBUG:
            value->iValue = model->UFSdebug; 
            data->type = IF_INTEGER;
            return(OK);

	/* Integer Type parameters */ 
	case UFS_MOD_SELFT:
	case UFS_MOD_BODY:
	case UFS_MOD_TPG:
	case UFS_MOD_TPS:
	    Error = ufsGetModelParam(pModel, which, &ParamValue);
	    if (Error)
                return(E_BADPARM);
	    value->iValue = (int) ParamValue;
        data->type = IF_INTEGER;
	    return (OK);

	/* double type parameters */ 
        default:
	    Error = ufsGetModelParam(pModel, which, &ParamValue);
	    if (Error)
                return(E_BADPARM);
	    value->rValue = ParamValue;
    }   

    return(OK);
}
