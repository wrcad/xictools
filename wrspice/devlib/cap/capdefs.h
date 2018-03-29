
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
         1993 Stephen R. Whiteley
****************************************************************************/

#ifndef CAPDEFS_H
#define CAPDEFS_H

#include "device.h"

//
// structures used to describe capacitors
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

namespace CAP {

struct CAPdev : public IFdevice
{
    CAPdev();
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
    int temperature(sGENmodel*, sCKT*);    
    int getic(sGENmodel*, sCKT*);  
//    int accept(sCKT*, sGENmodel*); 
    int trunc(sGENmodel*, sCKT*, double*);  
//    int convTest(sGENmodel*, sCKT*);  

    int setInst(int, IFdata*, sGENinstance*);  
    int setModl(int, IFdata*, sGENmodel*);   
    int askInst(const sCKT*, const sGENinstance*, int, IFdata*);    
    int askModl(const sGENmodel*, int, IFdata*); 
//    int findBranch(sCKT*, sGENmodel*, IFuid); 

    int acLoad(sGENmodel*, sCKT*); 
    int pzLoad(sGENmodel*, sCKT*, IFcomplex*); 
//    int disto(int, sGENmodel*, sCKT*);  
//    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
    void initTranFuncs(sGENmodel*, double, double);
};

struct sCAPinstance : public sGENinstance
{
    sCAPinstance()
        {
            memset(this, 0, sizeof(sCAPinstance));
            GENnumNodes = 2;
        }
    ~sCAPinstance()
        {
            delete CAPtree;
            delete [] CAPvalues;
            delete [] CAPeqns;
            delete [] CAPpolyCoeffs;
        }

    sCAPinstance *next()
        { return (static_cast<sCAPinstance*>(GENnextInstance)); }

    int CAPposNode;    // number of positive node of capacitor
    int CAPnegNode;    // number of negative node of capacitor

    double CAPcapac;    // capacitance, after temp adj
    double CAPnomCapac; // nominal capacitance
    double CAPinitCond; // initial capacitor voltage if specified
    double CAPceq;      // storage for ceq, rhs entry
    double CAPgeq;      // stroage for geq, matrix entry
    double CAPwidth;    // width of the capacitor
    double CAPlength;   // length of the capacitor
    double CAPtemp;     // temperature at which this capacitor operates
    double CAPtc1;      // first order temp. coeff.
    double CAPtc2;      // second order temp. coeff.
    double CAPtcFactor; // temperature adj. factor

    // for parse tree support
    IFparseTree *CAPtree;    // large-signal capacitance expression
    double *CAPvalues;       // controlling node voltage values
    int *CAPeqns;            // controlling node indices

    // for POLY
    double *CAPpolyCoeffs;   // array of values
    int CAPpolyNumCoeffs;    // size of array

    double *CAPposPosptr;    // pointer to sparse matrix diagonal at 
                             //  (positive,positive)
    double *CAPnegNegptr;    // pointer to sparse matrix diagonal at 
                             //  (negative,negative)
    double *CAPposNegptr;    // pointer to sparse matrix offdiagonal at 
                             //  (positive,negative)
    double *CAPnegPosptr;    // pointer to sparse matrix offdiagonal at 
                             //  (negative,positive)

    // flags to indicate... 
    unsigned CAPcapGiven    : 1; // capacitance was specified
    unsigned CAPicGiven     : 1; // init. condition was specified
    unsigned CAPwidthGiven  : 1; // capacitor width given
    unsigned CAPlengthGiven : 1; // capacitor length given
    unsigned CAPtempGiven   : 1; // temperature specified
    unsigned CAPtc1Given    : 1; // tc1 specified
    unsigned CAPtc2Given    : 1; // tc2 specified
};

struct sCAPmodel : public sGENmodel
{
    sCAPmodel()         { memset(this, 0, sizeof(sCAPmodel)); }
    sCAPmodel *next()   { return (static_cast<sCAPmodel*>(GENnextModel)); }
    sCAPinstance *inst() { return (static_cast<sCAPinstance*>(GENinstances)); }

    double CAPcj;       // Unit Area Capacitance ( F/ M**2 )
    double CAPcjsw;     // Unit Length Sidewall Capacitance ( F / M )
    double CAPdefWidth; // the default width of a capacitor
    double CAPnarrow;   // amount by which length/width are less than drawn
    double CAPtnom;       // temperature at which capacitance measured
    double CAPtempCoeff1; // first temperature coefficient of capacitors
    double CAPtempCoeff2; // second temperature coefficient of capacitors

    unsigned CAPcjGiven : 1;       // Unit Area Capacitance ( F/ M**2 )
    unsigned CAPcjswGiven : 1;     // Unit Length Sidewall Capacitance(F/M)
    unsigned CAPdefWidthGiven : 1; // flag indicates default width given
    unsigned CAPnarrowGiven : 1;   // flag indicates narrowing factor given
    unsigned CAPtnomGiven: 1;      // nominal temp. was given
    unsigned CAPtc1Given : 1;      // tc1 was specified
    unsigned CAPtc2Given : 1;      // tc2 was specified
};

} // namespace CAP
using namespace CAP;


#define CAPqcap GENstate     // charge on the capacitor
#define CAPccap GENstate+1   // current through the capacitor
                            
// device parameters
// DO NOT CHANGE THIS without updating aski/seti tables!
enum {
    CAP_CAP = 1,
    CAP_IC,
    CAP_WIDTH,
    CAP_LENGTH,
    CAP_TEMP,
    CAP_TC1,
    CAP_TC2,
    CAP_POLY,
    CAP_CHARGE,
    CAP_VOLTAGE,
    CAP_CURRENT,
    CAP_POWER,
    CAP_POSNODE,
    CAP_NEGNODE,
    CAP_TREE
};

// model parameters
enum {
    CAP_MOD_CJ = 1000,
    CAP_MOD_CJSW,
    CAP_MOD_DEFWIDTH,
    CAP_MOD_C,
    CAP_MOD_NARROW,
    CAP_MOD_TNOM,
    CAP_MOD_TC1,
    CAP_MOD_TC2
};

#endif // CAPDEFS_H

