
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: gtkcflags.cc,v 5.20 2017/04/18 03:13:55 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "gtkmain.h"
#include "gtkfont.h"
#include "gtkinlines.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "cd_celldb.h"
#include "editif.h"


//--------------------------------------------------------------------------
// Pop-up to set cell flags (immutable, library).
//

namespace {
    struct cf_elt
    {
        cf_elt(CDcellName n, bool i, bool l)
            {
                next = 0;
                name = n;
                immutable = i;
                library = l;
            }

        static void destroy(const cf_elt *e)
            {
                while (e) {
                    const cf_elt *ex = e;
                    e = e->next;
                    delete ex;
                }
            }

        cf_elt *next;
        CDcellName name;
        bool immutable;
        bool library;
    };

    namespace gtkcflags {
        struct sCF : public gtk_bag
        {
            sCF(GRobject, const stringlist*, int);
            ~sCF();

            void update(const stringlist*, int);

        private:
            void init();
            void set(int, int);
            void refresh(bool = false);

            static void cf_font_changed();
            static void cf_cancel_proc(GtkWidget*, void*);
            static void cf_btn_proc(GtkWidget*, void*);
            static int cf_btn_hdlr(GtkWidget*, GdkEvent*, void*);

            GRobject cf_caller;
            GtkWidget *cf_label;
            cf_elt *cf_list;                    // list of cells
            int cf_field;                       // name field width
            int cf_dmode;                       // display mode of cells
        };

        sCF *CF;
    }
}

using namespace gtkcflags;


// Pop up a list of cell names, and enable changing the Immutable and
// Library flags.  The dmode specifies the display mode:  Physical or
// Electrical.
//
void
cMain::PopUpCellFlags(GRobject caller, ShowMode mode, const stringlist *list,
    int dmode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete CF;
        return;
    }
    if (mode == MODE_UPD) {
        if (CF)
            CF->update(list, dmode);
        return;
    }
    if (CF) {
        CF->update(list, dmode);
        return;
    }

    new sCF(caller, list, dmode);
    if (!CF->Shell()) {
        delete CF;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(CF->Shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(LW_LL), CF->Shell(), mainBag()->Viewport());
    gtk_widget_show(CF->Shell());
}
// End of cMain functions.


sCF::sCF(GRobject caller, const stringlist *sl, int dmode)
{
    CF = this;
    cf_caller = caller;
    cf_label = 0;
    cf_list = 0;
    cf_field = 0;
    cf_dmode = dmode;

    wb_shell = gtk_NewPopup(0, "Set Cell Flags", cf_cancel_proc, 0);
    if (!wb_shell)
        return;
    GtkWidget *form = gtk_table_new(4, 1, false);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);
    gtk_widget_show(form);

    GtkWidget *label = gtk_label_new("IMMUTABLE");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    gtk_widget_show(label);

    int rowcnt = 0;
    gtk_table_attach(GTK_TABLE(form), label, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    label = gtk_label_new("LIBRARY");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    gtk_widget_show(label);

    gtk_table_attach(GTK_TABLE(form), label, 2, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *button = gtk_button_new_with_label("None");
    gtk_widget_set_name(button, "NoneImmutable");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cf_btn_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label("All");
    gtk_widget_set_name(button, "AllImmutable");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cf_btn_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 1, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label("None");
    gtk_widget_set_name(button, "NoneLibrary");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cf_btn_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 2, 3, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label("All");
    gtk_widget_set_name(button, "AllLibrary");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cf_btn_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 3, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);

    cf_label = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(cf_label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(cf_label), 0, 0.5);
    gtk_widget_show(cf_label);
    gtk_container_add(GTK_CONTAINER(frame), cf_label);

    // Use a fixed font in the label, same as the text area, so can
    // match columns.
    GTKfont::setupFont(cf_label, FNT_FIXED, true);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    rowcnt++;

    GtkWidget *contr;
    text_scrollable_new(&contr, &wb_textarea, FNT_FIXED);

    gtk_widget_add_events(wb_textarea, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(wb_textarea), "button-press-event",
        GTK_SIGNAL_FUNC(cf_btn_hdlr), 0);
    gtk_signal_connect_after(GTK_OBJECT(wb_textarea), "realize",
        GTK_SIGNAL_FUNC(text_realize_proc), 0);

    // The font change pop-up uses this to redraw the widget
    gtk_object_set_data(GTK_OBJECT(wb_textarea), "font_changed",
        (void*)cf_font_changed);

    gtk_table_attach(GTK_TABLE(form), contr, 0, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);
    rowcnt++;

    button = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(button, "Apply");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cf_btn_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 0, 2, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "Dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(cf_cancel_proc), 0);

    gtk_table_attach(GTK_TABLE(form), button, 2, 4, rowcnt, rowcnt+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    update(sl, dmode);
}


sCF::~sCF()
{
    CF = 0;
    cf_elt::destroy(cf_list);
    if (cf_caller)
        GRX->Deselect(cf_caller);
    if (wb_shell)
        gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
            GTK_SIGNAL_FUNC(cf_cancel_proc), wb_shell);
}


void
sCF::update(const stringlist *sl, int dmode)
{
    if (!sl) {
        refresh(true);
        return;
    }

    cf_dmode = dmode;
    cf_elt::destroy(cf_list);
    cf_list = 0;
    cf_elt *ce = 0;

    while (sl) {
        if (cf_dmode == Physical) {
            CDs *sd = CDcdb()->findCell(sl->string, Physical);
            if (sd) {
                if (!cf_list)
                    cf_list = ce = new cf_elt(sd->cellname(),
                        sd->isImmutable(), sd->isLibrary());
                else {
                    ce->next = new cf_elt(sd->cellname(),
                        sd->isImmutable(), sd->isLibrary());
                    ce = ce->next;
                }
            }
        }
        else {
            CDs *sd = CDcdb()->findCell(sl->string, Electrical);
            if (sd) {
                if (!cf_list)
                    cf_list = ce = new cf_elt(sd->cellname(),
                        sd->isImmutable(), sd->isLibrary());
                else {
                    ce->next = new cf_elt(sd->cellname(),
                        sd->isImmutable(), sd->isLibrary());
                    ce = ce->next;
                }
            }
        }
        int w = strlen(sl->string);
        if (w > cf_field)
            cf_field = w;
        sl = sl->next;
    }

    char lab[256];
    strcpy(lab, "Cells in memory - set flags (click on yes/no)\n");
    char *t = lab + strlen(lab);
    for (int i = 0; i <= cf_field + 1; i++)
        *t++ = ' ';
    strcpy(t, "Immut Libr");
    gtk_label_set_text(GTK_LABEL(cf_label), lab);

    int ww = (cf_field + 12)*GTKfont::stringWidth(wb_textarea, 0);
    if (ww < 200)
        ww = 200;
    else if (ww > 600)
        ww = 600;
    ww += 15;  // scrollbar
    gtk_widget_set_usize(GTK_WIDGET(wb_textarea), ww, -1);

    refresh();
}


// Set a flag for all cells in list.
//
void
sCF::set(int im, int lb)
{
    for (cf_elt *cf = cf_list; cf; cf = cf->next) {
        if (im >= 0)
            cf->immutable = im;
        if (lb >= 0)
            cf->library = lb;
    }
}


// Redraw the text area.
//
void
sCF::refresh(bool check_cc)
{
    if (CF) {
        CDs *cursdesc = check_cc && DSP()->CurMode() == cf_dmode ?
            CurCell(DSP()->CurMode()) : 0;
        char buf[256];
        GdkColor *nc = gtk_PopupColor(GRattrColorNo);
        GdkColor *yc = gtk_PopupColor(GRattrColorYes);
        double val = text_get_scroll_value(wb_textarea);
        text_set_chars(wb_textarea, "");
        for (cf_elt *cf = cf_list; cf; cf = cf->next) {
            if (cursdesc && cf->name == cursdesc->cellname()) {
                cf->immutable = cursdesc->isImmutable();
                cf->library = cursdesc->isLibrary();
            }
            sprintf(buf, "%-*s  ", cf_field, Tstring(cf->name));
            text_insert_chars_at_point(wb_textarea, 0, buf, -1, -1);
            const char *yn = cf->immutable ? "yes" : "no";
            sprintf(buf, "%-3s  ", yn);
            text_insert_chars_at_point(wb_textarea, *yn == 'y' ? yc : nc,
                buf, -1, -1);
            yn = cf->library ? "yes" : "no";
            sprintf(buf, "%-3s\n", yn);
            text_insert_chars_at_point(wb_textarea, *yn == 'y' ? yc : nc, buf,
                -1, -1);
        }
        text_set_scroll_value(wb_textarea, val);
    }
}


// Static function.
// This is called when the font is changed
//
void
sCF::cf_font_changed()
{
    if (CF)
        CF->refresh();
}


// Static function.
void
sCF::cf_cancel_proc(GtkWidget*, void*)
{
    XM()->PopUpCellFlags(0, MODE_OFF, 0, 0);
}


// Static function.
void
sCF::cf_btn_proc(GtkWidget *caller, void*)
{
    if (!CF)
        return;
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;
    if (!strcmp(name, "AllImmutable")) {
        CF->set(1, -1);
        CF->refresh();
    }
    else if (!strcmp(name, "AllLibrary")) {
        CF->set(-1, 1);
        CF->refresh();
    }
    else if (!strcmp(name, "NoneImmutable")) {
        CF->set(0, -1);
        CF->refresh();
    }
    else if (!strcmp(name, "NoneLibrary")) {
        CF->set(-1, 0);
        CF->refresh();
    }
    else if (!strcmp(name, "Apply")) {
        CDs *cursd = CurCell(DSP()->CurMode());
        bool cc_changed = false;
        for (cf_elt *cf = CF->cf_list; cf; cf = cf->next) {
            if (CF->cf_dmode == Physical) {
                CDs *sd = CDcdb()->findCell(cf->name, Physical);
                if (sd) {
                    sd->setImmutable(cf->immutable);
                    sd->setLibrary(cf->library);
                    if (sd == cursd)
                        cc_changed = true;
                }
            }
            else {
                CDs *sd = CDcdb()->findCell(cf->name, Electrical);
                if (sd) {
                    sd->setImmutable(cf->immutable);
                    sd->setLibrary(cf->library);
                    if (sd == cursd)
                        cc_changed = true;
                }
            }
        }
        XM()->PopUpCellFlags(0, MODE_OFF, 0, 0);
        XM()->ShowParameters(0);
        if (cc_changed)
            EditIf()->setEditingMode(!cursd || !cursd->isImmutable());
    }
}


// Static function.
// Handle button presses in the text area
//
int
sCF::cf_btn_hdlr(GtkWidget *caller, GdkEvent *event, void*)
{
    if (!CF)
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
    int col = start > CF->cf_field + 5 ? 1 : 0;
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
            for (cf_elt *cf = CF->cf_list; cf; cf = cf->next) {
                if (!strcmp(cname, Tstring(cf->name))) {
                    if (col == 0) {
                        if (*yn == 'y')
                            cf->immutable = true;
                        else if (*yn == 'n')
                            cf->immutable = false;
                    }
                    if (col == 1) {
                        if (*yn == 'y')
                            cf->library = true;
                        else if (*yn == 'n')
                            cf->library = false;
                    }
                    break;
                }
            }
            delete [] cname;
        }
    }
    delete [] string;
    return (true);
}

