
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
JJdev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sJJmodel *model = static_cast<sJJmodel*>(genmod); 
    IFvalue *value = &data->v;

    switch (param) {
    case JJ_MOD_PI:
        model->JJpi = (value->iValue != 0);
        model->JJpiGiven = true;
        break;
    case JJ_MOD_RT:
        model->JJrtype = value->iValue;
        model->JJrtypeGiven = true;
        break;
    case JJ_MOD_IC:
        model->JJictype = value->iValue;
        model->JJictypeGiven = true;
        break;
    case JJ_MOD_VG:
        model->JJvg = value->rValue;
        model->JJvgGiven = true;
        break;
    case JJ_MOD_DV:
        model->JJdelv = value->rValue;
        model->JJdelvGiven = true;
        break;
    case JJ_MOD_CRT:
        model->JJcriti = value->rValue;
        model->JJcritiGiven = true;
        break;
    case JJ_MOD_CAP:
        model->JJcap = value->rValue;
        model->JJcapGiven = true;
        break;
    case JJ_MOD_CMU:
        model->JJcmu = value->rValue;
        model->JJcmuGiven = true;
        break;
    case JJ_MOD_VM:
        model->JJvm = value->rValue;
        model->JJvmGiven = true;
        break;
    case JJ_MOD_R0:
        model->JJr0 = value->rValue;
        model->JJr0Given = true;
        break;
    case JJ_MOD_ICR:
        model->JJicrn = value->rValue;
        model->JJicrnGiven = true;
        break;
    case JJ_MOD_RN:
        model->JJrn = value->rValue;
        model->JJrnGiven = true;
        break;
    case JJ_MOD_GMU:
        model->JJgmu = value->rValue;
        model->JJgmuGiven = true;
        break;
    case JJ_MOD_NOISE:
        model->JJnoise = value->rValue;
        model->JJnoiseGiven = true;
        break;
    case JJ_MOD_CCS:
        model->JJccsens = value->rValue;
        model->JJccsensGiven = true;
        break;
    case JJ_MOD_ICF:
        model->JJicFactor = value->rValue;
        model->JJicfGiven = true;
        break;
    case JJ_MOD_VSHUNT:
        model->JJvShunt = value->rValue;
        model->JJvShuntGiven = true;
        break;
#ifdef NEWLSH
    case JJ_MOD_LSH0:
        model->JJlsh0 = value->rValue;
        model->JJlsh0Given = true;
        break;
    case JJ_MOD_LSH1:
        model->JJlsh1 = value->rValue;
        model->JJlsh1Given = true;
        break;
#endif
    case JJ_MOD_TSFACT:
        model->JJtsfact = value->rValue;
        model->JJtsfactGiven = true;
        break;
    case JJ_MOD_JJ:
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}
