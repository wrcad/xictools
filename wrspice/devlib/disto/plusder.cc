
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
 $Id: plusder.cc,v 1.0 1998/01/30 05:29:29 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1989 Jaijeet S. Roychowdhury
**********/

#include <math.h>
#include "distdefs.h"


// PlusDeriv computes the partial derivatives of the addition
// function where the arguments to the function are
// functions of three variables p, q, and r.
//
void
PlusDeriv(Dderivs *newd, Dderivs *old1, Dderivs *old2)
{
    newd->value = old1->value + old2->value;
    newd->d1_p = old1->d1_p + old2->d1_p;
    newd->d1_q = old1->d1_q + old2->d1_q;
    newd->d1_r = old1->d1_r + old2->d1_r;
    newd->d2_p2 = old1->d2_p2 + old2->d2_p2;
    newd->d2_q2 = old1->d2_q2 + old2->d2_q2;
    newd->d2_r2 = old1->d2_r2 + old2->d2_r2;
    newd->d2_pq = old1->d2_pq + old2->d2_pq;
    newd->d2_qr = old1->d2_qr + old2->d2_qr;
    newd->d2_pr = old1->d2_pr + old2->d2_pr;
    newd->d3_p3 = old1->d3_p3 + old2->d3_p3;
    newd->d3_q3 = old1->d3_q3 + old2->d3_q3;
    newd->d3_r3 = old1->d3_r3 + old2->d3_r3;
    newd->d3_p2r = old1->d3_p2r + old2->d3_p2r;
    newd->d3_p2q = old1->d3_p2q + old2->d3_p2q;
    newd->d3_q2r = old1->d3_q2r + old2->d3_q2r;
    newd->d3_pq2 = old1->d3_pq2 + old2->d3_pq2;
    newd->d3_pr2 = old1->d3_pr2 + old2->d3_pr2;
    newd->d3_qr2 = old1->d3_qr2 + old2->d3_qr2;
    newd->d3_pqr = old1->d3_pqr + old2->d3_pqr;
}
