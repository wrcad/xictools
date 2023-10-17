
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
#include <QPushButton>
#include <QStatusBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>

// If the message would exceed this size, it is split into multiple
// messages
#define MAIL_MAXSIZE 1600000

// defaults
static const char *mail_addr = "bugs@wrcad.com";
static const char *mail_subject = "bug report";

// for hardcopies
static HCcb edHCcb =
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
static const char * const attach_xpm[] = {
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


QTeditDlg::QTeditDlg(QTbag *owner, QTeditDlg::WidgetType type,
    const char *file_or_string, bool with_source, void *arg) :
    QDialog(owner ? owner->Shell() : 0), QTbag(this)
{
    p_parent = owner;
    p_cb_arg = arg;
    ed_widget_type = type;

    ed_lastEvent = QUIT;
    ed_savedAs = 0;
    ed_sourceFile = 0;
    ed_dropFile = 0;
    ed_lastSearch = 0;
    ed_textChanged = false;
    ed_ignCase = false;
    ed_ignChange = false;
    ed_searcher = 0;

    ed_menubar = 0;
    ed_filemenu = 0;
    ed_editmenu = 0;
    ed_optmenu = 0;
    ed_helpmenu = 0;
    ed_to_entry = 0;
    ed_subj_entry = 0;
    ed_text_editor = 0;
    ed_status_bar = 0;

    ed_Load = 0;
    ed_Read = 0;
    ed_Save = 0;
    ed_SaveAs = 0;
    ed_HelpMenu = 0;

    if (owner)
        owner->MonitorAdd(this);
    setAttribute(Qt::WA_DeleteOnClose);

    ed_menubar = new QMenuBar(this);
    ed_filemenu = new QMenu(this);
    ed_filemenu->setTitle(QString(tr("&File")));
    if (ed_widget_type == Editor || ed_widget_type == Browser) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        ed_filemenu->addAction(QString(tr("&Open")), Qt::CTRL|Qt::Key_O,
            this, SLOT(open_slot()));
        ed_Load = ed_filemenu->addAction(QString(tr("&Load")),
            Qt::CTRL|Qt::Key_L, this, SLOT(load_slot()));
#else
        ed_filemenu->addAction(QString(tr("&Open")), this, SLOT(open_slot()),
            Qt::CTRL|Qt::Key_O);
        ed_Load = ed_filemenu->addAction(QString(tr("&Load")), this,
            SLOT(load_slot()), Qt::CTRL|Qt::Key_L);
#endif
    }
    if (ed_widget_type != Browser) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        ed_Read = ed_filemenu->addAction(QString(tr("&Read")),
            Qt::CTRL|Qt::Key_R, this, SLOT(read_slot()));
#else
        ed_Read = ed_filemenu->addAction(QString(tr("&Read")), this,
            SLOT(read_slot()), Qt::CTRL|Qt::Key_R);
#endif
    }
    if (ed_widget_type == Editor || ed_widget_type == StringEditor) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        ed_Save = ed_filemenu->addAction(QString(tr("&Save")),
            Qt::CTRL|Qt::Key_S, this, SLOT(save_slot()));
#else
        ed_Save = ed_filemenu->addAction(QString(tr("&Save")), this,
            SLOT(save_slot()), Qt::CTRL|Qt::Key_S);
#endif
    }
    if (ed_widget_type != StringEditor) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        ed_SaveAs = ed_filemenu->addAction(QString(tr("Save &As")),
            Qt::CTRL|Qt::Key_A, this, SLOT(save_as_slot()));
        ed_filemenu->addAction(QString(tr("&Print")), Qt::CTRL|Qt::Key_P,
            this, SLOT(print_slot()));
#else
        ed_SaveAs = ed_filemenu->addAction(QString(tr("Save &As")), this,
            SLOT(save_as_slot()), Qt::CTRL|Qt::Key_A);
        ed_filemenu->addAction(QString(tr("&Print")), this,
            SLOT(print_slot()), Qt::CTRL|Qt::Key_P);
#endif
    }
    ed_filemenu->addSeparator();
    if (ed_widget_type == Mailer)  {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        ed_filemenu->addAction(QString(tr("Send &Mail")), Qt::CTRL|Qt::Key_M,
            this, SLOT(send_slot()));
#else
        ed_filemenu->addAction(QString(tr("Send &Mail")), this,
            SLOT(send_slot()), Qt::CTRL|Qt::Key_M);
#endif
    }
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    ed_filemenu->addAction(QString(tr("&Quit")), Qt::CTRL|Qt::Key_Q,
        this, SLOT(quit_slot()));
#else
    ed_filemenu->addAction(QString(tr("&Quit")), this, SLOT(quit_slot()),
            Qt::CTRL|Qt::Key_Q);
#endif
    ed_menubar->addMenu(ed_filemenu);

    ed_editmenu = new QMenu(this);
    ed_editmenu->setTitle(QString(tr("&Edit")));
    ed_editmenu->addAction(QString(tr("&Cut")), this,
        SLOT(cut_slot()));
    ed_editmenu->addAction(QString(tr("&Copy")), this,
        SLOT(copy_slot()));
    ed_editmenu->addAction(QString(tr("&Paste")), this,
        SLOT(paste_slot()));
    ed_menubar->addMenu(ed_editmenu);

    ed_optmenu = new QMenu(this);
    ed_optmenu->setTitle(QString(tr("&Options")));
    ed_optmenu->addAction(QString(tr("&Search")), this,
        SLOT(search_slot()));
    if ((ed_widget_type == Editor || ed_widget_type == StringEditor) &&
            with_source) {
        ed_optmenu->addAction(QString(tr("&Source")), this,
            SLOT(source_slot()));
    }
    if (ed_widget_type == Mailer) {
        ed_optmenu->addAction(QString(tr("&Attach")), this,
            SLOT(attach_slot()));
    }
    ed_optmenu->addAction(QString(tr("&Font")), this,
        SLOT(font_slot()));
    ed_menubar->addMenu(ed_optmenu);

    ed_menubar->addSeparator();
    ed_helpmenu = new QMenu(this);
    ed_helpmenu->setTitle(QString(tr("&Help")));
    ed_helpmenu->addAction(QString(tr("&Help")), this,
        SLOT(help_slot()));
    ed_HelpMenu = ed_menubar->addMenu(ed_helpmenu);

    QMargins qmtop(2, 2, 2, 2);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMenuBar(ed_menubar);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    if (ed_widget_type == Mailer) {
        QGroupBox *gb = new QGroupBox(this);
        gb->setTitle(QString(tr("To:")));
        QVBoxLayout *vb = new QVBoxLayout(gb);
        vb->setContentsMargins(qmtop);
        ed_to_entry = new QLineEdit(gb);
        vb->addWidget(ed_to_entry);
        vbox->addWidget(gb);
        ed_to_entry->setText(QString(mail_addr));

        gb = new QGroupBox(this);
        gb->setTitle(QString(tr("Subject:")));
        vb = new QVBoxLayout(gb);
        vb->setContentsMargins(qmtop);
        ed_subj_entry = new QLineEdit(gb);
        vb->addWidget(ed_subj_entry);
        vbox->addWidget(gb);
        ed_subj_entry->setText(QString(mail_subject));
    }
    ed_text_editor = new QTextEdit(this);
    vbox->addWidget(ed_text_editor);
    ed_text_editor->setLineWrapMode(QTextEdit::NoWrap);

    ed_status_bar = new QStatusBar(this);
    vbox->addWidget(ed_status_bar);

    static bool checked_env;
    if (!checked_env) {
        checked_env = true;
        const char *fn = getenv("XEDITOR_FONT");
        if (fn)
            FC.setName(fn, FNT_EDITOR);
    }
    QFont *fnt;
    if (FC.getFont(&fnt, FNT_EDITOR))
        ed_text_editor->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    if (ed_widget_type == Editor || ed_widget_type == Browser) {
        if (ed_widget_type == Browser) {
            ed_text_editor->setReadOnly(true);
            ed_text_editor->viewport()->setCursor(Qt::ArrowCursor);
        }

        char *fname = pathlist::expand_path(file_or_string, false, true);
        check_t which = filestat::check_file(fname, R_OK);
        if (which == NOGO) {
            fname = filestat::make_temp("xe");
            if (ed_widget_type == Browser)
                ed_status_bar->showMessage(QString("Can't open file!"));
        }
        else if (which == READ_OK) {
            if (!read_file(fname, true))
                ed_status_bar->showMessage(QString("Can't open file!"));
            if (ed_widget_type == Browser)
                ed_status_bar->showMessage(QString("Read-Only"));
        }
        else if (which == NO_EXIST) {
            if (ed_widget_type == Browser)
                ed_status_bar->showMessage(QString("Can't open file!"));
        }
        else {
            ed_status_bar->showMessage(QString("Read-Only"));
            ed_text_editor->setReadOnly(true);
        }
        set_source(fname);
        delete [] fname;
    }
    else if (ed_widget_type == Mailer) {
        ed_text_editor->setPlainText(file_or_string);
        ed_status_bar->showMessage(QString("Please enter your comment"));
    }
    else
        ed_text_editor->setPlainText(file_or_string);

    ed_textChanged = false;
    connect(ed_text_editor, SIGNAL(textChanged()),
        this, SLOT(text_changed_slot()));

    ed_savedAs = 0;
    ed_lastEvent = LOAD;
}


QTeditDlg::~QTeditDlg()
{
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller) {
        QObject *o = (QObject*)p_caller;
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn)
                btn->setChecked(false);
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a)
                a->setChecked(false);
        }
    }
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    }
    delete [] ed_savedAs;
    delete [] ed_sourceFile;
    delete [] ed_dropFile;
    delete [] ed_lastSearch;
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
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn)
                connect(btn, SIGNAL(clicked()),
                    this, SLOT(quit_slot()));
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a)
                connect(a, SIGNAL(triggered()),
                    this, SLOT(quit_slot()));
        }
    }
}


void
QTeditDlg::set_mailaddr(const char *str)
{
    if (ed_to_entry)
        ed_to_entry->setText(QString(str));
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
    delete [] ed_dropFile;
    ed_dropFile = lstring::copy(fname);
    if (wb_input)
        wb_input->popdown();
    ed_Load->activate(QAction::Trigger);
}


// Pop up the file selector.
//
void
QTeditDlg::open_slot()
{
    // open the file selector in the directory of the current file
    char *path = lstring::copy(ed_sourceFile);
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
    if (fsel)
        connect(fsel, SIGNAL(file_selected(const char*, void*)),
            this, SLOT(file_selection_slot(const char*, void*)));

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
    ed_text_editor->setReadOnly(true);

    bool ok = read_file(fname, true);
    if (ed_widget_type != Browser)
        ed_text_editor->setReadOnly(false);
    if (wb_input)
        wb_input->popdown();
    if (!ok) {
        char tbuf[256];
        if (strlen(fname) > 64)
            strcpy(fname + 60, "...");
        snprintf(tbuf, sizeof(tbuf), "Can't open %s!", fname);
        ed_status_bar->showMessage(QString(tbuf));
        delete [] fname;
        return;
    }
    set_source(fname);
    if (ed_textChanged) {
        ed_textChanged = false;
        if (ed_Save)
            ed_Save->setEnabled(false);
    }
    if (ed_savedAs) {
        delete [] ed_savedAs;
        ed_savedAs = 0;
    }
    delete [] fname;
}


// Callback to load a new file into the editor
//
void
QTeditDlg::load_slot()
{
    if (ed_textChanged) {
        if (ed_lastEvent != LOAD) {
            ed_lastEvent = LOAD;
            PopUpMessage(
                "Text has been modified.  Press Load again to load", false);
            return;
        }
        if (wb_message)
            wb_message->popdown();
    }
    char buf[512];
    *buf = '\0';
    if (ed_dropFile) {
        strcpy(buf, ed_dropFile);
        delete [] ed_dropFile;
        ed_dropFile = 0;
    }
    else if (p_callback)
        (*p_callback)(buf, p_cb_arg, XE_LOAD);
    if (wb_input)
        wb_input->popdown();
    PopUpInput(0, buf, "Load File", 0, 0);
    connect(wb_input, SIGNAL(action_call(const char*, void*)),
        this, SLOT(load_file_slot(const char*, void*)));
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
    ed_text_editor->setReadOnly(true);

    char tbuf[256];
    if (!read_file(fname, false)) {
        if (strlen(fname) > 64)
            strcpy(fname + 60, "...");
        snprintf(tbuf, sizeof(tbuf), "Can't open %s!", fname);
        ed_status_bar->showMessage(QString(tbuf));
    }
    else {
        text_changed_slot();  // this isn't called otherwise
        ed_text_editor->setReadOnly(false);
        if (strlen(fname) > 64)
            strcpy(fname + 60, "...");
        snprintf(tbuf, sizeof(tbuf), "Successfully read %s", fname);
        ed_status_bar->showMessage(QString(tbuf));
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
    if (ed_dropFile) {
        strcpy(buf, ed_dropFile);
        delete [] ed_dropFile;
        ed_dropFile = 0;
    }
    PopUpInput(0, buf, "Read File", 0, 0);
    connect(wb_input, SIGNAL(action_call(const char*, void*)),
        this, SLOT(read_file_slot(const char*, void*)));
}


// Save file.
//
void
QTeditDlg::save_slot()
{
    ed_lastEvent = SAVE;

    if (p_callback) {
        char *st =
            lstring::copy(ed_text_editor->toPlainText().toLatin1().constData());
        bool ret = (*p_callback)(st, p_cb_arg, XE_SAVE);
        delete [] st;
        if (ret)
            delete this;
        return;
    }
    char *fname = ed_sourceFile;
    if (filestat::check_file(fname, W_OK) == NOGO) {
        PopUpMessage(filestat::error_msg(), true);
        return;
    }
    if (!write_file(fname, 0 , -1)) {
        PopUpMessage("Error: can't write file", true);
        return;
    }

    ed_status_bar->showMessage(QString(tr("Text saved")));
    // This can be called with ed_textChanged false if we saved
    // under a new name, and made no subsequent changes.
    if (ed_textChanged)
        ed_textChanged = false;
    ed_SaveAs->setEnabled(false);
    if (ed_savedAs) {
        delete [] ed_savedAs;
        ed_savedAs = 0;
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
        const char *oldfname = ed_sourceFile;
        // no selected text
        if (same(fname, oldfname)) {
            if (!ed_textChanged) {
                ed_status_bar->showMessage(QString("No save needed"));
                delete [] fname;
                return;
            }
            if (!write_file(fname, 0 , -1)) {
                PopUpMessage("Write error, text not saved", true);
                delete [] fname;
                return;
            }
            if (ed_savedAs) {
                delete [] ed_savedAs;
                ed_savedAs = 0;
            }
            ed_textChanged = false;
            if (ed_Save)
                ed_Save->setEnabled(false);
        }
        else {
            if (!write_file(fname, 0 , -1)) {
                PopUpMessage("Write error, text not saved", true);
                delete [] fname;
                return;
            }
            if (ed_savedAs)
                delete [] ed_savedAs;
            ed_savedAs = lstring::copy(fname);
            ed_textChanged = false;
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
    ed_status_bar->showMessage(QString(mesg));
    delete [] fname;
}


// Save file under a new name.
//
void
QTeditDlg::save_as_slot()
{
    ed_lastEvent = SAVEAS;
    if (wb_input)
        wb_input->popdown();

    QTextCursor c = ed_text_editor->textCursor();
    int start = c.anchor();
    int end = c.position();
    if (start == end) {
        const char *fname = ed_sourceFile;
        if (ed_widget_type == Mailer)
            fname = "";
        PopUpInput(0, fname, "Save File", 0, 0);
        connect(wb_input, SIGNAL(action_call(const char*, void*)),
            this, SLOT(save_file_as_slot(const char*, void*)));
    }
    else {
        PopUpInput(0, "", "Save Block", 0, 0);
        connect(wb_input, SIGNAL(action_call(const char*, void*)),
            this, SLOT(save_file_as_slot(const char*, void*)));
    }
}


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
    QList<QAction*> acts = ed_menubar->actions();
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
                    snprintf(buf, sizeof(buf),
                        "Warning: operation returned error status %d.\n", rt);
                    fprintf(stderr, buf);
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
    if ((ed_widget_type == Editor || ed_widget_type == StringEditor) &&
            ed_textChanged) {
        if (ed_lastEvent != QUIT) {
            ed_lastEvent = QUIT;
            PopUpMessage(
                "Text has been modified.  Press Quit again to quit", false);
            return;
        }
    }
    deleteLater();
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
        ed_searcher = new QTsearchDlg(this, ed_lastSearch);
        ed_searcher->register_usrptr((void**)&ed_searcher);
        ed_searcher->set_visible(true);

        ed_searcher->set_ign_case(ed_ignCase);
        connect(ed_searcher, SIGNAL(search_down()),
            this, SLOT(search_down_slot()));
        connect(ed_searcher, SIGNAL(search_up()),
            this, SLOT(search_up_slot()));
        connect(ed_searcher, SIGNAL(ignore_case(bool)),
            this, SLOT(ignore_case_slot(bool)));
    }
}


// Send the file to the application to be used as input.
//
void
QTeditDlg::source_slot()
{
    const char *fname;
    ed_lastEvent = SOURCE;
    if (ed_textChanged) {
        fname = filestat::make_temp("sp");
        if (!write_file(fname, 0 , -1)) {
            PopUpMessage("Error: can't write temp file", true);
            delete [] fname;
            return;
        }
    }
    else {
        if (ed_savedAs)
            fname = lstring::copy(ed_savedAs);
        else
            fname = lstring::copy(ed_sourceFile);
    }
    if (p_callback) {
        (*p_callback)(fname, p_cb_arg, XE_SOURCE);
        ed_status_bar->showMessage(QString(tr("Text sourced")));
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
        ed_status_bar->showMessage(QString(tr(tbuf)));
        delete [] fname;
        return;
    }

    QAction *a = new QAction(this);
    a->setData(QVariant(QString(fname)));
    QMenu *menu = new QMenu(this);
    a->setMenu(menu);
    a->setIcon(QIcon(QPixmap(attach_xpm)));
    a->setText(QString("    "));  // need to set text or nothing will show!
    ed_menubar->insertAction(ed_HelpMenu, a);
    QAction *a_path = a;

    char *fn = lstring::copy(lstring::strip_path(fname));
    if (strlen(fn) > 23)
        strcpy(fn + 20, "...");
    a = menu->addAction(QString(fn));
    a->setEnabled(false);
    delete fn;
    a = new QAction(this);
    a->setText(QString("Unattach"));
    a->setData(QVariant((unsigned long long)(unsigned long)a_path));
    menu->addAction(a);
    connect(menu, SIGNAL(triggered(QAction*)),
        this, SLOT(unattach_slot(QAction*)));

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
    if (ed_dropFile) {
        strcpy(buf, ed_dropFile);
        delete [] ed_dropFile;
        ed_dropFile = 0;
    }
    PopUpInput(0, buf, "Attach File", 0, 0);
    connect(wb_input, SIGNAL(action_call(const char*, void*)),
        this, SLOT(attach_file_slot(const char*, void*)));
}


// Unattach a file.
//
void
QTeditDlg::unattach_slot(QAction *a)
{
    if (a) {
        QAction *a_path = (QAction*)(unsigned long)a->data().toULongLong();
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
            ed_widget_type == Mailer ? "mailclient" : "xeditor");
    else
        PopUpHelp(ed_widget_type == Mailer ? "mailclient" : "xeditor");
}


// Update state to reflect text modified.
//
void
QTeditDlg::text_changed_slot()
{
    if (ed_ignChange)
        return;
    ed_lastEvent = TEXTMOD;
    ed_status_bar->showMessage(QString(""));
    if (wb_message)
        wb_message->popdown();
    if (ed_textChanged)
        return;
    ed_textChanged = true;
    if (ed_Save)
        ed_Save->setEnabled(true);
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
        for (;;) {
            int n = fread(buf, 1, 1024, fp);
            ed_text_editor->append(QString(QByteArray(buf, n)));
            if (n < 1024)
                break;
        }
        fclose(fp);
        QTextCursor c = ed_text_editor->textCursor();
        c.setPosition(0);
        ed_text_editor->setTextCursor(c);
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
        QString qs = ed_text_editor->toPlainText();
        int length = qs.size();
        char *buffer = lstring::copy(qs.toLatin1().constData());
        if (endpos >= 0 && endpos < length)
            length = endpos;
        int start = startpos;
        for (;;) {
            int end = start + WRT_BLOCK;
            if (end > length)
                end = length;
            if (end == start)
                break;
            if (fwrite(buffer + start, 1, end - start, fp) <
                    (unsigned)(end - start)) {
                delete [] buffer;
                fclose(fp);
                return (false);
            }
            if (end - start < WRT_BLOCK)
                break;
            start = end;
        }
        fclose(fp);
        delete [] buffer;
        return (true);
    }
    return (false);
}


void
QTeditDlg::set_source(const char *str)
{
    delete [] ed_sourceFile;
    ed_sourceFile = lstring::copy(str);
}


void
QTeditDlg::set_sens(bool set)
{
    if (ed_Load)
        ed_Load->setEnabled(set);
    if (ed_Read)
        ed_Read->setEnabled(set);
    if (ed_SaveAs)
        ed_SaveAs->setEnabled(set);
}


//
// Actions for the search dialog.
//

void
QTeditDlg::search_down_slot()
{
    QString target = ed_searcher->get_target();
    if (!target.isNull() && !target.isEmpty()) {
        delete [] ed_lastSearch;
        ed_lastSearch = lstring::copy(target.toLatin1().constData());

        QTextCursor c = ed_text_editor->textCursor();
        QTextDocument *doc = ed_text_editor->document();
        QTextCursor nc = doc->find(target, c,
            ed_ignCase ? (QTextDocument::FindFlag)0 :
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
        delete [] ed_lastSearch;
        ed_lastSearch = lstring::copy(target.toLatin1().constData());

        QTextCursor c = ed_text_editor->textCursor();
        QTextDocument *doc = ed_text_editor->document();
        QTextCursor nc = doc->find(target, c,
            ed_ignCase ? QTextDocument::FindBackward :
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
    ed_ignCase = set;
}


void
QTeditDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_EDITOR) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_EDITOR))
            ed_text_editor->setFont(*fnt);
    }
}

