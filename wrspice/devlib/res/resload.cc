
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
 $Id: resload.cc,v 2.13 2016/03/10 04:39:40 stevew Exp $
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
        sRESinstance *inst = (sRESinstance*)in_inst;
        if (!inst->RESpolyCoeffs && (!inst->REStree ||
                inst->REStree->num_vars() == 0))
            return (~OK);
    }
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

        inst->RESresist = R;
        double G = 1.0/(R * inst->REStcFactor);
        if (G > gmax)
            G = gmax;
        else if (G < -gmax)
            G = -gmax;
        inst->RESconduct = G;

        ckt->ldadd(inst->RESposPosptr, G);
        ckt->ldadd(inst->RESnegNegptr, G);
        ckt->ldadd(inst->RESposNegptr, -G);
        ckt->ldadd(inst->RESnegPosptr, -G);

        double F = inst->REStcFactor*G*G*vres;
        double rhs = 0.0;
        double fderiv = F * D;
        rhs += vres*fderiv;
        ckt->ldadd(inst->RESposPosptr, -fderiv);
        ckt->ldadd(inst->RESnegNegptr, -fderiv);
        ckt->ldadd(inst->RESposNegptr, fderiv);
        ckt->ldadd(inst->RESnegPosptr, fderiv);
        ckt->rhsadd(inst->RESposNode, -rhs);
        ckt->rhsadd(inst->RESnegNode, rhs);
        if (ckt->CKTmode & MODEAC)
            inst->RESv = vres;
    }
    if (inst->REStree && inst->REStree->num_vars() > 0) {
        int numvars = inst->REStree->num_vars();
        for (int i = 0; i < numvars; i++)
            inst->RESvalues[i] = *(ckt->CKTrhsOld + inst->RESeqns[i]);

        double R = 0;
        BEGIN_EVAL
        int ret = inst->REStree->eval(&R, inst->RESvalues, inst->RESderivs);
        END_EVAL
        if (ret == OK) {
            double gmax = ckt->CKTcurTask->TSKgmax;
            if (gmax <= 0.0)
                gmax = RES_GMAX;

            inst->RESresist = R;
            double G = 1.0/(R * inst->REStcFactor);
            if (G > gmax)
                G = gmax;
            else if (G < -gmax)
                G = -gmax;
            inst->RESconduct = G;

            ckt->ldadd(inst->RESposPosptr, G);
            ckt->ldadd(inst->RESnegNegptr, G);
            ckt->ldadd(inst->RESposNegptr, -G);
            ckt->ldadd(inst->RESnegPosptr, -G);

            double vres = *(ckt->CKTrhsOld + inst->RESposNode) - 
                *(ckt->CKTrhsOld + inst->RESnegNode);
            double F = inst->REStcFactor*G*G*vres;
            double rhs = 0.0;
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
        }
        else
            return (ret);
        return (OK);
    }

    if (ckt->CKTmode & MODEAC) {
        // This is used in ac/noise only.
        inst->RESv = *(ckt->CKTrhsOld + inst->RESposNode) - 
            *(ckt->CKTrhsOld + inst->RESnegNode);
    }
#ifdef USE_PRELOAD
    else
        return (LOAD_SKIP_FLAG);
    // The resistor list is sorted, we don't need to
    // call load anymore, tell caller.
#else
    ckt->ldadd(inst->RESposPosptr, inst->RESconduct);
    ckt->ldadd(inst->RESnegNegptr, inst->RESconduct);
    ckt->ldadd(inst->RESposNegptr, -inst->RESconduct);
    ckt->ldadd(inst->RESnegPosptr, -inst->RESconduct);
#endif
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

