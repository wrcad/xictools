
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
 $Id: bjtic.cc,v 1.0 1998/01/30 05:26:31 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "bjtdefs.h"


// This routine gets the device initial conditions for the BJTs
// from the RHS vector
//
int
BJTdev::getic(sGENmodel *genmod, sCKT *ckt)
{
    sBJTmodel *model = static_cast<sBJTmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sBJTinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            
            if (!inst->BJTicVBEGiven)
                inst->BJTicVBE = *(ckt->CKTrhs + inst->BJTbaseNode) - 
                    *(ckt->CKTrhs + inst->BJTemitNode);

            if (!inst->BJTicVCEGiven)
                inst->BJTicVCE = *(ckt->CKTrhs + inst->BJTcolNode) - 
                    *(ckt->CKTrhs + inst->BJTemitNode);
        }
    }
    return (OK);
}
