
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
Authors: 1985 Hong June Park, Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#ifndef B1DEFS_H
#define B1DEFS_H

#include "device.h"

//
// declarations for B1 MOSFETs
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

namespace B1 {

struct sB1model;
struct sB1instance;

struct B1dev : public IFdevice
{
    B1dev();
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
    int disto(int, sGENmodel*, sCKT*);  
//    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
private:
    int dSetup(sB1model*, sCKT*);
    void evaluate(double, double, double, sB1instance*, sB1model*, double*,
        double*, double*, double*, double*, double*, double*, double*,
        double*, double*, double*, double*, double*, double*, double*,
        double*, double*, double*, sCKT*);
    void mosCap(sCKT*, double, double, double, double*, double,
        double, double, double, double, double, double*, double*,
        double*, double*, double*, double*, double*, double*, double*,
        double*, double*, double*, double*, double*, double*, double*);
};

#define B1numStates 35           
#define B1NDCOEFFS    82

struct sB1instance : sGENinstance
{
    sB1instance()
        {
            memset(this, 0, sizeof(sB1instance));
            GENnumNodes = 4;
        }
    sB1instance *next()
        { return (static_cast<sB1instance*>(GENnextInstance)); }

    int B1dNode;   // number of the gate node of the mosfet
    int B1gNode;   // number of the gate node of the mosfet
    int B1sNode;   // number of the source node of the mosfet
    int B1bNode;   // number of the bulk node of the mosfet

    int B1dNodePrime; // number of the internal drain node of the mosfet
    int B1sNodePrime; // number of the internal source node of the mosfet

    double B1m;
    double B1l;    // the length of the channel region
    double B1w;    // the width of the channel region
    double B1drainArea;   // the area of the drain diffusion
    double B1sourceArea;  // the area of the source diffusion
    double B1drainSquares;    // the length of the drain in squares
    double B1sourceSquares;   // the length of the source in squares
    double B1drainPerimeter;
    double B1sourcePerimeter;
    double B1sourceConductance;   // conductance of source(or 0):set in setup
    double B1drainConductance;    // conductance of drain(or 0):set in setup

    double B1icVBS;   // initial condition B-S voltage
    double B1icVDS;   // initial condition D-S voltage
    double B1icVGS;   // initial condition G-S voltage
    double B1von;
    double B1vdsat;
    int B1off;        // non-zero to indicate device is off for dc analysis
    int B1mode;       // device mode : 1 = normal, -1 = inverse

    double B1vfb;       // flat band voltage at given L and W
    double B1phi;       // surface potential at strong inversion
    double B1K1;        // bulk effect coefficient 1
    double B1K2;        // bulk effect coefficient 2
    double B1eta;       // drain induced barrier lowering
    double B1etaB;      // Vbs dependence of Eta
    double B1etaD;      // Vds dependence of Eta
    double B1betaZero;  // Beta at vds = 0 and vgs = Vth
    double B1betaZeroB; // Vbs dependence of BetaZero
    double B1betaVdd;   // Beta at vds=Vdd and vgs=Vth
    double B1betaVddB;  // Vbs dependence of BVdd
    double B1betaVddD;  // Vds dependence of BVdd
    double B1ugs;       // Mobility degradation due to gate field
    double B1ugsB;      // Vbs dependence of Ugs
    double B1uds;       // Drift Velocity Saturation due to Vds
    double B1udsB;      // Vbs dependence of Uds
    double B1udsD;      // Vds dependence of Uds
    double B1subthSlope; // slope of subthreshold current with Vgs
    double B1subthSlopeB; // Vbs dependence of Subthreshold Slope
    double B1subthSlopeD; // Vds dependence of Subthreshold Slope
    double B1GDoverlapCap;// Gate Drain Overlap Capacitance
    double B1GSoverlapCap;// Gate Source Overlap Capacitance
    double B1GBoverlapCap;// Gate Bulk Overlap Capacitance
    double B1vt0;
    double B1vdd;         // Supply Voltage
    double B1temp;
    double B1oxideThickness;
    double B1channelChargePartitionFlag;

    unsigned B1mGiven :1;
    unsigned B1lGiven :1;
    unsigned B1wGiven :1;
    unsigned B1drainAreaGiven :1;
    unsigned B1sourceAreaGiven    :1;
    unsigned B1drainSquaresGiven  :1;
    unsigned B1sourceSquaresGiven :1;
    unsigned B1drainPerimeterGiven    :1;
    unsigned B1sourcePerimeterGiven   :1;
    unsigned B1dNodePrimeSet  :1;
    unsigned B1sNodePrimeSet  :1;
    unsigned B1icVBSGiven :1;
    unsigned B1icVDSGiven :1;
    unsigned B1icVGSGiven :1;
    unsigned B1vonGiven   :1;
    unsigned B1vdsatGiven :1;


    double *B1DdPtr;      // pointer to sparse matrix element at
                          //  (Drain node,drain node)
    double *B1GgPtr;      // pointer to sparse matrix element at
                          //  (gate node,gate node)
    double *B1SsPtr;      // pointer to sparse matrix element at
                          //  (source node,source node)
    double *B1BbPtr;      // pointer to sparse matrix element at
                          //  (bulk node,bulk node)
    double *B1DPdpPtr;    // pointer to sparse matrix element at
                          //  (drain prime node,drain prime node)
    double *B1SPspPtr;    // pointer to sparse matrix element at
                          //  (source prime node,source prime node)
    double *B1DdpPtr;     // pointer to sparse matrix element at
                          //  (drain node,drain prime node)
    double *B1GbPtr;      // pointer to sparse matrix element at
                          //  (gate node,bulk node)
    double *B1GdpPtr;     // pointer to sparse matrix element at
                          //  (gate node,drain prime node)
    double *B1GspPtr;     // pointer to sparse matrix element at
                          //  (gate node,source prime node)
    double *B1SspPtr;     // pointer to sparse matrix element at
                          //  (source node,source prime node)
    double *B1BdpPtr;     // pointer to sparse matrix element at
                          //  (bulk node,drain prime node)
    double *B1BspPtr;     // pointer to sparse matrix element at
                          //  (bulk node,source prime node)
    double *B1DPspPtr;    // pointer to sparse matrix element at
                          //  (drain prime node,source prime node)
    double *B1DPdPtr;     // pointer to sparse matrix element at
                          //  (drain prime node,drain node)
    double *B1BgPtr;      // pointer to sparse matrix element at
                          //  (bulk node,gate node)
    double *B1DPgPtr;     // pointer to sparse matrix element at
                          //  (drain prime node,gate node)

    double *B1SPgPtr;     // pointer to sparse matrix element at
                          //  (source prime node,gate node)
    double *B1SPsPtr;     // pointer to sparse matrix element at
                          // (source prime node,source node)
    double *B1DPbPtr;     // pointer to sparse matrix element at
                          //  (drain prime node,bulk node)
    double *B1SPbPtr;     // pointer to sparse matrix element at
                          //  (source prime node,bulk node)
    double *B1SPdpPtr;    // pointer to sparse matrix element at
                          //  (source prime node,drain prime node)

#ifndef NODISTO
    double B1dCoeffs[B1NDCOEFFS];
#else
    double *B1dCoeffs;
#endif

};

#define B1vbd   GENstate + 0
#define B1vbs   GENstate + 1
#define B1vgs   GENstate + 2
#define B1vds   GENstate + 3
#define B1cd    GENstate + 4
#define B1id    GENstate + 4
#define B1cbs   GENstate + 5
#define B1ibs   GENstate + 5
#define B1cbd   GENstate + 6
#define B1ibd   GENstate + 6
#define B1gm    GENstate + 7
#define B1gds   GENstate + 8
#define B1gmbs  GENstate + 9
#define B1gbd   GENstate + 10
#define B1gbs   GENstate + 11
#define B1qb    GENstate + 12
#define B1cqb   GENstate + 13
#define B1iqb   GENstate + 13
#define B1qg    GENstate + 14
#define B1cqg   GENstate + 15
#define B1iqg   GENstate + 15
#define B1qd    GENstate + 16
#define B1cqd   GENstate + 17
#define B1iqd   GENstate + 17
#define B1cggb  GENstate + 18
#define B1cgdb  GENstate + 19
#define B1cgsb  GENstate + 20
#define B1cbgb  GENstate + 21
#define B1cbdb  GENstate + 22
#define B1cbsb  GENstate + 23
#define B1capbd GENstate + 24
#define B1iqbd  GENstate + 25
#define B1cqbd  GENstate + 25
#define B1capbs GENstate + 26
#define B1iqbs  GENstate + 27
#define B1cqbs  GENstate + 27
#define B1cdgb  GENstate + 28
#define B1cddb  GENstate + 29
#define B1cdsb  GENstate + 30
#define B1vono  GENstate + 31
#define B1vdsato GENstate + 32
#define B1qbs   GENstate + 33
#define B1qbd   GENstate + 34

//
// the following naming convention is used:
// x = vgs
// y = vbs
// z = vds
// DrC is the DrCur
// therefore qg_xyz stands for the coefficient of the vgs*vbs*vds
// term in the multidimensional Taylor expansion for qg; and DrC_x2y
// for the coeff. of the vgs*vgs*vbs term in the DrC expansion.
//

#ifdef DISTO
#define    qg_x        B1dCoeffs[0]
#define    qg_y        B1dCoeffs[1]
#define    qg_z        B1dCoeffs[2]
#define    qg_x2       B1dCoeffs[3]
#define    qg_y2       B1dCoeffs[4]
#define    qg_z2       B1dCoeffs[5]
#define    qg_xy       B1dCoeffs[6]
#define    qg_yz       B1dCoeffs[7]
#define    qg_xz       B1dCoeffs[8]
#define    qg_x3       B1dCoeffs[9]
#define    qg_y3       B1dCoeffs[10]
#define    qg_z3       B1dCoeffs[11]
#define    qg_x2z      B1dCoeffs[12]
#define    qg_x2y      B1dCoeffs[13]
#define    qg_y2z      B1dCoeffs[14]
#define    qg_xy2      B1dCoeffs[15]
#define    qg_xz2      B1dCoeffs[16]
#define    qg_yz2      B1dCoeffs[17]
#define    qg_xyz      B1dCoeffs[18]
#define    qb_x        B1dCoeffs[19]
#define    qb_y        B1dCoeffs[20]
#define    qb_z        B1dCoeffs[21]
#define    qb_x2       B1dCoeffs[22]
#define    qb_y2       B1dCoeffs[23]
#define    qb_z2       B1dCoeffs[24]
#define    qb_xy       B1dCoeffs[25]
#define    qb_yz       B1dCoeffs[26]
#define    qb_xz       B1dCoeffs[27]
#define    qb_x3       B1dCoeffs[28]
#define    qb_y3       B1dCoeffs[29]
#define    qb_z3       B1dCoeffs[30]
#define    qb_x2z      B1dCoeffs[31]
#define    qb_x2y      B1dCoeffs[32]
#define    qb_y2z      B1dCoeffs[33]
#define    qb_xy2      B1dCoeffs[34]
#define    qb_xz2      B1dCoeffs[35]
#define    qb_yz2      B1dCoeffs[36]
#define    qb_xyz      B1dCoeffs[37]
#define    qd_x        B1dCoeffs[38]
#define    qd_y        B1dCoeffs[39]
#define    qd_z        B1dCoeffs[40]
#define    qd_x2       B1dCoeffs[41]
#define    qd_y2       B1dCoeffs[42]
#define    qd_z2       B1dCoeffs[43]
#define    qd_xy       B1dCoeffs[44]
#define    qd_yz       B1dCoeffs[45]
#define    qd_xz       B1dCoeffs[46]
#define    qd_x3       B1dCoeffs[47]
#define    qd_y3       B1dCoeffs[48]
#define    qd_z3       B1dCoeffs[49]
#define    qd_x2z      B1dCoeffs[50]
#define    qd_x2y      B1dCoeffs[51]
#define    qd_y2z      B1dCoeffs[52]
#define    qd_xy2      B1dCoeffs[53]
#define    qd_xz2      B1dCoeffs[54]
#define    qd_yz2      B1dCoeffs[55]
#define    qd_xyz      B1dCoeffs[56]
#define    DrC_x       B1dCoeffs[57]
#define    DrC_y       B1dCoeffs[58]
#define    DrC_z       B1dCoeffs[59]
#define    DrC_x2      B1dCoeffs[60]
#define    DrC_y2      B1dCoeffs[61]
#define    DrC_z2      B1dCoeffs[62]
#define    DrC_xy      B1dCoeffs[63]
#define    DrC_yz      B1dCoeffs[64]
#define    DrC_xz      B1dCoeffs[65]
#define    DrC_x3      B1dCoeffs[66]
#define    DrC_y3      B1dCoeffs[67]
#define    DrC_z3      B1dCoeffs[68]
#define    DrC_x2z     B1dCoeffs[69]
#define    DrC_x2y     B1dCoeffs[70]
#define    DrC_y2z     B1dCoeffs[71]
#define    DrC_xy2     B1dCoeffs[72]
#define    DrC_xz2     B1dCoeffs[73]
#define    DrC_yz2     B1dCoeffs[74]
#define    DrC_xyz     B1dCoeffs[75]
#define    gbs1        B1dCoeffs[76]
#define    gbs2        B1dCoeffs[77]
#define    gbs3        B1dCoeffs[78]
#define    gbd1        B1dCoeffs[79]
#define    gbd2        B1dCoeffs[80]
#define    gbd3        B1dCoeffs[81]
#endif

struct sB1model : sGENmodel
{
    sB1model()          { memset(this, 0, sizeof(sB1model)); }
    sB1model *next()    { return (static_cast<sB1model*>(GENnextModel)); }
    sB1instance *inst() { return (static_cast<sB1instance*>(GENinstances)); }

    int B1type;       // device type : 1 = nmos,  -1 = pmos

    double B1vfb0;
    double B1vfbL;
    double B1vfbW;
    double B1phi0;
    double B1phiL;
    double B1phiW;
    double B1K10;
    double B1K1L;
    double B1K1W;
    double B1K20;
    double B1K2L;
    double B1K2W;
    double B1eta0;
    double B1etaL;
    double B1etaW;
    double B1etaB0;
    double B1etaBl;
    double B1etaBw;
    double B1etaD0;
    double B1etaDl;
    double B1etaDw;
    double B1deltaL;
    double B1deltaW;
    double B1mobZero;
    double B1mobZeroB0;
    double B1mobZeroBl;
    double B1mobZeroBw ;
    double B1mobVdd0;
    double B1mobVddl;
    double B1mobVddw;
    double B1mobVddB0;
    double B1mobVddBl;
    double B1mobVddBw;
    double B1mobVddD0;
    double B1mobVddDl;
    double B1mobVddDw;
    double B1ugs0;
    double B1ugsL;
    double B1ugsW;
    double B1ugsB0;
    double B1ugsBL;
    double B1ugsBW;
    double B1uds0;
    double B1udsL;
    double B1udsW;
    double B1udsB0;
    double B1udsBL;
    double B1udsBW;
    double B1udsD0;
    double B1udsDL;
    double B1udsDW;
    double B1subthSlope0;
    double B1subthSlopeL;
    double B1subthSlopeW;
    double B1subthSlopeB0;
    double B1subthSlopeBL;
    double B1subthSlopeBW;
    double B1subthSlopeD0;
    double B1subthSlopeDL;
    double B1subthSlopeDW;
    double B1oxideThickness;              // unit: micron
    double B1Cox;                         // unit: F/cm**2
    double B1temp;
    double B1vdd;
    double B1gateSourceOverlapCap;
    double B1gateDrainOverlapCap;
    double B1gateBulkOverlapCap;
    double B1channelChargePartitionFlag;

    double B1sheetResistance;
    double B1jctSatCurDensity;
    double B1bulkJctPotential;
    double B1bulkJctBotGradingCoeff;
    double B1bulkJctSideGradingCoeff;
    double B1sidewallJctPotential;
    double B1unitAreaJctCap;
    double B1unitLengthSidewallJctCap;
    double B1defaultWidth;
    double B1deltaLength;


    unsigned  B1vfb0Given   :1;
    unsigned  B1vfbLGiven   :1;
    unsigned  B1vfbWGiven   :1;
    unsigned  B1phi0Given   :1;
    unsigned  B1phiLGiven   :1;
    unsigned  B1phiWGiven   :1;
    unsigned  B1K10Given   :1;
    unsigned  B1K1LGiven   :1;
    unsigned  B1K1WGiven   :1;
    unsigned  B1K20Given   :1;
    unsigned  B1K2LGiven   :1;
    unsigned  B1K2WGiven   :1;
    unsigned  B1eta0Given   :1;
    unsigned  B1etaLGiven   :1;
    unsigned  B1etaWGiven   :1;
    unsigned  B1etaB0Given   :1;
    unsigned  B1etaBlGiven   :1;
    unsigned  B1etaBwGiven   :1;
    unsigned  B1etaD0Given   :1;
    unsigned  B1etaDlGiven   :1;
    unsigned  B1etaDwGiven   :1;
    unsigned  B1deltaLGiven   :1;
    unsigned  B1deltaWGiven   :1;
    unsigned  B1mobZeroGiven   :1;
    unsigned  B1mobZeroB0Given   :1;
    unsigned  B1mobZeroBlGiven   :1;
    unsigned  B1mobZeroBwGiven   :1;
    unsigned  B1mobVdd0Given   :1;
    unsigned  B1mobVddlGiven   :1;
    unsigned  B1mobVddwGiven   :1;
    unsigned  B1mobVddB0Given   :1;
    unsigned  B1mobVddBlGiven   :1;
    unsigned  B1mobVddBwGiven   :1;
    unsigned  B1mobVddD0Given   :1;
    unsigned  B1mobVddDlGiven   :1;
    unsigned  B1mobVddDwGiven   :1;
    unsigned  B1ugs0Given   :1;
    unsigned  B1ugsLGiven   :1;
    unsigned  B1ugsWGiven   :1;
    unsigned  B1ugsB0Given   :1;
    unsigned  B1ugsBLGiven   :1;
    unsigned  B1ugsBWGiven   :1;
    unsigned  B1uds0Given   :1;
    unsigned  B1udsLGiven   :1;
    unsigned  B1udsWGiven   :1;
    unsigned  B1udsB0Given   :1;
    unsigned  B1udsBLGiven   :1;
    unsigned  B1udsBWGiven   :1;
    unsigned  B1udsD0Given   :1;
    unsigned  B1udsDLGiven   :1;
    unsigned  B1udsDWGiven   :1;
    unsigned  B1subthSlope0Given   :1;
    unsigned  B1subthSlopeLGiven   :1;
    unsigned  B1subthSlopeWGiven   :1;
    unsigned  B1subthSlopeB0Given   :1;
    unsigned  B1subthSlopeBLGiven   :1;
    unsigned  B1subthSlopeBWGiven   :1;
    unsigned  B1subthSlopeD0Given   :1;
    unsigned  B1subthSlopeDLGiven   :1;
    unsigned  B1subthSlopeDWGiven   :1;
    unsigned  B1oxideThicknessGiven   :1;
    unsigned  B1tempGiven   :1;
    unsigned  B1vddGiven   :1;
    unsigned  B1gateSourceOverlapCapGiven   :1;
    unsigned  B1gateDrainOverlapCapGiven   :1;
    unsigned  B1gateBulkOverlapCapGiven   :1;
    unsigned  B1channelChargePartitionFlagGiven   :1;
    unsigned  B1sheetResistanceGiven   :1;
    unsigned  B1jctSatCurDensityGiven   :1;
    unsigned  B1bulkJctPotentialGiven   :1;
    unsigned  B1bulkJctBotGradingCoeffGiven   :1;
    unsigned  B1sidewallJctPotentialGiven   :1;
    unsigned  B1bulkJctSideGradingCoeffGiven   :1;
    unsigned  B1unitAreaJctCapGiven   :1;
    unsigned  B1unitLengthSidewallJctCapGiven   :1;
    unsigned  B1defaultWidthGiven   :1;
    unsigned  B1deltaLengthGiven   :1;
    unsigned  B1typeGiven   :1;
};

} // namespace B1
using namespace B1;


#ifndef NMOS
#define NMOS 1
#define PMOS -1
#endif

// device parameters
enum {
    BSIM1_W = 1,
    BSIM1_L,
    BSIM1_AS,
    BSIM1_AD,
    BSIM1_PS,
    BSIM1_PD,
    BSIM1_NRS,
    BSIM1_NRD,
    BSIM1_OFF,
    BSIM1_IC_VBS,
    BSIM1_IC_VDS,
    BSIM1_IC_VGS,
    BSIM1_IC,
    BSIM1_M,

    BSIM1_VBD,
    BSIM1_VBS,
    BSIM1_VGS,
    BSIM1_VDS,
    BSIM1_VON,
    BSIM1_CD,
    BSIM1_CBS,
    BSIM1_CBD,
    BSIM1_SOURCECOND,
    BSIM1_DRAINCOND,
    BSIM1_GM,
    BSIM1_GDS,
    BSIM1_GMBS,
    BSIM1_GBD,
    BSIM1_GBS,
    BSIM1_QB,
    BSIM1_QG,
    BSIM1_QD,
    BSIM1_QBS,
    BSIM1_QBD,
    BSIM1_CQB,
    BSIM1_CQG,
    BSIM1_CQD,
    BSIM1_CGG,
    BSIM1_CGD,
    BSIM1_CGS,
    BSIM1_CBG,
    BSIM1_CAPBD,
    BSIM1_CQBD,
    BSIM1_CAPBS,
    BSIM1_CQBS,
    BSIM1_CDG,
    BSIM1_CDD,
    BSIM1_CDS,
    BSIM1_DNODE,
    BSIM1_GNODE,
    BSIM1_SNODE,
    BSIM1_BNODE,
    BSIM1_DNODEPRIME,
    BSIM1_SNODEPRIME 
};

// model parameters
enum {
    BSIM1_MOD_VFB0 = 1000,
    BSIM1_MOD_VFBL,
    BSIM1_MOD_VFBW,
    BSIM1_MOD_PHI0,
    BSIM1_MOD_PHIL,
    BSIM1_MOD_PHIW,
    BSIM1_MOD_K10,
    BSIM1_MOD_K1L,
    BSIM1_MOD_K1W,
    BSIM1_MOD_K20,
    BSIM1_MOD_K2L,
    BSIM1_MOD_K2W,
    BSIM1_MOD_ETA0,
    BSIM1_MOD_ETAL,
    BSIM1_MOD_ETAW,
    BSIM1_MOD_ETAB0,
    BSIM1_MOD_ETABL,
    BSIM1_MOD_ETABW,
    BSIM1_MOD_ETAD0,
    BSIM1_MOD_ETADL,
    BSIM1_MOD_ETADW,
    BSIM1_MOD_DELTAL,
    BSIM1_MOD_DELTAW,
    BSIM1_MOD_MOBZERO,
    BSIM1_MOD_MOBZEROB0,
    BSIM1_MOD_MOBZEROBL,
    BSIM1_MOD_MOBZEROBW,
    BSIM1_MOD_MOBVDD0,
    BSIM1_MOD_MOBVDDL,
    BSIM1_MOD_MOBVDDW,
    BSIM1_MOD_MOBVDDB0,
    BSIM1_MOD_MOBVDDBL,
    BSIM1_MOD_MOBVDDBW,
    BSIM1_MOD_MOBVDDD0,
    BSIM1_MOD_MOBVDDDL,
    BSIM1_MOD_MOBVDDDW,
    BSIM1_MOD_UGS0,
    BSIM1_MOD_UGSL,
    BSIM1_MOD_UGSW,
    BSIM1_MOD_UGSB0,
    BSIM1_MOD_UGSBL,
    BSIM1_MOD_UGSBW,
    BSIM1_MOD_UDS0,
    BSIM1_MOD_UDSL,
    BSIM1_MOD_UDSW,
    BSIM1_MOD_UDSB0,
    BSIM1_MOD_UDSBL,
    BSIM1_MOD_UDSBW,
    BSIM1_MOD_UDSD0,
    BSIM1_MOD_UDSDL,
    BSIM1_MOD_UDSDW,
    BSIM1_MOD_N00,
    BSIM1_MOD_N0L,
    BSIM1_MOD_N0W,
    BSIM1_MOD_NB0,
    BSIM1_MOD_NBL,
    BSIM1_MOD_NBW,
    BSIM1_MOD_ND0,
    BSIM1_MOD_NDL,
    BSIM1_MOD_NDW,
    BSIM1_MOD_TOX,
    BSIM1_MOD_TEMP,
    BSIM1_MOD_VDD,
    BSIM1_MOD_CGSO,
    BSIM1_MOD_CGDO,
    BSIM1_MOD_CGBO,
    BSIM1_MOD_XPART,
    BSIM1_MOD_RSH,
    BSIM1_MOD_JS,
    BSIM1_MOD_PB,
    BSIM1_MOD_MJ,
    BSIM1_MOD_PBSW,
    BSIM1_MOD_MJSW,
    BSIM1_MOD_CJ,
    BSIM1_MOD_CJSW,
    BSIM1_MOD_DEFWIDTH,
    BSIM1_MOD_DELLENGTH,
    BSIM1_MOD_NMOS,
    BSIM1_MOD_PMOS 
};

#endif // B1DEFS_H

