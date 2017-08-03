
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
Authors: 1985 S. Hwang
         1993 Stephen R. Whiteley
****************************************************************************/

#ifndef MESDEFS_H
#define MESDEFS_H

#include "device.h"

//
// structures used to describe MESFET Transistors 
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

namespace MES {

struct sMESmodel;

struct MESdev : public IFdevice
{
    MESdev();
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
    int dSetup(sMESmodel*, sCKT *ckt);
};

struct sMESinstance : sGENinstance
{
    sMESinstance()
        {
            memset(this, 0, sizeof(sMESinstance));
            GENnumNodes = 3;
        }
    sMESinstance *next()
        { return (static_cast<sMESinstance*>(GENnextInstance)); }
    void ac_cd(const sCKT*, double*, double*) const;
    void ac_cs(const sCKT*, double*, double*) const;
    void ac_cg(const sCKT*, double*, double*) const;

    int MESdrainNode;   // number of drain node of mesfet
    int MESgateNode;    // number of gate node of mesfet
    int MESsourceNode;  // number of source node of mesfet

    int MESdrainPrimeNode;     // number of internal drain node of mesfet
    int MESsourcePrimeNode;    // number of internal source node of mesfet
    int MESmode;
    double MESarea;    // area factor for the mesfet
    double MESicVDS;   // initial condition voltage D-S
    double MESicVGS;   // initial condition voltage G-S
    double *MESdrainDrainPrimePtr; // pointer to sparse matrix at 
                                   //  (drain,drain prime)
    double *MESgateDrainPrimePtr;  // pointer to sparse matrix at 
                                   //  (gate,drain prime)
    double *MESgateSourcePrimePtr; // pointer to sparse matrix at 
                                   //  (gate,source prime)
    double *MESsourceSourcePrimePtr;   // pointer to sparse matrix at 
                                       //  (source,source prime)
    double *MESdrainPrimeDrainPtr; // pointer to sparse matrix at 
                                   //  (drain prime,drain)
    double *MESdrainPrimeGatePtr;  // pointer to sparse matrix at 
                                   //  (drain prime,gate)
    double *MESdrainPrimeSourcePrimePtr;   // pointer to sparse matrix
                                           //  (drain prime,source prime)
    double *MESsourcePrimeGatePtr; // pointer to sparse matrix at 
                                   //  (source prime,gate)
    double *MESsourcePrimeSourcePtr;   // pointer to sparse matrix at 
                                       //  (source prime,source)
    double *MESsourcePrimeDrainPrimePtr;   // pointer to sparse matrix
                                           //  (source prime,drain prime)
    double *MESdrainDrainPtr;  // pointer to sparse matrix at 
                               //  (drain,drain)
    double *MESgateGatePtr;    // pointer to sparse matrix at 
                               //  (gate,gate)
    double *MESsourceSourcePtr;    // pointer to sparse matrix at 
                                   //  (source,source)
    double *MESdrainPrimeDrainPrimePtr;    // pointer to sparse matrix
                                           //  (drain prime,drain prime)
    double *MESsourcePrimeSourcePrimePtr;  // pointer to sparse matrix
                                           //  (source prime,source prime)

    int MESoff;   // 'off' flag for mesfet
    unsigned MESareaGiven : 1;   // flag to indicate area was specified
    unsigned MESicVDSGiven : 1;  // initial condition given flag for V D-S
    unsigned MESicVGSGiven : 1;  // initial condition given flag for V G-S

//
// naming convention:
// x = vgs
// y = vgd
// z = vds
// cdr = cdrain
//

#define MESNDCOEFFS  27

#ifndef NODISTO
    double MESdCoeffs[MESNDCOEFFS];
#else
    double *MESdCoeffs;
#endif

#ifdef DISTO
#define    cdr_x       MESdCoeffs[0]
#define    cdr_z       MESdCoeffs[1]
#define    cdr_x2      MESdCoeffs[2]
#define    cdr_z2      MESdCoeffs[3]
#define    cdr_xz      MESdCoeffs[4]
#define    cdr_x3      MESdCoeffs[5]
#define    cdr_z3      MESdCoeffs[6]
#define    cdr_x2z     MESdCoeffs[7]
#define    cdr_xz2     MESdCoeffs[8]

#define    ggs3        MESdCoeffs[9]
#define    ggd3        MESdCoeffs[10]
#define    ggs2        MESdCoeffs[11]
#define    ggd2        MESdCoeffs[12]

#define    qgs_x2      MESdCoeffs[13]
#define    qgs_y2      MESdCoeffs[14]
#define    qgs_xy      MESdCoeffs[15]
#define    qgs_x3      MESdCoeffs[16]
#define    qgs_y3      MESdCoeffs[17]
#define    qgs_x2y     MESdCoeffs[18]
#define    qgs_xy2     MESdCoeffs[19]

#define    qgd_x2      MESdCoeffs[20]
#define    qgd_y2      MESdCoeffs[21]
#define    qgd_xy      MESdCoeffs[22]
#define    qgd_x3      MESdCoeffs[23]
#define    qgd_y3      MESdCoeffs[24]
#define    qgd_x2y     MESdCoeffs[25]
#define    qgd_xy2     MESdCoeffs[26]
#endif

// indices to the array of MESFET noise sources

#define MESRDNOIZ     0
#define MESRSNOIZ     1
#define MESIDNOIZ     2
#define MESFLNOIZ     3
#define MESTOTNOIZ    4

#define MESNSRCS      5     // the number of MESFET noise sources

#ifndef NONOISE
    double MESnVar[NSTATVARS][MESNSRCS];
#else
    double **MESnVar;
#endif

};

#define MESvgs  GENstate 
#define MESvgd  GENstate+1 
#define MEScg   GENstate+2 
#define MEScd   GENstate+3 
#define MEScgd  GENstate+4 
#define MESgm   GENstate+5 
#define MESgds  GENstate+6 
#define MESggs  GENstate+7 
#define MESggd  GENstate+8 
#define MESqgs  GENstate+9 
#define MEScqgs GENstate+10 
#define MESqgd  GENstate+11 
#define MEScqgd GENstate+12 

#define MESnumStates 13

struct sMESmodel : sGENmodel
{
    sMESmodel()         { memset(this, 0, sizeof(sMESmodel)); }
    sMESmodel *next()   { return (static_cast<sMESmodel*>(GENnextModel)); }
    sMESinstance *inst() { return (static_cast<sMESinstance*>(GENinstances)); }

    int MEStype;

    double MESthreshold;
    double MESalpha;
    double MESbeta;
    double MESlModulation;
    double MESb;
    double MESdrainResist;
    double MESsourceResist;
    double MEScapGS;
    double MEScapGD;
    double MESgatePotential;
    double MESgateSatCurrent;
    double MESdepletionCapCoeff;
    double MESfNcoef;
    double MESfNexp;

    double MESdrainConduct;
    double MESsourceConduct;
    double MESdepletionCap;
    double MESf1;
    double MESf2;
    double MESf3;
    double MESvcrit;

    unsigned MESthresholdGiven : 1;
    unsigned MESalphaGiven : 1;
    unsigned MESbetaGiven : 1;
    unsigned MESlModulationGiven : 1;
    unsigned MESbGiven : 1;
    unsigned MESdrainResistGiven : 1;
    unsigned MESsourceResistGiven : 1;
    unsigned MEScapGSGiven : 1;
    unsigned MEScapGDGiven : 1;
    unsigned MESgatePotentialGiven : 1;
    unsigned MESgateSatCurrentGiven : 1;
    unsigned MESdepletionCapCoeffGiven : 1;
    unsigned MESfNcoefGiven : 1;
    unsigned MESfNexpGiven : 1;
};

} // namespace MES
using namespace MES;


#ifndef NMF
#define NMF 1
#define PMF -1
#endif

// device parameters
enum {
    MES_AREA = 1,
    MES_OFF,
    MES_IC_VDS,
    MES_IC_VGS,
    MES_IC,

    MES_VGS,
    MES_VGD,
    MES_CG,
    MES_CD,
    MES_CGD,
    MES_CS,
    MES_POWER,
    MES_GM,
    MES_GDS,
    MES_GGS,
    MES_GGD,
    MES_QGS,
    MES_QGD,
    MES_CQGS,
    MES_CQGD,
    MES_DRAINNODE,
    MES_GATENODE,
    MES_SOURCENODE,
    MES_DRAINPRIMENODE,
    MES_SOURCEPRIMENODE
};

// model parameters
enum {
    MES_MOD_VTO = 1000,
    MES_MOD_ALPHA,
    MES_MOD_BETA,
    MES_MOD_LAMBDA,
    MES_MOD_B,
    MES_MOD_RD,
    MES_MOD_RS,
    MES_MOD_CGS,
    MES_MOD_CGD,
    MES_MOD_PB,
    MES_MOD_IS,
    MES_MOD_FC,
    MES_MOD_NMF,
    MES_MOD_PMF,
    MES_MOD_KF,
    MES_MOD_AF,

    MES_MOD_DRAINCOND,
    MES_MOD_SOURCECOND,
    MES_MOD_DEPLETIONCAP,
    MES_MOD_VCRIT,
    MES_MOD_TYPE
};

#endif // MESDEFS_H

