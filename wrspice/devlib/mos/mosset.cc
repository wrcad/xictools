
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

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1989 Takayasu Sakurai
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mosdefs.h"


namespace {
    int get_node_ptr(sCKT *ckt, sMOSinstance *inst)
    {
        TSTALLOC(MOSDdPtr,   MOSdNode,      MOSdNode)
        TSTALLOC(MOSGgPtr,   MOSgNode,      MOSgNode)
        TSTALLOC(MOSSsPtr,   MOSsNode,      MOSsNode)
        TSTALLOC(MOSBbPtr,   MOSbNode,      MOSbNode)
        TSTALLOC(MOSDPdpPtr, MOSdNodePrime, MOSdNodePrime)
        TSTALLOC(MOSSPspPtr, MOSsNodePrime, MOSsNodePrime)
        TSTALLOC(MOSDdpPtr,  MOSdNode,      MOSdNodePrime)
        TSTALLOC(MOSGbPtr,   MOSgNode,      MOSbNode)
        TSTALLOC(MOSGdpPtr,  MOSgNode,      MOSdNodePrime)
        TSTALLOC(MOSGspPtr,  MOSgNode,      MOSsNodePrime)
        TSTALLOC(MOSSspPtr,  MOSsNode,      MOSsNodePrime)
        TSTALLOC(MOSBdpPtr,  MOSbNode,      MOSdNodePrime)
        TSTALLOC(MOSBspPtr,  MOSbNode,      MOSsNodePrime)
        TSTALLOC(MOSDPspPtr, MOSdNodePrime, MOSsNodePrime)
        TSTALLOC(MOSDPdPtr,  MOSdNodePrime, MOSdNode)
        TSTALLOC(MOSBgPtr,   MOSbNode,      MOSgNode)
        TSTALLOC(MOSDPgPtr,  MOSdNodePrime, MOSgNode)
        TSTALLOC(MOSSPgPtr,  MOSsNodePrime, MOSgNode)
        TSTALLOC(MOSSPsPtr,  MOSsNodePrime, MOSsNode)
        TSTALLOC(MOSDPbPtr,  MOSdNodePrime, MOSbNode)
        TSTALLOC(MOSSPbPtr,  MOSsNodePrime, MOSbNode)
        TSTALLOC(MOSSPdpPtr, MOSsNodePrime, MOSdNodePrime)
        return (OK);
    }
}


int
MOSdev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
    sMOSmodel *model = static_cast<sMOSmodel*>(genmod);
    for ( ; model; model = model->next()) {

        if (!model->MOStypeGiven)
            model->MOStype = NMOS;
        if (!model->MOSlevelGiven ||
                (model->MOSlevel != 1 &&
                model->MOSlevel != 2 &&
                model->MOSlevel != 3 &&
                model->MOSlevel != 6)) {
            model->MOSlevel = 1;
        }
        // tnom

        // vto
        if (!model->MOSvt0Given)
            model->MOSvt0 = 0;

        // kp
        if (!model->MOStransconductanceGiven)
            model->MOStransconductance = 2e-5;

        // gamma
        if (!model->MOSgammaGiven)
            model->MOSgamma = 0;

        // phi
        if (!model->MOSphiGiven)
            model->MOSphi = .6;

        // rd
        if (!model->MOSdrainResistanceGiven)
            model->MOSdrainResistance = 0;

        // rs
        if (!model->MOSsourceResistanceGiven)
            model->MOSsourceResistance = 0;

        // cbd
        if (!model->MOScapBDGiven)
            model->MOScapBD = 0;

        // cbs
        if (!model->MOScapBSGiven)
            model->MOScapBS = 0;

        // is
        if (!model->MOSjctSatCurGiven)
            model->MOSjctSatCur = 1e-14;

        // pb
        if (!model->MOSbulkJctPotentialGiven)
            model->MOSbulkJctPotential = .8;

        // cgso
        if (!model->MOSgateSourceOverlapCapFactorGiven)
            model->MOSgateSourceOverlapCapFactor = 0;

        // cgdo
        if (!model->MOSgateDrainOverlapCapFactorGiven)
            model->MOSgateDrainOverlapCapFactor = 0;

        // cgbo
        if (!model->MOSgateBulkOverlapCapFactorGiven)
            model->MOSgateBulkOverlapCapFactor = 0;

        // cj
        if (!model->MOSbulkCapFactorGiven)
            model->MOSbulkCapFactor = 0;

        // mj
        if (!model->MOSbulkJctBotGradingCoeffGiven)
            model->MOSbulkJctBotGradingCoeff = .5;

        // cjsw
        if (!model->MOSsideWallCapFactorGiven)
            model->MOSsideWallCapFactor = 0;

        // mjsw
        if (!model->MOSbulkJctSideGradingCoeffGiven) {
            if (model->MOSlevel == 1 || model->MOSlevel == 6)
                model->MOSbulkJctSideGradingCoeff = .5;
            else
                model->MOSbulkJctSideGradingCoeff = .33;
        }

        // js
        if (!model->MOSjctSatCurDensityGiven)
            model->MOSjctSatCurDensity = 0;

        // tox
        if (!model->MOSoxideThicknessGiven) {
            if (model->MOSlevel == 2 || model->MOSlevel == 3)
                model->MOSoxideThickness = 1e-7;
            else
                model->MOSoxideThickness = 0;
        }

        // ld
        if (!model->MOSlatDiffGiven)
            model->MOSlatDiff = 0;

        // rsh
        if (!model->MOSsheetResistanceGiven)
            model->MOSsheetResistance = 0;

        // u0

        // fc
        if (!model->MOSfwdCapDepCoeffGiven)
            model->MOSfwdCapDepCoeff = .5;

        // nss
        // nsub
        // tpg

        // kf
        if (!model->MOSfNcoefGiven)
            model->MOSfNcoef = 0;

        // af
        if (!model->MOSfNexpGiven)
            model->MOSfNexp = 1;

        // lambda, 1,2 and 6
        if (!model->MOSlambdaGiven)
            model->MOSlambda = 0;

        // uexp, 2
        if (!model->MOScritFieldExpGiven)
            model->MOScritFieldExp = 0;

        // neff, 2
        if (!model->MOSchannelChargeGiven)
            model->MOSchannelCharge = 1;

        // ucrit, 2
        if (!model->MOScritFieldGiven)
            model->MOScritField = 1e4;

        // nfs, 2 and 3
        if (!model->MOSfastSurfaceStateDensityGiven)
            model->MOSfastSurfaceStateDensity = 0;

        // delta, 2 and 3
        if (!model->MOSdeltaGiven)
            model->MOSdelta = 0;
        if (!model->MOSnarrowFactorGiven)
            model->MOSnarrowFactor = 0;

        // vmax, 2 and 3
        if (!model->MOSmaxDriftVelGiven)
            model->MOSmaxDriftVel = 0;

        // xj, 2 and 3
        if (!model->MOSjunctionDepthGiven)
            model->MOSjunctionDepth = 0;

        // eta, 3
        if (!model->MOSetaGiven)
            model->MOSeta = 0;

        // theta, 3
        if (!model->MOSthetaGiven)
            model->MOStheta = 0;

        // kappa, 3
        if (!model->MOSkappaGiven)
            model->MOSkappa = .2;

        // level 6
        if (!model->MOSkvGiven)
            model->MOSkv = 2;
        if (!model->MOSnvGiven)
            model->MOSnv = 0.5;
        if (!model->MOSkcGiven)
            model->MOSkc = 5e-5;
        if (!model->MOSncGiven)
            model->MOSnc = 1;
        if (!model->MOSlamda0Given) {
            model->MOSlamda0 = 0;
            if (model->MOSlambdaGiven)
                model->MOSlamda0 = model->MOSlambda;
        }
        if (!model->MOSlamda1Given)
            model->MOSlamda1 = 0;
        if (!model->MOSsigmaGiven)
            model->MOSsigma = 0;
        if(!model->MOSgamma1Given)
            model->MOSgamma1 = 0;

        sMOSinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            inst->MOSmode = 1;
            inst->MOSvon = 0;

            // allocate a chunk of the state vector
            inst->GENstate = *states;
            *states += MOSnumStates;

            // m
            if (!inst->MOSmGiven)
                inst->MOSmult = 1.0;

            // temp
            if (!inst->MOStempGiven)
                inst->MOStemp = ckt->CKTcurTask->TSKtemp;

            // pd
            if (!inst->MOSdrainPerimeterGiven)
                inst->MOSdrainPerimeter = 0;

            // nrd
            if (!inst->MOSdrainSquaresGiven || inst->MOSdrainSquares == 0)
                inst->MOSdrainSquares = 1;

            // ps
            if (!inst->MOSsourcePerimeterGiven)
                inst->MOSsourcePerimeter = 0;

            // nrs
            if (!inst->MOSsourceSquaresGiven || inst->MOSsourceSquares == 0)
                inst->MOSsourceSquares = 1;

            // ic: vbs
            if (!inst->MOSicVBSGiven)
                inst->MOSicVBS = 0;

            // ic: vds
            if (!inst->MOSicVDSGiven)
                inst->MOSicVDS = 0;

            // ic: vgs
            if (!inst->MOSicVGSGiven)
                inst->MOSicVGS = 0;

            // no assigned parameter?
            if (!inst->MOSvdsatGiven)
                inst->MOSvdsat = 0;

            if (!inst->MOSlGiven)
                inst->MOSl = ckt->mos_default_l();
            if (!inst->MOSwGiven)
                inst->MOSw = ckt->mos_default_w();
            if (!inst->MOSdrainAreaGiven)
                inst->MOSdrainArea = ckt->mos_default_ad();
            if (!inst->MOSsourceAreaGiven)
                inst->MOSsourceArea = ckt->mos_default_as();

            if (model->MOSdrainResistanceGiven) {
                if (model->MOSdrainResistance != 0)
                    inst->MOSdrainConductance = 1/model->MOSdrainResistance;
                else
                    inst->MOSdrainConductance = 0;
            }
            else if (model->MOSsheetResistanceGiven) {
                if (model->MOSsheetResistance != 0)
                    inst->MOSdrainConductance = 1/(model->MOSsheetResistance*
                        inst->MOSdrainSquares);
                else
                    inst->MOSdrainConductance = 0;
            }
            else
                inst->MOSdrainConductance = 0;

            if (model->MOSsourceResistanceGiven) {
                if (model->MOSsourceResistance != 0)
                    inst->MOSsourceConductance = 1/model->MOSsourceResistance;
                else
                    inst->MOSsourceConductance = 0;
            }
            else if (model->MOSsheetResistanceGiven) {
                if (model->MOSsheetResistance != 0)
                    inst->MOSsourceConductance = 1/(model->MOSsheetResistance*
                        inst->MOSsourceSquares);
                else
                    inst->MOSsourceConductance = 0;
            }
            else
                inst->MOSsourceConductance = 0;

            inst->MOSeffectiveLength = inst->MOSl - 2*model->MOSlatDiff;
            if (inst->MOSeffectiveLength < 0) {
                DVO.textOut(OUT_WARNING,
                    "%s: effective channel length less than zero",
                    inst->GENname);
                inst->MOSeffectiveLength = 0;
            }

            // assign internal nodes

            if ((model->MOSdrainResistance != 0
                || model->MOSsheetResistance != 0)
                    && inst->MOSdNodePrime == 0) {
                sCKTnode *tmp;
                int error = ckt->mkVolt(&tmp, inst->GENname,
                    "internal#drain");
                if (error)
                    return(error);
                inst->MOSdNodePrime = tmp->number();
            }
            else
                inst->MOSdNodePrime = inst->MOSdNode;

            if (((model->MOSsourceResistance != 0) || 
                    ((inst->MOSsourceSquares != 0) &&
                     (model->MOSsheetResistance != 0))) && 
                        (inst->MOSsNodePrime == 0)) {
                sCKTnode *tmp;
                int error = ckt->mkVolt(&tmp, inst->GENname,
                    "internal#source");
                if (error)
                    return(error);
                inst->MOSsNodePrime = tmp->number();
            }
            else
                inst->MOSsNodePrime = inst->MOSsNode;


            int error = get_node_ptr(ckt, inst);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}


int
MOSdev::unsetup(sGENmodel *genmod, sCKT*)
{
    sMOSmodel *model = static_cast<sMOSmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sMOSinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            if (inst->MOSdNodePrime != inst->MOSdNode)
                inst->MOSdNodePrime = 0;
            if (inst->MOSsNodePrime != inst->MOSsNode)
                inst->MOSsNodePrime = 0;
        }
    }
    return (OK);
}


// SRW - reset the matrix element pointers.
//
int
MOSdev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sMOSmodel *model = (sMOSmodel*)inModel; model;
            model = model->next()) {
        for (sMOSinstance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}

