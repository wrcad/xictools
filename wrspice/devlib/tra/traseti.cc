
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
TRAdev::setInst(int param, IFdata *data, sGENinstance *geninst)
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
        &&L_TRA_IC};
        // &&L_TRA_QUERY_V1,
        // &&L_TRA_QUERY_I1,
        // &&L_TRA_QUERY_V2,
        // &&L_TRA_QUERY_I2,
        // &&L_TRA_POS_NODE1,
        // &&L_TRA_NEG_NODE1,
        // &&L_TRA_POS_NODE2,
        // &&L_TRA_NEG_NODE2,
        // &&L_TRA_BR_EQ1,
        // &&L_TRA_BR_EQ2,
        // &&L_TRA_INPUT1,
        // &&L_TRA_INPUT2,
        // &&L_TRA_DELAY,
        // &&L_TRA_MAXSTEP};

    if ((unsigned int)param > TRA_IC)
        return (E_BADPARM);
#endif

    sTRAinstance *inst = static_cast<sTRAinstance*>(geninst);
    IFvalue *value = &data->v;

#ifdef WITH_CMP_GOTO
    void *jmp = array[param];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_TRA_LEVEL:
        inst->TRAlevel = value->iValue;
        inst->TRAlevelGiven = true;
        return (OK);
    L_TRA_LENGTH:
        inst->TRAlength = value->rValue;
        inst->TRAlengthGiven = true;
        return (OK);
    L_TRA_L:
        inst->TRAl = value->rValue;
        inst->TRAlGiven = true;
        return (OK);
    L_TRA_C:
        inst->TRAc = value->rValue;
        inst->TRAcGiven = true;
        return (OK);
    L_TRA_R:
        inst->TRAr = value->rValue;
        inst->TRArGiven = true;
        return (OK);
    L_TRA_G:
        inst->TRAg = value->rValue;
        inst->TRAgGiven = true;
        return (OK);
    L_TRA_Z0:
        inst->TRAz = value->rValue;
        inst->TRAzGiven = true;
        return (OK);
    L_TRA_TD:
        inst->TRAtd = value->rValue;
        inst->TRAtdGiven = true;
        return (OK);
    L_TRA_FREQ:
        inst->TRAf = value->rValue;
        inst->TRAfGiven = true;
        return (OK);
    L_TRA_NL:
        inst->TRAnl = value->rValue;
        inst->TRAnlGiven = true;
        return (OK);
    L_TRA_LININTERP:
        inst->TRAhowToInterp = TRA_LININTERP;
        inst->TRAinterpGiven = true;
        return (OK);
    L_TRA_QUADINTERP:
        inst->TRAhowToInterp = TRA_QUADINTERP;
        inst->TRAinterpGiven = true;
        return (OK);
    L_TRA_TRUNCDONTCUT:
        inst->TRAlteConType = TRA_TRUNCDONTCUT;
        inst->TRAcutGiven = true;
        return (OK);
    L_TRA_TRUNCCUTSL:
        inst->TRAlteConType = TRA_TRUNCCUTSL;
        inst->TRAcutGiven = true;
        return (OK);
    L_TRA_TRUNCCUTLTE:
        inst->TRAlteConType = TRA_TRUNCCUTLTE;
        inst->TRAcutGiven = true;
        return (OK);
    L_TRA_TRUNCCUTNR:
        inst->TRAlteConType = TRA_TRUNCCUTNR;
        inst->TRAcutGiven = true;
        return (OK);
    L_TRA_NOBREAKS:
        inst->TRAbreakType = TRA_NOBREAKS;
        inst->TRAbreakGiven = true;
        return (OK);
    L_TRA_ALLBREAKS:
        inst->TRAbreakType = TRA_ALLBREAKS;
        inst->TRAbreakGiven = true;
        return (OK);
    L_TRA_TESTBREAKS:
        inst->TRAbreakType = TRA_TESTBREAKS;
        inst->TRAbreakGiven = true;
        return (OK);
    L_TRA_SLOPETOL:
        inst->TRAslopetol = value->rValue;
        inst->TRAslopetolGiven = true;
        return (OK);
    L_TRA_COMPACTREL:
        inst->TRAstLineReltol = value->rValue;
        inst->TRAcompactrelGiven = true;
        return (OK);
    L_TRA_COMPACTABS:
        inst->TRAstLineAbstol = value->rValue;
        inst->TRAcompactabsGiven = true;
        return (OK);
    L_TRA_RELTOL:
        inst->TRAreltol = value->rValue;
        inst->TRAreltolGiven = true;
        return (OK);
    L_TRA_ABSTOL:
        inst->TRAabstol = value->rValue;
        inst->TRAabstolGiven = true;
        return (OK);
    L_TRA_V1:
        inst->TRAinitVolt1 = value->rValue;
        return (OK);
    L_TRA_I1:
        inst->TRAinitCur1 = value->rValue;
        return (OK);
    L_TRA_V2:
        inst->TRAinitVolt2 = value->rValue;
        return (OK);
    L_TRA_I2:
        inst->TRAinitCur2 = value->rValue;
        return (OK);
    L_TRA_IC:
        switch (value->v.numValue) {
        case 4:
            inst->TRAinitCur2 = *(value->v.vec.rVec+3);
            // fallthrough
        case 3:
            inst->TRAinitVolt2 =  *(value->v.vec.rVec+2);
            // fallthrough
        case 2:
            inst->TRAinitCur1 = *(value->v.vec.rVec+1);
            // fallthrough
        case 1:
            inst->TRAinitVolt1 = *(value->v.vec.rVec);
            data->cleanup();
            break;
        default:
            data->cleanup();
            return (E_BADPARM);
        }
        return (OK);
#else
    switch (param) {
    case TRA_LEVEL:
        inst->TRAlevel = value->iValue;
        inst->TRAlevelGiven = true;
        break;
    case TRA_LENGTH:
        inst->TRAlength = value->rValue;
        inst->TRAlengthGiven = true;
        break;
    case TRA_L:
        inst->TRAl = value->rValue;
        inst->TRAlGiven = true;
        break;
    case TRA_C:
        inst->TRAc = value->rValue;
        inst->TRAcGiven = true;
        break;
    case TRA_R:
        inst->TRAr = value->rValue;
        inst->TRArGiven = true;
        break;
    case TRA_G:
        inst->TRAg = value->rValue;
        inst->TRAgGiven = true;
        break;
    case TRA_Z0:
        inst->TRAz = value->rValue;
        inst->TRAzGiven = true;
        break;
    case TRA_TD:
        inst->TRAtd = value->rValue;
        inst->TRAtdGiven = true;
        break;
    case TRA_FREQ:
        inst->TRAf = value->rValue;
        inst->TRAfGiven = true;
        break;
    case TRA_NL:
        inst->TRAnl = value->rValue;
        inst->TRAnlGiven = true;
        break;
    case TRA_LININTERP:
        inst->TRAhowToInterp = TRA_LININTERP;
        inst->TRAinterpGiven = true;
        break;
    case TRA_QUADINTERP:
        inst->TRAhowToInterp = TRA_QUADINTERP;
        inst->TRAinterpGiven = true;
        break;
    case TRA_TRUNCDONTCUT:
        inst->TRAlteConType = TRA_TRUNCDONTCUT;
        inst->TRAcutGiven = true;
        break;
    case TRA_TRUNCCUTSL:
        inst->TRAlteConType = TRA_TRUNCCUTSL;
        inst->TRAcutGiven = true;
        break;
    case TRA_TRUNCCUTLTE:
        inst->TRAlteConType = TRA_TRUNCCUTLTE;
        inst->TRAcutGiven = true;
        break;
    case TRA_TRUNCCUTNR:
        inst->TRAlteConType = TRA_TRUNCCUTNR;
        inst->TRAcutGiven = true;
        break;
    case TRA_NOBREAKS:
        inst->TRAbreakType = TRA_NOBREAKS;
        inst->TRAbreakGiven = true;
        break;
    case TRA_ALLBREAKS:
        inst->TRAbreakType = TRA_ALLBREAKS;
        inst->TRAbreakGiven = true;
        break;
    case TRA_TESTBREAKS:
        inst->TRAbreakType = TRA_TESTBREAKS;
        inst->TRAbreakGiven = true;
        break;
    case TRA_SLOPETOL:
        inst->TRAslopetol = value->rValue;
        inst->TRAslopetolGiven = true;
        break;
    case TRA_COMPACTREL:
        inst->TRAstLineReltol = value->rValue;
        inst->TRAcompactrelGiven = true;
        break;
    case TRA_COMPACTABS:
        inst->TRAstLineAbstol = value->rValue;
        inst->TRAcompactabsGiven = true;
        break;
    case TRA_RELTOL:
        inst->TRAreltol = value->rValue;
        inst->TRAreltolGiven = true;
        break;
    case TRA_ABSTOL:
        inst->TRAabstol = value->rValue;
        inst->TRAabstolGiven = true;
        break;
    case TRA_V1:
        inst->TRAinitVolt1 = value->rValue;
        break;
    case TRA_I1:
        inst->TRAinitCur1 = value->rValue;
        break;
    case TRA_V2:
        inst->TRAinitVolt2 = value->rValue;
        break;
    case TRA_I2:
        inst->TRAinitCur2 = value->rValue;
        break;
    case TRA_IC:
        switch (value->v.numValue) {
        case 4:
            inst->TRAinitCur2 = *(value->v.vec.rVec+3);
        case 3:
            inst->TRAinitVolt2 =  *(value->v.vec.rVec+2);
        case 2:
            inst->TRAinitCur1 = *(value->v.vec.rVec+1);
        case 1:
            inst->TRAinitVolt1 = *(value->v.vec.rVec);
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
