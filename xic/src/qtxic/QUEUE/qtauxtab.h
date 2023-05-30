
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

#ifndef QTAUXTAB_H
#define QTAUXTAB_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>

class cAuxTab : public QDialog, public QTbag
{
    Q_OBJECT

public:
    cAuxTab(GRobject);
    ~cAuxTab();

    void update();

    static cAuxTab *self()          { return (instPtr); }

private slots:

private:
    void action_hdlr(GtkWidget*, void*);
    int button_hdlr(bool, GtkWidget*, GdkEvent*);
    int motion_hdlr(GtkWidget*, GdkEvent*);
    void select_range(int, int);
    void check_sens();

    /*
    static void at_cancel(GtkWidget*, void*);
    static void at_action_proc(GtkWidget*, void*);
    static ESret at_add_cb(const char*, void*);
    static void at_remove_cb(bool, void*);
    static void at_clear_cb(bool, void*);

    static void at_drag_data_get(GtkWidget *widget, GdkDragContext*,
        GtkSelectionData*, guint, guint, void*);
    static void at_drag_data_received(GtkWidget*, GdkDragContext*,
        gint, gint, GtkSelectionData*, guint, guint);
    static int at_btn_hdlr(GtkWidget*, GdkEvent*, void*);
    static int at_btn_release_hdlr(GtkWidget*, GdkEvent*, void*);
    static int at_motion_hdlr(GtkWidget*, GdkEvent*, void*);
    static void at_resize_hdlr(GtkWidget*, GtkAllocation*);
    static void at_realize_hdlr(GtkWidget*, void*);
    */

    GRobject at_caller;
    /*
    GtkWidget *at_addbtn;
    GtkWidget *at_rembtn;
    GtkWidget *at_clearbtn;
    GtkWidget *at_over;
    GtkWidget *at_skip;
    */
    GRledPopup *at_add_pop;
    GRaffirmPopup *at_remove_pop;
    GRaffirmPopup *at_clear_pop;

    int at_start;
    int at_end;
    int at_drag_x;
    int at_drag_y;
    bool at_dragging;

    static *cAuxTab instPtr;
};

#endif

