
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
    if (ckt->CKTmode & MODEINITTRAN) {
        delete [] ckt->CKTtimePoints;
        ckt->CKTtimeIndex = 0;
        ckt->CKTsizeIncr = 10;
        ckt->CKTtimeListSize =
            (int)(ckt->CKTfinalTime/ckt->CKTmaxStep + 0.5);
        ckt->CKTtimePoints = new double[ckt->CKTtimeListSize];
        *ckt->CKTtimePoints = ckt->CKTtime;
    }
    else if (ckt->CKTmode & MODETRAN) {
        if (ckt->CKTtimePoints) {
            ckt->CKTtimeIndex++;
            if (ckt->CKTtimeIndex >= ckt->CKTtimeListSize) {
                // need more space
                ckt->CKTsizeIncr = 5 + (int)(ckt->CKTtimeIndex*
                    (ckt->CKTfinalTime - ckt->CKTtime)/ckt->CKTtime);
                if (ckt->CKTsizeIncr > 100000)
                    ckt->CKTsizeIncr = 100000;
                ckt->CKTtimeListSize += ckt->CKTsizeIncr;
                Realloc(&ckt->CKTtimePoints, ckt->CKTtimeListSize,
                    ckt->CKTtimeListSize - ckt->CKTsizeIncr);
            }
            *(ckt->CKTtimePoints + ckt->CKTtimeIndex) = ckt->CKTtime;
        }
        else
            return (OK);
    }
    else
        return (OK);

    int compact = 0;
    if (ckt->CKTcurTask->TSKtryToCompact && ckt->CKTtimeIndex >= 2)
        compact = 1;
    double mintd = 0.0;  // Minimum line delay in circuit.
    sTRAmodel *model = static_cast<sTRAmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sTRAinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (inst->TRAtd > 0.0) {
                if (mintd == 0.0 || inst->TRAtd < mintd)
                    mintd = inst->TRAtd;
            }

            int error = inst->accept(ckt, &compact);
            if (error)
                return (error);

            if (inst->TRAlevel == PADE_LEVEL && ckt->CKTtimeIndex) {
                TXLine *tx = &inst->TRAtx;
                inst->TRAtx2.copy_to(tx);
                double delta = ckt->CKTtimePoints[ckt->CKTtimeIndex] -
                    ckt->CKTtimePoints[ckt->CKTtimeIndex - 1];
                sTRAtimeval *tv_before =
                    &inst->TRAvalues[ckt->CKTtimeIndex - 1];

                double v = tv_before->v_i;
                double v1 = tx->Vin = inst->TRAvalues[ckt->CKTtimeIndex].v_i;
                tx->dVin = (v1 - v)/delta;

                v = tv_before->v_o;
                v1 = tx->Vout = inst->TRAvalues[ckt->CKTtimeIndex].v_o;
                tx->dVout = (v1 - v)/delta;

                if (!tx->lsl) {
                    tx->update_cnv(delta);
                    if (tx->ext)
                        tx->update_delayed_cnv(delta,
                            &inst->TRAvalues[ckt->CKTtimeIndex]);
                }
            }

            error = inst->set_breaks(ckt);
            if (error)
                return (error);
        }
    }
    if (compact) {
        // last three timepoints have variables lying on a straight
        // line, do a compaction

        model = static_cast<sTRAmodel*>(genmod);
        for ( ; model; model = model->next()) {
            sTRAinstance *inst;
            for (inst = model->inst(); inst; inst = inst->next()) {
                inst->TRAvalues[ckt->CKTtimeIndex - 1] =
                    inst->TRAvalues[ckt->CKTtimeIndex];
            }
        }
        *(ckt->CKTtimePoints + ckt->CKTtimeIndex - 1) = 
            *(ckt->CKTtimePoints + ckt->CKTtimeIndex);
        ckt->CKTtimeIndex--;

#ifdef TRADEBUG
        fprintf(stdout,"compacted at time=%g\n",
            *(ckt->CKTtimePoints+ckt->CKTtimeIndex));
        fflush(stdout);
#endif
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
sTRAinstance::accept(sCKT *ckt, int *compact)
{
    // Store present values in history list
    //
    if (ckt->CKTmode & MODEINITTRAN) {
        TRAinstListSize = 10;
        delete [] TRAvalues;
        TRAvalues = new sTRAtimeval[TRAinstListSize];
    }
    else if (ckt->CKTtimeIndex >= TRAinstListSize) {
        // need more space
        int prevsize = TRAinstListSize;
        TRAinstListSize += ckt->CKTsizeIncr;
        sTRAtimeval *tv = new sTRAtimeval[TRAinstListSize];
        for (int i = 0; i < prevsize; i++)
            tv[i] = TRAvalues[i];
        delete [] TRAvalues;
        TRAvalues = tv;
    }
    TRAvalues[ckt->CKTtimeIndex].v_i =
        *(ckt->CKTrhsOld + TRAposNode1) - *(ckt->CKTrhsOld + TRAnegNode1);
    TRAvalues[ckt->CKTtimeIndex].v_o =
        *(ckt->CKTrhsOld + TRAposNode2) - *(ckt->CKTrhsOld + TRAnegNode2);
    TRAvalues[ckt->CKTtimeIndex].i_i = *(ckt->CKTrhsOld + TRAbrEq1);
    TRAvalues[ckt->CKTtimeIndex].i_o = *(ckt->CKTrhsOld + TRAbrEq2);

    if (TRAlevel == CONV_LEVEL) {

        sTRAconvModel *model = TRAconvModel;
        if (TRAcase == TRA_RC || TRAcase == TRA_RLC) {
            if (ckt->CKTmode & MODEINITTRAN) {
                model->TRAmodelListSize = 10;
                model->TRAh1dashCoeffs = new double[model->TRAmodelListSize];
                model->TRAh2Coeffs = new double[model->TRAmodelListSize];
                model->TRAh3dashCoeffs = new double[model->TRAmodelListSize];
            }
            else if (ckt->CKTtimeIndex >= model->TRAmodelListSize) {
                // need more space
                int prevsize = model->TRAmodelListSize;
                model->TRAmodelListSize += ckt->CKTsizeIncr;
                Realloc(&model->TRAh1dashCoeffs, model->TRAmodelListSize,
                    prevsize);
                Realloc(&model->TRAh2Coeffs, model->TRAmodelListSize,
                    prevsize);
                Realloc(&model->TRAh3dashCoeffs, model->TRAmodelListSize,
                    prevsize);
            }
        }
        else if (TRAcase == TRA_RG)
            return (OK);

        if (*compact) {
            // figure out if the last 3 points lie on a straight line for
            // all the terminal variables
            //
            double t1 = *(ckt->CKTtimePoints + ckt->CKTtimeIndex - 2);
            double t2 = *(ckt->CKTtimePoints + ckt->CKTtimeIndex - 1);
            double t3 = *(ckt->CKTtimePoints + ckt->CKTtimeIndex);

            *compact = straightLineCheck(
                t1, TRAvalues[ckt->CKTtimeIndex - 2].v_i,
                t2, TRAvalues[ckt->CKTtimeIndex - 1].v_i,
                t3, TRAvalues[ckt->CKTtimeIndex].v_i,
                TRAstLineReltol, TRAstLineAbstol);
            if (*compact) {
                *compact = straightLineCheck(
                    t1, TRAvalues[ckt->CKTtimeIndex - 2].v_o,
                    t2, TRAvalues[ckt->CKTtimeIndex - 1].v_o,
                    t3, TRAvalues[ckt->CKTtimeIndex].v_o,
                    TRAstLineReltol, TRAstLineAbstol);
            }
            if (*compact) {
                *compact = straightLineCheck(
                    t1, TRAvalues[ckt->CKTtimeIndex - 2].i_i,
                    t2, TRAvalues[ckt->CKTtimeIndex - 1].i_i,
                    t3, TRAvalues[ckt->CKTtimeIndex].i_i,
                    TRAstLineReltol, TRAstLineAbstol);
            }
            if (*compact) {
                *compact = straightLineCheck(
                    t1, TRAvalues[ckt->CKTtimeIndex - 2].i_o,
                    t2, TRAvalues[ckt->CKTtimeIndex - 1].i_o,
                    t3, TRAvalues[ckt->CKTtimeIndex].i_o,
                    TRAstLineReltol, TRAstLineAbstol);
            }
        }
    }
    else
        *compact = 0;
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
    int tp = ckt->CKTtimeIndex;
    if (tp < 2)
        return (OK);

    // Breakpoint setting
    if (ckt->CKTbreak) {
        if (TRAbreakType == TRA_ALLBREAKS) {
            int error = ckt->breakSet(ckt->CKTtimePoints[tp-1] + TRAtd);
            return (error);
        }
        else if (TRAbreakType == TRA_TESTBREAKS) {
            double v1, v2, v3, d1, d2;
            v1 = TRAvalues[tp].v_i + TRAz*TRAvalues[tp].i_i;
            v2 = TRAvalues[tp-1].v_i + TRAz*TRAvalues[tp-1].i_i;
            v3 = TRAvalues[tp-2].v_i + TRAz*TRAvalues[tp-2].i_i;
            d1 = (v1-v2)/ckt->CKTdeltaOld[0];
            d2 = (v2-v3)/ckt->CKTdeltaOld[1];
            if (FABS(d1-d2) > FACTOR*SPMAX(FABS(d1), FABS(d2)) &&
                    check(v1, v2, v3)) {
                int error = ckt->breakSet(ckt->CKTtimePoints[tp-1] + TRAtd);
                return (error);
            }
            v1 = TRAvalues[tp].v_o + TRAz*TRAvalues[tp].i_o;
            v2 = TRAvalues[tp-1].v_o + TRAz*TRAvalues[tp-1].i_o;
            v3 = TRAvalues[tp-2].v_o + TRAz*TRAvalues[tp-2].i_o;
            d1 = (v1-v2)/ckt->CKTdeltaOld[0];
            d2 = (v2-v3)/ckt->CKTdeltaOld[1];
            if (FABS(d1-d2) > FACTOR*SPMAX(FABS(d1), FABS(d2)) &&
                    check(v1, v2, v3)) {
                int error = ckt->breakSet(ckt->CKTtimePoints[tp-1] + TRAtd);
                return (error);
            }
        }
    }
    return (OK);
}

