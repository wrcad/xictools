
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
 $Id: equalder.cc,v 1.0 1998/01/30 05:29:29 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1989 Jaijeet S. Roychowdhury
**********/

#include <math.h>
#include "distdefs.h"


// EqualDeriv equates partial derivatives.
//
void 
EqualDeriv(Dderivs *newd, Dderivs *old)
{
    newd->value = old->value;
    newd->d1_p = old->d1_p;
    newd->d1_q = old->d1_q;
    newd->d1_r = old->d1_r;
    newd->d2_p2 = old->d2_p2;
    newd->d2_q2 = old->d2_q2;
    newd->d2_r2 = old->d2_r2;
    newd->d2_pq = old->d2_pq;
    newd->d2_qr = old->d2_qr;
    newd->d2_pr = old->d2_pr;
    newd->d3_p3 = old->d3_p3;
    newd->d3_q3 = old->d3_q3;
    newd->d3_r3 = old->d3_r3;
    newd->d3_p2r = old->d3_p2r;
    newd->d3_p2q = old->d3_p2q;
    newd->d3_q2r = old->d3_q2r;
    newd->d3_pq2 = old->d3_pq2;
    newd->d3_pr2 = old->d3_pr2;
    newd->d3_qr2 = old->d3_qr2;
    newd->d3_pqr = old->d3_pqr;
}
