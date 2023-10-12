
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
 * Simulation defaults popup
 *
 **************************************************************************/

#include "config.h"
#include "graph.h"
#include "circuit.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "kwords_analysis.h"
#include "variable.h"
#include "simulator.h"
#include "gtktoolb.h"


#define STR(x) #x
#define STRINGIFY(x) STR(x)
#define KWGET(string) (xKWent*)sHtab::get(Sp.Options(), string)


namespace {
    void si_cancel_proc(GtkWidget*, void*);
    void si_help_proc(GtkWidget*, void*);
    int si_choice_hdlr(GtkWidget*, GdkEvent*, void*);

    inline void
    dblpr(char *buf, int n, double d, bool ex)
    {
        snprintf(buf, 32, ex ? "%.*e" : "%.*f", n, d);
    }
}


// The simulator defaults popup, initiated from the toolbar.
//
void
GTKtoolbar::PopUpSimDefs(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        if (!sd_shell)
            return;
        TB()->PopUpTBhelp(MODE_OFF, 0, 0, TBH_SD);
        SetLoc(ntb_simdefs, sd_shell);

        GTKdev::Deselect(tb_simdefs);
        g_signal_handlers_disconnect_by_func(G_OBJECT(sd_shell),
            (gpointer)si_cancel_proc, sd_shell);

        for (int i = 0; KW.sim(i)->word; i++) {
            xKWent *ent = static_cast<xKWent*>(KW.sim(i));
            if (ent->ent) {
                if (ent->xent()->entry) {
                    const char *str =
                        gtk_entry_get_text(GTK_ENTRY(ent->xent()->entry));
                    delete [] ent->lastv1;
                    ent->lastv1 = lstring::copy(str);
                }
                if (ent->xent()->entry2) {
                    const char *str =
                        gtk_entry_get_text(GTK_ENTRY(ent->xent()->entry2));
                    delete [] ent->lastv2;
                    ent->lastv2 = lstring::copy(str);
                }
                delete ent->xent();
                ent->ent = 0;
            }
        }
        gtk_widget_destroy(sd_shell);
        sd_shell = 0;
        SetActive(ntb_simdefs, false);
        return;
    }
    if (sd_shell)
        return;

    sd_shell = gtk_NewPopup(0, "Simulation Options", si_cancel_proc, 0);
    if (x || y) {
        FixLoc(&x, &y);
        gtk_window_move(GTK_WINDOW(sd_shell), x, y);
    }

    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(sd_shell), form);

    GtkWidget *hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    GtkWidget *label = gtk_label_new("Set simulation options");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(hbox), frame, false, false, 0);

    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(si_cancel_proc), sd_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 2);

    button = gtk_toggle_button_new_with_label("Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "toggled",
        G_CALLBACK(si_help_proc), sd_shell);
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

    label = gtk_label_new("General Variables");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 4, entrycount,
        entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    xKWent *entry = KWGET(spkw_maxdata);
    if (entry) {
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(256000.0, 1000.0, 0, 0, 0);
        entry->xent()->create_widgets(entry, "256000");

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }
    entry = KWGET(spkw_extprec);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }
    entry = KWGET(spkw_noklu);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 2, 3,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }
    entry = KWGET(spkw_nomatsort);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 3, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_fpemode);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->xent()->setup(0, 1.0, 0.0, 0.0, 0);
        entry->xent()->create_widgets(entry, "0");

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }
    entry = KWGET(spkw_savecurrent);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }
    entry = KWGET(spkw_dcoddstep);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 2, 3,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_loadthrds);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->xent()->setup(0, 1.0, 0.0, 0.0, 0);
        entry->xent()->create_widgets(entry, "0");

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }
    entry = KWGET(spkw_loopthrds);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->xent()->setup(0, 1.0, 0.0, 0.0, 0);
        entry->xent()->create_widgets(entry, "0");

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    char tbuf[64];
    //
    // Timestep
    //
    entrycount = 0;
    form = gtk_table_new(4, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("Timestep");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("Timestep and Integration");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    gtk_table_attach(GTK_TABLE(form), frame, 0, 4, entrycount,
        entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(spkw_steptype);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->xent()->create_widgets(entry, KW.step(0)->word,
            si_choice_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_interplev);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->xent()->setup(DEF_polydegree, 1.0, 0.0, 0.0, 0);
        entry->xent()->create_widgets(entry, STRINGIFY(DEF_polydegree));

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 2, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_method);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->xent()->create_widgets(entry, KW.method(0)->word, si_choice_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_maxord);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->xent()->setup(DEF_maxOrder, 1.0, 0.0, 0.0, 0);
        entry->xent()->create_widgets(entry, STRINGIFY(DEF_maxOrder));

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 2, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_trapratio);
    if (entry) {
        dblpr(tbuf, 2, DEF_trapRatio, false);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_trapRatio, .1, 0.0, 0.0, 2);
        entry->xent()->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_trapcheck);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 2, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_xmu);
    if (entry) {
        dblpr(tbuf, 3, DEF_xmu, false);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_xmu, .01, 0.0, 0.0, 3);
        entry->xent()->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_spice3);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 2, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_trtol);
    if (entry) {
        dblpr(tbuf, 2, DEF_trtol, false);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_trtol, .1, 0.0, 0.0, 2);
        entry->xent()->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_chgtol);
    if (entry) {
        dblpr(tbuf, 2, DEF_chgtol, true);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_chgtol, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_FLOAT;
        entry->xent()->create_widgets(entry, tbuf, kw_float_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 2, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_dphimax);
    if (entry) {
        dblpr(tbuf, 3, DEF_dphiMax, false);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_dphiMax, .01, 0.0, 0.0, 3);
        entry->xent()->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_jjaccel);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 2, 3,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_nojjtp);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 3, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_delmin);
    if (entry) {
        dblpr(tbuf, 2, DEF_delMin, false);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_delMin, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_FLOAT;
        entry->xent()->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_minbreak);
    if (entry) {
        dblpr(tbuf, 2, DEF_minBreak, false);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_minBreak, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_FLOAT;
        entry->xent()->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 2, 4,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    /* NOT CURRENTLY USED
    entry = KWGET(spkw_noiter);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }
    */

    //
    // Tolerance
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("Tolerance");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("Tolerance Variables");
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
    entry = KWGET(spkw_abstol);
    if (entry) {
        dblpr(tbuf, 2, DEF_abstol, true);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_abstol, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_FLOAT;
        entry->xent()->create_widgets(entry, tbuf, kw_float_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_reltol);
    if (entry) {
        dblpr(tbuf, 2, DEF_reltol, true);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_reltol, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_FLOAT;
        entry->xent()->create_widgets(entry, tbuf, kw_float_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_vntol);
    if (entry) {
        dblpr(tbuf, 2, DEF_voltTol, true);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_voltTol, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_FLOAT;
        entry->xent()->create_widgets(entry, tbuf, kw_float_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_pivrel);
    if (entry) {
        dblpr(tbuf, 2, DEF_pivotRelTol, true);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_pivotRelTol, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_FLOAT;
        entry->xent()->create_widgets(entry, tbuf, kw_float_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_gmin);
    if (entry) {
        dblpr(tbuf, 2, DEF_gmin, true);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_gmin, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_FLOAT;
        entry->xent()->create_widgets(entry, tbuf, kw_float_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_pivtol);
    if (entry) {
        dblpr(tbuf, 2, DEF_pivotAbsTol, true);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_pivotAbsTol, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_FLOAT;
        entry->xent()->create_widgets(entry, tbuf, kw_float_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // Convergence
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("Convergence");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("Convergence Variables");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    entrycount++;
    entry = KWGET(spkw_gminsteps);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->xent()->setup(DEF_numGminSteps, 1.0, 0.0, 0.0, 0);
        entry->xent()->create_widgets(entry, STRINGIFY(DEF_numGminSteps));

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_srcsteps);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->xent()->setup(DEF_numSrcSteps, 1.0, 0.0, 0.0, 0);
        entry->xent()->create_widgets(entry, STRINGIFY(DEF_numSrcSteps));

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_gminfirst);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_noopiter);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_dcmu);
    if (entry) {
        dblpr(tbuf, 3, DEF_dcMu, false);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_dcMu, 0.01, 0.0, 0.0, 3);
        entry->xent()->create_widgets(entry, tbuf, kw_float_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_itl1);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->xent()->setup(DEF_dcMaxIter, 1.0, 0.0, 0.0, 0);
        entry->xent()->create_widgets(entry, STRINGIFY(DEF_dcMaxIter));

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_itl2);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->xent()->setup(DEF_dcTrcvMaxIter, 1.0, 0.0, 0.0, 0);
        entry->xent()->create_widgets(entry, STRINGIFY(DEF_dcTrcvMaxIter));

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_itl4);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->xent()->setup(DEF_tranMaxIter, 1.0, 0.0, 0.0, 0);
        entry->xent()->create_widgets(entry, STRINGIFY(DEF_tranMaxIter));

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_itl2gmin);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->xent()->setup(DEF_dcOpGminMaxIter, 1.0, 0.0, 0.0, 0);
        entry->xent()->create_widgets(entry, STRINGIFY(DEF_dcOpGminMaxIter));

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_itl2src);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->xent()->setup(DEF_dcOpSrcMaxIter, 1.0, 0.0, 0.0, 0);
        entry->xent()->create_widgets(entry, STRINGIFY(DEF_dcOpSrcMaxIter));

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_gmax);
    if (entry) {
        dblpr(tbuf, 2, DEF_gmax, true);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_gmax, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_FLOAT;
        entry->xent()->create_widgets(entry, tbuf, kw_float_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_forcegmin);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_rampup);
    if (entry) {
        dblpr(tbuf, 2, 0.0, true);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(0.0, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_FLOAT;
        entry->xent()->create_widgets(entry, tbuf, kw_float_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // Devices
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("Devices");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("Device Parameters");
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
    entry = KWGET(spkw_defad);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%g", DEF_defaultMosAD);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_defaultMosAD, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_NO_SPIN;
        entry->xent()->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_defas);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%g", DEF_defaultMosAS);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_defaultMosAS, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_NO_SPIN;
        entry->xent()->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_defl);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%g", DEF_defaultMosL);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_defaultMosL, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_NO_SPIN;
        entry->xent()->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_defw);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%g", DEF_defaultMosW);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_defaultMosW, 0.1, 0.0, 0.0, 2);
        entry->xent()->mode = KW_NO_SPIN;
        entry->xent()->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_bypass);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%d", DEF_bypass);
        entry->ent = new xEnt(kw_int_func);
        entry->xent()->setup(1.0, 1.0, 0.0, 0.0, 0);
        entry->xent()->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    hbox = gtk_hbox_new(false, 0);
    entry = KWGET(spkw_oldlimit);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_widget_show(hbox);
        gtk_box_pack_start(GTK_BOX(hbox), entry->xent()->frame, true,
            true, 0);
    }

    entry = KWGET(spkw_trytocompact);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_widget_show(hbox);
        gtk_box_pack_start(GTK_BOX(hbox), entry->xent()->frame, true,
            true, 0);
    }
    gtk_table_attach(GTK_TABLE(form), hbox, 1, 2,
        entrycount, entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    entrycount++;

    entry = KWGET(spkw_useadjoint);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // Temperature
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("Temperature");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("Temperature Parameters");
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
    entry = KWGET(spkw_temp);
    if (entry) {
        dblpr(tbuf, 2, DEF_temp - 273.15, false);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_temp - 273.15, .1, 0.0, 0.0, 2);
        entry->xent()->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_tnom);
    if (entry) {
        dblpr(tbuf, 2, DEF_nomTemp - 273.15, false);
        entry->ent = new xEnt(kw_real_func);
        entry->xent()->setup(DEF_temp - 273.15, .1, 0.0, 0.0, 2);
        entry->xent()->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // Parser
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("Parser");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("Parser Variables");
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
    entry = KWGET(kw_modelcard);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->xent()->create_widgets(entry, ".model");

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_subend);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->xent()->create_widgets(entry, ".ends");

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_subinvoke);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->xent()->create_widgets(entry, "x");

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_substart);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->xent()->create_widgets(entry, ".subckt");

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_nobjthack);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_renumber);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_optmerge);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->xent()->create_widgets(entry, KW.optmerge(0)->word, si_choice_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(spkw_parhier);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->xent()->create_widgets(entry, KW.parhier(0)->word, si_choice_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(spkw_hspice);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->xent()->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->xent()->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    gtk_window_set_transient_for(GTK_WINDOW(sd_shell),
        GTK_WINDOW(context->Shell()));
    gtk_widget_show(sd_shell);
    SetActive(ntb_simdefs, true);
}


namespace {
    //
    // Callbacks to process the button selections.
    //

    void
    si_cancel_proc(GtkWidget*, void*)
    {
        TB()->PopUpSimDefs(MODE_OFF, 0, 0);
    }


    void
    si_help_proc(GtkWidget *caller, void *client_data)
    {
        GtkWidget *parent = static_cast<GtkWidget*>(client_data);
        bool state = GTKdev::GetStatus(caller);
        if (state)
            TB()->PopUpTBhelp(MODE_ON, parent, caller, TBH_SD);
        else
            TB()->PopUpTBhelp(MODE_OFF, 0, 0, TBH_SD);
    }


    int
    si_choice_hdlr(GtkWidget *caller, GdkEvent*, void *client_data)
    {
        xKWent *entry = static_cast<xKWent*>(client_data);
        if (GTKdev::GetStatus(entry->xent()->active))
            return (true);
        int i;
        if (!strcmp(entry->word, spkw_method)) {
            char *string =
                gtk_editable_get_chars(GTK_EDITABLE(entry->xent()->entry), 0, -1);
            for (i = 0; KW.method(i)->word; i++)
                if (!strcmp(string, KW.method(i)->word))
                    break;
            if (!KW.method(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad method found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.method(i)->word && KW.method(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.method(i)->word)
                        i = 0;
                }
            }
            delete [] string;
            gtk_entry_set_text(GTK_ENTRY(entry->xent()->entry),
                KW.method(i)->word);
        }
        else if (!strcmp(entry->word, spkw_optmerge)) {
            char *string =
                gtk_editable_get_chars(GTK_EDITABLE(entry->xent()->entry), 0, -1);
            for (i = 0; KW.optmerge(i)->word; i++)
                if (!strcmp(string, KW.optmerge(i)->word))
                    break;
            if (!KW.optmerge(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad optmerge key found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.optmerge(i)->word && KW.optmerge(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.optmerge(i)->word)
                        i = 0;
                }
            }
            delete [] string;
            gtk_entry_set_text(GTK_ENTRY(entry->xent()->entry),
                KW.optmerge(i)->word);
        }
        else if (!strcmp(entry->word, spkw_parhier)) {
            char *string =
                gtk_editable_get_chars(GTK_EDITABLE(entry->xent()->entry), 0, -1);
            for (i = 0; KW.parhier(i)->word; i++)
                if (!strcmp(string, KW.parhier(i)->word))
                    break;
            if (!KW.parhier(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad parhier key found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.parhier(i)->word && KW.parhier(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.parhier(i)->word)
                        i = 0;
                }
            }
            delete [] string;
            gtk_entry_set_text(GTK_ENTRY(entry->xent()->entry),
                KW.parhier(i)->word);
        }
        else if (!strcmp(entry->word, spkw_steptype)) {
            char *string =
                gtk_editable_get_chars(GTK_EDITABLE(entry->xent()->entry), 0, -1);
            for (i = 0; KW.step(i)->word; i++)
                if (!strcmp(string, KW.step(i)->word))
                    break;
            if (!KW.step(i)->word) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "bad steptype found: %s.\n", string);
                i = 0;
            }
            else {
                if (g_object_get_data(G_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.step(i)->word && KW.step(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.step(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->xent()->entry), KW.step(i)->word);
            delete [] string;
        }
        return (true);
    }
}

