
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

#include "simulator.h"
#include "graph.h"
#include "runop.h"
#include "output.h"
#include "cshell.h"
#include "commands.h"
#include "inpline.h"
#include "input.h"
#include "spnumber/spnumber.h"


//
// Spice-2 compatibility stuff for .plot, .print, .four, and .width.
//

namespace {
    void fixdotplot(wordlist**, bool);
    bool setcplot(const char*);
}


// Extract the .option cards from the deck...
//
sLine *
IFsimulator::GetDotOpts(sLine *deck)
{
    // Here we do .TEMP handling as follows.  If a .TEMP line is
    // found, set a dummy "temp = xxx" option at the beginning of the
    // options list, using the first temperature given in the .TEMP
    // line.

    char *dummytemp = 0;

    sLine *last = deck, *next;
    sLine *opts = 0, *oend = 0;
    for (sLine *dd = deck->next(); dd; dd = next) {
        next = dd->next();
        if (lstring::cimatch(OPTIONS_KW, dd->line()) ||
                lstring::cimatch(OPTION_KW, dd->line()) ||
                lstring::cimatch(OPT_KW, dd->line())) {
            dd->fix_line();
            last->set_next(next);
            dd->set_next(0);
            if (opts) {
                oend->set_next(dd);
                oend = dd;
            }
            else
                opts = oend = dd;
            continue;
        }
        if (!dummytemp && lstring::cimatch(TEMP_KW, dd->line())) {
            // We'll leave the .TEMP line in the deck, extract the
            // first temperature and make sure it is a float.

            const char *line = dd->line();
            char *token = IP.getTok(&line, true);
            if (token) {
                delete [] token;
                token = IP.getTok(&line, true);
                if (token) {
                    const char *t = token;
                    int error = 0;
                    IP.getFloat(&t, &error, true);
                    if (!error)
                        dummytemp = token;
                    else
                        delete [] token;
                }
            }
        }
        last = dd;
    }
    if (dummytemp) {
        // Add a dummy option to the front of the list.

        char buf[256];
        sprintf(buf, ".options temp=%s", dummytemp);
        delete [] dummytemp;
        sLine *dd = new sLine;
        dd->set_line(buf);
        dd->set_next(opts);
        opts = dd;
    }
    return (opts);
}


// Extract all the .save lines.  If ".save output", extract saves from
// .plot, .print, .four lines.  The keyword ".probe" is a synonym for
// ".save".
//
void
IFsimulator::GetDotSaves()
{
    if (!ft_curckt) // Shouldn't happen
        return;
    wordlist *iline, *wl = 0;
    for (iline = ft_curckt->commands(); iline; iline = iline->wl_next) {
        if (lstring::cimatch(SAVE_KW, iline->wl_word) ||
                lstring::cimatch(PROBE_KW, iline->wl_word)) {
            char *s = iline->wl_word;
            lstring::advtok(&s);
            if (lstring::cieq(s, "output")) {
                SaveDotArgs();
                lstring::advtok(&s);
            }
            wordlist *w = CP.LexString(s);
            if (w)
                wl = wordlist::append(wl, w);
        }
    }
    if (wl) {
        CommandTab::com_save(wl);
        wordlist::destroy(wl);
    }
    // If there aren't any .saves, all vectors will be saved, 
    // which is the default action for rawfiles when com_save()
    // isn't called.
}


// Go through the dot lines given and make up a big "save" command with
// all the node names mentioned. Note that if a node is requested for
// one analysis, it is saved for all of them.
//
void
IFsimulator::SaveDotArgs()
{
    if (!ft_curckt) // Shouldn't happen
        return;
    wordlist *wl = 0, *iline;
    for (iline = ft_curckt->commands(); iline; iline = iline->wl_next) {
        char *s = iline->wl_word;
        if (lstring::cimatch(PRINT_KW, s) || lstring::cimatch(PLOT_KW, s) ||
                lstring::cimatch(FOUR_KW, s)) {
            lstring::advtok(&s);
            lstring::advtok(&s);
            wordlist *w = CP.LexString(s);
            if (!w) {
                GRpkg::self()->ErrPrintf(ET_WARN, "no nodes given: %s.\n",
                    iline->wl_word);
            }
            else
                wl = wordlist::append(wl, w);
        }
    }
    if (wl) {
        fixdotplot(&wl, true);
        if (wl) {
            CommandTab::com_save(wl);
            wordlist::destroy(wl);
        }
    }
    else {
        GRpkg::self()->ErrPrintf(ET_WARN,
            "no .print, .plot, or .four lines in input.\n");
    }
}


// Run in batch mode, then execute the .whatever lines found in the
// deck, after we are done running.  We'll be cheap and use
// CP.LexString to get the words.  This should make us spice-2
// compatible.
//
void
IFsimulator::RunBatch()
{
    if (ft_flags[FT_RAWFGIVEN]) {
        GetDotSaves();
        Run(OP.getOutDesc()->outFile());
    }
    else {
        SaveDotArgs();
        Run(0);  
    }
    if (!ft_curckt)
        return;

    if (ft_curckt->commands()) {
        wordlist *coms = ft_curckt->commands();
        CP.SetFlag(CP_INTERACTIVE, false);

        TTY.init_more();

        // Circuit name/Date/Temperature.
        TTY.printf("%s: %s\nDate: %s\n\n", ft_curckt->name(),
            ft_curckt->descr(), datestring());
        sDataVec *dv = OP.vecGet("temper", ft_curckt->runckt(), true);
        if (dv)
            TTY.printf("Temperature: %.3fC\n", dv->realval(0));
        TTY.printf("\n");

        // In Spice3, some of these things were skipped in the case of
        // a rawfile.  Here, we will skip the plots only.

        // Source listing.
        if (ft_flags[FT_LISTPRNT]) {
            Listing(0, ft_curckt->deck(), ft_curckt->options(), LS_DECK);
            TTY.send("\n\n");
        }

        // Model parameters.
        if (ft_flags[FT_MODSPRNT]) {
            TTY.printf("Models:\n\n");
            wordlist ww;
            ww.wl_word = (char*)"-m";
            CommandTab::com_show(&ww);
        }

        // Device instances.
        if (ft_flags[FT_DEVSPRNT]) {
            TTY.printf("Devices:\n\n");
            wordlist ww;
            ww.wl_word = (char*)"-d";
            CommandTab::com_show(&ww);
        }

        if (setcplot("tf")) {
            TTY.printf("Transfer function information:\n");
            wordlist all;
            all.wl_next = 0;
            all.wl_word = (char*)"all";
            CommandTab::com_print(&all);
            TTY.printf("\n");
        }

        // Now all the '.' lines
        while (coms) {
            wordlist *command = CP.LexString(coms->wl_word);
            if (!command) {
                TTY.printf("\nError:bad command \"%s\".\n\n",
                    coms->wl_word);
                coms = coms->wl_next;
                continue;
            }

            wordlist *cl = command;
            if (lstring::cieq(command->wl_word, WIDTH_KW)) {
                do {
                    command = command->wl_next;
                } while (command &&
                        !lstring::ciprefix("out", command->wl_word));
                if (command) {
                    char *s = strchr(command->wl_word, '=');
                    if (!s || !s[1]) {
                        TTY.printf("Error: bad line %s.\n", coms->wl_word);
                        coms = coms->wl_next;
                        wordlist::destroy(cl);
                        continue;
                    }
                    int i = atoi(++s);
                    SetVar("width", i);
                }
            }
            else if (lstring::cieq(command->wl_word, PRINT_KW)) {
                command = command->wl_next;
                if (!command) {
                    TTY.printf("Error: bad line %s.\n", coms->wl_word);
                    coms = coms->wl_next;
                    wordlist::destroy(cl);
                    continue;
                }
                char *plottype = command->wl_word;
                command = command->wl_next;
                wordlist twl;
                twl.wl_word = (char*)"col";
                twl.wl_next = command;
                if (setcplot(plottype)) 
                    CommandTab::com_print(&twl);
                TTY.printf("\n\n");
            }
            else if (lstring::cieq(command->wl_word, PLOT_KW)) {
                if (!ft_flags[FT_RAWFGIVEN]) {
                    command = command->wl_next;
                    if (!command) {
                        TTY.printf("Error: bad line %s.\n", coms->wl_word);
                        coms = coms->wl_next;
                        wordlist::destroy(cl);
                        continue;
                    }
                    char *plottype = command->wl_word;
                    command = command->wl_next;
                    fixdotplot(&command, false);
                    TTY.init_more();
                    if (command && setcplot(plottype))
                        CommandTab::com_asciiplot(command);
                    TTY.printf("\n\n");
                }
            }
            else if (lstring::cimatch(FOUR_KW, command->wl_word)) {
                if (setcplot("tran")) {
                    CommandTab::com_fourier(command->wl_next);
                    TTY.printf("\n\n");
                }
                else
                    TTY.printf(
                        "No transient data available for fourier analysis");
            }
            else if (!lstring::cieq(command->wl_word, SAVE_KW) &&
                    !lstring::cieq(command->wl_word, PROBE_KW) &&
                    !lstring::cieq(command->wl_word, OP_KW) &&
                    !lstring::cieq(command->wl_word, TF_KW)) {
                TTY.printf("Error:unknown command \"%s\".\n",
                    command->wl_word);
            }
            wordlist::destroy(cl);
            coms = coms->wl_next;
        }
    }

    for (sRunopMeas *m = ft_curckt->measures(); m; m = m->next()) {
        char *s = m->print_meas();
        TTY.printf(s);
        delete [] s;
    }

    // The options.
    if (ft_flags[FT_OPTSPRNT]) {
        TTY.printf("Options:\n\n");
        VarPrint(0);
        TTY.out_printf("\n");
    }

    // And finally the accounting info.
    if (ft_flags[FT_ACCTPRNT]) {
        wordlist ww;
        ww.wl_word = (char*)"everything";
        CommandTab::com_rusage(&ww);
    }
    else
        CommandTab::com_rusage(0);

    CommandTab::com_devcnt(0);

    TTY.printf("\n");
    TTY.printf("%s-%s done.\n\n", Simulator(), Version());
}


// Look through the commands for the n'th plot line matching the type
// given, or of the current plot, and return the vector list.
//
wordlist *
IFsimulator::ExtractPlotCmd(int nth, const char *an)
{
    if (!an && OP.curPlot())
        an = OP.curPlot()->type_name();
    if (!an)
        return (0);
    an = PlotAbbrev(an);
    wordlist *coms = ft_curckt ? ft_curckt->commands() : 0;
    while (coms) {
        if (lstring::cimatch(PLOT_KW, coms->wl_word)) {
            char *s = coms->wl_word;
            lstring::advtok(&s);
            if (lstring::ciprefix(an, s)) {
                if (!nth) {
                    wordlist *wl = CP.LexString(coms->wl_word);
                    wordlist *cmd = wl->wl_next;
                    delete wl;  // '.plot'
                    wl = cmd;
                    cmd = cmd->wl_next;
                    delete wl;  // type
                    cmd->wl_prev = 0;
                    fixdotplot(&cmd, false);
                    return (cmd);
                }
                nth--;
            }
        }
        coms = coms->wl_next;
    }
    return (0);
}


// Look through the commands for the n'th plot line matching the type
// of the current plot, and return the vector list.
//
wordlist *
IFsimulator::ExtractPrintCmd(int nth)
{
    if (OP.curPlot() && ft_curckt) {
        const char *an = OP.curPlot()->type_name();
        an = PlotAbbrev(an);
        wordlist *coms = ft_curckt->commands();
        while (coms) {
            if (lstring::cimatch(PRINT_KW, coms->wl_word)) {
                char *s = coms->wl_word;
                lstring::advtok(&s);
                if (lstring::ciprefix(an, s)) {
                    if (!nth) {
                        wordlist *wl = CP.LexString(coms->wl_word);
                        wordlist *cmd = wl->wl_next;
                        delete wl;  // '.print'
                        wl = cmd;
                        cmd = cmd->wl_next;
                        delete wl;  // type
                        cmd->wl_prev = 0;
                        return (cmd);
                    }
                    nth--;
                }
            }
            coms = coms->wl_next;
        }
    }
    return (0);
}


namespace {
    // This function strips and processes the limits (a, b)
    // specification which can appear in SPICE2 plot lines.  Only the
    // last one is used, and only if nolimits is not set.
    //
    void fixdotplot(wordlist **wlist, bool nolimits)
    {
        wordlist *wl = *wlist;
        while (wl) {
            char *s = strrchr(wl->wl_word, '(');
            if (s && (s == wl->wl_word || *(s-1) == ')')) {
                char buf[BSIZE_SP];
                strcpy(buf, s);
                if (s != wl->wl_word)
                    *s = '\0';
                else {
                    if (wl->wl_prev)
                        wl->wl_prev->wl_next = wl->wl_next;
                    else {
                        // badness, no plot
                        *wlist = 0;
                        return;
                    }
                    if (wl->wl_next)
                        wl->wl_next->wl_prev = wl->wl_prev;
                    wordlist *ww = wl->wl_prev;
                    delete [] wl->wl_word;
                    delete wl;
                    wl = ww;
                }
                if (!strchr(buf, ')')) {
                    wl = wl->wl_next;
                    wordlist *tl = wl;
                    while (!strchr(tl->wl_word, ')'))
                        tl = tl->wl_next;
                    wl->wl_prev->wl_next = tl->wl_next;
                    if (tl->wl_next)
                        tl->wl_next->wl_prev = wl->wl_prev;
                    wordlist *ww = wl->wl_prev;
                    tl->wl_next = 0;
                    for (; wl; wl = tl) {
                        tl = wl->wl_next;
                        strcat(buf, wl->wl_word);
                        delete [] wl->wl_word;
                        delete wl;
                    }
                    wl = ww;
                }
                if (!nolimits && !wl->wl_next) {
                    const char *c = buf + 1;
                    double *d = SPnum.parse(&c, false);
                    while (isspace(*c))
                        c++;
                    if (*c != ',' || !d) {
                        GRpkg::self()->ErrPrintf(ET_WARN,
                            "bad limits \"%s\".\n", buf);
                        return;
                    }
                    double d1 = *d;
                    c++;
                    while (isspace(*c))
                        c++;
                    d = SPnum.parse(&c, false);
                    if (!d) {
                        GRpkg::self()->ErrPrintf(ET_ERROR,
                            "bad limits \"%s\".\n", buf);
                        return;
                    }
                    double d2 = *d;
                    sprintf(buf, "set xlimit = ( %s", SPnum.printnum(d1)); 
                    sprintf(buf + strlen(buf), "%s )", SPnum.printnum(d2));
                    CP.EvLoop(buf);
                }
            }
            wl = wl->wl_next;
        }
    }


    // Don't bother with ccom strangeness here.

    bool setcplot(const char *name)
    {
        for (sPlot *pl = OP.plotList(); pl; pl = pl->next_plot()) {
            if (lstring::ciprefix(name, pl->type_name())) {
                OP.setCurPlot(pl);
                return (true);
            }
        }
        return (false);
    }
}

