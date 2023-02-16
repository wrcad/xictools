
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
#include "edit.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "cvrt_variables.h"


//--------------------------------------------------------------------------
// Pop up for the Flatten command
//
// Help system keywords used:
//  xic:flatn

namespace {
    namespace gtkflatten {
        struct sFlt
        {
            sFlt (GRobject, bool(*)(const char*, bool, const char*, void*),
                void*, int, bool);
            ~sFlt();

            GtkWidget *shell() { return (fl_popup); }

            void update();

        private:
            static void fl_cancel_proc(GtkWidget*, void*);
            static void fl_action_proc(GtkWidget*, void*);
            static void fl_depth_proc(GtkWidget*, void*);

            GRobject fl_caller;
            GtkWidget *fl_popup;
            GtkWidget *fl_novias;
            GtkWidget *fl_nopcells;
            GtkWidget *fl_nolabels;
            GtkWidget *fl_merge;
            GtkWidget *fl_go;
            bool (*fl_callback)(const char*, bool, const char*, void*);
            void *fl_arg;
        };

        sFlt *Flt;
    }
}

using namespace gtkflatten;

#define DMAX 6


// Pop-up for the Flatten command, consists of a depth option selector,
// fast-mode toggle, go and dismiss buttons.
//
void
cEdit::PopUpFlatten(GRobject caller, ShowMode mode,
    bool (*callback)(const char*, bool, const char*, void*),
    void *arg, int depth, bool fmode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Flt;
        return;
    }
    if (mode == MODE_UPD) {
        if (Flt)
            Flt->update();
        return;
    }
    if (Flt)
        return;

    new sFlt(caller, callback, arg, depth, fmode);
    if (!Flt->shell()) {
        delete Flt;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Flt->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), Flt->shell(), mainBag()->Viewport());
    gtk_widget_show(Flt->shell());
}
// End of cEdit functions.


sFlt::sFlt (GRobject c, bool(*callback)(const char*, bool, const char*, void*),
    void *arg, int depth, bool fmode)
{
    Flt = this;
    fl_caller = c;
    fl_popup = 0;
    fl_novias = 0;
    fl_nopcells = 0;
    fl_nolabels = 0;
    fl_merge = 0;
    fl_go = 0;
    fl_callback = callback;
    fl_arg = arg;

    fl_popup = gtk_NewPopup(0, "Flatten Hierarchy", fl_cancel_proc, 0);
    if (!fl_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(fl_popup), false);

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(fl_popup), form);
    int rowcnt = 0;

    //
    // label in frame plus help btn
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new("Selected subcells will be flattened.");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(row), frame, true, true, 0);
    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fl_action_proc), 0);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // depth option
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    label = gtk_label_new("Depth to flatten");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(row), label, false, false, 0);

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
        G_CALLBACK(fl_depth_proc), 0);

    gtk_box_pack_start(GTK_BOX(row), entry, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // check boxes
    //
    button = gtk_check_button_new_with_label(
        "Don't flatten standard vias, move to top");
    gtk_widget_set_name(button, "StdVias");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fl_action_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    fl_novias = button;

    button = gtk_check_button_new_with_label(
        "Don't flatten param. cells, move to top");
    gtk_widget_set_name(button, "PCells");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fl_action_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    fl_nopcells = button;

    button = gtk_check_button_new_with_label("Ignore labels in subcells");
    gtk_widget_set_name(button, "Labels");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fl_action_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    fl_nolabels = button;

    button = gtk_check_button_new_with_label("Use fast mode, NOT UNDOABLE");
    gtk_widget_set_name(button, "Mode");
    gtk_widget_show(button);
    GRX->SetStatus(button, fmode);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fl_action_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label(
        "Use object merging when flattening");
    gtk_widget_set_name(button, "Merge");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fl_action_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;
    fl_merge = button;

    //
    // flatten and dismiss buttons
    //
    row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    button = gtk_button_new_with_label("Flatten");
    gtk_widget_set_name(button, "Flatten");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fl_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    fl_go = button;

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(fl_cancel_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    gtk_window_set_focus(GTK_WINDOW(fl_popup), button);

    update();
}


sFlt::~sFlt()
{
    Flt = 0;
    if (fl_caller)
        GRX->Deselect(fl_caller);
    if (fl_callback)
        (*fl_callback)(0, false, 0, fl_arg);
    if (fl_popup)
        gtk_widget_destroy(fl_popup);
}


void
sFlt::update()
{
    GRX->SetStatus(fl_novias, CDvdb()->getVariable(VA_NoFlattenStdVias));
    GRX->SetStatus(fl_nopcells, CDvdb()->getVariable(VA_NoFlattenPCells));
    GRX->SetStatus(fl_nolabels, CDvdb()->getVariable(VA_NoFlattenLabels));
}


// Static function.
void
sFlt::fl_cancel_proc(GtkWidget*, void*)
{
    ED()->PopUpFlatten(0, MODE_OFF, 0, 0);
}


// Static function.
void
sFlt::fl_action_proc(GtkWidget *caller, void*)
{
    if (!Flt)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!strcmp(name, "Help"))
        DSPmainWbag(PopUpHelp("xic:flatn"))
    else if (!strcmp(name, "StdVias")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoFlattenStdVias, "");
        else
            CDvdb()->clearVariable(VA_NoFlattenStdVias);
    }
    else if (!strcmp(name, "PCells")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoFlattenPCells, "");
        else
            CDvdb()->clearVariable(VA_NoFlattenPCells);
    }
    else if (!strcmp(name, "Labels")) {
        if (GRX->GetStatus(caller))
            CDvdb()->setVariable(VA_NoFlattenLabels, "");
        else
            CDvdb()->clearVariable(VA_NoFlattenLabels);
    }
    else if (!strcmp(name, "Mode")) {
        if (Flt->fl_callback)
            (*Flt->fl_callback)("mode", GRX->GetStatus(caller), 0,
                Flt->fl_arg);
    }
    else if (!strcmp(name, "Merge")) {
        if (Flt->fl_callback)
            (*Flt->fl_callback)("merge", GRX->GetStatus(caller), 0,
                Flt->fl_arg);
    }
    else if (!strcmp(name, "Flatten")) {
        if (Flt->fl_callback)
            (*Flt->fl_callback)("flatten", true, 0, Flt->fl_arg);
    }
}


// Static function.
void
sFlt::fl_depth_proc(GtkWidget *caller, void*)
{
    if (!Flt)
        return;
    if (Flt->fl_callback) {
        char *str = gtk_combo_box_text_get_active_text(
            GTK_COMBO_BOX_TEXT(caller));
        if (str) {
            (*Flt->fl_callback)("depth", true, str, Flt->fl_arg);
            g_free(str);
        }
    }
}

