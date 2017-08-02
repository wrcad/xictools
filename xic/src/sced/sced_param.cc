
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "main.h"
#include "sced_param.h"
#include "cd_hypertext.h"
#include "paramsub.h"
#include "errorlog.h"


//
// A class for parameter handling, based on WRspice code.
//

cParamCx::cParamCx(const CDs *sdesc)
{
    pc_tab = buildParamTab(sdesc);
    pc_stack = 0;
    pc_local_parhier = localParHier(sdesc);
}


cParamCx::~cParamCx()
{
    delete pc_tab;
    while (pc_stack) {
        PTlist *x = pc_stack;
        pc_stack = pc_stack->next;
        delete x->ptab;
        delete x;
    }
}


// Push the parameter table context to a subcircuit, according to the
// parhier (global or local).  The inst_prms is the instantiation
// parameter string needed it we're flattening.
//
void
cParamCx::push(const CDs *sdesc, const char *inst_prms)
{
    if (!sdesc)
        return;
    pc_stack = new PTlist(pc_tab, pc_stack);
    pc_tab = 0;

    if (pc_local_parhier) {
        // Local parhier, instance and master definitions supersede
        // higher definitions.

        pc_tab = sParamTab::copy(pc_stack->ptab);
        sParamTab *tmpptab = buildParamTab(sdesc);
        pc_tab = sParamTab::update(pc_tab, tmpptab);
        delete tmpptab;
        if (inst_prms) {
            if (!pc_tab)
                pc_tab = new sParamTab;
            pc_tab->update(inst_prms);
        }
    }
    else {
        // Global parhier (the default), higher-level definitions take
        // precedence.

        pc_tab = buildParamTab(sdesc);
        if (inst_prms) {
            if (!pc_tab)
                pc_tab = new sParamTab;
            pc_tab->update(inst_prms);
        }
        pc_tab = sParamTab::update(pc_tab, pc_stack->ptab);
    }
    if (pc_tab->errString) {
        Log()->ErrorLog("param expand", pc_tab->errString);
        delete [] pc_tab->errString;
        pc_tab->errString = 0;
    }
}


// Pop back to the parent context level.
//
void
cParamCx::pop()
{
    delete pc_tab;
    pc_tab = 0;
    PTlist *t = pc_stack;
    pc_stack = pc_stack->next;
    if (t) {
        pc_tab = t->ptab;
        delete t;
    }
}


// Expand the parameters found in pstr.  The original string is freed.
//
void
cParamCx::update(char **pstr)
{
    if (pc_tab) {
        pc_tab->param_subst_all_collapse(pstr);
        if (pc_tab->errString) {
            Log()->ErrorLog("param expand", pc_tab->errString);
            delete [] pc_tab->errString;
            pc_tab->errString = 0;
        }
    }
}


// Return true if prm is a known parameter name.
//
bool
cParamCx::has_param(const char *prm)
{
    if (pc_tab)
        return (pc_tab->get(prm) != 0);
    return (false);
}


namespace {
    // If "parhier" is found in the .options line, return the token
    // that follows.
    //
    char *find_parhier(const char *str)
    {
        const char *t = str;
        lstring::advtok(&t);
        char *tok;
        while ((tok = lstring::gettok(&t, "=,")) != 0) {
            if (lstring::cieq(tok, "parhier")) {
                delete [] tok;
                return (lstring::gettok(&t));
            }
            delete [] tok;
        }
        return (0);
    }

    CDl *sptx_ld;

    // The findLayer call is slow, so use caching.
    //
    CDl *find_SPTX()
    {
        if (!sptx_ld)
            sptx_ld = CDldb()->findLayer("SPTX", Electrical);
        return (sptx_ld);
    }
}


// Static function.
// Search sdesc for the parhier option in .options lines found in
// labels on the SPTX layer.  Return true if the option is found and
// set to Local, false otherwise.
//
bool
cParamCx::localParHier(const CDs *sdesc)
{
    if (!sdesc || !sdesc->isElectrical())
        return (0);
    if (sdesc->owner())
        sdesc = sdesc->owner();
    CDl *ld = find_SPTX();
    if (!ld)
        return (false);

    CDg gdesc;
    gdesc.init_gen(sdesc, ld);
    CDo *od;
    while ((od = gdesc.next()) != 0) {
        if (od->type() != CDLABEL)
            continue;
        hyList *hp = ((CDla*)od)->label();
        if (hp) {
            char *string = hyList::string(hp, HYcvPlain, false);
            if (string) {
                char *s = string;
                while (isspace(*s))
                    s++;
                if (lstring::ciprefix(".opt", s)) {
                    char *ph = find_parhier(s);
                    if (ph) {
                        delete [] string;
                        if (*ph == 'l' || *ph == 'L') {
                            delete [] ph;
                            return (true);
                        }
                        delete [] ph;
                        return (false);
                    }
                }
                delete [] string;
            }
        }
    }
    return (false);
}


// Static function.
// Build a parameter table from the properties and labels of the given
// electrical cell.  Call this for the electrical top cell before
// duality/LVS operations.
//
sParamTab *
cParamCx::buildParamTab(const CDs *sdesc)
{
    if (!sdesc || !sdesc->isElectrical())
        return (0);
    if (sdesc->owner())
        sdesc = sdesc->owner();

    sParamTab *ptab = new sParamTab;
    CDp_user *pp = (CDp_user*)sdesc->prpty(P_PARAM);
    for ( ; pp; pp = pp->next()) {
        hyList *hp = pp->data();
        if (hp) {
            char *string = hyList::string(hp, HYcvPlain, false);
            if (string) {
                sParamTab::extract_params(ptab, string);
                delete [] string;
            }
        }
    }
    // Also look for .param lines in spicetext labels.

    CDl *ld = find_SPTX();
    if (!ld)
        return (ptab);

    CDg gdesc;
    gdesc.init_gen(sdesc, ld);
    CDo *od;
    while ((od = gdesc.next()) != 0) {
        if (od->type() != CDLABEL)
            continue;
        hyList *hp = ((CDla*)od)->label();
        if (hp) {
            char *string = hyList::string(hp, HYcvPlain, false);
            if (string) {
                char *s = string;
                while (isspace(*s))
                    s++;
                if (lstring::ciprefix(".param", s))
                    sParamTab::extract_params(ptab, s);
                delete [] string;
            }
        }
    }
    return (ptab);
}

