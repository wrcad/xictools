
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
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinterf/gtkfont.h"


//---------------------------------------------------------------------------
// Pop-up interface for the following extraction commands:
//  PNET, ENET, SOURC, EXSET
//

namespace {
    // Drag/drop stuff
    //
    GtkTargetEntry target_table[] = {
        { (char*)"TWOSTRING",   0, 0 },
        { (char*)"STRING",      0, 1 },
        { (char*)"text/plain",  0, 2 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

    namespace gtkextcmd {
        struct sCmd
        {
            sCmd(GRobject, sExtCmd*,
                bool(*)(const char*, void*, bool, const char*, int, int),
                void*, int);
            ~sCmd();

            GtkWidget *shell() { return (cmd_popup); }
            sExtCmd *excmd() { return (cmd_excmd); }

            void update();

        private:
            static void cmd_cancel_proc(GtkWidget*, void*);
            static void cmd_action_proc(GtkWidget*, void*);
            static void cmd_help_proc(GtkWidget*, void*);
            static void cmd_depth_proc(GtkWidget*, void*);
            static void cmd_drag_data_received(GtkWidget*, GdkDragContext*,
                gint, gint, GtkSelectionData*, guint, guint time);

            GRobject cmd_caller;
            GtkWidget *cmd_popup;
            GtkWidget *cmd_label;
            GtkWidget *cmd_text;
            GtkWidget *cmd_go;
            GtkWidget *cmd_cancel;
            GtkWidget **cmd_bx;

            sExtCmd *cmd_excmd;
            bool (*cmd_action)(const char*, void*, bool, const char*, int,
                int);
            void *cmd_arg;
        };

        sCmd *Cmd;
    }
}

using namespace gtkextcmd;

// Depth choices are 0 -- DMAX-1, "all"
#define DMAX 6


// Pop up/down the interface.  Only one pop-up can exist at a given time.
// Args are:
//  caller          invoking button
//  mode            MODE_ON/MODE_OFF/MODE_UPD
//  cmd             buttons and other details
//  x, y            position of pop-up
// action_cb        callback function
//   const char*      button label
//   void*            action_arg
//   bool             button state
//   const char*      entry or depth
//   int, int         widget location
// action_arg       arg to callback function
// depth            initial depth
//
void
cExt::PopUpExtCmd(GRobject caller, ShowMode mode, sExtCmd *cmd,
    bool (*action_cb)(const char*, void*, bool, const char*, int, int),
    void *action_arg, int depth)
{
    if (!GRX || !GTKmainwin::self())
        return;
    if (mode == MODE_OFF) {
        delete Cmd;
        return;
    }
    if (mode == MODE_UPD) {
        if (Cmd)
            Cmd->update();
        return;
    }
    if (Cmd) {
        sExtCmd *oldmode = Cmd->excmd();
        PopUpExtCmd(0, MODE_OFF, 0, 0, 0);
        if (oldmode == cmd)
            return;
    }
    if (!cmd)
        return;

    new sCmd(caller, cmd, action_cb, action_arg, depth);
    if (!Cmd->shell()) {
        delete Cmd;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Cmd->shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GRX->SetPopupLocation(GRloc(), Cmd->shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(Cmd->shell());
}
// End of cExt functions.


sCmd::sCmd(GRobject c, sExtCmd *cmd,
    bool (*action_cb)(const char*, void*, bool, const char*, int, int),
    void *action_arg, int depth)
{
    Cmd = this;
    cmd_caller = c;
    cmd_popup = 0;
    cmd_label = 0;
    cmd_text = 0;
    cmd_go = 0;
    cmd_cancel = 0;

    cmd_excmd = cmd;
    cmd_bx = new GtkWidget*[cmd->num_buttons()];
    for (int i = 0; i < cmd->num_buttons(); i++)
        cmd_bx[i] = 0;

    cmd_action = action_cb;
    cmd_arg = action_arg;

    cmd_popup = gtk_NewPopup(0, cmd_excmd->wintitle(), cmd_cancel_proc, 0);
    if (!cmd_popup)
        return;

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(cmd_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    const char *titlemsg = 0;
    const char *hkw = 0;
    switch (cmd_excmd->type()) {
    case ExtDumpPhys:
        titlemsg = "Dump Physical Netlist";
        hkw = "xic:pnet";
        break;
    case ExtDumpElec:
        titlemsg = "Dump Schematic Netlist";
        hkw = "xic:enet";
        break;
    case ExtLVS:
        titlemsg = "Compare Layout Vs. Schematic";
        hkw = "xic:lvs";
        break;
    case ExtSource:
        titlemsg = "Schematic from SPICE File";
        hkw = "xic:sourc";
        break;
    case ExtSet:
        titlemsg = "Schematic from Layout";
        hkw = "xic:exset";
        break;
    };

    GtkWidget *label = gtk_label_new(titlemsg);
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
        G_CALLBACK(cmd_help_proc), (void*)hkw);
    gtk_box_pack_end(GTK_BOX(hbox), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    frame = gtk_frame_new(cmd_excmd->btntitle());
    gtk_widget_show(frame);
    GtkWidget *btab = gtk_table_new(1, 1, false);
    gtk_widget_show(btab);
    gtk_container_add(GTK_CONTAINER(frame), btab);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    // activate button
    GtkWidget *row2 = gtk_hbox_new(false, 2);
    gtk_widget_show(row2);
    button = gtk_toggle_button_new_with_label(cmd_excmd->gotext());
    gtk_widget_set_name(button, cmd_excmd->gotext());
    gtk_box_pack_start(GTK_BOX(row2), button, true, true, 0);
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cmd_action_proc), 0);
    cmd_go = button;

    // setup check buttons
    for (int i = 0; i < cmd_excmd->num_buttons(); i++) {
        if (!cmd_excmd->button(i)->name())
            continue;
        button = gtk_check_button_new_with_label(cmd_excmd->button(i)->name());
        gtk_widget_set_name(button, cmd_excmd->button(i)->name());
        int col = cmd_excmd->button(i)->col();
        int row = cmd_excmd->button(i)->row();
        gtk_table_attach(GTK_TABLE(btab), button, col, col+1, row, row+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        cmd_bx[i] = button;

        if (cmd_excmd->button(i)->is_active())
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), true);
        gtk_widget_show(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(cmd_action_proc), 0);
    }

    // set sensitivity
    for (int i = 0; i < cmd_excmd->num_buttons(); i++) {
        bool sens = false;
        bool set = false;
        for (int j = 0; j < EXT_BSENS; j++) {
            unsigned int ix = cmd_excmd->button(i)->sens()[j];
            if (ix) {
                ix--;
                if ((int)ix < cmd_excmd->num_buttons()) {
                    set = true;
                    sens = gtk_toggle_button_get_active(
                        GTK_TOGGLE_BUTTON(cmd_bx[ix]));
                    if (sens)
                        break;
                }
            }
        }
        gtk_widget_set_sensitive(cmd_bx[i], !set || sens);
    }

    if (cmd_excmd->has_depth()) {
        //
        // depth option menu
        //
        GtkWidget* row1 = gtk_hbox_new(false, 2);
        gtk_widget_show(row1);
        label = gtk_label_new("Depth to process");
        gtk_widget_show(label);
        gtk_misc_set_padding(GTK_MISC(label), 2, 2);
        gtk_box_pack_start(GTK_BOX(row1), label, false, false, 0);

        GtkWidget *entry = gtk_combo_box_text_new();
        gtk_widget_set_name(entry, "Depth");
        gtk_widget_show(entry);
        if (depth < 0)
            depth = 0;
        if (depth > DMAX)
            depth = DMAX;
        for (int i = 0; i <= DMAX; i++) {
            char buf[16];
            if (i == DMAX)
                strcpy(buf, "all");
            else
                sprintf(buf, "%d", i);
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry), buf);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(entry), depth);
        g_signal_connect(G_OBJECT(entry), "changed",
            G_CALLBACK(cmd_depth_proc), 0);
        gtk_box_pack_start(GTK_BOX(row1), entry, false, false, 0);
        gtk_table_attach(GTK_TABLE(form), row1, 0, 1, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        rowcnt++;
    }

    if (cmd_excmd->message()) {
        //
        // label in frame
        //
        cmd_label = gtk_label_new(cmd_excmd->message());
        gtk_widget_show(cmd_label);
        gtk_misc_set_padding(GTK_MISC(cmd_label), 2, 2);
        frame = gtk_frame_new(0);
        gtk_widget_show(frame);
        gtk_container_add(GTK_CONTAINER(frame), cmd_label);
        gtk_table_attach(GTK_TABLE(form), frame, 0, 1, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
        rowcnt++;

        cmd_text = gtk_entry_new();
        gtk_widget_show(cmd_text);
        if (cmd_excmd->filename())
            gtk_entry_set_text(GTK_ENTRY(cmd_text), cmd_excmd->filename());
        gtk_editable_set_editable(GTK_EDITABLE(cmd_text), true);
        gtk_table_attach(GTK_TABLE(form), cmd_text, 0, 1, rowcnt, rowcnt+1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 0);
        rowcnt++;

        int wid = 250;
        if (cmd_excmd->filename()) {
            int twid = GTKfont::stringWidth(cmd_text, cmd_excmd->filename());
            if (wid < twid)
                wid = twid;
        }
        gtk_widget_set_size_request(cmd_text, wid + 10, -1);

        // drop site
        GtkDestDefaults DD = (GtkDestDefaults)
            (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT);
        gtk_drag_dest_set(cmd_text, DD, target_table, n_targets,
            GDK_ACTION_COPY);
        g_signal_connect_after(G_OBJECT(cmd_text), "drag-data-received",
            G_CALLBACK(cmd_drag_data_received), 0);
    }

    const char *cn = "Cancel";
    cmd_cancel = gtk_button_new_with_label(cn);
    gtk_widget_set_name(cmd_cancel, cn);
    gtk_widget_show(cmd_cancel);
    g_signal_connect(G_OBJECT(cmd_cancel), "clicked",
        G_CALLBACK(cmd_cancel_proc), 0);
    gtk_box_pack_start(GTK_BOX(row2), cmd_cancel, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row2, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(cmd_popup), cmd_cancel);
}


sCmd::~sCmd()
{
    Cmd = 0;
    if (cmd_caller)
        GRX->Deselect(cmd_caller);
    // call action, passing 0 on popdown
    if (cmd_action)
        (*cmd_action)(0, cmd_arg, false, 0, 0, 0);
    if (cmd_popup)
        gtk_widget_destroy(cmd_popup);
    delete [] cmd_bx;
}


// Static function.
void
sCmd::cmd_cancel_proc(GtkWidget*, void*)
{
    EX()->PopUpExtCmd(0, MODE_OFF, 0, 0, 0);
}


void
sCmd::update()
{
    for (int i = 0; i < cmd_excmd->num_buttons(); i++) {
        if (cmd_excmd->button(i)->var()) {
            bool bstate = gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(cmd_bx[i]));
            bool vstate = CDvdb()->getVariable(cmd_excmd->button(i)->var());
            if (lstring::ciprefix("no", cmd_excmd->button(i)->var())) {
                if (bstate == vstate)
                    gtk_button_clicked(GTK_BUTTON(cmd_bx[i]));
            }
            else {
                if (bstate != vstate)
                    gtk_button_clicked(GTK_BUTTON(cmd_bx[i]));
            }
        }
    }
}


// Static function.
void
sCmd::cmd_action_proc(GtkWidget *caller, void*)
{
    if (!Cmd)
        return;
    if (caller == Cmd->cmd_go && !GRX->GetStatus(caller))
        return;
    bool down = false;
    if (!Cmd->cmd_action)
        down = true;
    else {
        for (int i = 0; i < Cmd->cmd_excmd->num_buttons(); i++) {
            bool sens = false;
            bool set = false;
            for (int j = 0; j < EXT_BSENS; j++) {
                unsigned int ix = Cmd->cmd_excmd->button(i)->sens()[j];
                if (ix) {
                    ix--;
                    if ((int)ix < Cmd->cmd_excmd->num_buttons()) {
                        set = true;
                        sens = gtk_toggle_button_get_active(
                            GTK_TOGGLE_BUTTON(Cmd->cmd_bx[ix]));
                        if (sens)
                            break;
                    }
                }
            }
            gtk_widget_set_sensitive(Cmd->cmd_bx[i], !set || sens);
        }
        const char *string = 0;
        int x = 0, y = 0;
        if (Cmd->cmd_text && caller == Cmd->cmd_go) {
            string = gtk_entry_get_text(GTK_ENTRY(Cmd->cmd_text));
            gdk_window_get_origin(gtk_widget_get_window(Cmd->cmd_popup),
                &x, &y);
        }

        // Hide the widget during computation.  If the action returns true,
        // we don't pop down, so make the widget visible again if it still
        // exists.  The widget may have been destroyed.

        if (caller == Cmd->cmd_go)
            gtk_widget_hide(Cmd->cmd_popup);
        if (!Cmd->cmd_action || !(*Cmd->cmd_action)(GRX->GetLabel(caller),
                Cmd->cmd_arg, GRX->GetStatus(caller), string, x, y))
            down = true;
        else if (Cmd)
            gtk_widget_show(Cmd->cmd_popup);
        if (Cmd && caller == Cmd->cmd_go)
            GRX->Deselect(caller);
    }
    if (down)
        EX()->PopUpExtCmd(0, MODE_OFF, 0, 0, 0);
}


void
sCmd::cmd_help_proc(GtkWidget*, void *arg)
{
    const char *hkw = (const char*)arg;
    DSPmainWbag(PopUpHelp(hkw))
}


// Static function.
void
sCmd::cmd_depth_proc(GtkWidget *caller, void*)
{
    if (!Cmd)
        return;
    if (Cmd->cmd_action) {
        char *t = gtk_combo_box_text_get_active_text(
            GTK_COMBO_BOX_TEXT(caller));
        (*Cmd->cmd_action)("depth", Cmd->cmd_arg, true, t, 0, 0);
        g_free(t);
    }
}


// Static function.
// Drag data received in editing window, grab it.
//
void
sCmd::cmd_drag_data_received(GtkWidget *entry, GdkDragContext *context,
    gint, gint, GtkSelectionData *data, guint, guint time)
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

