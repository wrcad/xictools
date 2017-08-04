
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1987 Kanwar Jit Singh
         1993 Stephen R. Whiteley
****************************************************************************/

#ifndef SRCDEFS_H
#define SRCDEFS_H

#include "device.h"

//
// structures to describe Arbitrary sources
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

// Use WRspice pre-loading of constant elements.
#define USE_PRELOAD

namespace SRC {

struct sSRCinstance;

struct SRCdev : public IFdevice
{
    SRCdev();
    sGENmodel *newModl();
    sGENinstance *newInst();
    int destroy(sGENmodel**);
    int delInst(sGENmodel*, IFuid, sGENinstance*);
    int delModl(sGENmodel**, IFuid, sGENmodel*);

    void parse(int, sCKT*, sLine*);
    int loadTest(sGENinstance*, sCKT*);   
    int load(sGENinstance*, sCKT*);   
    int setup(sGENmodel*, sCKT*, int*);  
    int unsetup(sGENmodel*, sCKT*);
    int resetup(sGENmodel*, sCKT*);
//    int temperature(sGENmodel*, sCKT*);    
//    int getic(sGENmodel*, sCKT*);  
//    int accept(sCKT*, sGENmodel*); 
    int trunc(sGENmodel*, sCKT*, double*);  
    int convTest(sGENmodel*, sCKT*);  

    int setInst(int, IFdata*, sGENinstance*);  
//    int setModl(int, IFdata*, sGENmodel*);   
    int askInst(const sCKT*, const sGENinstance*, int, IFdata*);    
//    int askModl(const sGENmodel*, int, IFdata*); 
    int findBranch(sCKT*, sGENmodel*, IFuid); 

    int acLoad(sGENmodel*, sCKT*); 
    int pzLoad(sGENmodel*, sCKT*, IFcomplex*); 
//    int disto(int, sGENmodel*, sCKT*);  
//    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
    void initTran(sGENmodel*, double, double);
private:
    void src_parse(sLine*, const char**, sCKT*, sGENinstance*,
        const char*, const char*);
};

// Types of functional dependence for load.
enum SRCfuncType {
    SRC_none, SRC_dc, SRC_ccvs, SRC_vcvs, SRC_cccs, SRC_vccs, SRC_func };

struct sSRCinstance : public sGENSRCinstance
{
    sSRCinstance()
        {
            memset(this, 0, sizeof(sSRCinstance));
            GENnumNodes = 2;
        }
    ~sSRCinstance();
    sSRCinstance *next()
        { return (static_cast<sSRCinstance*>(GENnextInstance)); }

    double **SRCposptr; // pointer to pointers of the elements in the sparse
                        // matrix
    SRCfuncType SRCdcFunc;  // DC load function type
    SRCfuncType SRCtranFunc; // TRAN load function type
    IFcomplex SRCcoeff;     // linear gain coefficient
    IFuid SRCcontName;      // pointer to name of controlling instance
    double SRCacVec[2];     // AC magnitude and phase
#define SRCacMag SRCacVec[0]
#define SRCacPhase SRCacVec[1]
    const char *SRCacTabName; // name of AC table
    sCKTtable *SRCacTab;    // pointer to AC table

    // Order is important here, addresses are returned for IF_REALVEC.
    double SRCacReal;       // AC real component
    double SRCacImag;       // AC imaginary component
    double SRCdF1mag;       // distortion f1 magnitude
    double SRCdF1phase;     // distortion f1 phase
    double SRCdF2mag;       // distortion f2 magnitude
    double SRCdF2phase;     // distortion f2 phase

    IFparseTree *SRCtree;   // the parse tree
    double *SRCacValues;    // store rhs and derivatives for ac anal
    double *SRCvalues;      // store input values to function
    double SRCvalue;        // present source value
    double *SRCderivs;      // store partial derivatives
    int *SRCeqns;           // store equation numbers of input values
    double SRCprev;         // previous value for the convergence test

    // For convolution-based transient analysis
    double SRCdeltaFreq;    // frequence increment used in fft
    IFcomplex *SRCfreqResp; // fft vector of impulse response
    IFcomplex *SRCtimeResp; // cumulative response
    IFcomplex *SRCworkArea; // interim results
    int SRCfreqRespLen;     // length of vectors above

    bool SRCgetFreqResp(sCKT*);
    bool SRCdo_fft(sCKT*, double, double*);
    // 

    double *SRCposContPosptr; // pointer to sparse matrix element at 
                              //  (positive node, control positive node)
    double *SRCposContNegptr; // pointer to sparse matrix element at 
                              //  (positive node, control negative node)
    double *SRCnegContPosptr; // pointer to sparse matrix element at 
                              //  (negative node, control positive node)
    double *SRCnegContNegptr; // pointer to sparse matrix element at 
                              //  (negative node, control negative node)
    double *SRCposContBrptr;  // pointer to sparse matrix element at 
                              //  (positive node, control branch eq)
    double *SRCnegContBrptr;  // pointer to sparse matrix element at 
                              //  (negative node, control branch eq)
    double *SRCibrContPosptr; // pointer to sparse matrix element at 
                              //  (branch equation, control positive node)
    double *SRCibrContNegptr; // pointer to sparse matrix element at 
                              //  (branch equation, control negative node)
    double *SRCibrContBrptr;  // pointer to sparse matrix element at 
                              //  (branch equation, control branch eq)
    double *SRCposIbrptr;     // pointer to sparse matrix element at
                              //  (positive node, branch equation)
    double *SRCnegIbrptr;     // pointer to sparse matrix element at
                              //  (negative node, branch equation)
    double *SRCibrPosptr;     // pointer to sparse matrix element at
                              //  (branch equation, positive node)
    double *SRCibrNegptr;     // pointer to sparse matrix element at
                              //  (branch equation, negative node)
    double *SRCibrIbrptr;     // pointer to sparse matrix element at
                              //  (branch equation, branch equation)

    // flags to indicate:
    unsigned SRCdcGiven      :1; // dc value given
    unsigned SRCacGiven      :1; // ac keyword given
    unsigned SRCacMGiven     :1; // ac magnitude given
    unsigned SRCacPGiven     :1; // ac phase given
    unsigned SRCdGiven       :1; // source is a disto input
    unsigned SRCdF1given     :1; // source is an f1 dist input
    unsigned SRCdF2given     :1; // source is an f2 dist input
    unsigned SRCccCoeffGiven :1; // current controlled gain coeff given
    unsigned SRCvcCoeffGiven :1; // voltage controlled gain coeff given
};

struct sSRCmodel : public sGENmodel
{
    sSRCmodel()         { memset(this, 0, sizeof(sSRCmodel)); }
    sSRCmodel *next()   { return (static_cast<sSRCmodel*>(GENnextModel)); }
    sSRCinstance *inst() { return (static_cast<sSRCinstance*>(GENinstances)); }
};

} // namespace SRC
using namespace SRC;

#define SRC_CC GENSRC_CC
#define SRC_VC GENSRC_VC

// device parameters
// DO NOT CHANGE THIS without updating aski/seti tables!
enum {
    SRC_I = GENSRC_I,  // GENSRC_I = 1
    SRC_V = GENSRC_V,
    SRC_DEP,
    SRC_DC,
    SRC_AC,
    SRC_AC_MAG,
    SRC_AC_PHASE,
    SRC_AC_REAL,
    SRC_AC_IMAG,
    SRC_FUNC,
    SRC_D_F1,
    SRC_D_F2,
    SRC_GAIN,
    SRC_CONTROL,
    SRC_VOLTAGE,
    SRC_CURRENT,
    SRC_POWER,
    SRC_POS_NODE,
    SRC_NEG_NODE,
    SRC_CONT_P_NODE,
    SRC_CONT_N_NODE,
    SRC_BR_NODE
};

#endif // SRCDEFS_H

