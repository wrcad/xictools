
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

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Hong J. Park, Thomas L. Quarles 
         1993 Stephen R. Whiteley
****************************************************************************/

#define DISTO
#include "b1defs.h"
#include "distdefs.h"
#include "errors.h"

int
B1dev::dSetup(sB1model *model, sCKT *ckt)
{
    sB1instance *inst;
    sGENinstance *geninst;
    double DrainSatCurrent;
    double EffectiveLength;
    double GateBulkOverlapCap;
    double GateDrainOverlapCap;
    double GateSourceOverlapCap;
    double SourceSatCurrent;
    double DrainArea;
    double SourceArea;
    double DrainPerimeter;
    double SourcePerimeter;
//    double vt0;
    double evbs;
    double lgbs1, lgbs2, lgbs3;
    double czbd, czbs, czbdsw, czbssw;
    double PhiB, MJ, MJSW, PhiBSW;
    double arg, argsw, sarg, sargsw;
    double capbs1, capbs2, capbs3;
    double capbd1, capbd2, capbd3;
//    double qg;
//    double qb;
//    double qd;
    double Vfb;
    double Phi;
    double K1;
    double K2;
    double Vdd;
    double Ugs;
    double Uds;
    double Leff;
    double Eta;
    double Vpb;
    double SqrtVpb;
    double Von;
    double Vth;
    double DrCur;
    double G;
    double A;
    double Arg;
    double Beta;
    double Beta_Vds_0;
    double BVdd;
    double Beta0;
    double VddSquare;
    double C1;
    double C2;
    double VdsSat;
    double Argl1;
    double Argl2;
    double Vc;
    double Term1;
    double K;
//    double Args1;
    double Args2;
    double Args3;
    double Warg1;
    double Vcut;
    double N;
    double N0;
    double NB;
    double ND;
    double Warg2;
    double Wds;
    double Wgs;
    double Ilimit;
    double Iexp;
    double Vth0;
    double Arg1;
    double Arg2;
    double Arg3;
    double Arg5;
    double Ent;
    double Vcom;
    double Vgb;
    double Vgb_Vfb;
    double VdsPinchoff;
    double EntSquare;
    /*
    double VgsVthSquare;
    */
    double Argl5;
    double Argl6;
//    double Argl7;
    double WLCox;
    double Vtsquare;
    int ChargeComputationNeeded;
    double co4v15;
    double VgsVth  = 0.0;
    double lgbd1, lgbd2, lgbd3, evbd; 
    double vbd = 0.0;
//    double vgd = 0.0;
//    double vgb = 0.0;
    double vds = 0.0;
    double vgs = 0.0;
    double vbs = 0.0;
    double dBVdddVds;
    Dderivs d_Argl6;
    Dderivs d_Vgb, d_Vgb_Vfb, d_EntSquare, d_Argl5, d_Argl7;
    Dderivs d_Arg5, d_Ent, d_Vcom, d_VdsPinchoff;
    Dderivs d_qb, d_qd, d_Vth0, d_Arg1, d_Arg2, d_Arg3;
    Dderivs d_dummy, d_Vth, d_BVdd, d_Warg1, d_qg;
    Dderivs d_N, d_Wds, d_Wgs, d_Iexp;
    Dderivs d_Argl1, d_Argl2, d_Args1, d_Args2, d_Args3;
    Dderivs d_Beta, d_Vc, d_Term1, d_K, d_VdsSat;
    Dderivs d_Beta_Vds_0, d_C1, d_C2, d_Beta0;
    Dderivs d_VgsVth, d_G, d_A, d_Arg, d_DrCur;
    Dderivs d_Ugs, d_Uds, d_Eta, d_Vpb, d_SqrtVpb, d_Von;
    Dderivs d_p, d_q, d_r, d_zero;
    double gmin = ckt->CKTcurTask->TSKgmin;

    for ( ; model != 0; model = (sB1model*)model->GENnextModel ) {
        for (geninst = model->GENinstances; geninst != 0;
                geninst = geninst->GENnextInstance) {
            inst = (sB1instance*)geninst;

            EffectiveLength=inst->B1l - model->B1deltaL * 1.e-6;/* m */
            DrainArea = inst->B1drainArea;
            SourceArea = inst->B1sourceArea;
            DrainPerimeter = inst->B1drainPerimeter;
            SourcePerimeter = inst->B1sourcePerimeter;
            if( (DrainSatCurrent=DrainArea*model->B1jctSatCurDensity) 
                    < 1e-15){
                DrainSatCurrent = 1.0e-15;
            }
            if( (SourceSatCurrent=SourceArea*model->B1jctSatCurDensity)
                    <1.0e-15){
                SourceSatCurrent = 1.0e-15;
            }
            GateSourceOverlapCap = model->B1gateSourceOverlapCap *inst->B1w;
            GateDrainOverlapCap = model->B1gateDrainOverlapCap * inst->B1w;
            GateBulkOverlapCap = model->B1gateBulkOverlapCap *EffectiveLength;
//            vt0 = model->B1type * inst->B1vt0;

                    vbs = model->B1type * ( 
                        *(ckt->CKTrhsOld+inst->B1bNode) -
                        *(ckt->CKTrhsOld+inst->B1sNodePrime));
                    vgs = model->B1type * ( 
                        *(ckt->CKTrhsOld+inst->B1gNode) -
                        *(ckt->CKTrhsOld+inst->B1sNodePrime));
                    vds = model->B1type * ( 
                        *(ckt->CKTrhsOld+inst->B1dNodePrime) -
                        *(ckt->CKTrhsOld+inst->B1sNodePrime));

            if(vds >= 0) {
                /* normal mode */
                inst->B1mode = 1;
            } else {
                /* inverse mode */
                inst->B1mode = -1;
        vds = -vds;
        vgs = vgs + vds; /* these are the local(fake) values now */
        vbs = vbs + vds;
            }

//            vgb = vgs - vbs;
//            vgd = vgs - vds;
            vbd = vbs - vds;

            if(vbs <= 0.0 ) {
                lgbs1 = SourceSatCurrent / CONSTvt0 + gmin;
        lgbs2 = lgbs3 = 0.0;
            } else {
                evbs = exp(vbs/CONSTvt0);
                lgbs1 = SourceSatCurrent*evbs/CONSTvt0 + gmin;
        lgbs2 = (lgbs1 - gmin)/(CONSTvt0*2);
        lgbs3 = lgbs2/(CONSTvt0*3);
            }
            if(vbd <= 0.0) {
                lgbd1 = DrainSatCurrent / CONSTvt0 + gmin;
        lgbd2 = lgbd3 = 0.0;
            } else {
                evbd = exp(vbd/CONSTvt0);
                lgbd1 = DrainSatCurrent*evbd/CONSTvt0 + gmin;
        lgbd2 = (lgbd1 - gmin)/(CONSTvt0*2);
        lgbd3 = lgbd2/(CONSTvt0*3);
            }
            /* line 400 */
            /* call B1evaluate to calculate drain current and its 
             * derivatives and charge and capacitances related to gate
             * drain, and bulk
             */
          
/* check quadratic interpolation for beta0 ; line 360 */
/*
 * Copyright (c) 1985 Hong J. Park, Thomas L. Quarles
 * modified 1988 Jaijeet Roychowdhury 
 */

/* This routine evaluates the drain current, its derivatives and the
 * charges associated with the gate,bulk and drain terminal
 * using the BSIM1 (Berkeley Short-Channel IGFET Model) Equations.
 */
 /* as usual p=vgs, q=vbs, r=vds */
 {

        ChargeComputationNeeded  =  1;

    Vfb  =  inst->B1vfb;
    Phi  =  inst->B1phi;
    K1   =  inst->B1K1;
    K2   =  inst->B1K2;
    Vdd  =  model->B1vdd;
d_p.value = 0.0;
d_p.d1_p = 1.0;
d_p.d1_q = 0.0;
d_p.d1_r = 0.0;
d_p.d2_p2 = 0.0;
d_p.d2_q2 = 0.0;
d_p.d2_r2 = 0.0;
d_p.d2_pq = 0.0;
d_p.d2_qr = 0.0;
d_p.d2_pr = 0.0;
d_p.d3_p3 = 0.0;
d_p.d3_q3 = 0.0;
d_p.d3_r3 = 0.0;
d_p.d3_p2r = 0.0;
d_p.d3_p2q = 0.0;
d_p.d3_q2r = 0.0;
d_p.d3_pq2 = 0.0;
d_p.d3_pr2 = 0.0;
d_p.d3_qr2 = 0.0;
d_p.d3_pqr = 0.0;
    EqualDeriv(&d_q,&d_p);
    EqualDeriv(&d_r,&d_p);
    EqualDeriv(&d_zero,&d_p);
    d_q.d1_p = d_r.d1_p = d_zero.d1_p = 0.0;
    d_q.d1_q = d_r.d1_r = 1.0;
    d_p.value = vgs; d_q.value = vbs; d_r.value = vds;

    if((Ugs  =  inst->B1ugs + inst->B1ugsB * vbs) <= 0 ) {
        Ugs = 0;
    EqualDeriv(&d_Ugs,&d_zero);
    } else {
    EqualDeriv(&d_Ugs,&d_q);
    d_Ugs.value = Ugs;
    d_Ugs.d1_q = inst->B1ugsB;
    }
    if((Uds  =  inst->B1uds + inst->B1udsB * vbs + 
            inst->B1udsD*(vds-Vdd)) <= 0 ) {    
        Uds = 0.0;
    EqualDeriv(&d_Uds,&d_zero);
    } else {
        Leff  =  inst->B1l * 1.e6 - model->B1deltaL; /* Leff in um */
    /*const*/
        Uds  =  Uds / Leff;
    /* Uds = (inst->B1uds + inst->B1udsB * vbs inst->B1udsD*
        (vds-Vdd))/Leff */
    EqualDeriv(&d_Uds,&d_r);
    d_Uds.value = Uds;
    d_Uds.d1_r = inst->B1udsD/Leff;
    d_Uds.d1_q = inst->B1udsB/Leff;
    }
    Eta  =  inst->B1eta + inst->B1etaB * vbs + inst->B1etaD * 
        (vds - Vdd);
    EqualDeriv(&d_Eta,&d_r);
    d_Eta.value = Eta;
    d_Eta.d1_r = inst->B1etaD;
    d_Eta.d1_q = inst->B1etaB;

    if( Eta <= 0 ) {   
        Eta  = 0; 
        EqualDeriv(&d_Eta,&d_zero);
    } else if ( Eta > 1 ) {
        Eta = 1;
    EqualDeriv(&d_Eta,&d_zero);
    d_Eta.value = 1.0;
    } 
    if( vbs < 0 ) {
        Vpb  =  Phi - vbs;
    EqualDeriv(&d_Vpb,&d_q);
    d_Vpb.value = Vpb;
    d_Vpb.d1_q = -1;
    } else {
        Vpb  =  Phi;
    EqualDeriv(&d_Vpb,&d_zero);
    d_Vpb.value = Phi;
    }
    SqrtVpb  =  sqrt( Vpb );
    SqrtDeriv(&d_SqrtVpb,&d_Vpb);
    Von  = Vfb + Phi + K1 * SqrtVpb - K2 * Vpb - Eta * vds;
    EqualDeriv(&d_dummy,&d_r);
    d_dummy.value = -Eta*vds;
    d_dummy.d1_r = -Eta;
    TimesDeriv(&d_Von,&d_Vpb,-K2);
    PlusDeriv(&d_Von,&d_Von,&d_dummy);
    TimesDeriv(&d_dummy,&d_SqrtVpb,K1);
    PlusDeriv(&d_Von,&d_dummy,&d_Von);
    d_Von.value = Von;
    Vth = Von;
    EqualDeriv(&d_Vth,&d_Von);
    VgsVth  =  vgs - Vth;
    TimesDeriv(&d_VgsVth,&d_Vth,-1.0);
    d_VgsVth.value = VgsVth;
    d_VgsVth.d1_p += 1.0;

    G  =   1./(1.744+0.8364 * Vpb);
    TimesDeriv(&d_G,&d_Vpb,0.8364);
    d_G.value += 1.744;
    InvDeriv(&d_G,&d_G);
    G = 1 - G;
    TimesDeriv(&d_G,&d_G,-1.0);
    d_G.value += 1.0;
    A  =  G/SqrtVpb;
    DivDeriv(&d_A,&d_G,&d_SqrtVpb);
    A = 1.0 + 0.5*K1*A;
    TimesDeriv(&d_A,&d_A,0.5*K1);
    d_A.value += 1.0;
    A = SPMAX( A, 1.0);   /* Modified */
    if (A <= 1.0) {
    EqualDeriv(&d_A,&d_zero);
    d_A.value = 1.0;
    }
    Arg  = SPMAX(( 1 + Ugs * VgsVth), 1.0);
    MultDeriv(&d_dummy,&d_Ugs,&d_VgsVth);
    d_dummy.value += 1.0;
    if (d_dummy.value <= 1.0) {
    EqualDeriv(&d_Arg,&d_zero);
    d_Arg.value = 1.0;
    } 
    else
    EqualDeriv(&d_Arg,&d_dummy);

    if( VgsVth < 0 ) {
        /* cutoff */
        DrCur  = 0;
    EqualDeriv(&d_DrCur,&d_zero);
        goto SubthresholdComputation;
    }

    /* Quadratic Interpolation for Beta0 (Beta at vgs  =  0, vds=Vds) */

    Beta_Vds_0  =  (inst->B1betaZero + inst->B1betaZeroB * vbs);
    EqualDeriv(&d_Beta_Vds_0,&d_q);
    d_Beta_Vds_0.value = Beta_Vds_0;
    d_Beta_Vds_0.d1_q = inst->B1betaZeroB;
    BVdd  =  (inst->B1betaVdd + inst->B1betaVddB * vbs);
    EqualDeriv(&d_BVdd,&d_q);
    d_BVdd.value = BVdd;
    d_BVdd.d1_q = inst->B1betaVddB;
    dBVdddVds = SPMAX( inst->B1betaVddD, 0.0);


    /* is the above wrong ?!? */



    if( vds > Vdd ) {
        Beta0  =  BVdd + dBVdddVds * (vds - Vdd);
    EqualDeriv(&d_Beta0,&d_r); d_Beta0.value = vds - Vdd;
    TimesDeriv(&d_Beta0,&d_Beta0,dBVdddVds);
    PlusDeriv(&d_Beta0,&d_Beta0,&d_BVdd);
    /* dBVdddVds = const */


    /* and this stuff inst? */

    } else {
        VddSquare  =  Vdd * Vdd; /* const */
        C1  =  ( -BVdd + Beta_Vds_0 + dBVdddVds * Vdd) / VddSquare;
    TimesDeriv(&d_C1,&d_BVdd,-1.0);
    PlusDeriv(&d_C1,&d_C1,&d_Beta_Vds_0);
    d_C1.value += dBVdddVds * Vdd;
    TimesDeriv(&d_C1,&d_C1,1/VddSquare);

        C2  =  2 * (BVdd - Beta_Vds_0) / Vdd - dBVdddVds;
    TimesDeriv(&d_C2,&d_Beta_Vds_0,-1.0);
    PlusDeriv(&d_C2,&d_C2,&d_BVdd);
    TimesDeriv(&d_C2,&d_C2,2/Vdd);
    d_C2.value -= dBVdddVds;
        Beta0  =  (C1 * vds + C2) * vds + Beta_Vds_0;
    MultDeriv(&d_Beta0,&d_r,&d_C1);
    PlusDeriv(&d_Beta0,&d_Beta0,&d_C2);
    MultDeriv(&d_Beta0,&d_Beta0,&d_dummy);
    PlusDeriv(&d_Beta0,&d_Beta0,&d_Beta_Vds_0);

    /*
        dBeta0dVds  =  2*C1*vds + C2;
        dBeta0dVbs  =  dC1dVbs * vds * vds + dC2dVbs * vds + dBeta_Vds_0_dVbs;
    maybe we'll need these later */
    }

    /*Beta  =  Beta0 / ( 1 + Ugs * VgsVth );*/

    Beta = Beta0 / Arg ;
    DivDeriv(&d_Beta,&d_Beta0,&d_Arg);

    /*VdsSat  = SPMAX( VgsVth / ( A + Uds * VgsVth ),  0.0);*/

    Vc  =  Uds * VgsVth / A;
    DivDeriv(&d_Vc,&d_VgsVth,&d_A);
    MultDeriv(&d_Vc,&d_Vc,&d_Uds);

    if(Vc < 0.0 ) {
    EqualDeriv(&d_Vc,&d_zero);
    Vc=0.0;
    }
    Term1  =  sqrt( 1 + 2 * Vc );
    TimesDeriv(&d_Term1,&d_Vc,2.0);
    d_Term1.value += 1.0;
    SqrtDeriv(&d_Term1,&d_Term1);
    K  =  0.5 * ( 1 + Vc + Term1 );
    PlusDeriv(&d_K,&d_Vc,&d_Term1);
    d_K.value += 1.0;
    TimesDeriv(&d_K,&d_K,0.5);
    VdsSat = VgsVth / ( A * sqrt(K));
    if (VdsSat < 0.0) {
    EqualDeriv(&d_VdsSat,&d_zero);
    VdsSat = 0.0;
    }
    else
    {
    SqrtDeriv(&d_VdsSat,&d_K);
    MultDeriv(&d_VdsSat,&d_VdsSat,&d_A);
    DivDeriv(&d_VdsSat,&d_VgsVth,&d_VdsSat);
    }

    if( vds < VdsSat ) {
        /* Triode Region */
        /*Argl1  =  1 + Uds * vds;*/
    Argl1 = 1 + Uds * vds;
    if (Argl1 < 1.0) {
    Argl1 = 1.0;
    EqualDeriv(&d_Argl1,&d_zero);
    d_Argl1.value = 1.0;
    }
    else
    {
    MultDeriv(&d_Argl1,&d_r,&d_Uds);
    d_Argl1.value += 1.0;
    }


        Argl2  =  VgsVth - 0.5 * A * vds;
    MultDeriv(&d_Argl2,&d_r,&d_A);
    TimesDeriv(&d_Argl2,&d_Argl2,-0.5);
    PlusDeriv(&d_Argl2,&d_Argl2,&d_VgsVth);
        DrCur  =  Beta * Argl2 * vds / Argl1;
    DivDeriv(&d_DrCur,&d_r,&d_Argl1);
    MultDeriv(&d_DrCur,&d_DrCur,&d_Argl2);
    MultDeriv(&d_DrCur,&d_DrCur,&d_Beta);
    } else {  
        /* Pinchoff (Saturation) Region */
    /* history
        Args1  =   1.0 + 1. / Term1;
    InvDeriv(&d_Args1,&d_Term1);
    d_Args1.value += 1.0;
    */
        /* dVcdVgs  =  Uds / A;
        dVcdVds  =  VgsVth * dUdsdVds / A - dVcdVgs * dVthdVds;
        dVcdVbs  =  ( VgsVth * dUdsdVbs - Uds * 
            (dVthdVbs + VgsVth * dAdVbs / A ))/ A;
        dKdVc  =  0.5* Args1;
        dKdVgs  =  dKdVc * dVcdVgs;
        dKdVds  =  dKdVc * dVcdVds;
        dKdVbs  =  dKdVc * dVcdVbs; */
        Args2  =  VgsVth / A / K;
    MultDeriv(&d_Args2,&d_A,&d_K);
    DivDeriv(&d_Args2,&d_VgsVth,&d_Args2);
        Args3  =  Args2 * VgsVth;
    MultDeriv(&d_Args3,&d_Args2,&d_VgsVth);
        DrCur  =  0.5 * Beta * Args3;
    MultDeriv(&d_DrCur,&d_Beta,&d_Args3);
    TimesDeriv(&d_DrCur,&d_DrCur,0.5);
    }

SubthresholdComputation:

    N0  =  inst->B1subthSlope;/*const*/
    Vcut  =  - 40. * N0 * CONSTvt0 ;/*const*/
    if( (N0 >=  200) || (VgsVth < Vcut ) || (VgsVth > (-0.5*Vcut))) {
        goto ChargeComputation;
    }
    
    NB  =  inst->B1subthSlopeB;/*const*/
    ND  =  inst->B1subthSlopeD;/*const*/
    N  =  N0 + NB * vbs + ND * vds; /* subthreshold slope */
    EqualDeriv(&d_N,&d_r);
    d_N.value = N;
    d_N.d1_q = NB;
    d_N.d1_r = ND;
    if( N < 0.5 ){ N  =  0.5;
    d_N.value = 0.5;
    d_N.d1_q = d_N.d1_r = 0.0;
    }
    Warg1  =  exp( - vds / CONSTvt0 );
    TimesDeriv(&d_Warg1,&d_r,-1/CONSTvt0);
    ExpDeriv(&d_Warg1,&d_Warg1);
    Wds  =  1 - Warg1;
    TimesDeriv(&d_Wds,&d_Warg1,-1.0);
    d_Wds.value += 1.0;
    Wgs  =  exp( VgsVth / ( N * CONSTvt0 ));
    DivDeriv(&d_Wgs,&d_VgsVth,&d_N);
    TimesDeriv(&d_Wgs,&d_Wgs,1/CONSTvt0);
    ExpDeriv(&d_Wgs,&d_Wgs);
    Vtsquare = CONSTvt0 * CONSTvt0 ;/*const*/
    Warg2  =  6.04965 * Vtsquare * inst->B1betaZero;/*const*/
    Ilimit  =  4.5 * Vtsquare * inst->B1betaZero;/*const*/
    Iexp = Warg2 * Wgs * Wds;
    MultDeriv(&d_Iexp,&d_Wgs,&d_Wds);
    TimesDeriv(&d_Iexp,&d_Iexp,Warg2);
    DrCur  =  DrCur + Ilimit * Iexp / ( Ilimit + Iexp );
    EqualDeriv(&d_dummy,&d_Iexp);
    d_dummy.value += Ilimit;
    InvDeriv(&d_dummy,&d_dummy);
    MultDeriv(&d_dummy,&d_dummy,&d_Iexp);
    TimesDeriv(&d_dummy,&d_dummy,Ilimit);
    PlusDeriv(&d_DrCur,&d_DrCur,&d_dummy);

    /* gds term has been modified to prevent blow up at Vds=0 */
    /* gds = gds + Temp3 * ( -Wds / N / CONSTvt0 * (dVthdVds + 
        VgsVth * ND / N ) + Warg1 / CONSTvt0 ); */

ChargeComputation:

    /* Some Limiting of DC Parameters */
/*    if(DrCur < 0.0) DrCur = 0.0;
    if(gm < 0.0) gm = 0.0;
    if(gds < 0.0) gds = 0.0;
    if(gmbs < 0.0) gmbs = 0.0;
    */

    WLCox = model->B1Cox * 
        (inst->B1l - model->B1deltaL * 1.e-6) * 
        (inst->B1w - model->B1deltaW * 1.e-6) * 1.e4;   /* F */

    if( ! ChargeComputationNeeded )  {  
//        qg  = 0;
//        qd = 0;
//        qb = 0;
    EqualDeriv(&d_qg,&d_zero);
    EqualDeriv(&d_qb,&d_zero);
    EqualDeriv(&d_qd,&d_zero);
        goto finished;
    }
    G  =   1.0 - 1./(1.744+0.8364 * Vpb);
    TimesDeriv(&d_G,&d_Vpb,-0.8364);
    d_G.value -= 1.744;
    InvDeriv(&d_G,&d_G);
    d_G.value += 1.0;

    A  =  1.0 + 0.5*K1*G/SqrtVpb;
    if (A < 1.0) {
    A = 1.0;
    EqualDeriv(&d_A,&d_zero);
    d_A.value = 1.0;
    }
    else
    {
    DivDeriv(&d_A,&d_G,&d_SqrtVpb);
    TimesDeriv(&d_A,&d_A,0.5*K1);
    d_A.value += 1.0;
    }

    /*Arg  =  1 + Ugs * VgsVth;*/
    Phi  =  SPMAX( 0.1, Phi);/*const*/

    if( model->B1channelChargePartitionFlag >= 1 ) {

/*0/100 partitioning for drain/source chArges at the saturation region*/
        Vth0 = Vfb + Phi + K1 * SqrtVpb;
    TimesDeriv(&d_Vth0,&d_SqrtVpb,K1);
    d_Vth0.value += Vfb + Phi;
        VgsVth = vgs - Vth0;
    TimesDeriv(&d_VgsVth,&d_Vth0,-1.0);
    PlusDeriv(&d_VgsVth,&d_VgsVth,&d_p);
        Arg1 = A * vds;
    MultDeriv(&d_Arg1,&d_A,&d_r);
        Arg2 = VgsVth - 0.5 * Arg1;
    TimesDeriv(&d_Arg2,&d_Arg1,-0.5);
    PlusDeriv(&d_Arg2,&d_Arg2,&d_VgsVth);
        Arg3 = vds - Arg1;
    TimesDeriv(&d_Arg3,&d_Arg1,-1.0);
    PlusDeriv(&d_Arg3,&d_Arg3,&d_r);
        Arg5 = Arg1 * Arg1;
    MultDeriv(&d_Arg5,&d_Arg1,&d_Arg1);

        Ent = SPMAX(Arg2,1.0e-8);
    if (Arg2 < 1.0e-8) {
    EqualDeriv(&d_Ent,&d_zero);
    d_Ent.value = 1.0e-8;
    }
    else
    {
    EqualDeriv(&d_Ent,&d_Arg2);
    }

        Vcom = VgsVth * VgsVth / 6.0 - 1.25e-1 * Arg1 * 
            VgsVth + 2.5e-2 * Arg5;
        TimesDeriv(&d_dummy,&d_Arg1,-0.125);
        TimesDeriv(&d_Vcom,&d_VgsVth,1/6.0);
        PlusDeriv(&d_Vcom,&d_Vcom,&d_dummy);
        MultDeriv(&d_Vcom,&d_Vcom,&d_VgsVth);
        TimesDeriv(&d_dummy,&d_Arg5,2.5e-2);
        PlusDeriv(&d_Vcom,&d_Vcom,&d_dummy);
    VdsPinchoff = VgsVth / A;
    if (VdsPinchoff < 0.0) {
    VdsPinchoff = 0.0;
    EqualDeriv(&d_VdsPinchoff,&d_zero);
    }
    else
    {
    DivDeriv(&d_VdsPinchoff,&d_VgsVth,&d_A);
    }
        Vgb  =  vgs  -  vbs ;
    EqualDeriv(&d_Vgb,&d_p);
    d_Vgb.value = Vgb;
    d_Vgb.d1_q = -1.0;
        Vgb_Vfb  =  Vgb  -  Vfb;
    EqualDeriv(&d_Vgb_Vfb,&d_Vgb);
    d_Vgb_Vfb.value -= Vfb;

        if( Vgb_Vfb < 0){
            /* Accumulation Region */
//            qg  =  WLCox * Vgb_Vfb;
        TimesDeriv(&d_qg,&d_Vgb_Vfb,WLCox);
//            qb  =  - qg;
        TimesDeriv(&d_qb,&d_qg,-1.0);
//            qd  =  0. ;
        EqualDeriv(&d_qd,&d_zero);
            goto finished;
        } else if ( vgs < Vth0 ){
            /* Subthreshold Region */
//            qg  =  0.5 * WLCox * K1 * K1 * (-1 + sqrt(1 + 4 * Vgb_Vfb / (K1 * K1)));
            TimesDeriv(&d_qg,&d_Vgb_Vfb,4/(K1*K1));
        d_qg.value += 1.0;
        SqrtDeriv(&d_qg,&d_qg);
        d_qg.value -= 1.0;
        TimesDeriv(&d_qg,&d_qg,0.5 * WLCox * K1 * K1);
//            qb  =  -qg;
        TimesDeriv(&d_qb,&d_qg,-1.0);
//            qd  =  0.;
        EqualDeriv(&d_qd,&d_zero);
            goto finished;
        } else if( vds < VdsPinchoff ){    /* triode region  */
            /*VgsVth2 = VgsVth*VgsVth;*/
            EntSquare = Ent * Ent;
        MultDeriv(&d_EntSquare,&d_Ent,&d_Ent);
            Argl1 = 1.2e1 * EntSquare;
        TimesDeriv(&d_Argl1,&d_EntSquare,12.0);
            Argl2 = 1.0 - A;
        TimesDeriv(&d_Argl2,&d_A,-1.0);
        d_Argl2.value += 1.0;
        /*
            Argl3 = Arg1 * vds;
        MultDeriv(&d_Argl3,&d_Arg1,&d_r);
        */
            /*Argl4 = Vcom/Ent/EntSquare;*/
            if (Ent > 1.0e-8) {   
                Argl5 = Arg1 / Ent;
        DivDeriv(&d_Argl5,&d_Arg1,&d_Ent);
                /*Argl6 = Vcom/EntSquare;*/  
            } else {   
                Argl5 = 2.0;
        EqualDeriv(&d_Argl5,&d_zero);
        d_Argl5.value = 2.0;
                Argl6 = 4.0 / 1.5e1;/*const*/
            }
//            Argl7 = Argl5 / 1.2e1;
        TimesDeriv(&d_Argl7,&d_Argl5,1.0/12.0);
        /*
            Argl8 = 6.0 * Ent;
        TimesDeriv(&d_Argl8,&d_Ent,6.0);
            Argl9 = 0.125 * Argl5 * Argl5;
        MultDeriv(&d_Argl9,&d_Argl5,&d_Argl5);
        TimesDeriv(&d_Argl9,&d_Argl9,0.125);
        */
//            qg = WLCox * (vgs - Vfb - Phi - 0.5 * vds + vds * Argl7);
        EqualDeriv(&d_qg,&d_Argl7);
        d_qg.value -= 0.5;
        MultDeriv(&d_qg,&d_qg,&d_r);
        d_qg.value  += vgs - Vfb - Phi;
        d_qg.d1_p += 1.0;
        TimesDeriv(&d_qg,&d_qg,WLCox);
//            qb = WLCox * ( - Vth0 + Vfb + Phi + 0.5 * Arg3 - Arg3 * Argl7);
        TimesDeriv(&d_qb,&d_Argl7,-1.0);
        d_qb.value += 0.5;
        MultDeriv(&d_qb,&d_qb,&d_Arg3);
        d_qb.value += Vfb + Phi;
        TimesDeriv(&d_dummy,&d_Vth0,-1.0);
        PlusDeriv(&d_qb,&d_qb,&d_dummy);
        TimesDeriv(&d_qb,&d_qb,WLCox);
//            qd =  - WLCox * (0.5 * VgsVth - 0.75 * Arg1 + 0.125 * Arg1 * Argl5);
        TimesDeriv(&d_qd,&d_Argl5,0.125);
        d_qd.value -= 0.75;
        MultDeriv(&d_qd,&d_qd,&d_Arg1);
        TimesDeriv(&d_dummy,&d_VgsVth,0.5);
        PlusDeriv(&d_qd,&d_qd,&d_dummy);
        TimesDeriv(&d_qd,&d_qd, -WLCox);
            goto finished;
        } else if( vds >= VdsPinchoff ) {    /* saturation region   */
//            Args1 = 1.0 /  (3*A);
        TimesDeriv(&d_Args1,&d_A,3.0);
        InvDeriv(&d_Args1,&d_Args1);
//            qg = WLCox * (vgs - Vfb - Phi - VgsVth * Args1);
        MultDeriv(&d_qg,&d_VgsVth,&d_Args1);
        d_qg.value += Vfb + Phi - vgs;
        d_qg.d1_p -= 1.0;
        TimesDeriv(&d_qg,&d_qg,-WLCox);
//            qb = WLCox * (Vfb + Phi - Vth0 + (1.0 - A) * VgsVth * Args1);
        TimesDeriv(&d_dummy,&d_A,-1.0);
        d_dummy.value += 1.0;
        MultDeriv(&d_qb,&d_VgsVth, &d_dummy);
        MultDeriv(&d_qb,&d_qb,&d_Args1);
        d_qb.value += Vfb + Phi;
        TimesDeriv(&d_dummy,&d_Vth0,-1.0);
        PlusDeriv(&d_qb,&d_qb,&d_dummy);
        TimesDeriv(&d_qb,&d_qb,WLCox);
//            qd = 0.0;
        EqualDeriv(&d_qd,&d_zero);
            goto finished;
        }

        goto finished;

    } else {

/*0/100 partitioning for drain/source chArges at the saturation region*/
    co4v15 = 1./15.;
        Vth0 = Vfb + Phi + K1 * SqrtVpb;
    TimesDeriv(&d_Vth0,&d_SqrtVpb,K1);
    d_Vth0.value += Vfb + Phi;
        VgsVth = vgs - Vth0;
    TimesDeriv(&d_VgsVth,&d_Vth0,-1.0);
    d_VgsVth.value += vgs;
    d_VgsVth.d1_p += 1.0;
        Arg1 = A * vds;
    MultDeriv(&d_Arg1,&d_A,&d_r);
        Arg2 = VgsVth - 0.5 * Arg1;
    TimesDeriv(&d_Arg2,&d_Arg1,-0.5);
    PlusDeriv(&d_Arg2,&d_Arg2,&d_VgsVth);
        Arg3 = vds - Arg1;
    TimesDeriv(&d_Arg3,&d_Arg1,-1.0);
    d_Arg3.value = Arg3;
    d_Arg3.d1_r += 1.0;
        Arg5 = Arg1 * Arg1;
    MultDeriv(&d_Arg5,&d_Arg1,&d_Arg1);
        Ent = SPMAX(Arg2,1.0e-8);
    if (Arg2 < 1.0e-8) {
    EqualDeriv(&d_Ent,&d_zero);
    d_Ent.value = Ent;
    }
    else
    {
    EqualDeriv(&d_Ent,&d_Arg2);
    }

        Vcom = VgsVth * VgsVth / 6.0 - 1.25e-1 * Arg1 * 
            VgsVth + 2.5e-2 * Arg5;
    TimesDeriv(&d_dummy,&d_VgsVth,1/6.0);
    TimesDeriv(&d_Vcom,&d_Arg1,-0.125);
    PlusDeriv(&d_Vcom,&d_Vcom,&d_dummy);
    MultDeriv(&d_Vcom,&d_Vcom,&d_VgsVth);
    TimesDeriv(&d_dummy,&d_Arg5,2.5e-2);
    PlusDeriv(&d_Vcom,&d_Vcom,&d_dummy);
    VdsPinchoff = VgsVth / A;
    if (VdsPinchoff < 0.0) {
    VdsPinchoff = 0.0;
    EqualDeriv(&d_VdsPinchoff,&d_zero);
    }
    else
    {
    DivDeriv(&d_VdsPinchoff,&d_VgsVth,&d_A);
    }

        Vgb  =  vgs  -  vbs ;
    EqualDeriv(&d_Vgb,&d_p);
    d_Vgb.value = Vgb;
    d_Vgb.d1_q = -1.0;
        Vgb_Vfb  =  Vgb  -  Vfb;
    EqualDeriv(&d_Vgb_Vfb,&d_Vgb);
    d_Vgb_Vfb.value = Vgb_Vfb;

        if( Vgb_Vfb < 0){
            /* Accumulation Region */
//            qg  =  WLCox * Vgb_Vfb;
        TimesDeriv(&d_qg,&d_Vgb_Vfb,WLCox);
//            qb  =  - qg;
        TimesDeriv(&d_qb,&d_qg,-1.0);
//            qd  =  0. ;
        EqualDeriv(&d_qd,&d_zero);
            goto finished;
        } else if ( vgs < Vth0 ){
            /* Subthreshold Region */
//            qg  =  0.5 * WLCox * K1 * K1 * (-1 + sqrt(1 + 4 * Vgb_Vfb / (K1 * K1)));
            TimesDeriv(&d_qg,&d_Vgb_Vfb,4/(K1*K1));
        d_qg.value += 1.0;
        SqrtDeriv(&d_qg,&d_qg);
        d_qg.value -= 1.0;
        TimesDeriv(&d_qg,&d_qg,0.5 * WLCox * K1 * K1);
//            qb  =  -qg;
        TimesDeriv(&d_qb,&d_qg,-1.0);
//            qd  =  0.;
        EqualDeriv(&d_qd,&d_zero);
            goto finished;
        } else if( vds < VdsPinchoff ){    /* triode region  */
        /*
            VgsVthSquare = VgsVth*VgsVth;
        MultDeriv(&d_VgsVthSquare,&d_VgsVth,&d_VgsVth);
        */
            EntSquare = Ent * Ent;
        MultDeriv(&d_EntSquare,&d_Ent,&d_Ent);
            Argl1 = 1.2e1 * EntSquare;
        TimesDeriv(&d_Argl1,&d_EntSquare,12.0);
            Argl2 = 1.0 - A;
        TimesDeriv(&d_Argl2,&d_A,-1.0);
        d_Argl2.value += 1.0;
        /*
            Argl3 = Arg1 * vds;
        MultDeriv(&d_Argl3,&d_Arg1,&d_r);
            Argl4 = Vcom/Ent/EntSquare;
        MultDeriv(&d_Argl4,&d_Ent,&d_EntSquare);
        DivDeriv(&d_Argl4,&d_Vcom,&d_Argl4);
        */
            if (Ent > 1.0e-8) {   
                Argl5 = Arg1 / Ent;
        DivDeriv(&d_Argl5,&d_Arg1,&d_Ent);
                Argl6 = Vcom/EntSquare;
        DivDeriv(&d_Argl6,&d_Vcom,&d_EntSquare);
            } else {   
                Argl5 = 2.0;
        EqualDeriv(&d_Argl5,&d_zero);
        d_Argl5.value = Argl5;
                Argl6 = 4.0 / 1.5e1;
        EqualDeriv(&d_Argl6,&d_zero);
        d_Argl6.value = Argl6;
            }
//            Argl7 = Argl5 / 1.2e1;
        TimesDeriv(&d_Argl7,&d_Argl5,1/12.0);
        /*
            Argl8 = 6.0 * Ent;
        TimesDeriv(&d_Argl8,&d_Ent,6.0);
            Argl9 = 0.125 * Argl5 * Argl5;
        MultDeriv(&d_Argl9,&d_Argl5,&d_Argl5);
        TimesDeriv(&d_Argl9,&d_Argl9,0.125);
        */
//            qg = WLCox * (vgs - Vfb - Phi - 0.5 * vds + vds * Argl7);
        EqualDeriv(&d_qg,&d_Argl7);
        d_qg.value -= 0.5;
        MultDeriv(&d_qg,&d_qg,&d_r);
        d_qg.value  += vgs - Vfb - Phi;
        d_qg.d1_p += 1.0;
        TimesDeriv(&d_qg,&d_qg,WLCox);
//            qb = WLCox * ( - Vth0 + Vfb + Phi + 0.5 * Arg3 - Arg3 * Argl7);
        TimesDeriv(&d_qb,&d_Argl7,-1.0);
        d_qb.value += 0.5;
        MultDeriv(&d_qb,&d_qb,&d_Arg3);
        d_qb.value += Vfb + Phi;
        TimesDeriv(&d_dummy,&d_Vth0,-1.0);
        PlusDeriv(&d_qb,&d_qb,&d_dummy);
        TimesDeriv(&d_qb,&d_qb,WLCox);
//            qd =  - WLCox * (0.5 * (VgsVth -  Arg1) + Arg1 * Argl6);
        MultDeriv(&d_dummy,&d_Arg1,&d_Argl6);
        TimesDeriv(&d_qd,&d_Arg1,-1.0);
        PlusDeriv(&d_qd,&d_qd,&d_VgsVth);
        TimesDeriv(&d_qd,&d_qd,0.5);
        PlusDeriv(&d_qd,&d_qd,&d_dummy);
        TimesDeriv(&d_qd,&d_qd,-WLCox);
            goto finished;
        } else if( vds >= VdsPinchoff ) {    /* saturation region   */
//            Args1 = 1.0 /  (3*A);
        TimesDeriv(&d_Args1,&d_A,3.0);
        InvDeriv(&d_Args1,&d_Args1);

//            qg = WLCox * (vgs - Vfb - Phi - VgsVth * Args1);
        MultDeriv(&d_qg,&d_VgsVth,&d_Args1);
        d_qg.value += Vfb + Phi - vgs;
        d_qg.d1_p -= 1.0;
        TimesDeriv(&d_qg,&d_qg,-WLCox);
//            qb = WLCox * (Vfb + Phi - Vth0 + (1.0 - A) * VgsVth * Args1);
        TimesDeriv(&d_dummy,&d_A,-1.0);
        d_dummy.value += 1.0;
        MultDeriv(&d_qb,&d_VgsVth, &d_dummy);
        MultDeriv(&d_qb,&d_qb,&d_Args1);
        d_qb.value += Vfb + Phi;
        TimesDeriv(&d_dummy,&d_Vth0,-1.0);
        PlusDeriv(&d_qb,&d_qb,&d_dummy);
        TimesDeriv(&d_qb,&d_qb,WLCox);
//            qd = -co4v15*WLCox*VgsVth;
        TimesDeriv(&d_qd,&d_VgsVth,-co4v15*WLCox);
            goto finished;
        }


    }

}   
finished:       /* returning Values to Calling Routine */
       
      
        

        /*
         * the above has set up (DrCur) and (the node q's)
         * and (their derivatives upto third order wrt vgs, vbs, and
         * vds)
         */
            /*
             *  
             */
                /*
                 *  charge storage elements
                 *
                 *   bulk-drain and bulk-source depletion capacitances
                 *  czbd : zero bias drain junction capacitance
                 *  czbs : zero bias source junction capacitance
                 * czbdsw:zero bias drain junction sidewall capacitance
                 * czbssw:zero bias source junction sidewall capacitance
                 */

                czbd  = model->B1unitAreaJctCap * DrainArea;
                czbs  = model->B1unitAreaJctCap * SourceArea;
                czbdsw= model->B1unitLengthSidewallJctCap * DrainPerimeter;
                czbssw= model->B1unitLengthSidewallJctCap * SourcePerimeter;
                PhiB = model->B1bulkJctPotential;
                PhiBSW = model->B1sidewallJctPotential;
                MJ = model->B1bulkJctBotGradingCoeff;
                MJSW = model->B1bulkJctSideGradingCoeff;

                /* Source Bulk Junction */
                if( vbs < 0 ) {  
                    arg = 1 - vbs / PhiB;
                    argsw = 1 - vbs / PhiBSW;
                    sarg = exp(-MJ*log(arg));
                    sargsw = exp(-MJSW*log(argsw));
                    /* *(ckt->CKTstate0 + inst->B1qbs) =
                        PhiB * czbs * (1-arg*sarg)/(1-MJ) + PhiBSW * 
                    czbssw * (1-argsw*sargsw)/(1-MJSW); */
                    capbs1 = czbs * sarg + czbssw * sargsw ;
            capbs2 = (czbs * MJ * sarg / (PhiB*arg)
                + czbssw * MJSW * sargsw /(PhiBSW*argsw))/2.0;
            capbs3 = (czbs * (MJ) * (MJ + 1.) * sarg /((PhiB*arg)*(PhiB*arg))
                + czbssw * MJSW * (MJSW + 1.) * sargsw / (PhiBSW*argsw*PhiBSW*argsw))/6.0;
                } else {  
                    /* *(ckt->CKTstate0+inst->B1qbs) =
                        vbs*(czbs+czbssw)+ vbs*vbs*(czbs*MJ*0.5/PhiB 
                        + czbssw * MJSW * 0.5/PhiBSW); */
                    capbs1 = czbs + czbssw + vbs *(czbs*MJ/PhiB+
                        czbssw * MJSW / PhiBSW );
            capbs2 = (czbs*MJ/PhiB+czbssw * MJSW / PhiBSW )*0.5;
            capbs3 = 0.0;
                }

                /* Drain Bulk Junction */
                if( vbd < 0 ) {  
                    arg = 1 - vbd / PhiB;
                    argsw = 1 - vbd / PhiBSW;
                    sarg = exp(-MJ*log(arg));
                    sargsw = exp(-MJSW*log(argsw));
                    /* *(ckt->CKTstate0 + inst->B1qbd) =
                        PhiB * czbd * (1-arg*sarg)/(1-MJ) + PhiBSW * 
                    czbdsw * (1-argsw*sargsw)/(1-MJSW); */
                    capbd1 = czbd * sarg + czbdsw * sargsw ;
            capbd2 = (czbd * MJ * sarg / (PhiB*arg)
                + czbdsw * MJSW * sargsw /(PhiBSW*argsw))*0.5;
            capbd3 = (czbd * (MJ) * (MJ + 1.) * sarg /((PhiB*arg)*(PhiB*arg))
                + czbdsw * MJSW * (MJSW + 1.) * sargsw / (PhiBSW*argsw*PhiBSW*argsw))/6.0;
                } else {  
                    /* *(ckt->CKTstate0+inst->B1qbd) =
                        vbd*(czbd+czbdsw)+ vbd*vbd*(czbd*MJ*0.5/PhiB 
                        + czbdsw * MJSW * 0.5/PhiBSW); */
                    capbd1 = czbd + czbdsw + vbd *(czbd*MJ/PhiB+
                        czbdsw * MJSW / PhiBSW );
            capbd2 = 0.5*(czbs*MJ/PhiB+czbssw * MJSW / PhiBSW );
            capbd3 = 0.0;
                }





/* line755: */
             
    /*
    qgd = GateDrainOverlapCap * (vgs - vds);
    qgs = GateSourceOverlapCap * vgs;
    qgb = GateBulkOverlapCap * (vgs -vbs);
    *qGatePointer = *qGatePointer + qgd + qgs + qgb;
    *qBulkPointer = *qBulkPointer - qgb;
    *qDrainPointer = *qDrainPointer - qgd;
    *qSourcePointer = -(*qGatePointer + *qBulkPointer + *qDrainPointer);
    */
    d_qg.d1_p += GateDrainOverlapCap + GateSourceOverlapCap + GateBulkOverlapCap;
    d_qg.d1_q += -GateBulkOverlapCap;
    d_qg.d1_r += -GateDrainOverlapCap;
    d_qb.d1_p += -GateBulkOverlapCap;
    d_qb.d1_q += GateBulkOverlapCap + capbs1 + capbd1;
    d_qb.d1_r += -capbd1;
    d_qd.d1_p += - GateDrainOverlapCap;
    d_qd.d1_q += -capbd1;
    d_qd.d1_r += GateDrainOverlapCap + capbd1;
    /*
    d[23]_qg_d[vlgbds23] += 0.0;
    d2_qb_dvgs2 += 0.0;
    d3_qb_dvgs3 += 0.0
    d[23]_qb_dvgs[dvbds23] += 0.0
    */
    d_qb.d2_q2 += 2* ( capbd2 + capbs2);
    d_qb.d3_q3 += 6*(capbd3 + capbs3);
    d_qb.d2_qr += -2*capbd2;
    d_qb.d3_q2r += -capbd3*6;
    d_qb.d3_qr2 += capbd3*6;
    d_qb.d2_r2 += 2*capbd2;
    d_qb.d3_r3 += -6*capbd3;
    /*
    d[23]_qd_dp[dvbds23] += 0.0
    */
    d_qd.d2_q2 -= 2*capbd2;
    d_qd.d3_q3 -= 6*capbd3;
    d_qd.d2_r2 -= 2*capbd2;
    d_qd.d3_r3 -= -6*capbd3;
    d_qd.d2_qr -= -2*capbd2;
    d_qd.d3_q2r -= -6*capbd3;
    d_qd.d3_qr2 -= 6*capbd3;


    /* get all the coefficients and adjust for mode and type */
    if (inst->B1mode == 1)
        {
        /* normal mode - no source-drain interchange */
inst->qg_x = d_qg.d1_p;
inst->qg_y = d_qg.d1_q;
inst->qg_z = d_qg.d1_r;
inst->qg_x2 = d_qg.d2_p2;
inst->qg_y2 = d_qg.d2_q2;
inst->qg_z2 = d_qg.d2_r2;
inst->qg_xy = d_qg.d2_pq;
inst->qg_yz = d_qg.d2_qr;
inst->qg_xz = d_qg.d2_pr;
inst->qg_x3 = d_qg.d3_p3;
inst->qg_y3 = d_qg.d3_q3;
inst->qg_z3 = d_qg.d3_r3;
inst->qg_x2z = d_qg.d3_p2r;
inst->qg_x2y = d_qg.d3_p2q;
inst->qg_y2z = d_qg.d3_q2r;
inst->qg_xy2 = d_qg.d3_pq2;
inst->qg_xz2 = d_qg.d3_pr2;
inst->qg_yz2 = d_qg.d3_qr2;
inst->qg_xyz = d_qg.d3_pqr;

inst->qb_x = d_qb.d1_p;
inst->qb_y = d_qb.d1_q;
inst->qb_z = d_qb.d1_r;
inst->qb_x2 = d_qb.d2_p2;
inst->qb_y2 = d_qb.d2_q2;
inst->qb_z2 = d_qb.d2_r2;
inst->qb_xy = d_qb.d2_pq;
inst->qb_yz = d_qb.d2_qr;
inst->qb_xz = d_qb.d2_pr;
inst->qb_x3 = d_qb.d3_p3;
inst->qb_y3 = d_qb.d3_q3;
inst->qb_z3 = d_qb.d3_r3;
inst->qb_x2z = d_qb.d3_p2r;
inst->qb_x2y = d_qb.d3_p2q;
inst->qb_y2z = d_qb.d3_q2r;
inst->qb_xy2 = d_qb.d3_pq2;
inst->qb_xz2 = d_qb.d3_pr2;
inst->qb_yz2 = d_qb.d3_qr2;
inst->qb_xyz = d_qb.d3_pqr;

inst->qd_x = d_qd.d1_p;
inst->qd_y = d_qd.d1_q;
inst->qd_z = d_qd.d1_r;
inst->qd_x2 = d_qd.d2_p2;
inst->qd_y2 = d_qd.d2_q2;
inst->qd_z2 = d_qd.d2_r2;
inst->qd_xy = d_qd.d2_pq;
inst->qd_yz = d_qd.d2_qr;
inst->qd_xz = d_qd.d2_pr;
inst->qd_x3 = d_qd.d3_p3;
inst->qd_y3 = d_qd.d3_q3;
inst->qd_z3 = d_qd.d3_r3;
inst->qd_x2z = d_qd.d3_p2r;
inst->qd_x2y = d_qd.d3_p2q;
inst->qd_y2z = d_qd.d3_q2r;
inst->qd_xy2 = d_qd.d3_pq2;
inst->qd_xz2 = d_qd.d3_pr2;
inst->qd_yz2 = d_qd.d3_qr2;
inst->qd_xyz = d_qd.d3_pqr;

inst->DrC_x = d_DrCur.d1_p;
inst->DrC_y = d_DrCur.d1_q;
inst->DrC_z = d_DrCur.d1_r;
inst->DrC_x2 = d_DrCur.d2_p2;
inst->DrC_y2 = d_DrCur.d2_q2;
inst->DrC_z2 = d_DrCur.d2_r2;
inst->DrC_xy = d_DrCur.d2_pq;
inst->DrC_yz = d_DrCur.d2_qr;
inst->DrC_xz = d_DrCur.d2_pr;
inst->DrC_x3 = d_DrCur.d3_p3;
inst->DrC_y3 = d_DrCur.d3_q3;
inst->DrC_z3 = d_DrCur.d3_r3;
inst->DrC_x2z = d_DrCur.d3_p2r;
inst->DrC_x2y = d_DrCur.d3_p2q;
inst->DrC_y2z = d_DrCur.d3_q2r;
inst->DrC_xy2 = d_DrCur.d3_pq2;
inst->DrC_xz2 = d_DrCur.d3_pr2;
inst->DrC_yz2 = d_DrCur.d3_qr2;
inst->DrC_xyz = d_DrCur.d3_pqr;

inst->gbs1 = lgbs1;
inst->gbs2 = lgbs2;
inst->gbs3 = lgbs3;

inst->gbd1 = lgbd1;
inst->gbd2 = lgbd2;
inst->gbd3 = lgbd3;

} else {
        /*
         * inverse mode - source and drain interchanged
         * inversion equations for realqg and realqb are the 
         * same; a minus is added for the
         * realDrCur equation; 
         * realqd = -(realqb + realqg + fakeqd)
         */

inst->qg_x = -(-d_qg.d1_p);
inst->qg_y = -(-d_qg.d1_q);
inst->qg_z = -(d_qg.d1_p + d_qg.d1_q + d_qg.d1_r);
inst->qg_x2 = -(-d_qg.d2_p2);
inst->qg_y2 = -(-d_qg.d2_q2);
inst->qg_z2 = -(-(d_qg.d2_p2 + d_qg.d2_q2 + d_qg.d2_r2 + 2*(d_qg.d2_pq + d_qg.d2_pr + d_qg.d2_qr)));
inst->qg_xy = -(-d_qg.d2_pq);
inst->qg_yz = -(d_qg.d2_pq + d_qg.d2_q2 + d_qg.d2_qr);
inst->qg_xz = -(d_qg.d2_p2 + d_qg.d2_pq + d_qg.d2_pr);
inst->qg_x3 = -(-d_qg.d3_p3);
inst->qg_y3 = -(-d_qg.d3_q3);
inst->qg_z3 = -(d_qg.d3_p3 + d_qg.d3_q3 + d_qg.d3_r3 + 3*(d_qg.d3_p2q + d_qg.d3_p2r + d_qg.d3_pq2 + d_qg.d3_q2r + d_qg.d3_pr2 + d_qg.d3_qr2) + 6*d_qg.d3_pqr );
inst->qg_x2z = -(d_qg.d3_p3 + d_qg.d3_p2q + d_qg.d3_p2r);
inst->qg_x2y = -(-d_qg.d3_p2q);
inst->qg_y2z = -(d_qg.d3_pq2 + d_qg.d3_q3 + d_qg.d3_q2r);
inst->qg_xy2 = -(-d_qg.d3_pq2);
inst->qg_xz2 = -(-(d_qg.d3_p3 + 2*(d_qg.d3_p2q + d_qg.d3_p2r + d_qg.d3_pqr) + d_qg.d3_pq2 + d_qg.d3_pr2));
inst->qg_yz2 = -(-(d_qg.d3_q3 + 2*(d_qg.d3_pq2 + d_qg.d3_q2r + d_qg.d3_pqr) + d_qg.d3_p2q + d_qg.d3_qr2));
inst->qg_xyz = -(d_qg.d3_p2q + d_qg.d3_pq2 + d_qg.d3_pqr);

inst->qb_x = -(-d_qb.d1_p);
inst->qb_y = -(-d_qb.d1_q);
inst->qb_z = -(d_qb.d1_p + d_qb.d1_q + d_qb.d1_r);
inst->qb_x2 = -(-d_qb.d2_p2);
inst->qb_y2 = -(-d_qb.d2_q2);
inst->qb_z2 = -(-(d_qb.d2_p2 + d_qb.d2_q2 + d_qb.d2_r2 + 2*(d_qb.d2_pq + d_qb.d2_pr + d_qb.d2_qr)));
inst->qb_xy = -(-d_qb.d2_pq);
inst->qb_yz = -(d_qb.d2_pq + d_qb.d2_q2 + d_qb.d2_qr);
inst->qb_xz = -(d_qb.d2_p2 + d_qb.d2_pq + d_qb.d2_pr);
inst->qb_x3 = -(-d_qb.d3_p3);
inst->qb_y3 = -(-d_qb.d3_q3);
inst->qb_z3 = -(d_qb.d3_p3 + d_qb.d3_q3 + d_qb.d3_r3 + 3*(d_qb.d3_p2q + d_qb.d3_p2r + d_qb.d3_pq2 + d_qb.d3_q2r + d_qb.d3_pr2 + d_qb.d3_qr2) + 6*d_qb.d3_pqr );
inst->qb_x2z = -(d_qb.d3_p3 + d_qb.d3_p2q + d_qb.d3_p2r);
inst->qb_x2y = -(-d_qb.d3_p2q);
inst->qb_y2z = -(d_qb.d3_pq2 + d_qb.d3_q3 + d_qb.d3_q2r);
inst->qb_xy2 = -(-d_qb.d3_pq2);
inst->qb_xz2 = -(-(d_qb.d3_p3 + 2*(d_qb.d3_p2q + d_qb.d3_p2r + d_qb.d3_pqr) + d_qb.d3_pq2 + d_qb.d3_pr2));
inst->qb_yz2 = -(-(d_qb.d3_q3 + 2*(d_qb.d3_pq2 + d_qb.d3_q2r + d_qb.d3_pqr) + d_qb.d3_p2q + d_qb.d3_qr2));
inst->qb_xyz = -(d_qb.d3_p2q + d_qb.d3_pq2 + d_qb.d3_pqr);

inst->qd_x= -inst->qg_x - inst->qb_x +(-d_qd.d1_p);
inst->qd_y= -inst->qg_y - inst->qb_y +(-d_qd.d1_q);
inst->qd_z= -inst->qg_z - inst->qb_z +(d_qd.d1_p + d_qd.d1_q + d_qd.d1_r);
inst->qd_x2 = -inst->qg_x2 - inst->qb_x2 +(-d_qd.d2_p2);
inst->qd_y2 = -inst->qg_y2 - inst->qb_y2 +(-d_qd.d2_q2);
inst->qd_z2 = -inst->qg_z2 - inst->qb_z2 +(-(d_qd.d2_p2 + d_qd.d2_q2 + d_qd.d2_r2 + 2*(d_qd.d2_pq + d_qd.d2_pr + d_qd.d2_qr)));
inst->qd_xy = -inst->qg_xy - inst->qb_xy +(-d_qd.d2_pq);
inst->qd_yz = -inst->qg_yz - inst->qb_yz +(d_qd.d2_pq + d_qd.d2_q2 + d_qd.d2_qr);
inst->qd_xz = -inst->qg_xz - inst->qb_xz +(d_qd.d2_p2 + d_qd.d2_pq + d_qd.d2_pr);
inst->qd_x3 = -inst->qg_x3 - inst->qb_x3 +(-d_qd.d3_p3);
inst->qd_y3 = -inst->qg_y3 - inst->qb_y3 +(-d_qd.d3_q3);
inst->qd_z3 = -inst->qg_z3 - inst->qb_z3 +(d_qd.d3_p3 + d_qd.d3_q3 + d_qd.d3_r3 + 3*(d_qd.d3_p2q + d_qd.d3_p2r + d_qd.d3_pq2 + d_qd.d3_q2r + d_qd.d3_pr2 + d_qd.d3_qr2) + 6*d_qd.d3_pqr );
inst->qd_x2z = -inst->qg_x2z - inst->qb_x2z +(d_qd.d3_p3 + d_qd.d3_p2q + d_qd.d3_p2r);
inst->qd_x2y = -inst->qg_x2y - inst->qb_x2y +(-d_qd.d3_p2q);
inst->qd_y2z = -inst->qg_y2z - inst->qb_y2z +(d_qd.d3_pq2 + d_qd.d3_q3 + d_qd.d3_q2r);
inst->qd_xy2 = -inst->qg_xy2 - inst->qb_xy2 +(-d_qd.d3_pq2);
inst->qd_xz2 = -inst->qg_xz2 - inst->qb_xz2 +(-(d_qd.d3_p3 + 2*(d_qd.d3_p2q + d_qd.d3_p2r + d_qd.d3_pqr) + d_qd.d3_pq2 + d_qd.d3_pr2));
inst->qd_yz2 = -inst->qg_yz2 - inst->qb_yz2 +(-(d_qd.d3_q3 + 2*(d_qd.d3_pq2 + d_qd.d3_q2r + d_qd.d3_pqr) + d_qd.d3_p2q + d_qd.d3_qr2));
inst->qd_xyz = -inst->qg_xyz - inst->qb_xyz +(d_qd.d3_p2q + d_qd.d3_pq2 + d_qd.d3_pqr);

inst->DrC_x = -d_DrCur.d1_p;
inst->DrC_y = -d_DrCur.d1_q;
inst->DrC_z = d_DrCur.d1_p + d_DrCur.d1_q + d_DrCur.d1_r;
inst->DrC_x2 = -d_DrCur.d2_p2;
inst->DrC_y2 = -d_DrCur.d2_q2;
inst->DrC_z2 = -(d_DrCur.d2_p2 + d_DrCur.d2_q2 + d_DrCur.d2_r2 + 2*(d_DrCur.d2_pq + d_DrCur.d2_pr + d_DrCur.d2_qr));
inst->DrC_xy = -d_DrCur.d2_pq;
inst->DrC_yz = d_DrCur.d2_pq + d_DrCur.d2_q2 + d_DrCur.d2_qr;
inst->DrC_xz = d_DrCur.d2_p2 + d_DrCur.d2_pq + d_DrCur.d2_pr;
inst->DrC_x3 = -d_DrCur.d3_p3;
inst->DrC_y3 = -d_DrCur.d3_q3;
inst->DrC_z3 = d_DrCur.d3_p3 + d_DrCur.d3_q3 + d_DrCur.d3_r3 + 3*(d_DrCur.d3_p2q + d_DrCur.d3_p2r + d_DrCur.d3_pq2 + d_DrCur.d3_q2r + d_DrCur.d3_pr2 + d_DrCur.d3_qr2) + 6*d_DrCur.d3_pqr ;
inst->DrC_x2z = d_DrCur.d3_p3 + d_DrCur.d3_p2q + d_DrCur.d3_p2r;
inst->DrC_x2y = -d_DrCur.d3_p2q;
inst->DrC_y2z = d_DrCur.d3_pq2 + d_DrCur.d3_q3 + d_DrCur.d3_q2r;
inst->DrC_xy2 = -d_DrCur.d3_pq2;
inst->DrC_xz2 = -(d_DrCur.d3_p3 + 2*(d_DrCur.d3_p2q + d_DrCur.d3_p2r + d_DrCur.d3_pqr) + d_DrCur.d3_pq2 + d_DrCur.d3_pr2);
inst->DrC_yz2 = -(d_DrCur.d3_q3 + 2*(d_DrCur.d3_pq2 + d_DrCur.d3_q2r + d_DrCur.d3_pqr) + d_DrCur.d3_p2q + d_DrCur.d3_qr2);
inst->DrC_xyz = d_DrCur.d3_p2q + d_DrCur.d3_pq2 + d_DrCur.d3_pqr;

inst->gbs1 = lgbd1;
inst->gbs2 = lgbd2;
inst->gbs3 = lgbd3;

inst->gbd1 = lgbs1;
inst->gbd2 = lgbs2;
inst->gbd3 = lgbs3;
}

/* now to adjust for type and multiply by factors to convert to Taylor coeffs. */

inst->qg_x2 = 0.5*model->B1type*inst->qg_x2;
inst->qg_y2 = 0.5*model->B1type*inst->qg_y2;
inst->qg_z2 = 0.5*model->B1type*inst->qg_z2;
inst->qg_xy = model->B1type*inst->qg_xy;
inst->qg_yz = model->B1type*inst->qg_yz;
inst->qg_xz = model->B1type*inst->qg_xz;
inst->qg_x3 = inst->qg_x3/6.;
inst->qg_y3 = inst->qg_y3/6.;
inst->qg_z3 = inst->qg_z3/6.;
inst->qg_x2z = 0.5*inst->qg_x2z;
inst->qg_x2y = 0.5*inst->qg_x2y;
inst->qg_y2z = 0.5*inst->qg_y2z;
inst->qg_xy2 = 0.5*inst->qg_xy2;
inst->qg_xz2 = 0.5*inst->qg_xz2;
inst->qg_yz2 = 0.5*inst->qg_yz2;

inst->qb_x2 = 0.5*model->B1type*inst->qb_x2;
inst->qb_y2 = 0.5*model->B1type*inst->qb_y2;
inst->qb_z2 = 0.5*model->B1type*inst->qb_z2;
inst->qb_xy = model->B1type*inst->qb_xy;
inst->qb_yz = model->B1type*inst->qb_yz;
inst->qb_xz = model->B1type*inst->qb_xz;
inst->qb_x3 = inst->qb_x3/6.;
inst->qb_y3 = inst->qb_y3/6.;
inst->qb_z3 = inst->qb_z3/6.;
inst->qb_x2z = 0.5*inst->qb_x2z;
inst->qb_x2y = 0.5*inst->qb_x2y;
inst->qb_y2z = 0.5*inst->qb_y2z;
inst->qb_xy2 = 0.5*inst->qb_xy2;
inst->qb_xz2 = 0.5*inst->qb_xz2;
inst->qb_yz2 = 0.5*inst->qb_yz2;

inst->qd_x2 = 0.5*model->B1type*inst->qd_x2;
inst->qd_y2 = 0.5*model->B1type*inst->qd_y2;
inst->qd_z2 = 0.5*model->B1type*inst->qd_z2;
inst->qd_xy = model->B1type*inst->qd_xy;
inst->qd_yz = model->B1type*inst->qd_yz;
inst->qd_xz = model->B1type*inst->qd_xz;
inst->qd_x3 = inst->qd_x3/6.;
inst->qd_y3 = inst->qd_y3/6.;
inst->qd_z3 = inst->qd_z3/6.;
inst->qd_x2z = 0.5*inst->qd_x2z;
inst->qd_x2y = 0.5*inst->qd_x2y;
inst->qd_y2z = 0.5*inst->qd_y2z;
inst->qd_xy2 = 0.5*inst->qd_xy2;
inst->qd_xz2 = 0.5*inst->qd_xz2;
inst->qd_yz2 = 0.5*inst->qd_yz2;

inst->DrC_x2 = 0.5*model->B1type*inst->DrC_x2;
inst->DrC_y2 = 0.5*model->B1type*inst->DrC_y2;
inst->DrC_z2 = 0.5*model->B1type*inst->DrC_z2;
inst->DrC_xy = model->B1type*inst->DrC_xy;
inst->DrC_yz = model->B1type*inst->DrC_yz;
inst->DrC_xz = model->B1type*inst->DrC_xz;
inst->DrC_x3 = inst->DrC_x3/6.;
inst->DrC_y3 = inst->DrC_y3/6.;
inst->DrC_z3 = inst->DrC_z3/6.;
inst->DrC_x2z = 0.5*inst->DrC_x2z;
inst->DrC_x2y = 0.5*inst->DrC_x2y;
inst->DrC_y2z = 0.5*inst->DrC_y2z;
inst->DrC_xy2 = 0.5*inst->DrC_xy2;
inst->DrC_xz2 = 0.5*inst->DrC_xz2;
inst->DrC_yz2 = 0.5*inst->DrC_yz2;

inst->gbs2 = model->B1type*inst->gbs2;
inst->gbd2 = model->B1type*inst->gbd2;
        }   /* End of Mosfet Instance */

    }       /* End of Model Instance */
    return(OK);
}
