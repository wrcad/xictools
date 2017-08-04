
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
Based on jfetmask.c
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1987 Mathew Lew and Thomas L. Quarles

Modified to add PS model and new parameter definitions ( Anthony E. Parker )
   Copyright 1994  Macquarie University, Sydney Australia.
   10 Feb 1994: Added call to jfetparm.h
**********/

#include "jfet2defs.h"


int
JFET2dev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sJFET2model *model = static_cast<const sJFET2model*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch(which) {
    case JFET2_MOD_TNOM:
        value->rValue = model->JFET2tnom-CONSTCtoK;
        return(OK);

    // SRW -- extracted from jfet2parm.h
    case JFET2_MOD_ACGAM   : value->rValue = model->JFET2acgam; return (OK);
    case JFET2_MOD_AF      : value->rValue = model->JFET2fNexp; return (OK);
    case JFET2_MOD_BETA    : value->rValue = model->JFET2beta; return (OK);
    case JFET2_MOD_CDS     : value->rValue = model->JFET2capds; return (OK);
    case JFET2_MOD_CGD     : value->rValue = model->JFET2capgd; return (OK);
    case JFET2_MOD_CGS     : value->rValue = model->JFET2capgs; return (OK);
    case JFET2_MOD_DELTA   : value->rValue = model->JFET2delta; return (OK);
    case JFET2_MOD_HFETA   : value->rValue = model->JFET2hfeta; return (OK);
    case JFET2_MOD_HFE1    : value->rValue = model->JFET2hfe1; return (OK);
    case JFET2_MOD_HFE2    : value->rValue = model->JFET2hfe2; return (OK);
    case JFET2_MOD_HFG1    : value->rValue = model->JFET2hfg1; return (OK);
    case JFET2_MOD_HFG2    : value->rValue = model->JFET2hfg2; return (OK);
    case JFET2_MOD_MVST    : value->rValue = model->JFET2mvst; return (OK);
    case JFET2_MOD_MXI     : value->rValue = model->JFET2mxi; return (OK);
    case JFET2_MOD_FC      : value->rValue = model->JFET2fc; return (OK);
    case JFET2_MOD_IBD     : value->rValue = model->JFET2ibd; return (OK);
    case JFET2_MOD_IS      : value->rValue = model->JFET2is; return (OK);
    case JFET2_MOD_KF      : value->rValue = model->JFET2fNcoef; return (OK);
    case JFET2_MOD_LAMBDA  : value->rValue = model->JFET2lambda; return (OK);
    case JFET2_MOD_LFGAM   : value->rValue = model->JFET2lfgam; return (OK);
    case JFET2_MOD_LFG1    : value->rValue = model->JFET2lfg1; return (OK);
    case JFET2_MOD_LFG2    : value->rValue = model->JFET2lfg2; return (OK);
    case JFET2_MOD_N       : value->rValue = model->JFET2n; return (OK);
    case JFET2_MOD_P       : value->rValue = model->JFET2p; return (OK);
    case JFET2_MOD_PB      : value->rValue = model->JFET2phi; return (OK);
    case JFET2_MOD_Q       : value->rValue = model->JFET2q; return (OK);
    case JFET2_MOD_RD      : value->rValue = model->JFET2rd; return (OK);
    case JFET2_MOD_RS      : value->rValue = model->JFET2rs; return (OK);
    case JFET2_MOD_TAUD    : value->rValue = model->JFET2taud; return (OK);
    case JFET2_MOD_TAUG    : value->rValue = model->JFET2taug; return (OK);
    case JFET2_MOD_VBD     : value->rValue = model->JFET2vbd; return (OK);
    case JFET2_MOD_VER     : value->rValue = model->JFET2ver; return (OK);
    case JFET2_MOD_VST     : value->rValue = model->JFET2vst; return (OK);
    case JFET2_MOD_VTO     : value->rValue = model->JFET2vto; return (OK);
    case JFET2_MOD_XC      : value->rValue = model->JFET2xc; return (OK);
    case JFET2_MOD_XI      : value->rValue = model->JFET2xi; return (OK);
    case JFET2_MOD_Z       : value->rValue = model->JFET2z; return (OK);
    case JFET2_MOD_HFGAM   : value->rValue = model->JFET2hfgam; return (OK);

    case JFET2_MOD_DRAINCONDUCT:
        value->rValue = model->JFET2drainConduct;
        return(OK);
    case JFET2_MOD_SOURCECONDUCT:
        value->rValue = model->JFET2sourceConduct;
        return(OK);
    case JFET2_MOD_TYPE:
        if (model->JFET2type == NJF)
            value->sValue = "njf";
        else
            value->sValue = "pjf";
        data->type = IF_STRING;
        return(OK);
    default:
        return(E_BADPARM);
    }
    /* NOTREACHED */
}

