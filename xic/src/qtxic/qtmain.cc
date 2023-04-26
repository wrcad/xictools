
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
struct NULLwin : public DSPwbag, public NULLwbag,  NULLwinApp,
    public NULLdraw
{
};


#define GS_NBTNS 8

namespace {
    /* XXX
    // Struct to hold state for button press grab action.  This overrides
    // the "automatic grab" so that the cursor tracks the subwindow
    // crossings.
    //
    struct grabstate_t
    {
        grabstate_t()
            {
                for (int i = 0; i < GS_NBTNS; i++) {
                    caller[i] = 0;
                    send_event[i] = false;
                }
                noopbtn = false;
            }

        void set(GtkWidget *w, GdkEventButton *be)
            {
                if (be->button >= GS_NBTNS)
                    return;
                gdk_pointer_grab(gtk_widget_get_window(w), true,
                    GDK_BUTTON_RELEASE_MASK, 0, 0, be->time);
                caller[be->button] = w;
                send_event[be->button] = be->send_event;

                if (be->send_event)
                    gdk_pointer_ungrab(be->time);
            }

        bool is_armed(GdkEventButton *be)
            { return (be->button < GS_NBTNS && caller[be->button] != 0); }

        void clear(GdkEventButton *be)
            {
                if (be->button >= GS_NBTNS)
                    return;
                caller[be->button] = 0;
                send_event[be->button] = false;
                gdk_pointer_ungrab(be->time);
            }

        void clear(unsigned int btn, unsigned int time)
            {
                if (btn >= GS_NBTNS)
                    return;
                caller[btn] = 0;
                send_event[btn] = false;
                gdk_pointer_ungrab(time);
            }

        bool check_noop(bool n)
            { bool x = noopbtn; noopbtn = n; return (x); }

        GtkWidget *widget(int n) { return (n <= GS_NBTNS ? caller[n] : 0); }

    private:
        GtkWidget *caller[GS_NBTNS];    // target windows
        bool send_event[GS_NBTNS];      // true if event was synthesized
        bool noopbtn;                   // true when simulating no-op button
    };

    unsigned int grabbed_key;
    grabstate_t grabstate;
    */

    /*
    // Event handling.  This contains the main event handler, plus a queue
    // for saving events for later dispatch.
    //
    struct sEventHdlr
    {
        // Event list element
        //
        struct evl_t
        {
            evl_t(GdkEvent *e) { ev = e; next = 0; }
            ~evl_t() { if (ev) gdk_event_free(ev); }

            GdkEvent *ev;
            evl_t *next;
        };

        sEventHdlr() { event_list = 0; }

        bool has_saved_events() { return (event_list != 0); }

        // Process and clear the event list.
        //
        void do_saved_events()
            {
                while (event_list) {
                    evl_t *event = event_list;
                    event_list = event->next;
                    gtk_main_do_event(event->ev);
                    delete event;
                }
            }

        // Append an event to the list.
        //
        void save_event(GdkEvent *ev)
            {
                ev = gdk_event_copy(ev);
                if (!event_list)
                    event_list = new evl_t(ev);
                else {
                    evl_t *evl = event_list;
                    while (evl->next)
                        evl = evl->next;
                    evl->next = new evl_t(ev);
                }
            }

        static void main_event_handler(GdkEvent *event, void*);

    private:
        evl_t *event_list;
    };

    bool button_state[GS_NBTNS];
    sEventHdlr event_hdlr;
    */

    namespace main_local {
        void wait_cursor(bool);
        int timer_cb(void*);
#ifdef HAVE_MOZY
        void quit_help(void*);
        void form_submit_hdlr(void*);
#endif
    };

    /* XXX
    inline bool is_modifier_key(unsigned keysym)
    {
        return ((keysym >= GDK_KEY_Shift_L && keysym <= GDK_KEY_Hyper_R) ||
            keysym == GDK_KEY_Mode_switch || keysym == GDK_KEY_Num_Lock);
    }
    */

    // When the application is busy, all button presses and all key
    // presses except for Ctrl-C are locked out, and upon receipt a
    // pop-up appears telling the user to use Ctrl-C to abort.  The
    // pop-up disappears in a few seconds.
    //
    // All other events are dispatched normally when busy.
    //

    // Timeout procedure for message pop-up.
    //
    int
    busy_msg_timeout(void*)
    {
        if (qtPkgIf()->BusyPopup())
            qtPkgIf()->BusyPopup()->popdown();
        return (false);
    }

    void
    pop_busy()
    {
        const char *busy_msg =
            "Working...\nPress Control-C in main window to abort.";

        if (!qtPkgIf()->BusyPopup() && QTmainwin::self()) {
            qtPkgIf()->SetBusyPopup(
                QTmainwin::self()->PopUpErrText(busy_msg, STY_NORM));
            if (qtPkgIf()->BusyPopup())
                qtPkgIf()->BusyPopup()->
                    register_usrptr((void**)qtPkgIf()->BusyPopupHome());
            qtPkgIf()->RegisterTimeoutProc(3000, busy_msg_timeout, 0);
        }
    }
}


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
        return (new NULLwin);
    if (MainDev()->ident == _devQT_)
        return (new QTmainwin);
    return (0);
}


static void
messageOutput(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:
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

        NULLwin *w = dynamic_cast<NULLwin*>(wcp);
        DSP()->SetWindow(0, new WindowDesc);
        EV()->SetCurrentWin(DSP()->MainWdesc());
        DSP()->MainWdesc()->SetWbag(w);
        DSP()->MainWdesc()->SetWdraw(w);

        DSP()->SetCurMode(XM()->InitialMode());

        DSP()->ColorTab()->init();
        DSP()->Initialize(600, 400, 0, true);
        LT()->InitLayerTable();

        return (false);
    }
    if (MainDev()->ident != _devQT_)
        return (true);

    pkg_idle_control = new idle_proc();

    QTmainwin *w = dynamic_cast<QTmainwin*>(wcp);
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

#ifdef HAVE_MOZY
    // callback to tell application when quitting help
    HLP()->context()->registerQuitHelpProc(main_local::quit_help);
    HLP()->context()->registerFormSubmitProc(main_local::form_submit_hdlr);
#endif

    PL()->Init();
    qtEdit()->init();
//    GRX->RegisterBigWindow(w->Shell());

    Gst()->SetGhost(GFnone);
    if (!QTmainwin::self())
        // Halt called
        return (true);
    return (false);
}


// Reinitialize the application to shut down all graphics.
//
void
QTpkg::ReinitNoGraphics()
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return;
    if (!QTmainwin::self())
        return;
    DSPmainDraw(Halt());
    DSP()->MainWdesc()->SetWbag(0);
    DSP()->MainWdesc()->SetWdraw(0);
    CloseGraphicsConnection();

    PL()->SetNoGraphics();
    LT()->SetNoGraphics();

    GRpkgIf()->SetNullGraphics();
    EV()->SetCurrentWin(DSP()->MainWdesc());
    NULLwin *w = new NULLwin;
    DSP()->MainWdesc()->SetWbag(w);
    DSP()->MainWdesc()->SetWdraw(w);
}


// Halt application cleanup function.
//
void
QTpkg::Halt()
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return;
//    GRX->RegisterBigWindow(0);
    EV()->InitCallback();
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
    if (!MainDev() || MainDev()->ident != _devQT_)
        return;
    RegisterEventHandler(0, 0);
    pkg_in_main_loop = true;
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
    if (!QTmainwin::self())
        return (false);
    if (wnum < 1 || wnum >= DSP_NUMWINS)
        return (false);
    QTsubwin *w = new QTsubwin(wnum, QTmainwin::self());
    w->subw_initialize(wnum);
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
    QTsubwin *w = dynamic_cast<QTsubwin*>(wdesc->Wbag());
    if (w) {
        wdesc->SetWbag(0);
        wdesc->SetWdraw(0);
        w->pre_destroy(wnum);
        delete w;
    }
    Menu()->DestroySubwinMenu(wnum);
}


// Increment/decrement the busy flag and switch to watch cursor.  If
// already busy when passing true, return false.
//
bool
QTpkg::SetWorking(bool busy)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return (false);

    // Have to be careful with reentrancy, specifically app_busy must
    // have its final value before calling the functions (which can
    // call back!).

    bool ready = true;
    if (busy) {
        if (!app_busy) {
            DSP()->SetInterrupt(DSPinterNone);
            if (!app_override_busy) {
                app_busy++;
                main_local::wait_cursor(true);
                return (ready);
            }
        }
        else
            ready = false;
        app_busy++;
    }
    else {
        if (app_busy == 1) {
            app_busy--;
            main_local::wait_cursor(false);
            /*
            if (event_hdlr.has_saved_events())
                // dispatch saved events
                RegisterIdleProc(saved_events_idle, 0);
            */
            return (ready);
        }
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
            main_local::wait_cursor(false);
        app_override_busy = true;
    }
    else {
        if (app_busy)
            main_local::wait_cursor(true);
        app_override_busy = false;
    }
}


// Print into buf an identifier string for the main window, the
// X window number in this case.
//
bool
QTpkg::GetMainWinIdentifier(char *buf)
{
    *buf = 0;
//    if (!MainDev() || MainDev()->ident != _devQT_)
//        return (false);
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
    /*
#ifdef WITH_X11
    return (true);
#else
    */
    return (false);
//#endif
}


void
QTpkg::CloseGraphicsConnection()
{
    if (!MainDev() || MainDev()->ident != _devQT_)
        return;
    if (GRX->ConnectFd() > 0) {
//        gtk_main_quit();
        close(GRX->ConnectFd());
    }
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
    return (true);
}


int
QTpkg::RegisterIdleProc(int(*cb)(void*), void *arg)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
         return (0);
    return (pkg_idle_control->add(cb, arg));
}


bool
QTpkg::RemoveIdleProc(int id)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
         return (false);
    return (pkg_idle_control->remove(id));
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
QTpkg::StartTimer(int time, bool *set)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
         return (0);
    int id = 0;
    if (set)
        *set = false;
    if (time > 0)
        id = GRX->AddTimer(time, main_local::timer_cb, set);
    return (id);
}


// Set the application fonts.  This should work for either Pango
// or XFD font names.
//
void
QTpkg::SetFont(const char *fname, int fnum, FNT_FMT fmt)
{
    if (!MainDev() || MainDev()->ident != _devQT_)
         return;

    // Ignore XFDs, but anything else should be ok.
    if (fmt == FNT_FMT_X)
        return;

    if (!xfd_t::is_xfd(fname))
        FC.setName(fname, fnum);
}


// Return the application font names.
//
const char *
QTpkg::GetFont(int fnum)
{
    // Valid for NULL graphics also.
    return (FC.getName(fnum));
}


// Return the format code for the description string returned by GetFont.
//
FNT_FMT
QTpkg::GetFontFmt()
{
    return (FNT_FMT_Q);
}


// Register an event handler, which is called for each event received
// by the application.
//
void
QTpkg::RegisterEventHandler(void(*handler)(QEvent*, void*), void *arg)
{
    (void)handler;
/*XXX
    if (handler)
        gdk_event_handler_set(handler, arg, 0);
    else
        gdk_event_handler_set(event_hdlr.main_event_handler, 0, 0);
*/
}
// End of QTpkg functions


//-----------------------------------------------------------------------------
// cKeys functions

cKeys::cKeys(int wnum, QWidget *prnt) : draw_qt_w(false, prnt)
{
    keypos = 0;
    memset(keys, 0, CBUFMAX+1);
    win_number = wnum;

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_SCREEN))
        set_font(fnt);
}


void
cKeys::show_keys()
{
    char *s = keys + (keypos > 5 ? keypos - 5 : 0);
    clear();
    int yy = line_height();
    draw_text(2, yy, s, -1);
    update();
}


void
cKeys::set_keys(const char *text)
{
    while (keypos)
        keys[--keypos] = '\0';
    if (text) {
        strncpy(keys, text, CBUFMAX);
        keypos = strlen(keys);
    }
}


void
cKeys::bsp_keys()
{
    if (keypos)
        keys[--keypos] = '\0';
}


void
cKeys::check_exec(bool exact)
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
// QTsubwin functions

DrawType QTsubwin::sw_drawtype = DrawNative;

QTsubwin::QTsubwin(int wnum, QWidget *prnt) : QDialog(prnt), QTbag(this),
    QTdraw(XW_DRAWING)
{
    sw_windesc = 0;

    if (wnum < 0)
        sw_win_number = -1;
    else
        sw_win_number = wnum;
    sw_expand = 0;

    sw_menubar = new QMenuBar(this);

    if (sw_win_number == 0) {
        const char *str = getenv("GR_SYSTEM");
        if (str) {
            if (!strcasecmp(str, "X"))
                sw_drawtype = DrawX;
            if (!strcasecmp(str, "GL"))
                sw_drawtype = DrawGL;
            if (!strcasecmp(str, "QT") || !strcasecmp(str, "native"))
                sw_drawtype = DrawNative;
        }
        if (sw_drawtype == DrawGL)
            printf("Using OpenGL graphics system.\n");
        else if (sw_drawtype == DrawX)
            printf("Using custom X-Windows graphics system.\n");
        else
            printf("Using QT native graphics system.\n");
    }
    gd_viewport = draw_if::new_draw_interface(sw_drawtype, true, this);
    Gbag()->set_draw_if(gd_viewport);
    Viewport()->setFocusPolicy(Qt::StrongFocus);

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_SCREEN))
        gd_viewport->set_font(fnt);

    sw_keys_pressed = new cKeys(sw_win_number, this);

    connect(Viewport(), SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));
    connect(Viewport(), SIGNAL(new_painter(QPainter*)),
        this, SLOT(new_painter_slot(QPainter*)));
    connect(Viewport(), SIGNAL(paint_event(QPaintEvent*)),
        this, SLOT(paint_slot(QPaintEvent*)));
    connect(Viewport(), SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(button_down_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(button_up_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(move_event(QMouseEvent*)),
        this, SLOT(motion_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(key_press_event(QKeyEvent*)),
        this, SLOT(key_down_slot(QKeyEvent*)));
    connect(Viewport(), SIGNAL(key_release_event(QKeyEvent*)),
        this, SLOT(key_up_slot(QKeyEvent*)));
    connect(Viewport(), SIGNAL(enter_event(QEvent*)),
        this, SLOT(enter_slot(QEvent*)));
    connect(Viewport(), SIGNAL(leave_event(QEvent*)),
        this, SLOT(leave_slot(QEvent*)));
    connect(Viewport(), SIGNAL(drag_enter_event(QDragEnterEvent*)),
        this, SLOT(drag_enter_slot(QDragEnterEvent*)));
    connect(Viewport(), SIGNAL(drop_event(QDropEvent*)),
        this, SLOT(drop_slot(QDropEvent*)));

    if (sw_win_number == 0)
        // being subclassed for main window
        return;

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    sw_keys_pressed->setFixedHeight(line_height() + 4);
    sw_keys_pressed->setFixedWidth(6*char_width());
    sw_keys_pressed->move(160, 2);

    vbox->setMenuBar(sw_menubar);
    vbox->addWidget(Viewport());
}


QTsubwin::~QTsubwin()
{
    PopUpExpand(0, MODE_OFF, 0, 0, 0, false);
    PopUpGrid(0, MODE_OFF);
    PopUpZoom(0, MODE_OFF);
}


// Function to initialize a subwindow.
//
void
QTsubwin::subw_initialize(int wnum)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%s %d", XM()->Product(), wnum);
    setWindowTitle(buf);

    sw_windesc = DSP()->Window(wnum);
    DSP()->Window(wnum)->SetWbag(this);
    DSP()->Window(wnum)->SetWdraw(this);

    // Create new menu, just copy template.
    Menu()->CreateSubwinMenu(wnum);

    connect(this, SIGNAL(update_coords(int, int)),
        QTmainwin::self(), SLOT(update_coords_slot(int, int)));
/*XXX
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
        ShellGeometry(QTmainwin::self()->shell, &rect, 0);
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

    SetWindowBackground(0);
    Clear();

    // Application initialization callback.
    //
    QSize qs = Viewport()->size();
//XXX draws twice, first here than later from resize
    DSP()->Initialize(qs.width(), qs.height(), wnum);
    show();
    raise();
    activateWindow();
    XM()->UpdateCursor(sw_windesc, (CursorType)Gbag()->get_cursor_type(),
        true);
}


void
QTsubwin::pre_destroy(int wnum)
{
    /* XXX
    GdkRectangle rect, rect_d;
    gtk_ShellGeometry(wb_shell, &rect, &rect_d);

    // Save relative to corner of main window.  Absolute coords can
    // change if the main window is moved to a different monitor in
    // multi-monitor setups.

    int x, y;
    gdk_window_get_origin(gtk_widget_get_window(GTKmainwin::self()->Shell()),
        &x, &y);
    LastPos[wnum].x = rect_d.x - x;
    LastPos[wnum].y = rect_d.y - y;
    LastPos[wnum].width = rect.width;
    LastPos[wnum].height = rect.height;

    g_signal_handlers_disconnect_by_func(G_OBJECT(wb_shell),
        (gpointer)subwin_cancel_proc, (void*)(intptr_t)wnum);
    */
}


// cAppWinFuncs interface

// The following two functions implement pixmap buffering for screen
// redisplays.  This often leads to faster redraws.  This function
// will create a pixmap and make it the current context drawable.
//
void
QTsubwin::SwitchToPixmap()
{
    if (!sw_windesc)
        return;
    // set draw to pixmap
}


// Copy out the BB area of the pixmap to the screen, and switch the
// context drawable back to the screen.
//
void
QTsubwin::SwitchFromPixmap(const BBox *BB)
{
    CopyPixmap(BB);
}


// Return the pixmap, set by a previous call to SwitchToPixmap(), and
// reset the context to normal.
//
GRobject
QTsubwin::DrawableReset()
{
    return (0);
}


// Copy out the BB area of the pixmap to the screen.
//
void
QTsubwin::CopyPixmap(const BBox *BB)
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
QTsubwin::DestroyPixmap()
{
}


// Return true if the pixmap is alive and well.
//
bool
QTsubwin::PixmapOk()
{
    return (true);
}


// Function to dump a window to a file, not part of the hardcopy system.
//
bool
QTsubwin::DumpWindow(const char *filename, const BBox *AOI = 0)
{
#ifdef NOTDEF
#ifdef HAVE_MOZY
// This uses the imsave package from mozy.

    // Note that the bounding values are included in the display.
    if (!sw_windesc)
        return (false);
    BBox BB = AOI ? *AOI : sw_windesc->Viewport();
    ViewportClip(BB, sw_windesc->Viewport());
    if (BB.right < BB.left || BB.bottom < BB.top)
        return (false);

#if GTK_CHECK_VERSION(3,0,0)
    ndkPixmap *pm = 0;
#else
    GdkPixmap *pm = 0;
#endif
    bool native = false;
    int vp_width = sw_windesc->ViewportWidth();
    int vp_height = sw_windesc->ViewportHeight();
#if GTK_CHECK_VERSION(3,0,0)
    if (GetDrawable()->get_pixmap() &&
            GetDrawable()->get_pixmap()->get_width() == vp_width &&
            GetDrawable()->get_pixmap()->get_height() == vp_height) { 
        pm = GetDrawable()->get_pixmap();
#else
    if (wib_draw_pixmap && wib_px_width == vp_width &&
            wib_px_height == vp_height) {
        pm = wib_draw_pixmap;
#endif
        native = true;
    }
    else {
#if GTK_CHECK_VERSION(3,0,0)
        pm = new ndkPixmap(GetDrawable()->get_window(), vp_width, vp_height);
        if (!pm)
            return (false);
        pm->inc_ref();
        GetDrawable()->set_pixmap(pm);
        wib_windesc->RedisplayDirect();
        GetDrawable()->set_window(0);
        GetDrawable()->set_window(gtk_widget_get_window(gd_viewport));
#else
        pm = gdk_pixmap_new(gd_window, vp_width, vp_height,
            gdk_visual_get_depth(GRX->Visual()));
        if (!pm)
            return (false);
        GdkWindow *tmpw = gd_window;
        gd_window = pm;
        wib_windesc->RedisplayDirect();
        gd_window = tmpw;
#endif
    }
#ifdef WIN32
    GdkGCValuesMask Win32GCvalues = (GdkGCValuesMask)(
        GDK_GC_FOREGROUND |
        GDK_GC_BACKGROUND);
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        BB.left -= xoff;
        BB.bottom -= yoff;
        BB.right -= xoff;
        BB.top -= yoff;
    }
    Image *im = create_image_from_drawable(dc, 0,
        BB.left, BB.top, BB.width() + 1, abs(BB.height()) + 1);
    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);
#else
#ifdef WITH_X11
#if GTK_CHECK_VERSION(3,0,0)
    Image *im = create_image_from_drawable(gr_x_display(),
        pm->get_xid(), BB.left, BB.top, BB.width() + 1, abs(BB.height()) + 1);
#else
    Image *im = create_image_from_drawable(gr_x_display(),
        GDK_WINDOW_XWINDOW(pm),
        BB.left, BB.top, BB.width() + 1, abs(BB.height()) + 1);
#endif
#else
#ifdef WITH_QUARTZ
//XXX Need equiv. code for Quartz.
    Image *im = 0;
#endif  // WITH_QUARTZ
#endif  // WITH_X11
#endif  // WIN32

    if (!native)
#if GTK_CHECK_VERSION(3,0,0)
        pm->dec_ref();
#else
        gdk_pixmap_unref(pm);
#endif
    if (!im)
        return (false);
    ImErrType ret = im->save_image(filename, 0);
    if (ret == ImError) {
        Log()->ErrorLog(mh::Internal,
            "Image creation failed, internal error.");
    }
    else if (ret == ImNoSupport) {
        Log()->ErrorLog(mh::Processing,
            "Image creation failed, file type unsupported.");
    }
    delete im;
    return (true);
#else
    (void)filename;
    (void)AOI;
    return (false);
#endif
#endif  // NOTDEF
(void)filename;
(void)AOI;
return (false);
}


// Get the "keyspressed" in buf
//
void
QTsubwin::GetTextBuf(char *buf)
{
    for (int i = 0; i < sw_keys_pressed->key_pos(); i++)
        buf[i] = sw_keys_pressed->key(i);
    buf[sw_keys_pressed->key_pos()] = 0;
}


// Set the "keyspressed".
//
void
QTsubwin::SetTextBuf(const char *buf)
{
    sw_keys_pressed->set_keys(buf);
    sw_keys_pressed->show_keys();
}


void
QTsubwin::ShowKeys()
{
    sw_keys_pressed->show_keys();
}


void
QTsubwin::SetKeys(const char *text)
{
    sw_keys_pressed->set_keys(text);
}


void
QTsubwin::BspKeys()
{
    sw_keys_pressed->bsp_keys();
}


bool
QTsubwin::AddKey(int c)
{
    int keypos = sw_keys_pressed->key_pos();
    if (c > ' ' && c <= '~' && keypos < CBUFMAX) {
        sw_keys_pressed->append(c);
        return (true);
    }
    if (c == ' ' && keypos > 0 && keypos < CBUFMAX) {
        sw_keys_pressed->append(c);
        return (true);
    }
    return (false);
}


bool
QTsubwin::CheckBsp()
{
    int keypos = sw_keys_pressed->key_pos();
    if (keypos > 0 &&
            sw_keys_pressed->key(keypos - 1) == Kmap()->SuppressChar()) {
        BspKeys();
        return (true);
    }
    return (false);
}


void
QTsubwin::CheckExec(bool exact)
{
    sw_keys_pressed->check_exec(exact);
}


char *
QTsubwin::KeyBuf()
{
    return (sw_keys_pressed->key_buf());
}


int
QTsubwin::KeyPos()
{
    return (sw_keys_pressed->key_pos());
}


void
QTsubwin::SetLabelText(const char*)
{
}


void
QTsubwin::PopUpGrid(GRobject, ShowMode)
{
}


void
QTsubwin::PopUpExpand(GRobject caller, ShowMode mode,
    bool (*callback)(const char*, void*),
    void *arg, const char *string, bool nopeek)
{
    if (mode == MODE_OFF) {
        if (sw_expand)
            sw_expand->popdown();
        return;
    }
    if (mode == MODE_UPD) {
        if (sw_expand)
            sw_expand->update(string);
        return;
    }
    if (sw_expand)
        return;

    sw_expand = new cExpand(this, string, nopeek, arg);
    sw_expand->register_caller(caller);
    sw_expand->register_callback(callback);
    sw_expand->set_visible(true);
}


void
QTsubwin::PopUpZoom(GRobject caller, ShowMode mode)
{
/*
    if (!GRX || !QTmainwin::self())
        return;
    if (mode == MODE_OFF) {
        if (sw_zoompop)
            sw_zoompop->popdown();
        return;
    }
    if (mode == MODE_UPD) {
        if (sw_zoompop)
            sw_zoompop->update();
        return;
    }
    if (sw_zoompop)
        return;

    sw_zoompop = new sZm(this, wib_windesc);
    sw_zoompop->register_usrptr((void**)&wib_zoompop);
    if (!wib_zoompop->shell()) {
        delete sw_zoompop;
        sw_zoompop = 0;
        return;
    }

    sw_zoompop->register_caller(caller);
    sw_zoompop->initialize(x, y);
    sw_zoompop->set_visible(true);
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
QTsubwin::keypress_handler(unsigned int keyval, unsigned int state,
    const char *keystring, bool from_prline, bool up)
{
    if (!sw_windesc)
        return (false);

//XXX QT uses its own key codes FIXME!

    // The code: 0x00 - 0x16 are the KEYcode enum values.
    //           0x17 - 0x1f unused
    //           0x20 - 0xffff X keysym
    // The X keysyms 0x20 - 0x7e match the ascii character.
    int code = 0;
    if (up) {
        for (keymap *k = Kmap()->KeymapUp(); k->keyval; k++) {
            if (k->keyval == keyval) {
                code = k->code;
                break;
            }
        }
        if (code == SHIFTUP_KEY || code == CTRLUP_KEY)
            EV()->KeypressCallback(sw_windesc, code, (char*)"", state);
        return (true);
    }

    // Tell the application if the mouse is currently over the prompt
    // area.  If so, we may want to handle some events differently.
    EV()->SetFromPromptLine(from_prline);

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
        if (fstr) {
            if (*fstr == '!') {
                XM()->TextCmd(fstr + 1, false);
                delete [] fstr;
                return (true);
            }
            strncpy(tbuf, fstr, CBUFMAX);
            delete [] fstr;
        }

        if (EV()->KeypressCallback(sw_windesc, code, tbuf, state))
            return (true);
        SetKeys(tbuf);
        ShowKeys();
        CheckExec(true);
        return (true);
    }
    if (code == 0 && keyval >= 0x20)
        code = keyval;

    for (keyaction *k = Kmap()->ActionMapPre(); k->code; k++) {
        if (k->code == code &&
                (!k->state || k->state == (state & MODMASK))) {
            if (EV()->KeyActions(sw_windesc, k->action, &code))
                return (true);
            break;
        }
    }

    char tbuf[CBUFMAX + 1];
    memset(tbuf, 0, sizeof(tbuf));
    if (keystring)
        strncpy(tbuf, keystring, CBUFMAX);
    if (EV()->KeypressCallback(sw_windesc, code, tbuf, state))
        return (true);

    if (state & GR_ALT_MASK)
        return (false);

    for (keyaction *k = Kmap()->ActionMapPost(); k->code; k++) {
       if (k->code == code &&
                (!k->state || k->state == (state & MODMASK))) {
            if (EV()->KeyActions(sw_windesc, k->action, &code))
                return (true);
            break;
        }
    }
    if (code == RETURN_KEY) {
        if (sw_keys_pressed->key_pos() > 0) {
            CheckExec(true);
            return (true);
        }
        return (false);
    }
    if (!AddKey(*tbuf))
        return (false);
    ShowKeys();
    CheckExec(false);
    return (true);
}


// QTsubwin slots.

// Resize after the main or subwindow size change.  Call the application
// with the new size.
//
void
QTsubwin::resize_slot(QResizeEvent *ev)
{
    if (!sw_windesc)
        return;
    EV()->ResizeCallback(sw_windesc, ev->size().width(), ev->size().height());
    sw_windesc->Redisplay(0);
    ev->setAccepted(true);

    if (DSP()->NoPixmapStore() &&
            (!DSP()->CurCellName() || !XM()->IsAppReady())) {
        *sw_windesc->ClipRect() = sw_windesc->Viewport();
        sw_windesc->ShowGrid();
    }
    XM()->SetAppReady(true);
}


void
QTsubwin::new_painter_slot(QPainter *p)
{
printf("new painter\n");
}


void
QTsubwin::paint_slot(QPaintEvent *ev)
{
printf("paint event\n");
}


// Dispatch button press events to the application callbacks.
//
void
QTsubwin::button_down_slot(QMouseEvent *ev)
{
    if (!sw_windesc)
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

    bool showing_ghost = gd_gbag->showing_ghost();
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
            else {
                EV()->ButtonNopCallback(sw_windesc, ev->x(), ev->y(),
                    mod_state(state));
            }
        }
        else {
            EV()->Button1Callback(sw_windesc, ev->x(), ev->y(),
                mod_state(state));
        }
        break;
    case 2:
        // Pan operation
        if (XM()->IsDoingHelp() && !(state & Qt::ShiftModifier))
            PopUpHelp("button2");
        else {
            EV()->Button2Callback(sw_windesc, ev->x(), ev->y(),
                mod_state(state));
        }
        break;
    case 3:
        // Zoom opertion
        if (XM()->IsDoingHelp() && !(state & Qt::ShiftModifier))
            PopUpHelp("button3");
        else {
            EV()->Button3Callback(sw_windesc, ev->x(), ev->y(),
                mod_state(state));
        }
        break;
    default:
        // No-op, update coordinates
        if (XM()->IsDoingHelp() && !(state & Qt::ShiftModifier))
            PopUpHelp("button4");
        else {
            EV()->ButtonNopCallback(sw_windesc, ev->x(), ev->y(),
                mod_state(state));
        }
        break;
    }
    if (showing_ghost)
        ShowGhost(DISPLAY);
}


// Dispatch button release events to the application callbacks.
//
void
QTsubwin::button_up_slot(QMouseEvent *ev)
{
    if (!sw_windesc)
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
    const BBox *BB = &sw_windesc->Viewport();
    bool in = (ev->x() >= 0 && ev->x() < BB->right &&
        ev->y() >= 0 && ev->y() < BB->bottom);

    bool showing_ghost = gd_gbag->showing_ghost();
    if (showing_ghost)
        ShowGhost(ERASE);

    int state = ev->modifiers();

    switch (button) {
    case 1:
//        if (grabstate.check_simb4(false)) {
        if (0) {
            EV()->ButtonNopReleaseCallback((in ? sw_windesc : 0),
                ev->x(), ev->y(), mod_state(state));
        }
        else {
            EV()->Button1ReleaseCallback((in ? sw_windesc : 0),
                ev->x(), ev->y(), mod_state(state));
        }
        break;
    case 2:
        EV()->Button2ReleaseCallback((in ? sw_windesc : 0),
            ev->x(), ev->y(), mod_state(state));
        break;
    case 3:
        EV()->Button3ReleaseCallback((in ? sw_windesc : 0),
            ev->x(), ev->y(), mod_state(state));
        break;
    default:
        EV()->ButtonNopReleaseCallback((in ? sw_windesc : 0),
            ev->x(), ev->y(), mod_state(state));
        break;
    }
    if (showing_ghost)
        ShowGhost(DISPLAY);
}


// Handle pointer motion in the main and subwindows.  In certain
// commands, "ghost" XOR drawing is utilized.
//
void
QTsubwin::motion_slot(QMouseEvent *ev)
{
    if (sw_windesc) {
        EV()->MotionCallback(sw_windesc, mod_state(ev->modifiers()));
        WindowDesc *worig = EV()->ZoomWin();
        if (!worig)
            worig = DSP()->Windesc(EV()->Cursor().get_window());
        if (!worig)
            worig = DSP()->MainWdesc();
        if (sw_windesc->IsSimilar(worig)) {
            UndrawGhost();
            DrawGhost(ev->x(), ev->y());
        }
        int xx = ev->x();
        int yy = ev->y();
        sw_windesc->PToL(xx, yy, xx, yy);
        emit update_coords(xx, yy);
    }
}


namespace {
    // Return true if the mouse pointer is currently over the prompt
    // line area in the main window.
    //
    bool pointer_in_prompt_area()
    {
        /*XXX 
        int x, y;
        GRX->PointerRootLoc(&x, &y);
        GtkWidget *w = GTKmainwin::self()->TextArea();
        GdkRectangle r;
        gtk_ShellGeometry(w, &r , 0);
        return (x >= r.x && x <= r.x + r.width &&
            y >= r.y && y <= r.y + r.height);
        */
        return (false);
    }
}


// Key press processing for the drawing windows
//
void
QTsubwin::key_down_slot(QKeyEvent *ev)
{
    /*
    if (!QTmainwin::self() || qtPkgIf()->NotMapped())
        return (true);
    */

    if (message)
        message->popdown();

    QString qs = ev->text().toLatin1();
    const char *string = (const char*)qs.constData();
    int kpos = sw_keys_pressed->key_pos();
    if (!is_modifier_key(ev->key()) && kpos &&
            sw_keys_pressed->key(kpos - 1) == Kmap()->SuppressChar())
        sw_keys_pressed->bsp_keys();
/*XXX
    else if (Kmap()->MacroExpand(kev->keyval, kev->state, false))
        return (true);

    if (ev->key() == Qt::Key_Shift || ev->key() == Qt::Key_Control) {
        gdk_keyboard_grab(caller->window, true, kev->time);
        grabbed_key = kev->keyval;
    }
*/

    if (keypress_handler(ev->key(), mod_state(ev->modifiers()), string,
            pointer_in_prompt_area(), false))
        ev->accept();
}


// Key release processing for the drawing windows
//
void
QTsubwin::key_up_slot(QKeyEvent *ev)
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
            false, true))
        ev->accept();
}


// Reinitialize "ghost" drawing when the pointer enters a window.
// If the pointer enters a window with an active grab, check if
// a button release has occurred.  If so, and the condition has
// not already been cleared, call the appropriate callback to
// clean up.  This is for button up events that get 'lost'.
//
void
QTsubwin::enter_slot(QEvent *ev)
{
    if (!sw_windesc)
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

//XXX    EV()->MotionCallback(sw_windesc, mod_state(ev->modifiers()));
    EV()->MotionCallback(sw_windesc, mod_state(0));
    ev->setAccepted(true);
}


// Gracefully terminate ghost drawing when the pointer leaves a
// window.
//
void
QTsubwin::leave_slot(QEvent *ev)
{
    if (!sw_windesc)
        return;
//XXX    EV()->MotionCallback(sw_windesc, mod_state(ev->modifiers()));
    EV()->MotionCallback(sw_windesc, mod_state(0));
    UndrawGhost();
    gd_gbag->set_ghost_func(gd_gbag->get_ghost_func());  // set first flag
    ev->setAccepted(true);
}


void
QTsubwin::drag_enter_slot(QDragEnterEvent *ev)
{
}


void
QTsubwin::drop_slot(QDropEvent *ev)
{
}
// End of QRsubwin slots.
 

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


//-----------------------------------------------------------------------------
// GTKmainwin functions

QTmainwin::QTmainwin() : QTsubwin(0, 0)
{
    mw_top_button_box = 0;
    mw_phys_button_box = 0;
    mw_elec_button_box = 0;

    mw_promptline = 0;
    mw_coords = 0;
    mw_layertab = 0;
    mw_status = 0;

    QAction *a = sw_menubar->addAction(QString("wr"));
    a->setIcon(QIcon(QPixmap(wr_xpm)));
    connect(a, SIGNAL(triggered()), this, SLOT(wr_btn_slot()));

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);
    // By default, this does nothiong in Apple, menus are set in the
    // main display frame.
    vbox->setMenuBar(sw_menubar);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    mw_top_button_box = new QWidget(this);
    hbox->addWidget(mw_top_button_box);
    mw_coords = new cCoord(this);
    hbox->addWidget(mw_coords);
    hbox->addWidget(new QWidget(this));  // Filler

    hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    mw_phys_button_box = new QWidget(this);
    hbox->addWidget(mw_phys_button_box);
    mw_elec_button_box = new QWidget(this);
    hbox->addWidget(mw_elec_button_box);
    mw_layertab = new QTltab(false, this);
    hbox->addWidget(mw_layertab);
    hbox->addWidget(Viewport());

    hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    int w = char_width() * 7 + 4;
    int h = line_height() + 4;
    sw_keys_pressed->setFixedSize(w, h);
    hbox->addWidget(sw_keys_pressed);

    mw_promptline = draw_if::new_draw_interface(DrawNative, false, this);
    mw_promptline->widget()->setMinimumHeight(h);
    mw_promptline->widget()->setMaximumHeight(h);
    hbox->addWidget(mw_promptline->widget());

    h = line_height();
    mw_status = new cParam(this);
    mw_status->setMinimumHeight(h);
    mw_status->setMaximumHeight(h);
    vbox->addWidget(mw_status);

    connect(this, SIGNAL(update_coords(int, int)),
        this, SLOT(update_coords_slot(int, int)));
}


void
QTmainwin::initialize()
{
    DSP()->SetWindow(0, new WindowDesc);
    EV()->SetCurrentWin(DSP()->MainWdesc());
    sw_windesc = DSP()->MainWdesc();

    // We use w_draw for rendering, as this pointer may point to a
    // hard-copy context.  Rendering through w_wbag will always go
    // to the screen.
    //
    DSP()->MainWdesc()->SetWbag(this);
    DSP()->MainWdesc()->SetWdraw(this);

    // This is done again in AppInit(), but it is needed for the menus.
    DSP()->SetCurMode(XM()->InitialMode());

//    xrm_load_colors();
    DSP()->ColorTab()->init();

    qtMenu()->InitMainMenu();
    qtMenu()->InitTopButtonMenu();
    qtMenu()->InitSideButtonMenus();

    PL()->SetEdit(new QTedit(false, this));
    LT()->SetLtab(mw_layertab);
}


void
QTmainwin::send_key_event(sKeyEvent *kev)
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
QTmainwin::wr_btn_slot()
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
QTmainwin::update_coords_slot(int xx, int yy)
{
    mw_coords->print(xx, yy, cCoord::COOR_MOTION);
}


//-----------------------------------------------------------------------
// Local functions

/*
void
main_local::wait_cursor(bool waiting)
{
    static CursorType cursor_type;
    if (waiting) {
        if (XM()->GetCursor() != CursorBusy) {
            cursor_type = XM()->GetCursor();
            XM()->UpdateCursor(0, CursorBusy);
        }
    }
    else
        XM()->UpdateCursor(0, cursor_type);
}
*/

void
main_local::wait_cursor(bool waiting)
{
    if (waiting)
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    else
        QApplication::restoreOverrideCursor();
}


// Callback for general purpose timer.  Set *set true when time limit
// reached.
//
int
main_local::timer_cb(void *client_data)
{
    if (client_data)
        *((bool*)client_data) = true;
    return (false);
}


#ifdef HAVE_MOZY

void
main_local::quit_help(void*)
{
    XM()->QuitHelp();
}


//
// This is the HTML form submission handler.  It sets the variables
// from the form, and calls a script for processing.
//

// The form of the action string is the following token followed by the
// name of the script (can be a path)
//
#define ACTION_TOKEN "action_local_xic"

void
main_local::form_submit_hdlr(void *data)
{
    static bool lock;  // the script interpreter isn't reentrant
    htmFormCallbackStruct *cbs = (htmFormCallbackStruct*)data;

    const char *t = cbs->action;
    char *tok = lstring::getqtok(&t);
    if (strcmp(tok, ACTION_TOKEN)) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "unknown action_local submission.\n");
        delete [] tok;
        return;
    }
    delete [] tok;
    if (lock)
        return;
    lock = true;
    char *script = lstring::getqtok(&t);

    SIfile *sfp;
    stringlist *wl;
    XM()->OpenScript(script, &sfp, &wl);
    if (!sfp && !wl) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't find action_local script.\n");
        delete [] script;
        lock = false;
        return;
    }

    siVariable *variables = 0;
    int scnt = 0;
    for (int i = 0; i < cbs->ncomponents; i++) {
        siVariable *v = new siVariable;
        v->name = lstring::copy(cbs->components[i].name);
        switch (cbs->components[i].type) {
        case FORM_NONE:
            delete v;
            continue;
        case FORM_SELECT:
            // deal with multiple selections
            if (i && cbs->components[i-1].type == FORM_SELECT &&
                    !strcmp(cbs->components[i-1].name,
                    cbs->components[i].name)) {
                char buf[64];
                snprintf(buf, 64, "%s_extra%d", cbs->components[i].name, scnt);
                delete [] v->name;
                v->name = lstring::copy(buf);
                scnt++;
            }
            else
                scnt = 0;
            // fall through
        case FORM_OPTION:
        case FORM_PASSWD:
        case FORM_TEXT:
        case FORM_FILE:
        case FORM_TEXTAREA:
        case FORM_CHECK:
        case FORM_RADIO:
        case FORM_IMAGE:
        case FORM_HIDDEN:
        case FORM_RESET:
        case FORM_SUBMIT:
            v->type = TYP_STRING;
            v->content.string = lstring::copy(cbs->components[i].value);
            v->flags |= VF_ORIGINAL;
            break;
        }
        v->next = variables;
        variables = v;
    }

    EV()->InitCallback();
    CDs *cursd = CurCell();
    if (cursd) {

        // Preset "#define SUBMIT" when calling script.
        SImacroHandler *mh = new SImacroHandler;
        mh->parse_macro("SUBMIT", true);

        EditIf()->ulListCheck(script, cursd, false);
        SIparse()->presetVariables(variables);
        SI()->PresetMacros(mh);
        SI()->Interpret(sfp, wl, 0, 0);
        if (sfp)
            delete sfp;
        EditIf()->ulCommitChanges(true);
    }
    lock = false;
}

#endif // HAVE_MOZY



#ifdef NOTDEF

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

