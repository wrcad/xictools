
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
         1993 Stephen R. Whiteley
****************************************************************************/

#ifndef BJTDEFS_H
#define BJTDEFS_H

#include "device.h"

//
// structures to describe Bipolar Junction Transistors
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

namespace BJT {

struct sBJTmodel;

struct BJTdev : public IFdevice
{
    BJTdev();
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
    int dSetup(sBJTmodel*, sCKT*);
};

struct sBJTinstance : public sGENinstance
{
    sBJTinstance()
        {
            memset(this, 0, sizeof(sBJTinstance));
            GENnumNodes = 4;
        }
    ~sBJTinstance() { delete [] (char*)BJTbacking; }
    sBJTinstance *next()
        { return (static_cast<sBJTinstance*>(GENnextInstance)); }
    void ac_gm(const sCKT*, double*, double*) const;
    void ac_cc(const sCKT*, double*, double*) const;
    void ac_cb(const sCKT*, double*, double*) const;
    void ac_ce(const sCKT*, double*, double*) const;

    int BJTcolNode;       // number of collector node of bjt
    int BJTbaseNode;      // number of base node of bjt
    int BJTemitNode;      // number of emitter node of bjt
    int BJTsubstNode;     // number of substrate node of bjt

    int BJTcolPrimeNode;  // number of internal collector node of bjt
    int BJTbasePrimeNode; // number of internal base node of bjt
    int BJTemitPrimeNode; // number of internal emitter node of bjt

    double BJTarea;       // area factor for the bjt
    double BJTicVBE;      // initial condition voltage B-E
    double BJTicVCE;      // initial condition voltage C-E
    double BJTtemp;       // instance temperature
    double BJTtSatCur;    // temperature adjusted saturation current
    double BJTtBetaF;     // temperature adjusted forward beta
    double BJTtBetaR;     // temperature adjusted reverse beta
    double BJTtBEleakCur; // temperature adjusted B-E leakage current
    double BJTtBCleakCur; // temperature adjusted B-C leakage current
    double BJTtBEcap;     // temperature adjusted B-E capacitance
    double BJTtBEpot;     // temperature adjusted B-E potential
    double BJTtBCcap;     // temperature adjusted B-C capacitance
    double BJTtBCpot;     // temperature adjusted B-C potential
    double BJTtDepCap;    // temperature adjusted join point in diode curve
    double BJTtf1;        // temperature adjusted polynomial coefficient
    double BJTtf4;        // temperature adjusted polynomial coefficient
    double BJTtf5;        // temperature adjusted polynomial coefficient
    double BJTtVcrit;     // temperature adjusted critical voltage
    double BJTcapbe;
    double BJTcapbc;
    double BJTcapcs;
    double BJTcapbx;
    double BJTcc;
    double BJTcb;
    double BJTgpi;
    double BJTgmu;
    double BJTgm;
    double BJTgo;
    double BJTgx;
    double BJTgeqcb;
    double BJTgccs;
    double BJTgeqbx;

    double *BJTcolColPrimePtr;   // pointer to sparse matrix at
                                 //  (collector,collector prime)
    double *BJTbaseBasePrimePtr; // pointer to sparse matrix at
                                 //  (base,base prime)
    double *BJTemitEmitPrimePtr; // pointer to sparse matrix at
                                 //  (emitter,emitter prime)
    double *BJTcolPrimeColPtr;   // pointer to sparse matrix at
                                 //  (collector prime,collector)
    double *BJTcolPrimeBasePrimePtr; // pointer to sparse matrix at
                                 //  (collector prime,base prime)
    double *BJTcolPrimeEmitPrimePtr; // pointer to sparse matrix at
                                 //  (collector prime,emitter prime)
    double *BJTbasePrimeBasePtr; // pointer to sparse matrix at
                                 //  (base prime,base )
    double *BJTbasePrimeColPrimePtr; // pointer to sparse matrix at
                                 //  (base prime,collector prime)
    double *BJTbasePrimeEmitPrimePtr; // pointer to sparse matrix at
                                 //  (base primt,emitter prime)
    double *BJTemitPrimeEmitPtr; // pointer to sparse matrix at
                                 //  (emitter prime,emitter)
    double *BJTemitPrimeColPrimePtr; // pointer to sparse matrix at
                                 //  (emitter prime,collector prime)
    double *BJTemitPrimeBasePrimePtr; // pointer to sparse matrix at
                                 //  (emitter prime,base prime)
    double *BJTcolColPtr;        // pointer to sparse matrix at
                                 //  (collector,collector)
    double *BJTbaseBasePtr;      // pointer to sparse matrix at
                                 //  (base,base)
    double *BJTemitEmitPtr;      // pointer to sparse matrix at
                                 //  (emitter,emitter)
    double *BJTcolPrimeColPrimePtr; // pointer to sparse matrix at
                                 //  (collector prime,collector prime)
    double *BJTbasePrimeBasePrimePtr; // pointer to sparse matrix at
                                 //  (base prime,base prime)
    double *BJTemitPrimeEmitPrimePtr; // pointer to sparse matrix at
                                 //  (emitter prime,emitter prime)
    double *BJTsubstSubstPtr;    // pointer to sparse matrix at
                                 //  (substrate,substrate)
    double *BJTcolPrimeSubstPtr; // pointer to sparse matrix at
                                 //  (collector prime,substrate)
    double *BJTsubstColPrimePtr; // pointer to sparse matrix at
                                 //  (substrate,collector prime)
    double *BJTbaseColPrimePtr;  // pointer to sparse matrix at
                                 //  (base,collector prime)
    double *BJTcolPrimeBasePtr;  // pointer to sparse matrix at
                                 //  (collector prime,base)

    // This provides a means to back up and restore a known-good
    // state.
    void *BJTbacking;
    void backup(DEV_BKMODE m)
        {
            if (m == DEV_SAVE) {
                if (!BJTbacking)
                    BJTbacking = new char[sizeof(sBJTinstance)];
                memcpy(BJTbacking, this, sizeof(sBJTinstance));
            }
            else if (m == DEV_RESTORE) {
                if (BJTbacking)
                    memcpy(this, BJTbacking, sizeof(sBJTinstance));
            }
            else {
                // DEV_CLEAR
                delete [] (char*)BJTbacking;
                BJTbacking = 0;
            }
        }

// distortion stuff
// the following naming convention is used:
// x = vbe
// y = vbc
// z = vbb
// w = vbed (vbe delayed for the linear gm delay)
// therefore ic_xyz stands for the coefficient of the vbe*vbc*vbb
// term in the multidimensional Taylor expansion for ic; and ibb_x2y
// for the coeff. of the vbe*vbe*vbc term in the ibb expansion.

#define BJTNDCOEFFS 65

#ifndef NODISTO
    double BJTdCoeffs[BJTNDCOEFFS];
#else
    double *BJTdCoeffs;
#endif

#ifdef DISTO
#define    ic_x        BJTdCoeffs[0]
#define    ic_y        BJTdCoeffs[1]
#define    ic_xd       BJTdCoeffs[2]
#define    ic_x2       BJTdCoeffs[3]
#define    ic_y2       BJTdCoeffs[4]
#define    ic_w2       BJTdCoeffs[5]
#define    ic_xy       BJTdCoeffs[6]
#define    ic_yw       BJTdCoeffs[7]
#define    ic_xw       BJTdCoeffs[8]
#define    ic_x3       BJTdCoeffs[9]
#define    ic_y3       BJTdCoeffs[10]
#define    ic_w3       BJTdCoeffs[11]
#define    ic_x2w      BJTdCoeffs[12]
#define    ic_x2y      BJTdCoeffs[13]
#define    ic_y2w      BJTdCoeffs[14]
#define    ic_xy2      BJTdCoeffs[15]
#define    ic_xw2      BJTdCoeffs[16]
#define    ic_yw2      BJTdCoeffs[17]
#define    ic_xyw      BJTdCoeffs[18]

#define    ib_x        BJTdCoeffs[19]
#define    ib_y        BJTdCoeffs[20]
#define    ib_x2       BJTdCoeffs[21]
#define    ib_y2       BJTdCoeffs[22]
#define    ib_xy       BJTdCoeffs[23]
#define    ib_x3       BJTdCoeffs[24]
#define    ib_y3       BJTdCoeffs[25]
#define    ib_x2y      BJTdCoeffs[26]
#define    ib_xy2      BJTdCoeffs[27]

#define    ibb_x       BJTdCoeffs[28]
#define    ibb_y       BJTdCoeffs[29]
#define    ibb_z       BJTdCoeffs[30]
#define    ibb_x2      BJTdCoeffs[31]
#define    ibb_y2      BJTdCoeffs[32]
#define    ibb_z2      BJTdCoeffs[33]
#define    ibb_xy      BJTdCoeffs[34]
#define    ibb_yz      BJTdCoeffs[35]
#define    ibb_xz      BJTdCoeffs[36]
#define    ibb_x3      BJTdCoeffs[37]
#define    ibb_y3      BJTdCoeffs[38]
#define    ibb_z3      BJTdCoeffs[39]
#define    ibb_x2z     BJTdCoeffs[40]
#define    ibb_x2y     BJTdCoeffs[41]
#define    ibb_y2z     BJTdCoeffs[42]
#define    ibb_xy2     BJTdCoeffs[43]
#define    ibb_xz2     BJTdCoeffs[44]
#define    ibb_yz2     BJTdCoeffs[45]
#define    ibb_xyz     BJTdCoeffs[46]

#define    qbe_x       BJTdCoeffs[47]
#define    qbe_y       BJTdCoeffs[48]
#define    qbe_x2      BJTdCoeffs[49]
#define    qbe_y2      BJTdCoeffs[50]
#define    qbe_xy      BJTdCoeffs[51]
#define    qbe_x3      BJTdCoeffs[52]
#define    qbe_y3      BJTdCoeffs[53]
#define    qbe_x2y     BJTdCoeffs[54]
#define    qbe_xy2     BJTdCoeffs[55]

#define    capbc1      BJTdCoeffs[56]
#define    capbc2      BJTdCoeffs[57]
#define    capbc3      BJTdCoeffs[58]

#define    capbx1      BJTdCoeffs[59]
#define    capbx2      BJTdCoeffs[60]
#define    capbx3      BJTdCoeffs[61]

#define    capsc1      BJTdCoeffs[62]
#define    capsc2      BJTdCoeffs[63]
#define    capsc3      BJTdCoeffs[64]
#endif


// indices to array of BJT noise sources

#define BJTRCNOIZ    0
#define BJTRBNOIZ    1
#define BJT_RE_NOISE 2
#define BJTICNOIZ    3
#define BJTIBNOIZ    4
#define BJTFLNOIZ    5
#define BJTTOTNOIZ   6

#define BJTNSRCS     7     // the number of BJT noise sources

#ifndef NONOISE
      double BJTnVar[NSTATVARS][BJTNSRCS];
#else
      double **BJTnVar;
#endif
// the above to avoid allocating memory when it is not needed


    unsigned BJToff         :1;  // 'off' flag for bjt
    unsigned BJTtempGiven   :1;  // temperature given  for bjt instance
    unsigned BJTareaGiven   :1;  // flag to indicate area was specified
    unsigned BJTicVBEGiven  :1;  // flag to indicate VBE init. cond. given
    unsigned BJTicVCEGiven  :1;  // flag to indicate VCE init. cond. given

};


// entries in the state vector for bjt:
#define BJTvbe       GENstate
#define BJTvbc       GENstate+1
#define BJTvbx       GENstate+2
#define BJTvcs       GENstate+3
#define BJTqbe       GENstate+4
#define BJTcqbe      GENstate+5
#define BJTqbc       GENstate+6
#define BJTcqbc      GENstate+7
#define BJTqcs       GENstate+8
#define BJTcqcs      GENstate+9
#define BJTqbx       GENstate+10
#define BJTcqbx      GENstate+11
#define BJTcexbc     GENstate+12
// for BJTask()
#define BJTa_cc      GENstate+13
#define BJTa_cb      GENstate+14
#define BJTa_gpi     GENstate+15
#define BJTa_gmu     GENstate+16
#define BJTa_gm      GENstate+17
#define BJTa_go      GENstate+18
#define BJTa_gx      GENstate+19
#define BJTa_cexbc   GENstate+20
#define BJTa_geqcb   GENstate+21
#define BJTa_gccs    GENstate+22
#define BJTa_geqbx   GENstate+23
#define BJTa_capbe   GENstate+24
#define BJTa_capbc   GENstate+25
#define BJTa_capbx   GENstate+26
#define BJTa_capcs   GENstate+27

#define BJTnumStates 28


struct sBJTmodel : public sGENmodel
{
    sBJTmodel()         { memset(this, 0, sizeof(sBJTmodel)); }
    sBJTmodel *next()   { return (static_cast<sBJTmodel*>(GENnextModel)); }
    sBJTinstance *inst() { return (static_cast<sBJTinstance*>(GENinstances)); }

    int BJTtype;
    double BJTtnom;            // nominal temperature
    double BJTsatCur;          // input - don't use
    double BJTbetaF;           // input - don't use
    double BJTemissionCoeffF;
    double BJTearlyVoltF;
    double BJTrollOffF;
    double BJTleakBEcurrent;   // input - don't use
    double BJTleakBEemissionCoeff;
    double BJTbetaR;           // input - don't use
    double BJTemissionCoeffR;
    double BJTearlyVoltR;
    double BJTrollOffR;
    double BJTleakBCcurrent;   // input - don't use
    double BJTleakBCemissionCoeff;
    double BJTbaseResist;
    double BJTbaseCurrentHalfResist;
    double BJTminBaseResist;
    double BJTemitterResist;
    double BJTcollectorResist;
    double BJTdepletionCapBE;  // input - don't use
    double BJTpotentialBE;     // input - don't use
    double BJTjunctionExpBE;
    double BJTtransitTimeF;
    double BJTtransitTimeBiasCoeffF;
    double BJTtransitTimeFVBC;
    double BJTtransitTimeHighCurrentF;
    double BJTexcessPhase;
    double BJTdepletionCapBC;  // input - don't use
    double BJTpotentialBC;     // input - don't use
    double BJTjunctionExpBC;
    double BJTbaseFractionBCcap;
    double BJTtransitTimeR;
    double BJTcapCS;
    double BJTpotentialSubstrate;
    double BJTexponentialSubstrate;
    double BJTbetaExp;
    double BJTenergyGap;
    double BJTtempExpIS;
    double BJTdepletionCapCoeff;
    double BJTfNcoef;
    double BJTfNexp;
    
    double BJTinvEarlyVoltF;    // inverse of BJTearlyVoltF
    double BJTinvEarlyVoltR;    // inverse of BJTearlyVoltR
    double BJTinvRollOffF;      // inverse of BJTrollOffF
    double BJTinvRollOffR;      // inverse of BJTrollOffR
    double BJTcollectorConduct; // collector conductance
    double BJTemitterConduct;   // emitter conductance
    double BJTtransitTimeVBCFactor;
    double BJTexcessPhaseFactor;
    double BJTf2;
    double BJTf3;
    double BJTf6;
    double BJTf7;

    unsigned BJTtnomGiven : 1;
    unsigned BJTsatCurGiven : 1;
    unsigned BJTbetaFGiven : 1;
    unsigned BJTemissionCoeffFGiven : 1;
    unsigned BJTearlyVoltFGiven : 1;
    unsigned BJTrollOffFGiven : 1;
    unsigned BJTleakBEcurrentGiven : 1;
    unsigned BJTleakBEemissionCoeffGiven : 1;
    unsigned BJTbetaRGiven : 1;
    unsigned BJTemissionCoeffRGiven : 1;
    unsigned BJTearlyVoltRGiven : 1;
    unsigned BJTrollOffRGiven : 1;
    unsigned BJTleakBCcurrentGiven : 1;
    unsigned BJTleakBCemissionCoeffGiven : 1;
    unsigned BJTbaseResistGiven : 1;
    unsigned BJTbaseCurrentHalfResistGiven : 1;
    unsigned BJTminBaseResistGiven : 1;
    unsigned BJTemitterResistGiven : 1;
    unsigned BJTcollectorResistGiven : 1;
    unsigned BJTdepletionCapBEGiven : 1;
    unsigned BJTpotentialBEGiven : 1;
    unsigned BJTjunctionExpBEGiven : 1;
    unsigned BJTtransitTimeFGiven : 1;
    unsigned BJTtransitTimeBiasCoeffFGiven : 1;
    unsigned BJTtransitTimeFVBCGiven : 1;
    unsigned BJTtransitTimeHighCurrentFGiven : 1;
    unsigned BJTexcessPhaseGiven : 1;
    unsigned BJTdepletionCapBCGiven : 1;
    unsigned BJTpotentialBCGiven : 1;
    unsigned BJTjunctionExpBCGiven : 1;
    unsigned BJTbaseFractionBCcapGiven : 1;
    unsigned BJTtransitTimeRGiven : 1;
    unsigned BJTcapCSGiven : 1;
    unsigned BJTpotentialSubstrateGiven : 1;
    unsigned BJTexponentialSubstrateGiven : 1;
    unsigned BJTbetaExpGiven : 1;
    unsigned BJTenergyGapGiven : 1;
    unsigned BJTtempExpISGiven : 1;
    unsigned BJTdepletionCapCoeffGiven : 1;
    unsigned BJTfNcoefGiven : 1;
    unsigned BJTfNexpGiven :1;
};

} // namespace BJT
using namespace BJT;


#ifndef NPN
#define NPN 1
#define PNP -1
#endif

// device parameters
// DO NOT CHANGE THIS without updating aski/seti tables!
enum {
    BJT_AREA = 1,
    BJT_OFF,
    BJT_TEMP,
    BJT_IC_VBE,
    BJT_IC_VCE,
    BJT_IC,
    BJT_QUEST_VBE,
    BJT_QUEST_VBC,
    BJT_QUEST_VCS,
    BJT_QUEST_CC,
    BJT_QUEST_CB,
    BJT_QUEST_CE,
    BJT_QUEST_CS,
    BJT_QUEST_POWER,
    BJT_QUEST_FT,
    BJT_QUEST_GPI,
    BJT_QUEST_GMU,
    BJT_QUEST_GM,
    BJT_QUEST_GO,
    BJT_QUEST_GX,
    BJT_QUEST_GEQCB,
    BJT_QUEST_GCCS,
    BJT_QUEST_GEQBX,
    BJT_QUEST_QBE,
    BJT_QUEST_QBC,
    BJT_QUEST_QCS,
    BJT_QUEST_QBX,
    BJT_QUEST_CQBE,
    BJT_QUEST_CQBC,
    BJT_QUEST_CQCS,
    BJT_QUEST_CQBX,
    BJT_QUEST_CEXBC,
    BJT_QUEST_CPI,
    BJT_QUEST_CMU,
    BJT_QUEST_CBX,
    BJT_QUEST_CCS,
    BJT_QUEST_COLNODE,
    BJT_QUEST_BASENODE,
    BJT_QUEST_EMITNODE,
    BJT_QUEST_SUBSTNODE,
    BJT_QUEST_COLPRIMENODE,
    BJT_QUEST_BASEPRIMENODE,
    BJT_QUEST_EMITPRIMENODE
};

// model parameters
enum {
    BJT_MOD_NPN = 1000,
    BJT_MOD_PNP,
    BJT_MOD_IS,
    BJT_MOD_BF,
    BJT_MOD_NF,
    BJT_MOD_VAF,
    BJT_MOD_IKF,
    BJT_MOD_ISE,
    BJT_MOD_NE,
    BJT_MOD_BR,
    BJT_MOD_NR,
    BJT_MOD_VAR,
    BJT_MOD_IKR,
    BJT_MOD_ISC,
    BJT_MOD_NC,
    BJT_MOD_RB,
    BJT_MOD_IRB,
    BJT_MOD_RBM,
    BJT_MOD_RE,
    BJT_MOD_RC,
    BJT_MOD_CJE,
    BJT_MOD_VJE,
    BJT_MOD_MJE,
    BJT_MOD_TF,
    BJT_MOD_XTF,
    BJT_MOD_VTF,
    BJT_MOD_ITF,
    BJT_MOD_PTF,
    BJT_MOD_CJC,
    BJT_MOD_VJC,
    BJT_MOD_MJC,
    BJT_MOD_XCJC,
    BJT_MOD_TR,
    BJT_MOD_CJS,
    BJT_MOD_VJS,
    BJT_MOD_MJS,
    BJT_MOD_XTB,
    BJT_MOD_EG,
    BJT_MOD_XTI,
    BJT_MOD_FC,
    BJT_MOD_TNOM,
    BJT_MOD_AF,
    BJT_MOD_KF,
    BJT_MOD_INVEARLYF,
    BJT_MOD_INVEARLYR,
    BJT_MOD_INVROLLOFFF,
    BJT_MOD_INVROLLOFFR,
    BJT_MOD_COLCONDUCT,
    BJT_MOD_EMITTERCONDUCT,
    BJT_MOD_TRANSVBCFACT,
    BJT_MOD_EXCESSPHASEFACTOR,
    BJT_MOD_TYPE
};

#endif // BJTDEFS_H

