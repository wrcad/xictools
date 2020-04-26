
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
#include "select.h"
#include "cd_celldb.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkmenu.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkfont.h"


//--------------------------------------------------------------------------
//
// Pop up to allow the user to select/deselect cell instances.
//

namespace {
    void start_modal(GtkWidget *w)
    {
        gtkMenu()->SetSensGlobal(false);
        gtkMenu()->SetModal(w);
        dspPkgIf()->SetOverrideBusy(true);
        DSPmainDraw(ShowGhost(ERASE))
    }


    void end_modal()
    {
        gtkMenu()->SetModal(0);
        gtkMenu()->SetSensGlobal(true);
        dspPkgIf()->SetOverrideBusy(false);
        DSPmainDraw(ShowGhost(DISPLAY))
    }
}


namespace {
    // List element for cell instances.
    //
    struct ci_item
    {
        ci_item()
            {
                cdesc = 0;
                name = 0;
                index = 0;
                sel = false;
            }

        CDc *cdesc;                         // instance desc
        const char *name;                   // master cell name
        int index;                          // instance index
        bool sel;                           // selection flag
    };

    namespace gtkselinst {
        struct sCI : public gtk_bag
        {
            sCI(CDol*, bool = false);
            ~sCI();

            void update(CDol*);

        private:
            void refresh();

            static void ci_font_changed();
            static void ci_cancel_proc(GtkWidget*, void*);
            static void ci_btn_proc(GtkWidget*, void*);
            static int ci_btn_hdlr(GtkWidget*, GdkEvent*, void*);

            ci_item *ci_list;                   // list of cell instances
            GtkWidget *ci_label;                // label widget
            int ci_field;                       // max cell name length
            bool ci_filt;                       // instance filtering mode
        };

        sCI *CI;
        CDol *ci_return;

        enum { CI_nil, CI_select, CI_desel };
    }
}

using namespace gtkselinst;


// Modal pop-up allows selection/deselection of the instances in list.
//
void
cMain::PopUpSelectInstances(CDol *list)
{
    if (!GRX || !mainBag())
        return;
    if (RunMode() != ModeNormal)
        return;
    if (CI) {
        CI->update(list);
        return;
    }
    if (!list)
        return;

    new sCI(list);
    if (!CI->Shell()) {
        delete CI;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(CI->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_LL), CI->Shell(), mainBag()->Viewport());
    gtk_widget_show(CI->Shell());

    start_modal(CI->Shell());
    GRX->MainLoop();  // wait for user's response
}


// Modal pop-up returns a new list of instances from those passed,
// user can control whether to keep or ignore.
//
// The argument is consumed, do not use after calling this function.
//
CDol *
cMain::PopUpFilterInstances(CDol *list)
{
    if (!GRX || !mainBag())
        return (list);
    if (RunMode() != ModeNormal)
        return (list);
    if (CI) {
        CI->update(list);
        return (list);
    }
    if (!list)
        return (list);

    new sCI(list, true);
    if (!CI->Shell()) {
        delete CI;
        return (list);
    }
    CDol::destroy(list);

    gtk_window_set_transient_for(GTK_WINDOW(CI->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_LL), CI->Shell(), mainBag()->Viewport());
    gtk_widget_show(CI->Shell());

    ci_return = 0;
    start_modal(CI->Shell());
    GRX->MainLoop();  // wait for user's response
    return (ci_return);
}
// End of cMain functions.


sCI::sCI(CDol *l, bool filtmode)
{
    CI = this;
    ci_list = 0;
    ci_label = 0;
    ci_field = 0;
    ci_filt = filtmode;

    wb_shell = gtk_NewPopup(0,
        ci_filt ? "Choose Instances" : "Select Instances", ci_cancel_proc, 0);
    if (!wb_shell)
        return;
    GtkWidget *form = gtk_table_new(2, 4, false);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    gtk_widget_show(form);

    GtkWidget *button = gtk_button_new_with_label(
        ci_filt ? "Choose All" : "Select All");
    gtk_widget_set_name(button, "SelectAll");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ci_btn_proc), (void*)CI_select);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label(
        ci_filt ? "Ignore All" : "Desel All");
    gtk_widget_set_name(button, "DeselAll");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ci_btn_proc), (void*)CI_desel);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);

    ci_label = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(ci_label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(ci_label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(ci_label), 4, 2);
    gtk_widget_show(ci_label);
    gtk_container_add(GTK_CONTAINER(frame), ci_label);

    // Use a fixed font in the label, same as the text area, so can
    // match columns.
    GTKfont::setupFont(ci_label, FNT_FIXED, true);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 2, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *contr;
    text_scrollable_new(&contr, &wb_textarea, FNT_FIXED);

    gtk_widget_add_events(wb_textarea, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "button-press-event",
        GTK_SIGNAL_FUNC(ci_btn_hdlr), 0);
    gtk_signal_connect_after(GTK_OBJECT(wb_textarea), "realize",
        GTK_SIGNAL_FUNC(text_realize_proc), 0);

    // The font change pop-up uses this to redraw the widget
    gtk_object_set_data(GTK_OBJECT(wb_textarea), "font_changed",
        (void*)ci_font_changed);

    gtk_table_attach(GTK_TABLE(form), contr, 0, 2, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);

    button = gtk_button_new_with_label(
        ci_filt ? "Continue" : "Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(ci_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    update(l);
    int ww = (ci_field + 6)*GTKfont::stringWidth(wb_textarea, 0);
    if (ww < 200)
        ww = 200;
    else if (ww > 600)
        ww = 600;
    ww += 15;  // scrollbar
    gtk_widget_set_size_request(GTK_WIDGET(wb_textarea), ww, -1);
}


sCI::~sCI()
{
    CI = 0;
    if (ci_filt) {
        CDol *c0 = 0, *cn = 0;
        for (ci_item *s = ci_list; s->name; s++) {
            if (s->sel) {
                if (!c0)
                    c0 = cn = new CDol(s->cdesc, 0);
                else {
                    cn->next = new CDol(s->cdesc, 0);
                    cn = cn->next;
                }
            }
        }
        ci_return = c0;
    }
    delete [] ci_list;
    if (wb_shell) {
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(ci_cancel_proc), wb_shell);
    }
    if (GRX->LoopLevel() > 1)
        GRX->BreakLoop();
    end_modal();
}


void
sCI::update(CDol *ol)
{
    if (ol) {
        int sz = 0;
        for (CDol *o = ol; o; o = o->next)
            sz++;
        ci_list = new ci_item[sz+1];
        ci_field = 0;
        ci_item *itm = ci_list;
        for (CDol *o = ol; o; o = o->next) {
            if (o->odesc->type() != CDINSTANCE)
                continue;
            CDc *cd = (CDc*)o->odesc;
            CDs *prnt = cd->parent();
            if (!prnt)
                continue;
            if (!prnt->isInstNumValid())
                prnt->numberInstances();

            itm->cdesc = cd;
            itm->name = Tstring(cd->cellname());
            itm->index = cd->index();
            itm->sel = false;
            int w = strlen(itm->name);
            if (w > ci_field)
                ci_field = w;
            itm++;
        }
        ci_field += 5;

        char lab[256];
        strcpy(lab, "Cell Instances        Click on yes/no\n");
        char *t = lab + strlen(lab);
        for (int i = 0; i <= ci_field; i++)
            *t++ = ' ';
        strcpy(t, ci_filt ? "Choose? " : "Select?");

        gtk_label_set_text(GTK_LABEL(ci_label), lab);
    }
    refresh();
}


// Redraw the text area.
//
void
sCI::refresh()
{
    if (CI && ci_list) {
        char buf[256];
        GdkColor *nc = gtk_PopupColor(GRattrColorNo);
        GdkColor *yc = gtk_PopupColor(GRattrColorYes);
        double val = text_get_scroll_value(wb_textarea);
        text_set_chars(wb_textarea, "");
        for (ci_item *s = ci_list; s->name; s++) {
            sprintf(buf, "%s%c%d", s->name, CD_INST_NAME_SEP, s->index);
            int len = strlen(buf);
            char *e = buf + len;
            while (len <= ci_field) {
                *e++ = ' ';
                len++;
            }
            *e = 0;
            text_insert_chars_at_point(wb_textarea, 0, buf, -1, -1);
            if (!ci_filt)
                s->sel = (s->cdesc->state() == CDobjSelected);
            sprintf(buf, "%-3s\n", s->sel ? "yes" : "no");
            text_insert_chars_at_point(wb_textarea, s->sel ? yc : nc, buf,
                -1, -1);
        }
        text_set_scroll_value(wb_textarea, val);
    }
}


// Static function.
// This is called when the font is changed.
//
void
sCI::ci_font_changed()
{
    if (CI)
        CI->refresh();
}


// Static function.
void
sCI::ci_cancel_proc(GtkWidget*, void*)
{
    delete CI;
}


namespace {
    void apply(ci_item *s)
    {
        CDs *sd = CurCell();
        if (s->cdesc->state() == CDobjVanilla && s->sel) {
            if (XM()->IsBoundaryVisible(sd, s->cdesc)) {
                Selections.insertObject(sd, s->cdesc);
            }
        }
        else if (s->cdesc->state() == CDobjSelected && !s->sel) {
            if (XM()->IsBoundaryVisible(sd, s->cdesc)) {
                Selections.removeObject(sd, s->cdesc);
            }
        }
    }
}


// Static function.
void
sCI::ci_btn_proc(GtkWidget*, void *client_data)
{
    if (!CI)
        return;
    int mode = (long)client_data;
    if (mode == CI_select) {
        for (ci_item *s = CI->ci_list; s->name; s++) {
            s->sel = true;
            if (!CI->ci_filt)
                apply(s);
        }
        CI->refresh();
    }
    else if (mode == CI_desel) {
        for (ci_item *s = CI->ci_list; s->name; s++) {
            s->sel = false;
            if (!CI->ci_filt)
                apply(s);
        }
        CI->refresh();
    }
}


// Static function.
// Handle button presses in the text area.
//
int
sCI::ci_btn_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (!CI)
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
            char *e = strrchr(cname, CD_INST_NAME_SEP);
            if (e) {
                *e++ = 0;
                int ix = atoi(e);
                for (ci_item *s = CI->ci_list; s->name; s++) {
                    if (!strcmp(s->name, cname) && ix == s->index) {
                        s->sel = (*yn != 'n');
                        if (!CI->ci_filt)
                            apply(s);
                        break;
                    }
                }
            }
            delete cname;
        }
    }
    delete [] string;

    return (true);
}

