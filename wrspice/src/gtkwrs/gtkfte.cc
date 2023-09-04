
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/**************************************************************************
 *
 * Popups for plots, vectors, circuits, etc.
 *
 **************************************************************************/

#include "config.h"
#include "simulator.h"
#include "graph.h"
#include "output.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "gtktoolb.h"
#include "gtkinterf/gtkfont.h"
#include "gtkinterf/gtkpfiles.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#endif


// Keywords referenced in help database:
//  plotspanel
//  vectorspanel
//  circuitspanel
//  filespanel
//  tracepanel
//  variablespanel

// Default window size, assumes 6X13 chars, 80 cols, 16 rows
// with a 2-pixel margin
#define DEF_TWIDTH 484
#define DEF_THEIGHT 212


namespace {
    // Handler for the dismiss buttons.
    //
    void
    tp_cancel_proc(GtkWidget*, void *client_data)
    {
        GtkWidget *which = (GtkWidget*)client_data;

        if (which == TB()->pl_shell)
            TB()->PopDownPlots();
        else if (which == TB()->ve_shell)
            TB()->PopDownVectors();
        else if (which == TB()->ci_shell)
            TB()->PopDownCircuits();
        else if (which == TB()->tr_shell)
            TB()->PopDownTrace();
        else if (which == TB()->va_shell)
            TB()->PopDownVariables();
    }
}


void
GTKtoolbar::SuppressUpdate(bool suppress)
{
    tb_suppress_update = suppress;
}


//===========================================================================
// Pop up to display a list of the plots.  Pointing at the list selects the
// 'current' plot.
// Buttons:
//  cancel: pop down
//  help:   bring up help panel
//  new:    create and add a 'new' plot, making it current
//  delete: delete the current plot

struct sPlots
{
    sPlots(int x, int y, const char *s)
        {
            TB()->pl_shell = TB()->TextPop(x, y, "Plots",
                "Currently Active Plots", s, pl_btn_hdlr, pl_btns, 2,
                pl_actions);
        }
private:
    static void pl_actions(GtkWidget*, void*);
    static void pl_dfunc();
    static void pl_btn_hdlr(GtkWidget*, int, int);

    static const char *pl_btns[];
};

const char *sPlots::pl_btns[] = { "New Plot", "Delete Current" };


// Static function.
// Handle the command button presses
//
void
sPlots::pl_actions(GtkWidget *caller, void *client_data)
{
    if (client_data == (void*)1)
        // 'New' button pressed, create a new plot and make it current.
        OP.setCurPlot("new");
    else if (client_data == (void*)2) {
        // 'Delete' button pressed, ask for confirmation.
        if (OP.curPlot() == OP.constants()) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "can't destroy constants plot.\n");
        }
        else
            TB()->RUsure(TB()->pl_shell, pl_dfunc);
    }
    GTKdev::Deselect(caller);
}


// Static function.
// Callback to destroy the current plot
//
void
sPlots::pl_dfunc()
{
    OP.removePlot(OP.curPlot());
}


// Static function.
// Handle mouse button presses.  Determine which plot entry was pointed to,
// and make it the current plot.
//
void
sPlots::pl_btn_hdlr(GtkWidget *caller, int x, int y)
{
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter iline;
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    y = gtk_text_iter_get_line(&iline);

    sPlot *p;
    int i;
    for (i = 0, p = OP.plotList(); p; i++, p = p->next_plot())
        if (i == y)
            break;
    if (p) {
        OP.setCurPlot(p);
        OP.curPlot()->run_commands();
        TB()->UpdatePlots(0);
        if (p->circuit()) {
            Sp.OptUpdate();
            Sp.SetCircuit(p->circuit());
        }
    }
}
// End of sPlots functions


void
GTKtoolbar::PopUpPlots(int x, int y)
{
    if (pl_shell)
        return;
    sLstr lstr;
    for (sPlot *p = OP.plotList(); p; p = p->next_plot()) {
        char buf[256];
        if (OP.curPlot() == p)
            snprintf(buf, sizeof(buf), "Current %-11s%-20s (%s)\n",
            p->type_name(), p->title(), p->name());
        else
            snprintf(buf, sizeof(buf), "        %-11s%-20s (%s)\n",
                p->type_name(), p->title(), p->name());
        lstr.add(buf);
    }
    FixLoc(&x, &y);
    sPlots plts(x, y, lstr.string());
    pl_text = (GtkWidget*)g_object_get_data(G_OBJECT(pl_shell), "text");
    SetActive(ntb_plots, true);
}


// Pop down and destroy the list of plots.
//
void
GTKtoolbar::PopDownPlots()
{
    if (!pl_shell)
        return;
    SetLoc(ntb_plots, pl_shell);

    GtkWidget *confirm = (GtkWidget*)g_object_get_data(G_OBJECT(pl_shell),
        "confirm");
    if (confirm)
        gtk_widget_destroy(confirm);

    GTKdev::SetStatus(tb_plots, false);
    g_signal_handlers_disconnect_by_func(G_OBJECT(pl_shell),
        (gpointer)tp_cancel_proc, pl_shell);
    gtk_widget_destroy(pl_shell);
    pl_shell = 0;

    SetActive(ntb_plots, false);
}


// Update the list of plots.  This should be called whenever the internal
// plot list changes.  If called with 'lev' nonzero, defer updates until
// second call with same lev value.
//
void
GTKtoolbar::UpdatePlots(int lev)
{
    static int level;
    if (tb_suppress_update)
        return;
    UpdateVectors(lev);
    if (!pl_shell)
        return;
    if (lev) {
        if (level < lev) {
            level = lev;
            return;
        }
        else if (level == lev)
            level = 0;
        else
            return;
    }
    else if (level)
        return;

    char buf[512];
    sLstr lstr;
    for (sPlot *p = OP.plotList(); p; p = p->next_plot()) {
        if (OP.curPlot() == p) {
            snprintf(buf, sizeof(buf), "Current %-11s%-20s (%s)\n",
            p->type_name(), p->title(), p->name());
        }
        else {
            snprintf(buf, sizeof(buf), "        %-11s%-20s (%s)\n",
                p->type_name(), p->title(), p->name());
        }
        lstr.add(buf);
    }

    double val = text_get_scroll_value(pl_text);
    text_set_chars(pl_text, lstr.string());
    text_set_scroll_value(pl_text, val);
}


//===========================================================================
// Pop up to display the list of vectors for the current plot.  Point at
// entries to select them (indicated by '>' in the first column.
// Buttons:
//  cancel: pop down
//  help:   bring up help panel
//  print:  print the selected vectors
//  plot    plot the selected vectors
//  delete: delete the selected vectors

struct sVectors
{
    sVectors(int x, int y, const char *s)
        {
            TB()->ve_shell = TB()->TextPop(x, y, "Vectors",
                "Vectors in Current Plot", s, ve_btn_hdlr, ve_btns, 4,
                ve_actions);
        }

    static void update();

private:
    static wordlist *ve_selections();
    static void ve_actions(GtkWidget*, void*);
    static void ve_dfunc();
    static void ve_btn_hdlr(GtkWidget*, int, int);
    static void ve_recolor();
    static void ve_desel();

    static const char *ve_btns[];
};

const char *sVectors::ve_btns[] = { "Desel All", "Print", "Plot", "Delete" };


// Static function.
void
sVectors::update()
{
    GtkWidget *text = TB()->ve_text;
    if (!text)
        return;

    sLstr lstr;
    OP.vecPrintList(0, &lstr);

    double val = text_get_scroll_value(text);
    text_set_chars(text, lstr.string());
    ve_recolor();
    text_set_scroll_value(text, val);
}


// Static function.
// Return a wordlist containing the selected vector names.
//
wordlist *
sVectors::ve_selections()
{
    char buf[128];
    wordlist *wl=0, *wl0=0;
    char *str = text_get_chars(TB()->ve_text, 0, -1);
    const char *t = str;
    int i;
    for (i = 0; *t; t++) {
        if (*t == '\n') {
            i++;
            if (i < 3)
                continue;
            while (*t == '\n')
                t++;
            if (*t == '>') {
                t++;
                while (isspace(*t))
                    t++;
                char *s = buf;
                // Double quote each entry, otherwise vector names
                // that include net expression indexing and similar
                // may not parse correctly.
                *s++ = '"';
                while (*t && !isspace(*t))
                    *s++ = *t++;
                *s++ = '"';
                *s = '\0';
                if (!wl0)
                    wl = wl0 = new wordlist(buf, 0);
                else {
                    wl->wl_next = new wordlist(buf, wl);
                    wl = wl->wl_next;
                }
            }
            if (!*t)
                break;
        }
    }
    delete [] str;
    if (!wl0)
        GRpkg::self()->ErrPrintf(ET_ERROR, "no vectors are selected.\n");
    return (wl0);
}


// Static function.
// Handle command button presses
//
void
sVectors::ve_actions(GtkWidget *caller, void *client_data)
{
    if (client_data == (void*)1) {
        // 'Desel All' button pressed, deselect all selections.
        ve_desel();
    }
    else if (client_data == (void*)2) {
        // 'Print' button pressed, print selected vectors.
        wordlist *wl = ve_selections();
        if (wl) {
            // Deselect now, if the print is aborted at "more", the
            // button won't be deselected otherwise.

            GTKdev::Deselect(caller);
            CommandTab::com_print(wl);
            wordlist::destroy(wl);
            CP.Prompt();
            return;
        }
    }
    else if (client_data == (void*)3) {
        // 'Plot' button pressed, plot selected vectors.
        wordlist *wl = ve_selections();
        if (wl) {
            CommandTab::com_plot(wl);
            wordlist::destroy(wl);
        }
    }
    else if (client_data == (void*)4) {
        // 'Delete' button pressed, ask for confirmation.
        TB()->RUsure(TB()->ve_shell, ve_dfunc);
    }
    GTKdev::Deselect(caller);
}


// Static function.
// Callback to delete the selected vectors.
//
void
sVectors::ve_dfunc()
{
    wordlist *wl = ve_selections();
    if (wl) {
        CommandTab::com_unlet(wl);
        wordlist::destroy(wl);
    }
}


// Static function.
// Handle button pressed in the text area.
//
void
sVectors::ve_btn_hdlr(GtkWidget *caller, int x, int y)
{
    char *string = text_get_chars(TB()->ve_text, 0, -1);
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(caller), &ihere, x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    x = gtk_text_iter_get_offset(&ihere) - gtk_text_iter_get_offset(&iline);
    char *line_start = string + gtk_text_iter_get_offset(&iline);
    y = gtk_text_iter_get_line(&iline);
    if (y <= 3) {
        delete [] string;
        return;
    }

    // is it selected? set to opposite
    bool select = (*line_start == ' ');
    char *t = line_start + 1;
    // get vector's name
    while (isspace(*t))
        t++;
    char buf[128];
    char *s = buf;
    while (*t && !isspace(*t))
        *s++ = *t++;
    *s = '\0';
    // grab the dvec from storage
    sDataVec *dv = (sDataVec*)OP.curPlot()->get_perm_vec(buf);
    if (!dv) {
        delete [] string;
        return;
    }
    // The flags save the selected status persistently.
    if (select)
        dv->set_flags(dv->flags() | VF_SELECTED);
    else
        dv->set_flags(dv->flags() & ~VF_SELECTED);

    int posn = line_start - string;
    delete [] string;
    GdkColor clr, *c = 0;
    if (gdk_color_parse("red", &clr))
        c = &clr;
    text_set_editable(TB()->ve_text, true);
    text_replace_chars(TB()->ve_text, c, select ? ">" : " ", posn, posn+1);
    text_set_editable(TB()->ve_text, false);
}


// Static function.
void
sVectors::ve_recolor()
{
    GdkColor clr, *c = 0;
    if (gdk_color_parse("red", &clr))
        c = &clr;
    bool wasret = true;
    char *string = text_get_chars(TB()->ve_text, 0, -1);
    if (!string)
        return;
    int n = 0;
    text_set_editable(TB()->ve_text, true);
    for (char *s = string; *s; s++) {
        if (wasret) {
            wasret = false;
            if (*s == '>')
                text_replace_chars(TB()->ve_text, c, ">", n, n+1);
        }
        if (*s == '\n')
            wasret = true;
        n++;
    }
    delete [] string;
    text_set_editable(TB()->ve_text, false);
}


// Static function.
void
sVectors::ve_desel()
{
    GtkWidget *text = TB()->ve_text;
    if (!text)
        return;

    GdkColor clr, *c = 0;
    if (gdk_color_parse("red", &clr))
        c = &clr;

    bool wasret = true;
    char *string = text_get_chars(text, 0, -1);
    if (!string)
        return;
    double val = text_get_scroll_value(text);
    int n = 0;
    text_set_editable(text, true);
    for (char *s = string; *s; s++) {
        if (wasret) {
            wasret = false;
            if (*s == '>')
                text_replace_chars(text, c, " ", n, n+1);
        }
        if (*s == '\n')
            wasret = true;
        n++;
    }
    delete [] string;
    text_set_editable(text, false);
    text_set_scroll_value(text, val);
    OP.curPlot()->clear_selected();
}
// End of sVectors functions


void
GTKtoolbar::PopUpVectors(int x, int y)
{
    if (ve_shell)
        return;
    FixLoc(&x, &y);
    sVectors vec(x, y, "");
    ve_text = (GtkWidget*)g_object_get_data(G_OBJECT(ve_shell), "text");
    sVectors::update();
    SetActive(ntb_vectors, true);
}


// Pop down and destroy the vector list.
//
void
GTKtoolbar::PopDownVectors()
{
    if (!ve_shell)
        return;
    SetLoc(ntb_vectors, ve_shell);

    GtkWidget *confirm = (GtkWidget*)g_object_get_data(G_OBJECT(ve_shell),
        "confirm");
    if (confirm)
        gtk_widget_destroy(confirm);

    GTKdev::SetStatus(tb_vectors, false);
    g_signal_handlers_disconnect_by_func(G_OBJECT(ve_shell),
        (gpointer)tp_cancel_proc, ve_shell);
    gtk_widget_destroy(ve_shell);
    ve_shell = 0;

    SetActive(ntb_vectors, false);
}


// Update the vector list.  Called when a new vector is created or a
// vector is destroyed.  The 'lev' if not 0 will prevent updating
// between successive calls with lev set.  Thus, if several vectors are
// changed, the update can be deferred.
//
void
GTKtoolbar::UpdateVectors(int lev)
{
    if (tb_suppress_update)
        return;
    static int level;
    if (!ve_shell)
        return;
    if (lev) {
        if (level < lev) {
            level = lev;
            return;
        }
        else if (level == lev)
            level = 0;
        else
            return;
    }
    else if (level)
        return;

    sVectors::update();
}


//===========================================================================
// Pop up that displays a list of the circuits that have been loaded.
// Pointing at an entry will make it the 'current' circuit.
// Buttons:
//  cancel: pop down
//  help:   bring up help panel
//  delete: delete the current circuit

struct sCircuits
{
    sCircuits(int x, int y, const char *s)
        {
            TB()->ci_shell = TB()->TextPop(x, y, "Circuits",
                "Currently Active Circuits", s, ci_btn_hdlr, ci_btns, 1,
                ci_actions);
        }

    static char *ci_str();
private:
    static void ci_actions(GtkWidget*, void*);
    static void ci_dfunc();
    static void ci_btn_hdlr(GtkWidget*, int, int); 

    static const char *ci_btns[];
};

const char *sCircuits::ci_btns[] = { "Delete Current" };


// Static function.
// Create a string containing the circuit list.
//
char *
sCircuits::ci_str()
{
    if (!Sp.CircuitList())
        return (lstring::copy("There are no circuits loaded."));

    sLstr lstr;
    for (sFtCirc *p = Sp.CircuitList(); p; p = p->next()) {
        char buf[512];
        if (Sp.CurCircuit() == p) {
            snprintf(buf, sizeof(buf), "Current %-6s %s\n", p->name(),
                p->descr());
        }
        else {
            snprintf(buf, sizeof(buf), "        %-6s %s\n", p->name(),
                p->descr());
        }
        lstr.add(buf);
    }
    return (lstr.string_trim());
}


// Static function.
// 'Delete' button pressed, ask for confirmation.
//
void
sCircuits::ci_actions(GtkWidget *caller, void*)
{
    if (Sp.CurCircuit())
        TB()->RUsure(TB()->ci_shell, ci_dfunc);
    else
        GRpkg::self()->ErrPrintf(ET_ERROR, "no current circuit.\n");
    GTKdev::SetStatus(caller, false);
}


// Static function.
// Callback to delete the current circuit.
//
void
sCircuits::ci_dfunc()
{
    delete Sp.CurCircuit();
}


// Static function.
// Handle button presses in the text area.
//
void
sCircuits::ci_btn_hdlr(GtkWidget *caller, int x, int y)
{
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter iline;
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    y = gtk_text_iter_get_line(&iline);

    sFtCirc *p;
    int i;
    for (i = 0, p = Sp.CircuitList(); p; i++, p = p->next()) {
        if (i == y)
            break;
    }
    if (p)
        Sp.SetCircuit(p->name());
}
// End of sCircuits functions


void
GTKtoolbar::PopUpCircuits(int x, int y)
{
    if (ci_shell)
        return;
    char *s = sCircuits::ci_str();
    FixLoc(&x, &y);
    sCircuits ckts(x, y, s);
    ci_text = (GtkWidget*)g_object_get_data(G_OBJECT(ci_shell), "text");
    delete [] s;
    SetActive(ntb_circuits, true);
}


// Pop down and destroy the circuit list.
//
void
GTKtoolbar::PopDownCircuits()
{
    if (!ci_shell)
        return;
    SetLoc(ntb_circuits, ci_shell);

    GtkWidget *confirm = (GtkWidget*)g_object_get_data(G_OBJECT(ci_shell),
        "confirm");
    if (confirm)
        gtk_widget_destroy(confirm);

    GTKdev::SetStatus(tb_circuits, false);
    g_signal_handlers_disconnect_by_func(G_OBJECT(ci_shell),
        (gpointer)tp_cancel_proc, ci_shell);
    gtk_widget_destroy(ci_shell);
    ci_shell = 0;

    SetActive(ntb_circuits, false);
}


// Update the circuit list.  Called when circuit list changes.
//
void
GTKtoolbar::UpdateCircuits()
{
    if (tb_suppress_update)
        return;
    if (!ci_shell)
        return;
    char *s = sCircuits::ci_str();

    double val = text_get_scroll_value(ci_text);
    text_set_chars(ci_text, s);
    delete [] s;
    text_set_scroll_value(ci_text, val);
}


//===========================================================================
// Pop up to display a listing of files found along the 'sourcepath'.
// Buttons:
//  cancel: pop down
//  help:   bring up help panel
//  source: source the circuit pointed to while active
//  edit:   open the editor with file pointed to while active


extern struct sFiles *FL();

struct sFiles : public files_bag
{
    friend sFiles *FL() { return (dynamic_cast<sFiles*>(instance())); }

    sFiles() : files_bag(TB()->context, fi_btns, 3,
        "Files Found Along the Source Path",
        fi_actions, fi_btn_hdlr, fi_listing, fi_cancel_proc, 0) { }

    void setup()
        {
            TB()->fi_text = wb_textarea;
            TB()->fi_edit = f_buttons[0];
            TB()->fi_source = f_buttons[1];
        }

private:
    static void fi_cancel_proc(GtkWidget*, void*);
    static sPathList *fi_listing(int);
    static int fi_btn_hdlr(GtkWidget*, GdkEvent*, void*);
    static void fi_actions(GtkWidget*, void*);

    static const char *fi_btns[];
    static const char *fi_nofiles_msg;
};

const char *sFiles::fi_btns[3] = { "Edit", "Source", "Help" };
const char *sFiles::fi_nofiles_msg = "no files found";


// Static function.
void
sFiles::fi_cancel_proc(GtkWidget*, void*)
{
    TB()->PopDownFiles();
}


// Static function.
sPathList *
sFiles::fi_listing(int cols)
{
    wordlist *wl = CP.VarEval(kw_sourcepath);
    char *path = wordlist::flatten(wl);
    wordlist::destroy(wl);
    sPathList *pl = new sPathList(path, 0, fi_nofiles_msg, 0, 0, cols, false);
    delete [] path;
    return (pl);
}


// Static function.
// Handle button presses in the text area.
//
int
sFiles::fi_btn_hdlr(GtkWidget *caller, GdkEvent *event, void *arg)
{
    if (event->type != GDK_BUTTON_PRESS)
        return (true);

    bool editstate = GTKdev::GetStatus(TB()->fi_edit);
    bool srcstate = GTKdev::GetStatus(TB()->fi_source);

    // set to "current" text win
    FL()->wb_textarea = GTK_WIDGET(caller);
    TB()->fi_text = GTK_WIDGET(caller);

    char *string = text_get_chars(TB()->fi_text, 0, -1);
    if (!strcmp(string, fi_nofiles_msg)) {
        delete [] string;
        return (true);
    }

    files_bag *fb = (files_bag*)arg;
    int x = (int)event->button.x;
    int y = (int)event->button.y;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(caller), &ihere, x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    x = gtk_text_iter_get_offset(&ihere) - gtk_text_iter_get_offset(&iline);
    char *line_start = string + gtk_text_iter_get_offset(&iline);

    if (lstring::prefix(pathexpDirPrefix, line_start)) {
        // directory line
        fb->select_range(caller, 0, 0);
        delete [] string;
        return (true);
    }
    int i;
    for (i = 0; line_start[i] != '\n' && line_start[i] != 0; i++) ;
    if (x >= i) {
        fb->select_range(caller, 0, 0);
        delete [] string;
        return (true);
    }

    char *s = line_start + x;
    if (isspace(*s)) {
        fb->select_range(caller, 0, 0);
        delete [] string;
        return (true);
    }
    while (!isspace(*s))
        s--;
    s++;
    char *start = s;

    char buf[256];
    char *t = buf;
    while(*s && !isspace(*s))
        *t++ = *s++;
    *t = '\0';

    fb->select_range(caller, start - string, s - string);
    delete [] string;

    wordlist wl, wl1;
    if (srcstate) {
        wl.wl_prev = wl.wl_next = 0;
        wl.wl_word = buf;
        CommandTab::com_source(&wl);
        CP.Prompt();
        GTKdev::SetStatus(TB()->fi_source, false);
    }
    else if (editstate) {
        wl1.wl_prev = 0;
        wl1.wl_word = (char*)"-n";
        wl1.wl_next = &wl;
        wl.wl_prev = &wl1;
        wl.wl_word = Sp.FullPath(buf);
        wl.wl_next = 0;
        if (wl.wl_word) {
            CommandTab::com_edit(&wl1);
            delete [] wl.wl_word;
        }
        GTKdev::SetStatus(TB()->fi_edit, false);
    }
    else {
        FL()->f_drag_start = true;
        FL()->f_drag_btn = event->button.button;
        FL()->f_drag_x = (int)event->button.x;
        FL()->f_drag_y = (int)event->button.y;
    }
    return (true);
}


// Static function.
// If there is something selected, perform the action.
//
void
sFiles::fi_actions(GtkWidget *caller, void*)
{
    const char *name = gtk_widget_get_name(caller);
    if (!name)
        return;

    if (!strcmp(name, fi_btns[2])) {
#ifdef HAVE_MOZY
        HLP()->word("filespanel");
#endif
        GTKdev::Deselect(caller);
        return;
    }
    bool state = GTKdev::GetStatus(caller);
    if (!state)
        return;
    int start, end;
    text_get_selection_pos(TB()->fi_text, &start, &end);
    if (start == end)
        return;

    char *string = text_get_chars(TB()->fi_text, start, end);
    char buf[256];
    strcpy(buf, string);
    delete [] string;

    wordlist wl, wl1;
    if (!strcmp(name, fi_btns[1])) {
        wl.wl_prev = wl.wl_next = 0;
        wl.wl_word = buf;
        CommandTab::com_source(&wl);
        CP.Prompt();
    }
    if (!strcmp(name, fi_btns[0])) {
        wl1.wl_prev = 0;
        wl1.wl_word = (char*)"-n";
        wl1.wl_next = &wl;
        wl.wl_prev = &wl1;
        wl.wl_word = Sp.FullPath(buf);
        wl.wl_next = 0;
        if (wl.wl_word) {
            CommandTab::com_edit(&wl1);
            delete [] wl.wl_word;
        }
    }
    GTKdev::Deselect(caller);
}
// End of sFiles functions


void
GTKtoolbar::PopUpFiles(int x, int y)
{
    new sFiles;
    fi_shell = FL()->Shell();
    if (fi_shell) {
        FL()->setup();

        gtk_window_set_transient_for(GTK_WINDOW(fi_shell),
            GTK_WINDOW(TB()->context->Shell()));
        FixLoc(&x, &y);
        GTKdev::self()->SetPopupLocation(GRloc(LW_XYA, x, y), fi_shell,
            TB()->context->Shell());
        gtk_widget_show(fi_shell);
    }
    SetActive(ntb_files, true);
}


// Pop down and destroy the file list.
//
void
GTKtoolbar::PopDownFiles()
{
    if (!fi_shell)
        return;
    SetLoc(ntb_files, fi_shell);

    GTKdev::SetStatus(tb_files, false);
    delete FL();
    fi_shell = 0;

    SetActive(ntb_files, false);
}


// Update the file listing.  Called when the sourcepath changes
//
void
GTKtoolbar::UpdateFiles()
{
    if (tb_suppress_update)
        return;
    if (!fi_shell)
        return;
    wordlist *wl = CP.VarEval(kw_sourcepath);
    char *path = wordlist::flatten(wl);
    wordlist::destroy(wl);
    FL()->update(path);
    delete [] path;
}


//===========================================================================
// Pop up to display the current traces, iplots, etc.
// Buttons:
//  cancel: pop down
//  help:   bring up help panel
//  delete: delete entry

struct sTraces
{
    sTraces(int x, int y, const char *s)
        {
            TB()->tr_shell = TB()->TextPop(x, y, "Trace",
                "Active Traces and Debugs", s, tr_btn_hdlr, tr_btns, 1,
                tr_actions);
        }

    static void update();

private:
    static void tr_actions(GtkWidget*, void*);
    static void tr_dfunc();
    static void tr_btn_hdlr(GtkWidget*, int, int);
    static void tr_recolor();

    static const char *tr_btns[];
};

const char *sTraces::tr_btns[] = { "Delete Inactive" };


void
sTraces::update()
{
    sLstr lstr;
    OP.statusCmd(&lstr);
    GtkWidget *text = TB()->tr_text;
    if (!text)
        return;
    double val = text_get_scroll_value(text);
    text_set_chars(text, lstr.string());
    tr_recolor();
    text_set_scroll_value(text, val);
}


// Static function.
// 'Delete' button pressed, ask for confirmation.
//
void
sTraces::tr_actions(GtkWidget *caller, void*)
{
    char *s = text_get_chars(TB()->tr_text, 0, -1);
    if (*s != 'I') {
        char *t;
        for (t = s; *t; t++) {
            if (*t == '\n') {
                if (*(t+1) == 'I')
               	    break;
            }
        }
        if (!*t) {
            GRpkg::self()->ErrPrintf(ET_ERROR, "no inactive debugs.\n");
            GTKdev::SetStatus(caller, false);
            delete [] s;
            return;
        }
    }
    delete [] s;
    TB()->RUsure(TB()->tr_shell, tr_dfunc);
    GTKdev::SetStatus(caller, false);
}


// Static function.
// Callback to perform the deletions.
//
void
sTraces::tr_dfunc()
{
    OP.deleteRunop(DF_ALL, true, -1);
    TB()->UpdateTrace();
}


// Static function.
// Handle button presses in the text area.
//
void
sTraces::tr_btn_hdlr(GtkWidget *caller, int x, int y)
{
    char *string = text_get_chars(TB()->tr_text, 0, -1);
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(caller), &ihere, x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    x = gtk_text_iter_get_offset(&ihere) - gtk_text_iter_get_offset(&iline);
    char *line_start = string + gtk_text_iter_get_offset(&iline);

    // The first column should start with ' ' for active debugs, 'I'
    // for inactive ones.  Anything else is a 'no debugs in effect'
    // message.
    //
    if (*line_start != ' ' && *line_start != 'I') {
        delete [] string;
        return;
    }

    // is it active? set to opposite
    bool active = (*line_start == ' ') ? false : true;

    int dnum;
    if (sscanf(line_start+2, "%d", &dnum) != 1) {
        delete [] string;
        return;
    }

    OP.setRunopActive(dnum, active);
    int posn = line_start - string;
    delete [] string;

    GdkColor clr, *c = 0;
    if (gdk_color_parse("red", &clr))
        c = &clr;
    text_set_editable(TB()->tr_text, true);
    text_replace_chars(TB()->tr_text, c, active ? " " : "I", posn, posn+1);
    text_set_editable(TB()->tr_text, false);
}


// Static function.
void
sTraces::tr_recolor()
{
    GdkColor clr, *c = 0;
    if (gdk_color_parse("red", &clr))
        c = &clr;
    bool wasret = true;
    char *string = text_get_chars(TB()->tr_text, 0, -1);
    if (!string)
        return;
    int n = 0;
    text_set_editable(TB()->tr_text, true);
    for (char *s = string; *s; s++) {
        if (wasret) {
            wasret = false;
            if (*s == 'I')
                text_replace_chars(TB()->tr_text, c, "I", n, n+1);
        }
        if (*s == '\n')
            wasret = true;
        n++;
    }
    text_set_editable(TB()->tr_text, false);
    delete [] string();
}
// End of sTraces functions


void
GTKtoolbar::PopUpTrace(int x, int y)
{
    if (tr_shell)
        return;
    FixLoc(&x, &y);
    sTraces traces(x, y, "");
    tr_text = (GtkWidget*)g_object_get_data(G_OBJECT(tr_shell), "text");
    sTraces::update();
    SetActive(ntb_trace, true);
}


// Pop down and destroy the trace pop up.
//
void
GTKtoolbar::PopDownTrace()
{
    if (!tr_shell)
        return;
    SetLoc(ntb_trace, tr_shell);

    GtkWidget *confirm = (GtkWidget*)g_object_get_data(G_OBJECT(tr_shell),
        "confirm");
    if (confirm)
        gtk_widget_destroy(confirm);

    GTKdev::SetStatus(tb_trace, false);
    g_signal_handlers_disconnect_by_func(G_OBJECT(tr_shell),
        (gpointer)tp_cancel_proc, tr_shell);
    gtk_widget_destroy(tr_shell);
    tr_shell = 0;

    SetActive(ntb_trace, false);
}


// Update the trace list.  Called when debugs are added or deleted.
//
void
GTKtoolbar::UpdateTrace()
{
    if (tb_suppress_update)
        return;
    if (!tr_shell)
        return;
    sTraces::update();
}


//===========================================================================
// Pop-up to display the current shell variables.
// Buttons:
//  cancel: pop down
//  help:   bring up help panel

struct sVariables
{
    sVariables(int x, int y, const char *s)
        {
            TB()->va_shell = TB()->TextPop(x, y, "Variables",
                "Shell Variables", s, 0, 0, 0, 0);
        }
};
// End of sVariables functions.


void
GTKtoolbar::PopUpVariables(int x, int y)
{
    if (va_shell)
        return;
    sLstr lstr;
    Sp.VarPrint(&lstr);
    FixLoc(&x, &y);
    sVariables vars(x, y, lstr.string());
    va_text = (GtkWidget*)g_object_get_data(G_OBJECT(va_shell), "text");
    SetActive(ntb_variables, true);
}


// Pop down and destroy the variables pop up.
//
void
GTKtoolbar::PopDownVariables()
{
    if (!va_shell)
        return;
    SetLoc(ntb_variables, va_shell);

    GTKdev::SetStatus(tb_variables, false);
    g_signal_handlers_disconnect_by_func(G_OBJECT(va_shell),
        (gpointer)tp_cancel_proc, va_shell);
    gtk_widget_destroy(va_shell);
    va_shell = 0;

    SetActive(ntb_variables, false);
}


// Update the variables list.  Called when variables are added or deleted.
//
void
GTKtoolbar::UpdateVariables()
{
    if (tb_suppress_update)
        return;
    if (!va_shell)
        return;
    sLstr lstr;
    Sp.VarPrint(&lstr);
    double val = text_get_scroll_value(va_text);
    text_set_chars(va_text, lstr.string());
    text_set_scroll_value(va_text, val);
}


//===========================================================================
// Composite used for the popups in this file.
// Consists of a top label and help button, a text area, and a lower
// button row.
//

struct sTextPop
{
    sTextPop(const char*, const char*, const char*,
        void(*)(GtkWidget*, int, int), const char**, int,
        void(*)(GtkWidget*, void*));
    // No destructor, user should never destroy explicitly.

    GtkWidget *show(int, int);

private:
    static void tp_help_proc(GtkWidget*, void*);
    static int tp_btn_hdlr(GtkWidget*, GdkEvent*, void*);
    static void tp_destroy(void*);

    GtkWidget *tp_shell;
    GtkWidget *tp_text;
    int tp_lx;
    int tp_ly;
    void(*tp_callback)(GtkWidget*, int, int);
};


GtkWidget *
GTKtoolbar::TextPop(int x, int y, const char *title, const char *lstring,
    const char *textstr, void (*btn_hdlr)(GtkWidget*, int, int),
    const char **buttons, int numbuttons,
    void (*action_proc)(GtkWidget*, void*))
{
    sTextPop *pop = new sTextPop(title, lstring, textstr, btn_hdlr,
        buttons, numbuttons, action_proc);
    return (pop->show(x, y));
}


sTextPop::sTextPop(const char *title, const char *lstring, const char *textstr,
    void (*btn_hdlr)(GtkWidget*, int, int), const char **buttons,
    int numbuttons, void (*action_proc)(GtkWidget*, void*))
{
    tp_shell = gtk_NewPopup(0, title, tp_cancel_proc, 0);
    tp_text = 0;
    tp_lx = 0;
    tp_ly = 0;
    tp_callback = btn_hdlr;

    g_object_set_data_full(G_OBJECT(tp_shell), "textpop", this, tp_destroy);

    GtkWidget *form = gtk_table_new(1, 3, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(tp_shell), form);

    //
    // title label and help button
    //
    GtkWidget *hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    GtkWidget *label = gtk_label_new(lstring);
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 4, 2);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(hbox), frame, true, true, 0);

    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tp_help_proc), tp_shell);
    gtk_misc_set_padding(GTK_MISC(gtk_bin_get_child(GTK_BIN(button))), 4, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 2);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // scrolled text area
    //
    text_scrollable_new(&hbox, &tp_text, FNT_FIXED);

    g_object_set_data(G_OBJECT(tp_shell), "text", tp_text);
    text_set_chars(tp_text, textstr);

    gtk_widget_add_events(tp_text,
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(G_OBJECT(tp_text), "button_press_event",
        G_CALLBACK(tp_btn_hdlr), this);
    g_signal_connect(G_OBJECT(tp_text), "button_release_event",
        G_CALLBACK(tp_btn_hdlr), this);

    gtk_widget_set_size_request(tp_text, DEF_TWIDTH, DEF_THEIGHT);

    // This will provide an arrow cursor.
    g_signal_connect_after(G_OBJECT(tp_text), "realize",
        G_CALLBACK(text_realize_proc), 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    //
    // dismiss button row
    //
    hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    for (int n = 0; n < numbuttons; n++) {
        button = gtk_toggle_button_new_with_label(buttons[n]);
        gtk_widget_show(button);
        g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(action_proc), (void*)(long)(n+1));
        gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 2);
    }

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(tp_cancel_proc), tp_shell);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 2);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
}


GtkWidget *
sTextPop::show(int x, int y)
{
    if (x || y)
        gtk_window_move(GTK_WINDOW(tp_shell), x, y);
    gtk_window_set_transient_for(GTK_WINDOW(tp_shell),
        GTK_WINDOW(TB()->context->Shell()));
    gtk_widget_show(tp_shell);
    return (tp_shell);
}


// Static function.
// Handler for the help buttons
//
void
sTextPop::tp_help_proc(GtkWidget*, void *client_data)
{
#ifdef HAVE_MOZY
    GtkWidget *which = (GtkWidget*)client_data;

    if (which == TB()->pl_shell)
        HLP()->word("plotspanel");
    else if (which == TB()->ve_shell)
        HLP()->word("vectorspanel");
    else if (which == TB()->ci_shell)
        HLP()->word("circuitspanel");
    else if (which == TB()->fi_shell)
        HLP()->word("filespanel");
    else if (which == TB()->tr_shell)
        HLP()->word("tracepanel");
    else if (which == TB()->va_shell)
        HLP()->word("variablespanel");
#else
    (void)client_data;
#endif
}


// Static function.
// Handler for button actions in the text area.  User can drag to
// select text, or click to call the callback.
//
int
sTextPop::tp_btn_hdlr(GtkWidget *caller, GdkEvent *event, void *arg)
{
    sTextPop *pop = (sTextPop*)arg;
    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button.button == 1) {
            pop->tp_lx = (int)event->button.x;
            pop->tp_ly = (int)event->button.y;
            return (false);
        }
        return (true);
    }
    if (event->type == GDK_BUTTON_RELEASE) {
        if (event->button.button == 1) {
            int x = (int)event->button.x;
            int y = (int)event->button.y;
            if (abs(x - pop->tp_lx) <= 4 && abs(y - pop->tp_ly) <= 4) {
                if (pop->tp_callback)
                    (*pop->tp_callback)(caller, pop->tp_lx, pop->tp_ly);
            }
            return (false);
        }
    }
    return (true);
}


// Static function.
// Destroy proc for the widget, called from the shell destructor.  The
// sTextPop has no destructor, nor will it do anything but probably
// cause seg faults if freed explicitly.
//
void
sTextPop::tp_destroy(void *arg)
{
    delete (sTextPop*)arg;
}


//===========================================================================
// Confirmation pop up for delete operations
//

namespace {
    // Handler for confirmation widget button presses
    //
    void
    ru_sure_proc(GtkWidget *caller, void *client_data)
    {
        void (*yesfunc)() = caller ?
            (void(*)())g_object_get_data(G_OBJECT(caller), "yesfunc") : 0;
        GtkWidget *popup = (GtkWidget*)client_data;

        GtkWidget *parent = (GtkWidget*)g_object_get_data(G_OBJECT(popup),
            "parent");
        if (parent)
            g_object_set_data(G_OBJECT(parent), "confirm", 0);

        g_signal_handlers_disconnect_by_func(G_OBJECT(popup),
            (gpointer)ru_sure_proc, popup);
        gtk_widget_destroy(popup);
        if (yesfunc)
            (*yesfunc)();
    }
}


void
GTKtoolbar::RUsure(GtkWidget *parent, void(*yesfunc)())
{
    
    GtkWidget *popup = (GtkWidget*)g_object_get_data(G_OBJECT(parent),
        "confirm");
    if (popup)
        return;

    // The widget name is the same as used in PopUpError()
    popup = gtk_NewPopup(0, "Confirm", ru_sure_proc, 0);
    g_object_set_data(G_OBJECT(parent), "confirm", popup);
    g_object_set_data(G_OBJECT(popup), "parent", parent);

    GtkWidget *form = gtk_vbox_new(false, 2);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(popup), form);

    GtkWidget *label = gtk_label_new("Are you sure? ");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 0);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_box_pack_start(GTK_BOX(form), frame, false, false, 0);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(form), hbox, false, false, 0);

    GtkWidget *button = gtk_button_new_with_label("Yes, Delete");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(ru_sure_proc), popup);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
    g_object_set_data(G_OBJECT(button), "yesfunc", (void*)yesfunc);
    gtk_misc_set_padding(GTK_MISC(gtk_bin_get_child(GTK_BIN(button))), 4, 0);

    button = gtk_button_new_with_label("Abort");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(ru_sure_proc), popup);
    gtk_box_pack_start(GTK_BOX(hbox), button, true, true, 0);
    gtk_misc_set_padding(GTK_MISC(gtk_bin_get_child(GTK_BIN(button))), 4, 0);

    GTKdev::self()->SetPopupLocation(GRloc(), popup, parent);
    gtk_window_set_transient_for(GTK_WINDOW(popup),
        GTK_WINDOW(TB()->context->Shell()));
    gtk_widget_show(popup);
}

