
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
 $Id: sqrtder.cc,v 1.0 1998/01/30 05:29:29 stevew Exp $
 *========================================================================*/

/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1989 Jaijeet S. Roychowdhury
**********/

#include <math.h>
#include "distdefs.h"


// SqrtDeriv computes the partial derivatives of the sqrt
// function where the argument to the function is itself a
// function of three variables p, q, and r.
//
void
SqrtDeriv(Dderivs *newd, Dderivs *old)
{

    Dderivs temp;
    double k;

    EqualDeriv(&temp, old);
    newd->value = sqrt(temp.value);
    if (temp.value == 0.0) {
        newd->d1_p = 0.0;
        newd->d1_q = 0.0;
        newd->d1_r = 0.0;
        newd->d2_p2 = 0.0;
        newd->d2_q2 = 0.0;
        newd->d2_r2 = 0.0;
        newd->d2_pq = 0.0;
        newd->d2_qr = 0.0;
        newd->d2_pr = 0.0;
        newd->d3_p3 = 0.0;
        newd->d3_q3 = 0.0;
        newd->d3_r3 = 0.0;
        newd->d3_p2r = 0.0;
        newd->d3_p2q = 0.0;
        newd->d3_q2r = 0.0;
        newd->d3_pq2 = 0.0;
        newd->d3_pr2 = 0.0;
        newd->d3_qr2 = 0.0;
        newd->d3_pqr = 0.0;
        return;
    }
    k = 0.5/newd->value;
    newd->d1_p = k*temp.d1_p;
    newd->d1_q = k*temp.d1_q;
    newd->d1_r = k*temp.d1_r;
    newd->d2_p2 = k*(temp.d2_p2 - 0.5 * temp.d1_p * temp.d1_p/ temp.value);
    newd->d2_q2 = k*(temp.d2_q2 - 0.5 * temp.d1_q * temp.d1_q/ temp.value);
    newd->d2_r2 = k*(temp.d2_r2 - 0.5 * temp.d1_r * temp.d1_r/ temp.value);
    newd->d2_pq = k*(temp.d2_pq - 0.5 * temp.d1_p * temp.d1_q/ temp.value);
    newd->d2_qr = k*(temp.d2_qr - 0.5 * temp.d1_q * temp.d1_r/ temp.value);
    newd->d2_pr = k*(temp.d2_pr - 0.5 * temp.d1_p * temp.d1_r/ temp.value);
    newd->d3_p3 = 0.5*(temp.d3_p3/newd->value - 0.5/(temp.value*newd->value)*
        (-1.5/temp.value*temp.d1_p*temp.d1_p*temp.d1_p + temp.d1_p*temp.d2_p2 + 
        temp.d1_p*temp.d2_p2 + temp.d1_p* temp.d2_p2));
    newd->d3_q3 = k*(temp.d3_q3 - 0.5/temp.value*
        (-1.5/temp.value*temp.d1_q*temp.d1_q*temp.d1_q + temp.d1_q*temp.d2_q2 + 
        temp.d1_q*temp.d2_q2 + temp.d1_q* temp.d2_q2));
    newd->d3_r3 = k*(temp.d3_r3 - 0.5/temp.value*
        (-1.5/temp.value*temp.d1_r*temp.d1_r*temp.d1_r + temp.d1_r*temp.d2_r2 + 
        temp.d1_r*temp.d2_r2 + temp.d1_r* temp.d2_r2));
    newd->d3_p2r = k*(temp.d3_p2r - 0.5/temp.value*
        (-1.5/temp.value*temp.d1_p*temp.d1_p*temp.d1_r + temp.d1_p*temp.d2_pr + 
        temp.d1_p*temp.d2_pr + temp.d1_r* temp.d2_p2));
    newd->d3_p2q = k*(temp.d3_p2q - 0.5/temp.value*
        (-1.5/temp.value*temp.d1_p*temp.d1_p*temp.d1_q + temp.d1_p*temp.d2_pq + 
        temp.d1_p*temp.d2_pq + temp.d1_q* temp.d2_p2));
    newd->d3_q2r = k*(temp.d3_q2r - 0.5/temp.value*
        (-1.5/temp.value*temp.d1_q*temp.d1_q*temp.d1_r + temp.d1_q*temp.d2_qr + 
        temp.d1_q*temp.d2_qr + temp.d1_r* temp.d2_q2));
    newd->d3_pq2 = k*(temp.d3_pq2 - 0.5/temp.value*
        (-1.5/temp.value*temp.d1_q*temp.d1_q*temp.d1_p + temp.d1_q*temp.d2_pq + 
        temp.d1_q*temp.d2_pq + temp.d1_p* temp.d2_q2));
    newd->d3_pr2 = k*(temp.d3_pr2 - 0.5/temp.value*
        (-1.5/temp.value*temp.d1_r*temp.d1_r*temp.d1_p + temp.d1_r*temp.d2_pr + 
        temp.d1_r*temp.d2_pr + temp.d1_p* temp.d2_r2));
    newd->d3_qr2 = k*(temp.d3_qr2 - 0.5/temp.value*
        (-1.5/temp.value*temp.d1_r*temp.d1_r*temp.d1_q + temp.d1_r*temp.d2_qr + 
        temp.d1_r*temp.d2_qr + temp.d1_q* temp.d2_r2));
    newd->d3_pqr = k*(temp.d3_pqr - 0.5/temp.value*
        (-1.5/temp.value*temp.d1_p*temp.d1_q*temp.d1_r + temp.d1_p*temp.d2_qr + 
        temp.d1_q*temp.d2_pr + temp.d1_r* temp.d2_pq));
}
