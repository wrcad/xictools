
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
 $Id: mutpzld.cc,v 2.2 2015/09/09 18:22:21 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "inddefs.h"


int
MUTdev::pzLoad(sGENmodel *genmod, sCKT *ckt, IFcomplex *s)
{
    (void)ckt;
    sMUTmodel *model = static_cast<sMUTmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sMUTinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            if (inst->MUTfactor == 0.0) {
                inst->MUTfactor = inst->MUTcoupling *
                    sqrt(inst->MUTind1->INDinduct * inst->MUTind2->INDinduct);
            }
    
            double val = inst->MUTfactor;
            *(inst->MUTbr1br2)   -= val * s->real;
            *(inst->MUTbr1br2+1) -= val * s->imag;
            *(inst->MUTbr2br1)   -= val * s->real;
            *(inst->MUTbr2br1+1) -= val * s->imag;
        }
    }
    return (OK);
}

