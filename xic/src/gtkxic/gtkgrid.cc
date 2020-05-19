
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

#include "main.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "errorlog.h"
#include "tech.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkspinbtn.h"
#include <gdk/gdkkeysyms.h>


// The Grid Setup panel, used to control the grid in each drawing
// window.
//
// Help keyword used: xic:grid

namespace gtkgrid {
    struct sGrd : public gtk_bag, public gtk_draw, public GRpopup
    {
        sGrd(gtk_bag*, WindowDesc*);
        ~sGrd();

        // GRpopup override
        void set_visible(bool visib)
            {
                if (visib)
                    gtk_widget_show(wb_shell);
                else
                    gtk_widget_hide(wb_shell);
            }

        void popdown();
        void update(bool = false);
        void initialize();

    private:
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
        static int gd_redraw_hdlr(GtkWidget*, GdkEvent*, void*);
        static int gd_button_press_hdlr(GtkWidget*, GdkEvent*, void*);
        static int gd_button_release_hdlr(GtkWidget*, GdkEvent*, void*);
        static int gd_motion_hdlr(GtkWidget*, GdkEvent*, void*);
        static void gd_drag_data_get(GtkWidget*, GdkDragContext*,
            GtkSelectionData*, guint, guint, void*);

        GtkWidget *gd_mfglabel;
        GtkWidget *gd_snapbox;
        GtkWidget *gd_snapbtn;
        GtkWidget *gd_edgegrp;
        GtkWidget *gd_edge;
        GtkWidget *gd_off_grid;
        GtkWidget *gd_use_nm_edge;
        GtkWidget *gd_wire_edge;
        GtkWidget *gd_wire_path;

        GtkWidget *gd_showbtn;
        GtkWidget *gd_topbtn;
        GtkWidget *gd_noaxesbtn;
        GtkWidget *gd_plaxesbtn;
        GtkWidget *gd_oraxesbtn;
        GtkWidget *gd_nocoarse;
        GtkWidget *gd_sample;
        GtkWidget *gd_solidbtn;
        GtkWidget *gd_dotsbtn;
        GtkWidget *gd_stipbtn;
        GtkWidget *gd_crs_frame;

        GtkWidget *gd_apply;
        GtkWidget *gd_cancel;

        GridDesc gd_grid;
        unsigned int gd_mask_bak;
        int gd_win_num;
        int gd_last_n;
        int gd_drag_x;
        int gd_drag_y;
        bool gd_dragging;

        GTKspinBtn sb_resol;
        GTKspinBtn sb_snap;
        GTKspinBtn sb_cmult;
        GTKspinBtn sb_thresh;
        GTKspinBtn sb_crs;
    };

    enum { LstSolid, LstDots, LstStip };

    const char *edgevals[] =
    {
        "DISABLED",
        "Enabled in some commands",
        "Enabled always",
        0
    };

    sGrd *grid_pops[DSP_NUMWINS];
}

#define REVERT "revert"
#define LASTAPPL "last appl"

using namespace gtkgrid;


void
win_bag::PopUpGrid(GRobject caller, ShowMode mode)
{
    if (mode == MODE_OFF) {
        delete wib_gridpop;
        return;
    }
    if (mode == MODE_UPD) {
        if (wib_gridpop)
            wib_gridpop->update();
        return;
    }
    if (wib_gridpop)
        return;
    if (!mainBag())
        return;

    wib_gridpop = new sGrd(this, wib_windesc);
    wib_gridpop->register_usrptr((void**)&wib_gridpop);
    if (!wib_gridpop->Shell()) {
        delete wib_gridpop;
        wib_gridpop = 0;
        return;
    }
    wib_gridpop->register_caller(caller);
    wib_gridpop->initialize();
    wib_gridpop->set_visible(true);
}
// End of win_bag functions.


sGrd::sGrd(gtk_bag *owner, WindowDesc *wd)
{
    p_parent = owner;
    gd_mfglabel = 0;
    gd_snapbox = 0;
    gd_snapbtn = 0;
    gd_edgegrp = 0;
    gd_edge = 0;
    gd_off_grid = 0;
    gd_use_nm_edge = 0;
    gd_wire_edge = 0;
    gd_wire_path = 0;

    gd_showbtn = 0;
    gd_topbtn = 0;
    gd_noaxesbtn = 0;
    gd_plaxesbtn = 0;
    gd_oraxesbtn = 0;
    gd_nocoarse = 0;
    gd_sample = 0;
    gd_solidbtn = 0;
    gd_dotsbtn = 0;
    gd_stipbtn = 0;
    gd_crs_frame = 0;

    gd_apply = 0;
    gd_cancel = 0;

    if (wd)
        gd_grid = *wd->Attrib()->grid(wd->Mode());
    gd_mask_bak = 0;
    gd_win_num = wd ? wd->WinNumber() : -1;
    gd_last_n = 0;
    gd_drag_x = 0;
    gd_drag_y = 0;
    gd_dragging = false;

    if (gd_win_num < 0) {
        // Bail out if we don't have a valid window.
        wb_shell = 0;
        return;
    }

    grid_pops[gd_win_num] = this;

    if (owner)
        owner->MonitorAdd(this);

    wb_shell = gtk_NewPopup(0, "Grid Setup", gd_cancel_proc,
        grid_pops + gd_win_num);
    if (!wb_shell)
        return;
    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);

    gtk_signal_connect(GTK_OBJECT(wb_shell), "expose-event",
        GTK_SIGNAL_FUNC(gd_redraw_hdlr), grid_pops + gd_win_num);

    GtkWidget *topform = gtk_table_new(2, 1, false);
    gtk_widget_show(topform);
    gtk_container_add(GTK_CONTAINER(wb_shell), topform);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //

    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("Set grid parameters");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(row), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_btn_proc), grid_pops + gd_win_num);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(topform), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *nbook = gtk_notebook_new();
    gtk_widget_show(nbook);
    gtk_table_attach(GTK_TABLE(topform), nbook, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Snapping Page
    //

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);

    // Grid Snapping controls

    frame = gtk_frame_new("Snap Spacing");
    gtk_widget_show(frame);
    GtkWidget *vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    GtkWidget *sb = sb_resol.init(1.0, 0.0, 10000.0, 4);
    GtkWidget *focus_widget = sb;
    sb_resol.connect_changed(GTK_SIGNAL_FUNC(gd_resol_change_proc),
        grid_pops + gd_win_num, "Resolution");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_start(GTK_BOX(vbox), sb, false, false, 0);

    label = gtk_label_new("");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gd_mfglabel = label;
    gtk_box_pack_start(GTK_BOX(vbox), label, false, false, 0);

    int rcnt = 0;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    frame = gtk_frame_new("Snap");
    gtk_widget_show(frame);
    gd_snapbox = frame;

    vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    sb = sb_snap.init(1, 1.0, 10.0, 0);
    sb_snap.connect_changed(GTK_SIGNAL_FUNC(gd_snap_change_proc),
        grid_pops + gd_win_num, "Snap");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_start(GTK_BOX(vbox), sb, true, false, 0);

    button = gtk_check_button_new_with_label("GridPerSnap");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, false, false, 0);
    gd_snapbtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_snap_proc), grid_pops + gd_win_num);

    gtk_table_attach(GTK_TABLE(form), frame, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rcnt++;

    GtkWidget *hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);
    gtk_table_attach(GTK_TABLE(form), hsep, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rcnt++;

    // Edge Snapping group

    frame = gtk_frame_new("Edge Snapping");
    gtk_widget_show(frame);
    gd_edgegrp = frame;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rcnt++;

    GtkWidget *col = gtk_vbox_new(false, 2);
    gtk_widget_show(col);
    gtk_container_add(GTK_CONTAINER(frame), col);

    GtkWidget *entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "edgemenu");
    gtk_widget_show(entry);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "edgemenu");
    for (int i = 0; edgevals[i]; i++) {
        GtkWidget *mi = gtk_menu_item_new_with_label(edgevals[i]);
        gtk_widget_set_name(mi, edgevals[i]);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(gd_edge_menu_proc), grid_pops + gd_win_num);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    gd_edge = entry;
    gtk_box_pack_start(GTK_BOX(col), entry, true, true, 0);

    button = gtk_check_button_new_with_label(
        "Allow off-grid edge snapping");
    gtk_widget_set_name(button, "offg");
    gtk_widget_show(button);
    gd_off_grid = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_btn_proc), grid_pops + gd_win_num);
    gtk_box_pack_start(GTK_BOX(col), button, true, true, 0);

    button = gtk_check_button_new_with_label(
        "Include non-Manhattan edges");
    gtk_widget_set_name(button, "usenm");
    gtk_widget_show(button);
    gd_use_nm_edge = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_btn_proc), grid_pops + gd_win_num);
    gtk_box_pack_start(GTK_BOX(col), button, true, true, 0);

    button = gtk_check_button_new_with_label(
        "Include wire edges");
    gtk_widget_set_name(button, "wedge");
    gtk_widget_show(button);
    gd_wire_edge = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_btn_proc), grid_pops + gd_win_num);
    gtk_box_pack_start(GTK_BOX(col), button, true, true, 0);

    button = gtk_check_button_new_with_label(
        "Include wire path");
    gtk_widget_set_name(button, "wpath");
    gtk_widget_show(button);
    gd_wire_path = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_btn_proc), grid_pops + gd_win_num);
    gtk_box_pack_start(GTK_BOX(col), button, true, true, 0);

    label = gtk_label_new("Snapping");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), form, label);

    //
    // Style page
    //

    form = gtk_table_new(4, 5, false);
    gtk_widget_show(form);

    // Top buttons

    button = gtk_toggle_button_new_with_label("Show");
    gtk_widget_show(button);
    gtk_widget_set_name(button, "Show");
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_btn_proc), grid_pops + gd_win_num);
    GRX->SetStatus(button, gd_grid.displayed());
    gd_showbtn = button;
    rcnt = 0;
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_toggle_button_new_with_label("On Top");
    gtk_widget_show(button);
    gtk_widget_set_name(button, "OnTop");
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_btn_proc), grid_pops + gd_win_num);
    GRX->SetStatus(button, gd_grid.show_on_top());
    gd_topbtn = button;
    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *menubar = gtk_menu_bar_new();
    gtk_widget_set_name(menubar, "StoreB");
    gtk_widget_show(menubar);

    GtkWidget *item = gtk_menu_item_new_with_label("Store");
    gtk_widget_set_name(item, "Store");
    gtk_widget_show(item);
    gtk_menu_bar_append(GTK_MENU_BAR(menubar), item);
    {
        char buf[64];
        menu = gtk_menu_new();
        gtk_widget_set_name(menu, "StMenu");
        for (int i = 1; i < TECH_NUM_GRIDS; i++) {
            sprintf(buf, "reg%d", i);
            GtkWidget *menu_item = gtk_menu_item_new_with_label(buf);
            gtk_widget_set_name(menu_item, buf);
            gtk_menu_append(GTK_MENU(menu), menu_item);
            gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
                GTK_SIGNAL_FUNC(gd_sto_menu_proc), grid_pops + gd_win_num);
            gtk_widget_show(menu_item);
        }
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
    }

    gtk_table_attach(GTK_TABLE(form), menubar, 2, 3, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    menubar = gtk_menu_bar_new();
    gtk_widget_set_name(menubar, "RecallB");
    gtk_widget_show(menubar);

    item = gtk_menu_item_new_with_label("Recall");
    gtk_widget_set_name(item, "Recall");
    gtk_widget_show(item);
    gtk_menu_bar_append(GTK_MENU_BAR(menubar), item);
    {
        char buf[64];
        menu = gtk_menu_new();
        gtk_widget_set_name(menu, "RcMenu");

        strcpy(buf, REVERT);
        GtkWidget *menu_item = gtk_menu_item_new_with_label(buf);
        gtk_widget_set_name(menu_item, buf);
        gtk_menu_append(GTK_MENU(menu), menu_item);
        gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
            GTK_SIGNAL_FUNC(gd_rcl_menu_proc), grid_pops + gd_win_num);
        gtk_widget_show(menu_item);

        strcpy(buf, LASTAPPL);
        menu_item = gtk_menu_item_new_with_label(buf);
        gtk_widget_set_name(menu_item, buf);
        gtk_menu_append(GTK_MENU(menu), menu_item);
        gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
            GTK_SIGNAL_FUNC(gd_rcl_menu_proc), grid_pops + gd_win_num);
        gtk_widget_show(menu_item);

        for (int i = 1; i < TECH_NUM_GRIDS; i++) {
            sprintf(buf, "reg%d", i);
            menu_item = gtk_menu_item_new_with_label(buf);
            gtk_widget_set_name(menu_item, buf);
            gtk_menu_append(GTK_MENU(menu), menu_item);
            gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
                GTK_SIGNAL_FUNC(gd_rcl_menu_proc), grid_pops + gd_win_num);
            gtk_widget_show(menu_item);
        }
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
    }

    gtk_table_attach(GTK_TABLE(form), menubar, 3, 4, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rcnt++;

    // Axes and Coarse Mult

    vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);
    button = gtk_radio_button_new_with_label(0, "No Axes");
    gtk_widget_set_name(button, "NoAxes");
    gtk_widget_show(button);
    gtk_object_set_data(GTK_OBJECT(button), "axes", (void*)AxesNone);
    GSList *group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    gtk_box_pack_start(GTK_BOX(vbox), button, true, false, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_axes_proc), grid_pops + gd_win_num);
    gd_noaxesbtn = button;

    button = gtk_radio_button_new_with_label(group, "Plain Axes");
    gtk_widget_set_name(button, "PlainAxes");
    gtk_widget_show(button);
    gtk_object_set_data(GTK_OBJECT(button), "axes", (void*)AxesPlain);
    if (gd_grid.axes() == AxesPlain) {
        GTK_TOGGLE_BUTTON(button)->active = true;
        GTK_TOGGLE_BUTTON(gd_noaxesbtn)->active = false;
    }
    gtk_box_pack_start(GTK_BOX(vbox), button, true, false, 0);
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_axes_proc), grid_pops + gd_win_num);
    gd_plaxesbtn = button;

    button = gtk_radio_button_new_with_label(group, "Mark Origin");
    gtk_widget_set_name(button, "MarkOrigin");
    gtk_widget_show(button);
    gtk_object_set_data(GTK_OBJECT(button), "axes", (void*)AxesMark);
    if (gd_grid.axes() == AxesMark) {
        GTK_TOGGLE_BUTTON(button)->active = true;
        GTK_TOGGLE_BUTTON(gd_noaxesbtn)->active = false;
    }
    gtk_box_pack_start(GTK_BOX(vbox), button, true, false, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_axes_proc), grid_pops + gd_win_num);
    gd_oraxesbtn = button;

    gtk_table_attach(GTK_TABLE(form), vbox, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    frame = gtk_frame_new("Coarse Mult");
    gtk_widget_show(frame);

    sb = sb_cmult.init(1, 1.0, 50.0, 0);
    sb_cmult.connect_changed(GTK_SIGNAL_FUNC(gd_cmult_change_proc),
        grid_pops + gd_win_num, "CoarseMult");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_container_add(GTK_CONTAINER(frame), sb);

    gtk_table_attach(GTK_TABLE(form), frame, 2, 4, rcnt, rcnt+1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rcnt++;

    // Line Style Editor

    frame = gtk_frame_new("Line Style Editor");
    gtk_widget_show(frame);

    GtkWidget *eform = gtk_table_new(3, 4, false);
    gtk_widget_show(eform);
    gtk_container_add(GTK_CONTAINER(frame), eform);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 4, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(0), 2, 2);
    rcnt++;

    button = gtk_radio_button_new_with_label(0, "Solid");
    gtk_widget_set_name(button, "Solid");
    gtk_widget_show(button);
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    gtk_object_set_data(GTK_OBJECT(button), "lst", (void*)LstSolid);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_lst_proc), grid_pops + gd_win_num);
    gtk_table_attach(GTK_TABLE(eform), button, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gd_solidbtn = button;

    button = gtk_radio_button_new_with_label(group, "Dots");
    gtk_widget_set_name(button, "Dots");
    gtk_widget_show(button);
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    gtk_object_set_data(GTK_OBJECT(button), "lst", (void*)LstDots);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_lst_proc), grid_pops + gd_win_num);
    gtk_table_attach(GTK_TABLE(eform), button, 1, 2, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gd_dotsbtn = button;
    if (gd_grid.linestyle().mask == 0) {
        GTK_TOGGLE_BUTTON(button)->active = true;
        GTK_TOGGLE_BUTTON(gd_solidbtn)->active = false;
    }

    button = gtk_radio_button_new_with_label(group, "Textured");
    gtk_widget_set_name(button, "Textured");
    gtk_widget_show(button);
    gtk_object_set_data(GTK_OBJECT(button), "lst", (void*)LstStip);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_lst_proc), grid_pops + gd_win_num);
    gtk_table_attach(GTK_TABLE(eform), button, 2, 3, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gd_stipbtn = button;
    if (gd_grid.linestyle().mask != 0 && gd_grid.linestyle().mask != -1) {
        GTK_TOGGLE_BUTTON(button)->active = true;
        GTK_TOGGLE_BUTTON(gd_solidbtn)->active = false;
    }

    GtkWidget *bframe = gtk_frame_new("Cross Size");
    // start out hidden
    gd_crs_frame = bframe;

    sb = sb_crs.init(0, 0.0, 6.0, 0);
    sb_crs.connect_changed(GTK_SIGNAL_FUNC(gd_crs_change_proc),
        grid_pops + gd_win_num, "CrossSize");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_container_add(GTK_CONTAINER(bframe), sb);
    gtk_table_attach(GTK_TABLE(eform), bframe, 1, 2, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *darea = gtk_drawing_area_new();
    gtk_widget_show(darea);
    gtk_drawing_area_size(GTK_DRAWING_AREA(darea), 200, 10);
    gtk_table_attach(GTK_TABLE(eform), darea, 0, 3, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gd_sample = darea;

    // The sample is a drag source of a piece of plain text which is
    // the line style mask in "0xhhhh" format.  This might be useful
    // for exporting the line style.

    gtk_widget_add_events(darea, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(darea, GDK_BUTTON_RELEASE_MASK);
    gtk_widget_add_events(darea, GDK_POINTER_MOTION_MASK);
    gtk_signal_connect(GTK_OBJECT(darea), "button-press-event",
        GTK_SIGNAL_FUNC(gd_button_press_hdlr), grid_pops + gd_win_num);
    gtk_signal_connect(GTK_OBJECT(darea), "button-release-event",
        GTK_SIGNAL_FUNC(gd_button_release_hdlr), grid_pops + gd_win_num);
    gtk_signal_connect(GTK_OBJECT(darea), "motion-notify-event",
        GTK_SIGNAL_FUNC(gd_motion_hdlr), grid_pops + gd_win_num);
    gtk_signal_connect(GTK_OBJECT(darea), "drag-data-get",
        GTK_SIGNAL_FUNC(gd_drag_data_get), grid_pops + gd_win_num);

    darea = gtk_drawing_area_new();
    gtk_widget_show(darea);
    gtk_drawing_area_size(GTK_DRAWING_AREA(darea), 200, 10);
    gtk_widget_add_events(darea, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(darea, GDK_BUTTON_RELEASE_MASK);
    gtk_signal_connect(GTK_OBJECT(darea), "button-press-event",
        GTK_SIGNAL_FUNC(gd_button_press_hdlr), grid_pops + gd_win_num);
    gd_viewport = darea;
    gtk_signal_connect(GTK_OBJECT(darea), "button-release-event",
        GTK_SIGNAL_FUNC(gd_button_release_hdlr), grid_pops + gd_win_num);
    gtk_table_attach(GTK_TABLE(eform), darea, 0, 3, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    // Visibility controls (global)

    frame = gtk_frame_new("All Windows");
    if (gd_win_num == 0)
        gtk_widget_show(frame);
    else
        gtk_widget_hide(frame);

    GtkWidget *gform = gtk_table_new(2, 1, false);
    gtk_widget_show(gform);
    gtk_container_add(GTK_CONTAINER(frame), gform);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 4, rcnt, rcnt+1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rcnt++;

    button = gtk_check_button_new_with_label("No coarse when fine invisible");
    gtk_widget_set_name(button, "cvis");
    gtk_widget_show(button);
    gd_nocoarse = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_btn_proc), grid_pops + gd_win_num);

    gtk_table_attach(GTK_TABLE(gform), button, 0, 2, 0, 1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    label = gtk_label_new("Visibility Threshold");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(gform), label, 0, 1, 1, 2,
        (GtkAttachOptions)0,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    sb = sb_thresh.init(DSP_DEF_GRID_THRESHOLD, DSP_MIN_GRID_THRESHOLD,
        DSP_MAX_GRID_THRESHOLD, 0);
    sb_thresh.connect_changed(GTK_SIGNAL_FUNC(gd_thresh_change_proc),
        grid_pops + gd_win_num, "GridThreshold");
    gtk_widget_set_size_request(sb, 50, -1);
    gtk_table_attach(GTK_TABLE(gform), sb, 1, 2, 1, 2,
        (GtkAttachOptions)0,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    label = gtk_label_new("Style");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), form, label);

    //
    // Apply and Dismiss buttons
    //

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);

    button = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(button, "Apply Grid");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_apply_proc), grid_pops + gd_win_num);
    gd_apply = button;
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gd_cancel_proc), grid_pops + gd_win_num);
    gd_cancel = button;
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(topform), row, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    // Initially focus to the Snap Spacing entry.  Set up a handler
    // so that Enter will call apply, then a second Enter will call
    // cancel.
    //
    gtk_window_set_focus(GTK_WINDOW(wb_shell), focus_widget);
    gtk_signal_connect(GTK_OBJECT(wb_shell), "key-release-event",
        GTK_SIGNAL_FUNC(gd_key_hdlr), grid_pops + gd_win_num);

    update();
}


sGrd::~sGrd()
{
    if (gd_win_num >= 0)
        grid_pops[gd_win_num] = 0;

    if (p_parent) {
        win_bag *owner = dynamic_cast<win_bag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        GRX->Deselect(p_caller);
    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(gd_cancel_proc), grid_pops + gd_win_num);
}


// GRpopup override
//
void
sGrd::popdown()
{
    if (!p_parent)
        return;
    gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
    if (!owner || !owner->MonitorActive(this))
        return;

    delete this;
}


void
sGrd::update(bool skip_init)
{
    WindowDesc *wd = DSP()->Window(gd_win_num);
    if (!wd)
        return;
    if (!skip_init)
        gd_grid = *wd->Attrib()->grid(wd->Mode());

    // Snapping page

    double del = GridDesc::mfg_grid(wd->Mode());
    if (del <= 0.0)
        del = wd->Mode() == Physical ? MICRONS(1) : ELEC_MICRONS(100);

    sb_resol.set_min(del);
    sb_resol.set_delta(del);
    sb_resol.set_value(gd_grid.spacing(wd->Mode()));
    sb_snap.set_value(abs(gd_grid.snap()));

    char buf[64];
    del = GridDesc::mfg_grid(wd->Mode());
    if (del > 0.0)
        sprintf(buf, "MfgGrid: %.4f", del);
    else
        strcpy(buf, "MfgGrid: unset");
    gtk_label_set_text(GTK_LABEL(gd_mfglabel), buf);
    if (gd_grid.snap() < 0) {
        GRX->SetStatus(gd_snapbtn, true);
        gtk_frame_set_label(GTK_FRAME(gd_snapbox), "GridPerSnap");
    }
    else {
        GRX->SetStatus(gd_snapbtn, false);
        gtk_frame_set_label(GTK_FRAME(gd_snapbox), "SnapPerGrid");
    }

    gtk_option_menu_set_history(GTK_OPTION_MENU(gd_edge),
        wd->Attrib()->edge_snapping());
    GRX->SetStatus(gd_off_grid, wd->Attrib()->edge_off_grid());
    GRX->SetStatus(gd_use_nm_edge, wd->Attrib()->edge_non_manh());
    GRX->SetStatus(gd_wire_edge, wd->Attrib()->edge_wire_edge());
    GRX->SetStatus(gd_wire_path, wd->Attrib()->edge_wire_path());

    // Style page

    GRX->SetStatus(gd_showbtn, gd_grid.displayed());
    GRX->SetStatus(gd_topbtn, gd_grid.show_on_top());
    if (gd_grid.axes() == AxesNone)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gd_noaxesbtn), true);
    else if (gd_grid.axes() == AxesPlain)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gd_plaxesbtn), true);
    else if (gd_grid.axes() == AxesMark)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gd_oraxesbtn), true);
    if (wd->Mode() == Electrical) {
        gtk_widget_set_sensitive(gd_noaxesbtn, false);
        gtk_widget_set_sensitive(gd_plaxesbtn, false);
        gtk_widget_set_sensitive(gd_oraxesbtn, false);
        gtk_widget_set_sensitive(gd_nocoarse, false);
        gtk_widget_set_sensitive(gd_edgegrp, false);
    }
    else {
        gtk_widget_set_sensitive(gd_noaxesbtn, true);
        gtk_widget_set_sensitive(gd_plaxesbtn, true);
        gtk_widget_set_sensitive(gd_oraxesbtn, true);
        gtk_widget_set_sensitive(gd_nocoarse, true);
        gtk_widget_set_sensitive(gd_edgegrp, true);
    }

    sb_cmult.set_value(gd_grid.coarse_mult());

    GRX->SetStatus(gd_nocoarse, DSP()->GridNoCoarseOnly());
    sb_thresh.set_value(DSP()->GridThreshold());

    gd_mask_bak = 0;
    if (gd_grid.linestyle().mask == -1)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gd_solidbtn), true);
    else if (gd_grid.linestyle().mask == 0) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gd_dotsbtn), true);
        sb_crs.set_value(gd_grid.dotsize());
    }
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gd_stipbtn), true);

    if (GTK_WIDGET_MAPPED(wb_shell))
        gd_redraw_hdlr(0, 0, grid_pops + gd_win_num);
}


void
sGrd::initialize()
{
    win_bag *w = dynamic_cast<win_bag*>(p_parent);
    if (w) {
        gtk_window_set_transient_for(GTK_WINDOW(wb_shell),
            GTK_WINDOW(w->Shell()));
        GRX->SetPopupLocation(GRloc(), wb_shell, w->Viewport());
    }
}


// Static function.
void
sGrd::gd_cancel_proc(GtkWidget*, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (grd)
        grd->popdown();
}


// Static function.
int
sGrd::gd_key_hdlr(GtkWidget*, GdkEvent *event, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (!grd || !grd->gd_apply)
        return (0);

    // Look only at Enter key events.  If not focussed on the Dismiss
    // button, run the apply callback and set focus to the Dismiss
    // button.  A second Enter press will then dismiss the pop-up.

    if (event->key.keyval == GDK_Return) {
        GtkWidget *w = gtk_window_get_focus(GTK_WINDOW(grd->wb_shell));
        if (w != grd->gd_cancel) {
            grd->gd_apply_proc(0, arg);
            gtk_window_set_focus(GTK_WINDOW(grd->wb_shell), grd->gd_cancel);
            return (1);
        }
    }
    return (0);
}


// Static function.
void
sGrd::gd_apply_proc(GtkWidget*, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (!grd)
        return;
    WindowDesc *wd = DSP()->Window(grd->gd_win_num);
    if (!wd)
        return;

    DisplayMode mode = wd->Mode();
    if (mode == Electrical) {
        double spa = grd->gd_grid.spacing(mode);
        if (10*ELEC_INTERNAL_UNITS(spa) % CDelecResolution) {
            // "can't happen"
            Log()->ErrorLog(mh::Initialization,
                "Error: electrical mode snap points must be "
                "0.1 micron multiples.");
            return;
        }
        if (ELEC_INTERNAL_UNITS(spa) % CDelecResolution) {
            Log()->WarningLog(mh::Initialization,
                "Sub-micron snap points are NOT RECOMMENDED in\n"
                "electrical mode.  Connection points should be on\n"
                "a one micron grid.  The present grid is accepted\n"
                "to allow repair of old files.");
        }
    }
    DSPattrib *a = wd->Attrib();
    GridDesc lastgrid = *a->grid(mode);
    Tech()->SetGridReg(0, lastgrid, mode);
    a->grid(mode)->set(grd->gd_grid);
    bool axes_changed =
        (mode == Physical && a->grid(mode)->axes() != lastgrid.axes());
    if (lastgrid.visually_differ(a->grid(mode)))
        wd->Redisplay(0);
    else if (axes_changed) {
        if (lastgrid.axes() != AxesNone)
            wd->ShowAxes(ERASE);
        wd->ShowAxes(DISPLAY);
    }
    XM()->ShowParameters();
}


// Static function.
void
sGrd::gd_snap_proc(GtkWidget *widget, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (!grd)
        return;
    if (GRX->GetStatus(widget)) {
        gtk_frame_set_label(GTK_FRAME(grd->gd_snapbox), "GridPerSnap");
        int sn = grd->sb_snap.get_value_as_int();
        grd->gd_grid.set_snap(-sn);
    }
    else {
        gtk_frame_set_label(GTK_FRAME(grd->gd_snapbox), "SnapPerGrid");
        int sn = grd->sb_snap.get_value_as_int();
        grd->gd_grid.set_snap(sn);
    }
}


// Static function.
void
sGrd::gd_resol_change_proc(GtkWidget*, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (!grd)
        return;
    WindowDesc *wd = DSP()->Window(grd->gd_win_num);
    if (!wd)
        return;

    grd->gd_grid.set_spacing(grd->sb_resol.get_value());

    // Reset the value, as it might have been reset to the mfg grid.
    grd->sb_resol.set_value(grd->gd_grid.spacing(wd->Mode()));
}


// Static function.
void
sGrd::gd_snap_change_proc(GtkWidget*, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (grd) {
        bool neg = GRX->GetStatus(grd->gd_snapbtn);
        int sn = grd->sb_snap.get_value_as_int();
        grd->gd_grid.set_snap(neg ? -sn : sn);
    }
}


// Static function.
void
sGrd::gd_edge_menu_proc(GtkWidget*, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (!grd)
        return;
    WindowDesc *wd = DSP()->Window(grd->gd_win_num);
    if (!wd)
        return;

    switch (gtk_option_menu_get_history(GTK_OPTION_MENU(grd->gd_edge))) {
    case EdgeSnapNone:
        wd->Attrib()->set_edge_snapping(EdgeSnapNone);
        break;
    case EdgeSnapSome:
        wd->Attrib()->set_edge_snapping(EdgeSnapSome);
        break;
    case EdgeSnapAll:
        wd->Attrib()->set_edge_snapping(EdgeSnapAll);
        break;
    }
}


// Static function.
void
sGrd::gd_btn_proc(GtkWidget *widget, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (!grd)
        return;
    WindowDesc *wd = DSP()->Window(grd->gd_win_num);
    if (!wd)
        return;

    const char *name = gtk_widget_get_name(widget);
    if (name && !strcmp(name, "help")) {
        DSPmainWbag(PopUpHelp("xic:grid"));
        return;
    }

    bool state = GRX->GetStatus(widget);
    if (widget == grd->gd_off_grid)
        wd->Attrib()->set_edge_off_grid(state);
    else if (widget == grd->gd_use_nm_edge)
        wd->Attrib()->set_edge_non_manh(state);
    else if (widget == grd->gd_wire_edge)
        wd->Attrib()->set_edge_wire_edge(state);
    else if (widget == grd->gd_wire_path)
        wd->Attrib()->set_edge_wire_path(state);
    else if (widget == grd->gd_showbtn)
        grd->gd_grid.set_displayed(GRX->GetStatus(widget));
    else if (widget == grd->gd_topbtn)
        grd->gd_grid.set_show_on_top(GRX->GetStatus(widget));
    else if (widget == grd->gd_nocoarse) {
        if (GRX->GetStatus(widget))
            CDvdb()->setVariable(VA_GridNoCoarseOnly, "");
        else
            CDvdb()->clearVariable(VA_GridNoCoarseOnly);
    }
}


// Static function.
void
sGrd::gd_sto_menu_proc(GtkWidget *widget, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (!grd)
        return;
    WindowDesc *wd = DSP()->Window(grd->gd_win_num);
    if (!wd)
        return;

    DisplayMode mode = wd->Mode();
    const char *string = gtk_widget_get_name(widget);
    if (string) {
        while (isalpha(*string))
            string++;
        int indx = atoi(string);
        if (indx >= 0 && indx < TECH_NUM_GRIDS)
            Tech()->SetGridReg(indx, grd->gd_grid, mode);
    }
}


// Static function.
void
sGrd::gd_rcl_menu_proc(GtkWidget *widget, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (!grd)
        return;
    WindowDesc *wd = DSP()->Window(grd->gd_win_num);
    if (!wd)
        return;

    const char *string = gtk_widget_get_name(widget);
    if (string) {
        if (!strcmp(string, REVERT)) {
            // Revert to the current Attributes grid.
            grd->update();
            return;
        }
        if (!strcmp(string, LASTAPPL)) {
            // Return to the last grid applied (reg 0);
            GridDesc gref;
            DisplayMode mode = wd->Mode();
            GridDesc *g = Tech()->GridReg(0, mode);
            if (*g != gref) {
                grd->gd_grid.set(*g);
                grd->update(true);
            }
            return;
        }
        while (isalpha(*string))
            string++;
        int indx = atoi(string);
        if (indx >= 0 && indx < TECH_NUM_GRIDS) {
            GridDesc gref;
            DisplayMode mode = wd->Mode();
            GridDesc *g = Tech()->GridReg(indx, mode);
            if (*g != gref) {
                grd->gd_grid.set(*g);
                grd->update(true);
            }
        }
    }
}


// Static function.
void
sGrd::gd_axes_proc(GtkWidget *widget, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (!grd)
        return;
    WindowDesc *wd = DSP()->Window(grd->gd_win_num);
    if (!wd)
        return;
    if (GRX->GetStatus(widget) && wd->Mode() == Physical)
        grd->gd_grid.set_axes(
            (AxesType)(long)gtk_object_get_data(GTK_OBJECT(widget), "axes"));
}


// Static function.
void
sGrd::gd_lst_proc(GtkWidget *widget, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (grd && GRX->GetStatus(widget)) {
        int lst = (long)gtk_object_get_data(GTK_OBJECT(widget), "lst");
        if (lst == LstSolid) {
            if (!grd->gd_mask_bak && grd->gd_grid.linestyle().mask != 0 &&
                    grd->gd_grid.linestyle().mask != -1)
                grd->gd_mask_bak = grd->gd_grid.linestyle().mask;
            grd->gd_grid.linestyle().mask = -1;
        }
        else if (lst == LstDots) {
            if (!grd->gd_mask_bak && grd->gd_grid.linestyle().mask != 0 &&
                    grd->gd_grid.linestyle().mask != -1)
                grd->gd_mask_bak = grd->gd_grid.linestyle().mask;
            grd->gd_grid.linestyle().mask = 0;
        }
        else {
            if (grd->gd_mask_bak) {
                grd->gd_grid.linestyle().mask = grd->gd_mask_bak;
                grd->gd_mask_bak = 0;
            }
        }
        gd_redraw_hdlr(0, 0, grid_pops + grd->gd_win_num);
    }
}


// Static function.
void
sGrd::gd_cmult_change_proc(GtkWidget*, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (grd) {
        int cm = grd->sb_cmult.get_value_as_int();
        grd->gd_grid.set_coarse_mult(cm);
    }
}


// Static function.
void
sGrd::gd_thresh_change_proc(GtkWidget*, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (grd) {
        char buf[32];
        int n = grd->sb_thresh.get_value_as_int();
        if (n < DSP_MIN_GRID_THRESHOLD || n > DSP_MAX_GRID_THRESHOLD)
            return;
        sprintf(buf, "%d", n);
        if (n != DSP_DEF_GRID_THRESHOLD)
            CDvdb()->setVariable(VA_GridThreshold, buf);
        else
            CDvdb()->clearVariable(VA_GridThreshold);
    }
}


// Static function.
void
sGrd::gd_crs_change_proc(GtkWidget*, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (grd)
        grd->gd_grid.set_dotsize(grd->sb_crs.get_value_as_int());
}


// Static function.
int
sGrd::gd_redraw_hdlr(GtkWidget*, GdkEvent*, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (!grd)
        return (false);

    if (GTK_TOGGLE_BUTTON(grd->gd_stipbtn)->active) {
        gtk_widget_hide(grd->gd_crs_frame);
        gtk_widget_show(grd->gd_sample);
        gtk_widget_show(grd->gd_viewport);

        if (GTK_WIDGET_MAPPED(grd->gd_viewport)) {
            grd->gd_window = grd->gd_viewport->window;
            grd->SetWindowBackground(GRX->NameColor("white"));
            grd->Clear();
            grd->SetFillpattern(0);
            grd->SetColor(GRX->NameColor("blue"));
            int wid, hei;
            gdk_window_get_size(grd->gd_window, &wid, &hei);
            int w = wid/32 - 1;
            if (w < 2)
                w = 2;
            int tw = 32*w;
            int os = 0;
            if (tw < wid)
                os = (wid - tw)/2;
            unsigned mask = ~((~(unsigned)0) >> 1);
            int x = os;
            int xs = -1;
            for (int i = 0; i < 32; i++) {
                if (mask & grd->gd_grid.linestyle().mask) {
                    grd->Box(x, 1, x+w, hei-2);
                    if (xs < 0)
                        xs = x;
                }
                x += w;
                mask >>= 1;
            }
            if (xs < 0)
                xs = tw + os;
            GtkStyle *style = gtk_widget_get_style(grd->gd_viewport);
            if (os) {
                grd->SetColor(style->bg[GTK_STATE_NORMAL].pixel);
                grd->Box(0, 0, os, hei);
                grd->Box(os + tw, 0, wid, hei);
            }
            grd->SetColor(style->bg[GTK_STATE_ACTIVE].pixel);
            grd->Box(os, 0, xs, hei);

            grd->gd_window = grd->gd_sample->window;
            grd->SetWindowBackground(GRX->NameColor("black"));
            grd->Clear();
            if (grd->gd_grid.linestyle().mask) {
                // DefineLinestyle will convert 1,3,7,... to -1.
                unsigned ltmp = grd->gd_grid.linestyle().mask;
                grd->defineLinestyle(&grd->gd_grid.linestyle(), ltmp);
                grd->gd_grid.linestyle().mask = ltmp;
                grd->SetColor(GRX->NameColor("white"));
                grd->SetFillpattern(0);
                grd->Line(os+2, 5, os+tw-2, 5);
            }
            if (os) {
                grd->SetColor(style->bg[GTK_STATE_NORMAL].pixel);
                grd->Box(0, 0, os, hei);
                grd->Box(os + tw, 0, wid, hei);
            }
        }
    }
    else {
        if (GTK_TOGGLE_BUTTON(grd->gd_dotsbtn)->active)
            gtk_widget_show(grd->gd_crs_frame);
        else
            gtk_widget_hide(grd->gd_crs_frame);

        if (GTK_WIDGET_MAPPED(grd->gd_viewport)) {
            grd->gd_window = grd->gd_viewport->window;
            GtkStyle *style = gtk_widget_get_style(grd->gd_viewport);
            grd->SetWindowBackground(style->bg[GTK_STATE_NORMAL].pixel);
            grd->Clear();
            grd->gd_window = grd->gd_sample->window;
            grd->SetWindowBackground(style->bg[GTK_STATE_NORMAL].pixel);
            grd->Clear();
        }

        if (GTK_TOGGLE_BUTTON(grd->gd_solidbtn)->active) {
            gtk_widget_show(grd->gd_sample);
            gtk_widget_show(grd->gd_viewport);
        }
        else {
            gtk_widget_hide(grd->gd_sample);
            gtk_widget_hide(grd->gd_viewport);
        }
    }
    return (false);
}


// Static function.
int
sGrd::gd_button_press_hdlr(GtkWidget *widget, GdkEvent *event, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (!grd)
        return (false);

    if (widget == grd->gd_sample) {
        grd->gd_dragging = true;
        grd->gd_drag_x = (int)event->button.x;
        grd->gd_drag_y = (int)event->button.y;
        return (true);
    }
    if (GTK_TOGGLE_BUTTON(grd->gd_stipbtn)->active) {
        int x = (int)event->button.x;
        int wid, hei;
        gdk_window_get_size(grd->gd_window, &wid, &hei);
        int w = wid/32 - 1;
        if (w < 2)
            w = 2;
        int tw = 32*w;
        int os = 0;
        if (tw < wid)
            os = (wid - tw)/2;
        int n = (x - os)/w;
        if (n < 0 || n > 31)
            return (true);
        n = 31 - n;
        if (event->button.button == 1)
            grd->gd_grid.linestyle().mask ^= (1 << n);
        else if (event->button.button == 2)
            grd->gd_grid.linestyle().mask &= ~(1 << n);
        else if (event->button.button == 3)
            grd->gd_grid.linestyle().mask |= (1 << n);
        else
            return (false);
        grd->gd_last_n = n;
        gd_redraw_hdlr(0, 0, grid_pops + grd->gd_win_num);
    }
    return (true);
}


// Static function.
int
sGrd::gd_button_release_hdlr(GtkWidget *widget, GdkEvent *event, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (!grd)
        return (false);

    if (widget == grd->gd_sample) {
        grd->gd_dragging = false;
        grd->gd_drag_x = 0;
        grd->gd_drag_y = 0;
        return (true);
    }
    if (GTK_TOGGLE_BUTTON(grd->gd_stipbtn)->active) {

        int x = (int)event->button.x;
        int wid, hei;
        gdk_window_get_size(grd->gd_window, &wid, &hei);
        int w = wid/32 - 1;
        if (w < 2)
            w = 2;
        int tw = 32*w;
        int os = 0;
        if (tw < wid)
            os = (wid - tw)/2;
        int n = (x - os)/w;
        if (n < 0 || n > 31)
            return (true);
        n = 31 - n;
        if (n > grd->gd_last_n) {
            for (int i = grd->gd_last_n + 1; i <= n; i++) {
                if (event->button.button == 1)
                    grd->gd_grid.linestyle().mask ^= (1 << i);
                else if (event->button.button == 2)
                    grd->gd_grid.linestyle().mask &= ~(1 << i);
                else if (event->button.button == 3)
                    grd->gd_grid.linestyle().mask |= (1 << i);
                else
                    return (true);
            }
            gd_redraw_hdlr(0, 0, grid_pops + grd->gd_win_num);
        }
        else if (n < grd->gd_last_n) {
            for (int i = grd->gd_last_n - 1; i >= n; i--) {
                if (event->button.button == 1)
                    grd->gd_grid.linestyle().mask ^= (1 << i);
                else if (event->button.button == 2)
                    grd->gd_grid.linestyle().mask &= ~(1 << i);
                else if (event->button.button == 3)
                    grd->gd_grid.linestyle().mask |= (1 << i);
                else
                    return (true);
            }
            gd_redraw_hdlr(0, 0, grid_pops + grd->gd_win_num);
        }
    }
    return (true);
}


namespace {
    GtkTargetEntry gd_targets[] = {
        { (char*)"text/plain", 0, 4 },
    };
}


// Static function.
int
sGrd::gd_motion_hdlr(GtkWidget *caller, GdkEvent *event, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (!grd)
        return (false);

    int x = (int)event->motion.x;
    int y = (int)event->motion.y;
    if (grd->gd_dragging &&
            (abs(x - grd->gd_drag_x) > 2 || abs(y - grd->gd_drag_y) > 2)) {
        grd->gd_dragging = false;
        GtkTargetList *targets = gtk_target_list_new(gd_targets, 1);
        gtk_drag_begin(caller, targets, (GdkDragAction)GDK_ACTION_COPY,
            1, event);
    }
    return (true);
}


// Static function.
// Initialize data for drag/drop transfer from 'this'.
//
void
sGrd::gd_drag_data_get(GtkWidget*, GdkDragContext*,
    GtkSelectionData *selection_data, guint, guint, void *arg)
{
    sGrd *grd = *(sGrd**)arg;
    if (!grd)
        return;

    char buf[64];
    sprintf(buf, "0x%x", grd->gd_grid.linestyle().mask);
    gtk_selection_data_set(selection_data, selection_data->target,
        8, (unsigned char*)buf, strlen(buf)+1);
}

