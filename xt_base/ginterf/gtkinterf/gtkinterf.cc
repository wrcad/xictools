
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
#include "texttf.h"
#include "grlinedb.h"
#include "lstring.h"
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

#include <gdk/gdkkeysyms.h>
#ifdef WITH_X11
#include "gtkx11.h"
#include <X11/Xproto.h>
#ifdef HAVE_SHMGET
#include <X11/extensions/XShm.h>  
#endif
#endif


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


namespace {
    // colormap info, set in InitColormap()
    struct sColorAlloc
    {
        unsigned long plane_mask[1];
        unsigned long drawing_pixels[64];
        int num_allocated;       // number private single plane cells allocated
        int num_mask_allocated;  // number private dual plane cells allocated
        bool no_alloc;           // set of no private colors anywhere
    } ColorAlloc;


    // Graphics context storage.  The 0 element is the default.
    //
#define NUMGCS 10
    sGbag *app_gbags[NUMGCS];
}


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

#if GTK_CHECK_VERSION(2,10,0)
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
#endif
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
    XGetInputFocus(gdk_display, &xwin, &foc);
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
        gtk_object_ref(GTK_OBJECT(entry));
        gtk_widget_destroy(entry);
        g_object_unref(entry);
    }

#if GTK_CHECK_VERSION(2,10,0)
    // In GTK-2.10.4 (RHEL5), there is a spurious(?) g_print message
    // when dragging (using native handler) from a GtkTextView.
    old_print_handler = g_set_print_handler(new_print_handler);
#endif

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
    switch (dv_visual->type) {
    case GDK_VISUAL_STATIC_GRAY:
        printf(msg, "static gray", dv_visual->depth);
        dv_true_color = true;
        ColorAlloc.no_alloc = true;
        return (false);
    case GDK_VISUAL_STATIC_COLOR:
        printf(msg, "static color", dv_visual->depth);
        dv_true_color = true;
        ColorAlloc.no_alloc = true;
        return (false);
    case GDK_VISUAL_GRAYSCALE:
        printf(msg, "grayscale", dv_visual->depth);
        dv_true_color = false;
        break;
    case GDK_VISUAL_PSEUDO_COLOR:
        printf(msg, "pseudo color", dv_visual->depth);
        dv_true_color = false;
        break;
    case GDK_VISUAL_TRUE_COLOR:
        printf(msg, "true color", dv_visual->depth);
        dv_true_color = true;
        ColorAlloc.no_alloc = true;
        return (false);
    case GDK_VISUAL_DIRECT_COLOR:
        printf(msg, "direct color", dv_visual->depth);
        dv_true_color = true;
        ColorAlloc.no_alloc = true;
        return (false);
    }
    int ncolors = dv_visual->colormap_size;

    // limit allocation to 48 dual-plane cells
    if (ncolors > 96)
        ncolors = 96;

    if (dualplane) {
        if (2*maxcolors > ncolors)
            maxcolors = ncolors/2;
    }
    else {
        if (maxcolors > ncolors)
            maxcolors = ncolors;
    }
    if (maxcolors < mincolors) {
        fprintf(stderr, "Fatal error: Too many colors requested.\n");
        return (true);
    }

    int n = maxcolors;
    int ok = false;
    while (n >= 16) {
        if (dualplane)
            ok = gdk_colors_alloc(dv_cmap, false,
                ColorAlloc.plane_mask, 1, ColorAlloc.drawing_pixels, n);
        else
            ok = gdk_colors_alloc(dv_cmap, false,
                0, 0, ColorAlloc.drawing_pixels, n);
        if (ok)
            break;
        n -= 4;
    }
    if (!ok) {
        // Failed to allocate colors from default colormap.  Create
        // a new colormap and try again.
        printf("Allocating colormap.\n");
        n = maxcolors;
        GdkColormap *newcmap = gdk_colormap_new(dv_visual, false);
        ncolors = dv_visual->colormap_size;
        ncolors -= (dualplane ? 2*maxcolors : maxcolors);
        // ncolors is number of unallocated colorcells.  Allocate half
        // for friendliness with default colormap, and leave half
        // unallocated.
        ncolors /= 2;
        if (ncolors > 0) {
            unsigned long xx[256];
            gdk_colors_alloc(newcmap, true, 0, 0, xx, ncolors);
            for (int i = 0; i < ncolors; i++) {
                GdkColor clr;
                clr.pixel = i;
                gtk_QueryColor(&clr);
                gdk_color_change(newcmap, &clr);
            }
        }
        dv_cmap = newcmap;
        gtk_widget_set_default_colormap(dv_cmap);
        while (n >= mincolors) {
            if (dualplane)
                ok = gdk_colors_alloc(dv_cmap, false,
                    ColorAlloc.plane_mask, 1, ColorAlloc.drawing_pixels, n);
            else
                ok = gdk_colors_alloc(dv_cmap, false,
                    0, 0, ColorAlloc.drawing_pixels, n);
            if (ok)
                break;
            n -= 4;
        }
        if (!ok) {
            fprintf(stderr,
                "Fatal error: Can't allocate %d read/write colorcells.\n",
                mincolors);
            return (true);
        }
    }

    if (dualplane) {
        ColorAlloc.num_mask_allocated = n;
        printf("Allocating %d dual-plane private colorcells.\n", n);
        dv_dual_plane = true;
    }
    else {
        ColorAlloc.num_allocated = n;
        printf("Allocating %d single-plane private colorcells.\n", n);
    }
    return (false);
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
        gdk_color_change(dv_cmap, &newcolor);
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
    if (gdk_color_black(Colormap(), &c))
        return (c.pixel);
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
        if (!app_gbags[apptype])
            app_gbags[apptype] = new sGbag;
        w->SetGbag(app_gbags[apptype]);
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
    gtk_window_set_policy(GTK_WINDOW(w->Shell()), true, true, false);
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
    return (gtk_timeout_add(msecs, (GtkFunction)callback, arg));
}


// Remove installed timer.
//
void
GTKdev::RemoveTimer(int id)
{
    gtk_timeout_remove(id);
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
                (ev->key.keyval == GDK_c || ev->key.keyval == GDK_C) &&
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
                    if ((kev->keyval == GDK_c || kev->keyval == GDK_C) &&
                            (kev->state & GDK_CONTROL_MASK)) {
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


// Set the state of btn to unselected
//
void
GTKdev::Deselect(GRobject btn)
{
    if (!btn)
        return;
    if (GTK_IS_TOGGLE_BUTTON(btn)) {
        GtkToggleButton *toggle = (GtkToggleButton*)btn;
        if (toggle->active) {
            toggle->active = 0;
            toggle->button.depressed = false;
            GtkStateType new_state = (GTK_BUTTON(toggle)->in_button ?
                GTK_STATE_PRELIGHT : GTK_STATE_NORMAL);
            gtk_widget_set_state(GTK_WIDGET(toggle), new_state);
        }
    }
    else if (GTK_IS_CHECK_MENU_ITEM(btn)) {
        GTK_CHECK_MENU_ITEM(btn)->active = 0;
    }
}


// Set the state of btn to selected
//
void
GTKdev::Select(GRobject btn)
{
    if (!btn)
        return;
    if (GTK_IS_TOGGLE_BUTTON(btn)) {
        GtkToggleButton *toggle = (GtkToggleButton*)btn;
        if (!toggle->active) {
            toggle->active = 1;
            toggle->button.depressed = true;
            GtkStateType new_state = (GTK_BUTTON(toggle)->in_button ?
                GTK_STATE_PRELIGHT : GTK_STATE_ACTIVE);
            gtk_widget_set_state(GTK_WIDGET(toggle), new_state);
        }
    }
    else if (GTK_IS_CHECK_MENU_ITEM(btn)) {
        GTK_CHECK_MENU_ITEM(btn)->active = 1;
    }
}


// Return the status of btn
//
bool
GTKdev::GetStatus(GRobject btn)
{
    if (!btn)
        return (false);
    if (GTK_IS_TOGGLE_BUTTON(btn))
        return (GTK_TOGGLE_BUTTON(btn)->active);
    if (GTK_IS_CHECK_MENU_ITEM(btn))
        return (GTK_CHECK_MENU_ITEM(btn)->active);
    return (true);
}


// Set the status of btn
//
void
GTKdev::SetStatus(GRobject btn, bool state)
{
    if (!btn)
        return;
    if (GTK_IS_TOGGLE_BUTTON(btn)) {
        GtkToggleButton *toggle = (GtkToggleButton*)btn;
        if (toggle->active && !state) {
            toggle->active = false;
            toggle->button.depressed = false;
            GtkStateType new_state = (GTK_BUTTON(toggle)->in_button ?
                GTK_STATE_PRELIGHT : GTK_STATE_NORMAL);
            gtk_widget_set_state(GTK_WIDGET(toggle), new_state);
        }
        else if (!toggle->active && state) {
            toggle->active = true;
            toggle->button.depressed = true;
            GtkStateType new_state = (GTK_BUTTON(toggle)->in_button ?
                GTK_STATE_PRELIGHT : GTK_STATE_ACTIVE);
            gtk_widget_set_state(GTK_WIDGET(toggle), new_state);
        }
    }
    else if (GTK_IS_CHECK_MENU_ITEM(btn)) {
        GTK_CHECK_MENU_ITEM(btn)->active = state;
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
    if (obj && GTK_WIDGET(obj)->window) {
        GdkWindow *wnd = gtk_widget_get_parent_window(GTK_WIDGET(obj));
        int rx, ry;
        gdk_window_get_origin(wnd, &rx, &ry);
        GtkAllocation *a = &GTK_WIDGET(obj)->allocation;
        *x = rx + a->x + a->width;
        *y = ry + a->y;
        return;
    }
    GtkWidget *w = GTK_WIDGET(obj);
    while (w && w->parent)
        w = w->parent;
    if (w->window) {
        int rx, ry;
        gdk_window_get_origin(w->window, &rx, &ry);
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


// Return the label string of the button
//
char *
GTKdev::GetLabel(GRobject btn)
{
    if (!btn)
        return (0);
    char *string = 0;
    GtkWidget *child = GTK_BIN(btn)->child;
    if (!child)
        return (0);
    if (GTK_IS_LABEL(child))
        gtk_label_get(GTK_LABEL(child), &string);
    else if (GTK_IS_CONTAINER(child)) {
        GList *stuff = gtk_container_children(GTK_CONTAINER(child));
        for (GList *a = stuff; a; a = a->next) {
            GtkWidget *item = (GtkWidget*)a->data;
            if (GTK_IS_LABEL(item)) {
                gtk_label_get(GTK_LABEL(item), &string);
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
    GtkWidget *child = GTK_BIN(btn)->child;
    if (!child)
        return;
    if (GTK_IS_LABEL(child))
        gtk_label_set_text(GTK_LABEL(child), text);
    else if (GTK_IS_CONTAINER(child)) {
        GList *stuff = gtk_container_children(GTK_CONTAINER(child));
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
        if (!GTK_WIDGET_SENSITIVE(w))
            return (false);
        w = w->parent;
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
        if (!GTK_WIDGET_MAPPED(w))
            return (false);
        w = w->parent;
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
#if GTK_CHECK_VERSION(2,8,0)
        int x0, y0;
        GdkScreen *screen;
        GdkDisplay *display = gdk_display_get_default();
        gdk_display_get_pointer(display, &screen, &x0, &y0, 0);
        gdk_display_warp_pointer(display, screen, x0, y0);
#else
#ifdef WITH_X11
        XWarpPointer(gr_x_display(), None, None, 0, 0, 0, 0, 0, 0);
#endif
#endif
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
                gtk_timeout_remove(ghost_timer_id);
            ghost_timer_id = gtk_timeout_add(100, ghost_timeout, 0);
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
// gtk_draw functions

gtk_draw::gtk_draw(int type)
{
    gd_viewport = 0;
    gd_window = 0;
    gd_gbag = sGbag::default_gbag(type);
    gd_backg = 0;
    gd_foreg = (unsigned int)-1;
}


gtk_draw::~gtk_draw()
{
    if (gd_gbag) {
        // Unless the gbag is in the array, free it.
        int i;
        for (i = 0; i < NUMGCS; i++) {
            if (app_gbags[i] == gd_gbag)
                break;
        }
        if (i == NUMGCS)
            delete gd_gbag;
    }
}


void
gtk_draw::Halt()
{
    // Applies to hard copy drivers only.
}


// Clear the drawing window.
//
void
gtk_draw::Clear()
{
    if (!GDK_IS_PIXMAP(gd_window)) {
        // doesn't work for pixmaps
        gdk_window_clear(gd_window);
    }
    else {
        int w, h;
        gdk_window_get_size(gd_window, &w, &h);
        gdk_draw_rectangle(gd_window, GC(), true, 0, 0, w, h);
    }
}


#if defined(WIN32) && defined(DIRECT_TO_GDI)
// The current gdk-win32 does not handle stippled rendering.  Thus, we
// do our own rendering using native Win32 functions.  At least for
// now, we leave line and text rendering to gdk.

namespace {
    inline HBITMAP get_bitmap(sGbag *gb)
    {
        const GRfillType *fp = gb->get_fillpattern();
        if (fp)
            return ((HBITMAP)fp->xPixmap());
        return (0);
    }

    GdkGCValuesMask Win32GCvalues = (GdkGCValuesMask)(
        GDK_GC_FOREGROUND |
        GDK_GC_BACKGROUND);

    POINT *PTbuf;
    int PTbufSz;
}
#endif


// Draw a pixel at x, y.
//
void
gtk_draw::Pixel(int x, int y)
{
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        x -= xoff;
        y -= yoff;
    }

    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);
    HBITMAP bmap = get_bitmap(gd_gbag);
    unsigned rop = Gbag()->get_gc() == Gbag()->get_xorgc() ?
        PATINVERT : PATCOPY;
    if (bmap) {
        HBRUSH brush = CreatePatternBrush(bmap);
        brush = SelectBrush(dc, brush);

        PatBlt(dc, x, y, 1, 1, rop);

        brush = SelectBrush(dc, brush);
        if (brush)
            DeleteBrush(brush);
    }
    else
        PatBlt(dc, x, y, 1, 1, rop);
    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);

#else
#ifdef DIRECT_TO_X
    XDrawPoint(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()), x, y);
#else
    gdk_draw_point(gd_window, GC(), x, y);
#endif
#endif
}


// Draw multiple pixels from list.
//
void
gtk_draw::Pixels(GRmultiPt *data, int n)
{
    if (n < 1)
        return;
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    POINT *p = (POINT*)data->data();
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        if (xoff | yoff) {
            if (n > PTbufSz) {
                delete [] PTbuf;
                PTbufSz = n + 32;
                PTbuf = new POINT[PTbufSz];
            }
            for (int i = 0; i < n; i++) {
                PTbuf[i].x = p[i].x - xoff;
                PTbuf[i].y = p[i].y - yoff;
            }
            p = PTbuf;
        }
    }
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);
    HBITMAP bmap = get_bitmap(gd_gbag);

    // This seems a bit faster than SetPixelV.
    unsigned rop = Gbag()->get_gc() == Gbag()->get_xorgc() ?
        PATINVERT : PATCOPY;
    if (bmap) {
        HBRUSH brush = CreatePatternBrush(bmap);
        brush = SelectBrush(dc, brush);

        for (int i = 0; i < n; i++) {
            if (!i || p->x != (p-1)->x || p->y != (p-1)->y)
                PatBlt(dc, p->x, p->y, 1, 1, rop);
            p++;
        }

        brush = SelectBrush(dc, brush);
        if (brush)
            DeleteBrush(brush);
    }
    else {
        for (int i = 0; i < n; i++) {
            if (!i || p->x != (p-1)->x || p->y != (p-1)->y)
                PatBlt(dc, p->x, p->y, 1, 1, rop);
            p++;
        }
    }
    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);

#else
#ifdef DIRECT_TO_X
    XDrawPoints(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        (XPoint*)data->data(), n, CoordModeOrigin);
#else
    gdk_draw_points(gd_window, GC(), (GdkPoint*)data->data(), n);
#endif
#endif
}


// Draw a line.
//
void
gtk_draw::Line(int x1, int y1, int x2, int y2)
{
    /***** Example native line draw
    static GdkGCValuesMask LineGCvalues = (GdkGCValuesMask)(
        GDK_GC_FOREGROUND |
        GDK_GC_BACKGROUND |
        GDK_GC_LINE_WIDTH |
        GDK_GC_LINE_STYLE |
        GDK_GC_CAP_STYLE |
        GDK_GC_JOIN_STYLE);

    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        if (xoff | yoff) {
            x1 -= xoff;
            y1 -= yoff;
            x2 -= xoff;
            y2 -= yoff;
        }
    }
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), LineGCvalues);
    MoveToEx(dc, x1, y1, 0);
    LineTo(dc, x2, y2);
    gdk_win32_hdc_release(gd_window, GC(), LineGCvalues);
    return;
    *****/

    if (XorLineDb()) {
        // We are drawing in XOR mode, filter the Manhattan lines so
        // none is overdrawn.

        // Must draw vertical lines first to properly handle single-pixel
        // "lines".
        if (x1 == x2) {
            const llist_t *ll = XorLineDb()->add_vert(x1, y1, y2);
            while (ll) {
#ifdef DIRECT_TO_X
                XDrawLine(gr_x_display(), gr_x_window(gd_window),
                    gr_x_gc(GC()), x1, ll->vmin(), x1, ll->vmax());
#else
                gdk_draw_line(gd_window, GC(), x1, ll->vmin(), x1, ll->vmax());
#endif
                ll = ll->next();
            }
        }
        else if (y1 == y2) {
            const llist_t *ll = XorLineDb()->add_horz(y1, x1, x2);
            while (ll) {
#ifdef DIRECT_TO_X
                XDrawLine(gr_x_display(), gr_x_window(gd_window),
                    gr_x_gc(GC()), ll->vmin(), y1, ll->vmax(), y1);
#else
                gdk_draw_line(gd_window, GC(), ll->vmin(), y1, ll->vmax(), y1);
#endif
                ll = ll->next();
            }
        }
        else {
            const nmllist_t *ll = XorLineDb()->add_nm(x1, y1, x2, y2);
            while (ll) {
#ifdef DIRECT_TO_X
                XDrawLine(gr_x_display(), gr_x_window(gd_window),
                    gr_x_gc(GC()), ll->x1(), ll->y1(), ll->x2(), ll->y2());
#else
                gdk_draw_line(gd_window, GC(),
                    ll->x1(), ll->y1(), ll->x2(), ll->y2());
#endif
                ll = ll->next();
            }
        }
        return;
    }

#ifdef DIRECT_TO_X
    XDrawLine(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        x1, y1, x2, y2);
#else
    gdk_draw_line(gd_window, GC(), x1, y1, x2, y2);
#endif
}


// Draw a path.
//
void
gtk_draw::PolyLine(GRmultiPt *p, int n)
{
    if (n < 2)
        return;
    if (XorLineDb()) {
        n--;
        p->data_ptr_init();
        while (n--) {
            int x = p->data_ptr_x();
            int y = p->data_ptr_y();
            p->data_ptr_inc();
            if (abs(p->data_ptr_x() - x) <= 1 &&
                    abs(p->data_ptr_y() - y) <= 1) {
                // Keep tiny features from disappearing.
                Line(p->data_ptr_x(), p->data_ptr_y(),
                    p->data_ptr_x(), p->data_ptr_y());
            }
            else
                Line(x, y, p->data_ptr_x(), p->data_ptr_y());
        }
        return;
    }

#ifdef DIRECT_TO_X
    XDrawLines(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        (XPoint*)p->data(), n, CoordModeOrigin);
#else
    gdk_draw_lines(gd_window, GC(), (GdkPoint*)p->data(), n);
#endif
}


// Draw a collection of lines.
//
void
gtk_draw::Lines(GRmultiPt *p, int n)
{
    if (n < 1)
        return;
    if (XorLineDb()) {
        p->data_ptr_init();
        while (n--) {
            int x = p->data_ptr_x();
            int y = p->data_ptr_y();
            p->data_ptr_inc();
            Line(x, y, p->data_ptr_x(), p->data_ptr_y());
            p->data_ptr_inc();
        }
        return;
    }

#ifdef DIRECT_TO_X
    XDrawSegments(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        (XSegment*)p->data(), n);
#else
    gdk_draw_segments(gd_window, GC(), (GdkSegment*)p->data(), n);
#endif
}


// Draw a filled box.
//
void
gtk_draw::Box(int x1, int y1, int x2, int y2)
{
    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
    }
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        x1 -= xoff;
        x2 -= xoff;
        y1 -= yoff;
        y2 -= yoff;
    }

    x2++;
    y2++;
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);
    HBITMAP bmap = get_bitmap(gd_gbag);
    if (bmap) {
        HBRUSH brush = CreatePatternBrush(bmap);
        brush = SelectBrush(dc, brush);

        COLORREF fg = SetTextColor(dc, 0);
        COLORREF bg = SetBkColor(dc, RGB(255, 255, 255));
        // D <- P & D
        PatBlt(dc, x1, y1, x2-x1, y2-y1, 0xa000c9);
        SetBkColor(dc, 0);
        SetTextColor(dc, fg);
        // D <- P | D
        PatBlt(dc, x1, y1, x2-x1, y2-y1, 0xfa0089);
        SetBkColor(dc, bg);

        brush = SelectBrush(dc, brush);
        if (brush)
            DeleteBrush(brush);
    }
    else
        PatBlt(dc, x1, y1, x2-x1, y2-y1, PATCOPY);
    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);

#else
#ifdef DIRECT_TO_X
    XFillRectangle(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        x1, y1, x2-x1 + 1, y2-y1 + 1);
#else
    gdk_draw_rectangle(gd_window, GC(), true, x1, y1, x2-x1 + 1, y2-y1 + 1);
#endif
#endif
}


// Draw a collection of filled boxes.
//
void
gtk_draw::Boxes(GRmultiPt *data, int n)
{
    if (n < 1)
        return;
    // order: x, y, width, height
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    POINT *points = (POINT*)data->data();
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        if (xoff | yoff) {
            int two_n = n + n;
            if (two_n > PTbufSz) {
                delete [] PTbuf;
                PTbufSz = two_n + 32;
                PTbuf = new POINT[PTbufSz];
            }
            for (int i = 0; i < two_n; i += 2) {
                PTbuf[i].x = points[i].x - xoff;
                PTbuf[i].y = points[i].y - yoff;
            }
            points = PTbuf;
        }
    }
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);
    HBITMAP bmap = get_bitmap(gd_gbag);
    if (bmap) {
        HBRUSH brush = CreatePatternBrush(bmap);
        brush = SelectBrush(dc, brush);

        COLORREF fg = SetTextColor(dc, 0);
        COLORREF bg = SetBkColor(dc, RGB(255, 255, 255));
        POINT *p = points;
        for (int i = 0; i < n; i++) {
            // D <- P & D
            PatBlt(dc, p->x, p->y, (p+1)->x, (p+1)->y, 0xa000c9);
            p += 2;
        }
        SetBkColor(dc, 0);
        SetTextColor(dc, fg);
        p = points;
        for (int i = 0; i < n; i++) {
            // D <- P | D
            PatBlt(dc, p->x, p->y, (p+1)->x, (p+1)->y, 0xfa0089);
            p += 2;
        }
        SetBkColor(dc, bg);

        brush = SelectBrush(dc, brush);
        if (brush)
            DeleteBrush(brush);
    }
    else {
        POINT *p = points;
        for (int i = 0; i < n; i++) {
            PatBlt(dc, p->x, p->y, (p+1)->x, (p+1)->y, PATCOPY);
            p += 2;
        }
    }
    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);

#else
#ifdef DIRECT_TO_X
    XFillRectangles(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        (XRectangle*)data->data(), n);
#else
    data->data_ptr_init();
    while (n--) {
        int x = data->data_ptr_x();
        int y = data->data_ptr_y();
        data->data_ptr_inc();
        Box(x, y, x + data->data_ptr_x() - 1, y + data->data_ptr_y() - 1);
        data->data_ptr_inc();
    }
#endif
#endif
}


// Draw an arc.
//
void
gtk_draw::Arc(int x0, int y0, int rx, int ry, double theta1, double theta2)
{
    if (theta1 >= theta2)
        theta2 = 2 * M_PI + theta2;
    int t1 = (int)(64 * (180.0 / M_PI) * theta1);
    int t2 = (int)(64 * (180.0 / M_PI) * theta2 - t1);
    if (t2 == 0)
        return;
    if (rx <= 0 || ry <= 0)
        return;
    int dx = 2*rx;
    int dy = 2*ry;
#ifdef DIRECT_TO_X
    XDrawArc(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        x0 - rx, y0 - ry, dx, dy, t1, t2);
#else
    gdk_draw_arc(gd_window, GC(), false, x0 - rx, y0 - ry, dx, dy, t1, t2);
#endif
}


// Draw a filled polygon.
//
void
gtk_draw::Polygon(GRmultiPt *data, int numv)
{
    if (numv < 4)
        return;
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    POINT *points = (POINT*)data->data();
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        if (xoff | yoff) {
            if (numv > PTbufSz) {
                delete [] PTbuf;
                PTbufSz = numv + 32;
                PTbuf = new POINT[PTbufSz];
            }
            for (int i = 0; i < numv; i++) {
                PTbuf[i].x = points[i].x - xoff;
                PTbuf[i].y = points[i].y - yoff;
            }
            points = PTbuf;
        }
    }
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);
    HBITMAP bmap = get_bitmap(gd_gbag);
    HRGN rgn = CreatePolygonRgn(points, numv, WINDING);
    if (bmap) {
        HBRUSH brush = CreatePatternBrush(bmap);
        brush = SelectBrush(dc, brush);

        COLORREF fg = SetTextColor(dc, 0);
        COLORREF bg = SetBkColor(dc, RGB(255, 255, 255));
        // D <- P & D
        SetROP2(dc, R2_MASKPEN);
        PaintRgn(dc, rgn);
        SetBkColor(dc, 0);
        SetTextColor(dc, fg);
        // D <- P | D
        SetROP2(dc, R2_MERGEPEN);
        PaintRgn(dc, rgn);
        SetBkColor(dc, bg);
        SetROP2(dc, R2_COPYPEN);

        brush = SelectBrush(dc, brush);
        if (brush)
            DeleteBrush(brush);
    }
    else
        PaintRgn(dc, rgn);
    DeleteRgn(rgn);
    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);

#else
#ifdef DIRECT_TO_X
    XFillPolygon(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        (XPoint*)data->data(), numv, Complex, CoordModeOrigin);
#else
    gdk_draw_polygon(gd_window, GC(), true, (GdkPoint*)data->data(), numv);
#endif
#endif
}


// Draw a trapezoid.
void
gtk_draw::Zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
{
    if (yl >= yu)
        return;
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    POINT points[5];
#else
#ifdef DIRECT_TO_X
    XPoint points[5];
#else
    GdkPoint points[5];
#endif
#endif
    int n = 0;
    points[n].x = xll;
    points[n].y = yl;
    n++;
    points[n].x = xul;
    points[n].y = yu;
    n++;
    if (xur > xul) {
        points[n].x = xur;
        points[n].y = yu;
        n++;
    }
    points[n].x = xlr;
    points[n].y = yl;
    n++;
    if (xll < xlr) {
        points[n].x = xll;
        points[n].y = yl;
        n++;
    }
    if (n < 4)
        return;

#if defined(WIN32) && defined(DIRECT_TO_GDI)
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        if (xoff | yoff) {
            for (int i = 0; i < n; i++) {
                points[i].x -= xoff;
                points[i].y -= yoff;
            }
        }
    }
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);
    HBITMAP bmap = get_bitmap(gd_gbag);
    HRGN rgn = CreatePolygonRgn(points, n, WINDING);
    if (bmap) {
        HBRUSH brush = CreatePatternBrush(bmap);
        brush = SelectBrush(dc, brush);

        COLORREF fg = SetTextColor(dc, 0);
        COLORREF bg = SetBkColor(dc, RGB(255, 255, 255));
        // D <- P & D
        SetROP2(dc, R2_MASKPEN);
        PaintRgn(dc, rgn);
        SetBkColor(dc, 0);
        SetTextColor(dc, fg);
        // D <- P | D
        SetROP2(dc, R2_MERGEPEN);
        PaintRgn(dc, rgn);
        SetBkColor(dc, bg);
        SetROP2(dc, R2_COPYPEN);

        brush = SelectBrush(dc, brush);
        if (brush)
            DeleteBrush(brush);
    }
    else
        PaintRgn(dc, rgn);
    DeleteRgn(rgn);
    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);

#else
#ifdef DIRECT_TO_X
    XFillPolygon(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
        points, n, Convex, CoordModeOrigin);
#else
    gdk_draw_polygon(gd_window, GC(), true, points, n);
#endif
#endif
}


// Render text.  Go through hoops to provide rotated/mirrored
// rendering.  The x and y are the LOWER left corner of untransformed
// text.
//
// Note: Pango now handles rotated text, update this some day.
//
// This function DOES NOT support XOR drawing.  Native X drawing would
// work, Pango does not, and the rotation code manifestly requires
// straight copy.  We have to back the GC out of XOR drawing mode for
// the duration.
//
void
gtk_draw::Text(const char *text, int x, int y, int xform, int, int)
{
    if (!text || !*text)
        return;

    // Save the GC state.
    GdkGCValues vals;
    gdk_gc_get_values(GC(), &vals);

    if (GC() == XorGC()) {
        // Set the foreground to the true ghost-drawing color, this is
        // currently the true color xor'ed with the background.
        GdkColor clr;
        clr.pixel = gd_foreg;
        gdk_gc_set_foreground(GC(), &clr);
    }

    // Switch to copy function.
    gdk_gc_set_function(GC(), GDK_COPY);

    // We need to handle strings with embedded newlines on a single
    // line.  GTK-1 does this naturally.  With Pango, one can set
    // "single paragraph" mode to achieve this.  However, the glyph
    // used as a separator is much wider than the (monospace)
    // characters, badly breaking positioning.  We don't use this
    // mode, but instead map newlines to a unicode special character
    // that will be displayed with the correct width.

    PangoLayout *lout = gtk_widget_create_pango_layout(gd_viewport, 0);
    if (strchr(text, '\n')) {
        sLstr lstr;
        const char *t = text;
        while (*t) {
            if (*t == '\n') {
                // This is the "section sign" UTF-8 character.
                lstr.add_c(0xc2);
                lstr.add_c(0xa7);
            }
            else
                lstr.add_c(*t);
            t++;
        }
        pango_layout_set_text(lout, lstr.string(), -1);
    }
    else
        pango_layout_set_text(lout, text, -1);

    int wid, hei;
    pango_layout_get_pixel_size(lout, &wid, &hei);
    if (wid <= 0 || hei <= 0) {
        g_object_unref(lout);
        // Fix up the GC as it was.
        if (GC() == XorGC())
            gdk_gc_set_foreground(GC(), &vals.foreground);
        gdk_gc_set_function(GC(), vals.function);
        return;
    }
    y -= hei;

    if (xform & (TXTF_HJC | TXTF_HJR)) {
        if (xform & TXTF_HJR)
            x -= wid;
        else
            x -= wid/2;
    }
    if (xform & (TXTF_VJC | TXTF_VJT)) {
        if (xform & TXTF_VJT)
            y += hei;
        else
            y += hei/2;
    }

    xform &= (TXTF_ROT | TXTF_MX | TXTF_MY);
    if (!xform || xform == 14) {
        // 0 no rotation, 14 MX MY R180

        gdk_draw_layout(gd_window, GC(), x, y, lout);
        g_object_unref(lout);

        // Fix up the GC as it was.
        if (GC() == XorGC())
            gdk_gc_set_foreground(GC(), &vals.foreground);
        gdk_gc_set_function(GC(), vals.function);
        return;
    }

    // We have to tranform.  Create a pixmap with a constant background,
    // and render the text into it.

    unsigned int bg_pixel;
    if (GC() == XorGC())
        bg_pixel = gd_foreg;
    else
        bg_pixel = vals.foreground.pixel;
    if (bg_pixel == 0) {
        GdkColor c;
        if (gdk_color_white(GRX->Colormap(), &c))
            bg_pixel = c.pixel;
        else
            bg_pixel = -1;
    }
    else {
        GdkColor c;
        if (gdk_color_black(GRX->Colormap(), &c))
            bg_pixel = c.pixel;
        else
            bg_pixel = 0;
    }

    GdkPixmap *p = gdk_pixmap_new(gd_window, wid, hei, GRX->Visual()->depth);
    if (p) {
        GdkColor clr;
        clr.pixel = bg_pixel;
        gdk_gc_set_foreground(GC(), &clr);
        gdk_draw_rectangle(p, GC(), true, 0, 0, wid, hei);
        if (GC() == XorGC()) {
            clr.pixel = gd_foreg;
            gdk_gc_set_foreground(GC(), &clr);
        }
        else
            gdk_gc_set_foreground(GC(), &vals.foreground);
    }
    else {
        // Fix up the GC as it was.
        if (GC() == XorGC())
            gdk_gc_set_foreground(GC(), &vals.foreground);
        gdk_gc_set_function(GC(), vals.function);
        return;
    }

    gdk_draw_layout(p, GC(), 0, 0, lout);
    g_object_unref(lout);
    GdkImage *im = gdk_image_get(p, 0, 0, wid, hei);
    gdk_pixmap_unref(p);

    // Create a second image for the transformed copy.  This will cntain
    // the background pixels from the rendering area.

    GdkImage *im1;
    if (xform & 1) {
        // rotation
        p = (GdkPixmap*)GetRegion(x, y, hei, wid);
        im1 = gdk_image_get(p, 0, 0, hei, wid);
        gdk_pixmap_unref(p);
    }
    else {
        p = (GdkPixmap*)GetRegion(x, y, wid, hei);
        im1 = gdk_image_get(p, 0, 0, wid, hei);
        gdk_pixmap_unref(p);
    }

    // Transform and copy the pixels, only those that are non-background.

    int i, j;
    unsigned long px;
    switch (xform) {
    case 1:  // R90
    case 15: // MX MY R270
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, j, wid-i-1, px);
            }
        }
        y += wid;
        break;

    case 2:  // R180
    case 12: // MX MY
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, wid-i-1, hei-j-1, px);
            }
        }
        y += hei;
        break;

    case 3:  // R270
    case 13: // MX MY R90
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, hei-j-1, i, px);
            }
        }
        y += wid;
        break;

    case 4:  // MY
    case 10: // MX R180
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, i, hei-j-1, px);
            }
        }
        y += hei;
        break;

    case 5:  // MY R90
    case 11: // MX R270
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, j, i, px);
            }
        }
        y += wid;
        break;

    case 6:  // MY R180
    case 8:  // MX
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, wid-i-1, j, px);
            }
        }
        y += hei;
        break;

    case 7:  // MY R270
    case 9:  // MX R90
        for (i = 0; i < wid; i++) {
            for (j = 0; j < hei;  j++) {
                px = gdk_image_get_pixel(im, i, j);
                if (px != bg_pixel)
                    gdk_image_put_pixel(im1, hei-j-1, wid-i-1, px);
            }
        }
        y += wid;
        break;
    }

    gdk_image_destroy(im);
    if (xform & 1)
        // rotation
        gdk_draw_image(gd_window, GC(), im1, 0, 0, x, y, hei, wid);
    else
        gdk_draw_image(gd_window, GC(), im1, 0, 0, x, y, wid, hei);
    gdk_image_destroy(im1);

    // Fix up the GC as it was.
    if (GC() == XorGC())
        gdk_gc_set_foreground(GC(), &vals.foreground);
    gdk_gc_set_function(GC(), vals.function);
}


// Return the width/height of text.  If text is 0, return
// the max bounds of any character.
//
void
gtk_draw::TextExtent(const char *text, int *wid, int *hei)
{
    if (!text || !*text)
        text = "M";
    PangoLayout *pl;
    PangoContext *pc = 0;
    if (GTK_IS_WIDGET(gd_viewport)) {
        pl = gtk_widget_create_pango_layout(gd_viewport, text);
        GtkRcStyle *rc = gtk_widget_get_modifier_style(gd_viewport);
        if (rc->font_desc)
            pango_layout_set_font_description(pl, rc->font_desc);
    }
    else {
        pc = gdk_pango_context_get();
        pl = pango_layout_new(pc);
        PangoFontDescription *pfd =
            pango_font_description_from_string(FC.getName(FNT_SCREEN));
        pango_layout_set_font_description(pl, pfd);
        pango_font_description_free(pfd);
        pango_layout_set_text(pl, text, -1);
    }

    int tw, th;
    pango_layout_get_pixel_size(pl, &tw, &th);
    g_object_unref(pl);
    if (pc)
        g_object_unref(pc);
    if (wid)
        *wid = tw;
    if (hei)
        *hei = th;
}


// Move the pointer by x, y relative to current position, if absolute
// is false.  If true, move to given location.
//
void
gtk_draw::MovePointer(int x, int y, bool absolute)
{
    // Called with 0,0 this redraws ghost objects
    if (gd_window) {
#if GTK_CHECK_VERSION(2,8,0)
        int x0, y0;
        GdkScreen *screen;
        GdkDisplay *display = gdk_display_get_default();
        gdk_display_get_pointer(display, &screen, &x0, &y0, 0);
        if (absolute)
            gdk_window_get_root_origin(gd_window, &x0, &y0);
        x += x0;
        y += y0;
        gdk_display_warp_pointer(display, screen, x, y);
#else
#ifdef WITH_X11
        XWarpPointer(gr_x_display(), None,
            absolute ? gr_x_window(gd_window) : None, 0, 0, 0, 0, x, y);
#endif
#endif
    }
}


// Set x, y, and state of pointer referenced to window of context.
//
void
gtk_draw::QueryPointer(int *x, int *y, unsigned *state)
{
    int tx, ty;
    unsigned int ts;
    gdk_window_get_pointer(gd_window, &tx, &ty, (GdkModifierType*)&ts);
    if (x)
        *x = tx;
    if (y)
        *y = ty;
    if (state)
        *state = ts;
}


// Define a new rgb value for pixel (if read/write cell) or return a
// new pixel with a matching color.
//
void
gtk_draw::DefineColor(int *pixel, int red, int green, int blue)
{
    GdkColor newcolor;
    newcolor.red   = (red   * 256);
    newcolor.green = (green * 256);
    newcolor.blue  = (blue  * 256);
    newcolor.pixel = *pixel;
    if (ColorAlloc.no_alloc) {
        if (gdk_colormap_alloc_color(GRX->Colormap(), &newcolor, false, true))
            *pixel = newcolor.pixel;
        else
            *pixel = 0;
    }
    else {
        gdk_error_trap_push();
        GRX->SetSilenceErrs(true);
        gdk_color_change(GRX->Colormap(), &newcolor);
        gdk_flush();  // important!
        if (gdk_error_trap_pop()) {
            if (gdk_colormap_alloc_color(GRX->Colormap(), &newcolor,
                    false, true))
                *pixel = newcolor.pixel;
            else
                *pixel = 0;
        }
        if (gd_gbag && gd_gbag->get_gc())
            gdk_gc_set_foreground(GC(), &newcolor);
        GRX->SetSilenceErrs(false);
    }
}


// Set the window background in the GC's.
//
void
gtk_draw::SetBackground(int pixel)
{
    gd_backg = pixel;
    GdkColor clr;
    clr.pixel = pixel;
    gdk_gc_set_background(GC(), &clr);
    gdk_gc_set_background(XorGC(), &clr);
    if (!ColorAlloc.num_mask_allocated) {
        clr.pixel = gd_foreg ^ pixel;
        gdk_gc_set_foreground(XorGC(), &clr);
    }
}


// Actually change the drawing window background.
//
void
gtk_draw::SetWindowBackground(int pixel)
{
    if (!GDK_IS_PIXMAP(gd_window)) {
        GdkColor clr;
        clr.pixel = pixel;
        gdk_window_set_background(gd_window, &clr);
    }
}


// Set the color used for ghost drawing.  pixel is an existing cell
// of the proper color.  If two-plane cells have been allocated,
// copy the pixel value to the upper plane half space, otherwise
// just set the xor foreground color.
//
void
gtk_draw::SetGhostColor(int pixel)
{
    if (ColorAlloc.num_mask_allocated) {
        GdkColor newcolor;
        newcolor.pixel = pixel;
        newcolor.red = GRX->Colormap()->colors[pixel].red;
        newcolor.green = GRX->Colormap()->colors[pixel].green;
        newcolor.blue = GRX->Colormap()->colors[pixel].blue;
        for (int i = 0; i < ColorAlloc.num_mask_allocated; i++) {
            newcolor.pixel =
                ColorAlloc.plane_mask[0] | ColorAlloc.drawing_pixels[i];
            gdk_color_change(GRX->Colormap(), &newcolor);
        }
#ifdef WITH_X11
        XGCValues gcv;
        gcv.plane_mask = ColorAlloc.plane_mask[0];
        XChangeGC(gr_x_display(), gr_x_gc(XorGC()), GCPlaneMask, &gcv);
#endif
        gd_foreg = ColorAlloc.plane_mask[0] | ColorAlloc.drawing_pixels[0];
        newcolor.pixel = gd_foreg;
        gdk_gc_set_foreground(XorGC(), &newcolor);
    }
    else {
        gd_foreg = pixel;
        GdkColor newcolor;
        newcolor.pixel = pixel ^ gd_backg;
        gdk_gc_set_foreground(XorGC(), &newcolor);
    }
}


// Set the current foreground color.
//
void
gtk_draw::SetColor(int pixel)
{
    if (GC() == XorGC())
        return;
    GdkColor clr;
    clr.pixel = pixel;
    gdk_gc_set_foreground(GC(), &clr);
}


// Set the current linestyle.
//
void
gtk_draw::SetLinestyle(const GRlineType *lineptr)
{
    if (!lineptr || !lineptr->mask || lineptr->mask == -1) {
        gdk_gc_set_line_attributes(GC(), 0, GDK_LINE_SOLID,
            GDK_CAP_BUTT, GDK_JOIN_MITER);
        return;
    }
    gdk_gc_set_line_attributes(GC(), 0, GDK_LINE_ON_OFF_DASH,
         GDK_CAP_BUTT, GDK_JOIN_MITER);

    gdk_gc_set_dashes(GC(), lineptr->offset,
        (signed char*)lineptr->dashes, lineptr->length);
}


#ifdef WIN32
namespace {
    // Reverse bit order and complement.
    //
    unsigned int
    revnotbits(unsigned char c)
    {
        unsigned char out = 0;
        for (int i = 0;;) {
            if (!(c & 1))
                out |= 1;
            i++;
            if (i == 8)
                break;
            c >>= 1;
            out <<= 1;
        }
        return (out);
    }
}
#endif


// Create a new pixmap for the fill pattern.
//
void
gtk_draw::DefineFillpattern(GRfillType *fillp)
{
    if (!fillp)
        return;
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    if (fillp->xPixmap()) {
        DeleteBitmap((HBITMAP)fillp->xPixmap());
        fillp->setXpixmap(0);
    }
    if (fillp->hasMap()) {
        // 0 -> text fg color, 1 -> text bg color, so invert map pixels
        int bpl = (fillp->nX() + 7)/8;
        unsigned char *map = fillp->newBitmap();
        unsigned short *invmap =
            new unsigned short[fillp->nY()*(bpl > 2 ? 2:1)];
        if (fillp->nX() <= 16) {
            for (unsigned int i = 0; i < fillp->nY(); i++) {
                unsigned short c = revnotbits(map[i*bpl]);
                if (bpl > 1)
                    c |= revnotbits(map[i*bpl + 1]) << 8;
                invmap[i] = c;
            }
        }
        else {
            for (unsigned int i = 0; i < fillp->nY(); i++) {
                unsigned short c = revnotbits(map[i*bpl]);
                c |= revnotbits(map[i*bpl + 1]) << 8;
                invmap[2*i] = c;
                c = revnotbits(map[i*bpl + 2]);
                if (bpl > 3)
                    c |= revnotbits(map[i*bpl + 3]) << 8;
                invmap[2*i + 1] = c;
            }
        }
        fillp->setXpixmap((GRfillData)CreateBitmap(fillp->nX(), fillp->nY(),
            1, 1, invmap));
        delete [] invmap;
        delete [] map;
    }
#else
    if (fillp->xPixmap()) {
        gdk_pixmap_unref((GdkPixmap*)fillp->xPixmap());
        fillp->setXpixmap(0);
    }
    if (fillp->hasMap()) {
        unsigned char *map = fillp->newBitmap();
        fillp->setXpixmap((GRfillData)gdk_bitmap_create_from_data(gd_window,
            (char*)map, fillp->nX(), fillp->nY()));
        delete [] map;
    }
#endif
}


// Set the current fill pattern.
//
void
gtk_draw::SetFillpattern(const GRfillType *fillp)
{
#if defined(WIN32) && defined(DIRECT_TO_GDI)
    // Alas, gdk-win32 currently does not support stippled drawing,
    // which apparently got lost in the switch to cairo.  So, we roll
    // our own.

    gd_gbag->set_fillpattern(fillp);

#else
    if (!fillp || !fillp->xPixmap())
        gdk_gc_set_fill(GC(), GDK_SOLID);
    else {
        gdk_gc_set_stipple(GC(), (GdkPixmap*)fillp->xPixmap());
        gdk_gc_set_fill(GC(), GDK_STIPPLED);
    }
#endif
}


// Update the display.
//
void
gtk_draw::Update()
{
#ifdef WITH_QUARTZ
    // This is a horrible thing that forces the broken Quartz back end
    // to actually draw something.

    struct myrect : public GdkRectangle
    {
        myrect()
            {
                x = y = 0;
                width = height = 1; 
            }
    };
    static myrect onepixrect;

    if (gd_viewport && GDK_IS_WINDOW(gd_viewport->window))
        gdk_window_invalidate_rect(gd_viewport->window, &onepixrect, false);

    // Equivalent to above but slightly less efficient.
    // if (gd_viewport && GDK_IS_WINDOW(gd_viewport->window))
    //     gtk_widget_queue_draw_area(gd_viewport, 0, 0, 1, 1);
#endif
    gdk_flush();
}


void
gtk_draw::Input(int *keyret, int *butret, int *xret, int *yret)
{
    *keyret = *butret = 0;
    *xret = *yret = 0;
    if (!gd_window)
        return;
    for (;;) {
        GdkEvent *ev = gdk_event_get();
        if (ev) {
            if (ev->type == GDK_BUTTON_PRESS && ev->button.window == gd_window) {
                *butret = ev->button.button;
                *xret = (int)ev->button.x;
                *yret = (int)ev->button.y;
                gdk_event_free(ev);
                break;
            }
            if (ev->type == GDK_KEY_PRESS && ev->key.window == gd_window) {
                if (ev->key.string) {
                    *keyret = *ev->key.string;
                    *xret = 0;
                    *yret = 0;
                    gdk_event_free(ev);
                    break;
                }
            }
            gtk_main_do_event(ev);
            gdk_event_free(ev);
        }
    }
    UndrawGhost(true);
}


// Switch to/from ghost (xor) or highlight/unhighlight drawing context.
// Highlight/unhighlight works only with dual-plane color cells.  It is
// essential to call this with GRxNone after each mode change, due to
// the static storage.
//
void
gtk_draw::SetXOR(int val)
{
    switch (val) {
    case GRxNone:
        gdk_gc_set_function(XorGC(), GDK_XOR);
        gd_gbag->set_xor(false);
        break;
    case GRxXor:
        gd_gbag->set_xor(true);
        break;
    case GRxHlite:
        gdk_gc_set_function(XorGC(), GDK_OR);
        gd_gbag->set_xor(true);
        break;
    case GRxUnhlite:
        gdk_gc_set_function(XorGC(), GDK_AND_INVERT);
        gd_gbag->set_xor(true);
        break;
    }
}


// Show a glyph (from the list).
//

#define GlyphWidth 7

namespace {
    struct stmap
    {
#ifdef WIN32
    HBITMAP pmap;
#else
        GdkPixmap *pmap;
#endif
        char bits[GlyphWidth];
    };
    stmap glyphs[] =
    {
       { 0, {0x00, 0x1c, 0x22, 0x22, 0x22, 0x1c, 0x00} }, // circle
       { 0, {0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41} }, // cross (x)
       { 0, {0x08, 0x14, 0x22, 0x41, 0x22, 0x14, 0x08} }, // diamond
       { 0, {0x08, 0x14, 0x14, 0x22, 0x22, 0x41, 0x7f} }, // triangle
       { 0, {0x7f, 0x41, 0x22, 0x22, 0x14, 0x14, 0x08} }, // inverted triangle
       { 0, {0x7f, 0x41, 0x41, 0x41, 0x41, 0x41, 0x7f} }, // square
       { 0, {0x00, 0x00, 0x1c, 0x14, 0x1c, 0x00, 0x00} }, // dot
       { 0, {0x08, 0x08, 0x08, 0x7f, 0x08, 0x08, 0x08} }, // cross (+)
    };
}


void
gtk_draw::ShowGlyph(int gnum, int x, int y)
{
    x -= GlyphWidth/2;
    y -= GlyphWidth/2;
    gnum = gnum % (sizeof(glyphs)/sizeof(stmap));
    stmap *st = &glyphs[gnum];
#ifdef WIN32
    if (GDK_IS_WINDOW(gd_window)) {
        // Need this correction for windows without implementation.

        int xoff, yoff;
        gdk_window_get_internal_paint_info(gd_window, 0, &xoff, &yoff);
        x -= xoff;
        y -= yoff;
    }
    if (!st->pmap) {
        unsigned short invmap[8];
        for (int i = 0; i < GlyphWidth; i++)
            invmap[i] = revnotbits(st->bits[i]);
        invmap[7] = 0xffff;
        st->pmap = CreateBitmap(8, 8, 1, 1, (char*)invmap);
    }
    HDC dc = gdk_win32_hdc_get(gd_window, GC(), Win32GCvalues);

    HBRUSH brush = CreatePatternBrush(st->pmap);
    brush = SelectBrush(dc, brush);
    SetBrushOrgEx(dc, x, y, 0);

    COLORREF fg = SetTextColor(dc, 0);
    COLORREF bg = SetBkColor(dc, RGB(255, 255, 255));
    // D <- P & D
    PatBlt(dc, x, y, 8, 8, 0xa000c9);
    SetBkColor(dc, 0);
    SetTextColor(dc, fg);
    // D <- P | D
    PatBlt(dc, x, y, 8, 8, 0xfa0089);
    SetBkColor(dc, bg);

    brush = SelectBrush(dc, brush);
    if (brush)
        DeleteBrush(brush);

    gdk_win32_hdc_release(gd_window, GC(), Win32GCvalues);

#else
    if (!st->pmap)
        st->pmap = gdk_bitmap_create_from_data(gd_window, (char*)st->bits,
            GlyphWidth, GlyphWidth);
    gdk_gc_set_stipple(GC(), st->pmap);
    gdk_gc_set_fill(GC(), GDK_STIPPLED);
    gdk_gc_set_ts_origin(GC(), x, y);
    gdk_draw_rectangle(gd_window, GC(), true, x, y, GlyphWidth, GlyphWidth);
    gdk_gc_set_ts_origin(GC(), 0, 0);
    gdk_gc_set_fill(GC(), GDK_SOLID);
#endif
}


GRobject
gtk_draw::GetRegion(int x, int y, int wid, int hei)
{
    GdkPixmap *pm = gdk_pixmap_new(gd_window, wid, hei, GRX->Visual()->depth);
    gdk_window_copy_area(pm, GC(), 0, 0, gd_window, x, y, wid, hei);
    return ((GRobject)pm);
}


void
gtk_draw::PutRegion(GRobject pm, int x, int y, int wid, int hei)
{
    gdk_window_copy_area(gd_window, GC(), x, y, (GdkPixmap*)pm, 0, 0,
        wid, hei);
}


void
gtk_draw::FreeRegion(GRobject pm)
{
    gdk_pixmap_unref((GdkPixmap*)pm);
}


void
gtk_draw::DisplayImage(const GRimage *image, int x, int y,
    int width, int height)
{
#ifdef WITH_X11
#ifdef HAVE_SHMGET
    // Use MIT-SHM if available.  If an error occurrs, fall through and
    // use normal processing.

    if (image->shmid()) {
        XShmSegmentInfo shminfo;
        XImage *im = XShmCreateImage(gr_x_display(),
            gr_x_visual(GRX->Visual()), GRX->Visual()->depth, ZPixmap,
            0, &shminfo, image->width(), image->height());
        if (!im)
            goto normal;
        shminfo.shmid = image->shmid();
        shminfo.shmaddr = im->data = (char*)image->data();
        shminfo.readOnly = False;
        if (XShmAttach(gr_x_display(), &shminfo) != True) {
            im->data = 0;
            XDestroyImage(im);
            goto normal;
        }
        XShmPutImage(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
            im, x, y, x, y, width, height, True);
        XShmDetach(gr_x_display(), &shminfo);
        im->data = 0;
        XDestroyImage(im);
        return;
    }
normal:
#endif

/** not used
    // Variation that uses normal memory in the GRimage and does all
    // SHM stuff here.

    if (GRpkgIf()->UseSHM() > 0) {
        XShmSegmentInfo shminfo;
        XImage *im = XShmCreateImage(gr_x_display(),
            gr_x_visual(GRX->Visual()), GRX->Visual()->depth, ZPixmap,
            0, &shminfo, width, height);
        if (!im)
            return;
        shminfo.shmid = shmget(IPC_PRIVATE, im->bytes_per_line * im->height,
            IPC_CREAT | 0777);
        if (shminfo.shmid == -1) {
            perror("shmget");
            return;
        }
        shminfo.shmaddr = im->data = (char*)shmat(shminfo.shmid, 0, 0);
        shminfo.readOnly = False;
        if (XShmAttach(gr_x_display(), &shminfo) != True)
            return;

        for (int i = 0; i < height; i++) {
            int yd = i + y;
            if (yd < 0)
                continue;
            if ((unsigned int)yd >= image->height())
                break;
            unsigned int *lptr = image->data() + yd*image->width();
            for (int j = 0; j < width; j++) {
                int xd = j + x;
                if (xd < 0)
                    continue;
                if ((unsigned int)xd >= image->width())
                    break;
                XPutPixel(im, j, i, lptr[xd]);
            }
        }
        XShmPutImage(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()),
            im, 0, 0, x, y, width, height, True);
        XShmDetach(gr_x_display(), &shminfo);
        XDestroyImage(im);
        shmdt(shminfo.shmaddr);
        shmctl(shminfo.shmid, IPC_RMID, 0);
    }
*/

    if (GRX->ImageCode() == 0) {
        // Pure-X version, may have slightly less overhead than the gtk
        // code, but does not use SHM.  I can't see any difference
        // with/without SHM anyway.
        //
        // Why allocate a new map and copy with XPutPixel?  The pixel
        // was obtained from X for the present dieplay/visual, so there
        // shouldn't need to be a format change.  The bit padding is 32,
        // same as ours.  Maybe there is an endian issue?  Anyway, this
        // code seems to work, and avoids the copy overhead.

        XImage *im = XCreateImage(gr_x_display(),
            gr_x_visual(GRX->Visual()), GRX->Visual()->depth, ZPixmap,
            0, 0, image->width(), image->height(), 32, 0);
        im->data = (char*)image->data();
        XPutImage(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()), im,
            x, y, x, y, width, height);
        im->data = 0;
        XDestroyImage(im);
        return;
    }
    if (GRX->ImageCode() == 1) {
        // Pure-X version, may have slightly less overhead than the gtk
        // code, but does not use SHM.  I can't see any difference
        // with/without SHM anyway.

        XImage *im = XCreateImage(gr_x_display(),
            gr_x_visual(GRX->Visual()), GRX->Visual()->depth, ZPixmap,
            0, 0, width, height, 32, 0);
        im->data = (char*)malloc(im->bytes_per_line * im->height);

        for (int i = 0; i < height; i++) {
            int yd = i + y;
            if (yd < 0)
                continue;
            if ((unsigned int)yd >= image->height())
                break;
            unsigned int *lptr = image->data() + yd*image->width();
            for (int j = 0; j < width; j++) {
                int xd = j + x;
                if (xd < 0)
                    continue;
                if ((unsigned int)xd >= image->width())
                    break;
                XPutPixel(im, j, i, lptr[xd]);
            }
        }
        XPutImage(gr_x_display(), gr_x_window(gd_window), gr_x_gc(GC()), im,
            0, 0, x, y, width, height);
        XDestroyImage(im);
        return;
    }
#endif

    // Gdk version.

    GdkImage *im = gdk_image_new(GDK_IMAGE_FASTEST, GRX->Visual(),
        width, height);

    for (int i = 0; i < height; i++) {
        int yd = i + y;
        if (yd < 0)
            continue;
        if ((unsigned int)yd >= image->height())
            break;
        for (int j = 0; j < width; j++) {
            int xd = j + x;
            if (xd < 0)
                continue;
            if ((unsigned int)xd >= image->width())
                break;
            unsigned int px = image->data()[xd + yd*image->width()];
#ifdef WITH_QUARTZ
            // Hmmmm, seems that Quartz requires byte reversal.
            unsigned int qpx;
            unsigned char *c1 = ((unsigned char*)&px) + 3;
            unsigned char *c2 = (unsigned char*)&qpx;
            *c2++ = *c1--;
            *c2++ = *c1--;
            *c2++ = *c1--;
            *c2++ = *c1--;
            gdk_image_put_pixel(im, j, i, qpx);
#else
            gdk_image_put_pixel(im, j, i, px);
#endif
        }
    }

    gdk_draw_image(gd_window, GC(), im, 0, 0, x, y, width, height);
    gdk_image_destroy(im);
}
// End of gtk_draw functions.


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

        if (gtk_object_get_data(GTK_OBJECT(widget), "no_prop_key"))
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

        if (gtk_object_get_data(GTK_OBJECT(widget), "no_prop_key"))
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
            (void(*)(GtkWidget*, void*))gtk_object_get_data(
                GTK_OBJECT(widget), "delete_ev_cb");
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
        gtk_signal_connect(GTK_OBJECT(popup), "visibility-notify-event",
            GTK_SIGNAL_FUNC(ToTop), w ? w->Shell() : 0);
        gtk_widget_add_events(popup, GDK_BUTTON_PRESS_MASK);
        gtk_signal_connect_after(GTK_OBJECT(popup), "button-press-event",
            GTK_SIGNAL_FUNC(Btn1MoveHdlr), 0);
    }
    if (title)
        gtk_window_set_title(GTK_WINDOW(popup), title);
    if (quit_cb) {
        gtk_object_set_data(GTK_OBJECT(popup), "delete_ev_cb",
            (void*)quit_cb);
        gtk_signal_connect(GTK_OBJECT(popup), "delete-event",
            GTK_SIGNAL_FUNC(delete_event_hdlr), arg ? arg : popup);
        gtk_signal_connect(GTK_OBJECT(popup), "destroy",
            GTK_SIGNAL_FUNC(quit_cb), arg ? arg : popup);
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


// Convenience function for creating a colored tooltip.
//
GtkTooltips *
gtkinterf::gtk_NewTooltip()
{
    GtkTooltips *tt = gtk_tooltips_new();
    return (tt);
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

