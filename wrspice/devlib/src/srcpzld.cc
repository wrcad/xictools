
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
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

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1987 Kanwar Jit Singh
         1992 Stephen R. Whiteley
****************************************************************************/

#include "srcdefs.h"


int
SRCdev::pzLoad(sGENmodel *genmod, sCKT *ckt, IFcomplex *s)
{
    sSRCmodel *model = static_cast<sSRCmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sSRCinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            
            if (inst->SRCtype == SRC_V) {

                if (inst->SRCacGiven) {
                    // an ac source,
                    // no effective contribution
                    // diagonal element made 1
                    //
                    *(inst->SRCposIbrptr)  +=  1.0 ;
                    *(inst->SRCnegIbrptr)  += -1.0 ;
                    *(inst->SRCibrIbrptr)  +=  1.0 ;
                    continue;
                }
                if (inst->SRCposNode) {
                    *(inst->SRCposIbrptr) += 1.0;
                    *(inst->SRCibrPosptr) += 1.0;
                }
                if (inst->SRCnegNode) {
                    *(inst->SRCnegIbrptr) -= 1.0;
                    *(inst->SRCibrNegptr) -= 1.0;
                }
                if (inst->SRCtree) {
                    // a dc source,
                    // the connecting nodes are shorted
                    // unless it has dependence
                    //

                    int i;
                    for (i = 0; i < inst->SRCtree->num_vars(); i++)
                        inst->SRCvalues[i] =
                            *(ckt->CKTrhsOld + inst->SRCeqns[i]);

                    double value;
                    if (inst->SRCtree->eval(
                            &value, inst->SRCvalues, inst->SRCderivs) == OK) {

                        for (i = 0; i < inst->SRCtree->num_vars(); i++)
                            *(inst->SRCposptr[i]) -= inst->SRCderivs[i];
                    }
                    else
                        return (E_BADPARM);
                }
                else if (inst->SRCdep == SRC_CC) {
                    if (inst->SRCacTab) {
                        double freq = fabs(s->imag)/(2*M_PI);
                        IFcomplex c = inst->SRCacTab->tablEval(freq);
                        *(inst->SRCibrContBrptr) += c.real;
                        *(inst->SRCibrContBrptr +1) += c.imag;
                    }
                    else {
                        *(inst->SRCibrContBrptr) += inst->SRCcoeff.real;
                        *(inst->SRCibrContBrptr +1) += inst->SRCcoeff.imag;
                    }
                }
                else if (inst->SRCdep == SRC_VC) {
                    if (inst->SRCacTab) {
                        double freq = fabs(s->imag)/(2*M_PI);
                        IFcomplex c = inst->SRCacTab->tablEval(freq);
                        *(inst->SRCibrContPosptr) -= c.real;
                        *(inst->SRCibrContPosptr +1) -= c.imag;
                        *(inst->SRCibrContNegptr) += c.real;
                        *(inst->SRCibrContNegptr +1) += c.imag;
                    }
                    else {
                        *(inst->SRCibrContPosptr) -= inst->SRCcoeff.real;
                        *(inst->SRCibrContPosptr +1) -= inst->SRCcoeff.imag;
                        *(inst->SRCibrContNegptr) += inst->SRCcoeff.real;
                        *(inst->SRCibrContNegptr +1) += inst->SRCcoeff.imag;
                    }
                }
            }
            else {
                if (inst->SRCacGiven)
                    continue;
                if (inst->SRCtree) {
                    int i;
                    for (i = 0; i < inst->SRCtree->num_vars(); i++)
                        inst->SRCvalues[i] =
                            *(ckt->CKTrhsOld + inst->SRCeqns[i]);

                    double value;
                    if (inst->SRCtree->eval(
                        &value, inst->SRCvalues, inst->SRCderivs) == OK) {

                        int j;
                        for (j = 0, i = 0; i < inst->SRCtree->num_vars(); i++) {
                            double deriv = inst->SRCderivs[i];
                            *(inst->SRCposptr[j++]) += deriv;
                            *(inst->SRCposptr[j++]) -= deriv;
                        }
                    }
                    else
                        return (E_BADPARM);
                }
                else if (inst->SRCdep == SRC_CC) {
                    if (inst->SRCacTab) {
                        double freq = fabs(s->imag)/(2*M_PI);
                        IFcomplex c = inst->SRCacTab->tablEval(freq);
                        *(inst->SRCposContBrptr) += c.real;
                        *(inst->SRCposContBrptr +1) += c.imag;
                        *(inst->SRCnegContBrptr) -= c.real;
                        *(inst->SRCposContBrptr +1) += c.imag;
                    }
                    else {
                        *(inst->SRCposContBrptr) += inst->SRCcoeff.real;
                        *(inst->SRCposContBrptr +1) += inst->SRCcoeff.imag;
                        *(inst->SRCnegContBrptr) -= inst->SRCcoeff.real;
                        *(inst->SRCnegContBrptr +1) -= inst->SRCcoeff.imag;
                    }
                }
                else if (inst->SRCdep == SRC_VC) {
                    if (inst->SRCacTab) {
                        double freq = fabs(s->imag)/(2*M_PI);
                        IFcomplex c = inst->SRCacTab->tablEval(freq);
                        *(inst->SRCposContPosptr) += c.real;
                        *(inst->SRCposContPosptr +1) += c.imag;
                        *(inst->SRCposContNegptr) -= c.real;
                        *(inst->SRCposContNegptr +1) -= c.imag;
                        *(inst->SRCnegContPosptr) -= c.real;
                        *(inst->SRCnegContPosptr +1) -= c.imag;
                        *(inst->SRCnegContNegptr) += c.real;
                        *(inst->SRCnegContNegptr +1) += c.imag;
                    }
                    else {
                        *(inst->SRCposContPosptr) += inst->SRCcoeff.real;
                        *(inst->SRCposContPosptr +1) += inst->SRCcoeff.imag;
                        *(inst->SRCposContNegptr) -= inst->SRCcoeff.real;
                        *(inst->SRCposContNegptr +1) -= inst->SRCcoeff.imag;
                        *(inst->SRCnegContPosptr) -= inst->SRCcoeff.real;
                        *(inst->SRCnegContPosptr +1) -= inst->SRCcoeff.imag;
                        *(inst->SRCnegContNegptr) += inst->SRCcoeff.real;
                        *(inst->SRCnegContNegptr +1) += inst->SRCcoeff.imag;
                    }
                }
            }
        }
    }
    return (OK);
}
