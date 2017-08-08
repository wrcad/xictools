
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
#include "drc.h"
#include "fio.h"
#include "fio_chd.h"
#include "cd_digest.h"
#include "dsp_inlines.h"
#include "dsp_color.h"
#include "events.h"
#include "ghost.h"
#include "errorlog.h"
#include "promptline.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkspinbtn.h"
#ifdef WIN32
#include "windows.h"
#endif
#include <signal.h>


//------------------------------------------------------------------------
// DRC Run Pop-Up
//
// Allowe initiation and control of batch DRC runs.
//
// Help system keywords used:
//  xic:check

namespace {
    namespace gtkdrcrun {
        struct WinState : public CmdState
        {
            WinState(const char*, const char*);
            virtual ~WinState();

            void b1down();
            void b1up();
            void esc();

            void message() { PL()->ShowPrompt(msg1p); }

        private:
            BBox AOI;
            int state;
            bool ghost_on;

            static const char *msg1p;
        };

        struct sDC
        {
            friend struct WinState;

            sDC(GRobject);
            ~sDC();

            GtkWidget *shell() { return (dc_popup); }

            void update();
            void update_jobs_list();

        private:
            void select_range(int, int);
            int get_pid();
            void select_pid(int);

            static void dc_cancel_proc(GtkWidget*, void*);
            static void dc_action_proc(GtkWidget*, void*);
            static void dc_val_changed(GtkWidget*, void*);
            static void dc_text_changed(GtkWidget*, void*);
            static int dc_button_dn(GtkWidget*, GdkEvent*, void*);
            static void dc_font_changed();

            void dc_region();
            void dc_region_quit();

            GRobject dc_caller;
            GtkWidget *dc_popup;
            GtkWidget *dc_use;
            GtkWidget *dc_chdname;
            GtkWidget *dc_cname;
            GtkWidget *dc_none;
            GtkWidget *dc_wind;
            GtkWidget *dc_flat;
            GtkWidget *dc_set;
            GtkWidget *dc_l_label;
            GtkWidget *dc_b_label;
            GtkWidget *dc_r_label;
            GtkWidget *dc_t_label;

            GtkWidget *dc_jobs;
            GtkWidget *dc_kill;

            GTKspinBtn sb_part;
            GTKspinBtn sb_left;
            GTKspinBtn sb_bottom;
            GTKspinBtn sb_right;
            GTKspinBtn sb_top;

            int dc_start;
            int dc_end;
            int dc_line_selected;

            static double dc_last_part_size;
            static double dc_l_val;
            static double dc_b_val;
            static double dc_r_val;
            static double dc_t_val;
            static bool dc_use_win;
            static bool dc_flatten;
            static bool dc_use_chd;
        };

        WinState *WinCmd;
        sDC *DC;
    }
}

using namespace gtkdrcrun;

const char *WinState::msg1p = "Click twice or drag to set area.";

double sDC::dc_last_part_size = DRC_PART_DEF;
double sDC::dc_l_val = 0.0;
double sDC::dc_b_val = 0.0;
double sDC::dc_r_val = 0.0;
double sDC::dc_t_val = 0.0;
bool sDC::dc_use_win = false;
bool sDC::dc_flatten = false;
bool sDC::dc_use_chd = false;


void
cDRC::PopUpDrcRun(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete DC;
        return;
    }
    if (mode == MODE_UPD) {
        if (DC)
            DC->update();
        return;
    }
    if (DC)
        return;

    new sDC(caller);
    if (!DC->shell()) {
        delete DC;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(DC->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), DC->shell(), mainBag()->Viewport());
    gtk_widget_show(DC->shell());
}
// End of cDRC functions.


sDC::sDC(GRobject c)
{
    DC = this;
    dc_caller = c;
    dc_popup = 0;
    dc_use = 0;
    dc_chdname = 0;
    dc_cname = 0;
    dc_none = 0;
    dc_wind = 0;
    dc_flat = 0;
    dc_set = 0;
    dc_l_label = 0;
    dc_b_label = 0;
    dc_r_label = 0;
    dc_t_label = 0;
    dc_jobs = 0;
    dc_kill = 0;
    dc_start = 0;
    dc_end = 0;
    dc_line_selected = -1;

    dc_popup = gtk_NewPopup(0, "DRC Run Control", dc_cancel_proc, 0);
    if (!dc_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(dc_popup), false);

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(dc_popup), form);
    int f_rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("Initiate batch DRC run");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(row), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(dc_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, f_rowcnt, f_rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    f_rowcnt++;

    GtkWidget *nbook = gtk_notebook_new();
    gtk_widget_show(nbook);

    gtk_table_attach(GTK_TABLE(form), nbook, 0, 1, f_rowcnt, f_rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    f_rowcnt++;

    //
    // Run page
    //

    GtkWidget *table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);
    int rowcnt = 0;

    //
    // CHD name
    //
    label = gtk_label_new("CHD reference name");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(table), label, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    dc_use = gtk_toggle_button_new_with_label("Use");
    gtk_widget_set_name(dc_use, "use");
    gtk_widget_show(dc_use);
    gtk_signal_connect(GTK_OBJECT(dc_use), "clicked",
        GTK_SIGNAL_FUNC(dc_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), dc_use, false, false, 0);

    dc_chdname = gtk_entry_new();
    gtk_widget_show(dc_chdname);
    gtk_signal_connect(GTK_OBJECT(dc_chdname), "changed",
        GTK_SIGNAL_FUNC(dc_text_changed), 0);
    gtk_box_pack_start(GTK_BOX(row), dc_chdname, true, true, 0);

    gtk_table_attach(GTK_TABLE(table), row, 2, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("CHD top cell");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(table), label, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    dc_cname = gtk_entry_new();
    gtk_widget_show(dc_cname);
    gtk_signal_connect(GTK_OBJECT(dc_cname), "changed",
        GTK_SIGNAL_FUNC(dc_text_changed), 0);

    gtk_table_attach(GTK_TABLE(table), dc_cname, 2, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);

    gtk_table_attach(GTK_TABLE(table), hsep, 0, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Gridding.
    //
    label = gtk_label_new("Partition grid size");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);

    gtk_table_attach(GTK_TABLE(table), label, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    dc_none = gtk_toggle_button_new_with_label("None");
    gtk_widget_set_name(dc_none, "none");
    gtk_widget_show(dc_none);
    gtk_signal_connect(GTK_OBJECT(dc_none), "clicked",
        GTK_SIGNAL_FUNC(dc_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), dc_none, false, false, 0);

    GtkWidget *sb = sb_part.init(DRC_PART_DEF, DRC_PART_MIN, DRC_PART_MAX, 2);
    sb_part.connect_changed(GTK_SIGNAL_FUNC(dc_val_changed), 0, "partsize");
    gtk_widget_set_usize(sb, 80, -1);
    gtk_box_pack_start(GTK_BOX(row), sb, true, true, 0);

    gtk_table_attach(GTK_TABLE(table), row, 2, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);

    gtk_table_attach(GTK_TABLE(table), hsep, 0, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Use Window
    //
    dc_wind = gtk_check_button_new_with_label("Use window");
    gtk_widget_set_name(dc_wind, "wind");
    gtk_widget_show(dc_wind);
    gtk_signal_connect(GTK_OBJECT(dc_wind), "clicked",
        GTK_SIGNAL_FUNC(dc_action_proc), this);

    gtk_table_attach(GTK_TABLE(table), dc_wind, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    dc_flat = gtk_check_button_new_with_label("Flatten");
    gtk_widget_set_name(dc_flat, "flat");
    gtk_widget_show(dc_flat);
    gtk_signal_connect(GTK_OBJECT(dc_flat), "clicked",
        GTK_SIGNAL_FUNC(dc_action_proc), this);

    gtk_table_attach(GTK_TABLE(table), dc_flat, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    dc_set = gtk_toggle_button_new_with_label("Set");
    gtk_widget_set_name(dc_set, "set");
    gtk_widget_show(dc_set);
    gtk_signal_connect(GTK_OBJECT(dc_set), "clicked",
        GTK_SIGNAL_FUNC(dc_action_proc), 0);

    gtk_table_attach(GTK_TABLE(table), dc_set, 3, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Window LBRT
    //
    dc_l_label = gtk_label_new("Left");
    gtk_widget_show(dc_l_label);
    gtk_misc_set_padding(GTK_MISC(dc_l_label), 2, 2);

    gtk_table_attach(GTK_TABLE(table), dc_l_label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    int ndgt = CD()->numDigits();

    sb = sb_left.init(0.0, -1e6, 1e6, ndgt);
    sb_left.connect_changed(GTK_SIGNAL_FUNC(dc_val_changed), this, "left");
    gtk_widget_set_usize(sb, 80, -1);

    gtk_table_attach(GTK_TABLE(table), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    dc_b_label = gtk_label_new("Bottom");
    gtk_widget_show(dc_b_label);
    gtk_misc_set_padding(GTK_MISC(dc_b_label), 2, 2);
    gtk_table_attach(GTK_TABLE(table), dc_b_label, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_bottom.init(0.0, -1e6, 1e6, ndgt);
    sb_bottom.connect_changed(GTK_SIGNAL_FUNC(dc_val_changed), this, "bottom");
    gtk_widget_set_usize(sb, 80, -1);

    gtk_table_attach(GTK_TABLE(table), sb, 3, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    dc_r_label = gtk_label_new("Right");
    gtk_widget_show(dc_r_label);
    gtk_misc_set_padding(GTK_MISC(dc_r_label), 2, 2);

    gtk_table_attach(GTK_TABLE(table), dc_r_label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_right.init(0.0, -1e6, 1e6, ndgt);
    sb_right.connect_changed(GTK_SIGNAL_FUNC(dc_val_changed), this, "right");
    gtk_widget_set_usize(sb, 80, -1);

    gtk_table_attach(GTK_TABLE(table), sb, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    dc_t_label = gtk_label_new("Top");
    gtk_widget_show(dc_t_label);
    gtk_misc_set_padding(GTK_MISC(dc_t_label), 2, 2);
    gtk_table_attach(GTK_TABLE(table), dc_t_label, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sb = sb_top.init(0.0, -1e6, 1e6, ndgt);
    sb_top.connect_changed(GTK_SIGNAL_FUNC(dc_val_changed), this, "top");
    gtk_widget_set_usize(sb, 80, -1);

    gtk_table_attach(GTK_TABLE(table), sb, 3, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);

    gtk_table_attach(GTK_TABLE(table), hsep, 0, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Check, Check Bg buttons
    //
    button = gtk_toggle_button_new_with_label("Check");
    gtk_widget_set_name(button, "check");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(dc_action_proc), 0);
    GtkWidget *check_btn = button;

    // This is black magic to allow button pressess/releases to be
    // dispatched when the busy flag is set.  Un-setting the Check
    // button will pause the DRC run.
    gtk_object_set_data(GTK_OBJECT(button), "abort", (void*)1);

    gtk_table_attach(GTK_TABLE(table), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_toggle_button_new_with_label("Check in\nBackground");
    gtk_widget_set_name(button, "checkbg");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(dc_action_proc), 0);

    gtk_table_attach(GTK_TABLE(table), button, 2, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    label = gtk_label_new("Run");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), table, label);

    //
    // Jobs page
    //

    table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);
    rowcnt = 0;

    GtkWidget *contr;
    text_scrollable_new(&contr, &dc_jobs, FNT_FIXED);

    gtk_widget_add_events(dc_jobs, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(dc_jobs), "button-press-event",
        GTK_SIGNAL_FUNC(dc_button_dn), 0);

    // The font change pop-up uses this to redraw the widget
    gtk_object_set_data(GTK_OBJECT(dc_jobs), "font_changed",
        (void*)dc_font_changed);

    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(dc_jobs));
    const char *bclr = GRpkgIf()->GetAttrColor(GRattrColorLocSel);
    gtk_text_buffer_create_tag(textbuf, "primary", "background", bclr,
#if GTK_CHECK_VERSION(2,8,0)
        "paragraph-background", bclr,
#endif
        NULL);

    gtk_widget_set_usize(dc_jobs, 200, 200);

    gtk_table_attach(GTK_TABLE(table), contr, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_button_new_with_label("Abort job");
    gtk_widget_set_name(button, "abort");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(dc_action_proc), 0);
    dc_kill = button;

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Jobs");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), table, label);

    //
    // Dismiss button
    //

    hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);

    gtk_table_attach(GTK_TABLE(form), hsep, 0, 1, f_rowcnt, f_rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    f_rowcnt++;

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(dc_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, f_rowcnt, f_rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(dc_popup), check_btn);

    update();
}


sDC::~sDC()
{
    DC = 0;
    dc_region_quit();
    if (dc_caller)
        GRX->Deselect(dc_caller);
    if (dc_popup)
        gtk_widget_destroy(dc_popup);
}


void
sDC::update()
{
    const char *s = CDvdb()->getVariable(VA_DrcChdName);
    const char *ss = gtk_entry_get_text(GTK_ENTRY(DC->dc_chdname));
    if (!s)
        s = "";
    if (!ss)
        ss = "";
    if (strcmp(s, ss))
        gtk_entry_set_text(GTK_ENTRY(DC->dc_chdname), s);

    s = CDvdb()->getVariable(VA_DrcChdCell);
    ss = gtk_entry_get_text(GTK_ENTRY(DC->dc_cname));
    if (!s)
        s = "";
    if (!ss)
        ss = "";
    if (strcmp(s, ss))
        gtk_entry_set_text(GTK_ENTRY(DC->dc_cname), s);

    s = CDvdb()->getVariable(VA_DrcPartitionSize);
    if (s) {
        double d = atof(s);
        if (d >= DRC_PART_MIN && d <= DRC_PART_MAX) {
            GRX->SetStatus(dc_none, false);
            sb_part.set_sensitive(true);
            sb_part.set_value(d);
        }
        else {
            // Huh?  Bad value, clear it.
            CDvdb()->clearVariable(VA_DrcPartitionSize);
        }
    }
    else {
        GRX->SetStatus(dc_none, true);
        sb_part.set_sensitive(false, true);
    }

    GRX->SetStatus(dc_use, dc_use_chd);

    sb_left.set_value(dc_l_val);
    sb_bottom.set_value(dc_b_val);
    sb_right.set_value(dc_r_val);
    sb_top.set_value(dc_t_val);

    GRX->SetStatus(dc_wind, dc_use_win);
    gtk_widget_set_sensitive(dc_l_label, dc_use_win);
    DC->sb_left.set_sensitive(dc_use_win);
    gtk_widget_set_sensitive(dc_b_label, dc_use_win);
    DC->sb_bottom.set_sensitive(dc_use_win);
    gtk_widget_set_sensitive(dc_r_label, dc_use_win);
    DC->sb_right.set_sensitive(dc_use_win);
    gtk_widget_set_sensitive(dc_t_label, dc_use_win);
    DC->sb_top.set_sensitive(dc_use_win);

    GRX->SetStatus(dc_flat, dc_flatten);
    gtk_widget_set_sensitive(dc_flat, dc_use_chd);

    update_jobs_list();
}


void
sDC::update_jobs_list()
{
    if (!dc_jobs)
        return;
    GdkColor *c1 = gtk_PopupColor(GRattrColorHl4);

    int pid = get_pid();
    if (pid <= 0)
        gtk_widget_set_sensitive(dc_kill, false);

    double val = text_get_scroll_value(dc_jobs);
    text_set_chars(dc_jobs, "");

    char *list = DRC()->jobs();
    if (!list)
        text_insert_chars_at_point(dc_jobs, c1,
            "No background jobs running.", -1, -1);
    else
        text_insert_chars_at_point(dc_jobs, 0, list, -1, -1);
    text_set_scroll_value(dc_jobs, val);
    if (pid > 0)
        select_pid(pid);
    gtk_widget_set_sensitive(dc_kill, get_pid() > 0);
}


// Select the chars in the range, start=end deselects existing.  In
// GTK-1, selecting gives blue inverse, which turns gray if
// unselected, retaining an indication for the buttons.  GTK-2
// doesn't do this automatically so we provide something similar here.
//
void
sDC::select_range(int start, int end)
{
    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(dc_jobs));
    GtkTextIter istart, iend;
    if (dc_end != dc_start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, dc_start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, dc_end);
        gtk_text_buffer_remove_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    text_select_range(dc_jobs, start, end);
    if (end != start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, end);
        gtk_text_buffer_apply_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    dc_start = start;
    dc_end = end;
}


// Return the PID value of the selected line, or -1.
//
int
sDC::get_pid()
{
    if (dc_line_selected < 0)
        return (-1);
    char *string = text_get_chars(dc_jobs, 0, -1);
    int line = 0;
    for (const char *s = string; *s; s++) {
        if (line == dc_line_selected) {
            while (isspace(*s))
                s++;
            int pid;
            int r = sscanf(s, "%d", &pid);
            delete [] string;
            return (r == 1 ? pid : -1);
        }
        if (*s == '\n')
            line++;
    }
    delete [] string;
    return (-1);
}


void
sDC::select_pid(int p)
{
    char *string = text_get_chars(dc_jobs, 0, -1);
    bool nl = true;
    int line = 0;
    const char *cs = 0;
    for (const char *s = string; *s; s++) {
        if (nl) {
            cs = s;
            while (isspace(*s))
                s++;
            nl = false;
            int pid;
            int r = sscanf(s, "%d", &pid);
            if (r == 1 && p == pid) {
                const char *ce = cs;
                while (*ce && *ce != 'n')
                    ce++;
                select_range(cs - string, ce - string);
                delete [] string;
                dc_line_selected = line;
                gtk_widget_set_sensitive(dc_kill, true);
                return;
            }
        }
        if (*s == '\n') {
            nl = true;
            line++;
        }
    }
    delete [] string;
}


// Static functions.
void
sDC::dc_cancel_proc(GtkWidget*, void*)
{
    DRC()->PopUpDrcRun(0, MODE_OFF);
}


namespace {
    cCHD *find_chd(const char *name)
    {
        if (!name || !*name)
            return (0);
        cCHD *chd = CDchd()->chdRecall(name, false);
        if (chd)
            return (chd);

        sCHDin chd_in;
        if (!chd_in.check(name))
            return (0);
        chd = chd_in.read(name, sCHDin::get_default_cgd_type());
        return (chd);
    }
}


// Static functions.
void
sDC::dc_action_proc(GtkWidget *caller, void*)
{
    if (!DC)
        return;

    bool bg = false;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help"))
        DSPmainWbag(PopUpHelp("xic:check"))
    else if (!strcmp(name, "use")) {
        dc_use_chd = GRX->GetStatus(caller);
        gtk_widget_set_sensitive(DC->dc_flat, dc_use_chd);
    }
    else if (!strcmp(name, "none")) {
        if (GRX->GetStatus(caller)) {
            DC->dc_last_part_size = DC->sb_part.get_value();
            CDvdb()->clearVariable(VA_DrcPartitionSize);
        }
        else {
            char buf[32];
            sprintf(buf, "%.2f", DC->dc_last_part_size);
            CDvdb()->setVariable(VA_DrcPartitionSize, buf);
        }
    }
    else if (!strcmp(name, "wind")) {
        dc_use_win = GRX->GetStatus(caller);
        gtk_widget_set_sensitive(DC->dc_l_label, dc_use_win);
        DC->sb_left.set_sensitive(dc_use_win);
        gtk_widget_set_sensitive(DC->dc_b_label, dc_use_win);
        DC->sb_bottom.set_sensitive(dc_use_win);
        gtk_widget_set_sensitive(DC->dc_r_label, dc_use_win);
        DC->sb_right.set_sensitive(dc_use_win);
        gtk_widget_set_sensitive(DC->dc_t_label, dc_use_win);
        DC->sb_top.set_sensitive(dc_use_win);
    }
    else if (!strcmp(name, "flat")) {
        dc_flatten = GRX->GetStatus(caller);
    }
    else if (!strcmp(name, "set")) {
        DC->dc_region();
    }
    else if (!strcmp(name, "check") || (bg = !strcmp(name, "checkbg"))) {
        if (!GRX->GetStatus(caller)) {
            if (!bg) {
                DSP()->SetInterrupt(DSPinterUser);
                if (!XM()->ConfirmAbort())
                    GRX->SetStatus(caller, true);
                else
                    DRC()->setAbort(true);
                DSP()->SetInterrupt(DSPinterNone);
            }
            return;
        }
        if (DC->dc_use_chd) {
            const char *cname = gtk_entry_get_text(GTK_ENTRY(DC->dc_chdname));
            cCHD *chd = find_chd(cname);
            if (!chd) {
                Log()->ErrorLog("DRC", "DRC aborted, CHD not found.");
                GRX->Deselect(caller);
                return;
            }
            const char *cellname = 0;
            const char *cn = gtk_entry_get_text(GTK_ENTRY(DC->dc_cname));
            if (cn)
                cellname = lstring::gettok(&cn);
            if (DC->dc_use_win) {
                BBox BB(INTERNAL_UNITS(DC->sb_left.get_value()),
                    INTERNAL_UNITS(DC->sb_bottom.get_value()),
                    INTERNAL_UNITS(DC->sb_right.get_value()),
                    INTERNAL_UNITS(DC->sb_top.get_value()));
                DRC()->runDRC(&BB, bg, chd, cellname, DC->dc_flatten);
            }
            else
                DRC()->runDRC(0, bg, chd, cellname, DC->dc_flatten);
            delete [] cellname;
        }
        else {
            CDs *cursdp = CurCell(Physical);
            if (!cursdp) {
                GRX->Deselect(caller);
                return;
            }
            if (DC->dc_use_win) {
                BBox BB(INTERNAL_UNITS(DC->sb_left.get_value()),
                    INTERNAL_UNITS(DC->sb_bottom.get_value()),
                    INTERNAL_UNITS(DC->sb_right.get_value()),
                    INTERNAL_UNITS(DC->sb_top.get_value()));
                DRC()->runDRC(&BB, bg);
            }
            else
                DRC()->runDRC(cursdp->BB(), bg);
        }

        if (DC)
            GRX->Deselect(caller);
        DRC()->setAbort(false);
    }
    else if (!strcmp(name, "abort")) {
        int pid = DC->get_pid();
        if (pid > 0) {
#ifdef WIN32
            HANDLE h = OpenProcess(PROCESS_TERMINATE, 0, pid);
            TerminateProcess(h, 1);
#else
            kill(pid, SIGTERM);
#endif
            // Handler will clean up and update.
        }
    }
}


void
sDC::dc_text_changed(GtkWidget *caller, void*)
{
    if (!DC)
        return;
    if (caller == DC->dc_chdname) {
        const char *s = gtk_entry_get_text(GTK_ENTRY(caller));
        const char *ss = CDvdb()->getVariable(VA_DrcChdName);
        if (s && *s) {
            if (!ss || strcmp(ss, s))
                CDvdb()->setVariable(VA_DrcChdName, s);
        }
        else
            CDvdb()->clearVariable(VA_DrcChdName);
    }
    if (caller == DC->dc_cname) {
        const char *s = gtk_entry_get_text(GTK_ENTRY(caller));
        const char *ss = CDvdb()->getVariable(VA_DrcChdCell);
        if (s && *s) {
            if (!ss || strcmp(ss, s))
                CDvdb()->setVariable(VA_DrcChdCell, s);
        }
        else
            CDvdb()->clearVariable(VA_DrcChdCell);
    }
}


// Static functions.
void
sDC::dc_val_changed(GtkWidget *caller, void*)
{
    if (!DC)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "partsize")) {
        const char *s = DC->sb_part.get_string();
        double n;
        if (sscanf(s, "%lf", &n) == 1 && n >= DRC_PART_MIN &&
                n <= DRC_PART_MAX) {
            CDvdb()->setVariable(VA_DrcPartitionSize, s);
        }
    }
    else if (!strcmp(name, "left")) {
        const char *s = DC->sb_left.get_string();
        char *endp;
        double d = strtod(s, &endp);
        if (endp != s)
            dc_l_val = d;
    }
    else if (!strcmp(name, "bottom")) {
        const char *s = DC->sb_bottom.get_string();
        char *endp;
        double d = strtod(s, &endp);
        if (endp != s)
            dc_b_val = d;
    }
    else if (!strcmp(name, "right")) {
        const char *s = DC->sb_right.get_string();
        char *endp;
        double d = strtod(s, &endp);
        if (endp != s)
            dc_r_val = d;
    }
    else if (!strcmp(name, "top")) {
        const char *s = DC->sb_top.get_string();
        char *endp;
        double d = strtod(s, &endp);
        if (endp != s)
            dc_t_val = d;
    }
}


// Static function.
int
sDC::dc_button_dn(GtkWidget *caller, GdkEvent *event, void*)
{
    if (!DC)
        return (true);
    if (event->type != GDK_BUTTON_PRESS)
        return (true);

    char *string = text_get_chars(caller, 0, -1);
    int x = (int)event->button.x;
    int y = (int)event->button.y;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(caller), &ihere, x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    char *line_start = string + gtk_text_iter_get_offset(&iline);
    x = gtk_text_iter_get_offset(&ihere) - gtk_text_iter_get_offset(&iline);
    y = gtk_text_iter_get_line(&iline);

    // line_start points to start of selected line, or 0
    if (line_start && *line_start != '\n') {

        int start = line_start - string;
        int end = start;
        while (string[end] && string[end] != '\n')
            end++;

        DC->dc_line_selected = y;
        DC->select_range(start, end);
        delete [] string;
        gtk_widget_set_sensitive(DC->dc_kill, DC->get_pid() > 0);
        return (true);
    }
    DC->dc_line_selected = -1;
    delete [] string;
    DC->select_range(0, 0);
    gtk_widget_set_sensitive(DC->dc_kill, false);
    return (true);
}


// Static function.
void
sDC::dc_font_changed()
{
    if (DC)
        DC->update_jobs_list();
}


// Menu command for performing design rule checking on the current
// cell and its hierarchy.  The errors are printed on-screen, no
// file is generated.  The command will abort after DRC_PTMAX errors.
// The Enter key is ignored.  The command remains in effect until
// escaped.
//
void
sDC::dc_region()
{
    if (WinCmd) {
        WinCmd->esc();
        return;
    }

    WinCmd = new WinState("window", "xic:check#set");
    if (!EV()->PushCallback(WinCmd)) {
        delete WinCmd;
        return;
    }
    WinCmd->message();
}


void
sDC::dc_region_quit()
{
    if (WinCmd)
        WinCmd->esc();
}


WinState::WinState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    WinCmd = this;
    state = 0;
    ghost_on = false;
}


WinState::~WinState()
{
    WinCmd = 0;
}


void
WinState::b1down()
{
    int x, y;
    EV()->Cursor().get_xy(&x, &y);
    if (state <= 0) {
        AOI.left = x;
        AOI.bottom = y;
        AOI.right = x;
        AOI.top = y;

        XM()->SetCoordMode(CO_RELATIVE, AOI.left, AOI.bottom);
        Gst()->SetGhost(GFbox);
        ghost_on = true;
        state = 1;
    }
    else {
        Gst()->SetGhost(GFnone);
        ghost_on = false;
        XM()->SetCoordMode(CO_ABSOLUTE);
        AOI.right = x;
        AOI.top = y;
        AOI.fix();
        if (DC) {
            DC->sb_left.set_value(MICRONS(AOI.left));
            DC->sb_bottom.set_value(MICRONS(AOI.bottom));
            DC->sb_right.set_value(MICRONS(AOI.right));
            DC->sb_top.set_value(MICRONS(AOI.top));
        }
        state = 0;
    }
}


// Drc the box, if the pointer has moved, and the box has area.
// Otherwise set the state for next button 1 press.
//
void
WinState::b1up()
{
    if (state < 0) {
        state = 0;
        return;
    }
    if (state == 1) {
        if (!EV()->Cursor().is_release_ok()) {
            state = 2;
            return;
        }
        // dragged...
        int x, y;
        WindowDesc *wdesc = EV()->ButtonWin();
        if (!wdesc)
            wdesc = DSP()->MainWdesc();
        EV()->Cursor().get_release(&x, &y);
        wdesc->Snap(&x, &y);
        AOI.right = x;
        AOI.top = y;
        AOI.fix();
        int delta = (int)(2.0/wdesc->Ratio());
        if (delta >= AOI.width() && delta >= AOI.height()) {
            // ...but pointer didn't move enough
            state = 2;
            return;
        }
    }

    Gst()->SetGhost(GFnone);
    ghost_on = false;
    XM()->SetCoordMode(CO_ABSOLUTE);
    state = 0;

    if (DC) {
        DC->sb_left.set_value(MICRONS(AOI.left));
        DC->sb_bottom.set_value(MICRONS(AOI.bottom));
        DC->sb_right.set_value(MICRONS(AOI.right));
        DC->sb_top.set_value(MICRONS(AOI.top));
    }
}


// Esc entered, clean up and abort.
//
void
WinState::esc()
{
    if (ghost_on)
        Gst()->SetGhost(GFnone);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    XM()->SetCoordMode(CO_ABSOLUTE);
    if (DC)
        GRX->Deselect(DC->dc_set);
    delete this;
}

