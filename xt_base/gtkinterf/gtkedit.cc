
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
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

//
// Code for various manifestations of a text editor popup.
//

#include "config.h"
#include "gtkinterf.h"
#include "gtkedit.h"
#include "gtkfont.h"
#include "gtkfile.h"
#include "gtkutil.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "miscutil/timer.h"
#include "miscutil/encode.h"
#ifdef WIN32
#include "miscutil/msw.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include <gdk/gdkkeysyms.h>

// Help keywords used in this file:
// xeditor
// mailclient

// If the message would exceed this size, it is split into multiple
// messages.
#define MAIL_MAXSIZE 1600000

namespace {
    // defaults
    const char *mail_addr = "bugs@wrcad.com";
    const char *mail_subject = "bug report";

    GtkTargetEntry target_table[] =
    {
        { (char*)"STRING",     0, 0 },
        { (char*)"text/plain", 0, 1 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);


    // Maximum number of subshell edit windows open.
#define NUM_SUBED 5
    GTKeditPopup *EditWin[NUM_SUBED];

    // for hardcopies
    HCcb edHCcb =
    {
        0,            // hcsetup
        0,            // hcgo
        0,            // hcframe
        0,            // format
        0,            // drvrmask
        HClegNone,    // legend
        HCportrait,   // orient
        0,            // resolution
        0,            // command
        false,        // tofile
        "",           // tofilename
        0.25,         // left
        0.25,         // top
        8.0,          // width
        10.5          // height
    };
}


// Pop up a text editor.
//
GReditPopup *
gtk_bag::PopUpTextEditor(const char *fname,
    bool(*editsave)(const char*, void*, XEtype), void *arg, bool source)
{
    GTKeditPopup *we = new GTKeditPopup(this, GTKeditPopup::Editor, fname,
        source, arg);
    if (!we->wb_shell) {
        delete we;
        return (0);
    }
    we->register_callback(editsave);
    if (wb_shell) {
        gtk_window_set_transient_for(GTK_WINDOW(we->wb_shell),
            GTK_WINDOW(wb_shell));
        GRX->SetPopupLocation(GRloc(), we->wb_shell, wb_shell);
    }
    else if (we->transient_for()) {
        gtk_window_set_transient_for(GTK_WINDOW(we->wb_shell),
            GTK_WINDOW(we->transient_for()));
    }
    gtk_widget_show(we->wb_shell);
    if (!GTK_WIDGET_HAS_FOCUS(we->wb_textarea))
        gtk_widget_grab_focus(we->wb_textarea);
    return (we);
}


// Pop up the file for browsing (read only, no load or source).
//
GReditPopup *
gtk_bag::PopUpFileBrowser(const char *fname)
{
    // If we happen to already have this file open, reread it.
    // Called after something was appended to the file.
    for (int i = 0; i < NUM_SUBED; i++) {
        if (EditWin[i]) {
            GTKeditPopup *w = EditWin[i];
            if (w->incarnation() != GTKeditPopup::Browser)
                continue;
            const char *string = w->title_label_string();
            if (fname && string && !strcmp(fname, string)) {
                w->load_file(fname);
                return (w);
            }
        }
    }
    GTKeditPopup *we = new GTKeditPopup(this, GTKeditPopup::Browser, fname,
        false, 0);
    if (!we->wb_shell) {
        delete we;
        return (0);
    }
    gtk_window_set_transient_for(GTK_WINDOW(we->wb_shell),
        GTK_WINDOW(wb_shell));
    GRX->SetPopupLocation(GRloc(LW_UR), we->wb_shell, wb_shell);

    gtk_widget_show(we->wb_shell);
    return (we);
}


// Edit the string, rather than a file.  The string arg is the initial value
// of the string to edit.  When done, callback is called with a copy of the
// new string and arg.  If callback returns true, the widget is destroyed.
// Callback is also called on quit with a 0 string argument.
//
GReditPopup *
gtk_bag::PopUpStringEditor(const char *string,
    bool (*callback)(const char*, void*, XEtype), void *arg)
{
    if (!callback) {
        // pop down and destroy all string editor windows
        for (int i = 0; i < NUM_SUBED; i++) {
            if (EditWin[i] &&
                    EditWin[i]->incarnation() == GTKeditPopup::StringEditor) {
                EditWin[i]->call_callback(0, 0, XE_SAVE);
                EditWin[i]->popdown();
            }
        }
        return (0);
    }
    GTKeditPopup *we = new GTKeditPopup(this, GTKeditPopup::StringEditor,
        string, false, arg);
    if (!we->wb_shell) {
        delete we;
        return (0);
    }
    we->register_callback(callback);
    gtk_window_set_transient_for(GTK_WINDOW(we->wb_shell),
        GTK_WINDOW(wb_shell));
    GRX->SetPopupLocation(GRloc(), we->wb_shell, wb_shell);

    gtk_widget_show(we->wb_shell);
    if (!GTK_WIDGET_HAS_FOCUS(we->wb_textarea))
        gtk_widget_grab_focus(we->wb_textarea);
    return (we);
}


// This is a simple mail editor, calls ed_mail_proc with the GTKeditPopup
// when done.  The downproc, if given, is called before destruction.
//
GReditPopup *
gtk_bag::PopUpMail(const char *subject, const char *mailaddr,
    void(*downproc)(GReditPopup*), GRloc loc)
{
    for (int i = 0; i < NUM_SUBED; i++) {
        if (EditWin[i] && EditWin[i]->incarnation() == GTKeditPopup::Mailer)
            // already active
            return (EditWin[i]);
    }
    GTKeditPopup *we = new GTKeditPopup(this, GTKeditPopup::Mailer, 0,
        false, 0);
    if (!we->wb_shell) {
        delete we;
        return (0);
    }
    we->register_quit_callback(downproc);

    GtkWidget *entry = (GtkWidget*)gtk_object_get_data(
        GTK_OBJECT(we->wb_shell), "subject");
    if (entry)
        gtk_entry_set_text(GTK_ENTRY(entry), subject);
    entry = (GtkWidget*)gtk_object_get_data(GTK_OBJECT(we->wb_shell),
        "mailaddr");
    if (entry)
        gtk_entry_set_text(GTK_ENTRY(entry), mailaddr);

    gtk_window_set_transient_for(GTK_WINDOW(we->wb_shell),
        GTK_WINDOW(wb_shell));
    GRX->SetPopupLocation(loc, we->wb_shell, wb_shell);

    gtk_widget_show(we->wb_shell);
    if (!GTK_WIDGET_HAS_FOCUS(we->wb_textarea))
        gtk_widget_grab_focus(we->wb_textarea);
    return (we);
}
// End of gtk_bag functions

#ifdef WIN32
namespace {
    void crlf_proc(GtkWidget *w, void*)
    {
        GRX->SetCRLFtermination(GRX->GetStatus(w));
    }
}
#endif

GtkWindow *GTKeditPopup::ed_transient_for = 0;


// Create the editor popup.
//
// data set:
// main item factory name:  "<edit>"
// shell            "menubar"           menubar
// shell            "mailaddr"          entry
// shell            "subject"           entry
// menuitem         "attach"            file name
//
GTKeditPopup::GTKeditPopup(gtk_bag *owner, GTKeditPopup::WidgetType type,
    const char *file_or_string, bool with_source, void *arg)
{
    p_parent = owner;
    p_cb_arg = arg;
    ed_title = 0;
    ed_msg = 0;
    ed_fsels[0] = 0;
    ed_fsels[1] = 0;
    ed_fsels[2] = 0;
    ed_fsels[3] = 0;
    ed_string = 0;
    ed_saved_as = 0;
    ed_dropfile = 0;
    ed_search_pop = 0;
    ed_undo_list = 0;
    ed_redo_list = 0;
    ed_widget_type = type;
    ed_last_ev = QUIT;
    ed_text_changed = false;
    ed_havesource = with_source;
    ed_in_undo = true;

    ed_menubar = 0;
    ed_File_Load = 0;
    ed_File_Read = 0;
    ed_File_Save = 0;
    ed_File_SaveAs = 0;
    ed_Edit_Redo = 0;
    ed_Edit_Undo = 0;
    ed_Options_Attach = 0;

    if (register_edit(true)) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "too many edit windows open.\n");
        return;
    }
    wb_sens_set = ed_set_sens;

    if (ed_widget_type == Editor)
        wb_shell = gtk_NewPopup(owner, "Text Editor", ed_quit_proc, this);
    else if (ed_widget_type == Browser) {
        wb_shell = gtk_NewPopup(owner, "File Browser", ed_quit_proc, this);
        ed_havesource = false;
    }
    else if (ed_widget_type == StringEditor) {
        wb_shell = gtk_NewPopup(owner, "Text Editor", ed_quit_proc, this);
        ed_string = file_or_string;
        ed_havesource = false;
    }
    else if (ed_widget_type == Mailer) {
        wb_shell = gtk_NewPopup(owner, "Email Message Editor", ed_quit_proc,
            this);
        ed_havesource = false;
    }
    else
        return;

    if (owner)
        owner->MonitorAdd(this);

    GtkWidget *form = gtk_table_new(1, 3, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    // Menu bar.
    //
    int row = 0;
    GtkAccelGroup *accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(wb_shell), accel_group);
    ed_menubar = gtk_menu_bar_new();
    gtk_object_set_data(GTK_OBJECT(wb_shell), "menubar", ed_menubar);
    gtk_widget_show(ed_menubar);
    GtkWidget *item;

    // File menu.
    item = gtk_menu_item_new_with_mnemonic("_File");
    gtk_widget_set_name(item, "File");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(ed_menubar), item);
    GtkWidget *fileMenu = gtk_menu_new();
    gtk_widget_show(fileMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), fileMenu);

    if (ed_widget_type == Editor || ed_widget_type == Browser) {
        item = gtk_menu_item_new_with_mnemonic("_Open");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(ed_open_proc), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_o,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

        item = gtk_menu_item_new_with_mnemonic("_Load");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(ed_load_proc), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_l,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
        ed_File_Load = item;
    }
    if (ed_widget_type != Browser) {
        item = gtk_menu_item_new_with_mnemonic("_Read");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(ed_read_proc), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_r,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
        ed_File_Read = item;
    }
    if (ed_widget_type != Browser && ed_widget_type != Mailer) {
        item = gtk_menu_item_new_with_mnemonic("_Save");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(ed_save_proc), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_s,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
        if (ed_widget_type != StringEditor) {
            // Don't desensitize in string edit mode.
            gtk_widget_set_sensitive(item, false);
        }
        ed_File_Save = item;
    }
    if (ed_widget_type != StringEditor) {
        item = gtk_menu_item_new_with_mnemonic("Save _As");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(ed_save_as_proc), this);
//        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_a,
//            GDK_ALT_MASK, GTK_ACCEL_VISIBLE);
        ed_File_SaveAs = item;

        item = gtk_check_menu_item_new_with_mnemonic("_Print");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(ed_print_proc), this);
//        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_n,
//            GDK_ALT_MASK, GTK_ACCEL_VISIBLE);
    }

#ifdef WIN32
    if (ed_widget_type == Editor) {
        item = gtk_check_menu_item_new_with_mnemonic("_Write CRLF");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(crlf_proc), this);
        GRX->SetStatus(item, GRX->GetCRLFtermination());
    }
#endif

    item = gtk_separator_menu_item_new();
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);

    if (ed_widget_type == Mailer) {
        item = gtk_menu_item_new_with_mnemonic("_Send Mail");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(ed_mail_proc), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_s,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    }
    item = gtk_menu_item_new_with_mnemonic("_Quit");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(ed_quit_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_q,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    // Edit menu.
    item = gtk_menu_item_new_with_mnemonic("_Edit");
    gtk_widget_set_name(item, "Edit");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(ed_menubar), item);
    GtkWidget *editMenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), editMenu);

    if (ed_widget_type != Browser) {
        item = gtk_menu_item_new_with_label("Undo");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(ed_undo_proc), this);
//        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_u,
//            GDK_ALT_MASK, GTK_ACCEL_VISIBLE);
        ed_Edit_Undo = item;

        item = gtk_menu_item_new_with_label("Redo");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(ed_redo_proc), this);
//        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_r,
//            GDK_ALT_MASK, GTK_ACCEL_VISIBLE);
        ed_Edit_Redo = item;

        item = gtk_menu_item_new_with_label("Cut to Clipboard");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(ed_cut_proc), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_x,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    }
    item = gtk_menu_item_new_with_label("Copy To Clipboard");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(ed_copy_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_x,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    if (ed_widget_type != Browser) {
        item = gtk_menu_item_new_with_label("Paste from Clipboard");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(ed_paste_proc), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_v,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

        item = gtk_menu_item_new_with_label("Paste Primary");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(ed_paste_prim_proc), this);
//        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_p,
//            GDK_ALT_MASK, GTK_ACCEL_VISIBLE);
    }

    // Options menu.
    item = gtk_menu_item_new_with_mnemonic("_Options");
    gtk_widget_set_name(item, "Options");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(ed_menubar), item);
    GtkWidget *optnMenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), optnMenu);
    gtk_widget_show(optnMenu);

    item = gtk_check_menu_item_new_with_mnemonic("_Search");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(optnMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(ed_search_proc), this);
    if (ed_widget_type != Browser && ed_widget_type != Mailer && ed_havesource) {
        item = gtk_menu_item_new_with_mnemonic("_Source");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(optnMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(ed_source_proc), this);
    }
    if (ed_widget_type == Mailer) {
        item = gtk_menu_item_new_with_mnemonic("_Attach");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(optnMenu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(ed_attach_proc), this);
        ed_Options_Attach = item;
    }
    item = gtk_check_menu_item_new_with_mnemonic("_Font");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(optnMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(ed_font_proc), this);

    // Help menu.
    item = gtk_menu_item_new_with_mnemonic("_Help");
    gtk_widget_show(item);
    gtk_widget_set_name(item, "Help");
    gtk_menu_item_set_right_justified(GTK_MENU_ITEM(item), true);
    gtk_menu_shell_append(GTK_MENU_SHELL(ed_menubar), item);
    GtkWidget *helpMenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), helpMenu);
    gtk_widget_show(helpMenu);

    item = gtk_menu_item_new_with_mnemonic("_Help");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(ed_help_proc), this);
//    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_h,
//        GDK_ALT_MASK, GTK_ACCEL_VISIBLE);

    gtk_table_attach(GTK_TABLE(form), ed_menubar, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    if (ed_widget_type == Mailer) {
        GtkWidget *entry = gtk_entry_new();
        gtk_widget_set_name(entry, "To");
        gtk_widget_show(entry);
        GtkWidget *frame = gtk_frame_new("To:");
        gtk_widget_show(frame);
        gtk_container_add(GTK_CONTAINER(frame), entry);
        gtk_object_set_data(GTK_OBJECT(wb_shell), "mailaddr", entry);
        gtk_entry_set_editable(GTK_ENTRY(entry), true);

        gtk_table_attach(GTK_TABLE(form), frame, 0, 1, row, row+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        row++;

        entry = gtk_entry_new();
        gtk_widget_set_name(entry, "Subject");
        gtk_widget_show(entry);
        frame = gtk_frame_new("Subject:");
        gtk_widget_show(frame);
        gtk_container_add(GTK_CONTAINER(frame), entry);
        gtk_object_set_data(GTK_OBJECT(wb_shell), "subject", entry);
        gtk_entry_set_editable(GTK_ENTRY(entry), true);

        gtk_table_attach(GTK_TABLE(form), frame, 0, 1, row, row+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        row++;
    }

    // Scrolled text area, title label.
    //
    ed_title = gtk_frame_new("");
    gtk_widget_show(ed_title);

    GtkWidget *contr;
    text_scrollable_new(&contr, &wb_textarea, FNT_EDITOR);
    gtk_container_add(GTK_CONTAINER(ed_title), contr);

    if (ed_widget_type == StringEditor) {
        text_set_chars(wb_textarea, ed_string);
        text_set_insertion_point(wb_textarea, 0);
    }
    else {
        // drop site
        gtk_drag_dest_set(wb_textarea, GTK_DEST_DEFAULT_ALL, target_table,
            n_targets, GDK_ACTION_COPY);
        g_signal_connect(G_OBJECT(wb_textarea), "drag-data-received",
            G_CALLBACK(ed_drag_data_received), this);
        if (ed_widget_type == Browser)
            g_signal_connect_after(G_OBJECT(wb_textarea), "realize",
                G_CALLBACK(text_realize_proc), this);
    }

    // Default window size, 80 cols, 12 or 24 rows.
    int fw, fh;
    GTKfont::stringBounds(FNT_EDITOR, "=", &fw, &fh);
    int defw = 80*fw + 4;
    int defh = ed_widget_type == Mailer || ed_widget_type == StringEditor ?
        12*fh : 24*fh;
    gtk_widget_set_size_request(wb_textarea, defw, defh);

    g_signal_connect_after(G_OBJECT(wb_textarea), "button-press-event",
        G_CALLBACK(ed_btn_hdlr), this);

    gtk_table_attach(GTK_TABLE(form), ed_title, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    row++;

    // Message label.
    //
    ed_msg = gtk_label_new("Editor");
    gtk_widget_show(ed_msg);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), ed_msg);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    bool readonly = (ed_widget_type == Browser);
    if (ed_widget_type != StringEditor && ed_widget_type != Mailer) {
        char *fname = pathlist::expand_path(file_or_string, false, true);
        check_t which = filestat::check_file(fname, R_OK);
        if (which == NOGO) {
            fname = filestat::make_temp("xe");
            if (ed_widget_type == Browser)
                gtk_label_set_text(GTK_LABEL(ed_msg), "Can't open file!");
            else
                text_set_editable(wb_textarea, true);
        }
        else if (which == READ_OK) {
            if (!read_file(fname, true))
                gtk_label_set_text(GTK_LABEL(ed_msg), "Can't open file!");
            if (ed_widget_type == Browser)
                gtk_label_set_text(GTK_LABEL(ed_msg), "Read-Only");
            else
                text_set_editable(wb_textarea, true);
        }
        else if (which == NO_EXIST) {
            if (ed_widget_type == Browser)
                gtk_label_set_text(GTK_LABEL(ed_msg), "Can't open file!");
            else
                text_set_editable(wb_textarea, true);
        }
        else {
            readonly = true;
            gtk_label_set_text(GTK_LABEL(ed_msg), "Read-Only");
        }
        gtk_frame_set_label(GTK_FRAME(ed_title), fname);
        delete [] fname;
    }
    else {
        if (!readonly) {
            text_set_editable(wb_textarea, true);
            if (ed_widget_type == Mailer)
                gtk_frame_set_label(GTK_FRAME(ed_title),
                    "Please enter your comment");
            else
                gtk_frame_set_label(GTK_FRAME(ed_title), "Text Block");
        }
        else
            gtk_frame_set_label(GTK_FRAME(ed_title), "Read-Only Text Block");
    }

    if (!readonly) {
        GtkTextBuffer *tbf =
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
        g_signal_connect(G_OBJECT(tbf), "insert-text",
            G_CALLBACK(ed_insert_text_proc), this);
        g_signal_connect(G_OBJECT(tbf), "delete-range",
            G_CALLBACK(ed_delete_range_proc), this);
        ed_in_undo = false;
    }

    ed_text_changed = false;
    text_set_change_hdlr(wb_textarea, ed_change_proc, this, true);
    check_sens();

    ed_saved_as = 0;
    ed_last_ev = LOAD;
}


GTKeditPopup::~GTKeditPopup()
{
    if (p_parent) {
        gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }

    const char *fnamein = 0;
    if (ed_widget_type != Mailer && ed_widget_type != StringEditor) {
        if (ed_saved_as)
            fnamein = ed_saved_as;
        else if (ed_title)
            fnamein = gtk_frame_get_label(GTK_FRAME(ed_title));
        fnamein = lstring::copy(fnamein);
    }
    if (wb_fontsel)
        wb_fontsel->popdown();
    register_edit(false);
    if (wb_shell) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(wb_shell),
            (gpointer)ed_quit_proc, this);
    }
    delete ed_search_pop;
    for (int k = 0; k < 4; k++) {
        if (ed_fsels[k])
            ed_fsels[k]->popdown();
    }
    if (ed_widget_type != Mailer) {
        if (p_callback)
            (*p_callback)(fnamein, p_cb_arg, XE_QUIT);
        delete [] fnamein;
    }
    else if (p_cancel)
        (*p_cancel)(this);
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        GRX->Deselect(p_caller);

    delete [] ed_dropfile;

    if (wb_shell)
        gtk_widget_hide(wb_shell);
    // shell destroyed in gtk_bag destructor
}


// GRpopup override
//
void
GTKeditPopup::popdown()
{
    if (p_parent) {
        gtk_bag *owner = dynamic_cast<gtk_bag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// Keep track of open editor windows.
//
bool
GTKeditPopup::register_edit(bool mode)
{
    int i;
    if (mode == true) {
        for (i = 0; i < NUM_SUBED; i++)
            if (EditWin[i] == 0)
                break;
        if (i == NUM_SUBED)
            return (true);
        EditWin[i] = this;
    }
    else {
        for (i = 0; i < NUM_SUBED; i++)
            if (EditWin[i] == this)
                break;
        if (i == NUM_SUBED)
            return (true);
        EditWin[i] = 0;
    }
    return (false);
}


// Check/set menu item sensitivity for items that change during editing.
//
void
GTKeditPopup::check_sens()
{
    if (ed_Edit_Undo)
        gtk_widget_set_sensitive(ed_Edit_Undo, (ed_undo_list != 0));
    if (ed_Edit_Redo)
        gtk_widget_set_sensitive(ed_Edit_Redo, (ed_redo_list != 0));
}


// Function to read a file into the editor.
//
bool
GTKeditPopup::read_file(const char *fname, bool clear)
{
    FILE *fp = fopen(fname, "r");
    if (fp) {
        if (clear)
            text_set_chars(wb_textarea, "");
        char buf[1024];
        for (;;) {
            int n = fread(buf, 1, 1024, fp);
            text_insert_chars_at_point(wb_textarea, 0, buf, n, -1);
            if (n < 1024)
                break;
        }
        fclose(fp);
        text_set_insertion_point(wb_textarea, 0);
        return (true);
    }
    return (false);
}

#define WRT_BLOCK 2048

// Function to write a file from the window contents.
//
bool
GTKeditPopup::write_file(const char *fname, int startpos, int endpos)
{

    FILE *fp = fopen(fname, "wb");
    if (fp) {
        int length = text_get_length(wb_textarea);
        if (endpos >= 0 && endpos < length)
            length = endpos;
#ifdef WIN32
        char lastc = 0;
#endif
        int start = startpos;
        for (;;) {
            int end = start + WRT_BLOCK;
            if (end > length)
                end = length;
            if (end == start)
                break;
            char *s = text_get_chars(wb_textarea, start, end);
#ifdef WIN32
            for (int i = 0; i < (end - start); i++) {
                if (!GRX->GetCRLFtermination()) {
                    if (s[i] == '\r' && s[i+1] == '\n') {
                        lastc = s[i];
                        continue;
                    }
                }
                else if (s[i] == '\n' && lastc != '\r')
                    putc('\r', fp);
                putc(s[i], fp);
                lastc = s[i];
            }
#else
            if (fwrite(s, 1, end - start, fp) < (unsigned)(end - start)) {
                delete [] s;
                fclose(fp);
                return (false);
            }
#endif
            delete [] s;
            if (end - start < WRT_BLOCK)
                break;
            start = end;
        }
        fclose(fp);
        return (true);
    }
    return (false);
}


//-----------------------------------------------------------------------------
// GTK signal handlers

// Private static GTK signal handler.
// Clear the message label when user clicks in text area.
//
int
GTKeditPopup::ed_btn_hdlr(GtkWidget*, GdkEvent*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        gtk_label_set_text(GTK_LABEL(w->ed_msg), "");
        if (w->wb_message)
            w->wb_message->popdown();
    }
    return (true);
}


// Private static GTK signal handler.
// Quit callback.
//
void
GTKeditPopup::ed_quit_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w && w->ed_widget_type != GTKeditPopup::Browser &&
            w->ed_text_changed && gtk_widget_get_window(w->wb_shell)) {

        // If there is no window, "delete window" was received, and we
        // can't pop up the confirmation.  Changes will be lost.  Too
        // bad, but that's why there is a "Quit" button.

        if (w->ed_last_ev != GTKeditPopup::QUIT) {
            w->ed_last_ev = GTKeditPopup::QUIT;
            w->PopUpMessage(
                "Text has been modified.  Press Quit again to quit", false);
            return;
        }
    }
    w->popdown();
}


// Private static GTK signal handler.
// Save file callback.
//
void
GTKeditPopup::ed_save_proc(GtkWidget *caller, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        w->ed_last_ev = GTKeditPopup::SAVE;

        if (w->ed_widget_type == GTKeditPopup::StringEditor) {
            char *string = text_get_chars(w->wb_textarea, 0, -1);
            if (w->p_callback) {
                if ((*w->p_callback)(string, w->p_cb_arg, XE_SAVE))
                    w->popdown();
            }
            return;
        }

        const char *fname = gtk_frame_get_label(GTK_FRAME(w->ed_title));
        if (filestat::check_file(fname, W_OK) == NOGO) {
            w->PopUpMessage(filestat::error_msg(), true);
            return;
        }
        if (!w->write_file(fname, 0 , -1)) {
            w->PopUpMessage("Error: can't write file", true);
            return;
        }

        gtk_label_set_text(GTK_LABEL(w->ed_msg), "Text saved");
        // This can be called with ed_text_changed false if we saved
        // under a new name, and made no subsequent changes.
        if (w->ed_text_changed)
            w->ed_text_changed = false;
        gtk_widget_set_sensitive(GTK_WIDGET(caller), false);
        if (w->ed_saved_as) {
            delete [] w->ed_saved_as;
            w->ed_saved_as = 0;
        }
        if (w->p_callback)
            (*w->p_callback)(fname, w->p_cb_arg, XE_SAVE);
    }
}


// Private static GTK signal handler.
// Save file under a new name callback.
//
void
GTKeditPopup::ed_save_as_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        w->ed_last_ev = GTKeditPopup::SAVEAS;
        if (w->wb_input)
            w->wb_input->popdown();

        int start, end;
        text_get_selection_pos(w->wb_textarea, &start, &end);
        if (start == end) {
            const char *fname = gtk_frame_get_label(GTK_FRAME(w->ed_title));
            if (w->ed_widget_type == GTKeditPopup::Mailer)
                fname = "";
            w->PopUpInput(0, fname, "Save File", ed_do_saveas_proc,
                client_data);
        }
        else
            w->PopUpInput(0, "", "Save Block", ed_do_saveas_proc, client_data);
        // flag as not accepting a drop
        w->wb_input->set_no_drops(true);
    }
}


// Private static GTK signal handler.
// Bring up the printer control popup.
//
void
GTKeditPopup::ed_print_proc(GtkWidget *caller, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        if (edHCcb.command)
            delete [] edHCcb.command;
        edHCcb.command = lstring::copy(GRappIf()->GetPrintCmd());
        w->PopUpPrint(caller, &edHCcb, HCtext);
    }
}


// Private static GTK signal handler.
// Undo last insert/delete.
//
void
GTKeditPopup::ed_undo_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (!w || !w->ed_undo_list)
        return;
    histlist *h = w->ed_undo_list;
    w->ed_undo_list = w->ed_undo_list->h_next;
    w->ed_in_undo = true;
    GtkTextBuffer *tbf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->wb_textarea));
    GtkTextIter istart;
    gtk_text_buffer_get_iter_at_offset(tbf, &istart, h->h_cpos);
    gtk_text_buffer_place_cursor(tbf, &istart);
    if (h->h_deletion)
        gtk_text_buffer_insert(tbf, &istart, h->h_text, -1);
    else {
        GtkTextIter iend;
        gtk_text_buffer_get_iter_at_offset(tbf, &iend,
            h->h_cpos + strlen(h->h_text));
        gtk_text_buffer_delete(tbf, &istart, &iend);
    }
    w->ed_in_undo = false;
    h->h_next = w->ed_redo_list;
    w->ed_redo_list = h;
    w->check_sens();
}


// Private static GTK signal handler.
// Redo last undone operation.
//
void
GTKeditPopup::ed_redo_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (!w || !w->ed_redo_list)
        return;
    histlist *h = w->ed_redo_list;
    w->ed_redo_list = w->ed_redo_list->h_next;
    w->ed_in_undo = true;
    GtkTextBuffer *tbf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->wb_textarea));
    GtkTextIter istart;
    gtk_text_buffer_get_iter_at_offset(tbf, &istart, h->h_cpos);
    gtk_text_buffer_place_cursor(tbf, &istart);
    if (!h->h_deletion)
        gtk_text_buffer_insert(tbf, &istart, h->h_text, -1);
    else {
        GtkTextIter iend;
        gtk_text_buffer_get_iter_at_offset(tbf, &iend,
            h->h_cpos + strlen(h->h_text));
        gtk_text_buffer_delete(tbf, &istart, &iend);
    }
    w->ed_in_undo = false;
    h->h_next = w->ed_undo_list;
    w->ed_undo_list = h;
    w->check_sens();
}


// Private static GTK signal handler.
// Kill selected text, copy to clipboard.
//
void
GTKeditPopup::ed_cut_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w)
        text_cut_clipboard(w->wb_textarea);
}


// Private static GTK signal handler.
// Copy selected text to the clipboard.
//
void
GTKeditPopup::ed_copy_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w)
        text_copy_clipboard(w->wb_textarea);
}


// Private static GTK signal handler.
// Insert clipboard text.
//
void
GTKeditPopup::ed_paste_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w)
        text_paste_clipboard(w->wb_textarea);
}


// Private static GTK signal handler.
// Insert primary selection text.
//
void
GTKeditPopup::ed_paste_prim_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        GtkTextBuffer *tbf =
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->wb_textarea));
        GtkClipboard *cb = gtk_clipboard_get_for_display(
            gdk_display_get_default(), GDK_SELECTION_PRIMARY);
        gtk_text_buffer_paste_clipboard(tbf, cb, 0, true);
    }
}


// Private static GTK signal handler.
// Pop up the regular expression search dialog.
//
void
GTKeditPopup::ed_search_proc(GtkWidget *caller, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        if (!w->ed_search_pop)
            w->ed_search_pop =
                new GTKsearchPopup(caller, w->wb_textarea, 0, 0);
        if (GRX->GetStatus(caller))
            w->ed_search_pop->pop_up_search(MODE_ON);
        else
            w->ed_search_pop->pop_up_search(MODE_OFF);
    }
}


// Private static GTK signal handler.
// Font selector pop-up.
//
void
GTKeditPopup::ed_font_proc(GtkWidget *caller, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        if (GRX->GetStatus(caller))
            w->PopUpFontSel(caller, GRloc(), MODE_ON, 0, 0, FNT_EDITOR);
        else
            w->PopUpFontSel(caller, GRloc(), MODE_OFF, 0, 0, FNT_EDITOR);
    }
}


// Private static GTK signal handler.
// Send the file to the application to be used as input.
//
void
GTKeditPopup::ed_source_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        const char *fname;
        w->ed_last_ev = GTKeditPopup::SOURCE;
        if (w->ed_text_changed) {
            fname = filestat::make_temp("sp");
            if (!w->write_file(fname, 0 , -1)) {
                w->PopUpMessage("Error: can't write temp file", true);
                delete [] fname;
                return;
            }
            filestat::queue_deletion(fname);
        }
        else {
            if (w->ed_saved_as)
                fname = w->ed_saved_as;
            else
                fname = gtk_frame_get_label(GTK_FRAME(w->ed_title));
            fname = lstring::copy(fname);
        }
        if (w->p_callback) {
            gtk_label_set_text(GTK_LABEL(w->ed_msg), "Text sourced");
            (*w->p_callback)(fname, w->p_cb_arg, XE_SOURCE);
        }
        delete [] fname;
    }
}


// Private static GTK signal handler.
// Callback for the attach button - attach a file in mail mode.
//
void
GTKeditPopup::ed_attach_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        char buf[512];
        *buf = '\0';
        if (w->ed_dropfile) {
            strcpy(buf, w->ed_dropfile);
            delete [] w->ed_dropfile;
            w->ed_dropfile = 0;
        }
        w->PopUpInput(0, buf, "Attach File", ed_do_attach_proc, client_data);
    }
}


// Private static GTK signal handler.
// Callback for the unattach button
//
void
GTKeditPopup::ed_unattach_proc(GtkWidget*, void *client_data)
{
    gtk_widget_destroy(GTK_WIDGET(client_data));
}


// Private static GTK signal handler.
// Delete the file name stored as object data.
//
void
GTKeditPopup::ed_data_destr(void *data)
{
    delete [] (char*)data;
}


// Private static GTK signal handler.
// Callback from the bug popup, pack and send mail.
//
void
GTKeditPopup::ed_mail_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (!w)
        return;

    GRloc loc(LW_XYR, 50, 100);

    const char *mailaddr = mail_addr;
    GtkWidget *entry = (GtkWidget*)gtk_object_get_data(
        GTK_OBJECT(w->wb_shell), "mailaddr");
    if (entry)
        mailaddr = gtk_entry_get_text(GTK_ENTRY(entry));
    const char *subject = mail_subject;
    entry = (GtkWidget*)gtk_object_get_data(GTK_OBJECT(w->wb_shell),
        "subject");
    if (entry)
        subject = gtk_entry_get_text(GTK_ENTRY(entry));

    char *body = text_get_chars(w->wb_textarea, 0, -1);

#ifdef WIN32
    int anum = 0;
    const char **attach_ary = 0;
    GtkWidget *menubar =
        (GtkWidget*)gtk_object_get_data(GTK_OBJECT(w->wb_shell), "menubar");
    if (menubar) {
        GList *list = gtk_container_children(GTK_CONTAINER(menubar));
        for (GList *g = list; g; g = g->next) {
            GtkWidget *item = GTK_WIDGET(g->data);
            char *fname = (char*)gtk_object_get_data(GTK_OBJECT(item),
                "attach");
            if (fname && *fname)
                anum++;
        }
        if (anum > 0) {
            attach_ary = new const char*[anum+1];
            int cnt = 0;
            for (GList *g = list; g; g = g->next) {
                GtkWidget *item = GTK_WIDGET(g->data);
                char *fname = (char*)gtk_object_get_data(GTK_OBJECT(item),
                    "attach");
                if (fname && *fname)
                    attach_ary[cnt++] = lstring::copy(fname);
            }
            attach_ary[cnt] = 0;
        }
        g_list_free(list);
    }

    const char *err = msw::MapiSend(mailaddr, subject, body, anum, attach_ary);
    delete [] body;
    if (attach_ary) {
        for (int i = 0; i < anum; i++)
            delete [] attach_ary[i];
        delete [] attach_ary;
    }
    sLstr lstr;
    if (err) {
        lstr.add("Mailer returned error: ");
        lstr.add(err);
        lstr.add_c('.');
    }
    else {
        lstr.add("Message sent, thank you.");
        w->ed_text_changed = false;
    }
    w->PopUpMessage(lstr.string(), (err != 0), false, false, loc);

#else
    char *descname = filestat::make_temp("m1");
    FILE *descfp = fopen(descname, "w");
    if (!descfp) {
        w->PopUpMessage(
            "Error: can't open temporary file.", true, false, false, loc);
        delete [] descname;
        delete [] body;
        return;
    }
    fputs(body, descfp);
    delete [] body;
    fclose(descfp);

    MimeState state;
    state.outfname = filestat::make_temp("m2");
    char buf[256];

    char *header = new char[strlen(mailaddr) + 8];
    strcpy(header, "To: ");
    strcat(header, mailaddr);
    strcat(header, "\n");

    bool err = false;
    GtkWidget *menubar =
        (GtkWidget*)gtk_object_get_data(GTK_OBJECT(w->wb_shell),
            "menubar");
    if (menubar) {
        GList *list = gtk_container_children(GTK_CONTAINER(menubar));
        for (GList *g = list; g; g = g->next) {
            GtkWidget *item = GTK_WIDGET(g->data);
            char *fname = (char*)gtk_object_get_data(GTK_OBJECT(item),
                "attach");
            if (fname && *fname) {
                FILE *ifp = fopen(fname, "r");
                if (ifp) {
                    descfp = state.nfiles ? 0 : fopen(descname, "r");
                    if (encode(&state, ifp, fname, descfp, subject,
                            header, MAIL_MAXSIZE, 0)) {
                        w->PopUpMessage(
                            "Error: can't open temporary file.", true,
                            false, false, loc);
                        err = true;
                    }
                    if (descfp)
                        fclose(descfp);
                }
                else {
                    sprintf(buf, "Error: can't open attachment file %s.",
                        fname);
                    w->PopUpMessage(buf, true, false, false, loc);
                    err = true;
                }
                fclose (ifp);
            }
            if (err)
                break;
        }
        g_list_free(list);
        if (state.outfile)
            fclose(state.outfile);
    }
    delete [] header;

    if (!err) {
        if (!state.nfiles) {
            sprintf(buf, "mail -s \"%s\" %s < %s", subject, mailaddr,
                descname);
            system(buf);
        }
        else {
            for (int i = 0; i < state.nfiles; i++) {
                // What is "-oi"?  took this from mpack
                sprintf(buf, "sendmail -oi %s < %s", mailaddr,
                    state.fnames[i]);
                system(buf);
            }
        }
    }
    unlink(descname);
    delete [] descname;
    for (int i = 0; i < state.nfiles; i++) {
        unlink(state.fnames[i]);
        delete [] state.fnames[i];
    }
    delete [] state.fnames;
    if (!err) {
        w->PopUpMessage("Message sent, thank you.", false, false,
            false, loc);
        w->ed_text_changed = false;
    }
#endif
}


// Private static GTK signal handler.
// Callback to pop up the file selector.
//
void
GTKeditPopup::ed_open_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        int ix;
        for (ix = 0; ix < 4; ix++)
            if (!w->ed_fsels[ix])
                break;
        if (ix == 4) {
            w->PopUpMessage("Too many file selectors open.", true);
            return;
        }

        // open the file selector in the directory of the current file
        char *path = lstring::copy(
            gtk_frame_get_label(GTK_FRAME(w->ed_title)));
        if (path && *path) {
            char *t = strrchr(path, '/');
            if (t)
                *t = 0;
            else {
                delete [] path;
                path = lstring::tocpp(getcwd(0, 0));
            }
        }
        GRloc loc(LW_XYR, 100 + ix*20, 100 + ix*20);
        w->ed_fsels[ix] = w->PopUpFileSelector(fsSEL, loc, ed_fsel_callback,
            0, w, path);
        if (w->ed_fsels[ix])
            w->ed_fsels[ix]->register_usrptr((void**)&w->ed_fsels[ix]);
        delete [] path;
    }
}


// Private static GTK signal handler.
// Callback to load a new file into the editor.
//
void
GTKeditPopup::ed_load_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        if (w->ed_text_changed) {
            if (w->ed_last_ev != GTKeditPopup::LOAD) {
                w->ed_last_ev = GTKeditPopup::LOAD;
                w->PopUpMessage(
                    "Text has been modified.  Press Load again to load",
                    false);
                return;
            }
            if (w->wb_message)
                w->wb_message->popdown();
        }
        char buf[512];
        *buf = '\0';
        if (w->ed_dropfile) {
            strcpy(buf, w->ed_dropfile);
            delete [] w->ed_dropfile;
            w->ed_dropfile = 0;
        }
        else if (w->p_callback)
            (*w->p_callback)(buf, w->p_cb_arg, XE_LOAD);
        w->PopUpInput(0, buf, "Load File", ed_do_load_proc, client_data);
    }
}


// Private static GTK signal handler.
// Read a file into the editor at the current position.
//
void
GTKeditPopup::ed_read_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        char buf[512];
        *buf = '\0';
        if (w->ed_dropfile) {
            strcpy(buf, w->ed_dropfile);
            delete [] w->ed_dropfile;
            w->ed_dropfile = 0;
        }
        w->PopUpInput(0, buf, "Read File", ed_do_read_proc, client_data);
    }
}


// Private static GTK signal handler.
// This callback is called whenever the text is modified.
//
void
GTKeditPopup::ed_change_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        w->ed_last_ev = GTKeditPopup::TEXTMOD;
        gtk_label_set_text(GTK_LABEL(w->ed_msg), "");
        if (w->wb_message)
            w->wb_message->popdown();
        if (w->ed_text_changed)
            return;
        w->ed_text_changed = true;
        if (w->ed_File_Save)
            gtk_widget_set_sensitive(w->ed_File_Save, true);
    }
}


// Private static GTK signal handler.
// Callback to pop up a help window using the application database.
//
void
GTKeditPopup::ed_help_proc(GtkWidget*, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        const char *keyw = w->ed_widget_type == GTKeditPopup::Mailer ?
            "mailclient" : "xeditor";
        if (GRX->MainFrame())
            GRX->MainFrame()->PopUpHelp(keyw);
        else
            w->PopUpHelp(keyw);
    }
}


// Private static GTK signal handler.
// Receive drop data (a path name).
//
void
GTKeditPopup::ed_drag_data_received(GtkWidget *caller, GdkDragContext *context,
    gint, gint, GtkSelectionData *data, guint, guint time, void *client_data)
{
    if (GTK_IS_TEXT_VIEW(caller)) {
        // stop text view native handler
        g_signal_stop_emission_by_name(G_OBJECT(caller), "drag-data-received");
    }
    if (gtk_selection_data_get_length(data) > 0 &&
            gtk_selection_data_get_format(data) == 8) {
        char *src = (char*)gtk_selection_data_get_data(data);
        GdkModifierType mask;
        gdk_window_get_pointer(0, 0, 0, &mask);
        if (mask & GDK_CONTROL_MASK) {
            // If we're pressing Ctrl, insert text at cursor.
            int n = text_get_insertion_point(caller);
            text_insert_chars_at_point(caller, 0, src, strlen(src), n);
            gtk_drag_finish(context, true, false, time);
            return;
        }
        GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
        if (w) {
            delete [] w->ed_dropfile;
            w->ed_dropfile = 0;
            if (w->wb_input) {
                if (!w->wb_input->no_drops()) {
                    w->wb_input->update(0, src);
                    gtk_drag_finish(context, true, false, time);
                    return;
                }
                if (w->wb_input)
                    w->wb_input->popdown();
            }
            GtkWidget *item = w->ed_File_Load;
            if (!item)
                item = w->ed_Options_Attach;
            if (item) {
                w->ed_dropfile = lstring::copy(src);
                gtk_menu_item_activate(GTK_MENU_ITEM(item));
                gtk_drag_finish(context, true, false, time);
                return;
            }
        }
    }
    gtk_drag_finish(context, false, false, time);
}


void
GTKeditPopup::ed_insert_text_proc(GtkTextBuffer*, GtkTextIter *istart,
    char *text, int len, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (!w || w->ed_in_undo || !len)
        return;
    int start = gtk_text_iter_get_offset(istart);
    char *ntext = new char[len+1];
    strncpy(ntext, text, len);
    ntext[len] = 0;
    w->ed_undo_list = new histlist(ntext, start, false, w->ed_undo_list);
    histlist::destroy(w->ed_redo_list);
    w->ed_redo_list = 0;
    w->check_sens();
}


void
GTKeditPopup::ed_delete_range_proc(GtkTextBuffer*, GtkTextIter *istart,
    GtkTextIter *iend, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (!w || w->ed_in_undo)
        return;
    int start = gtk_text_iter_get_offset(istart);
    char *s = lstring::tocpp(gtk_text_iter_get_text(istart, iend));
    w->ed_undo_list = new histlist(s, start, true, w->ed_undo_list);
    histlist::destroy(w->ed_redo_list);
    w->ed_redo_list = 0;
    w->check_sens();
}


//-----------------------------------------------------------------------------
// Mail interface (for attachments)
//
// Attachments are marked by an envelope icon which appears in the menu
// bar.  Pressing the icon drops down a menu containing the attached
// file name as a label, and an "unattach" button.  Pressing unattach
// removes the menu bar icon.
//

namespace {
    // Envelope xpm used to denote attachments.
    // XPM
    static const char * attachpm[] = {
    "32 18 4 1",
    " 	s None	c None",
    ".	c black",
    "x	c white",
    "o	c sienna",
    "                                ",
    "                                ",
    "   .........................    ",
    "   .x..xxxxxxxxxxxxxxxxx..x.    ",
    "   .xxx..xxxxxxxxxxxxx..xxx.    ",
    "   .xxxxx..xxxxxxxxx..xxxxx.    ",
    "   .xooxoxx..xxxxx..xxxxxxx.    ",
    "   .xxoxxxxxx.....xxxxxxxxx.    ",
    "   .xxxxxxxxxxxxxxxxxxxxxxx.    ",
    "   .xxxxxxxxxxxxxxxxxxxxxxx.    ",
    "   .xxxxxxxoxxxxxxxoxxxxxxx.    ",
    "   .xxxxxxxxoxxoxooxxxxxxxx.    ",
    "   .xxxxxxxoxxoxoxxoxxxxxxx.    ",
    "   .xxxxxxxxxxxxxxxxxxxxxxx.    ",
    "   .xxxxxxxxxxxxxxxxxxxxxxx.    ",
    "   .........................    ",
    "                                ",
    "                                "};
}


// Private static callback.
// Callback from the input popup to attach a file to a mail message.
//
void
GTKeditPopup::ed_do_attach_proc(const char *fnamein, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (!w)
        return;
    char *fname = pathlist::expand_path(fnamein, false, true);
    if (!fname)
        return;
    check_t which = filestat::check_file(fname, R_OK);
    if (which != READ_OK) {
        char tbuf[256];
        if (strlen(fname) > 64)
            strcpy(fname + 60, "...");
        sprintf(tbuf, "Can't open %s!", fname);
        gtk_label_set_text(GTK_LABEL(w->ed_msg), tbuf);
        delete [] fname;
        return;
    }
    GtkWidget *item = gtk_menu_item_new();
    gtk_widget_show(item);

    GdkPixbuf *pb = gdk_pixbuf_new_from_xpm_data(attachpm);
    GtkWidget *img = gtk_image_new_from_pixbuf(pb);
    g_object_unref(pb);
    gtk_widget_show(img);
    gtk_container_add(GTK_CONTAINER(item), img);

    gtk_menu_bar_insert(GTK_MENU_BAR(w->ed_menubar), item, 3);

    GtkWidget *menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

    gtk_object_set_data_full(GTK_OBJECT(item), "attach", lstring::copy(fname),
        ed_data_destr);

    if (strlen(fname) > 20)
        strcpy(fname + 20, "...");
    GtkWidget *mitem = gtk_menu_item_new_with_label(fname);
    gtk_widget_show(mitem);
    gtk_menu_append(GTK_MENU(menu), mitem);
    gtk_widget_set_sensitive(mitem, false);

    mitem = gtk_menu_item_new_with_label("Unattach");
    gtk_widget_show(mitem);
    g_signal_connect(G_OBJECT(mitem), "activate",
        G_CALLBACK(ed_unattach_proc), item);
    gtk_menu_append(GTK_MENU(menu), mitem);

    delete [] fname;
    if (w->wb_input)
        w->wb_input->popdown();
}


//-----------------------------------------------------------------------------
// Misc. dialog callbacks

// Private static callback.
// Callback from the file selection widget.
//
void
GTKeditPopup::ed_fsel_callback(const char *fname, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        delete [] w->ed_dropfile;
        w->ed_dropfile = lstring::copy(fname);
        if (w->wb_input)
            w->wb_input->popdown();
        if (w->ed_File_Load)
            gtk_menu_item_activate(GTK_MENU_ITEM(w->ed_File_Load));
    }
}


namespace {
    // See if the two names are the same, discounting white space.
    //
    bool
    same(const char *s, const char *t)
    {
        while (isspace(*s)) s++;
        while (isspace(*t)) t++;
        for (; *s && *t; s++, t++)
            if (*s != *t) return (false);
        if (*s && !isspace(*s)) return (false);
        if (*t && !isspace(*t)) return (false);
        return (true);
    }
}


// Private static callback.
// Callback passed to PopUpInput() to actually save the file.
// If a block is selected, only the block is saved in the new file.
//
void
GTKeditPopup::ed_do_saveas_proc(const char *fnamein, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        char *fname = pathlist::expand_path(fnamein, false, true);
        if (!fname)
            return;
        if (filestat::check_file(fname, W_OK) == NOGO) {
            w->PopUpMessage(filestat::error_msg(), true);
            delete [] fname;
            return;
        }
        const char *mesg;
        int start, end;
        text_get_selection_pos(w->wb_textarea, &start, &end);
        if (start == end) {
            const char *oldfname = gtk_frame_get_label(GTK_FRAME(w->ed_title));
            // no selected text
            if (same(fname, oldfname)) {
                if (!w->ed_text_changed) {
                    gtk_label_set_text(GTK_LABEL(w->ed_msg), "No save needed");
                    delete [] fname;
                    return;
                }
                if (!w->write_file(fname, 0 , -1)) {
                    w->PopUpMessage("Write error, text not saved", true);
                    delete [] fname;
                    return;
                }
                if (w->ed_saved_as) {
                    delete [] w->ed_saved_as;
                    w->ed_saved_as = 0;
                }
                w->ed_text_changed = false;
                if (w->ed_File_Save)
                    gtk_widget_set_sensitive(w->ed_File_Save, false);
            }
            else {
                if (!w->write_file(fname, 0 , -1)) {
                    w->PopUpMessage("Write error, text not saved", true);
                    delete [] fname;
                    return;
                }
                if (w->ed_saved_as)
                    delete [] w->ed_saved_as;
                w->ed_saved_as = lstring::copy(fname);
                if (w->ed_text_changed)
                    w->ed_text_changed = false;
            }
            mesg = "Text saved";
        }
        else {
            if (!w->write_file(fname, start, end)) {
                w->PopUpMessage("Unknown error, block not saved", true);
                delete [] fname;
                return;
            }
            mesg = "Block saved";
        }
        if (w->wb_input)
            w->wb_input->popdown();
        gtk_label_set_text(GTK_LABEL(w->ed_msg), mesg);
        delete [] fname;
    }
}


// Private static callback.
// Callback passed to PopUpInput() to actually read in the new file
// to edit.
//
void
GTKeditPopup::ed_do_load_proc(const char *fnamein, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        char *fname = pathlist::expand_path(fnamein, false, true);
        if (!fname)
            return;
        check_t which = filestat::check_file(fname, R_OK);
        if (which == NOGO) {
            w->PopUpMessage(filestat::error_msg(), true);
            delete [] fname;
            return;
        }
        w->ed_in_undo = true;
        text_set_editable(w->wb_textarea, false);
        bool ok = w->read_file(fname, true);
        if (w->ed_widget_type != GTKeditPopup::Browser)
            text_set_editable(w->wb_textarea, true);
        if (w->wb_input)
            w->wb_input->popdown();
        w->ed_in_undo = false;

        if (!ok) {
            char tbuf[256];
            if (strlen(fname) > 64)
                strcpy(fname + 60, "...");
            sprintf(tbuf, "Can't open %s!", fname);
            gtk_label_set_text(GTK_LABEL(w->ed_msg), tbuf);
            delete [] fname;
            return;
        }

        histlist::destroy(w->ed_undo_list);
        w->ed_undo_list = 0;
        histlist::destroy(w->ed_redo_list);
        w->ed_redo_list = 0;
        w->check_sens();

        gtk_frame_set_label(GTK_FRAME(w->ed_title), fname);
        if (w->ed_text_changed) {
            w->ed_text_changed = false;
            if (w->ed_File_Save)
                gtk_widget_set_sensitive(w->ed_File_Save, false);
        }
        if (w->ed_saved_as) {
            delete [] w->ed_saved_as;
            w->ed_saved_as = 0;
        }
        delete [] fname;
    }
}


// Private static callback.
// Callback passed to PopUpInput() to read in the file starting at
// the current cursor position.
//
void
GTKeditPopup::ed_do_read_proc(const char *fnamein, void *client_data)
{
    GTKeditPopup *w = static_cast<GTKeditPopup*>(client_data);
    if (w) {
        char *fname = pathlist::expand_path(fnamein, false, true);
        if (!fname)
            return;
        check_t which = filestat::check_file(fname, R_OK);
        if (which == NOGO) {
            w->PopUpMessage(filestat::error_msg(), true);
            delete [] fname;
            return;
        }
        text_set_editable(w->wb_textarea, false);
        int pos = text_get_insertion_point(w->wb_textarea);
        text_set_insertion_point(w->wb_textarea, pos);

        char tbuf[256];
        if (!w->read_file(fname, false)) {
            if (strlen(fname) > 64)
                strcpy(fname + 60, "...");
            sprintf(tbuf, "Can't open %s!", fname);
            gtk_label_set_text(GTK_LABEL(w->ed_msg), tbuf);
        }
        else {
            ed_change_proc(0, client_data);  // this isn't called otherwise
            text_set_editable(w->wb_textarea, true);
            if (strlen(fname) > 64)
                strcpy(fname + 60, "...");
            sprintf(tbuf, "Successfully read %s", fname);
            gtk_label_set_text(GTK_LABEL(w->ed_msg), tbuf);
        }
        if (w->wb_input)
            w->wb_input->popdown();
        delete [] fname;
    }
}


// Private static function.
// Set/reset sensitivity of menu items that use PopUpInput.
//
void
GTKeditPopup::ed_set_sens(gtk_bag *wb, bool set, int)
{
    GTKeditPopup *w = dynamic_cast<GTKeditPopup*>(wb);
    if (!w)
        return;
    if (set) {
        if (w->ed_File_Load)
            gtk_widget_set_sensitive(w->ed_File_Load, true);
        if (w->ed_File_Read)
            gtk_widget_set_sensitive(w->ed_File_Read, true);
        if (w->ed_File_SaveAs)
            gtk_widget_set_sensitive(w->ed_File_SaveAs, true);
    }
    else {
        if (w->ed_File_Load)
            gtk_widget_set_sensitive(w->ed_File_Load, false);
        if (w->ed_File_Read)
            gtk_widget_set_sensitive(w->ed_File_Read, false);
        if (w->ed_File_SaveAs)
            gtk_widget_set_sensitive(w->ed_File_SaveAs, false);
    }
}

