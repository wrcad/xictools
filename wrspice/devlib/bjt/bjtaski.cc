
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Mathew Lew and Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "bjtdefs.h"


// This routine gives access to the internal device 
// parameters for BJTs
//
int
BJTdev::askInst(const sCKT *ckt, const sGENinstance *geninst, int which,
    IFdata *data)
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
        0, // &&L_BJT_IC,
        &&L_BJT_QUEST_VBE,
        &&L_BJT_QUEST_VBC,
        &&L_BJT_QUEST_VCS,
        &&L_BJT_QUEST_CC,
        &&L_BJT_QUEST_CB,
        &&L_BJT_QUEST_CE,
        &&L_BJT_QUEST_CS,
        &&L_BJT_QUEST_POWER,
        &&L_BJT_QUEST_FT,
        &&L_BJT_QUEST_GPI,
        &&L_BJT_QUEST_GMU,
        &&L_BJT_QUEST_GM,
        &&L_BJT_QUEST_GO,
        &&L_BJT_QUEST_GX,
        &&L_BJT_QUEST_GEQCB,
        &&L_BJT_QUEST_GCCS,
        &&L_BJT_QUEST_GEQBX,
        &&L_BJT_QUEST_QBE,
        &&L_BJT_QUEST_QBC,
        &&L_BJT_QUEST_QCS,
        &&L_BJT_QUEST_QBX,
        &&L_BJT_QUEST_CQBE,
        &&L_BJT_QUEST_CQBC,
        &&L_BJT_QUEST_CQCS,
        &&L_BJT_QUEST_CQBX,
        &&L_BJT_QUEST_CEXBC,
        &&L_BJT_QUEST_CPI,
        &&L_BJT_QUEST_CMU,
        &&L_BJT_QUEST_CBX,
        &&L_BJT_QUEST_CCS,
        &&L_BJT_QUEST_COLNODE,
        &&L_BJT_QUEST_BASENODE,
        &&L_BJT_QUEST_EMITNODE,
        &&L_BJT_QUEST_SUBSTNODE,
        &&L_BJT_QUEST_COLPRIMENODE,
        &&L_BJT_QUEST_BASEPRIMENODE,
        &&L_BJT_QUEST_EMITPRIMENODE};

    if ((unsigned int)which > BJT_QUEST_EMITPRIMENODE)
        return (E_BADPARM);
#endif

    const sBJTinstance *inst = static_cast<const sBJTinstance*>(geninst);
    double tmp, tmp1;

    // Need to override this for non-real returns.
    data->type = IF_REAL;

#ifdef WITH_CMP_GOTO
    void *jmp = array[which];
    if (!jmp)
        return (E_BADPARM);
    goto *jmp;
    L_BJT_AREA:
        data->v.rValue = inst->BJTarea;
        return (OK);
    L_BJT_OFF:
        data->type = IF_FLAG;
        data->v.iValue = inst->BJToff;
        return (OK);
    L_BJT_TEMP:
        data->v.rValue = inst->BJTtemp - CONSTCtoK;
        return (OK);
    L_BJT_IC_VBE:
        data->v.rValue = inst->BJTicVBE;
        return (OK);
    L_BJT_IC_VCE:
        data->v.rValue = inst->BJTicVCE;
        return (OK);
    L_BJT_QUEST_VBE:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->BJTbasePrimeNode) -
                ckt->rhsOld(inst->BJTemitPrimeNode);
            data->v.cValue.imag = ckt->irhsOld(inst->BJTbasePrimeNode) -
                ckt->irhsOld(inst->BJTemitPrimeNode);
        }
        else {
            sBJTmodel *model = static_cast<sBJTmodel*>(inst->GENmodPtr);
            data->v.rValue = model->BJTtype > 0 ?
                ckt->interp(inst->BJTvbe) : -ckt->interp(inst->BJTvbe);
        }
        return (OK);
    L_BJT_QUEST_VBC:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->BJTbasePrimeNode) -
                ckt->rhsOld(inst->BJTcolPrimeNode);
            data->v.cValue.imag = ckt->irhsOld(inst->BJTbasePrimeNode) -
                ckt->irhsOld(inst->BJTcolPrimeNode);
        }
        else {
            sBJTmodel *model = static_cast<sBJTmodel*>(inst->GENmodPtr);
            data->v.rValue = model->BJTtype > 0 ?
                ckt->interp(inst->BJTvbc) : -ckt->interp(inst->BJTvbc);
        }
        return (OK);
    L_BJT_QUEST_VCS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->BJTcolPrimeNode) -
                ckt->rhsOld(inst->BJTsubstNode);
            data->v.cValue.imag = ckt->irhsOld(inst->BJTcolPrimeNode) -
                ckt->irhsOld(inst->BJTsubstNode);
        }
        else {
            sBJTmodel *model = static_cast<sBJTmodel*>(inst->GENmodPtr);
            data->v.rValue = model->BJTtype > 0 ?
                ckt->interp(inst->BJTvcs) : -ckt->interp(inst->BJTvcs);
        }
        return (OK);
    L_BJT_QUEST_CC:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cc(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else
            data->v.rValue = ckt->interp(inst->BJTa_cc);
        return (OK);
    L_BJT_QUEST_CB:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cb(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else
            data->v.rValue = ckt->interp(inst->BJTa_cb);
        return (OK);
    L_BJT_QUEST_CE:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_ce(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else {
            data->v.rValue = -ckt->interp(inst->BJTa_cc);
            data->v.rValue -= ckt->interp(inst->BJTa_cb);
            data->v.rValue += ckt->interp(inst->BJTcqcs);
        }
        return (OK);
    L_BJT_QUEST_CS:  
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            double xccs = inst->BJTcapcs * ckt->CKTomega;
            data->v.cValue.real =
                -xccs*(ckt->irhsOld(inst->BJTsubstNode) -
                    ckt->irhsOld(inst->BJTcolPrimeNode));
            data->v.cValue.imag =
                xccs*(ckt->rhsOld(inst->BJTsubstNode) -
                    ckt->rhsOld(inst->BJTcolPrimeNode));
        }
        else
            data->v.rValue = -ckt->interp(inst->BJTcqcs);
        return (OK);
    L_BJT_QUEST_POWER:
        {
            double vbe = ckt->interp(inst->BJTvbe);
            double vce = -ckt->interp(inst->BJTvbc) - vbe;
            data->v.rValue = (ckt->interp(inst->BJTa_cc) -
                ckt->interp(inst->BJTcqcs))*vce +
                ckt->interp(inst->BJTa_cb)*vbe;
        }
        return (OK);
    L_BJT_QUEST_FT:
        {
            tmp = ckt->interp(inst->BJTcqbc);
            tmp1 = ckt->interp(inst->BJTcqbx);
            if (tmp1 > tmp)
                tmp = tmp1;
            tmp1 = ckt->interp(inst->BJTcqbe);
            if (tmp1 > tmp)
                tmp = tmp1;
            if (tmp != 0.0)
                data->v.rValue = ckt->interp(inst->BJTa_gm)/(2*M_PI*tmp);
        }
        return (OK);
    L_BJT_QUEST_GPI:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = inst->BJTgpi;
            data->v.cValue.imag = inst->BJTcapbe * ckt->CKTomega;
        }
        else
            data->v.rValue = ckt->interp(inst->BJTa_gpi);
        return (OK);
    L_BJT_QUEST_GMU:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = inst->BJTgmu;
            data->v.cValue.imag = inst->BJTcapbc * ckt->CKTomega;
        }
        else
            data->v.rValue = ckt->interp(inst->BJTa_gmu);
        return (OK);
    L_BJT_QUEST_GM:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            double gm, xgm;
            inst->ac_gm(ckt, &gm, &xgm);
            if (xgm != 0.0) {
                data->type = IF_COMPLEX;
                data->v.cValue.real = gm;
                data->v.cValue.imag = xgm;
            }
            else
                data->v.rValue = gm;
        }
        else
            data->v.rValue = ckt->interp(inst->BJTa_gm);
        return (OK);
    L_BJT_QUEST_GO:
        data->v.rValue = ckt->interp(inst->BJTa_go);
        return (OK);
    L_BJT_QUEST_GX:
        data->v.rValue = ckt->interp(inst->BJTa_gx);
        return (OK);
    L_BJT_QUEST_GEQCB:
        data->v.rValue = ckt->interp(inst->BJTa_geqcb);
        return (OK);
    L_BJT_QUEST_GCCS:
        data->v.rValue = ckt->interp(inst->BJTa_gccs);
        return (OK);
    L_BJT_QUEST_GEQBX:
        data->v.rValue = ckt->interp(inst->BJTa_geqbx);
        return (OK);
    L_BJT_QUEST_QBE:
        data->v.rValue = ckt->interp(inst->BJTqbe);
        return (OK);
    L_BJT_QUEST_QBC:
        data->v.rValue = ckt->interp(inst->BJTqbc);
        return (OK);
    L_BJT_QUEST_QCS:
        data->v.rValue = ckt->interp(inst->BJTqcs);
        return (OK);
    L_BJT_QUEST_QBX:
        data->v.rValue = ckt->interp(inst->BJTqbx);
        return (OK);
    L_BJT_QUEST_CQBE:
        data->v.rValue = ckt->interp(inst->BJTcqbe);
        return (OK);
    L_BJT_QUEST_CQBC:
        data->v.rValue = ckt->interp(inst->BJTcqbc);
        return (OK);
    L_BJT_QUEST_CQCS:
        data->v.rValue = ckt->interp(inst->BJTcqcs);
        return (OK);
    L_BJT_QUEST_CQBX:
        data->v.rValue = ckt->interp(inst->BJTcqbx);
        return (OK);
    L_BJT_QUEST_CEXBC:
        data->v.rValue = ckt->interp(inst->BJTcexbc);
        return (OK);
    L_BJT_QUEST_CPI:
        data->v.rValue = ckt->interp(inst->BJTa_capbe);
        return (OK);
    L_BJT_QUEST_CMU:
        data->v.rValue = ckt->interp(inst->BJTa_capbc);
        return (OK);
    L_BJT_QUEST_CBX:
        data->v.rValue = ckt->interp(inst->BJTa_capbx);
        return (OK);
    L_BJT_QUEST_CCS:
        data->v.rValue = ckt->interp(inst->BJTa_capcs);
        return (OK);
    L_BJT_QUEST_COLNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->BJTcolNode;
        return (OK);
    L_BJT_QUEST_BASENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->BJTbaseNode;
        return (OK);
    L_BJT_QUEST_EMITNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->BJTemitNode;
        return (OK);
    L_BJT_QUEST_SUBSTNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->BJTsubstNode;
        return (OK);
    L_BJT_QUEST_COLPRIMENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->BJTcolPrimeNode;
        return (OK);
    L_BJT_QUEST_BASEPRIMENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->BJTbasePrimeNode;
        return (OK);
    L_BJT_QUEST_EMITPRIMENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->BJTemitPrimeNode;
        return (OK);
#else
    switch (which) {
    case BJT_AREA:
        data->v.rValue = inst->BJTarea;
        break;
    case BJT_OFF:
        data->type = IF_FLAG;
        data->v.iValue = inst->BJToff;
        break;
    case BJT_TEMP:
        data->v.rValue = inst->BJTtemp - CONSTCtoK;
        break;
    case BJT_IC_VBE:
        data->v.rValue = inst->BJTicVBE;
        break;
    case BJT_IC_VCE:
        data->v.rValue = inst->BJTicVCE;
        break;
    case BJT_QUEST_VBE:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->BJTbasePrimeNode) -
                ckt->rhsOld(inst->BJTemitPrimeNode);
            data->v.cValue.imag = ckt->irhsOld(inst->BJTbasePrimeNode) -
                ckt->irhsOld(inst->BJTemitPrimeNode);
        }
        else {
            sBJTmodel *model = static_cast<sBJTmodel*>(inst->GENmodPtr);
            data->v.rValue = model->BJTtype > 0 ?
                ckt->interp(inst->BJTvbe) : -ckt->interp(inst->BJTvbe);
        }
        break;
    case BJT_QUEST_VBC:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->BJTbasePrimeNode) -
                ckt->rhsOld(inst->BJTcolPrimeNode);
            data->v.cValue.imag = ckt->irhsOld(inst->BJTbasePrimeNode) -
                ckt->irhsOld(inst->BJTcolPrimeNode);
        }
        else {
            sBJTmodel *model = static_cast<sBJTmodel*>(inst->GENmodPtr);
            data->v.rValue = model->BJTtype > 0 ?
                ckt->interp(inst->BJTvbc) : -ckt->interp(inst->BJTvbc);
        }
        break;
    case BJT_QUEST_VCS:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = ckt->rhsOld(inst->BJTcolPrimeNode) -
                ckt->rhsOld(inst->BJTsubstNode);
            data->v.cValue.imag = ckt->irhsOld(inst->BJTcolPrimeNode) -
                ckt->irhsOld(inst->BJTsubstNode);
        }
        else {
            sBJTmodel *model = static_cast<sBJTmodel*>(inst->GENmodPtr);
            data->v.rValue = model->BJTtype > 0 ?
                ckt->interp(inst->BJTvcs) : -ckt->interp(inst->BJTvcs);
        }
        break;
    case BJT_QUEST_CC:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cc(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else
            data->v.rValue = ckt->interp(inst->BJTa_cc);
        break;
    case BJT_QUEST_CB:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_cb(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else
            data->v.rValue = ckt->interp(inst->BJTa_cb);
        break;
    case BJT_QUEST_CE:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            inst->ac_ce(ckt, &data->v.cValue.real, &data->v.cValue.imag);
        }
        else {
            data->v.rValue = -ckt->interp(inst->BJTa_cc);
            data->v.rValue -= ckt->interp(inst->BJTa_cb);
            data->v.rValue += ckt->interp(inst->BJTcqcs);
        }
        break;
    case BJT_QUEST_CS:  
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            double xccs = inst->BJTcapcs * ckt->CKTomega;
            data->v.cValue.real =
                -xccs*(ckt->irhsOld(inst->BJTsubstNode) -
                    ckt->irhsOld(inst->BJTcolPrimeNode));
            data->v.cValue.imag =
                xccs*(ckt->rhsOld(inst->BJTsubstNode) -
                    ckt->rhsOld(inst->BJTcolPrimeNode));
        }
        else
            data->v.rValue = -ckt->interp(inst->BJTcqcs);
        break;
    case BJT_QUEST_POWER:
        {
            double vbe = ckt->interp(inst->BJTvbe);
            double vce = -ckt->interp(inst->BJTvbc) - vbe;
            data->v.rValue = (ckt->interp(inst->BJTa_cc) -
                ckt->interp(inst->BJTcqcs))*vce +
                ckt->interp(inst->BJTa_cb)*vbe;
        }
        break;
    case BJT_QUEST_FT:
        {
            tmp = ckt->interp(inst->BJTcqbc);
            tmp1 = ckt->interp(inst->BJTcqbx);
            if (tmp1 > tmp)
                tmp = tmp1;
            tmp1 = ckt->interp(inst->BJTcqbe);
            if (tmp1 > tmp)
                tmp = tmp1;
            if (tmp != 0.0)
                data->v.rValue = ckt->interp(inst->BJTa_gm)/(2*M_PI*tmp);
        }
        break;
    case BJT_QUEST_GPI:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = inst->BJTgpi;
            data->v.cValue.imag = inst->BJTcapbe * ckt->CKTomega;
        }
        else
            data->v.rValue = ckt->interp(inst->BJTa_gpi);
        break;
    case BJT_QUEST_GMU:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            data->type = IF_COMPLEX;
            data->v.cValue.real = inst->BJTgmu;
            data->v.cValue.imag = inst->BJTcapbc * ckt->CKTomega;
        }
        else
            data->v.rValue = ckt->interp(inst->BJTa_gmu);
        break;
    case BJT_QUEST_GM:
        if (ckt->CKTcurrentAnalysis & DOING_AC) {
            double gm, xgm;
            inst->ac_gm(ckt, &gm, &xgm);
            if (xgm != 0.0) {
                data->type = IF_COMPLEX;
                data->v.cValue.real = gm;
                data->v.cValue.imag = xgm;
            }
            else
                data->v.rValue = gm;
        }
        else
            data->v.rValue = ckt->interp(inst->BJTa_gm);
        break;
    case BJT_QUEST_GO:
        data->v.rValue = ckt->interp(inst->BJTa_go);
        break;
    case BJT_QUEST_GX:
        data->v.rValue = ckt->interp(inst->BJTa_gx);
        break;
    case BJT_QUEST_GEQCB:
        data->v.rValue = ckt->interp(inst->BJTa_geqcb);
        break;
    case BJT_QUEST_GCCS:
        data->v.rValue = ckt->interp(inst->BJTa_gccs);
        break;
    case BJT_QUEST_GEQBX:
        data->v.rValue = ckt->interp(inst->BJTa_geqbx);
        break;
    case BJT_QUEST_QBE:
        data->v.rValue = ckt->interp(inst->BJTqbe);
        break;
    case BJT_QUEST_QBC:
        data->v.rValue = ckt->interp(inst->BJTqbc);
        break;
    case BJT_QUEST_QCS:
        data->v.rValue = ckt->interp(inst->BJTqcs);
        break;
    case BJT_QUEST_QBX:
        data->v.rValue = ckt->interp(inst->BJTqbx);
        break;
    case BJT_QUEST_CQBE:
        data->v.rValue = ckt->interp(inst->BJTcqbe);
        break;
    case BJT_QUEST_CQBC:
        data->v.rValue = ckt->interp(inst->BJTcqbc);
        break;
    case BJT_QUEST_CQCS:
        data->v.rValue = ckt->interp(inst->BJTcqcs);
        break;
    case BJT_QUEST_CQBX:
        data->v.rValue = ckt->interp(inst->BJTcqbx);
        break;
    case BJT_QUEST_CEXBC:
        data->v.rValue = ckt->interp(inst->BJTcexbc);
        break;
    case BJT_QUEST_CPI:
        data->v.rValue = ckt->interp(inst->BJTa_capbe);
        break;
    case BJT_QUEST_CMU:
        data->v.rValue = ckt->interp(inst->BJTa_capbc);
        break;
    case BJT_QUEST_CBX:
        data->v.rValue = ckt->interp(inst->BJTa_capbx);
        break;
    case BJT_QUEST_CCS:
        data->v.rValue = ckt->interp(inst->BJTa_capcs);
        break;
    case BJT_QUEST_COLNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->BJTcolNode;
        break;
    case BJT_QUEST_BASENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->BJTbaseNode;
        break;
    case BJT_QUEST_EMITNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->BJTemitNode;
        break;
    case BJT_QUEST_SUBSTNODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->BJTsubstNode;
        break;
    case BJT_QUEST_COLPRIMENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->BJTcolPrimeNode;
        break;
    case BJT_QUEST_BASEPRIMENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->BJTbasePrimeNode;
        break;
    case BJT_QUEST_EMITPRIMENODE:
        data->type = IF_INTEGER;
        data->v.iValue = inst->BJTemitPrimeNode;
        break;
    default:
        return (E_BADPARM);
    }
#endif
    return (OK);
}


void
sBJTinstance::ac_gm(const sCKT *ckt, double *gm, double *xgm) const
{
    if (!ckt->CKTstates[0]) {
        *gm = 0;
        *xgm = 0;
        return;
    }
    *gm = *(ckt->CKTstate0 + BJTa_gm);
    double arg =
        static_cast<const sBJTmodel*>(GENmodPtr)->BJTexcessPhaseFactor;
    if (arg == 0.0)
        *xgm = 0.0;
    else {
        arg *= ckt->CKTomega;
        double go = BJTgo;
        double tgm = *gm + go;
        *xgm = -tgm*sin(arg);
        *gm = tgm*cos(arg) - go;
    }
}


void
sBJTinstance::ac_cc(const sCKT *ckt, double *ccr, double *cci) const
{
    if (BJTcolNode != BJTcolPrimeNode) {
        double gcpr =
            static_cast<const sBJTmodel*>(GENmodPtr)->BJTcollectorConduct *
            BJTarea;
        *ccr = gcpr*(ckt->rhsOld(BJTcolNode) - ckt->rhsOld(BJTcolPrimeNode));
        *cci = gcpr*(ckt->irhsOld(BJTcolNode) - ckt->irhsOld(BJTcolPrimeNode));
        return;
    }
    // have to do it the hard way:
    // Ac'c'*vc' + Ac'b'*vb' + Ac'e'*ve' + Ac's*vs + Ac'b*vb
    //  (gmu + go , xcmu + xccs + xcbx)*vc'
    //  + (-gmu + gm, -xcmu + xgm)*vb' +
    //  + (-gm - go, -xgm)*ve'
    //  +  (-xccs)*vs + (-xcbx)*vb

    double xcmu = BJTcapbc * ckt->CKTomega;
    double xcbx = BJTcapbx * ckt->CKTomega;
    double xccs = BJTcapcs * ckt->CKTomega;
    double gm, xgm;
    ac_gm(ckt, &gm, &xgm);

    cIFcomplex a1(BJTgmu + BJTgo, xcmu + xccs + xcbx);
    cIFcomplex a2(-BJTgmu + gm, -xcmu + xgm);
    cIFcomplex a3(-gm - BJTgo, -xgm);
    cIFcomplex a4(0, -xccs);
    cIFcomplex a5(0, -xcbx);

    *ccr = a1.real* ckt->rhsOld(BJTcolPrimeNode) -
        a1.imag* ckt->irhsOld(BJTcolPrimeNode) +
        a2.real* ckt->rhsOld(BJTbasePrimeNode) -
        a2.imag* ckt->irhsOld(BJTbasePrimeNode) +
        a3.real* ckt->rhsOld(BJTemitPrimeNode) -
        a3.imag* ckt->irhsOld(BJTemitPrimeNode) -
        a4.imag* ckt->irhsOld(BJTsubstNode) -
        a5.imag* ckt->irhsOld(BJTbaseNode);

    *cci = a1.real* ckt->irhsOld(BJTcolPrimeNode) +
        a1.imag* ckt->rhsOld(BJTcolPrimeNode) +
        a2.real* ckt->irhsOld(BJTbasePrimeNode) +
        a2.imag* ckt->rhsOld(BJTbasePrimeNode) +
        a3.real* ckt->irhsOld(BJTemitPrimeNode) +
        a3.imag* ckt->rhsOld(BJTemitPrimeNode) +
        a4.imag* ckt->rhsOld(BJTsubstNode) +
        a5.imag* ckt->rhsOld(BJTbaseNode);
}


void
sBJTinstance::ac_cb(const sCKT *ckt, double *cbr, double *cbi) const
{
    if (BJTbaseNode != BJTbasePrimeNode) {
        double gx = BJTgx;
        double xcbx = BJTcapbx * ckt->CKTomega;
        *cbr = gx*(ckt->rhsOld(BJTbaseNode) - ckt->rhsOld(BJTbasePrimeNode))
            - xcbx*(ckt->irhsOld(BJTbaseNode) - ckt->irhsOld(BJTcolPrimeNode));
        *cbi = gx*(ckt->irhsOld(BJTbaseNode) - ckt->irhsOld(BJTbasePrimeNode))
            + xcbx*(ckt->rhsOld(BJTbaseNode) - ckt->rhsOld(BJTcolPrimeNode));
        return;
    }
    // have to do it the hard way:
    //  Ab'b'*vb' + Ab'e'*ve' + Ab'c'*vc'
    //  (gx + gpi + gmu, xcpi + xcmu + xcmcb)*vb'
    //  + (-gpi, -xcpi)*ve'
    //  + (-gmu, -xcmu - xcmcb)*vc'

    double xcpi = BJTcapbe * ckt->CKTomega;
    double xcmu = BJTcapbc * ckt->CKTomega;
    double xcmcb= BJTgeqcb * ckt->CKTomega;

    cIFcomplex a1(BJTgx + BJTgpi + BJTgmu, xcpi + xcmu + xcmcb);
    cIFcomplex a2(-BJTgpi, -xcpi);
    cIFcomplex a3(-BJTgmu, -xcmu - xcmcb);

    *cbr = a1.real* ckt->rhsOld(BJTbasePrimeNode) -
        a1.imag* ckt->irhsOld(BJTbasePrimeNode) +
        a2.real* ckt->rhsOld(BJTemitPrimeNode) -
        a2.imag* ckt->irhsOld(BJTemitPrimeNode) +
        a3.real* ckt->rhsOld(BJTcolPrimeNode) -
        a3.imag* ckt->irhsOld(BJTcolPrimeNode);

    *cbi = a1.real* ckt->irhsOld(BJTbasePrimeNode) +
        a1.imag* ckt->rhsOld(BJTbasePrimeNode) +
        a2.real* ckt->irhsOld(BJTemitPrimeNode) +
        a2.imag* ckt->rhsOld(BJTemitPrimeNode) +
        a3.real* ckt->irhsOld(BJTcolPrimeNode) +
        a3.imag* ckt->rhsOld(BJTcolPrimeNode);
}


void
sBJTinstance::ac_ce(const sCKT *ckt, double *cer, double *cei) const
{
    if (BJTemitNode != BJTemitPrimeNode) {
        double gepr =
            static_cast<const sBJTmodel*>(GENmodPtr)->BJTemitterConduct *
            BJTarea;
        *cer = gepr*(ckt->rhsOld(BJTemitNode) - ckt->rhsOld(BJTemitPrimeNode));
        *cei = gepr*(ckt->irhsOld(BJTemitNode) -
            ckt->irhsOld(BJTemitPrimeNode));
        return;
    }
    // have to do it the hard way
    //  Ae'e'*ve' + Ae'b'*vb' + Ae'c'*vc'
    //  (gpi + gm + go, xcpi + xgm)*ve'
    //  + (-gpi - gm, -xcpi - xgm - xcmcb)*vb'
    //  + (-go, xcmcb)*vc'

    double xcpi = BJTcapbe * ckt->CKTomega;
    double xcmcb = BJTgeqcb * ckt->CKTomega;
    double gm, xgm;
    ac_gm(ckt, &gm, &xgm);

    cIFcomplex a1(BJTgpi + gm + BJTgo, xcpi + xgm);
    cIFcomplex a2(-BJTgpi - gm, -xcpi - xgm - xcmcb);
    cIFcomplex a3(-BJTgo, xcmcb);

    *cer = a1.real* ckt->rhsOld(BJTemitPrimeNode) -
        a1.imag* ckt->irhsOld(BJTemitPrimeNode) +
        a2.real* ckt->rhsOld(BJTbasePrimeNode) -
        a2.imag* ckt->irhsOld(BJTbasePrimeNode) +
        a3.real* ckt->rhsOld(BJTcolPrimeNode) -
        a3.imag* ckt->irhsOld(BJTcolPrimeNode);

    *cei = a1.real* ckt->irhsOld(BJTemitPrimeNode) +
        a1.imag* ckt->rhsOld(BJTemitPrimeNode) +
        a2.real* ckt->irhsOld(BJTbasePrimeNode) +
        a2.imag* ckt->rhsOld(BJTbasePrimeNode) +
        a3.real* ckt->irhsOld(BJTcolPrimeNode) +
        a3.imag* ckt->rhsOld(BJTcolPrimeNode);
}

