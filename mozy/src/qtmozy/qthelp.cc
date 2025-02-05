
/*========================================================================
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
 * Qt MOZY help viewer.
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qthelp.h"
#include "qtviewer.h"
#include "qtclrdlg.h"
#include "qtinterf/qtfile.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qtinput.h"
#include "qtinterf/qtlist.h"
#include "qtinterf/qtsearch.h"
#include "queue_timer.h"

#include "help/help_startup.h"
#include "help/help_cache.h"
#include "help/help_topic.h"
#include "httpget/transact.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "miscutil/proxy.h"

#include <QApplication>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>
#include <QLabel>
#include <QResizeEvent>
#include <QScrollBar>
#include <QActionGroup>

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#include <sys/wait.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif


/*=======================================================================
 =
 =  HTML Viewer for WWW and Help System
 =
 =======================================================================*/

// Help keywords used in this file
//  helpsys

// Architecture note
// Each QThelpDlg has a corresponding "root_topic" topic struct
// linked into HLP()->context()->TopList.  In these topics, the
// context field points back to the QThelpDlg.  Each new page
// displayed in the QThelpDlg window has a corresponding topic struct
// linked from lastborn in the root_topic, and has the cur_topic field
// set to point to the topic.  Topics linked to lastborn do not have
// the context field set.

namespace { queue_timer QueueTimer; }
QueueLoop &HLPcontext::hcxImageQueueLoop = QueueTimer;

// Initiate queue processing.
//
void
queue_timer::start()
{
    if (!timer) {
        timer = new QTimer(0);
        timer->setInterval(50);
        connect(timer, &QTimer::timeout,
            this, &queue_timer::run_queue_slot);
    }
    timer->start();
}


// Suspend queue processing.  Have to do this while downloading or QT
// will block in the Transaction event loop.
//
void
queue_timer::suspend()
{
    if (timer)
        timer->stop();
}


// Resume queue processing.
//
void
queue_timer::resume()
{
    if (timer)
        timer->start();
}


// Process the queue until empty.
//
void
queue_timer::run_queue_slot()
{
    if (!HLP()->context()->processList()) {
        delete timer;
        timer = 0;
    }
}


//-----------------------------------------------------------------------------
// Local classes

namespace qtinterf
{
    class action_item : public QAction
    {
    public:
        action_item(HLPbookMark *b, QObject *prnt) : QAction(prnt)
        {
            ai_bookmark = b;
            QString qs = QString(b->title);
            qs.truncate(32);
            setText(qs);
        }

        ~action_item()
        {
            ai_bookmark =
                HLP()->context()->bookmarkUpdate(0, ai_bookmark->url);
            delete ai_bookmark;
        }

        void setBookmark(HLPbookMark *b) { ai_bookmark = b; }
        HLPbookMark *bookmark() { return (ai_bookmark); }

    private:
        HLPbookMark *ai_bookmark;
    };
}


//-----------------------------------------------------------------------------
// Local declarations

namespace {
    // XPM 
    const char * const forward_xpm[] = {
    "16 16 4 1",
    " 	c none",
    ".	c #0000dd",
    "x	c #0000ee",
    "+  c #0000ff",
    "                ",
    "    .+          ",
    "    .x+         ",
    "    .xx+        ",
    "    .xxx+       ",
    "    .xxxx+      ",
    "    .xxxxx+     ",
    "    .xxxxxx.    ",
    "    .xxxxx.     ",
    "    .xxxx.      ",
    "    .xxx.       ",
    "    .xx.        ",
    "    .x.         ",
    "    ..          ",
    "                ",
    "                "};

    // XPM 
    const char * const backward_xpm[] = {
    "16 16 4 1",
    " 	c none",
    ".	c #0000dd",
    "x	c #0000ee",
    "+  c #0000ff",
    "                ",
    "          +.    ",
    "         +x.    ",
    "        +xx.    ",
    "       +xxx.    ",
    "      +xxxx.    ",
    "     +xxxxx.    ",
    "    .xxxxxx.    ",
    "     .xxxxx.    ",
    "      .xxxx.    ",
    "       .xxx.    ",
    "        .xx.    ",
    "         .x.    ",
    "          ..    ",
    "                ",
    "                "};

    // XPM
    const char * const stop_xpm[] = {
    "16 16 4 1",
    " 	c none",
    ".	c red",
    "x  c pink",
    "+  c black",
    "                ",
    "     xxxxxx     ",
    "    x......x    ",
    "   x........x   ",
    "  x..........x  ",
    " x............x ",
    " x............x ",
    " x............x ",
    " x............x ",
    " x............x ",
    " x............x ",
    "  x..........x  ",
    "   x........x   ",
    "    x......x    ",
    "     xxxxxx     ",
    "                "};
}

//-----------------------------------------------------------------------------
// Exports

// Top level help popup call, takes care of accessing the database. 
// Return false if the topic is not found
//
bool
QTbag::PopUpHelp(const char *wordin)
{
    if (!HLP()->get_path(0)) {
        PopUpErr(MODE_ON, "Error: no path to database.");
        HLP()->context()->quitHelp();
        return (false);
    }
    char buf[256];
    buf[0] = 0;
    if (wordin) {
        while (isspace(*wordin))
            wordin++;
        if (*wordin) {
            strcpy(buf, wordin);
            char *s = buf + strlen(buf) - 1;
            while (*s == ' ')
                *s-- = '\0';
        }
    }
    char *word = buf[0] ? buf : 0;;

    HLPtopic *top = 0;
    if (HLP()->context()->resolveKeyword(word, &top, 0, 0, 0, false, true))
        return (false);
    if (!top) {
        if (!word)
            snprintf(buf, 256, "Error: no top level topic\n");
        else {
            char *tt = lstring::copy(word);
            snprintf(buf, 256, "Error: No such topic: %s\n", tt);
            delete [] tt;
        }
        PopUpErr(MODE_ON, buf);
        return (false);
    }
    top->show_in_window();
    return (true);
}


HelpWidget *
HelpWidget::get_widget(HLPtopic *top)
{
    if (!top)
        return (0);
    return (dynamic_cast<QThelpDlg*>(HLPtopic::get_parent(top)->context()));
}


HelpWidget *
HelpWidget::new_widget(GRwbag **ptr, int, int)
{
    QWidget *parent = 0;
    if (GRpkg::self()->MainWbag()) {
        QTbag *wb = dynamic_cast<QTbag*>(GRpkg::self()->MainWbag());
        if (wb)
            parent = wb->Shell();
    }

    QThelpDlg *w = new QThelpDlg(true, parent);
    if (ptr)
        *ptr = w;
    w->show();
    return (w);
}


//-----------------------------------------------------------------------------
// Constructor/destrucor

namespace {
    void sens_set(QTbag *wp, bool set, int)
    {
        QThelpDlg *w = dynamic_cast<QThelpDlg*>(wp);
        if (w)
            w->menu_sens_set(set);
    }
}


#ifdef Q_OS_MACOS
#define USE_QTOOLBAR
#endif


QThelpDlg::QThelpDlg(bool has_menu, QWidget *prnt) : QDialog(prnt),
    QTbag(this)
{
    // If has_menu is false, the widget will not have the menu or the
    // status bar visible.

    wb_shell = this;
    h_params = 0;
    h_root_topic = 0;
    h_cur_topic = 0;
    h_stop_btn_pressed = false;
    h_is_frame = !has_menu;
    h_fifo_name = 0;
    h_fifo_fd = -1;
    h_fifo_tid = 0;
#ifdef WIN32
    h_fifo_pipe = 0;
    h_fifo_tfiles = 0;
#endif
    h_cache_list = 0;
    h_clrdlg = 0;
    h_frame_array = 0;
    h_frame_array_size = 0;
    h_ign_case = false;
    h_frame_parent = 0;
    h_frame_name = 0;
    h_searcher = 0;
    h_last_search = 0;

    wb_sens_set = ::sens_set;

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(2, 2, 2, 2);
    vbox->setSpacing(2);
#ifdef USE_QTOOLBAR
    QToolBar *menubar = new QToolBar();
    vbox->addWidget(menubar);
    menubar->setMaximumHeight(24);
#else
    QMenuBar *menubar = new QMenuBar();
    vbox->setMenuBar(menubar);
#endif
    if (!has_menu)
        menubar->hide();
    else {
        int xx = 0, yy = 0;
        if (prnt) {
            QRect r = prnt->geometry();
            xx = r.left();
            yy = r.top();
        }
        xx += 50;
        yy += 50;
        setWindowFlags(Qt::Dialog);
        move(xx, yy);
    }

    h_viewer = new QTviewer(HLP_DEF_WIDTH, HLP_DEF_HEIGHT, this, this);
    vbox->addWidget(h_viewer);
    h_status_bar = new QLabel();
    h_status_bar->setAlignment(Qt::AlignCenter);
    vbox->addWidget(h_status_bar);
    if (!has_menu)
        h_status_bar->hide();

    if (!h_is_frame)
        h_params = new HLPparams(HLP()->no_file_fonts());

    h_viewer->freeze();

#if QT_VERSION >= QT_VERSION_CHECK(5,15,11)
    h_Backward = menubar->addAction(tr("back"),
        this, &QThelpDlg::backward_slot);
    h_Backward->setIcon(QIcon(QPixmap(backward_xpm)));

    h_Forward = menubar->addAction(tr("forw"),
        this, &QThelpDlg::forward_slot);
    h_Forward->setIcon(QIcon(QPixmap(forward_xpm)));

    h_Stop = menubar->addAction(tr("stop"),
        this, &QThelpDlg::stop_slot);
    h_Stop->setIcon(QIcon(QPixmap(stop_xpm)));
#else
    h_Backward = menubar->addAction(tr("back"));
    h_Backward->setIcon(QIcon(QPixmap(backward_xpm)));
    connect(h_Backward, &QAction::triggered, this, &QThelpDlg::backward_slot);
    h_Forward = menubar->addAction(tr("forw"));
    h_Forward->setIcon(QIcon(QPixmap(forward_xpm)));
    connect(h_Forward, &QAction::triggered, this, &QThelpDlg::forward_slot);
    h_Stop = menubar->addAction(tr("stop"));
    h_Stop->setIcon(QIcon(QPixmap(stop_xpm)));
    connect(h_Stop, &QAction::triggered, this, &QThelpDlg::stop_slot);
#endif

#ifdef USE_QTOOLBAR
    QAction *a = menubar->addAction(tr("&File"));
    h_main_menus[0] = new QMenu();
    a->setMenu(h_main_menus[0]);
    QToolButton *tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    h_main_menus[0] = menubar->addMenu(tr("&File"));
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    h_Open = h_main_menus[0]->addAction(tr("&Open"), Qt::CTRL|Qt::Key_O,
        this, &QThelpDlg::open_slot);
    h_OpenFile = h_main_menus[0]->addAction(tr("Open &File"),
        Qt::CTRL|Qt::Key_F, this, &QThelpDlg::open_file_slot);
    h_Save = h_main_menus[0]->addAction(tr("&Save"), Qt::CTRL|Qt::Key_S,
        this, &QThelpDlg::save_slot);
    h_Print = h_main_menus[0]->addAction(tr("&Print"), Qt::CTRL|Qt::Key_P,
        this, &QThelpDlg::print_slot);
    h_Reload = h_main_menus[0]->addAction(tr("&Reload"), Qt::CTRL|Qt::Key_R,
        this, &QThelpDlg::reload_slot);
    // "Old Charset" 8859 support removed.
    h_MkFIFO = h_main_menus[0]->addAction(tr("&Make FIFO"), Qt::CTRL|Qt::Key_M,
        this, &QThelpDlg::make_fifo_slot);
    h_MkFIFO->setCheckable(true);

    h_main_menus[0]->addSeparator();
    h_Quit = h_main_menus[0]->addAction(tr("&Quit"), Qt::CTRL|Qt::Key_Q,
        this, &QThelpDlg::quit_slot);
#else
    h_Open = h_main_menus[0]->addAction(tr("&Open"), this,
        &QThelpDlg::open_slot, Qt::CTRL|Qt::Key_O);
    h_OpenFile = h_main_menus[0]->addAction(tr("Open &File"), this,
        &QThelpDlg::open_file_slot,  Qt::CTRL|Qt::Key_F);
    h_Save = h_main_menus[0]->addAction(tr("&Save"), this,
        &QThelpDlg::save_slot, Qt::CTRL|Qt::Key_S);
    h_Print = h_main_menus[0]->addAction(tr("&Print"), this,
        &QThelpDlg::print_slot, Qt::CTRL|Qt::Key_P);
    h_Reload = h_main_menus[0]->addAction(tr("&Reload"), this,
        &QThelpDlg::reload_slot, Qt::CTRL|Qt::Key_R);
    // "Old Charset" 8859 support removed.
    h_MkFIFO = h_main_menus[0]->addAction(tr("&Make FIFO"), this,
        &QThelpDlg::make_fifo_slot, Qt::CTRL|Qt::Key_M);
    h_MkFIFO->setCheckable(true);

    h_main_menus[0]->addSeparator();
    h_Quit = h_main_menus[0]->addAction(tr("&Quit"), this,
        &QThelpDlg::quit_slot, Qt::CTRL|Qt::Key_Q);
#endif

#ifdef USE_QTOOLBAR
    a = menubar->addAction(tr("&Options"));
    h_main_menus[1] = new QMenu();
    a->setMenu(h_main_menus[1]);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    h_main_menus[1] = menubar->addMenu(tr("&Options"));
#endif
    h_Config = h_main_menus[1]->addAction(tr("Save Config"),
        this, &QThelpDlg::config_slot);
    h_Proxy = h_main_menus[1]->addAction(tr("Set Proxy"),
        this, &QThelpDlg::proxy_slot);
    h_Search = h_main_menus[1]->addAction(tr("&Search Database"),
        this, &QThelpDlg::search_slot);
    h_FindText = h_main_menus[1]->addAction(tr("Find &Text"),
        this, &QThelpDlg::find_text_slot);
    h_DefaultColors = h_main_menus[1]->addAction(tr("Default Colors"),
        this, &QThelpDlg::colors_slot);
    h_DefaultColors->setCheckable(true);
    h_SetFont = h_main_menus[1]->addAction(tr("Set &Font"));
    h_SetFont->setCheckable(true);
    connect(h_SetFont, &QAction::toggled,
        this, &QThelpDlg::set_font_slot);
    h_DontCache = h_main_menus[1]->addAction(tr("&Don't Cache"));
    h_DontCache->setCheckable(true);
    connect(h_DontCache, &QAction::toggled,
        this, &QThelpDlg::dont_cache_slot);
    h_DontCache->setChecked(h_params->NoCache);
    h_ClearCache = h_main_menus[1]->addAction(tr("&Clear Cache"),
        this, &QThelpDlg::clear_cache_slot);
    h_ReloadCache = h_main_menus[1]->addAction(tr("&Reload Cache"),
        this, &QThelpDlg::reload_cache_slot);
    h_ShowCache = h_main_menus[1]->addAction(tr("Show Cache"),
        this, &QThelpDlg::show_cache_slot);
    h_main_menus[1]->addSeparator();
    h_NoCookies = h_main_menus[1]->addAction(tr("No Cookies"));
    h_NoCookies->setCheckable(true);
    connect(h_NoCookies, &QAction::toggled,
        this, &QThelpDlg::no_cookies_slot);
    h_NoCookies->setChecked(h_params->NoCookies);

    QActionGroup *ag = new QActionGroup(this);
    h_NoImages = h_main_menus[1]->addAction(tr("No Images"));
    h_NoImages->setCheckable(true);
    connect(h_NoImages, &QAction::toggled,
        this, &QThelpDlg::no_images_slot);
    ag->addAction(h_NoImages);
    h_SyncImages = h_main_menus[1]->addAction(tr("Sync Images"));
    h_SyncImages->setCheckable(true);
    connect(h_SyncImages, &QAction::toggled,
        this, &QThelpDlg::sync_images_slot);
    ag->addAction(h_SyncImages);
    h_DelayedImages =
        h_main_menus[1]->addAction(tr("Delayed Images"));
    h_DelayedImages->setCheckable(true);
    connect(h_DelayedImages, &QAction::toggled,
        this, &QThelpDlg::delayed_images_slot);
    ag->addAction(h_DelayedImages);
    h_ProgressiveImages =
        h_main_menus[1]->addAction(tr("Progressive Images"));
    h_ProgressiveImages->setCheckable(true);
    connect(h_ProgressiveImages, &QAction::toggled,
        this, &QThelpDlg::progressive_images_slot);
    ag->addAction(h_ProgressiveImages);
    if (h_params->LoadMode == HLPparams::LoadProgressive)
        h_ProgressiveImages->setChecked(true);
    else if (h_params->LoadMode == HLPparams::LoadDelayed)
        h_DelayedImages->setChecked(true);
    else if (h_params->LoadMode == HLPparams::LoadSync)
        h_SyncImages->setChecked(true);
    else
        h_NoImages->setChecked(true);

    h_main_menus[1]->addSeparator();

    ag = new QActionGroup(this);
    h_AnchorPlain = h_main_menus[1]->addAction(tr("Anchor Plain"));
    h_AnchorPlain->setCheckable(true);
    connect(h_AnchorPlain, &QAction::toggled,
        this, &QThelpDlg::anchor_plain_slot);
    ag->addAction(h_AnchorPlain);
    h_AnchorButtons =
        h_main_menus[1]->addAction(tr("Anchor Buttons"));
    h_AnchorButtons->setCheckable(true);
    connect(h_AnchorButtons, &QAction::toggled,
        this, &QThelpDlg::anchor_buttons_slot);
    ag->addAction(h_AnchorButtons);
    h_AnchorUnderline =
        h_main_menus[1]->addAction(tr("Anchor Underline"));
    h_AnchorUnderline->setCheckable(true);
    connect(h_AnchorUnderline, &QAction::toggled,
        this, &QThelpDlg::anchor_underline_slot);
    ag->addAction(h_AnchorUnderline);
    if (h_params->AnchorButtons)
        h_AnchorButtons->setChecked(true);
    else if (h_params->AnchorUnderlined)
        h_AnchorUnderline->setChecked(true);
    else
        h_AnchorPlain->setChecked(true);

    h_AnchorHighlight =
        h_main_menus[1]->addAction(tr("Anchor Highlight"));
    h_AnchorHighlight->setCheckable(true);
    connect(h_AnchorHighlight, &QAction::toggled,
        this, &QThelpDlg::anchor_highlight_slot);
    h_AnchorHighlight->setChecked(h_params->AnchorHighlight);
    h_BadHTML = h_main_menus[1]->addAction(tr("Bad HTML Warnings"));
    h_BadHTML->setCheckable(true);
    connect(h_BadHTML, &QAction::toggled,
        this, &QThelpDlg::bad_html_slot);
    h_BadHTML->setChecked(h_params->BadHTMLwarnings);
    h_FreezeAnimations =
        h_main_menus[1]->addAction(tr("Freeze Animations"));
    h_FreezeAnimations->setCheckable(true);
    connect(h_FreezeAnimations, &QAction::toggled,
        this, &QThelpDlg::freeze_animations_slot);
    h_FreezeAnimations->setChecked(h_params->FreezeAnimations);
    h_LogTransactions =
        h_main_menus[1]->addAction(tr("Log Transactions"));
    h_LogTransactions->setCheckable(true);
    connect(h_LogTransactions, &QAction::toggled,
        this, &QThelpDlg::log_transactions_slot);
    h_LogTransactions->setChecked(h_params->PrintTransact);

#ifdef USE_QTOOLBAR
    a = menubar->addAction(tr("&Bookmarks"));
    h_main_menus[2] = new QMenu();
    a->setMenu(h_main_menus[2]);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    h_main_menus[2] = menubar->addMenu(tr("&Bookmarks"));
#endif
    h_AddBookmark = h_main_menus[2]->addAction(tr("Add"),
        this, &QThelpDlg::add_slot);
    h_DeleteBookmark = h_main_menus[2]->addAction(tr("Delete"),
        this, &QThelpDlg::delete_slot);
    h_DeleteBookmark->setCheckable(true);
    h_main_menus[2]->addSeparator();

    menubar->addSeparator();
#ifdef USE_QTOOLBAR
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    menubar->addAction(tr("Help"), Qt::CTRL|Qt::Key_H,
        this, &QThelpDlg::help_slot);
#else
    a = menubar->addAction(tr("&Help"), this, &QThelpDlg::help_slot);
    a->setShortcut(QKeySequence("Ctrl+H"));
#endif
#else
    h_main_menus[3] = menubar->addMenu(tr("&Help"));
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    h_Help = h_main_menus[3]->addAction(tr("&Help"), Qt::CTRL|Qt::Key_H,
        this, &QThelpDlg::help_slot);
#else
    h_Help = h_main_menus[3]->addAction(tr("&Help"), this,
        &QThelpDlg::help_slot, Qt::CTRL|Qt::Key_H);
#endif
#endif

    h_viewer->thaw();

    HLP()->context()->readBookmarks();
    for (HLPbookMark *b = HLP()->context()->bookmarks(); b; b = b->next) {
        QString qs = QString(b->title);
        qs.truncate(32);
        h_main_menus[2]->addAction(
            new action_item(b, h_main_menus[2]));
    }
    connect(h_main_menus[2], &QMenu::triggered,
        this, &QThelpDlg::bookmark_slot);

    h_Backward->setEnabled(false);
    h_Forward->setEnabled(false);
    h_Stop->setEnabled(false);
    if (prnt)
        set_transient_for(prnt);
}


QThelpDlg::~QThelpDlg()
{
    unregister_fifo();
    HLP()->context()->quitHelp();
    halt_images();
    HLP()->context()->removeTopic(h_root_topic);
    if (!h_is_frame)
        delete h_params;

    if (h_frame_array) {
        for (int i = 0; i < h_frame_array_size; i++)
            delete h_frame_array[i];
        delete [] h_frame_array;
    }
    delete [] h_frame_name;
}


void
QThelpDlg::menu_sens_set(bool set)
{
    h_Search->setEnabled(set);
    h_Save->setEnabled(set);
    h_Open->setEnabled(set);
}


//-----------------------------------------------------------------------------
// ViewWidget and HelpWidget virtual function overrides

void
QThelpDlg::freeze()
{
    h_viewer->freeze();
}


void
QThelpDlg::thaw()
{
    h_viewer->thaw();
}


// Set a pointer to the Transaction struct.
//
void
QThelpDlg::set_transaction(Transaction *t, const char *cookiedir)
{
    h_viewer->set_transaction(t, cookiedir);
    if (t) {
        t->set_timeout(h_params->Timeout);
        t->set_retries(h_params->Retries);
        t->set_http_port(h_params->HTTP_Port);
        t->set_ftp_port(h_params->FTP_Port);
        t->set_http_debug(h_params->DebugMode);
        if (h_params->PrintTransact)
            t->set_logfile("stderr");
        if (cookiedir && !h_params->NoCookies) {
            int len = strlen(cookiedir) + 20;
            char *cf = new char [len];
            snprintf(cf, len,  "%s/%s", cookiedir, "cookies");
            t->set_cookiefile(cf);
            delete [] cf;
        }
    }
}


// Return a pointer to the transaction struct.
//
Transaction *
QThelpDlg::get_transaction()
{
    return (h_viewer->get_transaction());
}


// Check if the transfer should be aborted, return true if so.
//
bool
QThelpDlg::check_halt_processing(bool run_events)
{
    if (run_events) {
        qApp->processEvents(QEventLoop::AllEvents);
        if (!h_Stop->isEnabled() && h_stop_btn_pressed)
            return (true);
    }
    else if (h_stop_btn_pressed)
        return (true);
    return (false);
}


// Enable/disable halt button sensitivity.
//
void
QThelpDlg::set_halt_proc_sens(bool set)
{
    h_Stop->setEnabled(set);
}


// Write text on the status line.
//
void
QThelpDlg::set_status_line(const char *msg)
{
    if (h_frame_parent)
        h_frame_parent->set_status_line(msg);
    else
        h_status_bar->setText(msg);
}


htmImageInfo *
QThelpDlg::new_image_info(const char *url, bool progressive)
{
    return (h_viewer->new_image_info(url, progressive));
}


bool
QThelpDlg::call_plc(const char *url)
{
    return (h_viewer->call_plc(url));
}


htmImageInfo *
QThelpDlg::image_procedure(const char *url)
{
    return (h_viewer->image_procedure(url));
}


void
QThelpDlg::image_replace(htmImageInfo *image, htmImageInfo *new_image)
{
    h_viewer->image_replace(image, new_image);
}


bool
QThelpDlg::is_body_image(const char *url)
{
    if (HLP()->context()->isImageInList(this))
        return (false);
    return (h_viewer->is_body_image(url));
}


const char *
QThelpDlg::get_url()
{
    return (h_cur_topic ? h_cur_topic->keyword() : 0);
}


bool
QThelpDlg::no_url_cache()
{
    return (h_params->NoCache);
}


int
QThelpDlg::image_load_mode()
{
    return (h_params->LoadMode);
}


int
QThelpDlg::image_debug_mode()
{
    return (h_params->LocalImageTest);
}


GRwbag *
QThelpDlg::get_widget_bag()
{
    return (this);
}


//
// HelpWidget overrides
//

// Link and display the new topic.
//
void
QThelpDlg::link_new(HLPtopic *top)
{
    HLP()->context()->linkNewTopic(top);
    h_root_topic = top;
    h_cur_topic = top;
    reuse(0, false);
}


// Strip HTML tokens out of the window title
//
static void
strip_html(char *buf)
{
    char tbuf[256];
    strcpy(tbuf, buf);
    char *d = buf;
    for (char *s = tbuf; *s; s++) {
        if (*s == '<' && (*(s+1) == '/' || isalpha(*(s+1)))) {
            while (*s && *s != '>')
                s++;
            continue;
        }
        *d++ = *s;
    }
    *d = 0;
}


// Reuse the current display to show and possible link in newtop.
//
void
QThelpDlg::reuse(HLPtopic *newtop, bool newlink)
{
    if (h_root_topic->lastborn()) {
        // in the forward/back operations, the new topic is stitched in
        // as lastborn, and the scroll update is handled elsewhere
        if (h_root_topic->lastborn() != newtop)
            h_root_topic->lastborn()->set_topline(get_scroll_pos());
    }
    else if (newtop)
        h_root_topic->set_topline(get_scroll_pos());

    if (!newtop)
        newtop = h_root_topic;

    if (!newtop->is_html())
        newtop->set_show_plain(HLP()->context()->isPlain(newtop->keyword()));
    if (newtop != h_root_topic) {
        h_root_topic->set_text(newtop->get_text());
        newtop->clear_text();
    }
    else
        h_root_topic->get_text();
    h_cur_topic = newtop;

    const char *t = HLP()->context()->findAnchorRef(newtop->keyword());
    char *anchor = 0;
    if (t)
        anchor = lstring::copy(t);
    if (newlink && newtop != h_root_topic) {
        newtop->set_sibling(h_root_topic->lastborn());
        h_root_topic->set_lastborn(newtop);
        newtop->set_parent(h_root_topic);
        h_Backward->setEnabled(true);
    }
    redisplay();
    if (anchor)
        set_scroll_pos(h_viewer->anchor_pos_by_name(anchor));
    else
        set_scroll_pos(newtop->get_topline());
    delete [] anchor;
    set_status_line(newtop->keyword());

    t = newtop->title();
    if (!t || !*t)
        t = h_viewer->get_title();
    if (!t || !*t)
        t = "Whiteley Research Inc.";
    char buf[256];
    snprintf(buf, 256, "%s -- %s", HLP()->get_name(), t);
    strip_html(buf);
    setWindowTitle(QString(buf));
    setAttribute(Qt::WA_DeleteOnClose);
    QueueTimer.start();
}


// Halt any current display activity and redisplay.
//
void
QThelpDlg::redisplay()
{
    Transaction *t = h_viewer->get_transaction();
    if (t) {
        // still downloading previous page, abort
        t->set_abort();
        h_viewer->set_transaction(0, 0);
    }
    halt_images();

    HLPtopic *top = h_root_topic->lastborn();
    if (!top)
        top = h_root_topic;

    if (top->show_plain() || !top->is_html())
        h_viewer->set_mime_type("text/plain");
    else
        h_viewer->set_mime_type("text/html");
    h_viewer->set_source(h_root_topic->get_cur_text());
}


HLPtopic *
QThelpDlg::get_topic()
{
    return (h_cur_topic);
}


// Unset the halt flag, which will be set if an abort is needed.
//
void
QThelpDlg::unset_halt_flag()
{
    h_stop_btn_pressed = false;
}


void
QThelpDlg::halt_images()
{
    stop_image_download();
    HLP()->context()->flushImages(this);
}


void
QThelpDlg::show_cache(int mode)
{
    if (mode == MODE_OFF) {
        if (h_cache_list)
            h_cache_list->popdown();
        return;
    }
    if (mode == MODE_UPD) {
        if (h_cache_list) {
            stringlist *s0 = HLP()->context()->listCache();
            h_cache_list->update(s0, "Cache Entries", 0);
            stringlist::destroy(s0);
        }
        return;
    }
    if (h_cache_list)
        return;
    stringlist *s0 = HLP()->context()->listCache();
    h_cache_list = PopUpList(s0, "Cache Entries", 0, 0, 0, false, false);
    stringlist::destroy(s0);
    if (h_cache_list) {
        h_cache_list->register_usrptr((void**)&h_cache_list);
        QTlistDlg *list = dynamic_cast<QTlistDlg*>(h_cache_list);
        if (list) {
            connect(list, &QTlistDlg::action_call,
                this, &QThelpDlg::cache_choice_slot);
        }
    }
}


//-----------------------------------------------------------------------------
// htmDataInterface methods

// All QTviewer "signals" are dispatched from here.
//
void
QThelpDlg::emit_signal(SignalID id, void *payload)
{
    switch (id) {
    case S_ARM:
        // htm_arm_proc(static_cast<htmCallbackInfo*>(payload));
        break;
    case S_ACTIVATE:
        htm_activate_proc(static_cast<htmAnchorCallbackStruct*>(payload));
        break;
    case S_ANCHOR_TRACK:
        {
            htmAnchorCallbackStruct *cbs =
                static_cast<htmAnchorCallbackStruct*>(payload);
            if (cbs && cbs->href)
                h_status_bar->setText(cbs->href);
            else
                h_status_bar->setText(h_cur_topic->keyword());
        }
        break;
    case S_ANCHOR_VISITED:
        {
            htmVisitedCallbackStruct *cbs =
                static_cast<htmVisitedCallbackStruct*>(payload);
            if (cbs)
                cbs->visited = HLP()->context()->isVisited(cbs->url);
        }
        break;
    case S_DOCUMENT:
        // htm_document_proc(static_cast<htmDocumentCallbackStruct*>(payload));
        break;
    case S_LINK:
        // htm_link_proc(static_cast<htmLinkCallbackStruct*>(payload));
        break;
    case S_FRAME:
        htm_frame_proc(static_cast<htmFrameCallbackStruct*>(payload));
        break;
    case S_FORM:
        {
            // Handle the "submit" request for an html form.  The form
            // return is always downloaded and never taken from the
            // cache, since this prevents multiple submissions of the
            // same form.

            htmFormCallbackStruct *cbs =
                static_cast<htmFormCallbackStruct*>(payload);
            if (cbs)
                HLP()->context()->formProcess(cbs, this);
        }
        break;
    case S_IMAGEMAP:
        // htm_imagemap_proc(static_cast<htmImagemapCallbackStruct*>(payload));
        break;
    case S_HTML_EVENT:
        // htm_html_event_proc(static_cast<htmEventCallbackStruct*>(payload));
        break;
    default:
        break;
    }
}


void *
QThelpDlg::event_proc(const char*)
{
    return (0);
}


void
QThelpDlg::panic_callback(const char*)
{
}


// Called by the widget to resolve image references.
//
htmImageInfo *
QThelpDlg::image_resolve(const char *fname)
{
    if (!fname)
        return (0);
    return (HLP()->context()->imageResolve(fname, this));
}


// This is the "get_data" callback for progressive image loading.
//
int
QThelpDlg::get_image_data(htmPLCStream *stream, void *buffer)
{
    HLPimageList *im = (HLPimageList*)stream->user_data;
    return (HLP()->context()->getImageData(im, stream, buffer));
}


// This is the "end_data" callback for progressive image loading.
//
void
QThelpDlg::end_image_data(htmPLCStream *stream, void*, int type, bool)
{
    if (type == PLC_IMAGE) {
        HLPimageList *im = (HLPimageList*)stream->user_data;
        HLP()->context()->inactivateImages(im);
    }
}


// This returns the client area that can be used for display frames. 
// We use the entire widget width, and the height between the menu and
// status bar.
//
void
QThelpDlg::frame_rendering_area(htmRect *rct)
{
    QRect r1 = h_viewer->frameGeometry();
    QRect r2 = h_status_bar->frameGeometry();
    QSize qs = size();

    rct->x = 0;
    rct->y = r1.top();
    rct->width = qs.width();
    rct->height = r2.top() - r1.top();
}


// If this is a frame, return the frame name.
// Otherwise return -1.
//
const char *
QThelpDlg::get_frame_name()
{
    return (h_frame_name);
}


// Return the keyword and title from the current topic, if possible.
//
void
QThelpDlg::get_topic_keys(char **pkw, char **ptitle)
{
    if (pkw)
        *pkw = 0;
    if (ptitle)
        *ptitle = 0;
    HLPtopic *t = h_cur_topic;
    if (t) {
        if (pkw)
            *pkw = lstring::copy(t->keyword());
        if (ptitle) {
            const char *title = t->title();
            if (title)
                *ptitle = lstring::copy(title);
        }
    }
}


// Ensure that the bounding box passed is visible.
//
void
QThelpDlg::scroll_visible(int l, int t, int r, int b)
{
    h_viewer->scroll_visible(l, t, r, b);
}


//-----------------------------------------------------------------------------
// QTbag functions

char *
QThelpDlg::GetPostscriptText(int font_family, const char *url,
    const char *title, bool use_headers, bool a4)
{
    return (h_viewer->get_postscript_text(font_family, url, title,
        use_headers, a4));
}


char *
QThelpDlg::GetPlainText()
{
    return (h_viewer->get_plain_text());
}


char *
QThelpDlg::GetHtmlText()
{
    return (h_viewer->get_html_text());
}


//-----------------------------------------------------------------------------
// Misc.

// scrollbar setting
int
QThelpDlg::get_scroll_pos(bool horiz)
{
    QScrollBar *sb;
    if (horiz)
        sb = h_viewer->horizontalScrollBar();
    else
        sb = h_viewer->verticalScrollBar();
    if (!sb)
        return (0);
    return (sb->value());
}


void
QThelpDlg::set_scroll_pos(int posn, bool horiz)
{
    h_viewer->set_scroll_position(posn, horiz);
    /*
    QScrollBar *sb;
    if (horiz)
        sb = h_viewer->horizontalScrollBar();
    else
        sb = h_viewer->verticalScrollBar();
    if (!sb)
        return;
    sb->setValue(posn);
    */
}


//-----------------------------------------------------------------------------
// Slots

void
QThelpDlg::backward_slot()
{
    HLPtopic *top = h_root_topic;
    if (top && top->lastborn()) {
        HLPtopic *last = top->lastborn();
        top->set_lastborn(last->sibling());
        last->set_sibling(top->next());
        top->set_next(last);
        last->set_topline(get_scroll_pos());
        top->reuse(top->lastborn(), false);

        h_Backward->setEnabled(top->lastborn());
        h_Forward->setEnabled(true);
   }
}


void
QThelpDlg::forward_slot()
{
    HLPtopic *top = h_root_topic;
    if (top && top->next()) {
        HLPtopic *next = top->next();
        top->set_next(next->sibling());
        next->set_sibling(top->lastborn());
        top->set_lastborn(next);
        if (next->sibling())
            next->sibling()->set_topline(get_scroll_pos());
        else
            top->set_topline(get_scroll_pos());
        top->reuse(top->lastborn(), false);

        h_Forward->setEnabled(top->next());
        h_Backward->setEnabled(true);
    }
}


void
QThelpDlg::stop_slot()
{
    stop_image_download();
    h_Stop->setEnabled(false);
    h_stop_btn_pressed = true;
}


void
QThelpDlg::open_slot()
{
    PopUpInput("Enter keyword, file name, or URL", "", "Open", 0, 0);
    connect(wb_input, &QTledDlg::action_call,
        this, &QThelpDlg::do_open_slot);
}


void
QThelpDlg::open_file_slot()
{
    GRfilePopup *fs = PopUpFileSelector(fsSEL, GRloc(), 0, 0, 0, 0);
    QTfileDlg *fsel = dynamic_cast<QTfileDlg*>(fs);
    if (fsel) {
        connect(fsel, &QTfileDlg::file_selected,
            this, &QThelpDlg::do_open_slot);
    }
}


void
QThelpDlg::save_slot()
{
    PopUpInput(0, "", "Save", 0, 0);
    connect(wb_input, &QTledDlg::action_call,
        this, &QThelpDlg::do_save_slot);
}


// for hardcopies
static HCcb hlpHCcb =
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


void
QThelpDlg::print_slot()
{
    if (!hlpHCcb.command)
        hlpHCcb.command = lstring::copy(GRappIf()->GetPrintCmd());
    PopUpPrint(0, &hlpHCcb, HCtextPS);
}


void
QThelpDlg::reload_slot()
{
    h_cur_topic->set_topline(get_scroll_pos());
    newtopic(h_cur_topic->keyword(), false, false, true);
}


void
QThelpDlg::make_fifo_slot(bool state)
{
    if (state) {
        if (register_fifo(0)) {
            sLstr tstr;
            tstr.add("Listening for input on pipe named\n");
            tstr.add(h_fifo_name);
            PopUpMessage(tstr.string(), false);
        }
    }
    else
        unregister_fifo();
}


void
QThelpDlg::quit_slot()
{
    // Old QT5 will crash without delayed delete.
    deleteLater();
}


void
QThelpDlg::config_slot()
{
    if (!h_params->dump())
        PopUpErr(MODE_ON,
            "Failed to write .mozyrc file, permission problem?");
    else
        PopUpMessage("Saved .mozyrc file.", false);
}


void
QThelpDlg::proxy_slot()
{
    char *pxy = proxy::get_proxy();
    PopUpInput("Enter proxy url:", pxy, "Proxy", h_proxy_proc, this);
    delete [] pxy;
}


void
QThelpDlg::search_slot()
{
    PopUpInput("Enter keyword for database search:", "", "Search", 0, 0);
    connect(wb_input, &QTledDlg::action_call,
        this, &QThelpDlg::do_search_slot);
}


void
QThelpDlg::find_text_slot()
{
    if (!h_searcher) {
        h_searcher = new QTsearchDlg(this, h_last_search);
        h_searcher->register_usrptr((void**)&h_searcher);
        h_searcher->set_transient_for(this);
        h_searcher->set_visible(true);

        h_searcher->set_ign_case(h_ign_case);
        connect(h_searcher, &QTsearchDlg::search_down,
            this, &QThelpDlg::search_down_slot);
        connect(h_searcher, &QTsearchDlg::search_up,
            this, &QThelpDlg::search_up_slot);
        connect(h_searcher, &QTsearchDlg::ignore_case,
            this, &QThelpDlg::ignore_case_slot);
    }
}


void
QThelpDlg::colors_slot(bool state)
{
    if (state) {
        if (!h_clrdlg) {
            h_clrdlg = new QTmozyClrDlg(this);
            h_clrdlg->set_transient_for(this);
            h_clrdlg->register_caller(sender());
            h_clrdlg->register_usrptr((void**)&h_clrdlg);
            QTdev::self()->SetPopupLocation(GRloc(LW_CENTER), h_clrdlg, this);
            h_clrdlg->show();
        }
    }
    else if (h_clrdlg)
        h_clrdlg->popdown();
}


void
QThelpDlg::set_font_slot(bool set)
{
    if (set)
        PopUpFontSel(h_SetFont, GRloc(), MODE_ON, 0, 0, FNT_MOZY);
    else
        PopUpFontSel(0, GRloc(), MODE_OFF, 0, 0, 0);
}


void
QThelpDlg::dont_cache_slot(bool set)
{
    h_params->NoCache = set;
}


void
QThelpDlg::clear_cache_slot()
{
    HLP()->context()->clearCache();
}


void
QThelpDlg::reload_cache_slot()
{
    HLP()->context()->reloadCache();
}


void
QThelpDlg::show_cache_slot()
{
    show_cache(MODE_ON);
}


void
QThelpDlg::no_cookies_slot(bool set)
{
    h_params->NoCookies = set;
}


void
QThelpDlg::no_images_slot(bool set)
{
    if (set) {
        stop_image_download();
        h_params->LoadMode = HLPparams::LoadNone;
    }
}


void
QThelpDlg::sync_images_slot(bool set)
{
    if (set) {
        stop_image_download();
        h_params->LoadMode = HLPparams::LoadSync;
    }
}


void
QThelpDlg::delayed_images_slot(bool set)
{
    if (set) {
        stop_image_download();
        h_params->LoadMode = HLPparams::LoadDelayed;
    }
}


void
QThelpDlg::progressive_images_slot(bool set)
{
    if (set) {
        stop_image_download();
        h_params->LoadMode = HLPparams::LoadProgressive;
    }
}


void
QThelpDlg::anchor_plain_slot(bool set)
{
    if (set) {
        h_params->AnchorButtons = false;
        h_params->AnchorUnderlined = false;
        h_viewer->set_anchor_style(ANC_PLAIN);

        int position = get_scroll_pos();
        set_scroll_pos(position);
    }
}


void
QThelpDlg::anchor_buttons_slot(bool set)
{
    if (set) {
        h_params->AnchorButtons = true;
        h_params->AnchorUnderlined = false;
        h_viewer->set_anchor_style(ANC_BUTTON);
    }
}


void
QThelpDlg::anchor_underline_slot(bool set)
{
    if (set) {
        h_params->AnchorButtons = false;
        h_params->AnchorUnderlined = true;
        h_viewer->set_anchor_style(ANC_SINGLE_LINE);

        int position = get_scroll_pos();
        set_scroll_pos(position);
    }
}


void
QThelpDlg::anchor_highlight_slot(bool set)
{
    h_params->AnchorHighlight = set;
    h_viewer->set_anchor_highlighting(set);
}


void
QThelpDlg::bad_html_slot(bool set)
{
    h_params->BadHTMLwarnings = set;
    h_viewer->set_html_warnings(set);
}


void
QThelpDlg::freeze_animations_slot(bool set)
{
    h_params->FreezeAnimations = set;
    h_viewer->set_freeze_animations(set);
}


void
QThelpDlg::log_transactions_slot(bool set)
{
    h_params->PrintTransact = set;
}


void
QThelpDlg::add_slot()
{
    if (!h_cur_topic)
        return;
    HLPtopic *tp = h_cur_topic;
    const char *ptitle = tp->title();
    char *title;
    if (ptitle && *ptitle)
        title = lstring::copy(ptitle);
    else
        title = lstring::copy(h_viewer->get_title());
    char *url = lstring::copy(tp->keyword());
    if (!url || !*url) {
        delete [] url;
        delete [] title;
        return;
    }
    if (!title || !*title) {
        delete [] title;
        title = lstring::copy(url);
    }
    HLPbookMark *b = HLP()->context()->bookmarkUpdate(title, url);
    delete [] title;
    delete [] url;

    QString qs = QString(b->title);
    qs.truncate(32);
    h_main_menus[2]->addAction(
        new action_item(b, h_main_menus[2]));
}


void
QThelpDlg::delete_slot()
{
}


namespace {
    void yncb(bool val, void *client_data)
    {
        if (val)
            delete static_cast<action_item*>(client_data);
    }
}


// Handler for bookmarks selected in the menu.
// 
void
QThelpDlg::bookmark_slot(QAction *action)
{
    action_item *ac = dynamic_cast<action_item*>(action);
    if (ac) {
        if (h_DeleteBookmark->isChecked()) {
            char buf[256];
            strcpy(buf, "Delete ");
            char *t = buf + strlen(buf);
            strncpy(t, ac->bookmark()->title, 32);
            t[33] = 0;
            strcat(t, " ?");
            PopUpAffirm(0, GRloc(), buf, yncb, ac);
        }
        else {
            HLPbookMark *b = ac->bookmark();
            newtopic(b->url, false, false, true);
        }
    }
}


void
QThelpDlg::help_slot()
{
    if (QTdev::self()->MainFrame())
        QTdev::self()->MainFrame()->PopUpHelp("helpsys");
    else
        PopUpHelp("helpsys");
}


// Handle selections from the cache listing.
//
void
QThelpDlg::cache_choice_slot(const char *string)
{
    lstring::advtok(&string);
    newtopic(string, false, false, true);
}


// Callback for the "Open" and "Open File" menu commands, opens a new
// keyword or file
//
void
QThelpDlg::do_open_slot(const char *name, void*)
{
    if (name) {
        while (isspace(*name))
            name++;
        if (*name) {
            char *url = 0;
            const char *t = strrchr(name, '.');
            if (t) {
                t++;
                if (lstring::cieq(t, "html") || lstring::cieq(t, "htm") ||
                        lstring::cieq(t, "jpg") || lstring::cieq(t, "gif") ||
                        lstring::cieq(t, "png")) {
                    if (!lstring::is_rooted(name)) {
                        char *cwd = getcwd(0, 256);
                        if (cwd) {
                            int len = strlen(cwd) + strlen(name) + 2;
                            url = new char[len];
                            snprintf(url, len, "%s/%s", cwd, name);
                            free(cwd);
                            if (access(url, R_OK)) {
                                // no such file
                                delete [] url;
                                url = 0;
                            }
                        }
                    }
                }
            }
            if (!url)
                url = lstring::copy(name);
            if (newtopic(url, false, false, true) != QThelpDlg::NTnone) {
                if (wb_input)
                    wb_input->popdown();
            }
            delete [] url;
        }
    }
}


// Callback passed to PopUpInput to actually save the text in a file.
//
void
QThelpDlg::do_save_slot(const char *fnamein, void*)
{
    char *fname = pathlist::expand_path(fnamein, false, true);
    if (!fname)
        return;
    if (filestat::check_file(fname, W_OK) == NOGO) {
        PopUpMessage(filestat::error_msg(), true);
        delete [] fname;
        return;
    }

    FILE *fp = fopen(fname, "w");
    if (!fp) {
        char tbuf[256];
        if (strlen(fname) > 64)
            strcpy(fname + 60, "...");
        snprintf(tbuf, 256, "Error: can't open file %s", fname);
        PopUpMessage(tbuf, true);
        delete [] fname;
        return;
    }
    char *tptr = h_viewer->get_plain_text();
    const char *mesg;
    if (tptr) {
        if (fputs(tptr, fp) == EOF) {
            PopUpMessage("Error: block write error", true);
            delete [] tptr;
            fclose(fp);
            delete [] fname;
            return;
        }
        delete [] tptr;
        mesg = "Text saved";
    }
    else
        mesg = "Text file is empty";

    fclose(fp);
    if (wb_input)
        wb_input->popdown();
    PopUpMessage(mesg, false);
    delete [] fname;
}


// Callback passed to PopUpInput to actually perform a database keyword
// search.
//
void
QThelpDlg::do_search_slot(const char *target, void*)
{
    if (target && *target) {
        HLPtopic *newtop = HLP()->search(target);
        if (!newtop)
            PopUpErr(MODE_ON, "Unresolved link.");
        else
            newtop->link_new_and_show(false, h_cur_topic);
    }
    if (wb_input)
        wb_input->popdown();
}


void
QThelpDlg::search_down_slot()
{
    QString target = h_searcher->get_target();
    if (!target.isNull() && !target.isEmpty()) {
        delete [] h_last_search;
        h_last_search = lstring::copy(target.toLatin1().constData());
        if (!h_viewer->find_words(h_last_search, false, h_ign_case))
            h_searcher->set_transient_message("Not found!");
    }
}


void
QThelpDlg::search_up_slot()
{
    QString target = h_searcher->get_target();
    if (!target.isNull() && !target.isEmpty()) {
        delete [] h_last_search;
        h_last_search = lstring::copy(target.toLatin1().constData());
        if (!h_viewer->find_words(h_last_search, true, h_ign_case))
            h_searcher->set_transient_message("Not found!");
    }
}


void
QThelpDlg::ignore_case_slot(bool ign)
{
    h_ign_case = ign;
}


//-----------------------------------------------------------------------------
// Private functions


// Handle htm signal from clicking on an anchor.
//
void
QThelpDlg::htm_activate_proc(htmAnchorCallbackStruct *cbs)
{
    if (cbs == 0 || cbs->href == 0)
        return;
    HLPtopic *prnt = h_cur_topic;
    cbs->visited = true;

    // add link to visited table
    HLP()->context()->addVisited(cbs->href);

    // download if shift pressed
    bool force_download = false;

    QMouseEvent *qme = static_cast<QMouseEvent*>(cbs->event);
    if (qme && (qme->modifiers() & Qt::ShiftModifier))
        force_download = true;

    bool spawn = false;
    if (!force_download) {
        if (cbs->target) {
            if (!prnt->target() ||
                    strcmp(prnt->target(), cbs->target)) {
                for (HLPtopic *t = HLP()->context()->topList(); t;
                        t = t->sibling()) {
                    if (t->target() && !strcmp(t->target(), cbs->target)) {
                        newtopic(cbs->href, false, false, false);
                        return;
                    }
                }
                // Special targets:
                //  _top    reuse same window, no frames
                //  _self   put in originating frame
                //  _blank  put in new window
                //  _parent put in parent frame (nested framesets)

                if (!strcmp(cbs->target, "_top")) {
                    newtopic(cbs->href, false, false, false);
                    return;
                }
                // note: _parent not handled, use new window
                if (strcmp(cbs->target, "_self"))
                    spawn = true;
            }
        }
    }

    if (!spawn) {
        // spawn a new window if button 2 pressed
        if (qme && qme->button() == Qt::MiddleButton)
            spawn = true;
    }

    newtopic(cbs->href, spawn, force_download, false);

    if (cbs->target && spawn) {
        for (HLPtopic *t = HLP()->context()->topList(); t; t = t->sibling()) {
            if (!strcmp(t->keyword(), cbs->href)) {
                t->set_target(cbs->target);
                break;
            }
        }
    }
}


// Handle htm signal for frames.
//
void
QThelpDlg::htm_frame_proc(htmFrameCallbackStruct *cbs)
{
    if (cbs->reason == HTM_FRAMECREATE) {
        h_viewer->hide_drawing_area(true);
        h_frame_array_size = cbs->nframes;
        h_frame_array = new QThelpDlg*[h_frame_array_size];
        for (int i = 0; i < h_frame_array_size; i++) {
            h_frame_array[i] = new QThelpDlg(false, this);
            // use parent's defaults
            h_frame_array[i]->h_params = h_params;
            h_frame_array[i]->set_frame_parent(this);
            h_frame_array[i]->set_frame_name(cbs->frames[i].name);

            h_frame_array[i]->setGeometry(cbs->frames[i].x, cbs->frames[i].y,
                cbs->frames[i].width, cbs->frames[i].height);

            if (cbs->frames[i].scroll_type == FRAME_SCROLL_NONE) {
                h_frame_array[i]->h_viewer->setVerticalScrollBarPolicy(
                    Qt::ScrollBarAlwaysOff);
                h_frame_array[i]->h_viewer->setHorizontalScrollBarPolicy(
                    Qt::ScrollBarAlwaysOff);
            }
            else if (cbs->frames[i].scroll_type == FRAME_SCROLL_AUTO) {
                h_frame_array[i]->h_viewer->setVerticalScrollBarPolicy(
                    Qt::ScrollBarAsNeeded);
                h_frame_array[i]->h_viewer->setHorizontalScrollBarPolicy(
                    Qt::ScrollBarAsNeeded);
            }
            else if (cbs->frames[i].scroll_type == FRAME_SCROLL_YES) {
                h_frame_array[i]->h_viewer->setVerticalScrollBarPolicy(
                    Qt::ScrollBarAlwaysOn);
                h_frame_array[i]->h_viewer->setHorizontalScrollBarPolicy(
                    Qt::ScrollBarAlwaysOn);
            }

            h_frame_array[i]->show();

            HLPtopic *newtop;
            char hanchor[128];
            HLP()->context()->resolveKeyword(cbs->frames[i].src, &newtop,
                hanchor, this, 0, false, false);
            if (!newtop) {
                char buf[256];
                snprintf(buf, 256, "Unresolved link: %s.", cbs->frames[i].src);
                PopUpErr(MODE_ON, buf);
            }

            h_frame_array[i]->wb_shell = wb_shell;
            newtop->set_target(cbs->frames[i].name);
            newtop->set_context(h_frame_array[i]);
            h_frame_array[i]->h_root_topic = newtop;
            h_frame_array[i]->h_cur_topic = newtop;

            if (!newtop->is_html() &&
                    HLP()->context()->isPlain(newtop->keyword())) {
                newtop->set_show_plain(true);
                h_frame_array[i]->h_viewer->set_mime_type("text/plain");
            }
            else {
                newtop->set_show_plain(false);
                h_frame_array[i]->h_viewer->set_mime_type("text/html");
            }
            h_frame_array[i]->h_viewer->set_source(newtop->get_text());
        }
    }
    else if (cbs->reason == HTM_FRAMERESIZE) {
        for (int i = 0; i < h_frame_array_size; i++) {
            h_frame_array[i]->setGeometry(cbs->frames[i].x, cbs->frames[i].y,
                cbs->frames[i].width, cbs->frames[i].height);
        }
    }
    else if (cbs->reason == HTM_FRAMEDESTROY) {
        for (int i = 0; i < h_frame_array_size; i++)
            delete h_frame_array[i];
        delete [] h_frame_array;
        h_frame_array = 0;
        h_frame_array_size = 0;
        h_viewer->show();
    }
}


// Function to display a new topic, or respond to a link.
//
QThelpDlg::NTtype
QThelpDlg::newtopic(const char *href, bool spawn, bool force_download,
    bool nonrelative)
{
    HLPtopic *newtop;
    char hanchor[128];
    if (HLP()->context()->resolveKeyword(href, &newtop, hanchor, this,
            h_cur_topic, force_download, nonrelative))
        return (NThandled);
    if (!newtop) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Unresolved link: %s.", href);
        PopUpErr(MODE_ON, buf);
        return (NTnone);
    }
    if (spawn)
        newtop->set_context(0);
    else
        newtop->set_context(this);

    newtop->link_new_and_show(spawn, h_cur_topic);
    return (NTnew);
}


QThelpDlg::NTtype
QThelpDlg::newtopic(const char *fname, FILE *fp, bool spawn)
{
    HLPtopic *top = new HLPtopic(fname, "");
    top->get_file(fp, fname);

    if (spawn)
        top->set_context(0);
    else
        top->set_context(this);

    top->link_new_and_show(spawn, h_cur_topic);
    return (QThelpDlg::NTnew);
}


// Halt all image transfers in progress for the widget, and clear the
// queue of jobs for this window
//
void
QThelpDlg::stop_image_download()
{
    if (h_params->LoadMode == HLPparams::LoadProgressive)
        h_viewer->progressive_kill();
    HLP()->context()->abortImageDownload(this);
}


// Static function.
// Callback passed to PopUpInput to set a proxy url.
//
// The string is in the form "url [port]", where the port can be part
// of the url, separated by a colon.  In this case, the second token
// should not be given.  An explicit port number must be provided by
// either means.
//
void
QThelpDlg::h_proxy_proc(const char *str, void *hlpptr)
{
    QThelpDlg *w = static_cast<QThelpDlg*>(hlpptr);
    if (!w || !str)
        return;
    char buf[256];
    char *addr = lstring::getqtok(&str);

    // If not address given, convert to "-", which indicates to move
    // .wrproxy -> .wrproxy.bak
    if (!addr)
        addr = lstring::copy("-");
    else if (!*addr) {
        delete [] addr;
        addr = lstring::copy("-");
    }
    if (*addr == '-' || *addr == '+') {
        const char *err = proxy::move_proxy(addr);
        if (err) {
            snprintf(buf, 256, "Operation failed: %s.", err);
            w->PopUpErr(MODE_ON, buf);
        }
        else {
            const char *t = addr+1;
            if (!*t)
                t = "bak";
            if (*addr == '-') {
                snprintf(buf, 256,
                    "Move .wrproxy file to .wrproxy.%s succeeded.\n", t);
                w->PopUpMessage(buf, false);
            }
            else {
                snprintf(buf, 256,
                    "Move .wrproxy.%s to .wrproxy file succeeded.\n", t);
                w->PopUpMessage(buf, false);
            }
        }
        delete [] addr;
        if (w->wb_input)
            w->wb_input->popdown();
        return;
    }
    if (!lstring::prefix("http:", addr)) {
        w->PopUpMessage("Error: \"http:\" prefix required in address.", true);
        delete [] addr;
        if (w->wb_input)
            w->wb_input->popdown();
        return;
    }

    bool a_has_port = false;
    const char *e = strrchr(addr, ':');
    if (e) {
        e++;
        if (isdigit(*e)) {
            e++;
            while (isdigit(*e))
                e++;
        }
        if (!*e)
            a_has_port = true;
    }

    char *port = lstring::gettok(&str, ":");
    if (!a_has_port && !port) {
        // Default to port 80.
        port = lstring::copy("80");
    }
    if (port) {
        for (const char *c = port; *c; c++) {
            if (!isdigit(*c)) {
                w->PopUpMessage("Error: port is not numeric.", true);
                delete [] addr;
                delete [] port;
                if (w->wb_input)
                    w->wb_input->popdown();
                return;
            }
        }
    }

    const char *err = proxy::set_proxy(addr, port);
    if (err) {
        snprintf(buf, 256, "Operation failed: %s.", err);
        w->PopUpErr(MODE_ON, buf);
    }
    else if (port) {
        snprintf(buf, 256, "Created .wrproxy file for %s:%s.\n", addr, port);
        w->PopUpMessage(buf, false);
    }
    else {
        snprintf(buf, 256, "Created .wrproxy file for %s.\n", addr);
        w->PopUpMessage(buf, false);
    }
    delete [] addr;
    delete [] port;
    if (w->wb_input)
        w->wb_input->popdown();
}


#define MOZY_FIFO "mozyfifo"

// Experimental new feature:  create a named pipe and set up a
// listener.  When anything is written to the pipe, grab it and show
// it.  This is intended for displaying HTML messages from an email
// client.
//
bool 
QThelpDlg::register_fifo(const char *fname)
{
    if (!fname)
        fname = getenv("MOZY_FIFO");
    if (!fname)
        fname = MOZY_FIFO;
        
    bool ret = false;
#ifdef WIN32
    if (fname)
        fname = lstring::strip_path(fname);
    sLstr lstr;
    lstr.add("\\\\.\\pipe\\");
    lstr.add(fname);
    int len = lstr.length();
    int cnt = 1;
    for (;;) {
        if (access(lstr.string(), F_OK) < 0)
            break;
        lstr.truncate(len, 0);
        lstr.add_i(cnt);
        cnt++;
    }
    SECURITY_DESCRIPTOR sd;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = false;

    // Convert to WCHAR, didn't need this in GTK!
    len = strlen(lstr.string());
    WCHAR *wcbuf = new WCHAR[len+1];
    mbstowcs(wcbuf, lstr.string(), len+1);
    HANDLE hpipe = CreateNamedPipe(wcbuf,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        2048,
        2048,
        NMPWAIT_USE_DEFAULT_WAIT,
        &sa);
    delete [] wcbuf;

    if (hpipe != INVALID_HANDLE_VALUE) {
        ret = true;
        delete [] h_fifo_name;
        h_fifo_name = lstr.string_trim();
        h_fifo_pipe = hpipe;
        _beginthread(pipe_thread_proc, 0, this);
    }
#else
    sLstr lstr;
    passwd *pw = getpwuid(getuid());
    if (pw == 0) {
        GRpkg::self()->Perror("getpwuid");
        char *cwd = getcwd(0, 0);
        lstr.add(cwd);
        if (strcmp(cwd, "/"))
            lstr.add_c('/');
        free(cwd);
    }
    else {
        lstr.add(pw->pw_dir);
        lstr.add_c('/');
    }
    lstr.add(fname);
    int len = lstr.length();
    int cnt = 1;
    for (;;) {
        if (access(lstr.string(), F_OK) < 0)
            break;
        lstr.truncate(len, 0);
        lstr.add_i(cnt);
        cnt++;
    }
    if (!mkfifo(lstr.string(), 0666)) {
        ret = true;
        delete [] h_fifo_name;
        h_fifo_name = lstr.string_trim();
        filestat::queue_deletion(h_fifo_name);
    }
#endif

    if (h_fifo_tid)
        QTdev::self()->RemoveTimer(h_fifo_tid);
    h_fifo_tid = QTdev::self()->AddTimer(100, fifo_check_proc, this);

    return (ret);
}


void 
QThelpDlg::unregister_fifo()
{
    if (h_fifo_tid) {
        QTdev::self()->RemoveTimer(h_fifo_tid);
        h_fifo_tid = 0;
    }
    if (h_fifo_name) {
#ifdef WIN32
        unlink(h_fifo_name);
        delete [] h_fifo_name;
        h_fifo_name = 0;
        if (h_fifo_pipe) {
            CloseHandle(h_fifo_pipe);
            h_fifo_pipe = 0;
        }
#else
        if (h_fifo_fd > 0)
            ::close(h_fifo_fd);
        h_fifo_fd = -1;
        unlink(h_fifo_name);
        delete [] h_fifo_name;
        h_fifo_name = 0;
#endif
    }
#ifdef WIN32
    stringlist *sl = h_fifo_tfiles;
    h_fifo_tfiles = 0;
    while (sl) {
        stringlist *sx = sl;
        sl = sl->next;
        unlink(sx->string);
        delete [] sx->string;
        delete sx;
    }
#endif
}


#ifdef WIN32

// Static function.
// Thread procedure to listen on the named pipe.
//
void
QThelpDlg::pipe_thread_proc(void *arg)
{
    QThelpDlg *hw = (QThelpDlg*)arg;
    if (!hw)
        return;

    for (;;) {
        HANDLE hpipe = hw->h_fifo_pipe;
        if (!hpipe)
            return;

        if (ConnectNamedPipe(hpipe, 0)) {

            if (hw->h_fifo_name) {
                char *tempfile = filestat::make_temp("mz");
                FILE *fp = fopen(tempfile, "w");
                if (fp) {
                    unsigned int total = 0;
                    char buf[2048];
                    for (;;) {
                        DWORD bytes_read;
                        bool ok = ReadFile(hpipe, buf, 2048, &bytes_read, 0);
                        if (!ok)
                            break;
                        if (bytes_read > 0) {
                            fwrite(buf, 1, bytes_read, fp);
                            total += bytes_read;
                        }
                    }
                    fclose(fp);
                    if (total > 0) {
                        hw->h_fifo_tfiles =
                            new stringlist(tempfile, hw->h_fifo_tfiles);
                        tempfile = 0;
                    }
                }
                else
                    perror(tempfile);
                delete [] tempfile;
            }
            DisconnectNamedPipe(hpipe);
        }
    }
}

#endif


// Static timer callback function.
//
int 
QThelpDlg::fifo_check_proc(void *arg)
{
    QThelpDlg *hw = (QThelpDlg*)arg;
    if (!hw)
        return (0);

#ifdef WIN32
    if (hw->h_fifo_tfiles) {
        stringlist *sl = hw->h_fifo_tfiles;
        hw->h_fifo_tfiles = sl->next;
        char *tempfile = sl->string;
        delete sl;
        FILE *fp = fopen(tempfile, "r");
        if (fp) {
            hw->newtopic("fifo", fp, false);
            fclose(fp);
        }
        unlink(tempfile);
        delete [] tempfile;
    }
    return (1);

#else
    // Unfortunately, the stat and open calls appear to fail with
    // Mingw, so can't use this.

    struct stat st;
    if (stat(hw->h_fifo_name, &st) < 0 || !(st.st_mode & S_IFIFO))
        return (1);

    if (hw->h_fifo_fd < 0) {
        if (hw->h_fifo_name)
            hw->h_fifo_fd = ::open(hw->h_fifo_name, O_RDONLY | O_NONBLOCK);
        if (hw->h_fifo_fd < 0) {
            hw->h_fifo_tid = 0;
            return (0);
        }
    }
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(hw->h_fifo_fd, &readfds);
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500;
    int i = select(hw->h_fifo_fd + 1, &readfds, 0, 0, &timeout);
    if (i < 0) {
        // interrupted
        return (1);
    }
    if (i == 0) {
        // nothing to read
        // return (1);
    }
    if (FD_ISSET(hw->h_fifo_fd, &readfds)) {
        FILE *fp = fdopen(hw->h_fifo_fd, "r");
        if (!fp) {
            // Something wrong, close the fd and try again.
            ::close(hw->h_fifo_fd);
            hw->h_fifo_fd = -1;
            return (1);
        }
        hw->newtopic("fifo", fp, false);
        fclose(fp);  // closes fd, too
        hw->h_fifo_fd = -1;
    }
    return (1);
#endif
}

