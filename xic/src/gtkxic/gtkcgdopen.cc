
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
#include "fio.h"
#include "gtkmain.h"
#include "gtkcv.h"
#include "gtkinlines.h"
#include <gdk/gdkkeysyms.h>


//-----------------------------------------------------------------------------
// Pop up to obtain CGD creation info.
//
// Help system keywords used:
//  xic:cgdopen

namespace {
    namespace gtkcgdopen {
        struct sCgo : public GTKbag
        {
            sCgo(GRobject, bool(*)(const char*, const char*, int, void*),
                void*, const char*, const char*);
            ~sCgo();

            void update(const char*, const char*);

        private:
            void apply_proc(GtkWidget*);
            char *encode_remote_spec(GtkWidget**);
            void load_remote_spec(const char*);

            static void cgo_cancel_proc(GtkWidget*, void*);
            static void cgo_action(GtkWidget*, void*);

            static int cgo_key_hdlr(GtkWidget*, GdkEvent*, void*);
            static void cgo_info_proc(GtkWidget*, void*);
            static void cgo_drag_data_received(GtkWidget*, GdkDragContext*,
                gint, gint, GtkSelectionData*, guint, guint);
            static void cgo_page_proc(GtkNotebook*, void*, int, void*);

            GRobject cgo_caller;
            GtkWidget *cgo_nbook;
            GtkWidget *cgo_p1_entry;
            GtkWidget *cgo_p2_entry;
            GtkWidget *cgo_p3_host;
            GtkWidget *cgo_p3_port;
            GtkWidget *cgo_p3_idname;
            GtkWidget *cgo_idname;
            GtkWidget *cgo_apply;

            llist_t *cgo_p1_llist;
            cnmap_t *cgo_p1_cnmap;

            bool(*cgo_callback)(const char*, const char*, int, void*);
            void *cgo_arg;
        };

        sCgo *Cgo;
    }

    // Drag/drop stuff, also used in PopUpInput(), PopUpEditString()
    //
    GtkTargetEntry target_table[] = {
        { (char*)"TWOSTRING",   0, 0 },
        { (char*)"STRING",      0, 1 },
        { (char*)"text/plain",  0, 2 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);
}

using namespace gtkcgdopen;


void
cConvert::PopUpCgdOpen(GRobject caller, ShowMode mode,
    const char *init_idname, const char *init_str, int x, int y,
    bool(*callback)(const char*, const char*, int, void*), void *arg)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Cgo;
        return;
    }
    if (mode == MODE_UPD) {
        if (Cgo)
            Cgo->update(init_idname, init_str);
        return;
    }
    if (Cgo)
        return;

    new sCgo(caller, callback, arg, init_idname, init_str);
    if (!Cgo->Shell()) {
        delete Cgo;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Cgo->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    int mwid;
    gtk_MonitorGeom(mainBag()->Shell(), 0, 0, &mwid, 0);
    GtkRequisition req;
    gtk_widget_get_requisition(Cgo->Shell(), &req);
    if (x + req.width > mwid)
        x = mwid - req.width;
    gtk_window_move(GTK_WINDOW(Cgo->Shell()), x, y);
    gtk_widget_show(Cgo->Shell());

    // OpenSuse-13.1 gtk-2.24.23 bug
//XXX
    gtk_window_move(GTK_WINDOW(Cgo->Shell()), x, y);
}


// When are the layer list and cell name mapping entries applicable?
// 1) Neither is applied in file or remote CGD creation.
// 2) Layer mapping is applied for memory CGDs from layout files or
//    from CHD names, and from CHD files without geometry.  If a CHD
//    file has geometry, its CGD will be read verbatim.  A CGD file
//    will be read verbatim.
// 3) Cell name mapping is applied only for memory CGDs and layout
//    files.  A CHD source will use the CHD's aliasing.  A CGD file
//    will be read verbatim.

sCgo::sCgo(GRobject caller,
    bool(*callback)(const char*, const char*, int, void*), void *arg,
    const char *init_idname, const char *init_str)
{
    Cgo = this;
    cgo_caller = caller;
    cgo_nbook = 0;
    cgo_p1_entry = 0;
    cgo_p2_entry = 0;
    cgo_p3_host = 0;
    cgo_p3_port = 0;
    cgo_p3_idname = 0;
    cgo_idname = 0;
    cgo_apply = 0;
    cgo_p1_llist = 0;
    cgo_p1_cnmap = 0;
    cgo_callback = callback;
    cgo_arg = arg;

    wb_shell = gtk_NewPopup(0, "Open Cell Geometry Digest",
        cgo_cancel_proc, 0);
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
        "Enter parameters to create new Cell Geometry Digest");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(hbox), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cgo_action), 0);
    gtk_box_pack_end(GTK_BOX(hbox), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    cgo_nbook = gtk_notebook_new();
    gtk_widget_show(cgo_nbook);
    gtk_table_attach(GTK_TABLE(form), cgo_nbook, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Memory page.
    //
    GtkWidget *tab_label = gtk_label_new("in memory");
    gtk_widget_show(tab_label);
    GtkWidget *tab_form = gtk_table_new(1, 1, false);
    gtk_widget_show(tab_form);
    int rcnt = 0;

    label =
        gtk_label_new("All geometry data will be kept in memory.");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(tab_form), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    label = gtk_label_new(
        "Enter path to layout, CHD, or CGD file, or CHD name:");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(tab_form), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    cgo_p1_entry = gtk_entry_new();
    gtk_widget_show(cgo_p1_entry);
    gtk_table_attach(GTK_TABLE(tab_form), cgo_p1_entry, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    // Drop site.
    GtkDestDefaults DD = (GtkDestDefaults)
        (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT);
    gtk_drag_dest_set(cgo_p1_entry, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(cgo_p1_entry), "drag-data-received",
        G_CALLBACK(cgo_drag_data_received), 0);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(tab_form), sep, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    label = gtk_label_new(
        "For layout file, CHD name, or CHD file without geometry only:");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(tab_form), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    //
    // Layer list.
    //
    cgo_p1_llist = new llist_t;
    gtk_table_attach(GTK_TABLE(tab_form), cgo_p1_llist->frame(), 0, 1, rcnt,
        rcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;


    sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(tab_form), sep, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    label = gtk_label_new("For layout file input only:");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(tab_form), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    //
    // Cell name mapping.
    //
    cgo_p1_cnmap = new cnmap_t(false);
    gtk_table_attach(GTK_TABLE(tab_form), cgo_p1_cnmap->frame(), 0, 1, rcnt,
        rcnt+1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    gtk_notebook_append_page(GTK_NOTEBOOK(cgo_nbook), tab_form, tab_label);

    //
    // File reference page.
    //
    tab_label = gtk_label_new("file reference");
    gtk_widget_show(tab_label);
    tab_form = gtk_table_new(1, 1, false);
    gtk_widget_show(tab_form);
    rcnt = 0;

    label = gtk_label_new("Geometry will be read from given file.");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(tab_form), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    label = gtk_label_new(
        "Enter path to CGD file or CHD file containing geometry:");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(tab_form), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    cgo_p2_entry = gtk_entry_new();
    gtk_widget_show(cgo_p2_entry);
    gtk_table_attach(GTK_TABLE(tab_form), cgo_p2_entry, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    // Drop site.
    gtk_drag_dest_set(cgo_p2_entry, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(cgo_p2_entry), "drag-data-received",
        G_CALLBACK(cgo_drag_data_received), 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(cgo_nbook), tab_form, tab_label);

    //
    // Remote reference page.
    //
    tab_label = gtk_label_new("remote server reference");
    gtk_widget_show(tab_label);
    tab_form = gtk_table_new(2, 1, false);
    gtk_widget_show(tab_form);
    rcnt = 0;

    label = gtk_label_new("Geometry will be read from server on remote host.");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_table_attach(GTK_TABLE(tab_form), label, 0, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    label = gtk_label_new("Host name:");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 0, 1);
    gtk_table_attach(GTK_TABLE(tab_form), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    cgo_p3_host = gtk_entry_new();
    gtk_widget_show(cgo_p3_host);
    gtk_table_attach(GTK_TABLE(tab_form), cgo_p3_host, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    label = gtk_label_new("Port number (optional):");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 0, 1);
    gtk_table_attach(GTK_TABLE(tab_form), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    cgo_p3_port = gtk_entry_new();
    gtk_widget_show(cgo_p3_port);
    gtk_table_attach(GTK_TABLE(tab_form), cgo_p3_port, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    label = gtk_label_new("Server CGD access name:");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 0, 1);
    gtk_table_attach(GTK_TABLE(tab_form), label, 0, 1, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    cgo_p3_idname = gtk_entry_new();
    gtk_widget_show(cgo_p3_idname);
    gtk_table_attach(GTK_TABLE(tab_form), cgo_p3_idname, 1, 2, rcnt, rcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rcnt++;

    gtk_notebook_append_page(GTK_NOTEBOOK(cgo_nbook), tab_form, tab_label);

    sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    label = gtk_label_new("Access name:");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 0, 1);
    gtk_table_attach(GTK_TABLE(form), label, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    cgo_idname = gtk_entry_new();
    gtk_widget_show(cgo_idname);
    gtk_table_attach(GTK_TABLE(form), cgo_idname, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    g_signal_connect(G_OBJECT(wb_shell), "key-press-event",
        G_CALLBACK(cgo_key_hdlr), 0);
    g_signal_connect(G_OBJECT(cgo_nbook), "switch-page",
        G_CALLBACK(cgo_page_proc), 0);

    sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Apply/Dismiss buttons.
    //
    cgo_apply = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(cgo_apply, "Apply");
    gtk_widget_show(cgo_apply);
    g_signal_connect(G_OBJECT(cgo_apply), "clicked",
        G_CALLBACK(cgo_action), 0);
    gtk_table_attach(GTK_TABLE(form), cgo_apply, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cgo_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), cgo_p1_entry);

    update(init_idname, init_str);
}


sCgo::~sCgo()
{
    Cgo = 0;
    delete cgo_p1_llist;
    delete cgo_p1_cnmap;
    if (cgo_caller)
        GRX->Deselect(cgo_caller);
}


void
sCgo::update(const char *init_idname, const char *init_str)
{
    if (init_idname)
        gtk_entry_set_text(GTK_ENTRY(cgo_idname), init_idname);
    if (init_str) {
        int pg = gtk_notebook_get_current_page(GTK_NOTEBOOK(cgo_nbook));
        if (pg == 0)
            gtk_entry_set_text(GTK_ENTRY(cgo_p1_entry), init_str);
        else if (pg == 1)
            gtk_entry_set_text(GTK_ENTRY(cgo_p2_entry), init_str);
        else
            load_remote_spec(init_str);
    }
    cgo_p1_llist->update();
    cgo_p1_cnmap->update();
}


// Call the callback, and pop down if the callback returns true.
//
void
sCgo::apply_proc(GtkWidget *widget)
{
    int ret = true;
    if (cgo_callback && widget == cgo_apply) {
        GRX->Deselect(widget);
        char *string;
        int pg = gtk_notebook_get_current_page(GTK_NOTEBOOK(cgo_nbook));
        if (pg == 0) {
            string = gtk_editable_get_chars(GTK_EDITABLE(cgo_p1_entry), 0, -1);
            string = lstring::tocpp(string);
            if (!string || !*string) {
                PopUpMessage("No input source entered.", true);
                delete [] string;
                return;
            }
        }
        else if (pg == 1) {
            string = gtk_editable_get_chars(GTK_EDITABLE(cgo_p2_entry), 0, -1);
            string = lstring::tocpp(string);
            if (!string || !*string) {
                PopUpMessage("No input source entered.", true);
                delete [] string;
                return;
            }
        }
        else {
            GtkWidget *bad;
            string = encode_remote_spec(&bad);
            if (bad) {
                const char *msg;
                if (bad == cgo_p3_host)
                    msg = "Host name text error.";
                else if (bad == cgo_p3_port)
                    msg = "Port nunmber text error.";
                else
                    msg = "Access name text error.";
                PopUpMessage(msg, true);
                return;
            }
        }

        char *idname = gtk_editable_get_chars(GTK_EDITABLE(cgo_idname), 0, -1);

        ret = (*cgo_callback)(idname, string, pg, cgo_arg);
        free(idname);
        delete [] string;
    }
    if (ret)
        cgo_cancel_proc(0, 0);
}


// Take the remote access entries and formuate a string in the format
//   hostname[:port]/idname
// If a bad entry is encountered, return the widget pointer in the arg,
// and a null string.
//
char *
sCgo::encode_remote_spec(GtkWidget **bad)
{
    if (bad)
        *bad = 0;
    sLstr lstr;
    const char *host = gtk_entry_get_text(GTK_ENTRY(cgo_p3_host));
    char *tok = lstring::gettok(&host);
    if (!tok) {
        if (bad)
            *bad = cgo_p3_host;
        return (0);
    }
    lstr.add(tok);
    delete [] tok;

    const char *port = gtk_entry_get_text(GTK_ENTRY(cgo_p3_port));
    tok = lstring::gettok(&port);
    if (tok) {
        int p;
        if (sscanf(tok, "%d", &p) == 1) {
            if (p > 0) {
                lstr.add_c(':');
                lstr.add_i(p);
            }
        }
        else {
            if (bad)
                *bad = cgo_p3_port;
            delete [] tok;
            return (0);
        }
        delete [] tok;
    }

    const char *idname = gtk_entry_get_text(GTK_ENTRY(cgo_p3_idname));
    tok = lstring::gettok(&idname);
    if (!tok) {
        if (bad)
            *bad = cgo_p3_idname;
        return (0);
    }
    lstr.add_c('/');
    lstr.add(tok);
    delete [] tok;
    return (lstr.string_trim());
}


// Parse the string in the form hostname[:port]/idname as much as
// possible, filling in the entries.
//
void
sCgo::load_remote_spec(const char *str)
{
    if (!str)
        return;
    while (isspace(*str))
        str++;
    char *host = lstring::copy(str);
    char *t = host;
    while (*t && *t != ':' && *t != '/')
        t++;
    if (!*t) {
        gtk_entry_set_text(GTK_ENTRY(cgo_p3_host), host);
        gtk_entry_set_text(GTK_ENTRY(cgo_p3_port), "");
        gtk_entry_set_text(GTK_ENTRY(cgo_p3_idname), "");
        return;
    }
    char c = *t;
    *t++ = 0;
    gtk_entry_set_text(GTK_ENTRY(cgo_p3_host), host);
    char *e = t;
    while (*e && *e != '/')
        e++;
    if (*e == '/')
        e++;
    gtk_entry_set_text(GTK_ENTRY(cgo_p3_idname), e);
    *e = 0;
    if (c == ':')
        gtk_entry_set_text(GTK_ENTRY(cgo_p3_port), t);
    else
        gtk_entry_set_text(GTK_ENTRY(cgo_p3_port), "");
}


// Static function.
void
sCgo::cgo_cancel_proc(GtkWidget*, void*)
{
    delete Cgo;
}


// Static function.
void
sCgo::cgo_action(GtkWidget *caller, void*)
{
    if (!Cgo)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help")) {
        DSPmainWbag(PopUpHelp("xic:cgdopen"))
        return;
    }
    if (!strcmp(name, "Apply"))
        Cgo->apply_proc(caller);
}


// Private static GTK signal handler.
// Return is taken as an Apply press, if chars have been entered.
//
int
sCgo::cgo_key_hdlr(GtkWidget*, GdkEvent *ev, void*)
{
    if (Cgo && ev->key.keyval == GDK_KEY_Return) {
        const char *string;
        int pg = gtk_notebook_get_current_page(GTK_NOTEBOOK(Cgo->cgo_nbook));
        if (pg == 0) {
            string = gtk_entry_get_text(GTK_ENTRY(Cgo->cgo_p1_entry));
            if (string && *string) {
                Cgo->apply_proc(Cgo->cgo_apply);
                return (true);
            }
        }
        else if (pg == 1) {
            string = gtk_entry_get_text(GTK_ENTRY(Cgo->cgo_p2_entry));
            if (string && *string) {
                Cgo->apply_proc(Cgo->cgo_apply);
                return (true);
            }
        }
        else {
            string = Cgo->encode_remote_spec(0);
            if (string && *string) {
                Cgo->apply_proc(Cgo->cgo_apply);
                delete [] string;
                return (true);
            }
        }
    }
    return (false);
}


// Private static GTK signal handler.
// Drag data received in editing window, grab it.
//
void
sCgo::cgo_drag_data_received(GtkWidget *entry,
    GdkDragContext *context, gint, gint, GtkSelectionData *data,
    guint, guint time)
{
    if (gtk_selection_data_get_length(data) >= 0 &&
            gtk_selection_data_get_format(data) == 8 &&
            gtk_selection_data_get_data(data)) {
        char *src = (char*)gtk_selection_data_get_data(data);
        if (gtk_selection_data_get_target(data) ==
                gdk_atom_intern("TWOSTRING", true)) {
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
sCgo::cgo_page_proc(GtkNotebook*, void*, int num, void*)
{
    if (!Cgo)
        return;
    if (num == 0)
        gtk_window_set_focus(GTK_WINDOW(Cgo->wb_shell), Cgo->cgo_p1_entry);
    else if (num == 1)
        gtk_window_set_focus(GTK_WINDOW(Cgo->wb_shell), Cgo->cgo_p2_entry);
    else
        gtk_window_set_focus(GTK_WINDOW(Cgo->wb_shell), Cgo->cgo_p3_host);
}

