
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
 $Id: cosderiv.cc,v 1.0 1998/01/30 05:29:29 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1989 Jaijeet S. Roychowdhury
**********/

#include <math.h>
#include "distdefs.h"


// CosDeriv computes the partial derivatives of the cosine
// function where the argument to the function is itself a
// function of three variables p, q, and r.
//
void
CosDeriv(Dderivs *newd, Dderivs *old)
{
    Dderivs temp;
    double s, c;

    EqualDeriv(&temp, old);
    s = sin(temp.value);
    c = cos(temp.value);

    newd->value = c;
    newd->d1_p = - s*temp.d1_p;
    newd->d1_q = - s*temp.d1_q;
    newd->d1_r = - s*temp.d1_r;
    newd->d2_p2 = -(c*temp.d1_p*temp.d1_p + s*temp.d2_p2);
    newd->d2_q2 = -(c*temp.d1_q*temp.d1_q + s*temp.d2_q2);
    newd->d2_r2 = -(c*temp.d1_r*temp.d1_r + s*temp.d2_r2);
    newd->d2_pq = -(c*temp.d1_p*temp.d1_q + s*temp.d2_pq);
    newd->d2_qr = -(c*temp.d1_q*temp.d1_r + s*temp.d2_qr);
    newd->d2_pr = -(c*temp.d1_p*temp.d1_r + s*temp.d2_pr);
    newd->d3_p3 = -(s*(temp.d3_p3 - temp.d1_p*temp.d1_p*temp.d1_p)
        + c*(temp.d1_p*temp.d2_p2 + temp.d1_p*temp.d2_p2
        + temp.d1_p*temp.d2_p2));
    newd->d3_q3 = -(s*(temp.d3_q3 - temp.d1_q*temp.d1_q*temp.d1_q)
        + c*(temp.d1_q*temp.d2_q2 + temp.d1_q*temp.d2_q2
        + temp.d1_q*temp.d2_q2));
    newd->d3_r3 = -(s*(temp.d3_r3 - temp.d1_r*temp.d1_r*temp.d1_r)
        + c*(temp.d1_r*temp.d2_r2 + temp.d1_r*temp.d2_r2
        + temp.d1_r*temp.d2_r2));
    newd->d3_p2r = -(s*(temp.d3_p2r - temp.d1_r*temp.d1_p*temp.d1_p)
        + c*(temp.d1_p*temp.d2_pr + temp.d1_p*temp.d2_pr
        + temp.d1_r*temp.d2_p2));
    newd->d3_p2q = -(s*(temp.d3_p2q - temp.d1_q*temp.d1_p*temp.d1_p)
        + c*(temp.d1_p*temp.d2_pq + temp.d1_p*temp.d2_pq
        + temp.d1_q*temp.d2_p2));
    newd->d3_q2r = -(s*(temp.d3_q2r - temp.d1_r*temp.d1_q*temp.d1_q)
        + c*(temp.d1_q*temp.d2_qr + temp.d1_q*temp.d2_qr
        + temp.d1_r*temp.d2_q2));
    newd->d3_pq2 = -(s*(temp.d3_pq2 - temp.d1_p*temp.d1_q*temp.d1_q)
        + c*(temp.d1_q*temp.d2_pq + temp.d1_q*temp.d2_pq
        + temp.d1_p*temp.d2_q2));
    newd->d3_pr2 = -(s*(temp.d3_pr2 - temp.d1_p*temp.d1_r*temp.d1_r)
        + c*(temp.d1_r*temp.d2_pr + temp.d1_r*temp.d2_pr
        + temp.d1_p*temp.d2_r2));
    newd->d3_qr2 = -(s*(temp.d3_qr2 - temp.d1_q*temp.d1_r*temp.d1_r)
        + c*(temp.d1_r*temp.d2_qr + temp.d1_r*temp.d2_qr
        + temp.d1_q*temp.d2_r2));
    newd->d3_pqr = -(s*(temp.d3_pqr - temp.d1_r*temp.d1_p*temp.d1_q)
        + c*(temp.d1_q*temp.d2_pr + temp.d1_p*temp.d2_qr
        + temp.d1_r*temp.d2_pq));
}
