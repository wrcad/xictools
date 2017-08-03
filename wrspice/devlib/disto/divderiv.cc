
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


// DivDeriv computes the partial derivatives of the division
// function where the arguments to the function are
// functions of three variables p, q, and r.
//
void
DivDeriv(Dderivs *newd, Dderivs *old1, Dderivs *old2)
{
    Dderivs num, den;

    EqualDeriv(&num, old1);
    EqualDeriv(&den, old2);

    newd->value = num.value/den.value;
    newd->d1_p = (num.d1_p - num.value*den.d1_p/den.value)/den.value;
    newd->d1_q = (num.d1_q - num.value*den.d1_q/den.value)/den.value;
    newd->d1_r = (num.d1_r - num.value*den.d1_r/den.value)/den.value;
    newd->d2_p2 = (num.d2_p2 - den.d1_p*newd->d1_p - newd->value*den.d2_p2
        + den.d1_p*(newd->value*den.d1_p - num.d1_p)/den.value)/den.value;
    newd->d2_q2 = (num.d2_q2 - den.d1_q*newd->d1_q - newd->value*den.d2_q2
        + den.d1_q*(newd->value*den.d1_q - num.d1_q)/den.value)/den.value;
    newd->d2_r2 = (num.d2_r2 - den.d1_r*newd->d1_r - newd->value*den.d2_r2
        + den.d1_r*(newd->value*den.d1_r - num.d1_r)/den.value)/den.value;
    newd->d2_pq = (num.d2_pq - den.d1_q*newd->d1_p - newd->value*den.d2_pq
        + den.d1_p*(newd->value*den.d1_q - num.d1_q)/den.value)/den.value;
    newd->d2_qr = (num.d2_qr - den.d1_r*newd->d1_q - newd->value*den.d2_qr
        + den.d1_q*(newd->value*den.d1_r - num.d1_r)/den.value)/den.value;
    newd->d2_pr = (num.d2_pr - den.d1_r*newd->d1_p - newd->value*den.d2_pr
        + den.d1_p*(newd->value*den.d1_r - num.d1_r)/den.value)/den.value;
    newd->d3_p3 = (-den.d1_p*newd->d2_p2 + num.d3_p3 -den.d2_p2*newd->d1_p
        - den.d1_p*newd->d2_p2 - newd->d1_p*den.d2_p2 - newd->value*den.d3_p3 + 
        (den.d1_p*(newd->d1_p*den.d1_p + newd->value*den.d2_p2 - num.d2_p2) +
        (newd->value*den.d1_p - num.d1_p)*(den.d2_p2 - den.d1_p*den.d1_p/
        den.value))/den.value)/den.value;
    newd->d3_q3 = (-den.d1_q*newd->d2_q2 + num.d3_q3 -den.d2_q2*newd->d1_q
        - den.d1_q*newd->d2_q2 - newd->d1_q*den.d2_q2 - newd->value*den.d3_q3 + 
        (den.d1_q*(newd->d1_q*den.d1_q + newd->value*den.d2_q2 - num.d2_q2) +
        (newd->value*den.d1_q - num.d1_q)*(den.d2_q2 - den.d1_q*den.d1_q/
        den.value))/den.value)/den.value;
    newd->d3_r3 = (-den.d1_r*newd->d2_r2 + num.d3_r3 -den.d2_r2*newd->d1_r
        - den.d1_r*newd->d2_r2 - newd->d1_r*den.d2_r2 - newd->value*den.d3_r3 + 
        (den.d1_r*(newd->d1_r*den.d1_r + newd->value*den.d2_r2 - num.d2_r2) +
        (newd->value*den.d1_r - num.d1_r)*(den.d2_r2 - den.d1_r*den.d1_r/
        den.value))/den.value)/den.value;
    newd->d3_p2r = (-den.d1_r*newd->d2_p2 + num.d3_p2r -den.d2_pr*newd->d1_p
        - den.d1_p*newd->d2_pr - newd->d1_r*den.d2_p2 - newd->value*den.d3_p2r + 
        (den.d1_p*(newd->d1_r*den.d1_p + newd->value*den.d2_pr - num.d2_pr) +
        (newd->value*den.d1_p - num.d1_p)*(den.d2_pr - den.d1_p*den.d1_r/
        den.value))/den.value)/den.value;
    newd->d3_p2q = (-den.d1_q*newd->d2_p2 + num.d3_p2q -den.d2_pq*newd->d1_p
        - den.d1_p*newd->d2_pq - newd->d1_q*den.d2_p2 - newd->value*den.d3_p2q + 
        (den.d1_p*(newd->d1_q*den.d1_p + newd->value*den.d2_pq - num.d2_pq) +
        (newd->value*den.d1_p - num.d1_p)*(den.d2_pq - den.d1_p*den.d1_q/
        den.value))/den.value)/den.value;
    newd->d3_q2r = (-den.d1_r*newd->d2_q2 + num.d3_q2r -den.d2_qr*newd->d1_q
        - den.d1_q*newd->d2_qr - newd->d1_r*den.d2_q2 - newd->value*den.d3_q2r + 
        (den.d1_q*(newd->d1_r*den.d1_q + newd->value*den.d2_qr - num.d2_qr) +
        (newd->value*den.d1_q - num.d1_q)*(den.d2_qr - den.d1_q*den.d1_r/
        den.value))/den.value)/den.value;
    newd->d3_pq2 = (-den.d1_p*newd->d2_q2 + num.d3_pq2 -den.d2_pq*newd->d1_q
        - den.d1_q*newd->d2_pq - newd->d1_p*den.d2_q2 - newd->value*den.d3_pq2 + 
        (den.d1_q*(newd->d1_p*den.d1_q + newd->value*den.d2_pq - num.d2_pq) +
        (newd->value*den.d1_q - num.d1_q)*(den.d2_pq - den.d1_q*den.d1_p/
        den.value))/den.value)/den.value;
    newd->d3_pr2 = (-den.d1_p*newd->d2_r2 + num.d3_pr2 -den.d2_pr*newd->d1_r
        - den.d1_r*newd->d2_pr - newd->d1_p*den.d2_r2 - newd->value*den.d3_pr2 + 
        (den.d1_r*(newd->d1_p*den.d1_r + newd->value*den.d2_pr - num.d2_pr) +
        (newd->value*den.d1_r - num.d1_r)*(den.d2_pr - den.d1_r*den.d1_p/
        den.value))/den.value)/den.value;
    newd->d3_qr2 = (-den.d1_q*newd->d2_r2 + num.d3_qr2 -den.d2_qr*newd->d1_r
        - den.d1_r*newd->d2_qr - newd->d1_q*den.d2_r2 - newd->value*den.d3_qr2 + 
        (den.d1_r*(newd->d1_q*den.d1_r + newd->value*den.d2_qr - num.d2_qr) +
        (newd->value*den.d1_r - num.d1_r)*(den.d2_qr - den.d1_r*den.d1_q/
        den.value))/den.value)/den.value;
    newd->d3_pqr = (-den.d1_r*newd->d2_pq + num.d3_pqr -den.d2_qr*newd->d1_p
        - den.d1_q*newd->d2_pr - newd->d1_r*den.d2_pq - newd->value*den.d3_pqr + 
        (den.d1_p*(newd->d1_r*den.d1_q + newd->value*den.d2_qr - num.d2_qr) +
        (newd->value*den.d1_q - num.d1_q)*(den.d2_pr - den.d1_p*den.d1_r/
        den.value))/den.value)/den.value;
}
