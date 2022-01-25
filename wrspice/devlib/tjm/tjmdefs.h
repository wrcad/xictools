
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2019 Whiteley Research Inc., all rights reserved.       *
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

#ifndef TJMDEFS_H
#define TJMDEFS_H

#include "device.h"
#include "ifcx.h"

//
// data structures used to describe Jopsephson junctions
//

// Uncomment to support series parasitic inductance.
#define NEWLSER

// Uncomment to support shunt resistor parasitic inductance.
#define NEWLSH

// Use WRspice pre-loading of constant elements.
#define USE_PRELOAD

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

struct sTJMinstancePOD
{
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

    double TJMarea;     // area factor for the junction
    double TJMics;      // area factor = ics/icrit

    double TJMinitCnd[2];   // initial condition vector
#define TJMinitVoltage TJMinitCnd[0]
#define TJMinitPhase TJMinitCnd[1]

    int TJMphsN;        // SFQ pulse count
    int TJMphsF;        // SFQ pulse emission flag
    double TJMphsT;     // SFQpulse emission time

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

    // Core MiTMoJCo implementation (github.com/mitmojco, D. R. Gulevich,
    // ITMO University St. Petersburg 197101, Russia) 
    double tjm_sinphi_2_old;
    double tjm_cosphi_2_old;
    double tjm_gcrit;
    double tjm_cqp;
    double tjm_cp;
    IFcomplex *tjm_Fc;
    IFcomplex *tjm_Fs;
    IFcomplex *tjm_Fcprev;
    IFcomplex *tjm_Fsprev;
    IFcomplex *tjm_alpha0;
    IFcomplex *tjm_alpha1;
    IFcomplex *tjm_exp_z;

    // These parameters scale with area
    double TJMcriti;        // junction critical current
    double TJMcap;          // junction capacitance
    double TJMg0;           // junction subgap conductance

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
    unsigned TJMnoiseGiven : 1;     // noise scaling was specified
    unsigned TJMoffGiven : 1;       // "off" was specified

    // for noise analysis
    double TJMnVar[NSTATVARS][2];
};

#define TJMvoltage  GENstate
#define TJMdvdt     GENstate + 1
#define TJMphase    GENstate + 2
#define TJMphsInt   GENstate + 3
#define TJMcrti     GENstate + 4
#define TJMqpi      GENstate + 5
#ifdef NEWLSER
#define TJMlserFlux GENstate + 6
#define TJMlserVolt GENstate + 7
#ifdef NEWLSH
#define TJMlshFlux  GENstate + 8
#define TJMlshVolt  GENstate + 9
#define TJMnumStates 10
#else
#define TJMnumStates 8
#endif
#else
#ifdef NEWLSH
#define TJMlshFlux  GENstate + 6
#define TJMlshVolt  GENstate + 7
#define TJMnumStates 8
#else
#define TJMnumStates 6
#endif
#endif

struct sTJMinstance : sGENinstance, sTJMinstancePOD
{
    sTJMinstance() : sGENinstance(), sTJMinstancePOD()
        { GENnumNodes = 3; }

    sTJMinstance *next()
        { return (static_cast<sTJMinstance*>(GENnextInstance)); }

    ~sTJMinstance()
        {
            delete [] tjm_Fc;
            // Fs and others are pointers into Fc.
        }

    void tjm_load(sCKT*, struct tjmstuff&);
    void tjm_init(double);
    void tjm_newstep(sCKT*);;
    void tjm_update(double);
    void tjm_accept(double);
};

struct sTJMmodelPOD
{
    // MiTMoJCo core parameters
    char        *tjm_coeffs;
    double      tjm_rejpt;
    double      tjm_kgap;
    double      tjm_kgap_rejpt;
    double      tjm_alphaN;
    IFcomplex   *tjm_p;
    IFcomplex   *tjm_A;
    IFcomplex   *tjm_B;

    int         TJMrtype;
    int         TJMictype;
    double      TJMdel1;
    double      TJMdel2;
    double      TJMvg;
    double      TJMtemp;
    double      TJMtnom;
    double      TJMtc1;
    double      TJMtc2;
    double      TJMtdebye1;
    double      TJMtdebye2;
    double      TJMsmf;
    int         TJMnterms;
    int         TJMnxpts;
    double      TJMthr;
    double      TJMcriti;
    double      TJMcap;
    double      TJMcpic;
    double      TJMcmu;
    double      TJMvm;
    double      TJMr0;
    double      TJMgmu;
    double      TJMnoise;
    double      TJMvdpbak;
    double      TJMomegaJ;
    double      TJMicFactor;
    double      TJMicTempFactor;
    double      TJMvShunt;
    double      TJMtsfact;
    double      TJMtsaccl;
#ifdef NEWLSH
    double      TJMlsh0;
    double      TJMlsh1;
#endif

    unsigned    tjm_coeffsGiven : 1;
    unsigned    TJMrtypeGiven : 1;
    unsigned    TJMictypeGiven : 1;
    unsigned    TJMdel1Given : 1;
    unsigned    TJMdel2Given : 1;
    unsigned    TJMvgGiven : 1;
    unsigned    TJMtempGiven : 1;
    unsigned    TJMtnomGiven : 1;
    unsigned    TJMtc1Given : 1;
    unsigned    TJMtc2Given : 1;
    unsigned    TJMtdebye1Given : 1;
    unsigned    TJMtdebye2Given : 1;
    unsigned    TJMsmfGiven : 1;
    unsigned    TJMntermsGiven : 1;
    unsigned    TJMnxptsGiven : 1;
    unsigned    TJMthrGiven : 1;
    unsigned    TJMvmGiven : 1;
    unsigned    TJMr0Given : 1;
    unsigned    TJMgmuGiven : 1;
    unsigned    TJMnoiseGiven : 1;
    unsigned    TJMcritiGiven : 1;
    unsigned    TJMcapGiven : 1;
    unsigned    TJMcpicGiven : 1;
    unsigned    TJMcmuGiven : 1;
    unsigned    TJMicfGiven : 1;
    unsigned    TJMvShuntGiven : 1;
    unsigned    TJMforceGiven : 1;
    unsigned    TJMtsfactGiven : 1;
    unsigned    TJMtsacclGiven : 1;
#ifdef NEWLSH
    unsigned    TJMlsh0Given : 1;
    unsigned    TJMlsh1Given : 1;
#endif
};

struct sTJMmodel : sGENmodel, sTJMmodelPOD
{
    sTJMmodel() : sGENmodel(), sTJMmodelPOD() { }
    ~sTJMmodel()
        {
            delete [] tjm_p;
            // A and B are pointers into p array
        }

    sTJMmodel *next()    { return (static_cast<sTJMmodel*>(GENnextModel)); }
    sTJMinstance *inst() { return (static_cast<sTJMinstance*>(GENinstances)); }

    int tjm_init();
};

// Tunnel parameters database return.  The coefficients are created
// externally using the MiTMoJCo methodology.
//
struct TJMcoeffSet
{
    TJMcoeffSet(const char *cname, int csize, const IFcomplex *pc,
            const IFcomplex *Ac, const IFcomplex *Bc)
        {
            cfs_next    = 0;
            cfs_name    = cname;
            cfs_p       = pc;
            cfs_A       = Ac;
            cfs_B       = Bc;
            cfs_size    = csize;
        }

    ~TJMcoeffSet()
        {
            delete [] cfs_name;
            delete [] cfs_p;
            delete [] cfs_A;
            delete [] cfs_B;
        }

    static char *fit_fname(double, double, double, double, int, int, double);
    static void check_coeffTab();
    static TJMcoeffSet *getTJMcoeffSet(double, double, double, double, int,
        int, double);
    static TJMcoeffSet *getTJMcoeffSet(const char*);

    TJMcoeffSet *next()             const {return (cfs_next); }
    void set_next(TJMcoeffSet *x)   { cfs_next = x; }
    const char *name()              const {return (cfs_name); }
    const IFcomplex *p()            const {return (cfs_p); }
    const IFcomplex *A()            const {return (cfs_A); }
    const IFcomplex *B()            const {return (cfs_B); }
    int size()                      const {return (cfs_size); }

private:
    TJMcoeffSet *cfs_next;
    const char *cfs_name;
    const IFcomplex *cfs_p;
    const IFcomplex *cfs_A;
    const IFcomplex *cfs_B;
    int cfs_size;

    static sTab<TJMcoeffSet> *TJMcoeffsTab;
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
    TJM_NOISE,

    TJM_QUEST_V,
    TJM_QUEST_PHS,
    TJM_QUEST_PHSN,
    TJM_QUEST_PHSF,
    TJM_QUEST_PHST,
    TJM_QUEST_CRT,
    TJM_QUEST_IC,
    TJM_QUEST_IJ,
    TJM_QUEST_IG,
    TJM_QUEST_I,
    TJM_QUEST_CAP,
    TJM_QUEST_G0,
    TJM_QUEST_GN,
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
    TJM_MOD_RT,
    TJM_MOD_CTP,
    TJM_MOD_DEL1,
    TJM_MOD_DEL2,
    TJM_MOD_VG,
    TJM_MOD_TEMP,
    TJM_MOD_TNOM,
    TJM_MOD_TC,
    TJM_MOD_TC1,
    TJM_MOD_TC2,
    TJM_MOD_TDEBYE,
    TJM_MOD_TDEBYE1,
    TJM_MOD_TDEBYE2,
    TJM_MOD_SMF,
    TJM_MOD_NTERMS,
    TJM_MOD_NXPTS,
    TJM_MOD_THR,
    TJM_MOD_CRT,
    TJM_MOD_CAP,
    TJM_MOD_CPIC,
    TJM_MOD_CMU,
    TJM_MOD_VM,
    TJM_MOD_R0,
    TJM_MOD_GMU,
    TJM_MOD_NOISE,
    TJM_MOD_ICF,
    TJM_MOD_VSHUNT,
    TJM_MOD_FORCE,
#ifdef NEWLSH
    TJM_MOD_LSH0,
    TJM_MOD_LSH1,
#endif
    TJM_MOD_TSFACT,
    TJM_MOD_TSACCL,

    TJM_MQUEST_VDP,
    TJM_MQUEST_OMEGAJ,
    TJM_MQUEST_BETAC,
    TJM_MQUEST_ICTEMPFCT,
    TJM_MQUEST_ALPHAN,
    TJM_MQUEST_KGAP,
    TJM_MQUEST_REJPT,
    TJM_MQUEST_KGAP_REJPT
};

#endif // TJMDEFS_H

