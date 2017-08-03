
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


int
B3dev::askModl(const sGENmodel *genmod, int which, IFdata *data)
{
    const sB3model *model = static_cast<const sB3model*>(genmod);
    IFvalue *value = &data->v;
    // Need to override this for non-real returns.
    data->type = IF_REAL;

    switch(which) {
    case B3_MOD_MOBMOD:
        value->iValue = model->B3mobMod; 
        data->type = IF_INTEGER;
        break;
    case B3_MOD_PARAMCHK:
        value->iValue = model->B3paramChk; 
        data->type = IF_INTEGER;
        break;
    case B3_MOD_BINUNIT:
        value->iValue = model->B3binUnit; 
        data->type = IF_INTEGER;
        break;
    case B3_MOD_CAPMOD:
        value->iValue = model->B3capMod; 
        data->type = IF_INTEGER;
        break;
    case B3_MOD_NOIMOD:
        value->iValue = model->B3noiMod; 
        data->type = IF_INTEGER;
        break;
    case  B3_MOD_VERSION :
        value->rValue = model->B3version;
        break;
    case  B3_MOD_TOX :
        value->rValue = model->B3tox;
        break;
    case  B3_MOD_TOXM :
        value->rValue = model->B3toxm;
        break;
    case  B3_MOD_CDSC :
        value->rValue = model->B3cdsc;
        break;
    case  B3_MOD_CDSCB :
        value->rValue = model->B3cdscb;
        break;

    case  B3_MOD_CDSCD :
        value->rValue = model->B3cdscd;
        break;

    case  B3_MOD_CIT :
        value->rValue = model->B3cit;
        break;
    case  B3_MOD_NFACTOR :
        value->rValue = model->B3nfactor;
        break;
    case B3_MOD_XJ:
        value->rValue = model->B3xj;
        break;
    case B3_MOD_VSAT:
        value->rValue = model->B3vsat;
        break;
    case B3_MOD_AT:
        value->rValue = model->B3at;
        break;
    case B3_MOD_A0:
        value->rValue = model->B3a0;
        break;

    case B3_MOD_AGS:
        value->rValue = model->B3ags;
        break;

    case B3_MOD_A1:
        value->rValue = model->B3a1;
        break;
    case B3_MOD_A2:
        value->rValue = model->B3a2;
        break;
    case B3_MOD_KETA:
        value->rValue = model->B3keta;
        break;   
    case B3_MOD_NSUB:
        value->rValue = model->B3nsub;
        break;
    case B3_MOD_NPEAK:
        value->rValue = model->B3npeak;
        break;
    case B3_MOD_NGATE:
        value->rValue = model->B3ngate;
        break;
    case B3_MOD_GAMMA1:
        value->rValue = model->B3gamma1;
        break;
    case B3_MOD_GAMMA2:
        value->rValue = model->B3gamma2;
        break;
    case B3_MOD_VBX:
        value->rValue = model->B3vbx;
        break;
    case B3_MOD_VBM:
        value->rValue = model->B3vbm;
        break;
    case B3_MOD_XT:
        value->rValue = model->B3xt;
        break;
    case  B3_MOD_K1:
        value->rValue = model->B3k1;
        break;
    case  B3_MOD_KT1:
        value->rValue = model->B3kt1;
        break;
    case  B3_MOD_KT1L:
        value->rValue = model->B3kt1l;
        break;
    case  B3_MOD_KT2 :
        value->rValue = model->B3kt2;
        break;
    case  B3_MOD_K2 :
        value->rValue = model->B3k2;
        break;
    case  B3_MOD_K3:
        value->rValue = model->B3k3;
        break;
    case  B3_MOD_K3B:
        value->rValue = model->B3k3b;
        break;
    case  B3_MOD_W0:
        value->rValue = model->B3w0;
        break;
    case  B3_MOD_NLX:
        value->rValue = model->B3nlx;
        break;
    case  B3_MOD_DVT0 :                
        value->rValue = model->B3dvt0;
        break;
    case  B3_MOD_DVT1 :             
        value->rValue = model->B3dvt1;
        break;
    case  B3_MOD_DVT2 :             
        value->rValue = model->B3dvt2;
        break;
    case  B3_MOD_DVT0W :                
        value->rValue = model->B3dvt0w;
        break;
    case  B3_MOD_DVT1W :             
        value->rValue = model->B3dvt1w;
        break;
    case  B3_MOD_DVT2W :             
        value->rValue = model->B3dvt2w;
        break;
    case  B3_MOD_DROUT :           
        value->rValue = model->B3drout;
        break;
    case  B3_MOD_DSUB :           
        value->rValue = model->B3dsub;
        break;
    case B3_MOD_VTH0:
        value->rValue = model->B3vth0; 
        break;
    case B3_MOD_UA:
        value->rValue = model->B3ua; 
        break;
    case B3_MOD_UA1:
        value->rValue = model->B3ua1; 
        break;
    case B3_MOD_UB:
        value->rValue = model->B3ub;  
        break;
    case B3_MOD_UB1:
        value->rValue = model->B3ub1;  
        break;
    case B3_MOD_UC:
        value->rValue = model->B3uc; 
        break;
    case B3_MOD_UC1:
        value->rValue = model->B3uc1; 
        break;
    case B3_MOD_U0:
        value->rValue = model->B3u0;
        break;
    case B3_MOD_UTE:
        value->rValue = model->B3ute;
        break;
    case B3_MOD_VOFF:
        value->rValue = model->B3voff;
        break;
    case B3_MOD_DELTA:
        value->rValue = model->B3delta;
        break;
    case B3_MOD_RDSW:
        value->rValue = model->B3rdsw; 
        break;             
    case B3_MOD_PRWG:
        value->rValue = model->B3prwg; 
        break;             
    case B3_MOD_PRWB:
        value->rValue = model->B3prwb; 
        break;             
    case B3_MOD_PRT:
        value->rValue = model->B3prt; 
        break;              
    case B3_MOD_ETA0:
        value->rValue = model->B3eta0; 
        break;               
    case B3_MOD_ETAB:
        value->rValue = model->B3etab; 
        break;               
    case B3_MOD_PCLM:
        value->rValue = model->B3pclm; 
        break;               
    case B3_MOD_PDIBL1:
        value->rValue = model->B3pdibl1; 
        break;               
    case B3_MOD_PDIBL2:
        value->rValue = model->B3pdibl2; 
        break;               
    case B3_MOD_PDIBLB:
        value->rValue = model->B3pdiblb; 
        break;               
    case B3_MOD_PSCBE1:
        value->rValue = model->B3pscbe1; 
        break;               
    case B3_MOD_PSCBE2:
        value->rValue = model->B3pscbe2; 
        break;               
    case B3_MOD_PVAG:
        value->rValue = model->B3pvag; 
        break;               
    case B3_MOD_WR:
        value->rValue = model->B3wr;
        break;
    case B3_MOD_DWG:
        value->rValue = model->B3dwg;
        break;
    case B3_MOD_DWB:
        value->rValue = model->B3dwb;
        break;
    case B3_MOD_B0:
        value->rValue = model->B3b0;
        break;
    case B3_MOD_B1:
        value->rValue = model->B3b1;
        break;
    case B3_MOD_ALPHA0:
        value->rValue = model->B3alpha0;
        break;
    case B3_MOD_ALPHA1:
        value->rValue = model->B3alpha1;
        break;
    case B3_MOD_BETA0:
        value->rValue = model->B3beta0;
        break;
    case B3_MOD_IJTH:
        value->rValue = model->B3ijth;
        break;
    case B3_MOD_VFB:
        value->rValue = model->B3vfb;
        break;

    case B3_MOD_ELM:
        value->rValue = model->B3elm;
        break;
    case B3_MOD_CGSL:
        value->rValue = model->B3cgsl;
        break;
    case B3_MOD_CGDL:
        value->rValue = model->B3cgdl;
        break;
    case B3_MOD_CKAPPA:
        value->rValue = model->B3ckappa;
        break;
    case B3_MOD_CF:
        value->rValue = model->B3cf;
        break;
    case B3_MOD_CLC:
        value->rValue = model->B3clc;
        break;
    case B3_MOD_CLE:
        value->rValue = model->B3cle;
        break;
    case B3_MOD_DWC:
        value->rValue = model->B3dwc;
        break;
    case B3_MOD_DLC:
        value->rValue = model->B3dlc;
        break;
    case B3_MOD_VFBCV:
        value->rValue = model->B3vfbcv; 
        break;
    case B3_MOD_ACDE:
        value->rValue = model->B3acde;
        break;
    case B3_MOD_MOIN:
        value->rValue = model->B3moin;
        break;
    case B3_MOD_NOFF:
        value->rValue = model->B3noff;
        break;
    case B3_MOD_VOFFCV:
        value->rValue = model->B3voffcv;
        break;
    case B3_MOD_TCJ:
        value->rValue = model->B3tcj;
        break;
    case B3_MOD_TPB:
        value->rValue = model->B3tpb;
        break;
    case B3_MOD_TCJSW:
        value->rValue = model->B3tcjsw;
        break;
    case B3_MOD_TPBSW:
        value->rValue = model->B3tpbsw;
        break;
    case B3_MOD_TCJSWG:
        value->rValue = model->B3tcjswg;
        break;
    case B3_MOD_TPBSWG:
        value->rValue = model->B3tpbswg;
        break;

    // Length dependence

    case  B3_MOD_LCDSC :
      value->rValue = model->B3lcdsc;
        break;
    case  B3_MOD_LCDSCB :
      value->rValue = model->B3lcdscb;
        break;
    case  B3_MOD_LCDSCD :
      value->rValue = model->B3lcdscd;
        break;
    case  B3_MOD_LCIT :
      value->rValue = model->B3lcit;
        break;
    case  B3_MOD_LNFACTOR :
      value->rValue = model->B3lnfactor;
        break;
    case B3_MOD_LXJ:
        value->rValue = model->B3lxj;
        break;
    case B3_MOD_LVSAT:
        value->rValue = model->B3lvsat;
        break;
    case B3_MOD_LAT:
        value->rValue = model->B3lat;
        break;
    case B3_MOD_LA0:
        value->rValue = model->B3la0;
        break;
    case B3_MOD_LAGS:
        value->rValue = model->B3lags;
        break;
    case B3_MOD_LA1:
        value->rValue = model->B3la1;
        break;
    case B3_MOD_LA2:
        value->rValue = model->B3la2;
        break;
    case B3_MOD_LKETA:
        value->rValue = model->B3lketa;
        break;   
    case B3_MOD_LNSUB:
        value->rValue = model->B3lnsub;
        break;
    case B3_MOD_LNPEAK:
        value->rValue = model->B3lnpeak;
        break;
    case B3_MOD_LNGATE:
        value->rValue = model->B3lngate;
        break;
    case B3_MOD_LGAMMA1:
        value->rValue = model->B3lgamma1;
        break;
    case B3_MOD_LGAMMA2:
        value->rValue = model->B3lgamma2;
        break;
    case B3_MOD_LVBX:
        value->rValue = model->B3lvbx;
        break;
    case B3_MOD_LVBM:
        value->rValue = model->B3lvbm;
        break;
    case B3_MOD_LXT:
        value->rValue = model->B3lxt;
        break;
    case  B3_MOD_LK1:
      value->rValue = model->B3lk1;
        break;
    case  B3_MOD_LKT1:
      value->rValue = model->B3lkt1;
        break;
    case  B3_MOD_LKT1L:
      value->rValue = model->B3lkt1l;
        break;
    case  B3_MOD_LKT2 :
      value->rValue = model->B3lkt2;
        break;
    case  B3_MOD_LK2 :
      value->rValue = model->B3lk2;
        break;
    case  B3_MOD_LK3:
      value->rValue = model->B3lk3;
        break;
    case  B3_MOD_LK3B:
      value->rValue = model->B3lk3b;
        break;
    case  B3_MOD_LW0:
      value->rValue = model->B3lw0;
        break;
    case  B3_MOD_LNLX:
      value->rValue = model->B3lnlx;
        break;
    case  B3_MOD_LDVT0:                
      value->rValue = model->B3ldvt0;
        break;
    case  B3_MOD_LDVT1 :             
      value->rValue = model->B3ldvt1;
        break;
    case  B3_MOD_LDVT2 :             
      value->rValue = model->B3ldvt2;
        break;
    case  B3_MOD_LDVT0W :                
      value->rValue = model->B3ldvt0w;
        break;
    case  B3_MOD_LDVT1W :             
      value->rValue = model->B3ldvt1w;
        break;
    case  B3_MOD_LDVT2W :             
      value->rValue = model->B3ldvt2w;
        break;
    case  B3_MOD_LDROUT :           
      value->rValue = model->B3ldrout;
        break;
    case  B3_MOD_LDSUB :           
      value->rValue = model->B3ldsub;
        break;
    case B3_MOD_LVTH0:
        value->rValue = model->B3lvth0; 
        break;
    case B3_MOD_LUA:
        value->rValue = model->B3lua; 
        break;
    case B3_MOD_LUA1:
        value->rValue = model->B3lua1; 
        break;
    case B3_MOD_LUB:
        value->rValue = model->B3lub;  
        break;
    case B3_MOD_LUB1:
        value->rValue = model->B3lub1;  
        break;
    case B3_MOD_LUC:
        value->rValue = model->B3luc; 
        break;
    case B3_MOD_LUC1:
        value->rValue = model->B3luc1; 
        break;
    case B3_MOD_LU0:
        value->rValue = model->B3lu0;
        break;
    case B3_MOD_LUTE:
        value->rValue = model->B3lute;
        break;
    case B3_MOD_LVOFF:
        value->rValue = model->B3lvoff;
        break;
    case B3_MOD_LDELTA:
        value->rValue = model->B3ldelta;
        break;
    case B3_MOD_LRDSW:
        value->rValue = model->B3lrdsw; 
        break;             
    case B3_MOD_LPRWB:
        value->rValue = model->B3lprwb; 
        break;             
    case B3_MOD_LPRWG:
        value->rValue = model->B3lprwg; 
        break;             
    case B3_MOD_LPRT:
        value->rValue = model->B3lprt; 
        break;              
    case B3_MOD_LETA0:
        value->rValue = model->B3leta0; 
        break;               
    case B3_MOD_LETAB:
        value->rValue = model->B3letab; 
        break;               
    case B3_MOD_LPCLM:
        value->rValue = model->B3lpclm; 
        break;               
    case B3_MOD_LPDIBL1:
        value->rValue = model->B3lpdibl1; 
        break;               
    case B3_MOD_LPDIBL2:
        value->rValue = model->B3lpdibl2; 
        break;               
    case B3_MOD_LPDIBLB:
        value->rValue = model->B3lpdiblb; 
        break;               
    case B3_MOD_LPSCBE1:
        value->rValue = model->B3lpscbe1; 
        break;               
    case B3_MOD_LPSCBE2:
        value->rValue = model->B3lpscbe2; 
        break;               
    case B3_MOD_LPVAG:
        value->rValue = model->B3lpvag; 
        break;               
    case B3_MOD_LWR:
        value->rValue = model->B3lwr;
        break;
    case B3_MOD_LDWG:
        value->rValue = model->B3ldwg;
        break;
    case B3_MOD_LDWB:
        value->rValue = model->B3ldwb;
        break;
    case B3_MOD_LB0:
        value->rValue = model->B3lb0;
        break;
    case B3_MOD_LB1:
        value->rValue = model->B3lb1;
        break;
    case B3_MOD_LALPHA0:
        value->rValue = model->B3lalpha0;
        break;
    case B3_MOD_LALPHA1:
        value->rValue = model->B3lalpha1;
        break;
    case B3_MOD_LBETA0:
        value->rValue = model->B3lbeta0;
        break;
    case B3_MOD_LVFB:
        value->rValue = model->B3lvfb;
        break;

    case B3_MOD_LELM:
        value->rValue = model->B3lelm;
        break;
    case B3_MOD_LCGSL:
        value->rValue = model->B3lcgsl;
        break;
    case B3_MOD_LCGDL:
        value->rValue = model->B3lcgdl;
        break;
    case B3_MOD_LCKAPPA:
        value->rValue = model->B3lckappa;
        break;
    case B3_MOD_LCF:
        value->rValue = model->B3lcf;
        break;
    case B3_MOD_LCLC:
        value->rValue = model->B3lclc;
        break;
    case B3_MOD_LCLE:
        value->rValue = model->B3lcle;
        break;
    case B3_MOD_LVFBCV:
        value->rValue = model->B3lvfbcv;
        break;
    case B3_MOD_LACDE:
        value->rValue = model->B3lacde;
        break;
    case B3_MOD_LMOIN:
        value->rValue = model->B3lmoin;
        break;
    case B3_MOD_LNOFF:
        value->rValue = model->B3lnoff;
        break;
    case B3_MOD_LVOFFCV:
        value->rValue = model->B3lvoffcv;
        break;

    // Width dependence

    case  B3_MOD_WCDSC :
      value->rValue = model->B3wcdsc;
        break;
    case  B3_MOD_WCDSCB :
      value->rValue = model->B3wcdscb;
        break;
    case  B3_MOD_WCDSCD :
      value->rValue = model->B3wcdscd;
        break;
    case  B3_MOD_WCIT :
      value->rValue = model->B3wcit;
        break;
    case  B3_MOD_WNFACTOR :
      value->rValue = model->B3wnfactor;
        break;
    case B3_MOD_WXJ:
        value->rValue = model->B3wxj;
        break;
    case B3_MOD_WVSAT:
        value->rValue = model->B3wvsat;
        break;
    case B3_MOD_WAT:
        value->rValue = model->B3wat;
        break;
    case B3_MOD_WA0:
        value->rValue = model->B3wa0;
        break;
    case B3_MOD_WAGS:
        value->rValue = model->B3wags;
        break;
    case B3_MOD_WA1:
        value->rValue = model->B3wa1;
        break;
    case B3_MOD_WA2:
        value->rValue = model->B3wa2;
        break;
    case B3_MOD_WKETA:
        value->rValue = model->B3wketa;
        break;   
    case B3_MOD_WNSUB:
        value->rValue = model->B3wnsub;
        break;
    case B3_MOD_WNPEAK:
        value->rValue = model->B3wnpeak;
        break;
    case B3_MOD_WNGATE:
        value->rValue = model->B3wngate;
        break;
    case B3_MOD_WGAMMA1:
        value->rValue = model->B3wgamma1;
        break;
    case B3_MOD_WGAMMA2:
        value->rValue = model->B3wgamma2;
        break;
    case B3_MOD_WVBX:
        value->rValue = model->B3wvbx;
        break;
    case B3_MOD_WVBM:
        value->rValue = model->B3wvbm;
        break;
    case B3_MOD_WXT:
        value->rValue = model->B3wxt;
        break;
    case  B3_MOD_WK1:
      value->rValue = model->B3wk1;
        break;
    case  B3_MOD_WKT1:
      value->rValue = model->B3wkt1;
        break;
    case  B3_MOD_WKT1L:
      value->rValue = model->B3wkt1l;
        break;
    case  B3_MOD_WKT2 :
      value->rValue = model->B3wkt2;
        break;
    case  B3_MOD_WK2 :
      value->rValue = model->B3wk2;
        break;
    case  B3_MOD_WK3:
      value->rValue = model->B3wk3;
        break;
    case  B3_MOD_WK3B:
      value->rValue = model->B3wk3b;
        break;
    case  B3_MOD_WW0:
      value->rValue = model->B3ww0;
        break;
    case  B3_MOD_WNLX:
      value->rValue = model->B3wnlx;
        break;
    case  B3_MOD_WDVT0:                
      value->rValue = model->B3wdvt0;
        break;
    case  B3_MOD_WDVT1 :             
      value->rValue = model->B3wdvt1;
        break;
    case  B3_MOD_WDVT2 :             
      value->rValue = model->B3wdvt2;
        break;
    case  B3_MOD_WDVT0W :                
      value->rValue = model->B3wdvt0w;
        break;
    case  B3_MOD_WDVT1W :             
      value->rValue = model->B3wdvt1w;
        break;
    case  B3_MOD_WDVT2W :             
      value->rValue = model->B3wdvt2w;
        break;
    case  B3_MOD_WDROUT :           
      value->rValue = model->B3wdrout;
        break;
    case  B3_MOD_WDSUB :           
      value->rValue = model->B3wdsub;
        break;
    case B3_MOD_WVTH0:
        value->rValue = model->B3wvth0; 
        break;
    case B3_MOD_WUA:
        value->rValue = model->B3wua; 
        break;
    case B3_MOD_WUA1:
        value->rValue = model->B3wua1; 
        break;
    case B3_MOD_WUB:
        value->rValue = model->B3wub;  
        break;
    case B3_MOD_WUB1:
        value->rValue = model->B3wub1;  
        break;
    case B3_MOD_WUC:
        value->rValue = model->B3wuc; 
        break;
    case B3_MOD_WUC1:
        value->rValue = model->B3wuc1; 
        break;
    case B3_MOD_WU0:
        value->rValue = model->B3wu0;
        break;
    case B3_MOD_WUTE:
        value->rValue = model->B3wute;
        break;
    case B3_MOD_WVOFF:
        value->rValue = model->B3wvoff;
        break;
    case B3_MOD_WDELTA:
        value->rValue = model->B3wdelta;
        break;
    case B3_MOD_WRDSW:
        value->rValue = model->B3wrdsw; 
        break;             
    case B3_MOD_WPRWB:
        value->rValue = model->B3wprwb; 
        break;             
    case B3_MOD_WPRWG:
        value->rValue = model->B3wprwg; 
        break;             
    case B3_MOD_WPRT:
        value->rValue = model->B3wprt; 
        break;              
    case B3_MOD_WETA0:
        value->rValue = model->B3weta0; 
        break;               
    case B3_MOD_WETAB:
        value->rValue = model->B3wetab; 
        break;               
    case B3_MOD_WPCLM:
        value->rValue = model->B3wpclm; 
        break;               
    case B3_MOD_WPDIBL1:
        value->rValue = model->B3wpdibl1; 
        break;               
    case B3_MOD_WPDIBL2:
        value->rValue = model->B3wpdibl2; 
        break;               
    case B3_MOD_WPDIBLB:
        value->rValue = model->B3wpdiblb; 
        break;               
    case B3_MOD_WPSCBE1:
        value->rValue = model->B3wpscbe1; 
        break;               
    case B3_MOD_WPSCBE2:
        value->rValue = model->B3wpscbe2; 
        break;               
    case B3_MOD_WPVAG:
        value->rValue = model->B3wpvag; 
        break;               
    case B3_MOD_WWR:
        value->rValue = model->B3wwr;
        break;
    case B3_MOD_WDWG:
        value->rValue = model->B3wdwg;
        break;
    case B3_MOD_WDWB:
        value->rValue = model->B3wdwb;
        break;
    case B3_MOD_WB0:
        value->rValue = model->B3wb0;
        break;
    case B3_MOD_WB1:
        value->rValue = model->B3wb1;
        break;
    case B3_MOD_WALPHA0:
        value->rValue = model->B3walpha0;
        break;
    case B3_MOD_WALPHA1:
        value->rValue = model->B3walpha1;
        break;
    case B3_MOD_WBETA0:
        value->rValue = model->B3wbeta0;
        break;
    case B3_MOD_WVFB:
        value->rValue = model->B3wvfb;
        break;

    case B3_MOD_WELM:
        value->rValue = model->B3welm;
        break;
    case B3_MOD_WCGSL:
        value->rValue = model->B3wcgsl;
        break;
    case B3_MOD_WCGDL:
        value->rValue = model->B3wcgdl;
        break;
    case B3_MOD_WCKAPPA:
        value->rValue = model->B3wckappa;
        break;
    case B3_MOD_WCF:
        value->rValue = model->B3wcf;
        break;
    case B3_MOD_WCLC:
        value->rValue = model->B3wclc;
        break;
    case B3_MOD_WCLE:
        value->rValue = model->B3wcle;
        break;
    case B3_MOD_WVFBCV:
        value->rValue = model->B3wvfbcv;
        break;
    case B3_MOD_WACDE:
        value->rValue = model->B3wacde;
        break;
    case B3_MOD_WMOIN:
        value->rValue = model->B3wmoin;
        break;
    case B3_MOD_WNOFF:
        value->rValue = model->B3wnoff;
        break;
    case B3_MOD_WVOFFCV:
        value->rValue = model->B3wvoffcv;
        break;

    // Cross-term dependence

    case  B3_MOD_PCDSC :
      value->rValue = model->B3pcdsc;
        break;
    case  B3_MOD_PCDSCB :
      value->rValue = model->B3pcdscb;
        break;
    case  B3_MOD_PCDSCD :
      value->rValue = model->B3pcdscd;
        break;
     case  B3_MOD_PCIT :
      value->rValue = model->B3pcit;
        break;
    case  B3_MOD_PNFACTOR :
      value->rValue = model->B3pnfactor;
        break;
    case B3_MOD_PXJ:
        value->rValue = model->B3pxj;
        break;
    case B3_MOD_PVSAT:
        value->rValue = model->B3pvsat;
        break;
    case B3_MOD_PAT:
        value->rValue = model->B3pat;
        break;
    case B3_MOD_PA0:
        value->rValue = model->B3pa0;
        break;
    case B3_MOD_PAGS:
        value->rValue = model->B3pags;
        break;
    case B3_MOD_PA1:
        value->rValue = model->B3pa1;
        break;
    case B3_MOD_PA2:
        value->rValue = model->B3pa2;
        break;
    case B3_MOD_PKETA:
        value->rValue = model->B3pketa;
        break;   
    case B3_MOD_PNSUB:
        value->rValue = model->B3pnsub;
        break;
    case B3_MOD_PNPEAK:
        value->rValue = model->B3pnpeak;
        break;
    case B3_MOD_PNGATE:
        value->rValue = model->B3pngate;
        break;
    case B3_MOD_PGAMMA1:
        value->rValue = model->B3pgamma1;
        break;
    case B3_MOD_PGAMMA2:
        value->rValue = model->B3pgamma2;
        break;
    case B3_MOD_PVBX:
        value->rValue = model->B3pvbx;
        break;
    case B3_MOD_PVBM:
        value->rValue = model->B3pvbm;
        break;
    case B3_MOD_PXT:
        value->rValue = model->B3pxt;
        break;
    case  B3_MOD_PK1:
      value->rValue = model->B3pk1;
        break;
    case  B3_MOD_PKT1:
      value->rValue = model->B3pkt1;
        break;
    case  B3_MOD_PKT1L:
      value->rValue = model->B3pkt1l;
        break;
    case  B3_MOD_PKT2 :
      value->rValue = model->B3pkt2;
        break;
    case  B3_MOD_PK2 :
      value->rValue = model->B3pk2;
        break;
    case  B3_MOD_PK3:
      value->rValue = model->B3pk3;
        break;
    case  B3_MOD_PK3B:
      value->rValue = model->B3pk3b;
        break;
    case  B3_MOD_PW0:
      value->rValue = model->B3pw0;
        break;
    case  B3_MOD_PNLX:
      value->rValue = model->B3pnlx;
        break;
    case  B3_MOD_PDVT0 :                
      value->rValue = model->B3pdvt0;
        break;
    case  B3_MOD_PDVT1 :             
      value->rValue = model->B3pdvt1;
        break;
    case  B3_MOD_PDVT2 :             
      value->rValue = model->B3pdvt2;
        break;
    case  B3_MOD_PDVT0W :                
      value->rValue = model->B3pdvt0w;
        break;
    case  B3_MOD_PDVT1W :             
      value->rValue = model->B3pdvt1w;
        break;
    case  B3_MOD_PDVT2W :             
      value->rValue = model->B3pdvt2w;
        break;
    case  B3_MOD_PDROUT :           
      value->rValue = model->B3pdrout;
        break;
    case  B3_MOD_PDSUB :           
      value->rValue = model->B3pdsub;
        break;
    case B3_MOD_PVTH0:
        value->rValue = model->B3pvth0; 
        break;
    case B3_MOD_PUA:
        value->rValue = model->B3pua; 
        break;
    case B3_MOD_PUA1:
        value->rValue = model->B3pua1; 
        break;
    case B3_MOD_PUB:
        value->rValue = model->B3pub;  
        break;
    case B3_MOD_PUB1:
        value->rValue = model->B3pub1;  
        break;
    case B3_MOD_PUC:
        value->rValue = model->B3puc; 
        break;
    case B3_MOD_PUC1:
        value->rValue = model->B3puc1; 
        break;
    case B3_MOD_PU0:
        value->rValue = model->B3pu0;
        break;
    case B3_MOD_PUTE:
        value->rValue = model->B3pute;
        break;
    case B3_MOD_PVOFF:
        value->rValue = model->B3pvoff;
        break;
    case B3_MOD_PDELTA:
        value->rValue = model->B3pdelta;
        break;
    case B3_MOD_PRDSW:
        value->rValue = model->B3prdsw; 
        break;             
    case B3_MOD_PPRWB:
        value->rValue = model->B3pprwb; 
        break;             
    case B3_MOD_PPRWG:
        value->rValue = model->B3pprwg; 
        break;             
    case B3_MOD_PPRT:
        value->rValue = model->B3pprt; 
        break;              
    case B3_MOD_PETA0:
        value->rValue = model->B3peta0; 
        break;               
    case B3_MOD_PETAB:
        value->rValue = model->B3petab; 
        break;               
    case B3_MOD_PPCLM:
        value->rValue = model->B3ppclm; 
        break;               
    case B3_MOD_PPDIBL1:
        value->rValue = model->B3ppdibl1; 
        break;               
    case B3_MOD_PPDIBL2:
        value->rValue = model->B3ppdibl2; 
        break;               
    case B3_MOD_PPDIBLB:
        value->rValue = model->B3ppdiblb; 
        break;               
    case B3_MOD_PPSCBE1:
        value->rValue = model->B3ppscbe1; 
        break;               
    case B3_MOD_PPSCBE2:
        value->rValue = model->B3ppscbe2; 
        break;               
    case B3_MOD_PPVAG:
        value->rValue = model->B3ppvag; 
        break;               
    case B3_MOD_PWR:
        value->rValue = model->B3pwr;
        break;
    case B3_MOD_PDWG:
        value->rValue = model->B3pdwg;
        break;
    case B3_MOD_PDWB:
        value->rValue = model->B3pdwb;
        break;
    case B3_MOD_PB0:
        value->rValue = model->B3pb0;
        break;
    case B3_MOD_PB1:
        value->rValue = model->B3pb1;
        break;
    case B3_MOD_PALPHA0:
        value->rValue = model->B3palpha0;
        break;
    case B3_MOD_PALPHA1:
        value->rValue = model->B3palpha1;
        break;
    case B3_MOD_PBETA0:
        value->rValue = model->B3pbeta0;
        break;
    case B3_MOD_PVFB:
        value->rValue = model->B3pvfb;
        break;

    case B3_MOD_PELM:
        value->rValue = model->B3pelm;
        break;
    case B3_MOD_PCGSL:
        value->rValue = model->B3pcgsl;
        break;
    case B3_MOD_PCGDL:
        value->rValue = model->B3pcgdl;
        break;
    case B3_MOD_PCKAPPA:
        value->rValue = model->B3pckappa;
        break;
    case B3_MOD_PCF:
        value->rValue = model->B3pcf;
        break;
    case B3_MOD_PCLC:
        value->rValue = model->B3pclc;
        break;
    case B3_MOD_PCLE:
        value->rValue = model->B3pcle;
        break;
    case B3_MOD_PVFBCV:
        value->rValue = model->B3pvfbcv;
        break;
    case B3_MOD_PACDE:
        value->rValue = model->B3pacde;
        break;
    case B3_MOD_PMOIN:
        value->rValue = model->B3pmoin;
        break;
    case B3_MOD_PNOFF:
        value->rValue = model->B3pnoff;
        break;
    case B3_MOD_PVOFFCV:
        value->rValue = model->B3pvoffcv;
        break;

    case  B3_MOD_TNOM :
      value->rValue = model->B3tnom - CONSTCtoK;
        break;
    case B3_MOD_CGSO:
        value->rValue = model->B3cgso; 
        break;
    case B3_MOD_CGDO:
        value->rValue = model->B3cgdo; 
        break;
    case B3_MOD_CGBO:
        value->rValue = model->B3cgbo; 
        break;
    case B3_MOD_XPART:
        value->rValue = model->B3xpart; 
        break;
    case B3_MOD_RSH:
        value->rValue = model->B3sheetResistance; 
        break;
    case B3_MOD_JS:
        value->rValue = model->B3jctSatCurDensity; 
        break;
    case B3_MOD_JSW:
        value->rValue = model->B3jctSidewallSatCurDensity; 
        break;
    case B3_MOD_PB:
        value->rValue = model->B3bulkJctPotential; 
        break;
    case B3_MOD_MJ:
        value->rValue = model->B3bulkJctBotGradingCoeff; 
        break;
    case B3_MOD_PBSW:
        value->rValue = model->B3sidewallJctPotential; 
        break;
    case B3_MOD_MJSW:
        value->rValue = model->B3bulkJctSideGradingCoeff; 
        break;
    case B3_MOD_CJ:
        value->rValue = model->B3unitAreaJctCap; 
        break;
    case B3_MOD_CJSW:
        value->rValue = model->B3unitLengthSidewallJctCap; 
        break;
    case B3_MOD_PBSWG:
        value->rValue = model->B3GatesidewallJctPotential; 
        break;
    case B3_MOD_MJSWG:
        value->rValue = model->B3bulkJctGateSideGradingCoeff; 
        break;
    case B3_MOD_CJSWG:
        value->rValue = model->B3unitLengthGateSidewallJctCap; 
        break;
    case B3_MOD_NJ:
        value->rValue = model->B3jctEmissionCoeff; 
        break;
    case B3_MOD_XTI:
        value->rValue = model->B3jctTempExponent; 
        break;
    case B3_MOD_LINT:
        value->rValue = model->B3Lint; 
        break;
    case B3_MOD_LL:
        value->rValue = model->B3Ll;
        break;
    case B3_MOD_LLC:
        value->rValue = model->B3Llc;
        break;
    case B3_MOD_LLN:
        value->rValue = model->B3Lln;
        break;
    case B3_MOD_LW:
        value->rValue = model->B3Lw;
        break;
    case B3_MOD_LWC:
        value->rValue = model->B3Lwc;
        break;
    case B3_MOD_LWN:
        value->rValue = model->B3Lwn;
        break;
    case B3_MOD_LWL:
        value->rValue = model->B3Lwl;
        break;
    case B3_MOD_LWLC:
        value->rValue = model->B3Lwlc;
        break;
    case B3_MOD_LMIN:
        value->rValue = model->B3Lmin;
        break;
    case B3_MOD_LMAX:
        value->rValue = model->B3Lmax;
        break;
    case B3_MOD_WINT:
        value->rValue = model->B3Wint;
        break;
    case B3_MOD_WL:
        value->rValue = model->B3Wl;
        break;
    case B3_MOD_WLC:
        value->rValue = model->B3Wlc;
        break;
    case B3_MOD_WLN:
        value->rValue = model->B3Wln;
        break;
    case B3_MOD_WW:
        value->rValue = model->B3Ww;
        break;
    case B3_MOD_WWC:
        value->rValue = model->B3Wwc;
        break;
    case B3_MOD_WWN:
        value->rValue = model->B3Wwn;
        break;
    case B3_MOD_WWL:
        value->rValue = model->B3Wwl;
        break;
    case B3_MOD_WWLC:
        value->rValue = model->B3Wwlc;
        break;
    case B3_MOD_WMIN:
        value->rValue = model->B3Wmin;
        break;
    case B3_MOD_WMAX:
        value->rValue = model->B3Wmax;
        break;
    case B3_MOD_NOIA:
        value->rValue = model->B3oxideTrapDensityA;
        break;
    case B3_MOD_NOIB:
        value->rValue = model->B3oxideTrapDensityB;
        break;
    case B3_MOD_NOIC:
        value->rValue = model->B3oxideTrapDensityC;
        break;
    case B3_MOD_EM:
        value->rValue = model->B3em;
        break;
    case B3_MOD_EF:
        value->rValue = model->B3ef;
        break;
    case B3_MOD_AF:
        value->rValue = model->B3af;
        break;
    case B3_MOD_KF:
        value->rValue = model->B3kf;
        break;
// SRW
    case B3_MOD_NQSMOD:
        value->iValue = model->B3nqsMod;
        data->type = IF_INTEGER;
        break;
    case B3_MOD_XW:
        value->rValue = model->B3xw;
        break;
    case B3_MOD_XL:
        value->rValue = model->B3xl;
        break;

    default:
        return(E_BADPARM);
    }
    return (OK);
}



