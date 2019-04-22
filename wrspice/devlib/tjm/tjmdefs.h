
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

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Author: 1992 Stephen R. Whiteley
****************************************************************************/

#ifndef TJMDEFS_H
#define TJMDEFS_H

#include "device.h"
//XXX
//#define PSCAN2

//
// data structures used to describe Jopsephson junctions
//

// Uncomment to support series parasitic inductance.
#define NEWLSER

// Uncomment to support shunt resistor parasitic inductance.
#define NEWLSH

// Use WRspice pre-loading of constant elements.
//XXX#define USE_PRELOAD

#define FABS            fabs
#define REFTEMP         wrsREFTEMP       
#define CHARGE          wrsCHARGE        
#define CONSTCtoK       wrsCONSTCtoK     
#define CONSTboltz      wrsCONSTboltz    
#define CONSTvt0        wrsCONSTvt0      
#define CONSTKoverQ     wrsCONSTKoverQ   
#define CONSTroot2      wrsCONSTroot2    
#define CONSTe          wrsCONSTe        

// Phi0/2*pi
// #define PHI0_2PI        3.291086546e-16
#define PHI0_2PI        wrsCONSTphi0_2pi

namespace TJM {

struct TJMdev : public IFdevice
{
    TJMdev();
    sGENmodel *newModl();
    sGENinstance *newInst();
    int destroy(sGENmodel**);
    int delInst(sGENmodel*, IFuid, sGENinstance*);
    int delModl(sGENmodel**, IFuid, sGENmodel*);

    void parse(int, sCKT*, sLine*);
//    int loadTest(sGENinstance*, sCKT*);   
    int load(sGENinstance*, sCKT*);   
    int setup(sGENmodel*, sCKT*, int*);  
//    int unsetup(sGENmodel*, sCKT*);
    int resetup(sGENmodel*, sCKT*);
//    int temperature(sGENmodel*, sCKT*);    
    int getic(sGENmodel*, sCKT*);  
    int accept(sCKT*, sGENmodel*); 
    int trunc(sGENmodel*, sCKT*, double*);  
//    int convTest(sGENmodel*, sCKT*);  

    int setInst(int, IFdata*, sGENinstance*);  
    int setModl(int, IFdata*, sGENmodel*);   
    int askInst(const sCKT*, const sGENinstance*, int, IFdata*);    
    int askModl(const sGENmodel*, int, IFdata*); 
//    int findBranch(sCKT*, sGENmodel*, IFuid); 

    int acLoad(sGENmodel*, sCKT*); 
//    int pzLoad(sGENmodel*, sCKT*, IFcomplex*); 
//    int disto(int, sGENmodel*, sCKT*);  
    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
//    void initTran(sGENmodel*, double, double);
};

// Maximal number of TJM model Dirichlet series terms.
#define MaxTJMCoeffArraySize 20

struct sTJMinstance : public sGENinstance
{
    sTJMinstance()
        {
            memset(this, 0, sizeof(sTJMinstance));
            GENnumNodes = 3;
        }
    sTJMinstance *next()
        { return (static_cast<sTJMinstance*>(GENnextInstance)); }

#ifdef NEWLSER
    int TJMrealPosNode; // number of model positive node
    int TJMnegNode;     // number of model negative node
    int TJMphsNode;     // number of phase node of junction
    int TJMposNode;     // number of positive node of junction
    int TJMlserBr;      // number of internal branch for series L
#else
    int TJMposNode;     // number of positive node of junction
    int TJMnegNode;     // number of negative node of junction
    int TJMphsNode;     // number of phase node of junction
#endif
#ifdef NEWLSH
    int TJMlshIntNode;  // number of internal shunt node
    int TJMlshBr;       // number of shunt inductor branch
#endif

    int TJMbranch;      // number of control current branch
    IFuid TJMcontrol;   // name of controlling device
    double TJMarea;     // area factor for the junction
    double TJMics;      // area factor = ics/icrit

    double TJMinitCnd[2];   // initial condition vector
#define TJMinitVoltage TJMinitCnd[0]
#define TJMinitPhase TJMinitCnd[1]

    double TJMinitControl;  // initial control current
#ifdef NEWLSER
    double TJMlser;         // parasitic series inductance
    double TJMlserReq;      // stamp Req
    double TJMlserVeq;      // stamp Veq
#endif
#ifdef NEWLSH
    double TJMlsh;          // parasitic shunt res. series inductance
    double TJMlshReq;       // stamp Req
    double TJMlshVeq;       // stamp Veq
#endif
    double TJMdelVdelT;     // dvdt storage

    double          tjm_sinphi2;
    double          tjm_cosphi2;
double          tjm_sinphi_2_old;
double          tjm_cosphi_2_old;
IFcomplex tjm_exp_z[MaxTJMCoeffArraySize];
    IFcomplex       tjm_Fc[MaxTJMCoeffArraySize];
    IFcomplex       tjm_Fs[MaxTJMCoeffArraySize];
    IFcomplex       tjm_Fcprev[MaxTJMCoeffArraySize];
    IFcomplex       tjm_Fsprev[MaxTJMCoeffArraySize];
    IFcomplex       tjm_Fcdt[MaxTJMCoeffArraySize];
    IFcomplex       tjm_Fsdt[MaxTJMCoeffArraySize];
    IFcomplex       tjm_alpha0[MaxTJMCoeffArraySize];
    IFcomplex       tjm_beta0[MaxTJMCoeffArraySize];
    IFcomplex       tjm_alpha1[MaxTJMCoeffArraySize];
//XXX
void tjm_init(double);
void tjm_newstep(sCKT*);;
void tjm_update(double, double*);
void tjm_accept(double);

    // These parameters scale with area
    double TJMcriti;        // junction critical current
    double TJMcap;          // junction capacitance
    double TJMg0;           // junction subgap conductance
    double TJMgn;           // junction normal conductance
    double TJMgs;           // junction step conductance
    double TJMg1;           // NbN model parameter
    double TJMg2;           // NbN model parameter

    double TJMcr1;          // cached constants for qp current
    double TJMcr2;

    double TJMdcrt;         // param to pass to ac load function
    double TJMgqp;          // intrinsic conductance at Vj = 0
    double TJMgshunt;       // shunt conductance if vshunt given
    double TJMnoise;        // noise scale factor

    double *TJMposNegPtr;   // pointer to sparse matrix at (positive, negative)
    double *TJMnegPosPtr;   // pointer to sparse matrix at (negative, positive)
    double *TJMposPosPtr;   // pointer to sparse matrix at (positive, positive)
    double *TJMnegNegPtr;   // pointer to sparse matrix at (negative, negative)
    double *TJMphsPhsPtr;   // pointer to sparse matrix at (phase, phase)
    double *TJMposIbrPtr;   // pointer to sparse matrix at (positive,
                            //  branch equation)
    double *TJMnegIbrPtr;   // pointer to sparse matrix at (negative,
                            //  branch equation)
    // "external" shunt resistor
    double *TJMrshPosPosPtr;
    double *TJMrshPosNegPtr;
    double *TJMrshNegPosPtr;
    double *TJMrshNegNegPtr;

#ifdef NEWLSER
    double *TJMlserPosIbrPtr;   // series inductance MNA stamp
    double *TJMlserNegIbrPtr;
    double *TJMlserIbrPosPtr;
    double *TJMlserIbrNegPtr;
    double *TJMlserIbrIbrPtr;
#endif
#ifdef NEWLSH
    double *TJMlshPosIbrPtr;    // shunt res. series inductance MNA stamp
    double *TJMlshNegIbrPtr;
    double *TJMlshIbrPosPtr;
    double *TJMlshIbrNegPtr;
    double *TJMlshIbrIbrPtr;
#endif

                                // Flags to indicate...
    unsigned TJMareaGiven : 1;      // area was specified
    unsigned TJMicsGiven : 1;       // ics was specified
#ifdef NEWLSER
    unsigned TJMlserGiven : 1;      // lser was specified
#endif
#ifdef NEWLSH
    unsigned TJMlshGiven : 1;       // lsh was specified
#endif
    unsigned TJMinitVoltGiven : 1;  // ic was specified
    unsigned TJMinitPhaseGiven : 1; // ic was specified
    unsigned TJMcontrolGiven : 1;   // control ind or vsource was specified
    unsigned TJMnoiseGiven : 1;     // noise scaling was specified
    unsigned TJMoffGiven : 1;       // "off" was specified

    // for noise analysis
    double TJMnVar[NSTATVARS][2];
};

#define TJMvoltage  GENstate
#define TJMdvdt     GENstate + 1
#define TJMphase    GENstate + 2
#define TJMconI     GENstate + 3
#define TJMphsInt   GENstate + 4
#define TJMcrti     GENstate + 5
#define TJMqpi      GENstate + 6
#ifdef NEWLSER
#define TJMlserFlux GENstate + 7
#define TJMlserVolt GENstate + 8
#ifdef NEWLSH
#define TJMlshFlux  GENstate + 9
#define TJMlshVolt  GENstate + 10
#define TJMnumStates 11
#else
#define TJMnumStates 9
#endif
#else
#ifdef NEWLSH
#define TJMlshFlux  GENstate + 7
#define TJMlshVolt  GENstate + 8
#define TJMnumStates 9
#else
#define TJMnumStates 7
#endif
#endif

struct sTJMmodel : sGENmodel
{
    sTJMmodel()          { memset(this, 0, sizeof(sTJMmodel)); }
    sTJMmodel *next()    { return (static_cast<sTJMmodel*>(GENnextModel)); }
    sTJMinstance *inst() { return (static_cast<sTJMinstance*>(GENinstances)); }

    static double subgap(sTJMmodel*, sTJMinstance*);

    char            *tjm_coeffs;

double tjm_kgap;
double tjm_Rejptilde0;
double tjm_kgap_over_Rejptilde0;
double tjm_a_supp;
double tjm_alphaN;
    double          tjm_wvg;
    double          tjm_wvrat;
    double          tjm_wrrat;
    double          tjm_sgw;
    IFcomplex       *tjm_A;
    IFcomplex       *tjm_B;
    IFcomplex       *tjm_P;
IFcomplex tjm_C[MaxTJMCoeffArraySize];
IFcomplex tjm_D[MaxTJMCoeffArraySize];
    int             tjm_narray;

    int TJMrtype;
    int TJMictype;
    double TJMvg;
    double TJMdelv;
    double TJMcriti;
    double TJMcap;
    double TJMcpic;
    double TJMcmu;
    double TJMvm;
    double TJMr0;
    double TJMicrn;
    double TJMrn;
    double TJMgmu;
    double TJMnoise;
    double TJMccsens;
    double TJMvless;
    double TJMvmore;
    double TJMvdpbak;
    double TJMicFactor;
    double TJMvShunt;
    double TJMtsfact;
#ifdef NEWLSH
    double TJMlsh0;
    double TJMlsh1;
#endif

    unsigned tjm_coeffsGiven : 1;
    unsigned TJMrtypeGiven : 1;
    unsigned TJMpi : 1;
    unsigned TJMpiGiven : 1;
    unsigned TJMictypeGiven : 1;
    unsigned TJMvgGiven : 1;
    unsigned TJMdelvGiven : 1;
    unsigned TJMccsensGiven : 1;
    unsigned TJMvmGiven : 1;
    unsigned TJMr0Given : 1;
    unsigned TJMicrnGiven : 1;
    unsigned TJMrnGiven : 1;
    unsigned TJMgmuGiven : 1;
    unsigned TJMnoiseGiven : 1;
    unsigned TJMcritiGiven : 1;
    unsigned TJMcapGiven : 1;
    unsigned TJMcpicGiven : 1;
    unsigned TJMcmuGiven : 1;
    unsigned TJMicfGiven : 1;
    unsigned TJMvShuntGiven : 1;
    unsigned TJMtsfactGiven : 1;
#ifdef NEWLSH
    unsigned TJMlsh0Given : 1;
    unsigned TJMlsh1Given : 1;
#endif

//XXX
int tjm_init();
};

struct TJMcoeffSet
{
    TJMcoeffSet(const char *name, int size, const IFcomplex *A,
            const IFcomplex *B, const IFcomplex *P)
        {
            cfs_next    = 0;
            cfs_name    = name;
            cfs_A       = A;
            cfs_B       = B;
            cfs_P       = P;
            cfs_size    = size;
        }

    static TJMcoeffSet *getTJMcoeffSet(const char*);

    TJMcoeffSet *cfs_next;
    const char *cfs_name;
    const IFcomplex *cfs_A;
    const IFcomplex *cfs_B;
    const IFcomplex *cfs_P;
    int cfs_size;
};
} // namespace TJM
using namespace TJM;

// device parameters
// DO NOT CHANGE THIS without updating aski/seti tables!
enum {
    TJM_AREA = 1, 
    TJM_ICS,
#ifdef NEWLSER
    TJM_LSER,
#endif
#ifdef NEWLSH
    TJM_LSH,
#endif
    TJM_OFF,
    TJM_IC,
    TJM_ICP,
    TJM_ICV,
    TJM_CON,
    TJM_NOISE,

    TJM_QUEST_V,
    TJM_QUEST_CRT,
    TJM_QUEST_IC,
    TJM_QUEST_IJ,
    TJM_QUEST_IG,
    TJM_QUEST_I,
    TJM_QUEST_CAP,
    TJM_QUEST_G0,
    TJM_QUEST_GN,
    TJM_QUEST_GS,
    TJM_QUEST_G1,
    TJM_QUEST_G2,
    TJM_QUEST_N1,
    TJM_QUEST_N2,
    TJM_QUEST_NP,
#ifdef NEWLSER
    TJM_QUEST_NI,
    TJM_QUEST_NB
#endif
#ifdef NEWLSH
#ifdef NEWLSER
    ,
#endif
    TJM_QUEST_NSHI,
    TJM_QUEST_NSHB
#endif
};

// model parameters
enum {
    TJM_MOD_TJM = 1000,
    TJM_MOD_COEFFS,
    TJM_MOD_PI,
    TJM_MOD_RT,
    TJM_MOD_IC,
    TJM_MOD_VG,
    TJM_MOD_DV,
    TJM_MOD_CRT,
    TJM_MOD_CAP,
    TJM_MOD_CPIC,
    TJM_MOD_CMU,
    TJM_MOD_VM,
    TJM_MOD_R0,
    TJM_MOD_ICR,
    TJM_MOD_RN,
    TJM_MOD_GMU,
    TJM_MOD_NOISE,
    TJM_MOD_CCS,
    TJM_MOD_ICF,
    TJM_MOD_VSHUNT,
#ifdef NEWLSH
    TJM_MOD_LSH0,
    TJM_MOD_LSH1,
#endif
    TJM_MOD_TSFACT,

    TJM_MQUEST_VL,
    TJM_MQUEST_VM,
    TJM_MQUEST_VDP,
    TJM_MQUEST_BETAC,
    TJM_MQUEST_WVG,
    TJM_MQUEST_WVRAT,
    TJM_MQUEST_WRRAT,
};

#endif // TJMDEFS_H

