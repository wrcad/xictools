
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
 $Id: capseti.cc,v 1.9 2016/03/10 04:39:40 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "capdefs.h"


int
CAPdev::setInst(int param, IFdata *data, sGENinstance *geninst)
{
#ifdef WITH_CMP_GOTO
    // Order MUST match parameter definition enum!
    static void *array[] = {
        0, // notused
        &&L_CAP_CAP,
        &&L_CAP_IC,
        &&L_CAP_WIDTH,
        &&L_CAP_LENGTH,
        &&L_CAP_TEMP,
        &&L_CAP_TC1,
        &&L_CAP_TC2,
        &&L_CAP_POLY};
        // &&L_CAP_CHARGE,
        // &&L_CAP_VOLTAGE,
        // &&L_CAP_CURRENT,
        // &&L_CAP_POWER,
        // &&L_CAP_POSNODE,
        // &&L_CAP_NEGNODE,
        // &&L_CAP_TREE};

    if ((unsigned int)param > CAP_POLY)
        return (E_BADPARM);
#endif

    sCAPinstance *inst = static_cast<sCAPinstance*>(geninst);
    IFvalue *value = &data->v;

#ifdef WITH_CMP_GOTO
    void *jmp = array[param];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_CAP_CAP:
        if ((data->type & IF_VARTYPES) == IF_PARSETREE) {
            if (inst->CAPtree) {
                delete value->tValue;
                value->tValue = 0;
                return (E_BADPARM);
            }
            if (value->tValue->isConst()) {
                double d;
                int er = value->tValue->eval(&d, 0, 0, 0);
                if (er == OK) {
                    inst->CAPnomCapac = d;
                    inst->CAPcapGiven = true;
                }
                delete value->tValue;
                value->tValue = 0;
                if (er != OK)
                    return (E_BADPARM);
            }
            else {
                inst->CAPtree = value->tValue;
                value->tValue = 0;
                // No need to compute derivatives, cap doesn't
                // use them.
            }
        }
        else if ((data->type & IF_VARTYPES) == IF_REAL) {
            inst->CAPnomCapac = value->rValue;
            inst->CAPcapGiven = true;
        }
        else
            return (E_BADPARM);
        return (OK);
    L_CAP_IC:
        inst->CAPinitCond = value->rValue;
        inst->CAPicGiven = true;
        return (OK);
    L_CAP_WIDTH:
        inst->CAPwidth = value->rValue;
        inst->CAPwidthGiven = true;
        return (OK);
    L_CAP_LENGTH:
        inst->CAPlength = value->rValue;
        inst->CAPlengthGiven = true;
        return (OK);
    L_CAP_TEMP:
        inst->CAPtemp = value->rValue + CONSTCtoK;
        inst->CAPtempGiven = true;
        return (OK);
    L_CAP_TC1:
        inst->CAPtc1 = value->rValue;
        inst->CAPtc1Given = true;
        return (OK);
    L_CAP_TC2:
        inst->CAPtc2 = value->rValue;
        inst->CAPtc2Given = true;
        return (OK);
    L_CAP_POLY:
        {
            int nv = value->v.numValue;
            if (nv > 0) {
                inst->CAPpolyNumCoeffs = nv;
                double *coeffs = new double[nv];
                for (int i = 0; i < nv; i++)
                    coeffs[i] = value->v.vec.rVec[i];
                inst->CAPpolyCoeffs = coeffs;
            }
            data->cleanup();
        }
        return (OK);
#else
    switch (param) {
    case CAP_CAP:
        if ((data->type & IF_VARTYPES) == IF_PARSETREE) {
            if (inst->CAPtree) {
                delete value->tValue;
                value->tValue = 0;
                return (E_BADPARM);
            }
            if (value->tValue->isConst()) {
                double d;
                int er = value->tValue->eval(&d, 0, 0, 0);
                if (er == OK) {
                    inst->CAPnomCapac = d;
                    inst->CAPcapGiven = true;
                }
                delete value->tValue;
                value->tValue = 0;
                if (er != OK)
                    return (E_BADPARM);
            }
            else {
                inst->CAPtree = value->tValue;
                value->tValue = 0;
                // No need to compute derivatives, cap doesn't
                // use them.
            }
        }
        else if ((data->type & IF_VARTYPES) == IF_REAL) {
            inst->CAPnomCapac = value->rValue;
            inst->CAPcapGiven = true;
        }
        else
            return (E_BADPARM);
        break;
    case CAP_IC:
        inst->CAPinitCond = value->rValue;
        inst->CAPicGiven = true;
        break;
    case CAP_WIDTH:
        inst->CAPwidth = value->rValue;
        inst->CAPwidthGiven = true;
        break;
    case CAP_LENGTH:
        inst->CAPlength = value->rValue;
        inst->CAPlengthGiven = true;
        break;
    case CAP_TEMP:
        inst->CAPtemp = value->rValue + CONSTCtoK;
        inst->CAPtempGiven = true;
        break;
    case CAP_TC1:
        inst->CAPtc1 = value->rValue;
        inst->CAPtc1Given = true;
        break;
    case CAP_TC2:
        inst->CAPtc2 = value->rValue;
        inst->CAPtc2Given = true;
        break;
    case CAP_POLY:
        {
            int nv = value->v.numValue;
            if (nv > 0) {
                double *coeffs = new double[nv];
                for (int i = 0; i < nv; i++)
                    coeffs[i] = value->v.vec.rVec[i];
                inst->CAPpolyCoeffs = coeffs;
                inst->CAPpolyNumCoeffs = nv;
            }
            value->cleanup();
        }
        return (OK);
    default:
        return (E_BADPARM);
    }
#endif
    return (OK);
}

