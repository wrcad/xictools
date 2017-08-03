
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
/********** new in 3f2
Sydney University mods Copyright(c) 1989 Anthony E. Parker, David J. Skellern
    Laboratory for Communication Science Engineering
    Sydney University Department of Electrical Engineering, Australia
**********/

#ifndef JFETDEFS_H
#define JFETDEFS_H

#include "device.h"

//
// structures used to describe Junction Field Effect Transistors
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

namespace JFET {

struct sJFETmodel;

struct JFETdev : public IFdevice
{
    JFETdev();
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
//    int convTest(sGENmodel*, sCKT*);  

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
    int dSetup(sJFETmodel*, sCKT*);
};

struct sJFETinstance : public sGENinstance
{
    sJFETinstance()
        {
            memset(this, 0, sizeof(sJFETinstance));
            GENnumNodes = 3;
        }
    sJFETinstance *next()
        { return (static_cast<sJFETinstance*>(GENnextInstance)); }
    void ac_cd(const sCKT*, double*, double*) const;
    void ac_cs(const sCKT*, double*, double*) const;
    void ac_cg(const sCKT*, double*, double*) const;

    int JFETdrainNode;   // number of drain node of jfet
    int JFETgateNode;    // number of gate node of jfet
    int JFETsourceNode;  // number of source node of jfet

    int JFETdrainPrimeNode;   // number of internal drain node of jfet
    int JFETsourcePrimeNode;  // number of internal source node of jfet

    double *JFETdrainDrainPrimePtr; // pointer to sparse matrix at 
                                    //  (drain,drain prime)
    double *JFETgateDrainPrimePtr;  // pointer to sparse matrix at 
                                    //  (gate,drain prime)
    double *JFETgateSourcePrimePtr; // pointer to sparse matrix at 
                                    //  (gate,source prime)
    double *JFETsourceSourcePrimePtr;   // pointer to sparse matrix at 
                                        //  (source,source prime)
    double *JFETdrainPrimeDrainPtr; // pointer to sparse matrix at 
                                    //  (drain prime,drain)
    double *JFETdrainPrimeGatePtr;  // pointer to sparse matrix at 
                                    //  (drain prime,gate)
    double *JFETdrainPrimeSourcePrimePtr;   // pointer to sparse matrix
                                            //  (drain prime,source prime)
    double *JFETsourcePrimeGatePtr; // pointer to sparse matrix at 
                                    //  (source prime,gate)
    double *JFETsourcePrimeSourcePtr;   // pointer to sparse matrix at 
                                        //  (source prime,source)
    double *JFETsourcePrimeDrainPrimePtr;   // pointer to sparse matrix
                                            //  (source prime,drain prime)
    double *JFETdrainDrainPtr;  // pointer to sparse matrix at 
                                //  (drain,drain)
    double *JFETgateGatePtr;    // pointer to sparse matrix at 
                                //  (gate,gate)
    double *JFETsourceSourcePtr;    // pointer to sparse matrix at 
                                    //  (source,source)
    double *JFETdrainPrimeDrainPrimePtr;    // pointer to sparse matrix
                                            //  (drain prime,drain prime)
    double *JFETsourcePrimeSourcePrimePtr;  // pointer to sparse matrix
                                            //  (source prime,source prime)
    int JFETmode;
    // distortion analysis Taylor coeffs.

//
// naming convention:
// x = vgs
// y = vds
// cdr = cdrain
//

#define JFETNDCOEFFS    21

#ifndef NODISTO
    double JFETdCoeffs[JFETNDCOEFFS];
#else
    double *JFETdCoeffs;
#endif

#ifdef DISTO
#define    cdr_x       JFETdCoeffs[0]
#define    cdr_y       JFETdCoeffs[1]
#define    cdr_x2      JFETdCoeffs[2]
#define    cdr_y2      JFETdCoeffs[3]
#define    cdr_xy      JFETdCoeffs[4]
#define    cdr_x3      JFETdCoeffs[5]
#define    cdr_y3      JFETdCoeffs[6]
#define    cdr_x2y     JFETdCoeffs[7]
#define    cdr_xy2     JFETdCoeffs[8]

#define    ggs1        JFETdCoeffs[9]
#define    ggd1        JFETdCoeffs[10]
#define    ggs2        JFETdCoeffs[11]
#define    ggd2        JFETdCoeffs[12]
#define    ggs3        JFETdCoeffs[13]
#define    ggd3        JFETdCoeffs[14]
#define    capgs1      JFETdCoeffs[15]
#define    capgd1      JFETdCoeffs[16]
#define    capgs2      JFETdCoeffs[17]
#define    capgd2      JFETdCoeffs[18]
#define    capgs3      JFETdCoeffs[19]
#define    capgd3      JFETdCoeffs[20]
#endif

// indices to an array of JFET noise sources

#define JFETRDNOIZ     0
#define JFETRSNOIZ     1
#define JFETIDNOIZ     2
#define JFETFLNOIZ     3
#define JFETTOTNOIZ    4

#define JFETNSRCS      5

#ifndef NONOISE
    double JFETnVar[NSTATVARS][JFETNSRCS];
#else
    double **JFETnVar;
#endif

    unsigned JFEToff :1;            // 'off' flag for jfet
    unsigned JFETareaGiven  : 1;    // flag to indicate area was specified
    unsigned JFETicVDSGiven : 1;    // initial condition given flag for V D-S
    unsigned JFETicVGSGiven : 1;    // initial condition given flag for V G-S
    unsigned JFETtempGiven  : 1;    // flag to indicate instance temp given

    double JFETarea;    // area factor for the jfet
    double JFETicVDS;   // initial condition voltage D-S
    double JFETicVGS;   // initial condition voltage G-S
    double JFETtemp;    // operating temperature
    double JFETtSatCur; // temperature adjusted saturation current
    double JFETtGatePot;    // temperature adjusted gate potential
    double JFETtCGS;    // temperature corrected G-S capacitance
    double JFETtCGD;    // temperature corrected G-D capacitance
    double JFETcorDepCap;   // joining point of the fwd bias dep. cap eq.s
    double JFETvcrit;   // critical voltage for the instance
    double JFETf1;      // coefficient of capacitance polynomial exp
};

#define JFETvgs   GENstate 
#define JFETvgd   GENstate+1 
#define JFETcg    GENstate+2 
#define JFETcd    GENstate+3 
#define JFETcgd   GENstate+4 
#define JFETgm    GENstate+5 
#define JFETgds   GENstate+6 
#define JFETggs   GENstate+7 
#define JFETggd   GENstate+8 
#define JFETqgs   GENstate+9 
#define JFETcqgs  GENstate+10 
#define JFETqgd   GENstate+11 
#define JFETcqgd  GENstate+12 

#define JFETnumStates 13

struct sJFETmodel : sGENmodel
{
    sJFETmodel()        { memset(this, 0, sizeof(sJFETmodel)); }
    sJFETmodel *next()  { return (static_cast<sJFETmodel*>(GENnextModel)); }
    sJFETinstance *inst()
                        { return (static_cast<sJFETinstance*>(GENinstances)); }

    int JFETtype;

    double JFETthreshold;
    double JFETbeta;
    double JFETlModulation;
    double JFETdrainResist;
    double JFETsourceResist;
    double JFETcapGS;
    double JFETcapGD;
    double JFETgatePotential;
    double JFETgateSatCurrent;
    double JFETdepletionCapCoeff;
    double JFETfNcoef;
    double JFETfNexp;


    double JFETdrainConduct;
    double JFETsourceConduct;
    double JFETf2;
    double JFETf3;
    // Modification for Sydney University JFET model
    double JFETb;     // doping profile parameter
    double JFETbFac;  // internal derived doping profile parameter
    // end Sydney University mod
    double JFETtnom;    // temperature at which parameters were measured

    unsigned JFETthresholdGiven : 1;
    unsigned JFETbetaGiven : 1;
    unsigned JFETlModulationGiven : 1;
    unsigned JFETdrainResistGiven : 1;
    unsigned JFETsourceResistGiven : 1;
    unsigned JFETcapGSGiven : 1;
    unsigned JFETcapGDGiven : 1;
    unsigned JFETgatePotentialGiven : 1;
    unsigned JFETgateSatCurrentGiven : 1;
    unsigned JFETdepletionCapCoeffGiven : 1;
    // Modification for Sydney University JFET model
    unsigned JFETbGiven : 1;
    // end Sydney University mod
    unsigned JFETtnomGiven : 1; // user specified Tnom for model
    unsigned JFETfNcoefGiven : 1;
    unsigned JFETfNexpGiven : 1;
};

} // namespace JFET
using namespace JFET;


#ifndef NJF
#define NJF 1
#define PJF -1
#endif

// device parameters
enum {
    JFET_AREA = 1,
    JFET_OFF,
    JFET_TEMP,
    JFET_IC_VDS,
    JFET_IC_VGS,
    JFET_IC,

    JFET_VGS,
    JFET_VGD,
    JFET_CG,
    JFET_CD,
    JFET_CGD,
    JFET_CS,
    JFET_POWER,
    JFET_GM,
    JFET_GDS,
    JFET_GGS,
    JFET_GGD,
    JFET_QGS,
    JFET_QGD,
    JFET_CQGS,
    JFET_CQGD,
    JFET_DRAINNODE,
    JFET_GATENODE,
    JFET_SOURCENODE,
    JFET_DRAINPRIMENODE,
    JFET_SOURCEPRIMENODE
};

// model parameters
enum {
    JFET_MOD_VTO = 1000,
    JFET_MOD_BETA,
    JFET_MOD_LAMBDA,
    JFET_MOD_RD,
    JFET_MOD_RS,
    JFET_MOD_CGS,
    JFET_MOD_CGD,
    JFET_MOD_PB,
    JFET_MOD_IS,
    JFET_MOD_FC,
    JFET_MOD_NJF,
    JFET_MOD_PJF,
    JFET_MOD_TNOM,
    JFET_MOD_KF,
    JFET_MOD_AF,
    // Modification for Sydney University JFET model
    JFET_MOD_B,
    // end Sydney University mod 

    JFET_MOD_DRAINCOND,
    JFET_MOD_SOURCECOND,
    JFET_MOD_DEPLETIONCAP,
    JFET_MOD_VCRIT,
    JFET_MOD_TYPE
};

#endif // JFETDEFS_H

