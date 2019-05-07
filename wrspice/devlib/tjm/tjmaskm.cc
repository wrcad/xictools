
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
TJMdev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sTJMmodel *model = static_cast<const sTJMmodel*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case TJM_MOD_COEFFS:
        value->sValue = model->tjm_coeffs;
        data->type = IF_STRING;
        break;
    case TJM_MOD_RTP:
        value->iValue = model->TJMrtype;
        data->type = IF_INTEGER;
        break;
    case TJM_MOD_CTP:
        value->iValue = model->TJMictype;
        data->type = IF_INTEGER;
        break;
    case TJM_MOD_VG:
        value->rValue = model->TJMvg;
        break;
    case TJM_MOD_CRT:
        value->rValue = model->TJMcriti;
        break;
    case TJM_MOD_CAP:
        value->rValue = model->TJMcap;
        break;
    case TJM_MOD_CPIC:
        value->rValue = model->TJMcpic;
        break;
    case TJM_MOD_CMU:
        value->rValue = model->TJMcmu;
        break;
    case TJM_MOD_VM:
        value->rValue = model->TJMvm;
        break;
    case TJM_MOD_R0:
        value->rValue = model->TJMr0;
        break;
    case TJM_MOD_GMU:
        value->rValue = model->TJMgmu;
        break;
    case TJM_MOD_NOISE:
        value->rValue = model->TJMnoise;
        break;
    case TJM_MOD_ICF:
        value->rValue = model->TJMicFactor;
        break;
    case TJM_MOD_VSHUNT:
        value->rValue = model->TJMvShunt;
        break;
#ifdef NEWLSH
    case TJM_MOD_LSH0:
        value->rValue = model->TJMlsh0;
        break;
    case TJM_MOD_LSH1:
        value->rValue = model->TJMlsh1;
        break;
#endif
    case TJM_MOD_TSFACT:
        value->rValue = model->TJMtsfact;
        break;
    case TJM_MQUEST_VDP:
        value->rValue = model->TJMvdpbak;
        break;
    case TJM_MQUEST_OMEGAJ:
        value->rValue = model->TJMomegaJ;
        break;
    case TJM_MQUEST_BETAC:
        value->rValue = 
            model->TJMvm*model->TJMvm*model->TJMcap/(model->TJMcriti*PHI0_2PI);
        break;
    case TJM_MOD_TJM:
        value->iValue = 1;
        data->type = IF_INTEGER;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}

