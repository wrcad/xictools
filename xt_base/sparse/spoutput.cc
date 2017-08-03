
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
#include "config.h"
#include "spconfig.h"
#include "spmatrix.h"
#include "spmacros.h"
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif
#ifndef DBL_MAX
#define DBL_MAX     1.79769313486231e+308
#endif

#ifdef WRSPICE
#include "ttyio.h"
#endif


//  MATRIX OUTPUT MODULE
//
//  Author:                     Advisor:
//      Kenneth S. Kundert          Alberto Sangiovanni-Vincentelli
//      UC Berkeley
//
// This file contains the output-to-file and output-to-screen
// functions for the matrix package.
//
//  >>> Public functions contained in this file:
//  spPrint
//  spFileMatrix
//  spFileVector
//  spFileStats


#if SP_OPT_DOCUMENTATION

//  PRINT MATRIX
//
// Formats and send the matrix to standard output.  Some elementary
// statistics are also output.  The matrix is output in a format that
// is readable by people.
//
//  >>> Arguments:
//
//  printReordered  <input>  (int)
//      Indicates whether the matrix should be printed out in its original
//      form, as input by the user, or whether it should be printed in its
//      reordered form, as used by the matrix functions.  A zero indicates
//      that the matrix should be printed as inputed, a one indicates that
//      it should be printed reordered.
//
//  data  <input>  (int)
//      Boolean flag that when false indicates that output should be
//      compressed such that only the existence of an element should be
//      indicated rather than giving the actual value.  Thus 11 times as
//      many can be printed on a row.  A zero signifies that the matrix
//      should be printed compressed. A one indicates that the matrix
//      should be printed in all its glory.
//
//  header  <input>  (int)
//      Flag indicating that extra information should be given, such as row
//      and column numbers.
//
//  >>> Local variables:
//
//  col  (int)
//      Column being printed.
//
//  elementCount  (int)
//      Variable used to count the number of nonzero elements in the matrix.
//
//  largestElement  (spREAL)
//      The magnitude of the largest element in the matrix.
//
//  largestDiag  (spREAL)
//      The magnitude of the largest diagonal in the matrix.
//
//  magnitude  (spREAL)
//      The absolute value of the matrix element being printed.
//
//  printOrdToIntColMap  (int [])
//      A translation array that maps the order that columns will be
//      printed in (if not PrintReordered) to the internal column numbers.
//
//  printOrdToIntRowMap  (int [])
//      A translation array that maps the order that rows will be
//      printed in (if not PrintReordered) to the internal row numbers.
//
//  pElement  (spMatrixElement*)
//      Pointer to the element in the matrix that is to be printed.
//
//  pImagElements  (spMatrixElement* [ ])
//      Array of pointers to elements in the matrix.  These pointers point
//      to the elements whose real values have just been printed.  They are
//      used to quickly access those same elements so their imaginary values
//      can be printed.
//
//  row  (int)
//      Row being printed.
//
//  smallestDiag  (spREAL)
//      The magnitude of the smallest diagonal in the matrix.
//
//  smallestElement  (spREAL)
//      The magnitude of the smallest element in the matrix excluding zero
//      elements.
//
//  startCol  (int)
//      The column number of the first column to be printed in the group of
//      columns currently being printed.
//
//  stopCol  (int)
//      The column number of the last column to be printed in the group of
//      columns currently being printed.
//
//  top  (int)
//      The largest expected external row or column number.
//
#ifdef WRSPICE
void
spMatrixFrame::spFPrint(int printReordered, int data, int header, FILE *fp)
#else
void
spMatrixFrame::spPrint(int printReordered, int data, int header, FILE *fp)
#endif
{
    double *pImagElements[PRINTER_WIDTH/10+1];
    double smallestDiag = 0.0, smallestElement = 0.0;
    double largestElement = 0.0, largestDiag = 0.0;
    int startCol = 1;
    int elementCount = 0;

    if (!fp)
        fp = stdout;

    // Create a packed external to internal row and column translation
    // array.
#if SP_OPT_TRANSLATE
    int top = AllocatedExtSize;
#else
    int top = AllocatedSize;
#endif
    int *printOrdToIntRowMap = new int[top + 1];
    int *printOrdToIntColMap = new int[top + 1];
    memset(printOrdToIntRowMap, 0, (top+1)*sizeof(int));
    memset(printOrdToIntColMap, 0, (top+1)*sizeof(int));
    for (int i = 1; i <= Size; i++) {
        printOrdToIntRowMap[ IntToExtRowMap[i] ] = i;
        printOrdToIntColMap[ IntToExtColMap[i] ] = i;
    }

    // Pack the arrays.
    for (int j = 1, i = 1; i <= top; i++) {
        if (printOrdToIntRowMap[i] != 0)
            printOrdToIntRowMap[ j++ ] = printOrdToIntRowMap[ i ];
    }
    for (int j = 1, i = 1; i <= top; i++) {
        if (printOrdToIntColMap[i] != 0)
            printOrdToIntColMap[ j++ ] = printOrdToIntColMap[ i ];
    }

    // Print header.
    if (header) {
        fprintf(fp, "MATRIX SUMMARY\n\n");
        fprintf(fp, "Size of matrix = %1d x %1d.\n", Size, Size);
        if (Reordered AND printReordered)
            fprintf(fp, "Matrix has been reordered.\n");
        if (ReorderFailed)
            fprintf(fp, "Matrix is partially reordered, reordering failed.\n");
        putc('\n', fp);

        if (Factored)
            fprintf(fp, "Matrix after factorization:\n");
        else
            fprintf(fp, "Matrix before factorization:\n");

        smallestElement = DBL_MAX;
        smallestDiag = smallestElement;
    }

    // Determine how many columns to use.
    int columns = PRINTER_WIDTH;
    if (header)
        columns -= 5;
    if (data)
        columns = (columns+1) / 10;

    // Print matrix by printing groups of complete columns until all the
    // columns are printed.

    int j = 0;
    while (j <= Size) {
        // Calculate index of last column to printed in this group.
        int stopCol = startCol + columns - 1;
        if (stopCol > Size)
            stopCol = Size;

        // Label the columns.
        if (header) {
            if (data) {
                fprintf(fp, "    ");
                for (int i = startCol; i <= stopCol; i++) {
                    int col;
                    if (printReordered)
                        col = i;
                    else
                        col = printOrdToIntColMap[i];
                    fprintf(fp, " %9d", IntToExtColMap[ col ]);
                }
                fprintf(fp, "\n\n");
            }
            else {
                if (printReordered)
                    fprintf(fp, "Columns %1d to %1d.\n", startCol, stopCol);
                else {
                    fprintf(fp, "Columns %1d to %1d.\n",
                        IntToExtColMap[ printOrdToIntColMap[startCol] ],
                        IntToExtColMap[ printOrdToIntColMap[stopCol] ]);
                }
            }
        }

        // Print every row ...
        for (int i = 1; i <= Size; i++) {
            int row;
            if (printReordered)
                row = i;
            else
                row = printOrdToIntRowMap[i];

            if (header) {
                if (printReordered AND NOT data)
                    fprintf(fp, "%4d", i);
                else
                    fprintf(fp, "%4d", IntToExtRowMap[ row ]);
                if (NOT data)
                    putc(' ', fp);
            }

            // ... in each column of the group
            for (j = startCol; j <= stopCol; j++) {
                int col;
                if (printReordered)
                    col = j;
                else
                    col = printOrdToIntColMap[j];

                spREAL *ptr = 0;
                if (Matrix)
                    ptr = Matrix->find(row-1, col-1);
                else {
                    spMatrixElement *pElement = FirstInCol[col];
                    while (pElement != 0 AND pElement->Row != row)
                        pElement = pElement->NextInCol;
                    if (pElement)
                        ptr = &(pElement->Real);
                }
                if (ptr != 0) {
                    // Case where element exists.
                    if (data) {
                        pImagElements[j - startCol] =  ptr;
                        if (LongDoubles)
#ifdef WIN32
                            fprintf(fp, " %9.3g", (double)LDBL(ptr));
#else
                            fprintf(fp, " %9.3Lg", LDBL(ptr));
#endif
                        else
                            fprintf(fp, " %9.3g", *ptr);
                    }
                    else
                        putc('x', fp);

                    // Update status variables
#if SP_OPT_COMPLEX
                    double magnitude;
                    if (Complex)
                        magnitude = ABS(*ptr) + ABS(*(ptr+1));
                    else if (LongDoubles) {
                        magnitude = (double)*(long double*)ptr;
                        if (magnitude < 0.0)
                            magnitude = -magnitude;
                    }
                    else
                        magnitude = ABS(*ptr);
#else
                    double magnitude = ABS(*ptr);
#endif
                    if (magnitude > largestElement)
                        largestElement = magnitude;
                    if ((magnitude < smallestElement) AND (magnitude != 0.0))
                        smallestElement = magnitude;
                    elementCount++;
                }
                else {
                    // Case where element is structurally zero.
                    if (data) {
                        pImagElements[j - startCol] = 0;
                        fprintf(fp, "       ...");
                    }
                    else
                        putc('.', fp);
                }
            }
            putc('\n', fp);

#if SP_OPT_COMPLEX
            if (Complex AND data) {
                fprintf(fp, "    ");
                for (j = startCol; j <= stopCol; j++) {
                    if (pImagElements[j - startCol] != 0)
                        fprintf(fp, " %8.2gj", *pImagElements[j-startCol]);
                    else
                        fprintf(fp, "          ");
                }
                putc('\n', fp);
            }
#endif
        }

        // Calculate index of first column in next group.
        startCol = stopCol;
        startCol++;
        putc('\n', fp);
    }
    if (header) {
        fprintf(fp, "\nLargest element in matrix = %-1.4g.\n", largestElement);
        fprintf(fp, "Smallest element in matrix = %-1.4g.\n", smallestElement);

        if (!Matrix) {
            // Search for largest and smallest diagonal values.
            for (int i = 1; i <= Size; i++) {
                if (Diag[i] != 0) {
#if SP_OPT_COMPLEX
                    spREAL magnitude;
                    if (Complex)
                        magnitude = E_MAG(Diag[i]);
                    else if (LongDoubles)
                        magnitude = LABS(LDBL(Diag[i]));
                    else
                        magnitude = ABS(Diag[i]->Real);
#else
                    double magnitude = ABS(Diag[i]->Real);
#endif
                    if (magnitude > largestDiag)
                        largestDiag = magnitude;
                    if (magnitude < smallestDiag)
                        smallestDiag = magnitude;
                }
            }

            // Print the largest and smallest diagonal values.
            if (Factored) {
                fprintf(fp, "\nLargest diagonal element = %-1.4g.\n",
                    largestDiag);
                fprintf(fp, "Smallest diagonal element = %-1.4g.\n",
                    smallestDiag);
            }
            else {
                fprintf(fp, "\nLargest pivot element = %-1.4g.\n", largestDiag);
                fprintf(fp, "Smallest pivot element = %-1.4g.\n", smallestDiag);
            }

            // Calculate and print sparsity and number of fill-ins created.
            fprintf(fp, "\nDensity = %2.2f%%.\n",
                ((double)(elementCount * 100)) / ((double)(Size * Size)));
            if (NOT NeedsOrdering)
                fprintf(fp, "Number of fill-ins = %1d.\n", Fillins);
        }
    }
    putc('\n', fp);
    fflush(fp);

    delete [] printOrdToIntColMap;
    delete [] printOrdToIntRowMap;
}


#ifdef WRSPICE

// This is a special version of spPrint that connects to the WRspice
// TTY system.
//
void
spMatrixFrame::spPrint(int printReordered, int data, int header)
{
    double *pImagElements[PRINTER_WIDTH/10+1];
    double smallestDiag = 0.0, smallestElement = 0.0;
    double largestElement = 0.0, largestDiag = 0.0;
    int startCol = 1;
    int elementCount = 0;

    // Create a packed external to internal row and column translation
    // array.
#if SP_OPT_TRANSLATE
    int top = AllocatedExtSize;
#else
    int top = AllocatedSize;
#endif
    int *printOrdToIntRowMap = new int[top + 1];
    int *printOrdToIntColMap = new int[top + 1];
    memset(printOrdToIntRowMap, 0, (top+1)*sizeof(int));
    memset(printOrdToIntColMap, 0, (top+1)*sizeof(int));
    for (int i = 1; i <= Size; i++) {
        printOrdToIntRowMap[ IntToExtRowMap[i] ] = i;
        printOrdToIntColMap[ IntToExtColMap[i] ] = i;
    }

    // Pack the arrays.
    for (int j = 1, i = 1; i <= top; i++) {
        if (printOrdToIntRowMap[i] != 0)
            printOrdToIntRowMap[ j++ ] = printOrdToIntRowMap[ i ];
    }
    for (int j = 1, i = 1; i <= top; i++) {
        if (printOrdToIntColMap[i] != 0)
            printOrdToIntColMap[ j++ ] = printOrdToIntColMap[ i ];
    }

    // Print header.
    if (header) {
        TTY.printf("MATRIX SUMMARY\n\n");
        TTY.printf("Size of matrix = %1d x %1d.\n", Size, Size);
        if (Reordered AND printReordered)
            TTY.printf("Matrix has been reordered.\n");
        if (ReorderFailed)
            TTY.printf("Matrix is partially reordered, reordering failed.\n");
        TTY.printf("\n");

        if (Factored)
            TTY.printf("Matrix after factorization:\n");
        else
            TTY.printf("Matrix before factorization:\n");

        smallestElement = DBL_MAX;
        smallestDiag = smallestElement;
    }

    // Determine how many columns to use.
    int columns = PRINTER_WIDTH;
    if (header)
        columns -= 5;
    if (data)
        columns = (columns+1) / 10;

    // Print matrix by printing groups of complete columns until all the
    // columns are printed.

    int j = 0;
    while (j <= Size) {
        // Calculate index of last column to printed in this group.
        int stopCol = startCol + columns - 1;
        if (stopCol > Size)
            stopCol = Size;

        // Label the columns.
        if (header) {
            if (data) {
                TTY.printf("    ");
                for (int i = startCol; i <= stopCol; i++) {
                    int col;
                    if (printReordered)
                        col = i;
                    else
                        col = printOrdToIntColMap[i];
                    TTY.printf(" %9d", IntToExtColMap[ col ]);
                }
                TTY.printf("\n\n");
            }
            else {
                if (printReordered)
                    TTY.printf("Columns %1d to %1d.\n", startCol, stopCol);
                else {
                    TTY.printf("Columns %1d to %1d.\n",
                        IntToExtColMap[ printOrdToIntColMap[startCol] ],
                        IntToExtColMap[ printOrdToIntColMap[stopCol] ]);
                }
            }
        }

        // Print every row ...
        for (int i = 1; i <= Size; i++) {
            int row;
            if (printReordered)
                row = i;
            else
                row = printOrdToIntRowMap[i];

            if (header) {
                if (printReordered AND NOT data)
                    TTY.printf("%4d", i);
                else
                    TTY.printf("%4d", IntToExtRowMap[ row ]);
                if (NOT data)
                    TTY.printf(" ");
            }

            // ... in each column of the group
            for (j = startCol; j <= stopCol; j++) {
                int col;
                if (printReordered)
                    col = j;
                else
                    col = printOrdToIntColMap[j];

                spREAL *ptr = 0;
                if (Matrix)
                    ptr = Matrix->find(row-1, col-1);
                else {
                    spMatrixElement *pElement = FirstInCol[col];
                    while (pElement != 0 AND pElement->Row != row)
                        pElement = pElement->NextInCol;
                    if (pElement)
                        ptr = &(pElement->Real);
                }
                if (ptr != 0) {
                    // Case where element exists.
                    if (data) {
                        pImagElements[j - startCol] =  ptr;
                        if (LongDoubles)
                            TTY.printf(" %9.3Lg", LDBL(ptr));
                        else
                            TTY.printf(" %9.3g", *ptr);
                    }
                    else
                        TTY.printf("x");

                    // Update status variables
#if SP_OPT_COMPLEX
                    double magnitude;
                    if (Complex)
                        magnitude = ABS(*ptr) + ABS(*(ptr+1));
                    else if (LongDoubles) {
                        magnitude = (double)*(long double*)ptr;
                        if (magnitude < 0.0)
                            magnitude = -magnitude;
                    }
                    else
                        magnitude = ABS(*ptr);
#else
                    double magnitude = ABS(*ptr);
#endif
                    if (magnitude > largestElement)
                        largestElement = magnitude;
                    if ((magnitude < smallestElement) AND (magnitude != 0.0))
                        smallestElement = magnitude;
                    elementCount++;
                }
                else {
                    // Case where element is structurally zero.
                    if (data) {
                        pImagElements[j - startCol] = 0;
                        TTY.printf("       ...");
                    }
                    else
                        TTY.printf(".");
                }
            }
            TTY.printf("\n");

#if SP_OPT_COMPLEX
            if (Complex AND data) {
                TTY.printf("    ");
                for (j = startCol; j <= stopCol; j++) {
                    if (pImagElements[j - startCol] != 0)
                        TTY.printf(" %8.2gj", *pImagElements[j-startCol]);
                    else
                        TTY.printf("          ");
                }
                TTY.printf("\n");
            }
#endif
        }

        // Calculate index of first column in next group.
        startCol = stopCol;
        startCol++;
        TTY.printf("\n");
    }
    if (header) {
        TTY.printf("\nLargest element in matrix = %-1.4g.\n", largestElement);
        TTY.printf("Smallest element in matrix = %-1.4g.\n", smallestElement);

        if (!Matrix) {
            // Search for largest and smallest diagonal values.
            for (int i = 1; i <= Size; i++) {
                if (Diag[i] != 0) {
#if SP_OPT_COMPLEX
                    spREAL magnitude;
                    if (Complex)
                        magnitude = E_MAG(Diag[i]);
                    else if (LongDoubles)
                        magnitude = LABS(LDBL(Diag[i]));
                    else
                        magnitude = ABS(Diag[i]->Real);
#else
                    double magnitude = ABS(Diag[i]->Real);
#endif
                    if (magnitude > largestDiag)
                        largestDiag = magnitude;
                    if (magnitude < smallestDiag)
                        smallestDiag = magnitude;
                }
            }

            // Print the largest and smallest diagonal values.
            if (Factored) {
                TTY.printf("\nLargest diagonal element = %-1.4g.\n",
                    largestDiag);
                TTY.printf("Smallest diagonal element = %-1.4g.\n",
                    smallestDiag);
            }
            else {
                TTY.printf("\nLargest pivot element = %-1.4g.\n", largestDiag);
                TTY.printf("Smallest pivot element = %-1.4g.\n", smallestDiag);
            }

            // Calculate and print sparsity and number of fill-ins created.
            TTY.printf("\nDensity = %2.2f%%.\n",
                ((double)(elementCount * 100)) / ((double)(Size * Size)));
            if (NOT NeedsOrdering)
                TTY.printf("Number of fill-ins = %1d.\n", Fillins);
        }
    }
    TTY.printf("\n");

    delete [] printOrdToIntColMap;
    delete [] printOrdToIntRowMap;
}

#endif


//  OUTPUT MATRIX TO FILE
//
// Writes matrix to file in format suitable to be read back in by the
// matrix test program.
//
//  >>> Returns:
//
// One is returned if function was successful, otherwise zero is
// returned.  The calling function can query errno (the system global
// error variable) as to the reason why this function failed.
//
//  >>> Arguments:
//
//  file  <input>  (char *)
//      Name of file into which matrix is to be written.
//
//  label  <input>  (char *)
//      String that is transferred to file and is used as a label.
//
//  reordered  <input> (spBOOLEAN)
//      Specifies whether matrix should be output in reordered form,
//      or in original order.
//
//  data  <input> (spBOOLEAN)
//      Indicates that the element values should be output along with
//      the indices for each element.  This parameter must be true if
//      matrix is to be read by the sparse test program.
//
//  header  <input> (spBOOLEAN)
//      Indicates that header is desired.  This parameter must be true if
//      matrix is to be read by the sparse test program.
//
//  >>> Local variables:
//
//  col  (int)
//      The original column number of the element being output.
//
//  pElement  (spMatrixElement*)
//      Pointer to an element in the matrix.
//
//  pMatrixFile  (FILE *)
//      File pointer to the matrix file.
//
//  row  (int)
//      The original row number of the element being output.
//
int
spMatrixFrame::spFileMatrix(char *file, char *label, int reordered, int data,
    int header)
{
    // Open file matrix file in write mode.
    FILE *pMatrixFile;
    if ((pMatrixFile = fopen(file, "w")) == 0)
        return 0;

    // Output header.
    if (header) {
        if (Factored AND data) {
            int err = fprintf(pMatrixFile,
                "Warning : The following matrix is factored in to LU form.\n");
            if (err < 0)
                return 0;
        }
        if (fprintf(pMatrixFile, "%s\n", label) < 0)
            return 0;
        int err = fprintf(pMatrixFile, "%d\t%s\n", Size,
            (Complex ? "complex" : "real"));
        if (err < 0)
            return 0;
    }

    // Output matrix.
    if (NOT data) {
        for (int i = 1; i <= Size; i++) {
            spMatrixElement *pElement = FirstInCol[i];
            while (pElement != 0) {
                int row, col;
                if (reordered) {
                    row = pElement->Row;
                    col = i;
                }
                else {
                    row = IntToExtRowMap[pElement->Row];
                    col = IntToExtColMap[i];
                }
                pElement = pElement->NextInCol;
                if (fprintf(pMatrixFile, "%d\t%d\n", row, col) < 0)
                    return 0;
            }
        }
        // Output terminator, a line of zeros.
        if (header)
            if (fprintf(pMatrixFile, "0\t0\n") < 0)
                return 0;
    }

#if SP_OPT_COMPLEX
    if (data AND Complex) {
        for (int i = 1; i <= Size; i++) {
            spMatrixElement *pElement = FirstInCol[i];
            while (pElement != 0) {
                int row, col;
                if (reordered) {
                    row = pElement->Row;
                    col = i;
                }
                else {
                    row = IntToExtRowMap[pElement->Row];
                    col = IntToExtColMap[i];
                }
                int err = fprintf(pMatrixFile,"%d\t%d\t%-.15g\t%-.15g\n",
                    row, col, pElement->Real, pElement->Imag);
                if (err < 0)
                    return 0;
                pElement = pElement->NextInCol;
            }
        }
        // Output terminator, a line of zeros.
        if (header)
            if (fprintf(pMatrixFile,"0\t0\t0.0\t0.0\n") < 0)
                return 0;

    }
#endif

#if SP_OPT_REAL
    if (data AND NOT Complex) {
        for (int i = 1; i <= Size; i++) {
            spMatrixElement *pElement = FirstInCol[i];
            while (pElement != 0) {
                int row = IntToExtRowMap[pElement->Row];
                int col = IntToExtColMap[i];
                int err;
                if (LongDoubles)
#ifdef WIN32
                    err = fprintf(pMatrixFile,"%d\t%d\t%-.15g\n",
                        row, col, (double)LDBL(pElement));
#else
                    err = fprintf(pMatrixFile,"%d\t%d\t%-.15Lg\n",
                        row, col, LDBL(pElement));
#endif
                else
                    err = fprintf(pMatrixFile,"%d\t%d\t%-.15g\n",
                        row, col, pElement->Real);
                if (err < 0)
                    return 0;
                pElement = pElement->NextInCol;
            }
        }
        // Output terminator, a line of zeros.
        if (header)
            if (fprintf(pMatrixFile,"0\t0\t0.0\n") < 0)
                return 0;

    }
#endif // SP_OPT_REAL

    // Close file
    if (fclose(pMatrixFile) < 0)
        return 0;
    return 1;
}


//  OUTPUT SOURCE VECTOR TO FILE
//
// Writes vector to file in format suitable to be read back in by the
// matrix test program.  This function should be executed after the
// function spFileMatrix.
//
//  >>> Returns:
//
// One is returned if function was successful, otherwise zero is
// returned.  The calling function can query errno (the system global
// error variable) as to the reason why this function failed.
//
//  >>> Arguments:
//
//  file  <input>  (char *)
//      Name of file into which matrix is to be written.
//
//  rhs  <input>  (spREAL *)
//      Right-hand side vector. This is only the real portion if
//      SP_OPT_SEPARATED_COMPLEX_VECTORS is true.
//
//  irhs  <input>  (spREAL *)
//      Right-hand side vector, imaginary portion.  Not necessary if matrix
//      is real or if SP_OPT_SEPARATED_COMPLEX_VECTORS is set false.
//
//  >>> Local variables:
//
//  pMatrixFile  (FILE *)
//      File pointer to the matrix file.
//
//  >>> Obscure Macros
//
//  IMAG_RHS_P
//      Replaces itself with `, spREAL* irhs' if the options
//      SP_OPT_COMPLEX and SP_OPT_SEPARATED_COMPLEX_VECTORS are set,
//      otherwise it disappears without a trace.
//
int
spMatrixFrame::spFileVector(char *file, spREAL *rhs IMAG_RHS_P)
{
    ASSERT(RHS != 0)

    // Open File in append mode.
    FILE *pMatrixFile;
    if ((pMatrixFile = fopen(file,"a")) == 0)
        return 0;

    // Correct array pointers for SP_OPT_ARRAY_OFFSET.
#if NOT SP_OPT_ARRAY_OFFSET
#if SP_OPT_COMPLEX
    if (Complex) {
#if SP_OPT_SEPARATED_COMPLEX_VECTORS
        ASSERT(iRHS != 0)
        --rhs;
        --irhs;
#else
        rhs -= 2;
#endif
    }
    else
#endif // SP_OPT_COMPLEX
        --rhs;
#endif // NOT SP_OPT_ARRAY_OFFSET


    // Output vector
#if SP_OPT_COMPLEX
    if (Complex) {
#if SP_OPT_SEPARATED_COMPLEX_VECTORS
        for (int i = 1; i <= Size; i++) {
            int err = fprintf(pMatrixFile, "%-.15g\t%-.15g\n",
                (double)rhs[i], (double)irhs[i]);
            if (err < 0)
                return 0;
        }
#else
        for (int i = 1; i <= Size; i++) {
            int err = fprintf(pMatrixFile, "%-.15g\t%-.15g\n",
                (double)rhs[2*i], (double)rhs[2*i+1]);
            if (err < 0)
                return 0;
        }
#endif
    }
#endif // SP_OPT_COMPLEX
#if SP_OPT_REAL AND SP_OPT_COMPLEX
    else
#endif
#if SP_OPT_REAL
    {
        for (int i = 1; i <= Size; i++) {
            if (fprintf(pMatrixFile, "%-.15g\n", (double)rhs[i]) < 0)
                return 0;
        }
    }
#endif // SP_OPT_REAL

// Close file
    if (fclose(pMatrixFile) < 0)
        return 0;
    return 1;
}


//  OUTPUT STATISTICS TO FILE
//
// Writes useful information concerning the matrix to a file.  Should
// be executed after the matrix is factored.
//
//
//  >>> Returns:
//
// One is returned if function was successful, otherwise zero is
// returned.  The calling function can query errno (the system global
// error variable) as to the reason why this function failed.
//
//  >>> Arguments:
//
//  file  <input>  (char *)
//      Name of file into which matrix is to be written.
//
//  label  <input>  (char *)
//      String that is transferred to file and is used as a label.
//
//  >>> Local variables:
//
//  data  (spREAL)
//      The value of the matrix element being output.
//
//  largestElement  (spREAL)
//      The largest element in the matrix.
//
//  numberOfElements  (int)
//      Number of nonzero elements in the matrix.
//
//  pElement  (spMatrixElement*)
//      Pointer to an element in the matrix.
//
//  pStatsFile  (FILE *)
//      File pointer to the statistics file.
//
//  smallestElement  (spREAL)
//      The smallest element in the matrix excluding zero elements.
//
int
spMatrixFrame::spFileStats(char *file, char *label)
{
    // Open File in append mode
    FILE *pStatsFile;
    if ((pStatsFile = fopen(file, "a")) == 0)
        return 0;

    // Output statistics
    if (NOT Factored)
        fprintf(pStatsFile, "Matrix has not been factored.\n");
    fprintf(pStatsFile, "|||  Starting new matrix  |||\n");
    fprintf(pStatsFile, "%s\n", label);
    if (Complex)
        fprintf(pStatsFile, "Matrix is complex.\n");
    else
        fprintf(pStatsFile, "Matrix is real.\n");
    fprintf(pStatsFile,"     Size = %d\n", Size);

    // Search matrix
    int numberOfElements = 0;
    double largestElement = 0.0;
    double smallestElement = DBL_MAX;

    for (int i = 1; i <= Size; i++) {
        spMatrixElement *pElement = FirstInCol[i];
        while (pElement != 0) {
            numberOfElements++;
            double data = E_MAG(pElement);
            if (data > largestElement)
                largestElement = data;
            if (data < smallestElement AND data != 0.0)
                smallestElement = data;
            pElement = pElement->NextInCol;
        }
    }

    smallestElement = SPMIN(smallestElement, largestElement);

    // Output remaining statistics
    fprintf(pStatsFile, "     Initial number of elements = %d\n",
            numberOfElements - Fillins);
    fprintf(pStatsFile,
            "     Initial average number of elements per row = %f\n",
            (double)(numberOfElements - Fillins) / (double)Size);
    fprintf(pStatsFile, "     Fill-ins = %d\n", Fillins);
    fprintf(pStatsFile, "     Average number of fill-ins per row = %f%%\n",
            (double)Fillins / (double)Size);
    fprintf(pStatsFile, "     Total number of elements = %d\n",
            numberOfElements);
    fprintf(pStatsFile, "     Average number of elements per row = %f\n",
            (double)numberOfElements / (double)Size);
    fprintf(pStatsFile,"     Density = %f%%\n",
            (double)(100.0*numberOfElements)/(double)(Size*Size));
    fprintf(pStatsFile,"     Relative Threshold = %e\n", RelThreshold);
    fprintf(pStatsFile,"     Absolute Threshold = %e\n", AbsThreshold);
    fprintf(pStatsFile,"     Largest Element = %e\n", largestElement);
    fprintf(pStatsFile,"     Smallest Element = %e\n\n\n", smallestElement);

    // Close file
    fclose(pStatsFile);
    return 1;
}

#endif // SP_OPT_DOCUMENTATION

