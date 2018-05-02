
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


int
TRAdev::accept(sCKT *ckt, sGENmodel *genmod)
{
    // Maintain a list of time values, will be used by all instances
    //
    if (!(ckt->CKTmode & MODETRAN))
        return (OK);
    double mintd = 0.0;  // Minimum line delay in circuit.
    sTRAmodel *model = static_cast<sTRAmodel*>(genmod);
    for ( ; model; model = model->next()) {

        for (sTRAconvModel *cv = model->TRAconvModels; cv; cv = cv->next) {
            if (ckt->CKTmode & MODEINITTRAN) {
                delete cv->TRAcvdb;
                cv->TRAcvdb = new timelist<sTRAconval>;
            }
            cv->TRAcvdb->link_new(ckt->CKTtime);
        }

        sTRAinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (inst->TRAtd > 0.0) {
                if (mintd == 0.0 || inst->TRAtd < mintd)
                    mintd = inst->TRAtd;
            }

            int error = inst->accept(ckt);
            if (error)
                return (error);

            if (inst->TRAlevel == PADE_LEVEL && ckt->CKTtimeIndex) {
                TXLine *tx = &inst->TRAtx;
                inst->TRAtx2.copy_to(tx);
                sTRAtimeval *tv = inst->TRAtvdb->head();
                sTRAtimeval *tv_before = tv->prev;
                double delta = tv->time - tv_before->time;

                double v = tv_before->v_i;
                double v1 = tx->Vin = tv->v_i;
                tx->dVin = (v1 - v)/delta;

                v = tv_before->v_o;
                v1 = tx->Vout = tv->v_o;
                tx->dVout = (v1 - v)/delta;

                if (!tx->lsl) {
                    tx->update_cnv(delta);
                    if (tx->ext)
                        tx->update_delayed_cnv(delta, tv);
                }
            }

            error = inst->set_breaks(ckt);
            if (error)
                return (error);
        }
    }

    if (mintd > 0.0) {
        // Include tramsmission line limiting in CKTdevMaxDelta, so as
        // to apply when the "jjaccel" option is in use.

        mintd *= 0.5;
        if (ckt->CKTdevMaxDelta == 0.0 || mintd < ckt->CKTdevMaxDelta)
            ckt->CKTdevMaxDelta = mintd;
    }

    return (OK);
}


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
sTRAinstance::accept(sCKT *ckt)
{
    // Store present values in history list
    //
    if (ckt->CKTmode & MODEINITTRAN) {
        delete TRAtvdb;
        TRAtvdb = new timelist<sTRAtimeval>;
    }
    sTRAtimeval *tv = TRAtvdb->link_new(ckt->CKTtime);
    if (!tv)
        return (E_BADPARM); // can't happen

    tv->v_i = *(ckt->CKTrhsOld + TRAposNode1) - *(ckt->CKTrhsOld + TRAnegNode1);
    tv->v_o = *(ckt->CKTrhsOld + TRAposNode2) - *(ckt->CKTrhsOld + TRAnegNode2);
    tv->i_i = *(ckt->CKTrhsOld + TRAbrEq1);
    tv->i_o = *(ckt->CKTrhsOld + TRAbrEq2);

    if (TRAlevel == CONV_LEVEL) {
        if (TRAcase == TRA_RG)
            return (OK);
    }
    else {
        if (tv->prev)
            TRAtvdb->free_tail(tv->prev->time - TRAtx2.taul);
    }
    return (OK);
}


#define FACTOR 0.5

inline bool
check(double a, double b, double c)
{
    double mx = SPMAX(a, b);
    if (c > mx)
        mx = c;
    double mn = SPMIN(a, b);
    if (c < mn)
        mn = c;
    double av = (a + b + c)/3.0;
    return (mx - mn > FACTOR*fabs(av));
}


int
sTRAinstance::set_breaks(sCKT *ckt)
{
    sTRAtimeval *tp0 = TRAtvdb->head();
    sTRAtimeval *tp1 = tp0->prev;
    if (!tp1)
        return (OK);
    sTRAtimeval *tp2 = tp1->prev;
    if (!tp2)
        return (OK);

    // Breakpoint setting
    if (ckt->CKTbreak) {
        if (TRAbreakType == TRA_ALLBREAKS) {
            int error = ckt->breakSet(tp1->time + TRAtd);
            return (error);
        }
        else if (TRAbreakType == TRA_TESTBREAKS) {
            double v1, v2, v3, d1, d2;
            v1 = tp0->v_i + TRAz*tp0->i_i;
            v2 = tp1->v_i + TRAz*tp1->i_i;
            v3 = tp2->v_i + TRAz*tp2->i_i;
            d1 = (v1-v2)/ckt->CKTdeltaOld[0];
            d2 = (v2-v3)/ckt->CKTdeltaOld[1];
            if (FABS(d1-d2) > FACTOR*SPMAX(FABS(d1), FABS(d2)) &&
                    check(v1, v2, v3)) {
                int error = ckt->breakSet(tp1->time + TRAtd);
                return (error);
            }
            v1 = tp0->v_o + TRAz*tp0->i_o;
            v2 = tp1->v_o + TRAz*tp1->i_o;
            v3 = tp2->v_o + TRAz*tp2->i_o;
            d1 = (v1-v2)/ckt->CKTdeltaOld[0];
            d2 = (v2-v3)/ckt->CKTdeltaOld[1];
            if (FABS(d1-d2) > FACTOR*SPMAX(FABS(d1), FABS(d2)) &&
                    check(v1, v2, v3)) {
                int error = ckt->breakSet(tp1->time + TRAtd);
                return (error);
            }
        }
    }
    return (OK);
}

