
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
#include "spglobal.h"
#include "input.h"
#include "frontend.h"
#include "ftedata.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "subexpand.h"
#include "miscutil/random.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "ginterf/graphics.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#include "help/help_topic.h"
#endif
#ifdef WIN32
#include "miscutil/msw.h"
#endif

#ifdef HAVE_LOCAL_ALLOCATOR
#include "malloc/local_malloc.h"
#endif

#define SYSTEM_MAIL       "mail -s \"%s (%s) Bug Report\" %s"


// Quick help, just list the command description string.
//
void
CommandTab::com_qhelp(wordlist *wl)
{
    bool allflag = false;
    if (wl && lstring::cieq(wl->wl_word, "all")) {
        allflag = true;
        wl = 0;
    }

    // We want to use more mode whether "moremode" is set or not.
    bool tmpmm = TTY.morestate();
    TTY.setmore(true);
    TTY.init_more();
    if (wl == 0) {

        // determine environment
        int env = 0;
        if (Sp.PlotList()->next_plot())
            env |= E_HASPLOTS;
        else
            env |= E_NOPLOTS;

        // determine level
        int level = E_INTERMED;
        VTvalue vv;
        if (Sp.GetVar(kw_level, VTYP_STRING, &vv) && vv.get_string()) {
            switch (*vv.get_string()) {
            case 'b':   level = E_BEGINNING;
                break;
            case 'i':   level = E_INTERMED;
                break;
            case 'a':   level = E_ADVANCED;
                break;
            }
        }

        // Sort the commands
        sCommand **cc;
        int numcoms = Cmds.Commands(&cc);
        TTY.send("\n");
        for (int i = 0; i < numcoms; i++) {
            if (allflag || ((cc[i]->co_env <= (unsigned)level) &&
                    (!(cc[i]->co_env & (E_BEGINNING-1)) ||
                    (env & cc[i]->co_env)))) {
                if (cc[i]->co_help == 0)
                    continue;
                TTY.printf("%s ", cc[i]->co_comname);
                TTY.printf(cc[i]->co_help, CP.Program());
                TTY.send("\n");
            }
        }
        delete [] cc;
    }
    else {
        for ( ; wl; wl = wl->wl_next) {
            sCommand *c = Cmds.FindCommand(wl->wl_word);
            if (c) {
                TTY.printf("%s ", c->co_comname);
                TTY.printf(c->co_help, CP.Program());
                TTY.send("\n");
            }
            else {
                // See if this is aliased
                sAlias *al = CP.GetAlias(wl->wl_word);
                if (al == 0)
                    TTY.printf("Sorry, no help for %s.\n", wl->wl_word);
                else {
                    TTY.printf("%s is aliased to ", wl->wl_word);
                    TTY.wlprint(al->text());
                    TTY.send("\n");
                }
            }
        }
    }
    TTY.send("\n");
    TTY.setmore(tmpmm);
}


// The main help command, accesses the help database.
//
void
CommandTab::com_help(wordlist *wl)
{
#ifdef HAVE_MOZY
    VTvalue vv;
    if (Sp.GetVar(kw_helpinitxpos, VTYP_NUM, &vv))
        HLP()->set_init_x(vv.get_int());
    if (Sp.GetVar(kw_helpinitypos, VTYP_NUM, &vv))
        HLP()->set_init_y(vv.get_int());
    if (!wl || !wl->wl_word || !*wl->wl_word) {
        // top of help tree
        static wordlist *wdef;
        //
        // Have to be a little careful.  If we give "wrspice" as the
        // topic, and the help database isn't found, we could load the
        // wrspice executable into the browser if it happens to exist
        // in the cwd.  The "wrspice_top_topic" is aliased to the
        // wrspice topic in the help database.
        //
        if (!wdef)
            wdef = new wordlist("wrspice_top_topic", 0);
        wl = wdef;
    }
    else if (!wl->wl_next && lstring::eq(wl->wl_word, "-c")) {
        HLP()->rehash();
        return;
    }

    // If multiple words are given, the window shows the last word,
    // and the back button will cycle through the words in reverse
    // order.

    HLPwords *h0 = 0, *he = 0;
    for ( ; wl; wl = wl->wl_next) {
        if (!h0)
            h0 = he = new HLPwords(wl->wl_word, 0);
        else {
            he->next = new HLPwords(wl->wl_word, he);
            he = he->next;
        }
    }
    HLP()->list(h0);
    HLPwords::destroy(h0);

    const char *err = HLP()->error_msg();
    if (err)
        GRpkgIf()->ErrPrintf(ET_ERROR, err);
#else
    (void)wl;
    GRpkgIf()->ErrPrintf(ET_MSG,
        "Help system is not available in this executable.");
#endif
}


void
CommandTab::com_helpreset(wordlist*)
{
#ifdef HAVE_MOZY
    HLP()->rehash();
#else
    GRpkgIf()->ErrPrintf(ET_MSG,
        "Help system is not available in this executable.");
#endif
}


void
CommandTab::com_quit(wordlist*)
{
    const char *zz = "Are you sure you want to quit [y]? ";
    bool noask = Sp.GetVar(kw_noaskquit, VTYP_BOOL, 0);
    
    // Confirm
    if (!noask && !Sp.GetFlag(FT_BATCHMODE) && !CP.GetFlag(CP_NOTTYIO)) {
        int ncc = 0;
        for (sFtCirc *cc = Sp.CircuitList(); cc; cc = cc->next())
            if (cc->inprogress())
                ncc++;
        int npl = 0;
        for (sPlot *pl = Sp.PlotList(); pl; pl = pl->next_plot())
            if (!pl->written() && pl->num_perm_vecs())
                npl++;
        if (ncc || npl) {
            TTY.init_more();
            TTY.printf("Warning: ");
            if (ncc) {
                TTY.printf( 
            "the following simulation%s still in progress:\n",
                        (ncc > 1) ? "s are" : " is");
                for (sFtCirc *cc = Sp.CircuitList(); cc; cc = cc->next())
                    if (cc->inprogress())
                        TTY.printf("\t%s\n", cc->name());
            }
            if (npl) {
                if (ncc)
                    TTY.printf("and ");
                TTY.printf("the following plot%s been saved:\n",
                    (npl > 1) ? "s haven't" : " hasn't");
                for (sPlot *pl = Sp.PlotList(); pl; pl = pl->next_plot())
                    if (!pl->written() && pl->num_perm_vecs())
                        TTY.printf("%s\t%s, %s\n",
                            pl->type_name(), pl->title(), pl->name());
            }
            if (!TTY.prompt_for_yn(true, zz))
                return;
        }
    }
    fatal(false);
}


void
CommandTab::com_bug(wordlist*)
{
    if (!Global.BugAddr() || !*Global.BugAddr()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no address to send bug reports to.\n");
            return;
    }
    const char *msg =
        "Please include the OS version number and machine architecture.\n"
        "If the problem is with a specific circuit, please include the\n"
        "input file.\n\n";
    char buf[BSIZE_SP];
    if (GRpkgIf()->CurDev()) {
        TTY.printf(msg);
        sprintf(buf, "WRspice %s bug report", Sp.Version());
        GRwbag *cx = GRpkgIf()->MainDev()->NewWbag("mail", 0);
        cx->SetCreateTopLevel();
        cx->PopUpMail(buf, Global.BugAddr());
    }
    else {
        TTY.printf("Calling the mail program . . .(sending to %s)\n\n",
            Global.BugAddr());
        TTY.printf(msg);
        TTY.printf(
    "You are in the mail program.  Type your message, end with \".\"\n"
    "on its own line.\n\n");

        sprintf(buf, SYSTEM_MAIL, Sp.Simulator(), Sp.Version(),
            Global.BugAddr());
        CP.System(buf);
        TTY.printf("Bug report sent.  Thank you.\n");
    }
}


void
CommandTab::com_version(wordlist *wl)
{
    TTY.init_more();
    if (!wl) {
        TTY.printf("Program: %s, version: %s\n", Sp.Simulator(),
            Sp.Version());
        if (Global.Notice() && *Global.Notice())
            TTY.printf("\t%s\n", Global.Notice());
        if (Global.BuildDate() && *Global.BuildDate())
            TTY.printf("Date built: %s\n", Global.BuildDate());
    }
    else {
        // assume version in form major.minor.rev
        char *s = wordlist::flatten(wl);
        int v1, v2, v3;
        int r1 = sscanf(s, "%d.%d.%d", &v1, &v2, &v3);
        int c1, c2, c3;
        int r2 = sscanf(Sp.Version(), "%d.%d.%d", &c1, &c2, &c3);
        if (r1 != r2 || (r1 >= 1 && v1 > c1) || (r1 >= 2 && v2 > c2) ||
                (r1 == 3 && v3 > c3)) {
            GRpkgIf()->ErrPrintf(ET_WARN,
                "rawfile is version %s (current version is %s).\n",
                wl->wl_word, Sp.Version());
        }
        delete [] s;
    }
}


void
CommandTab::com_seed(wordlist *wl)
{
    int seed;
    if (!wl || !wl->wl_word || sscanf(wl->wl_word, "%d", &seed) != 1)
#ifdef HAVE_GETPID
        seed = getpid();
#else
        seed = 17;
#endif
    Rnd.seed(seed);
}


void
CommandTab::com_wrupdate(wordlist*)
{
#ifdef HAVE_MOZY
    wordlist wl;
    char buf[16];
    strcpy(buf, ":xt_pkgs");
    wl.wl_word = buf;
    com_help(&wl);
#else
    GRpkgIf()->ErrPrintf(ET_MSG,
        "Package manager is not available in this executable.");
#endif
}


namespace {
    void pr_usage()
    {
        TTY.printf("Usage:\n"
        "\tcache help\n"
        "\tcache l[ist]\n"
        "\tcache d[ump] [tags...]\n"
        "\tcache r[emove] [tags...]\n"
        "\tcache c[lear]\n");
    }

    void pr_tags()
    {
        wordlist *tl = SPcache.listCache();
        if (!tl)
            TTY.printf("Subcircuit/model cache is empty.\n");
        else {
            TTY.printf("Subcircuit/model cache tags:\n");
            for (wordlist *w = tl; w; w = w->wl_next)
                TTY.printf("\t%s\n", w->wl_word);
            TTY.printf("\n");
        }
        wordlist::destroy(tl);
    }
}


void
CommandTab::com_cache(wordlist *wl)
{
    // cache [help|list|dump|remove|clear] [name]

    if (!wl) {
        pr_tags();
        return;
    }

    const char *word = wl->wl_word;
    wl = wl->wl_next;

    if (lstring::cieq(word, "h") || lstring::cieq(word, "?") ||
            lstring::cieq(word, "help")) {
        pr_usage();
    }
    else if (lstring::cieq(word, "l") || lstring::cieq(word, "list")) {
        pr_tags();
    }
    else if (lstring::cieq(word, "d") || lstring::cieq(word, "dump")) {
        if (!wl) {
            wordlist *tl = SPcache.dumpCache(0);
            if (!tl)
                TTY.printf("Subcircuit/model cache is empty.\n");
            else {
                for (wordlist *w = tl; w; w = w->wl_next)
                    TTY.printf("%s\n", w->wl_word);
                TTY.printf("\n");
                wordlist::destroy(tl);
            }
        }
        else {
            for (wordlist *w = wl; w; w = w->wl_next) {
                if (!SPcache.inCache(w->wl_word))
                    TTY.printf("Unknown tag %s.\n", w->wl_word);
                else {
                    wordlist *tl = SPcache.dumpCache(w->wl_word);
                    for (wordlist *t = tl; t; t = t->wl_next)
                        TTY.printf("%s\n", t->wl_word);
                    wordlist::destroy(tl);
                }
            }
        }
    }
    else if (lstring::cieq(word, "r") || lstring::cieq(word, "remove")) {
        if (!wl)
            TTY.printf("No tags given to remove.\n");
        else {
            for (wordlist *w = wl; w; w = w->wl_next) {
                if (!SPcache.inCache(w->wl_word))
                    TTY.printf("Unknown tag %s.\n", w->wl_word);
                else {
                    SPcache.removeCache(w->wl_word);
                    TTY.printf("Entry %s deleted.\n", w->wl_word);
                }
            }
        }
    }
    else if (lstring::cieq(word, "c") || lstring::cieq(word, "clear")) {
        SPcache.clearCache();
        TTY.printf("Subcircuit/model cache cleared.\n");
    }
    else {
        TTY.printf("Unknown directive.\n");
        pr_usage();
    }
}


// Undocumented interface to the allocation monitor, for core-leak
// hunting.  This does not work in production releases, the monitor
// must be enabled in the memory system.
//
// mmom start [depth]
// mmon stop [filename]
// mmon [status | check]
//
void
CommandTab::com_mmon(wordlist *wl)
{
#ifdef HAVE_LOCAL_ALLOCATOR
    if (wl && wl->wl_word) {
        if (lstring::cieq(wl->wl_word, "start")) {
            int depth = 4;
            if (wl->wl_next)
                depth = atoi(wl->wl_next->wl_word);
            if (depth < 1 || depth > 15) {
                TTY.printf(
                    "Error: depth must be in range 1-15 (default 4).\n");
                return;
            }
            if (!Memory()->mon_start(depth)) {
                TTY.printf(
                    "Error: memory monitor failed to start.\n");
                return;
            }
            TTY.printf(
                "Memory monitor started.\n");
            return;
        }
        if (lstring::cieq(wl->wl_word, "stop")) {
            Sp.VecGc();
            const char *fname = "mon.out";
            if (wl->wl_next)
                fname = wl->wl_next->wl_word;
            if (!Memory()->mon_stop()) {
                TTY.printf(
                    "Memory monitor is inactive.\n");
                return;
            }
            if (!Memory()->mon_dump(fname)) {
                TTY.printf(
                    "Error: data dump to file failed, monitor inactive.\n");
                return;
            }
            TTY.printf(
                "Memory monitor stopped, data in file \"%s\".\n", fname);
            return;
        }
        if (!lstring::cieq(wl->wl_word, "status") &&
                !lstring::cieq(wl->wl_word, "check")) {
            return;
        }
    }
    TTY.printf(
        "Allocation table contains %d entries.\n", Memory()->mon_count());
#else
    (void)wl;
    TTY.printf(
        "Allocation monitor not available.\n");
#endif
}

