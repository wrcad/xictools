
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
Copyright 2004 Regents of the University of California.  All rights reserved.
Author: 1995 Min-Chie Jeng and Mansun Chan.
Author: 1997-1999 Weidong Liu.
Author: 2001- Xuemei Xi
File: bsim3def.h
**********/

#ifndef B3DEFS_H
#define B3DEFS_H

#include "device.h"

#define B3VERSION "3.3.0"

#define NEWCONV

#define FABS            fabs
#define REFTEMP         wrsREFTEMP       
#define CHARGE          wrsCHARGE        
#define CONSTCtoK       wrsCONSTCtoK     
#define CONSTboltz      wrsCONSTboltz    
#define CONSTvt0        wrsCONSTvt0      
#define CONSTKoverQ     wrsCONSTKoverQ   
#define CONSTroot2      wrsCONSTroot2    
#define CONSTe          wrsCONSTe        

struct dvaMatrix;

namespace BSIM330 {

struct sBSIM3model;
struct sBSIM3instance;

struct BSIM3adj
{
    BSIM3adj();
    ~BSIM3adj();

    double *BSIM3DdPtr;
    double *BSIM3GgPtr;
    double *BSIM3SsPtr;
    double *BSIM3BbPtr;
    double *BSIM3DPdpPtr;
    double *BSIM3SPspPtr;
    double *BSIM3DdpPtr;
    double *BSIM3GbPtr;
    double *BSIM3GdpPtr;
    double *BSIM3GspPtr;
    double *BSIM3SspPtr;
    double *BSIM3BdpPtr;
    double *BSIM3BspPtr;
    double *BSIM3DPspPtr;
    double *BSIM3DPdPtr;
    double *BSIM3BgPtr;
    double *BSIM3DPgPtr;
    double *BSIM3SPgPtr;
    double *BSIM3SPsPtr;
    double *BSIM3DPbPtr;
    double *BSIM3SPbPtr;
    double *BSIM3SPdpPtr;

    double *BSIM3QqPtr;
    double *BSIM3QdpPtr;
    double *BSIM3QgPtr;
    double *BSIM3QspPtr;
    double *BSIM3QbPtr;
    double *BSIM3DPqPtr;
    double *BSIM3GqPtr;
    double *BSIM3SPqPtr;
    double *BSIM3BqPtr;

    dvaMatrix *matrix;
};

struct BSIM3dev : public IFdevice
{
    BSIM3dev();
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
//    int disto(int, sGENmodel*, sCKT*);
    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
private:
    int checkModel(sBSIM3model*, sBSIM3instance*, sCKT*);
};

struct sBSIM3instancePOD
{
    int BSIM3dNode;   // number of the drain node of the mosfet
    int BSIM3gNode;   // number of the gate node of the mosfet
    int BSIM3sNode;   // number of the source node of the mosfet
    int BSIM3bNode;   // number of the bulk node of the mosfet

    int BSIM3dNodePrime;
    int BSIM3sNodePrime;
    int BSIM3qNode; /* MCJ */

    BSIM3adj *BSIM3adjoint;

    // This provides a means to back up and restore a known-good
    // state.
    void *BSIM3backing;

    /* MCJ */
    double BSIM3ueff;
    double BSIM3thetavth; 
    double BSIM3von;
    double BSIM3vdsat;
    double BSIM3cgdo;
    double BSIM3cgso;
    double BSIM3vjsm;
    double BSIM3IsEvjsm;
    double BSIM3vjdm;
    double BSIM3IsEvjdm;

    double BSIM3l;
    double BSIM3w;
    double BSIM3drainArea;
    double BSIM3sourceArea;
    double BSIM3drainSquares;
    double BSIM3sourceSquares;
    double BSIM3drainPerimeter;
    double BSIM3sourcePerimeter;
    double BSIM3sourceConductance;
    double BSIM3drainConductance;

    double BSIM3icVBS;
    double BSIM3icVDS;
    double BSIM3icVGS;
    int BSIM3off;
    int BSIM3mode;
    int BSIM3nqsMod;
    int BSIM3acnqsMod;

    /* OP point */
    double BSIM3qinv;
    double BSIM3cd;
    double BSIM3cbs;
    double BSIM3cbd;
    double BSIM3csub;
    double BSIM3gm;
    double BSIM3gds;
    double BSIM3gmbs;
    double BSIM3gbd;
    double BSIM3gbs;

    double BSIM3gbbs;
    double BSIM3gbgs;
    double BSIM3gbds;

    double BSIM3cggb;
    double BSIM3cgdb;
    double BSIM3cgsb;
    double BSIM3cbgb;
    double BSIM3cbdb;
    double BSIM3cbsb;
    double BSIM3cdgb;
    double BSIM3cddb;
    double BSIM3cdsb;
    double BSIM3capbd;
    double BSIM3capbs;

    double BSIM3cqgb;
    double BSIM3cqdb;
    double BSIM3cqsb;
    double BSIM3cqbb;

    double BSIM3qgate;
    double BSIM3qbulk;
    double BSIM3qdrn;

    double BSIM3gtau;
    double BSIM3gtg;
    double BSIM3gtd;
    double BSIM3gts;
    double BSIM3gtb;
    double BSIM3rds;  /* Noise bugfix */
    double BSIM3Vgsteff;
    double BSIM3Vdseff;
    double BSIM3Abulk;
    double BSIM3AbovVgst2Vtm;
    double BSIM3taunet;

    struct bsim3SizeDependParam  *pParam;

    unsigned BSIM3lGiven :1;
    unsigned BSIM3wGiven :1;
    unsigned BSIM3drainAreaGiven :1;
    unsigned BSIM3sourceAreaGiven    :1;
    unsigned BSIM3drainSquaresGiven  :1;
    unsigned BSIM3sourceSquaresGiven :1;
    unsigned BSIM3drainPerimeterGiven    :1;
    unsigned BSIM3sourcePerimeterGiven   :1;
    unsigned BSIM3dNodePrimeSet  :1;
    unsigned BSIM3sNodePrimeSet  :1;
    unsigned BSIM3icVBSGiven :1;
    unsigned BSIM3icVDSGiven :1;
    unsigned BSIM3icVGSGiven :1;
    unsigned BSIM3nqsModGiven :1;
    unsigned BSIM3acnqsModGiven :1;

    double *BSIM3DdPtr;
    double *BSIM3GgPtr;
    double *BSIM3SsPtr;
    double *BSIM3BbPtr;
    double *BSIM3DPdpPtr;
    double *BSIM3SPspPtr;
    double *BSIM3DdpPtr;
    double *BSIM3GbPtr;
    double *BSIM3GdpPtr;
    double *BSIM3GspPtr;
    double *BSIM3SspPtr;
    double *BSIM3BdpPtr;
    double *BSIM3BspPtr;
    double *BSIM3DPspPtr;
    double *BSIM3DPdPtr;
    double *BSIM3BgPtr;
    double *BSIM3DPgPtr;
    double *BSIM3SPgPtr;
    double *BSIM3SPsPtr;
    double *BSIM3DPbPtr;
    double *BSIM3SPbPtr;
    double *BSIM3SPdpPtr;

    double *BSIM3QqPtr;
    double *BSIM3QdpPtr;
    double *BSIM3QgPtr;
    double *BSIM3QspPtr;
    double *BSIM3QbPtr;
    double *BSIM3DPqPtr;
    double *BSIM3GqPtr;
    double *BSIM3SPqPtr;
    double *BSIM3BqPtr;

/* indices to the array of BSIM3 NOISE SOURCES */

#define BSIM3RDNOIZ       0
#define BSIM3RSNOIZ       1
#define BSIM3IDNOIZ       2
#define BSIM3FLNOIZ       3
#define BSIM3TOTNOIZ      4

#define BSIM3NSRCS        5  /* the number of BSIM3 MOSFET noise sources */

#ifndef NONOISE
    double BSIM3nVar[NSTATVARS][BSIM3NSRCS];
#else /* NONOISE */
        double **BSIM3nVar;
#endif /* NONOISE */
};

#define BSIM3vbd GENstate + 0
#define BSIM3vbs GENstate + 1
#define BSIM3vgs GENstate + 2
#define BSIM3vds GENstate + 3

#define BSIM3qb GENstate + 4
#define BSIM3cqb GENstate + 5
#define BSIM3qg GENstate + 6
#define BSIM3cqg GENstate + 7
#define BSIM3qd GENstate + 8
#define BSIM3cqd GENstate + 9

#define BSIM3qbs  GENstate + 10
#define BSIM3qbd  GENstate + 11

#define BSIM3qcheq GENstate + 12
#define BSIM3cqcheq GENstate + 13
#define BSIM3qcdump GENstate + 14
#define BSIM3cqcdump GENstate + 15

#define BSIM3qdef GENstate + 16

// SRW - added for interpolation
#define BSIM3a_cd      GENstate + 17
#define BSIM3a_cbs     GENstate + 18
#define BSIM3a_cbd     GENstate + 19
#define BSIM3a_gm      GENstate + 20
#define BSIM3a_gds     GENstate + 21
#define BSIM3a_gmbs    GENstate + 22
#define BSIM3a_gbd     GENstate + 23
#define BSIM3a_gbs     GENstate + 24
#define BSIM3a_cggb    GENstate + 25
#define BSIM3a_cgdb    GENstate + 26
#define BSIM3a_cgsb    GENstate + 27
#define BSIM3a_cdgb    GENstate + 28
#define BSIM3a_cddb    GENstate + 29
#define BSIM3a_cdsb    GENstate + 30
#define BSIM3a_cbgb    GENstate + 31
#define BSIM3a_cbdb    GENstate + 32
#define BSIM3a_cbsb    GENstate + 33
#define BSIM3a_capbd   GENstate + 34
#define BSIM3a_capbs   GENstate + 35
#define BSIM3a_von     GENstate + 36
#define BSIM3a_vdsat   GENstate + 37

#define BSIM3a_id      GENstate + 38
#define BSIM3a_is      GENstate + 39
#define BSIM3a_ig      GENstate + 40
#define BSIM3a_ib      GENstate + 41

#define BSIM3numStates 42

struct sBSIM3instance : sGENinstance, sBSIM3instancePOD
{
    sBSIM3instance() : sGENinstance(), sBSIM3instancePOD()
        { GENnumNodes = 4; }
    ~sBSIM3instance()
        {
            delete BSIM3adjoint;
            delete [] (char*)BSIM3backing;
        }

    sBSIM3instance *next()
        { return (static_cast<sBSIM3instance*>(GENnextInstance)); }

    void backup(DEV_BKMODE m)
        {
            if (m == DEV_SAVE) {
                if (!BSIM3backing)
                    BSIM3backing = new char[sizeof(sBSIM3instance)];
                memcpy(BSIM3backing, this, sizeof(sBSIM3instance));
            }
            else if (m == DEV_RESTORE) {
                if (BSIM3backing)
                    memcpy(this, BSIM3backing, sizeof(sBSIM3instance));
            }
            else {
                // DEV_CLEAR
                delete [] (char*)BSIM3backing;
                BSIM3backing = 0;
            }
        }
};

struct bsim3SizeDependParam
{
    double Width;
    double Length;

    double BSIM3cdsc;           
    double BSIM3cdscb;    
    double BSIM3cdscd;       
    double BSIM3cit;           
    double BSIM3nfactor;      
    double BSIM3xj;
    double BSIM3vsat;         
    double BSIM3at;         
    double BSIM3a0;   
    double BSIM3ags;      
    double BSIM3a1;         
    double BSIM3a2;         
    double BSIM3keta;     
    double BSIM3nsub;
    double BSIM3npeak;        
    double BSIM3ngate;        
    double BSIM3gamma1;      
    double BSIM3gamma2;     
    double BSIM3vbx;      
    double BSIM3vbi;       
    double BSIM3vbm;       
    double BSIM3vbsc;       
    double BSIM3xt;       
    double BSIM3phi;
    double BSIM3litl;
    double BSIM3k1;
    double BSIM3kt1;
    double BSIM3kt1l;
    double BSIM3kt2;
    double BSIM3k2;
    double BSIM3k3;
    double BSIM3k3b;
    double BSIM3w0;
    double BSIM3nlx;
    double BSIM3dvt0;      
    double BSIM3dvt1;      
    double BSIM3dvt2;      
    double BSIM3dvt0w;      
    double BSIM3dvt1w;      
    double BSIM3dvt2w;      
    double BSIM3drout;      
    double BSIM3dsub;      
    double BSIM3vth0;
    double BSIM3ua;
    double BSIM3ua1;
    double BSIM3ub;
    double BSIM3ub1;
    double BSIM3uc;
    double BSIM3uc1;
    double BSIM3u0;
    double BSIM3ute;
    double BSIM3voff;
    double BSIM3vfb;
    double BSIM3delta;
    double BSIM3rdsw;       
    double BSIM3rds0;       
    double BSIM3prwg;       
    double BSIM3prwb;       
    double BSIM3prt;       
    double BSIM3eta0;         
    double BSIM3etab;         
    double BSIM3pclm;      
    double BSIM3pdibl1;      
    double BSIM3pdibl2;      
    double BSIM3pdiblb;      
    double BSIM3pscbe1;       
    double BSIM3pscbe2;       
    double BSIM3pvag;       
    double BSIM3wr;
    double BSIM3dwg;
    double BSIM3dwb;
    double BSIM3b0;
    double BSIM3b1;
    double BSIM3alpha0;
    double BSIM3alpha1;
    double BSIM3beta0;

    /* CV model */
    double BSIM3elm;
    double BSIM3cgsl;
    double BSIM3cgdl;
    double BSIM3ckappa;
    double BSIM3cf;
    double BSIM3clc;
    double BSIM3cle;
    double BSIM3vfbcv;
    double BSIM3noff;
    double BSIM3voffcv;
    double BSIM3acde;
    double BSIM3moin;

/* Pre-calculated constants */

    double BSIM3dw;
    double BSIM3dl;
    double BSIM3leff;
    double BSIM3weff;

    double BSIM3dwc;
    double BSIM3dlc;
    double BSIM3leffCV;
    double BSIM3weffCV;
    double BSIM3abulkCVfactor;
    double BSIM3cgso;
    double BSIM3cgdo;
    double BSIM3cgbo;
    double BSIM3tconst;

    double BSIM3u0temp;       
    double BSIM3vsattemp;   
    double BSIM3sqrtPhi;   
    double BSIM3phis3;   
    double BSIM3Xdep0;          
    double BSIM3sqrtXdep0;          
    double BSIM3theta0vb0;
    double BSIM3thetaRout; 

    double BSIM3cof1;
    double BSIM3cof2;
    double BSIM3cof3;
    double BSIM3cof4;
    double BSIM3cdep0;
    double BSIM3vfbzb;
    double BSIM3ldeb;
    double BSIM3k1ox;
    double BSIM3k2ox;

    struct bsim3SizeDependParam  *pNext;
};

struct sBSIM3modelPOD
{
    int BSIM3type;

    int    BSIM3mobMod;
    int    BSIM3capMod;
    int    BSIM3noiMod;
    int    BSIM3acnqsMod;
    int    BSIM3binUnit;
    int    BSIM3paramChk;
// SRW
    const char   *BSIM3version;             

    double BSIM3tox;             
    double BSIM3toxm;
    double BSIM3cdsc;           
    double BSIM3cdscb; 
    double BSIM3cdscd;          
    double BSIM3cit;           
    double BSIM3nfactor;      
    double BSIM3xj;
    double BSIM3vsat;         
    double BSIM3at;         
    double BSIM3a0;   
    double BSIM3ags;      
    double BSIM3a1;         
    double BSIM3a2;         
    double BSIM3keta;     
    double BSIM3nsub;
    double BSIM3npeak;        
    double BSIM3ngate;        
    double BSIM3gamma1;      
    double BSIM3gamma2;     
    double BSIM3vbx;      
    double BSIM3vbm;       
    double BSIM3xt;       
    double BSIM3k1;
    double BSIM3kt1;
    double BSIM3kt1l;
    double BSIM3kt2;
    double BSIM3k2;
    double BSIM3k3;
    double BSIM3k3b;
    double BSIM3w0;
    double BSIM3nlx;
    double BSIM3dvt0;      
    double BSIM3dvt1;      
    double BSIM3dvt2;      
    double BSIM3dvt0w;      
    double BSIM3dvt1w;      
    double BSIM3dvt2w;      
    double BSIM3drout;      
    double BSIM3dsub;      
    double BSIM3vth0;
    double BSIM3ua;
    double BSIM3ua1;
    double BSIM3ub;
    double BSIM3ub1;
    double BSIM3uc;
    double BSIM3uc1;
    double BSIM3u0;
    double BSIM3ute;
    double BSIM3voff;
    double BSIM3delta;
    double BSIM3rdsw;       
    double BSIM3prwg;
    double BSIM3prwb;
    double BSIM3prt;       
    double BSIM3eta0;         
    double BSIM3etab;         
    double BSIM3pclm;      
    double BSIM3pdibl1;      
    double BSIM3pdibl2;      
    double BSIM3pdiblb;
    double BSIM3pscbe1;       
    double BSIM3pscbe2;       
    double BSIM3pvag;       
    double BSIM3wr;
    double BSIM3dwg;
    double BSIM3dwb;
    double BSIM3b0;
    double BSIM3b1;
    double BSIM3alpha0;
    double BSIM3alpha1;
    double BSIM3beta0;
    double BSIM3ijth;
    double BSIM3vfb;

// SRW
    double BSIM3xw;
    double BSIM3xl;

    /* CV model */
    double BSIM3elm;
    double BSIM3cgsl;
    double BSIM3cgdl;
    double BSIM3ckappa;
    double BSIM3cf;
    double BSIM3vfbcv;
    double BSIM3clc;
    double BSIM3cle;
    double BSIM3dwc;
    double BSIM3dlc;
    double BSIM3noff;
    double BSIM3voffcv;
    double BSIM3acde;
    double BSIM3moin;
    double BSIM3tcj;
    double BSIM3tcjsw;
    double BSIM3tcjswg;
    double BSIM3tpb;
    double BSIM3tpbsw;
    double BSIM3tpbswg;

    /* Length Dependence */
    double BSIM3lcdsc;           
    double BSIM3lcdscb; 
    double BSIM3lcdscd;          
    double BSIM3lcit;           
    double BSIM3lnfactor;      
    double BSIM3lxj;
    double BSIM3lvsat;         
    double BSIM3lat;         
    double BSIM3la0;   
    double BSIM3lags;      
    double BSIM3la1;         
    double BSIM3la2;         
    double BSIM3lketa;     
    double BSIM3lnsub;
    double BSIM3lnpeak;        
    double BSIM3lngate;        
    double BSIM3lgamma1;      
    double BSIM3lgamma2;     
    double BSIM3lvbx;      
    double BSIM3lvbm;       
    double BSIM3lxt;       
    double BSIM3lk1;
    double BSIM3lkt1;
    double BSIM3lkt1l;
    double BSIM3lkt2;
    double BSIM3lk2;
    double BSIM3lk3;
    double BSIM3lk3b;
    double BSIM3lw0;
    double BSIM3lnlx;
    double BSIM3ldvt0;      
    double BSIM3ldvt1;      
    double BSIM3ldvt2;      
    double BSIM3ldvt0w;      
    double BSIM3ldvt1w;      
    double BSIM3ldvt2w;      
    double BSIM3ldrout;      
    double BSIM3ldsub;      
    double BSIM3lvth0;
    double BSIM3lua;
    double BSIM3lua1;
    double BSIM3lub;
    double BSIM3lub1;
    double BSIM3luc;
    double BSIM3luc1;
    double BSIM3lu0;
    double BSIM3lute;
    double BSIM3lvoff;
    double BSIM3ldelta;
    double BSIM3lrdsw;       
    double BSIM3lprwg;
    double BSIM3lprwb;
    double BSIM3lprt;       
    double BSIM3leta0;         
    double BSIM3letab;         
    double BSIM3lpclm;      
    double BSIM3lpdibl1;      
    double BSIM3lpdibl2;      
    double BSIM3lpdiblb;
    double BSIM3lpscbe1;       
    double BSIM3lpscbe2;       
    double BSIM3lpvag;       
    double BSIM3lwr;
    double BSIM3ldwg;
    double BSIM3ldwb;
    double BSIM3lb0;
    double BSIM3lb1;
    double BSIM3lalpha0;
    double BSIM3lalpha1;
    double BSIM3lbeta0;
    double BSIM3lvfb;

    /* CV model */
    double BSIM3lelm;
    double BSIM3lcgsl;
    double BSIM3lcgdl;
    double BSIM3lckappa;
    double BSIM3lcf;
    double BSIM3lclc;
    double BSIM3lcle;
    double BSIM3lvfbcv;
    double BSIM3lnoff;
    double BSIM3lvoffcv;
    double BSIM3lacde;
    double BSIM3lmoin;

    /* Width Dependence */
    double BSIM3wcdsc;           
    double BSIM3wcdscb; 
    double BSIM3wcdscd;          
    double BSIM3wcit;           
    double BSIM3wnfactor;      
    double BSIM3wxj;
    double BSIM3wvsat;         
    double BSIM3wat;         
    double BSIM3wa0;   
    double BSIM3wags;      
    double BSIM3wa1;         
    double BSIM3wa2;         
    double BSIM3wketa;     
    double BSIM3wnsub;
    double BSIM3wnpeak;        
    double BSIM3wngate;        
    double BSIM3wgamma1;      
    double BSIM3wgamma2;     
    double BSIM3wvbx;      
    double BSIM3wvbm;       
    double BSIM3wxt;       
    double BSIM3wk1;
    double BSIM3wkt1;
    double BSIM3wkt1l;
    double BSIM3wkt2;
    double BSIM3wk2;
    double BSIM3wk3;
    double BSIM3wk3b;
    double BSIM3ww0;
    double BSIM3wnlx;
    double BSIM3wdvt0;      
    double BSIM3wdvt1;      
    double BSIM3wdvt2;      
    double BSIM3wdvt0w;      
    double BSIM3wdvt1w;      
    double BSIM3wdvt2w;      
    double BSIM3wdrout;      
    double BSIM3wdsub;      
    double BSIM3wvth0;
    double BSIM3wua;
    double BSIM3wua1;
    double BSIM3wub;
    double BSIM3wub1;
    double BSIM3wuc;
    double BSIM3wuc1;
    double BSIM3wu0;
    double BSIM3wute;
    double BSIM3wvoff;
    double BSIM3wdelta;
    double BSIM3wrdsw;       
    double BSIM3wprwg;
    double BSIM3wprwb;
    double BSIM3wprt;       
    double BSIM3weta0;         
    double BSIM3wetab;         
    double BSIM3wpclm;      
    double BSIM3wpdibl1;      
    double BSIM3wpdibl2;      
    double BSIM3wpdiblb;
    double BSIM3wpscbe1;       
    double BSIM3wpscbe2;       
    double BSIM3wpvag;       
    double BSIM3wwr;
    double BSIM3wdwg;
    double BSIM3wdwb;
    double BSIM3wb0;
    double BSIM3wb1;
    double BSIM3walpha0;
    double BSIM3walpha1;
    double BSIM3wbeta0;
    double BSIM3wvfb;

    /* CV model */
    double BSIM3welm;
    double BSIM3wcgsl;
    double BSIM3wcgdl;
    double BSIM3wckappa;
    double BSIM3wcf;
    double BSIM3wclc;
    double BSIM3wcle;
    double BSIM3wvfbcv;
    double BSIM3wnoff;
    double BSIM3wvoffcv;
    double BSIM3wacde;
    double BSIM3wmoin;

    /* Cross-term Dependence */
    double BSIM3pcdsc;           
    double BSIM3pcdscb; 
    double BSIM3pcdscd;          
    double BSIM3pcit;           
    double BSIM3pnfactor;      
    double BSIM3pxj;
    double BSIM3pvsat;         
    double BSIM3pat;         
    double BSIM3pa0;   
    double BSIM3pags;      
    double BSIM3pa1;         
    double BSIM3pa2;         
    double BSIM3pketa;     
    double BSIM3pnsub;
    double BSIM3pnpeak;        
    double BSIM3pngate;        
    double BSIM3pgamma1;      
    double BSIM3pgamma2;     
    double BSIM3pvbx;      
    double BSIM3pvbm;       
    double BSIM3pxt;       
    double BSIM3pk1;
    double BSIM3pkt1;
    double BSIM3pkt1l;
    double BSIM3pkt2;
    double BSIM3pk2;
    double BSIM3pk3;
    double BSIM3pk3b;
    double BSIM3pw0;
    double BSIM3pnlx;
    double BSIM3pdvt0;      
    double BSIM3pdvt1;      
    double BSIM3pdvt2;      
    double BSIM3pdvt0w;      
    double BSIM3pdvt1w;      
    double BSIM3pdvt2w;      
    double BSIM3pdrout;      
    double BSIM3pdsub;      
    double BSIM3pvth0;
    double BSIM3pua;
    double BSIM3pua1;
    double BSIM3pub;
    double BSIM3pub1;
    double BSIM3puc;
    double BSIM3puc1;
    double BSIM3pu0;
    double BSIM3pute;
    double BSIM3pvoff;
    double BSIM3pdelta;
    double BSIM3prdsw;
    double BSIM3pprwg;
    double BSIM3pprwb;
    double BSIM3pprt;       
    double BSIM3peta0;         
    double BSIM3petab;         
    double BSIM3ppclm;      
    double BSIM3ppdibl1;      
    double BSIM3ppdibl2;      
    double BSIM3ppdiblb;
    double BSIM3ppscbe1;       
    double BSIM3ppscbe2;       
    double BSIM3ppvag;       
    double BSIM3pwr;
    double BSIM3pdwg;
    double BSIM3pdwb;
    double BSIM3pb0;
    double BSIM3pb1;
    double BSIM3palpha0;
    double BSIM3palpha1;
    double BSIM3pbeta0;
    double BSIM3pvfb;

    /* CV model */
    double BSIM3pelm;
    double BSIM3pcgsl;
    double BSIM3pcgdl;
    double BSIM3pckappa;
    double BSIM3pcf;
    double BSIM3pclc;
    double BSIM3pcle;
    double BSIM3pvfbcv;
    double BSIM3pnoff;
    double BSIM3pvoffcv;
    double BSIM3pacde;
    double BSIM3pmoin;

    double BSIM3tnom;
    double BSIM3cgso;
    double BSIM3cgdo;
    double BSIM3cgbo;
    double BSIM3xpart;
    double BSIM3cFringOut;
    double BSIM3cFringMax;

    double BSIM3sheetResistance;
    double BSIM3jctSatCurDensity;
    double BSIM3jctSidewallSatCurDensity;
    double BSIM3bulkJctPotential;
    double BSIM3bulkJctBotGradingCoeff;
    double BSIM3bulkJctSideGradingCoeff;
    double BSIM3bulkJctGateSideGradingCoeff;
    double BSIM3sidewallJctPotential;
    double BSIM3GatesidewallJctPotential;
    double BSIM3unitAreaJctCap;
    double BSIM3unitLengthSidewallJctCap;
    double BSIM3unitLengthGateSidewallJctCap;
    double BSIM3jctEmissionCoeff;
    double BSIM3jctTempExponent;

    double BSIM3Lint;
    double BSIM3Ll;
    double BSIM3Llc;
    double BSIM3Lln;
    double BSIM3Lw;
    double BSIM3Lwc;
    double BSIM3Lwn;
    double BSIM3Lwl;
    double BSIM3Lwlc;
    double BSIM3Lmin;
    double BSIM3Lmax;

    double BSIM3Wint;
    double BSIM3Wl;
    double BSIM3Wlc;
    double BSIM3Wln;
    double BSIM3Ww;
    double BSIM3Wwc;
    double BSIM3Wwn;
    double BSIM3Wwl;
    double BSIM3Wwlc;
    double BSIM3Wmin;
    double BSIM3Wmax;

/* Pre-calculated constants */
    /* MCJ: move to size-dependent param. */
    double BSIM3vtm;   
    double BSIM3cox;
    double BSIM3cof1;
    double BSIM3cof2;
    double BSIM3cof3;
    double BSIM3cof4;
    double BSIM3vcrit;
    double BSIM3factor1;
    double BSIM3PhiB;
    double BSIM3PhiBSW;
    double BSIM3PhiBSWG;
    double BSIM3jctTempSatCurDensity;
    double BSIM3jctSidewallTempSatCurDensity;
    double BSIM3unitAreaTempJctCap;
    double BSIM3unitLengthSidewallTempJctCap;
    double BSIM3unitLengthGateSidewallTempJctCap;

    double BSIM3oxideTrapDensityA;      
    double BSIM3oxideTrapDensityB;     
    double BSIM3oxideTrapDensityC;  
    double BSIM3em;  
    double BSIM3ef;  
    double BSIM3af;  
    double BSIM3kf;  
    double BSIM3lintnoi;  /* lint offset for noise calculation  */

    struct bsim3SizeDependParam *pSizeDependParamKnot;

// SRW
    int BSIM3nqsMod;

    /* Flags */
    unsigned  BSIM3mobModGiven :1;
    unsigned  BSIM3binUnitGiven :1;
    unsigned  BSIM3capModGiven :1;
    unsigned  BSIM3paramChkGiven :1;
    unsigned  BSIM3noiModGiven :1;
    unsigned  BSIM3acnqsModGiven :1;
    unsigned  BSIM3typeGiven   :1;
    unsigned  BSIM3toxGiven   :1;
    unsigned  BSIM3versionGiven   :1;
    unsigned  BSIM3toxmGiven   :1;
    unsigned  BSIM3cdscGiven   :1;
    unsigned  BSIM3cdscbGiven   :1;
    unsigned  BSIM3cdscdGiven   :1;
    unsigned  BSIM3citGiven   :1;
    unsigned  BSIM3nfactorGiven   :1;
    unsigned  BSIM3xjGiven   :1;
    unsigned  BSIM3vsatGiven   :1;
    unsigned  BSIM3atGiven   :1;
    unsigned  BSIM3a0Given   :1;
    unsigned  BSIM3agsGiven   :1;
    unsigned  BSIM3a1Given   :1;
    unsigned  BSIM3a2Given   :1;
    unsigned  BSIM3ketaGiven   :1;    
    unsigned  BSIM3nsubGiven   :1;
    unsigned  BSIM3npeakGiven   :1;
    unsigned  BSIM3ngateGiven   :1;
    unsigned  BSIM3gamma1Given   :1;
    unsigned  BSIM3gamma2Given   :1;
    unsigned  BSIM3vbxGiven   :1;
    unsigned  BSIM3vbmGiven   :1;
    unsigned  BSIM3xtGiven   :1;
    unsigned  BSIM3k1Given   :1;
    unsigned  BSIM3kt1Given   :1;
    unsigned  BSIM3kt1lGiven   :1;
    unsigned  BSIM3kt2Given   :1;
    unsigned  BSIM3k2Given   :1;
    unsigned  BSIM3k3Given   :1;
    unsigned  BSIM3k3bGiven   :1;
    unsigned  BSIM3w0Given   :1;
    unsigned  BSIM3nlxGiven   :1;
    unsigned  BSIM3dvt0Given   :1;   
    unsigned  BSIM3dvt1Given   :1;     
    unsigned  BSIM3dvt2Given   :1;     
    unsigned  BSIM3dvt0wGiven   :1;   
    unsigned  BSIM3dvt1wGiven   :1;     
    unsigned  BSIM3dvt2wGiven   :1;     
    unsigned  BSIM3droutGiven   :1;     
    unsigned  BSIM3dsubGiven   :1;     
    unsigned  BSIM3vth0Given   :1;
    unsigned  BSIM3uaGiven   :1;
    unsigned  BSIM3ua1Given   :1;
    unsigned  BSIM3ubGiven   :1;
    unsigned  BSIM3ub1Given   :1;
    unsigned  BSIM3ucGiven   :1;
    unsigned  BSIM3uc1Given   :1;
    unsigned  BSIM3u0Given   :1;
    unsigned  BSIM3uteGiven   :1;
    unsigned  BSIM3voffGiven   :1;
    unsigned  BSIM3rdswGiven   :1;      
    unsigned  BSIM3prwgGiven   :1;      
    unsigned  BSIM3prwbGiven   :1;      
    unsigned  BSIM3prtGiven   :1;      
    unsigned  BSIM3eta0Given   :1;    
    unsigned  BSIM3etabGiven   :1;    
    unsigned  BSIM3pclmGiven   :1;   
    unsigned  BSIM3pdibl1Given   :1;   
    unsigned  BSIM3pdibl2Given   :1;  
    unsigned  BSIM3pdiblbGiven   :1;  
    unsigned  BSIM3pscbe1Given   :1;    
    unsigned  BSIM3pscbe2Given   :1;    
    unsigned  BSIM3pvagGiven   :1;    
    unsigned  BSIM3deltaGiven  :1;     
    unsigned  BSIM3wrGiven   :1;
    unsigned  BSIM3dwgGiven   :1;
    unsigned  BSIM3dwbGiven   :1;
    unsigned  BSIM3b0Given   :1;
    unsigned  BSIM3b1Given   :1;
    unsigned  BSIM3alpha0Given   :1;
    unsigned  BSIM3alpha1Given   :1;
    unsigned  BSIM3beta0Given   :1;
    unsigned  BSIM3ijthGiven   :1;
    unsigned  BSIM3vfbGiven   :1;

    /* CV model */
    unsigned  BSIM3elmGiven  :1;     
    unsigned  BSIM3cgslGiven   :1;
    unsigned  BSIM3cgdlGiven   :1;
    unsigned  BSIM3ckappaGiven   :1;
    unsigned  BSIM3cfGiven   :1;
    unsigned  BSIM3vfbcvGiven   :1;
    unsigned  BSIM3clcGiven   :1;
    unsigned  BSIM3cleGiven   :1;
    unsigned  BSIM3dwcGiven   :1;
    unsigned  BSIM3dlcGiven   :1;
    unsigned  BSIM3noffGiven  :1;
    unsigned  BSIM3voffcvGiven :1;
    unsigned  BSIM3acdeGiven  :1;
    unsigned  BSIM3moinGiven  :1;
    unsigned  BSIM3tcjGiven   :1;
    unsigned  BSIM3tcjswGiven :1;
    unsigned  BSIM3tcjswgGiven :1;
    unsigned  BSIM3tpbGiven    :1;
    unsigned  BSIM3tpbswGiven  :1;
    unsigned  BSIM3tpbswgGiven :1;

    /* Length dependence */
    unsigned  BSIM3lcdscGiven   :1;
    unsigned  BSIM3lcdscbGiven   :1;
    unsigned  BSIM3lcdscdGiven   :1;
    unsigned  BSIM3lcitGiven   :1;
    unsigned  BSIM3lnfactorGiven   :1;
    unsigned  BSIM3lxjGiven   :1;
    unsigned  BSIM3lvsatGiven   :1;
    unsigned  BSIM3latGiven   :1;
    unsigned  BSIM3la0Given   :1;
    unsigned  BSIM3lagsGiven   :1;
    unsigned  BSIM3la1Given   :1;
    unsigned  BSIM3la2Given   :1;
    unsigned  BSIM3lketaGiven   :1;    
    unsigned  BSIM3lnsubGiven   :1;
    unsigned  BSIM3lnpeakGiven   :1;
    unsigned  BSIM3lngateGiven   :1;
    unsigned  BSIM3lgamma1Given   :1;
    unsigned  BSIM3lgamma2Given   :1;
    unsigned  BSIM3lvbxGiven   :1;
    unsigned  BSIM3lvbmGiven   :1;
    unsigned  BSIM3lxtGiven   :1;
    unsigned  BSIM3lk1Given   :1;
    unsigned  BSIM3lkt1Given   :1;
    unsigned  BSIM3lkt1lGiven   :1;
    unsigned  BSIM3lkt2Given   :1;
    unsigned  BSIM3lk2Given   :1;
    unsigned  BSIM3lk3Given   :1;
    unsigned  BSIM3lk3bGiven   :1;
    unsigned  BSIM3lw0Given   :1;
    unsigned  BSIM3lnlxGiven   :1;
    unsigned  BSIM3ldvt0Given   :1;   
    unsigned  BSIM3ldvt1Given   :1;     
    unsigned  BSIM3ldvt2Given   :1;     
    unsigned  BSIM3ldvt0wGiven   :1;   
    unsigned  BSIM3ldvt1wGiven   :1;     
    unsigned  BSIM3ldvt2wGiven   :1;     
    unsigned  BSIM3ldroutGiven   :1;     
    unsigned  BSIM3ldsubGiven   :1;     
    unsigned  BSIM3lvth0Given   :1;
    unsigned  BSIM3luaGiven   :1;
    unsigned  BSIM3lua1Given   :1;
    unsigned  BSIM3lubGiven   :1;
    unsigned  BSIM3lub1Given   :1;
    unsigned  BSIM3lucGiven   :1;
    unsigned  BSIM3luc1Given   :1;
    unsigned  BSIM3lu0Given   :1;
    unsigned  BSIM3luteGiven   :1;
    unsigned  BSIM3lvoffGiven   :1;
    unsigned  BSIM3lrdswGiven   :1;      
    unsigned  BSIM3lprwgGiven   :1;      
    unsigned  BSIM3lprwbGiven   :1;      
    unsigned  BSIM3lprtGiven   :1;      
    unsigned  BSIM3leta0Given   :1;    
    unsigned  BSIM3letabGiven   :1;    
    unsigned  BSIM3lpclmGiven   :1;   
    unsigned  BSIM3lpdibl1Given   :1;   
    unsigned  BSIM3lpdibl2Given   :1;  
    unsigned  BSIM3lpdiblbGiven   :1;  
    unsigned  BSIM3lpscbe1Given   :1;    
    unsigned  BSIM3lpscbe2Given   :1;    
    unsigned  BSIM3lpvagGiven   :1;    
    unsigned  BSIM3ldeltaGiven  :1;     
    unsigned  BSIM3lwrGiven   :1;
    unsigned  BSIM3ldwgGiven   :1;
    unsigned  BSIM3ldwbGiven   :1;
    unsigned  BSIM3lb0Given   :1;
    unsigned  BSIM3lb1Given   :1;
    unsigned  BSIM3lalpha0Given   :1;
    unsigned  BSIM3lalpha1Given   :1;
    unsigned  BSIM3lbeta0Given   :1;
    unsigned  BSIM3lvfbGiven   :1;

    /* CV model */
    unsigned  BSIM3lelmGiven  :1;     
    unsigned  BSIM3lcgslGiven   :1;
    unsigned  BSIM3lcgdlGiven   :1;
    unsigned  BSIM3lckappaGiven   :1;
    unsigned  BSIM3lcfGiven   :1;
    unsigned  BSIM3lclcGiven   :1;
    unsigned  BSIM3lcleGiven   :1;
    unsigned  BSIM3lvfbcvGiven   :1;
    unsigned  BSIM3lnoffGiven   :1;
    unsigned  BSIM3lvoffcvGiven :1;
    unsigned  BSIM3lacdeGiven   :1;
    unsigned  BSIM3lmoinGiven   :1;

    /* Width dependence */
    unsigned  BSIM3wcdscGiven   :1;
    unsigned  BSIM3wcdscbGiven   :1;
    unsigned  BSIM3wcdscdGiven   :1;
    unsigned  BSIM3wcitGiven   :1;
    unsigned  BSIM3wnfactorGiven   :1;
    unsigned  BSIM3wxjGiven   :1;
    unsigned  BSIM3wvsatGiven   :1;
    unsigned  BSIM3watGiven   :1;
    unsigned  BSIM3wa0Given   :1;
    unsigned  BSIM3wagsGiven   :1;
    unsigned  BSIM3wa1Given   :1;
    unsigned  BSIM3wa2Given   :1;
    unsigned  BSIM3wketaGiven   :1;    
    unsigned  BSIM3wnsubGiven   :1;
    unsigned  BSIM3wnpeakGiven   :1;
    unsigned  BSIM3wngateGiven   :1;
    unsigned  BSIM3wgamma1Given   :1;
    unsigned  BSIM3wgamma2Given   :1;
    unsigned  BSIM3wvbxGiven   :1;
    unsigned  BSIM3wvbmGiven   :1;
    unsigned  BSIM3wxtGiven   :1;
    unsigned  BSIM3wk1Given   :1;
    unsigned  BSIM3wkt1Given   :1;
    unsigned  BSIM3wkt1lGiven   :1;
    unsigned  BSIM3wkt2Given   :1;
    unsigned  BSIM3wk2Given   :1;
    unsigned  BSIM3wk3Given   :1;
    unsigned  BSIM3wk3bGiven   :1;
    unsigned  BSIM3ww0Given   :1;
    unsigned  BSIM3wnlxGiven   :1;
    unsigned  BSIM3wdvt0Given   :1;   
    unsigned  BSIM3wdvt1Given   :1;     
    unsigned  BSIM3wdvt2Given   :1;     
    unsigned  BSIM3wdvt0wGiven   :1;   
    unsigned  BSIM3wdvt1wGiven   :1;     
    unsigned  BSIM3wdvt2wGiven   :1;     
    unsigned  BSIM3wdroutGiven   :1;     
    unsigned  BSIM3wdsubGiven   :1;     
    unsigned  BSIM3wvth0Given   :1;
    unsigned  BSIM3wuaGiven   :1;
    unsigned  BSIM3wua1Given   :1;
    unsigned  BSIM3wubGiven   :1;
    unsigned  BSIM3wub1Given   :1;
    unsigned  BSIM3wucGiven   :1;
    unsigned  BSIM3wuc1Given   :1;
    unsigned  BSIM3wu0Given   :1;
    unsigned  BSIM3wuteGiven   :1;
    unsigned  BSIM3wvoffGiven   :1;
    unsigned  BSIM3wrdswGiven   :1;      
    unsigned  BSIM3wprwgGiven   :1;      
    unsigned  BSIM3wprwbGiven   :1;      
    unsigned  BSIM3wprtGiven   :1;      
    unsigned  BSIM3weta0Given   :1;    
    unsigned  BSIM3wetabGiven   :1;    
    unsigned  BSIM3wpclmGiven   :1;   
    unsigned  BSIM3wpdibl1Given   :1;   
    unsigned  BSIM3wpdibl2Given   :1;  
    unsigned  BSIM3wpdiblbGiven   :1;  
    unsigned  BSIM3wpscbe1Given   :1;    
    unsigned  BSIM3wpscbe2Given   :1;    
    unsigned  BSIM3wpvagGiven   :1;    
    unsigned  BSIM3wdeltaGiven  :1;     
    unsigned  BSIM3wwrGiven   :1;
    unsigned  BSIM3wdwgGiven   :1;
    unsigned  BSIM3wdwbGiven   :1;
    unsigned  BSIM3wb0Given   :1;
    unsigned  BSIM3wb1Given   :1;
    unsigned  BSIM3walpha0Given   :1;
    unsigned  BSIM3walpha1Given   :1;
    unsigned  BSIM3wbeta0Given   :1;
    unsigned  BSIM3wvfbGiven   :1;

    /* CV model */
    unsigned  BSIM3welmGiven  :1;     
    unsigned  BSIM3wcgslGiven   :1;
    unsigned  BSIM3wcgdlGiven   :1;
    unsigned  BSIM3wckappaGiven   :1;
    unsigned  BSIM3wcfGiven   :1;
    unsigned  BSIM3wclcGiven   :1;
    unsigned  BSIM3wcleGiven   :1;
    unsigned  BSIM3wvfbcvGiven   :1;
    unsigned  BSIM3wnoffGiven   :1;
    unsigned  BSIM3wvoffcvGiven :1;
    unsigned  BSIM3wacdeGiven   :1;
    unsigned  BSIM3wmoinGiven   :1;

    /* Cross-term dependence */
    unsigned  BSIM3pcdscGiven   :1;
    unsigned  BSIM3pcdscbGiven   :1;
    unsigned  BSIM3pcdscdGiven   :1;
    unsigned  BSIM3pcitGiven   :1;
    unsigned  BSIM3pnfactorGiven   :1;
    unsigned  BSIM3pxjGiven   :1;
    unsigned  BSIM3pvsatGiven   :1;
    unsigned  BSIM3patGiven   :1;
    unsigned  BSIM3pa0Given   :1;
    unsigned  BSIM3pagsGiven   :1;
    unsigned  BSIM3pa1Given   :1;
    unsigned  BSIM3pa2Given   :1;
    unsigned  BSIM3pketaGiven   :1;    
    unsigned  BSIM3pnsubGiven   :1;
    unsigned  BSIM3pnpeakGiven   :1;
    unsigned  BSIM3pngateGiven   :1;
    unsigned  BSIM3pgamma1Given   :1;
    unsigned  BSIM3pgamma2Given   :1;
    unsigned  BSIM3pvbxGiven   :1;
    unsigned  BSIM3pvbmGiven   :1;
    unsigned  BSIM3pxtGiven   :1;
    unsigned  BSIM3pk1Given   :1;
    unsigned  BSIM3pkt1Given   :1;
    unsigned  BSIM3pkt1lGiven   :1;
    unsigned  BSIM3pkt2Given   :1;
    unsigned  BSIM3pk2Given   :1;
    unsigned  BSIM3pk3Given   :1;
    unsigned  BSIM3pk3bGiven   :1;
    unsigned  BSIM3pw0Given   :1;
    unsigned  BSIM3pnlxGiven   :1;
    unsigned  BSIM3pdvt0Given   :1;   
    unsigned  BSIM3pdvt1Given   :1;     
    unsigned  BSIM3pdvt2Given   :1;     
    unsigned  BSIM3pdvt0wGiven   :1;   
    unsigned  BSIM3pdvt1wGiven   :1;     
    unsigned  BSIM3pdvt2wGiven   :1;     
    unsigned  BSIM3pdroutGiven   :1;     
    unsigned  BSIM3pdsubGiven   :1;     
    unsigned  BSIM3pvth0Given   :1;
    unsigned  BSIM3puaGiven   :1;
    unsigned  BSIM3pua1Given   :1;
    unsigned  BSIM3pubGiven   :1;
    unsigned  BSIM3pub1Given   :1;
    unsigned  BSIM3pucGiven   :1;
    unsigned  BSIM3puc1Given   :1;
    unsigned  BSIM3pu0Given   :1;
    unsigned  BSIM3puteGiven   :1;
    unsigned  BSIM3pvoffGiven   :1;
    unsigned  BSIM3prdswGiven   :1;      
    unsigned  BSIM3pprwgGiven   :1;      
    unsigned  BSIM3pprwbGiven   :1;      
    unsigned  BSIM3pprtGiven   :1;      
    unsigned  BSIM3peta0Given   :1;    
    unsigned  BSIM3petabGiven   :1;    
    unsigned  BSIM3ppclmGiven   :1;   
    unsigned  BSIM3ppdibl1Given   :1;   
    unsigned  BSIM3ppdibl2Given   :1;  
    unsigned  BSIM3ppdiblbGiven   :1;  
    unsigned  BSIM3ppscbe1Given   :1;    
    unsigned  BSIM3ppscbe2Given   :1;    
    unsigned  BSIM3ppvagGiven   :1;    
    unsigned  BSIM3pdeltaGiven  :1;     
    unsigned  BSIM3pwrGiven   :1;
    unsigned  BSIM3pdwgGiven   :1;
    unsigned  BSIM3pdwbGiven   :1;
    unsigned  BSIM3pb0Given   :1;
    unsigned  BSIM3pb1Given   :1;
    unsigned  BSIM3palpha0Given   :1;
    unsigned  BSIM3palpha1Given   :1;
    unsigned  BSIM3pbeta0Given   :1;
    unsigned  BSIM3pvfbGiven   :1;

    /* CV model */
    unsigned  BSIM3pelmGiven  :1;     
    unsigned  BSIM3pcgslGiven   :1;
    unsigned  BSIM3pcgdlGiven   :1;
    unsigned  BSIM3pckappaGiven   :1;
    unsigned  BSIM3pcfGiven   :1;
    unsigned  BSIM3pclcGiven   :1;
    unsigned  BSIM3pcleGiven   :1;
    unsigned  BSIM3pvfbcvGiven   :1;
    unsigned  BSIM3pnoffGiven   :1;
    unsigned  BSIM3pvoffcvGiven :1;
    unsigned  BSIM3pacdeGiven   :1;
    unsigned  BSIM3pmoinGiven   :1;

    unsigned  BSIM3useFringeGiven   :1;

    unsigned  BSIM3tnomGiven   :1;
    unsigned  BSIM3cgsoGiven   :1;
    unsigned  BSIM3cgdoGiven   :1;
    unsigned  BSIM3cgboGiven   :1;
    unsigned  BSIM3xpartGiven   :1;
    unsigned  BSIM3sheetResistanceGiven   :1;
    unsigned  BSIM3jctSatCurDensityGiven   :1;
    unsigned  BSIM3jctSidewallSatCurDensityGiven   :1;
    unsigned  BSIM3bulkJctPotentialGiven   :1;
    unsigned  BSIM3bulkJctBotGradingCoeffGiven   :1;
    unsigned  BSIM3sidewallJctPotentialGiven   :1;
    unsigned  BSIM3GatesidewallJctPotentialGiven   :1;
    unsigned  BSIM3bulkJctSideGradingCoeffGiven   :1;
    unsigned  BSIM3unitAreaJctCapGiven   :1;
    unsigned  BSIM3unitLengthSidewallJctCapGiven   :1;
    unsigned  BSIM3bulkJctGateSideGradingCoeffGiven   :1;
    unsigned  BSIM3unitLengthGateSidewallJctCapGiven   :1;
    unsigned  BSIM3jctEmissionCoeffGiven :1;
    unsigned  BSIM3jctTempExponentGiven	:1;

    unsigned  BSIM3oxideTrapDensityAGiven  :1;         
    unsigned  BSIM3oxideTrapDensityBGiven  :1;        
    unsigned  BSIM3oxideTrapDensityCGiven  :1;     
    unsigned  BSIM3emGiven  :1;     
    unsigned  BSIM3efGiven  :1;     
    unsigned  BSIM3afGiven  :1;     
    unsigned  BSIM3kfGiven  :1;     
    unsigned  BSIM3lintnoiGiven  :1;

    unsigned  BSIM3LintGiven   :1;
    unsigned  BSIM3LlGiven   :1;
    unsigned  BSIM3LlcGiven   :1;
    unsigned  BSIM3LlnGiven   :1;
    unsigned  BSIM3LwGiven   :1;
    unsigned  BSIM3LwcGiven   :1;
    unsigned  BSIM3LwnGiven   :1;
    unsigned  BSIM3LwlGiven   :1;
    unsigned  BSIM3LwlcGiven   :1;
    unsigned  BSIM3LminGiven   :1;
    unsigned  BSIM3LmaxGiven   :1;

    unsigned  BSIM3WintGiven   :1;
    unsigned  BSIM3WlGiven   :1;
    unsigned  BSIM3WlcGiven   :1;
    unsigned  BSIM3WlnGiven   :1;
    unsigned  BSIM3WwGiven   :1;
    unsigned  BSIM3WwcGiven   :1;
    unsigned  BSIM3WwnGiven   :1;
    unsigned  BSIM3WwlGiven   :1;
    unsigned  BSIM3WwlcGiven   :1;
    unsigned  BSIM3WminGiven   :1;
    unsigned  BSIM3WmaxGiven   :1;

// SRW
    unsigned  BSIM3xwGiven   :1;
    unsigned  BSIM3xlGiven   :1;
    unsigned  BSIM3nqsModGiven :1;
};

struct sBSIM3model : sGENmodel, sBSIM3modelPOD
{
    sBSIM3model() : sGENmodel(), sBSIM3modelPOD() { }

    sBSIM3model *next()     { return ((sBSIM3model*)GENnextModel); }
    sBSIM3instance *inst()  { return ((sBSIM3instance*)GENinstances); }
};
} // namespace BSIM330
using namespace BSIM330;


#ifndef NMOS
#define NMOS 1
#define PMOS -1
#endif

// device parameters
enum {
    BSIM3_W = 1,
    BSIM3_L,
    BSIM3_AS,
    BSIM3_AD,
    BSIM3_PS,
    BSIM3_PD,
    BSIM3_NRS,
    BSIM3_NRD,
    BSIM3_OFF,
    BSIM3_IC_VBS,
    BSIM3_IC_VDS,
    BSIM3_IC_VGS,
    BSIM3_IC,
    BSIM3_NQSMOD,
    BSIM3_ACNQSMOD,

    BSIM3_DNODE,
    BSIM3_GNODE,
    BSIM3_SNODE,
    BSIM3_BNODE,
    BSIM3_DNODEPRIME,
    BSIM3_SNODEPRIME,
    BSIM3_VBD,
    BSIM3_VBS,
    BSIM3_VGS,
    BSIM3_VDS,
    BSIM3_CD,
    BSIM3_CBS,
    BSIM3_CBD,
    BSIM3_GM,
    BSIM3_GDS,
    BSIM3_GMBS,
    BSIM3_GBD,
    BSIM3_GBS,
    BSIM3_QB,
    BSIM3_CQB,
    BSIM3_QG,
    BSIM3_CQG,
    BSIM3_QD,
    BSIM3_CQD,
    BSIM3_CGG,
    BSIM3_CGD,
    BSIM3_CGS,
    BSIM3_CBG,
    BSIM3_CAPBD,
    BSIM3_CQBD,
    BSIM3_CAPBS,
    BSIM3_CQBS,
    BSIM3_CDG,
    BSIM3_CDD,
    BSIM3_CDS,
    BSIM3_VON,
    BSIM3_VDSAT,
    BSIM3_QBS,
    BSIM3_QBD,
    BSIM3_SOURCECONDUCT,
    BSIM3_DRAINCONDUCT,
    BSIM3_CBDB,
    BSIM3_CBSB,

    // SRW - added these
    BSIM3_ID,
    BSIM3_IS,
    BSIM3_IG,
    BSIM3_IB
};

// model parameters
enum {
    BSIM3_MOD_CAPMOD = 1000,
    // SRW
    BSIM3_MOD_NQSMOD,
    BSIM3_MOD_MOBMOD,
    BSIM3_MOD_NOIMOD,

    BSIM3_MOD_TOX,

    BSIM3_MOD_CDSC,
    BSIM3_MOD_CDSCB,
    BSIM3_MOD_CIT,
    BSIM3_MOD_NFACTOR,
    BSIM3_MOD_XJ,
    BSIM3_MOD_VSAT,
    BSIM3_MOD_AT,
    BSIM3_MOD_A0,
    BSIM3_MOD_A1,
    BSIM3_MOD_A2,
    BSIM3_MOD_KETA,
    BSIM3_MOD_NSUB,
    BSIM3_MOD_NPEAK,
    BSIM3_MOD_NGATE,
    BSIM3_MOD_GAMMA1,
    BSIM3_MOD_GAMMA2,
    BSIM3_MOD_VBX,
    BSIM3_MOD_BINUNIT,

    BSIM3_MOD_VBM,

    BSIM3_MOD_XT,
    BSIM3_MOD_K1,
    BSIM3_MOD_KT1,
    BSIM3_MOD_KT1L,
    BSIM3_MOD_K2,
    BSIM3_MOD_KT2,
    BSIM3_MOD_K3,
    BSIM3_MOD_K3B,
    BSIM3_MOD_W0,
    BSIM3_MOD_NLX,

    BSIM3_MOD_DVT0,
    BSIM3_MOD_DVT1,
    BSIM3_MOD_DVT2,

    BSIM3_MOD_DVT0W,
    BSIM3_MOD_DVT1W,
    BSIM3_MOD_DVT2W,

    BSIM3_MOD_DROUT,
    BSIM3_MOD_DSUB,
    BSIM3_MOD_VTH0,
    BSIM3_MOD_UA,
    BSIM3_MOD_UA1,
    BSIM3_MOD_UB,
    BSIM3_MOD_UB1,
    BSIM3_MOD_UC,
    BSIM3_MOD_UC1,
    BSIM3_MOD_U0,
    BSIM3_MOD_UTE,
    BSIM3_MOD_VOFF,
    BSIM3_MOD_DELTA,
    BSIM3_MOD_RDSW,
    BSIM3_MOD_PRT,
    BSIM3_MOD_LDD,
    BSIM3_MOD_ETA,
    BSIM3_MOD_ETA0,
    BSIM3_MOD_ETAB,
    BSIM3_MOD_PCLM,
    BSIM3_MOD_PDIBL1,
    BSIM3_MOD_PDIBL2,
    BSIM3_MOD_PSCBE1,
    BSIM3_MOD_PSCBE2,
    BSIM3_MOD_PVAG,
    BSIM3_MOD_WR,
    BSIM3_MOD_DWG,
    BSIM3_MOD_DWB,
    BSIM3_MOD_B0,
    BSIM3_MOD_B1,
    BSIM3_MOD_ALPHA0,
    BSIM3_MOD_BETA0,
    BSIM3_MOD_PDIBLB,

    BSIM3_MOD_PRWG,
    BSIM3_MOD_PRWB,

    BSIM3_MOD_CDSCD,
    BSIM3_MOD_AGS,

    BSIM3_MOD_FRINGE,
    BSIM3_MOD_ELM,
    BSIM3_MOD_CGSL,
    BSIM3_MOD_CGDL,
    BSIM3_MOD_CKAPPA,
    BSIM3_MOD_CF,
    BSIM3_MOD_CLC,
    BSIM3_MOD_CLE,
    BSIM3_MOD_PARAMCHK,
    BSIM3_MOD_VERSION,
    BSIM3_MOD_VFBCV,
    BSIM3_MOD_ACDE,
    BSIM3_MOD_MOIN,
    BSIM3_MOD_NOFF,
    BSIM3_MOD_IJTH,
    BSIM3_MOD_ALPHA1,
    BSIM3_MOD_VFB,
    BSIM3_MOD_TOXM,
    BSIM3_MOD_TCJ,
    BSIM3_MOD_TCJSW,
    BSIM3_MOD_TCJSWG,
    BSIM3_MOD_TPB,
    BSIM3_MOD_TPBSW,
    BSIM3_MOD_TPBSWG,
    BSIM3_MOD_VOFFCV,
    BSIM3_MOD_LINTNOI,
    BSIM3_MOD_ACNQSMOD,

    // Length dependence
    BSIM3_MOD_LCDSC,
    BSIM3_MOD_LCDSCB,
    BSIM3_MOD_LCIT,
    BSIM3_MOD_LNFACTOR,
    BSIM3_MOD_LXJ,
    BSIM3_MOD_LVSAT,
    BSIM3_MOD_LAT,
    BSIM3_MOD_LA0,
    BSIM3_MOD_LA1,
    BSIM3_MOD_LA2,
    BSIM3_MOD_LKETA,
    BSIM3_MOD_LNSUB,
    BSIM3_MOD_LNPEAK,
    BSIM3_MOD_LNGATE,
    BSIM3_MOD_LGAMMA1,
    BSIM3_MOD_LGAMMA2,
    BSIM3_MOD_LVBX,

    BSIM3_MOD_LVBM,

    BSIM3_MOD_LXT,
    BSIM3_MOD_LK1,
    BSIM3_MOD_LKT1,
    BSIM3_MOD_LKT1L,
    BSIM3_MOD_LK2,
    BSIM3_MOD_LKT2,
    BSIM3_MOD_LK3,
    BSIM3_MOD_LK3B,
    BSIM3_MOD_LW0,
    BSIM3_MOD_LNLX,

    BSIM3_MOD_LDVT0,
    BSIM3_MOD_LDVT1,
    BSIM3_MOD_LDVT2,

    BSIM3_MOD_LDVT0W,
    BSIM3_MOD_LDVT1W,
    BSIM3_MOD_LDVT2W,

    BSIM3_MOD_LDROUT,
    BSIM3_MOD_LDSUB,
    BSIM3_MOD_LVTH0,
    BSIM3_MOD_LUA,
    BSIM3_MOD_LUA1,
    BSIM3_MOD_LUB,
    BSIM3_MOD_LUB1,
    BSIM3_MOD_LUC,
    BSIM3_MOD_LUC1,
    BSIM3_MOD_LU0,
    BSIM3_MOD_LUTE,
    BSIM3_MOD_LVOFF,
    BSIM3_MOD_LDELTA,
    BSIM3_MOD_LRDSW,
    BSIM3_MOD_LPRT,
    BSIM3_MOD_LLDD,
    BSIM3_MOD_LETA,
    BSIM3_MOD_LETA0,
    BSIM3_MOD_LETAB,
    BSIM3_MOD_LPCLM,
    BSIM3_MOD_LPDIBL1,
    BSIM3_MOD_LPDIBL2,
    BSIM3_MOD_LPSCBE1,
    BSIM3_MOD_LPSCBE2,
    BSIM3_MOD_LPVAG,
    BSIM3_MOD_LWR,
    BSIM3_MOD_LDWG,
    BSIM3_MOD_LDWB,
    BSIM3_MOD_LB0,
    BSIM3_MOD_LB1,
    BSIM3_MOD_LALPHA0,
    BSIM3_MOD_LBETA0,
    BSIM3_MOD_LPDIBLB,

    BSIM3_MOD_LPRWG,
    BSIM3_MOD_LPRWB,

    BSIM3_MOD_LCDSCD,
    BSIM3_MOD_LAGS,

    BSIM3_MOD_LFRINGE,
    BSIM3_MOD_LELM,
    BSIM3_MOD_LCGSL,
    BSIM3_MOD_LCGDL,
    BSIM3_MOD_LCKAPPA,
    BSIM3_MOD_LCF,
    BSIM3_MOD_LCLC,
    BSIM3_MOD_LCLE,
    BSIM3_MOD_LVFBCV,
    BSIM3_MOD_LACDE,
    BSIM3_MOD_LMOIN,
    BSIM3_MOD_LNOFF,
    BSIM3_MOD_LALPHA1,
    BSIM3_MOD_LVFB,
    BSIM3_MOD_LVOFFCV,

    // Width dependence
    BSIM3_MOD_WCDSC,
    BSIM3_MOD_WCDSCB,
    BSIM3_MOD_WCIT,
    BSIM3_MOD_WNFACTOR,
    BSIM3_MOD_WXJ,
    BSIM3_MOD_WVSAT,
    BSIM3_MOD_WAT,
    BSIM3_MOD_WA0,
    BSIM3_MOD_WA1,
    BSIM3_MOD_WA2,
    BSIM3_MOD_WKETA,
    BSIM3_MOD_WNSUB,
    BSIM3_MOD_WNPEAK,
    BSIM3_MOD_WNGATE,
    BSIM3_MOD_WGAMMA1,
    BSIM3_MOD_WGAMMA2,
    BSIM3_MOD_WVBX,

    BSIM3_MOD_WVBM,

    BSIM3_MOD_WXT,
    BSIM3_MOD_WK1,
    BSIM3_MOD_WKT1,
    BSIM3_MOD_WKT1L,
    BSIM3_MOD_WK2,
    BSIM3_MOD_WKT2,
    BSIM3_MOD_WK3,
    BSIM3_MOD_WK3B,
    BSIM3_MOD_WW0,
    BSIM3_MOD_WNLX,

    BSIM3_MOD_WDVT0,
    BSIM3_MOD_WDVT1,
    BSIM3_MOD_WDVT2,

    BSIM3_MOD_WDVT0W,
    BSIM3_MOD_WDVT1W,
    BSIM3_MOD_WDVT2W,

    BSIM3_MOD_WDROUT,
    BSIM3_MOD_WDSUB,
    BSIM3_MOD_WVTH0,
    BSIM3_MOD_WUA,
    BSIM3_MOD_WUA1,
    BSIM3_MOD_WUB,
    BSIM3_MOD_WUB1,
    BSIM3_MOD_WUC,
    BSIM3_MOD_WUC1,
    BSIM3_MOD_WU0,
    BSIM3_MOD_WUTE,
    BSIM3_MOD_WVOFF,
    BSIM3_MOD_WDELTA,
    BSIM3_MOD_WRDSW,
    BSIM3_MOD_WPRT,
    BSIM3_MOD_WLDD,
    BSIM3_MOD_WETA,
    BSIM3_MOD_WETA0,
    BSIM3_MOD_WETAB,
    BSIM3_MOD_WPCLM,
    BSIM3_MOD_WPDIBL1,
    BSIM3_MOD_WPDIBL2,
    BSIM3_MOD_WPSCBE1,
    BSIM3_MOD_WPSCBE2,
    BSIM3_MOD_WPVAG,
    BSIM3_MOD_WWR,
    BSIM3_MOD_WDWG,
    BSIM3_MOD_WDWB,
    BSIM3_MOD_WB0,
    BSIM3_MOD_WB1,
    BSIM3_MOD_WALPHA0,
    BSIM3_MOD_WBETA0,
    BSIM3_MOD_WPDIBLB,

    BSIM3_MOD_WPRWG,
    BSIM3_MOD_WPRWB,

    BSIM3_MOD_WCDSCD,
    BSIM3_MOD_WAGS,

    BSIM3_MOD_WFRINGE,
    BSIM3_MOD_WELM,
    BSIM3_MOD_WCGSL,
    BSIM3_MOD_WCGDL,
    BSIM3_MOD_WCKAPPA,
    BSIM3_MOD_WCF,
    BSIM3_MOD_WCLC,
    BSIM3_MOD_WCLE,
    BSIM3_MOD_WVFBCV,
    BSIM3_MOD_WACDE,
    BSIM3_MOD_WMOIN,
    BSIM3_MOD_WNOFF,
    BSIM3_MOD_WALPHA1,
    BSIM3_MOD_WVFB,
    BSIM3_MOD_WVOFFCV,

    // Cross-term dependence
    BSIM3_MOD_PCDSC,
    BSIM3_MOD_PCDSCB,
    BSIM3_MOD_PCIT,
    BSIM3_MOD_PNFACTOR,
    BSIM3_MOD_PXJ,
    BSIM3_MOD_PVSAT,
    BSIM3_MOD_PAT,
    BSIM3_MOD_PA0,
    BSIM3_MOD_PA1,
    BSIM3_MOD_PA2,
    BSIM3_MOD_PKETA,
    BSIM3_MOD_PNSUB,
    BSIM3_MOD_PNPEAK,
    BSIM3_MOD_PNGATE,
    BSIM3_MOD_PGAMMA1,
    BSIM3_MOD_PGAMMA2,
    BSIM3_MOD_PVBX,

    BSIM3_MOD_PVBM,

    BSIM3_MOD_PXT,
    BSIM3_MOD_PK1,
    BSIM3_MOD_PKT1,
    BSIM3_MOD_PKT1L,
    BSIM3_MOD_PK2,
    BSIM3_MOD_PKT2,
    BSIM3_MOD_PK3,
    BSIM3_MOD_PK3B,
    BSIM3_MOD_PW0,
    BSIM3_MOD_PNLX,

    BSIM3_MOD_PDVT0,
    BSIM3_MOD_PDVT1,
    BSIM3_MOD_PDVT2,

    BSIM3_MOD_PDVT0W,
    BSIM3_MOD_PDVT1W,
    BSIM3_MOD_PDVT2W,

    BSIM3_MOD_PDROUT,
    BSIM3_MOD_PDSUB,
    BSIM3_MOD_PVTH0,
    BSIM3_MOD_PUA,
    BSIM3_MOD_PUA1,
    BSIM3_MOD_PUB,
    BSIM3_MOD_PUB1,
    BSIM3_MOD_PUC,
    BSIM3_MOD_PUC1,
    BSIM3_MOD_PU0,
    BSIM3_MOD_PUTE,
    BSIM3_MOD_PVOFF,
    BSIM3_MOD_PDELTA,
    BSIM3_MOD_PRDSW,
    BSIM3_MOD_PPRT,
    BSIM3_MOD_PLDD,
    BSIM3_MOD_PETA,
    BSIM3_MOD_PETA0,
    BSIM3_MOD_PETAB,
    BSIM3_MOD_PPCLM,
    BSIM3_MOD_PPDIBL1,
    BSIM3_MOD_PPDIBL2,
    BSIM3_MOD_PPSCBE1,
    BSIM3_MOD_PPSCBE2,
    BSIM3_MOD_PPVAG,
    BSIM3_MOD_PWR,
    BSIM3_MOD_PDWG,
    BSIM3_MOD_PDWB,
    BSIM3_MOD_PB0,
    BSIM3_MOD_PB1,
    BSIM3_MOD_PALPHA0,
    BSIM3_MOD_PBETA0,
    BSIM3_MOD_PPDIBLB,

    BSIM3_MOD_PPRWG,
    BSIM3_MOD_PPRWB,

    BSIM3_MOD_PCDSCD,
    BSIM3_MOD_PAGS,

    BSIM3_MOD_PFRINGE,
    BSIM3_MOD_PELM,
    BSIM3_MOD_PCGSL,
    BSIM3_MOD_PCGDL,
    BSIM3_MOD_PCKAPPA,
    BSIM3_MOD_PCF,
    BSIM3_MOD_PCLC,
    BSIM3_MOD_PCLE,
    BSIM3_MOD_PVFBCV,
    BSIM3_MOD_PACDE,
    BSIM3_MOD_PMOIN,
    BSIM3_MOD_PNOFF,
    BSIM3_MOD_PALPHA1,
    BSIM3_MOD_PVFB,
    BSIM3_MOD_PVOFFCV,

    BSIM3_MOD_TNOM,
    BSIM3_MOD_CGSO,
    BSIM3_MOD_CGDO,
    BSIM3_MOD_CGBO,
    BSIM3_MOD_XPART,

    BSIM3_MOD_RSH,
    BSIM3_MOD_JS,
    BSIM3_MOD_PB,
    BSIM3_MOD_MJ,
    BSIM3_MOD_PBSW,
    BSIM3_MOD_MJSW,
    BSIM3_MOD_CJ,
    BSIM3_MOD_CJSW,
    BSIM3_MOD_NMOS,
    BSIM3_MOD_PMOS,

    BSIM3_MOD_NOIA,
    BSIM3_MOD_NOIB,
    BSIM3_MOD_NOIC,

    BSIM3_MOD_LINT,
    BSIM3_MOD_LL,
    BSIM3_MOD_LLN,
    BSIM3_MOD_LW,
    BSIM3_MOD_LWN,
    BSIM3_MOD_LWL,
    BSIM3_MOD_LMIN,
    BSIM3_MOD_LMAX,

    BSIM3_MOD_WINT,
    BSIM3_MOD_WL,
    BSIM3_MOD_WLN,
    BSIM3_MOD_WW,
    BSIM3_MOD_WWN,
    BSIM3_MOD_WWL,
    BSIM3_MOD_WMIN,
    BSIM3_MOD_WMAX,

    BSIM3_MOD_DWC,
    BSIM3_MOD_DLC,

    BSIM3_MOD_EM,
    BSIM3_MOD_EF,
    BSIM3_MOD_AF,
    BSIM3_MOD_KF,

    BSIM3_MOD_NJ,
    BSIM3_MOD_XTI,

    BSIM3_MOD_PBSWG,
    BSIM3_MOD_MJSWG,
    BSIM3_MOD_CJSWG,
    BSIM3_MOD_JSW,

    BSIM3_MOD_LLC,
    BSIM3_MOD_LWC,
    BSIM3_MOD_LWLC,

    BSIM3_MOD_WLC,
    BSIM3_MOD_WWC,
    BSIM3_MOD_WWLC,

    // SRW
    BSIM3_MOD_XW,
    BSIM3_MOD_XL
};

#endif // B3DEFS_H

