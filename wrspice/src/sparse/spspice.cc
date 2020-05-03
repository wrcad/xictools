
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

#include "sparse/spconfig.h"
#include "sparse/spmacros.h"
#include "sparse/spmatrix.h"
#include "circuit.h"
#include "miscutil/errorrec.h"
#include <math.h>

#ifdef WRSPICE

#ifdef USE_KLU
#include "klumatrix.h"
#include "kluif.h"
#endif

// Functions for WRSPICE support.
//   S. R. Whiteley 1993 (and subsequent)
//


// Switch to use of KLU for subsequent computation.
//
void
spMatrixFrame::spSwitchMatrix()
{
#ifdef USE_KLU
    ASSERT(IS_SPARSE(this));

    if (BuildState == 1 AND (NOT NoKLU) AND klu_if.is_ok()) {
        spSetMatlabMatrix(new KLUmatrix(Size, Elements, Complex, LongDoubles));
        // We can now destroy all previous elements and fill-ins.
        ElementAllocator.Clear();
        FillinAllocator.Clear();
        memset(FirstInCol, 0, (Size+1)*sizeof(void*));
        memset(FirstInRow, 0, (Size+1)*sizeof(void*));
        memset(Diag, 0, (Size+1)*sizeof(void*));
        DataAddressChange = YES;
        BuildState = 2;
    }
#endif
}


//  COPY REAL PART TO INITIALIZER PART OF MATRIX
//
// Sets the init component to the value of the real component for all
// matrix elements.  This is useful for initializing the real matrix
// with values stored in the init part, and more efficient than the
// SP_OPT_INITIALIZE option functions.
//
// This would be called after the initial matrix setup, during which
// all constant values (constant resistors, the 1's for voltage
// sources, etc.) have been entered.  The state of the matrix is saved
// and used to reinitialize the matrix ahead of subsequent iterations,
// and we skip all loading of constants.
//
//  >>> Local variables:
//
//  pElement  (spMatrixElement*)
//     A pointer to the element being set.
//
void
spMatrixFrame::spSaveForInitialization()
{
    ASSERT(IS_SPARSE(this));

    if (Matrix)
        Matrix->toInit();
    else {
        for (int i = Size; i > 0; i--) {
            spMatrixElement *pElement = FirstInCol[i];
            while (pElement != 0) {
                pElement->Init = pElement->Real;
#if SP_OPT_LONG_DBL_SOLVE
                // If long doubles are enabled, the Init value will be
                // a long double.
                pElement->Init2 = pElement->Imag;
#endif
                pElement = pElement->NextInCol;
            }
        }
    }

    // Empty the trash.
    TrashCan.Real = 0.0;
#if SP_OPT_COMPLEX
    TrashCan.Imag = 0.0;
#endif

    Error = spOKAY;
    Factored = NO;
    SingularCol = 0;
    SingularRow = 0;
}


//  COPY INITIALIZER TO REAL PART OF MATRIX
//
// Sets the real component to the value of the init component, and
// zero the imaginary part if the matrix is complex.  This is useful
// for initializing the real matrix with values stored in the init
// part, and more efficient than the SP_OPT_INITIALIZE option
// functions.
//
// This would be called instead of spClear at the start of each
// iteration.  If the initialization contains the comstant values, we
// can skip loading these, probably saving time.  For example, linear
// resistors can be skipped entirely.
//
//  >>> Local variables:
//
//  pElement  (spMatrixElement*)
//     A pointer to the element being set.
//
void
spMatrixFrame::spLoadInitialization()
{
    ASSERT(IS_SPARSE(this));

    if (Matrix)
        Matrix->fromInit();
    else {
        for (int i = Size; i > 0; i--) {
            spMatrixElement *pElement = FirstInCol[i];
            while (pElement != 0) {
#if SP_OPT_LONG_DBL_SOLVE
                if (LongDoubles) {
                    double *d = &pElement->Init;
                    LDBL(pElement) = LDBL(d);
                    pElement = pElement->NextInCol;
                    continue;
                }
#endif
                pElement->Real = pElement->Init;
                pElement = pElement->NextInCol;
            }
        }
    }

    // Empty the trash.
    TrashCan.Real = 0.0;
#if SP_OPT_COMPLEX
    TrashCan.Imag = 0.0;
#endif

    Error = spOKAY;
    Factored = NO;
    SingularCol = 0;
    SingularRow = 0;
}


// Simply negate all elements.
//
void
spMatrixFrame::spNegate()
{
    if (Matrix)
        Matrix->negate();
    else {
        for (int I = Size; I > 0; I--) {
            spMatrixElement *pElement = FirstInCol[I];
            while (pElement != 0) {
                pElement->Real = -pElement->Real;
#if SP_OPT_COMPLEX
                pElement->Imag = -pElement->Imag;
#endif
                pElement = pElement->NextInCol;
            }
        }
    }
}


// Return the largest real value.
//
double
spMatrixFrame::spLargest()
{
    if (Matrix)
        return (Matrix->largest());
    double lx = 0;
    for (int I = Size; I > 0; I--) {
        spMatrixElement *pElement = FirstInCol[I];
        while (pElement != 0) {
            double f = fabs(pElement->Real);
            if (f > lx)
                lx = f;
            pElement = pElement->NextInCol;
        }
    }
    return (lx);
}


// Return the smallest real value that is not 1.0, or 1.0 if no other
// value.
//
double
spMatrixFrame::spSmallest()
{
    if (Matrix)
        return (Matrix->smallest());
    double sx = HUGE_VAL;
    for (int I = Size; I > 0; I--) {
        spMatrixElement *pElement = FirstInCol[I];
        while (pElement != 0) {
            double f = fabs(pElement->Real);
            if (f < sx && f != 1.0 && f != 0.0)
                sx = f;
            pElement = pElement->NextInCol;
        }
    }
    if (sx == HUGE_VAL)
        sx = 1.0;
    return (sx);
}


// When checkmin and checkmax are false, add gmin to the diagonal
// element corresponding to node.  Otherwise, ensure that all
// non-negative entries are gmin or larger if checkmin is true, and
// that the entry is not larger than gmax if checkmax is true.  On
// success, true is returned.
//
bool
spMatrixFrame::spLoadGmin(int node,
    double gmin, double gmax, bool checkmin, bool checkmax)
{
    if (node <= 0 || node > Size)
        return (false);
#if SP_OPT_TRANSLATE
    int row = ExtToIntRowMap[node];
    int col = ExtToIntColMap[node];
#else
    int row = node;
    int col = node;
#endif
    if (row <= 0)
        return (false);
    if (Matrix) {
        double *r = Matrix->find(row-1, col-1);
        if (!r) {
            // There is no diagonal element.  Look for 1/-1 in the column, if
            // we find one, assume that this is a junction of branch (voltage
            // source and inductor) devices.  Accept the case of an
            // unconnected voltage source, these do no harm.

            return (Matrix->checkColOnes(col-1));
        }
        if (LongDoubles) {
            if (!checkmin && !checkmax)
                LDBL(r) += gmin;
            else if (checkmin && (LDBL(r) >= 0.0 && LDBL(r) < gmin))
                LDBL(r) = gmin;
            else if (checkmax && (LDBL(r) > gmax))
                LDBL(r) = gmax;
        }
        else {
            if (!checkmin && !checkmax)
                *r += gmin;
            else if (checkmin && (*r >= 0.0 && *r < gmin))
                *r = gmin;
            else if (checkmax && (*r > gmax))
                *r = gmax;
        }
        return (true);
    }

    if (row == col) {
        if (Diag[row]) {
            if (LongDoubles) {
                if (!checkmin && !checkmax)
                    LDBL(Diag[row]) += gmin;
                else if (checkmin && (LDBL(Diag[row]) >= 0.0 && LDBL(Diag[row]) < gmin))
                    LDBL(Diag[row]) = gmin;
                else if (checkmax && (LDBL(Diag[row]) > gmax))
                    LDBL(Diag[row]) = gmax;

            }
            else {
                if (!checkmin && !checkmax)
                    Diag[row]->Real += gmin;
                else if (checkmin && (Diag[row]->Real >= 0.0 && Diag[row]->Real < gmin))
                    Diag[row]->Real = gmin;
                else if (checkmax && (Diag[row]->Real > gmax))
                    Diag[row]->Real = gmax;
            }
            return (true);
        }
    }
    else {
        spMatrixElement *e = FirstInCol[col];
        while (e) {
            if (e->Row == row) {
                if (LongDoubles) {
                    if (!checkmin && !checkmax)
                        LDBL(e) += gmin;
                    else if (checkmin && (LDBL(e) >= 0.0 && LDBL(e) < gmin))
                        LDBL(e) = gmin;
                    else if (checkmax && (LDBL(e) > gmax))
                        LDBL(e) = gmax;
                }
                else {
                    if (!checkmin && !checkmax)
                        e->Real += gmin;
                    else if (checkmin && (e->Real >= 0.0 && e->Real < gmin))
                        e->Real = gmin;
                    else if (checkmax && (e->Real > gmax))
                        e->Real = gmax;
                }
                return (true);
            }
            e = e->NextInCol;
        }
    }

    // There is no diagonal element.  Look for 1/-1 in the column, if
    // we find one, assume that this is a junction of branch (voltage
    // source and inductor) devices.  Accept the case of an
    // unconnected voltage source, these do no harm.

    spMatrixElement *e = FirstInCol[col];
    while (e) {
        if (LongDoubles) {
            if (LDBL(e) == 1.0 || LDBL(e) == -1.0)
                return (true);
        }
        else {
            if (e->Real == 1.0 || e->Real == -1.0)
                return (true);
        }
        e = e->NextInCol;
    }
    return (false);
}


// Return true if node is properly mapped.  This must be called before
// switching to KLU.
//
bool
spMatrixFrame::spCheckNode(int node, sCKT *ckt)
{
    if (node <= 0 || node > Size) {
        Errs()->add_error("unmapped node, unconnected current source?");
        return (false);
    }
    int row = ExtToIntRowMap[node];
    if (row < 0) {
        Errs()->add_error("unmapped node, unconnected current source?");
        return (false);
    }
    sCKTnode *n = ckt->CKTnodeTab.find(node);
    if (!n) {
        Errs()->add_error("internal error, node doesn't exist!");
        return (false);
    }
    if (n->type() != SP_VOLTAGE)
        return (true);

    if (Matrix) {
        // Further testing not possible.
        return (true);
    }

    if (Diag[row])
        return (true);

    // There is no diagonal element for this node.  This is not
    // necessarily an error, as it is a normal result when voltage
    // sources and/or inductors are connected in series.

    // Look at the connections along the column.  If we find one that
    // is a "current" node, we do some checking.  The node can come
    // from the pos/neg ends of a voltage source or inductor, or the
    // control input of a vcvs.  In this latter case, the node exists
    // along the column, but not along the row.  We would much rather
    // search by rows, but they are not linked yet.  Find the column
    // head for the number of the row found, search it for the row
    // matching the original row.  If this exists, the connection is
    // not a vcvs control, and all is well.  If elements along the
    // original column contain only voltage nodes and vcvs control
    // nodes, the connectivity is indicated as bad.

    spMatrixElement *e = FirstInCol[row];
    while (e) {
        n = ckt->CKTnodeTab.find((unsigned int)IntToExtRowMap[e->Row]);
        if (!n) {
            Errs()->add_error(
        "internal error, internal node number %d has no corresponding node",
                IntToExtRowMap[e->Row]);
            return (false);
        }
        if (n->type() == SP_CURRENT) {
            int nc = ExtToIntColMap[n->number()];
            for (spMatrixElement *x = FirstInCol[nc]; x; x = x->NextInCol) {
                if (x->Row == row)
                    // OK, this can not be a vccs control input.
                    return (true);
            }
        }
        e = e->NextInCol;
    }
    Errs()->add_error("improperly connected node");
    return (false);
}


// Return some interesting things.
//
void
spMatrixFrame::spGetStat(int *size, int *nonZero, int *fillIns)
{
    const spMatrixFrame *thismf = this;
    if (thismf) {
        *size = Size;
        *nonZero = spElementCount();
        *fillIns = spFillinCount();
    }
    else {
        *size = 0;
        *nonZero = 0;
        *fillIns = 0;
    }
}


// spAddCol()
//
int
spMatrixFrame::spAddCol(int accumCol, int addendCol)
{
    accumCol = ExtToIntColMap[accumCol];
    addendCol = ExtToIntColMap[accumCol];
    spMatrixElement *accum = FirstInCol[accumCol];
    spMatrixElement *addend = FirstInCol[addendCol];
    spMatrixElement *prev = 0;
    while (addend != 0) {
        while (accum && accum->Row < addend->Row) {
            prev = accum;
            accum = accum->NextInCol;
        }
        if (!accum || accum->Row > addend->Row) {
            accum = CreateElement(addend->Row, accumCol, &prev, 0);
        }
        accum->Real += addend->Real;
        accum->Imag += addend->Imag;
        addend = addend->NextInCol;
    }
    return (spError());
}


// spZeroCol()
//
int
spMatrixFrame::spZeroCol(int col)
{
    col = ExtToIntColMap[col];
    for (spMatrixElement *element = FirstInCol[col]; element != 0;
            element = element->NextInCol) {
        element->Real = 0.0;
        element->Imag = 0.0;
    }
    return (spError());
}


#ifndef M_LN2
#define M_LN2   0.69314718055994530942
#endif
#ifndef M_LN10
#define M_LN10  2.30258509299404568402
#endif

namespace {
    // Avoid messing with checking for the presence of these functions
    // in the library, and whether logb() returns int or double.
    //
    double my_logb(double x)
    {
        double y = 0.0;

        if (x != 0.0) {
            if (x < 0.0)
                x = - x;
            while (x > 2.0) {
                y += 1.0;
                x /= 2.0;
            }
            while (x < 1.0) {
                y -= 1.0;
                x *= 2.0;
            }
        }
        else
            y = 0.0;

        return (y);
    }


    double my_scalb(double x, int n)
    {
        double y, z = 1.0, k = 2.0;

        if (n < 0) {
            n = -n;
            k = 0.5;
        }

        if (x != 0.0)
            for (y = 1.0; n; n >>= 1) {
                y *= k;
                if (n & 1)
                    z *= y;
            }

        return (x * z);
    }
}


// spDProd()
//
int
spMatrixFrame::spDProd(double *pMantissaR, double *pMantissaI, int *pExponent)
{
    double re, im, x, y, z;
    int p;

    spDeterminant(&p, &re, &im);

#ifdef debug_print
    printf("Determinant 10: (%20g,%20g)^%d\n", re, im, p);
#endif

    // Convert base 10 numbers to base 2 numbers, for comparison
    y = p * M_LN10 / M_LN2;
    x = (int) y;
    y -= x;

    // ASSERT
    //    x = integral part of exponent, y = fraction part of exponent
    //

    // Fold in the fractional part
#ifdef debug_print
    printf(" ** base10 -> base2 int =  %g, frac = %20g\n", x, y);
#endif
    z = pow(2.0, y);
    re *= z;
    im *= z;
#ifdef debug_print
    printf(" ** multiplier = %20g\n", z);
#endif

    // Re-normalize (re or im may be > 2.0 or both < 1.0
    if (re != 0.0) {
    y = my_logb(re);
    if (im != 0.0)
        z = my_logb(im);
    else
        z = 0;
    }
    else if (im != 0.0) {
        z = my_logb(im);
        y = 0;
    }
    else {
        // Singular
        //printf("10 -> singular\n");
        y = 0;
        z = 0;
    }

#ifdef debug_print
    printf(" ** renormalize changes = %g,%g\n", y, z);
#endif
    if (y < z)
        y = z;

    *pExponent = (int)(x + y);
    x = my_scalb(re, (int) -y);
    z = my_scalb(im, (int) -y);
#ifdef debug_print
    printf(" ** values are: re %g, im %g, y %g, re' %g, im' %g\n",
        re, im, y, x, z);
#endif
    *pMantissaR = my_scalb(re, (int) -y);
    *pMantissaI = my_scalb(im, (int) -y);

#ifdef debug_print
    printf("Determinant 10->2: (%20g,%20g)^%d\n", *pMantissaR,
    *pMantissaI, *pExponent);
#endif
    return (spError());
}

#endif // WRSPICE

