
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
Authors: 1988 Jaijeet S Roychowdhury
         1993 Stephen R. Whiteley
****************************************************************************/

#define DISTO
#include "mosdefs.h"
#include "distdefs.h"

#define P16 .16666666667
#define MOD_D_DELTA 1e-5


static void mos_deriv(sMOSmodel*, sMOSinstance*, mosstuff*, Dderivs*);
static double mos_exec(sMOSmodel*, sMOSinstance*, mosstuff*,
    double, double, double);


int
MOSdev::dSetup(sMOSmodel *model, sCKT *ckt)
{
    struct mosstuff ms;
    double gmin = ckt->CKTcurTask->TSKgmin;
    double arg;
    double evb;
    double vgst;
    double sarg;
    double sargsw;
    double lcapgs2;
    double lcapgd2; 
    double lcapgb2;
    double lcapgs3;  
    double lcapgd3; 
    double lcapgb3;
    double lgbs, lgbs2, lgbs3;
    double lgbd, lgbd2, lgbd3;
    double /*lcapbs,*/ lcapbs2, lcapbs3;
    double /*lcapbd,*/ lcapbd2, lcapbd3;
    Dderivs cd;

    for ( ; model; model = model->next()) {
        sMOSinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            ms.ms_vt = CONSTKoverQ * inst->MOStemp;

            if (model->MOStype > 0) {
                ms.ms_vbs = *(ckt->CKTrhsOld + inst->MOSbNode) -
                        *(ckt->CKTrhsOld + inst->MOSsNodePrime);
                ms.ms_vgs = *(ckt->CKTrhsOld + inst->MOSgNode) -
                        *(ckt->CKTrhsOld + inst->MOSsNodePrime);
                ms.ms_vds = *(ckt->CKTrhsOld + inst->MOSdNodePrime) -
                        *(ckt->CKTrhsOld + inst->MOSsNodePrime);
            }
            else {
                ms.ms_vbs = *(ckt->CKTrhsOld + inst->MOSsNodePrime) -
                        *(ckt->CKTrhsOld + inst->MOSbNode);
                ms.ms_vgs = *(ckt->CKTrhsOld + inst->MOSsNodePrime) -
                        *(ckt->CKTrhsOld + inst->MOSgNode);
                ms.ms_vds = *(ckt->CKTrhsOld + inst->MOSsNodePrime) -
                        *(ckt->CKTrhsOld + inst->MOSdNodePrime);
            }
            ms.ms_vbd = ms.ms_vbs - ms.ms_vds;
            ms.ms_vgd = ms.ms_vgs - ms.ms_vds;
            ms.ms_vgb = ms.ms_vgs - ms.ms_vbs;

            //
            // bulk-source and bulk-drain diodes
            // here we just evaluate the ideal diode current and the
            // corresponding derivative (conductance).
            //

            if (ms.ms_vbs <= 0) {
                lgbs  = inst->MOStSourceSatCur/ms.ms_vt + gmin;
                lgbs2 = lgbs3 = 0;
            }
            else {
                evb   = exp(ms.ms_vbs/ms.ms_vt);
                lgbs  = inst->MOStSourceSatCur*evb/ms.ms_vt + gmin;
                if (model->MOStype > 0) {
                    lgbs2 = 0.5*(lgbs - gmin)/ms.ms_vt;
                    lgbs3 = lgbs2/(ms.ms_vt*3);
                }
                else {
                    lgbs2 = -0.5*(lgbs - gmin)/ms.ms_vt;
                    lgbs3 = -lgbs2/(ms.ms_vt*3);
                }

            }
            if (ms.ms_vbd <= 0) {
                lgbd  = inst->MOStDrainSatCur/ms.ms_vt + gmin;
                lgbd2 = lgbd3 = 0;
            }
            else {
                evb   = exp(ms.ms_vbd/ms.ms_vt);
                lgbd  = inst->MOStDrainSatCur*evb/ms.ms_vt + gmin;
                if (model->MOStype > 0) {
                    lgbd2 = 0.5*(lgbd - gmin)/ms.ms_vt;
                    lgbd3 = lgbd2/(ms.ms_vt*3);
                }
                else {
                    lgbd2 = -0.5*(lgbd - gmin)/ms.ms_vt;
                    lgbd3 = -lgbd2/(ms.ms_vt*3);
                }
            }

            // now to determine whether the user was able to correctly
            // identify the source and drain of his device
            //
            if (ms.ms_vds >= 0) {
                // normal mode
                inst->MOSmode = 1;
            }
            else {
                // inverse mode
                inst->MOSmode = -1;
            }

            mos_deriv(model, inst, &ms, &cd);

            //
            //  charge storage elements
            //  bulk-drain and bulk-source depletion capacitances
            //

            if (ms.ms_vbs < inst->MOStDepCap) {
                arg = 1 - ms.ms_vbs/inst->MOStBulkPot;

                SARGS(arg,model->MOSbulkJctBotGradingCoeff,
                    model->MOSbulkJctSideGradingCoeff,sarg,sargsw);

//                lcapbs = inst->MOSCbs*sarg + inst->MOSCbssw*sargsw;
                lcapbs2 = 0.5/inst->MOStBulkPot*
                    (inst->MOSCbs*model->MOSbulkJctBotGradingCoeff*sarg +
                    inst->MOSCbssw*model->MOSbulkJctSideGradingCoeff*sargsw)
                    /arg;
                if (model->MOStype < 0)
                    lcapbs2 = -lcapbs2;
                lcapbs3 = inst->MOSCbs*sarg*
                    model->MOSbulkJctBotGradingCoeff*
                    (model->MOSbulkJctBotGradingCoeff+1);
                lcapbs3 += inst->MOSCbssw*sargsw*
                    model->MOSbulkJctSideGradingCoeff*
                    (model->MOSbulkJctSideGradingCoeff+1);
                lcapbs3 = lcapbs3/(6*inst->MOStBulkPot*
                    inst->MOStBulkPot*arg*arg);
            }
            else {
//                lcapbs  = inst->MOSf2s + inst->MOSf3s*ms.ms_vbs;
                lcapbs2 = 0.5*inst->MOSf3s;
                lcapbs3 = 0;
            }

            if (ms.ms_vbd < inst->MOStDepCap) {
                arg = 1 - ms.ms_vbd/inst->MOStBulkPot;

                SARGS(arg,model->MOSbulkJctBotGradingCoeff,
                    model->MOSbulkJctSideGradingCoeff,sarg,sargsw);

//                lcapbd = inst->MOSCbd*sarg + inst->MOSCbdsw*sargsw;
                lcapbd2 = model->MOStype*0.5/inst->MOStBulkPot*
                    (inst->MOSCbd*model->MOSbulkJctBotGradingCoeff*sarg +
                    inst->MOSCbdsw*model->MOSbulkJctSideGradingCoeff*sargsw)
                    /arg;
                if (model->MOStype < 0)
                    lcapbd2 = -lcapbd2;
                lcapbd3 = inst->MOSCbd*sarg*
                    model->MOSbulkJctBotGradingCoeff*
                    (model->MOSbulkJctBotGradingCoeff+1);
                lcapbd3 += inst->MOSCbdsw*sargsw*
                    model->MOSbulkJctSideGradingCoeff*
                    (model->MOSbulkJctSideGradingCoeff+1);
                lcapbd3 = lcapbd3/(6*inst->MOStBulkPot*
                    inst->MOStBulkPot*arg*arg);
            }
            else {
//                lcapbd  = inst->MOSf2d + ms.ms_vbd * inst->MOSf3d;
                lcapbd2 = 0.5*inst->MOSf3d;
                lcapbd3 = 0;
            }

            //
            //     meyer's capacitor model
            //
     
            // von, vgst and vdsat have already been adjusted for 
            // possible source-drain interchange
            //
            lcapgb2 = 0;
            lcapgb3 = 0;
            lcapgs2 = 0;
            lcapgs3 = 0;
            lcapgd2 = 0;
            lcapgd3 = 0;

            if (inst->MOSmode > 0)
                vgst = ms.ms_vgs - ms.ms_von;
            else
                vgst = ms.ms_vgd - ms.ms_von;

            if (vgst > -inst->MOStPhi) {

                if (vgst <= -.5*inst->MOStPhi) {
                    lcapgb2 = -inst->MOSoxideCap/(4*inst->MOStPhi);
                }
                else if (vgst <= 0) {
                    lcapgb2 = -inst->MOSoxideCap/(4*inst->MOStPhi);
                    lcapgs2 = inst->MOSoxideCap/(3*inst->MOStPhi);
                }
                else {
                    double vds;

                    if (inst->MOSmode > 0)
                        vds = ms.ms_vds;
                    else
                        vds = -ms.ms_vds;

                    if (ms.ms_vdsat > vds) {
                        double vddif, vddif1, vddif2, x1, x2;

                        vddif   = 2.0*ms.ms_vdsat - vds;
                        vddif1  = ms.ms_vdsat - vds;
                        vddif2  = vddif*vddif;
                        x1      = 1/(3*vddif*vddif2);
                        x2      = 1/(9*vddif2*vddif2);

                        lcapgd2 = -ms.ms_vdsat*vds*inst->MOSoxideCap*x1;
                        lcapgd3 = -vds*
                            inst->MOSoxideCap*(vddif - 6*ms.ms_vdsat)*x2;
                        lcapgs2 = -vddif1*vds*inst->MOSoxideCap*x1;
                        lcapgs3 = -vds*
                            inst->MOSoxideCap*(vddif - 6*vddif1)*x2;
                    }
                }
                if (model->MOStype < 0) {
                    lcapgb2 = -lcapgb2;
                    lcapgs2 = -lcapgs2;
                    lcapgd2 = -lcapgd2;
                }
            }

            // the b-s and b-d diodes need no processing ...
            inst->capbs2 = lcapbs2;
            inst->capbs3 = lcapbs3;
            inst->capbd2 = lcapbd2;
            inst->capbd3 = lcapbd3;
            inst->gbs2   = lgbs2;
            inst->gbs3   = lgbs3;
            inst->gbd2   = lgbd2;
            inst->gbd3   = lgbd3;
            inst->capgb2 = lcapgb2;
            inst->capgb3 = lcapgb3;

            //
            //   process to get Taylor coefficients, taking into
            //   account type and mode.
            //

            if (inst->MOSmode == 1) {
                // normal mode - no source-drain interchange

                inst->capgs2 = lcapgs2;
                inst->capgs3 = lcapgs3;
                inst->capgd2 = lcapgd2;
                inst->capgd3 = lcapgd3;

                if (model->MOStype > 0) {
                    inst->cdr_x2  = .5*cd.d2_p2;
                    inst->cdr_y2  = .5*cd.d2_q2;
                    inst->cdr_z2  = .5*cd.d2_r2;
                    inst->cdr_xy  = cd.d2_pq;
                    inst->cdr_yz  = cd.d2_qr;
                    inst->cdr_xz  = cd.d2_pr;
                }
                else {
                    inst->cdr_x2  = -.5*cd.d2_p2;
                    inst->cdr_y2  = -.5*cd.d2_q2;
                    inst->cdr_z2  = -.5*cd.d2_r2;
                    inst->cdr_xy  = -cd.d2_pq;
                    inst->cdr_yz  = -cd.d2_qr;
                    inst->cdr_xz  = -cd.d2_pr;
                }
                inst->cdr_x3  = cd.d3_p3*P16;
                inst->cdr_y3  = cd.d3_q3*P16;
                inst->cdr_z3  = cd.d3_r3*P16;
                inst->cdr_x2z = .5*cd.d3_p2r;
                inst->cdr_x2y = .5*cd.d3_p2q;
                inst->cdr_y2z = .5*cd.d3_q2r;
                inst->cdr_xy2 = .5*cd.d3_pq2;
                inst->cdr_xz2 = .5*cd.d3_pr2;
                inst->cdr_yz2 = .5*cd.d3_qr2;
                inst->cdr_xyz = cd.d3_pqr;
            }
            else {
                // inverse mode - source and drain interchanged
                inst->capgs2 = lcapgd2;
                inst->capgs3 = lcapgd3;
                inst->capgd2 = lcapgs2;
                inst->capgd3 = lcapgs3;

                if (model->MOStype > 0) {
                    inst->cdr_x2 = -.5*cd.d2_p2;
                    inst->cdr_y2 = -.5*cd.d2_q2;
                    inst->cdr_z2 = -.5*(cd.d2_p2 + cd.d2_q2 + cd.d2_r2 +
                                    2*(cd.d2_pq + cd.d2_pr + cd.d2_qr));
                    inst->cdr_xy = -cd.d2_pq;
                    inst->cdr_yz = cd.d2_pq + cd.d2_q2 + cd.d2_qr;
                    inst->cdr_xz = cd.d2_p2 + cd.d2_pq + cd.d2_pr;
                }
                else {
                    inst->cdr_x2 = .5*cd.d2_p2;
                    inst->cdr_y2 = .5*cd.d2_q2;
                    inst->cdr_z2 = .5*(cd.d2_p2 + cd.d2_q2 + cd.d2_r2 +
                                    2*(cd.d2_pq + cd.d2_pr + cd.d2_qr));
                    inst->cdr_xy = cd.d2_pq;
                    inst->cdr_yz = -(cd.d2_pq + cd.d2_q2 + cd.d2_qr);
                    inst->cdr_xz = -(cd.d2_p2 + cd.d2_pq + cd.d2_pr);
                }

                inst->cdr_x3 = -P16*cd.d3_p3;
                inst->cdr_y3 = -P16*cd.d3_q3;
                inst->cdr_z3 = P16*(cd.d3_p3 + cd.d3_q3 + cd.d3_r3 + 
                                3*(cd.d3_p2q + cd.d3_p2r + cd.d3_pq2 +
                                cd.d3_q2r + cd.d3_pr2 + cd.d3_qr2) +
                                6*cd.d3_pqr);
                inst->cdr_x2z = .5*(cd.d3_p3 + cd.d3_p2q + cd.d3_p2r);
                inst->cdr_x2y = -.5*cd.d3_p2q;
                inst->cdr_y2z = .5*(cd.d3_pq2 + cd.d3_q3 + cd.d3_q2r);
                inst->cdr_xy2 = -.5*cd.d3_pq2;
                inst->cdr_xz2 = -.5*(cd.d3_p3 + 2*(cd.d3_p2q + cd.d3_p2r +
                                cd.d3_pqr) + cd.d3_pq2 + cd.d3_pr2);
                inst->cdr_yz2 = -.5*(cd.d3_q3 + 2*(cd.d3_pq2 + cd.d3_q2r +
                                cd.d3_pqr) + cd.d3_p2q + cd.d3_qr2);
                inst->cdr_xyz = cd.d3_p2q + cd.d3_pq2 + cd.d3_pqr;

            }
        }
    }
    return(OK);
}


static void
mos_deriv(sMOSmodel *model, sMOSinstance *inst, mosstuff *ms, Dderivs *cd)
{
    double delta = MOD_D_DELTA, dd = 1.0/delta;

    double a222 = mos_exec(model, inst, ms, 0.0, 0.0, 0.0);

    double a322 = mos_exec(model, inst, ms, delta, 0.0, 0.0);
    double a232 = mos_exec(model, inst, ms, 0.0, delta, 0.0);
    double a223 = mos_exec(model, inst, ms, 0.0, 0.0, delta);

    double a122 = mos_exec(model, inst, ms, -delta, 0.0, 0.0);
    double a212 = mos_exec(model, inst, ms, 0.0, -delta, 0.0);
    double a221 = mos_exec(model, inst, ms, 0.0, 0.0, -delta);

    double a233 = mos_exec(model, inst, ms, 0.0, delta, delta);
    double a323 = mos_exec(model, inst, ms, delta, 0.0, delta);
    double a332 = mos_exec(model, inst, ms, delta, delta, 0.0);

    double a333 = mos_exec(model, inst, ms, delta, delta, delta);

    double a123 = mos_exec(model, inst, ms, -delta, 0.0, delta);
    double a132 = mos_exec(model, inst, ms, -delta, delta, 0.0);
    double a213 = mos_exec(model, inst, ms, 0.0, -delta, delta);
    double a312 = mos_exec(model, inst, ms, delta, -delta, 0.0);
    double a321 = mos_exec(model, inst, ms, delta, 0.0, -delta);
    double a231 = mos_exec(model, inst, ms, 0.0, delta, -delta);


    double a022 = mos_exec(model, inst, ms, -(delta+delta), 0.0, 0.0);
    double a202 = mos_exec(model, inst, ms, 0.0, -(delta+delta), 0.0);
    double a220 = mos_exec(model, inst, ms, 0.0, 0.0, -(delta+delta));

    cd->d2_p2  = ((a322 - a222)*dd - (a222 - a122)*dd)*dd;
    cd->d2_q2  = ((a232 - a222)*dd - (a222 - a212)*dd)*dd;
    cd->d2_r2  = ((a223 - a222)*dd - (a222 - a221)*dd)*dd;
    cd->d2_pq  = ((a332 - a322)*dd - (a232 - a222)*dd)*dd;
    cd->d2_qr  = ((a233 - a232)*dd - (a223 - a222)*dd)*dd;
    cd->d2_pr  = ((a323 - a322)*dd - (a223 - a222)*dd)*dd;

    cd->d3_p3  = (((a322 - a222)*dd - (a222 - a122)*dd)*dd -
                  ((a222 - a122)*dd - (a122 - a022)*dd)*dd)*dd;
    cd->d3_q3  = (((a232 - a222)*dd - (a222 - a212)*dd)*dd -
                  ((a222 - a212)*dd - (a212 - a202)*dd)*dd)*dd;
    cd->d3_r3  = (((a223 - a222)*dd - (a222 - a221)*dd)*dd -
                  ((a222 - a221)*dd - (a221 - a220)*dd)*dd)*dd;

    cd->d3_p2r = (((a323 - a223)*dd - (a223 - a123)*dd)*dd -
                  ((a322 - a222)*dd - (a222 - a122)*dd)*dd)*dd;

    cd->d3_p2q = (((a332 - a232)*dd - (a232 - a132)*dd)*dd -
                  ((a322 - a222)*dd - (a222 - a122)*dd)*dd)*dd;

    cd->d3_q2r = (((a233 - a223)*dd - (a223 - a213)*dd)*dd -
                  ((a232 - a222)*dd - (a222 - a212)*dd)*dd)*dd;

    cd->d3_pq2 = (((a332 - a232)*dd - (a322 - a222)*dd)*dd -
                  ((a322 - a222)*dd - (a312 - a212)*dd)*dd)*dd;

    cd->d3_pr2 = (((a323 - a223)*dd - (a322 - a222)*dd)*dd -
                  ((a322 - a222)*dd - (a321 - a221)*dd)*dd)*dd;

    cd->d3_qr2 = (((a233 - a223)*dd - (a232 - a222)*dd)*dd -
                  ((a232 - a222)*dd - (a231 - a221)*dd)*dd)*dd;

    cd->d3_pqr = (((a333 - a233)*dd - (a323 - a223)*dd)*dd -
                  ((a332 - a232)*dd - (a322 - a222)*dd)*dd)*dd;
        
}

// from now on, p=vgs, q=vbs, r=vds

static double
mos_exec(sMOSmodel *model, sMOSinstance *inst, mosstuff *ms, double dvgs,
    double dvbs, double dvds)
{
    double vgs, vbs, vds;

    vgs = ms->ms_vgs;
    vbs = ms->ms_vbs;
    vds = ms->ms_vds;

    ms->ms_vgs += dvgs;
    ms->ms_vbs += dvbs;
    ms->ms_vds += dvds;
    ms->ms_vbd = ms->ms_vbs - ms->ms_vds;
    ms->ms_vgd = ms->ms_vgs - ms->ms_vds;
    ms->ms_vgb = ms->ms_vgs - ms->ms_vbs;

    if (model->MOSlevel == 1)
        ms->ms_cdrain = ms->eq1(model, inst);
    else if (model->MOSlevel == 2)
        ms->ms_cdrain = ms->eq2(model, inst);
    else
        ms->ms_cdrain = ms->eq3(model, inst);

    ms->ms_vgs = vgs;
    ms->ms_vbs = vbs;
    ms->ms_vds = vds;
    ms->ms_vbd = ms->ms_vbs - ms->ms_vds;
    ms->ms_vgd = ms->ms_vgs - ms->ms_vds;
    ms->ms_vgb = ms->ms_vgs - ms->ms_vbs;

    return (ms->ms_cdrain);
}
