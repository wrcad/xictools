
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fl_action_proc), 0);
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
    GtkWidget *entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "Depth");
    gtk_widget_show(entry);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "Depth");

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
        GtkWidget *mi = gtk_menu_item_new_with_label(buf);
        gtk_widget_set_name(mi, buf);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(fl_depth_proc), 0);
    }

    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(entry), depth);
    gtk_box_pack_start(GTK_BOX(row), entry, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), row, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // check boxes
    //
    button = gtk_check_button_new_with_label("Use fast mode, NOT UNDOABLE");
    gtk_widget_set_name(button, "Mode");
    gtk_widget_show(button);
    GRX->SetStatus(button, fmode);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fl_action_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    button = gtk_check_button_new_with_label(
        "Use object merging when flattening");
    gtk_widget_set_name(button, "Merge");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fl_action_proc), 0);
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
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fl_action_proc), 0);
    gtk_box_pack_start(GTK_BOX(row), button, true, true, 0);
    fl_go = button;

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(fl_cancel_proc), 0);
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
    if (Flt->fl_callback)
        (*Flt->fl_callback)("depth", true, gtk_widget_get_name(caller),
            Flt->fl_arg);
}

