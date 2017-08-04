
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
 * Sparse Matrix Package
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

//======= Original Header ================================================
//
//  Revision and copyright information.
//
//  Copyright (c) 1985,86,87,88,89,90
//  by Kenneth S. Kundert and the University of California.
//
//  Permission to use, copy, modify, and distribute this software and
//  its documentation for any purpose and without fee is hereby granted,
//  provided that the copyright notices appear in all copies and
//  supporting documentation and that the authors and the University of
//  California are properly credited.  The authors and the University of
//  California make no representations as to the suitability of this
//  software for any purpose.  It is provided `as is', without express
//  or implied warranty.
//
//  "Sparse1.3: Copyright (c) 1985,86,87,88,89,90 by Kenneth S. Kundert";
//========================================================================

//  IMPORTS
//
//  spconfig.h
//    Macros that customize the sparse matrix functions.
//  spmatrix.h
//    Macros and declarations to be imported by the user.
//  spmacros.h
//    Macro definitions for the sparse matrix functions.
//
#include "config.h"
#define spINSIDE_SPARSE
#include "spconfig.h"
#include "spmatrix.h"
#include "spmacros.h"
#include <string.h>

#ifdef WRSPICE
#include "ttyio.h"
#define PRINTF TTY.err_printf
#else
#define PRINTF printf
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifndef SHRT_MAX
#define SHRT_MAX    32766
#endif
#ifndef LONG_MAX
#define LONG_MAX    2147483646
#endif


//  MATRIX FACTORIZATION MODULE
//
//  Author:                     Advising Professor:
//      Kenneth S. Kundert          Alberto Sangiovanni-Vincentelli
//      UC Berkeley
//
//  This file contains the functions to factor the matrix into LU form.
//
//  >>> Public functions contained in this file:
//
//  spOrderAndFactor
//  spFactor
//  spPartition
//
//  >>> Private functions contained in this file:
//
//  FactorComplexMatrix
//  CreateInternalVectors
//  CountMarkowitz
//  MarkowitzProducts
//  SearchForPivot
//  SearchForSingleton
//  QuicklySearchDiagonal
//  SearchDiagonal
//  SearchEntireMatrix
//  FindLargestInCol
//  FindBiggestInColExclude
//  ExchangeRowsAndCols
//  RowExchange
//  ColExchange
//  ExchangeColElements
//  ExchangeRowElements
//  RealRowColElimination
//  ComplexRowColElimination
//  UpdateMarkowitzNumbers
//  CreateFillin
//  MatrixIsSingular
//  ZeroPivot
//  WriteStatus

// #define UFEEDBACK
// #define TIMES 2
 
#ifdef TIMES
#include <sys/times.h>
#include <sys/param.h>
 
namespace {
    double sp_seconds()
    {
        static struct tms timestuff;
        times(&timestuff);
        return (timestuff.tms_utime/(double)HZ);
    }  
}
#endif


//  ORDER AND FACTOR MATRIX
//
// This function chooses a pivot order for the matrix and factors it
// into LU form.  It handles both the initial factorization and
// subsequent factorizations when a reordering is desired.  This is
// handled in a manner that is transparent to the user.  The function
// uses a variation of Gauss's method where the pivots are associated
// with L and the diagonal terms of U are one.
//
//  >>> Returned:
//
//  The error code is returned.  Possible errors are listed below.
//
//  >>> Arguments:
//
//  rhs  <input>  (spREAL*)
//      Representative right-hand side vector that is used to determine
//      pivoting order when the right hand side vector is sparse.  If
//      RHS is a 0 pointer then the RHS vector is assumed to
//      be full and it is not used when determining the pivoting
//      order.
//
//  relThreshold  <input>  (spREAL)
//      This number determines what the pivot relative threshold will
//      be.  It should be between zero and one.  If it is one then the
//      pivoting method becomes complete pivoting, which is very slow
//      and tends to fill up the matrix.  If it is set close to zero
//      the pivoting method becomes strict Markowitz with no
//      threshold.  The pivot threshold is used to eliminate pivot
//      candidates that would cause excessive element growth if they
//      were used.  Element growth is the cause of roundoff error.
//      Element growth occurs even in well-conditioned matrices.
//      Setting the RelThreshold large will reduce element growth and
//      roundoff error, but setting it too large will cause execution
//      time to be excessive and will result in a large number of
//      fill-ins.  If this occurs, accuracy can actually be degraded
//      because of the large number of operations required on the
//      matrix due to the large number of fill-ins.  A good value seems
//      to be 0.001.  The default is chosen by giving a value larger
//      than one or less than or equal to zero.  This value should be
//      increased and the matrix resolved if growth is found to be
//      excessive.  Changing the pivot threshold does not improve
//      performance on matrices where growth is low, as is often the
//      case with ill-conditioned matrices.  Once a valid threshold is
//      given, it becomes the new default.  The default value of
//      RelThreshold was choosen for use with nearly diagonally
//      dominant matrices such as node- and modified-node admittance
//      matrices.  For these matrices it is usually best to use
//      diagonal pivoting.  For matrices without a strong diagonal, it
//      is usually best to use a larger threshold, such as 0.01 or
//      0.1.
//
//  absThreshold  <input>  (spREAL)
//      The absolute magnitude an element must have to be considered
//      as a pivot candidate, except as a last resort.  This number
//      should be set significantly smaller than the smallest diagonal
//      element that is is expected to be placed in the matrix.  If
//      there is no reasonable prediction for the lower bound on these
//      elements, then AbsThreshold should be set to zero.
//      AbsThreshold is used to reduce the possibility of choosing as a
//      pivot an element that has suffered heavy cancellation and as a
//      result mainly consists of roundoff error.  Once a valid
//      threshold is given, it becomes the new default.
//
//  diagPivoting  <input>  (spBOOLEAN)
//      A flag indicating that pivot selection should be confined to the
//      diagonal if possible.  If DiagPivoting is nonzero and if
//      SP_OPT_DIAGONAL_PIVOTING is enabled pivots will be chosen only from
//      the diagonal unless there are no diagonal elements that satisfy
//      the threshold criteria.  Otherwise, the entire reduced
//      submatrix is searched when looking for a pivot.  The diagonal
//      pivoting in Sparse is efficient and well refined, while the
//      off-diagonal pivoting is not.  For symmetric and near symmetric
//      matrices, it is best to use diagonal pivoting because it
//      results in the best performance when reordering the matrix and
//      when factoring the matrix without ordering.  If there is a
//      considerable amount of nonsymmetry in the matrix, then
//      off-diagonal pivoting may result in a better equation ordering
//      simply because there are more pivot candidates to choose from.
//      A better ordering results in faster subsequent factorizations.
//      However, the initial pivot selection process takes considerably
//      longer for off-diagonal pivoting.
//
//  >>> Local variables:
//
//  pivot  (spMatrixElement*)
//      Pointer to the element being used as a pivot.
//
//  reorderingRequired  (spBOOLEAN)
//      Flag that indicates whether reordering is required.
//
//  >>> Possible errors:
//
//  spSINGULAR
//  spSMALL_PIVOT
//  Error is cleared in this function.
//
// The matrix elements must *not* be long doubles when this is called. 
// Being lazy, I'd like to avoid converting all this to work with long
// doubles.  For now, we'll start with doubles, and switch to long
// doubles after this function is called.  We make the decision
// whether to load doubles or long doubles before loading the matrix.
// If we anticipate calling this function, we load doubles.
//
int
spMatrixFrame::spOrderAndFactor(spREAL *rhs, spREAL relThreshold,
    spREAL absThreshold, spBOOLEAN diagPivoting)
{
    ASSERT(NOT Factored);

    Error = spOKAY;
    ReorderFailed = NO;

    if (Trace) {
#if SP_OPT_LONG_DBL_SOLVE
        PRINTF("ordering: cplx=%d extraPrec=%d needsOrdering=%d\n",
            Complex, LongDoubles, NeedsOrdering);
#else
        PRINTF("ordering: cplx=%d needsOrdering=%d\n",
            Complex, NeedsOrdering);
#endif
    }
    if (Matrix) {
        Error = Matrix->factor();
        if (Error == spOKAY) {
            NeedsOrdering = NO;
            Reordered = YES;
            Factored = YES;
        }
        else
            ReorderFailed = YES;
        return (Error);
    }

    DidReorder = NO;
    if (relThreshold <= 0.0)
        relThreshold = RelThreshold;
    if (relThreshold > 1.0)
        relThreshold = RelThreshold;
    RelThreshold = relThreshold;
    if (absThreshold < 0.0)
        absThreshold = AbsThreshold;
    AbsThreshold = absThreshold;
    spBOOLEAN reorderingRequired = NO;

    int step;
    if (NOT NeedsOrdering) {
        // Matrix has been factored before and reordering is not required.
        for (step = 1; step <= Size; step++) {
            spMatrixElement *pivot = Diag[step];
            spREAL largestInCol = FindLargestInCol(pivot->NextInCol);
            if ((largestInCol * relThreshold < E_MAG(pivot))) {
#if SP_OPT_COMPLEX
                if (Complex)
                    ComplexRowColElimination(pivot);
                else
#endif
                    RealRowColElimination(pivot);
            }
            else {
                reorderingRequired = YES;
                break; // for loop
            }
        }
        if (NOT reorderingRequired) {
            Reordered = YES;
            Factored = YES;
            return (Error);
        }
        else {
            // A pivot was not large enough to maintain accuracy,
            // so a partial reordering is required.

#if (ANNOTATE >= ON_STRANGE_BEHAVIOR)
            PRINTF("Reordering,  Step = %d\n", step);
#else
            if (Trace)
                PRINTF("Reordering,  Step = %d\n", step);
#endif
        }
    }
    else {
        // This is the first time the matrix has been factored.  These few
        // statements indicate to the rest of the code that a full reodering
        // is required rather than a partial reordering, which occurs during
        // a failure of a fast factorization.

        step = 1;
        if (NOT RowsLinked)
            LinkRows();
        if (NOT InternalVectorsAllocated)
            CreateInternalVectors();
        if (Error >= spFATAL)
            return (Error);
#if SP_BITFIELD
        ba_setup();
#endif
    }

    // Form initial Markowitz products.
    CountMarkowitz(rhs, step);
    MarkowitzProducts(step);
    MaxRowCountInLowerTri = -1;
    DidReorder = YES;
#ifdef TIMES
    double T0 = sp_seconds();
#endif

    // Perform reordering and factorization.
    for ( ; step <= Size; step++) {
        spMatrixElement *pivot = SearchForPivot(step, diagPivoting);
        if (pivot == 0) {
            ReorderFailed = YES;
            return (MatrixIsSingular(step));
        }
        ExchangeRowsAndCols(pivot, step);

#ifdef UFEEDBACK
#if defined(TIMES) AND TIMES == 2
        if (!(step % 100)) {
            double T1 = sp_seconds();
            PRINTF(" %d / %d %.2f\n", step, Size, T1 - T0);
            T0 = T1;
#else
        if (Size >= 100 && !(step % (Size/10))) {
            PRINTF(" %.1f", (100.0*step)/Size);
            fflush(stdout);
#endif
        }
#endif

#if SP_OPT_COMPLEX
        if (Complex)
            ComplexRowColElimination(pivot);
        else
#endif
            RealRowColElimination(pivot);

        if (Error >= spFATAL) {
            ReorderFailed = YES;
            return (Error);
        }
        UpdateMarkowitzNumbers(pivot);

#if (ANNOTATE == FULL)
        WriteStatus(step);
#endif
#if SP_OPT_INTERRUPT
        if (InterruptCallback && !(step & 0xff) && (*InterruptCallback)()) {
            ReorderFailed = YES;
            return (spABORTED);
        }
#endif
    }

#ifdef UFEEDBACK
#if defined(TIMES) AND TIMES == 2
#else
    if (Size >= 100)
        PRINTF("\n");
#endif
#endif
#ifdef TIMES
    PRINTF("Reorder/factor time %g\n", sp_seconds() - T0);
#endif

    NeedsOrdering = NO;
    Reordered = YES;
    Factored = YES;

    return (Error);
}


//  FACTOR MATRIX
//
// This function is the companion function to spOrderAndFactor(). 
// Unlike spOrderAndFactor(), spFactor() cannot change the ordering. 
// It is also faster than spOrderAndFactor().  The standard way of
// using these two functions is to first use spOrderAndFactor() for
// the initial factorization.  For subsequent factorizations,
// spFactor() is used if there is some assurance that little growth
// will occur (say for example, that the matrix is diagonally
// dominant).  If spFactor() is called for the initial factorization
// of the matrix, then spOrderAndFactor() is automatically called with
// the default threshold.  This function uses "row at a time" LU
// factorization.  Pivots are associated with the lower triangular
// matrix and the diagonals of the upper triangular matrix are ones.
//
//  >>> Returned:
//
//  The error code is returned.  Possible errors are listed below.
//
//  >>> Possible errors:
//
//  spSINGULAR
//  spZERO_DIAG
//  spSMALL_PIVOT
//  Error is cleared in this function.
//
int
spMatrixFrame::spFactor()
{
    ASSERT(NOT Factored);

    if (NeedsOrdering)
        return (spOrderAndFactor(0, 0.0, 0.0, DIAG_PIVOTING_AS_DEFAULT));

    if (Trace) {
#if SP_OPT_LONG_DBL_SOLVE
        PRINTF("factoring: cplx=%d extraPrec=%d factored=%d\n", Complex,
            LongDoubles, Factored);
#else
        PRINTF("factoring: cplx=%d factored=%d\n", Complex, Factored);
#endif
    }
    if (Matrix) {
        Error = Matrix->refactor();
        if (Error == spOKAY)
            Factored = YES;
        return (Error);
    }

    if (NOT Partitioned)
        spPartition(spDEFAULT_PARTITION);
#if SP_OPT_COMPLEX
    if (Complex)
        return (FactorComplexMatrix());
#endif

#if SP_OPT_REAL
#if SP_OPT_LONG_DBL_SOLVE
    if (LongDoubles) {
        if (LDBL(Diag[1]) == 0.0)
            return (ZeroPivot(1));
        LDBL(Diag[1]) = 1.0 / LDBL(Diag[1]);

        // Start factorization.
        if (PartitionMode == spINDIRECT_PARTITION) {

            // Update column using indirect addressing scatter-gather.
            spREAL **pDest = (spREAL **)Intermediate;

            for (int step = 2; step <= Size; step++) {

                // Scatter.
                spMatrixElement *pElement = FirstInCol[step];
                while (pElement != 0) {
                    pDest[pElement->Row] = &pElement->Real;
                    pElement = pElement->NextInCol;
                }

                // Update column.
                spMatrixElement *pColumn = FirstInCol[step];
                while (pColumn->Row < step) {
                    pElement = Diag[pColumn->Row];
                    long double mult =
                        (LDBL(pDest[pColumn->Row]) *= LDBL(pElement));
                    while ((pElement = pElement->NextInCol) != 0)
                        LDBL(pDest[pElement->Row]) -= mult * LDBL(pElement);
                    pColumn = pColumn->NextInCol;
                }

                // Check for singular matrix.
                if (LDBL(Diag[step]) == 0.0)
                    return (ZeroPivot(step));
                LDBL(Diag[step]) = 1.0 / LDBL(Diag[step]);
            }
        }
        else if (PartitionMode == spDIRECT_PARTITION) {

            // Update column using direct addressing scatter-gather.
            // Allocated size is ok here, Intermediate is already sized for
            // complex.
            long double *Dest = (long double *)Intermediate;

            for (int step = 2; step <= Size; step++) {

                // Scatter.
                spMatrixElement *pElement = FirstInCol[step];
                while (pElement != 0) {
                    Dest[pElement->Row] = LDBL(pElement);
                    pElement = pElement->NextInCol;
                }

                // Update column.
                spMatrixElement *pColumn = FirstInCol[step];
                while (pColumn->Row < step) {
                    pElement = Diag[pColumn->Row];
                    long double mult = Dest[pColumn->Row] * LDBL(pElement);
                    LDBL(pColumn) = mult;
                    while ((pElement = pElement->NextInCol) != 0)
                        Dest[pElement->Row] -= mult * LDBL(pElement);
                    pColumn = pColumn->NextInCol;
                }

                // Gather.
                pElement = Diag[step]->NextInCol;
                while (pElement != 0) {
                    LDBL(pElement) = Dest[pElement->Row];
                    pElement = pElement->NextInCol;
                }

                // Check for singular matrix.
                if (Dest[step] == 0.0)
                    return (ZeroPivot(step));
                LDBL(Diag[step]) = 1.0 / Dest[step];
            }
        }
        else {
            for (int step = 2; step <= Size; step++) {
                if (!DoRealDirect[step]) {

                    // Update column using indirect addressing scatter-gather.
                    spREAL **pDest = (spREAL **)Intermediate;

                    // Scatter.
                    spMatrixElement *pElement = FirstInCol[step];
                    while (pElement != 0) {
                        pDest[pElement->Row] = &pElement->Real;
                        pElement = pElement->NextInCol;
                    }

                    // Update column.
                    spMatrixElement *pColumn = FirstInCol[step];
                    while (pColumn->Row < step) {
                        pElement = Diag[pColumn->Row];
                        long double mult =
                            (LDBL(pDest[pColumn->Row]) *= LDBL(pElement));
                        while ((pElement = pElement->NextInCol) != 0)
                            LDBL(pDest[pElement->Row]) -= mult * LDBL(pElement);
                        pColumn = pColumn->NextInCol;
                    }

                    // Check for singular matrix.
                    if (LDBL(Diag[step]) == 0.0)
                        return (ZeroPivot(step));
                    LDBL(Diag[step]) = 1.0 / LDBL(Diag[step]);
                }
                else {
                    // Update column using direct addressing scatter-gather.
                    long double *Dest = (long double *)Intermediate;

                    // Scatter.
                    spMatrixElement *pElement = FirstInCol[step];
                    while (pElement != 0) {
                        Dest[pElement->Row] = LDBL(pElement);
                        pElement = pElement->NextInCol;
                    }

                    // Update column.
                    spMatrixElement *pColumn = FirstInCol[step];
                    while (pColumn->Row < step) {
                        pElement = Diag[pColumn->Row];
                        long double mult = Dest[pColumn->Row] * LDBL(pElement);
                        LDBL(pColumn) = mult;
                        while ((pElement = pElement->NextInCol) != 0)
                            Dest[pElement->Row] -= mult * LDBL(pElement);
                        pColumn = pColumn->NextInCol;
                    }

                    // Gather.
                    pElement = Diag[step]->NextInCol;
                    while (pElement != 0) {
                        LDBL(pElement) = Dest[pElement->Row];
                        pElement = pElement->NextInCol;
                    }

                    // Check for singular matrix.
                    if (Dest[step] == 0.0)
                        return (ZeroPivot(step));
                    LDBL(Diag[step]) = 1.0 / Dest[step];
                }
            }
        }
        Factored = YES;
        return (Error = spOKAY);
    }
#endif // SP_OPT_LONG_DBL_SOLVE

    if (Diag[1]->Real == 0.0)
        return (ZeroPivot(1));
    Diag[1]->Real = 1.0 / Diag[1]->Real;

    // Start factorization.
    if (PartitionMode == spINDIRECT_PARTITION) {

        // Update column using indirect addressing scatter-gather.
        spREAL **pDest = (spREAL **)Intermediate;

        for (int step = 2; step <= Size; step++) {

            // Scatter.
            spMatrixElement *pElement = FirstInCol[step];
            while (pElement != 0) {
                pDest[pElement->Row] = &pElement->Real;
                pElement = pElement->NextInCol;
            }

            // Update column.
            spMatrixElement *pColumn = FirstInCol[step];
            while (pColumn->Row < step) {
                pElement = Diag[pColumn->Row];
                spREAL mult = (*pDest[pColumn->Row] *= pElement->Real);
                while ((pElement = pElement->NextInCol) != 0)
                    *pDest[pElement->Row] -= mult * pElement->Real;
                pColumn = pColumn->NextInCol;
            }

            // Check for singular matrix.
            if (Diag[step]->Real == 0.0)
                return (ZeroPivot(step));
            Diag[step]->Real = 1.0 / Diag[step]->Real;
        }
    }
    else if (PartitionMode == spDIRECT_PARTITION) {

        // Update column using direct addressing scatter-gather.
        spREAL *Dest = (spREAL *)Intermediate;

        for (int step = 2; step <= Size; step++) {

            // Scatter.
            spMatrixElement *pElement = FirstInCol[step];
            while (pElement != 0) {
                Dest[pElement->Row] = pElement->Real;
                pElement = pElement->NextInCol;
            }

            // Update column.
            spMatrixElement *pColumn = FirstInCol[step];
            while (pColumn->Row < step) {
                pElement = Diag[pColumn->Row];
                spREAL mult = Dest[pColumn->Row] * pElement->Real;
                pColumn->Real = mult;
                while ((pElement = pElement->NextInCol) != 0)
                    Dest[pElement->Row] -= mult * pElement->Real;
                pColumn = pColumn->NextInCol;
            }

            // Gather.
            pElement = Diag[step]->NextInCol;
            while (pElement != 0) {
                pElement->Real = Dest[pElement->Row];
                pElement = pElement->NextInCol;
            }

            // Check for singular matrix.
            if (Dest[step] == 0.0)
                return (ZeroPivot(step));
            Diag[step]->Real = 1.0 / Dest[step];
        }
    }
    else {
        for (int step = 2; step <= Size; step++) {
            if (!DoRealDirect[step]) {

                // Update column using indirect addressing scatter-gather.
                spREAL **pDest = (spREAL **)Intermediate;

                // Scatter.
                spMatrixElement *pElement = FirstInCol[step];
                while (pElement != 0) {
                    pDest[pElement->Row] = &pElement->Real;
                    pElement = pElement->NextInCol;
                }

                // Update column.
                spMatrixElement *pColumn = FirstInCol[step];
                while (pColumn->Row < step) {
                    pElement = Diag[pColumn->Row];
                    spREAL mult = (*pDest[pColumn->Row] *= pElement->Real);
                    while ((pElement = pElement->NextInCol) != 0)
                        *pDest[pElement->Row] -= mult * pElement->Real;
                    pColumn = pColumn->NextInCol;
                }

                // Check for singular matrix.
                if (Diag[step]->Real == 0.0)
                    return (ZeroPivot(step));
                Diag[step]->Real = 1.0 / Diag[step]->Real;
            }
            else {
                // Update column using direct addressing scatter-gather.
                spREAL *Dest = (spREAL *)Intermediate;

                // Scatter.
                spMatrixElement *pElement = FirstInCol[step];
                while (pElement != 0) {
                    Dest[pElement->Row] = pElement->Real;
                    pElement = pElement->NextInCol;
                }

                // Update column.
                spMatrixElement *pColumn = FirstInCol[step];
                while (pColumn->Row < step) {
                    pElement = Diag[pColumn->Row];
                    spREAL mult = Dest[pColumn->Row] * pElement->Real;
                    pColumn->Real = mult;
                    while ((pElement = pElement->NextInCol) != 0)
                        Dest[pElement->Row] -= mult * pElement->Real;
                    pColumn = pColumn->NextInCol;
                }

                // Gather.
                pElement = Diag[step]->NextInCol;
                while (pElement != 0) {
                    pElement->Real = Dest[pElement->Row];
                    pElement = pElement->NextInCol;
                }

                // Check for singular matrix.
                if (Dest[step] == 0.0)
                    return (ZeroPivot(step));
                Diag[step]->Real = 1.0 / Dest[step];
            }
        }
    }
    Factored = YES;
    return (Error = spOKAY);
#endif // SP_OPT_REAL
}


#if SP_BITFIELD

// Uncomment to enable consistency testing for debugging (slow!)
// #define TEST_BITS

//
// We will use a bit field to keep track of which elements have been
// allocated, which can speed element insertion and removal.  The row
// and column swapping during reordering uses a lot of these
// operations.
//
// The problem is that in order to find the insertion point of the
// singly-linked rows and columns, one has to walk the linked lists.
// This can be very time consuming for a heavily populated matrix.
// One could use doubly-linked lists, which allows elements to be
// unlinked quickly, but this doesn't help when linking to a location
// that does not already contain an element.
//
// The bit field allows finding the element that is allocated just
// before a given index.  This is the element to which a newly
// inserted element must be linked (if it exists).  A set bit tells us
// that the element exists, and the element can then be obtained from
// the hash table that was created during the build process.
//
// We actually create two bit fields, one for rows and one for
// columns.  There will be some memory overhead, but the time savings
// can be very worthwhile.
//

namespace {
    // Return the position of the least significant one bit, so if v is
    // 10110100 (base 2), the result will be 3.
    // ASSUMES 32 BIT INT!!!
    //
    int least_set(unsigned int v)
    {
        if (!v)
            return (0);
        v &= -v;  // only the least one is set
        unsigned int c = 1;
        if (v & 0xffff0000)
            c += 0x10;
        if (v & 0xff00ff00)
            c += 0x8;
        if (v & 0xf0f0f0f0)
            c += 0x4;
        if (v & 0xcccccccc)
            c += 0x2;
        if (v & 0xaaaaaaaa)
            c += 0x1;
        return (c);
    }
}


// Set or unset the flag at row,col.
//
void
spMatrixFrame::ba_setbit(int row, int col, int set)
{
    unsigned int sz = Size + 1;
    unsigned int nd = sz/32 + (sz & 31 ? 1 : 0);
    unsigned int mask = 0x80000000 >> (col & 31);

    // Row bit
    unsigned int icol = col/32;
    unsigned int *p = BitField[row];
    unsigned int d = p[icol];
    if (set)
        d |= mask;
    else
        d &= ~mask;
    p[icol] = d;

    // Column bit
    mask = 0x80000000 >> (row & 31);
    unsigned int irow = row/32 + nd;
    p = BitField[col];
    d = p[irow];
    if (set)
        d |= mask;
    else
        d &= ~mask;
    p[irow] = d;
}


// Return true if the flag at row,col is set.
//
bool
spMatrixFrame::ba_getbit(int row, int col)
{
    unsigned int mask = 0x80000000 >> (col & 31);
    col /= 32;
    return (BitField[row][col] & mask);
}


// Return the nearest flagged column to the left of row,col.
//
spMatrixElement *
spMatrixFrame::ba_left(int row, int col)
{
    unsigned int mask = 0x80000000 >> (col & 31);
    col /= 32;
    mask <<= 1;
    if (!mask) {
        if (!col)
            return (0);
        col--;
        mask = 0x1;
    }
    mask--;
    mask = ~mask;
    while (col >= 0) {
        unsigned int d = (BitField[row][col] & mask);
        mask = -1;
        int b = least_set(d);
        if (b) {
            col = (32*col + 32 - b);
            row = IntToExtRowMap[row];
            col = IntToExtColMap[col];
            return (sph_get(row, col));
        }
        col--;
    }
    return (0);
}


// Return the nearest flagged row above row,col.
//
spMatrixElement *
spMatrixFrame::ba_above(int row, int col)
{
    unsigned int sz = Size + 1;
    unsigned int nd = sz/32 + (sz & 31 ? 1 : 0);
    unsigned int mask = 0x80000000 >> (row & 31);
    row /= 32;
    mask <<= 1;
    if (!mask) {
        if (!row)
            return (0);
        row--;
        mask = 0x1;
    }
    mask--;
    mask = ~mask;
    while (row >= 0) {
        unsigned int d = (BitField[col][row + nd] & mask);
        mask = -1;
        int b = least_set(d);
        if (b) {
            row = (32*row + 32 - b);
            row = IntToExtRowMap[row];
            col = IntToExtColMap[col];
            return (sph_get(row, col));
        }
        row--;
    }
    return (0);

    /************************************************************
     * This is an alternative that doesn't require a second bit
     * field, unfortunately this is unacceptably slow.
     *
    unsigned int mask = 0x80000000 >> (col & 31);
    int mcol = col/32;

    row--;
    while (row > 0) {
        if (BitField[row][mcol] & mask) {
            row = IntToExtRowMap[row];
            col = IntToExtColMap[col];
            return (sph_get(row, col));
        }
        row--;
    }
    return (0);
    ************************************************************/
}


// Set up a bitfield for the sparse matrix.  A bit will be set for
// every allocated entry.
//
void
spMatrixFrame::ba_setup()
{
    unsigned int sz = Size + 1;
    unsigned int nd = sz/32 + (sz & 31 ? 1 : 0);

    // Double the size, for the column bits.
    nd += nd;

    BitField = new unsigned int*[sz];
    for (unsigned int i = 0; i < sz; i++) {
        BitField[i] = new unsigned int[nd];
        memset(BitField[i],0, nd*sizeof(unsigned int));
    }

#ifdef TEST_BITS
    unsigned int cnt = 0;
#endif
    for (int i = 1; i <= Size; i++) {
        spMatrixElement *p = FirstInCol[i];
        for ( ; p; p = p->NextInCol) {
            ba_setbit(p->Row, p->Col, 1);
#ifdef TEST_BITS
            cnt++;
            int row = IntToExtRowMap[p->Row];
            int col = IntToExtColMap[p->Col];
            spMatrixElement *q = sph_get(row, col);
            if (p != q) {
                PRINTF("ba_setup: hash table col inconsistency %lx (%d) %lx "
                    "(%d)\n", (unsigned long)p, p->Row, (unsigned long)q,
                    q->Row);
            }
#endif
        }
    }
#ifdef TEST_BITS
    unsigned int alc;
    ElementHashTab->stats(0, &alc);
    if (cnt != alc) {
        PRINTF("ba_setup: hash table allocation mismatch: counted %d "
            "alloc %d\n", cnt, alc);
    }
    for (int i = 1; i <= Size; i++) {
        spMatrixElement *p = FirstInRow[i];
        for ( ; p; p = p->NextInRow) {
            int row = IntToExtRowMap[p->Row];
            int col = IntToExtColMap[p->Col];
            spMatrixElement *q = sph_get(row, col);
            if (p != q) {
                PRINTF("ba_setup: hash table row inconsistency %lx (%d) %lx "
                    "(%d)\n", (unsigned long)p, p->Col, (unsigned long)q,
                    q->Col);
            }
        }
    }
    for (int i = 1; i <= Size; i++) {
        spMatrixElement *p = FirstInCol[i];
        for ( ; p; p = p->NextInCol) {
            spMatrixElement *qq = ba_left(p->Row, p->Col);
            if (qq) {
                if (qq->NextInRow != p)
                    PRINTF("ba_left inconsistency 1 %lx %lx\n",
                        (unsigned long)qq->NextInRow, (unsigned long)p);
            }
            else {
                if (p != FirstInRow[p->Row])
                    PRINTF("ba_left inconsistency 2 %lx (%d) %lx (%d)\n",
                        (unsigned long)FirstInRow[p->Row],
                        FirstInRow[p->Row]->Col, (unsigned long)p, p->Col);
            }
            qq = ba_above(p->Row, p->Col);
            if (qq) {
                if (qq->NextInCol != p)
                    PRINTF("ba_above inconsistency 1 %lx %lx\n",
                        (unsigned long)qq->NextInCol, (unsigned long)p);
            }
            else {
                if (p != FirstInCol[p->Col])
                    PRINTF("ba_above inconsistency 2 %lx (%d) %lx (%d)\n",
                        (unsigned long)FirstInCol[p->Col],
                        FirstInCol[p->Col]->Row, (unsigned long)p, p->Row);
            }
        }
    }
#endif
}


void
spMatrixFrame::ba_destroy()
{
    if (BitField == 0)
        return;
    unsigned int sz = Size + 1;
    for (unsigned int i = 0; i < sz; i++)
        delete [] BitField[i];
    delete [] BitField;
    BitField = 0;
}

#endif


#if SP_OPT_COMPLEX

//  FACTOR COMPLEX MATRIX
//  Private function
//
// This function is the companion function to spFactor(), it handles
// complex matrices.  It is otherwise identical.
//
//  >>> Returned:
//
//  The error code is returned.  Possible errors are listed below.
//
//  >>> Possible errors:
//
//  spSINGULAR
//  Error is cleared in this function.
//
int
spMatrixFrame::FactorComplexMatrix()
{
    ASSERT(Complex);

    spMatrixElement *pElement = Diag[1];
    if (E_MAG(pElement) == 0.0)
        return (ZeroPivot(1));
    // Cmplx expr: *pPivot = 1.0 / *pPivot
    CMPLX_RECIPROCAL(*pElement, *pElement);

    // Start factorization.
    if (PartitionMode == spINDIRECT_PARTITION) {
        // Update column using indirect addressing scatter-gather.
        spCOMPLEX **pDest = (spCOMPLEX **)Intermediate;

        for (int step = 2; step <= Size; step++) {

            // Scatter
            pElement = FirstInCol[step];
            while (pElement != 0) {
                pDest[pElement->Row] = (spCOMPLEX *)pElement;
                pElement = pElement->NextInCol;
            }

            // Update column.
            spMatrixElement *pColumn = FirstInCol[step];
            while (pColumn->Row < step) {
                pElement = Diag[pColumn->Row];
                // Cmplx expr: Mult = *pDest[pColumn->Row] * (1.0 / *pPivot)
                spCOMPLEX mult;
                CMPLX_MULT(mult, *pDest[pColumn->Row], *pElement);
                CMPLX_ASSIGN(*pDest[pColumn->Row], mult);
                while ((pElement = pElement->NextInCol) != 0) {
                   // Cmplx expr: *pDest[pElement->Row] -= Mult * pElement
                   CMPLX_MULT_SUBT_ASSIGN(*pDest[pElement->Row], mult,
                       *pElement);
                }
                pColumn = pColumn->NextInCol;
            }

            // Check for singular matrix.
            pElement = Diag[step];
            if (E_MAG(pElement) == 0.0)
                return (ZeroPivot(step));
            CMPLX_RECIPROCAL(*pElement, *pElement);
        }
    }
    else if (PartitionMode == spDIRECT_PARTITION) {

        // Update column using direct addressing scatter-gather.
        spCOMPLEX *Dest = (spCOMPLEX *)Intermediate;

        for (int step = 2; step <= Size; step++) {
            // Scatter.
            pElement = FirstInCol[step];
            while (pElement != 0) {
                Dest[pElement->Row] = *(spCOMPLEX *)pElement;
                pElement = pElement->NextInCol;
            }

            // Update column.
            spMatrixElement *pColumn = FirstInCol[step];
            while (pColumn->Row < step) {
                pElement = Diag[pColumn->Row];
                // Cmplx expr: Mult = Dest[pColumn->Row] * (1.0 / *pPivot)
                spCOMPLEX mult;
                CMPLX_MULT(mult, Dest[pColumn->Row], *pElement);
                CMPLX_ASSIGN(*pColumn, mult);
                while ((pElement = pElement->NextInCol) != 0) {
                    // Cmplx expr: Dest[pElement->Row] -= Mult * pElement
                    CMPLX_MULT_SUBT_ASSIGN(Dest[pElement->Row], mult,
                        *pElement);
                }
                pColumn = pColumn->NextInCol;
            }

            // Gather.
            pElement = Diag[step]->NextInCol;
            while (pElement != 0) {
                *(spCOMPLEX *)pElement = Dest[pElement->Row];
                pElement = pElement->NextInCol;
            }

            // Check for singular matrix.
            spCOMPLEX pivot = Dest[step];
            if (CMPLX_1_NORM(pivot) == 0.0)
                return (ZeroPivot(step));
            CMPLX_RECIPROCAL(*Diag[step], pivot);
        }
    }
    else {
        for (int step = 2; step <= Size; step++) {
            if (NOT DoCmplxDirect[step]) {

                // Update column using indirect addressing scatter-gather.
                spCOMPLEX **pDest = (spCOMPLEX **)Intermediate;

                // Scatter
                pElement = FirstInCol[step];
                while (pElement != 0) {
                    pDest[pElement->Row] = (spCOMPLEX *)pElement;
                    pElement = pElement->NextInCol;
                }

                // Update column.
                spMatrixElement *pColumn = FirstInCol[step];
                while (pColumn->Row < step) {
                    pElement = Diag[pColumn->Row];
                    // Cmplx expr: Mult = *pDest[pColumn->Row] * (1.0 / *pPivot)
                    spCOMPLEX mult;
                    CMPLX_MULT(mult, *pDest[pColumn->Row], *pElement);
                    CMPLX_ASSIGN(*pDest[pColumn->Row], mult);
                    while ((pElement = pElement->NextInCol) != 0) {
                       // Cmplx expr: *pDest[pElement->Row] -= Mult * pElement
                       CMPLX_MULT_SUBT_ASSIGN(*pDest[pElement->Row], mult,
                           *pElement);
                    }
                    pColumn = pColumn->NextInCol;
                }

                // Check for singular matrix.
                pElement = Diag[step];
                if (E_MAG(pElement) == 0.0)
                    return (ZeroPivot(step));
                CMPLX_RECIPROCAL(*pElement, *pElement);
           }
           else {
                // Update column using direct addressing scatter-gather.
                spCOMPLEX *Dest = (spCOMPLEX *)Intermediate;

                // Scatter.
                pElement = FirstInCol[step];
                while (pElement != 0) {
                    Dest[pElement->Row] = *(spCOMPLEX *)pElement;
                    pElement = pElement->NextInCol;
                }

                // Update column.
                spMatrixElement *pColumn = FirstInCol[step];
                while (pColumn->Row < step) {
                    pElement = Diag[pColumn->Row];
                    // Cmplx expr: Mult = Dest[pColumn->Row] * (1.0 / *pPivot)
                    spCOMPLEX mult;
                    CMPLX_MULT(mult, Dest[pColumn->Row], *pElement);
                    CMPLX_ASSIGN(*pColumn, mult);
                    while ((pElement = pElement->NextInCol) != 0) {
                        // Cmplx expr: Dest[pElement->Row] -= Mult * pElement
                        CMPLX_MULT_SUBT_ASSIGN(Dest[pElement->Row], mult,
                            *pElement);
                    }
                    pColumn = pColumn->NextInCol;
                }

                // Gather.
                pElement = Diag[step]->NextInCol;
                while (pElement != 0) {
                    *(spCOMPLEX *)pElement = Dest[pElement->Row];
                    pElement = pElement->NextInCol;
                }

                // Check for singular matrix.
                spCOMPLEX pivot = Dest[step];
                if (CMPLX_1_NORM(pivot) == 0.0)
                    return (ZeroPivot(step));
                CMPLX_RECIPROCAL(*Diag[step], pivot);
            }
        }
    }

    Factored = YES;
    return (Error = spOKAY);
}

#endif


//  PARTITION MATRIX
//
// This function determines the cost to factor each row using both
// direct and indirect addressing and decides, on a row-by-row basis,
// which addressing mode is fastest.  This information is used in
// spFactor() to speed the factorization.
//
// When factoring a previously ordered matrix using spFactor(), Sparse
// operates on a row-at-a-time basis.  For speed, on each step, the
// row being updated is copied into a full vector and the operations
// are performed on that vector.  This can be done one of two ways,
// either using direct addressing or indirect addressing.  Direct
// addressing is fastest when the matrix is relatively dense and
// indirect addressing is best when the matrix is quite sparse.  The
// user selects the type of partition used with Mode.  If Mode is set
// to spDIRECT_PARTITION, then the all rows are placed in the direct
// addressing partition.  Similarly, if Mode is set to
// spINDIRECT_PARTITION, then the all rows are placed in the indirect
// addressing partition.  By setting Mode to spAUTO_PARTITION, the
// user allows Sparse to select the partition for each row
// individually.  spFactor() generally runs faster if Sparse is
// allowed to choose its own partitioning, however choosing a
// partition is expensive.  The time required to choose a partition is
// of the same order of the cost to factor the matrix.  If you plan to
// factor a large number of matrices with the same structure, it is
// best to let Sparse choose the partition.  Otherwise, you should
// choose the partition based on the predicted density of the matrix.
//
//  >>> Arguments:
//
//  mode  <input>  (int)
//      Mode must be one of three special codes: spDIRECT_PARTITION,
//      spINDIRECT_PARTITION, or spAUTO_PARTITION.
//
void
spMatrixFrame::spPartition(int mode)
{
    if (Partitioned)
        return;
    Partitioned = YES;

    // If partition is specified by the user, this is easy.
    if (mode == spDEFAULT_PARTITION)
        mode = SP_OPT_DEFAULT_PARTITION;
    PartitionMode = mode;
    if (mode == spDIRECT_PARTITION || mode == spINDIRECT_PARTITION)
        return;

    // Using auto-mode.
#if SP_OPT_DEBUG
    ASSERT(mode == spAUTO_PARTITION);
#endif

    // Create DoDirect vectors for use in spFactor().
    int size = Size;
#if SP_OPT_REAL
    if (DoRealDirect == 0) {
        DoRealDirect = new spBOOLEAN[size+1];
        memset(DoRealDirect, 0, (size+1)*sizeof(spBOOLEAN));
    }
#endif
#if SP_OPT_COMPLEX
    if (DoCmplxDirect == 0) {
        DoCmplxDirect = new spBOOLEAN[size+1];
        memset(DoCmplxDirect, 0, (size+1)*sizeof(spBOOLEAN));
    }
#endif

    // Otherwise, count all operations needed in when factoring matrix.
    int *nc = (int *)MarkowitzRow;
    int *no = (int *)MarkowitzCol;
    int *nm = (int *)MarkowitzProd;

    // Start mock-factorization.
    for (int step = 1; step <= size; step++) {
        nc[step] = no[step] = nm[step] = 0;

        spMatrixElement *pElement = FirstInCol[step];
        while (pElement != 0) {
            nc[step]++;
            pElement = pElement->NextInCol;
        }

        spMatrixElement *pColumn = FirstInCol[step];
        while (pColumn->Row < step) {
            pElement = Diag[pColumn->Row];
            nm[step]++;
            while ((pElement = pElement->NextInCol) != 0)
                no[step]++;
            pColumn = pColumn->NextInCol;
        }
    }

    for (int step = 1; step <= size; step++) {
        // The following are just estimates based on a count on the number of
        // machine instructions used on each machine to perform the various
        // tasks.  It was assumed that each machine instruction required the
        // same amount of time (I don't believe this is true for the VAX, and
        // have no idea if this is true for the 68000 family).  For optimum
        // performance, these numbers should be tuned to the machine.
        //   Nc is the number of nonzero elements in the column.
        //   Nm is the number of multipliers in the column.
        //   No is the number of operations in the inner loop.

#define generic
#ifdef hp9000s300
#if SP_OPT_REAL
        DoRealDirect[step] = (nm[step] + no[step] > 3*nc[step] - 2*nm[step]);
#endif
#if SP_OPT_COMPLEX
        // On the hp350, it is never profitable to use direct for complex
        DoCmplxDirect[step] = NO;
#endif
#undef generic
#endif

#ifdef vax
#if SP_OPT_REAL
        DoRealDirect[step] = (nm[step] + no[step] > 3*nc[step] - 2*nm[step]);
#endif
#if SP_OPT_COMPLEX
        DoCmplxDirect[step] = (nm[step] + no[step] > 7*nc[step] - 4*nm[step]);
#endif
#undef generic
#endif

#ifdef generic
#if SP_OPT_REAL
        DoRealDirect[step] = (nm[step] + no[step] > 3*nc[step] - 2*nm[step]);
#endif
#if SP_OPT_COMPLEX
        DoCmplxDirect[step] = (nm[step] + no[step] > 7*nc[step] - 4*nm[step]);
#endif
#undef generic
#endif
    }

#if (ANNOTATE == FULL)
    {
        int Ops = 0;
        for (int step = 1; step <= size; step++)
            Ops += no[step];
        PRINTF("Operation count for inner loop of factorization = %d.\n", Ops);
    }
#endif
}


//  CREATE INTERNAL VECTORS
//  Private function
//
// Creates the Markowitz and Intermediate vectors.
//
void
spMatrixFrame::CreateInternalVectors()
{
    // Create Markowitz arrays.
    if (MarkowitzRow == 0) {
        MarkowitzRow = new int[Size+1];
        memset(MarkowitzRow, 0, (Size+1)*sizeof(int));
    }
    if (MarkowitzCol == 0) {
        MarkowitzCol = new int[Size+1];
        memset(MarkowitzCol, 0, (Size+1)*sizeof(int));
    }
    if (MarkowitzProd == 0) {
        MarkowitzProd = new long[Size+2];
        memset(MarkowitzProd, 0, (Size+2)*sizeof(long));
    }

    // Create Intermediate vectors for use in MatrixSolve.
#if SP_OPT_COMPLEX
    if (Intermediate == 0) {
        Intermediate = new spREAL[2*(Size+1)];
        memset(Intermediate, 0, 2*(Size+1)*sizeof(spREAL));
    }
#else
    if (Intermediate == 0) {
        Intermediate = new spREAL[Size+1];
        memset(Intermediate, 0, (Size+1)*sizeof(spREAL));
    }
#endif

    InternalVectorsAllocated = YES;
}


//  COUNT MARKOWITZ
//  Private function
//
// Scans Matrix to determine the Markowitz counts for each row and
// column.
//
//  >>> Arguments:
//
//  rhs  <input>  (spREAL*)
//      Representative right-hand side vector that is used to determine
//      pivoting order when the right hand side vector is sparse.  If
//      rhs is a 0 pointer then the RHS vector is assumed to be full
//      and it is not used when determining the pivoting order.
//
//  step  <input>  (int)
//     Index of the diagonal currently being eliminated.
//
//  >>> Local variables:
//
//  count  (int)
//     Temporary counting variable.
//  extRow  (int)
//     The external row number that corresponds to I.
//  pElement  (spMatrixElement*)
//     Pointer to matrix elements.
//
void
spMatrixFrame::CountMarkowitz(spREAL *rhs, int step)
{
    // Correct array pointer for SP_OPT_ARRAY_OFFSET.
#if NOT SP_OPT_ARRAY_OFFSET
#if SP_OPT_SEPARATED_COMPLEX_VECTORS OR NOT SP_OPT_COMPLEX
        if (rhs != 0)
            --rhs;
#else
        if (rhs != 0) {
            if (Complex)
                rhs -= 2;
            else
                --rhs;
        }
#endif
#endif

    // Generate MarkowitzRow Count for each row.
    for (int i = step; i <= Size; i++) {
        // Set count to -1 initially to remove count due to pivot element.
        int count = -1;
        spMatrixElement *pElement = FirstInRow[i];
        while (pElement != 0 AND pElement->Col < step)
            pElement = pElement->NextInRow;
        while (pElement != 0) {
            count++;
            pElement = pElement->NextInRow;
        }

        // Include nonzero elements in the rhs vector.
        int extRow = IntToExtRowMap[i];

#if SP_OPT_SEPARATED_COMPLEX_VECTORS OR NOT SP_OPT_COMPLEX
        if (rhs != 0)
            if (rhs[extRow] != 0.0)
                count++;
#else
        if (rhs != 0) {
            if (Complex) {
                if ((rhs[2*extRow] != 0.0) OR (rhs[2*extRow+1] != 0.0))
                    count++;
            }
            else if (rhs[I] != 0.0)
                count++;
        }
#endif
        MarkowitzRow[i] = count;
    }

    // Generate the MarkowitzCol count for each column.
    for (int i = step; i <= Size; i++) {
        // Set Count to -1 initially to remove count due to pivot element.
        int count = -1;
        spMatrixElement *pElement = FirstInCol[i];
        while (pElement != 0 AND pElement->Row < step)
            pElement = pElement->NextInCol;
        while (pElement != 0) {
            count++;
            pElement = pElement->NextInCol;
        }
        MarkowitzCol[i] = count;
    }
}


//  MARKOWITZ PRODUCTS
//  Private function
//
// Calculates MarkowitzProduct for each diagonal element from the
// Markowitz counts.
//
//  >>> Arguments:
//
//  step  <input>  (int)
//      Index of the diagonal currently being eliminated.
//
//  >>> Local Variables:
//
//  pMarkowitzProduct  (long *)
//      Pointer that points into MarkowitzProduct array. Is used to
//      sequentially access entries quickly.
//
//  pMarkowitzRow  (int *)
//      Pointer that points into MarkowitzRow array. Is used to sequentially
//      access entries quickly.
//
//  pMarkowitzCol  (int *)
//      Pointer that points into MarkowitzCol array. Is used to sequentially
//      access entries quickly.
//
//  product  (long)
//      Temporary storage for Markowitz product.
//
void
spMatrixFrame::MarkowitzProducts(int step)
{
    Singletons = 0;

    long *pMarkowitzProduct = &(MarkowitzProd[step]);
    int *pMarkowitzRow = &(MarkowitzRow[step]);
    int *pMarkowitzCol = &(MarkowitzCol[step]);

    for (int i = step; i <= Size; i++) {
        // If chance of overflow, use real numbers.
        if ((*pMarkowitzRow > SHRT_MAX AND
                *pMarkowitzCol != 0) OR
                (*pMarkowitzCol > SHRT_MAX AND
                *pMarkowitzRow != 0)) {
            double fproduct =
                (double)(*pMarkowitzRow++) * (double)(*pMarkowitzCol++);
            if (fproduct >= LONG_MAX)
                *pMarkowitzProduct++ = LONG_MAX;
            else
                *pMarkowitzProduct++ = (long)fproduct;
        }
        else {
            long product = *pMarkowitzRow++ * *pMarkowitzCol++;
            if ((*pMarkowitzProduct++ = product) == 0)
                Singletons++;
        }
    }
}


//  SEARCH FOR BEST PIVOT
//  Private function
//
// Performs a search to determine the element with the lowest
// Markowitz Product that is also acceptable.  An acceptable element
// is one that is larger than the AbsThreshold and at least as large
// as RelThreshold times the largest element in the same column.  The
// first step is to look for singletons if any exist.  If none are
// found, then all the diagonals are searched.  The diagonal is
// searched once quickly using the assumption that elements on the
// diagonal are large compared to other elements in their column, and
// so the pivot can be chosen only on the basis of the Markowitz
// criterion.  After a element has been chosen to be pivot on the
// basis of its Markowitz product, it is checked to see if it is large
// enough.  Waiting to the end of the Markowitz search to check the
// size of a pivot candidate saves considerable time, but is not
// guaranteed to find an acceptable pivot.  Thus if unsuccessful a
// second pass of the diagonal is made.  This second pass checks to
// see if an element is large enough during the search, not after it. 
// If still no acceptable pivot candidate has been found, the search
// expands to cover the entire matrix.
//
//  >>> Returned:
//
// A pointer to the element chosen to be pivot.  If every element in the
// matrix is zero, then 0 is returned.
//
//  >>> Arguments:
//
//  step  <input>  (int)
//      The row and column number of the beginning of the reduced submatrix.
//
//  >>> Local variables:
//
//  chosenPivot  (spMatrixElement*)
//      Pointer to element that has been chosen to be the pivot.
//
//  >>> Possible errors:
//  spSINGULAR
//  spSMALL_PIVOT
//
spMatrixElement*
spMatrixFrame::SearchForPivot(int step, int diagPivoting)
{
    // If singletons exist, look for an acceptable one to use as pivot.
    if (Singletons) {
        spMatrixElement *chosenPivot = SearchForSingleton(step);
        if (chosenPivot != 0) {
            PivotSelectionMethod = 's';
            return (chosenPivot);
        }
    }

#if SP_OPT_DIAGONAL_PIVOTING
    if (diagPivoting) {
        // Either no singletons exist or they weren't acceptable.  Take quick
        // first pass at searching diagonal.  First search for element on
        // diagonal of remaining submatrix with smallest Markowitz product,
        // then check to see if it okay numerically.  If not,
        // QuicklySearchDiagonal fails.

        spMatrixElement *chosenPivot = QuicklySearchDiagonal(step);
        if (chosenPivot != 0) {
            PivotSelectionMethod = 'q';
            return (chosenPivot);
        }

        // Quick search of diagonal failed, carefully search diagonal and
        // check each pivot candidate numerically before even tentatively
        // accepting it.

        chosenPivot = SearchDiagonal(step);
        if (chosenPivot != 0) {
            PivotSelectionMethod = 'd';
            return (chosenPivot);
        }
    }
#endif // SP_OPT_DIAGONAL_PIVOTING

    // No acceptable pivot found yet, search entire matrix.
    spMatrixElement *chosenPivot = SearchEntireMatrix(step);
    PivotSelectionMethod = 'e';

    return (chosenPivot);
}


//  SEARCH FOR SINGLETON TO USE AS PIVOT
//  Private function
//
// Performs a search to find a singleton to use as the pivot.  The
// first acceptable singleton is used.  A singleton is acceptable if
// it is larger in magnitude than the AbsThreshold and larger than
// RelThreshold times the largest of any other elements in the same
// column.  It may seem that a singleton need not satisfy the relative
// threshold criterion, however it is necessary to prevent excessive
// growth in the RHS from resulting in overflow during the forward and
// backward substitution.  A singleton does not need to be on the
// diagonal to be selected.
//
//  >>> Returned:
//
// A pointer to the singleton chosen to be pivot.  If no singleton is
// acceptable, return 0.
//
//  >>> Arguments:
//
//  step  <input>  (int)
//      Index of the diagonal currently being eliminated.
//
//  >>> Local variables:
//
//  chosenPivot  (spMatrixElement*)
//      Pointer to element that has been chosen to be the pivot.
//
//  pivotMag  (spREAL)
//      Magnitude of ChosenPivot.
//
//  singletons  (int)
//      The count of the number of singletons that can be used as pivots.
//      A local version of Singletons.
//
//  pMarkowitzProduct  (long *)
//      Pointer that points into MarkowitzProduct array.  It is used to
//      quickly access successive Markowitz products.
//
spMatrixElement*
spMatrixFrame::SearchForSingleton(int step)
{
    // Initialize pointer that is to scan through MarkowitzProduct vector.
    long *pMarkowitzProduct = &(MarkowitzProd[Size+1]);
    MarkowitzProd[Size+1] = MarkowitzProd[step];

    // Decrement the count of available singletons, on the assumption that an
    // acceptable one will be found.
    int singletons = Singletons--;

    // Assure that following while loop will always terminate, this is just
    // preventive medicine, if things are working right this should never
    // be needed.

    MarkowitzProd[step-1] = 0;

    while (singletons-- > 0) {
        // Singletons exist, find them

        // This is tricky.  Am using a pointer to sequentially step
        // through the MarkowitzProduct array.  Search terminates when
        // singleton (Product = 0) is found.  Note that the
        // conditional in the while statement (*pMarkowitzProduct) is
        // true as long as the MarkowitzProduct is not equal to zero. 
        // The row (and column) index on the diagonal is then
        // calculated by subtracting the pointer to the Markowitz
        // product of the first diagonal from the pointer to the
        // Markowitz product of the desired element, the singleton.
        //
        // Search proceeds from the end (high row and column numbers)
        // to the beginning (low row and column numbers) so that rows
        // and columns with large Markowitz products will tend to be
        // move to the bottom of the matrix.  However, choosing
        // Diag[Step] is desirable because it would require no row and
        // column interchanges, so inspect it first by putting its
        // Markowitz product at the end of the MarkowitzProd vector.

        while (*pMarkowitzProduct--) {
            // N bottles of beer on the wall;
            // N bottles of beer.
            // you take one down and pass it around;
            // N-1 bottles of beer on the wall.
        }
        int i = pMarkowitzProduct - MarkowitzProd + 1;

        // Assure that i is valid
        if (i < step)
            break;  // while (Singletons-- > 0)
        if (i > Size)
            i = step;

        // Singleton has been found in either/both row or/and column i.
        spMatrixElement *chosenPivot;
        if ((chosenPivot = Diag[i]) != 0) {
            // Singleton lies on the diagonal.
            spREAL pivotMag = E_MAG(chosenPivot);
            if (pivotMag > AbsThreshold AND
                    pivotMag > RelThreshold *
                    FindBiggestInColExclude(chosenPivot, step))
                return (chosenPivot);
        }
        else {
            // Singleton does not lie on diagonal, find it.
            if (MarkowitzCol[i] == 0) {
                chosenPivot = FirstInCol[i];
                while ((chosenPivot != 0) AND (chosenPivot->Row < step))
                    chosenPivot = chosenPivot->NextInCol;
                if (chosenPivot != 0) {
                    // Reduced column has no elements, matrix is singular.
                    break;
                }
                spREAL pivotMag = E_MAG(chosenPivot);
                if (pivotMag > AbsThreshold AND
                        pivotMag > RelThreshold *
                        FindBiggestInColExclude(chosenPivot, step))
                    return (chosenPivot);
                else {
                    if (MarkowitzRow[i] == 0) {
                        chosenPivot = FirstInRow[i];
                        while((chosenPivot != 0) AND (chosenPivot->Col < step))
                            chosenPivot = chosenPivot->NextInRow;
                        if (chosenPivot != 0) {
                            // Reduced row has no elements, matrix is singular.
                            break;
                        }
                        pivotMag = E_MAG(chosenPivot);
                        if (pivotMag > AbsThreshold AND
                                pivotMag > RelThreshold *
                                FindBiggestInColExclude(chosenPivot,
                                step))
                            return (chosenPivot);
                    }
                }
            }
            else {
                chosenPivot = FirstInRow[i];
                while ((chosenPivot != 0) AND (chosenPivot->Col < step))
                    chosenPivot = chosenPivot->NextInRow;
                if (chosenPivot != 0) {
                    // Reduced row has no elements, matrix is singular.
                    break;
                }
                spREAL pivotMag = E_MAG(chosenPivot);
                if (pivotMag > AbsThreshold AND
                        pivotMag > RelThreshold *
                        FindBiggestInColExclude(chosenPivot, step))
                    return (chosenPivot);
            }
        }
        // Singleton not acceptable (too small), try another.
    }

    // All singletons were unacceptable.  Restore Singletons count.
    // Initial assumption that an acceptable singleton would be found was
    // wrong.

    Singletons++;
    return (0);
}


#if SP_OPT_DIAGONAL_PIVOTING
#if SP_OPT_MODIFIED_MARKOWITZ

//  QUICK SEARCH OF DIAGONAL FOR PIVOT WITH MODIFIED MARKOWITZ CRITERION
//  Private function
//
// Searches the diagonal looking for the best pivot.  For a pivot to
// be acceptable it must be larger than the pivot RelThreshold times
// the largest element in its reduced column.  Among the acceptable
// diagonals, the one with the smallest MarkowitzProduct is sought. 
// Search terminates early if a diagonal is found with a
// MarkowitzProduct of one and its magnitude is larger than the other
// elements in its row and column.  Since its MarkowitzProduct is one,
// there is only one other element in both its row and column, and, as
// a condition for early termination, these elements must be located
// symmetricly in the matrix.  If a tie occurs between elements of
// equal MarkowitzProduct, then the element with the largest ratio
// between its magnitude and the largest element in its column is
// used.  The search will be terminated after a given number of ties
// have occurred and the best (largest ratio) of the tied element will
// be used as the pivot.  The number of ties that will trigger an
// early termination is MinMarkowitzProduct * TIES_MULTIPLIER.
//
//  >>> Returned:
//
// A pointer to the diagonal element chosen to be pivot.  If no
// diagonal is acceptable, a 0 is returned.
//
//  >>> Arguments:
//
//  step  <input>  (int)
//      Index of the diagonal currently being eliminated.
//
//  >>> Local variables:
//
//  chosenPivot  (spMatrixElement*)
//      Pointer to the element that has been chosen to be the pivot.
//
//  largestOffDiagonal  (spREAL)
//      Magnitude of the largest of the off-diagonal terms associated with
//      a diagonal with MarkowitzProduct equal to one.
//
//  magnitude  (spREAL)
//      Absolute value of diagonal element.
//
//  maxRatio  (spREAL)
//      Among the elements tied with the smallest Markowitz product, MaxRatio
//      is the best (smallest) ratio of LargestInCol to the diagonal Magnitude
//      found so far.  The smaller the ratio, the better numerically the
//      element will be as pivot.
//
//  minMarkowitzProduct  (long)
//      Smallest Markowitz product found of pivot candidates that lie along
//      diagonal.
//
//  numberOfTies  (int)
//      A count of the number of Markowitz ties that have occurred at current
//      MarkowitzProduct.
//
//  pDiag  (spMatrixElement*)
//      Pointer to current diagonal element.
//
//  pMarkowitzProduct  (long *)
//      Pointer that points into MarkowitzProduct array. It is used to quickly
//      access successive Markowitz products.
//
//  ratio  (spREAL)
//      For the current pivot candidate, Ratio is the ratio of the largest
//      element in its column (excluding itself) to its magnitude.
//
//  tiedElements  (spMatrixElement*[])
//      Array of pointers to the elements with the minimum Markowitz
//      product.
//
//  pOtherInCol  (spMatrixElement*)
//      When there is only one other element in a column other than the
//      diagonal, pOtherInCol is used to point to it.  Used when Markowitz
//      product is to determine if off diagonals are placed symmetricly.
//
//  pOtherInRow  (spMatrixElement*)
//      When there is only one other element in a row other than the diagonal,
//      pOtherInRow is used to point to it.  Used when Markowitz product is
//      to determine if off diagonals are placed symmetricly.
//
spMatrixElement*
spMatrixFrame::QuicklySearchDiagonal(int step)
{
    spMatrixElement *tiedElements[MAX_MARKOWITZ_TIES + 1];

    int numberOfTies = -1;
    long minMarkowitzProduct = LONG_MAX;
    long *pMarkowitzProduct = &(MarkowitzProd[Size+2]);
    MarkowitzProd[Size+1] = MarkowitzProd[step];

    // Assure that following while loop will always terminate.
    MarkowitzProd[step-1] = -1;

    // This is tricky.  Am using a pointer in the inner while loop to
    // sequentially step through the MarkowitzProduct array.  Search
    // terminates when the Markowitz product of zero placed at
    // location Step-1 is found.  The row (and column) index on the
    // diagonal is then calculated by subtracting the pointer to the
    // Markowitz product of the first diagonal from the pointer to the
    // Markowitz product of the desired element.  The outer for loop
    // is infinite, broken by using break.
    //
    // Search proceeds from the end (high row and column numbers) to
    // the beginning (low row and column numbers) so that rows and
    // columns with large Markowitz products will tend to be move to
    // the bottom of the matrix.  However, choosing Diag[Step] is
    // desirable because it would require no row and column
    // interchanges, so inspect it first by putting its Markowitz
    // product at the end of the MarkowitzProd vector.

    for (;;) {
        while (minMarkowitzProduct < *(--pMarkowitzProduct)) {
            // N bottles of beer on the wall;
            // N bottles of beer.
            // You take one down and pass it around;
            // N-1 bottles of beer on the wall.
        }

        int i = pMarkowitzProduct - MarkowitzProd;

        // Assure that i is valid; if i < Step, terminate search.
        if (i < step)
            break; // Endless for loop
        if (i > Size)
            i = step;

        spMatrixElement *pDiag;
        if ((pDiag = Diag[i]) == 0)
            continue; // Endless for loop.
        spREAL magnitude;
        if ((magnitude = E_MAG(pDiag)) <= AbsThreshold)
            continue; // Endless for loop.

        if (*pMarkowitzProduct == 1) {
            // Case where only one element exists in row and column
            // other than diagonal.

            // Find off diagonal elements
            spMatrixElement *pOtherInRow = pDiag->NextInRow;
            spMatrixElement *pOtherInCol = pDiag->NextInCol;
            if (pOtherInRow == 0 AND pOtherInCol == 0) {
                pOtherInRow = FirstInRow[i];
                while (pOtherInRow != 0) {
                    if (pOtherInRow->Col >= step AND pOtherInRow->Col != i)
                        break;
                    pOtherInRow = pOtherInRow->NextInRow;
                }
                pOtherInCol = FirstInCol[i];
                while (pOtherInCol != 0) {
                    if (pOtherInCol->Row >= step AND pOtherInCol->Row != i)
                        break;
                    pOtherInCol = pOtherInCol->NextInCol;
                }
            }

            // Accept diagonal as pivot if diagonal is larger than off
            // diagonals and the off diagonals are placed symmetricly.
            if (pOtherInRow != 0  AND  pOtherInCol != 0) {
                if (pOtherInRow->Col == pOtherInCol->Row) {
                    spREAL val1 = E_MAG(pOtherInRow);
                    spREAL val2 = E_MAG(pOtherInCol);
                    spREAL largestOffDiagonal = SPMAX(val1, val2);
                    if (magnitude >= largestOffDiagonal) {
                        // Accept pivot, it is unlikely to contribute excess
                        // error;
                        return (pDiag);
                    }
                }
            }
        }

        if (*pMarkowitzProduct < minMarkowitzProduct) {
            // Notice strict inequality in test. This is a new smallest
            // MarkowitzProduct.
            tiedElements[0] = pDiag;
            minMarkowitzProduct = *pMarkowitzProduct;
            numberOfTies = 0;
        }
        else {
            // This case handles Markowitz ties.
            if (numberOfTies < MAX_MARKOWITZ_TIES) {
                tiedElements[++numberOfTies] = pDiag;
                if (numberOfTies >= minMarkowitzProduct * TIES_MULTIPLIER)
                    break;
            }
        }
    }

    // Test to see if any element was chosen as a pivot candidate.
    if (numberOfTies < 0)
        return (0);

    // Determine which of tied elements is best numerically
    spMatrixElement *chosenPivot = 0;
    spREAL maxRatio = 1.0 / RelThreshold;

    for (int i = 0; i <= numberOfTies; i++) {
        spMatrixElement *pDiag = tiedElements[i];
        spREAL magnitude = E_MAG(pDiag);
        spREAL largestInCol = FindBiggestInColExclude(pDiag, step);
        spREAL ratio = largestInCol / magnitude;
        if (ratio < maxRatio) {
            chosenPivot = pDiag;
            maxRatio = ratio;
        }
    }
    return (chosenPivot);
}


#else // Not SP_OPT_MODIFIED_MARKOWITZ

//  QUICK SEARCH OF DIAGONAL FOR PIVOT WITH CONVENTIONAL MARKOWITZ
//  CRITERION
//  Private function
//
// Searches the diagonal looking for the best pivot.  For a pivot to
// be acceptable it must be larger than the pivot RelThreshold times
// the largest element in its reduced column.  Among the acceptable
// diagonals, the one with the smallest MarkowitzProduct is sought. 
// Search terminates early if a diagonal is found with a
// MarkowitzProduct of one and its magnitude is larger than the other
// elements in its row and column.  Since its MarkowitzProduct is one,
// there is only one other element in both its row and column, and, as
// a condition for early termination, these elements must be located
// symmetricly in the matrix.
//
//  >>> Returned:
//
// A pointer to the diagonal element chosen to be pivot.  If no
// diagonal is acceptable, a 0 is returned.
//
//  >>> Arguments:
//
//  step  <input>  (int)
//      Index of the diagonal currently being eliminated.
//
//  >>> Local variables:
//
//  chosenPivot  (spMatrixElement*)
//      Pointer to the element that has been chosen to be the pivot.
//
//  largestOffDiagonal  (spREAL)
//      Magnitude of the largest of the off-diagonal terms associated with
//      a diagonal with MarkowitzProduct equal to one.
//
//  magnitude  (spREAL)
//      Absolute value of diagonal element.
//
//  minMarkowitzProduct  (long)
//      Smallest Markowitz product found of pivot candidates which are
//      acceptable.
//
//  pDiag  (spMatrixElement*)
//      Pointer to current diagonal element.
//
//  pMarkowitzProduct  (long *)
//      Pointer that points into MarkowitzProduct array. It is used to quickly
//      access successive Markowitz products.
//
//  pOtherInCol  (spMatrixElement*)
//      When there is only one other element in a column other than the
//      diagonal, pOtherInCol is used to point to it.  Used when Markowitz
//      product is to determine if off diagonals are placed symmetricly.
//
//  pOtherInRow  (spMatrixElement*)
//      When there is only one other element in a row other than the diagonal,
//      pOtherInRow is used to point to it.  Used when Markowitz product is
//      to determine if off diagonals are placed symmetricly.
//
spMatrixElement*
spMatrixFrame::QuicklySearchDiagonal(int step)
{
    spMatrixElement *chosenPivot = 0;
    long minMarkowitzProduct = LONG_MAX;
    long *pMarkowitzProduct = &(MarkowitzProd[Size+2]);
    MarkowitzProd[Size+1] = MarkowitzProd[step];

    // Assure that following while loop will always terminate.
    MarkowitzProd[step-1] = -1;

    // This is tricky.  Am using a pointer in the inner while loop to
    // sequentially step through the MarkowitzProduct array.  Search
    // terminates when the Markowitz product of zero placed at location
    // Step-1 is found.  The row (and column) index on the diagonal is then
    // calculated by subtracting the pointer to the Markowitz product of
    // the first diagonal from the pointer to the Markowitz product of the
    // desired element. The outer for loop is infinite, broken by using
    // break.
    //
    // Search proceeds from the end (high row and column numbers) to the
    // beginning (low row and column numbers) so that rows and columns with
    // large Markowitz products will tend to be move to the bottom of the
    // matrix.  However, choosing Diag[Step] is desirable because it would
    // require no row and column interchanges, so inspect it first by
    // putting its Markowitz product at the end of the MarkowitzProd
    // vector.

    for (;;) {
        while (*(--pMarkowitzProduct) >= minMarkowitzProduct) {
            // Just passing through.
        }

        int i = pMarkowitzProduct - MarkowitzProd;

        // Assure that i is valid; if i < step, terminate search.
        if (i < step)
            break; // Endless for loop.
        if (i > Size)
            i = step;

        spMatrixElement *pDiag = Diag[i];
        if (pDiag == 0)
            continue; // Endless for loop
        spREAL magnitude;
        if ((magnitude = E_MAG(pDiag)) <= AbsThreshold)
            continue; // Endless for loop

        if (*pMarkowitzProduct == 1) {
            // Case where only one element exists in row and column other
            // than diagonal.

            // Find off-diagonal elements.
            spMatrixElement *pOtherInRow = pDiag->NextInRow;
            spMatrixElement *pOtherInCol = pDiag->NextInCol;
            if (pOtherInRow == 0 AND pOtherInCol == 0) {
                pOtherInRow = FirstInRow[i];
                while (pOtherInRow != 0) {
                    if (pOtherInRow->Col >= step AND pOtherInRow->Col != i)
                        break;
                    pOtherInRow = pOtherInRow->NextInRow;
                }
                pOtherInCol = FirstInCol[i];
                while (pOtherInCol != 0) {
                    if (pOtherInCol->Row >= step AND pOtherInCol->Row != i)
                        break;
                    pOtherInCol = pOtherInCol->NextInCol;
                }
            }

            // Accept diagonal as pivot if diagonal is larger than
            // off-diagonals and the off-diagonals are placed symmetricly.
            if (pOtherInRow != 0  AND  pOtherInCol != 0) {
                if (pOtherInRow->Col == pOtherInCol->Row) {
                    spREAL val1 = E_MAG(pOtherInRow);
                    spREAL val2 = E_MAG(pOtherInCol);
                    spREAL largestOffDiagonal = SPMAX(val1, val2);
                    if (magnitude >= largestOffDiagonal) {
                        // Accept pivot, it is unlikely to contribute excess
                        // error.
                        return (pDiag);
                    }
                }
            }
        }

        minMarkowitzProduct = *pMarkowitzProduct;
        chosenPivot = pDiag;
    }

    if (chosenPivot != 0) {
        spREAL largestInCol = FindBiggestInColExclude(chosenPivot,
            step);
        if (E_MAG(chosenPivot) <= RelThreshold * largestInCol)
            chosenPivot = 0;
    }
    return (chosenPivot);
}

#endif // Not SP_OPT_MODIFIED_MARKOWITZ


//  SEARCH DIAGONAL FOR PIVOT WITH MODIFIED MARKOWITZ CRITERION
//  Private function
//
// Searches the diagonal looking for the best pivot.  For a pivot to
// be acceptable it must be larger than the pivot RelThreshold times
// the largest element in its reduced column.  Among the acceptable
// diagonals, the one with the smallest MarkowitzProduct is sought. 
// If a tie occurs between elements of equal MarkowitzProduct, then
// the element with the largest ratio between its magnitude and the
// largest element in its column is used.  The search will be
// terminated after a given number of ties have occurred and the best
// (smallest ratio) of the tied element will be used as the pivot. 
// The number of ties that will trigger an early termination is
// MinMarkowitzProduct * TIES_MULTIPLIER.
//
//  >>> Returned:
//
// A pointer to the diagonal element chosen to be pivot.  If no
// diagonal is acceptable, a 0 is returned.
//
//  >>> Arguments:
//
//  step  <input>  (int)
//      Index of the diagonal currently being eliminated.
//
//  >>> Local variables:
//
//  chosenPivot  (spMatrixElement*)
//      Pointer to the element that has been chosen to be the pivot.
//
//  magnitude  (spREAL)
//      Absolute value of diagonal element.
//
//  minMarkowitzProduct  (long)
//      Smallest Markowitz product found of those pivot candidates which are
//      acceptable.
//
//  numberOfTies  (int)
//      A count of the number of Markowitz ties that have occurred at current
//      MarkowitzProduct.
//
//  pDiag  (spMatrixElement*)
//      Pointer to current diagonal element.
//
//  pMarkowitzProduct  (long*)
//      Pointer that points into MarkowitzProduct array. It is used to quickly
//      access successive Markowitz products.
//
//  ratio  (spREAL)
//      For the current pivot candidate, Ratio is the
//      Ratio of the largest element in its column to its magnitude.
//
//  ratioOfAccepted  (spREAL)
//      For the best pivot candidate found so far, RatioOfAccepted is the
//      Ratio of the largest element in its column to its magnitude.
//
spMatrixElement*
spMatrixFrame::SearchDiagonal(int step)
{
    spMatrixElement *chosenPivot = 0;
    long minMarkowitzProduct = LONG_MAX;
    long *pMarkowitzProduct = &(MarkowitzProd[Size+2]);
    MarkowitzProd[Size+1] = MarkowitzProd[step];
    int numberOfTies = 0;
    spREAL ratioOfAccepted = 0.0;

    // Start search of diagonal.
    for (int j = Size+1; j > step; j--) {
        if (*(--pMarkowitzProduct) > minMarkowitzProduct)
            continue; // for loop
        int i;
        if (j > Size)
            i = step;
        else
            i = j;
        spMatrixElement *pDiag = Diag[i];
        if (pDiag == 0)
            continue; // for loop
        spREAL magnitude;
        if ((magnitude = E_MAG(pDiag)) <= AbsThreshold)
            continue; // for loop

        // Test to see if diagonal's magnitude is acceptable.
        spREAL largestInCol = FindBiggestInColExclude(pDiag, step);
        if (magnitude <= RelThreshold * largestInCol)
            continue; // for loop

        if (*pMarkowitzProduct < minMarkowitzProduct) {
            // Notice strict inequality in test. This is a new smallest
            // MarkowitzProduct.
            chosenPivot = pDiag;
            minMarkowitzProduct = *pMarkowitzProduct;
            ratioOfAccepted = largestInCol / magnitude;
            numberOfTies = 0;
        }
        else {
            // This case handles Markowitz ties.
            numberOfTies++;
            spREAL ratio = largestInCol / magnitude;
            if (ratio < ratioOfAccepted) {
                chosenPivot = pDiag;
                ratioOfAccepted = ratio;
            }
            if (numberOfTies >= minMarkowitzProduct * TIES_MULTIPLIER)
                return (chosenPivot);
        }
    }
    return (chosenPivot);
}

#endif // SP_OPT_DIAGONAL_PIVOTING


//  SEARCH ENTIRE MATRIX FOR BEST PIVOT
//  Private function
//
// Performs a search over the entire matrix looking for the acceptable
// element with the lowest MarkowitzProduct.  If there are several
// that are tied for the smallest MarkowitzProduct, the tie is broken
// by using the ratio of the magnitude of the element being considered
// to the largest element in the same column.  If no element is
// acceptable then the largest element in the reduced submatrix is
// used as the pivot and the matrix is declared to be spSMALL_PIVOT. 
// If the largest element is zero, the matrix is declared to be
// spSINGULAR.
//
//  >>> Returned:
//
// A pointer to the diagonal element chosen to be pivot.  If no
// element is found, then 0 is returned and the matrix is spSINGULAR.
//
//  >>> Arguments:
//
//  step  <input>  (int)
//      Index of the diagonal currently being eliminated.
//
//  >>> Local variables:
//
//  chosenPivot  (spMatrixElement*)
//      Pointer to the element that has been chosen to be the pivot.
//
//  largestElementMag  (spREAL)
//      Magnitude of the largest element yet found in the reduced submatrix.
//
//  magnitude  (spREAL)
//      Absolute value of diagonal element.
//
//  minMarkowitzProduct  (long)
//      Smallest Markowitz product found of pivot candidates which are
//      acceptable.
//
//  numberOfTies  (int)
//      A count of the number of Markowitz ties that have occurred at current
//      MarkowitzProduct.
//
//  pElement  (spMatrixElement*)
//      Pointer to current element.
//
//  pLargestElement  (spMatrixElement*)
//      Pointer to the largest element yet found in the reduced submatrix.
//
//  product  (long)
//      Markowitz product for the current row and column.
//
//  ratio  (spREAL)
//      For the current pivot candidate, Ratio is the
//      Ratio of the largest element in its column to its magnitude.
//
//  ratioOfAccepted  (spREAL)
//      For the best pivot candidate found so far, RatioOfAccepted is the
//      Ratio of the largest element in its column to its magnitude.
//
//  >>> Possible errors:
//
//  spSINGULAR
//  spSMALL_PIVOT
//
spMatrixElement*
spMatrixFrame::SearchEntireMatrix(int step)
{
    spMatrixElement *chosenPivot = 0, *pLargestElement = 0;
    spREAL largestElementMag = 0.0;
    spREAL ratioOfAccepted = 0.0;
    long minMarkowitzProduct = LONG_MAX;
    int numberOfTies = 0;

    // Start search of matrix on column by column basis.
    for (int i = step; i <= Size; i++) {
        spMatrixElement *pElement = FirstInCol[i];

        while (pElement != 0 AND pElement->Row < step)
            pElement = pElement->NextInCol;

        spREAL largestInCol;
        if ((largestInCol = FindLargestInCol(pElement)) == 0.0)
            continue; // for loop

        while (pElement != 0) {
            // Check to see if element is the largest encountered so far.
            // If so, record its magnitude and address.
            spREAL magnitude;
            if ((magnitude = E_MAG(pElement)) > largestElementMag) {
                largestElementMag = magnitude;
                pLargestElement = pElement;
            }
            // Calculate element's MarkowitzProduct.
            long product = MarkowitzRow[pElement->Row] *
                MarkowitzCol[pElement->Col];

            // Test to see if element is acceptable as a pivot candidate.
            if ((product <= minMarkowitzProduct) AND
                    (magnitude > RelThreshold * largestInCol) AND
                    (magnitude > AbsThreshold)) {
                // Test to see if element has lowest MarkowitzProduct yet
                // found, or whether it is tied with an element found earlier.
                if (product < minMarkowitzProduct) {
                    // Notice strict inequality in test. This is a new
                    // smallest MarkowitzProduct.
                    chosenPivot = pElement;
                    minMarkowitzProduct = product;
                    ratioOfAccepted = largestInCol / magnitude;
                    numberOfTies = 0;
                }
                else {
                    // This case handles Markowitz ties.
                    numberOfTies++;
                    spREAL ratio = largestInCol / magnitude;
                    if (ratio < ratioOfAccepted) {
                        chosenPivot = pElement;
                        ratioOfAccepted = ratio;
                    }
                    if (numberOfTies >= minMarkowitzProduct * TIES_MULTIPLIER)
                        return (chosenPivot);
                }
            }
            pElement = pElement->NextInCol;
        }
    }

    if (chosenPivot != 0)
        return (chosenPivot);

    if (largestElementMag == 0.0) {
        Error = spSINGULAR;
        return (0);
    }

    Error = spSMALL_PIVOT;
    return (pLargestElement);
}


//  DETERMINE THE MAGNITUDE OF THE LARGEST ELEMENT IN A COLUMN
//  Private function
//
// This function searches a column and returns the magnitude of the
// largest element.  This function begins the search at the element
// pointed to by pElement, the parameter.
//
// The search is conducted by starting at the element specified by a
// pointer, which should be one below the diagonal, and moving down
// the column.  On the way down the column, the magnitudes of the
// elements are tested to see if they are the largest yet found.
//
//  >>> Returned:
//
// The magnitude of the largest element in the column below and
// including the one pointed to by the input parameter.
//
//  >>> Arguments:
//
//  pElement  <input>  (spMatrixElement*)
//      The pointer to the first element to be tested.  Also, used by the
//      function to access all lower elements in the column.
//
//  >>> Local variables:
//
//  largest  (spREAL)
//      The magnitude of the largest element.
//
//  magnitude  (spREAL)
//      The magnitude of the currently active element.
//
spREAL
spMatrixFrame::FindLargestInCol(spMatrixElement *pElement)
{
    // Search column for largest element beginning at pElement.
    spREAL largest = 0.0;
    while (pElement != 0) {
        spREAL magnitude = E_MAG(pElement);
        if (magnitude > largest)
            largest = magnitude;
        pElement = pElement->NextInCol;
    }
    return (largest);
}


//  DETERMINE THE MAGNITUDE OF THE LARGEST ELEMENT IN A COLUMN
//  EXCLUDING AN ELEMENT
//  Private function
//
// This function searches a column and returns the magnitude of the
// largest element.  One given element is specifically excluded from
// the search.
//
// The search is conducted by starting at the first element in the
// column and moving down the column until the active part of the
// matrix is entered, i.e. the reduced submatrix.  The rest of the
// column is then traversed looking for the largest element.
//
//  >>> Returned:
//
// The magnitude of the largest element in the active portion of the
// column, excluding the specified element, is returned.
//
//  >>> Arguments:
//
//  pElement  <input>  (spMatrixElement*)
//      The pointer to the element that is to be excluded from search. Column
//      to be searched is one that contains this element.  Also used to
//      access the elements in the column.
//
//  step  <input>  (int)
//      Index of the diagonal currently being eliminated.  Indicates where
//      the active part of the matrix begins.
//
//  >>> Local variables:
//
//  col  (int)
//      The number of the column to be searched.  Also the column number of
//      the element to be avoided in the search.
//
//  largest  (spREAL)
//      The magnitude of the largest element.
//
//  magnitude  (spREAL)
//      The magnitude of the currently active element.
//
//  row  (int)
//      The row number of element to be excluded from the search.
//
spREAL
spMatrixFrame::FindBiggestInColExclude(spMatrixElement *pElement, int step)
{
    int row = pElement->Row;
    int col = pElement->Col;
    pElement = FirstInCol[col];

    // Travel down column until reduced submatrix is entered.
    while ((pElement != 0) AND (pElement->Row < step))
        pElement = pElement->NextInCol;

    // Initialize the variable largest.
    spREAL largest;
    if (pElement->Row != row)
        largest = E_MAG(pElement);
    else
        largest = 0.0;

    // Search rest of column for largest element, avoiding excluded element.
    while ((pElement = pElement->NextInCol) != 0) {
        spREAL magnitude = E_MAG(pElement);
        if (magnitude > largest) {
            if (pElement->Row != row)
                largest = magnitude;
        }
    }
    return (largest);
}


//  EXCHANGE ROWS AND COLUMNS
//  Private function
//
// Exchanges two rows and two columns so that the selected pivot is
// moved to the upper left corner of the remaining submatrix.
//
//  >>> Arguments:
//
//  pPivot  <input>  (spMatrixElement*)
//      Pointer to the current pivot.
//
//  step  <input>  (int)
//      Index of the diagonal currently being eliminated.
//
//  >>> Local variables:
//
//  col  (int)
//      Column where the pivot was found.
//
//  row  (int)
//      Row where the pivot was found.
//
//  oldMarkowitzProd_Col  (long)
//      Markowitz product associated with the diagonal element in the row
//      the pivot was found in.
//
//  oldMarkowitzProd_Row  (long)
//      Markowitz product associated with the diagonal element in the column
//      the pivot was found in.
//
//  oldMarkowitzProd_Step  (long)
//      Markowitz product associated with the diagonal element that is being
//      moved so that the pivot can be placed in the upper left-hand corner
//      of the reduced submatrix.
//
void
spMatrixFrame::ExchangeRowsAndCols(spMatrixElement *pPivot, int step)
{
    int row = pPivot->Row;
    int col = pPivot->Col;
    PivotsOriginalRow = row;
    PivotsOriginalCol = col;

    if ((row == step) AND (col == step))
        return;

    // Exchange rows and columns.
    if (row == col) {
        RowExchange(step, row);
        ColExchange(step, col);
        SWAP(long, MarkowitzProd[step], MarkowitzProd[row]);
        SWAP(spMatrixElement*, Diag[row], Diag[step]);
    }
    else {
        // Initialize variables that hold old Markowitz products.
        long oldMarkowitzProd_Step = MarkowitzProd[step];
        long oldMarkowitzProd_Row = MarkowitzProd[row];
        long oldMarkowitzProd_Col = MarkowitzProd[col];

        // Exchange rows.
        if (row != step) {
            RowExchange(step, row);
            NumberOfInterchangesIsOdd = NOT NumberOfInterchangesIsOdd;
            MarkowitzProd[row] = MarkowitzRow[row] * MarkowitzCol[row];

            // Update singleton count.
            if ((MarkowitzProd[row] == 0) != (oldMarkowitzProd_Row == 0)) {
                if (oldMarkowitzProd_Row == 0)
                    Singletons--;
                else
                    Singletons++;
            }
        }

        // Exchange columns.
        if (col != step) {
            ColExchange(step, col);
            NumberOfInterchangesIsOdd = NOT NumberOfInterchangesIsOdd;
            MarkowitzProd[col] = MarkowitzCol[col] * MarkowitzRow[col];

            // Update singleton count.
            if ((MarkowitzProd[col] == 0) != (oldMarkowitzProd_Col == 0)) {
                if (oldMarkowitzProd_Col == 0)
                    Singletons--;
                else
                    Singletons++;
            }

            Diag[col] = FindElementInCol(FirstInCol + col, col, col, NO);
        }
        if (row != step)
            Diag[row] = FindElementInCol(FirstInCol + row, row, row, NO);
        Diag[step] = FindElementInCol(FirstInCol + step, step, step, NO);

        // Update singleton count.
        MarkowitzProd[step] = MarkowitzCol[step] * MarkowitzRow[step];
        if ((MarkowitzProd[step] == 0) != (oldMarkowitzProd_Step == 0)) {
            if (oldMarkowitzProd_Step == 0)
                Singletons--;
            else
                Singletons++;
        }
    }
}


//  EXCHANGE ROWS
//  Private function
//
// Performs all required operations to exchange two rows.  Those
// operations include:  swap FirstInRow pointers, fixing up the
// NextInCol pointers, swapping row indexes in spMatrixElements, and
// swapping Markowitz row counts.
//
//  >>> Arguments:
//
//  row1  <input>  (int)
//      Row index of one of the rows, becomes the smallest index.
//
//  row2  <input>  (int)
//      Row index of the other row, becomes the largest index.
//
//  >>> Local variables:
//
//  column  (int)
//      Column in which row elements are currently being exchanged.
//
//  row1Ptr  (spMatrixElement*)
//      Pointer to an element in Row1.
//
//  row2Ptr  (spMatrixElement*)
//      Pointer to an element in Row2.
//
//  element1  (spMatrixElement*)
//      Pointer to the element in Row1 to be exchanged.
//
//  element2  (spMatrixElement*)
//      Pointer to the element in Row2 to be exchanged.
//
void
spMatrixFrame::RowExchange(int row1, int row2)
{
    if (row1 > row2)
        SWAP(int, row1, row2);

    spMatrixElement *row1Ptr = FirstInRow[row1];
    spMatrixElement *row2Ptr = FirstInRow[row2];
    while (row1Ptr != 0 OR row2Ptr != 0) {
        // Exchange elements in rows while traveling from left to right.
        int column;
        spMatrixElement *element1, *element2;
        if (row1Ptr == 0) {
            column = row2Ptr->Col;
            element1 = 0;
            element2 = row2Ptr;
            row2Ptr = row2Ptr->NextInRow;
        }
        else if (row2Ptr == 0) {
            column = row1Ptr->Col;
            element1 = row1Ptr;
            element2 = 0;
            row1Ptr = row1Ptr->NextInRow;
        }
        else if (row1Ptr->Col < row2Ptr->Col) {
            column = row1Ptr->Col;
            element1 = row1Ptr;
            element2 = 0;
            row1Ptr = row1Ptr->NextInRow;
        }
        else if (row1Ptr->Col > row2Ptr->Col) {
            column = row2Ptr->Col;
            element1 = 0;
            element2 = row2Ptr;
            row2Ptr = row2Ptr->NextInRow;
        }
        else {
            // Row1Ptr->Col == Row2Ptr->Col
            column = row1Ptr->Col;
            element1 = row1Ptr;
            element2 = row2Ptr;
            row1Ptr = row1Ptr->NextInRow;
            row2Ptr = row2Ptr->NextInRow;
        }

        ExchangeColElements(row1, element1, row2, element2, column);
    }

    if (InternalVectorsAllocated)
        SWAP(int, MarkowitzRow[row1], MarkowitzRow[row2]);
    SWAP(spMatrixElement*, FirstInRow[row1], FirstInRow[row2]);
    SWAP(int, IntToExtRowMap[row1], IntToExtRowMap[row2]);
#if SP_OPT_TRANSLATE
    ExtToIntRowMap[ IntToExtRowMap[row1] ] = row1;
    ExtToIntRowMap[ IntToExtRowMap[row2] ] = row2;
#endif
}


//  EXCHANGE COLUMNS
//  Private function
//
// Performs all required operations to exchange two columns.  Those
// operations include:  swap FirstInCol pointers, fixing up the
// NextInRow pointers, swapping column indexes in spMatrixElements, and
// swapping Markowitz column counts.
//
//  >>> Arguments:
//
//  col1  <input>  (int)
//      Column index of one of the columns, becomes the smallest index.
//
//  col2  <input>  (int)
//      Column index of the other column, becomes the largest index
//
//  >>> Local variables:
//
//  row  (int)
//      Row in which column elements are currently being exchanged.
//
//  col1Ptr  (spMatrixElement*)
//      Pointer to an element in Col1.
//
//  col2Ptr  (spMatrixElement*)
//      Pointer to an element in Col2.
//
//  element1  (spMatrixElement*)
//      Pointer to the element in Col1 to be exchanged.
//
//  element2  (spMatrixElement*)
//      Pointer to the element in Col2 to be exchanged.
//
void
spMatrixFrame::ColExchange(int col1, int col2)
{
    if (col1 > col2)
        SWAP(int, col1, col2);

    spMatrixElement *col1Ptr = FirstInCol[col1];
    spMatrixElement *col2Ptr = FirstInCol[col2];
    while (col1Ptr != 0 OR col2Ptr != 0) {
        // Exchange elements in rows while traveling from top to bottom.
        int row;
        spMatrixElement *element1, *element2;
        if (col1Ptr == 0) {
            row = col2Ptr->Row;
            element1 = 0;
            element2 = col2Ptr;
            col2Ptr = col2Ptr->NextInCol;
        }
        else if (col2Ptr == 0) {
            row = col1Ptr->Row;
            element1 = col1Ptr;
            element2 = 0;
            col1Ptr = col1Ptr->NextInCol;
        }
        else if (col1Ptr->Row < col2Ptr->Row) {
            row = col1Ptr->Row;
            element1 = col1Ptr;
            element2 = 0;
            col1Ptr = col1Ptr->NextInCol;
        }
        else if (col1Ptr->Row > col2Ptr->Row) {
            row = col2Ptr->Row;
            element1 = 0;
            element2 = col2Ptr;
            col2Ptr = col2Ptr->NextInCol;
        }
        else {
            // col1Ptr->Row == col2Ptr->Row
            row = col1Ptr->Row;
            element1 = col1Ptr;
            element2 = col2Ptr;
            col1Ptr = col1Ptr->NextInCol;
            col2Ptr = col2Ptr->NextInCol;
        }

        ExchangeRowElements(col1, element1, col2, element2, row);
    }

    if (InternalVectorsAllocated)
        SWAP(int, MarkowitzCol[col1], MarkowitzCol[col2]);
    SWAP(spMatrixElement*, FirstInCol[col1], FirstInCol[col2]);
    SWAP(int, IntToExtColMap[col1], IntToExtColMap[col2]);
#if SP_OPT_TRANSLATE
    ExtToIntColMap[ IntToExtColMap[col1] ] = col1;
    ExtToIntColMap[ IntToExtColMap[col2] ] = col2;
#endif
}


//  EXCHANGE TWO ELEMENTS IN A COLUMN
//  Private function
//
// Performs all required operations to exchange two elements in a
// column.  Those operations are:  restring NextInCol pointers and
// swapping row indexes in the spMatrixElements.
//
//  >>> Arguments:
//
//  row1  <input>  (int)
//      Row of top element to be exchanged.
//
//  element1  <input>  (spMatrixElement*)
//      Pointer to top element to be exchanged.
//
//  row2  <input>  (int)
//      Row of bottom element to be exchanged.
//
//  element2  <input>  (spMatrixElement*)
//      Pointer to bottom element to be exchanged.
//
//  column <input>  (int)
//      Column that exchange is to take place in.
//
//  >>> Local variables:
//
//  elementAboveRow1  (spMatrixElement**)
//      Location of pointer which points to the element above Element1. This
//      pointer is modified so that it points to correct element on exit.
//
//  elementAboveRow2  (spMatrixElement**)
//      Location of pointer which points to the element above Element2. This
//      pointer is modified so that it points to correct element on exit.
//
//  elementBelowRow1  (spMatrixElement*)
//      Pointer to element below Element1.
//
//  elementBelowRow2  (spMatrixElement*)
//      Pointer to element below Element2.
//
//  pElement  (spMatrixElement*)
//      Pointer used to traverse the column.
//
void
spMatrixFrame::ExchangeColElements(int row1, spMatrixElement *element1,
    int row2, spMatrixElement *element2, int column)
{
    // Search to find the elementAboveRow1.
    spMatrixElement **elementAboveRow1, *pElement;
#if SP_BITFIELD
    pElement = ba_above(row1, column);
    if (pElement)
        elementAboveRow1 = &(pElement->NextInCol);
    else
        elementAboveRow1 = &FirstInCol[column];
    pElement = *elementAboveRow1;
#ifdef TEST_BITS
    spMatrixElement **tElementAboveRow1 = &FirstInCol[column];
    spMatrixElement *tpElement = *tElementAboveRow1;
    while (tpElement->Row < row1)
    {   tElementAboveRow1 = &tpElement->NextInCol;
        tpElement = *tElementAboveRow1;
    }
    if (tElementAboveRow1 != elementAboveRow1 || tpElement != pElement) {
        PRINTF("Row FOO 1\n");
        elementAboveRow1 = tElementAboveRow1;
        pElement = tpElement;
    }
#endif      
#else
    elementAboveRow1 = &(FirstInCol[column]);
    pElement = *elementAboveRow1;
    while (pElement->Row < row1) {
        elementAboveRow1 = &(pElement->NextInCol);
        pElement = *elementAboveRow1;
    }
#endif
    if (element1 != 0) {
        spMatrixElement *elementBelowRow1 = element1->NextInCol;
        if (element2 == 0) {
            // Element2 does not exist, move element1 down to row2.
            if (elementBelowRow1 != 0 AND elementBelowRow1->Row < row2) {
                // Element1 must be removed from linked list and moved.
                *elementAboveRow1 = elementBelowRow1;

                spMatrixElement **elementAboveRow2;
#if SP_BITFIELD
                pElement = ba_above(row2, column);
                if (pElement == element1)
                    elementAboveRow2 = elementAboveRow1;
                else
                    elementAboveRow2 = &pElement->NextInCol;
                pElement = pElement->NextInCol;
#ifdef TEST_BITS
                tpElement = elementBelowRow1;
                spMatrixElement **tElementAboveRow2;
                do {
                    tElementAboveRow2 = &tpElement->NextInCol;
                    tpElement = *tElementAboveRow2;
                }   while (tpElement AND tpElement->Row < row2);
                if (tElementAboveRow2 != elementAboveRow2 ||
                        tpElement != pElement) {
                    PRINTF("Row FOO 2\n");
                    elementAboveRow2 = tElementAboveRow2;
                    pElement = tpElement;
                }
#endif
#else
                // Search column for row2.
                pElement = elementBelowRow1;
                do {
                    elementAboveRow2 = &pElement->NextInCol;
                    pElement = *elementAboveRow2;
                } while (pElement != 0 AND pElement->Row < row2);
#endif

                // Place Element1 in row2.
                *elementAboveRow2 = element1;
                element1->NextInCol = pElement;
                *elementAboveRow1 = elementBelowRow1;
            }
            element1->Row = row2;
#if SP_BITFIELD
            ba_setbit(row1, column, 0);
            ba_setbit(row2, column, 1);
#endif
        }
        else {
            // Element2 does exist, and the two elements must be exchanged.
            if (elementBelowRow1->Row == row2) {
                // Element2 is just below element1, exchange them.
                element1->NextInCol = element2->NextInCol;
                element2->NextInCol = element1;
                *elementAboveRow1 = element2;
            }
            else {
                spMatrixElement **elementAboveRow2;
#if SP_BITFIELD
                pElement = ba_above(row2, column);
                elementAboveRow2 = &pElement->NextInCol;
                pElement = pElement->NextInCol;
#ifdef TEST_BITS
                tpElement = elementBelowRow1;
                spMatrixElement **tElementAboveRow2;
                do {
                    tElementAboveRow2 = &tpElement->NextInCol;
                    tpElement = *tElementAboveRow2;
                }   while (tpElement->Row < row2);
                if (tElementAboveRow2 != elementAboveRow2 ||
                        tpElement != pElement) {
                    PRINTF("Row FOO 3\n");
                    elementAboveRow2 = tElementAboveRow2;
                    pElement = tpElement;
                }
#endif
#else
                // Element2 is not just below Element1 and must be searched
                // for.
                pElement = elementBelowRow1;
                do {
                    elementAboveRow2 = &(pElement->NextInCol);
                    pElement = *elementAboveRow2;
                } while (pElement->Row < row2);
#endif

                spMatrixElement *elementBelowRow2 = element2->NextInCol;

                // Switch element1 and element2.
                *elementAboveRow1 = element2;
                element2->NextInCol = elementBelowRow1;
                *elementAboveRow2 = element1;
                element1->NextInCol = elementBelowRow2;
            }
            element1->Row = row2;
            element2->Row = row1;
        }
    }
    else {
        // Element1 does not exist.
        spMatrixElement *elementBelowRow1 = pElement;

        // Find element2.
        if (elementBelowRow1->Row != row2) {
            spMatrixElement **elementAboveRow2;
#if SP_BITFIELD
            pElement = ba_above(row2, column);
            if (pElement)
                elementAboveRow2 = &pElement->NextInCol;
            else
                elementAboveRow2 = &FirstInCol[column];
            pElement = *elementAboveRow2;
#ifdef TEST_BITS
            tpElement = elementBelowRow1;
            spMatrixElement **tElementAboveRow2;
            do {
                tElementAboveRow2 = &tpElement->NextInCol;
                tpElement = *tElementAboveRow2;
            }   while (tpElement->Row < row2);
            if (tElementAboveRow2 != elementAboveRow2 ||
                    tpElement != pElement) {
                PRINTF("Row FOO 4\n");
                elementAboveRow2 = tElementAboveRow2;
                pElement = tpElement;
            }
#endif
#else
            do {
                elementAboveRow2 = &(pElement->NextInCol);
                pElement = *elementAboveRow2;
            } while (pElement->Row < row2);
#endif

            // Move Element2 to row1.
            *elementAboveRow2 = element2->NextInCol;
            *elementAboveRow1 = element2;
            element2->NextInCol = elementBelowRow1;
        }
        element2->Row = row1;
#if SP_BITFIELD
        ba_setbit(row2, column, 0);
        ba_setbit(row1, column, 1);
#endif
    }
}


//  EXCHANGE TWO ELEMENTS IN A ROW
//  Private function
//
// Performs all required operations to exchange two elements in a row. 
// Those operations are:  restring NextInRow pointers and swapping
// column indexes in the spMatrixElements.
//
//  >>> Arguments:
//
//  col1  <input>  (int)
//      Col of left-most element to be exchanged.
//
//  element1  <input>  (spMatrixElement*)
//      Pointer to left-most element to be exchanged.
//
//  col2  <input>  (int)
//      Col of right-most element to be exchanged.
//
//  element2  <input>  (spMatrixElement*)
//      Pointer to right-most element to be exchanged.
//
//  row <input>  (int)
//      Row that exchange is to take place in.
//
//  >>> Local variables:
//
//  elementLeftOfCol1  (spMatrixElement**)
//      Location of pointer which points to the element to the left of
//      Element1. This pointer is modified so that it points to correct
//      element on exit.
//
//  elementLeftOfCol2  (spMatrixElement**)
//      Location of pointer which points to the element to the left of
//      Element2. This pointer is modified so that it points to correct
//      element on exit.
//
//  elementRightOfCol1  (spMatrixElement*)
//      Pointer to element right of Element1.
//
//  elementRightOfCol2  (spMatrixElement*)
//      Pointer to element right of Element2.
//
//  pElement  (spMatrixElement*)
//      Pointer used to traverse the row.
//
void
spMatrixFrame::ExchangeRowElements(int col1, spMatrixElement *element1,
    int col2, spMatrixElement *element2, int row)
{
    // Search to find the elementLeftOfCol1.
    spMatrixElement **elementLeftOfCol1, *pElement;
#if SP_BITFIELD  
    pElement = ba_left(row, col1);
    if (pElement)
        elementLeftOfCol1 = &pElement->NextInRow;
    else
        elementLeftOfCol1 = &FirstInRow[row];
    pElement = *elementLeftOfCol1;
#ifdef TEST_BITS
    spMatrixElement **tElementLeftOfCol1 = &FirstInRow[row];
    spMatrixElement *tpElement = *tElementLeftOfCol1;
    while (tpElement->Col < col1)
    {   tElementLeftOfCol1 = &tpElement->NextInRow;
        tpElement = *tElementLeftOfCol1;
    }
    if (tElementLeftOfCol1 != elementLeftOfCol1 || tpElement != pElement) {
        PRINTF("Col FOO 1\n");
        elementLeftOfCol1 = tElementLeftOfCol1;
        pElement = tpElement;
    }
#endif
#else
    elementLeftOfCol1 = &FirstInRow[row];
    pElement = *elementLeftOfCol1;
    while (pElement->Col < col1) {
        elementLeftOfCol1 = &(pElement->NextInRow);
        pElement = *elementLeftOfCol1;
    }
#endif
    if (element1 != 0) {
        spMatrixElement *elementRightOfCol1 = element1->NextInRow;
        if (element2 == 0) {
            // Element2 does not exist, move element1 to right to col2.
            if (elementRightOfCol1 != 0 AND elementRightOfCol1->Col < col2) {
                // Element1 must be removed from linked list and moved.
                *elementLeftOfCol1 = elementRightOfCol1;
                spMatrixElement **elementLeftOfCol2;
#if SP_BITFIELD
                pElement = ba_left(row, col2);
                if (pElement == element1)
                    elementLeftOfCol2 = elementLeftOfCol1;
                else
                    elementLeftOfCol2 = &pElement->NextInRow;
                pElement = pElement->NextInRow;
#ifdef TEST_BITS
                tpElement = elementRightOfCol1;
                spMatrixElement **tElementLeftOfCol2;
                do {
                    tElementLeftOfCol2 = &tpElement->NextInRow;
                    tpElement = *tElementLeftOfCol2;
                }   while (tpElement AND tpElement->Col < col2);
                if (tElementLeftOfCol2 != elementLeftOfCol2 ||
                        tpElement != pElement) {
                    PRINTF("Col FOO 2\n");
                    elementLeftOfCol2 = tElementLeftOfCol2;
                    pElement = tpElement;
                }
#endif
#else
                // Search row for col2.
                pElement = elementRightOfCol1;
                do {
                    elementLeftOfCol2 = &(pElement->NextInRow);
                    pElement = *elementLeftOfCol2;
                } while (pElement != 0 AND pElement->Col < col2);
#endif

                // Place element1 in col2.
                *elementLeftOfCol2 = element1;
                element1->NextInRow = pElement;
                *elementLeftOfCol1 = elementRightOfCol1;
            }
            element1->Col = col2;
#if SP_BITFIELD
            ba_setbit(row, col1, 0);
            ba_setbit(row, col2, 1);
#endif
        }
        else {
            // Element2 does exist, and the two elements must be exchanged.
            if (elementRightOfCol1->Col == col2) {
                // Element2 is just right of element1, exchange them.
                element1->NextInRow = element2->NextInRow;
                element2->NextInRow = element1;
                *elementLeftOfCol1 = element2;
            }
            else {
                spMatrixElement **elementLeftOfCol2;
#if SP_BITFIELD
                pElement = ba_left(row, col2);
                elementLeftOfCol2 = &pElement->NextInRow;
                pElement = pElement->NextInRow;
#ifdef TEST_BITS
                tpElement = elementRightOfCol1;
                spMatrixElement **tElementLeftOfCol2;
                do {
                    tElementLeftOfCol2 = &tpElement->NextInRow;
                    tpElement = *tElementLeftOfCol2;
                }   while (tpElement->Col < col2);
                if (tElementLeftOfCol2 != elementLeftOfCol2 ||
                        tpElement != pElement) {
                    PRINTF("Col FOO 3\n");
                    elementLeftOfCol2 = tElementLeftOfCol2;
                    pElement = tpElement;
                }
#endif
#else 
                // Element2 is not just right of Element1 and must be searched
                // for.
                pElement = elementRightOfCol1;
                do {
                    elementLeftOfCol2 = &(pElement->NextInRow);
                    pElement = *elementLeftOfCol2;
                } while (pElement->Col < col2);
#endif

                spMatrixElement *elementRightOfCol2 = element2->NextInRow;

                // Switch element1 and element2.
                *elementLeftOfCol1 = element2;
                element2->NextInRow = elementRightOfCol1;
                *elementLeftOfCol2 = element1;
                element1->NextInRow = elementRightOfCol2;
            }
            element1->Col = col2;
            element2->Col = col1;
        }
    }
    else {
        // Element1 does not exist.
        spMatrixElement *elementRightOfCol1 = pElement;

        // Find Element2
        spMatrixElement **elementLeftOfCol2;
        if (elementRightOfCol1->Col != col2) {
#if SP_BITFIELD
            pElement = ba_left(row, col2);
            if (pElement)
                elementLeftOfCol2 = &pElement->NextInRow;
            else
                elementLeftOfCol2 = &FirstInRow[row];
            pElement = *elementLeftOfCol2;
#ifdef TEST_BITS
            tpElement = elementRightOfCol1;
            spMatrixElement **tElementLeftOfCol2;
            do {
                tElementLeftOfCol2 = &tpElement->NextInRow;
                tpElement = *tElementLeftOfCol2;
            }   while (tpElement->Col < col2);
            if (tElementLeftOfCol2 != elementLeftOfCol2 ||
                    tpElement != pElement) {
                PRINTF("Col FOO 4\n");
                elementLeftOfCol2 = tElementLeftOfCol2;
                pElement = tpElement;
            }
#endif
#else
            do {
                elementLeftOfCol2 = &(pElement->NextInRow);
                pElement = *elementLeftOfCol2;
            } while (pElement->Col < col2);
#endif

            // Move element2 to col1.
            *elementLeftOfCol2 = element2->NextInRow;
            *elementLeftOfCol1 = element2;
            element2->NextInRow = elementRightOfCol1;
        }
        element2->Col = col1;
#if SP_BITFIELD
        ba_setbit(row, col1, 1);
        ba_setbit(row, col2, 0);
#endif
    }
}


//  PERFORM ROW AND COLUMN ELIMINATION ON REAL MATRIX
//  Private function
//
// Eliminates a single row and column of the matrix and leaves single
// row of the upper triangular matrix and a single column of the lower
// triangular matrix in its wake.  Uses Gauss's method.
//
//  >>> Argument:
//
//  pPivot  <input>  (spMatrixElement*)
//      Pointer to the current pivot.
//
//  >>> Local variables:
//
//  pLower  (spMatrixElement*)
//      Points to matrix element in lower triangular column.
//
//  pSub (spMatrixElement*)
//      Points to elements in the reduced submatrix.
//
//  row  (int)
//      Row index.
//
//  pUpper  (spMatrixElement*)
//      Points to matrix element in upper triangular row.
//
void
spMatrixFrame::RealRowColElimination(spMatrixElement *pPivot)
{
#if SP_OPT_REAL
#if SP_OPT_LONG_DBL_SOLVE
    if (LongDoubles) {
        // Test for zero pivot.
        if (LDBL(pPivot) == 0.0) {
            MatrixIsSingular(pPivot->Row);
            return;
        }
        LDBL(pPivot) = 1.0 / LDBL(pPivot);

        spMatrixElement *pUpper = pPivot->NextInRow;
        while (pUpper != 0) {
            // Calculate upper triangular element.
            LDBL(pUpper) *= LDBL(pPivot);

            spMatrixElement *pSub = pUpper->NextInCol;
            spMatrixElement *pLower = pPivot->NextInCol;
            while (pLower != 0) {
                int row = pLower->Row;

                // Find element in row that lines up with current lower
                // triangular element.
#if SP_BITFIELD
                if (pSub) {
                    pSub = ba_above(row, pSub->Col);
                    if (pSub)
                        pSub = pSub->NextInCol;   
                }
#else
                while (pSub != 0 AND pSub->Row < row)
                    pSub = pSub->NextInCol;
#endif

                // Test to see if desired element was not found, if not,
                // create fill-in.
                if (pSub == 0 OR pSub->Row > row) {
                    pSub = CreateFillin(row, pUpper->Col);
                    if (pSub == 0) {
                        Error = spNO_MEMORY;
                        return;
                    }
                }
                LDBL(pSub) -= LDBL(pUpper) * LDBL(pLower);
                pSub = pSub->NextInCol;
                pLower = pLower->NextInCol;
            }
            pUpper = pUpper->NextInRow;
        }
        return;
    }
#endif

    // Test for zero pivot.
    if (ABS(pPivot->Real) == 0.0) {
        MatrixIsSingular(pPivot->Row);
        return;
    }
    pPivot->Real = 1.0 / pPivot->Real;

    spMatrixElement *pUpper = pPivot->NextInRow;
    while (pUpper != 0) {
        // Calculate upper triangular element.
        pUpper->Real *= pPivot->Real;

        spMatrixElement *pSub = pUpper->NextInCol;
        spMatrixElement *pLower = pPivot->NextInCol;
        while (pLower != 0) {
            int row = pLower->Row;

            // Find element in row that lines up with current lower
            // triangular element.
#if SP_BITFIELD
            if (pSub) {
                pSub = ba_above(row, pSub->Col);
                if (pSub)
                    pSub = pSub->NextInCol;   
            }
#else
            while (pSub != 0 AND pSub->Row < row)
                pSub = pSub->NextInCol;
#endif

            // Test to see if desired element was not found, if not,
            // create fill-in.
            if (pSub == 0 OR pSub->Row > row) {
                pSub = CreateFillin(row, pUpper->Col);
                if (pSub == 0) {
                    Error = spNO_MEMORY;
                    return;
                }
            }
            pSub->Real -= pUpper->Real * pLower->Real;
            pSub = pSub->NextInCol;
            pLower = pLower->NextInCol;
        }
        pUpper = pUpper->NextInRow;
    }
#endif // SP_OPT_REAL
}


//  PERFORM ROW AND COLUMN ELIMINATION ON COMPLEX MATRIX
//  Private function
//
// Eliminates a single row and column of the matrix and leaves single
// row of the upper triangular matrix and a single column of the lower
// triangular matrix in its wake.  Uses Gauss's method.
//
//  >>> Argument:
//
//  pPivot  <input>  (spMatrixElement*)
//      Pointer to the current pivot.
//
//  >>> Local variables:
//
//  pLower  (spMatrixElement*)
//      Points to matrix element in lower triangular column.
//
//  pSub (spMatrixElement*)
//      Points to elements in the reduced submatrix.
//
//  row  (int)
//      Row index.
//
//  pUpper  (spMatrixElement*)
//      Points to matrix element in upper triangular row.
//
#if SP_OPT_COMPLEX
void
spMatrixFrame::ComplexRowColElimination(spMatrixElement *pPivot)
{
    // Test for zero pivot.
    if (E_MAG(pPivot) == 0.0) {
        MatrixIsSingular(pPivot->Row);
        return;
    }
    CMPLX_RECIPROCAL(*pPivot, *pPivot);

    spMatrixElement *pUpper = pPivot->NextInRow;
    while (pUpper != 0) {
        // Calculate upper triangular element.
        // Cmplx expr: *pUpper = *pUpper * (1.0 / *pPivot)
        CMPLX_MULT_ASSIGN(*pUpper, *pPivot);

        spMatrixElement *pSub = pUpper->NextInCol;
        spMatrixElement *pLower = pPivot->NextInCol;
        while (pLower != 0) {
            int row = pLower->Row;

            // Find element in row that lines up with current lower
            // triangular element.
#if SP_BITFIELD
            if (pSub) {
                pSub = ba_above(row, pSub->Col);
                if (pSub)
                    pSub = pSub->NextInCol;
            }
#else
            while (pSub != 0 AND pSub->Row < row)
                pSub = pSub->NextInCol;
#endif

            // Test to see if desired element was not found, if not,
            // create fill-in.
            if (pSub == 0 OR pSub->Row > row) {
                pSub = CreateFillin(row, pUpper->Col);
                if (pSub == 0) {
                    Error = spNO_MEMORY;
                    return;
                }
            }

            // Cmplx expr: pElement -= *pUpper * pLower
            CMPLX_MULT_SUBT_ASSIGN(*pSub, *pUpper, *pLower);
            pSub = pSub->NextInCol;
            pLower = pLower->NextInCol;
        }
        pUpper = pUpper->NextInRow;
    }
}
#endif


//  UPDATE MARKOWITZ NUMBERS
//  Private function
//
// Updates the Markowitz numbers after a row and column have been
// eliminated.  Also updates singleton count.
//
//  >>> Argument:
//
//  pPivot  <input>  (spMatrixElement*)
//      Pointer to the current pivot.
//
//  >>> Local variables:
//
//  row  (int)
//      Row index.
//
//  col  (int)
//      Column index.
//
//  colPtr  (spMatrixElement*)
//      Points to matrix element in upper triangular column.
//
//  rowPtr  (spMatrixElement*)
//      Points to matrix element in lower triangular row.
//
void
spMatrixFrame::UpdateMarkowitzNumbers(spMatrixElement *pPivot)
{
    int *markoRow = MarkowitzRow, *markoCol = MarkowitzCol;

    // Update Markowitz numbers.
    for (spMatrixElement *colPtr = pPivot->NextInCol; colPtr != 0;
            colPtr = colPtr->NextInCol) {
        int row = colPtr->Row;
        --markoRow[row];

        // Form Markowitz product while being cautious of overflows.
        if ((markoRow[row] > SHRT_MAX AND markoCol[row] != 0) OR
                (markoCol[row] > SHRT_MAX AND markoRow[row] != 0)) {

            double product = markoCol[row] * markoRow[row];
            if (product >= LONG_MAX)
                MarkowitzProd[row] = LONG_MAX;
            else
                MarkowitzProd[row] = (long)product;
        }
        else
            MarkowitzProd[row] = markoRow[row] * markoCol[row];
        if (markoRow[row] == 0)
            Singletons++;
    }

    for (spMatrixElement *rowPtr = pPivot->NextInRow; rowPtr != 0;
            rowPtr = rowPtr->NextInRow) {
        int col = rowPtr->Col;
        --markoCol[col];

        // Form Markowitz product while being cautious of overflows.
        if ((markoRow[col] > SHRT_MAX AND markoCol[col] != 0) OR
            (markoCol[col] > SHRT_MAX AND markoRow[col] != 0)) {

            double product = markoCol[col] * markoRow[col];
            if (product >= LONG_MAX)
                MarkowitzProd[col] = LONG_MAX;
            else
                MarkowitzProd[col] = (long)product;
        }
        else
            MarkowitzProd[col] = markoRow[col] * markoCol[col];
        if ((markoCol[col] == 0) AND (markoRow[col] != 0))
            Singletons++;
    }
}


//  CREATE FILL-IN
//  Pfrivate function
//
// This function is used to create fill-ins and splice them into the
// matrix.
//
//  >>> Returns:
//
// Pointer to fill-in.
//
//  >>> Arguments:
//
//  col  <input>  (int)
//      Column index for element.
//
//  row  <input>  (int)
//      Row index for element.
//
//  >>> Local variables:
//
//  pElement  (spMatrixElement*)
//      Pointer to an element in the matrix.
//
//  ppElementAbove  (spMatrixElement**)
//      This contains the address of the pointer to the element just above the
//      one being created. It is used to speed the search and it is updated
//      with address of the created element.
//
spMatrixElement*
spMatrixFrame::CreateFillin(int row, int col)
{
    // Find Element above fill-in.
#if SP_BITFIELD
    spMatrixElement *el = ba_above(row, col);
    spMatrixElement **ppElementAbove;
    if (el)
        ppElementAbove = &el->NextInCol;
    else
        ppElementAbove = &FirstInCol[col];
    spMatrixElement *pElement = *ppElementAbove;
#else
    spMatrixElement **ppElementAbove = &FirstInCol[col];
    spMatrixElement *pElement = *ppElementAbove;
    while (pElement != 0) {
        if (pElement->Row < row) {
            ppElementAbove = &pElement->NextInCol;
            pElement = *ppElementAbove;
        }
        else
            break;
    }
#endif

    // End of search, create the element.
    pElement = CreateElement(row, col, ppElementAbove, YES);
#if SP_BITFIELD
    ba_setbit(row, col, 1);
#endif
#if SP_BUILDHASH
    sph_add(IntToExtRowMap[row], IntToExtColMap[col], pElement);
#endif

    // Update Markowitz counts and products.
    MarkowitzProd[row] = ++MarkowitzRow[row] * MarkowitzCol[row];
    if ((MarkowitzRow[row] == 1) AND (MarkowitzCol[row] != 0))
        Singletons--;
    MarkowitzProd[col] = ++MarkowitzCol[col] * MarkowitzRow[col];
    if ((MarkowitzRow[col] != 0) AND (MarkowitzCol[col] == 1))
        Singletons--;

    return (pElement);
}


//  ZERO PIVOT ENCOUNTERED
//  Private function
//
// This function is called when a singular matrix is found.  It then
// records the current row and column and exits.
//
//  >>> Returned:
//
// The error code spSINGULAR or spZERO_DIAG is returned.
//
//  >>> Arguments:
//
//  step  <input>  (int)
//      Index of diagonal that is zero.
//
int
spMatrixFrame::MatrixIsSingular(int step)
{
    SingularRow = IntToExtRowMap[step];
    SingularCol = IntToExtColMap[step];
    if (Trace) {
        PRINTF("Singular! int diag %d (ext row %d, ext col %d) is zero.\n",
            step, SingularRow, SingularCol);
    }
    return (Error = spSINGULAR);
}


int
spMatrixFrame::ZeroPivot(int step)
{
    SingularRow = IntToExtRowMap[step];
    SingularCol = IntToExtColMap[step];
    if (Trace) {
        PRINTF("Zero pivot! int diag %d (ext row %d, ext col %d) is zero.\n",
            step, SingularRow, SingularCol);
    }
    return (Error = spZERO_DIAG);
}



//  WRITE STATUS
//  Private function
//
// Write a summary of important variables to standard output.
//
void
spMatrixFrame::WriteStatus(int step)
{
#if (ANNOTATE == FULL)
    PRINTF("Step = %1d   ", step);
    PRINTF("Pivot found at %1d,%1d using ",
        PivotsOriginalRow, PivotsOriginalCol);
    switch (PivotSelectionMethod) {
        case 's': PRINTF("SearchForSingleton\n");  break;
        case 'q': PRINTF("QuicklySearchDiagonal\n");  break;
        case 'd': PRINTF("SearchDiagonal\n");  break;
        case 'e': PRINTF("SearchEntireMatrix\n");  break;
    }

    PRINTF("MarkowitzRow     = ");
    for (int i = 1; i <= Size; i++)
        PRINTF("%2d  ", MarkowitzRow[i]);
    PRINTF("\n");

    PRINTF("MarkowitzCol     = ");
    for (int i = 1; i <= Size; i++)
        PRINTF("%2d  ", MarkowitzCol[i]);
    PRINTF("\n");

    PRINTF("MarkowitzProduct = ");
    for (int i = 1; i <= Size; i++)
        PRINTF("%2ld  ", MarkowitzProd[i]);
    PRINTF("\n");

    PRINTF("Singletons = %2d\n", Singletons);

    PRINTF("IntToExtRowMap     = ");
    for (int i = 1; i <= Size; i++)
        PRINTF("%2d  ", IntToExtRowMap[i]);
    PRINTF("\n");

    PRINTF("IntToExtColMap     = ");
    for (int i = 1; i <= Size; i++)
        PRINTF("%2d  ", IntToExtColMap[i]);
    PRINTF("\n");

    PRINTF("ExtToIntRowMap     = ");
    for (int i = 1; i <= ExtSize; i++)
        PRINTF("%2d  ", ExtToIntRowMap[i]);
    PRINTF("\n");

    PRINTF("ExtToIntColMap     = ");
    for (int i = 1; i <= ExtSize; i++)
        PRINTF("%2d  ", ExtToIntColMap[i]);
    PRINTF("\n\n");

//  spPrint(NO, YES);

#else
    (void)step;
#endif // ANNOTATE == FULL
}

