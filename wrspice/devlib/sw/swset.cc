
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
 $Id: swset.cc,v 1.3 2011/12/18 01:16:14 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Gordon M. Jacobs
         1992 Stephen R. Whiteley
****************************************************************************/

#include "swdefs.h"


namespace {
    int get_node_ptr(sCKT *ckt, sSWinstance *inst)
    {
        TSTALLOC(SWposPosptr, SWposNode, SWposNode)
        TSTALLOC(SWposNegptr, SWposNode, SWnegNode)
        TSTALLOC(SWnegPosptr, SWnegNode, SWposNode)
        TSTALLOC(SWnegNegptr, SWnegNode, SWnegNode)
        return (OK);
    }
}


int
SWdev::setup(sGENmodel *genmod, sCKT *ckt, int *states)
{
    sSWmodel *model = static_cast<sSWmodel*>(genmod);
    for ( ; model; model = model->next()) {

        // Default Value Processing for Switch Model
        if (!model->SWvThreshGiven)
            model->SWvThreshold = 0;
        if (!model->SWvHystGiven)
            model->SWvHysteresis = 0;
        if (!model->SWiThreshGiven)
            model->SWiThreshold = 0;
        if (!model->SWiHystGiven)
            model->SWiHysteresis = 0;
        if (!model->SWonGiven)  {
            model->SWonConduct = SW_ON_CONDUCTANCE;
            model->SWonResistance = 1.0/model->SWonConduct;
        } 
        if (!model->SWoffGiven)  {
            model->SWoffConduct = SW_OFF_CONDUCTANCE;
            model->SWoffResistance = 1.0/model->SWoffConduct;
        }

        sSWinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (inst->SWcontName) {
                inst->SWcontBranch = ckt->findBranch(inst->SWcontName);
                if (inst->SWcontBranch == 0) {
                    DVO.textOut(OUT_FATAL,
                        "%s: unknown controlling source %s",
                        inst->GENname, inst->SWcontName);
                    return (E_BADPARM);
                }
            }

            inst->GENstate = *states;
            *states += SW_NUM_STATES;

            int error = get_node_ptr(ckt, inst);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}


// SRW - reset the matrix element pointers.
//
int
SWdev::resetup(sGENmodel *inModel, sCKT *ckt)
{
    for (sSWmodel *model = (sSWmodel*)inModel; model;
            model = model->next()) {
        for (sSWinstance *here = model->inst(); here;
                here = here->next()) {
            int error = get_node_ptr(ckt, here);
            if (error != OK)
                return (error);
        }
    }
    return (OK);
}

