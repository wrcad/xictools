
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

/***  B4SOI 12/31/2009 Released by Tanvir Morshed  ***/

/**********
 * Copyright 2009 Regents of the University of California.  All rights reserved.
 * Authors: 1998 Samuel Fung, Dennis Sinitsky and Stephen Tang
 * Authors: 1999-2004 Pin Su, Hui Wan, Wei Jin, b3soild.c
 * Authors: 2005- Hui Wan, Xuemei Xi, Ali Niknejad, Chenming Hu.
 * Authors: 2009- Wenwei Yang, Chung-Hsun Lin, Ali Niknejad, Chenming Hu.
 * File: b4soild.c
 * Modified by Hui Wan, Xuemei Xi 11/30/2005
 * Modified by Wenwei Yang, Chung-Hsun Lin, Darsen Lu 03/06/2009
 * Modified by Tanvir Morshed 09/22/2009
 * Modified by Tanvir Morshed 12/31/2009
 **********/

#include <stdio.h>
#include "b4sdefs.h"
#include "gencurrent.h"


// See the file "BugReport" in the docs directory.  The supplied code
// for this model has convergence problems that appear to be triggered
// by use of the bypass code.  Thus, NOBYPASS is defined here to
// disable the bypass code (bypassing is used by default in Spice3 and
// WRspice).
//
#define NOBYPASS

#ifdef WIN32
#ifndef isnan
#define isnan _isnan
#endif
#endif


#define B4SOInextModel      next()
#define B4SOInextInstance   next()
#define B4SOIinstances      inst()
#define CKTreltol CKTcurTask->TSKreltol
#define CKTabstol CKTcurTask->TSKabstol
#define CKTvoltTol CKTcurTask->TSKvoltTol
#define CKTgmin CKTcurTask->TSKgmin
#define CKTbypass CKTcurTask->TSKbypass
#define CKTtemp CKTcurTask->TSKtemp
#define MAX SPMAX
//#define NIintegrate(ck, g, c, a, b) ((ck)->integrate(g, c, a, b),OK)
#define NIintegrate(ck, g, c, a, b) ((ck)->integrate(b),OK)

// SRW - added interpolation block in load function

#define EPS0 8.85418e-12 /*4.1*/
#define EPSOX 3.453133e-11
#define EPSSI 1.03594e-10
#define Charge_q 1.60219e-19
#define KboQ 8.617087e-5  /*  Kb / q   */
#define Eg300 1.115   /*  energy gap at 300K  */
#define DELTA   1.0E-9  /* v4.0 */
#define DELTA_1 0.02
#define DELTA_2 0.02
#define DELTA_3 0.02
/* Original is 0.02, for matching IBM model, change to 0.08 */
#define DELTA_3_SOI 0.08
#define DELTA_4 0.02
#define DELT_Vbseff  0.005
#define DELTA_VFB  0.02
#define OFF_Vbsitf 0.02   /* v3.1*/
#define CONST_2OV3 0.6666666666

#define MAX_EXPL 2.688117142e+43
#define MIN_EXPL 3.720075976e-44
#define EXPL_THRESHOLD 100.0
#define DEXP(A,B,C) {                                                         \
    if (A > EXPL_THRESHOLD) {                                                 \
        B = MAX_EXPL*(1.0+(A)-EXPL_THRESHOLD);                                \
        C = MAX_EXPL;                                                         \
    } else if (A < -EXPL_THRESHOLD)  {                                        \
        B = MIN_EXPL;                                                         \
        C = 0;                                                                \
    } else   {                                                                \
        B = exp(A);                                                           \
        C = B;                                                                \
    }                                                                         \
}

#define FLOG(A)  fabs(A) + 1e-14


/* B4SOIlimit(vnew,vold)
 *  limits the per-iteration change of any absolute voltage value
 */

static double
B4SOIlimit(double vnew, double vold, double limit, int *check)
{
    double T0, T1;

    if (isnan (vnew) || isnan (vold))
    {
//SRW        fprintf(stderr, "Alberto says:  YOU TURKEY!  The limiting function received NaN.\n");
//SRW        fprintf(stderr, "New prediction returns to 0.0!\n");
        DVO.textOut(OUT_WARNING,
                 "bad value (NAN) in B4SOI limiting, predictor set to 0.\n");
        vnew = 0.0;
        *check = 1;
    }
    T0 = vnew - vold;
    T1 = fabs(T0);
    if (T1 > limit)
    {
        if (T0 > 0.0)
            vnew = vold + limit;
        else
            vnew = vold - limit;
        *check = 1;
    }
    return vnew;
}


int
// SRW B4SOIdev::load(sGENmodel *genmod, sCKT *ckt)
B4SOIdev::load(sGENinstance *in_inst, sCKT *ckt)
{
// SRW    sB4SOImodel *model = static_cast<sB4SOImodel*>(genmod);
// SRW    sB4SOIinstance *here;
    sB4SOIinstance *here = (sB4SOIinstance*)in_inst;
    sB4SOImodel *model = (sB4SOImodel*)here->GENmodPtr;

    int selfheat;

    double /* SourceSatCurrent, DrainSatCurrent, */ Gmin;
    double ag0, qgd, qgs, /* qgb, von, cbhat,*/ VgstNVt, ExpVgst=0.0;
    double /*cdhat,*/ cdreq, ceqbd, ceqbs, ceqqb, ceqqd, ceqqg /*, ceq, geq */;
#if (!defined(NOBYPASS) || !defined(NEWCONV))
    double cbhat, cdhat;
#endif
    double /* evbd, evbs, */ arg /*, sarg */;
#if (!defined(NOBYPASS) || !defined(NEWCONV))
    double delvbd, delvbs, delvds, delvgd, delvgs;
#endif
    double Vfbeff, dVfbeff_dVg, dVfbeff_dVd, dVfbeff_dVb, V3, V4;
    double /* tol, PhiB, */ PhiBSWG, MJSWG;
    double gcgdb, gcggb, gcgsb, gcgeb, gcgT;
    double gcsdb, gcsgb, gcssb, gcseb, gcsT;
    double gcddb, gcdgb, gcdsb, gcdeb, gcdT;
    double gcbdb, gcbgb, gcbsb, gcbeb, gcbT;
    double gcedb, gcegb, gcesb, gceeb, gceT;
    double gcTt, gTtg, gTtb, gTtdp, gTtt, gTtsp;
    double vbd, vbs, vds, vgb, vgd, vgs, /*vgdo,*/ xfact;
#if (!defined(NOBYPASS) || !defined(NEWCONV))
    double vgdo;
#endif
    double vg, vd, vs, vp, ve/*, vb*/;
    double Vds, Vgs, Vbs, Gmbs, FwdSum, RevSum;

    double Vgs_eff, Vfb=0.0, dVfb_dVb, dVfb_dVd, dVfb_dT;
    double Phis, /*dPhis_dVb,*/ sqrtPhis, dsqrtPhis_dVb, Vth, dVth_dVb, dVth_dVd, dVth_dT;
    double Vgst, dVgst_dVg, dVgst_dVb, dVgs_eff_dVg /*, Nvtm */;
    double /* Vgdt, Vgsaddvth, Vgsaddvth2, Vgsaddvth1o3, */ n, dn_dVb, Vtm;
    double ExpArg, V0;
    double ueff, dueff_dVg, dueff_dVd, dueff_dVb, dueff_dT;
    double Esat, /* dEsat_dVg, dEsat_dVd, dEsat_dVb, */ Vdsat /*, Vdsat0 */;
    double EsatL, dEsatL_dVg, dEsatL_dVd, dEsatL_dVb, dEsatL_dT;
    double dVdsat_dVg, dVdsat_dVb, dVdsat_dVd, dVdsat_dT, Vasat /*, dAlphaz_dVg, dAlphaz_dVb */;
    double dVasat_dVg, dVasat_dVb, dVasat_dVd, dVasat_dT;
    double Va, /* Va2, */ dVa_dVd, dVa_dVg, dVa_dVb, dVa_dT;
    double Vbseff, dVbseff_dVb;
    double /* Alphaz, */ CoxWL;
//    double dVgdt_dVg, dVgdt_dVd, dVgdt_dVb;
    double T0, dT0_dVg, dT0_dVd, dT0_dVb, /* dT0_dVrg, */ dT0_dT;

    double T1, dT1_dVg, dT1_dVd, dT1_dVb, dT1_dT;
    double T2, dT2_dVg, dT2_dVd, dT2_dVb, dT2_dT/*, dT2_dVp*/;
    double T3, dT3_dVg, dT3_dVd, dT3_dVb, dT3_dT;
    double T4, /* dT4_dVg, */ dT4_dVd, dT4_dVb, dT4_dT;
    double T5, dT5_dVg, dT5_dVd, dT5_dVb, dT5_dT;
    double T6, dT6_dVg, dT6_dVd, dT6_dVb, dT6_dT/*, dT6_dVp*/;
    double T7 /*, dT7_dVg, dT7_dVd, dT7_dVb */;
    double T8, /* dT8_dVg, */ dT8_dVd /*, dT8_dVb, dT8_dVrg */;
    double T9, /* dT9_dVg, */ dT9_dVd /*, dT9_dVb, dT9_dVrg */;
    double T10, /* dT10_dVg, */ dT10_dVb, dT10_dVd;
    double T11, T12=0.0;
    double tmp, Abulk, dAbulk_dVb, Abulk0, dAbulk0_dVb;
//    double T100, T101;
    double VACLM, dVACLM_dVg, dVACLM_dVd, dVACLM_dVb, dVACLM_dT;
    double VADIBL, dVADIBL_dVg, dVADIBL_dVd, dVADIBL_dVb, dVADIBL_dT;
//    double VAHCE,  dVAHCE_dVg, dVAHCE_dVd, dVAHCE_dVb;
    double Xdep, dXdep_dVb, lt1, dlt1_dVb, ltw, dltw_dVb;
    double Delt_vth, dDelt_vth_dVb, dDelt_vth_dT;
    double Theta0, dTheta0_dVb /*, Theta1, dTheta1_dVb */;
    double /* Thetarout, dThetarout_dVb, */ TempRatio, tmp1, tmp2, tmp3, tmp4;
    double DIBL_Sft, dDIBL_Sft_dVd, /* DIBL_fact, */ Lambda, dLambda_dVg;
//    double Rout_Vgs_factor, dRout_Vgs_factor_dVg, dRout_Vgs_factor_dVb;
//    double dRout_Vgs_factor_dVd;

    double /* tempv, */ a1;

    double Vgsteff, dVgsteff_dVg, dVgsteff_dVd, dVgsteff_dVb, dVgsteff_dT;
    double Vdseff, dVdseff_dVg, dVdseff_dVd, dVdseff_dVb, dVdseff_dT;
    double VdseffCV, dVdseffCV_dVg, dVdseffCV_dVd, dVdseffCV_dVb;
    double diffVds /*, diffVdsCV */;
    double dAbulk_dVg, dn_dVd ;
    double beta, dbeta_dVg, dbeta_dVd, dbeta_dVb, dbeta_dT;
    double gche, dgche_dVg, dgche_dVd, dgche_dVb, dgche_dT;
    double fgche1, dfgche1_dVg, dfgche1_dVd, dfgche1_dVb, dfgche1_dT;
    double fgche2, dfgche2_dVg, dfgche2_dVd, dfgche2_dVb, dfgche2_dT;
    double Idl, dIdl_dVg, dIdl_dVd, dIdl_dVb, dIdl_dT;
    double Ids, Gm, Gds, Gmb;
    double CoxWovL;
    double Rds, dRds_dVg, dRds_dVb, dRds_dT, WVCox, WVCoxRds;
    double Vgst2Vtm, dVgst2Vtm_dT, VdsatCV/*, dVdsatCV_dVd, dVdsatCV_dVg, dVdsatCV_dVb*/;
    double Leff, Weff, dWeff_dVg, dWeff_dVb;
    double AbulkCV, dAbulkCV_dVb;
    double qgdo, qgso, cgdo, cgso;

//    double dxpart, sxpart;

    struct b4soiSizeDependParam *pParam;
    int ByPass, Check, ChargeComputationNeeded, /* J, */ error /*, I */;
//    double junk[50];

    double gbbsp, gbbdp, gbbg, gbbb, gbbp, gbbT;
    double gddpsp, gddpdp, gddpg, gddpb, gddpT;
    double gsspsp, gsspdp, gsspg, gsspb, gsspT;
    double Gbpbs, Gbpps;
    double /* vse, vde, */ ves, ved, veb, vge/*, delves, vedo, delved*/;
#if (!defined(NOBYPASS) || !defined(NEWCONV))
    double delves, delved, delvps, vedo;
#endif
    double vps, vpd, Vps/*, delvps*/;
    double Vbd, Ves, Vesfb, /* sqrtXdep, */ DeltVthtemp, dDeltVthtemp_dT;
    double Vbp, dVbp_dVb;
    double DeltVthw, dDeltVthw_dVb, dDeltVthw_dT;
    double Gm0, Gds0, Gmb0, GmT0, /*Gmc,*/ GmT;
    double dDIBL_Sft_dVb;
//    double diffVdsii  ;
    double Igidl, Ggidld, Ggidlg, Ggidlb;
    double Igisl, Ggisls, Ggislg, Ggislb;
    double Gjsd, /* Gjss, */ Gjsb, GjsT, Gjdd, Gjdb, GjdT;
    double Ibp, Iii, Giid, Giig, Giib, GiiT, Gcd, Gcb, GcT, ceqbody, ceqbodcon;
    double gppb, gppp;
    double delTemp, /*deldelTemp,*/ Temp;
#if (!defined(NOBYPASS) || !defined(NEWCONV))
    double deldelTemp;
#endif
    double ceqth, ceqqth;
//    double K1 /*, WL */;
    double qjs, gcjsbs, gcjsT;
    double qjd, gcjdbs, gcjdT;
    double qge;
    double ceqqe;
    double ni, Eg, Cbox, /* Nfb, */ CboxWL;
    double cjsbs;
    double dVfbeff_dVrg;
    double qinv, qgate, qbody, qdrn, qsrc, qsub, cqgate, cqbody, cqdrn, cqsub, cqtemp;

    double Cgg, Cgd, /* Cgs, */ Cgb /*, Cdg, Cdd, Cds, Cdb, Qg, Qd */;
    double Csg, Csd, /* Css, */ Csb, Cbg, Cbd, /* Cbs, */ Cbb /*, Qs, Qb */;
    double Cgg1, Cgb1, Cgd1, Cbg1, Cbb1, Cbd1, Csg1, Csd1, Csb1;
//    double Vbseff0;
    double Vdsatii /*, dVdsatii_dVg ,dVdsatii_dVd, dVdsatii_dVb, dVdsatii_dT */;
//    double Ibjt ,dIbjt_dVb ,dIbjt_dVd ,dIbjt_dT;
    double Ibs1 ,dIbs1_dVb ,dIbs1_dT;
    double Ibs2 ,dIbs2_dVb ,dIbs2_dT;
    double Ibs3 ,dIbs3_dVb ,dIbs3_dVd, dIbs3_dT;
    double Ibs4 ,dIbs4_dVb ,dIbs4_dT;
    double Ibd1 ,dIbd1_dVb ,dIbd1_dVd ,dIbd1_dT;
    double Ibd2 ,dIbd2_dVb ,dIbd2_dVd ,dIbd2_dT;
    double Ibd3 ,dIbd3_dVb ,dIbd3_dVd ,dIbd3_dT;
    double Ibd4 ,dIbd4_dVb ,dIbd4_dVd ,dIbd4_dT;
//    double ExpVbs1, dExpVbs1_dVb, dExpVbs1_dT;
//    double ExpVbs2, dExpVbs2_dVb, dExpVbs2_dT;
//    double ExpVbs4, dExpVbs4_dVb, dExpVbs4_dT;
//    double ExpVbd1, dExpVbd1_dVb, dExpVbd1_dT;
//    double ExpVbd2, dExpVbd2_dVb, dExpVbd2_dT;
//    double ExpVbd4, dExpVbd4_dVb, dExpVbd4_dT;
    double WTsi, NVtm1, NVtm2;
    double Ic ,dIc_dVb=0.0 ,dIc_dVd=0.0;
    double Ibs /* , dIbs_dVb , dIbs_dVd*/;
    double Ibd /* ,dIbd_dVb */;
    double Denomi ,dDenomi_dVg ,dDenomi_dVd ,dDenomi_dVb ,dDenomi_dT;
    double Qsub0  ,dQsub0_dVg   ,dQsub0_dVb  ,dQsub0_dVd ;
    double Qac0 ,dQac0_dVb   ,dQac0_dVd;
//    double Qdep0 ,dQdep0_dVb;
    double Qe1 , dQe1_dVb, dQe1_dVe, dQe1_dT;
    double Ce1b ,Ce1e, Ce1T;
    double dQac0_dVrg, dQsub0_dVrg;

    /*  for self-heating  */
    double vbi, vfbb, phi, sqrtPhi, Xdep0, /* jbjt, jdif, jrec, jtun, */ u0temp, vsattemp;
    double jdifs, jdifd, djdifs_dT, djdifd_dT;
    double jbjts, jbjtd, djbjts_dT, djbjtd_dT;
    double jrecs, jrecd, djrecs_dT, djrecd_dT;
    double jtuns, jtund, djtuns_dT, djtund_dT;
    double rds0=0.0, ua, ub, uc;
    double dvbi_dT, dvfbb_dT, /* djbjt_dT, djdif_dT, djrec_dT, djtun_dT, */ du0temp_dT;
    double dvsattemp_dT, drds0_dT=0.0, dua_dT, dub_dT, duc_dT, dni_dT, dVtm_dT;
    double dVfbeff_dT, dQac0_dT, dQsub0_dT;
    double CbT, CsT, CgT /*, CeT */;
    double rho, rho_ref, ku0temp; /* v4.0 */
    double drho_dT, drho_ref_dT, dku0temp_dT; /* v4.0 */

    /* v2.0 release */
    double Vbsh, dVbsh_dVb;
    double sqrtPhisExt, dsqrtPhisExt_dVb;
    double T13, T14;
    double dT11_dVb, dT13_dVb, dT14_dVb;
    double dVgst_dVd;
    double Vdsatii0, dVdsatii0_dT;
    double VgsStep, dVgsStep_dT, Ratio, dRatio_dVg, dRatio_dVb, dRatio_dVd, dRatio_dT, dTempRatio_dT;
    double Vdiff, dVdiff_dVg, dVdiff_dVb, dVdiff_dVd, dVdiff_dT;
    double dNVtm1_dT /*, dNVtm2_dT */;
    double NVtmf, NVtmr, dNVtmf_dT, dNVtmr_dT;
    double TempRatioMinus1;
    double Ahlis, dAhlis_dT, Ahlid, dAhlid_dT ;
    double WsTsi, WdTsi;
    double dPhiBSWG_dT, dcjsbs_dT, darg_dT, ddT3_dVb_dT;
    double dT7_dT, dT0_dT7, dT1_dT7, dT2_dT7;
    double CoxWLb, CoxWLcenb;
    double ExpVbsNVtm, dExpVbsNVtm_dVb, dExpVbsNVtm_dT;
    double ExpVbdNVtm, dExpVbdNVtm_dVb, dExpVbdNVtm_dVd, dExpVbdNVtm_dT;
    double Ien, dIen_dT, Iendif, dIendif_dT;
    double Ibsdif, dIbsdif_dVb, dIbsdif_dT;
    double Ibddif, dIbddif_dVb, dIbddif_dT;
    double Ehlis, dEhlis_dVb, dEhlis_dT;
    double EhlisFactor, dEhlisFactor_dVb, dEhlisFactor_dT;
    double Ehlid, dEhlid_dVb, dEhlid_dVd, dEhlid_dT;
    double EhlidFactor, dEhlidFactor_dVb, dEhlidFactor_dVd, dEhlidFactor_dT;
    double E2ndFactor, dE2ndFactor_dVb, dE2ndFactor_dVd, dE2ndFactor_dT;
    double dT10_dT, dT11_dT, DioMax;
    double cjdbs, dcjdbs_dT;
    double wdios, wdiod;

    /* for capMod3 */
    double Cox, Tox, Tcen, dTcen_dVg, dTcen_dVb, LINK, Ccen, Coxeff, dCoxeff_dVg, dCoxeff_dVb;
    double CoxWLcen, QovCox, dQac0_dVg, DeltaPhi, dDeltaPhi_dVg/*, dDeltaPhi_dVd, dDeltaPhi_dVb*/;
    double dTcen_dVd, dTcen_dT, dCoxeff_dVd, dCoxeff_dT, dCoxWLcenb_dT, qinoi, qbulk;
    double T3zb, lt1zb, ltwzb, Theta0zb;
    double Delt_vthzb, dDelt_vthzb_dT;
    double DeltVthwzb, dDeltVthwzb_dT;
    double DeltVthtempzb, dDeltVthtempzb_dT;
    double Vthzb=0.0, dVthzb_dT=0.0, Vfbzb, dVfbzb_dT;

    /* v3.2 */
    double noff, dnoff_dVd, dnoff_dVb;
    double vgmb;

    /* v3.1 added for RF */
    double geltd, gcrg, gcrgg, gcrgd, gcrgs, gcrgb, ceqgcrg;
    double vges, vgms, /*vgedo, vgmdo,*/ vged, vgmd/*, delvged, delvgmd*/;
    double /*delvges, delvgms,*/ vgme;
#ifndef NOBYPASS
    double delvges, delvgms;
#endif
    double gcgmgmb=0.0, gcgmdb, gcgmsb, gcdgmb, gcsgmb;
    double gcgmeb, gcegmb, qgme, qgmid, ceqqgmid;
    double gcgbb;
    double vgge, vggm;

    /* v3.0 */
    double Igc, dIgc_dVg, dIgc_dVd, dIgc_dVb, Igs, dIgs_dVg, dIgs_dVs, Igd, dIgd_dVg, dIgd_dVd;
    double Igcs, dIgcs_dVg, dIgcs_dVd, dIgcs_dVb, Igcd, dIgcd_dVg, dIgcd_dVd, dIgcd_dVb;
    double vgs_eff, dvgs_eff_dvg, vgd_eff, dvgd_eff_dvg;
    double VxNVt, ExpVxNVt;
    double gIstotg, gIstotd, gIstotb, gIstots, Istoteq;
    double gIdtotg, gIdtotd, gIdtotb, gIdtots, Idtoteq;
    double gIgtotg, gIgtotd, gIgtotb, gIgtots, Igtoteq;

    /* v3.0 */
    double Vbsitf, dVbsitf_dVg, dVbsitf_dVd, dVbsitf_dVb, dVbsitf_dVe, dVbsitf_dT;
    double dIgb1_dVe, gige, Giie, dT0_dVe, dRatio_dVe, dVdiff_dVe;
    double dT1_dVe, dT5_dVe, dIgb_dVe, dVox_dVe, dVoxdepinv_dVe=0.0, dVaux_dVe;
    double Gme, gTte, gbbe, gddpe, gsspe;
//    double Vbsdio, dVbsdio_dVg, dVbsdio_dVd, dVbsdio_dVe, dVbsdio_dVb, dVbsdio_dT;
    double Vbs0, dVbs0_dVg, dVbs0_dVd, /* dVbs0_dVb, */ dVbs0_dVe, dVbs0_dT;
    double Vbs0mos, dVbs0mos_dVe, dVbs0mos_dT;
    double Vbsmos, dVbsmos_dVg, dVbsmos_dVd, dVbsmos_dVb, dVbsmos_dVe, dVbsmos_dT;
    double PhiON, dPhiON_dVg, dPhiON_dVd, /* dPhiON_dVb, */ dPhiON_dVe, dPhiON_dT;
    double PhiFD, dPhiFD_dVg, dPhiFD_dVd, /* dPhiFD_dVb, */ dPhiFD_dVe, dPhiFD_dT;
    double Vbs0t, dVbs0t_dVg, dVbs0t_dVd, /* dVbs0t_dVb, */ dVbs0t_dVe, dVbs0t_dT;
    double VthFD, dVthFD_dVd, dVthFD_dVb, dVthFD_dVe, dVthFD_dT;
    double VtgsFD, ExpVtgsFD, VgstFD, ExpVgstFD;
    double VtgseffFD, dVtgseffFD_dVd, dVtgseffFD_dVg, dVtgseffFD_dVe, dVtgseffFD_dT;
    double VgsteffFD, dVgsteffFD_dVd, dVgsteffFD_dVg, dVgsteffFD_dVe, dVgsteffFD_dT;
    double dT2_dVe, dVbsh_dVg, dVbsh_dVd, dVbsh_dVe, dVbsh_dT;
    double dVgsteff_dVe, dVbseff_dVg, dVbseff_dVd, dVbseff_dVe, dVbseff_dT;

    /* v2.2 release */
    double Vgb=0.0, dVgb_dVg=0.0, dVgb_dVb, Vox, dVox_dVg, dVox_dVd, dVox_dVb;
    double OxideRatio, Vaux, dVaux_dVg, dVaux_dVd, dVaux_dVb;
    double Igb, dIgb_dVg, dIgb_dVd, dIgb_dVb;
    double ceqgate;
    double dT0_dVox, Voxeff, dVoxeff_dVox;
    double dVox_dT, dVaux_dT=0.0, dIgb_dT;
    double Voxacc=0.0, dVoxacc_dVg=0.0, dVoxacc_dVd, dVoxacc_dVb=0.0 /*, dVoxacc_dT */;
    double Voxdepinv=0.0, dVoxdepinv_dVg=0.0, dVoxdepinv_dVb=0.0, dVoxdepinv_dVd=0.0, dVoxdepinv_dT=0.0;
    double Igb1, dIgb1_dVg, dIgb1_dVd, dIgb1_dVb, dIgb1_dT;
    double Igb2, dIgb2_dVg, dIgb2_dVd, dIgb2_dVb, dIgb2_dT;
    double gigs, gigd, gigb, gigg, gigT;
    double gigpg, gigpp;

    /* v4.0 */
    double IdlovVdseff, dIdlovVdseff_dVg, dIdlovVdseff_dVd, dIdlovVdseff_dVb /*,
           dIdlovVdseff_dT */;
//    double IdovVds;
    double vdbs, vsbs, /* vdb, vsb, */ vdbd=0, vsbd, vsbdo, vbs_jct, vbd_jct;
    double Vsbs, Vdbd, Vdbs;
//    double delvdbd, delvsbs, /*delvdbs,*/ delvbd_jct,  delvbs_jct;
#if (!defined(NOBYPASS) || !defined(NEWCONV))
    double delvbd_jct, delvdbd, delvsbs;
#endif
#ifndef NOBYPASS
    double delvdbs;
#endif
    double gcdbdb, gcsbsb, gcsbb, gcdbb;
    double ceqqjd=0.0, ceqqjs=0.0;
    double Lpe_Vb; /* v4.0 for Vth */
    double DITS_Sft, DITS_Sft2, dDITS_Sft_dVb, dDITS_Sft_dVd, dDITS_Sft2_dVd, dDITS_Sft_dT;
    double FP, dFP_dT, dFP_dVg;
    double VADITS, dVADITS_dVg, dVADITS_dVd, dVADITS_dT; /* for DITS */
    double Iii_Igidl, /*Giigidl_b, Giigidl_d, Giigidl_g, Giigidl_e,*/ Giigidl_T;
//    double gjsdb;
    double Idbdp=0.0, Isbsp=0.0, cdbdp, csbsp, gcjdbdp, gcjsbsp, GGjdb, GGjsb;
    double vdes, vses/*, vdedo, delvdes, delvses, delvded, Isestot,*/
        /*cseshat, Idedtot, cdedhat*/;
#if (!defined(NOBYPASS) || !defined(NEWCONV))
    double delvses, delvded, Isestot, Idedtot, vdedo;
#endif
#ifndef NOBYPASS
    double delvdes, cseshat, cdedhat;
#endif
    double PowWeffWr, rd0=0.0, rs0=0.0, rdwmin=0.0, rswmin=0.0, drs0_dT, drd0_dT, drswmin_dT,
           drdwmin_dT, Rd, dRd_dVg, dRd_dVb, dRd_dT, Rs, dRs_dVg, dRs_dVb, dRs_dT;
    double dgstot_dvd, dgstot_dvg, dgstot_dvs, dgstot_dvb, dgstot_dve, dgstot_dT;
    double dgdtot_dvd, dgdtot_dvg, dgdtot_dvs, dgdtot_dvb, dgdtot_dve, dgdtot_dT;
    double gstot, gstotd, gstotg, gstots, gstotb, ceqgstot;
    double gdtot, gdtotd, gdtotg, gdtots, gdtotb, ceqgdtot;
    double gdpr, gspr;
    /*4.1*/
    double toxe, epsrox, epssub, epsgate /*,tsie */;
    double Tnom, Eg0, Vtm0;
    double Vbci, Idsmosfet, Iiibjt;
    double dVbci_dT, dIiibjt_dVd, dIiibjt_dVb, dIiibjt_dT;
    double VgsteffVth, dT11_dVg;
    /* v4.1 */
    /* Jun 09 */
    double toxe_mob ;
    /* Jun 09 */


    double dTheta0_dT, dn_dT, dsqrtPhisExt_dT, dT3zb_dT, dltwzb_dT, dlt1zb_dT, dTheta0zb_dT, dvth0_dT, dDIBL_Sft_dT,dtmp2_dT; /* v4.2 temp deriv */
    double Vgd, Vgd_eff, dVgd_eff_dVg;
    double dVbs0mos_dVd;
    double Ig_agbcp2, dIg_agbcp2_dVg, dIg_agbcp2_dVp;
    double vgp_eff, vgp, dvgp_eff_dvg/*, dvgp_eff_dvp*/;

    /* improved body contact charge model */
    double CoxWL2, CoxWLb2;
    double ExpVgst2, Vgsteff2=0.0, VgstNVt2, ExpArg2;
    double dVgsteff2_dVd=0.0, dVgsteff2_dVg=0.0, dVgsteff2_dVb=0.0, /*dVgsteff2_dVe,*/ dVgsteff2_dT=0.0;
    double T02=0.0;
    double Qac02, dQac02_dVrg=0.0, dQac02_dVd=0.0, dQac02_dVg=0.0, dQac02_dVb=0.0, dQac02_dT=0.0;
    double Vgs_eff2, dVgs_eff2_dVg;
//    double Vthzb2;
    double Vfbzb2=0.0, dVfbzb2_dT;
    double Vfb2, dVfb2_dVd, dVfb2_dVb, dVfb2_dT;
    double Vfbeff2=0.0, dVfbeff2_dVd, dVfbeff2_dVrg, dVfbeff2_dVg=0.0, dVfbeff2_dVb=0.0, dVfbeff2_dT;
    double Qsub02, dQsub02_dVg=0.0, dQsub02_dVrg=0.0, dQsub02_dVd=0.0, dQsub02_dVb=0.0, dQsub02_dT=0.0;
    double VdsatCV2/*, dVdsatCV2_dVg, dVdsatCV2_dVb*/;
    double VdseffCV2=0.0, dVdseffCV2_dVg=0.0, dVdseffCV2_dVd=0.0, dVdseffCV2_dVb=0.0;
    double Cbg12=0.0, Cbd12=0.0, Cbb12=0.0;
    double Cgg12=0.0, Cgd12=0.0, Cgb12=0.0;
    double Csg12=0.0, Csd12=0.0, Csb12=0.0;
    double Tcen2, dTcen2_dVg, dTcen2_dVd, dTcen2_dVb, dTcen2_dT;
    double Ccen2;
    double Coxeff2=0.0, dCoxeff2_dVg=0.0, dCoxeff2_dVd=0.0, dCoxeff2_dVb=0.0, dCoxeff2_dT=0.0;
    double CoxWLcenb2=0.0, dCoxWLcenb2_dT;
    double QovCox2;
    double DeltaPhi2, dDeltaPhi2_dVg/*, dDeltaPhi2_dVd, dDeltaPhi2_dVb*/;
    double CoxWLcen2=0.0;
    double T22=0.0, T52;
    double qsrc2, qbulk2;
    double Csg2, Csd2, Csb2;
    double  DELTA_3_SOI2;
    double dphi_dT,dsqrtPhi_dT,dXdep0_dT,cdep0,/*dcep0_dT,*/theta0vb0,dtheta0vb0_dT=0;
    double thetaRout/*,dthetaRout_dT,dcdep0_dT*/;
    double dPhis_dT,dsqrtPhis_dT,dXdep_dT,dlt1_dT,dltw_dT;
    double agidl, bgidl, cgidl, egidl, rgidl, kgidl, fgidl;

    double agisl, bgisl, cgisl, egisl, rgisl, kgisl, fgisl;
    double ucs, ud;                                     /* Bugfix # 21 Jul09*/
    double ndiode, ndioded;                             /* v4.2 bugfix */
    double nrecf0s, nrecf0d, nrecr0s, nrecr0d, vrec0s, vrec0d, ntuns, ntund, vtun0s,vtun0d;/*bugfix for junction DC swapping */

//printf("entering %x %g %g %g %d %g %g %d\n", ckt->CKTmode, ckt->CKTtime,
//ckt->CKTdelta, ckt->CKTdeltaOld[1], ckt->CKTorder, ckt->CKTag[0],
//ckt->CKTag[1], ckt->CKTbypass);

/* SRW
    for (; model != NULL; model = model->B4SOInextModel)
    {
        for (here = model->B4SOIinstances; here != NULL;
                here = here->B4SOInextInstance)
        {
*/
            Check = 0;
            ByPass = 0;
            selfheat = (model->B4SOIshMod == 1) && (here->B4SOIrth0 != 0.0);
            pParam = here->pParam;

            if ((ckt->CKTmode & MODEINITSMSIG))
            {
                vs = *(ckt->CKTrhsOld + here->B4SOIsNodePrime);
                if (!here->B4SOIvbsusrGiven)
                {
                    vbs = *(ckt->CKTstate0 + here->B4SOIvbs);
//                    vb = *(ckt->CKTrhsOld + here->B4SOIbNode);
                }
                else
                {
                    vbs = here->B4SOIvbsusr;
//                    vb = here->B4SOIvbsusr + vs;
                }
                vgs = *(ckt->CKTstate0 + here->B4SOIvgs);
                ves = *(ckt->CKTstate0 + here->B4SOIves);
                vps = *(ckt->CKTstate0 + here->B4SOIvps);
                vds = *(ckt->CKTstate0 + here->B4SOIvds);
                delTemp = *(ckt->CKTstate0 + here->B4SOIdeltemp);
                /* v4.0 */
                vdbs = *(ckt->CKTstate0 + here->B4SOIvdbs); /* v4.0 for rbody */
                vdbd = *(ckt->CKTstate0 + here->B4SOIvdbd); /* v4.0 for rbody */
                vsbs = *(ckt->CKTstate0 + here->B4SOIvsbs); /* v4.0 for rbody */
                vses = *(ckt->CKTstate0 + here->B4SOIvses); /* v4.0 for rdsmod*/
                vdes = *(ckt->CKTstate0 + here->B4SOIvdes); /* v4.0 for rdsmod*/

                /* v4.0 end */

                vg = *(ckt->CKTrhsOld + here->B4SOIgNode);
                vd = *(ckt->CKTrhsOld + here->B4SOIdNodePrime);
                vp = *(ckt->CKTrhsOld + here->B4SOIpNode);
                ve = *(ckt->CKTrhsOld + here->B4SOIeNode);

                /* v3.1 added for RF */
                vgge = *(ckt->CKTrhsOld + here->B4SOIgNodeExt);
                vggm = *(ckt->CKTrhsOld + here->B4SOIgNodeMid);

                vges = *(ckt->CKTstate0 + here->B4SOIvges);
                vgms = *(ckt->CKTstate0 + here->B4SOIvgms);
                /* v3.1 added for RF end*/
            }
            else if ((ckt->CKTmode & MODEINITTRAN))
            {
                vs = *(ckt->CKTrhsOld + here->B4SOIsNodePrime);
                if (!here->B4SOIvbsusrGiven)
                {
                    vbs = *(ckt->CKTstate1 + here->B4SOIvbs);
//                    vb = *(ckt->CKTrhsOld + here->B4SOIbNode);
                }
                else
                {
                    vbs = here->B4SOIvbsusr;
//                    vb = here->B4SOIvbsusr + vs;
                }
                vgs = *(ckt->CKTstate1 + here->B4SOIvgs);
                ves = *(ckt->CKTstate1 + here->B4SOIves);
                vps = *(ckt->CKTstate1 + here->B4SOIvps);
                vds = *(ckt->CKTstate1 + here->B4SOIvds);
                delTemp = *(ckt->CKTstate1 + here->B4SOIdeltemp);

                /* v4.0 */
                vdbs = *(ckt->CKTstate1 + here->B4SOIvdbs); /* v4.0 for rbody */
                vsbs = *(ckt->CKTstate1 + here->B4SOIvsbs); /* v4.0 for rbody */
                vses = *(ckt->CKTstate1 + here->B4SOIvses); /* v4.0 for rdsmod */
                vdes = *(ckt->CKTstate1 + here->B4SOIvdes); /* v4.0 for rdsmod */
                /* v4.0 end */

                vg = *(ckt->CKTrhsOld + here->B4SOIgNode);
                vd = *(ckt->CKTrhsOld + here->B4SOIdNodePrime);
                vp = *(ckt->CKTrhsOld + here->B4SOIpNode);
                ve = *(ckt->CKTrhsOld + here->B4SOIeNode);

                /* v3.1 added for RF */
                vgge = *(ckt->CKTrhsOld + here->B4SOIgNodeExt);
                vggm = *(ckt->CKTrhsOld + here->B4SOIgNodeMid);
                vges = *(ckt->CKTstate1 + here->B4SOIvges);
                vgms = *(ckt->CKTstate1 + here->B4SOIvgms);
                /* v3.1 added for RF end*/

            }
            else if ((ckt->CKTmode & MODEINITJCT) && !here->B4SOIoff)
            {
                vds = model->B4SOItype * here->B4SOIicVDS;
                vgs = model->B4SOItype * here->B4SOIicVGS;
                ves = model->B4SOItype * here->B4SOIicVES;
                vbs = model->B4SOItype * here->B4SOIicVBS;
                vps = model->B4SOItype * here->B4SOIicVPS;
                vdbs = vsbs = vbs; /* v4.0 */

                vg = vd = vs = vp = ve = 0.0;

                /* v3.1 added for RF */
                vges = vgms = vgs;
                vgge = vggm =0.0;
                /* v3.1 added for RF end*/

                if (vds > 0.0)      /* v4.0 */
                {
                    vdes = vds + 0.01;
                    vses = -0.01;
                }
                else if (vds < 0.0)
                {
                    vdes = vds - 0.01;
                    vses = 0.01;
                }
                else
                    vdes = vses = 0.0;

                delTemp = 0.0;
                here->B4SOIphi = pParam->B4SOIphi;



                if ((vds == 0.0) && (vgs == 0.0) && (vbs == 0.0) &&
                        ((ckt->CKTmode & (MODETRAN | MODEAC|MODEDCOP |
                                          MODEDCTRANCURVE)) || (!(ckt->CKTmode & MODEUIC))))
                {
                    vbs = 0.0;
                    /*                vgs = model->B4SOItype*0.1 + here->B4SOIvth0; */
                    vgs = model->B4SOItype * here->B4SOIvth0 + 0.1; /* v4.0 */
                    vds = 0.0;
                    ves = 0.0;
                    vps = 0.0;
                    vges = vgms = vgs; /* v3.1 */
                    vbs = vdbs = vsbs = 0.0; /* v4.0 */
                    vdes = 0.01;    /* v4.0 for rdsmod */
                    vses = -0.01; /* v4.0 for rdsmod */
                }
            }
            else if ((ckt->CKTmode & (MODEINITJCT | MODEINITFIX)) &&
                     (here->B4SOIoff))
            {
                delTemp = vps = vbs = vgs = vds = ves = 0.0;
                vg = vd = vs = vp = ve = 0.0;
                vgge = vggm = 0.0; /* v3.1 */
                vges = vgms =0.0;  /* v3.1 */
                vdbs = vsbs = vdes = vses = 0.0; /* v4.0 */
            }
            else
            {
#ifndef PREDICTOR

                if ((ckt->CKTmode & MODEINITPRED))
                {
                    xfact = ckt->CKTdelta / ckt->CKTdeltaOld[1];
                    *(ckt->CKTstate0 + here->B4SOIvbs) =
                        *(ckt->CKTstate1 + here->B4SOIvbs);
                    vbs = (1.0 + xfact)* (*(ckt->CKTstate1 + here->B4SOIvbs))
                          - (xfact * (*(ckt->CKTstate2 + here->B4SOIvbs)));
                    *(ckt->CKTstate0 + here->B4SOIvgs) =
                        *(ckt->CKTstate1 + here->B4SOIvgs);
                    vgs = (1.0 + xfact)* (*(ckt->CKTstate1 + here->B4SOIvgs))
                          - (xfact * (*(ckt->CKTstate2 + here->B4SOIvgs)));
                    *(ckt->CKTstate0 + here->B4SOIves) =
                        *(ckt->CKTstate1 + here->B4SOIves);
                    ves = (1.0 + xfact)* (*(ckt->CKTstate1 + here->B4SOIves))
                          - (xfact * (*(ckt->CKTstate2 + here->B4SOIves)));
                    *(ckt->CKTstate0 + here->B4SOIvps) =
                        *(ckt->CKTstate1 + here->B4SOIvps);
                    vps = (1.0 + xfact)* (*(ckt->CKTstate1 + here->B4SOIvps))
                          - (xfact * (*(ckt->CKTstate2 + here->B4SOIvps)));
                    *(ckt->CKTstate0 + here->B4SOIvds) =
                        *(ckt->CKTstate1 + here->B4SOIvds);
                    vds = (1.0 + xfact)* (*(ckt->CKTstate1 + here->B4SOIvds))
                          - (xfact * (*(ckt->CKTstate2 + here->B4SOIvds)));
                    *(ckt->CKTstate0 + here->B4SOIvbd) =
                        *(ckt->CKTstate0 + here->B4SOIvbs)
                        - *(ckt->CKTstate0 + here->B4SOIvds);
                    /* v4.0 */
                    *(ckt->CKTstate0 + here->B4SOIvdbs) =
                        *(ckt->CKTstate1 + here->B4SOIvdbs);
                    vdbs = (1.0 + xfact)* (*(ckt->CKTstate1 + here->B4SOIvdbs))
                           - (xfact * (*(ckt->CKTstate2 + here->B4SOIvdbs)));
                    *(ckt->CKTstate0 + here->B4SOIvdbd) =
                        *(ckt->CKTstate0 + here->B4SOIvdbs)
                        - *(ckt->CKTstate0 + here->B4SOIvds);
                    *(ckt->CKTstate0 + here->B4SOIvsbs) =
                        *(ckt->CKTstate1 + here->B4SOIvsbs);
                    vsbs = (1.0 + xfact)* (*(ckt->CKTstate1 + here->B4SOIvsbs))
                           - (xfact * (*(ckt->CKTstate2 + here->B4SOIvsbs)));
                    *(ckt->CKTstate0 + here->B4SOIvses) =
                        *(ckt->CKTstate1 + here->B4SOIvses);
                    vses = (1.0 + xfact)* (*(ckt->CKTstate1 + here->B4SOIvses))
                           - (xfact * (*(ckt->CKTstate2 + here->B4SOIvses)));
                    *(ckt->CKTstate0 + here->B4SOIvdes) =
                        *(ckt->CKTstate1 + here->B4SOIvdes);
                    vdes = (1.0 + xfact)* (*(ckt->CKTstate1 + here->B4SOIvdes))
                           - (xfact * (*(ckt->CKTstate2 + here->B4SOIvdes)));
                    /* v4.0 end */

                    *(ckt->CKTstate0 + here->B4SOIvg) = *(ckt->CKTstate1 + here->B4SOIvg);
                    *(ckt->CKTstate0 + here->B4SOIvd) = *(ckt->CKTstate1 + here->B4SOIvd);
                    *(ckt->CKTstate0 + here->B4SOIvs) = *(ckt->CKTstate1 + here->B4SOIvs);
                    *(ckt->CKTstate0 + here->B4SOIvp) = *(ckt->CKTstate1 + here->B4SOIvp);
                    *(ckt->CKTstate0 + here->B4SOIve) = *(ckt->CKTstate1 + here->B4SOIve);

                    /* v3.1 added for RF */
                    *(ckt->CKTstate0 + here->B4SOIvgge) = *(ckt->CKTstate1 + here->B4SOIvgge);
                    *(ckt->CKTstate0 + here->B4SOIvggm) = *(ckt->CKTstate1 + here->B4SOIvggm);
                    *(ckt->CKTstate0 + here->B4SOIvges) =
                        *(ckt->CKTstate1 + here->B4SOIvges);
                    vges = (1.0 + xfact)* (*(ckt->CKTstate1 + here->B4SOIvges))
                           - (xfact * (*(ckt->CKTstate2 + here->B4SOIvges)));
                    *(ckt->CKTstate0 + here->B4SOIvgms) =
                        *(ckt->CKTstate1 + here->B4SOIvgms);
                    vgms = (1.0 + xfact)* (*(ckt->CKTstate1 + here->B4SOIvgms))
                           - (xfact * (*(ckt->CKTstate2 + here->B4SOIvgms)));
                    /* v3.1 added for RF end */

                    /* Only predict ve */
                    ve = (1.0 + xfact)* (*(ckt->CKTstate1 + here->B4SOIve))
                         - (xfact * (*(ckt->CKTstate2 + here->B4SOIve)));
                    /* Then update vg, vs, vb, vd, vp base on ve */
                    vs = ve - model->B4SOItype * ves;
                    vg = model->B4SOItype * vgs + vs;
                    vd = model->B4SOItype * vds + vs;
//                    vb = model->B4SOItype * vbs + vs;
                    vp = model->B4SOItype * vps + vs;

                    vgge = model->B4SOItype * vges + vs; /* v3.1 */
                    vggm = model->B4SOItype * vgms + vs; /* v3.1 */

                    delTemp = (1.0 + xfact)* (*(ckt->CKTstate1 +
                                                here->B4SOIdeltemp))-(xfact * (*(ckt->CKTstate2 +
                                                        here->B4SOIdeltemp)));

                    /* v2.2.3 bug fix */
                    *(ckt->CKTstate0 + here->B4SOIdeltemp) =
                        *(ckt->CKTstate1 + here->B4SOIdeltemp);

                    /* if (selfheat)
                       {
                       here->B4SOIphi = 2.0 * here->B4SOIvtm
                     * log(pParam->B4SOInpeak /
                     here->B4SOIni);
                     }                                      v4.2 bugfix never used in the code */

                }
                else
                {
#endif /* PREDICTOR */

                    vg = B4SOIlimit(*(ckt->CKTrhsOld + here->B4SOIgNode),
                                    *(ckt->CKTstate0 + here->B4SOIvg), 3.0, &Check);
                    vd = B4SOIlimit(*(ckt->CKTrhsOld + here->B4SOIdNodePrime),
                                    *(ckt->CKTstate0 + here->B4SOIvd), 3.0, &Check);
                    vs = B4SOIlimit(*(ckt->CKTrhsOld + here->B4SOIsNodePrime),
                                    *(ckt->CKTstate0 + here->B4SOIvs), 3.0, &Check);
                    vp = B4SOIlimit(*(ckt->CKTrhsOld + here->B4SOIpNode),
                                    *(ckt->CKTstate0 + here->B4SOIvp), 3.0, &Check);
                    ve = B4SOIlimit(*(ckt->CKTrhsOld + here->B4SOIeNode),
                                    *(ckt->CKTstate0 + here->B4SOIve), 3.0, &Check);
                    /* v3.1 added for RF */
                    vgge = B4SOIlimit(*(ckt->CKTrhsOld + here->B4SOIgNodeExt),
                                      *(ckt->CKTstate0 + here->B4SOIvgge), 3.0, &Check);

                    vggm = B4SOIlimit(*(ckt->CKTrhsOld + here->B4SOIgNodeMid),
                                      *(ckt->CKTstate0 + here->B4SOIvggm), 3.0, &Check);
                    /* v3.1 added for RF end */

                    delTemp = *(ckt->CKTrhsOld + here->B4SOItempNode);

                    vbs = model->B4SOItype * (*(ckt->CKTrhsOld+here->B4SOIbNode)
                                              - *(ckt->CKTrhsOld+here->B4SOIsNodePrime));

                    vps = model->B4SOItype * (vp - vs);
                    vgs = model->B4SOItype * (vg - vs);
                    ves = model->B4SOItype * (ve - vs);
                    vds = model->B4SOItype * (vd - vs);

                    vges = model->B4SOItype * (vgge - vs); /* v3.1 */
                    vgms = model->B4SOItype * (vggm - vs); /* v3.1 */

                    /* v4.0 */
                    vdbs = model->B4SOItype
                           * (*(ckt->CKTrhsOld + here->B4SOIdbNode)
                              - *(ckt->CKTrhsOld + here->B4SOIsNodePrime));
                    vsbs = model->B4SOItype
                           * (*(ckt->CKTrhsOld + here->B4SOIsbNode)
                              - *(ckt->CKTrhsOld + here->B4SOIsNodePrime));
                    vses = model->B4SOItype
                           * (*(ckt->CKTrhsOld + here->B4SOIsNode)
                              - *(ckt->CKTrhsOld + here->B4SOIsNodePrime));
                    vdes = model->B4SOItype
                           * (*(ckt->CKTrhsOld + here->B4SOIdNode)
                              - *(ckt->CKTrhsOld + here->B4SOIsNodePrime));
                    /* v4.0 end */

#ifndef PREDICTOR
                }
#endif /* PREDICTOR */

                vbd = vbs - vds;
                vdbd = vdbs - vds; /* v4.0 */

                vgd = vgs - vds;
                ved = ves - vds;
#if (!defined(NOBYPASS) || !defined(NEWCONV))
                vgdo = *(ckt->CKTstate0 + here->B4SOIvgs)
                       - *(ckt->CKTstate0 + here->B4SOIvds);
                vedo = *(ckt->CKTstate0 + here->B4SOIves)
                       - *(ckt->CKTstate0 + here->B4SOIvds);
#endif

                /* v3.1 for RF */
//                vgedo = *(ckt->CKTstate0 + here->B4SOIvges)
//                        - *(ckt->CKTstate0 + here->B4SOIvds);
//                vgmdo = *(ckt->CKTstate0 + here->B4SOIvgms)
//                        - *(ckt->CKTstate0 + here->B4SOIvds);
                vged = vges - vds;
                vgmd = vgms - vds;
//                delvged = vged - vgedo;
//                delvgmd = vgmd - vgmdo;
                /* v3.1 for RF end*/

#if (!defined(NOBYPASS) || !defined(NEWCONV))
                delvbs = vbs - *(ckt->CKTstate0 + here->B4SOIvbs);
                delvbd = vbd - *(ckt->CKTstate0 + here->B4SOIvbd);
                delvgs = vgs - *(ckt->CKTstate0 + here->B4SOIvgs);
                delves = ves - *(ckt->CKTstate0 + here->B4SOIves);
                delvps = vps - *(ckt->CKTstate0 + here->B4SOIvps);
                deldelTemp = delTemp - *(ckt->CKTstate0 + here->B4SOIdeltemp);
                delvds = vds - *(ckt->CKTstate0 + here->B4SOIvds);
                delvgd = vgd - vgdo;
                delved = ved - vedo;
#endif
#ifndef NOBYPASS
                delvges = vges - *(ckt->CKTstate0 + here->B4SOIvges); /* v3.1 */
                delvgms = vgms - *(ckt->CKTstate0 + here->B4SOIvgms); /* v3.1 */
                delvdbd = vdbd - *(ckt->CKTstate0 + here->B4SOIvdbd); /* v4.0 */
                delvdbs = vdbs - *(ckt->CKTstate0 + here->B4SOIvdbs); /* v4.0 */
                delvsbs = vsbs - *(ckt->CKTstate0 + here->B4SOIvsbs); /* v4.0 */
#endif

#if (!defined(NOBYPASS) || !defined(NEWCONV))
                delvbd_jct = (!here->B4SOIrbodyMod) ? delvbd : delvdbd; /*v4.0*/
                delvbs_jct = (!here->B4SOIrbodyMod) ? delvbs : delvsbs; /*v4.0*/

                delvses = vses - *(ckt->CKTstate0 + here->B4SOIvses);/*v4.0*/
                vdedo = *(ckt->CKTstate0 + here->B4SOIvdes)
                        - *(ckt->CKTstate0 + here->B4SOIvds);   /* v4.0 */
#endif
#ifndef NOBYPASS
                delvdes = vdes - *(ckt->CKTstate0 + here->B4SOIvdes); /* v4.0 */
#endif
#if (!defined(NOBYPASS) || !defined(NEWCONV))
                delvded = vdes - vds - vdedo;       /* v4.0 */

                if (here->B4SOImode >= 0)
                {
                    cdhat = here->B4SOIcd
                            + (here->B4SOIgm-here->B4SOIgjdg)    * delvgs
                            + (here->B4SOIgds - here->B4SOIgjdd) * delvds
                            + (here->B4SOIgmbs * delvbs
                               - here->B4SOIgjdb * delvbs_jct ) /* v4.0 */
                            + (here->B4SOIgme - here->B4SOIgjde) * delves
                            + (here->B4SOIgmT - here->B4SOIgjdT) * deldelTemp; /* v3.0 */
                }
                else
                {
                    cdhat = here->B4SOIcd
                            + (here->B4SOIgm-here->B4SOIgjdg)    * delvgd
                            - (here->B4SOIgds - here->B4SOIgjdd) * delvds
                            + (here->B4SOIgmbs * delvbd
                               - here->B4SOIgjdb * delvbd_jct ) /*v4.0 */
                            + (here->B4SOIgme - here->B4SOIgjde) * delved
                            + (here->B4SOIgmT - here->B4SOIgjdT) * deldelTemp; /* v3.0 */

                }
                cbhat = here->B4SOIcb + here->B4SOIgbgs * delvgs
                        + here->B4SOIgbbs * delvbs
                        + here->B4SOIgbds * delvds
                        + here->B4SOIgbes * delves
                        + here->B4SOIgbps * delvps
                        + here->B4SOIgbT * deldelTemp; /* v3.0 */

                Isestot = here->B4SOIgstot * (*(ckt->CKTstate0 + here->B4SOIvses));
#endif
#ifndef NOBYPASS
                cseshat = Isestot + here->B4SOIgstot * delvses
                          + here->B4SOIgstotd * delvds + here->B4SOIgstotg * delvgs
                          + here->B4SOIgstotb * delvbs;
#endif

#if (!defined(NOBYPASS) || !defined(NEWCONV))
                Idedtot = here->B4SOIgdtot * vdedo;
#endif
#ifndef NOBYPASS
                cdedhat = Idedtot + here->B4SOIgdtot * delvded
                          + here->B4SOIgdtotd * delvds + here->B4SOIgdtotg * delvgs
                          + here->B4SOIgdtotb * delvbs;
#endif

#ifndef NOBYPASS
                /* following should be one big if connected by && all over
                 * the place, but some C compilers can't handle that, so
                 * we split it up here to let them digest it in stages
                 */
// SRW
// This code causes convergence problems and is not used.  I
// reformatted this in case a compiler error was at fault, but that
// does not seem to be the case.

                if ((!(ckt->CKTmode & MODEINITPRED)) && (ckt->CKTbypass) && Check == 0)
                {
                    if ((here->B4SOIsoiMod == 2) ||      /* v3.2 */
                            (FABS(delvbs) < (ckt->CKTreltol * MAX(FABS(vbs),
                                             FABS(*(ckt->CKTstate0+here->B4SOIvbs))) + ckt->CKTvoltTol)) )
                    {
                        if ((here->B4SOIsoiMod == 2) ||      /* v3.2 */
                                (FABS(delvbd) < (ckt->CKTreltol * MAX(FABS(vbd),
                                                 FABS(*(ckt->CKTstate0+here->B4SOIvbd))) + ckt->CKTvoltTol)) )
                        {
                            if ((FABS(delvgs) < (ckt->CKTreltol * MAX(FABS(vgs),
                                                 FABS(*(ckt->CKTstate0+here->B4SOIvgs))) + ckt->CKTvoltTol)))
                            {
                                if ((FABS(delves) < (ckt->CKTreltol * MAX(FABS(ves),
                                                     FABS(*(ckt->CKTstate0+here->B4SOIves))) + ckt->CKTvoltTol)))
                                {
                                    if ( (here->B4SOIbodyMod == 0) || (here->B4SOIbodyMod == 2) ||
                                            (FABS(delvps) < (ckt->CKTreltol * MAX(FABS(vps),
                                                             FABS(*(ckt->CKTstate0+here->B4SOIvps))) + ckt->CKTvoltTol)) )
                                    {
                                        if ( (here->B4SOItempNode == 0)  ||
                                                (FABS(deldelTemp) < (ckt->CKTreltol * MAX(FABS(delTemp),
                                                                     FABS(*(ckt->CKTstate0+here->B4SOIdeltemp)))
                                                                     + ckt->CKTvoltTol*1e4)))
                                        {

                                            /* v3.1 added for RF */
                                            if ((here->B4SOIrgateMod == 0) || (here->B4SOIrgateMod == 1)
                                                    || (FABS(delvges) < (ckt->CKTreltol * MAX(FABS(vges),
                                                                         FABS(*(ckt->CKTstate0 + here->B4SOIvges))) + ckt->CKTvoltTol)))
                                            {
                                                if ((here->B4SOIrgateMod != 3) || (FABS(delvgms) < (ckt->CKTreltol
                                                                                   * MAX(FABS(vgms), FABS(*(ckt->CKTstate0 + here->B4SOIvgms)))
                                                                                   + ckt->CKTvoltTol)))
                                                {
                                                    /* v3.1 added for RF end */
                                                    /* v4.0 */
                                                    if ((!here->B4SOIrbodyMod) || (FABS(delvdbs) < (ckt->CKTreltol
                                                                                   * MAX(FABS(vdbs), FABS(*(ckt->CKTstate0 + here->B4SOIvdbs)))
                                                                                   + ckt->CKTvoltTol)))
                                                    {
                                                        if ((!here->B4SOIrbodyMod) || (FABS(delvdbd) < (ckt->CKTreltol
                                                                                       * MAX(FABS(vdbd), FABS(*(ckt->CKTstate0 + here->B4SOIvdbd)))
                                                                                       + ckt->CKTvoltTol)))
                                                        {
                                                            if ((!here->B4SOIrbodyMod) || (FABS(delvsbs) < (ckt->CKTreltol
                                                                                           * MAX(FABS(vsbs), FABS(*(ckt->CKTstate0 + here->B4SOIvsbs)))
                                                                                           + ckt->CKTvoltTol)))
                                                            {
                                                                if ((!model->B4SOIrdsMod) || (FABS(delvses) < (ckt->CKTreltol
                                                                                              * MAX(FABS(vses), FABS(*(ckt->CKTstate0 + here->B4SOIvses)))
                                                                                              + ckt->CKTvoltTol)))
                                                                {
                                                                    if ((!model->B4SOIrdsMod) || (FABS(delvdes) < (ckt->CKTreltol
                                                                                                  * MAX(FABS(vdes), FABS(*(ckt->CKTstate0 + here->B4SOIvdes)))
                                                                                                  + ckt->CKTvoltTol)))
                                                                    {
                                                                        if ((!model->B4SOIrdsMod) || ((FABS(cseshat - Isestot) < ckt->CKTreltol
                                                                                                       * MAX(FABS(cseshat), FABS(Isestot)) + ckt->CKTabstol)))
                                                                        {
                                                                            if ((!model->B4SOIrdsMod) || ((FABS(cdedhat - Idedtot) < ckt->CKTreltol
                                                                                                           * MAX(FABS(cdedhat), FABS(Idedtot)) + ckt->CKTabstol)))
                                                                            {
                                                                                /* v4.0 end */

                                                                                if ((FABS(delvds) < (ckt->CKTreltol * MAX(FABS(vds),
                                                                                                     FABS(*(ckt->CKTstate0+here->B4SOIvds))) + ckt->CKTvoltTol)))
                                                                                {
                                                                                    if ((FABS(cdhat - here->B4SOIcd) < ckt->CKTreltol
                                                                                            * MAX(FABS(cdhat),FABS(here->B4SOIcd)) + ckt->CKTabstol))
                                                                                    {
                                                                                        if ((here->B4SOIsoiMod == 2) ||       /* v3.2 */
                                                                                                (FABS(cbhat - here->B4SOIcb) < ckt->CKTreltol
                                                                                                 * MAX(FABS(cbhat),FABS(here->B4SOIcb)) + ckt->CKTabstol) )
                                                                                        {
                                                                                            /* bypass code */
                                                                                            vbs = *(ckt->CKTstate0 + here->B4SOIvbs);
                                                                                            vbd = *(ckt->CKTstate0 + here->B4SOIvbd);
                                                                                            vgs = *(ckt->CKTstate0 + here->B4SOIvgs);
                                                                                            ves = *(ckt->CKTstate0 + here->B4SOIves);
                                                                                            vps = *(ckt->CKTstate0 + here->B4SOIvps);
                                                                                            vds = *(ckt->CKTstate0 + here->B4SOIvds);

                                                                                            /* v3.1 added for RF */
                                                                                            vges = *(ckt->CKTstate0 + here->B4SOIvges);
                                                                                            vgms = *(ckt->CKTstate0 + here->B4SOIvgms);
                                                                                            vged = vges - vds;
                                                                                            vgmd = vgms - vds;
                                                                                            vgme = vgms - ves;
                                                                                            /* v3.1 added for RF end */
                                                                                            vgmb = vgms - vbs; /* v3.2 bug fix */

                                                                                            /* v4.0 */
                                                                                            vdbs = *(ckt->CKTstate0 + here->B4SOIvdbs);
                                                                                            vdbd = *(ckt->CKTstate0 + here->B4SOIvdbd);
                                                                                            vsbs = *(ckt->CKTstate0 + here->B4SOIvsbs);
                                                                                            vbs_jct = (!here->B4SOIrbodyMod) ? vbs : vsbs;
                                                                                            vbd_jct = (!here->B4SOIrbodyMod) ? vbd : vdbd;
                                                                                            vses = *(ckt->CKTstate0 + here->B4SOIvses);
                                                                                            vdes = *(ckt->CKTstate0 + here->B4SOIvdes);
                                                                                            /* v4.0 end */

                                                                                            delTemp = *(ckt->CKTstate0 + here->B4SOIdeltemp);
                                                                                            /*  calculate Vds for temperature conductance calculation
                                                                                                in bypass (used later when filling Temp node matrix)  */
                                                                                            Vds = here->B4SOImode > 0 ? vds : -vds;

                                                                                            vgd = vgs - vds;
                                                                                            vgb = vgs - vbs;
                                                                                            veb = ves - vbs;

                                                                                            if ((ckt->CKTmode & (MODETRAN | MODEAC)) ||
                                                                                                    ((ckt->CKTmode & MODETRANOP) && (ckt->CKTmode & MODEUIC)))
                                                                                            {
                                                                                                ByPass = 1;
                                                                                                goto line755;
                                                                                            }
                                                                                            else
                                                                                            {
                                                                                                goto line850;
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

#endif /*NOBYPASS*/
//                von = here->B4SOIvon;


                if (*(ckt->CKTstate0 + here->B4SOIvds) >= 0.0)
                {
                    T0 = *(ckt->CKTstate0 + here->B4SOIvbs);

                    /* v3.1 added for RF */
                    if (here->B4SOIrgateMod == 3)
                    {
                        vged = vges - vds;
                        vgmd = vgms - vds;
                    }
                    else if ((here->B4SOIrgateMod == 1) || (here->B4SOIrgateMod == 2))
                    {
                        vged = vges - vds;
                    }
                    /* v3.1 added for RF end*/

                }
                else
                {
                    T0 = *(ckt->CKTstate0 + here->B4SOIvbd);

                    /* added for RF */
                    if (here->B4SOIrgateMod == 3)
                    {
                        vges = vged + vds;
                        vgms = vgmd + vds;
                    }
                    if ((here->B4SOIrgateMod == 1) || (here->B4SOIrgateMod == 2))
                    {
                        vges = vged + vds;
                    }
                    /* added for RF end*/

                }

                if (vds >= 0.0)
                {
                    vbs = B4SOIlimit(vbs, T0, 0.2, &Check);
                    vbd = vbs - vds;
//                    vb = model->B4SOItype * vbs + vs;

                    if (here->B4SOIrbodyMod) /* v4.0 */
                    {
                        vdbs = B4SOIlimit(vdbs,
                                          *(ckt->CKTstate0 + here->B4SOIvdbs), 0.2, &Check);
                        vdbd = vdbs - vds;
                        vsbs = B4SOIlimit(vsbs,
                                          *(ckt->CKTstate0 + here->B4SOIvsbs), 0.2, &Check);
                    }
                }
                else
                {
                    vbd = B4SOIlimit(vbd, T0, 0.2, &Check);
                    vbs = vbd + vds;
//                    vb = model->B4SOItype * vbs + vd;
                    /* v4.0 */
                    if (here->B4SOIrbodyMod)
                    {
                        vdbd = B4SOIlimit(vdbd,
                                          *(ckt->CKTstate0 + here->B4SOIvdbd), 0.2, &Check);
                        vdbs = vdbd + vds;
                        vsbdo = *(ckt->CKTstate0 + here->B4SOIvsbs)
                                - *(ckt->CKTstate0 + here->B4SOIvds);
                        vsbd = vsbs - vds;
                        vsbd = B4SOIlimit(vsbd, vsbdo, 0.2, &Check);
                        vsbs = vsbd + vds;
                    }
                    /* v4.0 end */
                }

                delTemp =B4SOIlimit(delTemp,
                                    *(ckt->CKTstate0 + here->B4SOIdeltemp),5.0,&Check);

            }
            if(model->B4SOImtrlMod)
            {
                epsrox = 3.9;
                toxe = model->B4SOIeot;
                epssub = EPS0 * model->B4SOIepsrsub;

            }
            else
            {
                epsrox = model->B4SOIepsrox;
                toxe = model->B4SOItox;
                epssub = EPSSI;

            }


            /*  Calculate temperature dependent values for self-heating effect  */
            Temp = delTemp + ckt->CKTtemp;
            dTempRatio_dT = 1 / model->B4SOItnom;
            TempRatio = Temp * dTempRatio_dT;
            here->B4SOITempSH = Temp;          /*v4.2 added for portability of SH Temp */
            if (selfheat)
            {
                if(model->B4SOImtrlMod==0)
                {
                    Vtm = KboQ * Temp;

                    T0 = 1108.0 + Temp;
                    T5 = Temp * Temp;
                    Eg = 1.16 - 7.02e-4 * T5 / T0;

                    T1 = ((7.02e-4 * T5) - T0 * (14.04e-4 * Temp)) / T0 / T0;
                    /*  T1 = dEg / dT  */

                    T2 = 1.9230584e-4;  /*  T2 = 1 / 300.15^(3/2)  */
                    T5 = sqrt(Temp);
                    T3 = 1.45e10 * Temp * T5 * T2;
                    T4 = exp(21.5565981 - Eg / (2.0 * Vtm));
                    ni = T3 * T4;
                    dni_dT = 2.175e10 * T2 * T5 * T4 + T3 * T4 *
                             (-Vtm * T1 + Eg * KboQ) / (2.0 * Vtm * Vtm);

                    T0 = log(1.0e20 * pParam->B4SOInpeak / (ni * ni));
                    vbi = Vtm * T0;
                    dvbi_dT = KboQ * T0 + Vtm * (-2.0 * dni_dT / ni);
                }
                else
                {
                    Tnom = model->B4SOItnom;
                    Vtm = KboQ * Temp;
                    Vtm0= KboQ * Tnom;

                    Eg0 = model->B4SOIeg0;
                    T0 = model->B4SOItbgbsub + Temp;
                    T5 = Temp * Temp;
                    Eg = model->B4SOIbg0sub - model->B4SOItbgasub * Temp * Temp
                         / (Temp + model->B4SOItbgbsub);

                    T1 = ((model->B4SOItbgasub * T5) - T0 * (2.0*model->B4SOItbgasub * Temp)) / T0 / T0;
                    /*  T1 = dEg / dT  */

                    T2 = 1/sqrt(Tnom*Tnom*Tnom);
                    T5 = sqrt(Temp);
                    T3 = model->B4SOIni0sub * Temp * T5 * T2;
                    T4 = exp(Eg0/(2.0*Vtm0) - Eg / (2.0 * Vtm));
                    ni = T3 * T4;
                    dni_dT=1.5*model->B4SOIni0sub*T5*T2*T4+
                           T3*T4*(-Vtm * T1 + Eg * KboQ) / (2.0 * Vtm * Vtm);

                    T0 = log(1.0e20 * pParam->B4SOInpeak / (ni * ni));
                    vbi = Vtm * T0;
                    dvbi_dT = KboQ * T0 + Vtm * (-2.0 * dni_dT / ni);


                }
                if (pParam->B4SOInsub > 0)
                {
                    T0 = log(pParam->B4SOInpeak / pParam->B4SOInsub);
                    vfbb = -model->B4SOItype * Vtm * T0;
                    dvfbb_dT = -model->B4SOItype * KboQ * T0;
                }
                else
                {
                    T0 = log(-pParam->B4SOInpeak * pParam->B4SOInsub / ni / ni);
                    vfbb = -model->B4SOItype * Vtm * T0;
                    dvfbb_dT = -model->B4SOItype *
                               (KboQ * T0 - Vtm * 2.0 * dni_dT / ni);
                }

                /* V4.0 changed phi */
                phi = 2.0 * Vtm * log(pParam->B4SOInpeak / ni);
                /*                phi = here->B4SOIphi;  */
                sqrtPhi = sqrt(phi);
                Xdep0 = sqrt(2.0 * epssub / (Charge_q
                                             * pParam->B4SOInpeak * 1.0e6))
                        * sqrtPhi;
                /* v4.1 SH bug fix */
                /* dphi_dT = phi / Vtm * KboQ; v4.2 Temp Derivative bug fix */
                dphi_dT = phi / Vtm * KboQ - 2.0 * Vtm * dni_dT / ni;
                dsqrtPhi_dT = 0.5 / sqrtPhi * dphi_dT;
                dXdep0_dT = Xdep0 / sqrtPhi * dsqrtPhi_dT;

                /* cdep0 = sqrt(Charge_q * EPSSI
                 * pParam->B4SOInpeak * 1.0e6 / 2.0) / sqrtPhi;     */      /* Bug fix #2 Jun 09 Body type is generalized for mtrlMod 1*/
                cdep0 = sqrt(Charge_q * epssub                                                                      /* Fix */
                             * pParam->B4SOInpeak * 1.0e6 / 2.0) / sqrtPhi;
//                dcep0_dT = cdep0 * sqrtPhi * (-1.0) / phi * dsqrtPhi_dT;

                /* T1 = sqrt(EPSSI / (model->B4SOIepsrox * EPSOX / 3.9) */                   /* Bug fix #3 Jun 09 Body type is generalized for mtrlMod 1*/
                /*  * model->B4SOItox * Xdep0); */
                T1 = sqrt(epssub / (epsrox * EPS0)                                                          /* Fix */
                          * toxe * Xdep0);
                dT1_dT = 0.5 * T1 / Xdep0 * dXdep0_dT;
                T0 = exp(-0.5 * pParam->B4SOIdsub * pParam->B4SOIleff / T1);
                dT0_dT = T0 * 0.5 *  pParam->B4SOIdsub * pParam->B4SOIleff
                         / T1 / T1 * dT1_dT;
                theta0vb0 = (T0 + 2.0 * T0 * T0);
                dtheta0vb0_dT = (1.0 + 4.0 * T0) * dT0_dT;
                T0 = exp(-0.5 * pParam->B4SOIdrout * pParam->B4SOIleff / T1);
                dT0_dT = T0 * 0.5 *  pParam->B4SOIdrout * pParam->B4SOIleff
                         / T1 / T1 * dT1_dT;
                T2 = (T0 + 2.0 * T0 * T0);
                thetaRout = pParam->B4SOIpdibl1 * T2 + pParam->B4SOIpdibl2;
//                dthetaRout_dT = pParam->B4SOIpdibl1 * (1.0 + 4.0 * T0) * dT0_dT;
                /*  Save the values below for phi calculation in B4SOIaccept()  */
                here->B4SOIvtm = Vtm;
                /* here->B4SOIni = ni;      v4.2 bugfix never used in the code */

                T3 = TempRatio - 1.0;
                T8 = 1/ model->B4SOItnom;
                T4 = Eg300 / Vtm * T3;
                dT4_dT = Eg300 / Vtm / Vtm * (Vtm * T8 - T3 * KboQ);

                T7 = pParam->B4SOIxbjt * T4 / pParam->B4SOIndiode;
                dT7_dT = pParam->B4SOIxbjt * dT4_dT
                         / pParam->B4SOIndiode;
                DEXP(T7, T0, dT0_dT7);
                dT0_dT = dT0_dT7 * dT7_dT;

                if (pParam->B4SOIxbjt == pParam->B4SOIxdif)
                {
                    T1 = T0;
                    dT1_dT = dT0_dT;
                }
                else
                {
                    T7 = pParam->B4SOIxdif * T4 / pParam->B4SOIndiode;
                    dT7_dT = pParam->B4SOIxdif * dT4_dT / pParam->B4SOIndiode;
                    DEXP(T7, T1, dT1_dT7);
                    dT1_dT = dT1_dT7 * dT7_dT;
                }

                T7 = pParam->B4SOIxrec * T4 / pParam->B4SOInrecf0;
                dT7_dT = pParam->B4SOIxrec * dT4_dT
                         / pParam->B4SOInrecf0;
                DEXP(T7, T2, dT2_dT7);
                dT2_dT = dT2_dT7 * dT7_dT;

                /* high level injection */
                Ahlis = pParam->B4SOIahli * T0;
                dAhlis_dT = pParam->B4SOIahli * dT0_dT;

                jbjts = pParam->B4SOIisbjt * T0;
                jdifs = pParam->B4SOIisdif * T1;
                jrecs = pParam->B4SOIisrec * T2;
                djbjts_dT = pParam->B4SOIisbjt * dT0_dT;
                djdifs_dT = pParam->B4SOIisdif * dT1_dT;
                djrecs_dT = pParam->B4SOIisrec * dT2_dT;

                T7 = pParam->B4SOIxtun * T3;
                dT7_dT = pParam->B4SOIxtun * T8;
                DEXP(T7, T0, dT0_dT7);
                dT0_dT = dT0_dT7 * dT7_dT;
                jtuns = pParam->B4SOIistun * T0;
                djtuns_dT = pParam->B4SOIistun * dT0_dT;

                /* drain side */
                T7 = pParam->B4SOIxbjt * T4 / pParam->B4SOIndioded;
                dT7_dT = pParam->B4SOIxbjt * dT4_dT / pParam->B4SOIndioded;
                DEXP(T7, T0, dT0_dT7);
                dT0_dT = dT0_dT7 * dT7_dT;

                if (pParam->B4SOIxbjt == pParam->B4SOIxdifd)
                {
                    T1 = T0;
                    dT1_dT = dT0_dT;
                }
                else
                {
                    T7 = pParam->B4SOIxdifd * T4 / pParam->B4SOIndioded;
                    dT7_dT = pParam->B4SOIxdifd * dT4_dT / pParam->B4SOIndioded;
                    DEXP(T7, T1, dT1_dT7);
                    dT1_dT = dT1_dT7 * dT7_dT;
                }

                T7 = pParam->B4SOIxrecd * T4 / pParam->B4SOInrecf0d;
                dT7_dT = pParam->B4SOIxrecd * dT4_dT / pParam->B4SOInrecf0d;
                DEXP(T7, T2, dT2_dT7);
                dT2_dT = dT2_dT7 * dT7_dT;

                /* high level injection */
                Ahlid = pParam->B4SOIahlid * T0;
                dAhlid_dT = pParam->B4SOIahlid * dT0_dT;

                jbjtd = pParam->B4SOIidbjt * T0;
                jdifd = pParam->B4SOIiddif * T1;

                jrecd = pParam->B4SOIidrec * T2;
                djbjtd_dT = pParam->B4SOIidbjt * dT0_dT;
                djdifd_dT = pParam->B4SOIiddif * dT1_dT;
                djrecd_dT = pParam->B4SOIidrec * dT2_dT;

                T7 = pParam->B4SOIxtund * T3;
                dT7_dT = pParam->B4SOIxtund * T8;
                DEXP(T7, T0, dT0_dT7);
                dT0_dT = dT0_dT7 * dT7_dT;
                jtund = pParam->B4SOIidtun * T0;
                djtund_dT = pParam->B4SOIidtun * dT0_dT;


                u0temp = pParam->B4SOIu0
                         * pow(TempRatio, pParam->B4SOIute);
                du0temp_dT = pParam->B4SOIu0 * pParam->B4SOIute *
                             pow(TempRatio, pParam->B4SOIute - 1.0) * T8;
                ku0temp = pParam->B4SOIku0 * (1.0
                                              /* + model->B4SOItku0 * TempRatio) + DELTA; v4.2 bugfix */
                                              + model->B4SOItku0 * T3) + DELTA;
                dku0temp_dT = pParam->B4SOIku0 * model->B4SOItku0 * T8;
                T2 = ku0temp * ku0temp;
                T7 = model->B4SOIku0 * pParam->B4SOIinv_od_ref;
                rho_ref = T7 / ku0temp;
                drho_ref_dT = -T7 / T2 * dku0temp_dT;
                T4 = model->B4SOIku0 * here->B4SOIInv_ODeff;
                rho = T4 / ku0temp;
                drho_dT = -T4 / T2 * dku0temp_dT;
                T2 = (1.0 + rho);
                T7 = (1.0 + rho_ref);
                T0 = T2 / T7;
                dT0_dT = (drho_dT * T7 - drho_ref_dT * T2 ) / T7 / T7;
                du0temp_dT = T0 * du0temp_dT + u0temp * dT0_dT;
                u0temp *= T0;

                vsattemp = pParam->B4SOIvsat - pParam->B4SOIat * T3;
                dvsattemp_dT = -pParam->B4SOIat * T8;
                T2 = (1.0 + here->B4SOIkvsat * rho);
                T7 = (1.0 + here->B4SOIkvsat * rho_ref);
                T0 = T2 / T7;
                dT0_dT = (here->B4SOIkvsat * drho_dT * T7 -
                          here->B4SOIkvsat * drho_ref_dT * T2)
                         / T7 / T7;
                dvsattemp_dT = dvsattemp_dT * T0 + vsattemp * dT0_dT;
                vsattemp *= T0;
                here->B4SOIvsattemp = vsattemp;

                if (!model->B4SOIrdsMod)
                {
                    rds0 = (pParam->B4SOIrdsw + pParam->B4SOIprt * T3)
                           / pParam->B4SOIrds0denom;
                    drds0_dT = pParam->B4SOIprt / pParam->B4SOIrds0denom
                               * T8;
                }
                else   /* v4.0 */
                {
                    PowWeffWr = pParam->B4SOIrds0denom * here->B4SOInf;
                    T10 = pParam->B4SOIprt * T3;

                    /* External Rd(V) */
                    T1 = pParam->B4SOIrdw + T10;
                    T2 = model->B4SOIrdwmin + T10;
                    rd0 = T1 / PowWeffWr;
                    rdwmin = T2 / PowWeffWr;
                    drd0_dT = pParam->B4SOIprt / PowWeffWr * T8;
                    drdwmin_dT = drd0_dT;

                    /* External Rs(V) */
                    T7 = pParam->B4SOIrsw + T10;
                    T4 = model->B4SOIrswmin + T10;
                    rs0 = T7 / PowWeffWr;
                    rswmin = T4 / PowWeffWr;
                    drs0_dT = drswmin_dT = drd0_dT;
                }

                ua = pParam->B4SOIuatemp + pParam->B4SOIua1 * T3;
                ub = pParam->B4SOIubtemp + pParam->B4SOIub1 * T3;
                uc = pParam->B4SOIuctemp + pParam->B4SOIuc1 * T3;
                dua_dT = pParam->B4SOIua1 * T8;
                dub_dT = pParam->B4SOIub1 * T8;
                duc_dT = pParam->B4SOIuc1 * T8;
            }
            else
            {
                vbi = pParam->B4SOIvbi;
                vfbb = pParam->B4SOIvfbb;
                phi = pParam->B4SOIphi;
                sqrtPhi = pParam->B4SOIsqrtPhi;
                Xdep0 = pParam->B4SOIXdep0;
                /* Eg = model->B4SOIeg0; */                                                 /* Bug fix #11 Jun 09 'Eg is evaluated at Temp, not Tnom' */
                Eg = model->B4SOIeg;                                                                /* 'model->B4SOIeg' computed in b4soitemp.c */
                /* v4.1 */                                                                          /* Since selfheat=0, using Eg from b4soitemp.c*/
                cdep0 = pParam->B4SOIcdep0;
                theta0vb0 = pParam->B4SOItheta0vb0;
                thetaRout = pParam->B4SOIthetaRout;

                jbjts = pParam->B4SOIjbjts; /* v4.0 */
                jbjtd = pParam->B4SOIjbjtd;
                jdifs = pParam->B4SOIjdifs;
                jdifd = pParam->B4SOIjdifd;
                jrecs = pParam->B4SOIjrecs;
                jrecd = pParam->B4SOIjrecd;
                jtuns = pParam->B4SOIjtuns;
                jtund = pParam->B4SOIjtund;

                /* v2.2.2 bug fix */
                Ahlis = pParam->B4SOIahli0s;
                Ahlid = pParam->B4SOIahli0d;

                u0temp = here->B4SOIu0temp;
                vsattemp = here->B4SOIvsattemp;
                ua = pParam->B4SOIua;
                ub = pParam->B4SOIub;
                uc = pParam->B4SOIuc;
                dni_dT = dvbi_dT = dvfbb_dT = 0.0;
                djbjts_dT = djdifs_dT = djrecs_dT = djtuns_dT = 0.0;
                djbjtd_dT = djdifd_dT = djrecd_dT = djtund_dT = 0.0;
                du0temp_dT = dvsattemp_dT = 0.0;
                dua_dT = dub_dT = duc_dT = 0.0;
                /* v4.1 */
                dphi_dT = dsqrtPhi_dT = dXdep0_dT = 0.0;
//                dcdep0_dT = dtheta0vb0_dT = dthetaRout_dT = 0.0;

                if (!model->B4SOIrdsMod)
                {
                    rds0 = pParam->B4SOIrds0;
                    drds0_dT = 0.0;
                }
                else
                {
                    rd0 = pParam->B4SOIrd0;
                    rs0 = pParam->B4SOIrs0;
                    rdwmin = pParam->B4SOIrdwmin;
                    rswmin = pParam->B4SOIrswmin;
                    drd0_dT = drs0_dT = drdwmin_dT = drswmin_dT = 0.0;
                }
                dAhlis_dT = dAhlid_dT = 0;
            }

            /* TempRatio used for Vth and mobility */
            if (selfheat)
            {
                TempRatioMinus1 = Temp / model->B4SOItnom - 1.0;
            }
            else
            {
                TempRatioMinus1 =  ckt->CKTtemp / model->B4SOItnom - 1.0;
            }

            /* determine DC current and derivatives */
            vbd = vbs - vds;
            vgd = vgs - vds;
            vgb = vgs - vbs;
            ved = ves - vds;
            veb = ves - vbs;
            vge = vgs - ves;
            vpd = vps - vds;
            vgp = vgs - vps;

            /* v3.1 added for RF */
            vged = vges - vds;
            vgmd = vgms - vds;
            vgme = vgms - ves;
            /* v3.1 added for RF end */
            vgmb = vgms - vbs; /* v3.2 bug fix */

            agidl = pParam->B4SOIagidl;
            bgidl = pParam->B4SOIbgidl;
            cgidl = pParam->B4SOIcgidl;
            egidl = pParam->B4SOIegidl;
            rgidl = pParam->B4SOIrgidl;
            kgidl = pParam->B4SOIkgidl;
            fgidl = pParam->B4SOIfgidl;

            agisl = pParam->B4SOIagisl;
            bgisl = pParam->B4SOIbgisl;
            cgisl = pParam->B4SOIcgisl;
            egisl = pParam->B4SOIegisl;
            rgisl = pParam->B4SOIrgisl;
            kgisl = pParam->B4SOIkgisl;
            fgisl = pParam->B4SOIfgisl;


            if (vds >= 0.0)
            {
                /* normal mode */
                here->B4SOImode = 1;
                Vds = vds;
                Vgs = vgs;
                Vbs = vbs;
                Vbd = vbd;
                Ves = ves;
                Vps = vps;
                Vsbs = vsbs; /* v4.0 */
                Vdbs = vdbs; /* v4.0 */
                Vdbd = Vdbs - Vds; /* v4.0 */
                Vgd  = vgd; /* v4.1 */
                wdios = pParam->B4SOIwdios;
                wdiod = pParam->B4SOIwdiod;
                ndiode = pParam->B4SOIndiode;                       /* v4.2 bugfix*/
                ndioded = pParam->B4SOIndioded;             /* v4.2 bugfix*/

                nrecf0s = pParam->B4SOInrecf0; /* bugfix_snps start for junction DC part*/
                nrecf0d = pParam->B4SOInrecf0d;
                nrecr0s = pParam->B4SOInrecr0;
                nrecr0d = pParam->B4SOInrecr0d;
                vrec0s  = pParam->B4SOIvrec0;
                vrec0d  = pParam->B4SOIvrec0d;
                ntuns   = pParam->B4SOIntun;
                ntund   = pParam->B4SOIntund;
                vtun0s  = pParam->B4SOIvtun0;
                vtun0d  = pParam->B4SOIvtun0d; /* bugfix_snps end for junction DC part*/
            }
            else
            {
                /* inverse mode */
                here->B4SOImode = -1;
                Vds = -vds;
                Vgs = vgd;
                Vbs = vbd;
                Vbd = vbs;
                Ves = ved;
                Vps = vpd;
                Vsbs = vdbd; /* v4.0 */
                Vdbd = vsbs; /* v4.0 */
                Vdbs = Vdbd + Vds; /* v4.0 */
                Vgd = vgs;  /* v4.1 */
                wdios = pParam->B4SOIwdiod;
                wdiod = pParam->B4SOIwdios;
                ndiode = pParam->B4SOIndioded;              /* v4.2 bugfix*/
                ndioded = pParam->B4SOIndiode;              /* v4.2 bugfix*/

                nrecf0s = pParam->B4SOInrecf0d; /* bugfix_snps start for junction DC part*/
                nrecf0d = pParam->B4SOInrecf0;
                nrecr0s = pParam->B4SOInrecr0d;
                nrecr0d = pParam->B4SOInrecr0;
                vrec0s  = pParam->B4SOIvrec0d;
                vrec0d  = pParam->B4SOIvrec0;
                ntuns   = pParam->B4SOIntund;
                ntund   = pParam->B4SOIntun;
                vtun0s  = pParam->B4SOIvtun0d;
                vtun0d  = pParam->B4SOIvtun0; /* bugfix_snps end for junction DC part*/
            }
            if( vds < 0.0)
            {
                /*Diode current*/
                T0 = jbjts;
                T1 = djbjts_dT;
                jbjts = jbjtd;
                djbjts_dT = djbjtd_dT;
                jbjtd = T0;
                djbjtd_dT = T1;

                T0 = jdifs;
                T1 = djdifs_dT;
                jdifs = jdifd;
                djdifs_dT = djdifd_dT;
                jdifd = T0;
                djdifd_dT = T1;

                T0 = jrecs;
                T1 = djrecs_dT;
                jrecs = jrecd;
                djrecs_dT = djrecd_dT;
                jrecd = T0;
                djrecd_dT = T1;

                T0 = jtuns;
                T1 = djtuns_dT;
                jtuns = jtund;
                djtuns_dT = djtund_dT;
                jtund = T0;
                djtund_dT = T1;

                /*GISL/GIDL*/
                T0 = agidl;
                agidl = agisl;
                agisl = T0;

                T0 = bgidl;
                bgidl = bgisl;
                bgisl = T0;

                T0 = cgidl;
                cgidl = cgisl;
                cgisl = T0;

                T0 = egidl;
                egidl = egisl;
                egisl = T0;

                T0 = rgidl;
                rgidl = rgisl;
                rgisl = T0;

                T0 = kgidl;
                kgidl = kgisl;
                kgisl = T0;

                T0 = fgidl;
                fgidl = fgisl;
                fgisl = T0;

                T0 = Ahlis;   /* bugfix_snps */
                Ahlis = Ahlid; /* bugfix_snps */
                Ahlid = T0;  /* bugfix_snps */

                T0 = dAhlis_dT;  /* bugfix_snps */
                dAhlis_dT = dAhlid_dT;  /* bugfix_snps */
                dAhlid_dT = T0;  /* bugfix_snps */

            }
            vbs_jct = (!here->B4SOIrbodyMod) ? Vbs : Vsbs; /* v4.0 */
            vbd_jct = (!here->B4SOIrbodyMod) ? Vbd : Vdbd; /* v4.0 */

            Vesfb = Ves - vfbb;
            Cbox = model->B4SOIcbox;
//            K1 = pParam->B4SOIk1eff;

            /* SRW
            ChargeComputationNeeded =
                ((ckt->CKTmode & (MODEAC | MODETRAN | MODEINITSMSIG)) ||
                 ((ckt->CKTmode & MODETRANOP) && (ckt->CKTmode & MODEUIC)))
                ? 1 : 0;
            */
            ChargeComputationNeeded = ckt->CKTchargeCompNeeded;

            if (here->B4SOIdebugMod <0)
                ChargeComputationNeeded = 1;

#ifdef B4SOI_DEBUG_OUT
            ChargeComputationNeeded = 1;
            here->B4SOIdebug1 = 0.0;
            here->B4SOIdebug2 = 0.0;
            here->B4SOIdebug3 = 0.0;
#endif

            /* Poly Gate Si Depletion Effect */
            T0 = here->B4SOIvfb + phi;
            if (model->B4SOImtrlMod==0)
                epsgate = epssub;
            else
                epsgate = model->B4SOIepsrgate * EPS0;

            if ((pParam->B4SOIngate > 1.e18) && (pParam->B4SOIngate < 1.e25)
                    && (Vgs > T0)&& (epsgate!=0))
                /* added to avoid the problem caused by ngate */
            {
                T1 = 1.0e6 * Charge_q * epsgate * pParam->B4SOIngate
                     / (model->B4SOIcox * model->B4SOIcox);
                T4 = sqrt(1.0 + 2.0 * (Vgs - T0) / T1);
                T2 = T1 * (T4 - 1.0);
                T3 = 0.5 * T2 * T2 / T1; /* T3 = Vpoly */
                T7 = 1.12 - T3 - 0.05;
                T6 = sqrt(T7 * T7 + 0.224);
                T5 = 1.12 - 0.5 * (T7 + T6);
                Vgs_eff = Vgs - T5;
                dVgs_eff_dVg = 1.0 - (0.5 - 0.5 / T4) * (1.0 + T7 / T6);
            }
            else
            {
                Vgs_eff = Vgs;
                dVgs_eff_dVg = 1.0;
            }

            if ((pParam->B4SOIngate > 1.e18) && (pParam->B4SOIngate < 1.e25)/* Bug fix # 25/26 Vgd_eff defined */
                    && (Vgd > T0)&& (epsgate!=0))
                /* added to avoid the problem caused by ngate */
            {
                T1 = 1.0e6 * Charge_q * epsgate * pParam->B4SOIngate
                     / (model->B4SOIcox * model->B4SOIcox);
                T4 = sqrt(1.0 + 2.0 * (Vgd - T0) / T1);
                T2 = T1 * (T4 - 1.0);
                T3 = 0.5 * T2 * T2 / T1; /* T3 = Vpoly */
                T7 = 1.12 - T3 - 0.05;
                T6 = sqrt(T7 * T7 + 0.224);
                T5 = 1.12 - 0.5 * (T7 + T6);
                Vgd_eff = Vgd - T5;
                dVgd_eff_dVg = 1.0 - (0.5 - 0.5 / T4) * (1.0 + T7 / T6);
            }
            else
            {
                Vgd_eff = Vgd;
                dVgd_eff_dVg = 1.0;
            }

            /*   if( here->B4SOImode != 1){
                 T1=Vgs_eff;
                 Vgs_eff=Vgd_eff;
                 Vgd_eff=T1;
                 T2=dVgs_eff_dVg;
                 dVgs_eff_dVg=dVgd_eff_dVg;
                 dVgd_eff_dVg=T2;
                 } */

            /* v4.1 for improved BT charge model, no poly depletion  */

            if (here->B4SOIagbcp2 > 0)
            {
                Vgs_eff2 = Vgs;
                dVgs_eff2_dVg = 1.0;
            }

            /* end v4.1 for improved BT charge model */
            Leff = pParam->B4SOIleff;

            if (selfheat)
            {
                Vtm = KboQ * Temp;
                dVtm_dT = KboQ;
            }
            else
            {
                Vtm = model->B4SOIvtm;
                dVtm_dT = 0.0;
            }

            V0 = vbi - phi;


            /* begin of v3.0 block addition */
            /* B/S built-in potential lowering calculation */
            if (here->B4SOIsoiMod == 0) /* BSIMPD */ /* v3.2 */
            {
                Vbsmos = Vbs;
                dVbsmos_dVg = 0.0;
                dVbsmos_dVd = 0.0;
                dVbsmos_dVb = 1.0;
                dVbsmos_dVe = 0.0;
                if (selfheat)  dVbsmos_dT = 0.0;
                else  dVbsmos_dT = 0.0;

                Vbp = Vbs - Vps;
                dVbp_dVb = 1;
            }
            else /* soiMod = 1 or 2: adding FD module on top of BSIMPD */
            {
                /* prepare Vbs0 & Vbs0mos for VthFD calculation */
                if (model->B4SOIfdMod == 0) /* v4.0 */
                {
                    T0 = -model->B4SOIdvbd1 * pParam->B4SOIleff / pParam->B4SOIlitl;
                    T1 = model->B4SOIdvbd0 * (exp(0.5*T0) + 2*exp(T0));
                    T2 = T1 * (vbi - phi);
                    T3 = 0.5 * pParam->B4SOIqsi / model->B4SOIcsi; /* v3.2 */
                    Vbs0t = phi - T3 + model->B4SOIvbsa + T2;
                    dVbs0t_dVd = 0.0;
                    dVbs0_dVd = 0.0;
                    if (selfheat)
                        dVbs0t_dT = T1 * dvbi_dT;
                    else
                        dVbs0t_dT = 0.0;

                    T0 = 1 + model->B4SOIcsi / Cbox;
                    T3 = -model->B4SOIdk2b * pParam->B4SOIleff / pParam->B4SOIlitl;
                    T5 = model->B4SOIk2b * (exp(0.5*T3) + 2*exp(T3));
                    T1 = (model->B4SOIk1b - T5) / T0;
                    T2 = T1 * Vesfb;
                    T4 = 1.0/(1 + Cbox / model->B4SOIcsi);
                    Vbs0 = T4 * Vbs0t + T2;
                    dVbs0_dVe = T1;
                    if (selfheat)
                        dVbs0_dT = T4 * dVbs0t_dT - T1 * dvfbb_dT;
                    else
                        dVbs0_dT = 0.0;
                }
                else
                {
                    T0 = 1.0/(model->B4SOIcsi + Cbox + model->B4SOIcdsbs);
                    T1 = -model->B4SOIdvbd1 * pParam->B4SOIleff / pParam->B4SOIlitl;
                    T2 = model->B4SOIdvbd0 * (exp(0.5*T1) + 2*exp(T1));
                    T3 = T2 * (Vds + model->B4SOIvsce);
                    T4 = 0.5 * pParam->B4SOIqsi / model->B4SOIcsi;
                    T5 = model->B4SOIcsi * T0 * (phi - T4 + model->B4SOIvbsa);
                    T6 = model->B4SOIcdsbs * T0 * T3;
                    Vbs0t = T5 + T6;
                    dVbs0t_dVd = model->B4SOIcdsbs * T0 * T2;
                    if (selfheat)
                        dVbs0t_dT = 0.0;
                    else
                        dVbs0t_dT = 0.0;

                    T7 = Cbox * T0 * Vesfb;
                    Vbs0 = Vbs0t + T7;
                    dVbs0_dVe = Cbox * T0;
                    dVbs0_dVd = dVbs0t_dVd;
                    if (selfheat)
                        dVbs0_dT = dVbs0t_dT - Cbox * T0 * dvfbb_dT;
                    else
                        dVbs0_dT = 0.0;

                }

                /* zero field body potential cal. */
                T1 = Vbs0t - Vbs0 - 0.005;
                T2 = sqrt(T1 * T1 + (2.5e-5));
                T3 = 0.5 * (T1 + T2);
                T4 = T3 * model->B4SOIcsi / pParam->B4SOIqsi; /* v3.2 */
                Vbs0mos = Vbs0 - 0.5 * T3 * T4;
                T5 = 0.5 * T4 * (1 + T1 / T2);
                dVbs0mos_dVe = dVbs0_dVe * (1 + T5);
                dVbs0mos_dVd = dVbs0_dVd + T5 * (dVbs0t_dVd - dVbs0_dVd);   /* v4.1 */
                if (selfheat)
                    dVbs0mos_dT = dVbs0_dT * (1 + T5) - T5 * dVbs0t_dT;
                else
                    dVbs0mos_dT = 0.0;


                /* set the upperbound of Vbs0mos to be phi for square root calc. */
                T1 = phi - 0.02;
                T2 = T1 - Vbs0mos - 0.005;
                T3 = sqrt(T2 * T2 + 4.0 * 0.005);
                Vbs0mos = T1 - 0.5 * (T2 + T3);
                T4 = 0.5 * (1 + T2 / T3);
                dVbs0mos_dVe = T4 * dVbs0mos_dVe;
                dVbs0mos_dVd = T4 * dVbs0mos_dVd;   /* v4.1 */
                if (selfheat)
                    /*  dVbs0mos_dT = T4 * dVbs0mos_dT; */
                    dVbs0mos_dT = dphi_dT - T4 * (dphi_dT - dVbs0mos_dT); /* v4.1 */
                else  dVbs0mos_dT = 0.0;


                /* VthFD calculation */
                Phis = phi - Vbs0mos;
//                dPhis_dVb = -1; /* w.r.t Vbs0mos */
                sqrtPhis = sqrt(Phis);
                dsqrtPhis_dVb = -0.5 / sqrtPhis;
                Xdep = Xdep0 * sqrtPhis / sqrtPhi;
                dXdep_dVb = (Xdep0 / sqrtPhi) * dsqrtPhis_dVb;
                /* v4.2 bugfix temp deriv */
                if (selfheat)
                {
                    dPhis_dT = dphi_dT - dVbs0mos_dT;
                    dsqrtPhis_dT = 0.5 / sqrtPhis * dPhis_dT;
                    dXdep_dT = dXdep0_dT * sqrtPhis / sqrtPhi
                               + Xdep0 * (dsqrtPhis_dT * sqrtPhi - sqrtPhis * dsqrtPhi_dT) / phi;
                }
                else
                {
                    dPhis_dT = 0.0;
                    dsqrtPhis_dT = 0.0;
                    dXdep_dT = 0.0;
                }/* v4.2 bugfix temp deriv */

                T3 = sqrt(Xdep);
                T0 = pParam->B4SOIdvt2 * Vbs0mos;
                dT3_dT = 1.0 / (2.0 * T3) * dXdep_dT; /* v4.2 bugfix temp deriv */
                dT0_dT = pParam->B4SOIdvt2 * dVbs0mos_dT; /* v4.2 bugfix temp deriv */
                if (T0 >= - 0.5)
                {
                    T1 = 1.0 + T0;
                    dT1_dT = dT0_dT; /* v4.2 bugfix temp deriv */
                    T2 = pParam->B4SOIdvt2 ;
                }
                else /* Added to avoid any discontinuity problems caused by dvt2 */
                {
                    T4 = 1.0 / (3.0 + 8.0 * T0);
                    /* T1 = (1.0 + 3.0 * T0) * T4; */ /* v4.2 bugfix temp deriv */
                    T5 = 1.0 + 3.0 * T0; /* v4.2 bugfix temp deriv */
                    T1 = T4 * T5; /* v4.2 bugfix temp deriv */
                    T2 = pParam->B4SOIdvt2 * T4 * T4 ;
                    dT1_dT = T4 * (3.0 - 8.0 * T5 * T4) * dT0_dT; /* v4.2 bugfix temp deriv */
                }
                lt1 = model->B4SOIfactor1 * T3 * T1;
                dlt1_dVb =model->B4SOIfactor1 * (0.5 / T3 * T1 * dXdep_dVb + T3 * T2);
                dlt1_dT = model->B4SOIfactor1 * ( dT3_dT * T1+ T3 * dT1_dT); /* v4.2 bugfix temp deriv */


                T0 = pParam->B4SOIdvt2w * Vbs0mos;
                dT0_dT = pParam->B4SOIdvt2w * dVbs0mos_dT; /* v4.2 bugfix temp deriv */
                if (T0 >= - 0.5)
                {
                    T1 = 1.0 + T0;
                    T2 = pParam->B4SOIdvt2w ;
                    dT1_dT = dT0_dT; /* v4.2 bugfix temp deriv */
                }
                else /* Added to avoid any discontinuity problems caused by dvt2w */
                {
                    T4 = 1.0 / (3.0 + 8.0 * T0);
                    /* T1 = (1.0 + 3.0 * T0) * T4; */ /* v4.2 bugfix temp deriv */
                    T5 = 1.0 + 3.0 * T0; /* v4.2 bugfix temp deriv */
                    T1 = T4 * T5; /* v4.2 bugfix temp deriv */
                    T2 = pParam->B4SOIdvt2w * T4 * T4 ;
                    dT1_dT=T4*(3.0-8.0*T5*T4)*dT0_dT ; /* v4.2 bugfix temp deriv */
                }
                ltw= model->B4SOIfactor1 * T3 * T1;
                dltw_dVb=model->B4SOIfactor1*(0.5 / T3 * T1 * dXdep_dVb + T3 * T2);

                dltw_dT=model->B4SOIfactor1 *( dT3_dT * T1+ T3 *dT1_dT);/* v4.2 bugfix temp deriv */

                T0 = -0.5 * pParam->B4SOIdvt1 * Leff / lt1;
                if (T0 > -EXPL_THRESHOLD)
                {
                    T1 = exp(T0);
                    Theta0 = T1 * (1.0 + 2.0 * T1);
                    dT1_dVb = -T0 / lt1 * T1 * dlt1_dVb;
                    dTheta0_dVb = (1.0 + 4.0 * T1) * dT1_dVb;
                    dT1_dT = -T0 / lt1 * T1 * dlt1_dT;   /* v4.2 bugfix temp deriv */
                    dTheta0_dT = (1.0 + 4.0 * T1) * dT1_dT; /* v4.2 bugfix temp deriv */
                }
                else
                {
                    T1 = MIN_EXPL;
                    Theta0 = T1 * (1.0 + 2.0 * T1);
                    dTheta0_dVb = 0.0;
                    dTheta0_dT = 0; /* v4.2 bugfix temp deriv */
                }

                T2 = pParam->B4SOInfactor * epssub / Xdep;
                dT2_dVb = - T2 / Xdep * dXdep_dVb;
                dT2_dT = - T2 / Xdep * dXdep_dT; /* v4.2 bugfix temp deriv */
                /* T3 = pParam->B4SOIcdsc + pParam->B4SOIcdscb * Vbseff
                   + pParam->B4SOIcdscd * Vds;*/
                /* v4.1 */
                T3 = pParam->B4SOIcdsc + pParam->B4SOIcdscb * Vbs0mos
                     + pParam->B4SOIcdscd * Vds;
                dT3_dVb = pParam->B4SOIcdscb;
                dT3_dVd = pParam->B4SOIcdscd;
                T4 = (T2 + T3 * Theta0 + pParam->B4SOIcit)
                     / model->B4SOIcox;
                dT4_dVb = (dT2_dVb + Theta0 * dT3_dVb
                           + dTheta0_dVb * T3) / model->B4SOIcox;
                dT4_dVd = Theta0 * dT3_dVd / model->B4SOIcox;
                dT4_dT = (dT2_dT + T3 * dTheta0_dT + pParam->B4SOIcdscb * dVbs0mos_dT * Theta0) / model->B4SOIcox; /* v4.2 bugfix temp deriv */
                if (T4 >= -0.5)
                {
                    n = 1.0 + T4;
                    dn_dVb = dT4_dVb;

                    dn_dVd = dT4_dVd;
                    dn_dT = dT4_dT; /* v4.2 bugfix temp deriv */
                }
                else   /* avoid  discontinuity problems caused by T4 */
                {
                    T0 = 1.0 / (3.0 + 8.0 * T4);
                    /*n = (1.0 + 3.0 * T4) * T0;*/ /* v4.2 bugfix temp deriv */
                    T5 = 1.0 + 3.0 * T4; /* v4.2 bugfix temp deriv */
                    n = T0 * T5;/* v4.2 bugfix temp deriv */
                    T0 *= T0;
                    dn_dVb = T0 * dT4_dVb;
                    dn_dVd = T0 * dT4_dVd;
                    dn_dT = T0 * (3.0 - 8.0 * T5 * T0) * dT4_dT; /* v4.2 bugfix temp deriv */
                }

                if (pParam->B4SOIdvtp0 > 0.0)   /* v4.0 */
                {
                    T0 = -pParam->B4SOIdvtp1 * Vds;
                    if (T0 < -EXPL_THRESHOLD)
                    {
                        T2 = MIN_EXPL;
                        dT2_dVd = 0.0;
                    }
                    else
                    {
                        T2 = exp(T0);
                        dT2_dVd = -pParam->B4SOIdvtp1 * T2;
                    }

                    T3 = Leff + pParam->B4SOIdvtp0 * (1.0 + T2);
                    dT3_dVd = pParam->B4SOIdvtp0 * dT2_dVd;
                    T4 = Vtm * log(Leff / T3);
                    dT4_dVd = -Vtm * dT3_dVd / T3;
                    DITS_Sft = n * T4;
                    dDITS_Sft_dVd = dn_dVd * T4 + n * dT4_dVd;
                    dDITS_Sft_dVb = T4 * dn_dVb;
                    if (selfheat)
                    {
                        /* dDITS_Sft_dT = n * KboQ * log(Leff / T3); *//* v4.2 bugfix temp deriv */
                        dDITS_Sft_dT = n * KboQ * log(Leff / T3) + dn_dT * T4; /* v4.2 bugfix temp deriv */
                    }
                    else
                        dDITS_Sft_dT = 0.0;
                }
                else
                {
                    DITS_Sft = dDITS_Sft_dVd = dDITS_Sft_dVb = 0.0;
                    dDITS_Sft_dT = 0.0;
                }

                here->B4SOIthetavth = pParam->B4SOIdvt0 * Theta0;
                Delt_vth = here->B4SOIthetavth * V0;
                dDelt_vth_dVb = pParam->B4SOIdvt0 * dTheta0_dVb * V0;
                if (selfheat)
                    /*dDelt_vth_dT = here->B4SOIthetavth * dvbi_dT;*/
                    /*dDelt_vth_dT = here->B4SOIthetavth * (dvbi_dT - dphi_dT); */ /* v4.1 */
                    dDelt_vth_dT = pParam->B4SOIdvt0 * (dTheta0_dT * V0 + Theta0 * (dvbi_dT - dphi_dT)); /* v4.2 bugfix temp deriv */
                else  dDelt_vth_dT = 0.0;
                T0 = -0.5 * pParam->B4SOIdvt1w * pParam->B4SOIweff * Leff / ltw;
                if (T0 > -EXPL_THRESHOLD)
                {
                    T1 = exp(T0);
                    T2 = T1 * (1.0 + 2.0 * T1);
                    dT1_dVb = -T0 / ltw * T1 * dltw_dVb;
                    dT2_dVb = (1.0 + 4.0 * T1) * dT1_dVb;
                    dT2_dT = -(1.0 + 4.0 * T1) * T1 * T0/ltw * dltw_dT;
                }
                else
                {
                    T1 = MIN_EXPL;
                    T2 = T1 * (1.0 + 2.0 * T1);
                    dT2_dVb = 0.0;
                    dT2_dT = 0;
                }
                T0 = pParam->B4SOIdvt0w * T2;
                DeltVthw = T0 * V0;
                dDeltVthw_dVb = pParam->B4SOIdvt0w * dT2_dVb * V0;
                if (selfheat)
                    /* dDeltVthw_dT = T0 * dvbi_dT; */
                    /* dDeltVthw_dT = T0 * (dvbi_dT - dphi_dT); */ /* v4.1 */ /* v4.2 bugfix temp deriv */
                    dDeltVthw_dT = T0 * (dvbi_dT - dphi_dT) + pParam->B4SOIdvt0w * dT2_dT * V0; /* v4.2 bugfix temp deriv */
                else   dDeltVthw_dT = 0.0;

                T0 = sqrt(1.0 + pParam->B4SOIlpe0 / Leff);
                T1 = (pParam->B4SOIkt1 + pParam->B4SOIkt1l / Leff
                      + pParam->B4SOIkt2 * Vbs0mos);

                /* v4.0 */
                /*                   DeltVthtemp = pParam->B4SOIk1eff * (T0 - 1.0) * sqrtPhi + T1 * TempRatioMinus1; */
                DeltVthtemp = pParam->B4SOIk1ox * (T0 - 1.0) * sqrtPhi
                              + T1 * TempRatioMinus1;
                /* v4.0 end */

                if (selfheat)
                    /*  dDeltVthtemp_dT = T1 / model->B4SOItnom;  */
                    /* dDeltVthtemp_dT = pParam->B4SOIk1ox * (T0 - 1.0) * dsqrtPhi_dT
                       + T1 / model->B4SOItnom; */  /* v4.1 */ /* v4.2 bugfix temp deriv */
                    dDeltVthtemp_dT = pParam->B4SOIk1ox * (T0 - 1.0) * dsqrtPhi_dT
                                      + T1 / model-> B4SOItnom+ pParam->B4SOIkt2 * dVbs0mos_dT* TempRatioMinus1;/* v4.2 bugfix temp deriv */
                else
                    dDeltVthtemp_dT = 0.0;

                tmp2 = toxe * phi / (pParam->B4SOIweff + pParam->B4SOIw0);
                dtmp2_dT = toxe * dphi_dT / (pParam->B4SOIweff + pParam->B4SOIw0); /* v4.2 bugfix temp deriv */
                T3 = here->B4SOIeta0 + pParam->B4SOIetab * Vbs0mos;/*v4.0*/
                dT3_dT = pParam->B4SOIetab * dVbs0mos_dT; /*v4.2 temp deriv*/
                if (T3 < 1.0e-4) /* avoid  discontinuity problems caused by etab */
                {
                    T9 = 1.0 / (3.0 - 2.0e4 * T3);
                    T5 = (2.0e-4 - T3); /*v4.2 temp deriv*/
                    T3 =  T5 * T9; /*(2.0e-4 - T3) * T9;*/ /*v4.2 temp deriv*/
                    T4 = T9 * T9 * pParam->B4SOIetab;
                    dT3_dVb = T4 ;
                    dT3_dT = (2.0e4 * T5 * T9 * T9 - T9) * dT3_dT; /*v4.2 temp deriv*/
                }
                else
                {
                    dT3_dVb = pParam->B4SOIetab ;
                }
                /*  DIBL_Sft = T3 * pParam->B4SOItheta0vb0 * Vds;
                    dDIBL_Sft_dVd = pParam->B4SOItheta0vb0 * T3;
                    dDIBL_Sft_dVb = pParam->B4SOItheta0vb0 * Vds * dT3_dVb; */ /* v4.2 bug fix */
                DIBL_Sft = T3 * theta0vb0 * Vds;
                dDIBL_Sft_dVd = theta0vb0 * T3;
                dDIBL_Sft_dVb = theta0vb0 * Vds * dT3_dVb;
                dDIBL_Sft_dT = Vds * (dT3_dT * theta0vb0 + T3 * dtheta0vb0_dT); /* v4.2 bug fix */
                Lpe_Vb = sqrt(1.0 + pParam->B4SOIlpeb / Leff);

                /* 4.1 */
                T0 = exp(2.0 * pParam->B4SOIdvtp4 * Vds);
                DITS_Sft2 = pParam->B4SOIdvtp2factor * (T0-1) / (T0+1);
                dDITS_Sft2_dVd = pParam->B4SOIdvtp2factor * pParam->B4SOIdvtp4 * 4.0 * T0 / ((T0+1) * (T0+1));

                VthFD = model->B4SOItype * here->B4SOIvth0
                        + (pParam->B4SOIk1ox * sqrtPhis
                           - pParam->B4SOIk1eff * sqrtPhi) * Lpe_Vb
                        - here->B4SOIk2ox * Vbs0mos- Delt_vth - DeltVthw
                        + (pParam->B4SOIk3 + pParam->B4SOIk3b * Vbs0mos)
                        * tmp2 + DeltVthtemp - DIBL_Sft - DITS_Sft - DITS_Sft2;


                T6 = pParam->B4SOIk3b * tmp2 - here->B4SOIk2ox
                     + pParam->B4SOIkt2 * TempRatioMinus1;
                dVthFD_dVb = Lpe_Vb * pParam->B4SOIk1ox * dsqrtPhis_dVb
                             - dDelt_vth_dVb - dDeltVthw_dVb
                             + T6 - dDIBL_Sft_dVb - dDITS_Sft_dVb;  /* v4.0 */
                /*  this is actually dVth_dVbs0mos  */

                dVthFD_dVe = dVthFD_dVb * dVbs0mos_dVe;
                /* dVthFD_dVd = -dDIBL_Sft_dVd -dDITS_Sft_dVd; */ /* v4.0 */
                dVthFD_dVd = dVthFD_dVb * dVbs0mos_dVd - dDIBL_Sft_dVd - dDITS_Sft_dVd - dDITS_Sft2_dVd;   /* v4.1 */

                if (selfheat)
                    /*   dVthFD_dT = dDeltVthtemp_dT - dDelt_vth_dT
                         - dDeltVthw_dT + dVthFD_dVb * dVbs0mos_dT
                         - dDITS_Sft_dT ;  */
                    dVthFD_dT = dDeltVthtemp_dT - dDelt_vth_dT
                                - dDeltVthw_dT + dVthFD_dVb * dVbs0mos_dT
                                - dDITS_Sft_dT
                                + Lpe_Vb * ( pParam->B4SOIk1ox * 0.5 / sqrtPhis * dphi_dT
                                             - pParam->B4SOIk1eff * dsqrtPhi_dT);  /* v4.1 */

                else  dVthFD_dT = 0.0;


                /* VtgseffFD calculation for PhiFD */
                VtgsFD = VthFD - Vgs_eff;
                T10 = model->B4SOInofffd * Vtm;
                DEXP((VtgsFD - model->B4SOIvofffd)/ T10, ExpVtgsFD, T0);
                VtgseffFD = T10 * log(1.0 + ExpVtgsFD);
                T0 /= (1.0 + ExpVtgsFD);
                dVtgseffFD_dVd = T0 * dVthFD_dVd;
                dVtgseffFD_dVg = -T0 * dVgs_eff_dVg;
                dVtgseffFD_dVe = T0 * dVthFD_dVe;
                if (selfheat)
                    dVtgseffFD_dT = T0 * (dVthFD_dT - (VtgsFD - model->B4SOIvofffd)/Temp)
                                    + VtgseffFD/Temp;
                else dVtgseffFD_dT = 0.0;


                /* surface potential modeling at strong inversion: PhiON */
                VgstFD = Vgs_eff - VthFD;
                DEXP((VgstFD - model->B4SOIvofffd)/ T10, ExpVgstFD, T0);
                VgsteffFD = T10 * log(1.0 + ExpVgstFD);
                T0 /= (1.0 + ExpVgstFD);
                dVgsteffFD_dVd = -T0 * dVthFD_dVd;
                dVgsteffFD_dVg = T0 * dVgs_eff_dVg;
                dVgsteffFD_dVe = -T0 * dVthFD_dVe;
                if (selfheat)
                    dVgsteffFD_dT = T0 * (-dVthFD_dT
                                          - (VgstFD - model->B4SOIvofffd)/Temp)
                                    + VgsteffFD/Temp;
                else dVgsteffFD_dT = 0.0;


                /*                     T1 = model->B4SOImoinFD*pParam->B4SOIk1eff*Vtm*Vtm;  */
                T1 = model->B4SOImoinFD*pParam->B4SOIk1ox*Vtm*Vtm;
                if (selfheat) dT1_dT = 2*T1/Temp;
                else dT1_dT=0.0;

                T2 = VgsteffFD+ 2*pParam->B4SOIk1eff*sqrt(phi);
                dT2_dVg = dVgsteffFD_dVg;
                dT2_dVd = dVgsteffFD_dVd;
                dT2_dVe = dVgsteffFD_dVe;
                /* if (selfheat) dT2_dT = dVgsteffFD_dT; */
                if (selfheat) dT2_dT = dVgsteffFD_dT + 2*pParam->B4SOIk1eff*dsqrtPhi_dT; /* v4.1 */
                else dT2_dT = 0.0;

                T0 = 1+ VgsteffFD * T2 / T1;
                dT0_dVg = (VgsteffFD * dT2_dVg + T2 * dVgsteffFD_dVg) / T1;
                dT0_dVd = (VgsteffFD * dT2_dVd + T2 * dVgsteffFD_dVd) / T1;
                dT0_dVe = (VgsteffFD * dT2_dVe + T2 * dVgsteffFD_dVe) / T1;
                if (selfheat)
                    dT0_dT = (VgsteffFD * (dT2_dT - T2/T1 * dT1_dT) + T2 * dVgsteffFD_dT) / T1;
                else dT0_dT = 0.0;


                PhiON = phi + Vtm* log(T0) ;
                dPhiON_dVg = Vtm* dT0_dVg/T0 ;
                dPhiON_dVd = Vtm* dT0_dVd/T0 ;
                dPhiON_dVe = Vtm* dT0_dVe/T0 ;
                if (selfheat)
                    dPhiON_dT = dphi_dT + Vtm* dT0_dT/T0 + (PhiON-phi)/Temp ; /* v4.1 */
                else dPhiON_dT = 0.0;


                /* surface potential from subthreshold to inversion: PhiFD */
                T0 = model->B4SOIcox / (model->B4SOIcox + 1.0/(1.0/model->B4SOIcsi + 1.0/Cbox));
                PhiFD = PhiON - T0 * VtgseffFD;
                dPhiFD_dVg = dPhiON_dVg - T0 * dVtgseffFD_dVg;
                dPhiFD_dVd = dPhiON_dVd - T0 * dVtgseffFD_dVd;
                dPhiFD_dVe = dPhiON_dVe - T0 * dVtgseffFD_dVe;
                if (selfheat)
                    dPhiFD_dT = dPhiON_dT - T0 * dVtgseffFD_dT;
                else dPhiFD_dT = 0;


                /* built-in potential lowering: Vbs0 */
                if (model->B4SOIfdMod == 0) /* v4.0 */
                {
                    T0 = -model->B4SOIdvbd1 * pParam->B4SOIleff / pParam->B4SOIlitl;
                    T1 = model->B4SOIdvbd0 * (exp(0.5*T0) + 2*exp(T0));
                    T2 = T1 * (vbi - phi);
                    T3 = 0.5 * pParam->B4SOIqsi / model->B4SOIcsi; /* v3.2 */
                    Vbs0t = PhiFD - T3 + model->B4SOIvbsa + T2;
                    dVbs0t_dVg = dPhiFD_dVg;
                    dVbs0t_dVd = dPhiFD_dVd;
                    dVbs0t_dVe = dPhiFD_dVe;
                    if (selfheat)
                        dVbs0t_dT = dPhiFD_dT + T1 * (dvbi_dT - dphi_dT); /* v4.1 */
                    else dVbs0t_dT = 0;


                    T0 = 1 + model->B4SOIcsi / Cbox;
                    T3 = -model->B4SOIdk2b * pParam->B4SOIleff / pParam->B4SOIlitl;
                    T5 = model->B4SOIk2b * (exp(0.5*T3) + 2*exp(T3));
                    T1 = (model->B4SOIk1b - T5) / T0;
                    T2 = T1 * Vesfb;
                    T0 = 1.0/(1 + Cbox / model->B4SOIcsi);
                    Vbs0 = T0 * Vbs0t + T2;
                    dVbs0_dVg = T0 * dVbs0t_dVg;
                    dVbs0_dVd = T0 * dVbs0t_dVd;
                    dVbs0_dVe = T0 * dVbs0t_dVe + T1;
                    if (selfheat)
                        dVbs0_dT =  T0 * dVbs0t_dT - T1 * dvfbb_dT;
                    else
                        dVbs0_dT = 0.0;
                }
                else  /* v4.1 */
                {
                    T0 = 1.0/(model->B4SOIcsi + Cbox + model->B4SOIcdsbs);
                    T1 = -model->B4SOIdvbd1 * pParam->B4SOIleff / pParam->B4SOIlitl;
                    T2 = model->B4SOIdvbd0 * (exp(0.5*T1) + 2*exp(T1));
                    T3 = T2 * (Vds + model->B4SOIvsce);
                    T4 = 0.5 * pParam->B4SOIqsi / model->B4SOIcsi;
                    T5 = model->B4SOIcsi * T0 * (PhiFD - T4 + model->B4SOIvbsa);
                    T6 = model->B4SOIcdsbs * T0 * T3;
                    Vbs0t = T5 + T6;
                    T8 = model->B4SOIcsi * T0;
                    dVbs0t_dVg = T8 * dPhiFD_dVg;
                    dVbs0t_dVd = T8 * dPhiFD_dVd + model->B4SOIcdsbs * T0 * T2;
                    dVbs0t_dVe = T8 * dPhiFD_dVe;
                    if (selfheat)
                        dVbs0t_dT = T8 * dPhiFD_dT;
                    else
                        dVbs0t_dT = 0.0;

                    T7 = Cbox * T0 * Vesfb;
                    Vbs0 = Vbs0t + T7;
                    dVbs0_dVg = dVbs0t_dVg;
                    dVbs0_dVe = dVbs0t_dVe + Cbox * T0;
                    dVbs0_dVd = dVbs0t_dVd;
                    if (selfheat)
                        dVbs0_dT = dVbs0t_dT - Cbox * T0 * dvfbb_dT;
                    else
                        dVbs0_dT = 0.0;

                }

                /* set lowerbound of Vbs (from SPICE) to Vbs0: Vbsitf (Vbs at back interface) */
                if (here->B4SOIsoiMod == 2) /* v3.2 */ /* v3.1 ideal FD: Vbsitf is pinned at Vbs0 */
                {
                    Vbs = Vbsitf = Vbs0 + OFF_Vbsitf;
                    dVbsitf_dVg = dVbs0_dVg;
                    dVbsitf_dVd = dVbs0_dVd;
                    dVbsitf_dVe = dVbs0_dVe;
                    dVbsitf_dVb = 0.0;
                    if (selfheat) dVbsitf_dT = dVbs0_dT;
                    else dVbsitf_dT = 0;
                }
                else /* soiMod = 1 */
                {
                    T1 = Vbs - (Vbs0 + OFF_Vbsitf) - 0.01;
                    T2 = sqrt(T1*T1 + 0.0001);
                    T3 = 0.5 * (1 + T1/T2);
                    Vbsitf = (Vbs0 + OFF_Vbsitf) + 0.5 * (T1 + T2);
                    dVbsitf_dVg = (1 - T3) * dVbs0_dVg;
                    dVbsitf_dVd = (1 - T3) * dVbs0_dVd;
                    dVbsitf_dVe = (1 - T3) * dVbs0_dVe;
                    dVbsitf_dVb = T3 ;
                    if (selfheat)  dVbsitf_dT = (1 - T3) * dVbs0_dT;
                    else  dVbsitf_dT = 0.0;
                }

                /* Based on Vbsitf, calculate zero-field body potential for MOS: Vbsmos */
                T1 = Vbs0t - Vbsitf - 0.005;
                T2 = sqrt(T1 * T1 + (2.5e-5));
                T3 = 0.5 * (T1 + T2);
                T4 = T3 * model->B4SOIcsi / pParam->B4SOIqsi; /* v3.2 */
                Vbsmos = Vbsitf - 0.5 * T3 * T4;
                T5 = 0.5 * T4 * (1 + T1 / T2);
                dVbsmos_dVg = dVbsitf_dVg * (1 + T5) - T5 * dVbs0t_dVg;
                dVbsmos_dVd = dVbsitf_dVd * (1 + T5) - T5 * dVbs0t_dVd;
                dVbsmos_dVb = dVbsitf_dVb * (1 + T5);
                dVbsmos_dVe = dVbsitf_dVe * (1 + T5) - T5 * dVbs0t_dVe;
                if (selfheat)
                    dVbsmos_dT = dVbsitf_dT * (1 + T5) - T5 * dVbs0t_dT;
                else
                    dVbsmos_dT = 0.0;
                /* Vbsmos should be used in MOS after some limiting (Vbseff) */


                Vbp = Vbs - Vps;
                dVbp_dVb = 1;
            }
            /* end of v3.0 block edition */


            /* v3.0 modification */
            /* T2 is Vbsmos limited above Vbsc=-5 */
            T0 = Vbsmos + 5 - 0.001;
            T1 = sqrt(T0 * T0 - 0.004 * (-5));
            T2 = (-5) + 0.5 * (T0 + T1);
            dT2_dVb = (0.5 * (1.0 + T0 / T1)) * dVbsmos_dVb;
            dT2_dVg = (0.5 * (1.0 + T0 / T1)) * dVbsmos_dVg;
            dT2_dVd = (0.5 * (1.0 + T0 / T1)) * dVbsmos_dVd;
            dT2_dVe = (0.5 * (1.0 + T0 / T1)) * dVbsmos_dVe;
            if (selfheat) dT2_dT = (0.5 * (1.0 + T0 / T1)) * dVbsmos_dT;
            else  dT2_dT = 0.0;

            /* Vbsh is T2 limited below 1.5 */
            T0 = 1.5;
            T1 = T0 - T2 - 0.002;
            T3 = sqrt(T1 * T1 + 0.008 * T0);
            Vbsh = T0 - 0.5 * (T1 + T3);
            dVbsh_dVb = 0.5 * (1.0 + T1 / T3) * dT2_dVb;
            dVbsh_dVg = 0.5 * (1.0 + T1 / T3) * dT2_dVg;
            dVbsh_dVd = 0.5 * (1.0 + T1 / T3) * dT2_dVd;
            dVbsh_dVe = 0.5 * (1.0 + T1 / T3) * dT2_dVe;
            if (selfheat) dVbsh_dT = 0.5 * (1.0 + T1 / T3) * dT2_dT;
            else  dVbsh_dT = 0.0;


            /* Vbseff is Vbsh limited to 0.95*phi */
            T0 = 0.95 * phi;
            T1 = T0 - Vbsh - 0.002;
            T2 = sqrt(T1 * T1 + 0.008 * T0);
            Vbseff = T0 - 0.5 * (T1 + T2);
            dVbseff_dVb = 0.5 * (1.0 + T1 / T2) * dVbsh_dVb;
            dVbseff_dVg = 0.5 * (1.0 + T1 / T2) * dVbsh_dVg;
            dVbseff_dVd = 0.5 * (1.0 + T1 / T2) * dVbsh_dVd;
            dVbseff_dVe = 0.5 * (1.0 + T1 / T2) * dVbsh_dVe;
            /* if (selfheat)  dVbseff_dT = 0.5 * (1.0 + T1 / T2) * dVbsh_dT; */
            if (selfheat)
            {
                dT0_dT = 0.95 * dphi_dT;
                dT1_dT = dT0_dT - dVbsh_dT;
                dVbseff_dT = dT0_dT - 0.5 * (1.0 + T1 / T2) * dT1_dT
                             - 0.002 * dT0_dT / T2;
            } /* v4.1 */
            else  dVbseff_dT = 0.0;
            here->B4SOIvbseff = Vbseff; /* SPICE sol. */
            /* end of v3.0 modification */


            /* Below all the variables refer to Vbseff */
            if (dVbseff_dVb < 1e-20)
            {
                dVbseff_dVb = 1e-20;
                dVbsh_dVb *= 1e20;
            }
            else
                dVbsh_dVb /= dVbseff_dVb;

            Phis = phi - Vbseff;
//            dPhis_dVb = -1;
            sqrtPhis = sqrt(Phis);
            dsqrtPhis_dVb = -0.5 / sqrtPhis;

            Xdep = Xdep0 * sqrtPhis / sqrtPhi;
            dXdep_dVb = (Xdep0 / sqrtPhi) * dsqrtPhis_dVb;
            /* v4.1 */
            if (selfheat)
            {
                dPhis_dT = dphi_dT - dVbseff_dT;
                dsqrtPhis_dT = 0.5 / sqrtPhis * dPhis_dT;
                /*    dXdep_dT = dXdep0_dT * sqrtPhis / sqrtPhi
                      + (dsqrtPhis_dT * sqrtPhi - sqrtPhis * dsqrtPhi_dT) / phi; v4.2 Temp Deriv bugfix */
                dXdep_dT = dXdep0_dT * sqrtPhis / sqrtPhi
                           + Xdep0 * (dsqrtPhis_dT * sqrtPhi - sqrtPhis * dsqrtPhi_dT) / phi;
            }
            else
            {
                dPhis_dT = 0.0;
                dsqrtPhis_dT = 0.0;
                dXdep_dT = 0.0;
            } /* end v4.1 */

            /* Calculate nstar v3.2 */
            here->B4SOInstar = model->B4SOIvtm / Charge_q *
                               (model->B4SOIcox + epssub / Xdep + pParam->B4SOIcit);

            /* Vth Calculation */
            T3 = sqrt(Xdep);

            T0 = pParam->B4SOIdvt2 * Vbseff;
            if (T0 >= - 0.5)
            {
                T1 = 1.0 + T0;
                T2 = pParam->B4SOIdvt2 ;
            }
            else /* Added to avoid any discontinuity problems caused by dvt2 */
            {
                T4 = 1.0 / (3.0 + 8.0 * T0);
                T1 = (1.0 + 3.0 * T0) * T4;
                T2 = pParam->B4SOIdvt2 * T4 * T4 ;
            }
            lt1 = model->B4SOIfactor1 * T3 * T1;
            dlt1_dVb =model->B4SOIfactor1 * (0.5 / T3 * T1 * dXdep_dVb + T3 * T2);
            if (selfheat) dlt1_dT = model->B4SOIfactor1 * T1 * 0.5 / T3 * dXdep_dT;
            else dlt1_dT = 0.0; /* v4.1 */

            T0 = pParam->B4SOIdvt2w * Vbseff;
            if (T0 >= - 0.5)
            {
                T1 = 1.0 + T0;
                T2 = pParam->B4SOIdvt2w ;
            }
            else /* Added to avoid any discontinuity problems caused by dvt2w */
            {
                T4 = 1.0 / (3.0 + 8.0 * T0);
                T1 = (1.0 + 3.0 * T0) * T4;
                T2 = pParam->B4SOIdvt2w * T4 * T4 ;
            }
            ltw= model->B4SOIfactor1 * T3 * T1;
            dltw_dVb=model->B4SOIfactor1*(0.5 / T3 * T1 * dXdep_dVb + T3 * T2);
            if (selfheat) dltw_dT = model->B4SOIfactor1 * T1 * 0.5 / T3 * dXdep_dT;
            else dltw_dT = 0.0; /* v4.1 */
            T0 = -0.5 * pParam->B4SOIdvt1 * Leff / lt1;
            if (T0 > -EXPL_THRESHOLD)
            {
                T1 = exp(T0);
                Theta0 = T1 * (1.0 + 2.0 * T1);
                dT1_dVb = -T0 / lt1 * T1 * dlt1_dVb;
                dTheta0_dVb = (1.0 + 4.0 * T1) * dT1_dVb;
                dT1_dT = -T0 / lt1 * T1 * dlt1_dT;   /* v4.2 bugfix temp deriv */
                dTheta0_dT = (1.0 + 4.0 * T1) * dT1_dT; /* v4.2 bugfix temp deriv */
            }
            else
            {
                T1 = MIN_EXPL;
                Theta0 = T1 * (1.0 + 2.0 * T1);
                dTheta0_dVb = 0.0;
                dTheta0_dT = 0; /* v4.2 bugfix temp deriv */
            }

            /* Calculate n */
            T2 = pParam->B4SOInfactor * epssub / Xdep;
            dT2_dVb = - T2 / Xdep * dXdep_dVb;
            dT2_dT = - T2 / Xdep * dXdep_dT; /* v4.2 bugfix temp deriv */
            T3 = pParam->B4SOIcdsc + pParam->B4SOIcdscb * Vbseff
                 + pParam->B4SOIcdscd * Vds;
            dT3_dVb = pParam->B4SOIcdscb;
            dT3_dVd = pParam->B4SOIcdscd;

            T4 = (T2 + T3 * Theta0 + pParam->B4SOIcit) / model->B4SOIcox;
            dT4_dVb = (dT2_dVb + Theta0 * dT3_dVb + dTheta0_dVb * T3)
                      / model->B4SOIcox;
            dT4_dVd = Theta0 * dT3_dVd / model->B4SOIcox;

            dT4_dT = (dT2_dT + dTheta0_dT* T3)/ model->B4SOIcox; /* v4.2 bugfix temp deriv */
            if (T4 >= -0.5)
            {
                n = 1.0 + T4;
                dn_dVb = dT4_dVb;
                dn_dVd = dT4_dVd;
                dn_dT = dT4_dT; /* v4.2 bugfix temp deriv */
            }
            else
                /* avoid  discontinuity problems caused by T4 */
            {
                T0 = 1.0 / (3.0 + 8.0 * T4);
                /* n = (1.0 + 3.0 * T4) * T0; */ /* v4.2 bugfix temp deriv */
                T5 = 1.0 + 3.0 * T4; /* v4.2 bugfix temp deriv */
                n = T0 * T5; /* v4.2 bugfix temp deriv */
                T0 *= T0;
                dn_dVb = T0 * dT4_dVb;
                dn_dVd = T0 * dT4_dVd;
                dn_dT = T0 * (3.0 - 8.0 * T5 * T0) * dT4_dT; /* v4.2 bugfix temp deriv */
            }

            /* v4.0 DITS */
            if (pParam->B4SOIdvtp0 > 0.0)
            {
                T0 = -pParam->B4SOIdvtp1 * Vds;
                if (T0 < -EXPL_THRESHOLD)
                {
                    T2 = MIN_EXPL;
                    dT2_dVd = 0.0;
                }
                else
                {
                    T2 = exp(T0);
                    dT2_dVd = -pParam->B4SOIdvtp1 * T2;
                }

                T3 = Leff + pParam->B4SOIdvtp0 * (1.0 + T2);
                dT3_dVd = pParam->B4SOIdvtp0 * dT2_dVd;
                T4 = Vtm * log(Leff / T3);
                dT4_dVd = -Vtm * dT3_dVd / T3;
                DITS_Sft = n * T4;
                dDITS_Sft_dVd = dn_dVd * T4 + n * dT4_dVd;
                dDITS_Sft_dVb = T4 * dn_dVb;
                if (selfheat)
                {
                    /* dDITS_Sft_dT = n * KboQ * log(Leff / T3); */ /* v4.2 bugfix temp deriv */
                    dDITS_Sft_dT = n * KboQ * log(Leff / T3) + dn_dT * T4; /* v4.2 bugfix temp deriv */
                }
                else
                    dDITS_Sft_dT = 0.0;
            }
            else
            {
                DITS_Sft = dDITS_Sft_dVd = dDITS_Sft_dVb = 0.0;
                dDITS_Sft_dT = 0.0;
            }

            here->B4SOIthetavth = pParam->B4SOIdvt0 * Theta0;
            Delt_vth = here->B4SOIthetavth * V0;
            dDelt_vth_dVb = pParam->B4SOIdvt0 * dTheta0_dVb * V0;
            if (selfheat)  /* dDelt_vth_dT = here->B4SOIthetavth * dvbi_dT; */ /* v4.2 bugfix temp deriv */
                dDelt_vth_dT = pParam->B4SOIdvt0 * (dTheta0_dT * V0 + Theta0 * (dvbi_dT - dphi_dT)); /* v4.2 bugfix temp deriv */
            else  dDelt_vth_dT = 0.0;

            T0 = -0.5 * pParam->B4SOIdvt1w * pParam->B4SOIweff
                 * Leff / ltw;
            if (T0 > -EXPL_THRESHOLD)
            {
                T1 = exp(T0);
                T2 = T1 * (1.0 + 2.0 * T1);
                dT1_dVb = -T0 / ltw * T1 * dltw_dVb;
                dT2_dVb = (1.0 + 4.0 * T1) * dT1_dVb;
                dT1_dT = -T0 / ltw * T1 * dltw_dT; /* v4.2 bugfix temp deriv */
                dT2_dT = (1.0 + 4.0 * T1) * dT1_dT;/* v4.2 bugfix temp deriv */
            }
            else
            {
                T1 = MIN_EXPL;
                T2 = T1 * (1.0 + 2.0 * T1);
                dT2_dVb = 0.0;
            }

            T0 = pParam->B4SOIdvt0w * T2;
            DeltVthw = T0 * V0;
            dDeltVthw_dVb = pParam->B4SOIdvt0w * dT2_dVb * V0;
            if (selfheat)   /*dDeltVthw_dT = T0 * dvbi_dT;*/ /* v4.2 bugfix temp deriv */
                dDeltVthw_dT = T0 * (dvbi_dT - dphi_dT) + pParam->B4SOIdvt0w * dT2_dT * V0; /* v4.2 bugfix temp deriv */
            else   dDeltVthw_dT = 0.0;

            T0 = sqrt(1.0 + pParam->B4SOIlpe0 / Leff);
            T1 = (pParam->B4SOIkt1 + pParam->B4SOIkt1l / Leff
                  + pParam->B4SOIkt2 * Vbseff);
            DeltVthtemp = pParam->B4SOIk1ox * (T0 - 1.0) * sqrtPhi
                          + T1 * TempRatioMinus1; /* v4.0 */
            if (selfheat)
                /*dDeltVthtemp_dT = T1 / model->B4SOItnom; */ /* v4.2 bugfix temp deriv */
                dDeltVthtemp_dT = pParam->B4SOIk1ox * (T0 - 1.0) * dsqrtPhi_dT + T1 / model-> B4SOItnom; /* v4.2 bugfix temp deriv */
            else
                dDeltVthtemp_dT = 0.0;

            tmp2 = toxe * phi / (pParam->B4SOIweff + pParam->B4SOIw0);
            dtmp2_dT = toxe * dphi_dT / (pParam->B4SOIweff + pParam->B4SOIw0); /* v4.2 bugfix temp deriv */
            T3 = here->B4SOIeta0 + pParam->B4SOIetab * Vbseff;
            if (T3 < 1.0e-4) /* avoid  discontinuity problems caused by etab */
            {
                T9 = 1.0 / (3.0 - 2.0e4 * T3);
                T3 = (2.0e-4 - T3) * T9;
                T4 = T9 * T9 * pParam->B4SOIetab;
                dT3_dVb = T4 ;
            }
            else
            {
                dT3_dVb = pParam->B4SOIetab ;
            }
            /* DIBL_Sft = T3 * pParam->B4SOItheta0vb0 * Vds;
               dDIBL_Sft_dVd = pParam->B4SOItheta0vb0 * T3;
               dDIBL_Sft_dVb = pParam->B4SOItheta0vb0 * Vds * dT3_dVb; */ /* v4.2 bugfix */
            DIBL_Sft = T3 * theta0vb0 * Vds;
            dDIBL_Sft_dVd = theta0vb0 * T3;
            dDIBL_Sft_dVb = theta0vb0 * Vds * dT3_dVb;
            dDIBL_Sft_dT = T3 * Vds * dtheta0vb0_dT; /* v4.2 bug fix */
            Lpe_Vb = sqrt(1.0 + pParam->B4SOIlpeb / Leff);

            T9 =  2.2361 / sqrtPhi;
            sqrtPhisExt = sqrtPhis - T9 * (Vbsh - Vbseff);
            dsqrtPhisExt_dVb = dsqrtPhis_dVb - T9 * (dVbsh_dVb - 1);
            dsqrtPhisExt_dT = dsqrtPhis_dT - T9 * (dVbsh_dT)
                              + 2.2361 * dsqrtPhi_dT * (Vbsh - Vbseff) / phi; /* v4.2 bugfix temp deriv */
            /* 4.1 */
            T0 = exp(2.0 * pParam->B4SOIdvtp4 * Vds);
            DITS_Sft2 = pParam->B4SOIdvtp2factor * (T0-1) / (T0+1);
            dDITS_Sft2_dVd = pParam->B4SOIdvtp2factor * pParam->B4SOIdvtp4 * 4.0 * T0 / ((T0+1) * (T0+1));

            Vth = model->B4SOItype * here->B4SOIvth0
                  + (pParam->B4SOIk1ox * sqrtPhisExt
                     - pParam->B4SOIk1eff * sqrtPhi) * Lpe_Vb
                  - here->B4SOIk2ox * Vbseff- Delt_vth - DeltVthw
                  +(pParam->B4SOIk3 + pParam->B4SOIk3b * Vbseff) * tmp2
                  + DeltVthtemp - DIBL_Sft - DITS_Sft - DITS_Sft2;

            if (selfheat)
            {
                if (!model->B4SOIvth0Given)
                    dvth0_dT = dphi_dT + pParam->B4SOIk1eff * dsqrtPhi_dT;
                else dvth0_dT=0;
            }

            else dvth0_dT=0;        /* v4.2 temp deriv */



            here->B4SOIvon = Vth;

            T6 = pParam->B4SOIk3b * tmp2 - here->B4SOIk2ox
                 + pParam->B4SOIkt2 * TempRatioMinus1;
            dVth_dVb = Lpe_Vb * pParam->B4SOIk1ox * dsqrtPhisExt_dVb
                       - dDelt_vth_dVb - dDeltVthw_dVb
                       + T6 - dDIBL_Sft_dVb - dDITS_Sft_dVb;
            /*  this is actually dVth_dVbseff  */

            dVth_dVd = -dDIBL_Sft_dVd - dDITS_Sft_dVd - dDITS_Sft2_dVd;

            if (selfheat)
                /*    dVth_dT = dDeltVthtemp_dT - dDelt_vth_dT - dDeltVthw_dT
                      - dDITS_Sft_dT; */
                dVth_dT = dDeltVthtemp_dT - dDelt_vth_dT - dDeltVthw_dT - dDITS_Sft_dT
                          +(pParam->B4SOIk1ox * dsqrtPhisExt_dT- pParam->B4SOIk1eff * dsqrtPhi_dT) * Lpe_Vb
                          + model->B4SOItype * dvth0_dT - dDIBL_Sft_dT;  /* v4.2 temp deriv */

            else  dVth_dT = 0.0;


            /* dVthzb_dT calculation */
            if ((model->B4SOIcapMod == 3) && (selfheat == 1))
            {
                T3zb = sqrt(Xdep0);
                ltwzb = lt1zb = model->B4SOIfactor1 * T3zb;
                dT3zb_dT = 1.0 / (2.0 * T3zb) * dXdep0_dT; /* v4.2 bugfix temp deriv */
                dltwzb_dT = dlt1zb_dT = model->B4SOIfactor1 * dT3zb_dT; /* v4.2 bugfix temp deriv */
                T0 = -0.5 * pParam->B4SOIdvt1 * Leff / lt1zb;
                if (T0 > -EXPL_THRESHOLD)
                {
                    T1 = exp(T0);

                    Theta0zb = T1 * (1.0 + 2.0 * T1);
                    dT0_dT = -(T0 / lt1zb) * dlt1zb_dT; /* v4.2 bugfix temp deriv */
                    dT1_dT = T1 * dT0_dT; /* v4.2 bugfix temp deriv */
                    dTheta0zb_dT = (1.0 + 4.0 * T1) * dT1_dT; /* v4.2 bugfix temp deriv */
                }
                else
                {
                    T1 = MIN_EXPL;
                    Theta0zb = T1 * (1.0 + 2.0 * T1);
                    dTheta0zb_dT=0; /* v4.2 bugfix temp deriv */
                }
                Delt_vthzb = pParam->B4SOIdvt0 * Theta0zb * V0;
                /* dDelt_vthzb_dT = pParam->B4SOIdvt0 * Theta0zb * dvbi_dT; */ /* v4.2 bugfix temp deriv */
                dDelt_vthzb_dT = pParam->B4SOIdvt0 *( Theta0zb * (dvbi_dT - dphi_dT)
                                                      + dTheta0zb_dT *V0); /* v4.2 bugfix temp deriv */

                T0 = -0.5 * pParam->B4SOIdvt1w * pParam->B4SOIweff * Leff / ltwzb;
                if (T0 > -EXPL_THRESHOLD)
                {
                    T1 = exp(T0);
                    T2 = T1 * (1.0 + 2.0 * T1);
                    dT0_dT = -(T0 / ltwzb) * dltwzb_dT; /* v4.2 bugfix temp deriv */
                    dT1_dT = T1 * dT0_dT; /* v4.2 bugfix temp deriv */
                    dT2_dT = (1.0 + 4.0 * T1) * dT1_dT; /* v4.2 bugfix temp deriv */
                }
                else
                {
                    T1 = MIN_EXPL;
                    T2 = T1 * (1.0 + 2.0 * T1);
                    dT2_dT=0; /* v4.2 bugfix temp deriv */
                }
                T0 = pParam->B4SOIdvt0w * T2;
                dT0_dT= pParam->B4SOIdvt0w * dT2_dT; /* v4.2 bugfix temp deriv */
                DeltVthwzb = T0 * V0;
                /* dDeltVthwzb_dT = T0 * dvbi_dT; *//* v4.2 bugfix temp deriv */
                dDeltVthwzb_dT = ( T0 * (dvbi_dT - dphi_dT)+ dT0_dT *V0); /* v4.2 bugfix temp deriv */

                T0 = sqrt(1.0 + pParam->B4SOIlpe0 / Leff);
                T1 = (pParam->B4SOIkt1 + pParam->B4SOIkt1l / Leff);
                DeltVthtempzb = pParam->B4SOIk1ox * (T0 - 1.0) * sqrtPhi
                                + T1 * TempRatioMinus1;
                dDeltVthtempzb_dT = pParam->B4SOIk1ox * (T0 - 1.0) * dsqrtPhi_dT + T1 / model->B4SOItnom; /* v4.2 bugfix temp deriv */

                Vthzb = model->B4SOItype * here->B4SOIvth0
                        - Delt_vthzb - DeltVthwzb + pParam->B4SOIk3 * tmp2
                        + DeltVthtempzb;
                dVthzb_dT = dDeltVthtempzb_dT - dDelt_vthzb_dT - dDeltVthwzb_dT;
                dVthzb_dT = model->B4SOItype * dvth0_dT - dDelt_vthzb_dT - dDeltVthwzb_dT + pParam->B4SOIk3 * dtmp2_dT + dDeltVthtempzb_dT; /* v4.2 bugfix temp deriv */
                /* Vthzb2 = Vthzb + 1.12; */ /* v4.1 */ /* v4.2 never used */
            }
            /* Effective Vgst (Vgsteff) Calculation */

            Vgst = Vgs_eff - Vth;
            dVgst_dVg = dVgs_eff_dVg;
            dVgst_dVd = -dVth_dVd;
            dVgst_dVb = -dVth_dVb;

            T10 = n * Vtm; /* v4.0 */
            VgstNVt = pParam->B4SOImstar * Vgst / T10; /* v4.0 */
            ExpArg = (pParam->B4SOIvoff - (1- pParam->B4SOImstar) * Vgst)
                     / T10;              /* v4.0 */

            /* MCJ: Very small Vgst */
            if (VgstNVt > EXPL_THRESHOLD)
            {
                Vgsteff = Vgst;
                /* T0 is dVgsteff_dVbseff */
                T0 = -dVth_dVb;
                dVgsteff_dVg = dVgs_eff_dVg + T0 * dVbseff_dVg; /* v3.0 */
                dVgsteff_dVd = -dVth_dVd + T0 * dVbseff_dVd; /* v3.0 */
                dVgsteff_dVb = T0 * dVbseff_dVb;
                dVgsteff_dVe = T0 * dVbseff_dVe; /* v3.0 */
                if (selfheat)
                    dVgsteff_dT  = -dVth_dT + T0 * dVbseff_dT; /* v3.0 */
                else
                    dVgsteff_dT = 0.0;
            }
            else if (ExpArg > EXPL_THRESHOLD)
            {
                T0 = (Vgst - pParam->B4SOIvoff) / (n * Vtm);
                ExpVgst = exp(T0);
                /*Vgsteff = Vtm * pParam->B4SOIcdep0 / model->B4SOIcox * ExpVgst; *//*v4.2 bug fix */
                Vgsteff = Vtm * cdep0 / model->B4SOIcox * ExpVgst; /* v4.2 bug fix */
                T3 = Vgsteff / (n * Vtm) ;
                /* T1 is dVgsteff_dVbseff */
                T1  = -T3 * (dVth_dVb + T0 * Vtm * dn_dVb);
                dVgsteff_dVg = T3 * dVgs_eff_dVg+ T1 * dVbseff_dVg; /* v3.0 */
                dVgsteff_dVd = -T3 * (dVth_dVd + T0 * Vtm * dn_dVd)+ T1 * dVbseff_dVd; /* v3.0 */
                dVgsteff_dVe = T1 * dVbseff_dVe; /* v3.0 */
                dVgsteff_dVb = T1 * dVbseff_dVb;
                if (selfheat)
                    /* dVgsteff_dT = -T3 * (dVth_dT + T0 * dVtm_dT * n)
                       + Vgsteff / Temp+ T1 * dVbseff_dT; */ /* v3.0 */ /* v4.2 temp deriv*/
                    dVgsteff_dT = -T3 * (dVth_dT + T0 * dVtm_dT * n + Vtm * dn_dT)
                                  + Vgsteff / Temp+ T1 * dVbseff_dT; /*v4.2 temp deriv*/
                else
                    dVgsteff_dT = 0.0;
            }
            else
            {
                ExpVgst = exp(VgstNVt);
                T1 = T10 * log(1.0 + ExpVgst);
                dT1_dVg = ExpVgst / (1.0 + ExpVgst) * pParam->B4SOImstar;
                dT1_dVb = -dT1_dVg * (dVth_dVb + Vgst / n * dn_dVb)
                          + T1 / n * dn_dVb;
                dT1_dVd = -dT1_dVg * (dVth_dVd + Vgst / n * dn_dVd)
                          + T1 / n * dn_dVd;
                /*T3 = (1.0 / Temp); */
                T3 = (1.0 / Temp + dn_dT / n); /* v4.2 temp deriv */
                if (selfheat)
                    dT1_dT = -dT1_dVg * (dVth_dT + Vgst * T3) + T1 * T3;
                else
                    dT1_dT = 0.0;

                /*dT2_dVg = -model->B4SOIcox / (Vtm * pParam->B4SOIcdep0)
                 * exp(ExpArg) * (1 - pParam->B4SOImstar);*/ /*v4.2 bug fix*/
                dT2_dVg = -model->B4SOIcox / (Vtm * cdep0)
                          * exp(ExpArg) * (1 - pParam->B4SOImstar); /*v4.2 bug fix*/
                T2 = pParam->B4SOImstar - T10 * dT2_dVg
                     / (1.0 - pParam->B4SOImstar);
                dT2_dVd = -dT2_dVg * (dVth_dVd - Vtm * ExpArg * dn_dVd
                                      / (1.0 - pParam->B4SOImstar))
                          + (T2 - pParam->B4SOImstar) / n * dn_dVd;
                dT2_dVb = -dT2_dVg * (dVth_dVb - Vtm * ExpArg * dn_dVb
                                      / (1.0 - pParam->B4SOImstar))
                          + (T2 - pParam->B4SOImstar) / n * dn_dVb;
                if (selfheat)
                    dT2_dT = -dT2_dVg * ( dVth_dT - ExpArg * T10 * T3
                                          / (1.0 - pParam->B4SOImstar) );
                else
                    dT2_dT = 0.0;

                Vgsteff = T1 / T2;
                T3 = T2 * T2;
                /*  T4 is dVgsteff_dVbseff  */
                T4 = (T2 * dT1_dVb - T1 * dT2_dVb) / T3;
                dVgsteff_dVb = T4 * dVbseff_dVb;
                dVgsteff_dVe = T4 * dVbseff_dVe; /* v3.0 */
                dVgsteff_dVg = (T2 * dT1_dVg - T1 * dT2_dVg)
                               / T3 * dVgs_eff_dVg
                               + T4 * dVbseff_dVg; /* v3.0 */
                dVgsteff_dVd = (T2 * dT1_dVd - T1 * dT2_dVd)
                               / T3+ T4 * dVbseff_dVd; /* v3.0 */
                if (selfheat)
                    dVgsteff_dT = (T2 * dT1_dT - T1 * dT2_dT)
                                  / T3+ T4 * dVbseff_dT; /* v3.0 */
                else
                    dVgsteff_dT = 0.0;
            }
            Vgst2Vtm = Vgsteff + 2.0 * Vtm;
            if (selfheat)  dVgst2Vtm_dT = dVgsteff_dT + 2.0 * dVtm_dT; /* v3.1.1 bug fix */
            else  dVgst2Vtm_dT = 0.0;
            here->B4SOIVgsteff = Vgsteff; /* v2.2.3 bug fix */

            /* v4.0 F-factor (degradation factor due to pocket implant) */
            if (pParam->B4SOIfprout <= 0.0)
            {
                FP = 1.0;
                dFP_dVg = dFP_dT = 0.0;
            }
            else
            {
                T9 = pParam->B4SOIfprout * sqrt(Leff) / Vgst2Vtm;
                FP = 1.0 / (1.0 + T9);
                dFP_dVg = FP * FP * T9 / Vgst2Vtm;
                if (selfheat)       dFP_dT = dFP_dVg * dVgst2Vtm_dT;
                else dFP_dT = 0.0;
            }

            /* Calculate Effective Channel Geometry */
            T9 = sqrtPhis - sqrtPhi;
            Weff = pParam->B4SOIweff - (2.0 - here->B4SOInbc)
                   * (pParam->B4SOIdwg * Vgsteff + pParam->B4SOIdwb * T9);
            dWeff_dVg = -(2.0 - here->B4SOInbc) * pParam->B4SOIdwg;
            dWeff_dVb = -(2.0 - here->B4SOInbc) * pParam->B4SOIdwb
                        * dsqrtPhis_dVb;

            if (Weff < 2.0e-8) /* to avoid the discontinuity problem due to Weff*/
            {
                T0 = 1.0 / (6.0e-8 - 2.0 * Weff);
                Weff = 2.0e-8 * (4.0e-8 - Weff) * T0;
                T0 *= T0 * 4.0e-16;
                dWeff_dVg *= T0;
                dWeff_dVb *= T0;
            }

            if (model->B4SOIrdsMod == 1)    /* v4.0 */
                Rds = dRds_dVg = dRds_dVb = dRds_dT = 0.0;
            else
            {
                T0 = pParam->B4SOIprwg * Vgsteff
                     + pParam->B4SOIprwb * T9;
                if (T0 >= -0.9)
                {
                    Rds = rds0 * (1.0 + T0);
                    dRds_dVg = rds0 * pParam->B4SOIprwg;
                    dRds_dVb = rds0 * pParam->B4SOIprwb * dsqrtPhis_dVb;

                    if (selfheat && (Rds!=0.0))
                        dRds_dT = (1.0 + T0) * drds0_dT;
                    else  dRds_dT = 0.0;

                }
                else
                    /* to avoid the discontinuity problem due to prwg and prwb*/
                {
                    T1 = 1.0 / (17.0 + 20.0 * T0);
                    Rds = rds0 * (0.8 + T0) * T1;
                    T1 *= T1;
                    dRds_dVg = rds0 * pParam->B4SOIprwg * T1;
                    dRds_dVb = rds0 * pParam->B4SOIprwb * dsqrtPhis_dVb
                               * T1;

                    if (selfheat && (Rds!=0.0))
                        dRds_dT = (0.8 + T0) * T1 * drds0_dT;
                    else  dRds_dT = 0.0;

                }
                /* here->B4SOIrds = Rds; */ /* v2.2.3 bug fix */ /* v4.2 bugfix # 39 */
            }
            here->B4SOIrds = Rds; /* v4.2 bugfix # 39 */
            /* Calculate Abulk */
            if (pParam->B4SOIa0 == 0.0)
            {

                Abulk0 = Abulk = 1.0;

                dAbulk0_dVb = dAbulk_dVg = dAbulk_dVb = 0.0;
            }
            else
            {
                T10 = pParam->B4SOIketa * Vbsh;
                if (T10 >= -0.9)
                {
                    T11 = 1.0 / (1.0 + T10);
                    dT11_dVb = -pParam->B4SOIketa * T11 * T11 * dVbsh_dVb;
                }
                else   /* added to avoid the problems caused by Keta */
                {
                    T12 = 1.0 / (0.8 + T10);
                    T11 = (17.0 + 20.0 * T10) * T12;
                    dT11_dVb = -pParam->B4SOIketa * T12 * T12 * dVbsh_dVb;
                }

                /* v3.0 bug fix */
                T10 = phi + pParam->B4SOIketas;

                T13 = (Vbsh * T11) / T10;
                dT13_dVb = (Vbsh * dT11_dVb + T11 * dVbsh_dVb) / T10;

                /* limit 1/sqrt(1-T13) to 6, starting at T13=0.96 */
                if (T13 < 0.96)
                {
                    T14 = 1 / sqrt(1-T13);
                    T10 = 0.5 * T14 / (1-T13);
                    dT14_dVb = T10 * dT13_dVb;
                }
                else
                {
                    /* IBM tweak */
                    T11 = 1.0 / (1.0 - 1.0593220339*T13);
                    T14 = (6.0169491525 - 6.3559322034 * T13) * T11;
                    T10 = 0.0179546 * T11 * T11;
                    dT14_dVb = T10 * dT13_dVb;
                }

                /* v3.0 bug fix */
                /*                      T10 = 0.5 * pParam->B4SOIk1eff
                                        / sqrt(phi + pParam->B4SOIketas); */
                T10 = 0.5 * pParam->B4SOIk1ox * Lpe_Vb
                      / sqrt(phi + pParam->B4SOIketas);       /* v4.0 */

                T1 = T10 * T14;
                dT1_dVb = T10 * dT14_dVb;

                T9 = sqrt(pParam->B4SOIxj * Xdep);
                tmp1 = Leff + 2.0 * T9;
                T5 = Leff / tmp1;
                tmp2 = pParam->B4SOIa0 * T5;
                tmp3 = pParam->B4SOIweff + pParam->B4SOIb1;
                tmp4 = pParam->B4SOIb0 / tmp3;
                T2 = tmp2 + tmp4;
                dT2_dVb = -T9 * tmp2 / tmp1 / Xdep * dXdep_dVb;
                T6 = T5 * T5;
                T7 = T5 * T6;

                Abulk0 = 1 + T1 * T2;
                dAbulk0_dVb = T1 * dT2_dVb + T2 * dT1_dVb;

                T8 = pParam->B4SOIags * pParam->B4SOIa0 * T7;
                dAbulk_dVg = -T1 * T8;
                Abulk = Abulk0 + dAbulk_dVg * Vgsteff;

                dAbulk_dVb = dAbulk0_dVb
                             - T8 * Vgsteff * (dT1_dVb + 3.0 * T1 * dT2_dVb / tmp2);
            }

            if (Abulk0 < 0.01)
            {
                T9 = 1.0 / (3.0 - 200.0 * Abulk0);
                Abulk0 = (0.02 - Abulk0) * T9;
                dAbulk0_dVb *= T9 * T9;
            }

            if (Abulk < 0.01)
            {
                T9 = 1.0 / (3.0 - 200.0 * Abulk);
                Abulk = (0.02 - Abulk) * T9;
                dAbulk_dVb *= T9 * T9;
                T10 = T9 * T9;      /* 3.2 bug fix */
                dAbulk_dVg *= T10;  /* 3.2 bug fix */
            }

            here->B4SOIAbulk = Abulk; /*v3.2 for noise */

            /* Mobility calculation */
            if (model->B4SOImtrlMod)
            {
                T14 = 2.0 * model->B4SOItype *(model->B4SOIphig - model->B4SOIeasub - 0.5 * Eg + 0.45);
                toxe_mob = model->B4SOIeot * model->B4SOIepsrsub / 3.9;
            }                    /* Bug fix #4 Jun 09 implementing Eeff correctly*/
            else
            {
                T14 = 0.0;
                toxe_mob = model->B4SOItox;
            }                                                                                /* Bug fix #4 Jun 09 implementing Eeff correctly*/
            if (model->B4SOImobMod == 1)
            {
                T0 = Vgsteff + Vth + Vth - T14;
                T2 = ua + uc * Vbseff;
                T3 = T0 / toxe_mob;                                                                                     /* Bug fix #4 Jun 09 implementing Eeff correctly*/
                T5 = T3 * (T2 + ub * T3);
                dDenomi_dVg = (T2 + 2.0 * ub * T3) / toxe_mob;                          /* Bug fix #4 Jun 09 implementing Eeff correctly*/
                dDenomi_dVd = dDenomi_dVg * 2 * dVth_dVd;
                dDenomi_dVb = dDenomi_dVg * 2 * dVth_dVb + uc * T3 ;
                if (selfheat)
                    dDenomi_dT = dDenomi_dVg * 2 * dVth_dT
                                 + (dua_dT + Vbseff * duc_dT
                                    + dub_dT * T3 ) * T3;
                else
                    dDenomi_dT = 0.0;
            }
            else if (model->B4SOImobMod == 2)                                                                   /* Bug fix #5 Jun 09 implementing Eeff correctly*/
            {
                T5 = (Vgsteff -T14)/ toxe * (ua                                                         /* MobMod=2 does not use Eeff */
                                             + uc * Vbseff + ub * (Vgsteff -T14)                                                 /* 'toxe' keeps code consistent with BSIMSOI4.1 Manual*/
                                             / toxe);
                dDenomi_dVg = (ua + uc * Vbseff
                               + 2.0 * ub * (Vgsteff -T14) / toxe)
                              / toxe;
                dDenomi_dVd = 0.0;
                dDenomi_dVb = (Vgsteff -T14) * uc / toxe;
                if (selfheat)
                    dDenomi_dT = (Vgsteff -T14) / toxe
                                 * (dua_dT + Vbseff * duc_dT + dub_dT
                                    * (Vgsteff -T14) / toxe);
                else
                    dDenomi_dT = 0.0;
            }
            else if (model->B4SOImobMod == 3) /*  mobMod == 3  */
            {
                T0 = Vgsteff + Vth + Vth - T14;
                T2 = 1.0 + uc * Vbseff;
                T3 = T0 / toxe_mob;                                                                                     /* Bug fix #4 Jun 09 implementing Eeff correctly*/
                T4 = T3 * (ua + ub * T3);
                T5 = T4 * T2;
                dDenomi_dVg = (ua + 2.0 * ub * T3) * T2
                              / toxe_mob;                                                                                         /* Bug fix #4 Jun 09 implementing Eeff correctly*/
                dDenomi_dVd = dDenomi_dVg * 2.0 * dVth_dVd;
                dDenomi_dVb = dDenomi_dVg * 2.0 * dVth_dVb
                              + uc * T4 ;
                if (selfheat)
                    dDenomi_dT = dDenomi_dVg * 2.0 * dVth_dT
                                 + (dua_dT + dub_dT * T3) * T3 * T2
                                 + T4 * Vbseff * duc_dT;
                else
                    dDenomi_dT = 0.0;
            }
            else /*  mobMod == 4  */
            {
                /*universal mobility*/
                T0 = (Vgsteff + here->B4SOIvtfbphi1)* 1.0e-8 / toxe/6.0;
                T1 = exp(pParam->B4SOIeu * log(T0));                                                            /* MobMod=4 does not use Eeff */
                /* using 'toxe' keeps code consistent with BSIM4 formulation */
                dT1_dVg = T1 * pParam->B4SOIeu * 1.0e-8/ T0 / toxe/6.0;

                /*T2 = pParam->B4SOIua + pParam->B4SOIuc * Vbseff; */                           /* v4.2 bugfix # 35 */
                T2 = ua + uc * Vbseff;
                /*Coulombic*/
                /* pParam->B4SOIucs = pParam->B4SOIucs * pow(TempRatio, pParam->B4SOIucste);    Bug# 21 Jul09*/
                /* pParam->B4SOIud = pParam->B4SOIud * pow(TempRatio, pParam->B4SOIud1) ;               Bug# 21 Jul09 */
                ucs = pParam->B4SOIucs * pow(TempRatio, pParam->B4SOIucste);
                ud = pParam->B4SOIud * pow(TempRatio, pParam->B4SOIud1) ;
                VgsteffVth = here->B4SOIvgsteffvth;

                /*T10 = exp(pParam->B4SOIucs * log(0.5 + 0.5 * Vgsteff/VgsteffVth));*/
                /* T10 = exp(pParam->B4SOIucs * log(1.0 +  Vgsteff/VgsteffVth));                                Bug# 21 Jul09 */
                /* T11 =  pParam->B4SOIud/T10;                                                                                                  Bug# 21 Jul09 */
                T10 = exp(ucs * log(1.0 +  Vgsteff/VgsteffVth));                                        /* Bug Fix # 21 Jul09*/
                T11 = ud/T10;                                                                                                           /* Bug Fix # 21 Jul09*/
                /*dT11_dVg = - 0.5 * pParam->B4SOIucs * T11 /(0.5 + 0.5*Vgsteff/VgsteffVth)/VgsteffVth;*/
                /* dT11_dVg = (pParam->B4SOIucs - 1.0)*pParam->B4SOIud/(VgsteffVth* exp((pParam->B4SOIucs-1.0) * log(1.0 +  Vgsteff/VgsteffVth))); Bug# 21 Jul09*/

                dT11_dVg = (ucs - 1.0)*ud/(VgsteffVth* exp((ucs-1.0) * log(1.0 +  Vgsteff/VgsteffVth))); /* Bug Fix # 21 Jul09*/

                dDenomi_dVg = T2 * dT1_dVg + dT11_dVg;
                dDenomi_dVd = 0.0;
                /* dDenomi_dVb = T1 * pParam->B4SOIuc;            v4.2 bugfix # 35 */
                dDenomi_dVb = T1 * uc;
                T5 = T1 * T2 + T11;
                if (selfheat)
                    dDenomi_dT = dDenomi_dVg * pParam->B4SOIeu * dVth_dT
                                 + (dua_dT + dub_dT * T3) * T3 * T2
                                 + T4 * Vbseff * duc_dT;
                else
                    dDenomi_dT = 0.0;


            }
            if (T5 >= -0.8)
            {
                Denomi = 1.0 + T5;
            }
            else /* Added to avoid the discontinuity problem caused by ua and ub*/
            {
                T9 = 1.0 / (7.0 + 10.0 * T5);
                Denomi = (0.6 + T5) * T9;
                T9 *= T9;
                dDenomi_dVg *= T9;
                dDenomi_dVd *= T9;
                dDenomi_dVb *= T9;
                if (selfheat)  dDenomi_dT *= T9;
                else   dDenomi_dT = 0.0;
            }

            here->B4SOIueff = ueff = u0temp / Denomi;
            T9 = -ueff / Denomi;
            dueff_dVg = T9 * dDenomi_dVg;
            dueff_dVd = T9 * dDenomi_dVd;
            dueff_dVb = T9 * dDenomi_dVb;
            if (selfheat)  dueff_dT = T9 * dDenomi_dT + du0temp_dT / Denomi;
            else  dueff_dT = 0.0;

            /* Saturation Drain Voltage  Vdsat */
            WVCox = Weff * vsattemp * model->B4SOIcox;
            WVCoxRds = WVCox * Rds;

            /*                  dWVCoxRds_dT = WVCox * dRds_dT
                                + Weff * model->B4SOIcox * Rds * dvsattemp_dT; */

            Esat = 2.0 * vsattemp / ueff;
            EsatL = Esat * Leff;
            T0 = -EsatL /ueff;
            dEsatL_dVg = T0 * dueff_dVg;
            dEsatL_dVd = T0 * dueff_dVd;
            dEsatL_dVb = T0 * dueff_dVb;
            if (selfheat)
                dEsatL_dT = T0 * dueff_dT + EsatL / vsattemp * dvsattemp_dT;
            else
                dEsatL_dT = 0.0;

            /* Sqrt() */
            a1 = pParam->B4SOIa1;
            if (a1 == 0.0)
            {
                Lambda = pParam->B4SOIa2;
                dLambda_dVg = 0.0;
            }
            else if (a1 > 0.0)
                /* Added to avoid the discontinuity problem caused by a1 and a2 (Lambda) */
            {
                T0 = 1.0 - pParam->B4SOIa2;
                T1 = T0 - pParam->B4SOIa1 * Vgsteff - 0.0001;
                T2 = sqrt(T1 * T1 + 0.0004 * T0);
                Lambda = pParam->B4SOIa2 + T0 - 0.5 * (T1 + T2);
                dLambda_dVg = 0.5 * pParam->B4SOIa1 * (1.0 + T1 / T2);
            }
            else
            {
                T1 = pParam->B4SOIa2 + pParam->B4SOIa1 * Vgsteff - 0.0001;
                T2 = sqrt(T1 * T1 + 0.0004 * pParam->B4SOIa2);
                Lambda = 0.5 * (T1 + T2);
                dLambda_dVg = 0.5 * pParam->B4SOIa1 * (1.0 + T1 / T2);
            }

            here->B4SOIAbovVgst2Vtm = Abulk /Vgst2Vtm; /* v2.2.3 bug fix */

            if (Rds > 0)
            {
                tmp2 = dRds_dVg / Rds + dWeff_dVg / Weff;
                tmp3 = dRds_dVb / Rds + dWeff_dVb / Weff;
            }
            else
            {
                tmp2 = dWeff_dVg / Weff;
                tmp3 = dWeff_dVb / Weff;
            }
            if ((Rds == 0.0) && (Lambda == 1.0))
            {
                T0 = 1.0 / (Abulk * EsatL + Vgst2Vtm);
                tmp1 = 0.0;
                T1 = T0 * T0;
                T2 = Vgst2Vtm * T0;
                T3 = EsatL * Vgst2Vtm;
                Vdsat = T3 * T0;

                dT0_dVg = -(Abulk * dEsatL_dVg + EsatL * dAbulk_dVg + 1.0) * T1;
                dT0_dVd = -(Abulk * dEsatL_dVd) * T1;
                dT0_dVb = -(Abulk * dEsatL_dVb + EsatL * dAbulk_dVb) * T1;
                if (selfheat)
                    dT0_dT  = -(Abulk * dEsatL_dT + dVgst2Vtm_dT) * T1;
                else dT0_dT  = 0.0;

                dVdsat_dVg = T3 * dT0_dVg + T2 * dEsatL_dVg + EsatL * T0;
                dVdsat_dVd = T3 * dT0_dVd + T2 * dEsatL_dVd;
                dVdsat_dVb = T3 * dT0_dVb + T2 * dEsatL_dVb;
                if (selfheat)
                    dVdsat_dT  = T3 * dT0_dT  + T2 * dEsatL_dT
                                 + EsatL * T0 * dVgst2Vtm_dT;
                else dVdsat_dT  = 0.0;
            }
            else
            {
                tmp1 = dLambda_dVg / (Lambda * Lambda);
                T9 = Abulk * WVCoxRds;
                T8 = Abulk * T9;
                T7 = Vgst2Vtm * T9;
                T6 = Vgst2Vtm * WVCoxRds;
                T0 = 2.0 * Abulk * (T9 - 1.0 + 1.0 / Lambda);
                dT0_dVg = 2.0 * (T8 * tmp2 - Abulk * tmp1
                                 + (2.0 * T9 + 1.0 / Lambda - 1.0) * dAbulk_dVg);
                /*                    dT0_dVb = 2.0 * (T8 * tmp3  this is equivalent to one below, but simpler
                              + (2.0 * T9 + 1.0 / Lambda - 1.0) * dAbulk_dVg);  */
                dT0_dVb = 2.0 * (T8 * (2.0 / Abulk * dAbulk_dVb + tmp3)
                                 + (1.0 / Lambda - 1.0) * dAbulk_dVb);
                dT0_dVd = 0.0;

                if (selfheat)
                {

                    if (Rds!=0.0)
                        tmp4 = dRds_dT / Rds + dvsattemp_dT / vsattemp;
                    else
                        tmp4 = dvsattemp_dT / vsattemp;

                    dT0_dT = 2.0 * T8 * tmp4;
                }
                else tmp4 = dT0_dT = 0.0;

                T1 = Vgst2Vtm * (2.0 / Lambda - 1.0) + Abulk * EsatL
                     + 3.0 * T7;

                dT1_dVg = (2.0 / Lambda - 1.0) - 2.0 * Vgst2Vtm * tmp1
                          + Abulk * dEsatL_dVg + EsatL * dAbulk_dVg
                          + 3.0 * (T9 + T7 * tmp2 + T6 * dAbulk_dVg);
                dT1_dVb = Abulk * dEsatL_dVb + EsatL * dAbulk_dVb
                          + 3.0 * (T6 * dAbulk_dVb + T7 * tmp3);
                dT1_dVd = Abulk * dEsatL_dVd;


                if (selfheat)
                {
                    tmp4 += dVgst2Vtm_dT / Vgst2Vtm;
                    dT1_dT  = (2.0 / Lambda - 1.0) * dVgst2Vtm_dT
                              + Abulk * dEsatL_dT + 3.0 * T7 * tmp4;
                }
                else dT1_dT = 0.0;

                T2 = Vgst2Vtm * (EsatL + 2.0 * T6);
                dT2_dVg = EsatL + Vgst2Vtm * dEsatL_dVg
                          + T6 * (4.0 + 2.0 * Vgst2Vtm * tmp2);
                dT2_dVb = Vgst2Vtm * (dEsatL_dVb + 2.0 * T6 * tmp3);
                dT2_dVd = Vgst2Vtm * dEsatL_dVd;
                if (selfheat)
                    dT2_dT  = Vgst2Vtm * dEsatL_dT + EsatL * dVgst2Vtm_dT
                              + 2.0 * T6 * (dVgst2Vtm_dT + Vgst2Vtm * tmp4);
                else
                    dT2_dT  = 0.0;

                T3 = sqrt(T1 * T1 - 2.0 * T0 * T2);
                Vdsat = (T1 - T3) / T0;

                dVdsat_dVg = (dT1_dVg - (T1 * dT1_dVg - dT0_dVg * T2
                                         - T0 * dT2_dVg) / T3 - Vdsat * dT0_dVg) / T0;
                dVdsat_dVb = (dT1_dVb - (T1 * dT1_dVb - dT0_dVb * T2
                                         - T0 * dT2_dVb) / T3 - Vdsat * dT0_dVb) / T0;
                dVdsat_dVd = (dT1_dVd - (T1 * dT1_dVd - T0 * dT2_dVd) / T3) / T0;
                if (selfheat)
                    dVdsat_dT  = (dT1_dT - (T1 * dT1_dT - dT0_dT * T2
                                            - T0 * dT2_dT) / T3 - Vdsat * dT0_dT) / T0;
                else dVdsat_dT  = 0.0;
            }
            here->B4SOIvdsat = Vdsat;


            /* Effective Vds (Vdseff) Calculation */
            T1 = Vdsat - Vds - pParam->B4SOIdelta;
            dT1_dVg = dVdsat_dVg;
            dT1_dVd = dVdsat_dVd - 1.0;
            dT1_dVb = dVdsat_dVb;
            dT1_dT  = dVdsat_dT;

            T2 = sqrt(T1 * T1 + 4.0 * pParam->B4SOIdelta * Vdsat);
            T0 = T1 / T2;
            T3 = 2.0 * pParam->B4SOIdelta / T2;
            dT2_dVg = T0 * dT1_dVg + T3 * dVdsat_dVg;
            dT2_dVd = T0 * dT1_dVd + T3 * dVdsat_dVd;
            dT2_dVb = T0 * dT1_dVb + T3 * dVdsat_dVb;
            if (selfheat)
                dT2_dT  = T0 * dT1_dT  + T3 * dVdsat_dT;
            else dT2_dT  = 0.0;

            Vdseff = Vdsat - 0.5 * (T1 + T2);
            dVdseff_dVg = dVdsat_dVg - 0.5 * (dT1_dVg + dT2_dVg);
            dVdseff_dVd = dVdsat_dVd - 0.5 * (dT1_dVd + dT2_dVd);
            dVdseff_dVb = dVdsat_dVb - 0.5 * (dT1_dVb + dT2_dVb);
            if (selfheat)
                dVdseff_dT  = dVdsat_dT  - 0.5 * (dT1_dT  + dT2_dT);
            else dVdseff_dT  = 0.0;

            if (Vdseff > Vds)
                Vdseff = Vds; /* This code is added to fixed the problem
                                 caused by computer precision when
                                 Vds is very close to Vdseff. */
            diffVds = Vds - Vdseff;
            here->B4SOIVdseff = Vdseff; /* v2.2.3 bug fix */

            /* Calculate VAsat */
            tmp4 = 1.0 - 0.5 * Abulk * Vdsat / Vgst2Vtm;
            T9 = WVCoxRds * Vgsteff;
            T8 = T9 / Vgst2Vtm;
            T0 = EsatL + Vdsat + 2.0 * T9 * tmp4;

            T7 = 2.0 * WVCoxRds * tmp4;
            dT0_dVg = dEsatL_dVg + dVdsat_dVg + T7 * (1.0 + tmp2 * Vgsteff)
                      - T8 * (Abulk * dVdsat_dVg - Abulk * Vdsat / Vgst2Vtm
                              + Vdsat * dAbulk_dVg);

            dT0_dVb = dEsatL_dVb + dVdsat_dVb + T7 * tmp3 * Vgsteff
                      - T8 * (dAbulk_dVb * Vdsat + Abulk * dVdsat_dVb);
            dT0_dVd = dEsatL_dVd + dVdsat_dVd - T8 * Abulk * dVdsat_dVd;

            if (selfheat)
            {

                if (Rds!=0.0)
                    tmp4 = dRds_dT / Rds + dvsattemp_dT / vsattemp;
                else tmp4 = dvsattemp_dT / vsattemp;

                dT0_dT  = dEsatL_dT + dVdsat_dT + T7 * tmp4 * Vgsteff
                          - T8 * (Abulk * dVdsat_dT - Abulk * Vdsat * dVgst2Vtm_dT
                                  / Vgst2Vtm);
            }
            else
                dT0_dT = 0.0;

            T9 = WVCoxRds * Abulk;
            T1 = 2.0 / Lambda - 1.0 + T9;
            dT1_dVg = -2.0 * tmp1 +  WVCoxRds * (Abulk * tmp2 + dAbulk_dVg);
            dT1_dVb = dAbulk_dVb * WVCoxRds + T9 * tmp3;
            if (selfheat)
                dT1_dT  = T9 * tmp4;
            else
                dT1_dT  = 0.0;

            Vasat = T0 / T1;
            dVasat_dVg = (dT0_dVg - Vasat * dT1_dVg) / T1;
            dVasat_dVb = (dT0_dVb - Vasat * dT1_dVb) / T1;
            dVasat_dVd = dT0_dVd / T1;
            if (selfheat) dVasat_dT  = (dT0_dT  - Vasat * dT1_dT)  / T1;
            else dVasat_dT  = 0.0;

            /* Calculate VACLM */
            if ((pParam->B4SOIpclm > 0.0) && (diffVds > 1.0e-10))
            {
                T0 = 1.0 / (pParam->B4SOIpclm * Abulk * pParam->B4SOIlitl);
                dT0_dVb = -T0 / Abulk * dAbulk_dVb;
                dT0_dVg = -T0 / Abulk * dAbulk_dVg;

                T2 = Vgsteff / EsatL;
                T1 = Leff * (Abulk + T2);
                dT1_dVg = Leff * ((1.0 - T2 * dEsatL_dVg) / EsatL + dAbulk_dVg);
                dT1_dVb = Leff * (dAbulk_dVb - T2 * dEsatL_dVb / EsatL);
                dT1_dVd = -T2 * dEsatL_dVd / Esat;
                if (selfheat) dT1_dT  = -T2 * dEsatL_dT / Esat;
                else dT1_dT  = 0.0;

                T9 = T0 * T1;
                VACLM = T9 * diffVds;
                dVACLM_dVg = T0 * dT1_dVg * diffVds - T9 * dVdseff_dVg
                             + T1 * diffVds * dT0_dVg;
                dVACLM_dVb = (dT0_dVb * T1 + T0 * dT1_dVb) * diffVds
                             - T9 * dVdseff_dVb;
                dVACLM_dVd = T0 * dT1_dVd * diffVds + T9 * (1.0 - dVdseff_dVd);
                if (selfheat)
                    dVACLM_dT  = T0 * dT1_dT * diffVds - T9 * dVdseff_dT;
                else dVACLM_dT  = 0.0;

            }
            else
            {
                VACLM = MAX_EXPL;
                dVACLM_dVd = dVACLM_dVg = dVACLM_dVb = dVACLM_dT = 0.0;
            }


            /* Calculate VADIBL */
            /* if (pParam->B4SOIthetaRout > 0.0) */                                     /* v4.2 bugfix # 36 */
            if (thetaRout > 0.0)
            {
                T8 = Abulk * Vdsat;
                T0 = Vgst2Vtm * T8;
                T1 = Vgst2Vtm + T8;
                dT0_dVg = Vgst2Vtm * Abulk * dVdsat_dVg + T8
                          + Vgst2Vtm * Vdsat * dAbulk_dVg;
                dT1_dVg = 1.0 + Abulk * dVdsat_dVg + Vdsat * dAbulk_dVg;
                dT1_dVb = dAbulk_dVb * Vdsat + Abulk * dVdsat_dVb;
                dT0_dVb = Vgst2Vtm * dT1_dVb;
                dT1_dVd = Abulk * dVdsat_dVd;
                dT0_dVd = Vgst2Vtm * dT1_dVd;
                if (selfheat)
                {
                    dT0_dT  = dVgst2Vtm_dT * T8 + Abulk * Vgst2Vtm * dVdsat_dT;
                    dT1_dT  = dVgst2Vtm_dT + Abulk * dVdsat_dT;
                }
                else
                    dT0_dT = dT1_dT = 0.0;

                T9 = T1 * T1;
                /*T2 = pParam->B4SOIthetaRout; */                                                       /* v4.2 bugfix # 36 */
                T2 = thetaRout;
                VADIBL = (Vgst2Vtm - T0 / T1) / T2;
                dVADIBL_dVg = (1.0 - dT0_dVg / T1 + T0 * dT1_dVg / T9) / T2;
                dVADIBL_dVb = (-dT0_dVb / T1 + T0 * dT1_dVb / T9) / T2;
                dVADIBL_dVd = (-dT0_dVd / T1 + T0 * dT1_dVd / T9) / T2;
                if (selfheat)
                    dVADIBL_dT = (dVgst2Vtm_dT - dT0_dT/T1 + T0*dT1_dT/T9) / T2;
                else dVADIBL_dT = 0.0;

                T7 = pParam->B4SOIpdiblb * Vbseff;
                if (T7 >= -0.9)
                {
                    T3 = 1.0 / (1.0 + T7);
                    VADIBL *= T3;
                    dVADIBL_dVg *= T3;
                    dVADIBL_dVb = (dVADIBL_dVb - VADIBL * pParam->B4SOIpdiblb)
                                  * T3;
                    dVADIBL_dVd *= T3;
                    if (selfheat)  dVADIBL_dT  *= T3;
                    else  dVADIBL_dT  = 0.0;
                }
                else
                    /* Added to avoid the discontinuity problem caused by pdiblcb */
                {
                    T4 = 1.0 / (0.8 + T7);
                    T3 = (17.0 + 20.0 * T7) * T4;
                    dVADIBL_dVg *= T3;
                    dVADIBL_dVb = dVADIBL_dVb * T3
                                  - VADIBL * pParam->B4SOIpdiblb * T4 * T4;
                    dVADIBL_dVd *= T3;
                    if (selfheat)  dVADIBL_dT  *= T3;
                    else  dVADIBL_dT  = 0.0;
                    VADIBL *= T3;
                }
            }
            else
            {
                VADIBL = MAX_EXPL;
                dVADIBL_dVd = dVADIBL_dVg = dVADIBL_dVb = dVADIBL_dT = 0.0;
            }

            /* v4.0 DITS */
            T0 = pParam->B4SOIpditsd * Vds;
            if (T0 > EXPL_THRESHOLD)
            {
                T1 = MAX_EXPL;
                dT1_dVd = 0;
            }
            else
            {
                T1 = exp(T0);
                dT1_dVd = T1 * pParam->B4SOIpditsd;
            }
            if (pParam->B4SOIpdits > MIN_EXPL)
            {
                T2 = 1.0 + model->B4SOIpditsl * Leff;
                VADITS = (1.0 + T2 * T1) / pParam->B4SOIpdits;
                dVADITS_dVg = VADITS * dFP_dVg;
                dVADITS_dVd = FP * T2 * dT1_dVd / pParam->B4SOIpdits;
                VADITS *= FP;
                if (selfheat) dVADITS_dT = VADITS * dFP_dT / FP;
                else dVADITS_dT = 0.0;
            }
            else
            {
                VADITS = MAX_EXPL;
                dVADITS_dVg = dVADITS_dVd = dVADITS_dT = 0;
            }

            /* Calculate VA */

            T8 = pParam->B4SOIpvag / EsatL;
            T9 = T8 * Vgsteff;
            if (T9 > -0.9)
            {
                T0 = 1.0 + T9;
                dT0_dVg = T8 * (1.0 - Vgsteff * dEsatL_dVg / EsatL);
                dT0_dVb = -T9 * dEsatL_dVb / EsatL;
                dT0_dVd = -T9 * dEsatL_dVd / EsatL;
                if (selfheat)
                    dT0_dT  = -T9 * dEsatL_dT / EsatL;
                else
                    dT0_dT  = 0.0;
            }
            else /* Added to avoid the discontinuity problems caused by pvag */
            {
                T1 = 1.0 / (17.0 + 20.0 * T9);
                T0 = (0.8 + T9) * T1;
                T1 *= T1;
                dT0_dVg = T8 * (1.0 - Vgsteff * dEsatL_dVg / EsatL) * T1;

                T9 *= T1 / EsatL;
                dT0_dVb = -T9 * dEsatL_dVb;
                dT0_dVd = -T9 * dEsatL_dVd;
                if (selfheat)
                    dT0_dT  = -T9 * dEsatL_dT;
                else
                    dT0_dT  = 0.0;
            }

            tmp1 = VACLM * VACLM;
            tmp2 = VADIBL * VADIBL;
            tmp3 = VACLM + VADIBL;

            T1 = VACLM * VADIBL / tmp3;
            tmp3 *= tmp3;
            dT1_dVg = (tmp1 * dVADIBL_dVg + tmp2 * dVACLM_dVg) / tmp3;
            dT1_dVd = (tmp1 * dVADIBL_dVd + tmp2 * dVACLM_dVd) / tmp3;
            dT1_dVb = (tmp1 * dVADIBL_dVb + tmp2 * dVACLM_dVb) / tmp3;
            if (selfheat)
                dT1_dT  = (tmp1 * dVADIBL_dT  + tmp2 * dVACLM_dT ) / tmp3;
            else dT1_dT  = 0.0;

            /* v4.0 adding DITS */
            tmp1 = T1 * T1;
            tmp2 = VADITS * VADITS;
            tmp3 = T1 + VADITS;
            T2 = T1 * VADITS / tmp3;
            tmp3 *= tmp3;
            dT2_dVg = (tmp1 * dVADITS_dVg + tmp2 * dT1_dVg) / tmp3;
            dT2_dVd = (tmp1 * dVADITS_dVd + tmp2 * dT1_dVd) / tmp3;
            dT2_dVb = (                   tmp2 * dT1_dVb) / tmp3;
            if (selfheat)
                dT2_dT  = (tmp1 * dVADITS_dT  + tmp2 * dT1_dT ) / tmp3;
            else dT2_dT  = 0.0;

            /*
               Va = Vasat + T0 * T1;
               dVa_dVg = dVasat_dVg + T1 * dT0_dVg + T0 * dT1_dVg;
               dVa_dVd = dVasat_dVd + T1 * dT0_dVd + T0 * dT1_dVd;
               dVa_dVb = dVasat_dVb + T1 * dT0_dVb + T0 * dT1_dVb;
               if (selfheat)
               dVa_dT  = dVasat_dT  + T1 * dT0_dT  + T0 * dT1_dT;
               else dVa_dT  = 0.0;
               */
            /* v4.0 */
            Va = Vasat + T0 * T2;
            dVa_dVg = dVasat_dVg + T2 * dT0_dVg + T0 * dT2_dVg;
            dVa_dVd = dVasat_dVd + T2 * dT0_dVd + T0 * dT2_dVd;
            dVa_dVb = dVasat_dVb + T2 * dT0_dVb + T0 * dT2_dVb;
            if (selfheat)
                dVa_dT  = dVasat_dT  + T2 * dT0_dT  + T0 * dT2_dT;
            else dVa_dT  = 0.0;

            /* Calculate Ids */
            CoxWovL = model->B4SOIcox * Weff / Leff;
            beta = ueff * CoxWovL;
            dbeta_dVg = CoxWovL * dueff_dVg + beta * dWeff_dVg / Weff ;
            dbeta_dVd = CoxWovL * dueff_dVd;
            dbeta_dVb = CoxWovL * dueff_dVb + beta * dWeff_dVb / Weff ;
            if (selfheat)  dbeta_dT  = CoxWovL * dueff_dT;
            else  dbeta_dT  = 0.0;

            T0 = 1.0 - 0.5 * Abulk * Vdseff / Vgst2Vtm;
            dT0_dVg = -0.5 * (Abulk * dVdseff_dVg
                              - Abulk * Vdseff / Vgst2Vtm + Vdseff * dAbulk_dVg) / Vgst2Vtm;
            dT0_dVd = -0.5 * Abulk * dVdseff_dVd / Vgst2Vtm;
            dT0_dVb = -0.5 * (Abulk * dVdseff_dVb + dAbulk_dVb * Vdseff)
                      / Vgst2Vtm;
            if (selfheat)
                dT0_dT  = -0.5 * (Abulk * dVdseff_dT
                                  - Abulk * Vdseff / Vgst2Vtm * dVgst2Vtm_dT)
                          / Vgst2Vtm;
            else dT0_dT = 0.0;

            fgche1 = Vgsteff * T0;
            dfgche1_dVg = Vgsteff * dT0_dVg + T0;
            dfgche1_dVd = Vgsteff * dT0_dVd;
            dfgche1_dVb = Vgsteff * dT0_dVb;
            if (selfheat)  dfgche1_dT  = Vgsteff * dT0_dT;
            else  dfgche1_dT  = 0.0;

            T9 = Vdseff / EsatL;
            fgche2 = 1.0 + T9;
            dfgche2_dVg = (dVdseff_dVg - T9 * dEsatL_dVg) / EsatL;
            dfgche2_dVd = (dVdseff_dVd - T9 * dEsatL_dVd) / EsatL;
            dfgche2_dVb = (dVdseff_dVb - T9 * dEsatL_dVb) / EsatL;
            if (selfheat)  dfgche2_dT  = (dVdseff_dT  - T9 * dEsatL_dT)  / EsatL;
            else  dfgche2_dT  = 0.0;

            gche = beta * fgche1 / fgche2;
            dgche_dVg = (beta * dfgche1_dVg + fgche1 * dbeta_dVg
                         - gche * dfgche2_dVg) / fgche2;
            dgche_dVd = (beta * dfgche1_dVd + fgche1 * dbeta_dVd
                         - gche * dfgche2_dVd) / fgche2;
            dgche_dVb = (beta * dfgche1_dVb + fgche1 * dbeta_dVb
                         - gche * dfgche2_dVb) / fgche2;
            if (selfheat)
                dgche_dT  = (beta * dfgche1_dT  + fgche1 * dbeta_dT
                             - gche * dfgche2_dT)  / fgche2;
            else dgche_dT  = 0.0;

            T0 = 1.0 + gche * Rds;
            T9 = Vdseff / T0;
            Idl = gche * T9;
            IdlovVdseff = gche / T0;

            /*  Whoa, these formulas for the derivatives of Idl are convoluted, but I
                verified them to be correct  */

            dIdl_dVg = (gche * dVdseff_dVg + T9 * dgche_dVg) / T0
                       - Idl * gche / T0 * dRds_dVg ;
            dIdl_dVd = (gche * dVdseff_dVd + T9 * dgche_dVd) / T0;
            dIdl_dVb = (gche * dVdseff_dVb + T9 * dgche_dVb
                        - Idl * dRds_dVb * gche) / T0;
            if (selfheat)
                dIdl_dT  = (gche * dVdseff_dT + T9 * dgche_dT
                            - Idl * dRds_dT * gche) / T0;
            else dIdl_dT  = 0.0;

            T9 =  diffVds / Va;
            T0 =  1.0 + T9;
            here->B4SOIids = Ids = Idl * T0 / here->B4SOInseg;

            here->B4SOIidovVds = IdlovVdseff * T0 / here->B4SOInseg;
            /* v4.0 bug fix */
//            IdovVds = IdlovVdseff * T0 / here->B4SOInseg; /* v4.0 */

            Gm0 = T0 * dIdl_dVg - Idl * (dVdseff_dVg + T9 * dVa_dVg) / Va;
            Gds0 = T0 * dIdl_dVd + Idl * (1.0 - dVdseff_dVd
                                          - T9 * dVa_dVd) / Va;
            Gmb0 = T0 * dIdl_dVb - Idl * (dVdseff_dVb + T9 * dVa_dVb) / Va;
//            Gmc = 0.0;

            if (selfheat)
                GmT0 = T0 * dIdl_dT - Idl * (dVdseff_dT + T9 * dVa_dT) / Va;
            else GmT0 = 0.0;

            /* This includes all dependencies from Vgsteff, Vbseff */

            Gm = (Gm0 * dVgsteff_dVg+ Gmb0 * dVbseff_dVg) / here->B4SOInseg; /* v3.0 */
            Gmb = (Gm0 * dVgsteff_dVb + Gmb0 * dVbseff_dVb) / here->B4SOInseg;
            Gds = (Gm0 * dVgsteff_dVd+ Gmb0 * dVbseff_dVd + Gds0) / here->B4SOInseg; /* v3.0 */
            Gme = (Gm0 * dVgsteff_dVe + Gmb0 * dVbseff_dVe) / here->B4SOInseg; /* v3.0 */
            if (selfheat)
                GmT = (Gm0 * dVgsteff_dT + Gmb0 * dVbseff_dT + GmT0) / here->B4SOInseg; /* v3.0 */
            else GmT = 0.0;


            /* v3.1 */
            if (here->B4SOIsoiMod != 2) /* v3.2 */
            {
                /*  calculate GISL/GIDL current  */
                /*4.1*/
                if(model->B4SOImtrlMod == 0)
                    T0 = 3.0 * 3.9 / epsrox * toxe;
                else
                    T0 = model->B4SOIepsrsub * toxe / epsrox;


                if (model->B4SOIgidlMod==0)
                {
                    if (model->B4SOImtrlMod==0)
                        /* T1 = (- Vds - Vgs_eff - egisl) / T0; */                      /* Bug # 25 Jul09*/
                        T1 = (- Vds - Vgd_eff - egisl) / T0;
                    else
                        /* T1 = (- Vds - Vgs_eff - egisl+pParam->B4SOIvfbsd) / T0; */
                        T1 = (- Vds - Vgd_eff - egisl + pParam->B4SOIvfbsd) / T0;
                    /* GISL */
                    if ((agisl <= 0.0) ||
                            (bgisl <= 0.0) || (T1 <= 0.0) ||
                            /*(cgisl < 0.0) || (Vbd > 0.0) ) */                         /* v4.2 Bug # 24 Jul09*/
                            (cgisl < 0.0) || (Vbs > 0.0) )
                        Igisl = Ggisls = Ggislg = Ggislb = 0.0;

                    else
                    {
                        dT1_dVd = 1 / T0;
                        /* dT1_dVg = - dT1_dVd * dVgs_eff_dVg; *//* Bug fix # 25 Jul09 */
                        dT1_dVg = - dT1_dVd * dVgd_eff_dVg;
                        T2 = bgisl / T1;
                        if (T2 < EXPL_THRESHOLD)
                        {
                            Igisl = wdios * agisl * T1 * exp(-T2);
                            T3 = Igisl / T1 * (T2 + 1);
                            Ggisls = T3 * dT1_dVd;
                            /* Ggisls = T3 * dT1_dVg; */                                                /* Bug # 28 Jul09*/
                            Ggislg = T3 * dT1_dVg;
                        }
                        else
                        {
                            T3 = wdios * agisl * MIN_EXPL;
                            Igisl = T3 * T1 ;
                            Ggisls  = T3 * dT1_dVd;
                            Ggislg  = T3 * dT1_dVg;
                        }
                        if(cgisl >= MIN_EXPL)
                        {
                            T4 = Vbs * Vbs;
                            T5 = -Vbs * T4;
                            T6 = cgisl + T5;
                            T7 = T5 / T6;
                            T8 = 3.0 * cgisl * T4 / T6 / T6;
                            Ggisls = Ggisls * T7 + Igisl * T8;
                            Ggislg = Ggislg * T7;
                            Ggislb = -Igisl * T8;
                            Igisl *= T7;
                        }
                        else
                            Ggislb = 0.0;
                    }
                    here->B4SOIigisl = Igisl;
                    /* End of GISL */

                    if (model->B4SOImtrlMod==0)
                        T1 = (Vds - Vgs_eff - egidl) / T0;
                    else
                        T1 = (Vds - Vgs_eff - egidl + pParam->B4SOIvfbsd) / T0;

                    /* GIDL */
                    if ((agidl <= 0.0) ||
                            (bgidl <= 0.0) || (T1 <= 0.0) ||
                            (cgidl < 0.0) || (Vbd > 0.0) )
                        Igidl = Ggidld = Ggidlg = Ggidlb = 0.0;

                    else
                    {
                        dT1_dVd = 1 / T0;
                        dT1_dVg = - dT1_dVd * dVgs_eff_dVg;
                        T2 = bgidl / T1;
                        if (T2 < EXPL_THRESHOLD)
                        {
                            Igidl = wdiod * agidl * T1 * exp(-T2);
                            T3 = Igidl / T1 * (T2 + 1);
                            Ggidld = T3 * dT1_dVd;
                            Ggidlg = T3 * dT1_dVg;
                        }
                        else
                        {
                            T3 = wdiod * agidl * MIN_EXPL;
                            Igidl = T3 * T1 ;
                            Ggidld  = T3 * dT1_dVd;
                            Ggidlg  = T3 * dT1_dVg;
                        }
                        if(cgidl >= MIN_EXPL)
                        {
                            T4 = Vbd * Vbd;
                            T5 = -Vbd * T4;
                            T6 = cgidl + T5;
                            T7 = T5 / T6;
                            T8 = 3.0 * cgidl * T4 / T6 / T6;
                            Ggidld = Ggidld * T7 + Igidl * T8;
                            Ggidlg = Ggidlg * T7;
                            Ggidlb = -Igidl * T8;
                            Igidl *= T7;
                        }
                        else
                            Ggidlb = 0.0;
                    }
                    here->B4SOIigidl = Igidl;
                    /* End of GIDL*/
                }
                else
                {
                    if (model->B4SOImtrlMod==0)
                        /* T1 = (-Vds - rgisl*Vgs_eff - pParam->B4SOIegisl) / T0;*/
                        T1 = (-Vds - rgisl*Vgd_eff - egisl) / T0;       /* Bug # 26 Jul09*/
                    else
                        /* T1 = (-Vds - rgisl*Vgs_eff - pParam->B4SOIegisl+pParam->B4SOIvfbsd) / T0; */
                        T1 = (-Vds - rgisl*Vgd_eff - egisl + pParam->B4SOIvfbsd) / T0;          /* Bug # 26 Jul09*/

                    /* GISL */

                    if ((agisl <= 0.0) ||
                            (bgisl <= 0.0) || (T1 <= 0.0) ||
                            (cgisl < 0.0)  )
                        Igisl = Ggisls = Ggislg = Ggislb = 0.0;
                    else
                    {
                        dT1_dVd = 1 / T0;
                        /*  dT1_dVg = - rgisl*dT1_dVd * dVgs_eff_dVg;*//*Bug fix #26*/
                        dT1_dVg = - rgisl*dT1_dVd * dVgd_eff_dVg;
                        T2 = bgisl / T1;
                        if (T2 < EXPL_THRESHOLD)
                        {
                            Igisl = wdios * agisl * T1 * exp(-T2);
                            T3 = Igisl / T1 * (T2 + 1);
                            Ggisls = T3 * dT1_dVd;
                            Ggislg = T3 * dT1_dVg;
                        }
                        else
                        {
                            T3 = wdios * agisl * MIN_EXPL;
                            Igisl = T3 * T1 ;
                            Ggisls  = T3 * dT1_dVd;
                            Ggislg  = T3 * dT1_dVg;
                        }
                        T4 = Vbs - fgisl;
                        /*if (T4==0)
                            T5 =1;
                        else
                            T5 = kgisl/T4;
                        T6 = exp(T5);
                        if (T6<EXPL_THRESHOLD)
                        {Ggisls*=exp(T5);
                            Ggislg*=exp(T5);
                            Ggislb = -Igisl*exp(T5)*T5/T4;
                            Igisl*=exp(T5);
                        }
                        else
                            Ggislb=0.0; v4.3 bug fix */
                        if (T4==0)
                            T5 = EXPL_THRESHOLD;
                        else
                            T5 = kgisl/T4;
                        if (T5<EXPL_THRESHOLD)
                        {
                            T6 = exp(T5);
                            Ggislb = -Igisl*T6*T5/T4;
                        }
                        else
                        {
                            T6 = MAX_EXPL;
                            Ggislb=0.0;
                        }
                        Ggisls*=T6;
                        Ggislg*=T6;
                        Igisl*=T6;
                    }
                    here->B4SOIigisl = Igisl;
                    /* End of GISL */
                    if (model->B4SOImtrlMod==0)
                        /*T1 = (Vds - rgidl*Vgs_eff - pParam->B4SOIegidl) / T0; */              /* v4.2 bugfix #26 */
                        T1 = (Vds - rgidl*Vgs_eff - egidl) / T0;
                    else
                        /*T1 = (Vds - rgidl*Vgs_eff - pParam->B4SOIegidl+pParam->B4SOIvfbsd) / T0;*/ /* v4.2 bugfix #26 */
                        T1 = (Vds - rgidl * Vgs_eff - egidl + pParam->B4SOIvfbsd) / T0;
                    /* GIDL */
                    if ((agidl <= 0.0) ||
                            (bgidl <= 0.0) || (T1 <= 0.0) ||
                            (cgidl < 0.0)  )
                        Igidl = Ggidld = Ggidlg = Ggidlb = 0.0;
                    else
                    {
                        dT1_dVd = 1 / T0;
                        dT1_dVg = - rgidl*dT1_dVd * dVgs_eff_dVg;
                        T2 = bgidl / T1;
                        if (T2 < EXPL_THRESHOLD)
                        {
                            Igidl = wdiod * agidl * T1 * exp(-T2);
                            T3 = Igidl / T1 * (T2 + 1);
                            Ggidld = T3 * dT1_dVd;
                            Ggidlg = T3 * dT1_dVg;
                        }
                        else
                        {
                            T3 = wdiod * agidl * MIN_EXPL;
                            Igidl = T3 * T1 ;
                            Ggidld  = T3 * dT1_dVd;
                            Ggidlg  = T3 * dT1_dVg;
                        }
                        T4 = Vbd - fgidl;
                        /*if (T4==0)
                            T5 =1;
                        else
                            T5 = kgidl/T4;
                        T6 = exp(T5);
                        if (T6<EXPL_THRESHOLD)
                        {Ggidld*=exp(T5);
                            Ggidlg*=exp(T5);
                            Ggidlb = -Igidl*exp(T5)*T5/T4;
                            Igidl*=exp(T5);
                        }
                        else
                            Ggidlb=0.0; v4.3 bug fix */
                        if (T4==0)
                            T5 = EXPL_THRESHOLD;
                        else
                            T5 = kgidl/T4;
                        if (T5<EXPL_THRESHOLD)
                        {
                            T6 = exp(T5);
                            Ggidlb = -Igidl*T6*T5/T4;
                        }
                        else
                        {
                            T6 = MAX_EXPL;
                            Ggidlb=0.0;
                        }
                        Ggidld*=T6;
                        Ggidlg*=T6;
                        Igidl*=T6;
                    }
                    here->B4SOIigidl = Igidl;
                    /* End of GIDL */

                }





                /* calculate diode and BJT current */
                WsTsi = wdios * model->B4SOItsi;
                WdTsi = wdiod * model->B4SOItsi;
                /* NVtm1 = Vtm * pParam->B4SOIndiode;    v4.2 bugfix */
                NVtm1 = Vtm * ndiode;
                if (selfheat)
                    /*dNVtm1_dT = pParam->B4SOIndiode * dVtm_dT;        v4.2 bugfix */
                    dNVtm1_dT = ndiode * dVtm_dT;
                else
                    dNVtm1_dT = 0;
                T0 = vbs_jct / NVtm1; /* v4.0 */
                dT0_dVb = 1.0 / NVtm1;
                if (selfheat)
                    dT0_dT = -vbs_jct / NVtm1 / NVtm1 * dNVtm1_dT;
                else
                    dT0_dT = 0;
                DEXP(T0, ExpVbsNVtm, T1);
                dExpVbsNVtm_dVb = T1 * dT0_dVb;
                if (selfheat)
                    dExpVbsNVtm_dT = T1 * dT0_dT;
                else
                    dExpVbsNVtm_dT = 0;
                /* NVtm1 = Vtm * pParam->B4SOIndioded;   v4.2 bugfix */
                NVtm1 = Vtm * ndioded; /* v4.0 drain side */
                if (selfheat)
                    /*dNVtm1_dT = pParam->B4SOIndioded* dVtm_dT; v4.2 bugfix */
                    dNVtm1_dT = ndioded * dVtm_dT;
                else
                    dNVtm1_dT = 0;
                T0 = vbd_jct / NVtm1; /* v4.0 */
                dT0_dVb = 1.0 / NVtm1;
                dT0_dVd = -dT0_dVb;
                if (selfheat)
                    dT0_dT = -vbd_jct / NVtm1 / NVtm1 * dNVtm1_dT;
                else
                    dT0_dT = 0;
                DEXP(T0, ExpVbdNVtm, T1);
                dExpVbdNVtm_dVb = T1 * dT0_dVb;
                dExpVbdNVtm_dVd = -dExpVbdNVtm_dVb;
                if (selfheat)
                    dExpVbdNVtm_dT = T1 * dT0_dT;
                else
                    dExpVbdNVtm_dT = 0;

                /* Ibs1: diffusion current */
                if (jdifs == 0)
                {
                    Ibs1 = dIbs1_dVb = dIbs1_dT = 0;
                }
                else
                {
                    T0 = WsTsi * jdifs;
                    if (selfheat)
                        dT0_dT = WsTsi * djdifs_dT;
                    else
                        dT0_dT = 0;
                    Ibs1 = T0 * (ExpVbsNVtm - 1);
                    dIbs1_dVb = T0 * dExpVbsNVtm_dVb;
                    if (selfheat)
                        dIbs1_dT = T0 * dExpVbsNVtm_dT + (ExpVbsNVtm - 1) * dT0_dT;
                    else
                        dIbs1_dT = 0;
                }

                /* Ibd1: diffusion current */
                if (jdifd == 0)
                {
                    Ibd1 = dIbd1_dVb = dIbd1_dVd = dIbd1_dT = 0;
                }
                else
                {
                    T0 = WdTsi * jdifd;

                    if (selfheat)
                        dT0_dT = WdTsi * djdifd_dT;
                    else
                        dT0_dT = 0;
                    Ibd1 = T0 * (ExpVbdNVtm - 1);
                    dIbd1_dVb = T0 * dExpVbdNVtm_dVb;
                    dIbd1_dVd = -dIbd1_dVb;
                    if (selfheat)
                        dIbd1_dT = T0 * dExpVbdNVtm_dT + (ExpVbdNVtm -1)
                                   * dT0_dT;
                    else
                        dIbd1_dT = 0;
                }


                /* Ibs2:recombination/trap-assisted tunneling current */

                if (jrecs == 0)
                {
                    Ibs2 = dIbs2_dVb = dIbs2_dT = 0;
                }
                else
                {
                    /* forward bias */
                    NVtmf = 0.026 * nrecf0s   /* bugfix_snps for DC swapping*/
                            * (1 + pParam->B4SOIntrecf * (TempRatio - 1));
                    NVtmr = 0.026 * nrecr0s   /* bugfix_snps for DC swapping*/
                            * (1 + pParam->B4SOIntrecr * (TempRatio - 1));
                    if (selfheat)
                    {
                        dNVtmf_dT = nrecf0s * 0.026   /* bugfix_snps for DC swapping*/
                                    * pParam->B4SOIntrecf * dTempRatio_dT;
                        dNVtmr_dT = nrecr0s * 0.026  /* bugfix_snps for DC swapping*/
                                    * pParam->B4SOIntrecr * dTempRatio_dT;
                    }
                    else
                        dNVtmf_dT = dNVtmr_dT = 0;

                    T0 = vbs_jct / NVtmf; /* v4.0 */
                    DEXP(T0,T10,T2);
                    T4 = 1 / NVtmf;
                    dT10_dVb = T4 * T2;
                    if (selfheat)
                        dT10_dT  = - T4 * T2 * vbs_jct / NVtmf * dNVtmf_dT ;
                    else   dT10_dT  = 0.0;

                    /* reverse bias */
                    if ((vrec0s- vbs_jct) < 1e-3)    /* bugfix_snps for DC swapping*/
                    {

                        /* v2.2.3 bug fix */
                        T1 = 1e3;
                        T0 = -vbs_jct / NVtmr * vrec0s * T1; /* bugfix_snps for DC swapping*/
                        T11 = -exp(T0);

                        dT11_dVb = dT11_dT = 0;
                    }
                    else
                    {
                        T1 = 1 / (vrec0s - vbs_jct); /* bugfix_snps for DC swapping*/
                        T0 = -vbs_jct / NVtmr * vrec0s * T1; /* bugfix_snps for DC swapping*/
                        dT0_dVb = -vrec0s / NVtmr *  /* bugfix_snps for DC swapping*/
                                  (T1 + vbs_jct * T1 * T1) ;
                        if (selfheat)
                            dT0_dT = -T0 / NVtmr * dNVtmr_dT;
                        else   dT0_dT = 0;

                        DEXP(T0, T11, T2);
                        T11 = -T11;
                        dT11_dVb = -T2 * dT0_dVb;
                        if (selfheat)
                            dT11_dT = -T2 * dT0_dT;
                        else   dT11_dT = 0;
                    }
                    T3 = WsTsi * jrecs;
                    Ibs2 = T3 * (T10 + T11);
                    dIbs2_dVb = T3 * (dT10_dVb + dT11_dVb);
                    if (selfheat)
                        dIbs2_dT = T3 * (dT10_dT + dT11_dT)
                                   + WsTsi * (T10 + T11) * djrecs_dT;
                    else   dIbs2_dT = 0;

                }

                if (jrecd == 0)
                {
                    Ibd2 = dIbd2_dVb = dIbd2_dVd = dIbd2_dT = 0;
                }
                else
                {
                    NVtmf = 0.026 * nrecf0d    /* bugfix_snps for DC swapping*/
                            * (1 + pParam->B4SOIntrecf * (TempRatio - 1));
                    NVtmr = 0.026 * nrecr0d        /* bugfix_snps for DC swapping*/
                            * (1 + pParam->B4SOIntrecr * (TempRatio - 1));
                    if (selfheat)
                    {
                        dNVtmf_dT = nrecf0d * 0.026   /*bugfix_snps for DC swapping*/
                                    * pParam->B4SOIntrecf * dTempRatio_dT;
                        dNVtmr_dT = nrecr0d * 0.026
                                    * pParam->B4SOIntrecr * dTempRatio_dT;  /* bugfix_snps for DC swapping*/
                    }
                    else
                        dNVtmf_dT = dNVtmr_dT = 0;

                    T0 = vbd_jct / NVtmf;
                    DEXP(T0,T10,T2);
                    T4 = 1 / NVtmf;
                    dT10_dVb = T4 * T2;
                    if (selfheat)
                        dT10_dT  = - T4 * T2 * vbd_jct / NVtmf * dNVtmf_dT ;
                    else   dT10_dT  = 0.0;

                    if ((vrec0d - vbd_jct) < 1e-3)     /* bugfix_snps for DC swapping*/
                    {

                        /* v2.2.3 bug fix */
                        T1 = 1e3;
                        T0 = -vbd_jct / NVtmr * vrec0d * T1;  /* bugfix_snps for DC swapping*/
                        T11 = -exp(T0);

                        dT11_dVb = dT11_dT = 0;
                    }
                    else
                    {
                        T1 = 1 / (vrec0d - vbd_jct);    /* bugfix_snps for DC swapping*/
                        T0 = -vbd_jct / NVtmr * vrec0d * T1;  /* bugfix_snps for DC swapping*/
                        dT0_dVb = -vrec0d / NVtmr /* bugfix_snps for DC swapping*/
                                  * (T1 + vbd_jct * T1 * T1) ;
                        if (selfheat)
                            dT0_dT = -T0 / NVtmr * dNVtmr_dT;
                        else
                            dT0_dT = 0;
                        DEXP(T0, T11, T2);
                        T11 = - T11;
                        dT11_dVb = -T2 * dT0_dVb;
                        if (selfheat)
                            dT11_dT = -T2 * dT0_dT;
                        else
                            dT11_dT = 0;
                    }
                    T3 = WdTsi * jrecd;
                    Ibd2 = T3 * (T10 + T11);
                    dIbd2_dVb = T3 * (dT10_dVb + dT11_dVb);
                    dIbd2_dVd = -dIbd2_dVb;
                    if (selfheat)
                        dIbd2_dT = T3 * (dT10_dT + dT11_dT)
                                   + WdTsi * (T10 + T11) * djrecd_dT;
                    else
                        dIbd2_dT = 0;
                }

                /* Ibs3/Ibd3:  recombination current in neutral body */
                WTsi = pParam->B4SOIweff / here->B4SOInseg * model->B4SOItsi;
                if (jbjts == 0.0 && jbjtd == 0.0)
                {
                    Ibs3 = dIbs3_dVb = dIbs3_dVd = dIbs3_dT = 0.0;
                    Ibd3 = dIbd3_dVb = dIbd3_dVd = dIbd3_dT = 0.0;
                    Ibsdif = dIbsdif_dVb = dIbsdif_dT = 0;
                    /*Ibddif = dIbddif_dVb = dIbddif_dVd = dIbddif_dT = 0; v4.2 */
                    Ibddif = dIbddif_dVb = dIbddif_dT = 0;
                    here->B4SOIic = Ic = Gcd = Gcb = GcT = 0.0;
                }
                else
                {
                    Ien = WTsi * jbjts * pParam->B4SOIlratio;
                    if (selfheat)
                        dIen_dT = WTsi * djbjts_dT * pParam->B4SOIlratio;
                    else
                        dIen_dT = 0;

                    /* high level injection of source side */
                    if ((Ehlis = Ahlis * (ExpVbsNVtm - 1)) < 1e-5)
                    {
                        Ehlis = dEhlis_dVb = dEhlis_dT = 0;
                        EhlisFactor = 1;
                        dEhlisFactor_dVb = dEhlisFactor_dT = 0;
                    }
                    else
                    {
                        dEhlis_dVb = Ahlis * dExpVbsNVtm_dVb;
                        if (selfheat)
                            dEhlis_dT = Ahlis * dExpVbsNVtm_dT + (ExpVbsNVtm - 1) * dAhlis_dT;
                        else
                            dEhlis_dT = 0;
                        EhlisFactor = 1.0 / sqrt(1 + Ehlis);
                        T0 = -0.5 * EhlisFactor / (1 + Ehlis);
                        dEhlisFactor_dVb = T0 * dEhlis_dVb;
                        if (selfheat)
                            dEhlisFactor_dT = T0 * dEhlis_dT;
                        else
                            dEhlisFactor_dT = 0;
                    }

                    /* high level injection of drain side */
                    if ((Ehlid = Ahlid * (ExpVbdNVtm - 1)) < 1e-5)
                    {
                        Ehlid = dEhlid_dVb = dEhlid_dVd = dEhlid_dT = 0;
                        EhlidFactor = 1;
                        dEhlidFactor_dVb = dEhlidFactor_dVd = dEhlidFactor_dT = 0;
                    }
                    else
                    {
                        dEhlid_dVb = Ahlid * dExpVbdNVtm_dVb;
                        dEhlid_dVd = -dEhlid_dVb;
                        if (selfheat)
                            dEhlid_dT = Ahlid * dExpVbdNVtm_dT + (ExpVbdNVtm - 1) * dAhlid_dT;
                        else
                            dEhlid_dT = 0;
                        EhlidFactor = 1.0 / sqrt(1 + Ehlid);
                        T0 = -0.5 * EhlidFactor / (1 + Ehlid);
                        dEhlidFactor_dVb = T0 * dEhlid_dVb;
                        dEhlidFactor_dVd = -dEhlidFactor_dVb;
                        if (selfheat)
                            dEhlidFactor_dT = T0 * dEhlid_dT;
                        else
                            dEhlidFactor_dT = 0;
                    }


                    /* v3.1.1 bug fix for Ibjt(L) discontinuity */
                    T0 = 1 - pParam->B4SOIarfabjt;
                    T1 = T0 * Ien;
                    if (selfheat)
                        dT1_dT = T0 * dIen_dT;
                    else
                        dT1_dT = 0;

                    Ibs3 = T1 * (ExpVbsNVtm - 1) * EhlisFactor;
                    dIbs3_dVb = T1 * (dExpVbsNVtm_dVb * EhlisFactor
                                      + (ExpVbsNVtm - 1) * dEhlisFactor_dVb);
                    dIbs3_dVd = 0;
                    if (selfheat)
                        dIbs3_dT = dT1_dT * (ExpVbsNVtm - 1) * EhlisFactor
                                   + T1 * (dExpVbsNVtm_dT * EhlisFactor
                                           + (ExpVbsNVtm - 1) * dEhlisFactor_dT);
                    else
                        dIbs3_dT = 0.0;

                    Ien = WTsi * jbjtd * pParam->B4SOIlratio;
                    if (selfheat)
                        dIen_dT = WTsi * djbjtd_dT * pParam->B4SOIlratio;
                    else
                        dIen_dT = 0;

                    T1 = T0 * Ien;
                    if (selfheat)
                        dT1_dT = T0 * dIen_dT;
                    else
                        dT1_dT = 0;

                    Ibd3 = T1 * (ExpVbdNVtm - 1) * EhlidFactor;
                    dIbd3_dVb = T1 * (dExpVbdNVtm_dVb * EhlidFactor
                                      + (ExpVbdNVtm - 1) * dEhlidFactor_dVb);
                    dIbd3_dVd = -dIbd3_dVb;
                    if (selfheat)
                        dIbd3_dT = dT1_dT * (ExpVbdNVtm - 1) * EhlidFactor
                                   + T1 * (dExpVbdNVtm_dT * EhlidFactor
                                           + (ExpVbdNVtm - 1) * dEhlidFactor_dT);
                    else
                        dIbd3_dT = 0.0;


                    /* effective diffusion current for capacitance calcu. */
                    Iendif = WTsi * jbjts * pParam->B4SOIlratiodif;
                    if (selfheat)
                        dIendif_dT = WTsi * djbjts_dT * pParam->B4SOIlratiodif;
                    else
                        dIendif_dT = 0;

                    Ibsdif = Iendif * (ExpVbsNVtm - 1) * EhlisFactor;
                    dIbsdif_dVb = Iendif * (dExpVbsNVtm_dVb * EhlisFactor
                                            + (ExpVbsNVtm - 1) * dEhlisFactor_dVb);
                    if (selfheat)
                        dIbsdif_dT = dIendif_dT * (ExpVbsNVtm - 1) * EhlisFactor
                                     + Iendif * (dExpVbsNVtm_dT * EhlisFactor
                                                 + (ExpVbsNVtm - 1) * dEhlisFactor_dT);
                    else
                        dIbsdif_dT = 0;

                    Iendif = WTsi * jbjtd * pParam->B4SOIlratiodif;
                    if (selfheat)
                        dIendif_dT = WTsi * djbjtd_dT * pParam->B4SOIlratiodif;
                    else
                        dIendif_dT = 0;

                    Ibddif = Iendif * (ExpVbdNVtm - 1) * EhlidFactor;
                    dIbddif_dVb = Iendif * (dExpVbdNVtm_dVb * EhlidFactor
                                            + (ExpVbdNVtm - 1) * dEhlidFactor_dVb);
                    /*dIbddif_dVd = -dIbddif_dVb; v4.2 */
                    if (selfheat)
                        dIbddif_dT = dIendif_dT * (ExpVbdNVtm - 1) * EhlidFactor
                                     + Iendif * (dExpVbdNVtm_dT * EhlidFactor
                                                 + (ExpVbdNVtm - 1) * dEhlidFactor_dT);
                    else
                        dIbddif_dT = 0;

                    /* Ic: Bjt collector current */
                    if ((here->B4SOIbjtoff == 1) || (Vds == 0.0))
                    {
                        here->B4SOIic = Ic = Gcd = Gcb = GcT = 0.0;
                        dIc_dVb = dIc_dVd = 0.0;  /*bugfix_snps for setting zero */
                    }
                    else
                    {
                        /* second order effects */
                        /* T0 = 1 + (Vbs + Vbd) / pParam->B4SOIvearly; v4.3 bugfix */
                        T0 = 1 + (vbs_jct + vbd_jct) / pParam->B4SOIvearly;
                        dT0_dVb = 2.0 / pParam->B4SOIvearly;
                        dT0_dVd = -1.0 / pParam->B4SOIvearly;
                        T1 = Ehlis + Ehlid;
                        dT1_dVb = dEhlis_dVb + dEhlid_dVb;
                        dT1_dVd = dEhlid_dVd;
                        if (selfheat)
                            dT1_dT = dEhlis_dT + dEhlid_dT;
                        else
                            dT1_dT = 0;

                        T3 = sqrt(T0 * T0 + 4 * T1);
                        dT3_dVb = 0.5 / T3 * (2 * T0 * dT0_dVb + 4 * dT1_dVb);
                        dT3_dVd = 0.5 / T3 * (2 * T0 * dT0_dVd + 4 * dT1_dVd);
                        if (selfheat)
                            dT3_dT = 2 * dT1_dT / T3;
                        else
                            dT3_dT = 0;

                        T2 = (T0 + T3) / 2.0;
                        dT2_dVb = (dT0_dVb + dT3_dVb) / 2.0;
                        dT2_dVd = (dT0_dVd + dT3_dVd) / 2.0;
                        if (selfheat)
                            dT2_dT = dT3_dT /2.0;
                        else
                            dT2_dT = 0;

                        if (T2 < .1)
                        {
                            E2ndFactor = 10.0;
                            dE2ndFactor_dVb = dE2ndFactor_dVd = dE2ndFactor_dT = 0;
                        }

                        else
                        {
                            E2ndFactor = 1.0 / T2;
                            dE2ndFactor_dVb = -E2ndFactor / T2 * dT2_dVb;
                            dE2ndFactor_dVd = -E2ndFactor / T2 * dT2_dVd;
                            if (selfheat)
                                dE2ndFactor_dT = -E2ndFactor / T2 * dT2_dT;
                            else
                                dE2ndFactor_dT = 0;
                        }

                        T0 = pParam->B4SOIarfabjt * Ien;        /* here Ien refers to the drain side to simplify the code */
                        if (selfheat)
                            dT0_dT = pParam->B4SOIarfabjt * dIen_dT;
                        else
                            dT0_dT = 0;
                        here->B4SOIic = Ic
                                        = T0 * (ExpVbsNVtm - ExpVbdNVtm) * E2ndFactor;
                        Gcb = dIc_dVb
                              = T0 * ((dExpVbsNVtm_dVb - dExpVbdNVtm_dVb) * E2ndFactor
                                      + (ExpVbsNVtm - ExpVbdNVtm) * dE2ndFactor_dVb);
                        Gcd = dIc_dVd
                              = T0 * (-dExpVbdNVtm_dVd * E2ndFactor
                                      + (ExpVbsNVtm - ExpVbdNVtm) * dE2ndFactor_dVd);
                        if (selfheat)
                            GcT = T0 * (dExpVbsNVtm_dT - dExpVbdNVtm_dT) * E2ndFactor
                                  + dT0_dT * (ExpVbsNVtm - ExpVbdNVtm) * E2ndFactor
                                  + T0 * (ExpVbsNVtm - ExpVbdNVtm) * dE2ndFactor_dT;
                        else
                            GcT = 0;
                    }
                }

                /* Ibs4/Ibd4 : tunneling */
                if (jtuns == 0 && jtund == 0)
                {
                    Ibs4 = Ibd4 = dIbs4_dVb = dIbs4_dT = dIbd4_dVb = dIbd4_dVd = dIbd4_dT = 0;
                }
                else
                {
                    NVtm2 = 0.026 * ntuns;     /* bugfix_snps for junction DC swapping*/
                    if ((vtun0s - vbs_jct) < 1e-3)  /* bugfix_snps for junction DC swapping*/
                    {
                        /* v2.2.3 bug fix */
                        T1=1e3;
                        T0 = -vbs_jct / NVtm2 * vtun0s * T1; /* bugfix_snps for junction DC swapping*/
                        T1 = exp(T0);
                        T3 = WsTsi * jtuns;
                        Ibs4 = T3 * (1- T1);

                        /*dIbs4_dVb = dIbs4_dT = 0; */
                        dIbs4_dVb = 0.0;
                        if (selfheat)
                            dIbs4_dT = (1 - T1) * WsTsi * djtuns_dT;
                        else
                            dIbs4_dT = 0;

                    }
                    else
                    {
                        T1 = 1 / (vtun0s - vbs_jct);   /*bugfix for junction DC swapping*/
                        T0 = -vbs_jct / NVtm2 * vtun0s * T1;  /*bugfix for junction DC swapping*/
                        dT0_dVb = -vtun0s / NVtm2 * (T1 + vbs_jct * T1 * T1) ; /*bugfix for junction DC swapping*/

                        DEXP(T0, T1, T2);
                        T3 = WsTsi * jtuns;
                        Ibs4 =  T3 * (1- T1);
                        dIbs4_dVb = -T3 * T2 * dT0_dVb;
                        if (selfheat)
                            dIbs4_dT = (1 - T1) * WsTsi * djtuns_dT;
                        else   dIbs4_dT = 0;
                    }

                    NVtm2 = 0.026 * ntund;   /* bugfix_snps for junction DC swapping*/
                    if ((vtun0d - vbd_jct) < 1e-3)   /* bugfix_snps for junction DC swapping*/
                    {

                        /* v2.2.3 bug fix */
                        T1=1e3;
                        T0 = -vbd_jct / NVtm2 * vtun0d * T1; /* bugfix_snps for junction DC swapping*/
                        T1 = exp(T0);
                        T3 = WdTsi * jtund;
                        Ibd4 = T3 * (1- T1);

                        /*dIbd4_dVb = dIbd4_dT = 0;*/
                        dIbd4_dVb = 0;
                        dIbd4_dVd = 0;
                        if (selfheat)
                            /* dIbs4_dT = (1 - T1) * WsTsi * djtuns_dT; */ /* Bug fix #8 Jun 09 'typo's corrected for Drain side */
                            /* else   dIbs4_dT = 0;     */
                            dIbd4_dT = (1 - T1) * WdTsi * djtund_dT;            /* Fix */
                        else   dIbd4_dT = 0;
                    }
                    else
                    {
                        T1 = 1 / (vtun0d - vbd_jct); /* bugfix_snps for junction DC swapping*/
                        T0 = -vbd_jct / NVtm2 * vtun0d * T1; /* bugfix_snps for junction DC swapping*/
                        dT0_dVb = -vtun0d / NVtm2 * (T1 + vbd_jct * T1 * T1) ; /* bugfix_snps for junction DC swapping*/

                        DEXP(T0, T1, T2);
                        T3 = WdTsi * jtund;
                        Ibd4 =  T3 * (1- T1);
                        dIbd4_dVb = -T3 * T2 * dT0_dVb;

                        dIbd4_dVd = -dIbd4_dVb;

                        if (selfheat)
                            dIbd4_dT = (1 - T1) * WdTsi * djtund_dT;
                        else   dIbd4_dT = 0;
                    }
                }

                here->B4SOIitun = - Ibd3 - Ibd4;
                Ibs = Ibs1 + Ibs2 + Ibs3 + Ibs4;
                Ibd = Ibd1 + Ibd2 + Ibd3 + Ibd4;

                Gjsb = dIbs1_dVb + dIbs2_dVb + dIbs3_dVb + dIbs4_dVb;
                Gjsd = dIbs3_dVd;
                if (selfheat)  GjsT = dIbs1_dT + dIbs2_dT + dIbs3_dT + dIbs4_dT;
                else   GjsT = 0.0;

                Gjdb = dIbd1_dVb + dIbd2_dVb + dIbd3_dVb + dIbd4_dVb;
                Gjdd = dIbd1_dVd + dIbd2_dVd + dIbd3_dVd + dIbd4_dVd;
                if (selfheat)  GjdT = dIbd1_dT  + dIbd2_dT + dIbd3_dT + dIbd4_dT;
                else   GjdT = 0.0;
            }
            else /* v3.1 soiMod=2: ideal FD */
            {
                here->B4SOIigidl= Igidl
                                  = Ggidld = Ggidlg = Ggidlb = 0.0;
                here->B4SOIigisl= Igisl                                         /* Bug fix #9 Jun 09 Code added to set Igisl components to zero */
                                  = Ggisls = Ggislg = Ggislb = 0.0;                   /* This is an appx solution */
                /* Final code will comply with BSIM MG in future releases */
                here->B4SOIitun = 0;
                Ibs = 0;
                Ibd = 0;
                here->B4SOIic = Ic = Gcd = Gcb = GcT = 0.0;

                Gjsb = 0;
                Gjsd = 0;
                GjsT = 0;

                Gjdb = 0;
                Gjdd = 0;
                GjdT = 0;
            }
            if (here->B4SOImode > 0)
            {
                here->B4SOIibs = Ibs;
                here->B4SOIibd = Ibd;
            }
            else
            {
                here->B4SOIibd = Ibs;
                here->B4SOIibs = Ibd;
            }

            /* v3.0: gate-tunneling */
            if ((model->B4SOIigbMod != 0) || (model->B4SOIigcMod != 0))
            {
                Vgb = Vgs_eff - Vbs;
                dVgb_dVg = dVgs_eff_dVg;
                dVgb_dVb = -1;

                /* Calculate Vox first */
                Vfb = model->B4SOItype * here->B4SOIvth0  /* v4.0 */
                      - phi - pParam->B4SOIk1eff * sqrtPhi;

                T3 = Vfb - Vgs_eff + Vbs - DELTA_3;
                dT3_dVg = -dVgs_eff_dVg;
                dT3_dVd = 0;
                dT3_dVb = 1;

                if (Vfb <= 0.0)
                {
                    T0 = sqrt(T3 * T3 - 4.0 * DELTA_3 * Vfb);
                    dT0_dVg = 1.0/(2.0 * T0) * 2.0*T3 * dT3_dVg;
                    dT0_dVb = 0.5*(1.0/T0) * 2.0*T3 * dT3_dVb;
                }
                else
                {
                    T0 = sqrt(T3 * T3 + 4.0 * DELTA_3 * Vfb);
                    dT0_dVg = 1.0/(2.0 * T0) * 2.0*T3 * dT3_dVg;
                    dT0_dVb = 0.5*(1.0/T0) * 2.0*T3 * dT3_dVb;
                }

                Vfbeff = Vfb - 0.5 * (T3 + T0);
                dVfbeff_dVg = -0.5 * (dT3_dVg + dT0_dVg);
                dVfbeff_dVb = -0.5 * (dT3_dVb + dT0_dVb);

                Voxacc = Vfb - Vfbeff;
                dVoxacc_dVg = -dVfbeff_dVg;
                dVoxacc_dVd = 0.0;
                dVoxacc_dVb = -dVfbeff_dVb;
                if (Voxacc < 0.0)
                    Voxacc = dVoxacc_dVg = dVoxacc_dVb = 0.0;


                T0 = Vgs_eff - Vgsteff - Vfbeff - Vbseff;
                dT0_dVg = dVgs_eff_dVg - dVgsteff_dVg - dVfbeff_dVg - dVbseff_dVg; /* v3.0 */
                dT0_dVd = -dVgsteff_dVd - dVbseff_dVd; /* v3.0 */
                dT0_dVb = -dVgsteff_dVb - dVfbeff_dVb - dVbseff_dVb;
                dT0_dVe = -dVgsteff_dVe - dVbseff_dVe;

                if (selfheat)
                    dT0_dT = -dVgsteff_dT - dVbseff_dT; /* v3.0 */

                if (pParam->B4SOIk1ox == 0.0) /* v4.0 */
                {
                    Voxdepinv = dVoxdepinv_dVg = dVoxdepinv_dVd = dVoxdepinv_dVb
                                                 = dVoxdepinv_dT = 0.0;
                }
                else
                {
                    if (T0 < 0.0)
                    {
                        T1 = T0/pParam->B4SOIk1ox;
                        dT1_dVg = dT0_dVg/pParam->B4SOIk1ox;
                        dT1_dVd = dT0_dVd/pParam->B4SOIk1ox;
                        dT1_dVb = dT0_dVb/pParam->B4SOIk1ox;
                        dT1_dVe = dT0_dVe/pParam->B4SOIk1ox; /* v3.0 */
                        if (selfheat) dT1_dT = dT0_dT/pParam->B4SOIk1ox;
                    }
                    else
                    {
                        T1 = pParam->B4SOIk1ox/2*(-1 + sqrt(1 +
                                                            4*T0/pParam->B4SOIk1ox/pParam->B4SOIk1ox));
                        T2 = pParam->B4SOIk1ox/2 *
                             0.5/sqrt(1 + 4*T0/pParam->B4SOIk1ox/pParam->B4SOIk1ox) *
                             4/pParam->B4SOIk1ox/pParam->B4SOIk1ox;
                        dT1_dVg = T2 * dT0_dVg;
                        dT1_dVd = T2 * dT0_dVd;
                        dT1_dVb = T2 * dT0_dVb;
                        dT1_dVe = T2 * dT0_dVe; /* v3.0 */
                        if (selfheat)
                            dT1_dT = T2 * dT0_dT;
                    }

                    Voxdepinv = Vgs_eff - (T1*T1 + Vbs) - Vfb;
                    dVoxdepinv_dVg = dVgs_eff_dVg - (2.0*T1*dT1_dVg);
                    dVoxdepinv_dVd = -(2.0*T1*dT1_dVd);
                    dVoxdepinv_dVb = -(2.0*T1*dT1_dVb + 1);
                    dVoxdepinv_dVe = -(2.0*T1*dT1_dVe); /* v3.0 */
                    if (selfheat)
                        dVoxdepinv_dT = -(2.0*T1*dT1_dT);
                }
            }


            /* gate-channel tunneling component */
            if (model->B4SOIigcMod)
            {
                T0 = Vtm * pParam->B4SOInigc;
                VxNVt = (Vgs_eff - model->B4SOItype * here->B4SOIvth0)
                        / T0; /* Vth instead of Vth0 may be used */
                if (VxNVt > EXPL_THRESHOLD)
                {
                    Vaux = Vgs_eff - model->B4SOItype * here->B4SOIvth0;
                    dVaux_dVg = dVgs_eff_dVg;
                    dVaux_dVd = 0.0;
                    dVaux_dVb = 0.0;
                }
                else if (VxNVt < -EXPL_THRESHOLD)
                {
                    Vaux = T0 * log(1.0 + MIN_EXPL);
                    dVaux_dVg = dVaux_dVd = dVaux_dVb = 0.0;
                }
                else
                {
                    ExpVxNVt = exp(VxNVt);
                    Vaux = T0 * log(1.0 + ExpVxNVt);
                    dVaux_dVg = ExpVxNVt / (1.0 + ExpVxNVt);
                    dVaux_dVd = -dVaux_dVg * 0.0;
                    dVaux_dVb = -dVaux_dVg * 0.0;
                    dVaux_dVg *= dVgs_eff_dVg;
                }

                T2 = Vgs_eff * Vaux;
                dT2_dVg = dVgs_eff_dVg * Vaux + Vgs_eff * dVaux_dVg;
                dT2_dVd = Vgs_eff * dVaux_dVd;
                dT2_dVb = Vgs_eff * dVaux_dVb;

                T11 = pParam->B4SOIAechvb;
                T12 = pParam->B4SOIBechvb;
                T3 = pParam->B4SOIaigc * pParam->B4SOIcigc
                     - pParam->B4SOIbigc;
                T4 = pParam->B4SOIbigc * pParam->B4SOIcigc;
                T5 = T12 * (pParam->B4SOIaigc + T3 * Voxdepinv
                            - T4 * Voxdepinv * Voxdepinv);

                if (T5 > EXPL_THRESHOLD)
                {
                    T6 = MAX_EXPL;
                    dT6_dVg = dT6_dVd = dT6_dVb = 0.0;
                }
                else if (T5 < -EXPL_THRESHOLD)
                {
                    T6 = MIN_EXPL;
                    dT6_dVg = dT6_dVd = dT6_dVb = 0.0;
                }
                else
                {
                    T6 = exp(T5);
                    dT6_dVg = T6 * T12 * (T3 - 2.0 * T4 * Voxdepinv);
                    dT6_dVd = dT6_dVg * dVoxdepinv_dVd;
                    dT6_dVb = dT6_dVg * dVoxdepinv_dVb;
                    dT6_dVg *= dVoxdepinv_dVg;
                }

                Igc = T11 * T2 * T6;
                dIgc_dVg = T11 * (T2 * dT6_dVg + T6 * dT2_dVg);
                dIgc_dVd = T11 * (T2 * dT6_dVd + T6 * dT2_dVd);
                dIgc_dVb = T11 * (T2 * dT6_dVb + T6 * dT2_dVb);

                T7 = -pParam->B4SOIpigcd * Vds;
                T8 = T7 * T7 + 2.0e-4;
                dT8_dVd = -2.0 * pParam->B4SOIpigcd * T7;
                if (T7 > EXPL_THRESHOLD)
                {
                    T9 = MAX_EXPL;
                    dT9_dVd = 0.0;
                }
                else if (T7 < -EXPL_THRESHOLD)
                {
                    T9 = MIN_EXPL;
                    dT9_dVd = 0.0;
                }
                else
                {
                    T9 = exp(T7);
                    dT9_dVd = -T9 * pParam->B4SOIpigcd;
                }

                T0 = T8 * T8;
                T1 = T9 - 1.0 + 1.0e-4;
                T10 = (T1 - T7) / T8;
                dT10_dVd = ((pParam->B4SOIpigcd + dT9_dVd) * T8
                            - (T1 - T7) * dT8_dVd) / T0;
                Igcs = Igc * T10;
                dIgcs_dVg = dIgc_dVg * T10;
                dIgcs_dVd = dIgc_dVd * T10 + Igc * dT10_dVd;
                dIgcs_dVb = dIgc_dVb * T10;

                T1 = T9 - 1.0 - 1.0e-4;
                T10 = (T7 * T9 - T1) / T8;
                dT10_dVd = (-pParam->B4SOIpigcd * T9 + (T7 - 1.0)
                            * dT9_dVd - T10 * dT8_dVd) / T8;
                Igcd = Igc * T10;
                dIgcd_dVg = dIgc_dVg * T10;
                dIgcd_dVd = dIgc_dVd * T10 + Igc * dT10_dVd;
                dIgcd_dVb = dIgc_dVb * T10;

                here->B4SOIIgcs = Igcs;
                here->B4SOIgIgcsg = dIgcs_dVg;
                here->B4SOIgIgcsd = dIgcs_dVd;
                here->B4SOIgIgcsb =  dIgcs_dVb * dVbseff_dVb;
                here->B4SOIIgcd = Igcd;
                here->B4SOIgIgcdg = dIgcd_dVg;
                here->B4SOIgIgcdd = dIgcd_dVd;
                here->B4SOIgIgcdb = dIgcd_dVb * dVbseff_dVb;


                T0 = vgs - pParam->B4SOIvfbsd;
                vgs_eff = sqrt(T0 * T0 + 1.0e-4);
                dvgs_eff_dvg = T0 / vgs_eff;

                T2 = vgs * vgs_eff;
                dT2_dVg = vgs * dvgs_eff_dvg + vgs_eff;
                /*  T11 = pParam->B4SOIAechvbEdge; */
                T13 = pParam->B4SOIAechvbEdges;
                T14 = pParam->B4SOIAechvbEdged;
                T12 = pParam->B4SOIBechvbEdge;
                T3 = pParam->B4SOIaigsd * pParam->B4SOIcigsd
                     - pParam->B4SOIbigsd;
                T4 = pParam->B4SOIbigsd * pParam->B4SOIcigsd;
                T5 = T12 * (pParam->B4SOIaigsd + T3 * vgs_eff
                            - T4 * vgs_eff * vgs_eff);
                if (T5 > EXPL_THRESHOLD)
                {
                    T6 = MAX_EXPL;
                    dT6_dVg = 0.0;
                }
                else if (T5 < -EXPL_THRESHOLD)
                {
                    T6 = MIN_EXPL;
                    dT6_dVg = 0.0;
                }
                else
                {
                    T6 = exp(T5);
                    dT6_dVg = T6 * T12 * (T3 - 2.0 * T4 * vgs_eff)
                              * dvgs_eff_dvg;
                }
                /*  Igs = T11 * T2 * T6; */
                Igs = T13 * T2 * T6;
                dIgs_dVg = T13 * (T2 * dT6_dVg + T6 * dT2_dVg);
                dIgs_dVs = -dIgs_dVg;

                T0 = vgd - pParam->B4SOIvfbsd;
                vgd_eff = sqrt(T0 * T0 + 1.0e-4);
                dvgd_eff_dvg = T0 / vgd_eff;

                T2 = vgd * vgd_eff;
                dT2_dVg = vgd * dvgd_eff_dvg + vgd_eff;
                T5 = T12 * (pParam->B4SOIaigsd + T3 * vgd_eff
                            - T4 * vgd_eff * vgd_eff);
                if (T5 > EXPL_THRESHOLD)
                {
                    T6 = MAX_EXPL;
                    dT6_dVg = 0.0;
                }
                else if (T5 < -EXPL_THRESHOLD)
                {
                    T6 = MIN_EXPL;
                    dT6_dVg = 0.0;
                }
                else
                {
                    T6 = exp(T5);
                    dT6_dVg = T6 * T12 * (T3 - 2.0 * T4 * vgd_eff)
                              * dvgd_eff_dvg;
                }
                /*  Igd = T11 * T2 * T6; */
                Igd = T14 * T2 * T6;
                dIgd_dVg = T14 * (T2 * dT6_dVg + T6 * dT2_dVg);
                dIgd_dVd = -dIgd_dVg;

                here->B4SOIIgs = Igs;
                here->B4SOIgIgsg = dIgs_dVg;
                here->B4SOIgIgss = dIgs_dVs;
                here->B4SOIIgd = Igd;
                here->B4SOIgIgdg = dIgd_dVg;
                here->B4SOIgIgdd = dIgd_dVd;
            }
            else
            {
                here->B4SOIIgcs = here->B4SOIgIgcsg = here->B4SOIgIgcsd
                                                      = here->B4SOIgIgcsb = 0.0;
                here->B4SOIIgcd = here->B4SOIgIgcdg = here->B4SOIgIgcdd
                                                      = here->B4SOIgIgcdb = 0.0;
                here->B4SOIIgs = here->B4SOIgIgsg = here->B4SOIgIgss = 0.0;
                here->B4SOIIgd = here->B4SOIgIgdg = here->B4SOIgIgdd = 0.0;
            }

            here->B4SOIgIgcss = -(here->B4SOIgIgcsg + here->B4SOIgIgcsd
                                  + here->B4SOIgIgcsb);
            here->B4SOIgIgcds = -(here->B4SOIgIgcdg + here->B4SOIgIgcdd
                                  + here->B4SOIgIgcdb);


            /* gate-body tunneling component */
            if ((model->B4SOIigbMod!= 0) && (here->B4SOIsoiMod != 2))  /* v3.2 */
                /* v3.1: the Igb calculation is skipped for the ideal FD mode */
            {
                OxideRatio = pParam->B4SOIoxideRatio;

                Vox = Voxdepinv;
                /* Voxeff is Vox limited below Voxh */
                T0 = model->B4SOIvoxh;
                T1 = T0 - Vox - model->B4SOIdeltavox;
                T3 = sqrt(T1 * T1 + 4*model->B4SOIdeltavox * T0);
                Voxeff = T0 - 0.5 * (T1 + T3);
                dVoxeff_dVox = 0.5 * (1.0 + T1 / T3);

                Vox = Voxeff;
                dVox_dVg = dVoxdepinv_dVg * dVoxeff_dVox;
                dVox_dVd = dVoxdepinv_dVd * dVoxeff_dVox;
                dVox_dVb = dVoxdepinv_dVb * dVoxeff_dVox;
                dVox_dVe = dVoxdepinv_dVe * dVoxeff_dVox; /* v3.0 */
                if (selfheat)                                                                                   /* v4.2 Bug # 23 Jul09 */
                    dVox_dT = dVoxdepinv_dT * dVoxeff_dVox;


                T0 = (Vox - model->B4SOIebg)/model->B4SOIvevb;
                if (selfheat)
                    dT0_dT = dVox_dT /model->B4SOIvevb;

                DEXP(T0, T1, T2); /* T1=exp(T0), T2=dT1_dT0 */
                if (selfheat)
                    dT1_dT = T2 * dT0_dT;

                Vaux = model->B4SOIvevb * log(1 + T1);
                dVaux_dVg = T2 / (1 + T1) * dVox_dVg;
                dVaux_dVd = T2 / (1 + T1) * dVox_dVd;
                dVaux_dVb = T2 / (1 + T1) * dVox_dVb;
                dVaux_dVe = T2 / (1 + T1) * dVox_dVe; /* v3.0 */
                if (selfheat)
                    dVaux_dT = T2 / (1 + T1) * dVox_dT;

                if (model->B4SOIvgb1 != 0)
                {
                    T0 = 1 - Vox / model->B4SOIvgb1;
                    dT0_dVox = -1.0/model->B4SOIvgb1;
                    if (selfheat)
                        dT0_dT = -dVox_dT / model->B4SOIvgb1;
                }
                else
                {
                    T0 = 1;
                    dT0_dVox = dT0_dT = 0.0;
                }

                if (T0 < 0.01)
                {
                    T0 = 0.01;
                    dT0_dVox = dT0_dT = 0.0;
                }

                /* v2.2.3 bug fix */
                T1 = (Leff * Weff / here->B4SOInseg + here->B4SOIagbcpd/here->B4SOInf) * 3.7622e-7 * OxideRatio;

                T2 = -3.1051e10 * model->B4SOItoxqm;
                T3 = pParam->B4SOIalphaGB1;
                T4 = pParam->B4SOIbetaGB1;

                T6 = T2*(T3 - T4 * Vox) / T0;
                if (selfheat) dT6_dT = -T2 * T4 * dVox_dT / T0 - T6/T0 * dT0_dT;

                DEXP(T6, T5, T7); /* T5=exp(T6), T7=dT5_dT6 */
                dT5_dVg = -T7 * dVox_dVg * T2 / T0 * (T4 + (T3 - T4 * Vox) / T0 * dT0_dVox);
                dT5_dVd = -T7 * dVox_dVd * T2 / T0 * (T4 + (T3 - T4 * Vox) / T0 * dT0_dVox);
                dT5_dVb = -T7 * dVox_dVb * T2 / T0 * (T4 + (T3 - T4 * Vox) / T0 * dT0_dVox);
                dT5_dVe = -T7 * dVox_dVe * T2 / T0 * (T4 + (T3 - T4 * Vox) / T0 * dT0_dVox); /* v3.0 */
                if (selfheat)
                    dT5_dT = T7 * dT6_dT;

                Igb1 = T1 * Vgb * Vaux * T5;
                dIgb1_dVg = T1 * (Vgb*Vaux*dT5_dVg + dVgb_dVg*Vaux*T5 +
                                  Vgb*T5*dVaux_dVg);
                dIgb1_dVd = T1 * (Vgb*Vaux*dT5_dVd + Vgb*T5*dVaux_dVd);
                dIgb1_dVb = T1 * (Vgb*Vaux*dT5_dVb + dVgb_dVb*Vaux*T5 +
                                  Vgb*T5*dVaux_dVb);
                dIgb1_dVe = T1 * (Vgb*Vaux*dT5_dVe + Vgb*T5*dVaux_dVe); /* v3.0 */
                if (selfheat)
                    dIgb1_dT = T1 * Vgb * (Vaux*dT5_dT + T5*dVaux_dT);
                else dIgb1_dT = 0.0;


                Vox = Voxacc;
                /* Voxeff is Vox limited below Voxh */
                T0 = model->B4SOIvoxh;
                T1 = T0 - Vox - model->B4SOIdeltavox;
                T3 = sqrt(T1 * T1 + 4*model->B4SOIdeltavox * T0);
                Voxeff = T0 - 0.5 * (T1 + T3);
                dVoxeff_dVox = 0.5 * (1.0 + T1 / T3);

                Vox = Voxeff;
                dVox_dVg = dVoxacc_dVg * dVoxeff_dVox;
                dVox_dVd = dVoxacc_dVd * dVoxeff_dVox;
                dVox_dVb = dVoxacc_dVb * dVoxeff_dVox;
                dVox_dT = 0;

                T0 = (-Vgb+(Vfb))/model->B4SOIvecb;
                if (selfheat)
                    dT0_dT = 0;

                DEXP(T0, T1, T2); /* T1=exp(T0), T2=dT1_dT0 */
                if (selfheat)
                    dT1_dT = 0;

                Vaux = model->B4SOIvecb* log(1 + T1);
                dVaux_dVg = -T2 / (1 + T1);
                dVaux_dVd = 0;
                dVaux_dVb = -dVaux_dVg;
                if (selfheat)
                    dVaux_dT = 0;

                if (model->B4SOIvgb2 != 0)
                {
                    T0 = 1 - Vox / model->B4SOIvgb2;
                    dT0_dVox = -1.0/model->B4SOIvgb2;
                    if (selfheat) dT0_dT = -dVox_dT / model->B4SOIvgb2;
                }
                else
                {
                    T0 = 1;
                    dT0_dVox = dT0_dT =0.0;
                }

                if (T0 < 0.01)
                {
                    T0 = 0.01;
                    dT0_dVox = dT0_dT =0.0;
                }

                /* v2.2.3 bug fix */
                T1 = (Leff * Weff / here->B4SOInseg + here->B4SOIagbcpd/here->B4SOInf) * 4.9758e-7  * OxideRatio;

                T2 = -2.357e10 * model->B4SOItoxqm;
                T3 = pParam->B4SOIalphaGB2;
                T4 = pParam->B4SOIbetaGB2;

                T6 = T2*(T3 - T4 * Vox) / T0;
                if (selfheat) dT6_dT = -T2 * T4 * dVox_dT / T0 - T6/T0 * dT0_dT;

                DEXP(T6, T5, T7); /* T5=exp(T6), T7=dT5_dT6 */
                dT5_dVg = -T7 * dVox_dVg * T2 / T0 * (T4 + (T3 - T4 * Vox) / T0 * dT0_dVox);
                dT5_dVd = -T7 * dVox_dVd * T2 / T0 * (T4 + (T3 - T4 * Vox) / T0 * dT0_dVox);
                dT5_dVb = -T7 * dVox_dVb * T2 / T0 * (T4 + (T3 - T4 * Vox) / T0 * dT0_dVox);
                if (selfheat)
                    dT5_dT = T7 * dT6_dT;

                Igb2 = T1 * Vgb * Vaux * T5;
                dIgb2_dVg = T1 * (Vgb*Vaux*dT5_dVg + dVgb_dVg*Vaux*T5 +
                                  Vgb*T5*dVaux_dVg);
                dIgb2_dVd = T1 * (Vgb*Vaux*dT5_dVd + Vgb*T5*dVaux_dVd);
                dIgb2_dVb = T1 * (Vgb*Vaux*dT5_dVb + dVgb_dVb*Vaux*T5 +
                                  Vgb*T5*dVaux_dVb);
                if (selfheat)
                    dIgb2_dT = T1 * Vgb * (Vaux*dT5_dT + T5*dVaux_dT);
                else dIgb2_dT = 0.0;


                /* Igb1 dominates in inversion region, while Igb2 dominates in accumulation */
                /* v2.2.3 bug fix for residue at low Vgb */
                if (Vgb >= 0)
                {
                    Igb = Igb1;
                    dIgb_dVg = dIgb1_dVg;
                    dIgb_dVd = dIgb1_dVd;
                    dIgb_dVb = dIgb1_dVb;
                    dIgb_dVe = dIgb1_dVe; /* v3.0 */
                    dIgb_dT = dIgb1_dT;
                }
                else
                {
                    Igb = Igb2;
                    dIgb_dVg = dIgb2_dVg;
                    dIgb_dVd = dIgb2_dVd;
                    dIgb_dVb = dIgb2_dVb;
                    dIgb_dVe = 0; /* v3.0 */
                    dIgb_dT = dIgb2_dT;
                }
                Vfb2 = Vfb + 1.12;  /* Bug fix #18 Jul09*/
            }
            else
            {
                Igb = 0.0;
                dIgb_dVg = 0.0;
                dIgb_dVd = 0.0;
                dIgb_dVb = 0.0;
                dIgb_dVe = 0.0; /* v3.0 */
                dIgb_dT = 0.0;
            }
            here->B4SOIig = Igb;
            here->B4SOIgigg = dIgb_dVg;
            here->B4SOIgigd = dIgb_dVd;
            here->B4SOIgigb = dIgb_dVb;
            here->B4SOIgige = dIgb_dVe; /* v3.0 */
            here->B4SOIgigs = -(dIgb_dVg + dIgb_dVd + dIgb_dVb + dIgb_dVe); /* v3.0 */
            here->B4SOIgigT = dIgb_dT;

            /* v4.1 */
            /* gate tunneling component in the AGBCP2 region */
            /* Vfb2 = Vfb + 1.12;   Bug fix #18 Jul09 Code moved to 4370 where Vfb definition is valid*/

            if ((model->B4SOIigbMod!= 0) && (here->B4SOIsoiMod != 2) &&
                    (here->B4SOIbodyMod != 0) && (here->B4SOIagbcp2 > 0) &&
                    (vgp < Vfb2))
                /* v4.1: the Igb2_agbcp2 calculation is skipped for the ideal FD mode or if there is no "p" node */
            {
                /* Vfb, Vfb2 are taken as constants in derivative calculation for simplicity */
                T0 = vgp - Vfb2;

                T1 = sqrt(T0 * T0 + 1.0e-4);
                vgp_eff = 0.5 * (-T0 + T1 - 1.0e-2);
                dvgp_eff_dvg = 0.5 * (-1.0 + T0 / T1);
//                dvgp_eff_dvp = -dvgp_eff_dvg;

                /* T11=A*  T12=B* */
                T11 = (model->B4SOItype == NMOS) ? 3.42537e-7 : 4.97232e-7;
                T12 = (model->B4SOItype == NMOS) ? 1.16645e12 : 7.45669e11;

                T2 = vgp * vgp_eff;
                dT2_dVg = vgp * dvgp_eff_dvg + vgp_eff;
//                dT2_dVp = vgp * dvgp_eff_dvp - vgp_eff;

                T3 = pParam->B4SOIaigbcp2 * pParam->B4SOIcigbcp2
                     - pParam->B4SOIbigbcp2;
                T4 = pParam->B4SOIbigbcp2 * pParam->B4SOIcigbcp2;
                T5 = (-T12) * model->B4SOItoxqm * (pParam->B4SOIaigbcp2
                                                   + T3 * vgp_eff - T4 * vgp_eff * vgp_eff);
                if (T5 > EXPL_THRESHOLD)
                {
                    T6 = MAX_EXPL;
                    dT6_dVg = 0.0;
//                    dT6_dVp = 0.0;
                }
                else if (T5 < -EXPL_THRESHOLD)
                {
                    T6 = MIN_EXPL;
                    dT6_dVg = 0.0;
//                    dT6_dVp = 0.0;
                }
                else
                {
                    T6 = exp(T5);
                    T7 = T6 * (-T12) * model->B4SOItoxqm *
                         (T3 - 2.0 * T4 * vgp_eff);
                    dT6_dVg = T7 * dvgp_eff_dvg;
//                    dT6_dVp = T7 * dvgp_eff_dvg;
                }
                T11 = T11 * here->B4SOIagbcp2 * pParam->B4SOIoxideRatio/here->B4SOInf;
                Ig_agbcp2 = T11 * T2 * T6;
                dIg_agbcp2_dVg = T11 * (T2 * dT6_dVg + T6 * dT2_dVg);
                dIg_agbcp2_dVp = -dIg_agbcp2_dVg;
            }
            else
            {
                Ig_agbcp2 = 0.0;
                dIg_agbcp2_dVg = 0.0;
                dIg_agbcp2_dVp = 0.0;
            }
            here->B4SOIigp = Ig_agbcp2;
            here->B4SOIgigpg = dIg_agbcp2_dVg;
            here->B4SOIgigpp = dIg_agbcp2_dVp;

            /* end of gate-body tunneling */
            /* end of v3.0 gate-tunneling  */

            /* v3.1 */
            if (here->B4SOIsoiMod != 2) /* v3.2 */
            {
                Idsmosfet = 0.0;
                Ratio = dRatio_dVg = dRatio_dVd = dRatio_dVb = dRatio_dVe = dRatio_dT = 0.0;
                if (model->B4SOIiiiMod == 0 )
                {
                    /* calculate substrate current Iii */
                    if (pParam->B4SOIalpha0 <= 0.0)
                    {
                        Giig = Giib = Giid = GiiT = 0.0;
                        Giie = 0; /* v3.0 */
                        here->B4SOIiii = Iii = 0.0;
                    }
                    else
                    {
                        Vdsatii0 = pParam->B4SOIvdsatii0 * (1 + model->B4SOItii * (TempRatio-1.0))
                                   - pParam->B4SOIlii / Leff;
                        if (selfheat)
                            dVdsatii0_dT = pParam->B4SOIvdsatii0 * model->B4SOItii * dTempRatio_dT;
                        else
                            dVdsatii0_dT = 0;

                        /* Calculate VgsStep */
                        T0 = pParam->B4SOIesatii * Leff; /* v3.0 bug fix: T0 is dimentionless (i.e., scaled by 1V) */
                        T1 = pParam->B4SOIsii0 * T0 / (1.0 + T0);

                        T0 = 1 / (1 + pParam->B4SOIsii1 * Vgsteff);
                        if (selfheat)
                            dT0_dT = - pParam->B4SOIsii1 * T0 * T0 *dVgsteff_dT;
                        else
                            dT0_dT = 0;
                        T3 = T0 + pParam->B4SOIsii2;
                        T4 = Vgst * pParam->B4SOIsii1 * T0 * T0;
                        T2 = Vgst * T3;
                        dT2_dVg = T3 * (dVgst_dVg - dVth_dVb * dVbseff_dVg) - T4 * dVgsteff_dVg; /* v3.0 */
                        dT2_dVb = T3 * dVgst_dVb * dVbseff_dVb - T4 * dVgsteff_dVb;
                        dT2_dVe = T3 * dVgst_dVb * dVbseff_dVe - T4 * dVgsteff_dVe; /* v3.0 */
                        dT2_dVd = T3 * (dVgst_dVd - dVth_dVb * dVbseff_dVd) - T4 * dVgsteff_dVd; /* v3.0 */
                        if (selfheat)
                            dT2_dT = -(dVth_dT + dVth_dVb * dVbseff_dT) * T3 + Vgst * dT0_dT; /* v3.0 */
                        else dT2_dT = 0;


                        T3 = 1 / (1 + pParam->B4SOIsiid * Vds);
                        dT3_dVd = - pParam->B4SOIsiid * T3 * T3;

                        VgsStep = T1 * T2 * T3;
                        if (selfheat)
                            dVgsStep_dT = T1 * T3 * dT2_dT;
                        else dVgsStep_dT = 0;
                        Vdsatii = Vdsatii0 + VgsStep;
                        Vdiff = Vds - Vdsatii;
                        dVdiff_dVg = - T1 * T3 * dT2_dVg;
                        dVdiff_dVb = - T1 * T3 * dT2_dVb;
                        dVdiff_dVe = - T1 * T3 * dT2_dVe; /* v3.0 */
                        dVdiff_dVd = 1.0 - T1 * (T3 * dT2_dVd + T2 * dT3_dVd);
                        if (selfheat)
                            dVdiff_dT  = -(dVdsatii0_dT + dVgsStep_dT);
                        else dVdiff_dT = 0;

                        T0 = pParam->B4SOIbeta2 + pParam->B4SOIbeta1 * Vdiff
                             + pParam->B4SOIbeta0 * Vdiff * Vdiff;
                        if (T0 < 1e-5)
                        {
                            T0 = 1e-5;
                            dT0_dVg = dT0_dVd = dT0_dVb = dT0_dT = 0.0;
                            dT0_dVe = 0; /* v3.0 */
                        }
                        else
                        {
                            T1 = pParam->B4SOIbeta1 + 2 * pParam->B4SOIbeta0 * Vdiff;
                            dT0_dVg = T1 * dVdiff_dVg;
                            dT0_dVb = T1 * dVdiff_dVb;
                            dT0_dVd = T1 * dVdiff_dVd;
                            dT0_dVe = T1 * dVdiff_dVe; /* v3.0 */
                            if (selfheat)
                                dT0_dT = T1 * dVdiff_dT;
                            else
                                dT0_dT = 0;
                        }

                        if ((T0 < Vdiff / EXPL_THRESHOLD) && (Vdiff > 0.0))
                        {
                            Ratio = pParam->B4SOIalpha0 * MAX_EXPL;
                            dRatio_dVg = dRatio_dVb = dRatio_dVd = dRatio_dT = 0.0;
                            dRatio_dVe = 0; /* v3.0 */
                        }
                        else if ((T0 < -Vdiff / EXPL_THRESHOLD) && (Vdiff < 0.0))
                        {
                            Ratio = pParam->B4SOIalpha0 * MIN_EXPL;
                            dRatio_dVg = dRatio_dVb = dRatio_dVd = dRatio_dT = 0.0;
                            dRatio_dVe = 0; /* v3.0 */
                        }
                        else
                        {
                            Ratio = pParam->B4SOIalpha0 * exp(Vdiff / T0);
                            T1 = Ratio / T0 / T0;
                            dRatio_dVg = T1 * (T0 * dVdiff_dVg - Vdiff * dT0_dVg);
                            dRatio_dVb = T1 * (T0 * dVdiff_dVb - Vdiff * dT0_dVb);
                            dRatio_dVd = T1 * (T0 * dVdiff_dVd - Vdiff * dT0_dVd);
                            /* v3.0 */
                            dRatio_dVe = T1 * (T0 * dVdiff_dVe - Vdiff * dT0_dVe);

                            if (selfheat)
                                dRatio_dT = T1 * (T0 * dVdiff_dT - Vdiff * dT0_dT);
                            else
                                dRatio_dT = 0;
                        }

                        /* Avoid too high ratio */
                        if (Ratio > 10.0)
                        {
                            Ratio = 10.0;
                            dRatio_dVg = dRatio_dVb = dRatio_dVd = dRatio_dT = 0.0;
                            dRatio_dVe = 0; /* v3.0 */
                        }

                        T0 = Ids + pParam->B4SOIfbjtii * Ic;
                        here->B4SOIiii = Iii = Ratio * T0;
                        Giig = Ratio * Gm + T0 * dRatio_dVg;
                        Giib = Ratio * (Gmb + pParam->B4SOIfbjtii * Gcb)
                               + T0 * dRatio_dVb;
                        Giid = Ratio * (Gds + pParam->B4SOIfbjtii * Gcd)
                               + T0 * dRatio_dVd;
                        /* v3.0 */
                        Giie = Ratio * Gme + T0 * dRatio_dVe;

                        if (selfheat)
                            GiiT = Ratio * (GmT + pParam->B4SOIfbjtii * GcT)
                                   + T0 * dRatio_dT;
                        else
                            GiiT = 0.0;

                    }
                }
                else /*new Iii model*/
                {
                    /*Idsmosfet part*/
                    if (pParam->B4SOIalpha0 <= 0.0)
                    {
                        /* Giig = Giib = Giid = GiiT = 0.0;
                           Giie = 0; */ /* v3.0 */
                        /* here->B4SOIiii = Iii = 0.0; */ /* v4.2 bugfix # 38 */
                        /* Idsmosfet = 0.0; */                          /*v 4.2 bugfix #38 */
                        /*dIiibjt_dVb = 0.0; */                            /* v4.2 bugfix # 38 */
                        /*dIiibjt_dVd = 0.0;
                          dIiibjt_dT  = 0.0; */
                        Ratio = 0;                                                      /* v4.2 bugfix # 38 */
                    }
                    else
                    {
                        Vdsatii0 = pParam->B4SOIvdsatii0 * (1 + model->B4SOItii * (TempRatio-1.0))
                                   - pParam->B4SOIlii / Leff;
                        if (selfheat)
                            dVdsatii0_dT = pParam->B4SOIvdsatii0 * model->B4SOItii * dTempRatio_dT;
                        else
                            dVdsatii0_dT = 0;

                        /* Calculate VgsStep */
                        T0 = pParam->B4SOIesatii * Leff; /* v3.0 bug fix: T0 is dimensionless (i.e., scaled by 1V) */
                        T1 = pParam->B4SOIsii0 * T0 / (1.0 + T0);

                        T0 = 1 / (1 + pParam->B4SOIsii1 * Vgsteff);
                        if (selfheat)
                            dT0_dT = - pParam->B4SOIsii1 * T0 * T0 *dVgsteff_dT;
                        else
                            dT0_dT = 0;
                        T3 = T0 + pParam->B4SOIsii2;
                        T4 = Vgst * pParam->B4SOIsii1 * T0 * T0;
                        T2 = Vgst * T3;
                        dT2_dVg = T3 * (dVgst_dVg - dVth_dVb * dVbseff_dVg) - T4 * dVgsteff_dVg; /* v3.0 */
                        dT2_dVb = T3 * dVgst_dVb * dVbseff_dVb - T4 * dVgsteff_dVb;
                        dT2_dVe = T3 * dVgst_dVb * dVbseff_dVe - T4 * dVgsteff_dVe; /* v3.0 */
                        dT2_dVd = T3 * (dVgst_dVd - dVth_dVb * dVbseff_dVd) - T4 * dVgsteff_dVd; /* v3.0 */
                        if (selfheat)
                            dT2_dT = -(dVth_dT + dVth_dVb * dVbseff_dT) * T3 + Vgst * dT0_dT; /* v3.0 */
                        else dT2_dT = 0;


                        T3 = 1 / (1 + pParam->B4SOIsiid * Vds);
                        dT3_dVd = - pParam->B4SOIsiid * T3 * T3;

                        VgsStep = T1 * T2 * T3;
                        if (selfheat)
                            dVgsStep_dT = T1 * T3 * dT2_dT;
                        else dVgsStep_dT = 0;
                        Vdsatii = Vdsatii0 + VgsStep;
                        Vdiff = Vds - Vdsatii;
                        dVdiff_dVg = - T1 * T3 * dT2_dVg;
                        dVdiff_dVb = - T1 * T3 * dT2_dVb;
                        dVdiff_dVe = - T1 * T3 * dT2_dVe; /* v3.0 */
                        dVdiff_dVd = 1.0 - T1 * (T3 * dT2_dVd + T2 * dT3_dVd);
                        if (selfheat)
                            dVdiff_dT  = -(dVdsatii0_dT + dVgsStep_dT);
                        else dVdiff_dT = 0;

                        T0 = pParam->B4SOIbeta2 + pParam->B4SOIbeta1 * Vdiff
                             + pParam->B4SOIbeta0 * Vdiff * Vdiff;
                        if (T0 < 1e-5)
                        {
                            T0 = 1e-5;
                            dT0_dVg = dT0_dVd = dT0_dVb = dT0_dT = 0.0;
                            dT0_dVe = 0; /* v3.0 */
                        }
                        else
                        {
                            T1 = pParam->B4SOIbeta1 + 2 * pParam->B4SOIbeta0 * Vdiff;
                            dT0_dVg = T1 * dVdiff_dVg;
                            dT0_dVb = T1 * dVdiff_dVb;
                            dT0_dVd = T1 * dVdiff_dVd;
                            dT0_dVe = T1 * dVdiff_dVe; /* v3.0 */
                            if (selfheat)
                                dT0_dT = T1 * dVdiff_dT;
                            else
                                dT0_dT = 0;
                        }

                        if ((T0 < Vdiff / EXPL_THRESHOLD) && (Vdiff > 0.0))
                        {
                            Ratio = pParam->B4SOIalpha0 * MAX_EXPL;
                            dRatio_dVg = dRatio_dVb = dRatio_dVd = dRatio_dT = 0.0;
                            dRatio_dVe = 0; /* v3.0 */
                        }
                        else if ((T0 < -Vdiff / EXPL_THRESHOLD) && (Vdiff < 0.0))
                        {
                            Ratio = pParam->B4SOIalpha0 * MIN_EXPL;
                            dRatio_dVg = dRatio_dVb = dRatio_dVd = dRatio_dT = 0.0;
                            dRatio_dVe = 0; /* v3.0 */
                        }
                        else
                        {
                            Ratio = pParam->B4SOIalpha0 * exp(Vdiff / T0);
                            T1 = Ratio / T0 / T0;
                            dRatio_dVg = T1 * (T0 * dVdiff_dVg - Vdiff * dT0_dVg);
                            dRatio_dVb = T1 * (T0 * dVdiff_dVb - Vdiff * dT0_dVb);
                            dRatio_dVd = T1 * (T0 * dVdiff_dVd - Vdiff * dT0_dVd);
                            /* v3.0 */
                            dRatio_dVe = T1 * (T0 * dVdiff_dVe - Vdiff * dT0_dVe);

                            if (selfheat)
                                dRatio_dT = T1 * (T0 * dVdiff_dT - Vdiff * dT0_dT);
                            else
                                dRatio_dT = 0;
                        }

                        /* Avoid too high ratio */
                        if (Ratio > 10.0)
                        {
                            Ratio = 10.0;
                            dRatio_dVg = dRatio_dVb = dRatio_dVd = dRatio_dT = 0.0;
                            dRatio_dVe = 0; /* v3.0 */
                        }

                        T0 = Ids;
                        Idsmosfet = Ratio * T0;
                    }
                    /*New BJT part*/

                    T0 = (pParam->B4SOIcbjtii + pParam->B4SOIebjtii * Leff)/Leff;

                    Vbci= pParam->B4SOIvbci*(1.0+model->B4SOItvbci*(TempRatio-1.0));
                    /*T1 = Vbci - (Vbs - Vds);          v4.3 bugfix*/
                    T1 = Vbci - (vbs_jct - Vds);

                    T2 = pParam->B4SOImbjtii -1.0;

                    /*
                       if(T1 == 0.0)
                       T3 =1.0;
                       else
                       T3 = -pParam->B4SOIabjtii * pow(T1,T2);
                       */

                    if(T1<=0.0)
                        T3 = 0.0;
                    else
                        T3 = -pParam->B4SOIabjtii * pow(T1,T2);





                    if (T3> EXPL_THRESHOLD)
                        T4 = MAX_EXPL;
                    else if (T3 < -EXPL_THRESHOLD)
                        T4 = MIN_EXPL;
                    else
                        T4 = exp(T3);


                    if (T1==0.0)
                    {
                        if(T3> EXPL_THRESHOLD)
                        {
                            dT4_dVd = 0.0;
                            dT4_dVb = 0.0;
                        }
                        else if (T3 < -EXPL_THRESHOLD)
                        {
                            dT4_dVd = 0.0;
                            dT4_dVb = 0.0;
                        }
                        else
                        {
                            dT4_dVd = - T4 * pParam->B4SOIabjtii* T2 ;
                            dT4_dVb = T4 * pParam->B4SOIabjtii* T2 ;
                        }
                    }
                    else
                    {
                        if(T3> EXPL_THRESHOLD)
                        {
                            dT4_dVd = 0.0;
                            dT4_dVb = 0.0;
                        }
                        else if (T3 < -EXPL_THRESHOLD)
                        {
                            dT4_dVd = 0.0;
                            dT4_dVb = 0.0;
                        }
                        else
                        {
                            T5 = T2-1.0;
                            if (T1<=0.0)
                            {
                                dT4_dVd = 0.0;
                                dT4_dVd = 0.0;

                            }
                            else
                            {
                                dT4_dVd = - T4 * pParam->B4SOIabjtii* T2 * pow(T1,T5);
                                dT4_dVb = T4 * pParam->B4SOIabjtii* T2 * pow(T1,T5);
                            }
                        }

                    }

                    Iiibjt = T0 * Ic * T1 * T4;

                    if (selfheat)
                    {
                        T5= T2-1.0;
                        dVbci_dT = pParam->B4SOIvbci * model->B4SOItvbci *model->B4SOItnom;
                        if(T1<=0.0)
                            dT4_dT = 0.0;
                        else
                            dT4_dT = -T4 * pParam->B4SOIabjtii* T2 * pow(T1,T5)*dVbci_dT;

                        dIiibjt_dT = T0 * Ic * T4 * dVbci_dT
                                     + T0 *Ic *T1 * dT4_dT + T0 * GcT *T1 * T4;   /* Samuel Mertens */

                    }
                    else
                    {
                        dVbci_dT = 0.0;
                        dT4_dT =0.0;
                        dIiibjt_dT = 0.0;
                    }

                    // Xue fix 10/29/2009
                    dIiibjt_dVd = T0 * Ic *T4
                                  + T0 *Ic *T1*dT4_dVd + T0 * dIc_dVd * T1 * T4;
                    dIiibjt_dVb = -T0 * Ic *T4 + T0*Ic*T1*dT4_dVb + T0 * dIc_dVb * T1 * T4;




                    /*Total Iii*/
                    T0 = Ids;
                    here->B4SOIiii = Iii = Idsmosfet + Iiibjt;


                    Giig = Ratio * Gm + T0 * dRatio_dVg;
                    Giib = Ratio * Gmb + T0 * dRatio_dVb + dIiibjt_dVb;
                    Giid = Ratio * Gds + T0 * dRatio_dVd + dIiibjt_dVd;
                    Giie = Ratio * Gme + T0 * dRatio_dVe;

                    if (selfheat)
                        GiiT = Ratio * GmT + T0 * dRatio_dT
                               + dIiibjt_dT ;
                    else
                        GiiT = 0.0;

                }

                /* Current through body resistor */
                /* Current going out is +ve */
                if ((here->B4SOIbodyMod == 0) || (here->B4SOIbodyMod == 2))
                {
                    Ibp = Gbpbs = Gbpps = 0.0;
                }
                else   /* here->B4SOIbodyMod == 1 */
                {
                    if (pParam->B4SOIrbody < 1e-3)      /* 3.2 bug fix */
                    {
                        if (here->B4SOIrbodyext <= 1e-3) /* 3.2 bug fix */
                            T0 = 1.0 / 1e-3; /* 3.2 bug fix */
                        else
                            T0 = 1.0 / here->B4SOIrbodyext;
                        Ibp = Vbp * T0;
                        Gbpbs = T0 * dVbp_dVb;
                        Gbpps = -T0 * dVbp_dVb;
                    }
                    else
                    {
                        Gbpbs = 1.0 / (pParam->B4SOIrbody + here->B4SOIrbodyext);
                        Ibp = Vbp * Gbpbs;
                        Gbpps = - Gbpbs;
                    }
                }

                here->B4SOIibp = Ibp;
                here->B4SOIgbpbs = Gbpbs;
                here->B4SOIgbpps = Gbpps;
                here->B4SOIgbpT = 0.0;
                here->B4SOIcbodcon = (Ibp - (Gbpbs * Vbs + Gbpps * Vps));

            }

            else /* v3.1 soiMod=2: ideal FD */
            {
                Giig = Giib = Giid = Giie = GiiT = 0.0;
                here->B4SOIiii = Iii = 0.0;

                here->B4SOIibp = Ibp = 0.0;
                here->B4SOIgbpbs = 0.0;
                here->B4SOIgbpps = here->B4SOIgbpT = here->B4SOIcbodcon = 0.0;
                Gbpbs = Gbpps = 0.0;
            }
            /* v3.1 */



            /*  Current going out of drainprime node into the drain of device  */
            /*  "node" means the SPICE circuit node  */

            here->B4SOIcdrain = Ids + Ic;
            here->B4SOIcd = Ids + Ic - Ibd + Iii + Igidl;
            here->B4SOIcb = Ibs + Ibd + Ibp / here->B4SOInf - Iii - Igidl - Igisl - Igb; /* v4.2 bug fix # 27*/
            here->B4SOIgds = Gds + Gcd;
            here->B4SOIgm = Gm;
            here->B4SOIgmbs = Gmb + Gcb;
            /* v3.0 */
            here->B4SOIgme = Gme;


            /* v3.1 for RF */
            /* Calculate Rg */
            if (here->B4SOIrgateMod >1)
            {
                T9 = pParam->B4SOIxrcrg2 * model->B4SOIvtm;
                T0 = T9 *beta;
                dT0_dVd = (dbeta_dVd + dbeta_dVg * dVgsteff_dVd) * T9;
                dT0_dVb = (dbeta_dVb + dbeta_dVg * dVgsteff_dVb) * T9;
                dT0_dVg = dbeta_dVg * T9;
                T1 = 1 + gche * Rds;
                T2 = 1 / T1;

                here->B4SOIgcrg = pParam->B4SOIxrcrg1
                                  * (T0 + here->B4SOIidovVds);
                dIdlovVdseff_dVg = (T2 * dgche_dVg
                                    - IdlovVdseff * gche * dRds_dVg) / T1;
                dIdlovVdseff_dVd = T2 * dgche_dVd / T1;
                dIdlovVdseff_dVb = (T2 * dgche_dVb
                                    - IdlovVdseff * gche * dRds_dVb) / T1;

                T9 =  diffVds / Va;
                T3 =  1.0 + T9;

                T4 = T3 * dIdlovVdseff_dVg
                     - IdlovVdseff * (dVdseff_dVg + T9 * dVa_dVg) / Va;
                T5 = T3 * dIdlovVdseff_dVd + IdlovVdseff
                     * (1.0 - dVdseff_dVd - T9 * dVa_dVd) / Va;
                T6 = T3 * dIdlovVdseff_dVb
                     - IdlovVdseff * (dVdseff_dVb + T9 * dVa_dVb) / Va;

                tmp1 = (T4 * dVgsteff_dVd + T6 * dVbseff_dVd + T5)
                       / here->B4SOInseg;
                tmp2 = (T4 * dVgsteff_dVg + T6 * dVbseff_dVg)
                       / here->B4SOInseg;
                tmp3 = (T4 * dVgsteff_dVb + T6 * dVbseff_dVb)
                       / here->B4SOInseg;

                here->B4SOIgcrgd = pParam->B4SOIxrcrg1 * (dT0_dVd +tmp1);
                here->B4SOIgcrgg = pParam->B4SOIxrcrg1
                                   * (dT0_dVg * dVgsteff_dVg + tmp2);
                here->B4SOIgcrgb = pParam->B4SOIxrcrg1
                                   * (dT0_dVb * dVbseff_dVb + tmp3);

                if (here->B4SOInf != 1.0)
                {
                    here->B4SOIgcrg *= here->B4SOInf;
                    here->B4SOIgcrgg *= here->B4SOInf;
                    here->B4SOIgcrgd *= here->B4SOInf;
                    here->B4SOIgcrgb *= here->B4SOInf;
                }

                if (here->B4SOIrgateMod == 2)
                {
                    T10 = here->B4SOIgrgeltd * here->B4SOIgrgeltd;
                    T11 = here->B4SOIgrgeltd + here->B4SOIgcrg;
                    here->B4SOIgcrg = here->B4SOIgrgeltd
                                      * here->B4SOIgcrg / T11;
                    T12 = T10 / T11 /T11;
                    here->B4SOIgcrgg *= T12;
                    here->B4SOIgcrgd *= T12;
                    here->B4SOIgcrgb *= T12;
                }

                here->B4SOIgcrgs = -(here->B4SOIgcrgg + here->B4SOIgcrgd
                                     + here->B4SOIgcrgb);
            } /* v3.1 added Rg for RF end */

            /* v4.0 Calculate bias-dependent external S/D resistance */
            if (model->B4SOIrdsMod)
            {
                /* Rs(V) */
                T0 = vgs - pParam->B4SOIvfbsd;
                T1 = sqrt(T0 * T0 + 1.0e-4);
                vgs_eff = 0.5 * (T0 + T1);
                dvgs_eff_dvg = vgs_eff / T1;

                T0 = 1.0 + pParam->B4SOIprwg * vgs_eff;
                dT0_dVg = -pParam->B4SOIprwg / T0 / T0 * dvgs_eff_dvg;
                T1 = -pParam->B4SOIprwb * vbs;
                dT1_dVb = -pParam->B4SOIprwb;

                T2 = 1.0 / T0 + T1;
                T3 = T2 + sqrt(T2 * T2 + 0.01);
                dT3_dVg = T3 / (T3 - T2);
                dT3_dVb = dT3_dVg * dT1_dVb;
                dT3_dVg *= dT0_dVg;

                T4 = rs0 * 0.5;
                Rs = rswmin + T3 * T4;
                dRs_dVg = T4 * dT3_dVg;
                dRs_dVb = T4 * dT3_dVb;

                T0 = 1.0 + here->B4SOIsourceConductance * Rs;
                here->B4SOIgstot = here->B4SOIsourceConductance / T0;
                T0 = -here->B4SOIgstot * here->B4SOIgstot;
                dgstot_dvd = 0.0; /* place holder */
                dgstot_dve = 0.0; /* place holder */
                dgstot_dvg = T0 * dRs_dVg;
                dgstot_dvb = T0 * dRs_dVb;
                dgstot_dvs = -(dgstot_dvg + dgstot_dvb + dgstot_dvd
                               + dgstot_dve);
                if (selfheat)
                {
                    dRs_dT  = drswmin_dT + T3 * 0.5 * drs0_dT;
                    dgstot_dT = T0 * dRs_dT;
                }
                else dRs_dT = dgstot_dT = 0.0;

                /* Rd(V) */
                T0 = vgd - pParam->B4SOIvfbsd;
                T1 = sqrt(T0 * T0 + 1.0e-4);
                vgd_eff = 0.5 * (T0 + T1);
                dvgd_eff_dvg = vgd_eff / T1;

                T0 = 1.0 + pParam->B4SOIprwg * vgd_eff;
                dT0_dVg = -pParam->B4SOIprwg / T0 / T0 * dvgd_eff_dvg;
                T1 = -pParam->B4SOIprwb * vbd;
                dT1_dVb = -pParam->B4SOIprwb;

                T2 = 1.0 / T0 + T1;
                T3 = T2 + sqrt(T2 * T2 + 0.01);
                dT3_dVg = T3 / (T3 - T2);
                dT3_dVb = dT3_dVg * dT1_dVb;
                dT3_dVg *= dT0_dVg;

                /*T4 = pParam->B4SOIrd0 * 0.5;*/  /* v4.2 bugfix # 37 */
                /*Rd = pParam->B4SOIrdwmin + T3 * T4;*/ /* v4.2 bugfix # 37 */
                T4 = rd0 * 0.5;
                Rd = rdwmin + T3 * T4;
                dRd_dVg = T4 * dT3_dVg;
                dRd_dVb = T4 * dT3_dVb;
                T0 = 1.0 + here->B4SOIdrainConductance * Rd;
                here->B4SOIgdtot = here->B4SOIdrainConductance / T0;
                T0 = -here->B4SOIgdtot * here->B4SOIgdtot;
                dgdtot_dvs = 0.0;
                dgdtot_dve = 0.0;
                dgdtot_dvg = T0 * dRd_dVg;
                dgdtot_dvb = T0 * dRd_dVb;
                dgdtot_dvd = -(dgdtot_dvg + dgdtot_dvb + dgdtot_dvs
                               + dgdtot_dve);
                if (selfheat)
                {
                    dRd_dT  = drdwmin_dT + T3 * 0.5 * drd0_dT;
                    dgdtot_dT = T0 * dRd_dT;
                }
                else dRd_dT = dgdtot_dT = 0.0;

                here->B4SOIgstotd = vses * dgstot_dvd;
                here->B4SOIgstotg = vses * dgstot_dvg;
                here->B4SOIgstots = vses * dgstot_dvs;
                here->B4SOIgstotb = vses * dgstot_dvb;

                T2 = vdes - vds;
                here->B4SOIgdtotd = T2 * dgdtot_dvd;
                here->B4SOIgdtotg = T2 * dgdtot_dvg;
                here->B4SOIgdtots = T2 * dgdtot_dvs;
                here->B4SOIgdtotb = T2 * dgdtot_dvb;
            }
            else
            {
                here->B4SOIgstot = here->B4SOIgstotd = here->B4SOIgstotg
                                                       = here->B4SOIgstots = here->B4SOIgstotb
                                                               = 0.0;
                here->B4SOIgdtot = here->B4SOIgdtotd = here->B4SOIgdtotg
                                                       = here->B4SOIgdtots = here->B4SOIgdtotb
                                                               = 0.0;
            }

            if (selfheat)
                here->B4SOIgmT = GmT + GcT;
            else
                here->B4SOIgmT = 0.0;

            /*  note that sign is switched because power flows out
                of device into the temperature node.
                Currently omit self-heating due to bipolar current
                because it can cause convergence problem*/

            here->B4SOIgtempg = -model->B4SOItype*Gm * Vds;
            here->B4SOIgtempb = -model->B4SOItype*Gmb * Vds;
            /* v3.0 */
            here->B4SOIgtempe = -model->B4SOItype*Gme * Vds;

            here->B4SOIgtempT = -GmT * Vds;
            here->B4SOIgtempd = -model->B4SOItype* (Gds * Vds + Ids);
            here->B4SOIcth = - Ids * Vds - model->B4SOItype *
                             (here->B4SOIgtempg * Vgs + here->B4SOIgtempb * Vbs
                              + here->B4SOIgtempe * Ves
                              + here->B4SOIgtempd * Vds)
                             - here->B4SOIgtempT * delTemp; /* v3.0 */


            /*  Body current which flows into drainprime node from the drain of device  */

            here->B4SOIgjdb = Gjdb - Giib -Ggidlb - Ggislb; /* v4.0 */
            here->B4SOIgjdd = Gjdd - (Giid + Ggidld);
            here->B4SOIgjdg = - (Giig + Ggidlg + Ggislg);
            here->B4SOIgjde = - Giie;
            if (selfheat) here->B4SOIgjdT = GjdT - GiiT;
            else here->B4SOIgjdT = 0.0;
            here->B4SOIcjd = Ibd - Iii - Igidl
                             - (here->B4SOIgjdb * Vbs
                                +  here->B4SOIgjdd * Vds
                                +  here->B4SOIgjdg * Vgs
                                +  here->B4SOIgjde * Ves
                                +  here->B4SOIgjdT * delTemp); /* v3.0 */

            if (!here->B4SOIrbodyMod)
            {
                /* Giigidl_b = Giigidl_d = Giigidl_g = Giigidl_e
                                                    = */ Giigidl_T = Iii_Igidl = 0.0;
            }
            else
            {
                here->B4SOIgiigidlb = Giib + Ggidlb + Ggislb;
                here->B4SOIgiigidld = Giid + Ggidld;
//                Giigidl_b =  - Giib -Ggidlb - Ggislb;
//                Giigidl_d =  - Giid -Ggidld;
//                Giigidl_g =  - Giig -Ggidlg - Ggislg;
//                Giigidl_e =  - Giie;
                if (selfheat) Giigidl_T = -GiiT;
                else GiiT = Giigidl_T = 0.0;

                /*Idbdp = Ibd - ( Gjdb * vbs_jct + Gjdd * Vds
                  + GjdT * delTemp);            v4.2 bugfix */
                Idbdp = Ibd - ( Gjdb * vbd_jct + Gjdd * Vds + GjdT * delTemp);
                /*                      Iii_Igidl = - Iii - Igidl
                            + Giigidl_b * Vbs + Giigidl_d * Vds
                            + Giigidl_g * Vgs + Giigidl_e * Ves
                            + Giigidl_T * delTemp ; */
            }

            /*  Body current which flows into sourceprime node from the source of device  */

            here->B4SOIgjsg = 0.0;
            here->B4SOIgjsd = Gjsd;
            here->B4SOIgjsb = Gjsb; /* v4.0 */
            if (selfheat) here->B4SOIgjsT = GjsT;
            else here->B4SOIgjsT = 0.0;
            here->B4SOIcjs = Ibs - Igisl
                             -( here->B4SOIgjsb * Vbs
                                + here->B4SOIgjsd * Vds
                                + here->B4SOIgjsg * Vgs
                                + here->B4SOIgjsT * delTemp);

            if (here->B4SOIrbodyMod)
            {
                Isbsp = Ibs - ( Gjsb * vbs_jct + Gjsd * Vds
                                + GjsT * delTemp );
            }

            /*  Current flowing into body node  */

            here->B4SOIgbbs = Giib - Gjsb - Gjdb - Gbpbs / here->B4SOInf; /* v4.2 bug fix #27 */
            here->B4SOIgbgs = Giig + Ggidlg + Ggislg;
            here->B4SOIgbds = Giid + Ggidld + Ggisls - Gjsd - Gjdd;
            here->B4SOIgbes = Giie;
            here->B4SOIgbps = - Gbpps / here->B4SOInf;  /* v4.2 bug fix #27 */
            if (selfheat) here->B4SOIgbT = GiiT - GjsT - GjdT;
            else here->B4SOIgbT = 0.0;

            if (!here->B4SOIrbodyMod)
            {
                here->B4SOIcbody = Iii + Igidl + Igisl - Ibs - Ibd
                                   - Ibp / here->B4SOInf + Igb                                         /* v4.2 bug fix #27 */
                                   - ( (here->B4SOIgbbs + dIgb_dVb) * Vbs
                                       + (here->B4SOIgbgs + dIgb_dVg) * Vgs
                                       + (here->B4SOIgbds + dIgb_dVd) * Vds
                                       + here->B4SOIgbps * Vps
                                       + (here->B4SOIgbes + dIgb_dVe) * Ves
                                       + (here->B4SOIgbT + dIgb_dT) * delTemp);
            }

            if (here->B4SOIrbodyMod)
            {
                here->B4SOIgbgiigbpb = Giib - Gbpbs / here->B4SOInf; /* v4.3 bug fix */
                here->B4SOIcbody = Iii + Igidl + Igisl - Ibp / here->B4SOInf + Igb              /* v4.2 bug fix #27 */
                                   - ( (Giib - Gbpbs / here->B4SOInf + dIgb_dVb) * Vbs                                         /* v4.2 bug fix #27 */
                                       + (here->B4SOIgbgs + dIgb_dVg) * Vgs
                                       + (Giid + Ggidld + dIgb_dVd) * Vds
                                       + here->B4SOIgbps * Vps
                                       + (here->B4SOIgbes + dIgb_dVe) * Ves
                                       + (GiiT + dIgb_dT) * delTemp );
            }

            here->B4SOIcgate = Igb
                               - (dIgb_dVb * Vbs + dIgb_dVe * Ves + dIgb_dVg * Vgs
                                  + dIgb_dVd * Vds + dIgb_dT * delTemp); /* v3.0 */

            /* Calculate Qinv for Noise analysis */

            T1 = Vgsteff * (1.0 - 0.5 * Abulk * Vdseff / Vgst2Vtm);
            here->B4SOIqinv = -model->B4SOIcox * pParam->B4SOIweff
                              * here->B4SOInf * Leff * T1; /* v4.0 */

            if (here->B4SOInf != 1)
            {
                here->B4SOIcdrain *= here->B4SOInf;
                here->B4SOIcd *= here->B4SOInf;
                here->B4SOIcb *= here->B4SOInf;
                here->B4SOIgds *= here->B4SOInf;
                here->B4SOIgm *= here->B4SOInf;
                here->B4SOIgmbs *= here->B4SOInf;
                here->B4SOIgme *= here->B4SOInf;
                // Xue fix 10/29/2009
                /* here->B4SOIgmT *= here->B4SOInf; *added in line 5424 */
                here->B4SOIcbody *= here->B4SOInf;


                here->B4SOIcgate *= here->B4SOInf;

                here->B4SOIIgcs *= here->B4SOInf;
                here->B4SOIgIgcsg *= here->B4SOInf;
                here->B4SOIgIgcsd *= here->B4SOInf;
                here->B4SOIgIgcsb *= here->B4SOInf;
                here->B4SOIIgcd *= here->B4SOInf;
                here->B4SOIgIgcdg *= here->B4SOInf;
                here->B4SOIgIgcdd *= here->B4SOInf;
                here->B4SOIgIgcdb *= here->B4SOInf;

                here->B4SOIIgs *= here->B4SOInf;
                here->B4SOIgIgsg *= here->B4SOInf;
                here->B4SOIgIgss *= here->B4SOInf;
                here->B4SOIIgd *= here->B4SOInf;
                here->B4SOIgIgdg *= here->B4SOInf;
                here->B4SOIgIgdd *= here->B4SOInf;

                here->B4SOIig *= here->B4SOInf;
                here->B4SOIgigg *= here->B4SOInf;
                here->B4SOIgigd *= here->B4SOInf;
                here->B4SOIgigb *= here->B4SOInf;
                here->B4SOIgige *= here->B4SOInf;
                here->B4SOIgigT *= here->B4SOInf;

                here->B4SOIcjs *= here->B4SOInf;
                here->B4SOIcjd *= here->B4SOInf;
                here->B4SOIibs *= here->B4SOInf;
                here->B4SOIibd *= here->B4SOInf;
                Idbdp *= here->B4SOInf;         /*v4.2 bug fix Idbdp needs update as Ibd for nf!=1*/
                Isbsp *= here->B4SOInf;         /*v4.2 bug fix Isbsp needs update as Ibd for nf!=1*/
                here->B4SOIgbbs *= here->B4SOInf;
                here->B4SOIgbgs *= here->B4SOInf;
                here->B4SOIgbds *= here->B4SOInf;
                here->B4SOIgbes *= here->B4SOInf;
                here->B4SOIgbps *= here->B4SOInf;
                here->B4SOIgbT  *= here->B4SOInf;
                here->B4SOIigidl *= here->B4SOInf;
                here->B4SOIigisl *= here->B4SOInf;

                /* bugfix_snps NF*/
                here->B4SOIgjdb *= here->B4SOInf;
                here->B4SOIgjdd *= here->B4SOInf;
                here->B4SOIgjdg *= here->B4SOInf;
                here->B4SOIgjde *= here->B4SOInf;
                here->B4SOIgjdT *= here->B4SOInf;
                here->B4SOIgjsb *= here->B4SOInf;
                here->B4SOIgjsd *= here->B4SOInf;
                here->B4SOIgjsg *= here->B4SOInf;
                here->B4SOIgjsT *= here->B4SOInf;

                here->B4SOIcth  *= here->B4SOInf;
                here->B4SOIgmT  *= here->B4SOInf;
                here->B4SOIgtempg *= here->B4SOInf;
                here->B4SOIgtempb *= here->B4SOInf;
                here->B4SOIgtempe *= here->B4SOInf;
                here->B4SOIgtempT *= here->B4SOInf;
                here->B4SOIgtempd *= here->B4SOInf;
                here->B4SOIiii  *= here->B4SOInf;
                /* bugfix NF ends */
            }
            here->B4SOIgigs = -(here->B4SOIgigg + here->B4SOIgigd
                                + here->B4SOIgigb + here->B4SOIgige);
            here->B4SOIgIgcss = -(here->B4SOIgIgcsg + here->B4SOIgIgcsd
                                  + here->B4SOIgIgcsb);
            here->B4SOIgIgcds = -(here->B4SOIgIgcdg + here->B4SOIgIgcdd
                                  + here->B4SOIgIgcdb);

            /*  Begin CV (charge) model  */

            if ((model->B4SOIxpart < 0) || (!ChargeComputationNeeded))
            {
                qgate  = qdrn = qsrc = qbody = qsub = 0.0; /* v2.2.3 bug fix */
                Qsub0=Qac0=Cbb=Cbg=Cbd=0;                                               /* Bugfix #19 Jul09*/
                here->B4SOIcggb = here->B4SOIcgsb = here->B4SOIcgdb = 0.0;
                here->B4SOIcdgb = here->B4SOIcdsb = here->B4SOIcddb = 0.0;
                here->B4SOIcbgb = here->B4SOIcbsb = here->B4SOIcbdb = 0.0;
                goto finished;
            }
            else
            {
                CoxWL  = model->B4SOIcox * (pParam->B4SOIweffCV
                                            / here->B4SOInseg * here->B4SOInf /* v4.0 */
                                            * pParam->B4SOIleffCV + here->B4SOIagbcp);
                CoxWLb = model->B4SOIfbody * model->B4SOIcox
                         * (pParam->B4SOIweffCV / here->B4SOInseg
                            * here->B4SOInf     /* v4.0 */
                            * pParam->B4SOIleffCVb + here->B4SOIagbcp);
                /* v4.1 for improved BT charge model */

                CoxWL2  = model->B4SOIcox * here->B4SOIagbcp2;
                CoxWLb2 = model->B4SOIfbody * model->B4SOIcox * here->B4SOIagbcp2;
                /* end v4.1 */


                /* v3.2 Seperate VgsteffCV with noff */
                noff = n * pParam->B4SOInoff;
                dnoff_dVd = pParam->B4SOInoff * dn_dVd;
                dnoff_dVb = pParam->B4SOInoff * dn_dVb;
                if (model->B4SOIvgstcvMod == 0)
                {
                    if ((VgstNVt > -EXPL_THRESHOLD) && (VgstNVt < EXPL_THRESHOLD))
                    {
                        ExpVgst *= ExpVgst;
                        ExpVgst *= exp( -(pParam->B4SOIdelvt / (noff * Vtm)));
                        Vgsteff = noff * Vtm * log(1.0 + ExpVgst);

                        T0 = ExpVgst / (1.0 + ExpVgst);
                        T1 = -T0 * (dVth_dVb + (Vgst-pParam->B4SOIdelvt) / noff * dnoff_dVb)
                             + Vgsteff / noff * dnoff_dVb;
                        dVgsteff_dVd = -T0 * (dVth_dVd + dVth_dVb*dVbseff_dVd + (Vgst-pParam->B4SOIdelvt)/ noff * dnoff_dVd)
                                       + Vgsteff / noff * dnoff_dVd;
                        dVgsteff_dVg = T0 * (dVgs_eff_dVg - dVth_dVb*dVbseff_dVg);
                        dVgsteff_dVb = T1 * dVbseff_dVb;
                        dVgsteff_dVe = T1 * dVbseff_dVe;
                        if (selfheat)
                            dVgsteff_dT = -T0 * (dVth_dT+dVth_dVb*dVbseff_dT
                                                 + (Vgst - pParam->B4SOIdelvt) / Temp)
                                          + Vgsteff / Temp;
                        else dVgsteff_dT  = 0.0;
                        /* v4.1 */
                        if (here->B4SOIagbcp2 > 0)
                        {
                            ExpVgst2 = ExpVgst * exp(-1.12 / noff / Vtm);
                            Vgsteff2 = noff * Vtm * log(1.0 + ExpVgst2);
                            T02 = ExpVgst2 / (1.0 + ExpVgst2);
                            T12 = -T02 * (dVth_dVb + (Vgst-1.12-pParam->B4SOIdelvt) / noff * dnoff_dVb)
                                  + Vgsteff2 / noff * dnoff_dVb;
                            dVgsteff2_dVd = -T02 * (dVth_dVd + dVth_dVb*dVbseff_dVd + (Vgst-pParam->B4SOIdelvt-1.12) / noff * dnoff_dVd)
                                            + Vgsteff2 / noff * dnoff_dVd;
                            dVgsteff2_dVg = T02 * (dVgs_eff_dVg - dVth_dVb*dVbseff_dVg);
                            dVgsteff2_dVb = T12 * dVbseff_dVb;
//                            dVgsteff2_dVe = T12 * dVbseff_dVe;
                            if (selfheat)
                                dVgsteff2_dT = -T02 * (dVth_dT+dVth_dVb*dVbseff_dT
                                                       + (Vgst - 1.12 - pParam->B4SOIdelvt) / Temp)
                                               + Vgsteff2 / Temp;
                            else dVgsteff2_dT  = 0.0;
                        }
                    }
                }
                else if (model->B4SOIvgstcvMod == 1)
                {
                    ExpVgst = exp(VgstNVt/(pParam->B4SOImstar * pParam->B4SOInoff));
                    ExpVgst *= exp( -(pParam->B4SOIdelvt / (noff * Vtm)));
                    Vgsteff = noff * Vtm * log(1.0 + ExpVgst);

                    T0 = ExpVgst / (1.0 + ExpVgst);
                    T1 = -T0 * (dVth_dVb + (Vgst-pParam->B4SOIdelvt) / noff * dnoff_dVb)
                         + Vgsteff / noff * dnoff_dVb;
                    dVgsteff_dVd = -T0 * (dVth_dVd + dVth_dVb*dVbseff_dVd + (Vgst-pParam->B4SOIdelvt) / noff * dnoff_dVd)
                                   + Vgsteff / noff * dnoff_dVd;
                    dVgsteff_dVg =  T0 * (dVgs_eff_dVg - dVth_dVb*dVbseff_dVg);

                    dVgsteff_dVb = T1 * dVbseff_dVb;
                    dVgsteff_dVe = T1 * dVbseff_dVe;
                    if (selfheat)
                        dVgsteff_dT = -T0 * (dVth_dT+dVth_dVb*dVbseff_dT
                                             + (Vgst - pParam->B4SOIdelvt) / Temp)
                                      + Vgsteff / Temp;
                    else dVgsteff_dT  = 0.0;
                    /* v4.1 */
                    if (here->B4SOIagbcp2 > 0)
                    {
                        ExpVgst2 = ExpVgst * exp(-1.12 / noff / Vtm);
                        Vgsteff2 = noff * Vtm * log(1.0 + ExpVgst2);
                        T02 = ExpVgst2 / (1.0 + ExpVgst2);
                        T12 = -T02 * (dVth_dVb + (Vgst-1.12-pParam->B4SOIdelvt) / noff * dnoff_dVb)
                              + Vgsteff2 / noff * dnoff_dVb;
                        dVgsteff2_dVd = -T02 * (dVth_dVd + dVth_dVb*dVbseff_dVd + (Vgst-pParam->B4SOIdelvt - 1.12) / noff * dnoff_dVd)
                                        + Vgsteff2 / noff * dnoff_dVd;
                        dVgsteff2_dVg = T02 * (dVgs_eff_dVg - dVth_dVb*dVbseff_dVg);
                        dVgsteff2_dVb = T12 * dVbseff_dVb;
//                        dVgsteff2_dVe = T12 * dVbseff_dVe;
                        if (selfheat)
                            dVgsteff2_dT = -T02 * (dVth_dT+dVth_dVb*dVbseff_dT
                                                   + (Vgst - 1.12 - pParam->B4SOIdelvt) / Temp)
                                           + Vgsteff2 / Temp;
                        else dVgsteff2_dT  = 0.0;
                    }


                }
                else
                {
                    T10 = noff * Vtm;
                    VgstNVt = pParam->B4SOImstarcv * (Vgst - pParam->B4SOIdelvt) / T10;
                    ExpArg = (pParam->B4SOIvoffcv -
                              (1- pParam->B4SOImstarcv) * (Vgst - pParam->B4SOIdelvt))/ T10;

                    /* MCJ: Very small Vgst */
                    if (VgstNVt > EXPL_THRESHOLD)
                    {
                        Vgsteff = Vgst - pParam->B4SOIdelvt;
                        /* T0 is dVgsteff_dVbseff */
                        T0 = -dVth_dVb;
                        dVgsteff_dVg = dVgs_eff_dVg + T0 * dVbseff_dVg;
                        dVgsteff_dVd = -dVth_dVd + T0 * dVbseff_dVd;
                        dVgsteff_dVb = T0 * dVbseff_dVb;
                        dVgsteff_dVe = T0 * dVbseff_dVe;
                        if (selfheat)
                            dVgsteff_dT  = -dVth_dT + T0 * dVbseff_dT;
                        else
                            dVgsteff_dT = 0.0;
                    }
                    else if (ExpArg > EXPL_THRESHOLD)
                    {
                        T0 = (Vgst - pParam->B4SOIdelvt - pParam->B4SOIvoffcv) / (noff * Vtm);
                        ExpVgst = exp(T0);
                        /*Vgsteff = Vtm * pParam->B4SOIcdep0 / model->B4SOIcox * ExpVgst;*/ /*v4.2 bug fix*/
                        Vgsteff = Vtm * cdep0 / model->B4SOIcox * ExpVgst; /* v4.2 bug fix */
                        T3 = Vgsteff / (noff * Vtm) ;
                        /* T1 is dVgsteff_dVbseff */
                        T1  = -T3 * (dVth_dVb + T0 * Vtm * dnoff_dVb);
                        dVgsteff_dVg = T3 * dVgs_eff_dVg+ T1 * dVbseff_dVg;
                        dVgsteff_dVd = -T3 * (dVth_dVd + T0 * Vtm * dnoff_dVd)+ T1 * dVbseff_dVd;
                        dVgsteff_dVe = T1 * dVbseff_dVe;
                        dVgsteff_dVb = T1 * dVbseff_dVb;
                        if (selfheat)
                            dVgsteff_dT = -T3 * (dVth_dT + T0 * dVtm_dT * noff)
                                          + Vgsteff / Temp+ T1 * dVbseff_dT;
                        else
                            dVgsteff_dT = 0.0;
                    }
                    else
                    {
                        ExpVgst = exp(VgstNVt);
                        T1 = T10 * log(1.0 + ExpVgst);
                        dT1_dVg = ExpVgst / (1.0 + ExpVgst) * pParam->B4SOImstarcv;
                        dT1_dVb = -dT1_dVg * (dVth_dVb + (Vgst-pParam->B4SOIdelvt) / noff * dnoff_dVb)
                                  + T1 / noff * dnoff_dVb;
                        dT1_dVd = -dT1_dVg * (dVth_dVd + (Vgst-pParam->B4SOIdelvt) / noff * dnoff_dVd)
                                  + T1 / noff * dnoff_dVd;
                        T3 = (1.0 / Temp);
                        if (selfheat)
                            dT1_dT = -dT1_dVg * (dVth_dT + (Vgst-pParam->B4SOIdelvt) * T3) + T1 * T3;
                        else
                            dT1_dT = 0.0;

                        /* dT2_dVg = -model->B4SOIcox / (Vtm * pParam->B4SOIcdep0)
                         * exp(ExpArg) * (1 - pParam->B4SOImstarcv); */ /* v4.2 bug fix */
                        dT2_dVg = -model->B4SOIcox / (Vtm * cdep0)
                                  * exp(ExpArg) * (1 - pParam->B4SOImstarcv); /* v4.2 bug fix */
                        T2 = pParam->B4SOImstarcv - T10 * dT2_dVg
                             / (1.0 - pParam->B4SOImstarcv);
                        dT2_dVd = -dT2_dVg * (dVth_dVd - Vtm * ExpArg * dnoff_dVd
                                              / (1.0 - pParam->B4SOImstarcv))
                                  + (T2 - pParam->B4SOImstarcv) / noff * dnoff_dVd;
                        dT2_dVb = -dT2_dVg * (dVth_dVb - Vtm * ExpArg * dnoff_dVb
                                              / (1.0 - pParam->B4SOImstarcv))
                                  + (T2 - pParam->B4SOImstarcv) / noff * dnoff_dVb;
                        if (selfheat)
                            dT2_dT = -dT2_dVg * ( dVth_dT - ExpArg * T10 * T3
                                                  / (1.0 - pParam->B4SOImstarcv) );
                        else
                            dT2_dT = 0.0;

                        Vgsteff = T1 / T2;

                        T3 = T2 * T2;
                        /*  T4 is dVgsteff_dVbseff  */
                        T4 = (T2 * dT1_dVb - T1 * dT2_dVb) / T3;
                        dVgsteff_dVb = T4 * dVbseff_dVb;
                        dVgsteff_dVe = T4 * dVbseff_dVe;
                        dVgsteff_dVg = (T2 * dT1_dVg - T1 * dT2_dVg)
                                       / T3 * dVgs_eff_dVg
                                       + T4 * dVbseff_dVg;

                        dVgsteff_dVd = (T2 * dT1_dVd - T1 * dT2_dVd)
                                       / T3+ T4 * dVbseff_dVd;
                        if (selfheat)
                            dVgsteff_dT = (T2 * dT1_dT - T1 * dT2_dT)
                                          / T3+ T4 * dVbseff_dT;
                        else
                            dVgsteff_dT = 0.0;
                    }



                    if (here->B4SOIagbcp2 > 0)
                    {
                        VgstNVt2 = pParam->B4SOImstarcv * (Vgst - pParam->B4SOIdelvt - 1.12) / T10;
                        ExpArg2 = (pParam->B4SOIvoffcv -
                                   (1- pParam->B4SOImstarcv) * (Vgst - pParam->B4SOIdelvt - 1.12))/ T10;

                        /* MCJ: Very small Vgst */
                        if (VgstNVt2 > EXPL_THRESHOLD)
                        {
                            Vgsteff2 = Vgst - pParam->B4SOIdelvt - 1.12;
                            /* T0 is dVgsteff2_dVbseff */
                            T0 = -dVth_dVb;
                            dVgsteff2_dVg = dVgs_eff_dVg + T0 * dVbseff_dVg;
                            dVgsteff2_dVd = -dVth_dVd + T0 * dVbseff_dVd;
                            dVgsteff2_dVb = T0 * dVbseff_dVb;
//                            dVgsteff2_dVe = T0 * dVbseff_dVe;
                            if (selfheat)
                                dVgsteff2_dT  = -dVth_dT + T0 * dVbseff_dT;
                            else
                                dVgsteff2_dT = 0.0;
                        }
                        else if (ExpArg2 > EXPL_THRESHOLD)
                        {
                            T0 = (Vgst - pParam->B4SOIdelvt - pParam->B4SOIvoffcv - 1.12) / (noff * Vtm);
                            ExpVgst2 = exp(T0);
                            /*Vgsteff2 = Vtm * pParam->B4SOIcdep0 / model->B4SOIcox * ExpVgst*/
                            Vgsteff2 = Vtm * cdep0 / model->B4SOIcox * ExpVgst2; /*v4.2 bug fix */
                            T3 = Vgsteff2 / (noff * Vtm) ;
                            /* T1 is dVgsteff2_dVbseff */
                            T1  = -T3 * (dVth_dVb + T0 * Vtm * dnoff_dVb);
                            dVgsteff2_dVg = T3 * dVgs_eff_dVg+ T1 * dVbseff_dVg;
                            dVgsteff2_dVd = -T3 * (dVth_dVd + T0 * Vtm * dnoff_dVd)+ T1 * dVbseff_dVd;
//                            dVgsteff2_dVe = T1 * dVbseff_dVe;
                            dVgsteff2_dVb = T1 * dVbseff_dVb;
                            if (selfheat)
                                dVgsteff2_dT = -T3 * (dVth_dT + T0 * dVtm_dT * noff)
                                               + Vgsteff2 / Temp+ T1 * dVbseff_dT;
                            else
                                dVgsteff2_dT = 0.0;
                        }
                        else
                        {
                            ExpVgst2 = exp(VgstNVt2);
                            T1 = T10 * log(1.0 + ExpVgst2);
                            dT1_dVg = ExpVgst2 / (1.0 + ExpVgst2) * pParam->B4SOImstarcv;
                            dT1_dVb = -dT1_dVg * (dVth_dVb + (Vgst - pParam->B4SOIdelvt - 1.12) / noff * dnoff_dVb)
                                      + T1 / noff * dnoff_dVb;
                            dT1_dVd = -dT1_dVg * (dVth_dVd + (Vgst - pParam->B4SOIdelvt - 1.12) / noff * dnoff_dVd)
                                      + T1 / noff * dnoff_dVd;
                            T3 = (1.0 / Temp);
                            if (selfheat)
                                dT1_dT = -dT1_dVg * (dVth_dT + (Vgst - pParam->B4SOIdelvt - 1.12) * T3) + T1 * T3;
                            else
                                dT1_dT = 0.0;

                            /* dT2_dVg = -model->B4SOIcox / (Vtm * pParam->B4SOIcdep0)
                             * exp(ExpArg2) * (1 - pParam->B4SOImstarcv);*/
                            dT2_dVg = -model->B4SOIcox / (Vtm * cdep0)
                                      * exp(ExpArg2) * (1 - pParam->B4SOImstarcv); /*v4.2 bug fix */
                            T2 = pParam->B4SOImstarcv - T10 * dT2_dVg
                                 / (1.0 - pParam->B4SOImstarcv);
                            dT2_dVd = -dT2_dVg * (dVth_dVd - Vtm * ExpArg2 * dnoff_dVd
                                                  / (1.0 - pParam->B4SOImstarcv))
                                      + (T2 - pParam->B4SOImstarcv) / noff * dnoff_dVd;
                            dT2_dVb = -dT2_dVg * (dVth_dVb - Vtm * ExpArg2 * dnoff_dVb
                                                  / (1.0 - pParam->B4SOImstarcv))
                                      + (T2 - pParam->B4SOImstarcv) / noff * dnoff_dVb;
                            if (selfheat)
                                dT2_dT = -dT2_dVg * ( dVth_dT - ExpArg2 * T10 * T3
                                                      / (1.0 - pParam->B4SOImstarcv) );
                            else
                                dT2_dT = 0.0;

                            Vgsteff2 = T1 / T2;
                            T3 = T2 * T2;
                            /*  T4 is dVgsteff2_dVbseff  */
                            T4 = (T2 * dT1_dVb - T1 * dT2_dVb) / T3;
                            dVgsteff2_dVb = T4 * dVbseff_dVb;
//                            dVgsteff2_dVe = T4 * dVbseff_dVe;
                            dVgsteff2_dVg = (T2 * dT1_dVg - T1 * dT2_dVg)
                                            / T3 * dVgs_eff_dVg
                                            + T4 * dVbseff_dVg;
                            dVgsteff2_dVd = (T2 * dT1_dVd - T1 * dT2_dVd)
                                            / T3+ T4 * dVbseff_dVd;
                            if (selfheat)
                                dVgsteff2_dT = (T2 * dT1_dT - T1 * dT2_dT)
                                               / T3+ T4 * dVbseff_dT;
                            else
                                dVgsteff2_dT = 0.0;
                        }
                    }
                }
                /* v3.2 */
                /* v3.2 */


                if (model->B4SOIcapMod == 2)
                {

                    /* v3.1 */
                    if (here->B4SOIsoiMod == 2) /* v3.2 */ /* ideal FD */
                    {
                        Qac0 = dQac0_dVrg = dQac0_dVd = dQac0_dVb = dQac0_dT = 0.0;
                        dQac02_dVrg = dQac02_dVd = dQac02_dVb = dQac02_dT  = 0.0;
                        Qsub0 = dQsub0_dVrg = dQsub0_dVg = dQsub0_dVd = dQsub0_dVb = dQsub0_dT = 0.0;
                        dQsub02_dVrg = dQsub02_dVg = dQsub02_dVd = dQsub02_dVb = dQsub02_dT = 0.0;
                    }
                    else /* soiMod = 0 or 1 */
                    {
                        Vfb = Vth - phi - pParam->B4SOIk1eff * sqrtPhis + pParam->B4SOIdelvt;
                        dVfb_dVb = dVth_dVb - pParam->B4SOIk1eff * dsqrtPhis_dVb;
                        dVfb_dVd = dVth_dVd;
                        dVfb_dT  = dVth_dT;

                        V3 = Vfb - Vgs_eff + Vbseff - DELTA_3_SOI;
                        if (Vfb <= 0.0)
                        {
                            T0 = sqrt(V3 * V3 - 4.0 * DELTA_3_SOI * Vfb);
                            T2 = -DELTA_3_SOI / T0;
                        }
                        else
                        {
                            T0 = sqrt(V3 * V3 + 4.0 * DELTA_3_SOI * Vfb);
                            T2 = DELTA_3_SOI / T0;
                        }

                        T1 = 0.5 * (1.0 + V3 / T0);
                        Vfbeff = Vfb - 0.5 * (V3 + T0);
                        dVfbeff_dVd = (1.0 - T1 - T2) * dVfb_dVd;
                        dVfbeff_dVb = (1.0 - T1 - T2) * dVfb_dVb - T1;
                        dVfbeff_dVrg = T1 * dVgs_eff_dVg;
                        if (selfheat) dVfbeff_dT = (1.0 - T1 - T2) * dVfb_dT;
                        else  dVfbeff_dT = 0.0;

                        Qac0 = CoxWLb * (Vfbeff - Vfb);
                        dQac0_dVrg = CoxWLb * dVfbeff_dVrg;
                        dQac0_dVd = CoxWLb * (dVfbeff_dVd - dVfb_dVd);
                        dQac0_dVb = CoxWLb * (dVfbeff_dVb - dVfb_dVb);
                        if (selfheat) dQac0_dT = CoxWLb * (dVfbeff_dT - dVfb_dT);
                        else  dQac0_dT = 0.0;
                        /* v4.1 */
                        if ((here->B4SOIsoiMod != 2) &&                                                         /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' Jun 09 */
                                ( here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                        {
                            Vfb2 = Vfb + 1.12;
                            dVfb2_dVb = dVfb_dVb;
                            dVfb2_dVd = dVfb_dVd;
                            dVfb2_dT  = dVfb_dT;
                            DELTA_3_SOI2 =  DELTA_3_SOI;
                            V3 = Vfb2 - Vgs_eff2 + Vbseff - DELTA_3_SOI2;
                            if (Vfb2 <= 0.0)
                            {
                                T0 = sqrt(V3 * V3 - 100.0 * DELTA_3_SOI2 * Vfb2);
                                T2 = -25.0 * DELTA_3_SOI2 / T0;
                            }
                            else
                            {
                                T0 = sqrt(V3 * V3 + 100.0 * DELTA_3_SOI2 * Vfb2);
                                T2 = 25.0 * DELTA_3_SOI2 / T0;
                            }
                            T1 = 0.5 * (1.0 + V3 / T0);
                            Vfbeff2 = Vfb2 - 0.5 * (V3 + T0);
                            dVfbeff2_dVd = (1.0 - T1 - T2) * dVfb2_dVd;        /* Samuel Mertens */
                            dVfbeff2_dVb = (1.0 - T1 - T2) * dVfb2_dVb - T1;   /* Samuel Mertens */
                            dVfbeff2_dVrg = T1 * dVgs_eff2_dVg;
                            if (selfheat) dVfbeff2_dT = (1.0 - T1 - T2) * dVfb2_dT;
                            else  dVfbeff2_dT = 0.0;

                            Qac0 += CoxWLb2 * (Vfbeff2 - Vfb2);
                            dQac02_dVrg = CoxWLb2 * dVfbeff2_dVrg;
                            dQac02_dVd = CoxWLb2 * (dVfbeff2_dVd - dVfb2_dVd);
                            dQac02_dVb = CoxWLb2 * (dVfbeff2_dVb - dVfb2_dVb);
                            if (selfheat)
                                dQac02_dT = CoxWLb2 * (dVfbeff2_dT - dVfb2_dT);
                            else  dQac02_dT = 0.0;
                        }
                        /* end v4.1 */
                        T0 = 0.5 * pParam->B4SOIk1ox;
                        T3 = Vgs_eff - Vfbeff - Vbseff - Vgsteff;
                        if (pParam->B4SOIk1ox == 0.0)
                        {
                            T1 = 0.0;
                            T2 = 0.0;
                        }
                        else if (T3 < 0.0)
                        {
                            T1 = T0 + T3 / pParam->B4SOIk1ox;
                            T2 = CoxWLb;
                        }
                        else
                        {
                            T1 = sqrt(T0 * T0 + T3);
                            T2 = CoxWLb * T0 / T1;
                        }

                        Qsub0 = CoxWLb * pParam->B4SOIk1ox * (T1 - T0); /* 4.1 bug fix */
                        dQsub0_dVrg = T2 * (dVgs_eff_dVg - dVfbeff_dVrg);
                        dQsub0_dVg = -T2;
                        dQsub0_dVd = -T2 * dVfbeff_dVd;
                        dQsub0_dVb = -T2 * (dVfbeff_dVb + 1);
                        if (selfheat) dQsub0_dT  = -T2 * dVfbeff_dT;
                        else  dQsub0_dT = 0.0;
                        /* v4.1 */
                        if ((here->B4SOIsoiMod != 2) &&                                                 /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                                (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                        {
                            T3 = Vgs_eff2- Vfbeff2 - Vbseff - Vgsteff2;
                            if (T3 < 0.0)
                            {
                                T1 = T0 + T3 / pParam->B4SOIk1ox;
                                T2 = CoxWLb2;
                            }
                            else
                            {
                                T1 = sqrt(T0 * T0 + T3);
                                T2 = CoxWLb2 * T0 / T1;
                            }
                            Qsub0 += CoxWLb2 * pParam->B4SOIk1ox * (T1 - T0);
                            dQsub02_dVrg = T2 * (dVgs_eff2_dVg - dVfbeff2_dVrg);
                            dQsub02_dVg = -T2;
                            dQsub02_dVd = -T2 * dVfbeff2_dVd;
                            dQsub02_dVb = -T2 * (dVfbeff2_dVb + 1);
                            if (selfheat) dQsub02_dT = -T2 * dVfbeff2_dT;
                            else  dQsub02_dT = 0.0;
                        }
                    }
                    /* v3.1 */



                    AbulkCV = Abulk0 * pParam->B4SOIabulkCVfactor;
                    dAbulkCV_dVb = pParam->B4SOIabulkCVfactor * dAbulk0_dVb;

                    VdsatCV = Vgsteff / AbulkCV;
//                    dVdsatCV_dVg = 1.0 / AbulkCV;
//                    dVdsatCV_dVb = -VdsatCV * dAbulkCV_dVb / AbulkCV;

                    V4 = VdsatCV - Vds - DELTA_4;
                    T0 = sqrt(V4 * V4 + 4.0 * DELTA_4 * VdsatCV);
                    VdseffCV = VdsatCV - 0.5 * (V4 + T0);
                    T1 = 0.5 * (1.0 + V4 / T0);
                    T2 = DELTA_4 / T0;
                    T3 = (1.0 - T1 - T2) / AbulkCV;
                    dVdseffCV_dVg = T3;
                    dVdseffCV_dVd = T1;
                    dVdseffCV_dVb = -T3 * VdsatCV * dAbulkCV_dVb;

                    /* v4.1 */
                    if (here->B4SOIagbcp2 > 0)
                    {
                        VdsatCV2 = Vgsteff2 / AbulkCV;
//                        dVdsatCV2_dVg = 1.0 / AbulkCV;
//                        dVdsatCV2_dVb = -VdsatCV2 * dAbulkCV_dVb / AbulkCV;
                        V4 = VdsatCV2 - Vds - DELTA_4;
                        T0 = sqrt(V4 * V4 + 4.0 * DELTA_4 * VdsatCV2);
                        VdseffCV2 = VdsatCV2 - 0.5 * (V4 + T0);
                        T1 = 0.5 * (1.0 + V4 / T0);
                        T2 = DELTA_4 / T0;
                        T3 = (1.0 - T1 - T2) / AbulkCV;
                        dVdseffCV2_dVg = T3;
                        dVdseffCV2_dVd = T1;
                        dVdseffCV2_dVb = -T3 * VdsatCV2 * dAbulkCV_dVb;
                    }
                    /* end v4.1 */

                    /* v3.1 */
                    if (here->B4SOIsoiMod == 2) /* v3.2 */ /* ideal FD */
                    {
                        qbulk = Cbg1 = Cbd1 = Cbb1 = 0;
                        Cbg12 = Cbd12 = Cbb12 = 0; /* v4.1 */
                    }
                    else
                    {
                        T0 = AbulkCV * VdseffCV;
                        T1 = 12.0 * (Vgsteff - 0.5 * T0 + 1e-20);
                        T2 = VdseffCV / T1;
                        T3 = T0 * T2;
                        T4 = (1.0 - 12.0 * T2 * T2 * AbulkCV);
                        T5 = (6.0 * T0 * (4.0 * Vgsteff- T0) / (T1 * T1) - 0.5);
                        T6 = 12.0 * T2 * T2 * Vgsteff;

                        T7 = 1.0 - AbulkCV;
                        qbulk = CoxWLb * T7 * (0.5 * VdseffCV - T3);
                        T4 = -T7 * (T4 - 1.0);
                        T5 = -T7 * T5;
                        T6 = -(T7 * T6 + (0.5 * VdseffCV - T3));

                        Cbg1 = CoxWLb * (T4 + T5 * dVdseffCV_dVg);
                        Cbd1 = CoxWLb * T5 * dVdseffCV_dVd ;
                        Cbb1 = CoxWLb * (T5 * dVdseffCV_dVb + T6 * dAbulkCV_dVb);
                        /* v4.1 */
                        if ((here->B4SOIsoiMod != 2) &&                                 /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                                (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                        {
                            T0 = AbulkCV * VdseffCV2;
                            T1 = 12.0 * (Vgsteff2 - 0.5 * T0 + 1e-20);
                            T2 = VdseffCV2 / T1;
                            T3 = T0 * T2;
                            T4 = (1.0 - 12.0 * T2 * T2 * AbulkCV);
                            T5 = (6.0 * T0 * (4.0 * Vgsteff2 - T0) / (T1 * T1) - 0.5);
                            T6 = 12.0 * T2 * T2 * Vgsteff2;
                            T7 = 1.0 - AbulkCV;
                            qbulk += CoxWLb2 * T7 * (0.5 * VdseffCV2 - T3);
                            T4 = -T7 * (T4 - 1.0);
                            T5 = -T7 * T5;
                            T6 = -(T7 * T6 + (0.5 * VdseffCV2 - T3));
                            Cbg12 = CoxWLb2 * (T4 + T5 * dVdseffCV2_dVg);
                            Cbd12 = CoxWLb2 * T5 * dVdseffCV2_dVd ;
                            Cbb12 = CoxWLb2 * (T5 * dVdseffCV2_dVb + T6 * dAbulkCV_dVb);
                        }
                        /* end  v4.1 */
                    }
                    /* v3.1 */



                    /* Total inversion charge */
                    T0 = AbulkCV * VdseffCV;
                    T1 = 12.0 * (Vgsteff - 0.5 * T0 + 1e-20);
                    /*                    T2 = VdseffCV / T1;
                    */
                    T2 = T0 / T1;
                    T3 = T0 * T2;

                    /*                    T4 = (1.0 - 12.0 * T2 * T2 * AbulkCV);
                                  T5 = (6.0 * T0 * (4.0 * Vgsteff - T0) / (T1 * T1) - 0.5);
                                  T6 = 12.0 * T2 * T2 * Vgsteff;
                                  */
                    T4 = (1.0 - 12.0 * T2 * T2);/*bug fix */
                    T7 = T2 * (2.0 + 6.0 * T2) - 0.5; /*bug fix */

                    T5 = T7 * AbulkCV;
                    T6 = T7 * VdseffCV;

                    /*                    qinv = CoxWL * (Vgsteff - 0.5 * VdseffCV + T3);
                    */
                    qinv = CoxWL * (Vgsteff - 0.5 * T0 + T3);

                    here->B4SOIqinv = -qinv; /* for noise v3.2 */

                    Cgg1 = CoxWL * (T4 + T5 * dVdseffCV_dVg);
                    Cgd1 = CoxWL * T5 * dVdseffCV_dVd;
                    Cgb1 = CoxWL * (T5 * dVdseffCV_dVb + T6 * dAbulkCV_dVb);
                    /* v4.1 */
                    if ((here->B4SOIsoiMod != 2) &&                             /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                            (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                    {
                        T02 = AbulkCV * VdseffCV2;
                        T12 = 12.0 * (Vgsteff2 - 0.5 * T02 + 1e-20);
                        T2 = T02 / T12;
                        T3 = T02 * T2;
                        T4 = (1.0 - 12.0 * T2 * T2);
                        T7 = T2 * (2.0 + 6.0 * T2) - 0.5;

                        T5 = T7 * AbulkCV;
                        T6 = T7 * VdseffCV2;

                        qinv += CoxWL2 * (Vgsteff2 - 0.5 * T02 + T3);
                        here->B4SOIqinv = -qinv;

                        Cgg12 = CoxWL2 * (T4 + T5 * dVdseffCV2_dVg);
                        Cgd12 = CoxWL2 * T5 * dVdseffCV2_dVd;
                        Cgb12 = CoxWL2 * (T5 * dVdseffCV2_dVb + T6 * dAbulkCV_dVb);
                    }
                    /* end v4.1 */
                    /* Inversion charge partitioning into S / D */
                    if (model->B4SOIxpart > 0.5)
                    {
                        /* 0/100 Charge partition model */
                        T1 = T1 + T1;
                        qsrc = -CoxWL * (0.5 * Vgsteff + 0.25 * T0
                                         - T0 * T0 / T1);
                        T7 = (4.0 * Vgsteff - T0) / (T1 * T1);
                        T4 = -(0.5 + 24.0 * T0 * T0 / (T1 * T1));
                        T5 = -(0.25 * AbulkCV - 12.0 * AbulkCV * T0 * T7);
                        T6 = -(0.25 * VdseffCV - 12.0 * T0 * VdseffCV * T7);
                        Csg1 = CoxWL * (T4 + T5 * dVdseffCV_dVg);
                        Csd1 = CoxWL * T5 * dVdseffCV_dVd;
                        Csb1 = CoxWL * (T5 * dVdseffCV_dVb + T6 * dAbulkCV_dVb);

                        /* v4.1 */
                        if ((here->B4SOIsoiMod != 2) &&                         /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                                (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                        {
                            T12 = T12 + T12;
                            qsrc += -CoxWL2 * (0.5 * Vgsteff2 + 0.25 * T02
                                               - T02 * T02 / T12);
                            T7 = (4.0 * Vgsteff2 - T02) / (T12 * T12);
                            T4 = -(0.5 + 24.0 * T02 * T02 / (T12 * T12));
                            T5 = -(0.25 * AbulkCV - 12.0 * AbulkCV * T02 * T7);
                            T6 = -(0.25 * VdseffCV2 - 12.0 * T02 * VdseffCV2 * T7);
                            Csg12 = CoxWL2 * (T4 + T5 * dVdseffCV2_dVg);
                            Csd12 = CoxWL2 * T5 * dVdseffCV2_dVd;
                            Csb12 = CoxWL2 * (T5 * dVdseffCV2_dVb + T6 * dAbulkCV_dVb);
                        }
                        /* end v4.1 */

                    }
                    else if (model->B4SOIxpart < 0.5)
                    {
                        /* 40/60 Charge partition model */
                        T1 = T1 / 12.0;
                        T2 = 0.5 * CoxWL / (T1 * T1);
                        T3 = Vgsteff * (2.0 * T0 * T0 / 3.0 + Vgsteff
                                        * (Vgsteff - 4.0 * T0 / 3.0))
                             - 2.0 * T0 * T0 * T0 / 15.0;
                        qsrc = -T2 * T3;
                        T7 = 4.0 / 3.0 * Vgsteff * (Vgsteff - T0)
                             + 0.4 * T0 * T0;
                        T4 = -2.0 * qsrc / T1 - T2 * (Vgsteff * (3.0
                                                      * Vgsteff - 8.0 * T0 / 3.0)
                                                      + 2.0 * T0 * T0 / 3.0);
                        T5 = (qsrc / T1 + T2 * T7) * AbulkCV;
                        T6 = (qsrc / T1 * VdseffCV + T2 * T7 * VdseffCV);
                        Csg1 = T4 + T5 * dVdseffCV_dVg;
                        Csd1 = T5 * dVdseffCV_dVd;
                        Csb1 = T5 * dVdseffCV_dVb + T6 * dAbulkCV_dVb;
                        /* v4.1 */
                        if ((here->B4SOIsoiMod != 2) &&                 /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                                (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 >0)
                        {
                            T12 = T12 /12.0;
                            T2 = 0.5 * CoxWL2 / (T12 * T12);
                            T3 = Vgsteff2 * (2.0 * T02 * T02 / 3.0 + Vgsteff2
                                             * (Vgsteff2 - 4.0 * T02 / 3.0))
                                 - 2.0 * T02 * T02 * T02 / 15.0;
                            qsrc2 = -T2 * T3;

                            T7 = 4.0 / 3.0 * Vgsteff2 * (Vgsteff2 - T02)
                                 + 0.4 * T02 * T02;
                            T4 = -2.0 * qsrc2 / T12 - T2 * (Vgsteff2 * (3.0
                                                            * Vgsteff2 - 8.0 * T02 / 3.0)
                                                            + 2.0 * T02 * T02 / 3.0);
                            T5 = (qsrc2 / T12 + T2 * T7) * AbulkCV;
                            T6 = (qsrc2 / T12 * VdseffCV2 + T2 * T7 * VdseffCV2);
                            Csg12 = T4 + T5 * dVdseffCV2_dVg;
                            Csd12 = T5 * dVdseffCV2_dVd;
                            Csb12 = T5 * dVdseffCV2_dVb + T6 * dAbulkCV_dVb;
                            qsrc += qsrc2;
                        }

                        /* end v4.1 */
                    }
                    else
                    {
                        /* 50/50 Charge partition model */
                        qsrc = - 0.5 * (qinv + qbulk);
                        Csg1 = - 0.5 * (Cgg1 + Cbg1);
                        Csb1 = - 0.5 * (Cgb1 + Cbb1);
                        Csd1 = - 0.5 * (Cgd1 + Cbd1);
                        /* v4.1 */
                        if ((here->B4SOIsoiMod != 2) &&                 /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                                (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 >0)
                        {
                            Csg12 = -0.5 * (Cgg12 + Cbg12);
                            Csb12 = -0.5 * (Cgb12 + Cbb12);
                            Csd12 = -0.5 * (Cgd12 + Cbd12);
                        }
                        /* end v4.1 */
                    }



                    /* Backgate charge */
                    /* v3.1 */
                    if (here->B4SOIsoiMod == 2) /* v3.2 */ /* ideal FD */
                    {
                        Qe1 = dQe1_dVb = dQe1_dVe = dQe1_dT = 0;
                    }
                    else /* soiMod = 0 or 1 */
                    {
                        CboxWL = pParam->B4SOIkb1 * model->B4SOIfbody * Cbox
                                 * (pParam->B4SOIweffCV / here->B4SOInseg
                                    * here->B4SOInf     /* bugfix_snps nf*/
                                    * pParam->B4SOIleffCVbg + here->B4SOIaebcp);
                        Qe1 = CboxWL * (Vesfb - Vbs);
                        dQe1_dVb = -CboxWL;
                        dQe1_dVe = CboxWL;
                        if (selfheat) dQe1_dT = -CboxWL * dvfbb_dT;
                        else dQe1_dT = 0;
                    }
                    /* v3.1 */


                    qgate = qinv + Qac0 + Qsub0;
                    qbody = (qbulk - Qac0 - Qsub0 - Qe1);
                    qsub = Qe1;
                    qdrn = -(qgate + qsrc + qbody + qsub);

                    /* This transform all the dependency on Vgsteff, Vbseff
                       into real ones */
                    Ce1b = dQe1_dVb;
                    Ce1e = dQe1_dVe;

                    Csg = Csg1 * dVgsteff_dVg;
                    Csd = Csd1 + Csg1 * dVgsteff_dVd;
                    Csb = Csg1 * dVgsteff_dVb + Csb1 * dVbseff_dVb;
                    if (selfheat) CsT = Csg1 * dVgsteff_dT;
                    else  CsT = 0.0;

                    Cgg = (Cgg1 + dQsub0_dVg) * dVgsteff_dVg
                          + dQac0_dVrg + dQsub0_dVrg;
                    Cgd = (Cgg1 + dQsub0_dVg) * dVgsteff_dVd + Cgd1
                          + dQac0_dVd + dQsub0_dVd;
                    Cgb = (Cgg1 + dQsub0_dVg) * dVgsteff_dVb
                          + (Cgb1 + dQsub0_dVb + dQac0_dVb) * dVbseff_dVb;
                    if (selfheat)
                        CgT = (Cgg1 + dQsub0_dVg) * dVgsteff_dT
                              + dQac0_dT + dQsub0_dT;
                    else  CgT = 0.0;

                    Cbg = (Cbg1 - dQsub0_dVg) * dVgsteff_dVg
                          - dQac0_dVrg - dQsub0_dVrg;
                    Cbd = (Cbg1 - dQsub0_dVg) * dVgsteff_dVd + Cbd1
                          - dQac0_dVd - dQsub0_dVd;
                    Cbb = (Cbg1 - dQsub0_dVg) * dVgsteff_dVb - dQe1_dVb
                          + (Cbb1 - dQsub0_dVb - dQac0_dVb) * dVbseff_dVb;
                    if (selfheat)
                        CbT = (Cbg1 - dQsub0_dVg) * dVgsteff_dT
                              - dQac0_dT - dQsub0_dT - dQe1_dT;
                    else CbT = 0.0;
                    /* v4.1 */
                    if ((here->B4SOIsoiMod != 2) &&                             /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                            (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 >0)
                    {
                        Csg += Csg12 * dVgsteff2_dVg;
                        Csd += Csd12 + Csg12 * dVgsteff2_dVd;
                        Csb += Csg12 * dVgsteff2_dVb + Csb12 * dVbseff_dVb;
                        if (selfheat) CsT += Csg12 * dVgsteff2_dT;

                        Cgg += (Cgg12 + dQsub02_dVg) * dVgsteff2_dVg
                               + dQac02_dVrg + dQsub02_dVrg;
                        Cgd += (Cgg12 + dQsub02_dVg) * dVgsteff2_dVd + Cgd12
                               + dQac02_dVd + dQsub02_dVd;
                        Cgb += (Cgg12 + dQsub02_dVg) * dVgsteff2_dVb
                               + (Cgb12 + dQsub02_dVb + dQac02_dVb) * dVbseff_dVb;
                        if (selfheat)
                            CgT += (Cgg12 + dQsub02_dVg) * dVgsteff2_dT
                                   + dQac02_dT + dQsub02_dT;

                        Cbg += (Cbg12 - dQsub02_dVg) * dVgsteff2_dVg
                               - dQac02_dVrg - dQsub02_dVrg;
                        Cbd += (Cbg12 - dQsub02_dVg) * dVgsteff2_dVd + Cbd12
                               - dQac02_dVd - dQsub02_dVd;
                        Cbb += (Cbg12 - dQsub02_dVg) * dVgsteff2_dVb
                               + (Cbb12 - dQsub02_dVb - dQac02_dVb) * dVbseff_dVb;
                        if (selfheat)
                            CbT += (Cbg12 - dQsub02_dVg) * dVgsteff2_dT
                                   - dQac02_dT - dQsub02_dT;
                    }
                    /* end v4.1 */

                    here->B4SOIcggb = Cgg ;
                    here->B4SOIcgsb = - (Cgg  + Cgd  + Cgb);
                    here->B4SOIcgdb = Cgd;
                    here->B4SOIcgT = CgT;

                    here->B4SOIcbgb = Cbg;
                    here->B4SOIcbsb = -(Cbg  + Cbd  + Cbb)
                                      + Ce1e;
                    here->B4SOIcbdb = Cbd;
                    here->B4SOIcbeb = - Ce1e ;
                    here->B4SOIcbT = CbT;

                    here->B4SOIceeb = Ce1e ;
                    here->B4SOIceT = dQe1_dT;

                    here->B4SOIcdgb = -(Cgg + Cbg + Csg);
                    here->B4SOIcddb = -(Cgd + Cbd + Csd);
                    here->B4SOIcdeb = 0;
                    here->B4SOIcdT = -(CgT + CbT + CsT) - dQe1_dT;
                    here->B4SOIcdsb = (Cgg + Cgd + Cgb
                                       + Cbg + Cbd + Cbb
                                       + Csg + Csd + Csb) + Ce1b;
                } /* End of if capMod == 2 */

                else if (model->B4SOIcapMod == 3)
                {

                    dVgsteff_dVb /= dVbseff_dVb;
                    if(model->B4SOImtrlMod == 0)
                        Cox = 3.453133e-11 / model->B4SOItoxp;
                    else
                        Cox = epsrox * EPS0 / model->B4SOItoxp;
                    CoxWL *= toxe/ model->B4SOItoxp;
                    CoxWLb *= model->B4SOItox/ model->B4SOItoxp;
                    Tox=1.0e8*model->B4SOItoxp;


                    /* v4.1 */
                    if (here->B4SOIagbcp2 > 0)
                    {
                        dVgsteff2_dVb /= dVbseff_dVb;
                        CoxWL2 *= model->B4SOItox /
                                  model->B4SOItoxp;
                        CoxWLb2 *= model->B4SOItox/
                                   model->B4SOItoxp;
                    }
                    /* end v4.1 */

                    /* v3.1 */
                    if (here->B4SOIsoiMod == 2) /* v3.2 */ /* ideal FD */
                    {
                        Qac0 = dQac0_dVg = dQac0_dVb = dQac0_dT = 0.0;
                        dQac02_dVg = dQac02_dVb = dQac02_dT = 0.0;
                        Qsub0 = dQsub0_dVg = dQsub0_dVd = dQsub0_dVb = dQsub0_dT = 0.0;
                        dQsub02_dVg = dQsub02_dVd = dQsub02_dVb = dQsub02_dT = 0.0;
                        Vfbzb = dVfbzb_dT = 0; /* v4.2 bug fix # 20 */
                    }
                    else /* soiMod = 0 or 1 */
                    {
                        if (selfheat)
                        {
                            Vfbzb = Vthzb - phi - pParam->B4SOIk1eff * sqrtPhi
                                    + pParam->B4SOIdelvt;
                            dVfbzb_dT = dVthzb_dT;
                        }
                        else
                        {
                            Vfbzb = here->B4SOIvfbzb + pParam->B4SOIdelvt;
                            dVfbzb_dT = 0;
                        }

                        V3 = Vfbzb - Vgs_eff + Vbseff - DELTA_3;
                        if (Vfbzb <= 0.0)
                        {
                            T0 = sqrt(V3 * V3 - 4.0 * DELTA_3 * Vfbzb);
                            T2 = -DELTA_3 / T0;
                        }
                        else
                        {
                            T0 = sqrt(V3 * V3 + 4.0 * DELTA_3 * Vfbzb);
                            T2 = DELTA_3 / T0;
                        }

                        T1 = 0.5 * (1.0 + V3 / T0);
                        Vfbeff = Vfbzb - 0.5 * (V3 + T0);
                        dVfbeff_dVg = T1 * dVgs_eff_dVg;
                        dVfbeff_dVb = -T1;
                        if (selfheat) dVfbeff_dT = (1.0 - T1 - T2) * dVfbzb_dT;
                        else  dVfbeff_dT = 0.0;
                        /* v4.1 */
                        if (here->B4SOIagbcp2 >0)
                        {
                            Vfbzb2 = Vfbzb + 1.12;
                            if (selfheat) dVfbzb2_dT = dVfbzb_dT;
                            else dVfbzb2_dT = 0;
                            V3 = Vfbzb2 - Vgs_eff2 + Vbseff - DELTA_3;
                            if (Vfbzb2 <= 0.0)                                                                    /* Bug fix #12 Jun 09 Vfbzb changed to Vfbzb2 */
                            {
                                T0 = sqrt(V3 * V3 - 100.0 * DELTA_3 * Vfbzb2);    /* Value of 100 instead of 4 is used to make transition smooth*/
                                T2 = -25.0 * DELTA_3 / T0;                                                               /* p+/p has same smoothness as n+/p with 100, 4 makes it too steep*/
                            }
                            else
                            {
                                T0 = sqrt(V3 * V3 + 100.0 * DELTA_3 * Vfbzb2);
                                T2 = 25.0 * DELTA_3 / T0;
                            }
                            T1 = 0.5 * (1.0 + V3 / T0);
                            Vfbeff2 = Vfbzb2 - 0.5 * (V3 + T0);
                            dVfbeff2_dVg = T1 * dVgs_eff2_dVg;
                            dVfbeff2_dVb = -T1;
                            if (selfheat) dVfbeff2_dT = (1.0 - T1 - T2) * dVfbzb2_dT;
                            else  dVfbeff2_dT = 0.0;
                        }
                        /* end v4.1 */

                        T0 = (Vgs_eff - Vbseff - Vfbzb) / Tox;
                        dT0_dVg = dVgs_eff_dVg / Tox;
                        dT0_dVb = -1.0 / Tox;

                        tmp = T0 * pParam->B4SOIacde;
                        if ((-EXPL_THRESHOLD < tmp) && (tmp < EXPL_THRESHOLD))
                        {
                            Tcen = pParam->B4SOIldeb * exp(tmp);
                            dTcen_dVg = pParam->B4SOIacde * Tcen;
                            dTcen_dVb = dTcen_dVg * dT0_dVb;
                            dTcen_dVg *= dT0_dVg;
                            if (selfheat)
                                dTcen_dT = -Tcen * pParam->B4SOIacde * dVfbzb_dT / Tox;
                            else dTcen_dT = 0;
                        }
                        else if (tmp <= -EXPL_THRESHOLD)
                        {
                            Tcen = pParam->B4SOIldeb * MIN_EXPL;
                            dTcen_dVg = dTcen_dVb = dTcen_dT = 0.0;
                        }
                        else
                        {
                            Tcen = pParam->B4SOIldeb * MAX_EXPL;
                            dTcen_dVg = dTcen_dVb = dTcen_dT = 0.0;
                        }

                        /*LINK = 1.0e-3 * (toxe - model->B4SOIdtoxcv);  v2.2.3 */
                        LINK = 1.0e-3 * model->B4SOItoxp;
                        V3 = pParam->B4SOIldeb - Tcen - LINK;
                        V4 = sqrt(V3 * V3 + 4.0 * LINK * pParam->B4SOIldeb);
                        Tcen = pParam->B4SOIldeb - 0.5 * (V3 + V4);
                        T1 = 0.5 * (1.0 + V3 / V4);
                        /* v4.1 small Tcen can introduce numerical issue  */
                        if (Tcen < 1e-15)
                        {
                            Tcen = 1e-15;
                            T1 = 0;
                        }  /* end */

                        dTcen_dVg *= T1;
                        dTcen_dVb *= T1;
                        if (selfheat)
                            dTcen_dT *= T1;
                        else dTcen_dT = 0;
                        /* v4.1 */
                        if (here->B4SOIagbcp2 > 0)
                        {
                            T0 = (Vgs_eff2 - Vbseff - Vfbzb2) / Tox;
                            dT0_dVg = dVgs_eff2_dVg / Tox;
                            dT0_dVb = -1.0 / Tox;

                            tmp = T0 * pParam->B4SOIacde;
                            if ((-EXPL_THRESHOLD < tmp) && (tmp < EXPL_THRESHOLD))
                            {
                                Tcen2 = pParam->B4SOIldeb * exp(tmp);
                                dTcen2_dVg = pParam->B4SOIacde * Tcen2;
                                dTcen2_dVb = dTcen2_dVg * dT0_dVb;
                                dTcen2_dVg *= dT0_dVg;
                                if (selfheat)
                                    dTcen2_dT = -Tcen2 * pParam->B4SOIacde * dVfbzb2_dT / Tox;
                                else dTcen2_dT = 0;
                            }
                            else if (tmp <= -EXPL_THRESHOLD)
                            {
                                Tcen2 = pParam->B4SOIldeb * MIN_EXPL;
                                dTcen2_dVg = dTcen2_dVb = dTcen2_dT = 0.0;
                            }
                            else
                            {
                                Tcen2 = pParam->B4SOIldeb * MAX_EXPL;
                                dTcen2_dVg = dTcen2_dVb = dTcen2_dT = 0.0;
                            }

                            V3 = pParam->B4SOIldeb - Tcen2 - LINK;
                            V4 = sqrt(V3 * V3 + 4.0 * LINK * pParam->B4SOIldeb);
                            Tcen2 = pParam->B4SOIldeb - 0.5 * (V3 + V4);
                            T1 = 0.5 * (1.0 + V3 / V4);

                            if (Tcen2 < 1e-15)
                            {
                                Tcen2 = 1e-15;
                                T1 = 0;
                            }
                            dTcen2_dVg *= T1;
                            dTcen2_dVb *= T1;
                            if (selfheat)
                                dTcen2_dT *= T1;
                            else dTcen2_dT = 0;
                        }
                        /* end v4.1 */

                        Ccen = epssub / Tcen;
                        T2 = Cox / (Cox + Ccen);
                        Coxeff = T2 * Ccen;
                        T3 = -Ccen / Tcen;
                        dCoxeff_dVg = T2 * T2 * T3;
                        dCoxeff_dVb = dCoxeff_dVg * dTcen_dVb;
                        dCoxeff_dVg *= dTcen_dVg;
                        if (selfheat)
                            dCoxeff_dT = T3 * dTcen_dT * (T2 - Coxeff / (Cox + Ccen));
                        else dCoxeff_dT = 0;
                        /* v4.1 */
                        if ((here->B4SOIsoiMod != 2) &&                         /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                                (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                        {
                            /* Ccen2 = EPSSI / Tcen2; */                                /* Bug Fix # 30 Jul09 EPSSI changed to epssub */
                            Ccen2 = epssub / Tcen2;
                            T2 = Cox / (Cox + Ccen2);
                            Coxeff2 = T2 * Ccen2;
                            T3 = -Ccen2 / Tcen2;
                            dCoxeff2_dVg = T2 * T2 * T3;
                            dCoxeff2_dVb = dCoxeff2_dVg * dTcen2_dVb;
                            dCoxeff2_dVg *= dTcen2_dVg;
                            if (selfheat)
                                dCoxeff2_dT = T3 * dTcen2_dT * (T2 - Coxeff2 / (Cox + Ccen2));
                            else dCoxeff2_dT = 0;
                        }
                        /* end v4.1 */
                        CoxWLcenb = CoxWLb * Coxeff / Cox;
                        if (selfheat)
                            dCoxWLcenb_dT = CoxWLb * dCoxeff_dT / Cox;
                        else dCoxWLcenb_dT = 0;
                        /* v4.1 */
                        if (here->B4SOIagbcp2 > 0)
                        {
                            CoxWLcenb2 = CoxWLb2 * Coxeff2 / Cox;
                            if (selfheat)
                                dCoxWLcenb2_dT = CoxWLb2 * dCoxeff2_dT / Cox;
                            else dCoxWLcenb2_dT = 0;
                        }
                        /* end v4.1 */
                        Qac0 = CoxWLcenb * (Vfbeff - Vfbzb);
                        QovCox = Qac0 / Coxeff;
                        dQac0_dVg = CoxWLcenb * dVfbeff_dVg
                                    + QovCox * dCoxeff_dVg;
                        dQac0_dVb = CoxWLcenb * dVfbeff_dVb
                                    + QovCox * dCoxeff_dVb;
                        if (selfheat) dQac0_dT = CoxWLcenb * (dVfbeff_dT - dVfbzb_dT)
                                                     + dCoxWLcenb_dT * (Vfbeff - Vfbzb);
                        else  dQac0_dT = 0.0;
                        /* v4.1 */
                        if ((here->B4SOIsoiMod != 2) &&                         /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                                (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                        {
                            Qac02 = CoxWLcenb2 * (Vfbeff2 - Vfbzb2);
                            QovCox2 = Qac02 / Coxeff2;
                            dQac02_dVg = CoxWLcenb2 * dVfbeff2_dVg
                                         + QovCox2 * dCoxeff2_dVg;
                            dQac02_dVb = CoxWLcenb2 * dVfbeff2_dVb
                                         + QovCox2 * dCoxeff2_dVb;
                            if (selfheat) dQac02_dT = CoxWLcenb2 * (dVfbeff2_dT - dVfbzb2_dT)
                                                          + dCoxWLcenb2_dT * (Vfbeff2 - Vfbzb2);
                            else  dQac02_dT = 0.0;

                            Qac0 += Qac02;
                        }
                        /* end v4.1 */

                        T0 = 0.5 * pParam->B4SOIk1ox;
                        T3 = Vgs_eff - Vfbeff - Vbseff - Vgsteff;
                        if (pParam->B4SOIk1ox == 0.0)
                        {
                            T1 = 0.0;
                            T2 = 0.0;
                        }
                        else if (T3 < 0.0)
                        {
                            T1 = T0 + T3 / pParam->B4SOIk1ox;
                            T2 = CoxWLcenb;
                        }
                        else
                        {
                            T1 = sqrt(T0 * T0 + T3);
                            T2 = CoxWLcenb * T0 / T1;
                        }

                        Qsub0 = CoxWLcenb * pParam->B4SOIk1ox * (T1 - T0);
                        QovCox = Qsub0 / Coxeff;
                        dQsub0_dVg = T2 * (dVgs_eff_dVg - dVfbeff_dVg - dVgsteff_dVg)
                                     + QovCox * dCoxeff_dVg;
                        dQsub0_dVd = -T2 * dVgsteff_dVd;
                        dQsub0_dVb = -T2 * (dVfbeff_dVb + 1 + dVgsteff_dVb)
                                     + QovCox * dCoxeff_dVb;
                        if (selfheat)
                            dQsub0_dT = -T2 * (dVfbeff_dT + dVgsteff_dT)
                                        + dCoxWLcenb_dT * pParam->B4SOIk1ox * (T1 - T0);
                        else  dQsub0_dT = 0.0;

                        /* v4.1 */
                        if ((here->B4SOIsoiMod != 2) &&                         /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                                (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                        {
                            T3 = Vgs_eff2 - Vfbeff2 - Vbseff - Vgsteff2;
                            if (pParam->B4SOIk1ox == 0.0)
                            {
                                T1 = 0.0;
                                T2 = 0.0;
                            }
                            else if (T3 < 0.0)
                            {
                                T1 = T0 + T3 / pParam->B4SOIk1ox;
                                T2 = CoxWLcenb2;
                            }
                            else
                            {
                                T1 = sqrt(T0 * T0 + T3);
                                T2 = CoxWLcenb2 * T0 / T1;
                            }

                            Qsub02 = CoxWLcenb2 * pParam->B4SOIk1ox * (T1 - T0);
                            QovCox2 = Qsub02 / Coxeff2;
                            dQsub02_dVg = T2 * (dVgs_eff2_dVg - dVfbeff2_dVg - dVgsteff2_dVg)
                                          + QovCox2 * dCoxeff2_dVg;
                            dQsub02_dVd = -T2 * dVgsteff2_dVd;
                            dQsub02_dVb = -T2 * (dVfbeff2_dVb + 1 + dVgsteff2_dVb)
                                          + QovCox2 * dCoxeff2_dVb;
                            if (selfheat)
                                dQsub02_dT = -T2 * (dVfbeff2_dT + dVgsteff2_dT)
                                             + dCoxWLcenb2_dT * pParam->B4SOIk1ox * (T1 - T0);
                            else  dQsub02_dT = 0.0;

                            Qsub0 += Qsub02;
                        }
                        /* end v4.1 */

                    }
                    /* v3.1 */


                    /* Gate-bias dependent delta Phis begins */
                    if (pParam->B4SOIk1ox <= 0.0)
                    {
                        Denomi = 0.25 * pParam->B4SOImoin * Vtm;
                        T0 = 0.5 * pParam->B4SOIsqrtPhi;
                    }
                    else
                    {
                        Denomi = pParam->B4SOImoin * Vtm
                                 * pParam->B4SOIk1ox * pParam->B4SOIk1ox;
                        T0 = pParam->B4SOIk1ox * pParam->B4SOIsqrtPhi;
                    }
                    T1 = 2.0 * T0 + Vgsteff;

                    DeltaPhi = Vtm * log(1.0 + T1 * Vgsteff / Denomi);
                    dDeltaPhi_dVg = 2.0 * Vtm * (T1 -T0) / (Denomi + T1 * Vgsteff);
//                    dDeltaPhi_dVd = dDeltaPhi_dVg * dVgsteff_dVd;
//                    dDeltaPhi_dVb = dDeltaPhi_dVg * dVgsteff_dVb;

                    /* v4.1 */
                    if (here->B4SOIagbcp2 > 0)
                    {
                        T1 = 2.0 * T0 + Vgsteff2;
                        DeltaPhi2 = Vtm * log(1.0 + T1 * Vgsteff2 / Denomi);
                        dDeltaPhi2_dVg = 2.0 * Vtm * (T1 -T0) / (Denomi + T1 * Vgsteff2);
//                        dDeltaPhi2_dVd = dDeltaPhi2_dVg * dVgsteff2_dVd;
//                        dDeltaPhi2_dVb = dDeltaPhi2_dVg * dVgsteff2_dVb;
                    }
                    /* end v4.1 */
                    /* End of delta Phis */


                    /* v3.1.1 bug fix for discontinuity */
                    T3 = 4.0 * (Vth - Vfbzb - phi);
                    T2 = sqrt(T3*T3 + 0.0001);
                    T5 = 0.5 * (1 + T3/T2);
                    T4 = 0.5 * (T3 + T2);

                    Tox += Tox;
                    T0 = (Vgsteff + T4) / Tox;
                    tmp = exp(0.7 * log(T0));
                    T1 = 1.0 + tmp;
                    T2 = 0.7 * tmp / (T0 * Tox);
                    Tcen = 1.9e-9 / T1;
                    dTcen_dVg = -Tcen * T2 / T1;
                    dTcen_dVd = dTcen_dVg * (T5 * 4.0 * dVth_dVd + dVgsteff_dVd);
                    dTcen_dVb = dTcen_dVg * (T5 * 4.0 * dVth_dVb + dVgsteff_dVb);
                    dTcen_dVg *= dVgsteff_dVg;
                    if (selfheat)
                        dTcen_dT = -Tcen * T2 / T1
                                   * (T5 * 4.0 * (dVth_dT - dVfbzb_dT) + dVgsteff_dT);
                    else dTcen_dT = 0;


                    Ccen = epssub / Tcen;
                    T0 = Cox / (Cox + Ccen);
                    Coxeff = T0 * Ccen;
                    T1 = -Ccen / Tcen;
                    dCoxeff_dVg = T0 * T0 * T1;
                    dCoxeff_dVd = dCoxeff_dVg * dTcen_dVd;
                    dCoxeff_dVb = dCoxeff_dVg * dTcen_dVb;
                    dCoxeff_dVg *= dTcen_dVg;
                    if (selfheat)
                        dCoxeff_dT = T1 * dTcen_dT * (T0 - Coxeff / (Cox + Ccen));
                    else dCoxeff_dT = 0;
                    CoxWLcen = CoxWL * Coxeff / Cox;
                    CoxWLcenb = CoxWLb * Coxeff / Cox;
                    /* v4.1 */
                    if ((here->B4SOIsoiMod != 2) &&                     /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                            (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                    {
                        T3 = 4.0 * (Vth + 1.12 - Vfbzb2 - phi);
                        T2 = sqrt(T3*T3 + 0.0001);
                        T5 = 0.5 * (1 + T3/T2);
                        T4 = 0.5 * (T3 + T2);
                        /* Tox += Tox; */
                        T0 = (Vgsteff2 + T4) / Tox;
                        tmp = exp(0.7 * log(T0));
                        T1 = 1.0 + tmp;
                        T2 = 0.7 * tmp / (T0 * Tox);
                        Tcen2 = 1.9e-9 / T1;
                        dTcen2_dVg = -Tcen2 * T2 / T1;
                        dTcen2_dVd = dTcen2_dVg * (T5 * 4.0 * dVth_dVd + dVgsteff2_dVd);
                        dTcen2_dVb = dTcen2_dVg * (T5 * 4.0 * dVth_dVb + dVgsteff2_dVb);
                        dTcen2_dVg *= dVgsteff2_dVg;
                        if (selfheat)
                            dTcen2_dT = -Tcen2 * T2 / T1
                                        * (T5 * 4.0 * (dVth_dT - dVfbzb2_dT) + dVgsteff2_dT);
                        else dTcen2_dT = 0;
                        /*Ccen2 = EPSSI / Tcen2;*//*Bug Fix # 30 Jul09*/
                        Ccen2 = epssub/ Tcen2;
                        T0 = Cox / (Cox + Ccen2);
                        Coxeff2 = T0 * Ccen2;
                        T1 = -Ccen2 / Tcen2;
                        dCoxeff2_dVg = T0 * T0 * T1;
                        dCoxeff2_dVd = dCoxeff2_dVg * dTcen2_dVd;
                        dCoxeff2_dVb = dCoxeff2_dVg * dTcen2_dVb;
                        dCoxeff2_dVg *= dTcen2_dVg;
                        if (selfheat)
                            dCoxeff2_dT = T1 * dTcen2_dT * (T0 - Coxeff2 / (Cox + Ccen2));
                        else dCoxeff2_dT = 0;
                        CoxWLcen2 = CoxWL2 * Coxeff2 / Cox;
                        CoxWLcenb2 = CoxWLb2 * Coxeff2 / Cox;
                    }
                    /* end v4.1 */

                    AbulkCV = Abulk0 * pParam->B4SOIabulkCVfactor;
                    dAbulkCV_dVb = pParam->B4SOIabulkCVfactor * dAbulk0_dVb;
                    VdsatCV = (Vgsteff - DeltaPhi) / AbulkCV;
                    V4 = VdsatCV - Vds - DELTA_4;
                    T0 = sqrt(V4 * V4 + 4.0 * DELTA_4 * VdsatCV);
                    VdseffCV = VdsatCV - 0.5 * (V4 + T0);
                    T1 = 0.5 * (1.0 + V4 / T0);
                    T2 = DELTA_4 / T0;
                    T3 = (1.0 - T1 - T2) / AbulkCV;
                    T4 = T3 * ( 1.0 - dDeltaPhi_dVg);
                    dVdseffCV_dVg = T4;
                    dVdseffCV_dVd = T1;
                    dVdseffCV_dVb = -T3 * VdsatCV * dAbulkCV_dVb;

                    T0 = AbulkCV * VdseffCV;
                    T1 = Vgsteff - DeltaPhi;
                    T2 = 12.0 * (T1 - 0.5 * T0 + 1.0e-20);
                    T3 = T0 / T2;
                    T4 = 1.0 - 12.0 * T3 * T3;
                    T5 = AbulkCV * (6.0 * T0 * (4.0 * T1 - T0) / (T2 * T2) - 0.5);
                    T6 = T5 * VdseffCV / AbulkCV;

                    qinv = qgate = qinoi = CoxWLcen * (T1 - T0 * (0.5 - T3));
                    QovCox = qgate / Coxeff;
                    Cgg1 = CoxWLcen * (T4 * (1.0 - dDeltaPhi_dVg)
                                       + T5 * dVdseffCV_dVg);
                    Cgd1 = CoxWLcen * T5 * dVdseffCV_dVd + Cgg1
                           * dVgsteff_dVd + QovCox * dCoxeff_dVd;
                    Cgb1 = CoxWLcen * (T5 * dVdseffCV_dVb + T6 * dAbulkCV_dVb)
                           + Cgg1 * dVgsteff_dVb + QovCox * dCoxeff_dVb;
                    Cgg1 = Cgg1 * dVgsteff_dVg + QovCox * dCoxeff_dVg;

                    /* v4.1 */
                    if ((here->B4SOIsoiMod != 2) &&                             /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                            (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                    {
                        VdsatCV2 = (Vgsteff2 - DeltaPhi2) / AbulkCV;
                        V4 = VdsatCV2 - Vds - DELTA_4;
                        T02 = sqrt(V4 * V4 + 4.0 * DELTA_4 * VdsatCV2);
                        VdseffCV2 = VdsatCV2 - 0.5 * (V4 + T02);
                        T12 = 0.5 * (1.0 + V4 / T02);
                        T22 = DELTA_4 / T02;
                        T3 = (1.0 - T12 - T22) / AbulkCV;
                        T4 = T3 * ( 1.0 - dDeltaPhi2_dVg);
                        dVdseffCV2_dVg = T4;
                        dVdseffCV2_dVd = T12;
                        dVdseffCV2_dVb = -T3 * VdsatCV2 * dAbulkCV_dVb;

                        T02 = AbulkCV * VdseffCV2;
                        T12 = Vgsteff2 - DeltaPhi2;
                        T22 = 12.0 * (T12 - 0.5 * T02 + 1.0e-20);
                        T3 = T02 / T22;
                        T4 = 1.0 - 12.0 * T3 * T3;
                        T52 = AbulkCV * (6.0 * T02 * (4.0 * T12 - T02) / (T22 * T22) - 0.5);
                        T6 = T52 * VdseffCV2 / AbulkCV;
                        T7 = CoxWLcen2 * (T12 - T02 * (0.5 - T3));
                        qinv += T7;
                        qgate = qinoi = qinv;
                        QovCox2 = T7 / Coxeff2;
                        Cgg12 = CoxWLcen2 * (T4 * (1.0 - dDeltaPhi2_dVg)
                                             + T52 * dVdseffCV2_dVg);
                        Cgd12 = CoxWLcen2 * T52 * dVdseffCV2_dVd + Cgg12
                                * dVgsteff2_dVd + QovCox2 * dCoxeff2_dVd;
                        Cgb12 = CoxWLcen2 * (T52 * dVdseffCV2_dVb + T6 * dAbulkCV_dVb)
                                + Cgg12 * dVgsteff2_dVb + QovCox2 * dCoxeff2_dVb;
                        Cgg12 = Cgg12 * dVgsteff2_dVg + QovCox2 * dCoxeff2_dVg;
                    }

                    /* end v4.1 */


                    /* v3.1 */
                    if (here->B4SOIsoiMod == 2) /* v3.2 */ /* ideal FD */
                    {
                        // SRW qbulk = Cbg1 = Cbd1 = Cbb1 = Cbg1 = Cbg12 = Cbb12 = Cbd12 = 0;
                        qbulk = Cbg1 = Cbd1 = Cbb1 = Cbg12 = Cbb12 = Cbd12 = 0;
                    }
                    else /* soiMod = 0 or 1 */
                    {
                        T7 = 1.0 - AbulkCV;
                        T8 = T2 * T2;
                        T9 = 12.0 * T7 * T0 * T0 / (T8 * AbulkCV);
                        T10 = T9 * (1.0 - dDeltaPhi_dVg);
                        T11 = -T7 * T5 / AbulkCV;
                        T12 = -(T9 * T1 / AbulkCV + VdseffCV * (0.5 - T0 / T2));

                        qbulk = CoxWLcenb * T7 * (0.5 * VdseffCV - T0 * VdseffCV / T2);
                        QovCox = qbulk / Coxeff;
                        Cbg1 = CoxWLcenb * (T10 + T11 * dVdseffCV_dVg);
                        Cbd1 = CoxWLcenb * T11 * dVdseffCV_dVd + Cbg1
                               * dVgsteff_dVd + QovCox * dCoxeff_dVd;
                        Cbb1 = CoxWLcenb * (T11 * dVdseffCV_dVb + T12 * dAbulkCV_dVb)
                               + Cbg1 * dVgsteff_dVb + QovCox * dCoxeff_dVb;
                        Cbg1 = Cbg1 * dVgsteff_dVg + QovCox * dCoxeff_dVg;
                        /* v4.1 */
                        if ((here->B4SOIsoiMod != 2) &&                                 /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                                (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                        {
                            T8 = T22 * T22;
                            T9 = 12.0 * T7 * T02 * T02 / (T8 * AbulkCV);
                            T10 = T9 * (1.0 - dDeltaPhi2_dVg);
                            T11 = -T7 * T52 / AbulkCV;
                            T12 = -(T9 * (Vgsteff2 - DeltaPhi2) / AbulkCV + VdseffCV2 * (0.5 - T02 / T22));

                            qbulk2 = CoxWLcenb2 * T7 * (0.5 * VdseffCV2 - T02 * VdseffCV2 / T22);
                            QovCox2 = qbulk2 / Coxeff2;
                            Cbg12 = CoxWLcenb2 * (T10 + T11 * dVdseffCV2_dVg);
                            Cbd12 = CoxWLcenb2 * T11 * dVdseffCV2_dVd + Cbg12
                                    * dVgsteff2_dVd + QovCox2 * dCoxeff2_dVd;
                            Cbb12 = CoxWLcenb2 * (T11 * dVdseffCV2_dVb + T12 * dAbulkCV_dVb)
                                    + Cbg12 * dVgsteff2_dVb + QovCox2 * dCoxeff2_dVb;
                            Cbg12 = Cbg12 * dVgsteff2_dVg + QovCox2 * dCoxeff2_dVg;
                            qbulk += qbulk2;
                        }

                        /* end v4.1 */

                    }
                    /* v3.1 */

                    Csg2 = Csd2 = Csb2 = 0.0;    /* CJB  LFW */
                    if (model->B4SOIxpart > 0.5)
                    {
                        /* 0/100 partition */
                        qsrc = -CoxWLcen * (T1 / 2.0 + T0 / 4.0
                                            - 0.5 * T0 * T0 / T2);
                        QovCox = qsrc / Coxeff;
                        T2 += T2;
                        T3 = T2 * T2;
                        T7 = -(0.25 - 12.0 * T0 * (4.0 * T1 - T0) / T3);
                        T4 = -(0.5 + 24.0 * T0 * T0 / T3) * (1.0 - dDeltaPhi_dVg);
                        T5 = T7 * AbulkCV;
                        T6 = T7 * VdseffCV;

                        Csg = CoxWLcen * (T4 + T5 * dVdseffCV_dVg);
                        Csd = CoxWLcen * T5 * dVdseffCV_dVd + Csg * dVgsteff_dVd
                              + QovCox * dCoxeff_dVd;
                        Csb = CoxWLcen * (T5 * dVdseffCV_dVb + T6 * dAbulkCV_dVb)
                              + Csg * dVgsteff_dVb + QovCox * dCoxeff_dVb;
                        Csg = Csg * dVgsteff_dVg + QovCox * dCoxeff_dVg;
                        /* v4.1 */
                        if ((here->B4SOIsoiMod != 2) &&                                 /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                                (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                        {
                            T12 = Vgsteff2 - DeltaPhi2; /* must restore for derivatives below*/
                            qsrc2 = -CoxWLcen2 * ( (Vgsteff2 - DeltaPhi2) / 2.0 + T02 / 4.0
                                                   - 0.5 * T02 * T02 / T22);    /* CJB  LFW */
                            QovCox2 = qsrc2 / Coxeff2;
                            T22 += T22;
                            T3 = T22 * T22;
                            T7 = -(0.25 - 12.0 * T02 * (4.0 * T12 - T02) / T3);
                            T4 = -(0.5 + 24.0 * T02 * T02 / T3) * (1.0 - dDeltaPhi2_dVg);
                            T5 = T7 * AbulkCV;
                            T6 = T7 * VdseffCV2;
                            Csg2 = CoxWLcen2 * (T4 + T5 * dVdseffCV2_dVg);
                            Csd2 = CoxWLcen2 * T5 * dVdseffCV2_dVd + Csg2 * dVgsteff2_dVd
                                   + QovCox2 * dCoxeff2_dVd;
                            Csb2 = CoxWLcen2 * (T5 * dVdseffCV2_dVb + T6 * dAbulkCV_dVb)
                                   + Csg2 * dVgsteff2_dVb + QovCox2 * dCoxeff2_dVb;
                            Csg2 = Csg2 * dVgsteff2_dVg + QovCox2 * dCoxeff2_dVg;
                            qsrc += qsrc2;
                        }
                        /* end v4.1 */

                    }
                    else if (model->B4SOIxpart < 0.5)
                    {
                        /* 40/60 partition */
                        T2 = T2 / 12.0;
                        T3 = 0.5 * CoxWLcen / (T2 * T2);
                        T4 = T1 * (2.0 * T0 * T0 / 3.0 + T1 * (T1 - 4.0
                                                               * T0 / 3.0)) - 2.0 * T0 * T0 * T0 / 15.0;
                        qsrc = -T3 * T4;
                        QovCox = qsrc / Coxeff;
                        T8 = 4.0 / 3.0 * T1 * (T1 - T0) + 0.4 * T0 * T0;
                        T5 = -2.0 * qsrc / T2 - T3 * (T1 * (3.0 * T1 - 8.0
                                                            * T0 / 3.0) + 2.0 * T0 * T0 / 3.0);
                        T6 = AbulkCV * (qsrc / T2 + T3 * T8);
                        T7 = T6 * VdseffCV / AbulkCV;

                        Csg = T5 * (1.0 - dDeltaPhi_dVg) + T6 * dVdseffCV_dVg;
                        Csd = Csg * dVgsteff_dVd + T6 * dVdseffCV_dVd
                              + QovCox * dCoxeff_dVd;
                        Csb = Csg * dVgsteff_dVb + T6 * dVdseffCV_dVb
                              + T7 * dAbulkCV_dVb + QovCox * dCoxeff_dVb;
                        Csg = Csg * dVgsteff_dVg + QovCox * dCoxeff_dVg;
                        /* v4.1 */
                        if ((here->B4SOIsoiMod != 2) &&                                 /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                                (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                        {
                            T12 = Vgsteff2 - DeltaPhi2; /* must restore for derivatives below*/
                            T22 = T22 / 12.0;
                            T3 = 0.5 * CoxWLcen2 / (T22 * T22);
                            T4 = T12 * (2.0 * T02 * T02 / 3.0 + T12 * (T12 - 4.0
                                        * T02 / 3.0)) - 2.0 * T02 * T02 * T02 / 15.0;
                            qsrc2 = -T3 * T4;
                            QovCox2 = qsrc2 / Coxeff2;
                            T8 = 4.0 / 3.0 * T12 * (T12 - T02) + 0.4 * T02 * T02;
                            T5 = -2.0 * qsrc2 / T22 - T3 * (T12 * (3.0 * T12 - 8.0
                                                                   * T02 / 3.0) + 2.0 * T02 * T02 / 3.0);
                            T6 = AbulkCV * (qsrc2 / T22 + T3 * T8);
                            T7 = T6 * VdseffCV2 / AbulkCV;

                            Csg2 = T5 * (1.0 - dDeltaPhi2_dVg) + T6 * dVdseffCV2_dVg;
                            Csd2 = Csg2 * dVgsteff2_dVd + T6 * dVdseffCV2_dVd
                                   + QovCox2 * dCoxeff2_dVd;
                            Csb2 = Csg2 * dVgsteff2_dVb + T6 * dVdseffCV2_dVb
                                   + T7 * dAbulkCV_dVb + QovCox2 * dCoxeff2_dVb;
                            Csg2 = Csg2 * dVgsteff2_dVg + QovCox2 * dCoxeff2_dVg;
                            qsrc += qsrc2;
                        }
                        /* end v4.1 */
                    }
                    else
                    {
                        /* 50/50 partition */
                        qsrc = -0.5 * qgate;
                        Csg = -0.5 * Cgg1;
                        Csd = -0.5 * Cgd1;
                        Csb = -0.5 * Cgb1;
                        /* v4.1 */
                        if ((here->B4SOIsoiMod != 2) &&                                         /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                                (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                        {
                            Csg2 = -0.5 * Cgg12;
                            Csd -= 0.5 * Cgd12;
                            Csb -= 0.5 * Cgb12;
                            Csd2 = 0.0;     /* CJB  LFW */
                            Csb2 = 0.0;     /* CJB  LFW */
                        }
                        /* end v4.1 */
                    }


                    /* Backgate charge */
                    /* v3.1 */
                    if (here->B4SOIsoiMod == 2) /* v3.2 */ /* ideal FD */
                    {
                        Qe1 = Ce1b = Ce1e = Ce1T = dQe1_dT = 0;
                    }
                    else /* soiMod = 0 or 1 */
                    {
                        CboxWL = pParam->B4SOIkb1 * model->B4SOIfbody * Cbox
                                 * (pParam->B4SOIweffCV / here->B4SOInseg
                                    * here->B4SOInf  /* bugfix_snps nf*/
                                    * pParam->B4SOIleffCVbg + here->B4SOIaebcp);
                        Qe1 = CboxWL * (Vesfb - Vbs);
                        Ce1b = dQe1_dVb = -CboxWL;
                        Ce1e = dQe1_dVe = CboxWL;
                        if (selfheat) Ce1T = dQe1_dT = -CboxWL * dvfbb_dT;
                        else Ce1T = dQe1_dT = 0.0;
                    }
                    /* v3.1 */


                    qgate += Qac0 + Qsub0 - qbulk;
                    qbody = qbulk - Qac0 - Qsub0 - Qe1;
                    qsub = Qe1;
                    qdrn = -(qgate + qbody + qsub + qsrc);

                    Cbg = Cbg1 - dQac0_dVg - dQsub0_dVg;
                    Cbd = Cbd1 - dQsub0_dVd;
                    Cbb = Cbb1 - dQac0_dVb - dQsub0_dVb - Ce1b / dVbseff_dVb;
                    if (selfheat)
                        CbT = Cbg1 * dVgsteff_dT - dQac0_dT
                              - dQsub0_dT - dQe1_dT;
                    else CbT = 0.0;

                    Cgg = Cgg1 - Cbg;
                    Cgd = Cgd1 - Cbd;
                    Cgb = Cgb1 - Cbb - Ce1b / dVbseff_dVb;
                    if (selfheat)
                        CgT = Cgg1 * dVgsteff_dT + dQac0_dT
                              + dQsub0_dT;
                    else  CgT = 0.0;

                    Cgb *= dVbseff_dVb;
                    Cbb *= dVbseff_dVb;
                    Csb *= dVbseff_dVb;
                    if (selfheat) CsT = Csg * dVgsteff_dT;
                    else  CsT = 0.0;
                    /* v4.1 */
                    if ((here->B4SOIsoiMod != 2) &&                             /* Bug fix #10 Jun 09 'opposite type Q/C evaluated only if bodymod=1' */
                            (here->B4SOIbodyMod != 0) && here->B4SOIagbcp2 > 0)
                    {
                        Cbg += Cbg12 - dQac02_dVg - dQsub02_dVg;
                        Cbd += Cbd12 - dQsub02_dVd;
                        Cbb += (Cbb12 - dQac02_dVb - dQsub02_dVb)*dVbseff_dVb;
                        if (selfheat)
                            CbT += Cbg12 * dVgsteff2_dT - dQac02_dT
                                   - dQsub02_dT;
                        else CbT = 0.0;
                        Cgg = Cgg1 + Cgg12 - Cbg;
                        Cgd = Cgd1 + Cgd12 - Cbd;
                        Cgb = Cgb1 + Cgb12 - Cbb - Ce1b / dVbseff_dVb;
                        if (selfheat)
                            CgT += Cgg12 * dVgsteff2_dT + dQac02_dT
                                   + dQsub02_dT;
                        else  CgT = 0.0;

                        Cgb *= dVbseff_dVb;
                        /*  Cbb *= dVbseff_dVb; */
                        if (selfheat) CsT += Csg2 * dVgsteff2_dT;
                        else  CsT = 0.0;
                        Csg += Csg2;
                        Csd += Csd2;  /* CJB  LFW */
                        Csb += Csb2;  /* CJB  LFW */
                    }

                    /* end v4.1 */
                    here->B4SOIcggb = Cgg;
                    here->B4SOIcgsb = -(Cgg + Cgd + Cgb);
                    here->B4SOIcgdb = Cgd;
                    here->B4SOIcgT  = CgT;

                    here->B4SOIcbgb = Cbg;
                    here->B4SOIcbsb = -(Cbg + Cbd + Cbb)
                                      + Ce1e;
                    here->B4SOIcbdb = Cbd;
                    here->B4SOIcbeb = -Ce1e;
                    here->B4SOIcbT  = CbT;

                    here->B4SOIceT = Ce1T;
                    here->B4SOIceeb = Ce1e ;

                    here->B4SOIcdgb = -(Cgg + Cbg + Csg);
                    here->B4SOIcddb = -(Cgd + Cbd + Csd);
                    here->B4SOIcdeb = 0;
                    here->B4SOIcdT   = -(CgT+CbT+CsT) - Ce1T;
                    here->B4SOIcdsb = (Cgg + Cgd + Cgb + Cbg + Cbd + Cbb
                                       + Csg + Csd + Csb) + Ce1b;
                    here->B4SOIqinv = -qinoi;

                } /* End of if capMod ==3 */
                else   /* v4.0 */
                {
                    Qsub0 = Qac0 = 0.0;
                    qgate = qdrn = qsrc = qbody = qsub = 0.0;
                    Cbg = Cbd = Cbb = 0.0;
                    here->B4SOIcggb = here->B4SOIcgsb
                                      = here->B4SOIcgdb = 0.0;
                    here->B4SOIcdgb = here->B4SOIcdsb
                                      = here->B4SOIcddb = 0.0;
                    here->B4SOIcbgb = here->B4SOIcbsb
                                      = here->B4SOIcbdb = 0.0;
                }
            }
            here->B4SOIqgate = qgate;
            here->B4SOIqdrn = qdrn;
            here->B4SOIqbulk = qbody;
            here->B4SOIqsrc = qsrc;



finished: /* returning Values to Calling Routine */
            /*
             *  COMPUTE EQUIVALENT DRAIN CURRENT SOURCE
             */
            if (ChargeComputationNeeded)
            {
                /* Intrinsic S/D junction charge */

                /* v3.1 */
                if (here->B4SOIsoiMod == 2) /* v3.2 */ /* ideal FD */
                {
                    qjs = qjd = 0.0;
                    /*gcjdds = gcjdbs = gcjdT = 0.0; v4.2 */
                    gcjdbs = gcjdT = 0.0;
                    gcjsbs = gcjsT = 0.0;
                    here->B4SOIcjsb = here->B4SOIcjdb = 0.0 /*v4.0*/;

                }
                else /* soiMod = 0 or 1 */
                {
                    PhiBSWG = model->B4SOIGatesidewallJctSPotential;
                    dPhiBSWG_dT = -model->B4SOItpbswg;
                    PhiBSWG += dPhiBSWG_dT * (Temp - model->B4SOItnom);
                    MJSWG = model->B4SOIbodyJctGateSideSGradingCoeff;

                    cjsbs = model->B4SOIunitLengthGateSidewallJctCapS
                            * pParam->B4SOIwdiosCV * model->B4SOItsi * here->B4SOInf / 1e-7; /* bugfix_snps nf*/
                    dcjsbs_dT = cjsbs * model->B4SOItcjswg;
                    cjsbs += dcjsbs_dT * (Temp - model->B4SOItnom);

                    cjdbs = model->B4SOIunitLengthGateSidewallJctCapD
                            * pParam->B4SOIwdiodCV * model->B4SOItsi * here->B4SOInf / 1e-7; /* bugfix_snps nf*/
                    dcjdbs_dT = cjdbs * model->B4SOItcjswgd;
                    cjdbs += dcjdbs_dT * (Temp - model->B4SOItnom);

                    DioMax = 0.9 * (PhiBSWG);

                    /* arg = 1.0 - (Vbs > DioMax ? DioMax : Vbs) / PhiBSWG; */                  /* Bug fix #6 Vbs evaluated taking consideration of Rbody Mode*/
                    if (here->B4SOIrbodyMod)
                        arg = 1.0 - (vsbs > DioMax ? DioMax : vsbs) / PhiBSWG;          /* Bug fix #6 */
                    else
                        arg = 1.0 - (vbs > DioMax ? DioMax : vbs) / PhiBSWG;            /* Bug fix #6 */
                    if (selfheat)
                        darg_dT = (1 - arg) / PhiBSWG * dPhiBSWG_dT;

                    if (MJSWG == 0.5)
                    {
                        dT3_dVb = 1.0 / sqrt(arg);

                        if (selfheat) ddT3_dVb_dT = -0.5 * dT3_dVb / arg * darg_dT;
                    }
                    else
                    {
                        dT3_dVb = exp(-MJSWG * log(arg));

                        if (selfheat) ddT3_dVb_dT = -MJSWG * dT3_dVb / arg * darg_dT;
                    }
                    T3 = (1.0 - arg * dT3_dVb) * PhiBSWG / (1.0 - MJSWG);

                    if (selfheat)
                        dT3_dT = (1.0 - arg * dT3_dVb) * dPhiBSWG_dT / (1.0 - MJSWG)
                                 - (arg * ddT3_dVb_dT + darg_dT * dT3_dVb) * PhiBSWG / (1.0 - MJSWG);

                    /* if (vbs > DioMax)
                       T3 += dT3_dVb * (vbs - DioMax); */                               /* Bug fix #6 Vbs evaluated taking consideration of Rbody Mode*/

                    if (here->B4SOIrbodyMod)
                    {
                        if (vsbs > DioMax)                                                                      /* Bug fix #6 */
                            T3 += dT3_dVb * (vsbs - DioMax);
                    }
                    else
                    {
                        if (vbs > DioMax)                                                                       /* Bug fix #6 */
                            T3 += dT3_dVb * (vbs - DioMax);
                    }

                    if (here->B4SOImode > 0)
                    {
                        qjs = cjsbs * T3 + model->B4SOItt * Ibsdif * here->B4SOInf;
                        gcjsbs = cjsbs * dT3_dVb + model->B4SOItt * dIbsdif_dVb * here->B4SOInf;
                    }
                    else
                    {
                        qjs = cjsbs * T3 + model->B4SOItt * Ibddif * here->B4SOInf;
                        gcjsbs = cjsbs * dT3_dVb + model->B4SOItt * dIbddif_dVb * here->B4SOInf;
                    }

                    if (selfheat)
                        gcjsT = model->B4SOItt * dIbsdif_dT * here->B4SOInf + dcjsbs_dT * T3 + dT3_dT * cjsbs;
                    else  gcjsT = 0.0;

                    PhiBSWG = model->B4SOIGatesidewallJctDPotential;
                    dPhiBSWG_dT = -model->B4SOItpbswgd;
                    PhiBSWG += dPhiBSWG_dT * (Temp - model->B4SOItnom);
                    MJSWG = model->B4SOIbodyJctGateSideDGradingCoeff;

                    DioMax = 0.9 * (PhiBSWG);
                    /* arg = 1.0 - (vbd > DioMax ? DioMax : vbd) / PhiBSWG; */  /* Bug fix #6 Vbd evaluated taking consideration of Rbody Mode*/
                    if (here->B4SOIrbodyMod)
                        arg = 1.0 - (vdbd > DioMax ? DioMax : vdbd) / PhiBSWG;       /* Bug Fix #6 */
                    else
                        arg = 1.0 - (vbd > DioMax ? DioMax : vbd) / PhiBSWG;       /* Bug Fix #6 */

                    if (selfheat)
                        darg_dT = (1 - arg) / PhiBSWG * dPhiBSWG_dT;

                    if (MJSWG == 0.5)
                    {
                        dT3_dVb = 1.0 / sqrt(arg);

                        if (selfheat) ddT3_dVb_dT = -0.5 * dT3_dVb / arg * darg_dT;
                    }
                    else
                    {
                        dT3_dVb = exp(-MJSWG * log(arg));

                        if (selfheat) ddT3_dVb_dT = -MJSWG * dT3_dVb / arg * darg_dT;
                    }
                    T3 = (1.0 - arg * dT3_dVb) * PhiBSWG / (1.0 - MJSWG);

                    if (selfheat)
                        dT3_dT = (1.0 - arg * dT3_dVb) * dPhiBSWG_dT / (1.0 - MJSWG)
                                 - (arg * ddT3_dVb_dT + darg_dT * dT3_dVb) * PhiBSWG / (1.0 - MJSWG);

                    /* if (vbd > DioMax)
                       T3 += dT3_dVb * (vbd - DioMax); */                               /* Bug fix #6 Vbd evaluated taking consideration of Rbody Mode*/
                    if (here->B4SOIrbodyMod)
                    {
                        if (vdbd > DioMax)                                                              /* Bug fix #6 */
                            T3 += dT3_dVb * (vdbd - DioMax);
                    }
                    else
                    {
                        if (vbd > DioMax)                                                               /* Bug fix #6 */
                            T3 += dT3_dVb * (vbd - DioMax);
                    }
                    dT3_dVd = -dT3_dVb;

                    if (here->B4SOImode > 0)
                    {
                        qjd = cjdbs * T3 + model->B4SOItt * Ibddif * here->B4SOInf;
                        gcjdbs = cjdbs * dT3_dVb + model->B4SOItt * dIbddif_dVb * here->B4SOInf;
                    }
                    else
                    {
                        qjd = cjdbs * T3 + model->B4SOItt * Ibsdif * here->B4SOInf;
                        gcjdbs = cjdbs * dT3_dVb + model->B4SOItt * dIbsdif_dVb * here->B4SOInf;
                    }
                    /*gcjdds = cjdbs * dT3_dVd + model->B4SOItt * dIbddif_dVd; v4.2 */
                    if (selfheat)
                        gcjdT = model->B4SOItt * dIbddif_dT * here->B4SOInf + dcjdbs_dT * T3 + dT3_dT * cjdbs;
                    else  gcjdT = 0.0;
                }
                /* v3.1 */

                /* v4.0 */
                /*                    qdrn -= qjd;
                                      qbody += (qjs + qjd);
                                      qsrc = -(qgate + qbody + qdrn + qsub);
                                      */

                /* Update the conductance */
                /* v4.2 bugfix: qjs/qjd computed using unswapped voltages; however, total capacitances are swapped below
                   note that gcjdds = -gcjdbs always, so (gcjdds + gcjdbs) == 0
                   here->B4SOIcddb -= gcjdds;
                   here->B4SOIcdT -= gcjdT;
                   here->B4SOIcdsb += gcjdds + gcjdbs;


                   here->B4SOIcbdb += (gcjdds);
                   here->B4SOIcbT += (gcjdT + gcjsT);
                   here->B4SOIcbsb -= (gcjdds + gcjdbs + gcjsbs);

                   here->B4SOIcjsb = (gcjdds + gcjdbs + gcjsbs);
                   here->B4SOIcjdb = -gcjdds;
                   */
                here->B4SOIcbT += (gcjdT + gcjsT);
                if (here->B4SOImode > 0)
                {
                    here->B4SOIcddb += gcjdbs;
                    here->B4SOIcdT -= gcjdT;

                    here->B4SOIcbdb -= (gcjdbs);
                    here->B4SOIcbsb -= (gcjsbs);

                    here->B4SOIcjsb = gcjsbs;
                    here->B4SOIcjdb = gcjdbs;
                }
                else
                {
                    here->B4SOIcddb += gcjsbs;
                    here->B4SOIcdT -= gcjsT;

                    here->B4SOIcbdb -= (gcjsbs);
                    here->B4SOIcbsb -= (gcjdbs);

                    here->B4SOIcjsb = gcjdbs;
                    here->B4SOIcjdb = gcjsbs;
                }

                /* Extrinsic Bottom S/D to substrate charge */
                T10 = -model->B4SOItype * ves;
                /* T10 is vse without type conversion */
                T11 = model->B4SOItype * (vds - ves);
                /* T11 is vde without type conversion */

                if (model->B4SOIcsdmin != 0.0)
                {
                    if ( ((pParam->B4SOInsub > 0) && (model->B4SOItype > 0)) ||
                            ((pParam->B4SOInsub < 0) && (model->B4SOItype < 0)) )
                    {
                        if (T10 < pParam->B4SOIvsdfb)
                        {
                            here->B4SOIqse = here->B4SOIcsbox * (T10 - pParam->B4SOIvsdfb);
                            here->B4SOIgcse = here->B4SOIcsbox;
                        }
                        else if (T10 < pParam->B4SOIsdt1)
                        {
                            T0 = T10 - pParam->B4SOIvsdfb;
                            T1 = T0 * T0;
                            here->B4SOIqse = T0 * (here->B4SOIcsbox -
                                                   pParam->B4SOIst2 / 3 * T1) ;
                            here->B4SOIgcse = here->B4SOIcsbox - pParam->B4SOIst2 * T1;
                        }
                        else if (T10 < pParam->B4SOIvsdth)
                        {
                            T0 = T10 - pParam->B4SOIvsdth;
                            T1 = T0 * T0;
                            here->B4SOIqse = here->B4SOIcsmin * T10 + here->B4SOIst4 +
                                             pParam->B4SOIst3 / 3 * T0 * T1;
                            here->B4SOIgcse = here->B4SOIcsmin + pParam->B4SOIst3 * T1;
                        }
                        else
                        {
                            here->B4SOIqse = here->B4SOIcsmin * T10 + here->B4SOIst4;
                            here->B4SOIgcse = here->B4SOIcsmin;
                        }
                    }
                    else
                    {
                        if (T10 < pParam->B4SOIvsdth)
                        {
                            here->B4SOIqse = here->B4SOIcsmin * (T10 - pParam->B4SOIvsdth);
                            here->B4SOIgcse = here->B4SOIcsmin;
                        }
                        else if (T10 < pParam->B4SOIsdt1)
                        {
                            T0 = T10 - pParam->B4SOIvsdth;
                            T1 = T0 * T0;
                            here->B4SOIqse = T0 * (here->B4SOIcsmin - pParam->B4SOIst2 / 3 * T1) ;
                            here->B4SOIgcse = here->B4SOIcsmin - pParam->B4SOIst2 * T1;
                        }
                        else if (T10 < pParam->B4SOIvsdfb)
                        {
                            T0 = T10 - pParam->B4SOIvsdfb;
                            T1 = T0 * T0;
                            here->B4SOIqse = here->B4SOIcsbox * T10 + here->B4SOIst4 +
                                             pParam->B4SOIst3 / 3 * T0 * T1;
                            here->B4SOIgcse = here->B4SOIcsbox + pParam->B4SOIst3 * T1;
                        }
                        else
                        {
                            here->B4SOIqse = here->B4SOIcsbox * T10 + here->B4SOIst4;
                            here->B4SOIgcse = here->B4SOIcsbox;
                        }
                    }

                    if ( ((pParam->B4SOInsub > 0) && (model->B4SOItype > 0)) ||
                            ((pParam->B4SOInsub < 0) && (model->B4SOItype < 0)) )
                    {
                        if (T11 < pParam->B4SOIvsdfb)
                        {
                            here->B4SOIqde = here->B4SOIcdbox * (T11 - pParam->B4SOIvsdfb);
                            here->B4SOIgcde = here->B4SOIcdbox;
                        }
                        else if (T11 < pParam->B4SOIsdt1)
                        {
                            T0 = T11 - pParam->B4SOIvsdfb;
                            T1 = T0 * T0;
                            here->B4SOIqde = T0 * (here->B4SOIcdbox - pParam->B4SOIdt2 / 3 * T1) ;
                            here->B4SOIgcde = here->B4SOIcdbox - pParam->B4SOIdt2 * T1;
                        }
                        else if (T11 < pParam->B4SOIvsdth)
                        {
                            T0 = T11 - pParam->B4SOIvsdth;
                            T1 = T0 * T0;
                            here->B4SOIqde = here->B4SOIcdmin * T11 + here->B4SOIdt4 +
                                             pParam->B4SOIdt3 / 3 * T0 * T1;
                            here->B4SOIgcde = here->B4SOIcdmin + pParam->B4SOIdt3 * T1;
                        }
                        else
                        {
                            here->B4SOIqde = here->B4SOIcdmin * T11 + here->B4SOIdt4;
                            here->B4SOIgcde = here->B4SOIcdmin;
                        }
                    }
                    else
                    {
                        if (T11 < pParam->B4SOIvsdth)
                        {
                            here->B4SOIqde = here->B4SOIcdmin * (T11 - pParam->B4SOIvsdth);
                            here->B4SOIgcde = here->B4SOIcdmin;
                        }
                        else if (T11 < pParam->B4SOIsdt1)
                        {
                            T0 = T11 - pParam->B4SOIvsdth;
                            T1 = T0 * T0;
                            here->B4SOIqde = T0 * (here->B4SOIcdmin - pParam->B4SOIdt2 / 3 * T1) ;
                            here->B4SOIgcde = here->B4SOIcdmin - pParam->B4SOIdt2 * T1;
                        }
                        else if (T11 < pParam->B4SOIvsdfb)
                        {
                            T0 = T11 - pParam->B4SOIvsdfb;
                            T1 = T0 * T0;
                            here->B4SOIqde = here->B4SOIcdbox * T11 + here->B4SOIdt4 +
                                             pParam->B4SOIdt3 / 3 * T0 * T1;
                            here->B4SOIgcde = here->B4SOIcdbox + pParam->B4SOIdt3 * T1;
                        }
                        else
                        {
                            here->B4SOIqde = here->B4SOIcdbox * T11 + here->B4SOIdt4;
                            here->B4SOIgcde = here->B4SOIcdbox;
                        }
                    }
                }
                else
                {
                    here->B4SOIqse = here->B4SOIcsbox * T10;
                    here->B4SOIgcse = here->B4SOIcsbox;
                    here->B4SOIqde = here->B4SOIcdbox * T11;
                    here->B4SOIgcde = here->B4SOIcdbox;
                }

                /* Extrinsic : Sidewall fringing S/D charge */
                here->B4SOIqse += here->B4SOIcsesw * T10;
                here->B4SOIgcse += here->B4SOIcsesw;
                here->B4SOIqde += here->B4SOIcdesw * T11;
                here->B4SOIgcde += here->B4SOIcdesw;

                /* All charge are multiplied with type at the end, but qse and qde
                   have true polarity => so pre-multiplied with type */
                here->B4SOIqse *= model->B4SOItype;
                here->B4SOIqde *= model->B4SOItype;
            }
            else   /* v4.0 */
            {
                qjs = qjd = 0.0;
                here->B4SOIqse = here->B4SOIqde = 0.0;
                here->B4SOIgcse = here->B4SOIgcde = 0.0;
            }

            here->B4SOIcbb = Cbb;
            here->B4SOIcbd = Cbd;
            here->B4SOIcbg = Cbg;
            here->B4SOIqbf = -Qsub0 - Qac0;
            here->B4SOIqjs = qjs;
            here->B4SOIqjd = qjd;
            *(ckt->CKTstate0 + here->B4SOIqbs) = qjs; /* v4.0 */
            *(ckt->CKTstate0 + here->B4SOIqbd) = qjd; /* v4.0 */

            /*
             *  check convergence
             */
            if ((here->B4SOIoff == 0) || (!(ckt->CKTmode & MODEINITFIX)))
            {
                if (Check == 1)
                {
                    ckt->incNoncon();  // SRW
#ifndef NEWCONV
                }
                else
                {
                    double
                    tol = ckt->CKTreltol * MAX(FABS(cdhat), FABS(here->B4SOIcd))
                          + ckt->CKTabstol;
                    if (FABS(cdhat - here->B4SOIcd) >= tol)
                    {
                        ckt->incNoncon();  // SRW
                    }
                    else
                    {
                        tol = ckt->CKTreltol * MAX(FABS(cbhat),
                                                   FABS(here->B4SOIcbs + here->B4SOIcbd))
                              + ckt->CKTabstol;
                        if (FABS(cbhat - (here->B4SOIcbs + here->B4SOIcbd))
                                > tol)
                        {
                            ckt->incNoncon();  // SRW
                        }
                    }
#endif /* NEWCONV */
                }
            }

            *(ckt->CKTstate0 + here->B4SOIvg) = vg;
            *(ckt->CKTstate0 + here->B4SOIvd) = vd;
            *(ckt->CKTstate0 + here->B4SOIvs) = vs;
            *(ckt->CKTstate0 + here->B4SOIvp) = vp;
            *(ckt->CKTstate0 + here->B4SOIve) = ve;

            *(ckt->CKTstate0 + here->B4SOIvbs) = vbs;
            *(ckt->CKTstate0 + here->B4SOIvbd) = vbd;
            *(ckt->CKTstate0 + here->B4SOIvgs) = vgs;
            *(ckt->CKTstate0 + here->B4SOIvds) = vds;
            *(ckt->CKTstate0 + here->B4SOIves) = ves;
            *(ckt->CKTstate0 + here->B4SOIvps) = vps;
            *(ckt->CKTstate0 + here->B4SOIdeltemp) = delTemp;

            /* v3.1 added for RF */
            *(ckt->CKTstate0 + here->B4SOIvgge) = vgge;
            *(ckt->CKTstate0 + here->B4SOIvggm) = vggm;
            *(ckt->CKTstate0 + here->B4SOIvges) = vges;
            *(ckt->CKTstate0 + here->B4SOIvgms) = vgms;
            /* v3.1 added for RF end*/
            *(ckt->CKTstate0 + here->B4SOIvdbs) = vdbs; /* v4.0 */
            *(ckt->CKTstate0 + here->B4SOIvdbd) = vdbd; /* v4.0 */
            *(ckt->CKTstate0 + here->B4SOIvsbs) = vsbs; /* v4.0 */
            *(ckt->CKTstate0 + here->B4SOIvses) = vses;
            *(ckt->CKTstate0 + here->B4SOIvdes) = vdes;

            /* bulk and channel charge plus overlaps */

            if (!ChargeComputationNeeded)
                goto line850;

#ifndef NOBYPASS
line755:
#endif
            ag0 = ckt->CKTag[0];

            T0 = vgd + DELTA_1;
            if (here->B4SOIrgateMod == 3) T0 = vgmd + DELTA_1; /* v3.2 bug fix */
            T1 = sqrt(T0 * T0 + 4.0 * DELTA_1);
            T2 = 0.5 * (T0 - T1);

            /* v2.2.3 bug fix */
            T3 = pParam->B4SOIwdiodCV * pParam->B4SOIcgdl; /* v3.1 bug fix */

            T4 = sqrt(1.0 - 4.0 * T2 / pParam->B4SOIckappa);
            cgdo = pParam->B4SOIcgdo + T3 - T3 * (1.0 - 1.0 / T4)
                   * (0.5 - 0.5 * T0 / T1);
            qgdo = (pParam->B4SOIcgdo + T3) * vgd - T3 * (T2
                    + 0.5 * pParam->B4SOIckappa * (T4 - 1.0));

            if (here->B4SOIrgateMod == 3)
            {
                qgdo = (pParam->B4SOIcgdo + T3) * vgmd - T3 * (T2
                        + 0.5 * pParam->B4SOIckappa * (T4 - 1.0));
            }   /* v3.2 bug fix */

            T0 = vgs + DELTA_1;
            if (here->B4SOIrgateMod == 3) T0 = vgms + DELTA_1; /* v3.2 bug fix */
            T1 = sqrt(T0 * T0 + 4.0 * DELTA_1);
            T2 = 0.5 * (T0 - T1);

            /* v2.2.3 bug fix */
            T3 = pParam->B4SOIwdiosCV * pParam->B4SOIcgsl; /* v3.1 bug fix */

            T4 = sqrt(1.0 - 4.0 * T2 / pParam->B4SOIckappa);
            cgso = pParam->B4SOIcgso + T3 - T3 * (1.0 - 1.0 / T4)
                   * (0.5 - 0.5 * T0 / T1);
            qgso = (pParam->B4SOIcgso + T3) * vgs - T3 * (T2
                    + 0.5 * pParam->B4SOIckappa * (T4 - 1.0));

            if (here->B4SOIrgateMod == 3)
            {
                qgso = (pParam->B4SOIcgso + T3) * vgms - T3 * (T2
                        + 0.5 * pParam->B4SOIckappa * (T4 - 1.0));
            }   /* v3.2 bug fix */

            if (here->B4SOInf != 1.0)
            {
                cgdo *= here->B4SOInf;
                cgso *= here->B4SOInf;
                qgdo *= here->B4SOInf;
                qgso *= here->B4SOInf;
            }
            /*            here->B4SOIcgdo = cgdo;
                          here->B4SOIcgso = cgso;
                          */
            if (here->B4SOIdebugMod < 0)
                goto line850;


            if (here->B4SOImode > 0)
            {

                /* v3.1 added for RF */
                if (here->B4SOIrgateMod == 3)
                {
                    gcgmgmb = (cgdo + cgso + pParam->B4SOIcgeo) * ag0;

                    gcgmdb = -cgdo * ag0;
                    gcgmsb = -cgso * ag0;
                    gcgmeb = -pParam->B4SOIcgeo * ag0;
                    gcdgmb = gcgmdb;
                    gcsgmb = gcgmsb;
                    gcegmb = gcgmeb;

                    gcggb = here->B4SOIcggb * ag0;
                    gcgdb = here->B4SOIcgdb * ag0;
                    gcgsb = here->B4SOIcgsb * ag0;
                    gcgeb = 0 ;/*v3.1 changed*/
                    gcgbb = -(gcggb + gcgdb + gcgsb + gcgeb);

                    gcdgb = here->B4SOIcdgb * ag0;
                    gcegb = gcgeb; /*v3.1 added*/
                    gcsgb = -(here->B4SOIcggb + here->B4SOIcbgb
                              + here->B4SOIcdgb) * ag0 - gcegb;
                    gcbgb = here->B4SOIcbgb * ag0;

                    qgd = qgdo;
                    qgs = qgso;
                    qge = 0; /* v3.1 change */

                    qgme = pParam->B4SOIcgeo * vgme;
                    qgmid = qgdo + qgso + qgme;
                    qdrn += here->B4SOIqde - qgd;
                    qsub -= qgme + here->B4SOIqse + here->B4SOIqde;
                    qsrc = -(qgate + qgmid + qbody + qdrn + qsub) - qjs;
                    qdrn -= qjd;
                    if (!here->B4SOIrbodyMod) qbody += qjd + qjs;

                }
                else
                {
                    gcggb = (here->B4SOIcggb + cgdo + cgso
                             + pParam->B4SOIcgeo) * ag0;
                    gcgdb = (here->B4SOIcgdb - cgdo) * ag0;
                    gcgsb = (here->B4SOIcgsb - cgso) * ag0;
                    gcgeb = (-pParam->B4SOIcgeo) *ag0;
                    gcgbb = -(gcggb + gcgdb + gcgsb + gcgeb);

                    gcegb = (- pParam->B4SOIcgeo) * ag0;
                    gcdgb = (here->B4SOIcdgb - cgdo) * ag0;
                    gcsgb = -(here->B4SOIcggb + here->B4SOIcbgb
                              + here->B4SOIcdgb + cgso) * ag0;
                    gcbgb = here->B4SOIcbgb * ag0;

                    gcdgmb = gcsgmb = gcegmb = 0.0;
                    gcgmdb = gcgmsb = gcgmeb = 0.0;

                    /* Lump the overlap capacitance and S/D parasitics */
                    qgd = qgdo;
                    qgs = qgso;
                    qge = pParam->B4SOIcgeo * vge;
                    qgate += qgd + qgs + qge;
                    qdrn += here->B4SOIqde - qgd;
                    qsub -= qge + here->B4SOIqse + here->B4SOIqde;
                    qsrc = -(qgate + qbody + qdrn + qsub) - qjs;
                    qdrn -= qjd;
                    if (!here->B4SOIrbodyMod) qbody += qjd + qjs;
                }

                gcddb = (here->B4SOIcddb + cgdo + here->B4SOIgcde) * ag0;
                gcdsb = here->B4SOIcdsb * ag0;
                gcdeb = (here->B4SOIcdeb - here->B4SOIgcde) * ag0;
                gcdT = model->B4SOItype * here->B4SOIcdT * ag0;

                gcsdb = -(here->B4SOIcgdb + here->B4SOIcbdb
                          + here->B4SOIcddb) * ag0;
                gcssb = (cgso + here->B4SOIgcse - (here->B4SOIcgsb
                                                   + here->B4SOIcbsb + here->B4SOIcdsb)) * ag0;
                gcseb = -(here->B4SOIgcse + here->B4SOIcbeb
                          + here->B4SOIcdeb + here->B4SOIceeb) * ag0;
                gcsT = - model->B4SOItype * (here->B4SOIcgT
                                             + here->B4SOIcbT + here->B4SOIcdT + here->B4SOIceT)
                       * ag0;

                gcgT = model->B4SOItype * here->B4SOIcgT * ag0;

                /*                     gcbdb = here->B4SOIcbdb * ag0;
                                       gcbsb = here->B4SOIcbsb * ag0;
                                       */
                gcbeb = here->B4SOIcbeb * ag0;
                gcbT = model->B4SOItype * here->B4SOIcbT * ag0;

                /* v4.0 */
                if (!here->B4SOIrbodyMod)
                {
                    gcjdbdp = gcjsbsp = 0.0;
                    gcdbb = -(gcdgb + gcddb + gcdsb + gcdgmb + gcdeb);
                    gcsbb = -(gcsgb + gcsdb + gcssb + gcsgmb + gcseb);
                    gcdbdb = gcsbsb = 0.0;
                    gcbdb = here->B4SOIcbdb * ag0;
                    gcbsb = here->B4SOIcbsb * ag0;
                    here->B4SOIGGjdb = GGjdb = 0.0;
                    here->B4SOIGGjsb = GGjsb = 0.0;
                }
                else
                {
                    gcjdbdp = gcjdbs * ag0;
                    gcjsbsp = gcjsbs * ag0;
                    gcdbb = -(gcdgb + gcddb + gcdsb + gcdgmb + gcdeb)
                            + gcjdbdp;
                    gcsbb = -(gcsgb + gcsdb + gcssb + gcsgmb + gcseb)
                            + gcjsbsp;
                    /* v4.2 optimization: gcjdds + gcjdbs = 0
                       gcdbdb = gcjdds * ag0;
                       gcsbsb = -(gcjdds + gcjdbs + gcjsbs) * ag0;
                       */
                    gcdbdb = -gcjdbs * ag0;
                    gcsbsb = -gcjsbs * ag0;
                    gcbdb = here->B4SOIcbdb * ag0 - gcdbdb;
                    gcbsb = here->B4SOIcbsb * ag0 - gcsbsb;
                    here->B4SOIGGjdb = GGjdb = Gjdb;
                    here->B4SOIGGjsb = GGjsb = Gjsb;
                }
                /* v4.0 end */

                gcedb = (- here->B4SOIgcde) * ag0;
                gcesb = (- here->B4SOIgcse) * ag0;
                gceeb = (here->B4SOIgcse + here->B4SOIgcde +
                         here->B4SOIceeb + pParam->B4SOIcgeo) * ag0;

                gceT = model->B4SOItype * here->B4SOIceT * ag0;

                gcTt = pParam->B4SOIcth * ag0;

//                sxpart = 0.6;
//                dxpart = 0.4;


                /* v3.1 moved the following original code ahead */
                /* Lump the overlap capacitance and S/D parasitics */
                /*                  qgd = qgdo;
                                    qgs = qgso;
                                    qge = pParam->B4SOIcgeo * vge;
                                    qgate += qgd + qgs + qge;
                                    qdrn += here->B4SOIqde - qgd;
                                    qsub -= qge + here->B4SOIqse + here->B4SOIqde;
                                    qsrc = -(qgate + qbody + qdrn + qsub);
                                    */
                /* v3.1 end */

            }

            else
            {
                if (here->B4SOIrgateMod == 3)
                {
                    gcgmgmb = (cgdo + cgso + pParam->B4SOIcgeo) * ag0;
                    gcgmdb = -cgdo * ag0;
                    gcgmsb = -cgso * ag0;
                    gcgmeb = -pParam->B4SOIcgeo * ag0;
                    gcdgmb = gcgmdb;
                    gcsgmb = gcgmsb;
                    gcegmb = gcgmeb;

                    gcggb = here->B4SOIcggb * ag0;
                    gcgsb = here->B4SOIcgdb * ag0;
                    gcgdb = here->B4SOIcgsb * ag0;
                    gcgeb = 0; /* v3.1 */
                    gcgbb = -(gcggb + gcgdb + gcgsb + gcgeb); /* v3.1 added gcgeb */

                    gcsgb = here->B4SOIcdgb * ag0;
                    gcegb = gcgeb; /* v3.1 added */
                    gcdgb = -(here->B4SOIcggb + here->B4SOIcbgb
                              + here->B4SOIcdgb) * ag0 - gcegb; /*v3.1 added gcegb*/
                    gcbgb = here->B4SOIcbgb * ag0;

                    qgd = qgdo;
                    qgs = qgso;
                    qge = 0; /* v3.1 */
                    qgme = pParam->B4SOIcgeo * vgme;
                    qgmid = qgdo + qgso + qgme;
                    qgate += qge;
                    qbody -= 0;
                    qsrc = qdrn - qgs + here->B4SOIqse;
                    qsub -= qgme + here->B4SOIqse + here->B4SOIqde;
                    qdrn = -(qgate + qgmid + qbody + qsrc + qsub) -qjd;
                    qsrc -= qjs;
                    if (!here->B4SOIrbodyMod) qbody += qjs + qjd;
                }
                else
                {
                    gcggb = (here->B4SOIcggb + cgdo + cgso + pParam->B4SOIcgeo) * ag0;
                    gcgdb = (here->B4SOIcgsb - cgdo) * ag0;
                    gcgsb = (here->B4SOIcgdb - cgso) * ag0;
                    gcgeb = (- pParam->B4SOIcgeo) * ag0;
                    gcgbb = -(gcggb + gcgdb + gcgsb + gcgeb); /*added gcgbb*/

                    gcegb = gcgeb; /* v3.1 added */
                    gcsgb = (here->B4SOIcdgb - cgso) * ag0;
                    gcdgb = -(here->B4SOIcggb + here->B4SOIcbgb + here->B4SOIcdgb + cgdo) * ag0;
                    gcbgb = here->B4SOIcbgb * ag0;

                    gcdgmb = gcsgmb = gcegmb = 0.0;
                    gcgmdb = gcgmsb = gcgmeb = 0.0;

                    /* Lump the overlap capacitance and S/D parasitics */
                    qgd = qgdo;
                    qgs = qgso;
                    qge = pParam->B4SOIcgeo * vge;
                    qgate += qgd + qgs + qge;
                    qsrc = qdrn - qgs + here->B4SOIqse;
                    qsub -= qge + here->B4SOIqse + here->B4SOIqde;
                    qdrn = -(qgate + qbody + qsrc + qsub) - qjd;
                    qsrc -= qjs;
                    if (!here->B4SOIrbodyMod) qbody += qjs + qjd;
                }

                gcssb = (here->B4SOIcddb + cgso + here->B4SOIgcse) * ag0;
                gcsdb = here->B4SOIcdsb * ag0;
                gcseb = (here->B4SOIcdeb - here->B4SOIgcse) * ag0;
                gcsT = model->B4SOItype * here->B4SOIcdT * ag0;

                gcdsb = -(here->B4SOIcgdb + here->B4SOIcbdb
                          + here->B4SOIcddb) * ag0;
                gcddb = (cgdo + here->B4SOIgcde - (here->B4SOIcgsb
                                                   + here->B4SOIcbsb + here->B4SOIcdsb)) * ag0;
                gcdeb = -(here->B4SOIgcde + here->B4SOIcbeb
                          + here->B4SOIcdeb + here->B4SOIceeb) * ag0;
                gcdT = - model->B4SOItype * (here->B4SOIcgT
                                             + here->B4SOIcbT + here->B4SOIcdT + here->B4SOIceT)
                       * ag0;

                gcgT = model->B4SOItype * here->B4SOIcgT * ag0;

                gcbeb = here->B4SOIcbeb * ag0;
                gcbT = model->B4SOItype * here->B4SOIcbT * ag0;
                /* v4.0                   gcbsb = here->B4SOIcbdb * ag0;
                   gcbdb = here->B4SOIcbsb * ag0;
                   */

                /* v4.0 */
                if (!here->B4SOIrbodyMod)
                {
                    gcjdbdp = gcjsbsp = 0.0;
                    gcdbb = -(gcdgb + gcddb + gcdsb + gcdgmb + gcdeb);
                    gcsbb = -(gcsgb + gcsdb + gcssb + gcsgmb + gcseb);
                    gcdbdb = gcsbsb = 0.0;
                    gcbdb = here->B4SOIcbsb * ag0;
                    gcbsb = here->B4SOIcbdb * ag0;
                    here->B4SOIGGjdb = GGjdb = 0.0;
                    here->B4SOIGGjsb = GGjsb = 0.0;
                }
                else
                {
                    /* v4.2 bugfix; qjd/qjs are not swapped
                       gcjdbdp = gcjsbs * ag0;
                       gcjsbsp = gcjdbs * ag0;
                       */
                    gcjdbdp = gcjdbs * ag0;
                    gcjsbsp = gcjsbs * ag0;
                    gcdbb = -(gcdgb + gcddb + gcdsb + gcdgmb + gcdeb)
                            + gcjdbdp;
                    gcsbb = -(gcsgb + gcsdb + gcssb + gcsgmb + gcseb)
                            + gcjsbsp;
                    /* v4.2 bugfix; qjd/qjs are not swapped
                       gcsbsb = gcjdds * ag0;
                       gcdbdb = -(gcjdds + gcjdbs + gcjsbs) * ag0;
                       */
                    gcsbsb = -gcjdbs * ag0;
                    gcdbdb = -gcjsbs * ag0;
                    gcbdb = here->B4SOIcbsb * ag0 - gcdbdb;
                    gcbsb = here->B4SOIcbdb * ag0 - gcsbsb;
                    here->B4SOIGGjdb = GGjdb = Gjsb;
                    here->B4SOIGGjsb = GGjsb = Gjdb;
                }
                /* v4.0 end */

                /*                  gcegb = (-pParam->B4SOIcgeo) * ag0; V3.2 bug fix */
                gcesb = (- here->B4SOIgcse) * ag0;
                gcedb = (- here->B4SOIgcde) * ag0;
                gceeb = (here->B4SOIceeb + pParam->B4SOIcgeo +
                         here->B4SOIgcse + here->B4SOIgcde) * ag0;
                gceT = model->B4SOItype * here->B4SOIceT * ag0;

                gcTt = pParam->B4SOIcth * ag0;

//                dxpart = 0.6;
//                sxpart = 0.4;


                /* v3.1 moved the following code ahead */
                /* Lump the overlap capacitance */
                /*
                   qgd = qgdo;
                   gs = qgso;
                   qge = pParam->B4SOIcgeo * vge;
                   qgate += qgd + qgs + qge;
                   qsrc = qdrn - qgs + here->B4SOIqse;
                   qsub -= qge + here->B4SOIqse + here->B4SOIqde;
                   qdrn = -(qgate + qbody + qsrc + qsub);
                   */
                /* v3.1 end */


            }

            here->B4SOIcgdo = cgdo;
            here->B4SOIcgso = cgso;

            if (ByPass) goto line860;

            *(ckt->CKTstate0 + here->B4SOIqe) = qsub;
            *(ckt->CKTstate0 + here->B4SOIqg) = qgate;
            *(ckt->CKTstate0 + here->B4SOIqd) = qdrn;
            *(ckt->CKTstate0 + here->B4SOIqb) = qbody;
            if ((model->B4SOIshMod == 1) && (here->B4SOIrth0!=0.0))
                *(ckt->CKTstate0 + here->B4SOIqth) = pParam->B4SOIcth * delTemp;
            if (here->B4SOIrgateMod == 3) /* 3.1 bug fix */
                *(ckt->CKTstate0 + here->B4SOIqgmid) = qgmid;


            /* store small signal parameters */
            if (ckt->CKTmode & MODEINITSMSIG)
            {
                goto line1000;
            }
            if (!ChargeComputationNeeded)
                goto line850;


            if (ckt->CKTmode & MODEINITTRAN)
            {
                *(ckt->CKTstate1 + here->B4SOIqb) =
                    *(ckt->CKTstate0 + here->B4SOIqb);
                *(ckt->CKTstate1 + here->B4SOIqg) =
                    *(ckt->CKTstate0 + here->B4SOIqg);
                *(ckt->CKTstate1 + here->B4SOIqd) =
                    *(ckt->CKTstate0 + here->B4SOIqd);
                *(ckt->CKTstate1 + here->B4SOIqe) =
                    *(ckt->CKTstate0 + here->B4SOIqe);
                *(ckt->CKTstate1 + here->B4SOIqth) =
                    *(ckt->CKTstate0 + here->B4SOIqth);
                if (here->B4SOIrgateMod == 3)
                    *(ckt->CKTstate1 + here->B4SOIqgmid) =
                        *(ckt->CKTstate0 + here->B4SOIqgmid);
                if (here->B4SOIrbodyMod) /* v4.0 */
                {
                    *(ckt->CKTstate1 + here->B4SOIqbs) =
                        *(ckt->CKTstate0 + here->B4SOIqbs);
                    *(ckt->CKTstate1 + here->B4SOIqbd) =
                        *(ckt->CKTstate0 + here->B4SOIqbd);
                }

            }

            error = NIintegrate(ckt, &geq, &ceq,0.0,here->B4SOIqb);
            if (error) return(error);
            error = NIintegrate(ckt, &geq, &ceq, 0.0, here->B4SOIqg);
            if (error) return(error);
            error = NIintegrate(ckt,&geq, &ceq, 0.0, here->B4SOIqd);
            if (error) return(error);
            error = NIintegrate(ckt,&geq, &ceq, 0.0, here->B4SOIqe);
            if (error) return(error);
            if ((model->B4SOIshMod == 1) && (here->B4SOIrth0!=0.0))
            {
                error = NIintegrate(ckt, &geq, &ceq, 0.0, here->B4SOIqth);
                if (error) return (error);
            }

            if (here->B4SOIrgateMod == 3)
            {
                error = NIintegrate(ckt, &geq, &ceq, 0.0, here->B4SOIqgmid);
                if (error) return(error);
            }   /*3.1 bug fix*/

            if (here->B4SOIrbodyMod)     /* v4.0 */
            {
                error = NIintegrate(ckt, &geq, &ceq, 0.0, here->B4SOIqbs);
                if (error) return(error);
                error = NIintegrate(ckt, &geq, &ceq, 0.0, here->B4SOIqbd);
                if (error) return(error);
            }

            goto line860;

line850:
            /* initialize to zero charge conductance and current */
            ceqqe = ceqqg = ceqqb = ceqqd = ceqqth= 0.0;

            gcdgb = gcddb = gcdsb = gcdeb = gcdT = 0.0;
            gcsgb = gcsdb = gcssb = gcseb = gcsT = 0.0;
            gcggb = gcgdb = gcgsb = gcgeb = gcgT = 0.0;
            gcbgb = gcbdb = gcbsb = gcbeb = gcbT = 0.0;
            gcegb = gcedb = gceeb = gcesb = gceT = 0.0;
            gcTt = 0.0;

            /* v3.1 added for RF */
            gcgmgmb = gcgmdb = gcgmsb = gcgmeb = 0.0;
            gcdgmb = gcsgmb = gcegmb = ceqqgmid = 0.0;
            gcgbb = gcsbb = gcdbb = 0.0;
            /* v3.1 added for RF end */

            gcdbdb = gcsbsb = gcjdbdp = gcjsbsp = 0.0; /* v4.0 */
            ceqqjd = ceqqjs = 0.0; /* v4.0 */
            GGjdb = GGjsb = 0.0;   /* v4.0 */

//            sxpart = (1.0 - (dxpart = (here->B4SOImode > 0) ? 0.4 : 0.6));

            goto line900;

line860:
            /* evaluate equivalent charge current */

            cqgate = *(ckt->CKTstate0 + here->B4SOIcqg);
            cqbody = *(ckt->CKTstate0 + here->B4SOIcqb);
            cqdrn = *(ckt->CKTstate0 + here->B4SOIcqd);
            cqsub = *(ckt->CKTstate0 + here->B4SOIcqe);
            cqtemp = *(ckt->CKTstate0 + here->B4SOIcqth);

            here->B4SOIcb += cqbody;
            here->B4SOIcd += cqdrn;

            ceqqg = cqgate - gcggb * vgb + gcgdb * vbd + gcgsb * vbs
                    - gcgeb * veb - gcgT * delTemp;

            ceqqb = cqbody - gcbgb * vgb + gcbdb * vbd + gcbsb * vbs
                    - gcbeb * veb - gcbT * delTemp; /* v3.2 bug fix */
            ceqqd = cqdrn - gcdgb * vgb + (gcddb + gcdbdb) * vbd
                    + gcdsb * vbs - gcdeb * veb - gcdT * delTemp
                    - gcdbdb * vbd_jct - gcdgmb * vgmb;/* v4.0 */

            ceqqe = cqsub - gcegb * vgb + gcedb * vbd + gcesb * vbs
                    - gceeb * veb - gceT * delTemp - gcegmb * vgmb; /* 3.2 bug fix */
            ceqqth = cqtemp - gcTt * delTemp;

            /* v3.1 added for RF */
            if (here->B4SOIrgateMod == 3)
                ceqqgmid = *(ckt->CKTstate0 + here->B4SOIcqgmid)
                           + gcgmdb * vbd + gcgmsb * vbs - gcgmgmb * vgmb;/* 3.2 bug fix */
            else
                ceqqgmid = 0.0;
            /* v3.1 added for RF end */

            if (here->B4SOIrbodyMod) /* v4.0 */
            {
                ceqqjs = *(ckt->CKTstate0 + here->B4SOIcqbs)
                         + gcsbsb * vbs_jct;
                ceqqjd = *(ckt->CKTstate0 + here->B4SOIcqbd)
                         + gcdbdb * vbd_jct;
            }

            if (ckt->CKTmode & MODEINITTRAN)
            {
                *(ckt->CKTstate1 + here->B4SOIcqe) =
                    *(ckt->CKTstate0 + here->B4SOIcqe);
                *(ckt->CKTstate1 + here->B4SOIcqb) =
                    *(ckt->CKTstate0 + here->B4SOIcqb);
                *(ckt->CKTstate1 + here->B4SOIcqg) =
                    *(ckt->CKTstate0 + here->B4SOIcqg);
                *(ckt->CKTstate1 + here->B4SOIcqd) =
                    *(ckt->CKTstate0 + here->B4SOIcqd);
                *(ckt->CKTstate1 + here->B4SOIcqth) =
                    *(ckt->CKTstate0 + here->B4SOIcqth);

                if (here->B4SOIrgateMod == 3) /* v3.1 */
                    *(ckt->CKTstate1 + here->B4SOIcqgmid) =
                        *(ckt->CKTstate0 + here->B4SOIcqgmid);

                if (here->B4SOIrbodyMod) /* v4.0 */
                {
                    *(ckt->CKTstate1 + here->B4SOIcqbs) =
                        *(ckt->CKTstate0 + here->B4SOIcqbs);
                    *(ckt->CKTstate1 + here->B4SOIcqbd) =
                        *(ckt->CKTstate0 + here->B4SOIcqbd);
                }

            }

            /*
             *  load current vector
             */
line900:

            if (here->B4SOImode >= 0)
            {
                Gm = here->B4SOIgm;
                Gmbs = here->B4SOIgmbs;
                /* v3.0 */
                Gme = here->B4SOIgme;

                GmT = model->B4SOItype * here->B4SOIgmT;
                FwdSum = Gm + Gmbs + Gme; /* v3.0 */
                RevSum = 0.0;

                /* v2.2.2 bug fix */
                cdreq = model->B4SOItype * (here->B4SOIcdrain
                                            - here->B4SOIgds * vds - Gm * vgs - Gmbs * vbs
                                            - Gme * ves) - GmT * delTemp; /* v3.0 */

                /* ceqbs now is compatible with cdreq, ie. going in is +ve */
                /* Equivalent current source from the diode */
                ceqbs = here->B4SOIcjs;
                ceqbd = here->B4SOIcjd;
                cdbdp = Idbdp;
                csbsp = Isbsp;

                /* Current going in is +ve */
                ceqbody = -here->B4SOIcbody;

                ceqgate = here->B4SOIcgate;
                gigg = here->B4SOIgigg;
                gigb = here->B4SOIgigb;
                gige = here->B4SOIgige; /* v3.0 */
                gigs = here->B4SOIgigs;
                gigd = here->B4SOIgigd;
                gigT = model->B4SOItype * here->B4SOIgigT;

                ceqth = here->B4SOIcth;
                ceqbodcon = here->B4SOIcbodcon;

                /* v4.1 */
                gigpg = here->B4SOIgigpg;
                gigpp = here->B4SOIgigpp;
                ceqgate += (here->B4SOIigp - gigpg * vgp);
                if(here->B4SOIbodyMod == 1)
                    ceqbodcon += (here->B4SOIigp - gigpg * vgp);
                else if(here->B4SOIbodyMod == 2)
                    ceqbody -= (here->B4SOIigp - gigpg * vgp);

                gbbg  = -here->B4SOIgbgs;
                gbbdp = -here->B4SOIgbds;
                gbbb  = -here->B4SOIgbbs;
                gbbp  = -here->B4SOIgbps;
                gbbT  = -model->B4SOItype * here->B4SOIgbT;
                /* v3.0 */
                gbbe  = -here->B4SOIgbes;

                if (here->B4SOIrbodyMod)   /* v4.0 */
                {
                    gbbdp = -Giid - Ggidld - Ggisls;
                    gbbb = -Giib + Gbpbs;
//                    gjsdb = Gjsb + Gjdb;
                }

                gbbsp = - ( gbbg + gbbdp + gbbb + gbbp + gbbe);

                gddpg  = -here->B4SOIgjdg;
                gddpdp = -here->B4SOIgjdd;
                if (!here->B4SOIrbodyMod) /* v4.0 */
                    gddpb  = -here->B4SOIgjdb;
                else
                    gddpb = Giib + Ggidlb + Ggislb;
                gddpT  = -model->B4SOItype * here->B4SOIgjdT;
                /* v3.0 */
                gddpe  = -here->B4SOIgjde;
                gddpsp = - ( gddpg + gddpdp + gddpb + gddpe);

                gsspg  = -here->B4SOIgjsg;
                gsspdp = -here->B4SOIgjsd;
                if (!here->B4SOIrbodyMod)
                    gsspb  = -here->B4SOIgjsb;
                else
                    gsspb = 0.0;
                gsspT  = -model->B4SOItype * here->B4SOIgjsT;
                /* v3.0 */
                gsspe  = 0.0;
                gsspsp = - (gsspg + gsspdp + gsspb + gsspe);

                gppb = -here->B4SOIgbpbs;
                gppp = -here->B4SOIgbpps;

                gTtg  = here->B4SOIgtempg;
                gTtb  = here->B4SOIgtempb;
                gTtdp = here->B4SOIgtempd;
                gTtt  = here->B4SOIgtempT;

                /* v3.0 */
                gTte  = here->B4SOIgtempe;
                gTtsp = - (gTtg + gTtb + gTtdp + gTte);


                /* v3.0 */
                if (model->B4SOIigcMod)
                {
                    gIstotg = here->B4SOIgIgsg + here->B4SOIgIgcsg;
                    gIstotd = here->B4SOIgIgcsd;
                    gIstots = here->B4SOIgIgss + here->B4SOIgIgcss;
                    gIstotb = here->B4SOIgIgcsb;
                    Istoteq = model->B4SOItype * (here->B4SOIIgs + here->B4SOIIgcs
                                                  - gIstotg * vgs - here->B4SOIgIgcsd * vds
                                                  - here->B4SOIgIgcsb * vbs);

                    gIdtotg = here->B4SOIgIgdg + here->B4SOIgIgcdg;
                    gIdtotd = here->B4SOIgIgdd + here->B4SOIgIgcdd;
                    gIdtots = here->B4SOIgIgcds;
                    gIdtotb = here->B4SOIgIgcdb;
                    Idtoteq = model->B4SOItype * (here->B4SOIIgd + here->B4SOIIgcd
                                                  - here->B4SOIgIgdg * vgd - here->B4SOIgIgcdg * vgs
                                                  - here->B4SOIgIgcdd * vds - here->B4SOIgIgcdb * vbs);

                    gIgtotg = gIstotg + gIdtotg;
                    gIgtotd = gIstotd + gIdtotd;
                    gIgtots = gIstots + gIdtots;
                    gIgtotb = gIstotb + gIdtotb;
                    Igtoteq = Istoteq + Idtoteq;
                }
                else
                {
                    gIstotg = gIstotd = gIstots = gIstotb = Istoteq = 0.0;
                    gIdtotg = gIdtotd = gIdtots = gIdtotb = Idtoteq = 0.0;

                    gIgtotg = gIgtotd = gIgtots = gIgtotb = Igtoteq = 0.0;
                }

                /* v3.1 added for RF */
                if (here->B4SOIrgateMod == 2)
                    T0 = vges - vgs;
                else if (here->B4SOIrgateMod == 3)
                    T0 = vgms - vgs;
                if (here->B4SOIrgateMod > 1)
                {
                    gcrgd = here->B4SOIgcrgd * T0;
                    gcrgg = here->B4SOIgcrgg * T0;
                    gcrgs = here->B4SOIgcrgs * T0;
                    gcrgb = here->B4SOIgcrgb * T0;

                    ceqgcrg = -(gcrgd * vds + gcrgg * vgs
                                + gcrgb * vbs);
                    gcrgg -= here->B4SOIgcrg;
                    gcrg = here->B4SOIgcrg;
                }
                else
                    ceqgcrg = gcrg = gcrgd = gcrgg = gcrgs = gcrgb = 0.0;
                /* v3.1 added for RF end */

            } /* end of soimode>=0 */

            else
            {
                Gm = -here->B4SOIgm;
                Gmbs = -here->B4SOIgmbs;
                /* v3.0 */
                Gme = -here->B4SOIgme;

                GmT = -model->B4SOItype * here->B4SOIgmT;
                FwdSum = 0.0;
                RevSum = -(Gm + Gmbs + Gme); /* v3.0 */


                /* v3.1 bug fix */
                cdreq = -model->B4SOItype * (here->B4SOIcdrain + here->B4SOIgds*vds
                                             + Gm * vgd + Gmbs * vbd + Gme * (ves - vds))
                        - GmT * delTemp;


                ceqbs = here->B4SOIcjd;
                ceqbd = here->B4SOIcjs;
                csbsp = Idbdp;
                cdbdp = Isbsp;

                /* Current going in is +ve */
                ceqbody = -here->B4SOIcbody;


                ceqgate = here->B4SOIcgate;
                gigg = here->B4SOIgigg;
                gigb = here->B4SOIgigb;
                gige = here->B4SOIgige; /* v3.0 */
                gigs = here->B4SOIgigd;
                gigd = here->B4SOIgigs;
                gigT = model->B4SOItype * here->B4SOIgigT;

                ceqth = here->B4SOIcth;
                ceqbodcon = here->B4SOIcbodcon;

                /* v4.1 */
                gigpg = here->B4SOIgigpg;
                gigpp = here->B4SOIgigpp;
                ceqgate += (here->B4SOIigp - gigpg * vgp);
                if(here->B4SOIbodyMod == 1)
                    ceqbodcon += (here->B4SOIigp - gigpg * vgp);
                else if(here->B4SOIbodyMod == 2)
                    ceqbody -= (here->B4SOIigp - gigpg * vgp);

                gbbg  = -here->B4SOIgbgs;
                gbbb  = -here->B4SOIgbbs;
                gbbp  = -here->B4SOIgbps;
                gbbsp = -here->B4SOIgbds;
                gbbT  = -model->B4SOItype * here->B4SOIgbT;
                /* v3.0 */
                gbbe  = -here->B4SOIgbes;

                if (here->B4SOIrbodyMod)   /* v4.0 */
                {
                    gbbsp = -Giid - Ggidld - Ggisls;
                    gbbb = -Giib + Gbpbs;
//                    gjsdb = Gjsb + Gjdb;
                }
                gbbdp = - ( gbbg + gbbsp + gbbb + gbbp + gbbe);

                gddpg  = -here->B4SOIgjsg;
                gddpsp = -here->B4SOIgjsd;
                if (!here->B4SOIrbodyMod)
                    gddpb  = -here->B4SOIgjsb;
                else
                    gddpb =  0.0;
                gddpT  = -model->B4SOItype * here->B4SOIgjsT;
                /* v3.0 */
                gddpe  = 0.0;
                gddpdp = - (gddpg + gddpsp + gddpb + gddpe);

                gsspg  = -here->B4SOIgjdg;
                gsspsp = -here->B4SOIgjdd;
                if (!here->B4SOIrbodyMod)
                    gsspb  = -here->B4SOIgjdb;
                else
                    gsspb = Giib + Ggidlb + Ggislb;
                gsspT  = -model->B4SOItype * here->B4SOIgjdT;
                /* v3.0 */
                gsspe  = -here->B4SOIgjde;
                gsspdp = - ( gsspg + gsspsp + gsspb + gsspe);

                gppb = -here->B4SOIgbpbs;
                gppp = -here->B4SOIgbpps;

                gTtg  = here->B4SOIgtempg;
                gTtb  = here->B4SOIgtempb;
                gTtsp = here->B4SOIgtempd;
                gTtt  = here->B4SOIgtempT;

                /* v3.0 */
                gTte  = here->B4SOIgtempe;
                gTtdp = - (gTtg + gTtb + gTtsp + gTte);

                /* v3.0 */
                if (model->B4SOIigcMod)
                {
                    gIstotg = here->B4SOIgIgsg + here->B4SOIgIgcdg;
                    gIstotd = here->B4SOIgIgcds;
                    gIstots = here->B4SOIgIgss + here->B4SOIgIgcdd;
                    gIstotb = here->B4SOIgIgcdb;
                    Istoteq = model->B4SOItype * (here->B4SOIIgs + here->B4SOIIgcd
                                                  - here->B4SOIgIgsg * vgs - here->B4SOIgIgcdg * vgd
                                                  + here->B4SOIgIgcdd * vds - here->B4SOIgIgcdb * vbd);

                    gIdtotg = here->B4SOIgIgdg + here->B4SOIgIgcsg;
                    gIdtotd = here->B4SOIgIgdd + here->B4SOIgIgcss;
                    gIdtots = here->B4SOIgIgcsd;
                    gIdtotb = here->B4SOIgIgcsb;
                    Idtoteq = model->B4SOItype * (here->B4SOIIgd + here->B4SOIIgcs
                                                  - (here->B4SOIgIgdg + here->B4SOIgIgcsg) * vgd
                                                  + here->B4SOIgIgcsd * vds - here->B4SOIgIgcsb * vbd);

                    gIgtotg = gIstotg + gIdtotg;
                    gIgtotd = gIstotd + gIdtotd;
                    gIgtots = gIstots + gIdtots;
                    gIgtotb = gIstotb + gIdtotb;
                    Igtoteq = Istoteq + Idtoteq;

                }
                else
                {
                    gIstotg = gIstotd = gIstots = gIstotb = Istoteq = 0.0;
                    gIdtotg = gIdtotd = gIdtots = gIdtotb = Idtoteq = 0.0;

                    gIgtotg = gIgtotd = gIgtots = gIgtotb = Igtoteq = 0.0;
                }

                /* v3.1 added for RF */
                if (here->B4SOIrgateMod == 2)
                    T0 = vges - vgs;
                else if (here->B4SOIrgateMod == 3)
                    T0 = vgms - vgs;
                if (here->B4SOIrgateMod > 1)
                {
                    gcrgd = here->B4SOIgcrgs * T0;
                    gcrgg = here->B4SOIgcrgg * T0;
                    gcrgs = here->B4SOIgcrgd * T0;
                    gcrgb = here->B4SOIgcrgb * T0;
                    ceqgcrg = -(gcrgg * vgd - gcrgs * vds
                                + gcrgb * vbd);
                    gcrgg -= here->B4SOIgcrg;
                    gcrg = here->B4SOIgcrg;
                }
                else
                    ceqgcrg = gcrg = gcrgd = gcrgg = gcrgs = gcrgb = 0.0;
                /* v3.1 added for RF end */

            } /* end of soimod<0 */


            if (model->B4SOIrdsMod == 1)
            {
                ceqgstot = model->B4SOItype * (here->B4SOIgstotd * vds
                                               + here->B4SOIgstotg * vgs + here->B4SOIgstotb * vbs);
                /* ceqgstot flowing away from sNodePrime */
                gstot = here->B4SOIgstot;
                gstotd = here->B4SOIgstotd;
                gstotg = here->B4SOIgstotg;
                gstots = here->B4SOIgstots - gstot;
                gstotb = here->B4SOIgstotb;

                ceqgdtot = -model->B4SOItype * (here->B4SOIgdtotd * vds
                                                + here->B4SOIgdtotg * vgs + here->B4SOIgdtotb * vbs);
                /* ceqgdtot defined as flowing into dNodePrime */
                gdtot = here->B4SOIgdtot;
                gdtotd = here->B4SOIgdtotd - gdtot;
                gdtotg = here->B4SOIgdtotg;
                gdtots = here->B4SOIgdtots;
                gdtotb = here->B4SOIgdtotb;
            }
            else
            {
                gstot = gstotd = gstotg = gstots
                                          = gstotb = ceqgstot = 0.0;
                gdtot = gdtotd = gdtotg = gdtots
                                          = gdtotb = ceqgdtot = 0.0;
            }

            if (model->B4SOItype > 0)
            {
                //ceqqg = ceqqg;
                //ceqqb = ceqqb;
                //ceqqe = ceqqe;
                //ceqqd = ceqqd;
            }
            else
            {
                ceqbodcon = -ceqbodcon;
                ceqbody = -ceqbody;
                ceqgate = -ceqgate;
                ceqbs = -ceqbs;
                ceqbd = -ceqbd;
                ceqqg = -ceqqg;
                ceqqb = -ceqqb;
                ceqqd = -ceqqd;
                ceqqe = -ceqqe;
                cdbdp = - cdbdp; /* v4.0 */
                csbsp = - csbsp; /* v4.0 */


                ceqgcrg = -ceqgcrg;  /* v3.1 */
                if (here->B4SOIrgateMod == 3)
                    ceqqgmid = -ceqqgmid;

                if (here->B4SOIrbodyMod) /* v4.0 */
                {
                    ceqqjs = -ceqqjs;
                    ceqqjd = -ceqqjd;
                }

            }

// SRW - save for interpolation
#define MSGN(x) (model->B4SOItype > 0 ? x : -(x))

            *(ckt->CKTstate0 + here->B4SOIa_cd) = MSGN(here->B4SOIcd);
            *(ckt->CKTstate0 + here->B4SOIa_cbs) = MSGN(here->B4SOIcjs);
            *(ckt->CKTstate0 + here->B4SOIa_cbd) = MSGN(here->B4SOIcjd);
            *(ckt->CKTstate0 + here->B4SOIa_cggb) = MSGN(here->B4SOIcggb);
            *(ckt->CKTstate0 + here->B4SOIa_cgdb) = MSGN(here->B4SOIcgdb);
            *(ckt->CKTstate0 + here->B4SOIa_cgsb) = MSGN(here->B4SOIcgsb);
            *(ckt->CKTstate0 + here->B4SOIa_cdgb) = MSGN(here->B4SOIcdgb);
            *(ckt->CKTstate0 + here->B4SOIa_cddb) = MSGN(here->B4SOIcddb);
            *(ckt->CKTstate0 + here->B4SOIa_cdsb) = MSGN(here->B4SOIcdsb);
            *(ckt->CKTstate0 + here->B4SOIa_cbgb) = MSGN(here->B4SOIcbgb);
            *(ckt->CKTstate0 + here->B4SOIa_cbdb) = MSGN(here->B4SOIcbdb);
            *(ckt->CKTstate0 + here->B4SOIa_cbsb) = MSGN(here->B4SOIcbsb);
            *(ckt->CKTstate0 + here->B4SOIa_von) = MSGN(here->B4SOIvon);
            *(ckt->CKTstate0 + here->B4SOIa_vdsat) = MSGN(here->B4SOIvdsat);

            *(ckt->CKTstate0 + here->B4SOIa_qbulk) = here->B4SOIqbulk;
            *(ckt->CKTstate0 + here->B4SOIa_qdrn) = here->B4SOIqdrn;

            *(ckt->CKTstate0 + here->B4SOIa_gm) = here->B4SOIgm;
            *(ckt->CKTstate0 + here->B4SOIa_gds) = here->B4SOIgds;
            *(ckt->CKTstate0 + here->B4SOIa_gmbs) = here->B4SOIgmbs;
            *(ckt->CKTstate0 + here->B4SOIa_gbd) = here->B4SOIgjdb;
            *(ckt->CKTstate0 + here->B4SOIa_gbs) = here->B4SOIgjsb;
// SRW - end

            /* v3.1 */

            /* v3.1 added ceqgcrg for RF */
            ckt->rhsadd(here->B4SOIgNode, -((ceqgate + ceqqg)
                                                  + Igtoteq - ceqgcrg));
            /* v3.1 added ceqgcrg for RF end */

            ckt->rhsadd(here->B4SOIdNodePrime, (ceqbd - cdreq
                    - ceqqd) + Idtoteq
                    /* v4.0 */                     + ceqgdtot);
            if (!here->B4SOIrbodyMod)
            {
                ckt->rhsadd(here->B4SOIsNodePrime, (cdreq + ceqbs
                        + ceqqg + ceqqb + ceqqd + ceqqe) + Istoteq
                        + ceqqgmid - ceqgstot); /* v4.0 */
            }
            else   /* v4.0 */
            {
                ckt->rhsadd(here->B4SOIsNodePrime, (cdreq + ceqbs
                        + ceqqg + ceqqb + ceqqd + ceqqe) + Istoteq
                        + ceqqgmid + ceqqjd + ceqqjs - ceqgstot);
            }

            ckt->rhsadd(here->B4SOIeNode, -ceqqe);

            if (here->B4SOIrgateMod == 2)
                ckt->rhsadd(here->B4SOIgNodeExt, -ceqgcrg);
            else if (here->B4SOIrgateMod == 3)
                ckt->rhsadd(here->B4SOIgNodeMid, -(ceqqgmid
                        + ceqgcrg));

            if (here->B4SOIbodyMod == 1)
            {
                ckt->rhsadd(here->B4SOIpNode, ceqbodcon);
            }

            if ( here->B4SOIsoiMod != 2 )
            {
                if (!here->B4SOIrbodyMod)
                    ckt->rhsadd(here->B4SOIbNode, -(ceqbody + ceqqb));
                else /* v4.0 */
                {
                    ckt->rhsadd(here->B4SOIdbNode, -(cdbdp + ceqqjd));
                    ckt->rhsadd(here->B4SOIbNode, -(ceqbody + ceqqb));
                    ckt->rhsadd(here->B4SOIsbNode, -(csbsp + ceqqjs));
                }
            }

            if (selfheat)
            {
                ckt->rhsadd(here->B4SOItempNode, -(ceqth + ceqqth));
            }

            if (model->B4SOIrdsMod)
            {
                ckt->rhsadd(here->B4SOIdNode, -ceqgdtot);
                ckt->rhsadd(here->B4SOIsNode, ceqgstot);
            }

            if (here->B4SOIdebugMod != 0)
            {
                *(ckt->CKTrhs + here->B4SOIvbsNode) = here->B4SOIvbseff;
                *(ckt->CKTrhs + here->B4SOIidsNode) = FLOG(here->B4SOIids);
                *(ckt->CKTrhs + here->B4SOIicNode) = FLOG(here->B4SOIic);
                *(ckt->CKTrhs + here->B4SOIibsNode) = FLOG(here->B4SOIibs);
                *(ckt->CKTrhs + here->B4SOIibdNode) = FLOG(here->B4SOIibd);
                *(ckt->CKTrhs + here->B4SOIiiiNode) = FLOG(here->B4SOIiii);
                *(ckt->CKTrhs + here->B4SOIigNode) = here->B4SOIig;
                *(ckt->CKTrhs + here->B4SOIgiggNode) = here->B4SOIgigg;
                *(ckt->CKTrhs + here->B4SOIgigdNode) = here->B4SOIgigd;
                *(ckt->CKTrhs + here->B4SOIgigbNode) = here->B4SOIgigb;
                *(ckt->CKTrhs + here->B4SOIigidlNode) = here->B4SOIigidl;
                *(ckt->CKTrhs + here->B4SOIitunNode) = here->B4SOIitun;
                *(ckt->CKTrhs + here->B4SOIibpNode) = here->B4SOIibp;
                *(ckt->CKTrhs + here->B4SOIcbbNode) = here->B4SOIcbb;
                *(ckt->CKTrhs + here->B4SOIcbdNode) = here->B4SOIcbd;
                *(ckt->CKTrhs + here->B4SOIcbgNode) = here->B4SOIcbg;
                *(ckt->CKTrhs + here->B4SOIqbfNode) = here->B4SOIqbf;
                *(ckt->CKTrhs + here->B4SOIqjsNode) = here->B4SOIqjs;
                *(ckt->CKTrhs + here->B4SOIqjdNode) = here->B4SOIqjd;

            }

            if (!model->B4SOIrdsMod)
            {
                gdpr = here->B4SOIdrainConductance;
                gspr = here->B4SOIsourceConductance;
            }
            else
                gdpr = gspr = 0.0;       /* v4.0 */

// SRW
            if (here->B4SOIadjoint)
            {
                dvaMatrix *mat = here->B4SOIadjoint->matrix;
                mat->clear();

                /* v3.1 added ceqgcrg for RF */
                (*mat->get_elem(here->B4SOIgNode) -= (ceqgate + ceqqg)
                                                     + Igtoteq - ceqgcrg);
                /* v3.1 added ceqgcrg for RF end */

                (*mat->get_elem(here->B4SOIdNodePrime) += (ceqbd - cdreq
                        - ceqqd) + Idtoteq
                        /* v4.0 */                  + ceqgdtot);
                if (!here->B4SOIrbodyMod)
                {
                    (*mat->get_elem(here->B4SOIsNodePrime) += (cdreq + ceqbs
                            + ceqqg + ceqqb + ceqqd + ceqqe) + Istoteq
                            + ceqqgmid - ceqgstot); /* v4.0 */
                }
                else   /* v4.0 */
                {
                    (*mat->get_elem(here->B4SOIsNodePrime) += (cdreq + ceqbs
                            + ceqqg + ceqqb + ceqqd + ceqqe) + Istoteq
                            + ceqqgmid + ceqqjd + ceqqjs - ceqgstot);
                }

                (*mat->get_elem(here->B4SOIeNode) -= ceqqe);

                if (here->B4SOIrgateMod == 2)
                    (*mat->get_elem(here->B4SOIgNodeExt) -= ceqgcrg);
                else if (here->B4SOIrgateMod == 3)
                    (*mat->get_elem(here->B4SOIgNodeMid) -= ceqqgmid
                                                            + ceqgcrg);

                if (here->B4SOIbodyMod == 1)
                {
                    (*mat->get_elem(here->B4SOIpNode) += ceqbodcon);
                }

                if ( here->B4SOIsoiMod != 2 )
                {
                    if (!here->B4SOIrbodyMod)
                        (*mat->get_elem(here->B4SOIbNode) -= (ceqbody + ceqqb));
                    else /* v4.0 */
                    {
                        (*mat->get_elem(here->B4SOIdbNode) -= (cdbdp + ceqqjd));
                        (*mat->get_elem(here->B4SOIbNode) -= (ceqbody + ceqqb));
                        (*mat->get_elem(here->B4SOIsbNode) -= (csbsp + ceqqjs));
                    }
                }

                if (selfheat)
                {
                    (*mat->get_elem(here->B4SOItempNode) -= ceqth + ceqqth);
                }

                if (model->B4SOIrdsMod)
                {
                    (*mat->get_elem(here->B4SOIdNode) -= ceqgdtot);
                    (*mat->get_elem(here->B4SOIsNode) += ceqgstot);
                }

            }
// SRW - end

            /*
             *  load y matrix
             */
            Gmin = ckt->CKTgmin * 1e-6;

            /* v3.1 added for RF */
            geltd = here->B4SOIgrgeltd;
            if (here->B4SOIrgateMod == 1)
            {
                ckt->ldadd(here->B4SOIGEgePtr, geltd);
                ckt->ldadd(here->B4SOIGgePtr, -geltd);
                ckt->ldadd(here->B4SOIGEgPtr, -geltd);
            }
            else if (here->B4SOIrgateMod == 2)
            {
                ckt->ldadd(here->B4SOIGEgePtr, gcrg);
                ckt->ldadd(here->B4SOIGEgPtr, gcrgg);
                ckt->ldadd(here->B4SOIGEdpPtr, gcrgd);
                ckt->ldadd(here->B4SOIGEspPtr, gcrgs);
                ckt->ldadd(here->B4SOIGgePtr, -gcrg);
                if (here->B4SOIsoiMod !=2) /* v3.2 */
                    ckt->ldadd(here->B4SOIGEbPtr, gcrgb);
            }
            else if (here->B4SOIrgateMod == 3)
            {
                ckt->ldadd(here->B4SOIGEgePtr, geltd);
                ckt->ldadd(here->B4SOIGEgmPtr, -geltd);
                ckt->ldadd(here->B4SOIGMgePtr, -geltd);
                ckt->ldadd(here->B4SOIGMgmPtr, geltd + gcrg + gcgmgmb);

                ckt->ldadd(here->B4SOIGMdpPtr, gcrgd + gcgmdb);
                ckt->ldadd(here->B4SOIGMgPtr, gcrgg);
                ckt->ldadd(here->B4SOIGMspPtr, gcrgs + gcgmsb);
                ckt->ldadd(here->B4SOIGMePtr, gcgmeb);
                if (here->B4SOIsoiMod !=2) /* v3.2 */
                    ckt->ldadd(here->B4SOIGMbPtr, gcrgb);

                ckt->ldadd(here->B4SOIDPgmPtr, gcdgmb);
                ckt->ldadd(here->B4SOIGgmPtr, -gcrg);
                ckt->ldadd(here->B4SOISPgmPtr, gcsgmb);
                ckt->ldadd(here->B4SOIEgmPtr, gcegmb);
            }
            /* v3.1 added for RF end*/


            /* v3.0 */
            if (here->B4SOIsoiMod != 0) /* v3.2 */
            {
                ckt->ldadd(here->B4SOIDPePtr, Gme + gddpe);
                ckt->ldadd(here->B4SOISPePtr, gsspe - Gme);

                if (here->B4SOIsoiMod != 2) /* v3.2 */
                {
                    ckt->ldadd(here->B4SOIGePtr, gige);
                    ckt->ldadd(here->B4SOIBePtr, -gige);
                }
            }

            ckt->ldadd(here->B4SOIEdpPtr, gcedb);
            ckt->ldadd(here->B4SOIEspPtr, gcesb);
            ckt->ldadd(here->B4SOIDPePtr, gcdeb);
            ckt->ldadd(here->B4SOISPePtr, gcseb);
            ckt->ldadd(here->B4SOIEgPtr, gcegb);
            ckt->ldadd(here->B4SOIGePtr, gcgeb);

            /* v3.1 */
            if (here->B4SOIsoiMod != 2) /* v3.2 */
            {
                ckt->ldadd(here->B4SOIEbPtr, -(gcegb + gcedb + gcesb + gceeb + gcegmb)); /* 3.2 bug fix */

                /* v3.1 changed GbPtr for RF */
                if ((here->B4SOIrgateMod == 0) || (here->B4SOIrgateMod == 1))
                    ckt->ldadd(here->B4SOIGbPtr, -(-gigb + gcggb + gcgdb + gcgsb
                                            + gcgeb - gIgtotb));
                else /* v3.1 for rgateMod = 2 or 3 */
                    ckt->ldadd(here->B4SOIGbPtr, gigb + gcgbb +gIgtotb - gcrgb);


                ckt->ldadd(here->B4SOIDPbPtr, -(-gddpb - Gmbs - gcdbb + gdtotb
                                         + gIdtotb )); /* v4.0 */

                /*                      ckt->ldadd(here->B4SOIDPbPtr, -((-gddpb - Gmbs + gcdgb + gcddb
                                        + gcdeb + gcdsb) + gcdgmb
                                        + gIdtotb));
                                        */

                ckt->ldadd(here->B4SOISPbPtr, -(-gsspb + Gmbs - gcsbb + gstotb
                                         + Gmin + gIstotb)); /* v4.0 */

                /*                      ckt->ldadd(here->B4SOISPbPtr, -((-gsspb + Gmbs + gcsgb + gcsdb
                                        + gcseb + gcssb) + gcsgmb
                                        + Gmin + gIstotb));
                                        */
                ckt->ldadd(here->B4SOIBePtr, gbbe + gcbeb); /* v3.0 */
                ckt->ldadd(here->B4SOIBgPtr, -gigg + gcbgb + gbbg);
                ckt->ldadd(here->B4SOIBdpPtr, -gigd + gcbdb + gbbdp );

                ckt->ldadd(here->B4SOIBspPtr, gcbsb + gbbsp - Gmin
                                         - gigs);
                /*                    if (!here->B4SOIrbodyMod)
                */
                ckt->ldadd(here->B4SOIBbPtr, -gigb + gbbb - gcbgb - gcbdb
                                        - gcbsb - gcbeb + Gmin) ;
                /*                    else
                                ckt->ldadd(here->B4SOIBbPtr, -gigb - (Giib - Gbpbs) - gcbgb
                                - gcbdb - gcbsb - gcbeb + Gmin) ;
                */
                /* v4.0 */
                if (here->B4SOIrbodyMod)
                {
                    ckt->ldadd(here->B4SOIDPdbPtr, -gcjdbdp - GGjdb);
                    ckt->ldadd(here->B4SOISPsbPtr, -gcjsbsp - GGjsb);
                    ckt->ldadd(here->B4SOIDBdpPtr, -gcjdbdp - GGjdb);
                    ckt->ldadd(here->B4SOIDBdbPtr, gcjdbdp + GGjdb
                                              + here->B4SOIgrbdb);
                    ckt->ldadd(here->B4SOIDBbPtr, -here->B4SOIgrbdb);
                    ckt->ldadd(here->B4SOISBspPtr, -gcjsbsp - GGjsb);
                    ckt->ldadd(here->B4SOISBbPtr, -here->B4SOIgrbsb);
                    ckt->ldadd(here->B4SOISBsbPtr, gcjsbsp + GGjsb
                                              + here->B4SOIgrbsb);
                    ckt->ldadd(here->B4SOIBdbPtr, -here->B4SOIgrbdb);
                    ckt->ldadd(here->B4SOIBsbPtr, -here->B4SOIgrbsb);
                    ckt->ldadd(here->B4SOIBbPtr, here->B4SOIgrbsb
                                            + here->B4SOIgrbdb);
                }
                if (model->B4SOIrdsMod)
                {
                    ckt->ldadd(here->B4SOIDbPtr, gdtotb);
                    ckt->ldadd(here->B4SOISbPtr, gstotb);
                }

            }
            /* v3.1 */
            if (model->B4SOIrdsMod)
            {
                ckt->ldadd(here->B4SOIDgPtr, gdtotg);
                ckt->ldadd(here->B4SOIDspPtr, gdtots);
                ckt->ldadd(here->B4SOISdpPtr, gstotd);
                ckt->ldadd(here->B4SOISgPtr, gstotg);
            }

            ckt->ldadd(here->B4SOIEePtr, gceeb);

            if (here->B4SOIrgateMod == 0)
            {
                ckt->ldadd(here->B4SOIGgPtr, gigg + gcggb + Gmin
                                        + gIgtotg);
                ckt->ldadd(here->B4SOIGdpPtr, gigd + gcgdb - Gmin
                                         + gIgtotd);
                ckt->ldadd(here->B4SOIGspPtr, gcgsb + gigs + gIgtots);
            }
            else if (here->B4SOIrgateMod == 1) /* v3.1 for RF */
            {
                ckt->ldadd(here->B4SOIGgPtr, gigg + gcggb + Gmin
                                       + gIgtotg + geltd);
                ckt->ldadd(here->B4SOIGdpPtr, gigd + gcgdb - Gmin
                                        + gIgtotd);
                ckt->ldadd(here->B4SOIGspPtr, gcgsb + gigs + gIgtots);
            }
            else /* v3.1 for RF rgateMod == 2 or 3 */
            {
                ckt->ldadd(here->B4SOIGgPtr, gigg + gcggb + Gmin
                                       + gIgtotg - gcrgg);
                ckt->ldadd(here->B4SOIGdpPtr, gigd + gcgdb - Gmin
                                        + gIgtotd - gcrgd);
                ckt->ldadd(here->B4SOIGspPtr, gcgsb + gigs + gIgtots - gcrgs);
            }


            ckt->ldadd(here->B4SOIDPgPtr, (Gm + gcdgb) + gddpg - Gmin
                                     - gIdtotg - gdtotg); /* v4.0 */
            ckt->ldadd(here->B4SOIDPdpPtr, (gdpr + here->B4SOIgds + gddpdp
                                       + RevSum + gcddb) + Gmin
                                      - gIdtotd - gdtotd); /* v4.0 */
            ckt->ldadd(here->B4SOIDPspPtr, -((-gddpsp + here->B4SOIgds + FwdSum
                                       - gcdsb) + gIdtots + gdtots));

            ckt->ldadd(here->B4SOIDPdPtr, -(gdpr + gdtot));

            ckt->ldadd(here->B4SOISPgPtr, gcsgb - Gm + gsspg - gIstotg
                                     - gstotg); /* v4.0 */
            ckt->ldadd(here->B4SOISPdpPtr, -((here->B4SOIgds - gsspdp + RevSum
                                       - gcsdb + gIstotd) + gstotd)); /* v4.0 */

            ckt->ldadd(here->B4SOISPspPtr, (gspr - gstots
                                       + here->B4SOIgds + gsspsp
                                       + FwdSum + gcssb)
                                      + Gmin - gIstots); /* v4.0 */

            ckt->ldadd(here->B4SOISPsPtr, -(gspr + gstot));


            ckt->ldadd(here->B4SOIDdPtr,  gdpr + gdtot);
            ckt->ldadd(here->B4SOIDdpPtr, -(gdpr - gdtotd));


            ckt->ldadd(here->B4SOISsPtr, gspr + gstot);
            ckt->ldadd(here->B4SOISspPtr, -(gspr - gstots));


            if (here->B4SOIbodyMod == 1)
            {
                ckt->ldadd(here->B4SOIBpPtr, -gppp);
                ckt->ldadd(here->B4SOIPbPtr, gppb);
                ckt->ldadd(here->B4SOIPpPtr, gppp);
            }

            /* v4.1  Ig_agbcp2 stamping */
            ckt->ldadd(here->B4SOIGgPtr, gigpg);
            if (here->B4SOIbodyMod == 1)
            {
                ckt->ldadd(here->B4SOIPpPtr, -gigpp);
                ckt->ldadd(here->B4SOIPgPtr, -gigpg);
                ckt->ldadd(here->B4SOIGpPtr, gigpp);
            }
            else if(here->B4SOIbodyMod == 2)
            {
                ckt->ldadd(here->B4SOIBbPtr, -gigpp);
                ckt->ldadd(here->B4SOIBgPtr, -gigpg);
                ckt->ldadd(here->B4SOIGbPtr, gigpp);
            }


            if (selfheat)
            {
                ckt->ldadd(here->B4SOIDPtempPtr, GmT + gddpT + gcdT);
                ckt->ldadd(here->B4SOISPtempPtr, -GmT + gsspT + gcsT);
                ckt->ldadd(here->B4SOIBtempPtr, gbbT + gcbT - gigT);
                ckt->ldadd(here->B4SOIEtempPtr, gceT);
                ckt->ldadd(here->B4SOIGtempPtr, gcgT + gigT);
                ckt->ldadd(here->B4SOITemptempPtr, gTtt  + 1/pParam->B4SOIrth + gcTt);
                ckt->ldadd(here->B4SOITempgPtr, gTtg);
                ckt->ldadd(here->B4SOITempbPtr, gTtb);
                ckt->ldadd(here->B4SOITempdpPtr, gTtdp);
                ckt->ldadd(here->B4SOITempspPtr, gTtsp);

                /* v3.0 */
                if (here->B4SOIsoiMod != 0) /* v3.2 */
                    ckt->ldadd(here->B4SOITempePtr, gTte);

            }

            if (here->B4SOIdebugMod != 0)
            {
                ckt->ldadd(here->B4SOIVbsPtr, 1.0);
                ckt->ldadd(here->B4SOIIdsPtr, 1.0);
                ckt->ldadd(here->B4SOIIcPtr, 1.0);
                ckt->ldadd(here->B4SOIIbsPtr, 1.0);
                ckt->ldadd(here->B4SOIIbdPtr, 1.0);
                ckt->ldadd(here->B4SOIIiiPtr, 1.0);
                ckt->ldadd(here->B4SOIIgPtr, 1.0);
                ckt->ldadd(here->B4SOIGiggPtr, 1.0);
                ckt->ldadd(here->B4SOIGigdPtr, 1.0);
                ckt->ldadd(here->B4SOIGigbPtr, 1.0);
                ckt->ldadd(here->B4SOIIgidlPtr, 1.0);
                ckt->ldadd(here->B4SOIItunPtr, 1.0);
                ckt->ldadd(here->B4SOIIbpPtr, 1.0);
                ckt->ldadd(here->B4SOICbgPtr, 1.0);
                ckt->ldadd(here->B4SOICbbPtr, 1.0);
                ckt->ldadd(here->B4SOICbdPtr, 1.0);
                ckt->ldadd(here->B4SOIQbfPtr, 1.0);
                ckt->ldadd(here->B4SOIQjsPtr, 1.0);
                ckt->ldadd(here->B4SOIQjdPtr, 1.0);
            }

// SRW
            if (here->B4SOIadjoint)
            {
                B4SOIadj *adj = here->B4SOIadjoint;

                if (here->B4SOIrgateMod == 1)
                {
                    *(adj->B4SOIGEgePtr) += geltd;
                    *(adj->B4SOIGgePtr) -= geltd;
                    *(adj->B4SOIGEgPtr) -= geltd;
                }
                else if (here->B4SOIrgateMod == 2)
                {
                    *(adj->B4SOIGEgePtr) += gcrg;
                    *(adj->B4SOIGEgPtr) += gcrgg;
                    *(adj->B4SOIGEdpPtr) += gcrgd;
                    *(adj->B4SOIGEspPtr) += gcrgs;
                    *(adj->B4SOIGgePtr) -= gcrg;
                    if (here->B4SOIsoiMod !=2) /* v3.2 */
                        *(adj->B4SOIGEbPtr) += gcrgb;
                }
                else if (here->B4SOIrgateMod == 3)
                {
                    *(adj->B4SOIGEgePtr) += geltd;
                    *(adj->B4SOIGEgmPtr) -= geltd;
                    *(adj->B4SOIGMgePtr) -= geltd;
                    *(adj->B4SOIGMgmPtr) += geltd + gcrg + gcgmgmb;

                    *(adj->B4SOIGMdpPtr) += gcrgd + gcgmdb;
                    *(adj->B4SOIGMgPtr) += gcrgg;
                    *(adj->B4SOIGMspPtr) += gcrgs + gcgmsb;
                    *(adj->B4SOIGMePtr) += gcgmeb;
                    if (here->B4SOIsoiMod !=2) /* v3.2 */
                        *(adj->B4SOIGMbPtr) += gcrgb;

                    *(adj->B4SOIDPgmPtr) += gcdgmb;
                    *(adj->B4SOIGgmPtr) -= gcrg;
                    *(adj->B4SOISPgmPtr) += gcsgmb;
                    *(adj->B4SOIEgmPtr) += gcegmb;
                }
                /* v3.1 added for RF end*/


                /* v3.0 */
                if (here->B4SOIsoiMod != 0) /* v3.2 */
                {
                    (*(adj->B4SOIDPePtr) += Gme + gddpe);
                    (*(adj->B4SOISPePtr) += gsspe - Gme);

                    if (here->B4SOIsoiMod != 2) /* v3.2 */
                    {
                        *(adj->B4SOIGePtr) += gige;
                        *(adj->B4SOIBePtr) -= gige;
                    }
                }

                *(adj->B4SOIEdpPtr) += gcedb;
                *(adj->B4SOIEspPtr) += gcesb;
                *(adj->B4SOIDPePtr) += gcdeb;
                *(adj->B4SOISPePtr) += gcseb;
                *(adj->B4SOIEgPtr) += gcegb;
                *(adj->B4SOIGePtr) += gcgeb;

                /* v3.1 */
                if (here->B4SOIsoiMod != 2) /* v3.2 */
                {
                    (*(adj->B4SOIEbPtr) -= gcegb + gcedb + gcesb + gceeb + gcegmb); /* 3.2 bug fix */

                    /* v3.1 changed GbPtr for RF */
                    if ((here->B4SOIrgateMod == 0) || (here->B4SOIrgateMod == 1))
                        (*(adj->B4SOIGbPtr) -= -gigb + gcggb + gcgdb + gcgsb
                                               + gcgeb - gIgtotb);
                    else /* v3.1 for rgateMod = 2 or 3 */
                        *(adj->B4SOIGbPtr) += gigb + gcgbb +gIgtotb - gcrgb;


                    (*(adj->B4SOIDPbPtr) -= -gddpb - Gmbs - gcdbb + gdtotb
                                            + gIdtotb ); /* v4.0 */

                    /*                      (*(adj->B4SOIDPbPtr) -= (-gddpb - Gmbs + gcdgb + gcddb
                                             + gcdeb + gcdsb) + gcdgmb
                                             + gIdtotb );
                    */

                    (*(adj->B4SOISPbPtr) -= -gsspb + Gmbs - gcsbb + gstotb
                                            + Gmin + gIstotb); /* v4.0 */

                    /*                      (*(adj->B4SOISPbPtr) -= (-gsspb + Gmbs + gcsgb + gcsdb
                                             + gcseb + gcssb) + gcsgmb
                                             + Gmin + gIstotb);
                    */
                    (*(adj->B4SOIBePtr) += gbbe + gcbeb); /* v3.0 */
                    (*(adj->B4SOIBgPtr) += -gigg + gcbgb + gbbg);
                    (*(adj->B4SOIBdpPtr) += -gigd + gcbdb + gbbdp );

                    (*(adj->B4SOIBspPtr) += gcbsb + gbbsp - Gmin
                                            - gigs);
                    /*                    if (!here->B4SOIrbodyMod)
                    */
                    (*(adj->B4SOIBbPtr) += -gigb + gbbb - gcbgb - gcbdb
                                           - gcbsb - gcbeb + Gmin) ;
                    /*                    else
                                  (*(adj->B4SOIBbPtr) += -gigb - (Giib - Gbpbs) - gcbgb
                                   - gcbdb - gcbsb - gcbeb + Gmin) ;
                    */
                    /* v4.0 */
                    if (here->B4SOIrbodyMod)
                    {
                        (*(adj->B4SOIDPdbPtr) += -gcjdbdp - GGjdb);
                        (*(adj->B4SOISPsbPtr) += -gcjsbsp - GGjsb);
                        (*(adj->B4SOIDBdpPtr) += -gcjdbdp - GGjdb);
                        (*(adj->B4SOIDBdbPtr) += gcjdbdp + GGjdb
                                                 + here->B4SOIgrbdb);
                        (*(adj->B4SOIDBbPtr) -= here->B4SOIgrbdb);
                        (*(adj->B4SOISBspPtr) += -gcjsbsp - GGjsb);
                        (*(adj->B4SOISBbPtr) -= here->B4SOIgrbsb);
                        (*(adj->B4SOISBsbPtr) += gcjsbsp + GGjsb
                                                 + here->B4SOIgrbsb);
                        (*(adj->B4SOIBdbPtr) -= here->B4SOIgrbdb);
                        (*(adj->B4SOIBsbPtr) -= here->B4SOIgrbsb);
                        (*(adj->B4SOIBbPtr) += here->B4SOIgrbsb
                                               + here->B4SOIgrbdb);
                    }
                    if (model->B4SOIrdsMod)
                    {
                        (*(adj->B4SOIDbPtr) += gdtotb);
                        (*(adj->B4SOISbPtr) += gstotb);
                    }

                }
                /* v3.1 */
                if (model->B4SOIrdsMod)
                {
                    (*(adj->B4SOIDgPtr) += gdtotg);
                    (*(adj->B4SOIDspPtr) += gdtots);
                    (*(adj->B4SOISdpPtr) += gstotd);
                    (*(adj->B4SOISgPtr) += gstotg);
                }

                (*(adj->B4SOIEePtr) += gceeb);

                if (here->B4SOIrgateMod == 0)
                {
                    (*(adj->B4SOIGgPtr) += gigg + gcggb + Gmin
                                           + gIgtotg);
                    (*(adj->B4SOIGdpPtr) += gigd + gcgdb - Gmin
                                            + gIgtotd);
                    (*(adj->B4SOIGspPtr) += gcgsb + gigs + gIgtots);
                }
                else if (here->B4SOIrgateMod == 1) /* v3.1 for RF */
                {
                    *(adj->B4SOIGgPtr) += gigg + gcggb + Gmin
                                          + gIgtotg + geltd;
                    *(adj->B4SOIGdpPtr) += gigd + gcgdb - Gmin
                                           + gIgtotd;
                    *(adj->B4SOIGspPtr) += gcgsb + gigs + gIgtots;
                }
                else /* v3.1 for RF rgateMod == 2 or 3 */
                {
                    *(adj->B4SOIGgPtr) += gigg + gcggb + Gmin
                                          + gIgtotg - gcrgg;
                    *(adj->B4SOIGdpPtr) += gigd + gcgdb - Gmin
                                           + gIgtotd - gcrgd;
                    *(adj->B4SOIGspPtr) += gcgsb + gigs + gIgtots - gcrgs;
                }


                (*(adj->B4SOIDPgPtr) += (Gm + gcdgb) + gddpg - Gmin
                                        - gIdtotg - gdtotg); /* v4.0 */
                (*(adj->B4SOIDPdpPtr) += (gdpr + here->B4SOIgds + gddpdp
                                          + RevSum + gcddb) + Gmin
                                         - gIdtotd - gdtotd); /* v4.0 */
                (*(adj->B4SOIDPspPtr) -= (-gddpsp + here->B4SOIgds + FwdSum
                                          - gcdsb) + gIdtots + gdtots);

                (*(adj->B4SOIDPdPtr) -= gdpr + gdtot);

                (*(adj->B4SOISPgPtr) += gcsgb - Gm + gsspg - gIstotg
                                        - gstotg); /* v4.0 */
                (*(adj->B4SOISPdpPtr) -= (here->B4SOIgds - gsspdp + RevSum
                                          - gcsdb + gIstotd) + gstotd); /* v4.0 */

                (*(adj->B4SOISPspPtr) += (gspr - gstots
                                          + here->B4SOIgds + gsspsp
                                          + FwdSum + gcssb)
                                         + Gmin - gIstots); /* v4.0 */

                (*(adj->B4SOISPsPtr) -= gspr + gstot);


                (*(adj->B4SOIDdPtr) += gdpr + gdtot);
                (*(adj->B4SOIDdpPtr) -= gdpr - gdtotd);


                (*(adj->B4SOISsPtr) += gspr + gstot);
                (*(adj->B4SOISspPtr) -= gspr - gstots);


                if (here->B4SOIbodyMod == 1)
                {
                    (*(adj->B4SOIBpPtr) -= gppp);
                    (*(adj->B4SOIPbPtr) += gppb);
                    (*(adj->B4SOIPpPtr) += gppp);
                }

                /* v4.1  Ig_agbcp2 stamping */
                (*(adj->B4SOIGgPtr) += gigpg);
                if (here->B4SOIbodyMod == 1)
                {
                    (*(adj->B4SOIPpPtr) -= gigpp);
                    (*(adj->B4SOIPgPtr) -= gigpg);
                    (*(adj->B4SOIGpPtr) += gigpp);
                }
                else if(here->B4SOIbodyMod == 2)
                {
                    (*(adj->B4SOIBbPtr) -= gigpp);
                    (*(adj->B4SOIBgPtr) -= gigpg);
                    (*(adj->B4SOIGbPtr) += gigpp);
                }

                if (selfheat)
                {
                    (*(adj->B4SOIDPtempPtr) += GmT + gddpT + gcdT);
                    (*(adj->B4SOISPtempPtr) += -GmT + gsspT + gcsT);
                    (*(adj->B4SOIBtempPtr) += gbbT + gcbT - gigT);
                    (*(adj->B4SOIEtempPtr) += gceT);
                    (*(adj->B4SOIGtempPtr) += gcgT + gigT);
                    (*(adj->B4SOITemptempPtr) += gTtt  + 1/pParam->B4SOIrth + gcTt);
                    (*(adj->B4SOITempgPtr) += gTtg);
                    (*(adj->B4SOITempbPtr) += gTtb);
                    (*(adj->B4SOITempdpPtr) += gTtdp);
                    (*(adj->B4SOITempspPtr) += gTtsp);

                    /* v3.0 */
                    if (here->B4SOIsoiMod != 0) /* v3.2 */
                        (*(adj->B4SOITempePtr) += gTte);

                }

                if (here->B4SOIdebugMod != 0)
                {
                    *(adj->B4SOIVbsPtr) += 1;
                    *(adj->B4SOIIdsPtr) += 1;
                    *(adj->B4SOIIcPtr) += 1;
                    *(adj->B4SOIIbsPtr) += 1;
                    *(adj->B4SOIIbdPtr) += 1;
                    *(adj->B4SOIIiiPtr) += 1;
                    *(adj->B4SOIIgPtr) += 1;
                    *(adj->B4SOIGiggPtr) += 1;
                    *(adj->B4SOIGigdPtr) += 1;
                    *(adj->B4SOIGigbPtr) += 1;
                    *(adj->B4SOIIgidlPtr) += 1;
                    *(adj->B4SOIItunPtr) += 1;
                    *(adj->B4SOIIbpPtr) += 1;
                    *(adj->B4SOICbgPtr) += 1;
                    *(adj->B4SOICbbPtr) += 1;
                    *(adj->B4SOICbdPtr) += 1;
                    *(adj->B4SOIQbfPtr) += 1;
                    *(adj->B4SOIQjsPtr) += 1;
                    *(adj->B4SOIQjdPtr) += 1;
                }

                *(ckt->CKTstate0 + here->B4SOIa_id) =
                    adj->matrix->compute_real(here->B4SOIdNode, ckt->CKTrhsOld);
                *(ckt->CKTstate0 + here->B4SOIa_is) =
                    adj->matrix->compute_real(here->B4SOIsNode, ckt->CKTrhsOld);
                *(ckt->CKTstate0 + here->B4SOIa_ig) =
                    adj->matrix->compute_real(here->B4SOIgNode, ckt->CKTrhsOld);
                *(ckt->CKTstate0 + here->B4SOIa_ib) =
                    adj->matrix->compute_real(here->B4SOIbNode, ckt->CKTrhsOld);
                *(ckt->CKTstate0 + here->B4SOIa_ie) =
                    adj->matrix->compute_real(here->B4SOIeNode, ckt->CKTrhsOld);
            }
// SRW - end

line1000:
            ;


// SRW        }  /* End of Mosfet Instance */
// SRW    }   /* End of Model Instance */

    return(OK);
}

