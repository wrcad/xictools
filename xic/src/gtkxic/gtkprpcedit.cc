
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
#include "events.h"
#include "promptline.h"
#include "gtkmain.h"
#include "gtkinterf/gtkfont.h"


//--------------------------------------------------------------------------
// Pop up to modify proerties of the current cell
//
// Help system keywords used:
//  xic:cprop

namespace {
    namespace gtkprpcedit {

        struct sAddEnt
        {
            sAddEnt(const char *n, int v)   { name = n; value = v; }

            const char *name;
            int value;
        };

        struct sPc : public GTKbag
        {
            sPc();
            ~sPc();

            void update();

        private:
            PrptyText *get_selection();
            void update_display();
            void select_range(int, int);

            static void pc_font_changed();
            static void pc_cancel_proc(GtkWidget*, void*);
            static void pc_action_proc(GtkWidget*, void*);
            static void pc_menu_proc(GtkWidget*, void*);
            static int pc_button_press(GtkWidget*, GdkEvent*);
            static void pc_help_proc(GtkWidget*, void*);
            static int pc_text_btn_hdlr(GtkWidget*, GdkEvent*, void*);
            static int pc_text_btn_release_hdlr(GtkWidget*, GdkEvent*, void*);

            GtkWidget *pc_edit;
            GtkWidget *pc_del;
            GtkWidget *pc_item;
            int pc_line_selected;
            int pc_action_calls;
            int pc_start;
            int pc_end;
            PrptyText *pc_list;
            int pc_dspmode;

            static sAddEnt pc_elec_addmenu[];
            static sAddEnt pc_phys_addmenu[];
        };

        sPc *Pc;
    }
}

using namespace gtkprpcedit;

sAddEnt sPc::pc_elec_addmenu[] = {
    sAddEnt("param", P_PARAM),
    sAddEnt("other", P_OTHER),
    sAddEnt("virtual", P_VIRTUAL),
    sAddEnt("flatten", P_FLATTEN),
    sAddEnt(0, 0)
};

sAddEnt sPc::pc_phys_addmenu[] = {
    sAddEnt("any", -1),
    sAddEnt("flags", XICP_FLAGS),
    sAddEnt("flatten", XICP_EXT_FLATTEN),
    sAddEnt("pc_script", XICP_PC_SCRIPT),
    sAddEnt("pc_params", XICP_PC_PARAMS),
    sAddEnt(0, 0)
};

#define EditCode   1
#define DeleteCode 2


// Pop up the cell properties editor.
//
void
cEdit::PopUpCellProperties(ShowMode mode)
{
    if (!GTKdev::exists() || !GTKmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete Pc;
        return;
    }
    if (Pc) {
        Pc->update();
        return;
    }
    if (mode == MODE_UPD)
        return;

    new sPc;
    if (!Pc->Shell()) {
        delete Pc;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Pc->Shell()),
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GTKdev::self()->SetPopupLocation(GRloc(LW_LL), Pc->Shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(Pc->Shell());
}
// End of cEdit functions.


sPc::sPc()
{
    Pc = this;
    pc_edit = 0;
    pc_del = 0;
    pc_item = 0;
    pc_line_selected = -1;
    pc_action_calls = 0;
    pc_start = 0;
    pc_end = 0;
    pc_list = 0;
    pc_dspmode = -1;

    wb_shell = gtk_NewPopup(0, "Cell Property Editor", pc_cancel_proc, 0);
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
    pc_edit = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(pc_action_proc), (void*)EditCode);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    button = gtk_toggle_button_new_with_label("Delete");
    gtk_widget_set_name(button, "Delete");
    gtk_widget_show(button);
    pc_del = button;
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(pc_action_proc), (void*)DeleteCode);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);

    // Add button and its menu
    //
    GtkWidget *menubar = gtk_menu_bar_new();
    gtk_widget_set_name(menubar, "Add");
    gtk_widget_show(menubar);

    pc_item = gtk_menu_item_new_with_label("Add");
    gtk_widget_set_name(pc_item, "AddI");
    gtk_widget_show(pc_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), pc_item);
    g_signal_connect(G_OBJECT(pc_item), "event",
        G_CALLBACK(pc_button_press), 0);
    gtk_box_pack_start(GTK_BOX(hbox), menubar, true, true, 0);

    button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(pc_help_proc), 0);
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
        G_CALLBACK(pc_text_btn_hdlr), 0);
    g_signal_connect(G_OBJECT(wb_textarea), "button-release-event",
        G_CALLBACK(pc_text_btn_release_hdlr), 0);
    g_signal_connect_after(G_OBJECT(wb_textarea), "realize",
        G_CALLBACK(text_realize_proc), 0);

    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
    const char *bclr = GTKpkg::self()->GetAttrColor(GRattrColorLocSel);
    gtk_text_buffer_create_tag(textbuf, "primary", "background", bclr,
        "paragraph-background", bclr, NULL);
    gtk_widget_set_size_request(wb_textarea, 300, 200);

    // The font change pop-up uses this to redraw the widget
    g_object_set_data(G_OBJECT(wb_textarea), "font_changed",
        (void*)pc_font_changed);

    gtk_table_attach(GTK_TABLE(form), contr, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // dismiss button
    //
    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(pc_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    update();
}


sPc::~sPc()
{
    if (Pc && Pc->pc_action_calls) {
        EV()->InitCallback();
        Pc->pc_action_calls = 0;
    }
    Pc = 0;
    PrptyText::destroy(pc_list);
    MainMenu()->MenuButtonSet(0, MenuCPROP, false);
    PL()->AbortLongText();
    if (wb_shell)
        g_signal_handlers_disconnect_by_func(G_OBJECT(wb_shell),
            (gpointer)pc_cancel_proc, wb_shell);
}


void
sPc::update()
{
    PrptyText::destroy(pc_list);
    CDs *cursd = CurCell();
    pc_list = cursd ? XM()->PrptyStrings(cursd) : 0;
    update_display();

    if (DSP()->CurMode() == pc_dspmode)
        return;
    pc_dspmode = DSP()->CurMode();

    // Set up the add menu.
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "AddM");
    if (DSP()->CurMode() == Physical) {
        for (int i = 0; ; i++) {
            const char *s = pc_phys_addmenu[i].name;
            if (!s)
                break;
            GtkWidget *menu_item = gtk_menu_item_new_with_label(s);
            gtk_widget_set_name(menu_item, s);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
            g_signal_connect(G_OBJECT(menu_item), "activate",
                G_CALLBACK(pc_menu_proc), &pc_phys_addmenu[i]);
            gtk_widget_show(menu_item);
        }
    }
    else {
        for (int i = 0; ; i++) {
            const char *s = pc_elec_addmenu[i].name;
            if (!s)
                break;
            GtkWidget *menu_item = gtk_menu_item_new_with_label(s);
            gtk_widget_set_name(menu_item, s);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
            g_signal_connect(G_OBJECT(menu_item), "activate",
                G_CALLBACK(pc_menu_proc), &pc_elec_addmenu[i]);
            gtk_widget_show(menu_item);
        }
    }
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(pc_item), menu);
}


// Return the PrptyText element corresponding to the selected line, or 0 if
// there is no selection
//
PrptyText *
sPc::get_selection()
{
    int start, end;
    start = pc_start;
    end = pc_end;
    if (start == end)
        return (0);
    for (PrptyText *p = pc_list; p; p = p->next()) {
        if (start >= p->start() && start < p->end())
            return (p);
    }
    return (0);
}


void
sPc::update_display()
{
    GdkColor *c1 = gtk_PopupColor(GRattrColorHl4);
    GdkColor *c2 = gtk_PopupColor(GRattrColorHl2);
    double val = text_get_scroll_value(wb_textarea);
    text_set_chars(wb_textarea, "");

    if (!pc_list)
        text_insert_chars_at_point(wb_textarea, c1,
            "Current cell has no properties.", -1, -1);
    else {
        int cnt = 0;
        for (PrptyText *p = pc_list; p; p = p->next()) {
            p->set_start(cnt);

            GdkColor *c;
            const char *s = p->head();
            if (*s == '(')
                s++;
            int num = atoi(s);
            if (DSP()->CurMode() == Physical) {
                if (prpty_gdsii(num) || prpty_global(num) ||
                        prpty_reserved(num))
                    c = 0;
                else if (prpty_pseudo(num))
                    c = c2;
                else
                    c = c1;
            }
            else {
                switch (num) {
                case P_PARAM:
                case P_OTHER:
                case P_VIRTUAL:
                case P_FLATTEN:
                case P_MACRO:
                    c = c1;
                    break;
                default:
                    c = 0;
                    break;
                }
            }
            text_insert_chars_at_point(wb_textarea, c, p->head(), -1, -1);
            cnt += strlen(p->head());
            text_insert_chars_at_point(wb_textarea, 0, p->string(), -1, -1);
            text_insert_chars_at_point(wb_textarea, 0, "\n", -1, -1);
            cnt += strlen(p->string());
            p->set_end(cnt);
            cnt++;
        }
    }
    text_set_scroll_value(wb_textarea, val);
    pc_line_selected = -1;
}


// Select the chars in the range, start=end deselects existing.  In
// GTK-1, selecting gives blue inverse, which turns gray if
// unselected, retaining an indication for the buttons.  GTK-2
// doesn't do this automatically so we provide something similar here.
//
void
sPc::select_range(int start, int end)
{
    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
    GtkTextIter istart, iend;
    if (pc_end != pc_start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, pc_start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, pc_end);
        gtk_text_buffer_remove_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    text_select_range(wb_textarea, start, end);
    if (end != start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, end);
        gtk_text_buffer_apply_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    pc_start = start;
    pc_end = end;
}


// Static function.
// This is called when the font is changed
//
void
sPc::pc_font_changed()
{
    if (Pc)
        Pc->update_display();
}


// Static function.
void
sPc::pc_cancel_proc(GtkWidget*, void*)
{
    ED()->PopUpCellProperties(MODE_OFF);
}


// Static function.
// If there is something selected, perform the action.
//
void
sPc::pc_action_proc(GtkWidget *caller, void *client_data)
{
    if (Pc) {
        bool state = GTKdev::GetStatus(caller);
        if (!state) {
            if (caller == Pc->pc_edit)
                EV()->InitCallback();
            return;
        }
        if (caller != Pc->pc_edit)
            GTKdev::Deselect(Pc->pc_edit);
        if (caller != Pc->pc_del)
            GTKdev::Deselect(Pc->pc_del);

        Pc->pc_action_calls++;
        if (client_data == (void*)EditCode) {
            PrptyText *p = Pc->get_selection();
            if (p)
                ED()->cellPrptyEdit(p);
            if (Pc)
                GTKdev::Deselect(caller);
        }
        else if (client_data == (void*)DeleteCode) {
            PrptyText *p = Pc->get_selection();
            if (p)
                ED()->cellPrptyRemove(p);
            if (Pc)
                GTKdev::Deselect(caller);
        }
        if (Pc)
            Pc->pc_action_calls--;
    }
}


// Static function.
// Handler for the Add menu.
//
void
sPc::pc_menu_proc(GtkWidget*, void *client_data)
{
    if (!Pc)
        return;
    Pc->pc_action_calls++;
    sAddEnt *ad = (sAddEnt*)client_data;
    ED()->cellPrptyAdd(ad->value);
    if (Pc)
        Pc->pc_action_calls--;
}


// Static function.
// Respond to a button-press by posting a menu passed in as widget.
//
// Note that the "widget" argument is the menu being posted, NOT
// the button that was pressed.
//
int
sPc::pc_button_press(GtkWidget *widget, GdkEvent *event)
{
    GtkWidget *menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget));
    if (event->type == GDK_BUTTON_PRESS) {
        GTKdev::Deselect(Pc->pc_edit);
        GTKdev::Deselect(Pc->pc_del);
        GdkEventButton *bevent = (GdkEventButton*)event;
        gtk_menu_popup(GTK_MENU(menu), 0, 0, 0, 0, bevent->button,
            bevent->time);
        return (true);
    }
    return (false);
}


// Static function.
void
sPc::pc_help_proc(GtkWidget*, void*)
{
    DSPmainWbag(PopUpHelp("xic:cprop"))
}


// Static function.
// Handle button presses in the text area.
//
int
sPc::pc_text_btn_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (!Pc)
        return (false);
    if (event->type != GDK_BUTTON_PRESS)
        return (true);

    char *string = text_get_chars(caller, 0, -1);
    int x = (int)event->button.x;
    int y = (int)event->button.y;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(caller), &ihere, x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    char *line_start = string + gtk_text_iter_get_offset(&iline);
    x = gtk_text_iter_get_offset(&ihere) - gtk_text_iter_get_offset(&iline);
    y = gtk_text_iter_get_line(&iline);
    // line_start points to start of selected line, or 0
    if (line_start && *line_start != '\n') {
        PrptyText *p = Pc->pc_list;
        int pos = line_start - string;
        for ( ; p; p = p->next()) {
            if (pos >= p->start() && pos < p->end())
                break;
        }
        if (p) {
            char *s = line_start;
            char *t = s;
            while (*t && *t != '\n')
                t++;
            if (x >= (t-s))
                p = 0;
        }
        if (p && p->start() + string != line_start) {
            int cnt = 0;
            for (char *s = string + p->start(); s < line_start; s++)
                if (*s == '\n')
                    cnt++;
            y -= cnt;
        }
        if (p && Pc->pc_line_selected != y) {
            Pc->pc_line_selected = y;
            Pc->select_range(p->start() + strlen(p->head()), p->end());
            delete [] string;
            return (true);
        }
    }
    Pc->pc_line_selected = -1;
    delete [] string;
    Pc->select_range(0, 0);
    return (true);
}


// Static function.
int
sPc::pc_text_btn_release_hdlr(GtkWidget*, GdkEvent*, void*)
{
    return (false);
}
// End of sPc functions.

