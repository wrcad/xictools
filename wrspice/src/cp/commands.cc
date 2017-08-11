
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

#include "cshell.h"
#include "commands.h"
#include "spnumber/hash.h"
#include <algorithm>


//
// The "shell" command database.
//

// Instantiate the command database.
CommandTab Cmds;


sCommand *
CommandTab::FindCommand(const char *cname)
{
    if (!cname)
        return (0);
    if (!ct_cmdtab) {
        ct_cmdtab = new sHtab;
        for (sCommand *c = ct_list; c->co_func; c++) {
            if (c->co_comname)
                ct_cmdtab->add(c->co_comname, c);
        }
    }
    return ((sCommand*)sHtab::get(ct_cmdtab, cname));
}


void
CommandTab::CcSetup()
{
    CP.CcRestart(false);
    for (sCommand *c = ct_list; c->co_func; c++)
        CP.AddCommand(c->co_comname, c->co_cctypes);
}


namespace {
    bool hcomp(const sCommand *c1, const sCommand *c2)
    {
        return (strcmp(c1->co_comname, c2->co_comname) < 0);
    }
}


int
CommandTab::Commands(sCommand ***list)
{
    int cnt = 0;
    for (sCommand *c = ct_list; c->co_func; c++)
        cnt++;

    if (list) {
        sCommand **clist = new sCommand*[cnt];
        *list = clist;
        cnt = 0;
        for (sCommand *c = ct_list; c->co_func; c++) {
            clist[cnt] = ct_list + cnt;
            cnt++;
        }
        std::sort(clist, clist + cnt, hcomp);
    }
    return (cnt);
}


// bitfields
#define Bnone 0
#define Bfile 1
#define Bvec  (1<<CT_VECTOR)
#define Budf  (1<<CT_UDFUNCS)
#define Bvar  (1<<CT_VARIABLES)
#define Bcmd  (1<<CT_COMMANDS)
#define Bplot (1<<CT_VECTOR)|(1<<CT_PLOTKEYWORDS)
#define Bplc  (1<<CT_PLOT)|(1<<CT_CKTNAMES)
#define Bpln  (1<<CT_PLOT)
#define Bckt  (1<<CT_CKTNAMES)
#define Blst  (1<<CT_LISTINGARGS)
#define Bshow (1<<CT_DEVNAMES)|(1<<CT_MODNAMES)|(1<<CT_RUSEARGS)|(1<<CT_OPTARGS)
#define Balt  (1<<CT_DEVNAMES)|(1<<CT_MODNAMES)|(1<<CT_OPTARGS)
#define Bstop (1<<CT_STOPARGS)|(1<<CT_NODENAMES)
#define Bnode (1<<CT_NODENAMES)
#define Bhlp  (1<<CT_COMMANDS)|(1<<CT_RUSEARGS)|(1<<CT_OPTARGS)
#define Brus  (1<<CT_RUSEARGS)
#define Btype (1<<CT_TYPENAMES)
 
namespace cmdnames {
    const char *cmd_ac          = "ac";
    const char *cmd_alias       = "alias";
    const char *cmd_alter       = "alter";
    const char *cmd_asciiplot   = "asciiplot";
    const char *cmd_aspice      = "aspice";
    const char *cmd_bug         = "bug";
    const char *cmd_cache       = "cache";
    const char *cmd_cd          = "cd";
    const char *cmd_cdump       = "cdump";
    const char *cmd_check       = "check";
    const char *cmd_codeblock   = "codeblock";
    const char *cmd_combine     = "combine";
    const char *cmd_compose     = "compose";
    const char *cmd_cross       = "cross";
    const char *cmd_dc          = "dc";
    const char *cmd_define      = "define";
    const char *cmd_deftype     = "deftype";
    const char *cmd_delete      = "delete";
    const char *cmd_destroy     = "destroy";
    const char *cmd_devcnt      = "devcnt";
    const char *cmd_devload     = "devload";
    const char *cmd_devls       = "devls";
    const char *cmd_devmod      = "devmod";
    const char *cmd_diff        = "diff";
    const char *cmd_display     = "display";
    const char *cmd_disto       = "disto";
    const char *cmd_dump        = "dump";
    const char *cmd_dumpnodes   = "dumpnodes";
    const char *cmd_dumpopts    = "dumpopts";
    const char *cmd_echo        = "echo";
    const char *cmd_echof       = "echof";
    const char *cmd_edit        = "edit";
    const char *cmd_fourier     = "fourier";
    const char *cmd_free        = "free";
    const char *cmd_hardcopy    = "hardcopy";
    const char *cmd_help        = "help";
    const char *cmd_helpreset   = "helpreset";
    const char *cmd_history     = "history";
    const char *cmd_iplot       = "iplot";
    const char *cmd_jobs        = "jobs";
    const char *cmd_let         = "let";
    const char *cmd_linearize   = "linearize";
    const char *cmd_listing     = "listing";
    const char *cmd_load        = "load";
    const char *cmd_loop        = "loop";       // alias for "sweep"
    const char *cmd_mapkey      = "mapkey";
    const char *cmd_mmon        = "mmon";
    const char *cmd_mplot       = "mplot";
    const char *cmd_noise       = "noise";
    const char *cmd_op          = "op";
    const char *cmd_passwd      = "passwd";
    const char *cmd_pause       = "pause";
    const char *cmd_pick        = "pick";
    const char *cmd_plot        = "plot";
    const char *cmd_plotwin     = "plotwin";
    const char *cmd_print       = "print";
    const char *cmd_proxy       = "proxy";
    const char *cmd_pwd         = "pwd";
    const char *cmd_pz          = "pz";
    const char *cmd_qhelp       = "qhelp";
    const char *cmd_quit        = "quit";
    const char *cmd_rehash      = "rehash";
    const char *cmd_reset       = "reset";
    const char *cmd_resume      = "resume";
    const char *cmd_retval      = "retval";
    const char *cmd_rhost       = "rhost";
    const char *cmd_rspice      = "rspice";
    const char *cmd_run         = "run";
    const char *cmd_rusage      = "rusage";
    const char *cmd_save        = "save";
    const char *cmd_sced        = "sced";
    const char *cmd_seed        = "seed";
    const char *cmd_sens        = "sens";
    const char *cmd_set         = "set";
    const char *cmd_setcase     = "setcase";
    const char *cmd_setcirc     = "setcirc";
    const char *cmd_setdim      = "setdim";
    const char *cmd_setfont     = "setfont";
    const char *cmd_setplot     = "setplot";
    const char *cmd_setrdb      = "setrdb";
    const char *cmd_setscale    = "setscale";
    const char *cmd_settype     = "settype";
    const char *cmd_shell       = "shell";
    const char *cmd_shift       = "shift";
    const char *cmd_spec        = "spec";
    const char *cmd_show        = "show";
    const char *cmd_source      = "source";
    const char *cmd_state       = "state";
    const char *cmd_stats       = "stats";
    const char *cmd_status      = "status";
    const char *cmd_step        = "step";
    const char *cmd_stop        = "stop";
    const char *cmd_strcicmp    = "strcicmp";
    const char *cmd_strciprefix = "strciprefix";
    const char *cmd_strcmp      = "strcmp";
    const char *cmd_strprefix   = "strprefix";
    const char *cmd_sweep       = "sweep";
    const char *cmd_tbsetup     = "tbsetup";
    const char *cmd_tf          = "tf";
    const char *cmd_trace       = "trace";
    const char *cmd_tran        = "tran";
    const char *cmd_unalias     = "unalias";
    const char *cmd_undefine    = "undefine";
    const char *cmd_unlet       = "unlet";
    const char *cmd_unset       = "unset";
    const char *cmd_update      = "update";
    const char *cmd_usrset      = "usrset";
    const char *cmd_version     = "version";
    const char *cmd_where       = "where";
    const char *cmd_write       = "write";
    const char *cmd_wrupdate    = "wrupdate";
    const char *cmd_xeditor     = "xeditor";
    const char *cmd_xgraph      = "xgraph";

    // not documented, for testing error reporting
    const char *cmd_debug_fault = "fault";

    // language elements
    const char *cmd_while       = "while";
    const char *cmd_repeat      = "repeat";
    const char *cmd_dowhile     = "dowhile";
    const char *cmd_foreach     = "foreach";
    const char *cmd_if          = "if";
    const char *cmd_else        = "else";
    const char *cmd_end         = "end";
    const char *cmd_break       = "break";
    const char *cmd_continue    = "continue";
    const char *cmd_label       = "label";
    const char *cmd_goto        = "goto";
}
using namespace cmdnames;


// fields:  name, func, stringargs, spiceonly, major, {cctypes}, helpenv,
//     minargs, maxargs, promptfunc, helpmsg
//
sCommand CommandTab::ct_list[] = {
    sCommand( cmd_ac, com_ac, false, true, true,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[.ac card args] : Do an ac analysis." ) ,
    sCommand( cmd_alias, com_alias, false, false, false,
      Bcmd, Bcmd, Bcmd, Bcmd, E_ADVANCED, 0, LOTS, 0,
      "[[word] alias] : Define an alias." ) ,
    sCommand( cmd_alter, com_alter, false, true, false,
      Balt, Balt, Balt, Balt, E_DEFHMASK, 0, LOTS, 0,
      "devspecs : parmname value : Alter device parameters." ) ,
    sCommand( cmd_asciiplot, com_asciiplot, false, false, true,
      Bplot, Bplot, Bplot, Bplot, E_DEFHMASK, 1, LOTS, 0,
      "expr ... [vs expr] : Produce ascii plots." ) ,
    sCommand( cmd_aspice, com_aspice, false, false, false,
      Bfile, Bfile, Bfile, Bfile, E_DEFHMASK, 1, 2, 0,
      "file [outfile] : Run a spice job asynchronously." ) ,
    sCommand( cmd_bug, com_bug, false, false, true,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": Report a %s bug." ) ,
    sCommand( cmd_cache, com_cache, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[h|l|d|r|c] [tagname ...] : manipulate subckt/model cache." ) ,
    sCommand( cmd_cd, com_cd, false, false, false,
      Bfile, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 1, 0,
      "[directory] : Change working directory." ) ,
    sCommand( cmd_cdump, com_cdump, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": Dump the current control structures." ) ,
    sCommand( cmd_check, com_check, false, true, true,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[-options] [analysis]: Perform operating range analysis." ) ,
    sCommand( cmd_codeblock, com_codeblock, false, false, false,
      Bfile, Bfile, Bfile, Bfile, E_DEFHMASK, 0, 3, 0,
      "codeblock [filename] [print] [free]: Name an executable codeblock." ) ,
    sCommand( cmd_combine, com_combine, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      " : Combine plots." ) ,
    sCommand( cmd_compose, com_compose, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 2, LOTS, 0,
      "var parm=val ... : Compose a vector." ) ,
    sCommand( cmd_cross, com_cross, false, false, true,
      Bvec, Bnone, Bvec, Bvec, E_DEFHMASK, 2, LOTS, 0,
      "vecname number [ vector ... ] : Make a vector in a strange way." ) ,
    sCommand( cmd_dc, com_dc, false, true, true,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[.dc card args] : Do a dc analysis." ) ,
    sCommand( cmd_define, com_define, false, false, true,
      Budf, Bvec, Bvec, Bvec, E_DEFHMASK, 0, LOTS, 0,
      "[[func (args)] stuff] : Define a user-definable function." ) ,
    sCommand( cmd_deftype, com_deftype, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 3, LOTS, 0,
      "spec name pat ... : Redefine vector and plot types." ) ,
    sCommand( cmd_delete, com_delete, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[all] [break number ...] : Delete breakpoints and traces." ) ,
    sCommand( cmd_destroy, com_destroy, false, false, false,
      Bpln, Bpln, Bpln, Bpln, E_DEFHMASK, 0, LOTS, 0,
      "[plotname] ... : Throw away all the data in the plot." ) ,
    sCommand( cmd_devcnt, com_devcnt, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "modnames : Print device instance counts." ) ,
    sCommand( cmd_devload, com_devload, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 1, 0,
      "path : Load a device object module." ) ,
    sCommand( cmd_devls, com_devls, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      ": List known devices." ) ,
    sCommand( cmd_devmod, com_devmod, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 1, LOTS, 0,
      ": Set model levels." ) ,
    sCommand( cmd_diff, com_diff, false, false, false,
      Bpln, Bpln, Bvec, Bvec, E_DEFHMASK, 0, LOTS, 0,
      "plotname plotname [vec ...] : 'diff' two plots." ) ,
    sCommand( cmd_display, com_display, false, false, true,
      Bvec, Bvec, Bvec, Bvec, E_BEGINNING, 0, LOTS, arg_display,
      ": Display vector status." ) ,
    sCommand( cmd_disto, com_disto, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[.disto card args] : Do an distortion analysis." ) ,
    sCommand( cmd_dump, com_dump, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": Print the current circuit matrix." ) ,
    sCommand( cmd_dumpnodes, com_dumpnodes, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": Print the current circuit last node voltages." ) ,
    sCommand( cmd_dumpopts, com_dumpopts, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": Print the current circuit options." ) ,
    sCommand( cmd_echo, com_echo, false, false, false,
      Bfile, Bfile, Bfile, Bfile, E_DEFHMASK, 0, LOTS, 0,
      "[stuff ...] : Print stuff." ) ,
    sCommand( cmd_echof, com_echof, false, false, false,
      Bfile, Bfile, Bfile, Bfile, E_DEFHMASK, 0, LOTS, 0,
      "[stuff ...] : Print stuff to current output file." ) ,
    sCommand( cmd_edit, com_edit, false, true, true,
      Bfile, Bfile, Bfile, Bfile, E_DEFHMASK, 0, 3, 0,
      "[filename] : Edit a spice deck and then load it in." ) ,
    sCommand( cmd_fourier, com_fourier, false, false, true,
      Bnone, Bvec, Bvec, Bvec, E_DEFHMASK, 1, LOTS, 0,
      "fund_freq vector ... : Do a fourier analysis of some data." ) ,
    sCommand( cmd_free, com_free, false, false, true,
      Bplc, Bplc, Bplc, Bplc, E_DEFHMASK, 0, LOTS, 0,
      ": delete current plot, circuit." ) ,
    sCommand( cmd_hardcopy, com_hardcopy, false, false, true,
      Bfile, Bplot, Bplot, Bplot, E_DEFHMASK, 0, LOTS, 0,
      "file plotargs : Produce hardcopy plots." ) ,
    sCommand( cmd_help, com_help, false, false, true,
      Bhlp, Bhlp, Bhlp, Bhlp, E_DEFHMASK, 0, LOTS, 0,
      "[subject] ... : Hierarchical documentation browser." ) ,
    sCommand( cmd_helpreset, com_helpreset, false, false, false,
      0, 0, 0, 0, E_DEFHMASK, 0, 0, 0,
      ": Re-read help database." ) ,
    sCommand( cmd_history, com_history, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 2, 0,
      "[-r] [number] : Print command history." ) ,
    sCommand( cmd_iplot, com_iplot, false, true, true,
      Bvec, Bvec, Bvec, Bvec, E_DEFHMASK, 0, LOTS, 0,
      "expr ... [vs expr] : Incrementally plot during simulation." ) ,
    sCommand( cmd_jobs, com_jobs, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": Report on asynchronous spice jobs." ) ,
    sCommand( cmd_let, com_let, false, false, true,
      Bvec, Bvec, Bvec, Bvec, E_DEFHMASK, 0, LOTS, arg_let,
      "varname = expr : Assign vector variables." ) ,
    sCommand( cmd_linearize, com_linearize, false, true, false,
      Bvec, Bvec, Bvec, Bvec, E_DEFHMASK, 0, LOTS, 0,
      "[ vec ... ] : Convert plot into one with linear scale." ) ,
    sCommand( cmd_listing, com_listing, false, true, true,
      Blst, Blst, Blst, Blst, E_DEFHMASK, 0, LOTS, 0,
      "[logical] [physical] [deck] : Print the current circuit." ) ,
    sCommand( cmd_load, com_load, false, false, true,
      Bfile, Bfile, Bfile, Bfile, E_BEGINNING|E_NOPLOTS, 1, LOTS, arg_load,
      "file ... : Load in data." ) ,
    sCommand( cmd_loop, com_sweep, false, true, true,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[loop args] [analysis]: run analysis over range." ) ,
    sCommand( cmd_mapkey, com_mapkey, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      ": manipulate keyboard mapping." ) ,
    sCommand( cmd_mmon, com_mmon, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 2, 0,
      ": memory monitor control." ) ,
    sCommand( cmd_mplot, com_mplot, false, false, false,
      Bfile, Bfile, Bfile, Bfile, E_DEFHMASK, 0, LOTS, 0,
      ": plot results from margin analysis." ) ,
    sCommand( cmd_noise, com_noise, false, true, true,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[.noise card args] : Do a noise analysis." ) ,
    sCommand( cmd_op, com_op, false, true, true,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[.op card args] : Determine the operating point of the circuit." ) ,
    sCommand( cmd_passwd, com_passwd, false, false, false,
      0, 0, 0, 0, E_DEFHMASK, 0, 0, 0,
      ": set update repository password." ),
    sCommand( cmd_pause, com_pause, false, false, false,
      0, 0, 0, 0, E_DEFHMASK, 0, 0, 0,
      ": Wait for keypress." ),
    sCommand( cmd_pick, com_pick, false, false, true,
      Bvec, Bnone, Bnone, Bvec, E_DEFHMASK, 4, LOTS, 0,
      "vecname offset period vector ... : Make a vector in a strange way." ) ,
    sCommand( cmd_plot, com_plot, false, false, true,
      Bplot, Bplot, Bplot, Bplot, E_BEGINNING|E_HASPLOTS, 0, LOTS, 0,
      "expr ... [vs expr] : Plot things." ),
    sCommand( cmd_plotwin, com_plotwin, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 2, 0,
      "[kill id]: pop down plot windows." ),
    sCommand( cmd_print, com_print, false, false, true,
      Bvec, Bvec, Bvec, Bvec, E_BEGINNING, 0, LOTS, 0,
      "[col] expr ... : Print vector values." ) ,
    sCommand( cmd_proxy, com_proxy, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 2, 0,
      ": Create/update .wrproxy file." ) ,
    sCommand( cmd_pwd, com_pwd, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": Print working directory." ) ,
    sCommand( cmd_pz, com_pz, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[.pz card args] : Do a pole / zero analysis." ) ,
    sCommand( cmd_qhelp, com_qhelp, false, false, true,
      Bcmd, Bcmd, Bcmd, Bcmd, E_DEFHMASK, 0, LOTS, 0,
      "[command name] ... : Print quick help." ) ,
    sCommand( cmd_quit, com_quit, false, false, true,
      Bnone, Bnone, Bnone, Bnone, E_BEGINNING, 0, 0, 0,
      ": Quit %s." ) ,
    sCommand( cmd_rehash, com_rehash, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": Rebuild the unix command database." ) ,
    sCommand( cmd_reset, com_reset, false, true, true,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 1, 0,
      ": Terminate a paused simulation." ) ,
    sCommand( cmd_resume, com_resume, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": Continue after a stop." ) ,
    sCommand( cmd_retval, com_retval, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 1, 1, 0,
      "value : Set $? to value." ) ,
    sCommand( cmd_rhost, com_rhost, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[-d] [-a] [hostname] ...: manipulate rspice host list."),
    sCommand( cmd_rspice, com_rspice, false, false, false,
      Bfile, Bfile, Bfile, Bfile, E_DEFHMASK, 0, LOTS, 0,
      "[input file] : Run a spice job remotely." ) ,
    sCommand( cmd_run, com_run, false, true, true,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 1, 0,
      "[rawfile] : Run the simulation as specified in the input file." ) ,
    sCommand( cmd_rusage, com_rusage, false, false, false,
      Brus, Brus, Brus, Brus, E_DEFHMASK, 0, LOTS, 0,
      "[resource ...] : Print current resource usage." ) ,
    sCommand( cmd_save, com_save, false, true, false,
      Bnode, Bnode, Bnode, Bnode, E_DEFHMASK, 0, LOTS, 0,
      "[all] [node ...] : Save a circuit output." ) ,
    sCommand( cmd_sced, com_sced, false, true, true,
      Bfile, Bfile, Bfile, Bfile, E_DEFHMASK, 0, 2, 0,
      "[-r/-n][filename] : Graphically edit a spice deck." ) ,
    sCommand( cmd_seed, com_seed, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 1, 0,
      "[integer] : Seed and reset random numbers." ) ,
    sCommand( cmd_sens, com_sens, false, true, true,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[.sens card args] : Do a sensitivity analysis." ) ,
    sCommand( cmd_set, com_set, false, false, true,
      Bvar, Bvar, Bvar, Bvar, E_DEFHMASK, 0, LOTS, arg_set,
      "[option] [option = value] ... : Set a variable." ) ,
    sCommand( cmd_setcase, com_setcase, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 1, 0,
      "[case_string] : Set program case sensitivity." ) ,
    sCommand( cmd_setcirc, com_setcirc, false, true, false,
      Bckt, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 1, 0,
      "[circuit name] : Change the current circuit." ) ,
    sCommand( cmd_setdim, com_setdim, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 8, 0,
      "ndims dim1 dim2 ... : Change current plot dimensions." ) ,
    sCommand( cmd_setfont, com_setfont, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 2, LOTS, 0,
      "font_num font_name : Set a screen font." ) ,
    sCommand( cmd_setplot, com_setplot, false, false, true,
      Bpln, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 1, 0,
      "[plotname] : Change the current working plot." ) ,
    sCommand( cmd_setrdb, com_setrdb, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 2, 0,
      "[resource: value] : set resources for X windows." ) ,
    sCommand( cmd_setscale, com_setscale, false, false, false,
      Bplot, Bvec, Bvec, Bvec, E_DEFHMASK, 1, LOTS, 0,
      "[list] vector : set default scale." ) ,
    sCommand( cmd_settype, com_settype, false, false, false,
      Btype, Bvec, Bvec, Bvec, E_DEFHMASK, 0, LOTS, 0,
      "type vec ... : Change the type of a vector." ) ,
    sCommand( cmd_shell, com_shell, false, false, true,
      Bfile, Bfile, Bfile, Bfile, E_DEFHMASK, 0, LOTS, 0,
      "[args] : Fork a shell, or execute the command." ) ,
    sCommand( cmd_shift, com_shift, false, false, false,
      Bvec, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 2, 0,
      "[var] [number] : Shift argv or the named list var to the left." ) ,
    sCommand( cmd_spec, com_spec, false, false, true,
      Bnone, Bnone, Bnone, Bvec, E_DEFHMASK, 4, LOTS, 0,
      "start_freq stop_freq step_freq vector ... : "
      "Create a frequency domain plot." ) ,
    sCommand( cmd_show, com_show, false, true, false,
      Bshow, Bshow, Bshow, Bshow, E_DEFHMASK, 0, LOTS, 0,
      "[devspecs ... , parmspecs ...] : Print out device parameters." ) ,
    sCommand( cmd_source, com_source, false, false, true,
      Bfile, Bfile, Bfile, Bfile, E_DEFHMASK, 1, LOTS, 0,
      "file : Source a %s file." ) ,
    sCommand( cmd_state, com_state, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "(unimplemented) : Print the state of the circuit." ),
    sCommand( cmd_stats, com_stats, false, false, false,
      Brus, Brus, Brus, Brus, E_DEFHMASK, 0, LOTS, 0,
      "[resource ...] : Print current run statistics." ) ,
    sCommand( cmd_status, com_status, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": Print the current breakpoints and traces." ) ,
    sCommand( cmd_step, com_step, false, true, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 1, 0,
      "[number] : Advance number points, or one." ) ,
    sCommand( cmd_stop, com_stop, false, true, false,
      Bstop, Bstop, Bstop, Bstop, E_DEFHMASK, 0, LOTS, 0,
      "[stop args] : Set a breakpoint." ) ,
    sCommand( cmd_strcicmp, com_strcicmp, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 2, 2, 0,
      "s1 s2 : Set $? to strcasecmp(s1, s2)." ) ,
    sCommand( cmd_strciprefix, com_strciprefix, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 2, 2, 0,
      "s1 s2 : Set $? to 1 if s1 is a prefix of s2, case insensitive." ) ,
    sCommand( cmd_strcmp, com_strcmp, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 2, 3, 0,
      "s1 s2 : Set $? to strcmp(s1, s2)." ) ,
    sCommand( cmd_strprefix, com_strprefix, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 2, 2, 0,
      "s1 s2 : Set $? to 1 if s1 is a prefix of s2, case sensitive." ) ,
    sCommand( cmd_sweep, com_sweep, false, true, true,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[sweep args] [analysis]: run analysis over range." ) ,
    sCommand( cmd_tbsetup, com_tbsetup, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_ADVANCED, 0, LOTS, 0,
      "[definitions]... : set up GUI from init file." ) ,
    sCommand( cmd_tf, com_tf, false, true, true,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[.tran card args] : Do a transient analysis." ) ,
    sCommand( cmd_trace, com_trace, false, true, false,
      Bnode, Bnode, Bnode, Bnode, E_DEFHMASK, 0, LOTS, 0,
      "[all] [expr ...] : Trace a circuit variable." ) ,
    sCommand( cmd_tran, com_tran, false, true, true,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[.tran card args] : Do a transient analysis." ) ,
    sCommand( cmd_unalias, com_unalias, false, false, false,
      Bcmd, Bcmd, Bcmd, Bcmd, E_DEFHMASK, 1, LOTS, 0,
      "word ... : Undefine an alias." ) ,
    sCommand( cmd_undefine, com_undefine, false, false, false,
      Budf, Budf, Budf, Budf, E_DEFHMASK, 0, LOTS, 0,
      "[func ...] : Undefine a user-definable function." ) ,
    sCommand( cmd_unlet, com_unlet, false, false, false,
      Bvec, Bvec, Bvec, Bvec, E_DEFHMASK, 1, LOTS, 0,
      "varname ... : Undefine vectors." ) ,
    sCommand( cmd_unset, com_unset, false, false, false,
      Bvec, Bvec, Bvec, Bvec, E_DEFHMASK, 0, LOTS, 0,
      "varname ... : Unset a variable." ) ,
    sCommand( cmd_update, com_update, false, false, false,
      Bfile, Bnone, Bnone, Bnone, E_ADVANCED, 0, 2, 0,
      "[file] : dump current setup to spinit file." ) ,
    sCommand( cmd_usrset, com_usrset, false, false, false,
      Bvar, Bvar, Bvar, Bvar, E_DEFHMASK, 0, LOTS, 0,
      ": Display list of option keywords." ) ,
    sCommand( cmd_version, com_version, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, LOTS, 0,
      "[number] : Print the version number." ) ,
    sCommand( cmd_where, com_where, false, true, true,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": Print last non-converging node or device" ) ,
    sCommand( cmd_write, com_write, false, false, true,
      Bfile, Bvec, Bvec, Bvec, E_DEFHMASK, 0, LOTS, 0,
      "file expr ... : Write data to a file." ) ,
    sCommand( cmd_wrupdate, com_wrupdate, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 5, 0,
      ": Download/install update." ),
    sCommand( cmd_xeditor, com_xeditor, false, true, true,
      Bfile, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 1, 0,
      "[filename] : Text editor for X windows." ) ,
    sCommand( cmd_xgraph, com_xgraph, false, false, true,
      Bfile, Bplot, Bplot, Bplot, E_DEFHMASK, 1, LOTS, 0,
      "file expr ... [vs expr] : Send plot to Xgraph-11." ) ,

    // not documented, for testing error reporting
    sCommand( cmd_debug_fault, com_fault, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      "debug_fault: kill program (for debugging)." ) ,

    sCommand( cmd_while, 0, false, false, false,
      Bvec, Bvec, Bvec, Bvec, E_DEFHMASK, 1, LOTS, 0,
      "condition : Execute while the condition is true." ) ,
    sCommand( cmd_repeat, 0, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 1, 0,
      "[number] : Repeat number times, or forever." ) ,
    sCommand( cmd_dowhile, 0, false, false, false,
      Bvec, Bvec, Bvec, Bvec, E_DEFHMASK, 1, LOTS, 0,
      "condition : Execute while the condition is true." ) ,
    sCommand( cmd_foreach, 0, false, false, false,
      Bnone, Bvec, Bvec, Bvec, E_DEFHMASK, 2, LOTS, 0,
      "variable value ... : Do once for each value." ) ,
    sCommand( cmd_if, 0, false, false, false,
      Bvec, Bvec, Bvec, Bvec, E_DEFHMASK, 1, LOTS, 0,
      "condition : Execute if the condition is true." ) ,
    sCommand( cmd_else, 0, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": Goes with if." ) ,
    sCommand( cmd_end, 0, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": End a block." ) ,
    sCommand( cmd_break, 0, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": Break out of a block." ) ,
    sCommand( cmd_continue, 0, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 0, 0, 0,
      ": Continue a loop." ) ,
    sCommand( cmd_label, 0, false, false, false,
      Bnone, Bnone, Bnone, Bnone, E_DEFHMASK, 1, 1, 0,
      "word : Create someplace to go to." ) ,
    sCommand( cmd_goto, 0, false, false, false,
      Bvar, Bnone, Bnone, Bnone, E_DEFHMASK, 1, 1, 0,
      "word : Go to a label." ) ,

    sCommand( 0, 0, false, false, false,
      0, 0, 0, 0, E_DEFHMASK, 0, LOTS, 0,
      0 )
};


void
CommandTab::com_fault(wordlist*)
{
    printf("Ouch, you killed me!\n");
    *(char*)-1 = 0;
}


// These functions are used to evalute arguments to a command
// and prompt the user if necessary.

namespace {
    inline int countargs(wordlist *wl)
    {
        int number = 0;
        for (wordlist *w = wl; w; w = w->wl_next)
            number++;
        return (number);
    }


    // A common prompt function.
    void common(const char *string, wordlist *wl, sCommand *command)
    {
        if (!countargs(wl)) {
            wordlist *w = CP.PromptUser(string);
            if (w == 0)
                return;
            (*command->co_func) (w);
            wordlist::destroy(w);
        }
    }
}


int
CommandTab::arg_load(wordlist *wl,  sCommand *command)
{
   // just call com_load
   (*command->co_func) (wl);
   return (0);
}


int
CommandTab::arg_let(wordlist *wl,  sCommand *command)
{
    common("which vector",  wl,  command);
    return (0);
}


int
CommandTab::arg_set(wordlist *wl,  sCommand *command)
{
    common("which variable",  wl,  command);
    return (0);
}


int
CommandTab::arg_display(wordlist*, sCommand*)
{
    // just return; display does the right thing
    return (0);
}

