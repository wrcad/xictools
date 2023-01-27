
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
#include "pushpop.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "select.h"
#include "events.h"
#include "tech.h"
#include "gtkmain.h"
#include "gtkparam.h"
#include "gtkcoord.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkfont.h"


//-----------------------------------------------------------------------------
// The Parameter Readout area
//
// Help system keywords used:
//  statusline

namespace {
    const GtkTargetEntry targets[] =
    {
        { (char*)"STRING", 0, 0 },
        { (char*)"text/plain", 0, 1 }
    };
    const gint n_targets = sizeof(targets) / sizeof(targets[0]);

    int  id;

    int print_idle(void*)
    {
        if (Param())
            Param()->print();
        id = 0;
        return (0);
    }
}


// Display the parameter text in the parameter readout area.
//
void
cMain::ShowParameters(const char*)
{
    if (!id)
        id = gtk_idle_add(print_idle, 0);
}


cParam *cParam::instancePtr = 0;

cParam::cParam()
{
    instancePtr = this;
    gd_viewport = gtk_drawing_area_new();
    gtk_widget_set_name(gd_viewport, "Readout");
    gtk_widget_show(gd_viewport);

    p_win_bak = 0;
    p_pm = 0;
    p_drag_x = 0;
    p_drag_y = 0;
    p_has_drag = false;
    p_dragged = false;
    p_xval = 0;
    p_yval = 0;
    p_width = 0;
    p_height = 0;

    gtk_widget_add_events(gd_viewport,
        GDK_BUTTON_PRESS_MASK |
        GDK_BUTTON_RELEASE_MASK |
        GDK_POINTER_MOTION_MASK);
    g_signal_connect(G_OBJECT(gd_viewport), "button-press-event",
        G_CALLBACK(readout_btn_hdlr), 0);
    g_signal_connect(G_OBJECT(gd_viewport), "button-release-event",
        G_CALLBACK(readout_btn_hdlr), 0);
    gtk_widget_add_events(gd_viewport, GDK_EXPOSURE_MASK);
    g_signal_connect(G_OBJECT(gd_viewport), "expose-event",
        G_CALLBACK(readout_redraw), 0);
    g_signal_connect(G_OBJECT(gd_viewport), "style-set",
        G_CALLBACK(readout_font_change), 0);
    g_signal_connect(G_OBJECT(gd_viewport), "motion-notify-event",
        G_CALLBACK(readout_motion_hdlr), 0);
    gtk_selection_add_targets(gd_viewport, GDK_SELECTION_PRIMARY, targets,
        n_targets);
#ifndef WIN32
    g_signal_connect(G_OBJECT(gd_viewport), "selection-clear-event",
        G_CALLBACK(readout_selection_clear), 0);
    g_signal_connect(G_OBJECT(gd_viewport), "selection-get",
        G_CALLBACK(readout_selection_get), 0);
#endif

    GTKfont::setupFont(gd_viewport, FNT_SCREEN, true);

    // Set size
    int wid = 600;
    int hei = GTKfont::stringHeight(gd_viewport, 0) + 2;
    gtk_widget_set_size_request(gd_viewport, wid, hei);
}


void
cParam::print()
{
    if (!gd_window)
        gd_window = gd_viewport->window;
    unsigned long c1 = DSP()->Color(PromptTextColor);
    unsigned long c2 = DSP()->Color(PromptEditTextColor);
    int fwid, fhei;
    TextExtent(0, &fwid, &fhei);

    p_xval = 2;
    p_yval = fhei;

    unsigned int selectno;
    Selections.countQueue(CurCell(), &selectno, 0);
    char textbuf[256];

    p_text.reset();

    const char *str = Tech()->TechnologyName();
    if (str && *str) {
        p_text.append_string("Tech: ", c1);
        p_text.append_string(str, c2);
        p_text.append_string("  ", c2);
    }
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        str = DSP()->MainWdesc()->DbName();
        p_text.append_string("Hier: ", c1);
        p_text.append_string(str, c2);

        if (DSP()->MainWdesc()->DbCellName()) {
            str = DSP()->MainWdesc()->DbCellName();
            p_text.append_string(" Cell: ", c1);
            p_text.append_string(str, c2);
        }
        p_text.append_string("  ", c2);
    }
    else {
        str = (DSP()->CurCellName() ? Tstring(DSP()->CurCellName()) : "none");
        p_text.append_string("Cell: ", c1);
        p_text.append_string(str, c2);

        CDs *cursd = CurCell();
        if (cursd) {
            if (cursd->isImmutable())
                p_text.append_string(" RO", DSP()->Color(PromptHighlightColor));
            else if (cursd->isModified())
                p_text.append_string(" Mod",DSP()->Color(PromptHighlightColor));
        }
        p_text.append_string("  ", c2);
    }

    if (DSP()->PhysVisGridOrigin()->x || DSP()->PhysVisGridOrigin()->y) {
        p_text.append_string("PhGridOffs: ",DSP()->Color(PromptHighlightColor));
        p_text.append_string("  ", c2);
    }

    DSPattrib *a = DSP()->MainWdesc()->Attrib();
    if (a) {
        DisplayMode m = DSP()->CurMode();
        GridDesc *g = a->grid(m);
        double spa = g->spacing(m);
        if (g->snap() < 0)
            sprintf(textbuf, "%g/%g", -spa/g->snap(), spa);
        else
            sprintf(textbuf, "%g/%g", spa*g->snap(), spa);
        p_text.append_string("Grid/Snap: ", c1);
        p_text.append_string(textbuf, c2);
        p_text.append_string("  ", c2);
    }
    if (selectno) {
        sprintf(textbuf, "%d", selectno);
        p_text.append_string("Select: ", c1);
        p_text.append_string(textbuf, c2);
        CDc *cd = (CDc*)Selections.firstObject(CurCell(), "c");
        if (cd) {
            sprintf(textbuf, " (%s)", Tstring(cd->cellname()));
            p_text.append_string(textbuf, c2);
        }
        p_text.append_string("  ", c2);
    }
    int clev = PP()->Level();
    if (clev) {
        sprintf(textbuf, "%d", clev);
        p_text.append_string("Push: ", c1);
        p_text.append_string(textbuf, c2);
        p_text.append_string("  ", c2);
    }

    char *tfstring = GEO()->curTx()->tform_string();
    if (tfstring) {
        // Above returns empty string for identity transform.
        if (*tfstring) {
            p_text.append_string("Transf: ", c1);
            p_text.append_string(tfstring, c2);
            p_text.append_string("  ", c2);
        }
        delete [] tfstring;
    }

    str = EV()->CurCmd() ? EV()->CurCmd()->Name() : 0;
    if (str) {
        p_text.append_string("Mode: ", c1);
        p_text.append_string(str, c2);
        p_text.append_string("  ", c2);
    }
    if (DSP()->CurMode() == Physical) {
        double dx = DSP()->MainWdesc()->ViewportWidth() /
            DSP()->MainWdesc()->Ratio()/CDphysResolution;
        double dy = DSP()->MainWdesc()->ViewportHeight() /
            DSP()->MainWdesc()->Ratio()/CDphysResolution;
        sprintf(textbuf, "%.2f X %.2f", dx, dy);
        p_text.append_string("Window: ", c1);
        p_text.append_string(textbuf, c2);
        p_text.append_string("  ", c2);
    }

    p_text.setup(this);
    display(0, 256);

    if (Coord())
        Coord()->print(0, 0, cCoord::COOR_REL);
}


void
cParam::display(int start, int end)
{
    gd_window = gd_viewport->window;
    if (!gd_window)
        return;

    int winw, winh;
    gdk_window_get_size(gd_window, &winw, &winh);
    if (winw != p_width || winh != p_height) {
        if (p_pm)
            gdk_pixmap_unref(p_pm);
        p_pm = gdk_pixmap_new(gd_window, winw, winh, GRX->Visual()->depth);
        p_width = winw;
        p_height = winh;
        start = 0;
        end = 256;
    }
    p_win_bak = gd_window;
    gd_window = p_pm;
    gd_viewport->window = gd_window;

    if (start == 0 && end == 256) {
        SetWindowBackground(DSP()->Color(PromptBackgroundColor));
        SetColor(DSP()->Color(PromptBackgroundColor));
        Box(0, 0, p_width, p_height);
    }

    p_text.display(this, start, end);

    gdk_window_copy_area(p_win_bak, CpyGC(), 0, 0, gd_window,
        0, 0, p_width, p_height);
    gd_window = p_win_bak;
    p_win_bak = 0;
    gd_viewport->window = gd_window;
}


// Select the chars that overlap the pixel coord range x1,x2.
//
void
cParam::select(int x1, int x2)
{
    if (x2 < x1) {
        int t = x2;
        x2 = x1;
        x1 = t;
    }
    unsigned int xo1 = 0, xo2 = 0;
    bool was_sel = p_text.has_sel(&xo1, &xo2);
    if (p_text.select(x1, x2)) {
        unsigned int xstart, xend;
        if (p_text.has_sel(&xstart, &xend)) {
            if (was_sel) {
                if (xo1 < xstart)
                    xstart = xo1;
                if (xo2 < xstart)
                    xstart = xo2;
                if (xo1 > xend)
                    xend = xo1;
                if (xo2 > xend)
                    xend = xo2;
            }
#ifdef WIN32
            // For some reason the owner_set code doesn't work in
            // Windows.

            char *str = Param()->p_text.get_sel();
            if (str) {
                GtkClipboard *cb = gtk_clipboard_get_for_display(
                    gdk_display_get_default(), GDK_SELECTION_PRIMARY);
                gtk_clipboard_set_with_owner(cb, targets, n_targets,
                    primary_get_cb, primary_clear_cb, G_OBJECT(Viewport()));
                delete [] str;
                display(xstart, xend);
            }
#else
            display(xstart, xend);
            gtk_selection_owner_set(Viewport(), GDK_SELECTION_PRIMARY,
                GDK_CURRENT_TIME);
#endif
        }
    }
    else if (was_sel) {
        deselect();
        gtk_selection_owner_set(0, GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
    }
}


// Select the word at pixel coord x.
//
void
cParam::select_word(int x)
{
    bool was_sel = deselect();
    if (p_text.select_word(x)) {
        unsigned int xstart, xend;
        if (p_text.has_sel(&xstart, &xend)) {
#ifdef WIN32
            // For some reason the owner_set code doesn't work in
            // Windows.

            char *str = Param()->p_text.get_sel();
            if (str) {
                GtkClipboard *cb = gtk_clipboard_get_for_display(
                    gdk_display_get_default(), GDK_SELECTION_PRIMARY);
                gtk_clipboard_set_with_owner(cb, targets, n_targets,
                    primary_get_cb, primary_clear_cb, G_OBJECT(Viewport()));
                delete [] str;
                display(xstart, xend);
            }
#else
            display(xstart, xend);
            gtk_selection_owner_set(Viewport(), GDK_SELECTION_PRIMARY,
                GDK_CURRENT_TIME);
#endif
        }
    }
    else if (was_sel)
        gtk_selection_owner_set(0, GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
}


// Deselect text.
//
bool
cParam::deselect()
{
    unsigned int xstart, xend;
    if (p_text.has_sel(&xstart, &xend)) {
        p_text.set_sel(0, 0);
        display(xstart, xend);
        return (true);
    }
    return (false);
}


#ifdef WIN32
// Static function.
void
cParam::primary_get_cb(GtkClipboard*, GtkSelectionData *data,
    unsigned int, void*)
{
    if (Param()) {
        char *str = Param()->p_text.get_sel();
        if (str) {
            gtk_selection_data_set_text(data, str, -1);
            delete [] str;
        }
    }
}


// Static function.
void
cParam::primary_clear_cb(GtkClipboard*, void*)
{
    if (Param())
        Param()->deselect();
}
#endif


// Static function.
// Pop up info about the parameter display area in help mode.
//
int
cParam::readout_btn_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    if (!Param())
        return (false);
    if (event->button.button == 1 && event->type == GDK_BUTTON_PRESS) {
        if (XM()->IsDoingHelp() && !is_shift_down())
            DSPmainWbag(PopUpHelp("statusline"))
        else {
            Param()->p_has_drag = true;
            Param()->p_dragged = false;
            Param()->p_drag_x = (int)event->button.x;
            Param()->p_drag_y = (int)event->button.y;
        }
        return (true);
    }
    if (event->button.button == 1 && event->type == GDK_BUTTON_RELEASE) {
        if (Param()->p_has_drag && !Param()->p_dragged)
            Param()->select_word(Param()->p_drag_x);
        Param()->p_has_drag = false;
        return (true);
    }
    return (false);
}


// Static functions.
// Pointer motion handler.
//
int
cParam::readout_motion_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    if (!Param())
        return (false);
    if (Param()->p_has_drag) {
        int mvx = (int)event->motion.x;
        Param()->select(Param()->p_drag_x, mvx);
        Param()->p_dragged = true;
        return (true);
    }   
    return (false);
}
        

// Static function.
// Expose handler.
//
int
cParam::readout_redraw(GtkWidget*, GdkEvent *event, void*)
{
    if (!Param())
        return (false);
    GdkEventExpose *pev = (GdkEventExpose*)event;
    if (Param() && GDK_IS_DRAWABLE(Param()->gd_window)) {
        GdkRectangle *rects;
        int nrects;
        gdk_region_get_rectangles(pev->region, &rects, &nrects);
        for (int i = 0; i < nrects; i++) {
            gdk_window_copy_area(Param()->gd_window, Param()->CpyGC(),
                rects[i].x, rects[i].y, Param()->p_pm,
                rects[i].x, rects[i].y, rects[i].width, rects[i].height);
        }
        g_free(rects);
    }

    return (true);
}


// Static function.
// Font change handler.
//
void
cParam::readout_font_change(GtkWidget*, void*, void*)
{
    if (Param() && GDK_IS_DRAWABLE(Param()->gd_window)) {
        int fw, fh;
        Param()->TextExtent(0, &fw, &fh);
        int winw, winh;
        gdk_window_get_size(Param()->gd_window, &winw, &winh);
        gtk_widget_set_size_request(Param()->gd_viewport, -1, fh + 2);
        Param()->print();
    }
}


// Static function.
// Selection clear handler.
//
int
cParam::readout_selection_clear(GtkWidget*, GdkEventSelection*, void*)
{
    if (Param())
        Param()->deselect();
    return (true);
}


void
cParam::readout_selection_get(GtkWidget*, GtkSelectionData *selection_data,
    guint, guint, void*)
{
    if (selection_data->selection != GDK_SELECTION_PRIMARY)
        return;
    if (!Param())
        return;
        
    char *str = Param()->p_text.get_sel();
    if (!str)
        return;  // refuse
    int length = strlen(str);

    gtk_selection_data_set(selection_data, GDK_SELECTION_TYPE_STRING,
        8*sizeof(char), (unsigned char*)str, length);
    delete [] str;
}
// End of cParam functions.


// Select the chars that overlap the pixel coord range xmin,xmax. 
// Return true if success, false if out of range.
//
bool
ptext_t::select(int xmin, int xmax)
{
    if (!pt_set)
        return (false);

    unsigned int xstart = pt_end - 1;
    for (int i = pt_end - 1; i >= 0; i--) {
        if (xmin >= pt_chars[i].pc_posn + pt_chars[i].pc_width)
            break;
        xstart = i;
    }

    unsigned int xend = 0;
    for (unsigned int i = 0; i < pt_end; i++) {
        if (xmax < pt_chars[i].pc_posn)
            break;
        xend = i;
    }

    if (xend >= xstart) {
        set_sel(xstart, xend + 1);
        return (true);
    }
    return (false);
}


// Select the word at pixel coord x.
//
bool
ptext_t::select_word(int x)
{
    if (!pt_set)
        return (false);

    unsigned int xc = pt_end;
    for (unsigned int i = 0; i < pt_end; i++) {
        if (x < pt_chars[i].pc_posn)
            break;
        xc = i;
    }
    if (xc == pt_end || isspace(pt_chars[xc].pc_char))
        return (false);

    int xb = xc;
    while (xb > 0) {
        if (!isspace(pt_chars[xb-1].pc_char))
            xb--;
        else
            break;
    }

    int xe = xc + 1;
    while (xe < pt_end) {
        if (!isspace(pt_chars[xe].pc_char))
            xe++;
        else
            break;
    }
    set_sel(xb, xe);
    return (true);
}


// Set up the character offsets and widths.
//
void
ptext_t::setup(cParam *prm)
{
    char bf[2];
    bf[1] = 0;
    for (unsigned int i = 0; i < pt_end; i++) {
        bf[0] = pt_chars[i].pc_char;
        if (!i)
            pt_chars[i].pc_posn = prm->xval();
        else
            pt_chars[i].pc_posn = pt_chars[i-1].pc_posn +
                pt_chars[i-1].pc_width;
        pt_chars[i].pc_width = GTKfont::stringWidth(prm->Viewport(), bf);
    }
    pt_sel_start = 0;
    pt_sel_end = 0;
    pt_set = true;
}


// Display the chars in the index range fc through lc-1.
//
void
ptext_t::display(cParam *prm, unsigned int fc, unsigned int lc)
{
    if (!pt_set)
        return;
    if (lc < fc) {
        unsigned int t = lc;
        lc = fc;
        fc = t;
    }
    if (fc >= pt_end)
        return;
    if (lc > pt_end)
        lc = pt_end;

    prm->SetFillpattern(0);
    prm->SetLinestyle(0);

    prm->SetWindowBackground(DSP()->Color(PromptBackgroundColor));
    prm->SetColor(DSP()->Color(PromptBackgroundColor));
    prm->Box(pt_chars[fc].pc_posn, 0,
        pt_chars[lc-1].pc_posn + pt_chars[lc-1].pc_width - 1, prm->height());

    char bf[2];
    bf[1] = 0;
    for (unsigned int i = fc; i < lc; i++) {
        bf[0] = pt_chars[i].pc_char;
        if (pt_sel_end > pt_sel_start && i >= pt_sel_start && i < pt_sel_end) {
            prm->SetColor(DSP()->Color(PromptBackgroundColor) ^ -1);
            prm->Box(pt_chars[i].pc_posn, 0,
                pt_chars[i].pc_posn + pt_chars[i].pc_width - 1, prm->height());
            prm->SetColor(DSP()->Color(PromptBackgroundColor));
            prm->Text(bf, pt_chars[i].pc_posn, prm->yval(), 0);
        }
        else {
            prm->SetColor(pt_chars[i].pc_color);
            prm->Text(bf, pt_chars[i].pc_posn, prm->yval(), 0);
        }
    }
}


// Display the chars that overlap the pixel range xmin,xmax.
//
void
ptext_t::display_c(cParam *prm, int xmin, int xmax)
{
    if (!pt_set)
        return;

    unsigned int xstart = pt_end - 1;
    for (int i = pt_end - 1; i >= 0; i--) {
        if (xmin >= pt_chars[i].pc_posn + pt_chars[i].pc_width)
            break;
        xstart = i;
    }

    unsigned int xend = 0;
    for (unsigned int i = 0; i < pt_end; i++) {
        if (xmax < pt_chars[i].pc_posn)
            break;
        xend = i;
    }

    if (xend >= xstart)
        prm->display(xstart, xend + 1);
}

