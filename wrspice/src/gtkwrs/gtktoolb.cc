
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
 * Toolbar popup for WRspice
 *
 **************************************************************************/

#include "config.h"
#include "spglobal.h"
#include "graph.h"
#include "cshell.h"
#include "commands.h"
#include "simulator.h"
#include "circuit.h"
#include "input.h"
#include "output.h"
#include "kwords_analysis.h"
#include "kwords_fte.h"
#include "gtktoolb.h"
#include "gtkinterf/gtkutil.h"
#include "gtkinterf/gtkfont.h"
#include "gtkinterf/gtkedit.h"
#include "gtkinterf/gtkpfiles.h"
#include "spnumber/spnumber.h"
#include "miscutil/filestat.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#include "gtkmozy/gtkhelp.h"
#endif
#include <signal.h>

#ifdef WITH_X11
#include <gdk/gdkx.h>
#endif
#ifdef WIN32
#include <windows.h>
#else
#include "../../icons/wrspice_16x16.xpm"
#include "../../icons/wrspice_32x32.xpm"
#include "../../icons/wrspice_48x48.xpm"
#endif

#include <gdk/gdkkeysyms.h>

#ifdef WIN32
// Reference a symbol in the resource module so the resources are linked.
extern int ResourceModuleId;
namespace { int dummy = ResourceModuleId; }
#endif


// Instantiate and export the toolbar.
namespace { GTKtoolbar _tb_; }
sToolbar *ToolBar() { return (TB()); }


void
CommandTab::com_setfont(wordlist *wl)
{
    if (!Sp.GetFlag(FT_BATCHMODE) && CP.Display()) {
        if (!wl || !wl->wl_next)
            return;
        int n = atoi(wl->wl_word);
        char *fn = wordlist::flatten(wl->wl_next);
        switch (n) {
        case FNT_FIXED:
        case FNT_PROP:
        case FNT_SCREEN:
        case FNT_EDITOR:
        case FNT_MOZY:
        case FNT_MOZY_FIXED:
            Fnt()->setName(fn, n);
        }
        delete [] fn;
    }
}


//
// The toolbar menus and widgets
//

namespace {
    // See if str means true.
    //
    bool
    affirm(const char *str)
    {
        if (!strcmp(str, "on") || !strcmp(str, "true") || !strcmp(str, "1")
                || !strcmp(str, "yes"))
            return (true);
        return (false);
    }
}


// Command to parse the default toolbar setup.  Must be called before
// Toolbar(), i.e., in the startup script.  Syntax:
// tbsetup [vert] [old] [toolbar on|off x y] [name on|off x y] ...
// The vert and old keywords are ignored in this version
// 'name' must be one of the ntb_xxx keywords.
// on|off (literal) tells whether the popup is up or not at startup.
// x, y are screen coords of upper left corner of respective popup.
//
void
CommandTab::com_tbsetup(wordlist *wl)
{
    if (!CP.Display())
        return;
    if (TB()->Saved())
        return;
    while (wl && (!strcmp(wl->wl_word, "vert") || !strcmp(wl->wl_word, "old")))
        wl = wl->wl_next;
    for ( ; wl; wl = wl->wl_next) {
        char *word = wl->wl_word;
        wl = wl->wl_next;
        if (!wl)
            break;
        char *on = wl->wl_word;
        wl = wl->wl_next;
        const char *posx, *posy;
        posx = posy = "0";
        if (wl) {
            posx = wl->wl_word;
            wl = wl->wl_next;
            if (wl)
                posy = wl->wl_word;
        }

        tbent *ent = TB()->FindEnt(word);
        if (!ent) {
            GRpkg::self()->ErrPrintf(ET_WARN,
                "tbsetup: bad keyword %s ignored.\n", word);
            if (!wl)
                break;
            continue;
        }
        ent->active = affirm(on);
        if (sscanf(posx, "%d", &ent->x) != 1)
            ent->x = 0;
        if (sscanf(posy, "%d", &ent->y) != 1)
            ent->y = 0;
        if (!wl)
            break;
    }
    TB()->SetSaved(true);
}


// Update the startup file for the current tool configuration.
//
void
CommandTab::com_tbupdate(wordlist*)
{
    if (!CP.Display()) {
        GRpkg::self()->ErrPrintf(ET_MSG, "No update needed.\n");
        return;
    }
    char buf[512];
    char *startup_filename;
    bool found = IFsimulator::StartupFileName(&startup_filename);
    char *bkfile = new char[strlen(startup_filename) + 5];
    char *s = lstring::stpcpy(bkfile, startup_filename);
    strcpy(s, ".bak");

    s = 0;
    if (found) {
        if (!filestat::move_file_local(bkfile, startup_filename))
            goto bad;
        FILE *fp = fopen(startup_filename, "w");
        if (!fp)
            goto bad;
        FILE *gp = fopen(bkfile, "r");
        if (!gp)
            goto bad;
        bool contd = false;
        bool wrote = false;
        while (fgets(buf, 512, gp)) {
            if (contd) {
                s = buf + strlen(buf) - 1;
                while (isspace(*s))
                   s--;
                if (*s != '\\') {
                    contd = false;
                    if (!wrote) {
                        s = TB()->ConfigString();
                        if (s) {
                            fputs(s, fp);
                            delete [] s;
                        }
                        wrote = true;
                    }
                }
                continue;
            }
            s = buf;
            while (isspace(*s))
                s++;
            if (!strncmp("tbsetup", s, 7)) {
                s = buf + strlen(buf) - 1;
                while (isspace(*s))
                   s--;
                if (*s == '\\')
                    contd = true;
                else {
                    if (!wrote) {
                        s = TB()->ConfigString();
                        if (s) {
                            fputs(s, fp);
                            delete [] s;
                        }
                        wrote = true;
                    }
                }
                continue;
            }
            if (!strncmp("setfont", s, 7))
                continue;
            fputs(buf, fp);
        }
        if (!wrote) {
            s = TB()->ConfigString();
            if (s) {
                fputs(s, fp);
                delete [] s;
            }
            wrote = true;
        }
        fclose(fp);
        fclose(gp);
    }
    else {
        FILE *fp = fopen(startup_filename, "w");
        fputs("* title line, don't delete!\n", fp);
        if (fp) {
            s = TB()->ConfigString();
            if (s) {
                fputs(s, fp);
                delete [] s;
            }
            fclose(fp);
        }
        else
            goto bad;
    }
    delete [] startup_filename;
    delete [] bkfile;
    return;
bad:
    GRpkg::self()->ErrPrintf(ET_WARN, "could not update %s.\n",
        startup_filename);
    delete [] startup_filename;
    delete [] bkfile;
}
// End of CommandTab functions.


GTKtoolbar::GTKtoolbar()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class GTKtoolbar already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    context = 0;

    toolbar = 0;
    tb_bug = 0;
    bg_shell = 0;
    bg_text = 0;
    tb_circuits = 0;
    ci_shell = 0;
    ci_text = 0;
    tb_colors = 0;
    co_shell = 0;
    tb_commands = 0;
    cm_shell = 0;
    tb_debug = 0;
    db_shell = 0;
    tb_files = 0;
    fi_shell = 0;
    fi_text = 0;
    fi_source = 0;
    fi_edit = 0;
    tb_font = 0;
    ft_shell = 0;
    tb_plotdefs = 0;
    pd_shell = 0;
    tb_plots = 0;
    pl_shell = 0;
    pl_text = 0;
    tb_shell = 0;
    sh_shell = 0;
    tb_simdefs = 0;
    sd_shell = 0;
    tb_runop = 0;
    tr_shell = 0;
    tr_text = 0;
    tb_variables = 0;
    va_shell = 0;
    va_text = 0;
    tb_vectors = 0;
    ve_shell = 0;
    ve_text = 0;

    for (int i = 0; i < TBH_end; i++) {
        tb_kw_help[i] = 0;
        tb_kw_help_pos[i].x = 0;
        tb_kw_help_pos[i].y = 0;
    }

    ntb_bug       = "bug";
    ntb_circuits  = "circuits";
    ntb_colors    = "colors";
    ntb_commands  = "commands";
    ntb_debug     = "debug";
    ntb_files     = "files";
    ntb_font      = "font";
    ntb_plotdefs  = "plotdefs";
    ntb_plots     = "plots";
    ntb_shell     = "shell";
    ntb_simdefs   = "simdefs";
    ntb_toolbar   = "toolbar";
    ntb_runop     = "runop";
    ntb_variables = "variables";
    ntb_vectors   = "vectors";

    // the menu
    entries[0].name = ntb_toolbar;
    entries[1].name = ntb_bug;
    entries[2].name = ntb_font;
    entries[3].name = ntb_files;
    entries[4].name = ntb_circuits;
    entries[5].name = ntb_plots;
    entries[6].name = ntb_plotdefs;
    entries[7].name = ntb_colors;
    entries[8].name = ntb_vectors;
    entries[9].name = ntb_variables;
    entries[10].name = ntb_shell;
    entries[11].name = ntb_simdefs;
    entries[12].name = ntb_commands;
    entries[13].name = ntb_runop;
    entries[14].name = ntb_debug;
    entries[15].name = 0;

    tb_elapsed_start = 0.0;
    tb_dropfile = 0;

    tb_file_menu = 0;
    tb_source_btn = 0;
    tb_load_btn = 0;
    tb_edit_menu = 0;
    tb_tools_menu = 0;

    tb_mailer = 0;
    tb_clr_1 = 0;
    tb_clr_2 = 0;
    tb_clr_3 = 0;
    tb_clr_4 = 0;
    tb_suppress_update = false;
    tb_saved = false;
}

GTKtoolbar *GTKtoolbar::instancePtr;


// Private static error exit.
//
void
GTKtoolbar::on_null_ptr()
{
    fprintf(stderr, "Singleton class GTKtoolbar used before instantiated.\n");
    exit(1);
}


// Pop up the toolbar.
//
void
GTKtoolbar::Toolbar()
{
    if (!CP.Display())
        return;

    gtk_window_set_default_icon_name("wrspice");
    tbpop(true);

    // now launch the application windows that start realized
    for (tbent *tb = entries; tb && tb->name; tb++) {
        if (!tb->active)
            continue;
        tb->active = false;
        menu_proc(0, (gpointer)(tb - entries));
    }
}

// XXX fixme from QT
void
GTKtoolbar::PopUpToolbar(ShowMode, int, int)
{
}

//==========================================================================
//
//  Bug report pop-up

namespace {
    void
    mail_destroy_cb(GReditPopup *w)
    {
        GTKeditPopup *we = dynamic_cast<GTKeditPopup*>(w);
        if (we)
            TB()->SetLoc(TB()->ntb_bug, we->Shell());
    }
}


// Pop up to email bug report text
//
void
GTKtoolbar::PopUpBugRpt(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        if (tb_mailer)
            tb_mailer->popdown();
        return;
    }
    if (!toolbar)
        return;
    if (!tb_mailer) {
        if (!Global.BugAddr()) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "no IP address set for bug reports.");
            return;
        }
        char buf[128];
        snprintf(buf, sizeof(buf), "WRspice %s bug", Global.Version());
        FixLoc(&x, &y);
        tb_mailer = context->PopUpMail(buf, Global.BugAddr(), mail_destroy_cb,
            GRloc(LW_XYA, x, y));
        if (tb_mailer)
            tb_mailer->register_usrptr((void**)&tb_mailer);
    }
}


//==========================================================================
//
//  Tool Font pop-up

namespace {
    void
    font_cb(const char *btn, const char *name, void*)
    {
        if (!name && !btn) {
            TB()->SetLoc(TB()->ntb_font, TB()->ft_shell);
            TB()->SetActive(TB()->ntb_font, false);
            TB()->ft_shell = 0;
            GTKdev::SetStatus(TB()->tb_font, false);
            return;
        }
    }
}


void
GTKtoolbar::PopUpFont(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        if (!ft_shell)
            return;
        SetLoc(ntb_font, ft_shell);
        context->ActiveFontsel()->popdown();
        ft_shell = 0;
        SetActive(ntb_font, false);
        GTKdev::SetStatus(tb_font, false);
        return;
    }
    if (!toolbar)
        return;
    FixLoc(&x, &y);
    context->PopUpFontSel(0, GRloc(LW_XYA, x, y), MODE_ON, font_cb, 0,
        FNT_FIXED);
    ft_shell = ((GTKfontPopup*)context->ActiveFontsel())->Shell();
    SetActive(ntb_font, true);
}


//========================================================================
// Pop up to display keyword help lists.  Pointing at the list entries
// calls the main help routine.  This is called from the popups which
// contain lists of 'set' variables to modify.
//

struct sTBhelp
{
    friend void GTKtoolbar::PopUpTBhelp(ShowMode, GRobject, GRobject, TBH_type);

    sTBhelp(GRobject, GRobject);
    // No destructor, this struct should not be destroyed explicitly.

    GtkWidget *show(TBH_type);

private:
    void select(GtkWidget*, int, int);

    static void th_cancel_proc(GtkWidget*, void*);
    static int th_btn_hdlr(GtkWidget*, GdkEvent*, void*);
    static void th_destroy(void*);

    GtkWidget *th_popup;
    GtkWidget *th_text;

    int th_lx;
    int th_ly;
};


void
GTKtoolbar::PopUpTBhelp(ShowMode mode, GRobject parent, GRobject call_btn,
    TBH_type type)
{
    if (mode == MODE_OFF) {
        if (!tb_kw_help[type])
            return;
        g_signal_handlers_disconnect_by_func(G_OBJECT(tb_kw_help[type]),
            (gpointer)sTBhelp::th_cancel_proc, (gpointer)tb_kw_help[type]);
        gdk_window_get_root_origin(gtk_widget_get_window(tb_kw_help[type]),
            &tb_kw_help_pos[type].x, &tb_kw_help_pos[type].y);
        gtk_widget_destroy(GTK_WIDGET(tb_kw_help[type]));
        tb_kw_help[type] = 0;
        return;
    }
    if (tb_kw_help[type])
        return;
    sTBhelp *th = new sTBhelp(parent, call_btn);
    tb_kw_help[type] = th->show(type);;
    g_object_set_data(G_OBJECT(tb_kw_help[type]), "tbtype",
        (void*)(long)type);
}


sTBhelp::sTBhelp(GRobject parent, GRobject call_btn)
{
    th_popup = gtk_NewPopup(0, "Keyword Help", th_cancel_proc, 0);
    th_text = 0;
    th_lx = 0;
    th_ly = 0;

    g_object_set_data_full(G_OBJECT(th_popup), "tbhelp", this, th_destroy);
    g_object_set_data(G_OBJECT(th_popup), "caller", call_btn);

    GtkWidget *form = gtk_table_new(1, 4, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(th_popup), form);

    //
    // label in frame
    //
    GtkWidget *label = gtk_label_new("Click on entries for more help: ");
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    sLstr lstr;
    int i;
    if (parent == (GRobject)TB()->sh_shell) {
        for (i = 0; KW.shell(i)->word; i++)
            KW.shell(i)->print(&lstr);
    }
    else if (parent == (GRobject)TB()->sd_shell) {
        for (i = 0; KW.sim(i)->word; i++)
            KW.sim(i)->print(&lstr);
    }
    else if (parent == (GRobject)TB()->cm_shell) {
        for (i = 0; KW.cmds(i)->word; i++)
            KW.cmds(i)->print(&lstr);
    }
    else if (parent == (GRobject)TB()->pd_shell) {
        for (i = 0; KW.plot(i)->word; i++)
            KW.plot(i)->print(&lstr);
    }
    else if (parent == (GRobject)TB()->db_shell) {
        for (i = 0; KW.debug(i)->word; i++)
            KW.debug(i)->print(&lstr);
    }
    else
        lstr.add("Internal error.");

    //
    // text area
    //
    GtkWidget *hbox;
    text_scrollable_new(&hbox, &th_text, FNT_FIXED);

    text_set_chars(th_text, lstr.string());

    gtk_widget_add_events(th_text,
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

    g_signal_connect(G_OBJECT(th_text), "button_press_event",
        G_CALLBACK(th_btn_hdlr), this);
    g_signal_connect(G_OBJECT(th_text), "button_release_event",
        G_CALLBACK(th_btn_hdlr), this);

    // This will provide an arrow cursor.
    g_signal_connect_after(G_OBJECT(th_text), "realize",
        G_CALLBACK(text_realize_proc), 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 0);

    int wid = 80*GTKfont::stringWidth(th_text, 0);
    int hei = 12*GTKfont::stringHeight(th_text, 0);
    gtk_window_set_default_size(GTK_WINDOW(th_popup), wid + 8, hei + 20);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_widget_show(sep);
    gtk_table_attach(GTK_TABLE(form), sep, 0, 1, 2, 3,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    //
    // buttons
    //
    GtkWidget *button = gtk_button_new_with_label("Dismiss");
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(th_cancel_proc), th_popup);

    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 3, 4,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
}


GtkWidget *
sTBhelp::show(TBH_type type)
{

    gtk_window_set_transient_for(GTK_WINDOW(th_popup),
        GTK_WINDOW(TB()->context->Shell()));
    if (TB()->tb_kw_help_pos[type].x != 0 && TB()->tb_kw_help_pos[type].y != 0)
        gtk_window_move(GTK_WINDOW(th_popup), TB()->tb_kw_help_pos[type].x,
            TB()->tb_kw_help_pos[type].y);
    gtk_widget_show(th_popup);
    return (th_popup);
}


void
sTBhelp::select(GtkWidget *caller, int x, int y)
{
    char *string = text_get_chars(caller, 0, -1);
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(caller),
        GTK_TEXT_WINDOW_WIDGET, x, y, &x, &y);
    GtkTextIter ihere, iline;
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(caller), &ihere, x, y);
    gtk_text_view_get_line_at_y(GTK_TEXT_VIEW(caller), &iline, y, 0);
    char *line_start = string + gtk_text_iter_get_offset(&iline);

    // find first word
    while (isspace(*line_start) && *line_start != '\n')
        line_start++;
    if (*line_start == 0 || *line_start == '\n') {
        text_select_range(caller, 0, 0);
        delete [] string;
        return;
    }

    int start = line_start - string;
    char buf[64];
    char *s = buf;
    while (!isspace(*line_start))
        *s++ = *line_start++;
    *s = '\0';
    int end = line_start - string;

    text_select_range(caller, start, end);
#ifdef HAVE_MOZY
    HLP()->word(buf);
#endif
    delete [] string;
}


// Static function.
//
void
sTBhelp::th_cancel_proc(GtkWidget*, void *client_data)
{
    GtkWidget *popup = (GtkWidget*)client_data;
    GtkWidget *caller = (GtkWidget*)g_object_get_data(G_OBJECT(popup),
        "caller");
    if (caller)
        GTKdev::Deselect(caller);
    int type = (intptr_t)g_object_get_data(G_OBJECT(popup), "tbtype");
    TB()->PopUpTBhelp(MODE_OFF, 0, 0, (TBH_type)type);
}


// Static function.
//
int
sTBhelp::th_btn_hdlr(GtkWidget *caller, GdkEvent *event, void *arg)
{
    sTBhelp *th = (sTBhelp*)arg;
    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button.button == 1) {
            th->th_lx = (int)event->button.x;
            th->th_ly = (int)event->button.y;
            return (false);
        }
        return (true);
    }
    if (event->type == GDK_BUTTON_RELEASE) {
        if (event->button.button == 1) {
            int x = (int)event->button.x;
            int y = (int)event->button.y;
            if (abs(x - th->th_lx) <= 4 && abs(y - th->th_ly) <= 4)
                th->select(caller, th->th_lx, th->th_ly);
            return (false);
        }
    }
    return (true);
}


// Static function.
//
void
sTBhelp::th_destroy(void *arg)
{
    delete (sTBhelp*)arg;
}


//==========================================================================
//
// Error window


struct ErrMsgBox : public cMsgHdlr
{
    void PopUpErr(const char*);
    void ToLog(const char *s) { first_message(s); }

private:
    void stuff_msg(const char*);

    static void er_wrap_proc(GtkWidget*, void*);
    static int er_btn_hdlr(GtkWidget*, GdkEvent*, void*);
    static void er_cancel_proc(GtkWidget*, void*);

    GtkWidget *er_popup;
    GtkWidget *er_text;
    int er_x;
    int er_y;
    bool er_wrap;
};

namespace { ErrMsgBox MB; }


void
ErrMsgBox::PopUpErr(const char *string)
{
    if (er_popup) {
        // The message is not immediately loaded into the window,
        // instead messages are queued and handled in an idle
        // procedure.  This is for efficiency in handling a huge
        // cascade of messages that can occur.
        //
        append_message(string);
        return;
    }
    first_message(string);
    er_popup = gtk_NewPopup(0, "ERROR", er_cancel_proc, 0);
    gtk_widget_add_events(er_popup, GDK_EXPOSURE_MASK);

    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(er_popup), form);

    // 
    // scrolled text area
    //
    GtkWidget *hbox;
    text_scrollable_new(&hbox, &er_text, FNT_PROP);
    g_object_set_data(G_OBJECT(er_popup), "text", er_text);
    text_set_chars(er_text, string);
    gtk_widget_add_events(er_text, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(er_text), "button_press_event",
        G_CALLBACK(er_btn_hdlr), 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    //
    // the wrap and dismiss buttons
    //
    hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    GtkWidget *wrap = gtk_toggle_button_new_with_label("Wrap Lines");
    gtk_widget_show(wrap);
    g_signal_connect(G_OBJECT(wrap), "clicked",
        G_CALLBACK(er_wrap_proc), 0);
    GTKdev::SetStatus(wrap, er_wrap);
    gtk_box_pack_start(GTK_BOX(hbox), wrap, false, false, 0);

    GtkWidget *cancel = gtk_button_new_with_label("Dismiss");
    gtk_widget_show(cancel);
    g_signal_connect(G_OBJECT(cancel), "clicked",
        G_CALLBACK(er_cancel_proc), er_popup);
    g_object_set_data(G_OBJECT(cancel), "shell", er_popup);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(er_text),
        er_wrap ? GTK_WRAP_WORD_CHAR : GTK_WRAP_NONE);
    gtk_box_pack_start(GTK_BOX(hbox), cancel, true, true, 0);
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    GTKdev::self()->SetDoubleClickExit(er_popup, cancel);
    gtk_widget_set_can_default(cancel, true);
    gtk_window_set_default(GTK_WINDOW(er_popup), cancel);

    int mwid, mhei;
    gtk_MonitorGeom(0, 0, 0, &mwid, &mhei);

    int wid = 400;
    int hei = 120;
    gtk_widget_set_size_request(er_popup, wid, hei);
    if (er_x == 0 && er_y == 0) {
        er_x = (mwid - wid)/2;
        er_y = 0;
    }

    // If there is a current plot window, don't put the error popup on
    // top of it.  The situation can arise that cancelling the error
    // popup causes an fpe in the plot redraw which immediately produces
    // a new error popup, ad infinitum.
    //
    hei += 20;  // for title bar
    if (GRpkg::self()->CurDev()->devtype == GRmultiWindow && GP.Cur() &&
            GP.Cur()->apptype() == GR_PLOT) {
        // can't do this in hardcopy context
        GtkWidget *plot = ((GTKbag*)GP.Cur()->dev())->Shell();
        if (plot) {
            GdkRectangle rect;
            gtk_ShellGeometry(plot, 0, &rect);
            if (er_y < rect.y + rect.height && er_y + hei > rect.y &&
                    er_x < rect.x + wid && er_x + wid > rect.x) {
                if (mhei - (rect.y + rect.height) > rect.y)
                    er_y = rect.y + rect.height + 10;
                else
                    er_y = rect.y - hei - 10;
            }
        }
    }

    // MSW seems to need this before gtk_window_show.
    TB()->RevertFocus(er_popup);

    gtk_window_move(GTK_WINDOW(er_popup), er_x, er_y);
    gtk_widget_show(er_popup);
#if GTK_CHECK_VERSION(3,0,0)
    if (TB()->context && TB()->context->GetDrawable()) {
#else
    if (TB()->context && TB()->context->Window()) {
#endif
        gtk_window_set_transient_for(GTK_WINDOW(er_popup),
            GTK_WINDOW(TB()->context->Shell()));
    }
}


// Add the string to the display, at the end.  We keep only
// MAX_ERR_LINES lines in the display.  Oldest lines are deleted to
// maintain the limit.
//
void
ErrMsgBox::stuff_msg(const char *string)
{
    if (!er_popup || !er_text)
        return;

    char *t;
    char *s = text_get_chars(er_text, 0, -1);
    int cnt = 0;
    for (t = s; *t; t++)
        if (*t == '\n')
            cnt++;
    text_set_editable(er_text, true);
    if (cnt > MAX_ERR_LINES) {
        cnt = 0;
        for ( ; t > s; t--) {
            if (*t == '\n') {
                cnt++;
                if (cnt == MAX_ERR_LINES) {
                    text_delete_chars(er_text, 0, t + 1 - s);
                    break;
                }
            }
        }
    }
    delete [] s;

    text_insert_chars_at_point(er_text, 0, string, -1, -1);
    text_set_editable(er_text, false);
    GtkAdjustment *adj = gtk_text_view_get_vadjustment(GTK_TEXT_VIEW(er_text));
    if (adj && gtk_adjustment_get_value(adj) <
            gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj)) {
        gtk_adjustment_set_value(adj,
            gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj));
        g_signal_emit_by_name(G_OBJECT(adj), "value_changed");
    }
}


// Static function.
void
ErrMsgBox::er_cancel_proc(GtkWidget*, void*)
{
    if (MB.er_popup) {
        if (gtk_widget_is_drawable(MB.er_popup)) {
            gdk_window_get_root_origin(gtk_widget_get_window(MB.er_popup),
                &MB.er_x, &MB.er_y);
        }
        gtk_widget_destroy(MB.er_popup);
        MB.er_popup = 0;
    }
}


// Static function.
void
ErrMsgBox::er_wrap_proc(GtkWidget *btn, void*)
{
    if (MB.er_popup && MB.er_text) {
        MB.er_wrap = GTKdev::GetStatus(btn);
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(MB.er_text),
            MB.er_wrap ? GTK_WRAP_WORD_CHAR : GTK_WRAP_NONE);
    }
}


// Static function.
int
ErrMsgBox::er_btn_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    if (event->type != GDK_BUTTON_PRESS)
        return (true);
    return (false);
}
// End of ErrMsgBox functions.


// Handle message printing, takes care of the pop-up, log file, and
// stdout printing.
//
void
GTKtoolbar::PopUpSpiceErr(bool to_stdout, const char *string)
{
    if (!string || !*string)
        return;
    if (!CP.Display() || !GRpkg::self()->CurDev())
        to_stdout = true;
    if (Sp.GetFlag(FT_NOERRWIN))
        to_stdout = true;

    if (to_stdout) {
        if (TTY.is_tty() && CP.GetFlag(CP_WAITING)) {
            TTY.err_printf("\n");
            CP.SetFlag(CP_WAITING, false);
        }
        TTY.err_printf("%s", string);
        MB.ToLog(string);
        return;
    }

    MB.PopUpErr(string);
}


//=========================================================================
//
// Message window

namespace {
    void
    ms_cancel_proc(GtkWidget*, void *client_data)
    {
        GtkWidget *popup = (GtkWidget*)client_data;
        gtk_widget_destroy(popup);
    }
}


// Pop up a message at x, y.
//
void
GTKtoolbar::PopUpSpiceMessage(const char *string, int x, int y)
{
    if (!CP.Display())
        return;
    if (!GRpkg::self()->CurDev())
        return;
    if (!string || !*string)
        return;
    GtkWidget *popup = gtk_NewPopup(0, "Message", ms_cancel_proc, 0);

    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_set_border_width(GTK_CONTAINER(form), 2);
    gtk_container_add(GTK_CONTAINER(popup), form);

    //
    // the label, in a frame
    //
    GtkWidget *label = gtk_label_new(string);
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), label);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 4, 2);

    //
    // the dismiss button
    //
    GtkWidget *cancel = gtk_button_new_with_label("Dismiss");
    gtk_widget_show(cancel);
    g_signal_connect(G_OBJECT(cancel), "clicked",
        G_CALLBACK(ms_cancel_proc), popup);
    g_object_set_data(G_OBJECT(cancel), "shell", popup);
    gtk_table_attach(GTK_TABLE(form), cancel, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    GTKdev::self()->SetDoubleClickExit(popup, cancel);

    FixLoc(&x, &y);
    gtk_widget_realize(popup);
    int mwid, mhei;
    gtk_MonitorGeom(0, 0, 0, &mwid, &mhei);
    // make sure the label is fully on-screen
    GtkRequisition req;
    gtk_widget_get_requisition(popup, &req);
    int wd = req.width;
    int ht = req.height;
    if (x + wd > mwid)
        x = mwid - wd;
    if (y + ht > mhei)
        y = mhei - ht;

    // MSW seems to need this before gtk_window_show.
    RevertFocus(popup);

    gtk_window_move(GTK_WINDOW(popup), x, y);
    gtk_widget_show(popup);
#if GTK_CHECK_VERSION(3,0,0)
    if (TB()->context && TB()->context->GetDrawable()) {
#else
    if (TB()->context && TB()->context->Window()) {
#endif
        gtk_window_set_transient_for(GTK_WINDOW(popup),
            GTK_WINDOW(TB()->context->Shell()));
    }
}


//=========================================================================
//
// Resource printing

namespace {
    // Convert elapsed seconds to hours:minutes:seconds.
    //
    void
    hms(double elapsed, int *hours, int *minutes, double *secs)
    {
        *hours = (int)(elapsed/3600);
        elapsed -= *hours*3600;
        *minutes = (int)(elapsed/60);
        elapsed -= *minutes*60;
        *secs = elapsed;
    }
}


// Redraw the resource information.
//
void
GTKtoolbar::UpdateMain(ResUpdType update)
{
    if (!GP.isMainThread())
        return;
    if (update == RES_UPD_TIME) {
        double elapsed, user, cpu;
        ResPrint::get_elapsed(&elapsed, &user, &cpu);
        tb_elapsed_start = elapsed;
    }
#if GTK_CHECK_VERSION(3,0,0)
    else if (context && context->GetDrawable()->get_window()) {
        int wid = context->GetDrawable()->get_width();
        int hei = context->GetDrawable()->get_height();
        context->GetDrawable()->set_draw_to_pixmap();
#else
    else if (context && context->Window()) {
        int wid = gdk_window_get_width(context->Window());
        int hei = gdk_window_get_height(context->Window());
        context->switch_to_pixmap();
#endif
        int fwid, dy;
        context->TextExtent(0, &fwid, &dy);
        context->SetWindowBackground(SpGrPkg::DefColors[0].pixel);
        context->SetBackground(SpGrPkg::DefColors[0].pixel);
        int y = dy + 2;
        int x = 4;
        int ux = 18*fwid;
        int vx = ux + 14*fwid;

        context->SetColor(SpGrPkg::DefColors[0].pixel);
        context->Box(0, 0, wid, hei);

        double elapsed, user, cpu;
        ResPrint::get_elapsed(&elapsed, &user, &cpu);
        elapsed -= tb_elapsed_start;
        int hours, minutes;
        double secs;
        char buf[128];
        if (elapsed < 0.0)
            elapsed = 0.0;
        hms(elapsed, &hours, &minutes, &secs);
        context->SetColor(tb_clr_1);
        context->Text("elapsed", x, y, 0);
        context->SetColor(tb_clr_2);
        *buf = 0;
        if (hours)
            snprintf(buf, sizeof(buf), "%d:", hours);
        if (minutes || hours) {
            int len = strlen(buf);
            snprintf(buf + len, sizeof(buf) - len, "%d:", minutes);
        }
        int len = strlen(buf);
        snprintf(buf + len, sizeof(buf) - len, "%.2f", secs);
        context->Text(buf, ux, y, 0);
        if (Sp.GetFlag(FT_SIMFLAG)) {
            strcpy(buf, "running");
            context->SetColor(tb_clr_3);
        }
        else if (Sp.CurCircuit() && Sp.CurCircuit()->inprogress()) {
            strcpy(buf, "stopped");
            context->SetColor(tb_clr_4);
        }
        else
            strcpy(buf, "idle");
        context->Text(buf, vx, y, 0);
        y += dy;
        if (Sp.GetFlag(FT_SIMFLAG) && Sp.CurCircuit()) {
            double pct = Sp.CurCircuit()->getPctDone();
            if (pct > 0.0) {
                snprintf(buf, sizeof(buf), "%.1f%%", pct);
                context->Text(buf, vx, y, 0);
            }
        }
        if (user >= 0.0) {
            hms(user, &hours, &minutes, &secs);
            context->SetColor(tb_clr_1);
            context->Text("total user", x, y, 0);
            context->SetColor(tb_clr_2);
            *buf = 0;
            if (hours)
                snprintf(buf, sizeof(buf), "%d:", hours);
            if (minutes || hours) {
                len = strlen(buf);
                snprintf(buf + len, sizeof(buf) - len, "%d:", minutes);
            }
            len = strlen(buf);
            snprintf(buf + len, sizeof(buf) - len, "%.2f", secs);
            context->Text(buf, ux, y, 0);
            y += dy;
        }
        if (cpu >= 0.0) {
            hms(cpu, &hours, &minutes, &secs);
            context->SetColor(tb_clr_1);
            context->Text("total system", x, y, 0);
            context->SetColor(tb_clr_2);
            *buf = 0;
            if (hours)
                snprintf(buf, sizeof(buf), "%d:", hours);
            if (minutes || hours) {
                len = strlen(buf);
                snprintf(buf + len, sizeof(buf) - len, "%d:", minutes);
            }
            len = strlen(buf);
            snprintf(buf + len, sizeof(buf) - len, "%.2f", secs);
            context->Text(buf, ux, y, 0);
            y += dy;
        }
        unsigned int data, hlimit, slimit;
        ResPrint::get_space(&data, &hlimit, &slimit);
        if (data) {
            context->SetColor(tb_clr_1);
            context->Text("data size", x, y, 0);
            context->SetColor(tb_clr_2);
            snprintf(buf, sizeof(buf), "%d", data/1024);
            context->Text(buf, ux, y, 0);
            y += dy;

            double val = DEF_maxData;
            VTvalue vv;
            if (Sp.GetVar(spkw_maxdata, VTYP_REAL, &vv, Sp.CurCircuit()))
                val = vv.get_real();
            context->SetColor(tb_clr_1);
            context->Text("program limit", x, y, 0);
            context->SetColor(tb_clr_2);
            snprintf(buf, sizeof(buf), "%d", (int)val);
            context->Text(buf, ux, y, 0);
            y += dy;

            if (hlimit || slimit) {
                if (hlimit == 0 || (slimit > 0 && slimit < hlimit))
                    hlimit = slimit;
                context->SetColor(tb_clr_1);
                context->Text("system limit", x, y, 0);
                context->SetColor(tb_clr_2);
                snprintf(buf, sizeof(buf), "%d", hlimit/1024);
                context->Text(buf, ux, y, 0);
                y += dy;
            }
        }
        context->Update();
#if GTK_CHECK_VERSION(3,0,0)
        context->GetDrawable()->set_draw_to_window();
        context->GetDrawable()->copy_pixmap_to_window(
            context->CpyGC(), 0, 0, -1, -1);
#else
        context->switch_from_pixmap();
#endif
    }
}


void
GTKtoolbar::CloseGraphicsConnection()
{
    if (GTKdev::exists() && GTKdev::self()->ConnectFd() > 0)
        close(GTKdev::ConnectFd());
}


int
GTKtoolbar::RegisterIdleProc(int(*proc)(void*), void *arg)
{
    if (GTKdev::exists())
        return (g_idle_add(proc, arg));
    return (0);
}


bool
GTKtoolbar::RemoveIdleProc(int id)
{
    if (GTKdev::exists())
        g_source_remove(id);
    return (true);
}


int
GTKtoolbar::RegisterTimeoutProc(int ms, int(*proc)(void*), void *arg)
{
    if (GTKdev::exists())
        return (GTKdev::self()->AddTimer(ms, proc, arg));
    return (0);
}


bool
GTKtoolbar::RemoveTimeoutProc(int id)
{
    if (GTKdev::exists())
        GTKdev::self()->RemoveTimer(id);
    return (true);
}


void
GTKtoolbar::RegisterBigForeignWindow(unsigned int w)
{
    if (GTKdev::exists())
        GTKdev::self()->RegisterBigForeignWindow(w);
}


enum RVTtype { RVTauto, RVTnone, RVTrhel6, RVTrhel7, RVTmac, RVTmsw };
namespace {
    RVTtype RevertMode; 
}

// We don't want the toolbar or plot windows to take focus from the
// console when it pops up, but these windows do have
// keyboard sensitivity.  The gtk_window_set_focus_on_map function
// seems like just the ticket, unfortunately it is only a hint to the
// window manager, and is ignored in some cases.  This is a huge
// headache since there a basically no commonality between window
// managers, and the behavior hopefully set here can be extremely
// annoying if not working right.  Basically, we want new windows to
// not take focus when popped up, leaving the focus where it is. 
// However, the windows are capable of focus and can be selected
// explicitly by the user to accept focus.  A new window should appear
// on top, but appear beneath the console if the console is clicked
// in.
//
// There are several different modes here depending on different
// window systems.  By default, one is selected as a best guess, but
// the user can set a variable to override.

namespace {
    int set_accept_focus(void *arg)
    {
        if (RevertMode == RVTmac) {
            // If launched from a Terminal window, the Console will be 0
            // since the terminal is Cocoa, not X (regular xterms work
            // fine).  The following AppleScript will revert focus to the
            // Cocoa terminal.
#ifdef __APPLE__
#ifdef WITH_X11
            if (GTKdev::self()->ConsoleXid() == 0)
#endif
                system(
            "osascript -e \"tell application \\\"Terminal\\\" to activate\"");
#endif
            gtk_window_set_keep_above(GTK_WINDOW(arg), false);
        }
        else if (RevertMode == RVTrhel7)
            gtk_window_set_keep_above(GTK_WINDOW(arg), false);
        else if (RevertMode == RVTmsw) {
            gtk_window_set_keep_above(GTK_WINDOW(arg), false);
            gtk_window_set_accept_focus(GTK_WINDOW(arg), true);
        }
#ifdef WITH_X11
        // This is probably crap.
        if (GTKdev::self()->ConsoleXid() &&
                Sp.GetVar("wmfocusfix", VTYP_BOOL, 0)) {
            XSetInputFocus(gdk_x11_get_default_xdisplay(),
                GTKdev::self()->ConsoleXid(), RevertToPointerRoot,
                CurrentTime);
        }
#endif
        return (false);
    }

    // Expose handler, remove handler and set idle proc
    //
    int revert_proc(GtkWidget *widget, GdkEvent*, void*)
    {
        g_signal_handlers_disconnect_by_func(G_OBJECT(widget),
            (gpointer)revert_proc, widget);
        // Use a timeout rather than an idle, KDE seems to need the delay.
        g_timeout_add(800, set_accept_focus, widget);
        // g_idle_add(set_accept_focus, widget);
        return (0);
    }
}


// Call this on a pop-up shell to cause the focus to revert to the console
// when the pop-up first appears.
//
void
GTKtoolbar::RevertFocus(GtkWidget *widget)
{
    VTvalue vv;
    if (Sp.GetVar(kw_revertmode, VTYP_NUM, &vv))
        RevertMode = (RVTtype)vv.get_int();
    if (RevertMode == RVTauto) {
#ifdef __linux__
        RevertMode = RVTrhel7;
#else
#ifdef __APPLE__
        RevertMode = RVTmac;
#else
#ifdef WIN32
        RevertMode = RVTmsw;
#endif
#endif
#endif
    }

    if (RevertMode == RVTrhel7) {
        // RHEL 7 or newer, uses KDE.  Without the set_keep_above, new
        // plot windows are created below the console, which stinks.

        if (!Sp.GetVar("nototop", VTYP_BOOL, 0)) {
            gtk_window_set_urgency_hint(GTK_WINDOW(widget), true);
            gtk_window_set_keep_above(GTK_WINDOW(widget), true);
        }
        gtk_window_set_focus_on_map(GTK_WINDOW(widget), false);

        // Start with sensitivity off, and use a timer to turn it back
        // on, presumably well after mapping.
        g_signal_connect(G_OBJECT(widget), "expose_event",
            G_CALLBACK(revert_proc), widget);
    }
    else if (RevertMode == RVTrhel6) {
        // RHEL 6 or older, uses Gnome.  The set_keep_above ends up
        // causing the plot windows to disappear beneath other windows
        // after the short timer delay, which is unacceptable.  Below
        // keeps the focus in the console, but the new window may or
        // may not be on top.  This is Gnome behavior and there seems
        // to be no way to fix it.

        gtk_window_set_urgency_hint(GTK_WINDOW(widget), true);
        gtk_window_set_focus_on_map(GTK_WINDOW(widget), false);
    }
    else if (RevertMode == RVTmac) {
        if (!Sp.GetVar("nototop", VTYP_BOOL, 0)) {
            gtk_window_set_urgency_hint(GTK_WINDOW(widget), true);
            gtk_window_set_keep_above(GTK_WINDOW(widget), true);
        }
        gtk_window_set_focus_on_map(GTK_WINDOW(widget), false);
        g_signal_connect(G_OBJECT(widget), "expose_event",
            G_CALLBACK(revert_proc), widget);
    }
    else if (RevertMode == RVTmsw) {
        if (!Sp.GetVar("nototop", VTYP_BOOL, 0)) {
            gtk_window_set_urgency_hint(GTK_WINDOW(widget), true);
            gtk_window_set_keep_above(GTK_WINDOW(widget), true);
        }
        gtk_window_set_focus_on_map(GTK_WINDOW(widget), false);
        gtk_window_set_accept_focus(GTK_WINDOW(widget), false);
        g_signal_connect(G_OBJECT(widget), "expose_event",
            G_CALLBACK(revert_proc), widget);
    }
}


// Return the entry of the named tool, if found.
//
tbent *
GTKtoolbar::FindEnt(const char *str)
{
    if (str) {
        // Handle old "trace" keyword, now called "runops".
        if (!strcmp(str, "trace"))
            str = "runops";
        for (tbent *tb = entries; tb && tb->name; tb++) {
            if (!strcmp(str, tb->name))
                return (tb);
        }
    }
    return (0);
}


// Return the tool shell corresponding to the keyword.
//
GtkWidget *
GTKtoolbar::GetShell(const char *str)
{
    if (str == ntb_bug)
        return (bg_shell);
    else if (str == ntb_circuits)
        return (ci_shell);
    else if (str == ntb_colors)
        return (co_shell);
    else if (str == ntb_commands)
        return (cm_shell);
    else if (str == ntb_debug)
        return (db_shell);
    else if (str == ntb_files)
        return (fi_shell);
    else if (str == ntb_font)
        return (ft_shell);
    else if (str == ntb_plotdefs)
        return (pd_shell);
    else if (str == ntb_plots)
        return (pl_shell);
    else if (str == ntb_shell)
        return (sh_shell);
    else if (str == ntb_simdefs)
        return (sd_shell);
    else if (str == ntb_runop)
        return (tr_shell);
    else if (str == ntb_variables)
        return (va_shell);
    else if (str == ntb_vectors)
        return (ve_shell);

    return (0);
}


// Register that a tool is active, or not.
//
void
GTKtoolbar::SetActive(const char *str, bool state)
{
    for (tbent *tb = entries; tb && tb->name; tb++) {
        if (tb->name == str) {
            tb->active = state;
            return;
        }
    }
}


// Save the location of a tool.
//
void
GTKtoolbar::SetLoc(const char *str, GtkWidget *shell)
{
    if (gtk_widget_is_drawable(shell)) {
        for (tbent *tb = entries; tb && tb->name; tb++) {
            if (tb->name == str) {
                gdk_window_get_root_origin(gtk_widget_get_window(shell),
                    &tb->x, &tb->y);
                int x, y;
                gtk_MonitorGeom(shell, &x, &y);
                tb->x -= x;
                tb->y -= y;
                return;
            }
        }
    }
}


// Modify the location by adding the monitor origin, which was
// subtracted off in SetLoc.  This will allow switching monitors in
// multi-monitor systems.
//
void
GTKtoolbar::FixLoc(int *px, int *py)
{
    int x, y;
    gtk_MonitorGeom(toolbar, &x, &y);
    *px += x;
    *py += y;
}


// Function to return a command line for tbsetup which represents the
// current configuration.
//
char *
GTKtoolbar::ConfigString()
{
    sLstr lstr;
    char buf[512];
    const char *off = "off", *on = "on";
    const char *fmt = "\\\n %s %s %d %d";
    if (!toolbar)
        lstr.add("tbsetup toolbar off");
    else {
        lstr.add("tbsetup");
        if (gtk_widget_get_window(toolbar)) {
            int x, y;
            gdk_window_get_root_origin(gtk_widget_get_window(toolbar), &x, &y);
            snprintf(buf, sizeof(buf), fmt, "toolbar", "on", x, y);
            lstr.add(buf);
        }
        for (tbent *tb = entries; tb && tb->name; tb++) {
            if (!strcmp(tb->name, "toolbar"))
                continue;
            GdkRectangle rect;
            rect.x = tb->x;
            rect.y = tb->y;
            if (tb->active) {
                GtkWidget *wsh = GetShell(tb->name);
                if (wsh)
                    gtk_ShellGeometry(wsh, 0, &rect);
            }
            snprintf(buf, sizeof(buf), fmt, tb->name, tb->active ? on : off,
                rect.x, rect.y);
            lstr.add(buf);
        }
    }
    lstr.add_c('\n');

    // Add the fonts
    const char *fn = Fnt()->getName(FNT_FIXED);
    if (fn) {
        snprintf(buf, sizeof(buf), "setfont 1 %s\n", fn);
        lstr.add(buf);
    }
    fn = Fnt()->getName(FNT_PROP);
    if (fn) {
        snprintf(buf, sizeof(buf), "setfont 2 %s\n", fn);
        lstr.add(buf);
    }
    fn = Fnt()->getName(FNT_SCREEN);
    if (fn) {
        snprintf(buf, sizeof(buf), "setfont 3 %s\n", fn);
        lstr.add(buf);
    }
    fn = Fnt()->getName(FNT_EDITOR);
    if (fn) {
        snprintf(buf, sizeof(buf), "setfont 4 %s\n", fn);
        lstr.add(buf);
    }
    fn = Fnt()->getName(FNT_MOZY);
    if (fn) {
        snprintf(buf, sizeof(buf), "setfont 5 %s\n", fn);
        lstr.add(buf);
    }
    fn = Fnt()->getName(FNT_MOZY_FIXED);
    if (fn) {
        snprintf(buf, sizeof(buf), "setfont 6 %s\n", fn);
        lstr.add(buf);
    }
    return (lstr.string_trim());
}


namespace {
    // Whiteley Research Logo bitmap.
    //    
    // XPM
    const char *tm30[] = {
        // width height ncolors chars_per_pixel
        "30 20 2 1",
        // colors
        "   c none",
        ".  c blue",
        // pixels
        "                              ",
        "                              ",
        "   ..             ......      ",
        "  ...      .      ........    ",
        "  ....     ..    ..........   ",
        "   ...     ..    ...  .....   ",
        "   ....   ....   ...    ...   ",
        "    ....  ....  ....    ...   ",
        "    ....  ..... ...........   ",
        "     .......... ...........   ",
        "     .....................    ",
        "      ...... ...........      ",
        "      .....  .....  ....      ",
        "       ....   ....   ....     ",
        "       ....   ....   .....    ",
        "        ..     ...    .....   ",
        "        ..      .      ....   ",
        "         .      .       ..    ",
        "                              ",
        "                              "};

    const char *run_xpm[] = {
        // width height ncolors chars_per_pixel
        "20 20 3 1",
        // colors
        "   c none",
        ".  c green",
        "x  c darkgreen",
        // pixels
        "                    ",
        "                    ",
        "  x                 ",
        "  x.x               ",
        "  x...x             ",
        "  x.....x           ",
        "  x.......x         ",
        "  x.........x       ",
        "  x...........x     ",
        "  x.............x   ",
        "  x...........x     ",
        "  x.........x       ",
        "  x.......x         ",
        "  x.....x           ",
        "  x...x             ",
        "  x.x               ",
        "  x                 ",
        "                    ",
        "                    ",
        "                    "};

    const char *stop_xpm[] = {
        // width height ncolors chars_per_pixel
        "20 20 2 1",
        // colors
        "   c none",
        ".  c red",
        // pixels
        "                    ",
        "                    ",
        "  ...          ...  ",
        "   ...        ...   ",
        "    ...      ...    ",
        "     ...    ...     ",
        "      ...  ...      ",
        "       ......       ",
        "        ....        ",
        "        ....        ",
        "       ......       ",
        "      ...  ...      ",
        "     ...    ...     ",
        "    ...      ...    ",
        "   ...        ...   ",
        "  ...          ...  ",
        "                    ",
        "                    ",
        "                    ",
        "                    "};
}


namespace {
    //
    // Stuff to register the main window as a drop site
    //

    GtkTargetEntry target_table[] = {
        { (char*)"STRING",     0, 0 },
        { (char*)"text/plain", 0, 1 }
    };
    guint n_targets = sizeof(target_table) / sizeof(target_table[0]);
    bool drag_active;


    // Periodically update the resource listing in the toolbar widget.
    //
    int
    res_timeout(void*)
    {
        TB()->UpdateMain(RES_UPD);
        return (true);
    }


    // Redraw the resource listing
    //
    int
#if GTK_CHECK_VERSION(3,0,0)
    expose_hdlr(GtkWidget*, cairo_t*, void*)
#else
    expose_hdlr(GtkWidget*, GdkEvent*, void*)
#endif
    {
        TB()->UpdateMain(RES_BEGIN);
        return (true);
    }

    // Change the drawing area size on font change.
    //
    void
    font_change_hdlr(GtkWidget*, void*, void*)
    {
        tb_bag *w = TB()->context;
        if (w) {
            int fw, fh;
            w->TextExtent(0, &fw, &fh);
            gtk_widget_set_size_request(w->Viewport(), 40*fw + 6, 6*fh + 6);
        }
    }

    // Handler for the Run/Stop buttons
    void
    rs_btn_hdlr(GtkWidget*, void *arg)
    {
        if (arg == 0)
            // Run button
            CommandTab::com_resume(0);
        else
            raise(SIGINT);
    }
}


// Function to actually create the main window and menus
//
void
GTKtoolbar::tbpop(bool up)
{
    // Unlike previous versions, this is not intended to be
    // reconfigured, thus the directive to pop down is ignored
    //
    if (!up || toolbar)
        return;
    tb_bag *w = new tb_bag(GR_TB);
    GRpkg::self()->NewWbag(GR_TBstr, w);
    context = w;
    toolbar = w->Shell();

    // Export the top level to the help system, so that the
    // transient-for hint will be applied in help windows.  Skipping
    // this may cause subtle things like help windows not being raised
    // when mapped.
#ifdef HAVE_MOZY
    GTKhelpPopup::set_transient_for(GTK_WINDOW(toolbar));
#endif
    GTKeditPopup::set_transient_for(GTK_WINDOW(toolbar));

#ifdef WIN32
    // Icons are obtained from resources.
#else
    GList *g1 = new GList;
    g1->data = gdk_pixbuf_new_from_xpm_data(wrs_48x48_xpm);
    GList *g2 = new GList;
    g2->data = gdk_pixbuf_new_from_xpm_data(wrs_32x32_xpm);
    g1->next = g2; 
    GList *g3 = new GList;
    g3->data = gdk_pixbuf_new_from_xpm_data(wrs_16x16_xpm);
    g3->next = 0;  
    g2->next = g3;
    gtk_window_set_icon_list(GTK_WINDOW(w->Shell()), g1);
#endif

    g_signal_connect(G_OBJECT(toolbar), "destroy",
        G_CALLBACK(quit_proc), 0);
    g_signal_connect(G_OBJECT(toolbar), "delete_event",
        G_CALLBACK(quit_proc), 0);

    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(toolbar), form);

    // Menu bar.
    //
    GtkAccelGroup *accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(w->Shell()), accel_group);
    GtkWidget *menubar = gtk_menu_bar_new();
    g_object_set_data(G_OBJECT(w->Shell()), "menubar", menubar);
    gtk_widget_show(menubar);
    GtkWidget *item;

    // File menu.
    item = gtk_menu_item_new_with_mnemonic("_File");
    gtk_widget_set_name(item, "File");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    tb_file_menu = gtk_menu_new();
    gtk_widget_show(tb_file_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), tb_file_menu);

    // "/File/_File Select", "<control>O", open_proc, 0
    item = gtk_menu_item_new_with_mnemonic("_File Select");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(tb_file_menu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(open_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_o,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_set_tooltip_text(item, "Show File Selection panel");

    // "/File/_Source", "<control>S", source_proc, 0
    item = gtk_menu_item_new_with_mnemonic("_Source");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(tb_file_menu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(source_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_s,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_set_tooltip_text(item, "Source input file");
    tb_source_btn = item;

    // "/File/_Load", "<control>L", load_proc, 0
    item = gtk_menu_item_new_with_mnemonic("_Load");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(tb_file_menu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(load_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_l,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_set_tooltip_text(item, "Load plot data file");
    tb_load_btn = item;

    // "/File/Update _Tools", 0, update_proc, 0
    item = gtk_menu_item_new_with_mnemonic("Update _Tools");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(tb_file_menu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(update_proc), this);
    gtk_widget_set_tooltip_text(item, "Update tool window locations");

    // "/File/Update _WRspice", 0, wrupdate_proc, 0
    item = gtk_menu_item_new_with_mnemonic("Update _WRspice");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(tb_file_menu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(wrupdate_proc), this);
    gtk_widget_set_tooltip_text(item, "Update WRspice");

    item = gtk_separator_menu_item_new();
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(tb_file_menu), item);

    if (CP.GetFlag(CP_NOTTYIO)) {
        // "/File/_Close", "<control>Q", quit_proc, 0
        item = gtk_menu_item_new_with_mnemonic("_Close");
        gtk_widget_set_tooltip_text(item, "Close WRspice");
    }
    else {
        // "/File/_Quit", "<control>Q", quit_proc, 0
        item = gtk_menu_item_new_with_mnemonic("_Quit");
        gtk_widget_set_tooltip_text(item, "Quit WRspice");
    }
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(tb_file_menu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(quit_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_q,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    // Edit menu.
    item = gtk_menu_item_new_with_mnemonic("_Edit");
    gtk_widget_set_name(item, "Edit");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    tb_edit_menu = gtk_menu_new();
    gtk_widget_show(tb_edit_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), tb_edit_menu);

    // "/Edit/_Text Editor", "<control>T", edit_proc, 0
    item = gtk_menu_item_new_with_mnemonic("_Text Editor");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(tb_edit_menu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(edit_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_t,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_set_tooltip_text(item, "Pop up text editor");

    if (!CP.GetFlag(CP_NOTTYIO)) {
        // "/Edit/_Xic", "<control>X", xic_proc, 0
        item = gtk_menu_item_new_with_mnemonic("_Xic");
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(tb_edit_menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(xic_proc), this);
        gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_x,
            GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
        gtk_widget_set_tooltip_text(item, "Start Xic");
    }

    // Tools menu.
    // Note that the tools order is alterable from tbsetup.
    item = gtk_menu_item_new_with_mnemonic("_Tools");
    gtk_widget_set_name(item, "Tools");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    tb_tools_menu = gtk_menu_new();
    gtk_widget_show(tb_tools_menu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), tb_tools_menu);

    unsigned long ix = 0;
    for (tbent *tb = entries; tb && tb->name; tb++, ix++) {
        if (tb->name == ntb_toolbar)
            continue;
        if (tb->name == ntb_bug)
            continue;  // in WR button
        else if (tb->name == ntb_circuits) {
            // "/Tools/_Circuits", "<alt>C", menu_proc, ix, "<CheckItem>"
            item = gtk_check_menu_item_new_with_mnemonic("_Circuits");
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(tb_tools_menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(menu_proc), (gpointer)ix);
            gtk_widget_add_accelerator(item, "activate", accel_group,
                GDK_KEY_c, (GdkModifierType)(GDK_CONTROL_MASK|GDK_SHIFT_MASK),
                GTK_ACCEL_VISIBLE);
            gtk_widget_set_tooltip_text(item, "List circuits");
            tb_circuits = item;
        }
        else if (tb->name == ntb_colors) {
            // "/Tools/C_olors", "<alt>O", menu_proc, ix, "<CheckItem>");
            item = gtk_check_menu_item_new_with_mnemonic("C_olors");
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(tb_tools_menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(menu_proc), (gpointer)ix);
            gtk_widget_add_accelerator(item, "activate", accel_group,
                GDK_KEY_c, (GdkModifierType)(GDK_CONTROL_MASK|GDK_SHIFT_MASK),
                GTK_ACCEL_VISIBLE);
            gtk_widget_set_tooltip_text(item, "Set plot colors");
            tb_colors = item;
        }
        else if (tb->name == ntb_commands) {
            // "/Tools/Co_mmands", "<alt>M", menu_proc, ix, "<CheckItem>"
            item = gtk_check_menu_item_new_with_mnemonic("Co_mmands");
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(tb_tools_menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(menu_proc), (gpointer)ix);
            gtk_widget_add_accelerator(item, "activate", accel_group,
                GDK_KEY_m, (GdkModifierType)(GDK_CONTROL_MASK|GDK_SHIFT_MASK),
                GTK_ACCEL_VISIBLE);
            gtk_widget_set_tooltip_text(item, "Set command options");
            tb_commands = item;
        }
        else if (tb->name == ntb_debug) {
            // "/Tools/_Debug", "<alt>D", menu_proc, ix, "<CheckItem>");
            item = gtk_check_menu_item_new_with_mnemonic("_Debug");
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(tb_tools_menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(menu_proc), (gpointer)ix);
            gtk_widget_add_accelerator(item, "activate", accel_group,
                GDK_KEY_d, (GdkModifierType)(GDK_CONTROL_MASK|GDK_SHIFT_MASK),
                GTK_ACCEL_VISIBLE);
            gtk_widget_set_tooltip_text(item, "Set debugging options");
            tb_debug = item;
        }
        else if (tb->name == ntb_files) {
            // "/Tools/_Files", "<alt>Z", menu_proc, ix, "<CheckItem>"
            item = gtk_check_menu_item_new_with_mnemonic("_Files");
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(tb_tools_menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(menu_proc), (gpointer)ix);
            gtk_widget_add_accelerator(item, "activate", accel_group,
                GDK_KEY_z, (GdkModifierType)(GDK_CONTROL_MASK|GDK_SHIFT_MASK),
                GTK_ACCEL_VISIBLE);
            gtk_widget_set_tooltip_text(item, "List search path files");
            tb_files = item;
        }
        else if (tb->name == ntb_font) {
            // "/Tools/Fo_nts", "<alt>N", menu_proc, ix, "<CheckItem>");
            item = gtk_check_menu_item_new_with_mnemonic("Fo_nts");
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(tb_tools_menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(menu_proc), (gpointer)ix);
            gtk_widget_add_accelerator(item, "activate", accel_group,
                GDK_KEY_n, (GdkModifierType)(GDK_CONTROL_MASK|GDK_SHIFT_MASK),
                GTK_ACCEL_VISIBLE);
            gtk_widget_set_tooltip_text(item, "Set window fonts");
            tb_font = item;
        }
        else if (tb->name == ntb_plotdefs) {
            // "/Tools/P_lot Opts", "<alt>L", menu_proc, ix, "<CheckItem>"
            item = gtk_check_menu_item_new_with_mnemonic("P_lot Opts");
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(tb_tools_menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(menu_proc), (gpointer)ix);
            gtk_widget_add_accelerator(item, "activate", accel_group,
                GDK_KEY_l, (GdkModifierType)(GDK_CONTROL_MASK|GDK_SHIFT_MASK),
                GTK_ACCEL_VISIBLE);
            gtk_widget_set_tooltip_text(item, "Set plot options");
            tb_plotdefs = item;
        }
        else if (tb->name == ntb_plots) {
            // "/Tools/_Plots", "<alt>P", menu_proc, ix, "<CheckItem>");
            item = gtk_check_menu_item_new_with_mnemonic("_Plots");
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(tb_tools_menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(menu_proc), (gpointer)ix);
            gtk_widget_add_accelerator(item, "activate", accel_group,
                GDK_KEY_p, (GdkModifierType)(GDK_CONTROL_MASK|GDK_SHIFT_MASK),
                GTK_ACCEL_VISIBLE);
            gtk_widget_set_tooltip_text(item, "List result plot data");
            tb_plots = item;
        }
        else if (tb->name == ntb_shell) {
            // "/Tools/_Shell", "<alt>S", GIFC(menu_proc), ix, "<CheckItem>");
            item = gtk_check_menu_item_new_with_mnemonic("_Shell");
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(tb_tools_menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(menu_proc), (gpointer)ix);
            gtk_widget_add_accelerator(item, "activate", accel_group,
                GDK_KEY_s, (GdkModifierType)(GDK_CONTROL_MASK|GDK_SHIFT_MASK),
                GTK_ACCEL_VISIBLE);
            gtk_widget_set_tooltip_text(item, "Set shell options");
            tb_shell = item;
        }
        else if (tb->name == ntb_simdefs) {
            // "/Tools/S_im Opts", "<alt>I", menu_proc, ix, "<CheckItem>"
            item = gtk_check_menu_item_new_with_mnemonic("S_im Opts");
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(tb_tools_menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(menu_proc), (gpointer)ix);
            gtk_widget_add_accelerator(item, "activate", accel_group,
                GDK_KEY_i, (GdkModifierType)(GDK_CONTROL_MASK|GDK_SHIFT_MASK),
                GTK_ACCEL_VISIBLE);
            gtk_widget_set_tooltip_text(item, "Set simulation options");
            tb_simdefs = item;
        }
        else if (tb->name == ntb_runop) {
            // "/Tools/_Runops", "<alt>A", menu_proc, ix, "<CheckItem>");
            item = gtk_check_menu_item_new_with_mnemonic("_Runops");
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(tb_tools_menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(menu_proc), (gpointer)ix);
            gtk_widget_add_accelerator(item, "activate", accel_group,
                GDK_KEY_a, (GdkModifierType)(GDK_CONTROL_MASK|GDK_SHIFT_MASK),
                GTK_ACCEL_VISIBLE);
            gtk_widget_set_tooltip_text(item, "List runops in effect");
            tb_runop = item;
        }
        else if (tb->name == ntb_variables) {
            // "/Tools/Va_riables", "<alt>R", menu_proc, ix, "<CheckItem>");
            item = gtk_check_menu_item_new_with_mnemonic("Va_riables");
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(tb_tools_menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(menu_proc), (gpointer)ix);
            gtk_widget_add_accelerator(item, "activate", accel_group,
                GDK_KEY_r, (GdkModifierType)(GDK_CONTROL_MASK|GDK_SHIFT_MASK),
                GTK_ACCEL_VISIBLE);
            gtk_widget_set_tooltip_text(item, "List set shell variables");
            tb_variables = item;
        }
        else if (tb->name == ntb_vectors) {
            // "/Tools/_Vectors", "<alt>V", GIFC(menu_proc), ix, "<CheckItem>"
            item = gtk_check_menu_item_new_with_mnemonic("_Vectors");
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(tb_tools_menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(menu_proc), (gpointer)ix);
            gtk_widget_add_accelerator(item, "activate", accel_group,
                GDK_KEY_v, (GdkModifierType)(GDK_CONTROL_MASK|GDK_SHIFT_MASK),
                GTK_ACCEL_VISIBLE);
            gtk_widget_set_tooltip_text(item, "List vectors in current plot");
            tb_vectors = item;
        }
    }

    // Help menu.
    item = gtk_menu_item_new_with_mnemonic("_Help");
    gtk_widget_set_name(item, "Help");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    GtkWidget *helpMenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), helpMenu);

    // "/Help/_Help", "<control>H", help_proc, 0
    item = gtk_menu_item_new_with_mnemonic("_Help");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(help_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_h,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_set_tooltip_text(item, "Pop up Help window");

    // "/Help/_About", "<control>A", about_proc, 0
    item = gtk_menu_item_new_with_mnemonic("_About");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(about_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_a,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_set_tooltip_text(item, "Pop up About window");

    // "/Help/_Notes", "<control>N", GIFC(notes_proc), 0
    item = gtk_menu_item_new_with_mnemonic("_Notes");
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(notes_proc), this);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_n,
        GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_set_tooltip_text(item, "Show release notes");

    for (tbent *tb = entries; tb && tb->name; tb++) {
        if (tb->name == ntb_toolbar)
            continue;
        if (tb->name == ntb_bug)
            continue;  // in WR button
        else if (tb->name == ntb_circuits) {
            if (tb_circuits)
                GTKdev::SetStatus(tb_circuits, tb->active);
        }
        else if (tb->name == ntb_colors) {
            if (tb_colors)
                GTKdev::SetStatus(tb_colors, tb->active);
        }
        else if (tb->name == ntb_commands) {
            if (tb_commands)
                GTKdev::SetStatus(tb_commands, tb->active);
        }
        else if (tb->name == ntb_debug) {
            if (tb_debug)
                GTKdev::SetStatus(tb_debug, tb->active);
        }
        else if (tb->name == ntb_files) {
            if (tb_files)
                GTKdev::SetStatus(tb_files, tb->active);
        }
        else if (tb->name == ntb_font) {
            if (tb_font)
                GTKdev::SetStatus(tb_font, tb->active);
        }
        else if (tb->name == ntb_plotdefs) {
            if (tb_plotdefs)
                GTKdev::SetStatus(tb_plotdefs, tb->active);
        }
        else if (tb->name == ntb_plots) {
            if (tb_plots)
                GTKdev::SetStatus(tb_plots, tb->active);
        }
        else if (tb->name == ntb_shell) {
            if (tb_shell)
                GTKdev::SetStatus(tb_shell, tb->active);
        }
        else if (tb->name == ntb_simdefs) {
            if (tb_simdefs)
                GTKdev::SetStatus(tb_simdefs, tb->active);
        }
        else if (tb->name == ntb_runop) {
            if (tb_runop)
                GTKdev::SetStatus(tb_runop, tb->active);
        }
        else if (tb->name == ntb_variables) {
            if (tb_variables)
                GTKdev::SetStatus(tb_variables, tb->active);
        }
        else if (tb->name == ntb_vectors) {
            if (tb_vectors)
                GTKdev::SetStatus(tb_vectors, tb->active);
        }
    }

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    // the WR logo button
    GtkWidget *pixbtn = gtk_NewPixmapButton(tm30, "WR", false);
    gtk_widget_show(pixbtn);
    tb_bug = pixbtn;
    g_signal_connect(G_OBJECT(pixbtn), "clicked",
        G_CALLBACK(wr_btn_hdlr), 0);
    gtk_box_pack_start(GTK_BOX(hbox), pixbtn, false, false, 0);
    gtk_widget_set_tooltip_text(pixbtn, "Pop up email client");

    // the Run button
    pixbtn = gtk_NewPixmapButton(run_xpm, "Run", false);
    gtk_widget_show(pixbtn);
    g_signal_connect(G_OBJECT(pixbtn), "clicked",
        G_CALLBACK(rs_btn_hdlr), 0);
    gtk_box_pack_start(GTK_BOX(hbox), pixbtn, false, false, 0);
    gtk_widget_set_tooltip_text(pixbtn, "Run current circuit");

    // the Stop button
    pixbtn = gtk_NewPixmapButton(stop_xpm, "Stop", false);
    gtk_widget_show(pixbtn);
    g_signal_connect(G_OBJECT(pixbtn), "clicked",
        G_CALLBACK(rs_btn_hdlr), (void*)1);
    gtk_box_pack_start(GTK_BOX(hbox), pixbtn, false, false, 0);
    gtk_widget_set_tooltip_text(pixbtn, "Pause current analysis");

    gtk_box_pack_start(GTK_BOX(hbox), menubar, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    w->SetViewport(gtk_drawing_area_new());
    gtk_widget_show(w->Viewport());

    GTKfont::setupFont(w->Viewport(), FNT_SCREEN, true);

    int wid, hei;
    w->TextExtent(0, &wid, &hei);
    gtk_widget_set_size_request(w->Viewport(), 40*wid + 6, 6*hei + 6);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), w->Viewport());

    // drop handler setup
    gtk_drag_dest_set(frame,
        (GtkDestDefaults)(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP),
        target_table, n_targets, GDK_ACTION_COPY);
    g_signal_connect(G_OBJECT(frame), "drag_data_received",
        G_CALLBACK(drag_data_received), 0);
    g_signal_connect(G_OBJECT(frame), "drag_leave",
        G_CALLBACK(target_drag_leave), 0);
    g_signal_connect(G_OBJECT(frame), "drag-motion",
        G_CALLBACK(target_drag_motion), 0);

#if GTK_CHECK_VERSION(3,0,0)
    g_signal_connect(G_OBJECT(w->Viewport()), "draw",
        G_CALLBACK(expose_hdlr), w);
#else
    g_signal_connect(G_OBJECT(w->Viewport()), "expose_event",
        G_CALLBACK(expose_hdlr), w);
#endif
    g_signal_connect(G_OBJECT(w->Viewport()), "style_set",
        G_CALLBACK(font_change_hdlr), 0);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 2, 2);

    for (tbent *tb = entries; tb && tb->name; tb++) {
        if (tb->name == ntb_toolbar) {
            int x = tb->x;
            int y = tb->y;
            FixLoc(&x, &y);
            gtk_window_move(GTK_WINDOW(toolbar), x, y);
            continue;
        }
    }

    // MSW seems to need this before gtk_window_show.
    RevertFocus(toolbar);

    gtk_widget_show(toolbar);
#if GTK_CHECK_VERSION(3,0,0)
    w->GetDrawable()->set_window(gtk_widget_get_window(w->Viewport()));
#else
    w->SetWindow(w->Viewport()->window);
#endif
    char tbuf[28];
    snprintf(tbuf, sizeof(tbuf), "WRspice-%s", Global.Version());
    w->Title(tbuf, "WRspice");

    // set up initial xor color
    GdkColor clr;
    clr.pixel = SpGrPkg::DefColors[0].pixel ^ SpGrPkg::DefColors[1].pixel;
#if GTK_CHECK_VERSION(3,0,0)
    w->XorGC()->set_foreground(&clr);
#else
    gdk_gc_set_foreground(w->XorGC(), &clr);
#endif

    // drawing colors
    const char *s = XRMgetFromDb("fgcolor1");
    if (!s)
        s = "sienna";
    tb_clr_1 = GTKdev::self()->NameColor(s);
    s = XRMgetFromDb("fgcolor2");
    if (!s)
        s = "black";
    tb_clr_2 = GTKdev::self()->NameColor(s);
    s = XRMgetFromDb("fgcolor3");
    if (!s)
        s = "red";
    tb_clr_3 = GTKdev::self()->NameColor(s);
    s = XRMgetFromDb("fgcolor4");
    if (!s)
        s = "blue";
    tb_clr_4 = GTKdev::self()->NameColor(s);

    w->SetWindowBackground(SpGrPkg::DefColors[0].pixel);
    w->SetBackground(SpGrPkg::DefColors[0].pixel);
    w->Clear();
    g_timeout_add(2000, res_timeout, 0);
}


// Static Function
// Receive a file name in the main window.  If the Source or Load popup
// is active, put the file name in the popup.  Otherwise, pop up the
// Source popup loaded with the name.
//
void
GTKtoolbar::drag_data_received(GtkWidget*, GdkDragContext *context, gint, gint,
    GtkSelectionData *data, guint, guint time)
{
    if (gtk_selection_data_get_length(data) >= 0 &&
            gtk_selection_data_get_format(data) == 8) {
        char *src = (char*)gtk_selection_data_get_data(data);
        delete [] TB()->tb_dropfile;
        TB()->tb_dropfile = 0;
        if (TB()->context->ActiveInput()) {
            GtkWidget *entry = (GtkWidget*)g_object_get_data
                (G_OBJECT(TB()->context->ActiveInput()), "text");
            if (entry) {
                gtk_entry_set_text(GTK_ENTRY(entry), src);
                gtk_drag_finish(context, true, false, time);
                return;
            }
            // shouldn't get here
            if (TB()->context->ActiveInput())
                TB()->context->ActiveInput()->popdown();
        }
        GtkWidget *item = TB()->tb_source_btn;
        if (!item)
            item = TB()->tb_load_btn;
        if (item) {
            TB()->tb_dropfile = lstring::copy(src);
            gtk_menu_item_activate(GTK_MENU_ITEM(item));
            gtk_drag_finish(context, true, false, time);
            return;
        }
    }
    gtk_drag_finish(context, false, false, time);
}


// Static Function
gboolean
GTKtoolbar::target_drag_motion(GtkWidget *widget, GdkDragContext*, gint, gint,
    guint)
{
    if (!drag_active) {
        drag_active = true;
        gtk_drag_highlight(widget);
    }
    return (true);
}


// Static Function
void  
GTKtoolbar::target_drag_leave(GtkWidget *widget, GdkDragContext*, guint)
{
    if (drag_active) {
        drag_active = false;
        gtk_drag_unhighlight(widget);
    }
}


// Static Function
// The WR button callback, linked to the bug report popup.
//
void
GTKtoolbar::wr_btn_hdlr(GtkWidget*, void*)
{
    for (tbent *tb = TB()->entries; tb && tb->name; tb++) {
        if (tb->name == TB()->ntb_bug) {
            if (!tb->active)
                TB()->PopUpBugRpt(MODE_ON, tb->x, tb->y);
            else
                TB()->PopUpBugRpt(MODE_OFF, 0, 0);
            break;
        }
    }
}


namespace {
    // OK button callback for the file selection.
    //
    void
    file_sel_cb(const char *fname, void*)
    {
        wordlist wl;
        wl.wl_word = (char*)fname;

        // if the file is a rawfile, load it
        FILE *fp = fopen(fname, "r");
        if (fp) {
            char buf[128];
            fgets(buf, 128, fp);
            if (lstring::prefix("Title:", buf)) {
                fgets(buf, 128, fp);
                if (lstring::prefix("Date:", buf)) {
                    fclose(fp);
                    TTY.monitor();
                    CommandTab::com_load(&wl);
                    if (TTY.wasoutput())
                        CP.Prompt();
                    return;
                }
            }
            fclose(fp);
        }
        TTY.monitor();
        CommandTab::com_source(&wl);
        if (TTY.wasoutput())
            CP.Prompt();
    }
}


// Static Function
// Pop up the file selection panel.
//
void
GTKtoolbar::open_proc(GtkWidget*, void*)
{
    static int cnt;
    int x = 100, y = 100;
    x += cnt*200;
    y += cnt*100;
    TB()->FixLoc(&x, &y);
    cnt++;
    if (cnt == 3)
        cnt = 0;
    TB()->context->PopUpFileSelector(fsSEL, GRloc(LW_XYA, x, y), file_sel_cb,
        0, 0, 0);
}


namespace {
    // Source circuit, passed to pop-up.
    //
    void source_cb(const char *fname, void*)
    {
        if (fname && *fname) {
            wordlist wl;
            wl.wl_word = (char*)fname;
            TTY.monitor();
            CommandTab::com_source(&wl);
            if (TTY.wasoutput())
                CP.Prompt();
            if (TB()->context->ActiveInput())
                TB()->context->ActiveInput()->popdown();
        }
    }
}
       

// Static Function
// Prompt for circuit to source.
//
void
GTKtoolbar::source_proc(GtkWidget*, void*)
{
    char buf[512];
    *buf = '\0';
    if (TB()->tb_dropfile) {
        strcpy(buf, TB()->tb_dropfile);
        delete [] TB()->tb_dropfile;
        TB()->tb_dropfile = 0;
    }
    TB()->context->PopUpInput("Name of circuit file to source?", buf,
        "Source", source_cb, 0);
}


namespace {
    // Load rawfile, passed to pop-up.
    //
    void load_cb(const char *fname, void*)
    {
        if (fname && *fname) {
            wordlist wl;
            wl.wl_word = (char*)fname;
            TTY.monitor();
            CommandTab::com_load(&wl);
            if (TTY.wasoutput())
                CP.Prompt();
            if (TB()->context->ActiveInput())
                TB()->context->ActiveInput()->popdown();
        }
    }
}
       

// Static Function
// Prompt for rawfile to load.
//
void
GTKtoolbar::load_proc(GtkWidget*, void*)
{
    char buf[512];
    *buf = '\0';
    if (TB()->tb_dropfile) {
        strcpy(buf, TB()->tb_dropfile);
        delete [] TB()->tb_dropfile;
        TB()->tb_dropfile = 0;
    }
    TB()->context->PopUpInput("Name of rawfile to load?", buf, "Load",
        load_cb, 0);
}


// Static Function
// Update the tool configuration.
//
void
GTKtoolbar::update_proc(GtkWidget*, void*)
{
    CommandTab::com_tbupdate(0);
}


// Static Function
// Check for updates, download/install update.
//
void
GTKtoolbar::wrupdate_proc(GtkWidget*, void*)
{
    CommandTab::com_wrupdate(0);
    raise(SIGINT);  // for new prompt, else it hangs
}


// Static Function
// Quit the program, confirm if work is unsaved
//
int
GTKtoolbar::quit_proc(GtkWidget*, void*)
{
    if (CP.GetFlag(CP_NOTTYIO)) {
        // In server mode, just hide ourself.
        gtk_widget_hide(TB()->toolbar);
#if GTK_CHECK_VERSION(3,0,0)
        TB()->context->GetDrawable()->set_window(0);
#else
        TB()->context->SetWindow(0);
#endif
    }
    else {
        CommandTab::com_quit(0);
        raise(SIGINT);  // for new prompt, else it hangs
        return (1);
    }
    return (0);
}


// Static Function
// Pop up a text editor.
//
void
GTKtoolbar::edit_proc(GtkWidget*, void*)
{
    CommandTab::com_edit(0);
}


// Static Function
// Start the Xic program.
//
void
GTKtoolbar::xic_proc(GtkWidget*, void*)
{
    bool usearg = false;
    wordlist wl;
    if (Sp.CurCircuit() && Sp.CurCircuit()->filename()) {
        FILE *fp = fopen(Sp.CurCircuit()->filename(), "r");
        if (fp) {
            char *s, buf[132];
            while ((s = fgets(buf, 128, fp)) != 0) {
                while (isspace(*s))
                    s++;
                if (!*s)
                    continue;
                if (lstring::prefix(GFX_PREFIX, s)) {
                    wl.wl_next = wl.wl_prev = 0;
                    wl.wl_word = (char*)Sp.CurCircuit()->filename();
                    usearg = true;
                }
                break;
            }
            fclose(fp);
        }
    }
    CommandTab::com_sced(usearg ? &wl : 0);
}


// Static Function
// Handler for the "tools" menu.
//
void
GTKtoolbar::menu_proc(GtkWidget*, void *data)
{
    unsigned int indx = (uintptr_t)data;
    tbent *tb = &TB()->entries[indx];
    if (tb->name == TB()->ntb_bug) {
        if (!tb->active)
            TB()->PopUpBugRpt(MODE_ON, tb->x, tb->y);
        else
            TB()->PopUpBugRpt(MODE_OFF, 0, 0);
    }
    else if (tb->name == TB()->ntb_circuits) {
        if (!tb->active)
            TB()->PopUpCircuits(MODE_ON, tb->x, tb->y);
        else
            TB()->PopUpCircuits(MODE_OFF, 0, 0);
    }
    else if (tb->name == TB()->ntb_colors) {
        if (!tb->active)
            TB()->PopUpColors(MODE_ON, tb->x, tb->y);
        else
            TB()->PopUpColors(MODE_OFF, 0, 0);
    }
    else if (tb->name == TB()->ntb_commands) {
        if (!tb->active)
            TB()->PopUpCmdConfig(MODE_ON, tb->x, tb->y);
        else
            TB()->PopUpCmdConfig(MODE_OFF, 0, 0);
    }
    else if (tb->name == TB()->ntb_debug) {
        if (!tb->active)
            TB()->PopUpDebugDefs(MODE_ON, tb->x, tb->y);
        else
            TB()->PopUpDebugDefs(MODE_OFF, 0, 0);
    }
    else if (tb->name == TB()->ntb_files) {
        if (!tb->active)
            TB()->PopUpFiles(MODE_ON, tb->x, tb->y);
        else
            TB()->PopUpFiles(MODE_OFF, 0, 0);
    }
    else if (tb->name == TB()->ntb_font) {
        if (!tb->active)
            TB()->PopUpFont(MODE_ON, tb->x, tb->y);
        else
            TB()->PopUpFont(MODE_OFF, 0, 0);
    }
    else if (tb->name == TB()->ntb_plotdefs) {
        if (!tb->active)
            TB()->PopUpPlotDefs(MODE_ON, tb->x, tb->y);
        else
            TB()->PopUpPlotDefs(MODE_OFF, 0, 0);
    }
    else if (tb->name == TB()->ntb_plots) {
        if (!tb->active)
            TB()->PopUpPlots(MODE_ON, tb->x, tb->y);
        else
            TB()->PopUpPlots(MODE_OFF, 0, 0);
    }
    else if (tb->name == TB()->ntb_shell) {
        if (!tb->active)
            TB()->PopUpShellDefs(MODE_ON, tb->x, tb->y);
        else
            TB()->PopUpShellDefs(MODE_OFF, 0, 0);
    }
    else if (tb->name == TB()->ntb_simdefs) {
        if (!tb->active)
            TB()->PopUpSimDefs(MODE_ON, tb->x, tb->y);
        else
            TB()->PopUpSimDefs(MODE_OFF, 0, 0);
    }
    else if (tb->name == TB()->ntb_runop) {
        if (!tb->active)
            TB()->PopUpRunops(MODE_ON, tb->x, tb->y);
        else
            TB()->PopUpRunops(MODE_OFF, 0, 0);
    }
    else if (tb->name == TB()->ntb_variables) {
        if (!tb->active)
            TB()->PopUpVariables(MODE_ON, tb->x, tb->y);
        else
            TB()->PopUpVariables(MODE_OFF, 0, 0);
    }
    else if (tb->name == TB()->ntb_vectors) {
        if (!tb->active)
            TB()->PopUpVectors(MODE_ON, tb->x, tb->y);
        else
            TB()->PopUpVectors(MODE_OFF, 0, 0);
    }
}


// Static Function
// Pop up the help browser.
//
void
GTKtoolbar::help_proc(GtkWidget*, void*)
{
    CommandTab::com_help(0);
}


// Static Function
// Pop up a file containing the "about" information
//
void
GTKtoolbar::about_proc(GtkWidget*, void*)
{
    char buf[256];
    if (Global.StartupDir() && *Global.StartupDir()) {
        snprintf(buf, sizeof(buf), "%s/%s", Global.StartupDir(),
            "wrspice_mesg");
        FILE *fp = fopen(buf, "r");
        if (fp) {
            bool didsub = false;
            sLstr lstr;
            while (fgets(buf, 256, fp) != 0) {
                if (!didsub) {
                    char *s = strchr(buf, '$');
                    if (s) { 
                        didsub = true;
                        *s = '\0';
                        lstr.add(buf);
                        lstr.add(Global.Version());
                        lstr.add(s+1);
                        continue;
                    }
                }
                lstr.add(buf);
            }
            fclose(fp);
            GdkRectangle rect;
            gtk_ShellGeometry(TB()->toolbar, 0, &rect);
            int mwid, mhei;
            gtk_MonitorGeom(0, 0, 0, &mwid, &mhei);
            LWenum code;
            if (mhei - (rect.y + rect.height) < rect.y) {
                if (mwid - (rect.x + rect.width) < rect.x)
                    code = LW_LR;
                else
                    code = LW_LL;
            }
            else {
                if (mwid - (rect.x + rect.width) < rect.x)
                    code = LW_UR;
                else
                    code = LW_UL;
            }
                
            TB()->context->PopUpHTMLinfo(MODE_ON, lstr.string(), GRloc(code));
            return;
        }
    }
    fprintf(stderr, "Warning: can't open wrspice_mesg file.\n");
}


// Static Function
// Pop up a file browser loaded with the release notes.
//
void
GTKtoolbar::notes_proc(GtkWidget*, void*)
{
    if (!TB()->toolbar)
        return;
    const char *docspath = 0;
    VTvalue vv;
    if (Sp.GetVar("docsdir", VTYP_STRING, &vv))
        docspath = vv.get_string();
    else {
        docspath = getenv("SPICE_DOCS_DIR");
        if (!docspath)
            docspath = Global.DocsDir();
    }
    if (!docspath || !*docspath) {
        TB()->context->PopUpMessage("No path to release notes (set docsdir).",
            true);
        return;
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "%s/wrs%s", docspath, Global.Version());

    // Remove last component of version, file name is like "wrs3.0".
    char *t = strrchr(buf, '.');
    if (t)
        *t = 0;

    check_t ret = filestat::check_file(buf, R_OK);
    if (ret == NOGO) {
        TB()->context->PopUpMessage(filestat::error_msg(), true);
        return;
    }
    if (ret == NO_EXIST) {
        int len = strlen(buf) + 64;
        char *tt = new char[len];
        snprintf(tt, len, "Can't find file %s.", buf);
        TB()->context->PopUpMessage(tt, true);
        delete [] tt;
        return;
    }
    TB()->context->PopUpFileBrowser(buf);
}
// End of GTKtoolbar functions.


#if GTK_CHECK_VERSION(3,0,0)
#else

void
tb_bag::switch_to_pixmap()
{
    int w = gdk_window_get_width(gd_window);
    int h = gdk_window_get_height(gd_window);
    if (!b_pixmap || w != b_wid || h != b_hei) {
        GdkPixmap *pm = b_pixmap;
        if (pm)
            gdk_pixmap_unref(pm);
        b_wid = w;
        b_hei = h;
        pm = gdk_pixmap_new(gd_window, w, h,
            gdk_visual_get_depth(GTKdev::self()->Visual()));
        if (pm)
            b_pixmap = pm;
        else {
            b_pixmap = 0;
            b_wid = 0;
            b_hei = 0;
        }
    }
    b_winbak = gd_window;
    gd_window = b_pixmap;
    gd_viewport->window = gd_window;
}

// Copy the pixmap to the window, and swap it out.
//
void
tb_bag::switch_from_pixmap()
{
    if (b_winbak) {
        gdk_window_copy_area(b_winbak, CpyGC(), 0, 0, gd_window,
            0, 0, b_wid, b_hei);
        gd_window = b_winbak;
        b_winbak = 0;
        gtk_widget_set_window(gd_viewport, gd_window);
    }
}
// End of tb_bag functions.
#endif



//==========================================================================
//
// The keyword entry composite functions

namespace {
    // Handler for the "set" button:  set the value, freeze the entry
    // area.
    //
    void action_proc(GtkWidget*, void *client_data)
    {
        xKWent *entry = static_cast<xKWent*>(client_data);
        entry->xent()->handler(entry);
    }


    // Handler for the "def" button: reset to the default value.
    //
    void def_proc(GtkWidget*, void *client_data)
    {
        xEnt *ent = (xEnt*)client_data;
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ent->active)))
            gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(ent->active));
        if (ent->defstr)
            gtk_entry_set_text(GTK_ENTRY(ent->entry), ent->defstr);
        else if (GTK_IS_SPIN_BUTTON(ent->entry))
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(ent->entry), ent->val);
        if (ent->entry2)
            gtk_entry_set_text(GTK_ENTRY(ent->entry2),
                ent->defstr ? ent->defstr : "");
    }


    // Set the "Def" button sensitive when the text is different from
    // the default text.
    //
    void value_changed(GtkWidget*, void *client_data)
    {
        xKWent *kwent = (xKWent*)client_data;
        xEnt *ent = kwent->xent();
        if (ent->defstr && ent->active &&
                !GTKdev::GetStatus(ent->active)) {
            const char *str = gtk_entry_get_text(GTK_ENTRY(ent->entry));
            const char *str2 = 0;
            if (ent->entry2)
                str2 = gtk_entry_get_text(GTK_ENTRY(ent->entry2));
            bool isdef = true;
            if (strcmp(str, ent->defstr))
                isdef = false;
            else if (str2 && strcmp(str2, ent->defstr))
                isdef = false;
            if (isdef)
                gtk_widget_set_sensitive(ent->deflt, false);
            else
                gtk_widget_set_sensitive(ent->deflt, true);
        }
    }
}


// Function to create the keyword entry composite
// char *defstring           Default test (Def button sets this)
// int(*cb)()                Callback function for item list.
//
void
xEnt::create_widgets(xKWent *kwstruct, const char *defstring,
    int(*cb)(GtkWidget*, GdkEvent*, void*))
{
    variable *v = Sp.GetRawVar(kwstruct->word);
    if (!defstring)
        defstring = "";
    defstr = lstring::copy(defstring);

    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);

    active = gtk_check_button_new_with_label("Set");
    gtk_widget_show(active);
    gtk_box_pack_start(GTK_BOX(hbox), active, false, false, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active), v ? true : false);

    if (kwstruct->type != VTYP_BOOL &&
            !(kwstruct->type == VTYP_LIST && mode == KW_NO_CB)) {
        // second term is for "debug" button in debug panel
        deflt = gtk_button_new_with_label("Def");
        gtk_widget_show(deflt);
        g_signal_connect(G_OBJECT(deflt), "clicked",
            G_CALLBACK(def_proc), this);
        gtk_misc_set_padding(GTK_MISC(gtk_bin_get_child(GTK_BIN(deflt))), 4, 0);
        gtk_box_pack_start(GTK_BOX(hbox), deflt, false, false, 2);
    }

    if ((mode != KW_FLOAT && mode != KW_NO_SPIN) &&
            (kwstruct->type == VTYP_NUM || kwstruct->type == VTYP_REAL ||
            (kwstruct->type == VTYP_STRING &&
            (mode == KW_INT_2 || mode == KW_REAL_2)))) {
        if (mode == KW_REAL_2) {
            // no spin - may want to add options with and without spin
            entry = gtk_entry_new();
            gtk_widget_show(entry);
            gtk_widget_set_size_request(entry, 80, -1);
            gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 2);
            entry2 = gtk_entry_new();
            gtk_widget_show(entry2);
            gtk_widget_set_size_request(entry2, 80, -1);
            gtk_box_pack_start(GTK_BOX(hbox), entry2, true, true, 2);
        }
        else {
            GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new(val,
                kwstruct->min, kwstruct->max, del, pgsize, 0);
            entry = gtk_spin_button_new(adj, rate, numd);
            gtk_widget_show(entry);
            gtk_widget_set_size_request(entry, 80, -1);
            gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(entry), true);
            gtk_box_pack_start(GTK_BOX(hbox), entry, false, false, 2);
            if (mode == KW_INT_2) {
                adj = (GtkAdjustment*)gtk_adjustment_new(val, kwstruct->min,
                    kwstruct->max, del, pgsize, 0);
                entry2 = gtk_spin_button_new(adj, rate, numd);
                gtk_widget_show(entry2);
                gtk_widget_set_size_request(entry2, 80, -1);
                gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(entry2), true);
                gtk_box_pack_start(GTK_BOX(hbox), entry2, false, false, 2);
            }
        }
    }
    else if (mode == KW_FLOAT ||
            mode == KW_NO_SPIN || kwstruct->type == VTYP_STRING ||
            kwstruct->type == VTYP_LIST) {
        if (kwstruct->type != VTYP_LIST || mode != KW_NO_CB) {
            entry = gtk_entry_new();
            gtk_widget_show(entry);
            gtk_widget_set_size_request(entry, 80, -1);
            gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 2);
        }
        if (cb && mode != KW_NO_SPIN && mode != KW_NO_CB) {
            gtk_editable_set_editable(GTK_EDITABLE(entry), false);
            if (mode == KW_NORMAL)
                mode = KW_STR_PICK;
            GtkWidget *vbox = gtk_vbox_new(false, 0);
            gtk_widget_show(vbox);

            GtkWidget *arrow = gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_OUT);
            gtk_widget_show(arrow);
            GtkWidget *ebox = gtk_event_box_new();
            gtk_widget_show(ebox);
            gtk_container_add(GTK_CONTAINER(ebox), arrow);
            gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);
            g_signal_connect_after(G_OBJECT(ebox), "button_press_event",
                G_CALLBACK(cb), kwstruct);
            if (mode == KW_FLOAT) {
                gtk_widget_add_events(ebox, GDK_BUTTON_RELEASE_MASK);
                g_signal_connect_after(G_OBJECT(ebox),
                    "button_release_event", G_CALLBACK(cb), kwstruct);
            }
            gtk_box_pack_start(GTK_BOX(vbox), ebox, false, false, 0);

            arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
            gtk_widget_show(arrow);
            ebox = gtk_event_box_new();
            gtk_widget_show(ebox);
            gtk_container_add(GTK_CONTAINER(ebox), arrow);
            gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);
            g_signal_connect_after(G_OBJECT(ebox), "button_press_event",
                G_CALLBACK(cb), kwstruct);
            if (mode == KW_FLOAT) {
                gtk_widget_add_events(ebox, GDK_BUTTON_RELEASE_MASK);
                g_signal_connect_after(G_OBJECT(ebox),
                    "button_release_event", G_CALLBACK(cb), kwstruct);
            }
            g_object_set_data(G_OBJECT(ebox), "down", (void*)1);
            gtk_box_pack_start(GTK_BOX(vbox), ebox, false, false, 0);

            gtk_box_pack_start(GTK_BOX(hbox), vbox, false, false, 2);
        }
    }
    else if (mode == KW_NO_CB) {
        // KW_NO_CB is used exclusively by the buttons in the debug
        // group.

        VTvalue vv;
        if (Sp.GetVar("debug", VTYP_LIST, &vv)) {
            for (variable *vx = vv.get_list(); vx; vx = vx->next()) {
                if (vx->type() == VTYP_STRING) {
                    if (!strcmp(kwstruct->word, vx->string())) {
                        gtk_toggle_button_set_active(
                            GTK_TOGGLE_BUTTON(active), true);
                        break;
                    }
                }
            }
        }
        else if (Sp.GetVar("debug", VTYP_STRING, &vv)) {
            if (!strcmp(kwstruct->word, vv.get_string()))
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active), true);
        }
        else if (Sp.GetVar("debug", VTYP_BOOL, &vv)) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active), true);
        }
    }
    if (entry) {
        if (!v) {
            gtk_entry_set_text(GTK_ENTRY(entry),
                kwstruct->lastv1 ? kwstruct->lastv1 : defstr);
            if (entry2)
                gtk_entry_set_text(GTK_ENTRY(entry2),
                    kwstruct->lastv2 ? kwstruct->lastv2 : defstr);
        }
        else if (update)
            (*update)(true, v, this);
        g_signal_connect(G_OBJECT(entry), "changed",
            G_CALLBACK(value_changed), kwstruct);
        if (entry2)
            g_signal_connect(G_OBJECT(entry2), "changed",
                G_CALLBACK(value_changed), kwstruct);
    }
    if (mode != KW_NO_CB)
        set_state(v ? true : false);

    frame = gtk_frame_new(kwstruct->word);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    if (mode != KW_NO_CB)
        g_signal_connect(G_OBJECT(active), "clicked",
            G_CALLBACK(action_proc), kwstruct);
}


// Set sensitivity status in accord with Set button status
//
void
xEnt::set_state(bool state)
{
    if (active)
        GTKdev::SetStatus(active, state);
    if (deflt) {
        if (state)
            gtk_widget_set_sensitive(deflt, false);
        else {
            bool isdef = false;
            if (entry) {
                const char *str = gtk_entry_get_text(GTK_ENTRY(entry));
                if (!strcmp(defstr, str))
                    isdef = true;
            }
            gtk_widget_set_sensitive(deflt, !isdef);
        }
    }
    if (entry) {
        if (mode != KW_STR_PICK)
            gtk_editable_set_editable(GTK_EDITABLE(entry), !state);
        else
            gtk_editable_set_editable(GTK_EDITABLE(entry), false);
        gtk_widget_set_sensitive(entry, !state);
    }
}


namespace {
    void error_pr(const char *which, const char *minmax, const char *what)
    {
        GRpkg::self()->ErrPrintf(ET_ERROR, "bad %s%s value, must be %s.\n",
            which, minmax ? minmax : "", what);
    }
}


// Processing for the "set" button
//
void
xEnt::handler(void *data)
{
    xKWent *kwstruct = (xKWent*)data;
    variable v;
    bool state = GTKdev::GetStatus(active);
    // reset button temporarily, final status is set by callback()
    GTKdev::SetStatus(active, !state);

    if (kwstruct->type == VTYP_BOOL) {
        v.set_boolean(state);
        kwstruct->callback(state, &v);
    }
    else if (kwstruct->type == VTYP_NUM) {
        const char *string = gtk_entry_get_text(GTK_ENTRY(entry));
        int ival;
        if (sscanf(string, "%d", &ival) != 1) {
            error_pr(kwstruct->word, 0, "an integer");
            return;
        }
        v.set_integer(ival);
        kwstruct->callback(state, &v);
    }
    else if (kwstruct->type == VTYP_REAL) {
        const char *string = gtk_entry_get_text(GTK_ENTRY(entry));
        double *d = SPnum.parse(&string, false);
        if (!d) {
            error_pr(kwstruct->word, 0, "a real");
            return;
        }
        v.set_real(*d);
        kwstruct->callback(state, &v);
    }
    else if (kwstruct->type == VTYP_STRING) {
        const char *string = gtk_entry_get_text(GTK_ENTRY(entry));
        if (entry2) {
            // hack for two numeric fields
            const char *s = string;
            double *d = SPnum.parse(&s, false);
            if (!d) {
                error_pr(kwstruct->word, " min", "a real");
                return;
            }
            const char *string2 = gtk_entry_get_text(GTK_ENTRY(entry2));
            s = string2;
            d = SPnum.parse(&s, false);
            if (!d) {
                error_pr(kwstruct->word, " max", "a real");
                return;
            }
            char buf[256];
            snprintf(buf, sizeof(buf), "%s %s", string, string2);
            v.set_string(buf);
            kwstruct->callback(state, &v);
            return;
        }
        v.set_string(string);
        kwstruct->callback(state, &v);
    }
    else if (kwstruct->type == VTYP_LIST) {
        const char *string = gtk_entry_get_text(GTK_ENTRY(entry));
        wordlist *wl = CP.LexString(string);
        if (wl) {
            if (!strcmp(wl->wl_word, "(")) {
                // a list
                wordlist *wl0 = wl;
                wl = wl->wl_next;
                variable *v0 = CP.GetList(&wl);
                wordlist::destroy(wl0);
                if (v0) {
                    v.set_list(v0);
                    kwstruct->callback(state, &v);
                    return;
                }
            }
            else {
                char *s = wordlist::flatten(wl);
                v.set_string(s);
                delete [] s;
                kwstruct->callback(state, &v);
                wordlist::destroy(wl);
                return;
            }
        }
        GRpkg::self()->ErrPrintf(ET_ERROR, "parse error in string for %s.\n",
            kwstruct->word);
    }
}


//
// Some global callback functions for use as an argument to
// xEnt::create_widgets(), for generic data
//

// Boolean data
//
void
kw_bool_func(bool isset, variable*, xEnt *ent)
{
    ent->set_state(isset);
}


// Integer data
//
void
kw_int_func(bool isset, variable *v, xEnt *ent)
{
    ent->set_state(isset);
    if (ent->entry && isset) {
        if (GTK_IS_SPIN_BUTTON(ent->entry)) {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(ent->entry),
                v->integer());
        }
        else {
            char buf[64];
            snprintf(buf, sizeof(buf), "%d", v->integer());
            gtk_entry_set_text(GTK_ENTRY(ent->entry), buf);
        }
    }
}


// Real valued data
//
void
kw_real_func(bool isset, variable *v, xEnt *ent)
{
    ent->set_state(isset);
    if (ent->entry && isset) {
        if (GTK_IS_SPIN_BUTTON(ent->entry)) {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(ent->entry),
                v->real());
        }
        else {
            char buf[64];
            if (ent->mode == KW_NO_SPIN)
                snprintf(buf, sizeof(buf), "%g", v->real());
            else if (ent->mode == KW_FLOAT)
                snprintf(buf, sizeof(buf), "%.*e", ent->numd, v->real());
            else
                snprintf(buf, sizeof(buf), "%.*f", ent->numd, v->real());
            gtk_entry_set_text(GTK_ENTRY(ent->entry), buf);
        }
    }
}


// String data.
//
void
kw_string_func(bool isset, variable *v, xEnt *ent)
{
    ent->set_state(isset);
    if (ent->entry && isset)
        gtk_entry_set_text(GTK_ENTRY(ent->entry), v->string());
}


//
// The following functions implement an exponential notation spin button.
//

namespace {
    // Increment or decrement the current value.
    //
    void bump(xKWent *entry)
    {
        if (!gtk_widget_is_sensitive(entry->xent()->entry))
            return;
        char *string =
            gtk_editable_get_chars(GTK_EDITABLE(entry->xent()->entry), 0, -1);
        double d;
        sscanf(string, "%lf", &d);
        bool neg = false;
        if (d < 0) {
            neg = true;
            d = -d;
        }
        double logd = log10(d);
        int ex = (int)floor(logd);
        logd -= ex;
        double mant = pow(10.0, logd);

        if ((!entry->xent()->down && !neg) || (entry->xent()->down && neg))
            mant += entry->xent()->del;
        else {
            if (mant - entry->xent()->del < 1.0)
                mant = 1.0 - (1.0 - mant + entry->xent()->del)/10;
            else
                mant -= entry->xent()->del;
        }
        d = mant * pow(10.0, ex);
        if (neg)
            d = -d;
        if (d > entry->max)
            d = entry->max;
        else if (d < entry->min)
            d = entry->min;
        char buf[128];
        snprintf(buf, sizeof(buf), "%.*e", entry->xent()->numd, d);
        gtk_entry_set_text(GTK_ENTRY(entry->xent()->entry), buf);
    }


    // Repetitive timing loop.
    //
    int repeat_timer(void *client_data)
    {
        bump((xKWent*)client_data);
        return (true);
    }


    // Initial delay timer.
    //
    int delay_timer(void *client_data)
    {
        xKWent *entry = static_cast<xKWent*>(client_data);
        entry->xent()->thandle = g_timeout_add(50, repeat_timer, client_data);
        return (false);
    }
}


// This handler is passed in the callback arg of xEnt::create_widgets()
//
int
kw_float_hdlr(GtkWidget *caller, GdkEvent *event, void *client_data)
{
    xKWent *entry = static_cast<xKWent*>(client_data);
    if (event->type == GDK_BUTTON_PRESS) {
        entry->xent()->down =
            g_object_get_data(G_OBJECT(caller), "down") ? true : false;
        bump(entry);
        entry->xent()->thandle = g_timeout_add(200, delay_timer, entry);
    }
    else if (event->type == GDK_BUTTON_RELEASE)
        g_source_remove(entry->xent()->thandle);
    return (true);
}

