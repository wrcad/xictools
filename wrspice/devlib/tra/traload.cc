
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
TRAdev::load(sGENinstance *in_inst, sCKT *ckt)
{
    sTRAinstance *inst = (sTRAinstance*)in_inst;
    if (inst->TRAlevel == PADE_LEVEL) {
        int error = inst->pade_load(ckt);
        if (error)
            return (error);
    }
    else if (inst->TRAlevel == CONV_LEVEL) {
        int error = inst->ltra_load(ckt);
        if (error)
            return (error);
    }
    else
        return (E_PARMVAL);
    return (OK);
}


//--------------------------------------------------------------------------
//  Pade approximation convolution
//--------------------------------------------------------------------------

int
sTRAinstance::pade_load(sCKT *ckt)
{
    TXLine *tx = &TRAtx;
    if (!tx->lsl && ckt->CKTdelta > tx->taul) {
        DVO.textOut(OUT_FATAL,
            "txl: Your time step is too large for tau.\n"
            "Please decrease max time step in .tran card:\n"
            "  .tran tstep tstop tstart tmax.\n"
            "Make tmax smaller than %e and try again.\n",
            tx->taul);

        return (E_TOOMUCH);
    }

    TXLine *tx2 = &TRAtx2;
    if (ckt->CKTmode & MODEDC) {

//XXX Figure out how this stamp works.
        ckt->ldset(TRApos1Ibr1Ptr, 1.0);
        ckt->ldset(TRAneg1Ibr1Ptr, -1.0);
        ckt->ldset(TRApos2Ibr2Ptr, 1.0);
        ckt->ldset(TRAneg2Ibr2Ptr, -1.0);

        ckt->ldset(TRAibr1Ibr1Ptr, 1.0);
        ckt->ldset(TRAibr1Ibr2Ptr, 1.0);

        ckt->ldset(TRAibr2Pos1Ptr, 1.0);
        ckt->ldset(TRAibr2Pos2Ptr, -1.0);
        ckt->ldset(TRAibr2Neg1Ptr, -1.0);
        ckt->ldset(TRAibr2Neg2Ptr, 1.0);

#ifdef NEWJJDC
        if (ckt->CKTjjDCphase) {
            // If there is no series resistance, treat the inductance
            // as a resistance which takes the voltage difference as a
            // phase.  This applies to DC analysis when Josephson
            // junctions are present (see jjload.cc).

            if (TRAr == 0.0) {
                double res = 2*M_PI*TRAl*TRAlength/wrsCONSTphi0;
                ckt->ldset(TRAibr2Ibr1Ptr, -res);
                ckt->ldset(TRAibr1Ibr2Ptr, -res);
            }
        }
        else {
            ckt->ldset(TRAibr2Ibr1Ptr, -TRAr*TRAlength);
        }
#else
        ckt->ldset(TRAibr2Ibr1Ptr, -TRAr*TRAlength);
#endif
    }
    else {
        double h1 = 0.5*ckt->CKTdelta;
        ckt->ldset(TRAibr1Ibr1Ptr, -1.0);
        ckt->ldset(TRAibr2Ibr2Ptr, -1.0);
        ckt->ldset(TRApos1Ibr1Ptr, 1.0);
        ckt->ldset(TRApos2Ibr2Ptr, 1.0);
        ckt->ldset(TRAneg1Ibr1Ptr, -1.0);
        ckt->ldset(TRAneg2Ibr2Ptr, -1.0);
        double tval = tx->sqtCdL + h1*tx->h1C;
        ckt->ldset(TRAibr1Pos1Ptr, tval);
        ckt->ldset(TRAibr2Pos2Ptr, tval);
        ckt->ldset(TRAibr1Neg1Ptr, -tval);
        ckt->ldset(TRAibr2Neg2Ptr, -tval);

        if (ckt->CKTmode & MODEINITTRAN) {
#ifdef NEWTL
            sTRAtimeval *tv = TRAtvdb->tail();
#else
            sTRAtimeval *tv = &TRAvalues[0];
#endif
            if (ckt->CKTmode & MODEUIC) {
                tx->Vin = TRAinitVolt1;
                tx->Vout = TRAinitVolt2;
                tv->i_i = TRAinitCur1;
                tv->i_o = TRAinitCur2;
            }
            else {
                tx->Vin =
                    ckt->CKTrhsOld[TRAposNode1] - ckt->CKTrhsOld[TRAnegNode1];
                tx->Vout =
                    ckt->CKTrhsOld[TRAposNode2] - ckt->CKTrhsOld[TRAnegNode2];
                tv->i_i = *(ckt->CKTrhsOld + TRAbrEq1);
                tv->i_o = *(ckt->CKTrhsOld + TRAbrEq2);
            }

            tx->dc1 = tx2->dc1 = tx->Vin;
            tx->dc2 = tx2->dc2 = tx->Vout;
            tv->v_i = tx->dc1;
            tv->v_o = tx->dc2;

            if (!tx->lsl) {
                for (int i = 0; i < 3; i++) {
                    tx->h1_term[i].cnv_i = 
                       -tx->dc1*tx->h1_term[i].c/tx->h1_term[i].x;
                    tx->h1_term[i].cnv_o = 
                       -tx->dc2*tx->h1_term[i].c/tx->h1_term[i].x;
                }
                for (int i = 0; i < 3; i++) {
                    tx->h2_term[i].cnv_i = 0.0;
                    tx->h2_term[i].cnv_o = 0.0;
                }
                for (int i = 0; i < 6; i++) {
                    tx->h3_term[i].cnv_i = 
                        -tx->dc1*tx->h3_term[i].c/tx->h3_term[i].x;
                    tx->h3_term[i].cnv_o = 
                        -tx->dc2*tx->h3_term[i].c/tx->h3_term[i].x;
                }
            }
        }
        if (ckt->CKTmode & (MODEINITPRED | MODEINITTRAN)) {
            tx->copy_to(tx2);
            TRAdoload = pade_pred(ckt, ckt->CKTtime - ckt->CKTdelta,
                ckt->CKTtime, ckt->CKTdelta, h1, &tx->ratio);
            if (TRAdoload == 0)
                tx->ext = 0;
            else {
                if (tx->lsl) {
                    TRAtemp1 = tx->ratio*tx->h3_aten;
                    TRAtemp2 = tx->ratio*tx->h2_aten;
                }
                else {
                    tx->ext = 1;
                    if (tx->ratio > 0.0) {
                        TRAtemp1 =
                            tx->ratio*(h1*(tx->h3_term[0].c +
                            tx->h3_term[1].c + tx->h3_term[2].c +
                            tx->h3_term[3].c + tx->h3_term[4].c +
                            tx->h3_term[5].c) + tx->h3_aten);
                        TRAtemp2 =
                            tx->ratio*(h1*(tx->h2_term[0].c +
                            tx->h2_term[1].c + tx->h2_term[2].c) +
                            tx->h2_aten);
                    }
                }
            }
        }
        if (TRAdoload && (tx->lsl || tx->ratio > 0.0)) {
            ckt->ldset(TRAibr1Pos2Ptr, -TRAtemp1);
            ckt->ldset(TRAibr2Pos1Ptr, -TRAtemp1);
            ckt->ldset(TRAibr1Neg2Ptr, TRAtemp1);
            ckt->ldset(TRAibr2Neg1Ptr, TRAtemp1);
            ckt->ldset(TRAibr1Ibr2Ptr, -TRAtemp2);
            ckt->ldset(TRAibr2Ibr1Ptr, -TRAtemp2);
        }
        ckt->rhsadd(TRAbrEq1, TRAinput1);
        ckt->rhsadd(TRAbrEq2, TRAinput2);
    }
    return (OK);
}


inline void
multC(double ar, double ai, double br, double bi, double *cr, double *ci)
{
    double tp = ar*br - ai*bi;
    *ci = ar*bi + ai*br;
    *cr = tp;
}


inline void
expC(double ar, double ai, double h, double *cr, double *ci)
{
    double e = exp(ar*h);
    double cs = cos(ai*h);
    double si = sin(ai*h);
    *cr = e*cs;
    *ci = e*si;
}


int
sTRAinstance::pade_pred(sCKT *ckt, double t, double time, double h,
    double h1, double *ratio)
{
    double ff = 0.0, gg = 0.0;
    if (!TRAtx2.lsl) {
        double ff1 = 0.0;
        for (int i = 0; i < 3; i++) {
            double e = TRAtx2.h1e[i] = exp(TRAtx2.h1_term[i].x * h);
            ff1 -= TRAtx2.h1_term[i].c * e;
            ff  -= TRAtx2.h1_term[i].cnv_i * e;
            gg  -= TRAtx2.h1_term[i].cnv_o * e;
        }
        ff += ff1*h1*TRAtx2.Vin;
        gg += ff1*h1*TRAtx2.Vout;
    }

    // interpolate previous values
    double v1_i, v2_i, i1_i, i2_i;
    double v1_o, v2_o, i1_o, i2_o;
    int ext = 0;
    double ta = t - TRAtx2.taul;
    double tb = time - TRAtx2.taul;
    if (tb <= 0) {
        v1_i = v2_i = TRAtx2.dc1; 
        v2_o = v1_o = TRAtx2.dc2;
        i1_i = i2_i = i1_o = i2_o = 0.0;
    }
    else {
#ifdef NEWTL
        sTRAtimeval *tv;
#else
        int hd;
#endif
        if (ta <= 0) {
            i1_i = i1_o = 0.0; 
            v1_i = TRAtx2.dc1;
            v1_o = TRAtx2.dc2; 
#ifdef NEWTL
            tv = TRAtx2.tv_head;
if (!tv)
    tv = TRAtvdb->tail();
#else
            hd = TRAtx2.tv_head;
#endif
        }
        else {
#ifdef NEWTL
    tv = TRAtvdb->tail();
//            tv = TRAtx2.tv_head->prev;
            while (tv->time < ta)
                tv = tv->next;
if (tv->prev)
            tv = tv->prev;

            double f = (ta - tv->time)/(tv->next->time - tv->time);
            v1_i = tv->v_i + f*(tv->next->v_i - tv->v_i);
            v1_o = tv->v_o + f*(tv->next->v_o - tv->v_o);
            i1_i = tv->i_i + f*(tv->next->i_i - tv->i_i);
            i1_o = tv->i_o + f*(tv->next->i_o - tv->i_o);
#else
            hd = TRAtx2.tv_head + 1;
            for ( ; ckt->CKTtimePoints[hd] < ta; hd++) ;
            hd--;
            sTRAtimeval *tv = &TRAvalues[hd];

            double f = (ta - ckt->CKTtimePoints[hd])/
                (ckt->CKTtimePoints[hd+1] - ckt->CKTtimePoints[hd]);
            v1_i = tv->v_i + f*((tv+1)->v_i - tv->v_i);
            v1_o = tv->v_o + f*((tv+1)->v_o - tv->v_o);
            i1_i = tv->i_i + f*((tv+1)->i_i - tv->i_i);
            i1_o = tv->i_o + f*((tv+1)->i_o - tv->i_o);
#endif
#ifdef NEWTL
            TRAtx2.tv_head = tv;
#else
            TRAtx2.tv_head = hd;
#endif
        }
        if (tb > t) {
            ext = 1;
            double f = (tb - t)/(time - t);
            *ratio = f;
#ifdef NEWTL
            tv = TRAtvdb->head()->prev;
#else
            hd = ckt->CKTtimeIndex - 1;
            sTRAtimeval *tv = &TRAvalues[hd];
#endif
            f = 1 - f;
            v2_i = tv->v_i*f;
            v2_o = tv->v_o*f;
            i2_i = tv->i_i*f;
            i2_o = tv->i_o*f;
        }
        else {
#ifdef NEWTL
            tv = tv->next;
            while (tv->time < tb)
                tv = tv->next;
            tv = tv->prev;

            double f = (tb - tv->time)/(tv->next->time - tv->time);
            v2_i = tv->v_i + f*(tv->next->v_i - tv->v_i);
            v2_o = tv->v_o + f*(tv->next->v_o - tv->v_o);
            i2_i = tv->i_i + f*(tv->next->i_i - tv->i_i);
            i2_o = tv->i_o + f*(tv->next->i_o - tv->i_o);
#else
            hd++;
            for ( ; ckt->CKTtimePoints[hd] < tb; hd++) ;
            hd--;
            sTRAtimeval *tv = &TRAvalues[hd];
          
            double f = (tb - ckt->CKTtimePoints[hd])/
                (ckt->CKTtimePoints[hd+1] - ckt->CKTtimePoints[hd]);
            v2_i = tv->v_i + f*((tv+1)->v_i - tv->v_i);
            v2_o = tv->v_o + f*((tv+1)->v_o - tv->v_o);
            i2_i = tv->i_i + f*((tv+1)->i_i - tv->i_i);
            i2_o = tv->i_o + f*((tv+1)->i_o - tv->i_o);
#endif
        }
    }

    if (TRAtx2.lsl) {
        ff = TRAtx2.h3_aten*v2_o + TRAtx2.h2_aten*i2_o;
        gg = TRAtx2.h3_aten*v2_i + TRAtx2.h2_aten*i2_i;
    }
    else {
        if (TRAtx2.ifImg) {
            for (int i = 0; i < 4; i++) {
                TERM *tm = &(TRAtx2.h3_term[i]);
                double e = exp(tm->x * h);
                tm->cnv_i = tm->cnv_i*e + h1*tm->c*(v1_i*e + v2_i);
                tm->cnv_o = tm->cnv_o*e + h1*tm->c*(v1_o*e + v2_o);
            }
            double er, ei;
            expC(TRAtx2.h3_term[4].x, TRAtx2.h3_term[5].x, h, &er, &ei);
            double a2 = h1*TRAtx2.h3_term[4].c;
            double b2 = h1*TRAtx2.h3_term[5].c;

            double a = TRAtx2.h3_term[4].cnv_i;
            double b = TRAtx2.h3_term[5].cnv_i;
            multC(a, b, er, ei, &a, &b);
            double a1, b1;
            multC(a2, b2, v1_i*er + v2_i, v1_i*ei, &a1, &b1);
            TRAtx2.h3_term[4].cnv_i = a + a1;
            TRAtx2.h3_term[5].cnv_i = b + b1;

            a = TRAtx2.h3_term[4].cnv_o;
            b = TRAtx2.h3_term[5].cnv_o;
            multC(a, b, er, ei, &a, &b);
            multC(a2, b2, v1_o*er + v2_o, v1_o*ei, &a1, &b1);
            TRAtx2.h3_term[4].cnv_o = a + a1;
            TRAtx2.h3_term[5].cnv_o = b + b1;

            ff += TRAtx2.h3_aten*v2_o;
            gg += TRAtx2.h3_aten*v2_i;

            for (int i = 0; i < 5; i++) {
                ff += TRAtx2.h3_term[i].cnv_o;
                gg += TRAtx2.h3_term[i].cnv_i;
            }
            ff += TRAtx2.h3_term[4].cnv_o;
            gg += TRAtx2.h3_term[4].cnv_i;

            TERM *tm = &(TRAtx2.h2_term[0]);
            double e =  exp(tm->x*h);
            tm->cnv_i = tm->cnv_i*e + h1*tm->c*(i1_i*e + i2_i);
            tm->cnv_o = tm->cnv_o*e + h1*tm->c*(i1_o*e + i2_o);

            expC(TRAtx2.h2_term[1].x, TRAtx2.h2_term[2].x, h, &er, &ei);
            a2 = h1*TRAtx2.h2_term[1].c;
            b2 = h1*TRAtx2.h2_term[2].c;

            a = TRAtx2.h2_term[1].cnv_i;
            b = TRAtx2.h2_term[2].cnv_i;
            multC(a, b, er, ei, &a, &b);
            multC(a2, b2, i1_i*er + i2_i, i1_i*ei, &a1, &b1);
            TRAtx2.h2_term[1].cnv_i = a + a1;
            TRAtx2.h2_term[2].cnv_i = b + b1;

            a = TRAtx2.h2_term[1].cnv_o;
            b = TRAtx2.h2_term[2].cnv_o;
            multC(a, b, er, ei, &a, &b);
            multC(a2, b2, i1_o*er + i2_o, i1_o*ei, &a1, &b1);
            TRAtx2.h2_term[1].cnv_o = a + a1;
            TRAtx2.h2_term[2].cnv_o = b + b1;

            ff += TRAtx2.h2_aten*i2_o + TRAtx2.h2_term[0].cnv_o +
                2.0*TRAtx2.h2_term[1].cnv_o;
            gg += TRAtx2.h2_aten*i2_i + TRAtx2.h2_term[0].cnv_i +
                2.0*TRAtx2.h2_term[1].cnv_i;
        }
        else {
            for (int i = 0; i < 6; i++) {
                TERM *tm = &(TRAtx2.h3_term[i]);
                double e = exp(tm->x * h);
                tm->cnv_i = tm->cnv_i*e + h1*tm->c*(v1_i*e + v2_i);
                tm->cnv_o = tm->cnv_o*e + h1*tm->c*(v1_o*e + v2_o);
            }

            ff += TRAtx2.h3_aten*v2_o;
            gg += TRAtx2.h3_aten*v2_i;
            for (int i = 0; i < 6; i++) {
                ff += TRAtx2.h3_term[i].cnv_o;
                gg += TRAtx2.h3_term[i].cnv_i;
            }

            for (int i = 0; i < 3; i++) {
                TERM *tm = &(TRAtx2.h2_term[i]);
                double e = exp(tm->x * h);
                tm->cnv_i = tm->cnv_i*e + h1*tm->c*(i1_i*e + i2_i);
                tm->cnv_o = tm->cnv_o*e + h1*tm->c*(i1_o*e + i2_o);
            }

            ff += TRAtx2.h2_aten*i2_o;
            gg += TRAtx2.h2_aten*i2_i;
            for (int i = 0; i < 3; i++) {
                ff += TRAtx2.h2_term[i].cnv_o;
                gg += TRAtx2.h2_term[i].cnv_i;
            }
        }
    }
    TRAinput1 = ff;
    TRAinput2 = gg;
    return (ext);
}


void
TXLine::copy_to(TXLine *newone)
{
    newone->lsl = lsl;
    newone->ext = ext;
    newone->ratio = ratio;
    newone->taul = taul;
    newone->sqtCdL = sqtCdL;
    newone->h2_aten = h2_aten;
    newone->h3_aten = h3_aten;
    newone->h1C = h1C;
    newone->Vin = Vin;
    newone->dVin = dVin;
    newone->Vout = Vout;
    newone->dVout = dVout;
    for (int i = 0; i < 3; i++) {
        newone->h1e[i] = h1e[i];
        newone->h1_term[i] = h1_term[i];
        newone->h2_term[i] = h2_term[i];
    }
    for (int i = 0; i < 6; i++)
        newone->h3_term[i] = h3_term[i];
    newone->ifImg = ifImg;
    while (newone->tv_head < tv_head)
        newone->tv_head++;
}


void
TXLine::update_cnv(double h)
{
    double ai = Vin;
    double ao = Vout;
    double bi = dVin;
    double bo = dVout;

    for (int i = 0; i < 3; i++) {
        TERM *tm = &h1_term[i];
        double e = h1e[i];
        double t = tm->c/tm->x;
        bi *= t;
        bo *= t;
        tm->cnv_i = (tm->cnv_i - bi*h)*e + (e - 1.0)*(ai*t + bi/tm->x);
        tm->cnv_o = (tm->cnv_o - bo*h)*e + (e - 1.0)*(ao*t + bo/tm->x);
    }
}


void 
TXLine::update_delayed_cnv(double h, sTRAtimeval *tv)
{
    h *= 0.5;
    if (ratio > 0.0) {
        TERM *tms = h3_term;
        double f = h*ratio*tv->v_i;
        tms[0].cnv_i += f*tms[0].c;
        tms[1].cnv_i += f*tms[1].c;
        tms[2].cnv_i += f*tms[2].c;
        tms[3].cnv_i += f*tms[3].c;
        tms[4].cnv_i += f*tms[4].c;
        tms[5].cnv_i += f*tms[5].c;

        f = h*ratio*tv->v_o;
        tms[0].cnv_o += f*tms[0].c;
        tms[1].cnv_o += f*tms[1].c;
        tms[2].cnv_o += f*tms[2].c;
        tms[3].cnv_o += f*tms[3].c;
        tms[4].cnv_o += f*tms[4].c;
        tms[5].cnv_o += f*tms[5].c;

        tms = h2_term;
        f = h*ratio*tv->i_i;
        tms[0].cnv_i += f*tms[0].c;
        tms[1].cnv_i += f*tms[1].c;
        tms[2].cnv_i += f*tms[2].c;

        f = h*ratio*tv->i_o;
        tms[0].cnv_o += f*tms[0].c;
        tms[1].cnv_o += f*tms[1].c;
        tms[2].cnv_o += f*tms[2].c;
    }
}


//--------------------------------------------------------------------------
//  Full convolution - SPICE3 LTRA
//--------------------------------------------------------------------------

namespace TRA {
    struct ltrastuff
    {
        void ltra_interp(double*, double*, double*, double*, sTRAinstance*);
        int ltra_lin_interp(double);
        int ltra_quad_interp(double);

        double ls_t1, ls_t2, ls_t3;
        double ls_qf1, ls_qf2, ls_qf3;
        double ls_lf2, ls_lf3;
#ifdef NEWTL
        sTRAtimeval *ls_saved;
        int ls_over;
#else
        int ls_saved, ls_over;
#endif
        int ls_qinterp;
    };
}


int
sTRAinstance::ltra_load(sCKT *ckt)
{
    struct ltrastuff ls;
    if (ckt->CKTmode & MODEDC) {
        if (TRAcase == TRA_RG) {
            double dummy1 = TRAlength*sqrt(TRAr*TRAg);
            double dummy2 = exp(-dummy1);
            dummy1 = exp(dummy1);
            TRAconvModel->TRAcoshlrootGR = 0.5*(dummy1 + dummy2);

            if (TRAg <= 1.0e-10)  // hack!
                TRAconvModel->TRArRsLrGRorG = TRAlength*TRAr;
            else
                TRAconvModel->TRArRsLrGRorG =
                    0.5*(dummy1 - dummy2)*sqrt(TRAr/TRAg);

            if (TRAr <= 1.0e-10)  // hack!
                TRAconvModel->TRArGsLrGRorR = TRAlength*TRAg;
            else
                TRAconvModel->TRArGsLrGRorR =
                    0.5*(dummy1 - dummy2)*sqrt(TRAg/TRAr);
        }
        else if (TRAcase != TRA_RC && TRAcase != TRA_LC && TRAcase != TRA_RLC)
            return (E_BADPARM);
    }
    else if (ckt->CKTmode & (MODEINITTRAN | MODEINITPRED)) {

        if (TRAcase == TRA_LC || TRAcase == TRA_RLC) {
            if (TRAcase == TRA_RLC) {
                // set up lists of values of the functions at the
                // necessary timepoints. 
                TRAconvModel->rlcCoeffsSetup(ckt);
            }

            // setting up the coefficients for interpolation
            if (ckt->CKTtime > TRAtd) {
                ls.ls_over = 1;

                // serious hack - fix!
                double dummy1 = ckt->CKTtime - TRAtd; 
#ifdef NEWTL
                sTRAtimeval *tv = TRAtvdb->head();
                for ( ; tv; tv = tv->prev) {
                    if (tv->time < dummy1)
                        break;
                }
                if (!tv->next)
                    tv = tv->prev;
                ls.ls_saved = tv;

                ls.ls_t2 = tv->time;
                ls.ls_t3 = tv->next->time;
                // linear interpolation
                ls.ltra_lin_interp(dummy1);

                ls.ls_qinterp = (TRAhowToInterp == TRA_QUADINTERP);
                if (tv->prev && ls.ls_qinterp) {
                    // quadratic interpolation
                    ls.ls_t1 = tv->prev->time;
                    ls.ltra_quad_interp(dummy1);
                }
#else
                int i;
                for (i = ckt->CKTtimeIndex; i >= 0; i--) {
                    if (*(ckt->CKTtimePoints + i) < dummy1)
                        break;
                }

                if (i == ckt->CKTtimeIndex)
                    i--;
                ls.ls_saved = i;

                ls.ls_t2 = *(ckt->CKTtimePoints + i);
                ls.ls_t3 = *(ckt->CKTtimePoints + i + 1);
                // linear interpolation
                ls.ltra_lin_interp(dummy1);

                ls.ls_qinterp = (TRAhowToInterp == TRA_QUADINTERP);
                if ((i != 0) && ls.ls_qinterp) {
                    // quadratic interpolation
                    ls.ls_t1 = *(ckt->CKTtimePoints + i - 1);
                    ls.ltra_quad_interp(dummy1);
                }
#endif
                // interpolation coefficients set-up
            }
            else
                ls.ls_over = 0;
        }
        else if (TRAcase == TRA_RC) {
            // set up lists of values of the coefficients at the
            // necessary timepoints. 
            TRAconvModel->rcCoeffsSetup(ckt);
        }
        else if (TRAcase != TRA_RG)
            return (E_BADPARM);
    }

    if ((ckt->CKTmode & MODEDC) || TRAcase == TRA_RG) {

        if (TRAcase == TRA_RG) {
            ckt->ldset(TRAibr1Pos1Ptr, 1.0);
            ckt->ldset(TRAibr1Neg1Ptr, -1.0);
            ckt->ldset(TRAibr1Pos2Ptr, -TRAconvModel->TRAcoshlrootGR);
            ckt->ldset(TRAibr1Neg2Ptr, TRAconvModel->TRAcoshlrootGR);
            ckt->ldset(TRAibr1Ibr2Ptr,
                (1+ckt->CKTcurTask->TSKgmin)*TRAconvModel->TRArRsLrGRorG);

            ckt->ldset(TRAibr2Ibr2Ptr, TRAconvModel->TRAcoshlrootGR);
            double tval =
                (1+ckt->CKTcurTask->TSKgmin)*TRAconvModel->TRArGsLrGRorR;
            ckt->ldset(TRAibr2Pos2Ptr, -tval);
            ckt->ldset(TRAibr2Neg2Ptr, tval);
            ckt->ldset(TRAibr2Ibr1Ptr, 1.0);

            ckt->ldset(TRApos1Ibr1Ptr, 1.0);
            ckt->ldset(TRAneg1Ibr1Ptr, -1.0);
            ckt->ldset(TRApos2Ibr2Ptr, 1.0);
            ckt->ldset(TRAneg2Ibr2Ptr, -1.0);
        }
        else if (TRAcase == TRA_LC || TRAcase == TRA_RLC ||
                TRAcase == TRA_RC) {
            // load a simple resistor
            ckt->ldset(TRApos1Ibr1Ptr, 1.0);
            ckt->ldset(TRAneg1Ibr1Ptr, -1.0);
            ckt->ldset(TRApos2Ibr2Ptr, 1.0);
            ckt->ldset(TRAneg2Ibr2Ptr, -1.0);

            ckt->ldset(TRAibr1Ibr1Ptr, 1.0);
            ckt->ldset(TRAibr1Ibr2Ptr, 1.0);
            ckt->ldset(TRAibr2Pos1Ptr, 1.0);
            ckt->ldset(TRAibr2Pos2Ptr, -1.0);
            ckt->ldset(TRAibr2Ibr1Ptr, -TRAr*TRAlength);
        }
        else
            return (E_BADPARM);
        return (OK);;
    }

    // all cases other than DC or the RG case

    // first timepoint after zero
    if (ckt->CKTmode & MODEINITTRAN) {
        if (!(ckt->CKTmode & MODEUIC)) {

            TRAinitVolt1 = 
                *(ckt->CKTrhsOld + TRAposNode1) -
                *(ckt->CKTrhsOld + TRAnegNode1);
            TRAinitVolt2 = 
                *(ckt->CKTrhsOld + TRAposNode2) -
                *(ckt->CKTrhsOld + TRAnegNode2);
            TRAinitCur1 = *(ckt->CKTrhsOld + TRAbrEq1);
            TRAinitCur2 = *(ckt->CKTrhsOld + TRAbrEq2);
        }
    }

    // matrix loading
    double dummy1;
    switch (TRAcase) {
    case TRA_RLC:
        // loading for convolution parts' first terms.

        dummy1 = TRAconvModel->TRAadmit*TRAconvModel->TRAh1dashFirstCoeff;
        ckt->ldset(TRAibr1Pos1Ptr, dummy1);
        ckt->ldset(TRAibr1Neg1Ptr, -dummy1);
        ckt->ldset(TRAibr2Pos2Ptr, dummy1);
        ckt->ldset(TRAibr2Neg2Ptr, -dummy1);
        // fall through

    case TRA_LC:
        // this section loads for the parts of the equations that
        // resemble the lossless equations.

        ckt->ldset(TRAibr1Pos1Ptr, TRAconvModel->TRAadmit);
        ckt->ldset(TRAibr1Neg1Ptr, -TRAconvModel->TRAadmit);
        ckt->ldset(TRAibr1Ibr1Ptr, -1.0);
        ckt->ldset(TRApos1Ibr1Ptr, 1.0);
        ckt->ldset(TRAneg1Ibr1Ptr, -1.0);

        ckt->ldset(TRAibr2Pos2Ptr, TRAconvModel->TRAadmit);
        ckt->ldset(TRAibr2Neg2Ptr, -TRAconvModel->TRAadmit);
        ckt->ldset(TRAibr2Ibr2Ptr, -1.0);
        ckt->ldset(TRApos2Ibr2Ptr, 1.0);
        ckt->ldset(TRAneg2Ibr2Ptr, -1.0);
        break;

    case TRA_RC:
        // this section loads for the parts of the equations that have
        // no convolution.

        ckt->ldset(TRAibr1Ibr1Ptr, -1.0);
        ckt->ldset(TRApos1Ibr1Ptr, 1.0);
        ckt->ldset(TRAneg1Ibr1Ptr, -1.0);

        ckt->ldset(TRAibr2Ibr2Ptr, -1.0);
        ckt->ldset(TRApos2Ibr2Ptr, 1.0);
        ckt->ldset(TRAneg2Ibr2Ptr, -1.0);

        // loading for convolution parts' first terms
        dummy1 = TRAconvModel->TRAh1dashFirstCoeff;
        ckt->ldset(TRAibr1Pos1Ptr, dummy1);
        ckt->ldset(TRAibr1Neg1Ptr, -dummy1);
        ckt->ldset(TRAibr2Pos2Ptr, dummy1);
        ckt->ldset(TRAibr2Neg2Ptr, -dummy1);

        dummy1 = TRAconvModel->TRAh2FirstCoeff;
        ckt->ldset(TRAibr1Ibr2Ptr, -dummy1);
        ckt->ldset(TRAibr2Ibr1Ptr, -dummy1);

        dummy1 = TRAconvModel->TRAh3dashFirstCoeff;
        ckt->ldset(TRAibr1Pos2Ptr, -dummy1);
        ckt->ldset(TRAibr1Neg2Ptr, dummy1);
        ckt->ldset(TRAibr2Pos1Ptr, -dummy1);
        ckt->ldset(TRAibr2Neg1Ptr, dummy1);
        break;

    default:
        return (E_BADPARM);
    }

    // Set up TRAinputs - to go into the RHS of the circuit equations.

    if (ckt->CKTmode & (MODEINITPRED | MODEINITTRAN)) {

        // first iteration of each timepoint
        int i = ltra_pred(ckt, &ls);
        if (i)
            return (i);
    }

    // load the RHS
    ckt->rhsadd(TRAbrEq1, TRAinput1);
    ckt->rhsadd(TRAbrEq2, TRAinput2);
    return (OK);
}


int
sTRAinstance::ltra_pred(sCKT *ckt, ltrastuff *ls)
{
    sTRAconvModel *model = TRAconvModel;
    TRAinput1 = TRAinput2 = 0.0;

    if (TRAcase == TRA_LC || TRAcase == TRA_RLC) {

        double v1d, v2d, i1d, i2d;
        ls->ltra_interp(&v1d, &v2d, &i1d, &i2d, this);

        if (TRAcase == TRA_RLC) {

            // begin convolution parts
            //
            // the matrix has already been loaded above
            //
            // convolution of h1dash with v1 and v2
            double dummy1 = 0.0;
            double dummy2 = 0.0;
#ifdef NEWTL
            sTRAconval *cv = model->TRAcvdb->head();
            sTRAtimeval *tv = TRAtvdb->head();
            for ( ; cv && tv; cv = cv->prev, tv = tv->prev) {
                if (cv->h1dashCoeff != 0.0) {
                    dummy1 += cv->h1dashCoeff * (tv->v_i - TRAinitVolt1);
                    dummy2 += cv->h1dashCoeff * (tv->v_o - TRAinitVolt2);
                }
            }
           
#else
            for (int i = ckt->CKTtimeIndex; i > 0; i--) {
                if (*(model->TRAh1dashCoeffs + i) != 0.0) {
                    dummy1 += *(model->TRAh1dashCoeffs + i)*
                        (TRAvalues[i].v_i - TRAinitVolt1);
                    dummy2 += *(model->TRAh1dashCoeffs + i)*
                        (TRAvalues[i].v_o - TRAinitVolt2);
                }
            }
#endif

            // the initial-condition terms
            dummy1 += TRAinitVolt1*model->TRAintH1dash;
            dummy2 += TRAinitVolt2*model->TRAintH1dash;
            dummy1 -= TRAinitVolt1*model->TRAh1dashFirstCoeff;
            dummy2 -= TRAinitVolt2*model->TRAh1dashFirstCoeff;

            TRAinput1 -= dummy1*model->TRAadmit;
            TRAinput2 -= dummy2*model->TRAadmit;
            // end convolution of h1dash with v1 and v2

            // convolution of h2 with i2 and i1
            dummy1 = 0.0;
            dummy2 = 0.0;
            if (ls->ls_over) {
                // have to interpolate values
                dummy1 = (i2d - TRAinitCur2)*model->TRAh2FirstCoeff;
                dummy2 = (i1d - TRAinitCur1)*model->TRAh2FirstCoeff;
    
                // the rest of the convolution
#ifdef NEWTL
                double tt = ckt->CKTtime - TRAtd;
                tv = TRAtvdb->tail();
                cv = model->TRAcvdb->tail();
                for ( ; cv->next; cv = cv->next, tv = tv->next) {
                    if (cv->next->time > tt)
                        break;
                }
                sTRAconval *cvbk = cv;
                sTRAtimeval *tvbk = tv;

                for ( ; cv && tv; cv = cv->prev, tv = tv->prev) {
                    if (cv->h2Coeff != 0.0) {
                        dummy1 += cv->h2Coeff * (tv->i_o - TRAinitCur2);
                        dummy2 += cv->h2Coeff * (tv->i_i - TRAinitCur1);
                    }
                }
                cv = cvbk;
                tv = tvbk;
#else
                for (int i = model->TRAauxIndex; i > 0; i--) {
                    if (*(model->TRAh2Coeffs + i) != 0.0) {
                        dummy1 += *(model->TRAh2Coeffs + i)*
                            (TRAvalues[i].i_o - TRAinitCur2);
                        dummy2 += *(model->TRAh2Coeffs + i)*
                            (TRAvalues[i].i_i - TRAinitCur1);
                    }
                }
#endif
            }

            // the initial-condition terms
            dummy1 += TRAinitCur2*model->TRAintH2;
            dummy2 += TRAinitCur1*model->TRAintH2;
    
            TRAinput1 += dummy1;
            TRAinput2 += dummy2;
            // end convolution of h2 with i2 and i1

            // convolution of h3dash with v2 and v1
            dummy1 = 0.0;
            dummy2 = 0.0;
            if (ls->ls_over) {
                // have to interpolate values
                dummy1 = (v2d - TRAinitVolt2)*model->TRAh3dashFirstCoeff;
                dummy2 = (v1d - TRAinitVolt1)*model->TRAh3dashFirstCoeff;
    
                // the rest of the convolution
#ifdef NEWTL
                for ( ; cv && tv; cv = cv->prev, tv = tv->prev) {
                    if (cv->h3dashCoeff  != 0.0) {
                        dummy1 += cv->h3dashCoeff * (tv->v_o - TRAinitVolt2);
                        dummy2 += cv->h3dashCoeff * (tv->v_i - TRAinitVolt1);
                    }
                }
#else
                for (int i = model->TRAauxIndex; i > 0; i--) {
                    if (*(model->TRAh3dashCoeffs + i) != 0.0) {
                        dummy1 += *(model->TRAh3dashCoeffs + i)*
                            (TRAvalues[i].v_o - TRAinitVolt2);
                        dummy2 += *(model->TRAh3dashCoeffs + i)*
                            (TRAvalues[i].v_i - TRAinitVolt1);
                    }
                }
#endif
            }

            // the initial-condition terms
            dummy1 += TRAinitVolt2*model->TRAintH3dash;
            dummy2 += TRAinitVolt1*model->TRAintH3dash;
    
            TRAinput1 += model->TRAadmit*dummy1;
            TRAinput2 += model->TRAadmit*dummy2;
            // end convolution of h3dash with v2 and v1
        }
        // begin lossless-like parts

        if (!ls->ls_over) {
            TRAinput1 += model->TRAattenuation*
                (TRAinitVolt2*model->TRAadmit + TRAinitCur2);
            TRAinput2 += model->TRAattenuation*
                (TRAinitVolt1*model->TRAadmit + TRAinitCur1);
        }
        else {
            // have to interpolate values
            TRAinput1 += model->TRAattenuation*(v2d*model->TRAadmit + i2d);
            TRAinput2 += model->TRAattenuation*(v1d*model->TRAadmit + i1d);
        }
        // end lossless-like parts
    }
    else if (TRAcase == TRA_RC) {

        // begin convolution parts
        //
        // the matrix has already been loaded above
        //
        // convolution of h1dash with v1 and v2
        double dummy1 = 0.0;
        double dummy2 = 0.0;
#ifdef NEWTL
        sTRAconval *cv = model->TRAcvdb->head();
        sTRAtimeval *tv = TRAtvdb->head();
        for ( ; cv && tv; cv = cv->prev, tv = tv->prev) {
            if (cv->h1dashCoeff  != 0.0) {
                dummy1 += cv->h1dashCoeff * (tv->v_i - TRAinitVolt1);
                dummy2 += cv->h1dashCoeff * (tv->v_o - TRAinitVolt2);
            }
        }
#else
        for (int i = ckt->CKTtimeIndex; i > 0; i--) {
            if (*(model->TRAh1dashCoeffs + i) != 0.0) {
                dummy1 += *(model->TRAh1dashCoeffs + i)*
                    (TRAvalues[i].v_i - TRAinitVolt1);
                dummy2 += *(model->TRAh1dashCoeffs + i)*
                    (TRAvalues[i].v_o - TRAinitVolt2);
            }
        }
#endif

        // the initial condition terms
        dummy1 += TRAinitVolt1*model->TRAintH1dash;
        dummy2 += TRAinitVolt2*model->TRAintH1dash;
        dummy1 -= TRAinitVolt1*model->TRAh1dashFirstCoeff;
        dummy2 -= TRAinitVolt2*model->TRAh1dashFirstCoeff;

        TRAinput1 -= dummy1;
        TRAinput2 -= dummy2;
        // end convolution of h1dash with v1 and v2

        // convolution of h2 with i2 and i1
        dummy1 = 0.0;
        dummy2 = 0.0;
#ifdef NEWTL
        cv = model->TRAcvdb->head();
        tv = TRAtvdb->head();
        for ( ; cv && tv; cv = cv->prev, tv = tv->prev) {
            if (cv->h2Coeff != 0.0) {
                dummy1 += cv->h2Coeff * (tv->i_o - TRAinitCur2);
                dummy2 += cv->h2Coeff * (tv->i_i - TRAinitCur1);
            }
        }
#else
        for (int i = ckt->CKTtimeIndex; i > 0; i--) {
            if (*(model->TRAh2Coeffs+ i) != 0.0) {
                dummy1 += *(model->TRAh2Coeffs + i)*
                    (TRAvalues[i].i_o - TRAinitCur2);
                dummy2 += *(model->TRAh2Coeffs + i)*
                    (TRAvalues[i].i_i - TRAinitCur1);
            }
        }
#endif

        // the initial-condition terms
        dummy1 += TRAinitCur2*model->TRAintH2;
        dummy2 += TRAinitCur1*model->TRAintH2;
        dummy1 -= TRAinitCur2*model->TRAh2FirstCoeff;
        dummy2 -= TRAinitCur1*model->TRAh2FirstCoeff;

        TRAinput1 += dummy1;
        TRAinput2 += dummy2;
        // end convolution of h2 with i2 and i1

        // convolution of h3dash with v2 and v1
        dummy1 = 0.0;
        dummy2 = 0.0;
#ifdef NEWTL
        cv = model->TRAcvdb->head();
        tv = TRAtvdb->head();
        for ( ; cv && tv; cv = cv->prev, tv = tv->prev) {
            if (cv->h3dashCoeff != 0.0) {
                dummy1 += cv->h3dashCoeff * (tv->v_o - TRAinitVolt2);
                dummy2 += cv->h3dashCoeff * (tv->v_i - TRAinitVolt1);
            }
        }
#else
        for (int i = ckt->CKTtimeIndex; i > 0; i--) {
            if (*(model->TRAh3dashCoeffs+ i) != 0.0) {
                dummy1 += *(model->TRAh3dashCoeffs + i)*
                    (TRAvalues[i].v_o - TRAinitVolt2);
                dummy2 += *(model->TRAh3dashCoeffs + i)*
                    (TRAvalues[i].v_i - TRAinitVolt1);
            }
        }
#endif

        // the initial-condition terms
        dummy1 += TRAinitVolt2*model->TRAintH3dash;
        dummy2 += TRAinitVolt1*model->TRAintH3dash;
        dummy1 -= TRAinitVolt2*model->TRAh3dashFirstCoeff;
        dummy2 -= TRAinitVolt1*model->TRAh3dashFirstCoeff;

        TRAinput1 += dummy1;
        TRAinput2 += dummy2;
        // end convolution of h3dash with v2 and v1
    }
    else
        return (E_BADPARM);
    return (OK);
}


void
ltrastuff::ltra_interp(double *v1, double *v2, double *i1, double *i2,
    sTRAinstance *inst)
{
    if (!ls_over) {
        *v1 = 0.0;
        *v2 = 0.0;
        *i1 = 0.0;
        *i2 = 0.0;
        return;
    }
#ifdef NEWTL
    sTRAtimeval *tv = ls_saved;
    if (tv->prev && ls_qinterp) {
        double max, min;

        *v1 = tv->prev->v_i*ls_qf1 + tv->v_i*ls_qf2 + tv->next->v_i*ls_qf3;
        max = SPMAX(tv->prev->v_i, SPMAX(tv->v_i, tv->next->v_i));
        min = SPMIN(tv->prev->v_i, SPMIN(tv->v_i, tv->next->v_i));
        if (*v1 < min || *v1 > max)
            *v1 = tv->v_i*ls_lf2 + tv->next->v_i*ls_lf3;

        *v2 = tv->prev->v_o*ls_qf1 + tv->v_o*ls_qf2 + tv->next->v_o*ls_qf3;
        max = SPMAX(tv->prev->v_o, SPMAX(tv->v_o, tv->next->v_o));
        min = SPMIN(tv->prev->v_o, SPMIN(tv->v_o, tv->next->v_o));
        if (*v2 < min || *v2 > max)
            *v2 = tv->v_o*ls_lf2 + tv->next->v_o*ls_lf3;

        *i1 = tv->prev->i_i*ls_qf1 + tv->i_i*ls_qf2 + tv->next->i_i*ls_qf3;
        max = SPMAX(tv->prev->i_i, SPMAX(tv->i_i, tv->next->i_i));
        min = SPMIN(tv->prev->i_i, SPMIN(tv->i_i, tv->next->i_i));
        if (*i1 < min || *i1 > max)
            *i1 = tv->i_i*ls_lf2 + tv->next->i_i*ls_lf3;

        *i2 = tv->prev->i_o*ls_qf1 + tv->i_o*ls_qf2 + tv->next->i_o*ls_qf3;
        max = SPMAX(tv->prev->i_o, SPMAX(tv->i_o, tv->next->i_o));
        min = SPMIN(tv->prev->i_o, SPMIN(tv->i_o, tv->next->i_o));
        if (*i2 < min || *v2 > max)
            *i2 = tv->i_o*ls_lf2 + tv->next->i_o*ls_lf3;
        return;
    }
    *v1 = tv->v_i*ls_lf2 + tv->next->v_i*ls_lf3;
    *v2 = tv->v_o*ls_lf2 + tv->next->v_o*ls_lf3;
    *i1 = tv->i_i*ls_lf2 + tv->next->i_i*ls_lf3;
    *i2 = tv->i_o*ls_lf2 + tv->next->i_o*ls_lf3;
#else
    sTRAtimeval *tv = &inst->TRAvalues[ls_saved];
    if (ls_saved && ls_qinterp) {
        double max, min;

        *v1 = (tv-1)->v_i*ls_qf1 + tv->v_i*ls_qf2 + (tv+1)->v_i*ls_qf3;
        max = SPMAX((tv-1)->v_i, SPMAX(tv->v_i, (tv+1)->v_i));
        min = SPMIN((tv-1)->v_i, SPMIN(tv->v_i, (tv+1)->v_i));
        if (*v1 < min || *v1 > max)
            *v1 = tv->v_i*ls_lf2 + (tv+1)->v_i*ls_lf3;

        *v2 = (tv-1)->v_o*ls_qf1 + tv->v_o*ls_qf2 + (tv+1)->v_o*ls_qf3;
        max = SPMAX((tv-1)->v_o, SPMAX(tv->v_o, (tv+1)->v_o));
        min = SPMIN((tv-1)->v_o, SPMIN(tv->v_o, (tv+1)->v_o));
        if (*v2 < min || *v2 > max)
            *v2 = tv->v_o*ls_lf2 + (tv+1)->v_o*ls_lf3;

        *i1 = (tv-1)->i_i*ls_qf1 + tv->i_i*ls_qf2 + (tv+1)->i_i*ls_qf3;
        max = SPMAX((tv-1)->i_i, SPMAX(tv->i_i, (tv+1)->i_i));
        min = SPMIN((tv-1)->i_i, SPMIN(tv->i_i, (tv+1)->i_i));
        if (*i1 < min || *i1 > max)
            *i1 = tv->i_i*ls_lf2 + (tv+1)->i_i*ls_lf3;

        *i2 = (tv-1)->i_o*ls_qf1 + tv->i_o*ls_qf2 + (tv+1)->i_o*ls_qf3;
        max = SPMAX((tv-1)->i_o, SPMAX(tv->i_o, (tv+1)->i_o));
        min = SPMIN((tv-1)->i_o, SPMIN(tv->i_o, (tv+1)->i_o));
        if (*i2 < min || *v2 > max)
            *i2 = tv->i_o*ls_lf2 + (tv+1)->i_o*ls_lf3;
        return;
    }
    *v1 = tv->v_i*ls_lf2 + (tv+1)->v_i*ls_lf3;
    *v2 = tv->v_o*ls_lf2 + (tv+1)->v_o*ls_lf3;
    *i1 = tv->i_i*ls_lf2 + (tv+1)->i_i*ls_lf3;
    *i2 = tv->i_o*ls_lf2 + (tv+1)->i_o*ls_lf3;
#endif
}


// linear interpolation
//
int
ltrastuff::ltra_lin_interp(double t)
{
    if (ls_t2 == ls_t3)
        return (1);

    if (t == ls_t2) {
        ls_lf2 = 1.0;
        ls_lf3 = 0.0;
        return (0);
    }

    if (t == ls_t3) {
        ls_lf2 = 0.0;
        ls_lf3 = 1.0;
        return (0);
    }

    ls_lf3 = (t - ls_t2)/(ls_t3 - ls_t2);
    ls_lf2 = 1 - ls_lf3;
    return (0);
}


// quadratic interpolation
//
int
ltrastuff::ltra_quad_interp(double t)
{
    if (t == ls_t1) {
        ls_qf1 = 1.0;
        ls_qf2 = 0.0;
        ls_qf3 = 0.0;
        return (0);
    }
    if (t == ls_t2) {
        ls_qf1 = 0.0;
        ls_qf2 = 1.0;
        ls_qf3 = 0.0;
        return (0);
    }
    if (t == ls_t3) {
        ls_qf1 = 0.0;
        ls_qf2 = 0.0;
        ls_qf3 = 1.0;
        return (0);
    }
    if (ls_t2 - ls_t1 == 0 || ls_t3 - ls_t2 == 0 || ls_t1 - ls_t3 == 0)
        return(1);

    ls_qf1 = (t - ls_t2)*(t - ls_t3)/((ls_t1 - ls_t2)*(ls_t1 - ls_t3));
    ls_qf2 = (t - ls_t1)*(t - ls_t3)/((ls_t2 - ls_t1)*(ls_t2 - ls_t3));
    ls_qf3 = (t - ls_t1)*(t - ls_t2)/((ls_t2 - ls_t3)*(ls_t1 - ls_t3));
    return (0);
}

