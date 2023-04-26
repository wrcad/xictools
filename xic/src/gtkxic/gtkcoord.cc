
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

#include "main.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "events.h"
#include "gtkmain.h"
#include "gtkcoord.h"
#include "gtkinterf/gtkfont.h"


//-----------------------------------------------------------------------------
// The coordinate readout
//
// Help system keywords used:
//  coordline

cCoord *cCoord::instancePtr = 0;


// Application interface to set abs/rel and redraw after color change.
// The optional rx,ry are the reference in relative mode.
//
void
cMain::SetCoordMode(COmode mode, int rx, int ry)
{
    if (!Coord())
        return;
    if (mode == CO_ABSOLUTE)
        Coord()->set_mode(0, 0, false, true);
    else if (mode == CO_RELATIVE)
        Coord()->set_mode(rx, ry, true, true);
    else if (mode == CO_REDRAW)
        Coord()->redraw();
}


cCoord::cCoord() : GTKdraw(XW_TEXT)
{
    instancePtr = this;

#if GTK_CHECK_VERSION(3,0,0)
#else
    co_win_bak = 0;
    co_pm = 0;
#endif
    co_width = co_height = 0;
    co_x = co_y = 0;
    co_lx = co_ly = 0;
    co_xc = co_yc = 0;
    co_id = 0;
    co_rel = false;
    co_snap = true;

    SetViewport(gtk_drawing_area_new());
    gtk_widget_show(Viewport());
    gtk_widget_add_events(Viewport(), GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(Viewport()), "button-press-event",
        G_CALLBACK(co_btn), 0);
#if GTK_CHECK_VERSION(3,0,0)
    g_signal_connect(G_OBJECT(Viewport()), "draw",
        G_CALLBACK(co_redraw), 0);
#else
    gtk_widget_add_events(Viewport(), GDK_EXPOSURE_MASK);
    g_signal_connect(G_OBJECT(Viewport()), "expose-event",
        G_CALLBACK(co_redraw), 0);
#endif
    g_signal_connect(G_OBJECT(Viewport()), "style-set",
        G_CALLBACK(co_font_change), 0);

    GTKfont::setupFont(Viewport(), FNT_SCREEN, true);

    // Set size.
    int wid, hei;
    wid = 600;
    hei = GTKfont::stringHeight(Viewport(), 0) + 2;
    gtk_widget_set_size_request(Viewport(), wid, hei);
}


// Define this to put motion handling into an idle proc.
// Not needed here, since we're using an idle dispatch in gtkmain.cc.
// #define MOTION_IDLE

// We use an idle proc to update the motion events, since otherwise with
// Pango, cursor tracking can be slow.
//
void
cCoord::print(int xc, int yc, int update)
{
#ifdef MOTION_IDLE
    if (update != COOR_MOTION) {
        do_print(xc, yc, update);
        return;
    }
    if (co_id) {
        g_source_remove(co_id);
        co_id = 0;
    }
    co_xc = xc;
    co_yc = yc;
    co_id = g_idle_add(print_idle, this);
#else
    co_xc = xc;
    co_yc = yc;
    do_print(xc, yc, update);
#endif
}


int
cCoord::print_idle(void *arg)
{
    cCoord *c = (cCoord*)arg;
    c->do_print(c->co_xc, c->co_yc, COOR_MOTION);
    c->co_id = 0;
    return (0);
}


void
cCoord::do_print(int xc, int yc, int update)
{
    if (!DSP()->MainWdesc() || !EV()->CurrentWin())
        return;
#if GTK_CHECK_VERSION(3,0,0)
    if (!GetDrawable()->get_window())
        GetDrawable()->set_window(gtk_widget_get_window(Viewport()));
    GetDrawable()->set_draw_to_pixmap();
    co_width = GetDrawable()->get_width();
    co_height = GetDrawable()->get_height();
#else
    gd_window = gtk_widget_get_window(gd_viewport);
    if (!gd_window)
        return;

    int winw = gdk_window_get_width(gd_window);
    int winh = gdk_window_get_height(gd_window);
    if (winw != co_width || winh != co_height) {
        if (co_pm)
            gdk_pixmap_unref(co_pm);
        co_pm = gdk_pixmap_new(gd_window, winw, winh,
            gdk_visual_get_depth(GRX->Visual()));
        co_width = winw;
        co_height = winh;
    }
    co_win_bak = gd_window;
    gd_window = co_pm;
    gd_viewport->window = gd_window;
#endif

    const char *fmt;
    DisplayMode mode = EV()->CurrentWin()->Mode();
    if (mode == Physical && CDphysResolution != 1000)
        fmt = "%.4f";
    else
        fmt = "%.3f";

    if (CDvdb()->getVariable(VA_ScreenCoords)) {
        EV()->CurrentWin()->LToP(xc, yc, xc, yc);
        if (mode == Physical) {
            xc *= CDphysResolution;
            yc *= CDphysResolution;
        }
        else {
            xc *= CDelecResolution;
            yc *= CDelecResolution;
        }
    }
    else {
        WindowDesc *wd = EV()->CurrentWin();
        if (wd) {
            double ys = wd->YScale();
            yc = mmRnd(yc/ys);
        }
    }

    unsigned c1 = DSP()->Color(PromptTextColor);
    unsigned c2 = DSP()->Color(PromptEditTextColor);
    int fwid, fhei;
    TextExtent(0, &fwid, &fhei);
    int x = 2;
    int y = (co_height + fhei)/2;  // center justify

    if (co_snap)
        EV()->CurrentWin()->Snap(&xc, &yc);
    if (update == COOR_MOTION) {
        if (xc == co_lx && yc == co_ly) {
#if GTK_CHECK_VERSION(3,0,0)
            GetDrawable()->set_draw_to_window();
#else
            gd_window = co_win_bak;
            co_win_bak = 0;
            gtk_widget_set_window(gd_viewport, gd_window);
#endif
            return;
        }
        co_lx = xc;
        co_ly = yc;
    }
    else if (update == COOR_REL) {
        xc = co_lx;
        yc = co_ly;
    }

    SetColor(DSP()->Color(PromptBackgroundColor));
    SetFillpattern(0);
    Box(0, co_height, co_width, 0);
    SetBackground(DSP()->Color(PromptBackgroundColor));


    char buf[128];
    const char *str = "x,y";
    SetColor(c1);
    Text(str, x, y, 0);
    x += (strlen(str) + 2)*fwid;

    int xs = x;
    sprintf(buf, fmt, mode == Physical ? MICRONS(xc) : ELEC_MICRONS(xc));
    SetColor(c2);
    Text(buf, x, y, 0);
    x += strlen(buf)*fwid;
    SetColor(c1);
    Text(",", x, y, 0);
    x += 2*fwid;
    sprintf(buf, fmt, mode == Physical ? MICRONS(yc) : ELEC_MICRONS(yc));
    SetColor(c2);
    Text(buf, x, y, 0);

    xs += 24*fwid;
    x += (strlen(buf) + 4)*fwid;
    if (x < xs)
        x = xs;
    str = "dx,dy";
    SetColor(c1);
    Text(str, x, y, 0);
    x += (strlen(str) + 2)*fwid;

    int xr, yr;
    if (co_rel) {
        xr = co_x;
        yr = co_y;
    }
    else
        EV()->GetReference(&xr, &yr);

    xs = x;
    sprintf(buf, fmt,
        mode == Physical ? MICRONS(xc - xr) : ELEC_MICRONS(xc - xr));
    SetColor(c2);
    Text(buf, x, y, 0);
    x += (strlen(buf))*fwid;
    SetColor(c1);
    Text(",", x, y, 0);
    x += 2*fwid;
    sprintf(buf, fmt,
        mode == Physical ? MICRONS(yc - yr) : ELEC_MICRONS(yc - yr));
    SetColor(c2);
    Text(buf, x, y, 0);

    xs += 24*fwid;
    x += (strlen(buf) + 4)*fwid;
    if (x < xs)
        x = xs;
    str = co_rel ? "anchor" : "last";
    SetColor(c1);
    Text(str, x, y, 0);
    x += (strlen(str) + 2)*fwid;

    sprintf(buf, fmt, mode == Physical ? MICRONS(xr) : ELEC_MICRONS(xr));
    SetColor(c2);
    Text(buf, x, y, 0);
    x += strlen(buf)*fwid;
    SetColor(c1);
    Text(",", x, y, 0);
    x += 2*fwid;
    sprintf(buf, fmt, mode == Physical ? MICRONS(yr) : ELEC_MICRONS(yr));
    SetColor(c2);
    Text(buf, x, y, 0);

#if GTK_CHECK_VERSION(3,0,0)
    GetDrawable()->set_draw_to_window();
    GetDrawable()->copy_pixmap_to_window(CpyGC(), 0, 0, -1, -1);
#else
    gdk_window_copy_area(co_win_bak, CpyGC(), 0, 0, gd_window,
        0, 0, co_width, co_height);
    gd_window = co_win_bak;
    co_win_bak = 0;
    gtk_widget_set_window(gd_viewport, gd_window);
#endif
}


int
cCoord::co_btn(GtkWidget*, GdkEvent *event, void*)
{
    if (Coord() && XM()->IsDoingHelp() && event->type == GDK_BUTTON_PRESS &&
            !GTKmainwin::is_shift_down())
        DSPmainWbag(PopUpHelp("coordline"))
    return (true);
}


#if GTK_CHECK_VERSION(3,0,0)
int
cCoord::co_redraw(GtkWidget*, cairo_t *cr, void*)
#else
int
cCoord::co_redraw(GtkWidget*, GdkEvent *ev, void*)
#endif
{
    if (!Coord())
        return (false);
#if GTK_CHECK_VERSION(3,0,0)
    Coord()->GetDrawable()->refresh(Coord()->CpyGC(), cr);
    if (!Coord()->GetDrawable()->get_pixmap()) {
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        Coord()->print(x, y, COOR_BEGIN);
        return (true);
    }

#else
    if (!ev || !Coord()->co_pm) {
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        Coord()->print(x, y, COOR_BEGIN);
        return (true);
    }

    GdkEventExpose *pev = (GdkEventExpose*)ev;
    if (Coord() && GDK_IS_DRAWABLE(Coord()->gd_window)) {
        GdkRectangle *rects;
        int nrects;
        gdk_region_get_rectangles(pev->region, &rects, &nrects);
        for (int i = 0; i < nrects; i++) {
            gdk_window_copy_area(Coord()->gd_window, Coord()->CpyGC(),
                rects[i].x, rects[i].y, Coord()->co_pm,
                rects[i].x, rects[i].y, rects[i].width, rects[i].height);
        }
        g_free(rects);
    }
#endif
}


void
cCoord::co_font_change(GtkWidget*, void*, void*)
{
#if GTK_CHECK_VERSION(3,0,0)
    if (Coord() && Coord()->GetDrawable()->get_window()) {
        int fw, fh;
        Coord()->TextExtent(0, &fw, &fh);
        gtk_widget_set_size_request(Coord()->Viewport(), -1, fh + 2);
        Coord()->do_print(Coord()->co_xc, Coord()->co_yc, COOR_BEGIN);
    }
#else
    if (Coord() && GDK_IS_DRAWABLE(Coord()->gd_window)) {
        int fw, fh;
        Coord()->TextExtent(0, &fw, &fh);
        gtk_widget_set_size_request(Coord()->Viewport(), -1, fh + 2);
        Coord()->do_print(Coord()->co_xc, Coord()->co_yc, COOR_BEGIN);
    }
#endif
}

