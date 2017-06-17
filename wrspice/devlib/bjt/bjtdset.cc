
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Device Library                                                         *
 *                                                                        *
 *========================================================================*
 $Id: bjtdset.cc,v 1.0 1998/01/30 05:26:31 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1988 Jaijeet S Roychowdhury
         1992 Stephen R. Whiteley
****************************************************************************/

#define DISTO
#include "bjtdefs.h"
#include "distdefs.h"

//
// This function initialises the Taylor coeffs for the
// BJT's in the circuit (special for disto analysis).
//
int
BJTdev::dSetup(sBJTmodel *model, sCKT *ckt)
{
    sBJTinstance *inst;
    sGENinstance *geninst;
    double arg;
    double c2;
    double c4;
    double lcapbe1,lcapbe2,lcapbe3;
    double lcapbx1,lcapbx2,lcapbx3;
    double cb = 0;
    double cbc;
    double cbcn;
    double cbe;
    double cben;
    double cdis;
    double csat;
    double ctot;
    double czbc;
    double czbcf2;
    double czbe;
    double czbef2;
    double czbx;
    double czbxf2;
    double czcs;
    double evbc;
    double evbcn;
    double evbe;
    double evben;
    double f1;
    double f2;
    double f3;
    double fcpc;
    double fcpe;
    double gbb1 = 0;
    double gbc;
    double gbcn;
    double gbe;
    double gbe2,gbe3;
    double gbc2,gbc3;
    double gben2,gben3;
    double gbcn2,gbcn3;
    double gben;
    double gbb2 = 0, gbb3 = 0;
    double oik;
    double oikr;
    double ovtf;
    double pc;
    double pe;
    double ps;
    double q1;
    double q2;
    double qb;
    double rbpi;
    double rbpr;
    double sarg;
    double sqarg;
    double tf;
    double tr;
    double vbc;
    double vbe;
    double vbx;
    double vsc;
    double vt;
    double vtc;
    double vte;
    double vtn;
    double xjrb;
    double xjtf;
    double xmc;
    double xme;
    double xms;
    double xtf;
    double vbed;
    double vbb;

    double lcapbc1 = 0.0;
    double lcapbc2 = 0.0;
    double lcapbc3 = 0.0;

    double lcapsc1 = 0.0;
    double lcapsc2 = 0.0;
    double lcapsc3 = 0.0;
    double ic;
    double dummy;
    Dderivs d_p, d_q, d_r;
    Dderivs d_dummy, d_q1, d_qb, d_dummy2;
    Dderivs d_arg, d_sqarg, d_ic, d_q2;
    Dderivs d_z, d_tanz, d_vbb, d_ibb, d_rbb;
    Dderivs d_ib, d_cbe, d_tff, d_qbe;


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
    d_p.d3_p2q = 0.0;
    d_p.d3_p2r = 0.0;
    d_p.d3_pq2 = 0.0;
    d_p.d3_q2r = 0.0;
    d_p.d3_pr2 = 0.0;
    d_p.d3_qr2 = 0.0;
    d_p.d3_pqr = 0.0;

    EqualDeriv(&d_q, &d_p);
    d_q.d1_q = 1.0;
    d_q.d1_p = 0.0;

    EqualDeriv(&d_r, &d_p);
    d_r.d1_r = 1.0;
    d_r.d1_p = 0.0;

    for ( ; model != 0; model = (sBJTmodel*)model->GENnextModel) {
        for (geninst = model->GENinstances; geninst != 0; 
                geninst = geninst->GENnextInstance) {
            inst = (sBJTinstance*)geninst;
            
            vt = inst->BJTtemp * CONSTKoverQ;


            //
            //   dc model paramters
            //
            csat=inst->BJTtSatCur*inst->BJTarea;
            rbpr=model->BJTminBaseResist/inst->BJTarea;
            rbpi=model->BJTbaseResist/inst->BJTarea-rbpr;
            oik=model->BJTinvRollOffF/inst->BJTarea;
            c2=inst->BJTtBEleakCur*inst->BJTarea;
            vte=model->BJTleakBEemissionCoeff*vt;
            oikr=model->BJTinvRollOffR/inst->BJTarea;
            c4=inst->BJTtBCleakCur*inst->BJTarea;
            vtc=model->BJTleakBCemissionCoeff*vt;
            xjrb=model->BJTbaseCurrentHalfResist*inst->BJTarea;


            //
            //   initialization
            //
            vbe= model->BJTtype*(*(ckt->CKTrhsOld + inst->BJTbasePrimeNode) -
                *(ckt->CKTrhsOld + inst->BJTemitPrimeNode));
            vbc= model->BJTtype*(*(ckt->CKTrhsOld + inst->BJTbaseNode) -
                *(ckt->CKTrhsOld + inst->BJTcolPrimeNode));
            vbx=model->BJTtype*(
                *(ckt->CKTrhsOld+inst->BJTbaseNode)-
                *(ckt->CKTrhsOld+inst->BJTcolPrimeNode));
            vsc=model->BJTtype*(
                *(ckt->CKTrhsOld+inst->BJTsubstNode)-
                *(ckt->CKTrhsOld+inst->BJTcolPrimeNode));

            vbb=model->BJTtype*(
                *(ckt->CKTrhsOld+inst->BJTbaseNode) -
                *(ckt->CKTrhsOld+inst->BJTbasePrimeNode));

            vbed = vbe;
                // this is just a dummy variable
                // it is the delayed vbe to be
                // used in the delayed gm generator 
                //

            // ic = f1(vbe,vbc,vbed) + f2(vbc) + f3(vbc)
            // 
            //  we shall calculate the taylor coeffs of
            // ic wrt vbe, vbed, and vbc and store them away.
            // the equations f1 f2 and f3 are given elsewhere;
            // we shall start off with f1, compute
            // derivs. upto third order and then do f2 and
            // f3 and add their derivatives.
            //
            // Since f1 above is a function of three variables, it
            // will be convenient to use derivative structures
            // to compute the derivatives of f1. For this 
            // computation, p=vbe, q=vbc, r=vbed.
            //
            // ib = f1(vbe) + f2(vbc) (not the same f's as
            //               above, in case you are
            //               wondering!)
            // the gbe's gbc's gben's and gbcn's are
            // convenient subsidiary variables.
            //
            // irb = f(vbe, vbc, vbb) - the vbe & vbc
            //         dependencies arise from the
            //     qb term.
            // qbe = f1(vbe,vbc) + f2(vbe)
            //
            // derivative structures will be used again in the
            // above two equations. p=vbe, q=vbc, r=vbb.
            //
            // qbc = f(vbc) ; qbx = f(vbx)
            //
            // qss = f(vsc)
            //
            //
            //   determine dc current and derivitives
            //
            vtn=vt*model->BJTemissionCoeffF;
            if(vbe > -5*vtn){
                evbe=exp(vbe/vtn);
                cbe=csat*(evbe-1)+ckt->CKTcurTask->TSKgmin*vbe;
                gbe=csat*evbe/vtn+ckt->CKTcurTask->TSKgmin;
                gbe2 = csat*evbe/vtn/vtn;
                gbe3 = gbe2/vtn;

                // note - these are actually derivs, not Taylor
                // coeffs. - not divided by 2! and 3!
                //
                if (c2 == 0) {
                    cben=0;
                    gben=gben2=gben3=0;
                }
                else {
                    evben=exp(vbe/vte);
                    cben=c2*(evben-1);
                    gben=c2*evben/vte;
                    gben2=gben/vte;
                    gben3=gben2/vte;
                }
            }
            else {
                gbe = -csat/vbe+ckt->CKTcurTask->TSKgmin;
                gbe2=gbe3=gben2=gben3=0;
                cbe=gbe*vbe;
                gben = -c2/vbe;
                cben=gben*vbe;
            }
            vtn=vt*model->BJTemissionCoeffR;
            if(vbc > -5*vtn) {
                evbc=exp(vbc/vtn);
                cbc=csat*(evbc-1)+ckt->CKTcurTask->TSKgmin*vbc;
                gbc=csat*evbc/vtn+ckt->CKTcurTask->TSKgmin;
                gbc2=csat*evbc/vtn/vtn;
                gbc3=gbc2/vtn;
                if (c4 == 0) {
                    cbcn=0;
                    gbcn=0;
                    gbcn2=gbcn3=0;
                }
                else {
                    evbcn=exp(vbc/vtc);
                    cbcn=c4*(evbcn-1);
                    gbcn=c4*evbcn/vtc;
                    gbcn2=gbcn/vtc;
                    gbcn3=gbcn2/vtc;
                }
            }
            else {
                gbc = -csat/vbc+ckt->CKTcurTask->TSKgmin;
                gbc2=gbc3=0;
                cbc = gbc*vbc;
                gbcn = -c4/vbc;
                gbcn2=gbcn3=0;
                cbcn=gbcn*vbc;
            }
            //
            //   determine base charge terms
            //
            // q1 is a function of 2 variables p=vbe and q=vbc. r=
            // anything
            //
            q1=1/(1-model->BJTinvEarlyVoltF*vbc-model->BJTinvEarlyVoltR*vbe);
            dummy = (1-model->BJTinvEarlyVoltF*vbc-
                model->BJTinvEarlyVoltR*vbe);
            EqualDeriv(&d_dummy, &d_p);
            d_dummy.value = dummy;
            d_dummy.d1_p = - model->BJTinvEarlyVoltR;
            d_dummy.d1_q = -  model->BJTinvEarlyVoltF;
                // q1 = 1/dummy
            InvDeriv(&d_q1, &d_dummy);
            // now q1 and its derivatives are set up
            if(oik == 0 && oikr == 0) {
                qb=q1;
                EqualDeriv(&d_qb, &d_q1);
            }
            else {
                q2=oik*cbe+oikr*cbc;
                EqualDeriv(&d_q2, &d_p);
                d_q2.value = q2;
                d_q2.d1_p = oik*gbe;
                d_q2.d1_q = oikr*gbc;
                d_q2.d2_p2 = oik*gbe2;
                d_q2.d2_q2 = oikr*gbc2;
                d_q2.d3_p3 = oik*gbe3;
                d_q2.d3_q3 = oikr*gbc3;
                arg=SPMAX(0,1+4*q2);
                if (arg == 0.) {
                    EqualDeriv(&d_arg,&d_p);
                    d_arg.d1_p = 0.0;
                }
                else {
                    TimesDeriv(&d_arg,&d_q2,4.0);
                    d_arg.value += 1.;
                }
                sqarg=1;
                EqualDeriv(&d_sqarg,&d_p);
                d_sqarg.value = 1.0;
                d_sqarg.d1_p = 0.0;
                if(arg != 0){
                    sqarg=sqrt(arg);
                    SqrtDeriv(&d_sqarg, &d_arg);
                }

                qb=q1*(1+sqarg)/2;
                dummy = 1 + sqarg;
                EqualDeriv(&d_dummy, &d_sqarg);
                d_dummy.value += 1.0;
                MultDeriv(&d_qb, &d_q1, &d_dummy);
                TimesDeriv(&d_qb, &d_qb, 0.5);
            }

            ic = (cbe - cbc)/qb;
            // cbe is a fn of vbed only; cbc of vbc; and qb of vbe and vbc
            // p=vbe, q=vbc, r=vbed; now dummy = cbe - cbc
            EqualDeriv(&d_dummy, &d_p);
            d_dummy.d1_p = 0.0;
            d_dummy.value = cbe-cbc;
            d_dummy.d1_r = gbe;
            d_dummy.d2_r2 = gbe2;
            d_dummy.d3_r3 = gbe3;
            d_dummy.d1_q = -gbc;
            d_dummy.d2_q2 = -gbc2;
            d_dummy.d3_q3 = -gbc3;

            DivDeriv(&d_ic, &d_dummy, &d_qb);


            d_ic.value -= cbc/inst->BJTtBetaR + cbcn;
            d_ic.d1_q -= gbc/inst->BJTtBetaR + gbcn;
            d_ic.d2_q2 -= gbc2/inst->BJTtBetaR + gbcn2;
            d_ic.d3_q3 -= gbc3/inst->BJTtBetaR + gbcn3;

            // check this point: where is the f2(vbe) contribution to ic ?

            // ic derivatives all set up now
            // base spread resistance

            if ( !((rbpr == 0.0) && (rbpi == 0.0))) {
                cb=cbe/inst->BJTtBetaF+cben+cbc/inst->BJTtBetaR+cbcn;
                // we are calculating derivatives w.r.t cb itself
                /*
                gx=rbpr+rbpi/qb;
                */

                if (cb != 0.0) {
                    if((xjrb != 0.0) && (rbpi != 0.0)) {
                        // p = ib, q, r = anything
                        dummy=SPMAX(cb/xjrb,1e-9);
                        EqualDeriv(&d_dummy, &d_p);
                        d_dummy.value = dummy;
                        d_dummy.d1_p = 1/xjrb;
                        SqrtDeriv(&d_dummy, &d_dummy);
                        TimesDeriv(&d_dummy, &d_dummy, 2.4317); 

                        /*
                        dummy2=(-1+sqrt(1+14.59025*SPMAX(cb/xjrb,1e-9)));
                        */
                        EqualDeriv(&d_dummy2, &d_p);
                        d_dummy2.value = 1+14.59025*SPMAX(cb/xjrb,1e-9);
                        d_dummy2.d1_p = 14.59025/xjrb;
                        SqrtDeriv(&d_dummy2, &d_dummy2);
                        d_dummy2.value -= 1.0;

                        DivDeriv(&d_z, &d_dummy2, &d_dummy);
                        TanDeriv(&d_tanz, &d_z);

                        // now using dummy = tanz - z and dummy2 = z*tanz*tanz
                        TimesDeriv(&d_dummy, &d_z, -1.0);
                        PlusDeriv(&d_dummy, &d_dummy, &d_tanz);

                        MultDeriv(&d_dummy2, &d_tanz, &d_tanz);
                        MultDeriv(&d_dummy2, &d_dummy2, &d_z);

                        DivDeriv(&d_rbb , &d_dummy, &d_dummy2);
                        TimesDeriv(&d_rbb,&d_rbb, 3.0*rbpi);
                        d_rbb.value += rbpr;

                        MultDeriv(&d_vbb, &d_rbb, &d_p);

                        // power series inversion to get the conductance
                        // 'derivatives

                        if (d_vbb.d1_p != 0) {
                            gbb1 = 1/d_vbb.d1_p;
                            gbb2 = -(d_vbb.d2_p2*0.5)*gbb1*gbb1;
                            gbb3 = gbb1*gbb1*gbb1*gbb1*(-(d_vbb.d3_p3/6.0)
                                + 2*(d_vbb.d2_p2*0.5)*(d_vbb.d2_p2*0.5)*gbb1);
                        }
                        /*
                        else
                            printf(
                "\nd_vbb.d1_p = 0 in base spread resistance calculations\n");
                        */

                        // r = vbb
                        EqualDeriv(&d_ibb, &d_r);
                        d_ibb.value = cb;
                        d_ibb.d1_r = gbb1;
                        d_ibb.d2_r2 = 2*gbb2;
                        d_ibb.d3_r3 = 6.0*gbb3;
                    }
                    else {
                         /*
                         rbb = rbpr + rbpi/qb;
                         ibb = vbb /rbb; = f(vbe, vbc, vbb)
                         */

                         EqualDeriv(&d_rbb,&d_p);
                         d_rbb.d1_p = 0.0;
                         if (rbpi != 0.0) {
                             InvDeriv(&d_rbb, &d_qb);
                             TimesDeriv(&d_rbb, &d_rbb,rbpi);
                         }
                         d_rbb.value += rbpr;

                         EqualDeriv(&d_ibb,&d_r);
                         d_ibb.value = vbb;
                         DivDeriv(&d_ibb,&d_ibb,&d_rbb);
                    }
                }
                else {
                    EqualDeriv(&d_ibb,&d_r);
                    if (rbpr != 0.0)
                        d_ibb.d1_r = 1/rbpr;
                }
            }
            else {
                EqualDeriv(&d_ibb,&d_p);
                d_ibb.d1_p = 0.0;
            }

            // formulae for base spread resistance over!

            // ib term

            EqualDeriv(&d_ib, &d_p);
            d_ib.d1_p = 0.0;
            d_ib.value = cb;
            d_ib.d1_p = gbe/inst->BJTtBetaF + gben;
            d_ib.d2_p2 = gbe2/inst->BJTtBetaF + gben2;
            d_ib.d3_p3 = gbe3/inst->BJTtBetaF + gben3;

            d_ib.d1_q = gbc/inst->BJTtBetaR + gbcn;
            d_ib.d2_q2 = gbc2/inst->BJTtBetaR + gbcn2;
            d_ib.d3_q3 = gbc3/inst->BJTtBetaR + gbcn3;

            // ib term over
            //
            //   charge storage elements
            //
            tf=model->BJTtransitTimeF;
            tr=model->BJTtransitTimeR;
            czbe=inst->BJTtBEcap*inst->BJTarea;
            pe=inst->BJTtBEpot;
            xme=model->BJTjunctionExpBE;
            cdis=model->BJTbaseFractionBCcap;
            ctot=inst->BJTtBCcap*inst->BJTarea;
            czbc=ctot*cdis;
            czbx=ctot-czbc;
            pc=inst->BJTtBCpot;
            xmc=model->BJTjunctionExpBC;
            fcpe=inst->BJTtDepCap;
            czcs=model->BJTcapCS*inst->BJTarea;
            ps=model->BJTpotentialSubstrate;
            xms=model->BJTexponentialSubstrate;
            xtf=model->BJTtransitTimeBiasCoeffF;
            ovtf=model->BJTtransitTimeVBCFactor;
            xjtf=model->BJTtransitTimeHighCurrentF*inst->BJTarea;
            if(tf != 0 && vbe >0) {
                EqualDeriv(&d_cbe, &d_p);
                d_cbe.value = cbe;
                d_cbe.d1_p = gbe;
                d_cbe.d2_p2 = gbe2;
                d_cbe.d3_p3 = gbe3;
                if(xtf != 0){
                    if(ovtf != 0) {
                        // dummy = exp ( vbc*ovtf)
                        EqualDeriv(&d_dummy, &d_q);
                        d_dummy.value = vbc*ovtf;
                        d_dummy.d1_q = ovtf;
                        ExpDeriv(&d_dummy, &d_dummy);
                    }
                    else {
                        EqualDeriv(&d_dummy,&d_p);
                        d_dummy.value = 1.0;
                        d_dummy.d1_p = 0.0;
                    }
                    if(xjtf != 0) {
                        EqualDeriv(&d_dummy2, &d_cbe);
                        d_dummy2.value += xjtf;
                        DivDeriv(&d_dummy2, &d_cbe, &d_dummy2);
                        MultDeriv (&d_dummy2, &d_dummy2, &d_dummy2);
                    }
                    else {
                        EqualDeriv(&d_dummy2,&d_p);
                        d_dummy2.value = 1.0;
                        d_dummy2.d1_p = 0.0;
                    }
                
                    MultDeriv(&d_tff, &d_dummy, &d_dummy2);
                    TimesDeriv(&d_tff, &d_tff, tf*xtf);
                    d_tff.value += tf;
                }
                else {
                    EqualDeriv(&d_tff,&d_p);
                    d_tff.value = tf;
                    d_tff.d1_p = 0.0;
                }

                // qbe = tff/qb*cbe

                /*
                dummy = tff/qb;
                */
                // these are the cbe coeffs
                DivDeriv(&d_dummy, &d_tff, &d_qb);
                MultDeriv(&d_qbe, &d_dummy, &d_cbe);

            }
            else {
                EqualDeriv(&d_qbe, &d_p);
                d_qbe.value = 0.0;
                d_qbe.d1_p = 0.0;
            }
            if (vbe < fcpe) {
                arg=1-vbe/pe;
                sarg=exp(-xme*log(arg));
                lcapbe1 = czbe*sarg;
                lcapbe2 =
                0.5*czbe*xme*sarg/(arg*pe);
                lcapbe3 =
                czbe*xme*(xme+1)*sarg/(arg*arg*pe*pe*6);
            }
            else {
                f1=inst->BJTtf1;
                f2=model->BJTf2;
                f3=model->BJTf3;
                czbef2=czbe/f2;
                lcapbe1 = czbef2*(f3+xme*vbe/pe);
                lcapbe2 = 0.5*xme*czbef2/pe;
                lcapbe3 = 0.0;
            }
            d_qbe.d1_p += lcapbe1;
            d_qbe.d2_p2 += lcapbe2*2.;
            d_qbe.d3_p3 += lcapbe3*6.;

            fcpc=inst->BJTtf4;
            f1=inst->BJTtf5;
            f2=model->BJTf6;
            f3=model->BJTf7;
            if (vbc < fcpc) {
                arg=1-vbc/pc;
                sarg=exp(-xmc*log(arg));
                lcapbc1 = czbc*sarg;
                lcapbc2 =
                0.5*czbc*xmc*sarg/(arg*pc);
                lcapbc3 =
                czbc*xmc*(xmc+1)*sarg/(arg*arg*pc*pc*6);
            }
            else {
                czbcf2=czbc/f2;
                lcapbc1 = czbcf2*(f3+xmc*vbc/pc);
                lcapbc2 = 0.5*xmc*czbcf2/pc;
                lcapbc3 = 0;
            }
            if(vbx < fcpc) {
                arg=1-vbx/pc;
                sarg=exp(-xmc*log(arg));
                lcapbx1 = czbx*sarg;
                lcapbx2 =
                0.5*czbx*xmc*sarg/(arg*pc);
                lcapbx3 =
                czbx*xmc*(xmc+1)*sarg/(arg*arg*pc*pc*6);
            }
            else {
                czbxf2=czbx/f2;
                lcapbx1 = czbxf2*(f3+xmc*vbx/pc);
                lcapbx2 = 0.5*xmc*czbxf2/pc;
                lcapbx3 = 0;
            }
            if(vsc < 0){
                arg=1-vsc/ps;
                sarg=exp(-xms*log(arg));
                lcapsc1 = czcs*sarg;
                lcapsc2 =
                0.5*czcs*xms*sarg/(arg*ps);
                lcapsc3 =
                czcs*xms*(xms+1)*sarg/(arg*arg*ps*ps*6);
            }
            else {
                lcapsc1 = czcs*(1+xms*vsc/ps);
                lcapsc2 = czcs*0.5*xms/ps;
                lcapsc3 = 0;
            }

            //
            //   store small-signal parameters
            //
            inst->ic_x = d_ic.d1_p;
            inst->ic_y = d_ic.d1_q;
            inst->ic_xd = d_ic.d1_r;
            inst->ic_x2 = 0.5*model->BJTtype*d_ic.d2_p2;
            inst->ic_y2 = 0.5*model->BJTtype*d_ic.d2_q2;
            inst->ic_w2 = 0.5*model->BJTtype*d_ic.d2_r2;
            inst->ic_xy = model->BJTtype*d_ic.d2_pq;
            inst->ic_yw = model->BJTtype*d_ic.d2_qr;
            inst->ic_xw = model->BJTtype*d_ic.d2_pr;
            inst->ic_x3 = d_ic.d3_p3/6.;
            inst->ic_y3 = d_ic.d3_q3/6.;
            inst->ic_w3 = d_ic.d3_r3/6.;
            inst->ic_x2w = 0.5*d_ic.d3_p2r;
            inst->ic_x2y = 0.5*d_ic.d3_p2q;
            inst->ic_y2w = 0.5*d_ic.d3_q2r;
            inst->ic_xy2 = 0.5*d_ic.d3_pq2;
            inst->ic_xw2 = 0.5*d_ic.d3_pr2;
            inst->ic_yw2 = 0.5*d_ic.d3_qr2;
            inst->ic_xyw = d_ic.d3_pqr;

            inst->ib_x = d_ib.d1_p;
            inst->ib_y = d_ib.d1_q;
            inst->ib_x2 = 0.5*model->BJTtype*d_ib.d2_p2;
            inst->ib_y2 = 0.5*model->BJTtype*d_ib.d2_q2;
            inst->ib_xy = model->BJTtype*d_ib.d2_pq;
            inst->ib_x3 = d_ib.d3_p3/6.;
            inst->ib_y3 = d_ib.d3_q3/6.;
            inst->ib_x2y = 0.5*d_ib.d3_p2q;
            inst->ib_xy2 = 0.5*d_ib.d3_pq2;

            inst->ibb_x = d_ibb.d1_p;
            inst->ibb_y = d_ibb.d1_q;
            inst->ibb_z = d_ibb.d1_r;
            inst->ibb_x2 = 0.5*model->BJTtype*d_ibb.d2_p2;
            inst->ibb_y2 = 0.5*model->BJTtype*d_ibb.d2_q2;
            inst->ibb_z2 = 0.5*model->BJTtype*d_ibb.d2_r2;
            inst->ibb_xy = model->BJTtype*d_ibb.d2_pq;
            inst->ibb_yz = model->BJTtype*d_ibb.d2_qr;
            inst->ibb_xz = model->BJTtype*d_ibb.d2_pr;
            inst->ibb_x3 = d_ibb.d3_p3/6.;
            inst->ibb_y3 = d_ibb.d3_q3/6.;
            inst->ibb_z3 = d_ibb.d3_r3/6.;
            inst->ibb_x2z = 0.5*d_ibb.d3_p2r;
            inst->ibb_x2y = 0.5*d_ibb.d3_p2q;
            inst->ibb_y2z = 0.5*d_ibb.d3_q2r;
            inst->ibb_xy2 = 0.5*d_ibb.d3_pq2;
            inst->ibb_xz2 = 0.5*d_ibb.d3_pr2;
            inst->ibb_yz2 = 0.5*d_ibb.d3_qr2;
            inst->ibb_xyz = d_ibb.d3_pqr;

            inst->qbe_x = d_qbe.d1_p;
            inst->qbe_y = d_qbe.d1_q;
            inst->qbe_x2 = 0.5*model->BJTtype*d_qbe.d2_p2;
            inst->qbe_y2 = 0.5*model->BJTtype*d_qbe.d2_q2;
            inst->qbe_xy = model->BJTtype*d_qbe.d2_pq;
            inst->qbe_x3 = d_qbe.d3_p3/6.;
            inst->qbe_y3 = d_qbe.d3_q3/6.;
            inst->qbe_x2y = 0.5*d_qbe.d3_p2q;
            inst->qbe_xy2 = 0.5*d_qbe.d3_pq2;

            inst->capbc1 = lcapbc1;
            inst->capbc2 = lcapbc2;
            inst->capbc3 = lcapbc3;

            inst->capbx1 = lcapbx1;
            inst->capbx2 = lcapbx2;
            inst->capbx3 = lcapbx3;

            inst->capsc1 = lcapsc1;
            inst->capsc2 = lcapsc2;
            inst->capsc3 = lcapsc3;
        }
    }
    return(OK);
}
