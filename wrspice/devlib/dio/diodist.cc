
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
Authors: 1988 Jaijeet S Roychowdhury
         1992 Stephen R. Whiteley
****************************************************************************/

#define DISTO
#include "diodefs.h"
#include "distdefs.h"

extern int DIOdSetup(sDIOmodel*, sCKT *ckt);


int
DIOdev::disto(int mode, sGENmodel *genmod, sCKT *ckt)
{
    sDIOmodel *model = (sDIOmodel *) genmod;
    sDIOinstance *inst;
    sGENinstance *geninst;
    sDISTOAN* job = (sDISTOAN*) ckt->CKTcurJob;
    double g2,g3;
    double cdiff2,cdiff3;
    double cjunc2,cjunc3;
    double r1h1x,i1h1x;
    double r1h2x, i1h2x;
    double i1hm2x;
    double r2h11x,i2h11x;
    double r2h1m2x,i2h1m2x;
    double temp, itemp;

    if (mode == D_SETUP)
        return (dSetup(model, ckt));

    if ((mode == D_TWOF1) || (mode == D_THRF1) || 
        (mode == D_F1PF2) || (mode == D_F1MF2) ||
        (mode == D_2F1MF2)) {

        for ( ; genmod != 0; genmod = genmod->GENnextModel) {
            model = (sDIOmodel*)genmod;
            for (geninst = genmod->GENinstances; geninst != 0; 
                    geninst = geninst->GENnextInstance) {
                inst = (sDIOinstance*)geninst;

                // loading starts here

                switch (mode) {

                case D_TWOF1:
                    g2 = inst->id_x2;

                    cdiff2 = inst->cdif_x2;
                     
                    cjunc2 = inst->cjnc_x2;

                    // getting first order (linear) Volterra kernel
                    r1h1x = *(job->r1H1ptr + (inst->DIOposPrimeNode)) -
                        *(job->r1H1ptr + (inst->DIOnegNode));
                    i1h1x = *(job->i1H1ptr + (inst->DIOposPrimeNode)) -
                        *(job->i1H1ptr + (inst->DIOnegNode));

                    // formulae start here

                    temp = D1n2F1(g2,r1h1x,i1h1x);
                    itemp = D1i2F1(g2,r1h1x,i1h1x);

                    // the above are for the memoryless nonlinearity

                    if ((cdiff2 + cjunc2) != 0.0) {
                        temp +=  - ckt->CKTomega * D1i2F1
                            (cdiff2+cjunc2,r1h1x,i1h1x);
                        itemp += ckt->CKTomega * D1n2F1
                            ((cdiff2 + cjunc2),r1h1x,i1h1x);
                    }

                    *(ckt->CKTrhs + (inst->DIOposPrimeNode)) -= temp;
                    *(ckt->CKTirhs + (inst->DIOposPrimeNode)) -= itemp;
                    *(ckt->CKTrhs + (inst->DIOnegNode)) += temp;
                    *(ckt->CKTirhs + (inst->DIOnegNode)) += itemp;

                    break;

                case D_THRF1:

                    g2 = inst->id_x2;
                    g3 = inst->id_x3;

                    cdiff2 = inst->cdif_x2;
                    cdiff3 = inst->cdif_x3;
                     
                    cjunc2 = inst->cjnc_x2;
                    cjunc3 = inst->cjnc_x3;

                    // getting first order (linear) Volterra kernel
                    r1h1x = *(job->r1H1ptr + (inst->DIOposPrimeNode)) -
                        *(job->r1H1ptr + (inst->DIOnegNode));
                    i1h1x = *(job->i1H1ptr + (inst->DIOposPrimeNode)) -
                        *(job->i1H1ptr + (inst->DIOnegNode));

                    // getting second order kernel at (F1_F1)
                    r2h11x = *(job->r2H11ptr + (inst->DIOposPrimeNode)) -
                        *(job->r2H11ptr + (inst->DIOnegNode));
                    i2h11x = *(job->i2H11ptr + (inst->DIOposPrimeNode)) -
                        *(job->i2H11ptr + (inst->DIOnegNode));

                    // formulae start here

                    temp = D1n3F1(g2,g3,r1h1x,i1h1x,r2h11x,
                                    i2h11x);
                    itemp = D1i3F1(g2,g3,r1h1x,i1h1x,r2h11x,
                                    i2h11x);

                    // the above are for the memoryless nonlinearity
                    // the following are for the capacitors

                    if ((cdiff2 + cjunc2) != 0.0) {
                        temp += -ckt->CKTomega * D1i3F1
                            (cdiff2+cjunc2,cdiff3+cjunc3,r1h1x,
                                    i1h1x,r2h11x,i2h11x);

                        itemp += ckt->CKTomega * D1n3F1
                            (cdiff2+cjunc2,cdiff3+cjunc3,r1h1x,
                                    i1h1x,r2h11x,i2h11x);
                    }

                    // end of formulae

                    *(ckt->CKTrhs + (inst->DIOposPrimeNode)) -= temp;
                    *(ckt->CKTirhs + (inst->DIOposPrimeNode)) -= itemp;
                    *(ckt->CKTrhs + (inst->DIOnegNode)) += temp;
                    *(ckt->CKTirhs + (inst->DIOnegNode)) += itemp;

                    break;

                case D_F1PF2:

                    g2 = inst->id_x2;
                    g3 = inst->id_x3;

                    cdiff2 = inst->cdif_x2;
                    cdiff3 = inst->cdif_x3;
                     
                    cjunc2 = inst->cjnc_x2;
                    cjunc3 = inst->cjnc_x3;

                    // getting first order (linear) Volterra kernel for F1
                    r1h1x = *(job->r1H1ptr + (inst->DIOposPrimeNode)) -
                        *(job->r1H1ptr + (inst->DIOnegNode));
                    i1h1x = *(job->i1H1ptr + (inst->DIOposPrimeNode)) -
                        *(job->i1H1ptr + (inst->DIOnegNode));

                    // getting first order (linear) Volterra kernel for F2
                    r1h2x = *(job->r1H2ptr + (inst->DIOposPrimeNode)) -
                        *(job->r1H2ptr + (inst->DIOnegNode));
                    i1h2x = *(job->i1H2ptr + (inst->DIOposPrimeNode)) -
                        *(job->i1H2ptr + (inst->DIOnegNode));

                    // formulae start here

                    temp = D1nF12(g2,r1h1x,i1h1x,r1h2x,i1h2x);
                    itemp = D1iF12(g2,r1h1x,i1h1x,r1h2x,i1h2x);

                    // the above are for the memoryless nonlinearity
                    // the following are for the capacitors

                    if ((cdiff2 + cjunc2) != 0.0) {
                        temp += - ckt->CKTomega * D1iF12
                            (cdiff2+cjunc2,r1h1x,i1h1x,r1h2x,i1h2x);
                        itemp += ckt->CKTomega * D1nF12
                            (cdiff2+cjunc2,r1h1x,i1h1x,r1h2x,i1h2x);
                    }

                    // end of formulae

                    *(ckt->CKTrhs + (inst->DIOposPrimeNode)) -= temp;
                    *(ckt->CKTirhs + (inst->DIOposPrimeNode)) -= itemp;
                    *(ckt->CKTrhs + (inst->DIOnegNode)) += temp;
                    *(ckt->CKTirhs + (inst->DIOnegNode)) += itemp;

                    break;

                case D_F1MF2:

                    g2 = inst->id_x2;
                    g3 = inst->id_x3;

                    cdiff2 = inst->cdif_x2;
                    cdiff3 = inst->cdif_x3;
                     
                    cjunc2 = inst->cjnc_x2;
                    cjunc3 = inst->cjnc_x3;

                    // getting first order (linear) Volterra kernel for F1
                    r1h1x = *(job->r1H1ptr + (inst->DIOposPrimeNode)) -
                        *(job->r1H1ptr + (inst->DIOnegNode));
                    i1h1x = *(job->i1H1ptr + (inst->DIOposPrimeNode)) -
                        *(job->i1H1ptr + (inst->DIOnegNode));

                    // getting first order (linear) Volterra kernel for F2
                    r1h2x = *(job->r1H2ptr + (inst->DIOposPrimeNode)) -
                        *(job->r1H2ptr + (inst->DIOnegNode));
                    i1hm2x = -(*(job->i1H2ptr + (inst->DIOposPrimeNode)) -
                        *(job->i1H2ptr + (inst->DIOnegNode)));

                    // formulae start here

                    temp = D1nF12(g2,r1h1x,i1h1x,r1h2x,i1hm2x);
                    itemp = D1iF12(g2,r1h1x,i1h1x,r1h2x,i1hm2x);

                    // the above are for the memoryless nonlinearity
                    // the following are for the capacitors

                    if ((cdiff2 + cjunc2) != 0.0) {
                        temp += - ckt->CKTomega * D1iF12
                            (cdiff2+cjunc2,r1h1x,i1h1x,r1h2x,i1hm2x);
                        itemp += ckt->CKTomega * D1nF12
                            (cdiff2+cjunc2,r1h1x,i1h1x,r1h2x,i1hm2x);
                    }

                    // end of formulae

                    *(ckt->CKTrhs + (inst->DIOposPrimeNode)) -= temp;
                    *(ckt->CKTirhs + (inst->DIOposPrimeNode)) -= itemp;
                    *(ckt->CKTrhs + (inst->DIOnegNode)) += temp;
                    *(ckt->CKTirhs + (inst->DIOnegNode)) += itemp;

                    break;

                case D_2F1MF2:

                    g2 = inst->id_x2;
                    g3 = inst->id_x3;

                    cdiff2 = inst->cdif_x2;
                    cdiff3 = inst->cdif_x3;
                     
                    cjunc2 = inst->cjnc_x2;
                    cjunc3 = inst->cjnc_x3;

                    // getting first order (linear) Volterra kernel at F1
                    r1h1x = *(job->r1H1ptr + (inst->DIOposPrimeNode)) -
                        *(job->r1H1ptr + (inst->DIOnegNode));
                    i1h1x = *(job->i1H1ptr + (inst->DIOposPrimeNode)) -
                        *(job->i1H1ptr + (inst->DIOnegNode));

                    // getting first order (linear) Volterra kernel at minusF2
                    r1h2x = *(job->r1H2ptr + (inst->DIOposPrimeNode)) -
                        *(job->r1H2ptr + (inst->DIOnegNode));
                    i1hm2x = -(*(job->i1H2ptr + (inst->DIOposPrimeNode)) -
                        *(job->i1H2ptr + (inst->DIOnegNode)));

                    // getting second order kernel at (F1_F1)
                    r2h11x = *(job->r2H11ptr + (inst->DIOposPrimeNode)) -
                        *(job->r2H11ptr + (inst->DIOnegNode));
                    i2h11x = *(job->i2H11ptr + (inst->DIOposPrimeNode)) -
                        *(job->i2H11ptr + (inst->DIOnegNode));

                    // getting second order kernel at (F1_minusF2)
                    r2h1m2x = *(job->r2H1m2ptr + (inst->DIOposPrimeNode)) -
                        *(job->r2H1m2ptr + (inst->DIOnegNode));
                    i2h1m2x = *(job->i2H1m2ptr + (inst->DIOposPrimeNode)) -
                        *(job->i2H1m2ptr + (inst->DIOnegNode));

                    // formulae start here

                    temp = D1n2F12(g2,g3,r1h1x,i1h1x,r1h2x,
                            i1hm2x,r2h11x,i2h11x,
                                r2h1m2x,i2h1m2x);
                    itemp = D1i2F12(g2,g3,r1h1x,i1h1x,
                            r1h2x,i1hm2x,r2h11x,i2h11x,
                                r2h1m2x,i2h1m2x);

                    // the above are for the memoryless nonlinearity
                    // the following are for the capacitors

                    if ((cdiff2 + cjunc2) != 0.0) {
                        temp += -ckt->CKTomega * 
                            D1i2F12(cdiff2+cjunc2,cdiff3+cjunc3,
                            r1h1x,i1h1x,r1h2x,i1hm2x,r2h11x,
                                i2h11x,r2h1m2x,i2h1m2x);
                        itemp += ckt->CKTomega *
                            D1n2F12(cdiff2+cjunc2,cdiff3+cjunc3,
                            r1h1x,i1h1x,r1h2x,i1hm2x,r2h11x,
                                i2h11x,r2h1m2x,i2h1m2x);
                    }

                    // end of formulae

                    *(ckt->CKTrhs + (inst->DIOposPrimeNode)) -= temp;
                    *(ckt->CKTirhs + (inst->DIOposPrimeNode)) -= itemp;
                    *(ckt->CKTrhs + (inst->DIOnegNode)) += temp;
                    *(ckt->CKTirhs + (inst->DIOnegNode)) += itemp;

                    break;

                default:
                    ;
                }
            }
        }
        return(OK);
    }
    else
        return(E_BADPARM);
}
