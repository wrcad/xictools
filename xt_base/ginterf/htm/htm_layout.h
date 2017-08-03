
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

#ifndef HTM_LAYOUT_H
#define HTM_LAYOUT_H

//-----------------------------------------------------------------------------
// layout Management

namespace htm
{
    struct TextCx;
    struct TableCx;

    // Object bounding box.  Used for recursive layout computations in
    // tables and text flowing around images.
    //
    struct PositionBox
    {
        PositionBox()
        {
            x           = 0;
            y           = 0;
            lmargin     = 0;
            rmargin     = 0;
            tmargin     = 0;
            bmargin     = 0;
            width       = 0;
            height      = 0;
            min_width   = 0;
            min_height  = 0;
            left        = 0;
            right       = 0;
            idx         = 0;
        }

        int x;                      // absolute box upper left x position
        int y;                      // absolute box upper left y position
        int lmargin;                // left margin
        int rmargin;                // right margin
        int tmargin;                // top margin
        int bmargin;                // bottom margin
        int width;                  // absolute box width
        int height;                 // absolute box height
        int min_width;              // minimum box width
        int min_height;             // minimum box height
        int left;                   // absolute left position
        int right;                  // absolute right position
        int idx;                    // index of cell using this box
    };

    struct Exclude
    {
        Exclude() { margin = 0; bottom = 0; next = 0; }

        static void destroy(Exclude *e)
            {
                while (e) {
                    Exclude *ex = e;
                    e = e->next;
                    delete ex;
                }
            }

        int margin;
        int bottom;
        Exclude *next;
    };

    struct Margins
    {
        Margins() { left = 0; right = 0; }
        ~Margins() { clear(); }

        void clear()
            {
                Exclude::destroy(left);
                left = 0;
                Exclude::destroy(right);
                right = 0;
            }

        Exclude *left;
        Exclude *right;
    };
}

struct htmLayoutManager
{
    friend struct htm::TextCx;
    friend struct htm::TableCx;

    htmLayoutManager(htmWidget*);

    int core(htmObjectTable*, htmObjectTable*, PositionBox*, PositionBox*,
        bool, bool, bool, bool);

    int max_width() { return (lm_max_width); }
    int line() { return (lm_line); }

private:
    void storeAnchor(htmObjectTable*);
    void justifyText(htmWord**, int, int, unsigned int, int, int,
        int, int);
    void checkAlignment(htmWord**, int, int, int, int, bool, int, int);
    void setText(PositionBox*, htmObjectTable*,
        htmObjectTable*, bool, bool, bool, Margins*, int*);
    void computeText(PositionBox*, htmWord**,
        int, int*, bool, bool, bool, Margins*);
    void setApplet(PositionBox*, htmObjectTable*);
    void setBlock(PositionBox*, htmObjectTable*);
    void setRule(PositionBox*, htmObjectTable*, Margins*);
    void setBullet(PositionBox*, htmObjectTable*, Margins*);
    void setBreak(PositionBox*, htmObjectTable*);
    htmObjectTable *setTable(PositionBox*, htmObjectTable*,
        bool);
    void computeTable(PositionBox*,
        htmObjectTable*, htmObjectTable*, bool, bool);
    void checkVerticalAlignment(int, PositionBox*, htmObjectTable*,
        htmObjectTable*, Alignment);
    htmWord **getWords(htmObjectTable*, htmObjectTable*, int*);
    void adjustBaseline(htmWord*, htmWord**, int, int, int*, bool);

    htmWidget       *lm_html;
    htmWord         *lm_baseline_obj;
    htmObjectTable  *lm_bullet;
    int             lm_line;
    int             lm_last_text_line;
    int             lm_max_width;
    int             lm_curr_anchor;
    int             lm_named_anchor;
    bool            lm_had_break;      // indicates a paragraph had a break
};

#endif

