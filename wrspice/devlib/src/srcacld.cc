
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1987 Kanwar Jit Singh
         1992 Stephen R. Whiteley
****************************************************************************/

#include "srcdefs.h"
#include "input.h"


int
SRCdev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    sSRCmodel *model = static_cast<sSRCmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sSRCinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {
            
            // If SRCtree:
            // Get the function and its derivatives from the
            // field in the instance structure. The field is 
            // an array of doubles holding the rhs, and the
            // entries of the jacobian.

            if (inst->SRCacTabName) {
                if (!inst->SRCacTab) {
                    if (!IP.tablFind(inst->SRCacTabName, &inst->SRCacTab, ckt))
                        return (E_NOTFOUND);
                }
            }
            if (inst->SRCtype == SRC_V) {

                if (inst->SRCposNode) {
                    *(inst->SRCposIbrptr) += 1.0;
                    *(inst->SRCibrPosptr) += 1.0;
                }
                if (inst->SRCnegNode) {
                    *(inst->SRCnegIbrptr) -= 1.0;
                    *(inst->SRCibrNegptr) -= 1.0;
                }

                if (inst->SRCacGiven) {
                    if (inst->SRCacTab) {
                        double freq = ckt->CKTomega/(2*M_PI);
                        IFcomplex c = inst->SRCacTab->tablEval(freq);
                        *(ckt->CKTrhs  + (inst->SRCbranch)) += c.real;
                        *(ckt->CKTirhs + (inst->SRCbranch)) += c.imag;
                    }
                    else {
                        *(ckt->CKTrhs  + (inst->SRCbranch)) += inst->SRCacReal;
                        *(ckt->CKTirhs + (inst->SRCbranch)) += inst->SRCacImag;
                    }
                }
                else if (inst->SRCtree) {
                    double *derivs = inst->SRCacValues;
                    double rhs = (inst->SRCacValues)[inst->SRCtree->num_vars()];
                    for (int i = 0; i < inst->SRCtree->num_vars(); i++)
                        *(inst->SRCposptr[i]) -= derivs[i];
                    *(ckt->CKTrhs+(inst->SRCbranch)) += rhs;
                }
                else if (inst->SRCdep == SRC_CC) {
                    if (inst->SRCacTab) {
                        double freq = ckt->CKTomega/(2*M_PI);
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
                        double freq = ckt->CKTomega/(2*M_PI);
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
                if (inst->SRCacGiven) {
                    if (inst->SRCacTab) {
                        double freq = ckt->CKTomega/(2*M_PI);
                        IFcomplex c = inst->SRCacTab->tablEval(freq);
                        *(ckt->CKTrhs + (inst->SRCposNode)) -= c.real;
                        *(ckt->CKTrhs + (inst->SRCnegNode)) += c.real;
                        *(ckt->CKTirhs + (inst->SRCposNode)) -= c.imag;
                        *(ckt->CKTirhs + (inst->SRCnegNode)) += c.imag;
                    }
                    else {
                        *(ckt->CKTrhs + (inst->SRCposNode)) -= inst->SRCacReal;
                        *(ckt->CKTrhs + (inst->SRCnegNode)) += inst->SRCacReal;
                        *(ckt->CKTirhs + (inst->SRCposNode)) -= inst->SRCacImag;
                        *(ckt->CKTirhs + (inst->SRCnegNode)) += inst->SRCacImag;
                    }
                }
                else if (inst->SRCtree) {
                    double *derivs = inst->SRCacValues;
                    double rhs = (inst->SRCacValues)[inst->SRCtree->num_vars()];
                    for (int j = 0,i = 0; i < inst->SRCtree->num_vars(); i++) {
                        *(inst->SRCposptr[j++]) += derivs[i];
                        *(inst->SRCposptr[j++]) -= derivs[i];
                    }
                    *(ckt->CKTrhs+(inst->SRCposNode)) -= rhs;
                    *(ckt->CKTrhs+(inst->SRCnegNode)) += rhs;
                }
                else if (inst->SRCdep == SRC_CC) {
                    if (inst->SRCacTab) {
                        double freq = ckt->CKTomega/(2*M_PI);
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
                        double freq = ckt->CKTomega/(2*M_PI);
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
