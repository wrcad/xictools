
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
 $Id: ufsseti.cc,v 2.12 2015/07/29 04:50:20 stevew Exp $
 *========================================================================*/

/**********
Copyright 1997 University of Florida.  All rights reserved.
Author: Min-Chie Jeng (For SPICE3E2)
File: ufspar.c
**********/

#include "ufsdefs.h"

#define TRUE 1

// This is the interface used to set instance parameters


int
UFSdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
    sUFSinstance *here = (sUFSinstance*)geninst;
    IFvalue *value = &data->v;
    struct ufsAPI_InstData *pInst;
    double ParamValue;
    int Error;

// SRW this is done in constructor in ufs.cc
    if (here->DeviceInitialized == 0)
    {   here->pInst = ALLOC(struct ufsAPI_InstData, 1);
	if (here->pInst == NULL) 
            return(E_NOMEM);
        here->pOpInfo = ALLOC(struct ufsAPI_OPData, 1);
	if (here->pOpInfo == NULL)
            return(E_NOMEM);
        here->pInst->pTempModel = ALLOC(struct ufsTDModelData, 1);
	if (here->pInst->pTempModel == NULL)
            return(E_NOMEM);
	ufsInitInstFlag(here->pInst);
	here->DeviceInitialized = 1;
    }

    pInst = here->pInst;
    switch(param) 
    {   
        case UFS_OFF:
            here->UFSoff = value->iValue;
            break;
        case UFS_IC_VBS:
            here->UFSicVBS = value->rValue;
            here->UFSicVBSGiven = TRUE;
            break;
        case UFS_IC_VDS:
            here->UFSicVDS = value->rValue;
            here->UFSicVDSGiven = TRUE;
            break;
        case UFS_IC_VGFS:
            here->UFSicVGFS = value->rValue;
            here->UFSicVGFSGiven = TRUE;
            break;
        case UFS_IC_VGBS:
            here->UFSicVGBS = value->rValue;
            here->UFSicVGBSGiven = TRUE;
            break;
        case UFS_IC:
            switch(value->v.numValue){
            case 4:
                here->UFSicVBS = *(value->v.vec.rVec+3);
                here->UFSicVBSGiven = TRUE;
            case 3:
                here->UFSicVGBS = *(value->v.vec.rVec+2);
                here->UFSicVGBSGiven = TRUE;
            case 2:
                here->UFSicVGFS = *(value->v.vec.rVec+1);
                here->UFSicVGFSGiven = TRUE;
            case 1:
                here->UFSicVDS = *(value->v.vec.rVec);
                here->UFSicVDSGiven = TRUE;
                data->cleanup();
                break;
            default:
                data->cleanup();
                return(E_BADPARM);
            }
            break;

	/* Integer type parameters */
        /* case UFS_BJT:
	    ParamValue = (double) value->iValue;
	    Error = ufsSetInstParam(pInst, param, ParamValue);
	    if (Error)
                return(E_BADPARM);
	    return(0);                                           4.5 */

	/* double type parameters */
        default:
	    ParamValue = value->rValue;
	    Error = ufsSetInstParam(pInst, param, ParamValue);
	    if (Error)
                return(E_BADPARM);
    }
    return(OK);
}
