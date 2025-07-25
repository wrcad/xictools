
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

/***********************************************************************
 HiSIM (Hiroshima University STARC IGFET Model)
 Copyright (C) 2003 STARC

 VERSION : HiSIM 1.2.0
 FILE : hsm1eval120.c of HiSIM 1.2.0

 April 9, 2003 : released by STARC Physical Design Group
***********************************************************************/

/*********************************************************************
* Memorandum on programming
* 
* (1) Bias (x: b|d|g)
*     . sIN.vxs : Input argument.
*     . Vxse: External bias taking account device type (pMOS->nMOS).
*     . Vxsc: Confined bias within a specified region. 
*     . Vxs : Internal bias taking account Rs/Rd.
*     . Y_dVxs denotes the partial derivative of Y w.r.t. Vxs.
* 
* (2) Device Mode
*     . Normal mode (Vds>0 for nMOS) is assumed.
*     . In case of reverse mode, parent routines have to properly 
*       transform or interchange inputs and outputs except ones 
*       related to junction diodes, which are regarded as being 
*       fixed to the nodal S/D.
*
* (3) Modification for symmetry at Vds=0
*     . Vxsz: Modified bias.
*     . Ps0z: Modified Ps0.
*     . The following variables are calculated as a function of 
*       modified biases or potential.
*         Tox, Cox, (-- with quantum effect)
*         Vth*, dVth*, dPpg, Qnm, Qbm, Igate, Igidl, Igisl. 
*     . The following variables are calculated using a transform
*       function.
*         Lred, rp1(<-sIN.rpock1).
*     
* (4) Zones and Cases (terminology)
* 
*       Chi:=beta*(Ps0-Vbs)=       0    3    5
*
*                      Zone:    A  | D1 | D2 | D3
*                                  |
*                    (accumulation)|(depletion)
*                                  |
*                      Vgs =     Vgs_fb                Vth
*                                              /       /
*                      Case:    Nonconductive / Conductive
*                                            /
*             VgVt:=Qn0/Cox=             VgVt_small
*
*     . Ids is regarded as zero in zone-A and -D1.
*     . Procedure to calculate Psl and dependent variables is 
*       omitted in the nonconductive case. Ids and Qi are regarded
*       as zero in this case.
*
*********************************************************************/

/*===========================================================*
* Preamble.
*=================*/
/*---------------------------------------------------*
* Header files.
*-----------------*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
/* SRW
#ifdef __STDC__
# include <ieeefp.h>
#endif
*/

/* SRW */
#ifdef WIN32
#define finite(x) (!_isnan(x))
#else
#ifdef __APPLE__
#define finite isfinite
#endif
#endif

/*-----------------------------------*
* HiSIM macros and structures.
* - All inputs and outputs are defined here.
*-----------------*/
#include "hisim.h"
#include "hsm1evalenv.h"

#ifdef __STDC__
/*-----------------------------------*
 * Constants for Smoothing functions
 *---------------*/
static const double Vgsz_pol_sat = 2.0 ;
static const double Vdsz_pol_sat = 3.5 ;
static const double Vgsz_qme_sat = 2.0 ;
static const double Vdsz_lp_sat = 3.5 ;
static const double Vdsz_sc_sat = 3.5 ;
static const double pol_vgs_dlt = 2.0e-2 ;
static const double pol_vds_dlt = 2.0e-2 ;
static const double qme_vgs_dlt = 9.5e-3 ;
static const double lp_vds_dlt = 2.0e-2 ;
static const double sc_vds_dlt = 2.0e-2 ;
#endif

/*===========================================================*
* Function hsm1eval.
*=================*/
#ifdef __STDC__
int HSM1evaluate120
(
 HiSIM_input     sIN,
 HiSIM_output    *pOT,
 HiSIM_messenger *pMS 
) 
#else
int HSM1evaluate120( sIN , pOT , pMS ) 
     HiSIM_input     sIN ;
     HiSIM_output    *pOT ;
     HiSIM_messenger *pMS ;
#endif
{
    (void)pMS;

#ifndef __STDC__
/*-----------------------------------*
 * Constants for Smoothing functions
 *---------------*/
static double Vgsz_pol_sat = 2.0 ;
static double Vdsz_pol_sat = 3.5 ;
static double Vgsz_qme_sat = 2.0 ;
static double Vdsz_lp_sat = 3.5 ;
static double Vdsz_sc_sat = 3.5 ;
static double pol_vgs_dlt = 2.0e-2 ;
static double pol_vds_dlt = 2.0e-2 ;
static double qme_vgs_dlt = 9.5e-3 ;
static double lp_vds_dlt = 2.0e-2 ;
static double sc_vds_dlt = 2.0e-2 ;
#endif

/*---------------------------------------------------*
* Local variables. 
*-----------------*/
/* Constans ----------------------- */
int     lp_s0_max = 20 ;
int     lp_sl_max = 20 ;
int     lp_bs_max = 10 ;
double  Ids_tol = 1.0e-10 ;
double  Ids_maxvar = 1.0e-1 ;
double  dP_max  = 0.1e0 ;
double  ps_conv = 5.0e-13 ;
double  gs_conv = 1.0e-8 ;
/** depletion **/
double  znbd3 = 3.0e0 ;
double  znbd5 = 5.0e0 ;
double  cn_nc3 = C_SQRT_2 / 108e0 ;
/* 5-degree, contact:Chi=5 */
double  cn_nc51 =  0.707106781186548 ; /* sqrt(2)/2 */
double  cn_nc52 = -0.117851130197758 ; /* -sqrt(2)/12 */
double  cn_nc53 =  0.0178800506338833 ; /* (187 - 112*sqrt(2))/1600 */
double  cn_nc54 = -0.00163730162779191 ; /* (-131 + 88*sqrt(2))/4000 */
double  cn_nc55 =  6.36964918866352e-5 ; /* (1509-1040*sqrt(2))/600000 */
/** inversion **/
/* 3-degree polynomial approx for ( exp[Chi]-1 )^{1/2} */
double  cn_im53 =  2.9693154855770998e-1 ;
double  cn_im54 = -7.0536542840097616e-2 ;
double  cn_im55 =  6.1152888951331797e-3 ;
/* 3-degree polynomial approx for ( exp[Chi]-Chi-1 )^{1/2} */
//double  cn_ik53 =  2.6864599830664019e-1 ;
//double  cn_ik54 = -6.1399531828413338e-2 ;
//double  cn_ik55 =  5.3528499428744690e-3 ;
/** initial guess **/
double  c_ps0ini_2 = 8.0e-4 ;
double  c_pslini_1 = 0.3e0 ;
double  c_pslini_2 = 3.0e-2 ;
double  VgVt_small = 1.0e-12 ;
double  Vbs_max = 0.8e0 ; 
double  Vbs_min = -10.5e0 ;
double  Vds_max = 10.5e0 ; 
double  Vgs_max = 10.5e0 ; 
double  Vbd_max = 0.8e0 ;
double  Vbd_min = -10.5e0 ;
//double  Vsd_max = 10.5e0 ; 
double  Vgd_max = 10.5e0 ; 
double  epsm10 = 10.0e0 * C_EPS_M ;
double  small = 1.0e-50 ;
//double  Vz_dlt = 5.0e-3 ;
double  Gdsmin = 0.0 ;
double  Gjmin = sIN.gmin ;
double  cclmmdf = 1.0e-1 ;
double  qme_dlt = 1.0e-9 ;
double  eef_dlt = 1.0e-2 ;
double	sti1_dlt = -3.0e-3 ;
double  sti2_dlt = 2.0e-3 ;
//double  pol_dlt = 2.0e-1 ;
double  pol2_dlt = 1.0e-5 ;
double  pol_tmp = 1.0 ;

/* Internal flags  --------------------*/
int     flg_err = 0 ; /* error level */
//int     flg_ncnv = 0 ; /* Flag for negative conductance */
int     flg_rsrd ; /* Flag for bias loop accounting Rs and Rd */
int     flg_iprv ; /* Flag for initial guess of Ids */
int     flg_pprv ; /* Flag for initial guesses of Ps0 and Pds */
int     flg_noqi ; /* Flag for the cases regarding Qi=Qd=0 */
int     flg_vbsc = 0 ; /* Flag for Vbs confining */
int     flg_vdsc = 0 ; /* Flag for Vds confining */
int     flg_vgsc = 0 ; /* Flag for Vgs confining */
int     flg_vbdc = 0 ; /* Flag for Vbd confining */
int     flg_vsdc = 0 ; /* Flag for Vsd confining */
int     flg_vgdc = 0 ; /* Flag for Vgd confining */
int     flg_vxxc = 0 ; /* Flag whether some bias was confined */
int     flg_info = 0 ; 
int     flg_clamp = 0 ; /* Flag for clamping biases */

/* flags for smoothing (take account of smoothing or not) ----------*/
int     flg_smoothing_qme_vg = 1 ; /* Flag for smoothing vgs for QM */
int     flg_smoothing_pl_vd  = 1 ; /* Flag for smoothing vds for LP */
int     flg_smoothing_sc_vd  = 1 ; /* Flag for smoothing vds for SC */
int     flg_smoothing_pde_vg = 1 ; /* Flag for smoothing vgs for PDE */
int     flg_smoothing_pde_vd = 1 ; /* Flag for smoothing vds for PDE */

/* flag for D reference calc (GISL(normal), GIDL(reverse)) */
int     flg_gidsl_d = 0 ; 

/* Important Variables in HiSIM -------*/
/* external bias */
double  Vbse , Vdse , Vgse ;
double  Vbde , Vsde , Vgde ;
/* confine bias */
double  Vbsc=0.0 , Vdsc , Vgsc ;
double  Vbdc , Vsdc , Vgdc ;
double  Vbsc_dVbse = 1.0 ;
double  Vbdc_dVbde = 1.0 ;
/* internal bias */
double  Vbs , Vds , Vgs ;
double  Vbs_dVbse = 1.0 , Vbs_dVdse = 0.0 , Vbs_dVgse = 0.0 ;
double  Vds_dVbse = 0.0 , Vds_dVdse = 1.0 , Vds_dVgse = 0.0 ;
double  Vgs_dVbse = 0.0 , Vgs_dVdse = 0.0 , Vgs_dVgse = 1.0 ;
double  Vgp ;
double  Vgp_dVbs , Vgp_dVds , Vgp_dVgs ;
double  Vgs_fb ;
double  Vbd , Vsd , Vgd ;
double  Vbd_dVbde = 1.0 , Vbd_dVsde = 0.0 , Vbd_dVgde = 0.0 ;
double  Vsd_dVbde = 0.0 , Vsd_dVsde = 1.0 , Vsd_dVgde = 0.0 ;
double  Vgd_dVbde = 0.0 , Vgd_dVsde = 0.0 , Vgd_dVgde = 1.0 ;
/* Ps0 : surface potential at the source side */
double  Ps0 ;
double  Ps0_dVbs , Ps0_dVds , Ps0_dVgs ;
double  Ps0_ini , Ps0_iniA , Ps0_iniB ;
/* Psl : surface potential at the drain side */
double  Psl ;
double  Psl_dVbs , Psl_dVds , Psl_dVgs ;
double  Psl_lim ;
/* Pds := Psl - Ps0 */
double  Pds=0.0 ;
double  Pds_dVbs=0.0 , Pds_dVds=0.0 , Pds_dVgs=0.0 ;
double  Pds_ini ;
double  Pds_max ;
/* iteration numbers of Ps0 and Psl equations. */
int     lp_s0 , lp_sl ;
/* Xi0 := beta * ( Ps0 - Vbs ) - 1. */
double  Xi0 ;
double  Xi0_dVbs , Xi0_dVds , Xi0_dVgs ;
double  Xi0p12 ;
double  Xi0p12_dVbs , Xi0p12_dVds , Xi0p12_dVgs ;
double  Xi0p32 ;
//double  Xi0p32_dVbs , Xi0p32_dVds , Xi0p32_dVgs ;
/* Xil := beta * ( Psl - Vbs ) - 1. */
double  Xilp12 ;
double  Xilp32 ;
double  Xil ;
/* modified bias and potential for sym.*/
double  Vbsz , Vdsz , Vgsz ;
double  Vbsz_dVbs , Vbsz_dVds ;
double  Vdsz_dVds ;
//double  Vdsz_dVgs = 0.0 ;
//double  Vdsz_dVbs = 0.0 ;
double  Vgsz_dVgs , Vgsz_dVds ;
//double  Vgsz_dVbs = 0.0 ;
double  Vbs1 , Vbs2 , Vbsd ;
//double  Vbsd_dVbs , Vbsd_dVds ;
double  Vzadd , Vzadd_dVds , Vzadd_dVsd /*, Vzadd_dA*/ ;
//double  VzaddA , VzaddA_dVds ;
double  Ps0z , Ps0z_dVbs , Ps0z_dVds , Ps0z_dVgs ;
double  Pzadd , Pzadd_dVbs , Pzadd_dVds , Pzadd_dVgs ;
double  Ps0Vbsz , Ps0Vbsz_dVbs , Ps0Vbsz_dVds , Ps0Vbsz_dVgs ;
double  Vgpz , Vgpz_dVbs , Vgpz_dVds , Vgpz_dVgs ;
double  Xi0z ;
double  Xi0z_dVbs , Xi0z_dVds , Xi0z_dVgs ;
double  Xi0zp12 ;
double  Xi0zp12_dVbs , Xi0zp12_dVds , Xi0zp12_dVgs ;
double  Vbdz , Vsdz , Vgdz ;
double  Vbdz_dVbd , Vbdz_dVsd /*, Vbdz_dVgd*/ ;
double  Vsdz_dVsd ;
//double  Vsdz_dVgd = 0.0 ;
//double  Vsdz_dVbd = 0.0 ;
double  Vgdz_dVgd , Vgdz_dVsd ;
//double  Vgdz_dVbd = 0.0 ;
//double  Vbdzm ; 
//double  Vbdzm_dVb , Vbdzm_dVs ;
double  Vbd1 , Vbd2 , Vbdd ;
//double  Vbdd_dVbd , Vbdd_dVsd ;
/* Chi := beta * ( Ps{0/l} - Vbs ) */
double  Chi ;
double  Chi_dVbs , Chi_dVds , Chi_dVgs ;
/* Rho := beta * ( Psl - Vds ) */
double  Rho ;
/* threshold voltage */
double  Vth ;
double  Vth_dVb , Vth_dVd /*, Vth_dVs*/ , Vth_dVg ;
double  Vth0 ;
double  Vth0_dVb , Vth0_dVd , Vth0_dVs , Vth0_dVg ;
/* variation of threshold voltage */
double  dVth ;
double  dVth_dVb , dVth_dVd , dVth_dVg ;
double  dVth0 ;
double  dVth0_dVb , dVth0_dVd , dVth0_dVs , dVth0_dVg ;
double  dVthSC ;
double  dVthSC_dVb , dVthSC_dVd , dVthSC_dVs , dVthSC_dVg ;
double  delta0 = 1.0e-5 ;
double  Vgpa , Psi_a , Pb20a , Pb20b , cnst3 ;
double  Psi_a_dVg, Pb20a_dVg, Pb20b_dVg ;
double  dVthW ;
double  dVthW_dVb , dVthW_dVd , dVthW_dVs , dVthW_dVg ;
double  dVthP ;
double  dVthP_dVb , dVthP_dVs , dVthP_dVg ;
/* Alpha and related parameters */
double  Alpha ;
double  Alpha_dVbs , Alpha_dVds , Alpha_dVgs ;
double  Achi ;
double  Achi_dVbs , Achi_dVds , Achi_dVgs ;
double  VgVt = 0.0 ;
double  VgVt_dVbs , VgVt_dVds , VgVt_dVgs ;
double  Delta  , Vdsat ;
/* Q_B and capacitances */
double  Qb , Qb_dVbs , Qb_dVds , Qb_dVgs ;
double  Qb_dVbse , Qb_dVdse , Qb_dVgse ;
/* Q_I and capacitances */
double  Qi , Qi_dVbs , Qi_dVds , Qi_dVgs ;
double  Qi_dVbse , Qi_dVdse , Qi_dVgse ;
/* Q_D and capacitances */
double  Qd , Qd_dVbs , Qd_dVds , Qd_dVgs ;
double  Qd_dVbse , Qd_dVdse , Qd_dVgse ;
/* channel current */
double  Ids ;
double  Ids_dVbs , Ids_dVds , Ids_dVgs ;
double  Ids_dVbse , Ids_dVdse , Ids_dVgse ;
double  Ids0 ;
double  Ids0_dVbs , Ids0_dVds , Ids0_dVgs ;
double  Isd_dVbde , Isd_dVsde , Isd_dVgde ;
/* STI */
double  Vgssti ;
double  Vgssti_dVbs , Vgssti_dVds , Vgssti_dVgs ;
double  costi0 , costi1 /*, costi2*/ , costi3 ;
double  costi4 , costi5 , costi6 , costi7 ;
double  Psasti ;
double  Psasti_dVbs , Psasti_dVds , Psasti_dVgs ;
double  Asti ;
double  Psbsti ;
double  Psbsti_dVbs , Psbsti_dVds , Psbsti_dVgs ;
double  Psab ;
double  Psab_dVbs , Psab_dVds , Psab_dVgs ;
double  Psti ;
double  Psti_dVbs , Psti_dVds , Psti_dVgs ;
double  expsti ;
double  sq1sti ;
double  sq1sti_dVbs , sq1sti_dVds , sq1sti_dVgs ;
double  sq2sti ;
double  sq2sti_dVbs , sq2sti_dVds , sq2sti_dVgs ;
double  Qn0sti ;
double  Qn0sti_dVbs , Qn0sti_dVds , Qn0sti_dVgs ;
double  Idssti ;
double  Idssti_dVbs , Idssti_dVds , Idssti_dVgs ;
/* (for debug) */
//double  user1 , user2 , user3 , user4 ;
/* constants */
double  beta ;
double  beta2 ;
double  Leff /*, Leff_inv*/ ;
double  Weff ;
double  Ldby ;
double  Nsub , q_Nsub ;
double  Nin ;
double  Pb2 ;
double  Pb20 ;
double  Pb2c ;
double  Eg , Eg300 ;
double  Vfb ;
/* PART-1 */
double  Psum ;
double  Psum_dVb ;
double  Psum_dVd ;
double  Psum_dVs ;
double  sqrt_Psum ;
double  cnst0 , cnst1 ;
double  fac1 ;
double  fac1_dVbs , fac1_dVds , fac1_dVgs ;
double  fac1p2 ;
double  fs01 ;
double  fs01_dPs0 , fs01_dChi ;
double  fs01_dVbs , fs01_dVds , fs01_dVgs ;
double  fs02 ;
double  fs02_dPs0 , fs02_dChi ;
double  fs02_dVbs , fs02_dVds , fs02_dVgs ;
double  fsl1 ;
double  fsl1_dPsl ;
double  fsl1_dVbs , fsl1_dVds ;
double  fsl2 ;
double  fsl2_dPsl ;
double  fsl2_dVbs , fsl2_dVds ;
double  cfs1 ;
double  fb , fb_dChi ;
double  fi , fi_dChi ;
double  exp_Chi , exp_Rho , exp_bVbs , exp_bVbsVds ;
double  Fs0, Fsl ;
double  Fs0_dPs0 , Fsl_dPsl ;
double  dPs0 , dPsl ;
double  Qn0=0.0 ;
double  Qn0_dVbs , Qn0_dVds , Qn0_dVgs ;
double  Qb0 ;
double  Qb0_dVb , Qb0_dVd , Qb0_dVs ,Qb0_dVg=0.0 ;
double  Qn00=0.0 ;
double  Qn00_dVbs=0.0 , Qn00_dVds=0.0 , Qn00_dVgs=0.0 ;
double  Qbnm ;
double  Qbnm_dVbs , Qbnm_dVds , Qbnm_dVgs ;
double  DtPds ;
double  DtPds_dVbs , DtPds_dVds , DtPds_dVgs ;
//double  Fid2 ;
//double  Fid2_dVbs , Fid2_dVds , Fid2_dVgs ;
//double  Fid3 ;
//double  Fid3_dVbs , Fid3_dVds , Fid3_dVgs ;
//double  Fid4 ;
//double  Fid4_dVbs , Fid4_dVds , Fid4_dVgs ;
//double  Fid5 ;
//double  Fid5_dVbs , Fid5_dVds , Fid5_dVgs ;
double  Qbm ;
double  Qbm_dVbs , Qbm_dVds , Qbm_dVgs ;
double  Qinm ;
double  Qinm_dVbs , Qinm_dVds , Qinm_dVgs ;
double  Qidn ;
double  Qidn_dVbs , Qidn_dVds , Qidn_dVgs ;
double  Qdnm ;
double  Qdnm_dVbs , Qdnm_dVds , Qdnm_dVgs ;
double  Qddn ;
double  Qddn_dVbs , Qddn_dVds , Qddn_dVgs ;
double  Quot ;
double  Qdrat ;
double  Qdrat_dVbs , Qdrat_dVds , Qdrat_dVgs ;
double  Idd ;
double  Idd_dVbs , Idd_dVds , Idd_dVgs ;
double  Qnm ;
double  Qnm_dVbs , Qnm_dVds , Qnm_dVgs ;
double  Fdd ;
double  Fdd_dVbs , Fdd_dVds , Fdd_dVgs ;
double  Eeff ;
double  Eeff_dVbs , Eeff_dVds , Eeff_dVgs ;
double  Rns ;
double  Mu=0.0 ;
double  Mu_dVbs , Mu_dVds , Mu_dVgs ;
double  Muun , Muun_dVbs , Muun_dVds , Muun_dVgs ;
double  Ey=0.0 ;
double  Ey_dVbs , Ey_dVds , Ey_dVgs ;
double  Em ;
double  Em_dVbs , Em_dVds , Em_dVgs ;
double  Vmax ;
double  Eta ;
double  Eta_dVbs , Eta_dVds , Eta_dVgs ;
double  Eta1 , Eta1p12 , Eta1p32 , Eta1p52 ;
double  Zeta12 , Zeta32 , Zeta52 ;
double  F00 ;
double  F00_dVbs , F00_dVds , F00_dVgs ;
double  F10 ;
double  F10_dVbs , F10_dVds , F10_dVgs ;
double  F30 ;
double  F30_dVbs , F30_dVds , F30_dVgs ;
double  F11 ;
double  F11_dVbs , F11_dVds , F11_dVgs ;
double  Ps0_min ;
double  Acn , Acd , Ac1 , Ac2 , Ac3 , Ac4 , Ac31 , Ac41 ;
double  Acn_dVbs , Acn_dVds , Acn_dVgs ;
double  Acd_dVbs , Acd_dVds , Acd_dVgs ;
double  Ac1_dVbs , Ac1_dVds , Ac1_dVgs ;
double  Ac2_dVbs , Ac2_dVds , Ac2_dVgs ;
double  Ac3_dVbs , Ac3_dVds , Ac3_dVgs ;
double  Ac4_dVbs , Ac4_dVds , Ac4_dVgs ;
double  Ac31_dVbs , Ac31_dVds , Ac31_dVgs ;
/* PART-2 (Isub) */
double  Isub ;
double  Isub_dVbs , Isub_dVds , Isub_dVgs ;
double  Isub_dVbse , Isub_dVdse , Isub_dVgse ;
double  Vdep ;
double  Vdep_dVbs , Vdep_dVds , Vdep_dVgs ;
double  Epkf ;
double  Epkf_dVbs , Epkf_dVds , Epkf_dVgs ;
/* PART-3 (overlap) */
double  yn , yn2 , yn3 ;
double  yn_dVbs , yn_dVds , yn_dVgs ;
//double  yned , yned2 ;
//double  yned_dVbs , yned_dVds , yned_dVgs ;
double  Lov , Lov2 /*, Lov23*/ ; 
//double  Ndsat , Gjnp ; 
double  Qgos , Qgos_dVbs , Qgos_dVds , Qgos_dVgs ;
double  Qgos_dVbse , Qgos_dVdse , Qgos_dVgse ;
double  Qgod , Qgod_dVbs , Qgod_dVds , Qgod_dVgs ;
double  Qgod_dVbse , Qgod_dVdse , Qgod_dVgse ;
double  Cggo , Cgdo , Cgso , Cgbo ;
/* fringing capacitance */
double  Cf ;
double  Qfd , Qfs ;
/* Cqy */
double  Pslk , Pslk_dVbs , Pslk_dVds , Pslk_dVgs ;
double  Qy ;
double  Cqyd, Cqyg, Cqys, Cqyb ;
//double  qy_dlt ;
/* PART-4 (junction diode) */
double  Ibs , Ibd , Gbs , Gbd , Gbse , Gbde ;
double  js ;
double  jssw ;
double  isbs ;
double  isbd ;
double  Nvtm ;
/* junction capacitance */
double  Qbs , Qbd , Capbs , Capbd , Capbse , Capbde ;
double  czbd , czbdsw , czbdswg , czbs , czbssw , czbsswg ;
double  arg , sarg ;
/* PART-5 (noise) */
double  NFalp , NFtrp /*, Freq*/ , Cit , Nflic ;
/* Bias iteration accounting Rs/Rd */
int     lp_bs ;
double  Ids_last ;
double  vtol_iprv = 2.0e-1 ;
double  vtol_pprv = 5.0e-2 ;
double  Vbsc_dif , Vdsc_dif , Vgsc_dif , sum_vdif ;
double  Rs , Rd ;
double  Fbs , Fds , Fgs ;
double  DJ , DJI , DJIP ;
double  JI11 , JI12 , JI13 , JI21 , JI22 , JI23 , JI31 , JI32 , JI33 ;
double  dVbs=0.0 , dVds=0.0 , dVgs=0.0 ;
double  dV_sum ;
/* Junction Bias */
double  Vbsj , Vbdj ;
/* Accumulation zone */
double  Psa ;
double  Psa_dVbs , Psa_dVds , Psa_dVgs ;
/* CLM */
double  Psdl , Psdl_dVbs , Psdl_dVds , Psdl_dVgs ;
double  Ed , Ed_dVbs , Ed_dVds , Ed_dVgs ;
double  Ec , Ec_dVbs , Ec_dVds , Ec_dVgs ;
double  Lred=0 , Lred_dVbs , Lred_dVds , Lred_dVgs ;
double  Wd , Wd_dVbs , Wd_dVds , Wd_dVgs ;
double  Aclm ;
/* Pocket Implant */
double Vthp, Vthp_dVb, Vthp_dVd, Vthp_dVs, Vthp_dVg ;
double dVthLP, dVthLP_dVb, dVthLP_dVd, dVthLP_dVs, dVthLP_dVg ;
/* Poly-Depletion Effect */
double dPpg , dPpg_dVd , dPpg_dVg , dPpg_dVb ;
double dPpgP , dPpgP_dVs , dPpgP_dVg , dPpgP_dVb ;
/* Quantum Effect */
double Tox , Tox_dVb , Tox_dVd , Tox_dVg ;
double dTox , dTox_dVb , dTox_dVd , dTox_dVs , dTox_dVg ;
double Cox , Cox_dVb , Cox_dVd , Cox_dVg ;
double Cox_inv , Cox_inv_dVb , Cox_inv_dVd , Cox_inv_dVg ;
double Vthq, Vthq_dVb , Vthq_dVd , Vthq_dVs /*, Vthq_dVg*/ ;
double ToxP , ToxP_dVb , ToxP_dVs , ToxP_dVg ;
double CoxP , CoxP_dVb , CoxP_dVs , CoxP_dVg ;
double CoxP_inv , CoxP_inv_dVb , CoxP_inv_dVs , CoxP_inv_dVg ;
/* Igate , Igidl , Igisl */
double  Egp12 , Egp32 ;
double  E0 ;
double  E1 , E1_dVb , E1_dVd , E1_dVs , E1_dVg ;
double  E2 , E2_dVbs , E2_dVds , E2_dVgs ;
double  Etun , Etun_dVbs , Etun_dVds , Etun_dVgs ;
double  Egidl , Egidl_dVb , Egidl_dVd , Egidl_dVg ;
double  Egisl , Egisl_dVb , Egisl_dVs , Egisl_dVg ;
double  Igate , Igate_dVbs , Igate_dVds , Igate_dVgs ;
double  Igate_dVbse , Igate_dVdse , Igate_dVgse ;
double  Igidl , Igidl_dVbs , Igidl_dVds , Igidl_dVgs ;
double  Igidl_dVbse , Igidl_dVdse , Igidl_dVgse ;
double  Igisl , Igisl_dVbd , Igisl_dVsd , Igisl_dVgd ;
double  Igisl_dVbde , Igisl_dVsde , Igisl_dVgde ;
//double  Cox0 ;
double  Lgate ;
double  rp1 , rp1_dVds ;
/* connecting function */
double  FD2 , FD2_dVbs , FD2_dVds , FD2_dVgs ;
double  FMD , FMD_dVds=0.0 ;
/* Phonon scattering */
double	Wgate ;
double	mueph ;
/* temporary vars. */
double  T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, /*T12,*/ T13 ;
double  TX , TX_dVbs , TX_dVds , TX_dVgs ;
double  TY , TY_dVbs , TY_dVds , TY_dVgs ;
double  T1_dVb , T1_dVd /*, T1_dVs*/ ,T1_dVg ;
double  T2_dVb , T2_dVd /*, T2_dVs*/ , T2_dVg ;
double  T3_dVb , T3_dVd /*, T3_dVs*/ , T3_dVg ;
double  T4_dVb , T4_dVd , T4_dVs /*, T4_dVg*/ ;
double  /*T5_dVb ,*/ T5_dVd , T5_dVs , T5_dVg ;
//double  T6_dVb , T6_dVd , T6_dVs , T6_dVg ;
double  T7_dVb , T7_dVd , T7_dVs , T7_dVg ;
double  T8_dVb , T8_dVd , T8_dVs , T8_dVg ;
double  /*T9_dVb ,*/ T9_dVd , T9_dVs , T9_dVg ;
double  /*T10_dVb ,*/ T10_dVd , T10_dVs , T10_dVg ;
double  /*T11_dVb ,*/ T11_dVd , T11_dVs , T11_dVg ;
//double  T12_dVb , T12_dVd , T12_dVs , T12_dVg ;
double  /*T13_dVb ,*/ T13_dVd , T13_dVs , T13_dVg ;
double  T20 , T21 , T30 , T31 ;

double c_eox ;


/*================ Start of executable code.=================*/

    flg_info = sIN.info ;

    flg_gidsl_d = ((sIN.mode == HiSIM_NORMAL_MODE && sIN.cogisl) ||
		   (sIN.mode == HiSIM_REVERSE_MODE && sIN.cogidl)) ? 1 : 0 ;


/*-----------------------------------------------------------*
* Change units into CGS.
* - This section may be moved to an interface routine.
*-----------------*/

/* device instances */
    sIN.xl     *= C_m2cm ;
    sIN.xw     *= C_m2cm ;
    sIN.as     *= C_m2cm_p2 ;
    sIN.ad     *= C_m2cm_p2 ;
    sIN.ps     *= C_m2cm ;
    sIN.pd     *= C_m2cm ;

/* model parameters */
    sIN.tox    *= C_m2cm ;
    sIN.xld    *= C_m2cm ;
    sIN.xwd    *= C_m2cm ;
    sIN.xqy    *= C_m2cm ;
    sIN.lp     *= C_m2cm ;
    sIN.xpolyd *= C_m2cm ;
    sIN.tpoly  *= C_m2cm ;
    sIN.rs     *= C_m2cm ;
    sIN.rd     *= C_m2cm ;
    sIN.sc3    *= C_m2cm ;
    sIN.scp3   *= C_m2cm ;
    sIN.parl2  *= C_m2cm ;
    sIN.wfc    *= C_m2cm ;
    sIN.wsti   *= C_m2cm ;
    sIN.rpock1 *= C_m2cm_p1o2 ;
    sIN.qme1   *= C_m2cm ;
    sIN.qme3   *= C_m2cm ;
    sIN.gidl1  *= C_m2cm_p1o2 ;
    sIN.cgso   /= C_m2cm ;
    sIN.cgdo   /= C_m2cm ;
    sIN.cgbo   /= C_m2cm ;
    sIN.js0    /= C_m2cm_p2 ;
    sIN.js0sw  /= C_m2cm ;
    sIN.cj     /= C_m2cm_p2 ;
    sIN.cjsw   /= C_m2cm ;
    sIN.cjswg  /= C_m2cm ;


/*-----------------------------------------------------------*
* Start of the routine. (label)
*-----------------*/
//start_of_routine:



/*-----------------------------------------------------------*
* Temperature dependent constants. 
*-----------------*/
 
/* Inverse of the thermal voltage */
    beta    = C_QE / ( C_KB * sIN.temp ) ;
    beta2   = beta * beta ;
 
/* Band gap */
    Eg  = C_Eg0 - sIN.temp 
            * ( sIN.bgtmp1 + sIN.temp * sIN.bgtmp2 ) ;

    Eg300  = C_Eg0 - C_T300  
            * ( sIN.bgtmp1 + C_T300 * sIN.bgtmp2 ) ;

/* Intrinsic carrier concentration */
    Nin = C_Nin0 * pow( sIN.temp / C_T300  , 1.5e0 ) 
            * exp( - Eg / 2.0e0 * beta + Eg300 / 2.0e0 * C_b300 ) ;


/*-----------------------------------------------------------*
* Fixed part. 
*-----------------*/

/* Lgate in [cm] / [m] */
    Lgate = sIN.xl + 2.0e0 * sIN.xpolyd ;
    Wgate = sIN.xw + 2.0e0 * sIN.xdiffd ;

/* Phonon Scattering */
    T1 = log( Wgate ) ;
    T2 = sIN.w0 - T1 - sti1_dlt ;
    T3 = T2 * T2 ;
    T4 = sqrt( T3 + 4.0 * sti1_dlt * sIN.w0 ) ;
    T5 = sIN.w0 - ( T2 - T4 ) / 2 ;
    mueph =  sIN.mueph1 + sIN.mueph2 * T5 ;

/* Metallurgical channel geometry */
    Weff    = Wgate - 2.0e0 * sIN.xwd ;
    Leff    = Lgate - 2.0e0 * sIN.xld ;
//    Leff_inv    = 1.0e0 / Leff ;

/* Flat band voltage */
    Vfb = sIN.vfbc ;

/* Surface impurity profile */
    if( Leff > sIN.lp ){
      Nsub = ( sIN.nsubc * ( Leff - sIN.lp ) 
             + sIN.nsubp * sIN.lp ) / Leff ;
    } else {
      Nsub = sIN.nsubp
           + ( sIN.nsubp - sIN.nsubc ) * ( sIN.lp - Leff ) / sIN.lp ;
    }

    q_Nsub  = C_QE * Nsub ;

/* 2 phi_B */
        /* @temp, with pocket */
    Pb2 =  2.0e0 / beta * log( Nsub / Nin ) ;
        /* @300K, with pocket */
    Pb20 = 2.0e0 / C_b300 * log( Nsub / C_Nin0 ) ;
        /* @300K, w/o pocket */
    Pb2c = 2.0e0 / C_b300 * log( sIN.nsubc / C_Nin0 ) ;

/* Debye length */
    Ldby    = sqrt( C_ESI / beta / q_Nsub ) ;

/* Coefficient of the F function for bulk charge */
    cnst0   = q_Nsub * Ldby * C_SQRT_2 ;

/* cnst1: n_{p0} / p_{p0} */
    T1  = Nin / Nsub ;
    cnst1   = T1 * T1 ;

/* C_EOX */
    c_eox = C_VAC * sIN.kappa ;

/* Cox (clasical) */
//    Cox0 = c_eox / sIN.tox ;


/*-----------------------------------------------------------*
* Exchange bias conditions according to MOS type.
* - Vxse are external biases for HiSIM. ( type=NMOS , Vds >= 0
*   are assumed.) 
*-----------------*/

    Vbse = sIN.vbs ;
    Vdse = sIN.vds ;
    Vgse = sIN.vgs ;
    Vbde = Vbse - Vdse ;
    if ( flg_gidsl_d ) {
     Vgde = Vgse - Vdse ;
     Vsde = -Vdse ;
    }


/*---------------------------------------------------*
* Clamp too large biases. 
* -note: Quantities are extrapolated in PART-5.
*-----------------*/

    if ( Vbse > 0.0 ) {
      flg_vbsc = 1 ;
      T1 = Vbse / Vbs_max ;
      T2 = sqrt ( 1.0 + ( T1 * T1 ) ) ;
      Vbsc = Vbse / T2 ;
      Vbsc_dVbse = Vbs_max * Vbs_max 
	/ ( ( Vbs_max * Vbs_max + Vbse * Vbse ) * T2 ) ;
    }  else if ( Vbse < Vbs_min ) {
      flg_vbsc = -1 ;
      Vbsc = Vbs_min ;
    }  else if ( Vbse >= Vbs_min && Vbse <= 0.0 ) {
      flg_vbsc =  0 ;
      Vbsc = Vbse ;
    }

    if ( Vbde > 0.0 ) {
      flg_vbdc = 1 ;
      T1 = Vbde / Vbd_max ;
      T2 = sqrt ( 1.0 + ( T1 * T1 ) ) ;
      Vbdc = Vbde / T2 ;
      Vbdc_dVbde = Vbd_max * Vbd_max
	/ ( ( Vbd_max * Vbd_max + Vbde * Vbde ) * T2 ) ;
    }  else if ( Vbde < Vbd_min ) {
      flg_vbdc = -1 ;
      Vbdc = Vbd_min ;
    }  else if ( Vbde >= Vbd_min && Vbde <= 0.0 ) {
      flg_vbdc =  0 ;
      Vbdc = Vbde ;
    }

    if (flg_clamp) {
 
      if ( Vdse > Vds_max ) {
	flg_vdsc = 1 ;
	Vdsc = Vds_max ;
      } else {
	flg_vdsc =  0 ;
	Vdsc = Vdse ;
      }

      if ( Vgse > Vgs_max ) {
	flg_vgsc = 1 ;
	Vgsc = Vgs_max ;
      } else {
	flg_vgsc =  0 ;
	Vgsc = Vgse ;
      }
      if ( flg_gidsl_d ) {

	if ( Vsde < -Vds_max ) {
	  flg_vsdc = 1 ;
	  Vsdc = -Vds_max ;
	} else {
	  flg_vsdc =  0 ;
	  Vsdc = Vsde ;
	}

	if ( Vgde > Vgd_max ) {
	  flg_vgdc = 1 ;
	  Vgdc = Vgd_max ;
	} else {
	  flg_vgdc =  0 ;
	  Vgdc = Vgde ;
	}
	
      }
	
    }
    else {

      flg_vdsc =  0 ; Vdsc = Vdse ;
      flg_vgsc =  0 ; Vgsc = Vgse ;
      flg_vsdc =  0 ; Vsdc = Vsde ;
      flg_vgdc =  0 ; Vgdc = Vgde ;
    }

    if ( flg_vbsc == -1 || flg_vdsc != 0 || flg_vgsc != 0 ||
         flg_vbdc == -1 || flg_vsdc != 0 || flg_vgdc != 0 ) {
      flg_vxxc = 1 ;
    }


/*-------------------------------------------------------------------*
* Set flags. 
*-----------------*/

    flg_rsrd = 0 ;
    flg_iprv = 0 ;
    flg_pprv = 0 ;

    if (sIN.mode == HiSIM_NORMAL_MODE) {
      Rs  = sIN.rs / Weff ;
      Rd  = sIN.rd / Weff ;
    }
    else {
      Rd  = sIN.rs / Weff ;
      Rs  = sIN.rd / Weff ;
    }

    if ( Rs + Rd >= epsm10 && sIN.corsrd >= 1 ) {
        flg_rsrd  = 1 ;
    }

    if ( sIN.has_prv == 1 ) {

      Vbsc_dif = Vbsc - sIN.vbsc_prv ;
      Vdsc_dif = Vdsc - sIN.vdsc_prv ;
      Vgsc_dif = Vgsc - sIN.vgsc_prv ;

      sum_vdif  = fabs( Vbsc_dif ) + fabs( Vdsc_dif ) 
                + fabs( Vgsc_dif ) ;

      if ( sIN.coiprv >= 1 && sum_vdif <= vtol_iprv ) { flg_iprv = 1 ;}
      if ( sIN.copprv >= 1 && sum_vdif <= vtol_pprv ) { flg_pprv = 1 ;}
    }

    if ( flg_rsrd == 0  ) {
        lp_bs_max = 1 ;
        flg_iprv  = 0 ;
    }

    if ( sIN.cosmbi == 0 ) {
      flg_smoothing_qme_vg = 0 ;
      flg_smoothing_pl_vd  = 0 ;
      flg_smoothing_sc_vd  = 0 ;
      flg_smoothing_pde_vg = 0 ;
      flg_smoothing_pde_vd = 0 ;
    }


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*
* Bias loop: iteration to solve the system of equations of 
*            the small circuit taking account Rs and Rd.
* - Vxs are internal (or effective) biases.
* - Equations:
*     Vbs = Vbsc - Rs * Ids
*     Vds = Vdsc - ( Rs + Rd ) * Ids
*     Vgs = Vgsc - Rs * Ids
*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-----------------------------------------------------------*
* Initial guesses for biases.
*-----------------*/

    if ( flg_iprv == 1 ) {

        sIN.ids_dvbs_prv = Fn_Max( 0.0 , sIN.ids_dvbs_prv ) ;
        sIN.ids_dvds_prv = Fn_Max( 0.0 , sIN.ids_dvds_prv ) ;
        sIN.ids_dvgs_prv = Fn_Max( 0.0 , sIN.ids_dvgs_prv ) ;

        dVbs = Vbsc_dif * ( 1.0 - 
               1.0 / ( 1.0 + Rs * sIN.ids_dvbs_prv ) ) ;
        dVds = Vdsc_dif * ( 1.0 - 
               1.0 / ( 1.0 + ( Rs + Rd ) * sIN.ids_dvds_prv ) ) ;
        dVgs = Vgsc_dif * ( 1.0 - 
               1.0 / ( 1.0 + Rs * sIN.ids_dvgs_prv ) ) ;

	/*
	Ids = sIN.type * sIN.ids_prv 
            + sIN.ids_dvbs_prv * dVbs 
            + sIN.ids_dvds_prv * dVds 
            + sIN.ids_dvgs_prv * dVgs ;
	*/
        Ids = sIN.ids_prv 
            + sIN.ids_dvbs_prv * dVbs 
            + sIN.ids_dvds_prv * dVds 
            + sIN.ids_dvgs_prv * dVgs ;

        T1  = ( Ids - sIN.ids_prv ) ;
        T2  = fabs( T1 ) ;
        if ( Ids_maxvar * sIN.ids_prv < T2 ) {
            Ids = sIN.ids_prv * ( 1.0 + Fn_Sgn( T1 ) * Ids_maxvar ) ;
        }

        if ( Ids < 0 ) {
          Ids = 0.0 ;
        }

    } else {
        Ids = 0.0 ;

        if ( flg_pprv == 1 ) {
            dVbs = Vbsc_dif ;
            dVds = Vdsc_dif ;
            dVgs = Vgsc_dif ;
        }
    }

    Vbs = Vbsc - Ids * Rs ;

    Vds = Vdsc - Ids * ( Rs + Rd ) ;
    if ( Vds * Vdsc <= 0.0 ) { Vds = 0.0 ; } 

    Vgs = Vgsc - Ids * Rs ;

    if ( flg_pprv == 1 ) {

        Ps0 = sIN.ps0_prv ;

        Ps0_dVbs = sIN.ps0_dvbs_prv ;
        Ps0_dVds = sIN.ps0_dvds_prv ;
        Ps0_dVgs = sIN.ps0_dvgs_prv ;

        Pds = sIN.pds_prv ;

        Pds_dVbs = sIN.pds_dvbs_prv ;
        Pds_dVds = sIN.pds_dvds_prv ;
        Pds_dVgs = sIN.pds_dvgs_prv ;
    }


/*-----------------------------------------------------------*
* start of the loop.
*-----------------*/

  for ( lp_bs = 1 ; lp_bs <= lp_bs_max ; lp_bs ++ ) {


    Ids_last = Ids ;


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
* PART-1: Basic device characteristics. 
*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
 
/*-----------------------------------------------------------*
* Initialization. 
*-----------------*/

    /* Initialization of counters is needed for restart. */
    lp_s0   = 0 ;
    lp_sl   = 0 ;

 
/*-----------------------------------------------------------*
* Vxsz: Modified bias introduced to realize symmetry at Vds=0.
*-----------------*/

    T1 = exp( - Vbsc_dVbse * Vds / ( 2.0 * sIN.vzadd0 ) ) ;
    Vzadd = sIN.vzadd0 * T1 ;
    Vzadd_dVds = - 0.5 * Vbsc_dVbse * T1 ;

    if ( Vzadd < ps_conv ) {
        Vzadd = 0.0 ;
        Vzadd_dVds = 0.0 ;
    }

    Vbsz = Vbs + Vzadd ;
    Vbsz_dVbs = 1.0 ;
    Vbsz_dVds = Vzadd_dVds ;

    Vdsz = Vds + 2 * Vzadd ;
    Vdsz_dVds = 1.0 + 2 * Vzadd_dVds ;

    Vgsz = Vgs + Vzadd ;
    Vgsz_dVgs = 1.0 ;
    Vgsz_dVds = Vzadd_dVds ;


/*-----------------------------------------------------------*
* Quantum effect
*-----------------*/

    T9  = sIN.qme1 ;
    T11 = sIN.qme3 ;
    T10  = sIN.qme2 ;

    T1 = 2.0 * q_Nsub * C_ESI ;
    T2 = sqrt( T1 * ( Pb20 - Vbsz ) ) ;

    Vthq = Pb20 + Vfb + sIN.tox / c_eox * T2 + T10 ;

    T3 = - 0.5 * sIN.tox / c_eox * T1 / T2 ;
    Vthq_dVb = T3 * Vbsz_dVbs ;
    Vthq_dVd = T3 * Vbsz_dVds ;

    if (flg_smoothing_qme_vg) {
      T5 = Vgsz_qme_sat - Vgsz - qme_vgs_dlt ;
      T5_dVd = - Vgsz_dVds ;
      T5_dVg = - Vgsz_dVgs ;
      T6 = sqrt( T5 * T5 + 4.0 * qme_vgs_dlt * Vgsz_qme_sat) ;
      T13 = Vgsz_qme_sat - 0.5 * (T5 + T6) ;
      T13_dVd = -0.5 * ( T5_dVd + T5 * T5_dVd / T6 ) ;
      T13_dVg = -0.5 * ( T5_dVg + T5 * T5_dVg / T6 ) ;
    }
    else {
      T13 = Vgsz ;
      T13_dVd = Vgsz_dVds ;
      T13_dVg = Vgsz_dVgs ;
    }

    T1 = - T10 ;
    T2 = T13 - Vthq ;
    T3 = T9 * T1 * T1 + T11 ;
    T4 = T9 * T2 * T2 + T11 ;
    T5 = T4 - T3 - qme_dlt ;
    T6 = sqrt( T5 * T5 + 4.0 * qme_dlt * T4 ) ;

    dTox = T4 - 0.5 * ( T5 + T6 ) ;

    /* dTox_dT4 */
    T7 = 1.0 - 0.5 * ( 1.0 + ( T5 + 2.0 * qme_dlt ) / T6 ) ;
    T8 = 2.0 * T9 * T2 * T7 ;

    dTox_dVb = T8 * ( - Vthq_dVb ) ;
    dTox_dVd = T8 * ( T13_dVd - Vthq_dVd ) ;
    dTox_dVg = T8 * ( T13_dVg ) ;

    if ( T13 - Vthq > 0 ) {
      T5 = T11 - T3 - qme_dlt ;
      T6 = sqrt( T5 * T5 + 4.0 * qme_dlt * T11 ) ;
      
      dTox =  T11 - 0.5 * ( T5 + T6 ) ;
      dTox_dVb = 0.0 ;
      dTox_dVd = 0.0 ;
      dTox_dVg = 0.0 ;
    }

    Tox = sIN.tox + dTox ;
    Tox_dVb = dTox_dVb ;
    Tox_dVd = dTox_dVd ;
    Tox_dVg = dTox_dVg ;

    Cox = c_eox / Tox ;
    T1  = - c_eox / ( Tox * Tox ) ;
    Cox_dVb = T1 * Tox_dVb ;  
    Cox_dVd = T1 * Tox_dVd ;  
    Cox_dVg = T1 * Tox_dVg ;  

    Cox_inv  = Tox / c_eox ; 
    T1  = 1.0 / c_eox ;
    Cox_inv_dVb = T1 * Tox_dVb ;  
    Cox_inv_dVd = T1 * Tox_dVd ;  
    Cox_inv_dVg = T1 * Tox_dVg ;  


/*-----------------------------------------------------------*
* Threshold voltage. 
*-----------------*/

    Delta   = 0.1 ;

    Vbs1    = 2.0 - 0.25 * Vbsz ;
    Vbs2    = - Vbsz ;
    
    Vbsd    = Vbs1 - Vbs2 - Delta ; 
//    Vbsd_dVbs = 0.75 * Vbsz_dVbs ;
//    Vbsd_dVds = 0.75 * Vbsz_dVds ;

    T1      = sqrt( Vbsd * Vbsd + 4.0 * Delta ) ;
    
    Psum    = ( Pb20 - Vbsz ) ;

    if ( Psum >= epsm10 ) {
        Psum_dVb   = - Vbsz_dVbs ;
        Psum_dVd   = - Vbsz_dVds ;
    } else {
        Psum    = epsm10 ;
        Psum_dVb   = 0.0e0 ;
        Psum_dVd   = 0.0e0 ;
    }

    sqrt_Psum   = sqrt( Psum ) ;


/*---------------------------------------------------*
* Vthp : Vth with pocket.
*-----------------*/

    T1   = 2.0 * q_Nsub * C_ESI ;
    Qb0   = sqrt( T1 * ( Pb20 - Vbsz ) ) ;

    Qb0_dVb = 0.5 * T1 / Qb0 * ( - Vbsz_dVbs ) ;
    Qb0_dVd = 0.5 * T1 / Qb0 * ( - Vbsz_dVds ) ;

    Vthp = Pb20 + Vfb + Qb0 * Cox_inv ;

    Vthp_dVb = Qb0_dVb * Cox_inv + Qb0 * Cox_inv_dVb ;
    Vthp_dVd = Qb0_dVd * Cox_inv + Qb0 * Cox_inv_dVd ;
    Vthp_dVg = Qb0 * Cox_inv_dVg ;


/*-------------------------------------------*
* dVthLP : Short-channel effect induced by pocket.
* - Vth0 : Vth without pocket.
*-----------------*/

    if ( sIN.lp != 0.0 ) {

      T1   = 2.0 * C_QE * sIN.nsubc * C_ESI ;
      T2   = sqrt( T1 * ( Pb2c - Vbsz ) ) ;

      Vth0 = Pb2c + Vfb + T2 * Cox_inv ;

      Vth0_dVb = 0.5 * T1 / T2 * ( - Vbsz_dVbs ) * Cox_inv 
                + T2 * Cox_inv_dVb ;
      Vth0_dVd = 0.5 * T1 / T2 * ( - Vbsz_dVds ) * Cox_inv 
                + T2 * Cox_inv_dVd ;
      Vth0_dVg = T2 * Cox_inv_dVg ;

      T1  = C_ESI * Cox_inv ;
      T2  = sqrt( 2.0e0 * C_ESI / C_QE / sIN.nsubp ) ;
      T4  = 1.0e0 / sIN.parl1 / ( sIN.lp * sIN.lp ) ;
      T5  = 2.0e0 * ( C_Vbi - Pb20 ) * T1 * T2 * T4 ;

      dVth0   = T5 * sqrt_Psum ;

      T6  = 0.5 * T5 / sqrt_Psum ;
      T7  = 2.0e0 * ( C_Vbi - Pb20 ) * C_ESI * T2 * T4 * sqrt_Psum ;
      dVth0_dVb  = T6 * Psum_dVb + T7 * Cox_inv_dVb ;
      dVth0_dVd  = T6 * Psum_dVd + T7 * Cox_inv_dVd ;
      dVth0_dVg  = T7 * Cox_inv_dVg ;

      T4 = sIN.scp1 ;
      T5 = sIN.scp2 ;
      T6 = sIN.scp3 ;

      if (flg_smoothing_pl_vd) {
	T8 = Vdsz_lp_sat - Vdsz - lp_vds_dlt ;
	T8_dVd = - Vdsz_dVds ;
	T9 = sqrt( T8 * T8 + 4.0 * lp_vds_dlt * Vdsz_lp_sat) ;
	T7 = Vdsz_lp_sat - 0.5 * (T8 + T9) ;
	T7_dVd = -0.5 * ( T8_dVd + T8 * T8_dVd / T9 ) ;
      }
      else {
	T7 = Vdsz ;
	T7_dVd = Vdsz_dVds ;
      }

      T1 = Vthp - Vth0 ;
      T2 = T4 + T6 * Psum / sIN.lp ;
      T3 = T2 + T5 * T7 ;

      dVthLP  = T1 * dVth0 * T3 ;

      dVthLP_dVb = ( Vthp_dVb - Vth0_dVb ) * dVth0 * T3 
                    + T1 * dVth0_dVb * T3 
                    + T1 * dVth0 * T6 * Psum_dVb / sIN.lp ;

      dVthLP_dVd = ( Vthp_dVd - Vth0_dVd ) * dVth0 * T3 
                    + T1 * dVth0_dVd * T3 
                    + T1 * dVth0
                    * ( T6 * Psum_dVd / sIN.lp 
			+ T5 * T7_dVd ) ;

      dVthLP_dVg = ( Vthp_dVg - Vth0_dVg ) * dVth0 * T3 
                    + T1 * dVth0_dVg * T3 ;

    } else {
      dVthLP = 0.0e0 ;
      dVthLP_dVb = 0.0e0 ;
      dVthLP_dVd = 0.0e0 ;
      dVthLP_dVg = 0.0e0 ;
    }


/*---------------------------------------------------*
* dVthSC : Short-channel effect induced by Vds.
*-----------------*/

    /* Modify Pb20 to Pb20b */

    Vgpa = Vgsz - Vfb ;
    cnst3 = ( cnst0 * cnst0 ) / ( Cox * Cox ) ;
    T1 = 1.0e0 + 4.0e0 * (beta * Vgpa - 1.0e0 ) / ( cnst3 * beta * beta ) ;
    if ( T1 > 0.0 ) {
      T2 = sqrt(T1) ;
      Psi_a = Vgpa + cnst3 * beta / 2.0e0 * ( 1.0e0 - T2 ) ;
      Psi_a_dVg = Vgsz_dVgs * ( 1.0e0 - 1.0e0 / T2 ) ;
    } else {
      Psi_a = Psi_a_dVg = 0.0 ;
    }

    T1 = sqrt( ( Pb20 - Psi_a - delta0 ) * ( Pb20 - Psi_a - delta0 )
	       + 4.0e0 * delta0 * Pb20 ) ;
    Pb20a = Pb20 - 0.5e0 * ( Pb20 - Psi_a - delta0 + T1 ) ;
    Pb20a_dVg = 0.5e0 * Psi_a_dVg * ( 1.0e0 + ( Pb20 - Psi_a - delta0 ) / T1 ) ;
    Pb20b = Pb20 + sIN.pthrou * ( Pb20a - Pb20 ) ;
    Pb20b_dVg = sIN.pthrou * Pb20a_dVg ;

    T1  = C_ESI * Cox_inv ;
    T2  = sqrt( 2.0e0 * C_ESI / q_Nsub ) ;
    T3  = ( Leff - sIN.parl2 ) ;
    T4  = 1.0e0 / sIN.parl1 / ( T3 * T3 ) ;
    T5  = 2.0e0 * ( C_Vbi - Pb20b ) * T1 * T2 * T4 ;

    dVth0   = T5 * sqrt_Psum ;
    T6  = T5 / 2 / sqrt_Psum ;
    T7  = 2.0e0 * ( C_Vbi - Pb20b ) * C_ESI * T2 * T4 * sqrt_Psum ;
    dVth0_dVb  = T6 * Psum_dVb + T7 * Cox_inv_dVb ;
    dVth0_dVd  = T6 * Psum_dVd + T7 * Cox_inv_dVd ;
    dVth0_dVg  = T7 * Cox_inv_dVg + 2.0e0 * T1 * T2 * T4 * ( - Pb20b_dVg ) ;

    T5 = sIN.sc1 ;
    T6 = sIN.sc2 ;
    T7 = sIN.sc3 ;

    if (flg_smoothing_sc_vd) {
      T9 = Vdsz_sc_sat - Vdsz - sc_vds_dlt ;
      T9_dVd = - Vdsz_dVds ;
      T10 = sqrt( T9 * T9 + 4.0 * sc_vds_dlt * Vdsz_sc_sat) ;
      T8 = Vdsz_sc_sat - 0.5 * (T9 + T10) ;
      T8_dVd = -0.5 * ( T9_dVd + T9 * T9_dVd / T10 ) ;
    }
    else {
      T8 = Vdsz ;
      T8_dVd = Vdsz_dVds ;
    }

    T4 = T5 + T7 * Psum / Leff ;
    T4_dVb = T7 * Psum_dVb / Leff ;
    T4_dVd = T7 * Psum_dVd / Leff ;

    dVthSC  = dVth0 * ( T4 + T6 * T8 ) ;

    dVthSC_dVb = dVth0_dVb * ( T4 + T6 * T8 )
                + dVth0 * ( T4_dVb ) ;

    dVthSC_dVd = dVth0_dVd * ( T4 + T6 * T8 )
                + dVth0 * ( T4_dVd + T6 * T8_dVd ) ;

    dVthSC_dVg = dVth0_dVg * ( T4 + T6 * T8 ) ;


/*---------------------------------------------------*
* dVthW : narrow-channel effect.
*-----------------*/

    T5 = sIN.wfc ; 

    T1 = 1.0 / Cox ;
    T2 = T1 * T1 ;
    T3 = 1.0 / ( Cox + T5 / Weff ) ;
    T4 = T3 * T3 ;

    dVthW = Qb0 * ( T1 - T3 ) ;

    dVthW_dVb = Qb0_dVb * ( T1 - T3 )
               - Qb0 * Cox_dVb * ( T2 - T4 ) ;
    dVthW_dVd = Qb0_dVd * ( T1 - T3 )
               - Qb0 * Cox_dVd * ( T2 - T4 ) ;
    dVthW_dVg = - Qb0 * Cox_dVg * ( T2 - T4 ) ;


/*---------------------------------------------------*
* dVth : Total variation. 
* - Positive dVth means the decrease in Vth.
*-----------------*/
 
    dVth    = dVthSC + dVthLP + dVthW ;
    dVth_dVb   = dVthSC_dVb + dVthLP_dVb + dVthW_dVb ;
    dVth_dVd   = dVthSC_dVd + dVthLP_dVd + dVthW_dVd ;
    dVth_dVg   = dVthSC_dVg + dVthLP_dVg + dVthW_dVg ;


/*---------------------------------------------------*
* Vth : Threshold voltage. 
*-----------------*/
 
    Vth = Vthp - dVth ;


/*---------------------------------------------------*
* Poly-Depletion Effect
*-----------------*/

    T1 = sIN.pgd1 ;
    T2 = sIN.pgd2 ;
    T3 = sIN.pgd3 ;
    
    if (flg_smoothing_pde_vg) {
      T5 = Vgsz_pol_sat - Vgsz - pol_vgs_dlt ;
      /*	T5_dVb = 0.0 ;*/
      T5_dVd = - Vgsz_dVds ;
      T5_dVg = - Vgsz_dVgs ;
      T6 = sqrt( T5 * T5 + 4.0 * pol_vgs_dlt * Vgsz_pol_sat ) ;
      T7 = Vgsz_pol_sat - 0.5 * (T5 + T6) ;
      /*	T7_dVb = -0.5 * ( T5_dVb + T5 * T5_dVb / T6 ) ; */
      T7_dVd = -0.5 * ( T5_dVd + T5 * T5_dVd / T6 ) ;
      T7_dVg = -0.5 * ( T5_dVg + T5 * T5_dVg / T6 ) ;
    }
    else {
      T7 = Vgsz ;
      /*	T7_dVb = 0.0 ; */
      T7_dVd = Vgsz_dVds ;
      T7_dVg = Vgsz_dVgs ;
    }

    if (flg_smoothing_pde_vd) {
      T5 = Vdsz_pol_sat - Vdsz - pol_vds_dlt ;
      /*	T5_dVb = 0.0 ;*/
      T5_dVd = - Vdsz_dVds ;
      /*	T5_dVg = 0.0 ;*/
      T6 = sqrt( T5 * T5 + 4.0 * pol_vds_dlt * Vdsz_pol_sat ) ;
      T8 = Vdsz_pol_sat - 0.5 * (T5 + T6) ;
      /*	T8_dVb = -0.5 * ( T5_dVb + T5 * T5_dVb / T6 ) ; */
      T8_dVd = -0.5 * ( T5_dVd + T5 * T5_dVd / T6 ) ;
      /*	T8_dVg = -0.5 * ( T5_dVg + T5 * T5_dVg / T6 ) ; */
    }
    else {
      T8 = Vdsz ;
      /*	T8_dVb = 0.0 ; */
      T8_dVd = Vdsz_dVds ;
      /*	T8_dVg = 0.0 ; */
    }
    
    T9 = - T1 * ( T7 - T2 - T3 * T8 ) - pol2_dlt ;
    T9_dVd = - T1 * (T7_dVd - T3 * T8_dVd) ;
    T9_dVg = - T1 * (T7_dVg /*- T3 * T8_dVg*/) ;
    
    T10 = T9 * T9 ;
    T10_dVd = 2.0e0 * T9 * T9_dVd ;
    T10_dVg = 2.0e0 * T9 * T9_dVg ;

    T11 = sqrt( T10 + 4.0e0 * pol2_dlt * pol_tmp ) ;
    T11_dVd = T10_dVd / sqrt( T10 + 4.0e0 * pol2_dlt * pol_tmp ) * 0.5e0 ;
    T11_dVg = T10_dVg / sqrt( T10 + 4.0e0 * pol2_dlt * pol_tmp ) * 0.5e0 ;
    
    dPpg = ( T11 - T9 ) * 0.5e0 ;
    dPpg_dVd = ( T11_dVd - T9_dVd ) * 0.5e0 ;
    dPpg_dVg = ( T11_dVg - T9_dVg ) * 0.5e0 ;
    dPpg_dVb = 0.0 ;


/*---------------------------------------------------*
* Vgp : Effective gate bias with SCE & RSCE & flatband. 
*-----------------*/
 
    Vgp = Vgs - Vfb + dVth - dPpg ;

    Vgp_dVbs    = dVth_dVb - dPpg_dVb ;
    Vgp_dVds    = dVth_dVd - dPpg_dVd ;
    Vgp_dVgs    = 1.0e0 + dVth_dVg - dPpg_dVg ;
   

/*---------------------------------------------------*
* Vgs_fb : Actual flatband voltage taking account Vbs. 
* - note: if Vgs == Vgs_fb then Vgp == Ps0 == Vbs . 
*------------------*/
 
    Vgs_fb  = Vfb - dVth + dPpg + Vbs ;


/*-----------------------------------------------------------*
* Constants in the equation of Ps0 . 
*-----------------*/
 
    fac1    = cnst0 * Cox_inv ;
    fac1_dVbs = cnst0 * Cox_inv_dVb ;
    fac1_dVds = cnst0 * Cox_inv_dVd ;
    fac1_dVgs = cnst0 * Cox_inv_dVg ;

    fac1p2  = fac1 * fac1 ;

 
/*-----------------------------------------------------------*
* Accumulation zone. (zone-A)
* - evaluate basic characteristics and exit from this part.
*-----------------*/
 
    if ( Vgs < Vgs_fb ) { 
 
 
/*---------------------------------------------------*
* Evaluation of Ps0.
* - Psa : Analytical solution of 
*             Cox( Vgp - Psa ) = cnst0 * Qacc
*         where Qacc is the 3-degree series of (fdep)^{1/2}.
*         The unkown is transformed to Chi=beta(Ps0-Vbs).
* - Ps0_min : |Ps0_min| when Vbs=0.
*-----------------*/
 
        Ps0_min = Eg - Pb2 ;

        TX = beta * ( Vgp - Vbs ) ;

        TX_dVbs = beta * ( Vgp_dVbs - 1.0 ) ;
        TX_dVds = beta * Vgp_dVds ;
        TX_dVgs = beta * Vgp_dVgs ;

        TY = Cox / ( beta * cnst0 ) ;

        T1 = 1.0 / ( beta * cnst0 ) ;
        TY_dVbs = T1 * Cox_dVb ;
        TY_dVds = T1 * Cox_dVd ;
        TY_dVgs = T1 * Cox_dVg ;

        Ac41 = 2.0 + 3.0 * C_SQRT_2 * TY ;

        Ac4 = 8.0 * Ac41 * Ac41 * Ac41 ;

        T1 = 72.0 * Ac41 * Ac41 * C_SQRT_2 ;
        Ac4_dVbs = T1 * TY_dVbs ;
        Ac4_dVds = T1 * TY_dVds ;
        Ac4_dVgs = T1 * TY_dVgs ;

        Ac31 = 7.0 * C_SQRT_2 - 9.0 * TY * ( TX - 2.0 ) ;
        Ac31_dVbs = - 9.0 * ( TY_dVbs * ( TX - 2.0 ) + TY * TX_dVbs ) ;
        Ac31_dVds = - 9.0 * ( TY_dVds * ( TX - 2.0 ) + TY * TX_dVds ) ;
        Ac31_dVgs = - 9.0 * ( TY_dVgs * ( TX - 2.0 ) + TY * TX_dVgs ) ;

        Ac3 = Ac31 * Ac31 ;

        Ac3_dVbs = 2.0 * Ac31 * Ac31_dVbs ;
        Ac3_dVds = 2.0 * Ac31 * Ac31_dVds ;
        Ac3_dVgs = 2.0 * Ac31 * Ac31_dVgs ;

        Ac2 = sqrt( Ac4 + Ac3 ) ;
        Ac2_dVbs = 0.5 * ( Ac4_dVbs + Ac3_dVbs ) / Ac2 ;
        Ac2_dVds = 0.5 * ( Ac4_dVds + Ac3_dVds ) / Ac2 ;
        Ac2_dVgs = 0.5 * ( Ac4_dVgs + Ac3_dVgs ) / Ac2 ;

        Ac1 = -7.0 * C_SQRT_2
           + Ac2 + 9.0 * TY * ( TX - 2.0 ) ;

        Ac1_dVbs = Ac2_dVbs
                + 9.0 * ( TY_dVbs * ( TX - 2.0 ) + TY * TX_dVbs ) ;
        Ac1_dVds = Ac2_dVds
                + 9.0 * ( TY_dVds * ( TX - 2.0 ) + TY * TX_dVds ) ;
        Ac1_dVgs = Ac2_dVgs
                + 9.0 * ( TY_dVgs * ( TX - 2.0 ) + TY * TX_dVgs ) ;

        Acd = pow( Ac1 , C_1o3 ) ;

        T1 = C_1o3 / ( Acd * Acd ) ;
        Acd_dVbs = Ac1_dVbs * T1 ;
        Acd_dVds = Ac1_dVds * T1 ;
        Acd_dVgs = Ac1_dVgs * T1 ;

        Acn = -4.0 * C_SQRT_2 - 12.0 * TY
           + 2.0 * Acd + C_SQRT_2 * Acd * Acd ;

        Acn_dVbs = - 12.0 * TY_dVbs
                + ( 2.0 + 2.0 * C_SQRT_2 * Acd ) * Acd_dVbs ;
        Acn_dVds = - 12.0 * TY_dVds
                + ( 2.0 + 2.0 * C_SQRT_2 * Acd ) * Acd_dVds ;
        Acn_dVgs = - 12.0 * TY_dVgs
                + ( 2.0 + 2.0 * C_SQRT_2 * Acd ) * Acd_dVgs ;

        Chi = Acn / Acd ;

        T1 = 1.0 / ( Acd * Acd ) ;

        Chi_dVbs = ( Acn_dVbs * Acd - Acn * Acd_dVbs ) * T1 ;
        Chi_dVds = ( Acn_dVds * Acd - Acn * Acd_dVds ) * T1 ;
        Chi_dVgs = ( Acn_dVgs * Acd - Acn * Acd_dVgs ) * T1 ;

        Psa = Chi / beta + Vbs ;

        Psa_dVbs = Chi_dVbs / beta + 1.0 ;
        Psa_dVds = Chi_dVds / beta ;
        Psa_dVgs = Chi_dVgs / beta ;

        T1 = Psa - Vbs ;
        T2 = T1 / Ps0_min ;
        T3 = sqrt( 1.0 + ( T2 * T2 ) ) ;

        T9 = T2 / T3 / Ps0_min ;
        T3_dVb = T9 * ( Psa_dVbs - 1.0 ) ;
        T3_dVd = T9 * ( Psa_dVds ) ;
        T3_dVg = T9 * ( Psa_dVgs ) ;

        Ps0 = T1 / T3 + Vbs ;

        T9 = 1.0 / ( T3 * T3 ) ;

        Ps0_dVbs = T9 * ( ( Psa_dVbs - 1.0 ) * T3 - T1 * T3_dVb )
                 + 1.0 ;
        Ps0_dVds = T9 * ( Psa_dVds * T3 - T1 * T3_dVd ) ;
        Ps0_dVgs = T9 * ( Psa_dVgs * T3 - T1 * T3_dVg ) ;


/*---------------------------------------------------*
* Characteristics. 
*-----------------*/
 
        T0  = - Weff * Leff ;
        T1  = T0 * Cox ;
        T2  = ( Vgp - Ps0 ) ;

        Qb  = T1 * T2 ;
 
        Qb_dVbs = T1 * ( Vgp_dVbs - Ps0_dVbs ) 
                + T0 * Cox_dVb * T2 ;
        Qb_dVds = T1 * ( Vgp_dVds - Ps0_dVds ) 
                + T0 * Cox_dVd * T2 ;
        Qb_dVgs = T1 * ( Vgp_dVgs - Ps0_dVgs ) 
                + T0 * Cox_dVg * T2 ;

        Psl = Ps0 ;
        Psl_dVbs    = Ps0_dVbs ;
        Psl_dVds    = Ps0_dVds ;
        Psl_dVgs    = Ps0_dVgs ;

        Psdl = Psl ;
        Psdl_dVbs = Psl_dVbs ;
        Psdl_dVds = Psl_dVds ;      
        Psdl_dVgs = Psl_dVgs ;
 
        Qi  = 0.0e0 ;
        Qi_dVbs = 0.0e0 ;
        Qi_dVds = 0.0e0 ;
        Qi_dVgs = 0.0e0 ;
 
        Qd  = 0.0e0 ;
        Qd_dVbs = 0.0e0 ;
        Qd_dVds = 0.0e0 ;
        Qd_dVgs = 0.0e0 ;
 
        Ids = 0.0e0 ;
        Ids_dVbs    = 0.0e0 ;
        Ids_dVds    = 0.0e0 ;
        Ids_dVgs    = 0.0e0 ;

        VgVt  = 0.0 ;

        flg_noqi = 1 ;
 
        goto end_of_part_1 ;
 
    } 


/*-----------------------------------------------------------*
* Initial guess for Ps0. 
*-----------------*/

/*---------------------------------------------------*
* Ps0_iniA: solution of subthreshold equation assuming zone-D1/D2.
*-----------------*/
 
    TX  = 1.0e0 + 4.0e0 
        * ( beta * ( Vgp - Vbs ) - 1.0e0 ) / ( fac1p2 * beta2 ) ;
    TX  = Fn_Max( TX , epsm10 ) ;
 
    Ps0_iniA    = Vgp
                + fac1p2 * beta / 2.0e0 * ( 1.0e0 - sqrt( TX ) ) ;


/*---------------------------------------------------*
* Use previous value.
*-----------------*/
    if ( flg_pprv == 1 ) {

        Ps0_ini  = Ps0 + Ps0_dVbs * dVbs 
                 + Ps0_dVds * dVds  + Ps0_dVgs * dVgs ;

        T1 = Ps0_ini - Ps0 ;

        if ( T1 < - dP_max || T1 > dP_max ) {
            flg_pprv = 0 ;
        } else {
            Ps0_iniA = Fn_Max( Ps0_ini , Ps0_iniA ) ;
        }
 
    } 


/*---------------------------------------------------*
* Analytical initial guess.
*-----------------*/

    if ( flg_pprv == 0 ) {

/*-------------------------------------------*
* Common part.
*-----------------*/
 
        Chi = beta * ( Ps0_iniA - Vbs ) ;
 
/*-----------------------------------*
* zone-D1/D2
* - Ps0_ini is the analytical solution of Qs=Qb0 with
*   Qb0 being approximated to 3-degree polynomial.
*-----------------*/

        if ( Chi < znbd3 ) { 
 
            TY  = beta * ( Vgp - Vbs ) ;
            T1  = 1.0e0 / ( cn_nc3 * beta * fac1 ) ;
            T2  = 81 + 3 * T1 ;
            T3  = -2916 - 81 * T1 + 27 * T1 * TY ;
            T4  = 1458 - 81 * ( 54 + T1 ) + 27 * T1 * TY ;
            T4  = T4 * T4 ;
            T5  = pow( T3 + sqrt( 4 * T2 * T2 * T2 + T4 ) , C_1o3 ) ;
            TX  = 3 
                - ( C_2p_1o3 * T2 ) / ( 3 * T5 ) 
                + 1 / ( 3 * C_2p_1o3 ) * T5 ;
 
            Ps0_iniA    = TX / beta + Vbs ;
 
            Ps0_ini = Ps0_iniA ;
 
 
/*-----------------------------------*
* Weak inversion zone.  
*-----------------*/

        } else if ( Vgs <= Vth ) { 
 
            Ps0_ini = Ps0_iniA ;


/*-----------------------------------*
* Strong inversion zone.  
* - Ps0_iniB : upper bound.
*-----------------*/

        } else { 
 
            T1  = ( Cox * Cox ) / ( cnst0 * cnst0 ) / cnst1 ;
            T2  = T1 * Vgp * Vgp ;
            T3  = beta + 2.0 / Vgp ;

            Ps0_iniB    = log( T2 ) / T3 ;
 
            T1  = Ps0_iniB - Ps0_iniA - c_ps0ini_2 ;
            T2  = sqrt( T1 * T1 + 4.0e0 * c_ps0ini_2 * Ps0_iniB ) ;
 
            Ps0_ini = Ps0_iniB - ( T1 + T2 ) / 2 ;
 
        } 
    }

    if ( Ps0_ini < Vbs ) {
        Ps0_ini = Vbs ;
    }


/*---------------------------------------------------*
* Assign initial guess.
*-----------------*/
 
    Ps0 = Ps0_ini ;
 
    Psl_lim = Ps0_iniA ;


/*---------------------------------------------------*
* Calculation of Ps0. (beginning of Newton loop) 
* - Fs0 : Fs0 = 0 is the equation to be solved. 
* - dPs0 : correction value. 
*-----------------*/

    exp_bVbs    = exp( beta * Vbs ) ;

    cfs1  = cnst1 * exp_bVbs ;

    for ( lp_s0 = 1 ; lp_s0 <= lp_s0_max ; lp_s0 ++ ) { 
 
        Chi = beta * ( Ps0 - Vbs ) ;
 
        exp_Chi = exp( Chi ) ;

        fs01    = cfs1 * ( exp_Chi - 1.0e0 ) ;
        fs01_dPs0 = cfs1 * beta * ( exp_Chi ) ;


        if ( fs01 < epsm10 * cfs1 ) {
            fs01 = 0.0 ;
            fs01_dPs0 = 0.0 ;
        }


/*-------------------------------------------*
* zone-D1/D2.  (Ps0)
* - Qb0 is approximated to 5-degree polynomial.
*-----------------*/

        if ( Chi  < znbd5 ) { 

            fi  = Chi * Chi * Chi 
                * ( cn_im53 + Chi * ( cn_im54 + Chi * cn_im55 ) ) ;
            fi_dChi  = Chi * Chi 
                * ( 3 * cn_im53
                  + Chi * ( 4 * cn_im54 + Chi * 5 * cn_im55 ) ) ;

            fs01    = cfs1 * fi * fi ;
            fs01_dPs0 = cfs1 * beta * 2 * fi * fi_dChi ;


            fb  = Chi * ( cn_nc51 + Chi * ( cn_nc52 
                        + Chi * ( cn_nc53 
                        + Chi * ( cn_nc54 + Chi * cn_nc55 ) ) ) ) ;
            fb_dChi  =    cn_nc51 + Chi * ( 2 * cn_nc52 
                     + Chi * ( 3 * cn_nc53
                     + Chi * ( 4 * cn_nc54 + Chi * 5 * cn_nc55 ) ) ) ;
 
            fs02    = sqrt( fb * fb + fs01 ) ;
 
            if ( fs02 >= epsm10 ) {
                fs02_dPs0 = ( beta * fb_dChi * 2 * fb + fs01_dPs0 ) 
                        / ( fs02 + fs02 ) ;
            } else {
                fs02    = sqrt( fb * fb + fs01 ) ;
                fs02_dPs0 = beta * fb_dChi ;
            }
     
            Fs0    = Vgp - Ps0 - fac1 * fs02 ;
 
            Fs0_dPs0    = - 1.0e0 - fac1 * fs02_dPs0 ;
 
            dPs0  = - Fs0 / Fs0_dPs0 ;

 
/*-------------------------------------------*
* zone-D3.  (Ps0)
*-----------------*/

        } else { 

            Xi0 = Chi - 1.0e0 ;

            Xi0p12  = sqrt( Xi0 ) ;

            fs02    = sqrt( Xi0 + fs01 ) ;
 
            fs02_dPs0 = ( beta + fs01_dPs0 ) / ( fs02 + fs02 ) ;
     
            Fs0    = Vgp - Ps0 - fac1 * fs02 ;
 
            Fs0_dPs0    = - 1.0e0 - fac1 * fs02_dPs0 ;
     
            dPs0  = - Fs0 / Fs0_dPs0 ;
 
        } /* end of if ( Chi ... ) else block */

 
/*-------------------------------------------*
* Update Ps0 . 
* - clamped to Vbs if Ps0 < Vbs . 
*-----------------*/

        if ( fabs( dPs0 ) > dP_max ) { 
            dPs0  = fabs( dP_max ) * Fn_Sgn( dPs0 ) ;
        } 
 
        Ps0 = Ps0 + dPs0 ;
 
        if ( Ps0 < Vbs ) { 
            Ps0 = Vbs ;
        } 


/*-------------------------------------------*
* Check convergence. 
* NOTE: This condition may be too rigid. 
*-----------------*/
 
        if ( fabs( dPs0 ) <= ps_conv && fabs( Fs0 ) <= gs_conv ) { 
            break ;
        } 
 
    } /* end of Ps0 Newton loop */


/*-------------------------------------------*
* Procedure for diverged case.
*-----------------*/

    if ( lp_s0 > lp_s0_max ) { 
        fprintf( stderr , 
        "*** warning(HiSIM): Went Over Iteration Maximum (Ps0)\n" ) ;
        fprintf( stderr , 
            " Vbse   = %7.3f Vdse = %7.3f Vgse = %7.3f\n" , 
            Vbse , Vdse , Vgse ) ;
        if ( flg_info >= 2 ) {
          printf( 
          "*** warning(HiSIM): Went Over Iteration Maximum (Ps0)\n" ) ;
        }
    } 


/*---------------------------------------------------*
* Evaluate derivatives of  Ps0. 
* - note: Here, fs01_dVbs and fs02_dVbs are derivatives 
*   w.r.t. explicit Vbs. So, Ps0 in the fs01 and fs02
*   expressions is regarded as a constant.
*-----------------*/
 
    Chi = beta * ( Ps0 - Vbs ) ;

    exp_Chi = exp( Chi ) ;

    cfs1  = cnst1 * exp_bVbs ;
    if ( fs01 < epsm10 * cfs1 ) {
        fs01 = 0.0 ;
        fs01_dPs0 = 0.0 ;
        fs01_dVbs = 0.0 ;
    }


/*-------------------------------------------*
* zone-D1/D2. (Ps0)
*-----------------*/

    if ( Chi < znbd5 ) { 

        fi  = Chi * Chi * Chi 
            * ( cn_im53 + Chi * ( cn_im54 + Chi * cn_im55 ) ) ;
        fi_dChi  = Chi * Chi 
            * ( 3 * cn_im53
              + Chi * ( 4 * cn_im54 + Chi * 5 * cn_im55 ) ) ;

        fs01    = cfs1 * fi * fi ;
        fs01_dPs0 = cfs1 * beta * 2 * fi * fi_dChi ;
        fs01_dVbs = cfs1 * beta * fi * ( fi - 2 * fi_dChi ) ;

        fb  = Chi * ( cn_nc51 + Chi * ( cn_nc52 
                    + Chi * ( cn_nc53 
                    + Chi * ( cn_nc54 + Chi * cn_nc55 ) ) ) ) ;
        fb_dChi  =    cn_nc51 + Chi * ( 2 * cn_nc52 
                 + Chi * ( 3 * cn_nc53
                 + Chi * ( 4 * cn_nc54 + Chi * 5 * cn_nc55 ) ) ) ;

        fs02    = sqrt( fb * fb + fs01 ) ;

        T2  = 1.0e0 / ( fs02 + fs02 ) ;
 
        if ( fs02 >= epsm10 ) {
            fs02_dPs0 = ( beta * fb_dChi * 2 * fb + fs01_dPs0 ) * T2 ;
            fs02_dVbs   = ( - beta * fb_dChi * 2 * fb + fs01_dVbs ) 
                    * T2 ;
        } else {
            fs02_dPs0 = beta * fb_dChi ;
            fs02_dVbs   = - beta * fb_dChi ;
        }
 
        /* memo: Fs0   = Vgp - Ps0 - fac1 * fs02 */
 
        Fs0_dPs0    = - 1.0e0 - fac1 * fs02_dPs0 ;
 
        Ps0_dVbs    = - ( Vgp_dVbs 
                        - ( fac1 * fs02_dVbs + fac1_dVbs * fs02 )
                        ) / Fs0_dPs0 ;
        Ps0_dVds    = - ( Vgp_dVds
                        - fac1_dVds * fs02 
                        ) / Fs0_dPs0 ;
        Ps0_dVgs    = - ( Vgp_dVgs
                        - fac1_dVgs * fs02 
                        ) / Fs0_dPs0 ;

        T1  = cnst0 ;

        Qb0  = T1 * fb ;
        Qb0_dVb = ( Ps0_dVbs - 1.0e0 ) * T1 * beta * fb_dChi ;
        Qb0_dVd = Ps0_dVds * T1 * beta * fb_dChi ;
        Qb0_dVg = Ps0_dVgs * T1 * beta * fb_dChi ;

        fs01_dVbs   = Ps0_dVbs * fs01_dPs0 + fs01_dVbs ;
        fs01_dVds   = Ps0_dVds * fs01_dPs0 ;
        fs01_dVgs   = Ps0_dVgs * fs01_dPs0 ;
        fs02_dVbs   = Ps0_dVbs * fs02_dPs0 + fs02_dVbs ;
        fs02_dVds   = Ps0_dVds * fs02_dPs0 ;
        fs02_dVgs   = Ps0_dVgs * fs02_dPs0 ;

        T1 = 1.0 / ( fs02 + fb * fb ) ;
        T2 = T1 * T1 ;

        Qn0 = cnst0 * fs01 * T1 ;

        T3 = 2.0 * fb_dChi * fb * beta ;

        Qn0_dVbs = cnst0 * ( 
                     fs01_dVbs * T1 
                   - fs01 * ( fs02_dVbs
                            + T3 * ( Ps0_dVbs - 1.0 ) ) * T2 ) ;
        Qn0_dVds = cnst0 * ( 
                     fs01_dVds * T1 
                   - fs01 * ( fs02_dVds + T3 * Ps0_dVds ) * T2 ) ;
        Qn0_dVgs = cnst0 * ( 
                     fs01_dVgs * T1 
                   - fs01 * ( fs02_dVgs + T3 * Ps0_dVgs ) * T2 ) ;


/*-------------------------------------------*
* zone-D1. (Ps0)
* - Evaluate basic characteristics and exit from this part.
*-----------------*/

        if ( Chi < znbd3 ) {

            Psl = Ps0 ;
            Psl_dVbs    = Ps0_dVbs ;
            Psl_dVds    = Ps0_dVds ;
            Psl_dVgs    = Ps0_dVgs ;

            Pds = 0.0 ;
            Pds_dVbs = 0.0 ;
            Pds_dVds = 0.0 ;
            Pds_dVgs = 0.0 ;

            T1  = - Weff * Leff ;

            Qb  = T1 * Qb0 ;
            Qb_dVbs = T1 * Qb0_dVb ;
            Qb_dVds = T1 * Qb0_dVd ;
            Qb_dVgs = T1 * Qb0_dVg ;

            if ( Qn0 < Cox * VgVt_small ) {
                Qn0 = 0.0 ;
                Qn0_dVbs = 0.0 ;
                Qn0_dVds = 0.0 ;
                Qn0_dVgs = 0.0 ;
            }

            Qi  = T1 * Qn0 ;
            Qi_dVbs = T1 * Qn0_dVbs ;
            Qi_dVds = T1 * Qn0_dVds ;
            Qi_dVgs = T1 * Qn0_dVgs ;

            Qd  = 0.0e0 ;
            Qd_dVbs = 0.0e0 ;
            Qd_dVds = 0.0e0 ;
            Qd_dVgs = 0.0e0 ;
 
            Ids = 0.0e0 ;
            Ids_dVbs    = 0.0e0 ;
            Ids_dVds    = 0.0e0 ;
            Ids_dVgs    = 0.0e0 ;

            VgVt  = 0.0 ;

            flg_noqi = 1 ;
 
            goto end_of_part_1 ;
        }


/*-------------------------------------------*
* zone-D2
*-----------------*/

        Xi0 = Chi - 1.0e0 ;
 
        Xi0_dVbs    = beta * ( Ps0_dVbs - 1.0e0 ) ;
        Xi0_dVds    = beta * Ps0_dVds ;
        Xi0_dVgs    = beta * Ps0_dVgs ;

        Xi0p12  = sqrt( Xi0 ) ;
        Xi0p32  = Xi0 * Xi0p12 ;

        Xi0p12_dVbs = 0.5e0 * Xi0_dVbs / Xi0p12 ;
        Xi0p12_dVds = 0.5e0 * Xi0_dVds / Xi0p12 ;
        Xi0p12_dVgs = 0.5e0 * Xi0_dVgs / Xi0p12 ;
 
//        Xi0p32_dVbs = 1.5e0 * Xi0_dVbs * Xi0p12 ;
//        Xi0p32_dVds = 1.5e0 * Xi0_dVds * Xi0p12 ;
//        Xi0p32_dVgs = 1.5e0 * Xi0_dVgs * Xi0p12 ;

        Qn00 = Qn0 ;
        Qn00_dVbs = Qn0_dVbs ;
        Qn00_dVds = Qn0_dVds ;
        Qn00_dVgs = Qn0_dVgs ;

        fs01    = cfs1 * ( exp_Chi - 1.0 ) ;
    
        fs01_dPs0 = cfs1 * beta * ( exp_Chi ) ;
        fs01_dVbs   = - cfs1 * beta ;
    

        fs02    = sqrt( Xi0 + fs01 ) ;

        T2  = 0.5e0 / fs02 ;
 
        fs02_dPs0 = ( beta  + fs01_dPs0 ) * T2 ;

        fs02_dVbs   = ( - beta + fs01_dVbs ) * T2 ;
 

        flg_noqi = 0 ;


/*-------------------------------------------*
* zone-D3. (Ps0)
*-----------------*/

    } else { 

        fs01    = cfs1 * ( exp_Chi - 1.0 ) ;
    
        fs01_dPs0 = cfs1 * beta * ( exp_Chi ) ;
        fs01_dVbs   = - cfs1 * beta ;

        Xi0 = Chi - 1.0e0 ;

        Xi0p12  = sqrt( Xi0 ) ;
        Xi0p32  = Xi0 * Xi0p12 ;

        fs02    = sqrt( Xi0 + fs01 ) ;

        T2  = 0.5e0 / fs02 ;
 
        fs02_dPs0 = ( beta  + fs01_dPs0 ) * T2 ;

        fs02_dVbs   = ( - beta + fs01_dVbs ) * T2 ;
 
        /* memo: Fs0   = Vgp - Ps0 - fac1 * fs02 */

        T2  = 0.5e0 / Xi0p12 ;
 
        Fs0_dPs0    = - 1.0e0 - fac1 * fs02_dPs0 ;

        Ps0_dVbs    = - ( Vgp_dVbs
                        - ( fac1 * fs02_dVbs + fac1_dVbs * fs02 )
                        ) / Fs0_dPs0 ;
 
        Ps0_dVds    = - ( Vgp_dVds
                        - fac1_dVds * fs02
                        ) / Fs0_dPs0 ;
        Ps0_dVgs    = - ( Vgp_dVgs 
                        - fac1_dVgs * fs02
                        ) / Fs0_dPs0 ;
 
        Xi0_dVbs    = beta * ( Ps0_dVbs - 1.0e0 ) ;
        Xi0_dVds    = beta * Ps0_dVds ;
        Xi0_dVgs    = beta * Ps0_dVgs ;
 
        Xi0p12_dVbs = 0.5e0 * Xi0_dVbs / Xi0p12 ;
        Xi0p12_dVds = 0.5e0 * Xi0_dVds / Xi0p12 ;
        Xi0p12_dVgs = 0.5e0 * Xi0_dVgs / Xi0p12 ;
 
//        Xi0p32_dVbs = 1.5e0 * Xi0_dVbs * Xi0p12 ;
//        Xi0p32_dVds = 1.5e0 * Xi0_dVds * Xi0p12 ;
//        Xi0p32_dVgs = 1.5e0 * Xi0_dVgs * Xi0p12 ;

        flg_noqi = 0 ;

    } /* end of if ( Chi  ... ) block */


/*-----------------------------------------------------------*
* NOTE: The following sections of this part are only for 
*       the conductive case. 
*-----------------*/

/*-----------------------------------------------------------*
* Xi0 : beta * ( Ps0 - Vbs ) - 1 = Chi - 1 .
*-----------------*/
/*-----------------------------------------------------------*
* Qn0 : Qi at source side.
* - Qn0 := cnst0 * ( ( Xi0 + fs01 )^(1/2) - ( Xi0 )^(1/2) ) 
* - Derivatives of fs01 are redefined here.
*-----------------*/
/* note:------------------------
* fs01  = cnst1 * exp( Vbs ) * ( exp( Chi ) - Chi - 1.0e0 ) ;
* fs02  = sqrt( Xi0 + fs01 ) ;
*-------------------------------*/

    Qn0 = cnst0 * fs01 / ( fs02 + Xi0p12 ) ;

    fs01_dVbs   = Ps0_dVbs * fs01_dPs0 + fs01_dVbs ;
    fs01_dVds   = Ps0_dVds * fs01_dPs0 ;
    fs01_dVgs   = Ps0_dVgs * fs01_dPs0 ;
    fs02_dVbs   = Ps0_dVbs * fs02_dPs0 + fs02_dVbs ;
    fs02_dVds   = Ps0_dVds * fs02_dPs0 ;
    fs02_dVgs   = Ps0_dVgs * fs02_dPs0 ;
 
    Qn0_dVbs    = Qn0 
                * ( fs01_dVbs / fs01 
                  - ( fs02_dVbs + Xi0p12_dVbs ) / ( fs02 + Xi0p12 ) ) ;
    Qn0_dVds    = Qn0 
                * ( fs01_dVds / fs01 
                  - ( fs02_dVds + Xi0p12_dVds ) / ( fs02 + Xi0p12 ) ) ;
    Qn0_dVgs    = Qn0 
                * ( fs01_dVgs / fs01 
                  - ( fs02_dVgs + Xi0p12_dVgs ) / ( fs02 + Xi0p12 ) ) ;


/*-----------------------------------------------------------*
* Qb0 : Qb at source side.
*-----------------*/

  if ( Chi > znbd5 ) {

    Qb0 = cnst0 * Xi0p12 ;
    Qb0_dVb    = cnst0 * Xi0p12_dVbs ;
    Qb0_dVd    = cnst0 * Xi0p12_dVds ;
    Qb0_dVg    = cnst0 * Xi0p12_dVgs ;

  }


/*-----------------------------------------------------------*
* FD2 : connecting function for zone-D2.
*-----------------*/

    if ( Chi < znbd5 ) {

        T1 = 1.0 / ( znbd5 - znbd3 ) ;

        TX = T1 * ( Chi - znbd3 ) ;
        TX_dVbs = beta * T1 * ( Ps0_dVbs - 1.0 ) ;
        TX_dVds = beta * T1 * Ps0_dVds ;
        TX_dVgs = beta * T1 * Ps0_dVgs ;

        FD2 = TX * TX * TX * ( 10.0 + TX * ( -15.0 + TX * 6.0 ) ) ;
        T4 = TX * TX * ( 30.0 + TX * ( -60.0 + TX * 30.0 ) ) ;

        FD2_dVbs = T4 * TX_dVbs ;
        FD2_dVds = T4 * TX_dVds ;
        FD2_dVgs = T4 * TX_dVgs ;
    }


/*-----------------------------------------------------------*
* Modify Qn0 for zone-D2.
*-----------------*/

    if ( Chi < znbd5 ) {

        Qn0_dVbs = FD2 * Qn0_dVbs + FD2_dVbs * Qn0 
                 + ( 1.0 - FD2 ) * Qn00_dVbs - FD2_dVbs * Qn00 ;
        Qn0_dVds = FD2 * Qn0_dVds + FD2_dVds * Qn0 
                 + ( 1.0 - FD2 ) * Qn00_dVds - FD2_dVds * Qn00 ;
        Qn0_dVgs = FD2 * Qn0_dVgs + FD2_dVgs * Qn0 
                 + ( 1.0 - FD2 ) * Qn00_dVgs - FD2_dVgs * Qn00 ;

        Qn0 = FD2 * Qn0 + ( 1.0 - FD2 ) * Qn00 ;

        if ( Qn0 < 0.0 ) {
          Qn0 = 0.0 ;
          Qn0_dVbs = 0.0 ;
          Qn0_dVds = 0.0 ;
          Qn0_dVgs = 0.0 ;
        }

    }


/*---------------------------------------------------*
* VgVt : Vgp - Vth_qi. ( Vth_qi is Vth for Qi evaluation. ) 
*-----------------*/

    VgVt  = Qn0 * Cox_inv ;
    VgVt_dVbs = Qn0_dVbs * Cox_inv + Qn0 * Cox_inv_dVb ;
    VgVt_dVds = Qn0_dVds * Cox_inv + Qn0 * Cox_inv_dVd ;
    VgVt_dVgs = Qn0_dVgs * Cox_inv + Qn0 * Cox_inv_dVg ;


/*-----------------------------------------------------------*
* make Qi=Qd=Ids=0 if VgVt <= VgVt_small 
*-----------------*/

    if ( VgVt <= VgVt_small ) {

        Psl = Ps0 ;
        Psl_dVbs = Ps0_dVbs ;
        Psl_dVds = Ps0_dVds ;
        Psl_dVgs = Ps0_dVgs ;

        Psdl = Psl ;
        Psdl_dVbs = Psl_dVbs ;
        Psdl_dVds = Psl_dVds ;      
        Psdl_dVgs = Psl_dVgs ;

        Pds = 0.0 ;
        Pds_dVbs = 0.0 ;
        Pds_dVds = 0.0 ;
        Pds_dVgs = 0.0 ;

        T1 = - Leff * Weff ;
    	Qb = T1 * Qb0 ;
        Qb_dVbs = T1 * Qb0_dVb ;
        Qb_dVds = T1 * Qb0_dVd ;
        Qb_dVgs = T1 * Qb0_dVg ;

        Qi = 0.0 ;
        Qi_dVbs = 0.0 ;
        Qi_dVds = 0.0 ;
        Qi_dVgs = 0.0 ;
    
        Qd = 0.0 ;
        Qd_dVbs = 0.0 ;
        Qd_dVds = 0.0 ;
        Qd_dVgs = 0.0 ;
    
        Ids = 0.0e0 ;
        Ids_dVbs    = 0.0e0 ;
        Ids_dVds    = 0.0e0 ;
        Ids_dVgs    = 0.0e0 ;

        flg_noqi = 1 ;

        goto end_of_part_1 ;
    }


/*-----------------------------------------------------------*
* Start point of Psl (= Ps0 + Pds) calculation. (label)
*-----------------*/

//start_of_Psl: ;

    exp_bVbsVds    = exp( beta * ( Vbs - Vds ) ) ;

 
/*---------------------------------------------------*
* Skip Psl calculation when Vds is very small.
*-----------------*/

    if ( Vds <= epsm10 ) {
        Pds = 0.0 ;
        Psl = Ps0 ;
        goto end_of_loopl ;
    }


/*-----------------------------------------------------------*
* Initial guess for Pds ( = Psl - Ps0 ). 
*-----------------*/
 
/*---------------------------------------------------*
* Use previous value.
*-----------------*/

    if ( flg_pprv == 1  ) {

        Pds_ini  = Pds + Pds_dVbs * dVbs 
                 + Pds_dVds * dVds  + Pds_dVgs * dVgs ;

        T1 = Pds_ini - Pds ;

        if ( T1 < - dP_max || T1 > dP_max ) {
            flg_pprv = 0 ;
        }
 
    } 


/*---------------------------------------------------*
* Analytical initial guess.
*-----------------*/

    if ( flg_pprv == 0  ) {
        Pds_max = Fn_Max( Psl_lim - Ps0 , 0.0e0 ) ;

        T1  = ( 1.0e0 + c_pslini_1 ) * Pds_max ;
        T2  = T1 - Vds - c_pslini_2 ;
        T3  = sqrt( T2 * T2 + 4.0e0 * T1 * c_pslini_2 ) ;

        Pds_ini = T1 - ( T2 + T3 ) / 2 ;

        Pds_ini = Fn_Min( Pds_ini , Pds_max ) ;
    }

    if ( Pds_ini < 0.0 ) {
        Pds_ini = 0.0 ;
    } else if ( Pds_ini > Vds ) {
        Pds_ini = Vds ;
    }


/*---------------------------------------------------*
* Assign initial guess.
*-----------------*/

    Pds = Pds_ini ;
    Psl = Ps0 + Pds ;
 
 
/*---------------------------------------------------*
* Calculation of Psl by solving Poisson eqn.
* (beginning of Newton loop)
* - Fsl : Fsl = 0 is the equation to be solved.
* - dPsl : correction value.
*-----------------*/

    for ( lp_sl = 1 ; lp_sl <= lp_sl_max ; lp_sl ++ ) { 

        Chi = beta * ( Psl - Vbs ) ;


/*-------------------------------------------*
* zone-D2.  (Psl)
* - Qb0 is approximated to 5-degree polynomial.
*-----------------*/

        if ( Chi  < znbd5 ) { 

            fi  = Chi * Chi * Chi 
                * ( cn_im53 + Chi * ( cn_im54 + Chi * cn_im55 ) ) ;
            fi_dChi  = Chi * Chi 
                * ( 3 * cn_im53
                  + Chi * ( 4 * cn_im54 + Chi * 5 * cn_im55 ) ) ;

            cfs1  = cnst1 * exp_bVbsVds ;

            fsl1    = cfs1 * fi * fi ;
            fsl1_dPsl = cfs1 * beta * 2 * fi * fi_dChi ;

            fb  = Chi * ( cn_nc51 + Chi * ( cn_nc52 
                        + Chi * ( cn_nc53 
                        + Chi * ( cn_nc54 + Chi * cn_nc55 ) ) ) ) ;
            fb_dChi  =    cn_nc51 + Chi * ( 2 * cn_nc52 
                     + Chi * ( 3 * cn_nc53
                     + Chi * ( 4 * cn_nc54 + Chi * 5 * cn_nc55 ) ) ) ;

            fsl2    = sqrt( fb * fb + fsl1 ) ;
 
            if ( fsl2 >= epsm10 ) {
                fsl2_dPsl = ( beta * fb_dChi * 2 * fb + fsl1_dPsl ) 
                        / ( fsl2 + fsl2 ) ;
            } else {
                fsl2    = sqrt( fb * fb + fsl1 ) ;
                fsl2_dPsl = beta * fb_dChi ;
            }
     
            Fsl    = Vgp - Psl - fac1 * fsl2 ;
 
            Fsl_dPsl    = - 1.0e0 - fac1 * fsl2_dPsl ;
 
            dPsl  = - Fsl / Fsl_dPsl ;


/*-------------------------------------------*
* zone-D3.  (Psl)
*-----------------*/

        } else { 

            Rho = beta * ( Psl - Vds ) ;

            exp_Rho = exp( Rho ) ;

            fsl1  = cnst1 * ( exp_Rho - exp_bVbsVds ) ;
            fsl1_dPsl = cnst1 * beta * ( exp_Rho ) ;

            if ( fsl1 < epsm10 * cnst1 ) {
                fsl1 = 0.0 ;
                fsl1_dPsl = 0.0 ;
            }

            Xil = Chi - 1.0e0 ;

            Xilp12  = sqrt( Xil ) ;

            fsl2    = sqrt( Xil + fsl1 ) ;

            fsl2_dPsl = ( beta + fsl1_dPsl ) / ( fsl2 + fsl2 ) ;

            Fsl    = Vgp - Psl - fac1 * fsl2 ;

            Fsl_dPsl    = - 1.0e0 - fac1 * fsl2_dPsl ;

            dPsl  = - Fsl / Fsl_dPsl ;

        }


/*-------------------------------------------*
* Update Psl .
* - clamped to Vbs if Psl < Vbs .
*-----------------*/

        if ( fabs( dPsl ) > dP_max ) {
            dPsl  = fabs( dP_max ) * Fn_Sgn( dPsl ) ;
        }

        Psl = Psl + dPsl ;

        if ( Psl < Vbs ) {
            Psl = Vbs ;
        }


/*-------------------------------------------*
* Check convergence.
* NOTE: This condition may be too rigid.
*-----------------*/

        if ( fabs( dPsl ) <= ps_conv && fabs( Fsl ) <= gs_conv ) {
            break ;
        }

    } /* end of Psl Newton loop */


/*-------------------------------------------*
* Procedure for diverged case.
*-----------------*/

    if ( lp_sl > lp_sl_max ) {
        fprintf( stderr ,
        "*** warning(HiSIM): Went Over Iteration Maximum (Psl)\n" ) ;
        fprintf( stderr ,
            " Vbse   = %7.3f Vdse = %7.3f Vgse = %7.3f\n" ,
            Vbse , Vdse , Vgse ) ;
        if ( flg_info >= 2 ) {
          printf(
          "*** warning(HiSIM): Went Over Iteration Maximum (Psl)\n" ) ;
        }
    }


/*---------------------------------------------------*
* End of Psl calculation. (label)
*-----------------*/

end_of_loopl: ;


/*---------------------------------------------------*
* Assign Pds.
*-----------------*/

    Pds = Psl - Ps0 ;

    if ( Pds < ps_conv ) {
        Pds = 0.0 ;
        Psl = Ps0 ;
    }


/*---------------------------------------------------*
* Evaluate derivatives of  Psl.
* - note: Here, fsl1_dVbs and fsl2_dVbs are derivatives 
*   w.r.t. explicit Vbs. So, Psl in the fsl1 and fsl2
*   expressions is regarded as a constant.
*-----------------*/

    Chi = beta * ( Psl - Vbs ) ;

/*-------------------------------------------*
* zone-D2. (Psl)
*-----------------*/

    if ( Chi < znbd5 ) { 

        fi  = Chi * Chi * Chi 
            * ( cn_im53 + Chi * ( cn_im54 + Chi * cn_im55 ) ) ;
        fi_dChi  = Chi * Chi 
            * ( 3 * cn_im53
              + Chi * ( 4 * cn_im54 + Chi * 5 * cn_im55 ) ) ;

        /*note:   cfs1  = cnst1 * exp_bVbsVds */
        fsl1    = cfs1 * fi * fi ;
        fsl1_dPsl = cfs1 * beta * 2 * fi * fi_dChi ;
        fsl1_dVbs = cfs1 * beta * fi * ( fi - 2 * fi_dChi ) ;
        fsl1_dVds = - cfs1 * beta * fi * fi ;

        fb  = Chi * ( cn_nc51 + Chi * ( cn_nc52 
                    + Chi * ( cn_nc53 
                    + Chi * ( cn_nc54 + Chi * cn_nc55 ) ) ) ) ;
        fb_dChi  =    cn_nc51 + Chi * ( 2 * cn_nc52 
                 + Chi * ( 3 * cn_nc53
                 + Chi * ( 4 * cn_nc54 + Chi * 5 * cn_nc55 ) ) ) ;

        fsl2    = sqrt( fb * fb + fsl1 ) ;

        T2  = 0.5 / fsl2 ;
 
        if ( fsl2 >= epsm10 ) {
            fsl2_dPsl = ( beta * fb_dChi * 2 * fb + fsl1_dPsl ) * T2 ;
            fsl2_dVbs = ( - beta * fb_dChi * 2 * fb + fsl1_dVbs ) * T2 ;
            fsl2_dVds = fsl1_dVds * T2 ;
        } else {
            fsl2_dPsl = beta * fb_dChi ;
            fsl2_dVbs   = - beta * fb_dChi ;
            fsl2_dVds   = 0.0 ;
        }
 
        /* memo: Fsl   = Vgp - Psl - fac1 * fsl2 */
 
        Fsl_dPsl    = - 1.0e0 - fac1 * fsl2_dPsl ;
 
        Psl_dVbs    = - ( Vgp_dVbs 
                        - ( fac1 * fsl2_dVbs + fac1_dVbs * fsl2 )
                        ) / Fsl_dPsl ;
        Psl_dVds    = - ( Vgp_dVds
                        - ( fac1 * fsl2_dVds + fac1_dVds * fsl2 )
                        ) / Fsl_dPsl ;
        Psl_dVgs    = - ( Vgp_dVgs 
                         - fac1_dVgs * fsl2 
                        ) / Fsl_dPsl ;

        Pds_dVbs    = Psl_dVbs - Ps0_dVbs ;
        Pds_dVds    = Psl_dVds - Ps0_dVds ;
        Pds_dVgs    = Psl_dVgs - Ps0_dVgs ;

        Xil = Chi - 1.0e0 ;

        Xilp12  = sqrt( Xil ) ;
        Xilp32  = Xil * Xilp12 ;


/*-------------------------------------------*
* zone-D3. (Psl)
*-----------------*/

    } else { 

        Rho = beta * ( Psl - Vds ) ;

        exp_Rho = exp( Rho ) ;

        fsl1    = cnst1 * ( exp_Rho - exp_bVbsVds ) ;

        fsl1_dPsl = cnst1 * beta * ( exp_Rho ) ;

        fsl1_dVbs   = - cnst1 * beta * exp_bVbsVds ;
        fsl1_dVds   = - beta  * fsl1 ;

        if ( fsl1 < epsm10 * T1 ) {
            fsl1 = 0.0 ;
            fsl1_dPsl = 0.0 ;
            fsl1_dVds = 0.0 ;
        }

        Xil = Chi - 1.0e0 ;

        Xilp12  = sqrt( Xil ) ;
        Xilp32  = Xil * Xilp12 ;

        fsl2    = sqrt( Xil + fsl1 ) ;

        T2  = 0.5e0 / fsl2 ;

        fsl2_dPsl = ( beta  + fsl1_dPsl ) * T2 ;

        fsl2_dVbs   = ( - beta + fsl1_dVbs ) * T2 ;
        fsl2_dVds   = ( fsl1_dVds ) * T2 ;

        /* memo: Fsl   = Vgp - Psl - fac1 * fsl2 */

        T2  = 0.5e0 / Xilp12 ;

        Fsl_dPsl    = - 1.0e0 - fac1 * fsl2_dPsl ;

        Psl_dVbs    = - ( Vgp_dVbs
                        - ( fac1 * fsl2_dVbs + fac1_dVbs * fsl2 )
                        ) / Fsl_dPsl ;

        Psl_dVds    = - ( Vgp_dVds
                        - ( fac1 * fsl2_dVds + fac1_dVds * fsl2 )
                        ) / Fsl_dPsl ;
        Psl_dVgs    = - ( Vgp_dVgs 
                        - fac1_dVgs * fsl2 
                        ) / Fsl_dPsl ;

        Pds_dVbs    = Psl_dVbs - Ps0_dVbs ;
        Pds_dVds    = Psl_dVds - Ps0_dVds ;
        Pds_dVgs    = Psl_dVgs - Ps0_dVgs ;
    }

    if ( Pds < ps_conv ) {
        Pds_dVbs = 0.0 ;
        Pds_dVgs = 0.0 ;
        Psl_dVbs = Ps0_dVbs ;
        Psl_dVgs = Ps0_dVgs ;
    }


/*-----------------------------------------------------------*
* Evaluate Qb and Idd. 
* - Eta : substantial variable of QB'/Pds and Idd/Pds. 
* - note: Eta   = 4 * GAMMA_{hisim_0} 
*-----------------*/

    Eta = beta * Pds / Xi0 ;

    Eta_dVbs    = beta * ( Pds_dVbs - ( Ps0_dVbs - 1.0e0 ) * Eta ) 
                / Xi0 ;
    Eta_dVds    = beta * ( Pds_dVds - Ps0_dVds * Eta ) / Xi0 ;
    Eta_dVgs    = beta * ( Pds_dVgs - Ps0_dVgs * Eta ) / Xi0 ;

 
    /* ( Eta + 1 )^n */
    Eta1    = Eta + 1.0e0 ;
    Eta1p12 = sqrt( Eta1 ) ;
    Eta1p32 = Eta1p12 * Eta1 ;
    Eta1p52 = Eta1p32 * Eta1 ;
 
    /* 1 / ( ( Eta + 1 )^n + 1 ) */
    Zeta12  = 1.0e0 / ( Eta1p12 + 1.0e0 ) ;
    Zeta32  = 1.0e0 / ( Eta1p32 + 1.0e0 ) ;
    Zeta52  = 1.0e0 / ( Eta1p52 + 1.0e0 ) ;


/*---------------------------------------------------*
* F00 := PS00/Pds (n=1/2) 
*-----------------*/
 
    F00 = Zeta12 / Xi0p12 ;
 
    T3  = - 0.5e0 / Xi0p32 ;
    T4  = - 0.5e0 / Eta1p12 * F00 ;
 
    F00_dVbs    = Zeta12 * ( Xi0_dVbs * T3 + Eta_dVbs * T4 ) ;
    F00_dVds    = Zeta12 * ( Xi0_dVds * T3 + Eta_dVds * T4 ) ;
    F00_dVgs    = Zeta12 * ( Xi0_dVgs * T3 + Eta_dVgs * T4 ) ;


/*---------------------------------------------------*
* F10 := PS10/Pds (n=3/2) 
*-----------------*/
 
    T1  = 3.0e0 + Eta * ( 3.0e0 + Eta ) ;
 
    F10 = C_2o3 * Xi0p12 * Zeta32 * T1 ;
 
    T2  = 3.0e0 + Eta * 2.0e0 ;
    T3  = C_1o3 / Xi0p12 * T1 ;
    T4  = - 1.5e0 * Eta1p12 * F10 + C_2o3 * Xi0p12 * T2 ;

    F10_dVbs    = Zeta32 * ( Xi0_dVbs * T3 + Eta_dVbs * T4 ) ;
    F10_dVds    = Zeta32 * ( Xi0_dVds * T3 + Eta_dVds * T4 ) ;
    F10_dVgs    = Zeta32 * ( Xi0_dVgs * T3 + Eta_dVgs * T4 ) ;
    

/*---------------------------------------------------*
* F30 := PS30/Pds (n=5/2) 
*-----------------*/
 
    T1  = 5e0 
            + Eta * ( 10e0 + Eta * ( 10e0 + Eta * ( 5e0 + Eta ) ) ) ;
 
    F30 = 4e0 / ( 15e0 * beta ) * Xi0p32 * Zeta52 * T1 ;
 
    T2  = 10e0 + Eta * ( 20e0 + Eta * ( 15e0 + Eta * 4e0 ) ) ;
    T3  = 2e0 / ( 5e0 * beta ) * Xi0p12 * T1 ;
    T4  = - ( 5e0 / 2e0 ) * Eta1p32 * F30 
            + 4e0 / ( 15e0 * beta ) * Xi0p32 * T2 ;
 
    F30_dVbs    = Zeta52 * ( Xi0_dVbs * T3 + Eta_dVbs * T4 ) ;
    F30_dVds    = Zeta52 * ( Xi0_dVds * T3 + Eta_dVds * T4 ) ;
    F30_dVgs    = Zeta52 * ( Xi0_dVgs * T3 + Eta_dVgs * T4 ) ;


/*---------------------------------------------------*
* F11 := PS11/Pds. 
*-----------------*/
 
    F11 = Ps0 * F10 + C_2o3 / beta * Xilp32 - F30 ;
 
    F11_dVbs    = Ps0_dVbs * F10 + Ps0 * F10_dVbs 
            + ( Xi0_dVbs / beta + Pds_dVbs ) * Xilp12 
            - F30_dVbs ;
    F11_dVds    = Ps0_dVds * F10 + Ps0 * F10_dVds 
            + ( Xi0_dVds / beta + Pds_dVds ) * Xilp12 
            - F30_dVds ;
    F11_dVgs    = Ps0_dVgs * F10 + Ps0 * F10_dVgs 
            + ( Xi0_dVgs / beta + Pds_dVgs ) * Xilp12 
            - F30_dVgs ;


/*---------------------------------------------------*
* Fdd := Idd/Pds. 
*-----------------*/
 
    T1  = Vgp + 1.0e0 / beta - 0.5e0 * ( 2.0e0 * Ps0 + Pds ) ;
    T2  = - F10 + F00 ;
    T3  = beta * Cox ;
    T4  = beta * cnst0 ;
 
    Fdd  = T3 * T1 + T4 * T2 ;

    Fdd_dVbs = T3 * ( Vgp_dVbs - Ps0_dVbs - 0.5e0 * Pds_dVbs ) 
            + beta * Cox_dVb * T1 
            + T4 * ( - F10_dVbs + F00_dVbs ) ;
    Fdd_dVds = T3 * ( Vgp_dVds - Ps0_dVds - 0.5e0 * Pds_dVds ) 
            + beta * Cox_dVd * T1 
            + T4 * ( - F10_dVds + F00_dVds ) ;
    Fdd_dVgs = T3 * ( Vgp_dVgs - Ps0_dVgs - 0.5e0 * Pds_dVgs ) 
            + beta * Cox_dVg * T1 
            + T4 * ( - F10_dVgs + F00_dVgs ) ;


/*---------------------------------------------------*
* Q_B : bulk charge. 
*-----------------*/

    T1 = Vgp + 1.0e0 / beta ;
    T2 = T1 * F10 - F11 ;
 
    Qbnm    = cnst0 
            * ( cnst0 * ( 1.5e0 - ( Xi0 + 1.0e0 ) - 0.5e0 * beta * Pds )
              + Cox * T2 ) ;
 
    Qbnm_dVbs   = cnst0 
            * ( cnst0 * ( - Xi0_dVbs - 0.5e0 * beta * Pds_dVbs ) 
              + Cox * ( Vgp_dVbs * F10 + T1 * F10_dVbs - F11_dVbs ) 
              + Cox_dVb * T2 ) ;
    Qbnm_dVds   = cnst0 
            * ( cnst0 * ( - Xi0_dVds - 0.5e0 * beta * Pds_dVds ) 
              + Cox * ( Vgp_dVds * F10 + T1 * F10_dVds - F11_dVds ) 
              + Cox_dVd * T2 ) ;
    Qbnm_dVgs   = cnst0 
            * ( cnst0 * ( - Xi0_dVgs - 0.5e0 * beta * Pds_dVgs ) 
              + Cox * ( Vgp_dVgs * F10 + T1 * F10_dVgs - F11_dVgs ) 
              + Cox_dVg * T2 ) ;

    T1  = - Weff * beta * Leff ;
 
    Qb  = T1 * Qbnm / Fdd ;
 
    T2  = T1 / ( Fdd * Fdd ) ;
 
    Qb_dVbs = T2 * ( Fdd * Qbnm_dVbs - Qbnm * Fdd_dVbs ) ;
    Qb_dVds = T2 * ( Fdd * Qbnm_dVds - Qbnm * Fdd_dVds ) ;
    Qb_dVgs = T2 * ( Fdd * Qbnm_dVgs - Qbnm * Fdd_dVgs ) ;
    
/*---------------------------------------------------*
*  Breaking point for Qi=Qd=0.
*-----------------*/

    if ( flg_noqi != 0 ) {
      goto end_of_part_1 ;
    }

    
/*---------------------------------------------------*
*  Idd: 
*-----------------*/
 
    Idd = Pds * Fdd ;
 
    Idd_dVbs    = Pds_dVbs * Fdd + Pds * Fdd_dVbs ;
    Idd_dVds    = Pds_dVds * Fdd + Pds * Fdd_dVds ;
    Idd_dVgs    = Pds_dVgs * Fdd + Pds * Fdd_dVgs ;


/*-----------------------------------------------------------*
* Channel Length Modulation. Lred: \Delta L
*-----------------*/

      if( sIN.clm2 < epsm10 && sIN.clm3 < epsm10 ) {

          Lred = 0.0e0 ;
            Lred_dVbs = 0.0e0 ;
            Lred_dVds = 0.0e0 ;
            Lred_dVgs = 0.0e0 ;

          Psdl = Psl ;
            Psdl_dVbs = Psl_dVbs ;
            Psdl_dVds = Psl_dVds ;
            Psdl_dVgs = Psl_dVgs ;

          if ( Psdl > Ps0 + Vds - epsm10 ) {
              Psdl = Ps0 + Vds - epsm10 ;
          }

	  FMD = 1.0e0 ;

          goto end_of_CLM ;
      }

      Ec = Idd / beta / Qn0 / Leff ;
        Ec_dVbs = 1.0e0 / beta / Leff
                * ( Idd_dVbs / Qn0 - Idd * Qn0_dVbs / Qn0 / Qn0 ) ;
        Ec_dVds = ( Idd_dVds / Qn0 - Idd * Qn0_dVds / Qn0 / Qn0 )
                  / beta / Leff ;
        Ec_dVgs = 1.0e0 / beta / Leff
                * ( Idd_dVgs / Qn0 - Idd * Qn0_dVgs / Qn0 / Qn0 ) ;

	T13 = Vds ;
	T13_dVd = 1.0 ;

      /*
      T2 = Vds + Ps0 ;
        T2_dVb = Ps0_dVbs ;
        T2_dVd = 1.0 + Ps0_dVds ;
        T2_dVg = Ps0_dVgs ;
      */
      T2 = T13 + Ps0 ;
      T2_dVb = Ps0_dVbs ;
      T2_dVd = T13_dVd + Ps0_dVds ;
      T2_dVg = Ps0_dVgs ;

      T10 = sIN.clm1 ;
      
      Aclm = T10 ;
      Psdl = Aclm * T2 + ( 1.0e0 - Aclm ) * Psl ;
        Psdl_dVbs = Aclm * T2_dVb + ( 1.0e0 - Aclm ) * Psl_dVbs ;
        Psdl_dVds = Aclm * T2_dVd + ( 1.0e0 - Aclm ) * Psl_dVds ;
        Psdl_dVgs = Aclm * T2_dVg + ( 1.0e0 - Aclm ) * Psl_dVgs ;

	/*
        if ( Psdl > Ps0 + Vds - epsm10 ) {
            Psdl = Ps0 + Vds - epsm10 ;
        }
	*/
        if ( Psdl > Ps0 + T13 - epsm10 ) {
            Psdl = Ps0 + T13 - epsm10 ;
        }
      T1 = sqrt( 2.0e0 * C_ESI / q_Nsub ) ;
      T9 = sqrt( Psl - Vbs ) ;
      Wd = T1 * T9 ;
        Wd_dVbs = 0.5e0 * T1 / T9 * ( Psl_dVbs - 1.0e0 ) ;
        Wd_dVds = 0.5e0 * T1 / T9 * Psl_dVds ;
        Wd_dVgs = 0.5e0 * T1 / T9 * Psl_dVgs ;

      T7 = Qn0 / Wd ;
        T7_dVb = ( Qn0_dVbs / Wd - Qn0 / Wd / Wd * Wd_dVbs ) ;
        T7_dVd = ( Qn0_dVds / Wd - Qn0 / Wd / Wd * Wd_dVds ) ;
        T7_dVg = ( Qn0_dVgs / Wd - Qn0 / Wd / Wd * Wd_dVgs ) ;
  
      T8 = Ec * Ec + 2.0e0 / C_ESI * q_Nsub * ( Psdl - Psl ) + 1.0e5 ;
        T8_dVb = 2.0e0 * Ec * Ec_dVbs 
                + 2.0e0 / C_ESI * q_Nsub * ( Psdl_dVbs - Psl_dVbs ) ;
        T8_dVd = 2.0e0 * Ec * Ec_dVds 
                + 2.0e0 / C_ESI * q_Nsub * ( Psdl_dVds - Psl_dVds ) ;
        T8_dVg = 2.0e0 * Ec * Ec_dVgs 
                + 2.0e0 / C_ESI * q_Nsub * ( Psdl_dVgs - Psl_dVgs ) ;

      Ed = sqrt( T8 ) ;
        Ed_dVbs = 0.5e0 / Ed * T8_dVb ;
        Ed_dVds = 0.5e0 / Ed * T8_dVd ;
        Ed_dVgs = 0.5e0 / Ed * T8_dVg ;


      Lred = ( Ed - Ec )
           / ( sIN.clm2 * q_Nsub + sIN.clm3 * T7 ) * C_ESI ;
        T1 = 1.0 / ( sIN.clm2 * q_Nsub + sIN.clm3 * T7 ) ;
        T2 = T1 * T1 ;
        Lred_dVbs = ( ( Ed_dVbs - Ec_dVbs ) * T1
                   - ( Ed - Ec ) * T2 * sIN.clm3 * T7_dVb ) * C_ESI ;
        Lred_dVds = ( ( Ed_dVds - Ec_dVds ) * T1
                   - ( Ed - Ec ) * T2 * sIN.clm3 * T7_dVd ) * C_ESI ;
        Lred_dVgs = ( ( Ed_dVgs - Ec_dVgs ) * T1
                   - ( Ed - Ec ) * T2 * sIN.clm3 * T7_dVg ) * C_ESI ;


/*---------------------------------------------------*
* Modify Lred for symmetry.
*-----------------*/

	/* TX = Vds / cclmmdf ; */
        TX = T13 / cclmmdf ;
        T2 = TX * TX ;
        T5 = 1.0 + T2 ;

        FMD = 1.0 - 1.0 / T5 ;
	/* FMD_dVds = 2.0 * Vds / ( T5 * T5 * cclmmdf * cclmmdf ) ; */
        FMD_dVds = 2.0 * T13 * T13_dVd / ( T5 * T5 * cclmmdf * cclmmdf ) ;

        T6 = Lred ;

        Lred = FMD * T6 ;
        Lred_dVbs *= FMD ;
        Lred_dVds = FMD_dVds * ( T6 ) + FMD * Lred_dVds ;
        Lred_dVgs *= FMD ;


/*-------------------------------------------*
* End point of CLM. (label)
*-----------------*/

end_of_CLM: ;


/*---------------------------------------------------*
* preparation for Qi and Qd. 
*-----------------*/

    T1  = 2.0e0 * fac1 ;

    DtPds   = T1 * ( F10 - Xi0p12 ) ;

    T2  = 2.0 * ( F10 - Xi0p12 ) ;

    DtPds_dVbs  = T1 * ( F10_dVbs
                  - 0.5 * beta * ( Ps0_dVbs - 1.0e0 ) / Xi0p12 ) 
                + T2 * fac1_dVbs ;
    DtPds_dVds  = T1 * ( F10_dVds
                  - 0.5 * beta * Ps0_dVds / Xi0p12 )
                + T2 * fac1_dVds ;
    DtPds_dVgs  = T1 * ( F10_dVgs
                  - 0.5 * beta * Ps0_dVgs / Xi0p12 ) 
                + T2 * fac1_dVgs ;

    Achi    = Pds + DtPds ;
    Achi_dVbs   = Pds_dVbs + DtPds_dVbs ;
    Achi_dVds   = Pds_dVds + DtPds_dVds ;
    Achi_dVgs   = Pds_dVgs + DtPds_dVgs ;


/*-----------------------------------------------------------*
* Alpha : parameter to evaluate charges. 
* - clamped to 0 if Alpha < 0. 
*-----------------*/
 
    Alpha   = 1.0e0 - Achi / VgVt ;
 
    if ( Alpha >= 0.0e0 ) { 
 
        Alpha_dVbs  = - Achi_dVbs / VgVt + ( Achi / VgVt ) 
                * ( VgVt_dVbs / VgVt ) ;
        Alpha_dVds  = - Achi_dVds / VgVt + ( Achi / VgVt ) 
                * ( VgVt_dVds / VgVt ) ;
        Alpha_dVgs  = - Achi_dVgs / VgVt + ( Achi / VgVt ) 
                * ( VgVt_dVgs / VgVt ) ;
 
    } else { 
 
        Alpha   = 0.0e0 ;
        Alpha_dVbs  = 0.0e0 ;
        Alpha_dVds  = 0.0e0 ;
        Alpha_dVgs  = 0.0e0 ;
    }


/*-----------------------------------------------------------*
* Q_I : inversion charge.
*-----------------*/
 
    Qinm = 1.0e0 + Alpha * ( 1.0e0 + Alpha ) ;
    Qinm_dVbs = Alpha_dVbs  *  ( 1.0e0 + Alpha + Alpha ) ;
    Qinm_dVds = Alpha_dVds  *  ( 1.0e0 + Alpha + Alpha ) ;
    Qinm_dVgs = Alpha_dVgs  *  ( 1.0e0 + Alpha + Alpha ) ;
 
    Qidn = Fn_Max( 1.0e0 + Alpha , epsm10 ) ;
    Qidn_dVbs = Alpha_dVbs ;
    Qidn_dVds = Alpha_dVds ;
    Qidn_dVgs = Alpha_dVgs ;

    T1 = - Weff * ( Leff - Lred ) * C_2o3 * VgVt * Qinm / Qidn ;

    Qi = T1 * Cox ;

    Qi_dVbs = Qi * ( VgVt_dVbs / VgVt 
                   + Qinm_dVbs / Qinm - Qidn_dVbs / Qidn 
		   - Lred_dVbs/ ( Leff - Lred ) ) 
            + T1 * Cox_dVb ;
    Qi_dVds = Qi * ( VgVt_dVds / VgVt 
                   + Qinm_dVds / Qinm - Qidn_dVds / Qidn
                   - Lred_dVds/ ( Leff - Lred ) ) 
            + T1 * Cox_dVd ;
    Qi_dVgs = Qi * ( VgVt_dVgs / VgVt 
                   + Qinm_dVgs / Qinm - Qidn_dVgs / Qidn
                   - Lred_dVgs/ ( Leff - Lred ) ) 
            + T1 * Cox_dVg ;


/*-----------------------------------------------------------*
* Q_D : drain charge.
*-----------------*/
 
    Qdnm = 0.5e0 + Alpha ;
    Qdnm_dVbs = Alpha_dVbs ;
    Qdnm_dVds = Alpha_dVds ;
    Qdnm_dVgs = Alpha_dVgs ;
 
    Qddn = Qidn * Qinm ;
    Qddn_dVbs = Qidn_dVbs * Qinm + Qidn * Qinm_dVbs ;
    Qddn_dVds = Qidn_dVds * Qinm + Qidn * Qinm_dVds ;
    Qddn_dVgs = Qidn_dVgs * Qinm + Qidn * Qinm_dVgs ;
 
    Quot = 0.4e0 * Qdnm  / Qddn ;
    Qdrat = 0.6e0 - Quot ;
 
    if ( Qdrat <= 0.5e0 ) { 
        Qdrat_dVbs = Quot * ( Qddn_dVbs / Qddn - Qdnm_dVbs / Qdnm ) ;
        Qdrat_dVds = Quot * ( Qddn_dVds / Qddn - Qdnm_dVds / Qdnm ) ;
        Qdrat_dVgs = Quot * ( Qddn_dVgs / Qddn - Qdnm_dVgs / Qdnm ) ;
    } else { 
        Qdrat = 0.5e0 ;
        Qdrat_dVbs = 0.0e0 ;
        Qdrat_dVds = 0.0e0 ;
        Qdrat_dVgs = 0.0e0 ;
    } 
 
    Qd = Qi * Qdrat ;
    Qd_dVbs = Qi_dVbs * Qdrat + Qi * Qdrat_dVbs ;
    Qd_dVds = Qi_dVds * Qdrat + Qi * Qdrat_dVds ;
    Qd_dVgs = Qi_dVgs * Qdrat + Qi * Qdrat_dVgs ;


/*-----------------------------------------------------------*
* Modify charges for zone-D2.
* - FD2 must be defined previously.
*-----------------*/

    Chi = beta * ( Ps0 - Vbs ) ;

    if ( Chi < znbd5 ) {

        T1 = 1.0 / ( znbd5 - znbd3 ) ;

        TX = T1 * ( Chi - znbd3 ) ;
        TX_dVbs = beta * T1 * ( Ps0_dVbs - 1.0 ) ;
        TX_dVds = beta * T1 * Ps0_dVds ;
        TX_dVgs = beta * T1 * Ps0_dVgs ;

        T5 = - Leff * Weff ;

        Qb_dVbs = FD2 * Qb_dVbs + FD2_dVbs * Qb 
                 + ( 1.0 - FD2 ) * T5 * Qb0_dVb - FD2_dVbs * T5 * Qb0 ;
        Qb_dVds = FD2 * Qb_dVds + FD2_dVds * Qb
                 + ( 1.0 - FD2 ) * T5 * Qb0_dVd - FD2_dVds * T5 * Qb0 ;
        Qb_dVgs = FD2 * Qb_dVgs + FD2_dVgs * Qb 
                 + ( 1.0 - FD2 ) * T5 * Qb0_dVg - FD2_dVgs * T5 * Qb0 ;

        Qb = FD2 * Qb + ( 1.0 - FD2 ) * T5 * Qb0 ;

        if ( Qb > 0.0 ) {
          Qb = 0.0 ;
          Qb_dVbs = 0.0 ;
          Qb_dVds = 0.0 ;
          Qb_dVgs = 0.0 ;
        }

        Qi_dVbs = FD2 * Qi_dVbs + FD2_dVbs * Qi 
                 + ( 1.0 - FD2 ) * T5 * Qn0_dVbs - FD2_dVbs * T5 * Qn0 ;
        Qi_dVds = FD2 * Qi_dVds + FD2_dVds * Qi 
                 + ( 1.0 - FD2 ) * T5 * Qn0_dVds - FD2_dVds * T5 * Qn0 ;
        Qi_dVgs = FD2 * Qi_dVgs + FD2_dVgs * Qi 
                 + ( 1.0 - FD2 ) * T5 * Qn0_dVgs - FD2_dVgs * T5 * Qn0 ;

        Qi = FD2 * Qi + ( 1.0 - FD2 ) * T5 * Qn0 ;

        if ( Qi > 0.0 ) {
          Qi = 0.0 ;
          Qi_dVbs = 0.0 ;
          Qi_dVds = 0.0 ;
          Qi_dVgs = 0.0 ;
        }

        Qd_dVbs = FD2 * Qd_dVbs + FD2_dVbs * Qd 
                 + ( 1.0 - FD2 ) * T5 * Qi_dVbs / 2  
                 - FD2_dVbs * T5 * Qi / 2 ;
        Qd_dVds = FD2 * Qd_dVds + FD2_dVds * Qd 
                 + ( 1.0 - FD2 ) * T5 * Qi_dVds / 2 
                 - FD2_dVds * T5 * Qi / 2 ;
        Qd_dVgs = FD2 * Qd_dVgs + FD2_dVgs * Qd 
                 + ( 1.0 - FD2 ) * T5 * Qi_dVgs / 2 
                 - FD2_dVgs * T5 * Qi / 2 ;

        Qd = FD2 * Qd + ( 1.0 - FD2 ) * T5 * Qi / 2 ;

        if ( Qd > 0.0 ) {
          Qd = 0.0 ;
          Qd_dVbs = 0.0 ;
          Qd_dVds = 0.0 ;
          Qd_dVgs = 0.0 ;
        }

    }


/*-----------------------------------------------------------*
* Modified potential for symmetry. 
*-----------------*/

    T1 = exp( - ( Vds - Pds ) / ( 2.0 * sIN.pzadd0 ) ) ;

    Pzadd = sIN.pzadd0 * T1 ;

    T2 = - 0.5 * T1 ;

    Pzadd_dVbs = T2 * ( - Pds_dVbs ) ;
    Pzadd_dVds = T2 * ( 1.0 - Pds_dVds ) ;
    Pzadd_dVgs = T2 * ( - Pds_dVgs ) ;

    if ( fabs ( Pzadd ) < epsm10 ) {
        Pzadd = 0.0 ;
        Pzadd_dVbs = 0.0 ;
        Pzadd_dVds = 0.0 ;
        Pzadd_dVgs = 0.0 ;
    }

    Ps0z = Ps0 + Pzadd ;
    Ps0z_dVbs = Ps0_dVbs + Pzadd_dVbs ;
    Ps0z_dVds = Ps0_dVds + Pzadd_dVds ;
    Ps0z_dVgs = Ps0_dVgs + Pzadd_dVgs ;


/*-----------------------------------------------------------*
* Evaluate universal mobility. 
*-----------------*/

    Ps0Vbsz = Ps0z - Vbsz ;
    Ps0Vbsz_dVbs = Ps0z_dVbs - Vbsz_dVbs ;
    Ps0Vbsz_dVds = Ps0z_dVds - Vbsz_dVds ;
    Ps0Vbsz_dVgs = Ps0z_dVgs ;


/*-------------------------------------------*
* Qbm
*-----------------*/

    T1 = cnst0 ;
    T2 = sqrt( beta * Ps0Vbsz - 1.0 ) ;

    Qbm = T1 * T2 ;

    T3 = 0.5 * beta * T1 / T2 ;

    Qbm_dVbs = T3 * Ps0Vbsz_dVbs ;
    Qbm_dVds = T3 * Ps0Vbsz_dVds ;
    Qbm_dVgs = T3 * Ps0Vbsz_dVgs ;


/*-------------------------------------------*
* Qnm
*-----------------*/

    Chi = beta * Ps0Vbsz ;

    exp_Chi = exp( Chi ) ;

    exp_bVbs    = exp( beta * Vbsz ) ;

    cfs1  = cnst1 * exp_bVbs ;

    fs01    = cfs1 * ( exp_Chi - 1.0 ) ;

    /* regard fs01 as a function of Chi and Vbs */
    fs01_dChi = cfs1 * ( exp_Chi ) ;
    fs01_dVbs   = beta * fs01 ;


    if ( fs01 < epsm10 * cfs1 ) {
        fs01 = 0.0 ;
        fs01_dPs0 = 0.0 ;
        fs01_dVbs = 0.0 ;
    }
 
    Xi0z = Chi - 1.0e0 ;
 
    Xi0z_dVbs    = beta * Ps0Vbsz_dVbs ;
    Xi0z_dVds    = beta * Ps0Vbsz_dVds ;
    Xi0z_dVgs    = beta * Ps0Vbsz_dVgs ;

    fs02    = sqrt( Xi0z + fs01 ) ;

    T2  = 0.5e0 / fs02 ;
 
    /* regard fs01 as a function of Chi and Vbs */
    fs02_dChi = ( 1.0  + fs01_dChi ) * T2 ;
    fs02_dVbs   = fs01_dVbs * T2 ;

    Xi0zp12  = sqrt( Xi0z ) ;
 
    Xi0zp12_dVbs = 0.5e0 * Xi0z_dVbs / Xi0zp12 ;
    Xi0zp12_dVds = 0.5e0 * Xi0z_dVds / Xi0zp12 ;
    Xi0zp12_dVgs = 0.5e0 * Xi0z_dVgs / Xi0zp12 ;

    T1 = 1.0 / ( fs02 + Xi0zp12 ) ;
 
    Qnm = cnst0 * fs01 * T1 ;

    fs01_dVds   = beta * Ps0Vbsz_dVds * fs01_dChi 
                + fs01_dVbs * Vbsz_dVds ;
    fs01_dVbs   = beta * Ps0Vbsz_dVbs * fs01_dChi + fs01_dVbs ;
    fs01_dVgs   = beta * Ps0Vbsz_dVgs * fs01_dChi ;

    fs02_dVds   = beta * Ps0Vbsz_dVds * fs02_dChi 
                + fs02_dVbs * Vbsz_dVds ;
    fs02_dVbs   = beta * Ps0Vbsz_dVbs * fs02_dChi + fs02_dVbs ;
    fs02_dVgs   = beta * Ps0Vbsz_dVgs * fs02_dChi ;
 
    Qnm_dVbs  = Qnm 
              * ( fs01_dVbs / fs01 
                - ( fs02_dVbs + Xi0zp12_dVbs ) * T1 ) ;
    Qnm_dVds  = Qnm 
              * ( fs01_dVds / fs01 
                - ( fs02_dVds + Xi0zp12_dVds ) * T1 ) ;
    Qnm_dVgs  = Qnm 
              * ( fs01_dVgs / fs01 
                - ( fs02_dVgs + Xi0zp12_dVgs ) * T1 ) ;

    if ( Qbm < 0.0 ) {
      Qbm = 0.0 ;
    }
    if ( Qnm < 0.0 ) {
      Qnm = 0.0 ;
    }


/*---------------------------------------------------*
* Muun : universal mobility. 
*-----------------*/

    /* removed Eqs. "Qbm = Qb0", "Qnm = Qn0", ... (4/Dec/01) */

    T1 = sIN.ninv - sIN.ninvd * Vdsz ;
    T1_dVd = - sIN.ninvd * Vdsz_dVds ;

    Eeff    = ( sIN.ndep * Qbm + T1 * Qnm ) / C_ESI ;
    Eeff_dVbs   = ( sIN.ndep * Qbm_dVbs + T1 * Qnm_dVbs ) 
                / C_ESI ;
    Eeff_dVds   = ( sIN.ndep * Qbm_dVds + T1 * Qnm_dVds
                  + T1_dVd * Qnm ) 
                / C_ESI ;
    Eeff_dVgs   = ( sIN.ndep * Qbm_dVgs + T1 * Qnm_dVgs ) 
                / C_ESI ;
 
    Rns = Qnm / C_QE ;

    T10 = pow( sIN.temp / C_T300 , sIN.muetmp ) ;
    T21 = pow( Eeff , sIN.mueph0 - 1.0e0 ) ;
    T20 = T21 * Eeff ;

    T11 = sIN.muesr0 ;
    
    T31 = pow( Eeff , T11 - 1.0e0 ) ;
    T30 = T31 * Eeff ;
    /*
    T31 = pow( Eeff , sIN.muesr0 - 1.0e0 ) ;
    T30 = T31 * Eeff ;
    */

    T1  = 1e0 / ( sIN.muecb0 + sIN.muecb1 * Rns / 1e11 ) 
            + T10
            * T20 / mueph 
            + T30 / sIN.muesr1 ;
 
    Muun    = 1e0 / T1 ;
 
    T1  = 1e0 / ( T1 * T1 ) ;
    T2  = sIN.muecb0 + sIN.muecb1 * Rns / 1e11 ;
    T2  = 1e0 / ( T2 * T2 ) ;
    T3  = T10 * sIN.mueph0 * T21 / mueph ;
    T4  = T11 * T31 / sIN.muesr1 ;
    /*
    T4  = sIN.muesr0 * T31 / sIN.muesr1 ;
    */

    Muun_dVbs   = - 1 * ( - 1e-11 * sIN.muecb1 * Qnm_dVbs / C_QE * T2 
                   + Eeff_dVbs * T3 
                   + Eeff_dVbs * T4 ) 
            * T1 ;
    Muun_dVds   = - 1 * ( - 1e-11 * sIN.muecb1 * Qnm_dVds / C_QE * T2 
                   + Eeff_dVds * T3 
                   + Eeff_dVds * T4 ) 
            * T1 ;
    Muun_dVgs   = - 1 * ( - 1e-11 * sIN.muecb1 * Qnm_dVgs / C_QE * T2 
                   + Eeff_dVgs * T3 
                   + Eeff_dVgs * T4 ) 
            * T1 ;


/*-----------------------------------------------------------*
* Mu : mobility 
*-----------------*/
 
      T1 = 1.0e0 / beta / ( Qn0 + small ) / ( Leff - Lred ) ;
        T1_dVb = 1.0e0 / beta * ( - Qn0_dVbs / ( Qn0 + small ) 
                / ( Qn0 + small ) / ( Leff - Lred )
                 + Lred_dVbs / ( Qn0 + small ) / ( Leff - Lred ) 
                / ( Leff - Lred ) ) ;
        T1_dVd = 1.0e0 / beta * ( - Qn0_dVds / ( Qn0 + small ) 
                / ( Qn0 + small ) / ( Leff - Lred )
                 + Lred_dVds / ( Qn0 + small ) / ( Leff - Lred ) 
                / ( Leff - Lred ) ) ;
        T1_dVg = 1.0e0 / beta * ( - Qn0_dVgs / ( Qn0 + small ) 
                / ( Qn0 + small ) / ( Leff - Lred )
                 + Lred_dVgs / ( Qn0 + small ) / ( Leff - Lred ) 
                / ( Leff - Lred ) ) ;

      Ey = Idd * T1 ;
        Ey_dVbs = Idd_dVbs * T1 + Idd * T1_dVb ;
        Ey_dVds = Idd_dVds * T1 + Idd * T1_dVd ;
        Ey_dVgs = Idd_dVgs * T1 + Idd * T1_dVg ;

    Em  = Muun * Ey ;
    Em_dVbs = Muun_dVbs * Ey + Muun * Ey_dVbs ;
    Em_dVds = Muun_dVds * Ey + Muun * Ey_dVds ;
    Em_dVgs = Muun_dVgs * Ey + Muun * Ey_dVgs ;

    T1 = sIN.temp / C_T300 ;

    Vmax = sIN.vmax / ( 1.8 + 0.4 * T1 + 0.1 * T1 * T1 ) ;

    T2 = 1.0 - sIN.vover / pow( Leff , sIN.voverp ) ;

    if ( T2 < 0.01 ) {
        fprintf( stderr , 
                 "*** warning(HiSIM): Overshoot is too big.\n" ) ;
        T2 = 0.01 ;
    }

    Vmax = Vmax / T2 ;
 
    T1  = Em / Vmax ;

    /* note: sIN.bb = 2 (electron) ;1 (hole) */
    if ( 1.0e0 - epsm10 <= sIN.bb && sIN.bb <= 1.0e0 + epsm10 ) {
        T3  = 1.0e0 ;
    } else if ( 2.0e0 - epsm10 <= sIN.bb
             && sIN.bb <= 2.0e0 + epsm10 ) {
        T3  = T1 ;
    } else {
        T3  = pow( T1 , sIN.bb - 1.0e0 ) ;
    }
    T2  = T1 * T3 ;
    T4  = 1.0e0 + T2 ;
    T6  = pow( T4 , ( - 1.0e0 / sIN.bb - 1.0e0 ) ) ;
    T5  = T4 * T6 ;
 
    Mu  = Muun * T5 ;
    Mu_dVbs = Muun_dVbs * T5 - Muun / Vmax * T6 * T3 * Em_dVbs ;
    Mu_dVds = Muun_dVds * T5 - Muun / Vmax * T6 * T3 * Em_dVds ;
    Mu_dVgs = Muun_dVgs * T5 - Muun / Vmax * T6 * T3 * Em_dVgs ;

//end_of_mobility : ;


/*-----------------------------------------------------------*
* Ids: channel current.
*-----------------*/

    T1 = Weff / beta / ( Leff - Lred ) ;
      T1_dVb = T1 / ( Leff - Lred ) * Lred_dVbs ;
      T1_dVd = T1 / ( Leff - Lred ) * Lred_dVds ;
      T1_dVg = T1 / ( Leff - Lred ) * Lred_dVgs ;

    Ids0 = T1 * Idd * Mu ;
      Ids0_dVbs    = T1 * ( Mu * Idd_dVbs + Idd * Mu_dVbs ) 
                  + T1_dVb * Mu * Idd ;
      Ids0_dVds    = T1 * ( Mu * Idd_dVds + Idd * Mu_dVds ) 
                  + T1_dVd * Mu * Idd ;
      Ids0_dVgs    = T1 * ( Mu * Idd_dVgs + Idd * Mu_dVgs ) 
                  + T1_dVg * Mu * Idd ;

    T1 = Vdsz + sIN.rpock2 ;
    T2 = T1 * T1 + small ;
      T2_dVb = 0.0e0 ; 
      T2_dVd = 2.0 * T1 * Vdsz_dVds ;
      T2_dVg = 0.0e0 ; 
    
    rp1 = FMD * sIN.rpock1 ;
    rp1_dVds = FMD_dVds * sIN.rpock1 ;

    TX = pow( Ids0, sIN.rpocp1 ) * pow( Leff / 1.0e-4 , sIN.rpocp2 ) / Weff ;

    T3 = rp1 * TX ;
    T4 = ( T3 + T2 ) * ( T3 + T2 ) ;

    Ids = Ids0 / ( 1.0 + T3 / T2 ) ;

    T5 = T2 * ( T2 - ( sIN.rpocp1 - 1.0 ) * T3 ) / T4 ;
    T6 = Ids0 * T3 / T4 ;

      Ids_dVbs = T5 * Ids0_dVbs + T6 * T2_dVb ;
      Ids_dVds = T5 * Ids0_dVds + T6 * T2_dVd 
               - Ids0 * T2 / T4 * rp1_dVds * TX ;
      Ids_dVgs = T5 * Ids0_dVgs + T6 * T2_dVg ;

      if ( Pds < ps_conv ) {
        Ids_dVbs = 0.0 ;
        Ids_dVgs = 0.0 ;
      }

    Ids += Gdsmin * Vds ;
    Ids_dVds += Gdsmin ;


/*-----------------------------------------------------------*
* STI
*-----------------*/

    if ( sIN.coisti == 0 ) {
        goto end_of_STI ;
    }

      Vth_dVb = Vthp_dVb - dVth_dVb ;
      Vth_dVd = Vthp_dVd - dVth_dVd ;
      Vth_dVg = Vthp_dVg - dVth_dVg ;

     Vgssti = Vgs - Vfb + Vth * sIN.wvthsc ;
      Vgssti_dVbs = Vth_dVb * sIN.wvthsc ;
      Vgssti_dVds = Vth_dVd * sIN.wvthsc ;
      Vgssti_dVgs = 1.0e0 + Vth_dVg * sIN.wvthsc ;
      
     costi0 = sqrt (2.0e0 * C_QE * sIN.nsti * C_ESI / beta) ;
     costi1 = Nin * Nin / sIN.nsti / sIN.nsti ;
     costi3 = costi0 * costi0 / Cox / Cox ;
     costi4 = costi3 * beta / 2.0e0 ;
     costi5 = costi4 * beta * 2.0e0 ;
     T1 = 1.0e0 + 4.0e0 * ( beta * ( Vgssti - Vbs ) - 1.0e0 ) / costi5 ;
     if ( T1 > 0.0 ) {
       costi6 = sqrt( T1 ) ;
       Psasti = Vgssti + costi4 * (1.0e0 - costi6) ;
       Psasti_dVbs = Vgssti_dVbs - (Vgssti_dVbs - 1.0e0)/costi6 ;
       Psasti_dVds = Vgssti_dVds - Vgssti_dVds/costi6 ;
       Psasti_dVgs = Vgssti_dVgs - Vgssti_dVgs/costi6 ;    
     } else {
       Psasti = Psasti_dVbs = Psasti_dVds = Psasti_dVgs = 0.0 ;
     }
       
     Asti = 1.0e0 / costi1 / costi3 ;
     Psbsti = log(Asti * (Vgssti*Vgssti)) / (beta + 2.0e0 / Vgssti) ;
      
      Psbsti_dVbs = 2.0e0 / (beta*Vgssti + 2.0e0) 
                    * (1.0e0 + Psbsti/Vgssti) * Vgssti_dVbs ;
      Psbsti_dVds = 2.0e0 / (beta*Vgssti + 2.0e0) 
                    * (1.0e0 + Psbsti/Vgssti) * Vgssti_dVds ;
      Psbsti_dVgs = 2.0e0 / (beta*Vgssti + 2.0e0) 
                    * (1.0e0 + Psbsti/Vgssti) * Vgssti_dVgs ;
                
     Psab = Psbsti - Psasti - sti2_dlt ;
      
      Psab_dVbs = Psbsti_dVbs - Psasti_dVbs ;
      Psab_dVds = Psbsti_dVds - Psasti_dVds ;
      Psab_dVgs = Psbsti_dVgs - Psasti_dVgs ;     
      
     Psti = Psbsti - 0.5e0 * (Psab 
           + sqrt(Psab * Psab + 4.0e0 * sti2_dlt * Psbsti)) ;
     
      Psti_dVbs = Psbsti_dVbs - 0.5e0 * Psab_dVbs 
               - (Psab * Psab_dVbs +2.0e0 * sti2_dlt * Psbsti_dVbs)
              * 0.5e0 / sqrt(Psab * Psab + 4.0e0 * sti2_dlt * Psbsti) ;
      Psti_dVds = Psbsti_dVds - 0.5e0 * Psab_dVds
               - (Psab * Psab_dVds +2.0e0 * sti2_dlt * Psbsti_dVds)
             * 0.5e0 / sqrt(Psab * Psab + 4.0e0 * sti2_dlt * Psbsti) ;
      Psti_dVgs = Psbsti_dVgs - 0.5e0 * Psab_dVgs
               - (Psab * Psab_dVgs +2.0e0 * sti2_dlt * Psbsti_dVgs)
              * 0.5e0 / sqrt(Psab * Psab + 4.0e0 * sti2_dlt * Psbsti) ;

     expsti = exp(beta * Psti) ;
     sq1sti = sqrt(beta * (Psti - Vbs) - 1.0e0 +costi1 * expsti) ;

      sq1sti_dVbs = 0.50e0 * (beta * (Psti_dVbs - 1.0e0) 
                 + costi1 * beta * Psti_dVbs * expsti) / sq1sti ;
      sq1sti_dVds = 0.50e0 * (beta * Psti_dVds 
                 + costi1 * beta * Psti_dVds * expsti) / sq1sti ;
      sq1sti_dVgs = 0.50e0 * (beta * Psti_dVgs  
                 + costi1 * beta * Psti_dVgs * expsti) / sq1sti ;
     
     sq2sti = sqrt(beta * (Psti - Vbs) - 1.0e0) ;
      
      sq2sti_dVbs = 0.50e0 * beta *(Psti_dVbs - 1.0e0) / sq2sti ;
      sq2sti_dVds = 0.50e0 * beta *Psti_dVds / sq2sti ;
      sq2sti_dVgs = 0.50e0 * beta *Psti_dVgs / sq2sti ;    
                       
     Qn0sti = costi0 * (sq1sti - sq2sti) ;     
      Qn0sti_dVbs = costi0 * (sq1sti_dVbs-sq2sti_dVbs) ;
      Qn0sti_dVds = costi0 * (sq1sti_dVds-sq2sti_dVds) ;
      Qn0sti_dVgs = costi0 * (sq1sti_dVgs-sq2sti_dVgs) ;
           
     costi7 = 2.0e0 * sIN.wsti / beta ;
     Idssti = costi7 * Mu * Qn0sti * (1.0e0 - exp(-beta * Vds))
            / (Leff - Lred) ;
  
      Idssti_dVbs = costi7 * 
	((( Mu_dVbs * Qn0sti + Mu * Qn0sti_dVbs ) * 
	  ( 1.0e0 - exp( - beta * Vds ))) / ( Leff - Lred )
	 + Lred_dVbs * Mu * Qn0sti * ( 1.0e0 - exp( - beta * Vds ))
	 / ( Leff - Lred ) / ( Leff - Lred )) ;
      Idssti_dVds = costi7 * 
	((( Mu_dVds * Qn0sti + Mu * Qn0sti_dVds ) * 
	  ( 1.0e0 - exp( - beta * Vds ))
	  + beta * Mu * Qn0sti * exp( - beta * Vds )) / ( Leff - Lred )
	 + Lred_dVds * Mu * Qn0sti * ( 1.0e0 - exp( - beta * Vds ))
	 / ( Leff - Lred ) / ( Leff - Lred )) ;
      Idssti_dVgs = costi7 * 
	((( Mu_dVgs * Qn0sti + Mu * Qn0sti_dVgs ) * 
	  ( 1.0e0 - exp( - beta * Vgs ))) / ( Leff - Lred )
	 + Lred_dVgs * Mu * Qn0sti * ( 1.0e0 - exp( - beta * Vds ))
	 / ( Leff - Lred ) / ( Leff - Lred )) ;
         
      Ids = Ids + Idssti ;

      Ids_dVbs = Ids_dVbs + Idssti_dVbs ;
      Ids_dVds = Ids_dVds + Idssti_dVds ;
      Ids_dVgs = Ids_dVgs + Idssti_dVgs ;

   end_of_STI: ;


/*-----------------------------------------------------------*
* Break point for the case of Rs=Rd=0.
*-----------------*/

    if ( flg_rsrd == 0 ) {
        DJI = 1.0 ;
	DJIP = 1.0 ;
        break ;
    }


/*-----------------------------------------------------------*
* calculate corrections of biases.
* - Fbs = 0, etc. are the small ciucuit equations.
* - DJ, Jacobian of the small circuit matrix, is g.t. 1 
*   provided Rs, Rd and conductances are positive.
*-----------------*/

    Fbs = Vbs - Vbsc + Ids * Rs ;
    Fds = Vds - Vdsc + Ids * ( Rs + Rd ) ;
    Fgs = Vgs - Vgsc + Ids * Rs ;

    DJ = 1.0 + Rs * Ids_dVbs + ( Rs + Rd ) * Ids_dVds + Rs * Ids_dVgs ;
    DJI = 1.0 / DJ ;

    JI11 = 1 + ( Rs + Rd ) * Ids_dVds + Rs * Ids_dVgs ;
    JI12 = - Rs * Ids_dVds ;
    JI13 = - Rs * Ids_dVgs ;
    JI21 = - ( Rs + Rd ) * Ids_dVbs ;
    JI22 = 1 + Rs * Ids_dVbs + Rs * Ids_dVgs ;
    JI23 = - ( Rs + Rd ) * Ids_dVgs ;
    JI31 = - Rs * Ids_dVbs ;
    JI32 = - Rs * Ids_dVds ;
    JI33 = 1 + Rs * Ids_dVbs + ( Rs + Rd ) * Ids_dVds ;

    dVbs = - DJI * ( JI11 * Fbs + JI12 * Fds + JI13 * Fgs ) ;
    dVds = - DJI * ( JI21 * Fbs + JI22 * Fds + JI23 * Fgs ) ;
    dVgs = - DJI * ( JI31 * Fbs + JI32 * Fds + JI33 * Fgs ) ;

    dV_sum = fabs( dVbs ) + fabs( dVds ) + fabs( dVgs ) ; 


/*-----------------------------------------------------------*
* Break point for converged case.
* - Exit from the bias loop.
* - NOTE: Update of internal biases is avoided.
*-----------------*/

    if ( Ids_last * Ids_tol >= fabs( Ids_last - Ids ) ||
         dV_sum < ps_conv ) {
        break ;
    }


/*-----------------------------------------------------------*
* Update the internal biases.
*-----------------*/

    Vbs  = Vbs + dVbs ;
    Vds  = Vds + dVds ;
    Vgs  = Vgs + dVgs ;

    if ( Vds < 0.0 ) { 
        Vds  = 0.0 ; 
        dVds = 0.0 ; 
    } 


/*-----------------------------------------------------------*
* Bottom of bias loop. (label) 
*-----------------*/

//bottom_of_bias_loop : ;


/*-----------------------------------------------------------*
* Make initial guess flag of potential ON.
* - This effects for the 2nd and later iterations of bias loop.
*-----------------*/

    flg_pprv = 1 ;

  } /*++ End of the bias loop +++++++++++++++++++++++++++++*/

  if ( lp_bs > lp_bs_max ) { lp_bs -- ; }


/*-----------------------------------------------------------*
* End of PART-1. (label) 
*-----------------*/

end_of_part_1: ;


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
* PART-2: Substrate / gate / leak currents
*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
 
/*-----------------------------------------------------------*
* Isub : substrate current induced by impact ionization.
*-----------------*/
/*-------------------------------------------*
* Accumulation zone or nonconductive case, in which Ids==0. 
*-----------------*/

    if ( Ids <= 0.0e0 || sIN.coisub == 0 ) {
        Isub    = 0.0e0 ;
        Isub_dVbs   = 0.0e0 ;
        Isub_dVds   = 0.0e0 ;
        Isub_dVgs   = 0.0e0 ;
        goto end_of_Isub ;
    }


/*-------------------------------------------*
* Conductive case. 
*-----------------*/

    if ( sIN.sub1 > 0.0e0 && sIN.vmax > 0.0e0 ) {

        Vdep    = Vds + Ps0 - sIN.sub3 * Psl ;
        Vdep_dVbs   = Ps0_dVbs - sIN.sub3 * Psl_dVbs ;
        Vdep_dVds   = 1.0e0 + Ps0_dVds - sIN.sub3 * Psl_dVds ;
        Vdep_dVgs   = Ps0_dVgs - sIN.sub3 * Psl_dVgs ;

        TX  = - sIN.sub2 / Vdep ;

            Epkf    = exp( TX ) ;
            Epkf_dVbs   = - TX * Vdep_dVbs / Vdep * Epkf ;
            Epkf_dVds   = - TX * Vdep_dVds / Vdep * Epkf ;
            Epkf_dVgs   = - TX * Vdep_dVgs / Vdep * Epkf ;

        T1 = Ids * Epkf ;
          T1_dVb = Ids_dVbs * Epkf + Ids * Epkf_dVbs ;
          T1_dVd = Ids_dVds * Epkf + Ids * Epkf_dVds ;
          T1_dVg = Ids_dVgs * Epkf + Ids * Epkf_dVgs ;

        if( T1 < 1.0e-25 ){
        T1 = 1.0e-25 ;
          T1_dVb = 0.0e0 ;
          T1_dVd = 0.0e0 ;
          T1_dVg = 0.0e0 ;
        }

        Isub = sIN.sub1 * Vdep * T1 ;
          Isub_dVbs = sIN.sub1 * ( Vdep_dVbs * T1 + Vdep * T1_dVb ) ;
          Isub_dVds = sIN.sub1 * ( Vdep_dVds * T1 + Vdep * T1_dVd ) ;
          Isub_dVgs = sIN.sub1 * ( Vdep_dVgs * T1 + Vdep * T1_dVg ) ;

    } else {

        Isub    = 0.0e0 ;
        Isub_dVbs   = 0.0e0 ;
        Isub_dVds   = 0.0e0 ;
        Isub_dVgs   = 0.0e0 ;

    } /* end of if ( sIN.sub1 ... ) else block. */


/*-------------------------------------------*
* End of Isub. (label) 
*-----------------*/

end_of_Isub: ;

 
/*-----------------------------------------------------------*
* Igate : Gate current induced by tunneling.
* - Vzadd is used for symmetrizing.
*-----------------*/

    Egp12 = sqrt( Eg ) ;
    Egp32 = Eg * Egp12 ;

    if ( sIN.coiigs == 0 ) {
        Igate    = 0.0e0 ;
        Igate_dVbs   = 0.0e0 ;
        Igate_dVds   = 0.0e0 ;
        Igate_dVgs   = 0.0e0 ;
        goto end_of_Igate ;
    }
  
    Vgpz = Vgp + Vzadd ; 

    Vgpz_dVbs = Vgp_dVbs ;
    Vgpz_dVds = Vgp_dVds + Vzadd_dVds ;
    Vgpz_dVgs = Vgp_dVgs ;

    T1  = Vgpz - ( Psl + Ps0 + 2.0 * Vzadd ) * sIN.gleak3 ;

    E0  = 10.0 ;

    E2  = T1 / Tox ;

    E2_dVbs = E2
            * ( ( Vgpz_dVbs - ( Psl_dVbs + Ps0_dVbs ) * sIN.gleak3 )
                / T1
              - Tox_dVb / Tox ) ;
    E2_dVds = E2 
            * ( ( Vgpz_dVds
                - ( Psl_dVds + Ps0_dVds + 2.0 * Vzadd_dVds )
                * sIN.gleak3
                ) / T1 
              - Tox_dVd / Tox ) ;
    E2_dVgs = E2
            * ( ( Vgpz_dVgs - ( Psl_dVgs + Ps0_dVgs ) * sIN.gleak3 )
                / T1
              - Tox_dVg / Tox ) ;

    T1  = E2 - E0 - eef_dlt ;
    T2  = T1 * T1 ;

    T3  = sqrt( T2 + 4.0 * eef_dlt * E2 ) ;

    Etun  = E2 - ( T1 - T3 ) / 2 ;
    if ( Etun < 0 ) {
        Igate    = 0.0e0 ;
        Igate_dVbs   = 0.0e0 ;
        Igate_dVds   = 0.0e0 ;
        Igate_dVgs   = 0.0e0 ;
        goto end_of_Igate ;
    }

    T4 = 1.0 / ( 4.0 * T3 ) ;
    T5 =  2.0 * T1 * T4 + 0.5 ;

    Etun_dVbs = T5 * E2_dVbs ;
    Etun_dVds = T5 * E2_dVds ;
    Etun_dVgs = T5 * E2_dVgs ;

    T1 = exp( - sIN.gleak2 * Egp32 / Etun ) ;

    T2 = sIN.gleak1 / Egp12 * C_QE * Weff * Leff ;

    Igate = T2 * Etun * Etun * T1 ;

    T3 = T2 * T1 * ( 2.0 * Etun + sIN.gleak2 * Egp32 ) ;

    Igate_dVbs = T3 * Etun_dVbs ;
    Igate_dVds = T3 * Etun_dVds ;
    Igate_dVgs = T3 * Etun_dVgs ;

  end_of_Igate: ;
 

/*-----------------------------------------------------------*
* Igidl : GIDL 
*-----------------*/

    if ( (sIN.mode == HiSIM_NORMAL_MODE && sIN.cogidl == 0) ||
	 (sIN.mode == HiSIM_REVERSE_MODE && sIN.cogisl == 0) ) {
        Igidl    = 0.0e0 ;
        Igidl_dVbs   = 0.0e0 ;
        Igidl_dVds   = 0.0e0 ;
        Igidl_dVgs   = 0.0e0 ;
        goto end_of_IGIDL ;
    }

    T1 = sIN.gidl3 * Vdsz - Vgsz + Vfb - dVth + dPpg ;

    E1 = T1 / Tox ;

    E1_dVb = E1 * ( ( - dVth_dVb + dPpg_dVb ) / T1 - Tox_dVb / Tox ) ;
    E1_dVd = E1 * ( ( sIN.gidl3 * Vdsz_dVds 
		       - Vgsz_dVds - dVth_dVd + dPpg_dVd) / T1 
		     - Tox_dVd / Tox ) ;
    E1_dVg = E1 * ( ( -1.0 - dVth_dVg + dPpg_dVg) / T1 - Tox_dVg / Tox ) ;

    T1 = E0 - E1 - eef_dlt ;
    T2 = T1 * T1 ;

    T3 = sqrt( T2 + 4.0 * eef_dlt * E0 ) ;

    Egidl = E0 - ( T1 - T3 ) / 2 ;

    T4 = 1.0 / ( 4.0 * T3 ) ;
    T5 =  - 2.0 * T1 * T4 + 0.5 ;

    Egidl_dVb = T5 * E1_dVb ;
    Egidl_dVd = T5 * E1_dVd ;
    Egidl_dVg = T5 * E1_dVg ;

    T1 = exp( - sIN.gidl2 * Egp32 / Egidl ) ;

    T2 = sIN.gidl1 / Egp12 * C_QE * Weff ;

    Igidl = T2 * Egidl * Egidl * T1 ;

    T3 = T2 * T1 * ( 2.0 * Egidl + sIN.gidl2 * Egp32 ) ;

    Igidl_dVbs = T3 * Egidl_dVb ;
    Igidl_dVds = T3 * Egidl_dVd ;
    Igidl_dVgs = T3 * Egidl_dVg ;
      
  end_of_IGIDL: ;


/*-----------------------------------------------------------*
* Igisl : GISL
*-----------------*/

    if ( (sIN.mode == HiSIM_NORMAL_MODE && sIN.cogisl == 0) ||
	 (sIN.mode == HiSIM_REVERSE_MODE && sIN.cogidl == 0) ) {
        Igisl    = 0.0e0 ;
        Igisl_dVbd   = 0.0e0 ;
        Igisl_dVsd   = 0.0e0 ;
        Igisl_dVgd   = 0.0e0 ;
        goto end_of_IGISL ;
    }

/* dVthP and dPpgP are required for GISL */

/* internal biases (drain reference) */
    Vsd = -Vds ;
    Vgd = Vgs - Vds ;
    Vbd = Vbs - Vds ;
    
    T1 = exp(/* - */Vbdc_dVbde * Vsd / ( 2.0 * sIN.vzadd0 ) ) ;
    Vzadd = sIN.vzadd0 * T1 ;
    Vzadd_dVsd = 0.5 * Vbdc_dVbde * T1 ;
    
    if ( Vzadd < ps_conv ) {
      Vzadd = 0.0 ;
      Vzadd_dVsd = 0.0 ;
    }

    Vbdz = Vbd + Vzadd ;
//    Vbdz_dVgd = 0.0 ;
    Vbdz_dVsd = Vzadd_dVsd ;
    Vbdz_dVbd = 1.0 ;
    
    Vsdz = Vsd + 2 * Vzadd ;
//    Vsdz_dVgd = 0.0 ;
    Vsdz_dVsd = 1.0 + 2 * Vzadd_dVsd ;
//    Vsdz_dVbd = 0.0 ;
    
    Vgdz = Vgd + Vzadd ;
    Vgdz_dVgd = 1.0 ;
    Vgdz_dVsd = Vzadd_dVsd ;
//    Vgdz_dVbd = 0.0 ;

/* Quantum effect (GISL) */

    T9  = sIN.qme1 ;
    T11 = sIN.qme3 ;
    T10  = sIN.qme2 ;
    
    T1 = 2.0 * q_Nsub * C_ESI ;
    T2 = sqrt( T1 * ( Pb20 - Vbdz ) ) ;
    
    Vthq = Pb20 + Vfb + sIN.tox / c_eox * T2 + T10 ;
    
    T3 = - 0.5 * sIN.tox / c_eox * T1 / T2 ;
    Vthq_dVb = T3 * Vbdz_dVbd ;
    Vthq_dVs = T3 * Vbdz_dVsd ;

    if (flg_smoothing_qme_vg) {
      T5 = Vgsz_qme_sat - Vgdz - qme_vgs_dlt ;
      T5_dVs = - Vgdz_dVsd ;
      T5_dVg = - Vgdz_dVgd ;
      T6 = sqrt( T5 * T5 + 4.0 * qme_vgs_dlt * Vgsz_qme_sat) ;
      T13 = Vgsz_qme_sat - 0.5 * (T5 + T6) ;
      T13_dVs = -0.5 * ( T5_dVs + T5 * T5_dVs / T6 ) ;
      T13_dVg = -0.5 * ( T5_dVg + T5 * T5_dVg / T6 ) ;
    }
    else {
      T13 = Vgdz ;
      T13_dVs = Vgdz_dVsd ;
      T13_dVg = Vgdz_dVgd ;
    }
      
    T1 = - T10 ;
    T2 = T13 - Vthq ;
    T3 = T9 * T1 * T1 + T11 ;
    T4 = T9 * T2 * T2 + T11 ;
    T5 = T4 - T3 - qme_dlt ;
    T6 = sqrt( T5 * T5 + 4.0 * qme_dlt * T4 ) ;
    
    dTox = T4 - 0.5 * ( T5 + T6 ) ;

    /* dTox_dT4 */
    T7 = 1.0 - 0.5 * ( 1.0 + ( T5 + 2.0 * qme_dlt ) / T6 ) ;
    T8 = 2.0 * T9 * T2 * T7 ;
    
    dTox_dVb = T8 * ( - Vthq_dVb ) ;
    dTox_dVs = T8 * ( T13_dVs - Vthq_dVs ) ;
    dTox_dVg = T8 * ( T13_dVg ) ;

    if ( T13 - Vthq > 0 ) {
      T5 = T11 - T3 - qme_dlt ;
      T6 = sqrt( T5 * T5 + 4.0 * qme_dlt * T11 ) ;
      
      dTox =  T11 - 0.5 * ( T5 + T6 ) ;
      dTox_dVb = 0.0 ;
      dTox_dVs = 0.0 ;
      dTox_dVg = 0.0 ;
    }

    ToxP = sIN.tox + dTox ;
    ToxP_dVb = dTox_dVb ;
    ToxP_dVs = dTox_dVs ;
    ToxP_dVg = dTox_dVg ;
    
    CoxP = c_eox / ToxP ;
    T1  = - c_eox / ( ToxP * ToxP ) ;
    CoxP_dVb = T1 * ToxP_dVb ;  
    CoxP_dVs = T1 * ToxP_dVs ;  
    CoxP_dVg = T1 * ToxP_dVg ;  

    CoxP_inv  = ToxP / c_eox ; 
    T1  = 1.0 / c_eox ;
    CoxP_inv_dVb = T1 * ToxP_dVb ;  
    CoxP_inv_dVs = T1 * ToxP_dVs ;  
    CoxP_inv_dVg = T1 * ToxP_dVg ;  

/* Threshold voltage.  (GISL) */
      
    Delta   = 0.1 ;
      
    Vbd1    = 2.0 - 0.25 * Vbdz ;
    Vbd2    = - Vbdz ;
    
    Vbdd    = Vbd1 - Vbd2 - Delta ; 
//    Vbdd_dVbd = 0.75 * Vbdz_dVbd ;
//    Vbdd_dVsd = 0.75 * Vbdz_dVsd ;
    
    T1      = sqrt( Vbdd * Vbdd + 4.0 * Delta ) ;
    
//    Vbdzm   = - Vbd1 + 0.5 * ( Vbdd + T1 ) ;
//    Vbdzm_dVb = 0.25 * Vbdz_dVbd
//      + 0.5 * ( Vbdd_dVbd + Vbdd * Vbdd_dVbd / T1 ) ;
//    Vbdzm_dVs = 0.25 * Vbdz_dVsd
//      + 0.5 * ( Vbdd_dVsd + Vbdd * Vbdd_dVsd / T1 ) ;

    Psum    = ( Pb20 - Vbdz ) ;
    
    if ( Psum >= epsm10 ) {
      Psum_dVb   = - Vbdz_dVbd ;
      Psum_dVs   = - Vbdz_dVsd ;
    } else {
      Psum    = epsm10 ;
      Psum_dVb   = 0.0e0 ;
      Psum_dVs   = 0.0e0 ;
    }
    
    sqrt_Psum   = sqrt( Psum ) ;

/* Vthp : Vth with pocket. (GISL) */
    
    T1   = 2.0 * q_Nsub * C_ESI ;
    Qb0   = sqrt( T1 * ( Pb20 - Vbdz ) ) ;
    
    Qb0_dVb = 0.5 * T1 / Qb0 * ( - Vbdz_dVbd ) ;
    Qb0_dVs = 0.5 * T1 / Qb0 * ( - Vbdz_dVsd ) ;
    
    Vthp = Pb20 + Vfb + Qb0 * CoxP_inv ;

    Vthp_dVb = Qb0_dVb * CoxP_inv + Qb0 * CoxP_inv_dVb ;
    Vthp_dVs = Qb0_dVs * CoxP_inv + Qb0 * CoxP_inv_dVs ;
    Vthp_dVg = Qb0 * CoxP_inv_dVg ;
    
/* dVthLP : Short-channel effect induced by pocket.
 * - Vth0 : Vth without pocket.      (GISL)         */
    
    if ( sIN.lp != 0.0 ) {
      
      T1   = 2.0 * C_QE * sIN.nsubc * C_ESI ;
      T2   = sqrt( T1 * ( Pb2c - Vbdz ) ) ;
     
      Vth0 = Pb2c + Vfb + T2 * CoxP_inv ;
      
      Vth0_dVb = 0.5 * T1 / T2 * ( - Vbdz_dVbd ) * CoxP_inv 
	+ T2 * CoxP_inv_dVb ;
      Vth0_dVs = 0.5 * T1 / T2 * ( - Vbdz_dVsd ) * CoxP_inv 
	+ T2 * CoxP_inv_dVs ;
      Vth0_dVg = T2 * CoxP_inv_dVg ;
      
      T1  = C_ESI * CoxP_inv ;
      T2  = sqrt( 2.0e0 * C_ESI / C_QE / sIN.nsubp ) ;
      T4  = 1.0e0 / sIN.parl1 / ( sIN.lp * sIN.lp ) ;
      T5  = 2.0e0 * ( C_Vbi - Pb20 ) * T1 * T2 * T4 ;
      
      dVth0   = T5 * sqrt_Psum ;
      
      T6  = 0.5 * T5 / sqrt_Psum ;
      T7  = 2.0e0 * ( C_Vbi - Pb20 ) * C_ESI * T2 * T4 * sqrt_Psum ;
      dVth0_dVb  = T6 * Psum_dVb + T7 * CoxP_inv_dVb ;
      dVth0_dVs  = T6 * Psum_dVs + T7 * CoxP_inv_dVs ;
      dVth0_dVg  = T7 * CoxP_inv_dVg ;

      T4 = sIN.scp1 ;
      T5 = sIN.scp2 ;
      T6 = sIN.scp3 ;

      if (flg_smoothing_pl_vd) {
	T8 = Vdsz_lp_sat + Vsdz - lp_vds_dlt ;
	T8_dVs =  Vsdz_dVsd ;
	T9 = sqrt( T8 * T8 + 4.0 * lp_vds_dlt * Vdsz_lp_sat) ;
	T7 = -( Vdsz_lp_sat - 0.5 * (T8 + T9) ) ;
	T7_dVs = 0.5 * ( T8_dVs + T8 * T8_dVs / T9 ) ;
      }
      else {
	T7 = Vsdz ;
	T7_dVs = Vsdz_dVsd ;
      }

      T1 = Vthp - Vth0 ;
      T2 = T4 + T6 * Psum / sIN.lp ;
      T3 = T2 + T5 * T7 ;
      
      dVthLP  = T1 * dVth0 * T3 ;
      
      dVthLP_dVb = ( Vthp_dVb - Vth0_dVb ) * dVth0 * T3 
	+ T1 * dVth0_dVb * T3 
	+ T1 * dVth0 * T6 * Psum_dVb / sIN.lp ;
      
      dVthLP_dVs = ( Vthp_dVs - Vth0_dVs ) * dVth0 * T3 
	+ T1 * dVth0_dVs * T3 
	+ T1 * dVth0
	* ( T6 * Psum_dVs / sIN.lp 
	    + T5 * T7_dVs ) ;
      
      dVthLP_dVg = ( Vthp_dVg - Vth0_dVg ) * dVth0 * T3 
	+ T1 * dVth0_dVg * T3 ;
      
    } else {
      dVthLP = 0.0e0 ;
      dVthLP_dVb = 0.0e0 ;
      dVthLP_dVs = 0.0e0 ;
      dVthLP_dVg = 0.0e0 ;
    }

/* dVthSC : Short-channel effect induced by Vds. (GISL) */

    /* Modify Pb20 to Pb20b */

    Vgpa = Vgd - Vfb ;
    T1 = 1.0e0 + 4.0e0 * (beta * Vgpa - 1.0e0 ) / ( cnst3 * beta * beta ) ;
    if ( T1 > 0.0 ) {
      T2 = sqrt(T1) ;
      Psi_a = Vgpa + cnst3 * beta / 2.0e0 * ( 1.0e0 - T2 ) ;
      Psi_a_dVg = Vgdz_dVgd * ( 1.0e0 - 1.0e0 / T2 ) ;
    } else {
      Psi_a = Psi_a_dVg = 0.0 ;
    }

    T1 = sqrt( ( Pb20 - Psi_a - delta0 ) * ( Pb20 - Psi_a - delta0 )
	       + 4.0e0 * delta0 * Pb20 ) ;
    Pb20a = Pb20 - 0.5e0 * ( Pb20 - Psi_a - delta0 + T1 ) ;
    Pb20a_dVg = 0.5e0 * Psi_a_dVg * ( 1.0e0 + ( Pb20 - Psi_a - delta0 ) / T1 ) ;
    Pb20b = Pb20 + sIN.pthrou * ( Pb20a - Pb20 ) ;
    Pb20b_dVg = sIN.pthrou * Pb20a_dVg ;
    
    T1  = C_ESI * CoxP_inv ;
    T2  = sqrt( 2.0e0 * C_ESI / q_Nsub ) ;
    T3  = ( Leff - sIN.parl2 ) ;
    T4  = 1.0e0 / sIN.parl1 / ( T3 * T3 ) ;
    T5  = 2.0e0 * ( C_Vbi - Pb20b ) * T1 * T2 * T4 ;
    
    dVth0   = T5 * sqrt_Psum ;
    T6  = T5 / 2 / sqrt_Psum ;
    T7  = 2.0e0 * ( C_Vbi - Pb20b ) * C_ESI * T2 * T4 * sqrt_Psum ;
    dVth0_dVb  = T6 * Psum_dVb + T7 * CoxP_inv_dVb ;
    dVth0_dVs  = T6 * Psum_dVs + T7 * CoxP_inv_dVs ;
    dVth0_dVg  = T7 * CoxP_inv_dVg + 2.0e0 * T1 * T2 * T4 * ( - Pb20b_dVg ) ;

    T5 = sIN.sc1 ;
    T6 = sIN.sc2 ;
    T7 = sIN.sc3 ;

    if (flg_smoothing_sc_vd) {
      T9 = Vdsz_sc_sat + Vsdz - sc_vds_dlt ;
      T9_dVs =  Vsdz_dVsd ;
      T10 = sqrt( T9 * T9 + 4.0 * sc_vds_dlt * Vdsz_sc_sat) ;
      T8 = -(Vdsz_sc_sat - 0.5 * (T9 + T10)) ;
      T8_dVs = 0.5 * ( T9_dVs + T9 * T9_dVs / T10 ) ;
    }
    else {
      T8 = Vsdz ;
      T8_dVs = Vsdz_dVsd ;
    }

    T4 = T5 + T7 * Psum / Leff ;
    T4_dVb = T7 * Psum_dVb / Leff ;
    T4_dVs = T7 * Psum_dVs / Leff ;
    
    dVthSC  = dVth0 * ( T4 + T6 * T8 ) ;
      
    dVthSC_dVb = dVth0_dVb * ( T4 + T6 * T8 )
      + dVth0 * ( T4_dVb ) ;
    
    dVthSC_dVs = dVth0_dVs * ( T4 + T6 * T8 )
      + dVth0 * ( T4_dVs + T6 * T8_dVs ) ;
    
    dVthSC_dVg = dVth0_dVg * ( T4 + T6 * T8 ) ;

/* dVthW : narrow-channel effect. (GISL) */
      
    T5 = sIN.wfc ; 
    
    T1 = 1.0 / CoxP ;
    T2 = T1 * T1 ;
    T3 = 1.0 / ( CoxP + T5 / Weff ) ;
    T4 = T3 * T3 ;
    
    dVthW = Qb0 * ( T1 - T3 ) ;

    dVthW_dVb = Qb0_dVb * ( T1 - T3 ) - Qb0 * CoxP_dVb * ( T2 - T4 ) ;
    dVthW_dVs = Qb0_dVs * ( T1 - T3 ) - Qb0 * CoxP_dVs * ( T2 - T4 ) ;
    dVthW_dVg = - Qb0 * CoxP_dVg * ( T2 - T4 ) ;

/* dVth : Total variation. 
 * - Positive dVth means the decrease in Vth. (GISL) */
      
    dVthP    = dVthSC + dVthLP + dVthW ;
    dVthP_dVb   = dVthSC_dVb + dVthLP_dVb + dVthW_dVb ;
    dVthP_dVs   = dVthSC_dVs + dVthLP_dVs + dVthW_dVs ;
    dVthP_dVg   = dVthSC_dVg + dVthLP_dVg + dVthW_dVg ;

/* Poly-Depletion Effect (GISL) */

    T1 = sIN.pgd1 ;
    T2 = sIN.pgd2 ;
    T3 = sIN.pgd3 ;
    
    if (flg_smoothing_pde_vg) {
      T5 = Vgsz_pol_sat - Vgdz - pol_vgs_dlt ;
      /*	T5_dVbs = 0.0 ;*/
      T5_dVs = - Vgdz_dVsd ;
      T5_dVg = - Vgdz_dVgd ;
      T6 = sqrt( T5 * T5 + 4.0 * pol_vgs_dlt * Vgsz_pol_sat ) ;
      T7 = Vgsz_pol_sat - 0.5 * (T5 + T6) ;
      /*	T7_dVb = -0.5 * ( T5_dVb + T5 * T5_dVb / T6 ) ; */
      T7_dVs = -0.5 * ( T5_dVs + T5 * T5_dVs / T6 ) ;
      T7_dVg = -0.5 * ( T5_dVg + T5 * T5_dVg / T6 ) ;
    } else {
      T7 = Vgdz ;
      /*	T7_dVb = 0.0 ; */
      T7_dVs = Vgdz_dVsd ;
      T7_dVg = Vgdz_dVgd ;
    }
    
    if (flg_smoothing_pde_vd) {
      T5 = Vdsz_pol_sat + Vsdz - pol_vds_dlt ;
      /*	T5_dVb = 0.0 ;*/
      T5_dVs =  Vsdz_dVsd ;
      /*	T5_dVg = 0.0 ;*/
      T6 = sqrt( T5 * T5 + 4.0 * pol_vds_dlt * Vdsz_pol_sat ) ;
      T8 = -(Vdsz_pol_sat - 0.5 * (T5 + T6)) ;
      /*	T8_dVb = 0.5 * ( T5_dVb + T5 * T5_dVb / T6 ) ; */
      T8_dVs = 0.5 * ( T5_dVs + T5 * T5_dVs / T6 ) ;
      /*	T8_dVg = 0.5 * ( T5_dVg + T5 * T5_dVg / T6 ) ; */
      
    } else {
      T8 = Vsdz ;
      /*	T8_dVb = 0.0 ; */
      T8_dVs = Vsdz_dVsd ;
      /*	T8_dVg = 0.0 ; */
    }
    
    T9 = - T1 * ( T7 - T2 - T3 * T8 ) - pol2_dlt ;
    T9_dVs = - T1 * (T7_dVs - T3 * T8_dVs) ;
    T9_dVg = - T1 * (T7_dVg /*- T3 * T8_dVg*/) ;

    T10 = T9 * T9 ;
    T10_dVs = 2.0e0 * T9 * T9_dVs ;
    T10_dVg = 2.0e0 * T9 * T9_dVg ;

    T11 = sqrt( T10 + 4.0e0 * pol2_dlt * pol_tmp ) ;
    T11_dVs = T10_dVs / sqrt( T10 + 4.0e0 * pol2_dlt * pol_tmp ) * 0.5e0 ;
    T11_dVg = T10_dVg / sqrt( T10 + 4.0e0 * pol2_dlt * pol_tmp ) * 0.5e0 ;

    dPpgP = ( T11 - T9 ) * 0.5e0 ;
    dPpgP_dVs = ( T11_dVs - T9_dVs ) * 0.5e0 ;
    dPpgP_dVg = ( T11_dVg - T9_dVg ) * 0.5e0 ;
    dPpgP_dVb = 0.0 ;

/* GISL */

    T1 = sIN.gidl3 * Vsdz - Vgdz + Vfb - dVthP + dPpgP ;

    E1 = T1 / ToxP ;

    E1_dVb = E1 * ( ( - dVthP_dVb + dPpgP_dVb ) / T1 - ToxP_dVb / ToxP ) ;
    E1_dVs = E1 * ( ( sIN.gidl3 * Vsdz_dVsd 
		       - Vgdz_dVsd - dVthP_dVs + dPpgP_dVs) / T1 
		     - ToxP_dVs / ToxP ) ;
    E1_dVg = E1 * ( ( -1.0 - dVthP_dVg + dPpgP_dVg) / T1 - ToxP_dVg / ToxP ) ;

    T1 = E0 - E1 - eef_dlt ;
    T2 = T1 * T1 ;

    T3 = sqrt( T2 + 4.0 * eef_dlt * E0 ) ;

    Egisl = E0 - ( T1 - T3 ) / 2 ;

    T4 = 1.0 / ( 4.0 * T3 ) ;
    T5 =  - 2.0 * T1 * T4 + 0.5 ;

    Egisl_dVb = T5 * E1_dVb ;
    Egisl_dVs = T5 * E1_dVs ;
    Egisl_dVg = T5 * E1_dVg ;

    T1 = exp( - sIN.gidl2 * Egp32 / Egisl ) ;

    T2 = sIN.gidl1 / Egp12 * C_QE * Weff ;

    Igisl = T2 * Egisl * Egisl * T1 ;

    T3 = T2 * T1 * ( 2.0 * Egisl + sIN.gidl2 * Egp32 ) ;

    Igisl_dVbd = T3 * Egisl_dVb ;
    Igisl_dVsd = T3 * Egisl_dVs ;
    Igisl_dVgd = T3 * Egisl_dVg ;
      
  end_of_IGISL: ;


/*-----------------------------------------------------------*
* End of PART-2. (label) 
*-----------------*/

//end_of_part_2: ;


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
* PART-3: Overlap charge
*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/ 

/*-------------------------------------------*
* Calculation of Psdl for cases of flg_noqi==1.
*-----------------*/

    if ( flg_noqi != 0 ) {

      T2 = Vds + Ps0 ;
        T2_dVb = Ps0_dVbs ;
        T2_dVd = 1.0e0 + Ps0_dVds ;
        T2_dVg = Ps0_dVgs ;

	T10 = sIN.clm1 ;

      Aclm = T10 ;
      Psdl = Aclm * T2 + ( 1.0e0 - Aclm ) * Psl ;
        Psdl_dVbs = Aclm * T2_dVb + ( 1.0e0 - Aclm ) * Psl_dVbs ;
        Psdl_dVds = Aclm * T2_dVd + ( 1.0e0 - Aclm ) * Psl_dVds ;
        Psdl_dVgs = Aclm * T2_dVg + ( 1.0e0 - Aclm ) * Psl_dVgs ;

        Ec = 0.0e0 ;
        Ec_dVbs =0.0e0 ;
        Ec_dVds =0.0e0 ;
        Ec_dVgs =0.0e0 ;

    T1 = sqrt( 2.0e0 * C_ESI / q_Nsub ) ;

    if ( Psl - Vbs > 0 ){
      T9 = sqrt( Psl - Vbs ) ;
      Wd = T1 * T9 ;
        Wd_dVbs = 0.5e0 * T1 / T9 * ( Psl_dVbs - 1.0e0 ) ;
        Wd_dVds = 0.5e0 * T1 / T9 * Psl_dVds ;
        Wd_dVgs = 0.5e0 * T1 / T9 * Psl_dVgs ;
    }else{
      Wd = 0e0 ;
        Wd_dVbs = 0e0 ;
        Wd_dVds = 0e0 ;
        Wd_dVgs = 0e0 ;
    }

      if ( Psdl > Ps0 + Vds - epsm10 ) {
          Psdl = Ps0 + Vds - epsm10 ;
      }
    }


/*-------------------------------------------*
* Overlap charge (simple, default, CLM1 < 1.0) <-- changed 04/Dec/01
*                                              (orig. "CLM1 must be 1.0")
*-----------------*/

    if ( sIN.coovlp == 0 ){

    T1 = Cox * Weff ;
    Lov         = sIN.xld ;
    Qgos        = Vgs * Lov * T1 ;
      Qgos_dVbs = 0.0e0 ;
      Qgos_dVds = 0.0e0 ;
      Qgos_dVgs = Lov * T1 ;

    T2  = - 1.0e+11 ;
    yn  = sqrt ( ( Psdl - Ps0 - Vds) / T2 ) ;
    yn2 = yn * yn ;
    yn3 = yn2 * yn ;


    if ( yn <= Lov ){
      Qgod = ( ( Vgs - Vds ) * Lov - T2 * yn3 / 3.0e0 ) * T1 ;
        Qgod_dVbs = ( (-0.5e0) * yn * ( Psdl_dVbs - Ps0_dVbs ) ) * T1 ;
        Qgod_dVds = ( (-1.0e0) * Lov - 0.5e0 * yn 
                    * ( Psdl_dVds - Ps0_dVds - 1.0e0 ) ) * T1 ;
        Qgod_dVgs = ( Lov - 0.5e0 * yn * ( Psdl_dVgs - Ps0_dVgs )  ) * T1 ;
    }else{
      T3  = 0.5e0 / yn / T2 ;
      yn_dVbs = T3 * ( Psdl_dVbs - Ps0_dVbs ) ;
      yn_dVds = T3 * ( Psdl_dVds - Ps0_dVds - 1.0e0 ) ;
      yn_dVgs = T3 * ( Psdl_dVgs - Ps0_dVgs ) ;

      T4 = (Lov - yn ) * (Lov - yn ) ;
      T5 = T4 * (Lov - yn ) ;
      Qgod = ( ( Vgs - Vds ) * Lov - T2 / 3.0e0 * ( T5 + yn3 ) ) * T1 ;
        Qgod_dVbs = ( (-1.0e0) * T2 * yn2 * yn_dVbs
		    + T2 * T4 * yn_dVbs  ) * T1 ;
        Qgod_dVds = ( (-1.0e0) * Lov - T2 * yn2 * yn_dVds
		    + T2 * T4 * yn_dVds  ) * T1 ;
        Qgod_dVgs = ( Lov - T2 * yn2 * yn_dVgs
		    + T2 * T4 * yn_dVgs  ) * T1 ;
    }
 }


/*-------------------------------------------*
* Overlap charge (complete)
*-----------------*/

    else if( sIN.coovlp > 0 ){

    T1 = Cox * Weff ;
    Lov = sIN.xld ;
    Qgos       = Vgs * Lov * T1 ;
    Qgos_dVbs  = 0.0e0 ;
    Qgos_dVds  = 0.0e0 ;
    Qgos_dVgs  = Lov * T1 ;

    T3 = 1e21 / ( pow(Lov , 3e0) + epsm10 ) ;
    T1 = C_QE * T3 / ( C_ESI * 4e0 ) ;
    T2 = Cox * Weff ;

    Lov2       = Lov * Lov ;
    
    yn         = pow( C_ESI * 5e0 / ( C_QE * T3 )
               * ( Ps0 + Vds - Psdl )
               , 0.2e0 ) ;
    yn2        = yn * yn ;

    if ( yn < Lov ) {
    Qgod       = ( ( Vgs - Vds ) * Lov + T1 / 3e0 * yn2 * yn2 * yn2 ) * T2 ;
    Qgod_dVbs  = ( yn / 2e0 * ( Ps0_dVbs - Psdl_dVbs ) ) * T2 ;
    Qgod_dVds  = ( - Lov + yn / 2e0 * ( Ps0_dVds + 1e0 - Psdl_dVds ) ) * T2 ;
    Qgod_dVgs  = (   Lov + yn / 2e0 * ( Ps0_dVgs - Psdl_dVgs ) ) * T2 ;
    } else {
    T3 = C_ESI * 5e0 / ( C_QE * T3 ) ;

    Qgod       = ( ( Vgs + T1 * 4e0 / 5e0 * yn2 * yn2 * yn - Vds ) * Lov 
		   + T1 / 30e0 * Lov2 * Lov2 * Lov2
		   - T1 * yn2 * yn2 / 2e0 * Lov2 ) * T2 ;
    Qgod_dVbs  = ( ( T1 * 4e0 / 5e0 * T3 * ( Ps0_dVbs - Psdl_dVbs ) ) * Lov 
		   - 2e0 * T1 / 5e0 * T3 / yn * ( Ps0_dVbs - Psdl_dVbs ) * Lov2 )
               * T2 ;
    Qgod_dVds  = ( ( T1 * 4e0 / 5e0 * T3 * ( Ps0_dVds + 1e0 - Psdl_dVds ) - 1e0 )
		   * Lov 
		   - 2e0 * T1 / 5e0 * T3 / yn * ( Ps0_dVds + 1e0 - Psdl_dVds )
		   * Lov2 ) * T2 ;
    Qgod_dVgs  = ( ( 1e0 + T1 * 4e0 / 5e0 * T3 * ( Ps0_dVgs - Psdl_dVgs ) ) * Lov 
		   - 2e0 * T1 / 5e0 * T3 / yn * ( Ps0_dVgs - Psdl_dVgs ) * Lov2 )
               * T2 ;
    }
    }

/*-------------------------------------------*
* Overlap charge (off)
*-----------------*/

    else if( sIN.coovlp < 0 ){

    Qgos       = 0.0e0 ;
    Qgos_dVbs  = 0.0e0 ;
    Qgos_dVds  = 0.0e0 ;
    Qgos_dVgs  = 0.0e0 ;

    Qgod       = 0.0e0 ;
    Qgod_dVbs  = 0.0e0 ;
    Qgod_dVds  = 0.0e0 ;
    Qgod_dVgs  = 0.0e0 ;

    }


/*-----------------------------------------------------------* 
* End of PART-3. (label) 
*-----------------*/ 

//end_of_part_3: ;


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
* PART-4: Substrate-source/drain junction diode.
*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/ 

/*-----------------------------------------------------------*
* Cbsj, Cbdj: node-base S/D biases.
*-----------------*/

    if (sIN.mode == HiSIM_NORMAL_MODE ){
      Vbdj = Vbde ;
      Vbsj = Vbse ;
    } else {
      Vbdj = Vbse ;
      Vbsj = Vbde ;
    }

    /*    Vbdc = Vbsc - Vdsc ;*/
    
    js   = sIN.js0 * exp( ( Eg300 * C_b300 - Eg * beta
         + sIN.xti * log( sIN.temp / C_T300 ) ) / sIN.nj ) ;
    jssw = sIN.js0sw * exp( ( Eg300 * C_b300 - Eg * beta 
         + sIN.xti * log( sIN.temp / C_T300 ) ) 
           / sIN.njsw ) ;
    
    Nvtm = sIN.nj / beta ;
    
    /* ibd */
    isbd = sIN.ad * js + sIN.pd * jssw ;

    if ( Vbdj < sIN.vdiffj ) {
      Ibd = isbd * ( exp( Vbdj / Nvtm ) - 1 ) ;
      Gbd = isbd / Nvtm * exp( Vbdj / Nvtm ) ;

    } else {
      Ibd = isbd * ( exp( sIN.vdiffj / Nvtm ) - 1 )
          + isbd / Nvtm * exp( sIN.vdiffj / Nvtm ) * ( Vbdj - sIN.vdiffj ) ;
      Gbd = isbd / Nvtm * exp( sIN.vdiffj / Nvtm ) ;
    }  
      
    /* ibs */
    isbs = sIN.as * js + sIN.ps * jssw ;

    if ( Vbsj < sIN.vdiffj ) {
      Ibs = isbs * ( exp( Vbsj / Nvtm ) - 1 ) ;
      Gbs = isbs / Nvtm * exp( Vbsj / Nvtm ) ;

    } else {
      Ibs = isbs * ( exp( sIN.vdiffj / Nvtm ) - 1 )
          + isbs / Nvtm * exp( sIN.vdiffj / Nvtm ) * (Vbsj - sIN.vdiffj ) ;
      Gbs = isbs / Nvtm * exp( sIN.vdiffj / Nvtm ) ;
    }  


/*---------------------------------------------------*
* Add Gjmin.
*-----------------*/

    Ibd += Gjmin * Vbdj ;
    Ibs += Gjmin * Vbsj ;

    Gbd += Gjmin ;
    Gbs += Gjmin ;


/*-----------------------------------------------------------*
* Charges and Capacitances.
*-----------------*/
   /*  charge storage elements
               *  bulk-drain and bulk-source depletion capacitances
               *  czbd : zero bias drain junction capacitance
               *  czbs : zero bias source junction capacitance
               *  czbdsw:zero bias drain junction sidewall capacitance
               *  czbssw:zero bias source junction sidewall capacitance
               */

              czbd = sIN.cj * sIN.ad ;
              czbs = sIN.cj * sIN.as ;

              /* Source Bulk Junction */
          if (sIN.ps > Weff) {
              czbssw = sIN.cjsw * ( sIN.ps - Weff ) ;
              czbsswg = sIN.cjswg * Weff ;
	      if (Vbsj == 0.0)
		{   Qbs = 0.0 ;
		    Capbs = czbs + czbssw + czbsswg ;
		  }
	      else if (Vbsj < 0.0)
		{   if (czbs > 0.0)
		      {   arg = 1.0 - Vbsj / sIN.pb ;
			  if (sIN.mj == 0.5)
			    sarg = 1.0 / sqrt(arg) ;
			  else
			    sarg = exp(-sIN.mj * log(arg)) ;
			  Qbs = sIN.pb * czbs 
			    * (1.0 - arg * sarg) / (1.0 - sIN.mj) ;
			  Capbs = czbs * sarg ;
			}
		else
		  {   Qbs = 0.0 ;
		      Capbs = 0.0 ;
		    }
		    if (czbssw > 0.0)
		      {   arg = 1.0 - Vbsj / sIN.pbsw ;
			  if (sIN.mjsw == 0.5)
			    sarg = 1.0 / sqrt(arg) ;
			  else
			    sarg = exp(-sIN.mjsw * log(arg)) ;
			  Qbs += sIN.pbsw * czbssw
			    * (1.0 - arg * sarg) / (1.0 - sIN.mjsw) ;
			  Capbs += czbssw * sarg ;
			}
		  if (czbsswg > 0.0)
		    {   arg = 1.0 - Vbsj / sIN.pbswg ;
			if (sIN.mjswg == 0.5)
                          sarg = 1.0 / sqrt(arg) ;
			else
                          sarg = exp(-sIN.mjswg * log(arg)) ;
			Qbs += sIN.pbswg * czbsswg
			  * (1.0 - arg * sarg) / (1.0 - sIN.mjswg) ;
			Capbs += czbsswg * sarg ;
		      }
		  }
	      else
		{   Qbs = Vbsj * (czbs + czbssw + czbsswg) 
		      + Vbsj * Vbsj * (czbs * sIN.mj * 0.5 / sIN.pb 
				     + czbssw * sIN.mjsw * 0.5 / sIN.pbsw 
				     + czbsswg * sIN.mjswg 
				     * 0.5 / sIN.pbswg) ;
		    Capbs = czbs 
		      + czbssw + czbsswg + Vbsj * (czbs * sIN.mj /sIN.pb
					+ czbssw * sIN.mjsw / sIN.pbsw 
				        + czbsswg * sIN.mjswg / sIN.pbswg ) ;
              }

	  } else {
              czbsswg = sIN.cjswg * sIN.ps ;
	    if (Vbsj == 0.0)
	      {   Qbs = 0.0 ;
                  Capbs = czbs + czbsswg ;
		}
	    else if (Vbsj < 0.0)
	      {   if (czbs > 0.0)
		    {   arg = 1.0 - Vbsj / sIN.pb ;
			if (sIN.mj == 0.5)
                          sarg = 1.0 / sqrt(arg) ;
			else
                          sarg = exp(-sIN.mj * log(arg)) ;
			Qbs = sIN.pb * czbs 
			  * (1.0 - arg * sarg) / (1.0 - sIN.mj) ;
			Capbs = czbs * sarg ;
		      }
	      else
		{   Qbs = 0.0 ;
		    Capbs = 0.0 ;
		  }
		  if (czbsswg > 0.0)
		    {   arg = 1.0 - Vbsj / sIN.pbswg ;
			if (sIN.mjswg == 0.5)
                          sarg = 1.0 / sqrt(arg) ;
			else
                          sarg = exp(-sIN.mjswg * log(arg)) ;
			Qbs += sIN.pbswg * czbsswg
			  * (1.0 - arg * sarg) / (1.0 - sIN.mjswg) ;
			Capbs += czbsswg * sarg ;
		      }
		}
	    else
	      {   Qbs = Vbsj * (czbs + czbsswg) 
		    + Vbsj * Vbsj * (czbs * sIN.mj * 0.5 / sIN.pb 
                            + czbsswg * sIN.mjswg * 0.5 / sIN.pbswg) ;
                  Capbs = czbs 
		    + czbsswg + Vbsj * (czbs * sIN.mj /sIN.pb
				      + czbsswg * sIN.mjswg / sIN.pbswg ) ;
		}
	  }    

              /* Drain Bulk Junction */
          if (sIN.pd > Weff) {
              czbdsw = sIN.cjsw * ( sIN.pd - Weff ) ;
              czbdswg = sIN.cjswg * Weff ;
	      if (Vbdj == 0.0)
		{   Qbd = 0.0 ;
		    Capbd = czbd + czbdsw + czbdswg ;
		  }
	      else if (Vbdj < 0.0)
		{   if (czbd > 0.0)
		      {   arg = 1.0 - Vbdj / sIN.pb ;
			  if (sIN.mj == 0.5)
			    sarg = 1.0 / sqrt(arg) ;
			  else
			    sarg = exp(-sIN.mj * log(arg)) ;
			  Qbd = sIN.pb * czbd 
			    * (1.0 - arg * sarg) / (1.0 - sIN.mj) ;
			  Capbd = czbd * sarg ;
			}
		else
		  {   Qbd = 0.0 ;
		      Capbd = 0.0 ;
		    }
		    if (czbdsw > 0.0)
		      {   arg = 1.0 - Vbdj / sIN.pbsw ;
			  if (sIN.mjsw == 0.5)
			    sarg = 1.0 / sqrt(arg) ;
			  else
			    sarg = exp(-sIN.mjsw * log(arg)) ;
			  Qbd += sIN.pbsw * czbdsw
			    * (1.0 - arg * sarg) / (1.0 - sIN.mjsw) ;
			  Capbd += czbdsw * sarg ;
			}
		  if (czbdswg > 0.0)
		    {   arg = 1.0 - Vbdj / sIN.pbswg ;
			if (sIN.mjswg == 0.5)
                          sarg = 1.0 / sqrt(arg) ;
			else
                          sarg = exp(-sIN.mjswg * log(arg)) ;
			Qbd += sIN.pbswg * czbdswg
			  * (1.0 - arg * sarg) / (1.0 - sIN.mjswg) ;
			Capbd += czbdswg * sarg ;
		      }
		  }
	      else
		{   Qbd = Vbdj * (czbd + czbdsw + czbdswg) 
		      + Vbdj * Vbdj * (czbd * sIN.mj * 0.5 / sIN.pb 
				     + czbdsw * sIN.mjsw * 0.5 / sIN.pbsw 
				     + czbdswg * sIN.mjswg 
				     * 0.5 / sIN.pbswg) ;
		    Capbd = czbd 
		      + czbdsw + czbdswg + Vbdj * (czbd * sIN.mj /sIN.pb
					+ czbdsw * sIN.mjsw / sIN.pbsw 
				        + czbdswg * sIN.mjswg / sIN.pbswg ) ;
              }

	  } else {
              czbdswg = sIN.cjswg * sIN.pd ;
	    if (Vbdj == 0.0)
	      {   Qbd = 0.0 ;
                  Capbd = czbd + czbdswg ;
		}
	    else if (Vbdj < 0.0)
	      {   if (czbd > 0.0)
		    {   arg = 1.0 - Vbdj / sIN.pb ;
			if (sIN.mj == 0.5)
                          sarg = 1.0 / sqrt(arg) ;
			else
                          sarg = exp(-sIN.mj * log(arg)) ;
			Qbd = sIN.pb * czbd 
			  * (1.0 - arg * sarg) / (1.0 - sIN.mj) ;
			Capbd = czbd * sarg ;
		      }
	      else
		{   Qbd = 0.0 ;
		    Capbd = 0.0 ;
		  }
		  if (czbdswg > 0.0)
		    {   arg = 1.0 - Vbdj / sIN.pbswg ;
			if (sIN.mjswg == 0.5)
                          sarg = 1.0 / sqrt(arg) ;
			else
                          sarg = exp(-sIN.mjswg * log(arg)) ;
			Qbd += sIN.pbswg * czbdswg
			  * (1.0 - arg * sarg) / (1.0 - sIN.mjswg) ;
			Capbd += czbdswg * sarg ;
		      }
		}
	    else
	      {   Qbd = Vbdj * (czbd + czbdswg) 
		    + Vbdj * Vbdj * (czbd * sIN.mj * 0.5 / sIN.pb 
                            + czbdswg * sIN.mjswg * 0.5 / sIN.pbswg) ;
                  Capbd = czbd 
		    + czbdswg + Vbdj * (czbd * sIN.mj /sIN.pb
				      + czbdswg * sIN.mjswg / sIN.pbswg ) ;
		}
	  }


/*-----------------------------------------------------------* 
* End of PART-4. (label) 
*-----------------*/ 

//end_of_part_4: ;


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
* PART-5: Noise Calculation.
*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/ 

    if ( sIN.conois != 0 ){

    NFalp = sIN.nfalp ;
    NFtrp = sIN.nftrp ;
    Cit = sIN.cit ;

    T1 = Qn0 / C_QE ;
    T2 = (Cox+Qn0/(Ps0-Vbs)+Cit)/beta/C_QE ;
    T3 = -2.0E0*Qi/C_QE/(Leff-Lred)/Weff-T1 ;
    T4 = 1.0E0/(T1+T2)/(T3+T2)+2.0E0*NFalp*Ey*Mu/(T3-T1)
      *log((T3+T2)/(T1+T2))+NFalp*Ey*Mu*NFalp*Ey*Mu ;
    Nflic = Ids*Ids*NFtrp/((Leff-Lred)*beta*Weff)*T4 ;

    } else {
    Nflic = 0.0 ;
    }


/*-----------------------------------------------------------* 
* End of PART-5. (label) 
*-----------------*/ 

//end_of_part_5: ;


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
* PART-6: Evaluation of outputs. 
*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/ 

/*-----------------------------------------------------------* 
* Implicit quantities related to Alpha. 
*-----------------*/ 

    /* note: T1 = 1 + Delta */
    if ( flg_noqi == 0 && VgVt > VgVt_small && Pds > 0.0 ) {
        T1  = Achi / Pds ;
        Delta   = T1 - 1.0e0 ;
        Vdsat   = VgVt / T1 ;
    } else {
        Alpha   = 1.0 ;
        Delta   = 0.0 ;
        Vdsat   = 0.0 ;
        Achi    = 0.0 ;
    }


/*-----------------------------------------------------------* 
* Evaluate the derivatives w.r.t. external biases.
* - All derivatives that influence outputs must be modified here.
*-----------------*/ 

/*---------------------------------------------------* 
* Case ignoring Rs/Rd.
*-----------------*/ 

  if ( flg_rsrd == 0 || Ids < 0.0 ) {

    Ids_dVbse   = Ids_dVbs ;
    Ids_dVdse   = Ids_dVds ;
    Ids_dVgse   = Ids_dVgs ;
    
    Qb_dVbse	= Qb_dVbs ;
    Qb_dVdse	= Qb_dVds ;
    Qb_dVgse	= Qb_dVgs ;

    Qi_dVbse	= Qi_dVbs ;
    Qi_dVdse	= Qi_dVds ;
    Qi_dVgse	= Qi_dVgs ;

    Qd_dVbse	= Qd_dVbs ;
    Qd_dVdse	= Qd_dVds ;
    Qd_dVgse	= Qd_dVgs ;
    
    Isub_dVbse	= Isub_dVbs ;
    Isub_dVdse	= Isub_dVds ;
    Isub_dVgse	= Isub_dVgs ;
    
    Igate_dVbse	= Igate_dVbs ;
    Igate_dVdse	= Igate_dVds ;
    Igate_dVgse	= Igate_dVgs ;
    
    Igidl_dVbse = Igidl_dVbs ;
    Igidl_dVdse = Igidl_dVds ;
    Igidl_dVgse = Igidl_dVgs ;

    Qgos_dVbse	= Qgos_dVbs ;
    Qgos_dVdse	= Qgos_dVds ;
    Qgos_dVgse	= Qgos_dVgs ;

    Qgod_dVbse	= Qgod_dVbs ;
    Qgod_dVdse	= Qgod_dVds ;
    Qgod_dVgse	= Qgod_dVgs ;


/*---------------------------------------------------* 
* Case Rs>0 or Rd>0
*-----------------*/ 

  } else {


/*-------------------------------------------* 
* Conductances w.r.t. confined biases.
*-----------------*/ 

    Ids_dVbse    = Ids_dVbs * DJI ;
    Ids_dVdse    = Ids_dVds * DJI ;
    Ids_dVgse    = Ids_dVgs * DJI ;


/*-------------------------------------------* 
* Derivatives of internal biases  w.r.t. external biases.
*-----------------*/ 

    Vbs_dVbse	= ( 1.0 - Rs * Ids_dVbse ) ;
    Vbs_dVdse	= - Rs * Ids_dVdse ;
    Vbs_dVgse	= - Rs * Ids_dVgse ;

    Vds_dVbse	= - ( Rs + Rd ) * Ids_dVbse ;
    Vds_dVdse	= ( 1.0 - ( Rs + Rd ) * Ids_dVdse ) ;
    Vds_dVgse	= - ( Rs + Rd ) * Ids_dVgse ;

    Vgs_dVbse	= - Rs * Ids_dVbse ;
    Vgs_dVdse	= - Rs * Ids_dVdse ;
    Vgs_dVgse	= ( 1.0 - Rs * Ids_dVgse ) ;


/*-------------------------------------------* 
* Derivatives of charges.
*-----------------*/ 

    Qb_dVbse	= Qb_dVbs * Vbs_dVbse + Qb_dVds * Vds_dVbse
                + Qb_dVgs * Vgs_dVbse ;
    Qb_dVdse	= Qb_dVbs * Vbs_dVdse  + Qb_dVds * Vds_dVdse
                + Qb_dVgs * Vgs_dVdse ;
    Qb_dVgse	= Qb_dVbs * Vbs_dVgse  + Qb_dVds * Vds_dVgse
                + Qb_dVgs * Vgs_dVgse ;

    Qi_dVbse	= Qi_dVbs * Vbs_dVbse  + Qi_dVds * Vds_dVbse
                + Qi_dVgs * Vgs_dVbse ;
    Qi_dVdse	= Qi_dVbs * Vbs_dVdse  + Qi_dVds * Vds_dVdse
                + Qi_dVgs * Vgs_dVdse ;
    Qi_dVgse	= Qi_dVbs * Vbs_dVgse  + Qi_dVds * Vds_dVgse
                + Qi_dVgs * Vgs_dVgse ;

    Qd_dVbse	= Qd_dVbs * Vbs_dVbse  + Qd_dVds * Vds_dVbse
                + Qd_dVgs * Vgs_dVbse ;
    Qd_dVdse	= Qd_dVbs * Vbs_dVdse  + Qd_dVds * Vds_dVdse
                + Qd_dVgs * Vgs_dVdse ;
    Qd_dVgse	= Qd_dVbs * Vbs_dVgse  + Qd_dVds * Vds_dVgse
                + Qd_dVgs * Vgs_dVgse ;

    
/*-------------------------------------------* 
* Substrate/gate/leak conductances.
*-----------------*/ 

    Isub_dVbse	= Isub_dVbs * Vbs_dVbse  + Isub_dVds * Vds_dVbse
                + Isub_dVgs * Vgs_dVbse ;
    Isub_dVdse	= Isub_dVbs * Vbs_dVdse  + Isub_dVds * Vds_dVdse
                + Isub_dVgs * Vgs_dVdse ;
    Isub_dVgse	= Isub_dVbs * Vbs_dVgse  + Isub_dVds * Vds_dVgse
                + Isub_dVgs * Vgs_dVgse ;

    Igate_dVbse	= Igate_dVbs * Vbs_dVbse  + Igate_dVds * Vds_dVbse
                + Igate_dVgs * Vgs_dVbse ;
    Igate_dVdse	= Igate_dVbs * Vbs_dVdse  + Igate_dVds * Vds_dVdse
                + Igate_dVgs * Vgs_dVdse ;
    Igate_dVgse	= Igate_dVbs * Vbs_dVgse  + Igate_dVds * Vds_dVgse
                + Igate_dVgs * Vgs_dVgse ;

    Igidl_dVbse = Igidl_dVbs * Vbs_dVbse  + Igidl_dVds * Vds_dVbse
                + Igidl_dVgs * Vgs_dVbse ;
    Igidl_dVdse = Igidl_dVbs * Vbs_dVdse  + Igidl_dVds * Vds_dVdse
                + Igidl_dVgs * Vgs_dVdse ;
    Igidl_dVgse = Igidl_dVbs * Vbs_dVgse  + Igidl_dVds * Vds_dVgse
                + Igidl_dVgs * Vgs_dVgse ;

    
/*---------------------------------------------------* 
* Derivatives of overlap charges.
*-----------------*/ 

    Qgos_dVbse	= Qgos_dVbs * Vbs_dVbse  + Qgos_dVds * Vds_dVbse
                + Qgos_dVgs * Vgs_dVbse ;
    Qgos_dVdse	= Qgos_dVbs * Vbs_dVdse  + Qgos_dVds * Vds_dVdse
                + Qgos_dVgs * Vgs_dVdse ;
    Qgos_dVgse	= Qgos_dVbs * Vbs_dVgse  + Qgos_dVds * Vds_dVgse
                + Qgos_dVgs * Vgs_dVgse ;

    Qgod_dVbse	= Qgod_dVbs * Vbs_dVbse  + Qgod_dVds * Vds_dVbse
                + Qgod_dVgs * Vgs_dVbse ;
    Qgod_dVdse	= Qgod_dVbs * Vbs_dVdse  + Qgod_dVds * Vds_dVdse
                + Qgod_dVgs * Vgs_dVdse ;
    Qgod_dVgse	= Qgod_dVbs * Vbs_dVgse  + Qgod_dVds * Vds_dVgse
                + Qgod_dVgs * Vgs_dVgse ;

  } /* end of if ( flg_rsrd == 0 ) blocks */

  /* for GISL (GIDL in REVERSE mode) */
  if ( flg_rsrd == 0 || Ids < 0.0 ) {
    Igisl_dVbde = Igisl_dVbd ;
    Igisl_dVsde = Igisl_dVsd ;
    Igisl_dVgde = Igisl_dVgd ;
  } else {
    DJIP = 1.0 / 
      (1.0 + Rd * (-Ids_dVbs) + ( Rd + Rs ) * Ids_dVds + Rd * (-Ids_dVgs)) ;
    Isd_dVbde = (-Ids_dVbs) * DJIP ;
    Isd_dVsde = Ids_dVds * DJIP ;
    Isd_dVgde = (-Ids_dVgs) * DJIP ;
    
    Vbd_dVbde	= ( 1.0 - Rd * Isd_dVbde ) ;
    Vbd_dVsde	= - Rd * Isd_dVsde ;
    Vbd_dVgde	= - Rd * Isd_dVgde ;
    
    Vsd_dVbde	= - ( Rd + Rs ) * Isd_dVbde ;
    Vsd_dVsde	= ( 1.0 - ( Rd + Rs ) * Isd_dVsde ) ;
    Vsd_dVgde	= - ( Rd + Rs ) * Isd_dVgde ;
    
    Vgd_dVbde	= - Rd * Isd_dVbde ;
    Vgd_dVsde	= - Rd * Isd_dVsde ;
    Vgd_dVgde	= ( 1.0 - Rd * Isd_dVgde ) ;

    Igisl_dVbde = Igisl_dVbd * Vbd_dVbde  + Igisl_dVsd * Vsd_dVbde
      + Igisl_dVgd * Vgd_dVbde ;
    Igisl_dVsde = Igisl_dVbd * Vbd_dVsde  + Igisl_dVsd * Vsd_dVsde
      + Igisl_dVgd * Vgd_dVsde ;
    Igisl_dVgde = Igisl_dVbd * Vbd_dVgde  + Igisl_dVsd * Vsd_dVgde
      + Igisl_dVgd * Vgd_dVgde ;
  }


/*---------------------------------------------------* 
* Derivatives of junction diode currents and charges.
* - NOTE: These quantities are regarded as functions of
*         external biases.
* - NOTE: node-base S/D 
*-----------------*/ 

    Gbse    = Gbs ;
    Gbde    = Gbd ;
    Capbse  = Capbs ;
    Capbde  = Capbd ;


/*---------------------------------------------------*
* Extrapolate quantities if external biases are out of bounds.
*-----------------*/

    if ( flg_vbsc == 1 ) {
      Ids_dVbse *= Vbsc_dVbse ;
      Qb_dVbse *= Vbsc_dVbse ;
      Qi_dVbse *= Vbsc_dVbse ;
      Qd_dVbse *= Vbsc_dVbse ;
      Isub_dVbse *= Vbsc_dVbse ;
      Igate_dVbse *= Vbsc_dVbse ;
      Igidl_dVbse *= Vbsc_dVbse ;
      Qgos_dVbse *= Vbsc_dVbse ;
      Qgod_dVbse *= Vbsc_dVbse ;
    }

    if (flg_vbdc == 1) 
      Igisl_dVbde *= Vbdc_dVbde ;

    if ( flg_vxxc != 0 ) {

      if ( flg_vbsc == -1 ) {
        T1  = Vbse - Vbsc ;
      } else {
        T1  = 0.0 ;
      }

      if ( flg_vdsc != 0 ) {
        T2  = Vdse - Vdsc ;
      } else {
        T2  = 0.0 ;
      }

      if ( flg_vgsc != 0 ) {
        T3  = Vgse - Vgsc ;
      } else {
        T3  = 0.0 ;
      }

      if ( flg_vbdc != 0 ) {
        T4  = Vbde - Vbdc ;
      } else {
        T4  = 0.0 ;
      }

      TX = Ids + T1 * Ids_dVbse + T2 * Ids_dVdse + T3 * Ids_dVgse ;
      if ( TX * Ids >= 0.0 ) {
        Ids = TX ;
      } else {
        T7 = Ids / ( Ids - TX ) ;
        Ids_dVbse *= T7 ;
        Ids_dVdse *= T7 ;
        Ids_dVgse *= T7 ;
        Ids = 0.0 ;
      }

      TX = Qb  + T1 * Qb_dVbse + T2 * Qb_dVdse + T3 * Qb_dVgse ;
      /*note: The sign of Qb can be changed.*/
      Qb = TX ;

      TX = Qd  + T1 * Qd_dVbse + T2 * Qd_dVdse + T3 * Qd_dVgse ;
      if ( TX * Qd >= 0.0 ) {
        Qd = TX ;
      } else {
        T7 = Qd / ( Qd - TX ) ;
        Qd_dVbse *= T7 ;
        Qd_dVdse *= T7 ;
        Qd_dVgse *= T7 ;
        Qd = 0.0 ;
      }

      TX = Qi  + T1 * Qi_dVbse + T2 * Qi_dVdse + T3 * Qi_dVgse ;
      if ( TX * Qi >= 0.0 ) {
        Qi = TX ;
      } else {
        T7 = Qi / ( Qi - TX ) ;
        Qi_dVbse *= T7 ;
        Qi_dVdse *= T7 ;
        Qi_dVgse *= T7 ;
        Qi = 0.0 ;
      }

      TX = Isub + T1 * Isub_dVbse + T2 * Isub_dVdse + T3 * Isub_dVgse ;
      if ( TX * Isub >= 0.0 ) {
        Isub = TX ;
      } else {
        T7 = Isub / ( Isub - TX ) ;
        Isub_dVbse *= T7 ;
        Isub_dVdse *= T7 ;
        Isub_dVgse *= T7 ;
        Isub = 0.0 ;
      }

      TX = Igate + T1 * Igate_dVbse + T2 * Igate_dVdse + T3 * Igate_dVgse ;
      if ( TX * Igate >= 0.0 ) {
        Igate = TX ;
      } else {
        T7 = Igate / ( Igate - TX ) ;
        Igate_dVbse *= T7 ;
        Igate_dVdse *= T7 ;
        Igate_dVgse *= T7 ;
        Igate = 0.0 ;
      }

      TX = Igidl + T1 * Igidl_dVbse + T2 * Igidl_dVdse + T3 * Igidl_dVgse ;
      if ( TX * Igidl >= 0.0 ) {
        Igidl = TX ;
      } else {
        T7 = Igidl / ( Igidl - TX ) ;
        Igidl_dVbse *= T7 ;
        Igidl_dVdse *= T7 ;
        Igidl_dVgse *= T7 ;
        Igidl = 0.0 ;
      }

      /* for GISL */
      if ( flg_vbdc == -1 ) {
	T5  = Vbde - Vbdc ;
      } else {
	T5  = 0.0 ;
      }

      if ( flg_vdsc != 0 ) {
	T6  = Vsde - Vsdc ;
      } else {
	T6  = 0.0 ;
      }
      
      if ( flg_vgsc != 0 ) {
	T7  = Vgde - Vgdc ;
      } else {
	T7  = 0.0 ;
      }
      
      TX = Igisl + T5 * Igisl_dVbde + T6 * Igisl_dVsde + T7 * Igisl_dVgde ;
      if ( TX * Igisl >= 0.0 ) {
	Igisl = TX ;
      } else {
	T8 = Igisl / ( Igisl - TX ) ;
	Igisl_dVbde *= T8 ;
	Igisl_dVsde *= T8 ;
	Igisl_dVgde *= T8 ;
	Igisl = 0.0 ;
      }
    
      TX = Qgod + T1 * Qgod_dVbse + T2 * Qgod_dVdse + T3 * Qgod_dVgse ;
      if ( TX * Qgod >= 0.0 ) {
        Qgod = TX ;
      } else {
        T7 = Qgod / ( Qgod - TX ) ;
        Qgod_dVbse *= T7 ;
        Qgod_dVdse *= T7 ;
        Qgod_dVgse *= T7 ;
        Qgod = 0.0 ;
      }

      TX = Qgos + T1 * Qgos_dVbse + T2 * Qgos_dVdse + T3 * Qgos_dVgse ;
      if ( TX * Qgos >= 0.0 ) {
        Qgos = TX ;
      } else {
        T7 = Qgos / ( Qgos - TX ) ;
        Qgos_dVbse *= T7 ;
        Qgos_dVdse *= T7 ;
        Qgos_dVgse *= T7 ;
        Qgos = 0.0 ;
      }

    }


/*-----------------------------------------------------------* 
* Warn negative conductance.
* - T1 ( = d Ids / d Vds ) is the derivative w.r.t. circuit bias.
*-----------------*/ 

    if ( sIN.mode == HiSIM_NORMAL_MODE ) {
      T1 = Ids_dVdse ;
    } else {
      T1 = Ids_dVbse + Ids_dVdse + Ids_dVgse ;
    }

    if ( flg_info >= 1 && 
	 (Ids_dVbse < 0.0 || T1 < 0.0 || Ids_dVgse < 0.0) ) {
      printf( "*** warning(HiSIM): Negative Conductance\n" ) ;
      printf( " type = %d  mode = %d\n" , sIN.type , sIN.mode ) ;
      printf( " Vbse = %12.5e Vdse = %12.5e Vgse = %12.5e\n" , 
	      Vbse , Vdse , Vgse ) ;
      printf( " Ids_dVbse   = %12.5e\n" , Ids_dVbse ) ;
      printf( " Ids_dVdse   = %12.5e\n" , T1 ) ;
      printf( " Ids_dVgse   = %12.5e\n" , Ids_dVgse ) ;
    }

    if ( Ids_dVbse < 0.0e0 ) {
        fprintf( stderr , " Ids_dVbse   = %12.5e\n" , Ids_dVbse ) ;
//        flg_ncnv ++ ;
    }
    if ( T1 < 0.0e0 ) {
        fprintf( stderr , " Ids_dVdse   = %12.5e\n" , T1 ) ;
//        flg_ncnv ++ ;
    }
    if ( Ids_dVgse < 0.0e0 ) {
        fprintf( stderr , " Ids_dVgse   = %12.5e\n" , Ids_dVgse ) ;
//        flg_ncnv ++ ;
    }


/*-----------------------------------------------------------* 
* Redefine overlap charges/capacitances.
*-----------------*/

/*---------------------------------------------------* 
* Constant capacitance.
*-----------------*/ 

    if ( sIN.cocgbo >= 1 ) { 
      Cgbo    = sIN.cgbo * Lgate ;
    } else {
      Cgbo    = 0.0 ;
    }

    if ( sIN.cocgdo >= 1 ) { 
      Cgdo         = - sIN.cgdo * Weff ;
      Qgod         = - Cgdo * ( Vgse - Vdse ) ;
      Qgod_dVbse   = 0.0 ;
      Qgod_dVdse   =   Cgdo ;
      Qgod_dVgse   = - Qgod_dVdse ;
    } else {
      Cgdo         = Qgos_dVdse + Qgod_dVdse ;
    }

    if ( sIN.cocgso >= 1 ) { 
      Cgso         = - sIN.cgso * Weff ;
      Qgos         = - Cgso * Vgse ;
      Qgos_dVbse   = 0.0 ;
      Qgos_dVdse   = 0.0 ;
      Qgos_dVgse   = - Cgso ;
    } else {
      Cgso         = - ( Qgos_dVbse + Qgod_dVbse
                   + Qgos_dVdse + Qgod_dVdse
                   + Qgos_dVgse + Qgod_dVgse ) ;
    }

      Cggo         = Qgos_dVgse + Qgod_dVgse ;


/*---------------------------------------------------* 
* Fringing capacitance.
*-----------------*/ 

    Cf  = C_EOX / ( C_Pi / 2.0e0 ) * Weff 
        * log( 1.0e0 + sIN.tpoly / sIN. tox ) ; 

    Qfd = Cf * ( Vgse - Vdse ) ;
    Qfs = Cf * Vgse ;


/*---------------------------------------------------* 
* Add fringing charge/capacitance to overlap.
*-----------------*/ 

    Qgod += Qfd ;
    Qgos += Qfs ;

    Cggo += 2.0 * Cf ;
    Cgdo += - Cf ;
    Cgso += - Cf ;


/*-----------------------------------------------------------* 
* Assign outputs.
*-----------------*/

/*---------------------------------------------------*
* Channel current and conductances. 
*-----------------*/
 
    /*    pOT->ids    = sIN.type * Ids ; */
    pOT->ids    = Ids ; 
    pOT->gmbs   = (Ids_dVbse >= 0.0) ? Ids_dVbse : Gjmin ;
    pOT->gds    = (Ids_dVdse >= 0.0) ? Ids_dVdse : Gjmin ;
    pOT->gm     = (Ids_dVgse >= 0.0) ? Ids_dVgse : Gjmin ;


/*---------------------------------------------------*
* Intrinsic charges/capacitances.
*-----------------*/

    pOT->qg = - ( Qb + Qi ) ;
    pOT->qd = Qd ;
    pOT->qs = Qi - Qd ;

    pOT->cbgb   =   Qb_dVgse ;
    pOT->cbdb   =   Qb_dVdse ;
    pOT->cbsb   = - ( Qb_dVbse + Qb_dVdse + Qb_dVgse ) ;

    pOT->cggb   = - Qb_dVgse - Qi_dVgse ;
    pOT->cgdb   = - Qb_dVdse - Qi_dVdse ;
    pOT->cgsb   =   Qb_dVbse + Qb_dVdse + Qb_dVgse
                  + Qi_dVbse + Qi_dVdse + Qi_dVgse ;

    pOT->cdgb   =   Qd_dVgse ;
    pOT->cddb   =   Qd_dVdse ;
    pOT->cdsb   = - ( Qd_dVgse + Qd_dVdse + Qd_dVbse ) ;
 

/*---------------------------------------------------*
* Overlap capacitances.
*-----------------*/

    pOT->cgdo = Cgdo ;
    pOT->cgso = Cgso ;
    pOT->cgbo = Cgbo ;


/*---------------------------------------------------*
* Lateral-field-induced capacitance.
*-----------------*/

    if ( sIN.xqy == 0 ){
      Qy = 0e0 ;
        Cqyd = 0e0 ;
        Cqyg = 0e0 ;
        Cqyb = 0e0 ;
        Cqys = 0e0 ;
    } else {
      Pslk = Ec * Leff  + Ps0 ;
        Pslk_dVbs = Ec_dVbs * Leff + Ps0_dVbs;
        Pslk_dVds = Ec_dVds * Leff + Ps0_dVds;
        Pslk_dVgs = Ec_dVgs * Leff + Ps0_dVgs;
   if ( Pslk > Psdl ){
      Pslk = Psdl ;
       Pslk_dVbs = Psdl_dVbs ;
       Pslk_dVds = Psdl_dVds ;
       Pslk_dVgs = Psdl_dVgs ;
   }

      T1 = Aclm * ( Vds + Ps0 ) + ( 1.0e0 - Aclm ) * Pslk ;
         T1_dVb = Aclm * (  Ps0_dVbs ) + ( 1.0e0 - Aclm ) * Pslk_dVbs ;
         T1_dVd = Aclm * ( 1.0 + Ps0_dVds ) + ( 1.0e0 - Aclm ) * Pslk_dVds ;
         T1_dVg = Aclm * (  Ps0_dVgs ) + ( 1.0e0 - Aclm ) * Pslk_dVgs ;

         T10 = sqrt( 2.0e0 * C_ESI / q_Nsub ) ;
 
         T3 = T10 * 1.3 ;
         T2 = C_ESI * Weff * T3 ;

       Qy = ( ( Ps0 + Vds  - T1 ) / sIN.xqy ) * T2 ;
       Cqyd = ( ( Ps0_dVds + 1.0e0 - T1_dVd ) / sIN.xqy ) * T2 ;
       if ( Cqyd < 0.0e0 ) Cqyd = 0.0 ;
       Cqyg = ( ( Ps0_dVgs  - T1_dVg ) / sIN.xqy ) * T2 ;
       Cqyb = ( ( Ps0_dVbs  - T1_dVb ) / sIN.xqy ) * T2 ;
       Cqys = - ( Cqyb +  Cqyd + Cqyg ) ;
    }


/*---------------------------------------------------*
* Add S/D overlap charges/capacitances to intrinsic ones.
* - NOTE: This function depends on coadov, a control option.
*-----------------*/

    if ( sIN.coadov == 1 ) {
        pOT->qg += Qgod + Qgos + Qy ;
        pOT->qd += - Qgod - Qy ;
        pOT->qs += - Qgos ;

        pOT->cggb   += Cggo + Cqyg ;
        pOT->cgdb   += Cgdo + (-1) * Cqyd ;
        pOT->cgsb   += Cgso + (-1) * Cqys ;

        pOT->cdgb   += - Qgod_dVgse - Cf + Cqyg ;
        pOT->cddb   += - Qgod_dVdse + Cf - Cqyd ;
        pOT->cdsb   += Qgod_dVbse + Qgod_dVdse + Qgod_dVgse + Cqys ;

        pOT->cgdo = 0.0 ;
        pOT->cgso = 0.0 ;

        if ( pOT->cgdb > -1e-27 ) { pOT->cgdb = 0e0 ; }
        if ( pOT->cgsb > -1e-27 ) { pOT->cgsb = 0e0 ; }
    }


/*---------------------------------------------------* 
* Substrate/gate/leak currents.
*-----------------*/ 
 
    pOT->isub   = Isub ;

    pOT->gbbs   = Isub_dVbse ;
    pOT->gbds   = Isub_dVdse ;
    pOT->gbgs   = Isub_dVgse ;
    
    pOT->igateb   = (1.0 - sIN.glpart1) * Igate ;

    pOT->ggbbs   = (1.0 - sIN.glpart1) * Igate_dVbse ;
    pOT->ggbgs   = (1.0 - sIN.glpart1) * Igate_dVgse ;
    if (sIN.mode == HiSIM_NORMAL_MODE) {
      pOT->ggbds   = (1.0 - sIN.glpart1) * Igate_dVdse ;
    } else {
      pOT->ggbds   = - (1.0 - sIN.glpart1) * 
	( Igate_dVbse + Igate_dVdse + Igate_dVgse ) ;
    }

    if (sIN.mode == HiSIM_NORMAL_MODE) {
      pOT->igated   = sIN.glpart1 * sIN.glpart2 * Igate ;
      
      pOT->ggdbs   = sIN.glpart1 * sIN.glpart2 * Igate_dVbse ;
      pOT->ggdds   = sIN.glpart1 * sIN.glpart2 * Igate_dVdse ;
      pOT->ggdgs   = sIN.glpart1 * sIN.glpart2 * Igate_dVgse ;
    } else {
      pOT->igated   = sIN.glpart1 * (1.0 - sIN.glpart2) * Igate ;
      
      pOT->ggdbs   = sIN.glpart1 * (1.0 - sIN.glpart2) * Igate_dVbse ;
      pOT->ggdds   = sIN.glpart1 * (1.0 - sIN.glpart2) * Igate_dVdse ;
      pOT->ggdgs   = sIN.glpart1 * (1.0 - sIN.glpart2) * Igate_dVgse ;
    }

    if (sIN.mode == HiSIM_NORMAL_MODE) {
      pOT->igates   = sIN.glpart1 * (1.0 - sIN.glpart2) * Igate ;
      
      pOT->ggsbs   = sIN.glpart1 * (1.0 - sIN.glpart2) * Igate_dVbse ;
      pOT->ggsds   = sIN.glpart1 * (1.0 - sIN.glpart2) * Igate_dVdse ;
      pOT->ggsgs   = sIN.glpart1 * (1.0 - sIN.glpart2) * Igate_dVgse ;
    } else {
      pOT->igates   = sIN.glpart1 * sIN.glpart2 * Igate ;
      
      pOT->ggsbs   = sIN.glpart1 * sIN.glpart2 * Igate_dVbse ;
      pOT->ggsds   = sIN.glpart1 * sIN.glpart2 * Igate_dVdse ;
      pOT->ggsgs   = sIN.glpart1 * sIN.glpart2 * Igate_dVgse ;
    }
 
    pOT->igidl   = (sIN.mode == HiSIM_NORMAL_MODE) ? Igidl : Igisl ;

    pOT->ggidlbs   = (sIN.mode == HiSIM_NORMAL_MODE) ? Igidl_dVbse : Igisl_dVbde ;
    pOT->ggidlds   = (sIN.mode == HiSIM_NORMAL_MODE) ? Igidl_dVdse : Igisl_dVsde ;
    pOT->ggidlgs   = (sIN.mode == HiSIM_NORMAL_MODE) ? Igidl_dVgse : Igisl_dVgde ;

    pOT->igisl   = (sIN.mode == HiSIM_NORMAL_MODE) ? Igisl : Igidl ;

    pOT->ggislbd   = (sIN.mode == HiSIM_NORMAL_MODE) ? Igisl_dVbde : Igidl_dVbse ;
    pOT->ggislsd   = (sIN.mode == HiSIM_NORMAL_MODE) ? Igisl_dVsde : Igidl_dVdse ;
    pOT->ggislgd   = (sIN.mode == HiSIM_NORMAL_MODE) ? Igisl_dVgde : Igidl_dVgse ;


/*---------------------------------------------------* 
* Von, Vdsat.
*-----------------*/ 

    /*
    pOT->von    = sIN.type * Vth ;
    pOT->vdsat  = sIN.type * Vdsat ;
    */
    pOT->von    = Vth ;
    pOT->vdsat  = Vdsat ;


/*---------------------------------------------------* 
* Parasitic conductances. 
*-----------------*/ 

/*:org:
*    pOT->gd     = 1.0e+50 ;
*    pOT->gs     = 1.0e+50 ;
* modified according to CMIxxxeval.c.
* I don't know the reason why these can be zero.
*/
    pOT->gd     = 0.0e0 ;
    pOT->gs     = 0.0e0 ;


/*---------------------------------------------------* 
* Junction diode.
*-----------------*/ 

    /*
    pOT->ibs = sIN.type * Ibs ;
    pOT->ibd = sIN.type * Ibd ;
    */
    pOT->ibs = Ibs ;
    pOT->ibd = Ibd ;
    pOT->gbs = Gbse ;
    pOT->gbd = Gbde ;

    pOT->qbs = Qbs ;
    pOT->qbd = Qbd ;
    pOT->capbs = Capbse ;
    pOT->capbd = Capbde ;


/*-----------------------------------------------------------* 
* Warn floating-point exceptions.
* - Function finite() in libm is called.
* - Go to start with info==5.
*-----------------*/ 

    T1 = pOT->ids + pOT->gmbs + pOT->gds + pOT->gm ;
    T1 = T1 + pOT->qd + pOT->cdsb ;
    if ( ! finite( T1 ) ) {
      flg_err = 1 ;
      fprintf( stderr ,
               "*** warning(HiSIM): FP-exception (PART-1)\n" ) ;
      if ( flg_info >= 1 ) {
          printf( "*** warning(HiSIM): FP-exception\n" ) ;
          printf( "pOT->ids   = %12.5e\n" , pOT->ids       ) ;
          printf( "pOT->gmbs  = %12.5e\n" , pOT->gmbs       ) ;
          printf( "pOT->gds   = %12.5e\n" , pOT->gds       ) ;
          printf( "pOT->gm    = %12.5e\n" , pOT->gm       ) ;
          printf( "pOT->qd    = %12.5e\n" , pOT->qd       ) ;
          printf( "pOT->cdsb  = %12.5e\n" , pOT->cdsb       ) ;
      }
    }

    T1 = pOT->isub + pOT->gbbs + pOT->gbds + pOT->gbgs ;
    if ( ! finite( T1 ) ) {
      flg_err = 1 ;
      fprintf( stderr ,
               "*** warning(HiSIM): FP-exception (PART-2)\n" ) ;
      if ( flg_info >= 1 ) {
          printf( "*** warning(HiSIM): FP-exception\n" ) ;
      }
    }

    T1 = pOT->cgbo + Cgdo + Cgso + Cggo ;
    if ( ! finite( T1 ) ) {
      flg_err = 1 ;
      fprintf( stderr ,
               "*** warning(HiSIM): FP-exception (PART-3)\n" ) ;
      if ( flg_info >= 1 ) {
          printf( "*** warning(HiSIM): FP-exception\n" ) ;
      }
    }

    T1 = pOT->ibs + pOT->ibd + pOT->gbs + pOT->gbd ;
    T1 = T1 + pOT->qbs + pOT->qbd + pOT->capbs + pOT->capbd ;
    if ( ! finite( T1 ) ) {
      flg_err = 1 ;
      fprintf( stderr ,
               "*** warning(HiSIM): FP-exception (PART-4)\n" ) ;
      if ( flg_info >= 1 ) {
          printf( "*** warning(HiSIM): FP-exception\n" ) ;
      }
    }


/*-----------------------------------------------------------* 
* Exit for error case.
*-----------------*/ 

    if ( flg_err != 0 ) {
      return( HiSIM_ERROR ) ;
    }


/*-----------------------------------------------------------* 
* Restore values for next calculation.
*-----------------*/ 

    /* Confined biases */
    pOT->vbsc = Vbsc ;
    pOT->vdsc = Vdsc ;
    pOT->vgsc = Vgsc ;

    /* Surface potentials and derivatives w.r.t. internal biases */
    pOT->ps0 = Ps0 ;
    pOT->ps0_dvbs = Ps0_dVbs ;
    pOT->ps0_dvds = Ps0_dVds ;
    pOT->ps0_dvgs = Ps0_dVgs ;
    pOT->pds = Pds ;
    pOT->pds_dvbs = Pds_dVbs ;
    pOT->pds_dvds = Pds_dVds ;
    pOT->pds_dvgs = Pds_dVgs ;

    /* Derivatives of channel current w.r.t. internal biases */
    pOT->ids_dvbs = Ids_dVbs ;
    pOT->ids_dvds = Ids_dVds ;
    pOT->ids_dvgs = Ids_dVgs ;


/*-----------------------------------------------------------* 
* etc.
*-----------------*/ 

    pOT->nf = Nflic ;
    pOT->mu = Mu ;
    pOT->qg_int = - ( Qb + Qi ) ;
    pOT->qd_int = Qd ;
    pOT->qs_int = Qi - Qd ;
    pOT->qb_int = Qb ;


/*-----------------------------------------------------------* 
* End of PART-6. (label) 
*-----------------*/ 

//end_of_part_6: ;


/*-----------------------------------------------------------* 
* Bottom of hsm1eval. 
*-----------------*/ 



return ( HiSIM_OK ) ;

} /* end of hsm1eval120 */

