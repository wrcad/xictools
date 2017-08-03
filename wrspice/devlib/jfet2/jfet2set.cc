
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
Based on jfetset.c
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles

Modified to add PS model and new parameter definitions ( Anthony E. Parker )
   Copyright 1994  Macquarie University, Sydney Australia.
   10 Feb 1994: Added call to jfetparm.h, used JFET_STATE_COUNT
**********/

#include "jfet2defs.h"

#define JFET2nextModel      next()
#define JFET2nextInstance   next()
#define JFET2instances      inst()
#define CKTnode sCKTnode
#define JFET2name GENname
#define CKTmkVolt(a, b, c, d) (a)->mkVolt(b, c, d)
#define CKTdltNNum(c, n)
#define CKTnomTemp CKTcurTask->TSKnomTemp


namespace {
    int get_node_ptr(sCKT *ckt, sJFET2instance *inst)
    {
        TSTALLOC(JFET2drainDrainPrimePtr,JFET2drainNode,JFET2drainPrimeNode)
        TSTALLOC(JFET2gateDrainPrimePtr,JFET2gateNode,JFET2drainPrimeNode)
        TSTALLOC(JFET2gateSourcePrimePtr,JFET2gateNode,JFET2sourcePrimeNode)
        TSTALLOC(JFET2sourceSourcePrimePtr,JFET2sourceNode,
                JFET2sourcePrimeNode)
        TSTALLOC(JFET2drainPrimeDrainPtr,JFET2drainPrimeNode,JFET2drainNode)
        TSTALLOC(JFET2drainPrimeGatePtr,JFET2drainPrimeNode,JFET2gateNode)
        TSTALLOC(JFET2drainPrimeSourcePrimePtr,JFET2drainPrimeNode,
                JFET2sourcePrimeNode)
        TSTALLOC(JFET2sourcePrimeGatePtr,JFET2sourcePrimeNode,JFET2gateNode)
        TSTALLOC(JFET2sourcePrimeSourcePtr,JFET2sourcePrimeNode,
                JFET2sourceNode)
        TSTALLOC(JFET2sourcePrimeDrainPrimePtr,JFET2sourcePrimeNode,
                JFET2drainPrimeNode)
        TSTALLOC(JFET2drainDrainPtr,JFET2drainNode,JFET2drainNode)
        TSTALLOC(JFET2gateGatePtr,JFET2gateNode,JFET2gateNode)
        TSTALLOC(JFET2sourceSourcePtr,JFET2sourceNode,JFET2sourceNode)
        TSTALLOC(JFET2drainPrimeDrainPrimePtr,JFET2drainPrimeNode,
                JFET2drainPrimeNode)
        TSTALLOC(JFET2sourcePrimeSourcePrimePtr,JFET2sourcePrimeNode,
                JFET2sourcePrimeNode)
        return (OK);
    }
}


int
JFET2dev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
    sJFET2model *model = static_cast<sJFET2model*>(genmod);
    sJFET2instance *here;

    int error;
    CKTnode *tmp;

    /*  loop through all the diode models */
    for( ; model != NULL; model = model->JFET2nextModel ) {

        if( (model->JFET2type != NJF) && (model->JFET2type != PJF) ) {
            model->JFET2type = NJF;
        }

        // SRW -- extracted from jfet2parm.h
        if (!model->JFET2acgamGiven) model->JFET2acgam = 0;
        if (!model->JFET2fNexpGiven) model->JFET2fNexp = 1;
        if (!model->JFET2betaGiven) model->JFET2beta = 1e-4;
        if (!model->JFET2capDSGiven) model->JFET2capds = 0;
        if (!model->JFET2capGDGiven) model->JFET2capgd = 0;
        if (!model->JFET2capGSGiven) model->JFET2capgs = 0;
        if (!model->JFET2deltaGiven) model->JFET2delta = 0;
        if (!model->JFET2hfetaGiven) model->JFET2hfeta = 0;
        if (!model->JFET2hfe1Given) model->JFET2hfe1 = 0;
        if (!model->JFET2hfe2Given) model->JFET2hfe2 = 0;
        if (!model->JFET2hfg1Given) model->JFET2hfg1 = 0;
        if (!model->JFET2hfg2Given) model->JFET2hfg2 = 0;
        if (!model->JFET2mvstGiven) model->JFET2mvst = 0;
        if (!model->JFET2mxiGiven) model->JFET2mxi = 0;
        if (!model->JFET2fcGiven) model->JFET2fc = 0.5;
        if (!model->JFET2ibdGiven) model->JFET2ibd = 0;
        if (!model->JFET2isGiven) model->JFET2is = 1e-14;
        if (!model->JFET2kfGiven) model->JFET2fNcoef = 0;
        if (!model->JFET2lamGiven) model->JFET2lambda = 0;
        if (!model->JFET2lfgamGiven) model->JFET2lfgam = 0;
        if (!model->JFET2lfg1Given) model->JFET2lfg1 = 0;
        if (!model->JFET2lfg2Given) model->JFET2lfg2 = 0;
        if (!model->JFET2nGiven) model->JFET2n = 1;
        if (!model->JFET2pGiven) model->JFET2p = 2;
        if (!model->JFET2phiGiven) model->JFET2phi = 1;
        if (!model->JFET2qGiven) model->JFET2q = 2;
        if (!model->JFET2rdGiven) model->JFET2rd = 0;
        if (!model->JFET2rsGiven) model->JFET2rs = 0;
        if (!model->JFET2taudGiven) model->JFET2taud = 0;
        if (!model->JFET2taugGiven) model->JFET2taug = 0;
        if (!model->JFET2vbdGiven) model->JFET2vbd = 1;
        if (!model->JFET2verGiven) model->JFET2ver = 0;
        if (!model->JFET2vstGiven) model->JFET2vst = 0;
        if (!model->JFET2vtoGiven) model->JFET2vto = -2;
        if (!model->JFET2xcGiven) model->JFET2xc = 0;
        if (!model->JFET2xiGiven) model->JFET2xi = 1000;
        if (!model->JFET2zGiven) model->JFET2z = 1;
        if (!model->JFET2hfgGiven) model->JFET2hfgam = model->JFET2lfgam;

        /* loop through all the instances of the model */
        for (here = model->JFET2instances; here != NULL ;
                here=here->JFET2nextInstance) {
            
            if(!here->JFET2areaGiven) {
                here->JFET2area = 1;
            }
            here->JFET2state = *states;
            *states += JFET2_STATE_COUNT + 1;

            if(model->JFET2rs != 0 && here->JFET2sourcePrimeNode==0) {
                error = CKTmkVolt(ckt,&tmp,here->JFET2name,"source");
                if(error) return(error);
                here->JFET2sourcePrimeNode = tmp->number();
            } else {
                here->JFET2sourcePrimeNode = here->JFET2sourceNode;
            }
            if(model->JFET2rd != 0 && here->JFET2drainPrimeNode==0) {
                error = CKTmkVolt(ckt,&tmp,here->JFET2name,"drain");
                if(error) return(error);
                here->JFET2drainPrimeNode = tmp->number();
            } else {
                here->JFET2drainPrimeNode = here->JFET2drainNode;
            }

/* macro to make elements with built in test for out of memory */
/*
#define TSTALLOC(ptr,first,second) \
if((here->ptr = SMPmakeElt(matrix,here->first,here->second))==(double *)NULL){\
    return(E_NOMEM);\
}
*/
            error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return(OK);
}


// SRW
#undef inst
#define JFET2model sJFET2model
#define JFET2instance sJFET2instance


int
JFET2dev::unsetup(sGENmodel *inModel, sCKT*)
{
    JFET2model *model;
    JFET2instance *here;

    for (model = (JFET2model *)inModel; model != NULL;
            model = model->JFET2nextModel)
    {
        for (here = model->JFET2instances; here != NULL;
                here=here->JFET2nextInstance)
        {
            if (here->JFET2sourcePrimeNode
                    && here->JFET2sourcePrimeNode != here->JFET2sourceNode)
            {
                CKTdltNNum(ckt, here->JFET2sourcePrimeNode);
                here->JFET2sourcePrimeNode = 0;
            }
            if (here->JFET2drainPrimeNode
                    && here->JFET2drainPrimeNode != here->JFET2drainNode)
            {
                CKTdltNNum(ckt, here->JFET2drainPrimeNode);
                here->JFET2drainPrimeNode = 0;
            }
        }
    }
    return OK;
}


// SRW - reset the matrix element pointers.
//
int
JFET2dev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sJFET2model *model = (sJFET2model*)inModel; model;
            model = model->next()) {
        for (sJFET2instance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}

