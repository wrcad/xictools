
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

/*
 * Author: 2000 Wladek Grabinski; EKV v2.6 Model Upgrade
 * Author: 1997 Eckhard Brass;    EKV v2.5 Model Implementation
 *     (C) 1990 Regents of the University of California. Spice3 Format
 */

#include "ekvdefs.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


int
EKVdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
    sEKVinstance *here = static_cast<sEKVinstance*>(geninst);
    IFvalue *value = &data->v;

    switch(param) {
    case EKV_TEMP:
        here->EKVtemp = value->rValue+CONSTCtoK;
        here->EKVtempGiven = TRUE;
        return(OK);
    case EKV_W:
        here->EKVw = value->rValue;
        here->EKVwGiven = TRUE;
        return(OK);
    case EKV_L:
        here->EKVl = value->rValue;
        here->EKVlGiven = TRUE;
        return(OK);
    case EKV_AS:
        here->EKVsourceArea = value->rValue;
        here->EKVsourceAreaGiven = TRUE;
        return(OK);
    case EKV_AD:
        here->EKVdrainArea = value->rValue;
        here->EKVdrainAreaGiven = TRUE;
        return(OK);
    case EKV_PS:
        here->EKVsourcePerimiter = value->rValue;
        here->EKVsourcePerimiterGiven = TRUE;
        return(OK);
    case EKV_PD:
        here->EKVdrainPerimiter = value->rValue;
        here->EKVdrainPerimiterGiven = TRUE;
        return(OK);
    case EKV_NRS:
        here->EKVsourceSquares = value->rValue;
        here->EKVsourceSquaresGiven = TRUE;
        return(OK);
    case EKV_NRD:
        here->EKVdrainSquares = value->rValue;
        here->EKVdrainSquaresGiven = TRUE;
        return(OK);
    case EKV_OFF:
        here->EKVoff = value->iValue;
        return(OK);
    case EKV_IC_VBS:
        here->EKVicVBS = value->rValue;
        here->EKVicVBSGiven = TRUE;
        return(OK);
    case EKV_IC_VDS:
        here->EKVicVDS = value->rValue;
        here->EKVicVDSGiven = TRUE;
        return(OK);
    case EKV_IC_VGS:
        here->EKVicVGS = value->rValue;
        here->EKVicVGSGiven = TRUE;
        return(OK);
    case EKV_IC:
        switch(value->v.numValue){
        case 3:
            here->EKVicVBS = *(value->v.vec.rVec+2);
            here->EKVicVBSGiven = TRUE;
            // fallthrough
        case 2:
            here->EKVicVGS = *(value->v.vec.rVec+1);
            here->EKVicVGSGiven = TRUE;
            // fallthrough
        case 1:
            here->EKVicVDS = *(value->v.vec.rVec);
            here->EKVicVDSGiven = TRUE;
            data->cleanup();
            return(OK);
        default:
            data->cleanup();
            return(E_BADPARM);
        }
        break;
#ifdef HAS_SENSE2
    case EKV_L_SENS:
        if(value->iValue) {
            here->EKVsenParmNo = 1;
            here->EKVsens_l = 1;
        }
        return(OK);
    case EKV_W_SENS:
        if(value->iValue) {
            here->EKVsenParmNo = 1;
            here->EKVsens_w = 1;
        }
        return(OK);
#endif
    default:
        return(E_BADPARM);
    }
    return(OK);
}

