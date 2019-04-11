
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


int
TJMdev::setInst(int param, IFdata *data, sGENinstance *geninst)
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
        &&L_TJM_NOISE};

        // &&L_TJM_QUEST_V,
        // &&L_TJM_QUEST_CRT,
        // &&L_TJM_QUEST_IC,
        // &&L_TJM_QUEST_IJ,
        // &&L_TJM_QUEST_IG,
        // &&L_TJM_QUEST_I,
        // &&L_TJM_QUEST_CAP,
        // &&L_TJM_QUEST_G0,
        // &&L_TJM_QUEST_GN,
        // &&L_TJM_QUEST_GS,
        // &&L_TJM_QUEST_G1,
        // &&L_TJM_QUEST_G2,
        // &&L_TJM_QUEST_N1,
        // &&L_TJM_QUEST_N2,
        // &&L_TJM_QUEST_NP};

    if ((unsigned int)param > TJM_NOISE)
        return (E_BADPARM);
#endif

    sTJMinstance *inst = static_cast<sTJMinstance*>(geninst);
    IFvalue *value = &data->v;

#ifdef WITH_CMP_GOTO
    void *jmp = array[param];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_TJM_AREA:
        inst->TJMarea = value->rValue;
        inst->TJMareaGiven = true;
        return (OK);
    L_TJM_ICS:
        inst->TJMics = value->rValue;
        inst->TJMicsGiven = true;
        return (OK);
#ifdef NEWLSER
    L_TJM_LSER:
        inst->TJMlser = value->rValue;
        inst->TJMlserGiven = true;
        return (OK);
#endif
#ifdef NEWLSH
    L_TJM_LSH:
        inst->TJMlsh = value->rValue;
        inst->TJMlshGiven = true;
        return (OK);
#endif
    L_TJM_OFF:
        inst->TJMoffGiven = true;
        return (OK);
    L_TJM_IC:
        switch(value->v.numValue) {
        case 2:
            inst->TJMinitPhase = *(value->v.vec.rVec+1);
            inst->TJMinitPhaseGiven = true;
            // fallthrough
        case 1:
            inst->TJMinitVoltage = *(value->v.vec.rVec);
            inst->TJMinitVoltGiven = true;
            data->cleanup();
            break;
        default:
            data->cleanup();
            return (E_BADPARM);
        }
        return (OK);
    L_TJM_ICP:
        inst->TJMinitPhase = value->rValue;
        inst->TJMinitPhaseGiven = true;
        return (OK);
    L_TJM_ICV:
        inst->TJMinitVoltage = value->rValue;
        inst->TJMinitVoltGiven = true;
        return (OK);
    L_TJM_CON:
        inst->TJMcontrol = value->uValue;
        inst->TJMcontrolGiven = true;
        return (OK);
    L_TJM_NOISE:
        inst->TJMnoise = value->rValue;
        inst->TJMnoiseGiven = true;
        return (OK);
#else
    switch (param) {
    case TJM_AREA:
        inst->TJMarea = value->rValue;
        inst->TJMareaGiven = true;
        break;
    case TJM_ICS:
        inst->TJMics = value->rValue;
        inst->TJMicsGiven = true;
        break;
#ifdef NEWLSER
    case TJM_LSER:
        inst->TJMlser = value->rValue;
        inst->TJMlserGiven = true;
        break;
#endif
#ifdef NEWLSH
    case TJM_LSH:
        inst->TJMlsh = value->rValue;
        inst->TJMlshGiven = true;
        break;
#endif
    case TJM_OFF:
        inst->TJMoffGiven = true;
        break;
    case TJM_IC:
        switch(value->v.numValue) {
        case 2:
            inst->TJMinitPhase = *(value->v.vec.rVec+1);
            inst->TJMinitPhaseGiven = true;
        case 1:
            inst->TJMinitVoltage = *(value->v.vec.rVec);
            inst->TJMinitVoltGiven = true;
            data->cleanup();
            break;
        default:
            data->cleanup();
            return (E_BADPARM);
        }
        break;
    case TJM_ICP:
        inst->TJMinitPhase = value->rValue;
        inst->TJMinitPhaseGiven = true;
        break;
    case TJM_ICV:
        inst->TJMinitVoltage = value->rValue;
        inst->TJMinitVoltGiven = true;
        break;
    case TJM_CON:
        inst->TJMcontrol = value->uValue;
        inst->TJMcontrolGiven = true;
        break;
    case TJM_NOISE:
        inst->TJMnoise = value->uValue;
        inst->TJMnoiseGiven = true;
        break;
    default:
        return (E_BADPARM);
    }
#endif
    return (OK);
}
