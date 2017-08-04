
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

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1989 Jaijeet S. Roychowdhury
**********/

#include <math.h>
#include "distdefs.h"


// AtanDeriv computes the partial derivatives of the arctangent
// function where the argument to the atan function is itself a
// function of three variables p, q, and r.
//
void
AtanDeriv(Dderivs *newd, Dderivs *old)
{
    Dderivs temp;
    double k;

    EqualDeriv(&temp, old);

    newd->value = atan(temp.value);
    k = 1.0 + temp.value*temp.value;
    newd->d1_p = temp.d1_p/k;
    newd->d1_q = temp.d1_q/k;
    newd->d1_r = temp.d1_r/k;
    newd->d2_p2 = temp.d2_p2/k - 2*temp.value*newd->d1_p*newd->d1_p;
    newd->d2_q2 = temp.d2_q2/k - 2*temp.value*newd->d1_q*newd->d1_q;
    newd->d2_r2 = temp.d2_r2/k - 2*temp.value*newd->d1_r*newd->d1_r;
    newd->d2_pq = temp.d2_pq/k - 2*temp.value*newd->d1_p*newd->d1_q;
    newd->d2_qr = temp.d2_qr/k - 2*temp.value*newd->d1_q*newd->d1_r;
    newd->d2_pr = temp.d2_pr/k - 2*temp.value*newd->d1_p*newd->d1_r;
    newd->d3_p3 = (temp.d3_p3 - temp.d2_p2*newd->d1_p*2*temp.value)/k -
        2*(newd->d1_p*newd->d1_p*temp.d1_p + temp.value*(
        newd->d2_p2*newd->d1_p + newd->d2_p2*newd->d1_p));
    newd->d3_q3 = (temp.d3_q3 - temp.d2_q2*newd->d1_q*2*temp.value)/k -
        2*(newd->d1_q*newd->d1_q*temp.d1_q + temp.value*(
        newd->d2_q2*newd->d1_q + newd->d2_q2*newd->d1_q));
    newd->d3_r3 = (temp.d3_r3 - temp.d2_r2*newd->d1_r*2*temp.value)/k -
        2*(newd->d1_r*newd->d1_r*temp.d1_r + temp.value*(
        newd->d2_r2*newd->d1_r + newd->d2_r2*newd->d1_r));
    newd->d3_p2r = (temp.d3_p2r - temp.d2_p2*newd->d1_r*2*temp.value)/k -
        2*(newd->d1_p*newd->d1_p*temp.d1_r + temp.value*(
        newd->d2_pr*newd->d1_p + newd->d2_pr*newd->d1_p));
    newd->d3_p2q = (temp.d3_p2q - temp.d2_p2*newd->d1_q*2*temp.value)/k -
        2*(newd->d1_p*newd->d1_p*temp.d1_q + temp.value*(
        newd->d2_pq*newd->d1_p + newd->d2_pq*newd->d1_p));
    newd->d3_q2r = (temp.d3_q2r - temp.d2_q2*newd->d1_r*2*temp.value)/k -
        2*(newd->d1_q*newd->d1_q*temp.d1_r + temp.value*(
        newd->d2_qr*newd->d1_q + newd->d2_qr*newd->d1_q));
    newd->d3_pq2 = (temp.d3_pq2 - temp.d2_q2*newd->d1_p*2*temp.value)/k -
        2*(newd->d1_q*newd->d1_q*temp.d1_p + temp.value*(
        newd->d2_pq*newd->d1_q + newd->d2_pq*newd->d1_q));
    newd->d3_pr2 = (temp.d3_pr2 - temp.d2_r2*newd->d1_p*2*temp.value)/k -
        2*(newd->d1_r*newd->d1_r*temp.d1_p + temp.value*(
        newd->d2_pr*newd->d1_r + newd->d2_pr*newd->d1_r));
    newd->d3_qr2 = (temp.d3_qr2 - temp.d2_r2*newd->d1_q*2*temp.value)/k -
        2*(newd->d1_r*newd->d1_r*temp.d1_q + temp.value*(
        newd->d2_qr*newd->d1_r + newd->d2_qr*newd->d1_r));
    newd->d3_pqr = (temp.d3_pqr - temp.d2_pq*newd->d1_r*2*temp.value)/k -
        2*(newd->d1_p*newd->d1_q*temp.d1_r + temp.value*(
        newd->d2_pr*newd->d1_q + newd->d2_qr*newd->d1_p));
}
