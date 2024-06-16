
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

#include "qtviewmon.h"
#include "qtviewer.h"
#include "htm/htm_widget.h"
#include "htm/htm_callback.h"


using namespace qtinterf;

QTviewmon::QTviewmon(QTviewer *prnt) : QObject(prnt)
{
    html_viewer = prnt;

/****XXX FIXME
    connect(html_viewer, &QTviewer::arm,
        this, &QTviewmon::arm_slot);
    connect(html_viewer, &QTviewer::activate,
        this, &QTviewmon::activate_slot);
    connect(html_viewer, &QTviewer::anchor_track,
        this, &QTviewmon::anchor_track_slot);
    connect(html_viewer, &QTviewer::anchor_visited,
        this, &QTviewmon::anchor_visited_slot);
    connect(html_viewer, &QTviewer::document,
        this, &QTviewmon::document_slot);
    connect(html_viewer, &QTviewer::link,
        this, &QTviewmon::link_slot);
    connect(html_viewer, &QTviewer::frame,
        this, &QTviewmon::frame_slot);
    connect(html_viewer, &QTviewer::form,
        this, &QTviewmon::form_slot);
    connect(html_viewer, &QTviewer::imagemap,
        this, &QTviewmon::imagemap_slot);
    connect(html_viewer, &QTviewer::html_event,
        this, &QTviewmon::html_event_slot);
*/
}


void
QTviewmon::arm_slot(htmCallbackInfo*)
{
    printf("arm called\n");
}


void
QTviewmon::activate_slot(htmAnchorCallbackStruct*)
{
    printf("activate called\n");
}


void
QTviewmon::anchor_track_slot(htmAnchorCallbackStruct*)
{
    printf("anchor_track called\n");
}


void
QTviewmon::anchor_visited_slot(htmVisitedCallbackStruct*)
{
    printf("anchor_visited called\n");
}


void
QTviewmon::document_slot(htmDocumentCallbackStruct*)
{
    printf("document called\n");
}


void
QTviewmon::link_slot(htmLinkCallbackStruct*)
{
    printf("link called\n");
}


void
QTviewmon::frame_slot(htmFrameCallbackStruct*)
{
    printf("frame called\n");
}


void
QTviewmon::form_slot(htmFormCallbackStruct *cbs)
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
QTviewmon::imagemap_slot(htmImagemapCallbackStruct*)
{
    printf("imagemap called\n");
}


void
QTviewmon::html_event_slot(htmEventCallbackStruct*)
{
    printf("event called\n");
}


