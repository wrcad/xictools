
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
 $Id: jfetdist.cc,v 1.0 1998/01/30 05:30:17 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1988 Jaijeet S Roychowdhury
         1993 Stephen R. Whiteley
****************************************************************************/
/********** new in 3f2
Sydney University mods Copyright(c) 1989 Anthony E. Parker, David J. Skellern
    Laboratory for Communication Science Engineering
    Sydney University Department of Electrical Engineering, Australia
**********/

#define DISTO
#include "jfetdefs.h"
#include "distdefs.h"

extern int JFETdSetup(sJFETmodel*, sCKT*);


int
JFETdev::disto(int mode, sGENmodel *genmod, sCKT *ckt)
{
    sJFETmodel *model = (sJFETmodel*)genmod;
    sJFETinstance *inst;
    sGENinstance *geninst;
    sDISTOAN* job = (sDISTOAN*) ckt->CKTcurJob;
    DpassStr pass;
    double r1h1x,i1h1x;
    double r1h1y,i1h1y;
    double r1h2x, i1h2x;
    double r1h2y, i1h2y;
    double r1hm2x,i1hm2x;
    double r1hm2y,i1hm2y;
    double r2h11x,i2h11x;
    double r2h11y,i2h11y;
    double r2h1m2x,i2h1m2x;
    double r2h1m2y,i2h1m2y;
    double temp, itemp;

    if (mode == D_SETUP)
        return (dSetup(model, ckt));

    if ((mode == D_TWOF1) || (mode == D_THRF1) || 
        (mode == D_F1PF2) || (mode == D_F1MF2) ||
        (mode == D_2F1MF2)) {

    for ( ; genmod != 0; genmod = genmod->GENnextModel) {
        model = (sJFETmodel*)genmod;
        for (geninst = genmod->GENinstances; geninst != 0; 
                geninst = geninst->GENnextInstance) {
            inst = (sJFETinstance*)geninst;

    // loading starts here

    switch (mode) {
    case D_TWOF1:
    // x = vgs, y = vds

        // getting first order (linear) Volterra kernel
        r1h1x = *(job->r1H1ptr + (inst->JFETgateNode)) -
            *(job->r1H1ptr + (inst->JFETsourcePrimeNode));
        i1h1x = *(job->i1H1ptr + (inst->JFETgateNode)) -
            *(job->i1H1ptr + (inst->JFETsourcePrimeNode));

        r1h1y = *(job->r1H1ptr + (inst->JFETdrainPrimeNode)) -
            *(job->r1H1ptr + (inst->JFETsourcePrimeNode));
        i1h1y = *(job->i1H1ptr + (inst->JFETdrainPrimeNode)) -
            *(job->i1H1ptr + (inst->JFETsourcePrimeNode));


        // loading starts here
        // loading cdrain term

        temp = DFn2F1(inst->cdr_x2,
                    inst->cdr_y2,
                    0.0,
                    inst->cdr_xy,
                    0.0,
                    0.0,
                    r1h1x,
                    i1h1x,
                    r1h1y,
                    i1h1y,
                    0.0,
                    0.0);

        itemp = DFi2F1(inst->cdr_x2,
                    inst->cdr_y2,
                    0.0,
                    inst->cdr_xy,
                    0.0,
                    0.0,
                    r1h1x,
                    i1h1x,
                    r1h1y,
                    i1h1y,
                    0.0,
                    0.0);

        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;
    
        // cdrain term over

        // loading ggs term

        temp = D1n2F1(inst->ggs2,
                        r1h1x,
                        i1h1x);

        itemp = D1i2F1(inst->ggs2,
                        r1h1x,
                        i1h1x);


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;

        // ggs over

        // loading ggd term

        temp = D1n2F1(inst->ggd2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y);

        itemp = D1i2F1(inst->ggd2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y);

        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) += itemp;

        // ggd over

        // loading capgs term

        temp = -ckt->CKTomega *
                D1i2F1(inst->capgs2,
                        r1h1x,
                        i1h1x);

        itemp = ckt->CKTomega *
                D1n2F1(inst->capgs2,
                        r1h1x,
                        i1h1x);

        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;

        // capgs over

        // loading capgd term

        temp = -ckt->CKTomega *
                D1i2F1(inst->capgd2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y);

        itemp = ckt->CKTomega *
                D1n2F1(inst->capgd2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y);

        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) += itemp;

        // capgd over

    // all done

      break;

    case D_THRF1:
    // x = vgs, y = vds

        // getting first order (linear) Volterra kernel
        r1h1x = *(job->r1H1ptr + (inst->JFETgateNode)) -
            *(job->r1H1ptr + (inst->JFETsourcePrimeNode));
        i1h1x = *(job->i1H1ptr + (inst->JFETgateNode)) -
            *(job->i1H1ptr + (inst->JFETsourcePrimeNode));

        r1h1y = *(job->r1H1ptr + (inst->JFETdrainPrimeNode)) -
            *(job->r1H1ptr + (inst->JFETsourcePrimeNode));
        i1h1y = *(job->i1H1ptr + (inst->JFETdrainPrimeNode)) -
            *(job->i1H1ptr + (inst->JFETsourcePrimeNode));

        r2h11x = *(job->r2H11ptr + (inst->JFETgateNode)) -
            *(job->r2H11ptr + (inst->JFETsourcePrimeNode));
        i2h11x = *(job->i2H11ptr + (inst->JFETgateNode)) -
            *(job->i2H11ptr + (inst->JFETsourcePrimeNode));

        r2h11y = *(job->r2H11ptr + (inst->JFETdrainPrimeNode)) -
            *(job->r2H11ptr + (inst->JFETsourcePrimeNode));
        i2h11y = *(job->i2H11ptr + (inst->JFETdrainPrimeNode)) -
            *(job->i2H11ptr + (inst->JFETsourcePrimeNode));

        // loading starts here
        // loading cdrain term

        temp = DFn3F1(inst->cdr_x2,
                    inst->cdr_y2,
                    0.0,
                    inst->cdr_xy,
                    0.0,
                    0.0,
                    inst->cdr_x3,
                    inst->cdr_y3,
                    0.0,
                    inst->cdr_x2y,
                    0.0,
                    inst->cdr_xy2,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    r1h1x,
                    i1h1x,
                    r1h1y,
                    i1h1y,
                    0.0,
                    0.0,
                    r2h11x,
                    i2h11x,
                    r2h11y,
                    i2h11y,
                    0.0,
                    0.0);

        itemp = DFi3F1(inst->cdr_x2,
                    inst->cdr_y2,
                    0.0,
                    inst->cdr_xy,
                    0.0,
                    0.0,
                    inst->cdr_x3,
                    inst->cdr_y3,
                    0.0,
                    inst->cdr_x2y,
                    0.0,
                    inst->cdr_xy2,
                    0.0,
                    0.0,
                    0.0,
                    0.0,
                    r1h1x,
                    i1h1x,
                    r1h1y,
                    i1h1y,
                    0.0,
                    0.0,
                    r2h11x,
                    i2h11x,
                    r2h11y,
                    i2h11y,
                    0.0,
                    0.0);

        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;
    
        // cdrain term over

        // loading ggs term

        temp = D1n3F1(inst->ggs2,
                        inst->ggs3,
                        r1h1x,
                        i1h1x,
                        r2h11x,
                        i2h11x);

        itemp = D1i3F1(inst->ggs2,
                        inst->ggs3,
                        r1h1x,
                        i1h1x,
                        r2h11x,
                        i2h11x);


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;

        // ggs over

        // loading ggd term

        temp = D1n3F1(inst->ggd2,
                        inst->ggd3,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r2h11x - r2h11y,
                        i2h11x - i2h11y);

        itemp = D1i3F1(inst->ggd2,
                        inst->ggd3,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r2h11x - r2h11y,
                        i2h11x - i2h11y);



        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) += itemp;

        // ggd over

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


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;

        // capgs over

        // loading capgd term

        temp = -ckt->CKTomega *
                D1i3F1(inst->capgd2,
                        inst->capgd3,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r2h11x - r2h11y,
                        i2h11x - i2h11y);

        itemp = ckt->CKTomega *
                D1n3F1(inst->capgd2,
                        inst->capgd3,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r2h11x - r2h11y,
                        i2h11x - i2h11y);


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) += itemp;

        // capgd over

    // all done

      break;
    case D_F1PF2:
    // x = vgs, y = vds

        // getting first order (linear) Volterra kernel
        r1h1x = *(job->r1H1ptr + (inst->JFETgateNode)) -
            *(job->r1H1ptr + (inst->JFETsourcePrimeNode));
        i1h1x = *(job->i1H1ptr + (inst->JFETgateNode)) -
            *(job->i1H1ptr + (inst->JFETsourcePrimeNode));

        r1h1y = *(job->r1H1ptr + (inst->JFETdrainPrimeNode)) -
            *(job->r1H1ptr + (inst->JFETsourcePrimeNode));
        i1h1y = *(job->i1H1ptr + (inst->JFETdrainPrimeNode)) -
            *(job->i1H1ptr + (inst->JFETsourcePrimeNode));

        r1h2x = *(job->r1H2ptr + (inst->JFETgateNode)) -
            *(job->r1H2ptr + (inst->JFETsourcePrimeNode));
        i1h2x = *(job->i1H2ptr + (inst->JFETgateNode)) -
            *(job->i1H2ptr + (inst->JFETsourcePrimeNode));

        r1h2y = *(job->r1H2ptr + (inst->JFETdrainPrimeNode)) -
            *(job->r1H2ptr + (inst->JFETsourcePrimeNode));
        i1h2y = *(job->i1H2ptr + (inst->JFETdrainPrimeNode)) -
            *(job->i1H2ptr + (inst->JFETsourcePrimeNode));

        // loading starts here
        // loading cdrain term

        temp = DFnF12(inst->cdr_x2,
                    inst->cdr_y2,
                    0.0,
                    inst->cdr_xy,
                    0.0,
                    0.0,
                    r1h1x,
                    i1h1x,
                    r1h1y,
                    i1h1y,
                    0.0,
                    0.0,
                    r1h2x,
                    i1h2x,
                    r1h2y,
                    i1h2y,
                    0.0,
                    0.0);

        itemp = DFiF12(inst->cdr_x2,
                    inst->cdr_y2,
                    0.0,
                    inst->cdr_xy,
                    0.0,
                    0.0,
                    r1h1x,
                    i1h1x,
                    r1h1y,
                    i1h1y,
                    0.0,
                    0.0,
                    r1h2x,
                    i1h2x,
                    r1h2y,
                    i1h2y,
                    0.0,
                    0.0);

        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;
    
        // cdrain term over

        // loading ggs term

        temp = D1nF12(inst->ggs2,
                        r1h1x,
                        i1h1x,
                        r1h2x,
                        i1h2x);

        itemp = D1iF12(inst->ggs2,
                        r1h1x,
                        i1h1x,
                        r1h2x,
                        i1h2x);


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;

        // ggs over

        // loading ggd term

        temp = D1nF12(inst->ggd2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1h2x - r1h2y,
                        i1h2x - i1h2y);

        itemp = D1iF12(inst->ggd2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1h2x - r1h2y,
                        i1h2x - i1h2y);


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) += itemp;

        // ggd over

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


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;

        // capgs over

        // loading capgd term

        temp = -ckt->CKTomega *
                D1iF12(inst->capgd2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1h2x - r1h2y,
                        i1h2x - i1h2y);

        itemp = ckt->CKTomega *
                D1nF12(inst->capgd2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1h2x - r1h2y,
                        i1h2x - i1h2y);


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) += itemp;

        // capgd over

    // all done

      break;
    case D_F1MF2:
    // x = vgs, y = vds

        // getting first order (linear) Volterra kernel
        r1h1x = *(job->r1H1ptr + (inst->JFETgateNode)) -
            *(job->r1H1ptr + (inst->JFETsourcePrimeNode));
        i1h1x = *(job->i1H1ptr + (inst->JFETgateNode)) -
            *(job->i1H1ptr + (inst->JFETsourcePrimeNode));

        r1h1y = *(job->r1H1ptr + (inst->JFETdrainPrimeNode)) -
            *(job->r1H1ptr + (inst->JFETsourcePrimeNode));
        i1h1y = *(job->i1H1ptr + (inst->JFETdrainPrimeNode)) -
            *(job->i1H1ptr + (inst->JFETsourcePrimeNode));

        r1hm2x = *(job->r1H2ptr + (inst->JFETgateNode)) -
            *(job->r1H2ptr + (inst->JFETsourcePrimeNode));
        i1hm2x = -(*(job->i1H2ptr + (inst->JFETgateNode)) -
            *(job->i1H2ptr + (inst->JFETsourcePrimeNode)));

        r1hm2y = *(job->r1H2ptr + (inst->JFETdrainPrimeNode)) -
            *(job->r1H2ptr + (inst->JFETsourcePrimeNode));
        i1hm2y = -(*(job->i1H2ptr + (inst->JFETdrainPrimeNode)) -
            *(job->i1H2ptr + (inst->JFETsourcePrimeNode)));

        // loading starts here
        // loading cdrain term

        temp = DFnF12(inst->cdr_x2,
                    inst->cdr_y2,
                    0.0,
                    inst->cdr_xy,
                    0.0,
                    0.0,
                    r1h1x,
                    i1h1x,
                    r1h1y,
                    i1h1y,
                    0.0,
                    0.0,
                    r1hm2x,
                    i1hm2x,
                    r1hm2y,
                    i1hm2y,
                    0.0,
                    0.0);

        itemp = DFiF12(inst->cdr_x2,
                    inst->cdr_y2,
                    0.0,
                    inst->cdr_xy,
                    0.0,
                    0.0,
                    r1h1x,
                    i1h1x,
                    r1h1y,
                    i1h1y,
                    0.0,
                    0.0,
                    r1hm2x,
                    i1hm2x,
                    r1hm2y,
                    i1hm2y,
                    0.0,
                    0.0);

        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;
    
        // cdrain term over

        // loading ggs term

        temp = D1nF12(inst->ggs2,
                        r1h1x,
                        i1h1x,
                        r1hm2x,
                        i1hm2x);

        itemp = D1iF12(inst->ggs2,
                        r1h1x,
                        i1h1x,
                        r1hm2x,
                        i1hm2x);


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;

        // ggs over

        // loading ggd term

        temp = D1nF12(inst->ggd2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1hm2x - r1hm2y,
                        i1hm2x - i1hm2y);

        itemp = D1iF12(inst->ggd2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1hm2x - r1hm2y,
                        i1hm2x - i1hm2y);


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) += itemp;

        // ggd over

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


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;

        // capgs over

        // loading capgd term

        temp = -ckt->CKTomega *
                D1iF12(inst->capgd2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1hm2x - r1hm2y,
                        i1hm2x - i1hm2y);

        itemp = ckt->CKTomega *
                D1nF12(inst->capgd2,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1hm2x - r1hm2y,
                        i1hm2x - i1hm2y);


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) += itemp;

        // capgd over

    // all done

      break;
    case D_2F1MF2:
    // x = vgs, y = vds

        // getting first order (linear) Volterra kernel
        r1h1x = *(job->r1H1ptr + (inst->JFETgateNode)) -
            *(job->r1H1ptr + (inst->JFETsourcePrimeNode));
        i1h1x = *(job->i1H1ptr + (inst->JFETgateNode)) -
            *(job->i1H1ptr + (inst->JFETsourcePrimeNode));

        r1h1y = *(job->r1H1ptr + (inst->JFETdrainPrimeNode)) -
            *(job->r1H1ptr + (inst->JFETsourcePrimeNode));
        i1h1y = *(job->i1H1ptr + (inst->JFETdrainPrimeNode)) -
            *(job->i1H1ptr + (inst->JFETsourcePrimeNode));

        r2h11x = *(job->r2H11ptr + (inst->JFETgateNode)) -
            *(job->r2H11ptr + (inst->JFETsourcePrimeNode));
        i2h11x = *(job->i2H11ptr + (inst->JFETgateNode)) -
            *(job->i2H11ptr + (inst->JFETsourcePrimeNode));

        r2h11y = *(job->r2H11ptr + (inst->JFETdrainPrimeNode)) -
            *(job->r2H11ptr + (inst->JFETsourcePrimeNode));
        i2h11y = *(job->i2H11ptr + (inst->JFETdrainPrimeNode)) -
            *(job->i2H11ptr + (inst->JFETsourcePrimeNode));

        r1hm2x = *(job->r1H2ptr + (inst->JFETgateNode)) -
            *(job->r1H2ptr + (inst->JFETsourcePrimeNode));
        i1hm2x = -(*(job->i1H2ptr + (inst->JFETgateNode)) -
            *(job->i1H2ptr + (inst->JFETsourcePrimeNode)));

        r1hm2y = *(job->r1H2ptr + (inst->JFETdrainPrimeNode)) -
            *(job->r1H2ptr + (inst->JFETsourcePrimeNode));
        i1hm2y = -(*(job->i1H2ptr + (inst->JFETdrainPrimeNode)) -
            *(job->i1H2ptr + (inst->JFETsourcePrimeNode)));

        r2h1m2x = *(job->r2H1m2ptr + (inst->JFETgateNode)) -
            *(job->r2H1m2ptr + (inst->JFETsourcePrimeNode));
        i2h1m2x = *(job->i2H1m2ptr + (inst->JFETgateNode)) -
            *(job->i2H1m2ptr + (inst->JFETsourcePrimeNode));

        r2h1m2y = *(job->r2H1m2ptr + (inst->JFETdrainPrimeNode)) -
            *(job->r2H1m2ptr + (inst->JFETsourcePrimeNode));
        i2h1m2y = *(job->i2H1m2ptr + (inst->JFETdrainPrimeNode)) -
            *(job->i2H1m2ptr + (inst->JFETsourcePrimeNode));

        // loading starts here
        // loading cdrain term

        pass.cxx = inst->cdr_x2;
        pass.cyy = inst->cdr_y2;
        pass.czz = 0.0;
        pass.cxy = inst->cdr_xy;
        pass.cyz = 0.0;
        pass.cxz = 0.0;
        pass.cxxx = inst->cdr_x3;
        pass.cyyy = inst->cdr_y3;
        pass.czzz = 0.0;
        pass.cxxy = inst->cdr_x2y;
        pass.cxxz = 0.0;
        pass.cxyy = inst->cdr_xy2;
        pass.cyyz = 0.0;
        pass.cxzz = 0.0;
        pass.cyzz = 0.0;
        pass.cxyz = 0.0;
        pass.r1h1x = r1h1x;
        pass.i1h1x = i1h1x;
        pass.r1h1y = r1h1y;
        pass.i1h1y = i1h1y;
        pass.r1h1z = 0.0;
        pass.i1h1z = 0.0;
        pass.r1h2x = r1hm2x;
        pass.i1h2x = i1hm2x;
        pass.r1h2y = r1hm2y;
        pass.i1h2y = i1hm2y;
        pass.r1h2z = 0.0;
        pass.i1h2z = 0.0;
        pass.r2h11x = r2h11x;
        pass.i2h11x = i2h11x;
        pass.r2h11y = r2h11y;
        pass.i2h11y = i2h11y;
        pass.r2h11z = 0.0;
        pass.i2h11z = 0.0;
        pass.h2f1f2x = r2h1m2x;
        pass.ih2f1f2x = i2h1m2x;
        pass.h2f1f2y = r2h1m2y;
        pass.ih2f1f2y = i2h1m2y;
        pass.h2f1f2z = 0.0;
        pass.ih2f1f2z = 0.0;

        temp = DFn2F12(&pass);
        itemp = DFi2F12(&pass);

        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;
    
        // cdrain term over

        // loading ggs term

        temp = D1n2F12(inst->ggs2,
                        inst->ggs3,
                        r1h1x,
                        i1h1x,
                        r1hm2x,
                        i1hm2x,
                        r2h11x,
                        i2h11x,
                        r2h1m2x,
                        i2h1m2x);

        itemp = D1i2F12(inst->ggs2,
                        inst->ggs3,
                        r1h1x,
                        i1h1x,
                        r1hm2x,
                        i1hm2x,
                        r2h11x,
                        i2h11x,
                        r2h1m2x,
                        i2h1m2x);


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;

        // ggs over

        // loading ggd term

        temp = D1n2F12(inst->ggd2,
                        inst->ggd3,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1hm2x - r1hm2y,
                        i1hm2x - i1hm2y,
                        r2h11x - r2h11y,
                        i2h11x - i2h11y,
                        r2h1m2x - r2h1m2y,
                    i2h1m2x - i2h1m2y);

        itemp = D1i2F12(inst->ggd2,
                        inst->ggd3,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1hm2x - r1hm2y,
                        i1hm2x - i1hm2y,
                        r2h11x - r2h11y,
                        i2h11x - i2h11y,
                        r2h1m2x - r2h1m2y,
                    i2h1m2x - i2h1m2y);



        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) += itemp;

        // ggd over

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


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETsourcePrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETsourcePrimeNode)) += itemp;

        // capgs over

        // loading capgd term

        temp = -ckt->CKTomega *
                D1i2F12(inst->capgd2,
                        inst->capgd3,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1hm2x - r1hm2y,
                        i1hm2x - i1hm2y,
                        r2h11x - r2h11y,
                        i2h11x - i2h11y,
                        r2h1m2x - r2h1m2y,
                    i2h1m2x - i2h1m2y);

        itemp = ckt->CKTomega *
                D1n2F12(inst->capgd2,
                        inst->capgd3,
                        r1h1x - r1h1y,
                        i1h1x - i1h1y,
                        r1hm2x - r1hm2y,
                        i1hm2x - i1hm2y,
                        r2h11x - r2h11y,
                        i2h11x - i2h11y,
                        r2h1m2x - r2h1m2y,
                    i2h1m2x - i2h1m2y);


        *(ckt->CKTrhs + (inst->JFETgateNode)) -= temp;
        *(ckt->CKTirhs + (inst->JFETgateNode)) -= itemp;
        *(ckt->CKTrhs + (inst->JFETdrainPrimeNode)) += temp;
        *(ckt->CKTirhs + (inst->JFETdrainPrimeNode)) += itemp;

        // capgd over

    // all done

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
