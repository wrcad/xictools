
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

// Interface functions.  These are specific to spice.  The only
// changes to FTE that should be needed to make FTE work with a
// different simulator is to rewrite this file.  What each routine is
// expected to do can be found in the programmer's manual.  This file
// should be the only one that includes spice header files.

#include "config.h"
#include "spglobal.h"
#include "simulator.h"
#include "output.h"
#include "datavec.h"
#include "runop.h"
#include "parser.h"
#include "cshell.h"
#include "kwords_analysis.h"
#include "commands.h"
#include "input.h"
#include "optdefs.h"
#include "device.h"
#include "subexpand.h"
#include "toolbar.h"
#include "verilog.h"
#include "spnumber/paramsub.h"
int BoxFilled;  // security


void
CommandTab::com_state(wordlist*)
{
    if (!Sp.CurCircuit()) {
        Sp.Error(E_NOCURCKT);
        return;
    }
    TTY.printf("Current circuit: %s\n", Sp.CurCircuit()->name());
    if (!Sp.CurCircuit()->inprogress()) {
        TTY.printf("No run in progress.\n");
        return;
    }
    TTY.printf("Type of run: %s\n", OP.curPlot()->name());
    TTY.printf("Number of points so far: %d\n", 
        OP.curPlot()->scale()->length());
}


void
CommandTab::com_dump(wordlist *wl)
{
    if (!Sp.CurCircuit() || !Sp.CurCircuit()->runckt()) {
        Sp.Error(E_NOCURCKT);
        return;
    }
    bool reordered = false;
    bool data = true;
    bool header = true;
    const char *fname = 0;
    // dump [-r] [-c] [-t] [ -f filename]
    while (wl) {
        if (*wl->wl_word == '-') {
            bool ok = false;
            if (strchr(wl->wl_word+1, 'r')) {
                // Print reordered if matrix has been reordered.
                reordered = true;
                ok = true;
            }
            if (strchr(wl->wl_word+1, 'c')) {
                // Compact form, just show which entries are nonzero.
                data = false;
                ok = true;
            }
            if (strchr(wl->wl_word+1, 't')) {
                // Terse, omit header info.
                header = false;
                ok = true;
            }
            if (strchr(wl->wl_word+1, 'f')) {
                wl = wl->wl_next;
                if (wl)
                    fname = wl->wl_word;
                else
                    goto foobar;
                ok = true;
            }
            if (!ok)
                goto foobar;
        }
        else {
            goto foobar;
        }
        wl = wl->wl_next;
    }

    if (fname)
        Sp.CurCircuit()->runckt()->NIdbgPrint(reordered, data, header, fname);
    else
        Sp.CurCircuit()->runckt()->NIprint(reordered, data, header);
    return;
foobar:
    TTY.printf("dump: syntax error, expecting [-r][-c][-t][-f filename].\n");
}


void
CommandTab::com_where(wordlist*)
{
    if (!Sp.CurCircuit() || !Sp.CurCircuit()->runckt()) {
        Sp.Error(E_NOCURCKT);
        return;
    }
    const char *msg = Sp.CurCircuit()->runckt()->trouble(0);
    TTY.printf("Last nonconvergence at %s", msg);
}


void
CommandTab::com_vastep(wordlist*)
{
    if (!Sp.CurCircuit() || !Sp.CurCircuit()->runckt()) {
        Sp.Error(E_NOCURCKT);
        return;
    }
    sCKT *ckt = Sp.CurCircuit()->runckt();
    if (ckt->CKTvblk)
        ckt->CKTqueva = true;
}
// End of CommandTab functions.


// Set the current circuit to the one named.
//
void
IFsimulator::SetCircuit(const char *name)
{
    sFtCirc *p;
    for (p = ft_circuits; p; p = p->next())
        if (lstring::eq(name, p->name()))
            break;
    if (p == 0) {
        GRpkg::self()->ErrPrintf(ET_WARN, "no such circuit \"%s\".\n", name);
        return;
    }
    SetCurCircuit(p);
    ToolBar()->UpdateCircuits();
    ToolBar()->UpdateRunops();
}


void
IFsimulator::FreeCircuit(const char *name)
{
    sFtCirc *p;
    for (p = ft_circuits; p; p = p->next())
        if (lstring::eq(name, p->name()))
            break;
    if (p == 0)
        return;
    delete p;
}


//
// Utilities for codeblock handling.
//

// Bind codeblock to circuit.
//
void
IFsimulator::Bind(const char *name, bool exec)
{
    if (ft_curckt) {
        if (exec)
            ft_curckt->execBlk().set_name(name);
        else
            ft_curckt->controlBlk().set_name(name);
    }
    else
        GRpkg::self()->ErrPrintf(ET_ERROR, "no current circuit.\n");
}


// Unbind codeblock from circuit.
//
void
IFsimulator::Unbind(const char *name)
{
    if (!name)
        return;
    for (sFtCirc *ct = ft_circuits; ct; ct = ct->next()) {
        if (ct->execBlk().name() &&
                lstring::eq(ct->execBlk().name(), name))
            ct->execBlk().set_name(0);
        if (ct->controlBlk().name() &&
                lstring::eq(ct->controlBlk().name(), name))
            ct->controlBlk().set_name(0);
    }
}


// List bound blocks.
//
void
IFsimulator::ListBound()
{
    if (ft_curckt) {
        if (ft_curckt->execBlk().name()) {
            TTY.printf("\tBound exec codeblock: %s\n",
                ft_curckt->execBlk().name());
        }
        if (ft_curckt->controlBlk().name()) {
            TTY.printf("\tBound control codeblock: %s\n",
                ft_curckt->controlBlk().name());
        }
    }
}


// Set (isset true) or clear (isset false) an option in the circuit.
// Arguments are option name, type and value in the struct if setting.
// This is called only when setting an option through the shell, the
// .options are parsed and set directly.
//
void
IFsimulator::SetOption(bool isset, const char *word, IFdata *val)
{
    if (lstring::eq(word, spkw_acct)) {
        SetFlag(FT_ACCTPRNT, isset);
        return;
    }
    if (lstring::eq(word, spkw_dev)) {
        SetFlag(FT_DEVSPRNT, isset);
        return;
    }
    if (lstring::eq(word, spkw_list)) {
        SetFlag(FT_LISTPRNT, isset);
        return;
    }
    if (lstring::eq(word, spkw_mod)) {
        SetFlag(FT_MODSPRNT, isset);
        return;
    }
    if (lstring::eq(word, spkw_node)) {
        SetFlag(FT_NODESPRNT, isset);
        return;
    }
    if (lstring::eq(word, spkw_opts)) {
        SetFlag(FT_OPTSPRNT, isset);
        return;
    }
    if (lstring::eq(word, spkw_nopage)) {
        SetFlag(FT_NOPAGE, isset);
        return;
    }
    // numdgt is already set

    int i, err;
    for (i = 0; i < OPTinfo.numParms; i++) {
        if (lstring::eq(OPTinfo.analysisParms[i].keyword, word) &&
                (OPTinfo.analysisParms[i].dataType & IF_SET))
            break;
    }
    if (i == OPTinfo.numParms) {
        // See if this is unsupported or obsolete
        if (lstring::eq(word, spkw_itl3) ||
                lstring::eq(word, spkw_itl5) ||
                lstring::eq(word, spkw_lvltim) ||
                lstring::eq(word, spkw_cptime)) {
            GRpkg::self()->ErrPrintf(ET_WARN,
                "option %s is currently unsupported.\n", word);
            return;
        }
        if (lstring::eq(word, spkw_limpts) ||
                lstring::eq(word, spkw_limtim) ||
                lstring::eq(word, spkw_lvlcod) ||
                lstring::eq(word, spkw_nomod)) {
            GRpkg::self()->ErrPrintf(ET_WARN, "option %s is obsolete.\n", word);
            return;
        }
        return;
    }
    if (!isset) {
        if ((err = OPTinfo.setParm(0, OPTinfo.analysisParms[i].id, 0)) != OK)
            Error(err, "setAnalysisParm(options)");
        return;
    }

    IFdata pval;
    bool badtype = false;
    pval.type = (OPTinfo.analysisParms[i].dataType & IF_VARTYPES);
    int vtype = (val->type & IF_VARTYPES);
    if (vtype != pval.type) {
        if (pval.type == IF_REAL) {
            if (vtype == IF_INTEGER)
                pval.v.rValue = (double)val->v.iValue;
            else
                badtype = true;
        }
        else if (pval.type == IF_INTEGER) {
            if (vtype == IF_REAL)
                pval.v.iValue = (int)val->v.rValue;
            else if (vtype == IF_FLAG)
                pval.v.iValue = 1;
            else
                badtype = true;
        }
        else if (pval.type == IF_FLAG) {
            if (vtype == IF_INTEGER)
                pval.v.iValue = (val->v.iValue != 0);
            else
                pval.v.iValue = 1;
        }
        else if (pval.type == IF_STRING)
            badtype = true;
        else
            GRpkg::self()->ErrPrintf(ET_INTERR,
                "ci_setOption: bad option type %d.\n", pval.type);
    }
    else {
        if (pval.type == IF_FLAG)
            pval.v.iValue = 1;
        else
            pval.v = val->v;
    }

    if (!badtype) {
        err = OPTinfo.setParm(0, OPTinfo.analysisParms[i].id, &pval);
        if (err != OK)
            Error(err, "setAnalysisParm(options)");
        return;
    }

    const char *s1;
    int ptype = val->type & IF_VARTYPES;
    if (val->type & IF_VECTOR)
        s1 = "list";
    else if (ptype == IF_FLAG)
        s1 = "boolean";
    else if (ptype == IF_INTEGER)
        s1 = "integer";
    else if (ptype == IF_REAL)
        s1 = "real";
    else if (ptype == IF_STRING)
        s1 = "string";
    else
        s1 = "strange";

    const char *s;
    ptype = OPTinfo.analysisParms[i].dataType & IF_VARTYPES;
    if (ptype == IF_REAL)
        s = "real";
    else if (ptype == IF_INTEGER)
        s = "integer";
    else if (ptype == IF_STRING)
        s = "string";
    else if (ptype == IF_FLAG)
        s = "flag";
    else
        s = "unknown";

    GRpkg::self()->ErrPrintf(ET_ERROR,
        "%s type given for option %s, expected %s.\n", s1, word, s);
    if ((val->type & IF_VARTYPES) == IF_FLAG)
        GRpkg::self()->ErrPrintf(ET_MSG,
        "Note that you must use an = to separate option name and value.\n");
}


// This function changes the state of the currently set spice options
// according to the pl_ftopts in the current plot.  Called when the
// current plot changes to a different analysis plot.
//
void
IFsimulator::OptUpdate()
{
    variable *ovars = OP.curPlot()->options()->tovar();
    for (variable *v = ovars; v; v = v->next()) {
        if (v->type() == VTYP_BOOL && !v->boolean()) {
            // The variable is not set in the new plot, unset it unless
            // it is currently set in the shell.
            if (!CP.RawVarGet(v->name()))
                RemVar(v->name());
        }
        else {
            switch (v->type()) {
            case VTYP_BOOL:
                SetVar(v->name());
                break;
            case VTYP_NUM:
                SetVar(v->name(), v->integer());
                break;
            case VTYP_REAL:
                SetVar(v->name(), v->real());
                break;
            case VTYP_STRING:
                SetVar(v->name(), v->string());
                break;
            case VTYP_LIST:
                SetVar(v->name(), variable::copy(v->list()));
                break;
            default:
                break;
            }
        }
    }
    variable::destroy(ovars);
}


// Extract the node and device names from the line and add them to the command
// completion structure.  This is probably not a good thing to do if it
// takes too much time.
//
void
IFsimulator::AddNodeNames(const char *line)
{
    while (isspace(*line))
        line++;
    if (!*line || (*line == '*'))
        return;
    char buf[BSIZE_SP];
    if (*line == '.') {
        // We know that IP.kwMatchInit was called recently.
        if (SPcx.kwMatchModel(line)) {
            lstring::copytok(buf, &line);
            lstring::copytok(buf, &line);
            CP.AddKeyword(CT_MODNAMES, buf);
        }
        return;
    }
    lstring::copytok(buf, &line);
    CP.AddKeyword(CT_DEVNAMES, buf);

    // Inefficient and not very useful.
    int nmin, nmax;
    IP.numRefs(buf, &nmin, &nmax, 0, 0);
    if (nmin > 0) {
        for (int i = 0; i < nmin; i++) {
            if (lstring::copytok(buf, &line))
                CP.AddKeyword(CT_NODENAMES, buf);
        }
    }
}
// End of IFsimulator functions.


// Constructor
//
sFtCirc::sFtCirc()
{
    ci_next = 0;
    ci_name = 0;
    ci_descr = 0;
    ci_filename = 0;

    ci_deck = 0;
    ci_origdeck = 0;
    ci_verilog = 0;
    ci_options = 0;
    ci_params = 0;
    ci_vars = 0;
    ci_defines = 0;

    ci_commands = 0;

    ci_nodes = 0;
    ci_devices = 0;
    ci_models = 0;

    ci_defOpt = 0;
    ci_deferred = 0;
    ci_trial_deferred = 0;

    // The ci_symtab table is used for all UID types.  These are
    //  UID_ANALYSIS:
    //  UID_TASK:
    //  UID_INSTANCE:
    //  UID_OTHER:
    //  UID_MODEL:
    //
    // We'll make the table case-insensitive, which is appropriate for
    // model names.

    ci_symtab = new sSymTab(true);
    ci_runckt = 0;
    ci_sweep = 0;
    ci_check = 0;
    ci_runplot = 0;

    ci_runtype = 0;
    ci_inprogress = false;
    ci_runonce = false;
    ci_keep_deferred = false;
    ci_use_trial_deferred = false;

    static int count;
    count++;
    if (Sp.CircuitList()) {
        if (Sp.CircuitList()->ci_name) {
            const char *n = Sp.CircuitList()->ci_name;
            while (isalpha(*n))
                n++;
            if (isdigit(*n))
                count = atoi(n) + 1;
        }
    }
    else
        count = 1;

    char buf[32];
    snprintf(buf, sizeof(buf), "CKT%d", count);
    ci_name = lstring::copy(buf);
    ci_next = Sp.CircuitList();
    Sp.SetCircuitList(this);
}


// Destructor
//
sFtCirc::~sFtCirc()
{
    // Delete the references in the plots.
    for (sPlot *p = OP.plotList(); p; p = p->next_plot()) {
        if (p->circuit() && lstring::eq(p->circuit(), ci_name))
            p->set_circuit(0);
    }

    delete [] ci_name;
    clear();
    delete ci_symtab;

    // Pull it out of the circuits list and fix the global references.
    sFtCirc *cc, *cp = 0;
    for (cc = Sp.CircuitList(); cc; cc = cc->ci_next) {
        if (this == cc) {
            if (!cp)
                Sp.SetCircuitList(cc->ci_next);
            else
                cp->ci_next = cc->ci_next;
            break;
        }
        cp = cc;
    }
    if (!cc) {
        if (Sp.CurCircuit() == this)
            Sp.SetCurCircuit(0);
        GRpkg::self()->ErrPrintf(ET_INTERR, "deleted circuit not in list.\n");
    }
    else if (Sp.CurCircuit() == cc) {
        if (cc->ci_next)
            Sp.SetCurCircuit(cc->ci_next);
        else {
            for (cc = Sp.CircuitList(); cc && cc->ci_next; cc = cc->ci_next) ;
            Sp.SetCurCircuit(cc);
        }
    }
    ToolBar()->UpdateCircuits();
}


// The ci_name field is retained, also ci_next, all other fields are zeroed,
// after being freed
//
void
sFtCirc::clear()
{
    const sFtCirc *ftcirc = this;
    if (!ftcirc)
        return;

    delete [] ci_descr;         ci_descr = 0;
    delete [] ci_filename;      ci_filename = 0;

    sLine::destroy(ci_deck);    ci_deck = 0;
                                ci_origdeck = 0;
    sLine::destroy(ci_verilog); ci_verilog = 0;
    sLine::destroy(ci_options); ci_options = 0;
    delete ci_params;           ci_params = 0;
    variable::destroy(ci_vars); ci_vars = 0;
    delete ci_defines;          ci_defines = 0;

    ci_runops.clear();
    ci_execBlk.clear();
    ci_controlBlk.clear();
    ci_postrunBlk.clear();

    wordlist::destroy(ci_commands); ci_commands = 0;

    delete ci_nodes;            ci_nodes = 0;
    delete ci_devices;          ci_devices = 0;
    delete ci_models;           ci_models = 0;

    delete ci_defOpt;           ci_defOpt = 0;
    dfrdlist::destroy(ci_deferred);         ci_deferred = 0;
    dfrdlist::destroy(ci_trial_deferred);   ci_trial_deferred = 0;

    delete ci_symtab;           ci_symtab = new sSymTab(true);
    delete ci_runckt;           ci_runckt = 0;
    delete ci_sweep;            ci_sweep = 0;
                                ci_check = 0;
    if (ci_runplot)
        ci_runplot->set_active(false);
                                ci_runplot = 0;

                                ci_runtype = 0;
                                ci_inprogress = false;
                                ci_runonce = false;
                                ci_keep_deferred = false;
                                ci_use_trial_deferred = false;
}


namespace {
    // This returns a wordlist of parameters marked with IF_USEALL for the
    // device type
    //
    wordlist *plist(int type)
    {
        if (type < 0 || type >= DEV.numdevs())
            return (0);
        IFdevice *dv = DEV.device(type);
        if (!dv)
            return (0);
        wordlist *w0 = 0;
        for (int i = 0; ; i++) {
            IFparm *p = dv->instanceParm(i);
            if (!p)
                break;
            if (p->dataType & IF_USEALL) {
                wordlist *wl = new wordlist;
                wl->wl_word = lstring::copy(p->keyword);
                wl->wl_next = w0;
                w0 = wl;
            }
        }
        return (w0);
    }
}


int
sFtCirc::dotparam(const char *word, IFdata *data)
{
    if (!ci_params)
        return (E_BADPARM);
    const sParam *p = ci_params->get(word);
    if (p) {
        bool err;
        data->v.rValue = ci_params->eval(p, &err);
        // If the content does not parse as an expression, accept it
        // as a string.  Parameters can take string values so this is
        // legitimate.

        if (err) {
            data->v.sValue = p->sub();
            data->type = IF_STRING;
        }
        else
            data->type = IF_REAL;
        return (OK);
    }
    return (E_BADPARM);
}


// Report the percent if analysis completed.  This is used
// asynchronously for progress reporting.
//
double
sFtCirc::getPctDone()
{
    // Return 0 if not running analysis.
    if (!ci_runckt)
        return (0.0);

    // Return 0 if pointer not set for whatever reason.
    if (!ci_runckt->CKTcurTask || !ci_runckt->CKTcurJob)
        return (0.0);

    // Return 0 if running Operating Range or Monte Carlo analysis. 
    // For Operating Range, we really don't know the number of trials
    // needed beforehand.  We could probably do Monte Carlo,
    // implement this some time.
    if (ci_check)
        return (0.0);

    // Return 0 if multi-threading.
    if (ci_runckt->CKTcurTask->TSKloopThreads > 0 &&
            ci_runckt->CKTcurJob->threadable())
        return (0.0);

    // Deal with the two kinds of loops: chained dc and sweep.
    int nt = 1;
    if (ci_runckt->CKTnumDC > 1)
        nt *= ci_runckt->CKTnumDC;
    if (ci_sweep) {
        if (ci_sweep->points() > 1)
            nt *= ci_sweep->points();
    }

    if (nt > 1) {
        int t = 0;
        if (ci_sweep) {
            t = ci_sweep->loop_count;
            if (t > 0 && ci_runckt->CKTnumDC > 1)
                t *= ci_runckt->CKTnumDC;
        }
        t += ci_runckt->CKTcntDC;
        return ((100.0*t + ci_runckt->CKTstat->STATtranPctDone)/nt);

    }
    return (ci_runckt->CKTstat->STATtranPctDone);
}


// Concatenate and return the analysis directives from the circuit
// deck, stripping the '.'.
//
wordlist *
sFtCirc::getAnalysisFromDeck()
{
    wordlist *w0 = 0, *we = 0;
    for (sLine *c = ci_deck->next(); c; c = c->next()) {
        const char *s = c->line();
        char *tok = lstring::gettok(&s);
        if (!tok)
            continue;
        if (*tok != '.') {
            delete [] tok;
            continue;
        }
        if (lstring::cieq(tok, AC_KW) || lstring::cieq(tok, DC_KW) ||
                lstring::cieq(tok, DISTO_KW) || lstring::cieq(tok, NOISE_KW) ||
                lstring::cieq(tok, OP_KW) || lstring::cieq(tok, PZ_KW) ||
                lstring::cieq(tok, SENS_KW) || lstring::cieq(tok, TF_KW) ||
                lstring::cieq(tok, TRAN_KW)) {
            s = c->line();
            s = strchr(s, '.');
            s++;
            if (!w0)
                w0 = we = new wordlist(s, 0);
            else {
                we->wl_next = new wordlist(s, we);
                we = we->wl_next;
            }
        }
        delete [] tok;
    }
    return (w0);
}


// Return true and set data if name/range found in Verilog block
//
bool
sFtCirc::getVerilog(const char *word, const char *range, IFdata *data)
{
    if (!ci_runckt || !ci_runckt->CKTvblk)
        return (false);
    double val;
    if (ci_runckt->CKTvblk->query_var(word, range, &val)) {
        data->type = IF_REAL;
        data->v.rValue = val;
        return (true);
    }
    return (false);
}


// Add to the save list from the .save lines.
//
void
sFtCirc::getSaves(sSaveList *saved, const sCKT *ckt)
{
    for (wordlist *wl = ci_commands; wl; wl = wl->wl_next) {
        char *s = wl->wl_word;
        while (isspace(*s))
            s++;
        if (lstring::cimatch(SAVE_KW, s) || lstring::cimatch(PROBE_KW, s)) {
            lstring::advtok(&s); // skip ".save"
            char *t;
            while ((t = lstring::gettok(&s)) != 0) {
                saved->add_save(t);
                delete [] t;
            }
        }
    }

    // If "savecurrent" has been set, add all the device currents to the
    // saves list.
    if (ckt && Sp.GetVar(spkw_savecurrent, VTYP_BOOL, 0, this)) {
        sCKTmodGen mgen(ckt->CKTmodels);
        for (sGENmodel *m = mgen.next(); m; m = mgen.next()) {
            wordlist *wl = plist(m->GENmodType);
            if (wl) {
                for (sGENmodel *model = m; model;
                        model = model->GENnextModel) {
                    for (sGENinstance *inst = model->GENinstances; inst;
                            inst = inst->GENnextInstance) {
                        for (wordlist *w = wl; w; w = w->wl_next) {
                            char buf[128];
                            snprintf(buf, sizeof(buf), "@%s[%s]",
                                (char*)inst->GENname, w->wl_word);
                            saved->add_save(buf);
                        }
                    }
                }
                wordlist::destroy(wl);
            }
        }
    }
}
// End of sFtCirc functions.


void
sExBlk::clear()
{
    delete [] xb_name;
    xb_name = 0;
    wordlist::destroy(xb_text);
    xb_text = 0;
    CP.FreeControl(xb_tree);
    xb_tree = 0;
}


// Execute the codeblock.
//
void
sExBlk::exec(bool suppress)
{
    if (xb_name || xb_tree) {
        bool temp = CP.GetFlag(CP_INTERACTIVE);
        CP.SetFlag(CP_INTERACTIVE, false);
        TTY.ioPush();
        OP.pushPlot();

        if (suppress)
            ToolBar()->SuppressUpdate(true);
        ToolBar()->UpdateVectors(2);

        if (xb_name)
            CP.ExecBlock(xb_name);
        else if (xb_tree)
            CP.ExecControl(xb_tree);
        ToolBar()->UpdateVectors(2);

        if (suppress)
            ToolBar()->SuppressUpdate(false);

        OP.popPlot();
        TTY.ioPop();
        CP.SetFlag(CP_INTERACTIVE, temp);
    }
}

