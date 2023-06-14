
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef QTPRPTY_H
#define QTPRPTY_H

#include "main.h"
#include "edit.h"
#include "qtmain.h"

class QMimeData;

class cPrpBase : public QTbag
{
public:
    cPrpBase()
    {
        pb_line_selected = -1;
        pb_list = 0;
        pb_odesc = 0;
        pb_btn_callback = 0;
        pb_start = 0;
        pb_end = 0;
        pb_drag_x = 0;
        pb_drag_y = 0;
        pb_dragging = false;
    }
    virtual ~cPrpBase()         { PrptyText::destroy(pb_list); }

    PrptyText *resolve(int, CDo**);

    static cPrpBase *prptyInfoPtr();

protected:
    PrptyText *get_selection();
    void update_display();
    void select_range(int, int);
    void handle_button_down(QMouseEvent*);
    void handle_button_up(QMouseEvent*);
    void handle_mouse_motion(QMouseEvent*);
    void handle_mime_data_received(const QMimeData*);

    static int pb_bad_cb(void*);

    int         pb_line_selected;
    PrptyText   *pb_list;
    CDo         *pb_odesc;
    int         (*pb_btn_callback)(PrptyText*);
    int         pb_start;
    int         pb_end;
    int         pb_drag_x;
    int         pb_drag_y;
    bool        pb_dragging;
};

#endif

