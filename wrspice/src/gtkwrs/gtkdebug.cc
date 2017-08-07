
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
 * Debug defaults popup
 *
 **************************************************************************/

#include "config.h"
#include "outplot.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "frontend.h"
#include "ftehelp.h"
#include "gtktoolb.h"

typedef void ParseNode;
#include "spnumber/spparse.h"


#define KWGET(string) (xKWent*)sHtab::get(Sp.Options(), string)

namespace {
    void dbg_cancel_proc(GtkWidget*, void*);
    void dbg_help_proc(GtkWidget*, void*);
    void dbg_debug_proc(GtkWidget*, void*);
    void dbg_upd_proc(GtkWidget*, void*);
}


// The debugging defaults popup, initiated from the toolbar.
//
void
GTKtoolbar::PopUpDebugDefs(int x, int y)
{
    if (db_shell)
        return;

    db_shell = gtk_NewPopup(0, "Debug Options", dbg_cancel_proc, 0);
    if (x || y) {
        FixLoc(&x, &y);
        gtk_widget_set_uposition(db_shell, x, y);
    }

    GtkWidget *form = gtk_table_new(4, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(db_shell), form);

    GtkWidget *hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    GtkWidget *label = gtk_label_new("Debug Options");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(hbox), frame, false, false, 0);

    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        (GtkSignalFunc)dbg_cancel_proc, db_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 2);

    button = gtk_toggle_button_new_with_label("Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
        (GtkSignalFunc)dbg_help_proc, db_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 2);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 4, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    int entrycount = 1;
    xKWent *entry = KWGET(kw_program);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, CP.Program());

        gtk_widget_set_sensitive(entry->ent->active, false);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_display);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, CP.Display());

        gtk_widget_set_sensitive(entry->ent->active, false);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 2, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_debug);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->mode = KW_NO_CB;
        entry->ent->create_widgets(entry, 0);

        gtk_signal_connect(GTK_OBJECT(entry->ent->active), "clicked",
            GTK_SIGNAL_FUNC(dbg_debug_proc), entry);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    xKWent *dbent = entry;

    entry = KWGET(kw_async);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->mode = KW_NO_CB;
        entry->ent->create_widgets(entry, 0);

        gtk_signal_connect(GTK_OBJECT(entry->ent->active), "clicked",
            GTK_SIGNAL_FUNC(dbg_upd_proc), dbent);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 2, 3,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_control);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->mode = KW_NO_CB;
        entry->ent->create_widgets(entry, 0);

        gtk_signal_connect(GTK_OBJECT(entry->ent->active), "clicked",
            GTK_SIGNAL_FUNC(dbg_upd_proc), dbent);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 3, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_cshpar);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->mode = KW_NO_CB;
        entry->ent->create_widgets(entry, 0);

        gtk_signal_connect(GTK_OBJECT(entry->ent->active), "clicked",
            GTK_SIGNAL_FUNC(dbg_upd_proc), dbent);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_eval);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->mode = KW_NO_CB;
        entry->ent->create_widgets(entry, 0);

        gtk_signal_connect(GTK_OBJECT(entry->ent->active), "clicked",
            GTK_SIGNAL_FUNC(dbg_upd_proc), dbent);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_ginterface);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->mode = KW_NO_CB;
        entry->ent->create_widgets(entry, 0);

        gtk_signal_connect(GTK_OBJECT(entry->ent->active), "clicked",
            GTK_SIGNAL_FUNC(dbg_upd_proc), dbent);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 2, 3,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_helpsys);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->mode = KW_NO_CB;
        entry->ent->create_widgets(entry, 0);

        gtk_signal_connect(GTK_OBJECT(entry->ent->active), "clicked",
            GTK_SIGNAL_FUNC(dbg_upd_proc), dbent);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 3, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_plot);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->mode = KW_NO_CB;
        entry->ent->create_widgets(entry, 0);

        gtk_signal_connect(GTK_OBJECT(entry->ent->active), "clicked",
            GTK_SIGNAL_FUNC(dbg_upd_proc), dbent);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_parser);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->mode = KW_NO_CB;
        entry->ent->create_widgets(entry, 0);

        gtk_signal_connect(GTK_OBJECT(entry->ent->active), "clicked",
            GTK_SIGNAL_FUNC(dbg_upd_proc), dbent);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_siminterface);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->mode = KW_NO_CB;
        entry->ent->create_widgets(entry, 0);

        gtk_signal_connect(GTK_OBJECT(entry->ent->active), "clicked",
            GTK_SIGNAL_FUNC(dbg_upd_proc), dbent);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 2, 3,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_vecdb);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->mode = KW_NO_CB;
        entry->ent->create_widgets(entry, 0);

        gtk_signal_connect(GTK_OBJECT(entry->ent->active), "clicked",
            GTK_SIGNAL_FUNC(dbg_upd_proc), dbent);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 3, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    GtkWidget *hsep = gtk_hseparator_new();
    gtk_widget_show(hsep);
    gtk_table_attach(GTK_TABLE(form), hsep, 0, 4,
        entrycount, entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(kw_trantrace);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(0.0, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, "0");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_term);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, "");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 2, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_dontplot);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_nosubckt);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_strictnumparse);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 2, 3,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    gtk_window_set_transient_for(GTK_WINDOW(db_shell),
        GTK_WINDOW(context->Shell()));
    gtk_widget_show(db_shell);
    SetActive(ntb_debug, true);
}


// Remove the plot defaults popup.  Called from the toolbar.
//
void
GTKtoolbar::PopDownDebugDefs()
{
    if (!db_shell)
        return;
    TB()->PopDownTBhelp(TBH_DB);
    SetLoc(ntb_debug, db_shell);

    GRX->Deselect(tb_debug);
    gtk_signal_disconnect_by_func(GTK_OBJECT(db_shell),
        GTK_SIGNAL_FUNC(dbg_cancel_proc), db_shell);

    int i;
    for (i = 0; KW.debug(i)->word; i++) {
        xKWent *ent = static_cast<xKWent*>(KW.debug(i));
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
    for (i = 0; KW.dbargs(i)->word; i++) {
        xKWent *ent = static_cast<xKWent*>(KW.dbargs(i));
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
    gtk_widget_destroy(db_shell);
    db_shell = 0;
    SetActive(ntb_debug, false);
}


namespace {
    //
    // Callbacks to process the button selections.
    //

    void
    dbg_cancel_proc(GtkWidget*, void*)
    {
        TB()->PopDownDebugDefs();
    }


    void
    dbg_help_proc(GtkWidget *caller, void *client_data)
    {
        GtkWidget *parent = static_cast<GtkWidget*>(client_data);
        bool state = GRX->GetStatus(caller);
        if (state)
            TB()->PopUpTBhelp(parent, caller, TBH_DB);
        else
            TB()->PopDownTBhelp(TBH_DB);
    }


    void
    dbg_debug_proc(GtkWidget *caller, void *client_data)
    {
        xKWent *entry = static_cast<xKWent*>(client_data);
        bool isset = GRX->GetStatus(caller);
        variable v;
        if (!isset) {
            v.set_boolean(false);
            entry->callback(false, &v);
        }
        else {
            int i, j;
            for (j = 0, i = 0; KW.dbargs(i)->word; i++) {
                xKWent *k = static_cast<xKWent*>(KW.dbargs(i));
                if (GRX->GetStatus(k->ent->active))
                   j++;
            }
            if (j == i || j == 0) {
                v.set_boolean(true);
                entry->callback(true, &v);
            }
            else if (j == 1) {
                for (i = 0; KW.dbargs(i)->word; i++) {
                    xKWent *k = static_cast<xKWent*>(KW.dbargs(i));
                    if (GRX->GetStatus(k->ent->active)) {
                        v.set_string(k->word);
                        break;
                    }
                }
                entry->callback(true, &v);
            }
            else {
                variable *vl = 0, *vl0 = 0;
                for (i = 0; KW.dbargs(i)->word; i++) {
                    xKWent *k = static_cast<xKWent*>(KW.dbargs(i));
                    if (GRX->GetStatus(k->ent->active)) {
                        if (!vl)
                            vl = vl0 = new variable;
                        else {
                            vl->set_next(new variable);
                            vl = vl->next();
                        }
                        vl->set_string(k->word);
                    }
                }
                v.set_list(vl0);
                entry->callback(true, &v);
            }
        }
    }


    void
    dbg_upd_proc(GtkWidget *caller, void *client_data)
    {
        xKWent *entry = static_cast<xKWent*>(client_data);
        bool state = GRX->GetStatus(entry->ent->active);
        if (!state) {
            state = GRX->GetStatus(caller);
            if (state)
                GRX->SetStatus(entry->ent->active, true);
        }
        else {
            bool st = false;
            for (int i = 0; KW.dbargs(i)->word; i++) {
                xKWent *k = static_cast<xKWent*>(KW.dbargs(i));
                st = GRX->GetStatus(k->ent->active);
                if (st)
                   break;
            }
            if (!st) {
                variable v;
                v.set_boolean(false);
                GRX->SetStatus(entry->ent->active, false);
                entry->callback(false, &v);
                return;
            }
        }
        dbg_debug_proc(entry->ent->active, entry);
    }
}

