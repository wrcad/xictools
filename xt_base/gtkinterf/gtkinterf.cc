
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
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "gtkinterf.h"
#include "gtkfont.h"
#include "gtkhcopy.h"
#include "ginterf/grlinedb.h"
#include "miscutil/texttf.h"
#include "miscutil/lstring.h"
#include <sys/types.h>
#include <sys/time.h>
#ifdef WIN32
#include <winsock2.h>
#include <windowsx.h>
#include <gdk/gdkwin32.h>
#include "mswdraw.h"
#include "mswpdev.h"
using namespace mswinterf;
#else
#include <sys/select.h>
#endif

// Looks like Cairo and GTK don't support the X-windows shared memory
// extension.
//#define USE_XSHM

#include <gdk/gdkkeysyms.h>
#ifdef WITH_X11
#include "gtkx11.h"
#include <X11/Xproto.h>
#ifdef USE_XSHM
#ifdef HAVE_SHMGET
#include <X11/extensions/XShm.h>  
#endif
#endif
#endif

// Support ancient visuals.
//#define OLD_VISUALS

// Bypass gdk drawing functions for speed.
//
// Note:  Recent GTK (2.18.6) sets up a clipping context in the GC, so
// directly sharing the XGC causes display problems.
//
// We always use gdk drawing functions with GTK-2.  The direct to X
// functions work fine up to GTK-2.18 (probably), but since the GTK-2
// releases are dynamically linked, we can't count on the user having
// a compatible GTK-2 installation.
//
// #define DIRECT_TO_X

// Use our own Win32 drawing.  This is necessary, as currently gdk
// does not handle stippled drawing.
#define DIRECT_TO_GDI

// Global access pointer.
GTKdev *GRX;


// Device-dependent setup
//
void
GRpkg::DevDepInit(unsigned int cfg)
{
    if (cfg & _devGTK_)
        GRpkgIf()->RegisterDevice(new GTKdev);
#ifdef WIN32
    if (cfg && _devMSP_) {
        GRpkgIf()->RegisterDevice(new MSPdev);
        GRpkgIf()->RegisterHcopyDesc(&MSPdesc);
    }
#endif
}


// Graphics context storage.  The 0 element is the default.
//
sGbag *sGbag::app_gbags[NUMGCS];

// Color Allocations.
GTKdev::sColorAlloc GTKdev::ColorAlloc;


// Static method to create/return the default graphics context.  Note that
// this does not create the GCs.
//
sGbag *
sGbag::default_gbag(int type)
{
    if (type < 0 || type >= NUMGCS)
        type = 0;
    if (!app_gbags[type])
        app_gbags[type] = new sGbag;
    return (app_gbags[type]);
}


//-----------------------------------------------------------------------------
// GTKdev methods

GTKdev::GTKdev()
{
    if (GRX) {
        fprintf(stderr, "Singleton class GTKdev is already instantiated.\n");
        exit (1);
    }
    GRX = this;
    name = "GTK2";
    ident = _devGTK_;
    devtype = GRmultiWindow;

    dv_minx = 0;
    dv_miny = 0;
    dv_loop_level = 0;
    dv_image_code = 0;
    const char *ics = getenv("XT_IMAGE_CODE");
    if (ics) {
        dv_image_code = atoi(ics);
        if (dv_image_code < 0)
            dv_image_code = 0;
        else if (dv_image_code > 2)
            dv_image_code = 2;
    }
            
    dv_main_wbag = 0;

    dv_default_focus_win = 0;
    dv_cmap = 0;
    dv_visual = 0;
    dv_lower_win_offset = 0;
    dv_dual_plane = false;
    dv_true_color = false;
#ifdef WITH_X11
    dv_silence_xerrors = false;
    dv_no_to_top = false;
    dv_screen = 0;
    dv_console_xid = 0;
    dv_big_window_xid = 0;
#endif
#ifdef WIN32
    dv_crlf_terminate = true;
#endif

#ifdef DIRECT_TO_X
    // GRmultiPt uses short integers.
    GRmultiPt::set_short_data(true);
#endif
}


GTKdev::~GTKdev()
{
    GRX = 0;
}


#ifdef WITH_X11

// The handler below replaces the default gtk2 x error handler.  Since
// gtk2 internally uses error_trap_push/pop, we get what we need:
// silence during push, and our (non-exiting) handler otherwise.

namespace {
    // These aren't used
    int x_error_warnings = 1;
    int x_error_code;

    // Replacement handler for X errors, which prints a message and,
    // unlike the handler installed by gtk, does not exit.
    //
    int
    x_error_handler(Display *display, XErrorEvent *error)
    {
        static int tm_err_cnt;
        if (error->error_code) {
            if (x_error_warnings && !GRX->IsSilenceErrs()) {
                char buf[64];
                XGetErrorText(display, error->error_code, buf, 63);
                fprintf(stderr, "X-ERROR **: %s\n  serial %ld error_code %d "
                        "request_code %d minor_code %d\n",
                    buf, error->serial, error->error_code,
                    error->request_code, error->minor_code);
                if (error->request_code == X_QueryPointer &&
                        error->error_code == BadWindow) {
                    tm_err_cnt++;
                    if (tm_err_cnt == 2)
                        GRpkgIf()->ErrPrintf(ET_MSG,
            "It appears that \"trusted\" X11 forwarding is not enabled.\n"
            "This application will not work reliably in this case, as\n"
            "requests for mouse position and other information will fail,\n"
            "generating an error message.  Trusted forwarding can be\n"
            "established by using \"ssh -Y\" when connecting to the\n"
            "application host.\n");
                }
            }
            x_error_code = error->error_code;
        }
        return (0);
    }
}

#endif


namespace {
    // Local key handler for GtkEntry widgets, Ctrl-p will insert the
    // primary selection (like mouse button 2).

    int (*orig_key_press_event)(GtkWidget*, GdkEventKey*);

    void paste_received(GtkClipboard*, const char *text, void *arg)
    {
        if (text) {
            int p = gtk_editable_get_position(GTK_EDITABLE(arg));
            gtk_editable_insert_text(
                GTK_EDITABLE(arg), text, strlen(text), &p);
            gtk_editable_set_position(GTK_EDITABLE(arg), p);
        }
    }

    int my_key_press_event(GtkWidget *w, GdkEventKey *event)
    {
        if ((event->state & GDK_CONTROL_MASK) &&
                (event->keyval == 'p' || event->keyval == 'P')) {

            gtk_clipboard_request_text(
                gtk_widget_get_clipboard(w, GDK_SELECTION_PRIMARY),
                paste_received, w);
            return (true);
        }
        return ((*orig_key_press_event)(w, event));
    }

    GPrintFunc old_print_handler;

    void
    new_print_handler(const char *string)
    {
        if (!string)
            return;
        if (lstring::prefix("_gtk_text_util_create_rich", string))
            return;
        (*old_print_handler)(string);
    }
}


// Initialize the display.
//
bool
GTKdev::Init(int *argc, char **argv)
{
    // Translate some Xt options
    for (int i = 1; i < *argc; i++) {
        if (!strcmp(argv[i], "-display") || !strcmp(argv[i], "-d"))
            argv[i] = lstring::copy("--display");
        else if (!strcmp(argv[i], "-name"))
            argv[i] = lstring::copy("--name");
        else if (!strcmp(argv[i], "-synchronous"))
            argv[i] = lstring::copy("--sync");
    }

    /*** Don't do this!  Causes deadlock if gtk_main is called recursively.
    // Init multi-threading
    if (!g_thread_supported())
        g_thread_init(0);
    gdk_threads_init();
    ***/

#ifdef WITH_X11
    // Start with this set.
    XSetErrorHandler(x_error_handler);
#endif

#ifdef USE_XSHM
    // Here's a hack:  the --no-xshm argument is apparently no longer
    // supported.  Let's handle it ourselves.
    for (int i = 1; i < *argc; i++) {
        if (!strcmp(argv[i], "--no-xshm")) {
            gdk_set_use_xshm(false);
            (*argc)--;
            while (i < *argc) {
                argv[i] = argv[i+1];
                i++;
            }
            argv[*argc] = 0;
            break;
        }
    }
#endif

    if (!gtk_init_check(argc, &argv)) {
#ifdef WITH_X11
        const char *dsp = gdk_get_display();
        if (dsp)
            g_warning("cannot open display: %s", gdk_get_display());
        else
            g_warning("cannot open display");
#endif
        return (true);
    }

    FC.initFonts();

    // Here's a bit of a hack - some themes leave a lot of white space
    // around button content, taking a lot of area, and possibly
    // causing the side menu buttons to truncate the images if there
    // is insufficient screen resolution.  If the environment variable
    // is set, make this spacing zero.

    if (getenv("XT_GUI_COMPACT")) {
        const char *c_str =
"style \"mybtn\" = \"GtkButton\" { GtkButton::inner_border = {0, 0, 0, 0} }\n"
"style \"myent\" = \"GtkEntry\" { GtkEntry::inner_border = {0, 0, 0, 0} }\n" 
"class \"GtkButton\" style \"mybtn\"\n"
"class \"GtkEntry\" style \"myent\"\n";
        gtk_rc_parse_string(c_str);
    }

#ifdef WITH_X11
    // Set again, since GTK-2 has reset the handler.
    XSetErrorHandler(x_error_handler);
#endif

    // We want the window ID of the controlling terminal.  Assume that
    // it has the focus when this function is called, this is not
    // really good.
#ifdef WITH_X11
    int foc;
    Window xwin;
    XGetInputFocus(gr_x_display(), &xwin, &foc);
    dv_console_xid = xwin;
#endif

    // set correct information
    GdkDisplay *display = gdk_display_get_default();
    GdkScreen *scr = gdk_display_get_default_screen(display);
    GdkRectangle r;
    gdk_screen_get_monitor_geometry(scr, 0, &r);
    width = r.width;
    height = r.height;
    // application specific code must reset these
    int planes = 8;
    numcolors = 1 << planes;
    numlinestyles = 0;

    if (!orig_key_press_event) {
        // Add a Ctrl-p binding to the GtkEntry widget, which will insert
        // the primary selection at the cursor.
        //
        GtkWidget *entry = gtk_entry_new();
            GtkWidgetClass *ks = GTK_WIDGET_GET_CLASS(entry);
        if (ks) {
            orig_key_press_event = ks->key_press_event;
            ks->key_press_event = my_key_press_event;
        }
        g_object_ref(entry);
//XXX        gtk_object_ref(GTK_OBJECT(entry));
        gtk_widget_destroy(entry);
        g_object_unref(entry);
    }

    // In GTK-2.10.4 (RHEL5), there is a spurious(?) g_print message
    // when dragging (using native handler) from a GtkTextView.
    old_print_handler = g_set_print_handler(new_print_handler);

    return (false);
}


// Allocate the read/write colorcells.  Try to allocate 2-plane cells
// so ghost drawing always appears white.  If this fails, allocate
// single plane cells.  If mincolors or more cells can not be allocated,
// return true.  If maxcolors < 0, set a flag to prevent private cell
// allocation elsewhere.
//
bool
GTKdev::InitColormap(int mincolors, int maxcolors, bool dualplane)
{
    dv_cmap = gdk_colormap_get_system();
    dv_visual = gdk_visual_get_system();

    if (maxcolors <= 0) {
        if (maxcolors < 0)
            ColorAlloc.no_alloc = true;
        return (false);
    }

    // Make sure that the necessary visual is available
    if (!dv_visual) {
        fprintf(stderr, "Fatal error: Graphics mode unsupported.\n");
        return (true);
    }
    const char *msg = "Found %s visual, %d planes.\n";
    switch (gdk_visual_get_visual_type(dv_visual)) {
    case GDK_VISUAL_STATIC_GRAY:
        printf(msg, "static gray", gdk_visual_get_depth(dv_visual));
        dv_true_color = true;
        ColorAlloc.no_alloc = true;
        return (false);
    case GDK_VISUAL_STATIC_COLOR:
        printf(msg, "static color", gdk_visual_get_depth(dv_visual));
        dv_true_color = true;
        ColorAlloc.no_alloc = true;
        return (false);
    case GDK_VISUAL_GRAYSCALE:
        printf(msg, "grayscale", gdk_visual_get_depth(dv_visual));
        dv_true_color = false;
        break;
    case GDK_VISUAL_PSEUDO_COLOR:
        printf(msg, "pseudo color", gdk_visual_get_depth(dv_visual));
        dv_true_color = false;
        break;
    case GDK_VISUAL_TRUE_COLOR:
        printf(msg, "true color", gdk_visual_get_depth(dv_visual));
        dv_true_color = true;
        ColorAlloc.no_alloc = true;
        return (false);
    case GDK_VISUAL_DIRECT_COLOR:
        printf(msg, "direct color", gdk_visual_get_depth(dv_visual));
        dv_true_color = true;
        ColorAlloc.no_alloc = true;
        return (false);
    }
    // No longer support these visuals.
    fprintf(stderr,
        "Fatal error: Graphical environment is not unsupported.\n");
    return (true);
}

struct clr_t : public GdkColor
{
    unsigned long tab_key() { return (pixel); }
    clr_t *tab_next() { return (next); }
    void set_tab_next(clr_t *n) { next = n; }

    clr_t *next;
};


// Return the color value for pixel.  This is called from color hardcopy
// drivers when the SetColor method is called.  The value passed is an X
// pixel, if X is being used by the application.
//
// This is called a gazillion times when copying image maps, so a hash
// table is used to speed things up.
//
void
GTKdev::RGBofPixel(int pixel, int *r, int *g, int *b)
{
    static itable_t<clr_t> *pixtab;
    if (!pixtab)
        pixtab = new itable_t<clr_t>;

    clr_t *clr = pixtab->find(pixel);
    if (clr) {
        *r = clr->red;
        *g = clr->green;
        *b = clr->blue;
        return;
    }

    clr = new clr_t;
    clr->pixel = pixel;
    gtk_QueryColor(clr);
    clr->red >>= 8;
    clr->green >>= 8;
    clr->blue >>= 8;
    pixtab->link(clr, false);
    pixtab = pixtab->check_rehash();
    *r = clr->red;
    *g = clr->green;
    *b = clr->blue;
}


//XXX this should go away
// Allocate a private colorcell.  Take cells from the pool until exhausted,
// then allocate as single plane cells individually.  If that fails,
// get a read only cell, and return true.
//
int
GTKdev::AllocateColor(int *address, int red, int green, int blue)
{
    static int num_used;
    GdkColor newcolor;
    newcolor.red   = (red   * 256);
    newcolor.green = (green * 256);
    newcolor.blue  = (blue  * 256);
    if (num_used < ColorAlloc.num_mask_allocated ||
            num_used < ColorAlloc.num_allocated) {
        newcolor.pixel = ColorAlloc.drawing_pixels[num_used++];
        *address = (int)newcolor.pixel;
#ifdef OLD_VISUAL
        gdk_color_change(dv_cmap, &newcolor);
#endif
        return (false);
    }
    if ((ColorAlloc.num_mask_allocated || ColorAlloc.num_allocated) &&
            gdk_colormap_alloc_color(dv_cmap, &newcolor, true, false)) {
        *address = (int)newcolor.pixel;
        num_used++;
        return (false);
    }
    else {
        gdk_colormap_alloc_color(dv_cmap, &newcolor, false, true);
        *address = newcolor.pixel;
        return (true);
    }
}


// Return the pixel that is the closest match to colorname.  Also handle
// decimal triples.
//
int
GTKdev::NameColor(const char *colorname)
{
    GdkColor c;
    if (gtk_ColorSet(&c, colorname))
        return (c.pixel);
#ifdef OLD_VISUALS
    if (gdk_color_black(Colormap(), &c))
        return (c.pixel);
#endif
    return (0);
}


// Set indices[3] to the rgb of the named color.
//
bool
GTKdev::NameToRGB(const char *colorname, int *indices)
{
    indices[0] = 0;
    indices[1] = 0;
    indices[2] = 0;
    if (colorname && *colorname) {
        int r, g, b;
        GdkColor rgb;
        if (sscanf(colorname, "%d %d %d", &r, &g, &b) == 3) {
            if (r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
                    b >= 0 && b <= 255) {
                indices[0] = r;
                indices[1] = g;
                indices[2] = b;
                return (true);
            }
            else if (r >= 0 && r <= 65535 && g >= 0 && g <= 65535 &&
                    b >= 0 && b <= 65535) {
                indices[0] = r/256;
                indices[1] = g/256;
                indices[2] = b/256;
                return (true);
            }
            else
                return (false);
        }
        else if (!gdk_color_parse(colorname, &rgb))
            return (false);
        indices[0] = rgb.red/256;
        indices[1] = rgb.green/256;
        indices[2] = rgb.blue/256;
        return (true);
    }
    return (false);
}


GRdraw *
GTKdev::NewDraw(int apptype)
{
    gtk_draw *w = new gtk_draw();
    if (apptype > 0 && apptype < NUMGCS) {
        // Reset the w->gbag field to one specific to this apptype.
        // The default value is used othewise.
        if (!sGbag::app_gbags[apptype])
            sGbag::app_gbags[apptype] = new sGbag;
        w->SetGbag(sGbag::app_gbags[apptype]);
    }
    return (w);
}


// New window function.  Create a top level shell.  We can pass an
// existing newly-created subclass in the second argument, otherwise a
// new gtk_bag will be created.
//
GRwbag *
GTKdev::NewWbag(const char *appname, GRwbag *reuse)
{
    gtk_bag *w = reuse ? dynamic_cast<gtk_bag*>(reuse) : new gtk_bag();
    w->wb_shell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (appname) {
        char buf[128];
        strcpy(buf, appname);  // appname is actually the class name
        if (isupper(*buf))
            *buf = tolower(*buf);
        gtk_window_set_wmclass(GTK_WINDOW(w->Shell()), buf, appname);
    }
    return (w);
}


// Install a timer, return its ID.
//
int
GTKdev::AddTimer(int msecs, int(*callback)(void*), void *arg)
{
    // Note: callback must have an int return, not a bool!
    return (g_timeout_add(msecs, (GSourceFunc)callback, arg));
}


// Remove installed timer.
//
void
GTKdev::RemoveTimer(int id)
{
    g_source_remove(id);
}


namespace {
    // Interrupt handling.
    //
    GRsigintHdlr sigint_cb;
}

GRsigintHdlr
GTKdev::RegisterSigintHdlr(GRsigintHdlr hdlr)
{
    GRsigintHdlr tmp = sigint_cb;
    sigint_cb = hdlr;
    return (tmp);
}


// This function dispatches any pending events, and is called periodically
// by the hardcopy drivers.  If the return value is true, a ^C was typed,
// otherwise false is returned.
//
bool
GTKdev::CheckForEvents()
{
    bool ret = false;
    GdkEvent *ev;
    while ((ev = gdk_event_get()) != 0) {
        // trap ^C for pause
        if ((ev->type == GDK_KEY_PRESS || ev->type == GDK_KEY_RELEASE) &&
                (ev->key.keyval == GDK_KEY_c || ev->key.keyval == GDK_KEY_C) &&
                (ev->key.state & GDK_CONTROL_MASK)) {
            // ^C pressed...
            if (ev->type == GDK_KEY_PRESS)
                ret = true;
            gdk_event_free(ev);
            continue;
        }
        gtk_main_do_event(ev);
        gdk_event_free(ev);
    }
#ifdef WITH_QUARTZ
    // gtk_events_pending seems to always return true.
    int cnt = 0;
    do {
        gtk_main_iteration_do(false);  // flush the redraw queue
        cnt++;
    } while (gtk_events_pending() && cnt < 10);
#else
    do {
        gtk_main_iteration_do(false);  // flush the redraw queue
    } while (gtk_events_pending());
#endif
    if (ret && sigint_cb)
        (*sigint_cb)();
    return (ret);
}


// Grab a char.  If fd1 or fd2 >= 0, expect input from the stream(s)
// as well.  Return the fd which supplied the char.
//
int
GTKdev::Input(int fd1, int fd2, int *keyret)
{
    // We cause select() to time out periodically, so any timer
    // or alternate input events can be processed.
    //
    for (;;) {
        // First read off the queue before doing the select.
        gtk_DoEvents(100);

#ifdef WITH_X11
        int nfds = ConnectionNumber(gr_x_display());
#else
        int nfds = 0;
#endif
        if (fd1 > nfds)
            nfds = fd1;
        if (fd2 > nfds)
            nfds = fd2;
        fd_set readfds;
        FD_ZERO(&readfds);
#ifdef WITH_X11
        FD_SET(ConnectionNumber(gr_x_display()), &readfds);
#endif
        if (fd1 >= 0)
            FD_SET(fd1, &readfds);
        if (fd2 >= 0)
            FD_SET(fd2, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        int i = select(nfds + 1, &readfds, 0, 0, &timeout);
        if (i < 0)
            // interrupts do this
            continue;
        if (i == 0)
            // timeout
            continue;

        // handle X events first
#ifdef WITH_X11
        if (FD_ISSET(ConnectionNumber(gr_x_display()), &readfds)) {
            // handle ALL X events
            GdkEvent *ev;
            while ((ev = gdk_event_get()) != 0) {
                // trap ^C only
                if (ev->type == GDK_KEY_PRESS) {
                    GdkEventKey *kev = (GdkEventKey*)ev;
                    if ((kev->keyval == GDK_KEY_c || kev->keyval == GDK_KEY_C)
                            && (kev->state & GDK_CONTROL_MASK)) {
                        *keyret = '\003';  // Ctrl-C
                        gdk_event_free(ev);
                        return (ConnectionNumber(gr_x_display()));
                    }
                }
                gtk_main_do_event(ev);
                gdk_event_free(ev);
            }
        }
#endif
        if (fd1 >= 0 && FD_ISSET(fd1, &readfds)) {
            *keyret = GRpkgIf()->GetChar(fd1);
            return (fd1);
        }
        else if (fd2 >= 0 && FD_ISSET(fd2, &readfds)) {
            *keyret = GRpkgIf()->GetChar(fd2);
            return (fd2);
        }
    }
}


// Start a signal handling loop.  If init is true, reset the loop level.
// When we longjmp out of the main loop and reenter, the gtk internal
// loop counter is incremented, and there seems to be no way to directly
// control this variable.  Thus, we have our own counter: loop_level
//
void
GTKdev::MainLoop(bool init)
{
    if (init)
        dv_loop_level = 0;
    dv_loop_level++;
    gtk_main();
    dv_loop_level--;
}


// Return 1 if the current X server supports MIT_SHM.  Return 2 if it also
// supports shared pixmaps.
//
int
GTKdev::UseSHM()
{
#ifdef WITH_X11
#ifdef USE_XSHM
#ifdef HAVE_SHMGET
    if (!gdk_get_use_xshm())
        // --no_xshm was given on command line
        return (0);

    int major, minor, ignore;
    Bool pixmaps;
    if (XQueryExtension(gr_x_display(), "MIT-SHM", &ignore, &ignore,
            &ignore)) {
        if (XShmQueryVersion(gr_x_display(), &major, &minor, &pixmaps) == True)
             return (pixmaps == True ? 2 : 1);
    }
#endif
#endif
#endif
    return (0);
}
// End of virtual overrides.


// Return the connection file descriptor.
//
int
GTKdev::ConnectFd()
{
#ifdef WITH_X11
    if (gr_x_display())
        return (ConnectionNumber(gr_x_display()));
#endif
    return (-1);
}


namespace {
    void toggle_hdlr(GtkWidget *caller, void*)
    {
        g_signal_stop_emission_by_name(G_OBJECT(caller), "toggled");
        g_signal_handlers_disconnect_by_func(G_OBJECT(caller),
            (gpointer)toggle_hdlr, 0);
    }
}


// Set the state of btn to unselected, suppress the signal.
//
void
GTKdev::Deselect(GRobject btn)
{
    if (!btn)
        return;
    if (GTK_IS_TOGGLE_BUTTON(btn)) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn))) {
            g_signal_connect(G_OBJECT(btn), "toggled", G_CALLBACK(toggle_hdlr),
                (gpointer)0);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), 0);
        }
    }
    else if (GTK_IS_CHECK_MENU_ITEM(btn)) {
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(btn))) {
            g_signal_connect(G_OBJECT(btn), "toggled", G_CALLBACK(toggle_hdlr),
                (gpointer)0);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(btn), 0);
        }
    }
}


// Set the state of btn to selected, suppress the signal.
//
void
GTKdev::Select(GRobject btn)
{
    if (!btn)
        return;
    if (GTK_IS_TOGGLE_BUTTON(btn)) {
        if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn))) {
            g_signal_connect(G_OBJECT(btn), "toggled", G_CALLBACK(toggle_hdlr),
                (gpointer)0);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), 1);
        }
    }
    else if (GTK_IS_CHECK_MENU_ITEM(btn)) {
        if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(btn))) {
            g_signal_connect(G_OBJECT(btn), "toggled", G_CALLBACK(toggle_hdlr),
                (gpointer)0);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(btn), 1);
        }
    }
}


// Return the status of btn.
//
bool
GTKdev::GetStatus(GRobject btn)
{
    if (!btn)
        return (false);
    if (GTK_IS_TOGGLE_BUTTON(btn))
        return (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn)));
    if (GTK_IS_CHECK_MENU_ITEM(btn))
        return (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(btn)));
    return (true);
}


// Set the status of btn, subbress the signal.
//
void
GTKdev::SetStatus(GRobject btn, bool state)
{
    if (!btn)
        return;
    if (GTK_IS_TOGGLE_BUTTON(btn)) {
        bool cur = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn));
        if (cur != state) {
            g_signal_connect(G_OBJECT(btn), "toggled", G_CALLBACK(toggle_hdlr),
                (gpointer)0);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), state);
        }
    }
    else if (GTK_IS_CHECK_MENU_ITEM(btn)) {
        bool cur = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(btn));
        if (cur != state) {
            g_signal_connect(G_OBJECT(btn), "toggled", G_CALLBACK(toggle_hdlr),
                (gpointer)0);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(btn), state);
        }
    }
}


void
GTKdev::CallCallback(GRobject obj)
{
    if (!obj)
        return;
    if (!IsSensitive(obj))
        return;
    if (GTK_IS_BUTTON(obj) || GTK_IS_TOGGLE_BUTTON(obj) ||
            GTK_IS_RADIO_BUTTON(obj))
        gtk_button_clicked(GTK_BUTTON(obj));
    else if (GTK_IS_MENU_ITEM(obj) || GTK_IS_CHECK_MENU_ITEM(obj) ||
            GTK_IS_RADIO_MENU_ITEM(obj))
        gtk_menu_item_activate(GTK_MENU_ITEM(obj));
}


// Return the root window coordinates x+width, y of obj.
//
void
GTKdev::Location(GRobject obj, int *x, int *y)
{
    *x = 0;
    *y = 0;
    if (obj && gtk_widget_get_window(GTK_WIDGET(obj))) {
        GdkWindow *wnd = gtk_widget_get_parent_window(GTK_WIDGET(obj));
        int rx, ry;
        gdk_window_get_origin(wnd, &rx, &ry);
        GtkAllocation a;
        gtk_widget_get_allocation(GTK_WIDGET(obj), &a);
        *x = rx + a.x + a.width;
        *y = ry + a.y;
        return;
    }
    GtkWidget *w = GTK_WIDGET(obj);
    while (w && gtk_widget_get_parent(w))
        w = gtk_widget_get_parent(w);
    if (gtk_widget_get_window(w)) {
        int rx, ry;
        gdk_window_get_origin(gtk_widget_get_window(w), &rx, &ry);
        if (x)
            *x = rx + 50;
        if (y)
            *y = ry + 50;
        return;
    }
    if (dv_default_focus_win) {
        int rx, ry;
        gdk_window_get_origin(dv_default_focus_win, &rx, &ry);
        if (x)
            *x = rx + 100;
        if (y)
            *y = ry + 300;
    }
}


// Return the pointer position in root window coordinates
//
void
GTKdev::PointerRootLoc(int *x, int *y)
{
    GdkModifierType state;
    gdk_window_get_pointer(0, x, y, &state);
}


// Return the label string of the button, accelerators stripped.  Do not
// free the return.
//
const char *
GTKdev::GetLabel(GRobject btn)
{
    if (!btn)
        return (0);
    const char *string = 0;
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(btn));
    if (!child)
        return (0);
    if (GTK_IS_LABEL(child))
        string = gtk_label_get_label(GTK_LABEL(child));
    else if (GTK_IS_CONTAINER(child)) {
        GList *stuff = gtk_container_get_children(GTK_CONTAINER(child));
        for (GList *a = stuff; a; a = a->next) {
            GtkWidget *item = (GtkWidget*)a->data;
            if (GTK_IS_LABEL(item)) {
                string = gtk_label_get_label(GTK_LABEL(item));
                break;
            }
        }
        g_list_free(stuff);
    }
    return (string);
}


// Set the label of the button
//
void
GTKdev::SetLabel(GRobject btn, const char *text)
{
    if (!btn)
        return;
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(btn));
    if (!child)
        return;
    if (GTK_IS_LABEL(child))
        gtk_label_set_text(GTK_LABEL(child), text);
    else if (GTK_IS_CONTAINER(child)) {
        GList *stuff = gtk_container_get_children(GTK_CONTAINER(child));
        for (GList *a = stuff; a; a = a->next) {
            GtkWidget *item = (GtkWidget*)a->data;
            if (GTK_IS_LABEL(item)) {
                gtk_label_set_text(GTK_LABEL(item), text);
                break;
            }
        }
        g_list_free(stuff);
    }
}


void
GTKdev::SetSensitive(GRobject obj, bool sens_state)
{
    if (obj)
        gtk_widget_set_sensitive((GtkWidget*)obj, sens_state);
}


bool
GTKdev::IsSensitive(GRobject obj)
{
    if (!obj)
        return (false);
    GtkWidget *w = GTK_WIDGET(obj);
    while (w) {
        if (!gtk_widget_get_sensitive(w))
            return (false);
        w = gtk_widget_get_parent(w);
    }
    return (true);
}


void
GTKdev::SetVisible(GRobject obj, bool vis_state)
{
    if (obj) {
        if (vis_state)
            gtk_widget_show((GtkWidget*)obj);
        else
            gtk_widget_hide((GtkWidget*)obj);
    }
}


bool
GTKdev::IsVisible(GRobject obj)
{
    if (!obj)
        return (false);
    GtkWidget *w = GTK_WIDGET(obj);
    while (w) {
        if (!gtk_widget_get_mapped(w))
            return (false);
        w = gtk_widget_get_parent(w);
    }
    return (true);
}


void
GTKdev::DestroyButton(GRobject obj)
{
    if (obj)
        gtk_widget_destroy(GTK_WIDGET(obj));
}
// End of GTKdev functions


//-----------------------------------------------------------------------------
// sGdraw functions, ghost rendering

// Set up ghost drawing.  Whenever the pointer moves, the callback is
// called with the current position and the x, y reference.
//
void
sGdraw::set_ghost(GhostDrawFunc callback, int x, int y)
{
    if (callback) {
        gd_draw_ghost = callback;
        gd_ref_x = x;
        gd_ref_y = y;
        gd_last_x = 0;
        gd_last_y = 0;
        gd_ghost_cx_cnt = 0;
        gd_first_ghost = true;
        gd_show_ghost = true;
        gd_undraw = false;
        return;
    }
    if (gd_draw_ghost) {
        if (!gd_first_ghost) {
            // undraw last
            gd_linedb = new GRlineDb;
            (*gd_draw_ghost)(gd_last_x, gd_last_y, gd_ref_x, gd_ref_y,
                gd_undraw);
            delete gd_linedb;
            gd_linedb = 0;
            gd_undraw ^= true;
        }
        gd_draw_ghost = 0;
    }
}


// Below, show_ghost is called after every display refresh.  If the
// warp pointer call is made directly, when dragging a window across
// the main drawing area, this causes ugly things to happen to the
// display, particularly when running over a network.  Therefor we put
// the warp pointer call in a timeout which will cut the call
// frequency way down.
//
namespace {
    int ghost_timer_id;

    int ghost_timeout(void*)
    {
        ghost_timer_id = 0;

        // This redraws ghost objects.
        int x0, y0;
        GdkScreen *screen;
        GdkDisplay *display = gdk_display_get_default();
        gdk_display_get_pointer(display, &screen, &x0, &y0, 0);
        gdk_display_warp_pointer(display, screen, x0, y0);
        return (0);
    }
}


// Turn on/off display of ghosting.  Keep track of calls in ghostcxcnt.
//
void
sGdraw::show_ghost(bool show)
{
    if (!show) {
        if (!gd_ghost_cx_cnt) {
            undraw_ghost(false);
            gd_show_ghost = false;
            gd_first_ghost = true;
        }
        gd_ghost_cx_cnt++;
    }
    else {
        if (gd_ghost_cx_cnt)
            gd_ghost_cx_cnt--;
        if (!gd_ghost_cx_cnt) {
            gd_show_ghost = true;

            // The warp_pointer call mungs things if called too
            // frequently, so we put it in a timeout.

            if (ghost_timer_id)
                g_source_remove(ghost_timer_id);
            ghost_timer_id = g_timeout_add(100, ghost_timeout, 0);
        }
    }
}


// Erase the last ghost.
//
void
sGdraw::undraw_ghost(bool reset)
{
    if (gd_draw_ghost && gd_show_ghost && gd_undraw && !gd_first_ghost) {
        gd_linedb = new GRlineDb;
        (*gd_draw_ghost)(gd_last_x, gd_last_y, gd_ref_x, gd_ref_y, gd_undraw);
        delete gd_linedb;
        gd_linedb = 0;
        gd_undraw ^= true;
        if (reset)
            gd_first_ghost = true;
    }
}


// Draw a ghost at x, y.
//
void
sGdraw::draw_ghost(int x, int y)
{
    if (gd_draw_ghost && gd_show_ghost && !gd_undraw) {
        gd_last_x = x;
        gd_last_y = y;
        gd_linedb = new GRlineDb;
        (*gd_draw_ghost)(x, y, gd_ref_x, gd_ref_y, gd_undraw);
        delete gd_linedb;
        gd_linedb = 0;
        gd_undraw ^= true;
        gd_first_ghost = false;
    }
}
// End of sGbag functions


//-----------------------------------------------------------------------------
// gtk_bag methods

const char *gtk_bag::wb_open_folder_xpm[] = {
    "16 16 12 1",
    "   c None",
    ".  c #808080",
    "+  c #E0E0D0",
    "@  c #4F484F",
    "#  c #909000",
    "$  c #FFF8EF",
    "%  c #CFC860",
    "&  c #003090",
    "*  c #7F7800",
    "=  c #FFC890",
    "-  c #FFF890",
    ";  c #2F3000",
    "        .       ",
    "       .+@      ",
    "   ###.$$+@     ",
    "  #%%.$$$$+@    ",
    "  #%.$$$&$$+@** ",
    "  #.+++&+&+++@* ",
    "############++@ ",
    "#$$$$$$$$$=%#++@",
    "#$-------=-=#+; ",
    " #---=--=-==%#; ",
    " #-----=-=-==#; ",
    " #-=--=-=-=-=#; ",
    "  #=-=-=-=-==#; ",
    "  ############; ",
    "   ;;;;;;;;;;;  ",
    "                "
};

const char *gtk_bag::wb_closed_folder_xpm[] = {
    "16 16 8 1",
    "   c None",
    ".  c #909000",
    "+  c #000000",
    "@  c #EFE8EF",
    "#  c #FFF8CF",
    "$  c #FFF890",
    "%  c #CFC860",
    "&  c #FFC890",
    "                ",
    "  .....+        ",
    " .@##$$.+       ",
    ".%%%%%%%......  ",
    ".###########$%+ ",
    ".#$$$$$$$$$$&%+ ",
    ".#$$$$$$$&$&$%+ ",
    ".#$$$$$$$$&$&%+ ",
    ".#$$$$$&$&$&$%+ ",
    ".#$$$$$$&$&$&%+ ",
    ".#$$$&$&$&$&&%+ ",
    ".#&$&$&$&$&&&%+ ",
    ".%%%%%%%%%%%%%+ ",
    " ++++++++++++++ ",
    "                ",
    "                "
};

gtk_bag::gtk_bag()
{
    wb_shell = 0;
    wb_textarea = 0;
    wb_input = 0;
    wb_message = 0;
    wb_info = 0;
    wb_info2 = 0;
    wb_htinfo = 0;
    wb_warning = 0;
    wb_error = 0;
    wb_fontsel = 0;
    wb_hc = 0;
    wb_call_data = 0;
    wb_sens_set = 0;
    wb_warn_cnt = 1;
    wb_err_cnt = 1;
    wb_info_cnt = 1;
    wb_info2_cnt = 1;
    wb_htinfo_cnt = 1;
}


gtk_bag::~gtk_bag()
{
    ClearPopups();
    HcopyDisableMsgs();
    if (wb_hc)
        wb_hc->pop_down_print(this);
    if (wb_shell)
        gtk_widget_destroy(wb_shell);
    if (this == GRX->MainFrame())
        GRX->RegisterMainFrame(0);

}
// End of gtk_bag functions.


//
// Some GTK-specific convenience functions.
//

namespace {
    // Override the GtkWindow key event handling so that unhandled events
    // propagate to the main application window.
    //
    int(*key_dn_ev)(GtkWidget*, GdkEventKey*);
    int(*key_up_ev)(GtkWidget*, GdkEventKey*);

    int
    key_down_event(GtkWidget *widget, GdkEventKey *event)
    {
        int x = (*key_dn_ev)(widget, event);

        // Don't propagate modifiers, these can cause focus change to
        // the main window in the GTK-2 in RHEL6.
        switch (event->keyval) {
        case GDK_Shift_L:
        case GDK_Shift_R:
        case GDK_Control_L:
        case GDK_Control_R:
            return (x);
        default:
            break;
        }

        if (g_object_get_data(G_OBJECT(widget), "no_prop_key"))
            return (x);
        if (!x && GRX->MainFrame() && widget != GRX->MainFrame()->Shell()) {
            gtk_propagate_event(GRX->MainFrame()->Shell(), (GdkEvent*)event);

            // Revert focus to main window.  Terminating Enter press will
            // then go to main window.
            GRX->SetFocus(GRX->MainFrame()->Shell());

            return (1);
        }
        return (x);
    }


    int
    key_up_event(GtkWidget *widget, GdkEventKey *event)
    {
        int x = (*key_up_ev)(widget, event);

        switch (event->keyval) {
        case GDK_Shift_L:
        case GDK_Shift_R:
        case GDK_Control_L:
        case GDK_Control_R:
            return (x);
        default:
            break;
        }

        if (g_object_get_data(G_OBJECT(widget), "no_prop_key"))
            return (x);
        if (!x && GRX->MainFrame() && widget != GRX->MainFrame()->Shell()) {
            gtk_propagate_event(GRX->MainFrame()->Shell(), (GdkEvent*)event);
            return (1);
        }
        return (x);
    }


    // The "delete-event" (WM_DESTROY) handler, calls the quit
    // procedure.  This will be called once only in response to a
    // destroy window message from the window manager.
    //
    bool
    delete_event_hdlr(GtkWidget *widget, GdkEvent*, void *arg)
    {
        void (*cb)(GtkWidget*, void*) =
            (void(*)(GtkWidget*, void*))g_object_get_data(
                G_OBJECT(widget), "delete_ev_cb");
        if (cb) {
            (*cb)(widget, arg);
            // The callback takes care of deleting the window (or
            // not), the event is ignored.
            return (true);
        }
        return (false);
    }
}


// Create a new popup window, and connect to signals.
//
GtkWidget *
gtkinterf::gtk_NewPopup(gtk_bag *w, const char *title,
    void(*quit_cb)(GtkWidget*, void*), void *arg)
{
    GtkWidget *popup;
    if (w && w->wb_shell && w->IsSetTopLevel()) {
        // When this is set, use the existing shell, so that the
        // pop-up becomes an application window.  w can be deleted,
        // but it isn't safe to do it here.
        popup = w->wb_shell;
        w->wb_shell = 0;
    }
    else {
        popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_widget_set_name(popup, title);

        // This will send unhandled key events to the main application
        // window.
        static bool keyprop_init;
        if (GRX->MainFrame() && !keyprop_init) {
            GtkWidgetClass *ks = GTK_WIDGET_GET_CLASS(popup);
            key_dn_ev = ks->key_press_event;
            ks->key_press_event = key_down_event;
            key_up_ev = ks->key_release_event;
            ks->key_release_event = key_up_event;
            keyprop_init = true;
        }
        gtk_widget_add_events(popup, GDK_VISIBILITY_NOTIFY_MASK);
        g_signal_connect(G_OBJECT(popup), "visibility-notify-event",
            G_CALLBACK(ToTop), w ? w->Shell() : 0);
        gtk_widget_add_events(popup, GDK_BUTTON_PRESS_MASK);
        g_signal_connect_after(G_OBJECT(popup), "button-press-event",
            G_CALLBACK(Btn1MoveHdlr), 0);
    }
    if (title)
        gtk_window_set_title(GTK_WINDOW(popup), title);
    if (quit_cb) {
        g_object_set_data(G_OBJECT(popup), "delete_ev_cb",
            (void*)quit_cb);
        g_signal_connect(G_OBJECT(popup), "delete-event",
            G_CALLBACK(delete_event_hdlr), arg ? arg : popup);
        g_signal_connect(G_OBJECT(popup), "destroy",
            G_CALLBACK(quit_cb), arg ? arg : popup);
    }
    return (popup);
}


// A QueryColor function.
//
void
gtkinterf::gtk_QueryColor(GdkColor *clr)
{
    gdk_colormap_query_color(GRX->Colormap(), clr->pixel, clr);
}


// Set the RGB values and allocate the pixel from cname.  This supports
// "R G B" triples as well as the usual.  Returns true on success.
//
bool
gtkinterf::gtk_ColorSet(GdkColor *clr, const char *cname)
{
    if (cname && *cname) {
        int r, g, b;
        if (sscanf(cname, "%d %d %d", &r, &g, &b) == 3) {
            if (r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
                    b >= 0 && b <= 255) {
                clr->red = r*256;
                clr->green = g*256;
                clr->blue = b*256;
            }
            else if (r >= 0 && r <= 65535 && g >= 0 && g <= 65535 &&
                    b >= 0 && b <= 65535) {
                clr->red = r;
                clr->green = g;
                clr->blue = b;
            }
            else
                return (false);
        }
        else if (!gdk_color_parse(cname, clr))
            return (false);
        GdkColormap *cmap = GRX ? GRX->Colormap() : gdk_colormap_get_system();
        if (gdk_colormap_alloc_color(cmap, clr, false, true))
            return (true);
    }
    return (false);
}


// Return a GdkColor pointer, initialized with the given attr color.
//
GdkColor *
gtkinterf::gtk_PopupColor(GRattrColor c)
{
    static GdkColor pop_colors[GRattrColorEnd + 1];

    const char *colorname = GRpkgIf()->GetAttrColor(c);
    if (gtk_ColorSet(pop_colors + c, colorname))
        return (pop_colors + c);

    // Color name parse error, return black.
    GdkColor *blk = &pop_colors[GRattrColorEnd];
    blk->red = 0;
    blk->green = 0;
    blk->blue = 0;
    blk->pixel = 0;
    return (blk);
}

