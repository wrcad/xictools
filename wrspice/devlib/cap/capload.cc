
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
         1992 Stephen R. Whiteley
****************************************************************************/

#include "capdefs.h"


namespace {
    inline bool compute_cap(sCAPinstance *inst)
    {
        double C = 0;
        BEGIN_EVAL
        int ret = inst->CAPtree->eval(&C, inst->CAPvalues, 0);
        END_EVAL
        if (ret == OK)
            inst->CAPcapac = C * inst->CAPtcFactor;
        return (ret);
    }

    inline void compute_poly_cap(sCAPinstance *inst)
    {
        double V = inst->CAPvalues[0];
        double C = inst->CAPpolyCoeffs[0];
        double V0 = 1.0;
        for (int i = 1; i < inst->CAPpolyNumCoeffs; i++) {
            V0 *= V;
            C += inst->CAPpolyCoeffs[i]*V0;
        }
        inst->CAPcapac = C * inst->CAPtcFactor;
    }
}


int
CAPdev::load(sGENinstance *in_inst, sCKT *ckt)
{
    if (ckt->CKTmode & MODEDC)
        return (OK);

    sCAPinstance *inst = (sCAPinstance*)in_inst;

    if (ckt->CKTmode & MODEINITFLOAT) {
        if (inst->CAPpolyCoeffs) {
            *(ckt->CKTstate0 + inst->CAPqcap + 2) = 
                (*(ckt->CKTrhsOld + inst->CAPposNode) - 
                *(ckt->CKTrhsOld + inst->CAPnegNode));
        }
        else if (inst->CAPtree && inst->CAPtree->num_vars() > 0) {
            int numvars = inst->CAPtree->num_vars();
            for (int i = 0; i < numvars; i++) {
                *(ckt->CKTstate0 + inst->CAPqcap + 2 + i) = 
                    *(ckt->CKTrhsOld + inst->CAPeqns[i]);
            }
            // There is no need to recompute the capacitance,
            // it does not change during NR iterations.
        }
        double qcap = inst->CAPcapac * 
            (*(ckt->CKTrhsOld + inst->CAPposNode) - 
            *(ckt->CKTrhsOld + inst->CAPnegNode));
        *(ckt->CKTstate0 + inst->CAPqcap) = qcap;

        ckt->integrate(inst->CAPqcap, inst->CAPceq);
        inst->CAPceq = ckt->find_ceq(inst->CAPqcap);

        if (inst->CAPposNode == 0) {
            ckt->ldadd(inst->CAPnegNegptr, inst->CAPgeq);
            ckt->rhsadd(inst->CAPnegNode, inst->CAPceq);
            return (OK);
        }
        if (inst->CAPnegNode == 0) {
            ckt->ldadd(inst->CAPposPosptr, inst->CAPgeq);
            ckt->rhsadd(inst->CAPposNode, -inst->CAPceq);
            return (OK);
        }
        ckt->ldadd(inst->CAPposPosptr, inst->CAPgeq);
        ckt->ldadd(inst->CAPnegNegptr, inst->CAPgeq);
        ckt->ldadd(inst->CAPposNegptr, -inst->CAPgeq);
        ckt->ldadd(inst->CAPnegPosptr, -inst->CAPgeq);
        ckt->rhsadd(inst->CAPposNode, -inst->CAPceq);
        ckt->rhsadd(inst->CAPnegNode, inst->CAPceq);
        return (OK);
    }

    if (ckt->CKTmode & MODEINITPRED) {
        if (inst->CAPpolyCoeffs) {
            inst->CAPvalues[0] = *(ckt->CKTstate1 + inst->CAPqcap + 2);
            compute_poly_cap(inst);
        }
        else if (inst->CAPtree && inst->CAPtree->num_vars() > 0) {
            // The CAPvalues array contains the final values
            // from the previous time point.

            int numvars = inst->CAPtree->num_vars();
            for (int i = 0; i < numvars; i++) {
                inst->CAPvalues[i] =
                    *(ckt->CKTstate1 + inst->CAPqcap + 2 + i);
            }
            compute_cap(inst);
        }
        inst->CAPgeq = ckt->CKTag[0] * inst->CAPcapac;
        inst->CAPceq = ckt->find_ceq(inst->CAPqcap);

        if (inst->CAPposNode == 0) {
            ckt->ldadd(inst->CAPnegNegptr, inst->CAPgeq);
            ckt->rhsadd(inst->CAPnegNode, inst->CAPceq);
            return (OK);
        }
        if (inst->CAPnegNode == 0) {
            ckt->ldadd(inst->CAPposPosptr, inst->CAPgeq);
            ckt->rhsadd(inst->CAPposNode, -inst->CAPceq);
            return (OK);
        }
        ckt->ldadd(inst->CAPposPosptr, inst->CAPgeq);
        ckt->ldadd(inst->CAPnegNegptr, inst->CAPgeq);
        ckt->ldadd(inst->CAPposNegptr, -inst->CAPgeq);
        ckt->ldadd(inst->CAPnegPosptr, -inst->CAPgeq);
        ckt->rhsadd(inst->CAPposNode, -inst->CAPceq);
        ckt->rhsadd(inst->CAPnegNode, inst->CAPceq);
        return (OK);
    }

    if (ckt->CKTmode & MODEINITTRAN) {
        double vcap;
        if (inst->CAPpolyCoeffs) {
            if (ckt->CKTmode & MODEUIC) {
                inst->CAPvalues[0] = 0.0;
                *(ckt->CKTstate1 + inst->CAPqcap + 2) = 0.0;
                vcap = inst->CAPinitCond;
            }
            else {
                vcap = *(ckt->CKTrhsOld + inst->CAPposNode) -
                    *(ckt->CKTrhsOld + inst->CAPnegNode);
                inst->CAPvalues[0] = vcap;
                *(ckt->CKTstate1 + inst->CAPqcap + 2) = vcap;
            }
            compute_poly_cap(inst);
        }
        else if (inst->CAPtree && inst->CAPtree->num_vars() > 0) {
            int numvars = inst->CAPtree->num_vars();
            if (ckt->CKTmode & MODEUIC) {
                for (int i = 0; i < numvars; i++) {
                    inst->CAPvalues[i] = 0.0;
                    *(ckt->CKTstate1 + inst->CAPqcap + 2 + i) = 0.0;
                }
                vcap = inst->CAPinitCond;
            }
            else {
                for (int i = 0; i < numvars; i++) {
                    inst->CAPvalues[i] =
                        *(ckt->CKTrhsOld + inst->CAPeqns[i]);
                    *(ckt->CKTstate1 + inst->CAPqcap + 2 + i) =
                        inst->CAPvalues[i];
                }
                vcap = *(ckt->CKTrhsOld + inst->CAPposNode) -
                    *(ckt->CKTrhsOld + inst->CAPnegNode);
            }
            int ret = compute_cap(inst);
            if (ret != OK)
                return (ret);
        }
        else {
            if (ckt->CKTmode & MODEUIC)
                vcap = inst->CAPinitCond;
            else
                vcap = *(ckt->CKTrhsOld + inst->CAPposNode) -
                    *(ckt->CKTrhsOld + inst->CAPnegNode);
        }
        *(ckt->CKTstate1 + inst->CAPqcap) = inst->CAPcapac * vcap;
        inst->CAPgeq = ckt->CKTag[0] * inst->CAPcapac;
        inst->CAPceq = ckt->find_ceq(inst->CAPqcap);

        ckt->ldadd(inst->CAPposPosptr, inst->CAPgeq);
        ckt->ldadd(inst->CAPnegNegptr, inst->CAPgeq);
        ckt->ldadd(inst->CAPposNegptr, -inst->CAPgeq);
        ckt->ldadd(inst->CAPnegPosptr, -inst->CAPgeq);
        ckt->rhsadd(inst->CAPposNode, -inst->CAPceq);
        ckt->rhsadd(inst->CAPnegNode, inst->CAPceq);
    }
    return (OK);
}

/*-----------------------------------------------------------------------------
Theory:

Capacitance is defined as

(1) C = dQ/dV   (Note: NOT Q/V!)

implying

    CdV = dQ, CdV/dt = dQ/dt = I

For trapezoidal integration:

    (i + i0)*dt/2 = Q - Q0 = integral(V0,V)( Cdv )

Since (V - V0) is small, use a Taylor's series for C:

    integral(V0, V)( Cdv ) ~= integral(V0, V)( V(V0) + (V-V0)*(dC/dV |V0) dV )

    -> (V - V0)*C(V0) + 1/2 * (V - V0)^2 * (dC/dV |V0)

Throwing out the second-order terms:

    Q - Q0 ~= C(V0)*(V - V0)

Result: Use the same MNA stamp for a nonlinear capacitor, but use the
capacitance evaluated at the last voltage.

Related question:  "But, this can't be right, it predicts zero output from
a capacative microphone".

Consider a fixed voltage across a time-varying capacitor.  Then

    I = C(t)*dV/dt = 0 !?  (since dV/dt is 0)

Actually,

    C(t) = dQ(t)/dV, dQ(t)/dt = I,  so

    C(t)*dV = I*dt,

    d/dt [ integral(0,V)( C(t)*dV ) ] = I, or

    I = V*dC(t)/dt  (if C does not depend on V)
------------------------------------------------------------------------------*/

