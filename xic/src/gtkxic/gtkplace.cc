
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
#include "promptline.h"
#include "select.h"
#include "pbtn_menu.h"
#include "gtkmain.h"
#include "gtkmenu.h"
#include "gtkinterf/gtkspinbtn.h"


//-----------------------------------------------------------------------------
// Popup Form for managing the placing of subcells.
//
// Help system keywords used:
//  placepanel

// From GDSII spec.
#define MAX_ARRAY 32767

// Default window size, assumes 6X13 chars, 48 cols
#define DEF_WIDTH 292

#define PL_NEW_CODE "_new$$entry_"

namespace {
    GtkTargetEntry target_table[] = {
        { (char*)"TWOSTRING",   0, 0 },
        { (char*)"CELLNAME",    0, 1 },
        { (char*)"STRING",      0, 2 },
        { (char*)"text/plain",  0, 3 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

    namespace gtkplace {
        // Spin button code.
        enum { PL_NX, PL_NY, PL_DX, PL_DY };

        struct sPlc : public cEdit::sPCpopup
        {
            sPlc(bool);
            ~sPlc();

            GtkWidget *shell() { return (pl_popup); }

            void update();

            // virtual overrides
            void rebuild_menu();
            void desel_placebtn();
            bool smash_mode();

            static void update_params()
                {
                    if (DSP()->CurMode() == Physical)
                        pl_iap = ED()->arrayParams();
                }

        private:
            void set_sens(bool);

            static int pl_pop_idle(void*);
            static void pl_cancel_proc(GtkWidget*, void*);
            static void pl_replace_proc(GtkWidget*, void*);
            static void pl_refmenu_proc(GtkWidget*, void*);
            static void pl_array_proc(GtkWidget*, void*);
            static void pl_array_set_proc(GtkWidget*, void*);
            static void pl_val_changed(GtkWidget*, void*);
            static void pl_place_proc(GtkWidget*, void*);
            static void pl_menu_place_proc(GtkWidget*, void*);
            static ESret pl_new_cb(const char*, void*);
            static void pl_menu_proc(GtkWidget*, void*);
            static void pl_help_proc(GtkWidget*, void*);
            static void pl_drag_data_received(GtkWidget*, GdkDragContext*,
                gint, gint, GtkSelectionData*, guint, guint time, void*);

            GtkWidget *pl_popup;
            GtkWidget *pl_arraybtn;
            GtkWidget *pl_replbtn;
            GtkWidget *pl_smshbtn;
            GtkWidget *pl_refmenu;

            GtkWidget *pl_label_nx;
            GtkWidget *pl_label_ny;
            GtkWidget *pl_label_dx;
            GtkWidget *pl_label_dy;

            GtkWidget *pl_masterbtn;
            GtkWidget *pl_placebtn;
            GtkWidget *pl_menu_placebtn;

            GRledPopup *pl_str_editor;
            char *pl_dropfile;

            GTKspinBtn sb_nx;
            GTKspinBtn sb_dx;
            GTKspinBtn sb_ny;
            GTKspinBtn sb_dy;
            GTKspinBtn sb_mmlen;

            static iap_t pl_iap;
        };

        sPlc *Plc;
    }
}

using namespace gtkplace;

iap_t sPlc::pl_iap;


// Main function to bring up the Placement Control pop-up.  If
// noprompt it true, don't prompt for a master if none is defined
//
void
cEdit::PopUpPlace(ShowMode mode, bool noprompt)
{
    if (!GTKdev::exists() || !GTKmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete Plc;
        return;
    }
    if (mode == MODE_UPD) {
        if (Plc)
            Plc->update();
        else
            sPlc::update_params();
        return;
    }
    if (Plc)
        return;

    new sPlc(noprompt);
    if (!Plc->shell()) {
        delete Plc;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Plc->shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GTKdev::self()->SetPopupLocation(GRloc(LW_UL), Plc->shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(Plc->shell());
    // give focus to main window
    GTKdev::SetFocus(GTKmainwin::self()->Shell());
}
// End of cEdit functions.


sPlc::sPlc(bool noprompt)
{
    Plc = this;
    pl_popup = 0;
    pl_arraybtn = 0;
    pl_replbtn = 0;
    pl_smshbtn = 0;
    pl_refmenu = 0;

    pl_label_nx = 0;
    pl_label_ny = 0;
    pl_label_dx = 0;
    pl_label_dy = 0;

    pl_masterbtn = 0;
    pl_placebtn = 0;
    pl_menu_placebtn = 0;
    pl_str_editor = 0;
    pl_dropfile = 0;

    ED()->plInitMenuLen();

    pl_popup = gtk_NewPopup(0, "Cell Placement Control", pl_cancel_proc, 0);
    if (!pl_popup)
        return;
    gtk_window_set_resizable(GTK_WINDOW(pl_popup), false);

    GtkWidget *form = gtk_table_new(1, 5, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(pl_popup), form);

    //
    // First row buttons.
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    GtkWidget *button = gtk_toggle_button_new_with_label("Use Array");
    gtk_widget_set_name(button, "UseArray");
    gtk_widget_show(button);
    pl_arraybtn = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(pl_array_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    button = gtk_toggle_button_new_with_label("Replace");
    gtk_widget_set_name(button, "Replace");
    gtk_widget_show(button);
    pl_replbtn = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(pl_replace_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    button = gtk_toggle_button_new_with_label("Smash");
    gtk_widget_set_name(button, "Smash");
    gtk_widget_show(button);
    pl_smshbtn = button;
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    pl_refmenu = gtk_combo_box_text_new();
    gtk_widget_set_name(pl_refmenu, "Ref");
    gtk_widget_show(pl_refmenu);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pl_refmenu),
        "Origin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pl_refmenu),
        "Lower Left");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pl_refmenu),
        "Upper Left");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pl_refmenu),
        "Upper Right");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pl_refmenu),
        "Lower Right");
    gtk_combo_box_set_active(GTK_COMBO_BOX(pl_refmenu),
        ED()->instanceRef());
    g_signal_connect(G_OBJECT(pl_refmenu), "changed",
        G_CALLBACK(pl_refmenu_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), pl_refmenu, false, false, 0);

    button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(pl_help_proc), 0);

    int rowcnt = 0;
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Array set labels and entries.
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    GtkWidget *vbox = gtk_vbox_new(false, 2);
    gtk_widget_show(vbox);

    GtkWidget *label = gtk_label_new("Nx");
    gtk_widget_show(label);
    pl_label_nx = label;
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), label, true, true, 0);

    GtkWidget *sb = sb_nx.init(1.0, 1.0, MAX_ARRAY, 0);
    sb_nx.connect_changed(G_CALLBACK(pl_array_set_proc), (void*)PL_NX,
        "Nx");
    gtk_box_pack_start(GTK_BOX(vbox), sb, false, false, 0);

    label = gtk_label_new("Ny");
    gtk_widget_show(label);
    pl_label_ny = label;
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), label, true, true, 0);

    sb = sb_ny.init(1.0, 1.0, MAX_ARRAY, 0);
    sb_ny.connect_changed(G_CALLBACK(pl_array_set_proc), (void*)PL_NY,
        "Ny");
    gtk_box_pack_start(GTK_BOX(vbox), sb, false, false, 0);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, true, true, 0);

    vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);

    int ndgt = CD()->numDigits();

    label = gtk_label_new("Dx");
    gtk_widget_show(label);
    pl_label_dx = label;
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), label, true, true, 0);

    sb = sb_dx.init(0.0, -1e6, 1e6, ndgt);
    sb_dx.connect_changed(G_CALLBACK(pl_array_set_proc), (void*)PL_DX,
        "Dx");
    gtk_box_pack_start(GTK_BOX(vbox), sb, true, true, 0);

    label = gtk_label_new("Dy");
    gtk_widget_show(label);
    pl_label_dy = label;
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), label, true, true, 0);

    sb = sb_dy.init(0.0, -1e6, 1e6, ndgt);
    sb_dy.connect_changed(G_CALLBACK(pl_array_set_proc), (void*)PL_DY,
        "Dy");
    gtk_box_pack_start(GTK_BOX(vbox), sb, true, true, 0);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Master selection option menu.
    //
    GtkWidget *opmenu = gtk_combo_box_text_new();
    gtk_widget_set_name(opmenu, "Master");
    gtk_widget_show(opmenu);
    pl_masterbtn = opmenu;
    rebuild_menu();

    gtk_table_attach(GTK_TABLE(form), opmenu, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Label and entry area for max menu length.
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    label = gtk_label_new("Maximum menu length");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);

    sb = sb_mmlen.init(ED()->plMenuLen(), 1.0, 75.0, 0);
    sb_mmlen.connect_changed(G_CALLBACK(pl_val_changed), 0, "mmlen");
    gtk_widget_set_size_request(sb, 80, -1);
    gtk_box_pack_end(GTK_BOX(hbox), sb, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    //
    // Place and dismiss buttons.
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    MenuEnt *m = MainMenu()->FindEntry(MMside, MenuPLACE);
    if (m)
        pl_menu_placebtn = (GtkWidget*)m->cmd.caller;
    if (pl_menu_placebtn) {
        // pl_menu_placebtn is the menu "place" button, which
        // will be "connected" to the Place button in the widget.

        button = gtk_toggle_button_new_with_label("Place");
        gtk_widget_set_name(button, "Place");
        gtk_widget_show(button);
        pl_placebtn = button;
        bool status = GTKdev::GetStatus(pl_menu_placebtn);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), status);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(pl_place_proc), 0);
        gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
        g_signal_connect(G_OBJECT(pl_menu_placebtn), "clicked",
            G_CALLBACK(pl_menu_place_proc), 0);
    }

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(pl_cancel_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    if (DSP()->CurMode() == Electrical)
        gtk_widget_set_sensitive(pl_arraybtn, false);
    set_sens(false);
    gtk_combo_box_set_active(GTK_COMBO_BOX(pl_refmenu),
        ED()->instanceRef());

    // Give focus to menu, otherwise cancel button may get focus, and
    // Enter (intending to change reference corner) will pop down.
    gtk_widget_set_can_focus(pl_masterbtn, true);
    gtk_window_set_focus(GTK_WINDOW(pl_popup), pl_masterbtn);

    // drop site
    gtk_drag_dest_set(pl_popup, GTK_DEST_DEFAULT_ALL, target_table,
        n_targets, GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(pl_popup), "drag-data-received",
        G_CALLBACK(pl_drag_data_received), 0);

    if (!ED()->plMenu() && !noprompt)
        // Positioning is incorrect unless an idle proc is used here.
        GTKpkg::self()->RegisterIdleProc(pl_pop_idle, 0);
}


sPlc::~sPlc()
{
    Plc = 0;
    if (GTKdev::GetStatus(pl_placebtn))
        // exit Place mode
        GTKdev::CallCallback(pl_menu_placebtn);
    // Turn off the replace and array functions
    ED()->setReplacing(false);
    ED()->setUseArray(false);
    int state = GTKdev::GetStatus(pl_arraybtn);
    if (state)
        ED()->setArrayParams(iap_t());
    if (pl_str_editor)
        pl_str_editor->popdown();
    if (pl_popup)
        gtk_widget_destroy(pl_popup);
}


void
sPlc::update()
{
    gtk_combo_box_set_active(GTK_COMBO_BOX(pl_refmenu),
        ED()->instanceRef());

    ED()->plInitMenuLen();
    const char *s = sb_mmlen.get_string();
    int n;
    if (sscanf(s, "%d", &n) != 1 || n != ED()->plMenuLen())
        sb_mmlen.set_value(ED()->plMenuLen());

    if (DSP()->CurMode() == Electrical) {
        iap_t iaptmp = pl_iap;
        sb_nx.set_value(1);
        sb_ny.set_value(1);
        sb_dx.set_value(0.0);
        sb_dy.set_value(0.0);
        pl_iap = iaptmp;
        ED()->setArrayParams(iap_t());
        GTKdev::SetStatus(pl_arraybtn, false);
        gtk_widget_set_sensitive(pl_arraybtn, false);
        set_sens(false);
    }
    else {
        pl_iap = ED()->arrayParams();
        gtk_widget_set_sensitive(pl_arraybtn, true);
        if (GTKdev::GetStatus(pl_arraybtn)) {
            sb_nx.set_value(pl_iap.nx());
            sb_ny.set_value(pl_iap.ny());
            sb_dx.set_value(MICRONS(pl_iap.spx()));
            sb_dy.set_value(MICRONS(pl_iap.spy()));
        }
    }
}


void
sPlc::desel_placebtn()
{
    GTKdev::Deselect(pl_placebtn);
}


bool
sPlc::smash_mode()
{
    return (GTKdev::GetStatus(pl_smshbtn));
}


// Set the order of names in the menu, and rebuild.
//
void
sPlc::rebuild_menu()
{
    if (!pl_masterbtn)
        return;
    g_signal_handlers_disconnect_by_func(G_OBJECT(pl_masterbtn),
        (gpointer)pl_menu_proc, (gpointer)0);
    gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(
        GTK_COMBO_BOX(pl_masterbtn))));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pl_masterbtn),
        "New");
    for (stringlist *p = ED()->plMenu(); p; p = p->next) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pl_masterbtn),
            p->string);
    }
    if (ED()->plMenu())
        gtk_combo_box_set_active(GTK_COMBO_BOX(pl_masterbtn), 1);
    else
        gtk_combo_box_set_active(GTK_COMBO_BOX(pl_masterbtn), 0);
    g_signal_connect(G_OBJECT(pl_masterbtn), "changed",
        G_CALLBACK(pl_menu_proc), 0);
}


// Set the sensitivity of the array parameter entry widgets
//
void
sPlc::set_sens(bool set)
{
    gtk_widget_set_sensitive(pl_label_nx, set);
    sb_nx.set_sensitive(set);
    gtk_widget_set_sensitive(pl_label_ny, set);
    sb_ny.set_sensitive(set);
    gtk_widget_set_sensitive(pl_label_dx, set);
    sb_dx.set_sensitive(set);
    gtk_widget_set_sensitive(pl_label_dy, set);
    sb_dy.set_sensitive(set);
}


// Static function.
int
sPlc::pl_pop_idle(void*)
{
    pl_menu_proc(0, 0);
    return (0);
}


// Static function.
// Cancel callback.  Simply pop it down.
//
void
sPlc::pl_cancel_proc(GtkWidget*, void*)
{
    ED()->PopUpPlace(MODE_OFF, false);
}


// Static function.
// Replace callback.  When the replace command is active, pointing at
// existing instances will cause them to be replaced by the current
// master.  The current transform is added to the existing transform.
//
void
sPlc::pl_replace_proc(GtkWidget *caller, void*)
{
    if (GTKdev::GetStatus(caller)) {
        ED()->setReplacing(true);
        PL()->ShowPrompt("Select subcells to replace, press Esc to quit.");
        if (Plc) {
            GTKdev::SetStatus(Plc->pl_smshbtn, false);
            gtk_widget_set_sensitive(Plc->pl_smshbtn, false);
        }
    }
    else {
        ED()->setReplacing(false);
        PL()->ShowPrompt(
            "Click on locations to place cell, press Esc to quit.");
        if (Plc) {
            gtk_widget_set_sensitive(Plc->pl_smshbtn, true);
            if (DSP()->CurMode() == Physical)
                gtk_widget_set_sensitive(Plc->pl_arraybtn, true);
        }
    }
}


// Static function.
// The reference point of the cell, i.e., the point that is located at
// the cursor, is switchable between the corners of the untransformed
// cell, or the origin of the cell, in physical mode.
//
void
sPlc::pl_refmenu_proc(GtkWidget *caller, void*)
{
    int i = gtk_combo_box_get_active(GTK_COMBO_BOX(caller));
    if (i < 0)
        return;
    if (i == 0)
        ED()->setInstanceRef(PL_ORIGIN);
    else if (i == 1)
        ED()->setInstanceRef(PL_LL);
    else if (i == 2)
        ED()->setInstanceRef(PL_UL);
    else if (i == 3)
        ED()->setInstanceRef(PL_UR);
    else if (i == 4)
        ED()->setInstanceRef(PL_LR);
}


// Static function.
// When on, cells placed can be arrayed.  Otherwise only a single
// instance is created.
//
void
sPlc::pl_array_proc(GtkWidget *caller, void*)
{
    if (!Plc)
        return;
    int state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(caller));
    if (state) {
        Plc->sb_nx.set_value(Plc->pl_iap.nx());
        Plc->sb_ny.set_value(Plc->pl_iap.ny());
        Plc->sb_dx.set_value(MICRONS(Plc->pl_iap.spx()));
        Plc->sb_dy.set_value(MICRONS(Plc->pl_iap.spy()));
        Plc->set_sens(true);
        ED()->setUseArray(true);
    }
    else {
        iap_t iaptmp = pl_iap;
        Plc->sb_nx.set_value(1);
        Plc->sb_ny.set_value(1);
        Plc->sb_dx.set_value(0.0);
        Plc->sb_dy.set_value(0.0);
        pl_iap = iaptmp;
        Plc->set_sens(false);
        ED()->setUseArray(false);
    }
}


// Static function.
void
sPlc::pl_array_set_proc(GtkWidget*, void *arg)
{
    if (!Plc)
        return;
    int code = (intptr_t)arg;
    if (code == PL_NX)
        pl_iap.set_nx(Plc->sb_nx.get_value_as_int());
    else if (code == PL_NY)
        pl_iap.set_ny(Plc->sb_ny.get_value_as_int());
    else if (code == PL_DX)
        pl_iap.set_spx(INTERNAL_UNITS(Plc->sb_dx.get_value()));
    else if (code == PL_DY)
        pl_iap.set_spy(INTERNAL_UNITS(Plc->sb_dy.get_value()));

    if (GTKmainwin::self()) {
        GTKmainwin::self()->ShowGhost(ERASE);
        ED()->setArrayParams(pl_iap);
        GTKmainwin::self()->ShowGhost(DISPLAY);
    }
}


// Static function.
void
sPlc::pl_val_changed(GtkWidget *caller, void*)
{
    if (!Plc)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "mmlen")) {
        const char *s = Plc->sb_mmlen.get_string();
        int n;
        if (sscanf(s, "%d", &n) == 1 && n >= 1 && n <= 75) {
            if (n == DEF_PLACE_MENU_LEN)
                CDvdb()->clearVariable(VA_MasterMenuLength);
            else
                CDvdb()->setVariable(VA_MasterMenuLength, s);
        }
    }
}


// Static function.
void
sPlc::pl_place_proc(GtkWidget*, void*)
{
    if (!Plc)
        return;
    bool status = GTKdev::GetStatus(Plc->pl_placebtn);
    bool mbstatus = GTKdev::GetStatus(Plc->pl_menu_placebtn);
    if (status != mbstatus)
        GTKdev::CallCallback(Plc->pl_menu_placebtn);
}


// Static function.
void
sPlc::pl_menu_place_proc(GtkWidget*, void*)
{
    if (!Plc)
        return;
    bool status = GTKdev::GetStatus(Plc->pl_menu_placebtn);
    if (status)
        GTKdev::Select(Plc->pl_placebtn);
    else
        GTKdev::Deselect(Plc->pl_placebtn);
}


// Static function.
ESret
sPlc::pl_new_cb(const char *string, void*)
{
    if (string && *string) {
        // If two tokens, the first if a library or archive file name,
        // the second is the cell name.

        char *aname = lstring::getqtok(&string);
        char *cname = lstring::getqtok(&string);
        ED()->addMaster(aname, cname);
        delete [] aname;
        delete [] cname;
    }
    // give focus to main window
    GTKdev::SetFocus(GTKmainwin::self()->Shell());
    return (ESTR_DN);
}


// Static function.
void
sPlc::pl_menu_proc(GtkWidget *caller, void*)
{
    if (!Plc)
        return;
    int i = 0;
    if (caller) {
        i = gtk_combo_box_get_active(GTK_COMBO_BOX(caller));
        if (i < 0)
            return;
    }
    if (i == 0) {
        if (GTKdev::GetStatus(Plc->pl_placebtn))
            GTKdev::CallCallback(Plc->pl_menu_placebtn);
        char *dfile = Plc->pl_dropfile;
        Plc->pl_dropfile = 0;
        GCarray<char*> gc_dfile(dfile);
        const char *defname = dfile;
        if (!defname || !*defname) {
            defname = XM()->GetCurFileSelection();
            if (!defname) {
                CDc *cd = (CDc*)Selections.firstObject(CurCell(), "c");
                if (cd)
                    defname = Tstring(cd->cellname());
            }
        }
        if (Plc->pl_str_editor)
            Plc->pl_str_editor->popdown();
        int x, y;
        GTKdev::self()->Location(Plc->pl_masterbtn, &x, &y);
        Plc->pl_str_editor = DSPmainWbagRet(PopUpEditString(0,
            GRloc(LW_XYA, x - 200, y), "File or cell name of cell to place?",
            defname, pl_new_cb, 0, 200, 0));
        if (Plc->pl_str_editor)
            Plc->pl_str_editor->register_usrptr((void**)&Plc->pl_str_editor);
    }
    else {
        char *tok = gtk_combo_box_text_get_active_text(
            GTK_COMBO_BOX_TEXT(caller));
        char *string = tok;
        char *aname = lstring::getqtok(&string);
        char *cname = lstring::getqtok(&string);
        ED()->addMaster(aname, cname);
        delete [] aname;
        delete [] cname;
        g_free(tok);
    }
}


// Static function.
void
sPlc::pl_help_proc(GtkWidget*, void*)
{
    DSPmainWbag(PopUpHelp("placepanel"))
}


// Static function.
// Receive drop data (a path name).
//
void
sPlc::pl_drag_data_received(GtkWidget*, GdkDragContext *context, gint, gint,
    GtkSelectionData *data, guint, guint time, void*)
{
    if (gtk_selection_data_get_length(data) >= 0 &&
            gtk_selection_data_get_format(data) == 8 &&
            gtk_selection_data_get_data(data)) {
        char *src = (char*)gtk_selection_data_get_data(data);

        // The "filename" can actually be two space-separated tokens,
        // the first being an archive file of CHD name, the second being
        // a cell name.
        if (gtk_selection_data_get_target(data) ==
                gdk_atom_intern("TWOSTRING", true)) {
            char *t = strchr(src, '\n');
            if (t)
                *t = ' ';
        }

        delete [] Plc->pl_dropfile;
        Plc->pl_dropfile = 0;
        if (Plc->pl_str_editor) {
            Plc->pl_str_editor->update(0, src);
            gtk_drag_finish(context, true, false, time);
            return;
        }
        Plc->pl_dropfile = lstring::copy(src);
        pl_menu_proc(0, 0);
    }
    gtk_drag_finish(context, false, false, time);
}
// End of sPlc functions.

