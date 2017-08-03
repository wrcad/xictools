
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

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
Modified by Dietmar Warning 2003 and Paolo Nenzi 2003
**********/

#ifndef DIODEFS_H
#define DIODEFS_H

#include "device.h"

//
// data structures used to describe diodes */
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

namespace DIO {

struct sDIOmodel;

struct DIOdev : public IFdevice
{
    DIOdev();
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
    int dSetup(sDIOmodel*, sCKT*);
};

struct sDIOinstance : public sGENinstance
{
    sDIOinstance()
        {
            memset(this, 0, sizeof(sDIOinstance));
            GENnumNodes = 2;
        }
    sDIOinstance *next()
        { return (static_cast<sDIOinstance*>(GENnextInstance)); }
    void ac_cd(const sCKT*, double*, double*) const;

    int DIOposNode;     /* number of positive node of diode */
    int DIOnegNode;     /* number of negative node of diode */
    int DIOposPrimeNode;    /* number of positive prime node of diode */
    int DIOowner;  /* number of owner process */

    double *DIOposPosPrimePtr;      /* pointer to sparse matrix at 
                                     * (positive,positive prime) */
    double *DIOnegPosPrimePtr;      /* pointer to sparse matrix at 
                                     * (negative,positive prime) */
    double *DIOposPrimePosPtr;      /* pointer to sparse matrix at 
                                     * (positive prime,positive) */
    double *DIOposPrimeNegPtr;      /* pointer to sparse matrix at 
                                     * (positive prime,negative) */
    double *DIOposPosPtr;   /* pointer to sparse matrix at 
                             * (positive,positive) */
    double *DIOnegNegPtr;   /* pointer to sparse matrix at 
                             * (negative,negative) */
    double *DIOposPrimePosPrimePtr; /* pointer to sparse matrix at 
                                     * (positive prime,positive prime) */

    // This provides a means to back up and restore a known-good
    // state.
    void *DIObacking;
    void backup(DEV_BKMODE m)
        {
            if (m == DEV_SAVE) {
                if (!DIObacking)
                    DIObacking = new char[sizeof(sDIOinstance)];
                memcpy(DIObacking, this, sizeof(sDIOinstance));
            }
            else if (m == DEV_RESTORE) {
                if (DIObacking)
                    memcpy(this, DIObacking, sizeof(sDIOinstance));
            }
            else {
                // DEV_CLEAR
                delete [] (char*)DIObacking;
                DIObacking = 0;
            }
        }

    double DIOcap;   /* stores the diode capacitance */

    unsigned DIOoff : 1;   /* 'off' flag for diode */
    unsigned DIOareaGiven : 1;   /* flag to indicate area was specified */
    unsigned DIOpjGiven : 1;   /* flag to indicate perimeter was specified */
    unsigned DIOmGiven : 1;   /* flag to indicate multiplier was specified */

    unsigned DIOinitCondGiven : 1;  /* flag to indicate ic was specified */
    unsigned DIOsenPertFlag :1; /* indictes whether the the parameter of
                               the particular instance is to be perturbed */
    unsigned DIOtempGiven : 1;  /* flag to indicate temperature was specified */
    unsigned DIOdtempGiven : 1; /* flag to indicate dtemp given */

    double DIOarea;     /* area factor for the diode */
    double DIOpj;     /* perimeter for the diode */
    double DIOm;     /* multiplier for the diode */

    double DIOinitCond;      /* initial condition */
    double DIOtemp;          /* temperature of the instance */
    double DIOdtemp;         /* delta temperature of instance */
    double DIOtJctPot;       /* temperature adjusted junction potential */
    double DIOtJctCap;       /* temperature adjusted junction capacitance */
    double DIOtJctSWPot;     /* temperature adjusted sidewall junction potential */
    double DIOtJctSWCap;     /* temperature adjusted sidewall junction capacitance */
    double DIOtTransitTime;  /* temperature adjusted transit time */
    double DIOtGradingCoeff; /* temperature adjusted grading coefficient (MJ) */
    double DIOtConductance;  /* temperature adjusted series conductance */

    double DIOtDepCap;  /* temperature adjusted transition point in */
                        /* the curve matching (Fc * Vj ) */
    double DIOtSatCur;  /* temperature adjusted saturation current */
    double DIOtSatSWCur;  /* temperature adjusted side wall saturation current */

    double DIOtVcrit;   /* temperature adjusted V crit */
    double DIOtF1;      /* temperature adjusted f1 */
    double DIOtBrkdwnV; /* temperature adjusted breakdown voltage */
    
    double DIOtF2;     /* coeff. for capacitance equation precomputation */
    double DIOtF3;     /* coeff. for capacitance equation precomputation */
    double DIOtF2SW;   /* coeff. for capacitance equation precomputation */
    double DIOtF3SW;   /* coeff. for capacitance equation precomputation */

// SRW
    double DIOgd;

/*
 * naming convention:
 * x = vdiode
 */

/* the following are relevant to s.s. sinusoidal distortion analysis */

#define DIONDCOEFFS     6

#ifndef NODISTO
        double DIOdCoeffs[DIONDCOEFFS];
#else /* NODISTO */
        double *DIOdCoeffs;
#endif /* NODISTO */

#ifndef CONFIG

#define id_x2           DIOdCoeffs[0]
#define id_x3           DIOdCoeffs[1]
#define cdif_x2         DIOdCoeffs[2]
#define cdif_x3         DIOdCoeffs[3]
#define cjnc_x2         DIOdCoeffs[4]
#define cjnc_x3         DIOdCoeffs[5]

#endif

/* indices to array of diode noise  sources */

#define DIORSNOIZ       0
#define DIOIDNOIZ       1
#define DIOFLNOIZ 2
#define DIOTOTNOIZ    3

#define DIONSRCS     4

#ifndef NONOISE
     double DIOnVar[NSTATVARS][DIONSRCS];
#else /* NONOISE */
        double **DIOnVar;
#endif /* NONOISE */

};


#define DIOvoltage    GENstate+0
#define DIOcurrent    GENstate+1
#define DIOconduct    GENstate+2
#define DIOcapCharge  GENstate+3
#define DIOcapCurrent GENstate+4

// SRW, interpolation
#define DIOa_cap      GENstate+5

#define DIOnumStates 6


struct sDIOmodel : public sGENmodel
{
    sDIOmodel()         { memset(this, 0, sizeof(sDIOmodel)); }
    sDIOmodel *next()   { return (static_cast<sDIOmodel*>(GENnextModel)); }
    sDIOinstance *inst() { return (static_cast<sDIOinstance*>(GENinstances)); }


    double DIOsatCur;   /* saturation current */
    double DIOsatSWCur;   /* Sidewall saturation current */

    double DIOresist;             /* ohmic series resistance */ 
    double DIOresistTemp1;        /* series resistance 1st order temp. coeff. */
    double DIOresistTemp2;        /* series resistance 2nd order temp. coeff. */
    double DIOconductance;        /* conductance corresponding to ohmic R */
    double DIOemissionCoeff;      /* emission coefficient (N) */
    double DIOtransitTime;        /* transit time (TT) */
    double DIOtranTimeTemp1;      /* transit time 1st order coefficient */
    double DIOtranTimeTemp2;      /* transit time 2nd order coefficient */
    double DIOjunctionCap;        /* Junction Capacitance (Cj0) */
    double DIOjunctionPot;        /* Junction Potential (Vj) or (PB) */
    double DIOgradingCoeff;       /* grading coefficient (m) or (mj) */
    double DIOgradCoeffTemp1;     /* grading coefficient 1st order temp. coeff.*/
    double DIOgradCoeffTemp2;     /* grading coefficient 2nd order temp. coeff.*/
    double DIOjunctionSWCap;      /* Sidewall Junction Capacitance (Cjsw) */
    double DIOjunctionSWPot;      /* Sidewall Junction Potential (Vjsw) or (PBSW) */
    double DIOgradingSWCoeff;     /* Sidewall grading coefficient (mjsw) */
    double DIOforwardKneeCurrent; /* Forward Knee current */
    double DIOreverseKneeCurrent; /* Reverse Knee current */

    double DIOactivationEnergy; /* activation energy (EG) */
    double DIOsaturationCurrentExp; /* Saturation current exponential (XTI) */
    double DIOdepletionCapCoeff;    /* Depletion Cap fraction coefficient (FC)*/
    double DIOdepletionSWcapCoeff;    /* Depletion sw-Cap fraction coefficient (FCS)*/
    double DIObreakdownVoltage; /* Voltage at reverse breakdown */
    double DIObreakdownCurrent; /* Current at above voltage */

    double DIOnomTemp;  /* nominal temperature (temp at which parms measured */
    double DIOfNcoef;
    double DIOfNexp;
    double DIOpj;
    double DIOarea;

    double DIOcta;
    double DIOctp;
    double DIOtcv;
    double DIOtpb;
    double DIOtphp;

    int DIOtlev;
    int DIOtlevc;

    unsigned DIOsatCurGiven : 1;
    unsigned DIOsatSWCurGiven : 1;

    unsigned DIOresistGiven : 1;
    unsigned DIOresistTemp1Given : 1;
    unsigned DIOresistTemp2Given : 1;
    unsigned DIOemissionCoeffGiven : 1;
    unsigned DIOtransitTimeGiven : 1;
    unsigned DIOtranTimeTemp1Given : 1;
    unsigned DIOtranTimeTemp2Given : 1;
    unsigned DIOjunctionCapGiven : 1;
    unsigned DIOjunctionPotGiven : 1;
    unsigned DIOgradingCoeffGiven : 1;
    unsigned DIOgradCoeffTemp1Given : 1;
    unsigned DIOgradCoeffTemp2Given : 1;
    unsigned DIOjunctionSWCapGiven : 1;
    unsigned DIOjunctionSWPotGiven : 1;
    unsigned DIOgradingSWCoeffGiven : 1;
    unsigned DIOforwardKneeCurrentGiven : 1;
    unsigned DIOreverseKneeCurrentGiven : 1;

    unsigned DIOactivationEnergyGiven : 1;
    unsigned DIOsaturationCurrentExpGiven : 1;
    unsigned DIOdepletionCapCoeffGiven : 1;
    unsigned DIOdepletionSWcapCoeffGiven :1;
    unsigned DIObreakdownVoltageGiven : 1;
    unsigned DIObreakdownCurrentGiven : 1;
    unsigned DIOnomTempGiven : 1;
    unsigned DIOfNcoefGiven : 1;
    unsigned DIOfNexpGiven : 1;
    unsigned DIOpjGiven : 1;
    unsigned DIOareaGiven : 1;

    unsigned DIOctaGiven : 1;
    unsigned DIOctpGiven : 1;
    unsigned DIOtcvGiven : 1;
    unsigned DIOtpbGiven : 1;
    unsigned DIOtphpGiven : 1;
    unsigned DIOtlevGiven : 1;
    unsigned DIOtlevcGiven : 1;
};

} // namespace DIO
using namespace DIO;


// device parameters
// DO NOT CHANGE THIS without updating aski/seti tables!
enum {
    DIO_AREA = 1, 
    DIO_IC,
    DIO_OFF,
    DIO_TEMP,
    DIO_DTEMP,
    DIO_PJ,
    DIO_M,
    DIO_CAP,
    DIO_CURRENT,
    DIO_VOLTAGE,
    DIO_CHARGE,
    DIO_CAPCUR,
    DIO_CONDUCT,
    DIO_POWER,

    // SRW
    DIO_POSNODE,
    DIO_NEGNODE,
    DIO_INTNODE
};

// model parameters
enum {
    DIO_MOD_IS = 1000,
    DIO_MOD_RS,
    DIO_MOD_N,
    DIO_MOD_TT,
    DIO_MOD_CJO,
    DIO_MOD_VJ,
    DIO_MOD_M,
    DIO_MOD_EG,
    DIO_MOD_XTI,
    DIO_MOD_FC,
    DIO_MOD_BV,
    DIO_MOD_IBV,
    DIO_MOD_D,
    DIO_MOD_COND,
    DIO_MOD_TNOM,
    DIO_MOD_KF,
    DIO_MOD_AF,
    DIO_MOD_JSW,
    DIO_MOD_CJSW,
    DIO_MOD_VJSW,
    DIO_MOD_MJSW,
    DIO_MOD_IKF,
    DIO_MOD_IKR,
    DIO_MOD_FCS,
    DIO_MOD_TTT1,
    DIO_MOD_TTT2,
    DIO_MOD_TM1,
    DIO_MOD_TM2,
    DIO_MOD_TRS,
    DIO_MOD_TRS2,
    DIO_MOD_PJ,
    DIO_MOD_AREA,

    DIO_MOD_CTA,
    DIO_MOD_CTP,
    DIO_MOD_TCV,
    DIO_MOD_TLEV,
    DIO_MOD_TLEVC,
    DIO_MOD_TPB,
    DIO_MOD_TPHP
};

#endif // DIODEFS_H

