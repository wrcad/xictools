
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id: gtksearch.cc,v 2.7 2017/04/13 17:06:10 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "gtkinterf.h"
#include "gtksearch.h"
#include "lstring.h"

#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#include "libregex/regex.h"
#endif


//
// A text search pop-up, used with the text editor.
//

namespace {
    // XPM
    const char * up_xpm[] = {
    "32 18 3 1",
    " 	c none",
    ".	c blue",
    "x	c sienna",
    "                                ",
    "                                ",
    "               .                ",
    "              ...x              ",
    "             .....x             ",
    "            .......x            ",
    "           .........x           ",
    "          ...........x          ",
    "         .............x         ",
    "        ...............x        ",
    "       .................x       ",
    "      ...................x      ",
    "     .....................x     ",
    "    .......................x    ",
    "     xxxxxxxxxxxxxxxxxxxxxxx    ",
    "                                ",
    "                                ",
    "                                "};

    // XPM
    const char * down_xpm[] = {
    "32 18 3 1",
    " 	c none",
    ".	c blue",
    "x	c sienna",
    "                                ",
    "                                ",
    "     xxxxxxxxxxxxxxxxxxxxxxx    ",
    "    .......................x    ",
    "     .....................x     ",
    "      ...................x      ",
    "       .................x       ",
    "        ...............x        ",
    "         .............x         ",
    "          ...........x          ",
    "           .........x           ",
    "            .......x            ",
    "             .....x             ",
    "              ...x              ",
    "               .                ",
    "                                ",
    "                                ",
    "                                "};
}


// The cb and arg are optional.  If cb is null, the searchwin is
// assumed to contain the text.  With cb, the searchwin is used for
// positioning.  In any case searchwin must be provided.

GTKsearchPopup::GTKsearchPopup(GRobject caller, GtkWidget *searchwin,
    GtkSearchCb cb, void *arg)
{
    s_caller = caller;
    s_searchwin = searchwin;
    s_cb = cb;
    s_arg = arg;
    s_popup = 0;
    s_text = 0;
    s_label = 0;
    s_dn = 0;
    s_up = 0;
    s_igncase = 0;
    s_last_search = 0;
    s_timer_id = 0;
}


GTKsearchPopup::~GTKsearchPopup()
{
    if (s_popup)
        pop_up_search(MODE_OFF);
    delete [] s_last_search;
}


// The search dialog pop-up.
//
void
GTKsearchPopup::pop_up_search(int mode)
{
    if (mode == MODE_OFF) {
        if (s_cb)
            (*s_cb)(0, false, false, s_arg);
        gtk_signal_disconnect_by_func(GTK_OBJECT(s_popup),
            GTK_SIGNAL_FUNC(search_cancel), this);
        if (s_timer_id) {
            gtk_timeout_remove(s_timer_id);
            s_timer_id = 0;
        }
        if (s_caller)
            GRX->Deselect(s_caller);
        gtk_widget_destroy(s_popup);
        s_popup = 0;
        return;
    }
    if (s_popup)
        return;
    if (!s_searchwin)
        return;
    s_popup = gtk_NewPopup(0, "Search", search_cancel, this);

    GtkWidget *form = gtk_table_new(1,3, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(s_popup), form);

    // The label, in a frame.
    //
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    s_label = gtk_label_new("Enter search expression:");
    gtk_widget_show(s_label);
    gtk_misc_set_padding(GTK_MISC(s_label), 2, 2);
    gtk_container_add(GTK_CONTAINER(frame), s_label);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    // The entry area.
    //
    s_text = gtk_entry_new();
    gtk_widget_show(s_text);
    if (s_last_search)
        gtk_entry_set_text(GTK_ENTRY(s_text), s_last_search);
    gtk_table_attach(GTK_TABLE(form), s_text, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 0);

    gtk_window_set_focus(GTK_WINDOW(s_popup), s_text);

    // The button line.
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    s_dn = new_pixmap_button(down_xpm, 0, false);
    gtk_widget_set_name(s_dn, "SearchDown");
    gtk_widget_show(s_dn);
    gtk_signal_connect(GTK_OBJECT(s_dn), "clicked",
        GTK_SIGNAL_FUNC(search_action), this);
    gtk_box_pack_start(GTK_BOX(hbox), s_dn, true, true, 0);

    s_up = new_pixmap_button(up_xpm, 0, false);
    gtk_widget_set_name(s_up, "SearchUp");
    gtk_widget_show(s_up);
    gtk_signal_connect(GTK_OBJECT(s_up), "clicked",
        GTK_SIGNAL_FUNC(search_action), this);
    gtk_box_pack_start(GTK_BOX(hbox), s_up, true, true, 0);

    s_igncase = gtk_check_button_new_with_label(" No Case ");
    gtk_widget_set_name(s_igncase, "NoCase");
    gtk_widget_show(s_igncase);
    gtk_box_pack_start(GTK_BOX(hbox), s_igncase, true, true, 0);
    gtk_signal_connect(GTK_OBJECT(s_igncase), "clicked",
        GTK_SIGNAL_FUNC(search_action), this);

    GtkWidget *button = gtk_button_new_with_label(" Dismiss ");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(search_cancel), this);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    GtkWidget *parent = s_searchwin;
    while (parent->parent)
        parent = parent->parent;
    gtk_window_set_transient_for(GTK_WINDOW(s_popup), GTK_WINDOW(parent));
    GRX->SetPopupLocation(GRloc(), s_popup, s_searchwin);
    gtk_widget_show(s_popup);
}


// Private static GTK signal handler.
// Cancel proc for the search dialog.
//
void
GTKsearchPopup::search_cancel(GtkWidget*, void *client_data)
{
    GTKsearchPopup *w = static_cast<GTKsearchPopup*>(client_data);
    if (w)
        w->pop_up_search(MODE_OFF);
}


// Private static GTK signal handler.
// Action for the search dialog.
//
void
GTKsearchPopup::search_action(GtkWidget *caller, void *client_data)
{
    GTKsearchPopup *w = static_cast<GTKsearchPopup*>(client_data);
    if (!w)
        return;
    if (caller == w->s_dn) {
        char *target = gtk_editable_get_chars(GTK_EDITABLE(w->s_text), 0, -1);
        if (target && *target) {
            delete [] w->s_last_search;
            w->s_last_search = lstring::copy(target);
            if (w->s_cb) {
                if (!(*w->s_cb)(w->s_last_search, false,
                        GRX->GetStatus(w->s_igncase), w->s_arg)) {
                    gtk_label_set_text(GTK_LABEL(w->s_label), "Not found");
                    w->s_timer_id = gtk_timeout_add(3000,
                        (GtkFunction)fix_label_timeout, w);
                }
                free(target);
                return;
            }
            int start = text_get_insertion_point(w->s_searchwin);
            int sp, ep;
            text_get_selection_pos(w->s_searchwin, &sp, &ep);
            if (sp != ep) {
                if (start < ep)
                   start = ep;
            }
            char *s = text_get_chars(w->s_searchwin, start, -1);

            char *err;
            int end;
            bool ign_case = GRX->GetStatus(w->s_igncase);
            int nst = scan_text(s, target, ign_case, &err, &end);
            if (nst >= 0) {
                text_scroll_to_pos(w->s_searchwin, start + end);
                text_select_range(w->s_searchwin, nst + start,
                    end + start);
            }
            else if (err) {
                if (!w->s_timer_id) {
                    gtk_label_set_text(GTK_LABEL(w->s_label), err);
                    w->s_timer_id = gtk_timeout_add(5000,
                        (GtkFunction)fix_label_timeout, w);
                }
                delete [] err;
            }
            else {
                if (!w->s_timer_id) {
                    gtk_label_set_text(GTK_LABEL(w->s_label), "Not found");
                    w->s_timer_id = gtk_timeout_add(3000,
                        (GtkFunction)fix_label_timeout, w);
                }
            }
            delete [] s;
        }
        free(target);
    }
    else if (caller == w->s_up) {
        char *target = gtk_editable_get_chars(GTK_EDITABLE(w->s_text), 0, -1);
        if (target && *target) {
            delete [] w->s_last_search;
            w->s_last_search = lstring::copy(target);
            if (w->s_cb) {
                if (!(*w->s_cb)(w->s_last_search, true,
                        GRX->GetStatus(w->s_igncase), w->s_arg)) {
                    gtk_label_set_text(GTK_LABEL(w->s_label), "Not found");
                    w->s_timer_id = gtk_timeout_add(3000,
                        (GtkFunction)fix_label_timeout, w);
                }
                free(target);
                return;
            }
            int start = text_get_insertion_point(w->s_searchwin);
            int sp, ep;
            text_get_selection_pos(w->s_searchwin, &sp, &ep);
            if (sp != ep) {
                if (start > sp)
                   start = sp;
            }
            char *s = text_get_chars(w->s_searchwin, 0, start);

            bool ign_case = GRX->GetStatus(w->s_igncase);
            int last_nst = -1;
            int last_os = 0, last_end = 0;
            char *t = s;
            for (;;) {
                char *err;
                int end;
                int nst = scan_text(t, target, ign_case, &err, &end);
                if (nst >= 0) {
                    last_nst = nst;
                    last_os = (t - s);
                    last_end = end;
                    t += end;
                    continue;
                }
                break;
            }
            if (last_nst >= 0) {
                last_nst += last_os;
                last_end += last_os;
                text_scroll_to_pos(w->s_searchwin, last_nst);
                text_select_range(w->s_searchwin, last_nst, last_end);
                delete [] s;
                free(target);
                return;
            }

            delete [] s;
            if (!w->s_timer_id) {
                gtk_label_set_text(GTK_LABEL(w->s_label), "Not found");
                w->s_timer_id = gtk_timeout_add(3000,
                    (GtkFunction)fix_label_timeout, w);
            }
        }
        free(target);
    }
}


// Private static callback.
// Reset the search popup label after an interval.
//
int
GTKsearchPopup::fix_label_timeout(void *client_data)
{
    GTKsearchPopup *w = static_cast<GTKsearchPopup*>(client_data);
    if (w) {
        w->s_timer_id = 0;
        gtk_label_set_text(GTK_LABEL(w->s_label), "Enter search expression:");
    }
    return (false);
}


// Static function.
// Return the character index in text of the match beginning, or -1 if
// no match.  The end position is returned in the arg.  If there is a
// boo-boo return an error message in err.
//
int
GTKsearchPopup::scan_text(char *text, char *target, bool ign_case, char **err,
    int *end)
{
    *err = 0;

    unsigned flags = REG_EXTENDED | REG_NEWLINE;
    if (ign_case)
        flags |= REG_ICASE;
    regex_t preg;
    if (regcomp(&preg, target, flags)) {
        *err = lstring::copy("Syntax error in regular expression.");
        return (-1);
    }

    int ret = 0;
    regmatch_t pmatch[1];
    if (!regexec(&preg, text, 1, pmatch, 0)) {
        ret = pmatch[0].rm_so;
        *end = pmatch[0].rm_eo;
    }
    else {
        ret = -1;
        *end = 0;
    }

    regfree(&preg);
    return (ret);
}

