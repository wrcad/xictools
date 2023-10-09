
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
#include "keywords.h"
#include "kwords_analysis.h"
#include "kwords_fte.h"
#include "qttoolb.h"
#include "qttbdlg.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qtedit.h"
#include "qtinterf/qtcanvas.h"
#include "spnumber/spnumber.h"
#include "miscutil/filestat.h"
#include <signal.h>

#ifdef WIN32
#include <windows.h>
#else
#include "../../icons/wrspice_16x16.xpm"
#include "../../icons/wrspice_32x32.xpm"
#include "../../icons/wrspice_48x48.xpm"
#endif

#include <QLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDialog>
#include <QPushButton>
#include <QGroupBox>
#include <QScreen>
#include <QGuiApplication>

#ifdef WIN32
// Reference a symbol in the resource module so the resources are linked.
extern int ResourceModuleId;
namespace { int dummy = ResourceModuleId; }
#endif


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
            FC.setName(fn, n);
        }
        delete [] fn;
    }
}


namespace {
    // See if str means true.
    //
    bool affirm(const char *str)
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

        QTtoolbar::tbent_t *ent = TB()->FindEnt(word);
        if (!ent) {
            GRpkg::self()->ErrPrintf(ET_WARN,
                "tbsetup: bad keyword %s ignored.\n", word);
            if (!wl)
                break;
            continue;
        }
        ent->set_active(affirm(on));
        int x, y;
        if (sscanf(posx, "%d", &x) == 1 && sscanf(posy, "%d", &y) == 1)
            ent->set_xy(x, y);
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


// The Tools menu starts with fonts, in the same order.
QTtoolbar::tbent_t QTtoolbar::tb_entries[] = {
    tbent_t("toolbar",  tid_toolbar),
    tbent_t("bug",      tid_bug),
    tbent_t("font",     tid_font),
    tbent_t("files",    tid_files),
    tbent_t("circuits", tid_circuits),
    tbent_t("plots",    tid_plots),
    tbent_t("plotdefs", tid_plotdefs),
    tbent_t("colors",   tid_colors),
    tbent_t("vectors",  tid_vectors),
    tbent_t("variables", tid_variables),
    tbent_t("shell",    tid_shell),
    tbent_t("simdefs",  tid_simdefs),
    tbent_t("commands", tid_commands),
    tbent_t("runops",   tid_runops),
    tbent_t("debug",    tid_debug),
    tbent_t(0,          -1)
};

// Instantiate and export the toolbar container.
namespace { QTtoolbar _tb_; }
sToolbar *ToolBar() { return (TB()); }


QTtoolbar *QTtoolbar::instancePtr;

QTtoolbar::QTtoolbar()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class QTtoolbar already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    tb_mailer = 0;
    tb_fontsel = 0;
    for (int i = 0; i < TBH_end; i++) {
        tb_kw_help[i] = 0;
        tb_kw_help_pos[i].x = 0;
        tb_kw_help_pos[i].y = 0;
    }
    tb_suppress_update = false;
    tb_saved = false;
}


// Private static error exit.
//
void
QTtoolbar::on_null_ptr()
{
    fprintf(stderr, "Singleton class QTtoolbar used before instantiated.\n");
    exit(1);
}


//----- Toolbar interface stubs.

int
QTtoolbar::RegisterIdleProc(int(*proc)(void*), void *arg)
{
    if (QTdev::exists())
        return (QTdev::self()->AddIdleProc(proc, arg));
    return (0);
}


bool
QTtoolbar::RemoveIdleProc(int id)
{
    if (QTdev::exists())
        QTdev::self()->RemoveIdleProc(id);
    return (true);
}


int
QTtoolbar::RegisterTimeoutProc(int ms, int(*proc)(void*), void *arg)
{
    if (QTdev::exists())
        return (QTdev::self()->AddTimer(ms, proc, arg));
    return (0);
}


bool
QTtoolbar::RemoveTimeoutProc(int id)
{
    if (QTdev::exists())
        QTdev::self()->RemoveTimer(id);
    return (true);
}


void
QTtoolbar::RegisterBigForeignWindow(unsigned int w)
{
#ifdef WITH_X11
    if (QTdev::exists())
        QTdev::self()->RegisterBigForeignWindow(w);
#endif
}


void
QTtoolbar::SuppressUpdate(bool suppress)
{
    tb_suppress_update = suppress;
}


// Pop up the toolbar.
//
void
QTtoolbar::Toolbar()
{
    if (!CP.Display())
        return;

/* XXX App Icons?
gtk_window_set_default_icon_name("wrspice");
*/

    // Launch the application windows that start realized.
    for (tbent_t *tb = tb_entries; tb && tb->name(); tb++) {
        int id = tb->id();
        if (id == tid_toolbar) {
            TB()->PopUpToolbar(MODE_ON, tb->x(), tb->y());
            continue;
        }
        if (!tb->active())
            continue;
        if (id == tid_font)
            TB()->PopUpFont(MODE_ON, tb->x(), tb->y());
        else if (id == tid_files)
            TB()->PopUpFiles(MODE_ON, tb->x(), tb->y());
        else if (id == tid_circuits)
            TB()->PopUpCircuits(MODE_ON, tb->x(), tb->y());
        else if (id == tid_plots)
            TB()->PopUpPlots(MODE_ON, tb->x(), tb->y());
        else if (id == tid_plotdefs)
            TB()->PopUpPlotDefs(MODE_ON, tb->x(), tb->y());
        else if (id == tid_colors)
            TB()->PopUpColors(MODE_ON, tb->x(), tb->y());
        else if (id == tid_vectors)
            TB()->PopUpVectors(MODE_ON, tb->x(), tb->y());
        else if (id == tid_variables)
            TB()->PopUpVariables(MODE_ON, tb->x(), tb->y());
        else if (id == tid_shell)
            TB()->PopUpShellDefs(MODE_ON, tb->x(), tb->y());
        else if (id == tid_simdefs)
            TB()->PopUpSimDefs(MODE_ON, tb->x(), tb->y());
        else if (id == tid_commands)
            TB()->PopUpCmdConfig(MODE_ON, tb->x(), tb->y());
        else if (id == tid_runops)
            TB()->PopUpRunops(MODE_ON, tb->x(), tb->y());
        else if (id == tid_debug)
            TB()->PopUpDebugDefs(MODE_ON, tb->x(), tb->y());
        tb->set_active(false);
    }
}


//==========================================================================
//  Bug report dialog.

// Static function.
void
QTtoolbar::tb_mail_destroy_cb(GReditPopup *w)
{
    QTeditDlg *we = dynamic_cast<QTeditDlg*>(w);
    if (we)
        TB()->SetLoc(tid_bug, we);
}


// Dialog to email bug report text.
//
void
QTtoolbar::PopUpBugRpt(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        if (tb_mailer)
            tb_mailer->popdown();
        return;
    }
    if (!tb_mailer) {
        if (!Global.BugAddr()) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "no IP address set for bug reports.");
            return;
        }
        char buf[128];
        snprintf(buf, sizeof(buf), "WRspice %s bug", Global.Version());
        FixLoc(&x, &y);
        tb_mailer = PopUpMail(buf, Global.BugAddr(), tb_mail_destroy_cb,
            GRloc(LW_XYA, x, y));
        if (tb_mailer)
            tb_mailer->register_usrptr((void**)&tb_mailer);
    }
}


// Static function.
void
QTtoolbar::tb_font_cb(const char *btn, const char *name, void*)
{
    if (!name && !btn) {
        TB()->SetLoc(tid_font, TB()->tb_fontsel);
        TB()->SetActiveDlg(tid_font, 0);
    }
}


void
QTtoolbar::PopUpFont(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        if (!tb_fontsel)
            return;
        SetLoc(tid_font, tb_fontsel);
        ActiveFontsel()->popdown();
        tb_fontsel = 0;
        SetActiveDlg(tid_font, 0);
        return;
    }
    FixLoc(&x, &y);
    PopUpFontSel(this, GRloc(LW_XYA, x, y), MODE_ON, tb_font_cb, 0, FNT_FIXED);
    tb_fontsel = ((QTfontDlg*)ActiveFontsel());
    tb_fontsel->register_usrptr((void**)&tb_fontsel);
    SetActiveDlg(tid_font, tb_fontsel);
}

//XXX
void QTtoolbar::PopUpNotes()       { /* notes_proc(0, 0); */ }


void QTtoolbar::PopUpSpiceInfo(const char *msg)
{
    QTbag::PopUpInfo(MODE_ON, msg);
}

void QTtoolbar::UpdateMain(ResUpdType res)
{
    if (QTtbDlg::self())
        QTtbDlg::self()->update(res);
}

void
QTtoolbar::CloseGraphicsConnection()
{
    if (QTdev::exists() && QTdev::self()->ConnectFd() > 0)
        ::close(QTdev::ConnectFd());
}
//----- End Toolbar interface stubs.


// Static function.
// Save the location of a tool.
//
void
QTtoolbar::SetLoc(tid_id id, QDialog *w)
{
    tbent_t *tb = &tb_entries[id];
    QPoint pt = w->mapToGlobal(QPoint(0, 0));

    // Subtract the monitor origin.
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    QScreen *screen = w->screen();
#else
    QScreen *screen = QGuiApplication::primaryScreen();
#endif
    QRect r = screen->geometry();
    tb->set_xy(pt.x() - r.x(), pt.y() - r.y());
}


// Static function.
// Modify the location by adding the monitor origin, which was
// subtracted off in SetLoc.  This will allow switching monitors in
// multi-monitor systems.
//
void
QTtoolbar::FixLoc(int *px, int *py)
{
    if (QTtbDlg::self()) {
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
        QScreen *screen = QTtbDlg::self()->screen();
#else
        QScreen *screen = QGuiApplication::primaryScreen();
#endif
        QRect r = screen->geometry();
        *px += r.x();
        *py += r.y();
    }
}


// Static function.
// Function to return a command line for tbsetup which represents the
// current configuration.
//
char *
QTtoolbar::ConfigString()
{
    sLstr lstr;
    char buf[512];
    const char *off = "off", *on = "on";
    const char *fmt = "\\\n %s %s %d %d";
    if (!QTtbDlg::self())
        lstr.add("tbsetup toolbar off");
    else {
        lstr.add("tbsetup");
        QPoint pt = QTtbDlg::self()->mapToGlobal(QPoint(0, 0));
        snprintf(buf, sizeof(buf), fmt, "toolbar", "on", pt.x(), pt.y());
        lstr.add(buf);
        for (tbent_t *tb = tb_entries; tb && tb->name(); tb++) {
            if (tb->id() == tid_toolbar)
                continue;
            int x = tb->x();
            int y = tb->y();
            QDialog *dlg = tb->dialog();
            if (dlg) {
                pt = dlg->mapToGlobal(QPoint(0, 0));
                x = pt.x();
                y = pt.y();
                FixLoc(&x, &y);
            }
            snprintf(buf, sizeof(buf), fmt, tb->name(),
                tb->dialog() ? on : off, x, y);
            lstr.add(buf);
        }
    }
    lstr.add_c('\n');

    // Add the fonts
    const char *fn = FC.getName(FNT_FIXED);
    if (fn) {
        snprintf(buf, sizeof(buf), "setfont 1 %s\n", fn);
        lstr.add(buf);
    }
    fn = FC.getName(FNT_PROP);
    if (fn) {
        snprintf(buf, sizeof(buf), "setfont 2 %s\n", fn);
        lstr.add(buf);
    }
    fn = FC.getName(FNT_SCREEN);
    if (fn) {
        snprintf(buf, sizeof(buf), "setfont 3 %s\n", fn);
        lstr.add(buf);
    }
    fn = FC.getName(FNT_EDITOR);
    if (fn) {
        snprintf(buf, sizeof(buf), "setfont 4 %s\n", fn);
        lstr.add(buf);
    }
    fn = FC.getName(FNT_MOZY);
    if (fn) {
        snprintf(buf, sizeof(buf), "setfont 5 %s\n", fn);
        lstr.add(buf);
    }
    fn = FC.getName(FNT_MOZY_FIXED);
    if (fn) {
        snprintf(buf, sizeof(buf), "setfont 6 %s\n", fn);
        lstr.add(buf);
    }
    return (lstr.string_trim());
}

