
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

/*
 * Author: 2000 Wladek Grabinski; EKV v2.6 Model Upgrade
 * Author: 1997 Eckhard Brass;    EKV v2.5 Model Implementation
 *     (C) 1990 Regents of the University of California. Spice3 Format
 */

#ifndef EKVDEFS_H
#define EKVDEFS_H

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

namespace EKV26 {

struct sEKVmodel;
struct sEKVinstance;

#define EKVstates GENstate

/* declarations for EKV V2.6 MOSFETs */
/*
 * wg 17-SEP-2K  common definitions for EKV v2.6 rev.XII   
 */
#define  epso           8.854214871e-12
#define  epssil         1.035943140e-10
#define  boltz          1.3806226e-23
#define  charge         1.6021918e-19

#define  konq           boltz/charge

#define  twopi          6.2831853072e0
#define  reftmp         300.15e0
#define  tnomref        273.15e0
#define  exp40          2.35385266837019946e17
#define  expm5          6.73794699909e-3
#define  two3rds        2.0/3.0
#define  four15th       4.0/15.0
#define  twentyfive16th 25.0/16.0

#define  c_a            0.28
#define  c_ee           1.936e-3

struct EKVdev : public IFdevice
{
    EKVdev();
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
//    int pzLoad(sGENmodel*, sCKT*, IFcomplex*); 
//    int disto(int, sGENmodel*, sCKT*);  
    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
};

struct sEKVinstance : public sGENinstance
{
    sEKVinstance()
        {
            memset(this, 0, sizeof(sEKVinstance));
            GENnumNodes = 4;
        }
    ~sEKVinstance() { delete [] (char*)EKVbacking; }
    sEKVinstance *next()
        { return (static_cast<sEKVinstance*>(GENnextInstance)); }

    int EKVdNode;  /* number of the gate node of the mosfet */
    int EKVgNode;  /* number of the gate node of the mosfet */
    int EKVsNode;  /* number of the source node of the mosfet */
    int EKVbNode;  /* number of the bulk node of the mosfet */
    int EKVdNodePrime; /* number of the internal drain node of the mosfet */
    int EKVsNodePrime; /* number of the internal source node of the mosfet */

    double EKVl;   /* the length of the channel region */
    double EKVw;   /* the width of the channel region */
    double EKVdrainArea;   /* the area of the drain diffusion */
    double EKVsourceArea;  /* the area of the source diffusion */
    double EKVdrainSquares;    /* the length of the drain in squares */
    double EKVsourceSquares;   /* the length of the source in squares */
    double EKVdrainPerimiter;
    double EKVsourcePerimiter;
    double EKVsourceConductance;   /*conductance of source(or 0):set in setup*/
    double EKVdrainConductance;    /*conductance of drain(or 0):set in setup*/
    double EKVtemp;                /* operating temperature of this instance */

    // This provides a means to back up and restore a known-good
    // state.
    void *EKVbacking;
    void backup(DEV_BKMODE m)
        {
            if (m == DEV_SAVE) {
                if (!EKVbacking)
                    EKVbacking = new char[sizeof(sEKVinstance)];
                memcpy(EKVbacking, this, sizeof(sEKVinstance));
            }
            else if (m == DEV_RESTORE) {
                if (EKVbacking)
                    memcpy(this, EKVbacking, sizeof(sEKVinstance));
            }
            else {
                // DEV_CLEAR
                delete [] (char*)EKVbacking;
                EKVbacking = 0;
            }
        }

    double EKVtkp;                 /* temperature corrected transconductance*/
    double EKVtPhi;                /* temperature corrected Phi */
    double EKVtVto;                /* temperature corrected Vto */
    double EKVtSatCur;             /* temperature corrected saturation Cur. */
    double EKVtSatCurDens; /* temperature corrected saturation Cur. density*/
    double EKVtCbd;                /* temperature corrected B-D Capacitance */
    double EKVtCbs;                /* temperature corrected B-S Capacitance */
    double EKVtCj;         /* temperature corrected Bulk bottom Capacitance */
    double EKVtCjsw;       /* temperature corrected Bulk side Capacitance */
    double EKVtBulkPot;    /* temperature corrected Bulk potential */
    double EKVtpbsw;       /* temperature corrected pbsw */
    double EKVtDepCap;     /* temperature adjusted transition point in */
    /* the cureve matching Fc * Vj */
    double EKVtucrit;      /* temperature adjusted ucrit */
    double EKVtibb;        /* temperature adjusted ibb */
    double EKVtrs;         /* temperature adjusted rs */
    double EKVtrd;         /* temperature adjusted rd */
    double EKVtrsh;        /* temperature adjusted rsh */
    double EKVtrsc;        /* temperature adjusted rsc */
    double EKVtrdc;        /* temperature adjusted rdc */
    double EKVtjsw;        /* temperature adjusted jsw */
    double EKVtaf;         /* temperature adjusted af  */

    double EKVicVBS;   /* initial condition B-S voltage */
    double EKVicVDS;   /* initial condition D-S voltage */
    double EKVicVGS;   /* initial condition G-S voltage */
    double EKVvon;
    double EKVvdsat;
    double EKVvgeff;
    double EKVvgprime;
    double EKVvgstar;
    double EKVsourceVcrit; /* Vcrit for pos. vds */
    double EKVdrainVcrit;  /* Vcrit for pos. vds */
    double EKVcd;
    double EKVisub;
    double EKVvp;
    double EKVslope;
    double EKVif;
    double EKVir;
    double EKVirprime;
    double EKVtau;
    double EKVcbs;
    double EKVcbd;
    double EKVgmbs;
    double EKVgm;
    double EKVgms;
    double EKVgds;
    double EKVgbd;
    double EKVgbs;
    double EKVcapbd;
    double EKVcapbs;
    double EKVCbd;
    double EKVCbdsw;
    double EKVCbs;
    double EKVCbssw;
    double EKVf2d;
    double EKVf3d;
    double EKVf4d;
    double EKVf2s;
    double EKVf3s;
    double EKVf4s;

#ifdef notdef
/*
 * naming convention:
 * x = vgs
 * y = vbs
 * z = vds
 * cdr = cdrain
 */

#define EKVNDCOEFFS 30

#ifndef NODISTO
    double EKVdCoeffs[EKVNDCOEFFS];
#else /* NODISTO */
    double *EKVdCoeffs;
#endif /* NODISTO */

#ifndef CONFIG

#define capbs2      EKVdCoeffs[0]
#define capbs3      EKVdCoeffs[1]
#define capbd2      EKVdCoeffs[2]
#define capbd3      EKVdCoeffs[3]
#define gbs2        EKVdCoeffs[4]
#define gbs3        EKVdCoeffs[5]
#define gbd2        EKVdCoeffs[6]
#define gbd3        EKVdCoeffs[7]
#define capgb2      EKVdCoeffs[8]
#define capgb3      EKVdCoeffs[9]
#define cdr_x2      EKVdCoeffs[10]
#define cdr_y2      EKVdCoeffs[11]
#define cdr_z2      EKVdCoeffs[12]
#define cdr_xy      EKVdCoeffs[13]
#define cdr_yz      EKVdCoeffs[14]
#define cdr_xz      EKVdCoeffs[15]
#define cdr_x3      EKVdCoeffs[16]
#define cdr_y3      EKVdCoeffs[17]
#define cdr_z3      EKVdCoeffs[18]
#define cdr_x2z     EKVdCoeffs[19]
#define cdr_x2y     EKVdCoeffs[20]
#define cdr_y2z     EKVdCoeffs[21]
#define cdr_xy2     EKVdCoeffs[22]
#define cdr_xz2     EKVdCoeffs[23]
#define cdr_yz2     EKVdCoeffs[24]
#define cdr_xyz     EKVdCoeffs[25]
#define capgs2      EKVdCoeffs[26]
#define capgs3      EKVdCoeffs[27]
#define capgd2      EKVdCoeffs[28]
#define capgd3      EKVdCoeffs[29]

#endif
#endif

#define EKVRDNOIZ   0
#define EKVRSNOIZ   1
#define EKVIDNOIZ   2
#define EKVFLNOIZ   3
#define EKVTOTNOIZ  4

#define EKVNSRCS    5     /* the number of EKVFET noise sources*/

#ifndef NONOISE
    double EKVnVar[NSTATVARS][EKVNSRCS];
#else /* NONOISE */
    double **EKVnVar;
#endif /* NONOISE */

    int EKVmode;       /* device mode : 1 = normal, -1 = inverse */


    unsigned EKVoff:1;  /* non-zero to indicate device is off for dc analysis*/
    unsigned EKVtempGiven :1;  /* instance temperature specified */
    unsigned EKVlGiven :1;
    unsigned EKVwGiven :1;
    unsigned EKVdrainAreaGiven :1;
    unsigned EKVsourceAreaGiven    :1;
    unsigned EKVdrainSquaresGiven  :1;
    unsigned EKVsourceSquaresGiven :1;
    unsigned EKVdrainPerimiterGiven    :1;
    unsigned EKVsourcePerimiterGiven   :1;
    unsigned EKVdNodePrimeSet  :1;
    unsigned EKVsNodePrimeSet  :1;
    unsigned EKVicVBSGiven :1;
    unsigned EKVicVDSGiven :1;
    unsigned EKVicVGSGiven :1;
    unsigned EKVvonGiven   :1;
    unsigned EKVvdsatGiven :1;
    unsigned EKVmodeGiven  :1;


    double *EKVDdPtr;      /* pointer to sparse matrix element at
                                         * (Drain node,drain node) */
    double *EKVGgPtr;      /* pointer to sparse matrix element at
                                         * (gate node,gate node) */
    double *EKVSsPtr;      /* pointer to sparse matrix element at
                                         * (source node,source node) */
    double *EKVBbPtr;      /* pointer to sparse matrix element at
                                         * (bulk node,bulk node) */
    double *EKVDPdpPtr;    /* pointer to sparse matrix element at
                                         * (drain prime node,drain prime node) */
    double *EKVSPspPtr;    /* pointer to sparse matrix element at
                                         * (source prime node,source prime node) */
    double *EKVDdpPtr;     /* pointer to sparse matrix element at
                                         * (drain node,drain prime node) */
    double *EKVGbPtr;      /* pointer to sparse matrix element at
                                         * (gate node,bulk node) */
    double *EKVGdpPtr;     /* pointer to sparse matrix element at
                                         * (gate node,drain prime node) */
    double *EKVGspPtr;     /* pointer to sparse matrix element at
                                         * (gate node,source prime node) */
    double *EKVSspPtr;     /* pointer to sparse matrix element at
                                         * (source node,source prime node) */
    double *EKVBdpPtr;     /* pointer to sparse matrix element at
                                         * (bulk node,drain prime node) */
    double *EKVBspPtr;     /* pointer to sparse matrix element at
                                         * (bulk node,source prime node) */
    double *EKVDPspPtr;    /* pointer to sparse matrix element at
                                         * (drain prime node,source prime node) */
    double *EKVDPdPtr;     /* pointer to sparse matrix element at
                                         * (drain prime node,drain node) */
    double *EKVBgPtr;      /* pointer to sparse matrix element at
                                         * (bulk node,gate node) */
    double *EKVDPgPtr;     /* pointer to sparse matrix element at
                                         * (drain prime node,gate node) */

    double *EKVSPgPtr;     /* pointer to sparse matrix element at
                                         * (source prime node,gate node) */
    double *EKVSPsPtr;     /* pointer to sparse matrix element at
                                         * (source prime node,source node) */
    double *EKVDPbPtr;     /* pointer to sparse matrix element at
                                         * (drain prime node,bulk node) */
    double *EKVSPbPtr;     /* pointer to sparse matrix element at
                                         * (source prime node,bulk node) */
    double *EKVSPdpPtr;    /* pointer to sparse matrix element at
                                         * (source prime node,drain prime node) */

#ifdef HAS_SENSE2
    int  EKVsenParmNo;   /* parameter # for sensitivity use;
            set equal to 0  if  neither length
            nor width of the mosfet is a design
                parameter */
    unsigned EKVsens_l :1;   /* field which indicates whether  
            length of the mosfet is a design
                parameter or not */
    unsigned EKVsens_w :1;   /* field which indicates whether  
            width of the mosfet is a design
                parameter or not */
    unsigned EKVsenPertFlag :1; /* indictes whether the the 
            parameter of the particular instance is 
                to be perturbed */
    double EKVcgs;
    double EKVcgd;
    double EKVcgb;

    double *EKVsens;

#define EKVsenCgs EKVsens /* contains pertured values of cgs */
#define EKVsenCgd EKVsens + 6 /* contains perturbed values of cgd*/
#define EKVsenCgb EKVsens + 12 /* contains perturbed values of cgb*/
#define EKVsenCbd EKVsens + 18 /* contains perturbed values of cbd*/
#define EKVsenCbs EKVsens + 24 /* contains perturbed values of cbs*/
#define EKVsenGds EKVsens + 30 /* contains perturbed values of gds*/
#define EKVsenGbs EKVsens + 36 /* contains perturbed values of gbs*/
#define EKVsenGbd EKVsens + 42 /* contains perturbed values of gbd*/
#define EKVsenGm EKVsens + 48 /* contains perturbed values of gm*/
#define EKVsenGmbs EKVsens + 54 /* contains perturbed values of gmbs*/
#define EKVdphigs_dl EKVsens + 60
#define EKVdphigd_dl EKVsens + 61
#define EKVdphigb_dl EKVsens + 62
#define EKVdphibs_dl EKVsens + 63
#define EKVdphibd_dl EKVsens + 64
#define EKVdphigs_dw EKVsens + 65
#define EKVdphigd_dw EKVsens + 66
#define EKVdphigb_dw EKVsens + 67
#define EKVdphibs_dw EKVsens + 68
#define EKVdphibd_dw EKVsens + 69
#endif

};

#define EKVvbd EKVstates+ 0   /* bulk-drain voltage */
#define EKVvbs EKVstates+ 1   /* bulk-source voltage */
#define EKVvgs EKVstates+ 2   /* gate-source voltage */
#define EKVvds EKVstates+ 3   /* drain-source voltage */

#define EKVcapgs EKVstates+ 4  /* gate-source capacitor value */
#define EKVqgs   EKVstates+ 5  /* gate-source capacitor charge */
#define EKVcqgs  EKVstates+ 6  /* gate-source capacitor current */

#define EKVcapgd EKVstates+ 7 /* gate-drain capacitor value */
#define EKVqgd EKVstates+ 8   /* gate-drain capacitor charge */
#define EKVcqgd EKVstates+ 9  /* gate-drain capacitor current */

#define EKVcapgb EKVstates+10 /* gate-bulk capacitor value */
#define EKVqgb EKVstates+ 11  /* gate-bulk capacitor charge */
#define EKVcqgb EKVstates+ 12 /* gate-bulk capacitor current */

#define EKVqbd EKVstates+ 13  /* bulk-drain capacitor charge */
#define EKVcqbd EKVstates+ 14 /* bulk-drain capacitor current */

#define EKVqbs EKVstates+ 15  /* bulk-source capacitor charge */
#define EKVcqbs EKVstates+ 16 /* bulk-source capacitor current */

// SRW -- for interpolation
#define EKVa_cd EKVstates+ 17
#define EKVa_cbs EKVstates+ 18
#define EKVa_cbd EKVstates+ 19

#define EKVnumStates 20

#ifdef HAS_SENSE2
#define EKVsensxpgs EKVstates+20 /* charge sensitivities and 
their derivatives.  +21 for the derivatives:
pointer to the beginning of the array */
#define EKVsensxpgd  EKVstates+22
#define EKVsensxpgb  EKVstates+23
#define EKVsensxpbs  EKVstates+24
#define EKVsensxpbd  EKVstates+25

#define EKVnumSenStates 10
#endif


/* NOTE:  parameters marked 'input - use xxxx' are paramters for
     * which a temperature correction is applied in EKVtemp, thus
     * the EKVxxxx value in the per-instance structure should be used
     * instead in all calculations 
     */

struct sEKVmodel : sGENmodel
{
    sEKVmodel()         { memset(this, 0, sizeof(sEKVmodel)); }
    sEKVmodel *next()   { return ((sEKVmodel*)GENnextModel); }
    sEKVinstance *inst() { return ((sEKVinstance*)GENinstances); }

    double EKVtnom;    /* temperature at which parameters measured */
    double EKVekvint;
    double EKVvt0;     /* input - use tvt0 */
    double EKVkp;      /* input - use tkp */
    double EKVgamma;
    double EKVphi;     /* input - use tphi */
    double EKVcox;
    double EKVxj;
    double EKVtheta;
    double EKVucrit;   /* input - use tucrit */
    double EKVdw;
    double EKVdl;
    double EKVlambda;
    double EKVweta;
    double EKVleta;
    double EKViba;
    double EKVibb;     /* input - use tibb */
    double EKVibn;
    double EKVq0;
    double EKVlk;
    double EKVtcv;
    double EKVbex;
    double EKVucex;
    double EKVibbt;
    double EKVnqs;
    double EKVsatlim;
    double EKVfNcoef;
    double EKVfNexp;
    double EKVjctSatCur;           /* input - use tSatCur */
    double EKVjctSatCurDensity;    /* input - use tSatCurDens */
    double EKVjsw;
    double EKVn;
    double EKVcapBD;               /* input - use tCbd */
    double EKVcapBS;               /* input - use tCbs */
    double EKVbulkCapFactor;       /* input - use tCj */
    double EKVsideWallCapFactor;   /* input - use tCjsw */
    double EKVbulkJctBotGradingCoeff;
    double EKVbulkJctSideGradingCoeff;
    double EKVfwdCapDepCoeff;
    double EKVbulkJctPotential;
    double EKVpbsw;
    double EKVtt;
    double EKVgateSourceOverlapCapFactor;
    double EKVgateDrainOverlapCapFactor;
    double EKVgateBulkOverlapCapFactor;
    double EKVdrainResistance;     /* input - use trd */
    double EKVsourceResistance;    /* input - use trs */
    double EKVsheetResistance;     /* input - use trsh */
    double EKVrsc;                 /* input - use trsc */
    double EKVrdc;                 /* input - use trdc */
    double EKVxti;
    double EKVtr1;
    double EKVtr2;
    double EKVnlevel;
    double EKVe0;                  /* wg 17-SEP-2K */
    int EKVgateType;
    int EKVtype;       /* device type : 1 = nmos,  -1 = pmos */

    unsigned EKVtypeGiven  :1;
    unsigned EKVekvintGiven :1;
    unsigned EKVtnomGiven  :1;
    unsigned EKVvt0Given   :1;
    unsigned EKVkpGiven  :1;
    unsigned EKVgammaGiven :1;
    unsigned EKVphiGiven   :1;
    unsigned EKVcoxGiven :1;
    unsigned EKVxjGiven :1;
    unsigned EKVthetaGiven :1;
    unsigned EKVucritGiven :1;
    unsigned EKVdwGiven :1;
    unsigned EKVdlGiven :1;
    unsigned EKVlambdaGiven    :1;
    unsigned EKVwetaGiven :1;
    unsigned EKVletaGiven :1;
    unsigned EKVibaGiven :1;
    unsigned EKVibbGiven :1;
    unsigned EKVibnGiven :1;
    unsigned EKVq0Given :1;
    unsigned EKVlkGiven :1;
    unsigned EKVtcvGiven :1;
    unsigned EKVbexGiven :1;
    unsigned EKVucexGiven :1;
    unsigned EKVibbtGiven :1;
    unsigned EKVnqsGiven :1;
    unsigned EKVsatlimGiven :1;
    unsigned EKVfNcoefGiven  :1;
    unsigned EKVfNexpGiven   :1;
    unsigned EKVjctSatCurGiven :1;
    unsigned EKVjctSatCurDensityGiven  :1;
    unsigned EKVjswGiven :1;
    unsigned EKVnGiven :1;
    unsigned EKVcapBDGiven :1;
    unsigned EKVcapBSGiven :1;
    unsigned EKVbulkCapFactorGiven :1;
    unsigned EKVsideWallCapFactorGiven   :1;
    unsigned EKVbulkJctBotGradingCoeffGiven    :1;
    unsigned EKVbulkJctSideGradingCoeffGiven   :1;
    unsigned EKVfwdCapDepCoeffGiven    :1;
    unsigned EKVbulkJctPotentialGiven  :1;
    unsigned EKVpbswGiven :1;
    unsigned EKVttGiven :1;
    unsigned EKVgateSourceOverlapCapFactorGiven    :1;
    unsigned EKVgateDrainOverlapCapFactorGiven :1;
    unsigned EKVgateBulkOverlapCapFactorGiven  :1;
    unsigned EKVdrainResistanceGiven   :1;
    unsigned EKVsourceResistanceGiven  :1;
    unsigned EKVsheetResistanceGiven   :1;
    unsigned EKVrscGiven :1;
    unsigned EKVrdcGiven :1;
    unsigned EKVxtiGiven :1;
    unsigned EKVtr1Given :1;
    unsigned EKVtr2Given :1;
    unsigned EKVnlevelGiven :1;
    unsigned EKVe0Given :1;                  /* wg 17-SEP-2K */

    unsigned EKVxqcGiven :1;  // SRW
};

} // namespace EKV26
using namespace EKV26;


#ifndef NMOS
#define NMOS 1
#define PMOS -1
#endif

// device parameters
enum {
    EKV_W = 1,
    EKV_L,
    EKV_AS,
    EKV_AD,
    EKV_PS,
    EKV_PD,
    EKV_NRS,
    EKV_NRD,
    EKV_OFF,
    EKV_IC,
    EKV_IC_VBS,
    EKV_IC_VDS,
    EKV_IC_VGS,

#ifdef HAS_SENSE2
    EKV_W_SENS,
    EKV_L_SENS,
#endif

    EKV_CB,
    EKV_CG,
    EKV_CS,
    EKV_POWER,
    EKV_TEMP,

    EKV_TVTO,
    EKV_TPHI,
    EKV_TKP,
    EKV_TUCRIT,
    EKV_TIBB,
    EKV_TRS,
    EKV_TRD,
    EKV_TRSH,
    EKV_TRSC,
    EKV_TRDC,
    EKV_TIS,
    EKV_TJS,
    EKV_TJSW,
    EKV_TPB,
    EKV_TPBSW,
    EKV_TCBD,
    EKV_TCBS,
    EKV_TCJ,
    EKV_TCJSW,
    EKV_TAF,
    EKV_ISUB,
    EKV_VP,
    EKV_SLOPE,
    EKV_IF,
    EKV_IR,
    EKV_IRPRIME,
    EKV_TAU,

    EKV_CGS,
    EKV_CGD,
    EKV_DNODE,
    EKV_GNODE,
    EKV_SNODE,
    EKV_BNODE,
    EKV_DNODEPRIME,
    EKV_SNODEPRIME,
    EKV_SOURCECONDUCT,
    EKV_DRAINCONDUCT,
    EKV_VON,
    EKV_VDSAT,
    EKV_VGEFF,
    EKV_VGPRIME,
    EKV_SOURCEVCRIT,
    EKV_DRAINVCRIT,
    EKV_CD,
    EKV_CBS,
    EKV_CBD,
    EKV_GMBS,
    EKV_GM,
    EKV_GMS,
    EKV_GDS,
    EKV_GBD,
    EKV_GBS,
    EKV_CAPBD,
    EKV_CAPBS,
    EKV_CAPZEROBIASBD,
    EKV_CAPZEROBIASBDSW,
    EKV_CAPZEROBIASBS,
    EKV_CAPZEROBIASBSSW,
    EKV_VBD,
    EKV_VBS,
    EKV_VGS,
    EKV_VDS,
    EKV_CAPGS,
    EKV_QGS,
    EKV_CQGS,
    EKV_CAPGD,
    EKV_QGD,
    EKV_CQGD,
    EKV_CAPGB,
    EKV_QGB,
    EKV_CQGB,
    EKV_QBD,
    EKV_CQBD,
    EKV_QBS,
    EKV_CQBS,

#ifdef HAS_SENSE2
    EKV_L_SENS_REAL,
    EKV_L_SENS_IMAG,
    EKV_L_SENS_MAG,
    EKV_L_SENS_PH,
    EKV_L_SENS_CPLX,
    EKV_W_SENS_REAL,
    EKV_W_SENS_IMAG,
    EKV_W_SENS_MAG,
    EKV_W_SENS_PH,
    EKV_W_SENS_CPLX,
    EKV_L_SENS_DC,
    EKV_W_SENS_DC,
#endif

    EKV_SOURCERESIST,
    EKV_DRAINRESIST,
    EKV_VGSTAR
};

// model paramerers
enum {
    EKV_MOD_TNOM = 1000,
    EKV_MOD_VTO,
    EKV_MOD_KP,
    EKV_MOD_GAMMA,
    EKV_MOD_PHI,
    EKV_MOD_COX,
    EKV_MOD_XJ,
    EKV_MOD_THETA,
    EKV_MOD_UCRIT,
    EKV_MOD_DW,
    EKV_MOD_DL,
    EKV_MOD_LAMBDA,
    EKV_MOD_WETA,
    EKV_MOD_LETA,
    EKV_MOD_IBA,
    EKV_MOD_IBB,
    EKV_MOD_IBN,
    EKV_MOD_TCV,
    EKV_MOD_BEX,
    EKV_MOD_UCEX,
    EKV_MOD_IBBT,
    EKV_MOD_NQS,
    EKV_MOD_SATLIM,
    EKV_MOD_KF,
    EKV_MOD_AF,
    EKV_MOD_IS,
    EKV_MOD_JS,
    EKV_MOD_JSW,
    EKV_MOD_N,
    EKV_MOD_CBD,
    EKV_MOD_CBS,
    EKV_MOD_CJ,
    EKV_MOD_CJSW,
    EKV_MOD_MJ,
    EKV_MOD_MJSW,
    EKV_MOD_FC,
    EKV_MOD_PB,
    EKV_MOD_PBSW,
    EKV_MOD_TT,
    EKV_MOD_CGSO,
    EKV_MOD_CGDO,
    EKV_MOD_CGBO,
    EKV_MOD_RD,
    EKV_MOD_RS,
    EKV_MOD_RSH,
    EKV_MOD_RSC,
    EKV_MOD_RDC,
    EKV_MOD_XTI,
    EKV_MOD_TR1,
    EKV_MOD_TR2,
    EKV_MOD_NLEVEL,
    EKV_MOD_EKVINT,
    EKV_MOD_Q0,
    EKV_MOD_LK,
    EKV_MOD_NMOS,
    EKV_MOD_PMOS,
    EKV_MOD_TYPE,
    EKV_MOD_E0,

    // SRW
    EKV_MOD_XQC
};

#endif // EKVDEFS_H

