
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
#include "dsp_inlines.h"
#include "layertab.h"
#include "menu.h"
#include "select.h"
#include "gtkmain.h"
#include "gtkltab.h"
#include "gtkinlines.h"


//-----------------------------------------------------------------------------
// Pop-up to control selections
//
// help system keywords used:
//  xic:layer

namespace {
    namespace gtksel {
        struct sSel : public gtk_bag
        {
            sSel(GRobject);
            ~sSel();

            void update();

        private:
            static void sl_cancel_proc(GtkWidget*, void*);
            static void sl_ptr_mode_proc(GtkWidget*, void*);
            static void sl_area_mode_proc(GtkWidget*, void*);
            static void sl_add_mode_proc(GtkWidget*, void*);
            static void sl_obj_proc(GtkWidget*, void*);
            static void sl_btn_proc(GtkWidget*, void*);

            GRobject sl_caller;
            GtkWidget *sl_pm_norm;
            GtkWidget *sl_pm_sel;
            GtkWidget *sl_pm_mod;
            GtkWidget *sl_am_norm;
            GtkWidget *sl_am_enc;
            GtkWidget *sl_am_all;
            GtkWidget *sl_sel_norm;
            GtkWidget *sl_sel_togl;
            GtkWidget *sl_sel_add;
            GtkWidget *sl_sel_rem;
            GtkWidget *sl_cell;
            GtkWidget *sl_box;
            GtkWidget *sl_poly;
            GtkWidget *sl_wire;
            GtkWidget *sl_label;
            GtkWidget *sl_upbtn;
        };

        sSel *Sel;
    }
}

using namespace gtksel;


void
cMain::PopUpSelectControl(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Sel;
        return;
    }
    if (mode == MODE_UPD) {
        if (Sel)
            Sel->update();
        return;
    }
    if (Sel)
        return;

    new sSel(caller);
    if (!Sel->Shell()) {
        delete Sel;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Sel->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_LL), Sel->Shell(), mainBag()->Viewport());
    gtk_widget_show(Sel->Shell());

    // Bug in OpenSuse 13.1 gtk-2.24.23
    GRX->SetPopupLocation(GRloc(LW_LL), Sel->Shell(), mainBag()->Viewport());
}
// End of cMain functions.


sSel::sSel(GRobject c)
{
    Sel = this;
    sl_caller = c;
    sl_pm_norm = 0;
    sl_pm_sel = 0;
    sl_pm_mod = 0;
    sl_am_norm = 0;
    sl_am_enc = 0;
    sl_am_all = 0;
    sl_sel_norm = 0;
    sl_sel_togl = 0;
    sl_sel_add = 0;
    sl_sel_rem = 0;
    sl_cell = 0;
    sl_box = 0;
    sl_poly = 0;
    sl_wire = 0;
    sl_label = 0;
    sl_upbtn = 0;

    wb_shell = gtk_NewPopup(0, "Selection Control", sl_cancel_proc, 0);
    if (!wb_shell)
        return;
    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    //
    // pointer mode radio group
    //
    GtkWidget *vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);
    sl_pm_norm = gtk_radio_button_new_with_label(0, "Normal");
    gtk_widget_set_name(sl_pm_norm, "Normal");
    gtk_widget_show(sl_pm_norm);
    GSList *group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(sl_pm_norm));
    gtk_box_pack_start(GTK_BOX(vbox), sl_pm_norm, true, false, 0);
    g_signal_connect(G_OBJECT(sl_pm_norm), "clicked",
        G_CALLBACK(sl_ptr_mode_proc), (void*)PTRnormal);

    sl_pm_sel = gtk_radio_button_new_with_label(group, "Select");
    gtk_widget_set_name(sl_pm_sel, "Select");
    gtk_widget_show(sl_pm_sel);
    gtk_box_pack_start(GTK_BOX(vbox), sl_pm_sel, true, false, 0);
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(sl_pm_sel));
    g_signal_connect(G_OBJECT(sl_pm_sel), "clicked",
        G_CALLBACK(sl_ptr_mode_proc), (void*)PTRselect);

    sl_pm_mod = gtk_radio_button_new_with_label(group, "Modify");
    gtk_widget_set_name(sl_pm_mod, "Modify");
    gtk_widget_show(sl_pm_mod);
    gtk_box_pack_start(GTK_BOX(vbox), sl_pm_mod, true, false, 0);
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(sl_pm_mod));
    g_signal_connect(G_OBJECT(sl_pm_mod), "clicked",
        G_CALLBACK(sl_ptr_mode_proc), (void*)PTRmodify);

    GtkWidget *frame = gtk_frame_new("Pointer Mode");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // area mode radio group
    //
    vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);
    sl_am_norm = gtk_radio_button_new_with_label(0, "Normal");
    gtk_widget_set_name(sl_am_norm, "Normal");
    gtk_widget_show(sl_am_norm);
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(sl_am_norm));
    gtk_box_pack_start(GTK_BOX(vbox), sl_am_norm, true, false, 0);
    g_signal_connect(G_OBJECT(sl_am_norm), "clicked",
        G_CALLBACK(sl_area_mode_proc), (void*)ASELnormal);

    sl_am_enc = gtk_radio_button_new_with_label(group, "Enclosed");
    gtk_widget_set_name(sl_am_enc, "Enclosed");
    gtk_widget_show(sl_am_enc);
    gtk_box_pack_start(GTK_BOX(vbox), sl_am_enc, true, false, 0);
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(sl_am_enc));
    g_signal_connect(G_OBJECT(sl_am_enc), "clicked",
        G_CALLBACK(sl_area_mode_proc), (void*)ASELenclosed);

    sl_am_all = gtk_radio_button_new_with_label(group, "All");
    gtk_widget_set_name(sl_am_all, "All");
    gtk_widget_show(sl_am_all);
    gtk_box_pack_start(GTK_BOX(vbox), sl_am_all, true, false, 0);
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(sl_am_all));
    g_signal_connect(G_OBJECT(sl_am_all), "clicked",
        G_CALLBACK(sl_area_mode_proc), (void*)ASELall);

    frame = gtk_frame_new("Area Mode");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_table_attach(GTK_TABLE(form), frame, 1, 2, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // addition mode radio group
    //
    vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);
    sl_sel_norm = gtk_radio_button_new_with_label(0, "Normal");
    gtk_widget_set_name(sl_sel_norm, "Normal");
    gtk_widget_show(sl_sel_norm);
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(sl_sel_norm));
    gtk_box_pack_start(GTK_BOX(vbox), sl_sel_norm, true, false, 0);
    g_signal_connect(G_OBJECT(sl_sel_norm), "clicked",
        G_CALLBACK(sl_add_mode_proc), (void*)SELnormal);

    sl_sel_togl = gtk_radio_button_new_with_label(group, "Toggle");
    gtk_widget_set_name(sl_sel_togl, "Toggle");
    gtk_widget_show(sl_sel_togl);
    gtk_box_pack_start(GTK_BOX(vbox), sl_sel_togl, true, false, 0);
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(sl_sel_togl));
    g_signal_connect(G_OBJECT(sl_sel_togl), "clicked",
        G_CALLBACK(sl_add_mode_proc), (void*)SELtoggle);

    sl_sel_add = gtk_radio_button_new_with_label(group, "Add");
    gtk_widget_set_name(sl_sel_add, "Add");
    gtk_widget_show(sl_sel_add);
    gtk_box_pack_start(GTK_BOX(vbox), sl_sel_add, true, false, 0);
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(sl_sel_add));
    g_signal_connect(G_OBJECT(sl_sel_add), "clicked",
        G_CALLBACK(sl_add_mode_proc), (void*)SELselect);

    sl_sel_rem = gtk_radio_button_new_with_label(group, "Remove");
    gtk_widget_set_name(sl_sel_rem, "Remove");
    gtk_widget_show(sl_sel_rem);
    gtk_box_pack_start(GTK_BOX(vbox), sl_sel_rem, true, false, 0);
    group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(sl_sel_rem));
    g_signal_connect(G_OBJECT(sl_sel_rem), "clicked",
        G_CALLBACK(sl_add_mode_proc), (void*)SELdesel);

    frame = gtk_frame_new("Selections");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_table_attach(GTK_TABLE(form), frame, 2, 3, 0, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // objects group
    //
    vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);
    sl_cell = gtk_check_button_new_with_label("Cells");
    gtk_widget_set_name(sl_cell, "Cells");
    gtk_widget_show(sl_cell);
    gtk_box_pack_start(GTK_BOX(vbox), sl_cell, true, false, 0);
    g_signal_connect(G_OBJECT(sl_cell), "clicked",
        G_CALLBACK(sl_obj_proc), (void*)CDINSTANCE);

    sl_box = gtk_check_button_new_with_label("Boxes");
    gtk_widget_set_name(sl_box, "Boxes");
    gtk_widget_show(sl_box);
    gtk_box_pack_start(GTK_BOX(vbox), sl_box, true, false, 0);
    g_signal_connect(G_OBJECT(sl_box), "clicked",
        G_CALLBACK(sl_obj_proc), (void*)CDBOX);

    sl_poly = gtk_check_button_new_with_label("Polys");
    gtk_widget_set_name(sl_poly, "Polys");
    gtk_widget_show(sl_poly);
    gtk_box_pack_start(GTK_BOX(vbox), sl_poly, true, false, 0);
    g_signal_connect(G_OBJECT(sl_poly), "clicked",
        G_CALLBACK(sl_obj_proc), (void*)CDPOLYGON);

    sl_wire = gtk_check_button_new_with_label("Wires");
    gtk_widget_set_name(sl_wire, "Wires");
    gtk_widget_show(sl_wire);
    gtk_box_pack_start(GTK_BOX(vbox), sl_wire, true, false, 0);
    g_signal_connect(G_OBJECT(sl_wire), "clicked",
        G_CALLBACK(sl_obj_proc), (void*)CDWIRE);

    sl_label = gtk_check_button_new_with_label("Labels");
    gtk_widget_set_name(sl_label, "Labels");
    gtk_widget_show(sl_label);
    gtk_box_pack_start(GTK_BOX(vbox), sl_label, true, false, 0);
    g_signal_connect(G_OBJECT(sl_label), "clicked",
        G_CALLBACK(sl_obj_proc), (void*)CDLABEL);

    frame = gtk_frame_new("Objects");
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_table_attach(GTK_TABLE(form), frame, 3, 4, 0, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // buttons
    //
    GtkWidget *button = gtk_toggle_button_new_with_label(
        "Search Bottom to Top");
    sl_upbtn = button;
    gtk_widget_set_name(button, "Up");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(sl_btn_proc), (void*)0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(sl_btn_proc), (void*)1);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(sl_cancel_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 3, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    update();
}


sSel::~sSel()
{
    Sel = 0;
    if (sl_caller)
        GRX->Deselect(sl_caller);
    if (wb_shell) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(wb_shell),
            (gpointer)sl_cancel_proc, wb_shell);
    }
}


void
sSel::update()
{
    if (Selections.ptrMode() == PTRnormal) {
        GRX->Select(sl_pm_norm);
        GRX->Deselect(sl_pm_sel);
        GRX->Deselect(sl_pm_mod);
    }
    else if (Selections.ptrMode() == PTRselect) {
        GRX->Deselect(sl_pm_norm);
        GRX->Select(sl_pm_sel);
        GRX->Deselect(sl_pm_mod);
    }
    else if (Selections.ptrMode() == PTRmodify) {
        GRX->Deselect(sl_pm_norm);
        GRX->Deselect(sl_pm_sel);
        GRX->Select(sl_pm_mod);
    }

    if (Selections.areaMode() == ASELnormal) {
        GRX->Select(sl_am_norm);
        GRX->Deselect(sl_am_enc);
        GRX->Deselect(sl_am_all);
    }
    else if (Selections.areaMode() == ASELenclosed) {
        GRX->Deselect(sl_am_norm);
        GRX->Select(sl_am_enc);
        GRX->Deselect(sl_am_all);
    }
    else if (Selections.areaMode() == ASELall) {
        GRX->Deselect(sl_am_norm);
        GRX->Deselect(sl_am_enc);
        GRX->Select(sl_am_all);
    }

    if (Selections.selMode() == SELnormal) {
        GRX->Select(sl_sel_norm);
        GRX->Deselect(sl_sel_togl);
        GRX->Deselect(sl_sel_add);
        GRX->Deselect(sl_sel_rem);
    }
    if (Selections.selMode() == SELtoggle) {
        GRX->Deselect(sl_sel_norm);
        GRX->Select(sl_sel_togl);
        GRX->Deselect(sl_sel_add);
        GRX->Deselect(sl_sel_rem);
    }
    if (Selections.selMode() == SELselect) {
        GRX->Deselect(sl_sel_norm);
        GRX->Deselect(sl_sel_togl);
        GRX->Select(sl_sel_add);
        GRX->Deselect(sl_sel_rem);
    }
    if (Selections.selMode() == SELdesel) {
        GRX->Deselect(sl_sel_norm);
        GRX->Deselect(sl_sel_togl);
        GRX->Deselect(sl_sel_add);
        GRX->Select(sl_sel_rem);
    }

    GRX->SetStatus(sl_cell,
        (strchr(Selections.selectTypes(), CDINSTANCE) != 0));
    GRX->SetStatus(sl_box,
        (strchr(Selections.selectTypes(), CDBOX) != 0));
    GRX->SetStatus(sl_poly,
        (strchr(Selections.selectTypes(), CDPOLYGON) != 0));
    GRX->SetStatus(sl_wire,
        (strchr(Selections.selectTypes(), CDWIRE) != 0));
    GRX->SetStatus(sl_label,
        (strchr(Selections.selectTypes(), CDLABEL) != 0));

    GRX->SetStatus(sl_upbtn, Selections.layerSearchUp());
}


// Static function.
void
sSel::sl_cancel_proc(GtkWidget*, void*)
{
    XM()->PopUpSelectControl(0, MODE_OFF);
}


// Static function.
void
sSel::sl_ptr_mode_proc(GtkWidget *widget, void *arg)
{
    if (GRX->GetStatus(widget))
        Selections.setPtrMode((PTRmode)(intptr_t)(arg));
}


// Static function.
void
sSel::sl_area_mode_proc(GtkWidget *widget, void *arg)
{
    if (GRX->GetStatus(widget))
        Selections.setAreaMode((ASELmode)(intptr_t)(arg));
}


// Static function.
void
sSel::sl_add_mode_proc(GtkWidget *widget, void *arg)
{
    if (GRX->GetStatus(widget))
        Selections.setSelMode((SELmode)(intptr_t)(arg));
}


// Static function.
void
sSel::sl_obj_proc(GtkWidget *widget, void *arg)
{
    char c = (intptr_t)arg;
    bool state = GRX->GetStatus(widget);
    Selections.setSelectType(c, state);
}


// Static function.
void
sSel::sl_btn_proc(GtkWidget *widget, void *arg)
{
    if (arg == (void*)0) {
        // search up
        XM()->SetLayerSearchUpSelections(GRX->GetStatus(widget));
    }
    else if (arg == (void*)1) {
        // help
        DSPmainWbag(PopUpHelp("xic:layer"))
    }
}

