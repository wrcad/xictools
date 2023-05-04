
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
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef INDDEFS_H
#define INDDEFS_H

#include "device.h"

//
// structures used to describe inductors
//


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

namespace IND {

struct INDdev : public IFdevice
{
    INDdev();
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
//    int temperature(sGENmodel*, sCKT*);    
    int getic(sGENmodel*, sCKT*);  
//    int accept(sCKT*, sGENmodel*); 
    int trunc(sGENmodel*, sCKT*, double*);  
//    int convTest(sGENmodel*, sCKT*);  

    int setInst(int, IFdata*, sGENinstance*);  
    int setModl(int, IFdata*, sGENmodel*);   
    int askInst(const sCKT*, const sGENinstance*, int, IFdata*);    
    int askModl(const sGENmodel*, int, IFdata*); 
    int findBranch(sCKT*, sGENmodel*, IFuid); 

    int acLoad(sGENmodel*, sCKT*); 
    int pzLoad(sGENmodel*, sCKT*, IFcomplex*); 
//    int disto(int, sGENmodel*, sCKT*);  
//    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
    void initTranFuncs(sGENmodel*, double, double);
};

struct sINDinstancePOD
{
    int INDposNode;   // number of positive node of inductor
    int INDnegNode;   // number of negative node of inductor

    int INDbrEq; // number of the branch equation added for current

    double INDnomInduct;  // inductance as given
    double INDinitCond;   // initial inductor voltage if specified
    double INDm;          // device multiplier
    double INDinduct;     // inductance, scaled
    double INDveq;        // storage for veq
    double INDreq;        // storage for req
    double INDprevFlux;   // previous total flux
    double INDres;        // linear series resistance

    // For parse tree support
    IFparseTree *INDtree; // large signal inductance expression
    double *INDvalues;    // controlling values
    int *INDeqns;         // controlling node/branch indices

    // for POLY
    double *INDpolyCoeffs;   // array of values
    int INDpolyNumCoeffs;    // size of array

    double *INDposIbrptr; // pointer to sparse matrix diagonal at
                          //  (positive,branch eq)
    double *INDnegIbrptr; // pointer to sparse matrix diagonal at
                          //  (negative,branch eq)
    double *INDibrNegptr; // pointer to sparse matrix offdiagonal at
                          //  (branch eq,negative)
    double *INDibrPosptr; // pointer to sparse matrix offdiagonal at
                          //  (branch eq,positive)
    double *INDibrIbrptr; // pointer to sparse matrix offdiagonal at
                          //  (branch eq,branch eq)

    unsigned INDindGiven : 1;   // inductance was specified
    unsigned INDicGiven  : 1;   // init. cond. was specified
    unsigned INDmGiven   : 1;   // device multiplier was specified
    unsigned INDresGiven : 1;   // resistance was given
};

struct sINDinstance : sGENinstance, sINDinstancePOD
{
    sINDinstance() : sGENinstance(), sINDinstancePOD()
        { GENnumNodes = 2; }
    ~sINDinstance()
        {
            delete INDtree;
            delete [] INDvalues;
            delete [] INDeqns;
            delete [] INDpolyCoeffs;
        }

    sINDinstance *next()
        { return (static_cast<sINDinstance*>(GENnextInstance)); }
};

struct sINDmodelPOD
{

    double INDm;          // device multiplier

    unsigned INDmGiven   : 1;   // device multiplier was specified
};

struct sINDmodel : sGENmodel, sINDmodelPOD
{
    sINDmodel() : sGENmodel(), sINDmodelPOD() { }

    sINDmodel *next()   { return (static_cast<sINDmodel*>(GENnextModel)); }
    sINDinstance *inst() { return (static_cast<sINDinstance*>(GENinstances)); }
};

#define INDflux GENstate   // flux in the inductor
#define INDvolt GENstate+1 // voltage - save an entry in table

struct MUTdev : public IFdevice
{
    MUTdev();
    sGENmodel *newModl();
    sGENinstance *newInst();
    int destroy(sGENmodel**);
    int delInst(sGENmodel*, IFuid, sGENinstance*);
    int delModl(sGENmodel**, IFuid, sGENmodel*);

    void parse(int, sCKT*, sLine*);
//    int loadTest(sGENinstance*, sCKT*);
    int load(sGENinstance*, sCKT*);
//    int setup(sGENmodel*, sCKT*, int*);  
    int resetup(sGENmodel*, sCKT*);
//    int temperature(sGENmodel*, sCKT*);    
//    int getic(sGENmodel*, sCKT*);  
//    int accept(sCKT*, sGENmodel*); 
//    int trunc(sGENmodel*, sCKT*, double*);  
//    int convTest(sGENmodel*, sCKT*);  

    int setInst(int, IFdata*, sGENinstance*);  
//    int setModl(int, IFdata*, sGENmodel*);   
    int askInst(const sCKT*, const sGENinstance*, int, IFdata*);    
//    int askModl(const sGENmodel*, int, IFdata*); 
//    int findBranch(sCKT*, sGENmodel*, IFuid); 

    int acLoad(sGENmodel*, sCKT*); 
    int pzLoad(sGENmodel*, sCKT*, IFcomplex*); 
//    int disto(int, sGENmodel*, sCKT*);  
//    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
};

struct sMUTinstancePOD
{
    double MUTcoupling;    // mutual inductance input by user
    double MUTfactor;      // mutual inductance scaled for internal use
    IFuid MUTindName1;     // name of coupled inductor 1
    IFuid MUTindName2;     // name of coupled inductor 2
    sINDinstance *MUTind1; // pointer to coupled inductor 1
    sINDinstance *MUTind2; // pointer to coupled inductor 2
    double *MUTbr1br2;     // pointers to off-diagonal intersections of
    double *MUTbr2br1;     // current branch equations in matrix

    unsigned MUTindGiven : 1; // flag to indicate inductance was specified
};

struct sMUTinstance : sGENinstance, sMUTinstancePOD
{
    sMUTinstance() : sGENinstance(), sMUTinstancePOD() { }

    sMUTinstance *next()
        { return (static_cast<sMUTinstance*>(GENnextInstance)); }
};

struct sMUTmodel : public sGENmodel
{
    sMUTmodel() : sGENmodel() { }

    sMUTmodel *next()   { return (static_cast<sMUTmodel*>(GENnextModel)); }
    sMUTinstance *inst() { return (static_cast<sMUTinstance*>(GENinstances)); }
};
} // namespace IND
using namespace IND;

// device parameters
// DO NOT CHANGE THIS without updating aski/seti tables!
enum {
    IND_IND = 1,
    IND_IC,
    IND_M,
    IND_RES,
    IND_POLY,
    IND_FLUX,
    IND_VOLT,
    IND_CURRENT,
    IND_POWER,
    IND_POSNODE,
    IND_NEGNODE,
    IND_TREE,
    MUT_COEFF,
    MUT_FACTOR,
    MUT_IND1,
    MUT_IND2,
    IND_NOTUSED
};

// model parameters
enum {
    IND_MOD_L = 1000,
    IND_MOD_M
};
#endif // INDDEFS_H

