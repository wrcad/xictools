
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

#include "qtmain.h"
#include "qtmenu.h"
#include "qtcoord.h"
#include "qtparam.h"
#include "qtexpand.h"
#include "qtltab.h"
#include "qthtext.h"
#include "qtinterf/qtfile.h"
#include "qtinterf/qtmsg.h"
#include "qtinlines.h"
#include "extif.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "ginterf/nulldev.h"
#include "promptline.h"
#include "tech.h"
#include "events.h"
#include "select.h"
#include "keymap.h"
#include "keymacro.h"
#include "errorlog.h"
#include "ghost.h"
#include "miscutil/timer.h"
#include "miscutil/pathlist.h"
#include "help/help_context.h"
#include "qtinterf/idle_proc.h"

#include <QApplication>
#include <QAction>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>

#include "file_menu.h"
#include "view_menu.h"

// XXX
bool load_qtmain;

//-----------------------------------------------------------------------------
// Main Window
//
// Help system keywords used:
//  button4
//  button2
//  button3
//  keyspresd
//  xic:wrbtn

// Create and export the graphics package.
namespace { QTpkg _qt_; }


class NULLwinApp : virtual public cAppWinFuncs
{
public:
    // pixmap manipulations
    void SwitchToPixmap()                                           { }
    void SwitchFromPixmap(const BBox*)                              { }
    GRobject DrawableReset()                                { return (0); }
    void CopyPixmap(const BBox*)                                    { }
    void DestroyPixmap()                                            { }
    bool DumpWindow(const char*, const BBox*)               { return (false); }
    bool PixmapOk()                                         { return (false); }

    // key handling/display interface
    void GetTextBuf(char*)                                          { }
    void SetTextBuf(const char*)                                    { }
    void ShowKeys()                                                 { }
    void SetKeys(const char*)                                       { }
    void BspKeys()                                                  { }
    void CheckExec(bool)                                            { }
    char *KeyBuf()                                          { return (0); }
    int KeyPos()                                            { return (0); }

    // label
    void SetLabelText(const char*)                                  { }

    // misc. pop-ups
    void PopUpGrid(GRobject, ShowMode)                              { }
    void PopUpExpand(GRobject, ShowMode,
        bool(*)(const char*, void*), void*, const char*, bool)      { }
    void PopUpZoom(GRobject, ShowMode)                              { }
};

// Main and subwindow class for null graphics.
//
struct null_bag : public DSPwbag, public NULLwbag,  NULLwinApp,
    public NULLdraw
{
};


/* XXX
// Command logic for GTKpkg::Point
//
struct PointState : public CmdState
{
    PointState();
    virtual ~PointState();
    static bool got_btn1;
    static bool got_abort;
private:
    void b1down() { got_btn1 = true; }
    void b1up() { if (got_btn1) esc(); }
    void esc()
        {
            got_abort = Abort;
            if (GRX->LoopLevel() > 1)
                GRX->BreakLoop();
            EV()->PopCallback(this);
            delete this;
        }
};

bool PointState::got_btn1;
bool PointState::got_abort;

namespace { PointState *PointCmd; }


PointState::PointState()
{
    StateName = "POINT";
    got_btn1 = false;
}


PointState::~PointState()
{
    PointCmd = 0;
}
// End of PointState functions.
*/



//-----------------------------------------------------------------------------
// QTpkg functions

// Create a new main_bag.  This will be passed to QTdev::New() for
// registration, then on to Initialize().
//
GRwbag *
QTpkg::NewGX()
{
    if (!MainDev())
        return (0);
    if (MainDev()->ident == _devNULL_)
        return (new null_bag);
    if (MainDev()->ident == _devQT_)
        return (new mainwin);
    return (0);
}


static void
messageOutput(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtDebugMsg:
    case QtWarningMsg:
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", msg);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", msg);
        break;
    }
}


// Initialization function for the main window.  The argument *must* be
// a main_bag.
//
int
QTpkg::Initialize(GRwbag *wcp)
{
    if (!MainDev())
        return (true);
//XXXqInstallMsgHandler(messageOutput);
    if (MainDev()->ident == _devNULL_) {
        static QTltab qt_lt(true, 0);
        static QTedit qt_hy(true, 0);
        LT()->SetLtab(&qt_lt);
        PL()->SetEdit(&qt_hy);

        null_bag *w = dynamic_cast<null_bag*>(wcp);
        DSP()->SetWindow(0, new WindowDesc);
        EV()->SetCurrentWin(DSP()->MainWdesc());
        DSP()->MainWdesc()->SetWbag(w);
        DSP()->MainWdesc()->SetWdraw(w);

        DSP()->SetCurMode(XM()->InitialMode());

        DSP()->ColorTab()->init();
        XM()->AppInit();
        XM()->InitSignals(true);
        XM()->Rehash();

//XXX        Kmap()->Assert();
        return (false);
    }
    if (MainDev()->ident != _devQT_)
        return (true);

    idle_control = new idle_proc();

    mainwin *w = dynamic_cast<mainwin*>(wcp);
    if (!w)
        return (true);

    GRX->RegisterMainFrame(w);
    GRpkgIf()->RegisterMainWbag(w);
    w->initialize();
    w->show();

    QSize qs = w->Viewport()->size();
    DSP()->Initialize(qs.width(), qs.height(), 0,
        (XM()->RunMode() != ModeNormal));

    LT()->InitLayerTable();

    // callback to tell application when quitting help
//    HLP()->context()->registerQuitHelpProc(quit_help);
//    HLP()->context()->registerFormSubmitProc(form_submit_hdlr);

//    PL()->Edit()->hyInit();
    qtEdit()->init();
//    GRX->RegisterBigWindow(gr_x_window(w->shell->window));

    DSP()->ColorTab()->alloc();
    XM()->InitSignals(true);
    XM()->Rehash();

//XXX    Kmap()->Assert();
    Gst()->SetGhost(GFnone);
    if (!mainBag())
        // Halt called
        return (true);
    XM()->SetAppReady(true);  // XXX in expose handler?
    return (false);
}


// Reinitialize the application to shut down all graphics.
//
void
QTpkg::ReinitNoGraphics()
{
}


// Halt application cleanup function.
//
void
QTpkg::Halt()
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
        return;
//    GRX->RegisterBigWindow(0);
    EV()->InitCallback();
//    gtk_main_quit();
    if (DSP()->MainWdesc()) {
        DSP()->MainWdesc()->SetWbag(0);
        DSP()->MainWdesc()->SetWdraw(0);
    }
    XM()->SetAppReady(false);
}


// The main loop, does not return, but might be reentered with longjmp.
//
void
QTpkg::AppLoop()
{
//    RegisterEventHandler(0, 0);
//    in_main_loop = true;
    GRX->MainLoop(true);
}


// Set the interrupt flag and return true if a ^C event is found.
//
bool
QTpkg::CheckForInterrupt()
{
    static unsigned long lasttime;
//    if (!in_main_loop)
//        return (false);
    if (DSP()->Interrupt())
        return (true);

    // Cut expense if called too frequently.
    if (Timer()->elapsed_msec() == lasttime)
        return (false);
    lasttime = Timer()->elapsed_msec();

//    while (gtk_events_pending())
//        gtk_main_iteration();
    if (DSP()->Interrupt())
        return (true);
    return (false);
}


// If state is true, iconify the application.  Return false if the
// application has not been realized.  Note that the map_hdlr()
// function takes care of the popups.
//
int
QTpkg::Iconify(int)
{
    return (0);
}


// Initialize popup subwindow number wnum (1 - DSP_NUMWINS-1).
//
bool
QTpkg::SubwinInit(int wnum)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return (false);
    if (!mainBag())
        return (false);
    subwin_d *w = new subwin_d(wnum, mainBag());

    char buf[32];
    sprintf(buf, "%s %d", XM()->Product(), wnum+1);
    w->setWindowTitle(buf);

    DSP()->Window(wnum)->SetWbag(w);
    DSP()->Window(wnum)->SetWdraw(w);

    // Create new menu, just copy template.
    Menu()->CreateSubwinMenu(wnum);

    w->connect(w, SIGNAL(update_coords(int, int)),
        mainBag(), SLOT(update_coords_slot(int, int)));
/*
    int xpos, ypos;
    if (LastPos[wnum].width) {
        gtk_widget_set_uposition(w->shell, LastPos[wnum].x, LastPos[wnum].y);
        gtk_window_set_default_size(GTK_WINDOW(w->shell), LastPos[wnum].width,
            LastPos[wnum].height);
        xpos = LastPos[wnum].x;
        ypos = LastPos[wnum].y;
    }
    else {
        GdkRectangle rect;
        ShellGeometry(mainBag()->shell, &rect, 0);
        rect.x += rect.width - 560;
        rect.y += wnum*40 + 60; // make room for device toolbar
        gtk_widget_set_uposition(w->shell, rect.x, rect.y);
        xpos = rect.x;
        ypos = rect.y;
    }


    // Set up double-click cancel on label event box.
    //
    for (MenuEnt *mx = GTKmenuPtr->Msubwins[wnum]->menu; mx->entry; mx++) {
        if (!strcmp(mx->entry, MenuCANCL)) {
            GRX->SetDoubleClickExit(ebox, (GtkWidget*)mx->cmd.caller);
            break;
        }
    }
*/

    w->SetWindowBackground(w->xbag->backg);
    w->Clear();

    // Application initialization callback.
    //
    QSize qs = w->Viewport()->size();
//XXX draws twice, first here than later from resize
    DSP()->Initialize(qs.width(), qs.height(), wnum);
    w->show();
    w->raise();
    w->activateWindow();
    return (true);
}


void
QTpkg::SubwinDestroy(int wnum)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return;
    WindowDesc *wdesc = DSP()->Window(wnum);
    if (!wdesc)
        return;
    subwin_d *w = dynamic_cast<subwin_d*>(wdesc->Wbag());
    if (w) {
        wdesc->SetWbag(0);
        wdesc->SetWdraw(0);

        Menu()->DestroySubwinMenu(wnum);

    /*
        GdkRectangle rect, rect_d;
        ShellGeometry(w->shell, &rect, &rect_d);
        LastPos[wnum].x = rect_d.x;
        LastPos[wnum].y = rect_d.y;
        LastPos[wnum].width = rect.width;
        LastPos[wnum].height = rect.height;

        gtk_signal_disconnect_by_func(GTK_OBJECT(w->shell),
            GTK_SIGNAL_FUNC(subwin_cancel_proc), (void*)(long)(wnum+1));
    */

        delete w;
    }
}


static void
wait_cursor(bool waiting)
{
    if (waiting)
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    else
        QApplication::restoreOverrideCursor();
}


// Increment/decrement the busy flag and switch to watch cursor.  If
// already busy when passing true, return false.
//
bool
QTpkg::SetWorking(bool busy)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return (false);
    bool ready = true;
    if (busy) {
        if (!app_busy) {
            DSP()->SetInterrupt(DSPinterNone);
            if (!app_override_busy)
                wait_cursor(true);
        }
        else
            ready = false;
        app_busy++;
    }
    else {
        if (app_busy == 1)
            wait_cursor(false);
        if (app_busy)
            app_busy--;
    }
    return (ready);
}


void
QTpkg::SetOverrideBusy(bool ovr)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return;
    if (ovr) {
        if (app_busy)
            wait_cursor(false);
        app_override_busy = true;
    }
    else {
        if (app_busy)
            wait_cursor(true);
        app_override_busy = false;
    }
}


// Print into buf an identifier string for the main window, the
// X window number in this case.
//
bool
QTpkg::GetMainWinIdentifier(char *buf)
{
    (void)buf;
    return (false);
}


bool
QTpkg::IsDualPlane()
{
    return (false);
}


bool
QTpkg::IsTrueColor()
{
    return (true);  //XXX
}


bool
QTpkg::UsingX11()
{
#ifdef WITH_X11
    return (true);
#else
    return (false);
#endif
}


void
QTpkg::CloseGraphicsConnection()
{
}


// Write in buf the display string, making sure that it contains the
// host name.
//
const char*
QTpkg::GetDisplayString()
{
    return (0);
}


// Check if host which has data in hent has screen access to the local
// display in display_string.  Simply print a warning if access fails.
//
bool
QTpkg::CheckScreenAccess(hostent*, const char*, const char*)
{
    return (false);
}


int
QTpkg::RegisterIdleProc(int(*cb)(void*), void *arg)
{
    return (idle_control->add(cb, arg));
}


bool
QTpkg::RemoveIdleProc(int id)
{
    return (idle_control->remove(id));
}


int
QTpkg::RegisterTimeoutProc(int ms, int(*proc)(void*), void *arg)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
         return (0);
    return (GRX->AddTimer(ms, proc, arg));
}


bool
QTpkg::RemoveTimeoutProc(int id)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
         return (false);
    GRX->RemoveTimer(id);
    return (true);
}


// int time;  milliseconds
//
int
QTpkg::StartTimer(int, bool*)
{
    return (0);
}


// Set the application fonts.  This should work for either Pango
// or XFD font names.
//
void
QTpkg::SetFont(const char*, int, FNT_FMT)
{
}


// Return the application font names.
//
const char *
QTpkg::GetFont(int)
{
    return (0);
}


// Return the format code for the description string returned by GetFont.
//
FNT_FMT
QTpkg::GetFontFmt()
{
    return (FNT_FMT_Q);
}


// This function pushes into a new call state and waits for a button 1
// press (returns true) or an escape event (returns false).  This is used
// in scripts as a wait loop for placing objects
//
/*XXX
PTretType
QTpkg::PointTo()
{
    return (PTok);
}
*/
// End of QTpkg functions


//-----------------------------------------------------------------------------
// keys_w functions

keys_w::keys_w(int wnum, QWidget *prnt) : draw_qt_w(false, prnt)
{
    keypos = 0;
    memset(keys, 0, CBUFMAX+1);
    win_number = wnum;

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_SCREEN))
        set_font(fnt);
}


void
keys_w::show_keys()
{
    char *s = keys + (keypos > 5 ? keypos - 5 : 0);
    clear();
    int yy = line_height();
    draw_text(2, yy, s, -1);
    update();
}


void
keys_w::set_keys(const char *text)
{
    while (keypos)
        keys[--keypos] = '\0';
    if (text) {
        strncpy(keys, text, CBUFMAX);
        keypos = strlen(keys);
    }
}


void
keys_w::bsp_keys()
{
    if (keypos)
        keys[--keypos] = '\0';
}


void
keys_w::check_exec(bool exact)
{
    if (!keypos)
        return;
    MenuEnt *ent = Menu()->MatchEntry(keys, keypos, win_number, exact);
    if (ent) {
        if (ent->is_dynamic() && ent->is_menu())
            // Ignore the submenu buttons in the User menu
            return;
        if (!strcmp(ent->entry, MenuVIEW)) {
            // do something reasonable for the "view" command
            if (ent->cmd.wdesc)
                ent->cmd.wdesc->SetView("full");
            else
                DSP()->MainWdesc()->SetView("full");
            clear();
            int yy = line_height();
            draw_text(2, yy, MenuVIEW, -1);
            update();
        }
        else if (ent->cmd.caller) {
            // simulate a button press
            char buf[CBUFMAX + 1];
            strncpy(buf, ent->entry, CBUFMAX);
            buf[CBUFMAX] = 0;
            int n = strlen(buf) - 5;
            if (n < 0)
                n = 0;
            clear();
            int yy = line_height();
            draw_text(2, yy, buf + n, -1);
            update();
            if (ent->cmd.caller)
                Menu()->CallCallback(ent->cmd.caller);
        }
        set_keys(0);
    }
}


//-----------------------------------------------------------------------------
// subwin_d functions

DrawType subwin_d::drawtype = DrawX;

subwin_d::subwin_d(int wnum, QWidget *prnt) : QDialog(prnt), qt_bag(this)
{
    if (wnum < 0)
        win_number = -1;
    else
        win_number = wnum;
    expand = 0;

    menubar = new QMenuBar(this);

    if (win_number == 0) {
        const char *str = getenv("GR_SYSTEM");
        if (str) {
            if (!strcasecmp(str, "X"))
                drawtype = DrawX;
            if (!strcasecmp(str, "GL"))
                drawtype = DrawGL;
            if (!strcasecmp(str, "QT") || !strcasecmp(str, "native"))
                drawtype = DrawNative;
        }
        if (drawtype == DrawGL)
            printf("Using OpenGL graphics system.\n");
        else if (drawtype == DrawX)
            printf("Using custom X-Windows graphics system.\n");
        else
            printf("Using QT native graphics system.\n");
    }
    viewport = draw_if::new_draw_interface(drawtype, true, this);
    Viewport()->setFocusPolicy(Qt::StrongFocus);

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_SCREEN))
        viewport->set_font(fnt);

    keys_pressed = new keys_w(win_number, this);

    connect(Viewport(), SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(button_down_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(button_up_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(key_press_event(QKeyEvent*)),
        this, SLOT(key_down_slot(QKeyEvent*)));
    connect(Viewport(), SIGNAL(key_release_event(QKeyEvent*)),
        this, SLOT(key_up_slot(QKeyEvent*)));
    connect(Viewport(), SIGNAL(move_event(QMouseEvent*)),
        this, SLOT(motion_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(enter_event(QEvent*)),
        this, SLOT(enter_slot(QEvent*)));
    connect(Viewport(), SIGNAL(leave_event(QEvent*)),
        this, SLOT(leave_slot(QEvent*)));
    connect(Viewport(), SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));

    if (win_number == 0)
        // being subclassed for main window
        return;

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    keys_pressed->setFixedHeight(line_height() + 4);
    keys_pressed->setFixedWidth(6*char_width());
    keys_pressed->move(160, 2);

    vbox->setMenuBar(menubar);
    vbox->addWidget(Viewport());
}


subwin_d::~subwin_d()
{
    PopUpGrid(0, MODE_OFF);
}


// cAppWinFuncs interface

// The following two functions implement pixmap buffering for screen
// redisplays.  This often leads to faster redraws.  This function
// will create a pixmap and make it the current context drawable.
//
void
subwin_d::SwitchToPixmap()
{
}


// Copy out the BB area of the pixmap to the screen, and switch the
// context drawable back to the screen.
//
void
subwin_d::SwitchFromPixmap(const BBox *BB)
{
    CopyPixmap(BB);
}


// Return the pixmap, set by a previous call to SwitchToPixmap(), and
// reset the context to normal
//
GRobject
subwin_d::DrawableReset()
{
    return (0);
}


// Copy out the BB area of the pixmap to the screen
//
void
subwin_d::CopyPixmap(const BBox *BB)
{
    Viewport()->repaint(BB->left, BB->top, BB->right - BB->left + 1,
        BB->bottom - BB->top + 1);
    /*
        */
//Update();
}


// Destroy the pixmap (switching out first if necessary).
//
void
subwin_d::DestroyPixmap()
{
}


// Function to dump a window to a file, not part of the hardcopy system
//
bool
subwin_d::DumpWindow(const char *filename, const BBox *AOI = 0)
{
/*
    if (Global.RunMode != ModeNormal)
        return (false);
    if (!AOI)
        AOI = &w_viewport;
    BBox BB = *AOI;
    ViewportClip(BB, w_viewport);
    if (BB.right < BB.left || BB.bottom < BB.top)
        return (false);
    GdkPixmap *pm = 0;
    bool native = false;
    if (w_pixmap && w_pxwidth == w_viewport.right - w_viewport.left &&
            w_pxheight == w_viewport.bottom - w_viewport.top) {
        pm = (GdkPixmap*)w_pixmap;
        native = true;
    }
    else {
        widget_bag *wb =
            (this == DSP()->windows[0] ? GTKmainWin : (widget_bag*)w_draw);
        pm = gdk_pixmap_new(wb->window, w_viewport.right - w_viewport.left + 1,
            w_viewport.bottom - w_viewport.top + 1, GTKdev::visual->depth);
        if (!pm)
            return (false);
        GdkWindow *tmpw = wb->window;
        wb->window = pm;
        RedisplayDirect();
        wb->window = tmpw;
    }

    Image *im = create_image_from_drawable(gr_x_display(),
        GDK_WINDOW_XWINDOW(pm),
        BB.left - w_viewport.left, BB.top - w_viewport.top,
        BB.right - BB.left, BB.bottom - BB.top);
    if (!native)
        gdk_pixmap_unref(pm);
    if (!im) 
        return (false);
    ImErrType ret = im->save_image(filename, 0);
    if (ret == ImError)
        XM()->PopUpErr("Image creation failed, internal error.");
    else if (ret == ImNoSupport)
        XM()->PopUpErr("Image creation failed, file type unsupported.");
    delete im;
    return (true);
*/
(void)filename;
(void)AOI;
return (false);
}


bool
subwin_d::PixmapOk()
{
    return (true);
}


// Get the "keyspressed" in buf
//
void
subwin_d::GetTextBuf(char *buf)
{
    for (int i = 0; i < keys_pressed->key_pos(); i++)
        buf[i] = keys_pressed->key(i);
    buf[keys_pressed->key_pos()] = 0;
}


// Set the "keyspressed"
//
void
subwin_d::SetTextBuf(const char *buf)
{
    keys_pressed->set_keys(buf);
    keys_pressed->show_keys();
}


void
subwin_d::ShowKeys()
{
    keys_pressed->show_keys();
}


void
subwin_d::SetKeys(const char *text)
{
    keys_pressed->set_keys(text);
}


void
subwin_d::BspKeys()
{
    keys_pressed->bsp_keys();
}


void
subwin_d::CheckExec(bool exact)
{
    keys_pressed->check_exec(exact);
}


char *
subwin_d::KeyBuf()
{
    return (keys_pressed->key_buf());
}


int
subwin_d::KeyPos()
{
    return (keys_pressed->key_pos());
}


void
subwin_d::SetLabelText(const char*)
{
}


void
subwin_d::PopUpGrid(GRobject, ShowMode)
{
}


void
subwin_d::PopUpExpand(GRobject caller, ShowMode mode,
    bool (*callback)(const char*, void*),
    void *arg, const char *string, bool nopeek)
{
    if (mode == MODE_OFF) {
        if (expand)
            expand->popdown();
        return;
    }
    if (mode == MODE_UPD) {
        if (expand)
            expand->update(string);
        return;
    }
    if (expand)
        return;

    expand = new expand_d(this, string, nopeek, arg);
    expand->register_caller(caller);
    expand->register_callback(callback);
    expand->set_visible(true);
}


void
subwin_d::PopUpZoom(GRobject caller, ShowMode mode)
{
/*
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        if (wib_zoompop)
            wib_zoompop->popdown();
        return;
    }
    if (mode == MODE_UPD) {
        if (wib_zoompop)
            wib_zoompop->update();
        return;
    }
    if (wib_zoompop)
        return;

    wib_zoompop = new sZm(this, wib_windesc);
    wib_zoompop->register_usrptr((void**)&wib_zoompop);
    if (!wib_zoompop->shell()) {
        delete wib_zoompop;
        wib_zoompop = 0;
        return;
    }

    wib_zoompop->register_caller(caller);
    wib_zoompop->initialize(x, y);
    wib_zoompop->set_visible(true);
*/
}
// End of cAppWinFuncs interface


#define MODMASK (GR_SHIFT_MASK | GR_CONTROL_MASK | GR_ALT_MASK)

// Handle keypresses in the main and subwindows.  If certain modes
// are set, call the application.  Otherwise, save the keypress in
// a buffer, which is displayed in the keypress readout area.  If
// this text uniquely matches a command, send a button press to
// the appropriate command button, and clear the buffer.
//
bool
subwin_d::keypress_handler(unsigned int keyval, unsigned int state,
    const char *keystring, bool up)
{
    WindowDesc *wdesc = DSP()->Window(win_number);
    if (!wdesc)
        return (false);

    // The code: 0x00 - 0x16 are the KEYcode enum values.
    //           0x17 - 0x1f unused
    //           0x20 - 0xffff X keysym
    // The X keysyms 0x20 - 0x7e match the ascii character
    int code = 0;
    if (up) {
        for (keymap *k = Kmap()->KeymapUp(); k->keyval; k++) {
            if (k->keyval == keyval) {
                code = k->code;
                break;
            }
        }
        if (code == SHIFTUP_KEY || code == CTRLUP_KEY)
            EV()->KeypressCallback(wdesc, code, (char*)"", state);
        return (true);
    }

    int subcode = 0;
    for (keymap *k = Kmap()->KeymapDown(); k->keyval; k++) {
        if (k->keyval == keyval) {
            code = k->code;
            subcode = k->subcode;
            break;
        }
    }

    if (code == FUNC_KEY) {
        char tbuf[CBUFMAX + 1];
        memset(tbuf, 0, sizeof(tbuf));
        char *fstr = Tech()->GetFkey(subcode, 0);
        if (fstr)
            strncpy(tbuf, fstr, CBUFMAX);
        if (EV()->KeypressCallback(wdesc, code, tbuf, state))
            return (true);
        keys_pressed->set_keys(tbuf);
        keys_pressed->show_keys();
        keys_pressed->check_exec(true);
        return (true);
    }
    if (code == 0 && keyval >= 0x20)
        code = keyval;

    for (keyaction *k = Kmap()->ActionMapPre(); k->code; k++) {
        if (k->code == code &&
                (!k->state || k->state == (state & MODMASK))) {
            if (EV()->KeyActions(wdesc, k->action, &code))
                return (true);
            break;
        }
    }

    char tbuf[CBUFMAX + 1];
    memset(tbuf, 0, sizeof(tbuf));
    if (keystring)
        strncpy(tbuf, keystring, CBUFMAX);
    if (EV()->KeypressCallback(wdesc, code, tbuf, state))
        return (true);

    if (state & GR_ALT_MASK)
        return (false);

    for (keyaction *k = Kmap()->ActionMapPost(); k->code; k++) {
       if (k->code == code &&
                (!k->state || k->state == (state & MODMASK))) {
            if (EV()->KeyActions(wdesc, k->action, &code))
                return (true);
            break;
        }
    }

    bool exact = false;
    if (*tbuf > ' ' && *tbuf <= '~' && keys_pressed->key_pos() < CBUFMAX)
        keys_pressed->append(*tbuf);
    else if (code == RETURN_KEY && keys_pressed->key_pos())
        exact = true;
    else
        return (false);
    keys_pressed->show_keys();
    keys_pressed->check_exec(exact);
    return (true);
}


// Dispatch button press events to the application callbacks.
//
void
subwin_d::button_down_slot(QMouseEvent *ev)
{
    WindowDesc *wdesc = DSP()->Window(win_number);
    if (!wdesc)
        return;

    int button = 0;
    if (ev->button() == Qt::LeftButton)
        button = 1;
    else if (ev->button() == Qt::MidButton)
        button = 2;
    else if (ev->button() == Qt::RightButton)
        button = 3;

    button = Kmap()->ButtonMap(button);
/*
    if (!GTK_WIDGET_SENSITIVE(GTKmenuPtr->mainMenu) && button == 1)
        // menu is insensitive, so ignore press
        return (true);
    if (event->type == GDK_2BUTTON_PRESS ||
            event->type == GDK_3BUTTON_PRESS)
        return (true);
    xic_bag *w = static_cast<xic_bag*>(client_data);
*/
    if (message)
        message->popdown();

    bool showing_ghost = xbag->showghost;
    if (showing_ghost)
        ShowGhost(ERASE);
/*
    if (!XM()->IsDoingHelp() || XM()->IsCallback())
        grabstate.set(caller, bev);
*/

        // Override the "automatic grab" so that the cursor tracks
        // the subwindow crossings.
        //
        // Subtle point:  the callbacks can call their own event
        // loops, so we have to set the grab state before the
        // callbacks are called.

    int state = ev->modifiers();

    switch (button) {
    case 1:
        // basic point operation
        if ((state & Qt::ShiftModifier) &&
            (state & Qt::ControlModifier) &&
            (state & Qt::AltModifier)) {
            // Control-Shift-Alt-Button1 simulates Button4
//            grabstate.check_simb4(true);
            if (XM()->IsDoingHelp())
                PopUpHelp("button4");
            else
                EV()->ButtonNopCallback(wdesc, ev->x(), ev->y(),
                    mod_state(state));
        }
        else if (wdesc)
            EV()->Button1Callback(wdesc, ev->x(), ev->y(), mod_state(state));
        break;
    case 2:
        // Pan operation
        if (XM()->IsDoingHelp() && !(state & Qt::ShiftModifier))
            PopUpHelp("button2");
        else
            EV()->Button2Callback(wdesc, ev->x(), ev->y(), mod_state(state));
        break;
    case 3:
        // Zoom opertion
        if (XM()->IsDoingHelp() && !(state & Qt::ShiftModifier))
            PopUpHelp("button3");
        else
            EV()->Button3Callback(wdesc, ev->x(), ev->y(), mod_state(state));
        break;
    default:
        // No-op, update coordinates
        if (XM()->IsDoingHelp() && !(state & Qt::ShiftModifier))
            PopUpHelp("button4");
        else
            EV()->ButtonNopCallback(wdesc, ev->x(), ev->y(), mod_state(state));
        break;
    }
    if (showing_ghost)
        ShowGhost(DISPLAY);
}


// Dispatch button release events to the application callbacks.
//
void
subwin_d::button_up_slot(QMouseEvent *ev)
{
    WindowDesc *wdesc = DSP()->Window(win_number);
    if (!wdesc)
        return;

    int button = 0;
    if (ev->button() == Qt::LeftButton)
        button = 1;
    else if (ev->button() == Qt::MidButton)
        button = 2;
    else if (ev->button() == Qt::RightButton)
        button = 3;

    button = Kmap()->ButtonMap(button);
/*
    if (!GTK_WIDGET_SENSITIVE(GTKmenuPtr->mainMenu) && button == 1)
        // menu is insensitive, so ignore release
        return (true);
    xic_bag *w = static_cast<xic_bag*>(client_data);

    if (!grabstate.is_armed(bev)) {
        grabstate.check_simb4(false);
        return (true);
    }
    grabstate.clear(bev->time);

    // this finishes processing the button up event at the server,
    // otherwise motions are frozen until this function returns
    while (gtk_events_pending())
        gtk_main_iteration();
*/

    // The point can be outside of the viewport, due to the grab.
    // Check for this.
    const BBox *BB = &wdesc->Viewport();
    bool in = (ev->x() >= 0 && ev->x() < BB->right &&
        ev->y() >= 0 && ev->y() < BB->bottom);

    bool showing_ghost = xbag->showghost;
    if (showing_ghost)
        ShowGhost(ERASE);

    int state = ev->modifiers();

    switch (button) {
    case 1:
//        if (grabstate.check_simb4(false)) {
        if (0) {
            EV()->ButtonNopReleaseCallback((in ? wdesc : 0),
                ev->x(), ev->y(), mod_state(state));
        }
        else {
            EV()->Button1ReleaseCallback((in ? wdesc : 0),
                ev->x(), ev->y(), mod_state(state));
        }
        break;
    case 2:
        EV()->Button2ReleaseCallback((in ? wdesc : 0),
            ev->x(), ev->y(), mod_state(state));
        break;
    case 3:
        EV()->Button3ReleaseCallback((in ? wdesc : 0),
            ev->x(), ev->y(), mod_state(state));
        break;
    default:
        EV()->ButtonNopReleaseCallback((in ? wdesc : 0),
            ev->x(), ev->y(), mod_state(state));
        break;
    }
    if (showing_ghost)
        ShowGhost(DISPLAY);
}


// Key press processing for the drawing windows
//
void
subwin_d::key_down_slot(QKeyEvent *ev)
{
    /*
    if (!mainBag() || qtPkgIf()->NotMapped())
        return (true);
    */

    if (message)
        message->popdown();

    const char *string = ev->text().toLatin1().constData();
    int kpos = keys_pressed->key_pos();
    if (!is_modifier_key(ev->key()) && kpos &&
            keys_pressed->key(kpos - 1) == Kmap()->SuppressChar())
        keys_pressed->bsp_keys();
/*
    else if (Kmap()->MacroExpand(kev->keyval, kev->state, false))
        return (true);

    if (ev->key() == Qt::Key_Shift || ev->key() == Qt::Key_Control) {
        gdk_keyboard_grab(caller->window, true, kev->time);
        grabbed_key = kev->keyval;
    }
*/

    if (keypress_handler(ev->key(), mod_state(ev->modifiers()), string,
            false))
        ev->accept();
}


// Key release processing for the drawing windows
//
void
subwin_d::key_up_slot(QKeyEvent *ev)
{
/*
    if (grabbed_key && grabbed_key == event->key.keyval) {
        grabbed_key = 0;
        gdk_keyboard_ungrab(event->key.time);
    }
    if (GApp->AppNotMapped)
        return( true);
    if (Kmap()->MacroExpand(kev->keyval, kev->state, true))
        return (true);
*/
    const char *string = ev->text().toLatin1().constData();
    if (keypress_handler(ev->key(), mod_state(ev->modifiers()), string,
            true))
        ev->accept();
}


// Handle pointer motion in the main and subwindows.  In certain
// commands, "ghost" XOR drawing is utilized.
//
void
subwin_d::motion_slot(QMouseEvent *ev)
{
    WindowDesc *wdesc = DSP()->Window(win_number);
    if (wdesc) {
        EV()->MotionCallback(wdesc, mod_state(ev->modifiers()));
        WindowDesc *worig = EV()->ZoomWin();
        if (!worig)
            worig = DSP()->Windesc(EV()->Cursor().get_window());
        if (!worig)
            worig = DSP()->MainWdesc();
        if (wdesc->IsSimilar(worig)) {
            UndrawGhost();
            DrawGhost(ev->x(), ev->y());
        }
        int xx = ev->x();
        int yy = ev->y();
        wdesc->PToL(xx, yy, xx, yy);
        emit update_coords(xx, yy);
    }
}


// Reinitialize "ghost" drawing when the pointer enters a window.
// If the pointer enters a window with an active grab, check if
// a button release has occurred.  If so, and the condition has
// not already been cleared, call the appropriate callback to
// clean up.  This is for button up events that get 'lost'.
//
void
subwin_d::enter_slot(QEvent *ev)
{
    WindowDesc *wdesc = DSP()->Window(win_number);
    if (!wdesc)
        return;

    /*
    // pointer entered the drawing window
    xic_bag *w = static_cast<xic_bag*>(client_data);
    GTK_WIDGET_SET_FLAGS(caller, GTK_CAN_FOCUS);
#if GTK_CHECK_VERSION(1,3,15)
#else
    gtk_widget_draw_focus(caller);
#endif
    gtk_window_set_focus(GTK_WINDOW(w->shell), caller);
    */

    /*
    GdkEventCrossing *cev = (GdkEventCrossing*)event;
    if (grabstate.is_caller(caller)) {
        switch (grabstate.btn()) {
        case 1:
            if (!(cev->state & GDK_BUTTON1_MASK)) {
                grabstate.clear(cev->time);
                ((WindowDesc*)0)->button1_release_callback(0, 0, 0);
            }
            break;
        case 2:
            if (!(cev->state & GDK_BUTTON2_MASK)) {
                grabstate.clear(cev->time);
                ((WindowDesc*)0)->button2_release_callback(0, 0, 0);
            }
            break;
        case 3:
            if (!(cev->state & GDK_BUTTON3_MASK)) {
                grabstate.clear(cev->time);
                ((WindowDesc*)0)->button3_release_callback(0, 0, 0);
            }
            break;
        case 4:
            if (!(cev->state & GDK_BUTTON4_MASK)) {
                grabstate.clear(cev->time);
                ((WindowDesc*)0)->button4_release_callback(0, 0, 0);
            }
            break;
        }
    }
    */

//XXX    EV()->MotionCallback(wdesc, mod_state(ev->modifiers()));
    EV()->MotionCallback(wdesc, mod_state(0));
    ev->setAccepted(true);
}


// Gracefully terminate ghost drawing when the pointer leaves a
// window.
//
void
subwin_d::leave_slot(QEvent *ev)
{
    WindowDesc *wdesc = DSP()->Window(win_number);
    if (!wdesc)
        return;
//XXX    EV()->MotionCallback(wdesc, mod_state(ev->modifiers()));
    EV()->MotionCallback(wdesc, mod_state(0));
    UndrawGhost();
    xbag->firstghost = true;
    ev->setAccepted(true);
}


// Resize after the main or subwindow size change.  Call the application
// with the new size.
//
void
subwin_d::resize_slot(QResizeEvent *ev)
{
    WindowDesc *wdesc = DSP()->Window(win_number);
    if (!wdesc)
        return;
    EV()->ResizeCallback(wdesc, ev->size().width(), ev->size().height());
    wdesc->Redisplay(0);
    ev->setAccepted(true);
}

 
/*
// Redraw the main or subwindow after an expose.  Break it up into
// exposed rectangles to save time.  Call the application to
// actually redraw the viewport.
//
static int
redraw_hdlr(GtkWidget *widget, GdkEvent *event, void *client_data)
{
    if (widget->parent == DontRedrawMe) {
        DontRedrawMe = 0;
        return (false);
    }
    if (gtkHaveDrag && XM()->no_pixmap_store)
        return (false);

    xic_bag *w = static_cast<xic_bag*>(client_data);

    GdkEventExpose *pev = (GdkEventExpose*)event;
    if (!GetCurSymbol() || !XM()->app_ready) {
        w->windesc->w_clip_rect = w->windesc->w_viewport;
        w->windesc->ShowGrid();
        XM()->app_ready = true;
    }
    else {
#if GTK_CHECK_VERSION(1,3,15)
        GdkRectangle *rects;
        int nrects;
        gdk_region_get_rectangles(pev->region, &rects, &nrects);
        for (int i = 0; i < nrects; i++) {
            BBox BB(rects[i].x, rects[i].y + rects[i].height,
                rects[i].x + rects[i].width, rects[i].y);
            w->windesc->Update(&BB);
        }
        g_free(rects);
#else
        BBox BB(pev->area.x, pev->area.y + pev->area.height,
            pev->area.x + pev->area.width, pev->area.y);
        w->windesc->Update(&BB);
#endif
    }
    return (true);
}
*/


//-----------------------------------------------------------------------------

inline bool
is_shift_down()
{
    return (QApplication::keyboardModifiers() & Qt::ShiftModifier);
}


// Whiteley Research Logo bitmap
//
// XPM
static const char * const wr_xpm[] = {
    // width height ncolors chars_per_pixel
    "30 20 2 1",
    // colors
    " 	c none",
    ".	c blue",
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

menu_button::menu_button(MenuEnt *ent, QWidget *prnt) : QPushButton(prnt)
{
    setAutoDefault(false);
    if (ent->is_toggle())
        setCheckable(true);
    entry = ent;
    connect(this, SIGNAL(clicked()), this, SLOT(pressed_slot()));
}


mainwin::mainwin() : subwin_d(0, 0)
{
    QAction *a = menubar->addAction(QString("wr"));
    a->setIcon(QIcon(QPixmap(wr_xpm)));
    connect(a, SIGNAL(triggered()), this, SLOT(wr_btn_slot()));

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);
    vbox->setMenuBar(menubar);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    phys_button_box = new QWidget(this);
    hbox->addWidget(phys_button_box);
    elec_button_box = new QWidget(this);
    hbox->addWidget(elec_button_box);
    hbox->addWidget(Viewport());

    hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    int w = char_width() * 7 + 4;
    int h = line_height() + 4;
    keys_pressed->setFixedSize(w, h);
    hbox->addWidget(keys_pressed);

    promptline = draw_if::new_draw_interface(DrawNative, false, this);
    promptline->widget()->setMinimumHeight(h);
    promptline->widget()->setMaximumHeight(h);
    hbox->addWidget(promptline->widget());

    hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    coords = new coord_w(this);
    hbox->addWidget(coords);

    QVBoxLayout *vb = new QVBoxLayout(0);
    vb->setMargin(0);
    vb->setSpacing(2);
    hbox->addLayout(vb);

    int bh = line_height();
    bh += bh/2;
    layertab = new layertab_w(bh, this);
    vb->addWidget(layertab);

    h = line_height();
    status = new param_w(this);
    status->setMinimumHeight(h);
    status->setMaximumHeight(h);
    vb->addWidget(status);

    connect(this, SIGNAL(update_coords(int, int)),
        this, SLOT(update_coords_slot(int, int)));
}


void
mainwin::initialize()
{
    DSP()->SetWindow(0, new WindowDesc);
    EV()->SetCurrentWin(DSP()->MainWdesc());
//wib_windesc = DSP()->MainWdesc();

    DSP()->MainWdesc()->SetWbag(this);
    DSP()->MainWdesc()->SetWdraw(this);

    // This is done again in AppInit(), but it is needed for the menus.
    DSP()->SetCurMode(XM()->InitialMode());

//    xrm_load_colors();
    DSP()->ColorTab()->init();

    qtMenu()->InitMainMenu();
    qtMenu()->InitSideMenu();

    PL()->SetEdit(new QTedit(false, this));
    LT()->SetLtab(new QTltab(false, this));
}


void
mainwin::set_coord_mode(int x0, int y0, bool relative, bool snap)
{
    coords->set_mode(x0, y0, relative, snap);
}


void
mainwin::show_parameters()
{
    status->print();
    coords->print(0, 0, coord_w::COOR_REL);
}


// Move the keyboard focus to the given widget.
//
void
mainwin::set_focus(QWidget *widget)
{
/*
    GdkWindow *win = widget->window;
    if (win)
        XSetInputFocus(gr_x_display(), gr_x_window(win),
            RevertToPointerRoot, CurrentTime);
*/
}


void
mainwin::set_indicating(bool)
{
}


void
mainwin::send_key_event(sKeyEvent *kev)
{
    Qt::KeyboardModifiers mod = Qt::NoModifier;
    if (kev->state & GR_SHIFT_MASK)
        mod |= Qt::ShiftModifier;
    if (kev->state & GR_CONTROL_MASK)
        mod |= Qt::ControlModifier;
    if (kev->state & GR_ALT_MASK)
        mod |= Qt::AltModifier;

    activateWindow();
    if (kev->type == KEY_PRESS) {
        QKeyEvent ev(QEvent::KeyPress, kev->key, mod, QString(kev->text));
        QCoreApplication::sendEvent(Viewport(), &ev);
    }
    if (kev->type == KEY_RELEASE) {
        QKeyEvent ev(QEvent::KeyRelease, kev->key, mod, QString(kev->text));
        QCoreApplication::sendEvent(Viewport(), &ev);
    }
}


// Handler from the "wr" button.
//
void
mainwin::wr_btn_slot()
{
    if (XM()->IsDoingHelp() && !is_shift_down()) {
        PopUpHelp("xic:wrbtn");
        return;
    }
    char buf[128];
    sprintf(buf, "%s-%s bug", XM()->Product(), XM()->VersionString());
    PopUpMail(buf, Log()->MailAddress());
}


void
mainwin::update_coords_slot(int xx, int yy)
{
    coords->print(xx, yy, coord_w::COOR_MOTION);
}

#ifdef notdef

//-----------------------------------------------------------------------
//  Event and signal handlers
//-----------------------------------------------------------------------

static void
main_cancel_proc(GtkWidget*, void*)
{
    GTKmenuPtr->M_Exit(0);
}


static void
subwin_cancel_proc(GtkWidget*, void *client_data)
{
    int wnum = (long)client_data;
    wnum--;
    if (wnum >= 0 && wnum < NUM_SUBWINDOWS)
        delete XM()->subwins[wnum];
}


static int
map_hdlr(GtkWidget*, GdkEvent *event, void *client_data)
{
    return (true);
}


// Resize after the main or subwindow size change.  Call the application
// with the new size.
//
static int
resize_hdlr(GtkWidget *caller, GdkEvent *event, void *client_data)
{
    xic_bag *w = static_cast<xic_bag*>(client_data);
    if (event->type == GDK_CONFIGURE && w->windesc) {
        int width, height;
        gdk_window_get_size(caller->window, &width, &height);
        w->windesc->resize_callback(width, height);
    }
    return (true);
}

 
// Redraw the main or subwindow after an expose.  Break it up into
// exposed rectangles to save time.  Call the application to
// actually redraw the viewport.
//
static int
redraw_hdlr(GtkWidget *widget, GdkEvent *event, void *client_data)
{
    if (widget->parent == DontRedrawMe) {
        DontRedrawMe = 0;
        return (false);
    }
    if (gtkHaveDrag && XM()->no_pixmap_store)
        return (false);

    xic_bag *w = static_cast<xic_bag*>(client_data);

    GdkEventExpose *pev = (GdkEventExpose*)event;
    if (!GetCurSymbol() || !XM()->app_ready) {
        w->windesc->w_clip_rect = w->windesc->w_viewport;
        w->windesc->ShowGrid();
        XM()->app_ready = true;
    }
    else {
#if GTK_CHECK_VERSION(1,3,15)
        GdkRectangle *rects;
        int nrects;
        gdk_region_get_rectangles(pev->region, &rects, &nrects);
        for (int i = 0; i < nrects; i++) {
            BBox BB(rects[i].x, rects[i].y + rects[i].height,
                rects[i].x + rects[i].width, rects[i].y);
            w->windesc->Update(&BB);
        }
        g_free(rects);
#else
        BBox BB(pev->area.x, pev->area.y + pev->area.height,
            pev->area.x + pev->area.width, pev->area.y);
        w->windesc->Update(&BB);
#endif
    }
    return (true);
}


static unsigned int grabbed_key;



// Struct to hold state for button press grab action.
//
struct grabstate_t
{
    void set(GtkWidget *w, GdkEventButton *be)
        {
            gdk_pointer_grab(w->window, true, GDK_BUTTON_RELEASE_MASK,
                0, 0, be->time);
            caller = w;
            button = be->button;
            simbtn4 = false;
            send_event = be->send_event;
            if (send_event)
                gdk_pointer_ungrab(be->time);
        }

    bool is_armed(GdkEventButton *be)
        { return (caller && button == be->button); }

    void clear(unsigned int t)
        { caller = 0; gdk_pointer_ungrab(t); }

    bool is_caller(GtkWidget *w)
        { return (caller == w && !send_event); }

    bool check_simb4(bool n)
        { bool x = simbtn4; simbtn4 = n; return (x); }

    int btn()
        { return (button); }

private:
    GtkWidget *caller;      // target window
    unsigned button;        // mouse button code
    bool simbtn4;           // true when simulating button 4
    bool send_event;        // true if event was synthesized
};
static grabstate_t grabstate;


static int
focus_hdlr(GtkWidget *widget, GdkEvent *event, void *client_data)
{
    // Nothing to do but handle this to avoid default action of
    // issuing expose event
    return (true);
}

#endif

