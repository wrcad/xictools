
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Device Library                                                         *
 *                                                                        *
 *========================================================================*
 $Id: jjseti.cc,v 1.6 2016/10/16 01:33:10 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Author: 1992 Stephen R. Whiteley
****************************************************************************/

#include "jjdefs.h"


int
JJdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
#ifdef WITH_CMP_GOTO
    // Order MUST match parameter definition enum!
    static void *array[] = {
        0, // notused
        &&L_JJ_AREA, 
        &&L_JJ_PI,
        &&L_JJ_OFF,
        &&L_JJ_IC,
        &&L_JJ_ICP,
        &&L_JJ_ICV,
        &&L_JJ_CON};

        // &&L_JJ_QUEST_V,
        // &&L_JJ_QUEST_CRT,
        // &&L_JJ_QUEST_IC,
        // &&L_JJ_QUEST_IJ,
        // &&L_JJ_QUEST_IG,
        // &&L_JJ_QUEST_I,
        // &&L_JJ_QUEST_CAP,
        // &&L_JJ_QUEST_G0,
        // &&L_JJ_QUEST_GN,
        // &&L_JJ_QUEST_GS,
        // &&L_JJ_QUEST_G1,
        // &&L_JJ_QUEST_G2,
        // &&L_JJ_QUEST_N1,
        // &&L_JJ_QUEST_N2,
        // &&L_JJ_QUEST_NP};

    if ((unsigned int)param > JJ_CON)
        return (E_BADPARM);
#endif

    sJJinstance *inst = static_cast<sJJinstance*>(geninst);
    IFvalue *value = &data->v;

#ifdef WITH_CMP_GOTO
    void *jmp = array[param];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_JJ_AREA:
        inst->JJarea = value->rValue;
        inst->JJareaGiven = true;
        return (OK);
    L_JJ_PI:
        inst->JJpi = (value->iValue != 0);
        inst->JJpiGiven = true;
        return (OK);
    L_JJ_OFF:
        inst->JJoffGiven = true;
        return (OK);
    L_JJ_IC:
        switch(value->v.numValue) {
        case 2:
            inst->JJinitPhase = *(value->v.vec.rVec+1);
            inst->JJinitPhaseGiven = true;
            // fallthrough
        case 1:
            inst->JJinitVoltage = *(value->v.vec.rVec);
            inst->JJinitVoltGiven = true;
            data->cleanup();
            break;
        default:
            data->cleanup();
            return (E_BADPARM);
        }
        return (OK);
    L_JJ_ICP:
        inst->JJinitPhase = value->rValue;
        inst->JJinitPhaseGiven = true;
        return (OK);
    L_JJ_ICV:
        inst->JJinitVoltage = value->rValue;
        inst->JJinitVoltGiven = true;
        return (OK);
    L_JJ_CON:
        inst->JJcontrol = value->uValue;
        inst->JJcontrolGiven = true;
        return (OK);
#else
    switch (param) {
    case JJ_AREA:
        inst->JJarea = value->rValue;
        inst->JJareaGiven = true;
        break;
    case JJ_PI:
        inst->JJpi = (value->iValue != 0);
        inst->JJpiGiven = true;
        break;
    case JJ_OFF:
        inst->JJoffGiven = true;
        break;
    case JJ_IC:
        switch(value->v.numValue) {
        case 2:
            inst->JJinitPhase = *(value->v.vec.rVec+1);
            inst->JJinitPhaseGiven = true;
        case 1:
            inst->JJinitVoltage = *(value->v.vec.rVec);
            inst->JJinitVoltGiven = true;
            data->cleanup();
            break;
        default:
            data->cleanup();
            return (E_BADPARM);
        }
        break;
    case JJ_ICP:
        inst->JJinitPhase = value->rValue;
        inst->JJinitPhaseGiven = true;
        break;
    case JJ_ICV:
        inst->JJinitVoltage = value->rValue;
        inst->JJinitVoltGiven = true;
        break;
    case JJ_CON:
        inst->JJcontrol = value->uValue;
        inst->JJcontrolGiven = true;
        break;
    default:
        return (E_BADPARM);
    }
#endif
    return (OK);
}
