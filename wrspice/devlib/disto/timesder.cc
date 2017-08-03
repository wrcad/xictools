
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


// TimesDeriv computes the partial derivatives of the x*k
// function where the argument to the function is itself a
// function of three variables p, q, and r. k is a constant.
//
void
TimesDeriv(Dderivs *newd, Dderivs *old, double k)
{
    newd->value = k* old->value;
    newd->d1_p = k*old->d1_p;
    newd->d1_q = k*old->d1_q;
    newd->d1_r = k*old->d1_r;
    newd->d2_p2 = k*old->d2_p2;
    newd->d2_q2 = k*old->d2_q2;
    newd->d2_r2 = k*old->d2_r2;
    newd->d2_pq = k*old->d2_pq;
    newd->d2_qr = k*old->d2_qr;
    newd->d2_pr = k*old->d2_pr;
    newd->d3_p3 = k*old->d3_p3;
    newd->d3_q3 = k*old->d3_q3;
    newd->d3_r3 = k*old->d3_r3;
    newd->d3_p2r = k*old->d3_p2r;
    newd->d3_p2q = k*old->d3_p2q;
    newd->d3_q2r = k*old->d3_q2r;
    newd->d3_pq2 = k*old->d3_pq2;
    newd->d3_pr2 = k*old->d3_pr2;
    newd->d3_qr2 = k*old->d3_qr2;
    newd->d3_pqr = k*old->d3_pqr;
}
