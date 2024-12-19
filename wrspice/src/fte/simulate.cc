
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
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include <signal.h>
#include "config.h"
#include "spglobal.h"
#include "cshell.h"
#include "commands.h"
#include "kwords_fte.h"
#include "kwords_analysis.h"
#include "simulator.h"
#include "parser.h"
#include "csvfile.h"
#include "csdffile.h"
#include "psffile.h"
#include "graph.h"
#include "output.h"
#include "misc.h"
#include "ttyio.h"
#include "circuit.h"
#include "errors.h"
#include "input.h"
#include "toolbar.h"
#include "miscutil/pathlist.h"
#ifdef WIN32
#include "miscutil/msw.h"
#endif

extern int BoxFilled;  // security


//
// Circuit simulation commands, and some support functions.
//

void
CommandTab::com_setcirc(wordlist *wl)
{
    const char *zz = CP.GetFlag(CP_NOTTYIO) ?
        "Circuits in memory, run command again with desired name:" :
        "Type the name of the desired circuit: ";
    char buf[BSIZE_SP];
    if (Sp.CircuitList() == 0) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "there aren't any circuits loaded.\n");
        return;
    }
    if (wl == 0) {
        for (sFtCirc *p = Sp.CircuitList(); p; p = p->next()) {
            if (Sp.CurCircuit() == p)
                TTY.printf("Current %-6s %s\n", p->name(), p->descr());
            else
                TTY.printf("        %-6s %s\n", p->name(), p->descr());
        }
        TTY.printf("\n");
        if (CP.GetFlag(CP_NOTTYIO))
            return;
        TTY.prompt_for_input(buf, BSIZE_SP, zz);

        char *s;
        for (s = buf; *s && !isspace(*s); s++) ;
        *s = '\0';
    }
    else
        strcpy(buf, wl->wl_word);

    if (!*buf)
        return;
    Sp.SetCircuit(buf);
    if (wl) {
        if (Sp.CurCircuit()) {
            TTY.printf("%s: %s\n", Sp.CurCircuit()->name(),
                Sp.CurCircuit()->descr());
        }
        else
            TTY.printf("no current circuit!\n");
    }
}


void
CommandTab::com_pz(wordlist *wl)
{
    if (Sp.CurCircuit() && Sp.CurCircuit()->check()) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "operating range or Monte Carlo run is paused but active,\n"
            "can't do analysis, use \"check -c\" to clear.\n");
        return;
    }
    Sp.Simulate(SIMpz, wl);
}


void
CommandTab::com_op(wordlist *wl)
{
    if (Sp.CurCircuit() && Sp.CurCircuit()->check()) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "operating range or Monte Carlo run is paused but active,\n"
            "can't do analysis, use \"check -c\" to clear.\n");
        return;
    }
    Sp.Simulate(SIMop, wl);
}


void
CommandTab::com_dc(wordlist *wl)
{
    if (Sp.CurCircuit() && Sp.CurCircuit()->check()) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "operating range or Monte Carlo run is paused but active,\n"
            "can't do analysis, use \"check -c\" to clear.\n");
        return;
    }
    Sp.Simulate(SIMdc, wl);
}


void
CommandTab::com_ac(wordlist *wl)
{
    if (Sp.CurCircuit() && Sp.CurCircuit()->check()) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "operating range or Monte Carlo run is paused but active,\n"
            "can't do analysis, use \"check -c\" to clear.\n");
        return;
    }
    Sp.Simulate(SIMac, wl);
}


void
CommandTab::com_tf(wordlist *wl)
{
    if (Sp.CurCircuit() && Sp.CurCircuit()->check()) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "operating range or Monte Carlo run is paused but active,\n"
            "can't do analysis, use \"check -c\" to clear.\n");
        return;
    }
    Sp.Simulate(SIMtf, wl);
}


void
CommandTab::com_tran(wordlist *wl)
{
    if (Sp.CurCircuit() && Sp.CurCircuit()->check()) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "operating range or Monte Carlo run is paused but active,\n"
            "can't do analysis, use \"check -c\" to clear.\n");
        return;
    }
    Sp.Simulate(SIMtran, wl);
}


void
CommandTab::com_sens(wordlist *wl)
{
    if (Sp.CurCircuit() && Sp.CurCircuit()->check()) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "operating range or Monte Carlo run is paused but active,\n"
            "can't do analysis, use \"check -c\" to clear.\n");
        return;
    }
    Sp.Simulate(SIMsens, wl);
}


void
CommandTab::com_disto(wordlist *wl)
{
    if (Sp.CurCircuit() && Sp.CurCircuit()->check()) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "operating range or Monte Carlo run is paused but active,\n"
            "can't do analysis, use \"check -c\" to clear.\n");
        return;
    }
    Sp.Simulate(SIMdisto, wl);
}


void
CommandTab::com_noise(wordlist *wl)
{
    if (Sp.CurCircuit() && Sp.CurCircuit()->check()) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "operating range or Monte Carlo run is paused but active,\n"
            "can't do analysis, use \"check -c\" to clear.\n");
        return;
    }
    Sp.Simulate(SIMnoise, wl);
}


// Continue a simulation. If there are none in progress, this is the
// equivalent of "run".
//
void
CommandTab::com_resume(wordlist *wl)
{
    Sp.Simulate(SIMresume, wl);
}


// Usage is run [filename]
//
void
CommandTab::com_run(wordlist *wl)
{
    if (Sp.CurCircuit() && Sp.CurCircuit()->check()) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "operating range or Monte Carlo run is paused but active,\n"
            "can't do analysis, use \"check -c\" to clear.\n");
        return;
    }
    Sp.Simulate(SIMrun, wl);
}


// Throw out the circuit struct and recreate it from the original deck.
// The shell variables are updated.  Usage:
// reset [-c]
// The -c option will clear any margin or loop analysis, which are
// otherwise retained.
//
void
CommandTab::com_reset(wordlist *wl)
{
    if (Sp.CurCircuit() == 0) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "there is no circuit loaded.\n");
        return;
    }
    bool save_loop = false;
    if (!wl || !lstring::prefix("-c", wl->wl_word))
        save_loop = true;
    Sp.CurCircuit()->rebuild(save_loop);
}


// free command - free the current plot, circuit
//
void
CommandTab::com_free(wordlist *wl)
{
    const char *xx1 = "Delete all plots [n]? ";
    const char *xx2 = "Delete current plot [y]? ";
    const char *xx3 = "Delete all circuits [n]? ";
    const char *xx4 = "Delete current circuit [y]? ";
    bool yes = false;
    bool plots = false;
    bool circuits = false;
    bool all = false;

    for (wordlist *ww = wl; ww; ww = ww->wl_next) {
        if (lstring::ciprefix("p", ww->wl_word)) {
            plots = true;
            continue;
        }
        if (lstring::ciprefix("c", ww->wl_word)) {
            circuits = true;
            continue;
        }
        if (lstring::ciprefix("y", ww->wl_word)) {
            yes = true;
            continue;
        }
        if (lstring::ciprefix("a", ww->wl_word)) {
            all = true;
            continue;
        }
    }
    if (!plots && !circuits) {
        plots = true;
        circuits = true;
    }
    OP.vecGc();

    if (circuits) {
        if (!Sp.CurCircuit()) {
            GRpkg::self()->ErrPrintf(ET_WARN, "no circuit to delete.\n");
            return;
        }
        else {
            if (all) {
                if (yes || (!CP.GetFlag(CP_NOTTYIO) &&
                        TTY.prompt_for_yn(false, xx3))) {
                    while (Sp.CurCircuit())
                        delete Sp.CurCircuit();
                }
            }
            else {
                if (yes || (!CP.GetFlag(CP_NOTTYIO) &&
                        TTY.prompt_for_yn(true, xx4)))
                    delete Sp.CurCircuit();
            }
        }
    }
    if (plots) {
        if (!OP.curPlot()) {
            // shouldn't happen
            GRpkg::self()->ErrPrintf(ET_WARN, "no plot to delete.\n");
            return;
        }
        if (all) {
            if (yes || (!CP.GetFlag(CP_NOTTYIO) &&
                    TTY.prompt_for_yn(false, xx1)))
                OP.removePlot("all");
        }
        else {
            if (yes || (!CP.GetFlag(CP_NOTTYIO) &&
                    TTY.prompt_for_yn(true, xx2)))
                OP.removePlot(OP.curPlot());
        }
    }
}


void
IFsimulator::Simulate(SIMtype what, wordlist *wl)
{
    bool resume = false;
    if (what == SIMresume) {
        if (!ft_curckt)
            return;
        if (!ft_curckt->inprogress()) {
            // The check and loop analyses never set the inprogress
            // flag, instead they hang a structure pointer on the
            // current circuit.  If the inprogress flag is true, an
            // ordinary run was in progress when the pause was issued. 
            // Otherwise, if the structure pointers are not 0, a check
            // or loop analysis was paused.
            //
            if (ft_curckt->check()) {
                MargAnalysis(0);
                return;
            }
            if (ft_curckt->sweep()) {
                SweepAnalysis(0);
                return;
            }
            if (ft_flags[FT_SIMDB])
                GRpkg::self()->ErrPrintf(ET_MSGS, "resume: run starting.\n");
            what = SIMrun;
        }
        else
            resume = true;
    }
    if (!resume) {
        if (!ft_curckt) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "there aren't any circuits loaded.\n");
            return;
        }
        if (!ft_curckt->deck()) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "current circuit is not initialized.\n");
            return;
        }

        if (ft_curckt->inprogress() && what == SIMrun) {
            TTY.printf("Resuming run in progress.\n");
            what = SIMresume;
        }
        else {
            ft_flags[FT_INTERRUPT] = false;
            if (GetFlag(FT_SERVERMODE) || ((what == SIMrun) && wl)) {
                // Use an output file.

                OP.getOutDesc()->set_outBinary(!Global.AsciiOut());
                VTvalue vv;
                if (Sp.GetVar(kw_filetype, VTYP_STRING, &vv)) {
                    if (lstring::cieq(vv.get_string(), "binary"))
                        OP.getOutDesc()->set_outBinary(true);
                    else if (lstring::cieq(vv.get_string(), "ascii"))
                        OP.getOutDesc()->set_outBinary(false);
                    else {
                        GRpkg::self()->ErrPrintf(ET_WARN,
                            "unknown file type %s.\n", vv.get_string());
                    }
                }
                if (GetFlag(FT_SERVERMODE)) {
                    // Server mode sends everything to stdout.

                    OP.getOutDesc()->set_outFtype(OutFraw);
                    TTY.out_printf("async\n");
                    OP.getOutDesc()->set_outFp(TTY.outfile());
                    TTY.out_printf("@\n");
                }
                else {
                    const char *ofile = 0;
                    if (wl)
                        ofile = wl->wl_word;
                    if (!ofile || !*ofile)
                        ofile = OP.getOutDesc()->outFile();
                    if (!ofile || !*ofile)
                        ofile = "rawfile.raw";

                    if (cPSFout::is_psf(ofile))
                        OP.getOutDesc()->set_outFtype(OutFpsf);
                    else if (cCSDFout::is_csdf_ext(ofile))
                        OP.getOutDesc()->set_outFtype(OutFcsdf);
                    else if (cCSVout::is_csv_ext(ofile))
                        OP.getOutDesc()->set_outFtype(OutFcsv);
                    if (OP.getOutDesc()->outFtype() == OutFnone)
                        OP.getOutDesc()->set_outFtype(OutFraw);

                    if (OP.getOutDesc()->outFtype() == OutFraw) {
                        FILE *fp = fopen(ofile,
                            OP.getOutDesc()->outBinary() ? "wb" : "w");
                        if (!fp) {
                            GRpkg::self()->Perror(ofile);
                            return;
                        }
                        OP.getOutDesc()->set_outFp(fp);
                    }
                    else if (OP.getOutDesc()->outFtype() == OutFcsdf) {
                        FILE *fp = fopen(ofile, "w");
                        if (!fp) {
                            GRpkg::self()->Perror(ofile);
                            return;
                        }
                        OP.getOutDesc()->set_outFp(fp);
                    }
                }
            }
            else {
                OP.getOutDesc()->set_outFtype(OutFnone);
                OP.getOutDesc()->set_outFp(0);
            }

            ft_curckt->set_inprogress(true);
        }
    }

    ft_flags[FT_SIMFLAG] = true;
    int err = ft_curckt->run(what, wl);
    if (err < 0) {
        // The circuit was interrupted somewhere
        GP.HaltFullScreenGraphics();
        if (err == E_INTRPT) {
            if (!CP.GetFlag(CP_NOTTYIO)) {
                bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
                TTY.printf("Simulation interrupted.\n");
                if (need_pr)
                    CP.Prompt();
            }
        }
    }
    else if (err != OK) {
        GP.HaltFullScreenGraphics();
        ft_curckt->set_inprogress(false);
        bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
        TTY.printf("Simulation aborted.\n");
        if (need_pr)
            CP.Prompt();
    }
    else
        ft_curckt->set_inprogress(false);
    
    if (OP.getOutDesc()->outFp() &&
            OP.getOutDesc()->outFp() != TTY.outfile()) {
        fclose(OP.getOutDesc()->outFp());
        OP.getOutDesc()->set_outFp(0);
    }
    ft_flags[FT_SIMFLAG] = false;
    ft_curckt->set_runonce(true);
    ToolBar()->UpdateMain(RES_UPD_TIME);
    ToolBar()->UpdateMain(RES_BEGIN);
}


void
IFsimulator::Run(const char *file)
{
    if (file) {
        wordlist wl;
        wl.wl_word = lstring::copy(file);
        Simulate(SIMrun, &wl);
        delete [] wl.wl_word;
    }
    else
        Simulate(SIMrun, 0);
}


int
IFsimulator::InProgress()
{
    return (ft_curckt && ft_curckt->inprogress());
}
// End of IFsimulator functions.


// Do a run of the circuit, of the given type. Type "resume" is special --
// it means to resume whatever simulation that was in progress. The
// return value of this routine is 0 if the exit was ok, and 1 if there was
// a reason to interrupt the circuit (interrupt typed at the keyboard, 
// error in the simulation, etc).
// args may be the entire command line, 
// e.g. "tran 1 10 20 uic", 
// or only the arguments that follow "what".
//
int
sFtCirc::run(SIMtype what, wordlist *args)
{
    sCKT *ckt;
    sTASK *task;
    int err;
    // First parse the line...
    if (isAnalysis(what)) {
        err = newCKT(&ckt, 0);
        if (err != OK)
            return (err);
        char *tmp = wordlist::flatten(args);
        err = ckt->newTask(analysisString(what), tmp, &task);
        if (err != OK) {
            delete ckt;
            return (err);
        }
    }
    else if (what == SIMrun) {
        err = newCKT(&ckt, &task);
        if (err != OK)
            return (err);
    }

#ifdef HAVE_SECURE
    // Below is a booby trap in case the call to Validate() is patched
    // over.  This is part of the security system.
    //
    if (!BoxFilled) {
        char buf[256];
        char *uname = pathlist::get_user_name(false);
#ifdef WIN32
        snprintf(buf, sizeof(buf), "wrspice: plot %s\n", uname);
        delete [] uname;
        msw::MapiSend(Global.BugAddr(), 0, buf, 0, 0);
        raise(SIGTERM);
#else
        snprintf(buf, sizeof(buf), "mail %s", Global.BugAddr());
        FILE *fp = popen(buf, "w");
        if (fp) {
            fprintf(fp, "wrspice: plot %s\n", uname);
            pclose(fp);
        }
        kill(0, SIGKILL);
#endif
    }
#endif

    if (isAnalysis(what) || what == SIMrun) {

        bool noshellopts = Sp.GetVar(spkw_noshellopts, VTYP_BOOL, 0, this);
        task->setOptions(ci_defOpt, !noshellopts);
        ckt->setTask(task);
        delete ci_runckt;
        ci_runckt = ckt;

        Sp.SetRunCircuit(this);
        ToolBar()->UpdateMain(RES_UPD_TIME);
        ToolBar()->UpdateMain(RES_BEGIN);
        applyDeferred(ckt);
        err = ckt->doTask(true);
        if (err != OK) {
            Sp.SetRunCircuit(0);
            // Don't report convergence error if DCOSILENT set.
            if (err > 0 && (!Sp.GetFlag(FT_DCOSILENT) || err != E_ITERLIM))
                Sp.Error(err, "doTask");
            return (err);
        }
    }
    else if (what == SIMresume) {
        if (!ci_runckt) {
            GRpkg::self()->ErrPrintf(ET_INTERR, "no analysis to resume.\n");
            return (E_PANIC);
        }

        Sp.SetRunCircuit(this);
        ToolBar()->UpdateMain(RES_UPD_TIME);
        ToolBar()->UpdateMain(RES_BEGIN);
        err = ci_runckt->doTask(false);
        if (err != OK) {
            Sp.SetRunCircuit(0);
            if (err > 0)
                Sp.Error(err, "doTask");
            return (err);
        }
    }

    Sp.SetRunCircuit(0);
    return (OK);
}


int
sFtCirc::run(const char *anstr, wordlist *args)
{
    int i = analysisType(anstr);
    if (i < 0) {
        GRpkg::self()->ErrPrintf(ET_INTERR, "unknown run type %s.\n", anstr);
        return (E_PANIC);
    }
    return (run((SIMtype)i, args));
}


#define FTSAVE(zz) zz; zz = 0

// Throw out the circuit struct and recreate it from the original
// deck.  Call this when the circuit text changes, such as through
// variable changes which would alter the text on expansion.
// If save_loop is true, retain any margin or sweep analysis.
//
void
sFtCirc::rebuild(bool save_loop)
{
    sSWEEPprms *tsweep = 0;
    sCHECKprms *tcheck = 0;
    sPlot *trunplot = 0;
    sOPTIONS *tdopt = 0;
    dfrdlist *tdeferred = 0;
    bool tkeep_deferred = false;
    dfrdlist *ttrialdeferred = 0;
    bool tusetrialdeferred = false;
    if (save_loop) {
        tsweep = FTSAVE(ci_sweep);
        tcheck = FTSAVE(ci_check);
        trunplot = FTSAVE(ci_runplot);
        tdeferred = FTSAVE(ci_deferred);
        tkeep_deferred = FTSAVE(ci_keep_deferred);
        if (!tsweep && !tcheck && trunplot)
            trunplot->set_active(false);
        if (tsweep || tcheck) {
            tdopt = FTSAVE(ci_defOpt);
            ttrialdeferred = FTSAVE(ci_trial_deferred);
            tusetrialdeferred = FTSAVE(ci_use_trial_deferred);
        }
    }
    sSymTab *tsymtab = FTSAVE(ci_symtab);
    sTASK *ttask = 0;
    sSTATS *tstat = 0;
    if (ci_runckt) {
        ttask = FTSAVE(ci_runckt->CKTcurTask);
        if (ttask) {
            tstat = FTSAVE(ci_runckt->CKTstat);
        }
    }
    int trtype = ci_runtype;
    sLine *dd = ci_deck;
    sLine *tdeck = dd ? dd->actual() : 0;
    if (dd)
        dd->set_actual(0);
    ci_origdeck = 0;
    const char *tfilename = FTSAVE(ci_filename);

    char *ename = lstring::copy(ci_execBlk.name());
    ci_execBlk.set_name(0);
    sControl *eblock = ci_execBlk.tree();
    ci_execBlk.set_tree(0);
    wordlist *texecs = ci_execBlk.text();
    ci_execBlk.set_text(0);
    char *cname = lstring::copy(ci_controlBlk.name());
    ci_controlBlk.set_name(0);
    sControl *cblock = ci_controlBlk.tree();
    ci_controlBlk.set_tree(0);
    wordlist *tcontrols = ci_controlBlk.text();
    ci_controlBlk.set_text(0);

    sLine *tverilog = FTSAVE(ci_verilog);

    clear();

    // Note the execution of .exec lines is disabled here.
    Sp.SpDeck(tdeck, tfilename, texecs, tcontrols, tverilog, false, true, 0);

    sFtCirc *ct = Sp.CurCircuit();
    if (ct) {
        ct->ci_execBlk.set_name(ename);
        ct->ci_execBlk.set_tree(eblock);
        ct->ci_controlBlk.set_name(cname);
        ct->ci_controlBlk.set_tree(cblock);
        ct->ci_sweep = tsweep;
        ct->ci_check = tcheck;
        delete ct->ci_symtab;
        ct->ci_symtab = tsymtab;
        ct->ci_runplot = trunplot;
        ct->ci_runtype = trtype;
        ct->ci_deferred = tdeferred;
        ct->ci_keep_deferred = tkeep_deferred;
        if (tsweep || tcheck) {
            delete ct->ci_defOpt;
            ct->ci_defOpt = tdopt;
            ct->ci_trial_deferred = ttrialdeferred;
            ct->ci_use_trial_deferred = tusetrialdeferred;
        }
    }
    delete [] ename;
    delete [] cname;
    delete [] tfilename;

    if (ttask) {
        // This really shouldn't fail, but if it does just keep it
        // zeroed.  Caller should check this.

        if (newCKT(&ci_runckt, 0) == OK && ci_runckt) {
            ci_runckt->setTask(ttask);
            if (tstat) {
                delete ci_runckt->CKTstat;
                ci_runckt->CKTstat = tstat;
            }
        }
        else {
            delete ttask;
            delete tstat;
        }
    }
}


// Rebuild the sCKT, taking the existing text so won't incorporate
// text changes (use rebuild instead if needed).  This is used after
// direct parameter changes only.
//
void
sFtCirc::reset()
{
    sTASK *ttask = 0;
    sSTATS *tstat = 0;
    if (ci_runckt) {
        ttask = FTSAVE(ci_runckt->CKTcurTask);
        if (ttask) {
            tstat = FTSAVE(ci_runckt->CKTstat);
        }
        delete ci_runckt;
        ci_runckt = 0;
    }
    if (ttask) {
        // This really shouldn't fail, but if it does just keep it
        // zeroed.  Caller should check this.

        if (newCKT(&ci_runckt, 0) == OK && ci_runckt) {
            ci_runckt->setTask(ttask);
            if (tstat) {
                delete ci_runckt->CKTstat;
                ci_runckt->CKTstat = tstat;
            }
        }
        else {
            delete ttask;
            delete tstat;
        }
    }
}


// Run a margin/sweep trial, reset the options since some option (e.g.,
// temp) may change between trials.
//
int
sFtCirc::runTrial()
{
    Sp.SetRunCircuit(this);
    ci_runonce = true;

    // Suppress DCOP nonconvergence messages in trial, these will be
    // taken as a fail point silently.
    bool flg = Sp.GetFlag(FT_DCOSILENT);
    Sp.SetFlag(FT_DCOSILENT, true);
    applyDeferred(ci_runckt);
    int error = ci_runckt->doTask(true);
    Sp.SetFlag(FT_DCOSILENT, flg);
    Sp.SetRunCircuit(0);
    return (error);
}


// Reset the circuit, get ready for next trial.  Used by margin and
// sweep analysis.
//
void
sFtCirc::resetTrial(bool textchange)
{
    if (textchange) {
        sFtCirc *curct = Sp.CurCircuit();
        Sp.SetCurCircuit(this);
        ToolBar()->SuppressUpdate(true);
        Sp.CurCircuit()->rebuild(true);
        ToolBar()->SuppressUpdate(false);
        Sp.SetCurCircuit(curct);
    }
    else
        reset();
}


// Static function
//
bool
sFtCirc::isAnalysis(SIMtype what)
{
    if (what == SIMresume || what == SIMrun)
        return (false);
    return (true);
}


// Static function
//
const char *
sFtCirc::analysisString(SIMtype what)
{
    switch (what) {
    case SIMresume:
    case SIMrun:
        return (0);
    case SIMac:
        return ("ac");
    case SIMdc:
        return ("dc");
    case SIMdisto:
        return ("disto");
    case SIMnoise:
        return ("noise");
    case SIMop:
        return ("op");
    case SIMpz:
        return ("pz");
    case SIMsens:
        return ("sens");
    case SIMtf:
        return ("tf");
    case SIMtran:
        return ("tran");
    }
    return (0);  // not reached
}


// Static function
//
int
sFtCirc::analysisType(const char *str)
{
    if (!str)
        return (-1);
    if (lstring::cieq(str, "ac"))
        return (SIMac);
    if (lstring::cieq(str, "dc"))
        return (SIMdc);
    if (lstring::cieq(str, "disto"))
        return (SIMdisto);
    if (lstring::cieq(str, "noise"))
        return (SIMnoise);
    if (lstring::cieq(str, "op"))
        return (SIMop);
    if (lstring::cieq(str, "pz"))
        return (SIMpz);
    if (lstring::cieq(str, "sens"))
        return (SIMsens);
    if (lstring::cieq(str, "tf"))
        return (SIMtf);
    if (lstring::cieq(str, "tran"))
        return (SIMtran);
    return (-1);
}

