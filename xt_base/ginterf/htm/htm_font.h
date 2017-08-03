
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
 * MOZY html help viewer files                                            *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/*------------------------------------------------------------------------*
 * This file is part of the gtkhtm widget library.  The gtkhtm library
 * was derived from the gtk-xmhtml library by:
 *
 *   Stephen R. Whiteley  <srw@wrcad.com>
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

#ifndef HTM_FONT_H
#define HTM_FONT_H

#include "htm_hashtab.h"
#include "lstring.h"

namespace htm
{
    // Font style bits
    enum
    {
        FONT_BOLD           = 0x1,
        FONT_ITALIC         = 0x2,
        FONT_FIXED          = 0x4
    };
}

// A gtkhtm font. gtkhtm uses it's own font definition for performance
// reasons (the layout routines use a *lot* of font properties).
//
struct htmFont : public htmHashEnt
{
    htmFont(htmWidget*, const char*, int, unsigned char);
    ~htmFont();

    const char *font_name() { return (h_name); }
    const char *font_face() { return (ft_face); }

    // Do NOT call this if element is in table!
    void rename(const char *nm)
        {
            char *n = lstring::copy(nm);
            delete [] h_name;
            h_name = n;
        }

private:
    char            *ft_face;       // face name, derived from font name

public:
    unsigned char   style;          // this font's style
    int             ascent;         // font-wide ascent
    int             descent;        // font-wide descent
    int             lbearing;       // lbearing of largest character
    int             rbearing;       // rbearing of largest character
    int             width;          // width of largest character
    int             height;         // height of largest character
    int             lineheight;     // suggested lineheight
    unsigned int    isp;            // normal interword spacing
    unsigned int    eol_sp;         // additional end-of-line spacing
    int             sup_xoffset;    // additional superscript x-offset
    int             sup_yoffset;    // additional superscript y-offset
    int             sub_xoffset;    // additional subscript x-offset
    int             sub_yoffset;    // additional subscript y-offset
    int             ul_offset;      // additional underline offset
    unsigned int    ul_thickness;   // underline thickness
    int             st_offset;      // additional strikeout offset
    unsigned int    st_thickness;   // strikeout thickness
    void            *xfont;         // ptr to font definition
    htmWidget       *html;          // for destructor
};

// Font cache hash table
//
struct htmFontTab : public htmHashTab
{
};

#endif

