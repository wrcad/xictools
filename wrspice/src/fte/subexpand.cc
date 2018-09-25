
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
Authors: 1985 Wayne A. Christopher, Norbert Jeske
         1992 Stephen R. Whiteley
****************************************************************************/

#include "frontend.h"
#include "fteparse.h"
#include "input.h"
#include "inpmtab.h"
#include "kwords_fte.h"
#include "kwords_analysis.h"
#include "ttyio.h"
#include "misc.h"
#include "wlist.h"
#include "subexpand.h"
#include "spnumber/paramsub.h"
#include "ginterf/graphics.h"

// Time profiling.
//#define TIME_DEBUG

#ifdef TIME_DBG
#include "outdata.h"
#endif


//
// Subcircuit expansion.  This also handles subcircuit/model caching.
//

// Alias object
//
struct sTabc
{
    sTabc()
        {
            t_old = 0;
            t_new = 0;
        }

    ~sTabc()
        {
            delete [] t_old;
            delete [] t_new;
        }

     const char *oldw()     { return (t_old); }
     const char *neww()     { return (t_new); }

     void set_oldw(char *s) { t_old = s; }
     void set_neww(char *s) { t_new = s; }

private:
    const char *t_old;
    const char *t_new;
};

// Subcircuit object
//
struct sSubc
{
    sSubc()
        {
            su_name = 0;
            su_args = 0;
            su_numargs = 0;
            su_params = 0;
            su_body = 0;
        }

    sSubc(sLine*);
    ~sSubc();

    static sSubc *copy(const sSubc*);

    void check_args(const char*, int);

    const char *su_name;    // The name.
    const char *su_args;    // The arguments, space seperated.
    int su_numargs;         // The argument count.
    sParamTab *su_params;   // Name = Value parameter list.
    sLine *su_body;         // The deck that is to be substituted.
};

// Hash table for sSubc objects.
//
struct sSubcTab : public sHtab
{
    // Subckt names are case insensitive.
    sSubcTab() : sHtab(true) { }
    ~sSubcTab();

    static sSubcTab *copy(const sSubcTab*);
};

// List link for model name translations.
//
struct sMods
{
    sMods(char *no, char *nt, sMods *n)
        { name_orig = no; name_trans = nt; next = n; }
    ~sMods() { delete [] name_orig; delete [] name_trans; }

    static void destroy(sMods *m)
        {
            while (m) {
                sMods *mx = m;
                m = m->next;
                delete mx;
            }
        }

    char *name_orig;
    char *name_trans;
    sMods *next;
};

// Keep a context stack, for subcircuits defined within subcircuits.
//
struct sSCX
{
    sSCX() { mods = 0; subs = 0; }
    void clear() { mods = 0; subs = 0; }

    sMods *mods;
    sSubcTab *subs;
};


#define SUB_STK_DEPTH 16

struct sScGlobal
{
    sScGlobal()
        {
            sg_stack_ptr = 0;
            sg_glob_tab = 0;
            sg_submaps = 0;
            memset(sg_stack, 0, SUB_STK_DEPTH*sizeof(sSCX));
        }

    ~sScGlobal()
        {
            delete sg_glob_tab;
            string2list::destroy(sg_submaps);
        }

    void init(sFtCirc*);
    sLine *expand_and_replace(sLine*, sParamTab**, cUdf**, const char* = 0);
    bool do_submapping(sLine*);
    bool extract_cache_block(sLine**, char**, sLine**);
    bool extract_subckts(sLine**);
    void addmod(const char*, const char*);
    void addsub(sLine*);
    sSubc *findsub(const char*, int = -1);
    void param_expand(sLine*, sParamTab*);
    bool translate(sLine*, const char*, const char*, const char*, const char*,
        sParamTab*);
    bool cache_add(const char*, sLine**, const sParamTab*, const cUdf*);
    void parse_call(const char*, char**, char**, char**, char**);
    void finishLine(sLstr*, const char*, const char*, sTabc*);
    sTabc *settrans(const char*, const char*, const char*);
    char *polytrans(const char**, bool, const char*, sTabc*);

    bool is_global(const char *tok)
        {
            if (!sg_glob_tab || !tok)
                return (false);
            return (sHtab::get(sg_glob_tab, tok) != 0);
        }

    const char *gettrans(const char *name, sTabc *table)
        {
            if (lstring::eq(name, "0"))
                return (name);
            if (is_global(name))
                return (name);
            for (int i = 0; table[i].oldw(); i++) {
                if (sScGlobal::name_eq(table[i].oldw(), name))
                    return (table[i].neww());
            }
            return (0);
        }


    // Token equality, may be case-sensitive or not.
    static bool name_eq(const char *s1, const char *s2)
        {
            if (sHtab::get_ciflag(CSE_NODE))
                return (!strcasecmp(s1, s2));
            return (!strcmp(s1, s2));
        }


private:
    int sg_stack_ptr;

    sHtab *sg_glob_tab;             // Global nodename table.

    string2list *sg_submaps;        // List of mappings for subckt calls
                                    // that are mapped to device calls:
                                    // x<pfx>... -> <val>..., or if <val>==0
                                    // x<pfx>... -> <pfx>.  Hack for HSPICE
                                    // and Verilog-A compatibility.

    sSCX sg_stack[SUB_STK_DEPTH];   // Stack for nested subckt processing.
};

// Cache block entry.
//
struct sCblk
{
    sCblk(sSubcTab *s, sParamTab *p, sModTab *m, cUdf *c)
        {
            cb_subs = s;
            cb_prms = p;
            cb_mods = m;
            cb_defines = c;
        }

    ~sCblk()
        {
            delete cb_subs;
            delete cb_prms;
            delete cb_mods;
            delete cb_defines;
        }

    wordlist *to_wl(wordlist*);

    sSubcTab *cb_subs;
    sParamTab *cb_prms;
    sModTab *cb_mods;
    cUdf *cb_defines;
};

// Hash table for cache block entries.
//
struct sCblkTab : public sHtab
{
    // Cache block names are case insensitive.
    sCblkTab() : sHtab(true) { }
    ~sCblkTab();

    wordlist *dump_wl(const char*);
};

// Instahtiate the cache and the context container.
sSPcache SPcache;
sSPcx SPcx;

const char *spkw_submaps = "submaps";

// Expand all subcircuits in the deck.  This handles embedded .subckt
// definitions.  The variables substart, subend, and subinvoke can be
// used to redefine the controls used.  The syntax is invariant
// though.  NOTE:  the deck must be passed without the title card. 
// What we do is as follows:  first make one pass through the circuit
// and collect all of the subcircuits.  Then, whenever a card that
// starts with 'x' is found, copy the subcircuit associated with that
// name and splice it in.  A few of the problems:  the nodes in the
// spliced-in stuff must be unique, so when we copy it, append
// "subcktname:" to each node.  If we are in a nested subcircuit, use
// foo:bar:...:node.  Then we have to systematically change all
// references to the renamed nodes.  On top of that, we have to know
// how many args BJT's have, so we have to keep track of model names.
//
bool
sFtCirc::expandSubckts()
{
    sScGlobal sg;
    sg.init(this);
    sLine *edeck = ci_deck->next();
    ci_deck->set_next(0);

    sLine *cache_blk;
    char *cache_name;
    if (!sg.extract_cache_block(&edeck, &cache_name, &cache_blk)) {
        sLine::destroy(edeck);
        return (false);
    }
    if (cache_name) {
        bool ret = sg.cache_add(cache_name, &cache_blk, ci_params, ci_defines);
        sLine::destroy(cache_blk);
        if (!ret) {
            sLine::destroy(edeck);
            return (false);
        }
    }

    sLine *ll = sg.expand_and_replace(edeck, &ci_params, &ci_defines,
        cache_name);
    delete [] cache_name;

    // Now check to see if there are still subckt instances undefined...
    for (sLine *c = ll; c; c = c->next()) {
        if (SPcx.kwMatchSubinvoke(c->line())) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "%s\nSubcircuit expansion, unknown subcircuit.\n", c->line());
            sLine::destroy(ll);
            return (false);
        }
    }
    ci_deck->set_next(ll);
    return (true);
}


void
sScGlobal::init(sFtCirc *circ)
{
    SPcx.kwMatchInit();
    SPcx.init(circ);

    sg_stack_ptr = 0;
    sg_stack[0].clear();
    string2list::destroy(sg_submaps);
    sg_submaps = 0;

    VTvalue vv;
    if (Sp.GetVar(spkw_submaps, VTYP_STRING, &vv, circ)) {
        string2list *se = 0;
        const char *str = vv.get_string();
        char *tok;
        while ((tok = lstring::gettok(&str)) != 0) {
            char *va = strchr(tok, ',');
            if (va)
                *va++ = 0;
            if (va && *va)
                va = lstring::copy(va);
            char *nm = lstring::copy(tok);
            delete [] tok;
            if (!sg_submaps)
                sg_submaps = se = new string2list(nm, va, 0);
            else {
                se->next = new string2list(nm, va, 0);
                se = se->next;
            }
        }
    }

    // Add the global node names to a hash table.
    //
    for (sLine *c = circ->deck()->next(); c; c = c->next()) {
        if (lstring::cimatch(GLOBAL_KW, c->line())) {
            if (!sg_glob_tab)
                sg_glob_tab = new sHtab(sHtab::get_ciflag(CSE_NODE));

            const char *s = c->line();
            IP.advTok(&s, true);
            char *tok;
            while ((tok = IP.getTok(&s, true)) != 0) {
                if (sHtab::get(sg_glob_tab, tok) == 0)
                    sg_glob_tab->add(tok, (void*)1L);
                delete [] tok;
            }
            // Comment out the .global line.
            c->comment_out();
        }
    }
}


sLine *
sScGlobal::expand_and_replace(sLine *deck, sParamTab **parm_ptr,
    cUdf **udf_ptr, const char *cache_name)
{
#ifdef TIME_DBG
    double tstart = OP.seconds();
    double tend = 0.0;
#endif

    sg_stack[sg_stack_ptr].clear();
    if (cache_name) {
        sCblk *blk = SPcache.get(cache_name);
        if (!blk) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "can't find cache block named %s.\n", cache_name);
            return (0);
        }
        sg_stack[sg_stack_ptr].subs = sSubcTab::copy(blk->cb_subs);
        *parm_ptr = sParamTab::update(*parm_ptr, blk->cb_prms);
        IP.setModCache(blk->cb_mods);

        // Swap in the cached function definitions.
        delete *udf_ptr;
        *udf_ptr = blk->cb_defines->copy();
    }

    sLine *lc = 0;
    wordlist *badcalls = 0;

    if (!extract_subckts(&deck)) {
        sLine::destroy(deck);
        deck = 0;
        goto cleanup;
    }

    // Get all the global model names.
    for (sLine *c = deck; c; c = c->next()) {
        if (SPcx.kwMatchModel(c->line())) {
            const char *s = c->line();
            IP.advTok(&s, true);
            char *nm = IP.getTok(&s, true);
            addmod(nm, nm);
            delete [] nm;
        }
    }

    for (sLine *c = deck; c; ) {
        if (!SPcx.kwMatchSubinvoke(c->line()) || do_submapping(c)) {
            if (sg_stack_ptr == 0)
                param_expand(c, *parm_ptr);
            lc = c;
            c = c->next();
            continue;
        }

        char *subname, *instname, *args, *params;
        parse_call(c->line(), &args, &subname, &instname, &params);
        if (!subname || !instname) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "%s\n%Subcircuit expansion, syntax error.\n", c->line());
            sLine::destroy(deck);
            deck = 0;
            goto cleanup;
        }

        // Note that we have skipped parameter expanding the subinvoke
        // line, but we still need to expand the subname.  This allows
        // aliasing subcircuit names in calls, e.g.
        //
        // .subckt mysub ...
        // ...
        // .ends mysub
        // .param sub=mysub
        // X0 (nodes) sub (params)

        (*parm_ptr)->param_subst_all(&subname);

        sSubc *sss = findsub(subname, sg_stack_ptr);
        if (!sss || !sss->su_body) {

            if (!sss) {
                // Continue on to find all unresolved calls, we'll
                // die later.

                bool found = false;
                for (wordlist *w = badcalls; w; w = w->wl_next) {
                    if (!strcasecmp(subname, w->wl_word)) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "%s\nSubcircuit expansion, unknown subcircuit %s.\n", 
                        c->line(), subname);
                    wordlist *w = new wordlist(subname, 0);
                    w->wl_next = badcalls;
                    badcalls = w;
                }
            }
            else {
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "%s\nSubcircuit expansion, empty subcircuit %s ignored.\n",
                    c->line(), sss->su_name);
            }

            // Null subccircuit, just clip out invocation and ignore.
            if (lc)
                lc->set_next(c->next());
            else
                deck = c->next();

            c->set_next(0);
            sLine::destroy(c);

            if (lc)
                c = lc->next();
            else
                c = deck;

            delete [] args;
            delete [] subname;
            delete [] instname;
            delete [] params;
            continue;
        }

        // Now we have to replace this card with the macro definition.
        //
        sLine *lcc = sLine::copy(sss->su_body);

        sParamTab *ptab = 0;
        if (SPcx.parhier() == ParHierGlobal) {
            // The "parhier" option is set to "global". 
            // Above-scope parameter assignments override
            // lower assignments.

            ptab = sParamTab::update(ptab, sss->su_params);
            if (params) {
                if (!ptab)
                    ptab = new sParamTab;
                ptab->update(params);
            }
            ptab = sParamTab::update(ptab, *parm_ptr);
        }
        else {
            // The "local" expansion mode, lower level parameter
            // assignments override upper level.

            ptab = sParamTab::update(ptab, *parm_ptr);
            ptab = sParamTab::update(ptab, sss->su_params);
            if (params) {
                if (!ptab)
                    ptab = new sParamTab;
                ptab->update(params);
            }
        }
        if (sParamTab::errString) {
            c->errcat("Error during subcircuit call parameter expansion:\n");
            c->errcat(sParamTab::errString);
            delete [] sParamTab::errString;
            sParamTab::errString = 0;
        }

        // Location of these lines changed in 3.2.18 to fix nesting.
        sg_stack_ptr++;
        lcc = expand_and_replace(lcc, &ptab, udf_ptr);
        sg_stack_ptr--;

#ifdef TIME_DBG
        double ts_tr == OP.seconds();
#endif

        // Add the functions defined in the subckt context.
        ptab->define_macros();

        int err = translate(lcc, sss->su_args, args, instname, c->line(),
            ptab);

        delete [] args;
        delete [] instname;
        delete [] params;

        ptab->undefine_macros();
        delete ptab;

#ifdef TIME_DBG
        double te_tr = OP.seconds();
        printf("translate %s: %g\n", subname, te_tr - ts_tr);
        tstart = tend;
#endif

        delete [] subname;
        if (err) {
            sLine::destroy(deck);
            deck = 0;
            sLine::destroy(lcc);
            goto cleanup;
        }

        if (sg_stack_ptr == SUB_STK_DEPTH - 1) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "%s\nSubcircuit expansion, max call depth %d exceeded.\n",
                c->line(), SUB_STK_DEPTH);
            sLine::destroy(deck);
            deck = 0;
            sLine::destroy(lcc);
            goto cleanup;
        }

        // Original location of lines mentioned in comment above
        // regarding nesting fix.

        if (!lcc) {
            sLine::destroy(deck);
            deck = 0;
            goto cleanup;
        }

        c->comment_out();
        sLine *next = c->next();
        c->set_next(lcc);
        lcc = c;
        c = next;

        // Now splice the decks together.
        if (lc)
            lc->set_next(lcc);
        else
            deck = lcc;
        while (lcc->next())
            lcc = lcc->next();
        lcc->set_next(c);
        lc = lcc;
    }

#ifdef TIME_DBG
    tend = OP.seconds();
    printf("expand and replace: %g\n", tend - tstart);
#endif

cleanup:
    sMods::destroy(sg_stack[sg_stack_ptr].mods);
    delete sg_stack[sg_stack_ptr].subs;
    sg_stack[sg_stack_ptr].clear();
    if (badcalls) {
        wordlist::destroy(badcalls);
        sLine::destroy(deck);
        deck = 0;
    }
    return (deck);
}


// Map the name of subcircuits with prefix listed in the submaps list
// to a new name.  If a mapping is done, return true.
//
bool
sScGlobal::do_submapping(sLine *c)
{
    int ninv = strlen(SPcx.invoke());
    for (string2list *s = sg_submaps; s; s = s->next) {
        if (lstring::ciprefix(s->string, c->line()+ninv)) {
            char *str;
            if (!s->value)
                str = lstring::copy(c->line()+ninv);
            else {
                int n = strlen(s->string) + ninv;
                str = new char[strlen(c->line()) - n + strlen(s->value) + 1];
                sprintf(str, "%s%s", s->value, c->line() + n);
            }
            c->set_line(str);
            delete [] str;
            return (true);
        }
    }
    return (false);
}


bool
sScGlobal::extract_cache_block(sLine **deckp, char **namep, sLine **blkp)
{
    if (namep)
        *namep = 0;
    if (blkp)
        *blkp = 0;

    bool found = false;
    sLine *next_line, *prev_line = 0;
    sLine *b0 = 0, *be = 0;
    char *bname = 0;
    for (sLine *li = *deckp; li; li = next_line) {
        next_line = li->next();

        if (lstring::cimatch(CACHE_KW, li->line())) {
            if (bname) {
                // error: can't nest cache blocks.
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s\nMissing %s line.\n",
                    li->line(), ENDCACHE_KW);
                delete [] bname;
                sLine::destroy(b0);
                return (false);
            }
            if (found) {
                // error: for now, there is one block max
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s\nExtra %s line.\n",
                    li->line(), CACHE_KW);
                delete [] bname;
                sLine::destroy(b0);
                return (false);
            }
            // extract block name
            const char *t = li->line();
            IP.advTok(&t, true);
            bname = IP.getTok(&t, true);
            if (!bname) {
                // error; name missing
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s\nMissing name in %s line.\n",
                    li->line(), CACHE_KW);
                delete [] bname;
                sLine::destroy(b0);
                return (false);
            }
        }
        else if (lstring::cimatch(ENDCACHE_KW, li->line())) {
            // deal with block
            if (namep)
                *namep = bname;
            else
                delete [] namep;
            if (blkp)
                *blkp = b0;
            else
                sLine::destroy(b0);
            b0 = be = 0;
            bname = 0;
        }
        else if (bname) {
            prev_line->set_next(li->next());
            li->set_next(0);
            if (!b0)
                b0 = be = li;
            else {
                be->set_next(li);
                be = li;
            }
            continue;
        }
        prev_line = li;
    }
    if (bname) {
        // no block end found
        GRpkgIf()->ErrPrintf(ET_ERROR, "Missing %s line.\n", ENDCACHE_KW);
        delete [] bname;
        sLine::destroy(b0);
        return (false);
    }
    return (true);
}


bool
sScGlobal::extract_subckts(sLine **deckp)
{
    sLine *last, *lc;
    for (last = *deckp, lc = 0; last; ) {
        if (SPcx.kwMatchSubend(last->line())) {
            const char *s = last->line();
            char *t = IP.getTok(&s, true);
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "%s\nSubcircuit expansion, misplaced %s card.\n",
                last->line(), t);
            delete [] t;
            return (false);
        }
        else if (SPcx.kwMatchSubstart(last->line())) {
            const char *sname = last->line();
            IP.advTok(&sname, true);
            if (!*sname) {
                sname = last->line();
                char *t = IP.getTok(&sname, true);
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "%s\nSubcircuit expansion, no name given for %s.\n",
                    last->line(), t);
                delete [] t;
                return (false);
            }
            if (!last->next()) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "%s\nSubcircuit exapnsion, missing %s or .eom card.\n",
                     last->line(), SPcx.sbend());
                return (false);
            }
            sLine *lcc = 0, *c;
            int nest;
            for (nest = 0, c = last->next(); c; lcc = c, c = c->next()) {
                if (SPcx.kwMatchSubend(c->line())) {
                    if (!nest)
                        break;
                    else {
                        nest--;
                        continue;
                    }
                }
                else if (SPcx.kwMatchSubstart(c->line()))
                    nest++;
            }
            if (!c) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "%s\nSubcircuit exapnsion, missing %s or .eom card.\n",
                     last->line(), SPcx.sbend());
                return (false);
            }
            if (lcc)
                lcc->set_next(0);
            else
                last->set_next(0);

            if (lc)
                lc->set_next(c->next());
            else
                *deckp = c->next();

            addsub(last);
            last->set_next(0);
            sLine::destroy(last);
            last = c->next();
            c->set_next(0);
            sLine::destroy(c);
        }
        else {
            lc = last;
            last = last->next();
        }
    }
    return (true);
}


void
sScGlobal::addmod(const char *name, const char *trns)
{
    sg_stack[sg_stack_ptr].mods =
        new sMods(lstring::copy(name), lstring::copy(trns),
            sg_stack[sg_stack_ptr].mods);
}


void
sScGlobal::addsub(sLine *def)
{
    if (!sg_stack[sg_stack_ptr].subs)
        sg_stack[sg_stack_ptr].subs = new sSubcTab;
    sSubc *sss = new sSubc(def);
    sg_stack[sg_stack_ptr].subs->add(sss->su_name, sss);
}


sSubc *
sScGlobal::findsub(const char *subname, int ix)
{
    if (ix < 0) {
        if (sg_stack_ptr >= 0)
            return ((sSubc*)sHtab::get(sg_stack[sg_stack_ptr].subs, subname));
        return (0);
    }
    for (int j = ix; j >= 0; j--) {
        sSubc *sss = (sSubc*)sHtab::get(sg_stack[j].subs, subname);
        if (sss)
            return (sss);
    }
    return (0);
}


namespace {
    // When matching MOS models, ignore the bin extension in the real
    // name, unless both names have one.
    //
    bool modname_eq(const char *ncall, const char *nreal, char key)
    {
        if (!ncall || !nreal)
            return (false);
        if (key == 'm' || key == 'M') {
            // A MOS device.
            const char *s = ncall;
            const char *t = nreal;
            while (*s && *t) {
                if ((isupper(*s) ? tolower(*s) : *s) !=
                        (isupper(*t) ? tolower(*t) : *t))
                    return (false);
                s++;
                t++;
            }
            return (!*s && (!*t || *t == '.'));
        }
        return (lstring::cieq(ncall, nreal));
    }
}


// This handles parameter expansion of lines not in subcircuit
// definitions.
//
void
sScGlobal::param_expand(sLine *deck, sParamTab *ptab)
{
    if (!deck || !deck->line() || !ptab)
        return;
    const char *s = deck->line();
    while (isspace(*s))
        s++;
    if (!*s || *s == '*' || *s == '#' || (*s == '\\' && *(s+1) == '$'))
        return;

    if (SPcx.kwMatchSubstart(s)) {
        char *str = lstring::copy(deck->line());
        ptab->param_subst_subckt(&str);
        if (str && strcmp(deck->line(), str)) {
            deck->set_line(str);
            delete [] str;
        }
        return;
    }
    if (SPcx.kwMatchModel(s)) {
        char *str = lstring::copy(deck->line());
        ptab->param_subst_model(&str);
        if (str && strcmp(deck->line(), str)) {
            deck->set_line(str);
            delete [] str;
        }
        return;
    }
    if (lstring::cimatch(PARAM_KW, s)) {
        char *str = lstring::copy(deck->line());
        ptab->param_subst_param(&str);
        if (str && strcmp(deck->line(), str)) {
            deck->set_line(str);
            delete [] str;
        }
        return;
    }
    if (lstring::cimatch(MEAS_KW, s) || lstring::cimatch(MEASURE_KW, s)) {
        char *str = lstring::copy(deck->line());
        ptab->param_subst_measure(&str);
        if (str && strcmp(deck->line(), str)) {
            deck->set_line(str);
            delete [] str;
        }
        return;
    }
    if (*s == '.') {
        // Don't expand unsupported control cards.
        char *tok = IP.getTok(&s, true);
        for (char *t = tok+1; *t; t++) {
            if (isupper(*t))
                *t = tolower(*t);
        }
        int ret = IP.checkDotCard(tok);
        delete [] tok;
        if (ret)
            return;

        char *str = lstring::copy(deck->line());
        ptab->param_subst_all(&str);
        deck->set_line(str);
        delete [] str;
        return;
    }

    // Device or subcircuit calls only.  These names are required to
    // start with an alpha char.
    if (!isalpha(*s))
        return;

    // Grab the name, no parameter expanding of names.
    char *devname = IP.getTok(&s, true);
    if (!devname)
        return;
    char ch = *devname;

    sLstr lstr;
    lstr.add(devname);

    // Now figure out how many nodes and device references (for
    // dependent sources) to look for.  This is tricky because
    // devices may have optional nodes.

    int min_nodes = 0;
    int max_nodes = 0;
    int devs = 0;
    int has_mod = false;
    bool is_sc_call = false;

    if (SPcx.kwMatchSubinvoke(devname)) {
        // This is a subcircuit call.
        is_sc_call = true;

        // Just ignore errors here, they will be detected when we do
        // the expansion.

        char *tmpline = new char[strlen(devname) + strlen(s) + 2];
        char *e = lstring::stpcpy(tmpline, devname);
        *e++ = ' ';
        strcpy(e, s);

        char *aargs = 0, *subname = 0, *insname = 0, *params = 0;
        parse_call(tmpline, &aargs, &subname, &insname, &params);
        delete [] aargs;
        delete [] insname;
        delete [] params;
        if (subname) {
            sSubc *sss = findsub(subname, sg_stack_ptr);
            if (sss) {
                min_nodes = sss->su_numargs;
                max_nodes = min_nodes;
            }
            delete [] subname;
        }
        delete [] tmpline;
    }
    else
        IP.numRefs(devname, &min_nodes, &max_nodes, &devs, &has_mod);

    delete [] devname;

    // Get the "for sure" nodes.

    int nnodes = min_nodes;
    while (nnodes-- > 0) {
        char *nodename = IP.getTok(&s, true);
        if (SPcx.pexnodes()) {
            if (ptab && nodename)
                ptab->param_subst_all(&nodename);
        }
        if (!nodename) {
            // Error, too few nodes.  Just continue and let the
            // parser handle this.

            min_nodes = max_nodes = 0;
            devs = 0;
            break;
        }

        // Dependent source hack
        if (strchr("eEgG", ch)) {
            if (lstring::cieq(nodename, "poly") ||
                    lstring::cieq(nodename, "function") ||
                    lstring::cieq(nodename, "cur") ||
                    lstring::cieq(nodename, "vol")) {
                lstr.add_c(' ');
                lstr.add(nodename);
                delete [] nodename;
                min_nodes = max_nodes = 0;
                devs = 0;
                break;
            }
        }

        lstr.add_c(' ');
        lstr.add(nodename);
        delete [] nodename;
    }    

    // Assert that if a device has device references, then it
    // must have a fixed number of nodes.

    if (devs > 0 && min_nodes != max_nodes) {
        GRpkgIf()->ErrPrintf(ET_INTERR,
            "device key %c has device references and optional nodes.\n", ch);
        max_nodes = min_nodes;
    }

    // Now the device references if any.

    int ndevs = devs;
    while (ndevs-- > 0) {
        char *refname = IP.getTok(&s, true);
        if (!refname) {
            // Error, too few devices.  Just continue and let the
            // parser handle this.

            min_nodes = max_nodes = 0;
            break;
        }

        // Dependent source hack.
        if (strchr("fFhH", ch)) {
            if (lstring::cieq(refname, "poly") ||
                    lstring::cieq(refname, "function") ||
                    lstring::cieq(refname, "cur") ||
                    lstring::cieq(refname, "vol")) {
                lstr.add_c(' ');
                lstr.add(refname);
                delete [] refname;
                break;
            }
        }

        lstr.add_c(' ');
        lstr.add(refname);
        delete [] refname;
    }

    // Assert that if a device has optional nodes, then it
    // requires a model.  This is necessary to tell where the
    // node list ends by recognizing the model name.

    if (min_nodes != max_nodes && !has_mod) {
        GRpkgIf()->ErrPrintf(ET_INTERR,
            "device key %c has optional nodes and no model.\n", ch);
        // Optional nodes won't be translated.
        max_nodes = min_nodes;
    }

    // Get the optional nodes.

    bool gotmod = false;
    nnodes = max_nodes - min_nodes;
    while (nnodes-- > 0) {
        char *name = IP.getTok(&s, true);
        if (!name)
            break;
        // Model names are param expanded, nodes are not.
        char *pexname = lstring::copy(name);
        if (ptab && pexname)
            ptab->param_subst_all(&pexname);

        if (pexname) {
            for (sMods *m = sg_stack[0].mods; m; m = m->next) {
                if (lstring::cieq(pexname, m->name_orig)) {
                    delete [] name;
                    name = lstring::copy(m->name_trans);
                    gotmod = true;
                    break;
                }
            }
            if (!gotmod) {
                for (sMods *m = sg_stack[0].mods; m; m = m->next) {
                    if (modname_eq(pexname, m->name_trans, ch)) {
                        delete [] name;
                        name = pexname;
                        pexname = 0;
                        gotmod = true;
                        break;
                    }
                }
            }
            if (SPcx.pexnodes() && !gotmod) {
                delete [] name;
                name = pexname;
                pexname = 0;
            }
            delete [] pexname;
        }
        lstr.add_c(' ');
        lstr.add(name);
        delete [] name;
        if (gotmod)
            break;
    }

    // Look for the model if we haven't seen it yet.

    if (has_mod && !gotmod) {
        const char *bkp = s;
        char *name = IP.getTok(&s, true);
        if (name) {
            char *pexname = lstring::copy(name);
            if (ptab && pexname)
                ptab->param_subst_all(&pexname);

            if (pexname) {
                for (sMods *m = sg_stack[0].mods; m; m = m->next) {
                    if (lstring::cieq(pexname, m->name_orig)) {
                        delete [] name;
                        name = lstring::copy(m->name_trans);
                        gotmod = true;
                        break;
                    }
                }
                if (!gotmod) {
                    for (sMods *m = sg_stack[0].mods; m; m = m->next) {
                        if (modname_eq(pexname, m->name_trans, ch)) {
                            delete [] name;
                            name = pexname;
                            pexname = 0;
                            gotmod = true;
                            break;
                        }
                    }
                }
                delete [] pexname;
            }
            if (gotmod) {
                lstr.add_c(' ');
                lstr.add(name);
            }
            else {
                // Push it back and handle with parameters.
                s = bkp;
            }
            delete [] name;
        }
    }

    if (is_sc_call) {
        // Parameter substitute the name.
        char *name = IP.getTok(&s, true);
        if (ptab && name)
            ptab->param_subst_all(&name);
        if (name) {
            lstr.add_c(' ');
            lstr.add(name);
        }
    }
    
    // The rest of the line contains parameters and flags.

    // Parameter expand the rest of the line.  Do this after the
    // subcircuit substitutions, so that node voltage references
    // in single-quoted expressions have been transformed.

    while (isspace(*s))
        s++;
    if (*s) {
        const char *srcs = "aefghivAEFGHIV";
        const char *rlc = "rlcRLC";
        char *e = lstring::copy(s);
        if (ptab && e) {
            if (is_sc_call)
                ptab->param_subst_defn_list(&e, false);
            else if (strchr(srcs, ch))
                ptab->param_subst_all(&e);
            else if (has_mod && gotmod)
                ptab->param_subst_defn_list(&e, true);
            else if (strchr(rlc, ch))
                ptab->param_subst_all(&e);
            else
                ptab->param_subst_defn_list(&e, true);
        }
        if (e && *e) {
            lstr.add_c(' ');
            lstr.add(e);
        }
        delete [] e;
    }

    deck->set_line(lstr.string());
}


// Translate all of the device names and node names in the deck. 
// Nodes are translated unless they are in the formal list, in which
// case they are replaced with the corresponding entry in the actual
// list.  We also skip translating global node names of which there
// is at least one: the "0" ground node.
//
// deck     The subcircuit lines
// instr    The subcircuit call line being expanded
//
bool
sScGlobal::translate(sLine *deck, const char *formal, const char *actual,
    const char *instname, const char *instr, sParamTab *ptab)
{
    // First pass, translate the model cards.  We need the
    // translations in the second pass.

    for (sLine *c = deck; c; c = c->next()) {
        const char *s = c->line();
        while (isspace(*s))
            s++;
        if (SPcx.kwMatchModel(s)) {

#ifdef TIME_DBG
            double tstart = OP.seconds();
#endif
            sLstr lstr;
            char *mtok = IP.getTok(&s, true);  // ".model"
            lstr.add(mtok);
            delete [] mtok;

            char *mname = IP.getTok(&s, true); // model name
            if (!mname)
                continue;
            lstr.add_c(' ');
            if (SPcx.catmode() == SUBC_CATMODE_WR) {
                lstr.add(mname);
                lstr.add_c(SPcx.catchar());
                lstr.add(instname);
            }
            else {
                lstr.add(instname);
                lstr.add_c(SPcx.catchar());
                lstr.add(mname);
            }

            mtok = IP.getTok(&s, true); // model type name
            lstr.add_c(' ');
            lstr.add(mtok);
            delete [] mtok;

            char *e = lstring::copy(s);
            if (ptab && e)
                ptab->param_subst_defn_list(&e, false);
            if (e) {
                lstr.add_c(' ');
                lstr.add(e);
                delete [] e;
            }

            c->set_line(lstr.string());

            const char *t = c->line();
            IP.advTok(&t, true);
            char *trns = IP.getTok(&t, true);
            addmod(mname, trns);
            delete [] mname;
            delete [] trns;
#ifdef TIME_DBG
            double tend = OP.seconds();
            printf("%g %s\n", tend - tstart, c->line());
#endif
        }
    }

    // Set up the formal/actual translation table.

    sTabc *table = settrans(formal, actual, instr);
    if (!table)
        return (true);

    // Second pass, process the device calls.

    for (sLine *c = deck; c; c = c->next()) {

#ifdef TIME_DBG
        double tstart = OP.seconds();
#endif
        const char *s = c->line();
        while (isspace(*s))
            s++;

        // Device or subcircuit calls only.  These names are required
        // to start with an alpha char.

        if (!isalpha(*s))
            continue;

        // Grab the name, no parameter expanding of names.
        char *devname = IP.getTok(&s, true);
        if (!devname)
            continue;
        char ch = *devname;

        sLstr lstr;

        // Translate the name.
        if (SPcx.catmode() == SUBC_CATMODE_WR) {
            lstr.add(devname);
            lstr.add_c(SPcx.catchar());
            lstr.add(instname);
        }
        else {
            lstr.add_c(ch);
            lstr.add_c(SPcx.catchar());
            lstr.add(instname);
            if (devname[1] != SPcx.catchar())
                lstr.add_c(SPcx.catchar());
            lstr.add(devname+1);
        }

        // Now figure out how many nodes and device references (for
        // dependent sources) to look for.  This is tricky because
        // devices may have optional nodes.

        int min_nodes = 0;
        int max_nodes = 0;
        int devs = 0;
        int has_mod = false;
        bool is_sc_call = false;

        if (SPcx.kwMatchSubinvoke(devname)) {
            // This is a subcircuit call.
            is_sc_call = true;

            // Just ignore errors here, they will be detected when we
            // do the expansion.

            char *tmpline = new char[strlen(devname) + strlen(s) + 2];
            char *e = lstring::stpcpy(tmpline, devname);
            *e++ = ' ';
            strcpy(e, s);

            char *aargs = 0, *subname = 0, *insname = 0, *params = 0;
            parse_call(tmpline, &aargs, &subname, &insname, &params);
            delete [] aargs;
            delete [] insname;
            delete [] params;
            if (subname) {
                sSubc *sss = findsub(subname, sg_stack_ptr);
                if (sss) {
                    min_nodes = sss->su_numargs;
                    max_nodes = min_nodes;
                }
                delete [] subname;
            }
            delete [] tmpline;
        }
        else
            IP.numRefs(devname, &min_nodes, &max_nodes, &devs, &has_mod);

        delete [] devname;

        // Get the "for sure" nodes.

        int nnodes = min_nodes;
        while (nnodes-- > 0) {
            char *nodename = IP.getTok(&s, true);
            if (SPcx.pexnodes()) {
                if (ptab && nodename)
                    ptab->param_subst_all(&nodename);
            }
            if (!nodename) {
                // Error, too few nodes.  Just continue and let the
                // parser handle this.

                min_nodes = max_nodes = 0;
                devs = 0;
                break;
            }

            // If name is in the global list, keep it untranslated.
            if (is_global(nodename)) {
                lstr.add_c(' ');
                lstr.add(nodename);
                delete [] nodename;
                continue;
            }

            // Dependent source hack
            if (strchr("eEgG", ch)) {
                if (lstring::cieq(nodename, "poly")) {
                    delete [] nodename;
                    char *t = polytrans(&s, false, instname, table);
                    if (ptab && t)
                        ptab->param_subst_all(&t);
                    if (t) {
                        lstr.add_c(' ');
                        lstr.add(t);
                        delete [] t;
                    }
                    min_nodes = max_nodes = 0;
                    devs = 0;
                    break;
                }
                if (lstring::cieq(nodename, "function") ||
                        lstring::cieq(nodename, "cur") ||
                        lstring::cieq(nodename, "vol")) {
                    lstr.add_c(' ');
                    lstr.add(nodename);
                    delete [] nodename;
                    min_nodes = max_nodes = 0;
                    devs = 0;
                    break;
                }
            }

            const char *t = gettrans(nodename, table);
            lstr.add_c(' ');
            if (t)
                lstr.add(t);
            else {
                if (SPcx.catmode() == SUBC_CATMODE_WR) {
                    lstr.add(nodename);
                    lstr.add_c(SPcx.catchar());
                    lstr.add(instname);
                }
                else {
                    lstr.add(instname);
                    lstr.add_c(SPcx.catchar());
                    lstr.add(nodename);
                }
            }
            delete [] nodename;
        }    

        // Assert that if a device has device references, then it
        // must have a fixed number of nodes.

        if (devs > 0 && min_nodes != max_nodes) {
            GRpkgIf()->ErrPrintf(ET_INTERR,
                "device key %c has device references and optional nodes.\n",
                ch);
            max_nodes = min_nodes;
        }

        // Now the device references if any.

        int ndevs = devs;
        while (ndevs-- > 0) {
            char *refname = IP.getTok(&s, true);
            if (!refname) {
                // Error, too few devices.  Just continue and let the
                // parser handle this.

                min_nodes = max_nodes = 0;
                break;
            }

            // Dependent source hack.
            if (strchr("fFhH", ch)) {
                if (lstring::cieq(refname, "poly")) {
                    delete [] refname;
                    char *t = polytrans(&s, true, instname, table);
                    if (ptab && t)
                        ptab->param_subst_all(&t);
                    if (t) {
                        lstr.add_c(' ');
                        lstr.add(t);
                        delete [] t;
                    }
                    break;
                }
                if (lstring::cieq(refname, "function") ||
                        lstring::cieq(refname, "cur") ||
                        lstring::cieq(refname, "vol")) {
                    lstr.add_c(' ');
                    lstr.add(refname);
                    delete [] refname;
                    break;
                }
            }

            lstr.add_c(' ');
            if (SPcx.catmode() == SUBC_CATMODE_WR) {
                lstr.add(refname);
                lstr.add_c(SPcx.catchar());
                lstr.add(instname);
            }
            else {
                lstr.add_c(*refname);
                lstr.add_c(SPcx.catchar());
                lstr.add(instname);
                if (refname[1] != SPcx.catchar())
                    lstr.add_c(SPcx.catchar());
                lstr.add(refname + 1);
            }
            delete [] refname;
        }

        // Assert that if a device has optional nodes, then it
        // requires a model.  This is necessary to tell where the
        // node list ends by recognizing the model name.

        if (min_nodes != max_nodes && !has_mod) {
            GRpkgIf()->ErrPrintf(ET_INTERR,
                "device key %c has optional nodes and no model.\n", ch);
            // Optional nodes won't be translated.
            max_nodes = min_nodes;
        }

        // Get the optional nodes.

        bool gotmod = false;
        nnodes = max_nodes - min_nodes;
        while (nnodes-- > 0) {
            char *name = IP.getTok(&s, true);
            if (!name)
                break;
            // Model names are param expanded, nodes are not.
            char *pexname = lstring::copy(name);
            if (ptab && pexname)
                ptab->param_subst_all(&pexname);

            if (pexname) {
                for (sMods *m = sg_stack[sg_stack_ptr].mods; m; m = m->next) {
                    if (lstring::cieq(pexname, m->name_orig)) {
                        delete [] name;
                        name = lstring::copy(m->name_trans);
                        gotmod = true;
                        break;
                    }
                }
                if (!gotmod) {
                    for (int j = sg_stack_ptr; j >= 0; j--) {
                        for (sMods *m = sg_stack[j].mods; m; m = m->next) {
                            if (modname_eq(pexname, m->name_trans, ch)) {
                                delete [] name;
                                name = pexname;
                                pexname = 0;
                                gotmod = true;
                                break;
                            }
                        }
                        if (gotmod)
                            break;
                    }
                }
                if (SPcx.pexnodes() && !gotmod) {
                    delete [] name;
                    name = pexname;
                    pexname = 0;
                }
                delete [] pexname;
            }
            if (gotmod) {
                // The model name.
                lstr.add_c(' ');
                lstr.add(name);
                delete [] name;
                break;
            }

            // Not a model, must be a node.

            // If name is in the global list, keep it untranslated.
            if (is_global(name)) {
                lstr.add_c(' ');
                lstr.add(name);
                delete [] name;
                continue;
            }

            // We know that the device is not a dependent source.

            const char *t = gettrans(name, table);
            lstr.add_c(' ');
            if (t)
                lstr.add(t);
            else {
                if (SPcx.catmode() == SUBC_CATMODE_WR) {
                    lstr.add(name);
                    lstr.add_c(SPcx.catchar());
                    lstr.add(instname);
                }
                else {
                    lstr.add(instname);
                    lstr.add_c(SPcx.catchar());
                    lstr.add(name);
                }
            }
            delete [] name;
        }

        // Look for the model if we haven't seen it yet.

        if (has_mod && !gotmod) {
            const char *bkp = s;
            char *name = IP.getTok(&s, true);
            if (!name)
                break;
            char *pexname = lstring::copy(name);
            if (ptab && pexname)
                ptab->param_subst_all(&pexname);

            if (pexname) {
                for (sMods *m = sg_stack[sg_stack_ptr].mods; m; m = m->next) {
                    if (lstring::cieq(pexname, m->name_orig)) {
                        delete [] name;
                        name = lstring::copy(m->name_trans);
                        gotmod = true;
                        break;
                    }
                }
                if (!gotmod) {
                    for (int j = sg_stack_ptr; j >= 0; j--) {
                        for (sMods *m = sg_stack[j].mods; m; m = m->next) {
                            if (modname_eq(pexname, m->name_trans, ch)) {
                                delete [] name;
                                name = pexname;
                                pexname = 0;
                                gotmod = true;
                                break;
                            }
                        }
                        if (gotmod)
                            break;
                    }
                }
                delete [] pexname;
            }
            if (gotmod) {
                lstr.add_c(' ');
                lstr.add(name);
            }
            else {
                // Push it back and handle in finishLine.
                s = bkp;
            }
            delete [] name;
        }

        if (has_mod && !gotmod && min_nodes != max_nodes) {
            // We never found the model.  This is fatal with optional
            // nodes.

            GRpkgIf()->ErrPrintf(ET_ERROR,
                "%s\nSubcircuit expansion, unknown model.\n", c->line());

            delete [] table;
            return (true);
        }

        if (is_sc_call) {
            // Parameter substitute the name.
            char *name = IP.getTok(&s, true);
            if (ptab && name)
                ptab->param_subst_all(&name);
            if (name) {
                lstr.add_c(' ');
                lstr.add(name);
            }
        }

        // The rest of the line contains parameters and flags.

#ifdef TIME_DBG
        double tend = OP.seconds();
        printf("%g %s\n", tend - tstart, c->line());
        tstart = tend;
#endif
        // Parameter expand the rest of the line.  Do this after the
        // subcircuit substitutions, so that node voltage references
        // in single-quoted expressions have been transformed.

        while (isspace(*s))
            s++;
        if (*s) {
            const char *srcs = "aefghivAEFGHIV";
            const char *rlc = "rlcRLC";
            char *e = lstring::copy(s);
            if (ptab && e) {
                if (is_sc_call)
                    ptab->param_subst_defn_list(&e, false);
                else if (strchr(srcs, ch))
                    ptab->param_subst_all(&e);
                else if (has_mod && gotmod)
                    ptab->param_subst_defn_list(&e, true);
                else if (strchr(rlc, ch))
                    ptab->param_subst_all(&e);
                else
                    ptab->param_subst_defn_list(&e, true);
            }
            if (e && *e) {
                lstr.add_c(' ');
                finishLine(&lstr, e, instname, table);
            }
            delete [] e;
        }

        c->set_line(lstr.string());
#ifdef TIME_DBG
        tend = OP.seconds();
        printf("%g %s\n", tend - tstart, c->line());
#endif
    }
    delete [] table;
    return (false);
}


bool
sScGlobal::cache_add(const char *name, sLine **linep, const sParamTab *ptab,
    const cUdf *udfdb)
{
    if (!name || SPcache.inCache(name))
        return (true);

    sSubcTab *st = sg_stack[sg_stack_ptr].subs;
    sg_stack[sg_stack_ptr].subs = 0;
    bool ret = extract_subckts(linep);
    sSubcTab *subs = sg_stack[sg_stack_ptr].subs;
    sg_stack[sg_stack_ptr].subs = st;
    if (!ret) {
        delete subs;
        return (false);
    }

    // The linep now has subcircuit definitions removed.  Pull out
    // (top-level) model definitions, if any.

    sLine *lprv = 0, *lnxt;
    sLine *lm0 = 0, *lme = 0;
    for (sLine *l = *linep; l; l = lnxt) {
        lnxt = l->next();
        if (SPcx.kwMatchModel(l->line())) {
            if (lprv)
                lprv->set_next(l->next());
            else
                *linep = l->next();
            l->set_next(0);
            if (!lm0)
                lm0 = lme = l;
            else {
                lme->set_next(l);
                lme = l;
            }
            continue;
        }
        lprv = l;
    }

    // Process the models into a separate cache table.
    sModTab *tmp_tab = IP.swapModTab(0);
    for (sLine *l = lm0; l; l = l->next()) {
        IP.parseMod(l);
        if (l->error()) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s\nSubcircuit expansion, %s.\n",
                l->line(), l->error());
            l->clear_error();
        }
    }
    sLine::destroy(lm0);
    sModTab *new_tab = IP.swapModTab(tmp_tab);

    SPcache.add(name, subs, ptab, new_tab, udfdb);
    return (true);
}


namespace {
    // Advance to next token, delimiters are space and comma, '=' is a
    // special token.
    //
    void atok(const char **s)
    {
        if (s == 0 || *s == 0)
            return;
        while (isspace(**s) || **s == ',')
            (*s)++;
        if (!**s)
            return;
        if (**s == '=')
            (*s)++;
        else
            while (**s && !isspace(**s) && **s != '=')
                (*s)++;
        while (isspace(**s) || **s == ',')
            (*s)++;
    }
}


// Parse the subcircuit instantiation line, return the argument list,
// subcircuit name, instance name, and parameter text.  Returned strings
// are new.
//
void
sScGlobal::parse_call(const char *str, char **aargs, char **subname,
    char **instname, char **params)
{
    *aargs = 0;
    *subname = 0;
    *params = 0;
    const char *t = IP.getTok(&str, true);
    const char *tn = t + strlen(SPcx.invoke());
    if (SPcx.catmode() == SUBC_CATMODE_WR) {
        // WRspice mode, keep the invoke part if it is 'x' or 'X',
        // otherwise replace with 'x'.
        if (tn - t == 1 && (*t == 'x' || *t == 'X'))
            *instname = lstring::copy(t);
        else {
            *instname = new char[strlen(tn) + 2];
            (*instname)[0] = 'x';
            strcpy((*instname)+1, tn);
        }
    }
    else {
        // Spice3 mode, strip the invoke part, and leading catchar if any.
        if (*tn == SPcx.catchar())
            tn++;
        *instname = lstring::copy(tn);
    }
    delete [] t;

    const char *args = str;
    t = tn = 0;
    int nargs = 0;
    while (*str) {
        t = str;
        atok(&str);
        tn = str;
        if (*tn == '=')
            break;
        nargs++;
    }
    const char *pars = 0;
    if (tn && *tn == '=') {
        pars = t;
        t--;
        while ((isspace(*t) || *t == ',') && t > args)
            t--;
        if (t > args)
            t++;
        else {
            t = 0;
            args = 0;
        }
    }
    if (pars)
        *params = lstring::copy(pars);
    if (args) {
        if (!pars)
            t = args + strlen(args);
        char *targs = new char[(t - args) + 1];
        strncpy(targs, args, (t - args));
        targs[(t - args)] = 0;

        char *tt = targs + (t - args);
        if (!pars)
           tt--;
        while (*tt == ' ' || *tt == '\t')
            *tt-- = '\0';
        while (*tt != ' ' && *tt != '\t' && tt >= targs)
            tt--;
        tt++;
        *subname = lstring::copy(tt);
        if (tt > targs) {
            tt--;
            while ((*tt == ' ' || *tt == '\t' || *tt == ',') && tt >= targs)
                *tt-- = '\0';
        }
        else
            *tt = 0;
        *aargs = targs;
    }
    else 
        *aargs = lstring::copy("");
}


void
sScGlobal::finishLine(sLstr *lstr, const char *src, const char *instname,
    sTabc *table)
{
    bool lastwasalpha = false;
    while (*src) {
        if (*src == '@') {
            const char *s = src+1;
            while (*s && !isspace(*s) && *s != '[')
                s++;
            if (*s == '[') {
                // replace device in @device[param]
                if (SPcx.catmode() == SUBC_CATMODE_WR) {
                    while (*src != '[')
                        lstr->add_c(*src++);
                    lstr->add_c(SPcx.catchar());
                    lstr->add(instname);
                }
                else {
                    // Xstuff -> X:subckt:stuff
                    lstr->add_c(*src++);
                    lstr->add_c(*src++);
                    lstr->add_c(SPcx.catchar());
                    lstr->add(instname);
                    if (*src != SPcx.catchar())
                        lstr->add_c(SPcx.catchar());
                    while (*src != '[')
                        lstr->add_c(*src++);
                }
            }
            lstr->add_c(*src++);
            lastwasalpha = false;
            continue;
        }
        if ((*src == 'v' || *src == 'V' || *src == 'i' || *src == 'I') &&
                !lastwasalpha) {
            // recognize v vm vp vr vi vdb, same for i
            const char *s = src+1;
            char ch = isupper(*s) ? tolower(*s) : *s;
            if (ch == 'm' || ch == 'p' || ch == 'r' || ch == 'i')
                s++;
            else if (ch == 'd' && (s[1] == 'b' || s[1] == 'B'))
                s += 2;

            while (isspace(*s))
                s++;
            if (!*s || (*s != '(')) {
                lastwasalpha = isalpha(*src);
                lstr->add_c(*src++);
                continue;
            }
            lastwasalpha = false;
            char which = *src++;
            lstr->add_c(which);
            for ( ; src <= s; src++) {
                if (!isspace(*src))
                    lstr->add_c(*src);
            }
            while (isspace(*src))
                src++;

            sLstr tstr;
            int i;
            for (i = 0;
                    *src && !isspace(*src) && *src != ',' && (*src != ')');
                    i++)
                tstr.add_c(*src++);

            if (which == 'v' || which == 'V')
                s = gettrans(tstr.string(), table);
            else
                s = 0;

            if (s)
                lstr->add(s);
            else {
                if (SPcx.catmode() == SUBC_CATMODE_WR) {
                    lstr->add(tstr.string());
                    lstr->add_c(SPcx.catchar());
                    lstr->add(instname);
                }
                else {
                    if (which == 'v' || which == 'V') {
                        // node name
                        lstr->add(instname);
                        lstr->add_c(SPcx.catchar());
                        lstr->add(tstr.string());
                    }
                    else {
                        // device name
                        lstr->add_c(tstr.string()[0]);
                        lstr->add_c(SPcx.catchar());
                        lstr->add(instname);
                        if (tstr.string()[1] != SPcx.catchar())
                            lstr->add_c(SPcx.catchar());
                        lstr->add(tstr.string() + 1);
                    }
                }
            }
            tstr.free();

            // translate the reference node, as in the "2" in "v(4, 2)"

            if (which == 'v' || which == 'V') {
                while (*src && (isspace(*src) || *src == ','))
                    src++;
                if (*src && *src != ')') {
                    for (i = 0; *src && !isspace(*src) && (*src != ')'); i++)
                        tstr.add_c(*src++);
                    s = gettrans(tstr.string(), table);
                    lstr->add_c(',');
                    if (s)
                        lstr->add(s);
                    else {
                        if (SPcx.catmode() == SUBC_CATMODE_WR) {
                            lstr->add(tstr.string());
                            lstr->add_c(SPcx.catchar());
                            lstr->add(instname);
                        }
                        else {
                            lstr->add(instname);
                            lstr->add_c(SPcx.catchar());
                            lstr->add(tstr.string());
                        }
                    }
                    tstr.free();
                }
            }
        }
        else {
            lastwasalpha = isalpha(*src);
            lstr->add_c(*src++);
        }
    }
}


sTabc *
sScGlobal::settrans(const char *formal, const char *actual, const char *instr)
{ 
    const char *t = formal;
    int i = 0;
    while (*t) {
        IP.advTok(&t, true);
        i++;
    }
    i++;  // last entry 0, 0
    sTabc *table = new sTabc[i];

    for (i = 0; ; i++) {
        table[i].set_oldw(IP.getTok(&formal, true));
        table[i].set_neww(IP.getTok(&actual, true));

        if (table[i].neww() && !table[i].oldw()) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "%s\nSubcircuit expansion, too many arguments.\n", instr);
            delete [] table;
            return (0);    // Too few formal / too many actual
        }
        else if (table[i].oldw() && !table[i].neww()) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "%s\nSubcircuit expansion, too few arguments.\n", instr);
            delete [] table;
            return (0);    // Too few actual / too many formal
        }
        if (!table[i].oldw())
            break;
    }
    return (table);
}


// Handle the POLY(N) construct.
//
char *
sScGlobal::polytrans(const char **line, bool cc, const char *instname,
    sTabc *table)
{
    int error;
    int ndims = (int)IP.getFloat(line, &error, true);
    if (error)
        return (0);
    if (ndims < 0 || ndims > 20)
        return (0);

    sLstr lstr;
    lstr.add("poly(");
    lstr.add_i(ndims);
    lstr.add_c(')');

    for (int i = 0; i < ndims; i++) {
        if (cc) {
            // ccvs, cccs
            char *devn = IP.getTok(line, true);
            if (!devn)
                return (0);
            lstr.add_c(' ');
            if (SPcx.catmode() == SUBC_CATMODE_WR) {
                lstr.add(devn);
                lstr.add_c(SPcx.catchar());
                lstr.add(instname);
            }
            else {
                lstr.add_c(*devn);
                lstr.add_c(SPcx.catchar());
                lstr.add(instname);
                if (devn[1] != SPcx.catchar())
                    lstr.add_c(SPcx.catchar());
                lstr.add(devn + 1);
            }
            delete [] devn;
        }
        else {
            char *node = IP.getTok(line, true);
            if (!node)
                return (0);
            lstr.add_c(' ');
            lstr.add_c('(');
            const char *t = gettrans(node, table);
            if (t)
                lstr.add(t);
            else if (SPcx.catmode() == SUBC_CATMODE_WR) {
                lstr.add(node);
                lstr.add_c(SPcx.catchar());
                lstr.add(instname);
            }
            else {
                lstr.add(instname);
                lstr.add_c(SPcx.catchar());
                lstr.add(node);
            }
            delete [] node;
            lstr.add_c(',');
            node = IP.getTok(line, true);
            if (!node)
                return (0);
            t = gettrans(node, table);
            if (t)
                lstr.add(t);
            else if (SPcx.catmode() == SUBC_CATMODE_WR) {
                lstr.add(node);
                lstr.add_c(SPcx.catchar());
                lstr.add(instname);
            }
            else {
                lstr.add(instname);
                lstr.add_c(SPcx.catchar());
                lstr.add(node);
            }
            lstr.add_c(')');
            delete [] node;
        }
    }
    lstr.add_c(' ');

    for ( ; **line != '\0'; (*line)++) {
            if (**line == ' ') continue;
            if (**line == '\t') continue;
            if (**line == ',') continue;
            if (**line == ')') continue;
        break;
    }
    return (lstr.string_trim());
}
// End of sScGlobal functions.


sSubc::sSubc(sLine *def)
{
    const char *str = def->line();
    su_body = def->next();
    IP.advTok(&str, true);  // pass subinvoke
    su_name = IP.getTok(&str, true);
    const char *args = str, *t = 0, *tn = 0;
    int nargs = 0;
    while (*str) {
        t = str;
        atok(&str);
        tn = str;
        if (*tn == '=')
            break;
        nargs++;
    }
    const char *pars = 0;
    if (tn && *tn == '=') {
        pars = t;
        t--;
        while ((isspace(*t) || *t == ',') && t >= args)
            t--;
        if (t >= args)
            t++;
        else
            args = 0;
    }

    // First extract params defined in .subckt line.
    su_params = 0;
    if (pars) {
        su_params = sParamTab::extract_params(su_params, pars);
        if (sParamTab::errString) {
            def->errcat(sParamTab::errString);
            delete [] sParamTab::errString;
            sParamTab::errString = 0;
        }
    }

    // Next extract params found in the subckt body.  The .param lines
    // are commented to prevent further use.  These override
    // definitions in the .subckt line.

    for (sLine *li = def; li; li = li->next()) {
        if (lstring::cimatch(PARAM_KW, li->line())) {
            su_params = sParamTab::extract_params(su_params, li->line());
            li->comment_out();
            if (sParamTab::errString) {
                li->errcat(sParamTab::errString);
                delete [] sParamTab::errString;
                sParamTab::errString = 0;
            }
        }
    }
    check_args(args, nargs);
}


sSubc::~sSubc()
{
    delete [] su_name;
    delete [] su_args;
    delete su_params;
    sLine::destroy(su_body);
}


// Static function.
sSubc *
sSubc::copy(const sSubc *su)
{
    if (!su)
        return (0);
    sSubc *ns = new sSubc;
    ns->su_name = lstring::copy(su->su_name);
    ns->su_args = lstring::copy(su->su_args);
    ns->su_numargs = su->su_numargs;
    ns->su_params = sParamTab::copy(su->su_params);
    ns->su_body = sLine::copy(su->su_body);
    return (ns);
}


#define NODE_FMT "_#%d"
#define VS_FMT "v_%s_%d"

// If a formal argument is duplicated, assign the duplicate a new node,
// and add a dummy voltage source to the subcircuit definition.
//
void
sSubc::check_args(const char *args, int nargs)
{
    if (!args || !*args || nargs <= 0) {
        su_args = lstring::copy("");
        su_numargs = 0;
        return;
    }
    wordlist *w0 = 0, *end = 0;
    int count = 0;
    su_numargs = nargs;
    while (*args && count < nargs) {
        const char *tok = args;
        const char *t = tok;
        while (*t && !isspace(*t) && *t != ',')
            t++;
        if (!w0)
            w0 = end = new wordlist;
        else {
            end->wl_next = new wordlist;
            end = end->wl_next;
        }
        end->wl_word = new char[(t - tok) + 1];
        strncpy(end->wl_word, tok, (t - tok));
        end->wl_word[(t - tok)] = 0;
        atok(&args);
        count++;
    }
    count = 0;
    char buf[256];
    for (wordlist *w1 = w0; w1; w1 = w1->wl_next) {
        for (wordlist *w2 = w1->wl_next; w2; w2 = w2->wl_next) {
            if (sScGlobal::name_eq(w1->wl_word, w2->wl_word)) {
                sprintf(buf, NODE_FMT, count);
                delete [] w2->wl_word;
                w2->wl_word = lstring::copy(buf);
                sprintf(buf, VS_FMT" %s %s", su_name, count, w1->wl_word,
                    w2->wl_word);
                sLine *l = new sLine;
                l->set_line(buf);
                l->set_next(su_body);
                su_body = l;
                count++;
            }
        }
    }
    su_args = wordlist::flatten(w0);
    wordlist::destroy(w0);
}
// End of sSubc functions.


sSubcTab::~sSubcTab()
{
    sHgen gen(this, true);
    sHent *h;
    while ((h = gen.next()) != 0) {
        delete (sSubc*)h->data();
        delete h;
    }
}


// Static function.
sSubcTab *
sSubcTab::copy(const sSubcTab *sct)
{
    if (!sct)
        return (0);

    sSubcTab *tab = new sSubcTab;
    sHgen gen(sct);
    sHent *h;
    while ((h = gen.next()) != 0) {
        sSubc *sss = sSubc::copy((sSubc*)h->data());
        tab->add(h->name(), sss);
    }
    return (tab);
}
// End of sSubcTab functions.


wordlist *
sCblk::to_wl(wordlist *wl)
{
    sHgen gen(cb_subs);
    sHent *h;
    while ((h = gen.next()) != 0) {
        sSubc *sss = (sSubc*)h->data();
        for (sLine *l = sss->su_body; l; l = l->next()) {
            wl->wl_next = new wordlist(l->line(), wl);
            wl = wl->wl_next;
        }
    }
    gen = sHgen(cb_mods);
    while ((h = gen.next()) != 0) {
        sINPmodel *mod = (sINPmodel*)h->data();
        wl->wl_next = new wordlist(mod->modLine, wl);
        wl = wl->wl_next;
    }
    return (wl);
}
// End of sCblk functions.


sCblkTab::~sCblkTab()
{
    sHgen gen(this, true);
    sHent *h;
    while ((h = gen.next()) != 0) {
        delete (sCblk*)h->data();
        delete h;
    }
}


wordlist *
sCblkTab::dump_wl(const char *tag)
{
    const sCblkTab *cbt = this;
    if (!cbt)
        return (0);

    char buf[128];
    if (tag) {
        sCblk *blk = (sCblk*)get(this, tag);
        if (!blk)
            return (0);
        sprintf(buf, "TAG:  %s", tag);
        wordlist *tl = new wordlist(buf, 0);
        blk->to_wl(tl);
        return (tl);
    }

    sHgen gen(this);
    sHent *h;
    wordlist *w0 = 0, *tl = 0;
    while ((h = gen.next()) != 0) {
        sprintf(buf, "TAG:  %s", h->name());
        if (!w0)
            w0 = tl = new wordlist(buf, 0);
        else {
            tl->wl_next = new wordlist(buf, tl);
            tl = tl->wl_next;
        }
        sCblk *blk = (sCblk*)h->data();
        tl = blk->to_wl(tl);
    }
    return (w0);
}
// End of sCblkTab functions.


// Return true if cname is a cache block name.
//
bool
sSPcache::inCache(const char *cname)
{
    return (sHtab::get(cache_tab, cname) != 0);
}


// Return a list of cache block names.
//
wordlist *
sSPcache::listCache()
{
    return (sHtab::wl(cache_tab));
}


// Return a listing of the cache contents.
//
wordlist *
sSPcache::dumpCache(const char *tag)
{
    return (cache_tab->dump_wl(tag));
}


// Remove and destroy a cache block.
//
bool
sSPcache::removeCache(const char *name)
{
    sCblk *cb = (sCblk*)cache_tab->remove(name);
    if (cb) {
        delete cb;
        return (true);
    }
    return (false);
}


// Clear the cache.
//
void
sSPcache::clearCache()
{
    delete cache_tab;
    cache_tab = 0;
}


// Private interface function to actually add elements.
//
void
sSPcache::add(const char *name, sSubcTab *subs, const sParamTab *ptab,
    sModTab *new_tab, const cUdf *udfdb)
{
    sCblk *b = new sCblk(subs, sParamTab::copy(ptab), new_tab, udfdb->copy());
    if (!cache_tab)
        cache_tab = new sCblkTab;
    cache_tab->add(name, b);
}


sCblk *
sSPcache::get(const char *name)
{
    return ((sCblk*)sHtab::get(cache_tab, name));
}
// End of sSPcache functions.


// The following functions export keyword recognition, taking into
// account aliases, variables, etc.  This function should be called
// first.
//
void
sSPcx::kwMatchInit()
{
    // This is called before and after the current circuit variables
    // are set up.  For consistency, we avoid using values defined in
    // the circuit .options line, i.e., these variables are ignored
    // there.

    cx_pexnodes = Sp.GetVar(kw_pexnodes, VTYP_BOOL, 0);
    cx_nobjthack = Sp.GetVar(kw_nobjthack, VTYP_BOOL, 0);

    VTvalue vv;
    const char *s = SUBCKT_KW;
    if (Sp.GetVar(kw_substart, VTYP_STRING, &vv))
        s = vv.get_string();
    if (!cx_start || strcmp(cx_start, s)) {
        delete [] cx_start;
        cx_start = lstring::copy(s);
    }

    s = ENDS_KW;
    if (Sp.GetVar(kw_subend, VTYP_STRING, &vv))
        s = vv.get_string();
    if (!cx_sbend || strcmp(cx_sbend, s)) {
        delete [] cx_sbend;
        cx_sbend = lstring::copy(s);
    }

    s = "X";
    if (Sp.GetVar(kw_subinvoke, VTYP_STRING, &vv))
        s = vv.get_string();
    if (!cx_invoke || strcmp(cx_invoke, s)) {
        delete [] cx_invoke;
        cx_invoke = lstring::copy(s);
    }

    s = MODEL_KW;
    if (Sp.GetVar(kw_modelcard, VTYP_STRING, &vv))
        s = vv.get_string();
    if (!cx_model || strcmp(cx_model, s)) {
        delete [] cx_model;
        cx_model = lstring::copy(s);
    }
}


// Return true for a subcircuit definition start line.
//
bool
sSPcx::kwMatchSubstart(const char *string)
{
    return (lstring::cimatch(cx_start, string) ||
        lstring::cimatch(MACRO_KW, string));
}


// Return true for a subcircuit definition start line.
//
bool
sSPcx::kwMatchSubend(const char *string)
{
    return (lstring::cimatch(cx_sbend, string) ||
        lstring::cimatch(EOM_KW, string));
}


// Return true for a subcircuit invocation line.
//
bool
sSPcx::kwMatchSubinvoke(const char *string)
{
    return (lstring::ciprefix(cx_invoke, string));
}


// Return true for a model definition line.
//
bool
sSPcx::kwMatchModel(const char *string)
{
    return (lstring::cimatch(cx_model, string));
}


// Local provate function.
//
void
sSPcx::init(sFtCirc *circ)
{
    // These are overridden in .options lines.  The Sp.Subc... values
    // are defaults if not given in the circuit.

    VTvalue vv;

    cx_parhier = ParHierGlobal;
    if (Sp.GetVar(spkw_parhier, VTYP_STRING, &vv, circ)) {
        if (lstring::cieq(vv.get_string(), "local"))
            cx_parhier = ParHierLocal;
    }

    cx_catchar = Sp.SubcCatchar();
    if (Sp.GetVar(kw_subc_catchar, VTYP_STRING, &vv, circ)) {
        cx_catchar = *vv.get_string();
    }

    cx_catmode = Sp.SubcCatmode();
    if (Sp.GetVar(kw_subc_catmode, VTYP_STRING, &vv, circ)) {
        if (lstring::cieq(vv.get_string(), "spice3"))
            cx_catmode = SUBC_CATMODE_SPICE3;
        else if (lstring::cieq(vv.get_string(), "wrspice"))
            cx_catmode = SUBC_CATMODE_WR;
    }
}
// End of sSPcx functions.

