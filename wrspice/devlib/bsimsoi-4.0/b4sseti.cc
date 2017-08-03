
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

/***  B4SOI 11/30/2005 Xuemei (Jane) Xi Release   ***/

/**********
 * Copyright 2005 Regents of the University of California.  All rights reserved.
 * Authors: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
 * Authors: 1999-2004 Pin Su, Hui Wan, Wei Jin, b3soipar.c
 * Authors: 2005- Hui Wan, Xuemei Xi, Ali Niknejad, Chenming Hu.
 * File: b4soipar.c
 * Modified by Hui Wan, Xuemei Xi 11/30/2005
 **********/

#include "b4sdefs.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


int
B4SOIdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
    sB4SOIinstance *here = static_cast<sB4SOIinstance*>(geninst);
    IFvalue *value = &data->v;

    switch(param)
    {
    case B4SOI_W:
        here->B4SOIw = value->rValue;
        here->B4SOIwGiven = TRUE;
        break;
    case B4SOI_L:
        here->B4SOIl = value->rValue;
        here->B4SOIlGiven = TRUE;
        break;
    case B4SOI_AS:
        here->B4SOIsourceArea = value->rValue;
        here->B4SOIsourceAreaGiven = TRUE;
        break;
    case B4SOI_AD:
        here->B4SOIdrainArea = value->rValue;
        here->B4SOIdrainAreaGiven = TRUE;
        break;
    case B4SOI_PS:
        here->B4SOIsourcePerimeter = value->rValue;
        here->B4SOIsourcePerimeterGiven = TRUE;
        break;
    case B4SOI_PD:
        here->B4SOIdrainPerimeter = value->rValue;
        here->B4SOIdrainPerimeterGiven = TRUE;
        break;
    case B4SOI_NRS:
        here->B4SOIsourceSquares = value->rValue;
        here->B4SOIsourceSquaresGiven = TRUE;
        break;
    case B4SOI_NRD:
        here->B4SOIdrainSquares = value->rValue;
        here->B4SOIdrainSquaresGiven = TRUE;
        break;
    case B4SOI_OFF:
        here->B4SOIoff = value->iValue;
        here->B4SOIoffGiven = TRUE;
        break;
    case B4SOI_IC_VBS:
        here->B4SOIicVBS = value->rValue;
        here->B4SOIicVBSGiven = TRUE;
        break;
    case B4SOI_IC_VDS:
        here->B4SOIicVDS = value->rValue;
        here->B4SOIicVDSGiven = TRUE;
        break;
    case B4SOI_IC_VGS:
        here->B4SOIicVGS = value->rValue;
        here->B4SOIicVGSGiven = TRUE;
        break;
    case B4SOI_IC_VES:
        here->B4SOIicVES = value->rValue;
        here->B4SOIicVESGiven = TRUE;
        break;
    case B4SOI_IC_VPS:
        here->B4SOIicVPS = value->rValue;
        here->B4SOIicVPSGiven = TRUE;
        break;
    case B4SOI_BJTOFF:
        here->B4SOIbjtoff = value->iValue;
        here->B4SOIbjtoffGiven= TRUE;
        break;
    case B4SOI_DEBUG:
        here->B4SOIdebugMod = value->iValue;
        here->B4SOIdebugModGiven= TRUE;
        break;
    case B4SOI_RTH0:
        here->B4SOIrth0= value->rValue;
        here->B4SOIrth0Given = TRUE;
        break;
    case B4SOI_CTH0:
        here->B4SOIcth0= value->rValue;
        here->B4SOIcth0Given = TRUE;
        break;
    case B4SOI_NRB:
        here->B4SOIbodySquares = value->rValue;
        here->B4SOIbodySquaresGiven = TRUE;
        break;
    case B4SOI_FRBODY:
        here->B4SOIfrbody = value->rValue;
        here->B4SOIfrbodyGiven = TRUE;
        break;

        /* v4.0 added */
    case B4SOI_RBSB:
        here->B4SOIrbsb = value->rValue;
        here->B4SOIrbsbGiven = TRUE;
        break;
    case B4SOI_RBDB:
        here->B4SOIrbdb = value->rValue;
        here->B4SOIrbdbGiven = TRUE;
        break;
    case B4SOI_SA:
        here->B4SOIsa = value->rValue;
        here->B4SOIsaGiven = TRUE;
        break;
    case B4SOI_SB:
        here->B4SOIsb = value->rValue;
        here->B4SOIsbGiven = TRUE;
        break;
    case B4SOI_SD:
        here->B4SOIsd = value->rValue;
        here->B4SOIsdGiven = TRUE;
        break;
    case B4SOI_RBODYMOD:
        here->B4SOIrbodyMod = value->iValue;
        here->B4SOIrbodyModGiven = TRUE;
        break;
    case B4SOI_NF:
        here->B4SOInf = value->rValue;
        here->B4SOInfGiven = TRUE;
        break;
    case B4SOI_DELVTO:
        here->B4SOIdelvto = value->rValue;
        here->B4SOIdelvtoGiven = TRUE;
        break;

        /* v4.0 added end */

    case B4SOI_SOIMOD:
        here->B4SOIsoiMod = value->iValue;
        here->B4SOIsoiModGiven = TRUE;
        break; /* v3.2 */

        /* v3.1 added rgate */
    case B4SOI_RGATEMOD:
        here->B4SOIrgateMod = value->iValue;
        here->B4SOIrgateModGiven = TRUE;
        break;
        /* v3.1 added rgate end */


        /* v2.0 release */
    case B4SOI_NBC:
        here->B4SOInbc = value->rValue;
        here->B4SOInbcGiven = TRUE;
        break;
    case B4SOI_NSEG:
        here->B4SOInseg = value->rValue;
        here->B4SOInsegGiven = TRUE;
        break;
    case B4SOI_PDBCP:
        here->B4SOIpdbcp = value->rValue;
        here->B4SOIpdbcpGiven = TRUE;
        break;
    case B4SOI_PSBCP:
        here->B4SOIpsbcp = value->rValue;
        here->B4SOIpsbcpGiven = TRUE;
        break;
    case B4SOI_AGBCP:
        here->B4SOIagbcp = value->rValue;
        here->B4SOIagbcpGiven = TRUE;
        break;
    case B4SOI_AGBCPD:
        here->B4SOIagbcpd = value->rValue;
        here->B4SOIagbcpdGiven = TRUE;
        break;
    case B4SOI_AEBCP:
        here->B4SOIaebcp = value->rValue;
        here->B4SOIaebcpGiven = TRUE;
        break;
    case B4SOI_VBSUSR:
        here->B4SOIvbsusr = value->rValue;
        here->B4SOIvbsusrGiven = TRUE;
        break;
    case B4SOI_TNODEOUT:
        here->B4SOItnodeout = value->iValue;
        here->B4SOItnodeoutGiven = TRUE;
        break;


    case B4SOI_IC:
        switch(value->v.numValue)
        {
        case 5:
            here->B4SOIicVPS = *(value->v.vec.rVec+4);
            here->B4SOIicVPSGiven = TRUE;
            // fallthrough
        case 4:
            here->B4SOIicVES = *(value->v.vec.rVec+3);
            here->B4SOIicVESGiven = TRUE;
            // fallthrough
        case 3:
            here->B4SOIicVBS = *(value->v.vec.rVec+2);
            here->B4SOIicVBSGiven = TRUE;
            // fallthrough
        case 2:
            here->B4SOIicVGS = *(value->v.vec.rVec+1);
            here->B4SOIicVGSGiven = TRUE;
            // fallthrough
        case 1:
            here->B4SOIicVDS = *(value->v.vec.rVec);
            here->B4SOIicVDSGiven = TRUE;
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

