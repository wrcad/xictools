
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

#include "qtdebug.h"
#include "editif.h"
#include "fio.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "menu.h"
#include "promptline.h"
#include "events.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qtsearch.h"
#include "qtinterf/qttextw.h"
#include "qtinterf/qtmsg.h"
#include "qtinterf/qtinput.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"

#include <QApplication>
#include <QLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QGroupBox>
#include <QToolButton>
#include <QPushButton>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidget>
#include <QHeaderView>
#include <QMouseEvent>
#include <QMimeData>
#include <QTextBlock>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>


//-----------------------------------------------------------------------------
// QTscriptDebuggerDlg:  Dialog and supporting functions for script debugging.
// Called from main menu: User/Debugger.
//
// Help system keywords used:
//  xic:debug

#ifdef Q_OS_MACOS
#define USE_QTOOLBAR
#endif

// Menu command to bring up a panel which facilitates debugging of
// scripts.
//
void
cMain::PopUpDebug(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        // Old QT5 will crash without delayed delete.
        QTscriptDebuggerDlg::self()->deleteLater();
        return;
    }

    if (QTscriptDebuggerDlg::self())
        return;

    new QTscriptDebuggerDlg(caller);

    QTscriptDebuggerDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTscriptDebuggerDlg::self(),
        QTmainwin::self()->Viewport());
    QTscriptDebuggerDlg::self()->show();
}


// This is a callback from the main menu that sets the file name when
// one of the scripts in the debug menu is selected.
//
bool
cMain::DbgLoad(MenuEnt *ent)
{
    if (QTscriptDebuggerDlg::self())
        return (QTscriptDebuggerDlg::self()->load_from_menu(ent));
    return (false);
}
// End of cMain functions.


#define DEF_FILE "unnamed"

namespace {
    // for hardcopies
    HCcb dbgHCcb =
    {
        0,            // hcsetup
        0,            // hcgo
        0,            // hcframe
        0,            // format
        0,            // drvrmask
        HClegNone,    // legend
        HCportrait,   // orient
        0,            // resolution
        0,            // command
        false,        // tofile
        "",           // tofilename
        0.25,         // left
        0.25,         // top
        8.0,          // width
        10.5          // height
    };
}


// Assumptions about EditPrompt()
//  1.  returns 0 immediately if call is reentrant
//  2.  ErasePrompt() safe to call, does nothing while
//      editor is active


QTscriptDebuggerDlg *QTscriptDebuggerDlg::instPtr;

QTscriptDebuggerDlg::QTscriptDebuggerDlg(GRobject c) : QTbag(this)
{
    instPtr = this;
    db_caller = c;
    db_modelabel = 0;
    db_title = 0;
    db_modebtn = 0;
    db_saveas = 0;
    db_undo = 0;
    db_redo = 0;
    db_filemenu = 0;
    db_editmenu = 0;
    db_execmenu = 0;
    db_load_btn = 0;
    db_load_pop = 0;
    db_vars_pop = 0;

    db_file_path = 0;
    db_line_ptr = 0;
    db_line_save = 0;
    db_dropfile = 0;
    db_file_ptr = 0;
    db_vlist = 0;
    db_search_pop = 0;

    db_line = 0;
    db_last_code = 0;
    db_status = Equiescent;
    db_mode = DBedit;
    db_text_changed = false;
    db_row_cb_flag = false;
    db_in_edit = false;
    db_in_undo = true;
    db_undo_list = 0;
    db_redo_list = 0;

    setWindowTitle(tr("Script Debugger"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // menu bar
    //
#ifdef USE_QTOOLBAR
    QToolBar *menubar = new QToolBar(this);
#else
    QMenuBar *menubar = new QMenuBar(this);
#endif
    vbox->addWidget(menubar);

    // File menu.
    QAction *a;
#ifdef USE_QTOOLBAR
    a = menubar->addAction(tr("&File"));
    db_filemenu = new QMenu();
    a->setMenu(db_filemenu);
    QToolButton *tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    db_filemenu = menubar->addMenu(tr("&File"));
#endif
    // _New, 0, db_action_proc, NewCode, 0
    a = db_filemenu->addAction(tr("&New"));
    a->setData(NewCode);
    // _Load", <control>L", db_action_proc, LoadCode, 0
    a = db_filemenu->addAction(tr("&Load"));
    a->setData(LoadCode);
    a->setShortcut(QKeySequence("Ctrl+L"));
    db_load_btn = a;
    // _Print, <control>P, db_action_proc, PrintCode, 0
    a = db_filemenu->addAction(tr("&Print"));
    a->setData(PrintCode);
    a->setShortcut(QKeySequence("Ctrl+P"));
    // _Save As, <alt>A, db_action_proc, SaveAsCode, 0
    a = db_filemenu->addAction(tr("&Save As"));
    a->setData(SaveAsCode);
    a->setShortcut(QKeySequence("Alt+A"));
    db_saveas = a;
#ifdef Q_OS_WIN
    // _Write CRLF, 0, db_action_proc, CRLFcode, <CheckItem>"
    a = db_filemenu->addAction(tr("&Write CRLF"));
    a->setCheckable(true);
    a->setData(CRLFcode);
    QTdev::SetStatus(a, QTdev::self()->GetCRLFtermination());
#endif

    db_filemenu->addSeparator();
    // _Quit, <control>Q, db_action_proc, CancelCode, 0
    a = db_filemenu->addAction(tr("&Quit"));
    a->setData(CancelCode);
    a->setShortcut(QKeySequence("Ctrl+Q"));
    connect(db_filemenu, &QMenu::triggered,
        this, &QTscriptDebuggerDlg::file_menu_slot);

    // Edit menu.
#ifdef USE_QTOOLBAR
    a = menubar->addAction(tr("&Edit"));
    db_editmenu = new QMenu();
    a->setMenu(db_editmenu);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    db_editmenu = menubar->addMenu(tr("&Edit"));
#endif
    // Undo, <Alt>U, db_undo_proc, 0, 0
    a = db_editmenu->addAction(tr("Undo"));
    a->setShortcut(QKeySequence("Alt+U"));
    db_undo = a;
    // Redo, <Alt>R, db_redo_proc, 0, 0
    a = db_editmenu->addAction(tr("Redo"));
    a->setShortcut(QKeySequence("Alt+R"));
    db_redo = a;

    db_editmenu->addSeparator();
    // Cut to Clipboard, <control>X, db_cut_proc, 0, 0
    a = db_editmenu->addAction(tr("Cut to Clipboard"));
    a->setData(1);
    a->setShortcut(QKeySequence("Ctrl+X"));
    // Copy to Clipboard, <control>C, db_copy_proc,  0, 0
    a = db_editmenu->addAction(tr("Copy to Clipboard"));
    a->setData(2);
    a->setShortcut(QKeySequence("Ctrl+C"));
    // Paste from Clipboard, <control>V, db_paste_proc, 0, 0
    a = db_editmenu->addAction(tr("Paste from Clipboard"));
    a->setData(3);
    a->setShortcut(QKeySequence("Ctrl+V"));
#ifdef Q_OS_LINUX
    // Paste Primary, <alt>P, db_paste_prim_proc, 0, 0
    a = db_editmenu->addAction(tr("Paste Primary"));
    a->setData(4);
    a->setShortcut(QKeySequence("Alt+P"));
#endif
    connect(db_editmenu, &QMenu::triggered,
        this, &QTscriptDebuggerDlg::edit_menu_slot);

    // Execute menu.
#ifdef USE_QTOOLBAR
    a = menubar->addAction(tr("E&xecute"));
    db_execmenu = new QMenu();
    a->setMenu(db_execmenu);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    db_execmenu = menubar->addMenu(tr("E&xecute"));
#endif
    // _Run, <control>R, db_action_proc, RunCode, 0
    a = db_execmenu->addAction(tr("&Run"));
    a->setData(RunCode);
    a->setShortcut(QKeySequence("Ctrl+R"));
    // S_tep, <control>T, db_action_proc, StepCode, 0
    a = db_execmenu->addAction(tr("S&tep"));
    a->setData(StepCode);
    a->setShortcut(QKeySequence("Ctrl+T"));
    // R_eset, <control>E, db_action_proc, StartCode, 0
    a = db_execmenu->addAction(tr("R&eset"));
    a->setData(StartCode);
    a->setShortcut(QKeySequence("Ctrl+E"));
    // _Monitor, <control>M, db_action_proc, MonitorCode,0
    a = db_execmenu->addAction(tr("&Monitor"));
    a->setData(MonitorCode);
    a->setShortcut(QKeySequence("Ctrl+M"));
    connect(db_execmenu, &QMenu::triggered,
        this, &QTscriptDebuggerDlg::exec_menu_slot);

    // Options menu.
#ifdef USE_QTOOLBAR
    a = menubar->addAction(tr("&Options"));
    QMenu *menu = new QMenu();
    a->setMenu(menu);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    QMenu *menu = menubar->addMenu(tr("&Options"));
#endif
    // _Search, 0, db_search_proc, 0, <CheckItem>);
    a = menu->addAction(tr("&Search"));
    a->setData(1);
    a->setCheckable(true);
    // _Font, 0, db_font_proc, 0, <CheckItem>
    a = menu->addAction(tr("&Font"));
    a->setData(2);
    a->setCheckable(true);
    connect(menu, &QMenu::triggered,
        this, &QTscriptDebuggerDlg::options_menu_slot);

    // Help menu.
#ifdef USE_QTOOLBAR
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    menubar->addAction(tr("&Help"), Qt::CTRL|Qt::Key_H, this,
        &QTscriptDebuggerDlg::help_slot);
#else
    a = menubar->addAction(tr("&Help"), this, &QTscriptDebuggerDlg::help_slot);
    a->setShortcut(QKeySequence("Ctrl+H"));
#endif
#else
    menu = menubar->addMenu(tr("&Help"));
    // _Help, <control>H, db_action_proc, HelpCode, 0
    a = menu->addAction(tr("&Help"), this, &QTscriptDebuggerDlg::help_slot);
    a->setShortcut(QKeySequence("Ctrl+H"));
#endif

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    db_modebtn = new QToolButton();
    db_modebtn->setText(tr("Run"));
    hbox->addWidget(db_modebtn);
    db_modebtn->setEnabled(false);
    db_modebtn->setMaximumWidth(80);
    connect(db_modebtn, &QAbstractButton::clicked,
        this, &QTscriptDebuggerDlg::mode_btn_slot);

    // labels in frame
    //
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    db_modelabel = new QLabel(tr("Edit Mode"));
    hb->addWidget(db_modelabel);
    db_title = new QLabel("");
    hb->addWidget(db_title);

    // text window with scroll bar
    //
    wb_textarea = new QTtextEdit();
    wb_textarea->setMouseTracking(true);
    wb_textarea->setAcceptDrops(true);
    vbox->addWidget(wb_textarea);
    connect(wb_textarea, &QTtextEdit::press_event,
        this, &QTscriptDebuggerDlg::mouse_press_slot);
    connect(wb_textarea, &QTtextEdit::release_event,
        this, &QTscriptDebuggerDlg::mouse_release_slot);
    connect(wb_textarea, &QTtextEdit::textChanged,
        this, &QTscriptDebuggerDlg::text_changed_slot);
    connect(wb_textarea, &QTtextEdit::mime_data_handled,
        this, &QTscriptDebuggerDlg::mime_data_handled_slot);
    connect(wb_textarea, &QTtextEdit::mime_data_delivered,
        this, &QTscriptDebuggerDlg::mime_data_delivered_slot);
    connect(wb_textarea, &QTtextEdit::key_press_event,
        this, &QTscriptDebuggerDlg::key_press_slot);

    QTextDocument *doc = wb_textarea->document();
    connect(doc, &QTextDocument::contentsChange,
        this, &QTscriptDebuggerDlg::text_change_slot);

    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_FIXED))
        wb_textarea->setFont(*fnt);
    connect(QTfont::self(), &QTfont::fontChanged,
        this, &QTscriptDebuggerDlg::font_changed_slot, Qt::QueuedConnection);

    db_in_undo = false;
    check_sens();
    wb_textarea->setReadOnly(false);
    db_execmenu->setEnabled(false);
}


QTscriptDebuggerDlg::~QTscriptDebuggerDlg()
{
    instPtr = 0;
    stringlist::destroy(db_vlist);
    delete [] db_line_save;
    delete [] db_dropfile;
    delete [] db_file_path;

    if (db_in_edit || db_row_cb_flag)
        PL()->AbortEdit();
    if (db_load_pop)
        db_load_pop->popdown();
    if (db_vars_pop)
        db_vars_pop->popdown();
    if (db_caller)
        QTdev::Deselect(db_caller);
    histlist::destroy(db_undo_list);
    histlist::destroy(db_redo_list);

    SI()->Clear();
}


QSize
QTscriptDebuggerDlg::sizeHint() const
{
    int fw, fh;
    QTfont::stringBounds(0, FNT_FIXED, &fw, &fh);
    return (QSize(80*fw + 4, 32*fh));
}


// Check/set menu item sensitivity for items that change during editing.
//
void
QTscriptDebuggerDlg::check_sens()
{
    if (db_undo)
        db_undo->setEnabled(db_undo_list != 0);
    if (db_redo)
        db_redo->setEnabled(db_redo_list != 0);
}


// Set configuration mode.
//
void
QTscriptDebuggerDlg::set_mode(DBmode mode)
{
    if (mode == DBedit) {
        if (db_mode != DBedit) {
            db_mode = DBedit;
            wb_textarea->set_editable(true);
            refresh(true, locPresent);
            db_modelabel->setText(tr("Edit Mode"));
            db_execmenu->setEnabled(false);
            db_editmenu->setEnabled(true);

            wb_textarea->setCursor(Qt::IBeamCursor);
        }
    }
    else if (mode == DBrun) {
        if (db_mode != DBrun) {
            db_mode = DBrun;
            wb_textarea->set_editable(false);
            refresh(true, locPresent);
            start();
            db_modelabel->setText(tr("Exec Mode"));
            db_execmenu->setEnabled(true);
            db_editmenu->setEnabled(false);

            wb_textarea->setCursor(Qt::ArrowCursor);
        }
    }
}


bool
QTscriptDebuggerDlg::load_from_menu(MenuEnt *ent)
{
    if (db_load_pop) {
        const char *entry = ent->menutext + strlen("/User/");
        // entry is the same as ent->entry, but contains the menu path
        // for submenu items
        char buf[256];
        if (lstring::strdirsep(entry)) {
            // from submenu, add a distinguishing prefix to avoid
            // confusion with file path
            snprintf(buf, sizeof(buf), "%s%s", SCR_LIBCODE, entry);
            entry = buf;
        }
        char *scrfile = XM()->FindScript(entry);
        if (scrfile) {
            db_load_pop->update(0, scrfile);
            delete [] scrfile;
            return (true);
        }
    }
    return (false);
}


// Set db_line_ptr pointing at the line for the "Line" field.
//
void
QTscriptDebuggerDlg::set_line()
{
    delete [] db_line_save;
    db_line_save = 0;

    if (!wb_textarea) {
        db_line_ptr = 0;
        return;
    }
    char *string = wb_textarea->get_chars();
    int count = 0;
    char *t;
    for (t = string; *t && count < db_line; t++) {
        if (*t == '\n')
            count++;
    }
    if (!*t || !*(t+1) || !*(t+2))
        db_line_ptr = 0;
    else
        db_line_ptr = db_line_save = lstring::copy(t);
    delete [] string;
}


// See if db_line now points beyond the end, in which case we're done.
//
bool
QTscriptDebuggerDlg::is_last_line()
{
    char *string = wb_textarea->get_chars();
    int count = 0;
    char *t;
    for (t = string; *t && count < db_line; t++) {
        if (*t == '\n')
            count++;
    }
    bool ret = (!*t || !*(t+1) || !*(t+2));
    delete [] string;
    return (ret);
}


// Execute a single line of code.
//
void
QTscriptDebuggerDlg::step()
{
    if (!instPtr)
        return;
    if (db_row_cb_flag) {
        // We're waiting for input in the prompt line, back out.
        PL()->AbortEdit();
        return;
    }
    set_line();
    if (!SI()->IsInBlock() && !db_line_ptr) {
        PL()->SavePrompt();
        start();
        PL()->RestorePrompt();
        // Get focus back in case it was lost.
        activateWindow();
        wb_textarea->setFocus();
        return;
    }
    db_status = Eexecuting;
    set_sens(false);
    db_line = SI()->Interpret(0, 0, &db_line_ptr, 0, true);
    if (!instPtr) {
        // debugger window deleted
        SI()->Clear();
        EditIf()->ulCommitChanges();
        return;
    }
    if (SI()->IsHalted() || (!SI()->IsInBlock() && is_last_line())) {
        EditIf()->ulCommitChanges();
        PL()->SavePrompt();
        start();
        PL()->RestorePrompt();
    }
    else
        refresh(false, locFollowCurrent);
    db_status = Equiescent;
    set_sens(true);
    update_variables();
    // Get focus back in case it was lost.
    activateWindow();
    wb_textarea->setFocus();
}


// Execute until the next breakpoint.
//
void
QTscriptDebuggerDlg::run()
{
    if (db_row_cb_flag) {
        // We're waiting for input in the prompt line, back out.
        PL()->AbortEdit();
        return;
    }
    db_status = Eexecuting;
    set_sens(false);
    int tline = db_line;
    db_line = -1;
    refresh(false, locPresent);  // erase caret
    db_line = tline;
    for (;;) {
        set_line();
        if (!SI()->IsInBlock() && !db_line_ptr) {
            // shouldn't happen
            SI()->Clear();
            PL()->SavePrompt();
            start();
            PL()->RestorePrompt();
            break;
        }

        db_line = SI()->Interpret(0, 0, &db_line_ptr, 0, true);
        if (!instPtr) {
            // debugger window deleted
            SI()->Clear();
            EditIf()->ulCommitChanges();
            return;
        }
        if (SI()->IsHalted() || (!SI()->IsInBlock() &&
                (!db_line_ptr || !*db_line_ptr || !*(db_line_ptr+1) ||
                !*(db_line_ptr+2)))) {
            // finished
            SI()->Clear();
            PL()->SavePrompt();
            start();
            PL()->RestorePrompt();
            break;
        }
        if (QTpkg::self()->CheckForInterrupt()) {
            // ^C typed
            refresh(false, locFollowCurrent);
            break;
        }

        int i;
        for (i = 0; i < NUMBKPTS; i++)
            if (db_breaks[i].line == db_line && db_breaks[i].active)
                break;
        if (i < NUMBKPTS) {
            // hit a breakpoint
            refresh(false, locFollowCurrent);
            break;
        }
    }
    EditIf()->ulCommitChanges();
    db_status = Equiescent;
    set_sens(true);
    update_variables();
}


// Desensitize buttons while executing.
//
void
QTscriptDebuggerDlg::set_sens(bool sens)
{
    db_filemenu->setEnabled(sens);
    db_execmenu->setEnabled(sens);
}


// Reset a paused executing script.
//
void
QTscriptDebuggerDlg::start()
{
    if (db_row_cb_flag) {
        // We're waiting for input in the prompt line, back out.
        PL()->AbortEdit();
        return;
    }
    SI()->Init();
    db_line = 0;
    set_line();
    db_line = SI()->Interpret(0, 0, &db_line_ptr, 0, true);
    refresh(false, locStart);
    EV()->InitCallback();
    EditIf()->ulListCheck("run", CurCell(), false);
}


// Set/reset breakpoint at line.
//
void
QTscriptDebuggerDlg::breakpoint(int line)
{
    int i;
    if (line < 0) {
        for (i = 0; i < NUMBKPTS; i++)
            db_breaks[i].active = false;
        return;
    }
    if (line == db_line) {
        // Clicking on the active line steps it
        QTpkg::self()->RegisterIdleProc(db_step_idle, 0);
        return;
    }
    for (i = 0; i < NUMBKPTS; i++) {
        if (db_breaks[i].line == line) {
            db_breaks[i].active = !db_breaks[i].active;
            refresh(false, locPresent);
            return;
        }
    }
    for (i = NUMBKPTS-1; i > 0; i--)
        db_breaks[i] = db_breaks[i-1];
    db_breaks[0].line = line;
    db_breaks[0].active = true;
    refresh(false, locPresent);
}


// Dump the buffer to fname.  Return false if error.
//
bool
QTscriptDebuggerDlg::write_file(const char *fname)
{
    if (!filestat::create_bak(fname)) {
        QTpkg::self()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        return (false);
    }
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        // shouldn't happen
        QTbag::PopUpMessage("Can't open file, text not saved", true);
        return (false);
    }
#ifdef Q_OS_WIN
    char lastc = 0;
#endif
    char *string = wb_textarea->get_chars();
    if (db_mode == DBedit) {
#ifdef Q_OS_WIN
        char *s = string;
        while (*s) {
            if (!QTdev::self()->GetCRLFtermination()) {
                if (s[0] == '\r' && s[1] == '\n') {
                    lastc = *s++;
                    continue;
                }
            }
            else if (s[0] == '\n' && lastc != '\r')
                putc('\r', fp);
            putc(s[0], fp);
            lastc = *s++;
        }
#else
        fputs(string, fp);
#endif
    }
    else {
        char *s = string;
        if (*s && *(s+1)) {
            s += 2;
#ifdef Q_OS_WIN
            while (*s) {
                if (!QTdev::self()->GetCRLFtermination()) {
                    if (s[0] == '\r' && s[1] == '\n') {
                        lastc = *s++;
                        continue;
                    }
                }
                else if (s[0] == '\n' && lastc != '\r')
                    putc('\r', fp);
                putc(s[0], fp);
                lastc = *s++;
            }
#else
            while (*s) {
                if (*s == '\n') {
                    putc('\n', fp);
                    s++;
                    if (!*s || !*(s+1))
                        break;
                    s += 2;
                }
                else {
                    putc(*s, fp);
                    s++;
                }
            }
#endif
        }
    }
    fclose(fp);
    delete [] string;
    return (true);
}


// If unsaved text, warn.  Return true to abort operation.
//
bool
QTscriptDebuggerDlg::check_save(int code)
{
    if (db_text_changed) {
        const char *str;
        char buf[128];
        switch (code) {
        case CancelCode:
            str = "Press Quit again to quit\nwithout saving changes.";
            break;
        case LoadCode:
            str = "Press Load again to load.";
            break;
        case NewCode:
            str = "Press New again to clear.";
            break;
        default:
            return (false);
        }
        if (db_last_code != code) {
            db_last_code = code;
            snprintf(buf, sizeof(buf), "Text has been modified.  %s", str);
            QTbag::PopUpMessage(buf, false);
            return (true);
        }
        if (wb_message)
            wb_message->popdown();
    }
    return (false);
}


// Update the text listing.
// If mode_switch, strip margin for edit mode.
//
void
QTscriptDebuggerDlg::refresh(bool mode_switch, locType loc, bool clear)
{
    char *s = listing(clear ? false : mode_switch);
    double val = wb_textarea->get_scroll_value();
    if (db_mode == DBedit) {
        db_in_undo = true;
        wb_textarea->set_chars(s);
        db_in_undo = false;
        if (loc == locStart)
            wb_textarea->set_scroll_value(0);
        else
            wb_textarea->set_scroll_value(val);
    }
    else {
        db_in_undo = true;
        if (mode_switch || clear) {
            wb_textarea->clear();
            QColor c = QTbag::PopupColor(GRattrColorHl1);
            char *t = s;
            while (*t) {
                char *end = t;
                while (*end && *end != '\n')
                    end++;
                if (*end == '\n')
                    end++;
                wb_textarea->insert_chars_at_point(&c, t, 1, -1);
                wb_textarea->insert_chars_at_point(0, t+1, end-t-1, -1);
                if (!*end)
                    break;
                t = end;
            }
        }
        else {
            char *str = wb_textarea->get_chars();
            QColor c = QTbag::PopupColor(GRattrColorHl1);
            char *t = s;
            while (*t) {
                char *end = t;
                while (*end && *end != '\n')
                    end++;
                if (*end == '\n')
                    end++;
                if (*t != str[t-s])
                    wb_textarea->replace_chars(&c, t, t-s, t-s + 1);
                if (!*end)
                    break;
                t = end;
            }
            delete [] str;
        }
        db_in_undo = false;
        if (loc == locStart)
            wb_textarea->set_scroll_value(0);
        else if (loc == locFollowCurrent) {
            QTextCursor cur(wb_textarea->document()->findBlockByLineNumber(
                db_line));
            wb_textarea->moveCursor(QTextCursor::End);
            wb_textarea->setTextCursor(cur);
        }
        else
            wb_textarea->set_scroll_value(val);
    }
    delete [] s;
}


// If the db_file_ptr is not 0, read in the file.  Otherwise return the
// buffer contents.  If mode_switch is true, an edit mode switch is in
// progress.
//
char *
QTscriptDebuggerDlg::listing(bool mode_switch)
{
// line buffer size
#define LINESIZE 1024

    char buf[LINESIZE];
    if (!db_file_ptr) {
        char *str = wb_textarea->get_chars();
        char *t = str;
        if (mode_switch) {
            if (db_mode == DBedit) {
                // switching to edit mode, strip margin
                char *ostr = str;
                t = new char[strlen(t)+1];
                char *s = t;
                if (*str && *(str+1)) {
                    str += 2;
                    while (*str) {
                        if (*str == '\n') {
                            *s++ = '\n';
                            str++;
                            if (*str && *(str+1))
                                str += 2;
                            else
                                break;
                        }
                        else
                            *s++ = *str++;
                    }
                }
                *s = '\0';
                delete [] ostr;
                str = t;
            }
            else {
                // switching from edit mode, add margin
                int i;
                for (i = 0; *t; i++, t++) {
                    if (*t == '\n')
                        i += 2;
                }
                t = new char[i+5];
                char *s = t;
                *s++ = ' ';
                *s++ = ' ';
                char *ostr = str;
                while (*str) {
                    if (*str == '\n') {
                        *s++ = '\n';
                        *s++ = ' ';
                        *s++ = ' ';
                        str++;
                    }
                    else
                        *s++ = *str++;
                }
                *s = '\0';
                delete [] ostr;
                str = t;
            }
        }
        if (db_mode == DBrun) {
            int line = 0;
            for (;;) {
                if (!*t || !*(t+1) || !*(t+2))
                    break;
                int i;
                for (i = 0; i < NUMBKPTS; i++)
                    if (db_breaks[i].active && line == db_breaks[i].line)
                        break;
                if (i == NUMBKPTS)
                    t[0] = ' ';
                else
                    t[0] = 'B';
                if (line == db_line)
                    t[0] = '>';
                t[1] = ' ';
                while (*t && *t != '\n')
                    t++;
                if (!*t)
                    break;
                line++;
                t++;
            }
        }
        return (str);
    }
    char *str = lstring::copy("");
    int line = 0;
    long ftold = ftell(db_file_ptr);
    rewind(db_file_ptr);
    while (fgets(buf+2, 1021, db_file_ptr) != 0) {
        NTstrip(buf+2);
        if (db_mode == DBedit)
            str = lstring::build_str(str, buf+2);
        else {
            int i;
            for (i = 0; i < NUMBKPTS; i++)
                if (db_breaks[i].active && line == db_breaks[i].line)
                    break;
            if (i == NUMBKPTS)
                buf[0] = ' ';
            else
                buf[0] = 'B';
            if (line == db_line)
                buf[0] = '>';
            buf[1] = ' ';
            str = lstring::build_str(str, buf);
        }
        line++;
    }
    fseek(db_file_ptr, ftold, SEEK_SET);
    return (str);
}


// Solicit a list of variables, and pop up the monitor window.
//
void
QTscriptDebuggerDlg::monitor()
{
    char buf[256];
    char *s = buf;
    *s = 0;
    for (stringlist *wl = db_vlist; wl; wl = wl->next) {
        if (s - buf + strlen(wl->string) + 2 >= 256)
            break;
        if (s > buf)
           *s++ = ' ';
        strcpy(s, wl->string);
        while (*s)
            s++;
    }
    db_in_edit = true;
    char *in = PL()->EditPrompt("Variables? ", buf);
    db_in_edit = false;
    PL()->ErasePrompt();
    if (!in)
        return;
    stringlist::destroy(db_vlist);
    db_vlist = db_mklist(in);

    if (db_vars_pop) {
        db_vars_pop->update(db_vlist);
        return;
    }
    if (!db_vlist)
        return;
    if (!QTdev::exists() || !QTmainwin::exists())
        return;

    db_vars_pop = new QTdbgVarsDlg(&db_vars_pop, this);

    QTdev::self()->SetPopupLocation(GRloc(LW_LR), db_vars_pop,
        wb_shell);
    db_vars_pop->show();

    // Calling this from here avoids spontaneously selecting the first
    // entry, not sure that I know why.  The selection is very
    // undesirable since it prompts for a value.
    db_vars_pop->update(db_vlist);
}


void
QTscriptDebuggerDlg::open_file()
{
    db_file_ptr = fopen(db_file_path, "r");
    if (db_file_ptr) {
        histlist::destroy(db_undo_list);
        db_undo_list = 0;
        histlist::destroy(db_redo_list);
        db_redo_list = 0;
        check_sens();
        db_in_undo = true;
        set_mode(DBedit);
        SI()->Clear();
        breakpoint(-1);
        db_line = 0;
        start();
        db_text_changed = false;
        db_in_undo = false;
        db_saveas->setEnabled(false);
        db_title->setText(db_file_path);
        fclose(db_file_ptr);
        db_file_ptr = 0;
        db_modebtn->setEnabled(true);
    }
}


namespace {
    // Return a variable token.  The tokens can be separated by white
    // space and/or commas.  A token can have a following range
    // specification in square brackets that can contain commas and/or
    // white space.
    //
    char *
    getvar(const char **s)
    {
        char buf[512];
        int i = 0;
        if (s == 0 || *s == 0)
            return (0);
        while (isspace(**s) || **s == ',')
            (*s)++;
        if (!**s)
            return (0);
        while (**s && !isspace(**s) && **s != '[')
            buf[i++] = *(*s)++;
        if (**s == '[') {
            buf[i++] = *(*s)++;
            while (**s && **s != ']')
                buf[i++] = *(*s)++;
            if (**s == ']')
                buf[i++] = *(*s)++;
        }
        buf[i] = '\0';
        while (isspace(**s) || **s == ',')
            (*s)++;
        return (lstring::copy(buf));
    }
}


// Static function.
// Return a list of the tokens in str.
//
stringlist *
QTscriptDebuggerDlg::db_mklist(const char *str)
{
    if (!str)
        return (0);
    while (isspace(*str))
        str++;
    stringlist *wl = 0, *wl0 = 0;
    if (!strcmp(str, "all") || ((*str == '*' || *str == '.') && !*(str+1))) {
        for (Variable *v = SIparse()->getVariables(); v; v = v->next) {
            if (!wl0)
                wl = wl0 = new stringlist;
            else {
                wl->next = new stringlist;
                wl = wl->next;
            }
            wl->string = lstring::copy(v->name);
        }
    }
    else {
        char *tok;
        while ((tok = getvar(&str)) != 0) {
            if (!wl0)
                wl = wl0 = new stringlist(tok, 0);
            else {
                wl->next = new stringlist(tok, 0);
                wl = wl->next;
            }
        }
    }
    return (wl0);
}


// Static function.
// Export for the variables window selection handler.
//
const char *
QTscriptDebuggerDlg::var_prompt(const char *text, const char *buf, bool *busy)
{
    *busy = false;
    if (!instPtr)
        return (0);
    if (instPtr->db_row_cb_flag) {
        PL()->EditPrompt(buf, text, PLedUpdate);
        *busy = true;
        return (0);
    }
    instPtr->db_row_cb_flag = true;
    char *in = PL()->EditPrompt(buf, text);
    if (!instPtr) {
        *busy = true;
        return (0);
    }
    instPtr->db_row_cb_flag = false;
    return (in);
}


// Static function.
// Don't want to wait in the interrupt handler, take care of single
// step when user clicks on active line.
//
int
QTscriptDebuggerDlg::db_step_idle(void*)
{
    if (instPtr)
        instPtr->step();
    return (false);
}


// Static function.
// Callback for the load command popup.
//
ESret
QTscriptDebuggerDlg::db_open_cb(const char *namein, void*)
{
    delete [] instPtr->db_dropfile;
    instPtr->db_dropfile = 0;
    if (namein && *namein) {
        char *name = pathlist::expand_path(namein, false, true);
        char *t = strrchr(name, '.');
        if (!t || !lstring::cieq(t, SCR_SUFFIX)) {
            char *ct = new char[strlen(name) + strlen(SCR_SUFFIX) + 1];
            strcpy(ct, name);
            strcat(ct, SCR_SUFFIX);
            delete [] name;
            name = ct;
        }
        FILE *fp;
        if (lstring::strdirsep(name))
            fp = fopen(name, "r");
        else {
            // Search will check CWD first, then path.
            char *fpath;
            fp = pathlist::open_path_file(name,
                CDvdb()->getVariable(VA_ScriptPath), "r", &fpath, true);
            if (fpath) {
                delete [] name;
                name = fpath;
            }
        }
        if (fp) {
            fclose(fp);
            delete [] instPtr->db_file_path;
            instPtr->db_file_path = name;
            EV()->InitCallback();
            QTpkg::self()->RegisterIdleProc(db_open_idle, 0);
            return (ESTR_DN);
        }
        else
            delete [] name;
    }
    instPtr->db_load_pop->update("No file found, try again: ", 0);
    return (ESTR_IGN);
}


// Static function.
// This needs to be in an idle proc, otherwise problems if a script is
// already running.
//
int
QTscriptDebuggerDlg::db_open_idle(void*)
{
    if (instPtr)
        instPtr->open_file();
    return (0);
}


// Static function.
// Callback passed to PopUpInput() to actually save the file.
//
void
QTscriptDebuggerDlg::db_do_saveas_proc(const char *fnamein, void*)
{
    char *fname = pathlist::expand_path(fnamein, false, true);
    if (!instPtr->write_file(fname)) {
        delete [] fname;
        return;
    }
    instPtr->db_text_changed = false;
    instPtr->db_saveas->setEnabled(false);
    if (instPtr->wb_input)
        instPtr->wb_input->popdown();
    instPtr->db_title->setText(fname);
    delete [] fname;
}


void
QTscriptDebuggerDlg::file_menu_slot(QAction *a)
{
    if (a->data().toInt() == NewCode) {
        if (check_save(NewCode))
            return;
        histlist::destroy(db_undo_list);
        db_undo_list = 0;
        histlist::destroy(db_redo_list);
        db_redo_list = 0;
        check_sens();
        wb_textarea->clear();
        delete [] db_file_path;
        db_file_path = 0;
        SI()->Clear();
        SI()->Init();
        breakpoint(-1);
        db_line = 0;
        db_saveas->setEnabled(false);
        db_title->setText(DEF_FILE);
        set_mode(DBedit);
        db_text_changed = false;
        return;
    }
    if (a->data().toInt() == LoadCode) {
        if (db_load_pop) {
            db_load_pop->popdown();
            return;
        }
        if (check_save(LoadCode))
            return;
        db_load_pop = PopUpEditString(0, GRloc(),
            "Enter script file name: ", db_dropfile, db_open_cb,
                0, 214, 0);
        if (db_load_pop)
            db_load_pop->register_usrptr((void**)&db_load_pop);
        return;
    }
    if (a->data().toInt() == PrintCode) {
        if (dbgHCcb.command)
            delete [] dbgHCcb.command;
        dbgHCcb.command = lstring::copy(GRappIf()->GetPrintCmd());
        PopUpPrint(a, &dbgHCcb, HCtext);
        return;
    }
    if (a->data().toInt() == SaveAsCode) {
        if (wb_input) {
            wb_input->popdown();
            return;
        }
        PopUpInput(0, db_file_path, "Save File", db_do_saveas_proc, 0);
        return;
    }
    if (a->data().toInt() == CRLFcode) {
#ifdef Q_OS_WIN
        QTdev::self()->SetCRLFtermination(QTdev::GetStatus(a));
#endif
        return;
    }
    if (a->data().toInt() == CancelCode) {
        if (db_status == Eexecuting)
            EV()->InitCallback();  // force a return if pushed
        if (check_save(CancelCode)) {
            if (db_caller)
                QTdev::Select(db_caller);
            return;
        }
        XM()->PopUpDebug(0, MODE_OFF);
    }
}


void
QTscriptDebuggerDlg::edit_menu_slot(QAction *a)
{
    if (a == db_undo) {
        if (!db_undo_list)
            return;
        histlist *h = db_undo_list;
        db_undo_list = db_undo_list->h_next;
        db_in_undo = true;

        if (h->h_deletion)
            wb_textarea->insert_chars_at_point(0, h->h_text, h->h_cpos, -1);
        else
            wb_textarea->delete_chars(h->h_cpos, h->h_cpos + strlen(h->h_text));
        db_in_undo = false;
        h->h_next = db_redo_list;
        db_redo_list = h;
        check_sens();
        return;
    }

    if (a == db_redo) {
        if (!db_redo_list)
            return;
        histlist *h = db_redo_list;
        db_redo_list = db_redo_list->h_next;
        db_in_undo = true;

        if (!h->h_deletion)
            wb_textarea->insert_chars_at_point(0, h->h_text, h->h_cpos, -1);
        else
            wb_textarea->delete_chars(h->h_cpos, h->h_cpos + strlen(h->h_text));
        db_in_undo = false;
        h->h_next = db_undo_list;
        db_undo_list = h;
        check_sens();
    }
    if (a->data().toInt() == 1) {
        wb_textarea->cut();
        return;
    }
    if (a->data().toInt() == 2) {
        wb_textarea->copy();
        return;
    }
    if (a->data().toInt() == 3) {
        wb_textarea->paste();
        return;
    }
    if (a->data().toInt() == 4) {
        // Paste primary selection, X-Windows thing not supported.
    }
}


void
QTscriptDebuggerDlg::exec_menu_slot(QAction *a)
{
    if (a->data().toInt() == RunCode) {
        set_mode(DBrun);
        run();
        return;
    }
    if (a->data().toInt() == StepCode) {
        set_mode(DBrun);
        step();
        return;
    }
    if (a->data().toInt() == StartCode) {
        set_mode(DBrun);
        start();
        return;
    }
    if (a->data().toInt() == MonitorCode) {
        monitor();
    }
}


void
QTscriptDebuggerDlg::options_menu_slot(QAction *a)
{
    if (a->data().toInt() == 1) {
        if (QTdev::GetStatus(a)) {
            if (!db_search_pop) {
                char *initstr = wb_textarea->get_selection();
                db_search_pop = new QTsearchDlg(this, initstr);
                delete [] initstr;
            }
        }
        else if (db_search_pop)
            db_search_pop->popdown();
        return;
    }
    if (a->data().toInt() == 2) {
        if (QTdev::GetStatus(a))
            PopUpFontSel(a, GRloc(), MODE_ON, 0, 0, FNT_FIXED);
        else
            PopUpFontSel(a, GRloc(), MODE_OFF, 0, 0, FNT_FIXED);
    }
}


void
QTscriptDebuggerDlg::help_slot()
{
    DSPmainWbag(PopUpHelp("xic:debug"))
}


void
QTscriptDebuggerDlg::mode_btn_slot()
{
    if (db_mode == DBedit)
        set_mode(DBrun);
    else
        set_mode(DBedit);
}


namespace {
    // Return true if s matches word followed by null or space.
    //
    bool isword(const char *s, const char *word)
    {
        while (*word) {
            if (*s != *word)
                return (false);
            s++;
            word++;
        }
        return (!*s || isspace(*s));
    }


    // Return true if line is in a function definition.
    //
    bool infunc(const char *line)
    {
        const char *t = strchr(line, '\n');
        while (t) {
            while (isspace(*t))
                t++;
            if (isword(t, "function"))
                break;
            if (isword(t, "endfunc"))
                return (true);
            t = strchr(t, '\n');
        }
        return (false);
    }
}


void
QTscriptDebuggerDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }

    if (db_mode == DBedit) {
        ev->ignore();
        return;
    }
    ev->accept();

    int vsv = wb_textarea->verticalScrollBar()->value();
    int hsv = wb_textarea->horizontalScrollBar()->value();

    char *str = wb_textarea->get_chars();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    int posn = wb_textarea->document()->documentLayout()->hitTest(
        QPointF(xx + hsv, yy + vsv), Qt::ExactHit);

    const char *lineptr = str;
    int line = 0;
    for (int i = 0; i <= posn; i++) {
        if (str[i] == '\n') {
            if (i == posn) {
                // Clicked to right of line.
                break;
            }
            line++;
            lineptr = str + i+1;
        }
    }
    if (!lineptr) {
        wb_textarea->select_range(0, 0);
        delete [] str;
        return;
    }

    const char *s = lineptr;
    while (isspace(*s) && *s != '\n')
        s++;
    wb_textarea->select_range(0, 0);

    if (lineptr > str) {
        if (*(lineptr - 2) == '\\') {
            // continuation line, invalid target
            delete [] str;
            return;
        }
    }

    // don't allow line in a function block
    if (infunc(lineptr)) {
        delete [] str;
        return;
    }

    char buf[18];
    strncpy(buf, s, 16);
    s = buf;
    delete [] str;

    if (!*s || *s == '\n' || *s == '#' || (*s == '/' && *(s+1) == '/'))
        // clicked beyond text, or blank/comment line
        return;
    // these lines are never visited
    if (isword(s, "end"))
        return;
    if (isword(s, "else"))
        return;
    if (isword(s, "function"))
        return;
    if (isword(s, "endfunc"))
        return;
    breakpoint(line);
}


void
QTscriptDebuggerDlg::mouse_release_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonRelease) {
        ev->ignore();
        return;
    }

    if (db_mode == DBedit) {
        //wb_textarea->QTextEdit::mouseReleaseEvent(ev);
        ev->ignore();
        return;
    }
    ev->accept();
}


void
QTscriptDebuggerDlg::key_press_slot(QKeyEvent *ev)
{
    // Handle key presses in the debugger window.  This provides
    // additional accelerators for start/run/reset.

    if (db_mode == DBrun) {
        switch (ev->key()) {
        case Qt::Key_Space:
        case Qt::Key_T:
            step();
            ev->ignore();
            break;
        case Qt::Key_R:
            run();
            ev->ignore();
            break;
        case Qt::Key_Backspace:
        case Qt::Key_E:
            start();
            ev->ignore();
            break;
        }
    }
}


void
QTscriptDebuggerDlg::text_changed_slot()
{
    if (db_text_changed)
        return;
    db_text_changed = true;
    db_saveas->setEnabled(true);
    char buf[256];
    QByteArray title_ba = db_title->text().toLatin1();
    const char *fname = title_ba.constData();
    strcpy(buf, DEF_FILE);
    if (fname) {
        while (isspace(*fname))
            fname++;
    }
    if (fname && *fname) {
        strcpy(buf, fname);
        char *f = buf;
        while (*f && !isspace(*f))
            f++;
        *f++ = ' ';
        *f++ = ' ';
        strcpy(f, "(modified)");
    }
    db_title->setText(buf);
    db_modebtn->setEnabled(true);
}


void
QTscriptDebuggerDlg::text_change_slot(int strt, int nch_rm, int nch_add)
{
    if (nch_rm) {
        // chars_deleted
        if (db_in_undo)
            return;
        char *text = wb_textarea->get_chars();
        char *ntext = new char[nch_rm+1];
        strncpy(ntext, text+strt, nch_rm);
        ntext[nch_rm] = 0;
        delete [] text;
        db_undo_list = new histlist(ntext, strt, true, db_undo_list);
        histlist::destroy(db_redo_list);
        db_redo_list = 0;
        check_sens();
    }
    if (nch_add) {
        // chars inserted
        if (db_in_undo)
            return;
        char *text = wb_textarea->get_chars();
        char *ntext = new char[nch_add+1];
        strncpy(ntext, text+strt, nch_add);
        ntext[nch_add] = 0;
        delete [] text;
        db_undo_list = new histlist(ntext, strt, false, db_undo_list);
        histlist::destroy(db_redo_list);
        db_redo_list = 0;
        check_sens();
    }
}


void
QTscriptDebuggerDlg::mime_data_handled_slot(const QMimeData *dta, int *accpt)
const
{
    if (dta->hasFormat("text/twostring") || dta->hasFormat("text/plain"))
        *accpt = 1;
    else
        *accpt = -1;
}


void
QTscriptDebuggerDlg::mime_data_delivered_slot(const QMimeData *dta, int *accpt)
{
    // Receive drop data (a path name).

    *accpt = -1;
    QByteArray data_ba;
    if (dta->hasFormat("text/twostring"))
        data_ba = dta->data("text/twostring");
    else if (dta->hasFormat("text/plain"))
        data_ba = dta->data("text/plain");
    else
        return;
    char *src = lstring::copy(data_ba.constData());
    if (!src)
        return;

    if (db_mode == DBedit) {
        if (QApplication::queryKeyboardModifiers() & Qt::ControlModifier) {
            // If we're pressing Ctrl, insert text at cursor.
            int n = wb_textarea->get_insertion_point();
            wb_textarea->insert_chars_at_point(0, src, strlen(src), n);
            delete [] src;
            return;
        }
    }
    // Drops from content lists may be in the form
    // "fname_or_chd\ncellname".  Keep the filename.
    char *t = strchr(src, '\n');
    if (t)
        *t = 0;

    delete [] db_dropfile;
    db_dropfile = 0;
    set_mode(DBedit);
    if (db_load_pop)
        db_load_pop->update(0, src);
    else {
        db_dropfile = lstring::copy(src);
        if (db_load_pop) {
            db_load_pop->popdown();
            delete [] src;
            return;
        }
        if (check_save(LoadCode))
            return;
        db_load_pop = PopUpEditString(0, GRloc(),
            "Enter script file name: ", db_dropfile, db_open_cb,
                0, 214, 0);
        if (db_load_pop)
            db_load_pop->register_usrptr((void**)&db_load_pop);
    }
    delete [] src;
    *accpt = 1;
}


void
QTscriptDebuggerDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, fnum))
            wb_textarea->setFont(*fnt);
        refresh(false, locPresent, true);
    }
}
// End of QTscriptDebuggerDlg functions.


//
// The variables monitor.
//

QTdbgVarsDlg::QTdbgVarsDlg(void *p, QWidget *prnt) : QDialog(prnt)
{
    dv_pointer = p;

    setWindowTitle(tr("Variables"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // variable listing text
    //
    dv_list = new QTreeWidget();
    vbox->addWidget(dv_list);
    dv_list->setHeaderLabels(QStringList(QList<QString>() <<
        tr("Variable") << tr("value")));
    dv_list->header()->setMinimumSectionSize(25);
    dv_list->header()->resizeSection(0, 50);

    connect(dv_list, &QTreeWidget::currentItemChanged,
        this, &QTdbgVarsDlg::current_item_changed_slot);
    connect(dv_list, &QTreeWidget::itemActivated,
        this, &QTdbgVarsDlg::item_activated_slot);
    connect(dv_list, &QTreeWidget::itemClicked,
        this, &QTdbgVarsDlg::item_clicked_slot);
    connect(dv_list, &QTreeWidget::itemSelectionChanged,
        this, &QTdbgVarsDlg::item_selection_changed);

    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_FIXED))
        dv_list->setFont(*fnt);
    connect(QTfont::self(), &QTfont::fontChanged,
        this, &QTdbgVarsDlg::font_changed_slot, Qt::QueuedConnection);

    // Dismiss button
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    vbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QTdbgVarsDlg::dismiss_btn_slot);
}


QTdbgVarsDlg::~QTdbgVarsDlg()
{
    if (dv_pointer)
        *(void**)dv_pointer = 0;
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTdbgVarsDlg
#include "qtinterf/qtmacos_event.h"
#endif


// Update the variables listing.
//
void
QTdbgVarsDlg::update(stringlist *vars)
{
    if (!vars) {
        popdown();
        return;
    }
    dv_list->clear();

    char buf[256];
    for (stringlist *wl = vars; wl; wl = wl->next) {
        SIparse()->printVar(wl->string, buf);
        QTreeWidgetItem *item = new QTreeWidgetItem(dv_list);
        item->setText(0, wl->string);
        item->setText(1, buf);
    }
}


void
QTdbgVarsDlg::current_item_changed_slot(QTreeWidgetItem *itm, QTreeWidgetItem*)
{
    if (!QTscriptDebuggerDlg::self())
        return;
    QByteArray bavar = itm->text(0).toLatin1();
    QByteArray baval = itm->text(1).toLatin1();
    const char *var = bavar.constData();
    const char *val = baval.constData();

    if (var && val) {
        char buf[128];
        snprintf(buf, sizeof(buf), "assign %s = ", var);

        bool busy;
        const char *in = QTscriptDebuggerDlg::self()->var_prompt(val, buf,
            &busy);
        if (busy)
            return;

        if (!in) {
            PL()->ErasePrompt();
            return;
        }
        char *s = SIparse()->setVar(var, in);
        if (s) {
            PL()->ShowPrompt(s);
            delete [] s;
            return;
        }
        QTscriptDebuggerDlg::self()->update_variables();
        PL()->ErasePrompt();
    }
}


void
QTdbgVarsDlg::item_activated_slot(QTreeWidgetItem*, int)
{
}


void
QTdbgVarsDlg::item_clicked_slot(QTreeWidgetItem*, int)
{
}


void
QTdbgVarsDlg::item_selection_changed()
{
}


void
QTdbgVarsDlg::dismiss_btn_slot()
{
    popdown();
}


void
QTdbgVarsDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_FIXED))
            dv_list->setFont(*fnt);
    }
}

