
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
//  EXPORTS for sparse matrix functions with SPICE3.
//
//  Author:                     Advising professor:
//      Kenneth S. Kundert          Alberto Sangiovanni-Vincentelli
//      UC Berkeley
//
// This file contains definitions that are useful to the calling
// program.  In particular, this file contains error keyword
// definitions, some macro functions that are used to quickly enter
// data into the matrix and the type definition of a data structure
// that acts as a template for entering admittances into the matrix. 
// Also included is the type definitions for the various functions
// available to the user.
//
// This file is a modified version of spMatrix.h that is used when
// interfacing to Spice3.

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
//========================================================================

#ifndef SPMATRIX_H
#define SPMATRIX_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>

// SRW - These configure application support, defined in the makefile.
// #define WRSPICE
// #define XIC
// #define USE_KLU


//  PACKAGE OPTIONS
//
// These are compiler options.  Set each option to one to compile that
// section of the code.  If a feature is not desired, set the macro to
// 0.
//
//  >>> Option descriptions:
//
//  Arithmetic Precision
//      The precision of the arithmetic used by Sparse can be set by
//      changing changing the spREAL macro.  This macro is
//      contained in the file spMatrix.h.  It is strongly suggested to
//      used double precision with circuit simulators.  Note that
//      because C always performs arithmetic operations in double
//      precision, the only benefit to using single precision is that
//      less storage is required.  There is often a noticeable speed
//      penalty when using single precision.
//
//  SP_OPT_REAL
//      This specifies that the functions are expected to handle real
//      systems of equations.  The functions can be compiled to handle
//      both real and complex systems at the same time, but there is a
//      slight speed and memory advantage if the functions are complied
//      to handle only real systems of equations.
//
//  SP_OPT_COMPLEX
//      This specifies that the functions will be complied to handle
//      complex systems of equations.
//
//  SP_OPT_EXPANDABLE
//      Setting this compiler flag true (1) makes the matrix
//      expandable before it has been factored.  If the matrix is
//      expandable, then if an element is added that would be
//      considered out of bounds in the current matrix, the size of
//      the matrix is increased to hold that element.  As a result,
//      the size of the matrix need not be known before the matrix is
//      built.  The matrix can be allocated with size zero and
//      expanded.
//
//  SP_OPT_TRANSLATE
//      This option allows the set of external row and column numbers
//      to be non-packed.  In other words, the row and column numbers
//      do not have to be contiguous.  The priced paid for this
//      flexibility is that when SP_OPT_TRANSLATE is set true, the time
//      required to initially build the matrix will be greater because
//      the external row and column number must be translated into
//      internal equivalents.  This translation brings about other
//      benefits though.  First, the spGetElement() and
//      spGetAdmittance() functions may be used after the matrix has
//      been factored.  Further, elements, and even rows and columns,
//      may be added to the matrix, and row and columns may be deleted
//      from the matrix, after it has been factored.  Note that when
//      the set of row and column number is not a packed set, neither
//      are the RHS and Solution vectors.  Thus the size of these
//      vectors must be at least as large as the external size, which
//      is the value of the largest given row or column numbers.
//
//  SP_OPT_INITIALIZE
//      Causes the spInitialize(), spGetInitInfo(), and
//      spInstallInitInfo() functions to be compiled.  These functions
//      allow the user to store and read one pointer in each nonzero
//      element in the matrix.  spInitialize() then calls a user
//      specified function for each structural nonzero in the matrix,
//      and includes this pointer as well as the external row and
//      column numbers as arguments.  This allows the user to write
//      custom matrix initialization functions.
//
//  SP_OPT_DIAGONAL_PIVOTING
//      Many matrices, and in particular node- and modified-node
//      admittance matrices, tend to be nearly symmetric and nearly
//      diagonally dominant.  For these matrices, it is a good idea to
//      select pivots from the diagonal.  With this option enabled,
//      this is exactly what happens, though if no satisfactory pivot
//      can be found on the diagonal, an off-diagonal pivot will be
//      used.  If this option is disabled, Sparse does not
//      preferentially search the diagonal.  Because of this, Sparse
//      has a wider variety of pivot candidates available, and so
//      presumably fewer fill-ins will be created.  However, the
//      initial pivot selection process will take considerably longer.
//      If working with node admittance matrices, or other matrices
//      with a strong diagonal, it is probably best to use
//      SP_OPT_DIAGONAL_PIVOTING for two reasons.  First, accuracy will
//      be better because pivots will be chosen from the large diagonal
//      elements, thus reducing the chance of growth.  Second, a near
//      optimal ordering will be chosen quickly.  If the class of
//      matrices you are working with does not have a strong diagonal,
//      do not use SP_OPT_DIAGONAL_PIVOTING, but consider using a larger
//      threshold.  When SP_OPT_DIAGONAL_PIVOTING is turned off,
//      the following options and constants are not used:
//      SP_OPT_MODIFIED_MARKOWITZ, MAX_MARKOWITZ_TIES, and TIES_MULTIPLIER.
//
//  SP_OPT_ARRAY_OFFSET
//      This determines whether arrays start at an index of zero or one.
//      This option is necessitated by the fact that standard C
//      convention dictates that arrays begin with an index of zero but
//      the standard mathematic convention states that arrays begin with
//      an index of one.  So if you prefer to start your arrays with
//      zero, or your calling Sparse from FORTRAN, set SP_OPT_ARRAY_OFFSET
//      to 0.  Otherwise, set SP_OPT_ARRAY_OFFSET to 1.  Note that if
//      you use an offset of one, the arrays that you pass to Sparse
//      must have an allocated length of one plus the size of the
//      matrix.  SP_OPT_ARRAY_OFFSET must be either 0 or 1, no other
//      offsets are valid.
//
//  SP_OPT_SEPARATED_COMPLEX_VECTORS
//      This specifies the format for complex vectors.  If this is set
//      false then a complex vector is made up of one double sized
//      array of spREAL's in which the real and imaginary numbers
//      are placed in the alternately array in the array.  In other
//      words, the first entry would be Complex[1].Real, then comes
//      Complex[1].Imag, then Complex[1].Real, etc.  If
//      SP_OPT_SEPARATED_COMPLEX_VECTORS is set true, then each complex
//      vector is represented by two arrays of spREAL, one with
//      the real terms, the other with the imaginary. [NO]
//
//  SP_OPT_MODIFIED_MARKOWITZ
//      This specifies that the modified Markowitz method of pivot
//      selection is to be used.  The modified Markowitz method differs
//      from standard Markowitz in two ways.  First, under modified
//      Markowitz, the search for a pivot can be terminated early if a
//      adequate (in terms of sparsity) pivot candidate is found.
//      Thus, when using modified Markowitz, the initial factorization
//      can be faster, but at the expense of a suboptimal pivoting
//      order that may slow subsequent factorizations.  The second
//      difference is in the way modified Markowitz breaks Markowitz
//      ties.  When two or more elements are pivot candidates and they
//      all have the same Markowitz product, then the tie is broken by
//      choosing the element that is best numerically.  The numerically
//      best element is the one with the largest ratio of its magnitude
//      to the magnitude of the largest element in the same column,
//      excluding itself.  The modified Markowitz method results in
//      marginally better accuracy.  This option is most appropriate
//      for use when working with very large matrices where the initial
//      factor time represents an unacceptable burden. [NO]
//
//  SP_OPT_DELETE
//      This specifies that the spDeleteRowAndCol() function
//      should be compiled.  Note that for this function to be
//      compiled, both SP_OPT_DELETE and SP_OPT_TRANSLATE should be
//      set true.
//
//  SP_OPT_STRIP
//      This specifies that the spStripFills() function should be compiled.
//
//  SP_OPT_MODIFIED_NODAL
//      This specifies that the function that preorders modified node
//      admittance matrices should be compiled.  This function results
//      in greater speed and accuracy if used with this type of
//      matrix.
//
//  SP_OPT_QUAD_ELEMENT
//      This specifies that the functions that allow four related
//      elements to be entered into the matrix at once should be
//      compiled.  These elements are usually related to an
//      admittance.  The functions affected by SP_OPT_QUAD_ELEMENT
//      are the spGetAdmittance, spGetQuad and spGetOnes functions.
//
//  SP_OPT_TRANSPOSE
//      This specifies that the functions that solve the matrix as if
//      it was transposed should be compiled.  These functions are
//      useful when performing sensitivity analysis using the adjoint
//      method.
//
//  SP_OPT_SCALING
//      This specifies that the function that performs scaling on the
//      matrix should be complied.  Scaling is not strongly
//      supported.  The function to scale the matrix is provided, but
//      no functions are provided to scale and descale the RHS and
//      Solution vectors.  It is suggested that if scaling is desired,
//      it only be preformed when the pivot order is being chosen [in
//      spOrderAndFactor()].  This is the only time scaling has
//      an effect.  The scaling may then either be removed from the
//      solution by the user or the scaled factors may simply be
//      thrown away. [NO]
//
//  SP_OPT_DOCUMENTATION
//      This specifies that functions that are used to document the
//      matrix, such as spPrint() and spFileMatrix(), should be
//      compiled.
//
//  SP_OPT_DETERMINANT
//      This specifies that the function spDeterminant() should be complied.
//
//  SP_OPT_STABILITY
//      This specifies that spLargestElement() and spRoundoff() should
//      be compiled.  These functions are used to check the stability (and
//      hence the quality of the pivoting) of the factorization by
//      computing a bound on the size of the element is the matrix E =
//      A - LU.  If this bound is very high after applying
//      spOrderAndFactor(), then the pivot threshold should be raised.
//      If the bound increases greatly after using spFactor(), then the
//      matrix should probably be reordered.
//
//  SP_OPT_CONDITION
//      This specifies that spCondition() and spNorm(), the code that
//      computes a good estimate of the condition number of the matrix,
//      should be compiled.
//
//  SP_OPT_PSEUDOCONDITION
//      This specifies that spPseudoCondition(), the code that computes
//      a crude and easily fooled indicator of ill-conditioning in the
//      matrix, should be compiled.
//
//  SP_OPT_MULTIPLICATION
//      This specifies that the functions to multiply the unfactored
//      matrix by a vector should be compiled.
//
//  SP_OPT_INTERRUPT (SRW)
//      Allow registration of an interrupt-checking callback.  If the
//      callback returns true, the present function immediately returns
//      spABORTED.  The matrix should be considered dead after this.
//
//  SP_OPT_DEFAULT_PARTITION (SRW)
//      See note about PARTITION TYPES below.  This should be set to
//      one of the following, which sets the default partitioning scheme.
//
//      spDIRECT_PARTITION  -- each row uses direct addressing, best for
//          a few relatively dense matrices.
//      spINDIRECT_PARTITION  -- each row uses indirect addressing, best
//          for a few very sparse matrices.
//      spAUTO_PARTITION  -- direct or indirect addressing is chosen on
//          a row-by-row basis, carries a large overhead, but speeds up
//          both dense and sparse matrices, best if there is a large
//          number of matrices that can use the same ordering.
//
//  SP_OPT_LONG_DBL_SOLVE (SRW)
//      Factor and solve the real matrix using long double math.
//
//  SP_BUILDHASH (SRW)
//      This creates a hash table that can speed up building the sparse
//      matrix, dramatically so when the matrix is not so sparse.  This
//      avoids addressing by walking a linked list.
//
//  SP_BITFIELD (SRW)
//      This creates and manages a bitfield that identifies allocated
//      entries, requires BUILDHASH.  This provides fast random access  
//      to element pointers for linking/swapping matrix elements, which
//      speeds up reordering of no-so-sparse matrices considerably.
//
//  SP_OPT_DEBUG
//      This specifies that additional error checking will be compiled.
//      The type of error checked are those that are common when the
//      matrix functions are first integrated into a user's program.  Once
//      the functions have been integrated in and are running smoothly, this
//      option should be turned off.

#ifdef WRSPICE

#define  SP_OPT_REAL                        1
#define  SP_OPT_COMPLEX                     1
#define  SP_OPT_EXPANDABLE                  1
#define  SP_OPT_TRANSLATE                   1
#define  SP_OPT_INITIALIZE                  0
#define  SP_OPT_DIAGONAL_PIVOTING           1
#define  SP_OPT_ARRAY_OFFSET                1
#define  SP_OPT_SEPARATED_COMPLEX_VECTORS   1
#define  SP_OPT_MODIFIED_MARKOWITZ          0
#define  SP_OPT_DELETE                      0
#define  SP_OPT_STRIP                       0
#define  SP_OPT_MODIFIED_NODAL              1
#define  SP_OPT_QUAD_ELEMENT                0
#define  SP_OPT_TRANSPOSE                   1
#define  SP_OPT_SCALING                     0
#define  SP_OPT_DOCUMENTATION               1
#define  SP_OPT_MULTIPLICATION              1
#define  SP_OPT_DETERMINANT                 1
#define  SP_OPT_STABILITY                   0
#define  SP_OPT_CONDITION                   0
#define  SP_OPT_PSEUDOCONDITION             0
#define  SP_OPT_INTERRUPT                   0
#define  SP_OPT_DEFAULT_PARTITION           spINDIRECT_PARTITION
#define  SP_OPT_LONG_DBL_SOLVE              1
#define  SP_BUILDHASH                       0
#define  SP_BITFIELD                        0
#define  SP_OPT_DEBUG                       0

#else
#ifdef XIC

#define  SP_OPT_REAL                        1
#define  SP_OPT_COMPLEX                     0
#define  SP_OPT_EXPANDABLE                  1
#define  SP_OPT_TRANSLATE                   1
#define  SP_OPT_INITIALIZE                  0
#define  SP_OPT_DIAGONAL_PIVOTING           1
#define  SP_OPT_ARRAY_OFFSET                1
#define  SP_OPT_SEPARATED_COMPLEX_VECTORS   0
#define  SP_OPT_MODIFIED_MARKOWITZ          0
#define  SP_OPT_DELETE                      0
#define  SP_OPT_STRIP                       0
#define  SP_OPT_MODIFIED_NODAL              0
#define  SP_OPT_QUAD_ELEMENT                0
#define  SP_OPT_TRANSPOSE                   0
#define  SP_OPT_SCALING                     0
#define  SP_OPT_DOCUMENTATION               0
#define  SP_OPT_MULTIPLICATION              0
#define  SP_OPT_DETERMINANT                 0
#define  SP_OPT_STABILITY                   0
#define  SP_OPT_CONDITION                   0
#define  SP_OPT_PSEUDOCONDITION             0
#define  SP_OPT_INTERRUPT                   1
#define  SP_OPT_DEFAULT_PARTITION           spDIRECT_PARTITION
#define  SP_OPT_LONG_DBL_SOLVE              0
#define  SP_BUILDHASH                       1
#define  SP_BITFIELD                        1
#define  SP_OPT_SP_DEBUG                    0

#else

// Turn everything on for compilation test.
#define  SP_OPT_REAL                        1
#define  SP_OPT_COMPLEX                     1
#define  SP_OPT_EXPANDABLE                  1
#define  SP_OPT_TRANSLATE                   1
#define  SP_OPT_INITIALIZE                  1
#define  SP_OPT_DIAGONAL_PIVOTING           1
#define  SP_OPT_ARRAY_OFFSET                1
#define  SP_OPT_SEPARATED_COMPLEX_VECTORS   1
#define  SP_OPT_MODIFIED_MARKOWITZ          1
#define  SP_OPT_DELETE                      1
#define  SP_OPT_STRIP                       1
#define  SP_OPT_MODIFIED_NODAL              1
#define  SP_OPT_QUAD_ELEMENT                1
#define  SP_OPT_TRANSPOSE                   1
#define  SP_OPT_SCALING                     1
#define  SP_OPT_DOCUMENTATION               1
#define  SP_OPT_MULTIPLICATION              1
#define  SP_OPT_DETERMINANT                 1
#define  SP_OPT_STABILITY                   1
#define  SP_OPT_CONDITION                   1
#define  SP_OPT_PSEUDOCONDITION             1
#define  SP_OPT_INTERRUPT                   1
#define  SP_OPT_DEFAULT_PARTITION           spDIRECT_PARTITION
#define  SP_OPT_LONG_DBL_SOLVE              1
#define  SP_BUILDHASH                       1
#define  SP_BITFIELD                        1
#define  SP_OPT_DEBUG                       1

#endif
#endif


//  ERROR KEYWORDS
//
// The actual numbers used in the error codes are not sacred, they can
// be changed under the condition that the codes for the nonfatal
// errors are less than the code for spFATAL and similarly the codes
// for the fatal errors are greater than that for spFATAL.
//
//  >>> Error descriptions:
//
//  spOKAY
//      No error has occurred.
//
//  spSMALL_PIVOT
//      When reordering the matrix, no element was found which satisfies
//      the threshold criteria.  The largest element in the matrix was
//      chosen as pivot.  Non-fatal.
//
//  spZERO_DIAG
//      Fatal error.  A zero was encountered on the diagonal the matrix. 
//      This does not necessarily imply that the matrix is singular.  When
//      this error occurs, the matrix should be reconstructed and factored
//      using spOrderAndFactor().
//
//  spSINGULAR
//      Fatal error.  Matrix is singular, so no unique solution exists.
//
//  spNO_MEMORY
//      Fatal error.  This is lagacy from the original C code, and will not
//      be seen in the C++ version.
//
//  spPANIC
//      Fatal error indicating that the functions are not prepared to
//      handle the matrix that has been requested.  This may occur when the
//      matrix is specified to be real and the functions are not compiled
//      for real matrices, or when the matrix is specified to be complex
//      and the functions are not compiled to handle complex matrices.
//
//  spABORTED
//      Received interrupt, operation aborted (added by SRW).
//
//  spFATAL
//      Not an error flag, but rather the dividing line between fatal errors
//      and warnings.

#ifdef WRSPICE

#include "errors.h"  // Spice error definitions
// Begin error macros
#define  spABORTED              E_INTRPT
#define  spOKAY                 OK
#define  spSMALL_PIVOT          OK
#define  spFATAL                E_PANIC
#define  spPANIC                E_PANIC
#define  spZERO_DIAG            E_SINGULAR
#define  spSINGULAR             E_SINGULAR
#define  spNO_MEMORY            E_NOMEM

#else

#define  spABORTED              -1
#define  spOKAY                 0
#define  spSMALL_PIVOT          1
#define  spFATAL                2
#define  spPANIC                2
#define  spZERO_DIAG            3
#define  spSINGULAR             4
#define  spNO_MEMORY            5

#endif


// DATA TYPES
//
// Here we define what precision arithmetic Sparse will use.  Double
// precision is suggested as being most appropriate for circuit
// simulation and for C/C++.  However, it is possible to change spREAL
// to a float for single precision arithmetic.  Note that in C, single
// precision arithmetic is often slower than double precision.

typedef double spREAL;

//  COMPLEX NUMBER DATA STRUCTURE
//
//  >>> Structure fields:
//
//  Real  (spREAL)
//      The real portion of the number.  Real must be the first
//      field in this structure.
//
//  Imag  (spREAL)
//      The imaginary portion of the number. This field must follow
//      immediately after Real.
//
struct spCOMPLEX
{
    spREAL Real;
    spREAL Imag;
};

// Boolean data type.
typedef int spBOOLEAN;


//  PARTITION TYPES
//
// When factoring a previously ordered matrix using spFactor(), Sparse
// operates on a row-at-a-time basis.  For speed, on each step, the
// row being updated is copied into a full vector and the operations
// are performed on that vector.  This can be done one of two ways,
// either using direct addressing or indirect addressing.  Direct
// addressing is fastest when the matrix is relatively dense and
// indirect addressing is quite sparse.  The user can select which
// partitioning mode is used.  The following keywords are passed to
// spPartition() and indicate that Sparse should use only direct
// addressing, only indirect addressing, or that it should choose the
// best mode on a row-by-row basis.  The time required to choose a
// partition is of the same order of the cost to factor the matrix.
//
// If you plan to factor a large number of matrices with the same
// structure, it is best to let Sparse choose the partition. 
// Otherwise, you should choose the partition based on the predicted
// density of the matrix.

#define spDEFAULT_PARTITION     0
#define spDIRECT_PARTITION      1
#define spINDIRECT_PARTITION    2
#define spAUTO_PARTITION        3


//  MACRO FUNCTION DEFINITIONS
//
//  >>> Macro descriptions:
//  spADD_REAL_ELEMENT
//      Macro function that adds data to a real element in the matrix by a
//      pointer.
//
//  spADD_IMAG_ELEMENT
//      Macro function that adds data to a imaginary element in the matrix
//      by a pointer.
//
//  spADD_COMPLEX_ELEMENT
//      Macro function that adds data to a complex element in the matrix by
//      a pointer.
//
//  spADD_REAL_QUAD
//      Macro function that adds data to each of the four real matrix
//      elements specified by the given template.
//
//  spADD_IMAG_QUAD
//      Macro function that adds data to each of the four imaginary matrix
//      elements specified by the given template.
//
//  spADD_COMPLEX_QUAD
//      Macro function that adds data to each of the four complex matrix
//      elements specified by the given template.

#define  spADD_REAL_ELEMENT(element, real)      *(element) += real

#define  spADD_IMAG_ELEMENT(element, imag)      *(element+1) += imag

#define  spADD_COMPLEX_ELEMENT(element, real, imag) \
{   *(element) += real;                             \
    *(element+1) += imag;                           \
}

#define  spADD_REAL_QUAD(tmpl, real)            \
{   *((tmpl).Element1) += real;                 \
    *((tmpl).Element2) += real;                 \
    *((tmpl).Element3Negated) -= real;          \
    *((tmpl).Element4Negated) -= real;          \
}

#define  spADD_IMAG_QUAD(tmpl, imag)            \
{   *((tmpl).Element1+1) += imag;               \
    *((tmpl).Element2+1) += imag;               \
    *((tmpl).Element3Negated+1) -= imag;        \
    *((tmpl).Element4Negated+1) -= imag;        \
}

#define  spADD_COMPLEX_QUAD(tmpl, real, imag)   \
{   *((tmpl).Element1) += real;                 \
    *((tmpl).Element2) += real;                 \
    *((tmpl).Element3Negated) -= real;          \
    *((tmpl).Element4Negated) -= real;          \
    *((tmpl).Element1+1) += imag;               \
    *((tmpl).Element2+1) += imag;               \
    *((tmpl).Element3Negated+1) -= imag;        \
    *((tmpl).Element4Negated+1) -= imag;        \
}


//   TYPE DEFINITION FOR COMPONENT TEMPLATE
//
// This data structure is used to hold pointers to four related
// elements in matrix.  It is used in conjunction with the functions
//
//       spGetAdmittance
//       spGetQuad
//       spGetOnes
//
// These functions stuff the structure which is later used by the
// spADD_QUAD macro functions above.  It is also possible for the user
// to collect four pointers returned by spGetElement and stuff them
// into the template.  The spADD_QUAD functions stuff data into the
// matrix in locations specified by Element1 and Element2 without
// changing the data.  The data are negated before being placed in
// Element3 and Element4.

struct  spTemplate
{
    spTemplate()
        {
            Element1 = 0;
            Element2 = 0;
            Element3Negated = 0;
            Element4Negated = 0;
        }

    spREAL    *Element1;
    spREAL    *Element2;
    spREAL    *Element3Negated;
    spREAL    *Element4Negated;
};


//  MATRIX ELEMENT DATA STRUCTURE
//
// Every nonzero element in the matrix is stored in a dynamically
// allocated spMatrixElement structure.  These structures are linked
// together in an orthogonal linked list.  Two different spMatrixElement
// structures exist.  One is used when only real matrices are
// expected, it is missing an entry for imaginary data.  The other is
// used if complex matrices are expected.  It contains an entry for
// imaginary data.
//
//  >>> Structure fields:
//
//  Real  (spREAL)
//      The real portion of the value of the element.  Real must be the
//      first field in this structure.
//
//  Imag  (spREAL)
//      The imaginary portion of the value of the element.  If the matrix
//      functions are not compiled to handle complex matrices, then this
//      field does not exist.  If it exists, it must follow immediately
//      after Real.
//
//  Row  (int)
//      The row number of the element.
//
//  Col  (int)
//      The column number of the element.
//
//  NextInRow  (struct spMatrixElement *)
//      NextInRow contains a pointer to the next element in the row to the
//      right of this element.  If this element is the last nonzero in the
//      row then NextInRow contains 0.
//
//  NextInCol  (struct spMatrixElement *)
//      NextInCol contains a pointer to the next element in the column
//      below this element.  If this element is the last nonzero in the
//      column then NextInCol contains 0.
//
//  pInitInfo  (void *)
//      Pointer to user data used for initialization of the matrix element.
//      Initialized to 0.
//
struct  spMatrixElement
{
    spREAL          Real;
#if SP_OPT_COMPLEX
    spREAL          Imag;
#endif
#ifdef WRSPICE
    spREAL          Init;
#if SP_OPT_LONG_DBL_SOLVE
    spREAL          Init2;  // Init may actually be a long double.
#endif
#endif
    int             Row;
    int             Col;
    spMatrixElement   *NextInRow;
    spMatrixElement   *NextInCol;
#if SP_OPT_INITIALIZE
    void            *pInitInfo;
#endif
};


//  MATRIX ELEMENT FACTORY (SRW)
//
// A factory for spMatrixElement allocation.  spMatrixElements are obtained
// in blocks, corresponding to the system virtual memory page size.
//
struct spMatrixElementAllocator
{
    struct MatrixElementBlock
    {
        // No constructor or destructor.
        MatrixElementBlock *NextBlock;
        spMatrixElement Elements[1];
    };

    spMatrixElementAllocator()
        {
            Blocks = 0;
#ifdef WIN32
            BlockSize = 1 + (4096 - sizeof(MatrixElementBlock))/
                sizeof(spMatrixElement);
#else
            BlockSize = 1 + (getpagesize() - sizeof(MatrixElementBlock))/
                sizeof(spMatrixElement);
#endif
            Offset = BlockSize;
        }

    ~spMatrixElementAllocator()
        {
            Clear();
        }

    void Clear()
        {
            while (Blocks) {
                MatrixElementBlock *x = Blocks;
                Blocks = Blocks->NextBlock;
                delete [] (char*)x;
            }
            Offset = BlockSize;
        }

    spMatrixElement *NewElement()
        {
            if (Offset == BlockSize) {
                int bsz = sizeof(MatrixElementBlock) +
                    (BlockSize-1)*sizeof(spMatrixElement);
                char *c = new char[bsz];
                memset(c, 0, bsz);

                MatrixElementBlock *b = (MatrixElementBlock*)c;
                b->NextBlock = Blocks;
                Blocks = b;
                Offset = 0;
            }
            return (Blocks->Elements + Offset++);
        }

    // This is used by spStripFills to mark fillins by seting the
    // Row field of each spMatrixElement to zero.
    //
    void MarkFillins()
        {
            bool first = true;
            for (MatrixElementBlock *b = Blocks; b; b = b->NextBlock) {
                int sz;
                if (first) {
                    first = false;
                    sz = Offset;
                }
                else
                    sz = BlockSize;
                spMatrixElement *mptr = b->Elements;
                while (sz-- > 0)
                    (mptr++)->Row = 0;
            }
        }

private:
    MatrixElementBlock *Blocks;     // The current block being used.
    int Offset;                     // Offset of the next element to return.
    int BlockSize;                  // Number of elements per block.
};


// Function type passed to spMatrixFrame::spInitialize.
typedef int(*spInitializeFunc)(spREAL*, void*, int, int);


// A compact representation of the matrix, along with an interface
// for some basic operations.  If a derived class is supplied with
// SetMatlabMatrix, it will be used instead of the native matrix for
// factorization and solving.
//
struct spMatlabMatrix
{
    friend class spMatrixFrame;

    spMatlabMatrix(int size, int nelts, bool cplx, bool ldbl)
        {
            Ap = new int[size + 1];
            memset(Ap, 0, (size+1)*sizeof(int));
            Ai = new int[nelts];
            memset(Ai, 0, nelts*sizeof(int));
            Ax = new double[(1+(cplx || ldbl ? 1:0))*nelts];
            memset(Ax, 0, (1+(cplx || ldbl ? 1:0))*nelts*sizeof(double));
            Size = size;
            NumElts = nelts;
            Complex = cplx;
            LongDoubles = ldbl;
        }

    virtual ~spMatlabMatrix()
        {
            delete [] Ap;
            delete [] Ai;
            delete [] Ax;
        }

    spBOOLEAN is_complex()    { return (Complex); }

    virtual void clear() = 0;
    virtual void negate() = 0;
    virtual double largest() = 0;
    virtual double smallest() = 0;
    virtual void toInit() = 0;
    virtual void fromInit() = 0;
    virtual bool toComplex() = 0;
    virtual bool toReal(bool) = 0;
    virtual double *find(int, int) = 0;
    virtual bool checkColOnes(int) = 0;
    virtual int factor() = 0;
    virtual int refactor() = 0;
    virtual int solve(double*) = 0;
    virtual int tsolve(double*, bool) = 0;
    virtual bool where_singular(int*) = 0;

protected:
    int *Ap;
    int *Ai;
    double *Ax;
    int Size;
    int NumElts;
    spBOOLEAN Complex;
    spBOOLEAN LongDoubles;
};


//  MATRIX FRAME CONSTRUCTOR FLAGS
//
#define SP_COMPLEX      0x1
#define SP_NO_SORT      0x2
#define SP_NO_KLU       0x4
#define SP_EXT_PREC     0x8
#define SP_TRACE        0x10

#if SP_BUILDHASH
//
// Implement a hash table for quick access of elements by row/col.
//
struct spHelt
{
    int row;
    int col;
    spMatrixElement *eptr;
    struct spHelt *next;
};

#define SP_HBLKSZ 1024

struct spHeltBlk
{
    spHeltBlk *next;
    struct spHelt elts[SP_HBLKSZ];
};

struct spHtab
{
    spHtab();
    ~spHtab();

    spMatrixElement *get(int, int);
    void link(spHelt*);
    void stats(unsigned int*, unsigned int*);

private:
    void rehash();

    struct spHelt **entries;
    unsigned int mask;
    unsigned int allocated;
    unsigned int getcalls;
};
#endif


//  MATRIX FRAME CLASS
//
// This class contains all the pointers that support the orthogonal
// linked list that contains the matrix elements.  Also included in
// this class are other numbers and pointers that are used globally by
// the sparse matrix functions and are associated with one particular
// matrix.
//
//  >>> Structure fields:
//
//  AbsThreshold  (spREAL)
//      The absolute magnitude an element must have to be considered as a
//      pivot candidate, except as a last resort.
//
//  AllocatedExtSize  (int)
//      The allocated size of the arrays used to translate external row and
//      column numbers to their internal values.
//
//  AllocatedSize  (int)
//      The currently allocated size of the matrix; the size the matrix can
//      grow to when EXPANDABLE is set true and AllocatedSize is the largest
//      the matrix can get without requiring that the matrix frame be
//      reallocated.
//
//  Complex  (spBOOLEAN)
//      The flag which indicates whether the matrix is complex (true) or
//      real.
//
//  CurrentSize  (int)
//      This number is used during the building of the matrix when the
//      TRANSLATE option is set true.  It indicates the number of internal
//      rows and columns that have elements in them.
//
//  Diag  (spMatrixElement**)
//      Array of pointers that points to the diagonal elements.
//
//  DoCmplxDirect  (spBOOLEAN *)
//      Array of flags, one for each column in matrix.  If a flag is true
//      then corresponding column in a complex matrix should be eliminated
//      in spFactor() using direct addressing (rather than indirect
//      addressing).
//
//  DoRealDirect  (spBOOLEAN *)
//      Array of flags, one for each column in matrix.  If a flag is true
//      then corresponding column in a real matrix should be eliminated
//      in spFactor() using direct addressing (rather than indirect
//      addressing).
//
//  Elements  (int)
//      The number of original elements (total elements minus fill ins)
//      present in matrix.
//
//  Error  (int)
//      The error status of the sparse matrix package.
//
//  ExtSize  (int)
//      The value of the largest external row or column number encountered.
//
//  ExtToIntColMap  (int [])
//      An array that is used to convert external columns number to
//      internal external column numbers.  Present only if TRANSLATE option
//      is set true.
//
//  ExtToIntRowMap  (int [])
//      An array that is used to convert external row numbers to internal
//      external row numbers.  Present only if TRANSLATE option is set true.
//
//  Factored  (spBOOLEAN)
//      Indicates if matrix has been factored.  This flag is set true in
//      spFactor() and spOrderAndFactor() and set false in spCreate()
//      and spClear().
//
//  Fillins  (int)
//      The number of fill-ins created during the factorization the matrix.
//
//  FirstInCol  (spMatrixElement**)
//      Array of pointers that point to the first nonzero element of the
//      column corresponding to the index.
//
//  FirstInRow  (spMatrixElement**)
//      Array of pointers that point to the first nonzero element of the row
//      corresponding to the index.
//
//  ID  (unsigned long int)
//      A constant that provides the sparse data structure with a
//      signature.  When SP_DEBUG is true, all externally available sparse
//      functions check this signature to assure they are operating on a
//      valid matrix.
//
//  Intermediate  (spREAL*)
//      Temporary storage used in the spSolve functons.  Intermediate is an
//      array used during forward and backward substitution.  It is
//      commonly called y when the forward and backward substitution
//      process is denoted Ax = b => Ly = b and Ux = y.
//
//  InternalVectorsAllocated  (spBOOLEAN)
//      A flag that indicates whether the Markowitz vectors and the
//      Intermediate vector have been created.
//      These vectors are created in spcCreateInternalVectors().
//
//  IntToExtColMap  (int [])
//      An array that is used to convert internal column numbers to external
//      external column numbers.
//
//  IntToExtRowMap  (int [])
//      An array that is used to convert internal row numbers to external
//      external row numbers.
//
//  MarkowitzCol  (int [])
//      An array that contains the count of the non-zero elements excluding
//      the pivots for each column.  Used to generate and update
//      MarkowitzProd.
//
//  MarkowitzProd  (long [])
//      The array of the products of the Markowitz row and column counts. 
//      The element with the smallest product is the best pivot to use to
//      maintain sparsity.
//
//  MarkowitzRow  (int [])
//      An array that contains the count of the non-zero elements excluding
//      the pivots for each row. Used to generate and update MarkowitzProd.
//
//  MaxRowCountInLowerTri  (int)
//      The maximum number of off-diagonal element in the rows of L, the
//      lower triangular matrix.  This quantity is used when computing an
//      estimate of the roundoff error in the matrix.
//
//  NeedsOrdering  (spBOOLEAN)
//      This is a flag that signifies that the matrix needs to be ordered
//      or reordered.  NeedsOrdering is set true in spCreate() and
//      spGetElement() or spGetAdmittance() if new elements are added to the
//      matrix after it has been previously factored.  It is set false in
//      spOrderAndFactor().
//
//  NeedsOrdering  (spBOOLEAN)
//      Set true after an OrderAndFactor call if reordering was done.
//
//  NumberOfInterchangesIsOdd  (spBOOLEAN)
//      Flag that indicates the sum of row and column interchange counts is
//      an odd number.  Used when determining the sign of the determinant.
//
//  Partitioned  (spBOOLEAN)
//      This flag indicates that the columns of the matrix have been
//      partitioned into two groups.  Those that will be addressed directly
//      and those that will be addressed indirectly in spFactor().
//
//  PivotsOriginalCol  (int)
//      Column pivot was chosen from.
//
//  PivotsOriginalRow  (int)
//      Row pivot was chosen from.
//
//  PivotSelectionMethod  (char)
//      Character that indicates which pivot search method was successful.
//
//  RelThreshold  (spREAL)
//      The magnitude an element must have relative to others in its row
//      to be considered as a pivot candidate, except as a last resort.
//
//  Reordered  (spBOOLEAN)
//      This flag signifies that the matrix has been reordered.  It
//      is cleared in spCreate(), set in spMNA_Preorder() and
//      spOrderAndFactor() and is used in spPrint().
//
//  ReorderFailed  (spBOOLEAN)
//      Flag set when reordering fails, for spPrint();
//
//  RowsLinked  (spBOOLEAN)
//      A flag that indicates whether the row pointers exist.  The
//      AddByIndex functions do not generate the row pointers, which are
//      needed by some of the other functions, such as spOrderAndFactor()
//      and spScale().  The row pointers are generated in the function
//      LinkRows().
//
//  SingularCol  (int)
//      Normally zero, but if matrix is found to be singular, SingularCol
//      is assigned the external column number of pivot that was zero.
//
//  SingularRow  (int)
//      Normally zero, but if matrix is found to be singular, SingularRow
//      is assigned the external row number of pivot that was zero.
//
//  Singletons  (int)
//      The number of singletons available for pivoting.  Note that if row
//      I and column I both contain singletons, only one of them is
//      counted.
//
//  Size  (int)
//      Number of rows and columns in the matrix.  Does not change as
//      matrix is factored.
//
//  TrashCan  (spMatrixElement)
//      This is a dummy spMatrixElement that is used to by the user to stuff
//      data related to the zero row or column.  In other words, when the
//      user adds an element in row zero or column zero, then the matrix
//      returns a pointer to TrashCan.  In this way the user can have a
//      uniform way data into the matrix independent of whether a component
//      is connected to ground.
//
class  spMatrixFrame
{
public:

    // inlines
    //  SET MATRIX COMPLEX OR REAL
    void spSetReal()
        {
            Complex = 0;
            if (Matrix)
#if SP_OPT_LONG_DBL_SOLVE
                DataAddressChange = Matrix->toReal(LongDoubles);
#else
                DataAddressChange = Matrix->toReal(0);
#endif
        }

    void spSetComplex()
        {
            Complex = 1;
            if (Matrix)
                DataAddressChange = Matrix->toComplex();
        }

#if SP_OPT_LONG_DBL_SOLVE

    // Return true if the matrix is using extended precision.
    //
    spBOOLEAN spLongDoubles()
        {
            return (LongDoubles);
        }

#endif
    // Former macros.
    spBOOLEAN IS_VALID()    { return (Error >= spOKAY && Error < spFATAL); }
    spBOOLEAN IS_FACTORED() { return (Factored && !NeedsOrdering); }

    spBOOLEAN spNeedsOrdering()
        {
            return (NeedsOrdering);
        }

    spBOOLEAN spDidReorder()
        {
            return (DidReorder);
        }


    //  MATRIX SIZE
    //  >>> Arguments:
    //  external  <input>  (spBOOLEAN)
    //      If external is set true, the external size, i.e., the
    //      value of the largest external row or column number
    //      encountered is returned.  Otherwise the true size of the
    //      matrix is returned.  These two sizes may differ if the
    //      TRANSLATE option is set true.
    //
    int spGetSize(spBOOLEAN external)
        {
#if SP_OPT_TRANSLATE
            return (external ? ExtSize : Size);
#else
            (void)external;
            return (Size);
#endif
        }


    //  ELEMENT OR FILL-IN COUNT
    int spFillinCount()         { return (Fillins); }
    int spElementCount()        { return (Elements); }
    spBOOLEAN spDataAddressChange()
        {
            int tmp = DataAddressChange;
            DataAddressChange = 0;
            return (tmp);
        }

    // INTERRUPT CHECKING
    //
    // Register an external function that is called periodically during
    // factoring.  A nonzero return will cause exit.
    //
#if SP_OPT_INTERRUPT
    void spSetInterruptCheckFunc(int(*func)())  { InterruptCallback = func; }
#endif

    // spbuild.cc
    spMatrixFrame(int, int);
    ~spMatrixFrame();
    void     spClear();
    spREAL*  spGetElement(int, int);
#if SP_OPT_QUAD_ELEMENT
    int      spGetAdmittance(int, int, struct spTemplate*);
    int      spGetQuad(int, int, int, int, struct spTemplate*);
    int      spGetOnes(int, int, int, struct spTemplate*);
#endif
    void     spSetMatlabMatrix(spMatlabMatrix*);
    void     spSortElements();
#if SP_OPT_INITIALIZE
    int      spInitialize(spInitializeFunc);
#endif

    // spfactor.cc
    int      spOrderAndFactor(spREAL[], spREAL, spREAL, int);
    int      spFactor();
    void     spPartition(int);

    // spoutput.cc
#if SP_OPT_DOCUMENTATION
#ifdef WRSPICE
    void     spFPrint(int, int, int, FILE*);
    void     spPrint(int, int, int);
#else
    void     spPrint(int, int, int, FILE*);
#endif
    int      spFileMatrix(char*, char*, int, int, int);
#if SP_OPT_COMPLEX && SP_OPT_SEPARATED_COMPLEX_VECTORS
    int      spFileVector(char* , spREAL[], spREAL[]);
#else
    int      spFileVector(char* , spREAL[]);
#endif
    int      spFileStats(char*, char*);
#endif

    // spsolve.cc
#if SP_OPT_COMPLEX && SP_OPT_SEPARATED_COMPLEX_VECTORS
    int      spSolve(spREAL[], spREAL[], spREAL[], spREAL[]);
    int      spSolveTransposed(spREAL[], spREAL[], spREAL[], spREAL[]);
#else
    int      spSolve(spREAL[], spREAL[]);
    int      spSolveTransposed(spREAL[], spREAL[]);
#endif

    // spspice.cc
#ifdef WRSPICE
    void     spSaveForInitialization();
    void     spLoadInitialization();
    void     spNegate();
    double   spLargest();
    double   spSmallest();
    bool     spLoadGmin(int, double, double, bool, bool);
    bool     spCheckNode(int, struct sCKT*);
    void     spGetStat(int*, int*, int*);
    int      spAddCol(int, int);
    int      spZeroCol(int);
    int      spDProd(spREAL*, spREAL*, int*);
#endif

    // sputils.cc
#if SP_OPT_MODIFIED_NODAL
    void     spMNA_Preorder();
#endif
#if SP_OPT_SCALING
    void     spScale(spREAL[], spREAL[]);
#endif
#if SP_OPT_MULTIPLICATION
#if SP_OPT_COMPLEX && SP_OPT_SEPARATED_COMPLEX_VECTORS
    void     spMultiply(spREAL[], spREAL[], spREAL[], spREAL[]);
#else
    void     spMultiply(spREAL[], spREAL[]);
#endif
    void     spConstMult(spREAL);
#if SP_OPT_TRANSPOSE
#if SP_OPT_COMPLEX && SP_OPT_SEPARATED_COMPLEX_VECTORS
    void     spMultTransposed(spREAL[], spREAL[], spREAL[], spREAL[]);
#else
    void     spMultTransposed(spREAL[], spREAL[]);
#endif
#endif
#endif
#if SP_OPT_DETERMINANT
#if SP_OPT_COMPLEX
    void     spDeterminant(int*, spREAL*, spREAL*);
#else
    void     spDeterminant(int*, spREAL*);
#endif
#endif
#if SP_OPT_STRIP
    void     spStripFills();
#endif
#if SP_OPT_TRANSLATE && SP_OPT_DELETE
    void     spDeleteRowAndCol(int, int);
#endif
#if SP_OPT_PSEUDOCONDITION
    spREAL   spPseudoCondition();
#endif
#if SP_OPT_CONDITION
    spREAL   spCondition(spREAL, int*);
    spREAL   spNorm();
#endif
#if SP_OPT_STABILITY
    spREAL   spLargestElement();
    spREAL   spRoundoff(spREAL);
#endif
#if SP_OPT_DOCUMENTATION
    void     spErrorMessage(FILE*, const char*);
#endif
    const char *spErrorMessage(int);
    int      spError();
    void     spWhereSingular(int*, int*);

private:
    // inlines
    //  ELEMENT ALLOCATION
    spMatrixElement *GetElement() { return (ElementAllocator.NewElement()); }
    spMatrixElement *GetFillin()
        {
#if SP_OPT_STRIP
            return (FillinAllocator.NewElement());
#else
            return (ElementAllocator.NewElement());
#endif
        }

    // spbuild.cc
    void EnlargeMatrix(int);
#if SP_OPT_TRANSLATE
    void Translate(int*, int*);
    void ExpandTranslationArrays(int);
#endif
    spMatrixElement *FindElementInCol(spMatrixElement**, int, int, int);
    spMatrixElement *CreateElement(int, int, spMatrixElement**, int);
    void LinkRows();
#if SP_BUILDHASH
    // Add data to the hash table, return true if added, false if
    // already there or null.  Note that the Row and Col in data are
    // actually ignored.
    //
    bool sph_add(int row, int col, spMatrixElement *data)
        {
            if (!data)
                return (false);
            if (!ElementHashTab)
                ElementHashTab = new spHtab;
            else if (ElementHashTab->get(row, col))
                return (false);
            spHelt *h = sph_newhelt(row, col, data);
            ElementHashTab->link(h);
            return (true);
        }

    // Return the element at row,col or null if none exists.
    //
    spMatrixElement *sph_get(int row, int col)
        {
            return (ElementHashTab ? ElementHashTab->get(row, col) : 0);
        }

    // Return the sph_get call count and the allocated size.  The call
    // count is zeroed.
    //
    void sph_stats(unsigned int *pg, unsigned int *pa)
        {
            if (!ElementHashTab) {
                if (pg)
                    *pg = 0;
                if (pa)
                    *pa = 0;
                return;
            }
            ElementHashTab->stats(pg, pa);
        }

    void sph_destroy()
        {
            while (HashElementBlocks) {
                struct spHeltBlk *hx = HashElementBlocks;
                HashElementBlocks = HashElementBlocks->next;
                delete hx;
            }
            delete ElementHashTab;
            ElementHashTab = 0;
        }

    spHelt *sph_newhelt(int, int, spMatrixElement*);
#endif

    // spfactor.cc
#if SP_BITFIELD
    void ba_setbit(int, int, int);
    bool ba_getbit(int, int);
    spMatrixElement *ba_left(int, int);
    spMatrixElement *ba_above(int, int);
    void ba_setup();
    void ba_destroy();
#endif
#if SP_OPT_COMPLEX
    int  FactorComplexMatrix();
#endif
    void CreateInternalVectors();
    void CountMarkowitz(spREAL*, int);
    void MarkowitzProducts(int);
    spMatrixElement *SearchForPivot(int, int);
    spMatrixElement *SearchForSingleton(int);
#if SP_OPT_DIAGONAL_PIVOTING
    spMatrixElement *QuicklySearchDiagonal(int);
    spMatrixElement *SearchDiagonal(int);
#endif
    spMatrixElement *SearchEntireMatrix(int);
    spREAL FindLargestInCol(spMatrixElement*);
    spREAL FindBiggestInColExclude(spMatrixElement*, int);
    void ExchangeRowsAndCols(spMatrixElement*, int);
    void RowExchange(int, int);
    void ColExchange(int, int);
    void ExchangeColElements(int, spMatrixElement*, int, spMatrixElement*, int);
    void ExchangeRowElements(int, spMatrixElement*, int, spMatrixElement*, int);
    void RealRowColElimination(spMatrixElement*);
#if SP_OPT_COMPLEX
    void ComplexRowColElimination(spMatrixElement*);
#endif
    void UpdateMarkowitzNumbers(spMatrixElement*);
    spMatrixElement *CreateFillin(int, int);
    int  MatrixIsSingular(int);
    int  ZeroPivot(int);
    void WriteStatus(int);

    // spsolve.cc
#if SP_OPT_COMPLEX
#if SP_OPT_SEPARATED_COMPLEX_VECTORS
    void SolveComplexMatrix(spREAL*, spREAL*, spREAL*, spREAL*);
#if SP_OPT_TRANSPOSE
    void SolveComplexTransposedMatrix(spREAL*, spREAL*, spREAL*, spREAL*);
#endif
#else
    void SolveComplexMatrix(spREAL*, spREAL*);
#if SP_OPT_TRANSPOSE
    void SolveComplexTransposedMatrix(spREAL*, spREAL*);
#endif
#endif
#endif

    // sputils.cc
#if SP_OPT_COMPLEX
    int CountTwins(int, spMatrixElement**, spMatrixElement**);
    void SwapCols(spMatrixElement*, spMatrixElement*);
#if SP_OPT_SCALING
    void ScaleComplexMatrix(spREAL*, spREAL*);
#endif
#if SP_OPT_MULTIPLICATION
#if SP_OPT_SEPARATED_COMPLEX_VECTORS
    void ComplexMatrixMultiply(spREAL*, spREAL*, spREAL*, spREAL*);
#if SP_OPT_TRANSPOSE
    void ComplexTransposedMatrixMultiply(spREAL*, spREAL*, spREAL*, spREAL*);
#endif
#else
    void ComplexMatrixMultiply(spREAL*, spREAL*);
#if SP_OPT_TRANSPOSE
    void ComplexTransposedMatrixMultiply(spREAL*, spREAL*);
#endif
#endif
#endif
#if SP_OPT_CONDITION
    spREAL ComplexCondition(spREAL);
#endif
#if SP_OPT_LONG_DBL_SOLVE
    spREAL LongDoubleCondition(spREAL);
#endif
#endif

    spMatrixElement             **FirstInCol;
    spMatrixElement             **FirstInRow;
    spMatrixElement             **Diag;
    int*                        IntToExtColMap;
    int*                        IntToExtRowMap;
    int*                        ExtToIntColMap;
    int*                        ExtToIntRowMap;
    spMatrixElement             *SortedElements;
    int*                        MarkowitzRow;
    int*                        MarkowitzCol;
    long*                       MarkowitzProd;
    spREAL*                     Intermediate;
    spBOOLEAN*                  DoCmplxDirect;
    spBOOLEAN*                  DoRealDirect;
    spMatlabMatrix              *Matrix;
#if SP_BUILDHASH
    struct spHtab               *ElementHashTab;
    struct spHeltBlk            *HashElementBlocks;
    unsigned int                HashElementCount;
#endif
#if SP_BITFIELD
    unsigned int                **BitField;
#endif

    int                         Size;
    int                         CurrentSize;
    int                         ExtSize;
    int                         AllocatedSize;
    int                         AllocatedExtSize;
    int                         Elements;
    int                         Fillins;
    int                         Singletons;
    int                         MaxRowCountInLowerTri;
    int                         Error;

    spBOOLEAN                   Complex;
    spBOOLEAN                   Factored;
    spBOOLEAN                   Partitioned;
    spBOOLEAN                   Reordered;
    spBOOLEAN                   ReorderFailed;
    spBOOLEAN                   InternalVectorsAllocated;
    spBOOLEAN                   NeedsOrdering;
    spBOOLEAN                   DidReorder;
    spBOOLEAN                   NumberOfInterchangesIsOdd;
    spBOOLEAN                   RowsLinked;
    spBOOLEAN                   DataAddressChange;
    spBOOLEAN                   NoKLU;
    spBOOLEAN                   NoSort;
    spBOOLEAN                   Trace;
#if SP_OPT_LONG_DBL_SOLVE
    spBOOLEAN                   LongDoubles;
#endif

    int                         PartitionMode;
    int                         PivotsOriginalCol;
    int                         PivotsOriginalRow;
    int                         PivotSelectionMethod;
    int                         SingularCol;
    int                         SingularRow;

    spREAL                      RelThreshold;
    spREAL                      AbsThreshold;
    spMatrixElement             TrashCan;
#if SP_OPT_INTERRUPT
    int                         (*InterruptCallback)();
#endif

    spMatrixElementAllocator    ElementAllocator;
    spMatrixElementAllocator    FillinAllocator;
};


#if SP_OPT_INITIALIZE
extern void  spInstallInitInfo(spREAL*, void*);
extern void  *spGetInitInfo(spREAL*);
#endif

#endif

