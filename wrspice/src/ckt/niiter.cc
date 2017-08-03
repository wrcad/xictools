
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "config.h"
#include "optdefs.h"
#include "input.h"
#include "spmatrix.h"
#include "frontend.h"
#include "outdata.h"
#include "outplot.h"
#include "misc.h"
#include "device.h"
#include "ttyio.h"

#ifdef HAVE_FENV_H
#include <fenv.h>
#endif


#if 0
//-----------------------------------------------------------------------------
// Experimental
// Try to find some pattern to non-convergence.

#define MONO    0x1
#define OSC     0x2
#define INC     0x4

namespace {
    unsigned int
    test_iter(double d1, double d2, double d3, double d4)
    {
        double d21 = d2 - d1;
        double d32 = d3 - d2;
        double d43 = d4 - d3;

        bool p21 = (d21 >= 0.0);
        bool p32 = (d32 >= 0.0);
        bool p43 = (d43 >= 0.0);

        unsigned int f = 0;
        if (p21 == p32 && p32 == p43)
            f |= MONO;
        else if (p32 == !p21 && p32 == !p43)
            f |= OSC;
        if (fabs(d43) < fabs(d21))
            f |= INC;

        return (f);
    }


    void
    foo_test(sCKT *ckt, int itnum)
    {
        static double **sols;

        int sz = ckt->CKTmatrix->spGetSize(1) + 1;
        if (!sols) {
            sols = new double*[5];
            for (int i = 0; i < 5; i++)
                sols[i] = new double[sz];
        }
        double *tmp = sols[4];
        sols[4] = sols[3];
        sols[3] = sols[2];
        sols[2] = sols[1];
        sols[1] = sols[0];
        sols[0] = tmp;
        memcpy(tmp, ckt->CKTrhsOld, sz*sizeof(double));
        if (itnum > 4) {
            int nmon = 0;
            int nosc = 0;
            int ninc = 0;
            int npx = 0;
            int nnx = 0;
            for (int i = 1; i < sz; i++) {
                unsigned int f = test_iter(sols[3][i], sols[2][i], sols[1][i],
                    sols[0][i]);
                if (f & MONO)
                    nmon++;
                if (f & OSC)
                    nosc++;
                if (f & INC)
                    ninc++;
            }
            TTY.err_printf(
                "mono %d  osc %d  inc %d npx %d nnx %d\n", nmon, nosc, ninc);
        }
    }
}
//-----------------------------------------------------------------------------
#endif


namespace {
    // Floating point exception checking.  Exceptions can be enabled
    // in wrspice.cc for debugging.
    //
    inline int check_fpe(bool noerrret)
    {
        int err = OK;
#ifdef HAVE_FENV_H
        if (!noerrret && Sp.GetFPEmode() == FPEcheck) {
            if (fetestexcept(FE_DIVBYZERO))
                err = E_MATHDBZ;
            else if (fetestexcept(FE_OVERFLOW))
                err = E_MATHOVF;
            // Ignore underflow, SOI models generate these with great
            // frequency.  They really shouldn't be a problem.
            // else if (fetestexcept(FE_UNDERFLOW))
            //     err = E_MATHUNF;
            //
            else if (fetestexcept(FE_INVALID))
                err = E_MATHINV;
        }
        feclearexcept(FE_ALL_EXCEPT);
#else
        (void)noerrret;
#endif
        return (err);
    }
}


// Clip intermediate RHS voltages to the power supply.  This was in
// WRspice for a long time, however it seems to increase, not
// decrease, iteration count, so is dubious.
//
// #define TEST_VCLIP

// The damping algorithm is from ngspice.  At present, I don't see
// any benefit.
//
// #define TEST_DAMP


//  This function performs the actual numerical iteration.
//  It uses the sparse matrix stored in the circuit struct
//  along with the matrix loading program, the load data, the
//  convergence test function, and the convergence parameters
//  - return value is non-zero for convergence failure.
//
int
sCKT::NIiter(int maxIter)
{
    // assume that NIreinit() has been called
    int iterno = 0;
    int ipass = 0;
    CKTnoncon = 0;
#ifdef TEST_DAMP
    double *OldCKTstate0 = 0;
#endif

    CKTmatrix->spSetReal();
    if (CKTmatrix->spDataAddressChange()) {
        int error = resetup();
        if (error)
            return (error);
    }

    check_fpe(true);

    // The NISHOULDREORDER state must be known in load for long
    // doubles mode.
    if ((CKTmode & MODEINITJCT) || (CKTmode & MODEINITTRAN))
        CKTniState |= NISHOULDREORDER;
    int error = load();
    if (error) {
        if (CKTstepDebug)
            TTY.err_printf("load (1) returned error\n");
    }
    else {
        error = check_fpe(false);
        if (error) {
            if (CKTstepDebug)
                TTY.err_printf("load (1) generated FP error\n");
        }
        else if (!(CKTniState & NIDIDPREORDER)) {
            CKTmatrix->spMNA_Preorder();
            error = CKTmatrix->spError();
            if (error) {
                // badly formed matrix
                if (CKTstepDebug)
                    TTY.err_printf("pre-order returned error\n");
            }
            else {
                error = check_fpe(false);
                if (error) {
                    if (CKTstepDebug)
                        TTY.err_printf("pre-order generated FP error\n");
                }
                CKTniState |= NIDIDPREORDER;
            }
        }
    }

    if (error) {
        if (CKTstepDebug) {
            const char *vtype = (CKTmode & MODEDCTRANCURVE) ? "dcval" : "time";
            TTY.err_printf("%s=%15g, %d iters, %s\n", vtype, CKTtime, iterno,
                (error==OK) ? "ok" : "fail");
        }
        return (error);
    }

#ifdef TEST_CLIP
    // for clipping of initial values to supply maxima.
    bool skip_clip = false;
    bool valid_clip = false;
    double amin = 0.0;
    double amax = 0.0;
#endif

    for (;;) {

        check_fpe(true);
        iterno++;

        // Check for inturrupt and handle events while doing DCOP analysis.
        if ((CKTmode & MODEDC) && !(iterno & 0x3)) {
            GP.Checkup();
            if (Sp.GetFlag(FT_INTERRUPT)) {
                error = E_PAUSE;
                break;
            }
        }
        double startTime = OP.seconds();
        error = loadGmin();
        if (error)
            break;

        if (CKTniState & NISHOULDREORDER) {
            error = CKTmatrix->spOrderAndFactor(0,
                CKTcurTask->TSKpivotRelTol, CKTcurTask->TSKpivotAbsTol, 1);
            if (CKTtranTrace > 1)
                TTY.err_printf("Reordering matrix\n");
            CKTstat->STATreorderTime += OP.seconds() - startTime;
            if (error) {
                // can't handle these errors - pass up!
                if (CKTstepDebug) {
                    TTY.err_printf("reorder returned error ");
                    if (CKTmode & MODETRAN)
                        TTY.err_printf("at time=%g", CKTtime);
                    TTY.err_printf("\n");
                }
                break;
            }
            error = check_fpe(false);
            if (error) {
                if (CKTstepDebug)
                    TTY.err_printf("reorder generated FP error\n");
                break;
            }
            CKTniState &= ~NISHOULDREORDER;
            CKTmatrix->spGetStat(&CKTstat->STATmatSize, &CKTstat->STATnonZero,
                &CKTstat->STATfillIn);

            if (CKTmatrix->spDidReorder()) {
                // If we're using Sparse (not KLU) this call will sort
                // the matrix elements into a column-ordered sequence,
                // which is faster to process when the matrix is
                // large.  If using KLU, this is a no-op.
                //
                CKTmatrix->spSortElements();
                if (CKTmatrix->spDataAddressChange()) {
                    // The matrix was sorted, so now all of the pointers in
                    // the device structs are bad and need to be updated.

                    error = resetup();
                    if (error)
                        break;

                    // The old pointers are saved in the device
                    // reversion data.  If we revert, then we have to
                    // run resetup again or bad things happen.
                    //
                    CKTneedsRevertResetup = true;
                }
            }
        }
        else {
            error = CKTmatrix->spFactor();
            double dt = OP.seconds() - startTime;
            CKTstat->STATdecompTime += dt;
            if (!(CKTmode & MODEDC) && (CKTmode & MODETRAN))
                CKTstat->STATtranDecompTime += dt;
            if (error) {
                if (error == E_SINGULAR) {
                    CKTniState |= NISHOULDREORDER;
                    if (CKTstepDebug) {
                        TTY.err_printf("Matrix is singular, iterno=%d\n",
                            iterno);
                    }
                    if (iterno == 1) {
                        // If this is the first iteration, try to
                        // reorder here.  Otherwise, the RHS is
                        // probably too messed up, so just return. 
                        // The caller will try again with a smaller
                        // step, and we will reorder if necessary
                        // then.

                        error = load();
                        if (error) {
                            if (CKTstepDebug)
                                TTY.err_printf("load (3) returned error\n");
                            break;
                        }
                        if (CKTstepDebug)
                            TTY.err_printf("Forced reordering\n");
                        if (CKTtranTrace > 1) {
                            TTY.err_printf(
                                "Factorize failed, trying to reorder\n");
                        }
                        continue;
                    }
                }
                // seems to be singular - pass the bad news up
                if (CKTstepDebug)
                    TTY.err_printf("lufac returned error\n");
                break;
            }
            error = check_fpe(false);
            if (error) {
                if (CKTstepDebug)
                    TTY.err_printf("lufac generated FP error\n");
                break;
            }
        }
#ifdef TEST_DAMP
        if (!OldCKTstate0)
            OldCKTstate0 = new double[CKTnumStates+1];
        for (int i = 0; i < CKTnumStates; i++)
            *(OldCKTstate0 + i) = *(CKTstate0 + i);
#endif

        startTime = OP.seconds();
        CKTmatrix->spSolve(CKTrhs, CKTrhs, 0, 0);
        double dt = OP.seconds() - startTime;;
        CKTstat->STATsolveTime += dt;
        if (!(CKTmode & MODEDC) && (CKTmode & MODETRAN))
            CKTstat->STATtranSolveTime += dt;
        error = check_fpe(false);
        if (error) {
            if (CKTstepDebug)
                TTY.err_printf("solve generated FP error\n");
            break;
        }

#ifdef TEST_VCLIP
        // This block clips the initial values to the power supply
        // voltages, as an aid to convergence.  This only works if
        // there are no controlled sources and no current sources.
        //
        if (!skip_clip && (CKTmode & MODEINITFLOAT) &&
                (CKTmode & (MODEDCOP | MODETRANOP))) {

            if (!valid_clip)
                valid_clip = DEV.getMinMax(this, &amin, &amax);
            if (!valid_clip)
                skip_clip = true;
            else {
                double fct = CKTsrcFact;
                if (fct < 0.1)
                    fct = 0.1;
                double vmin = fct * amin;
                double vmax = fct * amax;

                sCKTnode *node = CKTnodes;
                int size = CKTmatrix->spGetSize(1);
                for (int i = 1; i <= size; i++) {
                    node = node->next;
                    if (!node)
                        break;
                    if (node->type == SP_VOLTAGE) {
                        if (CKTrhs[i] > vmax) {
                            CKTrhs[i] = vmax;
                            CKTnoncon++;
                        }
                        else if (CKTrhs[i] < vmin) {
                            CKTrhs[i] = vmin;
                            CKTnoncon++;
                        }
                    }
                }
            }
        }
#endif

#ifdef TEST_DAMP
#define CKTnodeDamping 1
        if (CKTnodeDamping != 0 && CKTnoncon != 0 &&
                ((CKTmode & MODETRANOP) || (CKTmode & MODEDCOP)) &&
                iterno > 1) {
            double maxdiff = 0.0;
            sCKTnode *node = CKTnodeTab.find(1);
            for ( ; node; node = CKTnodeTab.nextNode(node)) {
                if (node->type == SP_VOLTAGE) {
                    double diff =
                        fabs(CKTrhs[node->number] - CKTrhsOld[node->number]);
                    if (diff > maxdiff)
                        maxdiff = diff;
                }
            }
            if (maxdiff > 10.0) {
                double damp_factor = 10.0/maxdiff;
                if (damp_factor < 0.1)
                    damp_factor = 0.1;
                node = CKTnodeTab.find(1);
                for ( ; node; node = CKTnodeTab.nextNode(node)) {
                    double diff =
                        CKTrhs[node->number] - CKTrhsOld[node->number];
                    CKTrhs[node->number] = CKTrhsOld[node->number] +
                        (damp_factor * diff);
                }
                for (int i = 0; i < CKTnumStates; i++) {
                    double diff = *(CKTstate0 + i) - *(OldCKTstate0 + i);
                    *(CKTstate0 + i) = *(OldCKTstate0 + i) +
                        (damp_factor * diff);
                }
            }
        }
#endif

        double *tmp = CKTrhsOld;
        CKTrhsOld = CKTrhs;
        CKTrhs = tmp;

        *CKTrhs = 0;
        *CKTrhsSpare = 0;
        *CKTrhsOld = 0;

        if (iterno > maxIter) {
            error = E_ITERLIM;
//            IP.logError(0, "Too many iterations without convergence");
            if (CKTstepDebug)
                TTY.err_printf("iterlim exceeded, time = %g\n", CKTtime);
            break;
        }

        if (!jjaccel()) {
            if (CKTmode & (MODEINITFIX | MODEINITFLOAT)) {
                if (CKTnoncon == 0 && iterno != 1) {
                    startTime = OP.seconds();
                    CKTnoncon = NIconvTest();
                    CKTstat->STATcvChkTime += OP.seconds() - startTime;
                }
                else
                    CKTnoncon = 1;
            }
        }

        if (CKTmode & MODEINITFLOAT) {
            if ((CKTmode & MODEDC) && CKThadNodeset) {
                if (ipass)
                    CKTnoncon = ipass;
                ipass = 0;
            }
            if (CKTnoncon == 0) {
                error = OK;
                break;
            }
#if 0
            foo_test(this, iterno);
#endif
            if ((CKTmode & MODEDC) && iterno > 2 &&
                    CKTcurTask->TSKdcMu != 0.5) {

                // I found that by mixing in a little of the last
                // solution, DCOP convergence was improved for some
                // circuits.  In particular, with one post-extraction
                // mixed-mode CMOS circuit, gmin stepping would
                // succeed, but fail without the mixing, or too much
                // mixing.  Thus, the new dcmu parameter.

                int sz = CKTmatrix->spGetSize(1) + 1;
                double A = CKTcurTask->TSKdcMu + 0.5;
                double B = 1.0 - A;
                for (int i = 1; i < sz; i++)
                    CKTrhsOld[i] = A*CKTrhsOld[i] + B*CKTrhs[i];
            }
        }
        else if (CKTmode & MODEINITPRED)
            CKTmode = (CKTmode&(~INITF))|MODEINITFLOAT;
        else if (CKTmode & MODEINITTRAN) {
            if (iterno <= 1)
                CKTniState |= NISHOULDREORDER;
            CKTmode = (CKTmode&(~INITF))|MODEINITFLOAT;
        }
        else if (CKTmode & MODEINITSMSIG)
            CKTmode = (CKTmode&(~INITF))|MODEINITFLOAT;
        else if (CKTmode & MODEINITJCT) {
            CKTmode = (CKTmode&(~INITF))|MODEINITFIX;
            CKTniState |= NISHOULDREORDER;
        }
        else if (CKTmode & MODEINITFIX) {
            if (CKTnoncon == 0)
                CKTmode = (CKTmode&(~INITF))|MODEINITFLOAT;
            ipass = 1;
        }
        else {
            error = E_INTERN;
            if (CKTstepDebug)
                TTY.err_printf("bad initf state\n");
            break;
        }
        CKTnoncon = 0;
        error = load();
        if (error) {
            if (CKTstepDebug)
                TTY.err_printf("load (2) returned error\n");
            break;
        }
        error = check_fpe(false);
        if (error) {
            if (CKTstepDebug)
                TTY.err_printf("load (2) generated FP error\n");
            break;
        }
        if (jjaccel() && CKTnoncon == 0 && (CKTmode & MODEINITFLOAT) &&
                !(CKTmode & (MODEDCOP | MODETRANOP | MODEINITTRAN))) {
            error = OK;
            break;
        }
    }
    CKTstat->STATnumIter += iterno;
    if (!(CKTmode & MODEDC) && (CKTmode & MODETRAN))
        CKTstat->STATtranIter += iterno;
    if (CKTstepDebug) {
        const char *vtype = (CKTmode & MODEDCTRANCURVE) ? "dcval" : "time";
        TTY.err_printf("%s=%15g, %d iters, %s\n", vtype, CKTtime, iterno,
            (error==OK) ? "ok" : "fail");
    }
#ifdef TEST_DAMP
    delete [] OldCKTstate0;
#endif
    if (error == OK)
        DVO.dumpStrobe();
    return (error);
}

