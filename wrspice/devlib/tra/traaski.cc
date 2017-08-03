
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

//-------------------------------------------------------------------------
// This is a general transmission line model derived from:
//  1) the spice3 TRA (lossless) model
//  2) the spice3 LTRA (lossy, convolution) model
//  3) the kspice TXL (lossy, Pade approximation convolution) model
// Authors:
//  1985 Thomas L. Quarles
//  1990 Jaijeet S. Roychowdhury
//  1990 Shen Lin
//  1992 Charles Hough
//  2002 Stephen R. Whiteley
// Copyright Regents of the University of California.  All rights reserved.
//-------------------------------------------------------------------------

#include "tradefs.h"


int
TRAdev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
{
#ifdef WITH_CMP_GOTO
    // Order MUST match parameter definition enum!
    static void *array[] = {
        0, // notused
        &&L_TRA_LEVEL,
        &&L_TRA_LENGTH,
        &&L_TRA_L,
        &&L_TRA_C,
        &&L_TRA_R,
        &&L_TRA_G,
        &&L_TRA_Z0,
        &&L_TRA_TD,
        &&L_TRA_FREQ,
        &&L_TRA_NL,
        &&L_TRA_LININTERP,
        &&L_TRA_QUADINTERP,
        &&L_TRA_TRUNCDONTCUT,
        &&L_TRA_TRUNCCUTSL,
        &&L_TRA_TRUNCCUTLTE,
        &&L_TRA_TRUNCCUTNR,
        &&L_TRA_NOBREAKS,
        &&L_TRA_ALLBREAKS,
        &&L_TRA_TESTBREAKS,
        &&L_TRA_SLOPETOL,
        &&L_TRA_COMPACTREL,
        &&L_TRA_COMPACTABS,
        &&L_TRA_RELTOL,
        &&L_TRA_ABSTOL,
        &&L_TRA_V1,
        &&L_TRA_I1,
        &&L_TRA_V2,
        &&L_TRA_I2,
        0, // &&L_TRA_IC,
        &&L_TRA_QUERY_V1,
        &&L_TRA_QUERY_I1,
        &&L_TRA_QUERY_V2,
        &&L_TRA_QUERY_I2,
        &&L_TRA_POS_NODE1,
        &&L_TRA_NEG_NODE1,
        &&L_TRA_POS_NODE2,
        &&L_TRA_NEG_NODE2,
        &&L_TRA_BR_EQ1,
        &&L_TRA_BR_EQ2,
        0, // &&L_TRA_INPUT1,
        0, // &&L_TRA_INPUT2,
        0, // &&L_TRA_DELAY,
        &&L_TRA_MAXSTEP};

    if ((unsigned int)which > TRA_MAXSTEP)
        return (E_BADPARM);
#endif

    const sTRAinstance *inst = static_cast<const sTRAinstance*>(geninst);

    // Need to override this for non-real returns.
    data->type = IF_REAL;

#ifdef WITH_CMP_GOTO
    void *jmp = array[which];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_TRA_LEVEL:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TRAlevel;
        return (OK);
    L_TRA_LENGTH:
        data->v.rValue = inst->TRAlength;
        return (OK);
    L_TRA_L:
        data->v.rValue = inst->TRAl;
        return (OK);
    L_TRA_C:
        data->v.rValue = inst->TRAc;
        return (OK);
    L_TRA_R:
        data->v.rValue = inst->TRAr;
        return (OK);
    L_TRA_G:
        data->v.rValue = inst->TRAg;
        return (OK);
    L_TRA_Z0:
        data->v.rValue = inst->TRAz;
        return (OK);
    L_TRA_TD:
        data->v.rValue = inst->TRAtd;
        return (OK);
    L_TRA_FREQ:
        data->v.rValue = inst->TRAf;
        return (OK);
    L_TRA_NL:
        data->v.rValue = inst->TRAnl;
        return (OK);
    L_TRA_LININTERP:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAhowToInterp == TRA_LININTERP);
        return (OK);
    L_TRA_QUADINTERP:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAhowToInterp == TRA_QUADINTERP);
        return (OK);
    L_TRA_TRUNCDONTCUT:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAlteConType == TRA_TRUNCDONTCUT);
        return (OK);
    L_TRA_TRUNCCUTSL:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAlteConType == TRA_TRUNCCUTSL);
        return (OK);
    L_TRA_TRUNCCUTLTE:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAlteConType == TRA_TRUNCCUTLTE);
        return (OK);
    L_TRA_TRUNCCUTNR:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAlteConType == TRA_TRUNCCUTNR);
        return (OK);
    L_TRA_NOBREAKS:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAbreakType == TRA_NOBREAKS);
        return (OK);
    L_TRA_ALLBREAKS:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAbreakType == TRA_ALLBREAKS);
        return (OK);
    L_TRA_TESTBREAKS:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAbreakType == TRA_TESTBREAKS);
        return (OK);
    L_TRA_SLOPETOL:
        data->v.rValue = inst->TRAslopetol;
        return (OK);
    L_TRA_COMPACTREL:
        data->v.rValue = inst->TRAstLineReltol;
        return (OK);
    L_TRA_COMPACTABS:
        data->v.rValue = inst->TRAstLineAbstol;
        return (OK);
    L_TRA_RELTOL:
        data->v.rValue = inst->TRAreltol;
        return (OK);
    L_TRA_ABSTOL:
        data->v.rValue = inst->TRAabstol;
        return (OK);
    L_TRA_V1:
        data->v.rValue = inst->TRAinitVolt1;
        return (OK);
    L_TRA_I1:
        data->v.rValue = inst->TRAinitCur1;
        return (OK);
    L_TRA_V2:
        data->v.rValue = inst->TRAinitVolt2;
        return (OK);
    L_TRA_I2:
        data->v.rValue = inst->TRAinitCur2;
        return (OK);
    L_TRA_QUERY_V1:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->TRAposNode1) -
                ckt->rhsOld(inst->TRAnegNode1); 
            data->v.cValue.imag = ckt->irhsOld(inst->TRAposNode1) -
                ckt->irhsOld(inst->TRAnegNode1); 
        }
        else {
            data->v.rValue = ckt->rhsOld(inst->TRAposNode1) -
               ckt->rhsOld(inst->TRAnegNode1); 
        }
        return (OK);
    L_TRA_QUERY_I1:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->TRAbrEq1);
            data->v.cValue.imag = ckt->irhsOld(inst->TRAbrEq1);
        }
        else
            data->v.rValue = ckt->rhsOld(inst->TRAbrEq1);
        return (OK);
    L_TRA_QUERY_V2:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->TRAposNode2) -
                ckt->rhsOld(inst->TRAnegNode2); 
            data->v.cValue.imag = ckt->irhsOld(inst->TRAposNode2) -
                ckt->irhsOld(inst->TRAnegNode2); 
        }
        else {
            data->v.rValue = ckt->rhsOld(inst->TRAposNode2) -
                ckt->rhsOld(inst->TRAnegNode2); 
        }
        return (OK);
    L_TRA_QUERY_I2:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->TRAbrEq2);
            data->v.cValue.imag = ckt->irhsOld(inst->TRAbrEq2);
        }
        else
            data->v.rValue = ckt->rhsOld(inst->TRAbrEq2);
        return (OK);
    L_TRA_POS_NODE1:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TRAposNode1;
        return (OK);
    L_TRA_NEG_NODE1:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TRAnegNode1;
        return (OK);
    L_TRA_POS_NODE2:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TRAposNode2;
        return (OK);
    L_TRA_NEG_NODE2:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TRAnegNode2;
        return (OK);
    L_TRA_BR_EQ1:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TRAbrEq1;
        return (OK);
    L_TRA_BR_EQ2:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TRAbrEq2;
        return (OK);
/*XXX
    L_TRA_DELAY:
        data->type = IF_REALVEC;
        {
            int temp;
            data->v.v.numValue = temp = 5*(ckt->CKTtimeIndex + 1);
            data->v.v.vec.rVec = new double[temp];
            double *a = data->v.v.vec.rVec;
            for (int i = 0; i <= ckt->CKTtimeIndex; i++) {
                *a++ = ckt->CKTtimePoints[i];
                *a++ = inst->TRAvalues[i].v_i;
                *a++ = inst->TRAvalues[i].i_i;
                *a++ = inst->TRAvalues[i].v_o;
                *a++ = inst->TRAvalues[i].i_o;
            }
        }
        return (OK);
*/
    L_TRA_MAXSTEP:
        data->v.rValue = inst->TRAmaxSafeStep;
        return (OK);
#else
    switch (which) {
    case TRA_LEVEL:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TRAlevel;
        break;
    case TRA_LENGTH:
        data->v.rValue = inst->TRAlength;
        break;
    case TRA_L:
        data->v.rValue = inst->TRAl;
        break;
    case TRA_C:
        data->v.rValue = inst->TRAc;
        break;
    case TRA_R:
        data->v.rValue = inst->TRAr;
        break;
    case TRA_G:
        data->v.rValue = inst->TRAg;
        break;
    case TRA_Z0:
        data->v.rValue = inst->TRAz;
        break;
    case TRA_TD:
        data->v.rValue = inst->TRAtd;
        break;
    case TRA_FREQ:
        data->v.rValue = inst->TRAf;
        break;
    case TRA_NL:
        data->v.rValue = inst->TRAnl;
        break;
    case TRA_LININTERP:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAhowToInterp == TRA_LININTERP);
        break;
    case TRA_QUADINTERP:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAhowToInterp == TRA_QUADINTERP);
        break;
    case TRA_TRUNCDONTCUT:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAlteConType == TRA_TRUNCDONTCUT);
        break;
    case TRA_TRUNCCUTSL:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAlteConType == TRA_TRUNCCUTSL);
        break;
    case TRA_TRUNCCUTLTE:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAlteConType == TRA_TRUNCCUTLTE);
        break;
    case TRA_TRUNCCUTNR:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAlteConType == TRA_TRUNCCUTNR);
        break;
    case TRA_NOBREAKS:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAbreakType == TRA_NOBREAKS);
        break;
    case TRA_ALLBREAKS:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAbreakType == TRA_ALLBREAKS);
        break;
    case TRA_TESTBREAKS:
        data->type = IF_FLAG;
        data->v.iValue = (inst->TRAbreakType == TRA_TESTBREAKS);
        break;
    case TRA_SLOPETOL:
        data->v.rValue = inst->TRAslopetol;
        break;
    case TRA_COMPACTREL:
        data->v.rValue = inst->TRAstLineReltol;
        break;
    case TRA_COMPACTABS:
        data->v.rValue = inst->TRAstLineAbstol;
        break;
    case TRA_RELTOL:
        data->v.rValue = inst->TRAreltol;
        break;
    case TRA_ABSTOL:
        data->v.rValue = inst->TRAabstol;
        break;
    case TRA_V1:
        data->v.rValue = inst->TRAinitVolt1;
        break;
    case TRA_I1:
        data->v.rValue = inst->TRAinitCur1;
        break;
    case TRA_V2:
        data->v.rValue = inst->TRAinitVolt2;
        break;
    case TRA_I2:
        data->v.rValue = inst->TRAinitCur2;
        break;
    case TRA_QUERY_V1:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->TRAposNode1) -
                ckt->rhsOld(inst->TRAnegNode1); 
            data->v.cValue.imag = ckt->irhsOld(inst->TRAposNode1) -
                ckt->irhsOld(inst->TRAnegNode1); 
        }
        else {
            data->v.rValue = ckt->rhsOld(inst->TRAposNode1) -
               ckt->rhsOld(inst->TRAnegNode1); 
        }
        break;
    case TRA_QUERY_I1:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->TRAbrEq1);
            data->v.cValue.imag = ckt->irhsOld(inst->TRAbrEq1);
        }
        else
            data->v.rValue = ckt->rhsOld(inst->TRAbrEq1);
        break;
    case TRA_QUERY_V2:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->TRAposNode2) -
                ckt->rhsOld(inst->TRAnegNode2); 
            data->v.cValue.imag = ckt->irhsOld(inst->TRAposNode2) -
                ckt->irhsOld(inst->TRAnegNode2); 
        }
        else {
            data->v.rValue = ckt->rhsOld(inst->TRAposNode2) -
                ckt->rhsOld(inst->TRAnegNode2); 
        }
        break;
    case TRA_QUERY_I2:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->TRAbrEq2);
            data->v.cValue.imag = ckt->irhsOld(inst->TRAbrEq2);
        }
        else
            data->v.rValue = ckt->rhsOld(inst->TRAbrEq2);
        break;
    case TRA_POS_NODE1:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TRAposNode1;
        break;
    case TRA_NEG_NODE1:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TRAnegNode1;
        break;
    case TRA_POS_NODE2:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TRAposNode2;
        break;
    case TRA_NEG_NODE2:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TRAnegNode2;
        break;
    case TRA_BR_EQ1:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TRAbrEq1;
        break;
    case TRA_BR_EQ2:
        data->type = IF_INTEGER;
        data->v.iValue = inst->TRAbrEq2;
        break;
/*XXX
    case TRA_DELAY:
        data->type = IF_REALVEC;
        {
            int temp;
            data->v.v.numValue = temp = 5*(ckt->CKTtimeIndex + 1);
            data->v.v.vec.rVec = new double[temp];
            double *a = data->v.v.vec.rVec;
            for (int i = 0; i <= ckt->CKTtimeIndex; i++) {
                *a++ = ckt->CKTtimePoints[i];
                *a++ = inst->TRAvalues[i].v_i;
                *a++ = inst->TRAvalues[i].i_i;
                *a++ = inst->TRAvalues[i].v_o;
                *a++ = inst->TRAvalues[i].i_o;
            }
        }
        break;
*/
    case TRA_MAXSTEP:
        data->v.rValue = inst->TRAmaxSafeStep;
        break;

    default:
        return (E_BADPARM);
    }
#endif
    return (OK);
}

