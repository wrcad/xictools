
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#include "config.h"
#include "main.h"
#include "editif.h"
#include "scedif.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_macro.h"
#include "gtkmain.h"
#include "gtkfont.h"
#include "gtkhtext.h"
#include "gtkltab.h"
#include "gtkcoord.h"
#include "gtkparam.h"
#include "gtkmenu.h"
#include "gtkmenucfg.h"
#include "gtkutil.h"
#include "gtkinlines.h"
#include "nulldev.h"
#include "events.h"
#include "keymap.h"
#include "keymacro.h"
#include "tech.h"
#include "errorlog.h"
#include "ghost.h"
#include "timer.h"
#include "imsave/imsave.h"
#include "help/help_defs.h"
#include "help/help_context.h"
#include "htm/htm_widget.h"
#include "htm/htm_form.h"

#include "file_menu.h"
#include "view_menu.h"

#include <gdk/gdkkeysyms.h>
#ifdef WITH_X11
#include "gtkx11.h"
#include <X11/Xresource.h>
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef WIN32
#include <winsock2.h>
#include <gdk/gdkwin32.h>
#else
#include <netdb.h>
#include "../../lib/icons/xic_16x16.xpm"
#include "../../lib/icons/xic_32x32.xpm"
#include "../../lib/icons/xic_48x48.xpm"
#endif

#ifdef WIN32
// Reference a symbol in the resource module so the resources are linked.
extern int ResourceModuleId;
namespace { int dummy = ResourceModuleId; }
#endif


//-----------------------------------------------------------------------------
// Main Window
//
// Help system keywords used:
//  noopbutton
//  button2
//  button3
//  keyspresd
//  xic:itbtn
//  xic:ltbtn

// Create and export the graphics package.
namespace { GTKpkg _gtk_; }


// Wrappers visible in main app.

void
cMain::SetNoToTop(bool b)
{
#ifdef WITH_X11
    if (GRX)
        GRX->SetNoToTop(b);
#else
    (void)b;
#endif
}

void
cMain::SetLowerWinOffset(int offset)
{
    if (GRX)
        GRX->SetLowerWinOffset(offset);
}
// End of cMain functions


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

namespace {
    // Main and subwindow class for null graphics.
    //
    struct null_bag : public DSPwbag, public NULLwbag,  NULLwinApp,
        public NULLdraw
    {
    };


    // When the drawing window is highlighted and unhighlighted while
    // dragging, GTK sends an expose event to the main window.  This
    // will suppress the redraw, which is unneeded.
    GtkWidget *DontRedrawMe;

    // The main window and the prompt line are dnd receivers for file names
    //
    GtkTargetEntry main_targets[] = {
        { (char*)"TWOSTRING",  0, 0 },
        { (char*)"CELLNAME",   0, 1 },
        { (char*)"property",   0, 2 },
        { (char*)"STRING",     0, 3 },
        { (char*)"text/plain", 0, 4 },
    };
    guint n_main_targets = sizeof(main_targets) / sizeof(main_targets[0]);
}

#define GS_NBTNS 8

namespace {
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
                gdk_pointer_grab(w->window, true, GDK_BUTTON_RELEASE_MASK,
                    0, 0, be->time);
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

    unsigned int grabbed_key;
    grabstate_t grabstate;
    bool button_state[GS_NBTNS];
    sEventHdlr event_hdlr;

    namespace main_local {
        void wait_cursor(bool);
        int timer_cb(void*);
        void quit_help(void*);
        void form_submit_hdlr(void*);
    };

    inline bool is_modifier_key(unsigned keysym)
    {
        return ((keysym >= GDK_Shift_L && keysym <= GDK_Hyper_R) ||
            keysym == GDK_Mode_switch || keysym == GDK_Num_Lock);
    }

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
        if (gtkPkgIf()->busy_popup)
            gtkPkgIf()->busy_popup->popdown();
        return (false);
    }

    void
    pop_busy()
    {
        const char *busy_msg =
            "Working...\nPress Control-C in main window to abort.";

        if (!gtkPkgIf()->busy_popup && mainBag()) {
            gtkPkgIf()->busy_popup =
                mainBag()->PopUpErrText(busy_msg, STY_NORM);
            if (gtkPkgIf()->busy_popup)
                gtkPkgIf()->busy_popup->
                    register_usrptr((void**)&gtkPkgIf()->busy_popup);
            gtkPkgIf()->RegisterTimeoutProc(3000, busy_msg_timeout, 0);
        }
    }
}


//-----------------------------------------------------------------------------
// GTKpkg functions

// Create a new main_bag.  This will be passed to GTKdev::New() for
// registration, then on to Initialize().
//
GRwbag *
GTKpkg::NewGX()
{
    if (!MainDev())
        return (0);
    if (MainDev()->ident == _devNULL_)
        return (new null_bag);
    if (MainDev()->ident == _devGTK_)
        return (new main_bag);
    return (0);
}


// Initialization function for the main window.  The argument *must* be
// a main_bag.
//
int
GTKpkg::Initialize(GRwbag *wcp)
{
    if (!MainDev())
        return (true);
    if (MainDev()->ident == _devNULL_) {
        static GTKltab gtk_lt(true);
        static GTKedit gtk_hy(true);
        LT()->SetLtab(&gtk_lt);
        PL()->SetEdit(&gtk_hy);

        null_bag *w = dynamic_cast<null_bag*>(wcp);
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
    if (MainDev()->ident != _devGTK_)
        return (true);

    main_bag *w = dynamic_cast<main_bag*>(wcp);
    if (!w)
        return (true);

    GRX->RegisterMainFrame(w);
    GRpkgIf()->RegisterMainWbag(w);
    w->initialize();

    // Initialize the application's GUI.
    //
    int wid, hei;
    gdk_window_get_size(w->Window(), &wid, &hei);
    DSP()->Initialize(wid, hei, 0, (XM()->RunMode() != ModeNormal));
    LT()->InitLayerTable();

    // callback to tell application when quitting help
    HLP()->context()->registerQuitHelpProc(main_local::quit_help);
    HLP()->context()->registerFormSubmitProc(main_local::form_submit_hdlr);

    PL()->Init();
    GRX->RegisterBigWindow(w->Shell());

    // dispatch the events in queue (aesthetic reasons)
    GdkEvent *ev;
    while ((ev = gdk_event_get()) != 0) {
        if (ev->type == GDK_BUTTON_PRESS ||
                ev->type == GDK_2BUTTON_PRESS ||
                ev->type == GDK_3BUTTON_PRESS ||
                ev->type == GDK_BUTTON_RELEASE ||
                ev->type == GDK_KEY_PRESS ||
                ev->type == GDK_KEY_RELEASE) {
            gdk_event_free(ev);
            continue;
        }
        gtk_main_do_event(ev);
        gdk_event_free(ev);
    }

    Gst()->SetGhost(GFnone);
    if (!mainBag())
        // Halt called
        return (true);
    return (false);
}


// Reinitialize the application to shut down all graphics.
//
void
GTKpkg::ReinitNoGraphics()
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
        return;
    if (!mainBag())
        return;
    DSPmainDraw(Halt());
    DSP()->MainWdesc()->SetWbag(0);
    DSP()->MainWdesc()->SetWdraw(0);
    CloseGraphicsConnection();

    PL()->SetNoGraphics();
    LT()->SetNoGraphics();

    GRpkgIf()->SetNullGraphics();
    EV()->SetCurrentWin(DSP()->MainWdesc());
    null_bag *w = new null_bag;
    DSP()->MainWdesc()->SetWbag(w);
    DSP()->MainWdesc()->SetWdraw(w);
}


// Halt application cleanup function.
//
void
GTKpkg::Halt()
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
        return;
    GRX->RegisterBigWindow(0);
    EV()->InitCallback();
    gtk_main_quit();
    if (DSP()->MainWdesc()) {
        DSP()->MainWdesc()->SetWbag(0);
        DSP()->MainWdesc()->SetWdraw(0);
    }
    XM()->SetAppReady(false);
}


// The main loop, does not return, but might be reentered with longjmp.
//
void
GTKpkg::AppLoop()
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
        return;
    RegisterEventHandler(0, 0);
    in_main_loop = true;
    GRX->MainLoop(true);
}


namespace {
    struct evl
    {
        evl(GdkEvent *ev) { event = ev; next = 0; }
        ~evl() { gdk_event_free(event); }

        GdkEvent *event;
        evl *next;
    };
}


// Set the interrupt flag and return true if a ^C event is found.
//
bool
GTKpkg::CheckForInterrupt()
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
        return (false);
    static unsigned long lasttime;
    if (!in_main_loop)
        return (false);
    if (DSP()->Interrupt())
        return (true);

    // Cut expense if called too frequently.
    if (Timer()->elapsed_msec() == lasttime)
        return (false);
    lasttime = Timer()->elapsed_msec();

    if (dispatch_events) {
        GdkGC *gc = mainBag()->GC();
        if (gc == mainBag()->XorGC())
            // If ghost-drawing, defer event processing, which may
            // foul up the display.
            return (false);
        GdkGCValues gcv;
        gdk_gc_get_values(gc, &gcv);

        // Dispatch all events, this is the default operation.  Turn
        // off Peek mode in case we have to do some drawing.
        bool b = DSP()->SlowMode();
        DSP()->SetSlowMode(false);
        gtk_DoEvents(1000);
        DSP()->SetSlowMode(b);

        // Reset the GC, these may have changed during event processing.
        gdk_gc_set_foreground(gc, &gcv.foreground);
        gdk_gc_set_fill(gc, gcv.fill);
        gdk_gc_set_line_attributes(gc, gcv.line_width, gcv.line_style,
        gcv.cap_style, gcv.join_style);

        return (DSP()->Interrupt());
    }

    // Don't process any events, but look through the queue for
    // Ctrl-C presses.  Remove these and set the interupt flags,
    // but retain other events.

    bool intr = false;
    evl *events = 0, *eend = 0;
    GdkEvent *ev;
    while ((ev = gdk_event_get()) != 0) {
        if (ev->type == GDK_KEY_PRESS) {
            if ((ev->key.keyval == GDK_c || ev->key.keyval == GDK_C) &&
                    (ev->key.state & GDK_CONTROL_MASK)) {

                intr = true;
                gdk_event_free(ev);
                continue;
            }
        }
        else if (ev->type == GDK_KEY_RELEASE) {
            if ((ev->key.keyval == GDK_c || ev->key.keyval == GDK_C) &&
                    (ev->key.state & GDK_CONTROL_MASK)) {

                gdk_event_free(ev);
                continue;
            }
        }
        if (eend) {
            eend->next = new evl(ev);
            eend = eend->next;
        }
        else
            eend = events = new evl(ev);
    }
    while (events) {
        eend = events->next;
        gdk_event_put(events->event);
        delete events;
        events = eend;
    }
    if (intr)
        cMain::InterruptHandler();
    return (intr);
}


// If state is true, iconify the application.  Return false if the
// application has not been realized.  Note that the map_hdlr()
// function takes care of the popups.
//
int
GTKpkg::Iconify(int state)
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
        return (false);
    main_bag *w = mainBag();
    if (!w)
        return (false);
    if (!state)
        gtk_widget_show(w->Shell());
    else 
        gtk_window_iconify(GTK_WINDOW(w->Shell()));
    return (true);
}


// Initialize popup subwindow number wnum (1 - DSP_NUMWINS-1).
//
bool
GTKpkg::SubwinInit(int wnum)
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
        return (false);
    if (!mainBag())
        return (false);
    if (wnum < 1 || wnum >= DSP_NUMWINS)
        return (false);
    win_bag *w = new win_bag;
    w->subw_initialize(wnum);
    return (true);
}


void
GTKpkg::SubwinDestroy(int wnum)
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
        return;
    if (wnum < 1 || wnum >= DSP_NUMWINS)
        return;
    WindowDesc *wdesc = DSP()->Window(wnum);
    if (!wdesc)
        return;
    win_bag *w = dynamic_cast<win_bag*>(wdesc->Wbag());
    if (w) {
        wdesc->SetWbag(0);
        wdesc->SetWdraw(0);
        w->pre_destroy(wnum);
        delete w;
    }
    Menu()->DestroySubwinMenu(wnum);
}


namespace {
    int
    saved_events_idle(void*)
    {
        event_hdlr.do_saved_events();
        return (0);
    }
}


// Increment/decrement the busy flag and switch to watch cursor.  If
// already busy when passing true, return false.
//
bool
GTKpkg::SetWorking(bool busy)
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
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
            if (event_hdlr.has_saved_events())
                // dispatch saved events
                RegisterIdleProc(saved_events_idle, 0);
            return (ready);
        }
        if (app_busy)
            app_busy--;
    }
    return (ready);
}


void
GTKpkg::SetOverrideBusy(bool ovr)
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
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
GTKpkg::GetMainWinIdentifier(char *buf)
{
    *buf = 0;
    if (!MainDev() || MainDev()->ident != _devGTK_)
        return (false);
#ifdef WITH_X11
    if (mainBag() && mainBag()->Shell() && mainBag()->Shell()->window) {
        sprintf(buf, "%ld", (long)gr_x_window(mainBag()->Shell()->window));
        return (true);
    }
#endif
    return (false);
}


bool
GTKpkg::IsDualPlane()
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
        return (false);
    return (GRX->IsDualPlane());
}


bool
GTKpkg::IsTrueColor()
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
        return (false);
    return (GRX->IsTrueColor());
}


bool
GTKpkg::UsingX11()
{
#ifdef WITH_X11
    return (true);
#else
    return (false);
#endif
}


void
GTKpkg::CloseGraphicsConnection()
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
        return;
    if (GRX->ConnectFd() > 0) {
        gtk_main_quit();
        close(GRX->ConnectFd());
    }
}


// Return the name of the current X display.
//
const char *
GTKpkg::GetDisplayString()
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
        return (0);

#ifdef WITH_X11
    if (gr_x_display())
        return (DisplayString(gr_x_display()));
#endif
    return (0);
}


#ifdef WITH_X11
namespace {
    // Return true if addr matches an address in hent.
    //
    bool match_addr(char *addr, int len, hostent *hent)
    {
        if (len == hent->h_length) {
            for (int j = 0; hent->h_addr_list[j]; j++) {
                if (!memcmp(addr, hent->h_addr_list[j], len))
                    return (true);
            }
        }
        return (false);
    }
}
#endif


// Check if host which has data in hent and name in host has screen
// access to the local display in display_string.  Return false if
// access will surely fail, true otherwise (but it may still fail).
//
bool
GTKpkg::CheckScreenAccess(hostent *hent, const char *host,
    const char *display_string)
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
         return (false);
    if (!hent)
         return (true);

#ifdef WITH_X11
    bool foundit = false;
    int nhosts;
    int state;
    XHostAddress *host_list = XListHosts(gr_x_display(), &nhosts, &state);
    if (!state) {
        // no access control
        if (host_list)
            XFree((char*)host_list);
        host_list = 0;
        foundit = true;
    }
    else {
        if (host_list) {
            for (int i = 0; i < nhosts; i++) {
                if (match_addr(host_list[i].address, host_list[i].length,
                        hent)) {
                    foundit = true;
                    break;
                }
            }
            XFree((char*)host_list);
            host_list = 0;
        }
        if (!foundit && display_string) {
            // Didn't find the host in the access list.  If the local
            // and remote hosts are the same, or if the display name
            // looks like an ssh tunnel (that we can't check) set
            // foundit true.

            char buf[128];
            strncpy(buf, display_string, 128);
            char *c = strchr(buf, ':');
            if (c) {
                *c = 0;
                if (!strcmp(buf, "localhost") && atoi(c+1) >= 10) {
                    // The display is in the form localhost:1x.0,
                    // which looks like an ssh tunnel.  We can't
                    // tell if this is ok or not.  Allow it.
                    foundit = true;
                }
                else {
                    if (!buf[0])
                        strcpy(buf, "localhost");
                    int len = hent->h_length;
                    char *remote_addr = new char[len];
                    memcpy(remote_addr, hent->h_addr_list[0], len);
                    hent = gethostbyname(buf);
                    if (hent && match_addr(remote_addr, len, hent))
                        foundit = true;
                    // Reset the gethostname static data, otherwise the
                    // caller's hent may be screwy.
                    gethostbyname(host);
                    delete [] remote_addr;
                }
            }
        }
    }
    return (foundit);

#else
    (void)host;
    (void)display_string;
    return (false);
#endif
}


int
GTKpkg::RegisterIdleProc(int(*proc)(void*), void *arg)
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
         return (0);
    return (gtk_idle_add(proc, arg));
}


bool
GTKpkg::RemoveIdleProc(int id)
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
         return (false);
    gtk_idle_remove(id);
    return (true);
}


int
GTKpkg::RegisterTimeoutProc(int ms, int(*proc)(void*), void *arg)
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
         return (0);
    return (GRX->AddTimer(ms, proc, arg));
}


bool
GTKpkg::RemoveTimeoutProc(int id)
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
         return (false);
    GRX->RemoveTimer(id);
    return (true);
}


// int time;  milliseconds
//
int
GTKpkg::StartTimer(int time, bool *set)
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
         return (0);
    int id = 0;
    if (set)
        *set = false;
    if (time > 0)
        id = gtk_timeout_add(time, main_local::timer_cb, set);
    return (id);
}


// Set the application fonts.  This should work for either Pango
// or XFD font names.
//
void
GTKpkg::SetFont(const char *fname, int fnum, FNT_FMT fmt)
{
    if (!MainDev() || MainDev()->ident != _devGTK_)
         return;

    // GTK-2, ignore XFDs, but anything else should be ok.
    if (fmt == FNT_FMT_X)
        return;

    if (!xfd_t::is_xfd(fname))
        FC.setName(fname, fnum);
}


// Return the application font names.
//
const char *
GTKpkg::GetFont(int fnum)
{
    // valid for null graphics also
    return (FC.getName(fnum));
}


// Return the format code for the description string returned by GetFont.
//
FNT_FMT
GTKpkg::GetFontFmt()
{
    return (FNT_FMT_P);
}


// Register an event handler, which is called for each event received
// by the application.
//
void
GTKpkg::RegisterEventHandler(void(*handler)(GdkEvent*, void*), void *arg)
{
    if (handler)
        gdk_event_handler_set(handler, arg, 0);
    else
        gdk_event_handler_set(event_hdlr.main_event_handler, 0, 0);
}
// End of GTKpkg functions


//-----------------------------------------------------------------------------
// win_bag functions

bool win_bag::HaveDrag = false;

win_bag::win_bag()
{
    wib_windesc = 0;
    wib_gridpop = 0;
    wib_expandpop = 0;
    wib_zoompop = 0;
    wib_menulabel = 0;

    wib_keyspressed = 0;
    wib_keypos = 0;
    memset(wib_keys, 0, sizeof(wib_keys));
    wib_resized = false;

    wib_window_bak = 0;
    wib_draw_pixmap = 0;
    wib_px_width = 0;
    wib_px_height = 0;

    wib_id = 0;
    wib_state = 0;
    wib_x = wib_y = 0;
}


win_bag::~win_bag()
{
    PopUpExpand(0, MODE_OFF, 0, 0, 0, false);
    PopUpGrid(0, MODE_OFF);
    PopUpZoom(0, MODE_OFF);
    if (wib_draw_pixmap)
        gdk_pixmap_unref(wib_draw_pixmap);
}


// Keep track of previous locations.
namespace { GdkRectangle LastPos[DSP_NUMWINS]; }

// Function to initialize a subwindow.
//
void
win_bag::subw_initialize(int wnum)
{

    char buf[32];
    sprintf(buf, "%s %d", XM()->Product(), wnum);
    wb_shell = gtk_NewPopup(mainBag(), buf, subwin_cancel_proc,
        (void*)(long)wnum);

    wib_windesc = DSP()->Window(wnum);
    DSP()->Window(wnum)->SetWbag(this);
    DSP()->Window(wnum)->SetWdraw(this);

    if (LastPos[wnum].width) {
        // The saved value is relative to the main window, so will be
        // multi-monitor safe.

        int x, y;
        gdk_window_get_origin(mainBag()->Shell()->window, &x, &y);
        gtk_widget_set_uposition(wb_shell,
            LastPos[wnum].x + x, LastPos[wnum].y + y);
        gtk_window_set_default_size(GTK_WINDOW(wb_shell),
            LastPos[wnum].width, LastPos[wnum].height);
    }
    else {
        GdkRectangle rect;
        ShellGeometry(mainBag()->Shell(), &rect, 0);
        rect.x += rect.width - 560;
        rect.y += (wnum-1)*40 + 60; // make room for device toolbar
        gtk_widget_set_uposition(wb_shell, rect.x, rect.y);
    }

    GtkWidget *menu = GTK_WIDGET(Menu()->CreateSubwinMenu(wnum));
    gtk_widget_show(menu);

    // A dummy menubar item displaying a label.
    wib_menulabel = gtk_menu_item_new_with_label("");
    gtk_widget_show(wib_menulabel);
    gtk_menu_shell_insert(GTK_MENU_SHELL(menu), wib_menulabel, 2);

    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(wb_shell), form);

    gd_viewport = gtk_drawing_area_new();
    gtk_widget_set_name(gd_viewport, "Viewport");
    gtk_drawing_area_size(GTK_DRAWING_AREA(gd_viewport), 500, 400);
    gtk_widget_show(gd_viewport);

    gtk_widget_add_events(gd_viewport, GDK_STRUCTURE_MASK);
    gtk_widget_add_events(gd_viewport, GDK_EXPOSURE_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "expose-event",
        GTK_SIGNAL_FUNC(redraw_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_KEY_PRESS_MASK);
    gtk_signal_connect_after(GTK_OBJECT(gd_viewport), "key-press-event",
        GTK_SIGNAL_FUNC(key_dn_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_KEY_RELEASE_MASK);
    gtk_signal_connect_after(GTK_OBJECT(gd_viewport), "key-release-event",
        GTK_SIGNAL_FUNC(key_up_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect_after(GTK_OBJECT(gd_viewport), "button-press-event",
        GTK_SIGNAL_FUNC(button_dn_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_BUTTON_RELEASE_MASK);
    gtk_signal_connect_after(GTK_OBJECT(gd_viewport), "button-release-event",
        GTK_SIGNAL_FUNC(button_up_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_POINTER_MOTION_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "motion-notify-event",
        GTK_SIGNAL_FUNC(motion_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_ENTER_NOTIFY_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "enter-notify-event",
        GTK_SIGNAL_FUNC(enter_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_LEAVE_NOTIFY_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "leave-notify-event",
        GTK_SIGNAL_FUNC(leave_hdlr), this);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "scroll-event",
        GTK_SIGNAL_FUNC(scroll_hdlr), this);

    // For gtk2 - this avoids issue of an expose event on focus change.
    //
    gtk_widget_add_events(gd_viewport, GDK_FOCUS_CHANGE_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "focus-in-event",
        GTK_SIGNAL_FUNC(focus_hdlr), this);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "focus-out-event",
        GTK_SIGNAL_FUNC(focus_hdlr), this);

    // Drop handler setup.
    //
    GtkDestDefaults DD = (GtkDestDefaults)
        (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP);
    gtk_drag_dest_set(gd_viewport, DD, main_targets, n_main_targets,
        GDK_ACTION_COPY);
    gtk_signal_connect_after(GTK_OBJECT(gd_viewport), "drag-data-received",
        GTK_SIGNAL_FUNC(drag_data_received), 0);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "drag-leave",
        GTK_SIGNAL_FUNC(target_drag_leave), 0);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "drag-motion",
        GTK_SIGNAL_FUNC(target_drag_motion), 0);

    GTKfont::setupFont(gd_viewport, FNT_SCREEN, false);

    wib_keyspressed = gtk_label_new("");
    gtk_widget_show(wib_keyspressed);
    gtk_label_set_justify(GTK_LABEL(wib_keyspressed), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(wib_keyspressed), 0, 0.5);
    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), wib_keyspressed);
    GtkWidget *ebox = gtk_event_box_new();
    gtk_widget_show(ebox);
    gtk_container_add(GTK_CONTAINER(ebox), frame);

    // Set up keyspressed font and tracking.
    GTKfont::setupFont(wib_keyspressed, FNT_FIXED, true);

    int lwid = GTKfont::stringWidth(wib_keyspressed, "INPUT");
    GtkAllocation a;
    a.height = 0;
    a.width = lwid;
    a.x = 0;
    a.y = 0;
    gtk_widget_size_allocate(wib_keyspressed, &a);

    gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(ebox), "button-press-event",
        GTK_SIGNAL_FUNC(keys_hdlr), this);

    // Arrange and pack the widgets.
    //
    GtkWidget *hbox = gtk_hbox_new(false, 2);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(hbox), menu, true, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox), ebox, false, false, 0);
    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 0, 0);

    frame = gtk_frame_new(0);
    gtk_container_add(GTK_CONTAINER(frame), gd_viewport);
    gtk_widget_show(frame);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 0, 0);

    gtk_window_set_transient_for(GTK_WINDOW(wb_shell),
        GTK_WINDOW(mainBag()->Shell()));
    gtk_widget_show(wb_shell);
    gd_window = gd_viewport->window;

    gd_backg = GRX->NameColor("black");
    gd_foreg = GRX->NameColor("white");
    SetWindowBackground(gd_backg);
    Clear();

    // Application initialization callback.
    //
    int wid, hei;
    gdk_window_get_size(gd_window, &wid, &hei);
    DSP()->Initialize(wid, hei, wnum);

    // Have to set this after initialization.
    //
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "configure-event",
        GTK_SIGNAL_FUNC(resize_hdlr), this);

    XM()->UpdateCursor(wib_windesc, (CursorType)Gbag()->get_cursor_type(),
        true);
}


void
win_bag::pre_destroy(int wnum)
{
    GdkRectangle rect, rect_d;
    ShellGeometry(wb_shell, &rect, &rect_d);

    // Save relative to corner of main window.  Absolute coords can
    // change if the main window is moved to a different monitor in
    // multi-monitor setups.

    int x, y;
    gdk_window_get_origin(mainBag()->Shell()->window, &x, &y);
    LastPos[wnum].x = rect_d.x - x;
    LastPos[wnum].y = rect_d.y - y;
    LastPos[wnum].width = rect.width;
    LastPos[wnum].height = rect.height;

    gtk_signal_disconnect_by_func(GTK_OBJECT(wb_shell),
        GTK_SIGNAL_FUNC(subwin_cancel_proc), (void*)(long)wnum);
}


// The following two functions implement pixmap buffering for screen
// redisplays.  This often leads to faster redraws.  This function
// will create a pixmap and make it the current context drawable.
//
void
win_bag::SwitchToPixmap()
{
    if (!wib_windesc)
        return;

    if (wib_window_bak) {
        gd_window = wib_window_bak;
        wib_window_bak = 0;
    }

    int vp_width = wib_windesc->ViewportWidth();
    int vp_height = wib_windesc->ViewportHeight();

    if (!wib_draw_pixmap ||
            vp_width != wib_px_width || vp_height != wib_px_height) {
        GdkPixmap *pm = wib_draw_pixmap;
        if (pm)
            gdk_pixmap_unref(pm);
        wib_px_width = vp_width;
        wib_px_height = vp_height;
        pm = gdk_pixmap_new(gd_window, wib_px_width, wib_px_height,
            GRX->Visual()->depth);
        if (pm)
            wib_draw_pixmap = pm;
        else {
            wib_px_width = 0;
            wib_px_height = 0;
            wib_draw_pixmap = 0;
            return;
        }
    }

    // swap in the pixmap
    wib_window_bak = gd_window;
    gd_window = wib_draw_pixmap;
}


// Copy out the BB area of the pixmap to the screen, and switch the
// context drawable back to the screen.
//
void
win_bag::SwitchFromPixmap(const BBox *BB)
{
    // Note that the bounding values are included in the display.
    if (wib_window_bak) {
        gdk_window_copy_area(wib_window_bak, CpyGC(),
            BB->left, BB->top, gd_window, BB->left, BB->top,
            BB->width() + 1, abs(BB->height()) + 1);
        gd_window = wib_window_bak;
        wib_window_bak = 0;
    }
}


// Return the pixmap, set by a previous call to SwitchToPixmap(), and
// reset the context to normal.
//
GRobject
win_bag::DrawableReset()
{
    if (wib_window_bak) {
        gd_window = wib_window_bak;
        wib_window_bak = 0;
    }
    return (wib_draw_pixmap);
}


// Copy out the BB area of the pixmap to the screen
//
void
win_bag::CopyPixmap(const BBox *BB)
{
    // Note that the bounding values are included in the display.
    if (wib_window_bak) {
        gdk_window_copy_area(wib_window_bak, CpyGC(),
            BB->left, BB->top, gd_window, BB->left, BB->top,
            BB->width() + 1, abs(BB->height()) + 1);
    }
}


// Destroy the pixmap (switching out first if necessary).
//
void
win_bag::DestroyPixmap()
{
    if (wib_window_bak) {
        gd_window = wib_window_bak;
        wib_window_bak = 0;
    }
    if (wib_draw_pixmap)
        gdk_pixmap_unref(wib_draw_pixmap);
    wib_draw_pixmap = 0;
    wib_px_width = 0;
    wib_px_height = 0;
}


// Return true if the pixmap is alive and well.
//
bool
win_bag::PixmapOk()
{
    // Note that the bounding values are included in the display.
    if (!wib_windesc)
        return (false);
    int vp_width = wib_windesc->ViewportWidth();
    int vp_height = wib_windesc->ViewportHeight();
    return (wib_draw_pixmap && wib_px_width == vp_width &&
        wib_px_height == vp_height);
}


// Function to dump a window to a file, not part of the hardcopy system.
//
bool
win_bag::DumpWindow(const char *filename, const BBox *AOI = 0)
{
    // Note that the bounding values are included in the display.
    if (!wib_windesc)
        return (false);
    BBox BB = AOI ? *AOI : wib_windesc->Viewport();
    ViewportClip(BB, wib_windesc->Viewport());
    if (BB.right < BB.left || BB.bottom < BB.top)
        return (false);

    GdkPixmap *pm = 0;
    bool native = false;
    int vp_width = wib_windesc->ViewportWidth();
    int vp_height = wib_windesc->ViewportHeight();
    if (wib_draw_pixmap && wib_px_width == vp_width &&
            wib_px_height == vp_height) {
        pm = wib_draw_pixmap;
        native = true;
    }
    else {
        pm = gdk_pixmap_new(gd_window, vp_width, vp_height,
            GRX->Visual()->depth);
        if (!pm)
            return (false);
        GdkWindow *tmpw = gd_window;
        gd_window = pm;
        wib_windesc->RedisplayDirect();
        gd_window = tmpw;
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
    Image *im = create_image_from_drawable(gr_x_display(),
        GDK_WINDOW_XWINDOW(pm),
        BB.left, BB.top, BB.width() + 1, abs(BB.height()) + 1);
#else
#ifdef WITH_QUARTZ
//XXX Need equiv. code for Quartz.
    Image *im = 0;
#endif
#endif
#endif
    if (!native)
        gdk_pixmap_unref(pm);
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
}


// Get the "keyspressed" in buf.
//
void
win_bag::GetTextBuf(char *buf)
{
    for (int i = 0; i < wib_keypos; i++)
        buf[i] = wib_keys[i];
    buf[wib_keypos] = 0;
}


// Set the "keyspressed".
//
void
win_bag::SetTextBuf(const char *buf)
{
    SetKeys(buf);
    ShowKeys();
}


void
win_bag::ShowKeys()
{
    // Set the colors of the keys-pressed area in the main window. 
    // This is slightly tricky.  The keys-pressed label is parented by
    // an event box, which has an x-window to we can set the
    // background.  The label itself sets the foreground.

    /**********
     * NO, let's leave the keys pressed area alone, to take default
     * widget colors.  This gives some contrast from the status line,
     * if the colors are different.
     * This code is suspect in gtk-2, but seems to work.

    if (wib_windesc == DSP()->MainWdesc()) {
        static GtkStyle local_style;

        // The state will always be "normal".
        int state = wib_keyspressed->state;
        if (!local_style.fg_gc[state]) {

            // Restyle the label.
            GtkStyle *style = gtk_style_copy(wib_keyspressed->style);
            gtk_widget_set_style(wib_keyspressed, style);

            // Restyle the parent event box.
            style = gtk_style_copy(wib_keyspressed->parent->style);
            gtk_widget_set_style(wib_keyspressed->parent, style);

            // Replace the foreground gc in the label.
            GdkGC *gc = gdk_gc_new(wib_keyspressed->window);
            local_style.fg_gc[state] = gc;
            gdk_gc_copy(gc, wib_keyspressed->style->fg_gc[state]);
            wib_keyspressed->style->fg_gc[state] = gc;

            // Replace the background gc in the event box.
            gc = gdk_gc_new(wib_keyspressed->parent->window);
            local_style.bg_gc[state] = gc;
            gdk_gc_copy(gc, wib_keyspressed->parent->style->bg_gc[state]);
            wib_keyspressed->parent->style->bg_gc[state] = gc;
        }

        // Reset the drawing colors.
        GdkColor clr;
        clr.pixel = DSP()->Color(PromptEditTextColor);
        gdk_gc_set_foreground(local_style.fg_gc[state], &clr);
        clr.pixel = DSP()->Color(PromptBackgroundColor);
        gdk_gc_set_foreground(local_style.bg_gc[state], &clr);
    }

    **********/

    // This triggers screen refresh,
    gtk_widget_queue_resize(wib_keyspressed);

    char *s = wib_keys + (wib_keypos > 5 ? wib_keypos - 5 : 0);
    gtk_label_set_text(GTK_LABEL(wib_keyspressed), s);
}


void
win_bag::SetKeys(const char *text)
{
    while (wib_keypos)
        wib_keys[--wib_keypos] = '\0';
    if (text) {
        strncpy(wib_keys, text, CBUFMAX);
        wib_keypos = strlen(wib_keys);
    }
}


void
win_bag::BspKeys()
{
    if (wib_keypos)
        wib_keys[--wib_keypos] = '\0';
}


bool
win_bag::AddKey(int c)
{
    if (c > ' ' && c <= '~' && wib_keypos < CBUFMAX) {
        wib_keys[wib_keypos++] = c;
        return (true);
    }
    if (c == ' ' && wib_keypos > 0 && wib_keypos < CBUFMAX) {
        wib_keys[wib_keypos++] = c;
        return (true);
    }
    return (false);
}


bool
win_bag::CheckBsp()
{
    if (wib_keypos > 0 && wib_keys[wib_keypos - 1] == Kmap()->SuppressChar()) {
        BspKeys();
        return (true);
    }
    return (false);
}


void
win_bag::CheckExec(bool exact)
{
    if (!wib_keypos)
        return;
    int wnum;
    for (wnum = 1; wnum < DSP_NUMWINS; wnum++)
        if (wib_windesc == DSP()->Window(wnum))
            break;
    if (wnum == DSP_NUMWINS)
        wnum = -1;
    MenuEnt *ent = Menu()->MatchEntry(wib_keys, wib_keypos, wnum, exact);
    if (ent) {
        if (ent->is_dynamic()) {
            // Ignore the submenu buttons in the User menu
            const char *it = ent->item;
            if (it && !strcmp(it, "<Branch>"))
                return;
        }
        if (ent->cmd.caller) {
            // simulate a button press
            char buf[CBUFMAX + 1];
            strncpy(buf, ent->entry, CBUFMAX);
            buf[CBUFMAX] = 0;
            int n = strlen(buf) - 5;
            if (n < 0)
                n = 0;
            gtk_label_set_text(GTK_LABEL(wib_keyspressed), buf + n);

            if (ent->alt_caller)
                gtkMenu()->CallCallback(GTK_WIDGET(ent->alt_caller));
            else
                gtkMenu()->CallCallback(GTK_WIDGET(ent->cmd.caller));
        }
        SetKeys(0);
    }
}


char *
win_bag::KeyBuf()
{
    return (wib_keys);
}


int
win_bag::KeyPos()
{
    return (wib_keypos);
}


void
win_bag::SetLabelText(const char *text)
{
    if (wib_menulabel) {
#if GTK_CHECK_VERSION(2,16,0)
        gtk_menu_item_set_label(GTK_MENU_ITEM(wib_menulabel), text);
#else
        GtkWidget *la = gtk_bin_get_child(GTK_BIN(wib_menulabel));
        if (la)
            gtk_label_set_text(GTK_LABEL(la), text);
#endif
    }
}
// End of cAppWinFuncs interface.


#define MODMASK (GR_SHIFT_MASK | GR_CONTROL_MASK | GR_ALT_MASK)

// Handle keypresses in the main and subwindows.  If certain modes
// are set, call the application.  Otherwise, save the keypress in
// a buffer, which is displayed in the keypress readout area.  If
// this text uniquely matches a command, send a button press to
// the appropriate command button, and clear the buffer.
//
bool
win_bag::keypress_handler(unsigned keyval, unsigned state, char *keystring,
    bool from_prline, bool up)
{
    if (!wib_windesc)
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
            EV()->KeypressCallback(wib_windesc, code, (char*)"", state);
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
        char *fstr = Tech()->GetFkey(subcode, state);

        if (fstr) {
            if (*fstr == '!') {
                XM()->TextCmd(fstr + 1, false);
                delete [] fstr;
                return (true);
            }
            strncpy(tbuf, fstr, CBUFMAX);
        }
        delete [] fstr;
        if (EV()->KeypressCallback(wib_windesc, code, tbuf, state))
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
            if (EV()->KeyActions(wib_windesc, k->action, &code))
                return (true);
            break;
        }
    }

    char tbuf[CBUFMAX + 1];
    memset(tbuf, 0, sizeof(tbuf));
    if (keystring)
        strncpy(tbuf, keystring, CBUFMAX);
    if (EV()->KeypressCallback(wib_windesc, code, tbuf, state))
        return (true);

    if (state & GR_ALT_MASK)
        return (false);

    for (keyaction *k = Kmap()->ActionMapPost(); k->code; k++) {
        if (k->code == code &&
                (!k->state || k->state == (state & MODMASK))) {
            if (EV()->KeyActions(wib_windesc, k->action, &code))
                return (true);
            break;
        }
    }

    if (code == RETURN_KEY) {
        if (wib_keypos > 0) {
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


//
// Static signal handlers
//

// Static function.
int
win_bag::map_hdlr(GtkWidget*, GdkEvent*, void*)
{
    // Nothing to do but handle this to avoid default action of
    // issuing expose event.
    return (true);
}


// Static function.
// Resize after the main or subwindow size change.  Call the application
// with the new size.
//
int
win_bag::resize_hdlr(GtkWidget *caller, GdkEvent *event, void *client_data)
{
    win_bag *w = static_cast<win_bag*>(client_data);
    if (event->type == GDK_CONFIGURE && w->wib_windesc) {
        int width, height;
        gdk_window_get_size(caller->window, &width, &height);
        EV()->ResizeCallback(w->wib_windesc, width, height);
        w->wib_resized = true;  // used in redraw_hdlr
    }
    return (true);
}


// Static function.
// Redraw the main or subwindow after an expose.  Break it up into
// exposed rectangles to save time.  Call the application to
// actually redraw the viewport.
//
int
win_bag::redraw_hdlr(GtkWidget *widget, GdkEvent *event, void *client_data)
{
    if (widget->parent == DontRedrawMe) {
        DontRedrawMe = 0;
        return (false);
    }
    if (HaveDrag && DSP()->NoPixmapStore())
        return (false);

    win_bag *w = static_cast<win_bag*>(client_data);

    GdkEventExpose *pev = (GdkEventExpose*)event;
    if (DSP()->NoPixmapStore() &&
            (!DSP()->CurCellName() || !XM()->IsAppReady())) {
        *w->wib_windesc->ClipRect() = w->wib_windesc->Viewport();
        w->wib_windesc->ShowGrid();
        XM()->SetAppReady(true);
    }
    else {
        XM()->SetAppReady(true);
        GdkRectangle *rects;
        int nrects;
        gdk_region_get_rectangles(pev->region, &rects, &nrects);
        for (int i = 0; i < nrects; i++) {
            GdkRectangle *r = rects + i;
#ifdef WITH_QUARTZ
            // The Update method updates a single pixel at 0,0, which
            // triggers an expose event which is needed internally to
            // show the rendering.  Here we filter and ignore this
            // event.
            //
            // I have no clue why, but from viewports only, the
            // 1-pixel area is followed by an apparently spurious
            // region 2 pixels high along the entire bottom of the
            // window.  We'll just break to avoid this.

            if (r->x == 0 && r->y == 0 && r->width == 1 && r->height == 1)
                break;
#endif
            BBox BB(r->x, r->y + r->height, r->x + r->width, r->y);
#if GTK_CHECK_VERSION(2,20,0)
            // RHEL6 and later
            w->wib_windesc->Update(&BB, false);
#else
            // This did strange and not good things in OS X 10.9, the
            // test restricts this to RHEL5 where it is supposedly
            // needed.  It seems to have no effect in RHEL6.

            w->wib_windesc->Update(&BB, w->wib_resized);
#endif
        }
        g_free(rects);
    }
    // wib_resized is set in the resize handler, If set above, run the
    // redisplay queue immediately. This prevents some rather bad
    // flickering in earlier GTK-2 releases (from RHEL 5).

    w->wib_resized = false;
    return (true);
}


namespace {
    // Return true if the mouse pointer is currently over the prompt
    // line area in the main window.
    //
    bool pointer_in_prompt_area()
    {
        int x, y;
        GRX->PointerRootLoc(&x, &y);
        GtkWidget *w = mainBag()->TextArea();
        GdkRectangle r;
        ShellGeometry(w, &r , 0);
        return (x >= r.x && x <= r.x + r.width &&
            y >= r.y && y <= r.y + r.height);
    }
}


// Static function.
// Key press processing for the drawing windows
//
int
win_bag::key_dn_hdlr(GtkWidget *caller, GdkEvent *event, void *client_data)
{
    if (!mainBag() || gtkPkgIf()->NotMapped())
        return (true);
    win_bag *w = static_cast<win_bag*>(client_data);
    if (w->wb_message)
        w->wb_message->popdown();

    GdkEventKey *kev = (GdkEventKey*)event;
    if (!is_modifier_key(kev->keyval) && w->CheckBsp())
        ;
    else if (KbMac()->MacroExpand(kev->keyval, kev->state, false))
        return (true);

    if (kev->keyval == GDK_Shift_L || kev->keyval == GDK_Shift_R ||
            kev->keyval == GDK_Control_L || kev->keyval == GDK_Control_R) {
        gdk_keyboard_grab(caller->window, true, kev->time);
        grabbed_key = kev->keyval;
    }

    return (w->keypress_handler(kev->keyval, kev->state, kev->string,
        pointer_in_prompt_area(), false));
}


// Static function.
// Key release processing for the drawing windows
//
int
win_bag::key_up_hdlr(GtkWidget*, GdkEvent *event, void *client_data)
{
    if (grabbed_key && grabbed_key == event->key.keyval) {
        grabbed_key = 0;
        gdk_keyboard_ungrab(event->key.time);
    }
    if (!mainBag() || gtkPkgIf()->NotMapped())
        return (true);
    GdkEventKey *kev = (GdkEventKey*)event;
    win_bag *w = static_cast<win_bag*>(client_data);
    if (KbMac()->MacroExpand(kev->keyval, kev->state, true))
        return (true);
    return (w->keypress_handler(kev->keyval, kev->state, kev->string, false,
        true));
}


// Static function.
// Dispatch button press events to the application callbacks.
//
int
win_bag::button_dn_hdlr(GtkWidget *caller, GdkEvent *event, void *client_data)
{
    GdkEventButton *bev = (GdkEventButton*)event;
    int button = Kmap()->ButtonMap(bev->button);
    if (!GTK_WIDGET_SENSITIVE(gtkMenu()->MainMenu()) && button == 1)
        // menu is insensitive, so ignore press
        return (true);
    if (event->type == GDK_2BUTTON_PRESS ||
            event->type == GDK_3BUTTON_PRESS)
        return (true);
    if (bev->button < GS_NBTNS)
        button_state[bev->button] = true;

    win_bag *w = static_cast<win_bag*>(client_data);
    if (w->wb_message)
        w->wb_message->popdown();

    bool showing_ghost = w->Gbag()->showing_ghost();
    if (showing_ghost)
        w->ShowGhost(ERASE);
    if (!XM()->IsDoingHelp() || EV()->IsCallback())
        grabstate.set(caller, bev);

        // Override the "automatic grab" so that the cursor tracks
        // the subwindow crossings.
        //
        // Subtle point:  the callbacks can call their own event
        // loops, so we have to set the grab state before the
        // callbacks are called.

    switch (button) {
    case 1:
        // basic point operation
        if ((bev->state & GDK_SHIFT_MASK) &&
                (bev->state & GDK_CONTROL_MASK) &&
                (bev->state & GDK_MOD1_MASK)) {
            // Control-Shift-Alt-Button1 simulates a "no-op" button press.
            grabstate.check_noop(true);
            if (XM()->IsDoingHelp())
                DSPmainWbag(PopUpHelp("noopbutton"))
            else if (w->wib_windesc)
                EV()->ButtonNopCallback(w->wib_windesc,
                    (int)bev->x, (int)bev->y, bev->state);
        }
        else if (w->wib_windesc)
            EV()->Button1Callback(w->wib_windesc, (int)bev->x, (int)bev->y,
                bev->state);
        break;
    case 2:
        // Pan operation
        if (XM()->IsDoingHelp() && !is_shift_down())
            DSPmainWbag(PopUpHelp("button2"))
        else if (w->wib_windesc)
            EV()->Button2Callback(w->wib_windesc, (int)bev->x, (int)bev->y,
                bev->state);
        break;
    case 3:
        // Zoom opertion
        if (XM()->IsDoingHelp() && !is_shift_down())
            DSPmainWbag(PopUpHelp("button3"))
        else if (w->wib_windesc)
            EV()->Button3Callback(w->wib_windesc, (int)bev->x, (int)bev->y,
                bev->state);
        break;

    default:
        // No-op, update coordinates only.
        if (XM()->IsDoingHelp() && !is_shift_down())
            DSPmainWbag(PopUpHelp("noopbutton"))
        else if (w->wib_windesc)
            EV()->ButtonNopCallback(w->wib_windesc, (int)bev->x, (int)bev->y,
                bev->state);
        break;
    }
    if (showing_ghost)
        w->ShowGhost(DISPLAY);
    return (true);
}


// Static function.
// Dispatch button release events to the application callbacks.
//
int
win_bag::button_up_hdlr(GtkWidget*, GdkEvent *event, void *client_data)
{
    GdkEventButton *bev = (GdkEventButton*)event;
    int button = Kmap()->ButtonMap(bev->button);
    if (!GTK_WIDGET_SENSITIVE(gtkMenu()->MainMenu()) && button == 1)
        // menu is insensitive, so ignore release
        return (true);
    if (bev->button < GS_NBTNS)
        button_state[bev->button] = false;

    win_bag *w = static_cast<win_bag*>(client_data);

    if (!grabstate.is_armed(bev)) {
        grabstate.check_noop(false);
        return (true);
    }
    grabstate.clear(bev);

    // This finishes processing the button up event at the server,
    // otherwise motions are frozen until this function returns.
    gtk_DoEvents(100);

    // The point can be outside of the viewport, due to the grab.
    // Check for this.
    const BBox *BB = &w->wib_windesc->Viewport();
    bool in = (bev->x >= 0 && bev->x < BB->right &&
        bev->y >= 0 && bev->y < BB->bottom);

    bool showing_ghost = w->Gbag()->showing_ghost();
    if (showing_ghost)
        w->ShowGhost(ERASE);
    switch (button) {
    case 1:
        if (grabstate.check_noop(false))
            EV()->ButtonNopReleaseCallback((in ? w->wib_windesc : 0),
                (int)bev->x, (int)bev->y, bev->state);
        else
            EV()->Button1ReleaseCallback((in ? w->wib_windesc : 0),
                (int)bev->x, (int)bev->y, bev->state);
        break;
    case 2:
        EV()->Button2ReleaseCallback((in ? w->wib_windesc : 0),
            (int)bev->x, (int)bev->y, bev->state);
        break;
    case 3:
        EV()->Button3ReleaseCallback((in ? w->wib_windesc : 0),
            (int)bev->x, (int)bev->y, bev->state);
        break;
    case 4:
    case 5:
        break;
    default:
        EV()->ButtonNopReleaseCallback((in ? w->wib_windesc : 0),
            (int)bev->x, (int)bev->y, bev->state);
        break;
    }
    if (showing_ghost)
        w->ShowGhost(DISPLAY);
    return (true);
}


// Mouse-wheel scroll handler for GTK-2.
//
int
win_bag::scroll_hdlr(GtkWidget*, GdkEvent *event, void *client_data)
{
    win_bag *w = static_cast<win_bag*>(client_data);
    if (event->scroll.direction == GDK_SCROLL_UP) {

        if (event->scroll.state & GDK_CONTROL_MASK) {
            if (DSP()->MouseWheelZoomFactor() > 0.0)
                w->wib_windesc->Zoom(1.0 - DSP()->MouseWheelZoomFactor());
        }
        else if (event->scroll.state & GDK_SHIFT_MASK) {
            if (DSP()->MouseWheelPanFactor() > 0.0)
                w->wib_windesc->Pan(DirEast, DSP()->MouseWheelPanFactor());
        }
        else {
            if (DSP()->MouseWheelPanFactor() > 0.0)
                w->wib_windesc->Pan(DirNorth, DSP()->MouseWheelPanFactor());
        }
    }
    else if (event->scroll.direction == GDK_SCROLL_DOWN) {
        if (event->scroll.state & GDK_CONTROL_MASK) {
            if (DSP()->MouseWheelZoomFactor() > 0.0)
                w->wib_windesc->Zoom(1.0 + DSP()->MouseWheelZoomFactor());
        }
        else if (event->scroll.state & GDK_SHIFT_MASK) {
            if (DSP()->MouseWheelPanFactor() > 0.0)
                w->wib_windesc->Pan(DirWest, DSP()->MouseWheelPanFactor());
        }
        else {
            if (DSP()->MouseWheelPanFactor() > 0.0)
                w->wib_windesc->Pan(DirSouth, DSP()->MouseWheelPanFactor());
        }
    }
    return (true);
}


// Define this to put motion handling into an idle proc.
#define MOTION_IDLE

// Static function.
// Handle pointer motion in the main and subwindows.  In certain
// commands, "ghost" XOR drawing is utilized.
//
int
win_bag::motion_hdlr(GtkWidget*, GdkEvent *event, void *client_data)
{
    win_bag *w = static_cast<win_bag*>(client_data);
    GdkEventMotion *mev = (GdkEventMotion*)event;
#ifdef MOTION_IDLE
    if (w->wib_windesc) {
        if (w->wib_id) {
            gtk_idle_remove(w->wib_id);
            w->wib_id = 0;
        }
        w->wib_state = mev->state;
        w->wib_x = (int)mev->x;
        w->wib_y = (int)mev->y;
        w->wib_id = gtk_idle_add(motion_idle, client_data);
    }
#else
    if (w->wib_windesc) {
        EV()->MotionCallback(w->wib_windesc, mev->state);
        if (Gst()->ShowingGhostInWindow(w->wib_windesc)) {
            w->UndrawGhost();
            w->DrawGhost((int)mev->x, (int)mev->y);
#ifdef WITH_QUARTZ
            // This has some extra magic to get around broken Quartz
            // back-end.
            WindowDesc *wd;
            WDgen wgen(WDgen::MAIN, WDgen::CHD);
            while ((wd = wgen.next()) != 0)
                wd->Wdraw()->Update();
#endif
        }
        if (Coord()) {
            int x = (int)mev->x;
            int y = (int)mev->y;
            w->wib_windesc->PToL(x, y, x, y);
            Coord()->print(x, y, cCoord::COOR_MOTION);
        }
    }
#endif
    return (true);
}


int
win_bag::motion_idle(void *arg)
{
    win_bag *w = static_cast<win_bag*>(arg);
    w->wib_id = 0;
    if (w->windesc()) {
        EV()->MotionCallback(w->windesc(), w->wib_state);
        if (Gst()->ShowingGhostInWindow(w->wib_windesc)) {
            w->UndrawGhost();
            w->DrawGhost(w->wib_x, w->wib_y);
#ifdef WITH_QUARTZ
            // This has some extra magic to get around broken Quartz
            // back-end.
            WindowDesc *wd;
            WDgen wgen(WDgen::MAIN, WDgen::CHD);
            while ((wd = wgen.next()) != 0)
                wd->Wdraw()->Update();
#endif
        }
        if (Coord()) {
            int x = w->wib_x;
            int y = w->wib_y;
            w->windesc()->PToL(x, y, x, y);
            Coord()->print(x, y, cCoord::COOR_MOTION);
        }
    }
    return (0);
}


// Static function.
// If the pointer enters a window with an active grab, check if
// a button release has occurred.  If so, and the condition has
// not already been cleared, call the appropriate callback to
// clean up.  This is for button up events that get 'lost'.
//
int
win_bag::enter_hdlr(GtkWidget *caller, GdkEvent *event, void *client_data)
{
    // pointer entered the drawing window
    win_bag *w = static_cast<win_bag*>(client_data);

#ifdef MOTION_IDLE
    if (w->wib_id) {
        gtk_idle_remove(w->wib_id);
        w->wib_id = 0;
    }
#endif
    GTK_WIDGET_SET_FLAGS(caller, GTK_CAN_FOCUS);
    gtk_window_set_focus(GTK_WINDOW(w->Shell()), caller);

    GdkEventCrossing *cev = (GdkEventCrossing*)event;
    if (grabstate.widget(1) == caller) {
        if (!(cev->state & GDK_BUTTON1_MASK)) {
            grabstate.clear(1, cev->time);
            EV()->Button1ReleaseCallback(0, 0, 0, 0);
        }
    }
    if (grabstate.widget(2) == caller) {
        if (!(cev->state & GDK_BUTTON2_MASK)) {
            grabstate.clear(2, cev->time);
            EV()->Button2ReleaseCallback(0, 0, 0, 0);
        }
    }
    if (grabstate.widget(3) == caller) {
        if (!(cev->state & GDK_BUTTON3_MASK)) {
            grabstate.clear(3, cev->time);
            EV()->Button3ReleaseCallback(0, 0, 0, 0);
        }
    }
    if (grabstate.widget(4) == caller) {
        if (!(cev->state & GDK_BUTTON4_MASK))
            grabstate.clear(4, cev->time);
    }
    if (grabstate.widget(5) == caller) {
        if (!(cev->state & GDK_BUTTON4_MASK))
            grabstate.clear(5, cev->time);
    }
    if (grabstate.widget(6) == caller) {
        if (!(cev->state & GDK_BUTTON4_MASK)) {
            grabstate.clear(6, cev->time);
            EV()->ButtonNopReleaseCallback(0, 0, 0, 0);
        }
    }

    EV()->MotionCallback(w->wib_windesc, cev->state);
    return (true);
}


// Static function.
// Gracefully terminate ghost drawing when the pointer leaves a
// window.
//
int
win_bag::leave_hdlr(GtkWidget*, GdkEvent *event, void *client_data)
{
    // pointer left the drawing window
    win_bag *w = static_cast<win_bag*>(client_data);

    // Just ignore event if the window is not the current window, the
    // OS X back-end will do this.
    if (w->wib_windesc != EV()->CurrentWin())
        return (true);

#ifdef MOTION_IDLE
    if (w->wib_id) {
        gtk_idle_remove(w->wib_id);
        w->wib_id = 0;
    }
#endif
    GdkEventCrossing *cev = (GdkEventCrossing*)event;

    EV()->MotionCallback(w->wib_windesc, cev->state);
    w->UndrawGhost(true);
#ifdef WITH_QUARTZ
    // This has some extra magic to get around broken Quartz
    // back-end.
    WindowDesc *wd;
    WDgen wgen(WDgen::MAIN, WDgen::CHD);
    while ((wd = wgen.next()) != 0)
        wd->Wdraw()->Update();
#endif
    return (true);
}


// Static function.
int
win_bag::focus_hdlr(GtkWidget*, GdkEvent*, void*)
{
    // Nothing to do but handle this to avoid default action of
    // issuing expose event.
    return (true);
}


namespace {
    struct load_file_data
    {
        load_file_data(const char *f, const char *c)
            {
                filename = lstring::copy(f);
                cellname = lstring::copy(c);
            }

        ~load_file_data()
            {
                delete [] filename;
                delete [] cellname;
            }

        char *filename;
        char *cellname;
    };

    // Idle procedure to load a file, called from the drop handler.
    //
    int
    load_file_idle(void *arg)
    {
        load_file_data *data = (load_file_data*)arg;
        XM()->Load(EV()->CurrentWin(), data->filename, 0, data->cellname);
        delete data;
        return (0);
    }
}


// Static function.
// Drag data received in drawing window - open the cell.
//
void
win_bag::drag_data_received(GtkWidget*, GdkDragContext *context, gint, gint,
    GtkSelectionData *data, guint, guint time)
{
    bool success = false;
    if (!data->data)
        ;
    else if (data->target == gdk_atom_intern("property", true)) {
        if (gtkEdit() && gtkEdit()->is_active()) {
            unsigned char *val = data->data + sizeof(int);
            CDs *cursd = CurCell(true);
            hyList *hp = new hyList(cursd, (char*)val, HYcvAscii);
            gtkEdit()->insert(hp);
            hyList::destroy(hp);
            success = true;
        }
    }
    else if (data->length >= 0 && data->format == 8) {
        char *src = (char*)data->data;
        char *t = 0;
        if (data->target == gdk_atom_intern("TWOSTRING", true)) {
            // Drops from content lists may be in the form
            // "fname_or_chd\ncellname".
            t = strchr(src, '\n');
            if (t)
                *t++ = 0;
        }
        load_file_data *lfd = new load_file_data(src, t);

        bool didit = false;
        if (gtkEdit() && gtkEdit()->is_active()) {
            if (ScedIf()->doingPlot()) {
                // Plot/Iplot edit line is active, break out.
                gtkEdit()->abort();
            }
            else {
                // If editing, push into prompt line.
                // Keep the cellname.
                if (lfd->cellname)
                    gtkEdit()->insert(lfd->cellname);
                else
                    gtkEdit()->insert(lfd->filename);
                delete lfd;
                didit = true;
            }
        }
        if (!didit)
            gtkPkgIf()->RegisterIdleProc(load_file_idle, lfd);
        success = true;
    }
    gtk_drag_finish(context, success, false, time);
}


// Static function.
void
win_bag::target_drag_leave(GtkWidget *widget, GdkDragContext*, guint)
{
    // called on drop, too
    if (HaveDrag) {
        if (gtk_object_get_data(GTK_OBJECT(widget), "drag_hlite")) {
            gtk_drag_unhighlight(widget);
            gtk_object_remove_data(GTK_OBJECT(widget), "drag_hlite");
        }
        DontRedrawMe = widget;
        HaveDrag = false;
    }
}


// The default highlighting action causes a redraw of the main viewport.
// Avoid this by handling highlighting here.

// These are NOT called if the drag data is not targeted, i.e., for
// colors and fillpatterns.

// Static function.
gboolean
win_bag::target_drag_motion(GtkWidget *widget, GdkDragContext*, gint, gint,
    guint)
{
    if (!HaveDrag) {
        HaveDrag = true;
        DontRedrawMe = widget;
        gtk_drag_highlight(widget);
        gtk_object_set_data(GTK_OBJECT(widget), "drag_hlite", (void*)1);
    }
    return (true);
}


// Static function.
void
win_bag::subwin_cancel_proc(GtkWidget*, void *client_data)
{
    int wnum = (long)client_data;
    if (wnum > 0 && wnum < DSP_NUMWINS)
        delete DSP()->Window(wnum);
}


// Static function.
// Pop up info about the keys pressed area in help mode.
//
int
win_bag::keys_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    if (XM()->IsDoingHelp() && event->type == GDK_BUTTON_PRESS &&
            !is_shift_down())
        DSPmainWbag(PopUpHelp("keyspresd"))
    return (true);
}


//-----------------------------------------------------------------------------
// main_bag functions

// Setup function for main window.
//
void
main_bag::initialize()
{
#if GTK_CHECK_VERSION(2,10,4)
#ifdef WIN32
    // Icons are obtained from resources.
#else
    // I don't know which release this function arrived in.
    GList *g1 = new GList;
    g1->data = gdk_pixbuf_new_from_xpm_data(xic_48x48_xpm);
    GList *g2 = new GList;
    g2->data = gdk_pixbuf_new_from_xpm_data(xic_32x32_xpm);
    g1->next = g2;
    GList *g3 = new GList;
    g3->data = gdk_pixbuf_new_from_xpm_data(xic_16x16_xpm);
    g3->next = 0;
    g2->next = g3;
    gtk_window_set_icon_list(GTK_WINDOW(wb_shell), g1);
    delete g3;
    delete g2;
    delete g1;
#endif
    gtk_window_set_default_icon_name("xic");
#endif
    gtk_widget_set_name(wb_shell, "MainFrame");

    DSP()->SetWindow(0, new WindowDesc);
    EV()->SetCurrentWin(DSP()->MainWdesc());
    wib_windesc = DSP()->MainWdesc();

    // We use w_draw for rendering, as this pointer may point to a
    // hard-copy context.  Rendering through w_wbag will always go
    // to the screen.
    //
    DSP()->MainWdesc()->SetWbag(this);
    DSP()->MainWdesc()->SetWdraw(this);

    // This is done again in AppInit(), but it is needed for the menus.
    DSP()->SetCurMode(XM()->InitialMode());

    // set up colors
    xrm_load_colors();
    DSP()->ColorTab()->init();

    // set up signals for top level window
    gtk_widget_add_events(wb_shell, GDK_STRUCTURE_MASK);
    gtk_signal_connect(GTK_OBJECT(wb_shell), "map-event",
        GTK_SIGNAL_FUNC(map_hdlr), this);
    gtk_signal_connect(GTK_OBJECT(wb_shell), "unmap-event",
        GTK_SIGNAL_FUNC(map_hdlr), this);
    gtk_signal_connect(GTK_OBJECT(wb_shell), "destroy-event",
        GTK_SIGNAL_FUNC(main_destroy_proc), 0);
    gtk_signal_connect(GTK_OBJECT(wb_shell), "delete-event",
        GTK_SIGNAL_FUNC(main_destroy_proc), 0);

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 2);
    GtkWidget *form = gtk_table_new(1, 4, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(frame), form);
    gtk_container_add(GTK_CONTAINER(wb_shell), frame);
    int rowcnt = 0;

    // The WR logo button and main menu.
    //
    GtkWidget *hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);

    // We will add the WR button later.
    GtkWidget *main_menu_box = hbox;

    gtkMenu()->InitMainMenu(wb_shell);
    gtk_box_pack_start(GTK_BOX(hbox), gtkMenu()->MainMenu(), true,
        true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 0, 0);
    rowcnt++;

    // Button menu.
    //
    bool horiz_buttons = getenv("XIC_HORIZ_BUTTONS");
    GtkWidget *btnarray = 0;
    if (EditIf()->hasEdit()) {
        if (horiz_buttons)
            btnarray = gtk_hbox_new(false, 0);
        else
            btnarray = gtk_vbox_new(false, 0);
        gtk_widget_show(btnarray);

        gtkMenu()->InitSideButtonMenus(horiz_buttons);
        if (DSP()->CurMode() == Physical) {
            if (gtkMenu()->ButtonMenu(Physical))
                gtk_widget_show(gtkMenu()->ButtonMenu(Physical));
        }
        else {
            if (gtkMenu()->ButtonMenu(Electrical))
                gtk_widget_show(gtkMenu()->ButtonMenu(Electrical));
        }
        if (gtkMenu()->ButtonMenu(Physical))
            gtk_box_pack_start(GTK_BOX(btnarray),
                gtkMenu()->ButtonMenu(Physical), true, true, 0);
        if (gtkMenu()->ButtonMenu(Electrical))
            gtk_box_pack_start(GTK_BOX(btnarray),
                gtkMenu()->ButtonMenu(Electrical), true, true, 0);
        if (horiz_buttons) {
            gtk_table_attach(GTK_TABLE(form), btnarray, 0, 1, rowcnt, rowcnt+1,
                (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
                (GtkAttachOptions)0, 0, 0);
            rowcnt++;
        }
        else
            gtk_widget_set_usize(btnarray, 36, -1);
    }

    // Main drawing window (in frame).
    //
    gd_viewport = gtk_drawing_area_new();

    gtk_widget_set_name(gd_viewport, "Viewport");
    gtk_drawing_area_size(GTK_DRAWING_AREA(gd_viewport), 800, 600);
    gtk_widget_show(gd_viewport);

    gtk_widget_add_events(gd_viewport, GDK_STRUCTURE_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "configure-event",
        GTK_SIGNAL_FUNC(resize_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_EXPOSURE_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "expose-event",
        GTK_SIGNAL_FUNC(redraw_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_KEY_PRESS_MASK);
    gtk_signal_connect_after(GTK_OBJECT(gd_viewport), "key-press-event",
        GTK_SIGNAL_FUNC(key_dn_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_KEY_RELEASE_MASK);
    gtk_signal_connect_after(GTK_OBJECT(gd_viewport), "key-release-event",
        GTK_SIGNAL_FUNC(key_up_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect_after(GTK_OBJECT(gd_viewport), "button-press-event",
        GTK_SIGNAL_FUNC(button_dn_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_BUTTON_RELEASE_MASK);
    gtk_signal_connect_after(GTK_OBJECT(gd_viewport), "button-release-event",
        GTK_SIGNAL_FUNC(button_up_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_POINTER_MOTION_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "motion-notify-event",
        GTK_SIGNAL_FUNC(motion_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_ENTER_NOTIFY_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "enter-notify-event",
        GTK_SIGNAL_FUNC(enter_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_LEAVE_NOTIFY_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "leave-notify-event",
        GTK_SIGNAL_FUNC(leave_hdlr), this);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "scroll-event",
        GTK_SIGNAL_FUNC(scroll_hdlr), this);

    // For gtk2 - this avoids issue of an expose event on focus change.
    //
    gtk_widget_add_events(gd_viewport, GDK_FOCUS_CHANGE_MASK);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "focus-in-event",
        GTK_SIGNAL_FUNC(focus_hdlr), this);
    gtk_signal_connect(GTK_OBJECT(gd_viewport), "focus-out-event",
        GTK_SIGNAL_FUNC(focus_hdlr), this);

    GtkWidget *maindrawingwin = gtk_frame_new(0);
    gtk_widget_show(maindrawingwin);
    gtk_container_add(GTK_CONTAINER(maindrawingwin), gd_viewport);

    // Drop handler setup.
    //
    GtkDestDefaults DD = (GtkDestDefaults)
        (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP);
    gtk_drag_dest_set(maindrawingwin, DD, main_targets, n_main_targets,
        GDK_ACTION_COPY);
    gtk_signal_connect_after(GTK_OBJECT(maindrawingwin),
        "drag-data-received",
        GTK_SIGNAL_FUNC(drag_data_received), 0);
    gtk_signal_connect(GTK_OBJECT(maindrawingwin), "drag-leave",
        GTK_SIGNAL_FUNC(target_drag_leave), 0);
    gtk_signal_connect(GTK_OBJECT(frame), "drag-motion",
        GTK_SIGNAL_FUNC(target_drag_motion), 0);

    GTKfont::setupFont(gd_viewport, FNT_SCREEN, false);

    hbox = gtk_hbox_new(false, 4);
    gtk_widget_show(hbox);

    GTKltab *ltab = new GTKltab(false);
    mb_auxw = ltab->Viewport();
    LT()->SetLtab(ltab);
    gtk_box_pack_start(GTK_BOX(hbox), ltab->searcher(), false,
        false, 0);

    gtkMenu()->InitTopButtonMenu();

    // The "top button menu" includes the WR button as the first
    // entry.  This we add to the main menubar.
    //
    MenuBox *btn_menu = gtkMenu()->GetMiscMenu();
    if (btn_menu && btn_menu->menu) {
        GtkWidget *wrbtn = GTK_WIDGET(btn_menu->menu[1].cmd.caller);
        gtk_box_pack_start(GTK_BOX(main_menu_box), wrbtn, false, false, 0);
        gtk_box_reorder_child(GTK_BOX(main_menu_box), wrbtn, 0);
    }

    // The rest of the entries are contianed here.
    gtk_box_pack_start(GTK_BOX(hbox), gtkMenu()->TopButtonMenu(), false,
        false, 0);

    new cCoord;
    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), Coord()->Viewport());
    gtk_box_pack_start(GTK_BOX(hbox), frame, true, true, 0);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 0, 2);
    rowcnt++;

    GtkWidget *paned = gtk_hpaned_new();
    gtk_widget_show(paned);
    hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);
    if (!getenv("XIC_MENU_RIGHT")) {
        if (!horiz_buttons && btnarray)
            gtk_box_pack_start(GTK_BOX(hbox), btnarray, false, false, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ltab->scrollbar(), false, false, 0);
        gtk_box_pack_start(GTK_BOX(hbox), paned, true, true, 0);
        gtk_paned_pack1(GTK_PANED(paned), ltab->container(), false, true);
        gtk_paned_pack2(GTK_PANED(paned), maindrawingwin, true, true);
    }
    else {
        gtk_box_pack_start(GTK_BOX(hbox), paned, true, true, 0);
        gtk_paned_pack1(GTK_PANED(paned), maindrawingwin, true, true);
        gtk_paned_pack2(GTK_PANED(paned), ltab->container(), false, true);
        gtk_box_pack_start(GTK_BOX(hbox), ltab->scrollbar(), false, false, 0);
        if (!horiz_buttons && btnarray)
            gtk_box_pack_start(GTK_BOX(hbox), btnarray, false, false, 0);
    }

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 0, 0);
    rowcnt++;

    // The "keyspressed" area (in event box in frame), 'L' button, and
    // interactive prompt/reply text area.
    //
    GTKedit *edit = new GTKedit(false);
    wib_keyspressed = edit->keys();
    wb_textarea = edit->Viewport();
    hbox = edit->container();
    PL()->SetEdit(edit);

    gtk_table_attach(GTK_TABLE(form), hbox, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 0, 0);
    rowcnt++;

    // Parameter display area in frame.
    //
    new cParam;
    mb_readout = Param()->Viewport();

    frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), mb_readout);

    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, rowcnt, rowcnt + 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 0, 0);
    rowcnt++;

    if (!XM()->Geometry() || !*XM()->Geometry()) {
        GdkScreen *screen = gtk_widget_get_screen(wb_shell);

#if GTK_CHECK_VERSION(2,10,0)
        // In RHEL6.3, when the application is not started under
        // Gnome, the Monospace font displays incorrectly (M too
        // wide), and Liberation Mono is not detected as a mono font. 
        // The default hinting level breaks these fonts.  Below is a
        // work-around to fix the problem by setting the hint to the
        // Gnome default.

        cairo_font_options_t *fo = cairo_font_options_create();
        cairo_font_options_set_hint_style(fo, CAIRO_HINT_STYLE_SLIGHT);
        gdk_screen_set_font_options(screen, fo);
#endif

        int pmon = 0;
#if GTK_CHECK_VERSION(2,20,0)
        {
            int xx, yy;
            GRX->PointerRootLoc(&xx, &yy);
            pmon = gdk_screen_get_monitor_at_point(screen, xx, yy);
        }
#endif
        GdkRectangle r;
        gdk_screen_get_monitor_geometry(screen, pmon, &r);
        int w = (8*r.width)/10;
        int h = (8*r.height)/10;
        if (h < 600)
            h = r.height;
        int x = r.x + (r.width - w)/2;
        int y = r.y + (r.height - h)/2;
        gtk_window_set_default_size(GTK_WINDOW(wb_shell), w, h);
        gtk_widget_set_uposition(wb_shell, x, y);
    }
    else {
        gtk_window_parse_geometry(GTK_WINDOW(wb_shell), XM()->Geometry());
    }

    // Realize
    //
    gtk_widget_show(wb_shell);
    gd_window = gd_viewport->window;
    GRX->SetDefaultFocusWin(gd_window);

    // Make the GC's.
    //
    GdkGCValues gcvalues;
    gcvalues.cap_style = GDK_CAP_BUTT;
    Gbag()->set_gc(gdk_gc_new_with_values(gd_window, &gcvalues,
        (GdkGCValuesMask)GDK_GC_CAP_STYLE));

    // Growl... GTK doesn't have this
#ifdef WITH_X11
    XGCValues xval;
    xval.fill_rule = WindingRule;
    XChangeGC(gr_x_display(), gr_x_gc(GC()), GCFillRule, &xval);
#endif

    gcvalues.function = GDK_XOR;
    Gbag()->set_xorgc(gdk_gc_new_with_values(gd_window, &gcvalues,
        (GdkGCValuesMask)(GDK_GC_FUNCTION | GDK_GC_CAP_STYLE)));

    gd_backg = GRX->NameColor("black");
    gd_foreg = GRX->NameColor("white");

    GdkColor clr;
    clr.pixel = gd_backg;
    gtk_QueryColor(&clr);
    gdk_gc_set_background(GC(), &clr);
    gdk_gc_set_background(XorGC(), &clr);
    clr.pixel = gd_foreg;
    gtk_QueryColor(&clr);
    gdk_gc_set_foreground(GC(), &clr);
    clr.pixel = gd_backg ^ gd_foreg;
    gtk_QueryColor(&clr);
    gdk_gc_set_foreground(XorGC(), &clr);

    // Set backgrounds of text areas and drawing areas.
    //
    SetWindowBackground(gd_backg);
    clr.pixel = DSP()->Color(PromptBackgroundColor);
    gtk_QueryColor(&clr);
    gdk_window_set_background(wb_textarea->window, &clr);
    gdk_window_clear(wb_textarea->window);
    gdk_window_set_background(mb_readout->window, &clr);
    gdk_window_clear(mb_readout->window);

    // Set the I-beam cursor in the prompt and status text area.

    GdkCursor *i_cursor = gdk_cursor_new(GDK_XTERM);
    gdk_window_set_cursor(wb_textarea->window, i_cursor);
    gdk_window_set_cursor(mb_readout->window, i_cursor);
}


void
main_bag::send_key_event(sKeyEvent *k)
{
    if (k->type == KEY_PRESS)
        keypress_handler(k->key, k->state, k->text, false, false);
    else if (k->type == KEY_RELEASE)
        keypress_handler(k->key, k->state, k->text, false, true);
}


// Static function.
// Handle window manager delete of the main window.  Don't allow exiting
// when busy.
//
int
main_bag::main_destroy_proc(GtkWidget*, GdkEvent*, void*)
{
    if (gtkPkgIf()->IsBusy()) {
        pop_busy();
        return (true);
    }
    if (!gtkMenu()->IsGlobalInsensitive())
        XM()->Exit(ExitCheckMod);
    return (true);
}


// Static function.
// Load colors using the X resource mechanism.
//
void
main_bag::xrm_load_colors()
{
#ifdef WITH_X11
    // load string resource color names
    static bool doneit;
    if (!doneit) {
        // obtain the database
        doneit = true;
        passwd *pw = getpwuid(getuid());
        if (pw) {
            char buf[512];
            sprintf(buf, "%s/%s", pw->pw_dir, XM()->Product());
            if (access(buf, R_OK))
                return;
            XrmDatabase rdb = XrmGetFileDatabase(buf);
            XrmSetDatabase(gr_x_display(), rdb);
        }
    }
    char name[64], clss[64];
    sprintf(name, "%s.", XM()->Product());
    lstring::strtolower(name);
    char *tn = name + strlen(name);
    sprintf(clss, "%s.", XM()->Product());
    char *tc = clss + strlen(clss);
    XrmDatabase db = XrmGetDatabase(gr_x_display());
    for (int i = 0; i < ColorTableEnd; i++) {
        sColorTab::sColorTabEnt *c = DSP()->ColorTab()->color_ent(i);
        strcpy(tn, c->keyword());
        strcpy(tc, c->keyword());
        if (isupper(tn[0]))
            tn[0] = tolower(tn[0]);
        if (islower(tc[0]))
            tc[0] = toupper(tc[0]);
        char *ss;
        XrmValue v;
        if (XrmGetResource(db, name, clss, &ss, &v)) {
            if (v.addr && *v.addr)
                c->set_defclr(DSP()->ColorTab(), v.addr);
        }
    }
    for (int i = 0; i < GRattrColorEnd; i++) {
        const char *keyword = DSP()->ColorTab()->gui_color_name(i);
        if (!keyword)
            break;
        strcpy(tn, keyword);
        strcpy(tc, keyword);
        if (isupper(tn[0]))
            tn[0] = tolower(tn[0]);
        if (islower(tc[0]))
            tc[0] = toupper(tc[0]);
        char *ss;
        XrmValue v;
        if (XrmGetResource(db, name, clss, &ss, &v)) {
            if (v.addr && *v.addr)
                GRpkgIf()->SetAttrColor((GRattrColor)i, v.addr);
        }
    }
#endif
}


//-----------------------------------------------------------------------
// sEventhdlr functions

// Static function.
// Main event handler.
//
void
sEventHdlr::main_event_handler(GdkEvent *event, void*)
{
    if (event->type == GDK_KEY_PRESS) {
        LogEvent(event);
        if ((event->key.keyval == GDK_c || event->key.keyval == GDK_C) &&
                (event->key.state & GDK_CONTROL_MASK)) {

            cMain::InterruptHandler();
            return;
        }
        if (gtkPkgIf()->IsBusy()) {
            if (!is_modifier_key(event->key.keyval))
                pop_busy();
            return;
        }
    }
    else if (event->type == GDK_KEY_RELEASE) {
        LogEvent(event);
        if ((event->key.keyval == GDK_c || event->key.keyval == GDK_C) &&
                (event->key.state & GDK_CONTROL_MASK)) {

            return;
        }
        // Always handle key-up events, otherwise if busy modifier key
        // state may not be correct after operation.
    }
    else if (event->type == GDK_BUTTON_PRESS) {
        LogEvent(event);
        if (gtkPkgIf()->IsBusy()) {
            GtkWidget *evw = gtk_get_event_widget(event);
            if (gtk_object_get_data(GTK_OBJECT(evw), "abort"))
                gtk_main_do_event(event);
            else if (event->button.button < 4)
                // Ignore mouse wheel events.
                pop_busy();
            return;
        }
        if (gtkMenu()->IsGlobalInsensitive()) {
            GtkWidget *evw = gtk_get_event_widget(event);
            if (GTK_IS_DRAWING_AREA(evw))
                return;
            GtkWidget *top = gtk_widget_get_toplevel(evw);
            if (!top || !mainBag())
                return;
            if (top != mainBag()->Shell() && top != gtkMenu()->GetModal())
                return;
        }
    }
    else if (event->type == GDK_BUTTON_RELEASE) {
        LogEvent(event);
        if (!gtk_grab_get_current()) {
            if (gtkPkgIf()->IsBusy()) {
                GdkEventButton *bev = (GdkEventButton*)event;
                if (button_state[bev->button]) {
                    // If the button state is active, the mouse button must
                    // have been down when busy mode was entered.  Save the
                    // event for dispatch when leaving busy mode.
                    event_hdlr.save_event(event);
                    button_state[bev->button] = false;
                    return;
                }
                GtkWidget *evw = gtk_get_event_widget(event);
                if (gtk_object_get_data(GTK_OBJECT(evw), "abort"))
                    gtk_main_do_event(event);
                return;
            }
            if (gtkMenu()->IsGlobalInsensitive()) {
                GtkWidget *evw = gtk_get_event_widget(event);
                if (GTK_IS_DRAWING_AREA(evw))
                    return;
                GtkWidget *top = gtk_widget_get_toplevel(evw);
                if (!top || !mainBag())
                    return;
                if (top != mainBag()->Shell() && top != gtkMenu()->GetModal())
                    return;
            }
        }
    }
    else if (event->type == GDK_2BUTTON_PRESS ||
            event->type == GDK_3BUTTON_PRESS) {
        if (gtkPkgIf()->IsBusy())
            return;
    }
    gtk_main_do_event(event);
}


//-----------------------------------------------------------------------
// Local functions

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
                sprintf(buf, "%s_extra%d", cbs->components[i].name, scnt);
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

