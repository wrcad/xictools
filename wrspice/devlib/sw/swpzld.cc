
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
 $Id: swpzld.cc,v 1.0 1998/01/30 05:33:45 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Gordon M. Jacobs
         1992 Stephen R. Whiteley
****************************************************************************/

#include "swdefs.h"


int
SWdev::pzLoad(sGENmodel *genmod, sCKT *ckt, IFcomplex *s)
{
    (void)s;
    sSWmodel *model = static_cast<sSWmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sSWinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            // In AC analysis, just propogate the state...
            int current_state = (int)*(ckt->CKTstate0 + inst->GENstate);
            double g_now =
                current_state ? model->SWonConduct : model->SWoffConduct;

            *(inst->SWposPosptr) += g_now;
            *(inst->SWposNegptr) -= g_now;
            *(inst->SWnegPosptr) -= g_now;
            *(inst->SWnegNegptr) += g_now;
        }
    }
    return (OK);
}
