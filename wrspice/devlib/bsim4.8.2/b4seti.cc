
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

/******************************************************************************
 *  BSIM4 4.8.2 released by Chetan Kumar Dabhi 01/01/2020                     *
 *  BSIM4 Model Equations                                                     *
 ******************************************************************************

 ******************************************************************************
 *  Copyright (c) 2020 University of California                               *
 *                                                                            *
 *  Project Director: Prof. Chenming Hu.                                      *
 *  Current developers: Chetan Kumar Dabhi   (Ph.D. student, IIT Kanpur)      *
 *                      Prof. Yogesh Chauhan (IIT Kanpur)                     *
 *                      Dr. Pragya Kushwaha  (Postdoc, UC Berkeley)           *
 *                      Dr. Avirup Dasgupta  (Postdoc, UC Berkeley)           *
 *                      Ming-Yen Kao         (Ph.D. student, UC Berkeley)     *
 *  Authors: Gary W. Ng, Weidong Liu, Xuemei Xi, Mohan Dunga, Wenwei Yang     *
 *           Ali Niknejad, Chetan Kumar Dabhi, Yogesh Singh Chauhan,          *
 *           Sayeef Salahuddin, Chenming Hu                                   * 
 ******************************************************************************/

/*
Licensed under Educational Community License, Version 2.0 (the "License");
you may not use this file except in compliance with the License. You may
obtain a copy of the license at
    http://opensource.org/licenses/ECL-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT 
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations
under the License.

BSIM-CMG model is supported by the members of Silicon Integration
Initiative's Compact Model Coalition. A link to the most recent version of
this standard can be found at: http://www.si2.org/cmc 
*/

#include "b4defs.h"


#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

int
BSIM4dev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
    sBSIM4instance *here = static_cast<sBSIM4instance*>(geninst);
    IFvalue *value = &data->v;

    switch(param)
    {
    case BSIM4_W:
        here->BSIM4w = value->rValue;
        here->BSIM4wGiven = TRUE;
        break;
    case BSIM4_L:
        here->BSIM4l = value->rValue;
        here->BSIM4lGiven = TRUE;
        break;
    case BSIM4_NF:
        here->BSIM4nf = value->rValue;
        here->BSIM4nfGiven = TRUE;
        break;
    case BSIM4_MIN:
        here->BSIM4min = value->iValue;
        here->BSIM4minGiven = TRUE;
        break;
    case BSIM4_AS:
        here->BSIM4sourceArea = value->rValue;
        here->BSIM4sourceAreaGiven = TRUE;
        break;
    case BSIM4_AD:
        here->BSIM4drainArea = value->rValue;
        here->BSIM4drainAreaGiven = TRUE;
        break;
    case BSIM4_PS:
        here->BSIM4sourcePerimeter = value->rValue;
        here->BSIM4sourcePerimeterGiven = TRUE;
        break;
    case BSIM4_PD:
        here->BSIM4drainPerimeter = value->rValue;
        here->BSIM4drainPerimeterGiven = TRUE;
        break;
    case BSIM4_NRS:
        here->BSIM4sourceSquares = value->rValue;
        here->BSIM4sourceSquaresGiven = TRUE;
        break;
    case BSIM4_NRD:
        here->BSIM4drainSquares = value->rValue;
        here->BSIM4drainSquaresGiven = TRUE;
        break;
    case BSIM4_OFF:
        here->BSIM4off = value->iValue;
        break;
    case BSIM4_SA:
        here->BSIM4sa = value->rValue;
        here->BSIM4saGiven = TRUE;
        break;
    case BSIM4_SB:
        here->BSIM4sb = value->rValue;
        here->BSIM4sbGiven = TRUE;
        break;
    case BSIM4_SD:
        here->BSIM4sd = value->rValue;
        here->BSIM4sdGiven = TRUE;
        break;
    case BSIM4_SCA:
        here->BSIM4sca = value->rValue;
        here->BSIM4scaGiven = TRUE;
        break;
    case BSIM4_SCB:
        here->BSIM4scb = value->rValue;
        here->BSIM4scbGiven = TRUE;
        break;
    case BSIM4_SCC:
        here->BSIM4scc = value->rValue;
        here->BSIM4sccGiven = TRUE;
        break;
    case BSIM4_SC:
        here->BSIM4sc = value->rValue;
        here->BSIM4scGiven = TRUE;
        break;
    case BSIM4_RBSB:
        here->BSIM4rbsb = value->rValue;
        here->BSIM4rbsbGiven = TRUE;
        break;
    case BSIM4_RBDB:
        here->BSIM4rbdb = value->rValue;
        here->BSIM4rbdbGiven = TRUE;
        break;
    case BSIM4_RBPB:
        here->BSIM4rbpb = value->rValue;
        here->BSIM4rbpbGiven = TRUE;
        break;
    case BSIM4_RBPS:
        here->BSIM4rbps = value->rValue;
        here->BSIM4rbpsGiven = TRUE;
        break;
    case BSIM4_RBPD:
        here->BSIM4rbpd = value->rValue;
        here->BSIM4rbpdGiven = TRUE;
        break;
    case BSIM4_DELVTO:
        here->BSIM4delvto = value->rValue;
        here->BSIM4delvtoGiven = TRUE;
        break;
    case BSIM4_XGW:
        here->BSIM4xgw = value->rValue;
        here->BSIM4xgwGiven = TRUE;
        break;
    case BSIM4_NGCON:
        here->BSIM4ngcon = value->rValue;
        here->BSIM4ngconGiven = TRUE;
        break;
    case BSIM4_TRNQSMOD:
        here->BSIM4trnqsMod = value->iValue;
        here->BSIM4trnqsModGiven = TRUE;
        break;
    case BSIM4_ACNQSMOD:
        here->BSIM4acnqsMod = value->iValue;
        here->BSIM4acnqsModGiven = TRUE;
        break;
    case BSIM4_RBODYMOD:
        here->BSIM4rbodyMod = value->iValue;
        here->BSIM4rbodyModGiven = TRUE;
        break;
    case BSIM4_RGATEMOD:
        here->BSIM4rgateMod = value->iValue;
        here->BSIM4rgateModGiven = TRUE;
        break;
    case BSIM4_GEOMOD:
        here->BSIM4geoMod = value->iValue;
        here->BSIM4geoModGiven = TRUE;
        break;
    case BSIM4_RGEOMOD:
        here->BSIM4rgeoMod = value->iValue;
        here->BSIM4rgeoModGiven = TRUE;
        break;
    case BSIM4_IC_VDS:
        here->BSIM4icVDS = value->rValue;
        here->BSIM4icVDSGiven = TRUE;
        break;
    case BSIM4_IC_VGS:
        here->BSIM4icVGS = value->rValue;
        here->BSIM4icVGSGiven = TRUE;
        break;
    case BSIM4_IC_VBS:
        here->BSIM4icVBS = value->rValue;
        here->BSIM4icVBSGiven = TRUE;
        break;
    case BSIM4_IC:
        switch(value->v.numValue)
        {
        case 3:
            here->BSIM4icVBS = *(value->v.vec.rVec+2);
            here->BSIM4icVBSGiven = TRUE;
            // fallthrough
        case 2:
            here->BSIM4icVGS = *(value->v.vec.rVec+1);
            here->BSIM4icVGSGiven = TRUE;
            // fallthrough
        case 1:
            here->BSIM4icVDS = *(value->v.vec.rVec);
            here->BSIM4icVDSGiven = TRUE;
            data->cleanup();
            break;
        default:
            data->cleanup();
            return(E_BADPARM);
        }
        break;
// SRW
    case BSIM4_M:
        here->BSIM4m = value->rValue;
        here->BSIM4mGiven = TRUE;
        break;
    case BSIM4_WF:
        here->BSIM4wf = value->rValue;
        here->BSIM4wfGiven = TRUE;
        break;

    default:
        return(E_BADPARM);
    }
    return(OK);
}

