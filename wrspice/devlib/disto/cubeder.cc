
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
 $Id: cubeder.cc,v 1.0 1998/01/30 05:29:29 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1989 Jaijeet S. Roychowdhury
**********/

#include <math.h>
#include "distdefs.h"


// CubeDeriv computes the partial derivatives of the cube
// function where the argument to the function is itself a
// function of three variables p, q, and r.
//
void
CubeDeriv(Dderivs *newd, Dderivs *old)
{
    Dderivs temp;
    double tt;

    EqualDeriv(&temp, old);
    tt = temp.value*temp.value;

    newd->value = tt*temp.value;
    newd->d1_p = 3*tt*temp.d1_p;
    newd->d1_q = 3*tt*temp.d1_q;
    newd->d1_r = 3*tt*temp.d1_r;
    newd->d2_p2 = 3*(2*temp.value*temp.d1_p*temp.d1_p + tt*temp.d2_p2);
    newd->d2_q2 = 3*(2*temp.value*temp.d1_q*temp.d1_q + tt*temp.d2_q2);
    newd->d2_r2 = 3*(2*temp.value*temp.d1_r*temp.d1_r + tt*temp.d2_r2);
    newd->d2_pq = 3*(2*temp.value*temp.d1_p*temp.d1_q + tt*temp.d2_pq);
    newd->d2_qr = 3*(2*temp.value*temp.d1_q*temp.d1_r + tt*temp.d2_qr);
    newd->d2_pr = 3*(2*temp.value*temp.d1_p*temp.d1_r + tt*temp.d2_pr);
    newd->d3_p3 = 3*(2*(temp.d1_p*temp.d1_p*temp.d1_p + temp.value*
        (temp.d2_p2*temp.d1_p + temp.d2_p2*temp.d1_p + temp.d2_p2*temp.d1_p))
        + tt*temp.d3_p3);
    newd->d3_q3 = 3*(2*(temp.d1_q*temp.d1_q*temp.d1_q + temp.value*
        (temp.d2_q2*temp.d1_q + temp.d2_q2*temp.d1_q + temp.d2_q2*temp.d1_q))
        + tt*temp.d3_q3);
    newd->d3_r3 = 3*(2*(temp.d1_r*temp.d1_r*temp.d1_r + temp.value*
        (temp.d2_r2*temp.d1_r + temp.d2_r2*temp.d1_r + temp.d2_r2*temp.d1_r))
        + tt*temp.d3_r3);
    newd->d3_p2r = 3*(2*(temp.d1_p*temp.d1_p*temp.d1_r + temp.value*
        (temp.d2_p2*temp.d1_r + temp.d2_pr*temp.d1_p + temp.d2_pr*temp.d1_p))
        + tt*temp.d3_p2r);
    newd->d3_p2q = 3*(2*(temp.d1_p*temp.d1_p*temp.d1_q + temp.value*
        (temp.d2_p2*temp.d1_q + temp.d2_pq*temp.d1_p + temp.d2_pq*temp.d1_p))
        + tt*temp.d3_p2q);
    newd->d3_q2r = 3*(2*(temp.d1_q*temp.d1_q*temp.d1_r + temp.value*
        (temp.d2_q2*temp.d1_r + temp.d2_qr*temp.d1_q + temp.d2_qr*temp.d1_q))
        + tt*temp.d3_q2r);
    newd->d3_pq2 = 3*(2*(temp.d1_q*temp.d1_q*temp.d1_p + temp.value*
        (temp.d2_q2*temp.d1_p + temp.d2_pq*temp.d1_q + temp.d2_pq*temp.d1_q))
        + tt*temp.d3_pq2);
    newd->d3_pr2 = 3*(2*(temp.d1_r*temp.d1_r*temp.d1_p + temp.value*
        (temp.d2_r2*temp.d1_p + temp.d2_pr*temp.d1_r + temp.d2_pr*temp.d1_r))
        + tt*temp.d3_pr2);
    newd->d3_qr2 = 3*(2*(temp.d1_r*temp.d1_r*temp.d1_q + temp.value*
        (temp.d2_r2*temp.d1_q + temp.d2_qr*temp.d1_r + temp.d2_qr*temp.d1_r))
        + tt*temp.d3_qr2);
    newd->d3_pqr = 3*(2*(temp.d1_p*temp.d1_q*temp.d1_r + temp.value*
        (temp.d2_pq*temp.d1_r + temp.d2_qr*temp.d1_p + temp.d2_pr*temp.d1_q))
        + tt*temp.d3_pqr);
}
