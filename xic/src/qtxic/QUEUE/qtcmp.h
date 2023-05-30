
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

#ifndef QTCMP_H
#define QTCMP_H

#include "main.h"
#include "qtmain.h"

#include <QDialog>


class cCompare : public QDialog
{
    Q_OBJECT

public:
    friend struct sCmp_store;
    cCompare(GRobject);
    ~cCompare();

    void update();

    sCompare *self()            { return (instPtr); }

private slots:

private:
    void per_cell_obj_page();
    void per_cell_geom_page();
    void flat_geom_page();
    void p1_sens();
    char *compose_arglist();
    bool get_bb(BBox*);
    void set_bb(const BBox*);

    /*
    static void cmp_cancel_proc(GtkWidget*, void*);
    static void cmp_action(GtkWidget*, void*);
    static void cmp_p1_action(GtkWidget*, void*);
    static void cmp_p1_fltr_proc(GtkWidget*, void*);
    static int cmp_popup_btn_proc(GtkWidget*, GdkEvent*, void*);
    static void cmp_sto_menu_proc(GtkWidget*, void*);
    static void cmp_rcl_menu_proc(GtkWidget*, void*);
    static void cmp_drag_data_received(GtkWidget*, GdkDragContext*,
        gint, gint, GtkSelectionData*, guint, guint);
    */

    GRobject cmp_caller;
    GtkWidget *cmp_popup;
    GtkWidget *cmp_mode;
    GtkWidget *cmp_fname1;
    GtkWidget *cmp_fname2;
    GtkWidget *cmp_cnames1;
    GtkWidget *cmp_cnames2;
    GtkWidget *cmp_diff_only;
    GtkWidget *cmp_layer_list;
    GtkWidget *cmp_layer_use;
    GtkWidget *cmp_layer_skip;
    GTKspinBtn sb_max_errs;

    GtkWidget *cmp_p1_expand_arrays;
    GtkWidget *cmp_p1_slop;
    GtkWidget *cmp_p1_dups;
    GtkWidget *cmp_p1_boxes;
    GtkWidget *cmp_p1_polys;
    GtkWidget *cmp_p1_wires;
    GtkWidget *cmp_p1_labels;
    GtkWidget *cmp_p1_insta;
    GtkWidget *cmp_p1_boxes_prp;
    GtkWidget *cmp_p1_polys_prp;
    GtkWidget *cmp_p1_wires_prp;
    GtkWidget *cmp_p1_labels_prp;
    GtkWidget *cmp_p1_insta_prp;
    GtkWidget *cmp_p1_recurse;
    GtkWidget *cmp_p1_phys;
    GtkWidget *cmp_p1_elec;
    GtkWidget *cmp_p1_cell_prp;
    GtkWidget *cmp_p1_fltr;
    GtkWidget *cmp_p1_setup;
    PrpFltMode cmp_p1_fltr_mode;

    GtkWidget *cmp_p2_expand_arrays;
    GtkWidget *cmp_p2_recurse;
    GtkWidget *cmp_p2_insta;

    GtkWidget *cmp_p3_aoi_use;
    GtkWidget *cmp_p3_s_btn;
    GtkWidget *cmp_p3_r_btn;
    GtkWidget *cmp_p3_s_menu;
    GtkWidget *cmp_p3_r_menu;
    GTKspinBtn sb_p3_aoi_left;
    GTKspinBtn sb_p3_aoi_bottom;
    GTKspinBtn sb_p3_aoi_right;
    GTKspinBtn sb_p3_aoi_top;
    GTKspinBtn sb_p3_fine_grid;
    GTKspinBtn sb_p3_coarse_mult;

    static cCompare *instPtr;
};
#endif
