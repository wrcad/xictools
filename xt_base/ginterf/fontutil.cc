
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
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "fontutil.h"
#include "lstring.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


const char *
GRfont::getDefaultName(int fnum)
{
    if (fnum > 0 && fnum < num_app_fonts)
        return (app_fonts[fnum].default_fontname);
    return (0);
}


const char *
GRfont::getLabel(int fnum)
{
    if (fnum > 0 && fnum < num_app_fonts)
        return (app_fonts[fnum].label);
    return (0);
}


bool
GRfont::isFixed(int fnum)
{
    if (fnum > 0 && fnum < num_app_fonts)
        return (app_fonts[fnum].fixed);
    return (false);
}


bool
GRfont::isFamilyOnly(int fnum)
{
    if (fnum > 0 && fnum < num_app_fonts)
        return (app_fonts[fnum].family_only);
    return (false);
}


namespace { bool is_style_keyword(const char*); }

// Parse a font string in the form "face [style keywords] [size]",
// returning the family name, size, and style keyword list.  If no
// size is found, def_size is returned.  If style is null, style
// keywords are discarded.
//
void
GRfont::parse_freeform_font_string(const char *string, char **family,
    stringlist **style, int *sz, int def_size)
{
    char *strtofree = 0;
    if (xfd_t::is_xfd(string)) {
        xfd_t f(string);
        if (!f.get_pixsize() || !isdigit(*f.get_pixsize()))
            f.set_pixsize(def_size > 0 ? def_size : 12);
        strtofree = f.font_freeform();
        string = strtofree;
    }

    *family = 0;
    if (style)
        *style = 0;
    // Tokenize the string, in reverse order.
    stringlist *s0 = 0;
    char *tok;
    while ((tok = lstring::gettok(&string)) != 0)
        s0 = new stringlist(tok, s0);

    // The first (actually last) token should be the size.  If so,
    // save the size and advance s0.
    stringlist *sx = 0;
    if (s0 && isdigit(*s0->string)) {
        *sz = (int)(atof(s0->string) + 0.5);
        stringlist *st = s0;
        s0 = s0->next;
        st->next = sx;
        sx = st;
    }
    else
        // no size found, set a default
        *sz = def_size;
    stringlist::destroy(sx);

    // Link any style keywords.
    sx = 0;
    while (s0) {
        if (is_style_keyword(s0->string)) {
            stringlist *st = s0;
            s0 = s0->next;
            st->next = sx;
            sx = st;
            continue;
        }
        break;
    }
    if (style)
        *style = sx;
    else
        stringlist::destroy(sx);

    // Throw out the trash and reverse the remaining list.
    sx = 0;
    while (s0) {
        stringlist *st = s0;
        s0 = s0->next;
        st->next = sx;
        sx = st;
    }

    // sx contains the family name tokens in correct order.
    char buf[256];
    char *t = buf;
    *t = 0;
    for (stringlist *st = sx; st; st = st->next) {
        if (t != buf)
            *t++ = ' ';
        strcpy(t, st->string);
        for ( ; *t; t++) ;
    }
    stringlist::destroy(sx);
    if (buf[0])
        *family = lstring::copy(buf);
    delete [] strtofree;
}


//
// Methods for font name parsers, and related.
//

namespace {
    // List of style keywords.
    //
    const char *style_keywords[] =
    {
        // styles
        "Oblique",
        "Italic",

        // variants
        "Small-Caps",

        // weights
        "Ultra-Light",
        "Light",
        "Medium",
        "Semi-Bold",
        "Bold",
        "Ultra-Bold",
        "Heavy",

        // stretch
        "Ultra-Condensed",
        "Extra-Condensed",
        "Condensed",
        "Semi-Condensed",
        "Semi-Expanded",
        "Expanded",
        "Extra-Expanded",
        "Ultra-Expanded",
        0
    };


    // Return true if word matches a style keyword.
    //
    bool
    is_style_keyword(const char *word)
    {
        for (const char **s = style_keywords; *s; s++) {
            if (!strcasecmp(*s, word))
                return (true);
        }
        return (false);
    }


    // Tokenizer for XFD names.  The token starts with '-' and ends with
    // the next '-' or end or string.  The return is the text not
    // including the hyphen, which will be an empty string in the case of
    // "--" or "-<eos>".  The return is null if the leading character is
    // not '-'.
    //
    // If nohy is true, the leading hyphen is optional, which can be used
    // to parse the first token in case the leading '-' is omitted from
    // the description string (is it was for gtkhtm font family
    // descriptions).
    //
    char *
    xfd_field(const char **sp, bool nohy = false)
    {
        if (**sp == '-') {
            (*sp)++;
            const char *s = *sp;
            while (**sp && **sp != '-')
                (*sp)++;
            int n = *sp - s;
            char *t = new char[n + 1];
            strncpy(t, s, n);
            t[n] = 0;
            return (t);
        }
        else if (nohy) {
            const char *s = *sp;
            while (**sp && **sp != '-')
                (*sp)++;
            int n = *sp - s;
            char *t = new char[n + 1];
            strncpy(t, s, n);
            t[n] = 0;
            return (t);
        }
        return (0);
    }
}


// (static function)
// Return true if fname looks like an X font description.
//
bool
xfd_t::is_xfd(const char *fname)
{
    if (strchr(fname, '*'))
        return (true);
    if (!strcasecmp(fname, "fixed"))
        return (true);
    int n = 0;
    for (const char *s = fname; *s; s++) {
        if (*s == '-')
            n++;
    }
    return (n > 2);  // Can be as few as 3 in gtkthm family descr.
}


// Constructor, parse and set the descriptor fields.  An error is indicated
// if the family name is returned nil.
//
xfd_t::xfd_t(const char *fontname)
{
    if (!fontname)
        fontname = "";
    foundry = 0;
    family = 0;
    weight = 0;
    slant = 0;
    width = 0;
    style = 0;
    pixsize = 0;
    pointsz = 0;
    resol_x = 0;
    resol_y = 0;
    spacing = 0;
    avgwid = 0;
    charset = 0;
    encoding = 0;
    was_xfd = is_xfd(fontname);
    was_fixed = false;

    if (!was_xfd) {
        // Doesn't look like an XFD, assume freeform
        int sz;
        GRfont::parse_freeform_font_string(fontname, &family, 0, &sz);
        if (family) {
            pixsize = new char[16];
            sprintf(pixsize, "%d", sz);
            if (!strcasecmp(family, "Sans")) {
                delete [] family;
                family = lstring::copy("helvetica");
            }
            else if (!strcasecmp(family, "Serif")) {
                delete [] family;
                family = lstring::copy("times");
            }
            else if (!strcasecmp(family, "Monospace")) {
                delete [] family;
                family = lstring::copy("fixed");
            }
        }
    }
    else {
        if (!strcasecmp(fontname, "fixed")) {
            family = lstring::copy(fontname);
            was_fixed = true;
            return;
        }
        encoding = 0;
        if (*fontname++ == '-') {
            foundry = xfd_field(&fontname, true);
            family = xfd_field(&fontname);
            weight = xfd_field(&fontname);
            slant = xfd_field(&fontname);
            width = xfd_field(&fontname);
            if (!width && foundry && family && weight && slant) {
                // must have been a family description string
                // -foundry-family-width-spacing
                width = weight;
                weight = 0;
                spacing = slant;
                slant = 0;
                return;
            }
            style = xfd_field(&fontname);
            if (!style && foundry && family && weight && slant && width) {
                // must have been a gtkhtm family description string
                // -foundry-family-width-spacing-pixsize
                pixsize = width;
                width = weight;
                weight = 0;
                spacing = slant;
                slant = 0;
                return;
            }
            pixsize = xfd_field(&fontname);
            pointsz = xfd_field(&fontname);
            resol_x = xfd_field(&fontname);
            resol_y = xfd_field(&fontname);
            spacing = xfd_field(&fontname);
            avgwid = xfd_field(&fontname);
            charset = xfd_field(&fontname);
            encoding = xfd_field(&fontname);
        }
        if (!encoding) {
            delete [] foundry;
            delete [] family;
            delete [] weight;
            delete [] slant;
            delete [] width;
            delete [] style;
            delete [] pixsize;
            delete [] pointsz;
            delete [] resol_x;
            delete [] resol_y;
            delete [] spacing;
            delete [] avgwid;
            delete [] charset;
            foundry = 0;
            family = 0;
            weight = 0;
            slant = 0;
            width = 0;
            style = 0;
            pixsize = 0;
            pointsz = 0;
            resol_x = 0;
            resol_y = 0;
            spacing = 0;
            avgwid = 0;
            charset = 0;
        }
    }
}


xfd_t::~xfd_t()
{
    delete [] foundry;
    delete [] family;
    delete [] weight;
    delete [] slant;
    delete [] width;
    delete [] style;
    delete [] pixsize;
    delete [] pointsz;
    delete [] resol_x;
    delete [] resol_y;
    delete [] spacing;
    delete [] avgwid;
    delete [] charset;
    delete [] encoding;
}


// Set the foundry field, or revert to "*" if passed null.
//
void
xfd_t::set_foundry(const char *val)
{
    delete [] foundry;
    foundry = lstring::copy(val ? val : "*");
}


// Set the family field, or revert to "*" if passed null.
//
void
xfd_t::set_family(const char *val)
{
    delete [] family;
    family = lstring::copy(val ? val : "*");
}


// Set the weight field, almost always "medium" or "bold", but "regular"
// and "demibold" are also possible.  Reverts to "medium" if passed null.
//
void
xfd_t::set_weight(const char *val)
{
    delete [] weight;
    weight = lstring::copy(val ? val : "medium");
}


// Set the slant field to the argument ("i" (italic) or "o" (oblique))
// if given, return to "r" (roman) if null.
//
void
xfd_t::set_slant(const char *val)
{
    delete [] slant;
    slant = lstring::copy(val ? val : "r");
}


// Set the width field to the argument, e.g., "normal", "semicondensed".
//
void
xfd_t::set_width(const char *val)
{
    delete [] width;
    width = lstring::copy(val ? val : "normal");
}


// Set the pixel size field.  If sz is 0 or negative, set to "*".
//
void
xfd_t::set_pixsize(int sz)
{
    delete [] pixsize;
    char buf[32];
    if (sz > 0) {
        sprintf(buf, "%d", sz);
        delete [] pointsz;
        pointsz = lstring::copy("*");
    }
    else
        strcpy(buf, "*");
    pixsize = lstring::copy(buf);
}


// Set the point size field.  If sz is 0 or negative, set to "*".
//
void
xfd_t::set_pointsz(int sz)
{
    delete [] pointsz;
    char buf[32];
    if (sz > 0) {
        sprintf(buf, "%d", sz);
        delete [] pixsize;
        pixsize = lstring::copy("*");
    }
    else
        strcpy(buf, "*");
    pointsz = lstring::copy(buf);
}


// Return an XFD for the font.  If the original font name was "fixed",
// that name will be returned, unless nofixed is set.
//
char *
xfd_t::font_xfd(bool nofixed)
{
    if (was_fixed && !nofixed)
        return (lstring::copy("fixed"));

    char buf[256];
    char *t = buf;

    *t++ = '-';
    if (foundry && *foundry) {
        strcpy(t, foundry);     // foundry
        for ( ; *t; t++) ;
    }
    else
        *t++ = '*';
    *t++ = '-';

    if (family && *family) {
        strcpy(t, family);      // family
        for ( ; *t; t++) ;
    }
    else
        *t++ = '*';
    *t++ = '-';

    if (weight && *weight) {
        strcpy(t, weight);      // weight
        for ( ; *t; t++) ;
    }
    else {
        strcpy(t, "medium");
        for ( ; *t; t++) ;
    }
    *t++ = '-';

    if (slant && *slant) {
        strcpy(t, slant);       // slant
        for ( ; *t; t++) ;
    }
    else
        *t++ = 'r';
    *t++ = '-';

    if (width && *width) {
        strcpy(t, width);       // width
        for ( ; *t; t++) ;
    }
    else {
        strcpy(t, "normal");
        for ( ; *t; t++) ;
    }
    *t++ = '-';

    if (style && *style) {
        strcpy(t, style);       // style
        for ( ; *t; t++) ;
    }
    else
        *t++ = '*';
    *t++ = '-';

    if (pixsize && *pixsize) {
        strcpy(t, pixsize);     // pixsize
        for ( ; *t; t++) ;
    }
    else
        *t++ = '*';
    *t++ = '-';

    if (pointsz && *pointsz) {
        strcpy(t, pointsz);     // pointsz
        for ( ; *t; t++) ;
    }
    else
        *t++ = '*';
    *t++ = '-';

    if (resol_x && *resol_x) {
        strcpy(t, resol_x);     // resol_x
        for ( ; *t; t++) ;
    }
    else
        *t++ = '*';
    *t++ = '-';

    if (resol_y && *resol_y) {
        strcpy(t, resol_y);     // resol_y
        for ( ; *t; t++) ;
    }
    else
        *t++ = '*';
    *t++ = '-';

    if (spacing && *spacing) {
        strcpy(t, spacing);     // spacing
        for ( ; *t; t++) ;
    }
    else
        *t++ = '*';
    *t++ = '-';

    if (avgwid && *avgwid) {
        strcpy(t, avgwid);      // avgwid
        for ( ; *t; t++) ;
    }
    else
        *t++ = '*';
    *t++ = '-';

    if (charset && *charset) {
        strcpy(t, charset);     // charset
        for ( ; *t; t++) ;
    }
    else {
        strcpy(t, "iso8859");
        for ( ; *t; t++) ;
    }
    *t++ = '-';

    if (encoding && *encoding)
        strcpy(t, encoding);    // encoding
    else {
        *t++ = '*';
        *t = 0;
    }

    return (lstring::copy(buf));
}


// Return a freeform font description string for the font.
//
char *
xfd_t::font_freeform()
{
    char buf[256];
    char *t = buf;

    if (family && *family) {
        strcpy(t, family);
        for ( ; *t; t++) ;
    }
    else {
        strcpy(t, "times");
        for ( ; *t; t++) ;
    }
    *t++ = ' ';
    if (pixsize && *pixsize) {
        strcpy(t, pixsize);
        for ( ; *t; t++) ;
    }
    else
        sprintf(t, "%d", 12);
    return (lstring::copy(buf));
}


// Return the XFD family name.  If append is true, append -pixsize,
// for use in the gtkhtm help viewer.  If psz, return the pixel size
// in the pointer.
//
char *
xfd_t::family_xfd(int *psz, bool append)
{
    char buf[256];
    char *t = buf;

    *t++ = '-';
    if (foundry && *foundry) {
        strcpy(t, foundry);     // foundry
        for ( ; *t; t++) ;
    }
    else
        *t++ = '*';
    *t++ = '-';

    if (family && *family) {
        strcpy(t, family);      // family
        for ( ; *t; t++) ;
    }
    else
        *t++ = '*';
    *t++ = '-';

    if (width && *width) {
        strcpy(t, width);       // width
        for ( ; *t; t++) ;
    }
    else {
        strcpy(t, "normal");
        for ( ; *t; t++) ;
    }
    *t++ = '-';

    if (spacing && *spacing) {
        strcpy(t, spacing);     // spacing
        for ( ; *t; t++) ;
    }
    else {
        *t++ = '*';
        *t = 0;
    }

    if (psz || append) {
        int sz = 0;
        if (pixsize && isdigit(*pixsize))
            sz = atoi(pixsize);
        else if (pointsz && isdigit(*pointsz))
            sz = atoi(pointsz)/10;
        if (sz <= 0)
            sz = 14;
        if (psz)
            *psz = sz;
        if (append)
            sprintf(t, "-%d", sz);
    }
    return (lstring::copy(buf));
}
// End of xfd_t functions

