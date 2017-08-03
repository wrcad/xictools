
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
#include "bjtdefs.h"
#include "distdefs.h"

struct bjt_d {
    double r1h1x, i1h1x;
    double r1h1y, i1h1y;
    double r1h1z, i1h1z;
    double r1h2x, i1h2x;
    double r1h2y, i1h2y;
    double r1h2z, i1h2z;
    double r1hm2x, i1hm2x;
    double r1hm2y, i1hm2y;
    double r1hm2z, i1hm2z;
    double r2h11x, i2h11x;
    double r2h11y, i2h11y;
    double r2h11z, i2h11z;
    double r2h1m2x, i2h1m2x;
    double r2h1m2y, i2h1m2y;
    double r2h1m2z, i2h1m2z;
};

extern int BJTdSetup(sBJTmodel*, sCKT*);

static void bjt_setdprms(sDISTOAN*, sBJTinstance*, struct bjt_d*,
    double, int);
static void bjt_dload(sCKT*, sDISTOAN*, sBJTmodel*, sBJTinstance*,
    struct bjt_d*, int);


// assuming here that ckt->CKTomega has been initialised to 
// the correct value
//
int
BJTdev::disto(int mode, sGENmodel *genmod, sCKT *ckt)
{
    sBJTmodel *model = (sBJTmodel *) genmod;
    sBJTinstance *inst;
    sGENinstance *geninst;
    sDISTOAN* job = (sDISTOAN*) ckt->CKTcurJob;
    struct bjt_d d = bjt_d();
    double td;

    if (mode == D_SETUP)
        return (dSetup(model, ckt));

    if ((mode == D_TWOF1) || (mode == D_THRF1) || 
            (mode == D_F1PF2) || (mode == D_F1MF2) ||
            (mode == D_2F1MF2)) {

        for ( ; genmod != 0; genmod = genmod->GENnextModel) {
            model = (sBJTmodel*)genmod;
            td = model->BJTexcessPhaseFactor;
            for (geninst = genmod->GENinstances; geninst != 0; 
                    geninst = geninst->GENnextInstance) {
                inst = (sBJTinstance*)geninst;

                bjt_setdprms(job, inst, &d, td, mode);
                bjt_dload(ckt, job, model, inst, &d, mode);
            }
        }
        return (OK);
    }
    return (E_BADPARM);
}


static void
bjt_setdprms(sDISTOAN *job, sBJTinstance *inst, bjt_d *d, double td, int mode)
{
    double temp;

    // getting Volterra kernels
    // until further notice x = vbe, y = vbc, z = vbed

    d->r1h1x = *(job->r1H1ptr + (inst->BJTbasePrimeNode)) -
        *(job->r1H1ptr + (inst->BJTemitPrimeNode));
    d->i1h1x = *(job->i1H1ptr + (inst->BJTbasePrimeNode)) -
        *(job->i1H1ptr + (inst->BJTemitPrimeNode));

    d->r1h1y = *(job->r1H1ptr + (inst->BJTbasePrimeNode)) -
        *(job->r1H1ptr + (inst->BJTcolPrimeNode));
    d->i1h1y = *(job->i1H1ptr + (inst->BJTbasePrimeNode)) -
        *(job->i1H1ptr + (inst->BJTcolPrimeNode));

    if (td != 0) {
        temp = job->Domega1 * td;
        // multiplying r1h1x by exp(-j omega td)
        d->r1h1z = d->r1h1x*cos(temp) + d->i1h1x*sin(temp);
        d->i1h1z = d->i1h1x*cos(temp) - d->r1h1x*sin(temp);
    }
    else {
        d->r1h1z = d->r1h1x;
        d->i1h1z = d->i1h1x;
    }

    if ((mode == D_F1MF2) || (mode == D_2F1MF2)) {

        d->r1hm2x = *(job->r1H2ptr + (inst->BJTbasePrimeNode)) -
            *(job->r1H2ptr + (inst->BJTemitPrimeNode));
        d->i1hm2x = -(*(job->i1H2ptr + (inst->BJTbasePrimeNode)) -
            *(job->i1H2ptr + (inst->BJTemitPrimeNode)));

        d->r1hm2y = *(job->r1H2ptr + (inst->BJTbasePrimeNode)) -
            *(job->r1H2ptr + (inst->BJTcolPrimeNode));
        d->i1hm2y = -(*(job->i1H2ptr + (inst->BJTbasePrimeNode)) -
            *(job->i1H2ptr + (inst->BJTcolPrimeNode)));

        if (td != 0) {
            temp = -job->Domega2 * td;
            d->r1hm2z = d->r1hm2x*cos(temp) + d->i1hm2x*sin(temp);
            d->i1hm2z = d->i1hm2x*cos(temp) - d->r1hm2x*sin(temp);
        }
        else {
            d->r1hm2z = d->r1hm2x;
            d->i1hm2z = d->i1hm2x;
        }
    }
    if ((mode == D_THRF1) || (mode == D_2F1MF2)) {

        d->r2h11x = *(job->r2H11ptr + (inst->BJTbasePrimeNode)) -
            *(job->r2H11ptr + (inst->BJTemitPrimeNode));
        d->i2h11x = *(job->i2H11ptr + (inst->BJTbasePrimeNode)) -
            *(job->i2H11ptr + (inst->BJTemitPrimeNode));

        d->r2h11y = *(job->r2H11ptr + (inst->BJTbasePrimeNode)) -
            *(job->r2H11ptr + (inst->BJTcolPrimeNode));
        d->i2h11y = *(job->i2H11ptr + (inst->BJTbasePrimeNode)) -
            *(job->i2H11ptr + (inst->BJTcolPrimeNode));

        if (td != 0) {
            temp = 2*job->Domega1* td ;
            d->r2h11z = d->r2h11x*cos(temp) + d->i2h11x*sin(temp);
            d->i2h11z = d->i2h11x*cos(temp) - d->r2h11x*sin(temp);
        }
        else {
            d->r2h11z = d->r2h11x;
            d->i2h11z = d->i2h11x;
        }
    }
    if (mode == D_2F1MF2) {

        d->r2h1m2x = *(job->r2H1m2ptr + (inst->BJTbasePrimeNode)) -
            *(job->r2H1m2ptr + (inst->BJTemitPrimeNode));
        d->i2h1m2x = *(job->i2H1m2ptr + (inst->BJTbasePrimeNode)) -
            *(job->i2H1m2ptr + (inst->BJTemitPrimeNode));

        d->r2h1m2y = *(job->r2H1m2ptr + (inst->BJTbasePrimeNode)) -
            *(job->r2H1m2ptr + (inst->BJTcolPrimeNode));
        d->i2h1m2y = *(job->i2H1m2ptr + (inst->BJTbasePrimeNode)) -
            *(job->i2H1m2ptr + (inst->BJTcolPrimeNode));

        if (td != 0) {
            temp = (job->Domega1 - job->Domega2) * td;
            d->r2h1m2z = d->r2h1m2x*cos(temp) + d->i2h1m2x*sin(temp);
            d->i2h1m2z = d->i2h1m2x*cos(temp) - d->r2h1m2x*sin(temp);
        }
        else {
            d->r2h1m2z = d->r2h1m2x;
            d->i2h1m2z = d->i2h1m2x;
        }
    }
    if (mode == D_F1PF2) {

        d->r1h2x = *(job->r1H2ptr + (inst->BJTbasePrimeNode)) -
            *(job->r1H2ptr + (inst->BJTemitPrimeNode));
        d->i1h2x = *(job->i1H2ptr + (inst->BJTbasePrimeNode)) -
            *(job->i1H2ptr + (inst->BJTemitPrimeNode));

        d->r1h2y = *(job->r1H2ptr + (inst->BJTbasePrimeNode)) -
            *(job->r1H2ptr + (inst->BJTcolPrimeNode));
        d->i1h2y = *(job->i1H2ptr + (inst->BJTbasePrimeNode)) -
            *(job->i1H2ptr + (inst->BJTcolPrimeNode));

        if (td != 0) {
            temp = job->Domega2 * td;
            d->r1h2z = d->r1h2x*cos(temp) + d->i1h2x*sin(temp);
            d->i1h2z = d->i1h2x*cos(temp) - d->r1h2x*sin(temp);
        }
        else {
            d->r1h2z = d->r1h2x;
            d->i1h2z = d->i1h2x;
        }
    }
}


static void
bjt_dload(sCKT *ckt, sDISTOAN *job, sBJTmodel *model, sBJTinstance *inst,
    bjt_d *d, int mode)
{
    double temp, itemp;
    DpassStr pass;
#ifdef DISTODEBUG
    double time;
#endif

    if (mode == D_TWOF1) {
        // ic term

#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds();
#endif
        temp = DFn2F1( inst->ic_x2,
        inst->ic_y2,
        inst->ic_w2,
        inst->ic_xy,
        inst->ic_yw,
        inst->ic_xw,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        d->r1h1z,
        d->i1h1z);

        itemp = DFi2F1( inst->ic_x2,
        inst->ic_y2,
        inst->ic_w2,
        inst->ic_xy,
        inst->ic_yw,
        inst->ic_xw,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        d->r1h1z,
        d->i1h1z);
#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds() - time;
printf("Time for DFn2F1: %g seconds \n", time);
#endif

        *(ckt->CKTrhs + inst->BJTcolPrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // finish ic term
        // loading ib term
        // x and y still the same
        temp = DFn2F1( inst->ib_x2,
        inst->ib_y2,
        0.0,
        inst->ib_xy,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0);

        itemp = DFi2F1( inst->ib_x2,
        inst->ib_y2,
        0.0,
        inst->ib_xy,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0);

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // ib term over
        // loading ibb term
        // now x = vbe, y = vbc, z = vbb
        if ( !((model->BJTminBaseResist == 0.0) &&
                (model->BJTbaseResist == model->BJTminBaseResist))) {

            d->r1h1z = *(job->r1H1ptr + (inst->BJTbaseNode)) -
                *(job->r1H1ptr + (inst->BJTbasePrimeNode));
            d->i1h1z = *(job->i1H1ptr + (inst->BJTbaseNode)) -
                *(job->i1H1ptr + (inst->BJTbasePrimeNode));

            temp = DFn2F1( inst->ibb_x2,
            inst->ibb_y2,
            inst->ibb_z2,
            inst->ibb_xy,
            inst->ibb_yz,
            inst->ibb_xz,
            d->r1h1x,
            d->i1h1x,
            d->r1h1y,
            d->i1h1y,
            d->r1h1z,
            d->i1h1z);

            itemp = DFi2F1( inst->ibb_x2,
            inst->ibb_y2,
            inst->ibb_z2,
            inst->ibb_xy,
            inst->ibb_yz,
            inst->ibb_xz,
            d->r1h1x,
            d->i1h1x,
            d->r1h1y,
            d->i1h1y,
            d->r1h1z,
            d->i1h1z);

            *(ckt->CKTrhs + inst->BJTbaseNode) -= temp;
            *(ckt->CKTirhs + inst->BJTbaseNode) -= itemp;
            *(ckt->CKTrhs + inst->BJTbasePrimeNode) += temp;
            *(ckt->CKTirhs + inst->BJTbasePrimeNode) += itemp;
        }

        // ibb term over
        // loading qbe term
        // x = vbe, y = vbc, z not used
        // (have to multiply by j omega for charge storage 
        // elements to get the current)

        temp = - ckt->CKTomega*DFi2F1( inst->qbe_x2,
        inst->qbe_y2,
        0.0,
        inst->qbe_xy,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0);

        itemp = ckt->CKTomega*DFn2F1( inst->qbe_x2,
        inst->qbe_y2,
        0.0,
        inst->qbe_xy,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0);

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // qbe term over
        // loading qbx term
        // z = vbx= vb - vcPrime

        d->r1h1z = d->r1h1z + d->r1h1y;
        d->i1h1z = d->i1h1z + d->i1h1y;
#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds();
#endif
        temp = - ckt->CKTomega*D1i2F1(inst->capbx2,
        d->r1h1z,
        d->i1h1z);
        itemp = ckt->CKTomega*D1n2F1(inst->capbx2,
        d->r1h1z,
        d->i1h1z);
#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds() - time;
printf("Time for D1n2F1: %g seconds \n", time);
#endif

        *(ckt->CKTrhs + inst->BJTbaseNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbaseNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qbx term over

        // loading qbc term

        temp = - ckt->CKTomega*D1i2F1(inst->capbc2,
        d->r1h1y,
        d->i1h1y);
        itemp = ckt->CKTomega*D1n2F1(inst->capbc2,
        d->r1h1y,
        d->i1h1y);

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qbc term over

        // loading qsc term
        // z = vsc

        d->r1h1z = *(job->r1H1ptr + (inst->BJTsubstNode)) -
            *(job->r1H1ptr + (inst->BJTcolPrimeNode));
        d->i1h1z = *(job->i1H1ptr + (inst->BJTsubstNode)) -
            *(job->i1H1ptr + (inst->BJTcolPrimeNode));

        temp = - ckt->CKTomega*D1i2F1(inst->capsc2,
        d->r1h1z,
        d->i1h1z);
        itemp = ckt->CKTomega*D1n2F1(inst->capsc2,
        d->r1h1z,
        d->i1h1z);

        *(ckt->CKTrhs + inst->BJTsubstNode) -= temp;
        *(ckt->CKTirhs + inst->BJTsubstNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qsc term over
    }
    else if (mode == D_THRF1) {
        // ic term

#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds();
#endif
        temp = DFn3F1( inst->ic_x2,
        inst->ic_y2,
        inst->ic_w2,
        inst->ic_xy,
        inst->ic_yw,
        inst->ic_xw,
        inst->ic_x3,
        inst->ic_y3,
        inst->ic_w3,
        inst->ic_x2y,
        inst->ic_x2w,
        inst->ic_xy2,
        inst->ic_y2w,
        inst->ic_xw2,
        inst->ic_yw2,
        inst->ic_xyw,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        d->r1h1z,
        d->i1h1z,
        d->r2h11x,
        d->i2h11x,
        d->r2h11y,
        d->i2h11y,
        d->r2h11z,
        d->i2h11z);

        itemp = DFi3F1( inst->ic_x2,
        inst->ic_y2,
        inst->ic_w2,
        inst->ic_xy,
        inst->ic_yw,
        inst->ic_xw,
        inst->ic_x3,
        inst->ic_y3,
        inst->ic_w3,
        inst->ic_x2y,
        inst->ic_x2w,
        inst->ic_xy2,
        inst->ic_y2w,
        inst->ic_xw2,
        inst->ic_yw2,
        inst->ic_xyw,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        d->r1h1z,
        d->i1h1z,
        d->r2h11x,
        d->i2h11x,
        d->r2h11y,
        d->i2h11y,
        d->r2h11z,
        d->i2h11z);
#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds() - time;
printf("Time for DFn3F1: %g seconds \n", time);
#endif

        *(ckt->CKTrhs + inst->BJTcolPrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // finish ic term
        // loading ib term
        // x and y still the same
        temp = DFn3F1( inst->ib_x2,
        inst->ib_y2,
        0.0,
        inst->ib_xy,
        0.0,
        0.0,
        inst->ib_x3,
        inst->ib_y3,
        0.0,
        inst->ib_x2y,
        0.0,
        inst->ib_xy2,
        0.0,
        0.0,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0,
        d->r2h11x,
        d->i2h11x,
        d->r2h11y,
        d->i2h11y,
        0.0,
        0.0);

        itemp = DFi3F1( inst->ib_x2,
        inst->ib_y2,
        0.0,
        inst->ib_xy,
        0.0,
        0.0,
        inst->ib_x3,
        inst->ib_y3,
        0.0,
        inst->ib_x2y,
        0.0,
        inst->ib_xy2,
        0.0,
        0.0,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0,
        d->r2h11x,
        d->i2h11x,
        d->r2h11y,
        d->i2h11y,
        0.0,
        0.0);

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // ib term over
        // loading ibb term
        if ( !((model->BJTminBaseResist == 0.0) &&
                (model->BJTbaseResist == model->BJTminBaseResist))) {

            // now x = vbe, y = vbc, z = vbb
            d->r1h1z = *(job->r1H1ptr + (inst->BJTbaseNode)) -
                *(job->r1H1ptr + (inst->BJTbasePrimeNode));
            d->i1h1z = *(job->i1H1ptr + (inst->BJTbaseNode)) -
                *(job->i1H1ptr + (inst->BJTbasePrimeNode));

            d->r2h11z = *(job->r2H11ptr + (inst->BJTbaseNode)) -
                *(job->r2H11ptr + (inst->BJTbasePrimeNode));
            d->i2h11z = *(job->i2H11ptr + (inst->BJTbaseNode)) -
                *(job->i2H11ptr + (inst->BJTbasePrimeNode));

            temp = DFn3F1( inst->ibb_x2,
            inst->ibb_y2,
            inst->ibb_z2,
            inst->ibb_xy,
            inst->ibb_yz,
            inst->ibb_xz,
            inst->ibb_x3,
            inst->ibb_y3,
            inst->ibb_z3,
            inst->ibb_x2y,
            inst->ibb_x2z,
            inst->ibb_xy2,
            inst->ibb_y2z,
            inst->ibb_xz2,
            inst->ibb_yz2,
            inst->ibb_xyz,
            d->r1h1x,
            d->i1h1x,
            d->r1h1y,
            d->i1h1y,
            d->r1h1z,
            d->i1h1z,
            d->r2h11x,
            d->i2h11x,
            d->r2h11y,
            d->i2h11y,
            d->r2h11z,
            d->i2h11z);

            itemp = DFi3F1( inst->ibb_x2,
            inst->ibb_y2,
            inst->ibb_z2,
            inst->ibb_xy,
            inst->ibb_yz,
            inst->ibb_xz,
            inst->ibb_x3,
            inst->ibb_y3,
            inst->ibb_z3,
            inst->ibb_x2y,
            inst->ibb_x2z,
            inst->ibb_xy2,
            inst->ibb_y2z,
            inst->ibb_xz2,
            inst->ibb_yz2,
            inst->ibb_xyz,
            d->r1h1x,
            d->i1h1x,
            d->r1h1y,
            d->i1h1y,
            d->r1h1z,
            d->i1h1z,
            d->r2h11x,
            d->i2h11x,
            d->r2h11y,
            d->i2h11y,
            d->r2h11z,
            d->i2h11z);

            *(ckt->CKTrhs + inst->BJTbaseNode) -= temp;
            *(ckt->CKTirhs + inst->BJTbaseNode) -= itemp;
            *(ckt->CKTrhs + inst->BJTbasePrimeNode) += temp;
            *(ckt->CKTirhs + inst->BJTbasePrimeNode) += itemp;
        }
        // ibb term over

        // loading qbe term
        // x = vbe, y = vbc, z not used
        // (have to multiply by j omega for charge storage 
        // elements to get the current)

        temp = - ckt->CKTomega*DFi3F1( inst->qbe_x2,
        inst->qbe_y2,
        0.0,
        inst->qbe_xy,
        0.0,
        0.0,
        inst->qbe_x3,
        inst->qbe_y3,
        0.0,
        inst->qbe_x2y,
        0.0,
        inst->qbe_xy2,
        0.0,
        0.0,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0,
        d->r2h11x,
        d->i2h11x,
        d->r2h11y,
        d->i2h11y,
        0.0,
        0.0);

        itemp = ckt->CKTomega*DFn3F1( inst->qbe_x2,
        inst->qbe_y2,
        0.0,
        inst->qbe_xy,
        0.0,
        0.0,
        inst->qbe_x3,
        inst->qbe_y3,
        0.0,
        inst->qbe_x2y,
        0.0,
        inst->qbe_xy2,
        0.0,
        0.0,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0,
        d->r2h11x,
        d->i2h11x,
        d->r2h11y,
        d->i2h11y,
        0.0,
        0.0);

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // qbe term over
        // loading qbx term
        // z = vbx = vb - vcPrime

        d->r1h1z = d->r1h1z + d->r1h1y;
        d->i1h1z = d->i1h1z + d->i1h1y;
        d->r2h11z = d->r2h11z + d->r2h11y;
        d->i2h11z = d->i2h11z + d->i2h11y;
#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds();
#endif
        temp = - ckt->CKTomega*D1i3F1(inst->capbx2,
        inst->capbx3,
        d->r1h1z,
        d->i1h1z,
        d->r2h11z,
        d->i2h11z);
        itemp = ckt->CKTomega*D1n3F1(inst->capbx2,
        inst->capbx3,
        d->r1h1z,
        d->i1h1z,
        d->r2h11z,
        d->i2h11z);
#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds() - time;
printf("Time for D1n3F1: %g seconds \n", time);
#endif

        *(ckt->CKTrhs + inst->BJTbaseNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbaseNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qbx term over

        // loading qbc term

        temp = - ckt->CKTomega*D1i3F1(inst->capbc2,
        inst->capbc3,
        d->r1h1y,
        d->i1h1y,
        d->r2h11y,
        d->i2h11y);
        itemp = ckt->CKTomega*D1n3F1(inst->capbc2,
        inst->capbc3,
        d->r1h1y,
        d->i1h1y,
        d->r2h11y,
        d->i2h11y);

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qbc term over

        // loading qsc term
        // z = vsc

        d->r1h1z = *(job->r1H1ptr + (inst->BJTsubstNode)) -
            *(job->r1H1ptr + (inst->BJTcolPrimeNode));
        d->i1h1z = *(job->i1H1ptr + (inst->BJTsubstNode)) -
            *(job->i1H1ptr + (inst->BJTcolPrimeNode));

        d->r2h11z = *(job->r2H11ptr + (inst->BJTsubstNode)) -
            *(job->r2H11ptr + (inst->BJTcolPrimeNode));
        d->i2h11z = *(job->i2H11ptr + (inst->BJTsubstNode)) -
            *(job->i2H11ptr + (inst->BJTcolPrimeNode));

        temp = - ckt->CKTomega*D1i3F1(inst->capsc2,
        inst->capsc3,
        d->r1h1z,
        d->i1h1z,
        d->r2h11z,
        d->i2h11z);

        itemp = ckt->CKTomega*D1n3F1(inst->capsc2,
        inst->capsc3,
        d->r1h1z,
        d->i1h1z,
        d->r2h11z,
        d->i2h11z);

        *(ckt->CKTrhs + inst->BJTsubstNode) -= temp;
        *(ckt->CKTirhs + inst->BJTsubstNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qsc term over
    }
    else if (mode == D_F1PF2) {
        // ic term

        temp = DFnF12( inst->ic_x2,
        inst->ic_y2,
        inst->ic_w2,
        inst->ic_xy,
        inst->ic_yw,
        inst->ic_xw,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        d->r1h1z,
        d->i1h1z,
        d->r1h2x,
        d->i1h2x,
        d->r1h2y,
        d->i1h2y,
        d->r1h2z,
        d->i1h2z);

        itemp = DFiF12( inst->ic_x2,
        inst->ic_y2,
        inst->ic_w2,
        inst->ic_xy,
        inst->ic_yw,
        inst->ic_xw,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        d->r1h1z,
        d->i1h1z,
        d->r1h2x,
        d->i1h2x,
        d->r1h2y,
        d->i1h2y,
        d->r1h2z,
        d->i1h2z);

        *(ckt->CKTrhs + inst->BJTcolPrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // finish ic term
        // loading ib term
        // x and y still the same
#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds();
#endif
        temp = DFnF12( inst->ib_x2,
        inst->ib_y2,
        0.0,
        inst->ib_xy,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0,
        d->r1h2x,
        d->i1h2x,
        d->r1h2y,
        d->i1h2y,
        0.0,
        0.0);

        itemp = DFiF12( inst->ib_x2,
        inst->ib_y2,
        0.0,
        inst->ib_xy,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0,
        d->r1h2x,
        d->i1h2x,
        d->r1h2y,
        d->i1h2y,
        0.0,
        0.0);
#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds() - time;
printf("Time for DFnF12: %g seconds \n", time);
#endif

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // ib term over
        // loading ibb term
        if ( !((model->BJTminBaseResist == 0.0) &&
                (model->BJTbaseResist == model->BJTminBaseResist))) {

            /* now x = vbe, y = vbc, z = vbb */
            d->r1h1z = *(job->r1H1ptr + (inst->BJTbaseNode)) -
                *(job->r1H1ptr + (inst->BJTbasePrimeNode));
            d->i1h1z = *(job->i1H1ptr + (inst->BJTbaseNode)) -
                *(job->i1H1ptr + (inst->BJTbasePrimeNode));

            d->r1h2z = *(job->r1H2ptr + (inst->BJTbaseNode)) -
                *(job->r1H2ptr + (inst->BJTbasePrimeNode));
            d->i1h2z = *(job->i1H2ptr + (inst->BJTbaseNode)) -
                *(job->i1H2ptr + (inst->BJTbasePrimeNode));

            temp = DFnF12( inst->ibb_x2,
            inst->ibb_y2,
            inst->ibb_z2,
            inst->ibb_xy,
            inst->ibb_yz,
            inst->ibb_xz,
            d->r1h1x,
            d->i1h1x,
            d->r1h1y,
            d->i1h1y,
            d->r1h1z,
            d->i1h1z,
            d->r1h2x,
            d->i1h2x,
            d->r1h2y,
            d->i1h2y,
            d->r1h2z,
            d->i1h2z);

            itemp = DFiF12( inst->ibb_x2,
            inst->ibb_y2,
            inst->ibb_z2,
            inst->ibb_xy,
            inst->ibb_yz,
            inst->ibb_xz,
            d->r1h1x,
            d->i1h1x,
            d->r1h1y,
            d->i1h1y,
            d->r1h1z,
            d->i1h1z,
            d->r1h2x,
            d->i1h2x,
            d->r1h2y,
            d->i1h2y,
            d->r1h2z,
            d->i1h2z);

            *(ckt->CKTrhs + inst->BJTbaseNode) -= temp;
            *(ckt->CKTirhs + inst->BJTbaseNode) -= itemp;
            *(ckt->CKTrhs + inst->BJTbasePrimeNode) += temp;
            *(ckt->CKTirhs + inst->BJTbasePrimeNode) += itemp;

        }
        // ibb term over
        // loading qbe term
        // x = vbe, y = vbc, z not used
        // (have to multiply by j omega for charge storage 
        // elements - to get the current)

        temp = - ckt->CKTomega*DFiF12( inst->qbe_x2,
        inst->qbe_y2,
        0.0,
        inst->qbe_xy,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0,
        d->r1h2x,
        d->i1h2x,
        d->r1h2y,
        d->i1h2y,
        0.0,
        0.0);

        itemp = ckt->CKTomega*DFnF12( inst->qbe_x2,
        inst->qbe_y2,
        0.0,
        inst->qbe_xy,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0,
        d->r1h2x,
        d->i1h2x,
        d->r1h2y,
        d->i1h2y,
        0.0,
        0.0);

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // qbe term over
        // loading qbx term
        // z = vbx= vb - vcPrime

        d->r1h1z = d->r1h1z + d->r1h1y;
        d->i1h1z = d->i1h1z + d->i1h1y;
        d->r1h2z = d->r1h2z + d->r1h2y;
        d->i1h2z = d->i1h2z + d->i1h2y;
#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds();
#endif
        temp = - ckt->CKTomega*D1iF12(inst->capbx2,
        d->r1h1z,
        d->i1h1z,
        d->r1h2z,
        d->i1h2z);
        itemp = ckt->CKTomega*D1nF12(inst->capbx2,
        d->r1h1z,
        d->i1h1z,
        d->r1h2z,
        d->i1h2z);
#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds() - time;
printf("Time for D1nF12: %g seconds \n", time);
#endif

        *(ckt->CKTrhs + inst->BJTbaseNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbaseNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qbx term over

        // loading qbc term

        temp = - ckt->CKTomega*D1iF12(inst->capbc2,
        d->r1h1y,
        d->i1h1y,
        d->r1h2y,
        d->i1h2y);
        itemp = ckt->CKTomega*D1nF12(inst->capbc2,
        d->r1h1y,
        d->i1h1y,
        d->r1h2y,
        d->i1h2y);

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qbc term over

        // loading qsc term
        // z = vsc

        d->r1h1z = *(job->r1H1ptr + (inst->BJTsubstNode)) -
            *(job->r1H1ptr + (inst->BJTcolPrimeNode));
        d->i1h1z = *(job->i1H1ptr + (inst->BJTsubstNode)) -
            *(job->i1H1ptr + (inst->BJTcolPrimeNode));
        d->r1h2z = *(job->r1H2ptr + (inst->BJTsubstNode)) -
            *(job->r1H2ptr + (inst->BJTcolPrimeNode));
        d->i1h2z = *(job->i1H2ptr + (inst->BJTsubstNode)) -
            *(job->i1H2ptr + (inst->BJTcolPrimeNode));

        temp = - ckt->CKTomega*D1iF12(inst->capsc2,
        d->r1h1z,
        d->i1h1z,
        d->r1h2z,
        d->i1h2z);
        itemp = ckt->CKTomega*D1nF12(inst->capsc2,
        d->r1h1z,
        d->i1h1z,
        d->r1h2z,
        d->i1h2z);

        *(ckt->CKTrhs + inst->BJTsubstNode) -= temp;
        *(ckt->CKTirhs + inst->BJTsubstNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qsc term over
    }
    else if (mode == D_F1MF2) {
        // ic term

        temp = DFnF12( inst->ic_x2,
        inst->ic_y2,
        inst->ic_w2,
        inst->ic_xy,
        inst->ic_yw,
        inst->ic_xw,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        d->r1h1z,
        d->i1h1z,
        d->r1hm2x,
        d->i1hm2x,
        d->r1hm2y,
        d->i1hm2y,
        d->r1hm2z,
        d->i1hm2z);

        itemp = DFiF12( inst->ic_x2,
        inst->ic_y2,
        inst->ic_w2,
        inst->ic_xy,
        inst->ic_yw,
        inst->ic_xw,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        d->r1h1z,
        d->i1h1z,
        d->r1hm2x,
        d->i1hm2x,
        d->r1hm2y,
        d->i1hm2y,
        d->r1hm2z,
        d->i1hm2z);

        *(ckt->CKTrhs + inst->BJTcolPrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // finish ic term
        // loading ib term
        // x and y still the same
        temp = DFnF12( inst->ib_x2,
        inst->ib_y2,
        0.0,
        inst->ib_xy,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0,
        d->r1hm2x,
        d->i1hm2x,
        d->r1hm2y,
        d->i1hm2y,
        0.0,
        0.0);

        itemp = DFiF12( inst->ib_x2,
        inst->ib_y2,
        0.0,
        inst->ib_xy,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0,
        d->r1hm2x,
        d->i1hm2x,
        d->r1hm2y,
        d->i1hm2y,
        0.0,
        0.0);

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // ib term over
        // loading ibb term
        if ( !((model->BJTminBaseResist == 0.0) &&
                (model->BJTbaseResist == model->BJTminBaseResist))) {

            // now x = vbe, y = vbc, z = vbb
            d->r1h1z = *(job->r1H1ptr + (inst->BJTbaseNode)) -
                *(job->r1H1ptr + (inst->BJTbasePrimeNode));
            d->i1h1z = *(job->i1H1ptr + (inst->BJTbaseNode)) -
                *(job->i1H1ptr + (inst->BJTbasePrimeNode));

            d->r1hm2z = *(job->r1H2ptr + (inst->BJTbaseNode)) -
                *(job->r1H2ptr + (inst->BJTbasePrimeNode));
            d->i1hm2z = *(job->i1H2ptr + (inst->BJTbaseNode)) -
                *(job->i1H2ptr + (inst->BJTbasePrimeNode));

            temp = DFnF12( inst->ibb_x2,
            inst->ibb_y2,
            inst->ibb_z2,
            inst->ibb_xy,
            inst->ibb_yz,
            inst->ibb_xz,
            d->r1h1x,
            d->i1h1x,
            d->r1h1y,
            d->i1h1y,
            d->r1h1z,
            d->i1h1z,
            d->r1hm2x,
            d->i1hm2x,
            d->r1hm2y,
            d->i1hm2y,
            d->r1hm2z,
            d->i1hm2z);

            itemp = DFiF12( inst->ibb_x2,
            inst->ibb_y2,
            inst->ibb_z2,
            inst->ibb_xy,
            inst->ibb_yz,
            inst->ibb_xz,
            d->r1h1x,
            d->i1h1x,
            d->r1h1y,
            d->i1h1y,
            d->r1h1z,
            d->i1h1z,
            d->r1hm2x,
            d->i1hm2x,
            d->r1hm2y,
            d->i1hm2y,
            d->r1hm2z,
            d->i1hm2z);

            *(ckt->CKTrhs + inst->BJTbaseNode) -= temp;
            *(ckt->CKTirhs + inst->BJTbaseNode) -= itemp;
            *(ckt->CKTrhs + inst->BJTbasePrimeNode) += temp;
            *(ckt->CKTirhs + inst->BJTbasePrimeNode) += itemp;
        }

        // ibb term over
        // loading qbe term
        // x = vbe, y = vbc, z not used
        // (have to multiply by j omega for charge storage 
        // elements - to get the current)

        temp = - ckt->CKTomega*DFiF12( inst->qbe_x2,
        inst->qbe_y2,
        0.0,
        inst->qbe_xy,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0,
        d->r1hm2x,
        d->i1hm2x,
        d->r1hm2y,
        d->i1hm2y,
        0.0,
        0.0);

        itemp = ckt->CKTomega*DFnF12( inst->qbe_x2,
        inst->qbe_y2,
        0.0,
        inst->qbe_xy,
        0.0,
        0.0,
        d->r1h1x,
        d->i1h1x,
        d->r1h1y,
        d->i1h1y,
        0.0,
        0.0,
        d->r1hm2x,
        d->i1hm2x,
        d->r1hm2y,
        d->i1hm2y,
        0.0,
        0.0);

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // qbe term over
        // loading qbx term
        // z = vbx= vb - vcPrime

        d->r1h1z = d->r1h1z + d->r1h1y;
        d->i1h1z = d->i1h1z + d->i1h1y;
        d->r1hm2z = d->r1hm2z + d->r1hm2y;
        d->i1hm2z = d->i1hm2z + d->i1hm2y;
        temp = - ckt->CKTomega*D1iF12(inst->capbx2,
        d->r1h1z,
        d->i1h1z,
        d->r1hm2z,
        d->i1hm2z);
        itemp = ckt->CKTomega*D1nF12(inst->capbx2,
        d->r1h1z,
        d->i1h1z,
        d->r1hm2z,
        d->i1hm2z);

        *(ckt->CKTrhs + inst->BJTbaseNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbaseNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qbx term over

        // loading qbc term

        temp = - ckt->CKTomega*D1iF12(inst->capbc2,
        d->r1h1y,
        d->i1h1y,
        d->r1hm2y,
        d->i1hm2y);
        itemp = ckt->CKTomega*D1nF12(inst->capbc2,
        d->r1h1y,
        d->i1h1y,
        d->r1hm2y,
        d->i1hm2y);

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qbc term over

        // loading qsc term
        // z = vsc

        d->r1h1z = *(job->r1H1ptr + (inst->BJTsubstNode)) -
            *(job->r1H1ptr + (inst->BJTcolPrimeNode));
        d->i1h1z = *(job->i1H1ptr + (inst->BJTsubstNode)) -
            *(job->i1H1ptr + (inst->BJTcolPrimeNode));
        d->r1hm2z = *(job->r1H2ptr + (inst->BJTsubstNode)) -
            *(job->r1H2ptr + (inst->BJTcolPrimeNode));
        d->i1hm2z = *(job->i1H2ptr + (inst->BJTsubstNode)) -
            *(job->i1H2ptr + (inst->BJTcolPrimeNode));

        temp = - ckt->CKTomega*D1iF12(inst->capsc2,
        d->r1h1z,
        d->i1h1z,
        d->r1hm2z,
        d->i1hm2z);
        itemp = ckt->CKTomega*D1nF12(inst->capsc2,
        d->r1h1z,
        d->i1h1z,
        d->r1hm2z,
        d->i1hm2z);

        *(ckt->CKTrhs + inst->BJTsubstNode) -= temp;
        *(ckt->CKTirhs + inst->BJTsubstNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qsc term over
    }
    else if (mode == D_2F1MF2) {
        // ic term

        {
        pass.cxx =   inst->ic_x2;
        pass.cyy =   inst->ic_y2;
        pass.czz =   inst->ic_w2;
        pass.cxy =   inst->ic_xy;
        pass.cyz =   inst->ic_yw;
        pass.cxz =   inst->ic_xw;
        pass.cxxx =   inst->ic_x3;
        pass.cyyy =   inst->ic_y3;
        pass.czzz =   inst->ic_w3;
        pass.cxxy =   inst->ic_x2y;
        pass.cxxz =   inst->ic_x2w;
        pass.cxyy =   inst->ic_xy2;
        pass.cyyz =   inst->ic_y2w;
        pass.cxzz =   inst->ic_xw2;
        pass.cyzz =   inst->ic_yw2;
        pass.cxyz =   inst->ic_xyw;
        pass.r1h1x =   d->r1h1x;
        pass.i1h1x =   d->i1h1x;
        pass.r1h1y =   d->r1h1y;
        pass.i1h1y =   d->i1h1y;
        pass.r1h1z =   d->r1h1z;
        pass.i1h1z =   d->i1h1z;
        pass.r1h2x =   d->r1hm2x;
        pass.i1h2x =   d->i1hm2x;
        pass.r1h2y =   d->r1hm2y;
        pass.i1h2y =   d->i1hm2y;
        pass.r1h2z =   d->r1hm2z;
        pass.i1h2z =   d->i1hm2z;
        pass.r2h11x =   d->r2h11x;
        pass.i2h11x =  d->i2h11x;
        pass.r2h11y =   d->r2h11y;
        pass.i2h11y =  d->i2h11y;
        pass.r2h11z =   d->r2h11z;
        pass.i2h11z =  d->i2h11z;
        pass.h2f1f2x =   d->r2h1m2x;
        pass.ih2f1f2x =  d->i2h1m2x;
        pass.h2f1f2y =   d->r2h1m2y;
        pass.ih2f1f2y =  d->i2h1m2y;
        pass.h2f1f2z =  d->r2h1m2z;
        pass.ih2f1f2z =   d->i2h1m2z;
#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds();
#endif
        temp = DFn2F12(&pass);

        itemp = DFi2F12(&pass);
#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds() - time;
printf("Time for DFn2F12: %g seconds \n", time);
#endif
        }

        *(ckt->CKTrhs + inst->BJTcolPrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // finish ic term
        // loading ib term
        // x and y still the same
        {
        pass.cxx = inst->ib_x2;
        pass.cyy = inst->ib_y2;
        pass.czz = 0.0;
        pass.cxy = inst->ib_xy;
        pass.cyz = 0.0;
        pass.cxz = 0.0;
        pass.cxxx = inst->ib_x3;
        pass.cyyy = inst->ib_y3;
        pass.czzz = 0.0;
        pass.cxxy = inst->ib_x2y;
        pass.cxxz = 0.0;
        pass.cxyy = inst->ib_xy2;
        pass.cyyz = 0.0;
        pass.cxzz = 0.0;
        pass.cyzz = 0.0;
        pass.cxyz = 0.0;
        pass.r1h1x = d->r1h1x;
        pass.i1h1x = d->i1h1x;
        pass.r1h1y = d->r1h1y;
        pass.i1h1y = d->i1h1y;
        pass.r1h1z = 0.0;
        pass.i1h1z = 0.0;
        pass.r1h2x = d->r1hm2x;
        pass.i1h2x = d->i1hm2x;
        pass.r1h2y = d->r1hm2y;
        pass.i1h2y = d->i1hm2y;
        pass.r1h2z = 0.0;
        pass.i1h2z = 0.0;
        pass.r2h11x = d->r2h11x;
        pass.i2h11x = d->i2h11x;
        pass.r2h11y = d->r2h11y;
        pass.i2h11y = d->i2h11y;
        pass.r2h11z = 0.0;
        pass.i2h11z = 0.0;
        pass.h2f1f2x = d->r2h1m2x;
        pass.ih2f1f2x = d->i2h1m2x;
        pass.h2f1f2y = d->r2h1m2y;
        pass.ih2f1f2y = d->i2h1m2y;
        pass.h2f1f2z = 0.0;
        pass.ih2f1f2z = 0.0;
        temp = DFn2F12(&pass);

        itemp = DFi2F12(&pass);
        }

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // ib term over
        // loading ibb term
        if ( !((model->BJTminBaseResist == 0.0) &&
                (model->BJTbaseResist == model->BJTminBaseResist))) {

            // now x = vbe, y = vbc, z = vbb
            d->r1h1z = *(job->r1H1ptr + (inst->BJTbaseNode)) -
                *(job->r1H1ptr + (inst->BJTbasePrimeNode));
            d->i1h1z = *(job->i1H1ptr + (inst->BJTbaseNode)) -
                *(job->i1H1ptr + (inst->BJTbasePrimeNode));

            d->r1hm2z = *(job->r1H2ptr + (inst->BJTbaseNode)) -
                *(job->r1H2ptr + (inst->BJTbasePrimeNode));
            d->i1hm2z = -(*(job->i1H2ptr + (inst->BJTbaseNode)) -
                *(job->i1H2ptr + (inst->BJTbasePrimeNode)));

            d->r2h11z = *(job->r2H11ptr + (inst->BJTbaseNode)) -
                *(job->r2H11ptr + (inst->BJTbasePrimeNode));
            d->i2h11z = *(job->i2H11ptr + (inst->BJTbaseNode)) -
                *(job->i2H11ptr + (inst->BJTbasePrimeNode));

            d->r2h1m2z = *(job->r2H1m2ptr + (inst->BJTbaseNode)) -
                *(job->r2H1m2ptr + (inst->BJTbasePrimeNode));
            d->i2h1m2z = *(job->i2H1m2ptr + (inst->BJTbaseNode)) -
                *(job->i2H1m2ptr + (inst->BJTbasePrimeNode));

            {
            pass.cxx = inst->ibb_x2;
            pass.cyy = inst->ibb_y2;
            pass.czz = inst->ibb_z2;
            pass.cxy = inst->ibb_xy;
            pass.cyz = inst->ibb_yz;
            pass.cxz = inst->ibb_xz;
            pass.cxxx = inst->ibb_x3;
            pass.cyyy = inst->ibb_y3;
            pass.czzz = inst->ibb_z3;
            pass.cxxy = inst->ibb_x2y;
            pass.cxxz = inst->ibb_x2z;
            pass.cxyy = inst->ibb_xy2;
            pass.cyyz = inst->ibb_y2z;
            pass.cxzz = inst->ibb_xz2;
            pass.cyzz = inst->ibb_yz2;
            pass.cxyz = inst->ibb_xyz;
            pass.r1h1x = d->r1h1x;
            pass.i1h1x = d->i1h1x;
            pass.r1h1y = d->r1h1y;
            pass.i1h1y = d->i1h1y;
            pass.r1h1z = d->r1h1z;
            pass.i1h1z = d->i1h1z;
            pass.r1h2x = d->r1hm2x;
            pass.i1h2x = d->i1hm2x;
            pass.r1h2y = d->r1hm2y;
            pass.i1h2y = d->i1hm2y;
            pass.r1h2z = d->r1hm2z;
            pass.i1h2z = d->i1hm2z;
            pass.r2h11x = d->r2h11x;
            pass.i2h11x = d->i2h11x;
            pass.r2h11y = d->r2h11y;
            pass.i2h11y = d->i2h11y;
            pass.r2h11z = d->r2h11z;
            pass.i2h11z = d->i2h11z;
            pass.h2f1f2x = d->r2h1m2x;
            pass.ih2f1f2x = d->i2h1m2x;
            pass.h2f1f2y = d->r2h1m2y;
            pass.ih2f1f2y = d->i2h1m2y;
            pass.h2f1f2z = d->r2h1m2z;
            pass.ih2f1f2z = d->i2h1m2z;
            temp = DFn2F12(&pass);
  
            itemp = DFi2F12(&pass);
            }

            *(ckt->CKTrhs + inst->BJTbaseNode) -= temp;
            *(ckt->CKTirhs + inst->BJTbaseNode) -= itemp;
            *(ckt->CKTrhs + inst->BJTbasePrimeNode) += temp;
            *(ckt->CKTirhs + inst->BJTbasePrimeNode) += itemp;

        }
        // ibb term over
        // loading qbe term
        // x = vbe, y = vbc, z not used
        // (have to multiply by j omega for charge storage 
        // elements to get the current)

        {
        pass.cxx = inst->qbe_x2;
        pass.cyy = inst->qbe_y2;
        pass.czz = 0.0;
        pass.cxy = inst->qbe_xy;
        pass.cyz = 0.0;
        pass.cxz = 0.0;
        pass.cxxx = inst->qbe_x3;
        pass.cyyy = inst->qbe_y3;
        pass.czzz = 0.0;
        pass.cxxy = inst->qbe_x2y;
        pass.cxxz = 0.0;
        pass.cxyy = inst->qbe_xy2;
        pass.cyyz = 0.0;
        pass.cxzz = 0.0;
        pass.cyzz = 0.0;
        pass.cxyz = 0.0;
        pass.r1h1x = d->r1h1x;
        pass.i1h1x = d->i1h1x;
        pass.r1h1y = d->r1h1y;
        pass.i1h1y = d->i1h1y;
        pass.r1h1z = 0.0;
        pass.i1h1z = 0.0;
        pass.r1h2x = d->r1hm2x;
        pass.i1h2x = d->i1hm2x;
        pass.r1h2y = d->r1hm2y;
        pass.i1h2y = d->i1hm2y;
        pass.r1h2z = 0.0;
        pass.i1h2z = 0.0;
        pass.r2h11x = d->r2h11x;
        pass.i2h11x = d->i2h11x;
        pass.r2h11y = d->r2h11y;
        pass.i2h11y = d->i2h11y;
        pass.r2h11z = 0.0;
        pass.i2h11z = 0.0;
        pass.h2f1f2x = d->r2h1m2x;
        pass.ih2f1f2x = d->i2h1m2x;
        pass.h2f1f2y = d->r2h1m2y;
        pass.ih2f1f2y = d->i2h1m2y;
        pass.h2f1f2z = 0.0;
        pass.ih2f1f2z = 0.0;
        temp = - ckt->CKTomega*DFi2F12(&pass);

        itemp = ckt->CKTomega*DFn2F12(&pass);
        }

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTemitPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTemitPrimeNode) += itemp;

        // qbe term over
        // loading qbx term
        // z = vbx= vb - vcPrime

        d->r1h1z = d->r1h1z + d->r1h1y;
        d->i1h1z = d->i1h1z + d->i1h1y;
        d->r1hm2z = d->r1hm2z + d->r1hm2y;
        d->i1hm2z = d->i1hm2z + d->i1hm2y;
        d->r2h11z = d->r2h11z + d->r2h11y;
        d->i2h11z = d->i2h11z + d->i2h11y;
        d->r2h1m2z = d->r2h1m2z + d->r2h1m2y;
        d->i2h1m2z = d->i2h1m2z + d->i2h1m2y;
#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds();
#endif
        temp = - ckt->CKTomega*D1i2F12(inst->capbx2,
        inst->capbx3,
        d->r1h1z,
        d->i1h1z,
        d->r1hm2z,
        d->i1hm2z,
        d->r2h11z,
        d->i2h11z,
        d->r2h1m2z,
        d->i2h1m2z);
        itemp = ckt->CKTomega*D1n2F12(inst->capbx2,
        inst->capbx3,
        d->r1h1z,
        d->i1h1z,
        d->r1hm2z,
        d->i1hm2z,
        d->r2h11z,
        d->i2h11z,
        d->r2h1m2z,
        d->i2h1m2z);
#ifdef D_DBG_SMALLTIMES
time = SPoutput.seconds() - time;
printf("Time for D1n2F12: %g seconds \n", time);
#endif

        *(ckt->CKTrhs + inst->BJTbaseNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbaseNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qbx term over

        // loading qbc term

        temp = - ckt->CKTomega*D1i2F12(inst->capbc2,
        inst->capbc3,
        d->r1h1y,
        d->i1h1y,
        d->r1hm2y,
        d->i1hm2y,
        d->r2h11y,
        d->i2h11y,
        d->r2h1m2y,
        d->i2h1m2y);
        itemp = ckt->CKTomega*D1n2F12(inst->capbc2,
        inst->capbc3,
        d->r1h1y,
        d->i1h1y,
        d->r1hm2y,
        d->i1hm2y,
        d->r2h11y,
        d->i2h11y,
        d->r2h1m2y,
        d->i2h1m2y);

        *(ckt->CKTrhs + inst->BJTbasePrimeNode) -= temp;
        *(ckt->CKTirhs + inst->BJTbasePrimeNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qbc term over

        // loading qsc term
        // z = vsc

        d->r1h1z = *(job->r1H1ptr + (inst->BJTsubstNode)) -
            *(job->r1H1ptr + (inst->BJTcolPrimeNode));
        d->i1h1z = *(job->i1H1ptr + (inst->BJTsubstNode)) -
            *(job->i1H1ptr + (inst->BJTcolPrimeNode));

        d->r1hm2z = *(job->r1H2ptr + (inst->BJTsubstNode)) -
            *(job->r1H2ptr + (inst->BJTcolPrimeNode));
        d->i1hm2z = -(*(job->i1H2ptr + (inst->BJTsubstNode)) -
            *(job->i1H2ptr + (inst->BJTcolPrimeNode)));

        d->r2h11z = *(job->r2H11ptr + (inst->BJTsubstNode)) -
            *(job->r2H11ptr + (inst->BJTcolPrimeNode));
        d->i2h11z = *(job->i2H11ptr + (inst->BJTsubstNode)) -
            *(job->i2H11ptr + (inst->BJTcolPrimeNode));

        d->r2h1m2z = *(job->r2H1m2ptr + (inst->BJTsubstNode)) -
            *(job->r2H1m2ptr + (inst->BJTcolPrimeNode));
        d->i2h1m2z = *(job->i2H1m2ptr + (inst->BJTsubstNode)) -
            *(job->i2H1m2ptr + (inst->BJTcolPrimeNode));

        temp = - ckt->CKTomega*D1i2F12(inst->capsc2,
        inst->capsc3,
        d->r1h1z,
        d->i1h1z,
        d->r1hm2z,
        d->i1hm2z,
        d->r2h11z,
        d->i2h11z,
        d->r2h1m2z,
        d->i2h1m2z);

        itemp = ckt->CKTomega*D1n2F12(inst->capsc2,
        inst->capsc3,
        d->r1h1z,
        d->i1h1z,
        d->r1hm2z,
        d->i1hm2z,
        d->r2h11z,
        d->i2h11z,
        d->r2h1m2z,
        d->i2h1m2z);

        *(ckt->CKTrhs + inst->BJTsubstNode) -= temp;
        *(ckt->CKTirhs + inst->BJTsubstNode) -= itemp;
        *(ckt->CKTrhs + inst->BJTcolPrimeNode) += temp;
        *(ckt->CKTirhs + inst->BJTcolPrimeNode) += itemp;

        // qsc term over
    }
}
