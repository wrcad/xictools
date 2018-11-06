
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/**************************************************************************
 *
 * Shell defaults popup
 *
 **************************************************************************/

#include "config.h"
#include "graph.h"
#include "frontend.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "gtktoolb.h"


#define KWGET(string) (xKWent*)sHtab::get(Sp.Options(), string)

namespace {
    char *get_sourcepath();
    void sh_cancel_proc(GtkWidget*, void*);
    void sh_help_proc(GtkWidget*, void*);
    void sourcepath_func(bool, variable*, xEnt*);
}


// The plot defaults popup, initiated from the toolbar.
//
void
GTKtoolbar::PopUpShellDefs(int x, int y)
{
    if (sh_shell)
        return;

    sh_shell = gtk_NewPopup(0, "Shell Options", sh_cancel_proc, 0);
    if (x || y) {
        FixLoc(&x, &y);
        gtk_widget_set_uposition(sh_shell, x, y);
    }

    GtkWidget *form = gtk_table_new(4, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(sh_shell), form);

    GtkWidget *hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    GtkWidget *label = gtk_label_new("Shell Options");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(hbox), frame, false, false, 0);

    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        (GtkSignalFunc)sh_cancel_proc, sh_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 2);

    button = gtk_toggle_button_new_with_label("Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
        (GtkSignalFunc)sh_help_proc, sh_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 2);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 4, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    int entrycount = 1;
    xKWent *entry = KWGET(kw_history);
    if (entry) {
        char tbuf[64];
        sprintf(tbuf, "%d", CP_DefHistLen);
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(CP_DefHistLen, 1.0, 0, 0, 0);
        entry->ent->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_prompt);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, "-> ");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 2, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_width);
    if (entry) {
        char tbuf[64];
        sprintf(tbuf, "%d", DEF_WIDTH);
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(DEF_WIDTH, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_height);
    if (entry) {
        char tbuf[64];
        sprintf(tbuf, "%d", DEF_HEIGHT);
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(DEF_HEIGHT, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 2, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_cktvars);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_ignoreeof);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_noaskquit);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 2, 3,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_nocc);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 3, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_noclobber);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_noedit);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_noerrwin);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 2, 3,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_noglob);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 3, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_nomoremode);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_nonomatch);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_nosort);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 2, 3,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_unixcom);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 3, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_sourcepath);
    if (entry) {
        entry->ent = new xEnt(sourcepath_func);
        char *s = get_sourcepath();
        entry->ent->create_widgets(entry, s);
        delete [] s;

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    gtk_window_set_transient_for(GTK_WINDOW(sh_shell),
        GTK_WINDOW(context->Shell()));
    gtk_widget_show(sh_shell);
    SetActive(ntb_shell, true);
}


// Remove the plot defaults popup.  Called from the toolbar.
//
void
GTKtoolbar::PopDownShellDefs()
{
    if (!sh_shell)
        return;
    TB()->PopDownTBhelp(TBH_SH);
    SetLoc(ntb_shell, sh_shell);

    GRX->Deselect(tb_shell);
    gtk_signal_disconnect_by_func(GTK_OBJECT(sh_shell),
        GTK_SIGNAL_FUNC(sh_cancel_proc), sh_shell);

    for (int i = 0; KW.shell(i)->word; i++) {
        xKWent *ent = static_cast<xKWent*>(KW.shell(i));
        if (ent->ent) {
            if (ent->ent->entry) {
                const char *str =
                    gtk_entry_get_text(GTK_ENTRY(ent->ent->entry));
                delete [] ent->lastv1;
                ent->lastv1 = lstring::copy(str);
            }
            if (ent->ent->entry2) {
                const char *str =
                    gtk_entry_get_text(GTK_ENTRY(ent->ent->entry2));
                delete [] ent->lastv2;
                ent->lastv2 = lstring::copy(str);
            }
            delete ent->ent;
            ent->ent = 0;
        }
    }
    gtk_widget_destroy(sh_shell);
    sh_shell = 0;
    SetActive(ntb_shell, false);
}


namespace {
    char *
    get_sourcepath()
    {
        VTvalue vv;
        if (Sp.GetVar(kw_sourcepath, VTYP_STRING, &vv))
            return (lstring::copy(vv.get_string()));
        return (0);
    }


    //
    // Callbacks to process the button selections.
    //

    void
    sh_cancel_proc(GtkWidget*, void*)
    {
        TB()->PopDownShellDefs();
    }


    void
    sh_help_proc(GtkWidget *caller, void *client_data)
    {
        GtkWidget *parent = static_cast<GtkWidget*>(client_data);
        bool state = GRX->GetStatus(caller);
        if (state)
            TB()->PopUpTBhelp(parent, caller, TBH_SH);
        else
            TB()->PopDownTBhelp(TBH_SH);
    }


    void
    sourcepath_func(bool isset, variable*, xEnt *ent)
    {
        if (ent->active) {
            char *s;
            GRX->SetStatus(ent->active, isset);
            if (isset) {
                s = get_sourcepath();
                gtk_entry_set_text(GTK_ENTRY(ent->entry), s ? s : "");
                gtk_entry_set_editable(GTK_ENTRY(ent->entry), false);
                gtk_widget_set_sensitive(ent->entry, false);
                delete [] s;
            }
            else {
                gtk_entry_set_editable(GTK_ENTRY(ent->entry), true);
                gtk_widget_set_sensitive(ent->entry, true);
            }
        }
    }
}

