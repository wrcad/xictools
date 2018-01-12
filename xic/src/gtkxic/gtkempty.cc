
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
#include "cvrt.h"
#include "cd_celldb.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkfont.h"


//--------------------------------------------------------------------------
//
// Pop up to allow the user to delete empty cells from a hierarchy
//

namespace {
    // List element for empty cells
    //
    struct e_item
    {
        e_item() { name = 0; del = false; }

        const char *name;                   // cell name
        bool del;                           // deletion flag
    };

    namespace gtkempty {
        struct sEC : public gtk_bag
        {
            sEC(stringlist*);
            ~sEC();

            void update(stringlist*);

        private:
            void refresh();

            static void ec_font_changed();
            static void ec_cancel_proc(GtkWidget*, void*);
            static void ec_btn_proc(GtkWidget*, void*);
            static int ec_btn_hdlr(GtkWidget*, GdkEvent*, void*);

            e_item *ec_list;                    // list of cells
            SymTab *ec_tab;                     // table of checked cells
            GtkWidget *ec_label;                // label widget
            int ec_field;                       // max cell name length
            bool ec_changed;
        };

        sEC *EC;

        enum { EC_nil, EC_apply, EC_delete, EC_skip };
    }
}

using namespace gtkempty;


void
cConvert::PopUpEmpties(stringlist *list)
{
    if (!GRX || !mainBag())
        return;
    if (EC)
        return;
    if (XM()->RunMode() != ModeNormal)
        return;
    if (!list)
        return;

    new sEC(list);
    if (!EC->Shell()) {
        delete EC;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(EC->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_LL), EC->Shell(), mainBag()->Viewport());
    gtk_widget_show(EC->Shell());
}
// End of cConvert functions.


sEC::sEC(stringlist *l)
{
    EC = this;
    ec_list = 0;
    ec_tab = 0;
    ec_label = 0;
    ec_field = 0;
    ec_changed = false;

    wb_shell = gtk_NewPopup(0, "Empty Cells", ec_cancel_proc, 0);
    if (!wb_shell)
        return;
    GtkWidget *form = gtk_table_new(2, 4, false);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    gtk_widget_show(form);

    GtkWidget *button = gtk_button_new_with_label("Delete All");
    gtk_widget_set_name(button, "DeleteAll");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ec_btn_proc), (void*)EC_delete);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label("Skip All");
    gtk_widget_set_name(button, "SkipAll");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ec_btn_proc), (void*)EC_skip);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);

    ec_label = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(ec_label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(ec_label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(ec_label), 4, 2);
    gtk_widget_show(ec_label);
    gtk_container_add(GTK_CONTAINER(frame), ec_label);

    // Use a fixed font in the label, same as the text area, so can
    // match columns.
    GTKfont::setupFont(ec_label, FNT_FIXED, true);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *contr;
    text_scrollable_new(&contr, &wb_textarea, FNT_FIXED);

    gtk_widget_add_events(wb_textarea, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "button-press-event",
        GTK_SIGNAL_FUNC(ec_btn_hdlr), 0);
    gtk_signal_connect_after(GTK_OBJECT(wb_textarea), "realize",
        GTK_SIGNAL_FUNC(text_realize_proc), 0);

    // The font change pop-up uses this to redraw the widget
    gtk_object_set_data(GTK_OBJECT(wb_textarea), "font_changed",
        (void*)ec_font_changed);

    gtk_table_attach(GTK_TABLE(form), contr, 0, 2, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);

    button = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(button, "Apply");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ec_btn_proc), (void*)EC_apply);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ec_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    update(l);
    int ww = (ec_field + 6)*GTKfont::stringWidth(wb_textarea, 0);
    if (ww < 200)
        ww = 200;
    else if (ww > 600)
        ww = 600;
    ww += 15;  // scrollbar
    gtk_widget_set_usize(GTK_WIDGET(wb_textarea), ww, -1);
}


sEC::~sEC()
{
    EC = 0;
    if (ec_changed) {
        CDcbin cbin(DSP()->CurCellName());
        cbin.fixBBs();
        XM()->PopUpCells(0, MODE_UPD);
        XM()->PopUpTree(0, MODE_UPD, 0, TU_CUR);
        DSP()->RedisplayAll();
    }
    delete [] ec_list;
    delete ec_tab;
    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(ec_cancel_proc), wb_shell);
}


void
sEC::update(stringlist *sl)
{
    stringlist *s0 = 0;
    if (!ec_tab)
        ec_tab = new SymTab(false, false);
    if (!sl) {
        if (!DSP()->CurCellName())
            return;
        CDcbin cbin(DSP()->CurCellName());
        sl = cbin.listEmpties();
        stringlist::sort(sl);
        s0 = sl;
    }
    if (!sl) {
        ec_cancel_proc(0, 0);
        return;
    }
    delete [] ec_list;

    int sz = stringlist::length(sl);
    ec_list = new e_item[sz+1];
    ec_field = 0;
    e_item *itm = ec_list;
    while (sl) {
        // Save only names not seen before.
        if (SymTab::get(ec_tab, sl->string) == ST_NIL) {
            itm->name = lstring::copy(sl->string);
            ec_tab->add(itm->name, 0, false);
            int w = strlen(sl->string);
            if (w > ec_field)
                ec_field = w;
            itm++;
        }
        sl = sl->next;
    }
    stringlist::destroy(s0);

    if (itm == ec_list) {
        // No new items.
        ec_cancel_proc(0, 0);
        return;
    }

    char lab[256];
    strcpy(lab, "Empty Cells            Click on yes/no\n");
    char *t = lab + strlen(lab);
    for (int i = 0; i <= ec_field; i++)
        *t++ = ' ';
    strcpy(t, "Delete?");

    gtk_label_set_text(GTK_LABEL(ec_label), lab);
    refresh();
}


// Redraw the text area.
//
void
sEC::refresh()
{
    if (EC && ec_list) {
        char buf[256];
        GdkColor *nc = gtk_PopupColor(GRattrColorNo);
        GdkColor *yc = gtk_PopupColor(GRattrColorYes);
        double val = text_get_scroll_value(wb_textarea);
        text_set_chars(wb_textarea, "");
        for (e_item *s = ec_list; s->name; s++) {
            sprintf(buf, "%-*s  ", ec_field, s->name);
            text_insert_chars_at_point(wb_textarea, 0, buf, -1, -1);
            sprintf(buf, "%-3s\n", s->del ? "yes" : "no");
            text_insert_chars_at_point(wb_textarea, s->del ? yc : nc, buf,
                -1, -1);
        }
        text_set_scroll_value(wb_textarea, val);
    }
}


// Static function.
// This is called when the font is changed.
//
void
sEC::ec_font_changed()
{
    if (EC)
        EC->refresh();
}


// Static function.
void
sEC::ec_cancel_proc(GtkWidget*, void*)
{
    delete EC;
}


// Static function.
void
sEC::ec_btn_proc(GtkWidget*, void *client_data)
{
    if (!EC)
        return;
    int mode = (long)client_data;
    if (mode == EC_apply) {
        bool didone = false;
        bool leftone = false;
        for (e_item *s = EC->ec_list; s->name; s++) {
            if (s->del) {
                CDcbin cbin;
                if (CDcdb()->findSymbol(s->name, &cbin)) {
                    if (cbin.deleteCells())
                        didone = true;
                    continue;
                }
            }
            leftone = true;
        }
        if (didone)
            EC->ec_changed = true;
        if (didone && !leftone)
            EC->update(0);
        else
            ec_cancel_proc(0, 0);
    }
    else if (mode == EC_delete) {
        for (e_item *s = EC->ec_list; s->name; s++)
            s->del = true;
        EC->refresh();
    }
    else if (mode == EC_skip) {
        for (e_item *s = EC->ec_list; s->name; s++)
            s->del = false;
        EC->refresh();
    }
}


// Static function.
// Handle button presses in the text area.
//
int
sEC::ec_btn_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (!EC)
        return (true);
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

    int start = 0;
    for ( ; start < x; start++) {
        if (line_start[start] == '\n') {
            // pointing to right of line end
            delete [] string;
            return (true);
        }
    }
    if (isspace(line_start[start])) {
        // pointing at white space
        delete [] string;
        return (true);
    }
    int end = start;
    while (start > 0 && !isspace(line_start[start]))
        start--;
    if (isspace(line_start[start]))
        start++;
    if (start == 0) {
        // pointing at first word (cell name)
        delete [] string;
        return (true);
    }
    while (line_start[end] && !isspace(line_start[end]))
        end++;

    char buf[256];
    strncpy(buf, line_start, end);
    buf[end] = 0;

    const char *yn = 0;
    if (buf[start] == 'y')
        yn = "no ";
    else if (buf[start] == 'n')
        yn = "yes";
    if (yn) {
        start += (line_start - string);
        GdkColor *c = gtk_PopupColor(
            *yn == 'n' ? GRattrColorNo : GRattrColorYes);
        text_replace_chars(caller, c, yn, start, start+3);
        char *cname = lstring::gettok(&line_start);
        if (cname) {
            for (e_item *s = EC->ec_list; s->name; s++) {
                if (!strcmp(s->name, cname)) {
                    s->del = (*yn != 'n');
                    break;
                }
            }
            delete cname;
        }
    }
    delete [] string;

    return (true);
}

