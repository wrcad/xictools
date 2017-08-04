
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
Author: 1998 Samuel Fung
File: b3soidef.h
Modified by Pin Su and Jan Feng	99/2/15
Modified by Pin Su 99/4/30
Modified by Pin Su and Wei Jin 99/9/27
Modified by Pin Su 00/3/1
Modified by Pin Su 01/2/15
Modified by Pin Su and Hui Wan 02/3/5
Modified by Pin Su 02/5/20
Modified by Pin Su and Hui Wan 02/11/12
Modified by Pin Su and Hui Wan 03/07/30 version 3.2 beta
**********/

#ifndef B3SDEFS_H
#define B3SDEFS_H

#include "device.h"

#define SOICODE
//  #define BULKCODE
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

namespace B3SOI32 {

struct sB3SOImodel;
struct sB3SOIinstance;

struct B3SOIadj
{
    B3SOIadj();
    ~B3SOIadj();

/* v3.1 wanh added for RF */
    double *B3SOIGgmPtr;
    double *B3SOIGgePtr;

    double *B3SOIGMdpPtr;
    double *B3SOIGMgPtr;
    double *B3SOIGMspPtr;
    double *B3SOIGMgmPtr;
    double *B3SOIGMgePtr;
    double *B3SOIGMePtr;
    double *B3SOIGMbPtr;

    double *B3SOISPgmPtr;
    double *B3SOIDPgmPtr;
    double *B3SOIEgmPtr;

    double *B3SOIGEdpPtr;
    double *B3SOIGEgPtr;
    double *B3SOIGEgmPtr;
    double *B3SOIGEgePtr;
    double *B3SOIGEspPtr;
    double *B3SOIGEbPtr;
/* v3.1 wanh added for RF end */

    double *B3SOIGePtr;
    double *B3SOIDPePtr;
    double *B3SOISPePtr;

    double *B3SOIEePtr;
    double *B3SOIEbPtr;
    double *B3SOIBePtr;
    double *B3SOIEgPtr;
    double *B3SOIEdpPtr;
    double *B3SOIEspPtr;
    double *B3SOITemptempPtr;
    double *B3SOITempdpPtr;
    double *B3SOITempspPtr;
    double *B3SOITempgPtr;
    double *B3SOITempbPtr;
    double *B3SOITempePtr;   /* v3.0 */
    double *B3SOIGtempPtr;
    double *B3SOIDPtempPtr;
    double *B3SOISPtempPtr;
    double *B3SOIEtempPtr;
    double *B3SOIBtempPtr;
    double *B3SOIPtempPtr;
    double *B3SOIBpPtr;
    double *B3SOIPbPtr;
    double *B3SOIPpPtr;
    double *B3SOIDdPtr;
    double *B3SOIGgPtr;
    double *B3SOISsPtr;
    double *B3SOIBbPtr;
    double *B3SOIDPdpPtr;
    double *B3SOISPspPtr;
    double *B3SOIDdpPtr;
    double *B3SOIGbPtr;
    double *B3SOIGdpPtr;
    double *B3SOIGspPtr;
    double *B3SOISspPtr;
    double *B3SOIBdpPtr;
    double *B3SOIBspPtr;
    double *B3SOIDPspPtr;
    double *B3SOIDPdPtr;
    double *B3SOIBgPtr;
    double *B3SOIDPgPtr;
    double *B3SOISPgPtr;
    double *B3SOISPsPtr;
    double *B3SOIDPbPtr;
    double *B3SOISPbPtr;
    double *B3SOISPdpPtr;

    double *B3SOIVbsPtr;

    dvaMatrix *matrix;
};

struct B3SOIdev : public IFdevice
{
    B3SOIdev();
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
    int checkModel(sB3SOImodel*, sB3SOIinstance*, sCKT*);
};

struct sB3SOIinstance : public sGENinstance
{
    sB3SOIinstance()
        {
            memset(this, 0, sizeof(sB3SOIinstance));
            GENnumNodes = 7;
        }
    ~sB3SOIinstance()
        {
            delete B3SOIadjoint;
            delete [] (char*)B3SOIbacking;
        }
    sB3SOIinstance *next()
        { return (static_cast<sB3SOIinstance*>(GENnextInstance)); }

    int B3SOIdNode;
/* v3.1 wanh changed gNode to gNodeExt */
    int B3SOIgNodeExt;
/* v3.1 wanh changed gNode to gNodeExt end */
    int B3SOIsNode;
    int B3SOIeNode;
    int B3SOIpNode;
    int B3SOIbNode;
    int B3SOItempNode;
    int B3SOIdNodePrime;
    int B3SOIsNodePrime;
/* v3.1 wanh added for RF */
    int B3SOIgNode;
    int B3SOIgNodeMid;
/* v3.1 wanh added for RF end */

    int B3SOIvbsNode;
    /* for Debug */
    int B3SOIidsNode;
    int B3SOIicNode;
    int B3SOIibsNode;
    int B3SOIibdNode;
    int B3SOIiiiNode;
    int B3SOIigNode;
    int B3SOIgiggNode;
    int B3SOIgigdNode;
    int B3SOIgigbNode;
    int B3SOIigidlNode;
    int B3SOIitunNode;
    int B3SOIibpNode;
    int B3SOIcbbNode;
    int B3SOIcbdNode;
    int B3SOIcbgNode;

    int B3SOIqbfNode;
    int B3SOIqjsNode;
    int B3SOIqjdNode;

    B3SOIadj *B3SOIadjoint;

    // This provides a means to back up and restore a known-good
    // state.
    void *B3SOIbacking;
    void backup(DEV_BKMODE m)
        {
            if (m == DEV_SAVE) {
                if (!B3SOIbacking)
                    B3SOIbacking = new char[sizeof(sB3SOIinstance)];
                memcpy(B3SOIbacking, this, sizeof(sB3SOIinstance));
            }
            else if (m == DEV_RESTORE) {
                if (B3SOIbacking)
                    memcpy(this, B3SOIbacking, sizeof(sB3SOIinstance));
            }
            else {
                // DEV_CLEAR
                delete [] (char*)B3SOIbacking;
                B3SOIbacking = 0;
            }
        }

    double B3SOIphi;
    double B3SOIvtm;
    double B3SOIni;
    double B3SOIueff;
    double B3SOIthetavth; 
    double B3SOIvon;
    double B3SOIvdsat;
    double B3SOIcgdo;
    double B3SOIcgso;
    double B3SOIcgeo;

    double B3SOIids;
    double B3SOIic;
    double B3SOIibs;
    double B3SOIibd;
    double B3SOIiii;
    double B3SOIig;
    double B3SOIgigg;
    double B3SOIgigd;
    double B3SOIgigb;
    double B3SOIgige; /* v3.0 */
/* v3.1 wanh added for RF */
    double B3SOIgcrg;
    double B3SOIgcrgd;
    double B3SOIgcrgg;
    double B3SOIgcrgs;
    double B3SOIgcrgb;
/* v3.1 wanh added for end */

    double B3SOIigidl;
    double B3SOIitun;
    double B3SOIibp;
    double B3SOIabeff;
    double B3SOIvbseff;
    double B3SOIcbg;
    double B3SOIcbb;
    double B3SOIcbd;
    double B3SOIqb;
    double B3SOIqbf;
    double B3SOIqjs;
    double B3SOIqjd;
    int    B3SOIfloat;

/* v3.1 wanh added for RF */
    double B3SOIgrgeltd;
/* v3.1 wanh added for RF end */

    double B3SOIl;
    double B3SOIw;
    double B3SOIdrainArea;
    double B3SOIsourceArea;
    double B3SOIdrainSquares;
    double B3SOIsourceSquares;
    double B3SOIdrainPerimeter;
    double B3SOIsourcePerimeter;
    double B3SOIsourceConductance;
    double B3SOIdrainConductance;

    double B3SOIicVBS;
    double B3SOIicVDS;
    double B3SOIicVGS;
    double B3SOIicVES;
    double B3SOIicVPS;

    int B3SOIbjtoff;
    int B3SOIbodyMod;
    int B3SOIdebugMod;
    double B3SOIrth0;
    double B3SOIcth0;
    double B3SOIbodySquares;
    double B3SOIrbodyext;
    double B3SOIfrbody;


/* v2.0 release */
    double B3SOInbc;   
    double B3SOInseg;
    double B3SOIpdbcp;
    double B3SOIpsbcp;
    double B3SOIagbcp;
    double B3SOIaebcp;
    double B3SOIvbsusr;
    int B3SOItnodeout;

/* Deleted from pParam and moved to here */
    double B3SOIcsesw;
    double B3SOIcdesw;
    double B3SOIcsbox;
    double B3SOIcdbox;
    double B3SOIcsmin;
    double B3SOIcdmin;
    double B3SOIst4;
    double B3SOIdt4;

    int B3SOIoff;
    int B3SOImode;
/* v3.1 wanh added for RF */
    int B3SOIrgateMod;
/* v3.1 wanh added for RF end */

    int B3SOIsoiMod; /* v3.2 */

/* v3.2 */
    double B3SOInstar;
    double B3SOIAbulk; 
/* v3.2 end */

    /* OP point */
    double B3SOIqinv;
    double B3SOIcd;
    double B3SOIcjs;
    double B3SOIcjd;
    double B3SOIcbody;
/* v2.2 release */
    double B3SOIcgate;
    double B3SOIgigs;
    double B3SOIgigT;

    double B3SOIcbodcon;
    double B3SOIcth;
    double B3SOIcsubstrate;

    double B3SOIgm;
    double B3SOIgme;   /* v3.0 */
    double B3SOIcb;
    double B3SOIcdrain;
    double B3SOIgds;
    double B3SOIgmbs;
    double B3SOIgmT;

    double B3SOIgbbs;
    double B3SOIgbgs;
    double B3SOIgbds;
    double B3SOIgbes; /* v3.0 */
    double B3SOIgbps;
    double B3SOIgbT;

/* v3.0 */
    double B3SOIIgcs;
    double B3SOIgIgcsg;
    double B3SOIgIgcsd;
    double B3SOIgIgcss;
    double B3SOIgIgcsb;
    double B3SOIIgcd;
    double B3SOIgIgcdg;
    double B3SOIgIgcdd;
    double B3SOIgIgcds;
    double B3SOIgIgcdb;

    double B3SOIIgs;
    double B3SOIgIgsg;
    double B3SOIgIgss;
    double B3SOIIgd;
    double B3SOIgIgdg;
    double B3SOIgIgdd;


    double B3SOIgjsd;
    double B3SOIgjsb;
    double B3SOIgjsg;
    double B3SOIgjsT;

    double B3SOIgjdb;
    double B3SOIgjdd;
    double B3SOIgjdg;
    double B3SOIgjde; /* v3.0 */
    double B3SOIgjdT;

    double B3SOIgbpbs;
    double B3SOIgbpps;
    double B3SOIgbpT;

    double B3SOIgtempb;
    double B3SOIgtempg;
    double B3SOIgtempd;
    double B3SOIgtempe; /* v3.0 */
    double B3SOIgtempT;

    double B3SOIcggb;
    double B3SOIcgdb;
    double B3SOIcgsb;
    double B3SOIcgT;

    double B3SOIcbgb;
    double B3SOIcbdb;
    double B3SOIcbsb;
    double B3SOIcbeb;
    double B3SOIcbT;

    double B3SOIcdgb;
    double B3SOIcddb;
    double B3SOIcdsb;
    double B3SOIcdeb;
    double B3SOIcdT;

    double B3SOIceeb;
    double B3SOIceT;

    double B3SOIqse;
    double B3SOIgcse;
    double B3SOIqde;
    double B3SOIgcde;
    double B3SOIrds; /* v2.2.3 */
    double B3SOIVgsteff; /* v2.2.3 */
    double B3SOIVdseff;  /* v2.2.3 */
    double B3SOIAbovVgst2Vtm; /* v2.2.3 */

    struct b3soiSizeDependParam  *pParam;

    unsigned B3SOIlGiven :1;
    unsigned B3SOIwGiven :1;
    
    unsigned B3SOIdrainAreaGiven :1;
    unsigned B3SOIsourceAreaGiven    :1;
    unsigned B3SOIdrainSquaresGiven  :1;
    unsigned B3SOIsourceSquaresGiven :1;
    unsigned B3SOIdrainPerimeterGiven    :1;
    unsigned B3SOIsourcePerimeterGiven   :1;
    unsigned B3SOIdNodePrimeSet  :1;
    unsigned B3SOIsNodePrimeSet  :1;
    unsigned B3SOIicVBSGiven :1;
    unsigned B3SOIicVDSGiven :1;
    unsigned B3SOIicVGSGiven :1;
    unsigned B3SOIicVESGiven :1;
    unsigned B3SOIicVPSGiven :1;
    unsigned B3SOIbjtoffGiven :1;
    unsigned B3SOIdebugModGiven :1;
    unsigned B3SOIrth0Given :1;
    unsigned B3SOIcth0Given :1;
    unsigned B3SOIbodySquaresGiven :1;
    unsigned B3SOIfrbodyGiven:  1;

/* v2.0 release */
    unsigned B3SOInbcGiven :1;   
    unsigned B3SOInsegGiven :1;
    unsigned B3SOIpdbcpGiven :1;
    unsigned B3SOIpsbcpGiven :1;
    unsigned B3SOIagbcpGiven :1;
    unsigned B3SOIaebcpGiven :1;
    unsigned B3SOIvbsusrGiven :1;
    unsigned B3SOItnodeoutGiven :1;
    unsigned B3SOIoffGiven :1;

/* v3.1 wanh added for RF */
    unsigned B3SOIrgateModGiven :1;
/* v3.1 wanh added for RF end */

    unsigned B3SOIsoiModGiven :1; /* v3.2 */

/* v3.1 wanh added for RF */
    double *B3SOIGgmPtr;
    double *B3SOIGgePtr;

    double *B3SOIGMdpPtr;
    double *B3SOIGMgPtr;
    double *B3SOIGMspPtr;
    double *B3SOIGMgmPtr;
    double *B3SOIGMgePtr;
    double *B3SOIGMePtr;
    double *B3SOIGMbPtr;

    double *B3SOISPgmPtr;
    double *B3SOIDPgmPtr;
    double *B3SOIEgmPtr;

    double *B3SOIGEdpPtr;
    double *B3SOIGEgPtr;
    double *B3SOIGEgmPtr;
    double *B3SOIGEgePtr;
    double *B3SOIGEspPtr;
    double *B3SOIGEbPtr;
/* v3.1 wanh added for RF end */

    double *B3SOIGePtr;
    double *B3SOIDPePtr;
    double *B3SOISPePtr;

    double *B3SOIEePtr;
    double *B3SOIEbPtr;
    double *B3SOIBePtr;
    double *B3SOIEgPtr;
    double *B3SOIEdpPtr;
    double *B3SOIEspPtr;
    double *B3SOITemptempPtr;
    double *B3SOITempdpPtr;
    double *B3SOITempspPtr;
    double *B3SOITempgPtr;
    double *B3SOITempbPtr;
    double *B3SOITempePtr;   /* v3.0 */
    double *B3SOIGtempPtr;
    double *B3SOIDPtempPtr;
    double *B3SOISPtempPtr;
    double *B3SOIEtempPtr;
    double *B3SOIBtempPtr;
    double *B3SOIPtempPtr;
    double *B3SOIBpPtr;
    double *B3SOIPbPtr;
    double *B3SOIPpPtr;
    double *B3SOIDdPtr;
    double *B3SOIGgPtr;
    double *B3SOISsPtr;
    double *B3SOIBbPtr;
    double *B3SOIDPdpPtr;
    double *B3SOISPspPtr;
    double *B3SOIDdpPtr;
    double *B3SOIGbPtr;
    double *B3SOIGdpPtr;
    double *B3SOIGspPtr;
    double *B3SOISspPtr;
    double *B3SOIBdpPtr;
    double *B3SOIBspPtr;
    double *B3SOIDPspPtr;
    double *B3SOIDPdPtr;
    double *B3SOIBgPtr;
    double *B3SOIDPgPtr;
    double *B3SOISPgPtr;
    double *B3SOISPsPtr;
    double *B3SOIDPbPtr;
    double *B3SOISPbPtr;
    double *B3SOISPdpPtr;

    double *B3SOIVbsPtr;
    /* Debug */
    double *B3SOIIdsPtr;
    double *B3SOIIcPtr;
    double *B3SOIIbsPtr;
    double *B3SOIIbdPtr;
    double *B3SOIIiiPtr;
    double *B3SOIIgPtr;
    double *B3SOIGiggPtr;
    double *B3SOIGigdPtr;
    double *B3SOIGigbPtr;
    double *B3SOIIgidlPtr;
    double *B3SOIItunPtr;
    double *B3SOIIbpPtr;
    double *B3SOICbbPtr;
    double *B3SOICbdPtr;
    double *B3SOICbgPtr;
    double *B3SOIqbPtr;
    double *B3SOIQbfPtr;
    double *B3SOIQjsPtr;
    double *B3SOIQjdPtr;

/* indices to the array of B3SOI NOISE SOURCES */

#define B3SOIRDNOIZ       0
#define B3SOIRSNOIZ       1
#define B3SOIRGNOIZ       2
#define B3SOIIDNOIZ       3
#define B3SOIFLNOIZ       4
#define B3SOIFBNOIZ       5
#define B3SOIIGSNOIZ	  6
#define B3SOIIGDNOIZ	  7
#define B3SOIIGBNOIZ	  8
#define B3SOITOTNOIZ      9	/* v3.2 */

#define B3SOINSRCS        10     /* Number of MOSFET(3) noise sources v3.2 */

#ifndef NONOISE
    double B3SOInVar[NSTATVARS][B3SOINSRCS];
#else /* NONOISE */
        double **B3SOInVar;
#endif /* NONOISE */

};

#define B3SOIvbd GENstate + 0
#define B3SOIvbs GENstate + 1
#define B3SOIvgs GENstate + 2
#define B3SOIvds GENstate + 3
#define B3SOIves GENstate + 4
#define B3SOIvps GENstate + 5

#define B3SOIvg GENstate + 6
#define B3SOIvd GENstate + 7
#define B3SOIvs GENstate + 8
#define B3SOIvp GENstate + 9
#define B3SOIve GENstate + 10
#define B3SOIdeltemp GENstate + 11

#define B3SOIqb GENstate + 12
#define B3SOIcqb GENstate + 13
#define B3SOIqg GENstate + 14
#define B3SOIcqg GENstate + 15
#define B3SOIqd GENstate + 16
#define B3SOIcqd GENstate + 17
#define B3SOIqe GENstate + 18
#define B3SOIcqe GENstate + 19

#define B3SOIqbs  GENstate + 20
#define B3SOIqbd  GENstate + 21
#define B3SOIqbe  GENstate + 22

#define B3SOIqth GENstate + 23
#define B3SOIcqth GENstate + 24

/* v3.1 wanh added or changed for RF */
#define B3SOIvges GENstate + 25
#define B3SOIvgms GENstate + 26
#define B3SOIvgge GENstate + 27
#define B3SOIvggm GENstate + 28
#define B3SOIcqgmid GENstate + 29 
#define B3SOIqgmid GENstate  + 30 /*3.1 bug fix*/
/* v3.1 wanh added or changed for RF end */

// SRW - for interpolation     
#define B3SOIa_cd      GENstate + 31
#define B3SOIa_cbs     GENstate + 32
#define B3SOIa_cbd     GENstate + 33
#define B3SOIa_gm      GENstate + 34
#define B3SOIa_gds     GENstate + 35
#define B3SOIa_gmbs    GENstate + 36
#define B3SOIa_gbd     GENstate + 37
#define B3SOIa_gbs     GENstate + 38
#define B3SOIa_cggb    GENstate + 39
#define B3SOIa_cgdb    GENstate + 40
#define B3SOIa_cgsb    GENstate + 41
#define B3SOIa_cdgb    GENstate + 42
#define B3SOIa_cddb    GENstate + 43
#define B3SOIa_cdsb    GENstate + 44
#define B3SOIa_cbgb    GENstate + 45
#define B3SOIa_cbdb    GENstate + 46
#define B3SOIa_cbsb    GENstate + 47
#define B3SOIa_von     GENstate + 48
#define B3SOIa_vdsat   GENstate + 49

#define B3SOIa_id      GENstate + 50
#define B3SOIa_is      GENstate + 51
#define B3SOIa_ig      GENstate + 52
#define B3SOIa_ib      GENstate + 53
#define B3SOIa_ie      GENstate + 54

#define B3SOInumStates 55


struct b3soiSizeDependParam
{
    double Width;
    double Length;
    double Rth0;
    double Cth0;

    double B3SOIcdsc;           
    double B3SOIcdscb;    
    double B3SOIcdscd;       
    double B3SOIcit;           
    double B3SOInfactor;      
    double B3SOIvsat;         
    double B3SOIat;         
    double B3SOIa0;   
    double B3SOIags;      
    double B3SOIa1;         
    double B3SOIa2;         
    double B3SOIketa;     
    double B3SOInpeak;        
    double B3SOInsub;
    double B3SOIngate;        
    double B3SOIgamma1;      
    double B3SOIgamma2;     
    double B3SOIvbx;      
    double B3SOIvbi;       
    double B3SOIvbm;       
    double B3SOIvbsc;       
    double B3SOIxt;       
    double B3SOIphi;
    double B3SOIlitl;
    double B3SOIk1;
    double B3SOIkt1;
    double B3SOIkt1l;
    double B3SOIkt2;
    double B3SOIk2;
    double B3SOIk3;
    double B3SOIk3b;
    double B3SOIw0;
    double B3SOInlx;
    double B3SOIdvt0;      
    double B3SOIdvt1;      
    double B3SOIdvt2;      
    double B3SOIdvt0w;      
    double B3SOIdvt1w;      
    double B3SOIdvt2w;      
    double B3SOIdrout;      
    double B3SOIdsub;      
    double B3SOIvth0;
    double B3SOIua;
    double B3SOIua1;
    double B3SOIub;
    double B3SOIub1;
    double B3SOIuc;
    double B3SOIuc1;
    double B3SOIu0;
    double B3SOIute;
    double B3SOIvoff;
    double B3SOIvfb;
    double B3SOIuatemp;
    double B3SOIubtemp;
    double B3SOIuctemp;
    double B3SOIrbody;
    double B3SOIrth;
    double B3SOIcth;
    double B3SOIrds0denom;
    double B3SOIvfbb;
    double B3SOIjbjt;
    double B3SOIjdif;
    double B3SOIjrec;
    double B3SOIjtun;
    double B3SOIsdt1;
    double B3SOIst2;
    double B3SOIst3;
    double B3SOIdt2;
    double B3SOIdt3;
    double B3SOIdelta;
    double B3SOIrdsw;       
    double B3SOIrds0;       
    double B3SOIprwg;       
    double B3SOIprwb;       
    double B3SOIprt;       
    double B3SOIeta0;         
    double B3SOIetab;         
    double B3SOIpclm;      
    double B3SOIpdibl1;      
    double B3SOIpdibl2;      
    double B3SOIpdiblb;      
    double B3SOIpvag;       
    double B3SOIwr;
    double B3SOIdwg;
    double B3SOIdwb;
    double B3SOIb0;
    double B3SOIb1;
    double B3SOIalpha0;
    double B3SOIbeta0;

/* v3.0 */
    double B3SOIaigc;
    double B3SOIbigc;
    double B3SOIcigc;
    double B3SOIaigsd;
    double B3SOIbigsd;
    double B3SOIcigsd;
    double B3SOInigc;
    double B3SOIpigcd;
    double B3SOIpoxedge;
    double B3SOIdlcig;

/* v3.1 wanh added for RF */
    double B3SOIxrcrg1;
    double B3SOIxrcrg2;
/* v3.1 wanh added for RF end */

    /* CV model */
    double B3SOIcgsl;
    double B3SOIcgdl;
    double B3SOIckappa;
    double B3SOIcf;
    double B3SOIclc;
    double B3SOIcle;


/* Added for binning - START0 */
/* v3.1 */
    double B3SOIxj;
    double B3SOIalphaGB1;
    double B3SOIbetaGB1;
    double B3SOIalphaGB2;
    double B3SOIbetaGB2;
    double B3SOIndif;
    double B3SOIntrecf;
    double B3SOIntrecr;
    double B3SOIxbjt;
    double B3SOIxdif;
    double B3SOIxrec;
    double B3SOIxtun;

    double B3SOIkb1;
    double B3SOIk1w1;
    double B3SOIk1w2;
    double B3SOIketas;
    double B3SOIfbjtii;
    double B3SOIbeta1;
    double B3SOIbeta2;
    double B3SOIvdsatii0;
    double B3SOIlii;
    double B3SOIesatii;
    double B3SOIsii0;
    double B3SOIsii1;
    double B3SOIsii2;
    double B3SOIsiid;
    double B3SOIagidl;
    double B3SOIbgidl;
    double B3SOIngidl;
    double B3SOIntun;
    double B3SOIndiode;
    double B3SOInrecf0;
    double B3SOInrecr0;
    double B3SOIisbjt;
    double B3SOIisdif;
    double B3SOIisrec;
    double B3SOIistun;
    double B3SOIvrec0;
    double B3SOIvtun0;
    double B3SOInbjt;
    double B3SOIlbjt0;
    double B3SOIvabjt;
    double B3SOIaely;
    double B3SOIvsdfb;
    double B3SOIvsdth;
    double B3SOIdelvt;
/* Added by binning - END0 */

/* Pre-calculated constants */

    double B3SOIdw;
    double B3SOIdl;
    double B3SOIleff;
    double B3SOIweff;

    double B3SOIdwc;
    double B3SOIdlc;
    double B3SOIleffCV;
    double B3SOIweffCV;

    double B3SOIabulkCVfactor;
    double B3SOIcgso;
    double B3SOIcgdo;
    double B3SOIcgeo;

    double B3SOIu0temp;       
    double B3SOIvsattemp;   
    double B3SOIsqrtPhi;   
    double B3SOIphis3;   
    double B3SOIXdep0;          
    double B3SOIsqrtXdep0;          
    double B3SOItheta0vb0;
    double B3SOIthetaRout; 


/* v3.2 */
    double B3SOIqsi;

/* v2.2 release */
    double B3SOIoxideRatio;


/* v2.0 release */
    double B3SOIk1eff;
    double B3SOIwdios;
    double B3SOIwdiod;
    double B3SOIwdiodCV;
    double B3SOIwdiosCV;
    double B3SOIarfabjt;
    double B3SOIlratio;
    double B3SOIlratiodif;
    double B3SOIvearly;
    double B3SOIahli;
    double B3SOIahli0;
    double B3SOIvfbzb;
    double B3SOIldeb;
    double B3SOIacde;
    double B3SOImoin;
    double B3SOInoff; /* v3.2 */
    double B3SOIleffCVb;
    double B3SOIleffCVbg;

    double B3SOIcof1;
    double B3SOIcof2;
    double B3SOIcof3;
    double B3SOIcof4;
    double B3SOIcdep0;
/* v3.0 */
    double B3SOIToxRatio;
    double B3SOIAechvb;
    double B3SOIBechvb;
    double B3SOIToxRatioEdge;
    double B3SOIAechvbEdge;
    double B3SOIBechvbEdge;
    double B3SOIvfbsd;

    struct b3soiSizeDependParam  *pNext;
};


struct sB3SOImodel : sGENmodel
{
    sB3SOImodel()       { memset(this, 0, sizeof(sB3SOImodel)); }
    sB3SOImodel *next() { return (static_cast<sB3SOImodel*>(GENnextModel)); }
    sB3SOIinstance *inst()
                        { return (static_cast<sB3SOIinstance*>(GENinstances));}

    int B3SOItype;

    int    B3SOImobMod;
    int    B3SOIcapMod;
/*    int    B3SOInoiMod; v3.2 */
    int    B3SOIfnoiMod;	/* v3.2 */
    int    B3SOItnoiMod;	/* v3.2 */
    int    B3SOIshMod;
    int    B3SOIbinUnit;
    int    B3SOIparamChk;
    double B3SOIversion;             
    double B3SOItox;             
    double B3SOItoxm; /* v3.2 */
    double B3SOIdtoxcv; /* v2.2.3 */
    double B3SOIcdsc;           
    double B3SOIcdscb; 
    double B3SOIcdscd;          
    double B3SOIcit;           
    double B3SOInfactor;      
    double B3SOIvsat;         
    double B3SOIat;         
    double B3SOIa0;   
    double B3SOIags;      
    double B3SOIa1;         
    double B3SOIa2;         
    double B3SOIketa;     
    double B3SOInsub;
    double B3SOInpeak;        
    double B3SOIngate;        
    double B3SOIgamma1;      
    double B3SOIgamma2;     
    double B3SOIvbx;      
    double B3SOIvbm;       
    double B3SOIxt;       
    double B3SOIk1;
    double B3SOIkt1;
    double B3SOIkt1l;
    double B3SOIkt2;
    double B3SOIk2;
    double B3SOIk3;
    double B3SOIk3b;
    double B3SOIw0;
    double B3SOInlx;
    double B3SOIdvt0;      
    double B3SOIdvt1;      
    double B3SOIdvt2;      
    double B3SOIdvt0w;      
    double B3SOIdvt1w;      
    double B3SOIdvt2w;      
    double B3SOIdrout;      
    double B3SOIdsub;      
    double B3SOIvth0;
    double B3SOIua;
    double B3SOIua1;
    double B3SOIub;
    double B3SOIub1;
    double B3SOIuc;
    double B3SOIuc1;
    double B3SOIu0;
    double B3SOIute;
    double B3SOIvoff;
    double B3SOIdelta;
    double B3SOIrdsw;       
    double B3SOIprwg;
    double B3SOIprwb;
    double B3SOIprt;       
    double B3SOIeta0;         
    double B3SOIetab;         
    double B3SOIpclm;      
    double B3SOIpdibl1;      
    double B3SOIpdibl2;      
    double B3SOIpdiblb;
    double B3SOIpvag;       
    double B3SOIwr;
    double B3SOIdwg;
    double B3SOIdwb;
    double B3SOIb0;
    double B3SOIb1;
    double B3SOIalpha0;
    double B3SOItbox;
    double B3SOItsi;
    double B3SOIxj;
    double B3SOIkb1;
    double B3SOIrth0;
    double B3SOIcth0;
    double B3SOIngidl;
    double B3SOIagidl;
    double B3SOIbgidl;
    double B3SOIndiode;
    double B3SOIistun;
    double B3SOIxbjt;
    double B3SOIxdif;
    double B3SOIxrec;
    double B3SOIxtun;


/* v3.0 */
    int B3SOIsoiMod; /* v3.2 bug fix */
    double B3SOIvbs0pd; /* v3.2 */
    double B3SOIvbs0fd; /* v3.2 */
    double B3SOIvbsa;
    double B3SOInofffd;
    double B3SOIvofffd;
    double B3SOIk1b;
    double B3SOIk2b;
    double B3SOIdk2b;
    double B3SOIdvbd0;
    double B3SOIdvbd1;
    double B3SOImoinFD;

/* v3.0 */
    int B3SOIigbMod;
    int B3SOIigcMod;
    double B3SOIaigc;
    double B3SOIbigc;
    double B3SOIcigc;
    double B3SOIaigsd;
    double B3SOIbigsd;
    double B3SOIcigsd;
    double B3SOInigc;
    double B3SOIpigcd;
    double B3SOIpoxedge;
    double B3SOIdlcig;

/* v3.1 wanh added for RF */
    int    B3SOIrgateMod;
    double B3SOIxrcrg1;
    double B3SOIxrcrg2;
    double B3SOIrshg;
    double B3SOIngcon;
    double B3SOIxgw;
    double B3SOIxgl;
/* v3.1 wanh added for RF end */

/* v3.2 for noise */
    double B3SOItnoia;
    double B3SOItnoib;
    double B3SOIrnoia;
    double B3SOIrnoib;
    double B3SOIntnoi; 
/* v3.2 for noise end */

/* v2.2 release */
    double B3SOIwth0;
    double B3SOIrhalo;
    double B3SOIntox;
    double B3SOItoxref;
    double B3SOIebg;
    double B3SOIvevb;
    double B3SOIalphaGB1;
    double B3SOIbetaGB1;
    double B3SOIvgb1;
    double B3SOIvecb;
    double B3SOIalphaGB2;
    double B3SOIbetaGB2;
    double B3SOIvgb2;
    double B3SOItoxqm;
    double B3SOIvoxh;
    double B3SOIdeltavox;


/* v2.0 release */
    double B3SOIk1w1;  
    double B3SOIk1w2;
    double B3SOIketas;
    double B3SOIdwbc;
    double B3SOIbeta0;
    double B3SOIbeta1;
    double B3SOIbeta2;
    double B3SOIvdsatii0;
    double B3SOItii;
    double B3SOIlii;
    double B3SOIsii0;
    double B3SOIsii1;
    double B3SOIsii2;
    double B3SOIsiid;
    double B3SOIfbjtii;
    double B3SOIesatii;
    double B3SOIntun;
    double B3SOInrecf0;
    double B3SOInrecr0;
    double B3SOIisbjt;
    double B3SOIisdif;
    double B3SOIisrec;
    double B3SOIln;
    double B3SOIvrec0;
    double B3SOIvtun0;
    double B3SOInbjt;
    double B3SOIlbjt0;
    double B3SOIldif0;
    double B3SOIvabjt;
    double B3SOIaely;
    double B3SOIahli;
    double B3SOIrbody;
    double B3SOIrbsh;
    double B3SOItt;
    double B3SOIndif;
    double B3SOIvsdfb;
    double B3SOIvsdth;
    double B3SOIcsdmin;
    double B3SOIasd;
    double B3SOIntrecf;
    double B3SOIntrecr;
    double B3SOIdlcb;
    double B3SOIfbody;
    double B3SOItcjswg;
    double B3SOItpbswg;
    double B3SOIacde;
    double B3SOImoin;
    double B3SOInoff; /* v3.2 */
    double B3SOIdelvt;
    double B3SOIdlbg;

    /* CV model */
    double B3SOIcgsl;
    double B3SOIcgdl;
    double B3SOIckappa;
    double B3SOIcf;
    double B3SOIclc;
    double B3SOIcle;
    double B3SOIdwc;
    double B3SOIdlc;


    double B3SOItnom;
    double B3SOIcgso;
    double B3SOIcgdo;
    double B3SOIcgeo;

    double B3SOIxpart;
    double B3SOIcFringOut;
    double B3SOIcFringMax;

    double B3SOIsheetResistance;
    double B3SOIbodyJctGateSideGradingCoeff;
    double B3SOIGatesidewallJctPotential;
    double B3SOIunitLengthGateSidewallJctCap;
    double B3SOIcsdesw;

    double B3SOILint;
    double B3SOILl;
    double B3SOILlc; /* v2.2.3 */
    double B3SOILln;
    double B3SOILw;
    double B3SOILwc; /* v2.2.3 */
    double B3SOILwn;
    double B3SOILwl;
    double B3SOILwlc; /* v2.2.3 */
    double B3SOILmin;
    double B3SOILmax;

    double B3SOIWint;
    double B3SOIWl;
    double B3SOIWlc; /* v2.2.3 */
    double B3SOIWln;
    double B3SOIWw;
    double B3SOIWwc; /* v2.2.3 */
    double B3SOIWwn;
    double B3SOIWwl;
    double B3SOIWwlc; /* v2.2.3 */
    double B3SOIWmin;
    double B3SOIWmax;

/* Added for binning - START1 */
    /* Length Dependence */
/* v3.1 */
    double B3SOIlxj;
    double B3SOIlalphaGB1;
    double B3SOIlbetaGB1;
    double B3SOIlalphaGB2;
    double B3SOIlbetaGB2;
    double B3SOIlndif;
    double B3SOIlntrecf;
    double B3SOIlntrecr;
    double B3SOIlxbjt;
    double B3SOIlxdif;
    double B3SOIlxrec;
    double B3SOIlxtun;
    double B3SOIlcgsl;
    double B3SOIlcgdl;
    double B3SOIlckappa;
    double B3SOIlua1;
    double B3SOIlub1;
    double B3SOIluc1;
    double B3SOIlute;
    double B3SOIlkt1;
    double B3SOIlkt1l;
    double B3SOIlkt2;
    double B3SOIlat;
    double B3SOIlprt;

/* v3.0 */
    double B3SOIlaigc;
    double B3SOIlbigc;
    double B3SOIlcigc;
    double B3SOIlaigsd;
    double B3SOIlbigsd;
    double B3SOIlcigsd;
    double B3SOIlnigc;
    double B3SOIlpigcd;
    double B3SOIlpoxedge;

    double B3SOIlnpeak;        
    double B3SOIlnsub;
    double B3SOIlngate;        
    double B3SOIlvth0;
    double B3SOIlk1;
    double B3SOIlk1w1;
    double B3SOIlk1w2;
    double B3SOIlk2;
    double B3SOIlk3;
    double B3SOIlk3b;
    double B3SOIlkb1;
    double B3SOIlw0;
    double B3SOIlnlx;
    double B3SOIldvt0;      
    double B3SOIldvt1;      
    double B3SOIldvt2;      
    double B3SOIldvt0w;      
    double B3SOIldvt1w;      
    double B3SOIldvt2w;      
    double B3SOIlu0;
    double B3SOIlua;
    double B3SOIlub;
    double B3SOIluc;
    double B3SOIlvsat;         
    double B3SOIla0;   
    double B3SOIlags;      
    double B3SOIlb0;
    double B3SOIlb1;
    double B3SOIlketa;
    double B3SOIlketas;
    double B3SOIla1;         
    double B3SOIla2;         
    double B3SOIlrdsw;       
    double B3SOIlprwb;
    double B3SOIlprwg;
    double B3SOIlwr;
    double B3SOIlnfactor;      
    double B3SOIldwg;
    double B3SOIldwb;
    double B3SOIlvoff;
    double B3SOIleta0;         
    double B3SOIletab;         
    double B3SOIldsub;      
    double B3SOIlcit;           
    double B3SOIlcdsc;           
    double B3SOIlcdscb; 
    double B3SOIlcdscd;          
    double B3SOIlpclm;      
    double B3SOIlpdibl1;      
    double B3SOIlpdibl2;      
    double B3SOIlpdiblb;      
    double B3SOIldrout;      
    double B3SOIlpvag;       
    double B3SOIldelta;
    double B3SOIlalpha0;
    double B3SOIlfbjtii;
    double B3SOIlbeta0;
    double B3SOIlbeta1;
    double B3SOIlbeta2;         
    double B3SOIlvdsatii0;     
    double B3SOIllii;      
    double B3SOIlesatii;     
    double B3SOIlsii0;      
    double B3SOIlsii1;       
    double B3SOIlsii2;       
    double B3SOIlsiid;
    double B3SOIlagidl;
    double B3SOIlbgidl;
    double B3SOIlngidl;
    double B3SOIlntun;
    double B3SOIlndiode;
    double B3SOIlnrecf0;
    double B3SOIlnrecr0;
    double B3SOIlisbjt;       
    double B3SOIlisdif;
    double B3SOIlisrec;       
    double B3SOIlistun;       
    double B3SOIlvrec0;
    double B3SOIlvtun0;
    double B3SOIlnbjt;
    double B3SOIllbjt0;
    double B3SOIlvabjt;
    double B3SOIlaely;
    double B3SOIlahli;
   
/* v3.1 wanh added for RF */
    double B3SOIlxrcrg1;
    double B3SOIlxrcrg2;
/* v3.1 wanh added for RF end */
 
    /* CV model */
    double B3SOIlvsdfb;
    double B3SOIlvsdth;
    double B3SOIldelvt;
    double B3SOIlacde;
    double B3SOIlmoin;
    double B3SOIlnoff; /* v3.2 */

    /* Width Dependence */
/* v3.1 */
    double B3SOIwxj;
    double B3SOIwalphaGB1;
    double B3SOIwbetaGB1;
    double B3SOIwalphaGB2;
    double B3SOIwbetaGB2;
    double B3SOIwndif;
    double B3SOIwntrecf;
    double B3SOIwntrecr;
    double B3SOIwxbjt;
    double B3SOIwxdif;
    double B3SOIwxrec;
    double B3SOIwxtun;
    double B3SOIwcgsl;
    double B3SOIwcgdl;
    double B3SOIwckappa;
    double B3SOIwua1;
    double B3SOIwub1;
    double B3SOIwuc1;
    double B3SOIwute;
    double B3SOIwkt1;
    double B3SOIwkt1l;
    double B3SOIwkt2;
    double B3SOIwat;
    double B3SOIwprt;

/* v3.0 */
    double B3SOIwaigc;
    double B3SOIwbigc;
    double B3SOIwcigc;
    double B3SOIwaigsd;
    double B3SOIwbigsd;
    double B3SOIwcigsd;
    double B3SOIwnigc;
    double B3SOIwpigcd;
    double B3SOIwpoxedge;

    double B3SOIwnpeak;        
    double B3SOIwnsub;
    double B3SOIwngate;        
    double B3SOIwvth0;
    double B3SOIwk1;
    double B3SOIwk1w1;
    double B3SOIwk1w2;
    double B3SOIwk2;
    double B3SOIwk3;
    double B3SOIwk3b;
    double B3SOIwkb1;
    double B3SOIww0;
    double B3SOIwnlx;
    double B3SOIwdvt0;      
    double B3SOIwdvt1;      
    double B3SOIwdvt2;      
    double B3SOIwdvt0w;      
    double B3SOIwdvt1w;      
    double B3SOIwdvt2w;      
    double B3SOIwu0;
    double B3SOIwua;
    double B3SOIwub;
    double B3SOIwuc;
    double B3SOIwvsat;         
    double B3SOIwa0;   
    double B3SOIwags;      
    double B3SOIwb0;
    double B3SOIwb1;
    double B3SOIwketa;
    double B3SOIwketas;
    double B3SOIwa1;         
    double B3SOIwa2;         
    double B3SOIwrdsw;       
    double B3SOIwprwb;
    double B3SOIwprwg;
    double B3SOIwwr;
    double B3SOIwnfactor;      
    double B3SOIwdwg;
    double B3SOIwdwb;
    double B3SOIwvoff;
    double B3SOIweta0;         
    double B3SOIwetab;         
    double B3SOIwdsub;      
    double B3SOIwcit;           
    double B3SOIwcdsc;           
    double B3SOIwcdscb; 
    double B3SOIwcdscd;          
    double B3SOIwpclm;      
    double B3SOIwpdibl1;      
    double B3SOIwpdibl2;      
    double B3SOIwpdiblb;      
    double B3SOIwdrout;      
    double B3SOIwpvag;       
    double B3SOIwdelta;
    double B3SOIwalpha0;
    double B3SOIwfbjtii;
    double B3SOIwbeta0;
    double B3SOIwbeta1;
    double B3SOIwbeta2;         
    double B3SOIwvdsatii0;     
    double B3SOIwlii;      
    double B3SOIwesatii;     
    double B3SOIwsii0;      
    double B3SOIwsii1;       
    double B3SOIwsii2;       
    double B3SOIwsiid;
    double B3SOIwagidl;
    double B3SOIwbgidl;
    double B3SOIwngidl;
    double B3SOIwntun;
    double B3SOIwndiode;
    double B3SOIwnrecf0;
    double B3SOIwnrecr0;
    double B3SOIwisbjt;       
    double B3SOIwisdif;
    double B3SOIwisrec;       
    double B3SOIwistun;       
    double B3SOIwvrec0;
    double B3SOIwvtun0;
    double B3SOIwnbjt;
    double B3SOIwlbjt0;
    double B3SOIwvabjt;
    double B3SOIwaely;
    double B3SOIwahli;
   
/* v3.1 wanh added for RF */
    double B3SOIwxrcrg1;
    double B3SOIwxrcrg2;
/* v3.1 wanh added for RF end */
 
    /* CV model */
    double B3SOIwvsdfb;
    double B3SOIwvsdth;
    double B3SOIwdelvt;
    double B3SOIwacde;
    double B3SOIwmoin;
    double B3SOIwnoff; /* v3.2 */

    /* Cross-term Dependence */
/* v3.1 */
    double B3SOIpxj;
    double B3SOIpalphaGB1;
    double B3SOIpbetaGB1;
    double B3SOIpalphaGB2;
    double B3SOIpbetaGB2;
    double B3SOIpndif;
    double B3SOIpntrecf;
    double B3SOIpntrecr;
    double B3SOIpxbjt;
    double B3SOIpxdif;
    double B3SOIpxrec;
    double B3SOIpxtun;
    double B3SOIpcgsl;
    double B3SOIpcgdl;
    double B3SOIpckappa;
    double B3SOIpua1;
    double B3SOIpub1;
    double B3SOIpuc1;
    double B3SOIpute;
    double B3SOIpkt1;
    double B3SOIpkt1l;
    double B3SOIpkt2;
    double B3SOIpat;
    double B3SOIpprt;

/* v3.0 */
    double B3SOIpaigc;
    double B3SOIpbigc;
    double B3SOIpcigc;
    double B3SOIpaigsd;
    double B3SOIpbigsd;
    double B3SOIpcigsd;
    double B3SOIpnigc;
    double B3SOIppigcd;
    double B3SOIppoxedge;

    double B3SOIpnpeak;        
    double B3SOIpnsub;
    double B3SOIpngate;        
    double B3SOIpvth0;
    double B3SOIpk1;
    double B3SOIpk1w1;
    double B3SOIpk1w2;
    double B3SOIpk2;
    double B3SOIpk3;
    double B3SOIpk3b;
    double B3SOIpkb1;
    double B3SOIpw0;
    double B3SOIpnlx;
    double B3SOIpdvt0;      
    double B3SOIpdvt1;      
    double B3SOIpdvt2;      
    double B3SOIpdvt0w;      
    double B3SOIpdvt1w;      
    double B3SOIpdvt2w;      
    double B3SOIpu0;
    double B3SOIpua;
    double B3SOIpub;
    double B3SOIpuc;
    double B3SOIpvsat;         
    double B3SOIpa0;   
    double B3SOIpags;      
    double B3SOIpb0;
    double B3SOIpb1;
    double B3SOIpketa;
    double B3SOIpketas;
    double B3SOIpa1;         
    double B3SOIpa2;         
    double B3SOIprdsw;       
    double B3SOIpprwb;
    double B3SOIpprwg;
    double B3SOIpwr;
    double B3SOIpnfactor;      
    double B3SOIpdwg;
    double B3SOIpdwb;
    double B3SOIpvoff;
    double B3SOIpeta0;         
    double B3SOIpetab;         
    double B3SOIpdsub;      
    double B3SOIpcit;           
    double B3SOIpcdsc;           
    double B3SOIpcdscb; 
    double B3SOIpcdscd;          
    double B3SOIppclm;      
    double B3SOIppdibl1;      
    double B3SOIppdibl2;      
    double B3SOIppdiblb;      
    double B3SOIpdrout;      
    double B3SOIppvag;       
    double B3SOIpdelta;
    double B3SOIpalpha0;
    double B3SOIpfbjtii;
    double B3SOIpbeta0;
    double B3SOIpbeta1;
    double B3SOIpbeta2;         
    double B3SOIpvdsatii0;     
    double B3SOIplii;      
    double B3SOIpesatii;     
    double B3SOIpsii0;      
    double B3SOIpsii1;       
    double B3SOIpsii2;       
    double B3SOIpsiid;
    double B3SOIpagidl;
    double B3SOIpbgidl;
    double B3SOIpngidl;
    double B3SOIpntun;
    double B3SOIpndiode;
    double B3SOIpnrecf0;
    double B3SOIpnrecr0;
    double B3SOIpisbjt;       
    double B3SOIpisdif;
    double B3SOIpisrec;       
    double B3SOIpistun;       
    double B3SOIpvrec0;
    double B3SOIpvtun0;
    double B3SOIpnbjt;
    double B3SOIplbjt0;
    double B3SOIpvabjt;
    double B3SOIpaely;
    double B3SOIpahli;
/* v3.1 wanh added for RF */
    double B3SOIpxrcrg1;
    double B3SOIpxrcrg2;
/* v3.1 wanh added for RF end */

    /* CV model */
    double B3SOIpvsdfb;
    double B3SOIpvsdth;
    double B3SOIpdelvt;
    double B3SOIpacde;
    double B3SOIpmoin;
    double B3SOIpnoff; /* v3.2 */
/* Added for binning - END1 */

/* Pre-calculated constants */
    double B3SOIcbox;
    double B3SOIcsi;
    double B3SOIcsieff;
    double B3SOIcoxt;
    double B3SOInfb;
    double B3SOIadice;
    double B3SOIeg0;

    /* MCJ: move to size-dependent param. */
    double B3SOIvtm;   
    double B3SOIcox;
    double B3SOIcof1;
    double B3SOIcof2;
    double B3SOIcof3;
    double B3SOIcof4;
    double B3SOIvcrit;
    double B3SOIfactor1;

    double B3SOIoxideTrapDensityA;      
    double B3SOIoxideTrapDensityB;     
    double B3SOIoxideTrapDensityC;  
    double B3SOIem;  
    double B3SOIef;  
    double B3SOIaf;  
    double B3SOIkf;  
    double B3SOInoif;  

    struct b3soiSizeDependParam *pSizeDependParamKnot;

    /* Flags */
/* v3.1 wanh added for RF */
    unsigned B3SOIrgateModGiven :1;
/* v3.1 wanh added for RF end */

/* v3.0 */
    unsigned B3SOIsoiModGiven: 1;
    unsigned B3SOIvbs0pdGiven: 1; /* v3.2 */
    unsigned B3SOIvbs0fdGiven: 1; /* v3.2 */
    unsigned B3SOIvbsaGiven  : 1;
    unsigned B3SOInofffdGiven: 1;
    unsigned B3SOIvofffdGiven: 1;
    unsigned B3SOIk1bGiven:    1;
    unsigned B3SOIk2bGiven:    1;
    unsigned B3SOIdk2bGiven:   1;
    unsigned B3SOIdvbd0Given:  1;
    unsigned B3SOIdvbd1Given:  1;
    unsigned B3SOImoinFDGiven: 1;


    unsigned B3SOItboxGiven:1;
    unsigned B3SOItsiGiven :1;
    unsigned B3SOIxjGiven :1;
    unsigned B3SOIkb1Given :1;
    unsigned B3SOIrth0Given :1;
    unsigned B3SOIcth0Given :1;
    unsigned B3SOIngidlGiven :1;
    unsigned B3SOIagidlGiven :1;
    unsigned B3SOIbgidlGiven :1;
    unsigned B3SOIndiodeGiven :1;
    unsigned B3SOIxbjtGiven :1;
    unsigned B3SOIxdifGiven :1;
    unsigned B3SOIxrecGiven :1;
    unsigned B3SOIxtunGiven :1;
    unsigned B3SOIttGiven :1;
    unsigned B3SOIvsdfbGiven :1;
    unsigned B3SOIvsdthGiven :1;
    unsigned B3SOIasdGiven :1;
    unsigned B3SOIcsdminGiven :1;

    unsigned  B3SOImobModGiven :1;
    unsigned  B3SOIbinUnitGiven :1;
    unsigned  B3SOIcapModGiven :1;
    unsigned  B3SOIparamChkGiven :1;
/*    unsigned  B3SOInoiModGiven :1;  v3.2 */
    unsigned  B3SOIshModGiven :1;
    unsigned  B3SOItypeGiven   :1;
    unsigned  B3SOItoxGiven   :1;
    unsigned  B3SOItoxmGiven   :1; /* v3.2 */
    unsigned  B3SOIdtoxcvGiven   :1; /* v2.2.3 */
    unsigned  B3SOIversionGiven   :1;

    unsigned  B3SOIcdscGiven   :1;
    unsigned  B3SOIcdscbGiven   :1;
    unsigned  B3SOIcdscdGiven   :1;
    unsigned  B3SOIcitGiven   :1;
    unsigned  B3SOInfactorGiven   :1;
    unsigned  B3SOIvsatGiven   :1;
    unsigned  B3SOIatGiven   :1;
    unsigned  B3SOIa0Given   :1;
    unsigned  B3SOIagsGiven   :1;
    unsigned  B3SOIa1Given   :1;
    unsigned  B3SOIa2Given   :1;
    unsigned  B3SOIketaGiven   :1;    
    unsigned  B3SOInsubGiven   :1;
    unsigned  B3SOInpeakGiven   :1;
    unsigned  B3SOIngateGiven   :1;
    unsigned  B3SOIgamma1Given   :1;
    unsigned  B3SOIgamma2Given   :1;
    unsigned  B3SOIvbxGiven   :1;
    unsigned  B3SOIvbmGiven   :1;
    unsigned  B3SOIxtGiven   :1;
    unsigned  B3SOIk1Given   :1;
    unsigned  B3SOIkt1Given   :1;
    unsigned  B3SOIkt1lGiven   :1;
    unsigned  B3SOIkt2Given   :1;
    unsigned  B3SOIk2Given   :1;
    unsigned  B3SOIk3Given   :1;
    unsigned  B3SOIk3bGiven   :1;
    unsigned  B3SOIw0Given   :1;
    unsigned  B3SOInlxGiven   :1;
    unsigned  B3SOIdvt0Given   :1;   
    unsigned  B3SOIdvt1Given   :1;     
    unsigned  B3SOIdvt2Given   :1;     
    unsigned  B3SOIdvt0wGiven   :1;   
    unsigned  B3SOIdvt1wGiven   :1;     
    unsigned  B3SOIdvt2wGiven   :1;     
    unsigned  B3SOIdroutGiven   :1;     
    unsigned  B3SOIdsubGiven   :1;     
    unsigned  B3SOIvth0Given   :1;
    unsigned  B3SOIuaGiven   :1;
    unsigned  B3SOIua1Given   :1;
    unsigned  B3SOIubGiven   :1;
    unsigned  B3SOIub1Given   :1;
    unsigned  B3SOIucGiven   :1;
    unsigned  B3SOIuc1Given   :1;
    unsigned  B3SOIu0Given   :1;
    unsigned  B3SOIuteGiven   :1;
    unsigned  B3SOIvoffGiven   :1;
    unsigned  B3SOIrdswGiven   :1;      
    unsigned  B3SOIprwgGiven   :1;      
    unsigned  B3SOIprwbGiven   :1;      
    unsigned  B3SOIprtGiven   :1;      
    unsigned  B3SOIeta0Given   :1;    
    unsigned  B3SOIetabGiven   :1;    
    unsigned  B3SOIpclmGiven   :1;   
    unsigned  B3SOIpdibl1Given   :1;   
    unsigned  B3SOIpdibl2Given   :1;  
    unsigned  B3SOIpdiblbGiven   :1;  
    unsigned  B3SOIpvagGiven   :1;    
    unsigned  B3SOIdeltaGiven  :1;     
    unsigned  B3SOIwrGiven   :1;
    unsigned  B3SOIdwgGiven   :1;
    unsigned  B3SOIdwbGiven   :1;
    unsigned  B3SOIb0Given   :1;
    unsigned  B3SOIb1Given   :1;
    unsigned  B3SOIalpha0Given   :1;
/* v3.2 */
    unsigned  B3SOIfnoiModGiven	:1;
    unsigned  B3SOItnoiModGiven :1;
    unsigned  B3SOItnoiaGiven	:1;
    unsigned  B3SOItnoibGiven	:1;
    unsigned  B3SOIrnoiaGiven	:1;
    unsigned  B3SOIrnoibGiven	:1;
    unsigned  B3SOIntnoiGiven	:1;
/* v3.2 */

/* v3.1 wanh added for RF */
    unsigned B3SOIxrcrg1Given   :1;
    unsigned B3SOIxrcrg2Given   :1;
    unsigned B3SOIrshgGiven 	:1;
    unsigned B3SOIngconGiven 	:1;
    unsigned B3SOIxgwGiven 	:1;
    unsigned B3SOIxglGiven 	:1;
/* v3.1 wanh added for RF end */

/* v2.2 release */
    unsigned  B3SOIwth0Given   :1;
    unsigned  B3SOIrhaloGiven  :1;
    unsigned  B3SOIntoxGiven   :1;
    unsigned  B3SOItoxrefGiven   :1;
    unsigned  B3SOIebgGiven     :1;
    unsigned  B3SOIvevbGiven   :1;
    unsigned  B3SOIalphaGB1Given :1;
    unsigned  B3SOIbetaGB1Given  :1;
    unsigned  B3SOIvgb1Given        :1;
    unsigned  B3SOIvecbGiven        :1;
    unsigned  B3SOIalphaGB2Given    :1;
    unsigned  B3SOIbetaGB2Given     :1;
    unsigned  B3SOIvgb2Given        :1;
    unsigned  B3SOItoxqmGiven  :1;
    unsigned  B3SOIigbModGiven  :1; /* v3.0 */
    unsigned  B3SOIvoxhGiven   :1;
    unsigned  B3SOIdeltavoxGiven :1;
    unsigned  B3SOIigcModGiven  :1; /* v3.0 */
/* v3.0 */
    unsigned  B3SOIaigcGiven   :1;
    unsigned  B3SOIbigcGiven   :1;
    unsigned  B3SOIcigcGiven   :1;
    unsigned  B3SOIaigsdGiven   :1;
    unsigned  B3SOIbigsdGiven   :1;
    unsigned  B3SOIcigsdGiven   :1;   
    unsigned  B3SOInigcGiven   :1;
    unsigned  B3SOIpigcdGiven   :1;
    unsigned  B3SOIpoxedgeGiven   :1;
    unsigned  B3SOIdlcigGiven   :1;


/* v2.0 release */
    unsigned  B3SOIk1w1Given   :1;   
    unsigned  B3SOIk1w2Given   :1;
    unsigned  B3SOIketasGiven  :1;
    unsigned  B3SOIdwbcGiven  :1;
    unsigned  B3SOIbeta0Given  :1;
    unsigned  B3SOIbeta1Given  :1;
    unsigned  B3SOIbeta2Given  :1;
    unsigned  B3SOIvdsatii0Given  :1;
    unsigned  B3SOItiiGiven  :1;
    unsigned  B3SOIliiGiven  :1;
    unsigned  B3SOIsii0Given  :1;
    unsigned  B3SOIsii1Given  :1;
    unsigned  B3SOIsii2Given  :1;
    unsigned  B3SOIsiidGiven  :1;
    unsigned  B3SOIfbjtiiGiven :1;
    unsigned  B3SOIesatiiGiven :1;
    unsigned  B3SOIntunGiven  :1;
    unsigned  B3SOInrecf0Given  :1;
    unsigned  B3SOInrecr0Given  :1;
    unsigned  B3SOIisbjtGiven  :1;
    unsigned  B3SOIisdifGiven  :1;
    unsigned  B3SOIisrecGiven  :1;
    unsigned  B3SOIistunGiven  :1;
    unsigned  B3SOIlnGiven  :1;
    unsigned  B3SOIvrec0Given  :1;
    unsigned  B3SOIvtun0Given  :1;
    unsigned  B3SOInbjtGiven  :1;
    unsigned  B3SOIlbjt0Given  :1;
    unsigned  B3SOIldif0Given  :1;
    unsigned  B3SOIvabjtGiven  :1;
    unsigned  B3SOIaelyGiven  :1;
    unsigned  B3SOIahliGiven  :1;
    unsigned  B3SOIrbodyGiven :1;
    unsigned  B3SOIrbshGiven  :1;
    unsigned  B3SOIndifGiven  :1;
    unsigned  B3SOIntrecfGiven  :1;
    unsigned  B3SOIntrecrGiven  :1;
    unsigned  B3SOIdlcbGiven    :1;
    unsigned  B3SOIfbodyGiven   :1;
    unsigned  B3SOItcjswgGiven  :1;
    unsigned  B3SOItpbswgGiven  :1;
    unsigned  B3SOIacdeGiven  :1;
    unsigned  B3SOImoinGiven  :1;
    unsigned  B3SOInoffGiven:  1; /* v3.2 */
    unsigned  B3SOIdelvtGiven  :1;
    unsigned  B3SOIdlbgGiven  :1;

    
    /* CV model */
    unsigned  B3SOIcgslGiven   :1;
    unsigned  B3SOIcgdlGiven   :1;
    unsigned  B3SOIckappaGiven   :1;
    unsigned  B3SOIcfGiven   :1;
    unsigned  B3SOIclcGiven   :1;
    unsigned  B3SOIcleGiven   :1;
    unsigned  B3SOIdwcGiven   :1;
    unsigned  B3SOIdlcGiven   :1;

/* Added for binning - START2 */
    /* Length Dependence */
/* v3.1 */
    unsigned B3SOIlxjGiven :1;
    unsigned B3SOIlalphaGB1Given :1;
    unsigned B3SOIlbetaGB1Given :1;
    unsigned B3SOIlalphaGB2Given :1;
    unsigned B3SOIlbetaGB2Given :1;
    unsigned B3SOIlndifGiven :1;
    unsigned B3SOIlntrecfGiven :1;
    unsigned B3SOIlntrecrGiven :1;
    unsigned B3SOIlxbjtGiven :1;
    unsigned B3SOIlxdifGiven :1;
    unsigned B3SOIlxrecGiven :1;
    unsigned B3SOIlxtunGiven :1;
    unsigned B3SOIlcgslGiven :1;
    unsigned B3SOIlcgdlGiven :1;
    unsigned B3SOIlckappaGiven :1;
    unsigned B3SOIlua1Given :1;
    unsigned B3SOIlub1Given :1;
    unsigned B3SOIluc1Given :1;
    unsigned B3SOIluteGiven :1;
    unsigned B3SOIlkt1Given :1;
    unsigned B3SOIlkt1lGiven :1;
    unsigned B3SOIlkt2Given :1;
    unsigned B3SOIlatGiven :1;
    unsigned B3SOIlprtGiven :1;

/* v3.0 */
    unsigned  B3SOIlaigcGiven   :1;
    unsigned  B3SOIlbigcGiven   :1;
    unsigned  B3SOIlcigcGiven   :1;
    unsigned  B3SOIlaigsdGiven   :1;
    unsigned  B3SOIlbigsdGiven   :1;
    unsigned  B3SOIlcigsdGiven   :1;   
    unsigned  B3SOIlnigcGiven   :1;
    unsigned  B3SOIlpigcdGiven   :1;
    unsigned  B3SOIlpoxedgeGiven   :1;

    unsigned  B3SOIlnpeakGiven   :1;        
    unsigned  B3SOIlnsubGiven   :1;
    unsigned  B3SOIlngateGiven   :1;        
    unsigned  B3SOIlvth0Given   :1;
    unsigned  B3SOIlk1Given   :1;
    unsigned  B3SOIlk1w1Given   :1;
    unsigned  B3SOIlk1w2Given   :1;
    unsigned  B3SOIlk2Given   :1;
    unsigned  B3SOIlk3Given   :1;
    unsigned  B3SOIlk3bGiven   :1;
    unsigned  B3SOIlkb1Given   :1;
    unsigned  B3SOIlw0Given   :1;
    unsigned  B3SOIlnlxGiven   :1;
    unsigned  B3SOIldvt0Given   :1;      
    unsigned  B3SOIldvt1Given   :1;      
    unsigned  B3SOIldvt2Given   :1;      
    unsigned  B3SOIldvt0wGiven   :1;      
    unsigned  B3SOIldvt1wGiven   :1;      
    unsigned  B3SOIldvt2wGiven   :1;      
    unsigned  B3SOIlu0Given   :1;
    unsigned  B3SOIluaGiven   :1;
    unsigned  B3SOIlubGiven   :1;
    unsigned  B3SOIlucGiven   :1;
    unsigned  B3SOIlvsatGiven   :1;         
    unsigned  B3SOIla0Given   :1;   
    unsigned  B3SOIlagsGiven   :1;      
    unsigned  B3SOIlb0Given   :1;
    unsigned  B3SOIlb1Given   :1;
    unsigned  B3SOIlketaGiven   :1;
    unsigned  B3SOIlketasGiven   :1;
    unsigned  B3SOIla1Given   :1;         
    unsigned  B3SOIla2Given   :1;         
    unsigned  B3SOIlrdswGiven   :1;       
    unsigned  B3SOIlprwbGiven   :1;
    unsigned  B3SOIlprwgGiven   :1;
    unsigned  B3SOIlwrGiven   :1;
    unsigned  B3SOIlnfactorGiven   :1;      
    unsigned  B3SOIldwgGiven   :1;
    unsigned  B3SOIldwbGiven   :1;
    unsigned  B3SOIlvoffGiven   :1;
    unsigned  B3SOIleta0Given   :1;         
    unsigned  B3SOIletabGiven   :1;         
    unsigned  B3SOIldsubGiven   :1;      
    unsigned  B3SOIlcitGiven   :1;           
    unsigned  B3SOIlcdscGiven   :1;           
    unsigned  B3SOIlcdscbGiven   :1; 
    unsigned  B3SOIlcdscdGiven   :1;          
    unsigned  B3SOIlpclmGiven   :1;      
    unsigned  B3SOIlpdibl1Given   :1;      
    unsigned  B3SOIlpdibl2Given   :1;      
    unsigned  B3SOIlpdiblbGiven   :1;      
    unsigned  B3SOIldroutGiven   :1;      
    unsigned  B3SOIlpvagGiven   :1;       
    unsigned  B3SOIldeltaGiven   :1;
    unsigned  B3SOIlalpha0Given   :1;
    unsigned  B3SOIlfbjtiiGiven   :1;
    unsigned  B3SOIlbeta0Given   :1;
    unsigned  B3SOIlbeta1Given   :1;
    unsigned  B3SOIlbeta2Given   :1;         
    unsigned  B3SOIlvdsatii0Given   :1;     
    unsigned  B3SOIlliiGiven   :1;      
    unsigned  B3SOIlesatiiGiven   :1;     
    unsigned  B3SOIlsii0Given   :1;      
    unsigned  B3SOIlsii1Given   :1;       
    unsigned  B3SOIlsii2Given   :1;       
    unsigned  B3SOIlsiidGiven   :1;
    unsigned  B3SOIlagidlGiven   :1;
    unsigned  B3SOIlbgidlGiven   :1;
    unsigned  B3SOIlngidlGiven   :1;
    unsigned  B3SOIlntunGiven   :1;
    unsigned  B3SOIlndiodeGiven   :1;
    unsigned  B3SOIlnrecf0Given   :1;
    unsigned  B3SOIlnrecr0Given   :1;
    unsigned  B3SOIlisbjtGiven   :1;       
    unsigned  B3SOIlisdifGiven   :1;
    unsigned  B3SOIlisrecGiven   :1;       
    unsigned  B3SOIlistunGiven   :1;       
    unsigned  B3SOIlvrec0Given   :1;
    unsigned  B3SOIlvtun0Given   :1;
    unsigned  B3SOIlnbjtGiven   :1;
    unsigned  B3SOIllbjt0Given   :1;
    unsigned  B3SOIlvabjtGiven   :1;
    unsigned  B3SOIlaelyGiven   :1;
    unsigned  B3SOIlahliGiven   :1;
/* v3.1 wanh added for RF */
    unsigned B3SOIlxrcrg1Given  :1;
    unsigned B3SOIlxrcrg2Given  :1;
/* v3.1 wanh added for RF end */

    /* CV model */
    unsigned  B3SOIlvsdfbGiven   :1;
    unsigned  B3SOIlvsdthGiven   :1;
    unsigned  B3SOIldelvtGiven   :1;
    unsigned  B3SOIlacdeGiven   :1;
    unsigned  B3SOIlmoinGiven   :1;
    unsigned  B3SOIlnoffGiven   :1; /* v3.2 */

    /* Width Dependence */
/* v3.1 */
    unsigned B3SOIwxjGiven :1;
    unsigned B3SOIwalphaGB1Given :1;
    unsigned B3SOIwbetaGB1Given :1;
    unsigned B3SOIwalphaGB2Given :1;
    unsigned B3SOIwbetaGB2Given :1;
    unsigned B3SOIwndifGiven :1;
    unsigned B3SOIwntrecfGiven :1;
    unsigned B3SOIwntrecrGiven :1;
    unsigned B3SOIwxbjtGiven :1;
    unsigned B3SOIwxdifGiven :1;
    unsigned B3SOIwxrecGiven :1;
    unsigned B3SOIwxtunGiven :1;
    unsigned B3SOIwcgslGiven :1;
    unsigned B3SOIwcgdlGiven :1;
    unsigned B3SOIwckappaGiven :1;
    unsigned B3SOIwua1Given :1;
    unsigned B3SOIwub1Given :1;
    unsigned B3SOIwuc1Given :1;
    unsigned B3SOIwuteGiven :1;
    unsigned B3SOIwkt1Given :1;
    unsigned B3SOIwkt1lGiven :1;
    unsigned B3SOIwkt2Given :1;
    unsigned B3SOIwatGiven :1;
    unsigned B3SOIwprtGiven :1;

/* v3.0 */
    unsigned  B3SOIwaigcGiven   :1;
    unsigned  B3SOIwbigcGiven   :1;
    unsigned  B3SOIwcigcGiven   :1;
    unsigned  B3SOIwaigsdGiven   :1;
    unsigned  B3SOIwbigsdGiven   :1;
    unsigned  B3SOIwcigsdGiven   :1;   
    unsigned  B3SOIwnigcGiven   :1;
    unsigned  B3SOIwpigcdGiven   :1;
    unsigned  B3SOIwpoxedgeGiven   :1;

    unsigned  B3SOIwnpeakGiven   :1;        
    unsigned  B3SOIwnsubGiven   :1;
    unsigned  B3SOIwngateGiven   :1;        
    unsigned  B3SOIwvth0Given   :1;
    unsigned  B3SOIwk1Given   :1;
    unsigned  B3SOIwk1w1Given   :1;
    unsigned  B3SOIwk1w2Given   :1;
    unsigned  B3SOIwk2Given   :1;
    unsigned  B3SOIwk3Given   :1;
    unsigned  B3SOIwk3bGiven   :1;
    unsigned  B3SOIwkb1Given   :1;
    unsigned  B3SOIww0Given   :1;
    unsigned  B3SOIwnlxGiven   :1;
    unsigned  B3SOIwdvt0Given   :1;      
    unsigned  B3SOIwdvt1Given   :1;      
    unsigned  B3SOIwdvt2Given   :1;      
    unsigned  B3SOIwdvt0wGiven   :1;      
    unsigned  B3SOIwdvt1wGiven   :1;      
    unsigned  B3SOIwdvt2wGiven   :1;      
    unsigned  B3SOIwu0Given   :1;
    unsigned  B3SOIwuaGiven   :1;
    unsigned  B3SOIwubGiven   :1;
    unsigned  B3SOIwucGiven   :1;
    unsigned  B3SOIwvsatGiven   :1;         
    unsigned  B3SOIwa0Given   :1;   
    unsigned  B3SOIwagsGiven   :1;      
    unsigned  B3SOIwb0Given   :1;
    unsigned  B3SOIwb1Given   :1;
    unsigned  B3SOIwketaGiven   :1;
    unsigned  B3SOIwketasGiven   :1;
    unsigned  B3SOIwa1Given   :1;         
    unsigned  B3SOIwa2Given   :1;         
    unsigned  B3SOIwrdswGiven   :1;       
    unsigned  B3SOIwprwbGiven   :1;
    unsigned  B3SOIwprwgGiven   :1;
    unsigned  B3SOIwwrGiven   :1;
    unsigned  B3SOIwnfactorGiven   :1;      
    unsigned  B3SOIwdwgGiven   :1;
    unsigned  B3SOIwdwbGiven   :1;
    unsigned  B3SOIwvoffGiven   :1;
    unsigned  B3SOIweta0Given   :1;         
    unsigned  B3SOIwetabGiven   :1;         
    unsigned  B3SOIwdsubGiven   :1;      
    unsigned  B3SOIwcitGiven   :1;           
    unsigned  B3SOIwcdscGiven   :1;           
    unsigned  B3SOIwcdscbGiven   :1; 
    unsigned  B3SOIwcdscdGiven   :1;          
    unsigned  B3SOIwpclmGiven   :1;      
    unsigned  B3SOIwpdibl1Given   :1;      
    unsigned  B3SOIwpdibl2Given   :1;      
    unsigned  B3SOIwpdiblbGiven   :1;      
    unsigned  B3SOIwdroutGiven   :1;      
    unsigned  B3SOIwpvagGiven   :1;       
    unsigned  B3SOIwdeltaGiven   :1;
    unsigned  B3SOIwalpha0Given   :1;
    unsigned  B3SOIwfbjtiiGiven   :1;
    unsigned  B3SOIwbeta0Given   :1;
    unsigned  B3SOIwbeta1Given   :1;
    unsigned  B3SOIwbeta2Given   :1;         
    unsigned  B3SOIwvdsatii0Given   :1;     
    unsigned  B3SOIwliiGiven   :1;      
    unsigned  B3SOIwesatiiGiven   :1;     
    unsigned  B3SOIwsii0Given   :1;      
    unsigned  B3SOIwsii1Given   :1;       
    unsigned  B3SOIwsii2Given   :1;       
    unsigned  B3SOIwsiidGiven   :1;
    unsigned  B3SOIwagidlGiven   :1;
    unsigned  B3SOIwbgidlGiven   :1;
    unsigned  B3SOIwngidlGiven   :1;
    unsigned  B3SOIwntunGiven   :1;
    unsigned  B3SOIwndiodeGiven   :1;
    unsigned  B3SOIwnrecf0Given   :1;
    unsigned  B3SOIwnrecr0Given   :1;
    unsigned  B3SOIwisbjtGiven   :1;       
    unsigned  B3SOIwisdifGiven   :1;
    unsigned  B3SOIwisrecGiven   :1;       
    unsigned  B3SOIwistunGiven   :1;       
    unsigned  B3SOIwvrec0Given   :1;
    unsigned  B3SOIwvtun0Given   :1;
    unsigned  B3SOIwnbjtGiven   :1;
    unsigned  B3SOIwlbjt0Given   :1;
    unsigned  B3SOIwvabjtGiven   :1;
    unsigned  B3SOIwaelyGiven   :1;
    unsigned  B3SOIwahliGiven   :1;
/* v3.1 wanh added for RF */
    unsigned B3SOIwxrcrg1Given  :1;
    unsigned B3SOIwxrcrg2Given  :1;
/* v3.1 wanh added for RF end */

    /* CV model */
    unsigned  B3SOIwvsdfbGiven   :1;
    unsigned  B3SOIwvsdthGiven   :1;
    unsigned  B3SOIwdelvtGiven   :1;
    unsigned  B3SOIwacdeGiven   :1;
    unsigned  B3SOIwmoinGiven   :1;
    unsigned  B3SOIwnoffGiven   :1; /* v3.2 */

    /* Cross-term Dependence */
/* v3.1 */
    unsigned B3SOIpxjGiven :1;
    unsigned B3SOIpalphaGB1Given :1;
    unsigned B3SOIpbetaGB1Given :1;
    unsigned B3SOIpalphaGB2Given :1;
    unsigned B3SOIpbetaGB2Given :1;
    unsigned B3SOIpndifGiven :1;
    unsigned B3SOIpntrecfGiven :1;
    unsigned B3SOIpntrecrGiven :1;
    unsigned B3SOIpxbjtGiven :1;
    unsigned B3SOIpxdifGiven :1;
    unsigned B3SOIpxrecGiven :1;
    unsigned B3SOIpxtunGiven :1;
    unsigned B3SOIpcgslGiven :1;
    unsigned B3SOIpcgdlGiven :1;
    unsigned B3SOIpckappaGiven :1;
    unsigned B3SOIpua1Given :1;
    unsigned B3SOIpub1Given :1;
    unsigned B3SOIpuc1Given :1;
    unsigned B3SOIputeGiven :1;
    unsigned B3SOIpkt1Given :1;
    unsigned B3SOIpkt1lGiven :1;
    unsigned B3SOIpkt2Given :1;
    unsigned B3SOIpatGiven :1;
    unsigned B3SOIpprtGiven :1;

/* v3.0 */
    unsigned  B3SOIpaigcGiven   :1;
    unsigned  B3SOIpbigcGiven   :1;
    unsigned  B3SOIpcigcGiven   :1;
    unsigned  B3SOIpaigsdGiven   :1;
    unsigned  B3SOIpbigsdGiven   :1;
    unsigned  B3SOIpcigsdGiven   :1;   
    unsigned  B3SOIpnigcGiven   :1;
    unsigned  B3SOIppigcdGiven   :1;
    unsigned  B3SOIppoxedgeGiven   :1;

    unsigned  B3SOIpnpeakGiven   :1;        
    unsigned  B3SOIpnsubGiven   :1;
    unsigned  B3SOIpngateGiven   :1;        
    unsigned  B3SOIpvth0Given   :1;
    unsigned  B3SOIpk1Given   :1;
    unsigned  B3SOIpk1w1Given   :1;
    unsigned  B3SOIpk1w2Given   :1;
    unsigned  B3SOIpk2Given   :1;
    unsigned  B3SOIpk3Given   :1;
    unsigned  B3SOIpk3bGiven   :1;
    unsigned  B3SOIpkb1Given   :1;
    unsigned  B3SOIpw0Given   :1;
    unsigned  B3SOIpnlxGiven   :1;
    unsigned  B3SOIpdvt0Given   :1;      
    unsigned  B3SOIpdvt1Given   :1;      
    unsigned  B3SOIpdvt2Given   :1;      
    unsigned  B3SOIpdvt0wGiven   :1;      
    unsigned  B3SOIpdvt1wGiven   :1;      
    unsigned  B3SOIpdvt2wGiven   :1;      
    unsigned  B3SOIpu0Given   :1;
    unsigned  B3SOIpuaGiven   :1;
    unsigned  B3SOIpubGiven   :1;
    unsigned  B3SOIpucGiven   :1;
    unsigned  B3SOIpvsatGiven   :1;         
    unsigned  B3SOIpa0Given   :1;   
    unsigned  B3SOIpagsGiven   :1;      
    unsigned  B3SOIpb0Given   :1;
    unsigned  B3SOIpb1Given   :1;
    unsigned  B3SOIpketaGiven   :1;
    unsigned  B3SOIpketasGiven   :1;
    unsigned  B3SOIpa1Given   :1;         
    unsigned  B3SOIpa2Given   :1;         
    unsigned  B3SOIprdswGiven   :1;       
    unsigned  B3SOIpprwbGiven   :1;
    unsigned  B3SOIpprwgGiven   :1;
    unsigned  B3SOIpwrGiven   :1;
    unsigned  B3SOIpnfactorGiven   :1;      
    unsigned  B3SOIpdwgGiven   :1;
    unsigned  B3SOIpdwbGiven   :1;
    unsigned  B3SOIpvoffGiven   :1;
    unsigned  B3SOIpeta0Given   :1;         
    unsigned  B3SOIpetabGiven   :1;         
    unsigned  B3SOIpdsubGiven   :1;      
    unsigned  B3SOIpcitGiven   :1;           
    unsigned  B3SOIpcdscGiven   :1;           
    unsigned  B3SOIpcdscbGiven   :1; 
    unsigned  B3SOIpcdscdGiven   :1;          
    unsigned  B3SOIppclmGiven   :1;      
    unsigned  B3SOIppdibl1Given   :1;      
    unsigned  B3SOIppdibl2Given   :1;      
    unsigned  B3SOIppdiblbGiven   :1;      
    unsigned  B3SOIpdroutGiven   :1;      
    unsigned  B3SOIppvagGiven   :1;       
    unsigned  B3SOIpdeltaGiven   :1;
    unsigned  B3SOIpalpha0Given   :1;
    unsigned  B3SOIpfbjtiiGiven   :1;
    unsigned  B3SOIpbeta0Given   :1;
    unsigned  B3SOIpbeta1Given   :1;
    unsigned  B3SOIpbeta2Given   :1;         
    unsigned  B3SOIpvdsatii0Given   :1;     
    unsigned  B3SOIpliiGiven   :1;      
    unsigned  B3SOIpesatiiGiven   :1;     
    unsigned  B3SOIpsii0Given   :1;      
    unsigned  B3SOIpsii1Given   :1;       
    unsigned  B3SOIpsii2Given   :1;       
    unsigned  B3SOIpsiidGiven   :1;
    unsigned  B3SOIpagidlGiven   :1;
    unsigned  B3SOIpbgidlGiven   :1;
    unsigned  B3SOIpngidlGiven   :1;
    unsigned  B3SOIpntunGiven   :1;
    unsigned  B3SOIpndiodeGiven   :1;
    unsigned  B3SOIpnrecf0Given   :1;
    unsigned  B3SOIpnrecr0Given   :1;
    unsigned  B3SOIpisbjtGiven   :1;       
    unsigned  B3SOIpisdifGiven   :1;
    unsigned  B3SOIpisrecGiven   :1;       
    unsigned  B3SOIpistunGiven   :1;       
    unsigned  B3SOIpvrec0Given   :1;
    unsigned  B3SOIpvtun0Given   :1;
    unsigned  B3SOIpnbjtGiven   :1;
    unsigned  B3SOIplbjt0Given   :1;
    unsigned  B3SOIpvabjtGiven   :1;
    unsigned  B3SOIpaelyGiven   :1;
    unsigned  B3SOIpahliGiven   :1;
/* v3.1 wanh added for RF */
    unsigned B3SOIpxrcrg1Given  :1;
    unsigned B3SOIpxrcrg2Given  :1;
/* v3.1 wanh added for RF end */

    /* CV model */
    unsigned  B3SOIpvsdfbGiven   :1;
    unsigned  B3SOIpvsdthGiven   :1;
    unsigned  B3SOIpdelvtGiven   :1;
    unsigned  B3SOIpacdeGiven   :1;
    unsigned  B3SOIpmoinGiven   :1;
    unsigned  B3SOIpnoffGiven   :1;/* v3.2 */

/* Added for binning - END2 */

    unsigned  B3SOIuseFringeGiven   :1;

    unsigned  B3SOItnomGiven   :1;
    unsigned  B3SOIcgsoGiven   :1;
    unsigned  B3SOIcgdoGiven   :1;
    unsigned  B3SOIcgeoGiven   :1;

    unsigned  B3SOIxpartGiven   :1;
    unsigned  B3SOIsheetResistanceGiven   :1;
    unsigned  B3SOIGatesidewallJctPotentialGiven   :1;
    unsigned  B3SOIbodyJctGateSideGradingCoeffGiven   :1;
    unsigned  B3SOIunitLengthGateSidewallJctCapGiven   :1;
    unsigned  B3SOIcsdeswGiven :1;

    unsigned  B3SOIoxideTrapDensityAGiven  :1;         
    unsigned  B3SOIoxideTrapDensityBGiven  :1;        
    unsigned  B3SOIoxideTrapDensityCGiven  :1;     
    unsigned  B3SOIemGiven  :1;     
    unsigned  B3SOIefGiven  :1;     
    unsigned  B3SOIafGiven  :1;     
    unsigned  B3SOIkfGiven  :1;     
    unsigned  B3SOInoifGiven  :1;     

    unsigned  B3SOILintGiven   :1;
    unsigned  B3SOILlGiven   :1;
    unsigned  B3SOILlcGiven   :1; /* v2.2.3 */
    unsigned  B3SOILlnGiven   :1;
    unsigned  B3SOILwGiven   :1;
    unsigned  B3SOILwcGiven   :1; /* v2.2.3 */
    unsigned  B3SOILwnGiven   :1;
    unsigned  B3SOILwlGiven   :1;
    unsigned  B3SOILwlcGiven   :1; /* v2.2.3 */
    unsigned  B3SOILminGiven   :1;
    unsigned  B3SOILmaxGiven   :1;

    unsigned  B3SOIWintGiven   :1;
    unsigned  B3SOIWlGiven   :1;
    unsigned  B3SOIWlcGiven   :1; /* v2.2.3 */
    unsigned  B3SOIWlnGiven   :1;
    unsigned  B3SOIWwGiven   :1;
    unsigned  B3SOIWwcGiven   :1; /* v2.2.3 */
    unsigned  B3SOIWwnGiven   :1;
    unsigned  B3SOIWwlGiven   :1;
    unsigned  B3SOIWwlcGiven   :1;  /* v2.2.3 */
    unsigned  B3SOIWminGiven   :1;
    unsigned  B3SOIWmaxGiven   :1;

};

} // namespace B3SOI32
using namespace B3SOI32;


#ifndef NMOS
#define NMOS 1
#define PMOS -1
#endif

// device parameters
enum {
    B3SOI_W = 1,
    B3SOI_L,
    B3SOI_AS,
    B3SOI_AD,
    B3SOI_PS,
    B3SOI_PD,
    B3SOI_NRS,
    B3SOI_NRD,
    B3SOI_OFF,
    B3SOI_IC_VBS,
    B3SOI_IC_VDS,
    B3SOI_IC_VGS,
    B3SOI_IC_VES,
    B3SOI_IC_VPS,
    B3SOI_BJTOFF,
    B3SOI_RTH0,
    B3SOI_CTH0,
    B3SOI_NRB,
    B3SOI_IC,
    B3SOI_NQSMOD,
    B3SOI_DEBUG,

    // v2.0 release
    B3SOI_NBC,
    B3SOI_NSEG,
    B3SOI_PDBCP,
    B3SOI_PSBCP,
    B3SOI_AGBCP,
    B3SOI_AEBCP,
    B3SOI_VBSUSR,
    B3SOI_TNODEOUT,

    // v2.2.2
    B3SOI_FRBODY,

    // v3.1 wanh added for RF
    B3SOI_RGATEMOD,
    // v3.1 wanh added for RF end

    // v3.2
    B3SOI_SOIMOD,
//    B3SOI_MOD_FNOIMOD,
//    B3SOI_MOD_TNOIMOD,

    B3SOI_DNODE,
    B3SOI_GNODE,
    B3SOI_SNODE,
    B3SOI_BNODE,
    B3SOI_ENODE,
    B3SOI_DNODEPRIME,
    B3SOI_SNODEPRIME,
    B3SOI_VBD,
    B3SOI_VBS,
    B3SOI_VGS,
    B3SOI_VES,
    B3SOI_VDS,
    B3SOI_CD,
    B3SOI_CBS,
    B3SOI_CBD,
    B3SOI_GM,
    B3SOI_GDS,
    B3SOI_GMBS,
    B3SOI_GBD,
    B3SOI_GBS,
    B3SOI_QB,
    B3SOI_CQB,
    B3SOI_QG,
    B3SOI_CQG,
    B3SOI_QD,
    B3SOI_CQD,
    B3SOI_CGG,
    B3SOI_CGD,
    B3SOI_CGS,
    B3SOI_CBG,
    B3SOI_CAPBD,
    B3SOI_CQBD,
    B3SOI_CAPBS,
    B3SOI_CQBS,
    B3SOI_CDG,
    B3SOI_CDD,
    B3SOI_CDS,
    B3SOI_VON,
    B3SOI_VDSAT,
    B3SOI_QBS,
    B3SOI_QBD,
    B3SOI_SOURCECONDUCT,
    B3SOI_DRAINCONDUCT,
    B3SOI_CBDB,
    B3SOI_CBSB,
    B3SOI_GMID,

    // v3.1 wanh added for RF
    B3SOI_GNODEEXT,
    B3SOI_GNODEMID,
    // v3.1 wanh added for RF end

    // SRW - added
    B3SOI_ID,
    B3SOI_IS,
    B3SOI_IG,
    B3SOI_IB,
    B3SOI_IE
};

// model parameters
enum {
    B3SOI_MOD_CAPMOD = 1000,
    B3SOI_MOD_NQSMOD,
    B3SOI_MOD_MOBMOD,
//    B3SOI_MOD_NOIMOD,
    B3SOI_MOD_SHMOD,
    B3SOI_MOD_DDMOD,

    B3SOI_MOD_TOX,

    B3SOI_MOD_CDSC,
    B3SOI_MOD_CDSCB,
    B3SOI_MOD_CIT,
    B3SOI_MOD_NFACTOR,
    B3SOI_MOD_XJ,
    B3SOI_MOD_VSAT,
    B3SOI_MOD_AT,
    B3SOI_MOD_A0,
    B3SOI_MOD_A1,
    B3SOI_MOD_A2,
    B3SOI_MOD_KETA,
    B3SOI_MOD_NSUB,
    B3SOI_MOD_NPEAK,
    B3SOI_MOD_NGATE,
    B3SOI_MOD_GAMMA1,
    B3SOI_MOD_GAMMA2,
    B3SOI_MOD_VBX,
    B3SOI_MOD_BINUNIT,

    B3SOI_MOD_VBM,

    B3SOI_MOD_XT,
    B3SOI_MOD_K1,
    B3SOI_MOD_KT1,
    B3SOI_MOD_KT1L,
    B3SOI_MOD_K2,
    B3SOI_MOD_KT2,
    B3SOI_MOD_K3,
    B3SOI_MOD_K3B,
    B3SOI_MOD_W0,
    B3SOI_MOD_NLX,

    B3SOI_MOD_DVT0,
    B3SOI_MOD_DVT1,
    B3SOI_MOD_DVT2,

    B3SOI_MOD_DVT0W,
    B3SOI_MOD_DVT1W,
    B3SOI_MOD_DVT2W,

    B3SOI_MOD_DROUT,
    B3SOI_MOD_DSUB,
    B3SOI_MOD_VTH0,
    B3SOI_MOD_UA,
    B3SOI_MOD_UA1,
    B3SOI_MOD_UB,
    B3SOI_MOD_UB1,
    B3SOI_MOD_UC,
    B3SOI_MOD_UC1,
    B3SOI_MOD_U0,
    B3SOI_MOD_UTE,
    B3SOI_MOD_VOFF,
    B3SOI_MOD_DELTA,
    B3SOI_MOD_RDSW,
    B3SOI_MOD_PRT,
    B3SOI_MOD_LDD,
    B3SOI_MOD_ETA,
    B3SOI_MOD_ETA0,
    B3SOI_MOD_ETAB,
    B3SOI_MOD_PCLM,
    B3SOI_MOD_PDIBL1,
    B3SOI_MOD_PDIBL2,
    B3SOI_MOD_PSCBE1,
    B3SOI_MOD_PSCBE2,
    B3SOI_MOD_PVAG,
    B3SOI_MOD_WR,
    B3SOI_MOD_DWG,
    B3SOI_MOD_DWB,
    B3SOI_MOD_B0,
    B3SOI_MOD_B1,
    B3SOI_MOD_ALPHA0,
    B3SOI_MOD_PDIBLB,

    B3SOI_MOD_PRWG,
    B3SOI_MOD_PRWB,

    B3SOI_MOD_CDSCD,
    B3SOI_MOD_AGS,

    B3SOI_MOD_FRINGE,
    B3SOI_MOD_CGSL,
    B3SOI_MOD_CGDL,
    B3SOI_MOD_CKAPPA,
    B3SOI_MOD_CF,
    B3SOI_MOD_CLC,
    B3SOI_MOD_CLE,
    B3SOI_MOD_PARAMCHK,
    B3SOI_MOD_VERSION,

    B3SOI_MOD_TBOX,
    B3SOI_MOD_TSI,
    B3SOI_MOD_KB1,
    B3SOI_MOD_KB3,
    B3SOI_MOD_DELP,
    B3SOI_MOD_RBODY,
    B3SOI_MOD_ADICE0,
    B3SOI_MOD_ABP,
    B3SOI_MOD_MXC,
    B3SOI_MOD_RTH0,
    B3SOI_MOD_CTH0,
    B3SOI_MOD_ALPHA1,
    B3SOI_MOD_NGIDL,
    B3SOI_MOD_AGIDL,
    B3SOI_MOD_BGIDL,
    B3SOI_MOD_NDIODE,
    B3SOI_MOD_LDIOF,
    B3SOI_MOD_LDIOR,
    B3SOI_MOD_NTUN,
    B3SOI_MOD_ISBJT,
    B3SOI_MOD_ISDIF,
    B3SOI_MOD_ISREC,
    B3SOI_MOD_ISTUN,
    B3SOI_MOD_XBJT,
    B3SOI_MOD_XDIF,
    B3SOI_MOD_XREC,
    B3SOI_MOD_XTUN,
    B3SOI_MOD_TT,
    B3SOI_MOD_VSDTH,
    B3SOI_MOD_VSDFB,
    B3SOI_MOD_ASD,
    B3SOI_MOD_CSDMIN,
    B3SOI_MOD_RBSH,
    B3SOI_MOD_ESATII,

    // v2.0 release
    B3SOI_MOD_K1W1,
    B3SOI_MOD_K1W2,
    B3SOI_MOD_KETAS,
    B3SOI_MOD_DWBC,
    B3SOI_MOD_BETA0,
    B3SOI_MOD_BETA1,
    B3SOI_MOD_BETA2,
    B3SOI_MOD_VDSATII0,
    B3SOI_MOD_TII,
    B3SOI_MOD_LII,
    B3SOI_MOD_SII0,
    B3SOI_MOD_SII1,
    B3SOI_MOD_SII2,
    B3SOI_MOD_SIID,
    B3SOI_MOD_FBJTII,
    B3SOI_MOD_NRECF0,
    B3SOI_MOD_NRECR0,
    B3SOI_MOD_LN,
    B3SOI_MOD_VREC0,
    B3SOI_MOD_VTUN0,
    B3SOI_MOD_NBJT,
    B3SOI_MOD_LBJT0,
    B3SOI_MOD_VABJT,
    B3SOI_MOD_AELY,
    B3SOI_MOD_AHLI,
    B3SOI_MOD_NTRECF,
    B3SOI_MOD_NTRECR,
    B3SOI_MOD_DLCB,
    B3SOI_MOD_FBODY,
    B3SOI_MOD_NDIF,
    B3SOI_MOD_TCJSWG,
    B3SOI_MOD_TPBSWG,
    B3SOI_MOD_ACDE,
    B3SOI_MOD_MOIN,
    B3SOI_MOD_DELVT,
    B3SOI_MOD_DLBG,
    B3SOI_MOD_LDIF0,

    // v2.2 release
    B3SOI_MOD_WTH0,
    B3SOI_MOD_RHALO,
    B3SOI_MOD_NTOX,
    B3SOI_MOD_TOXREF,
    B3SOI_MOD_EBG,
    B3SOI_MOD_VEVB,
    B3SOI_MOD_ALPHAGB1,
    B3SOI_MOD_BETAGB1,
    B3SOI_MOD_VGB1,
    B3SOI_MOD_VECB,
    B3SOI_MOD_ALPHAGB2,
    B3SOI_MOD_BETAGB2,
    B3SOI_MOD_VGB2,
    B3SOI_MOD_TOXQM,
    B3SOI_MOD_IGBMOD,
    B3SOI_MOD_VOXH,
    B3SOI_MOD_DELTAVOX,
    B3SOI_MOD_IGCMOD,

    // v3.1 wanh added for RF
    B3SOI_MOD_RGATEMOD,
    B3SOI_MOD_XRCRG1,
    B3SOI_MOD_XRCRG2,
    B3SOI_MOD_RSHG,
    B3SOI_MOD_NGCON,
    // v3.1 wanh added for RF end

    // Added for binning - START
    B3SOI_MOD_LNPEAK,
    B3SOI_MOD_LNSUB,
    B3SOI_MOD_LNGATE,
    B3SOI_MOD_LVTH0,
    B3SOI_MOD_LK1,
    B3SOI_MOD_LK1W1,
    B3SOI_MOD_LK1W2,
    B3SOI_MOD_LK2,
    B3SOI_MOD_LK3,
    B3SOI_MOD_LK3B,
    B3SOI_MOD_LKB1,
    B3SOI_MOD_LW0,
    B3SOI_MOD_LNLX,
    B3SOI_MOD_LDVT0,
    B3SOI_MOD_LDVT1,
    B3SOI_MOD_LDVT2,
    B3SOI_MOD_LDVT0W,
    B3SOI_MOD_LDVT1W,
    B3SOI_MOD_LDVT2W,
    B3SOI_MOD_LU0,
    B3SOI_MOD_LUA,
    B3SOI_MOD_LUB,
    B3SOI_MOD_LUC,
    B3SOI_MOD_LVSAT,
    B3SOI_MOD_LA0,
    B3SOI_MOD_LAGS,
    B3SOI_MOD_LB0,
    B3SOI_MOD_LB1,
    B3SOI_MOD_LKETA,
    B3SOI_MOD_LKETAS,
    B3SOI_MOD_LA1,
    B3SOI_MOD_LA2,
    B3SOI_MOD_LRDSW,
    B3SOI_MOD_LPRWB,
    B3SOI_MOD_LPRWG,
    B3SOI_MOD_LWR,
    B3SOI_MOD_LNFACTOR,
    B3SOI_MOD_LDWG,
    B3SOI_MOD_LDWB,
    B3SOI_MOD_LVOFF,
    B3SOI_MOD_LETA0,
    B3SOI_MOD_LETAB,
    B3SOI_MOD_LDSUB,
    B3SOI_MOD_LCIT,
    B3SOI_MOD_LCDSC,
    B3SOI_MOD_LCDSCB,
    B3SOI_MOD_LCDSCD,
    B3SOI_MOD_LPCLM,
    B3SOI_MOD_LPDIBL1,
    B3SOI_MOD_LPDIBL2,
    B3SOI_MOD_LPDIBLB,
    B3SOI_MOD_LDROUT,
    B3SOI_MOD_LPVAG,
    B3SOI_MOD_LDELTA,
    B3SOI_MOD_LALPHA0,
    B3SOI_MOD_LFBJTII,
    B3SOI_MOD_LBETA0,
    B3SOI_MOD_LBETA1,
    B3SOI_MOD_LBETA2,
    B3SOI_MOD_LVDSATII0,
    B3SOI_MOD_LLII,
    B3SOI_MOD_LESATII,
    B3SOI_MOD_LSII0,
    B3SOI_MOD_LSII1,
    B3SOI_MOD_LSII2,
    B3SOI_MOD_LSIID,
    B3SOI_MOD_LAGIDL,
    B3SOI_MOD_LBGIDL,
    B3SOI_MOD_LNGIDL,
    B3SOI_MOD_LNTUN,
    B3SOI_MOD_LNDIODE,
    B3SOI_MOD_LNRECF0,
    B3SOI_MOD_LNRECR0,
    B3SOI_MOD_LISBJT,
    B3SOI_MOD_LISDIF,
    B3SOI_MOD_LISREC,
    B3SOI_MOD_LISTUN,
    B3SOI_MOD_LVREC0,
    B3SOI_MOD_LVTUN0,
    B3SOI_MOD_LNBJT,
    B3SOI_MOD_LLBJT0,
    B3SOI_MOD_LVABJT,
    B3SOI_MOD_LAELY,
    B3SOI_MOD_LAHLI,
    B3SOI_MOD_LVSDFB,
    B3SOI_MOD_LVSDTH,
    B3SOI_MOD_LDELVT,
    B3SOI_MOD_LACDE,
    B3SOI_MOD_LMOIN,

    // v3.1 wanh added for RF
    B3SOI_MOD_LXRCRG1,
    B3SOI_MOD_LXRCRG2,
    B3SOI_MOD_XGW,
    B3SOI_MOD_XGL,
    // v3.1 wanh added for RF end

    B3SOI_MOD_WNPEAK,
    B3SOI_MOD_WNSUB,
    B3SOI_MOD_WNGATE,
    B3SOI_MOD_WVTH0,
    B3SOI_MOD_WK1,
    B3SOI_MOD_WK1W1,
    B3SOI_MOD_WK1W2,
    B3SOI_MOD_WK2,
    B3SOI_MOD_WK3,
    B3SOI_MOD_WK3B,
    B3SOI_MOD_WKB1,
    B3SOI_MOD_WW0,
    B3SOI_MOD_WNLX,
    B3SOI_MOD_WDVT0,
    B3SOI_MOD_WDVT1,
    B3SOI_MOD_WDVT2,
    B3SOI_MOD_WDVT0W,
    B3SOI_MOD_WDVT1W,
    B3SOI_MOD_WDVT2W,
    B3SOI_MOD_WU0,
    B3SOI_MOD_WUA,
    B3SOI_MOD_WUB,
    B3SOI_MOD_WUC,
    B3SOI_MOD_WVSAT,
    B3SOI_MOD_WA0,
    B3SOI_MOD_WAGS,
    B3SOI_MOD_WB0,
    B3SOI_MOD_WB1,
    B3SOI_MOD_WKETA,
    B3SOI_MOD_WKETAS,
    B3SOI_MOD_WA1,
    B3SOI_MOD_WA2,
    B3SOI_MOD_WRDSW,
    B3SOI_MOD_WPRWB,
    B3SOI_MOD_WPRWG,
    B3SOI_MOD_WWR,
    B3SOI_MOD_WNFACTOR,
    B3SOI_MOD_WDWG,
    B3SOI_MOD_WDWB,
    B3SOI_MOD_WVOFF,
    B3SOI_MOD_WETA0,
    B3SOI_MOD_WETAB,
    B3SOI_MOD_WDSUB,
    B3SOI_MOD_WCIT,
    B3SOI_MOD_WCDSC,
    B3SOI_MOD_WCDSCB,
    B3SOI_MOD_WCDSCD,
    B3SOI_MOD_WPCLM,
    B3SOI_MOD_WPDIBL1,
    B3SOI_MOD_WPDIBL2,
    B3SOI_MOD_WPDIBLB,
    B3SOI_MOD_WDROUT,
    B3SOI_MOD_WPVAG,
    B3SOI_MOD_WDELTA,
    B3SOI_MOD_WALPHA0,
    B3SOI_MOD_WFBJTII,
    B3SOI_MOD_WBETA0,
    B3SOI_MOD_WBETA1,
    B3SOI_MOD_WBETA2,
    B3SOI_MOD_WVDSATII0,
    B3SOI_MOD_WLII,
    B3SOI_MOD_WESATII,
    B3SOI_MOD_WSII0,
    B3SOI_MOD_WSII1,
    B3SOI_MOD_WSII2,
    B3SOI_MOD_WSIID,
    B3SOI_MOD_WAGIDL,
    B3SOI_MOD_WBGIDL,
    B3SOI_MOD_WNGIDL,
    B3SOI_MOD_WNTUN,
    B3SOI_MOD_WNDIODE,
    B3SOI_MOD_WNRECF0,
    B3SOI_MOD_WNRECR0,
    B3SOI_MOD_WISBJT,
    B3SOI_MOD_WISDIF,
    B3SOI_MOD_WISREC,
    B3SOI_MOD_WISTUN,
    B3SOI_MOD_WVREC0,
    B3SOI_MOD_WVTUN0,
    B3SOI_MOD_WNBJT,
    B3SOI_MOD_WLBJT0,
    B3SOI_MOD_WVABJT,
    B3SOI_MOD_WAELY,
    B3SOI_MOD_WAHLI,
    B3SOI_MOD_WVSDFB,
    B3SOI_MOD_WVSDTH,
    B3SOI_MOD_WDELVT,
    B3SOI_MOD_WACDE,
    B3SOI_MOD_WMOIN,

    // v3.1 wanh added for RF
    B3SOI_MOD_WXRCRG1,
    B3SOI_MOD_WXRCRG2,
    // v3.1 wanh added for RF end

    B3SOI_MOD_PNPEAK,
    B3SOI_MOD_PNSUB,
    B3SOI_MOD_PNGATE,
    B3SOI_MOD_PVTH0,
    B3SOI_MOD_PK1,
    B3SOI_MOD_PK1W1,
    B3SOI_MOD_PK1W2,
    B3SOI_MOD_PK2,
    B3SOI_MOD_PK3,
    B3SOI_MOD_PK3B,
    B3SOI_MOD_PKB1,
    B3SOI_MOD_PW0,
    B3SOI_MOD_PNLX,
    B3SOI_MOD_PDVT0,
    B3SOI_MOD_PDVT1,
    B3SOI_MOD_PDVT2,
    B3SOI_MOD_PDVT0W,
    B3SOI_MOD_PDVT1W,
    B3SOI_MOD_PDVT2W,
    B3SOI_MOD_PU0,
    B3SOI_MOD_PUA,
    B3SOI_MOD_PUB,
    B3SOI_MOD_PUC,
    B3SOI_MOD_PVSAT,
    B3SOI_MOD_PA0,
    B3SOI_MOD_PAGS,
    B3SOI_MOD_PB0,
    B3SOI_MOD_PB1,
    B3SOI_MOD_PKETA,
    B3SOI_MOD_PKETAS,
    B3SOI_MOD_PA1,
    B3SOI_MOD_PA2,
    B3SOI_MOD_PRDSW,
    B3SOI_MOD_PPRWB,
    B3SOI_MOD_PPRWG,
    B3SOI_MOD_PWR,
    B3SOI_MOD_PNFACTOR,
    B3SOI_MOD_PDWG,
    B3SOI_MOD_PDWB,
    B3SOI_MOD_PVOFF,
    B3SOI_MOD_PETA0,
    B3SOI_MOD_PETAB,
    B3SOI_MOD_PDSUB,
    B3SOI_MOD_PCIT,
    B3SOI_MOD_PCDSC,
    B3SOI_MOD_PCDSCB,
    B3SOI_MOD_PCDSCD,
    B3SOI_MOD_PPCLM,
    B3SOI_MOD_PPDIBL1,
    B3SOI_MOD_PPDIBL2,
    B3SOI_MOD_PPDIBLB,
    B3SOI_MOD_PDROUT,
    B3SOI_MOD_PPVAG,
    B3SOI_MOD_PDELTA,
    B3SOI_MOD_PALPHA0,
    B3SOI_MOD_PFBJTII,
    B3SOI_MOD_PBETA0,
    B3SOI_MOD_PBETA1,
    B3SOI_MOD_PBETA2,
    B3SOI_MOD_PVDSATII0,
    B3SOI_MOD_PLII,
    B3SOI_MOD_PESATII,
    B3SOI_MOD_PSII0,
    B3SOI_MOD_PSII1,
    B3SOI_MOD_PSII2,
    B3SOI_MOD_PSIID,
    B3SOI_MOD_PAGIDL,
    B3SOI_MOD_PBGIDL,
    B3SOI_MOD_PNGIDL,
    B3SOI_MOD_PNTUN,
    B3SOI_MOD_PNDIODE,
    B3SOI_MOD_PNRECF0,
    B3SOI_MOD_PNRECR0,
    B3SOI_MOD_PISBJT,
    B3SOI_MOD_PISDIF,
    B3SOI_MOD_PISREC,
    B3SOI_MOD_PISTUN,
    B3SOI_MOD_PVREC0,
    B3SOI_MOD_PVTUN0,
    B3SOI_MOD_PNBJT,
    B3SOI_MOD_PLBJT0,
    B3SOI_MOD_PVABJT,
    B3SOI_MOD_PAELY,
    B3SOI_MOD_PAHLI,
    B3SOI_MOD_PVSDFB,
    B3SOI_MOD_PVSDTH,
    B3SOI_MOD_PDELVT,
    B3SOI_MOD_PACDE,
    B3SOI_MOD_PMOIN,

    // v3.1 wanh added for RF
    B3SOI_MOD_PXRCRG1,
    B3SOI_MOD_PXRCRG2,
    // v3.1 wanh added for RF end

    // Added for binning - END

    B3SOI_MOD_TNOM,
    B3SOI_MOD_CGSO,
    B3SOI_MOD_CGDO,
    B3SOI_MOD_CGEO,
    B3SOI_MOD_XPART,
    B3SOI_MOD_RSH,

    B3SOI_MOD_NMOS,
    B3SOI_MOD_PMOS,

    B3SOI_MOD_NOIA,
    B3SOI_MOD_NOIB,
    B3SOI_MOD_NOIC,

    B3SOI_MOD_LINT,
    B3SOI_MOD_LL,
    B3SOI_MOD_LLN,
    B3SOI_MOD_LW,
    B3SOI_MOD_LWN,
    B3SOI_MOD_LWL,

    B3SOI_MOD_WINT,
    B3SOI_MOD_WL,
    B3SOI_MOD_WLN,
    B3SOI_MOD_WW,
    B3SOI_MOD_WWN,
    B3SOI_MOD_WWL,

    // v2.2.3
    B3SOI_MOD_LWLC,
    B3SOI_MOD_LLC,
    B3SOI_MOD_LWC,
    B3SOI_MOD_WWLC,
    B3SOI_MOD_WLC,
    B3SOI_MOD_WWC,
    B3SOI_MOD_DTOXCV,

    B3SOI_MOD_DWC,
    B3SOI_MOD_DLC,

    B3SOI_MOD_EM,
    B3SOI_MOD_EF,
    B3SOI_MOD_AF,
    B3SOI_MOD_KF,
    B3SOI_MOD_NOIF,

    B3SOI_MOD_PBSWG,
    B3SOI_MOD_MJSWG,
    B3SOI_MOD_CJSWG,
    B3SOI_MOD_CSDESW,

    // v3.2
    B3SOI_MOD_TNOIA,
    B3SOI_MOD_TNOIB,
    B3SOI_MOD_RNOIA,
    B3SOI_MOD_RNOIB,
    B3SOI_MOD_NTNOI,
    B3SOI_MOD_FNOIMOD,
    B3SOI_MOD_TNOIMOD,
    B3SOI_MOD_NOFF,
    B3SOI_MOD_LNOFF,
    B3SOI_MOD_WNOFF,
    B3SOI_MOD_PNOFF,
    B3SOI_MOD_TOXM,
    B3SOI_MOD_VBS0PD,
    B3SOI_MOD_VBS0FD,

    // v3.0
    B3SOI_MOD_SOIMOD,
    B3SOI_MOD_VBSA,
    B3SOI_MOD_NOFFFD,
    B3SOI_MOD_VOFFFD,
    B3SOI_MOD_K1B,
    B3SOI_MOD_K2B,
    B3SOI_MOD_DK2B,
    B3SOI_MOD_DVBD0,
    B3SOI_MOD_DVBD1,
    B3SOI_MOD_MOINFD,

    // v3.0
    B3SOI_MOD_AIGC,
    B3SOI_MOD_BIGC,
    B3SOI_MOD_CIGC,
    B3SOI_MOD_AIGSD,
    B3SOI_MOD_BIGSD,
    B3SOI_MOD_CIGSD,
    B3SOI_MOD_NIGC,
    B3SOI_MOD_PIGCD,
    B3SOI_MOD_POXEDGE,
    B3SOI_MOD_DLCIG,

    B3SOI_MOD_LAIGC,
    B3SOI_MOD_LBIGC,
    B3SOI_MOD_LCIGC,
    B3SOI_MOD_LAIGSD,
    B3SOI_MOD_LBIGSD,
    B3SOI_MOD_LCIGSD,
    B3SOI_MOD_LNIGC,
    B3SOI_MOD_LPIGCD,
    B3SOI_MOD_LPOXEDGE,

    B3SOI_MOD_WAIGC,
    B3SOI_MOD_WBIGC,
    B3SOI_MOD_WCIGC,
    B3SOI_MOD_WAIGSD,
    B3SOI_MOD_WBIGSD,
    B3SOI_MOD_WCIGSD,
    B3SOI_MOD_WNIGC,
    B3SOI_MOD_WPIGCD,
    B3SOI_MOD_WPOXEDGE,

    B3SOI_MOD_PAIGC,
    B3SOI_MOD_PBIGC,
    B3SOI_MOD_PCIGC,
    B3SOI_MOD_PAIGSD,
    B3SOI_MOD_PBIGSD,
    B3SOI_MOD_PCIGSD,
    B3SOI_MOD_PNIGC,
    B3SOI_MOD_PPIGCD,
    B3SOI_MOD_PPOXEDGE,

    // v3.1
    B3SOI_MOD_LXJ,
    B3SOI_MOD_LALPHAGB1,
    B3SOI_MOD_LALPHAGB2,
    B3SOI_MOD_LBETAGB1,
    B3SOI_MOD_LBETAGB2,
    B3SOI_MOD_LNDIF,
    B3SOI_MOD_LNTRECF,
    B3SOI_MOD_LNTRECR,
    B3SOI_MOD_LXBJT,
    B3SOI_MOD_LXDIF,
    B3SOI_MOD_LXREC,
    B3SOI_MOD_LXTUN,
    B3SOI_MOD_LCGDL,
    B3SOI_MOD_LCGSL,
    B3SOI_MOD_LCKAPPA,
    B3SOI_MOD_LUTE,
    B3SOI_MOD_LKT1,
    B3SOI_MOD_LKT2,
    B3SOI_MOD_LKT1L,
    B3SOI_MOD_LUA1,
    B3SOI_MOD_LUB1,
    B3SOI_MOD_LUC1,
    B3SOI_MOD_LAT,
    B3SOI_MOD_LPRT,

    B3SOI_MOD_WXJ,
    B3SOI_MOD_WALPHAGB1,
    B3SOI_MOD_WALPHAGB2,
    B3SOI_MOD_WBETAGB1,
    B3SOI_MOD_WBETAGB2,
    B3SOI_MOD_WNDIF,
    B3SOI_MOD_WNTRECF,
    B3SOI_MOD_WNTRECR,
    B3SOI_MOD_WXBJT,
    B3SOI_MOD_WXDIF,
    B3SOI_MOD_WXREC,
    B3SOI_MOD_WXTUN,
    B3SOI_MOD_WCGDL,
    B3SOI_MOD_WCGSL,
    B3SOI_MOD_WCKAPPA,
    B3SOI_MOD_WUTE,
    B3SOI_MOD_WKT1,
    B3SOI_MOD_WKT2,
    B3SOI_MOD_WKT1L,
    B3SOI_MOD_WUA1,
    B3SOI_MOD_WUB1,
    B3SOI_MOD_WUC1,
    B3SOI_MOD_WAT,
    B3SOI_MOD_WPRT,

    B3SOI_MOD_PXJ,
    B3SOI_MOD_PALPHAGB1,
    B3SOI_MOD_PALPHAGB2,
    B3SOI_MOD_PBETAGB1,
    B3SOI_MOD_PBETAGB2,
    B3SOI_MOD_PNDIF,
    B3SOI_MOD_PNTRECF,
    B3SOI_MOD_PNTRECR,
    B3SOI_MOD_PXBJT,
    B3SOI_MOD_PXDIF,
    B3SOI_MOD_PXREC,
    B3SOI_MOD_PXTUN,
    B3SOI_MOD_PCGDL,
    B3SOI_MOD_PCGSL,
    B3SOI_MOD_PCKAPPA,
    B3SOI_MOD_PUTE,
    B3SOI_MOD_PKT1,
    B3SOI_MOD_PKT2,
    B3SOI_MOD_PKT1L,
    B3SOI_MOD_PUA1,
    B3SOI_MOD_PUB1,
    B3SOI_MOD_PUC1,
    B3SOI_MOD_PAT,
    B3SOI_MOD_PPRT
};

#endif // B3SDEFS_H

