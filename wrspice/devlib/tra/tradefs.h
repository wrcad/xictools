
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

//-------------------------------------------------------------------------
// This is a general transmission line model derived from:
//  1) the spice3 TRA (lossless) model
//  2) the spice3 LTRA (lossy, convolution) model
//  3) the kspice TXL (lossy, Pade approximation convolution) model
// Authors:
//  1985 Thomas L. Quarles
//  1990 Jaijeet S. Roychowdhury
//  1990 Shen Lin
//  1992 Charles Hough
//  2002 Stephen R. Whiteley
// Copyright Regents of the University of California.  All rights reserved.
//-------------------------------------------------------------------------

#ifndef TRADEFS_H
#define TRADEFS_H

#include "device.h"

//
// definitions used to describe transmission liness
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

#define PADE_LEVEL 1
#define CONV_LEVEL 2

namespace TRA {

struct TRAdev : public IFdevice
{
    TRAdev();
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
//    int getic(sGENmodel*, sCKT*);  
    int accept(sCKT*, sGENmodel*); 
    int trunc(sGENmodel*, sCKT*, double*);  
//    int convTest(sGENmodel*, sCKT*);  

    int setInst(int, IFdata*, sGENinstance*);  
    int setModl(int, IFdata*, sGENmodel*);   
    int askInst(const sCKT*, const sGENinstance*, int, IFdata*);    
    int askModl(const sGENmodel*, int, IFdata*); 
//    int findBranch(sCKT*, sGENmodel*, IFuid); 

    int acLoad(sGENmodel*, sCKT*); 
//    int pzLoad(sGENmodel*, sCKT*, IFcomplex*); 
//    int disto(int, sGENmodel*, sCKT*);  
//    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
};

struct sTRAtimeval
{
    double v_i, v_o;
    double i_i, i_o;  
};

struct TERM
{
    double c, x;
    double cnv_i, cnv_o;
};

struct TXLine
{
    TXLine()
        {
            memset(this, 0, sizeof(TXLine));
        }

    bool main_pade(double, double, double, double, double);
    void get_h3();
    void update_h1C_c();
    void copy_to(TXLine*);
    void update_cnv(double);
    void update_delayed_cnv(double, sTRAtimeval*);

    int       lsl;      // 1 if the line is lossless, otherwise 0
    int       ext;      // a flag, set if time step is greater than tau
    int       newtp;    // flag indicating new time point
    int       ifImg;    // set to 1 if non-real roots found
    int       tv_head;  // index into time values
    double    ratio;
    double    taul;
    double    sqtCdL;
    double    h2_aten;
    double    h3_aten;
    double    h1C;
    double    h1e[3];
    double    dc1, dc2;
    double    Vin;
    double    dVin;
    double    Vout;
    double    dVout;
    TERM      h1_term[3];
    TERM      h2_term[3];
    TERM      h3_term[6];
};

struct sTRAinstance;

struct sTRAconvModel
{
    sTRAconvModel()
        {
            memset(this, 0, sizeof(sTRAconvModel));
        }

    ~sTRAconvModel()
        {
            delete [] TRAh1dashCoeffs;
            delete [] TRAh2Coeffs;
            delete [] TRAh3dashCoeffs;
        }

    int setup(sCKT*, sTRAinstance*);
    void rcCoeffsSetup(sCKT*);       
    void rlcCoeffsSetup(sCKT*);
    double rlcH2Func(double);
    double rlcH3dashFunc(double, double, double, double);
    double lteCalculate(sCKT*, sTRAinstance*, double);

    double TRAl;             // inductance per length
    double TRAc;             // capacitance per length
    double TRAr;             // resistance per length
    double TRAg;             // conductance per length
    double TRAlength;        // length
    double TRAtd;            // delay

    double TRAadmit;         // admittance Y - calculated
    double TRAalpha;         // alpha - calculated
    double TRAbeta;          // beta - calculated
    double TRAattenuation;   // e^(-beta T) - calculated
    double TRAcByR;          // C/R - for the RC line - calculated
    double TRArclsqr;        // RCl^2 - for the RC line - calculated
    double TRAintH1dash;     // int 0-inf h'1(tau) d tau - calculated
    double TRAintH2;         // int 0-inf h 2(tau) d tau - calculated
    double TRAintH3dash;     // int 0-inf h'3(tau) d tau - calculated
    double TRAh1dashFirstCoeff; // first coeff of h1dash for current timepoint
    double TRAh2FirstCoeff;     // first coeff of h2 for current timepoint
    double TRAh3dashFirstCoeff; // first coeff of h3dash for current timepoint

    double TRAcoshlrootGR;   // cosh(l*sqrt(G*R)), used for DC anal
    double TRArRsLrGRorG;    // sqrt(R)*sinh(l*sqrt(G*R))/sqrt(G)
    double TRArGsLrGRorR;    // sqrt(G)*sinh(l*sqrt(G*R))/sqrt(R)

    double *TRAh1dashCoeffs; // list of other coefficients for h1dash
    double *TRAh2Coeffs;     // list of other coefficients for h2
    double *TRAh3dashCoeffs; // list of other coefficients for h3dash
    int TRAmodelListSize;    // size of above lists
    int TRAauxIndex;         // auxiliary index for h2 and h3dash

    sTRAconvModel *next;
};

struct ltrastuff;

struct sTRAinstance : sGENinstance
{
    sTRAinstance()
        {
            memset(this, 0, sizeof(sTRAinstance));
            GENnumNodes = 4;
        }
    ~sTRAinstance() { delete [] TRAvalues; }
    sTRAinstance *next()
        { return (static_cast<sTRAinstance*>(GENnextInstance)); }
    const char *tranline_params();
    int pade_setup(sCKT*);
    int ltra_setup(sCKT*);
    int accept(sCKT*, int*);
    int set_breaks(sCKT*);
    int limit_timestep(sCKT*, double*, double);
    int pade_load(sCKT*);
    int pade_pred(sCKT*, double, double, double, double, double*);
    int ltra_load(sCKT*);
    int ltra_pred(sCKT*, ltrastuff*);

    int TRAposNode1;    // positive node of end 1 of t. line
    int TRAnegNode1;    // negative node of end 1 of t. line
    int TRAposNode2;    // positive node of end 2 of t. line
    int TRAnegNode2;    // negative node of end 2 of t. line

    int TRAcase;        // TRA_LC, etc.
    int TRAlevel;       // 0: Pade approx, 1: full convolution
    int TRAbrEq1;       // number of branch equation for end 1 of t. line
    int TRAbrEq2;       // number of branch equation for end 2 of t. line

    double TRAlength;   // length, arbitrary units
    double TRAl;        // inductance per length
    double TRAc;        // capacitance per length
    double TRAr;        // resistance per length
    double TRAg;        // conductance per length
    double TRAz;        // impedance, sqrt(L/C)
    double TRAtd;       // propagation delay, 1/sqrt(LC)
    double TRAnl;       // normalized length
    double TRAf;        // frequency at which nl is measured

    double TRAinput1;       // accumulated excitation for port 1
    double TRAinput2;       // accumulated excitation for port 2
    double TRAinitVolt1;    // initial condition:  voltage on port 1
    double TRAinitCur1;     // initial condition:  current at port 1
    double TRAinitVolt2;    // initial condition:  voltage on port 2
    double TRAinitCur2;     // initial condition:  current at port 2
    double TRAreltol;       // not used
    double TRAabstol;       // not used
    double TRAslopetol;     // reltol for slope timestep control
    double TRAstLineReltol; // reltol for checking straight lines
    double TRAstLineAbstol; // abstol for checking straight lines
    double TRAmaxSafeStep;  // step limit
    double TRAtemp1;        // temp variables for caching
    double TRAtemp2;

    TXLine TRAtx;       // pointer to SWEC txline type
    TXLine TRAtx2;      // pointer to SWEC txline type. temporary storage

    sTRAconvModel *TRAconvModel;    // LTRA model parameters
    sTRAtimeval *TRAvalues;         // history values
    int TRAinstListSize;            // size of history list

    int TRAhowToInterp; // back time interpolation method
    int TRAlteConType;  // timetoint truncation method
    int TRAbreakType;   // breakpoint rescheduling method
    int TRAdoload;      // internal flag

    double *TRAibr1Pos1Ptr;     // pointers to sparse matrix
    double *TRAibr1Neg1Ptr;
    double *TRAibr1Pos2Ptr;
    double *TRAibr1Neg2Ptr;
    double *TRAibr1Ibr1Ptr;
    double *TRAibr1Ibr2Ptr;
    double *TRAibr2Pos1Ptr;
    double *TRAibr2Pos2Ptr;
    double *TRAibr2Neg1Ptr;
    double *TRAibr2Neg2Ptr;
    double *TRAibr2Ibr1Ptr;
    double *TRAibr2Ibr2Ptr;
    double *TRApos1Ibr1Ptr;
    double *TRAneg1Ibr1Ptr;
    double *TRApos2Ibr2Ptr;
    double *TRAneg2Ibr2Ptr;
    double *TRApos1Pos1Ptr;
    double *TRAneg1Neg1Ptr;
    double *TRApos2Pos2Ptr;
    double *TRAneg2Neg2Ptr;

    unsigned TRAlevelGiven : 1;
    unsigned TRArGiven : 1;
    unsigned TRAlGiven : 1;
    unsigned TRAgGiven : 1;
    unsigned TRAcGiven : 1;
    unsigned TRAlengthGiven : 1;
    unsigned TRAzGiven : 1;
    unsigned TRAtdGiven : 1;
    unsigned TRAfGiven : 1;
    unsigned TRAnlGiven : 1;
    unsigned TRAinterpGiven : 1;
    unsigned TRAcutGiven : 1;
    unsigned TRAslopetolGiven : 1;
    unsigned TRAcompactrelGiven : 1;
    unsigned TRAcompactabsGiven : 1;
    unsigned TRAbreakGiven : 1;
    unsigned TRAreltolGiven : 1;
    unsigned TRAabstolGiven : 1;
};

struct sTRAmodel : sGENmodel
{
    sTRAmodel()     { memset(this, 0, sizeof(sTRAmodel)); }
    ~sTRAmodel()
        {
            while (TRAconvModels) {
                sTRAconvModel *n = TRAconvModels->next;
                delete TRAconvModels;
                TRAconvModels = n;
            }
        }
    sTRAmodel *next() { return (static_cast<sTRAmodel*>(GENnextModel)); }
    sTRAinstance *inst() { return (static_cast<sTRAinstance*>(GENinstances)); }

    double TRAlength;   // length, arbitrary units
    double TRAl;        // inductance per length
    double TRAc;        // capacitance per length
    double TRAr;        // resistance per length
    double TRAg;        // conductance per length
    double TRAz;        // impedance, sqrt(L/C)
    double TRAtd;       // propagation delay, 1/sqrt(LC)
    double TRAnl;       // normalized length
    double TRAf;        // frequency at which nl is measured

    double TRAreltol;       // relative deriv. tol. for breakpoint setting
    double TRAabstol;       // absolute deriv. tol. for breakpoint setting
    double TRAslopetol;     // reltol for slope timestep control
    double TRAstLineReltol; // reltol for checking straight lines
    double TRAstLineAbstol; // abstol for checking straight lines

    int TRAlevel;       // algorithm level
    int TRAhowToInterp; // back time interpolation method
    int TRAlteConType;  // timetoint truncation method
    int TRAbreakType;   // breakpoint rescheduling method

    unsigned TRAlengthGiven : 1;
    unsigned TRAlGiven : 1;
    unsigned TRAcGiven : 1;
    unsigned TRArGiven : 1;
    unsigned TRAgGiven : 1;
    unsigned TRAzGiven : 1;
    unsigned TRAtdGiven : 1;
    unsigned TRAnlGiven : 1;
    unsigned TRAfGiven : 1;

    unsigned TRAreltolGiven : 1;
    unsigned TRAabstolGiven : 1;
    unsigned TRAslopetolGiven : 1;
    unsigned TRAstLineReltolGiven : 1;
    unsigned TRAstLineAbstolGiven : 1;

    unsigned TRAlevelGiven : 1;
    unsigned TRAhowToInterpGiven : 1;
    unsigned TRAlteConTypeGiven : 1;
    unsigned TRAbreakTypeGiven : 1;

    sTRAconvModel *TRAconvModels;
};

} // namespace TRA
using namespace TRA;

// values for TRAcase
enum {
    TRA_LC = 1,
    TRA_RLC,
    TRA_RC,
    TRA_RG,
    TRA_RL
};

// device parameters
// DO NOT CHANGE THIS without updating aski/seti tables!
enum {
    TRA_LEVEL = 1,
    TRA_LENGTH,
    TRA_L,
    TRA_C,
    TRA_R,
    TRA_G,
    TRA_Z0,
    TRA_TD,
    TRA_FREQ,
    TRA_NL,
    TRA_LININTERP,
    TRA_QUADINTERP,
    TRA_TRUNCDONTCUT,
    TRA_TRUNCCUTSL,
    TRA_TRUNCCUTLTE,
    TRA_TRUNCCUTNR,
    TRA_NOBREAKS,
    TRA_ALLBREAKS,
    TRA_TESTBREAKS,
    TRA_SLOPETOL,
    TRA_COMPACTREL,
    TRA_COMPACTABS,
    TRA_RELTOL,
    TRA_ABSTOL,
    TRA_V1,
    TRA_I1,
    TRA_V2,
    TRA_I2,
    TRA_IC,
    TRA_QUERY_V1,
    TRA_QUERY_I1,
    TRA_QUERY_V2,
    TRA_QUERY_I2,
    TRA_POS_NODE1,
    TRA_NEG_NODE1,
    TRA_POS_NODE2,
    TRA_NEG_NODE2,
    TRA_BR_EQ1,
    TRA_BR_EQ2,
    TRA_INPUT1,
    TRA_INPUT2,
    TRA_DELAY,
    TRA_MAXSTEP
};

// model parameters
enum {
    TRA_MOD_TRA = 1000,
    TRA_MOD_LEVEL,
    TRA_MOD_LEN,
    TRA_MOD_L,
    TRA_MOD_C,
    TRA_MOD_R,
    TRA_MOD_G,
    TRA_MOD_Z0,
    TRA_MOD_TD,
    TRA_MOD_F,
    TRA_MOD_NL,
    TRA_MOD_LININTERP,
    TRA_MOD_QUADINTERP,
    TRA_MOD_TRUNCDONTCUT,
    TRA_MOD_TRUNCCUTSL,
    TRA_MOD_TRUNCCUTLTE,
    TRA_MOD_TRUNCCUTNR,
    TRA_MOD_NOBREAKS,
    TRA_MOD_ALLBREAKS,
    TRA_MOD_TESTBREAKS,
    TRA_MOD_SLOPETOL,
    TRA_MOD_COMPACTREL,
    TRA_MOD_COMPACTABS,
    TRA_MOD_RELTOL,
    TRA_MOD_ABSTOL,
    TRA_MOD_LTRA
};

#endif // TRADEFS_H

