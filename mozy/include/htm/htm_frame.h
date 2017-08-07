
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

#ifndef HTM_FRAME_H
#define HTM_FRAME_H

namespace htm
{
    // Possible types of frame sizes
    enum FrameSize
    {
        FRAME_SIZE_FIXED = 1,           // size specified in pixels
        FRAME_SIZE_RELATIVE,            // size is relative
        FRAME_SIZE_OPTIONAL             // size is optional
    };
}

// Definition of frameset/frame (shared)
//
struct htmFrameWidget
{
    htmFrameWidget(int);
    ~htmFrameWidget();

    htmFrameWidget *new_frame(int, const char*);
    htmFrameWidget *new_frame_set(const char*);
    void layout();

    int             fr_type;            // HT_FRAME or HT_FRAMESET
    int             fr_x;               // computed frame x-position
    int             fr_y;               // computed frame y-position
    unsigned int    fr_width;           // computed frame width
    unsigned int    fr_height;          // computed frame height
    FrameScrolling  fr_scroll_type;     // frame scrolling
    char            *fr_src;            // source document
    char            *fr_name;           // internal frame name
    unsigned int    fr_margin_width;    // frame margin width
    unsigned int    fr_margin_height;   // frame margin height
    bool            fr_resize;          // may we resize this frame?
    int             fr_border;          // add a border around this frame?
    htmFrameWidget  *fr_children;       // frames of this frameset
    htmFrameWidget  *fr_next;           // next frame or frameset
    int             fr_frametype;       // rows or columns
    int             fr_nchildren;       // number of children
    int             *fr_sizes;          // array of child sizes
    FrameSize       *fr_size_types;     // array of possible size specs
};

// Data record for callbacks.
//
struct htmFrameDataRec
{
    htmFrameDataRec()
    {
        src     = 0;
        name    = 0;
        doit    = true;
        x       = 0;
        y       = 0;
        width   = 0;
        height  = 0;
        scroll_type = FRAME_SCROLL_AUTO;
    }

    const char *src;    // requested document
    const char *name;   // frame name
    bool doit;          // destroy/create vetoing flag
    int x;              // frame position and size
    int y;
    int width;
    int height;
    FrameScrolling scroll_type;     // frame scrolling
};

// Callback structure.  This callback is activated when
// one of the following events occurs:
// 1.  the widget wants to create frames, reason = HTM_FRAMECREATE
//     can be veto'd by setting doit to FALSE.
// 2.  the widget wants to destroy frames, reason = HTM_FRAMEDESTROY
//     can be veto'd by setting doit to FALSE.
// 3.  the widget needs to resize rames, reason = HTM_FRAMERESIZE.
//
struct htmFrameCallbackStruct
{
    htmFrameCallbackStruct(int r)
        {
            reason  = r;
            nframes = 0;
            frames  = 0;
        }

    ~htmFrameCallbackStruct()
        {
            delete [] frames;
        }

    int reason;             // the reason the callback was called
    int nframes;            // frame count
    htmFrameDataRec *frames;  // frame data
};

#endif

