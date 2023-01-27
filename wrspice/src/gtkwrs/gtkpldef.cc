
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
 * Plot defaults popup
 *
 **************************************************************************/

#include "config.h"
#include "graph.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "simulator.h"
#include "gtktoolb.h"
#include "spnumber/spnumber.h"


#define KWGET(string) (xKWent*)sHtab::get(Sp.Options(), string)

namespace {
    void pl_cancel_proc(GtkWidget*, void*);
    void pl_help_proc(GtkWidget*, void*);
    int pl_choice_hdlr(GtkWidget*, GdkEvent*, void*);
    void int2_func(bool, variable*, xEnt*);
    void real2_func(bool, variable*, xEnt*);
}


// The plot defaults popup, initiated from the toolbar.
//
void
GTKtoolbar::PopUpPlotDefs(int x, int y)
{
    if (pd_shell)
        return;

    pd_shell = gtk_NewPopup(0, "Plot Options", pl_cancel_proc, 0);
    if (x || y) {
        FixLoc(&x, &y);
        gtk_widget_set_uposition(pd_shell, x, y);
    }

    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(pd_shell), form);

    GtkWidget *hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    GtkWidget *label = gtk_label_new("Set plot options");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(hbox), frame, false, false, 0);

    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(pl_cancel_proc), pd_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 2);

    button = gtk_toggle_button_new_with_label("Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "toggled",
        G_CALLBACK(pl_help_proc), pd_shell);
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
    // Plot - 1
    //
    int entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("plot 1");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("Plot Variables, Page 1");
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
    xKWent *entry = KWGET(kw_plotgeom);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_title);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_xlabel);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_ylabel);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_plotstyle);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, KW.pstyles(0)->word, pl_choice_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);
    entry = KWGET(kw_ysep);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);
        gtk_box_pack_start(GTK_BOX(hbox), entry->ent->frame, true, true, 2);
    }

    entry = KWGET(kw_noplotlogo);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);
        gtk_box_pack_start(GTK_BOX(hbox), entry->ent->frame, true, true, 2);
    }
    gtk_box_set_homogeneous(GTK_BOX(hbox), true);
    gtk_table_attach(GTK_TABLE(form), hbox, 1, 2,
        entrycount, entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(kw_gridstyle);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, KW.gstyles(0)->word, pl_choice_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);
    entry = KWGET(kw_nogrid);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);
        gtk_box_pack_start(GTK_BOX(hbox), entry->ent->frame, true, true, 2);
    }

    entry = KWGET(kw_present);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);
        gtk_box_pack_start(GTK_BOX(hbox), entry->ent->frame, true, true, 2);
    }

    gtk_box_set_homogeneous(GTK_BOX(hbox), true);
    gtk_table_attach(GTK_TABLE(form), hbox, 1, 2,
        entrycount, entrycount + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    entrycount++;
    entry = KWGET(kw_scaletype);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, KW.scale(0)->word, pl_choice_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_ticmarks);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(10.0, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, "10");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // Plot - 2
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("plot 2");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("Plot Variables, Page 2");
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
    entry = KWGET(kw_xlimit);
    if (entry) {
        entry->ent = new xEnt(real2_func);
        entry->ent->setup(0.0, 0.0, 0.0, 0.0, 2);
        entry->ent->mode = KW_REAL_2;
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_ylimit);
    if (entry) {
        entry->ent = new xEnt(real2_func);
        entry->ent->setup(0.0, 0.0, 0.0, 0.0, 2);
        entry->ent->mode = KW_REAL_2;
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_xcompress);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(2.0, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, "2");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_xindices);
    if (entry) {
        entry->ent = new xEnt(int2_func);
        entry->ent->mode = KW_INT_2;
        entry->ent->setup(0.0, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, "0");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_xdelta);
    if (entry) {
        entry->ent = new xEnt(kw_real_func);
        entry->ent->mode = KW_NO_SPIN;
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_ydelta);
    if (entry) {
        entry->ent = new xEnt(kw_real_func);
        entry->ent->mode = KW_NO_SPIN;
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_polydegree);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(1.0, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, "1");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_polysteps);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(10.0, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, "10");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_gridsize);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(0.0, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, "0");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_pointchars);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, SpGrPkg::DefPointchars);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // asciiplot
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("asciiplot");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("Asciiplot Variables");
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
    entry = KWGET(kw_noasciiplotvalue);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_nobreak);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_nointerp);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // hardcopy
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("hardcopy");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);
    char tbuf[64];

    label = gtk_label_new("Hardcopy Variables");
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
    entry = KWGET(kw_hcopydriver);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, "postscript_line_draw",
            pl_choice_hdlr);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_hcopycommand);
    if (entry) {
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_hcopyresol);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(0.0, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, "0");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_hcopylandscape);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_hcopywidth);
    if (entry) {
        sprintf(tbuf, "%g", wrsHCcb.width);
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_hcopyheight);
    if (entry) {
        sprintf(tbuf, "%g", wrsHCcb.height);
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_hcopyxoff);
    if (entry) {
        sprintf(tbuf, "%g", wrsHCcb.left);
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_hcopyyoff);
    if (entry) {
        sprintf(tbuf, "%g", wrsHCcb.top);
        entry->ent = new xEnt(kw_string_func);
        entry->ent->create_widgets(entry, tbuf);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entrycount++;
    entry = KWGET(kw_hcopyrmdelay);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(0.0, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, "0");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    //
    // xgraph
    //
    entrycount = 0;
    form = gtk_table_new(2, 1, false);
    gtk_widget_show(form);
    label = gtk_label_new("xgraph");
    gtk_widget_show(label);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), form, label);

    label = gtk_label_new("Xgraph Variables");
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
    entry = KWGET(kw_xgmarkers);
    if (entry) {
        entry->ent = new xEnt(kw_bool_func);
        entry->ent->create_widgets(entry, 0);

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 0, 1,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    entry = KWGET(kw_xglinewidth);
    if (entry) {
        entry->ent = new xEnt(kw_int_func);
        entry->ent->setup(1.0, 1.0, 0.0, 0.0, 0);
        entry->ent->create_widgets(entry, "1");

        gtk_table_attach(GTK_TABLE(form), entry->ent->frame, 1, 2,
            entrycount, entrycount + 1,
            (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
            (GtkAttachOptions)0, 2, 2);
    }

    gtk_window_set_transient_for(GTK_WINDOW(pd_shell),
        GTK_WINDOW(context->Shell()));
    gtk_widget_show(pd_shell);
    SetActive(ntb_plotdefs, true);
}


// Remove the plot defaults popup.  Called from the toolbar.
//
void
GTKtoolbar::PopDownPlotDefs()
{
    if (!pd_shell)
        return;
    TB()->PopDownTBhelp(TBH_PD);
    SetLoc(ntb_plotdefs, pd_shell);

    GRX->Deselect(tb_plotdefs);
    g_signal_handlers_disconnect_by_func(G_OBJECT(pd_shell),
        (gpointer)pl_cancel_proc, pd_shell);

    for (int i = 0; KW.plot(i)->word; i++) {
        xKWent *ent = static_cast<xKWent*>(KW.plot(i));
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
    gtk_widget_destroy(pd_shell);
    pd_shell = 0;
    SetActive(ntb_plotdefs, false);
}


namespace {
    //
    // Callbacks to process the button selections
    //

    void
    pl_cancel_proc(GtkWidget*, void*)
    {
        TB()->PopDownPlotDefs();
    }


    void
    pl_help_proc(GtkWidget *caller, void *client_data)
    {
        GtkWidget *parent = static_cast<GtkWidget*>(client_data);
        bool state = GRX->GetStatus(caller);
        if (state)
            TB()->PopUpTBhelp(parent, caller, TBH_PD);
        else
            TB()->PopDownTBhelp(TBH_PD);
    }


    int
    pl_choice_hdlr(GtkWidget *caller, GdkEvent*, void *client_data)
    {
        xKWent *entry = static_cast<xKWent*>(client_data);
        if (GRX->GetStatus(entry->ent->active))
            return (true);
        int i;
        if (!strcmp(entry->word, kw_plotstyle)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            for (i = 0; KW.pstyles(i)->word; i++)
                if (!strcmp(string, KW.pstyles(i)->word))
                    break;
            if (!KW.pstyles(i)->word) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad plotstyle found: %s.\n",
                    string);
                i = 0;
            }
            else {
                if (gtk_object_get_data(GTK_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.pstyles(i)->word && KW.pstyles(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.pstyles(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry),
                KW.pstyles(i)->word);
        }
        else if (!strcmp(entry->word, kw_gridstyle)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            for (i = 0; KW.gstyles(i)->word; i++)
                if (!strcmp(string, KW.gstyles(i)->word))
                    break;
            if (!KW.gstyles(i)->word) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad gridstyle found: %s.\n",
                    string);
                i = 0;
            }
            else {
                if (gtk_object_get_data(GTK_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.gstyles(i)->word && KW.gstyles(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.gstyles(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry),
                KW.gstyles(i)->word);
        }
        else if (!strcmp(entry->word, kw_scaletype)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            for (i = 0; KW.scale(i)->word; i++)
                if (!strcmp(string, KW.scale(i)->word))
                    break;
            if (!KW.scale(i)->word) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad scaletype found: %s.\n",
                    string);
                i = 0;
            }
            else {
                if (gtk_object_get_data(GTK_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (KW.scale(i)->word && KW.scale(i+1)->word)
                            i++;
                    }
                }
                else {
                    i++;
                    if (!KW.scale(i)->word)
                        i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry),KW.scale(i)->word);
        }
        else if (!strcmp(entry->word, kw_hcopydriver)) {
            const char *string =
                gtk_entry_get_text(GTK_ENTRY(entry->ent->entry));
            i = GRpkgIf()->FindHCindex(string);
            if (i < 0)
                i = wrsHCcb.format;
            else {
                if (gtk_object_get_data(GTK_OBJECT(caller), "down")) {
                    i--;
                    if (i < 0) {
                        i = 0;
                        while (GRpkgIf()->HCof(i) && GRpkgIf()->HCof(i+1))
                            i++;
                    }
                }
                else {
                    i++;
                    if (!GRpkgIf()->HCof(i))
                       i = 0;
                }
            }
            gtk_entry_set_text(GTK_ENTRY(entry->ent->entry),
                GRpkgIf()->HCof(i)->keyword);
        }
        return (true);
    }


    void
    int2_func(bool isset, variable *v, xEnt *ent)
    {
        ent->set_state(isset);
        if (ent->entry) {
            if (isset) {
                int val1=0, val2=0;
                if (v->type() == VTYP_STRING) {
                    const char *string = v->string();
                    double *d = SPnum.parse(&string, false);
                    val1 = (int)*d;
                    while (*string && !isdigit(*string) &&
                        *string != '-' && *string != '+') string++;
                    d = SPnum.parse(&string, true);
                    val2 = (int)*d;
                }
                else if (v->type() == VTYP_LIST) {
                    variable *vx = v->list();
                    val1 = vx->integer();
                    vx = vx->next();
                    val2 = vx->integer();
                }
                char buf[64];
                sprintf(buf, "%d", val1);
                gtk_entry_set_text(GTK_ENTRY(ent->entry), buf);

                if (ent->entry2) {
                    sprintf(buf, "%d", val2);
                    gtk_entry_set_text(GTK_ENTRY(ent->entry2), buf);
                    gtk_entry_set_editable(GTK_ENTRY(ent->entry2), false);
                    gtk_widget_set_sensitive(ent->entry2, false);
                }
            }
            else {
                if (ent->entry2) {
                    gtk_entry_set_editable(GTK_ENTRY(ent->entry2), true);
                    gtk_widget_set_sensitive(ent->entry2, true);
                }
            }
        }
    }


    void
    real2_func(bool isset, variable *v, xEnt *ent)
    {
        ent->set_state(isset);
        if (ent->entry) {
            if (isset) {
                double dval1=0.0, dval2=0.0;
                if (v->type() == VTYP_STRING) {
                    const char *string = v->string();
                    double *d = SPnum.parse(&string, false);
                    dval1 = *d;
                    while (*string && !isdigit(*string) &&
                        *string != '-' && *string != '+') string++;
                    d = SPnum.parse(&string, true);
                    dval2 = *d;
                }
                else if (v->type() == VTYP_LIST) {
                    variable *vx = v->list();
                    dval1 = vx->real();
                    vx = vx->next();
                    dval2 = vx->real();
                }
                char buf[64];
                sprintf(buf, "%g", dval1);
                gtk_entry_set_text(GTK_ENTRY(ent->entry), buf);

                if (ent->entry2) {
                    sprintf(buf, "%g", dval2);
                    gtk_entry_set_text(GTK_ENTRY(ent->entry2), buf);
                    gtk_entry_set_editable(GTK_ENTRY(ent->entry2), false);
                    gtk_widget_set_sensitive(ent->entry2, false);
                }
            }
            else {
                if (ent->entry2) {
                    gtk_entry_set_editable(GTK_ENTRY(ent->entry2), true);
                    gtk_widget_set_sensitive(ent->entry2, true);
                }
            }
        }
    }
}
