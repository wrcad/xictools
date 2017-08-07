
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

#include "frontend.h"
#include "fteparse.h"
#include "ftedebug.h"
#include "ftemeas.h"
#include "outplot.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "toolbar.h"
#include "spnumber/hash.h"
#include "spnumber/spnumber.h"


//
// Code to deal with breakpoints and tracing.
//

// keywords
const char *kw_stop   = "stop";
const char *kw_trace  = "trace";
const char *kw_iplot  = "iplot";
const char *kw_save   = "save";
const char *kw_after  = "after";
const char *kw_at     = "at";
const char *kw_before = "before";
const char *kw_when   = "when";
const char *kw_active = "active";
const char *kw_inactive = "inactive";

// This if for debugs entered interactively
sDebug DB;

namespace {
    bool isvec(const char*);
    void dbsaveword(sFtCirc*, const char*); 
    bool name_eq(const char*, const char*);
}


// Set a breakpoint. Possible commands are:
//  stop after|at|before n
//  stop when expr
//
// If more than one is given on a command line, then this is a conjunction.
//
void
CommandTab::com_stop(wordlist *wl)
{
    sDbComm *d = 0, *thisone = 0;
    while (wl) {
        if (thisone == 0)
            thisone = d = new sDbComm;
        else {
            d->set_also(new sDbComm);
            d = d->also();
        }

        // Figure out what the first condition is.
        if (lstring::eq(wl->wl_word, kw_after) ||
                lstring::eq(wl->wl_word, kw_at) ||
                lstring::eq(wl->wl_word, kw_before)) {
            if (lstring::eq(wl->wl_word, kw_after))
                d->set_type(DB_STOPAFTER);
            else if (lstring::eq(wl->wl_word, kw_at))
                d->set_type(DB_STOPAT);
            else
                d->set_type(DB_STOPBEFORE);
            int i = 0;
            wl = wl->wl_next;
            if (wl) {
                if (wl->wl_word) {
                    for (char *s = wl->wl_word; *s; s++)
                        if (!isdigit(*s))
                            goto bad;
                    i = atoi(wl->wl_word);
                }
                wl = wl->wl_next;
            }
            d->set_point(i);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_when)) {
            d->set_type(DB_STOPWHEN);
            wl = wl->wl_next;
            if (!wl)
                goto bad;
            wordlist *wx;
            int i;
            for (i = 0, wx = wl; wx; wx = wx->wl_next) {
                if (lstring::eq(wx->wl_word, kw_after) ||
                        lstring::eq(wx->wl_word, kw_at) ||
                        lstring::eq(wx->wl_word, kw_before) ||
                        lstring::eq(wx->wl_word, kw_when))
                    break;
                i += strlen(wx->wl_word) + 1;
            }
            i++;
            char *string = new char[i];
            char *s = lstring::stpcpy(string, wl->wl_word);
            for (wl = wl->wl_next; wl; wl = wl->wl_next) {
                if (lstring::eq(wl->wl_word, kw_after) ||
                        lstring::eq(wl->wl_word, kw_at) ||
                        lstring::eq(wl->wl_word, kw_before) ||
                        lstring::eq(wl->wl_word, kw_when))
                    break;
                char *t = wl->wl_word;
                *s++ = ' ';
                while (*t)
                    *s++ = *t++;
            }
            *s = '\0';
            CP.Unquote(string);
            d->set_string(string);
        }
        else
            goto bad;
    }
    if (thisone) {
        thisone->set_number(DB.new_count());
        thisone->set_active(true);
        if (CP.GetFlag(CP_INTERACTIVE) || !Sp.CurCircuit()) {
            if (DB.stops()) {
                for (d = DB.stops(); d->next(); d = d->next())
                    ;
                d->set_next(thisone);
            }
            else
                DB.set_stops(thisone);
        }
        else {
            if (!Sp.CurCircuit()->debugs())
                Sp.CurCircuit()->set_debugs(new sDebug);
            sDebug *db = Sp.CurCircuit()->debugs();
            if (db->stops()) {
                for (d = db->stops(); d->next(); d = d->next())
                    ;
                d->set_next(thisone);
            }
            else
                db->set_stops(thisone);
        }
    }
    ToolBar()->UpdateTrace();
    return;

bad:
    GRpkgIf()->ErrPrintf(ET_ERROR, "syntax.\n");
}


// Trace a node. Usage is "trace expr ..."
//
void
CommandTab::com_trace(wordlist *wl)
{
    const char *msg = "already tracing %s, ignored.\n";

    sDbComm *d = new sDbComm;
    d->set_type(DB_TRACE);
    d->set_active(true);
    d->set_string(wordlist::flatten(wl));
    d->set_number(DB.new_count());

    if (CP.GetFlag(CP_INTERACTIVE) || !Sp.CurCircuit()) {
        if (DB.traces()) {
            sDbComm *ld = 0;
            for (sDbComm *td = DB.traces(); td; ld = td, td = td->next()) {
                if (lstring::eq(td->string(), d->string())) {
                    GRpkgIf()->ErrPrintf(ET_WARN, msg, td->string());
                    sDbComm::destroy(d);
                    DB.decrement_count();
                    return;
                }
            }
            ld->set_next(d);
        }
        else
            DB.set_traces(d);
    }
    else {
        if (!Sp.CurCircuit()->debugs())
            Sp.CurCircuit()->set_debugs(new sDebug);
        sDebug *db = Sp.CurCircuit()->debugs();
        if (db->traces()) {
            sDbComm *ld = 0;
            for (sDbComm *td = db->traces(); td; ld = td, td = td->next()) {
                if (lstring::eq(td->string(), d->string())) {
                    GRpkgIf()->ErrPrintf(ET_WARN, msg, td->string());
                    sDbComm::destroy(d);
                    DB.decrement_count();
                    return;
                }
            }
            ld->set_next(d);
        }
        else
            db->set_traces(d);
    }
    ToolBar()->UpdateTrace();
}


// Incrementally plot a value. This is just like trace.
//
void
CommandTab::com_iplot(wordlist *wl)
{
    if (!wl) {
        GRpkgIf()->ErrPrintf(ET_ERRORS, "no vectors given.\n");
        return;
    }
    wl = wordlist::copy(wl);
    for (wordlist *ww = wl; ww; ww = ww->wl_next) {
        if (lstring::eq(ww->wl_word, ".")) {
            wordlist *wx = Sp.ExtractPlotCmd(0, "tran");
            if (!wx) {
                GRpkgIf()->ErrPrintf(ET_ERRORS,
                    "no vectors found for '.'.\n");
                wordlist::destroy(wl);
                return;
            }
            if (ww == wl)
                wl = wx;
            ww = ww->splice(wx);
            continue;
        }
        if (ww->wl_word[0] == '.' && ww->wl_word[1] == '@' &&
                isdigit(ww->wl_word[2])) {
            int n = atoi(ww->wl_word + 2);
            wordlist *wx = Sp.ExtractPlotCmd(n ? n-1 : n, "tran");
            if (!wx) {
                GRpkgIf()->ErrPrintf(ET_ERRORS,
                    "no vectors found for '.@%d'.\n", n);
                wordlist::destroy(wl);
                return;
            }
            if (ww == wl)
                wl = wx;
            ww = ww->splice(wx);
            continue;
        }
    }

    sDbComm *d = new sDbComm;
    d->set_type(DB_IPLOT);
    d->set_active(true);
    d->set_string(wordlist::flatten(wl));
    wordlist::destroy(wl);
    d->set_number(DB.new_count());

    if (CP.GetFlag(CP_INTERACTIVE) || !Sp.CurCircuit()) {
        if (DB.iplots()) {
            sDbComm *td;
            for (td = DB.iplots(); td->next(); td = td->next()) ;
            td->set_next(d);
        }
        else
            DB.set_iplots(d);
    }
    else {
        if (!Sp.CurCircuit()->debugs())
            Sp.CurCircuit()->set_debugs(new sDebug);
        sDebug *db = Sp.CurCircuit()->debugs();
        if (db->iplots()) {
            sDbComm *td;
            for (td = db->iplots(); td->next(); td = td->next()) ;
            td->set_next(d);
        }
        else
            db->set_iplots(d);
    }
    ToolBar()->UpdateTrace();
}


void
CommandTab::com_save(wordlist *wl)
{
    char buf[BSIZE_SP];
    for ( ; wl; wl = wl->wl_next) {
        if (!isvec(wl->wl_word)) {
            dbsaveword(Sp.CurCircuit(), wl->wl_word);
            continue;
        }
        if (*wl->wl_word == 'v' || *wl->wl_word == 'V') {
            // deal with forms like vxx(a,b)
            char *s = strchr(wl->wl_word, '(') + 1;
            while (isspace(*s))
                s++;
            strcpy(buf, s);
            s = strchr(buf, ',');
            if (!s) {
                dbsaveword(Sp.CurCircuit(), wl->wl_word);
                continue;
            }
            char *n1 = buf;
            *s++ = '\0';
            while (isspace(*s))
                s++;
            char *n2 = s;
            for (s = n1 + strlen(n1) - 1; s > n1; s--) {
                if (isspace(*s))
                    *s = '\0';
                else
                    break;
            }
            for (s = n2 + strlen(n2) - 1; s > n2; s--) {
                if (isspace(*s) || *s == ')')
                    *s = '\0';
                else
                    break;
            }
            dbsaveword(Sp.CurCircuit(), n1);
            dbsaveword(Sp.CurCircuit(), n2);
        }
        else {
            // deal with forms like ixx(vyy)
            char *s = strchr(wl->wl_word, '(') + 1;
            while (isspace(*s))
                s++;
            strcpy(buf, s);
            char *n1 = buf;
            for (s = n1 + strlen(n1) - 1; s > n1; s--) {
                if (isspace(*s) || *s == ')')
                    *s = '\0';
                else
                    break;
            }
            strcat(buf, "#branch");
            dbsaveword(Sp.CurCircuit(), n1);
        }
    }
    ToolBar()->UpdateTrace();
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
    DB.set_step_count(n);
    DB.set_num_steps(n);
    Sp.Simulate(SIMresume, 0);
}


// Print out the currently active breakpoints and traces.
//
void
CommandTab::com_status(wordlist*)
{
    Sp.DebugStatus(true);
}


// Delete breakpoints and traces. Usage is:
//   delete [[in]active][all | stop | trace | iplot | save | number] ...
// With inactive true (args to right of keyword), the debug is
// cleared only if it is inactive.
//
void
CommandTab::com_delete(wordlist *wl)
{
    if (!wl) {
        sDbComm *d = DB.stops();
        // find the earlist debug
        if (!d || (DB.traces() && d->number() > DB.traces()->number()))
            d = DB.traces();
        if (!d || (DB.iplots() && d->number() > DB.iplots()->number()))
            d = DB.iplots();
        if (!d || (DB.saves() && d->number() > DB.saves()->number()))
            d = DB.saves();
        if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
            sDebug *db = Sp.CurCircuit()->debugs();
            if (!d || (db->stops() && d->number() > db->stops()->number()))
                d = db->stops();
            if (!d || (db->traces() && d->number() > db->traces()->number()))
                d = db->traces();
            if (!d || (db->iplots() && d->number() > db->iplots()->number()))
                d = db->iplots();
            if (!d || (db->saves() && d->number() > db->saves()->number()))
                d = db->saves();
        }
        if (!d) {
            TTY.printf("No debugs are in effect.\n");
            return;
        }
        d->print(0);
        if (CP.GetFlag(CP_NOTTYIO))
            return;
        if (!TTY.prompt_for_yn(true, "delete [y]? "))
            return;
        if (d == DB.stops())
            DB.set_stops(d->next());
        else if (d == DB.traces())
            DB.set_traces(d->next());
        else if (d == DB.iplots())
            DB.set_iplots(d->next());
        else if (d == DB.saves())
            DB.set_saves(d->next());
        else if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
            sDebug *db = Sp.CurCircuit()->debugs();
            if (d == db->stops())
                db->set_stops(d->next());
            else if (d == db->traces())
                db->set_traces(d->next());
            else if (d == db->iplots())
                db->set_iplots(d->next());
            else if (d == db->saves())
                db->set_saves(d->next());
        }
        ToolBar()->UpdateTrace();
        sDbComm::destroy(d);
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
            Sp.DeleteDbg(true, true, true, true, inactive, -1);
            return;
        }
        if (lstring::eq(wl->wl_word, kw_stop)) {
            Sp.DeleteDbg(true, false, false, false, inactive, -1);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_trace)) {
            Sp.DeleteDbg(false, true, false, false, inactive, -1);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_iplot)) {
            Sp.DeleteDbg(false, false, true, false, inactive, -1);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_save)) {
            Sp.DeleteDbg(false, false, false, true, inactive, -1);
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
                Sp.DeleteDbg(true, true, true, true, inactive, n1);
                n1++;
            }
            continue;
        }
                
        for (s = wl->wl_word; *s; s++) {
            if (!isdigit(*s)) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s isn't a debug number.\n",
                    wl->wl_word);
                break;
            }
        }
        if (*s)
            continue;
        int i = atoi(wl->wl_word);
        Sp.DeleteDbg(true, true, true, true, inactive, i);
    }
    ToolBar()->UpdateTrace();
}
// End of CommandTab functions.


void
IFsimulator::SetDbgActive(int dbnum, bool state)
{
    sDbComm *d;
    for (d = DB.stops(); d; d = d->next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }
    for (d = DB.traces(); d; d = d->next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }
    for (d = DB.iplots(); d; d = d->next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }
    for (d = DB.saves(); d; d = d->next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }
    if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
        sDebug *db = Sp.CurCircuit()->debugs();
        for (d = db->stops(); d; d = d->next()) {
            if (d->number() == dbnum) {
                d->set_active(state);
                return;
            }
        }
        for (d = db->traces(); d; d = d->next()) {
            if (d->number() == dbnum) {
                d->set_active(state);
                return;
            }
        }
        for (d = db->iplots(); d; d = d->next()) {
            if (d->number() == dbnum) {
                d->set_active(state);
                return;
            }
        }
        for (d = db->saves(); d; d = d->next()) {
            if (d->number() == dbnum) {
                d->set_active(state);
                return;
            }
        }
    }
}


char *
IFsimulator::DebugStatus(bool tofp)
{
    const char *msg = "No debugs are in effect.\n";
    if (!DB.isset() && (!Sp.CurCircuit() || !Sp.CurCircuit()->debugs() ||
            !Sp.CurCircuit()->debugs()->isset())) {
        if (tofp) {
            TTY.send(msg);
            return (0);
        }
        else
            return (lstring::copy(msg));
    }
    char *s = 0, **t;
    if (tofp)
        t = 0;
    else
        t = &s;
    sDbComm *d;
    sDebug *db = 0;
    if (Sp.CurCircuit() && Sp.CurCircuit()->debugs())
        db = Sp.CurCircuit()->debugs();
    for (d = DB.stops(); d; d = d->next())
        d->print(t);
    if (db)
        for (d = db->stops(); d; d = d->next())
            d->print(t);
    for (d = DB.traces(); d; d = d->next())
        d->print(t);
    if (db)
        for (d = db->traces(); d; d = d->next())
            d->print(t);
    for (d = DB.iplots(); d; d = d->next())
        d->print(t);
    if (db)
        for (d = db->iplots(); d; d = d->next())
            d->print(t);
    for (d = DB.saves(); d; d = d->next())
        d->print(t);
    if (db)
        for (d = db->saves(); d; d = d->next())
            d->print(t);
    return (s);
}


void
IFsimulator::DeleteDbg(bool stop, bool trace, bool iplot, bool save,
    bool inactive, int num)
{
    sDbComm *d, *dlast, *dnext;
    if (stop) {
        dlast = 0;
        for (d = DB.stops(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    DB.set_stops(dnext);
                else
                    dlast->set_next(dnext);
                sDbComm::destroy(d);
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
            sDebug *db = Sp.CurCircuit()->debugs();
            dlast = 0;
            for (d = db->stops(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db->set_stops(dnext);
                    else
                        dlast->set_next(dnext);
                    sDbComm::destroy(d);
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
    if (trace) {
        dlast = 0;
        for (d = DB.traces(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    DB.set_traces(dnext);
                else
                    dlast->set_next(dnext);
                sDbComm::destroy(d);
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
            sDebug *db = Sp.CurCircuit()->debugs();
            dlast = 0;
            for (d = db->traces(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db->set_traces(dnext);
                    else
                        dlast->set_next(dnext);
                    sDbComm::destroy(d);
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
    if (iplot) {
        dlast = 0;
        for (d = DB.iplots(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    DB.set_iplots(dnext);
                else
                    dlast->set_next(dnext);
                sDbComm::destroy(d);
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
            sDebug *db = Sp.CurCircuit()->debugs();
            dlast = 0;
            for (d = db->iplots(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db->set_iplots(dnext);
                    else
                        dlast->set_next(dnext);
                    sDbComm::destroy(d);
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
    if (save) {
        dlast = 0;
        for (d = DB.saves(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    DB.set_saves(dnext);
                else
                    dlast->set_next(dnext);
                sDbComm::destroy(d);
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
            sDebug *db = Sp.CurCircuit()->debugs();
            dlast = 0;
            for (d = db->saves(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db->set_saves(dnext);
                    else
                        dlast->set_next(dnext);
                    sDbComm::destroy(d);
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
}


// Fill in a list of vector names used in the debugs.  If there are only
// specials in the list as passed and in any db saves, save only specials,
// i.e., names starting with SpecCatchar().
//
// The .measure lines are also checked here.
//
void
IFsimulator::GetSaves(sFtCirc *circuit, sSaveList *saves)
{
    sDebug *db = 0;
    if (circuit && circuit->debugs())
        db = circuit->debugs();
    sDbComm *d;
    for (d = DB.saves(); d && d->active(); d = d->next())
        saves->add_save(d->string());
    if (db) {
        for (d = db->saves(); d && d->active(); d = d->next())
            saves->add_save(d->string());
    }
    bool saveall = true;
    sHgen gen(saves->table());
    sHent *h;
    while ((h = gen.next()) != 0) {
        if (*h->name() != SpecCatchar()) {
            saveall = false;
            break;
        }
    }

    // If there is nothing but specials in the list, saveall will be
    // true, which means that all circuit vectors are automatically
    // saved and the only saves we list here are the specials.
    //
    // However, we have the look for these the "hard way" in any case,
    // since we would like to recognize cases like p(Vsrc) which map
    // to @Vsrc[p].  When done, we'll go back an purge non-specials
    // if saveall is true.

    for (d = DB.traces(); d && d->active(); d = d->next())
        saves->list_expr(d->string());
    if (db) {
        for (d = db->traces(); d && d->active(); d = d->next())
            saves->list_expr(d->string());
    }
    for (d = DB.iplots(); d && d->active(); d = d->next()) {
        saves->list_expr(d->string());
        for (sDbComm *dd = d->also(); dd; dd = dd->also())
            saves->list_expr(d->string());
    }
    if (db) {
        for (d = db->iplots(); d && d->active(); d = d->next()) {
            saves->list_expr(d->string());
            for (sDbComm *dd = d->also(); dd; dd = dd->also())
                saves->list_expr(d->string());
        }
    }
    for (d = DB.stops(); d && d->active(); d = d->next())
        saves->list_expr(d->string());
    if (db) {
        for (d = db->stops(); d && d->active(); d = d->next())
            saves->list_expr(d->string());
    }

    for (sMeas *m = circuit->measures(); m; m = m->next) {
        if (m->start_name)
            saves->list_expr(m->start_name);
        if (m->end_name)
            saves->list_expr(m->end_name);
        if (m->expr2)
            saves->list_expr(m->expr2);
        if (m->start_when_expr1)
            saves->list_expr(m->start_when_expr1);
        if (m->start_when_expr2)
            saves->list_expr(m->start_when_expr2);
        if (m->end_when_expr1)
            saves->list_expr(m->end_when_expr1);
        if (m->end_when_expr2)
            saves->list_expr(m->end_when_expr2);
        for (sMfunc *f = m->funcs; f; f = f->next)
            saves->list_expr(f->expr);
    }
    if (saveall)
        saves->purge_non_special();
    else {
        // Remove all the measure result names, which may have been
        // added if they are referenced by another measure.  These
        // vectors don't exist until the measurement is done.

        for (sMeas *m = circuit->measures(); m; m = m->next)
            saves->remove_save(m->result);
    }
}


// Return true if an iplot is active
//
int
IFsimulator::IsIplot(bool resurrect)
{
    if (resurrect) {
        // Turn dead iplots back on.
        for (sDbComm *d = DB.iplots(); d; d = d->next()) {
            if (d->type() == DB_DEADIPLOT)
                d->set_type(DB_IPLOT);
        }
        if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
            sDebug *db = Sp.CurCircuit()->debugs();
            for (sDbComm *d = db->iplots(); d; d = d->next()) {
                if (d->type() == DB_DEADIPLOT)
                    d->set_type(DB_IPLOT);
            }
        }
    }

    for (sDbComm *d = DB.iplots(); d; d = d->next()) {
        if ((d->type() == DB_IPLOT || d->type() == DB_IPLOTALL) && d->active())
            return (true);
    }
    if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
        sDebug *db = Sp.CurCircuit()->debugs();
        for (sDbComm *d = db->iplots(); d; d = d->next())
            if ((d->type() == DB_IPLOT || d->type() == DB_IPLOTALL) &&
                    d->active())
                return (true);
    }
    return (false);
}
// End of IFsimulator functions.


// Static function.
void
sDbComm::destroy(sDbComm *dd)
{
    while (dd) {
        sDbComm *dx = dd;
        dd = dd->db_also;
        if (dx->db_type == DB_DEADIPLOT && dx->db_graphid) {
            // User killed the window.
            GP.DestroyGraph(dx->db_graphid);
        }
        delete dx;
    }
}


namespace { const char *Msg = "can't evaluate %s.\n"; }

bool
sDbComm::istrue()
{
    if (db_point < 0)
        return (true);

    wordlist *wl = CP.LexStringSub(db_string);
    if (!wl) {
        GRpkgIf()->ErrPrintf(ET_ERROR, Msg, db_string);
        db_point = -1;
        return (true);
    }
    char *str = wordlist::flatten(wl);
    wordlist::destroy(wl);

    const char *t = str;
    pnode *p = Sp.GetPnode(&t, true);
    delete [] str;
    sDataVec *v = 0;
    if (p) {
        v = Sp.Evaluate(p);
        delete p;
    }
    if (!v) {
        GRpkgIf()->ErrPrintf(ET_ERROR, Msg, db_string);
        db_point = -1;
        return (true);
    }
    if (v->realval(0) != 0.0 || v->imagval(0) != 0.0)
        return (true);
    return (false);
}


bool
sDbComm::should_stop(int pnt)
{
    if (!db_active)
        return (false);
    bool when = true;
    bool after = true;
    for (sDbComm *dt = this; dt; dt = dt->db_also) {
        if (dt->db_type == DB_STOPAFTER) {
            if (pnt < dt->db_point) {
                after = false;
                break;
            }
        }
        else if (dt->db_type == DB_STOPAT) {
            if (pnt != dt->db_point) {
                after = false;
                break;
            }
        }
        else if (dt->db_type == DB_STOPBEFORE) {
            if (pnt > dt->db_point) {
                after = false;
                break;
            }
        }
        else if (dt->db_type == DB_STOPWHEN) {
            if (!dt->istrue()) {
                when = false;
                break;
            }
        }
    }
    return (when && after);
}


void
sDbComm::print(char **retstr)
{
    char buf[BSIZE_SP];
    sDbComm *dc = 0;
    const char *msg0 = "%c %-4d %s %s\n";
    const char *msg1 = "%c %-4d stop";
    const char *msg2 = "%c %-4d %s %s";

    switch (db_type) {
    case DB_SAVE:
        if (!retstr)
            TTY.printf(msg0, db_active ? ' ' : 'I', db_number,
                kw_save,  db_string);
        else {
            sprintf(buf, msg0, db_active ? ' ' : 'I', db_number,
                kw_save,  db_string);
            *retstr = lstring::build_str(*retstr, buf);
        }
        break;

    case DB_TRACE:
        if (!retstr)
            TTY.printf(msg0, db_active ? ' ' : 'I', db_number,
                kw_trace,  db_string ? db_string : "");
        else {
            sprintf(buf, msg0, db_active ? ' ' : 'I', db_number,
                kw_trace,  db_string ? db_string : "");
            *retstr = lstring::build_str(*retstr, buf);
        }
        break;

    case DB_STOPWHEN:
    case DB_STOPAFTER:
    case DB_STOPAT:
    case DB_STOPBEFORE:
        if (!retstr) {
            TTY.printf(msg1, db_active ? ' ' : 'I', db_number);
            printcond(0);
        }
        else {
            sprintf(buf, msg1, db_active ? ' ' : 'I', db_number);
            *retstr = lstring::build_str(*retstr, buf);
            printcond(retstr);
        }
        break;

    case DB_IPLOT:
    case DB_DEADIPLOT:
        if (!retstr) {
            TTY.printf(msg2, db_active ? ' ' : 'I', db_number,
                kw_iplot, db_string);
            for (dc = db_also; dc; dc = dc->db_also)
                TTY.printf(" %s", dc->db_string);
            TTY.send("\n");
        }
        else {
            sprintf(buf, msg2, db_active ? ' ' : 'I', db_number,
                kw_iplot, db_string);
            for (dc = db_also; dc; dc = dc->db_also)
                sprintf(buf + strlen(buf), " %s", dc->db_string);
            strcat(buf, "\n");
            *retstr = lstring::build_str(*retstr, buf);
        }
        break;

    default:
        GRpkgIf()->ErrPrintf(ET_INTERR, "com_sttus: bad db %d.\n", db_type);
        break;
    }
}


// Print the node voltages for trace.
//
bool
sDbComm::print_trace(sPlot *plot, bool *flag, int pnt)
{
    if (!db_active)
        return (true);
    if (*flag) {
        sDataVec *v1 = plot->scale();
        if (v1)
            TTY.printf_force("%-5d %s = %s\n", pnt, v1->name(), 
                SPnum.printnum(v1->realval(0), v1->units(), false));
        *flag = false;
    }

    if (db_string) {
        if (db_point < 0)
            return (false);
        wordlist *wl = CP.LexStringSub(db_string);
        if (!wl) {
            GRpkgIf()->ErrPrintf(ET_ERROR, Msg, db_string);
            db_point = -1;
            return (false);
        }

        sDvList *dvl = 0;
        pnlist *pl = Sp.GetPtree(wl, true);
        wordlist::destroy(wl);
        if (pl)
            dvl = Sp.DvList(pl);
        if (!dvl) {
            GRpkgIf()->ErrPrintf(ET_ERROR, Msg, db_string);
            db_point = -1;
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
            if (v1->isreal())
                TTY.printf_force("  %-10s %s\n", v1->name(), 
                    SPnum.printnum(v1->realval(0), v1->units(), false));

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


// Print the condition.  If fp is 0 and retstr is not 0,
// add the text to *retstr, otherwise print to fp;
//
void
sDbComm::printcond(char **retstr)
{
    char buf[BSIZE_SP];
    const char *msg1 = " %s %d";
    const char *msg2 = " %s %s";

    buf[0] = '\0';
    for (sDbComm *dt = this; dt; dt = dt->db_also) {
        if (retstr) {
            if (dt->db_type == DB_STOPAFTER)
                sprintf(buf + strlen(buf), msg1, kw_after, dt->db_point);
            else if (dt->db_type == DB_STOPAT)
                sprintf(buf + strlen(buf), msg1, kw_at, dt->db_point);
            else if (dt->db_type == DB_STOPBEFORE)
                sprintf(buf + strlen(buf), msg1, kw_before, dt->db_point);
            else
                sprintf(buf + strlen(buf), msg2, kw_when, dt->db_string);
        }
        else {
            if (dt->db_type == DB_STOPAFTER)
                TTY.printf(msg1, kw_after, dt->db_point);
            else if (dt->db_type == DB_STOPAT)
                TTY.printf(msg1, kw_at, dt->db_point);
            else if (dt->db_type == DB_STOPBEFORE)
                TTY.printf(msg1, kw_before, dt->db_point);
            else
                TTY.printf(msg2, kw_when, dt->db_string);
        }
    }
    if (retstr) {
        strcat(buf, "\n");
        *retstr = lstring::build_str(*retstr, buf);
    }
    else
        TTY.send("\n");
}
// End of sDbComm functions.


sSaveList::~sSaveList()
{
    delete sl_tab;
}


// Return the number of words saved.
//
int
sSaveList::numsaves()
{
    if (!sl_tab)
        return (0);
    return (sl_tab->allocated());
}


// Set the used flag for the entry name, return true if the state was
// set.
//
bool
sSaveList::set_used(const char *name, bool used)
{
    if (!sl_tab)
        return (false);
    sHent *h = sHtab::get_ent(sl_tab, name);
    if (!h)
        return (false);
    h->set_data((void*)(long)used);
    return (true);
}


// Return 1/0 if the element name exists ant the user flag is
// set/unset.  return -1 if not in table.
//
int
sSaveList::is_used(const char *name)
{
    if (!sl_tab)
        return (-1);
    sHent *h = sHtab::get_ent(sl_tab, name);
    if (!h)
        return (-1);
    return (h->data() != 0);
}


// Add word to the save list, if it is not already there.
//
void
sSaveList::add_save(const char *word)
{
    if (!sl_tab)
        sl_tab = new sHtab(sHtab::get_ciflag(CSE_NODE));

    if (sHtab::get_ent(sl_tab, word))
        return;
    sl_tab->add(word, 0);
}


// Remove name from list, if found.
//
void
sSaveList::remove_save(const char *name)
{
    if (name && sl_tab)
        sl_tab->remove(name);
}


// Save all the vectors.
//
void
sSaveList::list_expr(const char *expr)
{
    if (expr) {
        wordlist *wl = CP.LexStringSub(expr);
        if (wl) {
            pnlist *pl0 = Sp.GetPtree(wl, false);
            for (pnlist *pl = pl0; pl; pl = pl->next())
                list_vecs(pl->node());
            pnlist::destroy(pl0);
            wordlist::destroy(wl);
        }
    }
}


// Remove all entries that are not "special", i.e., don't start with
// Sp.SpecCatchar().  The list is resized.
//
void
sSaveList::purge_non_special()
{
    if (sl_tab) {
        wordlist *wl0 = sHtab::wl(sl_tab);
        for (wordlist *wl = wl0; wl; wl = wl->wl_next) {
            if (*wl->wl_word != Sp.SpecCatchar())
                sl_tab->remove(wl->wl_word);
        }
        wordlist::destroy(wl0);
    }
}


// Go through the parse tree, and add any vector names found to the
// saves.
//
void
sSaveList::list_vecs(pnode *pn)
{
    char buf[128];
    if (pn->token_string()) {
        if (!pn->value())
            add_save(pn->token_string());
    }
    else if (pn->func()) {
        if (!pn->func()->func()) {
            if (*pn->func()->name() == 'v')
                sprintf(buf, "v(%s)", pn->left()->token_string());
            else
                sprintf(buf, "%s", pn->left()->token_string());
            add_save(buf);
        }
        else
            list_vecs(pn->left());
    }
    else if (pn->oper()) {
        list_vecs(pn->left());
        if (pn->right())
            list_vecs(pn->right());
    }
}
// End of sSaveList functions.


namespace {
    // Return true if s is one of v(, vm(, vp(, vr(, vi(, or same with i
    // replacing v.
    //
    bool isvec(const char *cs)
    {
        char buf[8];
        if (*cs != 'v' && *cs != 'V' && *cs != 'i' && *cs != 'I')
            return (false);
        strncpy(buf, cs+1, 7);
        buf[7] = '\0';
        char *s = buf;
        for ( ; *s; s++)
            if (isupper(*s))
                *s = tolower(*s);
        s = buf;
        if (*s == '(')
            return (true);
        if (*(s+1) == '(' &&
                (*s == 'm' || *s == 'p' || *s == 'r' || *s == 'i'))
            return (true);
        if (*(s+2) == '(' && *(s+1) == 'b' && *s == 'd')
            return (true);
        return (false);
    }


    // Save a vector.
    //
    void dbsaveword(sFtCirc *circuit, const char *name)
    {
        char *s = lstring::copy(name);
        CP.Unquote(s);
        sDbComm *td;
        for (td = DB.saves(); td; td = td->next()) {
            if (name_eq(s, td->string())) {
                delete [] s;
                return;
            }
        }
        if (circuit && circuit->debugs()) {
            for (td = circuit->debugs()->saves(); td; td = td->next()) {
                if (name_eq(s, td->string())) {
                    delete [] s;
                    return;
                }
            }
        }

        sDbComm *d = new sDbComm;
        d->set_type(DB_SAVE);
        d->set_active(true);
        d->set_string(s);
        d->set_number(DB.new_count());

        if (CP.GetFlag(CP_INTERACTIVE) || !circuit) {
            if (DB.saves()) {
                for (td = DB.saves(); td->next(); td = td->next()) ;
                td->set_next(d);
            }
            else
                DB.set_saves(d);
        }
        else {
            if (!circuit->debugs())
                circuit->set_debugs(new sDebug);
            sDebug *db = circuit->debugs();
            if (db->saves()) {
                for (td = db->saves(); td->next(); td = td->next()) ;
                td->set_next(d);
            }
            else
                db->set_saves(d);
        }
    }


    // This function must match two names with or without a V() around
    // them.
    //
    bool name_eq(const char *n1, const char *n2)
    {
        if (!n1 || !n2)
            return (false);
        const char *s = strchr(n1, '(');
        if (s)
            n1 = s+1;
        s = strchr(n2, '(');
        if (s)
            n2 = s+1;
        while (isspace(*n1))
            n1++;
        while (isspace(*n2))
            n2++;
        for ( ; ; n1++, n2++) {
            if ((*n1 == 0 || *n1 == ')' || isspace(*n1)) &&
                    (*n2 == 0 || *n2 == ')' || isspace(*n2)))
                return (true);
            if (*n1 != *n2)
                break;
        }
        return (false);
    }
}

