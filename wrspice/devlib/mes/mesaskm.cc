
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mesdefs.h"


int
MESdev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sMESmodel *model = static_cast<const sMESmodel*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch (which) {
    case MES_MOD_VTO:
        value->rValue = model->MESthreshold;
        break;
    case MES_MOD_ALPHA:
        value->rValue = model->MESalpha;
        break;
    case MES_MOD_BETA:
        value->rValue = model->MESbeta;
        break;
    case MES_MOD_LAMBDA:
        value->rValue = model->MESlModulation;
        break;
    case MES_MOD_B:
        value->rValue = model->MESb;
        break;
    case MES_MOD_RD:
        value->rValue = model->MESdrainResist;
        break;
    case MES_MOD_RS:
        value->rValue = model->MESsourceResist;
        break;
    case MES_MOD_CGS:
        value->rValue = model->MEScapGS;
        break;
    case MES_MOD_CGD:
        value->rValue = model->MEScapGD;
        break;
    case MES_MOD_PB:
        value->rValue = model->MESgatePotential;
        break;
    case MES_MOD_IS:
        value->rValue = model->MESgateSatCurrent;
        break;
    case MES_MOD_FC:
        value->rValue = model->MESdepletionCapCoeff;
        break;
    case MES_MOD_DRAINCOND:
        value->rValue = model->MESdrainConduct;
        break;
    case MES_MOD_SOURCECOND:
        value->rValue = model->MESsourceConduct;
        break;
    case MES_MOD_DEPLETIONCAP:
        value->rValue = model->MESdepletionCap;
        break;
    case MES_MOD_VCRIT:
        value->rValue = model->MESvcrit;
        break;
    /* new in ef2 */
    case MES_MOD_TYPE:
        if (model->MEStype == NMF)
            value->sValue = "nmf";
        else
            value->sValue = "pmf";
        data->type = IF_STRING;
        break;
    default:
        return (E_BADPARM);
    }
    return (OK);
}
