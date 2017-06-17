
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: gtkchdopen.cc,v 5.38 2017/04/13 17:06:22 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "cvrt.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_chd.h"
#include "fio_alias.h"
#include "gtkmain.h"
#include "gtkcv.h"
#include "gtkfont.h"
#include "gtkinlines.h"
#include <gdk/gdkkeysyms.h>


//-----------------------------------------------------------------------------
// Pop up to obtain CHD creation info.
//
// Help system keywords used:
//  xic:chdadd

namespace {
    namespace gtkchdopen {
        struct sCo : public gtk_bag
        {
            sCo(GRobject, bool(*)(const char*, const char*, int, void*), void*,
                const char*, const char*);
            ~sCo();

            void update(const char*, const char*);

        private:
            void apply_proc(GtkWidget*);

            static void co_cancel_proc(GtkWidget*, void*);
            static void co_action(GtkWidget*, void*);

            static int co_key_hdlr(GtkWidget*, GdkEvent*, void*);
            static void co_info_proc(GtkWidget*, void*);
            static void co_drag_data_received(GtkWidget*, GdkDragContext*,
                gint, gint, GtkSelectionData*, guint, guint);
            static void co_page_proc(GtkNotebook*, GtkNotebookPage*, int,
                void*);

            GRobject co_caller;
            GtkWidget *co_nbook;
            GtkWidget *co_p1_text;
            GtkWidget *co_p1_info;
            GtkWidget *co_p2_text;
            GtkWidget *co_p2_mem;
            GtkWidget *co_p2_file;
            GtkWidget *co_p2_none;
            GtkWidget *co_idname;
            GtkWidget *co_apply;

            cnmap_t *co_p1_cnmap;

            bool(*co_callback)(const char*, const char*, int, void*);
            void *co_arg;
        };

        sCo *Co;
    }
}

using namespace gtkchdopen;


void
cConvert::PopUpChdOpen(GRobject caller, ShowMode mode,
    const char *init_idname, const char *init_str, int x, int y,
    bool(*callback)(const char*, const char*, int, void*), void *arg)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Co;
        return;
    }
    if (mode == MODE_UPD) {
        if (Co)
            Co->update(init_idname, init_str);
        return;
    }
    if (Co)
        return;

    new sCo(caller, callback, arg, init_idname, init_str);
    if (!Co->Shell()) {
        delete Co;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Co->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    int mwid;
    MonitorGeom(mainBag()->Shell(), 0, 0, &mwid, 0);
    if (x + Co->Shell()->requisition.width > mwid)
        x = mwid - Co->Shell()->requisition.width;
    gtk_widget_set_uposition(Co->Shell(), x, y);
    gtk_widget_show(Co->Shell());

    // OpenSuse-13.1 gtk-2.24.23 bug
    gtk_widget_set_uposition(Co->Shell(), x, y);
}

namespace {
    // Drag/drop stuff, also used in PopUpInput(), PopUpEditString()
    //
    GtkTargetEntry target_table[] = {
        { (char*)"TWOSTRING",   0, 0 },
        { (char*)"STRING",      0, 1 },
        { (char*)"text/plain",  0, 2 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);
}


sCo::sCo(GRobject caller, bool(*callback)(const char*, const char*, int, void*),
    void *arg, const char *init_idname, const char *init_str)
{
    Co = this;
    co_caller = caller;
    co_nbook = 0;
    co_p1_text = 0;
    co_p1_info = 0;
    co_p2_text = 0;
    co_p2_mem = 0;
    co_p2_file = 0;
    co_p2_none = 0;
    co_idname = 0;
    co_apply = 0;
    co_p1_cnmap = 0;
    co_callback = callback;
    co_arg = arg;

    wb_shell = gtk_NewPopup(0, "Open Cell Hierarchy Digest",
        co_cancel_proc, 0);
    if (!wb_shell)
        return;

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    int rowcnt = 0;

    // Label in frame plus help button.
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    GtkWidget *label = gtk_label_new(
        "Enter path to layout or saved digest file");
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
        GTK_SIGNAL_FUNC(co_action), 0);
    gtk_box_pack_end(GTK_BOX(hbox), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // This allows user to change label text.
    gtk_object_set_data(GTK_OBJECT(wb_shell), "label", label);

    co_nbook = gtk_notebook_new();
    gtk_widget_show(co_nbook);
    gtk_table_attach(GTK_TABLE(form), co_nbook, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Layout file page.
    //
    GtkWidget *tab_label = gtk_label_new("layout file");
    gtk_widget_show(tab_label);
    GtkWidget *tab_form = gtk_table_new(1, 1, false);
    gtk_widget_show(tab_form);
    int rcnt = 0;

    co_p1_text = gtk_entry_new();
    gtk_widget_show(co_p1_text);
    gtk_entry_set_editable(GTK_ENTRY(co_p1_text), true);
    gtk_table_attach(GTK_TABLE(tab_form), co_p1_text, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    rcnt++;

    // drop site
    GtkDestDefaults DD = (GtkDestDefaults)
        (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT);
    gtk_drag_dest_set(co_p1_text, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    gtk_signal_connect_after(GTK_OBJECT(co_p1_text), "drag-data-received",
        GTK_SIGNAL_FUNC(co_drag_data_received), 0);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(tab_form), sep, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    co_p1_cnmap = new cnmap_t(false);
    gtk_table_attach(GTK_TABLE(tab_form), co_p1_cnmap->frame(), 0, 2, rcnt,
        rcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(tab_form), sep, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    // Info options
    label = gtk_label_new("Geometry Counts");
    gtk_widget_show(label);

    gtk_table_attach(GTK_TABLE(tab_form), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    co_p1_info = gtk_option_menu_new();
    gtk_widget_show(co_p1_info);

    GtkWidget *menu = gtk_menu_new();
    gtk_widget_show(menu);
    GtkWidget *mi = gtk_menu_item_new_with_label("no geometry info saved");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(co_info_proc), (void*)cvINFOnone);
    mi = gtk_menu_item_new_with_label("totals only");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(co_info_proc), (void*)cvINFOtotals);
    mi = gtk_menu_item_new_with_label("per-layer counts");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(co_info_proc), (void*)cvINFOpl);
    mi = gtk_menu_item_new_with_label("per-cell counts");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(co_info_proc), (void*)cvINFOpc);
    mi = gtk_menu_item_new_with_label("per-cell and per-layer counts");
    gtk_widget_show(mi);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(co_info_proc), (void*)cvINFOplpc);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(co_p1_info), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(co_p1_info), FIO()->CvtInfo());

    gtk_table_attach(GTK_TABLE(tab_form), co_p1_info, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    rcnt++;

    gtk_notebook_append_page(GTK_NOTEBOOK(co_nbook), tab_form, tab_label);

    //
    // CHD file page.
    //
    tab_label = gtk_label_new("CHD file");
    gtk_widget_show(tab_label);
    tab_form = gtk_table_new(1, 1, false);
    gtk_widget_show(tab_form);
    rcnt = 0;

    co_p2_text = gtk_entry_new();
    gtk_widget_show(co_p2_text);
    gtk_entry_set_editable(GTK_ENTRY(co_p2_text), true);
    gtk_table_attach(GTK_TABLE(tab_form), co_p2_text, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    rcnt++;

    // spacer
    label = gtk_label_new("");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(tab_form), label, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    rcnt++;

    sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(tab_form), sep, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    label = gtk_label_new("Handle geometry records in saved CHD file:");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(tab_form), label, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    rcnt++;

    co_p2_mem = gtk_radio_button_new_with_label(0,
        "Read geometry records into new MEMORY CGD");
    gtk_widget_set_name(co_p2_mem, "mem");
    gtk_widget_show(co_p2_mem);
    GSList *group = gtk_radio_button_group(GTK_RADIO_BUTTON(co_p2_mem));
    gtk_table_attach(GTK_TABLE(tab_form), co_p2_mem, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    rcnt++;

    co_p2_file = gtk_radio_button_new_with_label(group,
        "Read geometry records into new FILE CGD");
    gtk_widget_set_name(co_p2_file, "file");
    gtk_widget_show(co_p2_file);
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(co_p2_file));
    gtk_table_attach(GTK_TABLE(tab_form), co_p2_file, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    rcnt++;

    co_p2_none = gtk_radio_button_new_with_label(group,
        "Ignore geometry records");
    gtk_widget_set_name(co_p2_none, "none");
    gtk_widget_show(co_p2_none);
    gtk_table_attach(GTK_TABLE(tab_form), co_p2_none, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);

    switch (sCHDin::get_default_cgd_type()) {
    case CHD_CGDmemory:
        GRX->SetStatus(co_p2_mem, true);
        GRX->SetStatus(co_p2_file, false);
        GRX->SetStatus(co_p2_none, false);
        break;
    case CHD_CGDfile:
        GRX->SetStatus(co_p2_mem, false);
        GRX->SetStatus(co_p2_file, true);
        GRX->SetStatus(co_p2_none, false);
        break;
    case CHD_CGDnone:
        GRX->SetStatus(co_p2_mem, false);
        GRX->SetStatus(co_p2_file, false);
        GRX->SetStatus(co_p2_none, true);
        break;
    }

    gtk_notebook_append_page(GTK_NOTEBOOK(co_nbook), tab_form, tab_label);

    sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    label = gtk_label_new("Access name:");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);
    co_idname = gtk_entry_new();
    gtk_widget_show(co_idname);
    gtk_box_pack_start(GTK_BOX(hbox), co_idname, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;


    gtk_signal_connect(GTK_OBJECT(wb_shell), "key-press-event",
        GTK_SIGNAL_FUNC(co_key_hdlr), 0);
    gtk_signal_connect(GTK_OBJECT(co_nbook), "switch-page",
        GTK_SIGNAL_FUNC(co_page_proc), 0);

    //
    // Apply/Dismiss buttons
    //
    co_apply = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(co_apply, "Apply");
    gtk_widget_show(co_apply);
    gtk_signal_connect(GTK_OBJECT(co_apply), "clicked",
        GTK_SIGNAL_FUNC(co_action), 0);
    gtk_table_attach(GTK_TABLE(form), co_apply, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(co_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), co_p1_text);

    // Constrain overall widget width so title text isn't truncated.
    gtk_widget_set_usize(wb_shell, 340, -1);
    update(init_idname, init_str);
}


sCo::~sCo()
{
    Co = 0;
    delete co_p1_cnmap;
    if (co_caller)
        GRX->Deselect(co_caller);
}


void
sCo::update(const char *init_idname, const char *init_str)
{
    if (init_str) {
        int pg = gtk_notebook_get_current_page(GTK_NOTEBOOK(co_nbook));
        if (pg == 0)
            gtk_entry_set_text(GTK_ENTRY(co_p1_text), init_str);
        else
            gtk_entry_set_text(GTK_ENTRY(co_p2_text), init_str);
    }
    if (init_idname)
        gtk_entry_set_text(GTK_ENTRY(co_idname), init_idname);
    gtk_option_menu_set_history(GTK_OPTION_MENU(co_p1_info),
        FIO()->CvtInfo());
    co_p1_cnmap->update();
}


// Pop down and destroy, call callback.  Note that if the callback
// returns nonzero but not one, the caller is not deselected.
//
void
sCo::apply_proc(GtkWidget *widget)
{
    int ret = true;
    if (co_callback && widget == co_apply) {
        GRX->Deselect(widget);
        char *string;
        int m = 0;
        int pg = gtk_notebook_get_current_page(GTK_NOTEBOOK(co_nbook));
        if (pg == 0)
            string = gtk_editable_get_chars(GTK_EDITABLE(co_p1_text), 0, -1);
        else {
            string = gtk_editable_get_chars(GTK_EDITABLE(co_p2_text), 0, -1);
            if (GRX->GetStatus(co_p2_file))
                m = 1;
            else if (GRX->GetStatus(co_p2_none))
                m = 2;
        }
        if (!string || !*string) {
            PopUpMessage("No input source entered.", true);
            free(string);
            return;
        }
        char *idname = gtk_editable_get_chars(GTK_EDITABLE(co_idname), 0, -1);
        ret = (*co_callback)(idname, string, m, co_arg);
        free(string);
        free(idname);
    }
    if (ret)
        co_cancel_proc(0, 0);
}


// Static function.
void
sCo::co_cancel_proc(GtkWidget*, void*)
{
    delete Co;
}


// Static function.
void
sCo::co_action(GtkWidget *caller, void*)
{
    if (!Co)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:chdadd"))
        return;
    }
    if (!strcmp(name, "Apply"))
        Co->apply_proc(caller);
}


// Private static GTK signal handler.
// In single-line mode, Return is taken as an "OK" termination.
//
int
sCo::co_key_hdlr(GtkWidget*, GdkEvent *ev, void*)
{
    if (Co && ev->key.keyval == GDK_Return) {
        const char *string;
        int pg = gtk_notebook_get_current_page(GTK_NOTEBOOK(Co->co_nbook));
        if (pg == 0)
            string = gtk_entry_get_text(GTK_ENTRY(Co->co_p1_text));
        else
            string = gtk_entry_get_text(GTK_ENTRY(Co->co_p2_text));
        if (string && *string) {
            Co->apply_proc(Co->co_apply);
            return (true);
        }
    }
    return (false);
}


// Private static GTK signal handler.
void
sCo::co_info_proc(GtkWidget*, void *arg)
{
    cvINFO cv;
    switch ((long)arg) {
    case cvINFOnone:
        cv = cvINFOnone;
        break;
    case cvINFOtotals:
        cv = cvINFOtotals;
        break;
    case cvINFOpl:
        cv = cvINFOpl;
        break;
    case cvINFOpc:
        cv = cvINFOpc;
        break;
    case cvINFOplpc:
        cv = cvINFOplpc;
        break;
    default:
        return;
    }
    if (cv != FIO()->CvtInfo()) {
        FIO()->SetCvtInfo(cv);
        Cvt()->PopUpConvert(0, MODE_UPD, 0, 0, 0);
    }
}


// Private static GTK signal handler.
// Drag data received in editing window, grab it
//
void
sCo::co_drag_data_received(GtkWidget *entry,
    GdkDragContext *context, gint, gint, GtkSelectionData *data,
    guint, guint time)
{
    if (data->length >= 0 && data->format == 8 && data->data) {
        char *src = (char*)data->data;
        if (data->target == gdk_atom_intern("TWOSTRING", true)) {
            // Drops from content lists may be in the form
            // "fname_or_chd\ncellname".  Keep the filename.
            char *t = strchr(src, '\n');
            if (t)
                *t = 0;
        }
        gtk_entry_set_text(GTK_ENTRY(entry), src);
        gtk_drag_finish(context, true, false, time);
        return;
    }
    gtk_drag_finish(context, false, false, time);
}


// Private static GTK signal handler.
// Handle page change, set focus to text entry.
//
void
sCo::co_page_proc(GtkNotebook*, GtkNotebookPage*, int num, void*)
{
    if (!Co)
        return;
    if (num == 0)
        gtk_window_set_focus(GTK_WINDOW(Co->wb_shell), Co->co_p1_text);
    else
        gtk_window_set_focus(GTK_WINDOW(Co->wb_shell), Co->co_p2_text);
}

