
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
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1995 Min-Chie Jeng and Mansun Chan.
Modified by Weidong Liu (1997-1998).
* Revision 3.2 1998/6/16  18:00:00  Weidong 
* BSIM3v3.2 release
**********/

#include "b3defs.h"


#define MAX_EXP 5.834617425e14
#define MIN_EXP 1.713908431e-15
#define EXP_THRESHOLD 34.0
#define SMOOTHFACTOR 0.1
#define EPSOX 3.453133e-11
#define EPSSI 1.03594e-10
#define Charge_q 1.60219e-19
#define Meter2Micron 1.0e6

namespace {
    int get_node_ptr(sCKT *ckt, sB3instance *inst)
    {
        TSTALLOC(B3DdPtr, B3dNode, B3dNode)
        TSTALLOC(B3GgPtr, B3gNode, B3gNode)
        TSTALLOC(B3SsPtr, B3sNode, B3sNode)
        TSTALLOC(B3BbPtr, B3bNode, B3bNode)
        TSTALLOC(B3DPdpPtr, B3dNodePrime, B3dNodePrime)
        TSTALLOC(B3SPspPtr, B3sNodePrime, B3sNodePrime)
        TSTALLOC(B3DdpPtr, B3dNode, B3dNodePrime)
        TSTALLOC(B3GbPtr, B3gNode, B3bNode)
        TSTALLOC(B3GdpPtr, B3gNode, B3dNodePrime)
        TSTALLOC(B3GspPtr, B3gNode, B3sNodePrime)
        TSTALLOC(B3SspPtr, B3sNode, B3sNodePrime)
        TSTALLOC(B3BdpPtr, B3bNode, B3dNodePrime)
        TSTALLOC(B3BspPtr, B3bNode, B3sNodePrime)
        TSTALLOC(B3DPspPtr, B3dNodePrime, B3sNodePrime)
        TSTALLOC(B3DPdPtr, B3dNodePrime, B3dNode)
        TSTALLOC(B3BgPtr, B3bNode, B3gNode)
        TSTALLOC(B3DPgPtr, B3dNodePrime, B3gNode)
        TSTALLOC(B3SPgPtr, B3sNodePrime, B3gNode)
        TSTALLOC(B3SPsPtr, B3sNodePrime, B3sNode)
        TSTALLOC(B3DPbPtr, B3dNodePrime, B3bNode)
        TSTALLOC(B3SPbPtr, B3sNodePrime, B3bNode)
        TSTALLOC(B3SPdpPtr, B3sNodePrime, B3dNodePrime)

        TSTALLOC(B3QqPtr, B3qNode, B3qNode)

        TSTALLOC(B3QdpPtr, B3qNode, B3dNodePrime)
        TSTALLOC(B3QspPtr, B3qNode, B3sNodePrime)
        TSTALLOC(B3QgPtr, B3qNode, B3gNode)
        TSTALLOC(B3QbPtr, B3qNode, B3bNode)
        TSTALLOC(B3DPqPtr, B3dNodePrime, B3qNode)
        TSTALLOC(B3SPqPtr, B3sNodePrime, B3qNode)
        TSTALLOC(B3GqPtr, B3gNode, B3qNode)
        TSTALLOC(B3BqPtr, B3bNode, B3qNode)
        return (OK);
    }
}


int
B3dev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
    int error;
    sCKTnode *tmp;

    sB3model *model = static_cast<sB3model*>(genmod);
    for ( ; model; model = model->next()) {

        // Default value Processing for B3 MOSFET Models
        if (!model->B3typeGiven)
            model->B3type = NMOS;     
        if (!model->B3mobModGiven) 
            model->B3mobMod = 1;
        if (!model->B3binUnitGiven) 
            model->B3binUnit = 1;
        if (!model->B3paramChkGiven) 
            model->B3paramChk = 0;
        if (!model->B3capModGiven) 
            model->B3capMod = 3;
        if (!model->B3noiModGiven) 
            model->B3noiMod = 1;
        if (!model->B3versionGiven) 
            model->B3version = 3.2;
        if (!model->B3toxGiven)
            model->B3tox = 150.0e-10;
        model->B3cox = 3.453133e-11 / model->B3tox;
        if (!model->B3toxmGiven)
            model->B3toxm = model->B3tox;

        if (!model->B3cdscGiven)
            model->B3cdsc = 2.4e-4;     // unit Q/V/m^2
        if (!model->B3cdscbGiven)
            model->B3cdscb = 0.0;       // unit Q/V/m^2
        if (!model->B3cdscdGiven)
            model->B3cdscd = 0.0;       // unit Q/V/m^2
        if (!model->B3citGiven)
            model->B3cit = 0.0;         // unit Q/V/m^2
        if (!model->B3nfactorGiven)
            model->B3nfactor = 1;
        if (!model->B3xjGiven)
            model->B3xj = .15e-6;
        if (!model->B3vsatGiven)
            model->B3vsat = 8.0e4;      // unit m/s
        if (!model->B3atGiven)
            model->B3at = 3.3e4;        // unit m/s
        if (!model->B3a0Given)
            model->B3a0 = 1.0;  
        if (!model->B3agsGiven)
            model->B3ags = 0.0;
        if (!model->B3a1Given)
            model->B3a1 = 0.0;
        if (!model->B3a2Given)
            model->B3a2 = 1.0;
        if (!model->B3ketaGiven)
            model->B3keta = -0.047;     // unit  / V
        if (!model->B3nsubGiven)
            model->B3nsub = 6.0e16;     // unit 1/cm3
        if (!model->B3npeakGiven)
            model->B3npeak = 1.7e17;    // unit 1/cm3
        if (!model->B3ngateGiven)
            model->B3ngate = 0;         // unit 1/cm3
        if (!model->B3vbmGiven)
            model->B3vbm = -3.0;
        if (!model->B3xtGiven)
            model->B3xt = 1.55e-7;
        if (!model->B3kt1Given)
            model->B3kt1 = -0.11;       // unit V
        if (!model->B3kt1lGiven)
            model->B3kt1l = 0.0;        // unit V*m
        if (!model->B3kt2Given)
            model->B3kt2 = 0.022;       // No unit
        if (!model->B3k3Given)
            model->B3k3 = 80.0;      
        if (!model->B3k3bGiven)
            model->B3k3b = 0.0;      
        if (!model->B3w0Given)
            model->B3w0 = 2.5e-6;    
        if (!model->B3nlxGiven)
            model->B3nlx = 1.74e-7;     
        if (!model->B3dvt0Given)
            model->B3dvt0 = 2.2;    
        if (!model->B3dvt1Given)
            model->B3dvt1 = 0.53;      
        if (!model->B3dvt2Given)
            model->B3dvt2 = -0.032;     // unit 1 / V

        if (!model->B3dvt0wGiven)
            model->B3dvt0w = 0.0;    
        if (!model->B3dvt1wGiven)
            model->B3dvt1w = 5.3e6;    
        if (!model->B3dvt2wGiven)
            model->B3dvt2w = -0.032;   

        if (!model->B3droutGiven)
            model->B3drout = 0.56;     
        if (!model->B3dsubGiven)
            model->B3dsub = model->B3drout;     
        if (!model->B3vth0Given)
            model->B3vth0 = (model->B3type == NMOS) ? 0.7 : -0.7;
        if (!model->B3uaGiven)
            model->B3ua = 2.25e-9;      // unit m/V
        if (!model->B3ua1Given)
            model->B3ua1 = 4.31e-9;     // unit m/V
        if (!model->B3ubGiven)
            model->B3ub = 5.87e-19;     // unit (m/V)**2
        if (!model->B3ub1Given)
            model->B3ub1 = -7.61e-18;   // unit (m/V)**2
        if (!model->B3ucGiven)
            model->B3uc = (model->B3mobMod == 3) ? -0.0465 : -0.0465e-9;   
        if (!model->B3uc1Given)
            model->B3uc1 = (model->B3mobMod == 3) ? -0.056 : -0.056e-9;   
        if (!model->B3u0Given)
            model->B3u0 = (model->B3type == NMOS) ? 0.067 : 0.025;
        if (!model->B3uteGiven)
            model->B3ute = -1.5;    
        if (!model->B3voffGiven)
            model->B3voff = -0.08;
        if (!model->B3deltaGiven)  
            model->B3delta = 0.01;
        if (!model->B3rdswGiven)
            model->B3rdsw = 0;      
        if (!model->B3prwgGiven)
            model->B3prwg = 0.0;        // unit 1/V
        if (!model->B3prwbGiven)
            model->B3prwb = 0.0;      
        if (!model->B3prtGiven)
            model->B3prt = 0.0;      
        if (!model->B3eta0Given)
            model->B3eta0 = 0.08;       // no unit
        if (!model->B3etabGiven)
            model->B3etab = -0.07;      // unit  1/V
        if (!model->B3pclmGiven)
            model->B3pclm = 1.3;        // no unit
        if (!model->B3pdibl1Given)
            model->B3pdibl1 = .39;      // no unit
        if (!model->B3pdibl2Given)
            model->B3pdibl2 = 0.0086;   // no unit
        if (!model->B3pdiblbGiven)
            model->B3pdiblb = 0.0;      // 1/V
        if (!model->B3pscbe1Given)
            model->B3pscbe1 = 4.24e8;     
        if (!model->B3pscbe2Given)
            model->B3pscbe2 = 1.0e-5;    
        if (!model->B3pvagGiven)
            model->B3pvag = 0.0;     
        if (!model->B3wrGiven)  
            model->B3wr = 1.0;
        if (!model->B3dwgGiven)  
            model->B3dwg = 0.0;
        if (!model->B3dwbGiven)  
            model->B3dwb = 0.0;
        if (!model->B3b0Given)
            model->B3b0 = 0.0;
        if (!model->B3b1Given)  
            model->B3b1 = 0.0;
        if (!model->B3alpha0Given)  
            model->B3alpha0 = 0.0;
        if (!model->B3alpha1Given)
            model->B3alpha1 = 0.0;
        if (!model->B3beta0Given)  
            model->B3beta0 = 30.0;
        if (!model->B3ijthGiven)
            model->B3ijth = 0.1; // unit A

        if (!model->B3elmGiven)  
            model->B3elm = 5.0;
        if (!model->B3cgslGiven)  
            model->B3cgsl = 0.0;
        if (!model->B3cgdlGiven)  
            model->B3cgdl = 0.0;
        if (!model->B3ckappaGiven)  
            model->B3ckappa = 0.6;
        if (!model->B3clcGiven)  
            model->B3clc = 0.1e-6;
        if (!model->B3cleGiven)  
            model->B3cle = 0.6;
        if (!model->B3vfbcvGiven)  
            model->B3vfbcv = -1.0;
        if (!model->B3acdeGiven)
            model->B3acde = 1.0;
        if (!model->B3moinGiven)
            model->B3moin = 15.0;
        if (!model->B3noffGiven)
            model->B3noff = 1.0;
        if (!model->B3voffcvGiven)
            model->B3voffcv = 0.0;
        if (!model->B3tcjGiven)
            model->B3tcj = 0.0;
        if (!model->B3tpbGiven)
            model->B3tpb = 0.0;
        if (!model->B3tcjswGiven)
            model->B3tcjsw = 0.0;
        if (!model->B3tpbswGiven)
            model->B3tpbsw = 0.0;
        if (!model->B3tcjswgGiven)
            model->B3tcjswg = 0.0;
        if (!model->B3tpbswgGiven)
            model->B3tpbswg = 0.0;

        // Length dependence

        if (!model->B3lcdscGiven)
            model->B3lcdsc = 0.0;
        if (!model->B3lcdscbGiven)
            model->B3lcdscb = 0.0;
        if (!model->B3lcdscdGiven) 
            model->B3lcdscd = 0.0;
        if (!model->B3lcitGiven)
            model->B3lcit = 0.0;
        if (!model->B3lnfactorGiven)
            model->B3lnfactor = 0.0;
        if (!model->B3lxjGiven)
            model->B3lxj = 0.0;
        if (!model->B3lvsatGiven)
            model->B3lvsat = 0.0;
        if (!model->B3latGiven)
            model->B3lat = 0.0;
        if (!model->B3la0Given)
            model->B3la0 = 0.0; 
        if (!model->B3lagsGiven)
            model->B3lags = 0.0;
        if (!model->B3la1Given)
            model->B3la1 = 0.0;
        if (!model->B3la2Given)
            model->B3la2 = 0.0;
        if (!model->B3lketaGiven)
            model->B3lketa = 0.0;
        if (!model->B3lnsubGiven)
            model->B3lnsub = 0.0;
        if (!model->B3lnpeakGiven)
            model->B3lnpeak = 0.0;
        if (!model->B3lngateGiven)
            model->B3lngate = 0.0;
        if (!model->B3lvbmGiven)
            model->B3lvbm = 0.0;
        if (!model->B3lxtGiven)
            model->B3lxt = 0.0;
        if (!model->B3lkt1Given)
            model->B3lkt1 = 0.0; 
        if (!model->B3lkt1lGiven)
            model->B3lkt1l = 0.0;
        if (!model->B3lkt2Given)
            model->B3lkt2 = 0.0;
        if (!model->B3lk3Given)
            model->B3lk3 = 0.0;      
        if (!model->B3lk3bGiven)
            model->B3lk3b = 0.0;      
        if (!model->B3lw0Given)
            model->B3lw0 = 0.0;    
        if (!model->B3lnlxGiven)
            model->B3lnlx = 0.0;     
        if (!model->B3ldvt0Given)
            model->B3ldvt0 = 0.0;    
        if (!model->B3ldvt1Given)
            model->B3ldvt1 = 0.0;      
        if (!model->B3ldvt2Given)
            model->B3ldvt2 = 0.0;
        if (!model->B3ldvt0wGiven)
            model->B3ldvt0w = 0.0;    
        if (!model->B3ldvt1wGiven)
            model->B3ldvt1w = 0.0;      
        if (!model->B3ldvt2wGiven)
            model->B3ldvt2w = 0.0;
        if (!model->B3ldroutGiven)
            model->B3ldrout = 0.0;     
        if (!model->B3ldsubGiven)
            model->B3ldsub = 0.0;
        if (!model->B3lvth0Given)
            model->B3lvth0 = 0.0;
        if (!model->B3luaGiven)
            model->B3lua = 0.0;
        if (!model->B3lua1Given)
            model->B3lua1 = 0.0;
        if (!model->B3lubGiven)
            model->B3lub = 0.0;
        if (!model->B3lub1Given)
            model->B3lub1 = 0.0;
        if (!model->B3lucGiven)
            model->B3luc = 0.0;
        if (!model->B3luc1Given)
            model->B3luc1 = 0.0;
        if (!model->B3lu0Given)
            model->B3lu0 = 0.0;
        if (!model->B3luteGiven)
            model->B3lute = 0.0;    
        if (!model->B3lvoffGiven)
            model->B3lvoff = 0.0;
        if (!model->B3ldeltaGiven)  
            model->B3ldelta = 0.0;
        if (!model->B3lrdswGiven)
            model->B3lrdsw = 0.0;
        if (!model->B3lprwbGiven)
            model->B3lprwb = 0.0;
        if (!model->B3lprwgGiven)
            model->B3lprwg = 0.0;
        if (!model->B3lprtGiven)
            model->B3lprt = 0.0;
        if (!model->B3leta0Given)
            model->B3leta0 = 0.0;
        if (!model->B3letabGiven)
            model->B3letab = -0.0;
        if (!model->B3lpclmGiven)
            model->B3lpclm = 0.0; 
        if (!model->B3lpdibl1Given)
            model->B3lpdibl1 = 0.0;
        if (!model->B3lpdibl2Given)
            model->B3lpdibl2 = 0.0;
        if (!model->B3lpdiblbGiven)
            model->B3lpdiblb = 0.0;
        if (!model->B3lpscbe1Given)
            model->B3lpscbe1 = 0.0;
        if (!model->B3lpscbe2Given)
            model->B3lpscbe2 = 0.0;
        if (!model->B3lpvagGiven)
            model->B3lpvag = 0.0;     
        if (!model->B3lwrGiven)  
            model->B3lwr = 0.0;
        if (!model->B3ldwgGiven)  
            model->B3ldwg = 0.0;
        if (!model->B3ldwbGiven)  
            model->B3ldwb = 0.0;
        if (!model->B3lb0Given)
            model->B3lb0 = 0.0;
        if (!model->B3lb1Given)  
            model->B3lb1 = 0.0;
        if (!model->B3lalpha0Given)  
            model->B3lalpha0 = 0.0;
        if (!model->B3lalpha1Given)
            model->B3lalpha1 = 0.0;
        if (!model->B3lbeta0Given)  
            model->B3lbeta0 = 0.0;
        if (!model->B3lvfbGiven)
            model->B3lvfb = 0.0;

        if (!model->B3lelmGiven)  
            model->B3lelm = 0.0;
        if (!model->B3lcgslGiven)  
            model->B3lcgsl = 0.0;
        if (!model->B3lcgdlGiven)  
            model->B3lcgdl = 0.0;
        if (!model->B3lckappaGiven)  
            model->B3lckappa = 0.0;
        if (!model->B3lclcGiven)  
            model->B3lclc = 0.0;
        if (!model->B3lcleGiven)  
            model->B3lcle = 0.0;
        if (!model->B3lcfGiven)  
            model->B3lcf = 0.0;
        if (!model->B3lvfbcvGiven)  
            model->B3lvfbcv = 0.0;
        if (!model->B3lacdeGiven)
            model->B3lacde = 0.0;
        if (!model->B3lmoinGiven)
            model->B3lmoin = 0.0;
        if (!model->B3lnoffGiven)
            model->B3lnoff = 0.0;
        if (!model->B3lvoffcvGiven)
            model->B3lvoffcv = 0.0;

        // Width dependence

        if (!model->B3wcdscGiven)
            model->B3wcdsc = 0.0;
        if (!model->B3wcdscbGiven)
            model->B3wcdscb = 0.0;  
        if (!model->B3wcdscdGiven)
            model->B3wcdscd = 0.0;
        if (!model->B3wcitGiven)
            model->B3wcit = 0.0;
        if (!model->B3wnfactorGiven)
            model->B3wnfactor = 0.0;
        if (!model->B3wxjGiven)
            model->B3wxj = 0.0;
        if (!model->B3wvsatGiven)
            model->B3wvsat = 0.0;
        if (!model->B3watGiven)
            model->B3wat = 0.0;
        if (!model->B3wa0Given)
            model->B3wa0 = 0.0; 
        if (!model->B3wagsGiven)
            model->B3wags = 0.0;
        if (!model->B3wa1Given)
            model->B3wa1 = 0.0;
        if (!model->B3wa2Given)
            model->B3wa2 = 0.0;
        if (!model->B3wketaGiven)
            model->B3wketa = 0.0;
        if (!model->B3wnsubGiven)
            model->B3wnsub = 0.0;
        if (!model->B3wnpeakGiven)
            model->B3wnpeak = 0.0;
        if (!model->B3wngateGiven)
            model->B3wngate = 0.0;
        if (!model->B3wvbmGiven)
            model->B3wvbm = 0.0;
        if (!model->B3wxtGiven)
            model->B3wxt = 0.0;
        if (!model->B3wkt1Given)
            model->B3wkt1 = 0.0; 
        if (!model->B3wkt1lGiven)
            model->B3wkt1l = 0.0;
        if (!model->B3wkt2Given)
            model->B3wkt2 = 0.0;
        if (!model->B3wk3Given)
            model->B3wk3 = 0.0;      
        if (!model->B3wk3bGiven)
            model->B3wk3b = 0.0;      
        if (!model->B3ww0Given)
            model->B3ww0 = 0.0;    
        if (!model->B3wnlxGiven)
            model->B3wnlx = 0.0;     
        if (!model->B3wdvt0Given)
            model->B3wdvt0 = 0.0;    
        if (!model->B3wdvt1Given)
            model->B3wdvt1 = 0.0;      
        if (!model->B3wdvt2Given)
            model->B3wdvt2 = 0.0;
        if (!model->B3wdvt0wGiven)
            model->B3wdvt0w = 0.0;    
        if (!model->B3wdvt1wGiven)
            model->B3wdvt1w = 0.0;      
        if (!model->B3wdvt2wGiven)
            model->B3wdvt2w = 0.0;
        if (!model->B3wdroutGiven)
            model->B3wdrout = 0.0;     
        if (!model->B3wdsubGiven)
            model->B3wdsub = 0.0;
        if (!model->B3wvth0Given)
            model->B3wvth0 = 0.0;
        if (!model->B3wuaGiven)
            model->B3wua = 0.0;
        if (!model->B3wua1Given)
            model->B3wua1 = 0.0;
        if (!model->B3wubGiven)
            model->B3wub = 0.0;
        if (!model->B3wub1Given)
            model->B3wub1 = 0.0;
        if (!model->B3wucGiven)
            model->B3wuc = 0.0;
        if (!model->B3wuc1Given)
            model->B3wuc1 = 0.0;
        if (!model->B3wu0Given)
            model->B3wu0 = 0.0;
        if (!model->B3wuteGiven)
            model->B3wute = 0.0;    
        if (!model->B3wvoffGiven)
            model->B3wvoff = 0.0;
        if (!model->B3wdeltaGiven)  
            model->B3wdelta = 0.0;
        if (!model->B3wrdswGiven)
            model->B3wrdsw = 0.0;
        if (!model->B3wprwbGiven)
            model->B3wprwb = 0.0;
        if (!model->B3wprwgGiven)
            model->B3wprwg = 0.0;
        if (!model->B3wprtGiven)
            model->B3wprt = 0.0;
        if (!model->B3weta0Given)
            model->B3weta0 = 0.0;
        if (!model->B3wetabGiven)
            model->B3wetab = 0.0;
        if (!model->B3wpclmGiven)
            model->B3wpclm = 0.0; 
        if (!model->B3wpdibl1Given)
            model->B3wpdibl1 = 0.0;
        if (!model->B3wpdibl2Given)
            model->B3wpdibl2 = 0.0;
        if (!model->B3wpdiblbGiven)
            model->B3wpdiblb = 0.0;
        if (!model->B3wpscbe1Given)
            model->B3wpscbe1 = 0.0;
        if (!model->B3wpscbe2Given)
            model->B3wpscbe2 = 0.0;
        if (!model->B3wpvagGiven)
            model->B3wpvag = 0.0;     
        if (!model->B3wwrGiven)  
            model->B3wwr = 0.0;
        if (!model->B3wdwgGiven)  
            model->B3wdwg = 0.0;
        if (!model->B3wdwbGiven)  
            model->B3wdwb = 0.0;
        if (!model->B3wb0Given)
            model->B3wb0 = 0.0;
        if (!model->B3wb1Given)  
            model->B3wb1 = 0.0;
        if (!model->B3walpha0Given)  
            model->B3walpha0 = 0.0;
        if (!model->B3walpha1Given)
            model->B3walpha1 = 0.0;
        if (!model->B3wbeta0Given)  
            model->B3wbeta0 = 0.0;
        if (!model->B3wvfbGiven)
            model->B3wvfb = 0.0;

        if (!model->B3welmGiven)  
            model->B3welm = 0.0;
        if (!model->B3wcgslGiven)  
            model->B3wcgsl = 0.0;
        if (!model->B3wcgdlGiven)  
            model->B3wcgdl = 0.0;
        if (!model->B3wckappaGiven)  
            model->B3wckappa = 0.0;
        if (!model->B3wcfGiven)  
            model->B3wcf = 0.0;
        if (!model->B3wclcGiven)  
            model->B3wclc = 0.0;
        if (!model->B3wcleGiven)  
            model->B3wcle = 0.0;
        if (!model->B3wvfbcvGiven)  
            model->B3wvfbcv = 0.0;
        if (!model->B3wacdeGiven)
            model->B3wacde = 0.0;
        if (!model->B3wmoinGiven)
            model->B3wmoin = 0.0;
        if (!model->B3wnoffGiven)
            model->B3wnoff = 0.0;
        if (!model->B3wvoffcvGiven)
            model->B3wvoffcv = 0.0;

        // Cross-term dependence

        if (!model->B3pcdscGiven)
            model->B3pcdsc = 0.0;
        if (!model->B3pcdscbGiven)
            model->B3pcdscb = 0.0;   
        if (!model->B3pcdscdGiven)
            model->B3pcdscd = 0.0;
        if (!model->B3pcitGiven)
            model->B3pcit = 0.0;
        if (!model->B3pnfactorGiven)
            model->B3pnfactor = 0.0;
        if (!model->B3pxjGiven)
            model->B3pxj = 0.0;
        if (!model->B3pvsatGiven)
            model->B3pvsat = 0.0;
        if (!model->B3patGiven)
            model->B3pat = 0.0;
        if (!model->B3pa0Given)
            model->B3pa0 = 0.0; 
            
        if (!model->B3pagsGiven)
            model->B3pags = 0.0;
        if (!model->B3pa1Given)
            model->B3pa1 = 0.0;
        if (!model->B3pa2Given)
            model->B3pa2 = 0.0;
        if (!model->B3pketaGiven)
            model->B3pketa = 0.0;
        if (!model->B3pnsubGiven)
            model->B3pnsub = 0.0;
        if (!model->B3pnpeakGiven)
            model->B3pnpeak = 0.0;
        if (!model->B3pngateGiven)
            model->B3pngate = 0.0;
        if (!model->B3pvbmGiven)
            model->B3pvbm = 0.0;
        if (!model->B3pxtGiven)
            model->B3pxt = 0.0;
        if (!model->B3pkt1Given)
            model->B3pkt1 = 0.0; 
        if (!model->B3pkt1lGiven)
            model->B3pkt1l = 0.0;
        if (!model->B3pkt2Given)
            model->B3pkt2 = 0.0;
        if (!model->B3pk3Given)
            model->B3pk3 = 0.0;      
        if (!model->B3pk3bGiven)
            model->B3pk3b = 0.0;      
        if (!model->B3pw0Given)
            model->B3pw0 = 0.0;    
        if (!model->B3pnlxGiven)
            model->B3pnlx = 0.0;     
        if (!model->B3pdvt0Given)
            model->B3pdvt0 = 0.0;    
        if (!model->B3pdvt1Given)
            model->B3pdvt1 = 0.0;      
        if (!model->B3pdvt2Given)
            model->B3pdvt2 = 0.0;
        if (!model->B3pdvt0wGiven)
            model->B3pdvt0w = 0.0;    
        if (!model->B3pdvt1wGiven)
            model->B3pdvt1w = 0.0;      
        if (!model->B3pdvt2wGiven)
            model->B3pdvt2w = 0.0;
        if (!model->B3pdroutGiven)
            model->B3pdrout = 0.0;     
        if (!model->B3pdsubGiven)
            model->B3pdsub = 0.0;
        if (!model->B3pvth0Given)
            model->B3pvth0 = 0.0;
        if (!model->B3puaGiven)
            model->B3pua = 0.0;
        if (!model->B3pua1Given)
            model->B3pua1 = 0.0;
        if (!model->B3pubGiven)
            model->B3pub = 0.0;
        if (!model->B3pub1Given)
            model->B3pub1 = 0.0;
        if (!model->B3pucGiven)
            model->B3puc = 0.0;
        if (!model->B3puc1Given)
            model->B3puc1 = 0.0;
        if (!model->B3pu0Given)
            model->B3pu0 = 0.0;
        if (!model->B3puteGiven)
            model->B3pute = 0.0;    
        if (!model->B3pvoffGiven)
            model->B3pvoff = 0.0;
        if (!model->B3pdeltaGiven)  
            model->B3pdelta = 0.0;
        if (!model->B3prdswGiven)
            model->B3prdsw = 0.0;
        if (!model->B3pprwbGiven)
            model->B3pprwb = 0.0;
        if (!model->B3pprwgGiven)
            model->B3pprwg = 0.0;
        if (!model->B3pprtGiven)
            model->B3pprt = 0.0;
        if (!model->B3peta0Given)
            model->B3peta0 = 0.0;
        if (!model->B3petabGiven)
            model->B3petab = 0.0;
        if (!model->B3ppclmGiven)
            model->B3ppclm = 0.0; 
        if (!model->B3ppdibl1Given)
            model->B3ppdibl1 = 0.0;
        if (!model->B3ppdibl2Given)
            model->B3ppdibl2 = 0.0;
        if (!model->B3ppdiblbGiven)
            model->B3ppdiblb = 0.0;
        if (!model->B3ppscbe1Given)
            model->B3ppscbe1 = 0.0;
        if (!model->B3ppscbe2Given)
            model->B3ppscbe2 = 0.0;
        if (!model->B3ppvagGiven)
            model->B3ppvag = 0.0;     
        if (!model->B3pwrGiven)  
            model->B3pwr = 0.0;
        if (!model->B3pdwgGiven)  
            model->B3pdwg = 0.0;
        if (!model->B3pdwbGiven)  
            model->B3pdwb = 0.0;
        if (!model->B3pb0Given)
            model->B3pb0 = 0.0;
        if (!model->B3pb1Given)  
            model->B3pb1 = 0.0;
        if (!model->B3palpha0Given)  
            model->B3palpha0 = 0.0;
        if (!model->B3palpha1Given)
            model->B3palpha1 = 0.0;
        if (!model->B3pbeta0Given)  
            model->B3pbeta0 = 0.0;
        if (!model->B3pvfbGiven)
            model->B3pvfb = 0.0;

        if (!model->B3pelmGiven)  
            model->B3pelm = 0.0;
        if (!model->B3pcgslGiven)  
            model->B3pcgsl = 0.0;
        if (!model->B3pcgdlGiven)  
            model->B3pcgdl = 0.0;
        if (!model->B3pckappaGiven)  
            model->B3pckappa = 0.0;
        if (!model->B3pcfGiven)  
            model->B3pcf = 0.0;
        if (!model->B3pclcGiven)  
            model->B3pclc = 0.0;
        if (!model->B3pcleGiven)  
            model->B3pcle = 0.0;
        if (!model->B3pvfbcvGiven)  
            model->B3pvfbcv = 0.0;
        if (!model->B3pacdeGiven)
            model->B3pacde = 0.0;
        if (!model->B3pmoinGiven)
            model->B3pmoin = 0.0;
        if (!model->B3pnoffGiven)
            model->B3pnoff = 0.0;
        if (!model->B3pvoffcvGiven)
            model->B3pvoffcv = 0.0;

        // unit degree celcius
// SRW - badness here, on subsequent setup() calls with tnom given
// The scaling is now in the setm()/askm() routines
        if (!model->B3tnomGiven)  
            model->B3tnom = ckt->CKTcurTask->TSKnomTemp; 
//        else
//            model->B3tnom = model->B3tnom + 273.15;    

        if (!model->B3LintGiven)  
            model->B3Lint = 0.0;
        if (!model->B3LlGiven)  
            model->B3Ll = 0.0;
        if (!model->B3LlcGiven)
            model->B3Llc = model->B3Ll;
        if (!model->B3LlnGiven)  
            model->B3Lln = 1.0;
        if (!model->B3LwGiven)  
            model->B3Lw = 0.0;
        if (!model->B3LwcGiven)
            model->B3Lwc = model->B3Lw;
        if (!model->B3LwnGiven)  
            model->B3Lwn = 1.0;
        if (!model->B3LwlGiven)  
            model->B3Lwl = 0.0;
        if (!model->B3LwlcGiven)
            model->B3Lwlc = model->B3Lwl;
        if (!model->B3LminGiven)  
            model->B3Lmin = 0.0;
        if (!model->B3LmaxGiven)  
            model->B3Lmax = 1.0;
        if (!model->B3WintGiven)  
            model->B3Wint = 0.0;
        if (!model->B3WlGiven)  
            model->B3Wl = 0.0;
        if (!model->B3WlcGiven)
            model->B3Wlc = model->B3Wl;
        if (!model->B3WlnGiven)  
            model->B3Wln = 1.0;
        if (!model->B3WwGiven)  
            model->B3Ww = 0.0;
        if (!model->B3WwcGiven)
            model->B3Wwc = model->B3Ww;
        if (!model->B3WwnGiven)  
            model->B3Wwn = 1.0;
        if (!model->B3WwlGiven)  
            model->B3Wwl = 0.0;
        if (!model->B3WwlcGiven)
            model->B3Wwlc = model->B3Wwl;
        if (!model->B3WminGiven)  
            model->B3Wmin = 0.0;
        if (!model->B3WmaxGiven)  
            model->B3Wmax = 1.0;
        if (!model->B3dwcGiven)  
            model->B3dwc = model->B3Wint;
        if (!model->B3dlcGiven)  
            model->B3dlc = model->B3Lint;
        if (!model->B3cfGiven)
            model->B3cf = 2.0 * EPSOX / M_PI*log(1.0 + 0.4e-6 / model->B3tox);
        if (!model->B3cgdoGiven) {
            if (model->B3dlcGiven && (model->B3dlc > 0.0)) {
                model->B3cgdo = model->B3dlc * model->B3cox
                    - model->B3cgdl;
            }
            else
                model->B3cgdo = 0.6 * model->B3xj * model->B3cox; 
        }
        if (!model->B3cgsoGiven) {
            if (model->B3dlcGiven && (model->B3dlc > 0.0)) {
                model->B3cgso = model->B3dlc * model->B3cox
                    - model->B3cgsl ;
            }
            else
                model->B3cgso = 0.6 * model->B3xj * model->B3cox; 
        }

        if (!model->B3cgboGiven) {
            model->B3cgbo = 2.0 * model->B3dwc * model->B3cox;
        }
        if (!model->B3xpartGiven)
            model->B3xpart = 0.0;
        if (!model->B3sheetResistanceGiven)
            model->B3sheetResistance = 0.0;
        if (!model->B3unitAreaJctCapGiven)
            model->B3unitAreaJctCap = 5.0E-4;
        if (!model->B3unitLengthSidewallJctCapGiven)
            model->B3unitLengthSidewallJctCap = 5.0E-10;
        if (!model->B3unitLengthGateSidewallJctCapGiven)
            model->B3unitLengthGateSidewallJctCap = model->B3unitLengthSidewallJctCap ;
        if (!model->B3jctSatCurDensityGiven)
            model->B3jctSatCurDensity = 1.0E-4;
        if (!model->B3jctSidewallSatCurDensityGiven)
            model->B3jctSidewallSatCurDensity = 0.0;
        if (!model->B3bulkJctPotentialGiven)
            model->B3bulkJctPotential = 1.0;
        if (!model->B3sidewallJctPotentialGiven)
            model->B3sidewallJctPotential = 1.0;
        if (!model->B3GatesidewallJctPotentialGiven)
            model->B3GatesidewallJctPotential = model->B3sidewallJctPotential;
        if (!model->B3bulkJctBotGradingCoeffGiven)
            model->B3bulkJctBotGradingCoeff = 0.5;
        if (!model->B3bulkJctSideGradingCoeffGiven)
            model->B3bulkJctSideGradingCoeff = 0.33;
        if (!model->B3bulkJctGateSideGradingCoeffGiven)
            model->B3bulkJctGateSideGradingCoeff = model->B3bulkJctSideGradingCoeff;
        if (!model->B3jctEmissionCoeffGiven)
            model->B3jctEmissionCoeff = 1.0;
        if (!model->B3jctTempExponentGiven)
            model->B3jctTempExponent = 3.0;
        if (!model->B3oxideTrapDensityAGiven) {
            if (model->B3type == NMOS)
                model->B3oxideTrapDensityA = 1e20;
            else
                model->B3oxideTrapDensityA=9.9e18;
        }
        if (!model->B3oxideTrapDensityBGiven) {
            if (model->B3type == NMOS)
                model->B3oxideTrapDensityB = 5e4;
            else
                model->B3oxideTrapDensityB = 2.4e3;
        }
        if (!model->B3oxideTrapDensityCGiven) {
            if (model->B3type == NMOS)
                model->B3oxideTrapDensityC = -1.4e-12;
            else
                model->B3oxideTrapDensityC = 1.4e-12;
        }
        if (!model->B3emGiven)
            model->B3em = 4.1e7; // V/m
        if (!model->B3efGiven)
            model->B3ef = 1.0;
        if (!model->B3afGiven)
            model->B3af = 1.0;
        if (!model->B3kfGiven)
            model->B3kf = 0.0;

// SRW
        if (!model->B3nqsModGiven)
            model->B3nqsMod = 0;

        sB3instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            // allocate a chunk of the state vector
            inst->GENstate = *states;
            *states += B3numStates;
            // perform the parameter defaulting
            if (!inst->B3mGiven)
                inst->B3m = 1.0;
            if (!inst->B3drainAreaGiven)
                inst->B3drainArea = ckt->mos_default_ad();
            if (!inst->B3drainPerimeterGiven)
                inst->B3drainPerimeter = 0.0;
            if (!inst->B3drainSquaresGiven)
                inst->B3drainSquares = 1.0;
            if (!inst->B3icVBSGiven)
                inst->B3icVBS = 0.0;
            if (!inst->B3icVDSGiven)
                inst->B3icVDS = 0.0;
            if (!inst->B3icVGSGiven)
                inst->B3icVGS = 0.0;
            if (!inst->B3lGiven)
                inst->B3l = ckt->mos_default_l();
            if (!inst->B3sourceAreaGiven)
                inst->B3sourceArea = ckt->mos_default_as();
            if (!inst->B3sourcePerimeterGiven)
                inst->B3sourcePerimeter = 0.0;
            if (!inst->B3sourceSquaresGiven)
                inst->B3sourceSquares = 1.0;
            if (!inst->B3wGiven)
                inst->B3w = ckt->mos_default_w();

// SRW
/*
            if (!inst->B3nqsModGiven)
                inst->B3nqsMod = 0;
*/
            if (!inst->B3nqsModGiven)
                inst->B3nqsMod = model->B3nqsMod;
                    
            // process drain series resistance
            if ((model->B3sheetResistance > 0.0) && 
                    (inst->B3drainSquares > 0.0 ) &&
                    (inst->B3dNodePrime == 0)) {
                error = ckt->mkVolt(&tmp, inst->GENname, "drain");
                if (error)
                    return (error);
                inst->B3dNodePrime = tmp->number();
            }
            else {
                inst->B3dNodePrime = inst->B3dNode;
            }
                   
            // process source series resistance
            if ((model->B3sheetResistance > 0.0) && 
                    (inst->B3sourceSquares > 0.0 ) &&
                    (inst->B3sNodePrime == 0)) {
                error = ckt->mkVolt(&tmp, inst->GENname, "source");
                if (error)
                    return (error);
                inst->B3sNodePrime = tmp->number();
            }
            else {
                inst->B3sNodePrime = inst->B3sNode;
            }

            // internal charge node
                   
            if ((inst->B3nqsMod) && (inst->B3qNode == 0)) {
                error = ckt->mkVolt(&tmp, inst->GENname, "charge");
                if (error)
                    return (error);
                inst->B3qNode = tmp->number();
            }
            else {
                inst->B3qNode = 0;
            }

            // set Sparse Matrix Pointers
            error = get_node_ptr(ckt, inst);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}  


int
B3dev::unsetup(sGENmodel *genmod, sCKT*)
{
    sB3model *model = static_cast<sB3model*>(genmod);
    for ( ; model; model = model->next()) {
        sB3instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            if (inst->B3dNodePrime != inst->B3dNode)
                inst->B3dNodePrime = 0;
            if (inst->B3sNodePrime != inst->B3sNode)
                inst->B3sNodePrime = 0;
            if (inst->B3qNode)
                inst->B3qNode = 0;
        }
    }
    return (OK);
}


// SRW - reset the matrix element pointers.
//
int
B3dev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sB3model *model = (sB3model*)inModel; model;
            model = model->next()) {
        for (sB3instance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}

