
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
 $Id: srcaski.cc,v 2.13 2015/11/22 01:20:35 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1987 Kanwar Jit Singh
         1992 Stephen R. Whiteley
****************************************************************************/

#include "srcdefs.h"


int
SRCdev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
#ifdef WITH_CMP_GOTO
    // Order MUST match parameter definition enum!
    static void *array[] = {
        0, // notused
        0, // &&L_SRC_I,
        0, // &&L_SRC_V,
        &&L_SRC_DEP,
        &&L_SRC_DC,
        &&L_SRC_AC,
        &&L_SRC_AC_MAG,
        &&L_SRC_AC_PHASE,
        &&L_SRC_AC_REAL,
        &&L_SRC_AC_IMAG,
        &&L_SRC_FUNC,
        &&L_SRC_D_F1,
        &&L_SRC_D_F2,
        &&L_SRC_GAIN,
        &&L_SRC_CONTROL,
        &&L_SRC_VOLTAGE,
        &&L_SRC_CURRENT,
        &&L_SRC_POWER,
        &&L_SRC_POS_NODE,
        &&L_SRC_NEG_NODE,
        &&L_SRC_CONT_P_NODE,
        &&L_SRC_CONT_N_NODE,
        &&L_SRC_BR_NODE};

    if ((unsigned int)which > SRC_BR_NODE)
        return (E_BADPARM);
#endif

    const sSRCinstance *inst = static_cast<const sSRCinstance*>(geninst);

    // Need to override this for non-real returns.
    data->type = IF_REAL;

#ifdef WITH_CMP_GOTO
    void *jmp = array[which];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_SRC_DEP:
        data->type = IF_INTEGER;
        data->v.iValue = inst->SRCdep;
        return (OK);
    L_SRC_DC:
        data->v.rValue = inst->SRCdcValue;
        if (inst->SRCtype == SRC_V)
            data->type |= IF_VOLT;
        else
            data->type |= IF_AMP;
        return (OK);
    L_SRC_AC:
        if (inst->SRCacTabName) {
            data->type = IF_STRING;
            data->v.sValue = inst->SRCacTabName;
        }
        else {
            data->type = IF_REALVEC;
            data->v.v.numValue = 2;
            data->v.v.vec.rVec = inst->SRCacVec;
            if (inst->SRCtype == SRC_V)
                data->type |= IF_VOLT;
            else
                data->type |= IF_AMP;
        }
        return (OK);
    L_SRC_AC_MAG:
        data->v.rValue = inst->SRCacMag;
        if (inst->SRCtype == SRC_V)
            data->type |= IF_VOLT;
        else
            data->type |= IF_AMP;
        return (OK);
    L_SRC_AC_PHASE:
        data->v.rValue = inst->SRCacPhase;
        return (OK);
    L_SRC_AC_REAL:
        data->v.rValue = inst->SRCacReal;
        if (inst->SRCtype == SRC_V)
            data->type |= IF_VOLT;
        else
            data->type |= IF_AMP;
        return (OK);
    L_SRC_AC_IMAG:
        data->v.rValue = inst->SRCacImag;
        if (inst->SRCtype == SRC_V)
            data->type |= IF_VOLT;
        else
            data->type |= IF_AMP;
        return (OK);
    L_SRC_FUNC:
        data->type = IF_PARSETREE;
        data->v.tValue = inst->SRCtree;
        if (inst->SRCtype == SRC_V)
            data->type |= IF_VOLT;
        else
            data->type |= IF_AMP;
        return (OK);
    L_SRC_D_F1:
        data->type = IF_REALVEC;
        if (inst->SRCdGiven && inst->SRCdF1given) {
            data->v.v.numValue = 2;
            data->v.v.vec.rVec = &inst->SRCdF1mag;
            // phase must follow!
        }
        return (OK);
    L_SRC_D_F2:
        data->type = IF_REALVEC;
        if (inst->SRCdGiven && inst->SRCdF2given) {
            data->v.v.numValue = 2;
            data->v.v.vec.rVec = &inst->SRCdF2mag;
            // phase must follow!
        }
        return (OK);
    L_SRC_GAIN:
        if (inst->SRCcoeff.imag != 0.0) {
            data->type = IF_COMPLEX;
            data->v.cValue = inst->SRCcoeff;
        }
        else {
            data->v.rValue = inst->SRCcoeff.real;
        }
        if (inst->SRCtype == SRC_V  && inst->SRCdep == SRC_CC)
            data->type |= IF_RES;
        else if (inst->SRCtype == SRC_I && inst->SRCdep == SRC_VC)
            data->type |= IF_COND;
        return (OK);
    L_SRC_CONTROL:
        data->type = IF_INSTANCE;
        data->v.uValue = inst->SRCcontName;
        return (OK);
    L_SRC_VOLTAGE:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->SRCposNode)
                - ckt->rhsOld(inst->SRCnegNode);
            data->v.cValue.imag = ckt->irhsOld(inst->SRCposNode)
                - ckt->irhsOld(inst->SRCnegNode);
        }
        else {
            data->v.rValue =
                ckt->rhsOld(inst->SRCposNode) - ckt->rhsOld(inst->SRCnegNode);
        }
        return (OK);
    L_SRC_CURRENT:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            if (inst->SRCtype == SRC_V) {
                data->v.cValue.real = ckt->rhsOld(inst->SRCbranch);
                data->v.cValue.imag = ckt->irhsOld(inst->SRCbranch);
            }
            else {
                data->v.cValue.real = inst->SRCacReal;
                data->v.cValue.imag = inst->SRCacImag;
            }
        }
        else {
            if (inst->SRCtype == SRC_V)
                data->v.rValue = ckt->rhsOld(inst->SRCbranch);
            else if (inst->SRCtree)
                // The past history of the current in a current source is
                // kept in the state vector, so that we can interpolate to
                // get accurate values in transient analysis.
                //
                data->v.rValue = ckt->interp(inst->GENstate +
                    inst->SRCtree->num_vars());
            else if (!inst->SRCvcCoeffGiven && !inst->SRCccCoeffGiven)
                data->v.rValue = inst->SRCvalue;
            else
                // Linear cccs or vccs, we don't know the current!
                data->v.rValue = 0.0;
            
        }
        return (OK);
    L_SRC_POWER:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = 0.0;
        else {
            if (inst->SRCtype == SRC_V)
                data->v.rValue = ckt->rhsOld(inst->SRCbranch);
            else if (inst->SRCtree)
                data->v.rValue = ckt->interp(inst->GENstate +
                    inst->SRCtree->num_vars());
            else if (!inst->SRCvcCoeffGiven && !inst->SRCccCoeffGiven)
                data->v.rValue = inst->SRCvalue;
            else
                // Linear cccs or vccs, we don't know the current!
                data->v.rValue = 0.0;

            data->v.rValue *= -(ckt->rhsOld(inst->SRCposNode)
                - ckt->rhsOld(inst->SRCnegNode));
        }
        return (OK);
    L_SRC_POS_NODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->SRCposNode;
        return (OK);
    L_SRC_NEG_NODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->SRCnegNode;
        return (OK);
    L_SRC_CONT_P_NODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->SRCcontPosNode;
        return (OK);
    L_SRC_CONT_N_NODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->SRCcontNegNode;
        return (OK);
    L_SRC_BR_NODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->SRCbranch;
        return (OK);
#else
    switch (which) {
    case SRC_DEP:
        data->type = IF_INTEGER;
        data->v.iValue = inst->SRCdep;
        break;
    case SRC_DC:
        data->v.rValue = inst->SRCdcValue;
        if (inst->SRCtype == SRC_V)
            data->type |= IF_VOLT;
        else
            data->type |= IF_AMP;
        break;
    case SRC_AC:
        if (inst->SRCacTabName) {
            data->type = IF_STRING;
            data->v.sValue = inst->SRCacTabName;
        }
        else {
            data->type = IF_REALVEC;
            data->v.v.numValue = 2;
            data->v.v.vec.rVec = inst->SRCacVec;
            if (inst->SRCtype == SRC_V)
                data->type |= IF_VOLT;
            else
                data->type |= IF_AMP;
        }
        break;
    case SRC_AC_MAG:
        data->v.rValue = inst->SRCacMag;
        if (inst->SRCtype == SRC_V)
            data->type |= IF_VOLT;
        else
            data->type |= IF_AMP;
        break;
    case SRC_AC_PHASE:
        data->v.rValue = inst->SRCacPhase;
        break;
    case SRC_AC_REAL:
        data->v.rValue = inst->SRCacReal;
        if (inst->SRCtype == SRC_V)
            data->type |= IF_VOLT;
        else
            data->type |= IF_AMP;
        break;
    case SRC_AC_IMAG:
        data->v.rValue = inst->SRCacImag;
        if (inst->SRCtype == SRC_V)
            data->type |= IF_VOLT;
        else
            data->type |= IF_AMP;
        break;
    case SRC_FUNC:
        data->type = IF_PARSETREE;
        data->v.tValue = inst->SRCtree;
        if (inst->SRCtype == SRC_V)
            data->type |= IF_VOLT;
        else
            data->type |= IF_AMP;
        break;
    case SRC_D_F1:
        data->type = IF_REALVEC;
        if (inst->SRCdGiven && inst->SRCdF1given) {
            data->v.v.numValue = 2;
            data->v.v.vec.rVec = &inst->SRCdF1mag;
            // phase must follow!
        }
        break;
    case SRC_D_F2:
        data->type = IF_REALVEC;
        if (inst->SRCdGiven && inst->SRCdF2given) {
            data->v.v.numValue = 2;
            data->v.v.vec.rVec = &inst->SRCdF2mag;
            // phase must follow!
        }
        break;
    case SRC_GAIN:
        if (inst->SRCcoeff.imag != 0.0) {
            data->type = IF_COMPLEX;
            data->v.cValue = inst->SRCcoeff;
        }
        else {
            data->v.rValue = inst->SRCcoeff.real;
        }
        if (inst->SRCtype == SRC_V  && inst->SRCdep == SRC_CC)
            data->type |= IF_RES;
        else if (inst->SRCtype == SRC_I && inst->SRCdep == SRC_VC)
            data->type |= IF_COND;
        break;
    case SRC_CONTROL:
        data->type = IF_INSTANCE;
        data->v.uValue = inst->SRCcontName;
        break;
    case SRC_VOLTAGE:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->SRCposNode)
                - ckt->rhsOld(inst->SRCnegNode);
            data->v.cValue.imag = ckt->irhsOld(inst->SRCposNode)
                - ckt->irhsOld(inst->SRCnegNode);
        }
        else {
            data->v.rValue =
                ckt->rhsOld(inst->SRCposNode) - ckt->rhsOld(inst->SRCnegNode);
        }
        break;
    case SRC_CURRENT:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            if (inst->SRCtype == SRC_V) {
                data->v.cValue.real = ckt->rhsOld(inst->SRCbranch);
                data->v.cValue.imag = ckt->irhsOld(inst->SRCbranch);
            }
            else {
                data->v.cValue.real = inst->SRCacReal;
                data->v.cValue.imag = inst->SRCacImag;
            }
        }
        else {
            if (inst->SRCtype == SRC_V)
                data->v.rValue = ckt->rhsOld(inst->SRCbranch);
            else if (inst->SRCtree)
                // The past history of the current in a current source is
                // kept in the state vector, so that we can interpolate to
                // get accurate values in transient analysis.
                //
                data->v.rValue = ckt->interp(inst->GENstate +
                    inst->SRCtree->num_vars());
            else if (!inst->SRCvcCoeffGiven && !inst->SRCccCoeffGiven)
                data->v.rValue = inst->SRCvalue;
            else
                // Linear cccs or vccs, we don't know the current!
                data->v.rValue = 0.0;
            
        }
        break;
    case SRC_POWER:
        if (ckt->CKTcurrentAnalysis & DOING_AC)
            data->v.rValue = 0.0;
        else {
            if (inst->SRCtype == SRC_V)
                data->v.rValue = ckt->rhsOld(inst->SRCbranch);
            else if (inst->SRCtree)
                data->v.rValue = ckt->interp(inst->GENstate +
                    inst->SRCtree->num_vars());
            else if (!inst->SRCvcCoeffGiven && !inst->SRCccCoeffGiven)
                data->v.rValue = inst->SRCvalue;
            else
                // Linear cccs or vccs, we don't know the current!
                data->v.rValue = 0.0;

            data->v.rValue *= -(ckt->rhsOld(inst->SRCposNode)
                - ckt->rhsOld(inst->SRCnegNode));
        }
        break;
    case SRC_POS_NODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->SRCposNode;
        break;
    case SRC_NEG_NODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->SRCnegNode;
        break;
    case SRC_CONT_P_NODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->SRCcontPosNode;
        break;
    case SRC_CONT_N_NODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->SRCcontNegNode;
        break;
    case SRC_BR_NODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->SRCbranch;
        break;
    default:
        return (E_BADPARM);
    }
#endif
    return (OK);
}

