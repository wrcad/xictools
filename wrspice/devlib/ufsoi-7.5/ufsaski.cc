
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
Copyright 1997 University of Florida.  All rights reserved.
Author: Min-Chie Jeng (For SPICE3E2)
File: ufsask.c
**********/

#include "ufsdefs.h"

// This function is the interface used to obtain parameters from the
// device instance.  The instance pointer and parameter number are
// passed, and the result is returned in the IFdata struct.
// Only parameters with the IFask flag set get here.

// SRW -- add data->dype initialization

int
UFSdev::askInst(const sCKT*, const sGENinstance *geninst, int which,
    IFdata *data)
{
    const sUFSinstance *here = (const sUFSinstance*)geninst;
    IFvalue *value = &data->v;
    struct ufsAPI_InstData *pInst;
    struct ufsAPI_OPData *pOpInfo;
    int Error;
    double ParamValue;

    pInst = here->pInst;
    pOpInfo = here->pOpInfo;
    if ((pInst == NULL) || (pOpInfo == NULL))
        return(E_BADPARM);

    switch(which) 
    {   case UFS_L:
        case UFS_W:
        case UFS_M:
        case UFS_AS:
        case UFS_AD:
        case UFS_AB:
        case UFS_PSJ:          /* 4.5 */
        case UFS_PDJ:          /* 4.5 */
        case UFS_NRS:
        case UFS_NRD:
        case UFS_NRB:
        case UFS_RTH:
        case UFS_CTH:
        case UFS_TEMP:
            Error = ufsGetInstParam(pInst, which, &ParamValue);
            if (Error)
                return(E_BADPARM);
            data->type = IF_REAL;
            value->rValue = ParamValue;
            return(OK);
        /* case UFS_BJT:
            Error = ufsGetInstParam(pInst, which, &ParamValue);
            if (Error)
                return(E_BADPARM);
            value->iValue = (int) ParamValue;
            return(OK);                                             4.5 */

        case UFS_OFF:
            data->type = IF_INTEGER;
            value->iValue = here->UFSoff;
            return(OK);
        case UFS_IC_VBS:
            data->type = IF_REAL;
            value->rValue = here->UFSicVBS;
            return(OK);
        case UFS_IC_VDS:
            data->type = IF_REAL;
            value->rValue = here->UFSicVDS;
            return(OK);
        case UFS_IC_VGFS:
            data->type = IF_REAL;
            value->rValue = here->UFSicVGFS;
            return(OK);
        case UFS_IC_VGBS:
            data->type = IF_REAL;
            value->rValue = here->UFSicVGBS;
            return(OK);
        case UFS_DNODE:
            data->type = IF_INTEGER;
            value->iValue = here->UFSdNode;
            return(OK);
        case UFS_GNODE:
            data->type = IF_INTEGER;
            value->iValue = here->UFSgNode;
            return(OK);
        case UFS_SNODE:
            data->type = IF_INTEGER;
            value->iValue = here->UFSsNode;
            return(OK);
        case UFS_BNODE:
            data->type = IF_INTEGER;
            value->iValue = here->UFSbNode;
            return(OK);
        case UFS_BGNODE:
            data->type = IF_INTEGER;
            value->iValue = here->UFSbgNode;
            return(OK);
        case UFS_TNODE:
            data->type = IF_INTEGER;
            value->iValue = here->UFStNode;
            return(OK);
        case UFS_DNODEPRIME:
            data->type = IF_INTEGER;
            value->iValue = here->UFSdNodePrime;
            return(OK);
        case UFS_SNODEPRIME:
            data->type = IF_INTEGER;
            value->iValue = here->UFSsNodePrime;
            return(OK);
        case UFS_BNODEPRIME:
            data->type = IF_INTEGER;
            value->iValue = here->UFSbNodePrime;
            return(OK);

        /* OP point parameters */
        default:
            Error = ufsGetOpParam(pOpInfo, which, &ParamValue);
            if (Error)
                return(E_BADPARM);
            data->type = IF_REAL;
            value->rValue = ParamValue;
    }
    return(OK);
}
