
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
 $Id: spbuild.cc,v 2.24 2015/06/20 03:16:38 stevew Exp $
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
#include "spmacros.h"
#include "spmatrix.h"
#include <math.h>

#ifdef WRSPICE
#include "ttyio.h"
#define PRINTF TTY.err_printf
#else
#define PRINTF printf
#endif

//  MATRIX BUILD MODULE
//
//  Author:                     Advising professor:
//     Kenneth S. Kundert           Alberto Sangiovanni-Vincentelli
//     UC Berkeley
//
//  This file contains the functions associated with clearing, loading and
//  preprocessing the matrix for the sparse matrix functions.
//
//  >>> Public functions contained in this file:
//
//  spMatrixFrame Constructor
//  spMatrixFrame Destructor
//  spClear
//  spGetElement
//  spGetAdmittance
//  spGetQuad
//  spGetOnes
//  spInitialize
//
//  >>> Private functions contained in this file:
//
//  EnlargeMatrix
//  Translate
//  ExpandTranslationArrays
//  FindElementInCol
//  CreateElement
//  LinkRows


//  MATRIX CONSTRUCTOR
//
//  Allocates and initializes the data structures associated with a matrix.
//  One should check that spError returns spOK after calling.
//
//  >>> Arguments:
//
//  size  <input>  (int)
//      Size of matrix or estimate of size of matrix if matrix is
//      SP_OPT_EXPANDABLE.
//
//  flags  <input>  (int)
//
//      Zero of more of the following flags, OR'ed together.
//
//      SP_COMPLEX
//        Type of matrix.  If SP_COMPLEX is not given then the matrix is
//        real, otherwise the matrix will be complex.  Note that if the
//        functions are not set up to handle the type of matrix requested,
//        then Error will be set to spPanic on return.
//
//      SP_NO_SORT
//        By default, Sparse will copy the elements into a sorted array
//        after ordering/factoring, since this improves speed.  This is
//        an S. Whiteley extension.  Giving this flag wil suppress this,
//        providing stock Sparse behavior.
//
//      SP_NO_KLU
//        When the KLU plug-in is available, Sparse will call on KLU for
//        ordering/factoring/solving.  KLU is typically much faster.  This
//        flag prevents use of KLU.  The KLU plug-in is an S. Whiteley
//        extension.
//
//      SP_EXT_PREC
//        Use long doubles as the element type in the real matrix.  Yet
//        another S. Whiteley extension.  All input/output remains as
//        spREALs, though the matrix elements can be set directly.
//
//      SP_TRACE
//        Another S. Whiteley extension, enables some tracing via printf
//        statements, for debugging purposes.
//
//  >>> Local variables:
//
//  allocatedSize  (int)
//      The size of the matrix being allocated.
//
//  >>> Possible errors:
//
//  spPANIC
//
spMatrixFrame::spMatrixFrame(int size, int flags)
{
    Error = spOKAY;
    Complex = ((flags & SP_COMPLEX) ? YES : NO);

    // Test for valid size.
    if (size < 0 OR (size == 0 AND NOT SP_OPT_EXPANDABLE))
        Error = spPANIC;

    // Test for valid type.
#if NOT SP_OPT_COMPLEX
    if (Complex)
        Error = spPANIC;
#endif
#if NOT SP_OPT_REAL
    if (NOT Complex)
        Error = spPANIC;
#endif

    // Initialize
    FirstInCol                      = 0;
    FirstInRow                      = 0;
    Diag                            = 0;
    IntToExtColMap                  = 0;
    IntToExtRowMap                  = 0;
    ExtToIntColMap                  = 0;
    ExtToIntRowMap                  = 0;
    SortedElements                  = 0;
    MarkowitzRow                    = 0;
    MarkowitzCol                    = 0;
    MarkowitzProd                   = 0;
    Intermediate                    = 0;
    DoCmplxDirect                   = 0;
    DoRealDirect                    = 0;
    Matrix                          = 0;
#if SP_BUILDHASH
    ElementHashTab                  = 0;
    HashElementBlocks               = 0;
    HashElementCount                = 0;
#endif
#if SP_BITFIELD
    BitField                        = 0;
#endif

    Size                            = size;
    CurrentSize                     = 0;
    ExtSize                         = size;
    AllocatedSize                   = 0;
    AllocatedExtSize                = 0;
    Elements                        = 0;
    Fillins                         = 0;
    Singletons                      = 0;
    MaxRowCountInLowerTri           = 0;

    Factored                        = NO;
    Partitioned                     = NO;
    Reordered                       = NO;
    ReorderFailed                   = NO;
    InternalVectorsAllocated        = NO;
    NeedsOrdering                   = YES;
    DidReorder                      = NO;
    NumberOfInterchangesIsOdd       = NO;
    RowsLinked                      = NO;
    DataAddressChange               = NO;
    NoKLU                           = ((flags & SP_NO_KLU) ? YES : NO);
    NoSort                          = ((flags & SP_NO_SORT) ? YES : NO);
    Trace                           = ((flags & SP_TRACE) ? YES : NO);
#if SP_OPT_LONG_DBL_SOLVE
    LongDoubles                     = ((flags & SP_EXT_PREC) ? YES : NO);
#endif

    PartitionMode                   = spDEFAULT_PARTITION;
    PivotsOriginalCol               = 0;
    PivotsOriginalRow               = 0;
    PivotSelectionMethod            = 0;
    SingularCol                     = 0;
    SingularRow                     = 0;

    RelThreshold                    = DEFAULT_THRESHOLD;
    AbsThreshold                    = 0.0;
    ID                              = SPARSE_ID;
#if SP_OPT_INTERRUPT
    InterruptCallback               = 0;
#endif

    if (Error != spOKAY)
        return;

    // Create Matrix.
    int allocatedSize = SPMAX(size, MINIMUM_ALLOCATED_SIZE);
    unsigned sizePlusOne = (unsigned)(allocatedSize + 1);

    AllocatedSize                   = allocatedSize;
    AllocatedExtSize                = allocatedSize;

    // Allocate space in memory for FirstInCol pointer vector.
    FirstInCol = new spMatrixElement*[sizePlusOne];
    memset(FirstInCol, 0, sizePlusOne*sizeof(spMatrixElement*));

    // Allocate space in memory for FirstInRow pointer vector.
    FirstInRow = new spMatrixElement*[sizePlusOne];
    memset(FirstInRow, 0, sizePlusOne*sizeof(spMatrixElement*));

    // Allocate space in memory for Diag pointer vector.
    Diag = new spMatrixElement*[sizePlusOne];
    memset(Diag, 0, sizePlusOne*sizeof(spMatrixElement*));

    // Allocate space and initialize IntToExt vectors.
    IntToExtColMap = new int[sizePlusOne];
    IntToExtRowMap = new int[sizePlusOne];
    for (int i = 0; i <= allocatedSize; i++) {
        IntToExtRowMap[i] = i;
        IntToExtColMap[i] = i;
    }

#if SP_OPT_TRANSLATE
    // Allocate space and initialize ExtToInt vectors.
    ExtToIntColMap = new int[sizePlusOne];
    ExtToIntRowMap = new int[sizePlusOne];
    for (int i = 1; i <= allocatedSize; i++) {
        ExtToIntColMap[i] = -1;
        ExtToIntRowMap[i] = -1;
    }
    ExtToIntColMap[0] = 0;
    ExtToIntRowMap[0] = 0;
#endif
    if (Trace) {
#if SP_OPT_LONG_DBL_SOLVE
        PRINTF("new matrix: cplx=%d useExtraPrec=%d noKLU=%d noSort=%d\n",
            Complex, LongDoubles, NoKLU, NoSort);
#else
        PRINTF("new matrix: cplx=%d noKLU=%d noSort=%d\n",
            Complex, NoKLU, NoSort);
#endif
    }
}


//  MATRIX DESTRUCTOR
//
//  Deallocates pointers and elements of Matrix.
//
//  >>> Local variables:
//
//  listPtr  (AllocationRecord*)
//      Pointer into the linked list of pointers to allocated data
//      structures.  Points to pointer to structure to be freed.
//
//  nextListPtr  (AllocationRecord*)
//      Pointer into the linked list of pointers to allocated data
//      structures.  Points to the next pointer to structure to be freed. 
//      This is needed because the data structure to be freed could include
//      the current node in the allocation list.
//
spMatrixFrame::~spMatrixFrame()
{
    ASSERT(IS_SPARSE(this));

    // Deallocate the vectors that are located in the matrix frame.
    delete [] FirstInCol;
    delete [] FirstInRow;
    delete [] Diag;
    delete [] IntToExtColMap;
    delete [] IntToExtRowMap;
    delete [] ExtToIntColMap;
    delete [] ExtToIntRowMap;
    delete [] SortedElements;
    delete [] MarkowitzRow;
    delete [] MarkowitzCol;
    delete [] MarkowitzProd;
    delete [] Intermediate;
    delete [] DoCmplxDirect;
    delete [] DoRealDirect;
    delete Matrix;
#if SP_BUILDHASH
    sph_destroy();
#endif
#if SP_BITFIELD
    ba_destroy();
#endif
}


//  CLEAR MATRIX
//
// Sets every element of the matrix to zero and clears the error flag.
//
//  >>> Local variables:
//
//  pElement  (spMatrixElement*)
//     A pointer to the element being cleared.
//
void
spMatrixFrame::spClear()
{
    ASSERT(IS_SPARSE(this));

    if (Matrix)
        Matrix->clear();
    else {
        // Clear matrix
        for (int I = Size; I > 0; I--) {
            spMatrixElement *pElement = FirstInCol[I];
            while (pElement != 0) {
                pElement->Real = 0.0;
#if SP_OPT_COMPLEX
                pElement->Imag = 0.0;
#endif
                pElement = pElement->NextInCol;
            }
        }
    }

    // Empty the trash
    TrashCan.Real = 0.0;
#if SP_OPT_COMPLEX
    TrashCan.Imag = 0.0;
#endif

    Error = spOKAY;
    Factored = NO;
    SingularCol = 0;
    SingularRow = 0;
}


//  SINGLE ELEMENT ADDITION TO MATRIX BY INDEX
//
// Finds element [row,col] and returns a pointer to it.  If element is
// not found then it is created and spliced into matrix.  This
// function is only to be used before spMNA_Preorder(), spFactor() or
// spOrderAndFactor(), unless the element is known to exist.  Returns
// a pointer to the Real portion of a spMatrixElement.  This pointer is
// later used by spADD_xxx_ELEMENT to directly access element.
//
//  >>> Returns:
//
// Returns a pointer to the element.  This pointer is then used to
// directly access the element during successive builds.
//
//  >>> Arguments:
//
//  row  <input>  (int)
//      Row index for element.  Must be in the range of [0..Size] unless
//      the options SP_OPT_EXPANDABLE or SP_OPT_TRANSLATE are used. 
//      Elements placed in row zero are discarded.  In no case may Row be
//      less than zero.
//
//  col  <input>  (int)
//      Column index for element.  Must be in the range of [0..Size] unless
//      the options SP_OPT_EXPANDABLE or SP_OPT_TRANSLATE are used. 
//      Elements placed in column zero are discarded.  In no case may Col
//      be less than zero.
//
//  >>> Local variables:
//
//  pElement  (spREAL *)
//     Pointer to the element.
//
spREAL *
spMatrixFrame::spGetElement(int inrow, int incol)
{
    ASSERT(IS_SPARSE(this) AND inrow >= 0 AND incol >= 0);

    if ((inrow == 0) OR (incol == 0))
        return (&TrashCan.Real);

#if NOT SP_OPT_TRANSLATE
    ASSERT(NeedsOrdering);
#endif

    spMatrixElement *pElement;
#if SP_BUILDHASH
    if (!Matrix) {
        // The hashing doesn't work (seg faults) with KLU.
        pElement = sph_get(inrow, incol);
        if (pElement)
            return ((spREAL*)pElement);
    }
#endif

    int row = inrow;
    int col = incol;
#if SP_OPT_TRANSLATE
    Translate(&row, &col);
#endif

    if (Matrix)
        return (Matrix->find(row-1, col-1));

#if NOT SP_OPT_TRANSLATE
#if NOT SP_OPT_EXPANDABLE
    ASSERT(row <= Size AND col <= Size);
#endif

#if SP_OPT_EXPANDABLE
    // Re-size Matrix if necessary
    if ((row > Size) OR (col > Size))
        EnlargeMatrix(SPMAX(row, col));
#endif
#endif

    if ((row == col) AND (Diag[row] != 0))
        pElement = Diag[row];
    else
        pElement = FindElementInCol(&FirstInCol[col], row, col, YES);
#if SP_BUILDHASH
    sph_add(inrow, incol, pElement);
#endif
    return ((spREAL*)pElement);
}


#if SP_OPT_QUAD_ELEMENT

//  ADDITION OF ADMITTANCE TO MATRIX BY INDEX
//
// Performs same function as spGetElement except rather than one
// element, all four matrix elements for a floating component are
// added.  This function also works if component is grounded. 
// Positive elements are placed at [node1,node2] and [node2,node1]. 
// This function is only to be used before spMNA_Preorder(),
// spFactor() or spOrderAndFactor().
//
//  >>> Returns:
//
//  Error code.
//
//  >>> Arguments:
//
//  node1  <input>  (int)
//      Row and column indices for elements.  Must be in the range of
//      [0..Size] unless the options SP_OPT_EXPANDABLE or SP_OPT_TRANSLATE
//      are used.  Node zero is the ground node.  In no case may node1 be
//      less than zero.
//
//  node2  <input>  (int)
//      Row and column indices for elements.  Must be in the range of
//      [0..Size] unless the options SP_OPT_EXPANDABLE or SP_OPT_TRANSLATE
//      are used.  Node zero is the ground node.  In no case may node2 be
//      less than zero.
//
//  tmpl  <output>  (struct spTemplate *)
//     Collection of pointers to four elements that are later used to directly
//     address elements.  User must supply the template, this function will
//     fill it.
//
//  Error is not cleared in this function.
//
int
spMatrixFrame::spGetAdmittance(int node1, int node2, spTemplate *tmpl)
{
    tmpl->Element1 = spGetElement(node1, node1);
    tmpl->Element2 = spGetElement(node2, node2);
    tmpl->Element3Negated = spGetElement(node2, node1);
    tmpl->Element4Negated = spGetElement(node1, node2);
    if (node1 == 0)
        SWAP(spREAL*, tmpl->Element1, tmpl->Element2);
    return (spOKAY);
}


//  ADDITION OF FOUR ELEMENTS TO MATRIX BY INDEX
//
// Similar to spGetAdmittance, except that spGetAdmittance only
// handles 2-terminal components, whereas spGetQuad handles simple
// 4-terminals as well.  These 4-terminals are simply generalized
// 2-terminals with the option of having the sense terminals different
// from the source and sink terminals.  spGetQuad adds four elements
// to the matrix.  Positive elements occur at row1,col1 row2,col2
// while negative elements occur at row1,col2 and row2,col1.  The
// function works fine if any of the rows and columns are zero.  This
// function is only to be used before spMNA_Preorder(), spFactor() or
// spOrderAndFactor() unless SP_OPT_TRANSLATE is set true.
//
//  >>> Returns:
//
//  Error code.
//
//  >>> Arguments:
//  row1  <input>  (int)
//      First row index for elements.  Must be in the range of [0..Size]
//      unless the options SP_OPT_EXPANDABLE or SP_OPT_TRANSLATE are used. 
//      Zero is the ground row.  In no case may row1 be less than zero.
//
//  row2  <input>  (int)
//      Second row index for elements.  Must be in the range of [0..Size]
//      unless the options SP_OPT_EXPANDABLE or SP_OPT_TRANSLATE are used. 
//      Zero is the ground row.  In no case may row2 be less than zero.
//
//  col1  <input>  (int)
//      First column index for elements.  Must be in the range of [0..Size]
//      unless the options SP_OPT_EXPANDABLE or SP_OPT_TRANSLATE are used. 
//      Zero is the ground column.  In no case may col1 be less than zero.
//
//  col2  <input>  (int)
//      Second column index for elements.  Must be in the range of
//      [0..Size] unless the options SP_OPT_EXPANDABLE or SP_OPT_TRANSLATE
//      are used.  Zero is the ground column.  In no case may col2 be less
//      than zero.
//
//  tmpl  <output>  (struct spTemplate *)
//     Collection of pointers to four elements that are later used to directly
//     address elements.  User must supply the template, this function will
//     fill it.
//  Real  <input>  (spREAL)
//     Real data to be added to elements.
//  Imag  <input>  (spREAL)
//     Imag data to be added to elements.  If matrix is real, this argument
//     may be deleted.
//
//  Error is not cleared in this function.
//
int
spMatrixFrame::spGetQuad(int row1, int row2, int col1, int col2,
    spTemplate *tmpl)
{
    tmpl->Element1 = spGetElement(row1, col1);
    tmpl->Element2 = spGetElement(row2, col2);
    tmpl->Element3Negated = spGetElement(row2, col1);
    tmpl->Element4Negated = spGetElement(row1, col2);
    if (tmpl->Element1 == &TrashCan.Real)
        SWAP(spREAL *, tmpl->Element1, tmpl->Element2);
    return (spOKAY);
}


//  ADDITION OF FOUR STRUCTURAL ONES TO MATRIX BY INDEX
//
// Performs similar function to spGetQuad() except this function is
// meant for components that do not have an admittance representation.
//
// The following stamp is used:
//         Pos  Neg  Eqn
//  Pos  [  .    .    1  ]
//  Neg  [  .    .   -1  ]
//  Eqn  [  1   -1    .  ]
//
//  >>> Returns:
//
//  Error code.
//
//  >>> Arguments:
//
//  pos  <input>  (int)
//      See stamp above.  Must be in the range of [0..Size] unless the
//      options SP_OPT_EXPANDABLE or SP_OPT_TRANSLATE are used.  Zero is
//      the ground row.  In no case may pos be less than zero.
//
//  neg  <input>  (int)
//      See stamp above.  Must be in the range of [0..Size] unless the
//      options SP_OPT_EXPANDABLE or SP_OPT_TRANSLATE are used.  Zero is
//      the ground row.  In no case may neg be less than zero.
//
//  eqn  <input>  (int)
//      See stamp above.  Must be in the range of [0..Size] unless the
//      options SP_OPT_EXPANDABLE or SP_OPT_TRANSLATE are used.  Zero is
//      the ground row.  In no case may eqn be less than zero.
//
//  tmpl  <output>  (struct spTemplate *)
//     Collection of pointers to four elements that are later used to directly
//     address elements.  User must supply the template, this function will
//     fill it.
//
//  Error is not cleared in this function.
//
int
spMatrixFrame::spGetOnes(int pos, int neg, int eqn, spTemplate *tmpl)
{
    tmpl->Element4Negated = spGetElement(neg, eqn);
    tmpl->Element3Negated = spGetElement(eqn, neg);
    tmpl->Element2 = spGetElement(pos, eqn);
    tmpl->Element1 = spGetElement(eqn, pos);
    spADD_REAL_QUAD(*tmpl, 1.0);
    return (spOKAY);
}

#endif  // SP_OPT_QUAD_ELEMENT


//   SORT MATRIX (SRW)
//
// Copy all matrix elements and fill-ins into a new, contiguous array
// of spMatrixElement structs, sorted ascending in column-major order. 
// This will localize memory access for improved speed.
//
// This is done after spOrderAndFactor, and it invalidates all data
// pointers in user code, so they must be re-obtained.  It is safe to
// use spGetElement for this.  One can check if the matrix needs this
// by calling spDataAddressChange.
//
void
spMatrixFrame::spSortElements()
{
    if (Matrix || NoSort)
        return;
#if SP_BUILDHASH
    sph_destroy();
#endif

    // Create the new array of elements.
    spMatrixElement *sorted = new spMatrixElement[Elements];

    // Clear the row list heads.
    memset(FirstInRow, 0, (Size+1)*sizeof(spMatrixElement*));

    spMatrixElement *eptr = sorted;
    for (int i = 1; i <= Size; i++) {
        Diag[i] = 0;
        // Copy the elements in each successive column, in order,
        // to the new array.
        spMatrixElement *pElement = FirstInCol[i];
        bool firstElem = true;
        while (pElement) {
            eptr->Real = pElement->Real;
#if SP_OPT_COMPLEX
            eptr->Imag = pElement->Imag;
#endif
#ifdef WRSPICE
            eptr->Init = pElement->Init;
#if SP_OPT_LONG_DBL_SOLVE
            eptr->Init2 = pElement->Init2;
#endif
#endif
            eptr->Row = pElement->Row;
            eptr->Col = pElement->Col;
#if SP_OPT_INITIALIZE
            eptr->pInitInfo = pElement->pInitInfo;
#endif
            if (eptr->Row == eptr->Col)
                Diag[i] = eptr;
#if SP_BUILDHASH
            // This is imperitive if SP_BITFIELD set, as the mapping
            // is broken unless all elements are hashed.  It avoids
            // the search traversals in spGetElement so is a good
            // thing to do in any case.
            sph_add(IntToExtRowMap[eptr->Row], IntToExtColMap[eptr->Col],
                eptr);
#endif

            // Establish the pointer links.  The column link is trivial,
            // the row links take more work.
            eptr->NextInCol = pElement->NextInCol ? eptr+1 : 0;
            eptr->NextInRow = 0;

            int row = pElement->Row;
            if (FirstInRow[row] == 0 || (eptr->Col < FirstInRow[row]->Col)) {
                eptr->NextInRow = FirstInRow[row];
                FirstInRow[row] = eptr;
            }
            else {
                spMatrixElement *px = FirstInRow[row];
                while (px) {
                    if (eptr->Col > px->Col && (px->NextInRow == 0 ||
                            eptr->Col < px->NextInRow->Col)) {
                        eptr->NextInRow = px->NextInRow;
                        px->NextInRow = eptr;
                        break;
                    }
                    px = px->NextInRow;
                }
            }

            if (firstElem) {
                firstElem = false;
                FirstInCol[i] = eptr;
            }
            eptr++;
            pElement = pElement->NextInCol;
        }
    }

    // We can now destroy all previous elements and fill-ins.
    delete [] SortedElements;
    ElementAllocator.Clear();
    FillinAllocator.Clear();

    SortedElements = sorted;
    DataAddressChange = YES;
}


//   SET OVERRIDE MATRIX (SRW)
//
// This allows a class derived from spMatlabMatrix to be initialized and
// linked into this.  Through its methods, the new object will
// override the factoring/solving functions.  Thus, we use Sparse as a
// front-end to gather the matrix element data, but another package,
// such as KLU, to perform the actual matrix solving.
//
void
spMatrixFrame::spSetMatlabMatrix(spMatlabMatrix *mm)
{
    delete Matrix;

    int elcnt = 0;
    for (int i = 1; i <= Size; i++) {
        spMatrixElement *pElement = FirstInCol[i];
        mm->Ap[i-1] = elcnt;
        while (pElement) {
            mm->Ai[elcnt] = pElement->Row - 1;
#if SP_OPT_COMPLEX
            if (Complex) {
                mm->Ax[2*elcnt] = pElement->Real;
                mm->Ax[2*elcnt + 1] = pElement->Imag;
                elcnt++;
                pElement = pElement->NextInCol;
                continue;
            }
#endif
#if SP_OPT_LONG_DBL_SOLVE
            if (LongDoubles) {
                LDBL(&mm->Ax[2*elcnt]) = LDBL(pElement);
                elcnt++;
                pElement = pElement->NextInCol;
                continue;
            }
#endif
            mm->Ax[elcnt] = pElement->Real;
            elcnt++;
            pElement = pElement->NextInCol;
        }
    }
    mm->Ap[Size] = elcnt;
    ASSERT(elcnt == Elements)
    Matrix = mm;
}


#if SP_OPT_INITIALIZE

//   INITIALIZE MATRIX
//
// With the SP_OPT_INITIALIZE compiler option (see spconfig.h) set true,
// Sparse allows the user to keep initialization information with each
// structurally nonzero matrix element.  Each element has a pointer
// that is set and used by the user.  The user can set this pointer
// using spInstallInitInfo and may be read using spGetInitInfo.  Both
// may be used only after the element exists.  The function
// spInitialize() is a user customizable way to initialize the matrix. 
// Passed to this function is a function pointer.  spInitialize()
// sweeps through every element in the matrix and checks the pInitInfo
// pointer (the user supplied pointer).  If the pInitInfo is 0, which
// is true unless the user changes it (almost always true for
// fill-ins), then the element is zeroed.  Otherwise, the function
// pointer is called and passed the pInitInfo pointer as well as the
// element pointer and the external row and column numbers.  If the
// user sets the value of each element, then spInitialize() replaces
// spClear().
//
// The user function is expected to return a nonzero integer if there
// is a fatal error and zero otherwise.  Upon encountering a nonzero
// return code, spInitialize() terminates and returns the error code.
//
//   >>> Possible Errors:
//   Returns nonzero if error, zero otherwise.
//
void
spInstallInitInfo(spREAL *pElement, void *pInitInfo)
{
    ASSERT(pElement != 0);

    ((spMatrixElement*)pElement)->pInitInfo = pInitInfo;
}


void *
spGetInitInfo(spREAL *pElement)
{
    ASSERT(pElement != 0);

    return ((void *)((spMatrixElement*)pElement)->pInitInfo);
}


int
spMatrixFrame::spInitialize(spInitializeFunc pInit)
{
    ASSERT(IS_SPARSE(this));

    // Initialize the matrix
    for (int j = Size; j > 0; j--) {
        spMatrixElement *pElement = FirstInCol[j];
        int col = IntToExtColMap[j];
        while (pElement != 0) {
            if (pElement->pInitInfo == 0) {
                pElement->Real = 0.0;
#if SP_OPT_COMPLEX
                pElement->Imag = 0.0;
#endif
            }
            else {
                int error = (*pInit)((spREAL *)pElement,
                    pElement->pInitInfo, IntToExtRowMap[pElement->Row], col);
                if (error) {
                    Error = spFATAL;
                    return (error);
                }

            }
            pElement = pElement->NextInCol;
        }
    }

// Empty the trash
    TrashCan.Real = 0.0;
#if SP_OPT_COMPLEX
    TrashCan.Imag = 0.0;
#endif

    Error = spOKAY;
    Factored = NO;
    SingularCol = 0;
    SingularRow = 0;
    return (0);
}

#endif  // SP_OPT_INITIALIZE


//  ENLARGE MATRIX
//  Private function
//
// Increases the size of the matrix.
//
//  >>> Arguments:
//
//  newSize  <input>  (int)
//     The new size of the matrix.
//
//  >>> Local variables:
//
//  oldAllocatedSize  (int)
//     The allocated size of the matrix before it is expanded.
//
void
spMatrixFrame::EnlargeMatrix(int newSize)
{
    int oldAllocatedSize = AllocatedSize;

    Size = newSize;
    if (newSize <= oldAllocatedSize)
        return;

    // Expand the matrix frame.
    newSize = SPMAX(newSize, (int)(EXPANSION_FACTOR * oldAllocatedSize));
    AllocatedSize = newSize;

    oldAllocatedSize++;

    int *tmpi = new int[newSize+1];
    for (int i = 0; i < oldAllocatedSize; i++)
        tmpi[i] = IntToExtColMap[i];
    delete [] IntToExtColMap;
    IntToExtColMap = tmpi;

    tmpi = new int[newSize+1];
    for (int i = 0; i < oldAllocatedSize; i++)
        tmpi[i] = IntToExtRowMap[i];
    delete [] IntToExtRowMap;
    IntToExtRowMap = tmpi;

    spMatrixElement **tmpe = new spMatrixElement*[newSize+1];
    for (int i = 0; i < oldAllocatedSize; i++)
        tmpe[i] = Diag[i];
    delete [] Diag;
    Diag = tmpe;

    tmpe = new spMatrixElement*[newSize+1];
    for (int i = 0; i < oldAllocatedSize; i++)
        tmpe[i] = FirstInCol[i];
    delete [] FirstInCol;
    FirstInCol = tmpe;

    tmpe = new spMatrixElement*[newSize+1];
    for (int i = 0; i < oldAllocatedSize; i++)
        tmpe[i] = FirstInRow[i];
    delete [] FirstInRow;
    FirstInRow = tmpe;

    // Destroy the Markowitz and Intermediate vectors, they will be
    // recreated in spOrderAndFactor().

    delete [] MarkowitzRow;
    delete [] MarkowitzCol;
    delete [] MarkowitzProd;
    delete [] DoRealDirect;
    delete [] DoCmplxDirect;
    delete [] Intermediate;
    InternalVectorsAllocated = NO;

    // Initialize the new portion of the vectors.
    for (int i = oldAllocatedSize; i <= newSize; i++) {
        IntToExtColMap[i] = i;
        IntToExtRowMap[i] = i;
        Diag[i] = 0;
        FirstInRow[i] = 0;
        FirstInCol[i] = 0;
    }
}


#if SP_OPT_TRANSLATE

//  TRANSLATE EXTERNAL INDICES TO INTERNAL
//  Private function
//
// Convert external row and column numbers to internal row and column
// numbers.  Also updates Ext/Int maps.
//
//  >>> Arguments:
//
//  row  <input/output>  (int *)
//     Upon entry Row is either a external row number or an external node
//     number.  Upon return, the internal equivalent is supplied.
//
//  col  <input/output>  (int *)
//      Upon entry Column is either a external column number or an external
//      node number.  Upon return, the internal equivalent is supplied.
//
//  >>> Local variables:
//
//  extCol  (int)
//     Temporary variable used to hold the external column or node number
//     during the external to internal column number translation.
//
//  extRow  (int)
//     Temporary variable used to hold the external row or node number during
//     the external to internal row number translation.
//
//  intCol  (int)
//     Temporary variable used to hold the internal column or node number
//     during the external to internal column number translation.
//
//  intRow  (int)
//     Temporary variable used to hold the internal row or node number during
//     the external to internal row number translation.
//
void
spMatrixFrame::Translate(int *row, int *col)
{
    int extRow = *row;
    int extCol = *col;

    // Expand translation arrays if necessary.
    if ((extRow > AllocatedExtSize) OR (extCol > AllocatedExtSize))
        ExpandTranslationArrays(SPMAX(extRow, extCol));

    // Set ExtSize if necessary.
    if ((extRow > ExtSize) OR (extCol > ExtSize))
        ExtSize = SPMAX(extRow, extCol);

    // Translate external row or node number to internal row or node number.
    int intRow;
    if ((intRow = ExtToIntRowMap[extRow]) == -1) {
        ExtToIntRowMap[extRow] = ++CurrentSize;
        ExtToIntColMap[extRow] = CurrentSize;
        intRow = CurrentSize;

#if NOT SP_OPT_EXPANDABLE
        ASSERT(intRow <= Size);
#endif

#if SP_OPT_EXPANDABLE
        // Re-size Matrix if necessary.
        if (intRow > Size)
            EnlargeMatrix(intRow);
#endif

        IntToExtRowMap[intRow] = extRow;
        IntToExtColMap[intRow] = extRow;
    }

    // Translate external column or node number to internal column or node
    // number.
    int intCol;
    if ((intCol = ExtToIntColMap[extCol]) == -1) {
        ExtToIntRowMap[extCol] = ++CurrentSize;
        ExtToIntColMap[extCol] = CurrentSize;
        intCol = CurrentSize;

#if NOT SP_OPT_EXPANDABLE
        ASSERT(intCol <= Size);
#endif

#if SP_OPT_EXPANDABLE
        // Re-size Matrix if necessary.
        if (intCol > Size)
            EnlargeMatrix(intCol);
#endif

        IntToExtRowMap[intCol] = extCol;
        IntToExtColMap[intCol] = extCol;
    }

    *row = intRow;
    *col = intCol;
}


//  EXPAND TRANSLATION ARRAYS
//  Private function
//
// Increases the size arrays that are used to translate external to
// internal row and column numbers.
//
//  >>> Arguments:
//
//  newSize  <input>  (int)
//     The new size of the translation arrays.
//
//  >>> Local variables:
//
//  oldAllocatedSize  (int)
//     The allocated size of the translation arrays before being expanded.
//
void
spMatrixFrame::ExpandTranslationArrays(int newSize)
{
    int oldAllocatedSize = AllocatedExtSize;

    ExtSize = newSize;
    if (newSize <= oldAllocatedSize)
        return;

    // Expand the translation arrays ExtToIntRowMap and ExtToIntColMap.
    newSize = SPMAX(newSize, (int)(EXPANSION_FACTOR * oldAllocatedSize));
    AllocatedExtSize = newSize;

    oldAllocatedSize++;

    int *tmp = new int[newSize+1];
    for (int i = 0; i < oldAllocatedSize; i++)
        tmp[i] = ExtToIntRowMap[i];
    delete [] ExtToIntRowMap;
    ExtToIntRowMap = tmp;

    tmp = new int[newSize+1];
    for (int i = 0; i < oldAllocatedSize; i++)
        tmp[i] = ExtToIntColMap[i];
    delete [] ExtToIntColMap;
    ExtToIntColMap = tmp;

    // Initialize the new portion of the vectors
    for (int i = oldAllocatedSize; i <= newSize; i++) {
        ExtToIntRowMap[i] = -1;
        ExtToIntColMap[i] = -1;
    }
}

#endif  // SP_OPT_TRANSLATE


//  FIND ELEMENT BY SEARCHING COLUMN
//  Private function
//
// Searches column starting at element specified at ptrAddr and finds
// element in row.  If element does not exists, it is created.  The
// pointer to the element is returned.
//
//  >>> Returned:
//
//  A pointer to the desired element:
//
//  >>> Arguments:
//
//  lastAddr  <input-output>  (spMatrixElement **)
//      Address of pointer that initially points to the element in col at
//      which the search is started.  The pointer in this location may be
//      changed if a fill-in is required in and adjacent element.  For this
//      reason it is important that lastAddr be the address of a FirstInCol
//      or a NextInCol rather than a temporary variable.
//
//  row  <input>  (int)
//      Row being searched for.
//
//  col  (int)
//      Column being searched.
//
//  createIfMissing  <input>  (spBOOLEAN)
//      Indicates what to do if element is not found, create one or return
//      a 0 pointer.
//
//  >>> Local variables:
//
//  pElement  (spMatrixElement*)
//      Pointer used to search through matrix.
//
spMatrixElement*
spMatrixFrame::FindElementInCol(spMatrixElement **lastAddr, int row, int col,
    spBOOLEAN createIfMissing)
{
    // Search for element.
    spMatrixElement *pElement = *lastAddr;
    while (pElement != 0) {
        if (pElement->Row < row) {
            // Have not reached element yet.
            lastAddr = &pElement->NextInCol;
            pElement = pElement->NextInCol;
        }
        else if (pElement->Row == row) {
            // Reached element.
            return (pElement);
        }
        else
            break;
    }

    // Element does not exist and must be created.
    if (createIfMissing)
        return (CreateElement(row, col, lastAddr, NO));
    else
        return (0);
}


//  CREATE AND SPLICE ELEMENT INTO MATRIX
//  Private function
//
// This function is used to create new matrix elements and splice them
// into the matrix.
//
//  >>> Returned:
//
//  A pointer to the element that was created is returned.
//
//  >>> Arguments:
//
//  row  <input>  (int)
//      Row index for element.
//
//  col  <input>  (int)
//      Column index for element.
//
//  lastAddr  <input-output>  (spMatrixElement**)
//      This contains the address of the pointer to the element just above
//      the one being created.  It is used to speed the search and it is
//      updated with address of the created element.
//
//  fillin  <input>  (spBOOLEAN)
//      Flag that indicates if created element is to be a fill-in.
//
//  >>> Local variables:
//
//  pElement  (spMatrixElement*)
//      Pointer to an element in the matrix. It is used to refer to the newly
//      created element and to restring the pointers of the element's row and
//      column.
//
//  pLastElement  (spMatrixElement*)
//      Pointer to the element in the matrix that was just previously pointed
//      to by pElement. It is used to restring the pointers of the element's
//      row and column.
//
//  pCreatedElement  (spMatrixElement*)
//      Pointer to the desired element, the one that was just created.
//
spMatrixElement*
spMatrixFrame::CreateElement(int row, int col, spMatrixElement **lastAddr,
    spBOOLEAN fillin)
{
    spMatrixElement *pCreatedElement;
    if (RowsLinked) {
        // Row pointers cannot be ignored.
        spMatrixElement *pElement;
        if (fillin) {
            pElement = GetFillin();
            Fillins++;
        }
        else {
            pElement = GetElement();
            NeedsOrdering = YES;
        }
        if (pElement == 0)
            return (0);

        // If element is on diagonal, store pointer in Diag.
        if (row == col)
            Diag[row] = pElement;

        // Initialize Element
        pCreatedElement = pElement;
        pElement->Row = row;
        pElement->Col = col;

        // Splice element into column
        pElement->NextInCol = *lastAddr;
        *lastAddr = pElement;

        // Search row for proper element position
#if SP_BITFIELD
        spMatrixElement *pLastElement = ba_left(row, col);
        pElement = 0;
#else
        pElement = FirstInRow[row];
        spMatrixElement *pLastElement = 0;
        while (pElement != 0) {
            // Search for element row position
            if (pElement->Col < col) {
                // Have not reached desired element
                pLastElement = pElement;
                pElement = pElement->NextInRow;
            }
            else pElement = 0;
        }
#endif

        // Splice element into row
        pElement = pCreatedElement;
        if (pLastElement == 0) {
            // Element is first in row
            pElement->NextInRow = FirstInRow[row];
            FirstInRow[row] = pElement;
        }
        else {
            // Element is not first in row
            pElement->NextInRow = pLastElement->NextInRow;
            pLastElement->NextInRow = pElement;
        }
    }
    else {
        // Matrix has not been factored yet.  Thus get element rather than
        // fill-in.  Also, row pointers can be ignored.

        // Allocate memory for Element
        spMatrixElement *pElement = GetElement();
        if (pElement == 0)
            return (0);

        // If element is on diagonal, store pointer in Diag.
        if (row == col)
            Diag[row] = pElement;

        // Initialize Element
        pCreatedElement = pElement;
        pElement->Row = row;
#if SP_OPT_DEBUG
        pElement->Col = col;
#endif

        // Splice element into column
        pElement->NextInCol = *lastAddr;
        *lastAddr = pElement;
    }

    Elements++;
    return (pCreatedElement);
}


//  LINK ROWS
//  Private function
//
// This function is used to generate the row links.  The
// spGetElement() function do not create row links, which are needed
// by the spFactor() function.
//
//  >>> Local variables:
//
//  pElement  (spMatrixElement*)
//      Pointer to an element in the matrix.
//
//  firstInRowEntry  (spMatrixElement **)
//      A pointer into the FirstInRow array.  Points to the FirstInRow entry
//      currently being operated upon.
//
//  firstInRowArray  (spMatrixElement**)
//      A pointer to the FirstInRow array.  Same as FirstInRow but
//      resides in a register and requires less indirection so is faster to
//      use.
//
//  col  (int)
//      Column currently being operated upon.
//
void
spMatrixFrame::LinkRows()
{
    spMatrixElement **firstInRowArray = FirstInRow;
    for (int col = Size; col >= 1; col--) {
        // Generate row links for the elements in the col'th column
        spMatrixElement *pElement = FirstInCol[col];

        while (pElement != 0) {
            pElement->Col = col;
            spMatrixElement **firstInRowEntry = &firstInRowArray[pElement->Row];
            pElement->NextInRow = *firstInRowEntry;
            *firstInRowEntry = pElement;
            pElement = pElement->NextInCol;
        }
    }
    RowsLinked = YES;
}


#if SP_BUILDHASH

#define ST_MAX_DENS     5
#define ST_START_MASK   31

#if defined(__GNUC__) && (defined(i386) || defined(__x86_64__))
#define rol_bits(x, k) ({ unsigned int t; \
    asm("roll %1,%0" : "=g" (t) : "cI" (k), "0" (x)); t; })
#else
#define rol_bits(x, k) (((x)<<(k)) | ((x)>>(32-(k))))
#endif

namespace {
    // Hashing function for 4/8 byte integers, adapted from
    // lookup3.c, by Bob Jenkins, May 2006, Public Domain.
    //
    unsigned int number_hash(unsigned int x, unsigned int y,
        unsigned int hashmask)
    {
        unsigned int a, b, c; // Assumes 32-bit int.

        /*** for 8-byte ints
        if (sizeof(unsigned long) == 8) {
            union { unsigned long l; unsigned int i[2]; } u;
            a = b = c = 0xdeadbeef + 8;
            u.l = n;
            b += u.i[1];
            a += u.i[0];
        }
        ***/
        /*
        a = b = c = 0xdeadbeef + 4;
        a += n;
        */
        a = b = c = 0xdeadbeef + 8;
        b += x;
        a += y;

        c ^= b; c -= rol_bits(b, 14);
        a ^= c; a -= rol_bits(c, 11);
        b ^= a; b -= rol_bits(a, 25);
        c ^= b; c -= rol_bits(b, 16);
        a ^= c; a -= rol_bits(c, 4);
        b ^= a; b -= rol_bits(a, 14);
        c ^= b; c -= rol_bits(b, 24);

        return (c & hashmask);
    }
}


spHtab::spHtab()
{
    mask = ST_START_MASK;
    entries = new spHelt*[mask + 1];
    memset(entries, 0, (mask+1)*sizeof(spHelt*));
    allocated = 0;
    getcalls = 0;
}


spHtab::~spHtab()
{
    delete [] entries;
}


spMatrixElement *
spHtab::get(int row, int col)
{
    {
        spHtab *ht = this;
        if (!ht)
            return (0);
    }
    if (allocated) {
        getcalls++;
        unsigned int i = number_hash(row, col, mask);
        for (spHelt *h = entries[i]; h; h = h->next) {
            if (h->row == row && h->col == col)
                return (h->eptr);
        }
    }
    return (0);
}


void
spHtab::link(spHelt *h)
{
    unsigned int i = number_hash(h->row, h->col, mask);
    h->next = entries[i];
    entries[i] = h;
    allocated++;
    if (allocated/(mask + 1) > ST_MAX_DENS)
        rehash();
}


void
spHtab::stats(unsigned int *pg, unsigned int *pa)
{
    {
        spHtab *ht = this;
        if (!ht) {
            if (pg)
                *pg = 0;
            if (pa)
                *pa = 0;
            return;
        }
    }
    if (pg)
        *pg = getcalls;
    if (pa)
        *pa = allocated;
    getcalls = 0;
}


// Private function to increase the hash width by one bit.
//
void
spHtab::rehash()
{
    unsigned int i, oldmask = mask;
    mask = (oldmask << 1) | 1;
    spHelt **oldent = entries;
    entries = new spHelt*[mask + 1];
    for (i = 0; i <= mask; i++)
        entries[i] = 0;
    for (i = 0; i <= oldmask; i++) {
        spHelt *h, *hn;
        for (h = oldent[i]; h; h = hn) {
            hn = h->next;
            unsigned int j = number_hash(h->row, h->col, mask);
            h->next = entries[j];
            entries[j] = h;
        }
    }
    delete [] oldent;
}


struct spHelt *
spMatrixFrame::sph_newhelt(int row, int col, spMatrixElement *data)
{
    if (!HashElementBlocks || HashElementCount == SP_HBLKSZ) {
        spHeltBlk *hb = new spHeltBlk;
        hb->next = HashElementBlocks;
        HashElementBlocks = hb;
        HashElementCount = 0;
    }
    spHelt *h = HashElementBlocks->elts + HashElementCount++;
    h->next = 0;
    h->row = row;
    h->col = col;
    h->eptr = data;
    return (h);
};

#endif

