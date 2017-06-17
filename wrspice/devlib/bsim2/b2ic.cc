
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
 $Id: b2ic.cc,v 1.0 1998/01/30 05:27:36 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Hong June Park, Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "b2defs.h"


int
B2dev::getic(sGENmodel *genmod, sCKT *ckt)
{
    //
    // grab initial conditions out of rhs array.   User specified, so use
    // external nodes to get values
    //
    sB2model *model = static_cast<sB2model*>(genmod);
    for ( ; model; model = model->next()) {
        sB2instance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (!inst->B2icVBSGiven) {
                inst->B2icVBS = 
                        *(ckt->CKTrhs + inst->B2bNode) - 
                        *(ckt->CKTrhs + inst->B2sNode);
            }
            if (!inst->B2icVDSGiven) {
                inst->B2icVDS = 
                        *(ckt->CKTrhs + inst->B2dNode) - 
                        *(ckt->CKTrhs + inst->B2sNode);
            }
            if (!inst->B2icVGSGiven) {
                inst->B2icVGS = 
                        *(ckt->CKTrhs + inst->B2gNode) - 
                        *(ckt->CKTrhs + inst->B2sNode);
            }
        }
    }
    return(OK);
}

