
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


// TanDeriv computes the partial derivatives of the tangent
// function where the argument to the function is itself a
// function of three variables p, q, and r.
//
void
TanDeriv(Dderivs *newd, Dderivs *old)
{
    Dderivs temp;
    double k, kq;

    EqualDeriv(&temp, old);
    k = tan(temp.value);
    newd->value = k;
    kq = (1 + k*k);

    newd->d1_p = kq*temp.d1_p;
    newd->d1_q = kq*temp.d1_q;
    newd->d1_r = kq*temp.d1_r;
    newd->d2_p2 = kq*temp.d2_p2 + 2*newd->value*temp.d1_p*newd->d1_p;
    newd->d2_q2 = kq*temp.d2_q2 + 2*newd->value*temp.d1_q*newd->d1_q;
    newd->d2_r2 = kq*temp.d2_r2 + 2*newd->value*temp.d1_r*newd->d1_r;
    newd->d2_pq = kq*temp.d2_pq + 2*newd->value*temp.d1_p*newd->d1_q;
    newd->d2_qr = kq*temp.d2_qr + 2*newd->value*temp.d1_q*newd->d1_r;
    newd->d2_pr = kq*temp.d2_pr + 2*newd->value*temp.d1_p*newd->d1_r;
    newd->d3_p3 = kq*temp.d3_p3 +2*(newd->value*(
        temp.d2_p2*newd->d1_p + temp.d2_p2*newd->d1_p + newd->d2_p2*
        temp.d1_p) + temp.d1_p*newd->d1_p*newd->d1_p);
    newd->d3_q3 = kq*temp.d3_q3 +2*(newd->value*(
        temp.d2_q2*newd->d1_q + temp.d2_q2*newd->d1_q + newd->d2_q2*
        temp.d1_q) + temp.d1_q*newd->d1_q*newd->d1_q);
    newd->d3_r3 = kq*temp.d3_r3 +2*(newd->value*(
        temp.d2_r2*newd->d1_r + temp.d2_r2*newd->d1_r + newd->d2_r2*
        temp.d1_r) + temp.d1_r*newd->d1_r*newd->d1_r);
    newd->d3_p2r = kq*temp.d3_p2r +2*(newd->value*(
        temp.d2_p2*newd->d1_r + temp.d2_pr*newd->d1_p + newd->d2_pr*
        temp.d1_p) + temp.d1_p*newd->d1_p*newd->d1_r);
    newd->d3_p2q = kq*temp.d3_p2q +2*(newd->value*(
        temp.d2_p2*newd->d1_q + temp.d2_pq*newd->d1_p + newd->d2_pq*
        temp.d1_p) + temp.d1_p*newd->d1_p*newd->d1_q);
    newd->d3_q2r = kq*temp.d3_q2r +2*(newd->value*(
        temp.d2_q2*newd->d1_r + temp.d2_qr*newd->d1_q + newd->d2_qr*
        temp.d1_q) + temp.d1_q*newd->d1_q*newd->d1_r);
    newd->d3_pq2 = kq*temp.d3_pq2 +2*(newd->value*(
        temp.d2_q2*newd->d1_p + temp.d2_pq*newd->d1_q + newd->d2_pq*
        temp.d1_q) + temp.d1_q*newd->d1_q*newd->d1_p);
    newd->d3_pr2 = kq*temp.d3_pr2 +2*(newd->value*(
        temp.d2_r2*newd->d1_p + temp.d2_pr*newd->d1_r + newd->d2_pr*
        temp.d1_r) + temp.d1_r*newd->d1_r*newd->d1_p);
    newd->d3_qr2 = kq*temp.d3_qr2 +2*(newd->value*(
        temp.d2_r2*newd->d1_q + temp.d2_qr*newd->d1_r + newd->d2_qr*
        temp.d1_r) + temp.d1_r*newd->d1_r*newd->d1_q);
    newd->d3_pqr = kq*temp.d3_pqr +2*(newd->value*(
        temp.d2_pq*newd->d1_r + temp.d2_pr*newd->d1_q + newd->d2_qr*
        temp.d1_p) + temp.d1_p*newd->d1_q*newd->d1_r);
}
