
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
 $Id: mostemp.cc,v 1.3 2011/12/18 01:16:06 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1989 Takayasu Sakurai
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mosdefs.h"


// assuming silicon - make definition for epsilon of silicon
#define EPSSIL (11.7 * 8.854214871e-12)

// (cm**3/m**3)
#define CM3PM3 1e6

// (cm**2/m**2)
#define CM2PM2 1e4

// (cm**2/m**2)
#define M2PCM2 1e-4

static int mos_dopsetup(sMOSmodel*, double, double);


int
MOSdev::temperature(sGENmodel *genmod, sCKT *ckt)
{
    sMOSmodel *model = static_cast<sMOSmodel*>(genmod);
    for ( ; model; model = model->next()) {
        
        // perform model defaulting

        if (!model->MOStnomGiven)
            model->MOStnom = ckt->CKTcurTask->TSKnomTemp;
        double fact1  = model->MOStnom/REFTEMP;
        double vtnom  = model->MOStnom*CONSTKoverQ;
        double egfet1 = 1.16 - (7.02e-4*model->MOStnom*model->MOStnom)/
                (model->MOStnom + 1108);
        double tmp    = model->MOStnom*CONSTboltz;
        tmp = -egfet1/(tmp+tmp) + 1.1150877/(CONSTboltz*(REFTEMP+REFTEMP));
        double pbfact1 = -2*vtnom*(1.5*log(fact1) + CHARGE*tmp);

        if (!model->MOSoxideThicknessGiven ||
                model->MOSoxideThickness == 0) {

            if (model->MOSlevel == 1 || model->MOSlevel == 6) {
                model->MOSoxideCapFactor = 0;
                goto l1skip;
            }
            model->MOSoxideThickness = 1e-7;
        }
        model->MOSoxideCapFactor = 3.9 * 8.854214871e-12/
                model->MOSoxideThickness;

        if (!model->MOSsurfaceMobilityGiven)
            model->MOSsurfaceMobility = 600;

        if (model->MOSlevel == 6) {
            if (!model->MOSkcGiven) {
                model->MOSkc = .5*model->MOSsurfaceMobility * 
                    model->MOSoxideCapFactor * M2PCM2;
            }
        }
        else {
            if (!model->MOStransconductanceGiven) {
                model->MOStransconductance = model->MOSsurfaceMobility * 
                    model->MOSoxideCapFactor * M2PCM2;
            }
        }

        if (model->MOSsubstrateDopingGiven) {

            int error = mos_dopsetup(model,vtnom,egfet1);
            if (error)
                return (error);

            if (model->MOSlevel == 2) {
                model->MOSxd = sqrt((EPSSIL+EPSSIL)/
                    (CHARGE*model->MOSsubstrateDoping*CM3PM3));
            }
            else if (model->MOSlevel == 3) {
                model->MOSalpha = (EPSSIL+EPSSIL)/
                    (CHARGE*model->MOSsubstrateDoping*CM3PM3);
                model->MOSxd = sqrt(model->MOSalpha);
            }
        }
l1skip:

        if (model->MOSlevel == 2) {
            if (!model->MOSbulkCapFactorGiven) {
                model->MOSbulkCapFactor = sqrt(EPSSIL*CHARGE*
                    model->MOSsubstrateDoping*CM3PM3
                    /(2*model->MOSbulkJctPotential));
            }
        }
        else if (model->MOSlevel == 3) {
            model->MOSnarrowFactor =
                model->MOSdelta * 0.5 * M_PI * EPSSIL / 
                model->MOSoxideCapFactor ;
        }

        sMOSinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            double fact2 = inst->MOStemp/REFTEMP;
            double vt    = inst->MOStemp * CONSTKoverQ;
            double ratio = inst->MOStemp/model->MOStnom;
            double egfet = 1.16- (7.02e-4*inst->MOStemp*inst->MOStemp)/
                    (inst->MOStemp + 1108);
            tmp = inst->MOStemp * CONSTboltz;
            tmp = -egfet/(tmp+tmp) + 1.1150877/(CONSTboltz*(REFTEMP+REFTEMP));
            double pbfact = -2*vt*(1.5*log(fact2) + CHARGE*tmp);

            tmp = 1/(ratio*sqrt(ratio));
            if (model->MOSlevel == 6)
                inst->MOStTransconductance = model->MOSkc*tmp;
            else
                inst->MOStTransconductance = model->MOStransconductance*tmp;
            inst->MOStSurfMob = model->MOSsurfaceMobility*tmp;

            tmp = (model->MOSphi - pbfact1)/fact1;
            inst->MOStPhi = fact2*tmp + pbfact;

            if (model->MOStype > 0) {
                inst->MOStVbi =
                    model->MOSvt0 - model->MOSgamma*sqrt(model->MOSphi) +
                        .5*(egfet1 - egfet + inst->MOStPhi - model->MOSphi);
                inst->MOStVto =
                    inst->MOStVbi + model->MOSgamma*sqrt(inst->MOStPhi);
            }
            else {
                inst->MOStVbi = 
                    model->MOSvt0 + model->MOSgamma*sqrt(model->MOSphi) +
                        .5*(egfet1 - egfet - (inst->MOStPhi - model->MOSphi));
                inst->MOStVto =
                    inst->MOStVbi - model->MOSgamma*sqrt(inst->MOStPhi);
            }

            tmp = exp(-egfet/vt + egfet1/vtnom);
            inst->MOStSatCur = model->MOSjctSatCur*tmp;
            inst->MOStSatCurDens = model->MOSjctSatCurDensity*tmp;

            double pbo = (model->MOSbulkJctPotential - pbfact1)/fact1;
            double gmaold = (model->MOSbulkJctPotential - pbo)/pbo;
            double capfact = 1/(1 + model->MOSbulkJctBotGradingCoeff*
                    (4e-4*(model->MOStnom - REFTEMP) - gmaold));
            inst->MOStCbd = model->MOScapBD * capfact;
            inst->MOStCbs = model->MOScapBS * capfact;
            inst->MOStCj  = model->MOSbulkCapFactor * capfact;

            capfact = 1/(1 + model->MOSbulkJctSideGradingCoeff*
                    (4e-4*(model->MOStnom - REFTEMP) - gmaold));
            inst->MOStCjsw = model->MOSsideWallCapFactor * capfact;
            inst->MOStBulkPot = fact2*pbo + pbfact;

            double gmanew = (inst->MOStBulkPot-pbo)/pbo;
            capfact = (1 + model->MOSbulkJctBotGradingCoeff*
                    (4e-4*(inst->MOStemp - REFTEMP) - gmanew));
            inst->MOStCbd *= capfact;
            inst->MOStCbs *= capfact;
            inst->MOStCj  *= capfact;
            capfact = (1 + model->MOSbulkJctSideGradingCoeff*
                    (4e-4*(inst->MOStemp - REFTEMP) - gmanew));
            inst->MOStCjsw *= capfact;
            inst->MOStDepCap = model->MOSfwdCapDepCoeff * inst->MOStBulkPot;

            if ( (inst->MOStSatCurDens == 0) ||
                    (inst->MOSdrainArea == 0) ||
                    (inst->MOSsourceArea == 0) ) {
                inst->MOSsourceVcrit = inst->MOSdrainVcrit =
                        vt*log(vt/(CONSTroot2*inst->MOStSatCur));
                inst->MOStDrainSatCur = inst->MOStSatCur;
                inst->MOStSourceSatCur = inst->MOStSatCur;
            }
            else {
                inst->MOStDrainSatCur =
                    inst->MOStSatCurDens*inst->MOSdrainArea;
                inst->MOStSourceSatCur =
                    inst->MOStSatCurDens*inst->MOSsourceArea;
                inst->MOSdrainVcrit =
                        vt*log(vt/(CONSTroot2*inst->MOStDrainSatCur));
                inst->MOSsourceVcrit =
                        vt*log(vt/(CONSTroot2*inst->MOStSourceSatCur));
            }

            if (model->MOScapBDGiven)
                inst->MOSCbd = inst->MOStCbd;
            else {
                if (model->MOSbulkCapFactorGiven)
                    inst->MOSCbd = inst->MOStCj*inst->MOSdrainArea;
                else
                    inst->MOSCbd = 0;
            }
            if (model->MOSsideWallCapFactorGiven)
                inst->MOSCbdsw = inst->MOStCjsw*inst->MOSdrainPerimeter;
            else
                inst->MOSCbdsw = 0;

            if (model->MOScapBSGiven)
                inst->MOSCbs = inst->MOStCbs;
            else {
                if(model->MOSbulkCapFactorGiven)
                    inst->MOSCbs = inst->MOStCj*inst->MOSsourceArea;
                else
                    inst->MOSCbs = 0;
            }
            if (model->MOSsideWallCapFactorGiven)
                inst->MOSCbssw = inst->MOStCjsw*inst->MOSsourcePerimeter;
            else
                inst->MOSCbssw = 0;

            fd(model, inst);
            fs(model, inst);

            // cache a few useful parameters

            inst->MOSgateSourceOverlapCap =
                model->MOSgateSourceOverlapCapFactor * inst->MOSw;
            inst->MOSgateDrainOverlapCap =
                model->MOSgateDrainOverlapCapFactor * inst->MOSw;
            inst->MOSgateBulkOverlapCap =
                model->MOSgateBulkOverlapCapFactor * inst->MOSeffectiveLength;
            inst->MOSbeta = inst->MOStTransconductance *
                inst->MOSw/inst->MOSeffectiveLength;
            inst->MOSoxideCap =
                model->MOSoxideCapFactor * inst->MOSeffectiveLength * 
                    inst->MOSw;

        }
    }
    return (OK);
}


static int
mos_dopsetup(sMOSmodel *model, double vtnom, double egfet1)
{
    double doping = model->MOSsubstrateDoping * CM3PM3;
    if (doping > 1.45e16) {
        if (!model->MOSphiGiven) {
            model->MOSphi = 2*vtnom*log(doping/1.45e16);
            model->MOSphi = SPMAX(.1,model->MOSphi);
        }
        if (!model->MOSgateTypeGiven)
            model->MOSgateType = 1;
        if (!model->MOSgammaGiven)
            model->MOSgamma =
                sqrt(2*EPSSIL*CHARGE*doping)/model->MOSoxideCapFactor;
        if (!model->MOSvt0Given) {

            double wkfng = 3.2;
            if (model->MOSgateType != 0) {
                if (model->MOStype > 0)
                    wkfng = 3.25 + .5*egfet1*(1 - model->MOSgateType);
                else
                    wkfng = 3.25 + .5*egfet1*(1 + model->MOSgateType);
            }
            double wkfngs;
            if (model->MOStype > 0)
                wkfngs = wkfng - (3.25 + .5*(egfet1 + model->MOSphi));
            else
                wkfngs = wkfng - (3.25 + .5*(egfet1 - model->MOSphi));

            double vfb;
            if (!model->MOSsurfaceStateDensityGiven) {
                model->MOSsurfaceStateDensity = 0;
                vfb = wkfngs;
            }
            else
                vfb = wkfngs -
                    model->MOSsurfaceStateDensity*CM2PM2*CHARGE/
                        model->MOSoxideCapFactor;

            if (model->MOStype > 0)
                model->MOSvt0 = vfb + 
                    (model->MOSgamma*sqrt(model->MOSphi) + model->MOSphi);
            else
                model->MOSvt0 = vfb -
                    (model->MOSgamma*sqrt(model->MOSphi) + model->MOSphi);
        }
    }
    else {
        model->MOSsubstrateDoping = 0;
        DVO.textOut(OUT_FATAL,"%s: Nsub < Ni", model->GENmodName);
        return (E_BADPARM);
    }
    return (OK);
}


void
MOSdev::fd(sMOSmodel *model, sMOSinstance *inst)
{
    double arg    = 1 - model->MOSfwdCapDepCoeff;
    double sarg   = exp( (-model->MOSbulkJctBotGradingCoeff) * log(arg) );
    double sargsw = exp( (-model->MOSbulkJctSideGradingCoeff) * log(arg) );

    double pot   = inst->MOStBulkPot;
    double tmp1  = (1 - model->MOSfwdCapDepCoeff*
                (1 + model->MOSbulkJctBotGradingCoeff))/arg;
    double tmp2  = 1/(arg*pot);

    inst->MOSf2d = (inst->MOSCbd*sarg + inst->MOSCbdsw*sargsw)*tmp1;

    inst->MOSf3d =
        (inst->MOSCbd*model->MOSbulkJctBotGradingCoeff*sarg +
        inst->MOSCbdsw*model->MOSbulkJctSideGradingCoeff*sargsw)*tmp2;

    inst->MOSf4d =
        inst->MOSCbd*pot*(1 - arg*sarg)/
                (1 - model->MOSbulkJctBotGradingCoeff)
        + inst->MOSCbdsw*pot*(1 - arg*sargsw)/
                (1 - model->MOSbulkJctSideGradingCoeff)
        - inst->MOStDepCap*(inst->MOSf2d + .5*inst->MOSf3d*inst->MOStDepCap);
}


void
MOSdev::fs(sMOSmodel *model, sMOSinstance *inst)
{
    double arg    = 1 - model->MOSfwdCapDepCoeff;
    double sarg   = exp( (-model->MOSbulkJctBotGradingCoeff) * log(arg) );
    double sargsw = exp( (-model->MOSbulkJctSideGradingCoeff) * log(arg) );

    double pot   = inst->MOStBulkPot;
    double tmp1  = (1 - model->MOSfwdCapDepCoeff*
                (1 + model->MOSbulkJctBotGradingCoeff))/arg;
    double tmp2  = 1/(arg*pot);

    inst->MOSf2s = (inst->MOSCbs*sarg + inst->MOSCbssw*sargsw)*tmp1;

    inst->MOSf3s =
        (inst->MOSCbs*model->MOSbulkJctBotGradingCoeff*sarg +
        inst->MOSCbssw*model->MOSbulkJctSideGradingCoeff*sargsw)*tmp2;

    inst->MOSf4s =
        inst->MOSCbs*pot*(1 - arg*sarg)/
                (1 - model->MOSbulkJctBotGradingCoeff)
        + inst->MOSCbssw*pot*(1 - arg*sargsw)/
                (1 - model->MOSbulkJctSideGradingCoeff)
        - inst->MOStDepCap*(inst->MOSf2s + .5*inst->MOSf3s*inst->MOStDepCap);

    /* UCB has this for last 2 lines in MOS3.  error?
                (inst->MOS3tBulkPot*inst->MOS3tBulkPot)
        -inst->MOS3tBulkPot * inst->MOS3f2s;
    */
}
