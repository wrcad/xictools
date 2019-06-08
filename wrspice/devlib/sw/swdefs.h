
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
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Gordon M. Jacobs
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef SWDEFS_H
#define SWDEFS_H

#include "device.h"

//
// structures used to describe switches
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

namespace SW {

struct SWdev : public IFdevice
{
    SWdev();
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
};

struct sSWinstancePOD
{
    int SWposNode;      // number of positive node of switch
    int SWnegNode;      // number of negative node of switch
    int SWposCntrlNode; // positive controlling node of switch
    int SWnegCntrlNode; // negative controlling node of switch

    int SWcontBranch;   // number of branch of controlling current
    IFuid SWcontName;   // name of controlling source

    double *SWposPosptr;  // pointer to sparse matrix diagonal at
                          //  (positive,positive) for switch conductance
    double *SWnegPosptr;  // pointer to sparse matrix offdiagonal at
                          //  (neagtive,positive) for switch conductance
    double *SWposNegptr;  // pointer to sparse matrix offdiagonal at
                          //  (positive,neagtive) for switch conductance
    double *SWnegNegptr;  // pointer to sparse matrix diagonal at
                          //  (neagtive,neagtive) for switch conductance

    double SWcond; // conductance of the switch now

    unsigned SWzero_stateGiven : 1;  // flag to indicate initial state
#ifndef NONOISE
    double SWnVar[NSTATVARS];
#else
    double *SWnVar;
#endif
};

struct sSWinstance : sGENinstance, sSWinstancePOD
{
    sSWinstance() : sGENinstance(), sSWinstancePOD()
        { GENnumNodes = 2; }

    sSWinstance *next()
        { return (static_cast<sSWinstance*>(GENnextInstance)); }
};

// default on conductance = 1 mho
#define SW_ON_CONDUCTANCE 1.0
// default off conductance
#define SW_OFF_CONDUCTANCE ckt->CKTcurTask->TSKgmin
#define SW_NUM_STATES 1   

struct sSWmodelPOD
{
    double SWonResistance;  // switch "on" resistance
    double SWoffResistance; // switch "off" resistance
    double SWvThreshold;    // switching threshold voltage
    double SWvHysteresis;   // switching hysteresis voltage
    double SWiThreshold;    // switching threshold current
    double SWiHysteresis;   // switching hysteresis current
    double SWonConduct;     // switch "on" conductance
    double SWoffConduct;    // switch "off" conductance

    // flags to indicate...
    unsigned SWonGiven : 1;      // on-resistance was given
    unsigned SWoffGiven : 1;     // off-resistance was given
    unsigned SWvThreshGiven : 1; // threshold volt was given
    unsigned SWvHystGiven : 1;   // hysteresis volt was given
    unsigned SWiThreshGiven : 1; // threshold current was given
    unsigned SWiHystGiven : 1;   // hysteresis current was given
};

struct sSWmodel : sGENmodel, sSWmodelPOD
{
    sSWmodel() : sGENmodel(), sSWmodelPOD() { }

    sSWmodel *next()    { return (static_cast<sSWmodel*>(GENnextModel)); }
    sSWinstance *inst() { return (static_cast<sSWinstance*>(GENinstances)); }
};
} // namespace SW
using namespace SW;

// device parameters
// DO NOT CHANGE THIS without updating aski/seti tables!
enum {
    SW_IC_ON = 1,
    SW_IC_OFF,
    SW_IC,
    SW_CONTROL,
    SW_VOLTAGE,
    SW_CURRENT,
    SW_POWER,
    SW_POS_NODE,
    SW_NEG_NODE,
    SW_POS_CONT_NODE,
    SW_NEG_CONT_NODE
};

// model parameters
enum {
    SW_MOD_SW = 1000,
    SW_MOD_CSW,
    SW_MOD_RON,
    SW_MOD_ROFF,
    SW_MOD_VTH,
    SW_MOD_VHYS,
    SW_MOD_ITH,
    SW_MOD_IHYS,
    SW_MOD_GON,
    SW_MOD_GOFF
};

#endif // SWDEFS_H

