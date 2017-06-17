
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Device Library                                                         *
 *                                                                        *
 *========================================================================*
 $Id: b3temp.cc,v 2.6 2011/12/18 01:15:25 stevew Exp $
 *========================================================================*/

/***********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1995 Min-Chie Jeng and Mansun Chan.
Modified by Weidong Liu (1997-1998).
* Revision 3.2 1998/6/16  18:00:00  Weidong
* BSIM3v3.2 release
**********/

#include "b3defs.h"


#define Kb 1.3806226e-23
#define KboQ 8.617087e-5  //  Kb/q  where q = 1.60219e-19
#define EPSOX 3.453133e-11
#define EPSSI 1.03594e-10
#define MAX_EXP 5.834617425e14
#define MIN_EXP 1.713908431e-15
#define EXP_THRESHOLD 34.0
#define Charge_q 1.60219e-19


int
B3dev::temperature(sGENmodel *genmod, sCKT *ckt)
{
    bsim3SizeDependParam *pSizeDependParamKnot, *pLastKnot, *pParam;
    double tmp, tmp1, tmp2, tmp3, Eg, Eg0, ni, T0, T1, T2, T3, T4, T5, Ldrn;
    double Wdrn, delTemp, Temp, TRatio, Inv_L, Inv_W, Inv_LW, Vtm0, Tnom;
    double Nvtm, SourceSatCurrent, DrainSatCurrent;
    int Size_Not_Found;

    sB3model *model = static_cast<sB3model*>(genmod);
    for ( ; model; model = model->next()) {

        Temp = ckt->CKTcurTask->TSKtemp;
        if (model->B3bulkJctPotential < 0.1)  {
            model->B3bulkJctPotential = 0.1;
            DVO.textOut(OUT_WARNING,
                "Given pb is less than 0.1. Pb is set to 0.1.");
        }
        if (model->B3sidewallJctPotential < 0.1) {
            model->B3sidewallJctPotential = 0.1;
            DVO.textOut(OUT_WARNING,
                "Given pbsw is less than 0.1. Pbsw is set to 0.1.");
        }
        if (model->B3GatesidewallJctPotential < 0.1) {
            model->B3GatesidewallJctPotential = 0.1;
            DVO.textOut(OUT_WARNING,
                "Given pbswg is less than 0.1. Pbswg is set to 0.1.");
        }
        model->pSizeDependParamKnot = NULL;
        pLastKnot = NULL;

        Tnom = model->B3tnom;
        TRatio = Temp / Tnom;

        model->B3vcrit = CONSTvt0 * log(CONSTvt0 / (CONSTroot2 * 1.0e-14));
        model->B3factor1 = sqrt(EPSSI / EPSOX * model->B3tox);

        Vtm0 = KboQ * Tnom;
        Eg0 = 1.16 - 7.02e-4 * Tnom * Tnom / (Tnom + 1108.0);
        ni = 1.45e10 * (Tnom / 300.15) * sqrt(Tnom / 300.15) 
            * exp(21.5565981 - Eg0 / (2.0 * Vtm0));

        model->B3vtm = KboQ * Temp;
        Eg = 1.16 - 7.02e-4 * Temp * Temp / (Temp + 1108.0);
        if (Temp != Tnom) {
            T0 = Eg0 / Vtm0 - Eg / model->B3vtm + model->B3jctTempExponent
                * log(Temp / Tnom);
            T1 = exp(T0 / model->B3jctEmissionCoeff);
            model->B3jctTempSatCurDensity = model->B3jctSatCurDensity
                * T1;
            model->B3jctSidewallTempSatCurDensity
                = model->B3jctSidewallSatCurDensity * T1;
        }
        else {
            model->B3jctTempSatCurDensity = model->B3jctSatCurDensity;
            model->B3jctSidewallTempSatCurDensity
                = model->B3jctSidewallSatCurDensity;
        }

        if (model->B3jctTempSatCurDensity < 0.0)
            model->B3jctTempSatCurDensity = 0.0;
        if (model->B3jctSidewallTempSatCurDensity < 0.0)
            model->B3jctSidewallTempSatCurDensity = 0.0;

        // Temperature dependence of D/B and S/B diode capacitance begins
        delTemp = ckt->CKTcurTask->TSKtemp - model->B3tnom;
        T0 = model->B3tcj * delTemp;
        if (T0 >= -1.0) {
            model->B3unitAreaJctCap *= 1.0 + T0;
        }
        else if (model->B3unitAreaJctCap > 0.0) {
            model->B3unitAreaJctCap = 0.0;
            DVO.textOut(OUT_WARNING,
"Temperature effect has caused cj to be negative. Cj is clamped to zero.");
        }
        T0 = model->B3tcjsw * delTemp;
        if (T0 >= -1.0) {
            model->B3unitLengthSidewallJctCap *= 1.0 + T0;
        }
        else if (model->B3unitLengthSidewallJctCap > 0.0) {
            model->B3unitLengthSidewallJctCap = 0.0;
            DVO.textOut(OUT_WARNING,
"Temperature effect has caused cjsw to be negative. Cjsw is clamped to zero.");
        }
        T0 = model->B3tcjswg * delTemp;
        if (T0 >= -1.0) {
            model->B3unitLengthGateSidewallJctCap *= 1.0 + T0;
        }
        else if (model->B3unitLengthGateSidewallJctCap > 0.0) {
            model->B3unitLengthGateSidewallJctCap = 0.0;
            DVO.textOut(OUT_WARNING,
"Temperature effect has caused cjswg to be negative. Cjswg is clamped to zero.");
        }
        model->B3PhiB = model->B3bulkJctPotential
            - model->B3tpb * delTemp;
        if (model->B3PhiB < 0.01) {
            model->B3PhiB = 0.01;
            DVO.textOut(OUT_WARNING,
"Temperature effect has caused pb to be less than 0.01. Pb is clamped to 0.01.");
        }
        model->B3PhiBSW = model->B3sidewallJctPotential
            - model->B3tpbsw * delTemp;
        if (model->B3PhiBSW <= 0.01) {
            model->B3PhiBSW = 0.01;
            DVO.textOut(OUT_WARNING,
"Temperature effect has caused pbsw to be less than 0.01. Pbsw is clamped to 0.01.");
        }
        model->B3PhiBSWG = model->B3GatesidewallJctPotential
            - model->B3tpbswg * delTemp;
        if (model->B3PhiBSWG <= 0.01) {
            model->B3PhiBSWG = 0.01;
            DVO.textOut(OUT_WARNING,
"Temperature effect has caused pbswg to be less than 0.01. Pbswg is clamped to 0.01.");
        }
        // End of junction capacitance - Weidong & Min-Chie 5/1998

        // MCJ: Length and Width not initialized
        sB3instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            pSizeDependParamKnot = model->pSizeDependParamKnot;
            Size_Not_Found = 1;
            while ((pSizeDependParamKnot != NULL) && Size_Not_Found) {
                if ((inst->B3l == pSizeDependParamKnot->Length)
                        && (inst->B3w == pSizeDependParamKnot->Width)) {
                    Size_Not_Found = 0;
                    inst->pParam = pSizeDependParamKnot;
                }
                else {
                    pLastKnot = pSizeDependParamKnot;
                    pSizeDependParamKnot = pSizeDependParamKnot->pNext;
                }
            }

            if (Size_Not_Found) {
                pParam = new bsim3SizeDependParam;
                if (pLastKnot == NULL)
                    model->pSizeDependParamKnot = pParam;
                else
                    pLastKnot->pNext = pParam;
                pParam->pNext = NULL;
                inst->pParam = pParam;

// SRW
//                Ldrn = inst->B3l;
//                Wdrn = inst->B3w;
                Ldrn = inst->B3l - 0.5*model->B3xl;
                Wdrn = inst->B3w - 0.5*model->B3xw;
                pParam->Length = Ldrn;
                pParam->Width = Wdrn;
          
                T0 = pow(Ldrn, model->B3Lln);
                T1 = pow(Wdrn, model->B3Lwn);
                tmp1 = model->B3Ll / T0 + model->B3Lw / T1
                    + model->B3Lwl / (T0 * T1);
                pParam->B3dl = model->B3Lint + tmp1;
                tmp2 = model->B3Llc / T0 + model->B3Lwc / T1
                    + model->B3Lwlc / (T0 * T1);
                pParam->B3dlc = model->B3dlc + tmp2;

                T2 = pow(Ldrn, model->B3Wln);
                T3 = pow(Wdrn, model->B3Wwn);
                tmp1 = model->B3Wl / T2 + model->B3Ww / T3
                    + model->B3Wwl / (T2 * T3);
                pParam->B3dw = model->B3Wint + tmp1;
                tmp2 = model->B3Wlc / T2 + model->B3Wwc / T3
                    + model->B3Wwlc / (T2 * T3);
                pParam->B3dwc = model->B3dwc + tmp2;

                pParam->B3leff = inst->B3l - 2.0 * pParam->B3dl;
                if (pParam->B3leff <= 0.0) {
                    IFuid namarray[2];
                    namarray[0] = model->GENmodName;
                    namarray[1] = inst->GENname;
                    DVO.textOut(OUT_FATAL,
                    "B3: mosfet %s, model %s: Effective channel length <= 0",
                        namarray[0], namarray[1]);
                    return(E_BADPARM);
                }

                pParam->B3weff = inst->B3w - 2.0 * pParam->B3dw;
                if (pParam->B3weff <= 0.0) {
                    IFuid namarray[2];
                    namarray[0] = model->GENmodName;
                    namarray[1] = inst->GENname;
                    DVO.textOut(OUT_FATAL,
                    "B3: mosfet %s, model %s: Effective channel width <= 0",
                        namarray[0], namarray[1]);
                    return(E_BADPARM);
                }

                pParam->B3leffCV = inst->B3l - 2.0 * pParam->B3dlc;
                if (pParam->B3leffCV <= 0.0) {
                    IFuid namarray[2];
                    namarray[0] = model->GENmodName;
                    namarray[1] = inst->GENname;
                    DVO.textOut(OUT_FATAL,
                    "B3: mosfet %s, model %s: Effective channel length for C-V <= 0",
                        namarray[0], namarray[1]);
                    return(E_BADPARM);
                }

                pParam->B3weffCV = inst->B3w - 2.0 * pParam->B3dwc;
                if (pParam->B3weffCV <= 0.0) {
                    IFuid namarray[2];
                    namarray[0] = model->GENmodName;
                    namarray[1] = inst->GENname;
                    DVO.textOut(OUT_FATAL,
                    "B3: mosfet %s, model %s: Effective channel width for C-V <= 0",
                        namarray[0], namarray[1]);
                    return(E_BADPARM);
                }

                if (model->B3binUnit == 1) {
                    Inv_L = 1.0e-6 / pParam->B3leff;
                    Inv_W = 1.0e-6 / pParam->B3weff;
                    Inv_LW = 1.0e-12 / (pParam->B3leff
                        * pParam->B3weff);
                }
                else {
                    Inv_L = 1.0 / pParam->B3leff;
                    Inv_W = 1.0 / pParam->B3weff;
                    Inv_LW = 1.0 / (pParam->B3leff
                        * pParam->B3weff);
                }
                pParam->B3cdsc = model->B3cdsc
                    + model->B3lcdsc * Inv_L
                    + model->B3wcdsc * Inv_W
                    + model->B3pcdsc * Inv_LW;
                pParam->B3cdscb = model->B3cdscb
                    + model->B3lcdscb * Inv_L
                    + model->B3wcdscb * Inv_W
                    + model->B3pcdscb * Inv_LW; 
                     
                pParam->B3cdscd = model->B3cdscd
                    + model->B3lcdscd * Inv_L
                    + model->B3wcdscd * Inv_W
                    + model->B3pcdscd * Inv_LW; 
                     
                pParam->B3cit = model->B3cit
                    + model->B3lcit * Inv_L
                    + model->B3wcit * Inv_W
                    + model->B3pcit * Inv_LW;
                pParam->B3nfactor = model->B3nfactor
                    + model->B3lnfactor * Inv_L
                    + model->B3wnfactor * Inv_W
                    + model->B3pnfactor * Inv_LW;
                pParam->B3xj = model->B3xj
                    + model->B3lxj * Inv_L
                    + model->B3wxj * Inv_W
                    + model->B3pxj * Inv_LW;
                pParam->B3vsat = model->B3vsat
                    + model->B3lvsat * Inv_L
                    + model->B3wvsat * Inv_W
                    + model->B3pvsat * Inv_LW;
                pParam->B3at = model->B3at
                    + model->B3lat * Inv_L
                    + model->B3wat * Inv_W
                    + model->B3pat * Inv_LW;
                pParam->B3a0 = model->B3a0
                    + model->B3la0 * Inv_L
                    + model->B3wa0 * Inv_W
                    + model->B3pa0 * Inv_LW; 
                  
                pParam->B3ags = model->B3ags
                    + model->B3lags * Inv_L
                    + model->B3wags * Inv_W
                    + model->B3pags * Inv_LW;
                  
                pParam->B3a1 = model->B3a1
                    + model->B3la1 * Inv_L
                    + model->B3wa1 * Inv_W
                    + model->B3pa1 * Inv_LW;
                pParam->B3a2 = model->B3a2
                    + model->B3la2 * Inv_L
                    + model->B3wa2 * Inv_W
                    + model->B3pa2 * Inv_LW;
                pParam->B3keta = model->B3keta
                    + model->B3lketa * Inv_L
                    + model->B3wketa * Inv_W
                    + model->B3pketa * Inv_LW;
                pParam->B3nsub = model->B3nsub
                    + model->B3lnsub * Inv_L
                    + model->B3wnsub * Inv_W
                    + model->B3pnsub * Inv_LW;
                pParam->B3npeak = model->B3npeak
                    + model->B3lnpeak * Inv_L
                    + model->B3wnpeak * Inv_W
                    + model->B3pnpeak * Inv_LW;
                pParam->B3ngate = model->B3ngate
                    + model->B3lngate * Inv_L
                    + model->B3wngate * Inv_W
                    + model->B3pngate * Inv_LW;
                pParam->B3gamma1 = model->B3gamma1
                    + model->B3lgamma1 * Inv_L
                    + model->B3wgamma1 * Inv_W
                    + model->B3pgamma1 * Inv_LW;
                pParam->B3gamma2 = model->B3gamma2
                    + model->B3lgamma2 * Inv_L
                    + model->B3wgamma2 * Inv_W
                    + model->B3pgamma2 * Inv_LW;
                pParam->B3vbx = model->B3vbx
                    + model->B3lvbx * Inv_L
                    + model->B3wvbx * Inv_W
                    + model->B3pvbx * Inv_LW;
                pParam->B3vbm = model->B3vbm
                    + model->B3lvbm * Inv_L
                    + model->B3wvbm * Inv_W
                    + model->B3pvbm * Inv_LW;
                pParam->B3xt = model->B3xt
                    + model->B3lxt * Inv_L
                    + model->B3wxt * Inv_W
                    + model->B3pxt * Inv_LW;
                pParam->B3vfb = model->B3vfb
                    + model->B3lvfb * Inv_L
                    + model->B3wvfb * Inv_W
                    + model->B3pvfb * Inv_LW;
                pParam->B3k1 = model->B3k1
                    + model->B3lk1 * Inv_L
                    + model->B3wk1 * Inv_W
                    + model->B3pk1 * Inv_LW;
                pParam->B3kt1 = model->B3kt1
                    + model->B3lkt1 * Inv_L
                    + model->B3wkt1 * Inv_W
                    + model->B3pkt1 * Inv_LW;
                pParam->B3kt1l = model->B3kt1l
                    + model->B3lkt1l * Inv_L
                    + model->B3wkt1l * Inv_W
                    + model->B3pkt1l * Inv_LW;
                pParam->B3k2 = model->B3k2
                    + model->B3lk2 * Inv_L
                    + model->B3wk2 * Inv_W
                    + model->B3pk2 * Inv_LW;
                pParam->B3kt2 = model->B3kt2
                    + model->B3lkt2 * Inv_L
                    + model->B3wkt2 * Inv_W
                    + model->B3pkt2 * Inv_LW;
                pParam->B3k3 = model->B3k3
                    + model->B3lk3 * Inv_L
                    + model->B3wk3 * Inv_W
                    + model->B3pk3 * Inv_LW;
                pParam->B3k3b = model->B3k3b
                    + model->B3lk3b * Inv_L
                    + model->B3wk3b * Inv_W
                    + model->B3pk3b * Inv_LW;
                pParam->B3w0 = model->B3w0
                    + model->B3lw0 * Inv_L
                    + model->B3ww0 * Inv_W
                    + model->B3pw0 * Inv_LW;
                pParam->B3nlx = model->B3nlx
                    + model->B3lnlx * Inv_L
                    + model->B3wnlx * Inv_W
                    + model->B3pnlx * Inv_LW;
                pParam->B3dvt0 = model->B3dvt0
                    + model->B3ldvt0 * Inv_L
                    + model->B3wdvt0 * Inv_W
                    + model->B3pdvt0 * Inv_LW;
                pParam->B3dvt1 = model->B3dvt1
                    + model->B3ldvt1 * Inv_L
                    + model->B3wdvt1 * Inv_W
                    + model->B3pdvt1 * Inv_LW;
                pParam->B3dvt2 = model->B3dvt2
                    + model->B3ldvt2 * Inv_L
                    + model->B3wdvt2 * Inv_W
                    + model->B3pdvt2 * Inv_LW;
                pParam->B3dvt0w = model->B3dvt0w
                    + model->B3ldvt0w * Inv_L
                    + model->B3wdvt0w * Inv_W
                    + model->B3pdvt0w * Inv_LW;
                pParam->B3dvt1w = model->B3dvt1w
                    + model->B3ldvt1w * Inv_L
                    + model->B3wdvt1w * Inv_W
                    + model->B3pdvt1w * Inv_LW;
                pParam->B3dvt2w = model->B3dvt2w
                    + model->B3ldvt2w * Inv_L
                    + model->B3wdvt2w * Inv_W
                    + model->B3pdvt2w * Inv_LW;
                pParam->B3drout = model->B3drout
                    + model->B3ldrout * Inv_L
                    + model->B3wdrout * Inv_W
                    + model->B3pdrout * Inv_LW;
                pParam->B3dsub = model->B3dsub
                    + model->B3ldsub * Inv_L
                    + model->B3wdsub * Inv_W
                    + model->B3pdsub * Inv_LW;
                pParam->B3vth0 = model->B3vth0
                    + model->B3lvth0 * Inv_L
                    + model->B3wvth0 * Inv_W
                    + model->B3pvth0 * Inv_LW;
                pParam->B3ua = model->B3ua
                    + model->B3lua * Inv_L
                    + model->B3wua * Inv_W
                    + model->B3pua * Inv_LW;
                pParam->B3ua1 = model->B3ua1
                    + model->B3lua1 * Inv_L
                    + model->B3wua1 * Inv_W
                    + model->B3pua1 * Inv_LW;
                pParam->B3ub = model->B3ub
                    + model->B3lub * Inv_L
                    + model->B3wub * Inv_W
                    + model->B3pub * Inv_LW;
                pParam->B3ub1 = model->B3ub1
                    + model->B3lub1 * Inv_L
                    + model->B3wub1 * Inv_W
                    + model->B3pub1 * Inv_LW;
                pParam->B3uc = model->B3uc
                    + model->B3luc * Inv_L
                    + model->B3wuc * Inv_W
                    + model->B3puc * Inv_LW;
                pParam->B3uc1 = model->B3uc1
                    + model->B3luc1 * Inv_L
                    + model->B3wuc1 * Inv_W
                    + model->B3puc1 * Inv_LW;
                pParam->B3u0 = model->B3u0
                    + model->B3lu0 * Inv_L
                    + model->B3wu0 * Inv_W
                    + model->B3pu0 * Inv_LW;
                pParam->B3ute = model->B3ute
                    + model->B3lute * Inv_L
                    + model->B3wute * Inv_W
                    + model->B3pute * Inv_LW;
                pParam->B3voff = model->B3voff
                    + model->B3lvoff * Inv_L
                    + model->B3wvoff * Inv_W
                    + model->B3pvoff * Inv_LW;
                pParam->B3delta = model->B3delta
                    + model->B3ldelta * Inv_L
                    + model->B3wdelta * Inv_W
                    + model->B3pdelta * Inv_LW;
                pParam->B3rdsw = model->B3rdsw
                    + model->B3lrdsw * Inv_L
                    + model->B3wrdsw * Inv_W
                    + model->B3prdsw * Inv_LW;
                pParam->B3prwg = model->B3prwg
                    + model->B3lprwg * Inv_L
                    + model->B3wprwg * Inv_W
                    + model->B3pprwg * Inv_LW;
                pParam->B3prwb = model->B3prwb
                    + model->B3lprwb * Inv_L
                    + model->B3wprwb * Inv_W
                    + model->B3pprwb * Inv_LW;
                pParam->B3prt = model->B3prt
                    + model->B3lprt * Inv_L
                    + model->B3wprt * Inv_W
                    + model->B3pprt * Inv_LW;
                pParam->B3eta0 = model->B3eta0
                    + model->B3leta0 * Inv_L
                    + model->B3weta0 * Inv_W
                    + model->B3peta0 * Inv_LW;
                pParam->B3etab = model->B3etab
                    + model->B3letab * Inv_L
                    + model->B3wetab * Inv_W
                    + model->B3petab * Inv_LW;
                pParam->B3pclm = model->B3pclm
                    + model->B3lpclm * Inv_L
                    + model->B3wpclm * Inv_W
                    + model->B3ppclm * Inv_LW;
                pParam->B3pdibl1 = model->B3pdibl1
                    + model->B3lpdibl1 * Inv_L
                    + model->B3wpdibl1 * Inv_W
                    + model->B3ppdibl1 * Inv_LW;
                pParam->B3pdibl2 = model->B3pdibl2
                    + model->B3lpdibl2 * Inv_L
                    + model->B3wpdibl2 * Inv_W
                    + model->B3ppdibl2 * Inv_LW;
                pParam->B3pdiblb = model->B3pdiblb
                    + model->B3lpdiblb * Inv_L
                    + model->B3wpdiblb * Inv_W
                    + model->B3ppdiblb * Inv_LW;
                pParam->B3pscbe1 = model->B3pscbe1
                    + model->B3lpscbe1 * Inv_L
                    + model->B3wpscbe1 * Inv_W
                    + model->B3ppscbe1 * Inv_LW;
                pParam->B3pscbe2 = model->B3pscbe2
                    + model->B3lpscbe2 * Inv_L
                    + model->B3wpscbe2 * Inv_W
                    + model->B3ppscbe2 * Inv_LW;
                pParam->B3pvag = model->B3pvag
                    + model->B3lpvag * Inv_L
                    + model->B3wpvag * Inv_W
                    + model->B3ppvag * Inv_LW;
                pParam->B3wr = model->B3wr
                    + model->B3lwr * Inv_L
                    + model->B3wwr * Inv_W
                    + model->B3pwr * Inv_LW;
                pParam->B3dwg = model->B3dwg
                    + model->B3ldwg * Inv_L
                    + model->B3wdwg * Inv_W
                    + model->B3pdwg * Inv_LW;
                pParam->B3dwb = model->B3dwb
                    + model->B3ldwb * Inv_L
                    + model->B3wdwb * Inv_W
                    + model->B3pdwb * Inv_LW;
                pParam->B3b0 = model->B3b0
                    + model->B3lb0 * Inv_L
                    + model->B3wb0 * Inv_W
                    + model->B3pb0 * Inv_LW;
                pParam->B3b1 = model->B3b1
                    + model->B3lb1 * Inv_L
                    + model->B3wb1 * Inv_W
                    + model->B3pb1 * Inv_LW;
                pParam->B3alpha0 = model->B3alpha0
                    + model->B3lalpha0 * Inv_L
                    + model->B3walpha0 * Inv_W
                    + model->B3palpha0 * Inv_LW;
                pParam->B3alpha1 = model->B3alpha1
                    + model->B3lalpha1 * Inv_L
                    + model->B3walpha1 * Inv_W
                    + model->B3palpha1 * Inv_LW;
                pParam->B3beta0 = model->B3beta0
                    + model->B3lbeta0 * Inv_L
                    + model->B3wbeta0 * Inv_W
                    + model->B3pbeta0 * Inv_LW;
                // CV model
                pParam->B3elm = model->B3elm
                    + model->B3lelm * Inv_L
                    + model->B3welm * Inv_W
                    + model->B3pelm * Inv_LW;
                pParam->B3cgsl = model->B3cgsl
                    + model->B3lcgsl * Inv_L
                    + model->B3wcgsl * Inv_W
                    + model->B3pcgsl * Inv_LW;
                pParam->B3cgdl = model->B3cgdl
                    + model->B3lcgdl * Inv_L
                    + model->B3wcgdl * Inv_W
                    + model->B3pcgdl * Inv_LW;
                pParam->B3ckappa = model->B3ckappa
                    + model->B3lckappa * Inv_L
                    + model->B3wckappa * Inv_W
                    + model->B3pckappa * Inv_LW;
                pParam->B3cf = model->B3cf
                    + model->B3lcf * Inv_L
                    + model->B3wcf * Inv_W
                    + model->B3pcf * Inv_LW;
                pParam->B3clc = model->B3clc
                    + model->B3lclc * Inv_L
                    + model->B3wclc * Inv_W
                    + model->B3pclc * Inv_LW;
                pParam->B3cle = model->B3cle
                    + model->B3lcle * Inv_L
                    + model->B3wcle * Inv_W
                    + model->B3pcle * Inv_LW;
                pParam->B3vfbcv = model->B3vfbcv
                    + model->B3lvfbcv * Inv_L
                    + model->B3wvfbcv * Inv_W
                    + model->B3pvfbcv * Inv_LW;
                pParam->B3acde = model->B3acde
                    + model->B3lacde * Inv_L
                    + model->B3wacde * Inv_W
                    + model->B3pacde * Inv_LW;
                pParam->B3moin = model->B3moin
                    + model->B3lmoin * Inv_L
                    + model->B3wmoin * Inv_W
                    + model->B3pmoin * Inv_LW;
                pParam->B3noff = model->B3noff
                    + model->B3lnoff * Inv_L
                    + model->B3wnoff * Inv_W
                    + model->B3pnoff * Inv_LW;
                pParam->B3voffcv = model->B3voffcv
                    + model->B3lvoffcv * Inv_L
                    + model->B3wvoffcv * Inv_W
                    + model->B3pvoffcv * Inv_LW;

                pParam->B3abulkCVfactor = 1.0 + pow((pParam->B3clc
                    / pParam->B3leffCV), pParam->B3cle);

                T0 = (TRatio - 1.0);
                pParam->B3ua = pParam->B3ua + pParam->B3ua1 * T0;
                pParam->B3ub = pParam->B3ub + pParam->B3ub1 * T0;
                pParam->B3uc = pParam->B3uc + pParam->B3uc1 * T0;
                if (pParam->B3u0 > 1.0) 
                    pParam->B3u0 = pParam->B3u0 / 1.0e4;

                pParam->B3u0temp = pParam->B3u0
                    * pow(TRatio, pParam->B3ute); 
                pParam->B3vsattemp = pParam->B3vsat - pParam->B3at 
                    * T0;
                pParam->B3rds0 = (pParam->B3rdsw + pParam->B3prt * T0)
                    / pow(pParam->B3weff * 1E6, pParam->B3wr);

                if (checkModel(model, inst, ckt)) {
                    IFuid namarray[2];
                    namarray[0] = model->GENmodName;
                    namarray[1] = inst->GENname;
                    DVO.textOut(OUT_FATAL,
"Fatal error(s) detected during B3V3.2 parameter checking for %s in model %s",
                        namarray[0], namarray[1]);
                    return(E_BADPARM);   
                }

                pParam->B3cgdo = (model->B3cgdo + pParam->B3cf)
                    * pParam->B3weffCV;
                pParam->B3cgso = (model->B3cgso + pParam->B3cf)
                    * pParam->B3weffCV;
                pParam->B3cgbo = model->B3cgbo * pParam->B3leffCV;

                T0 = pParam->B3leffCV * pParam->B3leffCV;
                pParam->B3tconst = pParam->B3u0temp * pParam->B3elm /
                    (model->B3cox * pParam->B3weffCV * pParam->B3leffCV * T0);

                if (!model->B3npeakGiven && model->B3gamma1Given) {
                    T0 = pParam->B3gamma1 * model->B3cox;
                    pParam->B3npeak = 3.021E22 * T0 * T0;
                }

                pParam->B3phi = 2.0 * Vtm0 
                    * log(pParam->B3npeak / ni);

                pParam->B3sqrtPhi = sqrt(pParam->B3phi);
                pParam->B3phis3 = pParam->B3sqrtPhi * pParam->B3phi;

                pParam->B3Xdep0 = sqrt(2.0 * EPSSI / (Charge_q
                    * pParam->B3npeak * 1.0e6))
                    * pParam->B3sqrtPhi; 
                pParam->B3sqrtXdep0 = sqrt(pParam->B3Xdep0);
                pParam->B3litl = sqrt(3.0 * pParam->B3xj
                    * model->B3tox);
                pParam->B3vbi = Vtm0 * log(1.0e20
                    * pParam->B3npeak / (ni * ni));
                pParam->B3cdep0 = sqrt(Charge_q * EPSSI
                    * pParam->B3npeak * 1.0e6 / 2.0
                    / pParam->B3phi);

                pParam->B3ldeb = sqrt(EPSSI * Vtm0 / (Charge_q
                    * pParam->B3npeak * 1.0e6)) / 3.0;
                pParam->B3acde *= pow((pParam->B3npeak / 2.0e16), -0.25);


                if (model->B3k1Given || model->B3k2Given) {
                    if (!model->B3k1Given) {
                        DVO.textOut(OUT_WARNING,
                            "Warning: k1 should be specified with k2.");
                        pParam->B3k1 = 0.53;
                    }
                    if (!model->B3k2Given) {
                        DVO.textOut(OUT_WARNING,
                            "Warning: k2 should be specified with k1.");
                        pParam->B3k2 = -0.0186;
                    }
                    if (model->B3nsubGiven)
                        DVO.textOut(OUT_WARNING,
                        "Warning: nsub is ignored because k1 or k2 is given.");
                    if (model->B3xtGiven)
                        DVO.textOut(OUT_WARNING,
                        "Warning: xt is ignored because k1 or k2 is given.");
                    if (model->B3vbxGiven)
                        DVO.textOut(OUT_WARNING,
                        "Warning: vbx is ignored because k1 or k2 is given.");
                    if (model->B3gamma1Given)
                        DVO.textOut(OUT_WARNING,
                    "Warning: gamma1 is ignored because k1 or k2 is given.");
                    if (model->B3gamma2Given)
                        DVO.textOut(OUT_WARNING,
                    "Warning: gamma2 is ignored because k1 or k2 is given.");
                }
                else {
                    if (!model->B3vbxGiven)
                        pParam->B3vbx = pParam->B3phi - 7.7348e-4 
                            * pParam->B3npeak
                            * pParam->B3xt * pParam->B3xt;
                    if (pParam->B3vbx > 0.0)
                        pParam->B3vbx = -pParam->B3vbx;
                    if (pParam->B3vbm > 0.0)
                        pParam->B3vbm = -pParam->B3vbm;
           
                    if (!model->B3gamma1Given)
                        pParam->B3gamma1 = 5.753e-12
                            * sqrt(pParam->B3npeak)
                            / model->B3cox;
                    if (!model->B3gamma2Given)
                        pParam->B3gamma2 = 5.753e-12
                            * sqrt(pParam->B3nsub)
                            / model->B3cox;

                    T0 = pParam->B3gamma1 - pParam->B3gamma2;
                    T1 = sqrt(pParam->B3phi - pParam->B3vbx)
                        - pParam->B3sqrtPhi;
                    T2 = sqrt(pParam->B3phi * (pParam->B3phi
                        - pParam->B3vbm)) - pParam->B3phi;
                    pParam->B3k2 = T0 * T1 / (2.0 * T2 + pParam->B3vbm);
                    pParam->B3k1 = pParam->B3gamma2 - 2.0
                        * pParam->B3k2 * sqrt(pParam->B3phi
                        - pParam->B3vbm);
                }
 
                if (pParam->B3k2 < 0.0) {
                    T0 = 0.5 * pParam->B3k1 / pParam->B3k2;
                    pParam->B3vbsc = 0.9 * (pParam->B3phi - T0 * T0);
                    if (pParam->B3vbsc > -3.0)
                        pParam->B3vbsc = -3.0;
                    else if (pParam->B3vbsc < -30.0)
                        pParam->B3vbsc = -30.0;
                }
                else {
                    pParam->B3vbsc = -30.0;
                }
                if (pParam->B3vbsc > pParam->B3vbm)
                    pParam->B3vbsc = pParam->B3vbm;

                if (!model->B3vfbGiven) {
                    if (model->B3vth0Given) {
                        pParam->B3vfb = model->B3type * pParam->B3vth0
                            - pParam->B3phi - pParam->B3k1
                            * pParam->B3sqrtPhi;
                    }
                    else {
                        pParam->B3vfb = -1.0;
                    }
                }
                if (!model->B3vth0Given) {
                    pParam->B3vth0 = model->B3type * (pParam->B3vfb
                        + pParam->B3phi + pParam->B3k1
                        * pParam->B3sqrtPhi);
                }

                pParam->B3k1ox = pParam->B3k1 * model->B3tox
                    / model->B3toxm;
                pParam->B3k2ox = pParam->B3k2 * model->B3tox
                    / model->B3toxm;

                T1 = sqrt(EPSSI / EPSOX * model->B3tox
                    * pParam->B3Xdep0);
                T0 = exp(-0.5 * pParam->B3dsub * pParam->B3leff / T1);
                pParam->B3theta0vb0 = (T0 + 2.0 * T0 * T0);

                T0 = exp(-0.5 * pParam->B3drout * pParam->B3leff / T1);
                T2 = (T0 + 2.0 * T0 * T0);
                pParam->B3thetaRout = pParam->B3pdibl1 * T2
                    + pParam->B3pdibl2;

                // vfbzb for capMod 1, 2 & 3 - Weidong 4/1997
                tmp = sqrt(pParam->B3Xdep0);
                tmp1 = pParam->B3vbi - pParam->B3phi;
                tmp2 = model->B3factor1 * tmp;

                T0 = -0.5 * pParam->B3dvt1w * pParam->B3weff
                    * pParam->B3leff / tmp2;
                if (T0 > -EXP_THRESHOLD) {
                    T1 = exp(T0);
                    T2 = T1 * (1.0 + 2.0 * T1);
                }
                else {
                    T1 = MIN_EXP;
                    T2 = T1 * (1.0 + 2.0 * T1);
                }
                T0 = pParam->B3dvt0w * T2;
                T2 = T0 * tmp1;

                T0 = -0.5 * pParam->B3dvt1 * pParam->B3leff / tmp2;
                if (T0 > -EXP_THRESHOLD) {
                    T1 = exp(T0);
                    T3 = T1 * (1.0 + 2.0 * T1);
                }
                else {
                    T1 = MIN_EXP;
                    T3 = T1 * (1.0 + 2.0 * T1);
                }
                T3 = pParam->B3dvt0 * T3 * tmp1;

                T4 = model->B3tox * pParam->B3phi
                    / (pParam->B3weff + pParam->B3w0);

                T0 = sqrt(1.0 + pParam->B3nlx / pParam->B3leff);
                T5 = pParam->B3k1ox * (T0 - 1.0) * pParam->B3sqrtPhi
                    + (pParam->B3kt1 + pParam->B3kt1l / pParam->B3leff)
                    * (TRatio - 1.0);

                tmp3 = model->B3type * pParam->B3vth0
                    - T2 - T3 + pParam->B3k3 * T4 + T5;
                pParam->B3vfbzb = tmp3 - pParam->B3phi - pParam->B3k1
                    * pParam->B3sqrtPhi;
                // End of vfbzb
            }

            // process source/drain series resistance
            inst->B3drainConductance = model->B3sheetResistance 
                * inst->B3drainSquares;
            if (inst->B3drainConductance > 0.0)
                inst->B3drainConductance = 1.0
                    / inst->B3drainConductance;
            else
                inst->B3drainConductance = 0.0;
                  
            inst->B3sourceConductance = model->B3sheetResistance 
                * inst->B3sourceSquares;
            if (inst->B3sourceConductance > 0.0) 
                inst->B3sourceConductance = 1.0
                    / inst->B3sourceConductance;
            else
                inst->B3sourceConductance = 0.0;

            // SRW - this looks wrong
            /*
            inst->B3cgso = pParam->B3cgso;
            inst->B3cgdo = pParam->B3cgdo;
            */
            inst->B3cgso = inst->pParam->B3cgso;
            inst->B3cgdo = inst->pParam->B3cgdo;

            Nvtm = model->B3vtm * model->B3jctEmissionCoeff;
            if ((inst->B3sourceArea <= 0.0) &&
                    (inst->B3sourcePerimeter <= 0.0)) {
                SourceSatCurrent = 1.0e-14;
            }
            else {
                SourceSatCurrent = inst->B3sourceArea
                    * model->B3jctTempSatCurDensity
                    + inst->B3sourcePerimeter
                    * model->B3jctSidewallTempSatCurDensity;
            }
            if ((SourceSatCurrent > 0.0) && (model->B3ijth > 0.0)) {
                inst->B3vjsm = Nvtm * log(model->B3ijth
                    / SourceSatCurrent + 1.0);
            }

            if ((inst->B3drainArea <= 0.0) &&
                    (inst->B3drainPerimeter <= 0.0)) {
                DrainSatCurrent = 1.0e-14;
            }
            else {
                DrainSatCurrent = inst->B3drainArea
                    * model->B3jctTempSatCurDensity
                    + inst->B3drainPerimeter
                    * model->B3jctSidewallTempSatCurDensity;
            }
            if ((DrainSatCurrent > 0.0) && (model->B3ijth > 0.0)) {
                inst->B3vjdm = Nvtm * log(model->B3ijth
                    / DrainSatCurrent + 1.0);
            }
        }
    }
    return(OK);
}

