
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
Copyright 1997 University of Florida.  All rights reserved.
Author: Min-Chie Jeng (For SPICE3E2)
File: ufsdef.h
**********/

#ifndef UFSDEFS_H
#define UFSDEFS_H

#include "device.h"
extern "C" {
#include "ufs_api.h"         
}
#ifndef NULL
#define NULL 0
#endif

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

#define CKTreltol CKTcurTask->TSKreltol
#define CKTabstol CKTcurTask->TSKabstol
#define CKTvoltTol CKTcurTask->TSKvoltTol
#define CKTtemp CKTcurTask->TSKtemp
#define CKTnomTemp CKTcurTask->TSKnomTemp
#define CKTgmin CKTcurTask->TSKgmin
#define CKTbypass CKTcurTask->TSKbypass

//
// structures used to describe UFS-MOSFET
//

namespace UFS75 {

struct UFSdev : public IFdevice
{
    UFSdev();
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
    int accept(sCKT*, sGENmodel*); 
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
    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
};

struct sUFSinstance : public sGENinstance
{
    sUFSinstance()
        {
            memset(this, 0, sizeof(sUFSinstance));
            GENnumNodes = 5;

            // The sub-structs are normally allocated on the first call to
            // setInst().  We do it here instead.
            pInst = new ufsAPI_InstData;
            pOpInfo = new ufsAPI_OPData;
            pInst->pTempModel = new ufsTDModelData;
            ufsInitInstFlag(pInst);
            DeviceInitialized = 1;
        }
    ~sUFSinstance()
        {
            if (pInst)
                delete pInst->pTempModel;
            delete pInst;
            delete pOpInfo;
        }
    sUFSinstance *next()
        { return (static_cast<sUFSinstance*>(GENnextInstance)); }

//    struct sUFSmodel *UFSmodPtr;
//    struct sUFSinstance *UFSnextInstance;
//    IFuid UFSname;
#define UFSmodPtr (sUFSmodel*)GENmodel
#define UFSnextInstance next()
#define UFSname GENname

    int UFSdNode;
    int UFSgNode;
    int UFSsNode;
    int UFSbgNode;
    int UFSbNode;

    int UFSdNodePrime;
    int UFSsNodePrime;
    int UFSbNodePrime;
    int UFStNode;
    struct ufsAPI_InstData *pInst;
    struct ufsAPI_OPData  *pOpInfo;

    int UFSoff;
    double UFSicVBS;
    double UFSicVDS;
    double UFSicVGFS;
    double UFSicVGBS;

    unsigned UFSicVBSGiven :1;
    unsigned UFSicVDSGiven :1;
    unsigned UFSicVGFSGiven :1;
    unsigned UFSicVGBSGiven :1;
    unsigned DeviceInitialized :1;

    double *UFSGgPtr;                
    double *UFSGdpPtr;                
    double *UFSGspPtr;                
    double *UFSGbpPtr;                
    double *UFSGgbPtr;                
    double *UFSDPgPtr;                
    double *UFSDPdpPtr;
    double *UFSDPspPtr;
    double *UFSDPbpPtr;
    double *UFSDPgbPtr;
    double *UFSSPgPtr;                
    double *UFSSPdpPtr;
    double *UFSSPspPtr;
    double *UFSSPbpPtr;
    double *UFSSPgbPtr;
    double *UFSBPgPtr;                
    double *UFSBPdpPtr;
    double *UFSBPspPtr;
    double *UFSBPbpPtr;
    double *UFSBPgbPtr;
    double *UFSGBgPtr;                
    double *UFSGBdpPtr;
    double *UFSGBspPtr;
    double *UFSGBbpPtr;
    double *UFSGBgbPtr;

    double *UFSDdPtr;
    double *UFSDdpPtr;
    double *UFSDPdPtr;
    double *UFSSsPtr;
    double *UFSSPsPtr;
    double *UFSSspPtr;
    double *UFSBbPtr;
    double *UFSBPbPtr;
    double *UFSBbpPtr;

    double *UFSTtPtr;
    double *UFSGtPtr;                 
    double *UFSDPtPtr;
    double *UFSSPtPtr;
    double *UFSBPtPtr;
    double *UFSGBtPtr;
    double *UFSTgPtr;                 
    double *UFSTdpPtr;
    double *UFSTspPtr;
    double *UFSTbpPtr;
    double *UFSTgbPtr;

    int UFSstates;     /* index into state table for this device */
#define UFSvbd UFSstates+ 0
#define UFSvbs UFSstates+ 1
#define UFSvgfs UFSstates+ 2
#define UFSvds UFSstates+ 3
#define UFSvgbs UFSstates+ 4

#define UFSqb UFSstates+ 5
#define UFScqb UFSstates+ 6
#define UFSqg UFSstates+ 7
#define UFScqg UFSstates+ 8
#define UFSqd UFSstates+ 9
#define UFScqd UFSstates+ 10
#define UFSqgb UFSstates+ 11
#define UFScqgb UFSstates+ 12
#define UFSqt UFSstates+ 13
#define UFScqt UFSstates+ 14

#define UFStemp UFSstates+ 15
#define UFSnumStates 16

/* indices to the array of UFS NOISE SOURCES */
#define UFSRDNOIZ       0
#define UFSRSNOIZ       1
#define UFSRBNOIZ       2
#define UFSIDNOIZ       3
#define UFSSJNOIZ       4
#define UFSDJNOIZ       5
#define UFSBJTNOIZ      6
#define UFSFLNOIZ       7
#define UFSTOTNOIZ      8
#define UFSNSRCS        9     /* the number of MOSFET(3) noise sources */

#ifndef NONOISE
    double UFSnVar[NSTATVARS][UFSNSRCS];
#else /* NONOISE */
        double **UFSnVar;
#endif /* NONOISE */
};

struct sUFSmodel : public sGENmodel
{
    sUFSmodel()     { memset(this, 0, sizeof(sUFSmodel)); }
    ~sUFSmodel()    { delete pModel; }
    sUFSmodel *next() { return (static_cast<sUFSmodel*>(GENnextModel)); }
    sUFSinstance *inst() { return (static_cast<sUFSinstance*>(GENinstances)); }

//    int UFSmodType;
//    struct sUFSmodel *UFSnextModel;
//    UFSinstance *UFSinstances;
//    IFuid UFSmodName; 
#define UFSmodType GENtype
#define UFSnextModel next()
#define UFSinstances inst()
#define UFSmodName GENmodName

    int UFSparamChk;
    int UFSdebug;
    struct ufsAPI_ModelData *pModel;
    double UFSvcrit;
    unsigned UFSparamChkGiven :1;
    unsigned UFSdebugGiven :1;
    unsigned ModelInitialized :1;
};

} // namespace UFS75
using namespace UFS75;

#endif // UFSDEFS_H
