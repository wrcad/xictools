
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
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "config.h"
#include "simulator.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "kwords_analysis.h"
#include "commands.h"
#include "misc.h"
#include "optdefs.h"
#include "statdefs.h"
#include "ginterf/graphics.h"
#ifdef HAVE_LOCAL_ALLOCATOR
#include "malloc/local_malloc.h"
#else
#include "miscutil/coresize.h"
#endif


//
// Resource-related functions.
//

// Most modern OSes have both TIMES and GETRUSAGE interfaces.  Use
// getrusage on BSD-flavored systems.
#if defined(__FreeBSD__) || defined(__APPLE__)
#if defined(HAVE_GETRUSAGE) && defined(HAVE_GETTIMEOFDAY)
#ifdef HAVE_TIMES
#undef HAVE_TIMES
#endif
#endif
#endif

#ifdef HAVE_TIMES
#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>
#ifdef __APPLE__
#define HZ CLK_TCK
#endif
namespace {
    clock_t origsec;
    clock_t lastsec;
    clock_t lastusrsec;
    clock_t lastsyssec;
}
#endif

#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
#ifndef HAVE_TIMES
namespace {
    int origsec;
    int lastsec;
    int origusec;
    int lastusec;
}
#endif
#endif

#ifdef HAVE_GETRUSAGE
#include <sys/resource.h>
#ifndef HAVE_TIMES
namespace {
    int lastusrsec;
    int lastsyssec;
    int lastusrusec;
    int lastsysusec;
}
#endif
#endif

#ifdef WIN32
#include <windows.h>
#include <malloc.h>
#endif

#ifdef HAVE_LOCAL_ALLOCATOR
#include "malloc/local_malloc.h"
#endif

#if defined(HAVE_GETRLIMIT) && defined(RLIMIT_DATA) && defined(RLIM_INFINITY)
#else
#if defined(HAVE_ULIMIT) && defined(HAVE_ULIMIT_H)
#include <ulimit.h>
#endif 
#endif 

const char *kw_elapsed      = "elapsed";
const char *kw_totaltime    = "totaltime";
const char *kw_space        = "space";
const char *kw_faults       = "faults";
const char *kw_stats        = "stats";


// The rusage command.
//
void
CommandTab::com_rusage(wordlist *wl)
{
    Sp.ShowResource(wl, 0);
}


// The stats command.  This is like rusage, but if no argument it
// prints all statistics.
//
void
CommandTab::com_stats(wordlist *wl)
{
    wordlist xwl;
    char buf[16];
    xwl.wl_word = buf;
    if (!wl) {
        strcpy(buf, kw_stats);
        wl = &xwl;
    }
    Sp.ShowResource(wl, 0);
}
// End of CommandTab functions.


// Function to show resources listed in wl.  If retstr is not 0,
// output will be put in this string.
//
void
IFsimulator::ShowResource(wordlist *wl, char **retstr)
{
    sLstr *plstr = 0;
    if (!retstr)
        TTY.send("\n");
    else
        plstr = new sLstr;
    if (wl && (lstring::eq(wl->wl_word, kw_everything) ||
            lstring::cieq(wl->wl_word, kw_all)))
        ResPrint::print_res(0, plstr);
    else if (wl) {
        for (; wl; wl = wl->wl_next) {
            char *c = lstring::copy(wl->wl_word);
            CP.Unquote(c);
            ResPrint::print_res(c, plstr);
            delete [] c;
        }
    }
    else {
        ResPrint::print_res(kw_elapsed, plstr);
        ResPrint::print_res(kw_totaltime, plstr);
        ResPrint::print_res(kw_space, plstr);
    }
    if (retstr)
        *retstr = plstr ? plstr->string_trim() : 0;
    delete plstr;
}


// Find out if the user is approaching the maximum data size.
//
void
IFsimulator::CheckSpace()
{
#if defined(HAVE_GETRLIMIT) && defined(RLIMIT_DATA) && defined(RLIM_INFINITY)
    static long lastmax;
    const char *msg1 =
        "approaching max data size: cur size = %ld, limit = %ld.\n"; 
    struct rlimit rld;
    getrlimit(RLIMIT_DATA, &rld);
    if (rld.rlim_cur == RLIM_INFINITY)
        return;
#ifdef HAVE_LOCAL_ALLOCATOR
    long cur = (long)(1000*Memory()->coresize());
#else
    long cur = (long)(1000*coresize());
#endif
    if (cur > rld.rlim_max * 0.9) {
        if (cur > lastmax)
            GRpkg::self()->ErrPrintf(ET_WARN, msg1, cur, (long)rld.rlim_max);
        lastmax = cur;
    }
    else if (cur > rld.rlim_cur * 0.9) {
        if (cur > lastmax)
            GRpkg::self()->ErrPrintf(ET_WARN, msg1, cur, (long)rld.rlim_cur);
        lastmax = cur;
    }
#else
#ifdef HAVE_ULIMIT
    static long lastmax;
    const char *msg1 =
        "approaching max data size: cur size = %ld, limit = %ld.\n"; 
    long lim = ulimit(3, 0L);
#ifdef HAVE_LOCAL_ALLOCATOR
    long cur = (long)(1000*Memory()->coresize());
#else
    long cur = (long)(1000*coresize());
#endif
    if (cur > lim * 0.9) {
        if (cur > lastmax)
            GRpkg::self()->ErrPrintf(ET_WARN, msg1, (long)hi, (long)lim);
        lastmax = cur;
    }
#endif
#endif
}


// Initialize the variables used in the rusage command.  Called from
// application startup code.
//
void
IFsimulator::InitResource()
{
#ifdef HAVE_TIMES
    struct tms dummy;
    origsec = times(&dummy);
    lastusrsec = dummy.tms_utime;
    lastsyssec = dummy.tms_stime;
    lastsec = origsec;
#else
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    lastsec = tv.tv_sec;
    lastusec = tv.tv_usec;
    origsec = lastsec;
    origusec = lastusec;
#endif
#ifdef HAVE_GETRUSAGE
    struct rusage ruse;
    getrusage(RUSAGE_SELF, &ruse);
    lastusrsec = ruse.ru_utime.tv_sec;
    lastusrusec = ruse.ru_utime.tv_usec;
    lastsyssec = ruse.ru_stime.tv_sec;
    lastsysusec = ruse.ru_stime.tv_usec;
#endif
#endif
}


namespace {
    // Return the variable struct if name matches a circuit variable. 
    // The circuit variables are set on the .options line.
    //
    variable *get_cvar(const char *name)
    {
        if (!Sp.CurCircuit())
            return (0);
        for (variable *v = Sp.CurCircuit()->vars(); v; v = v->next()) {
            if (lstring::eq(name, v->name()))
                return (v);
        }
        return (0);
    }
}


// Function to print spice options listed in wl.  If retstr is not 0,
// output will be placed in this string.  If wl is 0, print the
// circuit variables.  The keyword "all" specifies to print the following
// entries from the database, or the entire database, if no words follow
// "all".  Otherwise, values are obtained from the circuit variables.
// The database variables are updated every time a simulation is run,
// and may be set by values obtained from the shell, which are not
// reflected in the circuit variable values listed.
//
void
IFsimulator::ShowOption(wordlist *wl, char **retstr)
{
    if (ft_curckt && ft_curckt->runckt()) {
        sLstr *plstr = 0;
        if (!retstr)
            TTY.send("\n");
        else
            plstr = new sLstr;
        if (!wl) {
            for (variable *v = ft_curckt->vars(); v; v = v->next())
                ResPrint::print_cktvar(v, plstr);
        }
        else {
            bool allseen = false;
            for (; wl; wl = wl->wl_next) {
                char *c = lstring::copy(wl->wl_word);
                CP.Unquote(c);
                variable *v;
                if (lstring::cieq(c, "all")) {
                    allseen = true;
                    if (!wl->wl_next)
                        ResPrint::print_opt(0, plstr);
                }
                else if (!allseen && (v = get_cvar(c)) != 0)
                    ResPrint::print_cktvar(v, plstr);
                else
                    ResPrint::print_opt(c, plstr);
                delete [] c;
            }
        }
        if (retstr)
            *retstr = plstr ? plstr->string_trim() : 0;
        delete plstr;
    }
    else
        GRpkg::self()->ErrPrintf(ET_WARN, "no active circuit available.\n");
}
// End of IFsimulator functions.


// Print out one piece of resource usage information.
//
void
ResPrint::print_res(const char *name, sLstr *plstr)
{
    char buf[BSIZE_SP];
    const char *fmtf = "%-15s%-14g%s\n";
    if (!name || lstring::eq(name, kw_totaltime)) {

#ifdef HAVE_TIMES
        struct tms ruse;
        long realt;

        realt = times(&ruse);
        snprintf(buf, sizeof(buf), fmtf, kw_totaltime,
            (double)(realt - origsec)/HZ +
            (double)((realt - origsec)%HZ)/HZ,
            "Total elapsed seconds");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);

        snprintf(buf, sizeof(buf), fmtf, "",
            (double)ruse.tms_utime/HZ +
            (double)(ruse.tms_utime%HZ)/HZ,
            "Total user cpu seconds");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);

        snprintf(buf, sizeof(buf), fmtf, "",
            (double)ruse.tms_stime/HZ +
            (double)(ruse.tms_stime%HZ)/HZ,
            "Total system cpu seconds");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);
#else
#ifdef HAVE_GETTIMEOFDAY
        struct timeval tv;
        struct timezone tz;

        gettimeofday(&tv, &tz);
        snprintf(buf, sizeof(buf), fmtf, kw_totaltime,
            (double)(tv.tv_sec - origsec) +
            (double)(tv.tv_usec - origusec)/1.0e6,
            "Total elapsed seconds");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);
#endif
#ifdef HAVE_GETRUSAGE
        struct rusage ruse;
        getrusage(RUSAGE_SELF, &ruse);
        snprintf(buf, sizeof(buf), fmtf,
#ifndef HAVE_GETTIMEOFDAY
            kw_totaltime,
#else
            "",
#endif
            (double)ruse.ru_utime.tv_sec +
            (double)ruse.ru_utime.tv_usec/1.0e6,
            "Total user cpu seconds");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);

        snprintf(buf, sizeof(buf), fmtf, "",
            (double)ruse.ru_stime.tv_sec +
            (double)ruse.ru_stime.tv_usec/1.0e6,
            "Total system cpu seconds");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);
#endif
#endif
        if (name) return;
    }

    if (!name || lstring::eq(name, kw_elapsed)) {

#ifdef HAVE_TIMES
        struct tms ruse;
        long realt;

        realt = times(&ruse);
        snprintf(buf, sizeof(buf), fmtf, kw_elapsed,
            (double)(realt - lastsec)/HZ +
            (double)((realt - lastsec)%HZ)/HZ,
            "Seconds since last call");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);

        snprintf(buf, sizeof(buf), fmtf, "",
            (double)(ruse.tms_utime - lastusrsec)/HZ +
            (double)((ruse.tms_utime - lastusrsec)%HZ)/HZ,
            "User cpu seconds since last call");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);

        snprintf(buf, sizeof(buf), fmtf, "",
            (double)(ruse.tms_stime - lastsyssec)/HZ +
            (double)((ruse.tms_stime - lastsyssec)%HZ)/HZ,
            "System cpu seconds since last call");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);
        lastusrsec = ruse.tms_utime;
        lastsyssec = ruse.tms_stime;
        lastsec = realt;
#else
#ifdef HAVE_GETTIMEOFDAY
        struct timeval tv;
        struct timezone tz;

        gettimeofday(&tv, &tz);
        snprintf(buf, sizeof(buf), fmtf, kw_elapsed,
            (double)(tv.tv_sec - lastsec) +
            (double)(tv.tv_usec - lastusec)/1.0e6,
            "Seconds since last call");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);
        lastsec = tv.tv_sec;
        lastusec = tv.tv_usec;
#endif
#ifdef HAVE_GETRUSAGE
        struct rusage ruse;
        getrusage(RUSAGE_SELF, &ruse);
        snprintf(buf, sizeof(buf), fmtf,
#ifndef HAVE_GETTIMEOFDAY
            kw_elapsed,
#else
            "",
#endif
            (double)(ruse.ru_utime.tv_sec - lastusrsec) +
            (double)(ruse.ru_utime.tv_usec - lastusrusec)/1.0e6,
            "User cpu seconds since last call");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);
        lastusrsec = ruse.ru_utime.tv_sec;
        lastusrusec = ruse.ru_utime.tv_usec;

        snprintf(buf, sizeof(buf), fmtf, "",
            (double)(ruse.ru_stime.tv_sec - lastsyssec) +
            (double)(ruse.ru_stime.tv_usec - lastsysusec)/1.0e6,
            "System cpu seconds since last call");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);
        lastsyssec = ruse.ru_stime.tv_sec;
        lastsysusec = ruse.ru_stime.tv_usec;
#endif
#endif
        if (name)
            return;
    } 

    if (!name || lstring::eq(name, kw_space)) {
#if defined(HAVE_GETRLIMIT) && defined(RLIMIT_DATA)
        const char *fmtd = "%-15s%-14d%s\n";
        const char *fmtg = "%-15s%-14g%s\n";
        struct rlimit rld;
        getrlimit(RLIMIT_DATA, &rld);
#ifdef HAVE_LOCAL_ALLOCATOR
        double szkb = Memory()->coresize();
#else
        double szkb = coresize();
#endif
        snprintf(buf, sizeof(buf), fmtg, kw_space, szkb,
            "Current data size KB");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);

        if (rld.rlim_max != RLIM_INFINITY) {
            snprintf(buf, sizeof(buf), fmtd, "", (unsigned)rld.rlim_max,
                "Hard data limit");
            if (plstr)
                plstr->add(buf);
            else
                TTY.send(buf);
        }

        if (rld.rlim_cur != RLIM_INFINITY) {
            snprintf(buf, sizeof(buf), fmtd, "", (unsigned)rld.rlim_cur,
                "Soft data limit");
            if (plstr)
                plstr->add(buf);
            else
                TTY.send(buf);
        }
#else
#ifdef HAVE_ULIMIT
        const char *fmtd = "%-15s%-14d%s\n";
        const char *fmtg = "%-15s%-14g%s\n";
        long lim = ulimit(3, 0L);
#ifdef HAVE_LOCAL_ALLOCATOR
        double szkb = Memory()->coresize();
#else
        double szkb = coresize();
#endif
        snprintf(buf, sizeof(buf), fmtg, kw_space, szkb,
            "Current data size KB");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);

        snprintf(buf, sizeof(buf), fmtd, "", lim, "Data limit");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);
#endif
#endif
        if (name) return;
    }

    if (!name || lstring::eq(name, kw_faults)) {
#ifdef HAVE_GETRUSAGE
        const char *fmtd = "%-15s%-14d%s\n";
        struct rusage ruse;

        getrusage(RUSAGE_SELF, &ruse);
        snprintf(buf, sizeof(buf), fmtd, kw_faults, ruse.ru_majflt,
            "Page Faults");
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);

        snprintf(buf, sizeof(buf), "%-15s%-14ld%s (vol %ld + invol %ld)\n",
            "", ruse.ru_nvcsw + ruse.ru_nivcsw, "Context switches",
            ruse.ru_nvcsw, ruse.ru_nivcsw);
        if (plstr)
            plstr->add(buf);
        else
            TTY.send(buf);
#endif
        if (name)
            return;
    }
    if (!name) {
        if (plstr)
            plstr->add_c('\n');
        else
            TTY.send("\n");
    }
    if (Sp.CurCircuit() && Sp.CurCircuit()->runckt())
        print_stat(name, plstr);
}


void
ResPrint::print_var(const variable *v, sLstr *plstr)
{
    if (!v)
        return;
    const char *fmtf = "%-15s%-14g%s\n";
    const char *fmtd = "%-15s%-14d%s\n";
    const char *fmts = "%-15s%-14s%s\n";
    char buf[BSIZE_SP];

    switch (v->type()) {
    case VTYP_BOOL:
        if (plstr) {
            snprintf(buf, BSIZE_SP, fmtd, v->name(), v->boolean(),
                v->reference());
            plstr->add(buf);
        }
        else
            TTY.printf(fmtd, v->name(), v->boolean(), v->reference());
        break;
    case VTYP_NUM:
        if (plstr) {
            snprintf(buf, BSIZE_SP, fmtd, v->name(), v->integer(),
                v->reference());
            plstr->add(buf);
        }
        else
            TTY.printf(fmtd, v->name(), v->integer(), v->reference());
        break;
    case VTYP_REAL:
        if (plstr) {
            snprintf(buf, BSIZE_SP, fmtf, v->name(), v->real(),
                v->reference());
            plstr->add(buf);
        }
        else
            TTY.printf(fmtf, v->name(), v->real(), v->reference());
        break;
    case VTYP_STRING:
        if (plstr) {
            snprintf(buf, BSIZE_SP, fmts, v->name(), v->string(),
                v->reference());
            plstr->add(buf);
        }
        else
            TTY.printf(fmts, v->name(), v->string(), v->reference());
        break;
    default:
        break;
    }
}


namespace {
    // Remove from the list and return the named variable.
    //
    variable *snipvar(variable **pv, const char *name)
    {
        if (!name)
            return (0);
        variable *vp = 0;
        for (variable *v = *pv; v; v = v->next()) {
            if (!strcmp(v->name(), name)) {
                if (vp)
                    vp->set_next(v->next());
                else
                    *pv = v->next();
                v->set_next(0);
                return (v);
            }
            vp = v;
        }
        return (0);
    }
}


void
ResPrint::print_stat(const char *name, sLstr *plstr)
{
    if (name && !strcmp(name, kw_stats))
        name = 0;
    variable *v = 0;
    if (Sp.CurCircuit() && Sp.CurCircuit()->runckt())
        v = STATinfo.getOpt(Sp.CurCircuit()->runckt(), 0, name);
    else
        return;
    if (name) {
        if (!v) {
            GRpkg::self()->ErrPrintf(ET_WARN, "unknown stat keyword %s.\n",
                name);
        }
        else {
            print_var(v, plstr);
            variable::destroy(v);
        }
        return;
    }
    // The v list is all stats in alphabetical keyorder.  We prefer
    // a more friendly order.

    for (int i = 0; ; i++) {
        const char *kw = stat_print_array[i];  // from statsetp.cc
        if (!kw)
            break;
        if (!*kw) {
            if (plstr)
                plstr->add_c('\n');
            else
                TTY.send("\n");
            continue;
        }
        variable *vv = snipvar(&v, kw);
        if (!vv) {
            // "can't happen"
            continue;
        }
        print_var(vv, plstr);
        delete vv;
    }
    if (v) {
        // There shouldn't be any variables left.
        for (variable *vv = v; vv; vv = vv->next())
            print_var(vv, plstr);
        variable::destroy(v);
    }
    if (!plstr)
        TTY.send("\n");
}


// Print the information on the variable name, to plstr if not 0. 
// The name is a spice option.  The information is derived from
// the options database.  Only the spice options that are IF_ASK
// are listed, and the values listed come from the database, which
// is updated before a simulation.  Thus, before a circuit is run,
// the default values will be printed.
//
void
ResPrint::print_opt(const char *name, sLstr *plstr)
{
    variable *v = 0;
    if (Sp.CurCircuit() && Sp.CurCircuit()->runckt()) {
        sCKT *ckt = Sp.CurCircuit()->runckt();
        v = OPTinfo.getOpt(ckt, 0, name);
    }
    else
        return;
    if (v) {
        for (variable *vv = v; vv; vv = vv->next())
            print_var(vv, plstr);
        variable::destroy(v);
    }
    else if (name)
        GRpkg::self()->ErrPrintf(ET_WARN, "no such option %s.\n", name);
}


// Print the value of the circuit variable, to plstr if not 0.  The
// values are as set in the .options line.
//
void
ResPrint::print_cktvar(const variable *v, sLstr *plstr)
{
    if (!v)
        return;

    char buf[BSIZE_SP];
    const char *fmtf = "%-15s%-14g%s\n";
    const char *fmtd = "%-15s%-14d%s\n";
    const char *fmts = "%-15s%-14s%s\n";

    const char *descr = 0;
    for (int i = 0; i < OPTinfo.numParms; i++) {
        IFparm *p = OPTinfo.analysisParms + i;
        if (lstring::cieq(v->name(), p->keyword)) {
            descr = p->description;
            break;
        }
    }
    if (!descr)
        descr = "";

    switch (v->type()) {
    case VTYP_BOOL:
        if (plstr) {
            snprintf(buf, BSIZE_SP, fmts, v->name(), "", descr);
            plstr->add(buf);
        }
        else
            TTY.printf(fmts, v->name(), "", descr);
        break;
    case VTYP_NUM:
        if (plstr) {
            snprintf(buf, BSIZE_SP, fmtd, v->name(), v->integer(), descr);
            plstr->add(buf);
        }
        else
            TTY.printf(fmtd, v->name(), v->integer(), descr);
        break;
    case VTYP_REAL:
        if (plstr) {
            snprintf(buf, BSIZE_SP, fmtf, v->name(), v->real(), descr);
            plstr->add(buf);
        }
        else
            TTY.printf(fmtf, v->name(), v->real(), descr);
        break;
    case VTYP_STRING:
        if (plstr) {
            snprintf(buf, BSIZE_SP, fmts, v->name(), v->string(), descr);
            plstr->add(buf);
        }
        else
            TTY.printf(fmts, v->name(), v->string(), descr);
        break;
    default:
        break;
    }
}


// Get the elapsed real, user, and cpu times.  If not available the
// returned time is -1.0,
//
void
ResPrint::get_elapsed(double *elapsed, double *user, double *cpu)
{
    if (elapsed)
        *elapsed = 0.0;
    if (user)
        *user = 0.0;
    if (cpu)
        *cpu = 0.0;
#ifdef HAVE_TIMES
    struct tms ruse;
    long realt = times(&ruse);
    if (elapsed) {
        *elapsed = (double)(realt - origsec)/HZ +
            (double)((realt - origsec)%HZ)/HZ;
    }
    if (user) {
        *user = (double)ruse.tms_utime/HZ +
            (double)(ruse.tms_utime%HZ)/HZ;
    }
    if (cpu) {
        *cpu = (double)ruse.tms_stime/HZ +
            (double)(ruse.tms_stime%HZ)/HZ;
    }
#else
#ifdef HAVE_GETTIMEOFDAY
    if (elapsed) {
        struct timeval tv;
        struct timezone tz;
        gettimeofday(&tv, &tz);
        *elapsed = (tv.tv_sec - origsec) + (tv.tv_usec - origusec)/1.0e6;
    }
#endif
#ifdef HAVE_GETRUSAGE
    if (user || cpu) {
        struct rusage ruse;
        getrusage(RUSAGE_SELF, &ruse);
        if (user)
            *user = ruse.ru_utime.tv_sec + ruse.ru_utime.tv_usec/1.0e6;
        if (cpu)
            *cpu = ruse.ru_stime.tv_sec + ruse.ru_stime.tv_usec/1.0e6;
    }
#endif
#endif
}


void
ResPrint::get_faults(unsigned int *fptr, unsigned int *cxptr)
{
    if (fptr)
        *fptr = 0;
    if (cxptr)
        *cxptr = 0;
#ifdef HAVE_GETRUSAGE
    struct rusage ruse;
    getrusage(RUSAGE_SELF, &ruse);
    if (fptr)
        *fptr = ruse.ru_majflt;
    if (cxptr)
        *cxptr = ruse.ru_nvcsw + ruse.ru_nivcsw;
#endif
}


// Get the current data size, and the hard and soft limits.
//
void
ResPrint::get_space(unsigned *data, unsigned *hlimit, unsigned *slimit)
{
    if (data)
        *data = 0.0;
    if (hlimit)
        *hlimit = 0.0;
    if (slimit)
        *slimit = 0.0;
#ifdef WIN32
    if (hlimit || slimit) {
        MEMORYSTATUS mem;
        memset(&mem, 0, sizeof(MEMORYSTATUS));
        GlobalMemoryStatus(&mem);
        if (slimit)
            *slimit = mem.dwAvailPhys + mem.dwAvailPageFile;
        if (hlimit)
            *hlimit = *slimit;
    }

    if (data) {
        unsigned d = 0;
        MEMORY_BASIC_INFORMATION m;
        for (char *ptr = 0; ptr < (char*)0x7ff00000; ptr += m.RegionSize) {
            VirtualQuery(ptr, &m, sizeof(m));
            if (m.AllocationProtect == PAGE_READWRITE &&
                    m.State == MEM_COMMIT &&
                    m.Type == MEM_PRIVATE)
                d += m.RegionSize;
        }
        *data = d;
    }

#else

    if (data) {
#ifdef HAVE_LOCAL_ALLOCATOR
        *data = (unsigned int)(1024*Memory()->coresize());
#else
        *data = (unsigned int)(1024*coresize());
#endif
    }

    if (hlimit || slimit) {
#if defined(HAVE_GETRLIMIT) && defined(RLIMIT_DATA)
        struct rlimit rld;
        getrlimit(RLIMIT_DATA, &rld);
        if (hlimit)
            *hlimit = rld.rlim_max;
        if (slimit)
            *slimit = rld.rlim_cur;
#else
#ifdef HAVE_ULIMIT
        if (hlimit)
            *hlimit = ulimit(3, 0L);
#endif // HAVE_ULIMIT
#endif // defined(HAVE_GETRLIMIT) && defined(RLIMIT_DATA)
    }
#endif // WIN32
}

