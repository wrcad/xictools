
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
#include "editif.h"
#include "cfilter.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_chd.h"
#include "events.h"
#include "menu.h"
#include "cell_menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkpfiles.h"
#include "gtkinterf/gtkfont.h"
#include "gtkinterf/gtkutil.h"
#include "miscutil/filestat.h"


//----------------------------------------------------------------------
//  Cells Listing Panel
//
// Help system keywords used:
//  cellspanel

namespace {
    GtkTargetEntry target_table[] = {
        { (char*)"CELLNAME",    2, 0 },
        { (char*)"STRING",      0, 1 },
        { (char*)"text/plain",  0, 2 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

    namespace gtkcells {
        struct sCells : public gtk_bag
        {
            sCells(GRobject);
            ~sCells();

            void update();

            char *get_selection() { return (text_get_selection(wb_textarea)); }

            void end_search() { GRX->Deselect(c_searchbtn); }

        private:
            void clear(const char*);
            void copy_cell(const char*);
            void replace_instances(const char*);
            void rename_cell(const char*);
            int button_hdlr(bool, GtkWidget*, GdkEvent*);
            void select_range(int, int);
            void action_hdlr(GtkWidget*, void*);
            int motion_hdlr(GtkWidget*, GdkEvent*);
            void resize_hdlr(GtkAllocation*);
            stringlist *raw_cell_list(int*, int*, bool);
            char *cell_list(int);
            void update_text(char*);
            void check_sens();

            static void c_clear_cb(bool, void*);
            static ESret c_copy_cb(const char*, void*);
            static void c_repl_cb(bool, void*);
            static ESret c_rename_cb(const char*, void*);

            static void c_page_proc(GtkWidget*, void*);
            static void c_mode_proc(GtkWidget*, void*);
            static void c_filter_cb(cfilter_t*, void*);
            static int c_highlight_idle(void*);
            static void c_drag_data_get(GtkWidget *widget, GdkDragContext*,
                GtkSelectionData*, guint, guint, void*);
            static int c_btn_hdlr(GtkWidget*, GdkEvent*, void*);
            static int c_btn_release_hdlr(GtkWidget*, GdkEvent*, void*);
            static int c_motion_hdlr(GtkWidget*, GdkEvent*, void*);
            static void c_resize_hdlr(GtkWidget*, GtkAllocation*);
            static void c_realize_hdlr(GtkWidget*, void*);
            static void c_action_proc(GtkWidget*, void*);
            static void c_help_proc(GtkWidget*, void*);
            static void c_cancel(GtkWidget*, void*);
            static void c_save_btn_hdlr(GtkWidget*, void*);
            static ESret c_save_cb(const char*, void*);
            static int c_timeout(void*);

            GRobject c_caller;
            GRaffirmPopup *c_clear_pop;
            GRaffirmPopup *c_repl_pop;
            GRledPopup *c_copy_pop;
            GRledPopup *c_rename_pop;
            GTKledPopup *c_save_pop;
            GTKmsgPopup *c_msg_pop;
            GtkWidget *c_label;
            GtkWidget *c_clearbtn;
            GtkWidget *c_treebtn;
            GtkWidget *c_openbtn;
            GtkWidget *c_placebtn;
            GtkWidget *c_copybtn;
            GtkWidget *c_replbtn;
            GtkWidget *c_renamebtn;
            GtkWidget *c_searchbtn;
            GtkWidget *c_flagbtn;
            GtkWidget *c_infobtn;
            GtkWidget *c_showbtn;
            GtkWidget *c_fltrbtn;
            GtkWidget *c_page_combo;
            GtkWidget *c_mode_combo;
            cfilter_t *c_pfilter;
            cfilter_t *c_efilter;
            char *c_copyname;
            char *c_replname;
            char *c_newname;
            bool c_no_select;
            bool c_no_update;
            bool c_dragging;
            int c_drag_x, c_drag_y;
            int c_cols;
            int c_page;
            DisplayMode c_mode;
            int c_start;
            int c_end;
        };

        sCells *Cells;
    }
}

using namespace gtkcells;


// Static function.
//
char *
main_bag::get_cell_selection()
{
    if (Cells)
        return (Cells->get_selection());
    return (0);
}


// Static function.
// Called on crash to prevent updates.
//
void
main_bag::cells_panic()
{
    Cells = 0;
}


void
cMain::PopUpCells(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Cells;
        return;
    }
    if (mode == MODE_UPD) {
        if (Cells)
            Cells->update();
        return;
    }
    if (Cells)
        return;

    new sCells(caller);
    if (!Cells->Shell()) {
        delete Cells;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Cells->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), Cells->Shell(), mainBag()->Viewport());
    gtk_widget_show(Cells->Shell());
}


namespace {
    namespace gtkcells {
        // Command state for the Search function.
        //
        struct ListState : public CmdState
        {
            ListState(const char *nm, const char *hk) : CmdState(nm, hk)
                {
                    lsAOI = CDinfiniteBB;
                }
            virtual ~ListState();

            static void show_search(bool);
            char *label_text();
            stringlist *cell_list(bool);

            void b1down() { cEventHdlr::sel_b1down(); }
            void b1up();
            void esc();
            void undo() { cEventHdlr::sel_undo(); }
            void redo() { cEventHdlr::sel_redo(); }
            void message() { PL()->ShowPrompt(info_msg); }

        private:
            BBox lsAOI;

            static const char *info_msg;
        };

        ListState *ListCmd;

        enum { NilCode, ClearCode, TreeCode, OpenCode, PlaceCode, CopyCode,
            ReplCode, RenameCode, SearchCode, FlagCode, InfoCode, ShowCode,
            FltrCode };
    }
}

const char *ListState::info_msg =
    "Use pointer to define area for subcell list.";


sCells::sCells(GRobject c)
{
    Cells = this;
    c_caller = c;
    c_clear_pop = 0;
    c_repl_pop = 0;
    c_copy_pop = 0;
    c_rename_pop = 0;
    c_save_pop = 0;
    c_msg_pop = 0;
    c_label = 0;
    c_clearbtn = 0;
    c_treebtn = 0;
    c_openbtn = 0;
    c_placebtn = 0;
    c_copybtn = 0;
    c_replbtn = 0;
    c_renamebtn = 0;
    c_searchbtn = 0;
    c_flagbtn = 0;
    c_infobtn = 0;
    c_showbtn = 0;
    c_fltrbtn = 0;
    c_page_combo = 0;
    c_mode_combo = 0;
    c_pfilter = cfilter_t::parse(0, Physical, 0);
    c_efilter = cfilter_t::parse(0, Electrical, 0);
    c_copyname = 0;
    c_replname = 0;
    c_newname = 0;
    c_no_select = false;
    c_no_update = false;
    c_dragging = false;
    c_drag_x = c_drag_y = 0;
    c_cols = 0;
    c_page = 0;
    c_mode = DSP()->CurMode();
    c_start = 0;
    c_end = 0;

    wb_shell = gtk_NewPopup(0, "Cells Listing", c_cancel, 0);
    if (!wb_shell)
        return;

    GtkWidget *form = gtk_table_new(1, 5, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    GtkWidget *vbox = gtk_vbox_new(false, 2);
    gtk_widget_show(vbox);

    //
    // button row
    //
    GtkWidget *button = gtk_button_new_with_label("Clear");
    gtk_widget_set_name(button, "Clear");
    gtk_widget_show(button);
    c_clearbtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_action_proc), (void*)ClearCode);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);
    if (!EditIf()->hasEdit())
        gtk_widget_hide(button);

    button = gtk_button_new_with_label("Tree");
    gtk_widget_set_name(button, "Tree");
    gtk_widget_show(button);
    c_treebtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_action_proc), (void*)TreeCode);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);

    button = gtk_button_new_with_label("Open");
    gtk_widget_set_name(button, "Open");
    gtk_widget_show(button);
    c_openbtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_action_proc), (void*)OpenCode);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);

    button = gtk_button_new_with_label("Place");
    gtk_widget_set_name(button, "Place");
    gtk_widget_show(button);
    c_placebtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_action_proc), (void*)PlaceCode);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);
    if (!EditIf()->hasEdit())
        gtk_widget_hide(button);

    button = gtk_button_new_with_label("Copy");
    gtk_widget_set_name(button, "Copy");
    gtk_widget_show(button);
    c_copybtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_action_proc), (void*)CopyCode);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);
    if (!EditIf()->hasEdit())
        gtk_widget_hide(button);

    button = gtk_button_new_with_label("Replace");
    gtk_widget_set_name(button, "Replace");
    gtk_widget_show(button);
    c_replbtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_action_proc), (void*)ReplCode);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);
    if (!EditIf()->hasEdit())
        gtk_widget_hide(button);

    button = gtk_button_new_with_label("Rename");
    gtk_widget_set_name(button, "Rename");
    gtk_widget_show(button);
    c_renamebtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_action_proc), (void*)RenameCode);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);
    if (!EditIf()->hasEdit())
        gtk_widget_hide(button);

    button = gtk_toggle_button_new_with_label("Search");
    gtk_widget_set_name(button, "Search");
    gtk_widget_show(button);
    c_searchbtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_action_proc), (void*)SearchCode);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);

    button = gtk_button_new_with_label("Flags");
    gtk_widget_set_name(button, "Flags");
    gtk_widget_show(button);
    c_flagbtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_action_proc), (void*)FlagCode);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);

    button = gtk_button_new_with_label("Info");
    gtk_widget_set_name(button, "Info");
    gtk_widget_show(button);
    c_infobtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_action_proc), (void*)InfoCode);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);

    button = gtk_toggle_button_new_with_label("Show");
    gtk_widget_set_name(button, "Show");
    gtk_widget_show(button);
    c_showbtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_action_proc), (void*)ShowCode);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);

    button = gtk_toggle_button_new_with_label("Filter");
    gtk_widget_set_name(button, "Filter");
    gtk_widget_show(button);
    c_fltrbtn = button;
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_action_proc), (void*)FltrCode);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);

    button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_help_proc), 0);
    gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), vbox, 0, 1, 0, 1,
        (GtkAttachOptions)0,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    vbox = gtk_vbox_new(false, 2);
    gtk_widget_show(vbox);

    //
    // title label
    //
    c_label = gtk_label_new("");
    gtk_widget_show(c_label);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), c_label);
    gtk_box_pack_start(GTK_BOX(vbox), frame, false, false, 0);

    //
    // scrolled text area
    //
    GtkWidget *contr;
    text_scrollable_new(&contr, &wb_textarea, FNT_FIXED);

    gtk_widget_add_events(wb_textarea, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "button-press-event",
        GTK_SIGNAL_FUNC(c_btn_hdlr), 0);
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "size-allocate",
        GTK_SIGNAL_FUNC(c_resize_hdlr), 0);
    // init for drag/drop
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "button-release-event",
        GTK_SIGNAL_FUNC(c_btn_release_hdlr), 0);
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "motion-notify-event",
        GTK_SIGNAL_FUNC(c_motion_hdlr), 0);
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "drag-data-get",
        GTK_SIGNAL_FUNC(c_drag_data_get), 0);
    gtk_signal_connect_after(GTK_OBJECT(wb_textarea), "realize",
        GTK_SIGNAL_FUNC(c_realize_hdlr), 0);

    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
    const char *bclr = GRpkgIf()->GetAttrColor(GRattrColorLocSel);
    gtk_text_buffer_create_tag(textbuf, "primary", "background", bclr, NULL);

    gtk_box_pack_start(GTK_BOX(vbox), contr, true, true, 0);
    gtk_table_attach(GTK_TABLE(form), vbox, 1, 2, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    button = gtk_toggle_button_new_with_label("Save Text ");
    gtk_widget_set_name(button, "Save");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_save_btn_hdlr), this);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    c_page_combo = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(hbox), c_page_combo, false, false, 0);

    //
    // dismiss button
    //
    button = gtk_toggle_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(c_cancel), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
    GtkWidget *dismiss_btn = button;

    // mode menu
    //
    c_mode_combo = gtk_option_menu_new();
    gtk_widget_show(c_mode_combo);
    gtk_box_pack_start(GTK_BOX(hbox), c_mode_combo, false, false, 0);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_show(menu);
    GtkWidget *mi = gtk_menu_item_new_with_label("Phys Cells");
    gtk_widget_show(mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(c_mode_proc), (void*)(long)Physical);
    gtk_menu_append(GTK_MENU(menu), mi);
    mi = gtk_menu_item_new_with_label("Elec Cells");
    gtk_widget_show(mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(c_mode_proc), (void*)(long)Electrical);
    gtk_menu_append(GTK_MENU(menu), mi);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(c_mode_combo), menu);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), dismiss_btn);

    if (c_caller)
        gtk_signal_connect(GTK_OBJECT(c_caller), "toggled",
            GTK_SIGNAL_FUNC(c_cancel), 0);
    check_sens();
}


sCells::~sCells()
{
    Cells = 0;
    XM()->SetTreeCaptive(false);
    if (c_caller) {
        gtk_signal_disconnect_by_func(GTK_OBJECT(c_caller),
            GTK_SIGNAL_FUNC(c_cancel), 0);
        GRX->Deselect(c_caller);
    }
    if (ListCmd)
        ListCmd->esc();
    if (GRX->GetStatus(c_showbtn))
        // erase highlighting
        DSP()->ShowCells(0);
    XM()->PopUpCellFlags(0, MODE_OFF, 0, 0);
    XM()->PopUpCellFilt(0, MODE_OFF, Physical, 0, 0);
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();
    if (c_save_pop)
        c_save_pop->popdown();
    if (c_msg_pop)
        c_msg_pop->popdown();
    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(c_cancel), wb_shell);
    delete c_pfilter;
    delete c_efilter;
    delete [] c_copyname;
    delete [] c_replname;
    delete [] c_newname;
}


void
sCells::update()
{
    select_range(0, 0);
    if (GRX->GetStatus(c_showbtn))
        DSP()->ShowCells(0);
    XM()->PopUpCellFlags(0, MODE_OFF, 0, 0);
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        if (c_clear_pop)
            c_clear_pop->popdown();
        if (c_copy_pop)
            c_copy_pop->popdown();
        if (c_repl_pop)
            c_repl_pop->popdown();
        if (c_rename_pop)
            c_rename_pop->popdown();

        GRX->Deselect(c_showbtn);

        gtk_widget_set_sensitive(c_clearbtn, false);
        gtk_widget_set_sensitive(c_openbtn, false);
        gtk_widget_set_sensitive(c_placebtn, false);
        gtk_widget_set_sensitive(c_copybtn, false);
        gtk_widget_set_sensitive(c_replbtn, false);
        gtk_widget_set_sensitive(c_renamebtn, false);
        gtk_widget_set_sensitive(c_flagbtn, false);

        gtk_widget_hide(c_clearbtn);
        gtk_widget_hide(c_openbtn);
        gtk_widget_hide(c_placebtn);
        gtk_widget_hide(c_copybtn);
        gtk_widget_hide(c_replbtn);
        gtk_widget_hide(c_renamebtn);
        gtk_widget_hide(c_flagbtn);
        gtk_widget_hide(c_fltrbtn);

        if (ActiveInput())
            ActiveInput()->popdown();
        XM()->PopUpCellFilt(0, MODE_OFF, Physical, 0, 0);
    }
    else {
        gtk_widget_set_sensitive(c_clearbtn, true);
        gtk_widget_set_sensitive(c_openbtn, true);
        if (EditIf()->hasEdit()) {
            gtk_widget_set_sensitive(c_placebtn, true);
            gtk_widget_set_sensitive(c_copybtn, true);
            gtk_widget_set_sensitive(c_replbtn, true);
            gtk_widget_set_sensitive(c_renamebtn, true);
        }
        gtk_widget_set_sensitive(c_flagbtn, true);

        gtk_widget_show(c_clearbtn);
        gtk_widget_show(c_openbtn);
        if (EditIf()->hasEdit()) {
            gtk_widget_show(c_placebtn);
            gtk_widget_show(c_copybtn);
            gtk_widget_show(c_replbtn);
            gtk_widget_show(c_renamebtn);
        }
        gtk_widget_show(c_flagbtn);
        gtk_widget_show(c_fltrbtn);
    }
    if (!c_no_update && wb_textarea && wb_textarea->window) {
        int width, height;
        gdk_window_get_size(wb_textarea->window, &width, &height);
        c_cols = (width-4)/GTKfont::stringWidth(Cells->wb_textarea, 0);
        char *s = cell_list(c_cols);
        update_text(s);
        delete [] s;
    }
    if (GRX->GetStatus(c_searchbtn) && ListCmd) {
        char *s = ListCmd->label_text();
        gtk_label_set_text(GTK_LABEL(c_label), s);
        delete [] s;
    }
    else if (DSP()->MainWdesc()->DbType() == WDchd) {
        const char *chd_msg = "Cells in displayed cell hierarchy";
        gtk_label_set_text(GTK_LABEL(c_label), chd_msg);
    }
    else {
        const char *pcells_msg = "Physical cells (+ modified, * top level)";
        const char *ecells_msg = "Electrical cells (+ modified, * top level)";
        if (c_mode == Physical)
            gtk_label_set_text(GTK_LABEL(c_label), pcells_msg);
        else
            gtk_label_set_text(GTK_LABEL(c_label), ecells_msg);
    }

    if (GRX->GetStatus(c_searchbtn)) {
        DisplayMode oldm = c_mode;
        c_mode = DSP()->CurMode();
        if (oldm != c_mode)
            XM()->PopUpCellFilt(0, MODE_UPD, c_mode, 0, 0);
        gtk_option_menu_set_history(GTK_OPTION_MENU(c_mode_combo), c_mode);
        gtk_widget_set_sensitive(c_mode_combo, false);
    }
    else
        gtk_widget_set_sensitive(c_mode_combo, true);
    check_sens();
}


// Function to clear cells from the database, or clear the entire
// database.
//
void
sCells::clear(const char *name)
{
    if (!name || !*name) {
        EV()->InitCallback();
        if (c_clear_pop)
            c_clear_pop->popdown();
        c_clear_pop = PopUpAffirm(c_clearbtn, GRloc(),
            "This will clear the database.\nContinue?", c_clear_cb, 0);
        if (c_clear_pop)
            c_clear_pop->register_usrptr((void**)&c_clear_pop);
    }
    else {
        CDcbin cbin;
        if (!CDcdb()->findSymbol(name, &cbin))
            // can't happen
            return;

        // Clear top-level empty parts.
        bool p_cleared = false;
        bool e_cleared = false;
        if (cbin.phys() && cbin.phys()->isEmpty() &&
                !cbin.phys()->isSubcell()) {
            delete cbin.phys();
            cbin.setPhys(0);
            p_cleared = true;
        }
        if (cbin.elec() && cbin.elec()->isEmpty() &&
                !cbin.elec()->isSubcell()) {
            delete cbin.elec();
            cbin.setElec(0);
            e_cleared = true;
        }
        if (p_cleared || e_cleared) {
            XM()->PopUpCells(0, MODE_UPD);
            XM()->PopUpTree(0, MODE_UPD, 0, TU_CUR);
        }
        if (!cbin.phys() && !cbin.elec()) {
            // Cell and top cell will either be gone, or both ok.
            if (!DSP()->TopCellName()) {
                FIOreadPrms prms;
                XM()->EditCell(XM()->DefaultEditName(), false, &prms);
            }
            else {
                EditIf()->ulListBegin(false, false);
                for (int i = 1; i < DSP_NUMWINS; i++) {
                    if (DSP()->Window(i) && !DSP()->Window(i)->CurCellName())
                        DSP()->Window(i)->SetCurCellName(DSP()->CurCellName());
                }
            }
            return;
        }

        if (!cbin.isSubcell()) {
            EV()->InitCallback();
            char buf[256];
            sprintf(buf,
                "This will clear %s and all its\n"
                "subcells from the database.  Continue?", name);
            if (c_clear_pop)
                c_clear_pop->popdown();
            c_clear_pop = PopUpAffirm(c_clearbtn, GRloc(), buf,
                c_clear_cb, lstring::copy(name));
            if (c_clear_pop)
                c_clear_pop->register_usrptr((void**)&c_clear_pop);
        }
        else {
            PopUpMessage(
                "Can't clear, cell is not top level in both modes.",
                true, false, false);
        }
    }
}


// Function to copy a selected cell to a new name.
//
void
sCells::copy_cell(const char *name)
{
    if (!name)
        return;
    delete [] c_copyname;
    c_copyname = lstring::copy(name);
    if (c_copy_pop)
        c_copy_pop->popdown();
    sLstr lstr;
    lstr.add("Copy cell ");
    lstr.add(c_copyname);
    lstr.add("\nEnter new name to create copy ");

    c_copy_pop = PopUpEditString((GRobject)c_copybtn, GRloc(),
        lstr.string(), c_copyname, c_copy_cb, c_copyname, 214, 0);
    if (c_copy_pop)
        c_copy_pop->register_usrptr((void**)&c_copy_pop);
}


// Function to replace selected instances with listing selection.
//
void
sCells::replace_instances(const char *name)
{
    if (!name)
        return;
    delete [] c_replname;
    c_replname = lstring::copy(name);
    if (c_repl_pop)
        c_repl_pop->popdown();
    sLstr lstr;
    lstr.add("This will replace selected cells\nwith ");
    lstr.add(c_replname);
    lstr.add(".  Continue? ");

    c_repl_pop = PopUpAffirm(Cells->c_replbtn, GRloc(), lstr.string(),
        c_repl_cb, c_replname);
    if (c_repl_pop)
        c_repl_pop->register_usrptr((void**)&c_repl_pop);
}


// Function to rename all instances of a cell in the database.
//
void
sCells::rename_cell(const char *name)
{
    if (!name)
        return;
    delete [] c_newname;
    c_newname = lstring::copy(name);
    if (c_rename_pop)
        c_rename_pop->popdown();
    sLstr lstr;
    lstr.add("Rename cell ");
    lstr.add(c_newname);
    lstr.add("\nEnter new name for cell ");
    c_rename_pop = PopUpEditString((GRobject)c_renamebtn,
        GRloc(), lstr.string(), c_newname, c_rename_cb, c_newname, 214, 0);
    if (c_rename_pop)
        c_rename_pop->register_usrptr((void**)&c_rename_pop);
}


int
sCells::button_hdlr(bool up, GtkWidget *caller, GdkEvent *event)
{
    if (up) {
        c_dragging = false;
        return (false);
    }

    if (event->type != GDK_BUTTON_PRESS)
        return (true);
    if (c_no_select)
        return (true);

    select_range(0, 0);

    char *string = text_get_chars(caller, 0, -1);
    int x = (int)event->button.x;
    int y = (int)event->button.y;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(caller), &ihere, x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    x = gtk_text_iter_get_offset(&ihere) - gtk_text_iter_get_offset(&iline);
    char *line_start = string + gtk_text_iter_get_offset(&iline);

    int start = 0;
    for ( ; start < x; start++) {
        if (line_start[start] == '\n' || line_start[start] == 0) {
            // pointing to right of line end
            delete [] string;
            return (true);
        }
    }
    if (isspace(line_start[start])) {
        // pointing at white space
        delete [] string;
        return (true);
    }
    int end = start;
    while (start > 0 && !isspace(line_start[start]))
        start--;
    if (isspace(line_start[start]))
        start++;
    while (line_start[end] && !isspace(line_start[end]))
        end++;

    char buf[256];
    char *t = buf;
    for (int i = start; i < end; i++)
        *t++ = line_start[i];
    *t = 0;

    t = buf;

    // The top level cells are listed with an '*'.
    // Modified cells are listed  with a '+'.
    while ((*t == '+' || *t == '*') && !CDcdb()->findSymbol(t)) {
        start++;
        t++;
    }

    start += (line_start - string);
    end += (line_start - string);
    delete [] string;

    if (start == end)
        return (true);
    select_range(start, end);

    c_dragging = true;
    c_drag_x = (int)event->button.x;
    c_drag_y = (int)event->button.y;

    return (true);
}


// Select the chars in the range, start=end deselects existing.  In
// GTK-1, selecting gives blue inverse, which turns gray if
// unselected, retaining an indication for the buttons.  GTK-2
// doesn't do this automatically so we provide something similar here.
//
void
sCells::select_range(int start, int end)
{
    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
    GtkTextIter istart, iend;
    if (c_end != c_start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, c_start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, c_end);
        gtk_text_buffer_remove_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    text_select_range(wb_textarea, start, end);
    if (end != start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, end);
        gtk_text_buffer_apply_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    c_start = start;
    c_end = end;
    check_sens();
}


void
sCells::action_hdlr(GtkWidget *caller, void *client_data)
{
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();

    c_no_select = true;
    char *cname = text_get_selection(wb_textarea);

    if (client_data == (void*)ClearCode) {
        clear(cname);
    }
    else if (client_data == (void*)TreeCode) {
        if (cname) {
            // find the main menu button for the tree popup
            MenuEnt *m = Menu()->FindEntry("cell", MenuTREE);
            GtkWidget *widg = 0;
            if (m)
                widg = (GtkWidget*)m->cmd.caller;
            XM()->PopUpTree(widg, MODE_ON, cname,
                c_mode == Physical ? TU_PHYS : TU_ELEC);
            // If tree is "captive" don't update after main window
            // display mode switch.
            XM()->SetTreeCaptive(true);

            if (widg)
                GRX->Select(widg);
        }
    }
    else if (client_data == (void*)OpenCode) {
        if (cname) {
            c_no_update = true;
            EV()->InitCallback();
            XM()->EditCell(cname, false);
            if (Cells)
                c_no_update = false;
        }
    }
    else if (client_data == (void*)PlaceCode) {
        if (cname) {
            EV()->InitCallback();
            EditIf()->addMaster(cname, 0);
        }
    }
    else if (client_data == (void*)CopyCode) {
        copy_cell(cname);
    }
    else if (client_data == (void*)ReplCode) {
        replace_instances(cname);
    }
    else if (client_data == (void*)RenameCode) {
        rename_cell(cname);
    }
    else if (client_data == (void*)SearchCode) {
        bool state = GRX->GetStatus(caller);
        if (state)
            EV()->InitCallback();
        ListState::show_search(state);
    }
    else if (client_data == (void*)FlagCode) {
        stringlist *s0;
        if (cname)
            s0 = new stringlist(lstring::copy(cname), 0);
        else
            s0 = raw_cell_list(0, 0, true);
        XM()->PopUpCellFlags(c_flagbtn, MODE_ON, s0, c_mode);
        stringlist::destroy(s0);
    }
    else if (client_data == (void*)InfoCode) {
        if (DSP()->MainWdesc()->DbType() == WDchd) {
            cCHD *chd = CDchd()->chdRecall(DSP()->MainWdesc()->DbName(), false);
            if (chd) {
                symref_t *p = chd->findSymref(cname, DSP()->CurMode(), true);
                if (p) {
                    stringlist *sl = new stringlist(
                        lstring::copy(Tstring(p->get_name())), 0);
                    int flgs = FIO_INFO_OFFSET | FIO_INFO_INSTANCES |
                        FIO_INFO_BBS | FIO_INFO_FLAGS;
                    char *str = chd->prCells(0, DSP()->CurMode(), flgs, sl);
                    stringlist::destroy(sl);
                    PopUpInfo(MODE_ON, str, STY_FIXED);
                    delete [] str;
                }
            }
        }
        else
            XM()->ShowCellInfo(cname, true, c_mode);
    }
    else if (client_data == (void*)ShowCode) {
        bool state = GRX->GetStatus(caller);
        if (state)
            DSP()->ShowCells(cname);
        else
            DSP()->ShowCells(0);
    }
    else if (client_data == (void*)FltrCode) {
        bool state = GRX->GetStatus(caller);
        if (state)
            XM()->PopUpCellFilt(caller, MODE_ON, c_mode, c_filter_cb, 0);
        else
            XM()->PopUpCellFilt(0, MODE_OFF, Physical, 0, 0);
    }

    if (Cells)
        c_no_select = false;
    delete [] cname;
}


int
sCells::motion_hdlr(GtkWidget *widget, GdkEvent *event)
{
    if (c_dragging) {
#if GTK_CHECK_VERSION(2,12,0)
        if (event->motion.is_hint)
            gdk_event_request_motions((GdkEventMotion*)event);
#else
        // Strange voodoo to "turn on" motion events, that are
        // otherwise suppressed since GDK_POINTER_MOTION_HINT_MASK
        // is set.  See GdkEventMask doc.
        gdk_window_get_pointer(widget->window, 0, 0, 0);
#endif
        if ((abs((int)event->motion.x - c_drag_x) > 4 ||
                abs((int)event->motion.y - c_drag_y) > 4)) {
            c_dragging = false;
            GtkTargetList *targets = gtk_target_list_new(target_table,
                n_targets);
            GdkDragContext *drcx = gtk_drag_begin(widget, targets,
                (GdkDragAction)GDK_ACTION_COPY, 1, event);
            gtk_drag_set_icon_default(drcx);
            return (true);
        }
    }
    return (false);
}


void
sCells::resize_hdlr(GtkAllocation *a)
{
    int cols = (a->width-4)/GTKfont::stringWidth(wb_textarea, 0) - 2;
    if (cols == c_cols)
        return;
    c_cols = cols;
    char *s = cell_list(cols);
    update_text(s);
    delete [] s;
    select_range(0, 0);
}


namespace {
    // Sort comparison, strip junk prepended to cell names.
    bool
    cl_comp(const char *s1, const char *s2)
    {
        while (isspace(*s1) || *s1 == '*' || *s1 == '+')
            s1++;
        while (isspace(*s2) || *s2 == '*' || *s2 == '+')
            s2++;
        return (strcmp(s1, s2) < 0);
    }
}


stringlist *
sCells::raw_cell_list(int *pcnt, int *ppgs, bool nomark)
{
    if (pcnt)
        *pcnt = 0;
    if (ppgs)
        *ppgs = 0;
    stringlist *s0 = 0;
    if (GRX->GetStatus(Cells->c_searchbtn) && ListCmd)
        s0 = ListCmd->cell_list(nomark);
    else {
        if (DSP()->MainWdesc()->DbType() == WDchd) {
            cCHD *chd = CDchd()->chdRecall(DSP()->MainWdesc()->DbName(), false);
            if (chd)
                s0 = chd->listCellnames(c_mode, true);
        }
        else {
            CDgenTab_cbin sgen;
            CDcbin cbin;
            while (sgen.next(&cbin) != false) {
                if (c_mode == Physical) {
                    if (!cbin.phys())
                        continue;
                    if (c_pfilter && !c_pfilter->inlist(&cbin))
                        continue;
                }
                if (c_mode == Electrical) {
                    if (!cbin.elec())
                        continue;
                    if (c_efilter && !c_efilter->inlist(&cbin))
                        continue;
                }
                if (nomark) {
                    s0 = new stringlist(lstring::copy(
                        Tstring(cbin.cellname())), s0);
                }
                else {
                    int j = 0;
                    char buf[128];
                    if (c_mode == Physical) {
                        if (cbin.phys()->isModified())
                            buf[j++] = '+';
                        if (!cbin.phys()->isSubcell())
                            buf[j++] = '*';
                    }
                    else {
                        if (cbin.elec()->isModified())
                            buf[j++] = '+';
                        if (!cbin.elec()->isSubcell())
                            buf[j++] = '*';
                    }
                    if (!j)
                        buf[j++] = ' ';
                    strcpy(buf+j, Tstring(cbin.cellname()));
                    s0 = new stringlist(lstring::copy(buf), s0);
                }
            }
            stringlist::sort(s0, &cl_comp);
        }
    }

    int pagesz = 0;
    const char *s = CDvdb()->getVariable(VA_ListPageEntries);
    if (s) {
        pagesz = atoi(s);
        if (pagesz < 100 || pagesz > 50000)
            pagesz = 0;
    }
    if (pagesz == 0)
        pagesz = DEF_LIST_MAX_PER_PAGE;

    int cnt = 0;
    for (stringlist *sl = s0; sl; sl = sl->next)
        cnt++;
    int min = c_page * pagesz;
    while (min >= cnt && min > 0) {
        c_page--;
        min = c_page * pagesz;
    }
    int max = min + pagesz;

    cnt = 0;
    stringlist *slprev = 0;
    for (stringlist *sl = s0; sl; slprev = sl, cnt++, sl = sl->next) {
        if (cnt < min)
            continue;
        if (cnt == min) {
            if (slprev) {
                slprev->next = 0;
                stringlist::destroy(s0);
                s0 = sl;
            }
            continue;
        }
        if (cnt >= max) {
            for (stringlist *st = sl; st; st = st->next)
                cnt++;
            stringlist::destroy(sl->next);
            sl->next = 0;
            break;
        }
    }
    if (pcnt)
        *pcnt = cnt;
    if (ppgs)
        *ppgs = pagesz;
    return (s0);
}


// Return a formatted list of cells found in the database.
//
char *
sCells::cell_list(int cols)
{
    int cnt, pagesz;
    stringlist *s0 = raw_cell_list(&cnt, &pagesz, false);
    if (cnt <= pagesz)
        gtk_widget_hide(c_page_combo);
    else {
        char buf[128];
        GtkWidget *menu = gtk_menu_new();
        gtk_widget_show(menu);
        for (int i = 0; i*pagesz < cnt; i++) {
            int tmpmax = (i+1)*pagesz;
            if (tmpmax > cnt)
                tmpmax = cnt;
            sprintf(buf, "%d - %d", i*pagesz, tmpmax);
            GtkWidget *mi = gtk_menu_item_new_with_label(buf);
            gtk_widget_show(mi);
            gtk_signal_connect(GTK_OBJECT(mi), "activate",
                GTK_SIGNAL_FUNC(c_page_proc), (void*)(long)i);
            gtk_menu_append(GTK_MENU(menu), mi);
        }
        gtk_option_menu_remove_menu(GTK_OPTION_MENU(c_page_combo));
        gtk_option_menu_set_menu(GTK_OPTION_MENU(c_page_combo), menu);
        gtk_option_menu_set_history(GTK_OPTION_MENU(c_page_combo), c_page);
        gtk_widget_show(c_page_combo);
    }

    char *t = stringlist::col_format(s0, cols);
    stringlist::destroy(s0);
    return (t);
}


// Refresh the text while keeping current top location.
//
void
sCells::update_text(char *newtext)
{
    if (wb_textarea == 0 || newtext == 0)
        return;
    double val = text_get_scroll_value(wb_textarea);
    text_set_chars(wb_textarea, newtext);
    text_set_scroll_value(wb_textarea, val);
}


// Set button sensitivity according to mode.
//
void
sCells::check_sens()
{
    bool has_sel = text_has_selection(wb_textarea);
    if (!has_sel) {
        if (GRX->GetStatus(c_showbtn))
            DSP()->ShowCells(0);
        gtk_widget_set_sensitive(c_treebtn, false);
        gtk_widget_set_sensitive(c_openbtn, false);
        gtk_widget_set_sensitive(c_placebtn, false);
        gtk_widget_set_sensitive(c_copybtn, false);
        gtk_widget_set_sensitive(c_replbtn, false);
        gtk_widget_set_sensitive(c_renamebtn, false);
    }
    else {
        bool mode_ok = (c_mode == DSP()->CurMode());
        gtk_widget_set_sensitive(c_treebtn, true);
        if (DSP()->MainWdesc()->DbType() == WDcddb) {
            gtk_widget_set_sensitive(c_openbtn, mode_ok);
            if (EditIf()->hasEdit()) {
                bool ed_ok = !CurCell() || !CurCell()->isImmutable();
                gtk_widget_set_sensitive(c_placebtn, mode_ok && ed_ok);
                gtk_widget_set_sensitive(c_copybtn, true);
                gtk_widget_set_sensitive(c_replbtn, mode_ok && ed_ok &&
                    (Selections.firstObject(CurCell(), "c")) != 0);
                gtk_widget_set_sensitive(c_renamebtn, ed_ok);
            }
        }
        if (GRX->GetStatus(c_showbtn)) {
            char *cn = text_get_selection(wb_textarea);
            DSP()->ShowCells(cn);
            delete [] cn;
        }
    }
}


// Static function.
// Callback for the clear button dialog.
//
void
sCells::c_clear_cb(bool state, void *arg)
{
    char *name = (char*)arg;
    if (state)
        XM()->Clear(name);
    delete [] name;
}


// Static function.
// Callback for the copy button dialog.
//
ESret
sCells::c_copy_cb(const char *newname, void *arg)
{
    char *name = (char*)arg;
    if (EditIf()->copySymbol(name, newname)) {
        CDcbin cbin;
        if (CDcdb()->findSymbol(newname, &cbin)) {
            // select new cell
            dspPkgIf()->RegisterIdleProc(c_highlight_idle,
                (void*)cbin.cellname());
        }
        return (ESTR_DN);
    }
    sLstr lstr;
    lstr.add("Copy cell ");
    lstr.add(name);
    lstr.add("\nName invalid or not unique, try again ");
    Cells->c_copy_pop->update(lstr.string(), 0);
    return (ESTR_IGN);
}


// Static function.
// Callback for the replace dialog.
//
void
sCells::c_repl_cb(bool state, void *arg)
{
    if (state) {
        Errs()->init_error();
        EV()->InitCallback();
        CDs *cursd = CurCell();
        if (!cursd)
            return;
        EditIf()->ulListCheck("replace", cursd, false);
        sSelGen sg(Selections, cursd, "c");
        CDc *cd;
        while ((cd = (CDc*)sg.next()) != 0) {
            CDcbin cbin;
            CD()->OpenExisting((const char*)arg, &cbin);
            if (!EditIf()->replaceInstance(cd, &cbin, false, false)) {
                Errs()->add_error("ReplaceCell failed");
                Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
            }
        }
        EditIf()->ulCommitChanges(true);
        XM()->PopUpCells(0, MODE_UPD);
        XM()->PopUpTree(0, MODE_UPD, 0, TU_CUR);
    }
}


// Static function.
// Callback for the rename dialog.
//
ESret
sCells::c_rename_cb(const char *newname, void *arg)
{
    if (!Cells)
        return (ESTR_DN);
    char *name = (char*)arg;
    if (EditIf()->renameSymbol(name, newname)) {
        EV()->InitCallback();
        XM()->ShowParameters();
        XM()->PopUpCells(0, MODE_UPD);
        XM()->PopUpTree(0, MODE_UPD, 0, TU_CUR);

        CDcbin cbin;
        if (CDcdb()->findSymbol(newname, &cbin))
            // select renamed cell
            dspPkgIf()->RegisterIdleProc(c_highlight_idle,
                (void*)cbin.cellname());

        WindowDesc *wd;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wd = wgen.next()) != 0) {
            if (wd->Attrib()->expand_level(wd->Mode()) != -1)
                wd->Redisplay(0);
        }
        const char *nn = Tstring(cbin.cellname());
        for (const char *n = nn; *n; n++) {
            if (isalnum(*n) || *n == '_' || *n == '$' || *n == '?')
                continue;
            Log()->WarningLogV("rename",
                "New cell name \"%s\" contains non-portable character,\n"
                "recommend cell names use alphanum and $_? only.", nn);
            break;
        }
        return (ESTR_DN);
    }
    sLstr lstr;
    lstr.add("Rename cell ");
    lstr.add(name);
    lstr.add(Errs()->get_error());
    Cells->c_rename_pop->update(lstr.string(), 0);
    return (ESTR_IGN);
}


void
sCells::c_page_proc(GtkWidget*, void *arg)
{
    if (Cells) {
        int i = (intptr_t)arg;
        if (Cells->c_page != i) {
            Cells->c_page = i;
            Cells->update();
        }
    }
}


void
sCells::c_mode_proc(GtkWidget*, void *arg)
{
    DisplayMode m = (DisplayMode)(intptr_t)arg;
    if (Cells && Cells->c_mode != m) {
        Cells->c_mode = m;
        XM()->PopUpCellFilt(0, MODE_UPD, Cells->c_mode, 0, 0);
        Cells->update();
        XM()->PopUpTree(0, MODE_UPD, 0,
            Cells->c_mode == Physical ? TU_PHYS : TU_ELEC);
    }
}


// Called from the filter panel Apply button.  The flt is a copy that
// we need to own.
//
void
sCells::c_filter_cb(cfilter_t *flt, void*)
{
    if (!Cells) {
        delete flt;
        return;
    }
    if (!flt)
        return;
    if (flt->mode() == Physical) {
        delete Cells->c_pfilter;
        Cells->c_pfilter = flt;
    }
    else {
        delete Cells->c_efilter;
        Cells->c_efilter = flt;
    }
    Cells->update();
}


// Static function.
// Idle proc to highlight the listing for name.
//
int
sCells::c_highlight_idle(void *arg)
{
    if (Cells && arg) {
        const char *name = (const char*)arg;
        int n = strlen(name);
        if (!n)
            return (0);
        char *s = text_get_chars(Cells->wb_textarea, 0, -1);
        char *t = s;
        while (*t) {
            while (isspace(*t))
                t++;
            if (*t == '*' || *t == '+')
                t++;
            if (*t == '*' || *t == '+')
                t++;
            if (!*t)
                break;
            if (!strncmp(t, name, n) && (isspace(t[n]) || !t[n])) {
                int top = t - s;
                Cells->select_range(top, top + n);
                break;
            }
            while (*t && !isspace(*t))
                t++;
        }
        delete [] s;
    }
    return (0);
}


// Static function.
// Data-get function, for drag/drop.
//
void
sCells::c_drag_data_get(GtkWidget *widget, GdkDragContext*,
    GtkSelectionData *selection_data, guint, guint, void*)
{
    if (GTK_IS_TEXT_VIEW(widget))
    // stop text view native handler
    gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "drag-data-get");

    char *string = text_get_selection(widget);
    if (string) {
        gtk_selection_data_set(selection_data, selection_data->target,
            8, (unsigned char*)string, strlen(string)+1);
        delete [] string;
    }
}


// Static function.
// Handle button presses in the text area.
//
int
sCells::c_btn_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (Cells)
        return (Cells->button_hdlr(false, caller, event));
    return (true);
}


// Static function.
int
sCells::c_btn_release_hdlr(GtkWidget*, GdkEvent*, void*)
{
    if (Cells)
        Cells->button_hdlr(true, 0, 0);
    return (false);
}


// Static function.
// Motion handler, begin drag.
//
int
sCells::c_motion_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (Cells)
        return (Cells->motion_hdlr(caller, event));
    return (false);
}


// Static function.
void
sCells::c_resize_hdlr(GtkWidget *widget, GtkAllocation *a)
{
    if (Cells && GTK_WIDGET_REALIZED(widget))
        Cells->resize_hdlr(a);
}


// Static function.
void
sCells::c_realize_hdlr(GtkWidget *widget, void *arg)
{
    text_realize_proc(widget, arg);
    if (Cells)
        Cells->update();
}


// Static function.
// If there is something selected, perform the action.
//
void
sCells::c_action_proc(GtkWidget *caller, void *client_data)
{
    if (Cells)
        Cells->action_hdlr(caller, client_data);
}


// Static function.
// Handler for the help button.
//
void
sCells::c_help_proc(GtkWidget*, void*)
{
    DSPmainWbag(PopUpHelp("cellspanel"))
}


// Static function.
// Pop down the cells popup
//
void
sCells::c_cancel(GtkWidget*, void*)
{
    XM()->PopUpCells(0, MODE_OFF);
}


// Private static GTK signal handler.
// Handle Save Text button presses.
//
void
sCells::c_save_btn_hdlr(GtkWidget *widget, void *arg)
{
    if (GRX->GetStatus(widget)) {
        sCells *cp = (sCells*)arg;
        if (cp->c_save_pop)
            return;
        cp->c_save_pop = new GTKledPopup(0,
            "Enter path to file for saved text:", "", 200, false, 0, arg);
        cp->c_save_pop->register_caller(widget, false, true);
        cp->c_save_pop->register_callback(
            (GRledPopup::GRledCallback)&cp->c_save_cb);
        cp->c_save_pop->register_usrptr((void**)&cp->c_save_pop);

        gtk_window_set_transient_for(GTK_WINDOW(cp->c_save_pop->pw_shell),
            GTK_WINDOW(cp->wb_shell));
        GRX->SetPopupLocation(GRloc(), cp->c_save_pop->pw_shell,
            cp->wb_shell);
        cp->c_save_pop->set_visible(true);
    }
}


// Private static handler.
// Callback for the save file name pop-up.
//
ESret
sCells::c_save_cb(const char *string, void *arg)
{
    sCells *cp = (sCells*)arg;
    if (string) {
        if (!filestat::create_bak(string)) {
            cp->c_save_pop->update(
                "Error backing up existing file, try again", 0);
            return (ESTR_IGN);
        }
        FILE *fp = fopen(string, "w");
        if (!fp) {
            cp->c_save_pop->update("Error opening file, try again", 0);
            return (ESTR_IGN);
        }
        char *txt = text_get_chars(cp->wb_textarea, 0, -1);
        if (txt) {
            unsigned int len = strlen(txt);
            if (len) {
                if (fwrite(txt, 1, len, fp) != len) {
                    cp->c_save_pop->update("Write failed, try again", 0);
                    delete [] txt;
                    fclose(fp);
                    return (ESTR_IGN);
                }
            }
            delete [] txt;
        }
        fclose(fp);

        if (cp->c_msg_pop)
            cp->c_msg_pop->popdown();
        cp->c_msg_pop = new GTKmsgPopup(0, "Text saved in file.", false);
        cp->c_msg_pop->register_usrptr((void**)&cp->c_msg_pop);
        gtk_window_set_transient_for(GTK_WINDOW(cp->c_msg_pop->pw_shell),
            GTK_WINDOW(cp->wb_shell));
        GRX->SetPopupLocation(GRloc(), cp->c_msg_pop->pw_shell,
            cp->wb_shell);
        cp->c_msg_pop->set_visible(true);
        gtk_timeout_add(2000, c_timeout, cp);
    }
    return (ESTR_DN);
}


int
sCells::c_timeout(void *arg)
{
    sCells *cp = (sCells*)arg;
    if (cp->c_msg_pop)
        cp->c_msg_pop->popdown();
    return (0);
}
// End of sCells functions.


//-----------------------
// ListState functions

ListState::~ListState()
{
    ListCmd = 0;
}


// Static function.
// The search function, print a listing of the cells found in an area
// of the screen that the user defines.
//
void
ListState::show_search(bool on)
{
    if (on) {
        if (ListCmd)
            return;
        ListCmd = new ListState("SEARCH", "xic:cells#search");
        if (!EV()->PushCallback(ListCmd)) {
            delete ListCmd;
            ListCmd = 0;
            return;
        }
        ListCmd->message();
    }
    else {
        if (!ListCmd)
            return;
        ListCmd->esc();
    }
    Cells->update();
}


char *
ListState::label_text()
{
    char buf[256];
    if (lsAOI == CDinfiniteBB) {
        buf[0] = 0;
        if (DSP()->MainWdesc()->DbType() == WDchd) {
            cCHD *chd = CDchd()->chdRecall(DSP()->MainWdesc()->DbName(), false);
            if (chd) {
                symref_t *p = chd->findSymref(DSP()->MainWdesc()->DbCellName(),
                    DSP()->CurMode(), true);
                sprintf(buf, "Cells under %s",
                    p ? Tstring(p->get_name()) : "<unknown>");
            }
        }
        else
            sprintf(buf, "Cells under %s", Tstring(DSP()->CurCellName()));
    }
    else {
        if (CDvdb()->getVariable(VA_InfoInternal))
            sprintf(buf, "Cells intersecting (%d,%d %d,%d)",
                lsAOI.left, lsAOI.bottom, lsAOI.right, lsAOI.top);
        else if (DSP()->CurMode() == Physical) {
            int ndgt = CD()->numDigits();
            sprintf(buf, "Cells intersecting (%.*f,%.*f %.*f,%.*f)",
                ndgt, MICRONS(lsAOI.left), ndgt, MICRONS(lsAOI.bottom),
                ndgt, MICRONS(lsAOI.right), ndgt, MICRONS(lsAOI.top));
        }
        else {
            sprintf(buf, "Cells intersecting (%.3f,%.3f %.3f,%.3f)",
                ELEC_MICRONS(lsAOI.left), ELEC_MICRONS(lsAOI.bottom),
                ELEC_MICRONS(lsAOI.right), ELEC_MICRONS(lsAOI.top));
        }
    }
    return (lstring::copy(buf));
}


stringlist *
ListState::cell_list(bool nomark)
{
    stringlist *s0 = 0;
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        cCHD *chd = CDchd()->chdRecall(DSP()->MainWdesc()->DbName(), false);
        if (chd) {
            const char *cname = DSP()->MainWdesc()->DbCellName();
            syrlist_t *sy0 = chd->listing(DSP()->CurMode(), cname, false,
                &lsAOI);
            for (syrlist_t *s = sy0; s; s = s->next) {
                s0 = new stringlist(
                    lstring::copy(Tstring(s->symref->get_name())), s0);
            }
            syrlist_t::destroy(sy0);
        }
        stringlist::destroy(s0);
    }
    else {
        if (!DSP()->CurCellName())
            return (0);
        s0 = CurCell()->listSubcells(-1, true, !nomark, &lsAOI);
    }
    return (s0);
}


// Finish selection.  If success, do the list.
//
void
ListState::b1up()
{
    BBox BB;
    if (cEventHdlr::sel_b1up(&BB, 0, B1UP_NOSEL)) {
        lsAOI = BB;
        Cells->update();
    }
}


// Esc entered, clean up and exit.
//
void
ListState::esc()
{
    cEventHdlr::sel_esc();
    if (Cells)
        Cells->end_search();
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    delete this;
}

