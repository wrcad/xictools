
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

#include "htm_widget.h"
#include "htm_font.h"
#include "htm_string.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>


//-----------------------------------------------------------------------------
// Font access definitions

namespace {
    // Create a string that we use as the hash tag for fonts.  This has
    // three colon-separated fields:  a face name or comma-separated list
    // of face names, the integer pixel size, and the integer style flags.
    // What ever font the allocator finds for this name will be cached
    // under this name.
    //
    char *
    mkname(const char *face, int size, int style)
    {
        if (!face)
            return (0);
        while (isspace(*face) || *face == '"' || *face == '\'')
            face++;
        int bsz = strlen(face) + 8;
        char *buf = new char[bsz];
        strcpy(buf, face);
        char *t = buf + strlen(buf) - 1;
        while (t >= buf && (isspace(*t) || *t == '"' || *t == '\''))
            *t-- = 0;
        if (t < buf) {
            delete [] buf;
            return (0);
        }
        style &= (FONT_BOLD | FONT_ITALIC | FONT_FIXED);
        int len = strlen(buf);
        snprintf(buf + len, bsz - len, ":%d:%d", size, style);
        return (buf);
    }
}


htmFont::htmFont(htmWidget *h, const char *face, int pixsize,
    unsigned char st) : htmHashEnt(mkname(face, pixsize, st))
{
    style           = st;

    // save the 'face' field independently for convenience
    const char *t = strchr(h_name, ':');
    ft_face = new char[t - h_name + 1];
    strncpy(ft_face, h_name, t - h_name);
    ft_face[t - h_name] = 0;

    ascent          = 0;
    descent         = 0;
    lbearing        = 0;
    rbearing        = 0;
    width           = 0;
    height          = 0;
    lineheight      = 0;
    isp             = 0;
    eol_sp          = 0;
    sup_xoffset     = 0;
    sup_yoffset     = 0;
    sub_xoffset     = 0;
    sub_yoffset     = 0;
    ul_offset       = 0;
    ul_thickness    = 0;
    st_offset       = 0;
    st_thickness    = 0;
    xfont           = 0;
    html            = h;
}


htmFont::~htmFont()
{
    delete [] h_name;
    delete [] ft_face;
    html->htm_tk->tk_release_font(xfont);
}
// End of htmFont functions


// Obtain a font from the cache or the system.  The face can actually
// be a comma-separated list of face names, tk_alloc_font must deal
// with this, or an XFD.
//
htmFont *
htmWidget::findFont(const char *face, int pixsize, unsigned char style)
{
    if (!htm_font_cache)
        htm_font_cache = new htmFontTab;
    char *nm = mkname(face, pixsize, style);
    htmFont *font = (htmFont*)htm_font_cache->get(nm);
    if (!font) {
        font = htm_tk->tk_alloc_font(face, pixsize, style);
        // The font returned may have different name.  Change the name
        // so caching will work.
        if (strcmp(nm, font->font_name()))
            font->rename(nm);
        htm_font_cache->add(font);
    }
    delete [] nm;
    return (font);
}


// Set and return the default font.
//
htmFont*
htmWidget::loadDefaultFont()
{
    htm_default_font = findFont(htm_font_family,
        htm_font_sizes.font_size(3, false), 0);
    return (htm_default_font);
}


// Loads a new font, with the style determined by the current font:
// if current font is bold, and new is italic then a bold-italic font
// will be returned.
//
htmFont*
htmWidget::loadFont(htmlEnum font_id, int size, htmFont *curr_font)
{
    unsigned char new_style = curr_font->style & (FONT_BOLD | FONT_ITALIC);
    const char *family;
    int pixsize;
    if (curr_font->style & FONT_FIXED) {
        new_style |= FONT_FIXED;
        family = htm_font_family_fixed;
        pixsize = htm_font_sizes.font_size(size, true);
    }
    else {
        family = curr_font->font_face();
        pixsize = htm_font_sizes.font_size(size, false);
    }

    htmFont *new_font = 0;
    switch (font_id) {
    case HT_CITE:
    case HT_I:
    case HT_EM:
    case HT_DFN:
    case HT_ADDRESS:
        new_style |= FONT_ITALIC;
        new_font = findFont(family, htm_font_sizes.font_size(size, false),
            new_style);
        break;
    case HT_STRONG:
    case HT_B:
    case HT_CAPTION:
        new_style |= FONT_BOLD;
        new_font = findFont(family, htm_font_sizes.font_size(size, false),
            new_style);
        break;

    // Fixed fonts always use the font specified by the value of the
    // fontFamilyFixed resource.

    case HT_SAMP:
    case HT_TT:
    case HT_VAR:
    case HT_CODE:
    case HT_KBD:
    case HT_PRE:
        new_style |= FONT_FIXED;
        new_font = findFont(htm_font_family_fixed,
            htm_font_sizes.font_size(size, true), new_style);
        break;

    // The <FONT> element is useable in every state
    case HT_FONT:
        new_font = findFont(family, htm_font_sizes.font_size(size, false),
            new_style);
        break;

    // Since HTML Headings may not occur inside a <font></font>
    // declaration, they *must* use the specified document font, and
    // not derive their true font from the current font.

    case HT_H1:
    case HT_H2:
    case HT_H3:
    case HT_H4:
    case HT_H5:
    case HT_H6:
        size = (HT_H6 - font_id) + 2;
        new_font = findFont(htm_font_family, htm_font_sizes.font_size(size,
            false), FONT_BOLD);
        break;

    // should never be reached
    default:
        // this will always succeed
        new_font = findFont(family, pixsize, 0);
        break;
    }
    return (new_font);
}


// Load a new font with given pixel size and face.  Style is determined
// by the current font:  if current font is bold, and new is italic
// then a bold-italic font will be returned.
//
// Note that face may actually be a comma-separated list of face names,
// findFont must deal with this.
//
htmFont*
htmWidget::loadFontWithFace(int size, const char *face, htmFont *curr_font)
{
    unsigned char new_style = curr_font->style & (FONT_BOLD | FONT_ITALIC);
    const char *family;
    if (curr_font->style & FONT_FIXED) {
        new_style |= FONT_FIXED;
        family = htm_font_family_fixed;
    }
    else
        family = face;

    return (findFont(family, htm_font_sizes.font_size(size, false),
        new_style));
}

