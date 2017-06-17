
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
 $Id: kluif.h,v 2.4 2014/04/20 05:13:02 stevew Exp $
 *========================================================================*/

#ifndef KLUIF_H
#define KLUIF_H


//
// Global interface to the KLU functions.  If the KLU module is
// available, the function pointers provide access to the parts of KLU
// used in WRspice.  The find_klu method is called on program startup.
// If KLU is loaded successfully, the is_ok method will return true.
//

#ifndef KLUMATRIX_H
// If both kluif.h and klumatrix.h are included, klumatrix.h must come
// first.
struct klu_common;
struct klu_numeric;
struct klu_symbolic;
#endif

struct KLUif
{
    KLUif()
        {
            pklu_defaults = 0;
            pklu_analyze = 0;
            pklu_factor = 0;
            pklu_ld_factor = 0;
            pklu_z_factor = 0;
            pklu_refactor = 0;
            pklu_ld_refactor = 0;
            pklu_z_refactor = 0;
            pklu_solve = 0;
            pklu_ld_solve = 0;
            pklu_z_solve = 0;
            pklu_tsolve = 0;
            pklu_ld_tsolve = 0;
            pklu_z_tsolve = 0;
            pklu_free_symbolic = 0;
            pklu_free_numeric = 0;
            pklu_ld_free_numeric = 0;
            pklu_z_free_numeric = 0;
            klu_is_ok = false;
        }

    void find_klu();

    bool is_ok()
        {
            return (klu_is_ok);
        }

    int klu_defaults(klu_common *common)
        {
            return ((*pklu_defaults)(common));
        }

    klu_symbolic *klu_analyze(int size, int *ap, int *ai, klu_common *common)
        {
            return ((*pklu_analyze)(size, ap, ai, common));
        }

    klu_numeric *klu_factor(int *ap, int *ai, double *ax,
        klu_symbolic *symbolic, klu_common *common)
        {
            return ((*pklu_factor)(ap, ai, ax, symbolic, common));
        }

    klu_numeric *klu_ld_factor(int *ap, int *ai, long double *ax,
        klu_symbolic *symbolic, klu_common *common)
        {
            return ((*pklu_ld_factor)(ap, ai, ax, symbolic, common));
        }

    klu_numeric *klu_z_factor(int *ap, int *ai, double *ax,
        klu_symbolic *symbolic, klu_common *common)
        {
            return ((*pklu_z_factor)(ap, ai, ax, symbolic, common));
        }

    int klu_refactor(int *ap, int *ai, double *ax, klu_symbolic *symbolic,
        klu_numeric *numeric, klu_common *common)
        {
            return ((*pklu_refactor)(ap, ai, ax, symbolic, numeric, common));
        }

    int klu_ld_refactor(int *ap, int *ai, long double *ax,
        klu_symbolic *symbolic, klu_numeric *numeric, klu_common *common)
        {
            return ((*pklu_ld_refactor)(ap, ai, ax, symbolic, numeric, common));
        }

    int klu_z_refactor(int *ap, int *ai, double *ax, klu_symbolic *symbolic,
        klu_numeric *numeric, klu_common *common)
        {
            return ((*pklu_z_refactor)(ap, ai, ax, symbolic, numeric, common));
        }

    int klu_solve(klu_symbolic *symbolic, klu_numeric *numeric, int size,
        int nc, double *rhs, klu_common *common)
        {
            return ((*pklu_solve)(symbolic, numeric, size, nc, rhs, common));
        }

    int klu_ld_solve(klu_symbolic *symbolic, klu_numeric *numeric, int size,
        int nc, long double *rhs, klu_common *common)
        {
            return ((*pklu_ld_solve)(symbolic, numeric, size, nc, rhs, common));
        }

    int klu_z_solve(klu_symbolic *symbolic, klu_numeric *numeric, int size,
        int nc, double *rhs, klu_common *common)
        {
            return ((*pklu_z_solve)(symbolic, numeric, size, nc, rhs, common));
        }

    int klu_tsolve(klu_symbolic *symbolic, klu_numeric *numeric, int size,
        int nc, double *rhs, klu_common *common)
        {
            return ((*pklu_tsolve)(symbolic, numeric, size, nc, rhs, common));
        }

    int klu_ld_tsolve(klu_symbolic *symbolic, klu_numeric *numeric, int size,
        int nc, long double *rhs, klu_common *common)
        {
            return ((*pklu_ld_tsolve)(symbolic, numeric, size, nc, rhs,
                common));
        }

    int klu_z_tsolve(klu_symbolic *symbolic, klu_numeric *numeric, int size,
        int nc, double *rhs, int conj, klu_common *common)
        {
            return ((*pklu_z_tsolve)(symbolic, numeric, size, nc, rhs, conj,
                common));
        }

    void klu_free_symbolic(klu_symbolic **symbp, klu_common *common)
        {
            (*pklu_free_symbolic)(symbp, common);
        }

    void klu_free_numeric(klu_numeric **nump, klu_common *common)
        {
            (*pklu_free_numeric)(nump, common);
        }

    void klu_ld_free_numeric(klu_numeric **nump, klu_common *common)
        {
            (*pklu_ld_free_numeric)(nump, common);
        }

    void klu_z_free_numeric(klu_numeric **nump, klu_common *common)
        {
            (*pklu_z_free_numeric)(nump, common);
        }

private:
    int (*pklu_defaults)(klu_common*);
    klu_symbolic *(*pklu_analyze)(int, int*, int*, klu_common*);
    klu_numeric *(*pklu_factor)(int*, int*, double*, klu_symbolic*,
        klu_common*);
    klu_numeric *(*pklu_ld_factor)(int*, int*, long double*, klu_symbolic*,
        klu_common*);
    klu_numeric *(*pklu_z_factor)(int*, int*, double*, klu_symbolic*,
        klu_common*);
    int (*pklu_refactor)(int*, int*, double*, klu_symbolic*, klu_numeric*,
        klu_common*);
    int (*pklu_ld_refactor)(int*, int*, long double*, klu_symbolic*,
        klu_numeric*, klu_common*);
    int (*pklu_z_refactor)(int*, int*, double*, klu_symbolic*, klu_numeric*,
        klu_common*);
    int (*pklu_solve)(klu_symbolic*, klu_numeric*, int, int, double*,
        klu_common*);
    int (*pklu_ld_solve)(klu_symbolic*, klu_numeric*, int, int, long double*,
        klu_common*);
    int (*pklu_z_solve)(klu_symbolic*, klu_numeric*, int, int, double*,
        klu_common*);
    int (*pklu_tsolve)(klu_symbolic*, klu_numeric*, int, int, double*,
        klu_common*);
    int (*pklu_ld_tsolve)(klu_symbolic*, klu_numeric*, int, int, long double*,
        klu_common*);
    int (*pklu_z_tsolve)(klu_symbolic*, klu_numeric*, int, int, double*,
        int, klu_common*);
    void (*pklu_free_symbolic)(klu_symbolic**, klu_common*);
    void (*pklu_free_numeric)(klu_numeric**, klu_common*);
    void (*pklu_ld_free_numeric)(klu_numeric**, klu_common*);
    void (*pklu_z_free_numeric)(klu_numeric**, klu_common*);

    bool klu_is_ok;
};
extern KLUif klu_if;  // Instantiated in kluif.cc.

#endif

