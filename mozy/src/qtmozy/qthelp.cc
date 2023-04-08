
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

#include "help/help_startup.h"
#include "help/help_cache.h"
#include "httpget/transact.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"

#include "qthelp.h"
#include "qtviewer.h"
#include "qtinterf/qtfile.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qtinput.h"
#include "qtinterf/qtlist.h"
#include "queue_timer.h"

#include <QApplication>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QResizeEvent>
#include <QScrollBar>

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

// Help keywords used in this file
//  helpsys

// Architecture note
// Each QThelpPopup has a corresponding "root_topic" topic struct
// linked into HLP()->context()->TopList.  In these topics, the
// context field points back to the QThelpPopup.  Each new page
// displayed in the QThelpPopup window has a corresponding topic struct
// linked from lastborn in the root_topic, and has the cur_topic field
// set to point to the topic.  Topics linked to lastborn do not have
// the context field set.

static queue_timer QueueTimer;
QueueLoop &help_context::hcxImageQueueLoop = QueueTimer;

// Initiate queue processing.
//
void
queue_timer::start()
{
    if (!timer) {
        timer = new QTimer(0);
        timer->setInterval(50);
        connect(timer, SIGNAL(timeout()), this, SLOT(run_queue_slot()));
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
        action_item(bookmark_t *b, QObject *prnt) : QAction(prnt)
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

        void setBookmark(bookmark_t *b) { ai_bookmark = b; }
        bookmark_t *bookmark() { return (ai_bookmark); }

    private:
        bookmark_t *ai_bookmark;
    };
}


//-----------------------------------------------------------------------------
// Local declarations

// XPM 
static const char * const forward_xpm[] = {
"16 16 4 1",
" 	c none",
".	c #00dd00",
"x	c #00ee00",
"+  c #00ff00",
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
static const char * const backward_xpm[] = {
"16 16 4 1",
" 	c none",
".	c #00dd00",
"x	c #00ee00",
"+  c #00ff00",
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
static const char * const stop_xpm[] = {
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

//-----------------------------------------------------------------------------
// Exports

// Top level help popup call, takes care of accessing the database. 
// Return false if the topic is not found
//
bool
qt_bag::PopUpHelp(const char *wordin)
{
    if (!this)
        return (false);
    if (!HLP()->get_path(0)) {
        PopUpErr(MODE_ON, "Error: no path to database.");
        HLP()->context()->quitHelp();
        return (false);
    }
    char buf[256];
    buf[0] = 0;
    char *word = buf;
    if (wordin) {
        strcpy(buf, wordin);
        char *s = buf + strlen(buf) - 1;
        while (*s == ' ')
            *s-- = '\0';
    }

    topic *top = 0;
    if (HLP()->context()->resolveKeyword(word, &top, 0, 0, 0, false, true))
        return (false);
    if (!top) {
        if (word && *word)
            sprintf(buf, "Error: No such topic: %s\n", word);
        else
            sprintf(buf, "Error: no top level topic\n");
        PopUpErr(MODE_ON, buf);
        return (false);
    }
    top->show_in_window();
    return (true);
}


HelpWidget *
HelpWidget::get_widget(topic *top)
{
    if (!top)
        return (0);
    return (dynamic_cast<QThelpPopup*>(top->get_parent()->context()));
}


HelpWidget *
HelpWidget::new_widget(GRwbag **ptr, int, int)
{
    QWidget *parent = 0;
    if (GRpkgIf()->MainWbag()) {
        qt_bag *wb = dynamic_cast<qt_bag*>(GRpkgIf()->MainWbag());
        if (wb)
            parent = wb->shell_widget();
    }

    QThelpPopup *w = new QThelpPopup(true, parent);
    if (ptr)
        *ptr = w;
    w->show();
    return (w);
}


//-----------------------------------------------------------------------------
// Constructor/destrucor

static void sens_set(qt_bag*, bool);

QThelpPopup::QThelpPopup(bool has_menu, QWidget *prnt) : QWidget(prnt),
    qt_bag(this)
{
    // If has_menu is false, the widget will not have the menu or the
    // status bar visible.
    params = 0;
    root_topic = 0;
    cur_topic = 0;
    stop_btn_pressed = false;
    is_frame = !has_menu;
    cache_list = 0;
    frame_array = 0;
    frame_array_size = 0;
    frame_parent = 0;
    frame_name = 0;

    sens_set = ::sens_set;

    menubar = new QMenuBar(this);
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
    html_viewer = new viewer_w(500, 400, this, this);
    status_bar = new QStatusBar(this);
    if (!has_menu)
        status_bar->hide();
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(4);
    vbox->setSpacing(2);
    vbox->setMenuBar(menubar);
    vbox->addWidget(html_viewer);
    vbox->addWidget(status_bar);

    if (!is_frame)
        params = new HlpParams(HLP()->no_file_fonts());

    html_viewer->freeze();

    a_Backward = menubar->addAction(QString("back"),
        this, SLOT(backward_slot()));
    a_Backward->setIcon(QIcon(QPixmap(backward_xpm)));

    a_Forward = menubar->addAction(QString("forw"),
        this, SLOT(forward_slot()));
    a_Forward->setIcon(QIcon(QPixmap(forward_xpm)));

    a_Stop = menubar->addAction(QString("stop"),
        this, SLOT(stop_slot()));
    a_Stop->setIcon(QIcon(QPixmap(stop_xpm)));

    main_menus[0] = new QMenu(this);
    main_menus[0]->setTitle(tr("&File"));
    menubar->addMenu(main_menus[0]);
    a_Open = main_menus[0]->addAction(QString(tr("&Open")),
        this, SLOT(open_slot()), Qt::CTRL+Qt::Key_O);
    a_OpenFile = main_menus[0]->addAction(QString(tr("Open &File")),
        this, SLOT(open_file_slot()), Qt::CTRL+Qt::Key_F);
    a_Save = main_menus[0]->addAction(QString(tr("&Save")),
        this, SLOT(save_slot()), Qt::CTRL+Qt::Key_S);
    a_Print = main_menus[0]->addAction(QString(tr("&Print")),
        this, SLOT(print_slot()), Qt::CTRL+Qt::Key_P);
    a_Reload = main_menus[0]->addAction(QString(tr("&Reload")),
        this, SLOT(reload_slot()), Qt::CTRL+Qt::Key_R);
    main_menus[0]->addSeparator();
    a_Quit = main_menus[0]->addAction(QString(tr("&Quit")),
        this, SLOT(quit_slot()), Qt::CTRL+Qt::Key_Q);

    main_menus[1] = new QMenu(this);
    main_menus[1]->setTitle(tr("&Options"));
    menubar->addMenu(main_menus[1]);
    a_Search = main_menus[1]->addAction(QString(tr("S&earch")),
        this, SLOT(search_slot()));
    a_FindText = main_menus[1]->addAction(QString(tr("Find Text")),
        this, SLOT(find_slot()));
    a_SetFont = main_menus[1]->addAction(QString(tr("Set &Font")));
    a_SetFont->setCheckable(true);
    connect(a_SetFont, SIGNAL(toggled(bool)),
        this, SLOT(set_font_slot(bool)));
    a_DontCache = main_menus[1]->addAction(QString(tr("&Don't Cache")));
    a_DontCache->setCheckable(true);
    connect(a_DontCache, SIGNAL(toggled(bool)),
        this, SLOT(dont_cache_slot(bool)));
    a_DontCache->setChecked(params->NoCache);
    a_ClearCache = main_menus[1]->addAction(QString(tr("&Clear Cache")),
        this, SLOT(clear_cache_slot()));
    a_ReloadCache = main_menus[1]->addAction(QString(tr("&Reload Cache")),
        this, SLOT(reload_cache_slot()));
    a_ShowCache = main_menus[1]->addAction(QString(tr("Show Cache")),
        this, SLOT(show_cache_slot()));
    main_menus[1]->addSeparator();
    a_NoCookies = main_menus[1]->addAction(QString(tr("No Cookies")));
    a_NoCookies->setCheckable(true);
    connect(a_NoCookies, SIGNAL(toggled(bool)),
        this, SLOT(no_cookies_slot(bool)));
    a_NoCookies->setChecked(params->NoCookies);

    QActionGroup *ag = new QActionGroup(this);
    a_NoImages = main_menus[1]->addAction(QString(tr("No Images")));
    a_NoImages->setCheckable(true);
    connect(a_NoImages, SIGNAL(toggled(bool)),
        this, SLOT(no_images_slot(bool)));
    ag->addAction(a_NoImages);
    a_SyncImages = main_menus[1]->addAction(QString(tr("Sync Images")));
    a_SyncImages->setCheckable(true);
    connect(a_SyncImages, SIGNAL(toggled(bool)),
        this, SLOT(sync_images_slot(bool)));
    ag->addAction(a_SyncImages);
    a_DelayedImages =
        main_menus[1]->addAction(QString(tr("Delayed Images")));
    a_DelayedImages->setCheckable(true);
    connect(a_DelayedImages, SIGNAL(toggled(bool)),
        this, SLOT(delayed_images_slot(bool)));
    ag->addAction(a_DelayedImages);
    a_ProgressiveImages =
        main_menus[1]->addAction(QString(tr("Progressive Images")));
    a_ProgressiveImages->setCheckable(true);
    connect(a_ProgressiveImages, SIGNAL(toggled(bool)),
        this, SLOT(progressive_images_slot(bool)));
    ag->addAction(a_ProgressiveImages);
    if (params->LoadMode == HlpParams::LoadProgressive)
        a_ProgressiveImages->setChecked(true);
    else if (params->LoadMode == HlpParams::LoadDelayed)
        a_DelayedImages->setChecked(true);
    else if (params->LoadMode == HlpParams::LoadSync)
        a_SyncImages->setChecked(true);
    else
        a_NoImages->setChecked(true);

    main_menus[1]->addSeparator();

    ag = new QActionGroup(this);
    a_AnchorPlain = main_menus[1]->addAction(QString(tr("Anchor Plain")));
    a_AnchorPlain->setCheckable(true);
    connect(a_AnchorPlain, SIGNAL(toggled(bool)),
        this, SLOT(anchor_plain_slot(bool)));
    ag->addAction(a_AnchorPlain);
    a_AnchorButtons =
        main_menus[1]->addAction(QString(tr("Anchor Buttons")));
    a_AnchorButtons->setCheckable(true);
    connect(a_AnchorButtons, SIGNAL(toggled(bool)),
        this, SLOT(anchor_buttons_slot(bool)));
    ag->addAction(a_AnchorButtons);
    a_AnchorUnderline =
        main_menus[1]->addAction(QString(tr("Anchor Underline")));
    a_AnchorUnderline->setCheckable(true);
    connect(a_AnchorUnderline, SIGNAL(toggled(bool)),
        this, SLOT(anchor_underline_slot(bool)));
    ag->addAction(a_AnchorUnderline);
    if (params->AnchorButtons)
        a_AnchorButtons->setChecked(true);
    else if (params->AnchorUnderlined)
        a_AnchorUnderline->setChecked(true);
    else
        a_AnchorPlain->setChecked(true);

    a_AnchorHighlight =
        main_menus[1]->addAction(QString(tr("Anchor Highlight")));
    a_AnchorHighlight->setCheckable(true);
    connect(a_AnchorHighlight, SIGNAL(toggled(bool)),
        this, SLOT(anchor_highlight_slot(bool)));
    a_AnchorHighlight->setChecked(params->AnchorHighlight);
    a_BadHTML = main_menus[1]->addAction(QString(tr("Bad HTML Warnings")));
    a_BadHTML->setCheckable(true);
    connect(a_BadHTML, SIGNAL(toggled(bool)),
        this, SLOT(bad_html_slot(bool)));
    a_BadHTML->setChecked(params->BadHTMLwarnings);
    a_FreezeAnimations =
        main_menus[1]->addAction(QString(tr("Freeze Animations")));
    a_FreezeAnimations->setCheckable(true);
    connect(a_FreezeAnimations, SIGNAL(toggled(bool)),
        this, SLOT(freeze_animations_slot(bool)));
    a_FreezeAnimations->setChecked(params->FreezeAnimations);
    a_LogTransactions =
        main_menus[1]->addAction(QString(tr("Log Transactions")));
    a_LogTransactions->setCheckable(true);
    connect(a_LogTransactions, SIGNAL(toggled(bool)),
        this, SLOT(log_transactions_slot(bool)));
    a_LogTransactions->setChecked(params->PrintTransact);

    main_menus[2] = new QMenu(this);
    main_menus[2]->setTitle(tr("&Bookmarks"));
    menubar->addMenu(main_menus[2]);
    a_AddBookmark = main_menus[2]->addAction(QString(tr("Add")),
        this, SLOT(add_slot()));
    a_DeleteBookmark = main_menus[2]->addAction(QString(tr("Delete")),
        this, SLOT(delete_slot()));
    a_DeleteBookmark->setCheckable(true);
    main_menus[2]->addSeparator();

    menubar->addSeparator();
    main_menus[3] = new QMenu(this);
    main_menus[3]->setTitle(tr("&Help"));
    menubar->addMenu(main_menus[3]);
    a_Help = main_menus[3]->addAction(QString(tr("&Help")),
        this, SLOT(help_slot()), Qt::CTRL+Qt::Key_H);

    const char *fn = FC.getName(FNT_MOZY);
    if (fn) {
        html_viewer->set_font(fn);
        FC.registerCallback(html_viewer, FNT_MOZY);
    }
    fn = FC.getName(FNT_MOZY_FIXED);
    if (fn) {
        html_viewer->set_fixed_font(fn);
        FC.registerCallback(html_viewer, FNT_MOZY_FIXED);
    }

    html_viewer->thaw();

    HLP()->context()->readBookmarks();
    for (bookmark_t *b = HLP()->context()->bookmarks(); b; b = b->next) {
        QString qs = QString(b->title);
        qs.truncate(32);
        main_menus[2]->addAction(
            new action_item(b, main_menus[2]));
    }
    connect(main_menus[2], SIGNAL(triggered(QAction*)), this,
        SLOT(bookmark_slot(QAction*)));

    a_Backward->setEnabled(false);
    a_Forward->setEnabled(false);
    a_Stop->setEnabled(false);
}


QThelpPopup::~QThelpPopup()
{
    FC.unregisterCallback(html_viewer, FNT_MOZY);
    FC.unregisterCallback(html_viewer, FNT_MOZY_FIXED);
    halt_images();
    HLP()->context()->removeTopic(root_topic);
    if (!is_frame)
        delete params;

    if (frame_array) {
        for (int i = 0; i < frame_array_size; i++)
            delete frame_array[i];
        delete [] frame_array;
    }
    delete [] frame_name;
}


void
QThelpPopup::menu_sens_set(bool set)
{
    a_Search->setEnabled(set);
    a_Save->setEnabled(set);
    a_Open->setEnabled(set);
    a_FindText->setEnabled(set);
}


static void
sens_set(qt_bag *wp, bool set)
{
    QThelpPopup *w = dynamic_cast<QThelpPopup*>(wp);
    if (w)
        w->menu_sens_set(set);
}


//-----------------------------------------------------------------------------
// ViewWidget and HelpWidget virtual function overrides

void
QThelpPopup::freeze()
{
    html_viewer->freeze();
}


void
QThelpPopup::thaw()
{
    html_viewer->thaw();
}


// Set a pointer to the Transaction struct.
//
void
QThelpPopup::set_transaction(Transaction *t, const char *cookiedir)
{
    html_viewer->set_transaction(t, cookiedir);
    if (t) {
        t->t_timeout = params->Timeout;
        t->t_retry = params->Retries;
        t->t_http_port = params->HTTP_Port;
        t->t_ftp_port = params->FTP_Port;
        t->t_http_debug = params->DebugMode;
        if (params->PrintTransact)
            t->t_logfile = lstring::copy("stderr");
        if (cookiedir && !params->NoCookies) {
            t->t_cookiefile = new char [strlen(cookiedir) + 20];
            sprintf(t->t_cookiefile, "%s/%s", cookiedir, "cookies");
        }
    }
}


// Return a pointer to the transaction struct.
//
Transaction *
QThelpPopup::get_transaction()
{
    return (html_viewer->get_transaction());
}


// Check if the transfer should be aborted, return true if so.
//
bool
QThelpPopup::check_halt_processing(bool run_events)
{
    if (run_events) {
        qApp->processEvents(QEventLoop::AllEvents);
        if (!a_Stop->isEnabled() && stop_btn_pressed)
            return (true);
    }
    else if (stop_btn_pressed)
        return (true);
    return (false);
}


// Enable/disable halt button sensitivity.
//
void
QThelpPopup::set_halt_proc_sens(bool set)
{
    a_Stop->setEnabled(set);
}


// Write text on the status line.
//
void
QThelpPopup::set_status_line(const char *msg)
{
    if (frame_parent)
        frame_parent->set_status_line(msg);
    else
        status_bar->showMessage(QString(msg));
}


htmImageInfo *
QThelpPopup::new_image_info(const char *url, bool progressive)
{
    return (html_viewer->new_image_info(url, progressive));
}


bool
QThelpPopup::call_plc(const char *url)
{
    return (html_viewer->call_plc(url));
}


htmImageInfo *
QThelpPopup::image_procedure(const char *url)
{
    return (html_viewer->image_procedure(url));
}


void
QThelpPopup::image_replace(htmImageInfo *image, htmImageInfo *new_image)
{
    html_viewer->image_replace(image, new_image);
}


bool
QThelpPopup::is_body_image(const char *url)
{
    if (HLP()->context()->isImageInList(this))
        return (false);
    return (html_viewer->is_body_image(url));
}


const char *
QThelpPopup::get_url()
{
    return (cur_topic ? cur_topic->keyword() : 0);
}


bool
QThelpPopup::no_url_cache()
{
    return (params->NoCache);
}


int
QThelpPopup::image_load_mode()
{
    return (params->LoadMode);
}


int
QThelpPopup::image_debug_mode()
{
    return (params->LocalImageTest);
}


GRwbag *
QThelpPopup::get_widget_bag()
{
    return (this);
}


//
// HelpWidget overrides
//

// Link and display the new topic.
//
void
QThelpPopup::link_new(topic *top)
{
    HLP()->context()->linkNewTopic(top);
    root_topic = top;
    cur_topic = top;
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
QThelpPopup::reuse(topic *newtop, bool newlink)
{
    if (root_topic->lastborn()) {
        // in the forward/back operations, the new topic is stitched in
        // as lastborn, and the scroll update is handled elsewhere
        if (root_topic->lastborn() != newtop)
            root_topic->lastborn()->set_topline(get_scroll_pos());
    }
    else if (newtop)
        root_topic->set_topline(get_scroll_pos());

    if (!newtop)
        newtop = root_topic;

    char *anchor = 0;
    newtop->set_show_plain(HLP()->context()->isPlain(newtop->keyword()));
    if (newtop != root_topic) {
        root_topic->set_text(newtop->get_text());
        newtop->clear_text();
    }
    else
        root_topic->get_text();
    cur_topic = newtop;
    const char *t = HLP()->context()->findAnchorRef(newtop->keyword());
    if (t)
        anchor = lstring::copy(t);
    if (newlink && newtop != root_topic) {
        newtop->set_sibling(root_topic->lastborn());
        root_topic->set_lastborn(newtop);
        newtop->set_parent(root_topic);
        a_Backward->setEnabled(true);
    }
    redisplay();
    if (anchor)
        set_scroll_pos(html_viewer->anchor_pos_by_name(anchor));
    else
        set_scroll_pos(newtop->get_topline());
    delete [] anchor;
    set_status_line(newtop->keyword());

    t = newtop->title();
    if (!t || !*t)
        t = html_viewer->get_title();
    if (!t || !*t)
        t = "Whiteley Research Inc.";
    char buf[256];
    sprintf(buf, "%s -- %s", HLP()->get_name(), t);
    strip_html(buf);
    setWindowTitle(QString(buf));
    QueueTimer.start();
}


// Halt any current display activity and redisplay.
//
void
QThelpPopup::redisplay()
{
    Transaction *t = html_viewer->get_transaction();
    if (t) {
        // still downloading previous page, abort
        t->set_abort();
        html_viewer->set_transaction(0, 0);
    }
    halt_images();
    if (!(root_topic->lastborn() ?
            root_topic->lastborn()->is_html() : root_topic->is_html()) &&
            HLP()->context()->isPlain(root_topic->lastborn() ?
            root_topic->lastborn()->keyword() : root_topic->keyword()))
        html_viewer->set_mime_type("text/plain");
    else
        html_viewer->set_mime_type("text/html");
    html_viewer->set_source(root_topic->get_cur_text());
}


topic *
QThelpPopup::get_topic()
{
    return (cur_topic);
}


// Unset the halt flag, which will be set if an abort is needed.
//
void
QThelpPopup::unset_halt_flag()
{
    stop_btn_pressed = false;
}


void
QThelpPopup::halt_images()
{
    stop_image_download();
    HLP()->context()->flushImages(this);
}


void
QThelpPopup::show_cache(int mode)
{
    if (mode == MODE_OFF) {
        if (cache_list)
            cache_list->popdown();
        return;
    }
    if (mode == MODE_UPD) {
        if (cache_list) {
            stringlist *s0 = HLP()->context()->listCache();
            cache_list->update(s0, "Cache Entries", 0);
            s0->free();
        }
        return;
    }
    if (cache_list)
        return;
    stringlist *s0 = HLP()->context()->listCache();
    cache_list = PopUpList(s0, "Cache Entries", 0, 0, 0, false);
    s0->free();
    if (cache_list) {
        cache_list->register_usrptr((void**)&cache_list);
        QTlistPopup *list = dynamic_cast<QTlistPopup*>(cache_list);
        if (list)
            connect(list, SIGNAL(action_call(const char*, void*)),
                this, SLOT(cache_choice_slot(const char*)));
    }
}


//-----------------------------------------------------------------------------
// htmDataInterface methods

// All viewer_w "signals" are dispatched from here.
//
void
QThelpPopup::emit_signal(SignalID id, void *payload)
{
    switch (id) {
    case S_ARM:
        // emit arm_slot(static_cast<htmCallbackInfo*>(payload));
        break;
    case S_ACTIVATE:
        newtopic_slot(static_cast<htmAnchorCallbackStruct*>(payload));
        break;
    case S_ANCHOR_TRACK:
        anchor_track_slot(static_cast<htmAnchorCallbackStruct*>(payload));
        break;
    case S_ANCHOR_VISITED:
        // anchor_visited_slot(static_cast<htmVisitedCallbackStruct*>(payload));
        break;
    case S_DOCUMENT:
        // document_slot(static_cast<htmDocumentCallbackStruct*>(payload));
        break;
    case S_LINK:
        // link_slot(static_cast<htmLinkCallbackStruct*>(payload));
        break;
    case S_FRAME:
        frame_slot(static_cast<htmFrameCallbackStruct*>(payload));
        break;
    case S_FORM:
        form_slot(static_cast<htmFormCallbackStruct*>(payload));
        break;
    case S_IMAGEMAP:
        // imagemap_slot(static_cast<htmImagemapCallbackStruct*>(payload));
        break;
    case S_HTML_EVENT:
        // html_event_slot(static_cast<htmEventCallbackStruct*>(payload));
        break;
    default:
        break;
    }
}


void *
QThelpPopup::event_proc(const char*)
{
    return (0);
}


void
QThelpPopup::panic_callback(const char*)
{
}


// Called by the widget to resolve image references.
//
htmImageInfo *
QThelpPopup::image_resolve(const char *fname)
{
    if (!fname)
        return (0);
    return (HLP()->context()->imageResolve(fname, this));
}


// This is the "get_data" callback for progressive image loading.
//
int
QThelpPopup::get_image_data(htmPLCStream *stream, void *buffer)
{
    sImageList *im = (sImageList*)stream->user_data;
    return (HLP()->context()->getImageData(im, stream, buffer));
}


// This is the "end_data" callback for progressive image loading.
//
void
QThelpPopup::end_image_data(htmPLCStream *stream, void*, int type, bool)
{
    if (type == PLC_IMAGE) {
        sImageList *im = (sImageList*)stream->user_data;
        HLP()->context()->inactivateImages(im);
    }
}


// This returns the client area that can be used for display frames. 
// We use the entire widget width, and the height between the menu and
// status bar.
//
void
QThelpPopup::frame_rendering_area(htmRect *rct)
{
    QRect r1 = html_viewer->frameGeometry();
    QRect r2 = status_bar->frameGeometry();
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
QThelpPopup::get_frame_name()
{
    return (frame_name);
}


// Return the keyword and title from the current topic, if possible.
//
void
QThelpPopup::get_topic_keys(char **pkw, char **ptitle)
{
    if (pkw)
        *pkw = 0;
    if (ptitle)
        *ptitle = 0;
    topic *t = cur_topic;
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
QThelpPopup::scroll_visible(int l, int t, int r, int b)
{
/*XXX
    const int slop = 20;
    int spy = htmlview->get_scroll_pos(false);
    int spx = htmlview->get_scroll_pos(true);
    htmRect r;
    htmlview->client_area(&r);
    if (y < spy + slop || y > spy + (int)r.height - slop) {
        y -= slop;
        if (y < 0)
            y = 0;
        htmlview->set_scroll_pos(y, false);
    }
    if (x < spx || x > spx + (int)r.width)
        htmlview->set_scroll_pos(x, true);
*/
(void)l;
(void)t;
(void)r;
(void)b;
}


//-----------------------------------------------------------------------------
// qt_bag functions

char *
QThelpPopup::GetPostscriptText(int font_family, const char *url,
    const char *title, bool use_headers, bool a4)
{
    return (html_viewer->get_postscript_text(font_family, url, title,
        use_headers, a4));
}


char *
QThelpPopup::GetPlainText()
{
    return (html_viewer->get_plain_text());
}


char *
QThelpPopup::GetHtmlText()
{
    return (html_viewer->get_html_text());
}


//-----------------------------------------------------------------------------
// Misc.

// scrollbar setting
int
QThelpPopup::get_scroll_pos(bool horiz)
{
    QScrollBar *sb;
    if (horiz)
        sb = html_viewer->horizontalScrollBar();
    else
        sb = html_viewer->verticalScrollBar();
    if (!sb)
        return (0);
    return (sb->value());
}


void
QThelpPopup::set_scroll_pos(int posn, bool horiz)
{
    QScrollBar *sb;
    if (horiz)
        sb = html_viewer->horizontalScrollBar();
    else
        sb = html_viewer->verticalScrollBar();
    if (!sb)
        return;
    sb->setValue(posn);
}


//-----------------------------------------------------------------------------
// Slots

void
QThelpPopup::backward_slot()
{
    topic *top = root_topic;
    if (top && top->lastborn()) {
        topic *last = top->lastborn();
        top->set_lastborn(last->sibling());
        last->set_sibling(top->next());
        top->set_next(last);
        last->set_topline(get_scroll_pos());
        top->reuse(top->lastborn(), false);

        a_Backward->setEnabled(top->lastborn());
        a_Forward->setEnabled(true);
   }
}


void
QThelpPopup::forward_slot()
{
    topic *top = root_topic;
    if (top && top->next()) {
        topic *next = top->next();
        top->set_next(next->sibling());
        next->set_sibling(top->lastborn());
        top->set_lastborn(next);
        if (next->sibling())
            next->sibling()->set_topline(get_scroll_pos());
        else
            top->set_topline(get_scroll_pos());
        top->reuse(top->lastborn(), false);

        a_Forward->setEnabled(top->next());
        a_Backward->setEnabled(true);
    }
}


void
QThelpPopup::stop_slot()
{
    stop_image_download();
    a_Stop->setEnabled(false);
    stop_btn_pressed = true;
}


void
QThelpPopup::open_slot()
{
    PopUpInput("Enter keyword, file name, or URL", "", "Open", 0, 0);
    connect(input, SIGNAL(action_call(const char*, void*)), this,
        SLOT(do_open_slot(const char*, void*)));
}


void
QThelpPopup::open_file_slot()
{
    GRfilePopup *fs = PopUpFileSelector(fsSEL, GRloc(), 0, 0, 0, 0);
    QTfilePopup *fsel = dynamic_cast<QTfilePopup*>(fs);
    if (fsel)
        connect(fsel, SIGNAL(file_selected(const char*, void*)),
            this, SLOT(do_open_slot(const char*, void*)));
}


void
QThelpPopup::save_slot()
{
    PopUpInput(0, "", "Save", 0, 0);
    connect(input, SIGNAL(action_call(const char*, void*)), this,
        SLOT(do_save_slot(const char*, void*)));
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
QThelpPopup::print_slot()
{
    if (!hlpHCcb.command)
        hlpHCcb.command = lstring::copy(GRappIf()->GetPrintCmd());
    PopUpPrint(0, &hlpHCcb, HCtextPS);
}


void
QThelpPopup::reload_slot()
{
    cur_topic->set_topline(get_scroll_pos());
    newtopic(cur_topic->keyword(), false, false, true);
}


void
QThelpPopup::quit_slot()
{
    emit dismiss();
}


void
QThelpPopup::search_slot()
{
    PopUpInput("Enter keyword for database search:", "", "Search", 0, 0);
    connect(input, SIGNAL(action_call(const char*, void*)), this,
        SLOT(do_search_slot(const char*, void*)));
}


void
QThelpPopup::find_slot()
{
    PopUpInput("Enter word for text search:", "", "Find Text", 0, 0);
    connect(input, SIGNAL(action_call(const char*, void*)), this,
        SLOT(do_find_text_slot(const char*, void*)));
}


void
QThelpPopup::set_font_slot(bool set)
{
    if (set) {
        PopUpFontSel(0, GRloc(), MODE_ON, 0, 0, FNT_MOZY);
        connect(fontsel, SIGNAL(dismiss()), this, SLOT(font_down_slot()));
        connect(fontsel, SIGNAL(select_action(int, const char*, void*)),
            this, SLOT(font_selected_slot(int, const char*)));
    }
    else
        PopUpFontSel(0, GRloc(), MODE_OFF, 0, 0, 0);
}


// Handle font selection in pop-up.
void
QThelpPopup::font_selected_slot(int font_id, const char *fontname)
{
    /*XXX
    if (font_id == FNT_MOZY) {
        params->store("FontFamily", fontname);
        html_viewer->set_font(fontname);
    }
    else if (font_id == FNT_MOZY_FIXED) {
        params->store("FixedFontFamily", fontname);
        html_viewer->set_fixed_font(fontname);
    }
    */
}


// Handle font selector dismissal.
//
void
QThelpPopup::font_down_slot()
{
    a_SetFont->setChecked(false);
}


void
QThelpPopup::dont_cache_slot(bool set)
{
    params->NoCache = set;
}


void
QThelpPopup::clear_cache_slot()
{
    HLP()->context()->clearCache();
}


void
QThelpPopup::reload_cache_slot()
{
    HLP()->context()->reloadCache();
}


void
QThelpPopup::show_cache_slot()
{
    show_cache(MODE_ON);
}


void
QThelpPopup::no_cookies_slot(bool set)
{
    params->NoCookies = set;
}


void
QThelpPopup::no_images_slot(bool set)
{
    if (set) {
        stop_image_download();
        params->LoadMode = HlpParams::LoadNone;
    }
}


void
QThelpPopup::sync_images_slot(bool set)
{
    if (set) {
        stop_image_download();
        params->LoadMode = HlpParams::LoadSync;
    }
}


void
QThelpPopup::delayed_images_slot(bool set)
{
    if (set) {
        stop_image_download();
        params->LoadMode = HlpParams::LoadDelayed;
    }
}


void
QThelpPopup::progressive_images_slot(bool set)
{
    if (set) {
        stop_image_download();
        params->LoadMode = HlpParams::LoadProgressive;
    }
}


void
QThelpPopup::anchor_plain_slot(bool set)
{
    if (set) {
        params->AnchorButtons = false;
        params->AnchorUnderlined = false;
        html_viewer->set_anchor_style(ANC_PLAIN);

        int position = get_scroll_pos();
        set_scroll_pos(position);
    }
}


void
QThelpPopup::anchor_buttons_slot(bool set)
{
    if (set) {
        params->AnchorButtons = true;
        params->AnchorUnderlined = false;
        html_viewer->set_anchor_style(ANC_BUTTON);
    }
}


void
QThelpPopup::anchor_underline_slot(bool set)
{
    if (set) {
        params->AnchorButtons = false;
        params->AnchorUnderlined = true;
        html_viewer->set_anchor_style(ANC_SINGLE_LINE);

        int position = get_scroll_pos();
        set_scroll_pos(position);
    }
}


void
QThelpPopup::anchor_highlight_slot(bool set)
{
    params->AnchorHighlight = set;
    html_viewer->set_anchor_highlighting(set);
}


void
QThelpPopup::bad_html_slot(bool set)
{
    params->BadHTMLwarnings = set;
    html_viewer->set_html_warnings(set);
}


void
QThelpPopup::freeze_animations_slot(bool set)
{
    params->FreezeAnimations = set;
    html_viewer->set_freeze_animations(set);
}


void
QThelpPopup::log_transactions_slot(bool set)
{
    params->PrintTransact = set;
}


void
QThelpPopup::add_slot()
{
    if (!cur_topic)
        return;
    topic *tp = cur_topic;
    const char *ptitle = tp->title();
    char *title;
    if (ptitle && *ptitle)
        title = lstring::copy(ptitle);
    else
        title = lstring::copy(html_viewer->get_title());
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
    bookmark_t *b = HLP()->context()->bookmarkUpdate(title, url);
    delete [] title;
    delete [] url;

    QString qs = QString(b->title);
    qs.truncate(32);
    main_menus[2]->addAction(
        new action_item(b, main_menus[2]));
}


void
QThelpPopup::delete_slot()
{
}


static void
yncb(bool val, void *client_data)
{
    if (val)
        delete static_cast<action_item*>(client_data);
}


// Handler for bookmarks selected in the menu.
// 
void
QThelpPopup::bookmark_slot(QAction *action)
{
    action_item *ac = dynamic_cast<action_item*>(action);
    if (ac) {
        if (a_DeleteBookmark->isChecked()) {
            char buf[256];
            strcpy(buf, "Delete ");
            char *t = buf + strlen(buf);
            strncpy(t, ac->bookmark()->title, 32);
            t[33] = 0;
            strcat(t, " ?");
            PopUpAffirm(0, GRloc(), buf, yncb, ac);
        }
        else {
            bookmark_t *b = ac->bookmark();
            newtopic(b->url, false, false, true);
        }
    }
}


void
QThelpPopup::help_slot()
{
    if (GRX->main_frame())
        GRX->main_frame()->PopUpHelp("helpsys");
    else
        PopUpHelp("helpsys");
}


// Handle selections from the cache listing.
//
void
QThelpPopup::cache_choice_slot(const char *string)
{
    lstring::advtok(&string);
    newtopic(string, false, false, true);
}


// Print the current anchor href in the status label.
//
void
QThelpPopup::anchor_track_slot(htmAnchorCallbackStruct *c)
{
    if (c && c->href)
        status_bar->showMessage(QString(c->href));
    else
        status_bar->showMessage(QString(cur_topic->keyword()));
}


// Handle clicking on an anchor.
//
void
QThelpPopup::newtopic_slot(htmAnchorCallbackStruct *c)
{
    if (c == 0 || c->href == 0)
        return;
    c->visited = true;

    // download if shift pressed
    bool force_download = false;
    QMouseEvent *qme = static_cast<QMouseEvent*>(c->event);
    if (qme && (qme->modifiers() & Qt::ShiftModifier))
        force_download = true;

    bool spawn = false;
    if (!force_download) {
        if (c->target) {
            if (!cur_topic->target() ||
                    strcmp(cur_topic->target(), c->target)) {
                topic *t = HLP()->context()->findUrlTopic(c->target);
                if (t) {
                    newtopic(c->href, false, false, false);
                    return;
                }
                // Special targets:
                //  _top    reuse same window, no frames
                //  _self   put in originating frame
                //  _blank  put in new window
                //  _parent put in parent frame (nested framesets)

                if (!strcmp(c->target, "_top")) {
                    newtopic(c->href, false, false, false);
                    return;
                }
                // note: _parent not handled, use new window
                if (strcmp(c->target, "_self"))
                    spawn = true;
            }
        }
    }

    if (!spawn) {
        // spawn a new window if button 2 pressed
        if (qme && qme->button() == Qt::MidButton)
            spawn = true;
    }

    newtopic(c->href, spawn, force_download, false);

    if (c->target && spawn) {
        topic *t = HLP()->context()->findKeywordTopic(c->href);
        if (t)
            t->set_target(c->target);
    }
}


// Handle the "submit" request for an html form.  The form return is
// always downloaded and never taken from the cache, since this
// prevents multiple submissions of the same form.
//
void
QThelpPopup::form_slot(htmFormCallbackStruct *cbs)
{
    HLP()->context()->formProcess(cbs, this);
}


// Handle frames.
//
void
QThelpPopup::frame_slot(htmFrameCallbackStruct *cbs)
{
    if (cbs->reason == HTM_FRAMECREATE) {
        html_viewer->hide_drawing_area(true);
        frame_array_size = cbs->nframes;
        frame_array = new QThelpPopup*[frame_array_size];
        for (int i = 0; i < frame_array_size; i++) {
            frame_array[i] = new QThelpPopup(false, this);
            // use parent's defaults
            frame_array[i]->params = params;
            frame_array[i]->set_frame_parent(this);
            frame_array[i]->set_frame_name(cbs->frames[i].name);

            frame_array[i]->setGeometry(cbs->frames[i].x, cbs->frames[i].y,
                cbs->frames[i].width, cbs->frames[i].height);

            if (cbs->frames[i].scroll_type == FRAME_SCROLL_NONE) {
                frame_array[i]->html_viewer->setVerticalScrollBarPolicy(
                    Qt::ScrollBarAlwaysOff);
                frame_array[i]->html_viewer->setHorizontalScrollBarPolicy(
                    Qt::ScrollBarAlwaysOff);
            }
            else if (cbs->frames[i].scroll_type == FRAME_SCROLL_AUTO) {
                frame_array[i]->html_viewer->setVerticalScrollBarPolicy(
                    Qt::ScrollBarAsNeeded);
                frame_array[i]->html_viewer->setHorizontalScrollBarPolicy(
                    Qt::ScrollBarAsNeeded);
            }
            else if (cbs->frames[i].scroll_type == FRAME_SCROLL_YES) {
                frame_array[i]->html_viewer->setVerticalScrollBarPolicy(
                    Qt::ScrollBarAlwaysOn);
                frame_array[i]->html_viewer->setHorizontalScrollBarPolicy(
                    Qt::ScrollBarAlwaysOn);
            }

            frame_array[i]->show();

            topic *newtop;
            char hanchor[128];
            HLP()->context()->resolveKeyword(cbs->frames[i].src, &newtop,
                hanchor, this, 0, false, false);
            if (!newtop) {
                char buf[256];
                sprintf(buf, "Unresolved link: %s.", cbs->frames[i].src);
                PopUpErr(MODE_ON, buf);
            }

            frame_array[i]->shell = shell;
            newtop->set_target(cbs->frames[i].name);
            newtop->set_context(frame_array[i]);
            frame_array[i]->root_topic = newtop;
            frame_array[i]->cur_topic = newtop;

            if (!newtop->is_html() &&
                    HLP()->context()->isPlain(newtop->keyword())) {
                newtop->set_show_plain(true);
                frame_array[i]->html_viewer->set_mime_type("text/plain");
            }
            else {
                newtop->set_show_plain(false);
                frame_array[i]->html_viewer->set_mime_type("text/html");
            }
            frame_array[i]->html_viewer->set_source(newtop->get_text());
        }
    }
    else if (cbs->reason == HTM_FRAMERESIZE) {
        for (int i = 0; i < frame_array_size; i++) {
            frame_array[i]->setGeometry(cbs->frames[i].x, cbs->frames[i].y,
                cbs->frames[i].width, cbs->frames[i].height);
        }
    }
    else if (cbs->reason == HTM_FRAMEDESTROY) {
        for (int i = 0; i < frame_array_size; i++)
            delete frame_array[i];
        delete [] frame_array;
        frame_array = 0;
        frame_array_size = 0;
        html_viewer->show();
    }
}


// Callback for the "Open" and "Open File" menu commands, opens a new
// keyword or file
//
void
QThelpPopup::do_open_slot(const char *name, void*)
{
    if (name) {
        while (isspace(*name))
            name++;
        if (*name) {
            char *url = 0;
            char *t = strrchr(name, '.');
            if (t) {
                t++;
                if (lstring::cieq(t, "html") || lstring::cieq(t, "htm") ||
                        lstring::cieq(t, "jpg") || lstring::cieq(t, "gif") ||
                        lstring::cieq(t, "png")) {
                    if (!lstring::is_rooted(name)) {
                        char *cwd = getcwd(0, 256);
                        if (cwd) {
                            url = new char[strlen(cwd) + strlen(name) + 2];
                            sprintf(url, "%s/%s", cwd, name);
                            delete [] cwd;
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
            newtopic(url, false, false, true);
            if (input)
                input->popdown();
            delete [] url;
        }
    }
}


// Callback passed to PopUpInput to actually save the text in a file.
//
void
QThelpPopup::do_save_slot(const char *fnamein, void*)
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
        sprintf(tbuf, "Error: can't open file %s", fname);
        PopUpMessage(tbuf, true);
        delete [] fname;
        return;
    }
    char *tptr = html_viewer->get_plain_text();
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
    if (input)
        input->popdown();
    PopUpMessage(mesg, false);
    delete [] fname;
}


// Callback passed to PopUpInput to actually perform a database keyword
// search.
//
void
QThelpPopup::do_search_slot(const char *target, void*)
{
    if (target && *target) {
        topic *newtop = HLP()->search(target);
        if (!newtop)
            PopUpErr(MODE_ON, "Unresolved link.");
        else
            newtop->link_new_and_show(false, cur_topic);
    }
    if (input)
        input->popdown();
}


void
QThelpPopup::do_find_text_slot(const char *target, void*)
{
    if (target && *target)
        html_viewer->find_words(target);
}


//-----------------------------------------------------------------------------
// Private functions

// Function to display a new topic, or respond to a link
//
void
QThelpPopup::newtopic(const char *href, bool spawn, bool force_download,
    bool nonrelative)
{
    topic *newtop;
    char hanchor[128];
    if (HLP()->context()->resolveKeyword(href, &newtop, hanchor, this,
            cur_topic, force_download, nonrelative))
        return;
    if (!newtop) {
        char buf[256];
        sprintf(buf, "Unresolved link: %s.", href);
        PopUpErr(MODE_ON, buf);
        return;
    }
    newtop->set_context(this);
    newtop->link_new_and_show(spawn, cur_topic);
}


// Halt all image transfers in progress for the widget, and clear the
// queue of jobs for this window
//
void
QThelpPopup::stop_image_download()
{
    if (params->LoadMode == HlpParams::LoadProgressive)
        html_viewer->progressive_kill();
    HLP()->context()->abortImageDownload(this);
}

