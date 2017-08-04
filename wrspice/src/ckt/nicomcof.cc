
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "circuit.h"
#include "errors.h"


namespace { int ni_gearcof(sCKT*); }

// This routine calculates the timestep-dependent terms used in the
// numerical integration.
// 
int
sCKT::NIcomCof()
{
    if (CKTcurTask->TSKintegrateMethod == TRAPEZOIDAL) {
        if (CKTorder == 2) {
            double t0 = 1.0/(1.0 - CKTcurTask->TSKxmu);
            CKTag[0] = t0/CKTdelta;
            CKTag[1] = t0*CKTcurTask->TSKxmu;
            return (OK);
        }
        if (CKTorder == 1) {
            CKTag[0] = 1.0/CKTdelta;
            CKTag[1] = -CKTag[0];
            return (OK);
        }
        return(E_ORDER);
    }
    if (CKTcurTask->TSKintegrateMethod == GEAR)
        return (ni_gearcof(this));
    return (E_METHOD);
}


namespace {
    int ni_gearcof(sCKT *ckt)
    {
        if (ckt->CKTorder < 1 || ckt->CKTorder > 6)
            return (E_ORDER);

        ckt->CKTag[0] = 0;
        ckt->CKTag[1] = -1.0/ckt->CKTdelta;
        ckt->CKTag[2] = 0;
        ckt->CKTag[3] = 0;
        ckt->CKTag[4] = 0;
        ckt->CKTag[5] = 0;
        ckt->CKTag[6] = 0;
        // first, set up the matrix
        double mat[8][8];   // matrix to compute the gear coefficients in
        int i;
        for (i = 0; i <= ckt->CKTorder; i++) { mat[0][i]=1; }
        for (i = 1; i <= ckt->CKTorder; i++) { mat[i][0]=0; }
        // SPICE2 difference warning
        // the following block builds the corrector matrix
        // using (sum of h's)/h(final) instead of just (sum of h's)
        // because the h's are typically ~1e-10, so h^7 is an
        // underflow on many machines, but the ratio is ~1
        // and produces much better results
        //
        double arg = 0;
        for (i = 1; i <= ckt->CKTorder; i++) {
            arg += ckt->CKTdeltaOld[i-1];
            double arg1 = 1;
            for (int j = 1; j <= ckt->CKTorder; j++) {
                arg1 *= arg/ckt->CKTdelta;
                mat[j][i] = arg1;
            }
        }
        // lu decompose
        // weirdness warning! 
        // The following loop (and the first one after the forward
        // substitution comment) start at one instead of zero
        // because of a SPECIAL CASE - the first column is 1 0 0 ...
        // thus, the first iteration of both loops does nothing,
        // so it is skipped
        //
        for (i = 1; i <= ckt->CKTorder; i++) {
            for (int j = i+1; j <= ckt->CKTorder; j++) {
                mat[j][i] /= mat[i][i];
                for (int k = i+1; k <= ckt->CKTorder; k++) {
                    mat[j][k] -= mat[j][i]*mat[i][k];
                }
            }
        }
        // forward substitution
        for (i = 1; i <= ckt->CKTorder; i++) {
            for (int j = i+1; j <= ckt->CKTorder; j++) {
                ckt->CKTag[j] = ckt->CKTag[j]-mat[j][i]*ckt->CKTag[i];
            }
        }
        // backward substitution
        ckt->CKTag[ckt->CKTorder] /= mat[ckt->CKTorder][ckt->CKTorder];
        for (i = ckt->CKTorder-1; i >= 0; i--) {
            for (int j = i+1; j <= ckt->CKTorder; j++) {
                ckt->CKTag[i] = ckt->CKTag[i] - mat[i][j]*ckt->CKTag[j];
            }
            ckt->CKTag[i] /= mat[i][i];
        }
        return (OK);
    }
}

