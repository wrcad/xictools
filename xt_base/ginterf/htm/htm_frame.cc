
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
#include "htm_frame.h"
#include "htm_parser.h"
#include "htm_tag.h"
#include "htm_callback.h"
#include "htm_string.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct GtkWidget;

#define ROW 1
#define COL 2

// max stacking depth
#define FSTACK_DEPTH 30

// min frame size
#define FMIN_SIZE 100


htmFrameWidget::htmFrameWidget(int type)
{
    fr_type             = type;
    fr_x                = 0;
    fr_y                = 0;
    fr_width            = 0;
    fr_height           = 0;
    fr_scroll_type      = FRAME_SCROLL_AUTO;
    fr_src              = 0;
    fr_name             = 0;
    fr_margin_width     = 0;
    fr_margin_height    = 0;
    fr_resize           = false;
    fr_border           = 0;
    fr_children         = 0;
    fr_next             = 0;
    fr_frametype        = 0;
    fr_nchildren        = 0;
    fr_sizes            = 0;
    fr_size_types       = 0;
}


htmFrameWidget::~htmFrameWidget()
{
    delete [] fr_name;
    delete [] fr_src;
    delete [] fr_sizes;
    delete [] fr_size_types;
}


// Fill an HTML frame structure with data from it's attributes.
//
htmFrameWidget*
htmFrameWidget::new_frame(int current_frame, const char *attributes)
{
    // create new entry
    htmFrameWidget *frame = new htmFrameWidget(HT_FRAME);

    int insert_pos;
    if (!fr_children) {
        insert_pos = 0;
        fr_children = frame;
    }
    else {
        htmFrameWidget *f = fr_children;
        insert_pos = 1;
        while (f->fr_next) {
            insert_pos++;
            f = f->fr_next;
        }
        if (insert_pos >= fr_nchildren) {
            delete frame;
            return (0);
        }
        f->fr_next = frame;
    }

    // pick up all remaining frame attributes
    frame->fr_src = htmTagGetValue(attributes, "src");

    // get frame name, default to _frame if not present
    if ((frame->fr_name = htmTagGetValue(attributes, "name")) == 0) {
        char buf[24];
        sprintf(buf, "_frame%i", current_frame);
        frame->fr_name = lstring::copy(buf);
    }

    frame->fr_margin_width = htmTagGetNumber(attributes, "marginwidth", 0);
    frame->fr_margin_height = htmTagGetNumber(attributes, "marginheight", 0);

    frame->fr_resize = !htmTagCheck(attributes, "noresize");
    frame->fr_scroll_type = FRAME_SCROLL_AUTO;

    char *chPtr;
    if ((chPtr = htmTagGetValue(attributes, "scrolling")) != 0) {
        if (!(strcasecmp(chPtr, "yes")))
            frame->fr_scroll_type = FRAME_SCROLL_YES;
        else if (!(strcasecmp(chPtr, "no")))
            frame->fr_scroll_type = FRAME_SCROLL_NONE;
        delete [] chPtr;
    }

    // set additional constraints for this frame
    frame->fr_border = fr_border;

    // disable resizing if we don't have a border
    if (!frame->fr_border)
        frame->fr_resize = false;

    return (frame);
}


// Create and fills a htmFrameWidget structure with the info in it's
// attributes.
//
htmFrameWidget*
htmFrameWidget::new_frame_set(const char *attributes)
{
    // create new entry
    htmFrameWidget *frameset = new htmFrameWidget(HT_FRAMESET);

    htmFrameWidget *fwt = this;
    if (fwt) {
        int insert_pos;
        if (!fr_children) {
            insert_pos = 0;
            fr_children = frameset;
        }
        else {
            htmFrameWidget *f = fr_children;
            insert_pos = 1;
            while (f->fr_next) {
                insert_pos++;
                f = f->fr_next;
            }
            if (insert_pos >= fr_nchildren) {
                delete frameset;
                return (0);
            }
            f->fr_next = frameset;
        }
    }

    frameset->fr_frametype = ROW;
    char *chPtr = htmTagGetValue(attributes, "rows");
    if (chPtr == 0) {
        chPtr = htmTagGetValue(attributes, "cols");
        if (chPtr == 0) {
            if (!fwt)
                delete frameset;
            return (0);
        }
        else
            frameset->fr_frametype = COL;
    }

    // Count how many children this frameset has:  the no of children
    // is given by the no of entries within the COLS or ROWS tag Note
    // that children can be frames and/or framesets as well.

    for (char *tmpPtr = chPtr; *tmpPtr; tmpPtr++) {
        if (*tmpPtr == ',')
            frameset->fr_nchildren++;
    }
    frameset->fr_nchildren++;

    frameset->fr_sizes = new int[frameset->fr_nchildren];
    frameset->fr_size_types = new FrameSize[frameset->fr_nchildren];

    // Get dimensions:  when we encounter a ``*'' in a size definition
    // it means we are free to choose any size we want.  When its a
    // number followed by a ``%'' we must choose the size relative
    // against the total width of the render area.  When it's a number
    // not followed by anything we have an absolute size.

    char *tmpPtr = chPtr;
    char *ptr = tmpPtr;
    int i = 0;
    while (true) {
        if (*tmpPtr == ',' || *tmpPtr == '\0') {
            if (*(tmpPtr-1) == '*')
                frameset->fr_size_types[i] = FRAME_SIZE_OPTIONAL;
            else if (*(tmpPtr-1) == '%')
                frameset->fr_size_types[i] = FRAME_SIZE_RELATIVE;
            else
                frameset->fr_size_types[i] = FRAME_SIZE_FIXED;

            frameset->fr_sizes[i++] = atoi(ptr);

            if (*tmpPtr == '\0')
                break;
            ptr = tmpPtr+1;
        }
        tmpPtr++;
        // sanity
        if (i == frameset->fr_nchildren)
            break;
    }
    delete [] chPtr;

    // Frame borders can be specified by both frameborder or border,
    // they are equal.

    chPtr = htmTagGetValue(attributes, "frameborder");
    if (chPtr) {

        // Sigh, stupid Netscape frameset definition allows a tag to
        // have a textvalue or a number.

        if (!(strcasecmp(chPtr, "no")) || *chPtr == '0')
            frameset->fr_border = 0;
        else
            frameset->fr_border = atoi(chPtr);
        delete [] chPtr;
    }
    else
        frameset->fr_border = htmTagGetNumber(attributes, "border", 0);

    return (frameset);
}


void
htmFrameWidget::layout()
{
    if (fr_type != HT_FRAMESET)
        return;

    int tsize = 0;
    int nopt = 0;
    int nch = 0;
    htmFrameWidget *f;
    if (fr_frametype == ROW) {
        for (f = fr_children; f; f = f->fr_next, nch++) {
            if (fr_size_types[nch] == FRAME_SIZE_FIXED)
                f->fr_height = fr_sizes[nch];
            else if (fr_size_types[nch] == FRAME_SIZE_RELATIVE)
                f->fr_height = (int)(fr_sizes[nch]*fr_height/100.0);
            else
                f->fr_height = 0;
            if (f->fr_height == 0)
                nopt++;
            else
                tsize += f->fr_height;
            f->fr_width = fr_width;
            f->fr_x = fr_x;
        }
        if (nopt > 0) {
            int del = fr_height - tsize;
            del /= nopt;
            if (del < FMIN_SIZE)
                del = FMIN_SIZE;
            for (f = fr_children; f; f = f->fr_next) {
                if (f->fr_height == 0)
                    f->fr_height = del;
            }
        }
        for (f = fr_children; f; f = f->fr_next) {
            if (f->fr_type == HT_FRAMESET) {
                f->layout();
                if (f->fr_width > fr_width)
                    fr_width = f->fr_width;
            }
        }
        tsize = fr_y;
        for (f = fr_children; f; f = f->fr_next) {
            f->fr_width = fr_width;
            f->fr_y = tsize;
            tsize += f->fr_height;
        }
        tsize -= fr_y;
        if (tsize > (int)fr_height)
            fr_height = tsize;
        for (f = fr_children; f; f = f->fr_next) {
            if (f->fr_type == HT_FRAMESET)
                f->layout();
        }
    }
    else if (fr_frametype == COL) {
        for (f = fr_children; f; f = f->fr_next, nch++) {
            if (fr_size_types[nch] == FRAME_SIZE_FIXED)
                f->fr_width = fr_sizes[nch];
            else if (fr_size_types[nch] == FRAME_SIZE_RELATIVE)
                f->fr_width = (int)(fr_sizes[nch]*fr_width/100.0);
            else
                f->fr_width = 0;
            if (f->fr_width == 0)
                nopt++;
            else
                tsize += f->fr_width;
            f->fr_height = fr_height;
            f->fr_y = fr_y;
        }
        if (nopt > 0) {
            int del = fr_width - tsize;
            del /= nopt;
            if (del < FMIN_SIZE)
                del = FMIN_SIZE;
            for (f = fr_children; f; f = f->fr_next) {
                if (f->fr_width == 0)
                    f->fr_width = del;
            }
        }
        for (f = fr_children; f; f = f->fr_next) {
            if (f->fr_type == HT_FRAMESET) {
                f->layout();
                if (f->fr_height > fr_height)
                    fr_height = f->fr_height;
            }
        }
        tsize = fr_x;
        for (f = fr_children; f; f = f->fr_next) {
            f->fr_height = fr_height;
            f->fr_x = tsize;
            tsize += f->fr_width;
        }
        tsize -= fr_x;
        if (tsize > (int)fr_width)
            fr_width = tsize;
        for (f = fr_children; f; f = f->fr_next) {
            if (f->fr_type == HT_FRAMESET)
                f->layout();
        }
    }
}
// End of htmFrameWidget functions


// Check if the given list of objects contains HTML frames, return a
// count of frames found.
//
int
htmWidget::checkForFrames(htmObject *objects)
{
    // Frames are not allowed to appear inside the BODY tag.  So we
    // never have to walk the entire contents of the current document
    // but simply break out of the loop once we encounter the <BODY>
    // tag.  This is a fairly huge performance increase.

    int nframes = 0;
    for (htmObject *tmp = objects; tmp && tmp->id != HT_BODY;
            tmp = tmp->next) {
        if (tmp->id == HT_FRAME)
            nframes++;
    }
    return (nframes);
}


// Create or update size/position of all frames.
//
bool
htmWidget::configureFrames()
{
    if (!htm_if || !htm_nframes)
        return (false);

    int reason = HTM_FRAMERESIZE;
    if (!htm_frames) {
        // create the list of HTML frame children
        htm_frames = new htmFrameWidget*[htm_nframes];
        memset(htm_frames, 0, htm_nframes*sizeof(htmFrameWidget*));

        // move to the first frameset declaration
        htmObject *tmp;
        for (tmp = htm_elements; tmp && tmp->id != HT_FRAMESET;
            tmp = tmp->next) ;

        // create all frames (and possibly nested framesets also)
        makeFrameSets(tmp);
        reason = HTM_FRAMECREATE;
    }

    // Compute the layout

    int nfs = 0;
    for (htmFrameWidget *f = htm_frameset; f; f = f->fr_next)
        nfs++;
    if (nfs == 0)
        return (false);  // can't happen

    htmRect rect;
    htm_if->frame_rendering_area(&rect);
    int work_height = rect.height/nfs;
    int extra = rect.height - nfs*work_height;

    int tsize = rect.top();
    for (htmFrameWidget *f = htm_frameset; f; f = f->fr_next) {
        f->fr_x = rect.left();;
        f->fr_y = tsize;
        f->fr_width = rect.width;
        f->fr_height = work_height;
        if (extra) {
            extra--;
            f->fr_height++;
        }
        f->layout();
        tsize += f->fr_height;
    }

    // and now create all frames
    htmFrameCallbackStruct cbs(reason);
    cbs.nframes = htm_nframes;
    cbs.frames = new htmFrameDataRec[htm_nframes];
    for (int i = 0; i < htm_nframes; i++) {
        htmFrameWidget *frame = htm_frames[i];
        cbs.frames[i].name = frame->fr_name;
        cbs.frames[i].src = frame->fr_src;
        cbs.frames[i].x = frame->fr_x + frame->fr_border;
        cbs.frames[i].y = frame->fr_y + frame->fr_border;
        cbs.frames[i].width = frame->fr_width - 2*frame->fr_border;
        cbs.frames[i].height = frame->fr_height - 2*frame->fr_border;
        cbs.frames[i].scroll_type = frame->fr_scroll_type;
    }
    htm_if->emit_signal(S_FRAME, &cbs);

    return (true);
}


// Frame destroyer.
//
void
htmWidget::destroyFrames()
{
    if (htm_frames) {
        if (htm_if) {
            htmFrameCallbackStruct cbs(HTM_FRAMEDESTROY);
            cbs.nframes = htm_nframes;
            cbs.frames = new htmFrameDataRec[htm_nframes];
            for (int i = 0; i < htm_nframes; i++) {
                cbs.frames[i].name = htm_frames[i]->fr_name;
                cbs.frames[i].src = htm_frames[i]->fr_src;
            }
            htm_if->emit_signal(S_FRAME, &cbs);
        }

        for (int i = 0; i < htm_nframes; i++)
            delete htm_frames[i];
        delete [] htm_frames;
        htm_frames = 0;
    }
    htm_nframes = 0;
}


void
htmWidget::makeFrameSets(htmObject *frameset)
{
    htmFrameWidget *stack[FSTACK_DEPTH];
    stack[0] = 0;
    int sp = 0;
    int frame_cnt = 0;
    int skipend = 0;
    for (htmObject *tmp = frameset; tmp; tmp = tmp->next) {
        if (tmp->id == HT_FRAMESET) {
            if (!tmp->is_end) {
                htmFrameWidget *f = (sp > 0 ? stack[sp-1] : 0);
                f = f->new_frame_set(tmp->attributes);
                if (f) {
                    if (sp < FSTACK_DEPTH)
                        stack[sp++] = f;
                    else
                        skipend++;
                }
                else {

                    // No more room available, this is an unspecified
                    // frameset, kill it and all children it might have.

                    int depth = 1;
                    int start_line = tmp->line;
                    skipend++;
                    for (tmp = tmp->next; tmp != 0; tmp = tmp->next) {
                        if (tmp->id == HT_FRAMESET) {
                            if (tmp->is_end) {
                                if (--depth == 0)
                                    break;
                            }
                            else
                                // child frameset
                                depth++;
                        }
                    }
                    warning("makeFrameSets",
                        "Bad <FRAMESET> tag: missing COLS or ROWS "
                        "attribute on parent set\n    Skipped all "
                        "frame declarations between line %d and %d "
                        "in input.", start_line, tmp ? (int)tmp->line : -1);
                    if (!tmp)
                        break;
                }
            }
            else {
                if (!skipend) {
                    if (sp) {
                        int nch = 0;
                        htmFrameWidget *f;
                        for (f = stack[sp-1]->fr_children; f; f = f->fr_next)
                            nch++;
                        stack[sp-1]->fr_nchildren = nch;
                        sp--;
                    }
                }
                else
                    skipend--;
            }
        }
        else if (tmp->id == HT_FRAME) {
            if (sp > 0) {
                htmFrameWidget *f =
                    stack[sp-1]->new_frame(frame_cnt, tmp->attributes);
                if (f) {
                    htm_frames[frame_cnt] = f;
                    frame_cnt++;
                }
                else {
                    warning("makeFrameSets",
                        "Bad <FRAME> tag on line %i of input: missing COLS "
                        "or ROWS attribute on parent set, skipped.",
                        tmp->line);
                }
            }
        }
        if (frame_cnt == htm_nframes)
            break;
    }
    htm_nframes = frame_cnt;
    htm_frameset = stack[0];
}

