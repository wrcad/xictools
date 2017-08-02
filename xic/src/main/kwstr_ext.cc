
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
#include "extif.h"
#include "kwstr_ext.h"
#include "dsp_layer.h"
#include "tech.h"
#include "tech_extract.h"
#include "tech_layer.h"
#include "tech_via.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "cd_lgen.h"
#include "promptline.h"
#include "events.h"
#include "layertab.h"
#include "errorlog.h"

//
// Handling for layer block keywords: extraction keywords.
//


namespace {
    // Return true if the second token is "Exclude".
    //
    bool
    isexcl(const char *str)
    {
        char *tok = lstring::gettok(&str);
        delete [] tok;
        tok = lstring::gettok(&str);
        if (tok && lstring::cieq(tok, Ekw.Exclude())) {
            delete [] tok;
            return (true);
        }
        delete [] tok;
        return (false);
    }
}


// Initialize the keyword list from string if not nil, or from the
// current layer otherwise.
//
void
extKWstruct::load_keywords(const CDl *ld, const char *string)
{
    clear_undo_list();
    stringlist::destroy(kw_list);
    kw_list = 0;
    char *localstr = 0;
    if (!string) {
        localstr = get_settings(ld);
        string = localstr;
    }
    if (string) {
        while (*string) {
            string = lstring::strip_space(string);
            const char *t = string;
            while (*t && *t != '\n')
                t++;
            if (*string) {
                char *ns = new char[t - string + 1];
                strncpy(ns, string, t - string);
                ns[t - string] = 0;
                kw_list = new stringlist(ns, kw_list);
            }
            if (*t)
                t++;
            string = t;
        }
    }
    delete [] localstr;
}


// Insert the keyword string into the list.  Try to be a little clever
// by avoiding keyword incompatibilities and redundancies.  The return
// is a status string (usually nil).
//
// The l1 and l2 strings apply only to Vias, and are the associated
// layer names of the via to replace.
//
char *
extKWstruct::insert_keyword_text(const char *str, const char *l1,
    const char *l2)
{
    exKW type = kwtype(str);
    if (type == exNil)
        return (lstring::copy("Unrecognized keyword."));

    char buf[256];
    clear_undo_list();
    if (type == exConductor) {
        remove_keyword_text(exConductor);
        if (!isexcl(str)) {
            const char *msg = "\"%s\" is already implied by \"%s\".";
            if (inlist(exRouting)) {
                sprintf(buf, msg, Ekw.Conductor(), Ekw.Routing());
                return (lstring::copy(buf));
            }
            if (inlist(exGroundPlane)) {
                sprintf(buf, msg, Ekw.Conductor(), Ekw.GroundPlane());
                return (lstring::copy(buf));
            }
            if (inlist(exGroundPlaneClear)) {
                sprintf(buf, msg, Ekw.Conductor(), Ekw.GroundPlaneClear());
                return (lstring::copy(buf));
            }
            if (inlist(exContact)) {
                sprintf(buf, msg, Ekw.Conductor(), Ekw.Contact());
                return (lstring::copy(buf));
            }
        }
        remove_keyword_text(exVia);
        remove_keyword_text(exDielectric);
    }
    else if (type == exRouting) {
        remove_keyword_text(exConductor, true);
        remove_keyword_text(exRouting);
        remove_keyword_text(exGroundPlane);
        remove_keyword_text(exGroundPlaneClear);
        remove_keyword_text(exVia);
        remove_keyword_text(exDielectric);
    }
    else if (type == exGroundPlane) {
        remove_keyword_text(exConductor, true);
        remove_keyword_text(exRouting);
        remove_keyword_text(exGroundPlane);
        remove_keyword_text(exGroundPlaneClear);
        remove_keyword_text(exVia);
        remove_keyword_text(exDielectric);
    }
    else if (type == exGroundPlaneClear) {
        remove_keyword_text(exConductor, true);
        remove_keyword_text(exRouting);
        remove_keyword_text(exGroundPlane);
        remove_keyword_text(exGroundPlaneClear);
        remove_keyword_text(exDarkField);
        remove_keyword_text(exVia);
        remove_keyword_text(exDielectric);
    }
    else if (type == exContact) {
        remove_keyword_text(exConductor, true);
        remove_keyword_text(exVia);
        remove_keyword_text(exDielectric);
    }
    else if (type == exVia) {
        remove_keyword_text(exConductor);
        remove_keyword_text(exRouting);
        remove_keyword_text(exGroundPlane);
        remove_keyword_text(exGroundPlaneClear);
        remove_keyword_text(exContact);
        remove_keyword_text(exVia, false, l1, l2);
        remove_keyword_text(exDarkField);
        remove_keyword_text(exDielectric);
    }
    else if (type == exDielectric) {
        remove_keyword_text(exConductor);
        remove_keyword_text(exRouting);
        remove_keyword_text(exGroundPlane);
        remove_keyword_text(exGroundPlaneClear);
        remove_keyword_text(exContact);
        remove_keyword_text(exVia);
        remove_keyword_text(exDielectric);
    }
    else if (type == exDarkField) {
        remove_keyword_text(exDarkField);
        const char *msg = "\"%s\" is already implied by \"%s\".";
        if (inlist(exGroundPlaneClear)) {
            sprintf(buf, msg, Ekw.DarkField(), Ekw.GroundPlaneClear());
            return (lstring::copy(buf));
        }
        if (inlist(exVia)) {
            sprintf(buf, msg, Ekw.DarkField(), Ekw.Via());
            return (lstring::copy(buf));
        }
    }
    else {
        // impossible
        return (lstring::copy("Unknown keyword."));
    }

    kw_newstr = lstring::copy(str);
    kw_list = new stringlist(kw_newstr, kw_list);
    if (kw_undolist && (kw_undolist->next ||
            strncmp(kw_undolist->string, str, 4))) {
        // don't show this if just replacing existing keyword
        const char *imsg = "Incompatible and redundant keywords removed.";
        return (lstring::copy(imsg));
    }
    return (0);
}


// Remove element or type, keep_cexcl is a special case for "Conductor
// Exclude", which is not removed if keep_cexcl is true.
//
void
extKWstruct::remove_keyword_text(int type, bool keep_cexcl,
    const char *l1, const char *l2)
{
    stringlist *lp = 0, *lnxt;
    for (stringlist *l = kw_list; l; l = lnxt) {
        lnxt = l->next;

        if (type == kwtype(l->string)) {
            if (type == exConductor && keep_cexcl) {
                char *s = l->string;
                lstring::advtok(&s);
                char *tok = lstring::gettok(&s);
                if (tok && lstring::cieq(tok, Ekw.Exclude())) {
                    delete [] tok;
                    lp = l;
                    continue;
                }
                delete [] tok;
                remove(lp, l);
                return;
            }
            if (type == exVia) {
                // The Via keyword is special in that it can appear
                // any number of times on a layer.  However, the two
                // associated layer names are unique, in either order.
                //
                // If two layer names are given, the matching line is
                // removed.  Otherwise, all Via lines are removed.

                if (l1 && l2) {
                    char *s = l->string;
                    lstring::advtok(&s);
                    char *tl1 = lstring::gettok(&s);
                    char *tl2 = lstring::gettok(&s);
                    if (tl1 && tl2) {
                        if (!strcmp(l1, tl1) && !strcmp(l2, tl2)) {
                            delete [] tl1;
                            delete [] tl2;
                            remove(lp, l);
                            return;
                        }
                        if (!strcmp(l1, tl2) && !strcmp(l2, tl1)) {
                            delete [] tl1;
                            delete [] tl2;
                            remove(lp, l);
                            return;
                        }
                        delete [] tl1;
                        delete [] tl2;
                        lp = l;
                        continue;
                    }
                    delete [] tl1;
                    delete [] tl2;
                }
                remove(lp, l);
                continue;
            }
            remove(lp, l);
            return;
        }
        lp = l;
    }
}


// Return a char string, with each keyword/value taking one line.
//
char *
extKWstruct::list_keywords()
{
    sort();
    char *s = stringlist::flatten(kw_list, "\n"); 
    if (!s)
        s = lstring::copy("");
    return (s);
}


// Return true if type is in list.
//
bool
extKWstruct::inlist(exKW type)
{
    for (stringlist *l = kw_list; l; l = l->next) {
        if (type == kwtype(l->string))
            return (true);
    }
    return (false);
}


namespace {
    // Sort comparison, order is same as enum.
    //
    bool sortcmp(const char *a, const char *b)
    {
        int i1 = extKWstruct::kwtype(a);
        int i2 = extKWstruct::kwtype(b);
        if (i1 != i2)
            return (i1 < i2);
        return (strcmp(a, b) < 0);
    }
}


void
extKWstruct::sort()
{
    stringlist::sort(kw_list, sortcmp);
}


// Get text input from user.
//
char *
extKWstruct::prompt(const char *inittext, const char *deftext)
{
    kw_editing = true;
    char *s = PL()->EditPrompt(inittext, deftext);
    kw_editing = false;
    if (!s)
        PL()->ErasePrompt();
    return (lstring::strip_space(s));
}


namespace {
    CDl *
    find_ground()
    {
        CDl *ld;
        CDlgen lgen(Physical);
        while ((ld = lgen.next()) != 0) {
            if (ld->isGroundPlane())
                return (ld);
        }
        return (0);
    }


    // Copy the next token into buf.  If isexp is true the next item
    // is a layer expression (possibly several tokens).
    //
    void
    nexttok(const char **str, char *buf, bool isexp)
    {
        *buf = 0;
        if (!str || !*str || !**str)
            return;
        if (!isexp) {
            char *t = lstring::gettok(str);
            strcpy(buf, t);
            delete [] t;
        }
        else {
            const char *s = *str;
            sLspec *exclude = new sLspec;
            bool ret = exclude->parseExpr(str);
            delete exclude;
            if (ret) {
                strncpy(buf, s, *str - s);
                buf[*str - s] = 0;
            }
            else {
                Errs()->get_error();
                char *t = lstring::gettok(str);
                strcpy(buf, t);
                delete [] t;
            }
        }
        char *t = buf + strlen(buf) - 1;
        while (t >= buf && isspace(*t))
            *t-- = 0;
    }
}


// Return the string for the keyword indicated by type.
//
char *
extKWstruct::get_string_for(int type, const char *orig)
{
    EV()->InitCallback();
    char *in, buf[256];
    char tbuf[128];
    CDl *gp;
    switch (type) {
    case exConductor:
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        if (lstring::cieq(tbuf, Ekw.Exclude()))
            nexttok(&orig, tbuf, true);
        in = prompt("Enter optional layer expression to exclude: ", tbuf);
        if (!in)
            return (0);
        in = lstring::strip_space(in);
        if (*in)
            sprintf(buf, "%s %s %s", Ekw.Conductor(), Ekw.Exclude(), in);
        else
            strcpy(buf, Ekw.Conductor());
        break;

    case exRouting:
        nexttok(&orig, tbuf, false);
        in = prompt("Enter optional routing parameters: ", orig);
        if (!in)
            return (0);
        in = lstring::strip_space(in);
        if (*in)
            sprintf(buf, "%s %s", Ekw.Routing(), in);
        else
            strcpy(buf, Ekw.Routing());
        break;

    case exGroundPlane:
    case exGroundPlaneClear:
        gp = find_ground();
        if (gp && gp != LT()->CurLayer()) {
            PL()->ShowPromptV("Ground plane already defined, layer %s.",
                gp->name());
            return (0);
        }
        if (type == exGroundPlane) {
            strcpy(buf, "GroundPlane");
            break;
        }
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt(
            "Handle wire nets on ground plane? (compute intensive): ",
                lstring::cieq(tbuf, Ekw.MultiNet()) ? "y" : "n");
        if (!in)
            return (0);
        in = lstring::strip_space(in);
        if (*in == 'y' || *in == 'Y') {
            nexttok(&orig, tbuf, false);
            if (isdigit(*tbuf))
                tbuf[1] = 0;
            else
                tbuf[0] = 0;
            in = prompt("Inversion method? (0-2): ", tbuf);
            if (!in)
                return (0);
            in = lstring::strip_space(in);
            if (*in == '1' || *in == '2') {
                in[1] = 0;
                sprintf(buf, "%s %s %s", Ekw.GroundPlaneClear(),
                    Ekw.MultiNet(), in);
            }
            else
                sprintf(buf, "%s %s", Ekw.GroundPlaneClear(), Ekw.MultiNet());
        }
        else
            strcpy(buf, Ekw.GroundPlaneClear());
        break;

    case exContact:
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt("Enter the name of the contact layer: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            char *tok = lstring::gettok(&in);
            if (tok) {
                sprintf(buf, "%s %s", Ekw.Contact(), tok);
                delete [] tok;
                break;
            }
            in = prompt(
                "Bad input, reenter contact layer name: ", tbuf);
        }

        nexttok(&orig, tbuf, true);
        in = prompt(
            "Enter optional layer expression that must be true for contact: ",
            tbuf);
        if (!in)
            return (0);
        in = lstring::strip_space(in);
        if (*in)
            sprintf(buf + strlen(buf), " %s", in);
        break;

    case exVia:
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt("Enter the first conductor layer name: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            char *tok = lstring::gettok(&in);
            if (tok) {
                sprintf(buf, "%s %s", Ekw.Via(), tok);
                delete [] tok;
                break;
            }
            in = prompt(
                "Bad input, reenter first conductor layer name: ", tbuf);
        }

        nexttok(&orig, tbuf, false);
        in = prompt("Enter the second conductor layer name: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            char *tok = lstring::gettok(&in);
            if (tok) {
                sprintf(buf + strlen(buf), " %s", tok);
                delete [] tok;
                break;
            }
            in = prompt(
                "Bad input, reenter second conductor layer name: ", tbuf);
        }

        nexttok(&orig, tbuf, true);
        in = prompt(
            "Enter optional layer expression that must be true for contact: ",
            tbuf);
        if (!in)
            return (0);
        in = lstring::strip_space(in);
        if (*in)
            sprintf(buf + strlen(buf), " %s", in);
        break;

    case exDielectric:
        strcpy(buf, Ekw.Dielectric());
        break;

    case exDarkField:
        strcpy(buf, Ekw.DarkField());
        break;

    default:
        PL()->ErasePrompt();
        return (0);
    }
    PL()->ErasePrompt();
    return (lstring::copy(buf));
}


// Static function.
// Return the type of the keyword string (static function).
//
exKW
extKWstruct::kwtype(const char *str)
{
    char *tok = lstring::gettok(&str);
    exKW ret = exNil;
    if (tok) {
        if (lstring::cieq(tok, Ekw.Conductor()))
            ret = exConductor;
        else if (lstring::cieq(tok, Ekw.Routing()))
            ret = exRouting;
        else if (lstring::cieq(tok, Ekw.GroundPlane()) ||
                lstring::cieq(tok, Ekw.GroundPlaneDark()))
            ret = exGroundPlane;
        else if (lstring::cieq(tok, Ekw.GroundPlaneClear()) ||
                lstring::cieq(tok, Ekw.TermDefault()))
            ret = exGroundPlaneClear;
        else if (lstring::cieq(tok, Ekw.Contact()))
            ret = exContact;
        else if (lstring::cieq(tok, Ekw.Via()))
            ret = exVia;
        else if (lstring::cieq(tok, Ekw.Dielectric()))
            ret = exDielectric;
        else if (lstring::cieq(tok, Ekw.DarkField()))
            ret = exDarkField;
    }
    delete [] tok;
    return (ret);
}


// Static function.
// Create a string containing the electrical/extract keywords and
// settings.
//
char *
extKWstruct::get_settings(const CDl *ld)
{
    if (!ld)
        return (lstring::copy("\n"));
    char buf[256];
    *buf = '\0';
    sLstr lstr;
    if (ld->isVia()) {
        for (sVia *v = tech_prm(ld)->via_list(); v; v = v->next()) {
            if (v->layer1() && v->layer2()) {
                sprintf(buf, "%s %s %s ", Ekw.Via(), v->layername1(),
                    v->layername2());
                lstr.add(buf);
                if (v->tree())
                    v->tree()->string(lstr);
                lstr.add_c('\n');
            }
        }
    }
    else if (ld->isDielectric()) {
        lstr.add(Ekw.Dielectric());
        lstr.add_c('\n');
    }
    else if (ld->isGroundPlane()) {
        if (ld->isDarkField()) {
            if (Tech()->IsInvertGroundPlane()) {
                lstr.add(Ekw.GroundPlaneClear());
                lstr.add_c(' ');
                lstr.add(Ekw.MultiNet());
                if (Tech()->GroundPlaneMode() == GPI_TOP)
                    lstr.add(" 1\n");
                else if (Tech()->GroundPlaneMode() == GPI_ALL)
                    lstr.add(" 2\n");
                else
                    lstr.add_c('\n');
            }
            else {
                lstr.add(Ekw.GroundPlaneClear());
                lstr.add_c('\n');
            }
        }
        else {
            lstr.add(Ekw.GroundPlane());
            lstr.add_c('\n');
        }
    }
    else if (ld->isRouting()) {
        if (tech_prm(ld)->exclude()) {
            lstr.add(Ekw.Conductor());
            char *stmp = tech_prm(ld)->exclude()->string();
            if (stmp) {
                lstr.add_c(' ');
                lstr.add(Ekw.Exclude());
                lstr.add_c(' ');
                lstr.add(stmp);
                delete [] stmp;
            }
            lstr.add_c('\n');
        }
        Tech()->WriteRouting(ld, 0, &lstr);
        lstr.add_c('\n');
    }
    else if (ld->isInContact()) {
        for (sVia *v = tech_prm(ld)->via_list(); v; v = v->next()) {
            if (v->layer1()) {
                sprintf(buf, "%s %s ", Ekw.Contact(), v->layername1());
                lstr.add(buf);
                if (v->tree())
                    v->tree()->string(lstr);
                lstr.add_c('\n');
            }
        }
    }
    else if (ld->isConductor()) {
        lstr.add(Ekw.Conductor());
        if (tech_prm(ld)->exclude()) {
            char *stmp = tech_prm(ld)->exclude()->string();
            if (stmp) {
                lstr.add_c(' ');
                lstr.add(Ekw.Exclude());
                lstr.add_c(' ');
                lstr.add(stmp);
                delete [] stmp;
            }
        }
        lstr.add_c('\n');
    }
    if (ld->isDarkField() && !ld->isGroundPlane() && !ld->isVia()) {
        lstr.add(Ekw.DarkField());
        lstr.add_c('\n');
    }

    lstr.add_c('\n');
    char *str = lstr.string_trim();
    return (str);
}


// Error/return flags used below.
//
#define ELP_ERR  0x1        // Badness
#define ELP_CO   0x2        // Conductor
#define ELP_RO   0x4        // Routing
#define ELP_GPD  0x8        // GroundPlandDark
#define ELP_GPC  0x10       // GroundPlaneClear
#define ELP_CN   0x20       // Contact
#define ELP_V    0x40       // Via
#define ELP_DI   0x80       // Dielectric
#define ELP_DF   0x100      // DarkField
#define ELP_UI   0x200      // MultiNet
#define ELP_UIM0 0x400      // Invert 0
#define ELP_UIM1 0x800      // Invert 1
#define ELP_UIM2 0x1000     // Invert 2
#define ELP_VP   0x2000     // Via parse tree

namespace {
    // Parse one line of text and set the layer desc accordingly,
    // returning an error code if unsuccessful.
    //
    int set_line(CDl *ld, const char **line, int flags)
    {
        if (!ld)
            return (ELP_ERR);
        int ret = 0;
        char kwbuf[128];
        char inbuf[256];
        *kwbuf = '\0';
        *inbuf = '\0';
        {
            const char *s = *line;
            char *bptr = kwbuf;
            while (isspace(*s))
                s++;
            while (*s && !isspace(*s))
                *bptr++ = *s++;
            *bptr = '\0';
            while (isspace(*s) && *s != '\n')
                s++;
            if (*s && *s != '\n') {
                bptr = inbuf;
                while (*s && *s != '\n')
                    *bptr++ = *s++;
                *bptr = '\0';
            }
            while (isspace(*s))
                s++;
            *line = s;
            if (!*kwbuf)
                return (ELP_ERR);
        }

        if (lstring::cieq(kwbuf, Ekw.Conductor())) {
            if (flags & (ELP_V | ELP_DI))
                return (ELP_CO | ELP_ERR);
            ld->setConductor(true);
            ret |= ELP_CO;

            const char *bptr = inbuf;
            char *s = lstring::gettok(&bptr);
            if (s) {
                if (lstring::cieq(s, Ekw.Exclude())) {
                    delete [] s;
                    sLspec *exclude = new sLspec;
                    if (!exclude->parseExpr(&bptr)) {
                        delete exclude;
                        Log()->PopUpErr(Errs()->get_error());
                        return (ret | ELP_ERR);
                    }
                    tech_prm(ld)->set_exclude(exclude);
                }
                else {
                    delete [] s;
                    return (ret | ELP_ERR);
                }
            }
        }
        else if (lstring::cieq(kwbuf, Ekw.Routing())) {
            if (flags & (ELP_V | ELP_DI))
                return (ELP_RO | ELP_ERR);
            if (!Tech()->ParseRouting(ld, inbuf)) {
                Log()->PopUpErr(Errs()->get_error());
                return (ELP_RO | ELP_ERR);
            }
            ld->setConductor(true);
            ld->setRouting(true);
            ret |= ELP_RO;
        }
        else if (lstring::cieq(kwbuf, Ekw.GroundPlane()) ||
                lstring::cieq(kwbuf, Ekw.GroundPlaneDark())) {
            if (flags & (ELP_V | ELP_DI))
                return (ELP_GPD | ELP_ERR);
            ld->setConductor(true);
            ld->setGroundPlane(true);
        }
        else if (lstring::cieq(kwbuf, Ekw.GroundPlaneClear()) ||
                lstring::cieq(kwbuf, Ekw.TermDefault())) {
            if (flags & (ELP_V | ELP_DI))
                return (ELP_GPC | ELP_ERR);
            ld->setConductor(true);
            ld->setGroundPlane(true);
            ld->setDarkField(true);
            ret |= (ELP_GPC | ELP_CO);
            const char *bptr = inbuf;
            char*s = lstring::gettok(&bptr);
            if (s) {
                if (lstring::cieq(s, "MULTINET")) {
                    ret |= ELP_UI;
                    delete [] s;
                    s = lstring::gettok(&bptr);
                    if (s) {
                        if (*s == '0' && *(s+1) == 0)
                            ret |= ELP_UIM0;
                        if (*s == '1' && *(s+1) == 0)
                            ret |= ELP_UIM1;
                        if (*s == '2' && *(s+1) == 0)
                            ret |= ELP_UIM2;
                        else {
                            delete [] s;
                            return (ELP_UI | ELP_UIM0 | ELP_ERR);
                        }
                        delete [] s;
                    }
                    else
                        ret |= ELP_UIM0;
                }
                else {
                    delete [] s;
                    return (ELP_UI | ELP_ERR);
                }
            }
        }
        else if (lstring::cieq(kwbuf, Ekw.Contact())) {
            if (flags & (ELP_V | ELP_DI))
                return (ELP_CN | ELP_ERR);
            const char*s = inbuf;
            char *vs1 = lstring::gettok(&s);
            if (vs1) {
                if (strlen(vs1) > 4)
                    vs1[4] = '\0';
            }
            else
                return (ELP_CN | ELP_ERR);
            ParseNode *tree = 0;
            if (*s) {
                tree = SIparse()->getLexprTree(&s);
                if (!tree) {
                    char *er = SIparse()->errMessage();
                    if (er) {
                        Log()->PopUpErr(er);
                        delete [] er;
                    }
                    delete [] vs1;
                    return (ELP_VP | ELP_ERR);
                }
            }
            sVia *via = new sVia(vs1, 0, tree);
            via->setNext(tech_prm(ld)->via_list());
            tech_prm(ld)->set_via_list(via);
            ld->setConductor(true);
            ld->setInContact(true);
            ret |= (ELP_CN | ELP_CO);
        }
        else if (lstring::cieq(kwbuf, Ekw.Via())) {
            if (flags & (ELP_CO | ELP_RO | ELP_GPD | ELP_GPC | ELP_CN |
                    ELP_DI | ELP_UI))
                return (ELP_V | ELP_ERR);
            const char *s = inbuf;
            char *vs1 = lstring::gettok(&s);
            if (vs1) {
                if (strlen(vs1) > 4)
                    vs1[4] = '\0';
            }
            else
                return (ELP_V | ELP_ERR);
            char *vs2 = lstring::gettok(&s);
            if (vs2) {
                if (strlen(vs2) > 4)
                    vs2[4] = '\0';
            }
            else
                return (ELP_V | ELP_ERR);
            ParseNode *tree = 0;
            if (*s) {
                tree = SIparse()->getLexprTree(&s);
                if (!tree) {
                    char *er = SIparse()->errMessage();
                    if (er) {
                        Log()->PopUpErr(er);
                        delete [] er;
                    }
                    delete [] vs1;
                    delete [] vs2;
                    return (ELP_VP | ELP_ERR);
                }
            }
            sVia *via = new sVia(vs1, vs2, tree);
            via->setNext(tech_prm(ld)->via_list());
            tech_prm(ld)->set_via_list(via);
            ld->setVia(true);
            ld->setDarkField(true);
            ret |= ELP_V;
        }
        else if (lstring::cieq(kwbuf, Ekw.Dielectric())) {
            if (flags & (ELP_CO | ELP_RO | ELP_GPD | ELP_GPC | ELP_CN |
                    ELP_V | ELP_UI))
                return (ELP_DI | ELP_ERR);
            ld->setDielectric(true);
            ret |= ELP_DI;
        }
        else if (lstring::cieq(kwbuf, Ekw.DarkField())) {
            ld->setDarkField(true);
            ret |= ELP_DF;
        }
        else
            ret |= ELP_ERR;
        return (ret);
    }


    // Parameter backup.
    struct esbak_t
    {
        esbak_t(CDl *ld)
            {
                mask = 0;
                if (ld->isConductor())
                    mask |= CDL_CONDUCTOR;
                ld->setConductor(false);
                if (ld->isRouting())
                    mask |= CDL_ROUTING;
                ld->setRouting(false);
                if (ld->isGroundPlane())
                    mask |= CDL_GROUNDPLANE;
                ld->setGroundPlane(false);
                if (ld->isInContact())
                    mask |= CDL_IN_CONTACT;
                ld->setInContact(false);
                if (ld->isVia())
                    mask |= CDL_VIA;
                ld->setVia(false);
                if (ld->isDielectric())
                    mask |= CDL_DIELECTRIC;
                ld->setDielectric(false);
                if (ld->isDarkField())
                    mask |= CDL_DARKFIELD;
                ld->setDarkField(false);

                TechLayerParams *lp = tech_prm(ld);

                via_list = lp->via_list();
                lp->set_via_list(0);

                exclude = lp->exclude();
                lp->set_exclude(0);

                gp_lname = lp->gp_lname();
                lp->set_gp_lname(0);

                ldesc = ld;
            }

        ~esbak_t()
            {
                sVia::destroy(via_list);
                delete exclude;
                delete [] gp_lname;
            }

        void revert()
            {
                if (!ldesc)
                    return;

                ldesc->setConductor(false);
                if (mask & CDL_CONDUCTOR)
                    ldesc->setConductor(true);
                ldesc->setRouting(false);
                if (mask & CDL_ROUTING)
                    ldesc->setRouting(true);
                ldesc->setGroundPlane(false);
                if (mask & CDL_GROUNDPLANE)
                    ldesc->setGroundPlane(true);
                ldesc->setInContact(false);
                if (mask & CDL_IN_CONTACT)
                    ldesc->setInContact(true);
                ldesc->setVia(false);
                if (mask & CDL_VIA)
                    ldesc->setVia(true);
                ldesc->setDielectric(false);
                if (mask & CDL_DIELECTRIC)
                    ldesc->setDielectric(true);
                ldesc->setDarkField(false);
                if (mask & CDL_DARKFIELD)
                    ldesc->setDarkField(true);

                TechLayerParams *lp = tech_prm(ldesc);

                sVia::destroy(lp->via_list());
                lp->set_via_list(via_list);
                via_list = 0;

                delete lp->exclude();
                lp->set_exclude(exclude);
                exclude = 0;

                delete [] lp->gp_lname();
                lp->set_gp_lname((char*)gp_lname);
                gp_lname = 0;
            }

    private:
        sVia *via_list;
        sLspec *exclude;
        const char *gp_lname;
        CDl *ldesc;
        int mask;
    };
}


// Static function.
// Establish the extraction settings from the string, returning a status
// message (must free) if error.
//
char *
extKWstruct::set_settings(CDl *ld, const char *string)
{
    if (!ld)
        return (lstring::copy("No current layer!"));

    // save and clear relevant info
    esbak_t esbak(ld);

    if (string) {
        int linecnt = 1, flags = 0;
        const char *str = string;
        while (*str) {
            int result = set_line(ld, &str, flags);
            if (result & ELP_ERR) {

                // Failed, revert
                result &= ~ELP_ERR;
                const char *msg;
                char buf[128];
                if (result &
                        (ELP_CO | ELP_RO | ELP_V | ELP_CN | ELP_GPD |
                        ELP_GPC | ELP_DF | ELP_UI)) {
                    if (flags &
                            (ELP_CO | ELP_RO | ELP_V | ELP_CN | ELP_GPD |
                            ELP_GPC | ELP_DF | ELP_UI))
                        msg = "Retry: inappropriate extract keyword line %d";
                    else if (result & ELP_VP)
                        msg = "Retry: conjunction parse error line %d";
                    else
                        msg = "Retry: bad input line %d";
                }
                else
                    msg = "Retry: unknown keyword line %d";
                sprintf(buf, msg, linecnt);

                esbak.revert();

                return (lstring::copy(buf));
            }
            flags |= result;
            linecnt++;

            if (result & ELP_GPC) {
                // this will invalidate ground plane
                if (result & ELP_UI)
                    CDvdb()->setVariable(VA_GroundPlaneMulti, 0);
                else
                    CDvdb()->clearVariable(VA_GroundPlaneMulti);
                if (result & ELP_UIM0)
                    CDvdb()->clearVariable(VA_GroundPlaneMethod);
                else if (result & ELP_UIM1)
                    CDvdb()->setVariable(VA_GroundPlaneMethod, "1");
                else if (result & ELP_UIM2)
                    CDvdb()->setVariable(VA_GroundPlaneMethod, "2");
            }
        }
    }

    ExtIf()->invalidateGroups();
    return (0);
}

