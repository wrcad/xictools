
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2019 Whiteley Research Inc., all rights reserved.       *
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

#include "tjmdefs.h"


int
TJMdev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sTJMmodel *model = static_cast<sTJMmodel*>(genmod); 
    IFvalue *value = &data->v;

    switch (param) {
    case TJM_MOD_COEFFS:
        if (value->sValue) {
            char *t = new char[strlen(value->sValue)+1];
            strcpy(t, value->sValue);
            if (model->tjm_coeffsGiven)
                delete [] model->tjm_coeffs;
            model->tjm_coeffs = t;
            model->tjm_coeffsGiven = true;
        }
        break;
    case TJM_MOD_CTP:
        model->TJMictype = value->iValue;
        model->TJMictypeGiven = true;
        break;
    case TJM_MOD_DEL1:
        model->TJMdel1 = value->rValue;
        model->TJMdel1Given = true;
        break;
    case TJM_MOD_DEL2:
        model->TJMdel2 = value->rValue;
        model->TJMdel2Given = true;
        break;
    case TJM_MOD_VG:
        model->TJMvg = value->rValue;
        model->TJMvgGiven = true;
        break;
    case TJM_MOD_TEMP:
        model->TJMtemp = value->rValue;
        model->TJMtempGiven = true;
        break;
    case TJM_MOD_TNOM:
        model->TJMtnom = value->rValue;
        model->TJMtnomGiven = true;
        break;
    case TJM_MOD_TC:
        model->TJMtc1 = value->rValue;
        model->TJMtc2 = value->rValue;
        model->TJMtc1Given = true;
        model->TJMtc2Given = true;
        break;
    case TJM_MOD_TC1:
        if (!model->TJMtc1Given) {
            model->TJMtc1 = value->rValue;
            model->TJMtc1Given = true;
        }
        break;
    case TJM_MOD_TC2:
        if (!model->TJMtc2Given) {
            model->TJMtc2 = value->rValue;
            model->TJMtc2Given = true;
        }
        break;
    case TJM_MOD_TDEBYE:
        model->TJMtdebye1 = value->rValue;
        model->TJMtdebye2 = value->rValue;
        model->TJMtdebye1Given = true;
        model->TJMtdebye2Given = true;
        break;
    case TJM_MOD_TDEBYE1:
        if (!model->TJMtdebye1Given) {
            model->TJMtdebye1 = value->rValue;
            model->TJMtdebye1Given = true;
        }
        break;
    case TJM_MOD_TDEBYE2:
        if (!model->TJMtdebye2Given) {
            model->TJMtdebye2 = value->rValue;
            model->TJMtdebye2Given = true;
        }
        break;
    case TJM_MOD_SMF:
        model->TJMsmf = value->rValue;
        model->TJMsmfGiven = true;
        break;
    case TJM_MOD_CRT:
        model->TJMcriti = value->rValue;
        model->TJMcritiGiven = true;
        break;
    case TJM_MOD_CAP:
        model->TJMcap = value->rValue;
        model->TJMcapGiven = true;
        break;
    case TJM_MOD_CPIC:
        model->TJMcpic = value->rValue;
        model->TJMcpicGiven = true;
        break;
    case TJM_MOD_CMU:
        model->TJMcmu = value->rValue;
        model->TJMcmuGiven = true;
        break;
    case TJM_MOD_VM:
        model->TJMvm = value->rValue;
        model->TJMvmGiven = true;
        break;
    case TJM_MOD_R0:
        model->TJMr0 = value->rValue;
        model->TJMr0Given = true;
        break;
    case TJM_MOD_GMU:
        model->TJMgmu = value->rValue;
        model->TJMgmuGiven = true;
        break;
    case TJM_MOD_NOISE:
        model->TJMnoise = value->rValue;
        model->TJMnoiseGiven = true;
        break;
    case TJM_MOD_ICF:
        model->TJMicFactor = value->rValue;
        model->TJMicfGiven = true;
        break;
    case TJM_MOD_VSHUNT:
        model->TJMvShunt = value->rValue;
        model->TJMvShuntGiven = true;
        break;
    case TJM_MOD_FORCE:
        model->TJMforceGiven = true;
        break;
#ifdef NEWLSH
    case TJM_MOD_LSH0:
        model->TJMlsh0 = value->rValue;
        model->TJMlsh0Given = true;
        break;
    case TJM_MOD_LSH1:
        model->TJMlsh1 = value->rValue;
        model->TJMlsh1Given = true;
        break;
#endif
    case TJM_MOD_TSFACT:
        model->TJMtsfact = value->rValue;
        model->TJMtsfactGiven = true;
        break;
    case TJM_MOD_TSACCL:
        model->TJMtsaccl = value->rValue;
        model->TJMtsacclGiven = true;
        break;
    case TJM_MOD_TJM:
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}
