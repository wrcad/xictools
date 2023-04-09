
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

#ifndef VIEWMON_H
#define VIEWMON_H

#include <QObject>
#include "qtinterf/qtinterf.h"

struct htmCallbackInfo;
struct htmAnchorCallbackStruct;
struct htmVisitedCallbackStruct;
struct htmDocumentCallbackStruct;
struct htmLinkCallbackStruct;
struct htmFrameCallbackStruct;
struct htmFormCallbackStruct;
struct htmImagemapCallbackStruct;
struct htmEventCallbackStruct;

namespace qtinterf
{
    class viewer_w;

    class viewmon : public QObject
    {
        Q_OBJECT

    public:
        viewmon(viewer_w*);

    private slots:
        void arm_slot(htmCallbackInfo*);
        void activate_slot(htmAnchorCallbackStruct*);
        void anchor_track_slot(htmAnchorCallbackStruct*);
        void anchor_visited_slot(htmVisitedCallbackStruct*);
        void document_slot(htmDocumentCallbackStruct*);
        void link_slot(htmLinkCallbackStruct*);
        void frame_slot(htmFrameCallbackStruct*);
        void form_slot(htmFormCallbackStruct*);
        void imagemap_slot(htmImagemapCallbackStruct*);
        void html_event_slot(htmEventCallbackStruct*);

    private:
        viewer_w *html_viewer;
    };
}

#endif

