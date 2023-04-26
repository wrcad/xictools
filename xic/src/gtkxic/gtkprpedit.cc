
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
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "menu.h"
#include "edit_menu.h"
#include "pbtn_menu.h"
#include "events.h"
#include "promptline.h"
#include "gtkmain.h"
#include "gtkprpty.h"


//--------------------------------------------------------------------------
// Pop up to modify proerties of objects
//
// Help system keywords used:
//  prppanel

namespace {
    GtkTargetEntry target_table[] = {
        { (char*)"property",     0, 0 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

    namespace gtkprpedit {

        struct sAddEnt
        {
            sAddEnt(const char *n, int v)   { name = n; value = v; }

            const char *name;
            int value;
        };

        struct sPo : public sPbase
        {
            sPo(CDo*, PRPmode);
            ~sPo();

            void update(CDo*, PRPmode);
            void purge(CDo*, CDo*);
            PrptyText *select(int);
            PrptyText *cycle(CDp*, bool(*)(const CDp*), bool);
            void set_btn_callback(int(*)(PrptyText*));

        private:
            void activate(bool);
            void call_prpty_add(int);
            void call_prpty_edit(PrptyText*);
            void call_prpty_del(PrptyText*);

            static void po_font_changed();
            static void po_cancel_proc(GtkWidget*, void*);
            static void po_action_proc(GtkWidget*, void*);
            static void po_menu_proc(GtkWidget*, void*);
            static int po_button_press(GtkWidget*, GdkEvent*);
            static void po_help_proc(GtkWidget*, void*);
            static int po_text_btn_hdlr(GtkWidget*, GdkEvent*, void*);
            static int po_text_btn_release_hdlr(GtkWidget*, GdkEvent*, void*);
            static int po_motion_hdlr(GtkWidget*, GdkEvent*, void*);
            static void po_drag_data_get(GtkWidget*, GdkDragContext*,
                GtkSelectionData*, guint, guint, void*);
            static void po_drag_data_received(GtkWidget*, GdkDragContext*, gint x,
                gint y, GtkSelectionData*, guint, guint);

            GtkWidget *po_activ;
            GtkWidget *po_edit;
            GtkWidget *po_del;
            GtkWidget *po_add;
            GtkWidget *po_item;
            GtkWidget *po_global;
            GtkWidget *po_info;
            GtkWidget *po_name_btn;
            int po_dspmode;

            static sAddEnt po_elec_addmenu[];
            static sAddEnt po_phys_addmenu[];
        };

        sPo *Po;

        enum {
            EditCode = 1,
            DeleteCode,
            GlobCode,
            InfoCode,
            ActivCode
        };
    }
}

using namespace gtkprpedit;

sAddEnt sPo::po_elec_addmenu[] = {
    sAddEnt("name", P_NAME),
    sAddEnt("model", P_MODEL),
    sAddEnt("value", P_VALUE),
    sAddEnt("param", P_PARAM),
    sAddEnt("devref", P_DEVREF),
    sAddEnt("other", P_OTHER),
    sAddEnt("nophys", P_NOPHYS),
    sAddEnt("flatten", P_FLATTEN),
    sAddEnt("nosymb", P_SYMBLC),
    sAddEnt("range", P_RANGE),
    sAddEnt(0, 0)
};

sAddEnt sPo::po_phys_addmenu[] = {
    sAddEnt("any", -1),
    sAddEnt("flatten", XICP_EXT_FLATTEN),
    sAddEnt("nomerge", XICP_NOMERGE),
    sAddEnt(0, 0)
};


// Pop up the property panel, list the properties of odesc.  The activ
// argument sets the state of the Activate button and menu sensitivity.
//
void
cEdit::PopUpProperties(CDo *odesc, ShowMode mode, PRPmode activ)
{
    if (!GRX || !GTKmainwin::self())
        return;
    if (mode == MODE_OFF) {
        delete Po;
        return;
    }
    if (Po) {
        Po->update(odesc, activ);
        return;
    }
    if (mode == MODE_UPD)
        return;

    new sPo(odesc, activ);
    if (!Po->Shell()) {
        delete Po;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Po->Shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_LL), Po->Shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(Po->Shell());
}


// Given the offset, return the containing PrptyText element and object desc,
// called from selection processing code, supports Property Editor and
// Property Info pop-ups.
//
PrptyText *
cEdit::PropertyResolve(int code, int offset, CDo **odp)
{
    if (odp)
        *odp = 0;
    if (code == 1) {
        if (Po)
            return (Po->resolve(offset, odp));
    }
    else if (code == 2) {
        if (sPbase::prptyInfoPtr())
            return (sPbase::prptyInfoPtr()->resolve(offset, odp));
    }
    return (0);
}


// Called when odold is being deleted.
//
void
cEdit::PropertyPurge(CDo *odold, CDo *odnew)
{
    if (Po)
        Po->purge(odold, odnew);
}


// Select and return the first matching property.
//
PrptyText *
cEdit::PropertySelect(int which)
{
    if (Po)
        return (Po->select(which));
    return (0);
}


PrptyText *
cEdit::PropertyCycle(CDp *pd, bool(*checkfunc)(const CDp*), bool rev)
{
    if (Po)
        return (Po->cycle(pd, checkfunc, rev));
    return (0);
}


void
cEdit::RegisterPrptyBtnCallback(int(*cb)(PrptyText*))
{
    if (Po)
        Po->set_btn_callback(cb);
}
// End of cEdit functions.


sPo::sPo(CDo *odesc, PRPmode activ)
{
    Po = this;
    po_activ = 0;
    po_edit = 0;
    po_del = 0;
    po_add = 0;
    po_item = 0;
    po_global = 0;
    po_info = 0;
    po_name_btn = 0;
    po_dspmode = -1;

    wb_shell = gtk_NewPopup(0, "Property Editor", po_cancel_proc, 0);
    if (!wb_shell)
        return;

    GtkWidget *form = gtk_table_new(1, 4, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    //
    // top row buttons
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    GtkWidget *button = gtk_toggle_button_new_with_label("Edit");
    gtk_widget_set_name(button, "Edit");
    gtk_widget_show(button);
    po_edit = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(po_action_proc), (void*)EditCode);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_toggle_button_new_with_label("Delete");
    gtk_widget_set_name(button, "Delete");
    gtk_widget_show(button);
    po_del = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(po_action_proc), (void*)DeleteCode);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    // Add button and its menu
    GtkWidget *menubar = gtk_menu_bar_new();
    gtk_widget_set_name(menubar, "Add");
    gtk_widget_show(menubar);
    po_add = menubar;

    po_item = gtk_menu_item_new_with_label("Add");
    gtk_widget_set_name(po_item, "AddI");
    gtk_widget_show(po_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), po_item);
    g_signal_connect(G_OBJECT(po_item), "event",
        G_CALLBACK(po_button_press), 0);
    gtk_box_pack_start(GTK_BOX(hbox), menubar, true, true, 0);

    button = gtk_toggle_button_new_with_label("Global");
    gtk_widget_set_name(button, "Global");
    gtk_widget_show(button);
    po_global = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(po_action_proc), (void*)GlobCode);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_toggle_button_new_with_label("Info");
    gtk_widget_set_name(button, "Info");
    gtk_widget_show(button);
    po_info = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(po_action_proc), (void*)InfoCode);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(po_help_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // scrolled text area
    //
    GtkWidget *contr;
    text_scrollable_new(&contr, &wb_textarea, FNT_FIXED);

    g_signal_connect(G_OBJECT(wb_textarea), "button-press-event",
        G_CALLBACK(po_text_btn_hdlr), 0);
    g_signal_connect(G_OBJECT(wb_textarea), "button-release-event",
        G_CALLBACK(po_text_btn_release_hdlr), 0);

    // dnd stuff
    g_signal_connect(G_OBJECT(wb_textarea), "motion-notify-event",
        G_CALLBACK(po_motion_hdlr), 0);
    g_signal_connect(G_OBJECT(wb_textarea), "drag-data-get",
        G_CALLBACK(po_drag_data_get), 0);
    GtkDestDefaults DD = (GtkDestDefaults)
        (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP);
    gtk_drag_dest_set(wb_textarea, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(wb_textarea), "drag-data-received",
        G_CALLBACK(po_drag_data_received), 0);
    g_signal_connect_after(G_OBJECT(wb_textarea), "realize",
        G_CALLBACK(text_realize_proc), 0);

    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
    const char *bclr = GRpkgIf()->GetAttrColor(GRattrColorLocSel);
    gtk_text_buffer_create_tag(textbuf, "primary", "background", bclr,
        "paragraph-background", bclr, NULL);

    // for passing hypertext via selections, see gtkhtext.cc
    g_object_set_data(G_OBJECT(wb_textarea), "hyexport", (void*)1);

    gtk_widget_set_size_request(wb_textarea, 400, 200);

    // The font change pop-up uses this to redraw the widget
    g_object_set_data(G_OBJECT(wb_textarea), "font_changed",
        (void*)po_font_changed);

    gtk_table_attach(GTK_TABLE(form), contr, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // activate and dismiss buttons
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    button = gtk_toggle_button_new_with_label("Activate");
    gtk_widget_set_name(button, "Activate");
    gtk_widget_show(button);
    po_activ = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(po_action_proc), (void*)ActivCode);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(po_cancel_proc), 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    update(odesc, activ);
}


sPo::~sPo()
{
    Po = 0;
    if (Menu()->MenuButtonStatus(MMedit, MenuPRPTY) == 1) {
        Menu()->MenuButtonPress(MMedit, MenuPRPTY);
        if (pi_odesc)
            DSP()->ShowCurrentObject(ERASE, pi_odesc, MarkerColor);
    }
    if (wb_shell) {
        g_signal_handlers_disconnect_by_func(G_OBJECT(wb_shell),
            (gpointer)(po_cancel_proc), wb_shell);
    }
}


void
sPo::update(CDo *odesc, PRPmode activ)
{
    if (pi_odesc)
        DSP()->ShowCurrentObject(ERASE, pi_odesc, MarkerColor);
    PrptyText::destroy(pi_list);
    pi_list = 0;
    pi_odesc = odesc;
    if (odesc) {
        CDs *cursd = CurCell();
        pi_list = (cursd ? XM()->PrptyStrings(odesc, cursd) : 0);
    }
    update_display();
    if (pi_odesc)
        DSP()->ShowCurrentObject(DISPLAY, pi_odesc, MarkerColor);

    if (activ == PRPnochange)
        return;
    if (activ == PRPinactive) {
        if (GRX->GetStatus(po_activ) == true) {
            GRX->SetStatus(po_activ, false);
            activate(false);
        }
        GRX->SetStatus(po_info, false);
        GRX->SetStatus(po_global, false);
    }
    else if (activ == PRPactive) {
        if (GRX->GetStatus(po_activ) == false) {
            GRX->SetStatus(po_activ, true);
            activate(true);
        }
    }

    if (DSP()->CurMode() == po_dspmode)
        return;
    po_dspmode = DSP()->CurMode();

    // Set up the add menu.
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "AddM");
    if (DSP()->CurMode() == Physical) {
        for (int i = 0; ; i++) {
            const char *s = po_phys_addmenu[i].name;
            if (!s)
                break;
            GtkWidget *menu_item = gtk_menu_item_new_with_label(s);
            gtk_widget_set_name(menu_item, s);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
            g_signal_connect(G_OBJECT(menu_item), "activate",
                G_CALLBACK(po_menu_proc), &po_phys_addmenu[i]);
            gtk_widget_show(menu_item);
        }
    }
    else {
        for (int i = 0; ; i++) {
            const char *s = po_elec_addmenu[i].name;
            if (!s)
                break;
            GtkWidget *menu_item = gtk_menu_item_new_with_label(s);
            gtk_widget_set_name(menu_item, s);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
            g_signal_connect(G_OBJECT(menu_item), "activate",
                G_CALLBACK(po_menu_proc), &po_elec_addmenu[i]);
            gtk_widget_show(menu_item);

            // Hard-code name button, index 0
            if (!i)
                po_name_btn = menu_item;
        }
    }
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(po_item), menu);
}


void
sPo::purge(CDo *odold, CDo *odnew)
{
    if (odold == pi_odesc)
        ED()->PopUpProperties(odnew, MODE_UPD, PRPnochange);
}


PrptyText *
sPo::select(int which)
{
    for (PrptyText *p = pi_list; p; p = p->next()) {
        if (!p->prpty())
            continue;
        if (p->prpty()->value() == which) {
            select_range(p->start() + strlen(p->head()), p->end());
            return (p);
        }
    }
    return (0);
}


PrptyText *
sPo::cycle(CDp *pd, bool(*checkfunc)(const CDp*), bool rev)
{
    int cnt = 0;
    for (PrptyText *p = pi_list; p; p = p->next(), cnt++) ;
    if (!cnt)
        return (0);
    PrptyText **ary = new PrptyText*[cnt];
    cnt = 0;
    for (PrptyText *p = pi_list; p; p = p->next(), cnt++)
        ary[cnt] = p;

    PrptyText *p0 = 0;
    int j = 0;
    if (pd) {
        for (j = 0; j < cnt; j++) {
            if (ary[j]->prpty() == pd)
                break;
        }
    }
    if (!pd || j == cnt) {
        for (int i = 0; i < cnt; i++) {
            if (!ary[i]->prpty())
                continue;
            if (!checkfunc || checkfunc(ary[i]->prpty())) {
                p0 = ary[i];
                break;
            }
        }
    }
    else {
        if (!rev) {
            for (int i = j+1; i < cnt; i++) {
                if (!ary[i]->prpty())
                    continue;
                if (!checkfunc || checkfunc(ary[i]->prpty())) {
                    p0 = ary[i];
                    break;
                }
            }
            if (!p0) {
                for (int i = 0; i <= j; i++) {
                    if (!ary[i]->prpty())
                        continue;
                    if (!checkfunc || checkfunc(ary[i]->prpty())) {
                        p0 = ary[i];
                        break;
                    }
                }
            }
        }
        else {
            for (int i = j-1; i >= 0; i--) {
                if (!ary[i]->prpty())
                    continue;
                if (!checkfunc || checkfunc(ary[i]->prpty())) {
                    p0 = ary[i];
                    break;
                }
            }
            if (!p0) {
                for (int i = cnt-1; i >= j; i--) {
                    if (!ary[i]->prpty())
                        continue;
                    if (!checkfunc || checkfunc(ary[i]->prpty())) {
                        p0 = ary[i];
                        break;
                    }
                }
            }
        }
    }
    delete [] ary;
    if (p0)
        select_range(p0->start() + strlen(p0->head()), p0->end());
    return (p0);
}


void
sPo::set_btn_callback(int(*cb)(PrptyText*))
{
    pi_btn_callback = cb;
}


void
sPo::activate(bool activ)
{
    gtk_widget_set_sensitive(po_edit, activ);
    gtk_widget_set_sensitive(po_del, activ);
    gtk_widget_set_sensitive(po_add, activ);
    gtk_widget_set_sensitive(po_global, activ);
    gtk_widget_set_sensitive(po_info, activ);
}


void
sPo::call_prpty_add(int which) {
    gtk_widget_set_sensitive(po_global, false);
    ED()->prptyAdd(which);
    if (Po)
        gtk_widget_set_sensitive(po_global, true);
}


void
sPo::call_prpty_edit(PrptyText *line) {
    gtk_widget_set_sensitive(po_global, false);
    ED()->prptyEdit(line);
    if (Po)
        gtk_widget_set_sensitive(po_global, true);
}


void
sPo::call_prpty_del(PrptyText *line) {
    gtk_widget_set_sensitive(po_global, false);
    ED()->prptyRemove(line);
    if (Po)
        gtk_widget_set_sensitive(po_global, true);
}


// Static function.
// This is called when the font is changed
//
void
sPo::po_font_changed()
{
    if (Po)
        Po->update_display();
}


// Static function.
void
sPo::po_cancel_proc(GtkWidget*, void*)
{
    ED()->PopUpProperties(0, MODE_OFF, PRPnochange);
}


// Static function.
// If there is something selected, perform the action.
//
void
sPo::po_action_proc(GtkWidget *caller, void *client_data)
{
    bool state = GRX->GetStatus(caller);
    if (client_data == (void*)GlobCode) {
        ED()->prptySetGlobal(state);
        if (Po->po_name_btn)
            gtk_widget_set_sensitive(Po->po_name_btn, !state);
        return;
    }

    if (EV()->CurCmd() && EV()->CurCmd() != ED()->prptyCmd())
        EV()->CurCmd()->esc();

    if (client_data == (void*)InfoCode) {
        ED()->prptySetInfoMode(state);
        return;
    }
    if (client_data == (void*)ActivCode) {
        if (state) {
            if (Menu()->MenuButtonStatus(MMedit, MenuPRPTY) == 0)
                Menu()->MenuButtonPress(MMedit, MenuPRPTY);
        }
        else {
            GRX->SetStatus(Po->po_info, false);
            GRX->SetStatus(Po->po_global, false);
            if (Menu()->MenuButtonStatus(MMedit, MenuPRPTY) == 1)
                Menu()->MenuButtonPress(MMedit, MenuPRPTY);
        }
        Po->activate(state);
        return;
    }
    if (!state)
       return;

    if (caller != Po->po_edit)
        GRX->Deselect(Po->po_edit);
    if (caller != Po->po_del)
        GRX->Deselect(Po->po_del);

    if (DSP()->CurMode() == Physical) {
        if (Po->po_add && caller != Po->po_add)
            GRX->Deselect(Po->po_add);
    }

    if (client_data == (void*)EditCode) {
        PrptyText *p = Po->get_selection();
        Po->call_prpty_edit(p);
        if (Po)
            GRX->Deselect(caller);
    }
    else if (client_data == (void*)DeleteCode) {
        PrptyText *p = Po->get_selection();
        Po->call_prpty_del(p);
        if (Po)
            GRX->Deselect(caller);
    }
}


// Static function.
// Handler for the Add menu
//
void
sPo::po_menu_proc(GtkWidget *caller, void *client_data)
{
    if (!Po)
        return;
    sAddEnt *ae = (sAddEnt*)client_data;
    Po->call_prpty_add(ae->value);
    if (Po)
        GRX->Deselect(caller);
}


// Static function.
// Respond to a button-press by posting a menu passed in as widget.
//
// Note that the "widget" argument is the menu being posted, NOT
// the button that was pressed.
//
int
sPo::po_button_press(GtkWidget *widget, GdkEvent *event)
{
    GtkWidget *menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget));
    if (event->type == GDK_BUTTON_PRESS) {
        GRX->Deselect(Po->po_edit);
        GRX->Deselect(Po->po_del);
        GdkEventButton *bevent = (GdkEventButton*)event;
        gtk_menu_popup(GTK_MENU(menu), 0, 0, 0, 0, bevent->button,
            bevent->time);
        return (true);
    }
    return (false);
}


// Static function.
void
sPo::po_help_proc(GtkWidget*, void*)
{
    DSPmainWbag(PopUpHelp("prppanel"))
}


// Static function.
// Handle button presses in the text area.
//
int
sPo::po_text_btn_hdlr(GtkWidget *caller, GdkEvent *event, void *arg)
{
    if (Po)
        return (Po->button_dn(caller, event, arg));
    return (false);
}


// Static function.
int
sPo::po_text_btn_release_hdlr(GtkWidget *caller, GdkEvent *event, void *arg)
{
    if (Po)
        return (Po->button_up(caller, event, arg));
    return (false);
}


// Static function.
int
sPo::po_motion_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (Po)
        return (Po->motion(caller, event, 0));
    return (false);
}


// Static function.
void
sPo::po_drag_data_get(GtkWidget*, GdkDragContext*,
    GtkSelectionData *selection_data, guint, guint, void*)
{
    if (Po)
        Po->drag_data_get(selection_data);
}


// Static function.
void
sPo::po_drag_data_received(GtkWidget *caller, GdkDragContext *context,
    gint, gint, GtkSelectionData *data, guint, guint time)
{
    if (Po)
        Po->data_received(caller, context, data, time);
}

