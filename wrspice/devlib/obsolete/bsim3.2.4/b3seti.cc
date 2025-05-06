
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
            // fallthrough
        case 2:
            here->BSIM3icVGS = *(value->v.vec.rVec+1);
            here->BSIM3icVGSGiven = TRUE;
            // fallthrough
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

