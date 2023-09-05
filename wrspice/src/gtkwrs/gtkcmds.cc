
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
 * Command configuration popup
 *
 **************************************************************************/

#include "config.h"
#include "simulator.h"
#include "spglobal.h"
#include "graph.h"
#include "output.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "kwords_analysis.h"
#include "gtktoolb.h"
#include "miscutil/pathlist.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#endif


#define STR(x) #x
#define STRINGIFY(x) STR(x)
#define KWGET(string) (xKWent*)sHtab::get(Sp.Options(), string)

namespace {
#ifdef HAVE_MOZY
    bool get_helppath(char**);
    void helppath_func(bool, variable*, xEnt*);
#endif
    void cmd_cancel_proc(GtkWidget*, void*);
    void cmd_help_proc(GtkWidget*, void*);
    int cmd_choice_hdlr(GtkWidget*, GdkEvent*, void*);

    inline void dblpr(char *buf, int n, double d, bool ex)
    {
        snprintf(buf, 32, ex ? "%.*e" : "%.*f", n, d);
    }
}


// The command configuration popup, initiated from the toolbar.
//
void
GTKtoolbar::PopUpCmdConfig(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        if (!cm_shell)
            return;
        TB()->PopUpTBhelp(MODE_OFF, 0, 0, TBH_CM);
        SetLoc(ntb_commands, cm_shell);

        GTKdev::Deselect(tb_commands);
        g_signal_handlers_disconnect_by_func(G_OBJECT(cm_shell),
            (gpointer)cmd_cancel_proc, cm_shell);

        for (int i = 0; KW.cmds(i)->word; i++) {
            xKWent *ent = static_cast<xKWent*>(KW.cmds(i));
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
        gtk_widget_destroy(cm_shell);
        cm_shell = 0;
        SetActive(ntb_commands, false);
        return;
    }
    if (cm_shell)
        return;

    cm_shell = gtk_NewPopup(0, "Command Options", cmd_cancel_proc, 0);
    if (x || y) {
        FixLoc(&x, &y);
        gtk_window_move(GTK_WINDOW(cm_shell), x, y);
    }

    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(cm_shell), form);

    GtkWidget *hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    GtkWidget *label = gtk_label_new("Command Defaults");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(hbox), frame, false, false, 0);

    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(cmd_cancel_proc), sd_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 2);

    button = gtk_toggle_button_new_with_label("Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "toggled",
        G_CALLBACK(cmd_help_proc), cm_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 2);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_widget_show(notebook);

    gtk_table_attach(GTK_TABLE(form), notebook, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // General
    //
    int entrycount = 0;
    form = gtk_table_new(4, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("General");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("General Parameters");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 5, entrycount,
        entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    xKWent *entry = KWGET(kw_numdgt);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(DEF_numdgt, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, STRINGIFY(DEF_numdgt));

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_units);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, KW.units(0)->word, cmd_choice_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 2, 5,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_dpolydegree);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(DEF_dpolydegree, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, STRINGIFY(DEF_dpolydegree));

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_dcoddstep);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_dollarcmt);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 2, 3,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_nopage);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 3, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_random);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 4, 5,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_errorlog);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, "");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 5,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // Aspice
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("aspice");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("The aspice Command");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, entrycount,
        entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(kw_spicepath);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, Global.ExecProg());

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // check
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("check");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("The check Command");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, entrycount,
        entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(kw_checkiterate);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(DEF_checkiterate, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, STRINGIFY(DEF_checkiterate));

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_mplot_cur);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, "");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // diff
    //
    char tbuf[64];
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("diff");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("The diff Command");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, entrycount,
        entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(kw_diff_abstol);
    if (entry) {
        dblpr(tbuf, 2, DEF_diff_abstol, true);
        entry->ent = new xEnt(kw_real_func);
        entry->ent->setup(DEF_diff_abstol, 0.1, 0.0, 0.0, 2);
        entry->ent->mode = KW_FLOAT;
        entry->ent->create_widgets(entry, tbuf, kw_float_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_diff_reltol);
    if (entry) {
        dblpr(tbuf, 2, DEF_diff_reltol, true);
        entry->ent = new xEnt(kw_real_func);
        entry->ent->setup(DEF_diff_reltol, 0.1, 0.0, 0.0, 2);
        entry->ent->mode = KW_FLOAT;
        entry->ent->create_widgets(entry, tbuf, kw_float_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_diff_vntol);
    if (entry) {
        dblpr(tbuf, 2, DEF_diff_vntol, true);
        entry->ent = new xEnt(kw_real_func);
        entry->ent->setup(DEF_diff_vntol, 0.1, 0.0, 0.0, 2);
        entry->ent->mode = KW_FLOAT;
        entry->ent->create_widgets(entry, tbuf, kw_float_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // edit
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("edit");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("The edit Command");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, entrycount,
        entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(kw_editor);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, "xeditor");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_xicpath);
    if (entry) {
        char *path;
        const char *progname = Global.GfxProgName() && *Global.GfxProgName() ?
            Global.GfxProgName() : "xic";
        const char *prefix = getenv("XT_PREFIX");
        if (!prefix || !lstring::is_rooted(prefix))
            prefix = "/usr/local";
#ifdef WIN32
        char *tpath = pathlist::mk_path("xictools/bin", progname);
#else
        char *tpath = pathlist::mk_path("xictools/xic/bin", progname);
#endif
        path = pathlist::mk_path(prefix, tpath);
        delete [] tpath;
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, path);
        delete [] path;

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // fourier
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("fourier");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("The fourier and spec Commands");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, entrycount,
        entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(kw_fourgridsize);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(DEF_fourgridsize, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, STRINGIFY(DEF_fourgridsize));

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_nfreqs);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(DEF_nfreqs, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, STRINGIFY(DEF_nfreqs));

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // spec
    //

    entrycount++;

    entry = KWGET(kw_specwindow);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, kw_hanning, cmd_choice_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_specwindoworder);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(DEF_specwindoworder, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, STRINGIFY(DEF_specwindoworder));

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;

    entry = KWGET(kw_spectrace);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }


#ifdef HAVE_MOZY
    //
    // help
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("help");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("The help Command");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, entrycount,
        entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(kw_helpinitxpos);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%d", HLP()->get_init_x());
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(HLP()->get_init_x(), 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_helpinitypos);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%d", HLP()->get_init_y());
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(HLP()->get_init_y(), 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_level);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, KW.level(0)->word, cmd_choice_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_helppath);
    if (entry) {
        char *s;
        get_helppath(&s);
        entry->ent = new xEnt(helppath_func);
        entry->ent->create_widgets(entry, s);
        delete [] s;

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }
#endif  // HAVE_MOZY

    //
    // print
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("print");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("The print Command");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, entrycount,
        entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(kw_printautowidth);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_printnoindex);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }
    entry = KWGET(kw_printnoscale);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_printnoheader);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }
    entry = KWGET(kw_printnopageheader);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // rawfile
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("rawfile");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("The rawfile Command");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, entrycount,
        entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(kw_filetype);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, KW.ft(0)->word, cmd_choice_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_rawfileprec);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(DEF_rawfileprec, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, STRINGIFY(DEF_rawfileprec));

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_nopadding);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_rawfile);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry,
            OP.getOutDesc()->outFile() ? OP.getOutDesc()->outFile() : "");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // rspice
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("rspice");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("The rspice Command");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, entrycount,
        entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(kw_rhost);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, "");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_rprogram);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, CP.Program());

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // source
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("source");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("The source Command");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, entrycount,
        entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(kw_noprtitle);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // write
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("write");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("The write Command");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, entrycount,
        entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(kw_appendwrite);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    gtk_window_set_transient_for(GTK_WINDOW(cm_shell),
        GTK_WINDOW(context->Shell()));
    gtk_widget_show(cm_shell);
    SetActive(ntb_commands, true);
}


namespace {
#ifdef HAVE_MOZY
    // Return true and set the path if the help path is currently set as
    // a variable.  Set path to the default help path in use if the help
    // path is not set as a variable, and return false.
    //
    bool get_helppath(char **path)
    {
        VTvalue vv;
        if (Sp.GetVar(kw_helppath, VTYP_STRING, &vv)) {
            *path = lstring::copy(vv.get_string());
            return (true);
        }
        // helppath not found in variable database.  find out what is really
        // being used from the help system.
        //
        *path = 0;
        HLP()->get_path(path);
        if (!*path)
            *path = lstring::copy("");
        return (false);
    }


    void helppath_func(bool isset, variable*, xEnt *ent)
    {
        if (ent->active) {
            GTKdev::SetStatus(ent->active, isset);
            if (isset) {
                char *s;
                get_helppath(&s);
                gtk_entry_set_text(GTK_ENTRY(ent->entry), s);
                gtk_editable_set_editable(GTK_EDITABLE(ent->entry), false);
                delete [] s;
            }
            else
                gtk_editable_set_editable(GTK_EDITABLE(ent->entry), true);
        }
    }
#endif


    //
    // Callbacks to process the button selections.
    //

    void cmd_cancel_proc(GtkWidget*, void*)
    {
        TB()->PopUpCmdConfig(MODE_OFF, 0, 0);
    }


    void cmd_help_proc(GtkWidget *caller, void *client_data)
    {
        GtkWidget *parent = static_cast<GtkWidget*>(client_data);
        bool state = GTKdev::GetStatus(caller);
        if (state)
            TB()->PopUpTBhelp(MODE_ON, parent, caller, TBH_CM);
        else
            TB()->PopUpTBhelp(MODE_OFF, 0, 0, TBH_CM);
    }


    int cmd_choice_hdlr(GtkWidget *caller, GdkEvent*, void *client_data)
    {
        xKWent *entry = static_cast<xKWent*>(client_data);
        if (GTKdev::GetStatus(entry->ent->active))
            return (true);
        int i;
        if (!strcmp(entry->word, kw_filetype)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            for (i = 0; KW.ft(i)->word; i++)
                if (!strcmp(string, KW.ft(i)->word))
                    break;
            if (!KW.ft(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad filetype found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.ft(i)->word && KW.ft(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.ft(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry), KW.ft(i)->word);
        }
        else if (!strcmp(entry->word, kw_level)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            for (i = 0; KW.level(i)->word; i++)
                if (!strcmp(string, KW.level(i)->word))
                    break;
            if (!KW.level(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad level found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.level(i)->word && KW.level(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.level(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry),KW.level(i)->word);
        }
        else if (!strcmp(entry->word, kw_specwindow)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            for (i = 0; KW.spec(i)->word; i++)
                if (!strcmp(string, KW.spec(i)->word))
                    break;
            if (!KW.spec(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad specwindow found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.spec(i)->word && KW.spec(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.spec(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry), KW.spec(i)->word);
        }
        else if (!strcmp(entry->word, kw_units)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            for (i = 0; KW.units(i)->word; i++)
                if (!strcmp(string, KW.units(i)->word))
                    break;
            if (!KW.units(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad units found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.units(i)->word && KW.units(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.units(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry),KW.units(i)->word);
        }
        return (true);
    }
}

