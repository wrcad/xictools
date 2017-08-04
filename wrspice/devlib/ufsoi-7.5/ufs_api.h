
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

/*****************************************************************************
Copyright 1997: UNIVERSITY OF FLORIDA;  ALL RIGHTS RESERVED.
Authors: Min-Chie Jeng and UF SOI Group 
         (Code evolved from UFSOI FD and NFD model routines in SOISPICE-4.41.)
File: ufs_api.h
*****************************************************************************/

#define  ELECTRON_CHARGE        1.6021918e-19 
#define  BOLTZMANN              1.3806226e-23 
#define  VACUUM_PERMITTIVITY    (8.8541879239442e-12)
#define  SILICON_PERMITTIVITY   (11.7*VACUUM_PERMITTIVITY)
#define  OXIDE_PERMITTIVITY     (3.9*VACUUM_PERMITTIVITY)
#define  Boltz 1.3806226e-23
#define  ELECTRON_REST_MASS     0.91095e-30                        /* 7.0Y */
#define  PLANCK_CONSTANT        6.62617e-34                        /* 7.0Y */
#define  REDUCED_PLANCK         1.05458e-34                        /* 7.0Y */
#define  BARRIER_CBE            (3.1*ELECTRON_CHARGE)              /* 7.0Y */
#define  EFFECTIVE_DOS_CB       3.2e25                             /* 7.0Y */
#define  EFFECTIVE_DOS_VB       1.8e25                             /* 7.0Y */
#define  LEMIN                  0.543e-9                           /* 6.1 */
#define  KoverQ                  (BOLTZMANN / ELECTRON_CHARGE)
#define  MAX_EXP 1.446257064e12
#define  MIN_EXP 6.914400107e-13
#define  EXP_THRESHOLD 28.0
#define  MAX_EXP1 1.202604284e6
#define  MIN_EXP1 8.315287191e-7
#define  EXP_THRESHOLD1 14.0
#define  MAX(a,b)       ((a) > (b) ? (a) : (b))
#define  MIN(a,b)       ((a) < (b) ? (a) : (b))

#define NO	0
#define YES	1
#define DeltaV  1.0e-4                                                             /* 4.5F */
#define DeltaV1 1.0e-4

#ifndef NMOS
#define NMOS 1
#define PMOS -1
#endif /*NMOS*/

/* Begin data structure */
struct  ufsAPI_InstData {
    double			 Length;
    double			 Width;
    double			 DrainArea;
    double			 SourceArea;
    double			 DrainJunctionPerimeter;                           /* 4.5 */
    double			 SourceJunctionPerimeter;                          /* 4.5 */
    double			 BodyArea;
    double			 Nrs;
    double			 Nrd;
    double			 Nrb;
    double			 MFinger;
    double			 Rth;
    double			 Cth;
    double			 Leff;
    double			 Weff;
    double			 Expf;
    double			 Expb;
    double			 Nblavg;                                           /* 4.5r */
    double			 Uo;                                               /* 4.5r */
    double			 Beta0;                                            /* 4.5r */
    double			 Fsat;                                             /* 4.5r */
    double			 Phib;                                             /* 4.5r */
    double			 Vbi;                                              /* 4.5r */
    double			 Wkf;                                              /* 4.5r */
    double			 Vfbf;                                             /* 4.5r */
    double			 Qb;                                               /* 4.5r */
    double			 Mubody;                                           /* 4.5r */
    double			 Taug;                                             /* 4.5r */
    double			 Const4;                                           /* 4.5r */
    double			 Ft1;                                              /* 4.5r */
    double			 Vthf;						   /* 4.51 */
    double			 Vthb;						   /* 4.51 */

    int				 Mode;

    double			 Temperature;
    double			 SourceConductance;
    double			 DrainConductance;
    double			 BodyConductance;

    struct ufsTDModelData	*pTempModel;
    struct ufsDebugData         *pDebug;

    unsigned			 LengthGiven : 1;
    unsigned			 WidthGiven : 1;
    unsigned			 DrainAreaGiven : 1;
    unsigned			 SourceAreaGiven : 1;
    unsigned			 DrainJunctionPerimeterGiven : 1;                  /* 4.5 */
    unsigned			 SourceJunctionPerimeterGiven : 1;                 /* 4.5 */
    unsigned			 BodyAreaGiven : 1;
    unsigned			 NrsGiven : 1;
    unsigned			 NrdGiven : 1;
    unsigned			 NrbGiven : 1;
    unsigned			 MFingerGiven : 1;
    unsigned			 RthGiven : 1;
    unsigned			 CthGiven : 1;
    unsigned			 BulkContact : 1;
};

/*
 * Begin temperature dependent model parameter data structures.
 */

struct  ufsTDModelData {

    int				 InitNeeded;

    double			 Vtm;
    double			 Eg;                                               /* 4.41 */
    double			 Qr;
    double			 Xnin;
    double			 Phib;
    double			 Phibb;
    double			 Pst;
    double			 Qst;
    double			 Vbi;
    double			 Vbih;
    double			 Jro;
    double			 Alpha;
    double			 Beta;
    double			 Wkf;
    double			 Wkb;
    double			 Vfbf;
    double			 Vfbb;
    double			 Tauo;
    double			 Taug;
    double			 Taugh;
    double			 Dbl;
    double			 Uo;
    double			 Mubody;
    double			 Mubh;
    double			 Mumaj;
    double			 Mumin;
    double			 Beta0;
    double			 Vsat;
    double			 Fsat;
    double			 Rldd;
    double			 GdsTempFac;
    double			 GdsTempFac2;                                      /* 4.5 */
    double			 GbTempFac;
    double			 Seff;
    double			 D0;
    double			 D0h;
    double			 Leminh;
    double			 Ndseff;
    double			 Efs;
    double			 Efd;
    double			 Vexpl1;
    double			 Gexpl1;
    double			 Ioffset1;
    double			 Vexpl2;
    double			 Gexpl2;
    double			 Ioffset2;
    double			 Vexpl3;
    double			 Gexpl3;
    double			 Ioffset3;
    double			 Vexpl4;                                           /* 4.5 */
    double			 Gexpl4;                                           /* 4.5 */
    double			 Ioffset4;                                         /* 4.5 */
};

/*
 * Operating point data structure
 */

struct ufsAPI_OPData {
    int 			 Reversed;
    double			 Vbs;
    double			 Vbd;
    double			 Vgfs;
    double			 Vgfd;
    double			 Vgbs;
    double			 Vds;
    double			 SourceConductance;
    double			 DrainConductance;
    double			 BodyConductance;

    double			 Ich;
    double			 Ibjt;
    double			 dId_dVgf;
    double			 dId_dVd;
    double			 dId_dVgb;
    double			 dId_dVb;

    double			 Ir;
    double			 dIr_dVb;

    double			 Igt;
    double			 dIgt_dVgf;
    double			 dIgt_dVd;
    double			 dIgt_dVgb;
    double			 dIgt_dVb;

    double			 Igi;
    double			 dIgi_dVgf;
    double			 dIgi_dVd;
    double			 dIgi_dVgb;
    double			 dIgi_dVb;

    double                       Igb;                                         /* 7.0Y */
    double                       dIgb_dVgf;                                   /* 7.0Y */
    double                       dIgb_dVd;                                    /* 7.0Y */
    double                       dIgb_dVgb;                                   /* 7.0Y */
    double                       dIgb_dVb;                                    /* 7.0Y */

    double			 Qgf;
    double			 dQgf_dVgf;
    double			 dQgf_dVd;
    double			 dQgf_dVgb;
    double			 dQgf_dVb;

    double			 Qd;
    double			 dQd_dVgf;
    double			 dQd_dVd;
    double			 dQd_dVgb;
    double			 dQd_dVb;

    double			 Qgb;
    double			 dQgb_dVgf;
    double			 dQgb_dVd;
    double			 dQgb_dVgb;
    double			 dQgb_dVb;

    double			 Qb;
    double			 dQb_dVgf;
    double			 dQb_dVd;
    double			 dQb_dVgb;
    double			 dQb_dVb;

    double			 Qs;
    double			 Qn;

    double			 Vtw;
    double			 Vts;
    double			 Vdsat;
    double			 Le;
    double			 Ueff;                                             /* 4.5 */
    double			 Power;
    double                       Sich;                                             /* 5.0 */

    double			 dId_dT;
    double			 dIr_dT;
    double			 dIgt_dT;
    double			 dIgi_dT;
    double                       dIgb_dT;                                         /* 7.0Y */
    double			 dQd_dT;
    double			 dQb_dT;
    double			 dQgf_dT;
    double			 dQgb_dT;
    double			 dP_dT;
    double			 dP_dVgf;
    double			 dP_dVd;
    double			 dP_dVgb;
    double			 dP_dVb;
};

/*
 * Begin model data structures.
 */

struct  ufsAPI_ModelData {
/*
 * User input parameters.
 */
    int			 	 Type;
    int				 Debug;
    int				 ParamCheck;
    int				 Selft;
    int			 	 NfdMod;
    int			 	 Tpg;
    int			 	 Tps;

    double			 Vfbf;
    double			 Vfbb;
    double			 Wkf;
    double			 Wkb;
    double			 Nqff;
    double			 Nqfb;
    double			 Nsf;
    double			 Nsb;
    double			 Toxf;
    double			 Toxb;
    double			 Nsub;
    double			 Ngate;
    double		 	 Nds;
    double		 	 Tb;
    double		 	 Nbody;
    double		 	 Lldd;
    double		 	 Nldd;                                             /* 4.5 */
    double		 	 Uo;
    double		 	 Theta;
    double		 	 Bfact;
    double		 	 Vsat;
    double		 	 Alpha;
    double		 	 Beta;
    double		 	 Gamma;
    double		 	 Kappa;
    double		 	 Tauo;
    double		 	 Jro;
    double		 	 M;
    double		 	 Ldiff;
    double		 	 Seff;
    double		 	 Fvbjt;
    double		 	 Tnom;
    double		 	 Cgfdo;
    double		 	 Cgfso;
    double		 	 Cgfbo;
    double		 	 Rhosd;
    double		 	 Rhob;
    double		 	 Rd;
    double		 	 Rs;
    double		 	 Rbody;
    double		 	 Dl;
    double		 	 Dw;
    double		 	 Fnk;
    double		 	 Fna;
    double		 	 Tf;
    double		 	 Thalo;
    double		 	 Nbl;
    double		 	 Nbh;
    double		 	 Nhalo;
    double		 	 Tmax;
    double		 	 Imax;
    double		 	 Bgidl;                                            /* 4.41 */
    double		 	 Ntr;                                              /* 4.5 */
    double		 	 Bjt;                                              /* 4.5 */
    double		 	 Lrsce;                                            /* 4.5r */
    double		 	 Nqfsw;                                            /* 4.5r */
    double		 	 Qm;                                               /* 4.5qm */
    double		 	 Vo;                                               /* 5.0vo */
    double                       Mox;                                              /* 7.0Y */
    double                       Svbe;                                             /* 7.0Y */
    double                       Scbe;                                             /* 7.0Y */
    double                       Kd;                                               /* 7.0Y */
    double                       Gex;                                              /* 7.5W */
    double                       Sfact;                                            /* 7.0Y */
    double                       Ffact;                                            /* 7.0Y */


/*
 * Calculated data.
 */
    double			 Coxf;
    double			 Coxb;
    double			 Cb;
    double			 Rf;
    double			 Rb;
    double			 Xalpha;
    double			 Xbeta;
    double			 Qb;
    double			 Factor1;
    double			 C;
    double			 A;
    double			 B;
    double			 Aa;
    double			 Ccb;
    double			 Ab;
    double			 Aab;
    double			 Bb;
    double			 Vbi;
    double			 Phib;
    double			 Phibb;
    double			 Xnin;
    double			 Const1;
    double			 Const2;
    double			 Const3;
    double			 Const4;
    double			 Const5;
    double			 Dum1;
    double			 Ft1;
    double			 Ft1h;
    double			 Mubody;
    double			 Mubh;
    double			 Mumaj;
    double			 Mumin;
    double			 Xalphab;
    double			 Lc;
    double			 Lcb;
    double			 Nbheff;
    double			 Teff;                                             /* 4.5 */

/* Flags */
    unsigned		 	 TypeGiven:1;
    unsigned			 DebugGiven:1;
    unsigned			 ParamCheckGiven:1;
    unsigned			 SelftGiven:1;
    unsigned		 	 NfdModGiven:1;

    unsigned			 VfbfGiven:1;
    unsigned			 VfbbGiven:1;
    unsigned			 WkfGiven:1;
    unsigned			 WkbGiven:1;
    unsigned			 NqffGiven:1;
    unsigned			 NqfbGiven:1;
    unsigned			 NsfGiven:1;
    unsigned			 NsbGiven:1;
    unsigned			 ToxfGiven:1;
    unsigned			 ToxbGiven:1;
    unsigned			 NsubGiven:1;
    unsigned			 NgateGiven:1;
    unsigned		 	 TpgGiven:1;
    unsigned		 	 TpsGiven:1;
    unsigned		 	 NdsGiven:1;
    unsigned		 	 TbGiven:1;
    unsigned		 	 NbodyGiven:1;
    unsigned		 	 LlddGiven:1;
    unsigned		 	 NlddGiven:1;                                      /* 4.5 */
    unsigned		 	 UoGiven:1;
    unsigned		 	 ThetaGiven:1;
    unsigned		 	 BfactGiven:1;
    unsigned		 	 VsatGiven:1;
    unsigned		 	 AlphaGiven:1;
    unsigned		 	 BetaGiven:1;
    unsigned		 	 GammaGiven:1;
    unsigned		 	 KappaGiven:1;
    unsigned		 	 TauoGiven:1;
    unsigned		 	 JroGiven:1;
    unsigned		 	 MGiven:1;
    unsigned		 	 LdiffGiven:1;
    unsigned		 	 SeffGiven:1;
    unsigned		 	 FvbjtGiven:1;
    unsigned		 	 TnomGiven:1;
    unsigned		 	 CgfdoGiven:1;
    unsigned		 	 CgfsoGiven:1;
    unsigned		 	 CgfboGiven:1;
    unsigned		 	 RhosdGiven:1;
    unsigned		 	 RhobGiven:1;
    unsigned		 	 RdGiven:1;
    unsigned		 	 RsGiven:1;
    unsigned		 	 RbodyGiven:1;
    unsigned		 	 DlGiven:1;
    unsigned		 	 DwGiven:1;
    unsigned		 	 FnkGiven:1;
    unsigned		 	 FnaGiven:1;
    unsigned		 	 TfGiven:1;
    unsigned		 	 ThaloGiven:1;
    unsigned		 	 NblGiven:1;
    unsigned		 	 NbhGiven:1;
    unsigned		 	 NhaloGiven:1;
    unsigned		 	 TmaxGiven:1;
    unsigned		 	 ImaxGiven:1;
    unsigned		 	 BgidlGiven:1;                                     /* 4.41 */
    unsigned		 	 NtrGiven:1;                                       /* 4.5 */
    unsigned		 	 BjtGiven:1;                                       /* 4.5 */
    unsigned		 	 LrsceGiven:1;                                     /* 4.5r */
    unsigned		 	 NqfswGiven:1;                                     /* 4.5r */
    unsigned		 	 QmGiven:1;                                        /* 4.5qm */
    unsigned		 	 VoGiven:1;                                        /* 5.0vo */
    unsigned                     MoxGiven:1;                                       /* 7.0Y */
    unsigned                     SvbeGiven:1;                                      /* 7.0Y */
    unsigned                     ScbeGiven:1;                                      /* 7.0Y */
    unsigned                     KdGiven:1;                                        /* 7.0Y */
    unsigned                     GexGiven:1;                                       /* 7.5W */
    unsigned                     SfactGiven:1;                                     /* 7.0Y */
    unsigned                     FfactGiven:1;                                     /* 7.0Y */



};

struct  ufsAPI_EnvData {
    double			 Temperature;
    double			 Tnom;
    double			 Gmin;
};

struct  ufsDebugData {
    double			 Weff;
};

#define ALLOC(type, number)     \
 ((type *)malloc((unsigned) (sizeof(type)*(number))))


/* device parameters */
#define UFS_W 1
#define UFS_L 2
#define UFS_M 3
#define UFS_AS 4
#define UFS_AD 5
#define UFS_AB 6
#define UFS_NRS 7
#define UFS_NRD 8
#define UFS_NRB 9
#define UFS_RTH 10
#define UFS_CTH 11
#define UFS_PSJ 12                                                                 /* 4.5 */
#define UFS_OFF 13
#define UFS_IC_VBS 14
#define UFS_IC_VDS 15
#define UFS_IC_VGFS 16
#define UFS_IC_VGBS 17
#define UFS_IC 18
#define UFS_PDJ 19                                                                 /* 4.5 */

/* model parameters */
#define UFS_MOD_PARAMCHK 101
#define UFS_MOD_DEBUG 102
#define UFS_MOD_SELFT 103
#define UFS_MOD_BODY 104

#define UFS_MOD_VFBF 111
#define UFS_MOD_VFBB 112
#define UFS_MOD_WKF 113
#define UFS_MOD_WKB 114
#define UFS_MOD_NQFF 115
#define UFS_MOD_NQFB 116
#define UFS_MOD_NSF 117
#define UFS_MOD_NSB 118
#define UFS_MOD_TOXF  119
#define UFS_MOD_TOXB 120
#define UFS_MOD_NSUB 121
#define UFS_MOD_NGATE 122
#define UFS_MOD_TPG 123
#define UFS_MOD_TPS 124
#define UFS_MOD_NMOS 125
#define UFS_MOD_PMOS 126
#define UFS_MOD_NDS 127
#define UFS_MOD_TB 128
#define UFS_MOD_NBODY 129
#define UFS_MOD_NTR 130                                                            /* 4.5 */
#define UFS_MOD_LLDD   131
#define UFS_MOD_NLDD  132                                                          /* 4.5 */
#define UFS_MOD_UO   133
#define UFS_MOD_U0   133
#define UFS_MOD_THETA  134
#define UFS_MOD_BFACT  135
#define UFS_MOD_VSAT 136
#define UFS_MOD_BT    137                                                          /* 4.5 */
#define UFS_MOD_BJT   138                                                          /* 4.5 */
#define UFS_MOD_LRSCE  139                                                         /* 4.5r */
#define UFS_MOD_ALPHA  140
#define UFS_MOD_BETA   141
#define UFS_MOD_GAMMA  142
#define UFS_MOD_KAPPA  143
#define UFS_MOD_TAUO 144
#define UFS_MOD_JRO 145
#define UFS_MOD_M  146
#define UFS_MOD_LDIFF  147
#define UFS_MOD_SEFF   148
#define UFS_MOD_FVBJT   149
#define UFS_MOD_TNOM   150
#define UFS_MOD_CGFDO   151
#define UFS_MOD_CGFSO   152
#define UFS_MOD_CGFBO   153
#define UFS_MOD_RHOSD   154
#define UFS_MOD_RHOB   155
#define UFS_MOD_RD   156
#define UFS_MOD_RS   157
#define UFS_MOD_RB   158
#define UFS_MOD_DL 159
#define UFS_MOD_DW 160
#define UFS_MOD_FNK 161
#define UFS_MOD_FNA 162
#define UFS_MOD_TF 163
#define UFS_MOD_THALO 164
#define UFS_MOD_NBL 165
#define UFS_MOD_NBH 166
#define UFS_MOD_NHALO 167
#define UFS_MOD_TMAX 168
#define UFS_MOD_IMAX 169
#define UFS_MOD_BGIDL 170                                                          /* 4.41 */
#define UFS_MOD_NQFSW 171                                                          /* 4.5r */
#define UFS_MOD_QM 172                                                             /* 4.5qm */
#define UFS_MOD_VO 173                                                             /* 5.0vo */
#define UFS_MOD_MOX 174                                                            /* 7.0Y */
#define UFS_MOD_SVBE 175                                                           /* 7.0Y */
#define UFS_MOD_KD 176                                                             /* 7.0Y */
#define UFS_MOD_GEX 177                                                            /* 7.5W */
#define UFS_MOD_SFACT 178                                                          /* 7.0Y */
#define UFS_MOD_SCBE 179                                                           /* 7.0Y */
#define UFS_MOD_FFACT 180                                                          /* 7.0Y */


/* device questions */
#define UFS_DNODE    701
#define UFS_GNODE    702
#define UFS_SNODE    703
#define UFS_BNODE    704
#define UFS_BGNODE   705
#define UFS_TNODE    706
#define UFS_DNODEPRIME 707
#define UFS_SNODEPRIME 708
#define UFS_BNODEPRIME 709

#define UFS_ICH      710
#define UFS_IBJT     711
#define UFS_IR       712
#define UFS_IGT      713
#define UFS_IGI      714
#define UFS_VBS      715
#define UFS_VBD      716
#define UFS_VGFS     717
#define UFS_VGFD     718
#define UFS_VDS      719
#define UFS_GDGF     720
#define UFS_GDD      721
#define UFS_GDGB     722
#define UFS_GDB      723
#define UFS_GDS      724
#define UFS_GRB      725
#define UFS_GRS      726
#define UFS_GGTGF    727
#define UFS_GGTD     728
#define UFS_GGTGB    729
#define UFS_GGTB     730
#define UFS_GGTS     731
#define UFS_GGIGF    732
#define UFS_GGID     733
#define UFS_GGIGB    734
#define UFS_GGIB     735
#define UFS_GGIS     736

#define UFS_QGF      737
#define UFS_QD       738
#define UFS_QGB      739
#define UFS_QB       740
#define UFS_QS       741

#define UFS_VTW      742
#define UFS_VTS      743
#define UFS_VDSAT    744

#define UFS_CGFGF    745
#define UFS_CGFD     746
#define UFS_CGFGB    747
#define UFS_CGFB     748
#define UFS_CGFS     749
#define UFS_CDGF     750
#define UFS_CDD      751
#define UFS_CDGB     752
#define UFS_CDB      753
#define UFS_CDS      754
#define UFS_CGBGF    755
#define UFS_CGBD     756
#define UFS_CGBGB    757
#define UFS_CGBB     758
#define UFS_CGBS     759
#define UFS_CBGF     760
#define UFS_CBD      761
#define UFS_CBGB     762
#define UFS_CBB      763
#define UFS_CBS      764
#define UFS_CSGF     765
#define UFS_CSD      766
#define UFS_CSGB     767
#define UFS_CSB      768
#define UFS_CSS      769

#define UFS_LE       770
#define UFS_POWER    771
#define UFS_TEMP     772
#define UFS_VGBS     773

#define UFS_GDT     774
#define UFS_GRT     775
#define UFS_GGTT    776
#define UFS_GGIT    777

#define UFS_CGFT    778
#define UFS_CDT     779
#define UFS_CGBT    780
#define UFS_CBT     781
#define UFS_CST     782

#define UFS_GPT     783
#define UFS_GPGF    784
#define UFS_GPD     785
#define UFS_GPGB    786
#define UFS_GPB     787
#define UFS_GPS     788

#define UFS_RD      789
#define UFS_RS      790
#define UFS_RB      791

#define UFS_QN      792                                                            /* 4.5 */
#define UFS_UEFF    793                                                            /* 4.5 */
#define UFS_SICH    794                                                            /* 5.0 */
#define UFS_IGB     795                                                            /* 7.0Y */
#define UFS_GGBGF   796                                                            /* 7.0Y */
#define UFS_GGBD    797                                                            /* 7.0Y */
#define UFS_GGBGB   798                                                            /* 7.0Y */
#define UFS_GGBB    799                                                            /* 7.0Y */
#define UFS_GGBS    800                                                            /* 7.0Y */

#define UFS_GGBT    801                                                            /* 7.0Y */

#ifdef __STDC__
extern int fdEvalMod( double, double, double, double, struct ufsAPI_OPData*,
	  struct ufsAPI_InstData*, struct ufsAPI_ModelData*, 
	  struct ufsAPI_EnvData*, int, int );                                      /* 5.0 */
extern int fdEvalDeriv( double, double, double, double, struct ufsAPI_OPData*,
	  struct ufsAPI_InstData*, struct ufsAPI_ModelData*, 
	  struct ufsAPI_EnvData*, int, int );                                      /* 5.0 */
extern int nfdEvalMod( double, double, double, double, struct ufsAPI_OPData*,
	  struct ufsAPI_InstData*, struct ufsAPI_ModelData*, 
	  struct ufsAPI_EnvData*, int, int );                                      /* 7.0Y */
extern int nfdEvalDeriv( double, double, double, double, struct ufsAPI_OPData*,
	  struct ufsAPI_InstData*, struct ufsAPI_ModelData*, 
	  struct ufsAPI_EnvData*, int, int );                                      /* 4.5d */
extern int ufsTempEffect(struct ufsAPI_ModelData*, struct ufsAPI_InstData*,
	  struct ufsAPI_EnvData*, double, int );                                   /* 4.5 */
extern int ufsInitModel(struct ufsAPI_ModelData*, struct ufsAPI_EnvData*);
extern int ufsUpdatePhi(struct ufsAPI_ModelData*, struct ufsAPI_InstData*,
	  struct ufsAPI_EnvData*);
extern int ufsInitInst(struct ufsAPI_ModelData*, struct ufsAPI_InstData*,
	  struct ufsAPI_EnvData*);
extern int ufsInitInstFlag(struct ufsAPI_InstData*);
extern int ufsInitModelFlag(struct ufsAPI_ModelData*);
extern int ufsGetInstParam(struct ufsAPI_InstData *, int, double*);
extern int ufsGetOpParam(struct ufsAPI_OPData *, int, double*);
extern int ufsSetInstParam(struct ufsAPI_InstData *, int, double);
extern int ufsGetModelParam(struct ufsAPI_ModelData *, int, double*);
extern int ufsSetModelParam(struct ufsAPI_ModelData *, int, double);
extern int ufsDefaultInstParam(struct ufsAPI_ModelData *,
	  struct ufsAPI_InstData*, struct ufsAPI_EnvData*);
extern int ufsDefaultModelParam(struct ufsAPI_ModelData *,
	  struct ufsAPI_EnvData*);
extern double Xnonloct(double, double, double, double, double, double, double,
	  double, double, double, double, double);
extern double Xnonlocm(double, double, double, double, double, double, double,
	  double, double, double, double, double, double, double);
extern double ufsLimiting( struct ufsAPI_InstData *, double, double,
          double, int, double, double);
extern double XIntSich(double, double, double, double, double, double, double,     /* 5.0 */
          double, double);                                                         /* 5.0 */

#else /* stdc */
extern int fdEvalMod();
extern int fdEvalDeriv();
extern int nfdEvalMod();
extern int nfdEvalDeriv();
extern int ufsTempEffect();
extern int ufsInitModel();
extern int ufsInitInst();
extern int ufsUpdatePhi();
extern int ufsInitModelFlag();
extern int ufsInitInstFlag();
extern int ufsGetInstParam();
extern int ufsGetOpParam();
extern int ufsSetInstParam();
extern int ufsGetModelParam();
extern int ufsSetModelParam();
extern int ufsDefaultInstParam()
extern int ufsDefaultModelParam()
extern double Xnonloct();
extern double Xnonlocm();
extern double ufsLimiting();
extern double XIntSich();                                                          /* 5.0 */
#endif /* stdc */
