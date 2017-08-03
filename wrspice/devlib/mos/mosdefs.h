
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
Authors: 1985 Thomas L. Quarles
         1989 Takayasu Sakurai
         1993 Stephen R. Whiteley
****************************************************************************/

#ifndef MOSDEFS_H
#define MOSDEFS_H

#include "device.h"

//
// declarations for analytical MOSFETs
//


#define FABS            fabs
#define REFTEMP         wrsREFTEMP       
#define CHARGE          wrsCHARGE        
#define CONSTCtoK       wrsCONSTCtoK     
#define CONSTboltz      wrsCONSTboltz    
#define CONSTvt0        wrsCONSTvt0      
#define CONSTKoverQ     wrsCONSTKoverQ   
#define CONSTroot2      wrsCONSTroot2    
#define CONSTe          wrsCONSTe        

namespace MOS {

struct sMOSmodel;
struct sMOSinstance;
struct mosstuff;

struct MOSdev : public IFdevice
{
    MOSdev();
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
    int disto(int, sGENmodel*, sCKT*);  
    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);

private:
    int dSetup(sMOSmodel*, sCKT*);
    void fd(sMOSmodel*, sMOSinstance*);
    void fs(sMOSmodel*, sMOSinstance*);
};

struct sMOSinstance : public sGENinstance
{
    sMOSinstance()
        {
            memset(this, 0, sizeof(sMOSinstance));
            GENnumNodes = 4;
        }
    ~sMOSinstance()     { delete [] (char*)MOSbacking; }
    sMOSinstance *next()
        { return (static_cast<sMOSinstance*>(GENnextInstance)); }
    void ac_cd(const sCKT*, double*, double*) const;
    void ac_cs(const sCKT*, double*, double*) const;
    void ac_cg(const sCKT*, double*, double*) const;
    void ac_cb(const sCKT*, double*, double*) const;

    int  MOSdNode;   // number of the gate node of the mosfet
    int  MOSgNode;   // number of the gate node of the mosfet
    int  MOSsNode;   // number of the source node of the mosfet
    int  MOSbNode;   // number of the bulk node of the mosfet

    int MOSdNodePrime; // number of the internal drain node of the mosfet
    int MOSsNodePrime; // number of the internal source node of the mosfet

    int MOSmode;   // device mode : 1 = normal, -1 = inverse

    double MOSmult;              // device multiplier
    double MOSl;                 // the length of the channel region
    double MOSw;                 // the width of the channel region
    double MOSdrainArea;         // the area of the drain diffusion
    double MOSsourceArea;        // the area of the source diffusion
    double MOSdrainSquares;      // the length of the drain in squares
    double MOSsourceSquares;     // the length of the source in squares
    double MOSdrainPerimeter;
    double MOSsourcePerimeter;
    double MOSsourceConductance; // conductance of source, set in setup
    double MOSdrainConductance;  // conductance of drain, set in setup
    double MOStemp;              // operating temperature of this instance

    // temperature corrected ...
    double MOStTransconductance; // transconductance
    double MOStSurfMob;    // surface mobility
    double MOStPhi;        // Phi
    double MOStVto;        // Vto
    double MOStSatCur;     // saturation Cur.
    double MOStSatCurDens; // saturation Cur. density
    double MOStCbd;        // B-D Capacitance
    double MOStCbs;        // B-S Capacitance
    double MOStCj;         // Bulk bottom Capacitance
    double MOStCjsw;       // Bulk side Capacitance
    double MOStBulkPot;    // Bulk potential
    double MOStDepCap;     // transition point in the curve matching Fc*Vj
    double MOStVbi;        // Vbi
    double MOStDrainSatCur;
    double MOStSourceSatCur;

    double MOSicVBS;       // initial condition B-S voltage
    double MOSicVDS;       // initial condition D-S voltage
    double MOSicVGS;       // initial condition G-S voltage
    double MOSvon;
    double MOSvdsat;
    double MOSsourceVcrit; // vcrit for pos. vds */
    double MOSdrainVcrit;  // vcrit for neg. vds */
    double MOScd;
    double MOScbs;
    double MOScbd;
    double MOSgmbs;
    double MOSgm;
    double MOSgds;
    double MOSgbd;
    double MOSgbs;
    double MOScapbd;
    double MOScapbs;
    double MOSCbd;
    double MOSCbdsw;
    double MOSCbs;
    double MOSCbssw;
    double MOSf2d;
    double MOSf3d;
    double MOSf4d;
    double MOSf2s;
    double MOSf3s;
    double MOSf4s;

    double MOSeffectiveLength;
    double MOSgateSourceOverlapCap;
    double MOSgateDrainOverlapCap;
    double MOSgateBulkOverlapCap;
    double MOSbeta;
    double MOSoxideCap;

    // This provides a means to back up and restore a known-good
    // state.
    void *MOSbacking;
    void backup(DEV_BKMODE m)
        {
            if (m == DEV_SAVE) {
                if (!MOSbacking)
                    MOSbacking = new char[sizeof(sMOSinstance)];
                memcpy(MOSbacking, this, sizeof(sMOSinstance));
            }
            else if (m == DEV_RESTORE) {
                if (MOSbacking)
                    memcpy(this, MOSbacking, sizeof(sMOSinstance));
            }
            else {
                // DEV_CLEAR
                delete [] (char*)MOSbacking;
                MOSbacking = 0;
            }
        }

    double *MOSDdPtr;      // pointer to sparse matrix element at
                           //  (Drain node,drain node)
    double *MOSGgPtr;      // pointer to sparse matrix element at
                           //  (gate node,gate node)
    double *MOSSsPtr;      // pointer to sparse matrix element at
                           //  (source node,source node)
    double *MOSBbPtr;      // pointer to sparse matrix element at
                           //  (bulk node,bulk node)
    double *MOSDPdpPtr;    // pointer to sparse matrix element at
                           //  (drain prime node,drain prime node)
    double *MOSSPspPtr;    // pointer to sparse matrix element at
                           //  (source prime node,source prime node)
    double *MOSDdpPtr;     // pointer to sparse matrix element at
                           //  (drain node,drain prime node)
    double *MOSGbPtr;      // pointer to sparse matrix element at
                           //  (gate node,bulk node)
    double *MOSGdpPtr;     // pointer to sparse matrix element at
                           //  (gate node,drain prime node)
    double *MOSGspPtr;     // pointer to sparse matrix element at
                           //  (gate node,source prime node)
    double *MOSSspPtr;     // pointer to sparse matrix element at
                           //  (source node,source prime node)
    double *MOSBdpPtr;     // pointer to sparse matrix element at
                           //  (bulk node,drain prime node)
    double *MOSBspPtr;     // pointer to sparse matrix element at
                           //  (bulk node,source prime node)
    double *MOSDPspPtr;    // pointer to sparse matrix element at
                           //  (drain prime node,source prime node)
    double *MOSDPdPtr;     // pointer to sparse matrix element at
                           //  (drain prime node,drain node)
    double *MOSBgPtr;      // pointer to sparse matrix element at
                           //  (bulk node,gate node)
    double *MOSDPgPtr;     // pointer to sparse matrix element at
                           //  (drain prime node,gate node)

    double *MOSSPgPtr;     // pointer to sparse matrix element at
                           //  (source prime node,gate node)
    double *MOSSPsPtr;     // pointer to sparse matrix element at
                           //  (source prime node,source node)
    double *MOSDPbPtr;     // pointer to sparse matrix element at
                           //  (drain prime node,bulk node)
    double *MOSSPbPtr;     // pointer to sparse matrix element at
                           //  (source prime node,bulk node)
    double *MOSSPdpPtr;    // pointer to sparse matrix element at
                           //  (source prime node,drain prime node)

// distortion stuff
//
// naming convention:
// x = vgs
// y = vbs
// z = vds
// cdr = cdrain
//

#define MOSNDCOEFFS 30

#ifndef NODISTO
    double MOSdCoeffs[MOSNDCOEFFS];
#else
    double *MOSdCoeffs;
#endif

#ifdef DISTO
#define capbs2   MOSdCoeffs[0]
#define capbs3   MOSdCoeffs[1]
#define capbd2   MOSdCoeffs[2]
#define capbd3   MOSdCoeffs[3]
#define gbs2     MOSdCoeffs[4]
#define gbs3     MOSdCoeffs[5]
#define gbd2     MOSdCoeffs[6]
#define gbd3     MOSdCoeffs[7]
#define capgb2   MOSdCoeffs[8]
#define capgb3   MOSdCoeffs[9]
#define cdr_x2   MOSdCoeffs[10]
#define cdr_y2   MOSdCoeffs[11]
#define cdr_z2   MOSdCoeffs[12]
#define cdr_xy   MOSdCoeffs[13]
#define cdr_yz   MOSdCoeffs[14]
#define cdr_xz   MOSdCoeffs[15]
#define cdr_x3   MOSdCoeffs[16]
#define cdr_y3   MOSdCoeffs[17]
#define cdr_z3   MOSdCoeffs[18]
#define cdr_x2z  MOSdCoeffs[19]
#define cdr_x2y  MOSdCoeffs[20]
#define cdr_y2z  MOSdCoeffs[21]
#define cdr_xy2  MOSdCoeffs[22]
#define cdr_xz2  MOSdCoeffs[23]
#define cdr_yz2  MOSdCoeffs[24]
#define cdr_xyz  MOSdCoeffs[25]
#define capgs2   MOSdCoeffs[26]
#define capgs3   MOSdCoeffs[27]
#define capgd2   MOSdCoeffs[28]
#define capgd3   MOSdCoeffs[29]
#endif
//  end distortion coeffs.

// indices to the array of MOSFET(3) noise sources

#define MOSRDNOIZ  0
#define MOSRSNOIZ  1
#define MOSIDNOIZ  2
#define MOSFLNOIZ  3
#define MOSTOTNOIZ 4

#define MOSNSRCS   5     // the number of MOSFET(3) noise sources

#ifndef NONOISE
    double MOSnVar[NSTATVARS][MOSNSRCS];
#else
    double **MOSnVar;
#endif

    unsigned MOSoff :1;        // device is off for dc analysis
    unsigned MOStempGiven :1;  // instance temperature specified
    unsigned MOSmGiven :1;
    unsigned MOSlGiven :1;
    unsigned MOSwGiven :1;
    unsigned MOSdrainAreaGiven :1;
    unsigned MOSsourceAreaGiven :1;
    unsigned MOSdrainSquaresGiven :1;
    unsigned MOSsourceSquaresGiven :1;
    unsigned MOSdrainPerimeterGiven :1;
    unsigned MOSsourcePerimeterGiven :1;
    unsigned MOSdNodePrimeSet :1;
    unsigned MOSsNodePrimeSet :1;
    unsigned MOSicVBSGiven :1;
    unsigned MOSicVDSGiven :1;
    unsigned MOSicVGSGiven :1;
    unsigned MOSvonGiven :1;
    unsigned MOSvdsatGiven :1;
    unsigned MOSmodeGiven :1;
};

#define MOSvbd         GENstate
#define MOSvbs         GENstate + 1
#define MOSvgs         GENstate + 2
#define MOSvds         GENstate + 3

// meyer capacitances
#define MOScapgs       GENstate + 4   // gate-source capacitor value
#define MOSqgs         GENstate + 5   // gate-source capacitor charge
#define MOScqgs        GENstate + 6   // gate-source capacitor current

#define MOScapgd       GENstate + 7   // gate-drain capacitor value
#define MOSqgd         GENstate + 8   // gate-drain capacitor charge
#define MOScqgd        GENstate + 9   // gate-drain capacitor current

#define MOScapgb       GENstate + 10  // gate-bulk capacitor value
#define MOSqgb         GENstate + 11  // gate-bulk capacitor charge
#define MOScqgb        GENstate + 12  // gate-bulk capacitor current

// diode capacitances
#define MOSqbd         GENstate + 13  // bulk-drain capacitor charge
#define MOScqbd        GENstate + 14  // bulk-drain capacitor current

#define MOSqbs         GENstate + 15  // bulk-source capacitor charge
#define MOScqbs        GENstate + 16  // bulk-source capacitor current

// stuff for MOSask()
#define MOSa_cd        GENstate + 17
#define MOSa_cbd       GENstate + 18
#define MOSa_cbs       GENstate + 19
#define MOSa_von       GENstate + 20
#define MOSa_vdsat     GENstate + 21
#define MOSa_dVcrit    GENstate + 22
#define MOSa_sVcrit    GENstate + 23
#define MOSa_gmbs      GENstate + 24
#define MOSa_gm        GENstate + 25
#define MOSa_gds       GENstate + 26
#define MOSa_gbd       GENstate + 27
#define MOSa_gbs       GENstate + 28
#define MOSa_capbd     GENstate + 29
#define MOSa_capbs     GENstate + 30
#define MOSa_Cbd       GENstate + 31
#define MOSa_Cbdsw     GENstate + 32
#define MOSa_Cbs       GENstate + 33
#define MOSa_Cbssw     GENstate + 34

#define MOSnumStates 35



// NOTE:  parameters marked 'input - use xxxx' are paramters for
// which a temperature correction is applied in MOStemp, thus
// the MOSxxxx value in the per-instance structure should be used
// instead in all calculations 

struct sMOSmodel : public sGENmodel
{
    sMOSmodel()         { memset(this, 0, sizeof(sMOSmodel)); }
    sMOSmodel *next()   { return (static_cast<sMOSmodel*>(GENnextModel)); }
    sMOSinstance *inst() { return (static_cast<sMOSinstance*>(GENinstances)); }

    int MOStype;       // device type : 1 = nmos,  -1 = pmos
    int MOSlevel;      // UCB model complexity level, 1-3
    int MOSgateType;
    double MOSoxideCapFactor;
    double MOStnom;    // temperature at which parameters measured
    double MOSvt0;               // input - use tVto
    double MOStransconductance;  // input - use tTransconductance
    double MOSgamma;
    double MOSphi;               // input - use tPhi
    double MOSdrainResistance;
    double MOSsourceResistance;
    double MOScapBD;             // input - use tCbs
    double MOScapBS;             // input - use tCbd
    double MOSjctSatCur;         // input - use tSatCur instead
    double MOSbulkJctPotential;  // input - use tBulkPot
    double MOSgateSourceOverlapCapFactor;
    double MOSgateDrainOverlapCapFactor;
    double MOSgateBulkOverlapCapFactor;
    double MOSbulkCapFactor;     // input - use tCj
    double MOSbulkJctBotGradingCoeff;
    double MOSsideWallCapFactor; // input - use tCjsw
    double MOSbulkJctSideGradingCoeff;
    double MOSjctSatCurDensity;  // input - use tSatCurDens
    double MOSoxideThickness;
    double MOSlatDiff;
    double MOSsheetResistance;
    double MOSsurfaceMobility;   // input - use tSurfMob
    double MOSfwdCapDepCoeff;
    double MOSsurfaceStateDensity;
    double MOSsubstrateDoping;
    double MOSfNcoef;
    double MOSfNexp;

    // levels 1,2 and 6
    double MOSlambda;

    // level 2
    double MOScritFieldExp;      // uexp
    double MOSchannelCharge;     // neff
    double MOScritField;         // ucrit

    // levels 2 and 3
    double MOSfastSurfaceStateDensity; // nfs
    double MOSnarrowFactor;      // delta
    double MOSmaxDriftVel;       // vmax
    double MOSjunctionDepth;
    double MOSxd;

    // level 3
    double MOSeta;
    double MOStheta;             // theta
    double MOSkappa;             // kappa
    double MOSdelta;             // input delta
    double MOSalpha;             // alpha

    // level 6
    double MOSkv;       // input - use tKv
    double MOSnv;       // drain linear conductance factor
    double MOSkc;       // input - use tKc
    double MOSnc;       // saturation current coeff.
    double MOSgamma1;   // secondary back-gate effect parametr
    double MOSsigma;
    double MOSlamda0;
    double MOSlamda1;

    unsigned MOStypeGiven :1;
    unsigned MOSgateTypeGiven :1;
    unsigned MOSlevelGiven :1;
    unsigned MOStnomGiven :1;
    unsigned MOSvt0Given :1;
    unsigned MOStransconductanceGiven :1;
    unsigned MOSgammaGiven :1;
    unsigned MOSphiGiven :1;
    unsigned MOSdrainResistanceGiven :1;
    unsigned MOSsourceResistanceGiven :1;
    unsigned MOScapBDGiven :1;
    unsigned MOScapBSGiven :1;
    unsigned MOSjctSatCurGiven :1;
    unsigned MOSbulkJctPotentialGiven :1;
    unsigned MOSgateSourceOverlapCapFactorGiven :1;
    unsigned MOSgateDrainOverlapCapFactorGiven :1;
    unsigned MOSgateBulkOverlapCapFactorGiven :1;
    unsigned MOSbulkCapFactorGiven :1;
    unsigned MOSbulkJctBotGradingCoeffGiven :1;
    unsigned MOSsideWallCapFactorGiven :1;
    unsigned MOSbulkJctSideGradingCoeffGiven :1;
    unsigned MOSjctSatCurDensityGiven :1;
    unsigned MOSoxideThicknessGiven :1;
    unsigned MOSlatDiffGiven :1;
    unsigned MOSsheetResistanceGiven :1;
    unsigned MOSsurfaceMobilityGiven :1;
    unsigned MOSfwdCapDepCoeffGiven :1;
    unsigned MOSsurfaceStateDensityGiven :1;
    unsigned MOSsubstrateDopingGiven :1;
    unsigned MOSfNcoefGiven :1;
    unsigned MOSfNexpGiven :1;

    // levels 1,2, and 6
    unsigned MOSlambdaGiven :1;

    // level 2
    unsigned MOScritFieldExpGiven :1;   // uexp
    unsigned MOSchannelChargeGiven :1;  // neff
    unsigned MOScritFieldGiven :1;      // ucrit

    // levels 2 and 3
    unsigned MOSfastSurfaceStateDensityGiven :1; // nfs
    unsigned MOSnarrowFactorGiven :1;   // delta
    unsigned MOSmaxDriftVelGiven :1;    // vmax
    unsigned MOSjunctionDepthGiven :1;

    // level 3
    unsigned MOSetaGiven :1;
    unsigned MOSthetaGiven :1;          // theta
    unsigned MOSkappaGiven :1;          // kappa
    unsigned MOSdeltaGiven :1;          // delta

    // level 6
    unsigned MOSkvGiven :1;
    unsigned MOSnvGiven :1;
    unsigned MOSkcGiven :1;
    unsigned MOSncGiven :1;
    unsigned MOSgamma1Given :1;
    unsigned MOSsigmaGiven :1;
    unsigned MOSlamda0Given :1;
    unsigned MOSlamda1Given :1;

};

// store some things to pass to functions
struct mosstuff
{
    int  limiting(sCKT*, sMOSinstance*);
    int  bypass(sCKT*, sMOSmodel*, sMOSinstance*);
    void iv(sCKT*, sMOSmodel*, sMOSinstance*);
    void cap(sCKT*, sMOSmodel*, sMOSinstance*);
    int  integ(sCKT*, sMOSinstance*);
    void load(sCKT*, sMOSmodel*, sMOSinstance*);
    void load_dc(sCKT*, sMOSmodel*, sMOSinstance*);

    double eq1(sMOSmodel*, sMOSinstance*);
    double eq2(sMOSmodel*, sMOSinstance*);
    double eq3(sMOSmodel*, sMOSinstance*);
    double eq6(sMOSmodel*, sMOSinstance*);

    double ms_vt;
    double ms_von;
    double ms_vdsat;
    double ms_vgs;
    double ms_vds;
    double ms_vbs;
    double ms_vbd;
    double ms_vgb;
    double ms_vgd;
    double ms_ceqgs;
    double ms_ceqgd;
    double ms_ceqgb;
    double ms_cdrain;
    double ms_gcgs;
    double ms_gcgd;
    double ms_gcgb;
    double ms_capgs;
    double ms_capgd;
    double ms_capgb;
};

} // namespace MOS
using namespace MOS;


#ifndef NMOS
#define NMOS 1
#define PMOS -1
#endif

// device parameters
enum {
    MOS_L = 1,
    MOS_W,
    MOS_AD,
    MOS_AS,
    MOS_PD,
    MOS_PS,
    MOS_NRD,
    MOS_NRS,
    MOS_TEMP,
    MOS_OFF,
    MOS_IC_VDS,
    MOS_IC_VGS,
    MOS_IC_VBS,
    MOS_IC,
    MOS_VBD,
    MOS_VBS,
    MOS_VGS,
    MOS_VDS,
    MOS_VON,
    MOS_VDSAT,
    MOS_DRAINVCRIT,
    MOS_SOURCEVCRIT,
    MOS_CD,
    MOS_CBD,
    MOS_CBS,
    MOS_CG,
    MOS_CS,
    MOS_CB,
    MOS_POWER,
    MOS_DRAINRES,
    MOS_SOURCERES,
    MOS_DRAINCOND,
    MOS_SOURCECOND,
    MOS_GMBS,
    MOS_GM,
    MOS_GDS,
    MOS_GBD,
    MOS_GBS,
    MOS_QGD,
    MOS_QGS,
    MOS_QBD,
    MOS_QBS,
    MOS_QGB,
    MOS_CAPBD,
    MOS_CAPBS,
    MOS_CAPZBBD,
    MOS_CAPZBBDSW,
    MOS_CAPZBBS,
    MOS_CAPZBBSSW,
    MOS_CAPGD,
    MOS_CQGD,
    MOS_CAPGS,
    MOS_CQGS,
    MOS_CAPGB,
    MOS_CQGB,
    MOS_CQBD,
    MOS_CQBS,
    MOS_DNODE,
    MOS_GNODE,
    MOS_SNODE,
    MOS_BNODE,
    MOS_DNODEPRIME,
    MOS_SNODEPRIME,
    MOS_M
};

// model parameters
enum {
    MOS_MOD_TYPE = 1000,
    MOS_MOD_LEVEL,
    MOS_MOD_TNOM,
    MOS_MOD_VTO,
    MOS_MOD_KP,
    MOS_MOD_GAMMA,
    MOS_MOD_PHI,
    MOS_MOD_RD,
    MOS_MOD_RS,
    MOS_MOD_CBD,
    MOS_MOD_CBS,
    MOS_MOD_IS,
    MOS_MOD_PB,
    MOS_MOD_CGSO,
    MOS_MOD_CGDO,
    MOS_MOD_CGBO,
    MOS_MOD_CJ,
    MOS_MOD_MJ,
    MOS_MOD_CJSW,
    MOS_MOD_MJSW,
    MOS_MOD_JS,
    MOS_MOD_TOX,
    MOS_MOD_LD,
    MOS_MOD_RSH,
    MOS_MOD_U0,
    MOS_MOD_FC,
    MOS_MOD_NSS,
    MOS_MOD_NSUB,
    MOS_MOD_TPG,
    MOS_MOD_NMOS,
    MOS_MOD_PMOS,
    MOS_MOD_KF,
    MOS_MOD_AF,
    MOS_MOD_LAMBDA,
    MOS_MOD_UEXP,
    MOS_MOD_NEFF,
    MOS_MOD_UCRIT,
    MOS_MOD_NFS,
    MOS_MOD_DELTA,
    MOS_MOD_VMAX,
    MOS_MOD_XJ,
    MOS_MOD_ETA,
    MOS_MOD_THETA,
    MOS_MOD_ALPHA,
    MOS_MOD_KAPPA,
    MOS_MOD_XD,
    MOS_MOD_IDELTA,
    MOS_MOD_KV,
    MOS_MOD_NV,
    MOS_MOD_KC,
    MOS_MOD_NC,
    MOS_MOD_GAMMA1,
    MOS_MOD_SIGMA,
    MOS_MOD_LAMDA0,
    MOS_MOD_LAMDA1
};

#define SARGS(arg,bot,side,sarg,sargsw) { \
  if(bot==side) \
    { if(bot==.5) sarg=sargsw=1/sqrt(arg); \
    else sarg=sargsw=exp(-bot*log(arg)); } \
  else \
    { if(bot==.5) sarg=1/sqrt(arg); \
    else sarg=exp(-bot*log(arg)); \
    if(side==.5) sargsw=1/sqrt(arg); \
    else sargsw=exp(-side*log(arg)); } \
}

#endif // MOSDEFS_H

