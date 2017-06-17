
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
 $Id: messet.cc,v 1.3 2015/07/24 02:51:30 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 S. Hwang
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mesdefs.h"


namespace {
    int get_node_ptr(sCKT *ckt, sMESinstance *inst)
    {
        TSTALLOC(MESdrainDrainPrimePtr, MESdrainNode, MESdrainPrimeNode)
        TSTALLOC(MESgateDrainPrimePtr, MESgateNode, MESdrainPrimeNode)
        TSTALLOC(MESgateSourcePrimePtr, MESgateNode, MESsourcePrimeNode)
        TSTALLOC(MESsourceSourcePrimePtr, MESsourceNode,
                MESsourcePrimeNode)
        TSTALLOC(MESdrainPrimeDrainPtr, MESdrainPrimeNode, MESdrainNode)
        TSTALLOC(MESdrainPrimeGatePtr, MESdrainPrimeNode, MESgateNode)
        TSTALLOC(MESdrainPrimeSourcePrimePtr, MESdrainPrimeNode,
                MESsourcePrimeNode)
        TSTALLOC(MESsourcePrimeGatePtr, MESsourcePrimeNode, MESgateNode)
        TSTALLOC(MESsourcePrimeSourcePtr, MESsourcePrimeNode,
                MESsourceNode)
        TSTALLOC(MESsourcePrimeDrainPrimePtr, MESsourcePrimeNode,
                MESdrainPrimeNode)
        TSTALLOC(MESdrainDrainPtr, MESdrainNode, MESdrainNode)
        TSTALLOC(MESgateGatePtr, MESgateNode, MESgateNode)
        TSTALLOC(MESsourceSourcePtr, MESsourceNode, MESsourceNode)
        TSTALLOC(MESdrainPrimeDrainPrimePtr, MESdrainPrimeNode,
                MESdrainPrimeNode)
        TSTALLOC(MESsourcePrimeSourcePrimePtr, MESsourcePrimeNode,
                MESsourcePrimeNode)
        return (OK);
    }
}


int
MESdev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
    sMESmodel *model = static_cast<sMESmodel*>(genmod);
    for ( ; model; model = model->next()) {

        if ( (model->MEStype != NMF) && (model->MEStype != PMF) )
            model->MEStype = NMF;
        if (!model->MESthresholdGiven)
            model->MESthreshold = -2;
        if (!model->MESbetaGiven)
            model->MESbeta = 2.5e-3;
        if (!model->MESbGiven)
            model->MESb = 0.3;
        if (!model->MESalphaGiven)
            model->MESalpha = 2;
        if (!model->MESlModulationGiven)
            model->MESlModulation = 0;
        if (!model->MESdrainResistGiven)
            model->MESdrainResist = 0;
        if (!model->MESsourceResistGiven)
            model->MESsourceResist = 0;
        if (!model->MEScapGSGiven)
            model->MEScapGS = 0;
        if (!model->MEScapGDGiven)
            model->MEScapGD = 0;
        if (!model->MESgatePotentialGiven)
            model->MESgatePotential = 1;
        if (!model->MESgateSatCurrentGiven)
            model->MESgateSatCurrent = 1e-14;
        if (!model->MESdepletionCapCoeffGiven)
            model->MESdepletionCapCoeff = .5;
        if (!model->MESfNcoefGiven)
            model->MESfNcoef = 0;
        if (!model->MESfNexpGiven)
            model->MESfNexp = 1;

        sMESinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            
            if (!inst->MESareaGiven)
                inst->MESarea = 1;
            inst->GENstate = *states;
            *states += MESnumStates;

            sCKTnode *tmp;
            if (model->MESsourceResist != 0 &&
                    inst->MESsourcePrimeNode == 0) {
                int error = ckt->mkVolt(&tmp, inst->GENname, "source");
                if (error)
                    return (error);
                inst->MESsourcePrimeNode = tmp->number();
            }
            else
                inst->MESsourcePrimeNode = inst->MESsourceNode;
            if (model->MESdrainResist != 0 &&
                    inst->MESdrainPrimeNode == 0) {
                int error = ckt->mkVolt(&tmp, inst->GENname, "drain");
                if (error)
                    return (error);
                inst->MESdrainPrimeNode = tmp->number();
            }
            else
                inst->MESdrainPrimeNode = inst->MESdrainNode;

            int error = get_node_ptr(ckt, inst);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}


int
MESdev::unsetup(sGENmodel *genmod, sCKT*)
{
    sMESmodel *model = static_cast<sMESmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sMESinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            if (inst->MESsourcePrimeNode != inst->MESsourceNode)
                inst->MESsourcePrimeNode = 0;
            if (inst->MESdrainPrimeNode != inst->MESdrainNode)
                inst->MESdrainPrimeNode = 0;
        }
    }
    return (OK);
}


// SRW - reset the matrix element pointers.
//
int
MESdev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sMESmodel *model = (sMESmodel*)inModel; model;
            model = model->next()) {
        for (sMESinstance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}

