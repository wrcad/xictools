
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
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: niniter.cc,v 2.20 2000/11/19 00:39:56 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Gary W. Ng
         1993 Stephen R. Whiteley
****************************************************************************/

#include "circuit.h"
#include "spmatrix.h"


// This routine solves the adjoint system.  It assumes that the matrix has
// already been loaded by a call to NIacIter, so it only alters the right
// hand side vector.  The unit-valued current excitation is applied
// between nodes posDrive and negDrive.
//
void
sCKT::NInzIter(int posDrive, int negDrive)
{
    // clear out the right hand side vector
    int size = CKTmatrix->spGetSize(1);
    for (int i = 0; i <= size; i++) {
        *(CKTrhs + i) = 0.0;
        *(CKTirhs + i) = 0.0;
    }

    *(CKTrhs + posDrive) = 1.0;     // apply unit current excitation
    *(CKTrhs + negDrive) = -1.0;
    CKTmatrix->spSolveTransposed(CKTrhs, CKTrhs, CKTirhs, CKTirhs);
    *CKTrhs = 0.0;
    *CKTirhs = 0.0;
}
