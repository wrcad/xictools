
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
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1995 Min-Chie Jeng and Mansun Chan
Modified by Weidong Liu (1997-1998).
 * Revision 3.2 1998/6/16  18:00:00  Weidong 
 * BSIM3v3.2 release
**********/

#ifndef B3DEFS_H
#define B3DEFS_H

#include "device.h"


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

namespace BSIM32 {

struct sB3model;
struct sB3instance;

struct B3dev : public IFdevice
{
    B3dev();
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
    int checkModel(sB3model*, sB3instance*, sCKT*);
};

struct sB3instancePOD
{
    int B3dNode;   // number of the drain node of the mosfet
    int B3gNode;   // number of the gate node of the mosfet
    int B3sNode;   // number of the source node of the mosfet
    int B3bNode;   // number of the bulk node of the mosfet

    int B3dNodePrime;
    int B3sNodePrime;
    int B3qNode; // MCJ

    // This provides a means to back up and restore a known-good
    // state.
    void *B3backing;

    // MCJ
    double B3ueff;
    double B3thetavth; 
    double B3von;
    double B3vdsat;
    double B3cgdo;
    double B3cgso;
    double B3vjsm;
    double B3vjdm;

    double B3m;
    double B3l;
    double B3w;
    double B3drainArea;
    double B3sourceArea;
    double B3drainSquares;
    double B3sourceSquares;
    double B3drainPerimeter;
    double B3sourcePerimeter;
    double B3sourceConductance;
    double B3drainConductance;

    double B3icVBS;
    double B3icVDS;
    double B3icVGS;
    int B3off;
    int B3mode;
    int B3nqsMod;

    // OP point
    double B3qinv;
    double B3cd;
    double B3cbs;
    double B3cbd;
    double B3csub;
    double B3gm;
    double B3gds;
    double B3gmbs;
    double B3gbd;
    double B3gbs;

    double B3gbbs;
    double B3gbgs;
    double B3gbds;

    double B3cggb;
    double B3cgdb;
    double B3cgsb;
    double B3cbgb;
    double B3cbdb;
    double B3cbsb;
    double B3cdgb;
    double B3cddb;
    double B3cdsb;
    double B3capbd;
    double B3capbs;

    double B3cqgb;
    double B3cqdb;
    double B3cqsb;
    double B3cqbb;

    double B3qgate;
    double B3qbulk;
    double B3qdrn;

    double B3gtau;
    double B3gtg;
    double B3gtd;
    double B3gts;
    double B3gtb;
    double B3tconst;

    struct bsim3SizeDependParam  *pParam;

    unsigned B3mGiven :1;
    unsigned B3lGiven :1;
    unsigned B3wGiven :1;
    unsigned B3drainAreaGiven :1;
    unsigned B3sourceAreaGiven    :1;
    unsigned B3drainSquaresGiven  :1;
    unsigned B3sourceSquaresGiven :1;
    unsigned B3drainPerimeterGiven    :1;
    unsigned B3sourcePerimeterGiven   :1;
    unsigned B3dNodePrimeSet  :1;
    unsigned B3sNodePrimeSet  :1;
    unsigned B3icVBSGiven :1;
    unsigned B3icVDSGiven :1;
    unsigned B3icVGSGiven :1;
    unsigned B3nqsModGiven :1;

    double *B3DdPtr;
    double *B3GgPtr;
    double *B3SsPtr;
    double *B3BbPtr;
    double *B3DPdpPtr;
    double *B3SPspPtr;
    double *B3DdpPtr;
    double *B3GbPtr;
    double *B3GdpPtr;
    double *B3GspPtr;
    double *B3SspPtr;
    double *B3BdpPtr;
    double *B3BspPtr;
    double *B3DPspPtr;
    double *B3DPdPtr;
    double *B3BgPtr;
    double *B3DPgPtr;
    double *B3SPgPtr;
    double *B3SPsPtr;
    double *B3DPbPtr;
    double *B3SPbPtr;
    double *B3SPdpPtr;

    double *B3QqPtr;
    double *B3QdpPtr;
    double *B3QgPtr;
    double *B3QspPtr;
    double *B3QbPtr;
    double *B3DPqPtr;
    double *B3GqPtr;
    double *B3SPqPtr;
    double *B3BqPtr;

// indices to the array of B3 NOISE SOURCES
#define B3RDNOIZ       0
#define B3RSNOIZ       1
#define B3IDNOIZ       2
#define B3FLNOIZ       3
#define B3TOTNOIZ      4
#define B3NSRCS        5     // the number of MOSFET(3) noise sources

#ifndef NONOISE
    double B3nVar[NSTATVARS][B3NSRCS];
#else
    double **B3nVar;
#endif
};

// state table
#define B3vbd       GENstate + 0
#define B3vbs       GENstate + 1
#define B3vgs       GENstate + 2
#define B3vds       GENstate + 3

#define B3qb        GENstate + 4
#define B3cqb       GENstate + 5
#define B3qg        GENstate + 6
#define B3cqg       GENstate + 7
#define B3qd        GENstate + 8
#define B3cqd       GENstate + 9

#define B3qbs       GENstate + 10
#define B3qbd       GENstate + 11

#define B3qcheq     GENstate + 12
#define B3cqcheq    GENstate + 13
#define B3qcdump    GENstate + 14
#define B3cqcdump   GENstate + 15

#define B3qdef      GENstate + 16

// for interpolation
#define B3a_cd      GENstate + 17
#define B3a_cbs     GENstate + 18
#define B3a_cbd     GENstate + 19
#define B3a_gm      GENstate + 20
#define B3a_gds     GENstate + 21
#define B3a_gmbs    GENstate + 22
#define B3a_gbd     GENstate + 23
#define B3a_gbs     GENstate + 24
#define B3a_cggb    GENstate + 25
#define B3a_cgdb    GENstate + 26
#define B3a_cgsb    GENstate + 27
#define B3a_cdgb    GENstate + 28
#define B3a_cddb    GENstate + 29
#define B3a_cdsb    GENstate + 30
#define B3a_cbgb    GENstate + 31
#define B3a_cbdb    GENstate + 32
#define B3a_cbsb    GENstate + 33
#define B3a_capbd   GENstate + 34
#define B3a_capbs   GENstate + 35
#define B3a_von     GENstate + 36
#define B3a_vdsat   GENstate + 37

#define B3numStates 38

struct sB3instance : sGENinstance, sB3instancePOD
{
    sB3instance() : sGENinstance(), sB3instancePOD()
        { GENnumNodes = 4; }
    ~sB3instance()  { delete [] (char*)B3backing; }

    sB3instance *next()
        { return (static_cast<sB3instance*>(GENnextInstance)); }

    void backup(DEV_BKMODE m)
        {
            if (m == DEV_SAVE) {
                if (!B3backing)
                    B3backing = new char[sizeof(sB3instance)];
                memcpy(B3backing, this, sizeof(sB3instance));
            }
            else if (m == DEV_RESTORE) {
                if (B3backing)
                    memcpy(this, B3backing, sizeof(sB3instance));
            }
            else {
                // DEV_CLEAR
                delete [] (char*)B3backing;
                B3backing = 0;
            }
        }

    void ac_cd(const sCKT*, double*, double*) const;
    void ac_cs(const sCKT*, double*, double*) const;
    void ac_cg(const sCKT*, double*, double*) const;
    void ac_cb(const sCKT*, double*, double*) const;
};

struct bsim3SizeDependParam
{
    double Width;
    double Length;

    double B3cdsc;           
    double B3cdscb;    
    double B3cdscd;       
    double B3cit;           
    double B3nfactor;      
    double B3xj;
    double B3vsat;         
    double B3at;         
    double B3a0;   
    double B3ags;      
    double B3a1;         
    double B3a2;         
    double B3keta;     
    double B3nsub;
    double B3npeak;        
    double B3ngate;        
    double B3gamma1;      
    double B3gamma2;     
    double B3vbx;      
    double B3vbi;       
    double B3vbm;       
    double B3vbsc;       
    double B3xt;       
    double B3phi;
    double B3litl;
    double B3k1;
    double B3kt1;
    double B3kt1l;
    double B3kt2;
    double B3k2;
    double B3k3;
    double B3k3b;
    double B3w0;
    double B3nlx;
    double B3dvt0;      
    double B3dvt1;      
    double B3dvt2;      
    double B3dvt0w;      
    double B3dvt1w;      
    double B3dvt2w;      
    double B3drout;      
    double B3dsub;      
    double B3vth0;
    double B3ua;
    double B3ua1;
    double B3ub;
    double B3ub1;
    double B3uc;
    double B3uc1;
    double B3u0;
    double B3ute;
    double B3voff;
    double B3vfb;
    double B3delta;
    double B3rdsw;       
    double B3rds0;       
    double B3prwg;       
    double B3prwb;       
    double B3prt;       
    double B3eta0;         
    double B3etab;         
    double B3pclm;      
    double B3pdibl1;      
    double B3pdibl2;      
    double B3pdiblb;      
    double B3pscbe1;       
    double B3pscbe2;       
    double B3pvag;       
    double B3wr;
    double B3dwg;
    double B3dwb;
    double B3b0;
    double B3b1;
    double B3alpha0;
    double B3alpha1;
    double B3beta0;

    // CV model
    double B3elm;
    double B3cgsl;
    double B3cgdl;
    double B3ckappa;
    double B3cf;
    double B3clc;
    double B3cle;
    double B3vfbcv;
    double B3noff;
    double B3voffcv;
    double B3acde;
    double B3moin;

    // Pre-calculated constants

    double B3dw;
    double B3dl;
    double B3leff;
    double B3weff;

    double B3dwc;
    double B3dlc;
    double B3leffCV;
    double B3weffCV;
    double B3abulkCVfactor;
    double B3cgso;
    double B3cgdo;
    double B3cgbo;
    double B3tconst;

    double B3u0temp;       
    double B3vsattemp;   
    double B3sqrtPhi;   
    double B3phis3;   
    double B3Xdep0;          
    double B3sqrtXdep0;          
    double B3theta0vb0;
    double B3thetaRout; 

    double B3cof1;
    double B3cof2;
    double B3cof3;
    double B3cof4;
    double B3cdep0;
    double B3vfbzb;
    double B3ldeb;
    double B3k1ox;
    double B3k2ox;

    struct bsim3SizeDependParam  *pNext;
};

struct sB3modelPOD
{
    int B3type;

    int    B3mobMod;
    int    B3capMod;
    int    B3noiMod;
    int    B3binUnit;
    int    B3paramChk;
    double B3version;
    double B3tox;
    double B3toxm;
    double B3cdsc;
    double B3cdscb;
    double B3cdscd;
    double B3cit;
    double B3nfactor;
    double B3xj;
    double B3vsat;
    double B3at;
    double B3a0;   
    double B3ags;      
    double B3a1;         
    double B3a2;         
    double B3keta;     
    double B3nsub;
    double B3npeak;        
    double B3ngate;        
    double B3gamma1;      
    double B3gamma2;     
    double B3vbx;      
    double B3vbm;       
    double B3xt;       
    double B3k1;
    double B3kt1;
    double B3kt1l;
    double B3kt2;
    double B3k2;
    double B3k3;
    double B3k3b;
    double B3w0;
    double B3nlx;
    double B3dvt0;      
    double B3dvt1;      
    double B3dvt2;      
    double B3dvt0w;      
    double B3dvt1w;      
    double B3dvt2w;      
    double B3drout;      
    double B3dsub;      
    double B3vth0;
    double B3ua;
    double B3ua1;
    double B3ub;
    double B3ub1;
    double B3uc;
    double B3uc1;
    double B3u0;
    double B3ute;
    double B3voff;
    double B3delta;
    double B3rdsw;       
    double B3prwg;
    double B3prwb;
    double B3prt;       
    double B3eta0;         
    double B3etab;         
    double B3pclm;      
    double B3pdibl1;      
    double B3pdibl2;      
    double B3pdiblb;
    double B3pscbe1;       
    double B3pscbe2;       
    double B3pvag;       
    double B3wr;
    double B3dwg;
    double B3dwb;
    double B3b0;
    double B3b1;
    double B3alpha0;
    double B3alpha1;
    double B3beta0;
    double B3ijth;
    double B3vfb;

// SRW
    double B3xw;
    double B3xl;

    // CV model
    double B3elm;
    double B3cgsl;
    double B3cgdl;
    double B3ckappa;
    double B3cf;
    double B3vfbcv;
    double B3clc;
    double B3cle;
    double B3dwc;
    double B3dlc;
    double B3noff;
    double B3voffcv;
    double B3acde;
    double B3moin;
    double B3tcj;
    double B3tcjsw;
    double B3tcjswg;
    double B3tpb;
    double B3tpbsw;
    double B3tpbswg;

    // Length Dependence
    double B3lcdsc;           
    double B3lcdscb; 
    double B3lcdscd;          
    double B3lcit;           
    double B3lnfactor;      
    double B3lxj;
    double B3lvsat;         
    double B3lat;         
    double B3la0;   
    double B3lags;      
    double B3la1;         
    double B3la2;         
    double B3lketa;     
    double B3lnsub;
    double B3lnpeak;        
    double B3lngate;        
    double B3lgamma1;      
    double B3lgamma2;     
    double B3lvbx;      
    double B3lvbm;       
    double B3lxt;       
    double B3lk1;
    double B3lkt1;
    double B3lkt1l;
    double B3lkt2;
    double B3lk2;
    double B3lk3;
    double B3lk3b;
    double B3lw0;
    double B3lnlx;
    double B3ldvt0;      
    double B3ldvt1;      
    double B3ldvt2;      
    double B3ldvt0w;      
    double B3ldvt1w;      
    double B3ldvt2w;      
    double B3ldrout;      
    double B3ldsub;      
    double B3lvth0;
    double B3lua;
    double B3lua1;
    double B3lub;
    double B3lub1;
    double B3luc;
    double B3luc1;
    double B3lu0;
    double B3lute;
    double B3lvoff;
    double B3ldelta;
    double B3lrdsw;       
    double B3lprwg;
    double B3lprwb;
    double B3lprt;       
    double B3leta0;         
    double B3letab;         
    double B3lpclm;      
    double B3lpdibl1;      
    double B3lpdibl2;      
    double B3lpdiblb;
    double B3lpscbe1;       
    double B3lpscbe2;       
    double B3lpvag;       
    double B3lwr;
    double B3ldwg;
    double B3ldwb;
    double B3lb0;
    double B3lb1;
    double B3lalpha0;
    double B3lalpha1;
    double B3lbeta0;
    double B3lvfb;

    // CV model
    double B3lelm;
    double B3lcgsl;
    double B3lcgdl;
    double B3lckappa;
    double B3lcf;
    double B3lclc;
    double B3lcle;
    double B3lvfbcv;
    double B3lnoff;
    double B3lvoffcv;
    double B3lacde;
    double B3lmoin;

    // Width Dependence
    double B3wcdsc;           
    double B3wcdscb; 
    double B3wcdscd;          
    double B3wcit;           
    double B3wnfactor;      
    double B3wxj;
    double B3wvsat;         
    double B3wat;         
    double B3wa0;   
    double B3wags;      
    double B3wa1;         
    double B3wa2;         
    double B3wketa;     
    double B3wnsub;
    double B3wnpeak;        
    double B3wngate;        
    double B3wgamma1;      
    double B3wgamma2;     
    double B3wvbx;      
    double B3wvbm;       
    double B3wxt;       
    double B3wk1;
    double B3wkt1;
    double B3wkt1l;
    double B3wkt2;
    double B3wk2;
    double B3wk3;
    double B3wk3b;
    double B3ww0;
    double B3wnlx;
    double B3wdvt0;      
    double B3wdvt1;      
    double B3wdvt2;      
    double B3wdvt0w;      
    double B3wdvt1w;      
    double B3wdvt2w;      
    double B3wdrout;      
    double B3wdsub;      
    double B3wvth0;
    double B3wua;
    double B3wua1;
    double B3wub;
    double B3wub1;
    double B3wuc;
    double B3wuc1;
    double B3wu0;
    double B3wute;
    double B3wvoff;
    double B3wdelta;
    double B3wrdsw;       
    double B3wprwg;
    double B3wprwb;
    double B3wprt;       
    double B3weta0;         
    double B3wetab;         
    double B3wpclm;      
    double B3wpdibl1;      
    double B3wpdibl2;      
    double B3wpdiblb;
    double B3wpscbe1;       
    double B3wpscbe2;       
    double B3wpvag;       
    double B3wwr;
    double B3wdwg;
    double B3wdwb;
    double B3wb0;
    double B3wb1;
    double B3walpha0;
    double B3walpha1;
    double B3wbeta0;
    double B3wvfb;

    // CV model
    double B3welm;
    double B3wcgsl;
    double B3wcgdl;
    double B3wckappa;
    double B3wcf;
    double B3wclc;
    double B3wcle;
    double B3wvfbcv;
    double B3wnoff;
    double B3wvoffcv;
    double B3wacde;
    double B3wmoin;

    // Cross-term Dependence
    double B3pcdsc;           
    double B3pcdscb; 
    double B3pcdscd;          
    double B3pcit;           
    double B3pnfactor;      
    double B3pxj;
    double B3pvsat;         
    double B3pat;         
    double B3pa0;   
    double B3pags;      
    double B3pa1;         
    double B3pa2;         
    double B3pketa;     
    double B3pnsub;
    double B3pnpeak;        
    double B3pngate;        
    double B3pgamma1;      
    double B3pgamma2;     
    double B3pvbx;      
    double B3pvbm;       
    double B3pxt;       
    double B3pk1;
    double B3pkt1;
    double B3pkt1l;
    double B3pkt2;
    double B3pk2;
    double B3pk3;
    double B3pk3b;
    double B3pw0;
    double B3pnlx;
    double B3pdvt0;      
    double B3pdvt1;      
    double B3pdvt2;      
    double B3pdvt0w;      
    double B3pdvt1w;      
    double B3pdvt2w;      
    double B3pdrout;      
    double B3pdsub;      
    double B3pvth0;
    double B3pua;
    double B3pua1;
    double B3pub;
    double B3pub1;
    double B3puc;
    double B3puc1;
    double B3pu0;
    double B3pute;
    double B3pvoff;
    double B3pdelta;
    double B3prdsw;
    double B3pprwg;
    double B3pprwb;
    double B3pprt;       
    double B3peta0;         
    double B3petab;         
    double B3ppclm;      
    double B3ppdibl1;      
    double B3ppdibl2;      
    double B3ppdiblb;
    double B3ppscbe1;       
    double B3ppscbe2;       
    double B3ppvag;       
    double B3pwr;
    double B3pdwg;
    double B3pdwb;
    double B3pb0;
    double B3pb1;
    double B3palpha0;
    double B3palpha1;
    double B3pbeta0;
    double B3pvfb;

    // CV model
    double B3pelm;
    double B3pcgsl;
    double B3pcgdl;
    double B3pckappa;
    double B3pcf;
    double B3pclc;
    double B3pcle;
    double B3pvfbcv;
    double B3pnoff;
    double B3pvoffcv;
    double B3pacde;
    double B3pmoin;

    double B3tnom;
    double B3cgso;
    double B3cgdo;
    double B3cgbo;
    double B3xpart;
    double B3cFringOut;
    double B3cFringMax;

    double B3sheetResistance;
    double B3jctSatCurDensity;
    double B3jctSidewallSatCurDensity;
    double B3bulkJctPotential;
    double B3bulkJctBotGradingCoeff;
    double B3bulkJctSideGradingCoeff;
    double B3bulkJctGateSideGradingCoeff;
    double B3sidewallJctPotential;
    double B3GatesidewallJctPotential;
    double B3unitAreaJctCap;
    double B3unitLengthSidewallJctCap;
    double B3unitLengthGateSidewallJctCap;
    double B3jctEmissionCoeff;
    double B3jctTempExponent;

    double B3Lint;
    double B3Ll;
    double B3Llc;
    double B3Lln;
    double B3Lw;
    double B3Lwc;
    double B3Lwn;
    double B3Lwl;
    double B3Lwlc;
    double B3Lmin;
    double B3Lmax;

    double B3Wint;
    double B3Wl;
    double B3Wlc;
    double B3Wln;
    double B3Ww;
    double B3Wwn;
    double B3Wwc;
    double B3Wwl;
    double B3Wwlc;
    double B3Wmin;
    double B3Wmax;

    // Pre-calculated constants

    // MCJ: move to size-dependent param.
    double B3vtm;   
    double B3cox;
    double B3cof1;
    double B3cof2;
    double B3cof3;
    double B3cof4;
    double B3vcrit;
    double B3factor1;
    double B3PhiB;
    double B3PhiBSW;
    double B3PhiBSWG;
    double B3jctTempSatCurDensity;
    double B3jctSidewallTempSatCurDensity;

    double B3oxideTrapDensityA;      
    double B3oxideTrapDensityB;     
    double B3oxideTrapDensityC;  
    double B3em;  
    double B3ef;  
    double B3af;  
    double B3kf;  

    struct bsim3SizeDependParam *pSizeDependParamKnot;

// SRW
    int B3nqsMod;

    // Flags
    unsigned  B3mobModGiven :1;
    unsigned  B3binUnitGiven :1;
    unsigned  B3capModGiven :1;
    unsigned  B3paramChkGiven :1;
    unsigned  B3noiModGiven :1;
    unsigned  B3typeGiven   :1;
    unsigned  B3toxGiven   :1;
    unsigned  B3versionGiven   :1;
    unsigned  B3toxmGiven   :1;
    unsigned  B3cdscGiven   :1;
    unsigned  B3cdscbGiven   :1;
    unsigned  B3cdscdGiven   :1;
    unsigned  B3citGiven   :1;
    unsigned  B3nfactorGiven   :1;
    unsigned  B3xjGiven   :1;
    unsigned  B3vsatGiven   :1;
    unsigned  B3atGiven   :1;
    unsigned  B3a0Given   :1;
    unsigned  B3agsGiven   :1;
    unsigned  B3a1Given   :1;
    unsigned  B3a2Given   :1;
    unsigned  B3ketaGiven   :1;    
    unsigned  B3nsubGiven   :1;
    unsigned  B3npeakGiven   :1;
    unsigned  B3ngateGiven   :1;
    unsigned  B3gamma1Given   :1;
    unsigned  B3gamma2Given   :1;
    unsigned  B3vbxGiven   :1;
    unsigned  B3vbmGiven   :1;
    unsigned  B3xtGiven   :1;
    unsigned  B3k1Given   :1;
    unsigned  B3kt1Given   :1;
    unsigned  B3kt1lGiven   :1;
    unsigned  B3kt2Given   :1;
    unsigned  B3k2Given   :1;
    unsigned  B3k3Given   :1;
    unsigned  B3k3bGiven   :1;
    unsigned  B3w0Given   :1;
    unsigned  B3nlxGiven   :1;
    unsigned  B3dvt0Given   :1;   
    unsigned  B3dvt1Given   :1;     
    unsigned  B3dvt2Given   :1;     
    unsigned  B3dvt0wGiven   :1;   
    unsigned  B3dvt1wGiven   :1;     
    unsigned  B3dvt2wGiven   :1;     
    unsigned  B3droutGiven   :1;     
    unsigned  B3dsubGiven   :1;     
    unsigned  B3vth0Given   :1;
    unsigned  B3uaGiven   :1;
    unsigned  B3ua1Given   :1;
    unsigned  B3ubGiven   :1;
    unsigned  B3ub1Given   :1;
    unsigned  B3ucGiven   :1;
    unsigned  B3uc1Given   :1;
    unsigned  B3u0Given   :1;
    unsigned  B3uteGiven   :1;
    unsigned  B3voffGiven   :1;
    unsigned  B3rdswGiven   :1;      
    unsigned  B3prwgGiven   :1;      
    unsigned  B3prwbGiven   :1;      
    unsigned  B3prtGiven   :1;      
    unsigned  B3eta0Given   :1;    
    unsigned  B3etabGiven   :1;    
    unsigned  B3pclmGiven   :1;   
    unsigned  B3pdibl1Given   :1;   
    unsigned  B3pdibl2Given   :1;  
    unsigned  B3pdiblbGiven   :1;  
    unsigned  B3pscbe1Given   :1;    
    unsigned  B3pscbe2Given   :1;    
    unsigned  B3pvagGiven   :1;    
    unsigned  B3deltaGiven  :1;     
    unsigned  B3wrGiven   :1;
    unsigned  B3dwgGiven   :1;
    unsigned  B3dwbGiven   :1;
    unsigned  B3b0Given   :1;
    unsigned  B3b1Given   :1;
    unsigned  B3alpha0Given   :1;
    unsigned  B3alpha1Given   :1;
    unsigned  B3beta0Given   :1;
    unsigned  B3ijthGiven   :1;
    unsigned  B3vfbGiven   :1;

    // CV model
    unsigned  B3elmGiven  :1;     
    unsigned  B3cgslGiven   :1;
    unsigned  B3cgdlGiven   :1;
    unsigned  B3ckappaGiven   :1;
    unsigned  B3cfGiven   :1;
    unsigned  B3vfbcvGiven   :1;
    unsigned  B3clcGiven   :1;
    unsigned  B3cleGiven   :1;
    unsigned  B3dwcGiven   :1;
    unsigned  B3dlcGiven   :1;
    unsigned  B3noffGiven  :1;
    unsigned  B3voffcvGiven :1;
    unsigned  B3acdeGiven  :1;
    unsigned  B3moinGiven  :1;
    unsigned  B3tcjGiven   :1;
    unsigned  B3tcjswGiven :1;
    unsigned  B3tcjswgGiven :1;
    unsigned  B3tpbGiven    :1;
    unsigned  B3tpbswGiven  :1;
    unsigned  B3tpbswgGiven :1;

    // Length dependence
    unsigned  B3lcdscGiven   :1;
    unsigned  B3lcdscbGiven   :1;
    unsigned  B3lcdscdGiven   :1;
    unsigned  B3lcitGiven   :1;
    unsigned  B3lnfactorGiven   :1;
    unsigned  B3lxjGiven   :1;
    unsigned  B3lvsatGiven   :1;
    unsigned  B3latGiven   :1;
    unsigned  B3la0Given   :1;
    unsigned  B3lagsGiven   :1;
    unsigned  B3la1Given   :1;
    unsigned  B3la2Given   :1;
    unsigned  B3lketaGiven   :1;    
    unsigned  B3lnsubGiven   :1;
    unsigned  B3lnpeakGiven   :1;
    unsigned  B3lngateGiven   :1;
    unsigned  B3lgamma1Given   :1;
    unsigned  B3lgamma2Given   :1;
    unsigned  B3lvbxGiven   :1;
    unsigned  B3lvbmGiven   :1;
    unsigned  B3lxtGiven   :1;
    unsigned  B3lk1Given   :1;
    unsigned  B3lkt1Given   :1;
    unsigned  B3lkt1lGiven   :1;
    unsigned  B3lkt2Given   :1;
    unsigned  B3lk2Given   :1;
    unsigned  B3lk3Given   :1;
    unsigned  B3lk3bGiven   :1;
    unsigned  B3lw0Given   :1;
    unsigned  B3lnlxGiven   :1;
    unsigned  B3ldvt0Given   :1;   
    unsigned  B3ldvt1Given   :1;     
    unsigned  B3ldvt2Given   :1;     
    unsigned  B3ldvt0wGiven   :1;   
    unsigned  B3ldvt1wGiven   :1;     
    unsigned  B3ldvt2wGiven   :1;     
    unsigned  B3ldroutGiven   :1;     
    unsigned  B3ldsubGiven   :1;     
    unsigned  B3lvth0Given   :1;
    unsigned  B3luaGiven   :1;
    unsigned  B3lua1Given   :1;
    unsigned  B3lubGiven   :1;
    unsigned  B3lub1Given   :1;
    unsigned  B3lucGiven   :1;
    unsigned  B3luc1Given   :1;
    unsigned  B3lu0Given   :1;
    unsigned  B3luteGiven   :1;
    unsigned  B3lvoffGiven   :1;
    unsigned  B3lrdswGiven   :1;      
    unsigned  B3lprwgGiven   :1;      
    unsigned  B3lprwbGiven   :1;      
    unsigned  B3lprtGiven   :1;      
    unsigned  B3leta0Given   :1;    
    unsigned  B3letabGiven   :1;    
    unsigned  B3lpclmGiven   :1;   
    unsigned  B3lpdibl1Given   :1;   
    unsigned  B3lpdibl2Given   :1;  
    unsigned  B3lpdiblbGiven   :1;  
    unsigned  B3lpscbe1Given   :1;    
    unsigned  B3lpscbe2Given   :1;    
    unsigned  B3lpvagGiven   :1;    
    unsigned  B3ldeltaGiven  :1;     
    unsigned  B3lwrGiven   :1;
    unsigned  B3ldwgGiven   :1;
    unsigned  B3ldwbGiven   :1;
    unsigned  B3lb0Given   :1;
    unsigned  B3lb1Given   :1;
    unsigned  B3lalpha0Given   :1;
    unsigned  B3lalpha1Given   :1;
    unsigned  B3lbeta0Given   :1;
    unsigned  B3lvfbGiven   :1;

    // CV model
    unsigned  B3lelmGiven  :1;     
    unsigned  B3lcgslGiven   :1;
    unsigned  B3lcgdlGiven   :1;
    unsigned  B3lckappaGiven   :1;
    unsigned  B3lcfGiven   :1;
    unsigned  B3lclcGiven   :1;
    unsigned  B3lcleGiven   :1;
    unsigned  B3lvfbcvGiven   :1;
    unsigned  B3lnoffGiven   :1;
    unsigned  B3lvoffcvGiven :1;
    unsigned  B3lacdeGiven   :1;
    unsigned  B3lmoinGiven   :1;

    // Width dependence
    unsigned  B3wcdscGiven   :1;
    unsigned  B3wcdscbGiven   :1;
    unsigned  B3wcdscdGiven   :1;
    unsigned  B3wcitGiven   :1;
    unsigned  B3wnfactorGiven   :1;
    unsigned  B3wxjGiven   :1;
    unsigned  B3wvsatGiven   :1;
    unsigned  B3watGiven   :1;
    unsigned  B3wa0Given   :1;
    unsigned  B3wagsGiven   :1;
    unsigned  B3wa1Given   :1;
    unsigned  B3wa2Given   :1;
    unsigned  B3wketaGiven   :1;    
    unsigned  B3wnsubGiven   :1;
    unsigned  B3wnpeakGiven   :1;
    unsigned  B3wngateGiven   :1;
    unsigned  B3wgamma1Given   :1;
    unsigned  B3wgamma2Given   :1;
    unsigned  B3wvbxGiven   :1;
    unsigned  B3wvbmGiven   :1;
    unsigned  B3wxtGiven   :1;
    unsigned  B3wk1Given   :1;
    unsigned  B3wkt1Given   :1;
    unsigned  B3wkt1lGiven   :1;
    unsigned  B3wkt2Given   :1;
    unsigned  B3wk2Given   :1;
    unsigned  B3wk3Given   :1;
    unsigned  B3wk3bGiven   :1;
    unsigned  B3ww0Given   :1;
    unsigned  B3wnlxGiven   :1;
    unsigned  B3wdvt0Given   :1;   
    unsigned  B3wdvt1Given   :1;     
    unsigned  B3wdvt2Given   :1;     
    unsigned  B3wdvt0wGiven   :1;   
    unsigned  B3wdvt1wGiven   :1;     
    unsigned  B3wdvt2wGiven   :1;     
    unsigned  B3wdroutGiven   :1;     
    unsigned  B3wdsubGiven   :1;     
    unsigned  B3wvth0Given   :1;
    unsigned  B3wuaGiven   :1;
    unsigned  B3wua1Given   :1;
    unsigned  B3wubGiven   :1;
    unsigned  B3wub1Given   :1;
    unsigned  B3wucGiven   :1;
    unsigned  B3wuc1Given   :1;
    unsigned  B3wu0Given   :1;
    unsigned  B3wuteGiven   :1;
    unsigned  B3wvoffGiven   :1;
    unsigned  B3wrdswGiven   :1;      
    unsigned  B3wprwgGiven   :1;      
    unsigned  B3wprwbGiven   :1;      
    unsigned  B3wprtGiven   :1;      
    unsigned  B3weta0Given   :1;    
    unsigned  B3wetabGiven   :1;    
    unsigned  B3wpclmGiven   :1;   
    unsigned  B3wpdibl1Given   :1;   
    unsigned  B3wpdibl2Given   :1;  
    unsigned  B3wpdiblbGiven   :1;  
    unsigned  B3wpscbe1Given   :1;    
    unsigned  B3wpscbe2Given   :1;    
    unsigned  B3wpvagGiven   :1;    
    unsigned  B3wdeltaGiven  :1;     
    unsigned  B3wwrGiven   :1;
    unsigned  B3wdwgGiven   :1;
    unsigned  B3wdwbGiven   :1;
    unsigned  B3wb0Given   :1;
    unsigned  B3wb1Given   :1;
    unsigned  B3walpha0Given   :1;
    unsigned  B3walpha1Given   :1;
    unsigned  B3wbeta0Given   :1;
    unsigned  B3wvfbGiven   :1;

    // CV model
    unsigned  B3welmGiven  :1;     
    unsigned  B3wcgslGiven   :1;
    unsigned  B3wcgdlGiven   :1;
    unsigned  B3wckappaGiven   :1;
    unsigned  B3wcfGiven   :1;
    unsigned  B3wclcGiven   :1;
    unsigned  B3wcleGiven   :1;
    unsigned  B3wvfbcvGiven   :1;
    unsigned  B3wnoffGiven   :1;
    unsigned  B3wvoffcvGiven :1;
    unsigned  B3wacdeGiven   :1;
    unsigned  B3wmoinGiven   :1;

    // Cross-term dependence
    unsigned  B3pcdscGiven   :1;
    unsigned  B3pcdscbGiven   :1;
    unsigned  B3pcdscdGiven   :1;
    unsigned  B3pcitGiven   :1;
    unsigned  B3pnfactorGiven   :1;
    unsigned  B3pxjGiven   :1;
    unsigned  B3pvsatGiven   :1;
    unsigned  B3patGiven   :1;
    unsigned  B3pa0Given   :1;
    unsigned  B3pagsGiven   :1;
    unsigned  B3pa1Given   :1;
    unsigned  B3pa2Given   :1;
    unsigned  B3pketaGiven   :1;    
    unsigned  B3pnsubGiven   :1;
    unsigned  B3pnpeakGiven   :1;
    unsigned  B3pngateGiven   :1;
    unsigned  B3pgamma1Given   :1;
    unsigned  B3pgamma2Given   :1;
    unsigned  B3pvbxGiven   :1;
    unsigned  B3pvbmGiven   :1;
    unsigned  B3pxtGiven   :1;
    unsigned  B3pk1Given   :1;
    unsigned  B3pkt1Given   :1;
    unsigned  B3pkt1lGiven   :1;
    unsigned  B3pkt2Given   :1;
    unsigned  B3pk2Given   :1;
    unsigned  B3pk3Given   :1;
    unsigned  B3pk3bGiven   :1;
    unsigned  B3pw0Given   :1;
    unsigned  B3pnlxGiven   :1;
    unsigned  B3pdvt0Given   :1;   
    unsigned  B3pdvt1Given   :1;     
    unsigned  B3pdvt2Given   :1;     
    unsigned  B3pdvt0wGiven   :1;   
    unsigned  B3pdvt1wGiven   :1;     
    unsigned  B3pdvt2wGiven   :1;     
    unsigned  B3pdroutGiven   :1;     
    unsigned  B3pdsubGiven   :1;     
    unsigned  B3pvth0Given   :1;
    unsigned  B3puaGiven   :1;
    unsigned  B3pua1Given   :1;
    unsigned  B3pubGiven   :1;
    unsigned  B3pub1Given   :1;
    unsigned  B3pucGiven   :1;
    unsigned  B3puc1Given   :1;
    unsigned  B3pu0Given   :1;
    unsigned  B3puteGiven   :1;
    unsigned  B3pvoffGiven   :1;
    unsigned  B3prdswGiven   :1;      
    unsigned  B3pprwgGiven   :1;      
    unsigned  B3pprwbGiven   :1;      
    unsigned  B3pprtGiven   :1;      
    unsigned  B3peta0Given   :1;    
    unsigned  B3petabGiven   :1;    
    unsigned  B3ppclmGiven   :1;   
    unsigned  B3ppdibl1Given   :1;   
    unsigned  B3ppdibl2Given   :1;  
    unsigned  B3ppdiblbGiven   :1;  
    unsigned  B3ppscbe1Given   :1;    
    unsigned  B3ppscbe2Given   :1;    
    unsigned  B3ppvagGiven   :1;    
    unsigned  B3pdeltaGiven  :1;     
    unsigned  B3pwrGiven   :1;
    unsigned  B3pdwgGiven   :1;
    unsigned  B3pdwbGiven   :1;
    unsigned  B3pb0Given   :1;
    unsigned  B3pb1Given   :1;
    unsigned  B3palpha0Given   :1;
    unsigned  B3palpha1Given   :1;
    unsigned  B3pbeta0Given   :1;
    unsigned  B3pvfbGiven   :1;

    // CV model
    unsigned  B3pelmGiven  :1;     
    unsigned  B3pcgslGiven   :1;
    unsigned  B3pcgdlGiven   :1;
    unsigned  B3pckappaGiven   :1;
    unsigned  B3pcfGiven   :1;
    unsigned  B3pclcGiven   :1;
    unsigned  B3pcleGiven   :1;
    unsigned  B3pvfbcvGiven   :1;
    unsigned  B3pnoffGiven   :1;
    unsigned  B3pvoffcvGiven :1;
    unsigned  B3pacdeGiven   :1;
    unsigned  B3pmoinGiven   :1;

    unsigned  B3useFringeGiven   :1;

    unsigned  B3tnomGiven   :1;
    unsigned  B3cgsoGiven   :1;
    unsigned  B3cgdoGiven   :1;
    unsigned  B3cgboGiven   :1;
    unsigned  B3xpartGiven   :1;
    unsigned  B3sheetResistanceGiven   :1;
    unsigned  B3jctSatCurDensityGiven   :1;
    unsigned  B3jctSidewallSatCurDensityGiven   :1;
    unsigned  B3bulkJctPotentialGiven   :1;
    unsigned  B3bulkJctBotGradingCoeffGiven   :1;
    unsigned  B3sidewallJctPotentialGiven   :1;
    unsigned  B3GatesidewallJctPotentialGiven   :1;
    unsigned  B3bulkJctSideGradingCoeffGiven   :1;
    unsigned  B3unitAreaJctCapGiven   :1;
    unsigned  B3unitLengthSidewallJctCapGiven   :1;
    unsigned  B3bulkJctGateSideGradingCoeffGiven   :1;
    unsigned  B3unitLengthGateSidewallJctCapGiven   :1;
    unsigned  B3jctEmissionCoeffGiven :1;
    unsigned  B3jctTempExponentGiven	:1;

    unsigned  B3oxideTrapDensityAGiven  :1;         
    unsigned  B3oxideTrapDensityBGiven  :1;        
    unsigned  B3oxideTrapDensityCGiven  :1;     
    unsigned  B3emGiven  :1;     
    unsigned  B3efGiven  :1;     
    unsigned  B3afGiven  :1;     
    unsigned  B3kfGiven  :1;     

    unsigned  B3LintGiven   :1;
    unsigned  B3LlGiven   :1;
    unsigned  B3LlcGiven   :1;
    unsigned  B3LlnGiven   :1;
    unsigned  B3LwGiven   :1;
    unsigned  B3LwcGiven   :1;
    unsigned  B3LwnGiven   :1;
    unsigned  B3LwlGiven   :1;
    unsigned  B3LwlcGiven   :1;
    unsigned  B3LminGiven   :1;
    unsigned  B3LmaxGiven   :1;

    unsigned  B3WintGiven   :1;
    unsigned  B3WlGiven   :1;
    unsigned  B3WlcGiven   :1;
    unsigned  B3WlnGiven   :1;
    unsigned  B3WwGiven   :1;
    unsigned  B3WwcGiven   :1;
    unsigned  B3WwnGiven   :1;
    unsigned  B3WwlGiven   :1;
    unsigned  B3WwlcGiven   :1;
    unsigned  B3WminGiven   :1;
    unsigned  B3WmaxGiven   :1;

// SRW
    unsigned  B3xwGiven   :1;
    unsigned  B3xlGiven   :1;
    unsigned  B3dumpLog :1;
    unsigned  B3nqsModGiven :1;  // SRW
};

struct sB3model : sGENmodel, sB3modelPOD
{
    sB3model() : sGENmodel(), sB3modelPOD() { }

    sB3model *next()    { return (static_cast<sB3model*>(GENnextModel)); }
    sB3instance *inst() { return (static_cast<sB3instance*>(GENinstances)); }
};
} // namespace BSIM32
using namespace BSIM32;


#ifndef NMOS
#define NMOS 1
#define PMOS -1
#endif

// device parameters
enum {
    B3_W = 1,
    B3_L,
    B3_AS,
    B3_AD,
    B3_PS,
    B3_PD,
    B3_NRS,
    B3_NRD,
    B3_OFF,
    B3_IC_VBS,
    B3_IC_VDS,
    B3_IC_VGS,
    B3_IC,
    B3_NQSMOD,
    B3_M,

    B3_DNODE,
    B3_GNODE,
    B3_SNODE,
    B3_BNODE,
    B3_DNODEPRIME,
    B3_SNODEPRIME,
    B3_VBD,
    B3_VBS,
    B3_VGS,
    B3_VDS,
    B3_CD,
    B3_CS,
    B3_CG,
    B3_CB,
    B3_CBS,
    B3_CBD,
    B3_GM,
    B3_GDS,
    B3_GMBS,
    B3_GBD,
    B3_GBS,
    B3_QB,
    B3_CQB,
    B3_QG,
    B3_CQG,
    B3_QD,
    B3_CQD,
    B3_CGG,
    B3_CGD,
    B3_CGS,
    B3_CBG,
    B3_CAPBD,
    B3_CQBD,
    B3_CAPBS,
    B3_CQBS,
    B3_CDG,
    B3_CDD,
    B3_CDS,
    B3_VON,
    B3_VDSAT,
    B3_QBS,
    B3_QBD,
    B3_SOURCECOND,
    B3_DRAINCOND,
    B3_CBDB,
    B3_CBSB
};

// model parameters
enum {
    B3_MOD_CAPMOD = 1000,
    B3_MOD_NQSMOD,
    B3_MOD_MOBMOD,
    B3_MOD_NOIMOD,

    B3_MOD_TOX,

    B3_MOD_CDSC,
    B3_MOD_CDSCB,
    B3_MOD_CIT,
    B3_MOD_NFACTOR,
    B3_MOD_XJ,
    B3_MOD_VSAT,
    B3_MOD_AT,
    B3_MOD_A0,
    B3_MOD_A1,
    B3_MOD_A2,
    B3_MOD_KETA,
    B3_MOD_NSUB,
    B3_MOD_NPEAK,
    B3_MOD_NGATE,
    B3_MOD_GAMMA1,
    B3_MOD_GAMMA2,
    B3_MOD_VBX,
    B3_MOD_BINUNIT,

    B3_MOD_VBM,

    B3_MOD_XT,
    B3_MOD_K1,
    B3_MOD_KT1,
    B3_MOD_KT1L,
    B3_MOD_K2,
    B3_MOD_KT2,
    B3_MOD_K3,
    B3_MOD_K3B,
    B3_MOD_W0,
    B3_MOD_NLX,

    B3_MOD_DVT0,
    B3_MOD_DVT1,
    B3_MOD_DVT2,

    B3_MOD_DVT0W,
    B3_MOD_DVT1W,
    B3_MOD_DVT2W,

    B3_MOD_DROUT,
    B3_MOD_DSUB,
    B3_MOD_VTH0,
    B3_MOD_UA,
    B3_MOD_UA1,
    B3_MOD_UB,
    B3_MOD_UB1,
    B3_MOD_UC,
    B3_MOD_UC1,
    B3_MOD_U0,
    B3_MOD_UTE,
    B3_MOD_VOFF,
    B3_MOD_DELTA,
    B3_MOD_RDSW,
    B3_MOD_PRT,
    B3_MOD_LDD,
    B3_MOD_ETA,
    B3_MOD_ETA0,
    B3_MOD_ETAB,
    B3_MOD_PCLM,
    B3_MOD_PDIBL1,
    B3_MOD_PDIBL2,
    B3_MOD_PSCBE1,
    B3_MOD_PSCBE2,
    B3_MOD_PVAG,
    B3_MOD_WR,
    B3_MOD_DWG,
    B3_MOD_DWB,
    B3_MOD_B0,
    B3_MOD_B1,
    B3_MOD_ALPHA0,
    B3_MOD_BETA0,
    B3_MOD_PDIBLB,

    B3_MOD_PRWG,
    B3_MOD_PRWB,

    B3_MOD_CDSCD,
    B3_MOD_AGS,

    B3_MOD_FRINGE,
    B3_MOD_ELM,
    B3_MOD_CGSL,
    B3_MOD_CGDL,
    B3_MOD_CKAPPA,
    B3_MOD_CF,
    B3_MOD_CLC,
    B3_MOD_CLE,
    B3_MOD_PARAMCHK,
    B3_MOD_VERSION,
    B3_MOD_VFBCV,
    B3_MOD_ACDE,
    B3_MOD_MOIN,
    B3_MOD_NOFF,
    B3_MOD_IJTH,
    B3_MOD_ALPHA1,
    B3_MOD_VFB,
    B3_MOD_TOXM,
    B3_MOD_TCJ,
    B3_MOD_TCJSW,
    B3_MOD_TCJSWG,
    B3_MOD_TPB,
    B3_MOD_TPBSW,
    B3_MOD_TPBSWG,
    B3_MOD_VOFFCV,

    B3_MOD_DMPLOG,

    // Length dependence
    B3_MOD_LCDSC,
    B3_MOD_LCDSCB,
    B3_MOD_LCIT,
    B3_MOD_LNFACTOR,
    B3_MOD_LXJ,
    B3_MOD_LVSAT,
    B3_MOD_LAT,
    B3_MOD_LA0,
    B3_MOD_LA1,
    B3_MOD_LA2,
    B3_MOD_LKETA,
    B3_MOD_LNSUB,
    B3_MOD_LNPEAK,
    B3_MOD_LNGATE,
    B3_MOD_LGAMMA1,
    B3_MOD_LGAMMA2,
    B3_MOD_LVBX,

    B3_MOD_LVBM,

    B3_MOD_LXT,
    B3_MOD_LK1,
    B3_MOD_LKT1,
    B3_MOD_LKT1L,
    B3_MOD_LK2,
    B3_MOD_LKT2,
    B3_MOD_LK3,
    B3_MOD_LK3B,
    B3_MOD_LW0,
    B3_MOD_LNLX,

    B3_MOD_LDVT0,
    B3_MOD_LDVT1,
    B3_MOD_LDVT2,

    B3_MOD_LDVT0W,
    B3_MOD_LDVT1W,
    B3_MOD_LDVT2W,

    B3_MOD_LDROUT,
    B3_MOD_LDSUB,
    B3_MOD_LVTH0,
    B3_MOD_LUA,
    B3_MOD_LUA1,
    B3_MOD_LUB,
    B3_MOD_LUB1,
    B3_MOD_LUC,
    B3_MOD_LUC1,
    B3_MOD_LU0,
    B3_MOD_LUTE,
    B3_MOD_LVOFF,
    B3_MOD_LDELTA,
    B3_MOD_LRDSW,
    B3_MOD_LPRT,
    B3_MOD_LLDD,
    B3_MOD_LETA,
    B3_MOD_LETA0,
    B3_MOD_LETAB,
    B3_MOD_LPCLM,
    B3_MOD_LPDIBL1,
    B3_MOD_LPDIBL2,
    B3_MOD_LPSCBE1,
    B3_MOD_LPSCBE2,
    B3_MOD_LPVAG,
    B3_MOD_LWR,
    B3_MOD_LDWG,
    B3_MOD_LDWB,
    B3_MOD_LB0,
    B3_MOD_LB1,
    B3_MOD_LALPHA0,
    B3_MOD_LBETA0,
    B3_MOD_LPDIBLB,

    B3_MOD_LPRWG,
    B3_MOD_LPRWB,

    B3_MOD_LCDSCD,
    B3_MOD_LAGS,

    B3_MOD_LFRINGE,
    B3_MOD_LELM,
    B3_MOD_LCGSL,
    B3_MOD_LCGDL,
    B3_MOD_LCKAPPA,
    B3_MOD_LCF,
    B3_MOD_LCLC,
    B3_MOD_LCLE,
    B3_MOD_LVFBCV,
    B3_MOD_LACDE,
    B3_MOD_LMOIN,
    B3_MOD_LNOFF,
    B3_MOD_LALPHA1,
    B3_MOD_LVFB,
    B3_MOD_LVOFFCV,

    // Width dependence
    B3_MOD_WCDSC,
    B3_MOD_WCDSCB,
    B3_MOD_WCIT,
    B3_MOD_WNFACTOR,
    B3_MOD_WXJ,
    B3_MOD_WVSAT,
    B3_MOD_WAT,
    B3_MOD_WA0,
    B3_MOD_WA1,
    B3_MOD_WA2,
    B3_MOD_WKETA,
    B3_MOD_WNSUB,
    B3_MOD_WNPEAK,
    B3_MOD_WNGATE,
    B3_MOD_WGAMMA1,
    B3_MOD_WGAMMA2,
    B3_MOD_WVBX,

    B3_MOD_WVBM,

    B3_MOD_WXT,
    B3_MOD_WK1,
    B3_MOD_WKT1,
    B3_MOD_WKT1L,
    B3_MOD_WK2,
    B3_MOD_WKT2,
    B3_MOD_WK3,
    B3_MOD_WK3B,
    B3_MOD_WW0,
    B3_MOD_WNLX,

    B3_MOD_WDVT0,
    B3_MOD_WDVT1,
    B3_MOD_WDVT2,

    B3_MOD_WDVT0W,
    B3_MOD_WDVT1W,
    B3_MOD_WDVT2W,

    B3_MOD_WDROUT,
    B3_MOD_WDSUB,
    B3_MOD_WVTH0,
    B3_MOD_WUA,
    B3_MOD_WUA1,
    B3_MOD_WUB,
    B3_MOD_WUB1,
    B3_MOD_WUC,
    B3_MOD_WUC1,
    B3_MOD_WU0,
    B3_MOD_WUTE,
    B3_MOD_WVOFF,
    B3_MOD_WDELTA,
    B3_MOD_WRDSW,
    B3_MOD_WPRT,
    B3_MOD_WLDD,
    B3_MOD_WETA,
    B3_MOD_WETA0,
    B3_MOD_WETAB,
    B3_MOD_WPCLM,
    B3_MOD_WPDIBL1,
    B3_MOD_WPDIBL2,
    B3_MOD_WPSCBE1,
    B3_MOD_WPSCBE2,
    B3_MOD_WPVAG,
    B3_MOD_WWR,
    B3_MOD_WDWG,
    B3_MOD_WDWB,
    B3_MOD_WB0,
    B3_MOD_WB1,
    B3_MOD_WALPHA0,
    B3_MOD_WBETA0,
    B3_MOD_WPDIBLB,

    B3_MOD_WPRWG,
    B3_MOD_WPRWB,

    B3_MOD_WCDSCD,
    B3_MOD_WAGS,

    B3_MOD_WFRINGE,
    B3_MOD_WELM,
    B3_MOD_WCGSL,
    B3_MOD_WCGDL,
    B3_MOD_WCKAPPA,
    B3_MOD_WCF,
    B3_MOD_WCLC,
    B3_MOD_WCLE,
    B3_MOD_WVFBCV,
    B3_MOD_WACDE,
    B3_MOD_WMOIN,
    B3_MOD_WNOFF,
    B3_MOD_WALPHA1,
    B3_MOD_WVFB,
    B3_MOD_WVOFFCV,

    // Cross-term dependence
    B3_MOD_PCDSC,
    B3_MOD_PCDSCB,
    B3_MOD_PCIT,
    B3_MOD_PNFACTOR,
    B3_MOD_PXJ,
    B3_MOD_PVSAT,
    B3_MOD_PAT,
    B3_MOD_PA0,
    B3_MOD_PA1,
    B3_MOD_PA2,
    B3_MOD_PKETA,
    B3_MOD_PNSUB,
    B3_MOD_PNPEAK,
    B3_MOD_PNGATE,
    B3_MOD_PGAMMA1,
    B3_MOD_PGAMMA2,
    B3_MOD_PVBX,

    B3_MOD_PVBM,

    B3_MOD_PXT,
    B3_MOD_PK1,
    B3_MOD_PKT1,
    B3_MOD_PKT1L,
    B3_MOD_PK2,
    B3_MOD_PKT2,
    B3_MOD_PK3,
    B3_MOD_PK3B,
    B3_MOD_PW0,
    B3_MOD_PNLX,

    B3_MOD_PDVT0,
    B3_MOD_PDVT1,
    B3_MOD_PDVT2,

    B3_MOD_PDVT0W,
    B3_MOD_PDVT1W,
    B3_MOD_PDVT2W,

    B3_MOD_PDROUT,
    B3_MOD_PDSUB,
    B3_MOD_PVTH0,
    B3_MOD_PUA,
    B3_MOD_PUA1,
    B3_MOD_PUB,
    B3_MOD_PUB1,
    B3_MOD_PUC,
    B3_MOD_PUC1,
    B3_MOD_PU0,
    B3_MOD_PUTE,
    B3_MOD_PVOFF,
    B3_MOD_PDELTA,
    B3_MOD_PRDSW,
    B3_MOD_PPRT,
    B3_MOD_PLDD,
    B3_MOD_PETA,
    B3_MOD_PETA0,
    B3_MOD_PETAB,
    B3_MOD_PPCLM,
    B3_MOD_PPDIBL1,
    B3_MOD_PPDIBL2,
    B3_MOD_PPSCBE1,
    B3_MOD_PPSCBE2,
    B3_MOD_PPVAG,
    B3_MOD_PWR,
    B3_MOD_PDWG,
    B3_MOD_PDWB,
    B3_MOD_PB0,
    B3_MOD_PB1,
    B3_MOD_PALPHA0,
    B3_MOD_PBETA0,
    B3_MOD_PPDIBLB,

    B3_MOD_PPRWG,
    B3_MOD_PPRWB,

    B3_MOD_PCDSCD,
    B3_MOD_PAGS,

    B3_MOD_PFRINGE,
    B3_MOD_PELM,
    B3_MOD_PCGSL,
    B3_MOD_PCGDL,
    B3_MOD_PCKAPPA,
    B3_MOD_PCF,
    B3_MOD_PCLC,
    B3_MOD_PCLE,
    B3_MOD_PVFBCV,
    B3_MOD_PACDE,
    B3_MOD_PMOIN,
    B3_MOD_PNOFF,
    B3_MOD_PALPHA1,
    B3_MOD_PVFB,
    B3_MOD_PVOFFCV,

    B3_MOD_TNOM,
    B3_MOD_CGSO,
    B3_MOD_CGDO,
    B3_MOD_CGBO,
    B3_MOD_XPART,

    B3_MOD_RSH,
    B3_MOD_JS,
    B3_MOD_PB,
    B3_MOD_MJ,
    B3_MOD_PBSW,
    B3_MOD_MJSW,
    B3_MOD_CJ,
    B3_MOD_CJSW,
    B3_MOD_NMOS,
    B3_MOD_PMOS,

    B3_MOD_NOIA,
    B3_MOD_NOIB,
    B3_MOD_NOIC,

    B3_MOD_LINT,
    B3_MOD_LL,
    B3_MOD_LLN,
    B3_MOD_LW,
    B3_MOD_LWN,
    B3_MOD_LWL,
    B3_MOD_LMIN,
    B3_MOD_LMAX,

    B3_MOD_WINT,
    B3_MOD_WL,
    B3_MOD_WLN,
    B3_MOD_WW,
    B3_MOD_WWN,
    B3_MOD_WWL,
    B3_MOD_WMIN,
    B3_MOD_WMAX,

    B3_MOD_DWC,
    B3_MOD_DLC,

    B3_MOD_EM,
    B3_MOD_EF,
    B3_MOD_AF,
    B3_MOD_KF,

    B3_MOD_NJ,
    B3_MOD_XTI,

    B3_MOD_PBSWG,
    B3_MOD_MJSWG,
    B3_MOD_CJSWG,
    B3_MOD_JSW,

    B3_MOD_LLC,
    B3_MOD_LWC,
    B3_MOD_LWLC,

    B3_MOD_WLC,
    B3_MOD_WWC,
    B3_MOD_WWLC,

    // SRW
    B3_MOD_XW,
    B3_MOD_XL
};

#endif // B3DEFS_H

