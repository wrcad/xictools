
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
 $Id: traacld.cc,v 2.10 2016/10/16 01:33:16 stevew Exp $
 *========================================================================*/

//-------------------------------------------------------------------------
// This is a general transmission line model derived from:
//  1) the spice3 TRA (lossless) model
//  2) the spice3 LTRA (lossy, convolution) model
//  3) the kspice TXL (lossy, Pade approximation convolution) model
// Authors:
//  1985 Thomas L. Quarles
//  1990 Jaijeet S. Roychowdhury
//  1990 Shen Lin
//  1992 Charles Hough
//  2002 Stephen R. Whiteley
// Copyright Regents of the University of California.  All rights reserved.
//-------------------------------------------------------------------------

#include "tradefs.h"


// Load for the ac case, the equations are the following:
//
// Y_0(s) * V_1(s) - I_1(s) =
//      exp(-lambda(s)*length) * (Y_0(s) * V_2(s) + I_2(s))
// Y_0(s) * V_2(s) - I_2(s) =
//       exp(-lambda(s)*length) * (Y_0(s) * V_1(s) + I_1(s))
//
// where Y_0(s) and lambda(s) are as follows:
//
// Y_0(s) = sqrt( (sC+G)/(sL+R) )
// lambda(s) = sqrt( (sC+G)*(sL+R) )
//
// for the RC, RLC, and LC cases, G=0. The RG case is handled
// exactly as the DC case, (and the above equations require
// reformulation because they become identical for the DC case.)
//
int
TRAdev::acLoad(sGENmodel *genmod, sCKT *ckt)
{
    sTRAmodel *model = static_cast<sTRAmodel*>(genmod);
    for ( ; model; model = model->next()) {
        sTRAinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            double y0_r, y0_i;
            double y0exp_r, y0exp_i;
            double explambda_r, explambda_i;

            if (inst->TRAcase == TRA_LC) {
                y0_r = 1.0/inst->TRAz;
                y0_i = 0.0;
                // Spice3 Bug
                double exparg_i = -inst->TRAtd*ckt->CKTomega;
                explambda_r = cos(exparg_i);
                explambda_i = sin(exparg_i);
                y0exp_r = y0_r*explambda_r;
                y0exp_i = y0_r*explambda_i;
            }
            else if (inst->TRAcase == TRA_RLC) {
                double wl = ckt->CKTomega*inst->TRAl;
                double theta = 0.5*atan(inst->TRAr/wl);

                double tmp = sqrt(inst->TRAr*inst->TRAr + wl*wl);
                double mag = sqrt(ckt->CKTomega*inst->TRAc/tmp);
                y0_r = mag*cos(theta);
                y0_i = mag*sin(theta);

                theta = M_PI/2 - theta;
                mag *= tmp;
                double lambda_r = mag*cos(theta);
                double lambda_i = mag*sin(theta);
                double exparg_r = -lambda_r*inst->TRAlength;
                double exparg_i = -lambda_i*inst->TRAlength;
                explambda_r = exp(exparg_r)*cos(exparg_i);
                explambda_i = exp(exparg_r)*sin(exparg_i);
                y0exp_r = y0_r*explambda_r - y0_i*explambda_i;
                y0exp_i = y0_r*explambda_i + y0_i*explambda_r;
            }
            else if (inst->TRAcase == TRA_RC) {
                y0_r = y0_i = sqrt(0.5*ckt->CKTomega*inst->TRAc/inst->TRAr);
                double lambda_r = sqrt(0.5*ckt->CKTomega*inst->TRAr*inst->TRAc);
                double lambda_i = lambda_r;
                double exparg_r = -lambda_r*inst->TRAlength;
                double exparg_i = -lambda_i*inst->TRAlength;
                explambda_r = exp(exparg_r)*cos(exparg_i);
                explambda_i = exp(exparg_r)*sin(exparg_i);
                y0exp_r = y0_r*explambda_r - y0_i*explambda_i;
                y0exp_i = y0_r*explambda_i + y0_i*explambda_r;
            }
            else if (inst->TRAcase == TRA_RG) {
                double tmp1 = inst->TRAlength*sqrt(inst->TRAr*inst->TRAg);
                double tmp2 = exp(-tmp1);
                tmp1 = exp(tmp1);
                double coshlrootGR = 0.5*(tmp1 + tmp2);

                double rRsLrGRorG;
                if (inst->TRAg <= 1.0e-10)  // hack!
                    rRsLrGRorG = inst->TRAlength*inst->TRAr;
                else
                    rRsLrGRorG = 0.5*(tmp1 - tmp2)*sqrt(inst->TRAr/inst->TRAg);

                double rGsLrGRorR;
                if (inst->TRAr <= 1.0e-10)  // hack!
                    rGsLrGRorR = inst->TRAlength*inst->TRAg;
                else
                    rGsLrGRorR = 0.5*(tmp1 - tmp2)*sqrt(inst->TRAg/inst->TRAr);
                rRsLrGRorG *= (1.0 + ckt->CKTcurTask->TSKgmin);

                *inst->TRAibr1Pos1Ptr += 1.0;
                *inst->TRAibr1Neg1Ptr -= 1.0;
                *inst->TRAibr1Pos2Ptr -= coshlrootGR;
                *inst->TRAibr1Neg2Ptr += coshlrootGR;
                *inst->TRAibr1Ibr2Ptr += rRsLrGRorG;

                *inst->TRAibr2Ibr2Ptr += coshlrootGR;
                *inst->TRAibr2Pos2Ptr -= rGsLrGRorR;
                *inst->TRAibr2Neg2Ptr += rGsLrGRorR;
                *inst->TRAibr2Ibr1Ptr += 1.0;

                *inst->TRApos1Ibr1Ptr += 1.0;
                *inst->TRAneg1Ibr1Ptr -= 1.0;
                *inst->TRApos2Ibr2Ptr += 1.0;
                *inst->TRAneg2Ibr2Ptr -= 1.0;
                continue;
            }
            else {
                double wl = ckt->CKTomega*inst->TRAl;
                double wc = ckt->CKTomega*inst->TRAc;

                double mag1 = sqrt(wc*wc + inst->TRAc*inst->TRAc);
                double theta1 = atan2(inst->TRAg, wc);
                double mag2 = sqrt(wl*wl + inst->TRAr*inst->TRAr);
                double theta2 = atan2(inst->TRAr, wl);
                double mag = sqrt(mag1/mag2);
                double theta = 0.5*(theta1 - theta2);
                y0_r = mag*cos(theta);
                y0_i = mag*sin(theta);

                mag *= mag2;
                theta = 0.5*(theta1 + theta2);
                double exparg_r = -mag*inst->TRAlength*cos(theta);
                double exparg_i = -mag*inst->TRAlength*sin(theta);
                explambda_r = exp(exparg_r)*cos(exparg_i);
                explambda_i = exp(exparg_r)*sin(exparg_i);
                y0exp_r = y0_r*explambda_r - y0_i*explambda_i;
                y0exp_i = y0_r*explambda_i + y0_i*explambda_r;
            }

            *(inst->TRAibr1Pos1Ptr + 0) += y0_r;
            *(inst->TRAibr1Pos1Ptr + 1) += y0_i;
            *(inst->TRAibr1Neg1Ptr + 0) -= y0_r;
            *(inst->TRAibr1Neg1Ptr + 1) -= y0_i;

            *(inst->TRAibr1Ibr1Ptr + 0) -= 1.0;

            *(inst->TRAibr1Pos2Ptr + 0) -= y0exp_r;
            *(inst->TRAibr1Pos2Ptr + 1) -= y0exp_i;
            *(inst->TRAibr1Neg2Ptr + 0) += y0exp_r;
            *(inst->TRAibr1Neg2Ptr + 1) += y0exp_i;

            *(inst->TRAibr1Ibr2Ptr + 0) -= explambda_r;
            *(inst->TRAibr1Ibr2Ptr + 1) -= explambda_i;

            *(inst->TRAibr2Pos2Ptr + 0) += y0_r;
            *(inst->TRAibr2Pos2Ptr + 1) += y0_i;
            *(inst->TRAibr2Neg2Ptr + 0) -= y0_r;
            *(inst->TRAibr2Neg2Ptr + 1) -= y0_i;

            *(inst->TRAibr2Ibr2Ptr + 0) -= 1.0;

            *(inst->TRAibr2Pos1Ptr + 0) -= y0exp_r;
            *(inst->TRAibr2Pos1Ptr + 1) -= y0exp_i;
            *(inst->TRAibr2Neg1Ptr + 0) += y0exp_r;
            *(inst->TRAibr2Neg1Ptr + 1) += y0exp_i;

            *(inst->TRAibr2Ibr1Ptr + 0) -= explambda_r;
            *(inst->TRAibr2Ibr1Ptr + 1) -= explambda_i;

            *(inst->TRApos1Ibr1Ptr + 0) += 1.0;
            *(inst->TRAneg1Ibr1Ptr + 0) -= 1.0;
            *(inst->TRApos2Ibr2Ptr + 0) += 1.0;
            *(inst->TRAneg2Ibr2Ptr + 0) -= 1.0;
        }
    }
    return (OK);
}

