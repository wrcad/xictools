
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
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "simulator.h"
#include "parser.h"
#include "runop.h"
#include "graph.h"
#include "input.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "toolbar.h"
#include "output.h"
#include "rundesc.h"
#include "device.h"
#include "spnumber/spnumber.h"


//
// The stop command and related, general support for runops.
//

// keywords
const char *kw_measure = "measure";
const char *kw_stop   = "stop";
const char *kw_after  = "after";
const char *kw_at     = "at";
const char *kw_before = "before";
const char *kw_when   = "when";
const char *kw_active = "active";
const char *kw_inactive = "inactive";
const char *kw_call   = "call";


// Set up a measure runop.
//
void
CommandTab::com_measure(wordlist *wl)
{
    OP.measureCmd(wl);
}


// Set up a stop runop.
//
void
CommandTab::com_stop(wordlist *wl)
{
    OP.stopCmd(wl);
}


// Step a number of output increments.
//
void
CommandTab::com_step(wordlist *wl)
{
    int n;
    if (wl)
        n = atoi(wl->wl_word);
    else
        n = 1;
    OP.runops()->set_step_count(n);
    OP.runops()->set_num_steps(n);
    Sp.Simulate(SIMresume, 0);
}


// Print out the currently active breakpoints and traces.
//
void
CommandTab::com_status(wordlist*)
{
    OP.statusCmd(0);
}


// Delete runops, many possibilities see below.
//
void
CommandTab::com_delete(wordlist *wl)
{
    OP.deleteCmd(wl);
}
// End of CommandTab functions.


void
IFoutput::measureCmd(wordlist *wl)
{
    char *str = wordlist::flatten(wl);
    char *errstr = 0;
    sRunopMeas *m = new sRunopMeas(str, &errstr);
    if (errstr) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "Measure: %s\n", errstr);
        delete [] errstr;
        delete m;
    }
    else {
        sFtCirc *curcir = Sp.CurCircuit();
        m->set_number(o_runops->new_count());
        m->set_active(true);
        if (CP.GetFlag(CP_INTERACTIVE) || !curcir) {
            if (o_runops->measures()) {
                sRunopMeas *td = o_runops->measures();
                for ( ; td->next(); td = td->next())
                    ;
                td->set_next(m);
            }
            else
                o_runops->set_measures(m);
        }
        else {
            if (curcir->measures()) {
                sRunopMeas *td = curcir->measures();
                for ( ; td->next(); td = td->next())
                    ;
                td->set_next(m);
            }
            else
                curcir->set_measures(m);
        }
        ToolBar()->UpdateTrace();
    }
}


void
IFoutput::stopCmd(wordlist *wl)
{
    char *str = wordlist::flatten(wl);
    char *errstr = 0;
    sRunopStop *m = new sRunopStop(str, &errstr);
    if (errstr) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "Stop: %s\n", errstr);
        delete [] errstr;
        delete m;
    }
    else {
        sFtCirc *curcir = Sp.CurCircuit();
        m->set_number(o_runops->new_count());
        m->set_active(true);
        if (CP.GetFlag(CP_INTERACTIVE) || !curcir) {
            if (o_runops->stops()) {
                sRunopStop *td = o_runops->stops();
                for ( ; td->next(); td = td->next())
                    ;
                td->set_next(m);
            }
            else
                o_runops->set_stops(m);
        }
        else {
            if (curcir->stops()) {
                sRunopStop *td = curcir->stops();
                for ( ; td->next(); td = td->next())
                    ;
                td->set_next(m);
            }
            else
                curcir->set_stops(m);
        }
        ToolBar()->UpdateTrace();
    }
}


void
IFoutput::statusCmd(char **ps)
{
    if (ps)
        *ps = 0;
    const char *msg = "No runops are in effect.\n";
    if (!o_runops->isset() &&
            (!Sp.CurCircuit() || !Sp.CurCircuit()->runops().isset())) {
        if (!ps)
            TTY.send(msg);
        else
            *ps = lstring::copy(msg);
        return;
    }
    char **t = ps;
    sRunopDb *db = Sp.CurCircuit() ? &Sp.CurCircuit()->runops() : 0;

    ROgen<sRunopSave> svgen(o_runops->saves(), db ? db->saves() : 0);
    for (sRunopSave *d = svgen.next(); d; d = svgen.next())
        d->print(t);

    ROgen<sRunopTrace> tgen(o_runops->traces(), db ? db->traces() : 0);
    for (sRunopTrace *d = tgen.next(); d; d = tgen.next())
        d->print(t);

    ROgen<sRunopIplot> igen(o_runops->iplots(), db ? db->iplots() : 0);
    for (sRunopIplot *d = igen.next(); d; d = igen.next())
        d->print(t);

    ROgen<sRunopMeas> mgen(o_runops->measures(), db ? db->measures() : 0);
    for (sRunopMeas *d = mgen.next(); d; d = mgen.next())
        d->print(t);

    ROgen<sRunopStop> sgen(o_runops->stops(), db ? db->stops() : 0);
    for (sRunopStop *d = sgen.next(); d; d = sgen.next())
        d->print(t);
}


// Delete breakpoints and traces. Usage is:
//   delete [[in]active][all | stop | trace | iplot | save | number] ...
// With inactive true (args to right of keyword), the runop is
// cleared only if it is inactive.
//
void
IFoutput::deleteCmd(wordlist *wl)
{
    if (!wl) {
        // Find the earlist runop.
        sRunop *d = o_runops->saves();
        if (!d ||
            (o_runops->traces() && d->number() > o_runops->traces()->number()))
            d = o_runops->traces();
        if (!d ||
            (o_runops->iplots() && d->number() > o_runops->iplots()->number()))
            d = o_runops->iplots();
        if (!d ||
            (o_runops->measures() && d->number() > o_runops->measures()->number()))
            d = o_runops->measures();
        if (!d ||
            (o_runops->stops() && d->number() > o_runops->stops()->number()))
            d = o_runops->stops();

        if (Sp.CurCircuit()) {
            sRunopDb &db = Sp.CurCircuit()->runops();
            if (!d || (db.saves() && d->number() > db.saves()->number()))
                d = db.saves();
            if (!d || (db.traces() && d->number() > db.traces()->number()))
                d = db.traces();
            if (!d || (db.iplots() && d->number() > db.iplots()->number()))
                d = db.iplots();
            if (!d || (db.measures() && d->number() > db.measures()->number()))
                d = db.measures();
            if (!d || (db.stops() && d->number() > db.stops()->number()))
                d = db.stops();
        }
        if (!d) {
            TTY.printf("No runops are in effect.\n");
            return;
        }
        d->print(0);
        if (CP.GetFlag(CP_NOTTYIO))
            return;
        if (!TTY.prompt_for_yn(true, "delete [y]? "))
            return;

        if (d == o_runops->saves())
            o_runops->set_saves(o_runops->saves()->next());
        else if (d == o_runops->traces())
            o_runops->set_traces(o_runops->traces()->next());
        else if (d == o_runops->iplots())
            o_runops->set_iplots(o_runops->iplots()->next());
        else if (d == o_runops->measures())
            o_runops->set_measures(o_runops->measures()->next());
        else if (d == o_runops->stops())
            o_runops->set_stops(o_runops->stops()->next());
        else if (Sp.CurCircuit()) {
            sRunopDb &db = Sp.CurCircuit()->runops();
            if (d == db.saves())
                db.set_saves(db.saves()->next());
            else if (d == db.traces())
                db.set_traces(db.traces()->next());
            else if (d == db.iplots())
                db.set_iplots(db.iplots()->next());
            else if (d == db.measures())
                db.set_measures(db.measures()->next());
            else if (d == db.stops())
                db.set_stops(db.stops()->next());
        }
        ToolBar()->UpdateTrace();
        d->destroy();
        return;
    }
    bool inactive = false;
    for (; wl; wl = wl->wl_next) {
        if (lstring::eq(wl->wl_word, kw_inactive)) {
            inactive = true;
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_active)) {
            inactive = false;
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_all)) {
            deleteRunop(DF_ALL, inactive, -1);
            return;
        }
        if (lstring::eq(wl->wl_word, kw_save)) {
            deleteRunop(DF_SAVE, inactive, -1);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_trace)) {
            deleteRunop(DF_TRACE, inactive, -1);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_iplot)) {
            deleteRunop(DF_IPLOT, inactive, -1);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_measure)) {
            deleteRunop(DF_MEASURE, inactive, -1);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_stop)) {
            deleteRunop(DF_STOP, inactive, -1);
            continue;
        }

        char *s;
        // look for range n1-n2
        if ((s = strchr(wl->wl_word, '-')) != 0) {
            if (s == wl->wl_word || !*(s+1)) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "badly formed range %s.\n",
                    wl->wl_word);
                continue;
            }
            char *t;
            for (t = wl->wl_word; *t; t++) {
                if (!isdigit(*t) && *t != '-') {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "badly formed range %s.\n",
                        wl->wl_word);
                    break;
                }
            }
            if (*t)
                continue;
            int n1 = atoi(wl->wl_word);
            s++;
            int n2 = atoi(s);
            if (n1 > n2) {
                int nt = n1;
                n1 = n2;
                n2 = nt;
            }
            while (n1 <= n2) {
                deleteRunop(DF_ALL, inactive, n1);
                n1++;
            }
            continue;
        }
                
        for (s = wl->wl_word; *s; s++) {
            if (!isdigit(*s)) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s isn't a runops number.\n",
                    wl->wl_word);
                break;
            }
        }
        if (*s)
            continue;
        int i = atoi(wl->wl_word);
        deleteRunop(DF_ALL, inactive, i);
    }
    ToolBar()->UpdateTrace();
}


void
IFoutput::initRunops(sRunDesc *run)
{
    if (!run)
        return;
    sRunopDb *db = run->circuit() ? &run->circuit()->runops() : 0;
    // called at beginning of run
    if (o_runops->step_count() != o_runops->num_steps()) {
        // left over from last run
        o_runops->set_step_count(0);
        o_runops->set_num_steps(0);
    }

    ROgen<sRunopIplot> igen(o_runops->iplots(), db ? db->iplots() : 0);
    for (sRunopIplot *d = igen.next(); d; d = igen.next()) {
        if (d->type() == RO_DEADIPLOT) {
            // user killed the window
            if (d->graphid())
                GP.DestroyGraph(d->graphid());
            d->set_type(RO_IPLOT);
        }
        if (run->check() && run->check()->out_mode == OutcCheckSeg)
            d->set_reuseid(d->graphid());
        if (!(run->check() && (run->check()->out_mode == OutcCheckMulti ||
                run->check()->out_mode == OutcLoop))) {
            d->set_graphid(0);
        }
    }

    ROgen<sRunopMeas> mgen(o_runops->measures(), db ? db->measures() : 0);
    for (sRunopMeas *d = mgen.next(); d; d = mgen.next()) {
        if (run->job()->JOBtype != d->analysis())
            continue;
        d->reset(run->runPlot());
    }

    ROgen<sRunopStop> sgen(o_runops->stops(), db ? db->stops() : 0);
    for (sRunopStop *d = sgen.next(); d; d = sgen.next()) {
        if (run->job()->JOBtype != d->analysis())
            continue;
        d->reset();
    }
}


void
IFoutput::setRunopActive(int dbnum, bool state)
{
    sRunopDb *db = Sp.CurCircuit() ? &Sp.CurCircuit()->runops() : 0;

    ROgen<sRunopSave> vgen(o_runops->saves(), db ? db->saves() : 0);
    for (sRunopSave *d = vgen.next(); d; d = vgen.next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }

    ROgen<sRunopTrace> tgen(o_runops->traces(), db ? db->traces() : 0);
    for (sRunopTrace *d = tgen.next(); d; d = tgen.next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }

    ROgen<sRunopIplot> igen(o_runops->iplots(), db ? db->iplots() : 0);
    for (sRunopIplot *d = igen.next(); d; d = igen.next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }

    ROgen<sRunopMeas> mgen(o_runops->measures(), db ? db->measures() : 0);
    for (sRunopMeas *d = mgen.next(); d; d = mgen.next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }

    ROgen<sRunopStop> sgen(o_runops->stops(), db ? db->stops() : 0);
    for (sRunopStop *d = sgen.next(); d; d = sgen.next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }
}


void
IFoutput::deleteRunop(int which, bool inactive, int num)
{
    sRunopDb *db = Sp.CurCircuit() ? &Sp.CurCircuit()->runops() : 0;

    if (which & DF_SAVE) {
        sRunopSave *dlast = 0, *dnext;
        for (sRunopSave *d = o_runops->saves(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    o_runops->set_saves(dnext);
                else
                    dlast->set_next(dnext);
                d->destroy();
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (db) {
            dlast = 0;
            for (sRunopSave *d = db->saves(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db->set_saves(dnext);
                    else
                        dlast->set_next(dnext);
                    d->destroy();
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
    if (which & DF_TRACE) {
        sRunopTrace *dlast = 0, *dnext;
        for (sRunopTrace *d = o_runops->traces(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    o_runops->set_traces(dnext);
                else
                    dlast->set_next(dnext);
                d->destroy();
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (db) {
            dlast = 0;
            for (sRunopTrace *d = db->traces(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db->set_traces(dnext);
                    else
                        dlast->set_next(dnext);
                    d->destroy();
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
    if (which & DF_IPLOT) {
        sRunopIplot *dlast = 0, *dnext;
        for (sRunopIplot *d = o_runops->iplots(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    o_runops->set_iplots(dnext);
                else
                    dlast->set_next(dnext);
                d->destroy();
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (db) {
            dlast = 0;
            for (sRunopIplot *d = db->iplots(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db->set_iplots(dnext);
                    else
                        dlast->set_next(dnext);
                    d->destroy();
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
    if (which & DF_MEASURE) {
        sRunopMeas *dlast = 0, *dnext;
        for (sRunopMeas *d = o_runops->measures(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    o_runops->set_measures(dnext);
                else
                    dlast->set_next(dnext);
                d->destroy();
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (db) {
            dlast = 0;
            for (sRunopMeas *d = db->measures(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db->set_measures(dnext);
                    else
                        dlast->set_next(dnext);
                    d->destroy();
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
    if (which & DF_STOP) {
        sRunopStop *dlast = 0, *dnext;
        for (sRunopStop *d = o_runops->stops(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    o_runops->set_stops(dnext);
                else
                    dlast->set_next(dnext);
                d->destroy();
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (db) {
            dlast = 0;
            for (sRunopStop *d = db->stops(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db->set_stops(dnext);
                    else
                        dlast->set_next(dnext);
                    d->destroy();
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
}


// Run runops, measures, and margin analysis tests.
//
void
IFoutput::checkRunops(sRunDesc *run, double ref)
{
    if (!run)
        return;
    sCHECKprms *chk = run->check();
    if (chk && chk->out_mode == OutcCheck) {
        // Only the current analysis point is saved.

        sRunopDb *db = run->circuit() ? &run->circuit()->runops() : 0;

        bool tflag = true;
        ROgen<sRunopTrace> tgen(o_runops->traces(), db ? db->traces() : 0);
        for (sRunopTrace *d = tgen.next(); d; d = tgen.next())
            d->print_trace(run->runPlot(), &tflag, run->pointsSeen());

/*XXX handle these somehow?
// can't do actual measurements since we don't have data, but
// process call/print and, maybe create new vector with expression
// result.
        bool measures_done = true;
        bool measure_queued = false;
        ROgen<sRunopMeas> mgen(o_runops->measures(), db ? db->measures() : 0);
        for (sRunopMeas *d = mgen.next(); d; d = mgen.next()) {
            if (run->anType() != d->analysis())
                continue;
            if (!d->check_measure(run))
                measures_done = false;
            if (d->measure_queued())
                measure_queued = true;
        }

        if (measure_queued) {
            run->segmentizeVecs();
            mgen = ROgen<sRunopMeas>(o_runops->measures(),
                db ? db->measures() : 0);
            for (sRunopMeas *d = mgen.next(); d; d = mgen.next()) {
                if (run->anType() != d->analysis())
                    continue;
                if (!d->do_measure(run))
                    measures_done = false;
            }
            run->unsegmentizeVecs();
        }
        if (measures_done) {
            // Reset stop flags so analysis can be continued.
            mgen = ROgen<sRunopMeas>(o_runops->measures(),
                db ? db->measures() : 0);
            for (sRunopMeas *d = mgen.next(); d; d = mgen.next()) {
                if (d->end_flag())
                    o_endit = true;
                if (d->stop_flag())
                    o_shouldstop = true;
                d->nostop();
            }
        }

        ROgen<sRunopStop> sgen(o_runops->stops(), db ? db->stops() : 0);
        for (sRunopStop *d = sgen.next(); d; d = sgen.next())
            ROret r = d->check_stop(run);
            if (r == RO_PAUSE || r == RO_ENDIT) {
                bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
                TTY.printf("%-2d: condition met: ", d->number());
                d->print_cond(0, false);
                if (r == RO_PAUSE)
                    o_shouldstop = true;
                else
                    o_endit = true;
                if (need_pr)
                    CP.Prompt();
            }
        }
*/
        if (chk->points()) {
            bool increasing = run->job()->JOBoutdata->initValue <=
                run->job()->JOBoutdata->finalValue;
            double val = chk->points()[chk->index()];
            if ((increasing && ref >= val) ||
                    (!increasing && ref <= val)) {

                CBret ret = chk->evaluate();
                chk->set_index(chk->index() + 1);
                if (ret == CBfail || chk->index() == chk->max_index())
                    o_endit = true;
                if (ret == CBendit)
                    chk->set_nogo(true);
            }
        }

        vecGc();
        return;
    }

    if (run->pointCount() > 0) {
        sRunopDb *db = run->circuit() ? &run->circuit()->runops() : 0;
        bool scalarized = false;

        bool tflag = true;
        ROgen<sRunopTrace> tgen(o_runops->traces(), db ? db->traces() : 0);
        for (sRunopTrace *d = tgen.next(); d; d = tgen.next()) {
            if (!scalarized) {
                run->scalarizeVecs();
                scalarized = true;
            }
            d->print_trace(run->runPlot(), &tflag, run->pointCount());
        }

        bool measures_done = true;
        bool measure_queued = false;
        ROgen<sRunopMeas> mgen(o_runops->measures(), db ? db->measures() : 0);
        for (sRunopMeas *d = mgen.next(); d; d = mgen.next()) {
            if (run->anType() != d->analysis())
                continue;
            if (!scalarized) {
                run->scalarizeVecs();
                scalarized = true;
            }

            if (!d->check_measure(run))
                measures_done = false;
            if (d->measure_queued())
                measure_queued = true;
        }

        ROgen<sRunopStop> sgen(o_runops->stops(), db ? db->stops() : 0);
        for (sRunopStop *d = sgen.next(); d; d = sgen.next()) {
            if (!scalarized) {
                run->scalarizeVecs();
                scalarized = true;
            }
            ROret r = d->check_stop(run);
            if (r == RO_PAUSE || r == RO_ENDIT) {
                if (r == RO_PAUSE)
                    o_shouldstop = true;
                else
                    o_endit = true;
                if (!d->silent()) {
                    bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
                    TTY.printf("%-2d: condition met: ", d->number());
                    d->print_cond(0, false);
                    if (need_pr)
                        CP.Prompt();
                }
            }
        }

        if (scalarized) {
            run->unscalarizeVecs();
            scalarized = false;

            if (measure_queued) {
                run->segmentizeVecs();
                mgen = ROgen<sRunopMeas>(o_runops->measures(),
                    db ? db->measures() : 0);
                for (sRunopMeas *d = mgen.next(); d; d = mgen.next()) {
                    if (run->anType() != d->analysis())
                        continue;
                    if (!d->do_measure(run))
                        measures_done = false;
                }
                run->unsegmentizeVecs();
            }
            if (measures_done) {
                // Reset stop flags so analysis can be continued.
                mgen = ROgen<sRunopMeas>(o_runops->measures(),
                    db ? db->measures() : 0);
                for (sRunopMeas *d = mgen.next(); d; d = mgen.next()) {
                    if (d->end_flag())
                        o_endit = true;
                    if (d->stop_flag())
                        o_shouldstop = true;
                    d->nostop();
                }
            }
        }

        if (o_runops->iplots() || (db && db->iplots())) {
            sDataVec *xs = run->runPlot()->scale();
            if (xs && (xs->length() >= IPOINTMIN ||
                    (xs->flags() & VF_ROLLOVER))) {
                ROgen<sRunopIplot> igen(o_runops->iplots(),
                    db ? db->iplots() : 0);
                for (sRunopIplot *d = igen.next(); d; d = igen.next()) {
                    if (d->type() == RO_IPLOT) {
                        iplot(d, run);
                        if (GRpkgIf()->CurDev() &&
                                GRpkgIf()->CurDev()->devtype == GRfullScreen) {
                            break;
                        }
                    }
                }
            }
        }
        if (o_runops->step_count() > 0 && o_runops->dec_step_count() == 0) {
            if (o_runops->num_steps() > 1) {
                bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
                TTY.printf("Stopped after %d steps.\n", o_runops->num_steps());
                if (need_pr)
                    CP.Prompt();
            }
            o_shouldstop = true;
        }
    }

    if (chk && (chk->out_mode == OutcCheckSeg ||
            chk->out_mode == OutcCheckMulti)) {

        // Get status from stops/measures for trial state.
        if (o_endit)
            chk->set_nogo(true);
        if (o_shouldstop) {
            o_endit = true;
            o_shouldstop = false;
            chk->set_failed(true);
        }

        if (!chk->points() || ref < chk->points()[chk->index()] ||
                chk->index() >= chk->max_index()) {
            vecGc();
            return;
        }

        CBret ret = CBfail;
        if (!chk->failed()) {
            run->scalarizeVecs();
            ret = chk->evaluate();
            run->unscalarizeVecs();
        }

        chk->set_index(chk->index() + 1);
        if ((chk->failed() || chk->index() == chk->max_index()) &&
                chk->out_mode != OutcCheckMulti)
            o_endit = true;
        if (ret == CBendit)
            chk->set_nogo(true);
    }

    vecGc();
}


// Check for requested pause.
//
int
IFoutput::pauseTest(sRunDesc *run)
{
    if (!Sp.GetFlag(FT_BATCHMODE))
        GP.Checkup();
    if (Sp.GetFlag(FT_INTERRUPT)) {
        o_shouldstop = false;
        Sp.SetFlag(FT_INTERRUPT, false);
        ToolBar()->UpdatePlots(0);
        endIplot(run);
        return (E_INTRPT);
    }
    else if (o_shouldstop) {
        o_shouldstop = false;
        ToolBar()->UpdatePlots(0);
        endIplot(run);
        return (E_PAUSE);
    }
    else
        return (OK);
}


// Return true if an runop of DF_XXX type given in which exists.
//
bool
IFoutput::hasRunop(unsigned int which)
{
    sRunopDb *db = Sp.CurCircuit() ? &Sp.CurCircuit()->runops() : 0;
    if (which & DF_SAVE) {
        ROgen<sRunopSave> vgen(o_runops->saves(), db ? db->saves() : 0);
        for (sRunopSave *d = vgen.next(); d; d = vgen.next()) {
            if (d->active())
                return (true);
        }
    }
    if (which & DF_TRACE) {
        ROgen<sRunopTrace> tgen(o_runops->traces(), db ? db->traces() : 0);
        for (sRunopTrace *d = tgen.next(); d; d = tgen.next()) {
            if (d->active())
                return (true);
        }
    }
    if (which & DF_IPLOT) {
        ROgen<sRunopIplot> igen(o_runops->iplots(), db ? db->iplots() : 0);
        for (sRunopIplot *d = igen.next(); d; d = igen.next()) {
            if (d->active())
                return (true);
        }
    }
    if (which & DF_MEASURE) {
        ROgen<sRunopMeas> mgen(o_runops->measures(), db ? db->measures() : 0);
        for (sRunopMeas *d = mgen.next(); d; d = mgen.next()) {
            if (d->active())
                return (true);
        }
    }
    if (which & DF_STOP) {
        ROgen<sRunopStop> sgen(o_runops->stops(), db ? db->stops() : 0);
        for (sRunopStop *d = sgen.next(); d; d = sgen.next()) {
            if (d->active())
                return (true);
        }
    }
    return (false);
}
// End of IFoutput functions.


void
sRunopSave::print(char **retstr)
{
    const char *msg0 = "%c %-4d %s %s\n";
    char buf[BSIZE_SP];
    if (!retstr)
        TTY.printf(msg0, ro_active ? ' ' : 'I', ro_number,
            kw_save,  ro_string);
    else {
        sprintf(buf, msg0, ro_active ? ' ' : 'I', ro_number,
            kw_save,  ro_string);
        *retstr = lstring::build_str(*retstr, buf);
    }
}


void
sRunopSave::destroy()
{
    delete this;
}
// End of sRunopSave functions.


void
sRunopTrace::print(char **retstr)
{
    const char *msg0 = "%c %-4d %s %s\n";
    char buf[BSIZE_SP];
    if (!retstr)
        TTY.printf(msg0, ro_active ? ' ' : 'I', ro_number,
            kw_trace,  ro_string ? ro_string : "");
    else {
        sprintf(buf, msg0, ro_active ? ' ' : 'I', ro_number,
            kw_trace,  ro_string ? ro_string : "");
        *retstr = lstring::build_str(*retstr, buf);
    }
}


void
sRunopTrace::destroy()
{
    delete this;
}


namespace { const char *Msg = "can't evaluate %s.\n"; }

// Print the node voltages for trace.
//
bool
sRunopTrace::print_trace(sPlot *plot, bool *flag, int pnt)
{
    if (!ro_active)
        return (true);
    if (*flag) {
        sDataVec *v1 = plot->scale();
        if (v1) {
            TTY.printf_force("%-5d %s = %s\n", pnt, v1->name(), 
                SPnum.printnum(v1->realval(0), v1->units(), false));
        }
        *flag = false;
    }

    if (ro_string) {
        if (ro_bad)
            return (false);
        wordlist *wl = CP.LexStringSub(ro_string);
        if (!wl) {
            GRpkgIf()->ErrPrintf(ET_ERROR, Msg, ro_string);
            ro_bad = true;
            return (false);
        }

        sDvList *dvl = 0;
        pnlist *pl = Sp.GetPtree(wl, true);
        wordlist::destroy(wl);
        if (pl)
            dvl = Sp.DvList(pl);
        if (!dvl) {
            GRpkgIf()->ErrPrintf(ET_ERROR, Msg, ro_string);
            ro_bad = true;
            return (false);
        }
        for (sDvList *dv = dvl; dv; dv = dv->dl_next) {
            sDataVec *v1 = dv->dl_dvec;
            if (v1 == plot->scale())
                continue;
            if (v1->length() <= 0) {
                if (pnt == 1)
                    GRpkgIf()->ErrPrintf(ET_WARN, Msg, v1->name());
                continue;
            }
            if (v1->isreal()) {
                TTY.printf_force("  %-10s %s\n", v1->name(), 
                    SPnum.printnum(v1->realval(0), v1->units(), false));
            }
            else {
                // remember printnum returns static data!
                TTY.printf_force("  %-10s %s, ", v1->name(), 
                    SPnum.printnum(v1->realval(0), v1->units(), false));
                TTY.printf_force("%s\n",
                    SPnum.printnum(v1->imagval(0), v1->units(), false));
            }
        }
        sDvList::destroy(dvl);
    }
    return (true);
}
// End of sRunopTrace functions.


void
sRunopIplot::print(char **retstr)
{
    const char *msg2 = "%c %-4d %s %s\n";
    char buf[BSIZE_SP];
    if (!retstr) {
        TTY.printf(msg2, ro_active ? ' ' : 'I', ro_number, kw_iplot,
            ro_string);
    }
    else {
        sprintf(buf, msg2, ro_active ? ' ' : 'I', ro_number, kw_iplot,
            ro_string);
        *retstr = lstring::build_str(*retstr, buf);
    }
}


void
sRunopIplot::destroy()
{
    if (ro_type == RO_DEADIPLOT && ro_graphid) {
        // User killed the window.
        GP.DestroyGraph(ro_graphid);
    }
    delete this;
}
// End of sRunopIplot functions.


void
sRunopDb::clear()
{
    sRunop::destroy_list(rd_save);
    sRunop::destroy_list(rd_trace);
    sRunop::destroy_list(rd_iplot);
    sRunop::destroy_list(rd_meas);
    sRunop::destroy_list(rd_stop);
    rd_save = 0;
    rd_trace = 0;
    rd_iplot = 0;
    rd_meas = 0;
    rd_stop = 0;
    rd_runopcnt = 1;
    rd_stepcnt = 0;
    rd_steps = 0;
}
// End of sRunopDb functions.

