
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
#include "dsp_inlines.h"
#include "cd_strmdata.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "layertab.h"


//--------------------------------------------------------------------
// Layer Editor pop-up
//
// Help system keywords used:
//  xic:edlyr

namespace {
    namespace gtkltedit {
        struct gtkLcb : public sLcb
        {
            gtkLcb(GRobject);
            virtual ~gtkLcb();

            GtkWidget *Shell() { return (le_shell); }

            // virtual overrides
            void update(CDll*);
            char *layername();
            void desel_rem();
            void popdown();

        private:
            static void le_popdown(GtkWidget*, void*);
            static void le_btn_proc(GtkWidget*, void*);
            static char *le_get_lname(GtkWidget*, GtkWidget*);

            GRobject le_caller;     // initiating button

            GtkWidget *le_shell;    // pop-up shell
            GtkWidget *le_add;      // add layer button
            GtkWidget *le_rem;      // remove layer button
            GtkWidget *le_opmenu;   // removed layers menu;

            static const char *initmsg;
        };
    }
}

using namespace gtkltedit;

const char *gtkLcb::initmsg = "Layer Editor -- add or remove layers.";


// Pop up the Layer editor.  The editor has buttons to add a layer, remove
// layers, and a combo box for layer name entry.  The combo contains a
// list of previously removed layers.
//
sLcb *
cMain::PopUpLayerEditor(GRobject c)
{
    if (!GRX || !mainBag())
        return (0);
    gtkLcb *cbs = new gtkLcb(c);
    if (!cbs->Shell()) {
        delete cbs;
        return (0);
    }
    gtk_window_set_transient_for(GTK_WINDOW(cbs->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), cbs->Shell(), mainBag()->Viewport());
    gtk_widget_show(cbs->Shell());
    return (cbs);
}
// End of cMain functions.


gtkLcb::gtkLcb(GRobject c)
{
    le_caller = c;
    le_shell = 0;
    le_add = 0;
    le_rem = 0;
    le_opmenu = 0;

    le_shell = gtk_NewPopup(mainBag(), "Layer Editor", le_popdown, this);
    if (!le_shell)
        return;
    gtk_window_set_resizable(GTK_WINDOW(le_shell), false);

    GtkWidget *form = gtk_table_new(1, 4, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(le_shell), form);

    const char *prompt_str = gtkLcb::initmsg;
    //
    // label in frame
    //
    GtkWidget *row = gtk_hbox_new(false, 2);
    gtk_widget_show(row);
    GtkWidget *label = gtk_label_new(prompt_str);
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
        GTK_SIGNAL_FUNC(le_btn_proc), this);
    gtk_box_pack_end(GTK_BOX(row), button, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), row, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_object_set_data(GTK_OBJECT(le_shell), "label", label);

    char *init_str = 0;
    //
    // combo box input area
    //
    GtkWidget *combo = gtk_combo_new();
    gtk_widget_show(combo);
    gtk_table_attach(GTK_TABLE(form), combo, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);
    GtkWidget *text = GTK_COMBO(combo)->entry;
    if (init_str)
        gtk_entry_set_text(GTK_ENTRY(text), init_str);
    gtk_object_set_data(GTK_OBJECT(le_shell), "text", text);
    le_opmenu = GTK_COMBO(combo)->list;

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // buttons
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    button = gtk_toggle_button_new_with_label("Add Layer");
    gtk_widget_set_name(button, "Add Layer");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gtkLcb::le_btn_proc), this);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
    le_add = button;

    button = gtk_toggle_button_new_with_label("Remove Layer");
    gtk_widget_set_name(button, "Remove Layer");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gtkLcb::le_btn_proc), this);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
    le_rem = button;

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(gtkLcb::le_popdown), this);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    gtk_window_set_focus(GTK_WINDOW(le_shell), text);
}


gtkLcb::~gtkLcb()
{
}


// Update the list of removed layers.
//
void
gtkLcb::update(CDll *list)
{
    gtk_list_clear_items(GTK_LIST(le_opmenu), 0, -1);
    if (!list) {
        GtkWidget *text =
            (GtkWidget*)gtk_object_get_data(GTK_OBJECT(le_shell), "text");
        if (text)
            gtk_entry_set_text(GTK_ENTRY(text), "");
    }
    for (CDll *l = list; l; l = l->next) {
        GtkWidget *item = gtk_list_item_new_with_label(l->ldesc->name());
        gtk_widget_show(item);
        gtk_container_add(GTK_CONTAINER(le_opmenu), item);
    }
}


// Return the current name in the text box.  The return value should be
// freed.  If the value isn't good, 0 is returned.  This is called when
// adding layers.
//
char *
gtkLcb::layername()
{
    GtkWidget *text =
        (GtkWidget*)gtk_object_get_data(GTK_OBJECT(le_shell), "text");
    if (!text)
        return (0);
    GtkWidget *label =
        (GtkWidget*)gtk_object_get_data(GTK_OBJECT(le_shell), "label");
    char *string = le_get_lname(text, label);
    if (!string) {
        GRX->Deselect(le_add);
        add_cb(false);
        return (0);
    }
    return (string);
}


// Deselect the Remove button.
//
void
gtkLcb::desel_rem()
{
    GRX->Deselect(le_rem);
}


// Pop down the widget (from the caller).
//
void
gtkLcb::popdown()
{
    le_popdown(0, this);
}


// Static function.
// Cancel widget callback
//
void
gtkLcb::le_popdown(GtkWidget*, void *client_data)
{
    gtkLcb *cbs = (gtkLcb*)client_data;
    if (!cbs)
        return;
    if (cbs->le_caller)
        GRX->Deselect(cbs->le_caller);
    if (cbs->le_shell) {
        gtk_signal_disconnect_by_func(GTK_OBJECT(cbs->le_shell),
            GTK_SIGNAL_FUNC(le_popdown), cbs);
        gtk_widget_destroy(GTK_WIDGET(cbs->le_shell));
        cbs->le_shell = 0;
    }
    cbs->quit_cb();
}


// Static function.
// Button press handler.
//
void
gtkLcb::le_btn_proc(GtkWidget *caller, void *client_data)
{
    gtkLcb *cbs = (gtkLcb*)client_data;
    if (!cbs)
        return;
    if (caller == cbs->le_add) {
        GtkWidget *label =
            (GtkWidget*)gtk_object_get_data(GTK_OBJECT(cbs->le_shell), "label");
        if (!label)
            return;
        if (!GRX->GetStatus(caller)) {
            gtk_label_set_text(GTK_LABEL(label), initmsg);
            cbs->add_cb(false);
            return;
        }
        GRX->Deselect(cbs->le_rem);
        GtkWidget *text =
            (GtkWidget*)gtk_object_get_data(GTK_OBJECT(cbs->le_shell), "text");
        if (!text)
            return;
        char *string = le_get_lname(text, label);
        if (!string) {
            GRX->Deselect(caller);
            return;
        }
        delete [] string;
        gtk_label_set_text(GTK_LABEL(label),
            "Click where new layer is to be added.");
        cbs->add_cb(true);
    }
    else if (caller == cbs->le_rem) {
        GtkWidget *label =
            (GtkWidget*)gtk_object_get_data(GTK_OBJECT(cbs->le_shell), "label");
        if (!label)
            return;
        if (!GRX->GetStatus(caller)) {
            gtk_label_set_text(GTK_LABEL(label), initmsg);
            cbs->rem_cb(false);
            return;
        }
        bool state;
        if (DSP()->CurMode() == Electrical)
            state = (CDldb()->layer(2, Electrical) != 0);
        else
            state = (CDldb()->layer(1, Physical) != 0);
        if (!state) {
            gtk_label_set_text(GTK_LABEL(label), "No removable layers left.");
            GRX->Deselect(cbs->le_rem);
            return;
        }
        GRX->Deselect(cbs->le_add);
        gtk_label_set_text(GTK_LABEL(label), "Click on layers to remove.");
        cbs->rem_cb(true);
    }
    else {
        const char *name = gtk_widget_get_name(caller);
        if (name && !strcmp(name, "Help")) {
            DSPmainWbag(PopUpHelp("xic:edlyr"))
        }
    }
}


// Static function.
// Return the text input, or 0 if no good.
//
char *
gtkLcb::le_get_lname(GtkWidget *text, GtkWidget *label)
{
    const char *string = gtk_entry_get_text(GTK_ENTRY(text));
    if (!string || !*string) {
        if (label)
            gtk_label_set_text(GTK_LABEL(label),
                "No name entered.  Enter a layer name:");
        return (0);
    }
    char *lname = lstring::copy(string);
    if (CDldb()->findLayer(lname, DSP()->CurMode())) {
        if (label) {
            char buf[256];
            sprintf(buf, "A layer %s already exists.  Enter a new name:",
                lname);
            gtk_label_set_text(GTK_LABEL(label), buf);
        }
        delete [] lname;
        return (0);
    }
    return (lname);
}

