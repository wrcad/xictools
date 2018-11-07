
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

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "trandefs.h"
#include "device.h"
#include "input.h"
#include "output.h"
#include "verilog.h"
#include "simulator.h"
#include "ttyio.h"
#include "sparse/spmatrix.h"


namespace {
    inline void swaprhs(sCKT *ckt)
    {
        double *temp = ckt->CKTrhs;
        ckt->CKTrhs = ckt->CKTrhsOld;
        ckt->CKTrhsOld = temp;
        *ckt->CKTrhs = 0.0;
        *ckt->CKTrhsOld = 0.0;
    }

    int is_trunc_dev(sCKT *ckt)
    {
        sCKTmodGen mgen(ckt->CKTmodels);
        sGENmodel *m;
        while ((m = mgen.next()) != 0) {
            if (DEV.device(m->GENmodType)->flags() & DV_TRUNC)
                return (true);
        }
        return (false);
    }
}


bool
sTRANAN::threadable()
{
    if (TS.t_nointerp)
        return (false);
    if (TRANmode & MODESCROLL)
        return (false);
    if (TRANsegBaseName)
        return (false);
    return (true);
}


int
sTRANAN::init(sCKT *ckt)
{
    if ((TRANmode & MODESCROLL) && TRANsegBaseName) {
        TRANmode &= ~MODESCROLL;
        OP.error(ERR_INFO, "Using segments, \"scroll\" ignored");
    }
    ckt->CKTmode = TRANmode;

    if (!TRANspec)
        return (E_PARMVAL);

    int part = TRANspec->part(ckt->CKTtime);
    TS.t_stop = TRANspec->end(TRANspec->nparts - 1);
    TS.t_step = TRANspec->step(part);
    TS.t_start = TRANspec->tstart;

    ckt->CKTstep = TRANspec->step(0);
    ckt->CKTfinalTime = TS.t_stop;
    ckt->CKTinitTime = TS.t_start;

    ckt->CKTmaxStep = TRANmaxStep;
    ckt->CKTtranDiffs[0] = 1;
    ckt->CKTtranDegree = 0;

    if (TS.t_step <= 0) {
        OP.error(ERR_FATAL, "zero or negative TRAN step given.");
        return (E_PARMVAL);
    }
    if (TS.t_start < 0) {
        OP.error(ERR_FATAL, "negative TRAN start time given.");
        return (E_PARMVAL);
    }
    if (TS.t_stop <= TS.t_start) {
        OP.error(ERR_FATAL, "TRAN end time not larger than start time.");
        return (E_PARMVAL);
    }
    if (TS.t_stop/TS.t_step > 1e9) {
        OP.error(ERR_FATAL, "too many TRAN steps in range, limit 1e9.");
        return (E_PARMVAL);
    }
    if (ckt->CKTmaxStep < 0) {
        OP.error(ERR_FATAL, "maxstep parameter is negative.");
        return (E_PARMVAL);
    }

    TS.t_spice3 = ckt->CKTcurTask->TSKspice3;
    TS.t_nocut = !is_trunc_dev(ckt);

    if (ckt->CKTmaxStep == 0) {
        ckt->CKTmaxStep = (TS.t_stop - TS.t_start)/50;
        // if (TS.t_spice3) {
        if (ckt->CKTcurTask->TSKoldStepLim) {
            // Spice3 chooses the larger of the two choices, which
            // is probably a bug.
            if (TS.t_step > ckt->CKTmaxStep)
                ckt->CKTmaxStep = TS.t_step;
        }
        else {
            // Take a slightly larger value, which may play better with
            // steptype=hitusertp.
            if (TS.t_step < ckt->CKTmaxStep)
                ckt->CKTmaxStep = 1.001*TS.t_step;
        }
    }
    TS.t_delmax = ckt->CKTmaxStep;

    TS.t_nointerp = false;
    TS.t_hitusertp = false;
    if (ckt->CKTvblk) {
        // if there is a verilog block, hit the user time points
        TS.t_hitusertp = true;
        ckt->CKTcurTask->TSKtranStepType = STEP_HITUSERTP;
    }
    else {
        switch (ckt->CKTcurTask->TSKtranStepType) {
        default:
        case STEP_NORMAL:
            break;
        case STEP_HITUSERTP:
            TS.t_hitusertp = true;
            break;
        case STEP_NOUSERTP:
            TS.t_nointerp = true;
            break;
        case STEP_FIXEDSTEP:
            TS.t_fixed_step = TS.t_step;
            TS.t_nointerp = true;
            break;
        }
    }

    TS.t_polydegree = ckt->CKTcurTask->TSKinterpLev;

    TS.t_delmin_given = false;
    if (ckt->CKTcurTask->TSKdelMin > 0)
        TS.t_delmin_given = true;
    else {
        double delmin;
        if (ckt->CKTcurTask->TSKoldStepLim) {
            // SPICE3 values.
            delmin = ckt->CKTmaxStep * 1e-9;
            double T1 = TS.t_stop * 1e-12;
            if (delmin < T1)
                delmin = T1;
        }
        else {
            // 4.2.12: Values here increased by 1e3.
            delmin = ckt->CKTmaxStep * 1e-6;
            // 4.2.13: Why consider t_stop at all?  Gives bad delmin for
            // very long runs.
            // double T1 = TS.t_stop * 1e-9;
            // if (delmin < T1)
            //     delmin = T1;
        }
        ckt->CKTcurTask->TSKdelMin = delmin;
    }
    TS.t_delmin = ckt->CKTcurTask->TSKdelMin;

    TS.t_minbreak_given = false;
    if (ckt->CKTcurTask->TSKminBreak > 0)
        TS.t_minbreak_given = true;
    else
        ckt->CKTcurTask->TSKminBreak = ckt->CKTcurTask->TSKdelMin * 50.0;
    // Beware!  if minBreak is greater than TD of a transmission
    // line, there will be problems.

    return (OK);
}
// End of sTRANAN functions.


int
TRANanalysis::anFunc(sCKT *ckt, int restart)
{
    sTRANAN *job = static_cast<sTRANAN*>(ckt->CKTcurJob);
    sCKTmodGen mgen(ckt->CKTmodels);
    for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
        if (DEV.device(m->GENmodType)->flags() & DV_NOTRAN) {
            OP.error(ERR_FATAL,
                "Transient analysis not possible with device %s.",
                DEV.device(m->GENmodType)->name());
            return (OK);
        }
    }
    ckt->CKTcurrentAnalysis |= DOING_TRAN;

    sTRANint *tran = &job->TS;

    sOUTdata *outd;
    int error;
    if (restart) {

        error = job->init(ckt);
        if (error != OK) {
            ckt->CKTcurrentAnalysis &= ~DOING_TRAN;
            return (error);
        }
        error = job->JOBdc.init(ckt);
        if (error != OK) {
            ckt->CKTcurrentAnalysis &= ~DOING_TRAN;
            return (error);
        }
        if (job->JOBdc.elt(0) && tran->t_nointerp) {
            OP.error(ERR_WARNING,
                "DCsource given, \"set steptype = nousertp\" ignored");
            tran->t_nointerp = false;
        }
        int multip = job->JOBdc.points(ckt);

        if (!job->JOBoutdata) {
            outd = new sOUTdata;
            job->JOBoutdata = outd;
        }
        else
            outd = job->JOBoutdata;
        error = ckt->names(&outd->numNames, &outd->dataNames);
        if (error) {
            delete outd;
            job->JOBoutdata = 0;
            ckt->CKTcurrentAnalysis &= ~DOING_TRAN;
            return (error);
        }

        ckt->newUid(&outd->refName, 0, "time", UID_OTHER);

        outd->numPts = job->points(ckt);

        outd->circuitPtr = ckt;
        outd->analysisPtr = ckt->CKTcurJob;
        outd->analName = ckt->CKTcurJob->JOBname;
        outd->refType = IF_REAL;
        outd->dataType = IF_REAL;
        outd->initValue = tran->t_start;
        outd->finalValue = tran->t_stop;
        outd->step = ckt->CKTstep;

        outd->count = 0;
        job->JOBrun = OP.beginPlot(outd, multip, job->TRANsegBaseName,
            job->TRANsegDelta);
        delete [] outd->dataNames;
        outd->dataNames = 0;
        if (!job->JOBrun) {
            ckt->CKTcurrentAnalysis &= ~DOING_TRAN;
            return (E_TOOMUCH);
        }
    }
    else
        outd = job->JOBoutdata;

    error = job->JOBdc.loop(tran_dcoperation, ckt, restart);
    if (error < 0) {
        if (ckt->CKTmode & MODESCROLL)
            OP.unrollPlot(job->JOBrun);
        // pause
        if (ckt->CKTvblk)
            ckt->CKTvblk->finalize(true);
        ckt->CKTcurrentAnalysis &= ~DOING_TRAN;
        return (error);
    }
    if (ckt->CKTvblk)
        ckt->CKTvblk->finalize(false);

    OP.endPlot(job->JOBrun, false);
    ckt->CKTcurrentAnalysis &= ~DOING_TRAN;
    return (error);
}


// Static private function.
//
int
TRANanalysis::tran_dcoperation(sCKT *ckt, int restart)
{
    sSTATS *stat = ckt->CKTstat;
    sTRANAN *job = static_cast<sTRANAN*>(ckt->CKTcurJob);
    sTRANint *tran = &job->TS;
    int error = 0;
    int afterpause = !restart;
    if (restart) {

        ckt->CKTtime = 0;
        ckt->CKTdelta = 0;
        ckt->CKTbreak = 0;
        ckt->breakInit();
        ckt->breakSet(job->TRANspec->tstart);
        for (int i = 0; i < job->TRANspec->nparts; i++)
            ckt->breakSet(job->TRANspec->end(i));

        // Enable the "tran" functions found in the sources and
        // device expressions.
        ckt->initTranFuncs(tran->t_step, tran->t_stop);

        tran->t_dumpit = false;
        tran->t_firsttime = true;

        tran->t_check = (tran->t_start || !tran->t_nointerp) ?
            tran->t_start : tran->t_stop;

        // set initial conditions
        error = ckt->ic();
        if (error)
            return (error);
        if (ckt->CKTvblk)
            ckt->CKTvblk->initialize();

        if (ckt->CKTmode & MODEUIC) {
            error = ckt->setic();
            if (error)
                return (error);
            // niiter() used to just do this and return
            swaprhs(ckt);
            ckt->CKTmode = (ckt->CKTmode &
                MODESCROLL) | MODEUIC | MODETRANOP | MODEINITJCT;
            error = ckt->load();
        }
        else {
            error = ckt->op(
                (ckt->CKTmode & MODESCROLL) | MODETRANOP | MODEINITJCT,
                (ckt->CKTmode & MODESCROLL) | MODETRANOP | MODEINITFLOAT,
                ckt->CKTcurTask->TSKdcMaxIter);

#ifdef NEWJJDC
            // With the phase-mode approach, we need to carry over the
            // JJ phase and inductor current computed in DCOP.  This
            // is done in model code.

            if (ckt->CKTjjDCphase) {
                // Important!  Zero the voltage of all phase nodes
                // that were computed, as they are numerically the
                // phase.  The actual voltage, for voltage-mode going
                // forward, is zero.

                const sCKTnode *node = ckt->CKTnodeTab.find(1);
                for ( ; node; node = ckt->CKTnodeTab.nextNode(node)) {
                    if (node->phase())
                        ckt->CKTrhsOld[node->number()] = 0.0;
                }
            }
#else
            if (ckt->CKTjjPresent) {
                // With JJ's, maintaining the phase relationship
                // between inductors and JJ's in loops requires that
                // inductor currents and JJ phases start at zero, and
                // evolve to nonzero values under external excitation. 
                // We set the calculated inductor currents to zero to
                // force this.  JJ phases are initially zero at this
                // point already.

                DEV.zeroInductorCurrent(ckt);
            }
#endif
        }
        if (error)
            return (error);

        // Initialize the "history".
        for (int i = 1; i <= ckt->CKTcurTask->TSKmaxOrder + 1; i++) {
            memcpy(ckt->CKTstates[i], ckt->CKTstates[0],
                ckt->CKTnumStates*sizeof(double));
        }

        stat->STATtimePts++;
        ckt->CKTorder = 1;

        if (ckt->CKTinitDelta > 0)
            ckt->CKTdelta = ckt->CKTinitDelta;
        else {
            ckt->CKTdelta = SPMIN(tran->t_delmax, tran->t_step);
            if (!tran->t_nocut)
                ckt->CKTdelta *= 0.1;
        }

        for (int i = 0; i < 7; i++)
            ckt->CKTdeltaOld[i] = ckt->CKTdelta; 
        ckt->CKTsaveDelta = ckt->CKTdelta;

        ckt->CKTmode =
            (ckt->CKTmode & (MODEUIC | MODESCROLL)) | MODETRAN | MODEINITTRAN;
        ckt->CKTag[0] = ckt->CKTag[1] = 0;
        memcpy(ckt->CKTstate1, ckt->CKTstate0,
            ckt->CKTnumStates*sizeof(double));

        if (ckt->CKTtime >= tran->t_start)
            tran->t_dumpit = true;
    }

    int done = false;
    double fctr = 100.0/ckt->CKTfinalTime;
    double loopStartTime = OP.seconds();
    for (;;) {
        double startTime = OP.seconds();
        error = tran->accept(ckt, stat, &done, &afterpause);
        stat->STATtranOutTime += (OP.seconds() - startTime);
        stat->STATtranPctDone = ckt->CKTtime*fctr;
        if (error || done)
            break;

        tran->setbp(ckt);

        // rotate delta vector
        for (int i = 5; i >= 0; i--)
            ckt->CKTdeltaOld[i+1] = ckt->CKTdeltaOld[i];
        ckt->CKTdeltaOld[0] = ckt->CKTdelta;

        // rotate states
        double *temp = ckt->CKTstates[ckt->CKTcurTask->TSKmaxOrder+1];
        for (int i = ckt->CKTcurTask->TSKmaxOrder; i >= 0; i--)
            ckt->CKTstates[i+1] = ckt->CKTstates[i];
        ckt->CKTstates[0] = temp;

        error = tran->step(ckt, stat);
        if (error)
            break;
    }
    stat->STATtranTime += (OP.seconds() - loopStartTime);
    return (error);
}
// End of TRANanalysis functions.


inline void
sTRANint::inc_check(sCKT *ckt)
{
    sTRANAN *job = static_cast<sTRANAN*>(ckt->CKTcurJob);
    int part = job->TRANspec->part(t_check + ckt->CKTcurTask->TSKminBreak);
    t_step = job->TRANspec->step(part);
    t_check += t_step;
}


int
sTRANint::accept(sCKT *ckt, sSTATS *stat, int *done, int *afterpause)
{
    sTRANAN *job = static_cast<sTRANAN*>(ckt->CKTcurJob);
    sOUTdata *outd = job->JOBoutdata;

    // afterpause is true if resuming.  If we paused during
    // interpolation, go back and finish up, otherwise just
    // return.
    //
    int error = OK;
    if (*afterpause) {
        *afterpause = false;
        if (t_hitusertp || t_nointerp)
            return (OK);
    }
    else {
        // Save last rhs vector, this will restore the rhs following
        // a rejected timepoint.
        int sz = ckt->CKTmatrix->spGetSize(1);
        memcpy(ckt->CKTrhsSpare+1, ckt->CKTrhsOld+1, sz*sizeof(double));

        error = ckt->accept();
        // check if current breakpoint is outdated; if so, clear
        if (ckt->CKTtime > *(ckt->CKTbreaks))
            ckt->breakClr();

        stat->STATaccepted++;
        ckt->CKTbreak = 0;
        if (error)
            return (error);
    }

    if (t_hitusertp || t_nointerp) {
        if (t_dumpit) {
            if (ckt->CKTvblk && outd->count)
                ckt->CKTvblk->run_step(outd);
            ckt->dump(ckt->CKTtime, job->JOBrun);
            if (!t_nointerp)
                t_dumpit = false;
        }
        if (!(ckt->CKTmode & MODESCROLL) && ckt->CKTtime >= t_stop - t_delmin)
            OP.set_endit(true);
    }
    else {
        error = interpolate(ckt);
        if (error == E_NOCHANGE)
            OP.set_endit(true);
    }

    if (OP.endit()) {
        OP.set_endit(false);
        *done = true;
        return (OK);
    }

    if (error < 0 || (error = OP.pauseTest(job->JOBrun)) < 0)
        // user requested pause...
        return (error);
    return (OK);
}


void
sTRANint::setbp(sCKT *ckt)
{
    // Breakpoint handling scheme:
    // When a timepoint t is accepted (by CKTaccept), clear all previous
    // breakpoints, because they will never be needed again.
    //
    // t may itself be a breakpoint, or indistinguishably close. DON'T
    // clear t itself; recognise it as a breakpoint and act accordingly
    //
    // if t is not a breakpoint, limit the timestep so that the next
    // breakpoint is not crossed

    ckt->CKTbreak = 0;
    if (ckt->CKTtime == 0.0 || *ckt->CKTbreaks > 0) {
        // are we at a breakpoint, or indistinguishably close?
        if ((ckt->CKTtime == *(ckt->CKTbreaks)) ||
                (*(ckt->CKTbreaks) - ckt->CKTtime <= t_delmin)) {

            // First timepoint after a breakpoint - cut integration order
            // and limit timestep to .1 times minimum of time to next
            // breakpoint, and previous timestep.
            //
            ckt->CKTorder = 1;

            // Keep order=1 for at least two time points.  This seems to
            // improve convergence.
            t_ordcnt = t_spice3 ? 0 : 1;

            double ttmp = *(ckt->CKTbreaks+1) - *(ckt->CKTbreaks);

            if (ckt->CKTsaveDelta < ttmp)
                ttmp = ckt->CKTsaveDelta;
            if (!t_nocut && !ckt->jjaccel())
                ttmp *= 0.1;

            // Have to be careful here.  If the delta is cut too much,
            // if can actually cause nonconvergence since the circuit
            // equations may fail for small delta due to
            // double-precision range problems.  This can be triggered
            // by breakpoints that are too close together, perhaps
            // from an overly "precise" pwl line.  We limit the delta
            // to being no smaller that 0.1 times the last delta.

            double td1 = 0.1 * ckt->CKTsaveDelta;
            if (ttmp < td1)
                ttmp = td1;

            if (ttmp < ckt->CKTdelta)
                ckt->CKTdelta = ttmp;

            // don't want to get below delmin for no reason
            ttmp = 2.0*t_delmin;
            if (ckt->CKTdelta < ttmp)
                ckt->CKTdelta = ttmp;
        }
        else if (ckt->CKTtime + 1.01*ckt->CKTdelta >= *(ckt->CKTbreaks)) {
            double del = *(ckt->CKTbreaks) - ckt->CKTtime;
            if (del > t_delmin) {
                ckt->CKTsaveDelta = ckt->CKTdelta;
                ckt->CKTdelta = del;

                // Set this flag to indicate that the next accepted time point
                // is at a breakpoint.  This is used by the TRA device.
                ckt->CKTbreak = 1;
                if (ckt->CKTtranTrace > 1) {
                    TTY.err_printf("BREAKPOINT at %g delta=%g\n",
                        *(ckt->CKTbreaks), ckt->CKTdelta);
                }
            }
        }
    }

    if (t_hitusertp || t_nointerp) {
        if (ckt->CKTtime + ckt->CKTdelta >= t_check - t_delmin) {
            if (ckt->CKTtime < t_check) {
                double ttmp = t_check - ckt->CKTtime;
                if (ttmp > 2.0*t_delmin) {
                    ckt->CKTsaveDelta = ckt->CKTdelta;
                    ckt->CKTdelta = ttmp;
                    ckt->CKTbreak = 0;
                }
                t_dumpit = true;
            }
            else {
                double delta = t_check + t_step - ckt->CKTtime;
                delta *= 0.5;
                if (ckt->CKTdelta > delta) {
                    ckt->CKTdelta = delta;
                    ckt->CKTbreak = 0;
                }
            }
            inc_check(ckt);
            if (t_nointerp)
                t_check = t_stop;
        }
    }
}


int
sTRANint::step(sCKT *ckt, sSTATS *stat)
{
    /*****
    // This really isn't reliable enough, algorithm needs more work.
    if (t_firsttime && !t_delmin_given) {
        double dm = ckt->computeMinDelta();
        if (dm > t_delmin) {
            t_delmin = dm;
            if (!t_minbreak_given)
                ckt->CKTcurTask->TSKminBreak = 50 * dm;
        }
    }
    *****/

    if (t_firsttime) {
        if (ckt->CKTcurTask->TSKrampUpTime > 0)
            ckt->breakSet(ckt->CKTcurTask->TSKrampUpTime);
    }

// Factor by which the time step shrinks after convergence failure,
// from SPICE2/3.  Make this adjustable?
#define DELTA_FACTOR 0.125

    bool first_time_really = t_firsttime;
    for (;;) {
        double olddelta = ckt->CKTdelta;
        if (ckt->CKTmaxStep < t_delmax)
            t_delmax = ckt->CKTmaxStep;
        if (t_fixed_step > 0.0)
            ckt->CKTdelta = t_fixed_step;
        ckt->CKTtime += ckt->CKTdelta;
        ckt->CKTdeltaOld[0] = ckt->CKTdelta;
        ckt->NIcomCof();
        ckt->predict();
        if (ckt->CKTcurTask->TSKrampUpTime > 0) {
            if (ckt->CKTtime < ckt->CKTcurTask->TSKrampUpTime)
                ckt->CKTsrcFact = ckt->CKTtime/ckt->CKTcurTask->TSKrampUpTime;
            else
                ckt->CKTsrcFact = 1.0;
        }

        // Turn on checking for trapezoidal integration oscillations.
        ckt->CKTtrapCheck =
            ckt->CKTcurTask->TSKtrapCheck &&
            ckt->CKTcurTask->TSKintegrateMethod == TRAPEZOIDAL &&
            ckt->CKTorder == 2;

        int niter = stat->STATnumIter;
        int maxiters = ckt->CKTcurTask->TSKtranMaxIter;
        int cverr = ckt->NIiter(maxiters);
        stat->STATtimePts++;
        int lastiters = stat->STATnumIter - niter;
        stat->STATtranLastIter = lastiters;
        ckt->CKTmode =
            (ckt->CKTmode & (MODEUIC | MODESCROLL)) | MODETRAN | MODEINITPRED;
        double startTime = OP.seconds();

        if (ckt->CKTtranTrace) {
            TTY.err_printf(
                "time=%12g delta=%12g iters= %d order= %2d\n", ckt->CKTtime,
                ckt->CKTdelta, stat->STATtranLastIter, ckt->CKTorder);
        }

        if (t_firsttime) {
            for (int i = 0; i < ckt->CKTnumStates; i++) {
                *(ckt->CKTstate2+i) = *(ckt->CKTstate1+i);
                if (ckt->CKTcurTask->TSKmaxOrder > 1)
                    *(ckt->CKTstate3+i) = *(ckt->CKTstate1+i);
            }
        }
        if (cverr) {
            if (cverr != E_ITERLIM || t_fixed_step > 0.0) {
                if (cverr == E_SINGULAR)
                    ckt->warnSingular();
                return (cverr);
            }
            ckt->CKTtime -= ckt->CKTdelta;
            stat->STATrejected++;
            stat->STATtranIterCut++;
            ckt->CKTdelta *= DELTA_FACTOR;

            if (t_firsttime) {
                ckt->CKTmode = (ckt->CKTmode &
                     (MODEUIC | MODESCROLL)) | MODETRAN | MODEINITTRAN;

                if (ckt->CKTjjPresent && first_time_really) {
                    first_time_really = false;
                    OP.error(ERR_WARNING,
    "The initial transient time-point failed.  With Josephson\n"
    "junctions present, the circuit should be quiescent with sources\n"
    "at the time=0 values, otherwise convergence failure may result.");
                }
            }
            if (ckt->CKTtranTrace > 1) {
                TTY.err_printf(
                    "REJECTED, ORDER CUT: iteration limit reached\n");
            }

            ckt->CKTorder = 1;
            ckt->CKTbreak = 0;

            // Restore rhs to last time-point final.
            int sz = ckt->CKTmatrix->spGetSize(1);
            memcpy(ckt->CKTrhsOld+1, ckt->CKTrhsSpare+1, sz*sizeof(double));
        }
        else {
            if (t_firsttime) {
                t_firsttime = false;
                // no check on first time point
                break;
            }
            if (t_fixed_step > 0)
                return (OK);

            if (ckt->jjaccel()) {
                ckt->CKTdelta = ckt->CKTdevMaxDelta;
                if (ckt->CKTorder < ckt->CKTcurTask->TSKmaxOrder)
                    ckt->CKTorder++;
                break;
            }

            double newd;
            // Note: in Spice3, trunc() would limit newd to 2*delta.
            int error = ckt->trunc(&newd);
            if (error)
                return (error);

            if (ckt->CKTtranTrace > 1)
                TTY.err_printf("LTE delta=%12g\n", newd);

            // If the trapezoid integration check fails, reject the time
            // point, cut the time step and integration order.
            if (ckt->CKTtrapBad) {
                ckt->CKTtime -= ckt->CKTdelta;
                stat->STATrejected++;
                stat->STATtranTrapCut++;
                ckt->CKTdelta *= DELTA_FACTOR;
                ckt->CKTorder = 1;

                // Restore rhs to last time-point final.
                int sz = ckt->CKTmatrix->spGetSize(1);
                memcpy(ckt->CKTrhsOld+1, ckt->CKTrhsSpare+1,
                    sz*sizeof(double));

                // Keep order=1 for at least two time points.  This
                // seems to improve convergence.
                t_ordcnt = t_spice3 ? 0 : 1;
                if (ckt->CKTtranTrace > 1) {
                    TTY.err_printf(
                        "REJECTED, ORDER CUT: TRAP convergence error\n");
                }
                stat->STATtranTsTime += (OP.seconds() - startTime);
                continue;
            }

            if (t_spice3) {
                if (newd > 2.0*ckt->CKTdelta)
                    newd = 2.0*ckt->CKTdelta;
                if (ckt->CKTdevMaxDelta > 0.0 && ckt->CKTdevMaxDelta < newd)
                    newd = ckt->CKTdevMaxDelta;

                // Note: spice2g had .5 here
                if (newd > .9*ckt->CKTdelta) {
                    if (ckt->CKTorder == 1) {
                        if (t_ordcnt)
                            t_ordcnt--;
                        else {
                            ckt->CKTorder++;  
                            if (ckt->CKTorder > ckt->CKTcurTask->TSKmaxOrder)  
                                ckt->CKTorder = ckt->CKTcurTask->TSKmaxOrder;
                            error = ckt->trunc(&newd);
                            if (error)
                                return (error);
                            if (newd > 2.0*ckt->CKTdelta)
                                newd = 2.0*ckt->CKTdelta;
                            if (ckt->CKTdevMaxDelta > 0.0 &&
                                    ckt->CKTdevMaxDelta < newd)
                                newd = ckt->CKTdevMaxDelta;
                            if (newd <= 1.05*ckt->CKTdelta)
                                ckt->CKTorder = 1;
                        }
                    }
                    // time point OK
                    ckt->CKTdelta = newd;
                    if (ckt->CKTdelta > t_delmax)
                        ckt->CKTdelta = t_delmax;
                    break;
                }
                else {
                    if (ckt->CKTtranTrace > 1)
                        TTY.err_printf("REJECTED: new delta too small\n");
                    ckt->CKTtime -= ckt->CKTdelta;
                    stat->STATrejected++;
                    ckt->CKTdelta = newd;

                    // Restore rhs to last time-point final.
                    int sz = ckt->CKTmatrix->spGetSize(1);
                    memcpy(ckt->CKTrhsOld+1, ckt->CKTrhsSpare+1,
                        sz*sizeof(double));
                }
            }
            else {
                // SPICE2G uses 0.5 here, whereas Spice3 uses 0.9. 
                // Jspice3 used 0.5.  We take 0.5, since the slope
                // cutting used in the TRA model can cause
                // timestep-too-small problems, its return is limited
                // to 0.6*delta.  There seems to be a state where this
                // is the return, no matter now small the time step,
                // so if we keep cutting the analysis will fail.
                //
                // Other than this, I have no idea which is the
                // "better" value.

                if (newd > .5*ckt->CKTdelta) {
                    double twodelta = 2.0*ckt->CKTdelta;
                    // Note: Spice3 does not incrememt order larger than 2.

                    // See if we should increment order.
                    //
                    if (ckt->CKTorder < ckt->CKTcurTask->TSKmaxOrder) {
                        if (t_ordcnt)
                            t_ordcnt--;
                        else {
                            ckt->CKTorder++;
                            double newdtmp;
                            error = ckt->trunc(&newdtmp);
                            if (error)
                                return (error);
                            if (ckt->CKTtranTrace > 1) {
                                TTY.err_printf("LTE delta=%12g order= %d\n",
                                    newdtmp, ckt->CKTorder);
                            }
                            if (newdtmp <= 1.05*SPMIN(newd, twodelta))
                                ckt->CKTorder--;
                            else
                                newd = newdtmp;
                        }
                    }
                    // Time point OK.
                    if (newd > twodelta)
                        newd = twodelta;
                    if (ckt->CKTdevMaxDelta > 0.0 && ckt->CKTdevMaxDelta < newd)
                        newd = ckt->CKTdevMaxDelta;
                    if (newd > t_delmax)
                        newd = t_delmax;
                    ckt->CKTdelta = newd;
                    break;
                }
                else {
                    if (ckt->CKTtranTrace > 1)
                        TTY.err_printf("REJECTED: new delta too small\n");
                    ckt->CKTtime -= ckt->CKTdelta;
                    stat->STATrejected++;
                    if (ckt->CKTdevMaxDelta > 0.0 && ckt->CKTdevMaxDelta < newd)
                        newd = ckt->CKTdevMaxDelta;
                    if (newd > t_delmax)
                        newd = t_delmax;
                    ckt->CKTdelta = newd;
                    ckt->CKTbreak = 0;

                    // Restore rhs to last time-point final.
                    int sz = ckt->CKTmatrix->spGetSize(1);
                    memcpy(ckt->CKTrhsOld+1, ckt->CKTrhsSpare+1,
                        sz*sizeof(double));
                }
            }
        }
        if (ckt->CKTdelta <= t_delmin) {
            if (olddelta > t_delmin)
                ckt->CKTdelta = t_delmin;
            else {
                char *ms = ckt->trouble("Timestep too small");
                IP.logError(0, ms);
                delete [] ms;
                return (E_TIMESTEP);
            }
        }
        stat->STATtranTsTime += (OP.seconds() - startTime);
    }
    return (OK);
}


// Previously, all raw timepoint data was saved in the output data
// structures, and interpolation was performed when the analysis was
// complete.  This can take huge amounts of storage if Josephson junctions
// are present.  Here, we (by default) save only data at user time points,
// performing interpolation "on the fly".
//
int
sTRANint::interpolate(sCKT *ckt)
{
    sTRANAN *job = static_cast<sTRANAN*>(ckt->CKTcurJob);
    sOUTdata *outd = job->JOBoutdata;
    if (ckt->CKTmode & MODEUIC) {
        // The first point output does not include the effects of the
        // device initial condition settings.  We skip this, and use
        // the first internal time point value instead.  This will
        // (mosly) avoid a discontinuity at the origin.  There may
        // still be a discontinuity with nonlinear capacitors or
        // inductors.

        if (t_firsttime)
            return (OK);
        if (outd->count == 0 && t_start == 0.0) {
            ckt->dump(t_check, job->JOBrun);
            inc_check(ckt);
            return (OK);
        }
    }

    // use CKTrhs to store the interpolated values, since it is not
    // used for anything - it will be cleared before the next time
    // point.

    double *diff = ckt->CKTtranDiffs;
    int size = ckt->CKTmatrix->spGetSize(1);
    double delta = .5*t_delmin;
    double finalmax = t_stop + delta;
    double finalmin = t_stop - delta;
    double time = ckt->CKTtime + delta;

    int degree = t_polydegree;
    if (degree > ckt->CKTcurTask->TSKmaxOrder)
        degree = ckt->CKTcurTask->TSKmaxOrder;

    if (degree <= 1) {

        double c2 = ckt->CKTdeltaOld[0];

        for (; t_check <= time; inc_check(ckt)) {
            int error;
            if ((error = OP.pauseTest(job->JOBrun)) < 0)
                // pause
                return (error);

            double d1 = ckt->CKTtime - t_check;
            double d2 = d1 - c2;

            // first order extrapolation factors
            diff[0] = d2/(-c2);
            diff[1] = d1/(c2);
            ckt->CKTtranDegree = 1;
 
            for (int i = 1; i <= size; i++) {
                ckt->CKTrhs[i] =
                    diff[0]*ckt->CKTsols[0][i] +
                    diff[1]*ckt->CKTsols[1][i];
            }
            if ((ckt->CKTmode & MODESCROLL) || t_check < finalmax) {
                swaprhs(ckt);
                ckt->dump(t_check, job->JOBrun);
                swaprhs(ckt);
                if (!(ckt->CKTmode & MODESCROLL) && finalmin < t_check)
                    return (E_NOCHANGE);
            }
            else
                return (E_NOCHANGE);
        }
    }
    else if (degree == 2) {

        double c2 = ckt->CKTdeltaOld[0];
        double c3 = c2 + ckt->CKTdeltaOld[1];

        for (; t_check <= time; inc_check(ckt)) {
            int error;
            if ((error = OP.pauseTest(job->JOBrun)) < 0)
                // pause
                return (error);

            double d1 = ckt->CKTtime - t_check;
            double d2 = d1 - c2;
            double d3 = d1 - c3;

            // second order extrapolation factors
            diff[0] = d2*d3/((c2)*(c3));
            diff[1] = d1*d3/((c2)*(c2-c3));
            diff[2] = d1*d2/((c3)*(c3-c2));
            ckt->CKTtranDegree = 2;
    
            for (int i = 1; i <= size; i++) {
                ckt->CKTrhs[i] =
                    diff[0]*ckt->CKTsols[0][i] +
                    diff[1]*ckt->CKTsols[1][i] +
                    diff[2]*ckt->CKTsols[2][i];
            }
            if ((ckt->CKTmode & MODESCROLL) || t_check < finalmax) {
                swaprhs(ckt);
                ckt->dump(t_check, job->JOBrun);
                swaprhs(ckt);
                if (!(ckt->CKTmode & MODESCROLL) && finalmin < t_check)
                    return (E_NOCHANGE);
            }
            else
                return (E_NOCHANGE);
        }
    }
    else if (degree >= 3) {

        double c2 = ckt->CKTdeltaOld[0];
        double c3 = c2 + ckt->CKTdeltaOld[1];
        double c4 = c3 + ckt->CKTdeltaOld[2];

        for (; t_check <= time; inc_check(ckt)) {
            int error;
            if ((error = OP.pauseTest(job->JOBrun)) < 0)
                // pause
                return (error);

            double d1 = ckt->CKTtime - t_check;
            double d2 = d1 - c2;
            double d3 = d1 - c3;
            double d4 = d1 - c4;

            // third order extrapolation factors
            diff[0] = d2*d3*d4/((-c2)*(c3)*(c4));
            diff[1] = d1*d3*d4/((c2)*(c2-c3)*(c2-c4));
            diff[2] = d1*d2*d4/((c3)*(c3-c2)*(c3-c4));
            diff[3] = d1*d2*d3/((c4)*(c4-c2)*(c4-c3));
            ckt->CKTtranDegree = 3;
    
            for (int i = 1; i <= size; i++) {
                ckt->CKTrhs[i] =
                    diff[0]*ckt->CKTsols[0][i] +
                    diff[1]*ckt->CKTsols[1][i] +
                    diff[2]*ckt->CKTsols[2][i] +
                    diff[3]*ckt->CKTsols[3][i];
            }
            if ((ckt->CKTmode & MODESCROLL) || t_check < finalmax) {
                swaprhs(ckt);
                ckt->dump(t_check, job->JOBrun);
                swaprhs(ckt);
                if (!(ckt->CKTmode & MODESCROLL) && finalmin < t_check)
                    return (E_NOCHANGE);
            }
            else
                return (E_NOCHANGE);
        }
    }
    if (!(ckt->CKTmode & MODESCROLL) && t_check >= finalmax)
        return (E_NOCHANGE);
    return (OK);
}

