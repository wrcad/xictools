
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

#ifndef RESDEFS_H
#define RESDEFS_H

#include "device.h"


#define FABS            fabs
#define REFTEMP         wrsREFTEMP       
#define CHARGE          wrsCHARGE        
#define CONSTCtoK       wrsCONSTCtoK     
#define CONSTboltz      wrsCONSTboltz    
#define CONSTvt0        wrsCONSTvt0      
#define CONSTKoverQ     wrsCONSTKoverQ   
#define CONSTroot2      wrsCONSTroot2    
#define CONSTe          wrsCONSTe        

//
// definitions used to describe resistors
//

// Use WRspice pre-loading of constant elements when possible.
#define USE_PRELOAD

// Minimum resistance allowed.
#define RES_RMIN 1e-5

namespace RES {

struct RESdev : public IFdevice
{
    RESdev();
    sGENmodel *newModl();
    sGENinstance *newInst();
    int destroy(sGENmodel**);
    int delInst(sGENmodel*, IFuid, sGENinstance*);
    int delModl(sGENmodel**, IFuid, sGENmodel*);

    void parse(int, sCKT*, sLine*);
    int loadTest(sGENinstance*, sCKT*);   
    int load(sGENinstance*, sCKT*);   
    int setup(sGENmodel*, sCKT*, int*);  
//    int unsetup(sGENmodel*, sCKT*);
    int resetup(sGENmodel*, sCKT*);
    int temperature(sGENmodel*, sCKT*);    
//    int getic(sGENmodel*, sCKT*);  
//    int accept(sCKT*, sGENmodel*); 
//    int trunc(sGENmodel*, sCKT*, double*);  
//    int convTest(sGENmodel*, sCKT*);  

    int setInst(int, IFdata*, sGENinstance*);  
    int setModl(int, IFdata*, sGENmodel*);   
    int askInst(const sCKT*, const sGENinstance*, int, IFdata*);    
    int askModl(const sGENmodel*, int, IFdata*); 
//    int findBranch(sCKT*, sGENmodel*, IFuid); 

    int acLoad(sGENmodel*, sCKT*); 
    int pzLoad(sGENmodel*, sCKT*, IFcomplex*); 
//    int disto(int, sGENmodel*, sCKT*);  
    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
    void initTranFuncs(sGENmodel*, double, double);
};

struct sRESinstancePOD
{
    int RESposNode;     // number of positive node of resistor
    int RESnegNode;     // number of negative node of resistor

    double REStemp;     // temperature at which this resistor operates
    double RESconduct;  // conductance at current analysis temperature and M
    double RESresist;   // resistance at temperature Tnom, M==1
    double RESwidth;    // width of the resistor
    double RESlength;   // length of the resistor
    double REStc1;      // first order temp. coeff.
    double REStc2;      // second order temp. coeff.
    double REStcFactor; // temperature correction factor
    double RESv;        // voltage across resistor
    double RESnoise;    // noise scale factor
    double RESm;        // conductance scale factor

    // for parse tree support
    IFparseTree *REStree;    // large-signal resistance expression
    double *RESvalues;       // controlling node voltage values
    double *RESacValues;     // ac loading parameters
    double *RESderivs;       // controlling node voltage derivatives
    int *RESeqns;            // controlling node indices

    // for POLY
    double *RESpolyCoeffs;   // array of values
    int RESpolyNumCoeffs;    // size of array

    double **RESposptr;      // pointer to pointers of the elements in the
                             // sparse matrix

    double *RESposPosptr;    // pointer to sparse matrix diagonal at 
                             //  (positive,positive)
    double *RESnegNegptr;    // pointer to sparse matrix diagonal at 
                             //  (negative,negative)
    double *RESposNegptr;    // pointer to sparse matrix offdiagonal at 
                             //  (positive,negative)
    double *RESnegPosptr;    // pointer to sparse matrix offdiagonal at 
                             //  (negative,positive)

    // flag to indicate...
    unsigned RESresGiven    : 1;  // resistance was specified
    unsigned RESwidthGiven  : 1;  // width given
    unsigned RESlengthGiven : 1;  // length given
    unsigned REStempGiven   : 1;  // temperature specified
    unsigned REStc1Given    : 1;  // tc1 specified
    unsigned REStc2Given    : 1;  // tc2 specified
    unsigned RESnoiseGiven  : 1;  // noise scale factor given
    unsigned RESmGiven      : 1;  // value scale factor given

    unsigned RESusePreload;

    // for noise analysis
    double RESnVar[NSTATVARS][2];
};

struct sRESinstance : sGENinstance, sRESinstancePOD
{
    sRESinstance() : sGENinstance(), sRESinstancePOD()
        { GENnumNodes = 2; }
    ~sRESinstance()
        {
            delete REStree;
            delete [] RESvalues;
            delete [] RESderivs;
            delete [] RESeqns;
            delete [] RESposptr;
            delete [] RESpolyCoeffs;
        }

    sRESinstance *next()
        { return (static_cast<sRESinstance*>(GENnextInstance)); }
};

struct sRESmodelPOD
{
    double REStnom;       // temperature at which resistance measured
    double REStemp;       // default operating temperature for instances
    double REStempCoeff1; // first temperature coefficient of resistors
    double REStempCoeff2; // second temperature coefficient of resistors
    double RESsheetRes;   // sheet resistance of devices in ohms/square
    double RESdefWidth;   // default width of a resistor
    double RESdefLength;  // default length of a resistor
    double RESnarrow;     // amount by which device is narrower than drawn
    double RESshorten;    // amount by which device is shorter than drawn
    double RESnoise;      // voise scale factor
    double RESm;          // conductance scale factor

    // flicker noise model
    double RESkf;         // flicker noise coefficient
    double RESaf;         // exponent of current
    double RESef;         // exponent of frequency
    double RESwf;         // exponent of width
    double RESlf;         // exponent of length

    // flag to indicate...
    unsigned REStnomGiven       : 1; // nominal temp. was given
    unsigned REStempGiven       : 1; // default temp. was given
    unsigned REStc1Given        : 1; // tc1 was specified
    unsigned REStc2Given        : 1; // tc2 was specified
    unsigned RESsheetResGiven   : 1; // sheet resistance given
    unsigned RESdefWidthGiven   : 1; // default width given
    unsigned RESdefLengthGiven  : 1; // default length given
    unsigned RESnarrowGiven     : 1; // narrow effect given
    unsigned RESshortenGiven    : 1; // shorten given
    unsigned RESkfGiven         : 1; // flicker noise coeff. given
    unsigned RESafGiven         : 1; // current exponent given
    unsigned RESefGiven         : 1; // frequenct exponent given
    unsigned RESwfGiven         : 1; // width exponent given
    unsigned RESlfGiven         : 1; // length exponent given
    unsigned RESnoiseGiven      : 1; // noise scale factor given
    unsigned RESmGiven          : 1; // value scale factor given
};

struct sRESmodel : sGENmodel, sRESmodelPOD
{
    sRESmodel() : sGENmodel(), sRESmodelPOD() { }

    sRESmodel *next()   { return (static_cast<sRESmodel*>(GENnextModel)); }
    sRESinstance *inst() { return (static_cast<sRESinstance*>(GENinstances)); }
};

#ifdef NEWJJDC
namespace {
    enum { GND, PHASE, VOLT };
    inline int nodetype(sCKT *ckt, int nnum)
    {
        if (nnum <= 0)
            return (GND);
        sCKTnode *nd = ckt->CKTnodeTab.find(nnum);
        if (!nd)
            return (VOLT);  // shouldn't happen
        return (nd->phase() ? PHASE : VOLT);
    }
}
#endif

} // namespace RES
using namespace RES;

// device parameters
// DO NOT CHANGE THIS without updating aski/seti tables!
enum {
    RES_RESIST = 1,
    RES_TEMP,
    RES_WIDTH,
    RES_LENGTH,
    RES_TC1,
    RES_TC2,
    RES_NOISE,
    RES_M,
    RES_POLY,
    RES_CONDUCT,
    RES_VOLTAGE,
    RES_CURRENT,
    RES_POWER,
    RES_POSNODE,
    RES_NEGNODE,
    RES_TREE
};

// model parameters
enum {
    RES_MOD_R = 1000,
    RES_MOD_RSH,
    RES_MOD_NARROW,
    RES_MOD_DL,
    RES_MOD_TC1,
    RES_MOD_TC2,
    RES_MOD_DEFWIDTH,
    RES_MOD_DEFLENGTH,
    RES_MOD_TNOM,
    RES_MOD_TEMP,
    RES_MOD_NOISE,
    RES_MOD_M,
    RES_MOD_KF,
    RES_MOD_AF,
    RES_MOD_EF,
    RES_MOD_WF,
    RES_MOD_LF
};

#endif // RESDEFS_H

