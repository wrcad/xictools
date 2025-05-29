
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qtedit.h"
#include "qtfile.h"
#include "qtfont.h"
#include "qtinput.h"
#include "qtmsg.h"
#include "qtsearch.h"
#include "qttextw.h"
#include "miscutil/encode.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"

#include <unistd.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QGroupBox>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>
#include <QStatusBar>
#include <QTextBlock>
#include <QTextCursor>

#ifdef Q_OS_MACOS
#define USE_QTOOLBAR
#endif

// If the message would exceed this size, it is split into multiple
// messages
#define MAIL_MAXSIZE 1600000

namespace {
    // defaults
    const char *mail_addr = "bugs@wrcad.com";
    const char *mail_subject = "bug report";

    // for hardcopies
    HCcb edHCcb =
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

    // Envelope xpm used to denote attachments
    // XPM
    const char * const attach_xpm[] = {
    "32 16 4 1",
    " 	s None	c None",
    ".	c black",
    "x	c white",
    "o	c sienna",
    "                                ",
    "   .........................    ",
    "   .x..xxxxxxxxxxxxxxxxx..x.    ",
    "   .xxx..xxxxxxxxxxxxx..xxx.    ",
    "   .xxxxx..xxxxxxxxx..xxxxx.    ",
    "   .xooxoxx..xxxxx..xxxxxxx.    ",
    "   .xxoxxxxxx.....xxxxxxxxx.    ",
    "   .xxxxxxxxxxxxxxxxxxxxxxx.    ",
    "   .xxxxxxxxxxxxxxxxxxxxxxx.    ",
    "   .xxxxxxxoxxxxxxxoxxxxxxx.    ",
    "   .xxxxxxxxoxxoxooxxxxxxxx.    ",
    "   .xxxxxxxoxxoxoxxoxxxxxxx.    ",
    "   .xxxxxxxxxxxxxxxxxxxxxxx.    ",
    "   .xxxxxxxxxxxxxxxxxxxxxxx.    ",
    "   .........................    ",
    "                                "};
}

QTeditDlg::QTeditDlg(QTbag *owner, QTeditDlg::EditorType type,
    const char *file_or_string, bool with_source, void *arg) :
    QDialog(owner ? owner->Shell() : 0), QTbag(this)
{
    p_parent = owner;
    p_cb_arg = arg;

    ed_editor_type = type;
    ed_last_event = QUIT;
    ed_saved_as = 0;
    ed_source_file = 0;
    ed_drop_file = 0;
    ed_last_search = 0;
    ed_text_changed = false;
    ed_have_source = with_source;
    ed_ign_case = false;
    ed_ign_change = false;
    ed_len = 0;
    ed_chksum = 0;

    ed_bar = 0;
    ed_searcher = 0;
    ed_filemenu = 0;
    ed_editmenu = 0;
    ed_optmenu = 0;
    ed_helpmenu = 0;
    ed_title = 0;
    ed_to_entry = 0;
    ed_subj_entry = 0;
    ed_text_editor = 0;
    ed_status_bar = 0;

    ed_File_Load = 0;
    ed_File_Read = 0;
    ed_File_Save = 0;
    ed_File_SaveAs = 0;
#ifdef WIN32
    ed_File_CRLF = 0;
#endif
    ed_Edit_Undo = 0;
    ed_Edit_Redo = 0;
    ed_Options_Attach = 0;
    ed_HelpMenu = 0;

    if (owner)
        owner->MonitorAdd(this);
    setAttribute(Qt::WA_DeleteOnClose);

    if (ed_editor_type == Editor)
        setWindowTitle(tr("Text Editor"));
    else if (ed_editor_type == Browser) {
        setWindowTitle(tr("File Browser"));
        ed_have_source = false;
    }
    else if (ed_editor_type == StringEditor) {
        setWindowTitle(tr("Text Editor"));
        ed_have_source = false;
    }
    else if (ed_editor_type == Mailer) {
        setWindowTitle(tr("Email Message Editor"));
        ed_have_source = false;
    }

    // Menu bar.
    //
#ifdef USE_QTOOLBAR
    QToolBar *menubar = new QToolBar();
    menubar->setMaximumHeight(22);
#else
    QMenuBar *menubar = new QMenuBar();
#endif
    ed_bar = menubar;

    // File menu.
    ed_filemenu = new QMenu(this);
    ed_filemenu->setTitle(tr("&File"));
    if (ed_editor_type == Editor || ed_editor_type == Browser) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        ed_filemenu->addAction(tr("&Open"), Qt::CTRL|Qt::Key_O,
            this, &QTeditDlg::open_slot);
        ed_File_Load = ed_filemenu->addAction(tr("&Load"),
            Qt::CTRL|Qt::Key_L, this, &QTeditDlg::load_slot);
#else
        ed_filemenu->addAction(tr("&Open"), this, &QTeditDlg::open_slot,
            Qt::CTRL|Qt::Key_O);
        ed_File_Load = ed_filemenu->addAction(tr("&Load"), this,
            &QTeditDlg::load_slot, Qt::CTRL|Qt::Key_L);
#endif
    }
    if (ed_editor_type != Browser) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        ed_File_Read = ed_filemenu->addAction(tr("&Read"),
            Qt::CTRL|Qt::Key_R, this, &QTeditDlg::read_slot);
#else
        ed_File_Read = ed_filemenu->addAction(tr("&Read"), this,
            &QTeditDlg::read_slot, Qt::CTRL|Qt::Key_R);
#endif
    }
    if (ed_editor_type == Editor || ed_editor_type == StringEditor) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        ed_File_Save = ed_filemenu->addAction(tr("&Save"),
            Qt::CTRL|Qt::Key_S, this, &QTeditDlg::save_slot);
#else
        ed_File_Save = ed_filemenu->addAction(tr("&Save"), this,
            &QTeditDlg::save_slot, Qt::CTRL|Qt::Key_S);
#endif
    }
    if (ed_editor_type != StringEditor) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        ed_File_SaveAs = ed_filemenu->addAction(tr("Save &As"),
            Qt::CTRL|Qt::Key_A, this, &QTeditDlg::save_as_slot);
        ed_filemenu->addAction(tr("&Print"), Qt::CTRL|Qt::Key_P,
            this, &QTeditDlg::print_slot);
#else
        ed_File_SaveAs = ed_filemenu->addAction(tr("Save &As"), this,
            &QTeditDlg::save_as_slot, Qt::CTRL|Qt::Key_A);
        ed_filemenu->addAction(tr("&Print"), this,
            &QTeditDlg::print_slot, Qt::CTRL|Qt::Key_P);
#endif
    }
#ifdef WIN32
    if (ed_editor_type == Editor) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        ed_File_CRLF = ed_filemenu->addAction(tr("&Write CRLF"), 0,
            this, &QTeditDlg::write_crlf_slot);
#else
        ed_File_CRLF = ed_filemenu->addAction(tr("&Write CRLF"), this,
            &QTeditDlg::write_crlf_slot, 0);
#endif
        ed_File_CRLF->setCheckable(true);
        ed_File_CRLF->setChecked(QTdev::self()->GetCRLFtermination());
    }
#endif
    ed_filemenu->addSeparator();
    if (ed_editor_type == Mailer)  {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        ed_filemenu->addAction(tr("Send &Mail"), Qt::CTRL|Qt::Key_M,
            this, &QTeditDlg::send_slot);
#else
        ed_filemenu->addAction(tr("Send &Mail"), this,
            &QTeditDlg::send_slot, Qt::CTRL|Qt::Key_M);
#endif
    }
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    ed_filemenu->addAction(tr("&Quit"), Qt::CTRL|Qt::Key_Q,
        this, &QTeditDlg::quit_slot);
#else
    ed_filemenu->addAction(tr("&Quit"), this, &QTeditDlg::quit_slot,
        Qt::CTRL|Qt::Key_Q);
#endif

#ifdef USE_QTOOLBAR
    QAction *a = menubar->addAction("File");
    a->setMenu(ed_filemenu);
    QToolButton *tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    menubar->addMenu(ed_filemenu);
#endif

    // Edit menu.
    ed_editmenu = new QMenu(this);
    ed_editmenu->setTitle(tr("&Edit"));
    if (ed_editor_type != Browser) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        ed_Edit_Undo = ed_editmenu->addAction(tr("Undo"), Qt::CTRL|Qt::Key_U,
            this, &QTeditDlg::undo_slot);
        ed_Edit_Redo = ed_editmenu->addAction(tr("Redo"), Qt::CTRL|Qt::Key_R,
            this, &QTeditDlg::redo_slot);
        ed_editmenu->addAction(tr("&Cut to Clipboard`"), Qt::CTRL|Qt::Key_X,
            this, &QTeditDlg::cut_slot);
#else
        ed_Edit_Undo = ed_editmenu->addAction(tr("Undo"), this,
            &QTeditDlg::undo_slot, Qt::CTRL|Qt::Key_U);
        ed_Edit_Redo = ed_editmenu->addAction(tr("Redo"), this,
            &QTeditDlg::redo_slot, Qt::CTRL|Qt::Key_R);
        ed_editmenu->addAction(tr("&Cut to Clipboard`"), this,
            &QTeditDlg::cut_slot, Qt::CTRL|Qt::Key_X);
#endif
        ed_Edit_Undo->setEnabled(false);
        ed_Edit_Redo->setEnabled(false);
    }

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    ed_editmenu->addAction(tr("&Copy to Clipboard"), Qt::CTRL|Qt::Key_C,
        this, &QTeditDlg::copy_slot);
    if (ed_editor_type != Browser) {
        ed_editmenu->addAction(tr("&Paste from Clipboard"),
            Qt::CTRL|Qt::Key_V, this, &QTeditDlg::paste_slot);
    }
#else
    ed_editmenu->addAction(tr("&Copy to Clipboard"), this,
        &QTeditDlg::copy_slot, Qt::CTRL|Qt::Key_C);
    if (ed_editor_type != Browser) {
        ed_editmenu->addAction(tr("&Paste from Clipboard"), this,
            &QTeditDlg::paste_slot, Qt::CTRL|Qt::Key_V);
    }
#endif

#ifdef USE_QTOOLBAR
    a = menubar->addAction("Edit");
    a->setMenu(ed_editmenu);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    menubar->addMenu(ed_editmenu);
#endif

    // Options menu.
    ed_optmenu = new QMenu(this);
    ed_optmenu->setTitle(tr("&Options"));
    ed_optmenu->addAction(tr("&Search"), this,
        &QTeditDlg::search_slot);
    if ((ed_editor_type == Editor || ed_editor_type == StringEditor) &&
            ed_have_source) {
        ed_optmenu->addAction(tr("&Source"), this,
            &QTeditDlg::source_slot);
    }
    if (ed_editor_type == Mailer) {
        ed_Options_Attach = ed_optmenu->addAction(tr("&Attach"),
            this, &QTeditDlg::attach_slot);
    }
    ed_optmenu->addAction(tr("&Font"), this,
        &QTeditDlg::font_slot);
#ifdef USE_QTOOLBAR
    a = menubar->addAction("Options");
    a->setMenu(ed_optmenu);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    menubar->addMenu(ed_optmenu);
#endif

    menubar->addSeparator();
#ifdef USE_QTOOLBAR
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    menubar->addAction(tr("&Help"), Qt::CTRL|Qt::Key_H, this,
        &QTeditDlg::help_slot);
#else
    a = menubar->addAction(tr("&Help"), this, &QTeditDlg::help_slot);
    a->setShortcut(QKeySequence("Ctrl+H"));
#endif
#else
    ed_helpmenu = new QMenu(this);
    ed_helpmenu->setTitle(tr("&Help"));
    ed_helpmenu->addAction(tr("&Help"), this, &QTeditDlg::help_slot);
    ed_HelpMenu = menubar->addMenu(ed_helpmenu);
#endif

    QMargins qmtop(2, 2, 2, 2);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMenuBar(menubar);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    if (ed_editor_type == Mailer) {
        QGroupBox *gb = new QGroupBox(tr("To:"));
        QVBoxLayout *vb = new QVBoxLayout(gb);
        vb->setContentsMargins(qmtop);
        ed_to_entry = new QLineEdit();
        vb->addWidget(ed_to_entry);
        vbox->addWidget(gb);
        ed_to_entry->setText(mail_addr);

        gb = new QGroupBox(tr("Subject:"));
        vb = new QVBoxLayout(gb);
        vb->setContentsMargins(qmtop);
        ed_subj_entry = new QLineEdit();
        vb->addWidget(ed_subj_entry);
        vbox->addWidget(gb);
        ed_subj_entry->setText(mail_subject);
    }

    ed_title = new QGroupBox();
    vbox->addWidget(ed_title);
    QVBoxLayout *vb = new QVBoxLayout(ed_title);
    vb->setContentsMargins(qmtop);
    ed_text_editor = new QTtextEdit();
    vb->addWidget(ed_text_editor);
    ed_text_editor->setLineWrapMode(QTextEdit::NoWrap);
    if (ed_editor_type == Browser)
        ed_text_editor->setUndoRedoEnabled(false);
    else {
        connect(ed_text_editor, &QTtextEdit::undoAvailable,
            this, &QTeditDlg::undo_available_slot);
        connect(ed_text_editor, &QTtextEdit::redoAvailable,
            this, &QTeditDlg::redo_available_slot);
    }

    ed_status_bar = new QStatusBar();
    vbox->addWidget(ed_status_bar);

    static bool checked_env;
    if (!checked_env) {
        checked_env = true;
        const char *fn = getenv("XEDITOR_FONT");
        if (fn)
            Fnt()->setName(fn, FNT_EDITOR);
    }
    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_EDITOR))
        ed_text_editor->setFont(*fnt);
    connect(QTfont::self(), &QTfont::fontChanged,
        this, &QTeditDlg::font_changed_slot, Qt::QueuedConnection);

    if (ed_editor_type == Editor || ed_editor_type == Browser) {
        if (ed_editor_type == Browser) {
            ed_text_editor->set_editable(false);
            ed_text_editor->viewport()->setCursor(Qt::ArrowCursor);
        }
//XXX
        else
            ed_text_editor->set_editable(true);

        char *fname = pathlist::expand_path(file_or_string, false, true);
        check_t which = filestat::check_file(fname, R_OK);
        if (which == NOGO) {
            fname = filestat::make_temp("xe");
            if (ed_editor_type == Browser)
                ed_status_bar->showMessage("Can't open file!");
        }
        else if (which == READ_OK) {
            if (!read_file(fname, true))
                ed_status_bar->showMessage("Can't open file!");
            if (ed_editor_type == Browser)
                ed_status_bar->showMessage("Read-Only");
        }
        else if (which == NO_EXIST) {
            if (ed_editor_type == Browser)
                ed_status_bar->showMessage("Can't open file!");
        }
        else {
            ed_status_bar->showMessage("Read-Only");
            ed_text_editor->set_editable(false);
        }
        set_source(fname);
        delete [] fname;
    }
    else if (ed_editor_type == Mailer) {
        ed_text_editor->setPlainText(file_or_string);
        ed_status_bar->showMessage("Please enter your comment");
    }
    else
        ed_text_editor->setPlainText(file_or_string);

    ed_text_changed = false;
    connect(ed_text_editor, &QTtextEdit::textChanged,
        this, &QTeditDlg::text_changed_slot);

    ed_saved_as = 0;
    ed_last_event = LOAD;
}


QTeditDlg::~QTeditDlg()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }

    const char *fnamein = 0;
    if (ed_editor_type != Mailer && ed_editor_type != StringEditor) {
        if (ed_saved_as)
            fnamein = lstring::copy(ed_saved_as);
        else
            fnamein = lstring::copy(ed_source_file);
    }
    if (wb_fontsel)
        wb_fontsel->popdown();

    delete ed_searcher;
    if (ed_editor_type != Mailer) {
        if (p_callback)
            (*p_callback)(fnamein, p_cb_arg, XE_QUIT);
        delete [] fnamein;
    }
    else if (p_cancel)
        (*p_cancel)(this);
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        QTdev::Deselect(p_caller);

    delete [] ed_saved_as;
    delete [] ed_source_file;
    delete [] ed_drop_file;
    delete [] ed_last_search;
}


// GRpopup override
//
void
QTeditDlg::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// Register the initiating button or action, which are assumed to be
// checkable.  The widget will pop down when this button or action is
// unchecked.  The button or action will be unchecked when the widget
// is destroyed.
//
void
QTeditDlg::set_caller(GRobject obj)
{
    p_caller = obj;
    if (obj) {
        QObject *o = (QObject*)obj;
        if (o->isWidgetType()) {
            QAbstractButton *btn = dynamic_cast<QAbstractButton*>(o);
            if (btn) {
                connect(btn, &QAbstractButton::clicked,
                    this, &QTeditDlg::quit_slot);
                return;
            }
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a) {
                connect(a, &QAction::triggered, this, &QTeditDlg::quit_slot);
                return;
            }
        }
    }
}


void
QTeditDlg::set_mailaddr(const char *str)
{
    if (ed_to_entry)
        ed_to_entry->setText(str);
}


void
QTeditDlg::set_mailsubj(const char *str)
{
    if (ed_subj_entry)
        ed_subj_entry->setText(str);
}


// Handler for the file selection widget.
//
void
QTeditDlg::file_selection_slot(const char *fname, void*)
{
    delete [] ed_drop_file;
    ed_drop_file = lstring::copy(fname);
    if (wb_input)
        wb_input->popdown();
    ed_File_Load->activate(QAction::Trigger);
}


// Pop up the file selector.
//
void
QTeditDlg::open_slot()
{
    // open the file selector in the directory of the current file
    char *path = lstring::copy(ed_source_file);
    if (path && *path) {
        char *t = strrchr(path, '/');
        if (t)
            *t = 0;
        else {
            delete [] path;
            path = getcwd(0, 256);
        }
    }
    GRfilePopup *fs = PopUpFileSelector(fsSEL, GRloc(), 0, 0, 0, path);
    QTfileDlg *fsel = dynamic_cast<QTfileDlg*>(fs);
    if (fsel) {
        connect(fsel, &QTfileDlg::file_selected,
            this, &QTeditDlg::file_selection_slot);
    }

    delete [] path;
}


// Connected to PopUpInput() to actually read in the new file
// to edit.
//
void
QTeditDlg::load_file_slot(const char *fnamein, void*)
{
    char *fname = pathlist::expand_path(fnamein, false, true);
    if (!fname)
        return;
    check_t which = filestat::check_file(fname, R_OK);
    if (which == NOGO) {
        PopUpMessage(filestat::error_msg(), true);
        delete [] fname;
        return;
    }
    ed_text_editor->set_editable(false);

    bool ok = read_file(fname, true);
    if (ed_editor_type != Browser)
        ed_text_editor->set_editable(true);
    if (wb_input)
        wb_input->popdown();
    if (!ok) {
        char tbuf[256];
        if (strlen(fname) > 64)
            strcpy(fname + 60, "...");
        snprintf(tbuf, sizeof(tbuf), "Can't open %s!", fname);
        ed_status_bar->showMessage(tbuf);
        delete [] fname;
        return;
    }
    set_source(fname);
    if (ed_text_changed) {
        ed_text_changed = false;
        if (ed_File_Save)
            ed_File_Save->setEnabled(false);
    }
    if (ed_saved_as) {
        delete [] ed_saved_as;
        ed_saved_as = 0;
    }
    delete [] fname;
}


// Callback to load a new file into the editor.
//
void
QTeditDlg::load_slot()
{
    if (text_changed()) {
        if (ed_last_event != LOAD) {
            ed_last_event = LOAD;
            PopUpMessage(
                "Text has been modified.  Press Load again to load", false);
            return;
        }
        if (wb_message)
            wb_message->popdown();
    }
    char buf[512];
    *buf = '\0';
    if (ed_drop_file) {
        strcpy(buf, ed_drop_file);
        delete [] ed_drop_file;
        ed_drop_file = 0;
    }
    else if (p_callback)
        (*p_callback)(buf, p_cb_arg, XE_LOAD);
    if (wb_input)
        wb_input->popdown();
    PopUpInput(0, buf, "Load File", 0, 0);
    connect(wb_input, &QTledDlg::action_call,
        this, &QTeditDlg::load_file_slot);
}


void
QTeditDlg::closeEvent(QCloseEvent *ev)
{
    ev->ignore();
    quit_slot();
}


// Passed to PopUpInput() to read in the file starting at the current
// cursor position.
//
void
QTeditDlg::read_file_slot(const char *fnamein, void*)
{
    char *fname = pathlist::expand_path(fnamein, false, true);
    if (!fname)
        return;
    check_t which = filestat::check_file(fname, R_OK);
    if (which == NOGO) {
        PopUpMessage(filestat::error_msg(), true);
        delete [] fname;
        return;
    }
    ed_text_editor->set_editable(false);

    char tbuf[256];
    if (!read_file(fname, false)) {
        if (strlen(fname) > 64)
            strcpy(fname + 60, "...");
        snprintf(tbuf, sizeof(tbuf), "Can't open %s!", fname);
        ed_status_bar->showMessage(tbuf);
    }
    else {
        text_changed_slot();  // this isn't called otherwise
        ed_text_editor->set_editable(true);
        if (strlen(fname) > 64)
            strcpy(fname + 60, "...");
        snprintf(tbuf, sizeof(tbuf), "Successfully read %s", fname);
        ed_status_bar->showMessage(tbuf);
    }
    if (wb_input)
        wb_input->popdown();
    delete [] fname;
}


// Read a file into the editor at the current position.
//
void
QTeditDlg::read_slot()
{
    char buf[512];
    *buf = '\0';
    if (ed_drop_file) {
        strcpy(buf, ed_drop_file);
        delete [] ed_drop_file;
        ed_drop_file = 0;
    }
    PopUpInput(0, buf, "Read File", 0, 0);
    connect(wb_input, &QTledDlg::action_call,
        this, &QTeditDlg::read_file_slot);
}


// Save file.
//
void
QTeditDlg::save_slot()
{
    ed_last_event = SAVE;

    if (ed_editor_type == StringEditor) {
        if (p_callback) {
            char *st = lstring::copy(
                ed_text_editor->toPlainText().toLatin1().constData());
            bool ret = (*p_callback)(st, p_cb_arg, XE_SAVE);
            delete [] st;
            if (ret)
                delete this;
        }
        return;
    }
    char *fname = ed_source_file;
    if (filestat::check_file(fname, W_OK) == NOGO) {
        PopUpMessage(filestat::error_msg(), true);
        return;
    }
    if (!write_file(fname, 0 , -1)) {
        PopUpMessage("Error: can't write file", true);
        return;
    }
    QByteArray ba = ed_text_editor->toPlainText().toLatin1();
    if (ed_editor_type == Editor) {
        ed_len = ba.size();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        ed_chksum = qChecksum(QByteArrayView(ba));
#else
        ed_chksum = qChecksum(ba.constData(), ed_len);
#endif
    }

    ed_status_bar->showMessage(tr("Text saved"));
    // This can be called with ed_text_changed false if we saved
    // under a new name, and made no subsequent changes.
    if (ed_text_changed)
        ed_text_changed = false;
    ed_File_Save->setEnabled(false);
    if (ed_saved_as) {
        delete [] ed_saved_as;
        ed_saved_as = 0;
    }
    if (p_callback)
        (*p_callback)(fname, p_cb_arg, XE_SAVE);
}


// See if the two names are the same, discounting white space
//
static bool
same(const char *s, const char *t)
{
    while (isspace(*s)) s++;
    while (isspace(*t)) t++;
    for (; *s && *t; s++, t++)
        if (*s != *t) return (false);
    if (*s && !isspace(*s)) return (false);
    if (*t && !isspace(*t)) return (false);
    return (true);
}


// Callback passed to PopUpInput() to actually save the file.
// If a block is selected, only the block is saved in the new file
//
void
QTeditDlg::save_file_as_slot(const char *fnamein, void*)
{
    char *fname = pathlist::expand_path(fnamein, false, true);
    if (!fname)
        return;
    if (filestat::check_file(fname, W_OK) == NOGO) {
        PopUpMessage(filestat::error_msg(), true);
        delete [] fname;
        return;
    }
    QTextCursor c = ed_text_editor->textCursor();
    int start = c.anchor();
    int end = c.position();
    const char *mesg;
    if (start == end) {
        const char *oldfname = ed_source_file;
        // no selected text
        if (same(fname, oldfname)) {
            if (!ed_text_changed) {
                ed_status_bar->showMessage("No save needed");
                delete [] fname;
                return;
            }
            if (!write_file(fname, 0 , -1)) {
                PopUpMessage("Write error, text not saved", true);
                delete [] fname;
                return;
            }
            if (ed_saved_as) {
                delete [] ed_saved_as;
                ed_saved_as = 0;
            }
            ed_text_changed = false;
            if (ed_File_Save)
                ed_File_Save->setEnabled(false);
        }
        else {
            if (!write_file(fname, 0 , -1)) {
                PopUpMessage("Write error, text not saved", true);
                delete [] fname;
                return;
            }
            if (ed_saved_as)
                delete [] ed_saved_as;
            ed_saved_as = lstring::copy(fname);
            ed_text_changed = false;
        }
        mesg = "Text saved";
    }
    else {
        if (!write_file(fname, start, end)) {
            PopUpMessage("Unknown error, block not saved", true);
            delete [] fname;
            return;
        }
        mesg = "Block saved";
    }
    if (wb_input)
        wb_input->popdown();
    ed_status_bar->showMessage(mesg);
    delete [] fname;
}


// Save file under a new name.
//
void
QTeditDlg::save_as_slot()
{
    ed_last_event = SAVEAS;
    if (wb_input)
        wb_input->popdown();

    QTextCursor c = ed_text_editor->textCursor();
    int start = c.anchor();
    int end = c.position();
    if (start == end) {
        const char *fname = ed_source_file;
        if (ed_editor_type == Mailer)
            fname = "";
        PopUpInput(0, fname, "Save File", 0, 0);
        connect(wb_input, &QTledDlg::action_call,
            this, &QTeditDlg::save_file_as_slot);
    }
    else {
        PopUpInput(0, "", "Save Block", 0, 0);
        connect(wb_input, &QTledDlg::action_call,
            this, &QTeditDlg::save_file_as_slot);
    }
}


#ifdef WIN32
void
QTeditDlg::write_crlf_slot(bool state)
{
    QTdev::self()->SetCRLFtermination(state);
}
#endif


// Bring up the printer control pop-up.
//
void
QTeditDlg::print_slot()
{
    if (edHCcb.command)
        delete [] edHCcb.command;
    edHCcb.command = lstring::copy(GRappIf()->GetPrintCmd());
    PopUpPrint(this, &edHCcb, HCtext);
}


void
QTeditDlg::send_slot()
{
    char *descname = filestat::make_temp("m1");
    FILE *descfp = fopen(descname, "w");
    if (!descfp) {
        PopUpMessage("Error: can't open temporary file.", true);
        delete [] descname;
        return;
    }
    char *s = lstring::copy(ed_text_editor->toPlainText().toLatin1().constData());
    fputs(s, descfp);
    delete [] s;
    fclose(descfp);

    MimeState state;
    state.outfname = filestat::make_temp("m2");
    char buf[256];

    char *mailaddr = lstring::copy(ed_to_entry->text().toLatin1().constData());
    char *header = new char[strlen(mailaddr) + 8];
    strcpy(header, "To: ");
    strcat(header, mailaddr);
    strcat(header, "\n");

    char *subject = lstring::copy(ed_subj_entry->text().toLatin1().constData());
    bool err = false;
    QList<QAction*> acts = ed_bar->actions();
    for (int i = 0; i < acts.size(); i++) {
        QString qs = acts.at(i)->text();
        if (qs.isNull() || qs.isEmpty())
            continue;
        QAction *a = acts.at(i);
        char *fname = lstring::copy(a->data().toString().toLatin1().constData());
        if (fname && *fname) {
            FILE *ifp = fopen(fname, "r");
            if (ifp) {
                descfp = state.nfiles ? 0 : fopen(descname, "r");
                if (encode(&state, ifp, fname, descfp, subject,
                        header, MAIL_MAXSIZE, 0)) {
                    PopUpMessage("Error: can't open temporary file.",
                        true);
                    err = true;
                }
                if (descfp)
                    fclose(descfp);
                fclose (ifp);
            }
            else {
                snprintf(buf, sizeof(buf),
                    "Error: can't open attachment file %s.", fname);
                PopUpMessage(buf, true);
                err = true;
            }
        }
        delete [] fname;
        if (err)
            break;
    }
    if (state.outfile)
        fclose(state.outfile);
    delete [] header;

//XXX Windows support needed

    if (!err) {
        if (!state.nfiles) {
            snprintf(buf, sizeof(buf), "mail -s \"%s\" %s < %s", subject,
                mailaddr, descname);
            int rt = system(buf);
            if (rt) {
                snprintf(buf, sizeof(buf),
                    "Warning: operation returned error status %d.\n", rt);
                PopUpMessage(buf, false);
            }
        }
        else {
            for (int i = 0; i < state.nfiles; i++) {
                // What is "-oi"?  took this from mpack
//XXX fixme, sendmail may not exist
                snprintf(buf, sizeof(buf), "sendmail -oi %s < %s", mailaddr,
                    state.fnames[i]);
                int rt = system(buf);
                if (rt) {
                    fprintf(stderr, 
                        "Warning: operation returned error status %d.\n", rt);
                }
            }
        }
    }

    unlink(descname);
    delete [] descname;
    delete [] subject;
    delete [] mailaddr;

    for (int i = 0; i < state.nfiles; i++) {
        unlink(state.fnames[i]);
        delete [] state.fnames[i];
    }
    delete [] state.fnames;
    if (!err) {
        PopUpMessage("Message sent, thank you.", false);
        sleep(1);
        popdown();
    }
}


// Quit the editor, after confirmation if unsaved work.
//
void
QTeditDlg::quit_slot()
{
    if ((ed_editor_type == Editor || ed_editor_type == StringEditor) &&
            text_changed()) {
        if (ed_last_event != QUIT) {
            ed_last_event = QUIT;
            PopUpMessage(
                "Text has been modified.  Press Quit again to quit", false);
            return;
        }
    }
    // Old QT5 will crash here without delayed delete.
    deleteLater();
}


void
QTeditDlg::undo_slot()
{
    ed_text_editor->undo();
}


void
QTeditDlg::undo_available_slot(bool avail)
{
    ed_Edit_Undo->setEnabled(avail);
}


void
QTeditDlg::redo_slot()
{
    ed_text_editor->redo();
}


void
QTeditDlg::redo_available_slot(bool avail)
{
    ed_Edit_Redo->setEnabled(avail);
}


// Kill selected text, copy to clipboard.
//
void
QTeditDlg::cut_slot()
{
    QTextCursor c = ed_text_editor->textCursor();
    if (c.hasSelection()) {
        QClipboard *cb = qApp->clipboard();
        if (cb->supportsSelection())
            cb->setText(c.selectedText(), QClipboard::Selection);
        else
            cb->setText(c.selectedText(), QClipboard::Clipboard);
        c.removeSelectedText();
    }
}


// Copy selected text to the clipboard.
//
void
QTeditDlg::copy_slot()
{
    QTextCursor c = ed_text_editor->textCursor();
    if (c.hasSelection()) {
        QClipboard *cb = qApp->clipboard();
        if (cb->supportsSelection())
            cb->setText(c.selectedText(), QClipboard::Selection);
        else
            cb->setText(c.selectedText(), QClipboard::Clipboard);
    }
}


// Insert clipboard text.
//
void
QTeditDlg::paste_slot()
{
    QClipboard *cb = qApp->clipboard();
    QTextCursor c = ed_text_editor->textCursor();
    if (cb->supportsSelection())
        c.insertText(cb->text(QClipboard::Selection));
    else
        c.insertText(cb->text(QClipboard::Clipboard));
}


// Pop up the regular expression search dialog.
//
void
QTeditDlg::search_slot()
{
    if (!ed_searcher) {
        ed_searcher = new QTsearchDlg(this, ed_last_search);
        ed_searcher->register_usrptr((void**)&ed_searcher);
        ed_searcher->set_visible(true);

        ed_searcher->set_ign_case(ed_ign_case);
        connect(ed_searcher, &QTsearchDlg::search_down,
            this, &QTeditDlg::search_down_slot);
        connect(ed_searcher, &QTsearchDlg::search_up,
            this, &QTeditDlg::search_up_slot);
        connect(ed_searcher, &QTsearchDlg::ignore_case,
            this, &QTeditDlg::ignore_case_slot);
    }
}


// Send the file to the application to be used as input.
//
void
QTeditDlg::source_slot()
{
    const char *fname;
    ed_last_event = SOURCE;
    if (text_changed()) {
        fname = filestat::make_temp("sp");
        if (!write_file(fname, 0 , -1)) {
            PopUpMessage("Error: can't write temp file", true);
            delete [] fname;
            return;
        }
    }
    else {
        if (ed_saved_as)
            fname = lstring::copy(ed_saved_as);
        else
            fname = lstring::copy(ed_source_file);
    }
    if (p_callback) {
        (*p_callback)(fname, p_cb_arg, XE_SOURCE);
        ed_status_bar->showMessage(tr("Text sourced"));
    }
    delete [] fname;
}


// Attachments are marked by an envelope icon which appears in the menu
// bar.  Pressing the icon drops down a menu containing the attached
// file name as a label, and an "unattach" button.  Pressing unattach
// removes the menu bar icon.

// Callback from the input popup to attach a file to a mail message
//
void
QTeditDlg::attach_file_slot(const char *fnamein, void*)
{
    char *fname = pathlist::expand_path(fnamein, false, true);
    if (!fname)
        return;
    check_t which = filestat::check_file(fname, R_OK);
    if (which == NOGO) {
        PopUpMessage(filestat::error_msg(), true);
        delete [] fname;
        return;
    }
    if (which != READ_OK) {
        char tbuf[256];
        if (strlen(fname) > 64)
            strcpy(fname + 60, "...");
        snprintf(tbuf, sizeof(tbuf), "Can't open %s!", fname);
        ed_status_bar->showMessage(tr(tbuf));
        delete [] fname;
        return;
    }

    QAction *a = new QAction(this);
    a->setData(QVariant(QString(fname)));
    QMenu *menu = new QMenu(this);
    a->setMenu(menu);
    a->setIcon(QIcon(QPixmap(attach_xpm)));
    a->setText("    ");  // need to set text or nothing will show!
    ed_bar->insertAction(ed_HelpMenu, a);
    QAction *a_path = a;

    char *fn = lstring::copy(lstring::strip_path(fname));
    if (strlen(fn) > 23)
        strcpy(fn + 20, "...");
    a = menu->addAction(fn);
    a->setEnabled(false);
    delete [] fn;
    a = new QAction(this);
    a->setText("Unattach");
    a->setData(QVariant((unsigned long long)(uintptr_t)a_path));
    menu->addAction(a);
    connect(menu, &QMenu::triggered, this, &QTeditDlg::unattach_slot);

    delete [] fname;
    if (wb_input)
        wb_input->popdown();
}


// Attach a file in mail mode.
//
void
QTeditDlg::attach_slot()
{
    char buf[512];
    *buf = '\0';
    if (ed_drop_file) {
        strcpy(buf, ed_drop_file);
        delete [] ed_drop_file;
        ed_drop_file = 0;
    }
    PopUpInput(0, buf, "Attach File", 0, 0);
    connect(wb_input, &QTledDlg::action_call,
        this, &QTeditDlg::attach_file_slot);
}


// Unattach a file.
//
void
QTeditDlg::unattach_slot(QAction *a)
{
    if (a) {
        QAction *a_path = (QAction*)(uintptr_t)a->data().toULongLong();
        delete a_path;
    }
}


// Font selector pop-up.
//
void
QTeditDlg::font_slot()
{
    PopUpFontSel(0, GRloc(), MODE_ON, 0, 0, FNT_EDITOR);
}


// Pop up a help window using the application database.
//
void
QTeditDlg::help_slot()
{
    if (GRpkg::self()->MainWbag())
        GRpkg::self()->MainWbag()->PopUpHelp(
            ed_editor_type == Mailer ? "mailclient" : "xeditor");
    else
        PopUpHelp(ed_editor_type == Mailer ? "mailclient" : "xeditor");
}


// Update state to reflect text modified.
//
void
QTeditDlg::text_changed_slot()
{
    if (ed_ign_change)
        return;
    ed_last_event = TEXTMOD;
    ed_status_bar->showMessage("");
    if (wb_message)
        wb_message->popdown();
    if (ed_text_changed)
        return;
    ed_text_changed = true;
    if (ed_File_Save)
        ed_File_Save->setEnabled(true);
}


// Function to read a file into the editor
//
bool
QTeditDlg::read_file(const char *fname, bool clear)
{
    FILE *fp = fopen(fname, "r");
    if (fp) {
        if (clear)
            ed_text_editor->clear();
        char buf[1024];
        QByteArray tba;
        for (;;) {
            int n = fread(buf, 1, 1024, fp);
            if (n > 0)
                tba.append(buf, n);
            if (n < 1024)
                break;
        }
        ed_text_editor->append(tba);
        tba.clear();
        fclose(fp);
        QTextCursor c = ed_text_editor->textCursor();
        c.setPosition(0);
        ed_text_editor->setTextCursor(c);
        if (clear) {
            ed_text_editor->document()->clearUndoRedoStacks();
            QByteArray ba = ed_text_editor->toPlainText().toLatin1();
            if (ed_editor_type == Editor) {
                ed_len = ba.size();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
                ed_chksum = qChecksum(QByteArrayView(ba));
#else
                ed_chksum = qChecksum(ba.constData(), ed_len);
#endif
            }
        }
        return (true);
    }
    return (false);
}


#define WRT_BLOCK 2048

// Function to write a file from the window contents
//
bool
QTeditDlg::write_file(const char *fname, int startpos, int endpos)
{
    FILE *fp = fopen(fname, "w");
    if (fp) {
        int length = ed_text_editor->get_length();
        if (endpos >= 0 && endpos < length)
            length = endpos;
#ifdef WIN32
        char lastc = 0;
#endif
        int start = startpos;
        for (;;) {
            int end = start + WRT_BLOCK;
            if (end > length)
                end = length;
            if (end == start)
                break;
            char *s = ed_text_editor->get_chars(start, end);
#ifdef WIN32
            for (int i = 0; i < (end - start); i++) {
                if (!QTdev::self()->GetCRLFtermination()) {
                    if (s[i] == '\r' && s[i+1] == '\n') {
                        lastc = s[i];
                        continue;
                    }
                }
                else if (s[i] == '\n' && lastc != '\r')
                    putc('\r', fp);
                putc(s[i], fp);
                lastc = s[i];
            }
#else
            if (fwrite(s, 1, end - start, fp) < (unsigned)(end - start)) {
                delete [] s;
                fclose(fp);
                return (false);
            }
#endif
            delete [] s;
            if (end - start < WRT_BLOCK)
                break;
            start = end;
        }
        fclose(fp);
        return (true);
    }
    return (false);
}


void
QTeditDlg::set_source(const char *str)
{
    delete [] ed_source_file;
    ed_source_file = lstring::copy(str);
    if (str) {
        char buf[80];
        if (strlen(str) > 72) {
            snprintf(buf, sizeof(buf), "...%s", str + strlen(str) - 72);
            ed_title->setTitle(buf);
            return;
        }
        ed_title->setTitle(str);
    }
}


void
QTeditDlg::set_sens(bool set)
{
    if (ed_File_Load)
        ed_File_Load->setEnabled(set);
    if (ed_File_Read)
        ed_File_Read->setEnabled(set);
    if (ed_File_SaveAs)
        ed_File_SaveAs->setEnabled(set);
}


bool
QTeditDlg::text_changed()
{
    if (ed_editor_type == Browser || ed_editor_type == Mailer)
        return (false);
    QByteArray ba = ed_text_editor->toPlainText().toLatin1();
    if ((unsigned int)ba.size() != ed_len)
        return (true);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    if (qChecksum(QByteArrayView(ba)) != ed_chksum)
#else
    if (qChecksum(ba.constData(), ed_len) != ed_chksum)
#endif
        return (true);
    return (false);
}


//
// Actions for the search dialog.
//

void
QTeditDlg::search_down_slot()
{
    QString target = ed_searcher->get_target();
    if (!target.isNull() && !target.isEmpty()) {
        delete [] ed_last_search;
        ed_last_search = lstring::copy(target.toLatin1().constData());

        QTextCursor c = ed_text_editor->textCursor();
        QTextDocument *doc = ed_text_editor->document();
        QTextCursor nc = doc->find(target, c,
            ed_ign_case ? (QTextDocument::FindFlag)0 :
                QTextDocument::FindCaseSensitively);
        if (!nc.isNull())
            ed_text_editor->setTextCursor(nc);
        else
            ed_searcher->set_transient_message("Not found!");
    }
}


void
QTeditDlg::search_up_slot()
{
    QString target = ed_searcher->get_target();
    if (!target.isNull() && !target.isEmpty()) {
        delete [] ed_last_search;
        ed_last_search = lstring::copy(target.toLatin1().constData());

        QTextCursor c = ed_text_editor->textCursor();
        QTextDocument *doc = ed_text_editor->document();
        QTextCursor nc = doc->find(target, c,
            ed_ign_case ? QTextDocument::FindBackward :
                QTextDocument::FindBackward |
                    QTextDocument::FindCaseSensitively);
        if (!nc.isNull())
            ed_text_editor->setTextCursor(nc);
        else
            ed_searcher->set_transient_message("Not found!");
    }
}


void
QTeditDlg::ignore_case_slot(bool set)
{
    ed_ign_case = set;
}


void
QTeditDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_EDITOR) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_EDITOR))
            ed_text_editor->setFont(*fnt);
    }
}

