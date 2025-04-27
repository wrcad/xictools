
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
Author: 1995 Min-Chie Jeng and Mansun Chan.
Modified by Weidong Liu (1997-1998).
* Revision 3.2 1998/6/16  18:00:00  Weidong 
* BSIM3v3.2 release
**********/

#include "b3defs.h"


int
B3dev::setModl(int param, IFdata *data, sGENmodel *genmod)
{
    sB3model *model = static_cast<sB3model*>(genmod);
    IFvalue *value = &data->v;

    switch (param) {
    case B3_MOD_MOBMOD:
        model->B3mobMod = value->iValue;
        model->B3mobModGiven = true;
        break;
    case B3_MOD_BINUNIT:
        model->B3binUnit = value->iValue;
        model->B3binUnitGiven = true;
        break;
    case B3_MOD_PARAMCHK:
        model->B3paramChk = value->iValue;
        model->B3paramChkGiven = true;
        break;
    case B3_MOD_CAPMOD:
        model->B3capMod = value->iValue;
        model->B3capModGiven = true;
        break;
    case B3_MOD_NOIMOD:
        model->B3noiMod = value->iValue;
        model->B3noiModGiven = true;
        break;
    case B3_MOD_VERSION:
        model->B3version = value->rValue;
        model->B3versionGiven = true;
        break;
    case B3_MOD_TOX:
        model->B3tox = value->rValue;
        model->B3toxGiven = true;
        break;
    case B3_MOD_TOXM:
        model->B3toxm = value->rValue;
        model->B3toxmGiven = true;
        break;

    case B3_MOD_CDSC:
        model->B3cdsc = value->rValue;
        model->B3cdscGiven = true;
        break;
    case B3_MOD_CDSCB:
        model->B3cdscb = value->rValue;
        model->B3cdscbGiven = true;
        break;

    case B3_MOD_CDSCD:
        model->B3cdscd = value->rValue;
        model->B3cdscdGiven = true;
        break;

    case B3_MOD_CIT:
        model->B3cit = value->rValue;
        model->B3citGiven = true;
        break;
    case B3_MOD_NFACTOR:
        model->B3nfactor = value->rValue;
        model->B3nfactorGiven = true;
        break;
    case B3_MOD_XJ:
        model->B3xj = value->rValue;
        model->B3xjGiven = true;
        break;
    case B3_MOD_VSAT:
        model->B3vsat = value->rValue;
        model->B3vsatGiven = true;
        break;
    case B3_MOD_A0:
        model->B3a0 = value->rValue;
        model->B3a0Given = true;
        break;
    
    case B3_MOD_AGS:
        model->B3ags = value->rValue;
        model->B3agsGiven = true;
        break;
    
    case B3_MOD_A1:
        model->B3a1 = value->rValue;
        model->B3a1Given = true;
        break;
    case B3_MOD_A2:
        model->B3a2 = value->rValue;
        model->B3a2Given = true;
        break;
    case B3_MOD_AT:
        model->B3at = value->rValue;
        model->B3atGiven = true;
        break;
    case B3_MOD_KETA:
        model->B3keta = value->rValue;
        model->B3ketaGiven = true;
        break;    
    case B3_MOD_NSUB:
        model->B3nsub = value->rValue;
        model->B3nsubGiven = true;
        break;
    case B3_MOD_NPEAK:
        model->B3npeak = value->rValue;
        model->B3npeakGiven = true;
        if (model->B3npeak > 1.0e20)
            model->B3npeak *= 1.0e-6;
        break;
    case B3_MOD_NGATE:
        model->B3ngate = value->rValue;
        model->B3ngateGiven = true;
        if (model->B3ngate > 1.0e23)
            model->B3ngate *= 1.0e-6;
        break;
    case B3_MOD_GAMMA1:
        model->B3gamma1 = value->rValue;
        model->B3gamma1Given = true;
        break;
    case B3_MOD_GAMMA2:
        model->B3gamma2 = value->rValue;
        model->B3gamma2Given = true;
        break;
    case B3_MOD_VBX:
        model->B3vbx = value->rValue;
        model->B3vbxGiven = true;
        break;
    case B3_MOD_VBM:
        model->B3vbm = value->rValue;
        model->B3vbmGiven = true;
        break;
    case B3_MOD_XT:
        model->B3xt = value->rValue;
        model->B3xtGiven = true;
        break;
    case B3_MOD_K1:
        model->B3k1 = value->rValue;
        model->B3k1Given = true;
        break;
    case B3_MOD_KT1:
        model->B3kt1 = value->rValue;
        model->B3kt1Given = true;
        break;
    case B3_MOD_KT1L:
        model->B3kt1l = value->rValue;
        model->B3kt1lGiven = true;
        break;
    case B3_MOD_KT2:
        model->B3kt2 = value->rValue;
        model->B3kt2Given = true;
        break;
    case B3_MOD_K2:
        model->B3k2 = value->rValue;
        model->B3k2Given = true;
        break;
    case B3_MOD_K3:
        model->B3k3 = value->rValue;
        model->B3k3Given = true;
        break;
    case B3_MOD_K3B:
        model->B3k3b = value->rValue;
        model->B3k3bGiven = true;
        break;
    case B3_MOD_NLX:
        model->B3nlx = value->rValue;
        model->B3nlxGiven = true;
        break;
    case B3_MOD_W0:
        model->B3w0 = value->rValue;
        model->B3w0Given = true;
        break;
    case B3_MOD_DVT0:               
        model->B3dvt0 = value->rValue;
        model->B3dvt0Given = true;
        break;
    case B3_MOD_DVT1:             
        model->B3dvt1 = value->rValue;
        model->B3dvt1Given = true;
        break;
    case B3_MOD_DVT2:             
        model->B3dvt2 = value->rValue;
        model->B3dvt2Given = true;
        break;
    case B3_MOD_DVT0W:               
        model->B3dvt0w = value->rValue;
        model->B3dvt0wGiven = true;
        break;
    case B3_MOD_DVT1W:             
        model->B3dvt1w = value->rValue;
        model->B3dvt1wGiven = true;
        break;
    case B3_MOD_DVT2W:             
        model->B3dvt2w = value->rValue;
        model->B3dvt2wGiven = true;
        break;
    case B3_MOD_DROUT:             
        model->B3drout = value->rValue;
        model->B3droutGiven = true;
        break;
    case B3_MOD_DSUB:             
        model->B3dsub = value->rValue;
        model->B3dsubGiven = true;
        break;
    case B3_MOD_VTH0:
        model->B3vth0 = value->rValue;
        model->B3vth0Given = true;
        break;
    case B3_MOD_UA:
        model->B3ua = value->rValue;
        model->B3uaGiven = true;
        break;
    case B3_MOD_UA1:
        model->B3ua1 = value->rValue;
        model->B3ua1Given = true;
        break;
    case B3_MOD_UB:
        model->B3ub = value->rValue;
        model->B3ubGiven = true;
        break;
    case B3_MOD_UB1:
        model->B3ub1 = value->rValue;
        model->B3ub1Given = true;
        break;
    case B3_MOD_UC:
        model->B3uc = value->rValue;
        model->B3ucGiven = true;
        break;
    case B3_MOD_UC1:
        model->B3uc1 = value->rValue;
        model->B3uc1Given = true;
        break;
    case B3_MOD_U0:
        model->B3u0 = value->rValue;
        model->B3u0Given = true;
        break;
    case B3_MOD_UTE:
        model->B3ute = value->rValue;
        model->B3uteGiven = true;
        break;
    case B3_MOD_VOFF:
        model->B3voff = value->rValue;
        model->B3voffGiven = true;
        break;
    case B3_MOD_DELTA:
        model->B3delta = value->rValue;
        model->B3deltaGiven = true;
        break;
    case B3_MOD_RDSW:
        model->B3rdsw = value->rValue;
        model->B3rdswGiven = true;
        break;                     
    case B3_MOD_PRWG:
        model->B3prwg = value->rValue;
        model->B3prwgGiven = true;
        break;                     
    case B3_MOD_PRWB:
        model->B3prwb = value->rValue;
        model->B3prwbGiven = true;
        break;                     
    case B3_MOD_PRT:
        model->B3prt = value->rValue;
        model->B3prtGiven = true;
        break;                     
    case B3_MOD_ETA0:
        model->B3eta0 = value->rValue;
        model->B3eta0Given = true;
        break;                 
    case B3_MOD_ETAB:
        model->B3etab = value->rValue;
        model->B3etabGiven = true;
        break;                 
    case B3_MOD_PCLM:
        model->B3pclm = value->rValue;
        model->B3pclmGiven = true;
        break;                 
    case B3_MOD_PDIBL1:
        model->B3pdibl1 = value->rValue;
        model->B3pdibl1Given = true;
        break;                 
    case B3_MOD_PDIBL2:
        model->B3pdibl2 = value->rValue;
        model->B3pdibl2Given = true;
        break;                 
    case B3_MOD_PDIBLB:
        model->B3pdiblb = value->rValue;
        model->B3pdiblbGiven = true;
        break;                 
    case B3_MOD_PSCBE1:
        model->B3pscbe1 = value->rValue;
        model->B3pscbe1Given = true;
        break;                 
    case B3_MOD_PSCBE2:
        model->B3pscbe2 = value->rValue;
        model->B3pscbe2Given = true;
        break;                 
    case B3_MOD_PVAG:
        model->B3pvag = value->rValue;
        model->B3pvagGiven = true;
        break;                 
    case B3_MOD_WR:
        model->B3wr = value->rValue;
        model->B3wrGiven = true;
        break;
    case B3_MOD_DWG:
        model->B3dwg = value->rValue;
        model->B3dwgGiven = true;
        break;
    case B3_MOD_DWB:
        model->B3dwb = value->rValue;
        model->B3dwbGiven = true;
        break;
    case B3_MOD_B0:
        model->B3b0 = value->rValue;
        model->B3b0Given = true;
        break;
    case B3_MOD_B1:
        model->B3b1 = value->rValue;
        model->B3b1Given = true;
        break;
    case B3_MOD_ALPHA0:
        model->B3alpha0 = value->rValue;
        model->B3alpha0Given = true;
        break;
    case B3_MOD_ALPHA1:
        model->B3alpha1 = value->rValue;
        model->B3alpha1Given = true;
        break;
    case B3_MOD_BETA0:
        model->B3beta0 = value->rValue;
        model->B3beta0Given = true;
        break;
    case B3_MOD_IJTH:
        model->B3ijth = value->rValue;
        model->B3ijthGiven = true;
        break;
    case B3_MOD_VFB:
        model->B3vfb = value->rValue;
        model->B3vfbGiven = true;
        break;

    case B3_MOD_ELM:
        model->B3elm = value->rValue;
        model->B3elmGiven = true;
        break;
    case B3_MOD_CGSL:
        model->B3cgsl = value->rValue;
        model->B3cgslGiven = true;
        break;
    case B3_MOD_CGDL:
        model->B3cgdl = value->rValue;
        model->B3cgdlGiven = true;
        break;
    case B3_MOD_CKAPPA:
        model->B3ckappa = value->rValue;
        model->B3ckappaGiven = true;
        break;
    case B3_MOD_CF:
        model->B3cf = value->rValue;
        model->B3cfGiven = true;
        break;
    case B3_MOD_CLC:
        model->B3clc = value->rValue;
        model->B3clcGiven = true;
        break;
    case B3_MOD_CLE:
        model->B3cle = value->rValue;
        model->B3cleGiven = true;
        break;
    case B3_MOD_DWC:
        model->B3dwc = value->rValue;
        model->B3dwcGiven = true;
        break;
    case B3_MOD_DLC:
        model->B3dlc = value->rValue;
        model->B3dlcGiven = true;
        break;
    case B3_MOD_VFBCV:
        model->B3vfbcv = value->rValue;
        model->B3vfbcvGiven = true;
        break;
    case B3_MOD_ACDE:
        model->B3acde = value->rValue;
        model->B3acdeGiven = true;
        break;
    case B3_MOD_MOIN:
        model->B3moin = value->rValue;
        model->B3moinGiven = true;
        break;
    case B3_MOD_NOFF:
        model->B3noff = value->rValue;
        model->B3noffGiven = true;
        break;
    case B3_MOD_VOFFCV:
        model->B3voffcv = value->rValue;
        model->B3voffcvGiven = true;
        break;
    case B3_MOD_TCJ:
        model->B3tcj = value->rValue;
        model->B3tcjGiven = true;
        break;
    case B3_MOD_TPB:
        model->B3tpb = value->rValue;
        model->B3tpbGiven = true;
        break;
    case B3_MOD_TCJSW:
        model->B3tcjsw = value->rValue;
        model->B3tcjswGiven = true;
        break;
    case B3_MOD_TPBSW:
        model->B3tpbsw = value->rValue;
        model->B3tpbswGiven = true;
        break;
    case B3_MOD_TCJSWG:
        model->B3tcjswg = value->rValue;
        model->B3tcjswgGiven = true;
        break;
    case B3_MOD_TPBSWG:
        model->B3tpbswg = value->rValue;
        model->B3tpbswgGiven = true;
        break;

    // Length dependence
    case B3_MOD_LCDSC:
        model->B3lcdsc = value->rValue;
        model->B3lcdscGiven = true;
        break;


    case B3_MOD_LCDSCB:
        model->B3lcdscb = value->rValue;
        model->B3lcdscbGiven = true;
        break;
    case B3_MOD_LCDSCD:
        model->B3lcdscd = value->rValue;
        model->B3lcdscdGiven = true;
        break;
    case B3_MOD_LCIT:
        model->B3lcit = value->rValue;
        model->B3lcitGiven = true;
        break;
    case B3_MOD_LNFACTOR:
        model->B3lnfactor = value->rValue;
        model->B3lnfactorGiven = true;
        break;
    case B3_MOD_LXJ:
        model->B3lxj = value->rValue;
        model->B3lxjGiven = true;
        break;
    case B3_MOD_LVSAT:
        model->B3lvsat = value->rValue;
        model->B3lvsatGiven = true;
        break;
    
    
    case B3_MOD_LA0:
        model->B3la0 = value->rValue;
        model->B3la0Given = true;
        break;
    case B3_MOD_LAGS:
        model->B3lags = value->rValue;
        model->B3lagsGiven = true;
        break;
    case B3_MOD_LA1:
        model->B3la1 = value->rValue;
        model->B3la1Given = true;
        break;
    case B3_MOD_LA2:
        model->B3la2 = value->rValue;
        model->B3la2Given = true;
        break;
    case B3_MOD_LAT:
        model->B3lat = value->rValue;
        model->B3latGiven = true;
        break;
    case B3_MOD_LKETA:
        model->B3lketa = value->rValue;
        model->B3lketaGiven = true;
        break;    
    case B3_MOD_LNSUB:
        model->B3lnsub = value->rValue;
        model->B3lnsubGiven = true;
        break;
    case B3_MOD_LNPEAK:
        model->B3lnpeak = value->rValue;
        model->B3lnpeakGiven = true;
    if (model->B3lnpeak > 1.0e20)
    model->B3lnpeak *= 1.0e-6;
        break;
    case B3_MOD_LNGATE:
        model->B3lngate = value->rValue;
        model->B3lngateGiven = true;
    if (model->B3lngate > 1.0e23)
    model->B3lngate *= 1.0e-6;
        break;
    case B3_MOD_LGAMMA1:
        model->B3lgamma1 = value->rValue;
        model->B3lgamma1Given = true;
        break;
    case B3_MOD_LGAMMA2:
        model->B3lgamma2 = value->rValue;
        model->B3lgamma2Given = true;
        break;
    case B3_MOD_LVBX:
        model->B3lvbx = value->rValue;
        model->B3lvbxGiven = true;
        break;
    case B3_MOD_LVBM:
        model->B3lvbm = value->rValue;
        model->B3lvbmGiven = true;
        break;
    case B3_MOD_LXT:
        model->B3lxt = value->rValue;
        model->B3lxtGiven = true;
        break;
    case B3_MOD_LK1:
        model->B3lk1 = value->rValue;
        model->B3lk1Given = true;
        break;
    case B3_MOD_LKT1:
        model->B3lkt1 = value->rValue;
        model->B3lkt1Given = true;
        break;
    case B3_MOD_LKT1L:
        model->B3lkt1l = value->rValue;
        model->B3lkt1lGiven = true;
        break;
    case B3_MOD_LKT2:
        model->B3lkt2 = value->rValue;
        model->B3lkt2Given = true;
        break;
    case B3_MOD_LK2:
        model->B3lk2 = value->rValue;
        model->B3lk2Given = true;
        break;
    case B3_MOD_LK3:
        model->B3lk3 = value->rValue;
        model->B3lk3Given = true;
        break;
    case B3_MOD_LK3B:
        model->B3lk3b = value->rValue;
        model->B3lk3bGiven = true;
        break;
    case B3_MOD_LNLX:
        model->B3lnlx = value->rValue;
        model->B3lnlxGiven = true;
        break;
    case B3_MOD_LW0:
        model->B3lw0 = value->rValue;
        model->B3lw0Given = true;
        break;
    case B3_MOD_LDVT0:               
        model->B3ldvt0 = value->rValue;
        model->B3ldvt0Given = true;
        break;
    case B3_MOD_LDVT1:             
        model->B3ldvt1 = value->rValue;
        model->B3ldvt1Given = true;
        break;
    case B3_MOD_LDVT2:             
        model->B3ldvt2 = value->rValue;
        model->B3ldvt2Given = true;
        break;
    case B3_MOD_LDVT0W:               
        model->B3ldvt0w = value->rValue;
        model->B3ldvt0wGiven = true;
        break;
    case B3_MOD_LDVT1W:             
        model->B3ldvt1w = value->rValue;
        model->B3ldvt1wGiven = true;
        break;
    case B3_MOD_LDVT2W:             
        model->B3ldvt2w = value->rValue;
        model->B3ldvt2wGiven = true;
        break;
    case B3_MOD_LDROUT:             
        model->B3ldrout = value->rValue;
        model->B3ldroutGiven = true;
        break;
    case B3_MOD_LDSUB:             
        model->B3ldsub = value->rValue;
        model->B3ldsubGiven = true;
        break;
    case B3_MOD_LVTH0:
        model->B3lvth0 = value->rValue;
        model->B3lvth0Given = true;
        break;
    case B3_MOD_LUA:
        model->B3lua = value->rValue;
        model->B3luaGiven = true;
        break;
    case B3_MOD_LUA1:
        model->B3lua1 = value->rValue;
        model->B3lua1Given = true;
        break;
    case B3_MOD_LUB:
        model->B3lub = value->rValue;
        model->B3lubGiven = true;
        break;
    case B3_MOD_LUB1:
        model->B3lub1 = value->rValue;
        model->B3lub1Given = true;
        break;
    case B3_MOD_LUC:
        model->B3luc = value->rValue;
        model->B3lucGiven = true;
        break;
    case B3_MOD_LUC1:
        model->B3luc1 = value->rValue;
        model->B3luc1Given = true;
        break;
    case B3_MOD_LU0:
        model->B3lu0 = value->rValue;
        model->B3lu0Given = true;
        break;
    case B3_MOD_LUTE:
        model->B3lute = value->rValue;
        model->B3luteGiven = true;
        break;
    case B3_MOD_LVOFF:
        model->B3lvoff = value->rValue;
        model->B3lvoffGiven = true;
        break;
    case B3_MOD_LDELTA:
        model->B3ldelta = value->rValue;
        model->B3ldeltaGiven = true;
        break;
    case B3_MOD_LRDSW:
        model->B3lrdsw = value->rValue;
        model->B3lrdswGiven = true;
        break;                     
    case B3_MOD_LPRWB:
        model->B3lprwb = value->rValue;
        model->B3lprwbGiven = true;
        break;                     
    case B3_MOD_LPRWG:
        model->B3lprwg = value->rValue;
        model->B3lprwgGiven = true;
        break;                     
    case B3_MOD_LPRT:
        model->B3lprt = value->rValue;
        model->B3lprtGiven = true;
        break;                     
    case B3_MOD_LETA0:
        model->B3leta0 = value->rValue;
        model->B3leta0Given = true;
        break;                 
    case B3_MOD_LETAB:
        model->B3letab = value->rValue;
        model->B3letabGiven = true;
        break;                 
    case B3_MOD_LPCLM:
        model->B3lpclm = value->rValue;
        model->B3lpclmGiven = true;
        break;                 
    case B3_MOD_LPDIBL1:
        model->B3lpdibl1 = value->rValue;
        model->B3lpdibl1Given = true;
        break;                 
    case B3_MOD_LPDIBL2:
        model->B3lpdibl2 = value->rValue;
        model->B3lpdibl2Given = true;
        break;                 
    case B3_MOD_LPDIBLB:
        model->B3lpdiblb = value->rValue;
        model->B3lpdiblbGiven = true;
        break;                 
    case B3_MOD_LPSCBE1:
        model->B3lpscbe1 = value->rValue;
        model->B3lpscbe1Given = true;
        break;                 
    case B3_MOD_LPSCBE2:
        model->B3lpscbe2 = value->rValue;
        model->B3lpscbe2Given = true;
        break;                 
    case B3_MOD_LPVAG:
        model->B3lpvag = value->rValue;
        model->B3lpvagGiven = true;
        break;                 
    case B3_MOD_LWR:
        model->B3lwr = value->rValue;
        model->B3lwrGiven = true;
        break;
    case B3_MOD_LDWG:
        model->B3ldwg = value->rValue;
        model->B3ldwgGiven = true;
        break;
    case B3_MOD_LDWB:
        model->B3ldwb = value->rValue;
        model->B3ldwbGiven = true;
        break;
    case B3_MOD_LB0:
        model->B3lb0 = value->rValue;
        model->B3lb0Given = true;
        break;
    case B3_MOD_LB1:
        model->B3lb1 = value->rValue;
        model->B3lb1Given = true;
        break;
    case B3_MOD_LALPHA0:
        model->B3lalpha0 = value->rValue;
        model->B3lalpha0Given = true;
        break;
    case B3_MOD_LALPHA1:
        model->B3lalpha1 = value->rValue;
        model->B3lalpha1Given = true;
        break;
    case B3_MOD_LBETA0:
        model->B3lbeta0 = value->rValue;
        model->B3lbeta0Given = true;
        break;
    case B3_MOD_LVFB:
        model->B3lvfb = value->rValue;
        model->B3lvfbGiven = true;
        break;

    case B3_MOD_LELM:
        model->B3lelm = value->rValue;
        model->B3lelmGiven = true;
        break;
    case B3_MOD_LCGSL:
        model->B3lcgsl = value->rValue;
        model->B3lcgslGiven = true;
        break;
    case B3_MOD_LCGDL:
        model->B3lcgdl = value->rValue;
        model->B3lcgdlGiven = true;
        break;
    case B3_MOD_LCKAPPA:
        model->B3lckappa = value->rValue;
        model->B3lckappaGiven = true;
        break;
    case B3_MOD_LCF:
        model->B3lcf = value->rValue;
        model->B3lcfGiven = true;
        break;
    case B3_MOD_LCLC:
        model->B3lclc = value->rValue;
        model->B3lclcGiven = true;
        break;
    case B3_MOD_LCLE:
        model->B3lcle = value->rValue;
        model->B3lcleGiven = true;
        break;
    case B3_MOD_LVFBCV:
        model->B3lvfbcv = value->rValue;
        model->B3lvfbcvGiven = true;
        break;
    case B3_MOD_LACDE:
        model->B3lacde = value->rValue;
        model->B3lacdeGiven = true;
        break;
    case B3_MOD_LMOIN:
        model->B3lmoin = value->rValue;
        model->B3lmoinGiven = true;
        break;
    case B3_MOD_LNOFF:
        model->B3lnoff = value->rValue;
        model->B3lnoffGiven = true;
        break;
    case B3_MOD_LVOFFCV:
        model->B3lvoffcv = value->rValue;
        model->B3lvoffcvGiven = true;
        break;

    // Width dependence
    case B3_MOD_WCDSC:
        model->B3wcdsc = value->rValue;
        model->B3wcdscGiven = true;
        break;
   
   
     case B3_MOD_WCDSCB:
        model->B3wcdscb = value->rValue;
        model->B3wcdscbGiven = true;
        break;
     case B3_MOD_WCDSCD:
        model->B3wcdscd = value->rValue;
        model->B3wcdscdGiven = true;
        break;
    case B3_MOD_WCIT:
        model->B3wcit = value->rValue;
        model->B3wcitGiven = true;
        break;
    case B3_MOD_WNFACTOR:
        model->B3wnfactor = value->rValue;
        model->B3wnfactorGiven = true;
        break;
    case B3_MOD_WXJ:
        model->B3wxj = value->rValue;
        model->B3wxjGiven = true;
        break;
    case B3_MOD_WVSAT:
        model->B3wvsat = value->rValue;
        model->B3wvsatGiven = true;
        break;


    case B3_MOD_WA0:
        model->B3wa0 = value->rValue;
        model->B3wa0Given = true;
        break;
    case B3_MOD_WAGS:
        model->B3wags = value->rValue;
        model->B3wagsGiven = true;
        break;
    case B3_MOD_WA1:
        model->B3wa1 = value->rValue;
        model->B3wa1Given = true;
        break;
    case B3_MOD_WA2:
        model->B3wa2 = value->rValue;
        model->B3wa2Given = true;
        break;
    case B3_MOD_WAT:
        model->B3wat = value->rValue;
        model->B3watGiven = true;
        break;
    case B3_MOD_WKETA:
        model->B3wketa = value->rValue;
        model->B3wketaGiven = true;
        break;    
    case B3_MOD_WNSUB:
        model->B3wnsub = value->rValue;
        model->B3wnsubGiven = true;
        break;
    case B3_MOD_WNPEAK:
        model->B3wnpeak = value->rValue;
        model->B3wnpeakGiven = true;
        if (model->B3wnpeak > 1.0e20)
            model->B3wnpeak *= 1.0e-6;
        break;
    case B3_MOD_WNGATE:
        model->B3wngate = value->rValue;
        model->B3wngateGiven = true;
        if (model->B3wngate > 1.0e23)
            model->B3wngate *= 1.0e-6;
        break;
    case B3_MOD_WGAMMA1:
        model->B3wgamma1 = value->rValue;
        model->B3wgamma1Given = true;
        break;
    case B3_MOD_WGAMMA2:
        model->B3wgamma2 = value->rValue;
        model->B3wgamma2Given = true;
        break;
    case B3_MOD_WVBX:
        model->B3wvbx = value->rValue;
        model->B3wvbxGiven = true;
        break;
    case B3_MOD_WVBM:
        model->B3wvbm = value->rValue;
        model->B3wvbmGiven = true;
        break;
    case B3_MOD_WXT:
        model->B3wxt = value->rValue;
        model->B3wxtGiven = true;
        break;
    case B3_MOD_WK1:
        model->B3wk1 = value->rValue;
        model->B3wk1Given = true;
        break;
    case B3_MOD_WKT1:
        model->B3wkt1 = value->rValue;
        model->B3wkt1Given = true;
        break;
    case B3_MOD_WKT1L:
        model->B3wkt1l = value->rValue;
        model->B3wkt1lGiven = true;
        break;
    case B3_MOD_WKT2:
        model->B3wkt2 = value->rValue;
        model->B3wkt2Given = true;
        break;
    case B3_MOD_WK2:
        model->B3wk2 = value->rValue;
        model->B3wk2Given = true;
        break;
    case B3_MOD_WK3:
        model->B3wk3 = value->rValue;
        model->B3wk3Given = true;
        break;
    case B3_MOD_WK3B:
        model->B3wk3b = value->rValue;
        model->B3wk3bGiven = true;
        break;
    case B3_MOD_WNLX:
        model->B3wnlx = value->rValue;
        model->B3wnlxGiven = true;
        break;
    case B3_MOD_WW0:
        model->B3ww0 = value->rValue;
        model->B3ww0Given = true;
        break;
    case B3_MOD_WDVT0:               
        model->B3wdvt0 = value->rValue;
        model->B3wdvt0Given = true;
        break;
    case B3_MOD_WDVT1:             
        model->B3wdvt1 = value->rValue;
        model->B3wdvt1Given = true;
        break;
    case B3_MOD_WDVT2:             
        model->B3wdvt2 = value->rValue;
        model->B3wdvt2Given = true;
        break;
    case B3_MOD_WDVT0W:               
        model->B3wdvt0w = value->rValue;
        model->B3wdvt0wGiven = true;
        break;
    case B3_MOD_WDVT1W:             
        model->B3wdvt1w = value->rValue;
        model->B3wdvt1wGiven = true;
        break;
    case B3_MOD_WDVT2W:             
        model->B3wdvt2w = value->rValue;
        model->B3wdvt2wGiven = true;
        break;
    case B3_MOD_WDROUT:             
        model->B3wdrout = value->rValue;
        model->B3wdroutGiven = true;
        break;
    case B3_MOD_WDSUB:             
        model->B3wdsub = value->rValue;
        model->B3wdsubGiven = true;
        break;
    case B3_MOD_WVTH0:
        model->B3wvth0 = value->rValue;
        model->B3wvth0Given = true;
        break;
    case B3_MOD_WUA:
        model->B3wua = value->rValue;
        model->B3wuaGiven = true;
        break;
    case B3_MOD_WUA1:
        model->B3wua1 = value->rValue;
        model->B3wua1Given = true;
        break;
    case B3_MOD_WUB:
        model->B3wub = value->rValue;
        model->B3wubGiven = true;
        break;
    case B3_MOD_WUB1:
        model->B3wub1 = value->rValue;
        model->B3wub1Given = true;
        break;
    case B3_MOD_WUC:
        model->B3wuc = value->rValue;
        model->B3wucGiven = true;
        break;
    case B3_MOD_WUC1:
        model->B3wuc1 = value->rValue;
        model->B3wuc1Given = true;
        break;
    case B3_MOD_WU0:
        model->B3wu0 = value->rValue;
        model->B3wu0Given = true;
        break;
    case B3_MOD_WUTE:
        model->B3wute = value->rValue;
        model->B3wuteGiven = true;
        break;
    case B3_MOD_WVOFF:
        model->B3wvoff = value->rValue;
        model->B3wvoffGiven = true;
        break;
    case B3_MOD_WDELTA:
        model->B3wdelta = value->rValue;
        model->B3wdeltaGiven = true;
        break;
    case B3_MOD_WRDSW:
        model->B3wrdsw = value->rValue;
        model->B3wrdswGiven = true;
        break;                     
    case B3_MOD_WPRWB:
        model->B3wprwb = value->rValue;
        model->B3wprwbGiven = true;
        break;                     
    case B3_MOD_WPRWG:
        model->B3wprwg = value->rValue;
        model->B3wprwgGiven = true;
        break;                     
    case B3_MOD_WPRT:
        model->B3wprt = value->rValue;
        model->B3wprtGiven = true;
        break;                     
    case B3_MOD_WETA0:
        model->B3weta0 = value->rValue;
        model->B3weta0Given = true;
        break;                 
    case B3_MOD_WETAB:
        model->B3wetab = value->rValue;
        model->B3wetabGiven = true;
        break;                 
    case B3_MOD_WPCLM:
        model->B3wpclm = value->rValue;
        model->B3wpclmGiven = true;
        break;                 
    case B3_MOD_WPDIBL1:
        model->B3wpdibl1 = value->rValue;
        model->B3wpdibl1Given = true;
        break;                 
    case B3_MOD_WPDIBL2:
        model->B3wpdibl2 = value->rValue;
        model->B3wpdibl2Given = true;
        break;                 
    case B3_MOD_WPDIBLB:
        model->B3wpdiblb = value->rValue;
        model->B3wpdiblbGiven = true;
        break;                 
    case B3_MOD_WPSCBE1:
        model->B3wpscbe1 = value->rValue;
        model->B3wpscbe1Given = true;
        break;                 
    case B3_MOD_WPSCBE2:
        model->B3wpscbe2 = value->rValue;
        model->B3wpscbe2Given = true;
        break;                 
    case B3_MOD_WPVAG:
        model->B3wpvag = value->rValue;
        model->B3wpvagGiven = true;
        break;                 
    case B3_MOD_WWR:
        model->B3wwr = value->rValue;
        model->B3wwrGiven = true;
        break;
    case B3_MOD_WDWG:
        model->B3wdwg = value->rValue;
        model->B3wdwgGiven = true;
        break;
    case B3_MOD_WDWB:
        model->B3wdwb = value->rValue;
        model->B3wdwbGiven = true;
        break;
    case B3_MOD_WB0:
        model->B3wb0 = value->rValue;
        model->B3wb0Given = true;
        break;
    case B3_MOD_WB1:
        model->B3wb1 = value->rValue;
        model->B3wb1Given = true;
        break;
    case B3_MOD_WALPHA0:
        model->B3walpha0 = value->rValue;
        model->B3walpha0Given = true;
        break;
    case B3_MOD_WALPHA1:
        model->B3walpha1 = value->rValue;
        model->B3walpha1Given = true;
        break;
    case B3_MOD_WBETA0:
        model->B3wbeta0 = value->rValue;
        model->B3wbeta0Given = true;
        break;
    case B3_MOD_WVFB:
        model->B3wvfb = value->rValue;
        model->B3wvfbGiven = true;
        break;

    case B3_MOD_WELM:
        model->B3welm = value->rValue;
        model->B3welmGiven = true;
        break;
    case B3_MOD_WCGSL:
        model->B3wcgsl = value->rValue;
        model->B3wcgslGiven = true;
        break;
    case B3_MOD_WCGDL:
        model->B3wcgdl = value->rValue;
        model->B3wcgdlGiven = true;
        break;
    case B3_MOD_WCKAPPA:
        model->B3wckappa = value->rValue;
        model->B3wckappaGiven = true;
        break;
    case B3_MOD_WCF:
        model->B3wcf = value->rValue;
        model->B3wcfGiven = true;
        break;
    case B3_MOD_WCLC:
        model->B3wclc = value->rValue;
        model->B3wclcGiven = true;
        break;
    case B3_MOD_WCLE:
        model->B3wcle = value->rValue;
        model->B3wcleGiven = true;
        break;
    case B3_MOD_WVFBCV:
        model->B3wvfbcv = value->rValue;
        model->B3wvfbcvGiven = true;
        break;
    case B3_MOD_WACDE:
        model->B3wacde = value->rValue;
        model->B3wacdeGiven = true;
        break;
    case B3_MOD_WMOIN:
        model->B3wmoin = value->rValue;
        model->B3wmoinGiven = true;
        break;
    case B3_MOD_WNOFF:
        model->B3wnoff = value->rValue;
        model->B3wnoffGiven = true;
        break;
    case B3_MOD_WVOFFCV:
        model->B3wvoffcv = value->rValue;
        model->B3wvoffcvGiven = true;
        break;

    // Cross-term dependence
    case B3_MOD_PCDSC:
        model->B3pcdsc = value->rValue;
        model->B3pcdscGiven = true;
        break;


    case B3_MOD_PCDSCB:
        model->B3pcdscb = value->rValue;
        model->B3pcdscbGiven = true;
        break;
    case B3_MOD_PCDSCD:
        model->B3pcdscd = value->rValue;
        model->B3pcdscdGiven = true;
        break;
    case B3_MOD_PCIT:
        model->B3pcit = value->rValue;
        model->B3pcitGiven = true;
        break;
    case B3_MOD_PNFACTOR:
        model->B3pnfactor = value->rValue;
        model->B3pnfactorGiven = true;
        break;
    case B3_MOD_PXJ:
        model->B3pxj = value->rValue;
        model->B3pxjGiven = true;
        break;
    case B3_MOD_PVSAT:
        model->B3pvsat = value->rValue;
        model->B3pvsatGiven = true;
        break;


    case B3_MOD_PA0:
        model->B3pa0 = value->rValue;
        model->B3pa0Given = true;
        break;
    case B3_MOD_PAGS:
        model->B3pags = value->rValue;
        model->B3pagsGiven = true;
        break;
    case B3_MOD_PA1:
        model->B3pa1 = value->rValue;
        model->B3pa1Given = true;
        break;
    case B3_MOD_PA2:
        model->B3pa2 = value->rValue;
        model->B3pa2Given = true;
        break;
    case B3_MOD_PAT:
        model->B3pat = value->rValue;
        model->B3patGiven = true;
        break;
    case B3_MOD_PKETA:
        model->B3pketa = value->rValue;
        model->B3pketaGiven = true;
        break;    
    case B3_MOD_PNSUB:
        model->B3pnsub = value->rValue;
        model->B3pnsubGiven = true;
        break;
    case B3_MOD_PNPEAK:
        model->B3pnpeak = value->rValue;
        model->B3pnpeakGiven = true;
        if (model->B3pnpeak > 1.0e20)
            model->B3pnpeak *= 1.0e-6;
        break;
    case B3_MOD_PNGATE:
        model->B3pngate = value->rValue;
        model->B3pngateGiven = true;
        if (model->B3pngate > 1.0e23)
            model->B3pngate *= 1.0e-6;
        break;
    case B3_MOD_PGAMMA1:
        model->B3pgamma1 = value->rValue;
        model->B3pgamma1Given = true;
        break;
    case B3_MOD_PGAMMA2:
        model->B3pgamma2 = value->rValue;
        model->B3pgamma2Given = true;
        break;
    case B3_MOD_PVBX:
        model->B3pvbx = value->rValue;
        model->B3pvbxGiven = true;
        break;
    case B3_MOD_PVBM:
        model->B3pvbm = value->rValue;
        model->B3pvbmGiven = true;
        break;
    case B3_MOD_PXT:
        model->B3pxt = value->rValue;
        model->B3pxtGiven = true;
        break;
    case B3_MOD_PK1:
        model->B3pk1 = value->rValue;
        model->B3pk1Given = true;
        break;
    case B3_MOD_PKT1:
        model->B3pkt1 = value->rValue;
        model->B3pkt1Given = true;
        break;
    case B3_MOD_PKT1L:
        model->B3pkt1l = value->rValue;
        model->B3pkt1lGiven = true;
        break;
    case B3_MOD_PKT2:
        model->B3pkt2 = value->rValue;
        model->B3pkt2Given = true;
        break;
    case B3_MOD_PK2:
        model->B3pk2 = value->rValue;
        model->B3pk2Given = true;
        break;
    case B3_MOD_PK3:
        model->B3pk3 = value->rValue;
        model->B3pk3Given = true;
        break;
    case B3_MOD_PK3B:
        model->B3pk3b = value->rValue;
        model->B3pk3bGiven = true;
        break;
    case B3_MOD_PNLX:
        model->B3pnlx = value->rValue;
        model->B3pnlxGiven = true;
        break;
    case B3_MOD_PW0:
        model->B3pw0 = value->rValue;
        model->B3pw0Given = true;
        break;
    case B3_MOD_PDVT0:               
        model->B3pdvt0 = value->rValue;
        model->B3pdvt0Given = true;
        break;
    case B3_MOD_PDVT1:             
        model->B3pdvt1 = value->rValue;
        model->B3pdvt1Given = true;
        break;
    case B3_MOD_PDVT2:             
        model->B3pdvt2 = value->rValue;
        model->B3pdvt2Given = true;
        break;
    case B3_MOD_PDVT0W:               
        model->B3pdvt0w = value->rValue;
        model->B3pdvt0wGiven = true;
        break;
    case B3_MOD_PDVT1W:             
        model->B3pdvt1w = value->rValue;
        model->B3pdvt1wGiven = true;
        break;
    case B3_MOD_PDVT2W:             
        model->B3pdvt2w = value->rValue;
        model->B3pdvt2wGiven = true;
        break;
    case B3_MOD_PDROUT:             
        model->B3pdrout = value->rValue;
        model->B3pdroutGiven = true;
        break;
    case B3_MOD_PDSUB:             
        model->B3pdsub = value->rValue;
        model->B3pdsubGiven = true;
        break;
    case B3_MOD_PVTH0:
        model->B3pvth0 = value->rValue;
        model->B3pvth0Given = true;
        break;
    case B3_MOD_PUA:
        model->B3pua = value->rValue;
        model->B3puaGiven = true;
        break;
    case B3_MOD_PUA1:
        model->B3pua1 = value->rValue;
        model->B3pua1Given = true;
        break;
    case B3_MOD_PUB:
        model->B3pub = value->rValue;
        model->B3pubGiven = true;
        break;
    case B3_MOD_PUB1:
        model->B3pub1 = value->rValue;
        model->B3pub1Given = true;
        break;
    case B3_MOD_PUC:
        model->B3puc = value->rValue;
        model->B3pucGiven = true;
        break;
    case B3_MOD_PUC1:
        model->B3puc1 = value->rValue;
        model->B3puc1Given = true;
        break;
    case B3_MOD_PU0:
        model->B3pu0 = value->rValue;
        model->B3pu0Given = true;
        break;
    case B3_MOD_PUTE:
        model->B3pute = value->rValue;
        model->B3puteGiven = true;
        break;
    case B3_MOD_PVOFF:
        model->B3pvoff = value->rValue;
        model->B3pvoffGiven = true;
        break;
    case B3_MOD_PDELTA:
        model->B3pdelta = value->rValue;
        model->B3pdeltaGiven = true;
        break;
    case B3_MOD_PRDSW:
        model->B3prdsw = value->rValue;
        model->B3prdswGiven = true;
        break;                     
    case B3_MOD_PPRWB:
        model->B3pprwb = value->rValue;
        model->B3pprwbGiven = true;
        break;                     
    case B3_MOD_PPRWG:
        model->B3pprwg = value->rValue;
        model->B3pprwgGiven = true;
        break;                     
    case B3_MOD_PPRT:
        model->B3pprt = value->rValue;
        model->B3pprtGiven = true;
        break;                     
    case B3_MOD_PETA0:
        model->B3peta0 = value->rValue;
        model->B3peta0Given = true;
        break;                 
    case B3_MOD_PETAB:
        model->B3petab = value->rValue;
        model->B3petabGiven = true;
        break;                 
    case B3_MOD_PPCLM:
        model->B3ppclm = value->rValue;
        model->B3ppclmGiven = true;
        break;                 
    case B3_MOD_PPDIBL1:
        model->B3ppdibl1 = value->rValue;
        model->B3ppdibl1Given = true;
        break;                 
    case B3_MOD_PPDIBL2:
        model->B3ppdibl2 = value->rValue;
        model->B3ppdibl2Given = true;
        break;                 
    case B3_MOD_PPDIBLB:
        model->B3ppdiblb = value->rValue;
        model->B3ppdiblbGiven = true;
        break;                 
    case B3_MOD_PPSCBE1:
        model->B3ppscbe1 = value->rValue;
        model->B3ppscbe1Given = true;
        break;                 
    case B3_MOD_PPSCBE2:
        model->B3ppscbe2 = value->rValue;
        model->B3ppscbe2Given = true;
        break;                 
    case B3_MOD_PPVAG:
        model->B3ppvag = value->rValue;
        model->B3ppvagGiven = true;
        break;                 
    case B3_MOD_PWR:
        model->B3pwr = value->rValue;
        model->B3pwrGiven = true;
        break;
    case B3_MOD_PDWG:
        model->B3pdwg = value->rValue;
        model->B3pdwgGiven = true;
        break;
    case B3_MOD_PDWB:
        model->B3pdwb = value->rValue;
        model->B3pdwbGiven = true;
        break;
    case B3_MOD_PB0:
        model->B3pb0 = value->rValue;
        model->B3pb0Given = true;
        break;
    case B3_MOD_PB1:
        model->B3pb1 = value->rValue;
        model->B3pb1Given = true;
        break;
    case B3_MOD_PALPHA0:
        model->B3palpha0 = value->rValue;
        model->B3palpha0Given = true;
        break;
    case B3_MOD_PALPHA1:
        model->B3palpha1 = value->rValue;
        model->B3palpha1Given = true;
        break;
    case B3_MOD_PBETA0:
        model->B3pbeta0 = value->rValue;
        model->B3pbeta0Given = true;
        break;

    case B3_MOD_PELM:
        model->B3pelm = value->rValue;
        model->B3pelmGiven = true;
        break;
    case B3_MOD_PCGSL:
        model->B3pcgsl = value->rValue;
        model->B3pcgslGiven = true;
        break;
    case B3_MOD_PCGDL:
        model->B3pcgdl = value->rValue;
        model->B3pcgdlGiven = true;
        break;
    case B3_MOD_PCKAPPA:
        model->B3pckappa = value->rValue;
        model->B3pckappaGiven = true;
        break;
    case B3_MOD_PCF:
        model->B3pcf = value->rValue;
        model->B3pcfGiven = true;
        break;
    case B3_MOD_PCLC:
        model->B3pclc = value->rValue;
        model->B3pclcGiven = true;
        break;
    case B3_MOD_PCLE:
        model->B3pcle = value->rValue;
        model->B3pcleGiven = true;
        break;
    case B3_MOD_PVFBCV:
        model->B3pvfbcv = value->rValue;
        model->B3pvfbcvGiven = true;
        break;
    case B3_MOD_PACDE:
        model->B3pacde = value->rValue;
        model->B3pacdeGiven = true;
        break;
    case B3_MOD_PMOIN:
        model->B3pmoin = value->rValue;
        model->B3pmoinGiven = true;
        break;
    case B3_MOD_PNOFF:
        model->B3pnoff = value->rValue;
        model->B3pnoffGiven = true;
        break;
    case B3_MOD_PVOFFCV:
        model->B3pvoffcv = value->rValue;
        model->B3pvoffcvGiven = true;
        break;

    case B3_MOD_TNOM:
        model->B3tnom = value->rValue + CONSTCtoK;
        model->B3tnomGiven = true;
        break;
    case B3_MOD_CGSO:
        model->B3cgso = value->rValue;
        model->B3cgsoGiven = true;
        break;
    case B3_MOD_CGDO:
        model->B3cgdo = value->rValue;
        model->B3cgdoGiven = true;
        break;
    case B3_MOD_CGBO:
        model->B3cgbo = value->rValue;
        model->B3cgboGiven = true;
        break;
    case B3_MOD_XPART:
        model->B3xpart = value->rValue;
        model->B3xpartGiven = true;
        break;
    case B3_MOD_RSH:
        model->B3sheetResistance = value->rValue;
        model->B3sheetResistanceGiven = true;
        break;
    case B3_MOD_JS:
        model->B3jctSatCurDensity = value->rValue;
        model->B3jctSatCurDensityGiven = true;
        break;
    case B3_MOD_JSW:
        model->B3jctSidewallSatCurDensity = value->rValue;
        model->B3jctSidewallSatCurDensityGiven = true;
        break;
    case B3_MOD_PB:
        model->B3bulkJctPotential = value->rValue;
        model->B3bulkJctPotentialGiven = true;
        break;
    case B3_MOD_MJ:
        model->B3bulkJctBotGradingCoeff = value->rValue;
        model->B3bulkJctBotGradingCoeffGiven = true;
        break;
    case B3_MOD_PBSW:
        model->B3sidewallJctPotential = value->rValue;
        model->B3sidewallJctPotentialGiven = true;
        break;
    case B3_MOD_MJSW:
        model->B3bulkJctSideGradingCoeff = value->rValue;
        model->B3bulkJctSideGradingCoeffGiven = true;
        break;
    case B3_MOD_CJ:
        model->B3unitAreaJctCap = value->rValue;
        model->B3unitAreaJctCapGiven = true;
        break;
    case B3_MOD_CJSW:
        model->B3unitLengthSidewallJctCap = value->rValue;
        model->B3unitLengthSidewallJctCapGiven = true;
        break;
    case B3_MOD_NJ:
        model->B3jctEmissionCoeff = value->rValue;
        model->B3jctEmissionCoeffGiven = true;
        break;
    case B3_MOD_PBSWG:
        model->B3GatesidewallJctPotential = value->rValue;
        model->B3GatesidewallJctPotentialGiven = true;
        break;
    case B3_MOD_MJSWG:
        model->B3bulkJctGateSideGradingCoeff = value->rValue;
        model->B3bulkJctGateSideGradingCoeffGiven = true;
        break;
    case B3_MOD_CJSWG:
        model->B3unitLengthGateSidewallJctCap = value->rValue;
        model->B3unitLengthGateSidewallJctCapGiven = true;
        break;
    case B3_MOD_XTI:
        model->B3jctTempExponent = value->rValue;
        model->B3jctTempExponentGiven = true;
        break;
    case B3_MOD_LINT:
        model->B3Lint = value->rValue;
        model->B3LintGiven = true;
        break;
    case B3_MOD_LL:
        model->B3Ll = value->rValue;
        model->B3LlGiven = true;
        break;
    case B3_MOD_LLC:
        model->B3Llc = value->rValue;
        model->B3LlcGiven = true;
        break;
    case B3_MOD_LLN:
        model->B3Lln = value->rValue;
        model->B3LlnGiven = true;
        break;
    case B3_MOD_LW:
        model->B3Lw = value->rValue;
        model->B3LwGiven = true;
        break;
    case B3_MOD_LWC:
        model->B3Lwc = value->rValue;
        model->B3LwcGiven = true;
        break;
    case B3_MOD_LWN:
        model->B3Lwn = value->rValue;
        model->B3LwnGiven = true;
        break;
    case B3_MOD_LWL:
        model->B3Lwl = value->rValue;
        model->B3LwlGiven = true;
        break;
    case B3_MOD_LWLC:
        model->B3Lwlc = value->rValue;
        model->B3LwlcGiven = true;
        break;
    case B3_MOD_LMIN:
        model->B3Lmin = value->rValue;
        model->B3LminGiven = true;
        break;
    case B3_MOD_LMAX:
        model->B3Lmax = value->rValue;
        model->B3LmaxGiven = true;
        break;
    case B3_MOD_WINT:
        model->B3Wint = value->rValue;
        model->B3WintGiven = true;
        break;
    case B3_MOD_WL:
        model->B3Wl = value->rValue;
        model->B3WlGiven = true;
        break;
    case B3_MOD_WLC:
        model->B3Wlc = value->rValue;
        model->B3WlcGiven = true;
        break;
    case B3_MOD_WLN:
        model->B3Wln = value->rValue;
        model->B3WlnGiven = true;
        break;
    case B3_MOD_WW:
        model->B3Ww = value->rValue;
        model->B3WwGiven = true;
        break;
    case B3_MOD_WWC:
        model->B3Wwc = value->rValue;
        model->B3WwcGiven = true;
        break;
    case B3_MOD_WWN:
        model->B3Wwn = value->rValue;
        model->B3WwnGiven = true;
        break;
    case B3_MOD_WWL:
        model->B3Wwl = value->rValue;
        model->B3WwlGiven = true;
        break;
    case B3_MOD_WWLC:
        model->B3Wwlc = value->rValue;
        model->B3WwlcGiven = true;
        break;
    case B3_MOD_WMIN:
        model->B3Wmin = value->rValue;
        model->B3WminGiven = true;
        break;
    case B3_MOD_WMAX:
        model->B3Wmax = value->rValue;
        model->B3WmaxGiven = true;
        break;

    case B3_MOD_NOIA:
        model->B3oxideTrapDensityA = value->rValue;
        model->B3oxideTrapDensityAGiven = true;
        break;
    case B3_MOD_NOIB:
        model->B3oxideTrapDensityB = value->rValue;
        model->B3oxideTrapDensityBGiven = true;
        break;
    case B3_MOD_NOIC:
        model->B3oxideTrapDensityC = value->rValue;
        model->B3oxideTrapDensityCGiven = true;
        break;
    case B3_MOD_EM:
        model->B3em = value->rValue;
        model->B3emGiven = true;
        break;
    case B3_MOD_EF:
        model->B3ef = value->rValue;
        model->B3efGiven = true;
        break;
    case B3_MOD_AF:
        model->B3af = value->rValue;
        model->B3afGiven = true;
        break;
    case B3_MOD_KF:
        model->B3kf = value->rValue;
        model->B3kfGiven = true;
        break;

    case B3_MOD_DMPLOG:
        model->B3dumpLog = true;
        break;

    case B3_MOD_NMOS:
        if(value->iValue) {
            model->B3type = 1;
            model->B3typeGiven = true;
        }
        break;
    case B3_MOD_PMOS:
        if(value->iValue) {
            model->B3type = - 1;
            model->B3typeGiven = true;
        }
        break;
// SRW
    case B3_MOD_NQSMOD:
        model->B3nqsMod = value->iValue;
        model->B3nqsModGiven = true;
        break;
    case B3_MOD_XW:
        model->B3xw = value->rValue;
        model->B3xwGiven = true;
        break;
    case B3_MOD_XL:
        model->B3xl = value->rValue;
        model->B3xlGiven = true;
        break;

    default:
        return (E_BADPARM);
    }
    return (OK);
}


