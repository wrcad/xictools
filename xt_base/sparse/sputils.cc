
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
//      Macros that customize the sparse matrix functions.
//  spmatrix.h
//      Macros and declarations to be imported by the user.
//  spmacros.h
//      Macro definitions for the sparse matrix functions.
//
#define spINSIDE_SPARSE
#include "config.h"
#include "spconfig.h"
#include "spmatrix.h"
#include "spmacros.h"
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif
#ifndef DBL_EPSILON
#define DBL_EPSILON 8.9e-15
#endif


//  MATRIX UTILITY MODULE
//
//  Author:                     Advising professor:
//      Kenneth S. Kundert          Alberto Sangiovanni-Vincentelli
//      UC Berkeley
//
// This file contains various optional utility functions.
//
//  >>> Public functions contained in this file:
//
//  spMNA_Preorder
//  spScale
//  spMultiply
//  spConstMult
//  spMultTransposed
//  spDeterminant
//  spStrip
//  spDeleteRowAndCol
//  spPseudoCondition
//  spCondition
//  spNorm
//  spLargestElement
//  spRoundoff
//  spErrorMessage
//  spError
//  spWhereSingular
//
//  >>> Private functions contained in this file:
//
//  CountTwins
//  SwapCols
//  ScaleComplexMatrix
//  ComplexMatrixMultiply
//  ComplexCondition


#if SP_OPT_MODIFIED_NODAL

//  PREORDER MODIFIED NODE ADMITTANCE MATRIX TO REMOVE ZEROS FROM DIAGONAL
//
// This function massages modified node admittance matrices to remove
// zeros from the diagonal.  It takes advantage of the fact that the
// row and column associated with a zero diagonal usually have
// structural ones placed symmetricly.  This function should be used
// only on modified node admittance matrices and should be executed
// after the matrix has been built but before the factorization
// begins.  It should be executed for the initial factorization only
// and should be executed before the rows have been linked.  Thus it
// should be run before using spScale(), spMultiply(),
// spDeleteRowAndCol(), or spNorm().
//
// This function exploits the fact that the structural ones are placed
// in the matrix in symmetric twins.  For example, the stamps for
// grounded and a floating voltage sources are
// grounded:              floating:
// [  x   x   1 ]         [  x   x   1 ]
// [  x   x     ]         [  x   x  -1 ]
// [  1         ]         [  1  -1     ]
// Notice for the grounded source, there is one set of twins, and for
// the floating, there are two sets.  We remove the zero from the diagonal
// by swapping the rows associated with a set of twins.  For example:
// grounded:              floating 1:            floating 2:
// [  1         ]         [  1  -1     ]         [  x   x   1 ]
// [  x   x     ]         [  x   x  -1 ]         [  1  -1     ]
// [  x   x   1 ]         [  x   x   1 ]         [  x   x  -1 ]
//
// It is important to deal with any zero diagonals that only have one
// set of twins before dealing with those that have more than one because
// swapping row destroys the symmetry of any twins in the rows being
// swapped, which may limit future moves.  Consider
// [  x   x   1     ]
// [  x   x  -1   1 ]
// [  1  -1         ]
// [      1         ]
// There is one set of twins for diagonal 4 and two for diagonal 3.
// Dealing with diagonal 4 first requires swapping rows 2 and 4.
// [  x   x   1     ]
// [      1         ]
// [  1  -1         ]
// [  x   x  -1   1 ]
// We can now deal with diagonal 3 by swapping rows 1 and 3.
// [  1  -1         ]
// [      1         ]
// [  x   x   1     ]
// [  x   x  -1   1 ]
// And we are done, there are no zeros left on the diagonal.  However, if
// we originally dealt with diagonal 3 first, we could swap rows 2 and 3
// [  x   x   1     ]
// [  1  -1         ]
// [  x   x  -1   1 ]
// [      1         ]
// Diagonal 4 no longer has a symmetric twin and we cannot continue.
//
// So we always take care of lone twins first.  When none remain, we
// choose arbitrarily a set of twins for a diagonal with more than one set
// and swap the rows corresponding to that twin.  We then deal with any
// lone twins that were created and repeat the procedure until no
// zero diagonals with symmetric twins remain.
//
// In this particular implementation, columns are swapped rather than rows.
// The algorithm used in this function was developed by Ken Kundert and
// Tom Quarles.
//
//  >>> Local variables;
//
//  j  (int)
//      Column with zero diagonal being currently considered.
//
//  pTwin1  (spMatrixElement*)
//      Pointer to the twin found in the column belonging to the zero diagonal.
//
//  pTwin2  (spMatrixElement*)
//      Pointer to the twin found in the row belonging to the zero diagonal.
//      belonging to the zero diagonal.
//
//  anotherPassNeeded  (spBOOLEAN)
//      Flag indicating that at least one zero diagonal with symmetric twins
//      remain.
//
//  startAt  (int)
//      Column number of first zero diagonal with symmetric twins.
//
//  swapped  (spBOOLEAN)
//      Flag indicating that columns were swapped on this pass.
//
//  twins  (int)
//      Number of symmetric twins corresponding to current zero diagonal.
//
void
spMatrixFrame::spMNA_Preorder()
{
    ASSERT(IS_VALID() AND NOT Factored);

    if (Matrix)
        return;

    if (RowsLinked)
        return;
    Reordered = YES;

    int startAt = 1;
    spMatrixElement *pTwin1 = 0, *pTwin2 = 0;
    for (;;) {
        spBOOLEAN swapped = NO;
        spBOOLEAN anotherPassNeeded = NO;

        // Search for zero diagonals with lone twins.
        for (int j = startAt; j <= Size; j++) {
            if (Diag[j] == 0) {
                int twins = CountTwins(j, &pTwin1, &pTwin2);
                if (twins == 1) {
                    // Lone twins found, swap rows.
                    SwapCols(pTwin1, pTwin2);
                    swapped = YES;
                }
                else if ((twins > 1) AND NOT anotherPassNeeded) {
                    anotherPassNeeded = YES;
                    startAt = j;
                }
            }
        }

        // All lone twins are gone, look for zero diagonals with
        // multiple twins.
        if (anotherPassNeeded) {
            for (int j = startAt; NOT swapped AND (j <= Size); j++) {
                if (Diag[j] == 0) {
                    CountTwins(j, &pTwin1, &pTwin2);
                    SwapCols(pTwin1, pTwin2);
                    swapped = YES;
                }
            }
        }
        if (!anotherPassNeeded)
            break;
    }
}


//  COUNT TWINS
//  Private function
//
// This function counts the number of symmetric twins associated with
// a zero diagonal and returns one set of twins if any exist.  The
// count is terminated early at two.
//
int
spMatrixFrame::CountTwins(int col, spMatrixElement **ppTwin1,
    spMatrixElement **ppTwin2)
{
    int twins = 0;
    spMatrixElement *pTwin1 = FirstInCol[col];
    while (pTwin1 != 0) {

#if SP_OPT_LONG_DBL_SOLVE
        if (LongDoubles) {
            if (LABS(LDBL(pTwin1)) == 1.0) {
                int row = pTwin1->Row;
                spMatrixElement *pTwin2 = FirstInCol[row];
                while ((pTwin2 != 0) AND (pTwin2->Row != col))
                    pTwin2 = pTwin2->NextInCol;
                if ((pTwin2 != 0) AND (LABS(LDBL(pTwin2)) == 1.0)) {
                    // Found symmetric twins.
                    if (++twins >= 2)
                        return (twins);
                    (*ppTwin1 = pTwin1)->Col = col;
                    (*ppTwin2 = pTwin2)->Col = row;
                }
            }
            pTwin1 = pTwin1->NextInCol;
            continue;
        }
#endif

        if (ABS(pTwin1->Real) == 1.0) {
            int row = pTwin1->Row;
            spMatrixElement *pTwin2 = FirstInCol[row];
            while ((pTwin2 != 0) AND (pTwin2->Row != col))
                pTwin2 = pTwin2->NextInCol;
            if ((pTwin2 != 0) AND (ABS(pTwin2->Real) == 1.0)) {
                // Found symmetric twins.
                if (++twins >= 2)
                    return (twins);
                (*ppTwin1 = pTwin1)->Col = col;
                (*ppTwin2 = pTwin2)->Col = row;
            }
        }
        pTwin1 = pTwin1->NextInCol;
    }
    return (twins);
}


//  SWAP COLUMNS
//  Private function
//
// This function swaps two columns and is applicable before the rows
// are linked.
//
void
spMatrixFrame::SwapCols(spMatrixElement *pTwin1, spMatrixElement *pTwin2)
{
    int col1 = pTwin1->Col, col2 = pTwin2->Col;

    SWAP (spMatrixElement*, FirstInCol[col1], FirstInCol[col2]);
    SWAP (int, IntToExtColMap[col1], IntToExtColMap[col2]);
#if SP_OPT_TRANSLATE
    ExtToIntColMap[IntToExtColMap[col2]] = col2;
    ExtToIntColMap[IntToExtColMap[col1]] = col1;
#endif

    Diag[col1] = pTwin2;
    Diag[col2] = pTwin1;
    NumberOfInterchangesIsOdd = NOT NumberOfInterchangesIsOdd;
}

#endif // SP_OPT_MODIFIED_NODAL


#if SP_OPT_SCALING

//  SCALE MATRIX
//
// This function scales the matrix to enhance the possibility of
// finding a good pivoting order.  Note that scaling enhances accuracy
// of the solution only if it affects the pivoting order, so it makes
// no sense to scale the matrix before spFactor().  If scaling is
// desired it should be done before spOrderAndFactor().  There are
// several things to take into account when choosing the scale
// factors.  First, the scale factors are directly multiplied against
// the elements in the matrix.  To prevent roundoff, each scale factor
// should be equal to an integer power of the number base of the
// machine.  Since most machines operate in base two, scale factors
// should be a power of two.  Second, the matrix should be scaled such
// that the matrix of element uncertainties is equilibrated.  Third,
// this function multiplies the scale factors by the elements, so if
// one row tends to have uncertainties 1000 times smaller than the
// other rows, then its scale factor should be 1024, not 1/1024. 
// Fourth, to save time, this function does not scale rows or columns
// if their scale factors are equal to one.  Thus, the scale factors
// should be normalized to the most common scale factor.  Rows and
// columns should be normalized separately.  For example, if the size
// of the matrix is 100 and 10 rows tend to have uncertainties near
// 1e-6 and the remaining 90 have uncertainties near 1e-12, then the
// scale factor for the 10 should be 1/1,048,576 and the scale factors
// for the remaining 90 should be 1.  Fifth, since this function
// directly operates on the matrix, it is necessary to apply the scale
// factors to the RHS and Solution vectors.  It may be easier to
// simply use spOrderAndFactor() on a scaled matrix to choose the
// pivoting order, and then throw away the matrix.  Subsequent
// factorizations, performed with spFactor(), will not need to have
// the RHS and Solution vectors descaled.  Lastly, this function
// should not be executed before the function spMNA_Preorder.
//
//  >>> Arguments:
//
//  solutionScaleFactors  <input>  (spREAL*)
//      The array of Solution scale factors.  These factors scale the columns.
//      All scale factors are real valued.
//
//  rhs_ScaleFactors  <input>  (spREAL*)
//      The array of RHS scale factors.  These factors scale the rows.
//      All scale factors are real valued.
//
//  >>> Local variables:
//
//  pElement  (spMatrixElement*)
//      Pointer to an element in the matrix.
//
//  pExtOrder  (int *)
//      Pointer into either IntToExtRowMap or IntToExtColMap vector. Used to
//      compensate for any row or column swaps that have been performed.
//
//  scaleFactor  (spREAL)
//      The scale factor being used on the current row or column.
//
void
spMatrixFrame::spScale(spREAL *rhs_ScaleFactors, spREAL *solutionScaleFactors)
{
    ASSERT(IS_VALID() AND NOT Factored);
    if (NOT RowsLinked)
        LinkRows();

#if SP_OPT_COMPLEX
    if (Complex) {
        ScaleComplexMatrix(rhs_ScaleFactors, solutionScaleFactors);
        return;
    }
#endif

#if SP_OPT_REAL
    // Correct pointers to arrays for SP_OPT_ARRAY_OFFSET.
#if NOT SP_OPT_ARRAY_OFFSET
    --rhs_ScaleFactors;
    --solutionScaleFactors;
#endif
#if SP_OPT_LONG_DBL_SOLVE
    if (LongDoubles) {
        // Scale Rows.
        int *pExtOrder = &IntToExtRowMap[1];
        for (int i = 1; i <= Size; i++) {
            long double scaleFactor;
            if ((scaleFactor = rhs_ScaleFactors[*(pExtOrder++)]) != 1.0) {
                spMatrixElement *pElement = FirstInRow[i];
                while (pElement != 0) {
                    LDBL(pElement) *= scaleFactor;
                    pElement = pElement->NextInRow;
                }
            }
        }

        // Scale Columns.
        pExtOrder = &IntToExtColMap[1];
        for (int i = 1; i <= Size; i++) {
            long double scaleFactor;
            if ((scaleFactor = solutionScaleFactors[*(pExtOrder++)]) != 1.0) {
                spMatrixElement *pElement = FirstInCol[i];
                while (pElement != 0) {
                    LDBL(pElement) *= scaleFactor;
                    pElement = pElement->NextInCol;
                }
            }
        }
        return;
    }
#endif

    // Scale Rows.
    int *pExtOrder = &IntToExtRowMap[1];
    for (int i = 1; i <= Size; i++) {
        spREAL scaleFactor;
        if ((scaleFactor = rhs_ScaleFactors[*(pExtOrder++)]) != 1.0) {
            spMatrixElement *pElement = FirstInRow[i];
            while (pElement != 0) {
                pElement->Real *= scaleFactor;
                pElement = pElement->NextInRow;
            }
        }
    }

    // Scale Columns.
    pExtOrder = &IntToExtColMap[1];
    for (int i = 1; i <= Size; i++) {
        spREAL scaleFactor;
        if ((scaleFactor = solutionScaleFactors[*(pExtOrder++)]) != 1.0) {
            spMatrixElement *pElement = FirstInCol[i];
            while (pElement != 0) {
                pElement->Real *= scaleFactor;
                pElement = pElement->NextInCol;
            }
        }
    }
#endif // SP_OPT_REAL
}

#endif // SP_OPT_SCALING


#if SP_OPT_COMPLEX AND SP_OPT_SCALING

//  SCALE COMPLEX MATRIX
//  Private function
//
// This function scales the matrix to enhance the possibility of
// finding a good pivoting order.  Note that scaling enhances accuracy
// of the solution only if it affects the pivoting order, so it makes
// no sense to scale the matrix before spFactor().  If scaling is
// desired it should be done before spOrderAndFactor().  There are
// several things to take into account when choosing the scale
// factors.  First, the scale factors are directly multiplied against
// the elements in the matrix.  To prevent roundoff, each scale factor
// should be equal to an integer power of the number base of the
// machine.  Since most machines operate in base two, scale factors
// should be a power of two.  Second, the matrix should be scaled such
// that the matrix of element uncertainties is equilibrated.  Third,
// this function multiplies the scale factors by the elements, so if
// one row tends to have uncertainties 1000 times smaller than the
// other rows, then its scale factor should be 1024, not 1/1024. 
// Fourth, to save time, this function does not scale rows or columns
// if their scale factors are equal to one.  Thus, the scale factors
// should be normalized to the most common scale factor.  Rows and
// columns should be normalized separately.  For example, if the size
// of the matrix is 100 and 10 rows tend to have uncertainties near
// 1e-6 and the remaining 90 have uncertainties near 1e-12, then the
// scale factor for the 10 should be 1/1,048,576 and the scale factors
// for the remaining 90 should be 1.  Fifth, since this function
// directly operates on the matrix, it is necessary to apply the scale
// factors to the RHS and Solution vectors.  It may be easier to
// simply use spOrderAndFactor() on a scaled matrix to choose the
// pivoting order, and then throw away the matrix.  Subsequent
// factorizations, performed with spFactor(), will not need to have
// the RHS and Solution vectors descaled.  Lastly, this function
// should not be executed before the function spMNA_Preorder.
//
//  >>> Arguments:
//
//  solutionScaleFactors  <input>  (spREAL*)
//      The array of Solution scale factors.  These factors scale the columns.
//      All scale factors are real valued.
//
//  rhS_ScaleFactors  <input>  (spREAL*)
//      The array of RHS scale factors.  These factors scale the rows.
//      All scale factors are real valued.
//
//  >>> Local variables:
//
//  pElement  (spMatrixElement*)
//      Pointer to an element in the matrix.
//
//  pExtOrder  (int *)
//      Pointer into either IntToExtRowMap or IntToExtColMap vector. Used to
//      compensate for any row or column swaps that have been performed.
//
//  ScaleFactor  (spREAL)
//      The scale factor being used on the current row or column.
//
void
spMatrixFrame::ScaleComplexMatrix(spREAL *rhs_ScaleFactors,
    spREAL *solutionScaleFactors)
{
    // Correct pointers to arrays for SP_OPT_ARRAY_OFFSET.
#if NOT SP_OPT_ARRAY_OFFSET
    --rhs_ScaleFactors;
    --solutionScaleFactors;
#endif

    // Scale Rows.
    int *pExtOrder = &IntToExtRowMap[1];
    for (int i = 1; i <= Size; i++) {
        spREAL scaleFactor;
        if ((scaleFactor = rhs_ScaleFactors[*(pExtOrder++)]) != 1.0) {
            spMatrixElement *pElement = FirstInRow[i];
            while (pElement != 0)
            {   pElement->Real *= scaleFactor;
                pElement->Imag *= scaleFactor;
                pElement = pElement->NextInRow;
            }
        }
    }

    // Scale Columns.
    pExtOrder = &IntToExtColMap[1];
    for (int i = 1; i <= Size; i++) {
        spREAL scaleFactor;
        if ((scaleFactor = solutionScaleFactors[*(pExtOrder++)]) != 1.0) {
            spMatrixElement *pElement = FirstInCol[i];
            while (pElement != 0)
            {   pElement->Real *= scaleFactor;
                pElement->Imag *= scaleFactor;
                pElement = pElement->NextInCol;
            }
        }
    }
}

#endif // SP_OPT_SCALING AND SP_OPT_COMPLEX


#if SP_OPT_MULTIPLICATION

//  MATRIX MULTIPLICATION
//
// Multiplies matrix by solution vector to find source vector. 
// Assumes matrix has not been factored.  This function can be used as
// a test to see if solutions are correct.  It should not be used
// before spMNA_Preorder().
//
//  >>> Arguments:
//
//  rhs  <output>  (spREAL*)
//      This is the right hand side. This is what is being solved for.
//
//  solution  <input>  (spREAL*)
//      This is the vector being multiplied by the matrix.
//
//  irhs  <output>  (spREAL*)
//      This is the imaginary portion of the right hand side. This is
//      what is being solved for.  This is only necessary if the matrix is
//      complex and SP_OPT_SEPARATED_COMPLEX_VECTORS is true.
//
//  isolution  <input>  (spREAL*)
//      This is the imaginary portion of the vector being multiplied
//      by the matrix. This is only necessary if the matrix is
//      complex and SP_OPT_SEPARATED_COMPLEX_VECTORS is true.
//
//  >>> Obscure Macros
//
//  IMAG_VECTORS_P
//      Replaces itself with `, spREAL* irhs, spREAL* isolution' if
//      the options SP_OPT_COMPLEX and SP_OPT_SEPARATED_COMPLEX_VECTORS are
//      set, otherwise it disappears without a trace.
//
//  IMAG_VECTORS
//      Replaces itself with `, irhs, isolution' if the options
//      SP_OPT_COMPLEX and SP_OPT_SEPARATED_COMPLEX_VECTORS are set,
//      otherwise it disappears without a trace.
//
void
spMatrixFrame::spMultiply(spREAL *rhs, spREAL *solution IMAG_VECTORS_P)
{
    ASSERT(NOT Factored);
    if (NOT RowsLinked)
        LinkRows();
    if (NOT InternalVectorsAllocated)
        CreateInternalVectors();

#if SP_OPT_COMPLEX
    if (Complex) {
        ComplexMatrixMultiply(rhs, solution IMAG_VECTORS);
        return;
    }
#endif

#if SP_OPT_REAL
#if NOT SP_OPT_ARRAY_OFFSET
    // Correct array pointers for SP_OPT_ARRAY_OFFSET
    --rhs;
    --solution;
#endif

#if SP_OPT_LONG_DBL_SOLVE
    if (LongDoubles) {
        // Initialize Intermediate vector with reordered Solution vector.
        long double *vector = (long double*)Intermediate;
        int *pExtOrder = &IntToExtColMap[Size];
        for (int i = Size; i > 0; i--)
            vector[i] = solution[*(pExtOrder--)];

        pExtOrder = &IntToExtRowMap[Size];
        for (int i = Size; i > 0; i--) {
            spMatrixElement *pElement = FirstInRow[i];
            long double sum = 0.0;

            while (pElement != 0) {
                sum += LDBL(pElement) * vector[pElement->Col];
                pElement = pElement->NextInRow;
            }
            rhs[*pExtOrder--] = sum;
        }
        return;
    }
#endif

    // Initialize Intermediate vector with reordered Solution vector.
    spREAL *vector = Intermediate;
    int *pExtOrder = &IntToExtColMap[Size];
    for (int i = Size; i > 0; i--)
        vector[i] = solution[*(pExtOrder--)];

    pExtOrder = &IntToExtRowMap[Size];
    for (int i = Size; i > 0; i--) {
        spMatrixElement *pElement = FirstInRow[i];
        spREAL sum = 0.0;

        while (pElement != 0) {
            sum += pElement->Real * vector[pElement->Col];
            pElement = pElement->NextInRow;
        }
        rhs[*pExtOrder--] = sum;
    }
#endif // SP_OPT_REAL
}


// new for spice3f
void
spMatrixFrame::spConstMult(spREAL constant)
{
#if SP_OPT_COMPLEX
    if (Complex) {
        for (int i = 1; i <= Size; i++) {
            for (spMatrixElement *e = FirstInCol[i]; e; e = e->NextInCol) {
                e->Real *= constant;
                e->Imag *= constant;
            }
        }
        return;
    }
#endif
#if SP_OPT_LONG_DBL_SOLVE
    if (LongDoubles) {
        for (int i = 1; i <= Size; i++) {
            for (spMatrixElement *e = FirstInCol[i]; e; e = e->NextInCol)
                LDBL(e) *= constant;
        }
        return;
    }
#endif
    for (int i = 1; i <= Size; i++) {
        for (spMatrixElement *e = FirstInCol[i]; e; e = e->NextInCol)
            e->Real *= constant;
    }
}

#endif // SP_OPT_MULTIPLICATION


#if SP_OPT_COMPLEX AND SP_OPT_MULTIPLICATION

//  COMPLEX MATRIX MULTIPLICATION
//  Private function
//
// Multiplies matrix by solution vector to find source vector. 
// Assumes matrix has not been factored.  This function can be used as
// a test to see if solutions are correct.
//
//  >>> Arguments:
//
//  rhs  <output>  (spREAL*)
//      This is the right hand side. This is what is being solved for.
//      This is only the real portion of the right-hand side if the matrix
//      is complex and SP_OPT_SEPARATED_COMPLEX_VECTORS is set true.
//
//  solution  <input>  (spREAL*)
//      This is the vector being multiplied by the matrix. This is only
//      the real portion if the matrix is complex and
//      SP_OPT_SEPARATED_COMPLEX_VECTORS is set true.
//
//  irhs  <output>  (spREAL*)
//      This is the imaginary portion of the right hand side. This is
//      what is being solved for.  This is only necessary if the matrix is
//      complex and SP_OPT_SEPARATED_COMPLEX_VECTORS is true.
//
//  isolution  <input>  (spREAL*)
//      This is the imaginary portion of the vector being multiplied
//      by the matrix. This is only necessary if the matrix is
//      complex and SP_OPT_SEPARATED_COMPLEX_VECTORS is true.
//
//  >>> Obscure Macros
//
//  IMAG_VECTORS_P
//      Replaces itself with `, spREAL* irhs, spREAL* isolution' if
//      the options SP_OPT_COMPLEX and SP_OPT_SEPARATED_COMPLEX_VECTORS are
//      set, otherwise it disappears without a trace.
//
void
spMatrixFrame::ComplexMatrixMultiply(spREAL *rhs, spREAL *solution
    IMAG_VECTORS_P)
{
    // Correct array pointers for SP_OPT_ARRAY_OFFSET.
#if NOT SP_OPT_ARRAY_OFFSET
#if SP_OPT_SEPARATED_COMPLEX_VECTORS
    --rhs;              --irhs;
    --solution;         --isolution;
#else
    rhs -= 2;           solution -= 2;
#endif
#endif

    // Initialize Intermediate vector with reordered solution vector.
    spCOMPLEX *Vector = (spCOMPLEX *)Intermediate;
    int *pExtOrder = &IntToExtColMap[Size];

#if SP_OPT_SEPARATED_COMPLEX_VECTORS
    for (int i = Size; i > 0; i--) {
        Vector[i].Real = solution[*pExtOrder];
        Vector[i].Imag = isolution[*(pExtOrder--)];
    }
#else
    for (int i = Size; i > 0; i--)
        Vector[i] = ((spCOMPLEX *)Solution)[*(pExtOrder--)];
#endif

    pExtOrder = &IntToExtRowMap[Size];
    for (int i = Size; i > 0; i--) {
        spMatrixElement *pElement = FirstInRow[i];
        spCOMPLEX sum;
        sum.Real = sum.Imag = 0.0;

        while (pElement != 0) {
            // Cmplx expression : Sum += Element * Vector[Col]
            CMPLX_MULT_ADD_ASSIGN(sum, *pElement, Vector[pElement->Col]);
            pElement = pElement->NextInRow;
        }

#if SP_OPT_SEPARATED_COMPLEX_VECTORS
        rhs[*pExtOrder] = sum.Real;
        irhs[*pExtOrder--] = sum.Imag;
#else
        ((spCOMPLEX *)rhs)[*pExtOrder--] = sum;
#endif
    }
}

#endif // SP_OPT_COMPLEX AND SP_OPT_MULTIPLICATION


#if SP_OPT_MULTIPLICATION AND SP_OPT_TRANSPOSE

//  TRANSPOSED MATRIX MULTIPLICATION
//
// Multiplies transposed matrix by solution vector to find source
// vector.  Assumes matrix has not been factored.  This function can
// be used as a test to see if solutions are correct.  It should not
// be used before spMNA_Preorder().
//
//  >>> Arguments:
//
//  rhs  <output>  (spREAL*)
//      This is the right hand side. This is what is being solved for.
//
//  solution  <input>  (spREAL*)
//      This is the vector being multiplied by the matrix.
//
//  irhs  <output>  (spREAL*)
//      This is the imaginary portion of the right hand side. This is
//      what is being solved for.  This is only necessary if the matrix is
//      complex and SP_OPT_SEPARATED_COMPLEX_VECTORS is true.
//
//  isolution  <input>  (spREAL*)
//      This is the imaginary portion of the vector being multiplied
//      by the matrix. This is only necessary if the matrix is
//      complex and SP_OPT_SEPARATED_COMPLEX_VECTORS is true.
//
//  >>> Obscure Macros
//
//  IMAG_VECTORS_P
//      Replaces itself with `, spREAL* irhs, spREAL* isolution' if
//      the options SP_OPT_COMPLEX and SP_OPT_SEPARATED_COMPLEX_VECTORS are
//      set, otherwise it disappears without a trace.
//
//  IMAG_VECTORS
//      Replaces itself with `, irhs, isolution' if the options
//      SP_OPT_COMPLEX and SP_OPT_SEPARATED_COMPLEX_VECTORS are set,
//      otherwise it disappears without a trace.
//
void
spMatrixFrame::spMultTransposed(spREAL *rhs, spREAL *solution IMAG_VECTORS_P)
{
    ASSERT(NOT Factored);
    if (NOT InternalVectorsAllocated)
        CreateInternalVectors();

#if SP_OPT_COMPLEX
    if (Complex) {
        ComplexTransposedMatrixMultiply(rhs, solution IMAG_VECTORS);
        return;
    }
#endif

#if SP_OPT_REAL
#if NOT SP_OPT_ARRAY_OFFSET
    // Correct array pointers for SP_OPT_ARRAY_OFFSET.
    --rhs;
    --solution;
#endif

#if SP_OPT_LONG_DBL_SOLVE
    if (LongDoubles) {
        // Initialize Intermediate vector with reordered Solution vector.
        long double *vector = (long double*)Intermediate;
        int *pExtOrder = &IntToExtRowMap[Size];
        for (int i = Size; i > 0; i--)
            vector[i] = solution[*(pExtOrder--)];

        pExtOrder = &IntToExtColMap[Size];
        for (int i = Size; i > 0; i--) {
            spMatrixElement *pElement = FirstInCol[i];
            long double sum = 0.0;

            while (pElement != 0) {
                sum += LDBL(pElement) * vector[pElement->Row];
                pElement = pElement->NextInCol;
            }
            rhs[*pExtOrder--] = sum;
        }
        return;
    }
#endif

    // Initialize Intermediate vector with reordered Solution vector.
    spREAL *vector = Intermediate;
    int *pExtOrder = &IntToExtRowMap[Size];
    for (int i = Size; i > 0; i--)
        vector[i] = solution[*(pExtOrder--)];

    pExtOrder = &IntToExtColMap[Size];
    for (int i = Size; i > 0; i--) {
        spMatrixElement *pElement = FirstInCol[i];
        spREAL sum = 0.0;

        while (pElement != 0) {
            sum += pElement->Real * vector[pElement->Row];
            pElement = pElement->NextInCol;
        }
        rhs[*pExtOrder--] = sum;
    }
#endif // SP_OPT_REAL
}

#endif // SP_OPT_MULTIPLICATION AND SP_OPT_TRANSPOSE


#if SP_OPT_COMPLEX AND SP_OPT_MULTIPLICATION AND SP_OPT_TRANSPOSE

//  COMPLEX TRANSPOSED MATRIX MULTIPLICATION
//  Private function
//
// Multiplies transposed matrix by solution vector to find source
// vector.  Assumes matrix has not been factored.  This function can
// be used as a test to see if solutions are correct.
//
//  >>> Arguments:
//
//  rhs  <output>  (spREAL*)
//      This is the right hand side. This is what is being solved for.
//      This is only the real portion of the right-hand side if the matrix
//      is complex and SP_OPT_SEPARATED_COMPLEX_VECTORS is set true.
//
//  solution  <input>  (spREAL*)
//      This is the vector being multiplied by the matrix. This is only
//      the real portion if the matrix is complex and
//      SP_OPT_SEPARATED_COMPLEX_VECTORS is set true.
//
//  irhs  <output>  (spREAL*)
//      This is the imaginary portion of the right hand side. This is
//      what is being solved for.  This is only necessary if the matrix is
//      complex and SP_OPT_SEPARATED_COMPLEX_VECTORS is true.
//
//  isolution  <input>  (spREAL*)
//      This is the imaginary portion of the vector being multiplied
//      by the matrix. This is only necessary if the matrix is
//      complex and SP_OPT_SEPARATED_COMPLEX_VECTORS is true.
//
//  >>> Obscure Macros
//
//  IMAG_VECTORS_P
//      Replaces itself with `, spREAL* irhs, spREAL* isolution' if
//      the options SP_OPT_COMPLEX and SP_OPT_SEPARATED_COMPLEX_VECTORS are
//      set, otherwise it disappears without a trace.
//
void
spMatrixFrame::ComplexTransposedMatrixMultiply(spREAL *rhs, spREAL *solution
    IMAG_VECTORS_P)
{
    // Correct array pointers for SP_OPT_ARRAY_OFFSET.
#if NOT SP_OPT_ARRAY_OFFSET
#if SP_OPT_SEPARATED_COMPLEX_VECTORS
    --rhs;              --irhs;
    --solution;         --isolution;
#else
    rhs -= 2;           solution -= 2;
#endif
#endif

    // Initialize Intermediate vector with reordered solution vector.
    spCOMPLEX *vector = (spCOMPLEX *)Intermediate;
    int *pExtOrder = &IntToExtRowMap[Size];

#if SP_OPT_SEPARATED_COMPLEX_VECTORS
    for (int i = Size; i > 0; i--) {
        vector[i].Real = solution[*pExtOrder];
        vector[i].Imag = isolution[*(pExtOrder--)];
    }
#else
    for (I = Matrix->Size; I > 0; I--)
        vector[I] = ((spCOMPLEX *)Solution)[*(pExtOrder--)];
#endif

    pExtOrder = &IntToExtColMap[Size];
    for (int i = Size; i > 0; i--) {
        spMatrixElement *pElement = FirstInCol[i];
        spCOMPLEX sum;
        sum.Real = sum.Imag = 0.0;

        while (pElement != 0) {
            // Cmplx expression : Sum += Element * vector[Row]
            CMPLX_MULT_ADD_ASSIGN(sum, *pElement, vector[pElement->Row]);
            pElement = pElement->NextInCol;
        }

#if SP_OPT_SEPARATED_COMPLEX_VECTORS
        rhs[*pExtOrder] = sum.Real;
        irhs[*pExtOrder--] = sum.Imag;
#else
        ((spCOMPLEX *)rhs)[*pExtOrder--] = sum;
#endif
    }
}

#endif // SP_OPT_COMPLEX AND SP_OPT_MULTIPLICATION AND SP_OPT_TRANSPOSE


#if SP_OPT_DETERMINANT

namespace {
    inline spREAL NORM(spCOMPLEX a)
    {
        spREAL nr = ABS((a).Real);
        spREAL ni = ABS((a).Imag);
        return SPMAX(nr, ni);
    }
}

//  CALCULATE DETERMINANT
//
// This function in capable of calculating the determinant of the
// matrix once the LU factorization has been performed.  Hence, only
// use this function after spFactor() and before spClear().  The
// determinant equals the product of all the diagonal elements of the
// lower triangular matrix L, except that this product may need
// negating.  Whether the product or the negative product equals the
// determinant is determined by the number of row and column
// interchanges performed.  Note that the determinants of matrices can
// be very large or very small.  On large matrices, the determinant
// can be far larger or smaller than can be represented by a floating
// point number.  For this reason the determinant is scaled to a
// reasonable value and the logarithm of the scale factor is returned.
//
//  >>> Arguments:
//
//  pExponent  <output>  (int *)
//      The logarithm base 10 of the scale factor for the determinant.  To
//      find the actual determinant, Exponent should be added to the
//      exponent of Determinant.
//
//  pDeterminant  <output>  (spREAL *)
//      The real portion of the determinant.   This number is scaled to be
//      greater than or equal to 1.0 and less than 10.0.
//
//  piDeterminant  <output>  (spREAL *)
//      The imaginary portion of the determinant.  When the matrix is real
//      this pointer need not be supplied, nothing will be returned.   This
//      number is scaled to be greater than or equal to 1.0 and less than 10.0.
//
//  >>> Local variables:
//
//  norm  (spREAL)
//      L-infinity norm of a complex number.
//
//  size  (int)
//      Local storage for Matrix->Size.  Placed in a register for speed.
//
//  temp  (spREAL)
//      Temporary storage for real portion of determinant.
//
#if SP_OPT_COMPLEX
void
spMatrixFrame::spDeterminant(int *pExponent, spREAL *pDeterminant,
    spREAL *piDeterminant)
#else
void
spMatrixFrame::spDeterminant(int *pExponent, spREAL *pDeterminant)
#endif
{
    ASSERT(IS_FACTORED());
    *pExponent = 0;

    if (Error == spSINGULAR) {
        *pDeterminant = 0.0;
#if SP_OPT_COMPLEX
        if (Complex)
            *piDeterminant = 0.0;
#endif
        return;
    }

    int i = 0;

#if SP_OPT_COMPLEX
    if (Complex) {
        spCOMPLEX cDeterminant;
        cDeterminant.Real = 1.0;
        cDeterminant.Imag = 0.0;

        while (++i <= Size) {
            spCOMPLEX pivot;
            CMPLX_RECIPROCAL(pivot, *Diag[i]);
            CMPLX_MULT_ASSIGN(cDeterminant, pivot);

            // Scale Determinant.
            spREAL norm = NORM(cDeterminant);
            if (norm != 0.0) {
                while (norm >= 1.0e12) {
                    cDeterminant.Real *= 1.0e-12;
                    cDeterminant.Imag *= 1.0e-12;
                    *pExponent += 12;
                    norm = NORM(cDeterminant);
                }
                while (norm < 1.0e-12) {
                    cDeterminant.Real *= 1.0e12;
                    cDeterminant.Imag *= 1.0e12;
                    *pExponent -= 12;
                    norm = NORM(cDeterminant);
                }
            }
        }

        // Scale Determinant again, this time to be between 1.0 <= x < 10.0
        spREAL norm = NORM(cDeterminant);
        if (norm != 0.0) {
            while (norm >= 10.0) {
                cDeterminant.Real *= 0.1;
                cDeterminant.Imag *= 0.1;
                (*pExponent)++;
                norm = NORM(cDeterminant);
            }
            while (norm < 1.0) {
                cDeterminant.Real *= 10.0;
                cDeterminant.Imag *= 10.0;
                (*pExponent)--;
                norm = NORM(cDeterminant);
            }
        }
        if (NumberOfInterchangesIsOdd)
            CMPLX_NEGATE(cDeterminant);

        *pDeterminant = cDeterminant.Real;
        *piDeterminant = cDeterminant.Imag;
    }
#endif // SP_OPT_COMPLEX
#if SP_OPT_REAL AND SP_OPT_COMPLEX
    else
#endif
#if SP_OPT_REAL
    {
#if SP_OPT_LONG_DBL_SOLVE
        // Real Case

        if (LongDoubles) {
            long double pd = 1.0;
            while (++i <= Size) {
                pd /= LDBL(Diag[i]);

                // Scale Determinant.
                if (pd != 0.0) {
                    while (LABS(pd) >= 1.0e12) {
                        pd *= 1.0e-12;
                        *pExponent += 12;
                    }
                    while (LABS(pd) < 1.0e-12) {
                        pd *= 1.0e12;
                        *pExponent -= 12;
                    }
                }
            }

            // Scale Determinant again, this time to be between 1.0 <= x < 10.0.
            if (pd != 0.0) {
                while (LABS(pd) >= 10.0) {
                    pd *= 0.1;
                    (*pExponent)++;
                }
                while (LABS(pd) < 1.0) {
                    pd *= 10.0;
                    (*pExponent)--;
                }
            }
            if (NumberOfInterchangesIsOdd)
                pd = -pd;
            *pDeterminant = pd;

            return;
        }
#endif

        spREAL pd = 1.0;
        while (++i <= Size) {
            pd /= Diag[i]->Real;

            // Scale Determinant.
            if (pd != 0.0) {
                while (ABS(pd) >= 1.0e12) {
                    pd *= 1.0e-12;
                    *pExponent += 12;
                }
                while (ABS(pd) < 1.0e-12) {
                    pd *= 1.0e12;
                    *pExponent -= 12;
                }
            }
        }

        // Scale Determinant again, this time to be between 1.0 <= x < 10.0.
        if (pd != 0.0) {
            while (ABS(pd) >= 10.0) {
                pd *= 0.1;
                (*pExponent)++;
            }
            while (ABS(pd) < 1.0) {
                pd *= 10.0;
                (*pExponent)--;
            }
        }
        if (NumberOfInterchangesIsOdd)
            pd = -pd;
        *pDeterminant = pd;
    }
#endif // SP_OPT_REAL
}

#endif // SP_OPT_DETERMINANT


#if SP_OPT_STRIP

//  STRIP FILL-INS FROM MATRIX
//
// Strips the matrix of all fill-ins.
//
//  >>> Local variables:
//
//  pElement  (spMatrixElement*)
//      Pointer that is used to step through the matrix.
//
//  ppElement  (spMatrixElement**)
//      Pointer to the location of a spMatrixElement*.  This location will be
//      updated if a fill-in is stripped from the matrix.
//
//  pFillin  (spMatrixElement*)
//      Pointer used to step through the lists of fill-ins while marking them.
//
//  pLastFillin  (spMatrixElement*)
//      A pointer to the last fill-in in the list.  Used to terminate a loop.
//
//  pListNode  (struct  FillinListNodeStruct *)
//      A pointer to a node in the FillinList linked-list.
//
void
spMatrixFrame::spStripFills()
{

    if (Fillins == 0)
        return;
    NeedsOrdering = YES;
    Elements -= Fillins;
    Fillins = 0;

    // Set the Row field of all fillin spMatrixElement structs to zero.
    FillinAllocator.MarkFillins();

    // Unlink fill-ins by searching for elements marked with Row = 0.
    {
        // Unlink fill-ins in all columns.
        for (int i = 1; i <= Size; i++) {
            spMatrixElement **ppElement = &(FirstInCol[i]);
            spMatrixElement *pElement;
            while ((pElement = *ppElement) != 0) {
                if (pElement->Row == 0) {
                    *ppElement = pElement->NextInCol;  // Unlink fill-in
                    if (Diag[pElement->Col] == pElement)
                        Diag[pElement->Col] = 0;
                }
                else
                    ppElement = &pElement->NextInCol;  // Skip element
            }
        }

        // Unlink fill-ins in all rows
        for (int i = 1; i <= Size; i++) {
            spMatrixElement **ppElement = &(FirstInRow[i]);
            spMatrixElement *pElement;
            while ((pElement = *ppElement) != 0) {
                if (pElement->Row == 0)
                    *ppElement = pElement->NextInRow;  // Unlink fill-in
                else
                    ppElement = &pElement->NextInRow;  // Skip element
            }
        }
    }

    // Flush 'em.
    FillinAllocator.Clear();
}

#endif


#if SP_OPT_TRANSLATE AND SP_OPT_DELETE

//  DELETE A ROW AND COLUMN FROM THE MATRIX
//
// Deletes a row and a column from a matrix.
//
// Sparse will abort if an attempt is made to delete a row or column that
// doesn't exist.
//
//  >>> Arguments:
//
//  row  <input>  (int)
//      Row to be deleted.
//
//  col  <input>  (int)
//      Column to be deleted.
//
//  >>> Local variables:
//
//  extCol  (int)
//      The external column that is being deleted.
//
//  extRow  (int)
//      The external row that is being deleted.
//
//  pElement  (spMatrixElement*)
//      Pointer to an element in the matrix.  Used when scanning rows and
//      columns in order to eliminate elements from the last row or column.
//
//  ppElement  (spMatrixElement**)
//      Pointer to the location of a spMatrixElement*.  This location will be
//      filled with a 0 pointer if it is the new last element in its row
//      or column.
//
//  pElement  (spMatrixElement*)
//      Pointer to an element in the last row or column of the matrix.
//
void
spMatrixFrame::spDeleteRowAndCol(int row, int col)
{
    ASSERT(row > 0 AND col > 0);
    ASSERT(row <= ExtSize AND col <= ExtSize);

    int extRow = row;
    int extCol = col;
    if (NOT RowsLinked)
        LinkRows();

    row = ExtToIntRowMap[row];
    col = ExtToIntColMap[col];
    ASSERT(row > 0 AND col > 0);

    // Move row so that it is the last row in the matrix.
    if (row != Size)
        RowExchange(row, Size);

    // Move col so that it is the last column in the matrix.
    if (col != Size)
        ColExchange(col, Size);

    // Correct Diag pointers.
    if (row == col)
        SWAP(spMatrixElement*, Diag[row], Diag[Size])
    else {
        Diag[row] = FindElementInCol(FirstInCol + row, row, row, NO);
        Diag[col] = FindElementInCol(FirstInCol + col, col, col, NO);
    }

    // Delete last row and column of the matrix.

    // Break the column links to every element in the last row.
    spMatrixElement *pLastElement = FirstInRow[Size];
    while (pLastElement != 0) {
        spMatrixElement **ppElement = &(FirstInCol[pLastElement->Col]);
        spMatrixElement *pElement;
        while ((pElement = *ppElement) != 0) {
            if (pElement == pLastElement)
                *ppElement = 0;  // Unlink last element in column.
            else
                ppElement = &pElement->NextInCol;  // Skip element.
        }
        pLastElement = pLastElement->NextInRow;
    }

    // Break the row links to every element in the last column.
    pLastElement = FirstInCol[Size];
    while (pLastElement != 0) {
        spMatrixElement **ppElement = &(FirstInRow[pLastElement->Row]);
        spMatrixElement *pElement;
        while ((pElement = *ppElement) != 0) {
            if (pElement == pLastElement)
                *ppElement = 0;  // Unlink last element in row
            else
                ppElement = &pElement->NextInRow;  // Skip element
        }
        pLastElement = pLastElement->NextInCol;
    }

    // Clean up some details.
    Size = Size - 1;
    Diag[Size] = 0;
    FirstInRow[Size] = 0;
    FirstInCol[Size] = 0;
    CurrentSize--;
    ExtToIntRowMap[extRow] = -1;
    ExtToIntColMap[extCol] = -1;
    NeedsOrdering = YES;
}

#endif


#if SP_OPT_PSEUDOCONDITION

//  CALCULATE PSEUDOCONDITION
//
// Computes the magnitude of the ratio of the largest to the smallest
// pivots.  This quantity is an indicator of ill-conditioning in the
// matrix.  If this ratio is large, and if the matrix is scaled such
// that uncertainties in the RHS and the matrix entries are
// equilibrated, then the matrix is ill-conditioned.  However, a small
// ratio does not necessarily imply that the matrix is
// well-conditioned.  This function must only be used after a matrix
// has been factored by spOrderAndFactor() or spFactor() and before it
// is cleared by spClear() or spInitialize().  The pseudocondition is
// faster to compute than the condition number calculated by
// spCondition(), but is not as informative.
//
//  >>> Returns:
//
// The magnitude of the ratio of the largest to smallest pivot used
// during previous factorization.  If the matrix was singular, zero is
// returned.
//
spREAL
spMatrixFrame::spPseudoCondition()
{
    ASSERT(IS_FACTORED());
    if (Error == spSINGULAR OR Error == spZERO_DIAG)
        return (0.0);

    spMatrixElement **diag = Diag;
    spREAL minPivot = E_MAG(diag[1]);
    spREAL maxPivot = minPivot;
    for (int i = 2; i <= Size; i++) {
        spREAL mag = E_MAG(diag[i]);
        if (mag > maxPivot)
            maxPivot = mag;
        else if (mag < minPivot)
            minPivot = mag;
    }
    ASSERT(maxPivot > 0.0);
    return (maxPivot / minPivot);
}

#endif


#if SP_OPT_CONDITION

//  ESTIMATE CONDITION NUMBER
//
// Computes an estimate of the condition number using a variation on
// the LINPACK condition number estimation algorithm.  This quantity
// is an indicator of ill-conditioning in the matrix.  To avoid
// problems with overflow, the reciprocal of the condition number is
// returned.  If this number is small, and if the matrix is scaled
// such that uncertainties in the RHS and the matrix entries are
// equilibrated, then the matrix is ill-conditioned.  If the this
// number is near one, the matrix is well conditioned.  This function
// must only be used after a matrix has been factored by
// spOrderAndFactor() or spFactor() and before it is cleared by
// spClear() or spInitialize().
//
// Unlike the LINPACK condition number estimator, this function
// returns the L infinity condition number.  This is an artifact of
// Sparse placing ones on the diagonal of the upper triangular matrix
// rather than the lower.  This difference should be of no importance.
//
// References:  A.K.  Cline, C.B.  Moler, G.W.  Stewart, J.H. 
// Wilkinson.  An estimate for the condition number of a matrix.  SIAM
// Journal on Numerical Analysis.  Vol.  16, No.  2, pages 368-375,
// April 1979.
//
// J.J.  Dongarra, C.B.  Moler, J.R.  Bunch, G.W.  Stewart.  LINPACK
// User's Guide.  SIAM, 1979.
//
// Roger G.  Grimes, John G.  Lewis.  Condition number estimation for
// sparse matrices.  SIAM Journal on Scientific and Statistical
// Computing.  Vol.  2, No.  4, pages 384-388, December 1981.
//
// Dianne Prost O'Leary.  Estimating matrix condition numbers.  SIAM
// Journal on Scientific and Statistical Computing.  Vol.  1, No.  2,
// pages 205-209, June 1980.
//
//  >>> Returns:
//
// The reciprocal of the condition number.  If the matrix was
// singular, zero is returned.
//
//  >>> Arguments:
//
//  normOfMatrix  <input>  (spREAL)
//      The L-infinity norm of the unfactored matrix as computed by
//      spNorm().
//
//  pError  <output>  (int *)
//      Used to return error code.
//
//  >>> Possible errors:
//  spSINGULAR
//
spREAL
spMatrixFrame::spCondition(spREAL normOfMatrix, int *pError)
{
#define SLACK   1e4

    ASSERT(IS_FACTORED());
    *pError = Error;
    if (Error >= spFATAL)
        return (0.0);
    if (normOfMatrix == 0.0) {
        *pError = spSINGULAR;
        return (0.0);
    }

#if SP_OPT_COMPLEX
    if (Complex)
        return (ComplexCondition(normOfMatrix));
#endif

#if SP_OPT_REAL
#if SP_OPT_LONG_DBL_SOLVE
    if (LongDoubles)
        return (LongDoubleCondition(normOfMatrix));
#endif
    spREAL *T = Intermediate;
#if SP_OPT_COMPLEX
    spREAL *Tm = Intermediate + Size;
#else
    spREAL *Tm = new spREAL[Size+1];
#endif
    for (int i = Size; i > 0; i--)
        T[i] = 0.0;

    // Part 1.  Ay = e.
    // Solve Ay = LUy = e where e consists of +1 and -1 terms with the sign
    // chosen to maximize the size of w in Lw = e.  Since the terms in w can
    // get very large, scaling is used to avoid overflow.

    // Forward elimination. Solves Lw = e while choosing e
    spREAL E = 1.0;
    for (int i = 1; i <= Size; i++) {
        spMatrixElement *pPivot = Diag[i];
        spREAL Em;
        if (T[i] < 0.0)
            Em = -E;
        else
            Em = E;
        spREAL Wm = (Em + T[i]) * pPivot->Real;
        if (ABS(Wm) > SLACK) {
            spREAL scaleFactor = 1.0 / SPMAX(SQR(SLACK), ABS(Wm));
            for (int k = Size; k > 0; k--)
                T[k] *= scaleFactor;
            E *= scaleFactor;
            Em *= scaleFactor;
            Wm = (Em + T[i]) * pPivot->Real;
        }
        spREAL Wp = (T[i] - Em) * pPivot->Real;
        spREAL ASp = ABS(T[i] - Em);
        spREAL ASm = ABS(Em + T[i]);

        // Update T for both values of W, minus value is placed in Tm.
        spMatrixElement *pElement = pPivot->NextInCol;
        while (pElement != 0) {
            int row = pElement->Row;
            Tm[row] = T[row] - (Wm * pElement->Real);
            T[row] -= (Wp * pElement->Real);
            ASp += ABS(T[row]);
            ASm += ABS(Tm[row]);
            pElement = pElement->NextInCol;
        }

        // If minus value causes more growth, overwrite T with its values.
        if (ASm > ASp) {
            T[i] = Wm;
            pElement = pPivot->NextInCol;
            while (pElement != 0) {
                T[pElement->Row] = Tm[pElement->Row];
                pElement = pElement->NextInCol;
            }
        }
        else T[i] = Wp;
    }

    // Compute 1-norm of T, which now contains w, and scale ||T|| to 1/SLACK.
    spREAL ASw = 0.0;
    for (int i = Size; i > 0; i--)
        ASw += ABS(T[i]);
    spREAL scaleFactor = 1.0 / (SLACK * ASw);
    if (scaleFactor < 0.5) {
        for (int i = Size; i > 0; i--)
            T[i] *= scaleFactor;
        E *= scaleFactor;
    }

    // Backward Substitution. Solves Uy = w.
    for (int i = Size; i >= 1; i--) {
        spMatrixElement *pElement = Diag[i]->NextInRow;
        while (pElement != 0) {
            T[i] -= pElement->Real * T[pElement->Col];
            pElement = pElement->NextInRow;
        }
        if (ABS(T[i]) > SLACK) {
            scaleFactor = 1.0 / SPMAX(SQR(SLACK), ABS(T[i]));
            for (int k = Size; k > 0; k--)
                T[k] *= scaleFactor;
            E *= scaleFactor;
        }
    }

    // Compute 1-norm of T, which now contains y, and scale ||T|| to 1/SLACK.
    spREAL ASy = 0.0;
    for (int i = Size; i > 0; i--)
        ASy += ABS(T[i]);
    scaleFactor = 1.0 / (SLACK * ASy);
    if (scaleFactor < 0.5) {
        for (int i = Size; i > 0; i--)
            T[i] *= scaleFactor;
        ASy = 1.0 / SLACK;
        E *= scaleFactor;
    }

    // Compute infinity-norm of T for O'Leary's estimate.
    spREAL maxY = 0.0;
    for (int i = Size; i > 0; i--) {
        if (maxY < ABS(T[i]))
            maxY = ABS(T[i]);
    }

    // Part 2.  A* z = y where the * represents the transpose.
    // Recall that A = LU implies A* = U* L*.

    // Forward elimination, U* v = y
    for (int i = 1; i <= Size; i++) {
        spMatrixElement *pElement = Diag[i]->NextInRow;
        while (pElement != 0) {
            T[pElement->Col] -= T[i] * pElement->Real;
            pElement = pElement->NextInRow;
        }
        if (ABS(T[i]) > SLACK) {
            scaleFactor = 1.0 / SPMAX(SQR(SLACK), ABS(T[i]));
            for (int k = Size; k > 0; k--)
                T[k] *= scaleFactor;
            ASy *= scaleFactor;
        }
    }

    // Compute 1-norm of T, which now contains v, and scale ||T|| to 1/SLACK.
    spREAL ASv = 0.0;
    for (int i = Size; i > 0; i--)
        ASv += ABS(T[i]);
    scaleFactor = 1.0 / (SLACK * ASv);
    if (scaleFactor < 0.5) {
        for (int i = Size; i > 0; i--)
            T[i] *= scaleFactor;
        ASy *= scaleFactor;
    }

    // Backward Substitution, L* z = v.
    for (int i = Size; i >= 1; i--) {
        spMatrixElement *pPivot = Diag[i];
        spMatrixElement *pElement = pPivot->NextInCol;
        while (pElement != 0) {
            T[i] -= pElement->Real * T[pElement->Row];
            pElement = pElement->NextInCol;
        }
        T[i] *= pPivot->Real;
        if (ABS(T[i]) > SLACK) {
            scaleFactor = 1.0 / SPMAX(SQR(SLACK), ABS(T[i]));
            for (int k = Size; k > 0; k--)
                T[k] *= scaleFactor;
            ASy *= scaleFactor;
        }
    }

    // Compute 1-norm of T, which now contains z.
    spREAL ASz = 0.0;
    for (int i = Size; i > 0; i--)
        ASz += ABS(T[i]);

#if NOT SP_OPT_COMPLEX
    delete [] Tm;
#endif

    spREAL linpack = ASy / ASz;
    spREAL oLeary = E / maxY;
    spREAL invNormOfInverse = SPMIN(linpack, oLeary);
    return (invNormOfInverse / normOfMatrix);
#endif // SP_OPT_REAL
}


#if SP_OPT_COMPLEX

//  ESTIMATE CONDITION NUMBER
//
//  Complex version of spCondition().
//
//  >>> Returns:
//  The reciprocal of the condition number.
//
//  >>> Arguments:
//
//  normOfMatrix  <input>  (spREAL)
//      The L-infinity norm of the unfactored matrix as computed by
//      spNorm().
//
//  >>> Possible errors:
//  spNO_MEMORY
//
spREAL
spMatrixFrame::ComplexCondition(spREAL normOfMatrix)
{
    spCOMPLEX *T = (spCOMPLEX *)Intermediate;
    spCOMPLEX *Tm = new spCOMPLEX[Size+1];
    for (int i = Size; i > 0; i--)
        T[i].Real = T[i].Imag = 0.0;

    // Part 1.  Ay = e.
    // Solve Ay = LUy = e where e consists of +1 and -1 terms with the sign
    // chosen to maximize the size of w in Lw = e.  Since the terms in w can
    // get very large, scaling is used to avoid overflow.

    // Forward elimination. Solves Lw = e while choosing e.
    spREAL E = 1.0;
    for (int i = 1; i <= Size; i++) {
        spMatrixElement *pPivot = Diag[i];
        spREAL Em;
        if (T[i].Real < 0.0)
            Em = -E;
        else
            Em = E;
        spCOMPLEX Wm = T[i];
        Wm.Real += Em;
        spREAL ASm = CMPLX_1_NORM(Wm);
        CMPLX_MULT_ASSIGN(Wm, *pPivot);
        if (CMPLX_1_NORM(Wm) > SLACK) {
            spREAL scaleFactor = 1.0 / SPMAX(SQR(SLACK), CMPLX_1_NORM(Wm));
            for (int k = Size; k > 0; k--)
                SCLR_MULT_ASSIGN(T[k], scaleFactor);
            E *= scaleFactor;
            Em *= scaleFactor;
            ASm *= scaleFactor;
            SCLR_MULT_ASSIGN(Wm, scaleFactor);
        }
        spCOMPLEX Wp = T[i];
        Wp.Real -= Em;
        spREAL ASp = CMPLX_1_NORM(Wp);
        CMPLX_MULT_ASSIGN(Wp, *pPivot);

        // Update T for both values of W, minus value is placed in Tm.
        spMatrixElement *pElement = pPivot->NextInCol;
        while (pElement != 0) {
            int row = pElement->Row;
            // Cmplx expr: Tm[Row] = T[Row] - (Wp * *pElement)
            CMPLX_MULT_SUBT(Tm[row], Wm, *pElement, T[row]);
            // Cmplx expr: T[Row] -= Wp * *pElement
            CMPLX_MULT_SUBT_ASSIGN(T[row], Wm, *pElement);
            ASp += CMPLX_1_NORM(T[row]);
            ASm += CMPLX_1_NORM(Tm[row]);
            pElement = pElement->NextInCol;
        }

        // If minus value causes more growth, overwrite T with its values.
        if (ASm > ASp) {
            T[i] = Wm;
            pElement = pPivot->NextInCol;
            while (pElement != 0) {
                T[pElement->Row] = Tm[pElement->Row];
                pElement = pElement->NextInCol;
            }
        }
        else T[i] = Wp;
    }

    // Compute 1-norm of T, which now contains w, and scale ||T|| to 1/SLACK.
    spREAL ASw = 0.0;
    for (int i = Size; i > 0; i--)
        ASw += CMPLX_1_NORM(T[i]);
    spREAL scaleFactor = 1.0 / (SLACK * ASw);
    if (scaleFactor < 0.5) {
        for (int i = Size; i > 0; i--)
            SCLR_MULT_ASSIGN(T[i], scaleFactor);
        E *= scaleFactor;
    }

    // Backward Substitution. Solves Uy = w.
    for (int i = Size; i >= 1; i--) {
        spMatrixElement *pElement = Diag[i]->NextInRow;
        while (pElement != 0) {
            // Cmplx expr: T[I] -= T[pElement->Col] * *pElement
            CMPLX_MULT_SUBT_ASSIGN(T[i], T[pElement->Col], *pElement);
            pElement = pElement->NextInRow;
        }
        if (CMPLX_1_NORM(T[i]) > SLACK) {
            scaleFactor = 1.0 / SPMAX(SQR(SLACK), CMPLX_1_NORM(T[i]));
            for (int k = Size; k > 0; k--)
                SCLR_MULT_ASSIGN(T[k], scaleFactor);
            E *= scaleFactor;
        }
    }

    // Compute 1-norm of T, which now contains y, and scale ||T|| to 1/SLACK.
    spREAL ASy = 0.0;
    for (int i = Size; i > 0; i--)
        ASy += CMPLX_1_NORM(T[i]);
    scaleFactor = 1.0 / (SLACK * ASy);
    if (scaleFactor < 0.5) {
        for (int i = Size; i > 0; i--)
            SCLR_MULT_ASSIGN(T[i], scaleFactor);
        ASy = 1.0 / SLACK;
        E *= scaleFactor;
    }

    // Compute infinity-norm of T for O'Leary's estimate.
    spREAL MaxY = 0.0;
    for (int i = Size; i > 0; i--) {
        if (MaxY < CMPLX_1_NORM(T[i]))
            MaxY = CMPLX_1_NORM(T[i]);
    }

    // Part 2.  A* z = y where the * represents the transpose.
    // Recall that A = LU implies A* = U* L*.

    // Forward elimination, U* v = y.
    for (int i = 1; i <= Size; i++) {
        spMatrixElement *pElement = Diag[i]->NextInRow;
        while (pElement != 0) {
            // Cmplx expr: T[pElement->Col] -= T[I] * *pElement
            CMPLX_MULT_SUBT_ASSIGN(T[pElement->Col], T[i], *pElement);
            pElement = pElement->NextInRow;
        }
        if (CMPLX_1_NORM(T[i]) > SLACK) {
            scaleFactor = 1.0 / SPMAX(SQR(SLACK), CMPLX_1_NORM(T[i]));
            for (int k = Size; k > 0; k--)
                SCLR_MULT_ASSIGN(T[k], scaleFactor);
            ASy *= scaleFactor;
        }
    }

    // Compute 1-norm of T, which now contains v, and scale ||T|| to 1/SLACK.
    spREAL ASv = 0.0;
    for (int i = Size; i > 0; i--)
        ASv += CMPLX_1_NORM(T[i]);
    scaleFactor = 1.0 / (SLACK * ASv);
    if (scaleFactor < 0.5) {
        for (int i = Size; i > 0; i--)
            SCLR_MULT_ASSIGN(T[i], scaleFactor);
        ASy *= scaleFactor;
    }

    // Backward Substitution, L* z = v.
    for (int i = Size; i >= 1; i--) {
        spMatrixElement *pPivot = Diag[i];
        spMatrixElement *pElement = pPivot->NextInCol;
        while (pElement != 0) {
            // Cmplx expr: T[I] -= T[pElement->Row] * *pElement
            CMPLX_MULT_SUBT_ASSIGN(T[i], T[pElement->Row], *pElement);
            pElement = pElement->NextInCol;
        }
        CMPLX_MULT_ASSIGN(T[i], *pPivot);
        if (CMPLX_1_NORM(T[i]) > SLACK) {
            scaleFactor = 1.0 / SPMAX(SQR(SLACK), CMPLX_1_NORM(T[i]));
            for (int k = Size; k > 0; k--)
                SCLR_MULT_ASSIGN(T[k], scaleFactor);
            ASy *= scaleFactor;
        }
    }

    // Compute 1-norm of T, which now contains z.
    spREAL ASz = 0.0;
    for (int i = Size; i > 0; i--)
        ASz += CMPLX_1_NORM(T[i]);

    delete [] Tm;

    spREAL linpack = ASy / ASz;
    spREAL oLeary = E / MaxY;
    spREAL invNormOfInverse = SPMIN(linpack, oLeary);
    return (invNormOfInverse / normOfMatrix);
}

#endif // SP_OPT_COMPLEX

#if SP_OPT_LONG_DBL_SOLVE

//  ESTIMATE CONDITION NUMBER
//
//  Long double version of spCondition().
//
//  >>> Returns:
//  The reciprocal of the condition number.
//
//  >>> Arguments:
//
//  normOfMatrix  <input>  (spREAL)
//      The L-infinity norm of the unfactored matrix as computed by
//      spNorm().
//
//  >>> Possible errors:
//  spNO_MEMORY
//
spREAL
spMatrixFrame::LongDoubleCondition(spREAL normOfMatrix)
{
    long double *T = (long double*)Intermediate;
    long double *Tm = new long double[Size+1];
    for (int i = Size; i > 0; i--)
        T[i] = 0.0;

    // Part 1.  Ay = e.
    // Solve Ay = LUy = e where e consists of +1 and -1 terms with the sign
    // chosen to maximize the size of w in Lw = e.  Since the terms in w can
    // get very large, scaling is used to avoid overflow.

    // Forward elimination. Solves Lw = e while choosing e
    long double E = 1.0;
    for (int i = 1; i <= Size; i++) {
        spMatrixElement *pPivot = Diag[i];
        long double Em;
        if (T[i] < 0.0)
            Em = -E;
        else
            Em = E;
        long double Wm = (Em + T[i]) * LDBL(pPivot);
        if (fabsl(Wm) > SLACK) {
            long double scaleFactor = 1.0 / SPMAX(SQR(SLACK), fabsl(Wm));
            for (int k = Size; k > 0; k--)
                T[k] *= scaleFactor;
            E *= scaleFactor;
            Em *= scaleFactor;
            Wm = (Em + T[i]) * LDBL(pPivot);
        }
        long double Wp = (T[i] - Em) * LDBL(pPivot);
        long double ASp = fabsl(T[i] - Em);
        long double ASm = fabsl(Em + T[i]);

        // Update T for both values of W, minus value is placed in Tm.
        spMatrixElement *pElement = pPivot->NextInCol;
        while (pElement != 0) {
            int row = pElement->Row;
            Tm[row] = T[row] - (Wm * LDBL(pElement));
            T[row] -= (Wp * LDBL(pElement));
            ASp += fabsl(T[row]);
            ASm += fabsl(Tm[row]);
            pElement = pElement->NextInCol;
        }

        // If minus value causes more growth, overwrite T with its values.
        if (ASm > ASp) {
            T[i] = Wm;
            pElement = pPivot->NextInCol;
            while (pElement != 0) {
                T[pElement->Row] = Tm[pElement->Row];
                pElement = pElement->NextInCol;
            }
        }
        else T[i] = Wp;
    }

    // Compute 1-norm of T, which now contains w, and scale ||T|| to 1/SLACK.
    long double ASw = 0.0;
    for (int i = Size; i > 0; i--)
        ASw += fabsl(T[i]);
    long double scaleFactor = 1.0 / (SLACK * ASw);
    if (scaleFactor < 0.5) {
        for (int i = Size; i > 0; i--)
            T[i] *= scaleFactor;
        E *= scaleFactor;
    }

    // Backward Substitution. Solves Uy = w.
    for (int i = Size; i >= 1; i--) {
        spMatrixElement *pElement = Diag[i]->NextInRow;
        while (pElement != 0) {
            T[i] -= LDBL(pElement) * T[pElement->Col];
            pElement = pElement->NextInRow;
        }
        if (fabsl(T[i]) > SLACK) {
            scaleFactor = 1.0 / SPMAX(SQR(SLACK), fabsl(T[i]));
            for (int k = Size; k > 0; k--)
                T[k] *= scaleFactor;
            E *= scaleFactor;
        }
    }

    // Compute 1-norm of T, which now contains y, and scale ||T|| to 1/SLACK.
    long double ASy = 0.0;
    for (int i = Size; i > 0; i--)
        ASy += fabsl(T[i]);
    scaleFactor = 1.0 / (SLACK * ASy);
    if (scaleFactor < 0.5) {
        for (int i = Size; i > 0; i--)
            T[i] *= scaleFactor;
        ASy = 1.0 / SLACK;
        E *= scaleFactor;
    }

    // Compute infinity-norm of T for O'Leary's estimate.
    long double maxY = 0.0;
    for (int i = Size; i > 0; i--) {
        if (maxY < fabsl(T[i]))
            maxY = fabsl(T[i]);
    }

    // Part 2.  A* z = y where the * represents the transpose.
    // Recall that A = LU implies A* = U* L*.

    // Forward elimination, U* v = y
    for (int i = 1; i <= Size; i++) {
        spMatrixElement *pElement = Diag[i]->NextInRow;
        while (pElement != 0) {
            T[pElement->Col] -= T[i] * LDBL(pElement);
            pElement = pElement->NextInRow;
        }
        if (fabsl(T[i]) > SLACK) {
            scaleFactor = 1.0 / SPMAX(SQR(SLACK), fabsl(T[i]));
            for (int k = Size; k > 0; k--)
                T[k] *= scaleFactor;
            ASy *= scaleFactor;
        }
    }

    // Compute 1-norm of T, which now contains v, and scale ||T|| to 1/SLACK.
    long double ASv = 0.0;
    for (int i = Size; i > 0; i--)
        ASv += fabsl(T[i]);
    scaleFactor = 1.0 / (SLACK * ASv);
    if (scaleFactor < 0.5) {
        for (int i = Size; i > 0; i--)
            T[i] *= scaleFactor;
        ASy *= scaleFactor;
    }

    // Backward Substitution, L* z = v.
    for (int i = Size; i >= 1; i--) {
        spMatrixElement *pPivot = Diag[i];
        spMatrixElement *pElement = pPivot->NextInCol;
        while (pElement != 0) {
            T[i] -= LDBL(pElement) * T[pElement->Row];
            pElement = pElement->NextInCol;
        }
        T[i] *= LDBL(pPivot);
        if (fabsl(T[i]) > SLACK) {
            scaleFactor = 1.0 / SPMAX(SQR(SLACK), fabsl(T[i]));
            for (int k = Size; k > 0; k--)
                T[k] *= scaleFactor;
            ASy *= scaleFactor;
        }
    }

    // Compute 1-norm of T, which now contains z.
    spREAL ASz = 0.0;
    for (int i = Size; i > 0; i--)
        ASz += fabsl(T[i]);

    delete [] Tm;

    spREAL linpack = ASy / ASz;
    spREAL oLeary = E / maxY;
    spREAL invNormOfInverse = SPMIN(linpack, oLeary);
    return (invNormOfInverse / normOfMatrix);
}

#endif


//  L-INFINITY MATRIX NORM
//
// Computes the L-infinity norm of an unfactored matrix.  It is a
// fatal error to pass this function a factored matrix.
//
// One difficulty is that the rows may not be linked.
//
//  >>> Returns:
//
// The largest absolute row sum of matrix.
//
spREAL
spMatrixFrame::spNorm()
{
    ASSERT(NOT IS_FACTORED());
    if (NOT RowsLinked)
        LinkRows();

    // Compute row sums.
    spREAL max = 0.0;
#if SP_OPT_COMPLEX
    if (Complex) {
        for (int i = Size; i > 0; i--) {
            spMatrixElement *pElement = FirstInRow[i];
            spREAL absRowSum = 0.0;
            while (pElement != 0) {
                absRowSum += CMPLX_1_NORM(*pElement);
                pElement = pElement->NextInRow;
            }
            if (max < absRowSum)
                max = absRowSum;
        }
        return (max);
    }
#endif
#if SP_OPT_REAL
#if SP_OPT_LONG_DBL_SOLVE
    if (LongDoubles) {
        for (int i = Size; i > 0; i--) {
            spMatrixElement *pElement = FirstInRow[i];
            spREAL absRowSum = 0.0;
            while (pElement != 0) {
                absRowSum += ABS(LDBL(pElement));
                pElement = pElement->NextInRow;
            }
            if (max < absRowSum)
                max = absRowSum;
        }
        return (max);
    }
#endif

    for (int i = Size; i > 0; i--) {
        spMatrixElement *pElement = FirstInRow[i];
        spREAL absRowSum = 0.0;
        while (pElement != 0) {
            absRowSum += ABS(pElement->Real);
            pElement = pElement->NextInRow;
        }
        if (max < absRowSum)
            max = absRowSum;
    }
#endif
    return (max);
}

#endif // SP_OPT_CONDITION


#if SP_OPT_STABILITY

//  STABILITY OF FACTORIZATION
//
// The following functions are used to gauge the stability of a
// factorization.  If the factorization is determined to be too
// unstable, then the matrix should be reordered.  The functions
// compute quantities that are needed in the computation of a bound on
// the error attributed to any one element in the matrix during the
// factorization.  In other words, there is a matrix E = [e_ij] of
// error terms such that A+E = LU.  This function finds a bound on
// |e_ij|.  Erisman & Reid [1] showed that |e_ij| < 3.01 u rho m_ij,
// where u is the machine rounding unit, rho = max a_ij where the max
// is taken over every row i, column j, and step k, and m_ij is the
// number of multiplications required in the computation of l_ij if i
// > j or u_ij otherwise.  Barlow [2] showed that rho < max_i || l_i
// ||_p max_j || u_j ||_q where 1/p + 1/q = 1.
//
// The first function finds the magnitude on the largest element in
// the matrix.  If the matrix has not yet been factored, the largest
// element is found by direct search.  If the matrix is factored, a
// bound on the largest element in any of the reduced submatrices is
// computed using Barlow with p = oo and q = 1.  The ratio of these
// two numbers is the growth, which can be used to determine if the
// pivoting order is adequate.  A large growth implies that
// considerable error has been made in the factorization and that it
// is probably a good idea to reorder the matrix.  If a large growth
// in encountered after using spFactor(), reconstruct the matrix and
// refactor using spOrderAndFactor().  If a large growth is
// encountered after using spOrderAndFactor(), refactor using
// spOrderAndFactor() with the pivot threshold increased, say to 0.1.
//
// Using only the size of the matrix as an upper bound on m_ij and
// Barlow's bound, the user can estimate the size of the matrix error
// terms e_ij using the bound of Erisman and Reid.  The second
// function computes a tighter bound (with more work) based on work by
// Gear [3], |e_ij| < 1.01 u rho (t c^3 + (1 + t)c^2) where t is the
// threshold and c is the maximum number of off-diagonal elements in
// any row of L.  The expensive part of computing this bound is
// determining the maximum number of off-diagonals in L, which changes
// only when the order of the matrix changes.  This number is computed
// and saved, and only recomputed if the matrix is reordered.
//
//  [1] A. M. Erisman, J. K. Reid.  Monitoring the stability of the
//      triangular factorization of a sparse matrix.  Numerische
//      Mathematik.  Vol. 22, No. 3, 1974, pp 183-186.
//
//  [2] J. L. Barlow.  A note on monitoring the stability of triangular
//      decomposition of sparse matrices.  "SIAM Journal of Scientific
//      and Statistical Computing."  Vol. 7, No. 1, January 1986, pp 166-168.
//
//  [3] I. S. Duff, A. M. Erisman, J. K. Reid.  "Direct Methods for Sparse
//      Matrices."  Oxford 1986. pp 99.


//  LARGEST ELEMENT IN MATRIX
//
//  >>> Returns:
//
// If matrix is not factored, returns the magnitude of the largest
// element in the matrix.  If the matrix is factored, a bound on the
// magnitude of the largest element in any of the reduced submatrices
// is returned.
//
spREAL
spMatrixFrame::spLargestElement()
{
    spREAL max = 0.0, maxRow = 0.0, maxCol = 0.0;
#if SP_OPT_COMPLEX
    if (Complex) {
        if (Factored) {
            if (Error == spSINGULAR)
                return (0.0);

            // Find the bound on the size of the largest element over all
            // factorization.
            for (int i = 1; i <= Size; i++) {
                spMatrixElement *pDiag = Diag[i];

                // Lower triangular matrix.
                spCOMPLEX cPivot;
                CMPLX_RECIPROCAL(cPivot, *pDiag);
                spREAL mag = CMPLX_1_NORM(cPivot);
                if (mag > maxRow)
                    maxRow = mag;
                spMatrixElement *pElement = FirstInRow[i];
                while (pElement != pDiag) {
                    mag = CMPLX_1_NORM(*pElement);
                    if (mag > maxRow)
                        maxRow = mag;
                    pElement = pElement->NextInRow;
                }

                // Upper triangular matrix.
                pElement = FirstInCol[i];
                spREAL absColSum = 1.0;  // Diagonal of U is unity.
                while (pElement != pDiag) {
                    absColSum += CMPLX_1_NORM(*pElement);
                    pElement = pElement->NextInCol;
                }
                if (absColSum > maxCol)
                    maxCol = absColSum;
            }
            max = (maxRow * maxCol);
        }
        else {
            for (int i = 1; i <= Size; i++) {
                spMatrixElement *pElement = FirstInCol[i];
                while (pElement != 0) {
                    spREAL mag = CMPLX_1_NORM(*pElement);
                    if (mag > max)
                        max = mag;
                    pElement = pElement->NextInCol;
                }
            }
        }
        return (max);
    }
#endif
#if SP_OPT_REAL
#if SP_OPT_LONG_DBL_SOLVE
    if (LongDoubles) {
        if (Factored) {
            if (Error == spSINGULAR)
                return (0.0);

            // Find the bound on the size of the largest element over all
            // factorization.
            for (int i = 1; i <= Size; i++) {
                spMatrixElement *pDiag = Diag[i];

                // Lower triangular matrix.
                spREAL pivot = 1.0 / LDBL(pDiag);
                spREAL mag = ABS(pivot);
                if (mag > maxRow)
                    maxRow = mag;
                spMatrixElement *pElement = FirstInRow[i];
                while (pElement != pDiag) {
                    mag = ABS(LDBL(pElement));
                    if (mag > maxRow)
                        maxRow = mag;
                    pElement = pElement->NextInRow;
                }

                // Upper triangular matrix
                pElement = FirstInCol[i];
                spREAL absColSum = 1.0;  // Diagonal of U is unity.
                while (pElement != pDiag) {
                    absColSum += ABS(LDBL(pElement));
                    pElement = pElement->NextInCol;
                }
                if (absColSum > maxCol) maxCol = absColSum;
            }
            max = (maxRow * maxCol);
        }
        else {
            for (int i = 1; i <= Size; i++) {
                spMatrixElement *pElement = FirstInCol[i];
                while (pElement != 0) {
                    spREAL mag = ABS(LDBL(pElement));
                    if (mag > max)
                        max = mag;
                    pElement = pElement->NextInCol;
                }
            }
        }
        return (max);
    }
#endif

    if (Factored) {
        if (Error == spSINGULAR)
            return (0.0);

        // Find the bound on the size of the largest element over all
        // factorization.
        for (int i = 1; i <= Size; i++) {
            spMatrixElement *pDiag = Diag[i];

            // Lower triangular matrix.
            spREAL pivot = 1.0 / pDiag->Real;
            spREAL mag = ABS(pivot);
            if (mag > maxRow)
                maxRow = mag;
            spMatrixElement *pElement = FirstInRow[i];
            while (pElement != pDiag) {
                mag = ABS(pElement->Real);
                if (mag > maxRow)
                    maxRow = mag;
                pElement = pElement->NextInRow;
            }

            // Upper triangular matrix
            pElement = FirstInCol[i];
            spREAL absColSum = 1.0;  // Diagonal of U is unity.
            while (pElement != pDiag) {
                absColSum += ABS(pElement->Real);
                pElement = pElement->NextInCol;
            }
            if (absColSum > maxCol) maxCol = absColSum;
        }
        max = (maxRow * maxCol);
    }
    else {
        for (int i = 1; i <= Size; i++) {
            spMatrixElement *pElement = FirstInCol[i];
            while (pElement != 0) {
                spREAL mag = ABS(pElement->Real);
                if (mag > max)
                    max = mag;
                pElement = pElement->NextInCol;
            }
        }
    }
#endif
    return (max);
}


//  MATRIX ROUNDOFF ERROR
//
//  >>> Returns:
//
// Returns a bound on the magnitude of the largest element in E = A - LU.
//
//  >>> Arguments:
//
//  rho  <input>  (spREAL)
//      The bound on the magnitude of the largest element in any of the
//      reduced submatrices.  This is the number computed by the function
//      spLargestElement() when given a factored matrix.  If this number is
//      negative, the bound will be computed automatically.
//
spREAL
spMatrixFrame::spRoundoff(spREAL rho)
{
    ASSERT(AND IS_FACTORED());

    int maxCount = 0;

    // Compute Barlow's bound if it is not given
    if (rho < 0.0)
        rho = spLargestElement();

    // Find the maximum number of off-diagonals in L if not previously
    // computed.
    if (MaxRowCountInLowerTri < 0) {
        for (int i = Size; i > 0; i--) {
            spMatrixElement *pElement = FirstInRow[i];
            int count = 0;
            while (pElement->Col < i) {
                count++;
                pElement = pElement->NextInRow;
            }
            if (count > maxCount)
                maxCount = count;
        }
        MaxRowCountInLowerTri = maxCount;
    }
    else
        maxCount = MaxRowCountInLowerTri;

    // Compute error bound.
    spREAL gear =
        1.01*((maxCount + 1) * RelThreshold + 1.0) * SQR(maxCount);
    spREAL reid = 3.01 * Size;

    if (gear < reid)
        return (DBL_EPSILON * rho * gear);
    else
        return (DBL_EPSILON * rho * reid);
}

#endif


#if SP_OPT_DOCUMENTATION

//  SPARSE ERROR MESSAGE
//
// This function prints a short message to a stream describing the
// error state of sparse.  No message is produced if there is no
// error.
//
//  >>> Arguments:
//
//  stream  <input>  (FILE *)
//      Stream to which the error message is to be printed.
//
//  originator  <input>  (char *)
//      Name of originator of error message.  If 0, `sparse' is used.
//      If zero-length string, no originator is printed.
//
void
spMatrixFrame::spErrorMessage(FILE *stream, const char *originator)
{
    int error;
    spMatrixFrame *mfpt = this;
    if (!mfpt)
        error = spNO_MEMORY;
    else {
        ASSERT(this->ID == SPARSE_ID);
        error = Error;
    }

    if (error == spOKAY)
        return;
    if (originator == 0)
        originator = "sparse";
    if (originator[0] != '\0')
        fprintf(stream, "%s: ", originator);
    if (error >= spFATAL)
        fprintf(stream, "fatal error, ");
    else
        fprintf(stream, "warning, ");

    // Print particular error message.
    // Do not use switch statement because error codes may not be unique.

    if (error == spPANIC)
        fprintf(stream, "Sparse called improperly.\n");
    else if (error == spNO_MEMORY)
        fprintf(stream, "insufficient memory available.\n");
    else if (error == spSINGULAR) {
        int row, col;
        spWhereSingular(&row, &col);
        fprintf(stream, "singular matrix detected at row %d and column %d.\n",
            row, col);
    }
    else if (Error == spZERO_DIAG) {
        int row, col;
        spWhereSingular(&row, &col);
        fprintf(stream, "zero diagonal detected at row %d and column %d.\n",
            row, col);
    }
    else if (error == spSMALL_PIVOT) {
        fprintf(stream,
            "unable to find a pivot that is larger than absolute threshold.\n");
    }
#if SP_OPT_DEBUG
    else ABORT();
#endif
}
#endif // SP_OPT_DOCUMENTATION


const char *
spMatrixFrame::spErrorMessage(int error)
{
    if (error == spOKAY)
        return (0);
    if (error == spSMALL_PIVOT)
        return ("sparse: pivot too small");
    if (error == spZERO_DIAG)
        return ("sparse: zero diagonal, bad matrix");
    if (error == spSINGULAR)
        return ("sparse: singular matrix");
    if (error == spNO_MEMORY)
        return ("sparse: memory allocation failure");
    if (error == spPANIC)
        return ("sparse: panic!");
    if (error == spFATAL)
        return ("sparse: fatal!");
    if (error == spABORTED)
        return ("sparse: interrupted!");
    return ("sparse: unknown error");
}


//  RETURN MATRIX ERROR STATUS
//
// This function is used to determine the error status of the given matrix.
//
//  >>> Returned:
//
// The error status of the given matrix.
//
int
spMatrixFrame::spError()
{
    spMatrixFrame *mfpt = this;
    if (mfpt) {
        ASSERT(this->ID == SPARSE_ID);
        return (Error);
    }
    return (spNO_MEMORY);  // This error may actually be spPANIC,
                           // no way to tell.
}


//  WHERE IS MATRIX SINGULAR
//
// This function returns the row and column number where the matrix was
// detected as singular or where a zero was detected on the diagonal.
//
//  >>> Arguments:
//
//  pRow  <output>  (int *)
//      The row number.
//
//  pCol  <output>  (int *)
//      The column number.
//
void
spMatrixFrame::spWhereSingular(int *pRow, int *pCol)
{
    if (Matrix) {
        int col;
        if (Matrix->where_singular(&col)) {
#if SP_OPT_TRANSLATE
            *pCol = IntToExtColMap[col+1];
            *pRow = IntToExtRowMap[col+1];
#else
            *pCol = col+1;
            *pRow = col+1;
#endif
        }
        else {
            *pCol = 0;
            *pRow = 0;
        }
        return;
    }

    if (Error == spSINGULAR OR Error == spZERO_DIAG) {
        *pRow = SingularRow;
        *pCol = SingularCol;
    }
    else
        *pRow = *pCol = 0;
}

