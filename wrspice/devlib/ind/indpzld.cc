
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
 $Id: indpzld.cc,v 1.2 2015/06/14 01:32:00 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "inddefs.h"


int
INDdev::pzLoad(sGENmodel *genmod, sCKT *ckt, IFcomplex *s)
{
    (void)ckt;
    sINDmodel *model = static_cast<sINDmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sINDinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            double val = inst->INDinduct;
            *(inst->INDposIbrptr) += 1;
            *(inst->INDnegIbrptr) -= 1;
            *(inst->INDibrPosptr) += 1;
            *(inst->INDibrNegptr) -= 1;
            *(inst->INDibrIbrptr)   -= val * s->real;
            *(inst->INDibrIbrptr+1) -= val * s->imag;
        }
    }
    return (OK);
}

