
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
#include <stdio.h>


namespace {
    int get_node_ptr(sCKT *ckt, sTRAinstance *inst)
    {
        TSTALLOC(TRAibr1Pos1Ptr, TRAbrEq1, TRAposNode1)
        TSTALLOC(TRAibr1Neg1Ptr, TRAbrEq1, TRAnegNode1)
        TSTALLOC(TRAibr1Pos2Ptr, TRAbrEq1, TRAposNode2)
        TSTALLOC(TRAibr1Neg2Ptr, TRAbrEq1, TRAnegNode2)
        TSTALLOC(TRAibr1Ibr1Ptr, TRAbrEq1, TRAbrEq1)
        TSTALLOC(TRAibr1Ibr2Ptr, TRAbrEq1, TRAbrEq2)
        TSTALLOC(TRAibr2Pos1Ptr, TRAbrEq2, TRAposNode1)
        TSTALLOC(TRAibr2Pos2Ptr, TRAbrEq2, TRAposNode2)
        TSTALLOC(TRAibr2Neg1Ptr, TRAbrEq2, TRAnegNode1)
        TSTALLOC(TRAibr2Neg2Ptr, TRAbrEq2, TRAnegNode2)
        TSTALLOC(TRAibr2Ibr1Ptr, TRAbrEq2, TRAbrEq1)
        TSTALLOC(TRAibr2Ibr2Ptr, TRAbrEq2, TRAbrEq2)
        TSTALLOC(TRApos1Ibr1Ptr, TRAposNode1, TRAbrEq1)
        TSTALLOC(TRAneg1Ibr1Ptr, TRAnegNode1, TRAbrEq1)
        TSTALLOC(TRApos2Ibr2Ptr, TRAposNode2, TRAbrEq2)
        TSTALLOC(TRAneg2Ibr2Ptr, TRAnegNode2, TRAbrEq2)

        // the following are done so that SMPpreOrder does not
        // screw up on occasion - for example, when one end
        // of the lossy line is hanging
        //
        TSTALLOC(TRApos1Pos1Ptr, TRAposNode1, TRAposNode1)
        TSTALLOC(TRAneg1Neg1Ptr, TRAnegNode1, TRAnegNode1)
        TSTALLOC(TRApos2Pos2Ptr, TRAposNode2, TRAposNode2)
        TSTALLOC(TRAneg2Neg2Ptr, TRAnegNode2, TRAnegNode2)
        return (OK);
    }
}


int
TRAdev::setup(sGENmodel *genmod, sCKT *ckt, int *state)
{
    (void)state;
    sTRAmodel *model = static_cast<sTRAmodel*>(genmod);
    for ( ; model; model = model->next()) {

        if (!model->TRAlevelGiven)
            model->TRAlevel = PADE_LEVEL;

        sTRAinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (!inst->TRAlevelGiven)
                inst->TRAlevel = model->TRAlevel;
            if (!inst->TRAlengthGiven)
                inst->TRAlength = model->TRAlength;
            if (!inst->TRAlGiven)
                inst->TRAl = model->TRAl;
            if (!inst->TRAcGiven)
                inst->TRAc = model->TRAc;
            if (!inst->TRArGiven)
                inst->TRAr = model->TRAr;
            if (!inst->TRAgGiven)
                inst->TRAg = model->TRAg;
            if (!inst->TRAzGiven)
                inst->TRAz = model->TRAz;
            if (!inst->TRAtdGiven)
                inst->TRAtd = model->TRAtd;
            if (!inst->TRAfGiven)
                inst->TRAf = model->TRAf;
            if (!inst->TRAnlGiven)
                inst->TRAnl = model->TRAnl;
            if (!inst->TRAinterpGiven)
                inst->TRAhowToInterp = model->TRAhowToInterp;
            if (!inst->TRAcutGiven)
                inst->TRAlteConType = model->TRAlteConType;
            if (!inst->TRAbreakGiven)
                inst->TRAbreakType = model->TRAbreakType;
            if (!inst->TRAslopetolGiven)
                inst->TRAslopetol = model->TRAslopetol;
            if (!inst->TRAcompactrelGiven)
                inst->TRAstLineReltol = model->TRAstLineReltol;
            if (!inst->TRAcompactabsGiven)
                inst->TRAstLineAbstol = model->TRAstLineAbstol;
            if (!inst->TRAreltolGiven)
                inst->TRAreltol = model->TRAreltol;
            if (!inst->TRAabstolGiven)
                inst->TRAabstol = model->TRAabstol;

            const char *s = inst->tranline_params();
            if (s) {
                DVO.textOut(OUT_FATAL, s, inst->GENname);
                return (E_PARMVAL);
            }
            if (inst->TRAlength == 0.0) {
                DVO.textOut(OUT_FATAL, "transmission line length is zero!");
                return (false);
            }

            sCKTnode *tmp;
            if (inst->TRAbrEq1 == 0) {
                int error = ckt->mkCur(&tmp, inst->GENname, "i1");
                if (error)
                    return(error);
                inst->TRAbrEq1 = tmp->number();
            }

            if (inst->TRAbrEq2 == 0) {
                int error = ckt->mkCur(&tmp, inst->GENname, "i2");
                if (error)
                    return(error);
                inst->TRAbrEq2 = tmp->number();
            }

            if ((inst->TRAhowToInterp != TRA_LININTERP) &&
                    (inst->TRAhowToInterp != TRA_QUADINTERP))
                inst->TRAhowToInterp = TRA_QUADINTERP;

            // Defaults:
            // Lossless case, any level:  truncsl
            // Level=1 (Pade): truncdontcut
            // Level=2 (convolution): trunclte
            //
            if ((inst->TRAlteConType != TRA_TRUNCDONTCUT) &&
                    (inst->TRAlteConType != TRA_TRUNCCUTSL) &&
                    (inst->TRAlteConType != TRA_TRUNCCUTLTE) &&
                    (inst->TRAlteConType != TRA_TRUNCCUTNR)) {
                if (inst->TRAcase == TRA_LC)
                    inst->TRAlteConType = TRA_TRUNCCUTSL;
                else if (inst->TRAlevel == CONV_LEVEL)
                    inst->TRAlteConType = TRA_TRUNCCUTLTE;
                else 
                    inst->TRAlteConType = TRA_TRUNCDONTCUT;
            }
            if (inst->TRAcase == TRA_LC &&
                    (inst->TRAlteConType == TRA_TRUNCCUTLTE ||
                    inst->TRAlteConType == TRA_TRUNCCUTNR)) {
                DVO.textOut(OUT_WARNING,
                    "%s: LC line, slope truncation being used",
                    inst->GENname);
                inst->TRAlteConType = TRA_TRUNCCUTSL;
            }
            if (inst->TRAlevel == PADE_LEVEL &&
                    (inst->TRAlteConType == TRA_TRUNCCUTLTE ||
                    inst->TRAlteConType == TRA_TRUNCCUTNR)) {
                DVO.textOut(OUT_WARNING,
                    "%s: Level 2 keyord %s ignored, truncdontcut used",
                    inst->GENname, inst->TRAlteConType == TRA_TRUNCCUTNR ?
                    "truncnr" : "trunclte");
                inst->TRAlteConType = TRA_TRUNCDONTCUT;
            }

            if (inst->TRAreltol <= 0.0)
                inst->TRAreltol = ckt->CKTcurTask->TSKreltol;
            else if (inst->TRAreltol > 0.1) {
                DVO.textOut(OUT_WARNING,
                    "%s: rel parameter is too large (Spice3 value?)",
                    inst->GENname);
            }
            if (inst->TRAabstol <= 0.0)
                inst->TRAabstol = ckt->CKTcurTask->TSKabstol;
            if (inst->TRAslopetol <= 0.0)
                inst->TRAslopetol = 0.1;
            if (inst->TRAstLineReltol == 0.0)
                inst->TRAstLineReltol = ckt->CKTcurTask->TSKreltol;
            if (inst->TRAstLineAbstol == 0.0)
                inst->TRAstLineAbstol = ckt->CKTcurTask->TSKabstol;
            if ((inst->TRAbreakType != TRA_NOBREAKS) &&
                    (inst->TRAbreakType != TRA_ALLBREAKS) &&
                    (inst->TRAbreakType != TRA_TESTBREAKS))
                inst->TRAbreakType = TRA_TESTBREAKS;

#ifdef NEWJJDC
            if (ckt->CKTjjDCphase && inst->TRAr == 0.0) {
                // The nodes that connect to inductors have the
                // "phase" flag set, for use in phase-mode DC analysis
                // with Josephson junctions.

                if (inst->TRAposNode1 > 0) {
                    sCKTnode *node = ckt->CKTnodeTab.find(inst->TRAposNode1);
                    if (node)
                        node->set_phase(true);
                }
                if (inst->TRAposNode2 > 0) {
                    sCKTnode *node = ckt->CKTnodeTab.find(inst->TRAposNode2);
                    if (node)
                        node->set_phase(true);
                }
                if (inst->TRAnegNode1 > 0) {
                    sCKTnode *node = ckt->CKTnodeTab.find(inst->TRAnegNode1);
                    if (node)
                        node->set_phase(true);
                }
                if (inst->TRAnegNode2 > 0) {
                    sCKTnode *node = ckt->CKTnodeTab.find(inst->TRAnegNode2);
                    if (node)
                        node->set_phase(true);
                }
            }
#endif

            int error = get_node_ptr(ckt, inst);
            if (error != OK)
                return (error);

            if (inst->TRAlevel == PADE_LEVEL) {
                error = inst->pade_setup(ckt);
                if (error)
                    return (error);
            }
            else if (inst->TRAlevel == CONV_LEVEL) {
                error = inst->ltra_setup(ckt);
                if (error)
                    return (error);
            }
            else
                return (E_PARMVAL);
        }
    }
    return (OK);
}


int
TRAdev::unsetup(sGENmodel *genmod, sCKT*)
{
    sTRAmodel *model = static_cast<sTRAmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sTRAinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            inst->TRAbrEq1 = 0;
            inst->TRAbrEq2 = 0;
        }
    }
    return (OK);
}


// SRW - reset the matrix element pointers.
//
int
TRAdev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sTRAmodel *model = (sTRAmodel*)inModel; model;
            model = model->next()) {
        for (sTRAinstance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}


// Find the missing parameters from those given (nonzero).
//  l    inductance/length
//  c    capacitance/length
//  td   total line delay time
//  len  length (arb units)
//  z    impedance
//  f    assumed frequency
//  nl   normalized length at frequency
//
// 0 is returned on success, an error string is returned if not
// enough or conflicting information is supplied.
//
const char *
sTRAinstance::tranline_params()
{
    if (TRAf != 0.0) {
        // Specify either f,nl or td
        if (TRAnl == 0.0)
            return ("%s: F given without Ln");
        if (TRAtd != 0.0)
            return ("%s: F, Ln, and Td given");
        TRAtd = TRAnl/TRAf;
    }
    else if (TRAnl != 0.0)
        return ("%s: Ln given without F");
    if (TRAl != 0.0) {
        if (TRAc != 0.0) {
            if (TRAz != 0.0)
                return ("%s: L, C and Zo given");
            TRAz = sqrt(TRAl/TRAc);
            if (TRAtd == 0.0) {
                if (TRAlength == 0.0)
                    TRAlength = 1.0;
                TRAtd = TRAz*TRAc*TRAlength;
            }
            else {
                if (TRAlength != 0.0)
                    return ("%s: L, C, Td and Len given");
                TRAlength = TRAtd/(TRAz*TRAc);
            }
        }
        else if (TRAz != 0.0) {
            if (TRAtd != 0.0) {
                if (TRAlength != 0.0)
                    return ("%s: L, Zo, Td, and Len given");
                TRAlength = (TRAz*TRAtd)/TRAl;
                TRAc = TRAl/(TRAz*TRAz);
            }
            else {
                if (TRAlength == 0.0)
                    TRAlength = 1.0;
                TRAc = TRAl/(TRAz*TRAz);
                TRAtd = TRAz*TRAc*TRAlength;
            }
        }
        else if (TRAtd != 0.0) {
            if (TRAlength == 0.0)
                TRAlength = 1.0;
            TRAc = (TRAtd*TRAtd)/(TRAl*TRAlength*TRAlength);
            TRAz = TRAtd/(TRAc*TRAlength);
        }
    }
    else if (TRAc != 0.0) {
        if (TRAz != 0.0) {
            if (TRAtd != 0.0) {
                if (TRAlength != 0.0)
                    return ("%s: C, Zo, Td, and Len given");
                TRAlength = TRAtd/(TRAz*TRAc);
                TRAl = TRAz*TRAz*TRAc;
            }
            else {
                if (TRAlength == 0.0)
                    TRAlength = 1.0;
                TRAl = TRAz*TRAz*TRAc;
                TRAtd = TRAz*TRAc*TRAlength;
            }
        }
        else if (TRAtd != 0.0) {
            if (TRAlength == 0.0)
                TRAlength = 1.0;
            TRAl = (TRAtd*TRAtd)/(TRAc*TRAlength*TRAlength);
            TRAz = TRAtd/(TRAlength*TRAc);
        }
    }
    else if (TRAz != 0.0) {
        if (TRAtd == 0.0)
            return ("%s: Zo given, need L, C, or Td");
        if (TRAlength == 0.0)
            TRAlength = 1.0;
        TRAl = TRAtd*TRAz/(TRAlength);
        TRAc = TRAtd/(TRAlength*TRAz);
    }

    TRAcase = 0;
    if (TRAr == 0.0 && TRAg == 0.0 && TRAc != 0.0 && TRAl != 0.0)
        TRAcase = TRA_LC;
    else if (TRAr != 0.0 && TRAg == 0.0 && TRAc != 0.0 && TRAl != 0.0)
        TRAcase = TRA_RLC;
    else if (TRAr != 0.0 && TRAg == 0.0 && TRAc != 0.0 && TRAl == 0.0)
        TRAcase = TRA_RC;
    else if (TRAr != 0.0 && TRAg != 0.0 && TRAc == 0.0 && TRAl == 0.0)
        TRAcase = TRA_RG;
    else if (TRAr != 0.0 && TRAg == 0.0 && TRAc == 0.0 && TRAl != 0.0)
        TRAcase = TRA_RL;
    if (TRAlength == 0.0)
        TRAlength = 1.0;

    if ((TRAr == 0.0 ? 0 : 1) + (TRAg == 0.0 ? 0 : 1) +
            (TRAl == 0.0 ? 0 : 1) + (TRAc == 0.0 ? 0 : 1) <= 1)
        return ("%s: insufficient input (LCRG) specified for line");

    if (TRAlevel == CONV_LEVEL) {
        if (TRAg != 0.0 && (TRAc != 0.0 || TRAl != 0.0))
            return (
                "%s: Nonzero G (except RG) not supported in convolution model"
                );
        if (TRAcase == TRA_RL)
            return ("%s: RL line not supported convolution model");
    }
    return (0);
}


//--------------------------------------------------------------------------
//  Pade approximation convolution
//--------------------------------------------------------------------------

namespace {
    bool pade33(double, double*, double[4], double[4]);
    int find_roots(double, double, double, double*, double*, double*);
    void mac(double, double, double*, double*, double*, double*, double*);
    void get_c(double, double, double, double, double, double, double,
        double*, double*);
}

#define epsi 1.0e-16
#define epsi2 1.0e-28


int
sTRAinstance::pade_setup(sCKT*)
{
    TRAtx = TXLine();
    TRAtx2 = TXLine();
    if (TRAr/TRAl < 5.0e+5 && TRAg < 1.0e-2) {
        TRAtx.lsl = 1;  // lossless line
        TRAtx.taul = sqrt(TRAc*TRAl)*TRAlength;
        TRAtx.h3_aten = TRAtx.sqtCdL = sqrt(TRAc/TRAl);
        TRAtx.h2_aten = 1.0;
        TRAtx.h1C = 0.0;
    }

    if (!TRAtx.lsl) {
        if (!TRAtx.main_pade(TRAr, TRAl, TRAg, TRAc, TRAlength))
            return (E_PARMVAL);
    }
    return (OK);
}


namespace {
    inline double
    eval2(double a, double b, double c, double x)
    {
        return (a*x*x + b*x + c);
    }
}


bool
TXLine::main_pade(double R, double L, double G, double C, double l)
{
    double sqtcdl = sqrt(C/L);
    double rdl = R/L;
    double gdc = G/C;

    double b[6];
    b[0] = 1.0;
    mac(gdc, rdl, b+1, b+2, b+3, b+4, b+5);
    double n[4], d[4];
    pade33(sqrt(gdc/rdl), b, n, d);

    double x1, x2, x3;
    if (find_roots(d[1], d[2], d[3], &x1, &x2, &x3)) {
        DVO.textOut(OUT_FATAL, "Pade approximation failed, complex roots");
        return (false);
    }
    double c1 = eval2(n[1] - d[1], n[2] - d[2], n[3] - d[3], x1)/ 
        eval2(3.0, 2.0*d[1], d[2], x1);
    double c2 = eval2(n[1] - d[1], n[2] - d[2], n[3] - d[3], x2)/ 
        eval2(3.0, 2.0*d[1], d[2], x2);
    double c3 = eval2(n[1] - d[1], n[2] - d[2], n[3] - d[3], x3)/ 
        eval2(3.0, 2.0*d[1], d[2], x3);

    sqtCdL = sqtcdl;
    h1_term[0].c = c1;
    h1_term[1].c = c2;
    h1_term[2].c = c3;
    h1_term[0].x = x1;
    h1_term[1].x = x2;
    h1_term[2].x = x3;

    double tau = sqrt(L*C);
    double aa = R/L;
    double bb = G/C;

    double y1 = 0.5*(aa + bb);
    double y2 = aa*bb - y1*y1;
    double y3 = -3.0*y1*y2;
    double y4 = -3.0*y2*y2 - 4.0*y1*y3;
    double y5 = -5.0*y1*y4 - 10.0*y2*y3;
    double y6 = -10.0*y3*y3 - 15.0*y2*y4 - 6.0*y1*y5;

    double a[6];
    double tt = tau;
    a[0] = -l*y1*tt;
    tt *= tau;
    a[1] = -l*y2*tt/2.0;
    tt *= tau;
    a[2] = -l*y3*tt/6.0;
    tt *= tau;
    a[3] = -l*y4*tt/24.0;
    tt *= tau;
    a[4] = -l*y5*tt/120.0;
    tt *= tau;
    a[5] = -l*y6*tt/720.0;

    b[0] = 1.0;
    b[1] = a[1];
    for (int i = 2; i <= 5; i++) {
        b[i] = 0.0;
        for (int j = 1; j <= i; j++)
            b[i] += j*a[j]*b[i-j];
        b[i] = b[i]/i;
    }

    pade33(exp(-a[0] - l*sqrt(R*G)), b, n, d);

    tt = 1.0/tau;
    double tt_1 = tt;
    d[1] *= tt;
    n[1] *= tt;
    tt *= tt_1;
    d[2] *= tt;
    n[2] *= tt;
    tt *= tt_1;
    d[3] *= tt;
    n[3] *= tt;

    ifImg = find_roots(d[1], d[2], d[3], &x1, &x2, &x3);
    c1 = eval2(n[1] - d[1], n[2] - d[2], n[3] - d[3], x1)/ 
        eval2(3.0, 2.0*d[1], d[2], x1);
    if (ifImg) 
        get_c(n[1] - d[1], n[2] - d[2], n[3] - d[3], d[1], d[2],
            x2, x3, &c2, &c3);
    else {
        c2 = eval2(n[1] - d[1], n[2] - d[2], n[3] - d[3], x2)/ 
            eval2(3.0, 2.0*d[1], d[2], x2);
        c3 = eval2(n[1] - d[1], n[2] - d[2], n[3] - d[3], x3)/ 
            eval2(3.0, 2.0*d[1], d[2], x3);
    }

    taul = tau*l;
    h2_aten = exp(a[0]);
    h2_term[0].c = c1;
    h2_term[1].c = c2;
    h2_term[2].c = c3;
    h2_term[0].x = x1;
    h2_term[1].x = x2;
    h2_term[2].x = x3;

    get_h3();
    update_h1C_c();
    return (true);
}


namespace {
    inline void
    div_C(double ar, double ai, double br, double bi, double *cr, double *ci)
    {
        *cr = ar*br + ai*bi;
        *ci = -ar*bi + ai*br;
        *cr = *cr/(br*br + bi*bi);
        *ci = *ci/(br*br + bi*bi);
    }
}


void
TXLine::get_h3()
{
    double xx1, xx2, xx3, xx4, xx5, xx6;
    h3_aten = h2_aten * sqtCdL;
    h3_term[0].x = xx1 = h1_term[0].x;
    h3_term[1].x = xx2 = h1_term[1].x;
    h3_term[2].x = xx3 = h1_term[2].x;
    h3_term[3].x = xx4 = h2_term[0].x;
    h3_term[4].x = xx5 = h2_term[1].x;
    h3_term[5].x = xx6 = h2_term[2].x;
    double cc1 = h1_term[0].c;
    double cc2 = h1_term[1].c;
    double cc3 = h1_term[2].c;
    double cc4 = h2_term[0].c;
    double cc5 = h2_term[1].c;
    double cc6 = h2_term[2].c;

    if (ifImg) {

        h3_term[0].c = cc1 + cc1 * (cc4/(xx1-xx4) + 
          2.0*(cc5*xx1-xx6*cc6-xx5*cc5)/(xx1*xx1-2.0*xx5*xx1+xx5*xx5+xx6*xx6));
        h3_term[1].c = cc2 + cc2 * (cc4/(xx2-xx4) + 
          2.0*(cc5*xx2-xx6*cc6-xx5*cc5)/(xx2*xx2-2.0*xx5*xx2+xx5*xx5+xx6*xx6));
        h3_term[2].c = cc3 + cc3 * (cc4/(xx3-xx4) + 
          2.0*(cc5*xx3-xx6*cc6-xx5*cc5)/(xx3*xx3-2.0*xx5*xx3+xx5*xx5+xx6*xx6));

        h3_term[3].c = cc4 + cc4 * (cc1/(xx4-xx1) +
          cc2/(xx4-xx2) + cc3/(xx4-xx3));

        double r, i;
        h3_term[4].c = cc5;
        h3_term[5].c = cc6;
        div_C(cc5, cc6, xx5-xx1, xx6, &r, &i);
        h3_term[4].c += r*cc1;
        h3_term[5].c += i*cc1;
        div_C(cc5, cc6, xx5-xx2, xx6, &r, &i);
        h3_term[4].c += r*cc2;
        h3_term[5].c += i*cc2;
        div_C(cc5, cc6, xx5-xx3, xx6, &r, &i);
        h3_term[4].c += r*cc3;
        h3_term[5].c += i*cc3;
    }
    else {
        h3_term[0].c =
            cc1 + cc1*(cc4/(xx1-xx4) + cc5/(xx1-xx5) + cc6/(xx1-xx6));
        h3_term[1].c =
            cc2 + cc2*(cc4/(xx2-xx4) + cc5/(xx2-xx5) + cc6/(xx2-xx6));
        h3_term[2].c =
            cc3 + cc3*(cc4/(xx3-xx4) + cc5/(xx3-xx5) + cc6/(xx3-xx6));

        h3_term[3].c =
            cc4 + cc4*(cc1/(xx4-xx1) + cc2/(xx4-xx2) + cc3/(xx4-xx3));
        h3_term[4].c =
            cc5 + cc5*(cc1/(xx5-xx1) + cc2/(xx5-xx2) + cc3/(xx5-xx3));
        h3_term[5].c =
            cc6 + cc6*(cc1/(xx6-xx1) + cc2/(xx6-xx2) + cc3/(xx6-xx3));
    }
}


void
TXLine::update_h1C_c()
{
    double d = 0;
    for (int i = 0; i < 3; i++) {
        h1_term[i].c *= sqtCdL;
        d += h1_term[i].c;
    }
    h1C = d;
    for (int i = 0; i < 3; i++) 
        h2_term[i].c *= h2_aten;
    for (int i = 0; i < 6; i++) 
        h3_term[i].c *= h3_aten;
}


namespace {
    //  b[0] + b[1]*y + b[2]*y^2 + ... + b[5]*y^5 + ...
    //    = (n3*y^3 + n2*y^2 + n1*y + 1) / (d3*y^3 + d2*y^2 + d1*y + 1)
    //     
    //  where b[0] is always equal to 1.0 and neglected, and y = 1/s.
    //
    bool 
    pade33(double a_b, double *b, double n[4], double d[4])
    {
        double A[3][4];
        A[0][0] = 1.0 - a_b;
        A[0][1] = b[1];
        A[0][2] = b[2];
        A[0][3] = -b[3];

        A[1][0] = b[1];
        A[1][1] = b[2];
        A[1][2] = b[3];
        A[1][3] = -b[4];

        A[2][0] = b[2];
        A[2][1] = b[3];
        A[2][2] = b[4];
        A[2][3] = -b[5];

        // Gaussian elimination
        for (int i = 0; i < 3; i++) {
            int imax = i;
            double max = FABS(A[i][i]);
            for (int j = i+1; j < 3; j++) {
                if (FABS(A[j][i]) > max) {
                    imax = j;
                    max = FABS(A[j][i]);
                }
            }
            if (max < epsi) {
                DVO.textOut(OUT_FATAL, "cpl, can not find a pivot");
                return (false);
            }
            if (imax != i) {
                for (int k = i; k <= 3; k++) {
                    double f = A[i][k];
                    A[i][k] = A[imax][k];
                    A[imax][k] = f;
                }
            }

            double f = 1.0/A[i][i];
            A[i][i] = 1.0;

            for (int j = i+1; j <= 3; j++)
                A[i][j] *= f;

            for (int j = 0; j < 3 ; j++) {
                if (i == j)
                    continue;
                f = A[j][i];
                A[j][i] = 0.0;
                for (int k = i+1; k <= 3; k++)
                    A[j][k] -= f*A[i][k];
            }
        }

        d[3] = A[0][3];
        d[2] = A[1][3];
        d[1] = A[2][3];
        d[0] = 1.0;
        n[0] = 1.0;
        n[1] = d[1] + b[1];
        n[2] = b[1]*d[1] + d[2] + b[2];
        n[3] = d[3]*a_b;

        return (true);
    }


    inline double
    root3(double a1, double a2, double a3, double x)
    {
        double t1 = x*x*x + a1*x*x + a2*x + a3;
        double t2 = 3.0*x*x + 2.0*a1*x + a2;
        return (x - t1/t2);
    }


    inline void
    div3(double a1, double, double a3, double x, double *p1, double *p2)
    {
        *p1 = a1 + x;
        *p2 = -a3/x;
    }


    int
    find_roots(double a1, double a2, double a3, double *x1, double *x2,
        double *x3)
    {
        double q = (a1*a1 - 3.0*a2)/9.0;
        double p = (2.0*a1*a1*a1 - 9.0*a1*a2 + 27.0*a3)/54.0;
        double t = q*q*q - p*p;
        double x;
        if (t >= 0.0) {
            t = acos(p/(q*sqrt(q)));
            x = -2.0*sqrt(q)*cos(t/3.0) - a1/3.0;
        }
        else {
            if (p > 0.0) {
                t = pow(sqrt(-t) + p, 1.0/3.0);
                x = -(t + q/t) - a1/3.0;
            }
            else if (p == 0.0)
                x = -a1/3.0;
            else {
                t = pow(sqrt(-t) - p, 1.0/3.0);
                x = (t + q/t) - a1/3.0;
            }
        }

        int i = 0;
        double xx1 = x;
        for (t = root3(a1, a2, a3, x); FABS(t-x) > 5.0e-4;
                t = root3(a1, a2, a3, x)) {
            if (++i == 32) {
                x = xx1;
                break;
            }
            else
                x = t;
        }

        *x1 = x;
        div3(a1, a2, a3, x, &a1, &a2);

        t = a1*a1 - 4.0*a2;
        if (t < 0) {
            printf("***** Two Imaginary Roots.\n");
            *x3 = 0.5*sqrt(-t);
            *x2 = -0.5*a1;
            return (1);
        }

        t = sqrt(t);
        if (a1 >= 0.0)
            *x2 = t = -0.5*(a1 + t);
        else
            *x2 = t = -0.5*(a1 - t);
        *x3 = a2/t;

        return (0);
    }


    // Calculate the Maclaurin series of F(z)
    //
    //   F(z) = sqrt((1+az) / (1+bz))
    //    = 1 + b1 z + b2 z^2 + b3 z^3 + b4 z^4 + b5 z^5 
    //
    void
    mac(double at, double bt, double *b1, double *b2, double *b3, double *b4,
        double *b5)
    {
        double a = at;
        double b = bt;

        double y1 = *b1 = 0.5 * (a - b);
        double y2 = 0.5*(3.0*b*b - 2.0*a*b - a*a)*y1/(a - b);
        double y3 = ((3.0*b*b + a*a)*y1*y1 + 0.5*(3.0*b*b -
            2.0*a*b - a*a)*y2)/(a - b);
        double y4 = ((3.0*b*b - 3.0*a*a)*y1*y1*y1 + (9.0*b*b + 3.0*a*a)*y1*y2 +
            0.5*(3.0*b*b - 2.0*a*b - a*a)*y3)/(a - b);
        double y5 = (12.0*a*a*y1*y1*y1*y1 + y1*y1*y2*(18.0*b*b - 18.0*a*a) +
            (9.0*b*b + 3.0*a*a)*(y2*y2 + y1*y3) + (3.0*b*b + a*a)*y1*y3 +
            0.5*(3.0*b*b - 2.0*a*b - a*a)*y4)/(a - b);

        *b2 = y2/2.0;
        *b3 = y3/6.0;
        *b4 = y4/24.0;
        *b5 = y5/120.0;
    }


    void
    get_c(double q1, double q2, double q3, double p1, double p2,
        double a, double b, double *cr, double *ci)
    {
        double d =
            (3.0*(a*a - b*b) + 2.0*p1*a + p2)*(3.0*(a*a - b*b) + 2.0*p1*a + p2);
        d += (6.0*a*b + 2.0*p1*b)*(6.0*a*b + 2.0*p1*b);
        double n = -(q1*(a*a - b*b) + q2*a + q3)*(6.0*a*b + 2.0*p1*b);
        n += (2.0*q1*a*b + q2*b)*(3.0*(a*a - b*b)+2.0*p1*a + p2);
        *ci = n/d;
        n = (3.0*(a*a - b*b) + 2.0*p1*a + p2)*(q1*(a*a - b*b) + q2*a + q3);
        n += (6.0*a*b + 2.0*p1*b)*(2.0*q1*a*b + q2*b);
        *cr = n/d;
    }
}


//--------------------------------------------------------------------------
//  Full convolution - SPICE3 LTRA
//--------------------------------------------------------------------------


// TRAstraightLineCheck - takes the co-ordinates of three points,
// finds the area of the triangle enclosed by these points and
// compares this area with the area of the quadrilateral formed by
// the line between the first point and the third point, the
// perpendiculars from the first and third points to the x-axis, and
// the x-axis. If within reltol, then it returns 1, else 0. The
// purpose of this function is to determine if three points lie
// acceptably close to a straight line. This area criterion is used
// because it is related to integrals and convolution
//
inline int
straightLineCheck(double x1, double y1, double x2, double y2,
    double x3, double y3, double reltol, double abstol)
{
    // this should work if y1,y2,y3 all have the same sign and x1,x2,x3
    // are in increasing order
    //
    double d = x2 - x1;
    double QUADarea1 = (FABS(y2)+FABS(y1))*0.5*FABS(d);
    d = x3 - x2;
    double QUADarea2 = (FABS(y3)+FABS(y2))*0.5*FABS(d);
    d = x3 - x1;
    double QUADarea3 = (FABS(y3)+FABS(y1))*0.5*FABS(d);
    double temp = QUADarea3 - QUADarea1 - QUADarea2;
    double TRarea = FABS(temp);
    double area = QUADarea1 + QUADarea2;
    if (area*reltol + abstol > TRarea)
        return (1);
    return (0);
}


int
sTRAinstance::ltra_setup(sCKT *ckt)
{
    if (ckt->CKTcurTask->TSKtryToCompact && TRAhowToInterp == TRA_QUADINTERP) {
        TRAhowToInterp = TRA_LININTERP;
        DVO.textOut(OUT_WARNING,
        "%s: using linear interpolation because trytocompact option specified",
            GENname);
    }

    for (sTRAconvModel *cm = ((sTRAmodel*)GENmodPtr)->TRAconvModels; cm;
            cm = cm->next) {
        if (cm->TRAl == TRAl && cm->TRAc == TRAc && cm->TRAr == TRAr &&
                cm->TRAg == TRAg && cm->TRAlength == TRAlength) {
            TRAconvModel = cm;
            break;
        }
    }
    if (!TRAconvModel) {
        TRAconvModel = new sTRAconvModel;
        int error = TRAconvModel->setup(ckt, this);
        if (error)
            return (error);
        TRAconvModel->next = ((sTRAmodel*)GENmodPtr)->TRAconvModels;
        ((sTRAmodel*)GENmodPtr)->TRAconvModels = TRAconvModel;
    }

    if (TRAcase == TRA_RLC && TRAlteConType != TRA_TRUNCDONTCUT) {

        // There was a bug<?) in spice3 here - the args to the second
        // line check were identical to the first call, so all of the
        // y2 terms were superfluous.  Replacing with the y2 values in
        // the original algorithm cut the resulting step size too
        // much, so here we do a binary search to find a more accurate
        // value.  This is worthwhile since maxSafeStep has a profound
        // effect on default simulation speed.

        int maxiter = 50;
        double xbig = TRAtd + 9*TRAtd;
        // hack! ckt is not yet initialised...
        double xsmall = TRAtd;
        double xmid = 0.5*(xbig + xsmall);
        double y1small = TRAconvModel->rlcH2Func(xsmall);
        double y2small = TRAconvModel->rlcH3dashFunc(xsmall, TRAtd,
            TRAconvModel->TRAbeta, TRAconvModel->TRAbeta);
        int iters = 0;
        int done0 = 0;
        double xdel = 0;
        for (;;) {
            iters++;
            double y1big = TRAconvModel->rlcH2Func(xbig);
            double y1mid = TRAconvModel->rlcH2Func(xmid);
            double y2big = TRAconvModel->rlcH3dashFunc(xbig, TRAtd,
                TRAconvModel->TRAbeta, TRAconvModel->TRAbeta);
            double y2mid = TRAconvModel->rlcH3dashFunc(xmid, TRAtd,
                TRAconvModel->TRAbeta, TRAconvModel->TRAbeta);
            int done = straightLineCheck(xbig, y1big, xmid, y1mid, xsmall,
                y1small, TRAstLineReltol, TRAstLineAbstol);
            done += straightLineCheck(xbig, y2big, xmid, y2mid, xsmall,
                y2small, TRAstLineReltol, TRAstLineAbstol);

            if (iters > maxiter)
                break;
            if (done == 2) {
                if (!done0) {
                    maxiter = iters + 8;
                    done0 = 1;
                    xdel = xbig - xmid;
                }
                xbig += xdel;
                xdel *= 0.5;
            }
            else {
                if (done0) {
                    xbig -= xdel;
                    xdel *= 0.5;
                }
                else
                    xbig = xmid;
            }
            xmid = 0.5*(xbig + xsmall);
        }
        TRAmaxSafeStep = xbig - TRAtd;
    }
    return (OK);
}


int
sTRAconvModel::setup(sCKT*, sTRAinstance *inst)
{
    TRAl = inst->TRAl;
    TRAc = inst->TRAc;
    TRAr = inst->TRAr;
    TRAg = inst->TRAg;
    TRAlength = inst->TRAlength;
    TRAtd = inst->TRAtd;

    if (inst->TRAcase == TRA_LC) {
        TRAadmit = 1.0/inst->TRAz;
        TRAattenuation = 1.0;
    }
    else if (inst->TRAcase == TRA_RLC) {
        TRAadmit = 1.0/inst->TRAz;

/*
        double gdc = TRAg/TRAc;
        double rdl = TRAr/TRAl;
        TRAalpha = 0.5*(rdl - gdc);
        TRAbeta = 0.5*(rdl + gdc);
*/

        TRAalpha = 0.5*TRAr/TRAl;
        TRAbeta = TRAalpha;
        TRAattenuation = exp(-TRAbeta*TRAtd);
        if (TRAalpha > 0.0) {
            TRAintH1dash = -1.0;
            TRAintH2 = 1.0 - TRAattenuation;
            TRAintH3dash = - TRAattenuation;
        }
        else if (TRAalpha == 0.0)
            TRAintH1dash = TRAintH2 = TRAintH3dash = 0.0;
    }
    else if (inst->TRAcase == TRA_RC) {
        TRAcByR = TRAc/TRAr;
        TRArclsqr = TRAr*TRAc*TRAlength*TRAlength;
        TRAintH1dash = 0.0;
        TRAintH2 = 1.0;
        TRAintH3dash = 0.0;
    }
    else if (inst->TRAcase == TRA_RG) {
        double tmp1 = TRAlength*sqrt(TRAr*TRAg);
        double tmp2 = exp(-tmp1);
        // warning: may overflow!
        tmp1 = exp(tmp1);
        TRAcoshlrootGR = 0.5*(tmp1 + tmp2);

        if (TRAg <= 1.0e-10)  // hack!
            TRArRsLrGRorG = TRAlength*TRAr;
        else
            TRArRsLrGRorG = 0.5*(tmp1 - tmp2)*sqrt(TRAr/TRAg);

        if (TRAr <= 1.0e-10)  // hack!
            TRArGsLrGRorR = TRAlength*TRAg;
        else
            TRArGsLrGRorR = 0.5*(tmp1 - tmp2)*sqrt(TRAg/TRAr);
    }
    else
        return (E_BADPARM);
    return (OK);
}

