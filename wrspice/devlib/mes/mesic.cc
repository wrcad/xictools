
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
 $Id: mesic.cc,v 1.0 1998/01/30 05:31:38 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 S. Hwang
         1993 Stephen R. Whiteley
****************************************************************************/

#include "mesdefs.h"


int
MESdev::getic(sGENmodel *genmod, sCKT *ckt)
{
    //
    // grab initial conditions out of rhs array.   User specified, so use
    // external nodes to get values
    //
    sMESmodel *model = static_cast<sMESmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sMESinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            if (!inst->MESicVDSGiven)
                inst->MESicVDS = 
                        *(ckt->CKTrhs + inst->MESdrainNode) - 
                        *(ckt->CKTrhs + inst->MESsourceNode);
            if (!inst->MESicVGSGiven)
                inst->MESicVGS = 
                        *(ckt->CKTrhs + inst->MESgateNode) - 
                        *(ckt->CKTrhs + inst->MESsourceNode);
        }
    }
    return(OK);
}
