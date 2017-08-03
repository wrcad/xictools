
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

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1989 Jaijeet S. Roychowdhury
**********/

#include <math.h>
#include "distdefs.h"


// MultDeriv computes the partial derivatives of the multiplication
// function where the arguments to the function are
// functions of three variables p, q, and r.
//
void
MultDeriv(Dderivs *newd, Dderivs *old1, Dderivs *old2)
{
    Dderivs temp1, temp2;

    EqualDeriv(&temp1, old1);
    EqualDeriv(&temp2, old2);

    newd->value = temp1.value * temp2.value;
    newd->d1_p = temp1.d1_p*temp2.value + temp1.value*temp2.d1_p;
    newd->d1_q = temp1.d1_q*temp2.value + temp1.value*temp2.d1_q;
    newd->d1_r = temp1.d1_r*temp2.value + temp1.value*temp2.d1_r;
    newd->d2_p2 = temp1.d2_p2*temp2.value + temp1.d1_p*temp2.d1_p
        + temp1.d1_p*temp2.d1_p + temp1.value*temp2.d2_p2;
    newd->d2_q2 = temp1.d2_q2*temp2.value + temp1.d1_q*temp2.d1_q
        + temp1.d1_q*temp2.d1_q + temp1.value*temp2.d2_q2;
    newd->d2_r2 = temp1.d2_r2*temp2.value + temp1.d1_r*temp2.d1_r
        + temp1.d1_r*temp2.d1_r + temp1.value*temp2.d2_r2;
    newd->d2_pq = temp1.d2_pq*temp2.value + temp1.d1_p*temp2.d1_q
        + temp1.d1_q*temp2.d1_p + temp1.value*temp2.d2_pq;
    newd->d2_qr = temp1.d2_qr*temp2.value + temp1.d1_q*temp2.d1_r
        + temp1.d1_r*temp2.d1_q + temp1.value*temp2.d2_qr;
    newd->d2_pr = temp1.d2_pr*temp2.value + temp1.d1_p*temp2.d1_r
        + temp1.d1_r*temp2.d1_p + temp1.value*temp2.d2_pr;
    newd->d3_p3 = temp1.d3_p3*temp2.value + temp1.d2_p2*temp2.d1_p
        + temp1.d2_p2*temp2.d1_p + temp2.d2_p2*temp1.d1_p
        + temp2.d2_p2*temp1.d1_p + temp1.d2_p2*temp2.d1_p
        + temp2.d2_p2*temp1.d1_p + temp1.value*temp2.d3_p3;
    newd->d3_q3 = temp1.d3_q3*temp2.value + temp1.d2_q2*temp2.d1_q
        + temp1.d2_q2*temp2.d1_q + temp2.d2_q2*temp1.d1_q
        + temp2.d2_q2*temp1.d1_q + temp1.d2_q2*temp2.d1_q
        + temp2.d2_q2*temp1.d1_q + temp1.value*temp2.d3_q3;
    newd->d3_r3 = temp1.d3_r3*temp2.value + temp1.d2_r2*temp2.d1_r
        + temp1.d2_r2*temp2.d1_r + temp2.d2_r2*temp1.d1_r
        + temp2.d2_r2*temp1.d1_r + temp1.d2_r2*temp2.d1_r
        + temp2.d2_r2*temp1.d1_r + temp1.value*temp2.d3_r3;
    newd->d3_p2r = temp1.d3_p2r*temp2.value + temp1.d2_p2*temp2.d1_r
        + temp1.d2_pr*temp2.d1_p + temp2.d2_p2*temp1.d1_r
        + temp2.d2_pr*temp1.d1_p + temp1.d2_pr*temp2.d1_p
        + temp2.d2_pr*temp1.d1_p + temp1.value*temp2.d3_p2r;
    newd->d3_p2q = temp1.d3_p2q*temp2.value + temp1.d2_p2*temp2.d1_q
        + temp1.d2_pq*temp2.d1_p + temp2.d2_p2*temp1.d1_q
        + temp2.d2_pq*temp1.d1_p + temp1.d2_pq*temp2.d1_p
        + temp2.d2_pq*temp1.d1_p + temp1.value*temp2.d3_p2q;
    newd->d3_q2r = temp1.d3_q2r*temp2.value + temp1.d2_q2*temp2.d1_r
        + temp1.d2_qr*temp2.d1_q + temp2.d2_q2*temp1.d1_r
        + temp2.d2_qr*temp1.d1_q + temp1.d2_qr*temp2.d1_q
        + temp2.d2_qr*temp1.d1_q + temp1.value*temp2.d3_q2r;
    newd->d3_pq2 = temp1.d3_pq2*temp2.value + temp1.d2_q2*temp2.d1_p
        + temp1.d2_pq*temp2.d1_q + temp2.d2_q2*temp1.d1_p
        + temp2.d2_pq*temp1.d1_q + temp1.d2_pq*temp2.d1_q
        + temp2.d2_pq*temp1.d1_q + temp1.value*temp2.d3_pq2;
    newd->d3_pr2 = temp1.d3_pr2*temp2.value + temp1.d2_r2*temp2.d1_p
        + temp1.d2_pr*temp2.d1_r + temp2.d2_r2*temp1.d1_p
        + temp2.d2_pr*temp1.d1_r + temp1.d2_pr*temp2.d1_r
        + temp2.d2_pr*temp1.d1_r + temp1.value*temp2.d3_pr2;
    newd->d3_qr2 = temp1.d3_qr2*temp2.value + temp1.d2_r2*temp2.d1_q
        + temp1.d2_qr*temp2.d1_r + temp2.d2_r2*temp1.d1_q
        + temp2.d2_qr*temp1.d1_r + temp1.d2_qr*temp2.d1_r
        + temp2.d2_qr*temp1.d1_r + temp1.value*temp2.d3_qr2;
    newd->d3_pqr = temp1.d3_pqr*temp2.value + temp1.d2_pq*temp2.d1_r
        + temp1.d2_pr*temp2.d1_q + temp2.d2_pq*temp1.d1_r
        + temp2.d2_qr*temp1.d1_p + temp1.d2_qr*temp2.d1_p
        + temp2.d2_pr*temp1.d1_q + temp1.value*temp2.d3_pqr;
}
