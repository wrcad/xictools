
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
#include "undolist.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "cd_hypertext.h"
#include "cd_propnum.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "gtkprpty.h"
#include "gtkinterf/gtkfont.h"
#include "gtkinterf/gtkutil.h"


//--------------------------------------------------------------------------
// Pop up to view object properties
//

namespace {
    GtkTargetEntry target_table[] = {
        { (char*)"property",     0, 0 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

    namespace gtkprpinfo {
        struct sPi : public sPbase
        {
            sPi(CDo*);
            ~sPi();

            void update(CDo*);
            void purge(CDo*, CDo*);
        };

        sPi *Pi;
    }
}

using namespace gtkprpinfo;


// Static function.
sPbase *
sPbase::prptyInfoPtr()
{
    return (Pi);
}


// Pop up the property panel, list the properties of odesc.
//
void
cEdit::PopUpPropertyInfo(CDo *odesc, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Pi;
        return;
    }
    if (Pi) {
        Pi->update(odesc);
        return;
    }
    if (mode == MODE_UPD)
        return;
    if (!odesc)
        return;

    new sPi(odesc);
    if (!Pi->Shell()) {
        delete Pi;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Pi->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_UR), Pi->Shell(), mainBag()->Viewport());
    gtk_widget_show(Pi->Shell());
}


// Called when odold is being deleted.
//
void
cEdit::PropertyInfoPurge(CDo *odold, CDo *odnew)
{
    if (Pi)
        Pi->purge(odold, odnew);
}
// End of cEdit functions.


sPi::sPi(CDo *odesc)
{
    Pi = this;
    wb_shell = gtk_NewPopup(0, "Properties", pi_cancel_proc, 0);
    if (!wb_shell)
        return;

    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    //
    // scrolled text area
    //
    GtkWidget *contr;
    text_scrollable_new(&contr, &wb_textarea, FNT_FIXED);

    gtk_signal_connect(GTK_OBJECT(wb_textarea), "button-press-event",
        GTK_SIGNAL_FUNC(pi_text_btn_hdlr), 0);
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "button-release-event",
        GTK_SIGNAL_FUNC(pi_text_btn_release_hdlr), 0);

    // dnd stuff
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "motion-notify-event",
        GTK_SIGNAL_FUNC(pi_motion_hdlr), 0);
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "drag-data-get",
        GTK_SIGNAL_FUNC(pi_drag_data_get), 0);
    GtkDestDefaults DD = (GtkDestDefaults)
        (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP);
    gtk_drag_dest_set(wb_textarea, DD, target_table, n_targets,
        GDK_ACTION_COPY);
    gtk_signal_connect_after(GTK_OBJECT(wb_textarea), "drag-data-received",
        GTK_SIGNAL_FUNC(pi_drag_data_received), 0);
    gtk_signal_connect_after(GTK_OBJECT(wb_textarea), "realize",
        GTK_SIGNAL_FUNC(text_realize_proc), 0);

    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
    const char *bclr = GRpkgIf()->GetAttrColor(GRattrColorLocSel);
    gtk_text_buffer_create_tag(textbuf, "primary", "background", bclr,
#if GTK_CHECK_VERSION(2,8,0)
        "paragraph-background", bclr,
#endif
        NULL);

    // for passing hypertext via selections, see gtkhtext.cc
    gtk_object_set_data(GTK_OBJECT(wb_textarea), "hyexport", (void*)2);

    gtk_widget_set_usize(wb_textarea, 300, 200);

    // The font change pop-up uses this to redraw the widget
    gtk_object_set_data(GTK_OBJECT(wb_textarea), "font_changed",
        (void*)pi_font_changed);

    gtk_table_attach(GTK_TABLE(form), contr, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // dismiss button
    //
    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(pi_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    gtk_window_set_focus(GTK_WINDOW(wb_shell), button);

    update(odesc);
}


sPi::~sPi()
{
    Pi = 0;
    if (pi_odesc)
        DSP()->ShowCurrentObject(ERASE, pi_odesc, HighlightingColor);
    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(pi_cancel_proc), wb_shell);
}


void
sPi::update(CDo *odesc)
{
    if (pi_odesc)
        DSP()->ShowCurrentObject(ERASE, pi_odesc, HighlightingColor);
    PrptyText::destroy(pi_list);
    pi_list = 0;
    if (odesc)
        pi_odesc = odesc;
    if (pi_odesc) {
        CDs *cursd = CurCell();
        pi_list = (cursd ? XM()->PrptyStrings(pi_odesc, cursd) : 0);
    }
    update_display();
    if (pi_odesc)
        DSP()->ShowCurrentObject(DISPLAY, pi_odesc, HighlightingColor);
}


void
sPi::purge(CDo *odold, CDo *odnew)
{
    if (odold == pi_odesc) {
        if (odnew)
            ED()->PopUpPropertyInfo(odnew, MODE_UPD);
        else
            ED()->PopUpPropertyInfo(0, MODE_OFF);
    }
}
// End of sPi functions.


PrptyText *
sPbase::resolve(int offset, CDo **odp)
{
    if (odp)
        *odp = pi_odesc;
    for (PrptyText *p = pi_list; p; p = p->next()) {
        if (offset >= p->start() && offset < p->end())
            return (p);
    }
    return (0);
}


// Return the PrptyText element corresponding to the selected line, or 0 if
// there is no selection.
//
PrptyText *
sPbase::get_selection()
{
    int start, end;
    start = pi_start;
    end = pi_end;
    if (start == end)
        return (0);
    for (PrptyText *p = pi_list; p; p = p->next()) {
        if (start >= p->start() && start < p->end())
            return (p);
    }
    return (0);
}


void
sPbase::update_display()
{
    GdkColor *c1 = gtk_PopupColor(GRattrColorHl4);
    GdkColor *c2 = gtk_PopupColor(GRattrColorHl2);
    GdkColor *c3 = gtk_PopupColor(GRattrColorHl1);
    double val = text_get_scroll_value(wb_textarea);
    text_set_chars(wb_textarea, "");

    if (!pi_list)
        text_insert_chars_at_point(wb_textarea, c1,
            pi_odesc ? "No properties found." : "No selection.", -1, -1);

    else {
        int cnt = 0;
        for (PrptyText *p = pi_list; p; p = p->next()) {
            p->set_start(cnt);

            GdkColor *c, *cx = 0;
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
                case P_NAME:
                    // Indicate name property set
                    if (pi_odesc && pi_odesc->type() == CDINSTANCE) {
                        CDp_cname *pn =
                            (CDp_cname*)OCALL(pi_odesc)->prpty(P_NAME);
                        if (pn && pn->assigned_name())
                            cx = c3;
                    }
                    // fallthrough
                case P_RANGE:
                case P_MODEL:
                case P_VALUE:
                case P_PARAM:
                case P_OTHER:
                case P_NOPHYS:
                case P_FLATTEN:
                case P_SYMBLC:
                case P_DEVREF:
                    c = c1;
                    break;
                default:
                    c = 0;
                    break;
                }
            }
            text_insert_chars_at_point(wb_textarea, c, p->head(), -1, -1);
            cnt += strlen(p->head());
            text_insert_chars_at_point(wb_textarea, cx, p->string(), -1, -1);
            text_insert_chars_at_point(wb_textarea, 0, "\n", -1, -1);
            cnt += strlen(p->string());
            p->set_end(cnt);
            cnt++;
        }
    }
    text_set_scroll_value(wb_textarea, val);
    pi_line_selected = -1;
}


// Select the chars in the range, start=end deselects existing.  In
// GTK-1, selecting gives blue inverse, which turns gray if
// unselected, retaining an indication for the buttons.  GTK-2
// doesn't do this automatically so we provide something similar here.
//
void
sPbase::select_range(int start, int end)
{
    GtkTextBuffer *textbuf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(wb_textarea));
    GtkTextIter istart, iend;
    if (pi_end != pi_start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, pi_start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, pi_end);
        gtk_text_buffer_remove_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    text_select_range(wb_textarea, start, end);
    if (end != start) {
        gtk_text_buffer_get_iter_at_offset(textbuf, &istart, start);
        gtk_text_buffer_get_iter_at_offset(textbuf, &iend, end);
        gtk_text_buffer_apply_tag_by_name(textbuf, "primary", &istart, &iend);
    }
    pi_start = start;
    pi_end = end;
}


int
sPbase::button_dn(GtkWidget *caller, GdkEvent *event, void*)
{
    pi_dragging = false;
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
        PrptyText *p = pi_list;
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
        if (p && pi_line_selected != y) {
            pi_line_selected = y;
            select_range(p->start() + strlen(p->head()), p->end());
            if (pi_btn_callback)
                (*pi_btn_callback)(p);
            delete [] string;
            pi_drag_x = (int)event->button.x;
            pi_drag_y = (int)event->button.y;
            pi_dragging = true;
            return (true);
        }
    }
    pi_line_selected = -1;
    delete [] string;
    select_range(0, 0);
    return (true);
}


int
sPbase::button_up(GtkWidget*, GdkEvent*, void*)
{
    pi_dragging = false;
    return (false);
}


int
sPbase::motion(GtkWidget *widget, GdkEvent *event, void*)
{
    if (pi_dragging) {
#if GTK_CHECK_VERSION(2,12,0)
        if (event->motion.is_hint)
            gdk_event_request_motions((GdkEventMotion*)event);
#else
        // Strange voodoo to "turn on" motion events, that are
        // otherwise suppressed since GDK_POINTER_MOTION_HINT_MASK
        // is set.  See GdkEventMask doc.
        gdk_window_get_pointer(widget->window, 0, 0, 0);
#endif
        if ((abs((int)event->motion.x - pi_drag_x) > 4 ||
                abs((int)event->motion.y - pi_drag_y) > 4) &&
                get_selection()) {
            pi_dragging = false;
            GtkTargetList *targets = gtk_target_list_new(target_table,
                n_targets);
            GdkDragContext *drcx = gtk_drag_begin(widget, targets,
                (GdkDragAction)GDK_ACTION_COPY, 1, event);
            gtk_drag_set_icon_default(drcx);
            return (true);
        }
    }
    return (false);
}


void
sPbase::drag_data_get(GtkSelectionData *selection_data)
{
    if (GTK_IS_TEXT_VIEW(wb_textarea)) {
        // stop text view native handler
        gtk_signal_emit_stop_by_name(GTK_OBJECT(wb_textarea), "drag-data-get");
    }

    PrptyText *p = get_selection();
    if (!p)
        return;

    int sz = 0;
    char *bf = 0;
    if (p->prpty()) {
        CDs *cursd =  CurCell(true);
        if (cursd) {
            hyList *hp = cursd->hyPrpList(pi_odesc, p->prpty());
            char *s = hyList::string(hp, HYcvAscii, true);
            hyList::destroy(hp);
            sz = sizeof(int) + strlen(s) + 1;
            bf = new char[sz];
            *(int*)bf = p->prpty()->value();
            strcpy(bf + sizeof(int), s);
            delete [] s;
        }
    }
    else {
        char *s = text_get_chars(wb_textarea, p->start() + strlen(p->head()),
            p->end());
        sz = sizeof(int) + strlen(s) + 1;
        bf = new char[sz];
        const char *q = p->head();
        if (!isdigit(*q))
            q++;
        *(int*)bf = atoi(q);
        strcpy(bf + sizeof(int), s);
        delete [] s;
    }
    gtk_selection_data_set(selection_data, selection_data->target,
        8, (unsigned char*)bf, sz);
    delete [] bf;
}


void
sPbase::data_received(GtkWidget *caller, GdkDragContext *context,
    GtkSelectionData *data, guint time)
{
    bool success = false;
    if (data->target == gdk_atom_intern("property", true) &&
            caller != gtk_drag_get_source_widget(context)) {
        if (!pi_odesc) {
            dspPkgIf()->RegisterTimeoutProc(3000, pi_bad_cb, this);
            PopUpMessage("Can't add property, no object selected.", false,
                false, false, GRloc(LW_LR));
        }
        else {
            int num = *(int*)data->data;
            unsigned char *val = data->data + sizeof(int);
            bool accept = false;
            // Note: the window text is updated by call to PrptyRelist() in
            // CommitChangges()
            if (DSP()->CurMode() == Electrical) {
                if (pi_odesc->type() == CDINSTANCE) {
                    if (num == P_MODEL || num == P_VALUE || num == P_PARAM ||
                            num == P_OTHER || num == P_NOPHYS ||
                            num == P_FLATTEN || num == P_SYMBLC ||
                            num == P_RANGE || num == P_DEVREF) {
                        CDs *cursde = CurCell(Electrical, true);
                        if (cursde) {
                            Ulist()->ListCheck("ddprp", cursde, false);
                            CDp *pdesc =
                                num != P_OTHER ? OCALL(pi_odesc)->prpty(num)
                                : 0;
                            hyList *hp = new hyList(cursde, (char*)val,
                                HYcvAscii);
                            ED()->prptyModify(OCALL(pi_odesc), pdesc, num,
                                0, hp);
                            hyList::destroy(hp);
                            Ulist()->CommitChanges(true);
                            accept = true;
                        }
                    }
                }
            }
            else {
                CDs *cursdp = CurCell(Physical);
                if (cursdp) {
                    Ulist()->ListCheck("ddprp", cursdp, false);
                    DSP()->ShowOdescPhysProperties(pi_odesc, ERASE);

                    CDp *newp = new CDp((char*)val, num);
                    Ulist()->RecordPrptyChange(cursdp, pi_odesc, 0, newp);

                    Ulist()->CommitChanges(true);
                    DSP()->ShowOdescPhysProperties(pi_odesc, DISPLAY);
                    accept = true;
                }
            }
            if (!accept) {
                dspPkgIf()->RegisterTimeoutProc(3000, pi_bad_cb, this);
                PopUpMessage("Can't add property, incorrect type.", false,
                    false, false, GRloc(LW_LR));
            }
        }
    }
    gtk_drag_finish(context, success, false, time);
}


// Static functions.
void
sPbase::pi_cancel_proc(GtkWidget*, void*)
{
    ED()->PopUpPropertyInfo(0, MODE_OFF);
}


// Static function.
// This is called when the font is changed.
//
void
sPbase::pi_font_changed()
{
    if (Pi)
        Pi->update_display();
}


// Static function.
int
sPbase::pi_bad_cb(void *arg)
{
    sPi *pi = (sPi*)arg;
    if (pi && pi->wb_message)
        pi->wb_message->popdown();
    return (false);
}


// Static functions.
// Handle button presses in the text area.
//
int
sPbase::pi_text_btn_hdlr(GtkWidget *caller, GdkEvent *event, void *arg)
{
    if (Pi)
        return (Pi->button_dn(caller, event, arg));
    return (false);
}


// Static functions.
int
sPbase::pi_text_btn_release_hdlr(GtkWidget *caller, GdkEvent *event, void *arg)
{
    if (Pi)
        return (Pi->button_up(caller, event, arg));
    return (false);
}


// Static functions.
int
sPbase::pi_motion_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (Pi)
        return (Pi->motion(caller, event, 0));
    return (false);
}


// Static functions.
void
sPbase::pi_drag_data_get(GtkWidget*, GdkDragContext*,
    GtkSelectionData *selection_data, guint, guint, void*)
{
    if (Pi)
        Pi->drag_data_get(selection_data);
}


// Static functions.
void
sPbase::pi_drag_data_received(GtkWidget *caller, GdkDragContext *context, gint,
    gint, GtkSelectionData *data, guint, guint time)
{
    if (Pi)
        Pi->data_received(caller, context, data, time);
}

