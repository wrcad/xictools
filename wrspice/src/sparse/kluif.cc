
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

#include "sparse/spmatrix.h"
#include "klumatrix.h"
#include "kluif.h"
#include "spglobal.h"
#include "miscutil/lstring.h"
#include <math.h>
#ifdef WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

//
// Functions for the KLU matrix solver plug-in.  If available, KLU
// will take over the factoring and solving duties from Sparse,
// providing a big speed advantage in many cases.
//

// Observing singular matrix problems with bsoi45 model in ring_osc,
// the Sparse package runs ok, and KLU with extended precision runs
// ok.  Tried COLAMD but it made matters worse, extended precision no
// longer works, and the failure occurs much earlier.
//
enum {AMD, COLAMD};
#define ORDERING AMD

#ifdef __APPLE__
#define KLU_SO "klu_wr.dylib"
#else
#ifdef WIN32
#define KLU_SO "klu_wr.dll"
#else
#define KLU_SO "klu_wr.so"
#endif
#endif

//#define VERBOSE

#define LDBL(foo) (*(long double*)foo)


// Instantiate the interface;
KLUif klu_if;


namespace {
#ifdef WIN32

    inline void *get_func(HINSTANCE handle, const char *funcname)
    {
        void *f = (void*)GetProcAddress(handle, funcname);
#ifdef VERBOSE
        if (!f)
            printf("GetProcAddress \"%s\" failed: %s\n", funcname,
                dlerror());
#endif
        return (f);
    }

#else

    inline void *get_func(void *handle, const char *funcname)
    {
        void *f = dlsym(handle, funcname);
#ifdef VERBOSE
        if (!f)
            printf("dlsym \"%s\" failed: %s\n", funcname, dlerror());
#endif
        return (f);
    }

#endif
}


void
KLUif::find_klu()
{
    sLstr lstr;
    const char *klu_path = getenv("XT_KLU_PATH");
    if (klu_path) {
        // User told us where to look.
        lstr.add(klu_path);
    }
    else {
        // Look in the startup directory.
        lstr.add(Global.StartupDir());
        lstr.add_c('/');
        lstr.add(KLU_SO);
    }

#ifdef WIN32
    HINSTANCE handle = LoadLibrary(lstr.string());
    if ((unsigned long)handle <= HINSTANCE_ERROR) {
#ifdef VERBOSE
        printf("LoadLibrary failed for %s\n", lstr.string());
#endif
        return;
    }
#else
    void *handle = dlopen(lstr.string(), RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
#ifdef VERBOSE
        printf("dlopen failed: %s\n", dlerror());
#endif
        return;
    }
#endif
    int(*vers)(int*) = (int(*)(int*))get_func(handle, "SuiteSparse_version");
    if (!vers) {
#ifdef VERBOSE
        printf("Can't find KLU version.\n");
#endif
        return;
    }
    int soversion[3];
    (*vers)(soversion);
    if (soversion[0] != SUITESPARSE_MAIN_VERSION ||
            soversion[1] != SUITESPARSE_SUB_VERSION ||
            soversion[2] != SUITESPARSE_SUBSUB_VERSION) {
#ifdef VERBOSE
        printf("KLU plug-in library version mismatch.\n");
#endif
        return;
    }

    pklu_defaults = (int(*)(klu_common*))get_func(handle, "klu_defaults");
    if (!pklu_defaults)
        return;

    pklu_analyze = (klu_symbolic*(*)(int, int*, int*, klu_common*))
        get_func(handle, "klu_analyze");
    if (!pklu_analyze)
        return;

    pklu_factor = (klu_numeric*(*)(int*, int*, double*, klu_symbolic*,
        klu_common*))get_func(handle, "klu_factor");
    if (!pklu_factor)
        return;

    pklu_ld_factor = (klu_numeric*(*)(int*, int*, long double*, klu_symbolic*,
        klu_common*))get_func(handle, "klu_ld_factor");
    if (!pklu_ld_factor)
        return;

    pklu_z_factor = (klu_numeric*(*)(int*, int*, double*, klu_symbolic*,
        klu_common*))get_func(handle, "klu_z_factor");
    if (!pklu_z_factor)
        return;

    pklu_refactor = (int(*)(int*, int*, double*, klu_symbolic*, klu_numeric*,
        klu_common*))get_func(handle, "klu_refactor");
    if (!pklu_refactor)
        return;

    pklu_ld_refactor = (int(*)(int*, int*, long double*, klu_symbolic*,
        klu_numeric*, klu_common*))get_func(handle, "klu_ld_refactor");
    if (!pklu_ld_refactor)
        return;

    pklu_z_refactor = (int(*)(int*, int*, double*, klu_symbolic*,
        klu_numeric*, klu_common*))get_func(handle, "klu_z_refactor");
    if (!pklu_z_refactor)
        return;

    pklu_solve = (int(*)(klu_symbolic*, klu_numeric*, int, int, double*,
        klu_common*))get_func(handle, "klu_solve");
    if (!pklu_solve)
        return;

    pklu_ld_solve = (int(*)(klu_symbolic*, klu_numeric*, int, int,
        long double*, klu_common*))get_func(handle, "klu_ld_solve");
    if (!pklu_ld_solve)
        return;

    pklu_z_solve = (int(*)(klu_symbolic*, klu_numeric*, int, int, double*,
        klu_common*))get_func(handle, "klu_z_solve");
    if (!pklu_z_solve)
        return;

    pklu_tsolve = (int(*)(klu_symbolic*, klu_numeric*, int, int, double*,
        klu_common*))get_func(handle, "klu_tsolve");
    if (!pklu_tsolve)
        return;

    pklu_ld_tsolve = (int(*)(klu_symbolic*, klu_numeric*, int, int,
        long double*, klu_common*))get_func(handle, "klu_ld_tsolve");
    if (!pklu_ld_tsolve)
        return;

    pklu_z_tsolve = (int(*)(klu_symbolic*, klu_numeric*, int, int, double*,
        int, klu_common*))get_func(handle, "klu_z_tsolve");
    if (!pklu_z_tsolve)
        return;

    pklu_free_symbolic = (void(*)(klu_symbolic**, klu_common*))
        get_func(handle, "klu_free_symbolic");
    if (!pklu_free_symbolic)
        return;

    pklu_free_numeric = (void(*)(klu_numeric**, klu_common*))
        get_func(handle, "klu_free_numeric");
    if (!pklu_free_numeric)
        return;

    pklu_ld_free_numeric = (void(*)(klu_numeric**, klu_common*))
        get_func(handle, "klu_ld_free_numeric");
    if (!pklu_ld_free_numeric)
        return;

    pklu_z_free_numeric = (void(*)(klu_numeric**, klu_common*))
        get_func(handle, "klu_z_free_numeric");
    if (!pklu_z_free_numeric)
        return;

    printf(
        "Loading KLU fast sparse matrix solver, by Tim Davis,\n"
        "Texas A&M University, "
        "http://faculty.cse.tamu.edu/davis/welcome.html.\n");

    klu_is_ok = true;
}
// End of KLUif functions.


KLUmatrix::KLUmatrix(int size, int nelts, bool cplx, bool ldbl) :
    spMatlabMatrix(size, nelts, cplx, ldbl)
{
    Ainit = 0;
    RhsTmp = 0;
    Symbolic = 0;
    Numeric = 0;
    if (klu_if.is_ok()) {
        klu_if.klu_defaults(&Common);
        Common.ordering = ORDERING;
    }
}


KLUmatrix::~KLUmatrix()
{
    delete [] Ainit;
    delete [] RhsTmp;
    if (klu_if.is_ok()) {
        klu_if.klu_free_symbolic(&Symbolic, &Common);
        if (Complex)
            klu_if.klu_z_free_numeric(&Numeric, &Common);
        else if (LongDoubles)
            klu_if.klu_ld_free_numeric(&Numeric, &Common);
        else
            klu_if.klu_free_numeric(&Numeric, &Common);
    }
}


// Reset all matrix element values to 0.
//
void
KLUmatrix::clear()
{
    int sz = Complex || LongDoubles ? NumElts + NumElts : NumElts;
    memset(Ax, 0, sz*sizeof(double));
}


// Negate all matrix values.
//
void
KLUmatrix::negate()
{
    for (int i = 0; i < NumElts; i++)
        Ax[i] = -Ax[i];
}


// Return the largest absolute value real element.
//
double
KLUmatrix::largest()
{
    double lx = 0.0;
    for (int i = 0; i < NumElts; i++) {
        double f = fabs(Ax[i]);
        if (f > lx)
            lx = f;
    }
    return (lx);
}


// Return the smallest absolute value real element that is not 1, or 1.
//
double
KLUmatrix::smallest()
{
    double sx = HUGE_VAL;
    for (int i = 0; i < NumElts; i++) {
        double f = fabs(Ax[i]);
        if (f < sx && f != 1.0 && f != 0.0 && f >= 1e-6)
            sx = f;
    }
    if (sx == HUGE_VAL)
        sx = 1.0;
    return (sx);
}


// Copy the matrix to a backup area, to be used for future
// initialization.
//
void
KLUmatrix::toInit()
{
    int sz = Complex || LongDoubles ? NumElts + NumElts : NumElts;
    if (!Ainit)
        Ainit = new double[sz];
    memcpy(Ainit, Ax, sz*sizeof(double));
}


// If we saved initialization, use it to reinitialize the matrix. 
// Otherwise, clear it.
//
void
KLUmatrix::fromInit()
{
    int sz = Complex || LongDoubles ? NumElts + NumElts : NumElts;
    if (Ainit)
        memcpy(Ax, Ainit, sz*sizeof(double));
    else
        memset(Ax, 0, sz*sizeof(double));
}


// Switch to a complex matrix, zeroing all imaginary entries while
// preserving the real entries.  If a switch occurs, true is returned.
//
bool
KLUmatrix::toComplex()
{
    if (Complex)
        return (false);
    if (LongDoubles) {
        int j = 0;
        for (int i = 0; i < NumElts; i++) {
            Ax[j] = LDBL(&Ax[j]);
            j++;
            Ax[j++] = 0.0;
        }
        if (Ainit) {
            j = 0;
            for (int i = 0; i < NumElts; i++) {
                Ainit[j] = LDBL(&Ainit[j]);
                j++;
                Ainit[j++] = 0;
            }
        }
    }
    else {
        double *az = new double[2*NumElts];
        int j = 0;
        for (int i = 0; i < NumElts; i++) {
            az[j++] = Ax[i];
            az[j++] = 0.0;
        }
        delete [] Ax;
        Ax = az;
        if (Ainit) {
            double *ain = new double[2*NumElts];
            j = 0;
            for (int i = 0; i < NumElts; i++) {
                ain[j++] = Ainit[i];
                ain[j++] = 0;
            }
            delete [] Ainit;
            Ainit = ain;
        }
    }

    Complex = true;
    if (LongDoubles) {
        LongDoubles = false;
        klu_if.klu_ld_free_numeric(&Numeric, &Common);
    }
    else
        klu_if.klu_free_numeric(&Numeric, &Common);
    Numeric = 0;
    klu_if.klu_defaults(&Common);
    Common.ordering = ORDERING;
    return (true);
}


// Switch to a real matrix, preserving the real values while throwing
// out complex values.  If a switch occurs, true is returned.
//
bool
KLUmatrix::toReal(bool ldbl)
{
    if (!Complex)
        return (false);
    if (ldbl) {
        long double *ax = (long double*)Ax;
        int j = 0;
        for (int i = 0; i < NumElts; i++) {
            ax[i] = Ax[j++];
            j++;
        }
        if (Ainit) {
            long double *ain = (long double*)Ainit;
            j = 0;
            for (int i = 0; i < NumElts; i++) {
                ain[i] = Ainit[j++];
                j++;
            }
        }
    }
    else {
        double *ax = new double[NumElts];
        int j = 0;
        for (int i = 0; i < NumElts; i++) {
            ax[i] = Ax[j++];
            j++;
        }
        delete [] Ax;
        Ax = ax;
        if (Ainit) {
            double *ain = new double[NumElts];
            j = 0;
            for (int i = 0; i < NumElts; i++) {
                ain[i] = Ainit[j++];
                j++;
            }
            delete [] Ainit;
            Ainit = ain;
        }
    }

    Complex = false;
    LongDoubles = ldbl;
    klu_if.klu_z_free_numeric(&Numeric, &Common);
    Numeric = 0;
    klu_if.klu_defaults(&Common);
    Common.ordering = ORDERING;
    return (true);
}


// Return a pointer to the (real) data for row,col.  Note that if
// LongDoubles, the pointer actually points to a long double number.
//
double *
KLUmatrix::find(int row, int col)
{
    int end = Ap[col+1];
    for (int i = Ap[col]; i < end; i++) {
        if (Ai[i] == row) {
            if (Complex || LongDoubles)
                return (Ax + i + i);
            return (Ax + i);
        }
    }
    return (0);
}


// If we find a column with no diagonal element (when loading
// gmin), it may be a junction of branch devices, in which case
// there will be one or more 1/-1 values in the column.  Return
// true if this is the case.  Note that an unconnected voltage
// source will pass this test.
//
bool
KLUmatrix::checkColOnes(int col)
{
    int end = Ap[col+1];
    if (Complex) {
        for (int i = Ap[col]; i < end; i++) {
            if (fabs(Ax[i+i]) == 1.0)
                return (true);
        }
    }
    else if (LongDoubles) {
        for (int i = Ap[col]; i < end; i++) {
            if (fabsl(LDBL(&Ax[i+i])) == 1.0L)
                return (true);
        }
    }
    else {
        for (int i = Ap[col]; i < end; i++) {
            if (fabs(Ax[i]) == 1.0)
                return (true);
        }
    }
    return (false);
}


namespace {
    inline int status(int s)
    {
        if (s < 0 || s == KLU_OK)
            return (spOKAY);
        if (s == KLU_SINGULAR)
            return (spSINGULAR);
        if (s == KLU_OUT_OF_MEMORY)
            return (spNO_MEMORY);
        return (spPANIC);
    }

    /**** For Debugging
    void prnt(double *ax, int sz, bool ld)
    {
        if (ld) {
            for (int i = 0; i < sz; i++)
                printf(">>%d %Lg\n", i, LDBL(&ax[i+i]));
        }
        else {
            for (int i = 0; i < sz; i++)
                printf(">>%d %g\n", i, ax[i]);
        }
    }
    ****/
}


int
KLUmatrix::factor()
{
    if (!klu_if.is_ok())
        return (spPANIC);
    Common.status = KLU_OK;
    klu_if.klu_free_symbolic(&Symbolic, &Common);
    Symbolic = klu_if.klu_analyze(Size, Ap, Ai, &Common);
    if (Complex) {
        klu_if.klu_z_free_numeric(&Numeric, &Common);
        Numeric = klu_if.klu_z_factor(Ap, Ai, Ax, Symbolic, &Common);
    }
    else if (LongDoubles) {
        klu_if.klu_ld_free_numeric(&Numeric, &Common);
        Numeric = klu_if.klu_ld_factor(Ap, Ai, (long double*)Ax, Symbolic,
            &Common);
    }
    else {
        klu_if.klu_free_numeric(&Numeric, &Common);
        Numeric = klu_if.klu_factor(Ap, Ai, Ax, Symbolic, &Common);
    }
    return (status(Common.status));
}


int
KLUmatrix::refactor()
{
    if (!klu_if.is_ok())
        return (spPANIC);
    if (Complex)
        klu_if.klu_z_refactor(Ap, Ai, Ax, Symbolic, Numeric, &Common);
    else if (LongDoubles)
        klu_if.klu_ld_refactor(Ap, Ai, (long double*)Ax, Symbolic, Numeric,
            &Common);
    else
        klu_if.klu_refactor(Ap, Ai, Ax, Symbolic, Numeric, &Common);
    return (status(Common.status));
}


int
KLUmatrix::solve(double *rhs)
{
    if (!klu_if.is_ok())
        return (spPANIC);
    if (Complex)
        klu_if.klu_z_solve(Symbolic, Numeric, Size, 1, rhs, &Common);
    else if (LongDoubles) {
        if (!RhsTmp)
            RhsTmp = new long double[Size];
        for (int i = 0; i < Size; i++)
            RhsTmp[i] = rhs[i];
        klu_if.klu_ld_solve(Symbolic, Numeric, Size, 1, RhsTmp, &Common);
        for (int i = 0; i < Size; i++)
            rhs[i] = RhsTmp[i];
    }
    else
        klu_if.klu_solve(Symbolic, Numeric, Size, 1, rhs, &Common);
    return (status(Common.status));
}


int
KLUmatrix::tsolve(double *rhs, bool conj)
{
    if (!klu_if.is_ok())
        return (spPANIC);
    if (Complex)
        klu_if.klu_z_tsolve(Symbolic, Numeric, Size, 1, rhs, conj, &Common);
    else if (LongDoubles) {
        if (!RhsTmp)
            RhsTmp = new long double[Size];
        for (int i = 0; i < Size; i++)
            RhsTmp[i] = rhs[i];
        klu_if.klu_ld_solve(Symbolic, Numeric, Size, 1, RhsTmp, &Common);
        for (int i = 0; i < Size; i++)
            rhs[i] = RhsTmp[i];
    }
    else
        klu_if.klu_solve(Symbolic, Numeric, Size, 1, rhs, &Common);
    return (status(Common.status));
}


bool
KLUmatrix::where_singular(int *col)
{
    if (Common.status == KLU_SINGULAR && Common.singular_col >= 0 &&
            Common.singular_col < Size) {
        *col = Common.singular_col;
        return (true);
    }
    *col = -1;
    return (false);
}

