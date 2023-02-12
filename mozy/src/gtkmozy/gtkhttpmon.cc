
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

/************************************************************************
 *
 * gtkhttpmon.cc: basic C++ class library for http/ftp communications
 *
 * S. R. Whiteley <stevew@srware.com> 2/13/00
 * Copyright (C) 2000 Whiteley Research Inc., All Rights Reserved.
 * Borrowed extensively from the XmHTML widget library
 * Copyright (C) 1994-1997 by Ripley Software Development
 * Copyright (C) 1997-1998 Richard Offer <offer@sgi.com>
 * Borrowed extensively from the Chimera WWW browser
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 ************************************************************************/

#include "httpget/transact.h"
#include <gtk/gtk.h>
#ifdef WITH_X11
#include <gdk/gdkx.h>
#endif
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


//
// The monitor widget and functions for the httpget library and
// program.
//

namespace {
    // Drain the event queue, return false if max_events (> 0) given
    // and reached.
    //
    inline bool
    gtk_DoEvents(int max_events)
    {
        int i = 0;
        while (gtk_events_pending()) {
            gtk_main_iteration();
            if (++i == max_events)
                return (false);
        }
        return (true);
    }


    // Timer callback, exits main loop.
    //
    int
    terminate(void*)
    {
        gtk_main_quit();
        return (0);
    }


    // Handle X events when the connection is active.
    //
    bool
    event_cb(int)
    {
        gtk_DoEvents(100);
        return (false);
    }
}


struct grbits : public http_monitor
{
    grbits()
        {
            g_popup = 0;
            g_text_area = 0;
            g_textbuf = 0;
            g_jbuf_set = false;
        }

    ~grbits()
        {
            if (g_popup)
                gtk_widget_destroy(g_popup);
        }

    bool widget_print(const char*);     // print to monitor

    void abort()
        {
            if (g_jbuf_set) {
                g_jbuf_set = false;
                longjmp(g_jbuf, -2);
            }
        }

    void run(Transaction *t)            // start transfer
        {
            g_jbuf_set = true;
            http_monitor::run(t);
            g_jbuf_set = false;

            // Die after the interval.
            g_timeout_add(2000, (GSourceFunc)terminate, 0);
        }

    bool graphics_enabled()
        {
            return (true);
        }

    void initialize(int&, char**)
        {
            // This will prevent use of themes.  The theme engine may
            // reference library functions that won't be satisfied if
            // this application is statically linked, causing an error
            // message or worse.
            //
            putenv(lstring::copy("GTK_RC_FILES="));
        }

    void setup_comm(cComm *comm)
        {
#ifdef WITH_X11
            comm->comm_set_gfd(ConnectionNumber(gdk_display), event_cb);
#else
            comm->comm_set_gfd(-1, event_cb);
#endif
        }

    void start(Transaction*);

private:
    static void g_dl_cancel_proc(GtkWidget*, void*);
    static void g_cancel_btn_proc(GtkWidget*, void*);
    static int g_da_expose(GtkWidget*, GdkEvent*, void*);
    static int g_idle_proc(void*);

public:
    GtkWidget *g_popup;
    GtkWidget *g_text_area;
    char *g_textbuf;
    bool g_jbuf_set;
    jmp_buf g_jbuf;
};


void
grbits::start(Transaction *t)
{
    grbits *gb = new grbits;
    t->set_gr(gb);
    if (setjmp(gb->g_jbuf) == -2) {
        t->set_err_return(HTTPAborted);
        return;
    }
    gtk_init(0, 0);

    gb->g_popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(gb->g_popup), "Download");
    g_signal_connect(G_OBJECT(gb->g_popup), "destroy",
        G_CALLBACK(grbits::g_dl_cancel_proc), gb);
    GtkWidget *form = gtk_table_new(1, 2, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(gb->g_popup), form);
    GtkWidget *frame = gtk_frame_new(t->url());
    gtk_widget_show(frame);
    gb->g_text_area = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(gb->g_text_area), "expose-event",
        G_CALLBACK(grbits::g_da_expose), gb);
    gtk_widget_show(gb->g_text_area);
    gtk_widget_set_size_request(gb->g_text_area, 320, 30);
    gtk_container_add(GTK_CONTAINER(frame), gb->g_text_area);
    gtk_table_attach(GTK_TABLE(form), frame, 0, 1, 0, 1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);
    GtkWidget *button = gtk_button_new_with_label("Cancel");
    gtk_widget_show(button);
    gtk_table_attach(GTK_TABLE(form), button, 0, 1, 1, 2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(grbits::g_cancel_btn_proc), gb);
    if (t->xpos() >= 0 && t->ypos() >= 0) {
        GdkScreen *scrn = gdk_screen_get_default();
        int x, y;
        gdk_window_get_pointer(0, &x, &y, 0);
        int pmon = gdk_screen_get_monitor_at_point(scrn, x, y);
        GdkRectangle r;
        gdk_screen_get_monitor_geometry(scrn, pmon, &r);
        gtk_window_move(GTK_WINDOW(gb->g_popup),
            t->xpos() + r.x, t->ypos() + r.y);
    }
    gtk_widget_show(gb->g_popup);

    g_idle_add((GSourceFunc)grbits::g_idle_proc, t);
    gtk_main();
}


// This will override the http_monitor stubs on httpget, when linked
// with this, enabling graphics.
//
namespace {
    grbits _monitor_;

    struct http_init_t
    {
        http_init_t()
            {
                httpget::Monitor = &_monitor_;
            }
    };

    http_init_t _http_init_;
}


//-----------------------------------------------------------------------------
// The download monitor widget

// Function to print a message in the text area.  If buf is 0, reprint
// from the t_textbuf.  This also services events, if the widget is
// realized, and returns true.  False is returned if the widget is not
// realized.
//
bool
grbits::widget_print(const char *buf)
{
    if (g_text_area && gtk_widget_get_window(g_text_area)) {
        char *str = lstring::copy(buf ? buf : g_textbuf);
        if (str) {
            char *e = str + strlen(str) - 1;
            while (e >= str && isspace(*e))
                *e-- = 0;
            char *s = str;
            while (*s == '\r' || *s == '\n')
                s++;
            if (*s) {
                if (buf) {
                    delete [] g_textbuf;
                    g_textbuf = lstring::copy(s);  // for expose
                }
                GdkWindow *win = gtk_widget_get_window(g_text_area);
                gdk_window_clear(win);
                GtkStyle *style = gtk_widget_get_style(g_text_area);
                GdkFont *fnt = gtk_style_get_font(style);

//                GdkGC *bgc = gtk_style_get_black_gc(style);
//                int asnt = gdk_font_get_ascent(fnt);
                gdk_draw_string(win, fnt,
                    style->black_gc, 2, fnt->ascent + 2, s);
            }
            delete [] str;
        }
        gtk_DoEvents(100);
        return (true);
    }
    return (false);
}


// Static private function.
// Window destroy callback, does not abort transfer.
//
void
grbits::g_dl_cancel_proc(GtkWidget*, void *data)
{
    grbits *gb = (grbits*)data;
    g_signal_handlers_disconnect_by_func(G_OBJECT(gb->g_popup),
        (gpointer)g_dl_cancel_proc, data);
    gtk_widget_destroy(GTK_WIDGET(gb->g_popup));
    delete [] gb->g_textbuf;
    gb->g_textbuf = 0;
    gb->g_text_area = 0;
    gb->g_popup = 0;
}


// Static private function.
// Cancel button callback, aborts transfer.
//
void
grbits::g_cancel_btn_proc(GtkWidget*, void *data)
{
    grbits *gb = (grbits*)data;
    g_signal_handlers_disconnect_by_func(G_OBJECT(gb->g_popup),
        (gpointer)g_dl_cancel_proc, gb);
    gtk_widget_destroy(GTK_WIDGET(gb->g_popup));
    delete [] gb->g_textbuf;
    gb->g_textbuf = 0;
    gb->g_text_area = 0;
    gb->g_popup = 0;
    gb->abort();
}


// Static private function.
// Expose handler for text area.
//
int
grbits::g_da_expose(GtkWidget*, GdkEvent*, void *data)
{
    http_monitor *gb = (grbits*)data;
    gb->widget_print(0);
    return (1);
}


// Static private function.
// Idle procedure to actually perform transfer.
//
int
grbits::g_idle_proc(void *arg)
{
    Transaction *t = (Transaction*)arg;
    http_monitor *gb = t->gr();
    gb->run(t);
    gtk_main_quit();
    return (false);
}

