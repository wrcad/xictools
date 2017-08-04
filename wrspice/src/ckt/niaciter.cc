
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
#include "device.h"
#include "spmatrix.h"


// This subroutine performs the actual numerical iteration.
// It uses the sparse matrix stored in the NIstruct by NIinit,
// along with the matrix loading program, the load data, the
// convergence test function, and the convergence parameters
// - return value is non-zero for convergence failure 
//
int
sCKT::NIacIter()
{
    CKTmatrix->spSetComplex();
    if (CKTmatrix->spDataAddressChange()) {
        int error = resetup();
        if (error)
            return (error);
    }

    for (;;) {
        CKTnoncon = 0;
        int error = acLoad();
        if (error)
            return (error);
    
        if (CKTniState & NIACSHOULDREORDER) {
            error = CKTmatrix->spOrderAndFactor(0,
                CKTcurTask->TSKpivotRelTol,
                CKTcurTask->TSKpivotAbsTol, 1);
            CKTniState &= ~NIACSHOULDREORDER;
            if (error != 0) {
                // either singular equations or no memory, in either case,
                // let caller handle problem
                //
                return (error);
            }
        }
        else {
            error = CKTmatrix->spFactor();
            if (error != 0) {
                if (error == E_SINGULAR) {
                    // the problem is that the matrix can't be solved with
                    // the current LU factorization.  Maybe if we reload and
                    // try to reorder again it will help...
                    //
                    CKTniState |= NIACSHOULDREORDER;
                    continue;
                }
                return (error); // can't handle E_BADMATRIX, so let caller
            }
        }
        break;
    }
    CKTmatrix->spSolve(CKTrhs, CKTrhs, CKTirhs, CKTirhs);

    *CKTrhs = 0;
    *CKTrhsSpare = 0;
    *CKTrhsOld = 0;
    *CKTirhs = 0;
    *CKTirhsSpare = 0;
    *CKTirhsOld = 0;

    double *tmp = CKTirhsOld;
    CKTirhsOld = CKTirhs;
    CKTirhs = tmp;

    tmp = CKTrhsOld;
    CKTrhsOld = CKTrhs;
    CKTrhs = tmp;

    DVO.dumpStrobe();
    return (OK);
}
