
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
#include "ftedebug.h"
#include "cshell.h"
#include "commands.h"
#include "toolbar.h"

//
// The trace and iplot commands.
//

// keywords
const char *kw_trace  = "trace";
const char *kw_iplot  = "iplot";


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
// End of CommandTab functions.


// Return true if an iplot is active.
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

