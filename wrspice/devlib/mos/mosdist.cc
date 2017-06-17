
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
 $Id: mosdist.cc,v 1.0 1998/01/30 05:32:17 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1988 Jaijeet S Roychowdhury
         1993 Stephen R. Whiteley
****************************************************************************/

#define DISTO
#include "mosdefs.h"
#include "distdefs.h"

extern int MOSdSetup(sMOSmodel*, sCKT*);


int
MOSdev::disto(int mode, sGENmodel *genmod, sCKT *ckt)
{
    sDISTOAN* job = static_cast<sDISTOAN*>(ckt->CKTcurJob);
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

    sMOSmodel *model = static_cast<sMOSmodel*>(genmod);
    if (mode == D_SETUP)
        return (dSetup(model, ckt));

if ((mode == D_TWOF1) || (mode == D_THRF1) || 
 (mode == D_F1PF2) || (mode == D_F1MF2) ||
 (mode == D_2F1MF2)) {

    for ( ; model; model = model->next()) {
        sMOSinstance *inst;
        for (inst = model->inst(); inst; inst = inst->next()) {

    // loading starts here

    switch (mode) {
    case D_TWOF1:
    // x = vgs, y = vbs z = vds

        // getting first order (linear) Volterra kernel
        r1h1x = *(job->r1H1ptr + (inst->MOSgNode)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1x = *(job->i1H1ptr + (inst->MOSgNode)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r1h1y = *(job->r1H1ptr + (inst->MOSbNode)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1y = *(job->i1H1ptr + (inst->MOSbNode)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r1h1z = *(job->r1H1ptr + (inst->MOSdNodePrime)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1z = *(job->i1H1ptr + (inst->MOSdNodePrime)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        // loading starts here
        // loading cdrain term

        temp = DFn2F1(inst->cdr_x2,
                    inst->cdr_y2,
                    inst->cdr_z2,
                    inst->cdr_xy,
                    inst->cdr_yz,
                    inst->cdr_xz,
                    r1h1x,
                    i1h1x,
                    r1h1y,
                    i1h1y,
                    r1h1z,
                    i1h1z);

        itemp = DFi2F1(inst->cdr_x2,
                    inst->cdr_y2,
                    inst->cdr_z2,
                    inst->cdr_xy,
                    inst->cdr_yz,
                    inst->cdr_xz,
                    r1h1x,
                    i1h1x,
                    r1h1y,
                    i1h1y,
                    r1h1z,
                    i1h1z);

        *(ckt->CKTrhs + (inst->MOSdNodePrime)) -= temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;
    
        // cdrain term over

        // loading gbs term

        temp = D1n2F1(inst->gbs2,
                        r1h1y,
                        i1h1y);

        itemp = D1i2F1(inst->gbs2,
                        r1h1y,
                        i1h1y);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // gbs over

        // loading gbd term

        temp = D1n2F1(inst->gbd2,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z);

        itemp = D1i2F1(inst->gbd2,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // gbd over

        // loading capgs term

        temp = -ckt->CKTomega *
                D1i2F1(inst->capgs2,
                        r1h1x,
                        i1h1x);

        itemp = ckt->CKTomega *
                D1n2F1(inst->capgs2,
                        r1h1x,
                        i1h1x);

        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // capgs over

        // loading capgd term

        temp = -ckt->CKTomega *
                D1i2F1(inst->capgd2,
                        r1h1x - r1h1z,
                        i1h1x - i1h1z);

        itemp = ckt->CKTomega *
                D1n2F1(inst->capgd2,
                        r1h1x - r1h1z,
                        i1h1x - i1h1z);


        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // capgd over 

        // loading capgb term

        temp = -ckt->CKTomega *
                D1i2F1(inst->capgb2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y);

        itemp = ckt->CKTomega *
                D1n2F1(inst->capgb2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y);

        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSbNode)) += temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) += itemp;

        // capgb over

        // loading capbs term

        temp = -ckt->CKTomega *
                D1i2F1(inst->capbs2,
                        r1h1y,
                        i1h1y);

        itemp = ckt->CKTomega *
                D1n2F1(inst->capbs2,
                        r1h1y,
                        i1h1y);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // capbs over

        // loading capbd term

        temp = -ckt->CKTomega *
                D1i2F1(inst->capbd2,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z);

        itemp = ckt->CKTomega *
                D1n2F1(inst->capbd2,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // capbd over
    // all done

      break;

    case D_THRF1:
    // x = vgs, y = vbs z = vds

        // getting first order (linear) Volterra kernel
        r1h1x = *(job->r1H1ptr + (inst->MOSgNode)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1x = *(job->i1H1ptr + (inst->MOSgNode)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r1h1y = *(job->r1H1ptr + (inst->MOSbNode)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1y = *(job->i1H1ptr + (inst->MOSbNode)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r1h1z = *(job->r1H1ptr + (inst->MOSdNodePrime)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1z = *(job->i1H1ptr + (inst->MOSdNodePrime)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r2h11x = *(job->r2H11ptr + (inst->MOSgNode)) -
                *(job->r2H11ptr + (inst->MOSsNodePrime));
        i2h11x = *(job->i2H11ptr + (inst->MOSgNode)) -
                *(job->i2H11ptr + (inst->MOSsNodePrime));

        r2h11y = *(job->r2H11ptr + (inst->MOSbNode)) -
                *(job->r2H11ptr + (inst->MOSsNodePrime));
        i2h11y = *(job->i2H11ptr + (inst->MOSbNode)) -
                *(job->i2H11ptr + (inst->MOSsNodePrime));

        r2h11z = *(job->r2H11ptr + (inst->MOSdNodePrime)) -
                *(job->r2H11ptr + (inst->MOSsNodePrime));
        i2h11z = *(job->i2H11ptr + (inst->MOSdNodePrime)) -
                *(job->i2H11ptr + (inst->MOSsNodePrime));
        // loading starts here
        // loading cdrain term

        temp = DFn3F1(inst->cdr_x2,
                    inst->cdr_y2,
                    inst->cdr_z2,
                    inst->cdr_xy,
                    inst->cdr_yz,
                    inst->cdr_xz,
                    inst->cdr_x3,
                    inst->cdr_y3,
                    inst->cdr_z3,
                    inst->cdr_x2y,
                    inst->cdr_x2z,
                    inst->cdr_xy2,
                    inst->cdr_y2z,
                    inst->cdr_xz2,
                    inst->cdr_yz2,
                    inst->cdr_xyz,
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
        itemp = DFi3F1(inst->cdr_x2,
                    inst->cdr_y2,
                    inst->cdr_z2,
                    inst->cdr_xy,
                    inst->cdr_yz,
                    inst->cdr_xz,
                    inst->cdr_x3,
                    inst->cdr_y3,
                    inst->cdr_z3,
                    inst->cdr_x2y,
                    inst->cdr_x2z,
                    inst->cdr_xy2,
                    inst->cdr_y2z,
                    inst->cdr_xz2,
                    inst->cdr_yz2,
                    inst->cdr_xyz,
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


        *(ckt->CKTrhs + (inst->MOSdNodePrime)) -= temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;
    
        // cdrain term over

        // loading gbs term

        temp = D1n3F1(inst->gbs2,
                        inst->gbs3,
                        r1h1y,
                        i1h1y,
                        r2h11y,
                        i2h11y);


        itemp = D1i3F1(inst->gbs2,
                        inst->gbs3,
                        r1h1y,
                        i1h1y,
                        r2h11y,
                        i2h11y);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // gbs over

        // loading gbd term

        temp = D1n3F1(inst->gbd2,
                        inst->gbd3,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r2h11y - r2h11z,
                        i2h11y - i2h11z);

        itemp = D1i3F1(inst->gbd2,
                        inst->gbd3,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r2h11y - r2h11z,
                        i2h11y - i2h11z);

        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // gbd over

        // loading capgs term

        temp = -ckt->CKTomega *
                D1i3F1(inst->capgs2,
                        inst->capgs3,
                        r1h1x,
                        i1h1x,
                        r2h11x,
                        i2h11x);

        itemp = ckt->CKTomega *
                D1n3F1(inst->capgs2,
                        inst->capgs3,
                        r1h1x,
                        i1h1x,
                        r2h11x,
                        i2h11x);

        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // capgs over

        // loading capgd term

        temp = -ckt->CKTomega *
                D1i3F1(inst->capgd2,
                        inst->capgd3,
                        r1h1x - r1h1z,
                        i1h1x - i1h1z,
                        r2h11x - r2h11z,
                        i2h11x - i2h11z);

        itemp = ckt->CKTomega *
                D1n3F1(inst->capgd2,
                        inst->capgd3,
                        r1h1x - r1h1z,
                        i1h1x - i1h1z,
                        r2h11x - r2h11z,
                        i2h11x - i2h11z);


        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // capgd over

        // loading capgb term

        temp = -ckt->CKTomega *
                D1i3F1(inst->capgb2,
                        inst->capgb3,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r2h11x - r2h11y,
                        i2h11x - i2h11y);

        itemp = ckt->CKTomega *
                D1n3F1(inst->capgb2,
                        inst->capgb3,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r2h11x - r2h11y,
                        i2h11x - i2h11y);

        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSbNode)) += temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) += itemp;

        // capgb over

        // loading capbs term

        temp = -ckt->CKTomega *
                D1i3F1(inst->capbs2,
                        inst->capbs3,
                        r1h1y,
                        i1h1y,
                        r2h11y,
                        i2h11y);

        itemp = ckt->CKTomega *
                D1n3F1(inst->capbs2,
                        inst->capbs3,
                        r1h1y,
                        i1h1y,
                        r2h11y,
                        i2h11y);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // capbs over

        // loading capbd term

        temp = -ckt->CKTomega *
                D1i3F1(inst->capbd2,
                        inst->capbd3,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r2h11y - r2h11z,
                        i2h11y - i2h11z);

        itemp = ckt->CKTomega *
                D1n3F1(inst->capbd2,
                        inst->capbd3,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r2h11y - r2h11z,
                        i2h11y - i2h11z);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // capbd over
    // all done

      break;
    case D_F1PF2:
    // x = vgs, y = vbs z = vds

        // getting first order (linear) Volterra kernel
        r1h1x = *(job->r1H1ptr + (inst->MOSgNode)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1x = *(job->i1H1ptr + (inst->MOSgNode)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r1h1y = *(job->r1H1ptr + (inst->MOSbNode)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1y = *(job->i1H1ptr + (inst->MOSbNode)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r1h1z = *(job->r1H1ptr + (inst->MOSdNodePrime)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1z = *(job->i1H1ptr + (inst->MOSdNodePrime)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r1h2x = *(job->r1H2ptr + (inst->MOSgNode)) -
            *(job->r1H2ptr + (inst->MOSsNodePrime));
        i1h2x = *(job->i1H2ptr + (inst->MOSgNode)) -
            *(job->i1H2ptr + (inst->MOSsNodePrime));

        r1h2y = *(job->r1H2ptr + (inst->MOSbNode)) -
            *(job->r1H2ptr + (inst->MOSsNodePrime));
        i1h2y = *(job->i1H2ptr + (inst->MOSbNode)) -
            *(job->i1H2ptr + (inst->MOSsNodePrime));

        r1h2z = *(job->r1H2ptr + (inst->MOSdNodePrime)) -
            *(job->r1H2ptr + (inst->MOSsNodePrime));
        i1h2z = *(job->i1H2ptr + (inst->MOSdNodePrime)) -
            *(job->i1H2ptr + (inst->MOSsNodePrime));

        // loading starts here
        // loading cdrain term

        temp = DFnF12(inst->cdr_x2,
                    inst->cdr_y2,
                    inst->cdr_z2,
                    inst->cdr_xy,
                    inst->cdr_yz,
                    inst->cdr_xz,
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

        itemp = DFiF12(inst->cdr_x2,
                    inst->cdr_y2,
                    inst->cdr_z2,
                    inst->cdr_xy,
                    inst->cdr_yz,
                    inst->cdr_xz,
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

        *(ckt->CKTrhs + (inst->MOSdNodePrime)) -= temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;
    
        // cdrain term over

        // loading gbs term

        temp = D1nF12(inst->gbs2,
                        r1h1y,
                        i1h1y,
                        r1h2y,
                        i1h2y);

        itemp = D1iF12(inst->gbs2,
                        r1h1y,
                        i1h1y,
                        r1h2y,
                        i1h2y);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // gbs over

        // loading gbd term

        temp = D1nF12(inst->gbd2,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r1h2y - r1h2z,
                        i1h2y - i1h2z);

        itemp = D1iF12(inst->gbd2,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r1h2y - r1h2z,
                        i1h2y - i1h2z);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // gbd over

        // loading capgs term

        temp = -ckt->CKTomega *
                D1iF12(inst->capgs2,
                        r1h1x,
                        i1h1x,
                        r1h2x,
                        i1h2x);

        itemp = ckt->CKTomega *
                D1nF12(inst->capgs2,
                        r1h1x,
                        i1h1x,
                        r1h2x,
                        i1h2x);

        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // capgs over

        // loading capgd term

        temp = -ckt->CKTomega *
                D1iF12(inst->capgd2,
                        r1h1x - r1h1z,
                        i1h1x - i1h1z,
                        r1h2x - r1h2z,
                        i1h2x - i1h2z);

        itemp = ckt->CKTomega *
                D1nF12(inst->capgd2,
                        r1h1x - r1h1z,
                        i1h1x - i1h1z,
                        r1h2x - r1h2z,
                        i1h2x - i1h2z);


        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // capgd over

        // loading capgb term

        temp = -ckt->CKTomega *
                D1iF12(inst->capgb2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1h2x - r1h2y,
                        i1h2x - i1h2y);

        itemp = ckt->CKTomega *
                D1nF12(inst->capgb2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1h2x - r1h2y,
                        i1h2x - i1h2y);

        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSbNode)) += temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) += itemp;

        // capgb over

        // loading capbs term

        temp = -ckt->CKTomega *
                D1iF12(inst->capbs2,
                        r1h1y,
                        i1h1y,
                        r1h2y,
                        i1h2y);

        itemp = ckt->CKTomega *
                D1nF12(inst->capbs2,
                        r1h1y,
                        i1h1y,
                        r1h2y,
                        i1h2y);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // capbs over

        // loading capbd term

        temp = -ckt->CKTomega *
                D1iF12(inst->capbd2,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r1h2y - r1h2z,
                        i1h2y - i1h2z);

        itemp = ckt->CKTomega *
                D1nF12(inst->capbd2,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r1h2y - r1h2z,
                        i1h2y - i1h2z);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // capbd over
    // all done

      break;
    case D_F1MF2:
    // x = vgs, y = vbs z = vds

        // getting first order (linear) Volterra kernel
        r1h1x = *(job->r1H1ptr + (inst->MOSgNode)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1x = *(job->i1H1ptr + (inst->MOSgNode)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r1h1y = *(job->r1H1ptr + (inst->MOSbNode)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1y = *(job->i1H1ptr + (inst->MOSbNode)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r1h1z = *(job->r1H1ptr + (inst->MOSdNodePrime)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1z = *(job->i1H1ptr + (inst->MOSdNodePrime)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r1hm2x = *(job->r1H2ptr + (inst->MOSgNode)) -
            *(job->r1H2ptr + (inst->MOSsNodePrime));
        i1hm2x = -(*(job->i1H2ptr + (inst->MOSgNode)) -
            *(job->i1H2ptr + (inst->MOSsNodePrime)));

        r1hm2y = *(job->r1H2ptr + (inst->MOSbNode)) -
            *(job->r1H2ptr + (inst->MOSsNodePrime));
        i1hm2y = -(*(job->i1H2ptr + (inst->MOSbNode)) -
            *(job->i1H2ptr + (inst->MOSsNodePrime)));

        r1hm2z = *(job->r1H2ptr + (inst->MOSdNodePrime)) -
            *(job->r1H2ptr + (inst->MOSsNodePrime));
    i1hm2z = -(*(job->i1H2ptr + (inst->MOSdNodePrime)) -
            *(job->i1H2ptr + (inst->MOSsNodePrime)));

        // loading starts here
        // loading cdrain term

        temp = DFnF12(inst->cdr_x2,
                    inst->cdr_y2,
                    inst->cdr_z2,
                    inst->cdr_xy,
                    inst->cdr_yz,
                    inst->cdr_xz,
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

        itemp = DFiF12(inst->cdr_x2,
                    inst->cdr_y2,
                    inst->cdr_z2,
                    inst->cdr_xy,
                    inst->cdr_yz,
                    inst->cdr_xz,
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

        *(ckt->CKTrhs + (inst->MOSdNodePrime)) -= temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;
    
        // cdrain term over

        // loading gbs term

        temp = D1nF12(inst->gbs2,
                        r1h1y,
                        i1h1y,
                        r1hm2y,
                        i1hm2y);

        itemp = D1iF12(inst->gbs2,
                        r1h1y,
                        i1h1y,
                        r1hm2y,
                        i1hm2y);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // gbs over

        // loading gbd term

        temp = D1nF12(inst->gbd2,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r1hm2y - r1hm2z,
                        i1hm2y - i1hm2z);

        itemp = D1iF12(inst->gbd2,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r1hm2y - r1hm2z,
                        i1hm2y - i1hm2z);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // gbd over

        // loading capgs term

        temp = -ckt->CKTomega *
                D1iF12(inst->capgs2,
                        r1h1x,
                        i1h1x,
                        r1hm2x,
                        i1hm2x);

        itemp = ckt->CKTomega *
                D1nF12(inst->capgs2,
                        r1h1x,
                        i1h1x,
                        r1hm2x,
                        i1hm2x);

        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // capgs over

        // loading capgd term

        temp = -ckt->CKTomega *
                D1iF12(inst->capgd2,
                        r1h1x - r1h1z,
                        i1h1x - i1h1z,
                        r1hm2x - r1hm2z,
                        i1hm2x - i1hm2z);

        itemp = ckt->CKTomega *
                D1nF12(inst->capgd2,
                        r1h1x - r1h1z,
                        i1h1x - i1h1z,
                        r1hm2x - r1hm2z,
                        i1hm2x - i1hm2z);


        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // capgd over
        // loading capgb term

        temp = -ckt->CKTomega *
                D1iF12(inst->capgb2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1hm2x - r1hm2y,
                        i1hm2x - i1hm2y);

        itemp = ckt->CKTomega *
                D1nF12(inst->capgb2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1hm2x - r1hm2y,
                        i1hm2x - i1hm2y);

        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSbNode)) += temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) += itemp;

        // capgb over

        // loading capbs term

        temp = -ckt->CKTomega *
                D1iF12(inst->capbs2,
                        r1h1y,
                        i1h1y,
                        r1hm2y,
                        i1hm2y);

        itemp = ckt->CKTomega *
                D1nF12(inst->capbs2,
                        r1h1y,
                        i1h1y,
                        r1hm2y,
                        i1hm2y);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // capbs over

        // loading capbd term

        temp = -ckt->CKTomega *
                D1iF12(inst->capbd2,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r1hm2y - r1hm2z,
                        i1hm2y - i1hm2z);

        itemp = ckt->CKTomega *
                D1nF12(inst->capbd2,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r1hm2y - r1hm2z,
                        i1hm2y - i1hm2z);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // capbd over
    // all done

      break;
    case D_2F1MF2:
    // x = vgs, y = vbs z = vds

        // getting first order (linear) Volterra kernel
        r1h1x = *(job->r1H1ptr + (inst->MOSgNode)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1x = *(job->i1H1ptr + (inst->MOSgNode)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r1h1y = *(job->r1H1ptr + (inst->MOSbNode)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1y = *(job->i1H1ptr + (inst->MOSbNode)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r1h1z = *(job->r1H1ptr + (inst->MOSdNodePrime)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i1h1z = *(job->i1H1ptr + (inst->MOSdNodePrime)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r1hm2x = *(job->r1H2ptr + (inst->MOSgNode)) -
            *(job->r1H2ptr + (inst->MOSsNodePrime));
        i1hm2x = -(*(job->i1H2ptr + (inst->MOSgNode)) -
            *(job->i1H2ptr + (inst->MOSsNodePrime)));

        r1hm2y = *(job->r1H2ptr + (inst->MOSbNode)) -
            *(job->r1H2ptr + (inst->MOSsNodePrime));
        i1hm2y = -(*(job->i1H2ptr + (inst->MOSbNode)) -
            *(job->i1H2ptr + (inst->MOSsNodePrime)));

        r1hm2z = *(job->r1H2ptr + (inst->MOSdNodePrime)) -
            *(job->r1H2ptr + (inst->MOSsNodePrime));
        i1hm2z = -(*(job->i1H2ptr + (inst->MOSdNodePrime)) -
            *(job->i1H2ptr + (inst->MOSsNodePrime)));

        r2h11x = *(job->r1H1ptr + (inst->MOSgNode)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i2h11x = *(job->i1H1ptr + (inst->MOSgNode)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r2h11y = *(job->r1H1ptr + (inst->MOSbNode)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i2h11y = *(job->i1H1ptr + (inst->MOSbNode)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r2h11z = *(job->r1H1ptr + (inst->MOSdNodePrime)) -
            *(job->r1H1ptr + (inst->MOSsNodePrime));
        i2h11z = *(job->i1H1ptr + (inst->MOSdNodePrime)) -
            *(job->i1H1ptr + (inst->MOSsNodePrime));

        r2h1m2x = *(job->r2H1m2ptr + (inst->MOSgNode)) -
            *(job->r2H1m2ptr + (inst->MOSsNodePrime));
        i2h1m2x = *(job->i2H1m2ptr + (inst->MOSgNode)) -
            *(job->i2H1m2ptr + (inst->MOSsNodePrime));

        r2h1m2y = *(job->r2H1m2ptr + (inst->MOSbNode)) -
            *(job->r2H1m2ptr + (inst->MOSsNodePrime));
        i2h1m2y = *(job->i2H1m2ptr + (inst->MOSbNode)) -
            *(job->i2H1m2ptr + (inst->MOSsNodePrime));

        r2h1m2z = *(job->r2H1m2ptr + (inst->MOSdNodePrime)) -
                *(job->r2H1m2ptr + (inst->MOSsNodePrime));
        i2h1m2z = *(job->i2H1m2ptr + (inst->MOSdNodePrime)) -
                *(job->i2H1m2ptr + (inst->MOSsNodePrime));

        // loading starts inst
        // loading cdrain term

        pass.cxx = inst->cdr_x2;
        pass.cyy = inst->cdr_y2;
        pass.czz = inst->cdr_z2;
        pass.cxy = inst->cdr_xy;
        pass.cyz = inst->cdr_yz;
        pass.cxz = inst->cdr_xz;
        pass.cxxx = inst->cdr_x3;
        pass.cyyy = inst->cdr_y3;
        pass.czzz = inst->cdr_z3;
        pass.cxxy = inst->cdr_x2y;
        pass.cxxz = inst->cdr_x2z;
        pass.cxyy = inst->cdr_xy2;
        pass.cyyz = inst->cdr_y2z;
        pass.cxzz = inst->cdr_xz2;
        pass.cyzz = inst->cdr_yz2;
        pass.cxyz = inst->cdr_xyz;
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


        *(ckt->CKTrhs + (inst->MOSdNodePrime)) -= temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;
    
        // cdrain term over

        // loading gbs term

        temp = D1n2F12(inst->gbs2,
                        inst->gbs3,
                        r1h1y,
                        i1h1y,
                        r1hm2y,
                        i1hm2y,
                        r2h11y,
                        i2h11y,
                        r2h1m2y,
                        i2h1m2y);



        itemp = D1i2F12(inst->gbs2,
                        inst->gbs3,
                        r1h1y,
                        i1h1y,
                        r1hm2y,
                        i1hm2y,
                        r2h11y,
                        i2h11y,
                        r2h1m2y,
                        i2h1m2y);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // gbs over

        // loading gbd term

        temp = D1n2F12(inst->gbd2,
                        inst->gbd3,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r1hm2y - r1hm2z,
                        i1hm2y - i1hm2z,
                        r2h11y - r2h11z,
                        i2h11y - i2h11z,
                        r2h1m2y - r2h1m2z,
                    i2h1m2y - i2h1m2z);

        itemp = D1i2F12(inst->gbd2,
                        inst->gbd3,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r1hm2y - r1hm2z,
                        i1hm2y - i1hm2z,
                        r2h11y - r2h11z,
                        i2h11y - i2h11z,
                        r2h1m2y - r2h1m2z,
                    i2h1m2y - i2h1m2z);

        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // gbd over

        // loading capgs term

        temp = -ckt->CKTomega *
            D1i2F12(inst->capgs2,
                        inst->capgs3,
                        r1h1x,
                        i1h1x,
                        r1hm2x,
                        i1hm2x,
                        r2h11x,
                        i2h11x,
                        r2h1m2x,
                        i2h1m2x);

        itemp = ckt->CKTomega *
            D1n2F12(inst->capgs2,
                        inst->capgs3,
                        r1h1x,
                        i1h1x,
                        r1hm2x,
                        i1hm2x,
                        r2h11x,
                        i2h11x,
                        r2h1m2x,
                        i2h1m2x);

        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // capgs over

        // loading capgd term

        temp = -ckt->CKTomega *
            D1i2F12(inst->capgd2,
                        inst->capgd3,
                        r1h1x - r1h1z,
                        i1h1x - i1h1z,
                        r1hm2x - r1hm2z,
                        i1hm2x - i1hm2z,
                        r2h11x - r2h11z,
                        i2h11x - i2h11z,
                        r2h1m2x - r2h1m2z,
                    i2h1m2x - i2h1m2z);

        itemp = ckt->CKTomega *
            D1n2F12(inst->capgd2,
                        inst->capgd3,
                        r1h1x - r1h1z,
                        i1h1x - i1h1z,
                        r1hm2x - r1hm2z,
                        i1hm2x - i1hm2z,
                        r2h11x - r2h11z,
                        i2h11x - i2h11z,
                        r2h1m2x - r2h1m2z,
                    i2h1m2x - i2h1m2z);


        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // capgd over
        // loading capgb term

        temp = -ckt->CKTomega *
            D1i2F12(inst->capgb2,
                        inst->capgb3,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1hm2x - r1hm2y,
                        i1hm2x - i1hm2y,
                        r2h11x - r2h11y,
                        i2h11x - i2h11y,
                        r2h1m2x - r2h1m2y,
                    i2h1m2x - i2h1m2y);

        itemp = ckt->CKTomega *
            D1n2F12(inst->capgb2,
                        inst->capgb3,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1hm2x - r1hm2y,
                        i1hm2x - i1hm2y,
                        r2h11x - r2h11y,
                        i2h11x - i2h11y,
                        r2h1m2x - r2h1m2y,
                    i2h1m2x - i2h1m2y);

        *(ckt->CKTrhs + (inst->MOSgNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSgNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSbNode)) += temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) += itemp;

        // capgb over

        // loading capbs term

        temp = -ckt->CKTomega *
            D1i2F12(inst->capbs2,
                        inst->capbs3,
                        r1h1y,
                        i1h1y,
                        r1hm2y,
                        i1hm2y,
                        r2h11y,
                        i2h11y,
                        r2h1m2y,
                        i2h1m2y);

        itemp = ckt->CKTomega *
            D1n2F12(inst->capbs2,
                        inst->capbs3,
                        r1h1y,
                        i1h1y,
                        r1hm2y,
                        i1hm2y,
                        r2h11y,
                        i2h11y,
                        r2h1m2y,
                        i2h1m2y);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSsNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSsNodePrime)) += itemp;

        // capbs over

        // loading capbd term

        temp = -ckt->CKTomega *
            D1i2F12(inst->capbd2,
                        inst->capbd3,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r1hm2y - r1hm2z,
                        i1hm2y - i1hm2z,
                        r2h11y - r2h11z,
                        i2h11y - i2h11z,
                        r2h1m2y - r2h1m2z,
                    i2h1m2y - i2h1m2z);

        itemp = ckt->CKTomega *
            D1n2F12(inst->capbd2,
                        inst->capbd3,
                        r1h1y - r1h1z,
                        i1h1y - i1h1z,
                        r1hm2y - r1hm2z,
                        i1hm2y - i1hm2z,
                        r2h11y - r2h11z,
                        i2h11y - i2h11z,
                        r2h1m2y - r2h1m2z,
                    i2h1m2y - i2h1m2z);


        *(ckt->CKTrhs + (inst->MOSbNode)) -= temp;
        *(ckt->CKTirhs + (inst->MOSbNode)) -= itemp;
        *(ckt->CKTrhs + (inst->MOSdNodePrime)) += temp;
        *(ckt->CKTirhs + (inst->MOSdNodePrime)) += itemp;

        // capbd over
    // all done

      break;
    default:
;
    }
  }
}
return(OK);
}
  else
    return (E_BADPARM);
}
