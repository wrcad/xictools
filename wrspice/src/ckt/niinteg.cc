
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#include "circuit.h"
#include "input.h"
#include "misc.h"


// For linear inductors and capacitors, one can call this when
// mode is MODEINITTRAN or MODEINITPRED, and save ceq, avoiding
// the repeated and redundant calls to NIintegrate().  An accept
// function can then finish the integration - see INDaccept().
//
double
sCKT::NIceq(int qcap)
{
    double cq = 0;
    if (CKTcurTask->TSKintegrateMethod == GEAR) {
        switch (CKTorder) {
        case 6:
            cq += CKTag[6]* *(CKTstate6+qcap);
            // fall through
        case 5:
            cq += CKTag[5]* *(CKTstate5+qcap);
            // fall through
        case 4:
            cq += CKTag[4]* *(CKTstate4+qcap);
            // fall through
        case 3:
            cq += CKTag[3]* *(CKTstate3+qcap);
            // fall through
        default:
        case 2:
            cq += CKTag[2]* *(CKTstate2+qcap);
            // fall through
        case 1:
            cq += CKTag[1]* *(CKTstate1+qcap);
            break;
        }
    }
    else {
        switch (CKTorder) {
        case 1:
            cq = CKTag[1] * (*(CKTstate1+qcap));
            break;
        default:
        case 2:
            cq = - *(CKTstate1 + qcap+1)*CKTag[1] -
                CKTag[0]* *(CKTstate1 + qcap);
            break;
        }
    }
    return (cq);
}


// Return true if trapezoid integration is showing oscillatory
// behavior.
//
// From the description in "Circuit Simulation with SPICE OPUS: 
// Theory and Practice" by Tadej Tuma, Arpad Burmen.  Check for
// oscillation due to instability of trapezoidal integration.
//
// According to the text, failure to satisfy this relation:
//
//    |i(tk)| <= trapratio * |q(tk+1) - q(tk)|/delta
//
// implies oscillation.  This does not seem to be sufficient, but does
// imply that i(tk) and i(tk+1) are large and have opposite signs.  We
// add a further test
//
//    |i(tk+1) - i(tk)| > trapratio * |i(tk+1) - i(tk-1)|
//
// which seems to provide a more positive test.
//
// There are still too many false positives, so we filter out small
// numbers.
//
bool
sCKT::NItrapCheck(int qcap)
{
    int ccap = qcap + 1;
    double c0 = *(CKTstate0 + ccap);

    // Ignore values less than 1uA, 
    if (fabs(c0) < 1e-6)
        return (false);

    // Ignore small charge.
    double q0 = *(CKTstate0 + qcap);
    if (fabs(q0) < 1e-16)
        return (false);

    double c1 = *(CKTstate1 + ccap);
    double c2 = *(CKTstate2 + ccap);
    double q1 = *(CKTstate1 + qcap);

    double dqdt = fabs(q0 - q1)/CKTdelta;
    double trapratio = CKTcurTask->TSKtrapRatio;

    if (fabs(c1) > trapratio*dqdt && fabs(c0 - c1) > trapratio*fabs(c0 - c2)) {
//        printf("trapcheck %g %g %.2e %g %.2e %.2e %.2e %.2e %.2e %.2e\n",
//            CKTtime, trapratio, dqdt, c1/dqdt, c0, c1, c2, q0, q1, CKTdelta);
        return (true);
    }
    return (false);
}

