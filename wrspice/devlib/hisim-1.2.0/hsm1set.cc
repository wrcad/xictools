
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
 $Id: hsm1set.cc,v 1.8 2016/09/26 01:48:07 stevew Exp $
 *========================================================================*/

/***********************************************************************
 HiSIM (Hiroshima University STARC IGFET Model)
 Copyright (C) 2003 STARC

 VERSION : HiSIM 1.2.0
 FILE : hsm1set.c of HiSIM 1.2.0

 April 9, 2003 : released by STARC Physical Design Group
***********************************************************************/

#include "hsm1defs.h"
#include "gencurrent.h"
#include <stdio.h>

#define HSM1nextModel      next()
#define HSM1nextInstance   next()
#define HSM1instances      inst()
#define CKTnode sCKTnode
#define HSM1states GENstate
#define HSM1name GENname
#define CKTmkVolt(a, b, c, d) (a)->mkVolt(b, c, d)
#define CKTdltNNum(c, n)


namespace {
    int get_node_ptr(sCKT *ckt, sHSM1instance *inst)
    {
      TSTALLOC(HSM1DdPtr, HSM1dNode, HSM1dNode)
      TSTALLOC(HSM1GgPtr, HSM1gNode, HSM1gNode)
      TSTALLOC(HSM1SsPtr, HSM1sNode, HSM1sNode)
      TSTALLOC(HSM1BbPtr, HSM1bNode, HSM1bNode)
      TSTALLOC(HSM1DPdpPtr, HSM1dNodePrime, HSM1dNodePrime)
      TSTALLOC(HSM1SPspPtr, HSM1sNodePrime, HSM1sNodePrime)
      TSTALLOC(HSM1DdpPtr, HSM1dNode, HSM1dNodePrime)
      TSTALLOC(HSM1GbPtr, HSM1gNode, HSM1bNode)
      TSTALLOC(HSM1GdpPtr, HSM1gNode, HSM1dNodePrime)
      TSTALLOC(HSM1GspPtr, HSM1gNode, HSM1sNodePrime)
      TSTALLOC(HSM1SspPtr, HSM1sNode, HSM1sNodePrime)
      TSTALLOC(HSM1BdpPtr, HSM1bNode, HSM1dNodePrime)
      TSTALLOC(HSM1BspPtr, HSM1bNode, HSM1sNodePrime)
      TSTALLOC(HSM1DPspPtr, HSM1dNodePrime, HSM1sNodePrime)
      TSTALLOC(HSM1DPdPtr, HSM1dNodePrime, HSM1dNode)
      TSTALLOC(HSM1BgPtr, HSM1bNode, HSM1gNode)
      TSTALLOC(HSM1DPgPtr, HSM1dNodePrime, HSM1gNode)
      TSTALLOC(HSM1SPgPtr, HSM1sNodePrime, HSM1gNode)
      TSTALLOC(HSM1SPsPtr, HSM1sNodePrime, HSM1sNode)
      TSTALLOC(HSM1DPbPtr, HSM1dNodePrime, HSM1bNode)
      TSTALLOC(HSM1SPbPtr, HSM1sNodePrime, HSM1bNode)
      TSTALLOC(HSM1SPdpPtr, HSM1sNodePrime, HSM1dNodePrime)
      return (OK);
    }
}


int
HSM1dev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
    sHSM1model *model = static_cast<sHSM1model*>(genmod);
    sHSM1instance *here;

  int error;
  CKTnode *tmp;
  
  /*  loop through all the HSM1 device models */
  for ( ;model != NULL ;model = model->HSM1nextModel ) {
    /* Default value Processing for HSM1 MOSFET Models */
    if ( !model->HSM1_type_Given )
      model->HSM1_type = NMOS ;
    /***/
    if ( !model->HSM1_info_Given ) model->HSM1_info = 0 ;
    if ( !model->HSM1_noise_Given) model->HSM1_noise = 5; /* select noise model 5 */
    if ( !model->HSM1_version_Given) {
        model->HSM1_version = 120; /* default 120 */
	printf("           120 is selected for VERSION. (default) \n");
    }
    if ( model->HSM1_version == 100 || model->HSM1_version == 101) {
	printf("warning(HiSIM1): For the model parameter VERSION, 102 or 112 or 120 is acceptable.\n");
	printf("           102 is selected for VERSION \n");
	model->HSM1_version = 102 ;
    }
    else if ( model->HSM1_version == 110 || model->HSM1_version == 111) {
	printf("warning(HiSIM1): For the model parameter VERSION, 102 or 112 or 120 is acceptable.\n");
	printf("           112 is selected for VERSION \n");
	model->HSM1_version = 112 ;
    }
    else if ( model->HSM1_version != 102 &&
	 model->HSM1_version != 112 &&
	 model->HSM1_version != 120) {
	printf("warning(HiSIM1): For the model parameter VERSION, 102 or 112 or 120 is acceptable.\n");
	printf("           120 is selected for VERSION compulsorily.\n");
	model->HSM1_version = 120 ;
    }
    else {
      printf("           %d is selected for VERSION \n", (int)model->HSM1_version);
    }

    if ( !model->HSM1_corsrd_Given ) model->HSM1_corsrd = 0 ;
    if ( !model->HSM1_coiprv_Given ) model->HSM1_coiprv = 1 ;
    if ( !model->HSM1_copprv_Given ) model->HSM1_copprv = 1 ;
    if ( !model->HSM1_cocgso_Given ) model->HSM1_cocgso = 0 ;
    if ( !model->HSM1_cocgdo_Given ) model->HSM1_cocgdo = 0 ;
    if ( !model->HSM1_cocgbo_Given ) model->HSM1_cocgbo = 0 ;
    if ( !model->HSM1_coadov_Given ) model->HSM1_coadov = 1 ;
    if ( !model->HSM1_coxx08_Given ) model->HSM1_coxx08 = 0 ;
    if ( !model->HSM1_coxx09_Given ) model->HSM1_coxx09 = 0 ;
    if ( !model->HSM1_coisub_Given ) model->HSM1_coisub = 0 ;
    if ( !model->HSM1_coiigs_Given ) model->HSM1_coiigs = 0 ;
    if ( !model->HSM1_cogidl_Given ) model->HSM1_cogidl = 0 ;
    if ( model->HSM1_version == 120 ) {/* HiSIM1.2.0 */
      if ( !model->HSM1_cogisl_Given ) model->HSM1_cogisl = 0 ;
    }
    if ( !model->HSM1_coovlp_Given ) model->HSM1_coovlp = 0 ;
    if ( !model->HSM1_conois_Given ) model->HSM1_conois = 0 ;
    if ( model->HSM1_version == 112 || model->HSM1_version == 120) {
      /* HiSIM1.1.2 HiSIM1.2.0*/
      if ( !model->HSM1_coisti_Given ) model->HSM1_coisti = 0 ;
    }
    if ( model->HSM1_version == 120 ) {/* HiSIM1.2.0 */
      if ( !model->HSM1_cosmbi_Given ) model->HSM1_cosmbi = 0 ;
    }
    else {
      if (model->HSM1_cosmbi_Given)
	printf ("warning(HiSIM1): COSMBI is only available for VERSION = 120\n");
      if ( model->HSM1_kappa_Given ) 
	printf ("warning(HiSIM1): KAPPA is only available for VERSION = 120\n");
      if ( model->HSM1_xdiffd_Given ) 
	printf ("warning(HiSIM1): XDIFFD is only available for VERSION = 120\n");
      if ( model->HSM1_vdiffj_Given ) 
	printf ("warning(HiSIM1): VDIFFJ is only available for VERSION = 120\n");
      if ( model->HSM1_pthrou_Given ) 
	printf ("warning(HiSIM1): PTHROU is only available for VERSION = 120\n");
      if ( model->HSM1_glpart1_Given ) 
	printf ("warning(HiSIM1): GLPART1 is only available for VERSION = 120\n");
      if ( model->HSM1_glpart2_Given ) 
	printf ("warning(HiSIM1): GLPART2 is only available for VERSION = 120\n");

      /*
      printf ("               This parameter is ignored.\n");
      */
    }
      
    /***/
    if ( !model->HSM1_vmax_Given ) model->HSM1_vmax = 7.00e+6 ;
    if ( !model->HSM1_bgtmp1_Given ) model->HSM1_bgtmp1 = 90.25e-6 ;
    if ( !model->HSM1_bgtmp2_Given ) model->HSM1_bgtmp2 = 100.0e-9 ;
    if ( !model->HSM1_tox_Given ) model->HSM1_tox = 5.0e-9 ;
    else if ( model->HSM1_tox < 0 ) {
      printf("warning(HiSIM1): The model parameter TOX must be positive.\n");
    }
    if ( !model->HSM1_xld_Given ) model->HSM1_xld = 0.0 ;
    if ( !model->HSM1_xwd_Given ) model->HSM1_xwd = 0.0 ;
    if ( model->HSM1_version == 102 ) { /* HiSIM1.0 */
      if ( !model->HSM1_xj_Given ) model->HSM1_xj = 0.0 ;
      else if ( model->HSM1_xj < 0 ) {
	printf("warning(HiSIM1): The model parameter XJ must be positive.\n");
      }
      if ( model->HSM1_xqy_Given ) {
	printf("warning(HiSIM1): XQY is only available for VERSION = 112 or 120\n");
      }
    }
    else if ( model->HSM1_version == 112 ||
	      model->HSM1_version == 120 ) { /* HiSIM1.1.2 / 1.2.0 */
      if ( !model->HSM1_xqy_Given ) model->HSM1_xqy = 0.0;
      else if ( model->HSM1_xqy < 0 ) {
	printf("warning(HiSIM1): The model parameter XQY must be positive.\n");
      }
      if ( model->HSM1_xj_Given ) {
	printf("warning(HiSIM1): XJ is only available for VERSION = 102\n");
      }
    }
    if ( !model->HSM1_rs_Given ) model->HSM1_rs = 80.0e-6 ;
    else if ( model->HSM1_rs < 0 ) {
      printf("warning(HiSIM1): The model parameter RS must be positive.\n");
    }
    if ( !model->HSM1_rd_Given ) model->HSM1_rd = 80.0e-6 ;
    else if ( model->HSM1_rd < 0 ) {
      printf("warning(HiSIM1): The model parameter RD must be positive.\n");
    }
    if ( !model->HSM1_vfbc_Given ) model->HSM1_vfbc = -1.0 ;
    if ( !model->HSM1_nsubc_Given ) model->HSM1_nsubc = 1.0e+17 ;
    else if ( model->HSM1_nsubc < 0 ) {
      printf("warning(HiSIM1): The model parameter NSUBC must be positive.\n");
    }
    if ( model->HSM1_version == 120 ) model->HSM1_parl1 = 1.0 ;
    else {
      if ( !model->HSM1_parl1_Given ) model->HSM1_parl1 = 1.0 ;
    }
    if ( !model->HSM1_parl2_Given ) model->HSM1_parl2 = 0.0 ;
    if ( !model->HSM1_lp_Given ) model->HSM1_lp = 15.0e-9 ;
    if ( !model->HSM1_nsubp_Given ) model->HSM1_nsubp = 1.0e+17 ;
    else if ( model->HSM1_nsubp < 0 ) {
      printf("warning(HiSIM1): The model parameter NSUBP must be positive.\n");
    }
    if ( !model->HSM1_scp1_Given ) model->HSM1_scp1 = 0.0 ;
    if ( !model->HSM1_scp2_Given ) model->HSM1_scp2 = 0.0 ;
    if ( !model->HSM1_scp3_Given ) model->HSM1_scp3 = 0.0 ;
    if ( !model->HSM1_sc1_Given ) model->HSM1_sc1 = 0.0 ;
    if ( !model->HSM1_sc2_Given ) model->HSM1_sc2 = 0.0 ;
    if ( !model->HSM1_sc3_Given ) model->HSM1_sc3 = 0.0 ;
    if ( !model->HSM1_pgd1_Given ) model->HSM1_pgd1 = 10.0e-3 ;
    if ( !model->HSM1_pgd2_Given ) model->HSM1_pgd2 = 1.0 ;
    if ( !model->HSM1_pgd3_Given ) model->HSM1_pgd3 = 0.8 ;
    if ( !model->HSM1_ndep_Given ) model->HSM1_ndep = 1.0 ;
    if ( !model->HSM1_ninv_Given ) model->HSM1_ninv = 0.5 ;
    if ( !model->HSM1_ninvd_Given ) model->HSM1_ninvd = 1.0e-9 ;
    if ( !model->HSM1_muecb0_Given ) model->HSM1_muecb0 = 300.0 ;
    if ( !model->HSM1_muecb1_Given ) model->HSM1_muecb1 = 30.0 ;
    if ( !model->HSM1_mueph1_Given ) model->HSM1_mueph1 = 25.0e3 ;
    if ( !model->HSM1_mueph0_Given ) model->HSM1_mueph0 = 300.0e-3 ;
    if ( !model->HSM1_mueph2_Given ) model->HSM1_mueph2 = 0.0 ;
    if ( !model->HSM1_w0_Given ) model->HSM1_w0 = 0.0 ;
    if ( !model->HSM1_muesr1_Given ) model->HSM1_muesr1 = 2.0e15;
    if ( !model->HSM1_muesr0_Given ) model->HSM1_muesr0 = 2.0 ;
    if ( !model->HSM1_muetmp_Given ) model->HSM1_muetmp = 1.5 ;
    /***/
    if ( !model->HSM1_bb_Given ) {
      if (model->HSM1_type == NMOS) model->HSM1_bb = 2.0 ;
      else model->HSM1_bb = 1.0 ;
    }
    /***/
    if ( !model->HSM1_sub1_Given ) model->HSM1_sub1 = 10.0 ;
    if ( !model->HSM1_sub2_Given ) model->HSM1_sub2 = 20.0 ;
    if ( !model->HSM1_sub3_Given ) model->HSM1_sub3 = 0.8 ;
    if ( model->HSM1_version == 112 ||
	 model->HSM1_version == 120)  { /* HiSIM1.1.2 / 1.2.0 */
      if ( !model->HSM1_wvthsc_Given ) model->HSM1_wvthsc = 0.0 ;
      if ( !model->HSM1_nsti_Given ) model->HSM1_nsti = 1.0e17 ;
      if ( !model->HSM1_wsti_Given ) model->HSM1_wsti = 0.0 ;
    } else {
      if ( model->HSM1_wvthsc_Given ) 
	printf ("warning(HiSIM1): WVTHSC is only available for VERSION = 112 or 120\n");
      if ( model->HSM1_nsti_Given ) 
	printf ("warning(HiSIM1): NSTI is only available for VERSION = 112 or 120\n");
      if ( model->HSM1_wsti_Given ) 
	printf ("warning(HiSIM1): WSTI is only available for VERSION = 112 or 120\n");
    }
    if ( !model->HSM1_tpoly_Given ) model->HSM1_tpoly = 0.0 ;
    if ( !model->HSM1_js0_Given ) model->HSM1_js0 = 1.0e-4 ;
    if ( !model->HSM1_js0sw_Given ) model->HSM1_js0sw = 0.0 ;
    if ( !model->HSM1_nj_Given ) model->HSM1_nj = 1.0 ;
    if ( !model->HSM1_njsw_Given ) model->HSM1_njsw = 1.0 ;
    if ( !model->HSM1_xti_Given ) model->HSM1_xti = 3.0 ;
    if ( !model->HSM1_cj_Given ) model->HSM1_cj = 5.0e-04 ;
    if ( !model->HSM1_cjsw_Given ) model->HSM1_cjsw = 5.0e-10 ;
    if ( !model->HSM1_cjswg_Given ) model->HSM1_cjswg = 5.0e-10 ;
    if ( !model->HSM1_mj_Given ) model->HSM1_mj = 0.5 ;
    if ( !model->HSM1_mjsw_Given ) model->HSM1_mjsw = 0.33 ;
    if ( !model->HSM1_mjswg_Given ) model->HSM1_mjswg = 0.33 ;
    if ( !model->HSM1_pb_Given ) model->HSM1_pb = 1.0 ;
    if ( !model->HSM1_pbsw_Given ) model->HSM1_pbsw = 1.0 ;
    if ( !model->HSM1_pbswg_Given ) model->HSM1_pbswg = 1.0 ;
    if ( !model->HSM1_xpolyd_Given ) model->HSM1_xpolyd = 0.0 ;
    if ( !model->HSM1_clm1_Given ) model->HSM1_clm1 = 700.0e-3 ;
    if ( !model->HSM1_clm2_Given ) model->HSM1_clm2 = 2.0 ;
    if ( !model->HSM1_clm3_Given ) model->HSM1_clm3 = 1.0 ;
    if ( !model->HSM1_rpock1_Given ) {
      if ( model->HSM1_version == 102 ) {
	model->HSM1_rpock1 = 10.0e-3 ;
      } else if ( model->HSM1_version == 112 || model->HSM1_version == 120 ) {
	model->HSM1_rpock1 = 0.1e-3 ;
      }
    }
    if ( !model->HSM1_rpock2_Given ) model->HSM1_rpock2 = 100.0e-3 ;
    if ( model->HSM1_version == 112 ||
	 model->HSM1_version == 120) { /* HiSIM1.1.2 / 1.2.0 */
      if ( !model->HSM1_rpocp1_Given ) model->HSM1_rpocp1 = 1.0 ;
      if ( !model->HSM1_rpocp2_Given ) model->HSM1_rpocp2 = 0.5 ;
    } else {
      if ( model->HSM1_rpocp1_Given ) 
	printf ("warning(HiSIM1): RPOCP1 is only available for VERSION = 112 or 120\n");
      if ( model->HSM1_rpocp2_Given ) 
	printf ("warning(HiSIM1): RPOCP2 is only available for VERSION = 112 or 120\n");
    }
    if ( !model->HSM1_vover_Given ) model->HSM1_vover = 10.0e-3 ;
    if ( !model->HSM1_voverp_Given ) model->HSM1_voverp = 100.0e-3 ;
    if ( !model->HSM1_wfc_Given ) model->HSM1_wfc = 0.0 ;
    if ( !model->HSM1_qme1_Given ) model->HSM1_qme1 = 40.0e-12 ;
    if ( !model->HSM1_qme2_Given ) model->HSM1_qme2 = 300.0e-12 ;
    if ( !model->HSM1_qme3_Given ) model->HSM1_qme3 = 0.0 ;
    if ( !model->HSM1_gidl1_Given ) {
      if (model->HSM1_version == 101) model->HSM1_gidl1 = 5.0e-3 ;
      else if (model->HSM1_version == 112 ||
	       model->HSM1_version == 120) model->HSM1_gidl1 = 5.0e-6 ;
    }
    if ( !model->HSM1_gidl2_Given ) model->HSM1_gidl2 = 1.0e6 ;
    if ( !model->HSM1_gidl3_Given ) model->HSM1_gidl3 = 300.0e-3 ;
    if ( !model->HSM1_gleak1_Given ) {
      if (model->HSM1_version == 101) model->HSM1_gleak1 = 0.01e6 ;
      else if (model->HSM1_version == 112 ||
	       model->HSM1_version == 120) model->HSM1_gleak1 = 10.0e3 ;
    }
    if ( !model->HSM1_gleak2_Given ) model->HSM1_gleak2 = 20.0e6 ;
    if ( !model->HSM1_gleak3_Given ) model->HSM1_gleak3 = 300.0e-3 ;
    if ( !model->HSM1_vzadd0_Given ) model->HSM1_vzadd0 = 10.0e-3 ;
    if ( !model->HSM1_pzadd0_Given ) model->HSM1_pzadd0 = 5.0e-3 ;
    if ( !model->HSM1_nftrp_Given ) model->HSM1_nftrp = 10e9 ;
    if ( !model->HSM1_nfalp_Given ) model->HSM1_nfalp = 1.0e-16 ;
    if ( !model->HSM1_cit_Given ) model->HSM1_cit = 0.0 ;
    if ( model->HSM1_version == 120) { /* HiSIM1.2.0 */
      if ( !model->HSM1_glpart1_Given ) model->HSM1_glpart1 = 1 ;
      if ( !model->HSM1_glpart2_Given ) model->HSM1_glpart2 = 0.5 ;
      if ( !model->HSM1_kappa_Given ) model->HSM1_kappa = 3.90 ;
      if ( !model->HSM1_xdiffd_Given ) model->HSM1_xdiffd = 0.0;
      if ( !model->HSM1_pthrou_Given ) model->HSM1_pthrou = 0.0;
      if ( !model->HSM1_vdiffj_Given ) model->HSM1_vdiffj = 0.5;
    }

    /* for flicker noise the same as BSIM3 */
    if ( !model->HSM1_ef_Given ) model->HSM1_ef = 0.0;
    if ( !model->HSM1_af_Given ) model->HSM1_af = 1.0;
    if ( !model->HSM1_kf_Given ) model->HSM1_kf = 0.0;

    /* loop through all the instances of the model */
    for ( here = model->HSM1instances ;here != NULL ;
	  here = here->HSM1nextInstance ) {
      /* allocate a chunk of the state vector */
      here->HSM1states = *states;
      *states += HSM1numStates;

      /* perform the parameter defaulting */
      if ( !here->HSM1_l_Given ) here->HSM1_l = ckt->mos_default_l();
      if ( !here->HSM1_w_Given ) here->HSM1_w = ckt->mos_default_w();
      if ( !here->HSM1_ad_Given ) here->HSM1_ad = ckt->mos_default_ad();
      if ( !here->HSM1_as_Given ) here->HSM1_as = ckt->mos_default_as();

      if ( !here->HSM1_pd_Given ) here->HSM1_pd = 0.0 ;
      if ( !here->HSM1_ps_Given ) here->HSM1_ps = 0.0 ;
      if ( !here->HSM1_nrd_Given ) here->HSM1_nrd = 0.0 ;
      if ( !here->HSM1_nrs_Given ) here->HSM1_nrs = 0.0 ;
      if ( !here->HSM1_temp_Given ) here->HSM1_temp = 300.15 ;
      if ( !here->HSM1_dtemp_Given ) here->HSM1_dtemp = 0.0 ;

      if ( !here->HSM1_icVBS_Given ) here->HSM1_icVBS = 0.0;
      if ( !here->HSM1_icVDS_Given ) here->HSM1_icVDS = 0.0;
      if ( !here->HSM1_icVGS_Given ) here->HSM1_icVGS = 0.0;
      
      here->HSM1_weff = here->HSM1_w - 2.0e0 * model->HSM1_xpolyd - 2.0e0 * model->HSM1_xld ;
      here->HSM1_leff = here->HSM1_l - 2.0e0 * model->HSM1_xdiffd - 2.0e0 * model->HSM1_xwd ;

      /* process source/drain series resistance added by K.M. */
      /* Drain and source conductances are always zero,
	 because there is no sheet resistance in HSM1 model param.
      if ( model->HSM1_corsrd ) 
	here->HSM1drainConductance = 0.0;
      else 
	here->HSM1drainConductance = model->HSM1_rs / here->HSM1_weff;
      
      if ( here->HSM1drainConductance > 0.0 )
	here->HSM1drainConductance = 1.0 / here->HSM1drainConductance;
      else
	here->HSM1drainConductance = 0.0;
      
      if ( model->HSM1_corsrd ) 
	here->HSM1sourceConductance = 0.0;
      else 
	here->HSM1sourceConductance = model->HSM1_rd / here->HSM1_weff;

      if ( here->HSM1sourceConductance > 0.0 ) 
	here->HSM1sourceConductance = 1.0 / here->HSM1sourceConductance;
      else
	here->HSM1sourceConductance = 0.0;
      */
      here->HSM1drainConductance = 0.0;
      here->HSM1sourceConductance = 0.0;
      
      /* process drain series resistance */
      if( here->HSM1drainConductance > 0.0 && here->HSM1dNodePrime == 0 ) {
	error = CKTmkVolt(ckt, &tmp, here->HSM1name, "drain");
	if (error) return(error);
	here->HSM1dNodePrime = tmp->number();
      } 
      else {
	here->HSM1dNodePrime = here->HSM1dNode;
      }
      
      /* process source series resistance */
      if( here->HSM1sourceConductance > 0.0 && here->HSM1sNodePrime == 0 ) {
	if ( here->HSM1sNodePrime == 0 ) {
	  error = CKTmkVolt(ckt, &tmp, here->HSM1name, "source");
	  if (error) return(error);
	  here->HSM1sNodePrime = tmp->number();
	}
      } 
      else  {
	here->HSM1sNodePrime = here->HSM1sNode;
      }
                   
      /* set Sparse Matrix Pointers */
      
      /* macro to make elements with built in test for out of memory */
/* SRW
#define TSTALLOC(ptr,first,second) \
if((here->ptr = SMPmakeElt(matrix,here->first,here->second))==(double *)NULL){\
    return(E_NOMEM);\
}
*/

            error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
    }
  }
  return(OK);
} 


// SRW
#undef inst
#define HSM1model sHSM1model
#define HSM1instance sHSM1instance

int
HSM1dev::unsetup(sGENmodel *inModel, sCKT*)
{
#ifndef HAS_BATCHSIM
    HSM1model *model;
    HSM1instance *here;

    for (model = (HSM1model *)inModel; model != NULL;
            model = model->HSM1nextModel)
    {
        for (here = model->HSM1instances; here != NULL;
                here=here->HSM1nextInstance)
        {
            if (here->HSM1dNodePrime
                    && here->HSM1dNodePrime != here->HSM1dNode)
            {
                CKTdltNNum(ckt, here->HSM1dNodePrime);
                here->HSM1dNodePrime = 0;
            }
            if (here->HSM1sNodePrime
                    && here->HSM1sNodePrime != here->HSM1sNode)
            {
                CKTdltNNum(ckt, here->HSM1sNodePrime);
                here->HSM1sNodePrime = 0;
            }
        }
    }
#endif
    return OK;
}


// SRW - reset the matrix element pointers.
//
int
HSM1dev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sHSM1model *model = (sHSM1model*)inModel; model;
            model = model->next()) {
        for (sHSM1instance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}


HSM1adj::HSM1adj() { matrix = new dvaMatrix; }

HSM1adj::~HSM1adj() { delete matrix; }

