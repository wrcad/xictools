
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
//  CONFIGURATION MACRO DEFINITIONS for sparse matrix package
//
//  Author:                     Advising professor:
//      Kenneth S. Kundert          Alberto Sangiovanni-Vincentelli
//      U.C. Berkeley
//
// This file contains macros for the sparse matrix functions that are
// used to define the personality of the functions.  The user is
// expected to modify this file to maximize the performance of the
// functions with his/her matrices.
//
// Macros are distinguished by using solely capital letters in their
// identifiers.  This contrasts with C defined identifiers which are
// strictly lower case, and program variable and procedure names which
// use both upper and lower case.

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

#ifndef spCONFIG_H
#define spCONFIG_H

#ifdef spINSIDE_SPARSE


//  MATRIX CONSTANTS
//
// These constants are used throughout the sparse matrix functions. 
// They should be set to suit the type of matrix being solved. 
// Recommendations are given in brackets.
//
// Some terminology should be defined.  The Markowitz row count is the
// number of non-zero elements in a row excluding the one being
// considered as pivot.  There is one Markowitz row count for every
// row.  The Markowitz column is defined similarly for columns.  The
// Markowitz product for an element is the product of its row and
// column counts.  It is a measure of how much work would be required
// on the next step of the factorization if that element were chosen
// to be pivot.  A small Markowitz product is desirable.
//
//  >>> Constants descriptions:
//
//  DEFAULT_THRESHOLD
//      The relative threshold used if the user enters an invalid
//      threshold.  Also the threshold used by spFactor() when
//      calling spOrderAndFactor().  The default threshold should
//      not be less than or equal to zero nor larger than one. [0.001]
//
//  DIAG_PIVOTING_AS_DEFAULT
//      This indicates whether spOrderAndFactor() should use diagonal
//      pivoting as default.  This issue only arises when
//      spOrderAndFactor() is called from spFactor().
//
//  SPACE_FOR_ELEMENTS
//      This number multiplied by the size of the matrix equals the number
//      of elements for which memory is initially allocated in
//      spCreate(). [6]
//
//  SPACE_FOR_FILL_INS
//      This number multiplied by the size of the matrix equals the number
//      of elements for which memory is initially allocated and specifically
//      reserved for fill-ins in spCreate(). [4]
//
//  ELEMENTS_PER_ALLOCATION
//      The number of matrix elements requested from the malloc utility on
//      each call to it.  Setting this value greater than 1 reduces the
//      amount of overhead spent in this system call. On a virtual memory
//      machine, its good to allocate slightly less than a page worth of
//      elements at a time (or some multiple thereof).
//      [For the VAX, for real only use 41, otherwise use 31]
//
//  MINIMUM_ALLOCATED_SIZE
//      The minimum allocated size of a matrix.  Note that this does not
//      limit the minimum size of a matrix.  This just prevents having to
//      resize a matrix many times if the matrix is expandable, large and
//      allocated with an estimated size of zero.  This number should not
//      be less than one.
//
//  EXPANSION_FACTOR
//      The amount the allocated size of the matrix is increased when it
//      is expanded.
//
//  MAX_MARKOWITZ_TIES
//      This number is used for two slightly different things, both of which
//      relate to the search for the best pivot.  First, it is the maximum
//      number of elements that are Markowitz tied that will be sifted
//      through when trying to find the one that is numerically the best.
//      Second, it creates an upper bound on how large a Markowitz product
//      can be before it eliminates the possibility of early termination
//      of the pivot search.  In other words, if the product of the smallest
//      Markowitz product yet found and TIES_MULTIPLIER is greater than
//      MAX_MARKOWITZ_TIES, then no early termination takes place.
//      Set MAX_MARKOWITZ_TIES to some small value if no early termination of
//      the pivot search is desired. An array of spREALs is allocated
//      of size MAX_MARKOWITZ_TIES so it must be positive and shouldn't
//      be too large.  Active when SP_OPT_MODIFIED_MARKOWITZ is 1 (true).
//      [100]
//
//  TIES_MULTIPLIER
//      Specifies the number of Markowitz ties that are allowed to occur
//      before the search for the pivot is terminated early.  Set to some
//      large value if no early termination of the pivot search is desired.
//      This number is multiplied times the Markowitz product to determine
//      how many ties are required for early termination.  This means that
//      more elements will be searched before early termination if a large
//      number of fill-ins could be created by accepting what is currently
//      considered the best choice for the pivot.  Active when
//      SP_OPT_MODIFIED_MARKOWITZ is 1 (true).  Setting this number to zero
//      effectively eliminates all pivoting, which should be avoided.
//      This number must be positive.  TIES_MULTIPLIER is also used when
//      diagonal pivoting breaks down. [5]
//

#define  DEFAULT_THRESHOLD              1.0e-3
#define  DIAG_PIVOTING_AS_DEFAULT       YES
#define  SPACE_FOR_ELEMENTS             6
#define  SPACE_FOR_FILL_INS             4
#define  ELEMENTS_PER_ALLOCATION        31
#define  MINIMUM_ALLOCATED_SIZE         6
#define  EXPANSION_FACTOR               1.5
#define  MAX_MARKOWITZ_TIES             100
#define  TIES_MULTIPLIER                5


//  PRINTER WIDTH
//
// This macro characterize the printer for the spPrint() function.
//
//  >>> Macros:
//
//  PRINTER_WIDTH
//      The number of characters per page width.  Set to 80 for terminal,
//      132 for line printer.

#define  PRINTER_WIDTH  80


//  ANNOTATION
//
// This macro changes the amount of annotation produced by the matrix
// functions.  The annotation is used as a debugging aid.  Change the
// number associated with ANNOTATE to change the amount of annotation
// produced by the program.

#define  ANNOTATE               NONE

#define  NONE                   0
#define  ON_STRANGE_BEHAVIOR    1
#define  FULL                   2

#endif // spINSIDE_SPARSE
#endif // spCONFIG_H

