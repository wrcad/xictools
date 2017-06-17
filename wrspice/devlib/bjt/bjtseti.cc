
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
 $Id: bjtseti.cc,v 1.4 2015/11/22 01:20:20 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "bjtdefs.h"


// This routine sets instance parameters for
// BJTs in the circuit.
//
int
BJTdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
#ifdef WITH_CMP_GOTO
    // Order MUST match parameter definition enum!
    static void *array[] = {
        0, // notused
        &&L_BJT_AREA,
        &&L_BJT_OFF,
        &&L_BJT_TEMP,
        &&L_BJT_IC_VBE,
        &&L_BJT_IC_VCE,
        &&L_BJT_IC};
        // &&L_BJT_QUEST_VBE,
        // &&L_BJT_QUEST_VBC,
        // &&L_BJT_QUEST_VCS,
        // &&L_BJT_QUEST_CC,
        // &&L_BJT_QUEST_CB,
        // &&L_BJT_QUEST_CE,
        // &&L_BJT_QUEST_CS,
        // &&L_BJT_QUEST_POWER,
        // &&L_BJT_QUEST_FT,
        // &&L_BJT_QUEST_GPI,
        // &&L_BJT_QUEST_GMU,
        // &&L_BJT_QUEST_GM,
        // &&L_BJT_QUEST_GO,
        // &&L_BJT_QUEST_GX,
        // &&L_BJT_QUEST_GEQCB,
        // &&L_BJT_QUEST_GCCS,
        // &&L_BJT_QUEST_GEQBX,
        // &&L_BJT_QUEST_QBE,
        // &&L_BJT_QUEST_QBC,
        // &&L_BJT_QUEST_QCS,
        // &&L_BJT_QUEST_QBX,
        // &&L_BJT_QUEST_CQBE,
        // &&L_BJT_QUEST_CQBC,
        // &&L_BJT_QUEST_CQCS,
        // &&L_BJT_QUEST_CQBX,
        // &&L_BJT_QUEST_CEXBC,
        // &&L_BJT_QUEST_CPI,
        // &&L_BJT_QUEST_CMU,
        // &&L_BJT_QUEST_CBX,
        // &&L_BJT_QUEST_CCS,
        // &&L_BJT_QUEST_COLNODE,
        // &&L_BJT_QUEST_BASENODE,
        // &&L_BJT_QUEST_EMITNODE,
        // &&L_BJT_QUEST_SUBSTNODE,
        // &&L_BJT_QUEST_COLPRIMENODE,
        // &&L_BJT_QUEST_BASEPRIMENODE,
        // &&L_BJT_QUEST_EMITPRIMENODE};

    if ((unsigned int)param > BJT_IC)
        return (E_BADPARM);
#endif

    sBJTinstance *inst = static_cast<sBJTinstance*>(geninst);
    IFvalue *value = &data->v;

#ifdef WITH_CMP_GOTO
    void *jmp = array[param];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_BJT_AREA:
        inst->BJTarea = value->rValue;
        inst->BJTareaGiven = true;
        return (OK);
    L_BJT_OFF:
        inst->BJToff = value->iValue;
        return (OK);
    L_BJT_TEMP:
        inst->BJTtemp = value->rValue+CONSTCtoK;
        inst->BJTtempGiven = true;
        return (OK);
    L_BJT_IC_VBE:
        inst->BJTicVBE = value->rValue;
        inst->BJTicVBEGiven = true;
        return (OK);
    L_BJT_IC_VCE:
        inst->BJTicVCE = value->rValue;
        inst->BJTicVCEGiven = true;
        return (OK);
    L_BJT_IC :
        switch (value->v.numValue) {
        case 2:
            inst->BJTicVCE = *(value->v.vec.rVec+1);
            inst->BJTicVCEGiven = true;
        case 1:
            inst->BJTicVBE = *(value->v.vec.rVec);
            inst->BJTicVBEGiven = true;
            data->cleanup();
            break;
        default:
            data->cleanup();
            return (E_BADPARM);
        }
        return (OK);
#else
    switch (param) {
    case BJT_AREA:
        inst->BJTarea = value->rValue;
        inst->BJTareaGiven = true;
        break;
    case BJT_OFF:
        inst->BJToff = value->iValue;
        break;
    case BJT_TEMP:
        inst->BJTtemp = value->rValue+CONSTCtoK;
        inst->BJTtempGiven = true;
        break;
    case BJT_IC_VBE:
        inst->BJTicVBE = value->rValue;
        inst->BJTicVBEGiven = true;
        break;
    case BJT_IC_VCE:
        inst->BJTicVCE = value->rValue;
        inst->BJTicVCEGiven = true;
        break;
    case BJT_IC :
        switch (value->v.numValue) {
        case 2:
            inst->BJTicVCE = *(value->v.vec.rVec+1);
            inst->BJTicVCEGiven = true;
        case 1:
            inst->BJTicVBE = *(value->v.vec.rVec);
            inst->BJTicVBEGiven = true;
            data->cleanup();
            break;
        default:
            data->cleanup();
            return (E_BADPARM);
        }
        break;
    default:
        return (E_BADPARM);
    }
#endif
    return (OK);
}
