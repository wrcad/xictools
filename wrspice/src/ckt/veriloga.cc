
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

#include "device.h"
#include "misc.h"
#include "ttyio.h"
#include "miscutil/random.h"


//
// The functions below are for Verilog-A model support.  These implement
// various Verilog-A functions.
//

// Make sure that this module is linked by referencing this.
bool CKThasVA;

// Support for the Verilog-A ddt operator.  The n is the offset into
// the state table set up uniquely for the calling function.
//
double
sCKT::va_ddt(int n, double x, double)
{
    if (CKTmode & (MODEDC | MODEAC))
        return (0.0);
    if (CKTag[0] == 0.0)
        return (0.0);

    if (CKTmode & MODEINITTRAN)
        *(CKTstates[1] + n) = x;
    double ceq = find_ceq(n);
    *(CKTstates[0] + n) = x;
    double ddt = ceq + x*CKTag[0];
    *(CKTstates[0] + n+1) = ddt;
    if (CKTmode & MODEINITTRAN)
        *(CKTstates[1] + n+1) = ddt;
    return (ddt);
}


// Support for the Verilog-A idt operator.  The n is the offset into
// the state table set up uniquely for the calling function.
//
double
sCKT::va_idt(int n, double x, double icval, bool reset, double)
{
    if (reset) {
        for (int i = 0; i < 7; i++) {
            if (!CKTstates[i])
                break;
            *(CKTstates[i] + n) = 0.0;
            *(CKTstates[i] + n+1) = 0.0;
        }
        return (icval);
    }

    double ceq = find_ceq(n);
    if (CKTag[0] == 0.0)
        return (icval);
    *(CKTstates[0] + n+1) = x;
    double q = (x - ceq)/CKTag[0]; 
    *(CKTstates[0] + n) = q;
    return (q + icval);
}


// Support for the Verilog-A $bound_step function.
//
void
sCKT::va_boundStep(double maxstep)
{
    if (CKTdevMaxDelta == 0.0 || maxstep < CKTdevMaxDelta)
        CKTdevMaxDelta = maxstep;
}


// Support for the Verilog-A analysis function.
//
bool
sCKT::va_analysis(const char *tok)
{
    if (lstring::cieq(tok, "ac")) {
        // .AC analysis (or pz, disto, noise, ac sens, ac tf)
        return (CKTcurrentAnalysis & DOING_AC);
    }
    if (lstring::cieq(tok, "dc")) {
        // .OP or .DC analysis (or dc sens, dc tf)
        return (CKTcurrentAnalysis & (DOING_DCOP | DOING_TRCV));
    }
    if (lstring::cieq(tok, "noise")) {
        // .NOISE analysis
        return (CKTcurrentAnalysis & DOING_NOISE);
    }
    if (lstring::cieq(tok, "tran")) {
        // .TRAN analysis
        return (CKTcurrentAnalysis & DOING_TRAN);
    }
    if (lstring::cieq(tok, "ic")) {
        // The initial-condition analysis that preceeds a transient
        // analysis.
        return (CKTmode & MODETRANOP);
    }
    if (lstring::cieq(tok, "static")) {
        // Any equilibrium point calculation, including a DC analysis
        // as well as those that precede another analysis, such as the
        // DC analysis that precedes an AC or noise analysis, or the
        // IC analysis that precedes a transient analysis.
        return ((CKTmode & MODEDC) && (CKTmode & MODEINITFLOAT));
    }
    if (lstring::cieq(tok, "nodeset")) {
        // The phase during an equilibrium point calculation where
        // nodesets are forced.
        return ((CKTmode & MODEDC) && !(CKTmode & MODEINITFLOAT));
    }
    return (false);
}


/******
 * From the Verilog-ams manual, truth table for initial_step and final_step.

                      DCOP  Sweep     TRAN        AC          NOISE
                      OP    d1 d2 dN  OP  p1  pN  OP  p1  pN  OP  p1  pN
initial_step()        1     1  0  0   1   0   0   1   0   0   1   0   0
initial_step("ac")    0     0  0  0   0   0   0   1   0   0   0   0   0
initial_step("noise") 0     0  0  0   0   0   0   0   0   0   1   0   0
initial_step("tran")  0     0  0  0   1   0   0   0   0   0   0   0   0
initial_step("dc")    1     1  0  0   0   0   0   0   0   0   0   0   0
initial_step(unknown) 0     0  0  0   0   0   0   0   0   0   0   0   0
final_step()          1     0  0  1   0   0   1   0   0   1   0   0   1
final_step("ac")      0     0  0  0   0   0   0   0   0   1   0   0   0
final_step("noise")   0     0  0  0   0   0   0   0   0   0   0   0   1
final_step("tran")    0     0  0  0   0   0   1   0   0   0   0   0   0
final_step("dc")      1     0  0  1   0   0   0   0   0   0   0   0   0
final_step(unknown)   0     0  0  0   0   0   0   0   0   0   0   0   0

ADMS only implements the non-argument syntax, user can call the $analysis
function in the block if needed.
******/

namespace {
    bool atend(double val, double start, double end, bool is_start)
    {
        double delta = (end - start)*1e-9;
        if (is_start) {
            if (end > start)
                return (val < start + delta);
            if (end < start)
                return (val > start - delta);
            return (val == start);
        }
        else {
            if (end > start)
                return (val > end - delta);
            if (end < start)
                return (val < end + delta);
            return (val == end);
        }
    }
}


// The logic for initial_step global event or block.
// The ADMS parser currently does not handle the analysis list argument.
//
bool
sCKT::va_initial_step()
{
    if (CKTcurrentAnalysis & DOING_DCOP)
        return (true);
    else if (CKTcurrentAnalysis & DOING_TRCV)
        return (atend(CKTtime, CKTinitV1, CKTfinalV1, true));
    else if (CKTcurrentAnalysis & DOING_TRAN)
        return (atend(CKTtime, 0.0, CKTfinalTime, true));
    else if (CKTcurrentAnalysis & DOING_AC) {
        return ((CKTmode & MODEDCOP) ||
            atend(CKTomega, CKTinitFreq, CKTfinalFreq, true));
    }
    return (false);
}


// The logic for final_step global event or block.
// The ADMS parser currently does not handle the analysis list argument.
//
bool
sCKT::va_final_step()
{
    if (CKTcurrentAnalysis & DOING_DCOP)
        return (true);
    else if (CKTcurrentAnalysis & DOING_TRCV)
        return (atend(CKTtime, CKTinitV1, CKTfinalV1, false));
    else if (CKTcurrentAnalysis & DOING_TRAN)
        return (atend(CKTtime, 0.0, CKTfinalTime, false));
    else if (CKTcurrentAnalysis & DOING_AC)
        return (atend(CKTomega, CKTinitFreq, CKTfinalFreq, false));
    return (false);
}


// Support for the Verilog-A absdelay function.
//XXX implement me
//
double
sCKT::va_absdelay(double var, double dtime)
{
    (void)dtime;
    TTY.err_printf("delay/absdelay not implemented\n");
    return (var);
}


int
sCKT::va_random(int seed)
{
    if (seed != Rnd.cur_seed())
        Rnd.seed(seed);
    return (Rnd.random_int());
}


// The distribution functions were added to support the rdist_normal
// calls in Eby Friedman's MTJ model.  To make this work, we have to
// make the output correlate with time, much like the gaussian noise
// source.  Here we assume a "fresh" random value at every transient
// analysis output point.  For other than transient analysis, only one
// random value is generated, and this value is returned on all calls
// during the analysis.
//
// Each random function call uses four state-vector values:  the
// present, next and previous random values, and a time value which is
// an integer multiple of tran steps, and is always larger than or
// equal to the simulation time.  The random value returned is
// linearly interpolated from these values.  The previous random value
// is kept in case we need to revert to an earlier time point due to
// non-convergence, which would be in the previous domain.


// Main distribution function, takes care of the correlation logic.
//
double
sCKT::va_rdist(int seed, double d1, double d2, double dt, int indx,
    double(*rfunc)(double, double))
{
    // This assumes that the state vector is clear when we see it
    // initially.
    if (!CKTstates[0]) {
        // probably called from xxxtopo.cc
        return (0.0);
    }
    double *vals = CKTstates[0] + indx;
    if (CKTmode & (MODETRAN | MODETRANOP)) {
        // We're doing transient analysis.  Set the initial random
        // values on the initial call.  State vector fields are:
        // vals[3]      Time of next random value, a multiple of dt
        //              and >= time.
        // vals[2]      The previous random value (for vals[3] - 2*dt).
        // vals[1]      The random value for time = vals[3].
        // vals[0]      The ransom value for time = vals[3] - dt.

        if (dt <= 1e2*CKTcurTask->TSKdelMin)
            dt = CKTstep;
        if (vals[3] == 0.0) {
            vals[3] = dt;
            // Prevent stepping more than a lattice increment.
            if (CKTmaxStep > vals[3])
                CKTmaxStep = vals[3];
            if (seed != Rnd.cur_seed())
                Rnd.seed(seed);
            vals[0] = (*rfunc)(d1, d2);
            vals[1] = (*rfunc)(d1, d2);
            vals[2] = vals[0];
        }
        else if (CKTmode & MODEINITPRED) {
            // Start of a new time step, safe to generate a new random
            // value here.

            if (CKTtime > vals[3]) {
                // Rotate in a new random value.

                if (seed != Rnd.cur_seed())
                    Rnd.seed(seed);
                vals[2] = vals[0];
                vals[0] = vals[1];
                vals[1] = (*rfunc)(d1, d2);
                while (CKTtime > vals[3])
                    vals[3] += dt;
            }
        }
        double A = (vals[3] - CKTtime)/dt;
        if (A > 1.0) {
            // Revert!
            vals[1] = vals[0];
            vals[0] = vals[2];
            vals[3] -= dt;
            A -= 1.0;
        }
        return (A*vals[0] + (1.0 - A)*vals[1]);
    }

    // Here we're not doing transient analysis.  Generate a random
    // value on the first call, and return this value only.

    if (vals[3] == 0.0) {
        vals[3] = 1.0;
        if (seed != Rnd.cur_seed())
            Rnd.seed(seed);
        vals[0] = (*rfunc)(d1, d2);
    }
    return (vals[0]);
}


namespace {
    double uniform(double start, double end)
    {
        return (start + (end - start)*Rnd.random());
    }

    double normal(double mean, double stddev)
    {
        return (mean + stddev*Rnd.gauss());
    }

    double exponential(double mean, double)
    {
        return (Rnd.dst_exponential(mean));
    }

    double poisson(double mean, double)
    {
        return (Rnd.dst_poisson(mean));
    }

    double chisq(double dof, double)
    {
        return (Rnd.dst_chisq(dof));
    }

    double tdist(double dof, double)
    {
        return (Rnd.dst_tdist(dof));
    }

    double erlang(double mean, double k)
    {
        return (Rnd.dst_erlang(mean, k));
    }
}


double
sCKT::va_rdist_uniform(int seed, double start, double end, double dt, int indx)
{
    return (va_rdist(seed, start, end, dt, indx, &uniform));
}


double
sCKT::va_rdist_normal(int seed, double mean, double stddev, double dt, int indx)
{
    return (va_rdist(seed, mean, stddev, dt, indx, &normal));
}


double
sCKT::va_rdist_exponential(int seed, double mean, double dt, int indx)
{
    return (va_rdist(seed, mean, 0.0, dt, indx, &exponential));
}


double
sCKT::va_rdist_poisson(int seed, double mean, double dt, int indx)
{
    return (va_rdist(seed, mean, 0.0, dt, indx, &poisson));
}


double
sCKT::va_rdist_chi_square(int seed, double dof, double dt, int indx)
{
    return (va_rdist(seed, dof, 0.0, dt, indx, &chisq));
}


double
sCKT::va_rdist_t(int seed, double dof, double dt, int indx)
{
    return (va_rdist(seed, dof, 0.0, dt, indx, &tdist));
}


double
sCKT::va_rdist_erlang(int seed, double k, double mean, double dt, int indx)
{
    return (va_rdist(seed, mean, k, dt, indx, &erlang));
}

