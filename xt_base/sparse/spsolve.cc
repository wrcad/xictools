
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
//     Macros that customize the sparse matrix functions.
//  spmatrix.h
//     Macros and declarations to be imported by the user.
//  spmacros.h
//     Macro definitions for the sparse matrix functions.
//
#define spINSIDE_SPARSE
#include "spconfig.h"
#include "spmatrix.h"
#include "spmacros.h"

#ifdef WRSPICE
#include "ttyio.h"
#define PRINTF TTY.err_printf
#else
#define PRINTF printf
#endif


//  MATRIX SOLVE MODULE
//
//  Author:                     Advising professor:
//      Kenneth S. Kundert          Alberto Sangiovanni-Vincentelli
//      UC Berkeley
//
// This file contains the forward and backward substitution functions
// for the sparse matrix functions.
//
//  >>> Public functions contained in this file:
//
//  spSolve
//  spSolveTransposed
//
//  >>> Private functions contained in this file:
//
//  SolveComplexMatrix
//  SolveComplexTransposedMatrix


//  SOLVE MATRIX EQUATION
//
// Performs forward elimination and back substitution to find the
// unknown vector from the RHS vector and factored matrix.  This
// function assumes that the pivots are associated with the lower
// triangular (L) matrix and that the diagonal of the upper triangular
// (U) matrix consists of ones.  This function arranges the
// computation in different way than is traditionally used in order to
// exploit the sparsity of the right-hand side.  See the reference in
// spRevision.
//
//  >>> Arguments:
//
//  rhs  <input>  (spREAL*)
//      This is the input data array, the right hand side. This data is
//      undisturbed and may be reused for other solves.
//
//  solution  <output>  (spREAL*)
//      This is the output data array.  This function is constructed
//      such that RHS and Solution can be the same array.
//
//  irhs  <input>  (spREAL*)
//      This is the imaginary portion of the input data array, the right
//      hand side.  This data is undisturbed and may be reused for other
//      solves.  This argument is only necessary if matrix is complex and
//      if SP_OPT_SEPARATED_COMPLEX_VECTOR is set true.
//
//  isolution  <output>  (spREAL*)
//      This is the imaginary portion of the output data array.  This
//      function is constructed such that irhs and isolution can be the same
//      array.  This argument is only necessary if matrix is complex and if
//      SP_OPT_SEPARATED_COMPLEX_VECTOR is set true.
//
//  >>> Local variables:
//
//  intermediate  (spREAL*)
//      Temporary storage for use in forward elimination and backward
//      substitution.  Commonly referred to as c, when the LU factorization
//      equations are given as Ax = b, Lc = b, Ux = c Local version of
//      this->Intermediate, which was created during the initial
//      factorization in function spcCreateInternalVectors() in the matrix
//      factorization module.
//
//  pElement  (spMatrixElement*)
//      Pointer used to address elements in both the lower and upper
//      triangle matrices.
//
//  pExtOrder  (int *)
//      Pointer used to sequentially access each entry in IntToExtRowMap
//      and IntToExtColMap arrays.  Used to quickly scramble and unscramble
//      rhs and solution to account for row and column interchanges.
//
//  pPivot  (spMatrixElement*)
//      Pointer that points to current pivot or diagonal element.
//
//  temp  (spREAL)
//      Temporary storage for entries in arrays.
//
//  >>> Obscure Macros
//
//  IMAG_VECTORS_P
//      Replaces itself with `, spREAL *irhs, spREAL *isolution' if
//      the options SP_OPT_COMPLEX and SP_OPT_SEPARATED_COMPLEX_VECTORS are
//      set, otherwise it disappears without a trace.
//
//  IMAG_VECTORS
//      Replaces itself with `, irhs, isolution' if the options
//      SP_OPT_COMPLEX and SP_OPT_SEPARATED_COMPLEX_VECTORS are set,
//      otherwise it disappears without a trace.
//
int
spMatrixFrame::spSolve(spREAL *rhs, spREAL *solution IMAG_VECTORS_P)
{
    ASSERT(IS_VALID() AND IS_FACTORED());

    if (Trace) {
#if SP_OPT_LONG_DBL_SOLVE
        PRINTF("solving: cplx=%d extraPrec=%d factored=%d\n", Complex,
            LongDoubles, Factored);
#else
        PRINTF("solving: cplx=%d factored=%d\n", Complex, Factored);
#endif
    }
    if (Matrix) {
        if (Complex != Matrix->is_complex())
            return (Error = spPANIC);

        if (Complex) {
            if (!Intermediate)
                Intermediate = new double[Size+Size+2];

            // Initialize Intermediate vector.
            int *pExtOrder = &IntToExtRowMap[Size];
            spCOMPLEX *pi = (spCOMPLEX *)Intermediate;
#if SP_OPT_SEPARATED_COMPLEX_VECTORS
            for (int i = Size; i > 0; i--) {
                pi[i].Real = rhs[*(pExtOrder)];
                pi[i].Imag = irhs[*(pExtOrder--)];
            }
#else
            spCOMPLEX *extVector = (spCOMPLEX *)rhs;
            for (int i = Size; i > 0; i--)
                pi[i] = extVector[*(pExtOrder--)];
#endif

            Error = Matrix->solve(Intermediate + 2);

            // Unscramble Intermediate vector while placing data in to
            // Solution vector.
            pExtOrder = &IntToExtColMap[Size];
            pi = (spCOMPLEX *)Intermediate;
#if SP_OPT_SEPARATED_COMPLEX_VECTORS
            for (int i = Size; i > 0; i--) {
                solution[*(pExtOrder)] = pi[i].Real;
                isolution[*(pExtOrder--)] = pi[i].Imag;
            }
#else
            extVector = (spCOMPLEX *)solution;
            for (int i = Size; i > 0; i--)
                extVector[*(pExtOrder--)] = pi[i];
#endif

        }
        else {
            if (!Intermediate)
                // Make this large enough for the complex case, in
                // case we switch.
                Intermediate = new double[Size+Size+2];

            // Initialize Intermediate vector.
            int *pExtOrder = &IntToExtRowMap[Size];
            for (int i = Size; i > 0; i--)
                Intermediate[i] = rhs[*(pExtOrder--)];

            Error = Matrix->solve(Intermediate + 1);

            // Unscramble Intermediate vector while placing data in to
            // Solution vector.
            pExtOrder = &IntToExtColMap[Size];
            for (int i = Size; i > 0; i--)
                solution[*(pExtOrder--)] = Intermediate[i];
        }
        return (Error);
    }

#if SP_OPT_COMPLEX
    if (Complex) {
        SolveComplexMatrix(rhs, solution IMAG_VECTORS);
        return (spOKAY);
    }
#endif

#if SP_OPT_REAL

    // Correct array pointers for SP_OPT_ARRAY_OFFSET.
#if NOT SP_OPT_ARRAY_OFFSET
    --rhs;
    --solution;
#endif

#if SP_OPT_LONG_DBL_SOLVE
    if (LongDoubles) {
        long double *intermediate = (long double*)Intermediate;

        // Initialize Intermediate vector.
        int *pExtOrder = &IntToExtRowMap[Size];
        for (int i = Size; i > 0; i--)
            intermediate[i] = rhs[*(pExtOrder--)];

        // Forward elimination. Solves Lc = b.
        for (int i = 1; i <= Size; i++) {
            // This step of the elimination is skipped if temp equals zero.
            long double temp;
            if ((temp = intermediate[i]) != 0.0) {
                spMatrixElement *pPivot = Diag[i];
                intermediate[i] = (temp *= LDBL(pPivot));

                spMatrixElement *pElement = pPivot->NextInCol;
                while (pElement != 0) {
                    intermediate[pElement->Row] -= temp * LDBL(pElement);
                    pElement = pElement->NextInCol;
                }
            }
        }

        // Backward Substitution. Solves Ux = c.
        for (int i = Size; i > 0; i--) {
            long double temp = intermediate[i];
            spMatrixElement *pElement = Diag[i]->NextInRow;
            while (pElement != 0) {
                temp -= LDBL(pElement) * intermediate[pElement->Col];
                pElement = pElement->NextInRow;
            }
            intermediate[i] = temp;
        }

        // Unscramble Intermediate vector while placing data in to Solution
        // vector.
        pExtOrder = &IntToExtColMap[Size];
        for (int i = Size; i > 0; i--)
            solution[*(pExtOrder--)] = intermediate[i];

        return (spOKAY);
    }
#endif // SP_OPT_LONG_DBL_SOLVE

    spREAL *intermediate = Intermediate;

    // Initialize Intermediate vector.
    int *pExtOrder = &IntToExtRowMap[Size];
    for (int i = Size; i > 0; i--)
        intermediate[i] = rhs[*(pExtOrder--)];

    // Forward elimination. Solves Lc = b.
    for (int i = 1; i <= Size; i++) {
        // This step of the elimination is skipped if temp equals zero.
        spREAL temp;
        if ((temp = intermediate[i]) != 0.0) {
            spMatrixElement *pPivot = Diag[i];
            intermediate[i] = (temp *= pPivot->Real);

            spMatrixElement *pElement = pPivot->NextInCol;
            while (pElement != 0) {
                intermediate[pElement->Row] -= temp * pElement->Real;
                pElement = pElement->NextInCol;
            }
        }
    }

    // Backward Substitution. Solves Ux = c.
    for (int i = Size; i > 0; i--) {
        spREAL temp = intermediate[i];
        spMatrixElement *pElement = Diag[i]->NextInRow;
        while (pElement != 0) {
            temp -= pElement->Real * intermediate[pElement->Col];
            pElement = pElement->NextInRow;
        }
        intermediate[i] = temp;
    }

    // Unscramble Intermediate vector while placing data in to Solution
    // vector.
    pExtOrder = &IntToExtColMap[Size];
    for (int i = Size; i > 0; i--)
        solution[*(pExtOrder--)] = intermediate[i];

#endif // SP_OPT_REAL
    return (spOKAY);
}


#if SP_OPT_COMPLEX

//  SOLVE COMPLEX MATRIX EQUATION
//  Private function
//
// Performs forward elimination and back substitution to find the
// unknown vector from the RHS vector and factored matrix.  This
// function assumes that the pivots are associated with the lower
// triangular (L) matrix and that the diagonal of the upper triangular
// (U) matrix consists of ones.  This function arranges the
// computation in different way than is traditionally used in order to
// exploit the sparsity of the right-hand side.  See the reference in
// spRevision.
//
//  >>> Arguments:
//
//  rhs  <input>  (spREAL*)
//      This is the real portion of the input data array, the right hand
//      side.  This data is undisturbed and may be reused for other solves.
//
//  solution  <output>  (spREAL*)
//      This is the real portion of the output data array.  This
//      function is constructed such that rhs and solution can be the same
//      array.
//
//  irhs  <input>  (spREAL*)
//      This is the imaginary portion of the input data array, the right
//      hand side.  This data is undisturbed and may be reused for other
//      solves.  If SP_OPT_SEPARATED_COMPLEX_VECTOR is set false, there is no
//      need to supply this array.
//
//  isolution  <output>  (spREAL*)
//      This is the imaginary portion of the output data array.  This
//      function is constructed such that irhs and isolution can be the same
//      array.  If SP_OPT_SEPARATED_COMPLEX_VECTOR is set false, there is no
//      need to supply this array.
//
//  >>> Local variables:
//
//  intermediate  (spCOMPLEX*)
//      Temporary storage for use in forward elimination and backward
//      substitution.  Commonly referred to as c, when the LU factorization
//      equations are given as Ax = b, Lc = b, Ux = c.  Local version of
//      this->Intermediate, which was created during the initial
//      factorization in function spcCreateInternalVectors() in the matrix
//      factorization module.
//
//  pElement  (spMatrixElement*)
//      Pointer used to address elements in both the lower and upper
//      triangle matrices.
//
//  pExtOrder  (int *)
//      Pointer used to sequentially access each entry in IntToExtRowMap
//      and IntToExtColMap arrays.  Used to quickly scramble and unscramble
//      rhs and solution to account for row and column interchanges.
//
//  pPivot  (spMatrixElement*)
//      Pointer that points to current pivot or diagonal element.
//
//  temp  (spCOMPLEX)
//      Temporary storage for entries in arrays.
//
//  >>> Obscure Macros
//
//  IMAG_VECTORS_P
//      Replaces itself with `, spREAL *irhs, spREAL *isolution' if
//      the options SP_OPT_COMPLEX and SP_OPT_SEPARATED_COMPLEX_VECTORS are
//      set, otherwise it disappears without a trace.
//
void
spMatrixFrame::SolveComplexMatrix(spREAL *rhs, spREAL *solution IMAG_VECTORS_P)
{
    spCOMPLEX *intermediate = (spCOMPLEX *)Intermediate;

    // Correct array pointers for SP_OPT_ARRAY_OFFSET.
#if NOT SP_OPT_ARRAY_OFFSET
#if SP_OPT_SEPARATED_COMPLEX_VECTORS
    --rhs;      --irhs;
    --solution; --isolution;
#else
    rhs -= 2; solution -= 2;
#endif
#endif

    // Initialize Intermediate vector.
    int *pExtOrder = &IntToExtRowMap[Size];

#if SP_OPT_SEPARATED_COMPLEX_VECTORS
    for (int i = Size; i > 0; i--) {
        intermediate[i].Real = rhs[*(pExtOrder)];
        intermediate[i].Imag = irhs[*(pExtOrder--)];
    }
#else
    spCOMPLEX *extVector = (spCOMPLEX *)rhs;
    for (int i = Size; i > 0; i--)
        intermediate[i] = extVector[*(pExtOrder--)];
#endif

    // Forward substitution. Solves Lc = b.
    for (int i = 1; i <= Size; i++) {
        spCOMPLEX temp = intermediate[i];

        // This step of the substitution is skipped if Temp equals zero.
        if ((temp.Real != 0.0) OR (temp.Imag != 0.0)) {
            spMatrixElement *pPivot = Diag[i];
            // Cmplx expr: Temp *= (1.0 / Pivot)
            CMPLX_MULT_ASSIGN(temp, *pPivot);
            intermediate[i] = temp;
            spMatrixElement *pElement = pPivot->NextInCol;
            while (pElement != 0) {
                // Cmplx expr: Intermediate[Element->Row] -= Temp * *Element
                CMPLX_MULT_SUBT_ASSIGN(intermediate[pElement->Row],
                    temp, *pElement);
                pElement = pElement->NextInCol;
            }
        }
    }

    // Backward Substitution. Solves Ux = c.
    for (int i = Size; i > 0; i--) {
        spCOMPLEX temp = intermediate[i];
        spMatrixElement *pElement = Diag[i]->NextInRow;

        while (pElement != 0) {
            // Cmplx expr: Temp -= *Element * Intermediate[Element->Col]
            CMPLX_MULT_SUBT_ASSIGN(temp, *pElement,
                intermediate[pElement->Col]);
            pElement = pElement->NextInRow;
        }
        intermediate[i] = temp;
    }

    // Unscramble Intermediate vector while placing data in to solution
    // vector.
    pExtOrder = &IntToExtColMap[Size];

#if SP_OPT_SEPARATED_COMPLEX_VECTORS
    for (int i = Size; i > 0; i--) {
        solution[*(pExtOrder)] = intermediate[i].Real;
        isolution[*(pExtOrder--)] = intermediate[i].Imag;
    }
#else
    ExtVector = (spCOMPLEX *)solution;
    for (int i = Size; i > 0; i--)
        ExtVector[*(pExtOrder--)] = intermediate[i];
#endif
}

#endif // SP_OPT_COMPLEX


#if SP_OPT_TRANSPOSE

//  SOLVE TRANSPOSED MATRIX EQUATION
//
// Performs forward elimination and back substitution to find the
// unknown vector from the RHS vector and transposed factored matrix. 
// This function is useful when performing sensitivity analysis on a
// circuit using the adjoint method.  This function assumes that the
// pivots are associated with the untransposed lower triangular (L)
// matrix and that the diagonal of the untransposed upper triangular
// (U) matrix consists of ones.
//
//  >>> Arguments:
//
//  rhs  <input>  (spREAL*)
//      This is the input data array, the right hand side.  This data is
//      undisturbed and may be reused for other solves.
//
//  solution  <output>  (spREAL*)
//      This is the output data array.  This function is constructed
//      such that rhs and solution can be the same array.
//
//  irhs  <input>  (spREAL*)
//      This is the imaginary portion of the input data array, the right
//      hand side.  This data is undisturbed and may be reused for other
//      solves.  If SP_OPT_SEPARATED_COMPLEX_VECTOR is set false, or if
//      matrix is real, there is no need to supply this array.
//
//  isolution  <output>  (spREAL*)
//      This is the imaginary portion of the output data array.  This
//      function is constructed such that irhs and isolution can be the
//      same array.  If SP_OPT_SEPARATED_COMPLEX_VECTOR is set false, or if
//      matrix is real, there is no need to supply this array.
//
//  >>> Local variables:
//
//  intermediate  (spREAL*)
//      Temporary storage for use in forward elimination and backward
//      substitution.  Commonly referred to as c, when the LU factorization
//      equations are given as Ax = b, Lc = b, Ux = c.  Local version of
//      this->Intermediate, which was created during the initial
//      factorization in function spcCreateInternalVectors() in the matrix
//      factorization module.
//
//  pElement  (spMatrixElement*)
//      Pointer used to address elements in both the lower and upper
//      triangle matrices.
//
//  pExtOrder  (int *)
//      Pointer used to sequentially access each entry in IntToExtRowMap
//      and IntToExtRowMap arrays.  Used to quickly scramble and unscramble
//      rhs and solution to account for row and column interchanges.
//
//  pPivot  (spMatrixElement*)
//      Pointer that points to current pivot or diagonal element.
//
//  temp  (spREAL)
//      Temporary storage for entries in arrays.
//
//  >>> Obscure Macros
//
//  IMAG_VECTORS_P
//      Replaces itself with `, spREAL *irhs, spREAL *isolution' if
//      the options SP_OPT_COMPLEX and SP_OPT_SEPARATED_COMPLEX_VECTORS are
//      set, otherwise it disappears without a trace.
//
//  IMAG_VECTORS
//      Replaces itself with `, irhs, isolution' if the options
//      SP_OPT_COMPLEX and SP_OPT_SEPARATED_COMPLEX_VECTORS are set,
//      otherwise it disappears without a trace.
//
int
spMatrixFrame::spSolveTransposed(spREAL *rhs, spREAL *solution IMAG_VECTORS_P)
{
    ASSERT(IS_VALID() AND IS_FACTORED());

    if (Trace) {
#if SP_OPT_LONG_DBL_SOLVE
        PRINTF("solving T: cplx=%d extraPrec=%d factored=%d\n", Complex,
            LongDoubles, Factored);
#else
        PRINTF("solving T: cplx=%d factored=%d\n", Complex, Factored);
#endif
    }
    if (Matrix) {
        if (Complex != Matrix->is_complex())
            return (Error = spPANIC);

        if (Complex) {
            if (!Intermediate)
                Intermediate = new double[Size+Size+2];

            // Initialize Intermediate vector.
            int *pExtOrder = &IntToExtColMap[Size];
            spCOMPLEX *pi = (spCOMPLEX *)Intermediate;
#if SP_OPT_SEPARATED_COMPLEX_VECTORS
            for (int i = Size; i > 0; i--) {
                pi[i].Real = rhs[*(pExtOrder)];
                pi[i].Imag = irhs[*(pExtOrder--)];
            }
#else
            spCOMPLEX *extVector = (spCOMPLEX *)rhs;
            for (int i = Size; i > 0; i--)
                pi[i] = extVector[*(pExtOrder--)];
#endif

            Error = Matrix->tsolve(Intermediate + 2, false);

            // Unscramble Intermediate vector while placing data in to
            // Solution vector.
            pExtOrder = &IntToExtRowMap[Size];
            pi = (spCOMPLEX *)Intermediate;
#if SP_OPT_SEPARATED_COMPLEX_VECTORS
            for (int i = Size; i > 0; i--) {
                solution[*(pExtOrder)] = pi[i].Real;
                isolution[*(pExtOrder--)] = pi[i].Imag;
            }
#else
            extVector = (spCOMPLEX *)solution;
            for (int i = Size; i > 0; i--)
                extVector[*(pExtOrder--)] = pi[i];
#endif

        }
        else {
            if (!Intermediate)
                // Make this large enough for the complex case, in
                // case we switch.
                Intermediate = new double[Size+Size+2];

            // Initialize Intermediate vector.
            int *pExtOrder = &IntToExtColMap[Size];
            for (int i = Size; i > 0; i--)
                Intermediate[i] = rhs[*(pExtOrder--)];

            Error = Matrix->tsolve(Intermediate + 1, false);

            // Unscramble Intermediate vector while placing data in to
            // Solution vector.
            pExtOrder = &IntToExtRowMap[Size];
            for (int i = Size; i > 0; i--)
                solution[*(pExtOrder--)] = Intermediate[i];
        }
        return (Error);
    }

#if SP_OPT_COMPLEX
    if (Complex) {
        SolveComplexTransposedMatrix(rhs, solution IMAG_VECTORS);
        return (spOKAY);
    }
#endif

#if SP_OPT_REAL

    // Correct array pointers for SP_OPT_ARRAY_OFFSET.
#if NOT SP_OPT_ARRAY_OFFSET
    --rhs;
    --solution;
#endif

#if SP_OPT_LONG_DBL_SOLVE
    if (LongDoubles) {
        long double *intermediate = (long double*)Intermediate;

        // Initialize Intermediate vector.
        int *pExtOrder = &IntToExtColMap[Size];
        for (int i = Size; i > 0; i--)
            intermediate[i] = rhs[*(pExtOrder--)];

        // Forward elimination.
        for (int i = 1; i <= Size; i++) {
            // This step of the elimination is skipped if temp equals zero.
            long double temp;
            if ((temp = intermediate[i]) != 0.0) {
                spMatrixElement *pElement = Diag[i]->NextInRow;
                while (pElement != 0) {
                    intermediate[pElement->Col] -= temp * LDBL(pElement);
                    pElement = pElement->NextInRow;
                }
            }
        }

        // Backward Substitution.
        for (int i = Size; i > 0; i--) {
            spMatrixElement *pPivot = Diag[i];
            long double temp = intermediate[i];
            spMatrixElement *pElement = pPivot->NextInCol;
            while (pElement != 0) {
                temp -= LDBL(pElement) * intermediate[pElement->Row];
                pElement = pElement->NextInCol;
            }
            intermediate[i] = temp * LDBL(pPivot);
        }

        // Unscramble Intermediate vector while placing data in to solution
        // vector.
        pExtOrder = &IntToExtRowMap[Size];
        for (int i = Size; i > 0; i--)
            solution[*(pExtOrder--)] = intermediate[i];

        return (spOKAY);
    }
#endif // SP_OPT_LONG_DBL_SOLVE

    spREAL *intermediate = Intermediate;

    // Initialize Intermediate vector.
    int *pExtOrder = &IntToExtColMap[Size];
    for (int i = Size; i > 0; i--)
        intermediate[i] = rhs[*(pExtOrder--)];

    // Forward elimination.
    for (int i = 1; i <= Size; i++) {
        // This step of the elimination is skipped if temp equals zero.
        spREAL temp;
        if ((temp = intermediate[i]) != 0.0) {
            spMatrixElement *pElement = Diag[i]->NextInRow;
            while (pElement != 0) {
                intermediate[pElement->Col] -= temp * pElement->Real;
                pElement = pElement->NextInRow;
            }

        }
    }

    // Backward Substitution.
    for (int i = Size; i > 0; i--) {
        spMatrixElement *pPivot = Diag[i];
        spREAL temp = intermediate[i];
        spMatrixElement *pElement = pPivot->NextInCol;
        while (pElement != 0) {
            temp -= pElement->Real * intermediate[pElement->Row];
            pElement = pElement->NextInCol;
        }
        intermediate[i] = temp * pPivot->Real;
    }

    // Unscramble Intermediate vector while placing data in to solution
    // vector.
    pExtOrder = &IntToExtRowMap[Size];
    for (int i = Size; i > 0; i--)
        solution[*(pExtOrder--)] = intermediate[i];

#endif // SP_OPT_REAL
    return (spOKAY);
}

#endif // SP_OPT_TRANSPOSE


#if SP_OPT_TRANSPOSE AND SP_OPT_COMPLEX

//  SOLVE COMPLEX TRANSPOSED MATRIX EQUATION
//  Private function
//
// Performs forward elimination and back substitution to find the
// unknown vector from the RHS vector and transposed factored matrix. 
// This function is useful when performing sensitivity analysis on a
// circuit using the adjoint method.  This function assumes that the
// pivots are associated with the untransposed lower triangular (L)
// matrix and that the diagonal of the untransposed upper triangular
// (U) matrix consists of ones.
//
//  >>> Arguments:
//
//  rhs  <input>  (spREAL*)
//      This is the input data array, the right hand side.  This data is
//      undisturbed and may be reused for other solves.  This vector is
//      only the real portion if the matrix is complex and
//      SP_OPT_SEPARATED_COMPLEX_VECTORS is set true.
//
//  solution  <output>  (spREAL*)
//      This is the real portion of the output data array.  This function
//      is constructed such that rhs and solution can be the same array. 
//      This vector is only the real portion if the matrix is complex and
//      SP_OPT_SEPARATED_COMPLEX_VECTORS is set true.
//
//  irhs  <input>  (spREAL*)
//      This is the imaginary portion of the input data array, the right
//      hand side.  This data is undisturbed and may be reused for other
//      solves.  If either SP_OPT_COMPLEX or
//      SP_OPT_SEPARATED_COMPLEX_VECTOR is set false, there is no need to
//      supply this array.
//
//  isolution  <output>  (spREAL*)
//      This is the imaginary portion of the output data array.  This
//      function is constructed such that irhs and isolution can be the
//      same array.  If SP_OPT_COMPLEX or SP_OPT_SEPARATED_COMPLEX_VECTOR
//      is set false, there is no need to supply this array.
//
//  >>> Local variables:
//
//  intermediate  (spCOMPLEX*)
//      Temporary storage for use in forward elimination and backward
//      substitution.  Commonly referred to as c, when the LU factorization
//      equations are given as Ax = b, Lc = b, Ux = c.  Local version of
//      this->Intermediate, which was created during the initial
//      factorization in function spcCreateInternalVectors() in the matrix
//      factorization module.
//
//  pElement  (spMatrixElement*)
//      Pointer used to address elements in both the lower and upper
//      triangle matrices.
//
//  pExtOrder  (int *)
//      Pointer used to sequentially access each entry in IntToExtRowMap
//      and IntToExtColMap arrays.  Used to quickly scramble and unscramble
//      rhs and solution to account for row and column interchanges.
//
//  pPivot  (spMatrixElement*)
//      Pointer that points to current pivot or diagonal element.
//
//  Temp  (spCOMPLEX)
//      Temporary storage for entries in arrays.
//
//  >>> Obscure Macros
//
//  IMAG_VECTORS_P
//      Replaces itself with `, spREAL *irhs, spREAL *isolution' if
//      the options SP_OPT_COMPLEX and SP_OPT_SEPARATED_COMPLEX_VECTORS are
//      set, otherwise it disappears without a trace.
//
void
spMatrixFrame::SolveComplexTransposedMatrix(spREAL *rhs, spREAL *solution
    IMAG_VECTORS_P)
{
    spCOMPLEX *intermediate = (spCOMPLEX *)Intermediate;

    // Correct array pointers for SP_OPT_ARRAY_OFFSET.
#if NOT SP_OPT_ARRAY_OFFSET
#if SP_OPT_SEPARATED_COMPLEX_VECTORS
    --rhs;      --irhs;
    --solution; --isolution;
#else
    rhs -= 2;   solution -= 2;
#endif
#endif

    // Initialize intermediate vector.
    int *pExtOrder = &IntToExtColMap[Size];

#if SP_OPT_SEPARATED_COMPLEX_VECTORS
    for (int i = Size; i > 0; i--) {
        intermediate[i].Real = rhs[*(pExtOrder)];
        intermediate[i].Imag = irhs[*(pExtOrder--)];
    }
#else
    spCOMPLEX *extVector = (spCOMPLEX *)rhs;
    for (int i = Size; i > 0; i--)
        intermediate[i] = extVector[*(pExtOrder--)];
#endif

    // Forward elimination.
    for (int i = 1; i <= Size; i++) {
        spCOMPLEX temp = intermediate[i];

        // This step of the elimination is skipped if temp equals zero.
        if ((temp.Real != 0.0) OR (temp.Imag != 0.0)) {
            spMatrixElement *pElement = Diag[i]->NextInRow;
            while (pElement != 0) {
                // Cmplx expr: Intermediate[Element->Col] -= Temp * *Element
                CMPLX_MULT_SUBT_ASSIGN(intermediate[pElement->Col],
                    temp, *pElement);
                pElement = pElement->NextInRow;
            }
        }
    }

    // Backward Substitution.
    for (int i = Size; i > 0; i--) {
        spMatrixElement *pPivot = Diag[i];
        spCOMPLEX temp = intermediate[i];
        spMatrixElement *pElement = pPivot->NextInCol;

        while (pElement != 0) {
            // Cmplx expr: Temp -= Intermediate[Element->Row] * *Element
            CMPLX_MULT_SUBT_ASSIGN(temp, intermediate[pElement->Row],
                *pElement);

            pElement = pElement->NextInCol;
        }
        // Cmplx expr: Intermediate = Temp * (1.0 / *pPivot)
        CMPLX_MULT(intermediate[i], temp, *pPivot);
    }

    // Unscramble Intermediate vector while placing data in to solution
    // vector.
    pExtOrder = &IntToExtRowMap[Size];

#if SP_OPT_SEPARATED_COMPLEX_VECTORS
    for (int i = Size; i > 0; i--) {
        solution[*(pExtOrder)] = intermediate[i].Real;
        isolution[*(pExtOrder--)] = intermediate[i].Imag;
    }
#else
    extVector = (spCOMPLEX *)solution;
    for (int i = Size; i > 0; i--)
        extVector[*(pExtOrder--)] = intermediate[i];
#endif
}

#endif // SP_OPT_TRANSPOSE AND SP_OPT_COMPLEX

