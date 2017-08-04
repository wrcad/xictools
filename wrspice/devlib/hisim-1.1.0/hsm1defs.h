
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
 HiSIM v1.1.0
 File: hsm1def.h of HiSIM v1.1.0

 Copyright (C) 2002 STARC

 June 30, 2002: developed by Hiroshima University and STARC
 June 30, 2002: posted by Keiichi MORIKAWA, STARC Physical Design Group
***********************************************************************/

#ifndef HSM1DEFS_H
#define HSM1DEFS_H

#include "device.h"

#define NEWCONV

#define FABS            fabs
#define REFTEMP         wrsREFTEMP       
#define CHARGE          wrsCHARGE        
#define CONSTCtoK       wrsCONSTCtoK     
#define CONSTboltz      wrsCONSTboltz    
#define CONSTvt0        wrsCONSTvt0      
#define CONSTKoverQ     wrsCONSTKoverQ   
#define CONSTroot2      wrsCONSTroot2    
#define CONSTe          wrsCONSTe        

struct dvaMatrix;

namespace HSM110 {

struct sHSM1model;
struct sHSM1instance;

struct HSM1adj
{
    HSM1adj();
    ~HSM1adj();


    dvaMatrix *matrix;
};

struct HSM1dev : public IFdevice
{
    HSM1dev();
    sGENmodel *newModl();
    sGENinstance *newInst();
    int destroy(sGENmodel**);
    int delInst(sGENmodel*, IFuid, sGENinstance*);
    int delModl(sGENmodel**, IFuid, sGENmodel*);

    void parse(int, sCKT*, sLine*);
//    int loadTest(sGENinstance*, sCKT*);   
    int load(sGENinstance*, sCKT*);   
    int setup(sGENmodel*, sCKT*, int*);  
    int unsetup(sGENmodel*, sCKT*);
    int resetup(sGENmodel*, sCKT*);
    int temperature(sGENmodel*, sCKT*);    
    int getic(sGENmodel*, sCKT*);  
//    int accept(sCKT*, sGENmodel*); 
    int trunc(sGENmodel*, sCKT*, double*);  
    int convTest(sGENmodel*, sCKT*);  

    void backup(sGENmodel*, DEV_BKMODE);

    int setInst(int, IFdata*, sGENinstance*);  
    int setModl(int, IFdata*, sGENmodel*);   
    int askInst(const sCKT*, const sGENinstance*, int, IFdata*);    
    int askModl(const sGENmodel*, int, IFdata*); 
//    int findBranch(sCKT*, sGENmodel*, IFuid); 

    int acLoad(sGENmodel*, sCKT*); 
    int pzLoad(sGENmodel*, sCKT*, IFcomplex*); 
//    int disto(int, sGENmodel*, sCKT*);  
    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
};

struct sHSM1instance : public sGENinstance
{
    sHSM1instance()
        {
            memset(this, 0, sizeof(sHSM1instance));
            GENnumNodes = 4;
        }
    ~sHSM1instance()    { delete [] (char*)HSM1backing; }
    sHSM1instance *next()
        { return (static_cast<sHSM1instance*>(GENnextInstance)); }

  int HSM1dNode;      /* number of the drain node of the mosfet */
  int HSM1gNode;      /* number of the gate node of the mosfet */
  int HSM1sNode;      /* number of the source node of the mosfet */
  int HSM1bNode;      /* number of the bulk node of the mosfet */
  int HSM1dNodePrime; /* number od the inner drain node */
  int HSM1sNodePrime; /* number od the inner source node */

  char HSM1_called[4]; /* string to check the first call */

  /* previous values to evaluate initial guess */
  double HSM1_vbsc_prv;
  double HSM1_vdsc_prv;
  double HSM1_vgsc_prv;
  double HSM1_ps0_prv;
  double HSM1_ps0_dvbs_prv;
  double HSM1_ps0_dvds_prv;
  double HSM1_ps0_dvgs_prv;
  double HSM1_pds_prv;
  double HSM1_pds_dvbs_prv;
  double HSM1_pds_dvds_prv;
  double HSM1_pds_dvgs_prv;
  double HSM1_ids_prv;
  double HSM1_ids_dvbs_prv;
  double HSM1_ids_dvds_prv;
  double HSM1_ids_dvgs_prv;
  
  double HSM1_nfc; /* for noise calc. */

    // This provides a means to back up and restore a known-good
    // state.
    void *HSM1backing;
    void backup(DEV_BKMODE m)
        {
            if (m == DEV_SAVE) {
                if (!HSM1backing)
                    HSM1backing = new char[sizeof(sHSM1instance)];
                memcpy(HSM1backing, this, sizeof(sHSM1instance));
            }
            else if (m == DEV_RESTORE) {
                if (HSM1backing)
                    memcpy(this, HSM1backing, sizeof(sHSM1instance));
            }
            else {
                // DEV_CLEAR
                delete [] (char*)HSM1backing;
                HSM1backing = 0;
            }
        }

  /* instance */
  double HSM1_l;    /* the length of the channel region */
  double HSM1_w;    /* the width of the channel region */
  double HSM1_ad;   /* the area of the drain diffusion */
  double HSM1_as;   /* the area of the source diffusion */
  double HSM1_pd;   /* perimeter of drain junction [m] */
  double HSM1_ps;   /* perimeter of source junction [m] */
  double HSM1_nrd;  /* equivalent num of squares of drain [-] (unused) */
  double HSM1_nrs;  /* equivalent num of squares of source [-] (unused) */
  double HSM1_temp; /* lattice temperature [K] */
  double HSM1_dtemp;

  /* added by K.M. */
  double HSM1_weff; /* the effectiv width of the channel region */
  double HSM1_leff; /* the effectiv length of the channel region */

  /* output */
  int    HSM1_capop;
  double HSM1_gd;
  double HSM1_gs;
  double HSM1_cgso;
  double HSM1_cgdo;
  double HSM1_cgbo;
  double HSM1_von; /* vth */
  double HSM1_vdsat;
  double HSM1_ids; /* cdrain, HSM1_cd */
  double HSM1_gds;
  double HSM1_gm;
  double HSM1_gmbs;
  double HSM1_ibs; /* HSM1_cbs */
  double HSM1_ibd; /* HSM1_cbd */
  double HSM1_gbs;
  double HSM1_gbd;
  double HSM1_capbs;
  double HSM1_capbd;
  /*
  double HSM1_qbs;
  double HSM1_qbd;
  */
  double HSM1_capgs;
  double HSM1_capgd;
  double HSM1_capgb;
  double HSM1_isub; /* HSM1_csub */
  double HSM1_gbgs;
  double HSM1_gbds;
  double HSM1_gbbs;
  double HSM1_qg;
  double HSM1_qd;
  /*  double HSM1_qs; */
  double HSM1_qb;  /* bulk charge qb = -(qg + qd + qs) */
  double HSM1_cggb;
  double HSM1_cgdb;
  double HSM1_cgsb;
  double HSM1_cbgb;
  double HSM1_cbdb;
  double HSM1_cbsb;
  double HSM1_cdgb;
  double HSM1_cddb;
  double HSM1_cdsb;
  /* no use in SPICE3f5
  double HSM1_nois_irs;
  double HSM1_nois_ird;
  double HSM1_nois_idsth;
  double HSM1_nois_idsfl;
  double HSM1_freq;
  */

  /* added by K.M. */
  double HSM1_mu; /* mobility */

  /* no use in SPICE3f5
      double HSM1drainSquares;       the length of the drain in squares
      double HSM1sourceSquares;      the length of the source in squares */
  double HSM1sourceConductance; /* cond. of source (or 0): set in setup */
  double HSM1drainConductance;  /* cond. of drain (or 0): set in setup */

  double HSM1_icVBS; /* initial condition B-S voltage */
  double HSM1_icVDS; /* initial condition D-S voltage */
  double HSM1_icVGS; /* initial condition G-S voltage */
  int HSM1_off;      /* non-zero to indicate device is off for dc analysis */
  int HSM1_mode;     /* device mode : 1 = normal, -1 = inverse */

  unsigned HSM1_l_Given :1;
  unsigned HSM1_w_Given :1;
  unsigned HSM1_ad_Given :1;
  unsigned HSM1_as_Given    :1;
  /*  unsigned HSM1drainSquaresGiven  :1;
      unsigned HSM1sourceSquaresGiven :1;*/
  unsigned HSM1_pd_Given    :1;
  unsigned HSM1_ps_Given   :1;
  unsigned HSM1_nrd_Given  :1;
  unsigned HSM1_nrs_Given  :1;
  unsigned HSM1_temp_Given  :1;
  unsigned HSM1_dtemp_Given  :1;
  unsigned HSM1dNodePrimeSet  :1;
  unsigned HSM1sNodePrimeSet  :1;
  unsigned HSM1_icVBS_Given :1;
  unsigned HSM1_icVDS_Given :1;
  unsigned HSM1_icVGS_Given :1;
  
  /* pointer to sparse matrix */
  double *HSM1DdPtr;      /* pointer to sparse matrix element at 
			     (Drain node,drain node) */
  double *HSM1GgPtr;      /* pointer to sparse matrix element at
			     (gate node,gate node) */
  double *HSM1SsPtr;      /* pointer to sparse matrix element at
			     (source node,source node) */
  double *HSM1BbPtr;      /* pointer to sparse matrix element at
			     (bulk node,bulk node) */
  double *HSM1DPdpPtr;    /* pointer to sparse matrix element at
			     (drain prime node,drain prime node) */
  double *HSM1SPspPtr;    /* pointer to sparse matrix element at
			     (source prime node,source prime node) */
  double *HSM1DdpPtr;     /* pointer to sparse matrix element at
			     (drain node,drain prime node) */
  double *HSM1GbPtr;      /* pointer to sparse matrix element at
			     (gate node,bulk node) */
  double *HSM1GdpPtr;     /* pointer to sparse matrix element at
			     (gate node,drain prime node) */
  double *HSM1GspPtr;     /* pointer to sparse matrix element at
			     (gate node,source prime node) */
  double *HSM1SspPtr;     /* pointer to sparse matrix element at
			     (source node,source prime node) */
  double *HSM1BdpPtr;     /* pointer to sparse matrix element at
			     (bulk node,drain prime node) */
  double *HSM1BspPtr;     /* pointer to sparse matrix element at
			     (bulk node,source prime node) */
  double *HSM1DPspPtr;    /* pointer to sparse matrix element at
			     (drain prime node,source prime node) */
  double *HSM1DPdPtr;     /* pointer to sparse matrix element at
			     (drain prime node,drain node) */
  double *HSM1BgPtr;      /* pointer to sparse matrix element at
			     (bulk node,gate node) */
  double *HSM1DPgPtr;     /* pointer to sparse matrix element at
			     (drain prime node,gate node) */
  double *HSM1SPgPtr;     /* pointer to sparse matrix element at
			     (source prime node,gate node) */
  double *HSM1SPsPtr;     /* pointer to sparse matrix element at
			     (source prime node,source node) */
  double *HSM1DPbPtr;     /* pointer to sparse matrix element at
			     (drain prime node,bulk node) */
  double *HSM1SPbPtr;     /* pointer to sparse matrix element at
			     (source prime node,bulk node) */
  double *HSM1SPdpPtr;    /* pointer to sparse matrix element at
			     (source prime node,drain prime node) */

  /* indices to the array of HiSIM1 NOISE SOURCES (the same as BSIM3) */
#define HSM1RDNOIZ       0
#define HSM1RSNOIZ       1
#define HSM1IDNOIZ       2
#define HSM1FLNOIZ       3
#define HSM1TOTNOIZ      4

#define HSM1NSRCS        5  /* the number of HiSIM1 MOSFET noise sources */

#ifndef NONOISE
  double HSM1nVar[NSTATVARS][HSM1NSRCS];
#else /* NONOISE */
  double **HSM1nVar;
#endif /* NONOISE */

};

  /* common state values in hisim1 module */
#define HSM1vbd GENstate + 0
#define HSM1vbs GENstate + 1
#define HSM1vgs GENstate + 2
#define HSM1vds GENstate + 3

#define HSM1qb  GENstate + 4
#define HSM1cqb GENstate + 5
#define HSM1qg  GENstate + 6
#define HSM1cqg GENstate + 7
#define HSM1qd  GENstate + 8
#define HSM1cqd GENstate + 9 

#define HSM1qbs GENstate + 10
#define HSM1qbd GENstate + 11

#define HSM1numStates 12

struct sHSM1model : sGENmodel
{
    sHSM1model()            { memset(this, 0, sizeof(sHSM1model)); }
    sHSM1model *next()      { return ((sHSM1model*)GENnextModel); }
    sHSM1instance *inst()   { return ((sHSM1instance*)GENinstances); }

  int HSM1_type;      		/* device type: 1 = nmos,  -1 = pmos */
  int HSM1_level;               /* level */
  int HSM1_info;                /* information */
  int HSM1_noise;               /* noise model selecter see hsm1noi.c */
  int HSM1_version;             /* model version 100/110 */
  int HSM1_show;                /* show physical value 1, 2, ... , 11 */

  /* flags for initial guess */
  int HSM1_corsrd ;
  int HSM1_coiprv ;
  int HSM1_copprv ;
  int HSM1_cocgso ;
  int HSM1_cocgdo ;
  int HSM1_cocgbo ;
  int HSM1_coadov ;
  int HSM1_coxx08 ;
  int HSM1_coxx09 ;
  int HSM1_coisub ;
  int HSM1_coiigs ;
  int HSM1_cogidl ;
  int HSM1_coovlp ;
  int HSM1_conois ;
  int HSM1_coisti ; /* HiSIM1.1 */
  /* HiSIM original */
  double HSM1_vmax ;
  double HSM1_bgtmp1 ;
  double HSM1_bgtmp2 ;
  double HSM1_tox ;
  double HSM1_dl ;
  double HSM1_dw ;
  double HSM1_xj ;   /* HiSIM1.0 */
  double HSM1_xqy ;  /* HiSIM1.1 */
  double HSM1_rs;     /* source contact resistance */
  double HSM1_rd;     /* drain contact resistance */
  double HSM1_vfbc ;
  double HSM1_nsubc ;
  double HSM1_parl1 ;
  double HSM1_parl2 ;
  double HSM1_lp ;
  double HSM1_nsubp ;
  double HSM1_scp1 ;
  double HSM1_scp2 ;
  double HSM1_scp3 ;
  double HSM1_sc1 ;
  double HSM1_sc2 ;
  double HSM1_sc3 ;
  double HSM1_pgd1 ;
  double HSM1_pgd2 ;
  double HSM1_pgd3 ;
  double HSM1_ndep ;
  double HSM1_ninv ;
  double HSM1_ninvd ;
  double HSM1_muecb0 ;
  double HSM1_muecb1 ;
  double HSM1_mueph1 ;
  double HSM1_mueph0 ;
  double HSM1_mueph2 ;
  double HSM1_w0 ;
  double HSM1_muesr1 ;
  double HSM1_muesr0 ;
  double HSM1_bb ;
  double HSM1_vds0 ;
  double HSM1_bc0 ;
  double HSM1_bc1 ;
  double HSM1_sub1 ;
  double HSM1_sub2 ;
  double HSM1_sub3 ;
  double HSM1_wvthsc ; /* HiSIM1.1 */
  double HSM1_nsti ;   /* HiSIM1.1 */
  double HSM1_wsti ;   /* HiSIM1.1 */
  double HSM1_cgso ;
  double HSM1_cgdo ;
  double HSM1_cgbo ;
  double HSM1_tpoly ;
  double HSM1_js0 ;
  double HSM1_js0sw ;
  double HSM1_nj ;
  double HSM1_njsw ;
  double HSM1_xti ;
  double HSM1_cj ;
  double HSM1_cjsw ;
  double HSM1_cjswg ;
  double HSM1_mj ;
  double HSM1_mjsw ;
  double HSM1_mjswg ;
  double HSM1_pb ;
  double HSM1_pbsw ;
  double HSM1_pbswg ;
  double HSM1_xpolyd ;
  double HSM1_clm1 ;
  double HSM1_clm2 ;
  double HSM1_clm3 ;
  double HSM1_muetmp ;
  double HSM1_rpock1 ;
  double HSM1_rpock2 ;
  double HSM1_rpocp1 ; /* HiSIM 1.1 */
  double HSM1_rpocp2 ; /* HiSIM 1.1 */
  double HSM1_vover ;
  double HSM1_voverp ;
  double HSM1_wfc ;
  double HSM1_qme1 ;
  double HSM1_qme2 ;
  double HSM1_qme3 ;
  double HSM1_gidl1 ;
  double HSM1_gidl2 ;
  double HSM1_gidl3 ;
  double HSM1_gleak1 ;
  double HSM1_gleak2 ;
  double HSM1_gleak3 ;
  double HSM1_vzadd0 ;
  double HSM1_pzadd0 ;
  double HSM1_nftrp ;
  double HSM1_nfalp ;
  double HSM1_cit ;

  /* for flicker noise of SPICE3 added by K.M. */
  double HSM1_ef;
  double HSM1_af;
  double HSM1_kf;

  /* flag for model */
  unsigned HSM1_type_Given  :1;
  unsigned HSM1_level_Given  :1;
  unsigned HSM1_info_Given  :1;
  unsigned HSM1_noise_Given :1;
  unsigned HSM1_version_Given :1;
  unsigned HSM1_show_Given :1;
  unsigned HSM1_corsrd_Given  :1;
  unsigned HSM1_coiprv_Given  :1;
  unsigned HSM1_copprv_Given  :1;
  unsigned HSM1_cocgso_Given  :1;
  unsigned HSM1_cocgdo_Given  :1;
  unsigned HSM1_cocgbo_Given  :1;
  unsigned HSM1_coadov_Given  :1;
  unsigned HSM1_coxx08_Given  :1;
  unsigned HSM1_coxx09_Given  :1;
  unsigned HSM1_coisub_Given  :1;
  unsigned HSM1_coiigs_Given  :1;
  unsigned HSM1_cogidl_Given  :1;
  unsigned HSM1_coovlp_Given  :1;
  unsigned HSM1_conois_Given  :1;
  unsigned HSM1_coisti_Given  :1; /* HiSIM1.1 */
  unsigned HSM1_vmax_Given  :1;
  unsigned HSM1_bgtmp1_Given  :1;
  unsigned HSM1_bgtmp2_Given  :1;
  unsigned HSM1_tox_Given  :1;
  unsigned HSM1_dl_Given  :1;
  unsigned HSM1_dw_Given  :1; 
  unsigned HSM1_xj_Given  :1;    /* HiSIM1.0 */
  unsigned HSM1_xqy_Given  :1;   /* HiSIM1.1 */
  unsigned HSM1_rs_Given  :1;
  unsigned HSM1_rd_Given  :1;
  unsigned HSM1_vfbc_Given  :1;
  unsigned HSM1_nsubc_Given  :1;
  unsigned HSM1_parl1_Given  :1;
  unsigned HSM1_parl2_Given  :1;
  unsigned HSM1_lp_Given  :1;
  unsigned HSM1_nsubp_Given  :1;
  unsigned HSM1_scp1_Given  :1;
  unsigned HSM1_scp2_Given  :1;
  unsigned HSM1_scp3_Given  :1;
  unsigned HSM1_sc1_Given  :1;
  unsigned HSM1_sc2_Given  :1;
  unsigned HSM1_sc3_Given  :1;
  unsigned HSM1_pgd1_Given  :1;
  unsigned HSM1_pgd2_Given  :1;
  unsigned HSM1_pgd3_Given  :1;
  unsigned HSM1_ndep_Given  :1;
  unsigned HSM1_ninv_Given  :1;
  unsigned HSM1_ninvd_Given  :1;
  unsigned HSM1_muecb0_Given  :1;
  unsigned HSM1_muecb1_Given  :1;
  unsigned HSM1_mueph1_Given  :1;
  unsigned HSM1_mueph0_Given  :1;
  unsigned HSM1_mueph2_Given  :1;
  unsigned HSM1_w0_Given  :1;
  unsigned HSM1_muesr1_Given  :1;
  unsigned HSM1_muesr0_Given  :1;
  unsigned HSM1_bb_Given  :1;
  unsigned HSM1_vds0_Given  :1;
  unsigned HSM1_bc0_Given  :1;
  unsigned HSM1_bc1_Given  :1;
  unsigned HSM1_sub1_Given  :1;
  unsigned HSM1_sub2_Given  :1;
  unsigned HSM1_sub3_Given  :1;
  unsigned HSM1_wvthsc_Given  :1; /* HiSIM1.1 */
  unsigned HSM1_nsti_Given  :1;   /* HiSIM1.1 */
  unsigned HSM1_wsti_Given  :1;   /* HiSIM1.1 */
  unsigned HSM1_cgso_Given  :1;
  unsigned HSM1_cgdo_Given  :1;
  unsigned HSM1_cgbo_Given  :1;
  unsigned HSM1_tpoly_Given  :1;
  unsigned HSM1_js0_Given  :1;
  unsigned HSM1_js0sw_Given  :1;
  unsigned HSM1_nj_Given  :1;
  unsigned HSM1_njsw_Given  :1;  
  unsigned HSM1_xti_Given  :1;
  unsigned HSM1_cj_Given  :1;
  unsigned HSM1_cjsw_Given  :1;
  unsigned HSM1_cjswg_Given  :1;
  unsigned HSM1_mj_Given  :1;
  unsigned HSM1_mjsw_Given  :1;
  unsigned HSM1_mjswg_Given  :1;
  unsigned HSM1_pb_Given  :1;
  unsigned HSM1_pbsw_Given  :1;
  unsigned HSM1_pbswg_Given  :1;
  unsigned HSM1_xpolyd_Given  :1;
  unsigned HSM1_clm1_Given  :1;
  unsigned HSM1_clm2_Given  :1;
  unsigned HSM1_clm3_Given  :1;
  unsigned HSM1_muetmp_Given  :1;
  unsigned HSM1_rpock1_Given  :1;
  unsigned HSM1_rpock2_Given  :1;
  unsigned HSM1_rpocp1_Given  :1; /* HiSIM1.1 */
  unsigned HSM1_rpocp2_Given  :1; /* HiSIM1.1 */
  unsigned HSM1_vover_Given  :1;
  unsigned HSM1_voverp_Given  :1;
  unsigned HSM1_wfc_Given  :1;
  unsigned HSM1_qme1_Given  :1;
  unsigned HSM1_qme2_Given  :1;
  unsigned HSM1_qme3_Given  :1;
  unsigned HSM1_gidl1_Given  :1;
  unsigned HSM1_gidl2_Given  :1;
  unsigned HSM1_gidl3_Given  :1;
  unsigned HSM1_gleak1_Given  :1;
  unsigned HSM1_gleak2_Given  :1;
  unsigned HSM1_gleak3_Given  :1;
  unsigned HSM1_vzadd0_Given  :1;
  unsigned HSM1_pzadd0_Given  :1;
  unsigned HSM1_nftrp_Given  :1;
  unsigned HSM1_nfalp_Given  :1;
  unsigned HSM1_cit_Given  :1;

  unsigned HSM1_ef_Given :1;
  unsigned HSM1_af_Given :1;
  unsigned HSM1_kf_Given :1;
};

} // namespace HSM110
using namespace HSM110;


#ifndef NMOS
#define NMOS 1
#define PMOS -1
#endif

#define HSM1_BAD_PARAM -1

// device parameters
enum {
    HSM1_L = 1,
    HSM1_W,
    HSM1_AD,
    HSM1_AS,
    HSM1_PD,
    HSM1_PS,
    HSM1_NRD,
    HSM1_NRS,
    HSM1_TEMP,
    HSM1_DTEMP,
    HSM1_OFF,
    HSM1_IC_VBS,
    HSM1_IC_VDS,
    HSM1_IC_VGS,
    HSM1_IC,

    HSM1_DNODE,
    HSM1_GNODE,
    HSM1_SNODE,
    HSM1_BNODE,
    HSM1_DNODEPRIME,
    HSM1_SNODEPRIME,
    HSM1_VBD,
    HSM1_VBS,
    HSM1_VGS,
    HSM1_VDS,
    HSM1_CD,
    HSM1_CBS,
    HSM1_CBD,
    HSM1_GM,
    HSM1_GDS,
    HSM1_GMBS,
    HSM1_GBD,
    HSM1_GBS,
    HSM1_QB,
    HSM1_CQB,
    HSM1_QG,
    HSM1_CQG,
    HSM1_QD,
    HSM1_CQD,
    HSM1_CGG,
    HSM1_CGD,
    HSM1_CGS,
    HSM1_CBG,
    HSM1_CAPBD,
    HSM1_CQBD,
    HSM1_CAPBS,
    HSM1_CQBS,
    HSM1_CDG,
    HSM1_CDD,
    HSM1_CDS,
    HSM1_VON,
    HSM1_VDSAT,
    HSM1_QBS,
    HSM1_QBD,
    HSM1_SOURCECONDUCT,
    HSM1_DRAINCONDUCT,
    HSM1_CBDB,
    HSM1_CBSB,

    // SRW - added
    HSM1_ID,
    HSM1_IS,
    HSM1_IG,
    HSM1_IB
};

// model parameters
enum {
    HSM1_MOD_VMAX = 1000,
    HSM1_MOD_BGTMP1,
    HSM1_MOD_BGTMP2,
    HSM1_MOD_TOX,
    HSM1_MOD_DL,
    HSM1_MOD_DW,
    HSM1_MOD_XJ,
    HSM1_MOD_XQY,
    HSM1_MOD_RS,
    HSM1_MOD_RD,
    HSM1_MOD_VFBC,
    HSM1_MOD_NSUBC,
    HSM1_MOD_PARL1,
    HSM1_MOD_PARL2,
    HSM1_MOD_SC1,
    HSM1_MOD_SC2,
    HSM1_MOD_SC3,
    HSM1_MOD_NDEP,
    HSM1_MOD_NINV,
    HSM1_MOD_MUECB0,
    HSM1_MOD_MUECB1,
    HSM1_MOD_MUEPH1,
    HSM1_MOD_MUEPH0,
    HSM1_MOD_MUEPH2,
    HSM1_MOD_W0,
    HSM1_MOD_MUESR1,
    HSM1_MOD_MUESR0,
    HSM1_MOD_BB,
    HSM1_MOD_VDS0,
    HSM1_MOD_BC0,
    HSM1_MOD_BC1,
    HSM1_MOD_SUB1,
    HSM1_MOD_SUB2,
    HSM1_MOD_SUB3,
    HSM1_MOD_CGSO,
    HSM1_MOD_CGDO,
    HSM1_MOD_CGBO,
    HSM1_MOD_JS0,
    HSM1_MOD_JS0SW,
    HSM1_MOD_NJ,
    HSM1_MOD_NJSW,
    HSM1_MOD_XTI,
    HSM1_MOD_CJ,
    HSM1_MOD_CJSW,
    HSM1_MOD_CJSWG,
    HSM1_MOD_MJ,
    HSM1_MOD_MJSW,
    HSM1_MOD_MJSWG,
    HSM1_MOD_PB,
    HSM1_MOD_PBSW,
    HSM1_MOD_PBSWG,
    HSM1_MOD_XPOLYD,
    HSM1_MOD_TPOLY,
    HSM1_MOD_LP,
    HSM1_MOD_NSUBP,
    HSM1_MOD_SCP1,
    HSM1_MOD_SCP2,
    HSM1_MOD_SCP3,
    HSM1_MOD_PGD1,
    HSM1_MOD_PGD2,
    HSM1_MOD_PGD3,
    HSM1_MOD_CLM1,
    HSM1_MOD_CLM2,
    HSM1_MOD_CLM3,
    HSM1_MOD_NINVD,
    HSM1_MOD_MUETMP,
    HSM1_MOD_RPOCK1,
    HSM1_MOD_RPOCK2,
    HSM1_MOD_VOVER,
    HSM1_MOD_VOVERP,
    HSM1_MOD_WFC,
    HSM1_MOD_QME1,
    HSM1_MOD_QME2,
    HSM1_MOD_QME3,
    HSM1_MOD_GIDL1,
    HSM1_MOD_GIDL2,
    HSM1_MOD_GIDL3,
    HSM1_MOD_GLEAK1,
    HSM1_MOD_GLEAK2,
    HSM1_MOD_GLEAK3,
    HSM1_MOD_VZADD0,
    HSM1_MOD_PZADD0,
    HSM1_MOD_WVTHSC,
    HSM1_MOD_NSTI,
    HSM1_MOD_WSTI,
    HSM1_MOD_RPOCP1,
    HSM1_MOD_RPOCP2,
    HSM1_MOD_NFTRP,
    HSM1_MOD_NFALP,
    HSM1_MOD_CIT,
    HSM1_MOD_EF,
    HSM1_MOD_AF,
    HSM1_MOD_KF,

    // flags
    HSM1_MOD_NMOS,
    HSM1_MOD_PMOS,
    HSM1_MOD_LEVEL,
    HSM1_MOD_INFO,
    HSM1_MOD_NOISE,
    HSM1_MOD_VERSION,
    HSM1_MOD_SHOW,
    HSM1_MOD_CORSRD,
    HSM1_MOD_COIPRV,
    HSM1_MOD_COPPRV,
    HSM1_MOD_COCGSO,
    HSM1_MOD_COCGDO,
    HSM1_MOD_COCGBO,
    HSM1_MOD_COADOV,
    HSM1_MOD_COXX08,
    HSM1_MOD_COXX09,
    HSM1_MOD_COISUB,
    HSM1_MOD_COIIGS,
    HSM1_MOD_COGIDL,
    HSM1_MOD_COOVLP,
    HSM1_MOD_CONOIS,
    HSM1_MOD_COISTI
};

#endif // HSM1DEFS_H

