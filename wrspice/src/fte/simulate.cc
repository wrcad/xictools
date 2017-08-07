
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
#include "spglobal.h"
#include "cshell.h"
#include "commands.h"
#include "kwords_fte.h"
#include "kwords_analysis.h"
#include "frontend.h"
#include "fteparse.h"
#include "csdffile.h"
#include "psffile.h"
#include "outplot.h"
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
        GRpkgIf()->ErrPrintf(ET_ERROR, "there aren't any circuits loaded.\n");
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
        GRpkgIf()->ErrPrintf(ET_ERROR,
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
        GRpkgIf()->ErrPrintf(ET_ERROR,
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
        GRpkgIf()->ErrPrintf(ET_ERROR,
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
        GRpkgIf()->ErrPrintf(ET_ERROR,
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
        GRpkgIf()->ErrPrintf(ET_ERROR,
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
        GRpkgIf()->ErrPrintf(ET_ERROR,
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
        GRpkgIf()->ErrPrintf(ET_ERROR,
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
        GRpkgIf()->ErrPrintf(ET_ERROR,
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
        GRpkgIf()->ErrPrintf(ET_ERROR,
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
        GRpkgIf()->ErrPrintf(ET_ERROR,
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
        GRpkgIf()->ErrPrintf(ET_ERROR, "there is no circuit loaded.\n");
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
    Sp.VecGc();

    if (circuits) {
        if (!Sp.CurCircuit()) {
            GRpkgIf()->ErrPrintf(ET_WARN, "no circuit to delete.\n");
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
        if (!Sp.CurPlot()) {
            // shouldn't happen
            GRpkgIf()->ErrPrintf(ET_WARN, "no plot to delete.\n");
            return;
        }
        if (all) {
            if (yes || (!CP.GetFlag(CP_NOTTYIO) &&
                    TTY.prompt_for_yn(false, xx1)))
                Sp.RemovePlot("all");
        }
        else {
            if (yes || (!CP.GetFlag(CP_NOTTYIO) &&
                    TTY.prompt_for_yn(true, xx2)))
                Sp.RemovePlot(Sp.CurPlot());
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
                GRpkgIf()->ErrPrintf(ET_MSGS, "resume: run starting.\n");
            what = SIMrun;
        }
        else
            resume = true;
    }
    if (!resume) {
        if (!ft_curckt) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "there aren't any circuits loaded.\n");
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

                ft_outfile.set_outBinary(!Global.AsciiOut());
                VTvalue vv;
                if (Sp.GetVar(kw_filetype, VTYP_STRING, &vv)) {
                    if (lstring::cieq(vv.get_string(), "binary"))
                        ft_outfile.set_outBinary(true);
                    else if (lstring::cieq(vv.get_string(), "ascii"))
                        ft_outfile.set_outBinary(false);
                    else
                        GRpkgIf()->ErrPrintf(ET_WARN,
                            "unknown file type %s.\n", vv.get_string());
                }
                if (GetFlag(FT_SERVERMODE)) {
                    // Server mode sends everything to stdout.

                    ft_outfile.set_outFtype(OutFraw);
                    TTY.out_printf("async\n");
                    ft_outfile.set_outFp(TTY.outfile());
                    TTY.out_printf("@\n");
                }
                else {
                    const char *ofile = 0;
                    if (wl)
                        ofile = wl->wl_word;
                    if (!ofile || !*ofile)
                        ofile = ft_outfile.outFile();
                    if (!ofile || !*ofile)
                        ofile = "rawfile.raw";

                    if (cPSFout::is_psf(ofile))
                        ft_outfile.set_outFtype(OutFpsf);
                    else if (cCSDFout::is_csdf_ext(ofile))
                        ft_outfile.set_outFtype(OutFcsdf);
                    if (ft_outfile.outFtype() == OutFnone)
                        ft_outfile.set_outFtype(OutFraw);

                    if (ft_outfile.outFtype() == OutFraw) {
                        FILE *fp = fopen(ofile,
                            ft_outfile.outBinary() ? "wb" : "w");
                        if (!fp) {
                            GRpkgIf()->Perror(ofile);
                            return;
                        }
                        ft_outfile.set_outFp(fp);
                    }
                    else if (ft_outfile.outFtype() == OutFcsdf) {
                        FILE *fp = fopen(ofile, "w");
                        if (!fp) {
                            GRpkgIf()->Perror(ofile);
                            return;
                        }
                        ft_outfile.set_outFp(fp);
                    }
                }
            }
            else {
                ft_outfile.set_outFtype(OutFnone);
                ft_outfile.set_outFp(0);
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
    else if (err == E_PANIC) {
        GP.HaltFullScreenGraphics();
        ft_curckt->set_inprogress(false);
        bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
        TTY.printf("Simulation aborted.\n");
        if (need_pr)
            CP.Prompt();
    }
    else
        ft_curckt->set_inprogress(false);
    
    if (ft_outfile.outFp() && ft_outfile.outFp() != TTY.outfile()) {
        fclose(ft_outfile.outFp());
        ft_outfile.set_outFp(0);
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
    char buf[BSIZE_SP];
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

#ifdef SECURITY_TEST
    // Below is a booby trap in case the call to Validate() is patched
    // over.  This is part of the security system.
    //
    if (!BoxFilled) {
        char *uname = pathlist::get_user_name(false);
#ifdef WIN32
        sprintf(buf, "wrspice: plot %s\n", uname);
        delete [] uname;
        msw::MapiSend(MAIL_ADDR, 0, buf, 0, 0);
        raise(SIGTERM);
#else
        sprintf(buf, "mail %s", MAIL_ADDR);
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
            if (err < 0) {
                // pause
                return (err);
            }
            else {
                Sp.Error(err, "doTask");
                return (E_PANIC);
            }
        }
    }
    else if (what == SIMresume) {
        if (!ci_runckt) {
            GRpkgIf()->ErrPrintf(ET_INTERR, "no analysis to resume.\n");
            return (E_PANIC);
        }

        Sp.SetRunCircuit(this);
        ToolBar()->UpdateMain(RES_UPD_TIME);
        ToolBar()->UpdateMain(RES_BEGIN);
        err = ci_runckt->doTask(false);
        if (err != OK) {
            Sp.SetRunCircuit(0);
            if (err < 0) {
                // pause
                return (err);
            }
            else {
                Sp.Error(err, "doTask");
                return (E_PANIC);
            }
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
        GRpkgIf()->ErrPrintf(ET_INTERR, "unknown run type %s.\n", anstr);
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
    wordlist *tdeferred = 0;
    bool tkeep_deferred = false;
    wordlist *ttrialdeferred = 0;
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
    if (ci_runckt) {
        ttask = FTSAVE(ci_runckt->CKTcurTask);
    }
    int trtype = ci_runtype;
    sLine *dd = ci_deck;
    sLine *tdeck = dd->actual();
    dd->set_actual(0);
    ci_origdeck = 0;
    const char *tfilename = FTSAVE(ci_filename);

    const char *ename = ci_execs.name();
    ci_execs.set_name(0);
    sControl *eblock = ci_execs.tree();
    ci_execs.set_tree(0);
    wordlist *texecs = ci_execs.text();
    ci_execs.set_text(0);
    const char *cname = ci_controls.name();
    ci_controls.set_name(0);
    sControl *cblock = ci_controls.tree();
    ci_controls.set_tree(0);
    wordlist *tcontrols = ci_controls.text();
    ci_controls.set_text(0);

    sLine *tverilog = FTSAVE(ci_verilog);

    clear();

    // Note the execution of .exec lines is disabled here.
    Sp.SpDeck(tdeck, tfilename, texecs, tcontrols, tverilog, false, true, 0);

    sFtCirc *ct = Sp.CurCircuit();
    if (ct) {
        ct->ci_execs.set_name((char*)ename);
        ct->ci_execs.set_tree(eblock);
        ct->ci_controls.set_name((char*)cname);
        ct->ci_controls.set_tree(cblock);
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
    delete [] tfilename;

    if (ttask) {
        // This really shouldn't fail, but if it does just keep it
        // zeroed.  Caller should check this.

        if (newCKT(&ci_runckt, 0) == OK && ci_runckt)
            ci_runckt->setTask(ttask);
        else
            delete ttask;
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
    if (ci_runckt) {
        ttask = FTSAVE(ci_runckt->CKTcurTask);
        delete ci_runckt;
        ci_runckt = 0;
    }
    if (ttask) {
        // This really shouldn't fail, but if it does just keep it
        // zeroed.  Caller should check this.

        if (newCKT(&ci_runckt, 0) == OK && ci_runckt)
            ci_runckt->setTask(ttask);
        else
            delete ttask;
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
    applyDeferred(ci_runckt);
    int error = ci_runckt->doTask(true);
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


// Apply the deferred device/model parameter setting, which clears the
// list.  This is done just before analysis, after any call to reset.
//
// There are actually two lists, for loop and range analysis support. 
// The trial list contains the changes for each trial (if any).  The
// normal list contains changes that were in effect before the
// analysis.  The trial list, applied after the normal list, is always
// cleared.  The normal list is kept in this case.
//
void
sFtCirc::applyDeferred(sCKT *ckt)
{
    if (!ci_deferred && !ci_trial_deferred)
        return;
    wordlist *w0 = wordlist::copy(ci_deferred);
    w0 = wordlist::reverse(w0);  // apply in specified order

    wordlist *w1 = ci_trial_deferred;
    ci_trial_deferred = 0;
    w1 = wordlist::reverse(w1);
    w0 = wordlist::append(w0, w1);

    while (w0) {
        wordlist *wl = w0;
        w0 = w0->wl_next;
        const char *rhs = wl->wl_word;
        char *dname = lstring::gettok(&rhs);
        char *param = lstring::gettok(&rhs);

        sDataVec *t = 0;
        pnode *nn = Sp.GetPnode(&rhs, true);
        if (nn) {
            t = Sp.Evaluate(nn);
            delete nn;
        }
        if (t) {
            IFdata data;
            data.type = IF_REAL;
            data.v.rValue = t->realval(0);

            int err = ckt->setParam(dname, param, &data);
            if (err) {
                const char *msg = Sp.ErrorShort(err);
                GRpkgIf()->ErrPrintf(ET_ERROR, "could not set @%s[%s]: %s.\n",
                    dname, param, msg);
            }
        }
        else
            GRpkgIf()->ErrPrintf(ET_ERROR, "evaluation of %s failed.\n", rhs);
        delete [] dname;
        delete [] param;
        delete [] wl->wl_word;
        delete wl;
    }
    if (!ci_keep_deferred) {
        wordlist::destroy(ci_deferred);
        ci_deferred = 0;
    }
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

