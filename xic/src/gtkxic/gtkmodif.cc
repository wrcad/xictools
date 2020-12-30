
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
#include "cvrt_variables.h"
#include "fio.h"
#include "cd_celldb.h"
#include "dsp_color.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "gtkmain.h"
#include "gtkmenu.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkfont.h"


//--------------------------------------------------------------------------
// Pop up to question user about saving modified cells
//
// Help system keywords used:
//  xic:sv

namespace {
    void
    start_modal(GtkWidget *w)
    {
        gtkMenu()->SetSensGlobal(false);
        gtkMenu()->SetModal(w);
        dspPkgIf()->SetOverrideBusy(true);
        DSPmainDraw(ShowGhost(ERASE))
    }


    void
    end_modal()
    {
        gtkMenu()->SetModal(0);
        gtkMenu()->SetSensGlobal(true);
        dspPkgIf()->SetOverrideBusy(false);
        DSPmainDraw(ShowGhost(DISPLAY))
    }


    const char *
    file_type_str(FileType ft)
    {
        if (ft == Fnative || ft == Fnone)
            return ("X");
        if (ft == Fgds)
            return ("G");
        if (ft == Fcgx)
            return ("B");
        if (ft == Foas)
            return ("O");
        if (ft == Fcif)
            return ("C");
        if (ft == Foa)
            return ("A");
        return ("");
    }


    // List element for modified cells.
    //
    struct s_item
    {
        s_item() { name = 0; path = 0; save = true; ft[0] = 0; }
        ~s_item() { delete [] name; delete [] path; }

        char *name;                         // cell name
        char *path;                         // full path name
        bool save;                          // save flag
        char ft[4];                         // file type code
    };

    namespace gtkmodif {
        struct sSC : public gtk_bag
        {
            sSC(stringlist*, bool(*)(const char*));
            ~sSC();

            bool is_empty()             { return (!sc_field || !sc_width); }
            static PMretType retval()   { return (sc_retval); }

        private:
            void refresh();

            static void sc_font_changed();
            static void sc_cancel_proc(GtkWidget*, void*);
            static void sc_btn_proc(GtkWidget*, void*);
            static int sc_btn_hdlr(GtkWidget*, GdkEvent*, void*);

            s_item *sc_list;                    // list of cells
            bool (*sc_saveproc)(const char*);   // save callback
            int sc_field;                       // max cell name length
            int sc_width;                       // max total string length

            static PMretType sc_retval;         // return flag
        };

        sSC *SC;

        enum { sc_nil, sc_save, sc_skip, sc_help };
    }
}

using namespace gtkmodif;

PMretType sSC::sc_retval;


// Pop-up to list modified cells, allowing the user to choose which to
// save.  This is a modal pop-up.  The return value is true unless
// saveproc returns false for some file.
//
PMretType
cEdit::PopUpModified(stringlist *list, bool(*saveproc)(const char*))
{
    if (!GRX || !mainBag())
        return (PMok);
    if (SC)
        return (PMok);
    if (!list)
        return (PMok);

    new sSC(list, saveproc);
    if (SC->is_empty()) {
        delete SC;
        return (PMok);
    }
    if (!SC->Shell()) {
        delete SC;
        return (PMerr);
    }
    gtk_window_set_transient_for(GTK_WINDOW(SC->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), SC->Shell(), mainBag()->Viewport());
    gtk_widget_show(SC->Shell());

    start_modal(SC->Shell());
    GRX->MainLoop();  // wait for user's response
    end_modal();

    return (sSC::retval());
}
// End of cEdit functions.


sSC::sSC(stringlist *l, bool(*s)(const char*))
{
    SC = this;
    stringlist::sort(l);
    int sz = stringlist::length(l);
    sc_list = new s_item[sz+1];
    sc_field = 0;
    sc_width = 0;
    s_item *itm = sc_list;
    bool ptoo = FIO()->IsListPCellSubMasters();
    bool vtoo = FIO()->IsListViaSubMasters();
    while (l) {
        CDcbin cbin;
        if (CDcdb()->findSymbol(l->string, &cbin)) {
            if (!ptoo && cbin.phys() && cbin.phys()->isPCellSubMaster()) {
                l = l->next;
                continue;
            }
            if (!vtoo && cbin.phys() && cbin.phys()->isViaSubMaster()) {
                l = l->next;
                continue;
            }

            itm->name = lstring::copy(l->string);
            if (cbin.fileType() == Fnative && cbin.fileName()) {
                itm->path = new char[strlen(cbin.fileName()) +
                    strlen(Tstring(cbin.cellname())) + 2];
                sprintf(itm->path, "%s/%s", cbin.fileName(),
                    Tstring(cbin.cellname()));
            }
            else if (FIO()->IsSupportedArchiveFormat(cbin.fileType()))
                itm->path = FIO()->DefaultFilename(&cbin, cbin.fileType());
            else
                itm->path = lstring::copy(cbin.fileName());
            if (!itm->path)
                itm->path = lstring::copy("(in current directory)");
            strcpy(itm->ft, file_type_str(cbin.fileType()));
            int w = strlen(itm->name);
            if (w > sc_field)
                sc_field = w;
            w += strlen(itm->path);
            w += 9;
            if (w > sc_width)
                sc_width = w;
            if (cbin.phys() && cbin.phys()->isPCellSubMaster()) {
                itm->save = false;
                itm->ft[0] = 'P';
                itm->ft[1] = 0;
            }
            if (cbin.phys() && cbin.phys()->isViaSubMaster()) {
                itm->save = false;
                itm->ft[0] = 'V';
                itm->ft[1] = 0;
            }
            itm++;
        }
        l = l->next;
    }
    sc_saveproc = s;
    sc_retval = PMok;
    if (!sc_field || !sc_width) {
        wb_shell = 0;
        return;
    }

    wb_shell = gtk_NewPopup(0, "Modified Cells", sc_cancel_proc, 0);
    if (!wb_shell)
        return;
    GtkWidget *form = gtk_table_new(2, 4, false);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    gtk_widget_show(form);

    GtkWidget *button = gtk_button_new_with_label("Save All");
    gtk_widget_set_name(button, "SaveAll");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_btn_proc), (void*)sc_save);

    int rowcnt = 0;
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label("Skip All");
    gtk_widget_set_name(button, "SkipAll");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_btn_proc), (void*)sc_skip);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    char lab[256];
    strcpy(lab, "Modified Cells            Click on yes/no \n");
    char *t = lab + strlen(lab);
    for (int i = 0; i <= sc_field + 1; i++)
        *t++ = ' ';
    strcpy(t, "Save?");
    GtkWidget *label = gtk_label_new(lab);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label), 4, 2);
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(frame), label);

    // Use a fixed font in the label, same as the text area, so can
    // match columns.
    GTKfont::setupFont(label, FNT_FIXED, true);

    gtk_box_pack_start(GTK_BOX(hbox), frame, true, true, 0);

    button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "Help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_btn_proc), (void*)sc_help);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 2, rowcnt,  rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *contr;
    text_scrollable_new(&contr, &wb_textarea, FNT_FIXED);

    refresh();
    gtk_widget_add_events(wb_textarea, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "button-press-event",
        GTK_SIGNAL_FUNC(sc_btn_hdlr), 0);
    gtk_signal_connect_after(GTK_OBJECT(wb_textarea), "realize",
        GTK_SIGNAL_FUNC(text_realize_proc), 0);

    int ww = sc_width*GTKfont::stringWidth(wb_textarea, 0);
    if (ww < 200)
        ww = 200;
    else if (ww > 600)
        ww = 600;
    ww += 15;  // scrollbar
    int hh = 8*GTKfont::stringHeight(wb_textarea, 0);
    gtk_widget_set_size_request(GTK_WIDGET(wb_textarea), ww, hh);

    // The font change pop-up uses this to redraw the widget
    gtk_object_set_data(GTK_OBJECT(wb_textarea), "font_changed",
        (void*)sc_font_changed);

    gtk_table_attach(GTK_TABLE(form), contr, 0, 2, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);
    rowcnt++;

    button = gtk_button_new_with_label("Apply - Continue");
    gtk_widget_set_name(button, "Apply");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(sc_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label("ABORT");
    gtk_widget_set_name(button, "Abort");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(sc_cancel_proc), (void*)1);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
}


sSC::~sSC()
{
    SC = 0;
    delete [] sc_list;
    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(sc_cancel_proc), wb_shell);
}


// Redraw the text area.
//
void
sSC::refresh()
{
    if (SC) {
        char buf[256];
        GdkColor *nc = gtk_PopupColor(GRattrColorNo);
        GdkColor *yc = gtk_PopupColor(GRattrColorYes);
        GdkColor *c1 = gtk_PopupColor(GRattrColorHl2);
        GdkColor *c2 = gtk_PopupColor(GRattrColorHl3);
        double val = text_get_scroll_value(wb_textarea);
        text_set_chars(wb_textarea, "");
        for (s_item *s = sc_list; s->name; s++) {
            sprintf(buf, "%-*s  ", sc_field, s->name);
            text_insert_chars_at_point(wb_textarea, 0, buf, -1, -1);
            sprintf(buf, "%-3s  ", s->save ? "yes" : "no");
            text_insert_chars_at_point(wb_textarea, s->save ? yc : nc, buf,
                -1, -1);
            sprintf(buf, "%c ", *s->ft);
            text_insert_chars_at_point(wb_textarea,
                *buf == 'X' || *buf == 'A' || *buf == 'P' ? c1 : c2,
                buf, -1, -1);
            sprintf(buf, "%s\n", s->path);
            text_insert_chars_at_point(wb_textarea, 0, buf, -1, -1);
        }
        text_set_scroll_value(wb_textarea, val);
    }
}


// Static function.
// This is called when the font is changed.
//
void
sSC::sc_font_changed()
{
    if (SC)
        SC->refresh();
}


// Static function.
void
sSC::sc_cancel_proc(GtkWidget*, void *arg)
{
    if (SC) {
        if (arg == (void*)1)
            SC->sc_retval = PMabort;
        else {
            if (SC->sc_saveproc) {
                for (s_item *s = SC->sc_list; s->name; s++) {
                    if (s->save && !(*SC->sc_saveproc)(s->name))
                        SC->sc_retval = PMerr;
                }
                SC->sc_saveproc = 0;
            }
        }
        if (GRX->LoopLevel() > 1)
            GRX->BreakLoop();
        delete SC;
    }
}


// Static function.
void
sSC::sc_btn_proc(GtkWidget*, void *client_data)
{
    if (!SC)
        return;
    int mode = (intptr_t)client_data;
    if (mode == sc_save) {
        for (s_item *s = SC->sc_list; s->name; s++)
            s->save = true;
        SC->refresh();
    }
    else if (mode == sc_skip) {
        for (s_item *s = SC->sc_list; s->name; s++)
            s->save = false;
        SC->refresh();
    }
    else if (mode == sc_skip) {
        DSPmainWbag(PopUpHelp("xic:sv"))
    }
}


// Static function.
// Handle button presses in the text area.
//
int
sSC::sc_btn_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (!SC)
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
            for (s_item *s = SC->sc_list; s->name; s++) {
                if (!strcmp(s->name, cname)) {
                    s->save = (*yn != 'n');
                    break;
                }
            }
            delete cname;
        }
    }
    delete [] string;

    return (true);
}

