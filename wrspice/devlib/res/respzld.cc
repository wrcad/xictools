
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
 $Id: respzld.cc,v 1.1 2011/07/10 06:15:34 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "resdefs.h"


int
RESdev::pzLoad(sGENmodel *genmod, sCKT *ckt, IFcomplex *s)
{
    (void)ckt;
    (void)s;
    sRESmodel *model = static_cast<sRESmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sRESinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            
            *(inst->RESposPosptr) += inst->RESconduct;
            *(inst->RESnegNegptr) += inst->RESconduct;
            *(inst->RESposNegptr) -= inst->RESconduct;
            *(inst->RESnegPosptr) -= inst->RESconduct;
        }
    }
    return (OK);
}

