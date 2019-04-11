
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

#include "tjmdefs.h"


namespace {
    //
    // Return interpolated Josephson current.
    //
    double jj_ji(const sCKT *ckt, const sTJMinstance *inst)
    {
        if (!ckt->CKTstates[0])
            return (0.0);
        double val = ckt->CKTtranDiffs[0]*
            *(ckt->CKTstates[0] + inst->TJMcrti)*
            sin(*(ckt->CKTstates[0] + inst->TJMphase));
        for (int i = 1; i <= ckt->CKTtranDegree; i++) {
            val += ckt->CKTtranDiffs[i]*
                *(ckt->CKTstates[i] + inst->TJMcrti)*
                sin(*(ckt->CKTstates[i] + inst->TJMphase));
        }
        return (val);
    }
}


int
TJMdev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
#ifdef WITH_CMP_GOTO
    // Order MUST match parameter definition enum!
    static void *array[] = {
        0, // notused
        &&L_TJM_AREA, 
        &&L_TJM_ICS, 
#ifdef NEWLSER
        &&L_TJM_LSER, 
#endif
#ifdef NEWLSH
        &&L_TJM_LSH, 
#endif
        &&L_TJM_OFF,
        &&L_TJM_IC,
        &&L_TJM_ICP,
        &&L_TJM_ICV,
        &&L_TJM_CON,
        &&L_TJM_NOISE,

        &&L_TJM_QUEST_V,
        &&L_TJM_QUEST_CRT,
        &&L_TJM_QUEST_IC,
        &&L_TJM_QUEST_IJ,
        &&L_TJM_QUEST_IG,
        &&L_TJM_QUEST_I,
        &&L_TJM_QUEST_CAP,
        &&L_TJM_QUEST_G0,
        &&L_TJM_QUEST_GN,
        &&L_TJM_QUEST_GS,
        &&L_TJM_QUEST_G1,
        &&L_TJM_QUEST_G2,
        &&L_TJM_QUEST_N1,
        &&L_TJM_QUEST_N2,
        &&L_TJM_QUEST_NP
#ifdef NEWLSER
        ,
        &&L_TJM_QUEST_NI,
        &&L_TJM_QUEST_NB
#endif
#ifdef NEWLSH
        ,
        &&L_TJM_QUEST_NSHI,
        &&L_TJM_QUEST_NSHB
#endif
        };

#ifdef NEWLSH
    if ((unsigned int)which > TJM_QUEST_NSHB)
#else
#ifdef NEWLSER
    if ((unsigned int)which > TJM_QUEST_NB)
#else
    if ((unsigned int)which > TJM_QUEST_NP)
#endif
#endif
        return (E_BADPARM);
#endif

    const sTJMinstance *inst = static_cast<const sTJMinstance*>(geninst);

    // Need to override this for non-real returns.
    data->type = IF_REAL;

#ifdef WITH_CMP_GOTO
    void *jmp = array[which];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_TJM_AREA:
        data->v.rValue = inst->TJMarea;
        return (OK);
    L_TJM_ICS:
        data->v.rValue = inst->TJMics;
        return (OK);
#ifdef NEWLSER
    L_TJM_LSER:
        data->v.rValue = inst->TJMlser;
        return (OK);
#endif
#ifdef NEWLSH
    L_TJM_LSH:
        data->v.rValue = inst->TJMlsh;
        return (OK);
#endif
    L_TJM_OFF:
        data->type = IF_FLAG;
        data->v.iValue = inst->TJMoffGiven;
        return (OK);
    L_TJM_IC:
        data->type = IF_REALVEC;
        data->v.v.vec.rVec = inst->TJMinitCnd;
        data->v.v.numValue = 2;
        return (OK);
    L_TJM_ICP:
        data->v.rValue = inst->TJMinitPhase;
        return (OK);
    L_TJM_ICV:
        data->v.rValue = inst->TJMinitVoltage;
        return (OK);
    L_TJM_CON:
        data->type = IF_INSTANCE;
        data->v.uValue = inst->TJMcontrol;
        return (OK);
    L_TJM_NOISE:
        data->v.rValue = inst->TJMnoise;
        return (OK);
    L_TJM_QUEST_V:
        data->v.rValue = (ckt->rhsOld(inst->TJMposNode) -
            ckt->rhsOld(inst->TJMnegNode));
        return (OK);
    L_TJM_QUEST_CRT:
        data->v.rValue = inst->TJMcriti;
        return (OK);
    L_TJM_QUEST_IC:
        data->v.rValue = ckt->interp(inst->TJMdvdt)*inst->TJMcap;
        return (OK);
    L_TJM_QUEST_IJ:
        data->v.rValue = jj_ji(ckt, inst);
        return (OK);
    L_TJM_QUEST_IG:
        data->v.rValue = ckt->interp(inst->TJMqpi);
        return (OK);
    L_TJM_QUEST_I:
        data->v.rValue = ckt->interp(inst->TJMdvdt)*inst->TJMcap +
            jj_ji(ckt, inst) + ckt->interp(inst->TJMqpi);
        return (OK);
    L_TJM_QUEST_CAP:
        data->v.rValue = inst->TJMcap;
        return (OK);
    L_TJM_QUEST_G0:
        data->v.rValue = inst->TJMg0;
        return (OK);
    L_TJM_QUEST_GN:
        data->v.rValue = inst->TJMgn;
        return (OK);
    L_TJM_QUEST_GS:
        data->v.rValue = inst->TJMgs;
        return (OK);
    L_TJM_QUEST_G1:
        data->v.rValue = inst->TJMg1;
        return (OK);
    L_TJM_QUEST_G2:
        data->v.rValue = inst->TJMg2;
        return (OK);
    L_TJM_QUEST_N1:
        data->type = IF_INTEGER;
#ifdef NEWLSER
        data->v.iValue = inst->TJMrealPosNode;
#else
        data->v.iValue = inst->TJMposNode;
#endif
        return (OK);
    L_TJM_QUEST_N2:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TJMnegNode;
        return (OK);
    L_TJM_QUEST_NP:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TJMphsNode;
        return (OK);
#ifdef NEWLSER
    L_TJM_QUEST_NI:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TJMposNode;
        return (OK);
    L_TJM_QUEST_NB:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TJMlserBr;
        return (OK);
#endif
#ifdef NEWLSH
    L_TJM_QUEST_NSHI:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TJMlshIntNode;
        return (OK);
    L_TJM_QUEST_NSHB:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TJMlshBr;
        return (OK);
#endif
#else
    switch (which) {
    case TJM_AREA:
        data->v.rValue = inst->TJMarea;
        break;
    case TJM_ICS:
        data->v.rValue = inst->TJMics;
        break;
#ifdef NEWLSER
    case TJM_LSER:
        data->v.rValue = inst->TJMlser;
        break;
#endif
#ifdef NEWLSH
    case TJM_LSH:
        data->v.rValue = inst->TJMlsh;
        break;
#endif
    case TJM_OFF:
        data->type = IF_FLAG;
        data->v.iValue = inst->TJMoffGiven;
        break;
    case TJM_IC:
        data->type = IF_REALVEC;
        data->v.v.vec.rVec = inst->TJMinitCnd;
        data->v.v.numValue = 2;
        break;
    case TJM_ICP:
        data->v.rValue = inst->TJMinitPhase;
        break;
    case TJM_ICV:
        data->v.rValue = inst->TJMinitVoltage;
        break;
    case TJM_CON:
        data->type = IF_INSTANCE;
        data->v.uValue = inst->TJMcontrol;
        break;
    case TJM_NOISE:
        data->v.uValue = inst->TJMnoise;
        break;
    case TJM_QUEST_V:
        data->v.rValue = (ckt->rhsOld(inst->TJMposNode) -
            ckt->rhsOld(inst->TJMnegNode));
        break;
    case TJM_QUEST_CRT:
        data->v.rValue = inst->TJMcriti;
        break;
    case TJM_QUEST_IC:
        data->v.rValue = ckt->interp(inst->TJMdvdt)*inst->TJMcap;
        break;
    case TJM_QUEST_IJ:
        data->v.rValue = jj_ji(ckt, inst);
        break;
    case TJM_QUEST_IG:
        data->v.rValue = ckt->interp(inst->TJMqpi);
        break;
    case TJM_QUEST_I:
        data->v.rValue = ckt->interp(inst->TJMdvdt)*inst->TJMcap +
            jj_ji(ckt, inst) + ckt->interp(inst->TJMqpi);
        break;
    case TJM_QUEST_CAP:
        data->v.rValue = inst->TJMcap;
        break;
    case TJM_QUEST_G0:
        data->v.rValue = inst->TJMg0;
        break;
    case TJM_QUEST_GN:
        data->v.rValue = inst->TJMgn;
        break;
    case TJM_QUEST_GS:
        data->v.rValue = inst->TJMgs;
        break;
    case TJM_QUEST_G1:
        data->v.rValue = inst->TJMg1;
        break;
    case TJM_QUEST_G2:
        data->v.rValue = inst->TJMg2;
        break;
    case TJM_QUEST_N1:
        data->type = IF_INTEGER;
#ifdef NEWLSER
        data->v.iValue = inst->TJMrealPosNode;
#else
        data->v.iValue = inst->TJMposNode;
#endif
        break;
    case TJM_QUEST_N2:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TJMnegNode;
        break;
    case TJM_QUEST_NP:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TJMphsNode;
        break;
#ifdef NEWLSER
    case TJM_QUEST_NI:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TJMposNode;
        return (OK);
    case TJM_QUEST_NB:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TJMlserBr;
        return (OK);
#endif
#ifdef NEWLSER
    case TJM_QUEST_NSHI:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TJMlshIntNode;
        return (OK);
    case TJM_QUEST_NSHB:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TJMlshBr;
        return (OK);
#endif
    default:
        return (E_BADPARM);
    }
#endif
    return (OK);
}

