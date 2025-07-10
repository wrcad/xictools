
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

#include "config.h"
#include "qtfiles.h"
#include "simulator.h"
#include "graph.h"
#include "output.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "qttoolb.h"
#include "qtinterf/qtmcol.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qtfile.h"
#include "qtinterf/qttextw.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#endif
#include <sys/stat.h>

#include <QLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QGroupBox>
#include <QComboBox>
#include <QToolButton>
#include <QPushButton>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QDrag>
#include <QMimeData>
#include <QAction>
#include <QApplication>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>


//===========================================================================
// Pop up to display a listing of files found along the 'sourcepath'.
// Buttons:
//  Dismiss: pop down
//  Source: source the circuit pointed to while active
//  Edit:   open the editor with file pointed to while active
//  Help:   bring up help panel

// Keywords referenced in help database:
//  filespanel


// Derived class to create mime data for selection, so we can prepend
// the file path.
//
class QTfileTextEdit : public QTtextEdit
{
public:
    QTfileTextEdit(QWidget *prnt = 0) : QTtextEdit(prnt) { }

protected:
    QMimeData * createMimeDataFromSelection() const;
};


QMimeData *
QTfileTextEdit::createMimeDataFromSelection() const
{
    if (QTfilesListDlg::self()) {
        char *sel = QTfilesListDlg::self()->get_selection();
        if (sel && *sel) {
            QMimeData *dat = new QMimeData();
            dat->setText(sel);
            return (dat);
        }
    }
    return (0);
}
// End of QTfileTextEdit functions.


void
QTtoolbar::PopUpFiles(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        delete QTfilesListDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTfilesListDlg::self())
            QTfilesListDlg::self()->update();
        return;
    }
    if (QTfilesListDlg::self())
        return;

    new QTfilesListDlg(x, y);
    QTfilesListDlg::self()->show();
}


void
QTtoolbar::UpdateFiles()
{
    if (QTfilesListDlg::self())
        QTfilesListDlg::self()->update();
}
// End of QTtoolbar functions.


// The buttons displayed.
#define FB_EDIT     "Edit"
#define FB_SOURCE   "Source"
#define FB_HELP     "Help"


const char *QTfilesListDlg::nofiles_msg = "  no recognized files found\n";
const char *QTfilesListDlg::files_msg = "Files found along the sourcepath";

// The directories in the path are monitored for changes.
//
sPathList *QTfilesListDlg::fl_path_list;
char *QTfilesListDlg::fl_cwd;
int QTfilesListDlg::fl_timer_tag;
QTfilesListDlg *QTfilesListDlg::instPtr;

QTfilesListDlg::QTfilesListDlg(int xx, int yy) : QTbag(this)
{
    instPtr = this;
    fl_caller = TB()->entries(tid_files)->action();;
    for (int i = 0; i < MAX_BTNS; i++)
        fl_buttons[i] = 0;
    fl_button_box = 0;
    fl_menu = 0;
    fl_notebook = 0;

    fl_start = 0;
    fl_end = 0;
    fl_drag_start = false;
    fl_drag_btn = 0;
    fl_drag_x = 0;
    fl_drag_y = 0;
    fl_directory = 0;

    fl_selection = 0;
    fl_noupdate = 0;

    setWindowTitle(tr("Search Path Files Listing"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // Create the layout for the button row.
    fl_button_box = new QHBoxLayout();
    vbox->addLayout(fl_button_box);
    fl_button_box->setContentsMargins(qm);
    fl_button_box->setSpacing(2);

    // title label
    //
    QGroupBox *gb = new QGroupBox();
    vbox->addWidget(gb);
    QHBoxLayout *hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qmtop);
    hbox->setSpacing(2);

    QLabel *label = new QLabel(tr(files_msg));
    label->setAlignment(Qt::AlignCenter);
    hbox->addWidget(label);

    int fw, fh;
    if (!QTfont::stringBounds(0, FNT_FIXED, &fw, &fh))
        fw = 8;
    int cols = (sizeHint().width()-8)/fw - 2;
    if (!fl_path_list) {
        fl_path_list = fl_listing(cols);
        fl_monitor_setup();
    }
    else if (cols != fl_path_list->columns()) {
        for (sDirList *dl = fl_path_list->dirs(); dl; dl = dl->next())
            dl->set_dirty(true);
        fl_path_list->set_columns(cols);
        fl_idle_proc(0);
    }

    fl_menu = new QComboBox();
    vbox->addWidget(fl_menu);

    // creates notebook
    init_viewing_area();
    vbox->addWidget(fl_notebook);

    // dismiss button
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QTfilesListDlg::dismiss_btn_slot);

    update();

    TB()->FixLoc(&xx, &yy);
    TB()->SetActiveDlg(tid_files, this);
    move(xx, yy);
}


QTfilesListDlg::~QTfilesListDlg()
{
    instPtr = 0;
    if (fl_path_list) {
        for (sDirList *dl = fl_path_list->dirs(); dl; dl = dl->next())
            dl->set_dataptr(0);
    }
    delete [] fl_directory;

    if (fl_caller)
        QTdev::Deselect(fl_caller);
    delete [] fl_selection;
    TB()->SetLoc(tid_files, this);
    TB()->SetActiveDlg(tid_files, 0);
}


void
QTfilesListDlg::update()
{
    if (fl_noupdate) {
        fl_noupdate++;
        return;
    }

    const char *btns[5];
    btns[0] = FB_EDIT;
    btns[1] = FB_SOURCE;
    btns[2] = FB_HELP;
    int nbtns = 3;

    wordlist *wl = CP.VarEval(kw_sourcepath);
    char *path = wordlist::flatten(wl);
    wordlist::destroy(wl);
    update(path, btns, nbtns);
    delete [] path;
    fl_desel();
}


// Update the directory listings.
//
void
QTfilesListDlg::update(const char *path, const char **buttons, int numbuttons)
{
    if (path && fl_path_list) {
        stringlist *s0 = 0, *se = 0;
        for (sDirList *dl = fl_path_list->dirs(); dl; dl = dl->next()) {
            if (!s0)
                s0 = se = new stringlist(lstring::copy(dl->dirname()), 0);
            else {
                se->next = new stringlist(lstring::copy(dl->dirname()), 0);
                se = se->next;
            }
        }
        if (fl_check_path_and_update(path))
            relist(s0);
        stringlist::destroy(s0);
    }

    if (numbuttons > 0 && buttons) {
        if (numbuttons > MAX_BTNS)
            numbuttons = MAX_BTNS;

        for (int i = 0; fl_buttons[i]; i++) {
            fl_button_box->removeWidget(fl_buttons[i]);
            delete fl_buttons[i];
            fl_buttons[i] = 0;
        }
        for (int i = 0; i < numbuttons; i++) {
            QToolButton *tbtn = new QToolButton();
            tbtn->setText(tr(buttons[i]));
            if (!strcmp(buttons[i], FB_HELP))
                fl_button_box->addStretch(1);
            fl_button_box->addWidget(tbtn);
            tbtn->setCheckable(true);
            fl_buttons[i] = tbtn;
            connect(tbtn, &QAbstractButton::toggled,
                this, &QTfilesListDlg::button_slot);
        }
    }
}


// Return the full path name of the selected file, or 0 if no
// selection.
//
char *
QTfilesListDlg::get_selection()
{
    if (!fl_directory || !*fl_directory)
        return (0);
    char *s = wb_textarea->get_selection();
    if (s) {
        if (*s) {
            sLstr lstr;
            lstr.add(fl_directory);
            lstr.add_c('/');
            lstr.add(s);
            delete [] s;
            return (lstr.string_trim());
        }
        delete [] s;
    }
    return (0);
}


// Initialize the notebook of file listings.
//
void
QTfilesListDlg::init_viewing_area()
{
    if (fl_path_list) {
        for (sDirList *dl = fl_path_list->dirs(); dl; dl = dl->next())
            dl->set_dataptr(0);
    }
    if (!fl_notebook)
        fl_notebook = new QStackedWidget();
    if (!fl_path_list)
        return;

    int init_page = 0;
    int maxchars = 120;
    if (fl_path_list && fl_directory) {
        int i = 0;
        for (sDirList *dl = fl_path_list->dirs(); dl; dl = dl->next()) {
            if (!strcmp(fl_directory, dl->dirname())) {
                init_page = i;
                break;
            }
            i++;
        }
    }
    delete [] fl_directory;
    fl_directory = 0;

    char buf[256];
    int i = 0;
    for (sDirList *dl = fl_path_list->dirs(); dl; i++, dl = dl->next()) {
        if (i == init_page)
            fl_directory = lstring::copy(dl->dirname());

        int len = strlen(dl->dirname());
        if (len <= maxchars)
            strcpy(buf, dl->dirname());
        else {
            int partchars = maxchars/2 - 2;
            strncpy(buf, dl->dirname(), partchars);
            strcpy(buf + partchars, " ... ");
            strcat(buf, dl->dirname() + len - partchars);
        }
        fl_menu->addItem(buf);

        create_page(dl);
        QTtextEdit *nbtext = (QTtextEdit*)dl->dataptr();
        if (nbtext) {
            fl_notebook->addWidget(nbtext);
        }
        if (i == init_page)
            wb_textarea = nbtext;
    }
    connect(fl_notebook, &QStackedWidget::currentChanged,
        this, &QTfilesListDlg::page_change_slot);

    fl_notebook->setCurrentIndex(init_page);
    fl_menu->setCurrentIndex(init_page);
    connect(fl_menu, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &QTfilesListDlg::menu_change_slot);
}


namespace {
    // Return the index of str, -1 if not in list.
    //
    int findstr(const char *str, const char **ary, int len)
    {
        if (str && ary && len > 0) {
            for (int i = 0; i < len; i++) {
                if (ary[i] && !strcmp(str, ary[i]))
                    return (i);
            }
        }
        return (-1);
    }
}


// Reset the notebook listings.  The fl_path_list has already been set. 
// The argument is the old path list, which still represents the state
// of the menu and pages.
//
void
QTfilesListDlg::relist(stringlist *oldlist)
{
    if (!oldlist) {
        // This is the original way of doing things, just rebuild
        // everything.

        init_viewing_area();
        return;
    }

    // New way:  keep what we already have, just add new directories. 
    // This is faster, and retains the selection.

    // Put the list in an array, easier to work with.
    int len = stringlist::length(oldlist);
    const char **ary = new const char*[len];
    stringlist *stmp = oldlist;
    for (int i = 0; i < len; i++) {
        ary[i] = stmp->string;
        stmp = stmp->next;
    }

    int n = 0;
    for (sDirList *dl = fl_path_list->dirs(); dl; n++, dl = dl->next()) {
        int oldn = findstr(dl->dirname(), ary, len);
        if (oldn == n)
            continue;
        if (oldn > 0) {
            // Directory moved to a new location in the path.  Make
            // the corresponding change to the notebook, menu, and the
            // array.

            QWidget *pg = fl_notebook->widget(oldn);
            fl_notebook->removeWidget(pg);
            fl_notebook->insertWidget(n, pg);

            const char *t = ary[oldn];
            // We know that oldn is larger than n.
            for (int i = oldn; i > n; i--)
                ary[i] = ary[i-1];
            ary[n] = t;
            continue;
        }
        // Directory wasn't found in the old list, insert a new page
        // and menu entry, and add to the array.

        QWidget *pg = create_page(dl);
        fl_notebook->insertWidget(n, pg);

        char buf[256];
        int maxchars = 120;
        int dlen = strlen(dl->dirname());
        if (dlen <= maxchars)
            strcpy(buf, dl->dirname());
        else {
            int partchars = maxchars/2 - 2;
            strncpy(buf, dl->dirname(), partchars);
            strcpy(buf + partchars, " ... ");
            strcat(buf, dl->dirname() + dlen - partchars);
        }

        const char **nary = new const char*[len+1];
        for (int i = 0; i < n; i++)
            nary[i] = ary[i];
        nary[n] = dl->dirname();
        for (int i = n; i < len; i++)
            nary[i+1] = ary[i];
        len++;
        delete [] ary;
        ary = nary;
    }
    delete [] ary;

    // Anything at position n or above is not in the list so should be
    // deleted.

    while (fl_notebook->widget(n) != 0)
        fl_notebook->removeWidget(fl_notebook->widget(n));

    // Clear the combo box.
    fl_menu->clear();

    for (sDirList *dl = fl_path_list->dirs(); dl; dl = dl->next()) {
        char buf[256];
        int maxchars = 120;
        int dlen = strlen(dl->dirname());
        if (dlen <= maxchars)
            strcpy(buf, dl->dirname());
        else {
            int partchars = maxchars/2 - 2;
            strncpy(buf, dl->dirname(), partchars);
            strcpy(buf + partchars, " ... ");
            strcat(buf, dl->dirname() + dlen - partchars);
        }
        fl_menu->addItem(buf);
    }
    n = fl_notebook->currentIndex();
    fl_menu->setCurrentIndex(n);
}


// Select the chars in the range, start=end deselects existing.
//
void
QTfilesListDlg::select_range(QTtextEdit *caller, int start, int end)
{
    caller->select_range(start, end);
    fl_start = start;
    fl_end = end;
    fl_desel();
}


QWidget *
QTfilesListDlg::create_page(sDirList *dl)
{
    QWidget *page = new QWidget();
    QVBoxLayout *vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(2, 2, 2, 2);
    vbox->setSpacing(2);

    // scrolled text area
    //
    QTtextEdit *nbtext = new QTfileTextEdit();
    vbox->addWidget(nbtext);
    dl->set_dataptr(nbtext);
    nbtext->setReadOnly(true);
    nbtext->setMouseTracking(true);
    nbtext->setAcceptDrops(true);
    nbtext->set_chars(dl->dirfiles());

    QFont *tfont;
    if (Fnt()->getFont(&tfont, FNT_SCREEN))
        nbtext->setFont(*tfont);
    connect(QTfont::self(), &QTfont::fontChanged,
        this, &QTfilesListDlg::font_changed_slot);

    connect(nbtext, &QTfileTextEdit::resize_event,
        this, &QTfilesListDlg::resize_slot);
    connect(nbtext, &QTfileTextEdit::press_event,
        this, &QTfilesListDlg::mouse_press_slot);
    connect(nbtext, &QTfileTextEdit::release_event,
        this, &QTfilesListDlg::mouse_release_slot);
    connect(nbtext, &QTfileTextEdit::motion_event,
        this, &QTfilesListDlg::mouse_motion_slot);
    connect(nbtext, &QTfileTextEdit::mime_data_handled,
        this, &QTfilesListDlg::mime_data_handled_slot);
    connect(nbtext, &QTfileTextEdit::mime_data_delivered,
        this, &QTfilesListDlg::mime_data_delivered_slot);
    connect(nbtext, &QTfileTextEdit::key_press_event,
        this, &QTfilesListDlg::key_press_slot);

    return (page);
}


// Pop up the contents listing for archives.
//
bool
QTfilesListDlg::show_content()
{
    return (false);
}


void
QTfilesListDlg::set_sensitive(const char *bname, bool state)
{
    QString qs(bname);
    for (int i = 0; fl_buttons[i]; i++) {
        if (fl_buttons[i]->text() == qs) {
            fl_buttons[i]->setEnabled(state);
            break;
        }
    }
}


//
// The following functions implement polling to keep the directory
// listing current.
//

// Static private function.
// Idle procedure to update the file list.
//
int
QTfilesListDlg::fl_idle_proc(void*)
{
    for (sDirList *dl = fl_path_list->dirs(); dl; dl = dl->next()) {
        if (dl->dirty()) {
            sFileList fl(dl->dirname());
            fl.read_list(fl_path_list->checkfunc(), fl_path_list->incldirs());
            int *colwid;
            char *txt = fl.get_formatted_list(fl_path_list->columns(),
                false, fl_path_list->no_files_msg(), &colwid);
            dl->set_dirfiles(txt, colwid);
            dl->set_dirty(false);
            if (instPtr) {
                fl_update_text((QTtextEdit*)dl->dataptr(), dl->dirfiles());
                if ((QTtextEdit*)dl->dataptr() == instPtr->wb_textarea) {
                    fl_desel();
                }
            }
        }
    }
    return (false);
}


// Static private function.
// Check if directory has been modified, and set dirty flag if so.
//
int
QTfilesListDlg::fl_timer(void*)
{
    // If the cwd changes, update everything.
    char *cwd = getcwd(0, 0);
    if (cwd) {
        if (!fl_cwd || strcmp(fl_cwd, cwd)) {
            delete [] fl_cwd;
            fl_cwd = lstring::tocpp(cwd);
            if (instPtr)
                instPtr->update(fl_path_list->path_string());
            return (true);
        }
        else
            free(cwd);
    }

    bool dirtyone = false;
    for (sDirList *dl = fl_path_list->dirs(); dl; dl = dl->next()) {
        if (dl->mtime() != 0) {
            struct stat st;
            if (stat(dl->dirname(), &st) == 0 && dl->mtime() != st.st_mtime) {
                dl->set_dirty(true);
                dl->set_mtime(st.st_mtime);
                dirtyone = true;
            }
        }
    }
    if (dirtyone)
        QTdev::self()->AddIdleProc(fl_idle_proc, 0);

    return (true);
}


// Static private function.
// Set up monitoring of the directories in the path.
//
void
QTfilesListDlg::fl_monitor_setup()
{
    if (!fl_cwd)
        fl_cwd = lstring::tocpp(getcwd(0, 0));

    for (sDirList *d = fl_path_list->dirs(); d; d = d->next()) {
        struct stat st;
        if (stat(d->dirname(), &st) == 0)
            d->set_mtime(st.st_mtime);
    }
    if (!fl_timer_tag)
        fl_timer_tag = QTdev::self()->AddTimer(1000, fl_timer, 0);
}


// Static private function.
// Return true if the given path does not match the path stored in
// the files list, and at the same time update the files list.
//
bool
QTfilesListDlg::fl_check_path_and_update(const char *path)
{
    int cnt = 0;
    sDirList *dl;
    for (dl = fl_path_list->dirs(); dl; cnt++, dl = dl->next()) ;
    sDirList **array = new sDirList*[cnt];
    cnt = 0;
    for (dl = fl_path_list->dirs(); dl; cnt++, dl = dl->next())
        array[cnt] = dl;
    // cnt is number of old path elements

    sDirList *d0 = 0, *de = 0;
    if (pathlist::is_empty_path(path))
        path = ".";
    bool changed = false;
    int newcnt = 0;
    pathgen pg(path);
    char *p;
    while ((p = pg.nextpath(true)) != 0) {
        if (newcnt < cnt && array[newcnt] &&
                !strcmp(p, array[newcnt]->dirname())) {
            if (!d0)
                de = d0 = array[newcnt];
            else {
                de->set_next(array[newcnt]);
                de = de->next();
            }
            array[newcnt] = 0;
            de->set_next(0);
        }
        else {
            changed = true;
            int i;
            for (i = 0; i < cnt; i++) {
                if (array[i] && !strcmp(p, array[i]->dirname())) {
                    if (!d0)
                        de = d0 = array[i];
                    else {
                        de->set_next(array[i]);
                        de = de->next();
                    }
                    array[i] = 0;
                    de->set_next(0);
                    break;
                }
            }
            if (i == cnt) {
                // not already there, create new element
                sDirList *d = new sDirList(p);
                sFileList fl(p);
                fl.read_list(fl_path_list->checkfunc(),
                    fl_path_list->incldirs());
                int *colwid;
                char *txt = fl.get_formatted_list(fl_path_list->columns(),
                    false, fl_path_list->no_files_msg(), &colwid);
                d->set_dirfiles(txt, colwid);
                if (!d0)
                    de = d0 = d;
                else {
                    de->set_next(d);
                    de = de->next();
                }
            }
        }
        newcnt++;
        delete [] p;
    }

    for (int i = 0; i < cnt; i++) {
        if (array[i]) {
            changed = true;
            delete array[i];
        }
    }
    delete [] array;
    if (changed)
        fl_path_list->set_dirs(d0);
    return (changed);
}


// Static private function.
// Refresh the text while keeping current top location.
//
void
QTfilesListDlg::fl_update_text(QTtextEdit *text, const char *newtext)
{
    if (instPtr == 0 || text == 0 || newtext == 0)
        return;
    text->set_chars(newtext);
}


// Static function.
// Create the listing struct.
//
sPathList *
QTfilesListDlg::fl_listing(int cols)
{
    wordlist *wl = CP.VarEval(kw_sourcepath);
    char *path = wordlist::flatten(wl);
    wordlist::destroy(wl);
    sPathList *l = new sPathList(path, 0, nofiles_msg, 0, 0, cols, false);
    delete [] path;
    return (l);
}


// Static function.
void
QTfilesListDlg::fl_down_cb()
{
    TB()->PopUpFiles(MODE_OFF, 0, 0);
}


// Static function.
void
QTfilesListDlg::fl_desel()
{
    if (!QTfilesListDlg::self())
        return;
    if (!QTfilesListDlg::self()->wb_textarea)
        return;
    if (QTfilesListDlg::self()->wb_textarea->has_selection()) {
        QTfilesListDlg::self()->set_sensitive(FB_EDIT, true);
        QTfilesListDlg::self()->set_sensitive(FB_SOURCE, true);
    }
    else {
        delete [] QTfilesListDlg::self()->fl_selection;
        QTfilesListDlg::self()->fl_selection = 0;
        QTfilesListDlg::self()->set_sensitive(FB_EDIT, false);
        QTfilesListDlg::self()->set_sensitive(FB_SOURCE, false);
    }
}


void
QTfilesListDlg::button_slot(bool state)
{
    QAbstractButton *caller = qobject_cast<QAbstractButton*>(sender());
    if (!caller)
        return;
    if (!wb_textarea) {
        QTdev::Deselect(caller);
        return;
    }

    if (caller->text() == FB_EDIT) {
        if (state) {
            if (fl_selection) {
                wordlist wl1, wl;
                wl1.wl_prev = 0;
                wl1.wl_word = (char*)"-n";
                wl1.wl_next = &wl;
                wl.wl_prev = &wl1;
                wl.wl_word = Sp.FullPath(fl_selection);
                wl.wl_next = 0;
                if (wl.wl_word) {
                    CommandTab::com_edit(&wl1);
                    delete [] wl.wl_word;
                }
            }
            QTdev::SetStatus(fl_buttons[0], false);
        }
    }
    else if (caller->text() == FB_SOURCE) {
        if (state) {
            if (fl_selection) {
                wordlist wl;
                wl.wl_prev = wl.wl_next = 0;
                wl.wl_word = fl_selection;
                CommandTab::com_source(&wl);
                CP.Prompt();
            }
            QTdev::SetStatus(fl_buttons[1], false);
        }
    }
    else if (caller->text() == QString(FB_HELP)) {
        QTdev::Deselect(caller);
        TB()->PopUpHelp("filespanel");
    }
}


void
QTfilesListDlg::page_change_slot(int pg)
{
    int i = 0;
    for (sDirList *dl = fl_path_list->dirs(); dl; i++, dl = dl->next()) {
        if (i == pg && dl->dataptr()) {
            delete [] fl_directory;
            fl_directory = lstring::copy(dl->dirname());
            if (wb_textarea)
                wb_textarea->select_range(0, 0);
            wb_textarea = (QTtextEdit*)dl->dataptr();
            wb_textarea->select_range(0, 0);
            break;
        }
    }
}


void
QTfilesListDlg::menu_change_slot(int i)
{
    fl_notebook->setCurrentIndex(i);
}


void
QTfilesListDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_FIXED))
            qobject_cast<QTtextEdit*>(sender())->setFont(*fnt);
    }
}


void
QTfilesListDlg::resize_slot(QResizeEvent *ev)
{
    int fw, fh;
    if (!QTfont::stringBounds(0, FNT_FIXED, &fw, &fh))
        fw = 8;
    int cols = ev->size().width()/fw - 2;
    if (cols != fl_path_list->columns()) {
        for (sDirList *dl = fl_path_list->dirs(); dl; dl = dl->next())
            dl->set_dirty(true);
        fl_path_list->set_columns(cols);
        fl_idle_proc(0);
    }
}


void
QTfilesListDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonRelease) {
        ev->accept();
        fl_drag_start = false;
        return;
    }
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    int vsv = wb_textarea->verticalScrollBar()->value();
    int hsv = wb_textarea->horizontalScrollBar()->value();

    set_sensitive(FB_EDIT, false);
    set_sensitive(FB_SOURCE, false);

    fl_drag_start = false;
    if (!wb_textarea)
        return;
    if (wb_textarea->toPlainText() == QString(nofiles_msg))
        return;
    QByteArray qba = wb_textarea->toPlainText().toLatin1();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    int posn = wb_textarea->document()->documentLayout()->hitTest(
        QPointF(xx + hsv, yy + vsv), Qt::ExactHit);
    const char *str = lstring::copy((const char*)qba.constData());
    const char *line_start = str;
    for (int i = 0; i <= posn; i++) {
        if (str[i] == '\n') {
            if (i == posn) {
                // Clicked to  right of line.
                delete [] str;
                return;
            }
            line_start = str + i+1;
        }
    }

    const int *colwid = 0;
    for (sDirList *dl = fl_path_list->dirs(); dl; dl = dl->next()) {
        if (dl->dataptr() && !strcmp(fl_directory, dl->dirname())) {
            colwid = dl->col_width();
            break;
        }
    }
    if (!colwid) {
        delete [] str;
        return;
    }

    int cpos = &str[posn] - line_start;
    int cstart = 0;
    int cend = 0;
    for (int i = 0; colwid[i]; i++) {
        cstart = cend;
        cend += colwid[i];
        if (cpos >= cstart && cpos < cend)
            break;
    }
    if (cstart == cend || cpos >= cend) {
            delete [] str;
            return;
    }
    for (int st = 0 ; st <= cpos; st++) {
        if (line_start[st] == '\n' || line_start[st] == 0) {
            // pointing to right of line end
            delete [] str;
            return;
        }
    }

    // We know that the file name starts at cstart, find the actual
    // end.  Note that we deal with file names that contain spaces.
    for (int st = 0 ; st < cend; st++) {
        if (line_start[st] == '\n') {
            cend = st;
            break;
        }
    }

    // Omit trailing space.
    while (isspace(line_start[cend-1]))
        cend--;
    if (cpos >= cend) {
        // Clicked on trailing white space.
        delete [] str;
        return;
    }

    cstart += (line_start - str);
    cend += (line_start - str);
    delete [] str;

    if (cstart == cend) {
        select_range(wb_textarea, 0, 0);
        return;
    }
    select_range(wb_textarea, cstart, cend);
    // Don't let the scroll position change.
    wb_textarea->verticalScrollBar()->setValue(vsv);
    wb_textarea->horizontalScrollBar()->setValue(hsv);

    // The fl_selection has the full path.
    delete [] fl_selection;
    fl_selection = get_selection();

    if (fl_selection) {
        set_sensitive(FB_EDIT, true);
        set_sensitive(FB_SOURCE, true);

        fl_drag_start = true;
        fl_drag_btn = ev->button();
        fl_drag_x = xx;
        fl_drag_y = yy;
    }
}


void
QTfilesListDlg::mouse_release_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseButtonRelease) {
        ev->ignore();
        return;
    }
    if (ev->button() != Qt::LeftButton) {
        ev->ignore();
        return;
    }
    ev->accept();
    fl_drag_start = false;
}


void
QTfilesListDlg::mouse_motion_slot(QMouseEvent *ev)
{
    if (!fl_drag_start)
        return;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    if (abs(ev->position().x() - fl_drag_x) < 5 &&
            abs(ev->position().y() - fl_drag_y) < 5)
#else
    if (abs(ev->x() - fl_drag_x) < 5 && abs(ev->y() - fl_drag_y) < 5)
#endif
        return;
    fl_drag_start = false;

    char *s = get_selection();
    if (!s)
        return;
    GFTtype ft = filestat::get_file_type(s);
    QDrag *drag = new QDrag(wb_textarea);
    QMimeData *mimedata = new QMimeData();
    QList<QUrl> ulst;
    ulst << QUrl(QString("File://") + s);
    mimedata->setUrls(ulst);
    delete [] s;
    drag->setMimeData(mimedata);
    if (ft == GFT_DIR) {
        QIcon dicon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        drag->setPixmap(dicon.pixmap(32, 32));
    }
    else {
        QIcon ficon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
        drag->setPixmap(ficon.pixmap(32, 32));
    }

    Qt::KeyboardModifiers m = QGuiApplication::queryKeyboardModifiers();
#ifdef Q_OS_MACOS
    // alt == option on Apple's planet
    if ((m & Qt::ShiftModifier) && (m & Qt::AltModifier)) {
        drag->exec(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction,
            Qt::LinkAction);
    }
    else if (m & Qt::ShiftModifier) {
        drag->exec(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction,
            Qt::CopyAction);
    }
    else {
        drag->exec(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction,
            Qt::MoveAction);
    }
#else
    if ((m & Qt::ShiftModifier) && (m & Qt::ControlModifier)) {
        drag->exec(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction,
            Qt::LinkAction);
    }
    else if (m & Qt::ShiftModifier) {
        drag->exec(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction,
            Qt::CopyAction);
    }
    else {
        drag->exec(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction,
            Qt::MoveAction);
    }
#endif
}


void
QTfilesListDlg::mime_data_handled_slot(const QMimeData *dta, int *accpt) const
{
    if (dta->hasFormat("text/twostring") || dta->hasFormat("text/plain"))
        *accpt = true;
}


void
QTfilesListDlg::mime_data_delivered_slot(const QMimeData *dta, int *accpt)
{
    if (dta->hasFormat("text/twostring") || dta->hasFormat("text/plain")) {
        *accpt = true;
        const char *dst = fl_directory;
        if (!dst || !*dst)
            return;  // sanity
        int proposed_action = wb_textarea->drop_action();
        QTfileDlg::ActionType a = QTfileDlg::A_NOOP;
        if (proposed_action & Qt::CopyAction)
            a = QTfileDlg::A_COPY;
        else if (proposed_action & Qt::MoveAction)
            a = QTfileDlg::A_MOVE;
        else if (proposed_action & Qt::LinkAction)
            a = QTfileDlg::A_LINK;
        else
            return;

        // Handles URLs, text/twostring, and regular strings.
        if (dta->hasUrls()) {
            foreach (const QUrl &url, dta->urls()) {
                QByteArray fnba = url.toLocalFile().toLatin1();
                const char *src = fnba.constData();
                if (!src || !*src)
                    continue;
                if (!strcmp(src, dst))
                    continue;
                QTfileDlg::DoFileAction(this, src, dst, a);
            }
        }
        else {
            QByteArray bary = dta->data("text/plain");
            const char *src = bary.constData();
            if (src && *src) {
                if (!strncmp(src, "File://", 7))
                    src += 7;
                char *pth = lstring::copy(src);
                char *t = strchr(pth, '\n');
                if (t) {
                    // text/twostring, keep the first token only.
                    *t = 0;
                }
                if (strcmp(pth, dst))
                    QTfileDlg::DoFileAction(this, pth, dst, a);
                delete [] pth;
            }
        }
    }
}


void
QTfilesListDlg::key_press_slot(QKeyEvent *ev)
{
    // Accept Ctrl-C as a copy operation, this is only done in
    // read-only mode.  Note that the QTtextEdit was modified to send
    // these.
    QTtextEdit *w = qobject_cast<QTtextEdit*>(sender());
    if (!w)
        return;
    if (!w->isReadOnly())
        return;
    if (ev->key() == Qt::Key_C && (ev->modifiers() & Qt::ControlModifier)) {
        ev->accept();
        w->copy();
    }
}


void
QTfilesListDlg::dismiss_btn_slot()
{
    TB()->PopUpFiles(MODE_OFF, 0, 0);
}

