
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
    case JJ_MOD_CPIC:
        value->rValue = model->JJcpic;
        break;
    case JJ_MOD_CMU:
        value->rValue = model->JJcmu;
        break;
    case JJ_MOD_VM:
        value->rValue = model->JJvm;
        break;
    case JJ_MOD_R0:
        value->rValue = model->JJr0;
        break;
    case JJ_MOD_ICR:
        value->rValue = model->JJicrn;
        break;
    case JJ_MOD_RN:
        value->rValue = model->JJrn;
        break;
    case JJ_MOD_GMU:
        value->rValue = model->JJgmu;
        break;
    case JJ_MOD_NOISE:
        value->rValue = model->JJnoise;
        break;
    case JJ_MOD_CCS:
        value->rValue = model->JJccsens;
        break;
    case JJ_MOD_ICF:
        value->rValue = model->JJicFactor;
        break;
    case JJ_MOD_VSHUNT:
        value->rValue = model->JJvShunt;
        break;
#ifdef NEWLSH
    case JJ_MOD_LSH0:
        value->rValue = model->JJlsh0;
        break;
    case JJ_MOD_LSH1:
        value->rValue = model->JJlsh1;
        break;
#endif
    case JJ_MOD_TSFACT:
        value->rValue = model->JJtsfact;
        break;
    case JJ_MOD_TSACCL:
        value->rValue = model->JJtsaccl;
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
    case JJ_MOD_JJ:
        value->iValue = 1;
        data->type = IF_INTEGER;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}

