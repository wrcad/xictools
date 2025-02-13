
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

/**********
STAG version 2.6
Copyright 2000 owned by the United Kingdom Secretary of State for Defence
acting through the Defence Evaluation and Research Agency.
Developed by :     Jim Benson,
                   Department of Electronics and Computer Science,
                   University of Southampton,
                   United Kingdom.
With help from :   Nele D'Halleweyn, Bill Redman-White, and Craig Easson.

Based on STAG version 2.1
Developed by :     Mike Lee,
With help from :   Bernard Tenbroek, Bill Redman-White, Mike Uren, Chris Edwards
                   and John Bunyan.
Acknowledgements : Rupert Howes and Pete Mole.
**********/

/*
#include "spice.h"
#include "cktdefs.h"
#include "util.h"
#include "suffix.h"
#include "soi3defs.h"
#include "trandefs.h"
#include "const.h"
*/
#include "soi3defs.h"

// this is from spice3
#define MAX_EXP_ARG   709.0


void
/*
SOI3cap(vgB,Phiplusvsb,gammaB,
           paramargs,
           Bfargs,alpha_args,psi_st0args,
           vGTargs,
           psi_sLargs,psi_s0args,
           ldargs,
           Qg,Qb,Qd,QgB,
           cggf,cgd,cgs,cgdeltaT,
           cbgf,cbd,cbs,cbdeltaT,cbgb,
           cdgf,cdd,cds,cddeltaT,
           cgbgb,cgbsb
           )

double vgB,Phiplusvsb,gammaB;
double paramargs[10];
double Bfargs[2],alpha_args[5];
double psi_st0args[5];
double vGTargs[5];
double psi_sLargs[5],psi_s0args[5];
double ldargs[5];

double *Qg,*Qb,*Qd,*QgB;
double *cggf,*cgd,*cgs,*cgdeltaT;
double *cbgf,*cbd,*cbs,*cbdeltaT,*cbgb;
double *cdgf,*cdd,*cds,*cddeltaT;
double *cgbgb,*cgbsb;
*/

SOI3cap(double /*vgB*/,double /*Phiplusvsb*/,double /*gammaB*/,
   double *paramargs,
   double *Bfargs,double *alpha_args,double *psi_st0args,
   double *vGTargs,
   double *psi_sLargs,double *psi_s0args,
   double *ldargs,
   double *Qg,double *Qb,double *Qd,double *QgB,
   double *cggf,double *cgd,double *cgs,double *cgdeltaT,
   double *cbgf,double *cbd,double *cbs,double *cbdeltaT,double *cbgb,
   double *cdgf,double *cdd,double *cds,double *cddeltaT,
   double *cgbgb,double *cgbsb
   )


/****** Part 1 - declare local variables.  ******/

{
double WCox,/*WCob,*/L;
double gamma,/*eta_s,*/vt,delta,sigma,chiFB;
double Bf,pDBf_Dpsi_st0;
double alpha,Dalpha_Dvgfb,Dalpha_Dvdb,Dalpha_Dvsb,Dalpha_DdeltaT;
double Dpsi_st0_Dvgfb,Dpsi_st0_Dvdb,Dpsi_st0_Dvsb,Dpsi_st0_DdeltaT;
double vGT,DvGT_Dvgfb,DvGT_Dvdb,DvGT_Dvsb,DvGT_DdeltaT;
double psi_sL,Dpsi_sL_Dvgfb,Dpsi_sL_Dvdb,Dpsi_sL_Dvsb,Dpsi_sL_DdeltaT;
double psi_s0,Dpsi_s0_Dvgfb,Dpsi_s0_Dvdb,Dpsi_s0_Dvsb,Dpsi_s0_DdeltaT;
double ld,Dld_Dvgfb,Dld_Dvdb,Dld_Dvsb,Dld_DdeltaT;

double Lprime,Fc;
double Qbprime,Qcprime,Qdprime,Qgprime,Qb2prime,Qc2prime,Qd2prime,Qg2prime;
double Dlimc,Dlimd;
double Vqd,Vqs;
double F,F2;
double cq,dq;
double dercq,derdq;
double sigmaC,Eqc,Eqd;
double DVqs_Dvgfb,DVqs_Dvdb,DVqs_Dvsb,DVqs_DdeltaT;
double DF_Dvgfb,DF_Dvdb,DF_Dvsb,DF_DdeltaT;
double ccgf,ccd,ccs,ccdeltaT;

//double A0B,EA0B,tmpsb;
//double Vgmax0B,VgBx0,EBmax0,tmpmax0,tmpmaxsb;
//double VgBy0,EBy0,tmpmin0;
//double SgB0;

double vg,vgacc,Egacc,tmpacc,Qacc;
double csf;

/****** Part 2 - extract variables passed from soi3load(), which  ******/
/****** have been passed to soi3cap() in *arg arrays.             ******/

WCox  = paramargs[0];
L     = paramargs[1];
gamma = paramargs[2];
//eta_s = paramargs[3];
vt    = paramargs[4];
delta = paramargs[5];
//WCob  = paramargs[6];
sigma = paramargs[7];
chiFB = paramargs[8];
csf   = paramargs[9];
Bf            = Bfargs[0];
pDBf_Dpsi_st0 = Bfargs[1];
alpha          = alpha_args[0];
Dalpha_Dvgfb   = alpha_args[1];
Dalpha_Dvdb    = alpha_args[2];
Dalpha_Dvsb    = alpha_args[3];
Dalpha_DdeltaT = alpha_args[4];

Dpsi_st0_Dvgfb   = psi_st0args[1];
Dpsi_st0_Dvdb    = psi_st0args[2];
Dpsi_st0_Dvsb    = psi_st0args[3];
Dpsi_st0_DdeltaT = psi_st0args[4];

vGT          = vGTargs[0];
DvGT_Dvgfb   = vGTargs[1];
DvGT_Dvdb    = vGTargs[2];
DvGT_Dvsb    = vGTargs[3];
DvGT_DdeltaT = vGTargs[4];

psi_sL          = psi_sLargs[0];
Dpsi_sL_Dvgfb   = psi_sLargs[1];
Dpsi_sL_Dvdb    = psi_sLargs[2];
Dpsi_sL_Dvsb    = psi_sLargs[3];
Dpsi_sL_DdeltaT = psi_sLargs[4];
psi_s0          = psi_s0args[0];
Dpsi_s0_Dvgfb   = psi_s0args[1];
Dpsi_s0_Dvdb    = psi_s0args[2];
Dpsi_s0_Dvsb    = psi_s0args[3];
Dpsi_s0_DdeltaT = psi_s0args[4];
ld          = ldargs[0];
Dld_Dvgfb   = ldargs[1];
Dld_Dvdb    = ldargs[2];
Dld_Dvsb    = ldargs[3];
Dld_DdeltaT = ldargs[4];


/****** Part 3 - define some important quantities.  ******/

sigmaC = 1E-8;

Vqd = (vGT - alpha*psi_sL); /* This is -qd/Cof */
Vqs = (vGT - alpha*psi_s0); /* This is -qs/Cof */
if (Vqs<=0) { /* deep subthreshold contingency */
  F = 1;
} else {
  F = Vqd/Vqs;
  if (F<0) { /* physically impossible situation */
    F=0;
  }
}
F2 = F*F;

Fc = 1 + ld/L;
Lprime = L/Fc;


/****** Part 4 - calculate normalised (see note below) terminal  ******/
/****** charge expressions for the GCA region.                   ******/

/* JimB - important note */
/* The charge expressions Qcprime, Qd2prime etc in this file are not charges */
/* but voltages! Each expression is equal to the derived expression for the  */
/* total charge in each region, but divided by a factor WL'Cof.  This is     */
/* compensated for later on. */

/* Channel charge Qc1 */
cq = (F*F + F + 1)/(F+1);
Qcprime = -2*Vqs*cq/3;

if ((-Qcprime/sigmaC)<MAX_EXP_ARG) {
  Eqc = exp(-Qcprime/sigmaC);
  Qcprime = -sigmaC*log(1 + Eqc);
  Dlimc = Eqc/(1+Eqc);
} else {
  Dlimc = 1;
}

/* Drain charge Qd1 */
dq = (3*F2*F + 6*F2 + 4*F + 2)/((1+F)*(1+F));
Qdprime = -2*Vqs*dq/15;

if((-Qdprime/sigmaC)<MAX_EXP_ARG) {
  Eqd = exp(-Qdprime/sigmaC);
  Qdprime = -sigmaC*log(1 + Eqd);
  Dlimd = Eqd/(1+Eqd);
} else {
  Dlimd = 1;
}

/* Body charge Qb1 */
Qbprime = -gamma*(Bf + (delta/alpha)*(vGT + Qcprime));

/* Gate charge Qg1 */
Qgprime = -Qcprime-Qbprime;


/****** Part 5 - calculate capacitances and transcapacitances  ******/
/****** for the GCA region.  For the moment, we are not taking ******/
/****** account of the bias dependence of ld and Lprime.  This ******/
/****** will be done in Part 8, when both GCA and drain charge ******/
/****** terms will be included in the final capacitance        ******/
/****** expressions.                                           ******/

DVqs_Dvgfb = DvGT_Dvgfb - alpha*Dpsi_s0_Dvgfb - psi_s0*Dalpha_Dvgfb;
DVqs_Dvdb = DvGT_Dvdb - alpha*Dpsi_s0_Dvdb - psi_s0*Dalpha_Dvdb;
DVqs_Dvsb = DvGT_Dvsb - alpha*Dpsi_s0_Dvsb - psi_s0*Dalpha_Dvsb;
DVqs_DdeltaT = DvGT_DdeltaT - alpha*Dpsi_s0_DdeltaT - psi_s0*Dalpha_DdeltaT;

if (Vqs==0) {
  DF_Dvgfb = 0;
  DF_Dvdb = 0;
  DF_Dvsb = 0;
  DF_DdeltaT = 0;
} else {
  DF_Dvgfb = (DvGT_Dvgfb - alpha*Dpsi_sL_Dvgfb - psi_sL*Dalpha_Dvgfb -
              F*DVqs_Dvgfb)/Vqs;
  DF_Dvdb = (DvGT_Dvdb - alpha*Dpsi_sL_Dvdb - psi_sL*Dalpha_Dvdb -
              F*DVqs_Dvdb)/Vqs;
  DF_Dvsb = (DvGT_Dvsb - alpha*Dpsi_sL_Dvsb - psi_sL*Dalpha_Dvsb -
              F*DVqs_Dvsb)/Vqs;
  DF_DdeltaT = (DvGT_DdeltaT - alpha*Dpsi_sL_DdeltaT - psi_sL*Dalpha_DdeltaT -
              F*DVqs_DdeltaT)/Vqs;
}

dercq = F*(2+F)/((1+F)*(1+F));

ccgf = Dlimc*(-2*(DVqs_Dvgfb*cq + Vqs*dercq*DF_Dvgfb)/3);
ccd  = Dlimc*(-2*(DVqs_Dvdb*cq + Vqs*dercq*DF_Dvdb)/3);
ccs  = Dlimc*(-2*(DVqs_Dvsb*cq + Vqs*dercq*DF_Dvsb)/3);
ccdeltaT  = Dlimc*(-2*(DVqs_DdeltaT*cq + Vqs*dercq*DF_DdeltaT)/3);

derdq = F*(3*F2 + 9*F + 8)/((1+F)*(1+F)*(1+F));

*cdgf = Dlimd*(-2*(DVqs_Dvgfb * dq + Vqs*derdq*DF_Dvgfb)/15);
*cdd  = Dlimd*(-2*(DVqs_Dvdb * dq + Vqs*derdq*DF_Dvdb)/15);
*cds  = Dlimd*(-2*(DVqs_Dvsb * dq + Vqs*derdq*DF_Dvsb)/15);
*cddeltaT  = Dlimd*(-2*(DVqs_DdeltaT * dq + Vqs*derdq*DF_DdeltaT)/15);

/* JimB - note that for the following expressions, the Vx dependence of */
/* delta is accounted for by the term (vGT+Qcprime)*(Dalpha_Dvx/gamma). */

*cbgf = -gamma * (pDBf_Dpsi_st0*Dpsi_st0_Dvgfb +
                 (alpha*(delta*(DvGT_Dvgfb + ccgf) +
                    (vGT+Qcprime)*(Dalpha_Dvgfb/gamma)) -
                  delta*(vGT+Qcprime)*Dalpha_Dvgfb
                 )/(alpha*alpha)
                );
*cbd = -gamma * (pDBf_Dpsi_st0*Dpsi_st0_Dvdb +
                 (alpha*(delta*(DvGT_Dvdb + ccd) +
                    (vGT+Qcprime)*(Dalpha_Dvdb/gamma)) -
                  delta*(vGT+Qcprime)*Dalpha_Dvdb
                 )/(alpha*alpha)
                );
*cbs = -gamma * (pDBf_Dpsi_st0*Dpsi_st0_Dvsb +
                 (alpha*(delta*(DvGT_Dvsb + ccs) +
                    (vGT+Qcprime)*(Dalpha_Dvsb/gamma)) -
                  delta*(vGT+Qcprime)*Dalpha_Dvsb
                 )/(alpha*alpha)
                );
*cbdeltaT = -gamma * (pDBf_Dpsi_st0*Dpsi_st0_DdeltaT +
                 (alpha*(delta*(DvGT_DdeltaT + ccdeltaT) +
                    (vGT+Qcprime)*(Dalpha_DdeltaT/gamma)) -
                  delta*(vGT+Qcprime)*Dalpha_DdeltaT
                 )/(alpha*alpha)
                );


/****** Part 6 - Normalised expressions from part 4 are adjusted  ******/
/****** by WCox*Lprime, then accumulation charge is added to give ******/
/****** final expression for GCA region charges.                  ******/

/* Accumulation charge to be added to Qb */

vg = vGT + gamma*Bf;
if ((-vg/vt) > MAX_EXP_ARG) {
  vgacc = vg;
  tmpacc = 1;
} else {
  Egacc = exp(-vg/vt);
  vgacc = -vt*log(1+Egacc);
  tmpacc = Egacc/(1+Egacc);
}
Qacc = -WCox*L*vgacc;

/* Now work out GCA region charges */

*Qb = WCox*Lprime*Qbprime + Qacc;

*Qd = WCox*Lprime*Qdprime;

*Qg = WCox*Lprime*Qgprime - Qacc;


/****** Part 7 - calculate normalised (see note below) terminal  ******/
/****** charge expressions for the saturated drain region.       ******/

Qc2prime = -Vqd;

/* Basic expression for the intrinsic body charge in the saturation region is */
/* modified by csf, to reflect the fact that the body charge will be shared   */
/* between the gate and the drain/body depletion region.  This factor must be */
/* between 0 and 1, since it represents the fraction of ld over which qb is   */
/* integrated to give Qb2. */
Qb2prime = -gamma*csf*(Bf + delta*psi_sL);

Qd2prime = 0.5*Qc2prime;
/* JimB - 9/1/99. Re-partition drain region charge */
/* Qd2prime = Qc2prime; */

Qg2prime = -Qc2prime-Qb2prime;

*Qb += WCox*ld*Qb2prime;
*Qd += WCox*ld*Qd2prime;
*Qg += WCox*ld*Qg2prime;


/****** Part 8 - calculate full capacitance expressions, accounting ******/
/****** for both GCA and drain region contributions.  As explained  ******/
/****** in part 5, *cbgf, *cbd etc only derivatives of GCA charge   ******/
/****** expression w.r.t. Vx.  Now need to include Lprime/ld        ******/
/****** dependence on Vx as well.                                   ******/

*cbgf = WCox*(Lprime*(*cbgf) - ld*csf*(pDBf_Dpsi_st0*Dpsi_st0_Dvgfb + delta*Dpsi_sL_Dvgfb +
					(psi_sL*Dalpha_Dvgfb/gamma)) +
					(Qb2prime - Qbprime/(Fc*Fc))*Dld_Dvgfb
               );
*cbd  = WCox*(Lprime*(*cbd) - ld*csf*(pDBf_Dpsi_st0*Dpsi_st0_Dvdb + delta*Dpsi_sL_Dvdb +
					(psi_sL*Dalpha_Dvdb/gamma)) +
					(Qb2prime - Qbprime/(Fc*Fc))*Dld_Dvdb
               );
*cbs  = WCox*(Lprime*(*cbs) - ld*csf*(pDBf_Dpsi_st0*Dpsi_st0_Dvsb + delta*Dpsi_sL_Dvsb +
					(psi_sL*Dalpha_Dvsb/gamma)) +
					(Qb2prime - Qbprime/(Fc*Fc))*Dld_Dvsb
               );
*cbdeltaT  = WCox*(Lprime*(*cbdeltaT) - ld*csf*(pDBf_Dpsi_st0*Dpsi_st0_DdeltaT + delta*Dpsi_sL_DdeltaT +
					(psi_sL*Dalpha_DdeltaT/gamma)) +
					(Qb2prime - Qbprime/(Fc*Fc))*Dld_DdeltaT
               );

               
ccgf = WCox*(Lprime*(ccgf) - ld*(DvGT_Dvgfb - alpha*Dpsi_sL_Dvgfb - psi_sL*Dalpha_Dvgfb) +
              (Qc2prime - Qcprime/(Fc*Fc))*Dld_Dvgfb
             );
ccd  = WCox*(Lprime*(ccd) - ld*(DvGT_Dvdb - alpha*Dpsi_sL_Dvdb - psi_sL*Dalpha_Dvdb) +
              (Qc2prime - Qcprime/(Fc*Fc))*Dld_Dvdb
             );
ccs  = WCox*(Lprime*(ccs) - ld*(DvGT_Dvsb - alpha*Dpsi_sL_Dvsb - psi_sL*Dalpha_Dvsb) +
              (Qc2prime - Qcprime/(Fc*Fc))*Dld_Dvsb
             );
ccdeltaT  = WCox*(Lprime*(ccdeltaT) - 
              ld*(DvGT_DdeltaT - alpha*Dpsi_sL_DdeltaT - psi_sL*Dalpha_DdeltaT) +
              (Qc2prime - Qcprime/(Fc*Fc))*Dld_DdeltaT
             );


/* JimB - 9/1/99. Re-partition drain region charge */

/*
*cdgf = WCox*(Lprime*(*cdgf) - ld*(DvGT_Dvgfb - alpha*Dpsi_sL_Dvgfb - psi_sL*Dalpha_Dvgfb) +
              (Qd2prime - Qdprime/(Fc*Fc))*Dld_Dvgfb
             );
*cdd  = WCox*(Lprime*(*cdd) - ld*(DvGT_Dvdb - alpha*Dpsi_sL_Dvdb - psi_sL*Dalpha_Dvdb) +
              (Qd2prime - Qdprime/(Fc*Fc))*Dld_Dvdb
             );
*cds  = WCox*(Lprime*(*cds) - ld*(DvGT_Dvsb - alpha*Dpsi_sL_Dvsb - psi_sL*Dalpha_Dvsb) +
              (Qd2prime - Qdprime/(Fc*Fc))*Dld_Dvsb
             );
*cddeltaT  = WCox*(Lprime*(*cddeltaT) - ld*(DvGT_DdeltaT -
                    alpha*Dpsi_sL_DdeltaT - psi_sL*Dalpha_DdeltaT) +
              (Qd2prime - Qdprime/(Fc*Fc))*Dld_DdeltaT
             );
*/

*cdgf = WCox*(Lprime*(*cdgf) - 0.5*ld*(DvGT_Dvgfb - alpha*Dpsi_sL_Dvgfb - psi_sL*Dalpha_Dvgfb) +
              (Qd2prime - Qdprime/(Fc*Fc))*Dld_Dvgfb
             );
*cdd  = WCox*(Lprime*(*cdd) - 0.5*ld*(DvGT_Dvdb - alpha*Dpsi_sL_Dvdb - psi_sL*Dalpha_Dvdb) +
              (Qd2prime - Qdprime/(Fc*Fc))*Dld_Dvdb
             );
*cds  = WCox*(Lprime*(*cds) - 0.5*ld*(DvGT_Dvsb - alpha*Dpsi_sL_Dvsb - psi_sL*Dalpha_Dvsb) +
              (Qd2prime - Qdprime/(Fc*Fc))*Dld_Dvsb
             );
*cddeltaT  = WCox*(Lprime*(*cddeltaT) - 0.5*ld*(DvGT_DdeltaT -
                    alpha*Dpsi_sL_DdeltaT - psi_sL*Dalpha_DdeltaT) +
              (Qd2prime - Qdprime/(Fc*Fc))*Dld_DdeltaT
             );


/****** Part 9 - Finally, include accumulation charge derivatives.   ******/

/* Now include accumulation charge derivs */

*cbgf += -WCox*L*tmpacc;
*cbd  += -WCox*L*tmpacc*sigma;
*cbs  += -WCox*L*tmpacc*(-sigma);
*cbdeltaT += -WCox*L*tmpacc*chiFB;

*cggf = -(ccgf + *cbgf);
*cgd = -(ccd + *cbd);
*cgs = -(ccs + *cbs);
*cgdeltaT = -(ccdeltaT + *cbdeltaT);


/****** Part 10 - Back gate stuff - doesn't work, so commented out.  ******/

/* front gate stuff is self consistent by itself, now add in back gate stuff
   we're not too interested in EXACT back gate behaviour, so this is quite a
   rough model.
   But even this causes convergence problems in transient - so leave it out
   for now.  Scope for further work.
*/
/*
if (Phiplusvsb > vt*MAX_EXP_ARG) {
  A0B = Phiplusvsb;
  tmpsb = 1;
} else {
  EA0B = exp(Phiplusvsb/vt);
  A0B = vt*log(1+EA0B);
  tmpsb = EA0B/(1+EA0B);
}

Vgmax0B = A0B + gammaB*sqrt(A0B);

if ((Vgmax0B-vgB)>vt*MAX_EXP_ARG) {
  VgBx0=vgB;
  tmpmax0 = 1;
  tmpmaxsb = 0;
} else {
  EBmax0 = exp((Vgmax0B - vgB)/vt);
  VgBx0=Vgmax0B - vt*log(1+EBmax0);
  tmpmax0 = EBmax0/(1+EBmax0);
  tmpmaxsb = 1/(1+EBmax0);
}

if (VgBx0>vt*MAX_EXP_ARG) {
  VgBy0 = VgBx0;
  tmpmin0 = 1;
} else {
  EBy0 = exp(VgBx0/vt);
  VgBy0 = vt*log(1+EBy0);
  tmpmin0 = EBy0/(1+EBy0);
}

SgB0 = sqrt(gammaB*gammaB + 4*VgBy0);

*QgB = -WCob*L*gammaB*0.5*(gammaB-SgB0);
*Qb -= *QgB;

*cgbgb = (WCob*L*gammaB/SgB0)*tmpmin0*tmpmax0;
*cgbsb = (WCob*L*gammaB/SgB0)*tmpmin0*tmpmaxsb*(1+0.5*gammaB/sqrt(A0B))*tmpsb;

*cbgb = -(*cgbgb);
*cbs  -= *cgbsb;
*/

*QgB = 0;
*cbgb = 0;
*cgbgb = 0;
*cgbsb = 0;

}

void
/*
SOI3capEval(ckt,
            Frontcapargs,
            Backcapargs,
            cgfgf,cgfd,cgfs,cgfdeltaT,
            cdgf,cdd,cds,cddeltaT,
            csgf,csd,css,csdeltaT,
            cbgf,cbd,cbs,cbdeltaT,cbgb,
            cgbgb,cgbsb,
            gcgfgf,gcgfd,gcgfs,gcgfdeltaT,
            gcdgf,gcdd,gcds,gcddeltaT,
            gcsgf,gcsd,gcss,gcsdeltaT,
            gcbgf,gcbd,gcbs,gcbdeltaT,gcbgb,
            gcgbgb,gcgbsb,gcgbdb,
            gcgbs0,gcgbd0,
            qgatef,qbody,qdrn,qsrc,qgateb)

register CKTcircuit *ckt;
double Frontcapargs[6];
double Backcapargs[6];


double cgfgf,cgfd,cgfs,cgfdeltaT;
double cdgf,cdd,cds,cddeltaT;
double csgf,csd,css,csdeltaT;
double cbgf,cbd,cbs,cbdeltaT,cbgb;
double cgbgb,cgbsb;

double *gcgfgf,*gcgfd,*gcgfs,*gcgfdeltaT;
double *gcdgf,*gcdd,*gcds,*gcddeltaT;
double *gcsgf,*gcsd,*gcss,*gcsdeltaT;
double *gcbgf,*gcbd,*gcbs,*gcbdeltaT,*gcbgb;
double *gcgbgb,*gcgbsb,*gcgbdb;
double *gcgbs0,*gcgbd0;

double *qgatef;
double *qbody;
double *qdrn;
double *qsrc;
double *qgateb;
*/
SOI3capEval(sCKT *ckt,
    double *Frontcapargs,
    double *Backcapargs,
    double cgfgf,double cgfd,double cgfs,double cgfdeltaT,
    double cdgf,double cdd,double cds,double cddeltaT,
    double csgf,double csd,double css,double csdeltaT,
    double cbgf,double cbd,double cbs,double cbdeltaT,double cbgb,
    double cgbgb,double cgbsb,
    double *gcgfgf,double *gcgfd,double *gcgfs,double *gcgfdeltaT,
    double *gcdgf,double *gcdd,double *gcds,double *gcddeltaT,
    double *gcsgf,double *gcsd,double *gcss,double *gcsdeltaT,
    double *gcbgf,double *gcbd,double *gcbs,double *gcbdeltaT,double *gcbgb,
    double *gcgbgb,double *gcgbsb,double *gcgbdb,
    double *gcgbs0,double *gcgbd0,
    double *qgatef,double *qbody,double *qdrn,double *qsrc,double *qgateb)

{
    (void)Backcapargs;
double vgfd,vgfs,vgfb;
//double vgbd,vgbs,vgbb;
double cgd0,cgs0,cgb0;
//double cgbd0,cgbs0,cgbb0;
double ag0;
double qgd,qgs,qgb;
double qgb_d,qgb_s,qgb_b;

cgd0 = Frontcapargs[0];
cgs0 = Frontcapargs[1];
cgb0 = Frontcapargs[2];
vgfd = Frontcapargs[3];
vgfs = Frontcapargs[4];
vgfb = Frontcapargs[5];

//cgbd0 = Backcapargs[0];
//cgbs0 = Backcapargs[1];
//cgbb0 = Backcapargs[2];
//vgbd = Backcapargs[3];
//vgbs = Backcapargs[4];
//vgbb = Backcapargs[5];

/* stuff below includes overlap caps' conductances */
ag0 = ckt->CKTag[0];

*gcgfgf = (cgfgf + cgd0 + cgs0 + cgb0) * ag0;
*gcgfd  = (cgfd - cgd0) * ag0;
*gcgfs  = (cgfs - cgs0) * ag0;
*gcgfdeltaT = cgfdeltaT * ag0;

*gcdgf = (cdgf - cgd0) * ag0;
*gcdd  = (cdd + cgd0) * ag0;
*gcds  = cds * ag0;
*gcddeltaT = cddeltaT * ag0;

*gcsgf = (csgf - cgs0) * ag0;
*gcsd  = csd * ag0;
*gcss  = (css + cgs0) * ag0;
*gcsdeltaT = csdeltaT * ag0;

*gcbgf = (cbgf - cgb0) * ag0;
*gcbd  = cbd * ag0;
*gcbs  = cbs * ag0;
*gcbdeltaT = cbdeltaT * ag0;

*gcbgb = cbgb * ag0;
/*
*gcbgb = (cbgb - cgbb0) * ag0;


*gcgbgb = (cgbgb + cgbb0 + cgbd0 + cgbs0) * ag0;
*gcgbsb = (cgbsb - cgbs0) * ag0;
*gcgbdb = -cgbd0 * ag0;

*gcgbd0 = cgbd0 * ag0;
*gcgbs0 = cgbs0 * ag0;
*/

*gcgbgb = cgbgb * ag0;
*gcgbsb = cgbsb * ag0;
*gcgbdb = 0;

*gcgbd0 = 0;
*gcgbs0 = 0;

qgd = cgd0 * vgfd;
qgs = cgs0 * vgfs;
qgb = cgb0 * vgfb;

/*
qgb_d = cgbd0 * vgbd;
qgb_s = cgbs0 * vgbs;
qgb_b = cgbb0 * vgbb;
*/
qgb_d = 0;
qgb_s = 0;
qgb_b = 0;

*qgatef = *qgatef + qgd + qgs + qgb;
*qbody  = *qbody - qgb - qgb_b;
*qdrn   = *qdrn - qgd - qgb_d;
*qgateb = *qgateb + qgb_d + qgb_s + qgb_b;
*qsrc = -(*qgatef + *qbody + *qdrn + *qgateb);
}
