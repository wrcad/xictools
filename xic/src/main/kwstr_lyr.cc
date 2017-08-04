
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "main.h"
#include "kwstr_lyr.h"
#include "dsp_layer.h"
#include "promptline.h"
#include "errorlog.h"
#include "events.h"
#include "layertab.h"

//
// Handling for layer block keywords: misc. layer keywords.
//


// Load the keyword list and the widget text (sorted).
//
void
lyrKWstruct::load_keywords(const CDl *ld, const char *string)
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


// Insert the keyword string into the list.  The return is a status
// string (usually nil).
//
char *
lyrKWstruct::insert_keyword_text(const char *str, const char*, const char*)
{
    lpKW type = kwtype(str);
    if (type == lpNil)
        return (lstring::copy("Unrecognized keyword."));

    clear_undo_list();
    remove_keyword_text(type, false, 0, 0);
    kw_newstr = lstring::copy(str);
    kw_list = new stringlist(kw_newstr, kw_list);
    return (0);
}


void
lyrKWstruct::remove_keyword_text(int type, bool, const char*, const char*)
{
    stringlist *lp = 0;
    for (stringlist *l = kw_list; l; l = l->next) {
        if (type == kwtype(l->string)) {
            remove(lp, l);
            return;
        }
        lp = l;
    }
}


// Return a char string, with each keyword/value taking one line.
//
char *
lyrKWstruct::list_keywords()
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
lyrKWstruct::inlist(lpKW type)
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
        int i1 = lyrKWstruct::kwtype(a);
        int i2 = lyrKWstruct::kwtype(b);
        if (i1 != i2)
            return (i1 < i2);
        return (strcmp(a, b) < 0);
    }
}


void
lyrKWstruct::sort()
{
    stringlist::sort(kw_list, sortcmp);
}


// Get text input from user.
//
char *
lyrKWstruct::prompt(const char *inittext, const char *deftext)
{
    kw_editing = true;
    char *s = PL()->EditPrompt(inittext, deftext);
    kw_editing = false;
    if (!s)
        PL()->ErasePrompt();
    return (lstring::strip_space(s));
}


// Return the string for the keyword indicated by which.
//
char *
lyrKWstruct::get_string_for(int type, const char *orig)
{
    EV()->InitCallback();
    char *in, buf[256];
    switch (type) {
    case lpLppName:
        lstring::advtok(&orig);
        in = prompt("Enter alias name for layer ", orig);
        if (!in)
            return (0);
        sprintf(buf, "%s %s", Tkw.LppName(), in);
        in = buf + strlen(buf) - 1;
        while (in >= buf && isspace(*in))
            *in-- = 0;
        break;
    case lpDescription:
        lstring::advtok(&orig);
        in = prompt("Enter description for layer ", orig);
        if (!in)
            return (0);
        sprintf(buf, "%s %s", Tkw.Description(), in);
        in = buf + strlen(buf) - 1;
        while (in >= buf && isspace(*in))
            *in-- = 0;
        break;
    case lpNoSelect:
        strcpy(buf, Tkw.NoSelect());
        break;
    case lpNoMerge:
        strcpy(buf, Tkw.NoMerge());
        break;
    case lpWireActive:
        strcpy(buf, Tkw.WireActive());
        break;
    case lpSymbolic:
        strcpy(buf, Tkw.Symbolic());
        break;
    case lpInvalid:
        strcpy(buf, Tkw.Invalid());
        break;
    case lpInvisible:
        strcpy(buf, Tkw.Invisible());
        break;
    case lpNoInstView:
        strcpy(buf, Tkw.NoInstView());
        break;
    case lpWireWidth:
        lstring::advtok(&orig);
        strcpy(buf, Tkw.WireWidth());
        in = prompt("Enter default wire width (microns) ", orig);
        for (;;) {
            if (!in)
                return (0);
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d >= 0.0 && d <= 100.0) {
                sprintf(buf + strlen(buf), " %.4f", d);
                break;
            }
            in = prompt(
                "Bad input, reenter wire width (0.0 - 100.0) : ", orig);
        }
        break;
    case lpCrossThick:
        lstring::advtok(&orig);
        strcpy(buf, Tkw.CrossThick());
        in = prompt("Enter thickness for Cross Section command (microns) ",
            orig);
        for (;;) {
            if (!in)
                return (0);
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d >= 0.01 && d <= 10.0) {
                sprintf(buf + strlen(buf), " %.4f", d);
                break;
            }
            in = prompt(
                "Bad input, reenter cross-section thickness (0.01-10.0) : ",
                orig);
        }
        break;
    }
    PL()->ErasePrompt();
    return (lstring::copy(buf));
}


// Static function.
// Return the type of the keyword string.
//
lpKW
lyrKWstruct::kwtype(const char *str)
{
    char *tok = lstring::gettok(&str);
    lpKW ret = lpNil;
    if (tok) {
        if (lstring::cieq(tok, Tkw.LppName()))
            ret = lpLppName;
        else if (lstring::cieq(tok, Tkw.Description()))
            ret = lpDescription;
        else if (lstring::cieq(tok, Tkw.NoSelect()))
            ret = lpNoSelect;
        else if (lstring::cieq(tok, Tkw.NoMerge()))
            ret = lpNoMerge;
        else if (lstring::cieq(tok, Tkw.WireActive()))
            ret = lpWireActive;
        else if (lstring::cieq(tok, Tkw.Symbolic()))
            ret = lpSymbolic;
        else if (lstring::cieq(tok, Tkw.Invalid()))
            ret = lpInvalid;
        else if (lstring::cieq(tok, Tkw.Invisible()))
            ret = lpInvisible;
        else if (lstring::cieq(tok, Tkw.NoInstView()))
            ret = lpNoInstView;
        else if (lstring::cieq(tok, Tkw.WireWidth()))
            ret = lpWireWidth;
        else if (lstring::cieq(tok, Tkw.CrossThick()))
            ret = lpCrossThick;
    }
    delete [] tok;
    return (ret);
}


char *
lyrKWstruct::get_settings(const CDl *ld)
{
    sLstr lstr;
    if (ld) {
        if (ld->lppName()) {
            lstr.add(Tkw.LppName());
            lstr.add_c(' ');
            lstr.add(ld->lppName());
            lstr.add_c('\n');
        }
        if (ld->description()) {
            lstr.add(Tkw.Description());
            lstr.add_c(' ');
            lstr.add(ld->description());
            lstr.add_c('\n');
        }
        if (ld->isRstNoSelect()) {
            lstr.add(Tkw.NoSelect());
            lstr.add_c('\n');
        }
        if (ld->isNoMerge()) {
            lstr.add(Tkw.NoMerge());
            lstr.add_c('\n');
        }
        if (ld->isWireActive()) {
            lstr.add(Tkw.WireActive());
            lstr.add_c('\n');
        }
        if (ld->isSymbolic()) {
            lstr.add(Tkw.Symbolic());
            lstr.add_c('\n');
        }
        if (ld->isInvalid()) {
            lstr.add(Tkw.Invalid());
            lstr.add_c('\n');
        }
        if (ld->isRstInvisible()) {
            lstr.add(Tkw.Invisible());
            lstr.add_c('\n');
        }
        if (ld->isNoInstView()) {
            lstr.add(Tkw.NoInstView());
            lstr.add_c('\n');
        }
        if (dsp_prm(ld)->wire_width() > 0) {
            lstr.add(Tkw.WireWidth());
            lstr.add_c(' ');
            lstr.add_d(MICRONS(dsp_prm(ld)->wire_width()), 4, false);
            lstr.add_c('\n');
        }
        if (dsp_prm(ld)->xsect_thickness() > 0) {
            lstr.add(Tkw.CrossThick());
            lstr.add_c(' ');
            lstr.add_d(MICRONS(dsp_prm(ld)->xsect_thickness()), 4, false);
            lstr.add_c('\n');
        }
    }

    lstr.add_c('\n');
    char *str = lstr.string_trim();
    return (str);
}


namespace {
    int set_line(CDl *ld, const char **line)
    {
        if (!ld)
            return (false);
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
                return (false);
        }

        if (lstring::cieq(kwbuf, Tkw.NoSelect())) {
            ld->setRstNoSelect(true);
            LT()->SetLayerSelectability(LToff, ld);
            return (true);
        }
        if (lstring::cieq(kwbuf, Tkw.NoMerge())) {
            ld->setNoMerge(true);
            return (true);
        }
        if (lstring::cieq(kwbuf, Tkw.WireActive())) {
            ld->setWireActive(true);
            return (true);
        }
        if (lstring::cieq(kwbuf, Tkw.Symbolic())) {
            ld->setSymbolic(true);
            return (true);
        }
        if (lstring::cieq(kwbuf, Tkw.Invalid())) {
            ld->setInvalid(true);
            return (true);
        }
        if (lstring::cieq(kwbuf, Tkw.Invisible())) {
            ld->setRstInvisible(true);
            LT()->SetLayerVisibility(LToff, ld, false);
            return (true);
        }
        if (lstring::cieq(kwbuf, Tkw.NoInstView())) {
            ld->setNoInstView(true);
            return (true);
        }

        if (!*inbuf)
            return (false);
        if (lstring::cieq(kwbuf, Tkw.LppName())) {
            if (!CDldb()->setLPPname(ld, inbuf)) {
                Log()->ErrorLogV(mh::Processing,
                    "Failed to set LPP name of %s to %s,\nname clash?",
                    ld->name(), inbuf);
            }
            return (true);
        }
        if (lstring::cieq(kwbuf, Tkw.Description())) {
            ld->setDescription(inbuf);
            return (true);
        }
        if (lstring::cieq(kwbuf, Tkw.WireWidth())) {
            double d;
            if (sscanf(inbuf, "%lf", &d) == 1 && d >= 0.0 && d <= 100.0) {
                dsp_prm(ld)->set_wire_width(INTERNAL_UNITS(d));
                return (true);
            }
        }
        if (lstring::cieq(kwbuf, Tkw.CrossThick())) {
            double d;
            if (sscanf(inbuf, "%lf", &d) == 1 && d >= 0.01 && d <= 10.0) {
                dsp_prm(ld)->set_xsect_thickness(INTERNAL_UNITS(d));
                return (true);
            }
        }
        return (false);
    }


    struct lpbak_t
    {
        lpbak_t(CDl *ld)
            {
                lname = lstring::copy(ld->lppName());
                CDldb()->setLPPname(ld, 0);

                descr = lstring::copy(ld->description());
                ld->setDescription(0);

                no_select = ld->isRstNoSelect();
                ld->setRstNoSelect(false);
                LT()->SetLayerSelectability(LTon, ld);

                nomerge = ld->isNoMerge();
                ld->setNoMerge(false);

                wire_active = ld->isWireActive();
                ld->setWireActive(false);

                symbolic = ld->isSymbolic();
                ld->setSymbolic(false);

                invisible = ld->isRstInvisible();
                ld->setRstInvisible(false);
                LT()->SetLayerVisibility(LTon, ld, false);

                no_inst_view = ld->isNoInstView();
                ld->setNoInstView(false);

                w_width = dsp_prm(ld)->wire_width();
                dsp_prm(ld)->set_wire_width(0);

                x_thick = dsp_prm(ld)->xsect_thickness();
                dsp_prm(ld)->set_xsect_thickness(0);

                ldesc = ld;
            }

        ~lpbak_t()
            {
                delete [] lname;
                delete [] descr;
            }

        void revert()
            {
                if (!ldesc)
                    return;

                if (!CDldb()->setLPPname(ldesc, lname)) {
                    Log()->ErrorLogV(mh::Processing,
                        "Failed to set LPP name of %s to %s,\nname clash?",
                        ldesc->name(), lname);
                }
                delete [] lname;
                lname = 0;

                ldesc->setDescription(descr);
                delete [] descr;
                descr = 0;

                ldesc->setRstNoSelect(no_select);
                LT()->SetLayerSelectability(no_select ? LToff : LTon, ldesc);
                ldesc->setNoMerge(nomerge);
                ldesc->setWireActive(wire_active);
                ldesc->setSymbolic(symbolic);
                ldesc->setRstInvisible(invisible);
                LT()->SetLayerVisibility(invisible ? LToff : LTon,
                    ldesc, false);
                ldesc->setNoInstView(no_inst_view);
                dsp_prm(ldesc)->set_wire_width(w_width);
                dsp_prm(ldesc)->set_xsect_thickness(x_thick);
            }

    private:
        CDl *ldesc;
        char *lname;
        char *descr;
        bool no_select;
        bool nomerge;
        bool wire_active;
        bool symbolic;
        bool invisible;
        bool no_inst_view;
        int w_width;
        int x_thick;
    };
}


char *
lyrKWstruct::set_settings(CDl *ld, const char *string)
{
    if (!ld)
        return (lstring::copy("No current layer!"));

    // save and clear relevant info
    lpbak_t lpbak(ld);

    if (string) {
        int linecnt = 1;
        const char *str = string;
        while (*str) {
            if (!set_line(ld, &str)) {

                // Failed, revert
                lpbak.revert();

                char buf[256];
                sprintf(buf, "Retry: error on line %d", linecnt);
                return (lstring::copy(buf));
            }
            linecnt++;
        }
    }
    return (0);
}

