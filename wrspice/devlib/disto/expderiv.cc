
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


// ExpDeriv computes the partial derivatives of the exponential
// function where the argument to the function is itself a
// function of three variables p, q, and r.
//
void
ExpDeriv(Dderivs *newd, Dderivs *old)
{
    Dderivs temp;
    double et;

    EqualDeriv(&temp, old);
    et = exp(temp.value);
    newd->value = et;
    newd->d1_p = et*temp.d1_p;
    newd->d1_q = et*temp.d1_q;
    newd->d1_r = et*temp.d1_r;
    newd->d2_p2 = et*temp.d2_p2 + temp.d1_p*newd->d1_p;
    newd->d2_q2 = et*temp.d2_q2 + temp.d1_q*newd->d1_q;
    newd->d2_r2 = et*temp.d2_r2 + temp.d1_r*newd->d1_r;
    newd->d2_pq = et*temp.d2_pq + temp.d1_p*newd->d1_q;
    newd->d2_qr = et*temp.d2_qr + temp.d1_q*newd->d1_r;
    newd->d2_pr = et*temp.d2_pr + temp.d1_p*newd->d1_r;
    newd->d3_p3 = et*temp.d3_p3 + temp.d2_p2*newd->d1_p
        + temp.d2_p2*newd->d1_p + newd->d2_p2*temp.d1_p;
    newd->d3_q3 = et*temp.d3_q3 + temp.d2_q2*newd->d1_q
        + temp.d2_q2*newd->d1_q + newd->d2_q2*temp.d1_q;
    newd->d3_r3 = et*temp.d3_r3 + temp.d2_r2*newd->d1_r
        + temp.d2_r2*newd->d1_r + newd->d2_r2*temp.d1_r;
    newd->d3_p2r = et*temp.d3_p2r + temp.d2_p2*newd->d1_r
        + temp.d2_pr*newd->d1_p + newd->d2_pr*temp.d1_p;
    newd->d3_p2q = et*temp.d3_p2q + temp.d2_p2*newd->d1_q
        + temp.d2_pq*newd->d1_p + newd->d2_pq*temp.d1_p;
    newd->d3_q2r = et*temp.d3_q2r + temp.d2_q2*newd->d1_r
        + temp.d2_qr*newd->d1_q + newd->d2_qr*temp.d1_q;
    newd->d3_pq2 = et*temp.d3_pq2 + temp.d2_q2*newd->d1_p
        + temp.d2_pq*newd->d1_q + newd->d2_pq*temp.d1_q;
    newd->d3_pr2 = et*temp.d3_pr2 + temp.d2_r2*newd->d1_p
        + temp.d2_pr*newd->d1_r + newd->d2_pr*temp.d1_r;
    newd->d3_qr2 = et*temp.d3_qr2 + temp.d2_r2*newd->d1_q
        + temp.d2_qr*newd->d1_r + newd->d2_qr*temp.d1_r;
    newd->d3_pqr = et*temp.d3_pqr + temp.d2_pq*newd->d1_r
        + temp.d2_pr*newd->d1_q + newd->d2_qr*temp.d1_p;
}
