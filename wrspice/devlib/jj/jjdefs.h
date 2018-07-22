
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

#ifndef JJDEFS_H
#define JJDEFS_H

#include "device.h"

//
// data structures used to describe Jopsephson junctions
//

#define NEWLSER


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

namespace JJ {

struct JJdev : public IFdevice
{
    JJdev();
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

struct sJJinstance : public sGENinstance
{
    sJJinstance()
        {
            memset(this, 0, sizeof(sJJinstance));
            GENnumNodes = 3;
        }
    sJJinstance *next()
        { return (static_cast<sJJinstance*>(GENnextInstance)); }

#ifdef NEWLSER
    int JJrealPosNode;  // number of model positive node
    int JJnegNode;      // number of model negative node
    int JJphsNode;      // number of phase node of junction
    int JJposNode;      // number of positive node of junction
    int JJlserBr;       // number of internal branch for series L
#else
    int JJposNode;    // number of positive node of junction
    int JJnegNode;    // number of negative node of junction
    int JJphsNode;    // number of phase node of junction
#endif

    int JJbranch;                  // number of control current branch
    IFuid JJcontrol;               // name of controlling device
    double JJarea;                 // area factor for the junction
    double JJics;                  // area factor = ics/icrit

    double JJinitCnd[2];           // initial condition vector
#define JJinitVoltage JJinitCnd[0]
#define JJinitPhase JJinitCnd[1]

    double JJinitControl;          // initial control current
#ifdef NEWLSER
    double JJlser;                 // parasitic series inductance
    double JJlserReq;              // stamp Req
    double JJlserVeq;              // stamp Veq
#endif
    double JJdelVdelT;             // dvdt storage

    // These parameters scale with area
    double JJcriti;                // junction critical current
    double JJcap;                  // junction capacitance
    double JJg0;                   // junction subgap conductance
    double JJgn;                   // junction normal conductance
    double JJgs;                   // junction step conductance
    double JJg1;                   // NbN model parameter
    double JJg2;                   // NbN model parameter

    double JJcr1;                  // cached constants for qp current
    double JJcr2;

    double JJdcrt;                 // param to pass to ac load function
    double JJconduct;              // shunt conductance av Vj = 0
    double JJnoise;                // noise scale factor

    double *JJposNegPtr;           // pointer to sparse matrix at 
                                   //  (positive, negative)
    double *JJnegPosPtr;           // pointer to sparse matrix at 
                                   //  (negative, positive)
    double *JJposPosPtr;           // pointer to sparse matrix at 
                                   //  (positive, positive)
    double *JJnegNegPtr;           // pointer to sparse matrix at 
                                   //  (negative, negative)
    double *JJphsPhsPtr;           // pointer to sparse matrix at 
                                   //  (phase, phase)
    double *JJposIbrPtr;           // pointer to sparse matrix at 
                                   //  (positive, branch equation)
    double *JJnegIbrPtr;           // pointer to sparse matrix at 
                                   //  (negative, branch equation)
#ifdef NEWLSER
    double *JJlPosIbrPtr;          // series inductance MNA stamp
    double *JJlNegIbrPtr;
    double *JJlIbrPosPtr;
    double *JJlIbrNegPtr;
    double *JJlIbrIbrPtr;
#endif

                                   // Flags to indicate...
    unsigned JJareaGiven : 1;      // area was specified
    unsigned JJicsGiven : 1;       // ics was specified
#ifdef NEWLSER
    unsigned JJlserGiven : 1;      // lser was specified
#endif
    unsigned JJinitVoltGiven : 1;  // ic was specified
    unsigned JJinitPhaseGiven : 1; // ic was specified
    unsigned JJcontrolGiven : 1;   // control ind or vsource was specified
    unsigned JJnoiseGiven : 1;     // noise scaling was specified
    unsigned JJoffGiven : 1;       // "off" was specified

    // for noise analysis
    double JJnVar[NSTATVARS][2];
};

#define JJvoltage   GENstate
#define JJdvdt      GENstate + 1
#define JJphase     GENstate + 2
#define JJconI      GENstate + 3
#define JJphsInt    GENstate + 4
#define JJcrti      GENstate + 5
#define JJqpi       GENstate + 6
#ifdef NEWLSER
#define JJlserFlux  GENstate + 7
#define JJlserVolt  GENstate + 8
#define JJnumStates 9
#else
#define JJnumStates 7
#endif

struct sJJmodel : sGENmodel
{
    sJJmodel()          { memset(this, 0, sizeof(sJJmodel)); }
    sJJmodel *next()    { return (static_cast<sJJmodel*>(GENnextModel)); }
    sJJinstance *inst() { return (static_cast<sJJinstance*>(GENinstances)); }

    int JJrtype;
    int JJictype;
    double JJvg;
    double JJdelv;
    double JJcriti;
    double JJcap;
    double JJcmu;
    double JJvm;
    double JJr0;
    double JJicrn;
    double JJrn;
    double JJgmu;
    double JJnoise;
    double JJccsens;
    double JJvless;
    double JJvmore;
    double JJvdpbak;
    double JJicFactor;
    double JJvShunt;
    double JJtsfact;

    unsigned JJrtypeGiven : 1;
    unsigned JJpi : 1;
    unsigned JJpiGiven : 1;
    unsigned JJictypeGiven : 1;
    unsigned JJvgGiven : 1;
    unsigned JJdelvGiven : 1;
    unsigned JJccsensGiven : 1;
    unsigned JJvmGiven : 1;
    unsigned JJr0Given : 1;
    unsigned JJicrnGiven : 1;
    unsigned JJrnGiven : 1;
    unsigned JJgmuGiven : 1;
    unsigned JJnoiseGiven : 1;
    unsigned JJcritiGiven : 1;
    unsigned JJcapGiven : 1;
    unsigned JJcmuGiven : 1;
    unsigned JJicfGiven : 1;
    unsigned JJvShuntGiven : 1;
    unsigned JJtsfactGiven : 1;
};

} // namespace JJ
using namespace JJ;

// device parameters
// DO NOT CHANGE THIS without updating aski/seti tables!
enum {
    JJ_AREA = 1, 
    JJ_ICS,
#ifdef NEWLSER
    JJ_LSER,
#endif
    JJ_OFF,
    JJ_IC,
    JJ_ICP,
    JJ_ICV,
    JJ_CON,
    JJ_NOISE,

    JJ_QUEST_V,
    JJ_QUEST_CRT,
    JJ_QUEST_IC,
    JJ_QUEST_IJ,
    JJ_QUEST_IG,
    JJ_QUEST_I,
    JJ_QUEST_CAP,
    JJ_QUEST_G0,
    JJ_QUEST_GN,
    JJ_QUEST_GS,
    JJ_QUEST_G1,
    JJ_QUEST_G2,
    JJ_QUEST_N1,
    JJ_QUEST_N2,
#ifdef NEWLSER
    JJ_QUEST_NP,
    JJ_QUEST_NI,
    JJ_QUEST_NB
#else
    JJ_QUEST_NP
#endif
};

// model parameters
enum {
    JJ_MOD_JJ = 1000,
    JJ_MOD_PI,
    JJ_MOD_RT,
    JJ_MOD_IC,
    JJ_MOD_VG,
    JJ_MOD_DV,
    JJ_MOD_CRT,
    JJ_MOD_CAP,
    JJ_MOD_CMU,
    JJ_MOD_VM,
    JJ_MOD_R0,
    JJ_MOD_ICR,
    JJ_MOD_RN,
    JJ_MOD_GMU,
    JJ_MOD_NOISE,
    JJ_MOD_CCS,
    JJ_MOD_ICF,
    JJ_MOD_VSHUNT,
    JJ_MOD_TSFACT,

    JJ_MQUEST_VL,
    JJ_MQUEST_VM,
    JJ_MQUEST_VDP
};

#endif // JJDEFS_H

