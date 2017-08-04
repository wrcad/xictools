
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
Based on jfetmpar.c
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles

Modified to add PS model and new parameter definitions ( Anthony E. Parker )
   Copyright 1994  Macquarie University, Sydney Australia.
   10 Feb 1994: Added call to jfetparm.h
**********/

#include "jfet2defs.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


int
JFET2dev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sJFET2model *model = static_cast<sJFET2model*>(genmod);
    IFvalue *value = &data->v;

    switch(param) {
        case JFET2_MOD_TNOM:
            model->JFET2tnomGiven = TRUE;
            model->JFET2tnom = value->rValue+CONSTCtoK;
            break;

        // SRW -- extracted from jfet2parm.h
        case JFET2_MOD_ACGAM   :model->JFET2acgamGiven = TRUE; model->JFET2acgam = value->rValue; break;
        case JFET2_MOD_AF      :model->JFET2fNexpGiven = TRUE; model->JFET2fNexp = value->rValue; break;
        case JFET2_MOD_BETA    :model->JFET2betaGiven = TRUE; model->JFET2beta  = value->rValue; break;
        case JFET2_MOD_CDS     :model->JFET2capDSGiven = TRUE; model->JFET2capds = value->rValue; break;
        case JFET2_MOD_CGD     :model->JFET2capGDGiven = TRUE; model->JFET2capgd = value->rValue; break;
        case JFET2_MOD_CGS     :model->JFET2capGSGiven = TRUE; model->JFET2capgs = value->rValue; break;
        case JFET2_MOD_DELTA   :model->JFET2deltaGiven = TRUE; model->JFET2delta = value->rValue; break;
        case JFET2_MOD_HFETA   :model->JFET2hfetaGiven = TRUE; model->JFET2hfeta = value->rValue; break;
        case JFET2_MOD_HFE1    :model->JFET2hfe1Given = TRUE; model->JFET2hfe1  = value->rValue; break;
        case JFET2_MOD_HFE2    :model->JFET2hfe2Given = TRUE; model->JFET2hfe2  = value->rValue; break;
        case JFET2_MOD_HFG1    :model->JFET2hfg1Given = TRUE; model->JFET2hfg1  = value->rValue; break;
        case JFET2_MOD_HFG2    :model->JFET2hfg2Given = TRUE; model->JFET2hfg2  = value->rValue; break;
        case JFET2_MOD_MVST    :model->JFET2mvstGiven = TRUE; model->JFET2mvst  = value->rValue; break;
        case JFET2_MOD_MXI     :model->JFET2mxiGiven = TRUE; model->JFET2mxi    = value->rValue; break;
        case JFET2_MOD_FC      :model->JFET2fcGiven = TRUE; model->JFET2fc      = value->rValue; break;
        case JFET2_MOD_IBD     :model->JFET2ibdGiven = TRUE; model->JFET2ibd    = value->rValue; break;
        case JFET2_MOD_IS      :model->JFET2isGiven = TRUE; model->JFET2is      = value->rValue; break;
        case JFET2_MOD_KF      :model->JFET2kfGiven = TRUE; model->JFET2fNcoef  = value->rValue; break;
        case JFET2_MOD_LAMBDA  :model->JFET2lamGiven = TRUE; model->JFET2lambda = value->rValue; break;
        case JFET2_MOD_LFGAM   :model->JFET2lfgamGiven = TRUE; model->JFET2lfgam = value->rValue; break;
        case JFET2_MOD_LFG1    :model->JFET2lfg1Given = TRUE; model->JFET2lfg1  = value->rValue; break;
        case JFET2_MOD_LFG2    :model->JFET2lfg2Given = TRUE; model->JFET2lfg2  = value->rValue; break;
        case JFET2_MOD_N       :model->JFET2nGiven = TRUE; model->JFET2n        = value->rValue; break;
        case JFET2_MOD_P       :model->JFET2pGiven = TRUE; model->JFET2p        = value->rValue; break;
        case JFET2_MOD_PB      :model->JFET2phiGiven = TRUE; model->JFET2phi    = value->rValue; break;
        case JFET2_MOD_Q       :model->JFET2qGiven = TRUE; model->JFET2q        = value->rValue; break;
        case JFET2_MOD_RD      :model->JFET2rdGiven = TRUE; model->JFET2rd      = value->rValue; break;
        case JFET2_MOD_RS      :model->JFET2rsGiven = TRUE; model->JFET2rs      = value->rValue; break;
        case JFET2_MOD_TAUD    :model->JFET2taudGiven = TRUE; model->JFET2taud  = value->rValue; break;
        case JFET2_MOD_TAUG    :model->JFET2taugGiven = TRUE; model->JFET2taug  = value->rValue; break;
        case JFET2_MOD_VBD     :model->JFET2vbdGiven = TRUE; model->JFET2vbd    = value->rValue; break;
        case JFET2_MOD_VER     :model->JFET2verGiven = TRUE; model->JFET2ver    = value->rValue; break;
        case JFET2_MOD_VST     :model->JFET2vstGiven = TRUE; model->JFET2vst    = value->rValue; break;
        case JFET2_MOD_VTO     :model->JFET2vtoGiven = TRUE; model->JFET2vto    = value->rValue; break;
        case JFET2_MOD_XC      :model->JFET2xcGiven = TRUE; model->JFET2xc      = value->rValue; break;
        case JFET2_MOD_XI      :model->JFET2xiGiven = TRUE; model->JFET2xi      = value->rValue; break;
        case JFET2_MOD_Z       :model->JFET2zGiven = TRUE; model->JFET2z        = value->rValue; break;
        case JFET2_MOD_HFGAM   :model->JFET2hfgGiven = TRUE; model->JFET2hfgam  = value->rValue; break;

        case JFET2_MOD_NJF:
            if(value->iValue) {
                model->JFET2type = NJF;
            }
            break;
        case JFET2_MOD_PJF:
            if(value->iValue) {
                model->JFET2type = PJF;
            }
            break;
        default:
            return(E_BADPARM);
    }
    return(OK);
}
