
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
#include "cvrt.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkcv.h"
#include "gtkinlines.h"
#include <gdk/gdkkeysyms.h>


//-----------------------------------------------------------------------------
// Pop up to save CHD file.
//
// Help system keywords used:
//  xic:chdsav

namespace {
    namespace gtkchdsave {
        struct sCs
        {
            sCs(GRobject, bool(*)(const char*, bool, void*), void*, const char*);
            ~sCs();

            GtkWidget *shell() { return (cs_popup); }

            void update(const char*);

        private:
            void button_hdlr(GtkWidget*);

            static void cs_cancel_proc(GtkWidget*, void*);
            static void cs_action(GtkWidget*, void*);

            static void cs_change_proc(GtkWidget*, void*);
            static int cs_key_hdlr(GtkWidget*, GdkEvent*, void*);
            static void cs_drag_data_received(GtkWidget*, GdkDragContext*,
                gint, gint, GtkSelectionData*, guint, guint);

            GRobject cs_caller;
            GtkWidget *cs_popup;
            GtkWidget *cs_label;
            GtkWidget *cs_text;
            GtkWidget *cs_geom;
            GtkWidget *cs_apply;
            llist_t *cs_llist;

            bool(*cs_callback)(const char*, bool, void*);
            void *cs_arg;
        };

        sCs *Cs;
    }
}

using namespace gtkchdsave;


void
cConvert::PopUpChdSave(GRobject caller, ShowMode mode,
    const char *chdname, int x, int y,
    bool(*callback)(const char*, bool, void*), void *arg)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Cs;
        return;
    }
    if (mode == MODE_UPD) {
        if (Cs)
            Cs->update(chdname);
        return;
    }
    if (Cs)
        return;

    new sCs(caller, callback, arg, chdname);
    if (!Cs->shell()) {
        delete Cs;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Cs->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    int mwid;
    MonitorGeom(mainBag()->Shell(), 0, 0, &mwid, 0);
    if (x + Cs->shell()->requisition.width > mwid)
        x = mwid - Cs->shell()->requisition.width;
    gtk_widget_set_uposition(Cs->shell(), x, y);
    gtk_widget_show(Cs->shell());
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


sCs::sCs(GRobject caller, bool(*callback)(const char*, bool, void*),
    void *arg, const char *chdname)
{
    Cs = this;
    cs_caller = caller;
    cs_label = 0;
    cs_text = 0;
    cs_geom = 0;
    cs_apply = 0;
    cs_llist = 0;
    cs_callback = callback;
    cs_arg = arg;

    cs_popup = gtk_NewPopup(0, "Save Hierarchy Digest File", cs_cancel_proc,
        0);
    if (!cs_popup)
        return;

    GtkWidget *form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(cs_popup), form);
    int rowcnt = 0;

    // Label in frame plus help button.
    //
    char buf[256];
    sprintf(buf, "Saving %s, enter pathname for file:",
        chdname ? chdname : "");
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    cs_label = gtk_label_new(buf);
    gtk_widget_show(cs_label);
    gtk_misc_set_padding(GTK_MISC(cs_label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), cs_label);
    gtk_box_pack_start(GTK_BOX(hbox), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cs_action), 0);
    gtk_box_pack_end(GTK_BOX(hbox), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // This allows user to change label text.
    gtk_object_set_data(GTK_OBJECT(cs_popup), "label", cs_label);

    cs_text = gtk_entry_new();
    gtk_widget_show(cs_text);
    gtk_entry_set_editable(GTK_ENTRY(cs_text), true);
    gtk_table_attach(GTK_TABLE(form), cs_text, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    rowcnt++;

    gtk_widget_set_size_request(cs_text, 320, -1);

    gtk_signal_connect_after(GTK_OBJECT(cs_text), "changed",
        GTK_SIGNAL_FUNC(cs_change_proc), 0);

    // drop site
    GtkDestDefaults DD = (GtkDestDefaults)
        (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT);
    gtk_drag_dest_set(cs_text, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    gtk_signal_connect_after(GTK_OBJECT(cs_text), "drag-data-received",
        GTK_SIGNAL_FUNC(cs_drag_data_received), 0);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    cs_geom = gtk_check_button_new_with_label(
        "Include geometry records in file");
    gtk_widget_set_name(cs_geom, "Geom");
    gtk_widget_show(cs_geom);
    gtk_signal_connect(GTK_OBJECT(cs_geom), "clicked",
        GTK_SIGNAL_FUNC(cs_action), 0);

    gtk_table_attach(GTK_TABLE(form), cs_geom, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Layer list
    //
    cs_llist = new llist_t;
    gtk_widget_set_sensitive(cs_llist->frame(), false);
    gtk_table_attach(GTK_TABLE(form), cs_llist->frame(), 0, 2, rowcnt,
        rowcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Apply/Dismiss buttons
    //
    cs_apply = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(cs_apply, "Apply");
    gtk_widget_show(cs_apply);
    gtk_signal_connect(GTK_OBJECT(cs_apply), "clicked",
        GTK_SIGNAL_FUNC(cs_action), 0);
    gtk_table_attach(GTK_TABLE(form), cs_apply, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cs_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(cs_popup), cs_text);
}


sCs::~sCs()
{
    Cs = 0;
    delete cs_llist;
    if (cs_caller)
        GRX->Deselect(cs_caller);
    if (cs_popup)
        gtk_widget_destroy(cs_popup);
}


void
sCs::update(const char *chdname)
{
    if (!chdname)
        return;
    char buf[256];
    sprintf(buf, "Saving %s, enter pathname for file:", chdname);
    gtk_label_set_text(GTK_LABEL(cs_label), buf);
    cs_llist->update();
}


// Pop down and destroy, call callback.  Note that if the callback
// returns nonzero but not one, the caller is not deselected.
//
void
sCs::button_hdlr(GtkWidget *widget)
{
    int ret = true;
    if (cs_callback && widget == cs_apply) {
        GRX->Deselect(widget);
        char *string = gtk_editable_get_chars(GTK_EDITABLE(cs_text), 0, -1);
        ret = (*cs_callback)(string, GRX->GetStatus(cs_geom), cs_arg);
        free(string);
    }
    if (ret)
        cs_cancel_proc(0, 0);
}


// Static function.
void
sCs::cs_cancel_proc(GtkWidget*, void*)
{
    delete Cs;
}


// Static function.
void
sCs::cs_action(GtkWidget *caller, void*)
{
    if (!Cs)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:chdsav"))
        return;
    }
    if (!strcmp(name, "Apply"))
        Cs->button_hdlr(caller);
    else if (!strcmp(name, "Geom"))
        gtk_widget_set_sensitive(Cs->cs_llist->frame(),
            GRX->GetStatus(caller));
}


// Private static GTK signal handler.
// The entry widget has the focus initially, and the Enter key is ignored.
// After the user types, the Enter key will call the Apply method.
//
void
sCs::cs_change_proc(GtkWidget*, void*)
{
    if (Cs) {
        gtk_signal_connect(GTK_OBJECT(Cs->cs_popup), "key-press-event",
            GTK_SIGNAL_FUNC(cs_key_hdlr), 0);
        gtk_signal_disconnect_by_func(GTK_OBJECT(Cs->cs_text),
            GTK_SIGNAL_FUNC(cs_change_proc), 0);
    }
}


// Private static GTK signal handler.
// In single-line mode, Return is taken as an "OK" termination.
//
int
sCs::cs_key_hdlr(GtkWidget*, GdkEvent *ev, void*)
{
    if (Cs && ev->key.keyval == GDK_Return) {
        Cs->button_hdlr(Cs->cs_apply);
        return (true);
    }
    return (false);
}


// Private static GTK signal handler.
// Drag data received in editing window, grab it
//
void
sCs::cs_drag_data_received(GtkWidget *entry,
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

