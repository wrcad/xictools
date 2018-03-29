
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "inddefs.h"


namespace {
    inline bool compute_ind(sINDinstance *inst)
    {
        double L = 0;
        BEGIN_EVAL
        int ret = inst->INDtree->eval(&L, inst->INDvalues, 0);
        END_EVAL
        if (ret == OK)
            inst->INDinduct = L;
        return (ret);
    }

    inline void compute_poly_ind(sINDinstance *inst)
    {
        double I = inst->INDvalues[0];
        double L = inst->INDpolyCoeffs[0];
        double I0 = 1.0;
        for (int i = 1; i < inst->INDpolyNumCoeffs; i++) {
            I0 *= I;
            L += inst->INDpolyCoeffs[i]*I0;
        }
        inst->INDinduct = L;
    }
}


//
// The inductors MUST be loaded AFTER the mutual inductors.
//


int
INDdev::load(sGENinstance *in_inst, sCKT *ckt)
{
    sINDinstance *inst = (sINDinstance*)in_inst;

    if (ckt->CKTmode & MODEDC) {
#ifdef NEWJJDC
        if (ckt->CKTjjDCphase) {
            // The voltage across the inductor is taken as the
            // inductor phase.  This is for use when doing DC analysis
            // when Josephson junctions are present.  See jjload.cc.

            double res = 2*M_PI*inst->INDinduct/wrsCONSTphi0;
            ckt->ldadd(inst->INDibrIbrptr, -res);
        }
#endif
        return (OK);
    }

    if (ckt->CKTmode & MODEINITFLOAT) {

        if (inst->INDpolyCoeffs) {
            *(ckt->CKTstate0 + inst->INDflux + 2) = 
                *(ckt->CKTrhsOld + inst->INDbrEq);
        }
        else if (inst->INDtree && inst->INDtree->num_vars() > 0) {
            int numvars = inst->INDtree->num_vars();
            for (int i = 0; i < numvars; i++) {
                *(ckt->CKTstate0 + inst->INDflux + 2 + i) =
                    *(ckt->CKTrhsOld + inst->INDeqns[i]);
            }
            // There is no need to recompute the inductance,
            // it does not change during NR iterations.
        }
        double flux = inst->INDinduct *
            *(ckt->CKTrhsOld + inst->INDbrEq) - inst->INDprevFlux;
        *(ckt->CKTstate0 + inst->INDflux) += flux;

        inst->INDprevFlux = *(ckt->CKTstate0 + inst->INDflux);
        ckt->integrate(inst->INDflux, inst->INDveq);

        ckt->rhsadd(inst->INDbrEq, inst->INDveq);
        ckt->ldadd(inst->INDibrIbrptr, -inst->INDreq);
#ifndef USE_PRELOAD
        if (inst->INDposNode) {
            ckt->ldset(inst->INDposIbrptr, 1.0);
            ckt->ldset(inst->INDibrPosptr, 1.0);
        }
        if (inst->INDnegNode) {
            ckt->ldset(inst->INDnegIbrptr, -1.0);
            ckt->ldset(inst->INDibrNegptr, -1.0);
        }
#endif
        return (OK);
    }

    if (ckt->CKTmode & MODEINITPRED) {

        if (inst->INDpolyCoeffs) {
            inst->INDvalues[0] = *(ckt->CKTstate1 + inst->INDflux + 2);
            compute_poly_ind(inst);
        }
        else if (inst->INDtree && inst->INDtree->num_vars() > 0) {
            // The INDvalues array contains the final values
            // from the previous time point.

            int numvars = inst->INDtree->num_vars();
            for (int i = 0; i < numvars; i++) {
                inst->INDvalues[i] =
                    *(ckt->CKTstate1 + inst->INDflux + 2 + i);
            }
            int ret = compute_ind(inst);
            if (ret != OK)
                return (ret);
        }
        inst->INDreq = ckt->CKTag[0] * inst->INDinduct;
        inst->INDveq = ckt->find_ceq(inst->INDflux);

        ckt->rhsadd(inst->INDbrEq, inst->INDveq);
        ckt->ldadd(inst->INDibrIbrptr, -inst->INDreq);

        inst->INDprevFlux = 0;
        *(ckt->CKTstate0 + inst->INDflux) = 0;
#ifndef USE_PRELOAD
        if (inst->INDposNode) {
            ckt->ldset(inst->INDposIbrptr, 1.0);
            ckt->ldset(inst->INDibrPosptr, 1.0);
        }
        if (inst->INDnegNode) {
            ckt->ldset(inst->INDnegIbrptr, -1.0);
            ckt->ldset(inst->INDibrNegptr, -1.0);
        }
#endif
        return (OK);
    }

    if (ckt->CKTmode & MODEINITTRAN) {

        double ival;
        if (inst->INDpolyCoeffs) {
            if (ckt->CKTmode & MODEUIC) {
                inst->INDvalues[0] = 0.0;
                *(ckt->CKTstate1 + inst->INDflux + 2) = 0.0;
                ival = inst->INDinitCond;
            }
            else {
                ival = *(ckt->CKTrhsOld + inst->INDbrEq);
                inst->INDvalues[0] = ival;
                *(ckt->CKTstate1 + inst->INDflux + 2) = ival;
            }
            compute_poly_ind(inst);
        }
        if (inst->INDtree && inst->INDtree->num_vars() > 0) {
            int numvars = inst->INDtree->num_vars();
            if (ckt->CKTmode & MODEUIC) {
                for (int i = 0; i < numvars; i++) {
                    inst->INDvalues[i] = 0.0;
                    *(ckt->CKTstate1 + inst->INDflux + 2 + i) =
                        inst->INDvalues[i];
                }
                ival = inst->INDinitCond;
            }
            else {
                for (int i = 0; i < numvars; i++) {
                    inst->INDvalues[i] =
                        *(ckt->CKTrhsOld + inst->INDeqns[i]);
                    *(ckt->CKTstate1 + inst->INDflux + 2 + i) =
                        inst->INDvalues[i];
                }
                ival = *(ckt->CKTrhsOld + inst->INDbrEq);
            }
            int ret = compute_ind(inst);
            if (ret != OK)
                return (ret);
        }
        else {
            if (ckt->CKTmode & MODEUIC)
                ival =  inst->INDinitCond;
            else
                ival = *(ckt->CKTrhsOld + inst->INDbrEq);
        }
        *(ckt->CKTstate1 + inst->INDflux) += inst->INDinduct * ival;

        inst->INDprevFlux = 0;
        *(ckt->CKTstate0 + inst->INDflux) = 0;

        inst->INDreq = ckt->CKTag[0] * inst->INDinduct;
        inst->INDveq = ckt->find_ceq(inst->INDflux);
        
        ckt->rhsadd(inst->INDbrEq, inst->INDveq);
        ckt->ldadd(inst->INDibrIbrptr, -inst->INDreq);
#ifndef USE_PRELOAD
        if (inst->INDposNode) {
            ckt->ldset(inst->INDposIbrptr, 1.0);
            ckt->ldset(inst->INDibrPosptr, 1.0);
        }
        if (inst->INDnegNode) {
            ckt->ldset(inst->INDnegIbrptr, -1.0);
            ckt->ldset(inst->INDibrNegptr, -1.0);
        }
#endif
    }
    return (OK);
}

