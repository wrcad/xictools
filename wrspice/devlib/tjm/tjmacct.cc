
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
Author: 1992 Stephen R. Whiteley
****************************************************************************/

#include "tjmdefs.h"


int
TJMdev::accept(sCKT *ckt, sGENmodel *genmod)
{
    sTJMmodel *model = static_cast<sTJMmodel*>(genmod);
    for ( ; model; model = model->next()) {

        bool didm = false;
        double vmax = 0;

        sTJMinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

            // keep phase  >= 0 and < 2*PI
            double phi = *(ckt->CKTstate0 + inst->TJMphase);
            int pint = *(int *)(ckt->CKTstate1 + inst->TJMphsInt);
            if (phi >= 2*M_PI) {
                phi -= 2*M_PI;
                pint++;
            }
            else if (phi < 0) {
                phi += 2*M_PI;
                pint--;
            }
            *(ckt->CKTstate0 + inst->TJMphase) = phi;
            *(int *)(ckt->CKTstate0 + inst->TJMphsInt) = pint;

//            double dphase = phi;
//double vx = sqrt(PHI0_2PI/model->TJMcpic);  // should be 0.6857mV
//            double dvoltage = *(ckt->CKTstate0 + inst->TJMvoltage) / vx;
//            double ddvoltage = *(ckt->CKTstate0 + inst->TJMdvdt) * 1000*1e-12;

//XXX
            // This method called when time step is accepted.
            //void TJModel::Values(ModelContext* ctx)
/*
            double dphase;
            double dvoltage;
            double ddvoltage;
            if (tjm_i == 0) {
                dphase = -tjm_node_phases[tjm_j-1];
                dvoltage = -tjm_node_voltages[tjm_j-1];
                ddvoltage = -tjm_node_dvoltages[tjm_j-1];
            }
            elif (tjm_j == 0) {
                dphase = tjm_node_phases[tjm_i-1];
                dvoltage = tjm_node_voltages[tjm_i-1];
                ddvoltage = tjm_node_dvoltages[tjm_i-1];
            }
            else {
                dphase = tjm_node_phases[tjm_i-1] - tjm_node_phases[tjm_j-1];
                dvoltage = tjm_node_voltages[tjm_i-1] - tjm_node_voltages[tjm_j-1];
                ddvoltage = tjm_node_dvoltages[tjm_i-1] - tjm_node_dvoltages[tjm_j-1];
            }

            if (tjm_i == 0)
                dphase = -tjm_node_phases[tjm_j-1];
            else if (tjm_j == 0)
                dphase = tjm_node_phases[tjm_i-1];
            else
                dphase = tjm_node_phases[tjm_i-1] - tjm_node_phases[tjm_j-1];
*/

//            double sinphi2 = sin(dphase/2.0);
//            double cosphi2 = cos(dphase/2.0);

//            cIFcomplex FcSum(0.0, 0.0);
//            cIFcomplex FsSum(0.0, 0.0);
            for (int i = 0; i < model->tjm_narray; i++) {
                inst->tjm_Fc[i] = inst->tjm_Fcdt[i] +
                    inst->tjm_alpha1[i]*inst->tjm_cosphi2;
                inst->tjm_Fs[i] = inst->tjm_Fsdt[i] +
                    inst->tjm_alpha1[i]*inst->tjm_sinphi2;
                inst->tjm_Fcprev[i] = inst->tjm_Fc[i];
                inst->tjm_Fsprev[i] = inst->tjm_Fs[i];
                /*
                FcSum = FcSum + (model->tjm_A[i] +
                    model->tjm_B[i])*inst->tjm_Fc[i];
                FsSum = FsSum + (model->tjm_A[i] -
                    model->tjm_B[i])*inst->tjm_Fs[i];
                */
            }

            /*
            double curr = FcSum.real*sinphi2 + FsSum.real*cosphi2;
            double crhs = inst->TJMcriti*
                (-curr + model->tjm_sgw*(dvoltage +
                model->tjm_sgw*model->tjm_beta*ddvoltage));
//inst->TJMcurr = crhs;
            */

/*
            if (tjm_i == 0) {
                tjm_elem_currents[tjm_eindex] = -tjm_crit_current*
                    (-curr + tjm_sgw*(dvoltage + tjm_sgw*tjm_beta*ddvoltage));
                tjm_elem_phases[tjm_eindex] = -tjm_node_phases[tjm_j-1];
                tjm_elem_voltages[tjm_eindex] = tjm_.node_voltages[tjm_j-1];
            }
            else if (tjm_j == 0) {
                tjm_elem_currents[tjm_eindex] = tjm_crit_current*
                    (-curr + tjm_sgw*(dvoltage + tjm_sgw*tjm_beta*ddvoltage));
                tjm_elem_phases[tjm_eindex] = tjm_node_phases[tjm_i-1];
                tjm_elem_voltages[tjm_eindex] = tjm_node_voltages[tjm_i-1];
            }
            else {
                tjm_elem_currents[tjm_eindex] = tjm_crit_current*
                    (-curr + tjm_sgw*(dvoltage + tjm_sgw*tjm_beta*ddvoltage));
                tjm_elem_phases[tjm_eindex] =
                    (tjm_node_phases[tjm_i-1] - tjm_node_phases[tjm_j-1]);
                tjm_elem_voltages[tjm_eindex] =
                    (tjm_node_voltages[tjm_i-1] - tjm_node_voltages[tjm_j-1]);
            }

            NQuanta(tjm_elem_phases[tjm_eindex], 
                &tjm_elem_n[tjm_eindex], &tjm_elem_inc[tjm_eindex], 
                &tjm_elem_dec[tjm_eindex], ctx.histeresis);
*/
      //////


            // find max vj for time step
            if (model->TJMictype != 0 && inst->TJMcriti > 0) {
                if (!didm) {
                    didm = true;
                    if (vmax < model->TJMvdpbak)
                        vmax = model->TJMvdpbak;
                }
                double vj = *(ckt->CKTstate0 + inst->TJMvoltage);
                if (vj < 0)
                    vj = -vj;
                if (vmax < vj)
                    vmax = vj;
            }

            if (inst->TJMphsNode > 0)
                *(ckt->CKTrhsOld + inst->TJMphsNode) = phi + (2*M_PI)*pint;
        }
        if (vmax > 0.0) {
            // Limit next time step.
            double delmax = M_PI*model->TJMtsfact*PHI0_2PI/vmax;
            if (ckt->CKTdevMaxDelta == 0.0 || delmax < ckt->CKTdevMaxDelta)
                ckt->CKTdevMaxDelta = delmax;
        }
    }
    return (OK);
}

//XXX
#ifdef notdef

// This method called when time step is accepted.
void TJModel::Values(ModelContext* ctx)
{
    double dphase;
    double dvoltage;
    double ddvoltage;
    if (tjm_i == 0) {
        dphase = -tjm_node_phases[tjm_j-1];
        dvoltage = -tjm_node_voltages[tjm_j-1];
        ddvoltage = -tjm_node_dvoltages[tjm_j-1];
    }
    elif (tjm_j == 0) {
        dphase = tjm_node_phases[tjm_i-1];
        dvoltage = tjm_node_voltages[tjm_i-1];
        ddvoltage = tjm_node_dvoltages[tjm_i-1];
    }
    else {
        dphase = tjm_node_phases[tjm_i-1] - tjm_node_phases[tjm_j-1];
        dvoltage = tjm_node_voltages[tjm_i-1] - tjm_node_voltages[tjm_j-1];
        ddvoltage = tjm_node_dvoltages[tjm_i-1] - tjm_node_dvoltages[tjm_j-1];
    }

    if (tjm_i == 0)
        dphase = -tjm_node_phases[tjm_j-1];
    else if (tjm_j == 0)
        dphase = tjm_node_phases[tjm_i-1];
    else
        dphase = tjm_node_phases[tjm_i-1] - tjm_node_phases[tjm_j-1];

    double sinphi2 = sin(dphase/2.0);
    double cosphi2 = cos(dphase/2.0);

    cIFcomplex FcSum(0.0, 0.0);
    cIFcomplex FsSum(0.0, 0.0);
    for (int i = 0; i < tjm_narray; i++) {
        tjm_Fc[i] = tjm_Fcdt[i] + tjm_alpha1[i]*tjm_cosphi2;
        tjm_Fs[i] = tjm_Fsdt[i] + tjm_alpha1[i]*tjm_sinphi2;
        tjm_Fcprev[i] = tjm_Fc[i];
        tjm_Fsprev[i] = tjm_Fs[i];
        FcSum = FcSum + (tjm_A[i] + tjm_B[i])*tjm_Fc[i];
        FsSum = FsSum + (tjm_A[i] - tjm_B[i])*tjm_Fs[i];
    }

    double curr = FcSum.real*sinphi2 + FsSum.real*cosphi2;

    if (tjm_i == 0) {
        tjm_elem_currents[tjm_eindex] = -tjm_crit_current*
            (-curr + tjm_sgw*(dvoltage + tjm_sgw*tjm_beta*ddvoltage));
        tjm_elem_phases[tjm_eindex] = -tjm_node_phases[tjm_j-1];
        tjm_elem_voltages[tjm_eindex] = tjm_.node_voltages[tjm_j-1];
    }
    else if (tjm_j == 0) {
        tjm_elem_currents[tjm_eindex] = tjm_crit_current*
            (-curr + tjm_sgw*(dvoltage + tjm_sgw*tjm_beta*ddvoltage));
        tjm_elem_phases[tjm_eindex] = tjm_node_phases[tjm_i-1];
        tjm_elem_voltages[tjm_eindex] = tjm_node_voltages[tjm_i-1];
    }
    else {
        tjm_elem_currents[tjm_eindex] = tjm_crit_current*
            (-curr + tjm_sgw*(dvoltage + tjm_sgw*tjm_beta*ddvoltage));
        tjm_elem_phases[tjm_eindex] =
            (tjm_node_phases[tjm_i-1] - tjm_node_phases[tjm_j-1]);
        tjm_elem_voltages[tjm_eindex] =
            (tjm_node_voltages[tjm_i-1] - tjm_node_voltages[tjm_j-1]);
    }

    NQuanta(tjm_elem_phases[tjm_eindex], 
        &tjm_elem_n[tjm_eindex], &tjm_elem_inc[tjm_eindex], 
        &tjm_elem_dec[tjm_eindex], ctx.histeresis);
}
#endif
