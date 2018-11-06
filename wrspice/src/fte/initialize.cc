
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
#include "frontend.h"
#include "spglobal.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "circuit.h"
#include "fteparse.h"
#include "graph.h"
#include "output.h"
#include "keywords.h"
#include "optdefs.h"
#include "statdefs.h"
#include "miscutil/random.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#include "help/help_topic.h"
#include "text_help.h"
#endif
#ifdef HAVE_FENV_H
#include <fenv.h>
#if defined(__APPLE__) && defined(__x86_64)
// This is a gcc thing...
#include <xmmintrin.h>
#endif
#endif
#ifdef WIN32
#include <float.h>
#endif


//
// Application initialization and interface code.
//

#ifdef HAVE_MOZY
namespace {
    // Text mode help instance.
    text_help thelp;
}
#endif

const char *kw_all        = "all";
const char *kw_everything = "everything";

// The real-valued constants for the constants plot, also used in the
// IFparseTree.  The units string consists of the units as would be
// typed in input, with '%c' replacing the UNITS_CATCHAR.
//
sConstant IFsimulator::ft_constants[] = {
    sConstant( "yes",       1.0,                0 ) ,
    sConstant( "true",      1.0,                0 ) ,
    sConstant( "no",        0.0,                0 ) ,
    sConstant( "false",     0.0,                0 ) ,
    sConstant( "pi",        M_PI,               0 ) ,
    sConstant( "const_e",   M_E,                0 ) ,
    sConstant( "const_c",   wrsCONSTc,          "%cM%cS" ) ,
    sConstant( "kelvin",    -wrsCONSTCtoK,      "C" ) ,
    sConstant( "echarge",   wrsCHARGE,          "Cl" ) ,
    sConstant( "boltz",     wrsCONSTboltz,      "VCl%cC" ) ,
    sConstant( "planck",    wrsCONSTplanck,     "ClWb" ) ,
    sConstant( "phi0",      wrsCONSTphi0,       "Wb" ) ,
    sConstant( 0,           0.0,                0 )
};

namespace { bool post_init_done = false; }


// Initialize, called before any startup scripts are sourced.
//
void
IFsimulator::PreInit()
{
    // Initialize random number generator.
#ifdef HAVE_GETPID
    Rnd.seed(getpid());
#else
    Rnd.seed(17);
#endif

    // Initialize resource use tracking.
    InitResource();

#ifdef HAVE_MOZY
    // Set up help system.
    HLP()->set_path(Global.HelpPath(), false);
#ifdef WIN32
    HLP()->define("Windows");
#endif
    HLP()->define("WRspice");
    HLP()->set_no_file_fonts(true);     // Don't use fonts from .mozyrc.
    HLP()->register_text_help(&thelp);  // Register text-mode help handler.
    if (!CP.Display())
        HLP()->set_using_graphics(false);
#ifdef HAVE_SECURE
    HLP()->define("xtlserv");
#endif
#endif

    SetVar(kw_history, CP.MaxHistLength());
    SetVar(kw_noglob);
    SetVar(kw_program, CP.Program());

    // Make the prompt use only the last component of the path...
    const char *s = lstring::strrdirsep(CP.Program());
    if (s)
        s++;
    else
        s = CP.Program();

    char *bf = new char[strlen(s) + 10];
    sprintf(bf, "%s ! -> ", s);
    // Capitalize one or two prefix characters ahead of "spice"
    if (islower(bf[0]) && bf[0] != 's')
        bf[0] = toupper(bf[0]);
    if (islower(bf[1]) && bf[1] != 's')
        bf[1] = toupper(bf[1]);
    SetVar(kw_prompt, bf);
    delete [] bf;

    // Set these to a default (used in decks for loop and check
    // commands).
    //
    SetVar("value1", 0.0);
    SetVar("value2", 0.0);

    // Set a few useful aliases.
    //
    CP.SetAlias("begin", "if 1");
    CP.SetAlias("endif", "end");
    CP.SetAlias("endwhile", "end");
    CP.SetAlias("endforeach", "end");
    CP.SetAlias("endrepeat", "end");
    CP.SetAlias("enddowhile", "end");
    CP.SetAlias("?", "help");
    CP.SetAlias("exit", "quit");
    CP.SetAlias("xic", "sced");

    // Set up the 'constants' plot, the values are read-only.  We do a
    // bit of trickery here to avoid converting to text.  This is needed
    // before the init files are read, so we don't know the case
    // sensitivity.  Have to fix this later.
    //
    // The ft_constants are real values only.
    for (sConstant *c = ft_constants; c->name; c++) {
        if (!c->units)
            OP.vecSet(c->name, "1", true);
        else {
            char buf[64];
            char *ss = buf;
            const char *tt = c->units;
            *ss++ = '1';
            while (*tt) {
                if (*tt == '%' && *(tt+1) == 'c') {
                    *ss++ = DEF_UNITS_CATCHAR;
                    tt += 2;
                    continue;
                }
                *ss++ = *tt++;
            }
            *ss = 0;
            OP.vecSet(c->name, buf, true);
        }
        sDataVec *v = OP.vecGet(c->name, 0);
        v->set_realval(0, c->value);
    }

    // The complex constants (only one).
    OP.vecSet("const_j", "0,1", true);
}


// Set some standard variables and aliases, etc, and init the ccom
// stuff.  Called after startup scripts are processed.
//
void
IFsimulator::PostInit()
{
    post_init_done = true;

    // Fix case sensitivity of the constants plot.
    OP.constants()->set_case(sHtab::get_ciflag(CSE_VEC));

    if (!CP.GetFlag(CP_NOCC)) {
        // Add commands...
        Cmds.CcSetup();
        // And keywords... These are the ones that are constant...
        CP.AddKeyword(CT_LISTINGARGS, "deck");
        CP.AddKeyword(CT_LISTINGARGS, "logical");
        CP.AddKeyword(CT_LISTINGARGS, "physical");
        CP.AddKeyword(CT_LISTINGARGS, "expand");

        CP.AddKeyword(CT_STOPARGS, kw_when);
        CP.AddKeyword(CT_STOPARGS, kw_after);
        CP.AddKeyword(CT_STOPARGS, kw_at);
        CP.AddKeyword(CT_STOPARGS, kw_before);

        CP.AddKeyword(CT_PLOT, "new");
        CP.AddKeyword(CT_PLOTKEYWORDS, "vs");

        // Add spice options.
        for (int i = 0; i < OPTinfo.numParms; i++) {
            if (!(OPTinfo.analysisParms[i].dataType & IF_ASK))
                continue;
            CP.AddKeyword(CT_OPTARGS, OPTinfo.analysisParms[i].keyword);
        }

        // Add resource keywords
        for (int i = 0; i < STATinfo.numParms; i++) {
            if (!(STATinfo.analysisParms[i].dataType & IF_ASK))
                continue;
            CP.AddKeyword(CT_RUSEARGS, STATinfo.analysisParms[i].keyword);
        }

        CP.AddKeyword(CT_RUSEARGS, kw_all);
        CP.AddKeyword(CT_RUSEARGS, kw_elapsed);
        CP.AddKeyword(CT_RUSEARGS, kw_everything);
        CP.AddKeyword(CT_RUSEARGS, kw_faults);
        CP.AddKeyword(CT_RUSEARGS, kw_space);
        CP.AddKeyword(CT_RUSEARGS, kw_totaltime);
        CP.AddKeyword(CT_OPTARGS, kw_all);
        CP.AddKeyword(CT_OPTARGS, kw_everything);

        CP.AddKeyword(CT_VECTOR, kw_all);

        for (int i = 0; ; i++) {
            const char *s;
            if (!(s = TypeNames(i)))
                break;
            CP.AddKeyword(CT_TYPENAMES, s);
        }
    }

    DefineUserFunc("max(x,y)", "x > y ? x : y");
    DefineUserFunc("min(x,y)", "x < y ? x : y");

    // Add the scripts directory to the path.
    if (Global.InputPath() && *Global.InputPath()) {
        char *bf = new char[strlen(Global.InputPath()) + 32];
        sprintf(bf, "%s = %s", kw_sourcepath, Global.InputPath());
        wordlist *wl = CP.Lexer(bf);
        delete [] bf;
        CP.DoGlob(&wl);
        CP.StripList(wl);
        SetVar(wl);
        wordlist::destroy(wl);
    }
    else
        GRpkgIf()->ErrPrintf(ET_WARN, "system scripts path not set.\n");
}


//
// Exports, mostly called from shell code.
//

// This gets called before every command is executed...
//
void
IFsimulator::Periodic()
{
    SetFlag(FT_INTERRUPT, false);
    CheckSpace();
    OP.checkAsyncJobs();
    OP.vecGc();
}


// Deal here with implicit 'let' and 'source'.
//
bool
IFsimulator::ImplicitCommand(wordlist *wl)
{
    if (strchr(wl->wl_word, '=') ||
        (wl->wl_next && *wl->wl_next->wl_word == '=')) {
        // Implicit 'let'.
        CommandTab::com_let(wl);
        return (true);
    }
    FILE *fp = Sp.PathOpen(wl->wl_word, "r");
    if (fp) {
        // Implicit 'source'.
        fclose(fp);
        CP.PushArg(wl);
        CommandTab::com_source(wl);
        CP.PopArg();
        return (true);
    }
    return (false);
}


namespace {
    bool testv(sDataVec *v)
    {
        for (int i = 0; i < v->length(); i++) {
            if (v->realval(i) != 0.0 || v->imagval(i) != 0.0)
                return (true);
        }
        return (false);
    }
}


// Decide whether a condition is true or not.
//
bool
IFsimulator::IsTrue(wordlist *wl)
{
    // First do all the csh-type stuff here...
    wl = wordlist::copy(wl);
    CP.VarSubst(&wl);
    CP.BackQuote(&wl);
    CP.StripList(wl);

    char *str = wordlist::flatten(wl);
    wordlist::destroy(wl);

    const char *s = str;
    pnode *pn = GetPnode(&s, true);
    delete [] str;
    if (pn == 0)
        return (false);
    sDataVec *v = Evaluate(pn);
    delete pn;

    if (v->link()) {
        // It makes no sense to say while (all), but what the heck...
        for (sDvList *dl = v->link(); dl; dl = dl->dl_next) {
            if (testv(dl->dl_dvec))
                return (true);
        }
    }
    else {
        if (testv(v))
            return (true);
    }
    return (false);
}


// Set up FPE signal handling.
//
void
IFsimulator::SetFPEmode(FPEmode mode)
{
#ifdef HAVE_FENV_H
    if (mode == FPEdebug) {
#if defined(__APPLE__)
#if defined(__x86_64)
        _MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() & ~_MM_MASK_OVERFLOW);
        _MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() & ~_MM_MASK_DIV_ZERO);
        _MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() & ~_MM_MASK_INVALID);
#endif
#else
#ifdef WIN32
        _controlfp(0, _EM_OVERFLOW | _EM_ZERODIVIDE | _EM_INVALID);
#else
        feenableexcept(FE_OVERFLOW);
        feenableexcept(FE_DIVBYZERO);
        feenableexcept(FE_INVALID);
#endif
#endif
    }
    else {
        if (mode != FPEnoCheck && mode != FPEcheck)
            mode = FPEdefault;
#if defined(__APPLE__)
#if defined(__x86_64)
        _MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() | _MM_MASK_OVERFLOW);
        _MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() | _MM_MASK_DIV_ZERO);
        _MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() | _MM_MASK_INVALID);
#endif
#else
#ifdef WIN32
        _controlfp(_MCW_EM, _EM_OVERFLOW | _EM_ZERODIVIDE | _EM_INVALID);
#else
        fedisableexcept(FE_OVERFLOW);
        fedisableexcept(FE_DIVBYZERO);
        fedisableexcept(FE_INVALID);
#endif
#endif
    }
#endif
    ft_fpe_mode = mode;
}


// Set the FPE mode to the value set in the .options of the current
// circuit, if set there.  This is done just before analysis.  The
// caller should use the return value to reset the mode when analysis
// returns.
//
FPEmode
IFsimulator::SetCircuitFPEmode()
{
    FPEmode fpemode_bak = ft_fpe_mode;
    if (ft_curckt) {
        for (variable *v = ft_curckt->vars(); v; v = v->next()) {
            if (lstring::cieq(v->name(), kw_fpemode)) {
                int inum = FPEdefault;
                if (v->type() == VTYP_BOOL)
                    inum = FPEnoCheck;
                else if (v->type() == VTYP_NUM)
                    inum = v->integer();
                else if (v->type() == VTYP_REAL)
                    inum = rnd(v->real());
                else
                    break;
                switch (inum) {
                case FPEcheck:
                    SetFPEmode(FPEcheck);
                    break;
                case FPEnoCheck:
                    SetFPEmode(FPEnoCheck);
                    break;
                case FPEdebug:
                    SetFPEmode(FPEdebug);
                    break;
                default:
                    GRpkgIf()->ErrPrintf(ET_WARN, 
                        "option %s bad value, ignored.", kw_fpemode);
                }
                break;
            }
        }
    }
    return (fpemode_bak);
}
// End of IFsimulator functions.


void
CommandTab::com_setcase(wordlist *wl)
{
    if (!wl || !wl->wl_word || !*wl->wl_word) {
        char buf[32];
        buf[0] = sHtab::get_ciflag(CSE_FUNC) ? 'F' : 'f';
        buf[1] = sHtab::get_ciflag(CSE_UDF) ? 'U' : 'u';
        buf[2] = sHtab::get_ciflag(CSE_VEC) ? 'V' : 'v';
        buf[3] = sHtab::get_ciflag(CSE_PARAM) ? 'P' : 'p';
        buf[4] = sHtab::get_ciflag(CSE_CBLK) ? 'C' : 'c';
        buf[5] = sHtab::get_ciflag(CSE_NODE) ? 'N' : 'n';
        buf[6] = 0;
        const char *cs = "are";
        const char *ci = "are NOT";
        sLstr lstr;
        TTY.printf("\n    Flag string:  %s\n", buf);
        TTY.printf("    Function names %s case-sensitive.\n",
            sHtab::get_ciflag(CSE_FUNC) ? ci : cs);
        TTY.printf("    User-defined function names %s case-sensitive.\n",
            sHtab::get_ciflag(CSE_UDF) ? ci : cs);
        TTY.printf("    Vector names %s case-sensitive.\n",
            sHtab::get_ciflag(CSE_VEC) ? ci : cs);
        TTY.printf("    .PARAM names %s case-sensitive.\n",
            sHtab::get_ciflag(CSE_PARAM) ? ci : cs);
        TTY.printf("    Codeblock names %s case-sensitive.\n",
            sHtab::get_ciflag(CSE_CBLK) ? ci : cs);
        TTY.printf("    Node and device names %s case-sensitive.\n\n",
            sHtab::get_ciflag(CSE_NODE) ? ci : cs);
        return;
    }
    if (post_init_done) {
        GRpkgIf()->ErrPrintf(ET_ERROR,
        "case sensitivity can be set with setcase only in startup files.\n");
        return;
    }
    sHtab::parse_ciflags(wl->wl_word);
}

