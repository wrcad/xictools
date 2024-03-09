
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
 * MOZY html help viewer files                                            *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/*------------------------------------------------------------------------*
 * This file is part of the gtkhtm widget library.  The gtkhtm library
 * was derived from the gtk-xmhtml library by:
 *
 *   Stephen R. Whiteley  <stevew@wrcad.com>
 *   Whiteley Research Inc.
 *------------------------------------------------------------------------*
 *  The gtk-xmhtml widget was derived from the XmHTML library by
 *  Miguel de Icaza  <miguel@nuclecu.unam.mx> and others from the GNOME
 *  project.
 *  11/97 - 2/98
 *------------------------------------------------------------------------*
 * Author:  newt
 * (C)Copyright 1995-1996 Ripley Software Development
 * All Rights Reserved
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *------------------------------------------------------------------------*/

#include "config.h"
#include "htm_widget.h"
#include "htm_font.h"
#include "htm_form.h"
#include "htm_string.h"
#include "htm_parser.h"
#include "htm_format.h"
#include "htm_image.h"
#include "htm_table.h"
#include <time.h>
#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#include "regex/regex.h"
#endif
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>


/*=======================================================================
 =
 =    A stand-alone HTML to ascii text converter
 =
 =======================================================================*/

namespace htm
{
    struct h2text
    {
        h2text() { LineWidth = 80; End = 0; Indent = 4; Verbatim = false;
            Terminate = false; *Linebuf = 0; }

        char *convert(const char*);
        void setLinewidth(int l) { LineWidth = l; }
        void setIndent(int i) { Indent = i; }

    private:
        htmObject *process(htmObject*, sLstr*);
        void add_text(char*, sLstr*);
        void flush_text(sLstr*);
        void reset_buf();

        int LineWidth;
        int End;
        int Indent;
        bool Verbatim;
        bool Terminate;
        char Linebuf[128];
    };
}


char *
htmPlainTextFromHTML(const char *html_text, int linewidth, int indent)
{
    h2text h2;
    h2.setLinewidth(linewidth);
    h2.setIndent(indent);
    return (h2.convert((char*)html_text));
}


char *
h2text::convert(const char *html_text)
{
    htmParser p(0);
    p.setSource(html_text);
    htmObject *mu = p.parse("text/html");

    sLstr lstr;
    reset_buf();
    for (htmObject *m = mu; m; )
        m = process(m, &lstr);
    flush_text(&lstr);
    char *s = lstr.string_trim();
    htmObject::destroy(mu);
    return (s);
}


htmObject *
h2text::process(htmObject *mu, sLstr *lstr)
{
    if (Verbatim) {
        if (mu->id != HT_ZTEXT && mu->id != HT_PRE)
        return (mu->next);
    }
    switch (mu->id) {
    case HT_ZTEXT:
        {
            char *t = mu->element;
            mu->element = expandEscapes(t, false);
            delete [] t;
            add_text(mu->element, lstr);
            if (Terminate) {
                flush_text(lstr);
                lstr->add_c('\n');
                Terminate = false;
            }
        }
        break;
    case HT_H1:
    case HT_H2:
    case HT_H3:
    case HT_H4:
    case HT_H5:
    case HT_H6:
        if (!mu->is_end) {
            flush_text(lstr);
            lstr->add_c('\n');
            Terminate = true;
        }
        break;
    case HT_P:
        flush_text(lstr);
        if (lstr->length() > 1 && *(lstr->string()+lstr->length()-2) != '\n')
            lstr->add_c('\n');
        break;
    case HT_PRE:
        if (!mu->is_end) {
            flush_text(lstr);
            lstr->add_c('\n');
            lstr->add(Linebuf);
            Verbatim = true;
        }
        else {
            flush_text(lstr);
            lstr->add_c('\n');
            Verbatim = false;
        }
        break;
    case HT_BLOCKQUOTE:
        if (!mu->is_end) {
            flush_text(lstr);
            lstr->add_c('\n');
            Indent += 4;
            reset_buf();
        }
        else {
            Indent -= 4;
            flush_text(lstr);
            lstr->add_c('\n');
        }
        break;
    case HT_BR:
        flush_text(lstr);
        break;

    default:
        break;
    }
    return (mu->next);
}


void
h2text::add_text(char *str, sLstr *lstr)
{
    if (!str)
        return;
    if (Verbatim) {
        for (char *t = str; *t; t++) {
            lstr->add_c(*t);
            if (*t == '\n') {
                reset_buf();
                lstr->add(Linebuf);
            }
        }
        return;
    }
    for (char *t = str; *t; t++)
        if (*t == '\n')
            *t = ' ';
    if (Linebuf[Indent] == 0)
        while (isspace(*str))
            str++;
    while (*str) {
        int len = strlen(str);

        if (End + len < LineWidth) {
            strcat(Linebuf, str);
            End += len;
            return;
        }
        else {
            char *s = str + (LineWidth - End) - 1;
            while (!isspace(*s) && s > str)
                s--;
            while (isspace(*s) && s > str)
                s--;
            if (s != str) {
                *++s = 0;
                strcat(Linebuf, str);
            }
            lstr->add(Linebuf);
            lstr->add_c('\n');
            reset_buf();
            if (s != str)
                s++;
            while (isspace(*s))
                s++;
            str = s;
        }
    }
}


void
h2text::flush_text(sLstr *lstr)
{
    if (Linebuf[Indent] != 0) {
        lstr->add(Linebuf);
        lstr->add_c('\n');
        reset_buf();
    }
}


void
h2text::reset_buf()
{
    int i;
    for (i = 0; i < Indent; i++)
        Linebuf[i] = ' ';
    Linebuf[i] = 0;
    End = i;
}


/*========================================================================
 =
 =   Text Converter for gtkhtm Widget
 =
 ========================================================================*/

#define CR '\015'
#define LF '\012'
#define L_PAREN          '('
#define R_PAREN          ')'
#define B_SLASH          '\\'
#define MAX_ASCII        '\177'

// MONO returns total intensity of r,g,b components .33R+ .5G+ .17B
#define MONO(rd, gn, bl) (((rd)*11 + (gn)*16 + (bl)*5) >> 13)

#define INCH        72
#define MM        INCH / 25.4

// PSconst_out outputs to the postscript buffer an array of constant
// strings
//
#define PSconst_out(txt) {               \
    int n=(sizeof txt)/(sizeof txt[0]);  \
    int i;                               \
    for (i=0; i<n; i++) {                \
        ps_printf("%s\n", txt[i]) ;      \
    }                                    \
}

namespace htm
{
    enum { F_FULLCOLOR, F_GREYSCALE, F_BWDITHER, F_REDUCED };

    // for regular-font, bold-font, italic-font, fixed-font
    enum PS_fontstyle { RF, BF, IF, FF, FB, FI };

    struct PAGE_DIMENS_T
    {
        double page_height;
        double page_width;
        double top_margin;
        double bot_margin;
        double left_margin;
        double right_margin;
        double text_height;
        double text_width;
    };

    PAGE_DIMENS_T a4_page_dimens = { 297 * MM, 210 * MM, 20 * MM, 20 * MM,
        20 * MM, 20 * MM, 0.0, 0.0 };

    PAGE_DIMENS_T us_letter_page_dimens = { 11 * INCH, 8.5 * INCH, 0.7 * INCH,
        0.7 * INCH, 0.9 * INCH, 0.9 * INCH, 0.0, 0.0 };

    class sPS
    {
    public:
        sPS(int, bool, bool, htmWidget*);

        char *ps_convert(const char*, const char*);

        bool PrintHeaders;      // flag whether page headers enabled
        int FontFamily;         // 0 Times, 1 Helvetica,
                                // 2 New Century Schoolbook,
                                // 3 Lucida Bright
        bool Paper_Size_A4;     // true if A4, false if US Letter

    private:
        void ps_printf(const char*, ...);
        void ps_hex(unsigned char, bool);
        void ps_font(htmFont*);
        void ps_showpage();
        void ps_newpage();
        void ps_init_latin1();
        void ps_header(const char*, const char*);
        void ps_trailer();
        void ps_moveto(int, int);
        void ps_text(htmWord*);
        void ps_bullet(htmObjectTable*);
        void ps_hrule(htmObjectTable*);
        void ps_table(htmObjectTable*);
        void ps_form(htmObjectTable*);
        int ps_rle_encode(unsigned char*, unsigned char*, int);
        void ps_color_image();
        void ps_colormap(int, int, unsigned short*, unsigned short*,
            unsigned short*);
        void ps_rle_cmapimage(int);
        void ps_write_bw(unsigned char*, int, int, int);
        void ps_image(htmWord*);

        htmWidget *ps_hw;       // source widget
        int ps_hexi;            // location index for hex()
        double ps_oldfs;        // current font size
        PS_fontstyle ps_oldfn;  // current font style
        int ps_curr_page;       // current page
        int ps_start_y;         // initial Y value, used as an offset for
                                //  pages > 1
        int ps_left_margin;     // left margin alteration, not used

        double ps_points_per_pixel;
        int ps_pixels_per_page;
        htmColor ps_fg_color, ps_bg_color;
        PAGE_DIMENS_T ps_page_dimens;

        unsigned char ps_hexline[80];
        sLstr ps_lstr;
    };
}


namespace {
    // Calculate the pixel density in dots per inch on the current widget
    // screen.
    //
    double
    get_dpi()
    {
    //    double dpi = 25.4 * gdk_screen_width() / gdk_screen_width_mm();
    //    if (dpi < 1.0 || dpi > 10000.0)
    double
            dpi = 72.0;
        return (dpi);
    }
}


sPS::sPS(int ff, bool ph, bool a4, htmWidget *hw)
{
    FontFamily = ff;
    PrintHeaders = ph;
    Paper_Size_A4 = a4;

    ps_hw = hw;
    ps_hexi = 0;
    ps_oldfs = 0.0;
    ps_oldfn = RF;
    ps_curr_page = 0;
    ps_start_y = 0;
    ps_left_margin = 0;
    ps_points_per_pixel = 0.0;
    ps_pixels_per_page = 0;

    ps_fg_color.pixel = hw->htm_cm.cm_body_fg;
    ps_bg_color.pixel = hw->htm_cm.cm_body_bg;
    hw->htm_tk->tk_query_colors(&ps_fg_color, 1);
    hw->htm_tk->tk_query_colors(&ps_bg_color, 1);

    // Setup page size according to user preference
    if (Paper_Size_A4)
        ps_page_dimens = a4_page_dimens;
    else
        ps_page_dimens = us_letter_page_dimens;
    ps_page_dimens.text_height = (ps_page_dimens.page_height -
        ps_page_dimens.top_margin - ps_page_dimens.bot_margin);
    ps_page_dimens.text_width  = (ps_page_dimens.page_width -
        ps_page_dimens.left_margin - ps_page_dimens.right_margin);

    // Calculate the number of Postscript points per pixel of current
    // screen, and the height of the page in pixels (used in figuring
    // when we've hit the bottom of the page).
    //
    ps_points_per_pixel = 72.0 / get_dpi();
    double pagewidth = hw->htm_formatted_width - 2*hw->htm_margin_width;
    if (pagewidth < hw->htm_viewarea.width)
        pagewidth = hw->htm_viewarea.width;

    if (pagewidth > ps_page_dimens.text_width)
        ps_points_per_pixel *= ps_page_dimens.text_width / pagewidth;
    ps_pixels_per_page =
        (int)(ps_page_dimens.text_height / ps_points_per_pixel);
}


// forget about the macro's
#undef Isgray
#undef Is_fg
#undef Is_bg

namespace {
    int
    line_height(htmWord *word)
    {
        unsigned line = word->line;
        unsigned height = word->area.height;
        for (htmObjectTable *eptr = word->owner; eptr; eptr = eptr->next) {
            if (eptr->n_words) {
                for (int n = 0; n < eptr->n_words; n++) {
                    htmWord *w = &eptr->words[n];
                    if (w->line < line)
                        continue;
                    if (w->line == line) {
                        if (w->area.height > height)
                            height = w->area.height;
                    }
                    if (w->line > line)
                        return (height);
                }
            }
            else {
                if (eptr->line == line) {
                    if (eptr->area.height > height)
                        height = eptr->area.height;
                }
                else
                    return (height);
            }
        }
        return (height);
    }


    int
    line_height(htmObjectTable *e0)
    {
        unsigned line = e0->line;
        unsigned height = e0->area.height;
        for (htmObjectTable *eptr = e0; eptr; eptr = eptr->next) {
            if (eptr->n_words) {
                for (int n = 0; n < eptr->n_words; n++) {
                    htmWord *w = &eptr->words[n];
                    if (w->line == line) {
                        if (w->area.height > height)
                            height = w->area.height;
                    }
                    else
                        return (height);
                }
            }
            else {
                if (eptr->line == line) {
                    if (eptr->area.height > height)
                        height = eptr->area.height;
                }
                else
                    return (height);
            }
        }
        return (height);
    }
}


// Actually perform conversion to PostScript.  Since the widget gives us
// each word or other object and coordinates, these are simply scaled
// to the page.  Since the font sizes don't match exactly, the word
// spacing is done here, which is ok for most text, but may not work too
// well in tables.
//
char *
sPS::ps_convert(const char *title, const char *url)
{
    if (!title)
        title = "Untitled";
    if (!url)
        url = "";
    ps_header(title, url);
    ps_newpage();

    int xpos;                 // line start x position
    int ypos = 0;             // y position
    int line = -1;            // line number
    int lastx = 0;            // used for word spacing computation

    for (htmObjectTable *eptr = ps_hw->htm_formatted; eptr;
            eptr = eptr->next) {
        if (eptr->object_type == OBJ_TEXT ||
                eptr->object_type == OBJ_PRE_TEXT) {

            for (int n = 0; n < eptr->n_words; n++) {
                htmWord *w = &eptr->words[n];

                // Check if this is a new line.
                if ((unsigned)line != w->line) {
                    line = w->line;
                    ypos = w->ybaseline;
                    xpos = w->area.x - ps_left_margin;
                    if (xpos < 0)
                        xpos = 0;

                    // Check if line fits completly on page, ypos is bottom
                    // of rendered area.
                    int lh = line_height(w);
                    if (ypos + lh > ps_start_y + ps_pixels_per_page) {
                        ps_start_y = ypos - lh;
                        ps_showpage();
                        ps_newpage();
                    }
                    ps_moveto(xpos, ypos);
                }
                else {
                    if (w->ybaseline != ypos) {
                        // Element in line (such as an image) has different
                        // y position, compensate here, along with word
                        // spacing.
                        if (w->area.x > lastx)
                            ps_printf("%d %d R\n", w->area.x - lastx,
                                ypos - w->ybaseline);
                        else {
                            // We get here in tables, since the line number
                            // seems not to change.  If the new x value is
                            // to the left, we are actually starting a new
                            // line.
                            xpos = w->area.x - ps_left_margin;
                            if (xpos < 0)
                                xpos = 0;
                            // Check if line fits completly on page
                            // of rendered area.
                            int lh = line_height(w);
                            if (ypos + lh > ps_start_y + ps_pixels_per_page) {
                                ps_start_y = ypos - lh;
                                ps_showpage();
                                ps_newpage();
                            }
                            ps_moveto(xpos, w->ybaseline);
                        }
                        ypos = w->ybaseline;
                    }
                    else {
                        // Add space from last object.
                        if (eptr->object_type == OBJ_PRE_TEXT)
                            ps_moveto(w->area.x - ps_left_margin,
                                w->ybaseline);
                        else
                            ps_printf("%d 0 R\n", w->area.x - lastx);
                    }
                }

                switch (w->type) {
                case OBJ_TEXT:
                    ps_text(w);
                    break;
                case OBJ_IMG:
                    ps_image(w);
                    break;
                case OBJ_BULLET:
                case OBJ_HRULE:
                case OBJ_TABLE:
                case OBJ_FORM:
                case OBJ_BLOCK:
                case OBJ_APPLET:
                case OBJ_TABLE_FRAME:
                case OBJ_PRE_TEXT:
                case OBJ_NONE:
                    break;
                }
                lastx = w->area.right();
            }
        }
        else if (eptr->object_type == OBJ_BULLET ||
                eptr->object_type == OBJ_HRULE) {
            // check if this is a new line
            if ((unsigned)line != eptr->line) {
                line = eptr->line;
                ypos = eptr->area.y;
                xpos = eptr->area.x - ps_left_margin;
                if (xpos < 0)
                    xpos = 0;

                // Check if line fits completly on page, ypos is bottom
                // of rendered area.
                int lh = line_height(eptr);
                if (ypos + lh > ps_start_y + ps_pixels_per_page) {
                    ps_start_y = ypos - lh;
                    ps_showpage();
                    ps_newpage();
                }
                ps_moveto(xpos, ypos);
            }
            else {
                if (eptr->area.y != ypos) {
                    // Element in line (such as an image) has different
                    // y position, compensate here, along with word
                    // spacing.
                    ps_printf("%d %d R\n", eptr->area.x - lastx,
                        ypos - eptr->area.y);
                    ypos = eptr->area.y;
                }
                else
                    // Add space from last object.
                    ps_printf("%d 0 R\n", eptr->area.x - lastx);
            }
            if (eptr->object_type == OBJ_BULLET)
                ps_bullet(eptr);
            else
                ps_hrule(eptr);
            lastx = eptr->area.right();
        }
        else if (eptr->object_type == OBJ_TABLE)
            ps_table(eptr);
        else if (eptr->object_type == OBJ_FORM)
            ps_form(eptr);
    }
    ps_showpage();
    ps_trailer();
    return (ps_lstr.string_trim());
}


// Dynamic string concatenation function.
//
void
sPS::ps_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char buf[1024];
    vsnprintf(buf, 1024, format, args);
    ps_lstr.add(buf);
    va_end(args);
}


// Output hex byte, caches until flush is true.
//
void
sPS::ps_hex(unsigned char val, bool flush)
{
    static char digit[] = "0123456789abcdef";

    if (!flush) {
        ps_hexline[ps_hexi++] = digit[((unsigned)val >> 4) & 0x0f];
        ps_hexline[ps_hexi++] = digit[(unsigned)val & 0x0f];
    }
    if ((flush && ps_hexi) || ps_hexi > 77) {
        ps_hexline[ps_hexi] = '\0';
        ps_hexi = 0;
        ps_printf("%s\n", ps_hexline);
    }
}


// Change font.
//
void
sPS::ps_font(htmFont *font)
{
    static char fnchar[6][3] = {"RF", "BF", "IF", "FF", "FB", "FI"};

    // Null case - reflush old font or the builtin default:
    if (font == 0) {
        if (ps_oldfs != 0.0)
            ps_printf("%2s %g SF\n", fnchar[ps_oldfn], ps_oldfs);
        return;
    }
    PS_fontstyle fn;
    if (!(font->style & FONT_FIXED)) {
        if (font->style & FONT_BOLD)
            fn = BF;
        else if (font->style & FONT_ITALIC)
            fn = IF;
        else
            fn = RF;
    }
    else {
        if (font->style & FONT_BOLD)
            fn = FB;
        else if (font->style & FONT_ITALIC)
            fn = FI;
        else
            fn = FF;
    }

    double fs = font->height * ps_points_per_pixel;
    if (fn != ps_oldfn || fs != ps_oldfs) {
        ps_printf( "%2s %g SF\n", fnchar[fn], fs);
        ps_oldfn = fn;
        ps_oldfs = fs;
    }
}


// End of page function.
//
void
sPS::ps_showpage()
{
    ps_printf("restore\n");
    ps_printf("showpage\n");
}


// Begin a fresh page, increment the page count and handle the structured
// comment conventions.
//
void
sPS::ps_newpage()
{
    ps_curr_page++;

    // The PostScript reference Manual states that the Page: Tag
    // should have a label and a ordinal; otherwise programs like
    // psutils fail    -gustaf

    ps_printf("%%%%Page: %d %d\n", ps_curr_page, ps_curr_page);
    ps_printf("save\n");
    if (PrintHeaders)
        ps_printf("%d ", ps_curr_page);
    ps_printf("NP\n");
    ps_font(0);        // force re-flush of last font used
}


// Handle ISO encoding.  Print out initializing PostScript text for
// ISO Latin1 font encoding This code is copied from the Idraw program
// (from Stanford's InterViews package), courtesy of Steinar
// Kjaernsr|d, steinar@ifi.uio.no.
//
void
sPS::ps_init_latin1()
{
    static const char *txt[] = {
        "/reencodeISO {",
        "dup dup findfont dup length dict begin",
        "{ 1 index /FID ne { def }{ pop pop } ifelse } forall",
        "/Encoding ISOLatin1Encoding D",
        "currentdict end definefont",
        "} D",
        "/ISOLatin1Encoding [",
        "/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef",
        "/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef",
        "/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef",
        "/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef",
        "/space/exclam/quotedbl/numbersign/dollar/percent/ampersand/quoteright",
        "/parenleft/parenright/asterisk/plus/comma/minus/period/slash",
        "/zero/one/two/three/four/five/six/seven/eight/nine/colon/semicolon",
        "/less/equal/greater/question/at/A/B/C/D/E/F/G/H/I/J/K/L/M/N",
        "/O/P/Q/R/S/T/U/V/W/X/Y/Z/bracketleft/backslash/bracketright",
        "/asciicircum/underscore/quoteleft/a/b/c/d/e/f/g/h/i/j/k/l/m",
        "/n/o/p/q/r/s/t/u/v/w/x/y/z/braceleft/bar/braceright/asciitilde",
        "/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef",
        "/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef",
        "/.notdef/dotlessi/grave/acute/circumflex/tilde/macron/breve",
        "/dotaccent/dieresis/.notdef/ring/cedilla/.notdef/hungarumlaut",
        "/ogonek/caron/space/exclamdown/cent/sterling/currency/yen/brokenbar",
        "/section/dieresis/copyright/ordfeminine/guillemotleft/logicalnot",
        "/hyphen/registered/macron/degree/plusminus/twosuperior/threesuperior",
        "/acute/mu/paragraph/periodcentered/cedilla/onesuperior/ordmasculine",
        "/guillemotright/onequarter/onehalf/threequarters/questiondown",
        "/Agrave/Aacute/Acircumflex/Atilde/Adieresis/Aring/AE/Ccedilla",
        "/Egrave/Eacute/Ecircumflex/Edieresis/Igrave/Iacute/Icircumflex",
        "/Idieresis/Eth/Ntilde/Ograve/Oacute/Ocircumflex/Otilde/Odieresis",
        "/multiply/Oslash/Ugrave/Uacute/Ucircumflex/Udieresis/Yacute",
        "/Thorn/germandbls/agrave/aacute/acircumflex/atilde/adieresis",
        "/aring/ae/ccedilla/egrave/eacute/ecircumflex/edieresis/igrave",
        "/iacute/icircumflex/idieresis/eth/ntilde/ograve/oacute/ocircumflex",
        "/otilde/odieresis/divide/oslash/ugrave/uacute/ucircumflex/udieresis",
        "/yacute/thorn/ydieresis",
        "] D",
        "[RF BF IF FF FB FI] {reencodeISO D} forall"
    };
    PSconst_out(txt);
}


// Initialize Postscript output.
//
// Prints out the prolog.  The following PostScript macros are defined
//        D         def - define a macro
//        E         exch - exhange parameters
//        M         moveto
//        R         rmoveto
//        L         lineto
//        RL        rlineto
//        SQ        draw a unit square
//        U         underline a string
//        B         draw a bullet
//        OB        draw an open bullet
//        HR        draw a horizontal rule
//        SF        set font
//        RF        roman font (dependent on font family)
//        BF        bold font (dependent on font family)
//        IF        italic font (dependent on font family)
//        FF        fixed font (courier)
//        FB        fixed bold font (courier bold)
//        FI        fixed italic font (courier oblique)
//        nstr      buffer for creating page number string
//        pageno    literal "Page "
//        url       URL of document
//        title     title of document
//        date      date modified/printed
//
void
sPS::ps_header(const char *title, const char *url)
{
    if (!title)
        title = "Untitled";
    if (!url)
        url = "";
    char *urlcpy = 0;
    if (strlen(url) > 64) {
        if (lstring::is_rooted(url))
            url = lstring::strip_path(url);
        else {
            urlcpy = lstring::copy(url);
            strcpy(urlcpy + 61, "...");
            url = urlcpy;
        }
    }

    int font = FontFamily;
    static const char *fontname[] = {
        // in order: regular, bold, italic
        "Times-Roman", "Times-Bold", "Times-Italic",
        "Helvetica", "Helvetica-Bold", "Helvetica-Oblique",
        "NewCenturySchlbk-Roman", "NewCenturySchlbk-Bold",
        "NewCenturySchlbk-Italic",
        // this is a nasty trick, I have put Times in place of
        // Lucida, because most printers don't have Lucida font
        //
        "Times-Roman", "Times-Bold", "Times-Italic"
            // "Lucida", "Lucida-Bold", "Lucida-Italic"
    };

    static const char *txt[] = {
        "/M {moveto} D",
        "/S {show} D",
        "/R {rmoveto} D",
        "/L {lineto} D",
        "/RL {rlineto} D",
        "/SQ {newpath 0 0 M 0 1 L 1 1 L 1 0 L closepath} D",
    "/U {gsave currentpoint currentfont /FontInfo get /UnderlinePosition get",
        " 0 E currentfont /FontMatrix get dtransform E pop add newpath moveto",
        " dup stringwidth rlineto stroke grestore S } D",
        "/B {/r E D gsave -13 0  R currentpoint ",
        "  newpath r 0 360 arc closepath fill grestore } D",
        "/OB {/r E D gsave -13 0  R currentpoint ",
        "  newpath r 0 360 arc closepath stroke grestore } D",
        "/BB {/r E D gsave -13 0 R currentpoint M r neg r neg R currentpoint ",
        "  newpath M 0 r 2 mul RL r 2 mul 0 RL 0 r -2 mul RL r -2 mul 0 RL ",
        " closepath stroke grestore } D",
    "/HR {/l E D gsave 0 1 RL l 0 RL 0 -1 RL l neg 0 RL stroke grestore } D",
        "/SF {E findfont E scalefont setfont } D",
        "/FF {/Courier } D",
        "/FB {/Courier-Bold } D",
        "/FI {/Courier-Oblique } D"
    };

    char time_buf[40];
    time_t clock = time(0);

    strftime(time_buf, sizeof(time_buf),
         "Printed %a %b %d %H:%M:%S %Y", localtime(&clock));

    ps_printf("%%!PS-Adobe-1.0\n");
    ps_printf("%%%%Creator: Whiteley Research Inc. from NCSA Mosaic\n");
    /*
    "%%%%Creator: NCSA Mosaic, Postscript by Ameet Raval, Frans van Hoesel\n";
    "%%%%         and Andrew Ford\n";
    */

    char tbuf[132];
    strncpy(tbuf, title, 132);
    tbuf[131] = 0;
    title = tbuf;
    for (char *tmp = tbuf; *tmp; tmp++) {
        if (*tmp == CR || *tmp == LF)
            *tmp = ' ';
    }

    ps_printf("%%%%Title: %s\n", title);
    ps_printf("%%%%CreationDate: %s\n", time_buf + 8);
    ps_printf("%%%%Pages: (atend)\n");
    ps_printf("%%%%PageOrder: Ascend\n");
    ps_printf("%%%%BoundingBox: %d %d %d %d\n",
        (int)ps_page_dimens.left_margin,
        (int)(ps_page_dimens.bot_margin - 12),
        (int)(ps_page_dimens.left_margin + ps_page_dimens.text_width + 0.5),
        (int)(ps_page_dimens.bot_margin + ps_page_dimens.text_height + 12.5));
    ps_printf(
        "%%%%DocumentFonts: %s %s %s Courier Courier-Bold Courier-Oblique\n",
        fontname[font*3], fontname[font*3+1], fontname[font*3+2]);
    ps_printf("%%%%EndComments\n");
    ps_printf("save /D {def} def /E {exch} D\n");
    ps_printf("/RF {/%s} D\n", fontname[font*3]);
    ps_printf("/BF {/%s} D\n", fontname[font*3+1]);
    ps_printf("/IF {/%s} D\n", fontname[font*3+2]);
    ps_printf("/nstr 6 string D /pgno (Page ) D\n");
    ps_printf("/url (%s) D\n", url);
    ps_printf("/title (%s) D\n", title);
    ps_printf("/date (%s) D\n", time_buf);
    PSconst_out(txt);

    // Output the newpage definition

    ps_printf("/NP {");
    if (PrintHeaders) {
        ps_printf("gsave 0.4 setlinewidth\n");
        ps_printf("  newpath %.2f %.2f M %.2f 0 RL stroke",
             ps_page_dimens.left_margin,
             (ps_page_dimens.bot_margin + ps_page_dimens.text_height),
             ps_page_dimens.text_width);
        ps_printf("  newpath %.2f %.2f M %.2f 0 RL stroke\n",
             ps_page_dimens.left_margin, ps_page_dimens.bot_margin,
             ps_page_dimens.text_width);
        ps_printf("  RF 10 SF %.2f %.2f M (%s) S\n",
             ps_page_dimens.left_margin,
             (ps_page_dimens.bot_margin + ps_page_dimens.text_height + 6),
             title);
        ps_printf("  nstr cvs dup stringwidth pop pgno stringwidth pop add\n");
        ps_printf("  %.2f E sub %.2f M pgno S S\n",
             (ps_page_dimens.left_margin + ps_page_dimens.text_width),
             (ps_page_dimens.bot_margin + ps_page_dimens.text_height + 6));
        ps_printf("  RF 8 SF %.2f %.2f M (%s) S\n",
             ps_page_dimens.left_margin, ps_page_dimens.bot_margin - 12, url);
        ps_printf("  (%s) dup stringwidth pop %.2f E sub %.2f M S grestore\n",
             time_buf, ps_page_dimens.left_margin + ps_page_dimens.text_width,
             ps_page_dimens.bot_margin - 12);
    }
    ps_printf("  %.2f %.2f translate %.5f %.5f scale } D\n",
         ps_page_dimens.left_margin,
         ps_page_dimens.bot_margin + ps_page_dimens.text_height,
         ps_points_per_pixel, ps_points_per_pixel);
    ps_init_latin1();

    ps_printf("%%%%EndProlog\n");
    delete [] urlcpy;
}


// Write postscript trailer.
//
void
sPS::ps_trailer()
{
    ps_printf("%%%%Trailer\n");
    ps_printf("restore\n");
    ps_printf("%%%%Pages: %d\n", ps_curr_page);
}


// Move to new x,y location.
//
void
sPS::ps_moveto(int x, int y)
{
    if (y > ps_start_y + ps_pixels_per_page) {
        // shouldn't happen
        ps_start_y = y;
        ps_showpage();
        ps_newpage();
    }
    ps_printf( "%d %d M\n", x, -(y - ps_start_y));
}


// Output text, show text, and protect special characters if needed.
// if Underline is non-zero, the text is underlined.
//
void
sPS::ps_text(htmWord *wptr)
{
    bool underline = false;
    ps_font(wptr->font);        // set font

    // Allocate a string long enough to hold the original string with
    // every character stored as an octal escape (worst case scenario).

    char *s2 = new char[strlen(wptr->word) * 4 + 1];

    //  For each char in s2, if it is a special char, insert "\"
    //  into the new string s2, then insert the actual char
    //
    char *s = wptr->word;
    unsigned char ch;
    char *stmp;
    for (stmp = s2; (ch = *s++) != '\0'; ) {
        if (ch == L_PAREN || ch == R_PAREN || ch == B_SLASH) {
            *stmp++ = B_SLASH;
            *stmp++ = ch;
        }
        else if (ch > (unsigned char)MAX_ASCII) {
            //  convert to octal
            *stmp++ = B_SLASH;
            *stmp++ = ((ch >> 6) & 007) + '0';
            *stmp++ = ((ch >> 3) & 007) + '0';
            *stmp++ = (ch & 007) + '0';
        }
        else
            *stmp++ = ch;
    }
    *stmp = 0;

    /* debugging
    ps_printf(
        "gsave %d %d M 0 %d RL %d 0 RL 0 %d RL closepath stroke\n",
        wptr->area.x, -(wptr->area.y - ps_start_y), -wptr->area.height,
        wptr->area.width, wptr->area.height);
    ps_printf("grestore\n");
    */

    ps_printf("(%s)%c\n", s2, underline ? 'U' : 'S');
    delete [] s2;
}


// Output a bullet.  The bullet is normally filled, except for a
// bullet with an indent level of two.  The size of the higher level
// bullets is just somewhat smaller.
//
void
sPS::ps_bullet(htmObjectTable *eptr)
{
    htmFont *ft = eptr->font;
    int height = ft->ascent + ft->descent;
    double size = height / 5.0;

    const char *bstr = "B";
    if (eptr->marker == MARKER_CIRCLE)
        bstr = "OB";
    else if (eptr->marker == MARKER_SQUARE) {
        bstr = "BB";
        size *= .7;
    }
    else if (eptr->marker == MARKER_DISC) {
        size *= .7;
    }
    else {
        ps_font(eptr->font);
        ps_printf("%d %d R\n", -height/2, -ft->ascent + 2);
        ps_printf("(%s)%c\n", eptr->text, 'S');
        ps_printf("%d %d R\n", height/2, ft->ascent - 2);
        return;
    }
    ps_printf(" %f %s\n", size, bstr);
}


// Draw a horizontal line with the given width.
//
void
sPS::ps_hrule(htmObjectTable *eptr)
{
    int length = eptr->area.width;
    ps_printf("%d HR\n", length);
}


// Draw the dividing lines in a table.
//
// Not implemented
//
void
sPS::ps_table(htmObjectTable *eptr)
{
    (void)eptr;
    /*
    // Two problems with this:
    // 1.  The x coords aren't right since we do word spacing locally.
    // 2.  How to handle page breaks?
    //
    if (eptr->table->t_properties &&
            eptr->table->t_properties->tp_border > 0) {
        // draw a box around the table
        int x = eptr->area.x - ps_left_margin;
        if (x < 0)
            x = 0;
        int y = eptr->area.y;
        unsigned int w = eptr->area.width;
        unsigned int h = eptr->area.height;
        ps_printf(
            "gsave %d %d M 0 %d RL %d 0 RL 0 %d RL closepath stroke\n",
            x, -(y - ps_start_y), -h, w, h);
        ps_printf("grestore\n");
    }

    htmTable *tptr = eptr->table;
    for (int r = 0; r < tptr->nrows; r++) {
        TableRow *row = &tptr->rows[r];
        for (int c = 0; c < row->ncells; c++) {
            TableCell *cell = &row->cells[c];
            int x = cell->start->x - ps_left_margin;
            if (x < 0)
                x = 0;
            int y = cell->start->area.y;
            unsigned int w = cell->start->area.width;
            unsigned int h = cell->start->area.height;
            ps_printf(
                "gsave %d %d M 0 %d RL %d 0 RL 0 %d RL closepath stroke\n",
                x, -(y - ps_start_y), -h, w, h);
            ps_printf("grestore\n");
        }
    }
    */
}


// Draw a form field, currently just draw a grey box of the dimensions
// of the field.
//
void
sPS::ps_form(htmObjectTable *eptr)
{
    htmForm *fptr = eptr->form;
    int w = fptr->width;
    int h = fptr->height;

    ps_printf("gsave currentpoint %d sub translate ", h);
    ps_printf("%d %d scale\n", w, h);
    ps_printf("SQ 0.9 setgray fill\n");
    ps_printf("grestore\n");
}


// Perform run length encoding.
//
// rle is encoded as such:
//  <count> <value>                        # 'run' of count+1 equal pixels
//  <count | 0x80> <count+1 data bytes>    # count+1 non-equal pixels
// count can range between 0 and 127
//
// Returns length of the rleline vector.
//
int
sPS::ps_rle_encode(unsigned char *scanline, unsigned char *rleline, int wide)
{
    int  i, j, blocklen, isrun, rlen;
    unsigned char block[256], pix;

    blocklen = isrun = rlen = 0;

    for (i = 0; i < wide; i++) {
        //  there are 5 possible states:
        //   0: block empty.
        //   1: block is a run, current pix == previous pix
        //   2: block is a run, current pix != previous pix
        //   3: block not a run, current pix == previous pix
        //   4: block not a run, current pix != previous pix
        //

        pix = scanline[i];

        if (!blocklen) {
            // case 0:  empty
            block[blocklen++] = pix;
            isrun = 1;
        }
        else if (isrun) {
            if (pix == block[blocklen-1]) {
                //  case 1:  isrun, prev == cur
                block[blocklen++] = pix;
            }
            else {
                //  case 2:  isrun, prev != cur
                if (blocklen > 1) {
                    //  we have a run block to flush
                    rleline[rlen++] = blocklen-1;
                    rleline[rlen++] = block[0];
                    //  start new run block with pix
                    block[0] = pix;
                    blocklen = 1;
                }
                else {
                    // blocklen<=1, turn into non-run
                    isrun = 0;
                    block[blocklen++] = pix;
                }
            }
        }
        else {
            // not a run
            if (pix == block[blocklen-1]) {
                // case 3: non-run, pre v== cur
                if (blocklen > 1) {
                    // have a non-run block to flush
                    rleline[rlen++] = (blocklen-1) | 0x80;
                    for (j = 0; j < blocklen; j++)
                        rleline[rlen++] = block[j];
                    // start new run block with pix
                    block[0] = pix;
                    blocklen = isrun = 1;
                }
                else {
                    // blocklen <= 1 turn into a run
                    isrun = 1;
                    block[blocklen++] = pix;
                }
            }
            else {
                // case 4:  non-run, prev != cur
                block[blocklen++] = pix;
            }
        }

        // max block length.  flush
        if (blocklen == 128) {
            if (isrun) {
                rleline[rlen++] = blocklen-1;
                rleline[rlen++] = block[0];
            }
            else {
                rleline[rlen++] = (blocklen-1) | 0x80;
                for (j = 0; j < blocklen; j++)
                    rleline[rlen++] = block[j];
            }
            blocklen = 0;
        }
    }

    // flush last block
    if (blocklen) {
        if (isrun) {
            rleline[rlen++] = blocklen-1;
            rleline[rlen++] = block[0];
        }
        else {
            rleline[rlen++] = (blocklen-1) | 0x80;
            for (j=0; j<blocklen; j++)
                rleline[rlen++] = block[j];
        }
    }

    return (rlen);
}


// Created postscript colorimage operator.  Spits out code that checks
// if the PostScript device in question knows about the 'colorimage'
// operator.  If it doesn't, it defines 'colorimage' in terms of image
// (ie, generates a greyscale image from RGB data).
//
void
sPS::ps_color_image()
{
    static const char *txt[] = {
        "% define 'colorimage' if it isn't defined",
        "%   ('colortogray' and 'mergeprocs' come from xwd2ps",
        "%         via xgrab)",
        "/colorimage where   % do we know about 'colorimage'?",
        "  { pop }                   % yes: pop off the 'dict' returned",
        "  {                                 % no:  define one",
        "        /colortogray {  % define an RGB->I function",
        "          /rgbdata exch store        % call input 'rgbdata'",
        "          rgbdata length 3 idiv",
        "          /npixls exch store",
        "          /rgbindx 0 store",
        "          /grays npixls string store  % str to hold the result",
        "          0 1 npixls 1 sub {",
        "                grays exch",
        "                rgbdata rgbindx           get 20 mul        % Red",
        "                rgbdata rgbindx 1 add get 32 mul        % Green",
        "                rgbdata rgbindx 2 add get 12 mul        % Blue",
        "                add add 64 idiv          % I = .5G + .31R + .18B",
        "                put",
        "                /rgbindx rgbindx 3 add store",
        "          } for",
        "          grays",
        "        } bind def\n",
        // Utility procedure for colorimage operator.
        // This procedure takes two procedures off the
        // stack and merges them into a single procedure
        //
        "        /mergeprocs { % def",
        "          dup length",
        "          3 -1 roll dup length dup 5 1 roll",
        "          3 -1 roll add array cvx dup",
        "          3 -1 roll 0 exch putinterval",
        "          dup 4 2 roll putinterval",
        "        } bind def\n",
        "        /colorimage { % def",
        // remove 'false 3' operands
        "          pop pop",
        "          {colortogray} mergeprocs",
        "          image",
        "        } bind def",
        // end of 'false' case
        "  } ifelse"
    };
    PSconst_out(txt);
}


// Write colormap.  Spits out code for the colormap of the following
// image if !color, it spits out a mono-ized graymap.
//
void
sPS::ps_colormap(int color, int nc, unsigned short *rmap, unsigned short *gmap,
    unsigned short *bmap)
{
    // define the colormap
    ps_printf("/cmap %d string def\n\n\n", nc * ((color) ? 3 : 1));

    // load up the colormap
    ps_printf("currentfile cmap readhexstring\n");

    for (int i = 0; i < nc; i++) {
        if (color)
            ps_printf("%02x%02x%02x ", rmap[i], gmap[i], bmap[i]);
        else
            ps_printf("%02x ", MONO(rmap[i], gmap[i], bmap[i]));
        if ((i%10) == 9)
            ps_printf("\n");
    }
    ps_printf("\n");
    ps_printf("pop pop\n");  // lose return values from readhexstring
}


// Define rlecmapimage operator
//
void
sPS::ps_rle_cmapimage(int color)
{
    static const char *txt[] = {

        // rlecmapimage expects to have 'w h bits matrix' on stack
        "/rlecmapimage {",
        "  /buffer 1 string def",
        "  /rgbval 3 string def",
        "  /block  384 string def",
        "  { currentfile buffer readhexstring pop",
        "        /bcount exch 0 get store",
        "        bcount 128 ge",
        "        { ",
        "          0 1 bcount 128 sub",
        "        { currentfile buffer readhexstring pop pop"
    };

    static const char *txt_color[] = {
        "                /rgbval cmap buffer 0 get 3 mul 3 getinterval store",
        "                block exch 3 mul rgbval putinterval",
        "          } for",
        "          block  0  bcount 127 sub 3 mul  getinterval",
        "        }",
        "        { ",
        "          currentfile buffer readhexstring pop pop",
        "          /rgbval cmap buffer 0 get 3 mul 3 getinterval store",
        "          0 1 bcount { block exch 3 mul rgbval putinterval } for",
        "          block 0 bcount 1 add 3 mul getinterval",
        "        } ifelse",
        "  }",
        "  false 3 colorimage",
        "} bind def"
    };

    static const char *txt_gray[] = {
        "                /rgbval cmap buffer 0 get 1 getinterval store",
        "                block exch rgbval putinterval",
        "          } for",
        "          block  0  bcount 127 sub  getinterval",
        "        }",
        "        { ",
        "          currentfile buffer readhexstring pop pop",
        "          /rgbval cmap buffer 0 get 1 getinterval store",
        "          0 1 bcount { block exch rgbval putinterval } for",
        "          block 0 bcount 1 add getinterval",
        "        } ifelse",
        "  }",
        "  image",
        "} bind def"
    };

    PSconst_out(txt);
    if (color) {
        PSconst_out(txt_color);
    }
    else {
        PSconst_out(txt_gray);
    }
}


// Write B&W image
// Write the given image array 'pic' (B/W stippled, 1 byte per pixel,
// 0=blk,1=wht) out as hexadecimal, max of 72 hex chars per line.  If
// 'flipbw', then 0=white, 1=black.  Returns '0' if everythings fine,
// 'EOF' if writing failed.
//
void
sPS::ps_write_bw(unsigned char *pic, int w, int h, int flipbw)
{
    unsigned char outbyte = 0;
    unsigned char bitnum = 0;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            unsigned char bit = *(pic++);
            outbyte = (outbyte<<1) | ((bit)&0x01);
            bitnum++;

            if (bitnum == 8) {
                if (flipbw)
                    outbyte = ~outbyte & 0xff;
                ps_hex(outbyte, false);
                outbyte = bitnum = 0;
            }
        }
        if (bitnum) {        // few bits left over in this row
            outbyte <<= 8-bitnum;
            if (flipbw)
                outbyte = ~outbyte & 0xff;
            ps_hex(outbyte, false);
            outbyte = bitnum = 0;
        }
    }
    ps_hex('\0', true);        // Flush the hex buffer if needed
}


// Isgray returns true if the nth color index is a gray value
#define Isgray(i, n) (i->reds[n] == i->greens[n] && i->reds[n] == i->blues[n])

// Is_bg returns true if the nth color index is the screen background
#define Is_bg(i, n)  (i->reds[n] == ps_bg_color.red && \
        i->greens[n] == ps_bg_color.green && i->blues[n] == ps_bg_color.blue)

// Is_fg returns true if the nth color index is the screen foreground
#define Is_fg(i, n)  (i->reds[n] == ps_fg_color.red && \
        i->greens[n] == ps_fg_color.green && i->blues[n] == ps_fg_color.blue)

// Generate image Postscript code.  Draw the image, unless there was
// no image, in which case an empty grey rectangle is shown.  If
// anchor is set, a black border is shown around the image.
// Positioning is not exactly that of the screen, but close enough.
//
void
sPS::ps_image(htmWord *wptr)
{
    htmImage *img = wptr->image;
    htmImageInfo *imgi = img->html_image;
    unsigned char *imgp = img->html_image->data;
    int ncolors = img->html_image->ncolors;
    int w = img->width;
    int h = img->height;
    int extra = 0;
    bool anchor = false;

    if (anchor) {
        //  draw an outline by drawing a slightly larger black square
        //  below the actual image
        //
        ps_printf("gsave currentpoint %d sub translate ", h);
        ps_printf("0 -2 translate %d %d scale\n", w+4, h+4);
        ps_printf("SQ fill\n");
        ps_printf("grestore\n");
        extra = 4;
    }

    if (imgp == 0) {
        // image was not available... do something instead
        // draw an empty square for example
        //
        ps_printf("gsave currentpoint %d sub translate", h);
        if (anchor)
            ps_printf(" 2 0 translate");
        else
            ps_printf(" 0 2 translate");
        ps_printf(" %d %d scale\n", w, h);
        ps_printf("0.9 setgray SQ fill\n");
        ps_printf("grestore\n");
        // move currentpoint just right of image
        ps_printf("%d 0 R\n", w + extra);
        return;
    }

    //  This is a hack to see if the image is Black & White,
    //  Greyscale or 8 bit color
    //  assume it's bw if it has only one or two colors, both some grey's
    //  assume it's greyscale if all the colors (>2) are grey
    //  Images with only one color do occur too.
    //

    int slen, colorps, colortype, bits;
    if (((ncolors == 2)
            && ((Isgray(imgi, 0) && Isgray(imgi, 1))
                || (Is_bg(imgi, 0) && Is_fg(imgi, 1))
                || (Is_fg(imgi, 0) && Is_bg(imgi, 1))))
            || ((ncolors == 1) && (Isgray(imgi, 0)
                || Is_bg(imgi, 0)
                || Is_fg(imgi, 0)))) {
        colortype = F_BWDITHER;
        slen = (w+7)/8;
        bits = 1;
        colorps = 0;
    }
    else {
        colortype = F_GREYSCALE;
        slen = w;
        bits = 8;
        colorps = 0;
        for (int i = 0; i < ncolors; i++) {
            if (!Isgray(imgi, i)) {
                colortype = F_REDUCED;
                slen = w*3;
                bits = 8;
                colorps = 1;
                break;
            }
        }
    }

    // build a temporary dictionary
    ps_printf("20 dict begin\n\n");

    // define string to hold a scanline's worth of data
    ps_printf("/pix %d string def\n\n", slen);

    // position and scaling
    ps_printf("gsave currentpoint %d sub translate", h);
    if (anchor)
        ps_printf(" 2 0 translate");
    else
        ps_printf(" 0 2 translate");
    ps_printf(" %d %d scale\n", w, h);

    if (colortype == F_BWDITHER) {
        // 1-bit dither code uses 'image'
        bool flipbw = false;

        // set if color#0 is 'white'
        if ((ncolors == 2 &&
                MONO(imgi->reds[0], imgi->greens[0], imgi->blues[0]) >
                MONO(imgi->reds[1], imgi->greens[1], imgi->blues[1])) ||
                (ncolors == 1 &&
                MONO(imgi->reds[0], imgi->greens[0], imgi->blues[0]) >
                MONO(127, 127, 127)))
            flipbw = true;

        // dimensions of data
        ps_printf("%d %d %d\n", w, h, bits);

        // mapping matrix
        ps_printf("[%d 0 0 %d 0 %d]\n\n", w, -h, h);

        ps_printf("{currentfile pix readhexstring pop}\n");
        ps_printf("image\n");

        // write the actual image data
        ps_write_bw(imgp, w, h, flipbw);
    }
    else {
        // all other formats
        unsigned char *rleline = 0;
        int rlen;

        // if we're using color, make sure 'colorimage' is defined
        if (colorps)
            ps_color_image();

        ps_colormap(colorps, ncolors, imgi->reds, imgi->greens, imgi->blues);
        ps_rle_cmapimage(colorps);

        // dimensions of data
        ps_printf("%d %d %d\n", w, h, bits);
        // mapping matrix
        ps_printf("[%d 0 0 %d 0 %d]\n", w, -h, h);
        ps_printf("rlecmapimage\n");

        rleline = new unsigned char[w * 2];

        for (int i = 0; i < h; i++) {
            rlen = ps_rle_encode(imgp, rleline, w);
            imgp += w;
            for (int j = 0; j < rlen; j++)
                ps_hex(rleline[j], false);
            ps_hex('\0', true);  //  Flush the hex buffer
        }
        delete [] rleline;
    }

    // stop using temporary dictionary
    ps_printf("end\n");
    ps_printf("grestore\n");

    // move currentpoint just right of image
    ps_printf("%d 0 R\n", w + extra);
}
// End of cPS functions.


// Entry point for postscript output.
//
// The font is encoded as:
//        0: times (default)
//        1: helvetica
//        2: new century schoolbook
//        3: lucida
//
// The title string appears in the upper header, the url string appears
// in the footer (if use_header is true).  Paper size is US Letter unless
// a4 is set.
//
char *
htmWidget::getPostscriptText(int fontfamily, const char *url,
    const char *title, bool use_header, bool a4)
{
    char *xtitle = 0, *xurl = 0;
    if (!title || !url) {
        if (htm_if)
            htm_if->get_topic_keys(&xurl, &xtitle);
        if (!title)
            title = xtitle;
        if (!title)
            title = getTitle();
        if (!url)
            url = xurl;
    }

    sPS *ps = new sPS(fontfamily, use_header, a4, this);
    char *s = ps->ps_convert(title, url);
    delete ps;
    delete xtitle;
    delete xurl;
    return (s);
}


// Return an ascii string representation of the widget contents.  All
// text will be shown, images and other graphics are ignored.
//
char *
htmWidget::getPlainText()
{
    bool newline = false;
    int space_width = 8;
    int len = 0;
    int spaces = 0;
    int columns = 76;
    int lmargin = 0;
    int line = -1;
    sLstr lstr;

    for (htmObjectTable *eptr = htm_formatted; eptr; eptr = eptr->next) {
        switch (eptr->object_type) {
        case OBJ_PRE_TEXT:
        case OBJ_TEXT: {
            if (newline)
                lstr.add_c('\n');
            if (eptr->text_data & TEXT_BREAK) {
                lstr.add_c('\n');
                for (int i = 0; i < spaces; i++)
                    lstr.add_c(' ');
                len = spaces;
            }
            if ((unsigned)line == eptr->line)
                if (!newline && !(eptr->text_data & TEXT_BREAK) &&
                        len > spaces)
                    lstr.add_c(' ');
            line = eptr->line;
            if (eptr->text) {
                char *tptr = eptr->text;
                if (newline) {
                    lstr.add_c('\n');
                    newline = false;
                    spaces = (eptr->area.x - lmargin) / space_width;
                    if (spaces < 0)
                        spaces = 0;
                    for (int i = 0; i < spaces; i++)
                        lstr.add_c(' ');
                    len = spaces;
                }
                while (len + strlen(tptr) > (unsigned)columns) {
                    char *s = tptr + columns - len;
                    while (!isspace(*s) && s > tptr)
                        s--;
                    char buf[128];
                    strncpy(buf, tptr, s - tptr);
                    buf[s - tptr] = 0;
                    lstr.add(buf);
                    lstr.add_c('\n');
                    if (isspace(*s))
                        tptr = s+1;
                    len = spaces;
                    for (int i = 0; i < spaces; i++)
                        lstr.add_c(' ');
                }
                lstr.add(tptr);
                len += strlen(tptr);
            }

            break;
        }
        case OBJ_BULLET: {
            lstr.add_c('\n');
            int bspaces = (eptr->area.x - lmargin) / space_width;
            if (bspaces < 0)
                bspaces = 0;
            for (int i = 0; i < bspaces; i++)
                lstr.add_c(' ');
            lstr.add("o ");
            len = bspaces + 2;
            newline = false;
            break;
        }
        case OBJ_BLOCK:
            newline = true;
            break;
        case OBJ_HRULE:
        case OBJ_TABLE:
        case OBJ_TABLE_FRAME:
        case OBJ_IMG:
        case OBJ_FORM:
        case OBJ_APPLET:
        case OBJ_NONE:
            break;
        }
    }
    lstr.add_c('\n');
    return (lstr.string_trim());
}


/*=======================================================================
 =
 =    Text search function
 =
 =======================================================================*/

namespace htm
{
    struct cxt_t
    {
        cxt_t(htmObjectTable *e, int i, int o)
            { obj = e; word_indx = i; offset = o; next = 0; }

        static void destroy(cxt_t *t)
            {
                while (t) {
                    cxt_t *tx = t;
                    t = t->next;
                    delete tx;
                }
            }

        cxt_t *find(int os)
            {
                cxt_t *cx = this;
                while (cx->next && cx->next->offset <= os)
                    cx = cx->next;
                return (cx);
            }

        htmObjectTable *obj;
        int word_indx;
        int offset;
        cxt_t *next;
    };
}


struct htmSearch
{
    htmSearch()
        {
            head = 0;
            cx_list = 0;
            flat_string = 0;
            expression = 0;
            cur_start = 0;
            cur_end = 0;
        }

    ~htmSearch()
        {
            clear();
        }

    void clear()
        {
            head = 0;
            cxt_t::destroy(cx_list);
            cx_list = 0;
            delete [] flat_string;
            flat_string = 0;
            delete [] expression;
            expression = 0;
            cur_start = 0;
            cur_end = 0;
        }

    void load(htmObjectTable*, const char*);
    bool down_match(htmWidget*, bool);
    bool up_match(htmWidget*, bool);

private:
    htmObjectTable *head;
    cxt_t *cx_list;
    char *flat_string;
    char *expression;
    unsigned int cur_start;
    unsigned int cur_end;
};


void
htmSearch::load(htmObjectTable *ot, const char *exp)
{
    if (!ot || !exp || !*exp)
        return;
    if (ot == head && expression && !strcmp(expression, exp))
        return;

    clear();
    head = ot;
    expression = lstring::copy(exp);
    cxt_t *ce = 0;

    sLstr lstr;
    for (htmObjectTable *el = head; el; el = el->next) {
        if (el->object_type == OBJ_TEXT || el->object_type == OBJ_PRE_TEXT) {
            for (int i = 0; i < el->n_words; i++) {
                htmWord *w = &el->words[i];
                if (!cx_list)
                    cx_list = ce = new cxt_t(el, i, lstr.length());
                else {
                    ce->next = new cxt_t(el, i, lstr.length());
                    ce = ce->next;
                }
                lstr.add(w->word);
                lstr.add_c(' ');
            }
        }
    }
    flat_string = lstr.string_trim();
    cur_start = 0;
    cur_end = 0;
}


bool
htmSearch::down_match(htmWidget *htm, bool case_insens)
{
    if (!htm || !flat_string)
        return (true);
    unsigned int offset = 0;
    if (cur_start == cur_end) {
        // No current match.  Start looking at the top of the visible
        // text and hunt dowwnards.
        for (htmObjectTable *el = head; el; el = el->next) {
            // Break ahead of first fully visible word.
            if (el->area.top() >= htm->htm_viewarea.top())
                break;
            bool brk = false;
            if (el->object_type == OBJ_TEXT ||
                    el->object_type == OBJ_PRE_TEXT) {
                for (int i = 0; i < el->n_words; i++) {
                    htmWord *w = &el->words[i];
                    if (w->area.top() >= htm->htm_viewarea.top()) {
                        brk = true;
                        break;
                    }
                    offset += (strlen(w->word) + 1);
                }
            }
            if (brk)
                break;
        }
    }
    else {
        // Start searching downwards from existing position.
        offset = cur_end;
    }
    char *search_string = flat_string + offset;
        
    regex_t preg;
    if (regcomp(&preg, expression, case_insens ? REG_ICASE : 0)) {
        return (false);
    }
    int n = preg.re_nsub;
    regmatch_t *m = new regmatch_t[n+1];
    int ret = regexec(&preg, search_string, 1, m, 0);
    if (!ret) {
        int start = offset + m->rm_so;
        int end = offset + m->rm_eo;
        cxt_t *cxs = cx_list->find(start);
        cxt_t *cxe = cxs->find(end-1);
        htmWord *ws = &cxs->obj->words[cxs->word_indx];
        htmWord *we = &cxe->obj->words[cxe->word_indx];
        int x1 = ws->area.x;
        int x2 = we->area.x;
        int l = x1 < x2 ? x1 : x2;
        x1 += ws->area.width;
        x2 += we->area.width;
        int r = x1 > x2 ? x1 : x2;
        int y1 = ws->area.y + 1;
        int y2 = we->area.y + 1;
        int t = y1 < y2 ? y1 : y2;
        int b = y1 > y2 ? y1 : y2;

        htm->selectRegion(l, t, r, b);
        if (htm->htm_if)
            htm->htm_if->scroll_visible(l, t, r, b);
        cur_start = start;
        cur_end = end;
    }
    delete [] m;
    regfree(&preg);
    return (ret == 0);
}


bool
htmSearch::up_match(htmWidget *htm, bool case_insens)
{
    if (!htm || !flat_string)
        return (true);
    unsigned int offset = 0;
    if (cur_start == cur_end) {
        // No current match.  Start looking at the bottom of the visible
        // text and hunt upwards.

        for (htmObjectTable *el = head; el; el = el->next) {
            if (el->area.top() >= htm->htm_viewarea.bottom())
                break;
            bool brk = false;
            if (el->object_type == OBJ_TEXT ||
                    el->object_type == OBJ_PRE_TEXT) {
                for (int i = 0; i < el->n_words; i++) {
                    htmWord *w = &el->words[i];
                    if (w->area.top() >= htm->htm_viewarea.bottom()) {
                        brk = true;
                        break;
                    }
                    offset += (strlen(w->word) + 1);
                }
            }
            if (brk)
                break;
        }
    }
    else {
        // Start searching upwards from existing position.
        offset = cur_start;
    }
    char tc = flat_string[offset];
    flat_string[offset] = 0;

    regex_t preg;
    if (regcomp(&preg, expression, case_insens ? REG_ICASE : 0)) {
        flat_string[offset] = tc;
        return (false);
    }
    int n = preg.re_nsub;
    regmatch_t *m = new regmatch_t[n+1];
    int ret = regexec(&preg, flat_string, 1, m, 0);
    char *st = flat_string;
    int last_os = 0;
    while (!ret) {
        st += m->rm_eo;
        regmatch_t *mlast = new regmatch_t[n+1];
        ret = regexec(&preg, st, 1, mlast, 0);
        if (!ret) {
            last_os += m->rm_eo;
            delete [] m;
            m = mlast;
        }
        else {
            delete [] mlast;
            ret = 0;
            break;
        }
    }
    if (!ret) {
        int start = last_os + m->rm_so;
        int end = last_os + m->rm_eo;
        cxt_t *cxs = cx_list->find(start);
        cxt_t *cxe = cxs->find(end-1);
        htmWord *ws = &cxs->obj->words[cxs->word_indx];
        htmWord *we = &cxe->obj->words[cxe->word_indx];
        int x1 = ws->area.x;
        int x2 = we->area.x;
        int l = x1 < x2 ? x1 : x2;
        x1 += ws->area.width;
        x2 += we->area.width;
        int r = x1 > x2 ? x1 : x2;
        int y1 = ws->area.y + 1;
        int y2 = we->area.y + 1;
        int t = y1 < y2 ? y1 : y2;
        int b = y1 > y2 ? y1 : y2;

        htm->selectRegion(l, t, r, b);
        if (htm->htm_if)
            htm->htm_if->scroll_visible(l, t, r, b);
        cur_start = start;
        cur_end = end;
    }
    delete [] m;
    regfree(&preg);
    flat_string[offset] = tc;
    return (ret == 0);
}


bool
htmWidget::findWords(const char *string, bool search_up, bool case_insens)
{
    if (!string || !*string) {
        clearSearch();
        return (true);
    }
    if (!htm_search)
        htm_search = new htmSearch;
    htm_search->load(htm_formatted, string);
    if (search_up)
        return (htm_search->up_match(this, case_insens));
    return (htm_search->down_match(this, case_insens));
}


void
htmWidget::clearSearch()
{
    delete htm_search;
    htm_search = 0;
}

