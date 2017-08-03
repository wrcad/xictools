
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


namespace {
    inline double true_time(double tmp, int order)
    {
        if (order == 2)
            return (sqrt(tmp));
        if (order == 1)
            return (tmp);
        if (order == 3)
            return (cbrt(tmp));
        if (order == 4)
            return (sqrt(sqrt(tmp)));
        return (exp(log(tmp)/order));
    }
}


int
TRAdev::trunc(sGENmodel *genmod, sCKT *ckt, double *timeStep)
{
    if (ckt->CKTtimeIndex <= 1)
        return (OK);

    // In WRspice, the timeStep passed to the trunc functions is
    // not normalized to integration order, which saves having to
    // call an expensive math operation at each CKTterr() call.
    // We have to undo this here.
    //
    double time_step = true_time(*timeStep, ckt->CKTorder);

    // Don't go below this value.
    double mindt = 0.5*ckt->CKTdeltaOld[0];

    bool had_conv = false;
    sTRAmodel *model = static_cast<sTRAmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sTRAinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (inst->TRAlevel == CONV_LEVEL)
                had_conv = true;
            int error = inst->limit_timestep(ckt, &time_step, mindt);
            if (error)
                return (error);
        }
    }
    if (had_conv) {
        // limit rate of increase of time step to reduce truncation error
        if (time_step > 1.1*ckt->CKTdeltaOld[0])
            time_step = 1.1*ckt->CKTdeltaOld[0];
    }

    // see note above regarding renormalization
    double tmp = time_step;
    for (int i = ckt->CKTorder - 1; i > 0; i--)
        tmp *= time_step;
    if (tmp < *timeStep)
        *timeStep = tmp;

    return (OK);
}


// This function computes the difference between the quadratic
// extrapolation, and linear extrapolation from the last two points.
// It puts the time delta where they differ by vd in *dt, if a
// solution exists, and returns 0.  One is returned if no solution
// exists.
//
inline int
step_compute(sCKT *ckt, double v1, double v2, double v3, double vd, double *dt)
{
    double d21 = ckt->CKTdeltaOld[0];
    double d32 = ckt->CKTdeltaOld[1];
    double d31 = d21 + d32;
    double c = v1/(d21*d31) - v2/(d32*d21) + v3/(d32*d31);
    if (c == 0.0) {
        *dt = 0.0;
        return (1);
    }
    if (c < 0.0)
        c = -c;
    *dt = 0.5*(-d21 + sqrt(d21*d21 + 4*vd/c));
    return (0);
}


namespace {
    inline bool check_set_min(double *td, double val, double mintd)
    {
        if (val < mintd)
            val = mintd;
        if (val < *td) {
            *td = val;
            return (true);
        }
        return (false);
    }
}


int
sTRAinstance::limit_timestep(sCKT *ckt, double *td, double mintd)
{
    // limit next time step
    if (TRAtd != 0) {
        check_set_min(td, 0.5*TRAtd, mintd);
    }
    if (TRAlevel == PADE_LEVEL) {
        TXLine *tx = &TRAtx;
        if (!tx->lsl) {
            // Use slopetol to give some control here.  The default value
            // is assumed to be 0.1.

            check_set_min(td, tx->taul*TRAslopetol, mintd);
        }
    }
    else if (TRAlevel == CONV_LEVEL) {
        if (TRAcase == TRA_RLC && TRAlteConType != TRA_TRUNCDONTCUT)
            check_set_min(td, TRAmaxSafeStep, mintd);

        if (TRAcase == TRA_RC && (TRAlteConType == TRA_TRUNCCUTLTE ||
                TRAlteConType == TRA_TRUNCCUTNR)) {

            // Estimate the local truncation error in each of the
            // three convolution equations, and if possible adjust the
            // timestep so that all of them remain within some bound. 
            // Unfortunately, the expression for the LTE in a
            // convolution operation is complicated and costly to
            // evaluate; in addition, no explicit inverse exists.
            //
            // So what we do here (for the moment) is check to see the
            // current error is acceptable.  If so, the timestep is
            // not changed.  If not, then an estimate is made for the
            // new timestep using a few iterations of the
            // newton-raphson method.
            //
            // modification:  we change the timestep to half its
            // previous value
            //
            int tp = ckt->CKTtimeIndex;
            double v1 = TRAvalues[tp].v_i + TRAz*TRAvalues[tp].i_i;
            double v2 = TRAvalues[tp].v_o + TRAz*TRAvalues[tp].i_o;
            double tolerance =
                ckt->CKTcurTask->TSKtrtol*(ckt->CKTcurTask->TSKreltol*
                    (FABS(v1) + FABS(v2)) 
                    + ckt->CKTcurTask->TSKabstol);

            double current_lte =
                TRAconvModel->lteCalculate(ckt, this, ckt->CKTtime);

            if (current_lte >= tolerance) {
                if (TRAlteConType == TRA_TRUNCCUTNR) {

                    double x = ckt->CKTtime;
                    double y = current_lte;
                    int maxiter = 2, iterations = 0;
                    for (;;) {
                        double deriv_delta =
                            0.01*(x - ckt->CKTtimePoints[ckt->CKTtimeIndex]);

#ifdef TRADEBUG
                        if (deriv_delta <= 0.0)
                            fprintf(stdout,
                        "TRAtrunc: error: timestep is now less than zero\n");
#endif
                        double deriv = TRAconvModel->lteCalculate(ckt, this,
                            x + deriv_delta) - y;
                        deriv /= deriv_delta;
                        double change = (tolerance - y)/deriv;
                        x += change;
                        if (maxiter == 0) {
                            if (FABS(change) <= FABS(deriv_delta))
                                break;
                        }
                        else {
                            iterations++;
                            if (iterations >= maxiter)
                                break;
                        }
                        y = TRAconvModel->lteCalculate(ckt, this, x);
                    }
                    double tmp = x - ckt->CKTtimePoints[ckt->CKTtimeIndex];
#ifdef TRADEBUG
                    if (check_set_min(td, tmp, mintd))
                        fprintf(stdout, "nr cut %g to %g\n", *td, tmp);
#else
                    check_set_min(td, tmp, mintd);
#endif
                }
                else {
#ifdef TRADEBUG
                    if (check_set_min(td, 0.5*ckt->CKTdeltaOld[0], mintd))
                        fprintf(stdout, "lte cut %g to %g\n", *td, tmp);
#else
                    check_set_min(td, 0.5*ckt->CKTdeltaOld[0], mintd);
#endif
                }
            }
        }
    }

    // Slope change truncation
    if (TRAlteConType == TRA_TRUNCCUTSL) {
        int tp = ckt->CKTtimeIndex;
        if (tp < 2)
            return (OK);

        /*****  Seems not helpful, disabled for now.
        // If the deltas are too different, slopetol calc may be bogus.
        double d1 = ckt->CKTdeltaOld[1]/ckt->CKTdeltaOld[0];
        if (d1 ? 10.0 || d1 < 0.1)
            return (OK);
        *****/

        double v1 = TRAvalues[tp].v_i + TRAz*TRAvalues[tp].i_i;
        double v2 = TRAvalues[tp-1].v_i + TRAz*TRAvalues[tp-1].i_i;
        double v3 = TRAvalues[tp-2].v_i + TRAz*TRAvalues[tp-2].i_i;
        double a1 = FABS(v1);
        double a2 = FABS(v2);
        double a3 = FABS(v3);
        double vd = TRAslopetol*SPMAX(a1, SPMAX(a2, a3)) +
                ckt->CKTcurTask->TSKvoltTol;
        double dti;
        step_compute(ckt, v1, v2, v3, vd, &dti);
        v1 = TRAvalues[tp].v_o + TRAz*TRAvalues[tp].i_o;
        v2 = TRAvalues[tp-1].v_o + TRAz*TRAvalues[tp-1].i_o;
        v3 = TRAvalues[tp-2].v_o + TRAz*TRAvalues[tp-2].i_o;
        a1 = FABS(v1);
        a2 = FABS(v2);
        a3 = FABS(v3);
        vd = TRAslopetol*SPMAX(a1, SPMAX(a2, a3)) +
                ckt->CKTcurTask->TSKvoltTol;
        double dto;
        step_compute(ckt, v1, v2, v3, vd, &dto);
        double dt;
        if (dti > 0.0 && dto > 0.0)
            dt = SPMIN(dti, dto);
        else if (dti > 0.0 || dto > 0.0) {
            dt = dti + dto;
#ifdef TRADEBUG
            if (check_set_min(td, dt, mintd))
                fprintf(stdout, "slope cut %g to %g at %g\n", *td, dt,
                    ckt->CKTtime);
#else
            check_set_min(td, dt, mintd);
#endif
        }
    }

    return (OK);
}

