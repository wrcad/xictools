
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

#ifndef QTGRID_H
#define QTGRID_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>

class QWidget;
class QDoubleSpinBox;


class cGridDlg : public QDialog, public QTbag, public QTdraw, public GRpopup
{
//    Q_OBJECT

public:
    cGridDlg(QTbag*, WindowDesc*);
    ~cGridDlg();

    // GRpopup override
    void set_visible(bool visib)
    {
        if (visib)
            show();
        else
            hide();
    }

    void popdown();
    void update(bool = false);
    void initialize();

private:
    /*
    static void gd_cancel_proc(GtkWidget*, void*);
    static int gd_key_hdlr(GtkWidget*, GdkEvent*event, void*);
    static void gd_apply_proc(GtkWidget*, void*);
    static void gd_snap_proc(GtkWidget*, void*);
    static void gd_resol_change_proc(GtkWidget*, void*);
    static void gd_snap_change_proc(GtkWidget*, void*);
    static void gd_edge_menu_proc(GtkWidget*, void*);
    static void gd_btn_proc(GtkWidget*, void*);
    static void gd_sto_menu_proc(GtkWidget*, void*);
    static void gd_rcl_menu_proc(GtkWidget*, void*);
    static void gd_axes_proc(GtkWidget*, void*);
    static void gd_lst_proc(GtkWidget*, void*);
    static void gd_cmult_change_proc(GtkWidget*, void*);
    static void gd_thresh_change_proc(GtkWidget*, void*);
    static void gd_crs_change_proc(GtkWidget*, void*);
#if GTK_CHECK_VERSION(3,0,0)
    static int gd_redraw_hdlr(GtkWidget*, cairo_t*, void*);
#else
    static int gd_redraw_hdlr(GtkWidget*, GdkEvent*, void*);
#endif
    static int gd_button_press_hdlr(GtkWidget*, GdkEvent*, void*);
    static int gd_button_release_hdlr(GtkWidget*, GdkEvent*, void*);
    static int gd_motion_hdlr(GtkWidget*, GdkEvent*, void*);
    static void gd_drag_data_get(GtkWidget*, GdkDragContext*,
        GtkSelectionData*, guint, guint, void*);
    */

    QWidget *gd_mfglabel;
    QWidget *gd_snapbox;
    QWidget *gd_snapbtn;
    QWidget *gd_edgegrp;
    QWidget *gd_edge;
    QWidget *gd_off_grid;
    QWidget *gd_use_nm_edge;
    QWidget *gd_wire_edge;
    QWidget *gd_wire_path;

    QWidget *gd_showbtn;
    QWidget *gd_topbtn;
    QWidget *gd_noaxesbtn;
    QWidget *gd_plaxesbtn;
    QWidget *gd_oraxesbtn;
    QWidget *gd_nocoarse;
    QWidget *gd_sample;
    QWidget *gd_solidbtn;
    QWidget *gd_dotsbtn;
    QWidget *gd_stipbtn;
    QWidget *gd_crs_frame;

    QWidget *gd_apply;
    QWidget *gd_cancel;

    GridDesc gd_grid;
    unsigned int gd_mask_bak;
    int gd_win_num;
    int gd_last_n;
    int gd_drag_x;
    int gd_drag_y;
    bool gd_dragging;

    QDoubleSpinBox *gd_resol;
    QDoubleSpinBox *gd_snap;
    QDoubleSpinBox *gd_cmult;
    QDoubleSpinBox *gd_thresh;
    QDoubleSpinBox *gd_crs;

    static cGridDlg *grid_pops[DSP_NUMWINS];
};

#endif

