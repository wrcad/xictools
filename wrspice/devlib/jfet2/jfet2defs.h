
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

/**********
Based on jfetdefs.h
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles

Modified to add PS model and new parameter definitions ( Anthony E. Parker )
   Copyright 1994  Macquarie University, Sydney Australia.
   10 Feb 1994: Added xiwoo, d3 and alpha to JFET2instance
                JFET2pave, JFET2vtrap ad JFET2_STATE_COUNT
                Changed model to call jfetparm.h, added JFET2za to model struct
                Defined JFET2_VTRAP and JFET2_PAVE
**********/

#ifndef JFET2DEFS_H
#define JFET2DEFS_H

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

namespace JFET2 {

struct sJFET2model;

struct JFET2dev : public IFdevice
{
    JFET2dev();
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
//    int pzLoad(sGENmodel*, sCKT*, IFcomplex*); 
//    int disto(int, sGENmodel*, sCKT*);  
    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
private:
    int dSetup(sJFET2model*, sCKT*);
};

struct sJFET2instance : public sGENinstance
{
    sJFET2instance()
        {
            memset(this, 0, sizeof(sJFET2instance));
            GENnumNodes = 3;
        }
    sJFET2instance *next()
        { return (static_cast<sJFET2instance*>(GENnextInstance)); }
    void ac_cd(const sCKT*, double*, double*) const;
    void ac_cs(const sCKT*, double*, double*) const;
    void ac_cg(const sCKT*, double*, double*) const;

    int JFET2drainNode;   // number of drain node of jfet
    int JFET2gateNode;    // number of gate node of jfet
    int JFET2sourceNode;  // number of source node of jfet

    int JFET2drainPrimeNode;   // number of internal drain node of jfet
    int JFET2sourcePrimeNode;  // number of internal source node of jfet

    double *JFET2drainDrainPrimePtr; /* pointer to sparse matrix at 
                                     * (drain,drain prime) */
    double *JFET2gateDrainPrimePtr;  /* pointer to sparse matrix at 
                                     * (gate,drain prime) */
    double *JFET2gateSourcePrimePtr; /* pointer to sparse matrix at 
                                     * (gate,source prime) */
    double *JFET2sourceSourcePrimePtr;   /* pointer to sparse matrix at 
                                         * (source,source prime) */
    double *JFET2drainPrimeDrainPtr; /* pointer to sparse matrix at 
                                     * (drain prime,drain) */
    double *JFET2drainPrimeGatePtr;  /* pointer to sparse matrix at 
                                     * (drain prime,gate) */
    double *JFET2drainPrimeSourcePrimePtr;   /* pointer to sparse matrix
                                             * (drain prime,source prime) */
    double *JFET2sourcePrimeGatePtr; /* pointer to sparse matrix at 
                                     * (source prime,gate) */
    double *JFET2sourcePrimeSourcePtr;   /* pointer to sparse matrix at 
                                         * (source prime,source) */
    double *JFET2sourcePrimeDrainPrimePtr;   /* pointer to sparse matrix
                                             * (source prime,drain prime) */
    double *JFET2drainDrainPtr;  /* pointer to sparse matrix at 
                                 * (drain,drain) */
    double *JFET2gateGatePtr;    /* pointer to sparse matrix at 
                                 * (gate,gate) */
    double *JFET2sourceSourcePtr;    /* pointer to sparse matrix at 
                                     * (source,source) */
    double *JFET2drainPrimeDrainPrimePtr;    /* pointer to sparse matrix
                                             * (drain prime,drain prime) */
    double *JFET2sourcePrimeSourcePrimePtr;  /* pointer to sparse matrix
                                             * (source prime,source prime) */

	int JFET2mode;
	/* distortion analysis Taylor coeffs. */

/*
 * naming convention:
 * x = vgs
 * y = vds
 * cdr = cdrain
 */

#ifdef notdef
#define JFET2NDCOEFFS	21

#ifndef NODISTO
	double JFET2dCoeffs[JFET2NDCOEFFS];
#else /* NODISTO */
	double *JFET2dCoeffs;
#endif /* NODISTO */

#ifndef CONFIG

#define	cdr_x		JFET2dCoeffs[0]
#define	cdr_y		JFET2dCoeffs[1]
#define	cdr_x2		JFET2dCoeffs[2]
#define	cdr_y2		JFET2dCoeffs[3]
#define	cdr_xy		JFET2dCoeffs[4]
#define	cdr_x3		JFET2dCoeffs[5]
#define	cdr_y3		JFET2dCoeffs[6]
#define	cdr_x2y		JFET2dCoeffs[7]
#define	cdr_xy2		JFET2dCoeffs[8]

#define	ggs1		JFET2dCoeffs[9]
#define	ggd1		JFET2dCoeffs[10]
#define	ggs2		JFET2dCoeffs[11]
#define	ggd2		JFET2dCoeffs[12]
#define	ggs3		JFET2dCoeffs[13]
#define	ggd3		JFET2dCoeffs[14]
#define	capgs1		JFET2dCoeffs[15]
#define	capgd1		JFET2dCoeffs[16]
#define	capgs2		JFET2dCoeffs[17]
#define	capgd2		JFET2dCoeffs[18]
#define	capgs3		JFET2dCoeffs[19]
#define	capgd3		JFET2dCoeffs[20]

#endif
#endif

/* indices to an array of JFET2 noise sources */

#define JFET2RDNOIZ       0
#define JFET2RSNOIZ       1
#define JFET2IDNOIZ       2
#define JFET2FLNOIZ       3
#define JFET2TOTNOIZ      4

#define JFET2NSRCS        5

#ifndef NONOISE
    double JFET2nVar[NSTATVARS][JFET2NSRCS];
#else /* NONOISE */
	double **JFET2nVar;
#endif /* NONOISE */

    unsigned JFET2off :1;            /* 'off' flag for jfet */
    unsigned JFET2areaGiven  : 1;    /* flag to indicate area was specified */
    unsigned JFET2icVDSGiven : 1;    /* initial condition given flag for V D-S*/
    unsigned JFET2icVGSGiven : 1;    /* initial condition given flag for V G-S*/
    unsigned JFET2tempGiven  : 1;    /* flag to indicate instance temp given */


    double JFET2area;    /* area factor for the jfet */
    double JFET2icVDS;   /* initial condition voltage D-S*/
    double JFET2icVGS;   /* initial condition voltage G-S*/
    double JFET2temp;    /* operating temperature */
    double JFET2tSatCur; /* temperature adjusted saturation current */
    double JFET2tGatePot;    /* temperature adjusted gate potential */
    double JFET2tCGS;    /* temperature corrected G-S capacitance */
    double JFET2tCGD;    /* temperature corrected G-D capacitance */
    double JFET2corDepCap;   /* joining point of the fwd bias dep. cap eq.s */
    double JFET2vcrit;   /* critical voltage for the instance */
    double JFET2f1;      /* coefficient of capacitance polynomial exp */
    double JFET2xiwoo;       /* velocity saturation potential */
    double JFET2d3;          /* Dual Power-law parameter */
    double JFET2alpha;       /* capacitance model transition parameter */
};

#define JFET2state    GENstate

#define JFET2vgs      JFET2state 
#define JFET2vgd      JFET2state+1 
#define JFET2cg       JFET2state+2 
#define JFET2cd       JFET2state+3 
#define JFET2cgd      JFET2state+4 
#define JFET2gm       JFET2state+5 
#define JFET2gds      JFET2state+6 
#define JFET2ggs      JFET2state+7 
#define JFET2ggd      JFET2state+8 
#define JFET2qgs      JFET2state+9 
#define JFET2cqgs     JFET2state+10 
#define JFET2qgd      JFET2state+11 
#define JFET2cqgd     JFET2state+12 
#define JFET2qds      JFET2state+13
#define JFET2cqds     JFET2state+14
#define JFET2pave     JFET2state+15
#define JFET2vtrap    JFET2state+16
#define JFET2vgstrap  JFET2state+17

#define JFET2_STATE_COUNT    18

struct sJFET2model : sGENmodel
{
    sJFET2model()       { memset(this, 0, sizeof(sJFET2model)); }
    sJFET2model *next() { return (static_cast<sJFET2model*>(GENnextModel)); }
    sJFET2instance *inst()
                        { return (static_cast<sJFET2instance*>(GENinstances)); }

    int JFET2type;

    // SRW -- extracted from jfet2parm.h
    double JFET2acgam;
    double JFET2fNexp;
    double JFET2beta;
    double JFET2capds;
    double JFET2capgd;
    double JFET2capgs;
    double JFET2delta;
    double JFET2hfeta;
    double JFET2hfe1;
    double JFET2hfe2;
    double JFET2hfg1;
    double JFET2hfg2;
    double JFET2mvst;
    double JFET2mxi;
    double JFET2fc;
    double JFET2ibd;
    double JFET2is;
    double JFET2fNcoef;
    double JFET2lambda;
    double JFET2lfgam;
    double JFET2lfg1;
    double JFET2lfg2;
    double JFET2n;
    double JFET2p;
    double JFET2phi;
    double JFET2q;
    double JFET2rd;
    double JFET2rs;
    double JFET2taud;
    double JFET2taug;
    double JFET2vbd;
    double JFET2ver;
    double JFET2vst;
    double JFET2vto;
    double JFET2xc;
    double JFET2xi;
    double JFET2z;
    double JFET2hfgam;

    double JFET2drainConduct;
    double JFET2sourceConduct;
    double JFET2f2;
    double JFET2f3;
    double JFET2za;      /* saturation index parameter */
    double JFET2tnom;    /* temperature at which parameters were measured */

    // SRW -- extracted from jfet2parm.h
    unsigned JFET2acgamGiven :1;
    unsigned JFET2fNexpGiven :1;
    unsigned JFET2betaGiven :1;
    unsigned JFET2capDSGiven :1;
    unsigned JFET2capGDGiven :1;
    unsigned JFET2capGSGiven :1;
    unsigned JFET2deltaGiven :1;
    unsigned JFET2hfetaGiven :1;
    unsigned JFET2hfe1Given :1;
    unsigned JFET2hfe2Given :1;
    unsigned JFET2hfg1Given :1;
    unsigned JFET2hfg2Given :1;
    unsigned JFET2mvstGiven :1;
    unsigned JFET2mxiGiven :1;
    unsigned JFET2fcGiven :1;
    unsigned JFET2ibdGiven :1;
    unsigned JFET2isGiven :1;
    unsigned JFET2kfGiven :1;
    unsigned JFET2lamGiven :1;
    unsigned JFET2lfgamGiven :1;
    unsigned JFET2lfg1Given :1;
    unsigned JFET2lfg2Given :1;
    unsigned JFET2nGiven :1;
    unsigned JFET2pGiven :1;
    unsigned JFET2phiGiven :1;
    unsigned JFET2qGiven :1;
    unsigned JFET2rdGiven :1;
    unsigned JFET2rsGiven :1;
    unsigned JFET2taudGiven :1;
    unsigned JFET2taugGiven :1;
    unsigned JFET2vbdGiven :1;
    unsigned JFET2verGiven :1;
    unsigned JFET2vstGiven :1;
    unsigned JFET2vtoGiven :1;
    unsigned JFET2xcGiven :1;
    unsigned JFET2xiGiven :1;
    unsigned JFET2zGiven :1;
    unsigned JFET2hfgGiven :1;

    unsigned JFET2tnomGiven : 1; /* user specified Tnom for model */

};

} // namespace JFET2
using namespace JFET2;


#ifndef NJF
#define NJF 1
#define PJF -1
#endif

// device parameters
enum {
    JFET2_AREA = 1,
    JFET2_IC_VDS,
    JFET2_IC_VGS,
    JFET2_IC,
    JFET2_OFF,
    JFET2_TEMP,

    JFET2_DRAINNODE,
    JFET2_GATENODE,
    JFET2_SOURCENODE,
    JFET2_DRAINPRIMENODE,
    JFET2_SOURCEPRIMENODE,
    JFET2_VGS,
    JFET2_VGD,
    JFET2_CG,
    JFET2_CD,
    JFET2_CGD,
    JFET2_GM,
    JFET2_GDS,
    JFET2_GGS,
    JFET2_GGD,
    JFET2_QGS,
    JFET2_CQGS,
    JFET2_QGD,
    JFET2_CQGD,
    JFET2_CS,
    JFET2_POWER,
    JFET2_VTRAP,
    JFET2_PAVE
};

// model parameters
enum {
    JFET2_MOD_ACGAM = 1000,
    JFET2_MOD_AF,
    JFET2_MOD_BETA,
    JFET2_MOD_CDS,
    JFET2_MOD_CGD,
    JFET2_MOD_CGS,
    JFET2_MOD_DELTA,
    JFET2_MOD_HFETA,
    JFET2_MOD_HFE1,
    JFET2_MOD_HFE2,
    JFET2_MOD_HFG1,
    JFET2_MOD_HFG2,
    JFET2_MOD_MVST,
    JFET2_MOD_MXI,
    JFET2_MOD_FC,
    JFET2_MOD_IBD,
    JFET2_MOD_IS,
    JFET2_MOD_KF,
    JFET2_MOD_LAMBDA,
    JFET2_MOD_LFGAM,
    JFET2_MOD_LFG1,
    JFET2_MOD_LFG2,
    JFET2_MOD_N,
    JFET2_MOD_P,
    JFET2_MOD_PB,
    JFET2_MOD_Q,
    JFET2_MOD_RD,
    JFET2_MOD_RS,
    JFET2_MOD_TAUD,
    JFET2_MOD_TAUG,
    JFET2_MOD_VBD,
    JFET2_MOD_VER,
    JFET2_MOD_VST,
    JFET2_MOD_VTO,
    JFET2_MOD_XC,
    JFET2_MOD_XI,
    JFET2_MOD_Z,
    JFET2_MOD_HFGAM,

    JFET2_MOD_DRAINCONDUCT,
    JFET2_MOD_SOURCECONDUCT,
    JFET2_MOD_DEPLETIONCAP,
    JFET2_MOD_VCRIT,
    JFET2_MOD_TYPE,

    // SRW -- extracted from jfet2parm.h
    JFET2_MOD_NJF,
    JFET2_MOD_PJF,
    JFET2_MOD_TNOM
};

extern void PSinstanceinit(sJFET2model*, sJFET2instance*);
extern double PSids(sCKT*, sJFET2model*, sJFET2instance*, double, double,
    double*, double*, double*, double*, double*, double*);
extern void PScharge(sCKT*, sJFET2model*, sJFET2instance*, double, double,
    double*, double*);
extern void PSacload(sCKT*, sJFET2model*, sJFET2instance*, double, double,
    double, double, double*, double*, double*, double*);

#endif // JFET2DEFS_H

