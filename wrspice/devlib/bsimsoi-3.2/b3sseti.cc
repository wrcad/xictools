
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
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
File: b3soipar.c          98/5/01
Modified by Pin Su      99/2/15
Modified by Pin Su      01/2/15
Modified by Hui Wan     02/11/12
Modified by Pin Su      03/07/30
**********/

#include "b3sdefs.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


int
B3SOIdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
    sB3SOIinstance *here = static_cast<sB3SOIinstance*>(geninst);
    IFvalue *value = &data->v;

    switch(param)
    {
    case B3SOI_W:
        here->B3SOIw = value->rValue;
        here->B3SOIwGiven = TRUE;
        break;
    case B3SOI_L:
        here->B3SOIl = value->rValue;
        here->B3SOIlGiven = TRUE;
        break;
    case B3SOI_AS:
        here->B3SOIsourceArea = value->rValue;
        here->B3SOIsourceAreaGiven = TRUE;
        break;
    case B3SOI_AD:
        here->B3SOIdrainArea = value->rValue;
        here->B3SOIdrainAreaGiven = TRUE;
        break;
    case B3SOI_PS:
        here->B3SOIsourcePerimeter = value->rValue;
        here->B3SOIsourcePerimeterGiven = TRUE;
        break;
    case B3SOI_PD:
        here->B3SOIdrainPerimeter = value->rValue;
        here->B3SOIdrainPerimeterGiven = TRUE;
        break;
    case B3SOI_NRS:
        here->B3SOIsourceSquares = value->rValue;
        here->B3SOIsourceSquaresGiven = TRUE;
        break;
    case B3SOI_NRD:
        here->B3SOIdrainSquares = value->rValue;
        here->B3SOIdrainSquaresGiven = TRUE;
        break;
    case B3SOI_OFF:
        here->B3SOIoff = value->iValue;
        here->B3SOIoffGiven = TRUE;
        break;
    case B3SOI_IC_VBS:
        here->B3SOIicVBS = value->rValue;
        here->B3SOIicVBSGiven = TRUE;
        break;
    case B3SOI_IC_VDS:
        here->B3SOIicVDS = value->rValue;
        here->B3SOIicVDSGiven = TRUE;
        break;
    case B3SOI_IC_VGS:
        here->B3SOIicVGS = value->rValue;
        here->B3SOIicVGSGiven = TRUE;
        break;
    case B3SOI_IC_VES:
        here->B3SOIicVES = value->rValue;
        here->B3SOIicVESGiven = TRUE;
        break;
    case B3SOI_IC_VPS:
        here->B3SOIicVPS = value->rValue;
        here->B3SOIicVPSGiven = TRUE;
        break;
    case B3SOI_BJTOFF:
        here->B3SOIbjtoff = value->iValue;
        here->B3SOIbjtoffGiven= TRUE;
        break;
    case B3SOI_DEBUG:
        here->B3SOIdebugMod = value->iValue;
        here->B3SOIdebugModGiven= TRUE;
        break;
    case B3SOI_RTH0:
        here->B3SOIrth0= value->rValue;
        here->B3SOIrth0Given = TRUE;
        break;
    case B3SOI_CTH0:
        here->B3SOIcth0= value->rValue;
        here->B3SOIcth0Given = TRUE;
        break;
    case B3SOI_NRB:
        here->B3SOIbodySquares = value->rValue;
        here->B3SOIbodySquaresGiven = TRUE;
        break;
    case B3SOI_FRBODY:
        here->B3SOIfrbody = value->rValue;
        here->B3SOIfrbodyGiven = TRUE;
        break;

    case B3SOI_SOIMOD:
        here->B3SOIsoiMod = value->iValue;
        here->B3SOIsoiModGiven = TRUE;
        break; /* v3.2 */

        /* v3.1 wanh added rgate */
    case B3SOI_RGATEMOD:
        here->B3SOIrgateMod = value->iValue;
        here->B3SOIrgateModGiven = TRUE;
        break;
        /* v3.1 wanh added rgate end */


        /* v2.0 release */
    case B3SOI_NBC:
        here->B3SOInbc = value->rValue;
        here->B3SOInbcGiven = TRUE;
        break;
    case B3SOI_NSEG:
        here->B3SOInseg = value->rValue;
        here->B3SOInsegGiven = TRUE;
        break;
    case B3SOI_PDBCP:
        here->B3SOIpdbcp = value->rValue;
        here->B3SOIpdbcpGiven = TRUE;
        break;
    case B3SOI_PSBCP:
        here->B3SOIpsbcp = value->rValue;
        here->B3SOIpsbcpGiven = TRUE;
        break;
    case B3SOI_AGBCP:
        here->B3SOIagbcp = value->rValue;
        here->B3SOIagbcpGiven = TRUE;
        break;
    case B3SOI_AEBCP:
        here->B3SOIaebcp = value->rValue;
        here->B3SOIaebcpGiven = TRUE;
        break;
    case B3SOI_VBSUSR:
        here->B3SOIvbsusr = value->rValue;
        here->B3SOIvbsusrGiven = TRUE;
        break;
    case B3SOI_TNODEOUT:
        here->B3SOItnodeout = value->iValue;
        here->B3SOItnodeoutGiven = TRUE;
        break;


    case B3SOI_IC:
        switch(value->v.numValue)
        {
        case 5:
            here->B3SOIicVPS = *(value->v.vec.rVec+4);
            here->B3SOIicVPSGiven = TRUE;
            // fallthrough
        case 4:
            here->B3SOIicVES = *(value->v.vec.rVec+3);
            here->B3SOIicVESGiven = TRUE;
            // fallthrough
        case 3:
            here->B3SOIicVBS = *(value->v.vec.rVec+2);
            here->B3SOIicVBSGiven = TRUE;
            // fallthrough
        case 2:
            here->B3SOIicVGS = *(value->v.vec.rVec+1);
            here->B3SOIicVGSGiven = TRUE;
            // fallthrough
        case 1:
            here->B3SOIicVDS = *(value->v.vec.rVec);
            here->B3SOIicVDSGiven = TRUE;
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

