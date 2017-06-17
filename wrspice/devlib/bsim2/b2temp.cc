
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
 $Id: b2temp.cc,v 1.2 2011/12/18 01:15:19 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Hong June Park, Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b2defs.h"


int
B2dev::temperature(sGENmodel *genmod, sCKT *ckt)
{
    (void)ckt;
    struct bsim2SizeDependParam *pSizeDependParamKnot, *pLastKnot;
    double  EffectiveLength;
    double EffectiveWidth;
    double CoxWoverL, Inv_L, Inv_W, tmp;
    int Size_Not_Found;

    sB2model *model = static_cast<sB2model*>(genmod);
    for ( ; model; model = model->next()) {
    
        // Default value Processing for B2 MOSFET Models
        // Some Limiting for Model Parameters
        if (model->B2bulkJctPotential < 0.1)
            model->B2bulkJctPotential = 0.1;
        if (model->B2sidewallJctPotential < 0.1)
            model->B2sidewallJctPotential = 0.1;

        model->B2Cox = 3.453e-13/(model->B2tox * 1.0e-4); // in F/cm**2
        model->B2vdd2 = 2.0 * model->B2vdd;
        model->B2vgg2 = 2.0 * model->B2vgg;
        model->B2vbb2 = 2.0 * model->B2vbb;
        model->B2Vtm = 8.625e-5 * (model->B2temp + 273.0);
        model->pSizeDependParamKnot = 0;
        pLastKnot = 0;

        sB2instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            pSizeDependParamKnot = model->pSizeDependParamKnot;
            Size_Not_Found = 1;

            while ((pSizeDependParamKnot != 0) && Size_Not_Found) {
                if ((inst->B2l == pSizeDependParamKnot->Length)
                        && (inst->B2w == pSizeDependParamKnot->Width)) {
                    Size_Not_Found = 0;
                    inst->pParam = pSizeDependParamKnot;
                }
                else {
                    pLastKnot = pSizeDependParamKnot;
                    pSizeDependParamKnot = pSizeDependParamKnot->pNext;
                }
            }

            if (Size_Not_Found) {
                inst->pParam = (struct bsim2SizeDependParam *)malloc(
                    sizeof(struct bsim2SizeDependParam));
                if (pLastKnot == 0)
                    model->pSizeDependParamKnot = inst->pParam;
                else
                    pLastKnot->pNext = inst->pParam;
                inst->pParam->pNext = 0;

                EffectiveLength = inst->B2l - model->B2deltaL * 1.0e-6;
                EffectiveWidth = inst->B2w - model->B2deltaW * 1.0e-6;

                if (EffectiveLength <= 0) {
                   DVO.textOut(OUT_FATAL,
                    "B2: mosfet %s, model %s: Effective channel length <= 0",
                    inst->GENname, model->GENmodName);
                   return (E_BADPARM);
                }
                if (EffectiveWidth <= 0) {
                   DVO.textOut(OUT_FATAL,
                    "B2: mosfet %s, model %s: Effective channel width <= 0",
                    inst->GENname, model->GENmodName);
                   return(E_BADPARM);
                }

                Inv_L = 1.0e-6 / EffectiveLength;
                Inv_W = 1.0e-6 / EffectiveWidth;
                inst->pParam->Width = inst->B2w;
                inst->pParam->Length = inst->B2l;
                inst->pParam->B2vfb = model->B2vfb0 + model->B2vfbW * Inv_W
                   + model->B2vfbL * Inv_L;
                inst->pParam->B2phi = model->B2phi0 + model->B2phiW * Inv_W
                   + model->B2phiL * Inv_L;
                inst->pParam->B2k1 = model->B2k10 + model->B2k1W * Inv_W
                   + model->B2k1L * Inv_L;
                inst->pParam->B2k2 = model->B2k20 + model->B2k2W * Inv_W
                   + model->B2k2L * Inv_L;
                inst->pParam->B2eta0 = model->B2eta00 
                   + model->B2eta0W * Inv_W
                   + model->B2eta0L * Inv_L;
                inst->pParam->B2etaB = model->B2etaB0 + model->B2etaBW 
                   * Inv_W + model->B2etaBL * Inv_L;
                inst->pParam->B2beta0 = model->B2mob00;
                inst->pParam->B2beta0B = model->B2mob0B0 
                   + model->B2mob0BW * Inv_W
                   + model->B2mob0BL * Inv_L;
                inst->pParam->B2betas0 = model->B2mobs00 
                   + model->B2mobs0W * Inv_W
                   + model->B2mobs0L * Inv_L;
                if (inst->pParam->B2betas0 < 1.01 * inst->pParam->B2beta0)
                    inst->pParam->B2betas0 = 1.01 * inst->pParam->B2beta0;
                inst->pParam->B2betasB = model->B2mobsB0 
                   + model->B2mobsBW * Inv_W
                   + model->B2mobsBL * Inv_L;
                tmp = (inst->pParam->B2betas0 - inst->pParam->B2beta0
                       - inst->pParam->B2beta0B * model->B2vbb);
                if ((-inst->pParam->B2betasB * model->B2vbb) > tmp)
                    inst->pParam->B2betasB = -tmp / model->B2vbb;
                inst->pParam->B2beta20 = model->B2mob200 
                  + model->B2mob20W * Inv_W
                  + model->B2mob20L * Inv_L;
                inst->pParam->B2beta2B = model->B2mob2B0 
                  + model->B2mob2BW * Inv_W
                  + model->B2mob2BL * Inv_L;
                inst->pParam->B2beta2G = model->B2mob2G0 
                  + model->B2mob2GW * Inv_W
                  + model->B2mob2GL * Inv_L;
                inst->pParam->B2beta30 = model->B2mob300 
                  + model->B2mob30W * Inv_W
                  + model->B2mob30L * Inv_L;
                inst->pParam->B2beta3B = model->B2mob3B0 
                  + model->B2mob3BW * Inv_W
                  + model->B2mob3BL * Inv_L;
                inst->pParam->B2beta3G = model->B2mob3G0 
                  + model->B2mob3GW * Inv_W
                  + model->B2mob3GL * Inv_L;
                inst->pParam->B2beta40 = model->B2mob400 
                  + model->B2mob40W * Inv_W
                  + model->B2mob40L * Inv_L;
                inst->pParam->B2beta4B = model->B2mob4B0 
                  + model->B2mob4BW * Inv_W
                  + model->B2mob4BL * Inv_L;
                inst->pParam->B2beta4G = model->B2mob4G0 
                  + model->B2mob4GW * Inv_W
                  + model->B2mob4GL * Inv_L;

                CoxWoverL = model->B2Cox * EffectiveWidth / EffectiveLength;

                inst->pParam->B2beta0 *= CoxWoverL;
                inst->pParam->B2beta0B *= CoxWoverL;
                inst->pParam->B2betas0 *= CoxWoverL;
                inst->pParam->B2betasB *= CoxWoverL;
                inst->pParam->B2beta30 *= CoxWoverL;
                inst->pParam->B2beta3B *= CoxWoverL;
                inst->pParam->B2beta3G *= CoxWoverL;
                inst->pParam->B2beta40 *= CoxWoverL;
                inst->pParam->B2beta4B *= CoxWoverL;
                inst->pParam->B2beta4G *= CoxWoverL;

                inst->pParam->B2ua0 = model->B2ua00 + model->B2ua0W * Inv_W
                   + model->B2ua0L * Inv_L;
                inst->pParam->B2uaB = model->B2uaB0 + model->B2uaBW * Inv_W
                   + model->B2uaBL * Inv_L;
                inst->pParam->B2ub0 = model->B2ub00 + model->B2ub0W * Inv_W
                   + model->B2ub0L * Inv_L;
                inst->pParam->B2ubB = model->B2ubB0 + model->B2ubBW * Inv_W
                   + model->B2ubBL * Inv_L;
                inst->pParam->B2u10 = model->B2u100 + model->B2u10W * Inv_W
                   + model->B2u10L * Inv_L;
                inst->pParam->B2u1B = model->B2u1B0 + model->B2u1BW * Inv_W
                   + model->B2u1BL * Inv_L;
                inst->pParam->B2u1D = model->B2u1D0 + model->B2u1DW * Inv_W
                   + model->B2u1DL * Inv_L;
                inst->pParam->B2n0 = model->B2n00 + model->B2n0W * Inv_W
                   + model->B2n0L * Inv_L;
                inst->pParam->B2nB = model->B2nB0 + model->B2nBW * Inv_W
                   + model->B2nBL * Inv_L;
                inst->pParam->B2nD = model->B2nD0 + model->B2nDW * Inv_W
                   + model->B2nDL * Inv_L;
                if (inst->pParam->B2n0 < 0.0)
                    inst->pParam->B2n0 = 0.0;

                inst->pParam->B2vof0 = model->B2vof00 
                   + model->B2vof0W * Inv_W
                   + model->B2vof0L * Inv_L;
                inst->pParam->B2vofB = model->B2vofB0 
                   + model->B2vofBW * Inv_W
                   + model->B2vofBL * Inv_L;
                inst->pParam->B2vofD = model->B2vofD0 
                   + model->B2vofDW * Inv_W
                   + model->B2vofDL * Inv_L;
                inst->pParam->B2ai0 = model->B2ai00 + model->B2ai0W * Inv_W
                   + model->B2ai0L * Inv_L;
                inst->pParam->B2aiB = model->B2aiB0 + model->B2aiBW * Inv_W
                   + model->B2aiBL * Inv_L;
                inst->pParam->B2bi0 = model->B2bi00 + model->B2bi0W * Inv_W
                   + model->B2bi0L * Inv_L;
                inst->pParam->B2biB = model->B2biB0 + model->B2biBW * Inv_W
                   + model->B2biBL * Inv_L;
                inst->pParam->B2vghigh = model->B2vghigh0 
                   + model->B2vghighW * Inv_W
                   + model->B2vghighL * Inv_L;
                inst->pParam->B2vglow = model->B2vglow0 
                   + model->B2vglowW * Inv_W
                   + model->B2vglowL * Inv_L;

                inst->pParam->CoxWL = model->B2Cox * EffectiveLength
                   * EffectiveWidth * 1.0e4;
                inst->pParam->One_Third_CoxWL = inst->pParam->CoxWL / 3.0;
                inst->pParam->Two_Third_CoxWL = 2.0 
                   * inst->pParam->One_Third_CoxWL;
                inst->pParam->B2GSoverlapCap = model->B2gateSourceOverlapCap 
                   * EffectiveWidth;
                inst->pParam->B2GDoverlapCap = model->B2gateDrainOverlapCap 
                   * EffectiveWidth;
                inst->pParam->B2GBoverlapCap = model->B2gateBulkOverlapCap 
                   * EffectiveLength;
                inst->pParam->SqrtPhi = sqrt(inst->pParam->B2phi);
                inst->pParam->Phis3 = inst->pParam->SqrtPhi
                   * inst->pParam->B2phi;
                inst->pParam->Arg = inst->pParam->B2betasB
                   - inst->pParam->B2beta0B - model->B2vdd
                   * (inst->pParam->B2beta3B - model->B2vdd
                   * inst->pParam->B2beta4B);
             }

            // process drain series resistance 
            if ((inst->B2drainConductance=model->B2sheetResistance *
                    inst->B2drainSquares) != 0.0)
                inst->B2drainConductance = 1. / inst->B2drainConductance;
                   
            // process source series resistance
            if ((inst->B2sourceConductance=model->B2sheetResistance *
                    inst->B2sourceSquares) != 0.0)
                inst->B2sourceConductance = 1. / inst->B2sourceConductance ;

            inst->pParam->B2vt0 = inst->pParam->B2vfb 
              + inst->pParam->B2phi 
              + inst->pParam->B2k1 * inst->pParam->SqrtPhi 
              - inst->pParam->B2k2 * inst->pParam->B2phi;
            inst->B2von = inst->pParam->B2vt0; // added for initialization
        }
    }
    return(OK);
}  



