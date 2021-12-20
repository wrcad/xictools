
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
         1993 Stephen R. Whiteley
****************************************************************************/

#include "resdefs.h"


int
RESdev::loadTest(sGENinstance *in_inst, sCKT *ckt)
{
#ifdef USE_PRELOAD
    if (!(ckt->CKTmode & MODEAC)) {
        if (((sRESinstance*)in_inst)->RESusePreload)
            return (~OK);
    }
#else
    (void)in_inst;
    (void)ckt;
#endif
    return (OK);
}


int
RESdev::load(sGENinstance *in_inst, sCKT *ckt)
{
    sRESinstance *inst = (sRESinstance*)in_inst;

    if (inst->RESpolyCoeffs) {
        double vres = *(ckt->CKTrhsOld + inst->RESposNode) - 
            *(ckt->CKTrhsOld + inst->RESnegNode);
        double V = vres;
        double R = inst->RESpolyCoeffs[0];
        double V0 = 1.0;
        double D = 0.0;
        for (int i = 1; i < inst->RESpolyNumCoeffs; i++) {
            D += i*V0*inst->RESpolyCoeffs[i];
            V0 *= V;
            R += inst->RESpolyCoeffs[i]*V0;
        }
        double gmax = ckt->CKTcurTask->TSKgmax;
        if (gmax <= 0.0)
            gmax = RES_GMAX;

        double sc = inst->RESm/inst->REStcFactor;
        double rmin = sc/gmax;
        if (fabs(R) < rmin) {
            if (R < 0.0)
                R = -rmin;
            else
                R = rmin;
        }
        inst->RESresist = R;
        double G = sc/R;
        inst->RESconduct = G;
        double F = G*G*vres/sc;
        double fderiv = F * D;
        double rhs = vres*fderiv;

#ifdef NEWJJDC
        if ((ckt->CKTmode & MODEDC) && ckt->CKTjjDCphase) {
            int ntpos = nodetype(ckt, inst->RESposNode);
            int ntneg = nodetype(ckt, inst->RESnegNode);
            if (ntpos != VOLT && ntneg != VOLT) {
                // Load GMIN otherwise matrix might be singular.
                ckt->ldadd(inst->RESposPosptr, ckt->CKTcurTask->TSKgmin);
                ckt->ldadd(inst->RESnegNegptr, ckt->CKTcurTask->TSKgmin);
                ckt->ldadd(inst->RESposNegptr, ckt->CKTcurTask->TSKgmin);
                ckt->ldadd(inst->RESnegPosptr, ckt->CKTcurTask->TSKgmin);
                return (OK);
            }
            if (ntpos == VOLT && ntneg == PHASE) {
                ckt->ldadd(inst->RESposPosptr, inst->RESconduct);
                ckt->ldadd(inst->RESnegPosptr, -inst->RESconduct);
                ckt->ldadd(inst->RESposPosptr, -fderiv);
                ckt->ldadd(inst->RESnegPosptr, fderiv);
                ckt->rhsadd(inst->RESposNode, -rhs);
                return (OK);
            }
            if (ntpos == PHASE && ntneg == VOLT) {
                ckt->ldadd(inst->RESnegNegptr, inst->RESconduct);
                ckt->ldadd(inst->RESposNegptr, -inst->RESconduct);
                ckt->ldadd(inst->RESnegNegptr, -fderiv);
                ckt->ldadd(inst->RESposNegptr, fderiv);
                ckt->rhsadd(inst->RESnegNode, rhs);
                return (OK);
            }
            // Else none are PHASE, load normally.
        }
#endif

        ckt->ldadd(inst->RESposPosptr, G);
        ckt->ldadd(inst->RESnegNegptr, G);
        ckt->ldadd(inst->RESposNegptr, -G);
        ckt->ldadd(inst->RESnegPosptr, -G);

        ckt->ldadd(inst->RESposPosptr, -fderiv);
        ckt->ldadd(inst->RESnegNegptr, -fderiv);
        ckt->ldadd(inst->RESposNegptr, fderiv);
        ckt->ldadd(inst->RESnegPosptr, fderiv);
        ckt->rhsadd(inst->RESposNode, -rhs);
        ckt->rhsadd(inst->RESnegNode, rhs);

        if (ckt->CKTmode & MODEAC)
            inst->RESv = vres;
        return (OK);
    }
    if (inst->REStree && inst->REStree->num_vars() > 0) {
        int numvars = inst->REStree->num_vars();
        for (int i = 0; i < numvars; i++)
            inst->RESvalues[i] = *(ckt->CKTrhsOld + inst->RESeqns[i]);

        double R = 0;
        BEGIN_EVAL
        int ret = inst->REStree->eval(&R, inst->RESvalues, inst->RESderivs);
        END_EVAL
        if (ret != OK)
            return (ret);

        double gmax = ckt->CKTcurTask->TSKgmax;
        if (gmax <= 0.0)
            gmax = RES_GMAX;

        double sc = inst->RESm/inst->REStcFactor;
        double rmin = sc/gmax;
        if (fabs(R) < rmin) {
            if (R < 0.0)
                R = -rmin;
            else
                R = rmin;
        }
        inst->RESresist = R;
        double G = sc/R;
        inst->RESconduct = G;

        double vres = *(ckt->CKTrhsOld + inst->RESposNode) - 
            *(ckt->CKTrhsOld + inst->RESnegNode);
        double F = G*G*vres/sc;
        double rhs = 0.0;

#ifdef NEWJJDC
        if ((ckt->CKTmode & MODEDC) && ckt->CKTjjDCphase) {
            int ntpos = nodetype(ckt, inst->RESposNode);
            int ntneg = nodetype(ckt, inst->RESnegNode);
            if (ntpos != VOLT && ntneg != VOLT) {
                // Load GMIN otherwise matrix might be singular.
                ckt->ldadd(inst->RESposPosptr, ckt->CKTcurTask->TSKgmin);
                ckt->ldadd(inst->RESnegNegptr, ckt->CKTcurTask->TSKgmin);
                ckt->ldadd(inst->RESposNegptr, ckt->CKTcurTask->TSKgmin);
                ckt->ldadd(inst->RESnegPosptr, ckt->CKTcurTask->TSKgmin);
                return (OK);
            }
            if (ntpos == VOLT && ntneg == PHASE) {
                ckt->ldadd(inst->RESposPosptr, inst->RESconduct);
                ckt->ldadd(inst->RESnegPosptr, -inst->RESconduct);
                for (int j = 0, i = 0; i < numvars; i++) {
                    if (inst->REStree->vars()[i].type == IF_NODE) {
                        int n = inst->REStree->vars()[i].v.nValue->number();
                        if (nodetype(ckt, n) != VOLT) {
                            j += 2;
                            continue;
                        }
                    }
                    double fderiv = F * inst->RESderivs[i];
                    rhs += inst->RESvalues[i]*fderiv;
                    ckt->ldadd(inst->RESposptr[j++], -fderiv);
                    j++;
                }
                ckt->rhsadd(inst->RESposNode, -rhs);
                return (OK);
            }
            if (ntpos == PHASE && ntneg == VOLT) {
                ckt->ldadd(inst->RESnegNegptr, inst->RESconduct);
                ckt->ldadd(inst->RESposNegptr, -inst->RESconduct);
                for (int j = 0, i = 0; i < numvars; i++) {
                    if (inst->REStree->vars()[i].type == IF_NODE) {
                        int n = inst->REStree->vars()[i].v.nValue->number();
                        if (nodetype(ckt, n) != VOLT) {
                            j += 2;
                            continue;
                        }
                    }
                    double fderiv = F * inst->RESderivs[i];
                    rhs += inst->RESvalues[i]*fderiv;
                    j++;
                    ckt->ldadd(inst->RESposptr[j++], fderiv);
                }
                ckt->rhsadd(inst->RESnegNode, rhs);
                return (OK);
            }
            // Else none are PHASE, load normally.
        }
#endif

        ckt->ldadd(inst->RESposPosptr, G);
        ckt->ldadd(inst->RESnegNegptr, G);
        ckt->ldadd(inst->RESposNegptr, -G);
        ckt->ldadd(inst->RESnegPosptr, -G);

        for (int j = 0, i = 0; i < numvars; i++) {
            double fderiv = F * inst->RESderivs[i];
            rhs += inst->RESvalues[i]*fderiv;
            ckt->ldadd(inst->RESposptr[j++], -fderiv);
            ckt->ldadd(inst->RESposptr[j++], fderiv);
        }
        ckt->rhsadd(inst->RESposNode, -rhs);
        ckt->rhsadd(inst->RESnegNode, rhs);

        if (ckt->CKTmode & MODEAC)
            inst->RESv = vres;
        return (OK);
    }

    if (ckt->CKTmode & MODEAC) {
        // This is used in ac/noise only.
        inst->RESv = *(ckt->CKTrhsOld + inst->RESposNode) - 
            *(ckt->CKTrhsOld + inst->RESnegNode);
    }
#ifdef USE_PRELOAD
    else if (inst->RESusePreload) {
        // The resistor list is sorted, we don't need to
        // call load anymore, tell caller.
        return (LOAD_SKIP_FLAG);
    }
#endif

#ifdef NEWJJDC
    if ((ckt->CKTmode & MODEDC) && ckt->CKTjjDCphase) {
        int ntpos = nodetype(ckt, inst->RESposNode);
        int ntneg = nodetype(ckt, inst->RESnegNode);
        if (ntpos != VOLT && ntneg != VOLT) {
            // Load GMIN otherwise matrix might be singular.
            ckt->ldadd(inst->RESposPosptr, ckt->CKTcurTask->TSKgmin);
            ckt->ldadd(inst->RESnegNegptr, ckt->CKTcurTask->TSKgmin);
            ckt->ldadd(inst->RESposNegptr, ckt->CKTcurTask->TSKgmin);
            ckt->ldadd(inst->RESnegPosptr, ckt->CKTcurTask->TSKgmin);
            return (OK);
        }
        if (ntpos == VOLT && ntneg == PHASE) {
            ckt->ldadd(inst->RESposPosptr, inst->RESconduct);
            ckt->ldadd(inst->RESnegPosptr, -inst->RESconduct);
            return (OK);
        }
        if (ntpos == PHASE && ntneg == VOLT) {
            ckt->ldadd(inst->RESnegNegptr, inst->RESconduct);
            ckt->ldadd(inst->RESposNegptr, -inst->RESconduct);
            return (OK);
        }
        // Else none are PHASE, load normally.
    }
#endif

    ckt->ldadd(inst->RESposPosptr, inst->RESconduct);
    ckt->ldadd(inst->RESnegNegptr, inst->RESconduct);
    ckt->ldadd(inst->RESposNegptr, -inst->RESconduct);
    ckt->ldadd(inst->RESnegPosptr, -inst->RESconduct);
    return (OK);
}


/*-----------------------------------------------------------------------------
Theory:

Assume that the "resistance" expression given for the element is the
"large signal" resistance, as a function of terminal voltages and other
node voltages.  One can express the resistor current as:

(1)     Ip = G(Vp, Vn, Vx...) = (Vp - Vn)/R(Vp, Vn, Vx...)

Linearize around some "last" voltage vector.

(2)     Ip ~ G(Vpl, Vnl, Vxl...) +
            (Vp - Vpl)dG/dVp + (Vn - Vnl)dG/dVn + (Vx - Vxl)dG/dVx + ...

The derivatives are all evaluated at the linearization point.  Express with
R:

(3)     Ip ~ (Vpl - Vnl)/R +
            (Vp - Vpl)(1/R - Vpl*(dR/dVp)/R^2 + Vnl*(dR/dVp)/R^2) +
            (Vn - Vnl)(-Vpl*(dR/dVn)/R^2 - 1/R + Vnl*(dR/dVn)/R^2 +
            (Vx - Vxl)(-(Vpl - Vnl)*(dR/dVx)/R^2) + ...

Evaluate the parse tree for R at the linearization point.  This provides
the R value and the derivatives Dp, Dn, Dx, ...

(4)     Ip ~ (Vpl - Vnl)*(1/R)) +
            (Vp - Vpl)*(1/R - (Vpl - Vnl)*Dp/R^2) +
            (Vn - Vnl)*(-1/R - (Vpl - Vnl)*Dn/R^2) +
            (Vx - Vxl)*(-(Vpl - Vnl)*Dx/R^2) + ...

Define Vl = (Vpl - Vnl)
Define F = Vl/R^2

(5)     Ip ~ Vl/R +
            (Vp - Vpl)*(1/R - F*Dp) +
            (Vn - Vnl)*(-1/R - F*Dn) +
            (Vx - Vxl)(-F*Dx) + ...

Matrix loading:
    Gpp = (1/R - F*Dp)
    Gpn = (-1/R - F*Dn)
    Gpx = (-F*Dx)

    Gnn = (1/R + Vl*Dn/R^2)
    Gnp = (-1/R + Vl*Dp/R^2)
    Gnx = (Vl*Dx/R^2)
    ...

RHS:
    Vl/R - Vpl*(1/R - F*Dp) - Vnl*(-1/R - F*Dn) - Vxl*(-F*Dx) + ...

    = Vpl*F*Dp + Vnl*F*Dn + Vxl*F*Dx + ...

------------------------------------------------------------------------------*/

