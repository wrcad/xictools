
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
 $Id: invderiv.cc,v 1.0 1998/01/30 05:29:29 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1989 Jaijeet S. Roychowdhury
**********/

#include <math.h>
#include "distdefs.h"


// InvDeriv computes the partial derivatives of the 1/x
// function where the argument to the function is itself a
// function of three variables p, q, and r.
//
void
InvDeriv(Dderivs *newd, Dderivs *old)
{
    Dderivs temp;
    double nv, nq;

    EqualDeriv(&temp, old);

    nv = 1/temp.value;
    newd->value = nv;
    nq = nv*nv;
    newd->d1_p = -nq*temp.d1_p;
    newd->d1_q = -nq*temp.d1_q;
    newd->d1_r = -nq*temp.d1_r;
    newd->d2_p2 = -nv*(2*newd->d1_p*temp.d1_p + nv*temp.d2_p2);
    newd->d2_q2 = -nv*(2*newd->d1_q*temp.d1_q + nv*temp.d2_q2);
    newd->d2_r2 = -nv*(2*newd->d1_r*temp.d1_r + nv*temp.d2_r2);
    newd->d2_pq = -nv*(2*newd->d1_q*temp.d1_p + nv*temp.d2_pq);
    newd->d2_qr = -nv*(2*newd->d1_r*temp.d1_q + nv*temp.d2_qr);
    newd->d2_pr = -nv*(2*newd->d1_r*temp.d1_p + nv*temp.d2_pr);
    newd->d3_p3 = -(2*(temp.d1_p*newd->d1_p*newd->d1_p + nv*(
        newd->d2_p2*temp.d1_p + newd->d1_p*temp.d2_p2 +
        newd->d1_p*temp.d2_p2)) + nq*temp.d3_p3);
    newd->d3_q3 = -(2*(temp.d1_q*newd->d1_q*newd->d1_q + nv*(
        newd->d2_q2*temp.d1_q + newd->d1_q*temp.d2_q2 +
        newd->d1_q*temp.d2_q2)) + nq*temp.d3_q3);
    newd->d3_r3 = -(2*(temp.d1_r*newd->d1_r*newd->d1_r + nv*(
        newd->d2_r2*temp.d1_r + newd->d1_r*temp.d2_r2 +
        newd->d1_r*temp.d2_r2)) + nq*temp.d3_r3);
    newd->d3_p2r = -(2*(temp.d1_p*newd->d1_p*newd->d1_r + nv*(
        newd->d2_pr*temp.d1_p + newd->d1_p*temp.d2_pr +
        newd->d1_r*temp.d2_p2)) + nq*temp.d3_p2r);
    newd->d3_p2q = -(2*(temp.d1_p*newd->d1_p*newd->d1_q + nv*(
        newd->d2_pq*temp.d1_p + newd->d1_p*temp.d2_pq +
        newd->d1_q*temp.d2_p2)) + nq*temp.d3_p2q);
    newd->d3_q2r = -(2*(temp.d1_q*newd->d1_q*newd->d1_r + nv*(
        newd->d2_qr*temp.d1_q + newd->d1_q*temp.d2_qr +
        newd->d1_r*temp.d2_q2)) + nq*temp.d3_q2r);
    newd->d3_pq2 = -(2*(temp.d1_q*newd->d1_q*newd->d1_p + nv*(
        newd->d2_pq*temp.d1_q + newd->d1_q*temp.d2_pq +
        newd->d1_p*temp.d2_q2)) + nq*temp.d3_pq2);
    newd->d3_pr2 = -(2*(temp.d1_r*newd->d1_r*newd->d1_p + nv*(
        newd->d2_pr*temp.d1_r + newd->d1_r*temp.d2_pr +
        newd->d1_p*temp.d2_r2)) + nq*temp.d3_pr2);
    newd->d3_qr2 = -(2*(temp.d1_r*newd->d1_r*newd->d1_q + nv*(
        newd->d2_qr*temp.d1_r + newd->d1_r*temp.d2_qr +
        newd->d1_q*temp.d2_r2)) + nq*temp.d3_qr2);
    newd->d3_pqr = -(2*(temp.d1_p*newd->d1_q*newd->d1_r + nv*(
        newd->d2_qr*temp.d1_p + newd->d1_q*temp.d2_pr +
        newd->d1_r*temp.d2_pq)) + nq*temp.d3_pqr);
}
