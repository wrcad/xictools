
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2011 Whiteley Research Inc, all rights reserved.        *
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
 $Id: klumatrix.h,v 2.10 2016/07/27 04:41:10 stevew Exp $
 *========================================================================*/

#ifndef KLUMATRIX_H
#define KLUMATRIX_H

#ifdef USE_KLU
#include "klu.h"


//
// The KLUmatrix struct is given to Sparse, providing overrides for
// the matrix factorization/solving functions.
//

struct KLUmatrix : public spMatlabMatrix
{
    KLUmatrix(int, int, bool, bool);
    ~KLUmatrix();

    void clear();
    void negate();
    double largest();
    double smallest();
    void toInit();
    void fromInit();
    bool toComplex();
    bool toReal(bool);
    double *find(int, int);
    bool checkColOnes(int);
    int factor();
    int refactor();
    int solve(double*);
    int tsolve(double*, bool);
    bool where_singular(int*);

private:
    double *Ainit;
    long double *RhsTmp;
    klu_symbolic *Symbolic;
    klu_numeric *Numeric;
    klu_common Common;
};

#endif
#endif

