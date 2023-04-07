
/*========================================================================
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
 * Qt MOZY help viewer.
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "viewmon.h"
#include "qtviewer.h"
#include "htm/htm_widget.h"
#include "htm/htm_callback.h"


using namespace qtinterf;

viewmon::viewmon(viewer_w *prnt) : QObject(prnt)
{
    html_viewer = prnt;

    connect(html_viewer, SIGNAL(arm(htmCallbackInfo*)),
        this, SLOT(arm_slot(htmCallbackInfo*)));
    connect(html_viewer, SIGNAL(activate(htmAnchorCallbackStruct*)),
        this, SLOT(activate_slot(htmAnchorCallbackStruct*)));
    connect(html_viewer, SIGNAL(anchor_track(htmAnchorCallbackStruct*)),
        this, SLOT(anchor_track_slot(htmAnchorCallbackStruct*)));
    connect(html_viewer, SIGNAL(anchor_visited(htmVisitedCallbackStruct*)),
        this, SLOT(anchor_visited_slot(htmVisitedCallbackStruct*)));
    connect(html_viewer, SIGNAL(document(htmDocumentCallbackStruct*)),
        this, SLOT(document_slot(htmDocumentCallbackStruct*)));
    connect(html_viewer, SIGNAL(link(htmLinkCallbackStruct*)),
        this, SLOT(link_slot(htmLinkCallbackStruct*)));
    connect(html_viewer, SIGNAL(frame(htmFrameCallbackStruct*)),
        this, SLOT(frame_slot(htmFrameCallbackStruct*)));
    connect(html_viewer, SIGNAL(form(htmFormCallbackStruct*)),
        this, SLOT(form_slot(htmFormCallbackStruct*)));
    connect(html_viewer, SIGNAL(imagemap(htmImagemapCallbackStruct*)),
        this, SLOT(imagemap_slot(htmImagemapCallbackStruct*)));
    connect(html_viewer, SIGNAL(html_event(htmEventCallbackStruct*)),
        this, SLOT(html_event_slot(htmEventCallbackStruct*)));
}


void
viewmon::arm_slot(htmCallbackInfo*)
{
    printf("arm called\n");
}


void
viewmon::activate_slot(htmAnchorCallbackStruct*)
{
    printf("activate called\n");
}


void
viewmon::anchor_track_slot(htmAnchorCallbackStruct*)
{
    printf("anchor_track called\n");
}


void
viewmon::anchor_visited_slot(htmVisitedCallbackStruct*)
{
    printf("anchor_visited called\n");
}


void
viewmon::document_slot(htmDocumentCallbackStruct*)
{
    printf("document called\n");
}


void
viewmon::link_slot(htmLinkCallbackStruct*)
{
    printf("link called\n");
}


void
viewmon::frame_slot(htmFrameCallbackStruct*)
{
    printf("frame called\n");
}


void
viewmon::form_slot(htmFormCallbackStruct *cbs)
{
    printf("form called\n");
    printf("action: %s\n", cbs->action ? cbs->action : "null");
    printf("enc type: %s\n", cbs->enctype ? cbs->enctype : "null");
    printf("method: %d\n", cbs->method);
    printf("components: %d\n", cbs->ncomponents);
    for (int i = 0; i < cbs->ncomponents; i++) {
        printf("  name: %s\n", cbs->components[i].name ?
            cbs->components[i].name : "null");
        printf("  value: %s\n", cbs->components[i].value ?
            cbs->components[i].value : "null");
    }
}


void
viewmon::imagemap_slot(htmImagemapCallbackStruct*)
{
    printf("imagemap called\n");
}


void
viewmon::html_event_slot(htmEventCallbackStruct*)
{
    printf("event called\n");
}


