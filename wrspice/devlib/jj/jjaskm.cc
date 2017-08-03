
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

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Author: 1992 Stephen R. Whiteley
****************************************************************************/

#include "jjdefs.h"


int
JJdev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sJJmodel *model = static_cast<const sJJmodel*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case JJ_MOD_PI:
        value->iValue = model->JJpi;
        data->type = IF_INTEGER;
        break;
    case JJ_MOD_RT:
        value->iValue = model->JJrtype;
        data->type = IF_INTEGER;
        break;
    case JJ_MOD_IC:
        value->iValue = model->JJictype;
        data->type = IF_INTEGER;
        break;
    case JJ_MOD_VG:
        value->rValue = model->JJvg;
        break;
    case JJ_MOD_DV:
        value->rValue = model->JJdelv;
        break;
    case JJ_MOD_CRT:
        value->rValue = model->JJcriti;
        break;
    case JJ_MOD_CAP:
        value->rValue = model->JJcap;
        break;
    case JJ_MOD_R0:
        value->rValue = model->JJr0;
        break;
    case JJ_MOD_RN:
        value->rValue = model->JJrn;
        break;
    case JJ_MOD_CCS:
        value->rValue = model->JJccsens;
        break;
    case JJ_MQUEST_VL:
        value->rValue = model->JJvless;
        break;
    case JJ_MQUEST_VM:
        value->rValue = model->JJvmore;
        break;
    case JJ_MQUEST_VDP:
        value->rValue = model->JJvdpbak;
        break;
    case JJ_MOD_ICF:
        value->rValue = model->JJicFactor;
        break;
    case JJ_MOD_VSHUNT:
        value->rValue = model->JJvShunt;
        break;
    case JJ_MOD_JJ:
        value->iValue = 1;
        data->type = IF_INTEGER;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}



