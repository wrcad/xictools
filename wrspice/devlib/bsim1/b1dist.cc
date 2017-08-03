
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
Authors: 1988 Jaijeet S Roychowdhury
         1993 Stephen R. Whiteley
****************************************************************************/

#define DISTO
#include "b1defs.h"
#include "distdefs.h"

int
B1dev::disto(int mode, sGENmodel *genmod, sCKT *ckt)
{
    sB1model *model = (sB1model*)genmod;
    sB1instance *inst;
    sGENinstance *geninst;
    sDISTOAN* job = (sDISTOAN*) ckt->CKTcurJob;
    DpassStr pass;
    double r1h1x,i1h1x;
    double r1h1y,i1h1y;
    double r1h1z, i1h1z;
    double r1h2x, i1h2x;
    double r1h2y, i1h2y;
    double r1h2z, i1h2z;
    double r1hm2x,i1hm2x;
    double r1hm2y,i1hm2y;
    double r1hm2z, i1hm2z;
    double r2h11x,i2h11x;
    double r2h11y,i2h11y;
    double r2h11z, i2h11z;
    double r2h1m2x,i2h1m2x;
    double r2h1m2y,i2h1m2y;
    double r2h1m2z, i2h1m2z;
    double temp, itemp;

    if (mode == D_SETUP)
        return (dSetup(model, ckt));

    if ((mode == D_TWOF1) || (mode == D_THRF1) || 
        (mode == D_F1PF2) || (mode == D_F1MF2) ||
        (mode == D_2F1MF2)) {

    for ( ; genmod != 0; genmod = genmod->GENnextModel) {
        model = (sB1model*)genmod;
        for (geninst = genmod->GENinstances; geninst != 0;
                geninst = geninst->GENnextInstance) {
            inst = (sB1instance*)geninst;

            // loading starts here

            switch (mode) {
            case D_TWOF1:
              // from now on, in the 3-var case, x=vgs,y=vbs,z=vds

              {
                // draincurrent term 
                r1h1x = *(job->r1H1ptr + inst->B1gNode) - 
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1gNode) - 
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1h1y = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1y = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1h1z = *(job->r1H1ptr + inst->B1dNodePrime) -
                    *(job->r1H1ptr + inst->B1sNodePrime);

                i1h1z = *(job->i1H1ptr + inst->B1dNodePrime) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                // draincurrent is a function of vgs,vbs,and vds;
                // have got their linear kernels; now to call
                // load functions 
                
                temp = DFn2F1(inst->DrC_x2,
                inst->DrC_y2,
                inst->DrC_z2,
                inst->DrC_xy,
                inst->DrC_yz,
                inst->DrC_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z);
                
                itemp = DFi2F1(inst->DrC_x2,
                inst->DrC_y2,
                inst->DrC_z2,
                inst->DrC_xy,
                inst->DrC_yz,
                inst->DrC_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z);
                
                *(ckt->CKTrhs + inst->B1dNodePrime) -= temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) -= itemp;
               
                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // draincurrent term loading over

                // loading qg term
                // kernels for vgs,vbs and vds already set up

                temp = -ckt->CKTomega * DFi2F1(inst->qg_x2,
                inst->qg_y2,
                inst->qg_z2,
                inst->qg_xy,
                inst->qg_yz,
                inst->qg_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z);

                itemp = ckt->CKTomega * DFn2F1(inst->qg_x2,
                inst->qg_y2,
                inst->qg_z2,
                inst->qg_xy,
                inst->qg_yz,
                inst->qg_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z);

                *(ckt->CKTrhs + inst->B1gNode) -= temp;
                *(ckt->CKTirhs + inst->B1gNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qg term over

                // loading qb term
                // kernels for vgs,vbs and vds already set up

                temp = -ckt->CKTomega * DFi2F1(inst->qb_x2,
                inst->qb_y2,
                inst->qb_z2,
                inst->qb_xy,
                inst->qb_yz,
                inst->qb_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z);

                itemp = ckt->CKTomega * DFn2F1(inst->qb_x2,
                inst->qb_y2,
                inst->qb_z2,
                inst->qb_xy,
                inst->qb_yz,
                inst->qb_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z);

                *(ckt->CKTrhs + inst->B1bNode) -= temp;
                *(ckt->CKTirhs + inst->B1bNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qb term over

                // loading qd term
                // kernels for vgs,vbs and vds already set up

                temp = -ckt->CKTomega * DFi2F1(inst->qd_x2,
                inst->qd_y2,
                inst->qd_z2,
                inst->qd_xy,
                inst->qd_yz,
                inst->qd_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z);

                itemp = ckt->CKTomega * DFn2F1(inst->qd_x2,
                inst->qd_y2,
                inst->qd_z2,
                inst->qd_xy,
                inst->qd_yz,
                inst->qd_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z);

                *(ckt->CKTrhs + inst->B1dNodePrime) -= temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qd term over

                // loading inst->B1gbs term

                r1h1x = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                // now r1h1x = vbs

                temp = D1n2F1(inst->gbs2,
                r1h1x,
                i1h1x);

                itemp = D1i2F1(inst->gbs2,
                r1h1x,
                i1h1x);

                *(ckt->CKTrhs + inst->B1bNode) -= temp;
                *(ckt->CKTirhs + inst->B1bNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // inst->B1gbs term over

                // loading inst->B1gbd term

                r1h1x = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1dNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1dNodePrime);

                // now r1h1x = vbd

                temp = D1n2F1(inst->gbd2,
                r1h1x,
                i1h1x);

                itemp = D1i2F1(inst->gbd2,
                r1h1x,
                i1h1x);

                *(ckt->CKTrhs + inst->B1gNode) -= temp;
                *(ckt->CKTirhs + inst->B1gNode) -= itemp;

                *(ckt->CKTrhs + inst->B1dNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) += itemp;

                // inst->B1gbd term over

                // all done
              }
              break;

            case D_THRF1:
              // from now on, in the 3-var case, x=vgs,y=vbs,z=vds

              {
                // draincurrent term
                r1h1x = *(job->r1H1ptr + inst->B1gNode) - 
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1gNode) - 
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1h1y = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1y = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1h1z = *(job->r1H1ptr + inst->B1dNodePrime) -
                    *(job->r1H1ptr + inst->B1sNodePrime);

                i1h1z = *(job->i1H1ptr + inst->B1dNodePrime) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r2h11x = *(job->r2H11ptr + inst->B1gNode) - 
                    *(job->r2H11ptr + inst->B1sNodePrime);
                i2h11x = *(job->i2H11ptr + inst->B1gNode) - 
                    *(job->i2H11ptr + inst->B1sNodePrime);

                r2h11y = *(job->r2H11ptr + inst->B1bNode) -
                    *(job->r2H11ptr + inst->B1sNodePrime);
                i2h11y = *(job->i2H11ptr + inst->B1bNode) -
                    *(job->i2H11ptr + inst->B1sNodePrime);

                r2h11z = *(job->r2H11ptr + inst->B1dNodePrime) -
                    *(job->r2H11ptr + inst->B1sNodePrime);

                i2h11z = *(job->i2H11ptr + inst->B1dNodePrime) -
                    *(job->i2H11ptr + inst->B1sNodePrime);

                // draincurrent is a function of vgs,vbs,and vds;
                // have got their linear kernels; now to call
                // load functions 
                
                temp = DFn3F1(inst->DrC_x2,
                inst->DrC_y2,
                inst->DrC_z2,
                inst->DrC_xy,
                inst->DrC_yz,
                inst->DrC_xz,
                inst->DrC_x3,
                inst->DrC_y3,
                inst->DrC_z3,
                inst->DrC_x2y,
                inst->DrC_x2z,
                inst->DrC_xy2,
                inst->DrC_y2z,
                inst->DrC_xz2,
                inst->DrC_yz2,
                inst->DrC_xyz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r2h11x,
                i2h11x,
                r2h11y,
                i2h11y,
                r2h11z,
                i2h11z);
              
                itemp = DFi3F1(inst->DrC_x2,
                inst->DrC_y2,
                inst->DrC_z2,
                inst->DrC_xy,
                inst->DrC_yz,
                inst->DrC_xz,
                inst->DrC_x3,
                inst->DrC_y3,
                inst->DrC_z3,
                inst->DrC_x2y,
                inst->DrC_x2z,
                inst->DrC_xy2,
                inst->DrC_y2z,
                inst->DrC_xz2,
                inst->DrC_yz2,
                inst->DrC_xyz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r2h11x,
                i2h11x,
                r2h11y,
                i2h11y,
                r2h11z,
                i2h11z);
               
                *(ckt->CKTrhs + inst->B1dNodePrime) -= temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) -= itemp;
               
                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // draincurrent term loading over

                // loading qg term
                // kernels for vgs,vbs and vds already set up

                temp = -ckt->CKTomega * DFi3F1(inst->qg_x2,
                inst->qg_y2,
                inst->qg_z2,
                inst->qg_xy,
                inst->qg_yz,
                inst->qg_xz,
                inst->qg_x3,
                inst->qg_y3,
                inst->qg_z3,
                inst->qg_x2y,
                inst->qg_x2z,
                inst->qg_xy2,
                inst->qg_y2z,
                inst->qg_xz2,
                inst->qg_yz2,
                inst->qg_xyz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r2h11x,
                i2h11x,
                r2h11y,
                i2h11y,
                r2h11z,
                i2h11z);

                itemp = ckt->CKTomega * DFn3F1(inst->qg_x2,
                inst->qg_y2,
                inst->qg_z2,
                inst->qg_xy,
                inst->qg_yz,
                inst->qg_xz,
                inst->qg_x3,
                inst->qg_y3,
                inst->qg_z3,
                inst->qg_x2y,
                inst->qg_x2z,
                inst->qg_xy2,
                inst->qg_y2z,
                inst->qg_xz2,
                inst->qg_yz2,
                inst->qg_xyz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r2h11x,
                i2h11x,
                r2h11y,
                i2h11y,
                r2h11z,
                i2h11z);

                *(ckt->CKTrhs + inst->B1gNode) -= temp;
                *(ckt->CKTirhs + inst->B1gNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qg term over

                // loading qb term
                // kernels for vgs,vbs and vds already set up

                temp = -ckt->CKTomega * DFi3F1(inst->qb_x2,
                inst->qb_y2,
                inst->qb_z2,
                inst->qb_xy,
                inst->qb_yz,
                inst->qb_xz,
                inst->qb_x3,
                inst->qb_y3,
                inst->qb_z3,
                inst->qb_x2y,
                inst->qb_x2z,
                inst->qb_xy2,
                inst->qb_y2z,
                inst->qb_xz2,
                inst->qb_yz2,
                inst->qb_xyz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r2h11x,
                i2h11x,
                r2h11y,
                i2h11y,
                r2h11z,
                i2h11z);

                itemp = ckt->CKTomega * DFn3F1(inst->qb_x2,
                inst->qb_y2,
                inst->qb_z2,
                inst->qb_xy,
                inst->qb_yz,
                inst->qb_xz,
                inst->qb_x3,
                inst->qb_y3,
                inst->qb_z3,
                inst->qb_x2y,
                inst->qb_x2z,
                inst->qb_xy2,
                inst->qb_y2z,
                inst->qb_xz2,
                inst->qb_yz2,
                inst->qb_xyz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r2h11x,
                i2h11x,
                r2h11y,
                i2h11y,
                r2h11z,
                i2h11z);

                *(ckt->CKTrhs + inst->B1bNode) -= temp;
                *(ckt->CKTirhs + inst->B1bNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qb term over

                // loading qd term
                // kernels for vgs,vbs and vds already set up

                temp = -ckt->CKTomega * DFi3F1(inst->qd_x2,
                inst->qd_y2,
                inst->qd_z2,
                inst->qd_xy,
                inst->qd_yz,
                inst->qd_xz,
                inst->qd_x3,
                inst->qd_y3,
                inst->qd_z3,
                inst->qd_x2y,
                inst->qd_x2z,
                inst->qd_xy2,
                inst->qd_y2z,
                inst->qd_xz2,
                inst->qd_yz2,
                inst->qd_xyz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r2h11x,
                i2h11x,
                r2h11y,
                i2h11y,
                r2h11z,
                i2h11z);

                itemp = ckt->CKTomega * DFn3F1(inst->qd_x2,
                inst->qd_y2,
                inst->qd_z2,
                inst->qd_xy,
                inst->qd_yz,
                inst->qd_xz,
                inst->qd_x3,
                inst->qd_y3,
                inst->qd_z3,
                inst->qd_x2y,
                inst->qd_x2z,
                inst->qd_xy2,
                inst->qd_y2z,
                inst->qd_xz2,
                inst->qd_yz2,
                inst->qd_xyz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r2h11x,
                i2h11x,
                r2h11y,
                i2h11y,
                r2h11z,
                i2h11z);

                *(ckt->CKTrhs + inst->B1dNodePrime) -= temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qd term over

                // loading inst->B1gbs term

                r1h1x = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r2h11x = *(job->r2H11ptr + inst->B1bNode) -
                    *(job->r2H11ptr + inst->B1sNodePrime);
                i2h11x = *(job->i2H11ptr + inst->B1bNode) -
                    *(job->i2H11ptr + inst->B1sNodePrime);

                // now r1h1x = vbs

                temp = D1n3F1(inst->gbs2,
                inst->gbs3,
                r1h1x,
                i1h1x,
                r2h11x,
                i2h11x);

                itemp = D1i3F1(inst->gbs2,
                inst->gbs3,
                r1h1x,
                i1h1x,
                r2h11x,
                i2h11x);

                *(ckt->CKTrhs + inst->B1bNode) -= temp;
                *(ckt->CKTirhs + inst->B1bNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // inst->B1gbs term over

                // loading inst->B1gbd term

                r1h1x = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1dNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1dNodePrime);

                r2h11x = *(job->r2H11ptr + inst->B1bNode) -
                    *(job->r2H11ptr + inst->B1dNodePrime);
                i2h11x = *(job->i2H11ptr + inst->B1bNode) -
                    *(job->i2H11ptr + inst->B1dNodePrime);

                // now r1h1x = vbd

                temp = D1n3F1(inst->gbd2,
                inst->gbd3,
                r1h1x,
                i1h1x,
                r2h11x,
                i2h11x);

                itemp = D1i3F1(inst->gbd2,
                inst->gbd3,
                r1h1x,
                i1h1x,
                r2h11x,
                i2h11x);

                *(ckt->CKTrhs + inst->B1bNode) -= temp;
                *(ckt->CKTirhs + inst->B1bNode) -= itemp;

                *(ckt->CKTrhs + inst->B1dNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) += itemp;

                // inst->B1gbd term over

                // all done
              }
              break;

            case D_F1PF2:
              // from now on, in the 3-var case, x=vgs,y=vbs,z=vds

              {
                // draincurrent term
                r1h1x = *(job->r1H1ptr + inst->B1gNode) - 
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1gNode) - 
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1h1y = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1y = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1h1z = *(job->r1H1ptr + inst->B1dNodePrime) -
                    *(job->r1H1ptr + inst->B1sNodePrime);

                i1h1z = *(job->i1H1ptr + inst->B1dNodePrime) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1h2x = *(job->r1H2ptr + inst->B1gNode) - 
                    *(job->r1H2ptr + inst->B1sNodePrime);
                i1h2x = (*(job->i1H2ptr + inst->B1gNode) - 
                    *(job->i1H2ptr + inst->B1sNodePrime));

                r1h2y = *(job->r1H2ptr + inst->B1bNode) -
                    *(job->r1H2ptr + inst->B1sNodePrime);
                i1h2y = (*(job->i1H2ptr + inst->B1bNode) -
                    *(job->i1H2ptr + inst->B1sNodePrime));

                r1h2z = *(job->r1H2ptr + inst->B1dNodePrime) -
                    *(job->r1H2ptr + inst->B1sNodePrime);

                i1h2z = (*(job->i1H2ptr + inst->B1dNodePrime) -
                    *(job->i1H2ptr + inst->B1sNodePrime));

                // draincurrent is a function of vgs,vbs,and vds;
                // have got their linear kernels; now to call
                // load functions 
                //
                
                temp = DFnF12(inst->DrC_x2,
                inst->DrC_y2,
                inst->DrC_z2,
                inst->DrC_xy,
                inst->DrC_yz,
                inst->DrC_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1h2x,
                i1h2x,
                r1h2y,
                i1h2y,
                r1h2z,
                i1h2z);
                
                itemp = DFiF12(inst->DrC_x2,
                inst->DrC_y2,
                inst->DrC_z2,
                inst->DrC_xy,
                inst->DrC_yz,
                inst->DrC_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1h2x,
                i1h2x,
                r1h2y,
                i1h2y,
                r1h2z,
                i1h2z);
                
                *(ckt->CKTrhs + inst->B1dNodePrime) -= temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) -= itemp;
                
                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // draincurrent term loading over

                // loading qg term
                // kernels for vgs,vbs and vds already set up

                temp = -ckt->CKTomega * DFiF12(inst->qg_x2,
                inst->qg_y2,
                inst->qg_z2,
                inst->qg_xy,
                inst->qg_yz,
                inst->qg_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1h2x,
                i1h2x,
                r1h2y,
                i1h2y,
                r1h2z,
                i1h2z);

                itemp = ckt->CKTomega * DFnF12(inst->qg_x2,
                inst->qg_y2,
                inst->qg_z2,
                inst->qg_xy,
                inst->qg_yz,
                inst->qg_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1h2x,
                i1h2x,
                r1h2y,
                i1h2y,
                r1h2z,
                i1h2z);

                *(ckt->CKTrhs + inst->B1gNode) -= temp;
                *(ckt->CKTirhs + inst->B1gNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qg term over

                // loading qb term
                // kernels for vgs,vbs and vds already set up

                temp = -ckt->CKTomega * DFiF12(inst->qb_x2,
                inst->qb_y2,
                inst->qb_z2,
                inst->qb_xy,
                inst->qb_yz,
                inst->qb_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1h2x,
                i1h2x,
                r1h2y,
                i1h2y,
                r1h2z,
                i1h2z);

                itemp = ckt->CKTomega * DFnF12(inst->qb_x2,
                inst->qb_y2,
                inst->qb_z2,
                inst->qb_xy,
                inst->qb_yz,
                inst->qb_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1h2x,
                i1h2x,
                r1h2y,
                i1h2y,
                r1h2z,
                i1h2z);

                *(ckt->CKTrhs + inst->B1bNode) -= temp;
                *(ckt->CKTirhs + inst->B1bNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qb term over

                // loading qd term
                // kernels for vgs,vbs and vds already set up

                temp = -ckt->CKTomega * DFiF12(inst->qd_x2,
                inst->qd_y2,
                inst->qd_z2,
                inst->qd_xy,
                inst->qd_yz,
                inst->qd_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1h2x,
                i1h2x,
                r1h2y,
                i1h2y,
                r1h2z,
                i1h2z);

                itemp = ckt->CKTomega * DFnF12(inst->qd_x2,
                inst->qd_y2,
                inst->qd_z2,
                inst->qd_xy,
                inst->qd_yz,
                inst->qd_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1h2x,
                i1h2x,
                r1h2y,
                i1h2y,
                r1h2z,
                i1h2z);

                *(ckt->CKTrhs + inst->B1dNodePrime) -= temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qd term over

                // loading inst->B1gbs term

                r1h1x = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1h2x = *(job->r1H2ptr + inst->B1bNode) -
                    *(job->r1H2ptr + inst->B1sNodePrime);
                i1h2x = *(job->i1H2ptr + inst->B1bNode) -
                    *(job->i1H2ptr + inst->B1sNodePrime);

                // now r1h1x = vbs

                temp = D1nF12(inst->gbs2,
                r1h1x,
                i1h1x,
                r1h2x,
                i1h2x);

                itemp = D1iF12(inst->gbs2,
                r1h1x,
                i1h1x,
                r1h2x,
                i1h2x);

                *(ckt->CKTrhs + inst->B1bNode) -= temp;
                *(ckt->CKTirhs + inst->B1bNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // inst->B1gbs term over

                // loading inst->B1gbd term

                r1h1x = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1dNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1dNodePrime);

                r1h2x = *(job->r1H2ptr + inst->B1bNode) -
                    *(job->r1H2ptr + inst->B1dNodePrime);
                i1h2x = *(job->i1H2ptr + inst->B1bNode) -
                    *(job->i1H2ptr + inst->B1dNodePrime);

                // now r1h1x = vbd

                temp = D1nF12(inst->gbd2,
                r1h1x,
                i1h1x,
                r1h2x,
                i1h2x);

                itemp = D1iF12(inst->gbd2,
                r1h1x,
                i1h1x,
                r1h2x,
                i1h2x);

                *(ckt->CKTrhs + inst->B1gNode) -= temp;
                *(ckt->CKTirhs + inst->B1gNode) -= itemp;

                *(ckt->CKTrhs + inst->B1dNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) += itemp;
                // inst->B1gbd term over

                // all done
              }
              break;

            case D_F1MF2:
              // from now on, in the 3-var case, x=vgs,y=vbs,z=vds

              {
                // draincurrent term
                r1h1x = *(job->r1H1ptr + inst->B1gNode) - 
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1gNode) - 
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1h1y = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1y = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1h1z = *(job->r1H1ptr + inst->B1dNodePrime) -
                    *(job->r1H1ptr + inst->B1sNodePrime);

                i1h1z = *(job->i1H1ptr + inst->B1dNodePrime) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1hm2x = *(job->r1H2ptr + inst->B1gNode) - 
                    *(job->r1H2ptr + inst->B1sNodePrime);
                i1hm2x = -(*(job->i1H2ptr + inst->B1gNode) - 
                    *(job->i1H2ptr + inst->B1sNodePrime));

                r1hm2y = *(job->r1H2ptr + inst->B1bNode) -
                    *(job->r1H2ptr + inst->B1sNodePrime);
                i1hm2y = -(*(job->i1H2ptr + inst->B1bNode) -
                    *(job->i1H2ptr + inst->B1sNodePrime));

                r1hm2z = *(job->r1H2ptr + inst->B1dNodePrime) -
                    *(job->r1H2ptr + inst->B1sNodePrime);

                i1hm2z = -(*(job->i1H2ptr + inst->B1dNodePrime) -
                    *(job->i1H2ptr + inst->B1sNodePrime));

                // draincurrent is a function of vgs,vbs,and vds;
                // have got their linear kernels; now to call
                // load functions 

                temp = DFnF12(inst->DrC_x2,
                inst->DrC_y2,
                inst->DrC_z2,
                inst->DrC_xy,
                inst->DrC_yz,
                inst->DrC_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1hm2x,
                i1hm2x,
                r1hm2y,
                i1hm2y,
                r1hm2z,
                i1hm2z);

                itemp = DFiF12(inst->DrC_x2,
                inst->DrC_y2,
                inst->DrC_z2,
                inst->DrC_xy,
                inst->DrC_yz,
                inst->DrC_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1hm2x,
                i1hm2x,
                r1hm2y,
                i1hm2y,
                r1hm2z,
                i1hm2z);

                *(ckt->CKTrhs + inst->B1dNodePrime) -= temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // draincurrent term loading over

                // loading qg term
                // kernels for vgs,vbs and vds already set up

                temp = -ckt->CKTomega * DFiF12(inst->qg_x2,
                inst->qg_y2,
                inst->qg_z2,
                inst->qg_xy,
                inst->qg_yz,
                inst->qg_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1hm2x,
                i1hm2x,
                r1hm2y,
                i1hm2y,
                r1hm2z,
                i1hm2z);

                itemp = ckt->CKTomega * DFnF12(inst->qg_x2,
                inst->qg_y2,
                inst->qg_z2,
                inst->qg_xy,
                inst->qg_yz,
                inst->qg_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1hm2x,
                i1hm2x,
                r1hm2y,
                i1hm2y,
                r1hm2z,
                i1hm2z);

                *(ckt->CKTrhs + inst->B1gNode) -= temp;
                *(ckt->CKTirhs + inst->B1gNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qg term over

                // loading qb term
                // kernels for vgs,vbs and vds already set up

                temp = -ckt->CKTomega * DFiF12(inst->qb_x2,
                inst->qb_y2,
                inst->qb_z2,
                inst->qb_xy,
                inst->qb_yz,
                inst->qb_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1hm2x,
                i1hm2x,
                r1hm2y,
                i1hm2y,
                r1hm2z,
                i1hm2z);

                itemp = ckt->CKTomega * DFnF12(inst->qb_x2,
                inst->qb_y2,
                inst->qb_z2,
                inst->qb_xy,
                inst->qb_yz,
                inst->qb_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1hm2x,
                i1hm2x,
                r1hm2y,
                i1hm2y,
                r1hm2z,
                i1hm2z);

                *(ckt->CKTrhs + inst->B1bNode) -= temp;
                *(ckt->CKTirhs + inst->B1bNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qb term over

                // loading qd term
                // kernels for vgs,vbs and vds already set up

                temp = -ckt->CKTomega * DFiF12(inst->qd_x2,
                inst->qd_y2,
                inst->qd_z2,
                inst->qd_xy,
                inst->qd_yz,
                inst->qd_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1hm2x,
                i1hm2x,
                r1hm2y,
                i1hm2y,
                r1hm2z,
                i1hm2z);

                itemp = ckt->CKTomega * DFnF12(inst->qd_x2,
                inst->qd_y2,
                inst->qd_z2,
                inst->qd_xy,
                inst->qd_yz,
                inst->qd_xz,
                r1h1x,
                i1h1x,
                r1h1y,
                i1h1y,
                r1h1z,
                i1h1z,
                r1hm2x,
                i1hm2x,
                r1hm2y,
                i1hm2y,
                r1hm2z,
                i1hm2z);

                *(ckt->CKTrhs + inst->B1dNodePrime) -= temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qd term over

                // loading inst->B1gbs term

                r1h1x = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1hm2x = *(job->r1H2ptr + inst->B1bNode) -
                    *(job->r1H2ptr + inst->B1sNodePrime);
                i1hm2x = -(*(job->i1H2ptr + inst->B1bNode) -
                    *(job->i1H2ptr + inst->B1sNodePrime));

                // now r1h1x = vbs

                temp = D1nF12(inst->gbs2,
                r1h1x,
                i1h1x,
                r1hm2x,
                i1hm2x);

                itemp = D1iF12(inst->gbs2,
                r1h1x,
                i1h1x,
                r1hm2x,
                i1hm2x);

                *(ckt->CKTrhs + inst->B1bNode) -= temp;
                *(ckt->CKTirhs + inst->B1bNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // inst->B1gbs term over

                // loading inst->B1gbd term

                r1h1x = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1dNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1dNodePrime);

                r1hm2x = *(job->r1H2ptr + inst->B1bNode) -
                    *(job->r1H2ptr + inst->B1dNodePrime);
                i1hm2x = -(*(job->i1H2ptr + inst->B1bNode) -
                    *(job->i1H2ptr + inst->B1dNodePrime));

                // now r1h1x = vbd

                temp = D1nF12(inst->gbd2,
                r1h1x,
                i1h1x,
                r1hm2x,
                i1hm2x);

                itemp = D1iF12(inst->gbd2,
                r1h1x,
                i1h1x,
                r1hm2x,
                i1hm2x);

                *(ckt->CKTrhs + inst->B1gNode) -= temp;
                *(ckt->CKTirhs + inst->B1gNode) -= itemp;

                *(ckt->CKTrhs + inst->B1dNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) += itemp;
                // inst->B1gbd term over

                // all done
              }
              break;

            case D_2F1MF2:
              // from now on, in the 3-var case, x=vgs,y=vbs,z=vds

              {
                // draincurrent term
                r1h1x = *(job->r1H1ptr + inst->B1gNode) - 
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1gNode) - 
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1h1y = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1y = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1h1z = *(job->r1H1ptr + inst->B1dNodePrime) -
                    *(job->r1H1ptr + inst->B1sNodePrime);

                i1h1z = *(job->i1H1ptr + inst->B1dNodePrime) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r1hm2x = *(job->r1H2ptr + inst->B1gNode) - 
                    *(job->r1H2ptr + inst->B1sNodePrime);
                i1hm2x = -(*(job->i1H2ptr + inst->B1gNode) - 
                    *(job->i1H2ptr + inst->B1sNodePrime));

                r1hm2y = *(job->r1H2ptr + inst->B1bNode) -
                    *(job->r1H2ptr + inst->B1sNodePrime);
                i1hm2y = -(*(job->i1H2ptr + inst->B1bNode) -
                    *(job->i1H2ptr + inst->B1sNodePrime));

                r1hm2z = *(job->r1H2ptr + inst->B1dNodePrime) -
                    *(job->r1H2ptr + inst->B1sNodePrime);

                i1hm2z = -(*(job->i1H2ptr + inst->B1dNodePrime) -
                    *(job->i1H2ptr + inst->B1sNodePrime));

                r2h11x = *(job->r2H11ptr + inst->B1gNode) - 
                    *(job->r2H11ptr + inst->B1sNodePrime);
                i2h11x = *(job->i2H11ptr + inst->B1gNode) - 
                    *(job->i2H11ptr + inst->B1sNodePrime);

                r2h11y = *(job->r2H11ptr + inst->B1bNode) -
                    *(job->r2H11ptr + inst->B1sNodePrime);
                i2h11y = *(job->i2H11ptr + inst->B1bNode) -
                    *(job->i2H11ptr + inst->B1sNodePrime);

                r2h11z = *(job->r2H11ptr + inst->B1dNodePrime) -
                    *(job->r2H11ptr + inst->B1sNodePrime);

                i2h11z = *(job->i2H11ptr + inst->B1dNodePrime) -
                    *(job->i2H11ptr + inst->B1sNodePrime);

                r2h1m2x = *(job->r2H1m2ptr + inst->B1gNode) - 
                    *(job->r2H1m2ptr + inst->B1sNodePrime);
                i2h1m2x = *(job->i2H1m2ptr + inst->B1gNode) - 
                    *(job->i2H1m2ptr + inst->B1sNodePrime);

                r2h1m2y = *(job->r2H1m2ptr + inst->B1bNode) -
                    *(job->r2H1m2ptr + inst->B1sNodePrime);
                i2h1m2y = *(job->i2H1m2ptr + inst->B1bNode) -
                    *(job->i2H1m2ptr + inst->B1sNodePrime);

                r2h1m2z = *(job->r2H1m2ptr + inst->B1dNodePrime) -
                    *(job->r2H1m2ptr + inst->B1sNodePrime);

                i2h1m2z = *(job->i2H1m2ptr + inst->B1dNodePrime) -
                    *(job->i2H1m2ptr + inst->B1sNodePrime);

                // draincurrent is a function of vgs,vbs,and vds;
                // have got their linear kernels; now to call
                // load functions 
                
                pass.cxx = inst->DrC_x2;
                pass.cyy = inst->DrC_y2;
                pass.czz = inst->DrC_z2;
                pass.cxy = inst->DrC_xy;
                pass.cyz = inst->DrC_yz;
                pass.cxz = inst->DrC_xz;
                pass.cxxx = inst->DrC_x3;
                pass.cyyy = inst->DrC_y3;
                pass.czzz = inst->DrC_z3;
                pass.cxxy = inst->DrC_x2y;
                pass.cxxz = inst->DrC_x2z;
                pass.cxyy = inst->DrC_xy2;
                pass.cyyz = inst->DrC_y2z;
                pass.cxzz = inst->DrC_xz2;
                pass.cyzz = inst->DrC_yz2;
                pass.cxyz = inst->DrC_xyz;
                pass.r1h1x = r1h1x;
                pass.i1h1x = i1h1x;
                pass.r1h1y = r1h1y;
                pass.i1h1y = i1h1y;
                pass.r1h1z = r1h1z;
                pass.i1h1z = i1h1z;
                pass.r1h2x = r1hm2x;
                pass.i1h2x = i1hm2x;
                pass.r1h2y = r1hm2y;
                pass.i1h2y = i1hm2y;
                pass.r1h2z = r1hm2z;
                pass.i1h2z = i1hm2z;
                pass.r2h11x = r2h11x;
                pass.i2h11x = i2h11x;
                pass.r2h11y = r2h11y;
                pass.i2h11y = i2h11y;
                pass.r2h11z = r2h11z;
                pass.i2h11z = i2h11z;
                pass.h2f1f2x = r2h1m2x;
                pass.ih2f1f2x = i2h1m2x;
                pass.h2f1f2y = r2h1m2y;
                pass.ih2f1f2y = i2h1m2y;
                pass.h2f1f2z = r2h1m2z;
                pass.ih2f1f2z = i2h1m2z;
                temp = DFn2F12(&pass);
                itemp = DFi2F12(&pass);

                *(ckt->CKTrhs + inst->B1dNodePrime) -= temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // draincurrent term loading over

                // loading qg term
                // kernels for vgs,vbs and vds already set up

                pass.cxx = inst->qg_x2;
                pass.cyy = inst->qg_y2;
                pass.czz = inst->qg_z2;
                pass.cxy = inst->qg_xy;
                pass.cyz = inst->qg_yz;
                pass.cxz = inst->qg_xz;
                pass.cxxx = inst->qg_x3;
                pass.cyyy = inst->qg_y3;
                pass.czzz = inst->qg_z3;
                pass.cxxy = inst->qg_x2y;
                pass.cxxz = inst->qg_x2z;
                pass.cxyy = inst->qg_xy2;
                pass.cyyz = inst->qg_y2z;
                pass.cxzz = inst->qg_xz2;
                pass.cyzz = inst->qg_yz2;
                pass.cxyz = inst->qg_xyz;
                pass.r1h1x = r1h1x;
                pass.i1h1x = i1h1x;
                pass.r1h1y = r1h1y;
                pass.i1h1y = i1h1y;
                pass.r1h1z = r1h1z;
                pass.i1h1z = i1h1z;
                pass.r1h2x = r1hm2x;
                pass.i1h2x = i1hm2x;
                pass.r1h2y = r1hm2y;
                pass.i1h2y = i1hm2y;
                pass.r1h2z = r1hm2z;
                pass.i1h2z = i1hm2z;
                pass.r2h11x = r2h11x;
                pass.i2h11x = i2h11x;
                pass.r2h11y = r2h11y;
                pass.i2h11y = i2h11y;
                pass.r2h11z = r2h11z;
                pass.i2h11z = i2h11z;
                pass.h2f1f2x = r2h1m2x;
                pass.ih2f1f2x = i2h1m2x;
                pass.h2f1f2y = r2h1m2y;
                pass.ih2f1f2y = i2h1m2y;
                pass.h2f1f2z = r2h1m2z;
                pass.ih2f1f2z = i2h1m2z;
                temp = -ckt->CKTomega * DFi2F12(&pass);

                itemp = ckt->CKTomega * DFn2F12(&pass);

                *(ckt->CKTrhs + inst->B1gNode) -= temp;
                *(ckt->CKTirhs + inst->B1gNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qg term over

                // loading qb term
                // kernels for vgs,vbs and vds already set up

                pass.cxx = inst->qb_x2;
                pass.cyy = inst->qb_y2;
                pass.czz = inst->qb_z2;
                pass.cxy = inst->qb_xy;
                pass.cyz = inst->qb_yz;
                pass.cxz = inst->qb_xz;
                pass.cxxx = inst->qb_x3;
                pass.cyyy = inst->qb_y3;
                pass.czzz = inst->qb_z3;
                pass.cxxy = inst->qb_x2y;
                pass.cxxz = inst->qb_x2z;
                pass.cxyy = inst->qb_xy2;
                pass.cyyz = inst->qb_y2z;
                pass.cxzz = inst->qb_xz2;
                pass.cyzz = inst->qb_yz2;
                pass.cxyz = inst->qb_xyz;
                pass.r1h1x = r1h1x;
                pass.i1h1x = i1h1x;
                pass.r1h1y = r1h1y;
                pass.i1h1y = i1h1y;
                pass.r1h1z = r1h1z;
                pass.i1h1z = i1h1z;
                pass.r1h2x = r1hm2x;
                pass.i1h2x = i1hm2x;
                pass.r1h2y = r1hm2y;
                pass.i1h2y = i1hm2y;
                pass.r1h2z = r1hm2z;
                pass.i1h2z = i1hm2z;
                pass.r2h11x = r2h11x;
                pass.i2h11x = i2h11x;
                pass.r2h11y = r2h11y;
                pass.i2h11y = i2h11y;
                pass.r2h11z = r2h11z;
                pass.i2h11z = i2h11z;
                pass.h2f1f2x = r2h1m2x;
                pass.ih2f1f2x = i2h1m2x;
                pass.h2f1f2y = r2h1m2y;
                pass.ih2f1f2y = i2h1m2y;
                pass.h2f1f2z = r2h1m2z;
                pass.ih2f1f2z = i2h1m2z;
                temp = -ckt->CKTomega * DFi2F12(&pass);

                itemp = ckt->CKTomega * DFn2F12(&pass);

                *(ckt->CKTrhs + inst->B1bNode) -= temp;
                *(ckt->CKTirhs + inst->B1bNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qb term over

                // loading qd term
                // kernels for vgs,vbs and vds already set up

                pass.cxx = inst->qd_x2;
                pass.cyy = inst->qd_y2;
                pass.czz = inst->qd_z2;
                pass.cxy = inst->qd_xy;
                pass.cyz = inst->qd_yz;
                pass.cxz = inst->qd_xz;
                pass.cxxx = inst->qd_x3;
                pass.cyyy = inst->qd_y3;
                pass.czzz = inst->qd_z3;
                pass.cxxy = inst->qd_x2y;
                pass.cxxz = inst->qd_x2z;
                pass.cxyy = inst->qd_xy2;
                pass.cyyz = inst->qd_y2z;
                pass.cxzz = inst->qd_xz2;
                pass.cyzz = inst->qd_yz2;
                pass.cxyz = inst->qd_xyz;
                pass.r1h1x = r1h1x;
                pass.i1h1x = i1h1x;
                pass.r1h1y = r1h1y;
                pass.i1h1y = i1h1y;
                pass.r1h1z = r1h1z;
                pass.i1h1z = i1h1z;
                pass.r1h2x = r1hm2x;
                pass.i1h2x = i1hm2x;
                pass.r1h2y = r1hm2y;
                pass.i1h2y = i1hm2y;
                pass.r1h2z = r1hm2z;
                pass.i1h2z = i1hm2z;
                pass.r2h11x = r2h11x;
                pass.i2h11x = i2h11x;
                pass.r2h11y = r2h11y;
                pass.i2h11y = i2h11y;
                pass.r2h11z = r2h11z;
                pass.i2h11z = i2h11z;
                pass.h2f1f2x = r2h1m2x;
                pass.ih2f1f2x = i2h1m2x;
                pass.h2f1f2y = r2h1m2y;
                pass.ih2f1f2y = i2h1m2y;
                pass.h2f1f2z = r2h1m2z;
                pass.ih2f1f2z = i2h1m2z;
                temp = -ckt->CKTomega * DFi2F12(&pass);

                itemp = ckt->CKTomega * DFn2F12(&pass);

                *(ckt->CKTrhs + inst->B1dNodePrime) -= temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // qd term over

                // loading inst->B1gbs term

                r1h1x = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1sNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1sNodePrime);

                r2h11x = *(job->r2H11ptr + inst->B1bNode) -
                    *(job->r2H11ptr + inst->B1sNodePrime);
                i2h11x = *(job->i2H11ptr + inst->B1bNode) -
                    *(job->i2H11ptr + inst->B1sNodePrime);

                r1hm2x = *(job->r1H2ptr + inst->B1bNode) -
                    *(job->r1H2ptr + inst->B1sNodePrime);
                i1hm2x = -(*(job->i1H2ptr + inst->B1bNode) -
                    *(job->i1H2ptr + inst->B1sNodePrime));

                r2h1m2x = *(job->r2H1m2ptr + inst->B1bNode) -
                    *(job->r2H1m2ptr + inst->B1sNodePrime);
                i2h1m2x = *(job->i2H1m2ptr + inst->B1bNode) -
                    *(job->i2H1m2ptr + inst->B1sNodePrime);

                // now r1h1x = vbs

                temp = D1n2F12(inst->gbs2,
                inst->gbs3,
                r1h1x,
                i1h1x,
                r1hm2x,
                i1hm2x,
                r2h11x,
                i2h11x,
                r2h1m2x,
                i2h1m2x);

                itemp = D1i2F12(inst->gbs2,
                inst->gbs3,
                r1h1x,
                i1h1x,
                r1hm2x,
                i1hm2x,
                r2h11x,
                i2h11x,
                r2h1m2x,
                i2h1m2x);

                *(ckt->CKTrhs + inst->B1bNode) -= temp;
                *(ckt->CKTirhs + inst->B1bNode) -= itemp;

                *(ckt->CKTrhs + inst->B1sNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1sNodePrime) += itemp;
                // nst->B1gbs term over

                // loading inst->B1gbd term

                r1h1x = *(job->r1H1ptr + inst->B1bNode) -
                    *(job->r1H1ptr + inst->B1dNodePrime);
                i1h1x = *(job->i1H1ptr + inst->B1bNode) -
                    *(job->i1H1ptr + inst->B1dNodePrime);

                r2h11x = *(job->r2H11ptr + inst->B1bNode) -
                    *(job->r2H11ptr + inst->B1dNodePrime);
                i2h11x = *(job->i2H11ptr + inst->B1bNode) -
                    *(job->i2H11ptr + inst->B1dNodePrime);

                r1hm2x = *(job->r1H2ptr + inst->B1bNode) -
                    *(job->r1H2ptr + inst->B1dNodePrime);
                i1hm2x = -(*(job->i1H2ptr + inst->B1bNode) -
                    *(job->i1H2ptr + inst->B1dNodePrime));

                r2h1m2x = *(job->r2H1m2ptr + inst->B1bNode) -
                    *(job->r2H1m2ptr + inst->B1dNodePrime);
                i2h1m2x = *(job->i2H1m2ptr + inst->B1bNode) -
                    *(job->i2H1m2ptr + inst->B1dNodePrime);

                // now r1h1x = vbd

                temp = D1n2F12(inst->gbd2,
                inst->gbd3,
                r1h1x,
                i1h1x,
                r1hm2x,
                i1hm2x,
                r2h11x,
                i2h11x,
                r2h1m2x,
                i2h1m2x);

                itemp = D1i2F12(inst->gbd2,
                inst->gbd3,
                r1h1x,
                i1h1x,
                r1hm2x,
                i1hm2x,
                r2h11x,
                i2h11x,
                r2h1m2x,
                i2h1m2x);

                *(ckt->CKTrhs + inst->B1bNode) -= temp;
                *(ckt->CKTirhs + inst->B1bNode) -= itemp;

                *(ckt->CKTrhs + inst->B1dNodePrime) += temp;
                *(ckt->CKTirhs + inst->B1dNodePrime) += itemp;
                // inst->B1gbd term over

                // all done
              }
              break;

            default:
                ;
            }
          }
        }
        return (OK);
    }
    else
      return (E_BADPARM);
}
