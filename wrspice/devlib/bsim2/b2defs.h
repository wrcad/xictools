
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
Authors: 1988 Min-Chie Jeng, Hong June Park, Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#ifndef B2DEFS_H
#define B2DEFS_H

#include "device.h"

//
// declarations for BSIM2 MOSFETs
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

namespace B2 {

struct sB2model;
struct sB2instance;

struct B2dev : public IFdevice
{
    B2dev();
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

    int setInst(int, IFdata*, sGENinstance*);  
    int setModl(int, IFdata*, sGENmodel*);   
    int askInst(const sCKT*, const sGENinstance*, int, IFdata*);    
    int askModl(const sGENmodel*, int, IFdata*); 
//    int findBranch(sCKT*, sGENmodel*, IFuid); 

    int acLoad(sGENmodel*, sCKT*); 
    int pzLoad(sGENmodel*, sCKT*, IFcomplex*); 
//    int disto(int, sGENmodel*, sCKT*);  
//    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
private:
    int dSetup(sB2model*, sCKT*);
    void evaluate(double, double, double, sB2instance*, sB2model*, double*,
        double*, double*, double*, double*, double*, double*, double*,
        double*, double*, double*, double*, double*, double*, double*,
        double*, double*, double*, sCKT*);
    void mosCap(sCKT*, double, double, double, double*, double, double,
        double, double, double, double, double*, double*, double*, double*,
        double*, double*, double*, double*, double*, double*, double*,
        double*, double*, double*, double*, double*);
};

struct sB2instance : sGENinstance
{
    sB2instance()
        {
            memset(this, 0, sizeof(sB2instance));
            GENnumNodes = 4;
        }
    sB2instance *next()
        { return (static_cast<sB2instance*>(GENnextInstance)); }

    int B2dNode;   // number of the gate node of the mosfet
    int B2gNode;   // number of the gate node of the mosfet
    int B2sNode;   // number of the source node of the mosfet
    int B2bNode;   // number of the bulk node of the mosfet

    int B2dNodePrime; // number of the internal drain node of the mosfet
    int B2sNodePrime; // number of the internal source node of the mosfet

    double B2m;    // instance multiplier
    double B2l;    // the length of the channel region
    double B2w;    // the width of the channel region
    double B2drainArea;   // the area of the drain diffusion
    double B2sourceArea;  // the area of the source diffusion
    double B2drainSquares;    // the length of the drain in squares
    double B2sourceSquares;   // the length of the source in squares
    double B2drainPerimeter;
    double B2sourcePerimeter;
    double B2sourceConductance;   // cond. of source (or 0): set in setup
    double B2drainConductance;    // cond. of drain (or 0): set in setup

    double B2icVBS;   // initial condition B-S voltage
    double B2icVDS;   // initial condition D-S voltage
    double B2icVGS;   // initial condition G-S voltage
    double B2von;
    double B2vdsat;
    int B2off;        // non-zero to indicate device is off for dc analysis
    int B2mode;       // device mode : 1 = normal, -1 = inverse

    struct bsim2SizeDependParam  *pParam;

    unsigned B2mGiven :1;
    unsigned B2lGiven :1;
    unsigned B2wGiven :1;
    unsigned B2drainAreaGiven :1;
    unsigned B2sourceAreaGiven    :1;
    unsigned B2drainSquaresGiven  :1;
    unsigned B2sourceSquaresGiven :1;
    unsigned B2drainPerimeterGiven    :1;
    unsigned B2sourcePerimeterGiven   :1;
    unsigned B2dNodePrimeSet  :1;
    unsigned B2sNodePrimeSet  :1;
    unsigned B2icVBSGiven :1;
    unsigned B2icVDSGiven :1;
    unsigned B2icVGSGiven :1;
    unsigned B2vonGiven   :1;
    unsigned B2vdsatGiven :1;


    double *B2DdPtr;      // pointer to sparse matrix element at
                          //  (Drain node,drain node)
    double *B2GgPtr;      // pointer to sparse matrix element at
                          //  (gate node,gate node)
    double *B2SsPtr;      // pointer to sparse matrix element at
                          //  (source node,source node)
    double *B2BbPtr;      // pointer to sparse matrix element at
                          //  (bulk node,bulk node)
    double *B2DPdpPtr;    // pointer to sparse matrix element at
                          //  (drain prime node,drain prime node)
    double *B2SPspPtr;    // pointer to sparse matrix element at
                          //  (source prime node,source prime node)
    double *B2DdpPtr;     // pointer to sparse matrix element at
                          //  (drain node,drain prime node)
    double *B2GbPtr;      // pointer to sparse matrix element at
                          //  (gate node,bulk node)
    double *B2GdpPtr;     // pointer to sparse matrix element at
                          //  (gate node,drain prime node)
    double *B2GspPtr;     // pointer to sparse matrix element at
                          //  (gate node,source prime node)
    double *B2SspPtr;     // pointer to sparse matrix element at
                          //  (source node,source prime node)
    double *B2BdpPtr;     // pointer to sparse matrix element at
                          //  (bulk node,drain prime node)
    double *B2BspPtr;     // pointer to sparse matrix element at
                          //  (bulk node,source prime node)
    double *B2DPspPtr;    // pointer to sparse matrix element at
                          //  (drain prime node,source prime node)
    double *B2DPdPtr;     // pointer to sparse matrix element at
                          //  (drain prime node,drain node)
    double *B2BgPtr;      // pointer to sparse matrix element at
                          //  (bulk node,gate node)
    double *B2DPgPtr;     // pointer to sparse matrix element at
                          //  (drain prime node,gate node)

    double *B2SPgPtr;     // pointer to sparse matrix element at
                          //  (source prime node,gate node)
    double *B2SPsPtr;     // pointer to sparse matrix element at
                          //  (source prime node,source node)
    double *B2DPbPtr;     // pointer to sparse matrix element at
                          //  (drain prime node,bulk node)
    double *B2SPbPtr;     // pointer to sparse matrix element at
                          //  (source prime node,bulk node)
    double *B2SPdpPtr;    // pointer to sparse matrix element at
                          //  (source prime node,drain prime node)

#define B2vbd GENstate + 0
#define B2vbs GENstate + 1
#define B2vgs GENstate + 2
#define B2vds GENstate + 3
#define B2cd GENstate + 4
#define B2id GENstate + 4
#define B2cbs GENstate + 5
#define B2ibs GENstate + 5
#define B2cbd GENstate + 6
#define B2ibd GENstate + 6
#define B2gm GENstate + 7
#define B2gds GENstate + 8
#define B2gmbs GENstate + 9
#define B2gbd GENstate + 10
#define B2gbs GENstate + 11
#define B2qb GENstate + 12
#define B2cqb GENstate + 13
#define B2iqb GENstate + 13
#define B2qg GENstate + 14
#define B2cqg GENstate + 15
#define B2iqg GENstate + 15
#define B2qd GENstate + 16
#define B2cqd GENstate + 17
#define B2iqd GENstate + 17
#define B2cggb GENstate + 18
#define B2cgdb GENstate + 19
#define B2cgsb GENstate + 20
#define B2cbgb GENstate + 21
#define B2cbdb GENstate + 22
#define B2cbsb GENstate + 23
#define B2capbd GENstate + 24
#define B2iqbd GENstate + 25
#define B2cqbd GENstate + 25
#define B2capbs GENstate + 26
#define B2iqbs GENstate + 27
#define B2cqbs GENstate + 27
#define B2cdgb GENstate + 28
#define B2cddb GENstate + 29
#define B2cdsb GENstate + 30
#define B2vono GENstate + 31
#define B2vdsato GENstate + 32
#define B2qbs  GENstate + 33
#define B2qbd  GENstate + 34

#define B2numStates 35           

};

struct bsim2SizeDependParam
{
    double Width;
    double Length;
    double B2vfb;      // flat band voltage at given L and W
    double B2phi;      // surface potential at strong inversion
    double B2k1;       // bulk effect coefficient 1
    double B2k2;       // bulk effect coefficient 2
    double B2eta0;     // drain induced barrier lowering
    double B2etaB;     // Vbs dependence of Eta
    double B2beta0;    // Beta at Vds = 0 and Vgs = Vth
    double B2beta0B;   // Vbs dependence of Beta0
    double B2betas0;   // Beta at Vds=Vdd and Vgs=Vth
    double B2betasB;   // Vbs dependence of Betas
    double B2beta20;   // Vds dependence of Beta in tanh term
    double B2beta2B;   // Vbs dependence of Beta2
    double B2beta2G;   // Vgs dependence of Beta2
    double B2beta30;   // Vds dependence of Beta in linear term
    double B2beta3B;   // Vbs dependence of Beta3
    double B2beta3G;   // Vgs dependence of Beta3
    double B2beta40;   // Vds dependence of Beta in quadra term
    double B2beta4B;   // Vbs dependence of Beta4
    double B2beta4G;   // Vgs dependence of Beta4
    double B2ua0;      // Linear Vgs dependence of Mobility
    double B2uaB;      // Vbs dependence of Ua
    double B2ub0;      // Quadratic Vgs dependence of Mobility
    double B2ubB;      // Vbs dependence of Ub
    double B2u10;      // Drift Velocity Saturation due to Vds
    double B2u1B;      // Vbs dependence of U1
    double B2u1D;      // Vds dependence of U1
    double B2n0;       // Subthreshold slope at Vds=0, Vbs=0
    double B2nB;       // Vbs dependence of n
    double B2nD;       // Vds dependence of n
    double B2vof0;     // Vth offset at Vds=0, Vbs=0
    double B2vofB;     // Vbs dependence of Vof
    double B2vofD;     // Vds dependence of Vof
    double B2ai0;      // Pre-factor in hot-electron effects
    double B2aiB;      // Vbs dependence of Ai
    double B2bi0;      // Exp-factor in hot-electron effects
    double B2biB;      // Vbs dependence of Bi
    double B2vghigh;   // Upper bound of cubic spline function
    double B2vglow;    // Lower bound of cubic spline function
    double B2GDoverlapCap;// Gate Drain Overlap Capacitance
    double B2GSoverlapCap;// Gate Source Overlap Capacitance
    double B2GBoverlapCap;// Gate Bulk Overlap Capacitance
    double SqrtPhi;
    double Phis3;
    double CoxWL;
    double One_Third_CoxWL;
    double Two_Third_CoxWL;
    double Arg;
    double B2vt0;
    struct bsim2SizeDependParam  *pNext;
};

struct sB2model : sGENmodel
{
    sB2model()          { memset(this, 0, sizeof(sB2model)); }
    sB2model *next()    { return (static_cast<sB2model*>(GENnextModel)); }
    sB2instance *inst() { return (static_cast<sB2instance*>(GENinstances)); }

    int B2type;               // device type: 1 = nmos,  -1 = pmos
    int pad;

    double B2vfb0;
    double B2vfbL;
    double B2vfbW;
    double B2phi0;
    double B2phiL;
    double B2phiW;
    double B2k10;
    double B2k1L;
    double B2k1W;
    double B2k20;
    double B2k2L;
    double B2k2W;
    double B2eta00;
    double B2eta0L;
    double B2eta0W;
    double B2etaB0;
    double B2etaBL;
    double B2etaBW;
    double B2deltaL;
    double B2deltaW;
    double B2mob00;
    double B2mob0B0;
    double B2mob0BL;
    double B2mob0BW ;
    double B2mobs00;
    double B2mobs0L;
    double B2mobs0W;
    double B2mobsB0;
    double B2mobsBL;
    double B2mobsBW;
    double B2mob200;
    double B2mob20L;
    double B2mob20W;
    double B2mob2B0;
    double B2mob2BL;
    double B2mob2BW;
    double B2mob2G0;
    double B2mob2GL;
    double B2mob2GW;
    double B2mob300;
    double B2mob30L;
    double B2mob30W;
    double B2mob3B0;
    double B2mob3BL;
    double B2mob3BW;
    double B2mob3G0;
    double B2mob3GL;
    double B2mob3GW;
    double B2mob400;
    double B2mob40L;
    double B2mob40W;
    double B2mob4B0;
    double B2mob4BL;
    double B2mob4BW;
    double B2mob4G0;
    double B2mob4GL;
    double B2mob4GW;
    double B2ua00;
    double B2ua0L;
    double B2ua0W;
    double B2uaB0;
    double B2uaBL;
    double B2uaBW;
    double B2ub00;
    double B2ub0L;
    double B2ub0W;
    double B2ubB0;
    double B2ubBL;
    double B2ubBW;
    double B2u100;
    double B2u10L;
    double B2u10W;
    double B2u1B0;
    double B2u1BL;
    double B2u1BW;
    double B2u1D0;
    double B2u1DL;
    double B2u1DW;
    double B2n00;
    double B2n0L;
    double B2n0W;
    double B2nB0;
    double B2nBL;
    double B2nBW;
    double B2nD0;
    double B2nDL;
    double B2nDW;
    double B2vof00;
    double B2vof0L;
    double B2vof0W;
    double B2vofB0;
    double B2vofBL;
    double B2vofBW;
    double B2vofD0;
    double B2vofDL;
    double B2vofDW;
    double B2ai00;
    double B2ai0L;
    double B2ai0W;
    double B2aiB0;
    double B2aiBL;
    double B2aiBW;
    double B2bi00;
    double B2bi0L;
    double B2bi0W;
    double B2biB0;
    double B2biBL;
    double B2biBW;
    double B2vghigh0;
    double B2vghighL;
    double B2vghighW;
    double B2vglow0;
    double B2vglowL;
    double B2vglowW;
    double B2tox;              // unit: micron
    double B2Cox;              // unit: F/cm**2
    double B2temp;
    double B2vdd;
    double B2vdd2;
    double B2vgg;
    double B2vgg2;
    double B2vbb;
    double B2vbb2;
    double B2gateSourceOverlapCap;
    double B2gateDrainOverlapCap;
    double B2gateBulkOverlapCap;
    double B2channelChargePartitionFlag;
    double B2Vtm;

    double B2sheetResistance;
    double B2jctSatCurDensity;
    double B2bulkJctPotential;
    double B2bulkJctBotGradingCoeff;
    double B2bulkJctSideGradingCoeff;
    double B2sidewallJctPotential;
    double B2unitAreaJctCap;
    double B2unitLengthSidewallJctCap;
    double B2defaultWidth;
    double B2deltaLength;

    struct bsim2SizeDependParam  *pSizeDependParamKnot;

    unsigned  B2vfb0Given   :1;
    unsigned  B2vfbLGiven   :1;
    unsigned  B2vfbWGiven   :1;
    unsigned  B2phi0Given   :1;
    unsigned  B2phiLGiven   :1;
    unsigned  B2phiWGiven   :1;
    unsigned  B2k10Given   :1;
    unsigned  B2k1LGiven   :1;
    unsigned  B2k1WGiven   :1;
    unsigned  B2k20Given   :1;
    unsigned  B2k2LGiven   :1;
    unsigned  B2k2WGiven   :1;
    unsigned  B2eta00Given   :1;
    unsigned  B2eta0LGiven   :1;
    unsigned  B2eta0WGiven   :1;
    unsigned  B2etaB0Given   :1;
    unsigned  B2etaBLGiven   :1;
    unsigned  B2etaBWGiven   :1;
    unsigned  B2deltaLGiven   :1;
    unsigned  B2deltaWGiven   :1;
    unsigned  B2mob00Given   :1;
    unsigned  B2mob0B0Given   :1;
    unsigned  B2mob0BLGiven   :1;
    unsigned  B2mob0BWGiven   :1;
    unsigned  B2mobs00Given   :1;
    unsigned  B2mobs0LGiven   :1;
    unsigned  B2mobs0WGiven   :1;
    unsigned  B2mobsB0Given   :1;
    unsigned  B2mobsBLGiven   :1;
    unsigned  B2mobsBWGiven   :1;
    unsigned  B2mob200Given   :1;
    unsigned  B2mob20LGiven   :1;
    unsigned  B2mob20WGiven   :1;
    unsigned  B2mob2B0Given   :1;
    unsigned  B2mob2BLGiven   :1;
    unsigned  B2mob2BWGiven   :1;
    unsigned  B2mob2G0Given   :1;
    unsigned  B2mob2GLGiven   :1;
    unsigned  B2mob2GWGiven   :1;
    unsigned  B2mob300Given   :1;
    unsigned  B2mob30LGiven   :1;
    unsigned  B2mob30WGiven   :1;
    unsigned  B2mob3B0Given   :1;
    unsigned  B2mob3BLGiven   :1;
    unsigned  B2mob3BWGiven   :1;
    unsigned  B2mob3G0Given   :1;
    unsigned  B2mob3GLGiven   :1;
    unsigned  B2mob3GWGiven   :1;
    unsigned  B2mob400Given   :1;
    unsigned  B2mob40LGiven   :1;
    unsigned  B2mob40WGiven   :1;
    unsigned  B2mob4B0Given   :1;
    unsigned  B2mob4BLGiven   :1;
    unsigned  B2mob4BWGiven   :1;
    unsigned  B2mob4G0Given   :1;
    unsigned  B2mob4GLGiven   :1;
    unsigned  B2mob4GWGiven   :1;
    unsigned  B2ua00Given   :1;
    unsigned  B2ua0LGiven   :1;
    unsigned  B2ua0WGiven   :1;
    unsigned  B2uaB0Given   :1;
    unsigned  B2uaBLGiven   :1;
    unsigned  B2uaBWGiven   :1;
    unsigned  B2ub00Given   :1;
    unsigned  B2ub0LGiven   :1;
    unsigned  B2ub0WGiven   :1;
    unsigned  B2ubB0Given   :1;
    unsigned  B2ubBLGiven   :1;
    unsigned  B2ubBWGiven   :1;
    unsigned  B2u100Given   :1;
    unsigned  B2u10LGiven   :1;
    unsigned  B2u10WGiven   :1;
    unsigned  B2u1B0Given   :1;
    unsigned  B2u1BLGiven   :1;
    unsigned  B2u1BWGiven   :1;
    unsigned  B2u1D0Given   :1;
    unsigned  B2u1DLGiven   :1;
    unsigned  B2u1DWGiven   :1;
    unsigned  B2n00Given   :1;
    unsigned  B2n0LGiven   :1;
    unsigned  B2n0WGiven   :1;
    unsigned  B2nB0Given   :1;
    unsigned  B2nBLGiven   :1;
    unsigned  B2nBWGiven   :1;
    unsigned  B2nD0Given   :1;
    unsigned  B2nDLGiven   :1;
    unsigned  B2nDWGiven   :1;
    unsigned  B2vof00Given   :1;
    unsigned  B2vof0LGiven   :1;
    unsigned  B2vof0WGiven   :1;
    unsigned  B2vofB0Given   :1;
    unsigned  B2vofBLGiven   :1;
    unsigned  B2vofBWGiven   :1;
    unsigned  B2vofD0Given   :1;
    unsigned  B2vofDLGiven   :1;
    unsigned  B2vofDWGiven   :1;
    unsigned  B2ai00Given   :1;
    unsigned  B2ai0LGiven   :1;
    unsigned  B2ai0WGiven   :1;
    unsigned  B2aiB0Given   :1;
    unsigned  B2aiBLGiven   :1;
    unsigned  B2aiBWGiven   :1;
    unsigned  B2bi00Given   :1;
    unsigned  B2bi0LGiven   :1;
    unsigned  B2bi0WGiven   :1;
    unsigned  B2biB0Given   :1;
    unsigned  B2biBLGiven   :1;
    unsigned  B2biBWGiven   :1;
    unsigned  B2vghigh0Given    :1;
    unsigned  B2vghighLGiven    :1;
    unsigned  B2vghighWGiven    :1;
    unsigned  B2vglow0Given     :1;
    unsigned  B2vglowLGiven     :1;
    unsigned  B2vglowWGiven     :1;
    unsigned  B2toxGiven   :1;
    unsigned  B2tempGiven   :1;
    unsigned  B2vddGiven   :1;
    unsigned  B2vggGiven   :1;
    unsigned  B2vbbGiven   :1;
    unsigned  B2gateSourceOverlapCapGiven   :1;
    unsigned  B2gateDrainOverlapCapGiven   :1;
    unsigned  B2gateBulkOverlapCapGiven   :1;
    unsigned  B2channelChargePartitionFlagGiven   :1;
    unsigned  B2sheetResistanceGiven   :1;
    unsigned  B2jctSatCurDensityGiven   :1;
    unsigned  B2bulkJctPotentialGiven   :1;
    unsigned  B2bulkJctBotGradingCoeffGiven   :1;
    unsigned  B2sidewallJctPotentialGiven   :1;
    unsigned  B2bulkJctSideGradingCoeffGiven   :1;
    unsigned  B2unitAreaJctCapGiven   :1;
    unsigned  B2unitLengthSidewallJctCapGiven   :1;
    unsigned  B2defaultWidthGiven   :1;
    unsigned  B2deltaLengthGiven   :1;
    unsigned  B2typeGiven   :1;
};

} // namespace B2
using namespace B2;


#ifndef NMOS
#define NMOS 1
#define PMOS -1
#endif

// device parameters
enum {
    BSIM2_W = 1,
    BSIM2_L,
    BSIM2_AS,
    BSIM2_AD,
    BSIM2_PS,
    BSIM2_PD,
    BSIM2_NRS,
    BSIM2_NRD,
    BSIM2_OFF,
    BSIM2_IC_VBS,
    BSIM2_IC_VDS,
    BSIM2_IC_VGS,
    BSIM2_IC,
    BSIM2_M,

    BSIM2_VBD,
    BSIM2_VBS,
    BSIM2_VGS,
    BSIM2_VDS,
    BSIM2_VON,
    BSIM2_CD,
    BSIM2_CBS,
    BSIM2_CBD,
    BSIM2_SOURCECOND,
    BSIM2_DRAINCOND,
    BSIM2_GM,
    BSIM2_GDS,
    BSIM2_GMBS,
    BSIM2_GBD,
    BSIM2_GBS,
    BSIM2_QB,
    BSIM2_QG,
    BSIM2_QD,
    BSIM2_QBS,
    BSIM2_QBD,
    BSIM2_CQB,
    BSIM2_CQG,
    BSIM2_CQD,
    BSIM2_CGG,
    BSIM2_CGD,
    BSIM2_CGS,
    BSIM2_CBG,
    BSIM2_CAPBD,
    BSIM2_CQBD,
    BSIM2_CAPBS,
    BSIM2_CQBS,
    BSIM2_CDG,
    BSIM2_CDD,
    BSIM2_CDS,
    BSIM2_DNODE,
    BSIM2_GNODE,
    BSIM2_SNODE,
    BSIM2_BNODE,
    BSIM2_DNODEPRIME,
    BSIM2_SNODEPRIME 
};

// model parameters
enum {
    BSIM2_MOD_VFB0 = 1000,
    BSIM2_MOD_VFBL,
    BSIM2_MOD_VFBW,
    BSIM2_MOD_PHI0,
    BSIM2_MOD_PHIL,
    BSIM2_MOD_PHIW,
    BSIM2_MOD_K10,
    BSIM2_MOD_K1L,
    BSIM2_MOD_K1W,
    BSIM2_MOD_K20,
    BSIM2_MOD_K2L,
    BSIM2_MOD_K2W,
    BSIM2_MOD_ETA00,
    BSIM2_MOD_ETA0L,
    BSIM2_MOD_ETA0W,
    BSIM2_MOD_ETAB0,
    BSIM2_MOD_ETABL,
    BSIM2_MOD_ETABW,
    BSIM2_MOD_DELTAL,
    BSIM2_MOD_DELTAW,
    BSIM2_MOD_MOB00,
    BSIM2_MOD_MOB0B0,
    BSIM2_MOD_MOB0BL,
    BSIM2_MOD_MOB0BW,
    BSIM2_MOD_MOBS00,
    BSIM2_MOD_MOBS0L,
    BSIM2_MOD_MOBS0W,
    BSIM2_MOD_MOBSB0,
    BSIM2_MOD_MOBSBL,
    BSIM2_MOD_MOBSBW,
    BSIM2_MOD_MOB200,
    BSIM2_MOD_MOB20L,
    BSIM2_MOD_MOB20W,
    BSIM2_MOD_MOB2B0,
    BSIM2_MOD_MOB2BL,
    BSIM2_MOD_MOB2BW,
    BSIM2_MOD_MOB2G0,
    BSIM2_MOD_MOB2GL,
    BSIM2_MOD_MOB2GW,
    BSIM2_MOD_MOB300,
    BSIM2_MOD_MOB30L,
    BSIM2_MOD_MOB30W,
    BSIM2_MOD_MOB3B0,
    BSIM2_MOD_MOB3BL,
    BSIM2_MOD_MOB3BW,
    BSIM2_MOD_MOB3G0,
    BSIM2_MOD_MOB3GL,
    BSIM2_MOD_MOB3GW,
    BSIM2_MOD_MOB400,
    BSIM2_MOD_MOB40L,
    BSIM2_MOD_MOB40W,
    BSIM2_MOD_MOB4B0,
    BSIM2_MOD_MOB4BL,
    BSIM2_MOD_MOB4BW,
    BSIM2_MOD_MOB4G0,
    BSIM2_MOD_MOB4GL,
    BSIM2_MOD_MOB4GW,
    BSIM2_MOD_UA00,
    BSIM2_MOD_UA0L,
    BSIM2_MOD_UA0W,
    BSIM2_MOD_UAB0,
    BSIM2_MOD_UABL,
    BSIM2_MOD_UABW,
    BSIM2_MOD_UB00,
    BSIM2_MOD_UB0L,
    BSIM2_MOD_UB0W,
    BSIM2_MOD_UBB0,
    BSIM2_MOD_UBBL,
    BSIM2_MOD_UBBW,
    BSIM2_MOD_U100,
    BSIM2_MOD_U10L,
    BSIM2_MOD_U10W,
    BSIM2_MOD_U1B0,
    BSIM2_MOD_U1BL,
    BSIM2_MOD_U1BW,
    BSIM2_MOD_U1D0,
    BSIM2_MOD_U1DL,
    BSIM2_MOD_U1DW,
    BSIM2_MOD_N00,
    BSIM2_MOD_N0L,
    BSIM2_MOD_N0W,
    BSIM2_MOD_NB0,
    BSIM2_MOD_NBL,
    BSIM2_MOD_NBW,
    BSIM2_MOD_ND0,
    BSIM2_MOD_NDL,
    BSIM2_MOD_NDW,
    BSIM2_MOD_VOF00,
    BSIM2_MOD_VOF0L,
    BSIM2_MOD_VOF0W,
    BSIM2_MOD_VOFB0,
    BSIM2_MOD_VOFBL,
    BSIM2_MOD_VOFBW,
    BSIM2_MOD_VOFD0,
    BSIM2_MOD_VOFDL,
    BSIM2_MOD_VOFDW,
    BSIM2_MOD_AI00,
    BSIM2_MOD_AI0L,
    BSIM2_MOD_AI0W,
    BSIM2_MOD_AIB0,
    BSIM2_MOD_AIBL,
    BSIM2_MOD_AIBW,
    BSIM2_MOD_BI00,
    BSIM2_MOD_BI0L,
    BSIM2_MOD_BI0W,
    BSIM2_MOD_BIB0,
    BSIM2_MOD_BIBL,
    BSIM2_MOD_BIBW,
    BSIM2_MOD_VGHIGH0,
    BSIM2_MOD_VGHIGHL,
    BSIM2_MOD_VGHIGHW,
    BSIM2_MOD_VGLOW0,
    BSIM2_MOD_VGLOWL,
    BSIM2_MOD_VGLOWW,
    BSIM2_MOD_TOX,
    BSIM2_MOD_TEMP,
    BSIM2_MOD_VDD,
    BSIM2_MOD_VGG,
    BSIM2_MOD_VBB,
    BSIM2_MOD_CGSO,
    BSIM2_MOD_CGDO,
    BSIM2_MOD_CGBO,
    BSIM2_MOD_XPART,
    BSIM2_MOD_RSH,
    BSIM2_MOD_JS,
    BSIM2_MOD_PB,
    BSIM2_MOD_MJ,
    BSIM2_MOD_PBSW,
    BSIM2_MOD_MJSW,
    BSIM2_MOD_CJ,
    BSIM2_MOD_CJSW,
    BSIM2_MOD_DEFWIDTH,
    BSIM2_MOD_DELLENGTH,
    BSIM2_MOD_NMOS,
    BSIM2_MOD_PMOS 
};

#endif // B2DEFS_H

