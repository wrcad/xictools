
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
#include "kwstr_phy.h"
#include "dsp_layer.h"
#include "tech.h"
#include "tech_extract.h"
#include "tech_layer.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_lspec.h"
#include "promptline.h"
#include "events.h"

//
// Handling for layer block keywords: physical attribute keywords.
//


// Initialize the keyword list from string if not nil, or from the
// current layer otherwise.
//
void
phyKWstruct::load_keywords(const CDl *ld, const char *string)
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
phyKWstruct::insert_keyword_text(const char *str, const char*, const char*)
{
    phKW type = kwtype(str);
    if (type == phNil)
        return (lstring::copy("Unrecognized keyword."));

    clear_undo_list();
    if (type == phPlanarize)
        remove_keyword_text(phPlanarize);
    else if (type == phThickness)
        remove_keyword_text(phThickness);
    else if (type == phFH_nhinc)
        remove_keyword_text(phFH_nhinc);
    else if (type == phFH_rh)
        remove_keyword_text(phFH_rh);
    else if (type == phRho) {
        remove_keyword_text(phRho);
        remove_keyword_text(phSigma);
    }
    else if (type == phSigma) {
        remove_keyword_text(phRho);
        remove_keyword_text(phSigma);
    }
    else if (type == phRsh)
        remove_keyword_text(phRsh);
    else if (type == phEpsRel)
        remove_keyword_text(phEpsRel);
    else if (type == phCapacitance)
        remove_keyword_text(phCapacitance);
    else if (type == phLambda)
        remove_keyword_text(phLambda);
    else if (type == phTline)
        remove_keyword_text(phTline);
    else if (type == phAntenna)
        remove_keyword_text(phAntenna);
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


// Remove the keyword for type.
//
void
phyKWstruct::remove_keyword_text(int type, bool, const char*, const char*)
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
phyKWstruct::list_keywords()
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
phyKWstruct::inlist(phKW type)
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
        int i1 = phyKWstruct::kwtype(a);
        int i2 = phyKWstruct::kwtype(b);
        if (i1 != i2)
            return (i1 < i2);
        return (strcmp(a, b) < 0);
    }
}


void
phyKWstruct::sort()
{
    stringlist::sort(kw_list, sortcmp);
}


// Get text input from user.
//
char *
phyKWstruct::prompt(const char *inittext, const char *deftext)
{
    kw_editing = true;
    char *s = PL()->EditPrompt(inittext, deftext);
    kw_editing = false;
    if (!s)
        PL()->ErasePrompt();
    return (lstring::strip_space(s));
}


namespace {
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
phyKWstruct::get_string_for(int type, const char *orig)
{
    EV()->InitCallback();
    char *in, buf[256];
    char tbuf[128];
    switch (type) {
    case phPlanarize:
        strcpy(buf, Ekw.Planarize());
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt("Enter optional \"yes\" or \"no\": ", tbuf);
        if (!in)
            return (0);
        in = lstring::strip_space(in);
        if (*in)
            sprintf(buf + strlen(buf), " %s", in);
        break;

    case phThickness:
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt("Enter layer physical thickness in microns: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d >= 0.0) {
                sprintf(buf, "%s %.4f", Ekw.Thickness(), d);
                break;
            }
            in = prompt(
                "Bad input, reenter film thickness: ", tbuf);
        }
        break;

    case phFH_nhinc:
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt("Enter layer FastHenry in-plane filament count: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            int n;
            if (sscanf(in, "%d", &n) == 1 && n > 0) {
                sprintf(buf, "%s %d", Ekw.FH_nhinc(), n);
                break;
            }
            in = prompt(
                "Bad input, reenter FastHenry nhinc: ", tbuf);
        }
        break;

    case phFH_rh:
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt(
            "Enter layer FastHenry in-plane adjacent filament height ratio: ",
            tbuf);
        for (;;) {
            if (!in)
                return (0);
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d > 0.0) {
                sprintf(buf, "%s %.4f", Ekw.FH_rh(), d);
                break;
            }
            in = prompt(
                "Bad input, reenter FastHenry rh: ", tbuf);
        }
        break;

    case phRho:
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt("Enter resistivity in ohm-meter: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d >= 0.0) {
                sprintf(buf, "%s %.4e", Ekw.Rho(), d);
                break;
            }
            in = prompt(
                "Bad input, reenter resistivity: ", tbuf);
        }
        break;

    case phSigma:
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt("Enter conductivity in Si/meter: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d > 0.0) {
                sprintf(buf, "%s %.4e", Ekw.Sigma(), d);
                break;
            }
            in = prompt(
                "Bad input, reenter conductivity: ", tbuf);
        }
        break;

    case phRsh:
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt("Enter sheet resistance ohms per square: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d >= 0.0) {
                sprintf(buf, "%s %.4e", Ekw.Rsh(), d);
                break;
            }
            in = prompt(
                "Bad input, reenter sheet resistance: ", tbuf);
        }
        break;

    case phEpsRel:
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt("Enter relative dielectric constant: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d >= 0.0) {
                sprintf(buf, "%s %.4f", Ekw.EpsRel(), d);
                break;
            }
            in = prompt(
                "Bad input, reenter dielectric constant: ", tbuf);
        }
        break;

    case phCapacitance:
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt("Enter per-area value: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d >= 0.0) {
                sprintf(buf, "%s %.4e", Ekw.Capacitance(), d);
                break;
            }
            in = prompt(
                "Bad input, reenter per-area value: ", tbuf);
        }

        nexttok(&orig, tbuf, false);
        in = prompt("Enter optional per-perimeter-length value: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            in = lstring::strip_space(in);
            if (!*in)
                break;
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d >= 0.0) {
                sprintf(buf + strlen(buf), " %.4e", d);
                break;
            }
            in = prompt(
                "Bad input, reenter per-perimeter value: ", tbuf);
        }
        break;

    case phLambda:
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt("Enter penetration depth in microns: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d >= 0.0) {
                sprintf(buf, "%s %.4e", Ekw.Lambda(), d);
                break;
            }
            in = prompt(
                "Bad input, reenter penetration depth: ", tbuf);
        }
        break;

    case phTline:
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt("Enter assumed ground plane layer name: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            char *tok = lstring::gettok(&in);
            if (tok) {
                sprintf(buf, "%s %s", Ekw.Tline(), tok);
                delete [] tok;
                break;
            }
            in = prompt(
                "Bad input, reenter ground plane layer name: ", tbuf);
        }

        nexttok(&orig, tbuf, false);
        in = prompt("Enter assumed dielectric thickness in microns: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d >= 0.0) {
                sprintf(buf + strlen(buf), " %.4f", d);
                break;
            }
            in = prompt(
                "Bad input, reenter dielectric thickness: ", tbuf);
        }

        nexttok(&orig, tbuf, false);
        in = prompt("Enter assumed relative dielectric constant: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d >= 0.0) {
                sprintf(buf + strlen(buf), " %.4f", d);
                break;
            }
            in = prompt(
                "Bad input, reenter dielectric constant: ", tbuf);
        }
        break;

    case phAntenna:
        nexttok(&orig, tbuf, false);
        nexttok(&orig, tbuf, false);
        in = prompt("Enter antenna ratio threshold: ", tbuf);
        for (;;) {
            if (!in)
                return (0);
            double d;
            if (sscanf(in, "%lf", &d) == 1 && d >= 0.0) {
                sprintf(buf, "%s %g", Ekw.Antenna(), d);
                break;
            }
            in = prompt(
                "Bad input, reenter antenna ratio: ", tbuf);
        }
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
phKW
phyKWstruct::kwtype(const char *str)
{
    char *tok = lstring::gettok(&str);
    phKW ret = phNil;
    if (tok) {
        if (lstring::cieq(tok, Ekw.Planarize()))
            ret = phPlanarize;
        else if (lstring::cieq(tok, Ekw.Thickness()))
            ret = phThickness;
        else if (lstring::cieq(tok, Ekw.FH_nhinc()))
            ret = phFH_nhinc;
        else if (lstring::cieq(tok, Ekw.FH_rh()))
            ret = phFH_rh;
        else if (lstring::cieq(tok, Ekw.Rho()))
            ret = phRho;
        else if (lstring::cieq(tok, Ekw.Sigma()))
            ret = phSigma;
        else if (lstring::cieq(tok, Ekw.Rsh()))
            ret = phRsh;
        else if (lstring::cieq(tok, Ekw.EpsRel()))
            ret = phEpsRel;
        else if (lstring::cieq(tok, Ekw.Capacitance()) ||
                lstring::cieq(tok, Ekw.Cap()))
            ret = phCapacitance;
        else if (lstring::cieq(tok, Ekw.Lambda()))
            ret = phLambda;
        else if (lstring::cieq(tok, Ekw.Tline()))
            ret = phTline;
        else if (lstring::cieq(tok, Ekw.Antenna()))
            ret = phAntenna;
    }
    delete [] tok;
    return (ret);
}


// Static function.
// Create a string containing the physical attribute keywords and
// settings.
//
char *
phyKWstruct::get_settings(const CDl *ld)
{
    if (!ld)
        return (lstring::copy("\n"));
    char buf[256];
    *buf = '\0';
    sLstr lstr;

    DspLayerParams *dp = dsp_prm(ld);
    TechLayerParams *lp = tech_prm(ld);

    if (ld->isPlanarizingSet()) {
        lstr.add(Ekw.Planarize());
        lstr.add_c(' ');
        lstr.add(ld->isPlanarizing() ? "yes" : "no");
        lstr.add_c('\n');
    }
    if (dp->thickness() > 0.0) {
        int ndgt = CD()->numDigits();
        sprintf(buf, "%s %.*f\n", Ekw.Thickness(), ndgt, dp->thickness());
        lstr.add(buf);
    }
    if (lp->fh_nhinc() > 1) {
        sprintf(buf, "%s %d\n", Ekw.FH_nhinc(), lp->fh_nhinc());
        lstr.add(buf);
    }
    if (lp->fh_rh() > 0.0 && lp->fh_rh() != 2.0) {
        sprintf(buf, "%s %g\n", Ekw.FH_rh(), lp->fh_rh());
        lstr.add(buf);
    }
    if (lp->rho() > 0.0) {
        sprintf(buf, "%s %g\n", Ekw.Rho(), lp->rho());
        lstr.add(buf);
    }
    if (lp->ohms_per_sq() > 0.0) {
        sprintf(buf, "%s %g\n", Ekw.Rsh(), lp->ohms_per_sq());
        lstr.add(buf);
    }
    if (lp->epsrel() > 0.0) {
        sprintf(buf, "%s %g\n", Ekw.EpsRel(), lp->epsrel());
        lstr.add(buf);
    }
    if (lp->cap_per_area() > 0.0 || lp->cap_per_perim() > 0.0) {
        sprintf(buf, "%s %g %g\n", Ekw.Capacitance(), lp->cap_per_area(),
            lp->cap_per_perim());
        lstr.add(buf);
    }
    if (lp->lambda() > 0.0) {
        sprintf(buf, "%s %g\n", Ekw.Lambda(), lp->lambda());
        lstr.add(buf);
    }
    if (lp->gp_lname() && *lp->gp_lname()) {
        int ndgt = CD()->numDigits();
        sprintf(buf, "%s %s %.*f %g\n", Ekw.Tline(), lp->gp_lname(),
            ndgt, lp->diel_thick(), lp->diel_const());
        lstr.add(buf);
    }
    if (lp->ant_ratio() > 0) {
        sprintf(buf, "%s %g\n", Ekw.Antenna(), lp->ant_ratio());
        lstr.add(buf);
    }

    lstr.add_c('\n');
    char *str = lstr.string_trim();
    return (str);
}


// Error/return flags used below.
//
#define ELP_ERR  0x1        // Badness
#define ELP_PL   0x2        // Planarize
#define ELP_TH   0x4        // Thickness
#define ELP_FN   0x8        // FH_nhinc
#define ELP_FR   0x10       // FH_rh
#define ELP_RH   0x20       // Rho
#define ELP_SG   0x40       // Sigma
#define ELP_R    0x80       // Rsh
#define ELP_EP   0x100      // EpsRel
#define ELP_CA   0x200      // Capacitance
#define ELP_LA   0x400      // Lambda
#define ELP_TR   0x800      // Tline
#define ELP_AT   0x1000     // Antenna

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
            while (*s == ' ')
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

        if (lstring::cieq(kwbuf, Ekw.Planarize())) {
            bool plz = Tech()->GetBoolean(inbuf);
            ld->setPlanarizing(true, plz);
            ret |= ELP_PL;
        }
        else if (lstring::cieq(kwbuf, Ekw.Thickness())) {
            if (flags & (ELP_TH))
                return (ELP_TH | ELP_ERR);
            double p0;
            if (sscanf(inbuf, "%lf", &p0) != 1 || p0 < 0)
                return (ELP_TH | ELP_ERR);
            dsp_prm(ld)->set_thickness(p0);
            ret |= ELP_TH;
        }
        else if (lstring::cieq(kwbuf, Ekw.FH_nhinc())) {
            if (flags & (ELP_FN))
                return (ELP_FN | ELP_ERR);
            int n;
            if (sscanf(inbuf, "%d", &n) != 1 || n < 1)
                return (ELP_FN | ELP_ERR);
            tech_prm(ld)->set_fh_nhinc(n);
            ret |= ELP_FN;
        }
        else if (lstring::cieq(kwbuf, Ekw.FH_rh())) {
            if (flags & (ELP_FR))
                return (ELP_FR | ELP_ERR);
            double p0;
            if (sscanf(inbuf, "%lf", &p0) != 1 || p0 <= 0.0)
                return (ELP_FR | ELP_ERR);
            tech_prm(ld)->set_fh_rh(p0);
            ret |= ELP_FR;
        }
        else if (lstring::cieq(kwbuf, Ekw.Rho())) {
            if (flags & (ELP_RH | ELP_SG))
                return (ELP_RH | ELP_ERR);
            double p0;
            if (sscanf(inbuf, "%lf", &p0) != 1 || p0 < 0)
                return (ELP_RH | ELP_ERR);
            tech_prm(ld)->set_rho(p0);
            ret |= ELP_RH;
        }
        else if (lstring::cieq(kwbuf, Ekw.Sigma())) {
            if (flags & (ELP_RH | ELP_SG))
                return (ELP_SG | ELP_ERR);
            double p0;
            if (sscanf(inbuf, "%lf", &p0) != 1 || p0 <= 0)
                return (ELP_SG | ELP_ERR);
            tech_prm(ld)->set_rho(1.0/p0);
            ret |= ELP_SG;
        }
        else if (lstring::cieq(kwbuf, Ekw.Rsh())) {
            double p0;
            if (flags & ELP_R)
                return (ELP_R | ELP_ERR);
            if (sscanf(inbuf, "%lf", &p0) < 1 || p0 < 0)
                return (ELP_R | ELP_ERR);
            tech_prm(ld)->set_ohms_per_sq(p0);
            ret |= ELP_R;
        }
        else if (lstring::cieq(kwbuf, Ekw.EpsRel())) {
            if (flags & ELP_EP)
                return (ELP_EP | ELP_ERR);
            double p0;
            if (sscanf(inbuf, "%lf", &p0) != 1 || p0 < 1.0)
                return (ELP_EP | ELP_ERR);
            tech_prm(ld)->set_epsrel(p0);
            ret |= ELP_EP;
        }
        else if (lstring::cieq(kwbuf, Ekw.Cap()) ||
                lstring::cieq(kwbuf, Ekw.Capacitance())) {
            double p0, p1;
            if (flags & ELP_CA)
                return (ELP_CA | ELP_ERR);
            int i = sscanf(inbuf, "%lf %lf", &p0, &p1);
            if ((i == 2 && p0 >= 0.0 && p1 >= 0.0) || (i == 1 && p0 >= 0.0)) {
                tech_prm(ld)->set_cap_per_area(p0);
                if (i == 2)
                    tech_prm(ld)->set_cap_per_perim(p1);
                else
                    tech_prm(ld)->set_cap_per_perim(0.0);
            }
            else
                return (ELP_CA | ELP_ERR);
            ret |= ELP_CA;
        }
        else if (lstring::cieq(kwbuf, Ekw.Lambda())) {
            if (flags & ELP_LA)
                return (ELP_LA | ELP_ERR);
            double p0;
            if (sscanf(inbuf, "%lf", &p0) != 1 || p0 < 0)
                return (ELP_LA | ELP_ERR);
            tech_prm(ld)->set_lambda(p0);
            ret |= ELP_LA;
        }
        else if (lstring::cieq(kwbuf, Ekw.Tline())) {
            double p0, p1;
            char tbuf[64];
            if (flags & ELP_TR)
                return (ELP_TR | ELP_ERR);
            if (sscanf(inbuf, "%s %lf %lf", tbuf, &p0, &p1) < 3)
                return (ELP_TR | ELP_ERR);
            if (p0 > 0.0 && p0 < 10.0 && p1 >= 1.0 && p1 < 100.0) {
                delete [] tech_prm(ld)->gp_lname();
                tech_prm(ld)->set_gp_lname(lstring::copy(tbuf));
                tech_prm(ld)->set_diel_thick(p0);
                tech_prm(ld)->set_diel_const(p1);
            }
            else
                return (ELP_TR | ELP_ERR);
            ret |= ELP_TR;
        }
        else if (lstring::cieq(kwbuf, Ekw.Antenna())) {
            if (flags & ELP_AT)
                return (ELP_AT | ELP_ERR);
            double p0;
            if (sscanf(inbuf, "%lf", &p0) != 1 || p0 < 0)
                return (ELP_AT | ELP_ERR);
            tech_prm(ld)->set_ant_ratio(p0);
            ret |= ELP_AT;
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

                DspLayerParams *dp = dsp_prm(ld);
                TechLayerParams *lp = tech_prm(ld);

                if (ld->isPlanarizingSet())
                    mask |= CDL_PLANARIZE;
                ld->setPlanarizingSet(false);

                thickness = dp->thickness();
                dp->set_thickness(0.0);
                fh_nhinc = lp->fh_nhinc();
                lp->set_fh_nhinc(1);
                fh_rh = lp->fh_rh();
                lp->set_fh_rh(2.0);

                rho = lp->rho();
                lp->set_rho(0.0);

                ohms_per_sq = lp->ohms_per_sq();
                lp->set_ohms_per_sq(0.0);

                epsrel = lp->epsrel();
                lp->set_epsrel(0.0);

                cap_per_area = lp->cap_per_area();
                lp->set_cap_per_area(0.0);
                cap_per_perim = lp->cap_per_perim();
                lp->set_cap_per_perim(0.0);

                lambda = lp->lambda();
                lp->set_lambda(0.0);

                diel_thick = lp->diel_thick();
                lp->set_diel_thick(0.0);
                diel_const = lp->diel_const();
                lp->set_diel_const(0.0);

                ant_ratio = lp->ant_ratio();
                lp->set_ant_ratio(0.0);

                plzasset = ld->isPlanarizing();

                ldesc = ld;
            }

        void revert()
            {
                if (!ldesc)
                    return;

                DspLayerParams *dp = dsp_prm(ldesc);
                TechLayerParams *lp = tech_prm(ldesc);

                ldesc->setPlanarizingSet(false);
                if (mask & CDL_PLANARIZE)
                    ldesc->setPlanarizing(true, plzasset);

                dp->set_thickness(thickness);
                lp->set_fh_nhinc(fh_nhinc);
                lp->set_fh_rh(fh_rh);

                lp->set_rho(rho);
                lp->set_ohms_per_sq(ohms_per_sq);
                lp->set_epsrel(epsrel);
                lp->set_cap_per_area(cap_per_area);
                lp->set_cap_per_perim(cap_per_perim);
                lp->set_lambda(lambda);
                lp->set_diel_thick(diel_thick);
                lp->set_diel_const(diel_const);
                lp->set_ant_ratio(ant_ratio);
            }

    private:
        double thickness;
        int fh_nhinc;
        double fh_rh;
        double rho;
        double ohms_per_sq;
        double epsrel;
        double cap_per_area;
        double cap_per_perim;
        double lambda;
        double diel_thick;
        double diel_const;
        double ant_ratio;
        CDl *ldesc;
        int mask;
        bool plzasset;
    };
}


// Static function.
// Establish the extraction settings from the string, returning a status
// message (must free) if error.
//
char *
phyKWstruct::set_settings(CDl *ld, const char *string)
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
                if (result & (ELP_R | ELP_CA | ELP_TR)) {
                    if (flags & (ELP_R | ELP_CA | ELP_TR))
                        msg = "Retry: inappropriate electrical keyword line %d";
                    else
                        msg = "Retry: bad input line %d";
                }
                else if (result &
                        (ELP_TH | ELP_FN | ELP_FR | ELP_RH | ELP_SG | ELP_EP |
                        ELP_LA | ELP_AT)) {
                    if (flags & (ELP_TH | ELP_FN | ELP_FR | ELP_RH | ELP_SG |
                            ELP_EP | ELP_LA | ELP_AT))
                        msg =
                    "Retry: inappropriate physical property keyword line %d";
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
        }
    }
    return (0);
}

