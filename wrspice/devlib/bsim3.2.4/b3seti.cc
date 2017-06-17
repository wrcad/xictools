
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
 $Id: b3seti.cc,v 2.12 2015/07/29 04:50:19 stevew Exp $
 *========================================================================*/

/**********
 * Copyright 2001 Regents of the University of California. All rights reserved.
 * File: b3par.c of BSIM3v3.2.4
 * Author: 1995 Min-Chie Jeng and Mansun Chan
 * Author: 1997-1999 Weidong Liu.
 * Author: 2001 Xuemei Xi
 **********/

#include "b3defs.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


int
BSIM3dev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
    sBSIM3instance *here = static_cast<sBSIM3instance*>(geninst);
    IFvalue *value = &data->v;

    switch(param)
    {
    case BSIM3_W:
        here->BSIM3w = value->rValue;
        here->BSIM3wGiven = TRUE;
        break;
    case BSIM3_L:
        here->BSIM3l = value->rValue;
        here->BSIM3lGiven = TRUE;
        break;
    case BSIM3_AS:
        here->BSIM3sourceArea = value->rValue;
        here->BSIM3sourceAreaGiven = TRUE;
        break;
    case BSIM3_AD:
        here->BSIM3drainArea = value->rValue;
        here->BSIM3drainAreaGiven = TRUE;
        break;
    case BSIM3_PS:
        here->BSIM3sourcePerimeter = value->rValue;
        here->BSIM3sourcePerimeterGiven = TRUE;
        break;
    case BSIM3_PD:
        here->BSIM3drainPerimeter = value->rValue;
        here->BSIM3drainPerimeterGiven = TRUE;
        break;
    case BSIM3_NRS:
        here->BSIM3sourceSquares = value->rValue;
        here->BSIM3sourceSquaresGiven = TRUE;
        break;
    case BSIM3_NRD:
        here->BSIM3drainSquares = value->rValue;
        here->BSIM3drainSquaresGiven = TRUE;
        break;
    case BSIM3_OFF:
        here->BSIM3off = value->iValue;
        break;
    case BSIM3_IC_VBS:
        here->BSIM3icVBS = value->rValue;
        here->BSIM3icVBSGiven = TRUE;
        break;
    case BSIM3_IC_VDS:
        here->BSIM3icVDS = value->rValue;
        here->BSIM3icVDSGiven = TRUE;
        break;
    case BSIM3_IC_VGS:
        here->BSIM3icVGS = value->rValue;
        here->BSIM3icVGSGiven = TRUE;
        break;
    case BSIM3_NQSMOD:
        here->BSIM3nqsMod = value->iValue;
        here->BSIM3nqsModGiven = TRUE;
        break;
    case BSIM3_IC:
        switch(value->v.numValue)
        {
        case 3:
            here->BSIM3icVBS = *(value->v.vec.rVec+2);
            here->BSIM3icVBSGiven = TRUE;
        case 2:
            here->BSIM3icVGS = *(value->v.vec.rVec+1);
            here->BSIM3icVGSGiven = TRUE;
        case 1:
            here->BSIM3icVDS = *(value->v.vec.rVec);
            here->BSIM3icVDSGiven = TRUE;
            data->cleanup();
            break;
        default:
            data->cleanup();
            return(E_BADPARM);
        }
        break;
    default:
        return(E_BADPARM);
    }
    return(OK);
}

