
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

#ifndef GTKPRPTY_H
#define GTKPRPTY_H

struct sPbase : public gtk_bag
{
    sPbase()
        {
            pi_line_selected = -1;
            pi_list = 0;
            pi_odesc = 0;
            pi_btn_callback = 0;
            pi_start = 0;
            pi_end = 0;
            pi_drag_x = 0;
            pi_drag_y = 0;
            pi_dragging = false;
        }
    virtual ~sPbase() { PrptyText::destroy(pi_list); }

    PrptyText *resolve(int, CDo**);

    static sPbase *prptyInfoPtr();

protected:
    PrptyText *get_selection();
    void update_display();
    void select_range(int, int);
    int button_dn(GtkWidget*, GdkEvent*, void*);
    int button_up(GtkWidget*, GdkEvent*, void*);
    int motion(GtkWidget*, GdkEvent*, void*);
    void drag_data_get(GtkSelectionData*);
    void data_received(GtkWidget*, GdkDragContext*, GtkSelectionData*, guint);

    static void pi_cancel_proc(GtkWidget*, void*);
    static void pi_font_changed();
    static int pi_bad_cb(void*);
    static int pi_text_btn_hdlr(GtkWidget*, GdkEvent*, void*);
    static int pi_text_btn_release_hdlr(GtkWidget*, GdkEvent*, void*);
    static int pi_motion_hdlr(GtkWidget*, GdkEvent*, void*);
    static void pi_drag_data_get(GtkWidget*, GdkDragContext*,
        GtkSelectionData*, guint, guint, void*);
    static void pi_drag_data_received(GtkWidget*, GdkDragContext*,
        gint x, gint y, GtkSelectionData*, guint, guint);

    int pi_line_selected;
    PrptyText *pi_list;
    CDo *pi_odesc;
    int(*pi_btn_callback)(PrptyText*);
    int pi_start;
    int pi_end;
    int pi_drag_x;
    int pi_drag_y;
    bool pi_dragging;
};

#endif

