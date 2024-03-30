
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

#include "qtfiles.h"
#include "cvrt.h"
#include "editif.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_library.h"
#include "fio_alias.h"
#include "fio_chd.h"
#include "events.h"
#include "errorlog.h"
#include "qtinterf/qtmcol.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qtfile.h"
#include "qtinterf/qttextw.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include <sys/stat.h>

#include <QApplication>
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
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>


//-----------------------------------------------------------------------------
// QTfilesListDlg::  Files Listing dialog, list files found in
// directories in the search path.
// Called from main menu: File/Files List.
//
// Help system keywords used:
//  filespanel

// Static function.
//
char *
QTmainwin::get_file_selection()
{
    if (QTfilesListDlg::self())
        return (QTfilesListDlg::self()->get_selection());
    return (0);
}


// Static function.
// Called on crash to prevent updates.
//
void
QTmainwin::files_panic()
{
    QTfilesListDlg::panic();
}
// End of QTmainwin functions.


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


// It can take a while to process the files, unfortunately the "busy"
// cursor seems to never appear with the standard logic.  In order to
// make the busy cursor appear, had to use a timeout as below.

#define FILES_TIMEOUT
#ifdef FILES_TIMEOUT
namespace {
    int msw_timeout(void *caller)
    {
        new QTfilesListDlg(caller);

        QTfilesListDlg::self()->set_transient_for(QTmainwin::self());
        QTdev::self()->SetPopupLocation(GRloc(), QTfilesListDlg::self(),
            QTmainwin::self()->Viewport());
        QTfilesListDlg::self()->show();

        QTpkg::self()->SetWorking(false);
        return (0);
    }
}
#endif


void
cConvert::PopUpFiles(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
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

    // This is needed to reliably show the busy cursor.
#ifdef FILES_TIMEOUT
    QTpkg::self()->SetWorking(true);
    QTpkg::self()->RegisterTimeoutProc(500, msw_timeout, caller);

#else
    new QTfilesListDlg(caller);

    QTfilesListDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTfilesListDlg::self(),
        QTmainwin::self()->Viewport());
    QTfilesListDlg::self()->show();
#endif
}
// End of cConvert functions.


// The buttons displayed for various modes:
// no editing:      view, content, help
// displaying chd:  content, help
// normal mode:     edit, master, content, help

#define FB_OPEN     "Open"
#define FB_PLACE    "Place"
#define FB_CONTENT  "Content"
#define FB_HELP     "Help"


const char *QTfilesListDlg::nofiles_msg = "  no recognized files found\n";
const char *QTfilesListDlg::files_msg =
    "Files from search path.\nTypes: B CGX, C CIF, "
#ifdef HANDLE_SCED
    "G GDSII, L library, O OASIS, S JSPICE3 (sced), X Xic";
#else
    "G GDSII, L library, O OASIS, X Xic";
#endif

// The directories in the path are monitored for changes.
//
sPathList *QTfilesListDlg::fl_path_list;
char *QTfilesListDlg::fl_cwd;
int QTfilesListDlg::fl_timer_tag;
QTfilesListDlg *QTfilesListDlg::instPtr;

QTfilesListDlg::QTfilesListDlg(GRobject c) : QTbag(this)
{
    instPtr = this;
    fl_caller = c;
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
    fl_contlib = 0;
    fl_content_pop = 0;
    fl_chd = 0;
    fl_noupdate = 0;

    setWindowTitle(tr("Path Files Listing"));
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
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
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
    if (fl_content_pop)
        fl_content_pop->popdown();
    delete [] fl_selection;
    delete [] fl_contlib;
    delete fl_chd;
}


void
QTfilesListDlg::update()
{
    if (fl_noupdate) {
        fl_noupdate++;
        return;
    }
    if (fl_content_pop) {
        if (DSP()->MainWdesc()->DbType() == WDchd)
            fl_content_pop->set_button_sens(0);
        else
            fl_content_pop->set_button_sens(-1);
    }

    const char *btns[5];
    int nbtns;
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        btns[0] = FB_CONTENT;
        btns[1] = FB_HELP;
        nbtns = 2;
    }
    else if (!EditIf()->hasEdit()) {
        btns[0] = FB_OPEN;
        btns[1] = FB_CONTENT;
        btns[2] = FB_HELP;
        nbtns = 3;
    }
    else {
        btns[0] = FB_OPEN;
        btns[1] = FB_PLACE;
        btns[2] = FB_CONTENT;
        btns[3] = FB_HELP;
        nbtns = 4;
    }
    update(FIO()->PGetPath(), btns, nbtns);
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
            QPushButton *btn = new QPushButton(tr(buttons[i]));
            fl_button_box->addWidget(btn);
            btn->setCheckable(true);
            btn->setAutoDefault(false);
            fl_buttons[i] = btn;
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
    if (fl_contlib && fl_content_pop) {
        char *sel = fl_content_pop->get_selection();
        if (sel) {
            int len = strlen(fl_contlib) + strlen(sel) + 2;
            char *tbuf = new char[len];
            char *t = lstring::stpcpy(tbuf, fl_contlib);
            *t++ = ' ';
            strcpy(t, sel);
            delete [] sel;
            return (tbuf);
        }
    }

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
    connect(fl_notebook, SIGNAL(currentChanged(int)),
        this, SLOT(page_change_slot(int)));

    fl_notebook->setCurrentIndex(init_page);
    fl_menu->setCurrentIndex(init_page);
    connect(fl_menu, SIGNAL(currentIndexChanged(int)),
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
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)));

    connect(nbtext, SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));
    connect(nbtext, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));
    connect(nbtext, SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(mouse_release_slot(QMouseEvent*)));
    connect(nbtext, SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(mouse_motion_slot(QMouseEvent*)));
    connect(nbtext, SIGNAL(mime_data_handled(const QMimeData*, int*)),
        this, SLOT(mime_data_handled_slot(const QMimeData*, int*)));
    connect(nbtext, SIGNAL(mime_data_delivered(const QMimeData*, int*)),
        this, SLOT(mime_data_delivered_slot(const QMimeData*, int*)));
    connect(nbtext, SIGNAL(key_press_event(QKeyEvent*)),
        this, SLOT(key_press_slot(QKeyEvent*)));

    return (page);
}


// Pop up the contents listing for archives.
//
bool
QTfilesListDlg::show_content()
{
    if (!fl_selection)
        return (false);

    FILE *fp = fopen(fl_selection, "rb");
    if (!fp)
        return (false);

    FileType filetype = Fnone;
    CFtype ct;
    bool issc = false;
    if (FIO()->IsGDSII(fp))  // must be first!
        filetype = Fgds;
    else if (FIO()->IsCGX(fp))
        filetype = Fcgx;
    else if (FIO()->IsOASIS(fp))
        filetype = Foas;
    else if (FIO()->IsLibrary(fp))
        filetype = Fnative;
    else if (FIO()->IsCIF(fp, &ct, &issc) && ct != CFnative && !issc)
        filetype = Fcif;
    fclose(fp);

    if (fl_chd && strcmp(fl_chd->filename(), fl_selection)) {
        delete fl_chd;
        fl_chd = 0;
    }

    if (filetype == Fnone) {
        stringlist *list = new stringlist(
            lstring::copy("Selected file is not an archive."), 0);
        sLstr lstr;
        lstr.add("Regular file\n");
        lstr.add(fl_selection);

        delete [] fl_contlib;
        fl_contlib = 0;

        if (fl_content_pop)
            fl_content_pop->update(list, lstr.string());
        else {
            const char *buttons[3];
            buttons[0] = FB_OPEN;
            buttons[1] = EditIf()->hasEdit() ? FB_PLACE : 0;
            buttons[2] = 0;

            int pagesz = 0;
            const char *s = CDvdb()->getVariable(VA_ListPageEntries);
            if (s) {
                pagesz = atoi(s);
                if (pagesz < 100 || pagesz > 50000)
                    pagesz = 0;
            }
            fl_content_pop = DSPmainWbagRet(PopUpMultiCol(list, lstr.string(),
                fl_content_cb, 0, buttons, pagesz));
            if (fl_content_pop) {
                fl_content_pop->register_usrptr((void**)&fl_content_pop);
                if (DSP()->MainWdesc()->DbType() == WDchd)
                    fl_content_pop->set_button_sens(0);
                else
                    fl_content_pop->set_button_sens(-1);
            }
        }
        stringlist::destroy(list);
        return (true);
    }
    else if (filetype == Fnative) {
        // library
        if (FIO()->OpenLibrary(0, fl_selection)) {
            stringlist *list = FIO()->GetLibNamelist(fl_selection, LIBuser);
            if (list) {
                sLstr lstr;
                lstr.add("References found in library - click to select\n");
                lstr.add(fl_selection);

                delete [] fl_contlib;
                fl_contlib = lstring::copy(fl_selection);

                if (fl_content_pop)
                    fl_content_pop->update(list, lstr.string());
                else {
                    const char *buttons[3];
                    buttons[0] = FB_OPEN;
                    buttons[1] = EditIf()->hasEdit() ? FB_PLACE : 0;
                    buttons[2] = 0;

                    int pagesz = 0;
                    const char *s = CDvdb()->getVariable(VA_ListPageEntries);
                    if (s) {
                        pagesz = atoi(s);
                        if (pagesz < 100 || pagesz > 50000)
                            pagesz = 0;
                    }
                    fl_content_pop = DSPmainWbagRet(PopUpMultiCol(list,
                        lstr.string(), fl_content_cb, 0, buttons, pagesz));
                   if (fl_content_pop) {
                        fl_content_pop->register_usrptr(
                            (void**)&fl_content_pop);
                        if (DSP()->MainWdesc()->DbType() == WDchd)
                            fl_content_pop->set_button_sens(0);
                        else
                            fl_content_pop->set_button_sens(-1);
                   }
                }
                stringlist::destroy(list);
                return (true);
            }
        }
    }
    else if (FIO()->IsSupportedArchiveFormat(filetype)) {
        if (!fl_chd) {
            unsigned int alias_mask = (CVAL_CASE | CVAL_PFSF | CVAL_FILE);
            FIOaliasTab *tab = FIO()->NewReadingAlias(alias_mask);
            if (tab)
                tab->read_alias(fl_selection);
            fl_chd = FIO()->NewCHD(fl_selection, filetype, Electrical, tab,
                cvINFOplpc);
            delete tab;
        }
        if (fl_chd) {
            stringlist *list = fl_chd->listCellnames(-1, false);
            sLstr lstr;
            lstr.add("Cells found in ");
            lstr.add(FIO()->TypeName(filetype));
            lstr.add(" file - click to select\n");
            lstr.add(fl_selection);

            delete [] fl_contlib;
            fl_contlib = lstring::copy(fl_selection);

            if (fl_content_pop)
                fl_content_pop->update(list, lstr.string());
            else {
                const char *buttons[3];
                buttons[0] = FB_OPEN;
                buttons[1] = EditIf()->hasEdit() ? FB_PLACE : 0;
                buttons[2] = 0;

                int pagesz = 0;
                const char *s = CDvdb()->getVariable(VA_ListPageEntries);
                if (s) {
                    pagesz = atoi(s);
                    if (pagesz < 100 || pagesz > 50000)
                        pagesz = 0;
                }
                fl_content_pop = DSPmainWbagRet(PopUpMultiCol(list,
                    lstr.string(), fl_content_cb, 0, buttons, pagesz));
                if (fl_content_pop) {
                    fl_content_pop->register_usrptr((void**)&fl_content_pop);
                    if (DSP()->MainWdesc()->DbType() == WDchd)
                        fl_content_pop->set_button_sens(0);
                    else
                        fl_content_pop->set_button_sens(-1);
                }
            }
            stringlist::destroy(list);
            return (true);
        }
        else {
            Log()->ErrorLogV(mh::Processing,
                "Content scan failed: %s", Errs()->get_error());
        }

    }
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
    QTpkg::self()->SetWorking(true);
    sPathList *l = new sPathList(FIO()->PGetPath(), fl_is_symfile, nofiles_msg,
        0, 0, cols, false);
    QTpkg::self()->SetWorking(false);
    return (l);
}


// Static function.
// Test condition for files to be listed.
// Return non-null if a known layout file.
//
char*
QTfilesListDlg::fl_is_symfile(const char *fname)
{
    FILE *fp = fopen(fname, "rb");
    if (!fp)
        return (0);
    fname = lstring::strip_path(fname);
    char buf[128];
    // *** The GDSII test MUST come first
    if (FIO()->IsGDSII(fp)) {
        // GDSII
        snprintf(buf, sizeof(buf), "G %s", fname);
        fclose(fp);
        return (lstring::copy(buf));
    }
    if (FIO()->IsCGX(fp)) {
        // CGX
        snprintf(buf, sizeof(buf), "B %s", fname);
        fclose(fp);
        return (lstring::copy(buf));
    }
    if (FIO()->IsOASIS(fp)) {
        // OASIS
        snprintf(buf, sizeof(buf), "O %s", fname);
        fclose(fp);
        return (lstring::copy(buf));
    }
    if (FIO()->IsLibrary(fp)) {
        // Library
        snprintf(buf, sizeof(buf), "L %s", fname);
        fclose(fp);
        return (lstring::copy(buf));
    }
    CFtype type;
    bool issced;
    if (FIO()->IsCIF(fp, &type, &issced)) {
        // CIF, XIC
        snprintf(buf, sizeof(buf), "  %s", fname);
        if (type == CFnative)
            buf[0] = 'X';
        else
            buf[0] = 'C';
        if (issced) {
#ifdef HANDLE_SCED
            buf[0] = 'S';
#else
            fclose(fp);
            return (0);
#endif
        }
        fclose(fp);
        return (lstring::copy(buf));
    }
    fclose(fp);
    return (0);
}


// Static function.
void
QTfilesListDlg::fl_content_cb(const char *cellname, void*)
{
    if (!QTfilesListDlg::self())
        return;
    if (!QTfilesListDlg::self()->fl_contlib ||
            !QTfilesListDlg::self()->fl_content_pop)
        return;
    if (!cellname)
        return;
    if (*cellname != '/')
        return;

    cellname++;
    if (!strcmp(cellname, FB_OPEN)) {
        char *sel = QTfilesListDlg::self()->fl_content_pop->get_selection();
        if (sel) {
            EV()->InitCallback();
            XM()->EditCell(QTfilesListDlg::self()->fl_contlib, false,
                FIO()->DefReadPrms(), sel, QTfilesListDlg::self()->fl_chd);
            delete [] sel;
        }
    }
    else if (!strcmp(cellname, FB_PLACE)) {
        char *sel = QTfilesListDlg::self()->fl_content_pop->get_selection();
        if (sel) {
            EV()->InitCallback();
            EditIf()->addMaster(QTfilesListDlg::self()->fl_contlib, sel,
                QTfilesListDlg::self()->fl_chd);
            delete [] sel;
        }
    }
}


// Static function.
void
QTfilesListDlg::fl_down_cb()
{
    Cvt()->PopUpFiles(0, MODE_OFF);
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
        QTfilesListDlg::self()->set_sensitive(FB_OPEN, true);
        QTfilesListDlg::self()->set_sensitive(FB_PLACE, true);
        QTfilesListDlg::self()->set_sensitive(FB_CONTENT, true);
    }
    else {
        delete [] QTfilesListDlg::self()->fl_selection;
        QTfilesListDlg::self()->fl_selection = 0;
        QTfilesListDlg::self()->set_sensitive(FB_OPEN, false);
        QTfilesListDlg::self()->set_sensitive(FB_PLACE, false);
        QTfilesListDlg::self()->set_sensitive(FB_CONTENT, false);
    }
}


void
QTfilesListDlg::button_slot(bool)
{
    QPushButton *caller = qobject_cast<QPushButton*>(sender());
    if (!caller)
        return;
    if (!wb_textarea) {
        QTdev::Deselect(caller);
        return;
    }

    // Note:  during open and place, callbacks to update are deferred. 
    // These are generated when the path changes when reading native
    // cells.
    //
    if (caller->text() == QString(FB_OPEN)) {
        QTdev::Deselect(caller);
        if (fl_selection) {
            fl_noupdate = 1;
            EV()->InitCallback();
            XM()->EditCell(fl_selection, false, FIO()->DefReadPrms(), 0, 0);
            if (QTfilesListDlg::self()) {
                if (fl_noupdate > 1) {
                    update(FIO()->PGetPath(), 0, 0);
                    fl_desel();
                }
                fl_noupdate = 0;
            }
        }
    }
    else if (caller->text() == QString(FB_PLACE)) {
        QTdev::Deselect(caller);
        if (fl_selection) {
            fl_noupdate = 1;
            EV()->InitCallback();
            EditIf()->addMaster(fl_selection, 0, 0);
            if (QTfilesListDlg::self()) {
                if (fl_noupdate > 1) {
                    update(FIO()->PGetPath(), 0, 0);
                    fl_desel();
                }
                fl_noupdate = 0;
            }
        }
    }
    else if (caller->text() == QString(FB_CONTENT)) {
        QTdev::Deselect(caller);
        show_content();
    }
    else if (caller->text() == QString(FB_HELP)) {
        QTdev::Deselect(caller);
        DSPmainWbag(PopUpHelp("filespanel"))
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
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    if (ev->button() != Qt::LeftButton) {
        ev->ignore();
        return;
    }
    ev->accept();

    int vsv = wb_textarea->verticalScrollBar()->value();
    int hsv = wb_textarea->horizontalScrollBar()->value();

    set_sensitive(FB_OPEN, false);
    set_sensitive(FB_PLACE, false);
    set_sensitive(FB_CONTENT, false);

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

    // Skip over the filetype indicator, which is a single character
    // and a space separator.
    char code = line_start[cstart];
    cstart += 2;

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
        set_sensitive(FB_OPEN, true);
        set_sensitive(FB_PLACE, true);
        if (strchr("GBOCL", code))
            set_sensitive(FB_CONTENT, true);

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
    mimedata->setText(s);
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
}


void
QTfilesListDlg::mime_data_handled_slot(const QMimeData *dta, int *accpt) const
{
    if (dta->hasUrls() || dta->hasFormat("text/twostring"))
        *accpt = 1;
    else
        *accpt = -1;
}


void
QTfilesListDlg::mime_data_delivered_slot(const QMimeData *dta, int *accpt)
{
    if (dta->hasUrls() || dta->hasFormat("text/twostring")) {
        *accpt = 1;

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

        const char *dst = fl_directory;
        if (!dst || !*dst)
            return;  // sanity

        if (dta->hasUrls()) {
            foreach (const QUrl &url, dta->urls()) {
                QByteArray fnba = url.toLocalFile().toLatin1();
                const char *src = fnba.constData();
                if (!src || !*src || !strcmp(src, dst))
                    continue;

                QTfileDlg::DoFileAction(this, src, dst, a);
            }
        }
        else if (dta->hasFormat("text/twostring")) {
            QByteArray bary = dta->data("text/twostring");
            const char *src = bary.constData();
            char *pth = lstring::copy(src);
            char *t = strchr(pth, '\n');
            if (t) {
                // text/twostring, keep the first token only.
                *t = 0;
            }
            if (strcmp(pth, dst)) {
                QTfileDlg::DoFileAction(this, pth, dst, a);
            }
            delete [] pth;
        }
        return;
    }
    *accpt = -1;
}


void
QTfilesListDlg::key_press_slot(QKeyEvent *ev)
{
    // Accept Ctrl-C as a copy operation, this is not done in read-only
    // mode.  Note that the QTtextEdit was modified to send these.
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
    Cvt()->PopUpFiles(0, MODE_OFF);
}

