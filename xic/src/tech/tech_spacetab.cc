
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

#include "cd.h"
#include "tech_spacetab.h"
#include "tech_kwords.h"
#include <algorithm>


namespace {
    inline bool st_cmp(const sTspaceTable &t1, const sTspaceTable &t2)
    {
        if (t1.width < t2.width)
            return (true);
        return (t1.width == t2.width && t1.length < t2.length);
    }
}


// Static function.
// Do some very basic testing and make sure that the elements are
// sorted correctly in ascending width, then length.  Return false if
// the table is no good.
//
bool
sTspaceTable::check_sort(sTspaceTable *thisst)
{
    if (!thisst)
        return (false);

    // The first record (this) is a dummy, the entries field contains
    // the number of records (including the first), the dimen field
    // contains the default spacing.  The width is the dimensionality
    // (1 or 2) and length contains flags from Virtuoso.

    if (thisst->entries < 2)
        return (false);
    if (thisst->width != 1 && thisst->width != 2)
        return (false);
    if (thisst->entries > 2)
        std::sort(thisst + 1, thisst + thisst->entries, st_cmp);
    return (true);
}


// Static function.
// Compose a string containing the Lisp record describing the table. 
// This is used when creating a Virtuoso-compatible tech file.  The
// argument contains a line prefix which is prepended to each line of
// text.  Return null on error.
//
char *
sTspaceTable::to_lisp_string(const sTspaceTable *thisst, const char *pref,
    const char *lname, const char *lname2)
{
    char buf[256];
    sLstr lstr;
    const sTspaceTable *tab = thisst;
    if (!tab)
        return (0);
    int dims = tab->width;
    if (dims != 1 && dims != 2)
        return (0);
    if (!lname)
        return (0);
    lstr.add(pref);
    lstr.add("( ");
    lstr.add("minSpacing      \"");
    lstr.add(lname);
    if (lname2) {
        lstr.add("\" \"");
        lstr.add(lname2);
    }
    lstr.add("\"\n");
    lstr.add(pref);
    if (dims == 1)
        lstr.add("  (( \"width\"   nil  nil )");
    else if (tab->length & STF_WIDWID) {
        lstr.add(
            "  (( \"width\"   nil  nil  \"width\"   nil   nil  )");
    }
    else if (tab->length & STF_TWOWID) {
        lstr.add(
            "  (( \"twoWidth\"   nil  nil  \"width\"   nil   nil  )");
    }
    else {
        lstr.add(
            "  (( \"width\"   nil  nil  \"length\"   nil   nil  )");
    }
    unsigned int flags = tab->length;
    bool h = flags & STF_HORIZ;
    bool v = flags & STF_VERT;
    if (h && !v)
        lstr.add(" 'horizontal");
    else if (!h && v)
        lstr.add(" 'vertical");
    if (flags & STF_SAMEMTL)
        lstr.add(" 'sameMetal");
    else {
        if (flags & STF_SAMENET)
            lstr.add(" 'sameNet");
        if (flags & STF_PGNET)
            lstr.add(" 'PGNet");
    }
    if (tab->dimen != 0) {
        lstr.add_c(' ');
        lstr.add_g(MICRONS(tab->dimen));
    }
    lstr.add(" )\n");
    lstr.add(pref);
    lstr.add("  (\n");
    tab++;
    int ents = tab->entries;
    for (int i = 0; i < ents; i++) {
        lstr.add(pref);
        if (dims == 1) {
            sprintf(buf, "    %-12g %12g\n",
                MICRONS(tab->width), MICRONS(tab->dimen));
        }
        else {
            sprintf(buf, "    (%-12g %-12g) %12g\n",
                MICRONS(tab->width), MICRONS(tab->length),
                MICRONS(tab->dimen));
        }
        lstr.add(buf);
        tab++;
    }
    lstr.add(pref);
    lstr.add("  )\n");
    lstr.add(pref);
    lstr.add(")\n");
    return (lstr.string_trim());
}


// Static function.
// Write the spacing table description to fp or lstr, in the format
// used in the Xic technology file.
//
void
sTspaceTable::tech_print(const sTspaceTable *thisst, FILE *fp, sLstr *lstr)
{
    const sTspaceTable *t = thisst;
    if (!t)
        return;
    if (lstr) {
        lstr->add(Tkw.SpacingTable());
        lstr->add_c(' ');
        lstr->add_d(MICRONS(t->dimen), 4);
        lstr->add_c(' ');
        lstr->add_i(t->width);  // dimensions, 1 or 2
        lstr->add_c(' ');
        lstr->add_h(t->length, true); // flags
        lstr->add_c(' ');
        lstr->add_i(t->entries - 1);
        lstr->add("\\\n");
        unsigned int dims = t->width;
        t++;
        int nent = t->entries;
        for (int i = 0; i < nent; i++) {
            lstr->add("    ");
            lstr->add_d(MICRONS(t->width), 4);
            if (dims == 2) {
                lstr->add_c(' ');
                lstr->add_d(MICRONS(t->length), 4);
            }
            lstr->add_c(' ');
            lstr->add_d(MICRONS(t->dimen), 4);
            lstr->add("\\\n");
            t++;
        }
    }
    else if (fp) {
        fprintf(fp, "%s %.4f %d 0x%x %d \\\n", Tkw.SpacingTable(),
            MICRONS(t->dimen), t->width, t->length, t->entries - 1);
        unsigned int dims = t->width;
        t++;
        int nent = t->entries;
        for (int i = 0; i < nent; i++) {
            if (dims == 2) {
                fprintf(fp, "    %.4f %.4f %.4f \\\n", MICRONS(t->width),
                    MICRONS(t->length), MICRONS(t->dimen));
            }
            else {
                fprintf(fp, "    %.4f %.4f \\\n", MICRONS(t->width),
                    MICRONS(t->dimen));
            }
            t++;
        }
    }
}


// Static function.
// Parse a spacing table definition as found in the Xic technology
// file.  Create and return a new table.  The passed text pointer
// address is incremented to the end of the table definition on
// success.
//
sTspaceTable *
sTspaceTable::tech_parse(const char **ptext, const char **errm)
{
    const char *tbak = *ptext;
    if (errm)
        *errm = 0;
    char *tok = lstring::gettok(ptext);
    if (!tok)
        return (0);
    if (tok && lstring::cieq(tok, Tkw.SpacingTable())) {
        // Allow optionally the leading keyword.
        delete [] tok;
        tok = lstring::gettok(ptext);
    }

    // The default space value in microns.
    double dw;
    if (!tok || sscanf(tok, "%lf", &dw) != 1) {
        delete [] tok;
        *ptext = tbak;
        *errm = "SpacingTable: missing default spacing";
        return (0);
    }
    delete [] tok;

    // The table dimensionality, 1 or 2.
    tok = lstring::gettok(ptext);
    unsigned int dims;
    if (!tok || sscanf(tok, "%u", &dims) != 1 ||
            (dims != 1 && dims != 2)) {
        delete [] tok;
        *ptext = tbak;
        *errm = "SpacingTable: dimension missing or not 1 or 2";
        return (0);
    }
    delete [] tok;

    // A flags integer.
    // Use strtoul here to handle "0x" hex numbers or decimal.
    tok = lstring::gettok(ptext);
    if (!tok) {
        *ptext = tbak;
        *errm = "SpacingTable: flags missing";
        return (0);
    }
    char *end;
    unsigned long flags = strtoul(tok, &end, 0);
    if (end == tok) {
        delete [] tok;
        *ptext = tbak;
        *errm = "SpacingTable: bad flags token";
        return (0);
    }
    delete [] tok;

    // The number of rows in the table.
    tok = lstring::gettok(ptext);
    int n;
    if (!tok || sscanf(tok, "%d", &n) != 1) {
        delete [] tok;
        *ptext = tbak;
        *errm = "SpacingTable: missing or bad row count";
        return (0);
    }
    delete [] tok;

    // Table data, all in microns, two or three numbers per row.
    const char *emsg = "SpacingTable: premature end of table data";
    sTspaceTable *st = new sTspaceTable[n+1];
    st->set(n+1, dims, flags, INTERNAL_UNITS(dw));
    sTspaceTable *s = st + 1;
    while (n > 0) {
        tok = lstring::gettok(ptext);
        double wid;
        if (!tok || sscanf(tok, "%lf", &wid) != 1) {
            delete [] tok;
            delete [] st;
            *ptext = tbak;
            *errm = emsg;
            return (0);
        }
        delete [] tok;
        double len = 0.0;
        if (dims == 2) {
            tok = lstring::gettok(ptext);
            if (!tok || sscanf(tok, "%lf", &len) != 1) {
                delete [] tok;
                delete [] st;
                *ptext = tbak;
                *errm = emsg;
                return (0);
            }
            delete [] tok;
        }
        tok = lstring::gettok(ptext);
        double dim;
        if (!tok || sscanf(tok, "%lf", &dim) != 1) {
            delete [] tok;
            delete [] st;
            *ptext = tbak;
            *errm = emsg;
            return (0);
        }
        delete [] tok;
        s->set(n, INTERNAL_UNITS(wid), INTERNAL_UNITS(len),
            INTERNAL_UNITS(dim));
        s++;
        n--;
    }
    return (st);
}

