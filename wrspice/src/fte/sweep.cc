
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

//
// Functions for loop analysis.
//

#include "frontend.h"
#include "cshell.h"
#include "commands.h"
#include "outplot.h"
#include "outdata.h"
#include "ttyio.h"
#include "input.h"
#include "toolbar.h"
#include "trnames.h"
#include <stdarg.h>


// The sweep command.  Usage:
// sweep [-c] [param1] start1 stop1 step1
//   [param2] start2 stop2 step2 analysis
// -c clears analysis in progress in pause
//
// The params are in the form devname[paramname] (square brackets are
// literal).  If the first parameter is not given, then the sacond
// parameter can not be given, and the sweep uses the value1/value2
// variables to pass trial values, as in the old loop command, and
// operating range analysis.  If the first parameter is given, then if
// there is a second block the secvond parameter must be viven.
//
void
CommandTab::com_sweep(wordlist *wl)
{
    Sp.SweepAnalysis(wl);
}


#define STEPFCT 1e-7

// The sweep analysis function.
//
void
IFsimulator::SweepAnalysis(wordlist *wl)
{
    if (!ft_curckt) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no current circuit.\n");
        return;
    }
    if (wl && *wl->wl_word == '-' && *(wl->wl_word+1) == 'c') {
        delete ft_curckt->sweep();
        ft_curckt->set_sweep(0);
        wl = wl->wl_next;
        if (!wl)
            return;
    }
    sSWEEPprms *sweep = ft_curckt->sweep();
    bool firsttime = false;
    bool didpause = false;
    wl_gc wgc;
    if (sweep) {
        // paused
        TTY.printf("Resuming sweep run in progress.\n");
        didpause = true;
    }
    else {
        sweep = new sSWEEPprms;
        int error = sweep->sweepParse(&wl);
        if (error) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "syntax.\n");
            delete sweep;
            return;
        }
        // analysis specification
        if (!wl || !wl->wl_word || *wl->wl_word == '\0') {
            wl = ft_curckt->getAnalysisFromDeck();
            if (wl == 0) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "no analysis specified.\n");
                delete sweep;
                return;
            }
            if (wl->wl_next) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "more than one analysis specified.\n");
                wordlist::destroy(wl);
                delete sweep;
                return;
            }

            // wl has the entire command in one word, need to tokenize.

            char *ts = wl->wl_word;
            wl->wl_word = 0;
            delete wl;
            wl = CP.Lexer(ts);
            delete [] ts;
            wgc.set(wl);    // free on function exit
        }
        char *c;
        for (c = wl->wl_word; *c; c++)
            if (isupper(*c)) *c = tolower(*c);

        if (!sweep->devs[0])
            sweep->names = sNames::set_names();
        sweep->out_mode = OutcLoop;
        sweep->out_plot = ft_plot_cur;  // reset later
        for (int i = 0; i <= sweep->nestLevel; i++) {
            sweep->state[i] = sweep->start[i];
            if (sweep->step[i] == 0.0 && sweep->stop[i] == sweep->start[i])
                sweep->step[i] = 1.0;
        }
        firsttime = true;
        ft_curckt->set_sweep(sweep);
        sweep->out_cir = ft_curckt;
    }

    int i = sweep->nestSave;
    ft_flags[FT_SIMFLAG] = true;
    for (;;) {

        if (!didpause) {
            double tt = sweep->state[i] - sweep->stop[i];

            if ((sweep->step[i] > 0 && tt > STEPFCT*sweep->step[i]) ||
                (sweep->step[i] < 0 && tt < STEPFCT*sweep->step[i]) ||
                sweep->step[i] == 0) {

                i++; 
                if (i > sweep->nestLevel)
                    break;
                if (!sweep->dims[0])
                    sweep->dims[0]++;
                sweep->dims[0]++;
                if (sweep->dims[2] <= 1) {
                    OP.setDims(sweep->out_rundesc, sweep->dims, 2);
                    ToolBar()->UpdateVectors(0);
                }
                sweep->state[i] += sweep->step[i];
                continue;
            }

            if (!sweep->dims[0])
                sweep->dims[1]++;
        
            while (i > 0) { 
                i--; 
                sweep->state[i] = sweep->start[i];
            }

            int error = sweep->setInput();
            if (error)
                return;
            // If not using variables to pass trial values, we can use
            // a much simpler reset.
            sweep->out_cir->resetTrial(sweep->names != 0);
        }
        else
            didpause = false;

        int error, count;
        if (firsttime) {
            count = 0;
            // Beware plot handling:  out_plot is the old ft_plot_cur,
            // which may be needed in setup_newplot(), but this should
            // be zeroed when running the circuit the first time since
            // we want to switch to a new plot.

            sPlot *tplot = sweep->out_plot;
            sweep->out_plot = 0;
            sweep->out_cir->set_keep_deferred(true);
            error = sweep->out_cir->run(wl->wl_word, wl->wl_next);
            sweep->out_cir->set_keep_deferred(false);
            sweep->out_plot = tplot;
            // copy any specification vectors to new current plot
            if (sweep->names)
                sweep->names->setup_newplot(sweep->out_cir, sweep->out_plot);
            sweep->out_plot = ft_plot_cur;

            sDimen *dm = new sDimen(
                sweep->names ? sweep->names->value1() : "value1",
                sweep->start[1] != sweep->stop[1] ?
                (sweep->names ? sweep->names->value2() : "value2") : 0);
            dm->set_start1(sweep->start[0]);
            dm->set_stop1(sweep->stop[0]);
            dm->set_step1(sweep->step[0]);
            if (sweep->start[1] != sweep->stop[1]) {
                dm->set_start2(sweep->start[1]);
                dm->set_stop2(sweep->stop[1]);
                dm->set_step2(sweep->step[1]);
            }
            sweep->out_plot->set_dimensions(dm);

            firsttime = false;
        }
        else {
            count = sweep->out_plot->scale()->length();
            sweep->out_cir->set_keep_deferred(true);
            error = sweep->out_cir->runTrial();
            sweep->out_cir->set_keep_deferred(false);
        }
        if (error) {
            sweep->out_plot->set_active(false);
            sweep->out_cir->set_runplot(0);
            OP.endIplot(sweep->out_rundesc);
            ft_flags[FT_SIMFLAG] = false;
            if (error < 0)
                sweep->nestSave = i;
            else {
                sweep->out_cir->set_sweep(0);
                delete sweep;
            }
            return;
        }

        // store block size in dims[]
        sweep->dims[2] = sweep->out_plot->scale()->length() - count;

        if (sweep->dims[0])
            OP.setDims(sweep->out_rundesc, sweep->dims, 3, true);
        else
            OP.setDims(sweep->out_rundesc, sweep->dims+1, 2, true);
        sweep->state[i] += sweep->step[i];
        ToolBar()->UpdateVectors(0);
        sweep->loop_count++;
    }

    OP.endIplot(sweep->out_rundesc);
    ft_flags[FT_SIMFLAG] = false;
    if (sweep->out_plot)
        sweep->out_plot->set_active(false);
    if (sweep->out_cir) {
        sweep->out_cir->set_sweep(0);
        sweep->out_cir->set_runplot(0);
    }
    delete sweep;
}
// End of IFsimulator functions.


sSWEEPprms::sSWEEPprms()
{
    start[0] = 0.0;
    start[1] = 0.0;
    stop[0] = 0.0;
    stop[1] = 0.0;
    step[0] = 0.0;
    step[1] = 0.0;
    state[0] = 0.0;
    state[1] = 0.0;
    dims[0] = 0;
    dims[1] = 0;
    dims[2] = 0;
    nestLevel = 0;
    nestSave = 0;
    names = 0;
    devs[0] = 0;
    devs[1] = 0;
    prms[0] = 0;
    prms[1] = 0;
    tot_points = -1;
    loop_count = 0;
}


sSWEEPprms::~sSWEEPprms()
{
    delete names;
    wordlist::destroy(devs[0]);
    wordlist::destroy(devs[1]);
    wordlist::destroy(prms[0]);
    wordlist::destroy(prms[1]);
}


// Parser for the sweep specification.
//
//   [prm1] start1 stop1 step1 [prm2] start2 stop2 step2
//
// The prms have the form devicename[paramname].  If prm1 is not given,
// prm2 must not be given, and vice-versa.  If the prms are not given,
// the value1/value2 variables, etc., will pass the trial values, as in
// the old loop command, and operating range analysis.  If given, the
// parameters are varied directly, and there are no variables or vectors
// used for trial values.
//
// The function advances the wordlist, and an analysis command should
// follow.  Most of the values are optional, and a sensible
// interpretation is made if missing.  If there is an error, E_SYNTAX
// is returned.
//
int
sSWEEPprms::sweepParse(wordlist **wl)
{
    if (wl == 0 || *wl == 0)
        return (E_SYNTAX);
    const char *line = (*wl)->wl_word;
    int error;
    double tmp = IP.getFloat(&line, &error, true);  // name1 or start1
    if (error == OK)
        start[0] = tmp;
    else {
        char *dstr = lstring::getqtok(&line);
        sFtCirc::parseDevParams(dstr, &devs[0], &prms[0], false);
        delete [] dstr;
        if (!devs[0] || !prms[0])
            return (E_SYNTAX);

        *wl = (*wl)->wl_next;
        if (!*wl)
            return (E_SYNTAX);
        line = (*wl)->wl_word;

        tmp = IP.getFloat(&line, &error, true);  // start1
        if (error == OK)
            start[0] = tmp;
        else
            return (E_SYNTAX);
    }
    nestLevel = 0;
    stop[0] = start[0];
    step[0] = 0;

    *wl = (*wl)->wl_next;
    if (!*wl)
        return (OK);

    line = (*wl)->wl_word;
    tmp = IP.getFloat(&line, &error, true);  // stop1
    if (error == OK) {
        stop[0] = tmp;
        step[0] = stop[0] - start[0];
    }
    else
        return (OK);

    (*wl) = (*wl)->wl_next;
    if (!*wl)
        return (OK);

    line = (*wl)->wl_word;
    tmp = IP.getFloat(&line, &error, true);  // step1
    if (error == OK)
        step[0] = tmp;
    else
        return (OK);

    (*wl) = (*wl)->wl_next;
    if (!*wl)
        return (OK);
    line = (*wl)->wl_word;

    tmp = IP.getFloat(&line, &error, true);  // name2 or start2
    if (error == OK) {
        if (devs[0])
            return (E_SYNTAX);
        start[1] = tmp;
    }
    else {
        // It is non-numeric, it is either a parameter spec or the
        // start of the analysis command.

        if (sFtCirc::analysisType(line) >= 0)
            return (OK);

        if (!devs[0])
            return (E_SYNTAX);

        char *dstr = lstring::getqtok(&line);
        sFtCirc::parseDevParams(dstr, &devs[1], &prms[1], false);
        delete [] dstr;
        if (!devs[1] || !prms[1])
            return (E_SYNTAX);

        // We've got the second parameter, we need at least the start
        // value.
        (*wl) = (*wl)->wl_next;
        if (!*wl)
            return (E_SYNTAX);
        line = (*wl)->wl_word;

        tmp = IP.getFloat(&line, &error, true);  // start2
        if (error == OK)
            start[1] = tmp;
        else
            return (E_SYNTAX);
    }
    stop[1] = start[1];
    step[1] = 0;
    nestLevel = 1;

    (*wl) = (*wl)->wl_next;
    if (!*wl)
        return (OK);
    line = (*wl)->wl_word;
    tmp = IP.getFloat(&line, &error, true);  // stop2
    if (error == OK) {
        stop[1] = tmp;
        step[1] = stop[1] - start[1];
    }
    else
        return (OK);

    (*wl) = (*wl)->wl_next;
    if (!*wl)
        return (OK);

    line = (*wl)->wl_word;
    tmp = IP.getFloat(&line, &error, true);  // step2
    if (error == OK)
        step[1] = tmp;
    else
        return (OK);
    (*wl) = (*wl)->wl_next;
    return (OK);
}


int
sSWEEPprms::setInput()
{
    if (names) {
        names->set_input(out_cir, out_plot, state[0], state[1]);
        return (OK);
    }

    char nbuf[32];
    if (devs[0]) {
        sprintf(nbuf, "%.15e", state[0]);
        for (wordlist *wd = devs[0]; wd; wd = wd->wl_next) {
            for (wordlist *wp = prms[0]; wp; wp = wp->wl_next)
                out_cir->addDeferred(wd->wl_word, wp->wl_word, nbuf);
        }
        if (devs[1]) {
            sprintf(nbuf, "%.15e", state[1]);
            for (wordlist *wd = devs[1]; wd; wd = wd->wl_next) {
                for (wordlist *wp = prms[1]; wp; wp = wp->wl_next)
                    out_cir->addDeferred(wd->wl_word, wp->wl_word, nbuf);
            }
        }
    }
    return (OK);
}


int
sSWEEPprms::points()
{
    if (tot_points < 0) {
        double v = start[0];
        int n = 0;
        for (;;) {
            double tt = v - stop[0];
            if ((step[0] > 0 && tt > STEPFCT*step[0]) ||
                    (step[0] < 0 && tt < STEPFCT*step[0]) ||
                    step[0] == 0)
                break;
            v += step[0];
            n++;
        }
        if (!n)
            n++;
        if (nestLevel > 0) {
            v = start[1];
            int m = 0;
            for (;;) {
                double tt = v - stop[1];
                if ((step[1] > 0 && tt > STEPFCT*step[1]) ||
                        (step[1] < 0 && tt < STEPFCT*step[1]) ||
                        step[1] == 0)
                    break;
                v += step[1];
                m++;
            }
            if (!m)
                m++;
            n *= m;
        }
        tot_points = n;
    }
    return (tot_points);
}

