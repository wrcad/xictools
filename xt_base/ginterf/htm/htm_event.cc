
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
#include "htm_format.h"
#include "htm_tag.h"
#include <string.h>


namespace {
    const char *const event_names[HTM_USEREVENT] =
    {
        "onload", "onunload", "onsubmit", "onreset", "onfocus", "onblur",
        "onselect", "onchange", "onclick", "ondblclick", "onmousedown",
        "onmouseup", "onmouseover", "onmousemove", "onmouseout",
        "onkeypress", "onkeydown", "onkeyup"
    };
}


AllEvents*
htmWidget::checkCoreEvents(const char *attr)
{
    AllEvents events;
    AllEvents *events_return = 0;
    bool have_events = false;

    // process all possible core events
    if ((events.onClick = checkEvent(HTM_CLICK, attr)) != 0)
        have_events = true;
    if ((events.onDblClick = checkEvent(HTM_DOUBLE_CLICK, attr)) != 0)
        have_events = true;
    if ((events.onMouseDown = checkEvent(HTM_MOUSEDOWN, attr)) != 0)
        have_events = true;
    if ((events.onMouseUp = checkEvent(HTM_MOUSEUP, attr)) != 0)
        have_events = true;
    if ((events.onMouseOver = checkEvent(HTM_MOUSEOVER, attr)) != 0)
        have_events = true;
    if ((events.onMouseMove = checkEvent(HTM_MOUSEMOVE, attr)) != 0)
        have_events = true;
    if ((events.onMouseOut = checkEvent(HTM_MOUSEOUT, attr)) != 0)
        have_events = true;
    if ((events.onKeyPress = checkEvent(HTM_KEYPRESS, attr)) != 0)
        have_events = true;
    if ((events.onKeyDown = checkEvent(HTM_KEYDOWN, attr)) != 0)
        have_events = true;
    if ((events.onKeyUp = checkEvent(HTM_KEYUP, attr)) != 0)
        have_events = true;

    // alloc & copy if we found any events
    if (have_events)
        events_return = new AllEvents(events);
    return (events_return);
}


AllEvents*
htmWidget::checkBodyEvents(const char *attributes)
{
    AllEvents events;
    AllEvents *events_return = 0;
    bool have_events = false;

    // check core events
    if ((events_return = checkCoreEvents(attributes)) != 0)
        have_events = true;

    // process all possible body events
    if ((events.onLoad = checkEvent(HTM_LOAD, attributes)) != 0)
        have_events = true;
    if ((events.onUnload= checkEvent(HTM_UNLOAD, attributes)) != 0)
        have_events = true;

    // alloc & copy if we found any events
    if (have_events) {
        if (!events_return)
            // no core events found
            events_return = new AllEvents(events);
        else {
            // has got core events as well
            events_return->onLoad   = events.onLoad;
            events_return->onUnload = events.onUnload;
        }
    }
    return (events_return);
}


AllEvents*
htmWidget::checkFormEvents(const char *attributes)
{
    AllEvents events;
    AllEvents *events_return = 0;
    bool have_events = false;

    // check core events
    if ((events_return = checkCoreEvents(attributes)) != 0)
        have_events = true;

    // process all possible form events
    if ((events.onSubmit = checkEvent(HTM_SUBMIT, attributes)) != 0)
        have_events = true;
    if ((events.onReset = checkEvent(HTM_RESET, attributes)) != 0)
        have_events = true;
    if ((events.onFocus = checkEvent(HTM_FOCUS, attributes)) != 0)
        have_events = true;
    if ((events.onBlur = checkEvent(HTM_BLUR, attributes)) != 0)
        have_events = true;
    if ((events.onSelect = checkEvent(HTM_SELECT, attributes)) != 0)
        have_events = true;
    if ((events.onChange = checkEvent(HTM_CHANGE, attributes)) != 0)
        have_events = true;

    // alloc & copy if we found any events
    if (have_events) {
        if (!events_return)
            // no core events found
            events_return = new AllEvents(events);
        else {
            // has got core events as well
            events_return->onSubmit = events.onSubmit;
            events_return->onReset  = events.onReset;
            events_return->onFocus  = events.onFocus;
            events_return->onBlur   = events.onBlur;
            events_return->onSelect = events.onSelect;
            events_return->onChange = events.onChange;
        }
    }
    return (events_return);
}


// Call the eventCallback callback resource for the given event.
//
void
htmWidget::processEvent(htmEvent *event, HTEvent *ht_event)
{
    if (htm_if) {
        htmEventCallbackStruct cbs;
        cbs.event = event;
        cbs.type  = ht_event->type;
        cbs.data  = ht_event->data;
        htm_if->emit_signal(S_HTML_EVENT, &cbs);
    }
}


// Destroy all registered events.  This routine is called when the
// current document is being unloaded.
//
void
htmWidget::freeEventDatabase()
{
    if (htm_if) {
        for (int i = 0; i < htm_nevents; i++) {
            htmEventCallbackStruct cbs;
            cbs.reason = HTM_EVENTDESTROY;
            cbs.event  = 0;
            cbs.type   = htm_events[i].type;
            cbs.data   = htm_events[i].data;
            htm_if->emit_signal(S_HTML_EVENT, &cbs);
        }
    }
    delete [] htm_events;
    htm_events = 0;
    htm_nevents = 0;
}


HTEvent *
htmWidget::storeEvent(int type, void* data)
{
    // check event array to see if we've already got an event with the
    // same data
    for (int i = 0; i < htm_nevents; i++)
        if (htm_events[i].data == data)
            return (&htm_events[i]);

    // not yet in event array
    if (htm_nevents) {
        HTEvent *te = htm_events;
        htm_events = new HTEvent[htm_nevents+1];
        memcpy(htm_events, te, htm_nevents*sizeof(HTEvent));
        delete [] te;
    }
    else
        // no event array yet
        htm_events  = new HTEvent;

    htm_events[htm_nevents].type = type;
    htm_events[htm_nevents].data = data;
    htm_nevents++;
    return (&htm_events[htm_nevents-1]);
}


HTEvent *
htmWidget::checkEvent(int type, const char *attributes)
{
    if (htm_if) {
        char *chPtr = htmTagGetValue(attributes, event_names[type]);
        if (chPtr) {
            void *data = htm_if->event_proc(chPtr);
            delete [] chPtr;
            if (data)
                return (storeEvent(type, data));
        }
    }
    return (0);
}

