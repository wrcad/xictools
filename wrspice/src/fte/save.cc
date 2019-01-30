
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
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "simulator.h"
#include "parser.h"
#include "runop.h"
#include "output.h"
#include "cshell.h"
#include "commands.h"
#include "toolbar.h"
#include "spnumber/hash.h"


//
// The save command handling.
//

// keywords
const char *kw_save   = "save";


namespace {
    // This function must match two names with or without a V() around
    // them.
    //
    bool name_eq(const char *n1, const char *n2)
    {
        if (!n1 || !n2)
            return (false);
        const char *s = strchr(n1, '(');
        if (s)
            n1 = s+1;
        s = strchr(n2, '(');
        if (s)
            n2 = s+1;
        while (isspace(*n1))
            n1++;
        while (isspace(*n2))
            n2++;
        for ( ; ; n1++, n2++) {
            if ((*n1 == 0 || *n1 == ')' || isspace(*n1)) &&
                    (*n2 == 0 || *n2 == ')' || isspace(*n2)))
                return (true);
            if (*n1 != *n2)
                break;
        }
        return (false);
    }


    // Return true if s is one of v(, vm(, vp(, vr(, vi(, or same with i
    // replacing v.
    //
    bool isvec(const char *cs)
    {
        char buf[8];
        if (*cs != 'v' && *cs != 'V' && *cs != 'i' && *cs != 'I')
            return (false);
        strncpy(buf, cs+1, 7);
        buf[7] = '\0';
        char *s = buf;
        for ( ; *s; s++)
            if (isupper(*s))
                *s = tolower(*s);
        s = buf;
        if (*s == '(')
            return (true);
        if (*(s+1) == '(' &&
                (*s == 'm' || *s == 'p' || *s == 'r' || *s == 'i'))
            return (true);
        if (*(s+2) == '(' && *(s+1) == 'b' && *s == 'd')
            return (true);
        return (false);
    }
}


// The save command.
//
void
CommandTab::com_save(wordlist *wl)
{
    OP.saveCmd(wl);
}
// End of CommandTab functions.


void
IFoutput::saveCmd(wordlist *wl)
{
    char buf[BSIZE_SP];
    for ( ; wl; wl = wl->wl_next) {
        if (!isvec(wl->wl_word)) {
            addSave(Sp.CurCircuit(), wl->wl_word);
            continue;
        }
        if (*wl->wl_word == 'v' || *wl->wl_word == 'V') {
            // deal with forms like vxx(a,b)
            char *s = strchr(wl->wl_word, '(') + 1;
            while (isspace(*s))
                s++;
            strcpy(buf, s);
            s = strchr(buf, ',');
            if (!s) {
                addSave(Sp.CurCircuit(), wl->wl_word);
                continue;
            }
            char *n1 = buf;
            *s++ = '\0';
            while (isspace(*s))
                s++;
            char *n2 = s;
            for (s = n1 + strlen(n1) - 1; s > n1; s--) {
                if (isspace(*s))
                    *s = '\0';
                else
                    break;
            }
            for (s = n2 + strlen(n2) - 1; s > n2; s--) {
                if (isspace(*s) || *s == ')')
                    *s = '\0';
                else
                    break;
            }
            addSave(Sp.CurCircuit(), n1);
            addSave(Sp.CurCircuit(), n2);
        }
        else {
            // deal with forms like ixx(vyy)
            char *s = strchr(wl->wl_word, '(') + 1;
            while (isspace(*s))
                s++;
            strcpy(buf, s);
            char *n1 = buf;
            for (s = n1 + strlen(n1) - 1; s > n1; s--) {
                if (isspace(*s) || *s == ')')
                    *s = '\0';
                else
                    break;
            }
            strcat(buf, "#branch");
            addSave(Sp.CurCircuit(), n1);
        }
    }
    ToolBar()->UpdateTrace();
}

// Save a vector.
//
void
IFoutput::addSave(sFtCirc *circuit, const char *name)
{
    char *s = lstring::copy(name);
    CP.Unquote(s);
    for (sRunopSave *td = o_runops->saves(); td; td = td->next()) {
        if (name_eq(s, td->string())) {
            delete [] s;
            return;
        }
    }
    if (circuit) {
        for (sRunopSave *td = circuit->saves(); td; td = td->next()) {
            if (name_eq(s, td->string())) {
                delete [] s;
                return;
            }
        }
    }

    sRunopSave *d = new sRunopSave;
    d->set_active(true);
    d->set_string(s);
    d->set_number(o_runops->new_count());

    if (CP.GetFlag(CP_INTERACTIVE) || !circuit) {
        if (o_runops->saves()) {
            sRunopSave *td = o_runops->saves();
            for ( ; td->next(); td = td->next()) ;
            td->set_next(d);
        }
        else
            o_runops->set_saves(d);
    }
    else {
        if (circuit->saves()) {
            sRunopSave *td = circuit->saves();
            for ( ; td->next(); td = td->next()) ;
            td->set_next(d);
        }
        else
            circuit->set_saves(d);
    }
}


// Fill in a list of vector names used in the runops.  If there are only
// specials in the list as passed and in any db saves, save only specials,
// i.e., names starting with SpecCatchar().
//
// The .measure lines are also checked here.
//
void
IFoutput::getSaves(sFtCirc *circuit, sSaveList *saved)
{
    sRunopDb *db = circuit ? &circuit->runops() : 0;

    ROgen<sRunopSave> svgen(o_runops->saves(), db ? db->saves() : 0);
    for (sRunopSave *d = svgen.next(); d; d = svgen.next()) {
        if (d->active())
            saved->add_save(d->string());
    }
    bool saveall = true;
    sHgen gen(saved->table());
    sHent *h;
    while ((h = gen.next()) != 0) {
        if (*h->name() != Sp.SpecCatchar()) {
            saveall = false;
            break;
        }
    }

    // If there is nothing but specials in the list, saveall will be
    // true, which means that all circuit vectors are automatically
    // saved and the only saves we list here are the specials.
    //
    // However, we have the look for these the "hard way" in any case,
    // since we would like to recognize cases like p(Vsrc) which map
    // to @Vsrc[p].  When done, we'll go back an purge non-specials
    // if saveall is true.

    ROgen<sRunopTrace> tgen(o_runops->traces(), db ? db->traces() : 0);
    for (sRunopTrace *d = tgen.next(); d; d = tgen.next()) {
        if (d->active())
            saved->list_expr(d->string());
    }

    ROgen<sRunopIplot> igen(o_runops->iplots(), db ? db->iplots() : 0);
    for (sRunopIplot *d = igen.next(); d; d = igen.next()) {
        if (d->active())
            saved->list_expr(d->string());
    }

    ROgen<sRunopMeas> mgen(o_runops->measures(), db ? db->measures() : 0);
    for (sRunopMeas *m = mgen.next(); m; m = mgen.next()) {
        if (m->active()) {
            if (m->expr2())
                saved->list_expr(m->expr2());
            for (sMpoint *pt = &m->start(); pt; pt = pt->conj()) {
                saved->list_expr(pt->when_expr1());
                saved->list_expr(pt->when_expr2());
            }
            for (sMpoint *pt = &m->end(); pt; pt = pt->conj()) {
                saved->list_expr(pt->when_expr1());
                saved->list_expr(pt->when_expr2());
            }
            for (sMfunc *f = m->funcs(); f; f = f->next())
                saved->list_expr(f->expr());
            for (sMfunc *f = m->finds(); f; f = f->next())
                saved->list_expr(f->expr());
        }
    }

    ROgen<sRunopStop> sgen(o_runops->stops(), db ? db->stops() : 0);
    for (sRunopStop *d = sgen.next(); d; d = sgen.next()) {
        if (d->active()) {
            for (sMpoint *pt = &d->start(); pt; pt = pt->conj()) {
                saved->list_expr(pt->when_expr1());
                saved->list_expr(pt->when_expr2());
            }
        }
    }

    if (saveall)
        saved->purge_non_special();
    else {
        // Remove all the measure result names, which may have been
        // added if they are referenced by another measure.  These
        // vectors don't exist until the measurement is done.

        mgen = ROgen<sRunopMeas>(o_runops->measures(), db ? db->measures() : 0);
        for (sRunopMeas *m = mgen.next(); m; m = mgen.next())
            saved->remove_save(m->result());
    }
}
// End of IFoutput functions.


sSaveList::~sSaveList()
{
    delete sl_tab;
}


// Return the number of words saved.
//
int
sSaveList::numsaves()
{
    if (!sl_tab)
        return (0);
    return (sl_tab->allocated());
}


// Set the used flag for the entry name, return true if the state was
// set.
//
bool
sSaveList::set_used(const char *name, bool used)
{
    if (!sl_tab)
        return (false);
    sHent *h = sHtab::get_ent(sl_tab, name);
    if (!h)
        return (false);
    h->set_data((void*)(long)used);
    return (true);
}


// Return 1/0 if the element name exists ant the user flag is
// set/unset.  return -1 if not in table.
//
int
sSaveList::is_used(const char *name)
{
    if (!sl_tab)
        return (-1);
    sHent *h = sHtab::get_ent(sl_tab, name);
    if (!h)
        return (-1);
    return (h->data() != 0);
}


// Add word to the save list, if it is not already there.
//
void
sSaveList::add_save(const char *word)
{
    if (!sl_tab)
        sl_tab = new sHtab(sHtab::get_ciflag(CSE_NODE));

    if (sHtab::get_ent(sl_tab, word))
        return;
    sl_tab->add(word, 0);
}


// Remove name from list, if found.
//
void
sSaveList::remove_save(const char *name)
{
    if (name && sl_tab)
        sl_tab->remove(name);
}


// Save all the vectors.
//
void
sSaveList::list_expr(const char *expr)
{
    if (expr) {
        wordlist *wl = CP.LexStringSub(expr);
        if (wl) {
            pnlist *pl0 = Sp.GetPtree(wl, false);
            for (pnlist *pl = pl0; pl; pl = pl->next())
                list_vecs(pl->node());
            pnlist::destroy(pl0);
            wordlist::destroy(wl);
        }
    }
}


// Remove all entries that are not "special", i.e., don't start with
// Sp.SpecCatchar().  The list is resized.
//
void
sSaveList::purge_non_special()
{
    if (sl_tab) {
        wordlist *wl0 = sHtab::wl(sl_tab);
        for (wordlist *wl = wl0; wl; wl = wl->wl_next) {
            if (*wl->wl_word != Sp.SpecCatchar())
                sl_tab->remove(wl->wl_word);
        }
        wordlist::destroy(wl0);
    }
}


// Go through the parse tree, and add any vector names found to the
// saves.
//
void
sSaveList::list_vecs(pnode *pn)
{
    char buf[128];
    if (pn->token_string()) {
        if (!pn->value())
            add_save(pn->token_string());
    }
    else if (pn->func()) {
        if (!pn->func()->func()) {
            if (*pn->func()->name() == 'v')
                sprintf(buf, "v(%s)", pn->left()->token_string());
            else
                sprintf(buf, "%s", pn->left()->token_string());
            add_save(buf);
        }
        else
            list_vecs(pn->left());
    }
    else if (pn->oper()) {
        list_vecs(pn->left());
        if (pn->right())
            list_vecs(pn->right());
    }
}
// End of sSaveList functions.

