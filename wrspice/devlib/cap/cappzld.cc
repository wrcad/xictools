
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
 $Id: cappzld.cc,v 1.0 1998/01/30 05:28:35 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "capdefs.h"


int
CAPdev::pzLoad(sGENmodel *genmod, sCKT *ckt, IFcomplex *s)
{
    (void)ckt;
    sCAPmodel *model = static_cast<sCAPmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sCAPinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
    
            double val = inst->CAPcapac;
            *(inst->CAPposPosptr   ) += val * s->real;
            *(inst->CAPposPosptr +1) += val * s->imag;
            *(inst->CAPnegNegptr   ) += val * s->real;
            *(inst->CAPnegNegptr +1) += val * s->imag;
            *(inst->CAPposNegptr   ) -= val * s->real;
            *(inst->CAPposNegptr +1) -= val * s->imag;
            *(inst->CAPnegPosptr   ) -= val * s->real;
            *(inst->CAPnegPosptr +1) -= val * s->imag;
        }
    }
    return (OK);
}
