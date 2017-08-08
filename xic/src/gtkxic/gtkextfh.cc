
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
#include "ext.h"
#include "ext_fh.h"
#include "ext_fxunits.h"
#include "ext_fxjob.h"
#include "dsp_inlines.h"
#include "dsp_color.h"
#include "menu.h"
#include "select.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkutil.h"
#include "gtkinterf/gtkspinbtn.h"


//-----------------------------------------------------------------------------
// Pop-up to control FastHenry interface.
//
// Help system keywords used:
//  fhpanel

namespace {
    namespace gtkextfh {
        struct sFh : public gtk_bag
        {
            sFh(GRobject);
            ~sFh();

            void update();
            void update_jobs_list();
            void update_label(const char*);

        private:
            void set_sens(bool);
            void update_fh_freq_widgets();
            void update_fh_freq();
            void select_range(int, int);
            int get_pid();
            void select_pid(int);

            static const char *fh_def_string(int);
            static int fh_option_history(GtkWidget*);
            static void fh_cancel_proc(GtkWidget*, void*);
            static void fh_help_proc(GtkWidget*, void*);
            static void fh_change_proc(GtkWidget*, void*);
            static void fh_units_proc(GtkWidget*, void*);
            static void fh_p_cb(bool, void*);
            static void fh_dump_cb(const char*, void*);
            static void fh_btn_proc(GtkWidget*, void*);
            static int fh_button_dn(GtkWidget*, GdkEvent*, void*);
            static void fh_font_changed();

            GRobject fh_caller;
            GtkWidget *fh_label;

            GtkWidget *fh_units;
            GtkWidget *fh_enab;

            GtkWidget *fh_foreg;
            GtkWidget *fh_out;
            GtkWidget *fh_file;
            GtkWidget *fh_args;
            GtkWidget *fh_fmin;
            GtkWidget *fh_fmax;
            GtkWidget *fh_ndec;
            GtkWidget *fh_path;

            GtkWidget *fh_jobs;
            GtkWidget *fh_kill;

            bool fh_no_reset;
            int fh_start;
            int fh_end;
            int fh_line_selected;

            GTKspinBtn sb_fh_min_rect;
            GTKspinBtn sb_fh_volel_target;
        };

        sFh *Fh;

        enum { FhRun, FhRunFile, FhDump, Foreg, ToCons, Enable, Kill };
        enum { MinRect, VolElTarg, FhPath, FhArgs, FhFreq };
    }

    // FastHenry units menu, must have same order and length as Units[]
    // in extif.c.
    //
    const char *fh_units_strings[] =
    {
        "meters",
        "centimeters",
        "millimeters",
        "microns",
        "inches",
        "mils",
        0
    };
}

using namespace gtkextfh;


// Pop up a panel to control the fastcap/fasthenry interface.
//
void
cFH::PopUpExtIf(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Fh;
        return;
    }
    if (mode == MODE_UPD) {
        if (Fh)
            Fh->update();
        return;
    }
    if (Fh)
        return;

    new sFh(caller);
    if (!Fh->Shell()) {
        delete Fh;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Fh->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_LR), Fh->Shell(), mainBag()->Viewport());
    gtk_widget_show(Fh->Shell());
    setPopUpVisible(true);
}


void
cFH::updateString()
{
    if (Fh) {
        char *s = statusString();
        Fh->update_label(s);
        delete [] s;
    }
}
// End of cFH functions.


sFh::sFh(GRobject c)
{
    Fh = this;
    fh_caller = c;
    fh_label = 0;
    fh_units = 0;
    fh_enab = 0;
    fh_foreg = 0;
    fh_out = 0;
    fh_file = 0;
    fh_args = 0;
    fh_fmin = 0;
    fh_fmax = 0;
    fh_ndec = 0;
    fh_path = 0;
    fh_jobs = 0;
    fh_kill = 0;
    fh_no_reset = false;
    fh_start = 0;
    fh_end = 0;
    fh_line_selected = -1;

    wb_shell = gtk_NewPopup(0, "LR Extraction", fh_cancel_proc, 0);
    if (!wb_shell)
        return;

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    int rowcnt = 0;

    //
    // Label in frame plus help btn
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    GtkWidget *label = gtk_label_new("FastHenry Interface");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(hbox), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fh_help_proc), 0);
    gtk_box_pack_end(GTK_BOX(hbox), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *nbook = gtk_notebook_new();
    gtk_widget_show(nbook);

    gtk_table_attach(GTK_TABLE(form), nbook, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Run page
    //
    GtkWidget *table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);
    int row = 0;

    hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    button = gtk_check_button_new_with_label("Run in foreground");
    gtk_widget_set_name(button, "FhForeg");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fh_btn_proc), (void*)Foreg);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
    fh_foreg = button;

    button = gtk_check_button_new_with_label("Out to console");
    gtk_widget_set_name(button, "FhCons");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fh_btn_proc), (void*)ToCons);
    gtk_box_pack_end(GTK_BOX(hbox), button, false, false, 0);
    fh_out = button;

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    GtkWidget *hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);

    gtk_table_attach(GTK_TABLE(table), hsep, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    row++;

    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    button = gtk_button_new_with_label("Run File");
    gtk_widget_set_name(button, "FhRunFile");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fh_btn_proc), (void*)FhRunFile);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    GtkWidget *entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 0);
    fh_file = entry;

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    button = gtk_button_new_with_label("Run FastHenry");
    gtk_widget_set_name(button, "FhRun");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fh_btn_proc), (void*)FhRun);

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label("Dump FastHenry File");
    gtk_widget_set_name(button, "FhDump");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fh_btn_proc), (void*)FhDump);

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    frame = gtk_frame_new("FhArgs");
    gtk_widget_show(frame);
    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_container_add(GTK_CONTAINER(frame), entry);
    gtk_signal_connect(GTK_OBJECT(entry), "changed",
        GTK_SIGNAL_FUNC(fh_change_proc), (void*)FhArgs);
    fh_args = entry;

    gtk_table_attach(GTK_TABLE(table), frame, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    frame = gtk_frame_new("FhFreq");
    gtk_widget_show(frame);
    hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    label = gtk_label_new(" fmin=");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);
    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_widget_set_usize(entry, 50, -1);
    gtk_signal_connect(GTK_OBJECT(entry), "changed",
        GTK_SIGNAL_FUNC(fh_change_proc), (void*)FhFreq);
    gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 0);
    fh_fmin = entry;

    label = gtk_label_new(" fmax=");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);
    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_widget_set_usize(entry, 50, -1);
    gtk_signal_connect(GTK_OBJECT(entry), "changed",
        GTK_SIGNAL_FUNC(fh_change_proc), (void*)FhFreq);
    gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 0);
    fh_fmax = entry;

    label = gtk_label_new(" ndec=");
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);
    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_widget_set_usize(entry, 30, -1);
    gtk_signal_connect(GTK_OBJECT(entry), "changed",
        GTK_SIGNAL_FUNC(fh_change_proc), (void*)FhFreq);
    gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 0);
    fh_ndec = entry;

    gtk_table_attach(GTK_TABLE(table), frame, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    frame = gtk_frame_new("Path to FastHenry");
    gtk_widget_show(frame);
    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_container_add(GTK_CONTAINER(frame), entry);
    gtk_signal_connect(GTK_OBJECT(entry), "changed",
        GTK_SIGNAL_FUNC(fh_change_proc), (void*)FhPath);
    fh_path = entry;

    gtk_table_attach(GTK_TABLE(table), frame, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    label = gtk_label_new("Run");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), table, label);

    //
    // Params page
    //
    table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);
    row = 0;

    frame = gtk_frame_new("FhUnits");
    gtk_widget_show(frame);
    entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "FhUnits");
    gtk_widget_show(entry);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "FhUnits");
    for (int i = 0; fh_units_strings[i]; i++) {
        GtkWidget *mi = gtk_menu_item_new_with_label(fh_units_strings[i]);
        gtk_widget_set_name(mi, fh_units_strings[i]);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(fh_units_proc),
                (void*)fh_units_strings[i]);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    gtk_container_add(GTK_CONTAINER(frame), entry);
    fh_units = entry;

    gtk_table_attach(GTK_TABLE(table), frame, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    frame = gtk_frame_new("FhMinRectSize");
    gtk_widget_show(frame);

    int ndgt = CD()->numDigits();
    double val = FH_MIN_RECT_SIZE_DEF;
    GtkWidget *sb = sb_fh_min_rect.init(val, FH_MIN_RECT_SIZE_MIN,
        FH_MIN_RECT_SIZE_MAX, ndgt);
    gtk_widget_set_usize(sb, 100, -1);
    sb_fh_min_rect.connect_changed(GTK_SIGNAL_FUNC(fh_change_proc),
        (void*)MinRect, "FhMinRectSize");
    gtk_container_add(GTK_CONTAINER(frame), sb);

    gtk_table_attach(GTK_TABLE(table), frame, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);

    gtk_table_attach(GTK_TABLE(table), hsep, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    row++;

    label = gtk_label_new("FastHenry Volume Element Refinement");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    gtk_table_attach(GTK_TABLE(table), frame, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    button = gtk_check_button_new_with_label("Enable");
    gtk_widget_set_name(button, "Enable");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fh_btn_proc), (void*)Enable);
    fh_enab = button;

    gtk_table_attach(GTK_TABLE(table), button, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    sb = sb_fh_volel_target.init(FH_DEF_TARG_VOLEL, FH_MIN_TARG_VOLEL,
        FH_MAX_TARG_VOLEL, 0);
    gtk_widget_set_usize(sb, 100, -1);
    sb_fh_volel_target.connect_changed(GTK_SIGNAL_FUNC(fh_change_proc),
        (void*)VolElTarg, "FhVolElTarget");
    frame = gtk_frame_new("FhVolElTarget");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), sb);

    gtk_table_attach(GTK_TABLE(table), frame, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    label = gtk_label_new("Params");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), table, label);

    //
    // Jobs page
    //
    table = gtk_table_new(2, 1, false);
    gtk_widget_show(table);
    row = 0;

    GtkWidget *contr;
    text_scrollable_new(&contr, &fh_jobs, FNT_FIXED);

    gtk_widget_add_events(fh_jobs, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(fh_jobs), "button-press-event",
        GTK_SIGNAL_FUNC(fh_button_dn), 0);

    // The font change pop-up uses this to redraw the widget
    gtk_object_set_data(GTK_OBJECT(fh_jobs), "font_changed",
        (void*)fh_font_changed);

    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(fh_jobs));
    const char *bclr = GRpkgIf()->GetAttrColor(GRattrColorLocSel);
    gtk_text_buffer_create_tag(textbuf, "primary", "background", bclr,
#if GTK_CHECK_VERSION(2,8,0)
        "paragraph-background", bclr,
#endif
        NULL);

    gtk_widget_set_usize(fh_jobs, 200, 200);

    gtk_table_attach(GTK_TABLE(table), contr, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    button = gtk_button_new_with_label("Abort job");
    gtk_widget_set_name(button, "Abort");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fh_btn_proc), (void*)Kill);
    fh_kill = button;

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    label = gtk_label_new("Jobs");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(nbook), table, label);

    //
    // Status line and Dismiss button
    //
    const char *s = FH()->statusString();
    fh_label = gtk_label_new(s);
    delete [] s;
    gtk_widget_show(fh_label);
    gtk_misc_set_alignment(GTK_MISC(fh_label), 0.5, 0.5);
    gtk_misc_set_padding(GTK_MISC(fh_label), 2, 2);
    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), fh_label);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fh_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    update();
}


sFh::~sFh()
{
    Fh = 0;
    if (fh_caller)
        GRX->Deselect(fh_caller);
    FH()->setPopUpVisible(false);
    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(sFh::fh_cancel_proc), wb_shell);
}


void
sFh::update()
{
    const char *var, *cur;
    fh_no_reset = true;

    // Run page
    GRX->SetStatus(fh_foreg, CDvdb()->getVariable(VA_FhForeg));
    GRX->SetStatus(fh_out, CDvdb()->getVariable(VA_FhMonitor));

    var = CDvdb()->getVariable(VA_FhArgs);
    if (!var)
        var = fh_def_string(FhArgs);
    cur = gtk_entry_get_text(GTK_ENTRY(fh_args));
    if (!cur)
        cur = "";
    if (strcmp(var, cur))
        gtk_entry_set_text(GTK_ENTRY(fh_args), var);

    var = CDvdb()->getVariable(VA_FhPath);
    if (!var)
        var = fh_def_string(FhPath);
    cur = gtk_entry_get_text(GTK_ENTRY(fh_path));
    if (!cur)
        cur = "";
    if (strcmp(var, cur))
        gtk_entry_set_text(GTK_ENTRY(fh_path), var);

    update_fh_freq_widgets();

    // Params page
    var = CDvdb()->getVariable(VA_FhUnits);
    if (!var)
        var = "";
    int uoff = FH()->getUnitsIndex(var);
    int ucur = fh_option_history(fh_units);
    if (uoff != ucur)
        gtk_option_menu_set_history(GTK_OPTION_MENU(fh_units), uoff);

    var = CDvdb()->getVariable(VA_FhMinRectSize);
    if (sb_fh_min_rect.is_valid(var))
        sb_fh_min_rect.set_value(atof(var));
    else {
        if (var)
            CDvdb()->clearVariable(VA_FhMinRectSize);
        sb_fh_min_rect.set_value(FH_MIN_RECT_SIZE_DEF);
    }

    static double fhvt_bak;
    var = CDvdb()->getVariable(VA_FhVolElTarget);
    if (sb_fh_volel_target.is_valid(var)) {
        sb_fh_volel_target.set_value(atof(var));
        sb_fh_volel_target.set_sensitive(true);
        GRX->SetStatus(fh_enab, true);
        fhvt_bak = sb_fh_volel_target.get_value();
    }
    else {
        if (var)
            CDvdb()->clearVariable(VA_FhVolElTarget);
        if (fhvt_bak > 0.0)
            sb_fh_volel_target.set_value(fhvt_bak);
        sb_fh_volel_target.set_sensitive(false);
        GRX->SetStatus(fh_enab, false);
    }

    // Jobs page
    update_jobs_list();

    fh_no_reset = false;
}


void
sFh::update_jobs_list()
{
    if (!fh_jobs)
        return;
    GdkColor *c1 = gtk_PopupColor(GRattrColorHl4);

    int pid = get_pid();
    if (pid <= 0)
        gtk_widget_set_sensitive(fh_kill, false);

    double val = text_get_scroll_value(fh_jobs);
    text_set_chars(fh_jobs, "");

    char *list = FH()->jobList();
    if (!list)
        text_insert_chars_at_point(fh_jobs, c1,
            "No background jobs running.", -1, -1);

    else
        text_insert_chars_at_point(fh_jobs, 0, list, -1, -1);
    text_set_scroll_value(fh_jobs, val);
    if (pid > 0)
        select_pid(pid);
    gtk_widget_set_sensitive(fh_kill, get_pid() > 0);
}


void
sFh::update_label(const char *s)
{
    gtk_label_set(GTK_LABEL(fh_label), s);
}


namespace {
    char *getword(const char *kw, const char *str)
    {
        const char *s = strstr(str, kw);
        if (!s)
            return (lstring::copy(""));
        s += strlen(kw);
        const char *t = s;
        while (*t && !isspace(*t))
            t++;
        char *r = new char[t-s + 1];
        strncpy(r, s, t-s);
        r[t-s] = 0;
        return (r);
    }
}


// Update frequency entries from variable.
//
void
sFh::update_fh_freq_widgets()
{
    if (Fh) {
        const char *str = CDvdb()->getVariable(VA_FhFreq);
        char *smin = str ? getword("fmin=", str) :
            lstring::copy(fh_def_string(FhFreq));
        char *smax = str ? getword("fmax=", str) :
            lstring::copy(fh_def_string(FhFreq));
        char *sdec = str ? getword("ndec=", str) : lstring::copy("");
        gtk_entry_set_text(GTK_ENTRY(fh_fmin), smin);
        gtk_entry_set_text(GTK_ENTRY(fh_fmax), smax);
        gtk_entry_set_text(GTK_ENTRY(fh_ndec), sdec);
        delete [] smin;
        delete [] smax;
        delete [] sdec;
    }
}


// Update variable from frequency entries.
//
void
sFh::update_fh_freq()
{
    if (Fh) {
        char buf[128];
        const char *smin = gtk_entry_get_text(GTK_ENTRY(fh_fmin));
        const char *smax = gtk_entry_get_text(GTK_ENTRY(fh_fmax));
        const char *sdec = gtk_entry_get_text(GTK_ENTRY(fh_ndec));
        smin = lstring::gettok(&smin);
        if (!smin)
            smin = lstring::copy("");
        smax = lstring::gettok(&smax);
        if (!smax)
            smax = lstring::copy("");
        sdec = lstring::gettok(&sdec);
        if (sdec)
            sprintf(buf, "fmin=%s fmax=%s ndec=%s", smin, smax, sdec);
        else {
            if (!strcmp(smin, fh_def_string(FhFreq)) &&
                    !strcmp(smax, fh_def_string(FhFreq))) {
                CDvdb()->clearVariable(VA_FhFreq);
                return;
            }
            sprintf(buf, "fmin=%s fmax=%s", smin, smax);
        }
        CDvdb()->setVariable(VA_FhFreq, buf);
    }
}


// Select the chars in the range, start=end deselects existing.  In
// GTK-1, selecting gives blue inverse, which turns gray if
// unselected, retaining an indication for the buttons.  GTK-2
// doesn't do this automatically so we provide something similar here.
//
void
sFh::select_range(int start, int end)
{
    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(fh_jobs));
    GtkTextIter istart, iend;
    if (fh_end != fh_start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, fh_start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, fh_end);
        gtk_text_buffer_remove_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    text_select_range(fh_jobs, start, end);
    if (end != start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, end);
        gtk_text_buffer_apply_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    fh_start = start;
    fh_end = end;
}


// Return the PID value of the selected line, or -1.
//
int
sFh::get_pid()
{
    if (fh_line_selected < 0)
        return (-1);
    char *string = text_get_chars(fh_jobs, 0, -1);
    int line = 0;
    for (const char *s = string; *s; s++) {
        if (line == fh_line_selected) {
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
sFh::select_pid(int p)
{
    char *string = text_get_chars(fh_jobs, 0, -1);
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
                fh_line_selected = line;
                gtk_widget_set_sensitive(fh_kill, true);
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


// Static function.
// Return the default text field text.
//
const char *
sFh::fh_def_string(int id)
{
    int ndgt = CD()->numDigits();
    static char tbuf[16];
    switch (id) {
    case MinRect:
        sprintf(tbuf, "%.*f", ndgt, FH_MIN_RECT_SIZE_DEF);
        return (tbuf);
    case VolElTarg:
        sprintf(tbuf, "%.*f", ndgt, FH_DEF_TARG_VOLEL);
        return (tbuf);
    case FhPath:
        return (FH_DEFAULT_PATH);
    case FhArgs:
        return ("");
    case FhFreq:
        return ("1e3");
    }
    return ("");
}


// Static function.
// Return the current index value of the option menu.  Too bad GTK+
// doesn't provide this.
//
int
sFh::fh_option_history(GtkWidget *opt)
{
    return (gtk_option_menu_get_history(GTK_OPTION_MENU(opt)));
}


// Static function.
void
sFh::fh_cancel_proc(GtkWidget*, void*)
{
    FH()->PopUpExtIf(0, MODE_OFF);
}


// Static function.
void
sFh::fh_help_proc(GtkWidget*, void*)
{
    DSPmainWbag(PopUpHelp("fhpanel"))
}


namespace {
    bool
    check_num(const char *s, double minv, double maxv)
    {
        if (!s || !*s)
            return (true);
        double d;
        if (sscanf(s, "%lf", &d) != 1)
            return (true);
        if (d < minv || d > maxv)
            return (true);
        return (false);
    }
}


// Static function.
void
sFh::fh_change_proc(GtkWidget *widget, void *arg)
{
    if (!Fh || Fh->fh_no_reset)
        return;
    const char *s = gtk_entry_get_text(GTK_ENTRY(widget));
    if (!s)
        return;
    int id = (long)arg;
    switch (id) {
    case MinRect:
        if (check_num(s, FH_MIN_RECT_SIZE_MIN, FH_MIN_RECT_SIZE_MAX))
            break;
        if (!strcmp(s, fh_def_string(id)))
            CDvdb()->clearVariable(VA_FhMinRectSize);
        else
            CDvdb()->setVariable(VA_FhMinRectSize, s);
        break;
    case VolElTarg:
        if (check_num(s, FH_MIN_TARG_VOLEL, FH_MAX_TARG_VOLEL))
            break;
        if (!strcmp(s, fh_def_string(id)))
            CDvdb()->clearVariable(VA_FhVolElTarget);
        else
            CDvdb()->setVariable(VA_FhVolElTarget, s);
        break;
    case FhPath:
        if (!strcmp(s, fh_def_string(id)))
            CDvdb()->clearVariable(VA_FhPath);
        else
            CDvdb()->setVariable(VA_FhPath, s);
        break;
    case FhArgs:
        if (!strcmp(s, fh_def_string(id)))
            CDvdb()->clearVariable(VA_FhArgs);
        else
            CDvdb()->setVariable(VA_FhArgs, s);
        break;
    case FhFreq:
        Fh->update_fh_freq();
        break;
    }
}


// Static function.
void
sFh::fh_units_proc(GtkWidget*, void *arg)
{
    const char *str = (const char*)arg;
    str = FH()->getUnitsString(str);
    if (str)
        CDvdb()->setVariable(VA_FhUnits, str);
    else
        CDvdb()->clearVariable(VA_FhUnits);
}


// Static function.
void
sFh::fh_p_cb(bool ok, void *arg)
{
    char *fname = (char*)arg;
    if (ok)
        DSPmainWbag(PopUpFileBrowser(fname))
    delete [] fname;
}


// Static function.
void
sFh::fh_dump_cb(const char *fname, void *client_data)
{
    switch ((long)client_data) {
    case FhDump:
        if (FH()->fhDump(fname)) {
            if (!Fh)
                return;
            const char *fn = lstring::strip_path(fname);
            char tbuf[256];
            sprintf(tbuf, "FastHenry input is in file %s.  View file? ", fn);
            Fh->PopUpAffirm(0, GRloc(LW_UL), tbuf, fh_p_cb,
                lstring::copy(fname));
        }
        break;
    }
    if (Fh && Fh->wb_input)
        Fh->wb_input->popdown();
}


// Static function.
void
sFh::fh_btn_proc(GtkWidget *widget, void *arg)
{
    if (!Fh)
        return;
    const char *s;
    switch ((long)arg) {
    case FhRun:
        FH()->fhRun(0, 0, 0);
        break;
    case FhRunFile:
        s = gtk_entry_get_text(GTK_ENTRY(Fh->fh_file));
        {
            char *tok = lstring::getqtok(&s);
            if (tok) {
                FH()->fhRun(tok, 0, 0, true);
                delete [] tok;
            }
            else {
                Fh->PopUpErr(MODE_ON, "No file name given!");
                return;
            }
        }
        break;
    case FhDump:
        s = FH()->getFileName(FH_INP_SFX);
        Fh->PopUpInput(0, s, "Dump", fh_dump_cb, (void*)FhDump);
        delete [] s;
        break;
    case Foreg:
        if (GRX->GetStatus(widget))
            CDvdb()->setVariable(VA_FhForeg, "");
        else
            CDvdb()->clearVariable(VA_FhForeg);
        break;
    case ToCons:
        if (GRX->GetStatus(widget))
            CDvdb()->setVariable(VA_FhMonitor, "");
        else
            CDvdb()->clearVariable(VA_FhMonitor);
        break;
    case Enable:
        if (GRX->GetStatus(widget)) {
            s = Fh->sb_fh_volel_target.get_string();
            if (!check_num(s, FH_MIN_TARG_VOLEL, FH_MAX_TARG_VOLEL))
                CDvdb()->setVariable(VA_FhVolElTarget, s);
            else {
                char tbf[32];
                sprintf(tbf, "%.1e", FH_DEF_TARG_VOLEL);
                CDvdb()->setVariable(VA_FhVolElTarget, tbf);
            }
            Fh->sb_fh_volel_target.set_sensitive(true);
        }
        else {
            CDvdb()->clearVariable(VA_FhVolElTarget);
            Fh->sb_fh_volel_target.set_sensitive(false);
        }
        break;
    case Kill:
        {
            int pid = Fh->get_pid();
            if (pid > 0)
                FH()->killProcess(pid);
        }
    }
}


// Static function.
int
sFh::fh_button_dn(GtkWidget *caller, GdkEvent *event, void*)
{
    if (!Fh)
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

        Fh->fh_line_selected = y;
        Fh->select_range(start, end);
        delete [] string;
        gtk_widget_set_sensitive(Fh->fh_kill, Fh->get_pid() > 0);
        return (true);
    }
    Fh->fh_line_selected = -1;
    delete [] string;
    Fh->select_range(0, 0);
    gtk_widget_set_sensitive(Fh->fh_kill, false);
    return (true);
}


// Static function.
void
sFh::fh_font_changed()
{
    if (Fh)
        Fh->update_jobs_list();
}

