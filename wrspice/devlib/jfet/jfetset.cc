
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
 $Id: jfetset.cc,v 1.3 2015/07/24 02:51:29 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/
/********** new in 3f2
Sydney University mods Copyright(c) 1989 Anthony E. Parker, David J. Skellern
    Laboratory for Communication Science Engineering
    Sydney University Department of Electrical Engineering, Australia
**********/

#include "jfetdefs.h"


namespace {
    int get_node_ptr(sCKT *ckt, sJFETinstance *inst)
    {
        TSTALLOC(JFETdrainDrainPrimePtr, JFETdrainNode, JFETdrainPrimeNode)
        TSTALLOC(JFETgateDrainPrimePtr, JFETgateNode, JFETdrainPrimeNode)
        TSTALLOC(JFETgateSourcePrimePtr, JFETgateNode, JFETsourcePrimeNode)
        TSTALLOC(JFETsourceSourcePrimePtr, JFETsourceNode,
                JFETsourcePrimeNode)
        TSTALLOC(JFETdrainPrimeDrainPtr, JFETdrainPrimeNode, JFETdrainNode)
        TSTALLOC(JFETdrainPrimeGatePtr, JFETdrainPrimeNode, JFETgateNode)
        TSTALLOC(JFETdrainPrimeSourcePrimePtr, JFETdrainPrimeNode,
                JFETsourcePrimeNode)
        TSTALLOC(JFETsourcePrimeGatePtr, JFETsourcePrimeNode, JFETgateNode)
        TSTALLOC(JFETsourcePrimeSourcePtr, JFETsourcePrimeNode,
                JFETsourceNode)
        TSTALLOC(JFETsourcePrimeDrainPrimePtr, JFETsourcePrimeNode,
                JFETdrainPrimeNode)
        TSTALLOC(JFETdrainDrainPtr, JFETdrainNode, JFETdrainNode)
        TSTALLOC(JFETgateGatePtr, JFETgateNode, JFETgateNode)
        TSTALLOC(JFETsourceSourcePtr, JFETsourceNode, JFETsourceNode)
        TSTALLOC(JFETdrainPrimeDrainPrimePtr, JFETdrainPrimeNode,
                JFETdrainPrimeNode)
        TSTALLOC(JFETsourcePrimeSourcePrimePtr, JFETsourcePrimeNode,
                JFETsourcePrimeNode)
        return (OK);
    }
}


int
JFETdev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
    sJFETmodel *model = static_cast<sJFETmodel*>(genmod);
    for ( ; model; model = model->next()) {

        if ( (model->JFETtype != NJF) && (model->JFETtype != PJF) )
            model->JFETtype = NJF;
        if (!model->JFETthresholdGiven)
            model->JFETthreshold = -2;
        if (!model->JFETbetaGiven)
            model->JFETbeta = 1e-4;
        if (!model->JFETlModulationGiven)
            model->JFETlModulation = 0;
        if (!model->JFETdrainResistGiven)
            model->JFETdrainResist = 0;
        if (!model->JFETsourceResistGiven)
            model->JFETsourceResist = 0;
        if (!model->JFETcapGSGiven)
            model->JFETcapGS = 0;
        if (!model->JFETcapGDGiven)
            model->JFETcapGD = 0;
        if (!model->JFETgatePotentialGiven)
            model->JFETgatePotential = 1;
        if (!model->JFETgateSatCurrentGiven)
            model->JFETgateSatCurrent = 1e-14;
        if (!model->JFETdepletionCapCoeffGiven)
            model->JFETdepletionCapCoeff = .5;
        if (!model->JFETfNcoefGiven)
            model->JFETfNcoef = 0;
        if (!model->JFETfNexpGiven)
            model->JFETfNexp = 1;

        // Modification for Sydney University JFET model
        if (!model->JFETbGiven)
            model->JFETb = 1.0;

        if (model->JFETdrainResist != 0)
            model->JFETdrainConduct = 1/model->JFETdrainResist;
        else
            model->JFETdrainConduct = 0;
        if (model->JFETsourceResist != 0)
            model->JFETsourceConduct = 1/model->JFETsourceResist;
        else
            model->JFETsourceConduct = 0;

        sJFETinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            
            if (!inst->JFETareaGiven)
                inst->JFETarea = 1;
            inst->GENstate = *states;
            *states += JFETnumStates;

            sCKTnode *tmp;
            if (model->JFETsourceResist != 0 &&
                    inst->JFETsourcePrimeNode == 0) {
                int error = ckt->mkVolt(&tmp, inst->GENname, "source");
                if (error)
                    return (error);
                inst->JFETsourcePrimeNode = tmp->number();
            }
            else
                inst->JFETsourcePrimeNode = inst->JFETsourceNode;
            if (model->JFETdrainResist != 0 &&
                    inst->JFETdrainPrimeNode == 0) {
                int error = ckt->mkVolt(&tmp, inst->GENname, "drain");
                if (error)
                    return (error);
                inst->JFETdrainPrimeNode = tmp->number();
            }
            else
                inst->JFETdrainPrimeNode = inst->JFETdrainNode;

            int error = get_node_ptr(ckt, inst);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}


int
JFETdev::unsetup(sGENmodel *genmod, sCKT*)
{
    sJFETmodel *model = static_cast<sJFETmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sJFETinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            if (inst->JFETsourcePrimeNode != inst->JFETsourceNode)
                inst->JFETsourcePrimeNode = 0;
            if (inst->JFETdrainPrimeNode != inst->JFETdrainNode)
                inst->JFETdrainPrimeNode = 0;
        }
    }
    return (OK);
}


// SRW - reset the matrix element pointers.
//
int
JFETdev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sJFETmodel *model = (sJFETmodel*)inModel; model;
            model = model->next()) {
        for (sJFETinstance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}

