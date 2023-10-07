
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
#include <QPushButton>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QDrag>
#include <QMimeData>
#include <QAction>
#include <QApplication>


//===========================================================================
// Pop up to display a listing of files found along the 'sourcepath'.
// Buttons:
//  Dismiss: pop down
//  Source: source the circuit pointed to while active
//  Edit:   open the editor with file pointed to while active
//  Help:   bring up help panel

// Keywords referenced in help database:
//  filespanel


// It can take a while to process the files, unfortunately the "busy"
// cursor seems to never appear with the standard logic.  In order to
// make the busy cursor appear, had to use a timeout as below.

/*
namespace {
    int msw_timeout(void *caller)
    {
        new QTfilesListDlg(caller);

        QTdev::self()->SetPopupLocation(GRloc(), QTfilesListDlg::self(),
            QTmainwin::self()->Viewport());
        QTfilesListDlg::self()->show();

        QTpkg::self()->SetWorking(false);
        return (0);
    }
}
*/


void
QTtoolbar::PopUpFiles(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        if (QTfilesListDlg::self())
            QTfilesListDlg::self()->deleteLater();
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
sPathList *QTfilesListDlg::f_path_list;
char *QTfilesListDlg::f_cwd;
int QTfilesListDlg::f_timer_tag;
QTfilesListDlg *QTfilesListDlg::instPtr;

QTfilesListDlg::QTfilesListDlg(int x, int y)
{
    fl_caller = TB()->entries(tid_files)->action();;
    instPtr = this;

    for (int i = 0; i < MAX_BTNS; i++)
        f_buttons[i] = 0;
    f_button_box = 0;
    f_menu = 0;
    f_notebook = 0;
    f_start = 0;
    f_end = 0;
    f_drag_start = false;
    f_drag_btn = 0;
    f_drag_x = 0;
    f_drag_y = 0;
    f_directory = 0;

    fl_selection = 0;
    fl_noupdate = 0;

    setWindowTitle(tr("Search Path Files Listing"));
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    // Create the layout for the button row.
    f_button_box = new QHBoxLayout();
    vbox->addLayout(f_button_box);
    f_button_box->setMargin(0);
    f_button_box->setSpacing(2);

    // title label
    //
    QGroupBox *gb = new QGroupBox();
    vbox->addWidget(gb);
    QHBoxLayout *hbox = new QHBoxLayout(gb);
    hbox->setMargin(2);
    hbox->setSpacing(2);

    QLabel *label = new QLabel(tr(files_msg));
    label->setAlignment(Qt::AlignCenter);
    hbox->addWidget(label);

    int fw, fh;
    if (!QTfont::stringBounds(0, FNT_FIXED, &fw, &fh))
        fw = 8;
    int cols = (sizeHint().width()-8)/fw - 2;
    if (!f_path_list) {
        f_path_list = fl_listing(cols);
        f_monitor_setup();
    }
    else if (cols != f_path_list->columns()) {
        for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next())
            dl->set_dirty(true);
        f_path_list->set_columns(cols);
        f_idle_proc(0);
    }

    f_menu = new QComboBox();
    vbox->addWidget(f_menu);

    // creates notebook
    init_viewing_area();
    vbox->addWidget(f_notebook);

    // dismiss button
    //
    QPushButton *btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();

    TB()->FixLoc(&x, &y);
    TB()->SetActiveDlg(tid_files, this);
    move(x, y);
}


QTfilesListDlg::~QTfilesListDlg()
{
    instPtr = 0;
    if (f_path_list) {
        for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next())
            dl->set_dataptr(0);
    }
    delete [] f_directory;

    if (fl_caller)
        QTdev::Deselect(fl_caller);
    delete [] fl_selection;
    TB()->SetLoc(tid_files, this);
    TB()->SetActiveDlg(tid_files, 0);
    QTtoolbar::entries(tid_files)->action()->setChecked(false);
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
    fl_desel(0);
}


// Update the directory listings.
//
void
QTfilesListDlg::update(const char *path, const char **buttons, int numbuttons)
{
    if (path && f_path_list) {
        stringlist *s0 = 0, *se = 0;
        for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next()) {
            if (!s0)
                s0 = se = new stringlist(lstring::copy(dl->dirname()), 0);
            else {
                se->next = new stringlist(lstring::copy(dl->dirname()), 0);
                se = se->next;
            }
        }
        if (f_check_path_and_update(path))
            relist(s0);
        stringlist::destroy(s0);
    }

    if (numbuttons > 0 && buttons) {
        if (numbuttons > MAX_BTNS)
            numbuttons = MAX_BTNS;

        for (int i = 0; f_buttons[i]; i++) {
            f_button_box->removeWidget(f_buttons[i]);
            f_buttons[i]->deleteLater();
            f_buttons[i] = 0;
        }
        for (int i = 0; i < numbuttons; i++) {
            QPushButton *btn = new QPushButton(tr(buttons[i]));
            f_button_box->addWidget(btn);
            btn->setCheckable(true);
            f_buttons[i] = btn;
            connect(btn, SIGNAL(toggled(bool)),
                this, SLOT(button_slot(bool)));
        }
    }
}


// Return the full path name of the selected file, or 0 if no
// selection.
//
char *
QTfilesListDlg::get_selection()
{
    if (!f_directory || !*f_directory)
        return (0);
    char *s = wb_textarea->get_selection();
    if (s) {
        if (*s) {
            sLstr lstr;
            lstr.add(f_directory);
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
    if (f_path_list) {
        for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next())
            dl->set_dataptr(0);
    }
    if (!f_notebook)
        f_notebook = new QStackedWidget();
    if (!f_path_list)
        return;

    int init_page = 0;
    int maxchars = 120;
    if (f_path_list && f_directory) {
        int i = 0;
        for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next()) {
            if (!strcmp(f_directory, dl->dirname())) {
                init_page = i;
                break;
            }
            i++;
        }
    }
    delete [] f_directory;
    f_directory = 0;

    char buf[256];
    int i = 0;
    for (sDirList *dl = f_path_list->dirs(); dl; i++, dl = dl->next()) {
        if (i == init_page)
            f_directory = lstring::copy(dl->dirname());

        int len = strlen(dl->dirname());
        if (len <= maxchars)
            strcpy(buf, dl->dirname());
        else {
            int partchars = maxchars/2 - 2;
            strncpy(buf, dl->dirname(), partchars);
            strcpy(buf + partchars, " ... ");
            strcat(buf, dl->dirname() + len - partchars);
        }
        f_menu->addItem(buf);

        create_page(dl);
        QTtextEdit *nbtext = (QTtextEdit*)dl->dataptr();
        if (nbtext) {
            f_notebook->addWidget(nbtext);
        }
        if (i == init_page)
            wb_textarea = nbtext;
    }
    connect(f_notebook, SIGNAL(currentChanged(int)),
        this, SLOT(page_change_slot(int)));

    f_notebook->setCurrentIndex(init_page);
    f_menu->setCurrentIndex(init_page);
    connect(f_menu, SIGNAL(currentIndexChanged(int)),
        this, SLOT(menu_change_slot(int)));
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


// Reset the notebook listings.  The f_path_list has already been set. 
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
    for (sDirList *dl = f_path_list->dirs(); dl; n++, dl = dl->next()) {
        int oldn = findstr(dl->dirname(), ary, len);
        if (oldn == n)
            continue;
        if (oldn > 0) {
            // Directory moved to a new location in the path.  Make
            // the corresponding change to the notebook, menu, and the
            // array.

            QWidget *pg = f_notebook->widget(oldn);
            f_notebook->removeWidget(pg);
            f_notebook->insertWidget(n, pg);

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
        f_notebook->insertWidget(n, pg);

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

    while (f_notebook->widget(n) != 0)
        f_notebook->removeWidget(f_notebook->widget(n));

    // Clear the combo box.
    f_menu->clear();

    for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next()) {
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
        f_menu->addItem(buf);
    }
    n = f_notebook->currentIndex();
    f_menu->setCurrentIndex(n);
}


// Select the chars in the range, start=end deselects existing.
//
void
QTfilesListDlg::select_range(QTtextEdit *caller, int start, int end)
{
    caller->select_range(start, end);
    f_start = start;
    f_end = end;
//    if (f_desel)
//        (*f_desel)(this);
}


QWidget *
QTfilesListDlg::create_page(sDirList *dl)
{
    QWidget *page = new QWidget();
    QVBoxLayout *vbox = new QVBoxLayout(page);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    // scrolled text area
    //
    QTtextEdit *nbtext = new QTtextEdit();
    vbox->addWidget(nbtext);
    nbtext->setReadOnly(true);
    nbtext->setMouseTracking(true);
    nbtext->setAcceptDrops(true);
    dl->set_dataptr(nbtext);
    nbtext->set_chars(dl->dirfiles());

    QFont *tfont;
    if (FC.getFont(&tfont, FNT_SCREEN))
        nbtext->setFont(*tfont);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)));

    connect(nbtext, SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));
    connect(nbtext, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));
    connect(nbtext, SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(mouse_motion_slot(QMouseEvent*)));
    connect(nbtext, SIGNAL(mime_data_received(const QMimeData*)),
        this, SLOT(mime_data_received_slot(const QMimeData*)));
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
    for (int i = 0; f_buttons[i]; i++) {
        if (f_buttons[i]->text() == qs) {
            f_buttons[i]->setEnabled(state);
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
QTfilesListDlg::f_idle_proc(void*)
{
    for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next()) {
        if (dl->dirty()) {
            sFileList fl(dl->dirname());
            fl.read_list(f_path_list->checkfunc(), f_path_list->incldirs());
            int *colwid;
            char *txt = fl.get_formatted_list(f_path_list->columns(),
                false, f_path_list->no_files_msg(), &colwid);
            dl->set_dirfiles(txt, colwid);
            dl->set_dirty(false);
            if (instPtr) {
                f_update_text((QTtextEdit*)dl->dataptr(), dl->dirfiles());
                if ((QTtextEdit*)dl->dataptr() == instPtr->wb_textarea) {
                    /* XXX
                    if (instPtr->f_desel)
                        (*instPtr->f_desel)(instPtr);
                        */
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
QTfilesListDlg::f_timer(void*)
{
    // If the cwd changes, update everything
    char *cwd = getcwd(0, 0);
    if (cwd) {
        if (!f_cwd || strcmp(f_cwd, cwd)) {
            delete [] f_cwd;
            f_cwd = lstring::tocpp(cwd);
            if (instPtr)
                instPtr->update(f_path_list->path_string());
            return (true);
        }
        else
            free(cwd);
    }

    bool dirtyone = false;
    for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next()) {
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
        QTdev::self()->AddIdleProc(f_idle_proc, 0);

    return (true);
}


// Static private function.
// Set up monitoring of the directories in the path.
//
void
QTfilesListDlg::f_monitor_setup()
{
    if (!f_cwd)
        f_cwd = lstring::tocpp(getcwd(0, 0));

    for (sDirList *d = f_path_list->dirs(); d; d = d->next()) {
        struct stat st;
        if (stat(d->dirname(), &st) == 0)
            d->set_mtime(st.st_mtime);
    }
    if (!f_timer_tag)
        f_timer_tag = QTdev::self()->AddTimer(1000, f_timer, 0);
}


// Static private function.
// Return true if the given path does not match the path stored in
// the files list, and at the same time update the files list.
//
bool
QTfilesListDlg::f_check_path_and_update(const char *path)
{
    int cnt = 0;
    sDirList *dl;
    for (dl = f_path_list->dirs(); dl; cnt++, dl = dl->next()) ;
    sDirList **array = new sDirList*[cnt];
    cnt = 0;
    for (dl = f_path_list->dirs(); dl; cnt++, dl = dl->next())
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
                fl.read_list(f_path_list->checkfunc(),
                    f_path_list->incldirs());
                int *colwid;
                char *txt = fl.get_formatted_list(f_path_list->columns(),
                    false, f_path_list->no_files_msg(), &colwid);
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
        f_path_list->set_dirs(d0);
    return (changed);
}


// Static private function.
// Refresh the text while keeping current top location.
//
void
QTfilesListDlg::f_update_text(QTtextEdit *text, const char *newtext)
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
QTfilesListDlg::fl_down_cb(void*)
{
    TB()->PopUpFiles(MODE_OFF, 0, 0);
}


// Static function.
void
QTfilesListDlg::fl_desel(void*)
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
    QPushButton *caller = qobject_cast<QPushButton*>(sender());
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
            QTdev::SetStatus(f_buttons[0], false);
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
            QTdev::SetStatus(f_buttons[1], false);
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
    for (sDirList *dl = f_path_list->dirs(); dl; i++, dl = dl->next()) {
        if (i == pg && dl->dataptr()) {
            delete [] f_directory;
            f_directory = lstring::copy(dl->dirname());
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
    f_notebook->setCurrentIndex(i);
}


void
QTfilesListDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_FIXED))
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
    if (cols != f_path_list->columns()) {
        for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next())
            dl->set_dirty(true);
        f_path_list->set_columns(cols);
        f_idle_proc(0);
    }
}


void
QTfilesListDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonRelease) {
        ev->accept();
        f_drag_start = false;
        return;
    }
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    set_sensitive(FB_EDIT, false);
    set_sensitive(FB_SOURCE, false);

    f_drag_start = false;
    if (!wb_textarea)
        return;
    if (wb_textarea->toPlainText() == QString(nofiles_msg))
        return;
    QByteArray qba = wb_textarea->toPlainText().toLatin1();
    int x = ev->x();
    int y = ev->y();
    QTextCursor cur = wb_textarea->cursorForPosition(QPoint(x, y));
    int pos = cur.position();
    const char *str = lstring::copy((const char*)qba.constData());
    const char *line_start = str;
    for (int i = 0; i <= pos; i++) {
        if (str[i] == '\n') {
            if (i == pos) {
                // Clicked to  right of line.
                delete [] str;
                return;
            }
            line_start = str + i+1;
        }
    }

    const int *colwid = 0;
    for (sDirList *dl = f_path_list->dirs(); dl; dl = dl->next()) {
        if (dl->dataptr() && !strcmp(f_directory, dl->dirname())) {
            colwid = dl->col_width();
            break;
        }
    }
    if (!colwid) {
        delete [] str;
        return;
    }

    int cpos = &str[pos] - line_start;
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

    // The fl_selection has the full path.
    delete [] fl_selection;
    fl_selection = get_selection();

    if (fl_selection) {
        set_sensitive(FB_EDIT, true);
        set_sensitive(FB_SOURCE, true);

        f_drag_start = true;
        f_drag_btn = ev->button();
        f_drag_x = x;
        f_drag_y = y;
    }
}


void
QTfilesListDlg::mouse_motion_slot(QMouseEvent *ev)
{
    if (!f_drag_start)
        return;
    if (abs(ev->x() - f_drag_x) < 5 && abs(ev->y() - f_drag_y) < 5)
        return;
    f_drag_start = false;

    char *s = get_selection();
    if (!s)
        return;
    GFTtype ft = filestat::get_file_type(s);
    int sz = strlen(s) + 1;
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
#ifdef __APPLE__
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
    delete drag;
}


void
QTfilesListDlg::mime_data_received_slot(const QMimeData *data)
{
    foreach (const QUrl &url, data->urls()) {
        QByteArray bary = url.toLocalFile().toLatin1();
        const char *src = bary.constData();
        if (src && *src && instPtr->wb_textarea) {
            const char *dst = f_directory;
            if (dst && *dst && strcmp(src, dst)) {
                QTfileDlg::DoFileAction(this, src, dst, QTfileDlg::A_NOOP);
                return;
            }
        }
    }
}


void
QTfilesListDlg::dismiss_btn_slot()
{
    TB()->PopUpFiles(MODE_OFF, 0, 0);
}

