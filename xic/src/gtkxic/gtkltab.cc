
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
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "keymap.h"
#include "select.h"
#include "misc_menu.h"
#include "gtkmain.h"
#include "gtkltab.h"
#include "gtkmenu.h"
#include "gtkinlines.h"
#include "gtkinterf/gtkfont.h"
#include "miscutil/miscutil.h"
#include "miscutil/timer.h"
#include "bitmaps/lsearch.xpm"
#include "bitmaps/lsearchn.xpm"

#ifdef WIN32
#include <windows.h>
#include <windowsx.h>
#include <gdk/gdkwin32.h>
#endif


//--------------------------------------------------------------------
// The Layer Table
//
// Help system keywords used:
//  layertab
//  misc. menu keywords

// gtkfillp.cc
extern const char *fillpattern_xpm[];

namespace {
    // The layer table is a dnd source for fill patterns, and a receiver for
    // fillpatterns and colors.
    //
    GtkTargetEntry ltab_targets[] = {
        { (char*)"fillpattern", 0, 0 },
        { (char*)"application/x-color", 0, 1 }
    };
    guint n_ltab_targets = sizeof(ltab_targets) / sizeof(ltab_targets[0]);
}


GTKltab *GTKltab::instancePtr = 0;

GTKltab::GTKltab(bool nogr) : GTKdraw(XW_LTAB)
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class GTKltab already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    ltab_container = 0;
    ltab_scrollbar = 0;
    ltab_search_container = 0;
    ltab_entry = 0;
    ltab_sbtn = 0;
    ltab_lsearch = 0;
    ltab_lsearchn = 0;
#if GTK_CHECK_VERSION(3,0,0)
#else
    ltab_pixmap = 0;
#endif
    ltab_pmap_width = 0;
    ltab_pmap_height = 0;
    ltab_pmap_dirty = false;
    ltab_hidden = false;
    ltab_last_index = 0;
    ltab_search_str = 0;
    ltab_timer_id = 0;
    ltab_last_mode = -1;
    ltab_sbvals[0] = 0.0;
    ltab_sbvals[1] = 0.0;

    lt_drag_x = 0;
    lt_drag_y = 0;
    lt_dragging = false;
    lt_disabled = nogr;
    if (nogr)
        return;

    gd_viewport = gtk_drawing_area_new();
    gtk_widget_set_double_buffered(gd_viewport, false);
    gtk_widget_show(gd_viewport);
    gtk_widget_set_name(gd_viewport, "LayerTable");

    gtk_widget_add_events(gd_viewport, GDK_STRUCTURE_MASK);
    g_signal_connect(G_OBJECT(gd_viewport), "configure-event",
        G_CALLBACK(ltab_resize_hdlr), this);
#if GTK_CHECK_VERSION(3,0,0)
    g_signal_connect(G_OBJECT(gd_viewport), "draw",
        G_CALLBACK(ltab_redraw_hdlr), this);
#else
    gtk_widget_add_events(gd_viewport, GDK_EXPOSURE_MASK);
    g_signal_connect(G_OBJECT(gd_viewport), "expose-event",
        G_CALLBACK(ltab_redraw_hdlr), this);
#endif
    gtk_widget_add_events(gd_viewport, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(gd_viewport), "button-press-event",
        G_CALLBACK(ltab_button_down_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(G_OBJECT(gd_viewport), "button-release-event",
        G_CALLBACK(ltab_button_up_hdlr), this);
    gtk_widget_add_events(gd_viewport, GDK_POINTER_MOTION_MASK);
    g_signal_connect(G_OBJECT(gd_viewport), "motion-notify-event",
        G_CALLBACK(ltab_motion_hdlr), this);
    g_signal_connect(G_OBJECT(gd_viewport), "drag-begin",
        G_CALLBACK(ltab_drag_begin), 0);
    g_signal_connect(G_OBJECT(gd_viewport), "drag-end",
        G_CALLBACK(ltab_drag_end), 0);
    g_signal_connect(G_OBJECT(gd_viewport), "drag-data-get",
        G_CALLBACK(ltab_drag_data_get), 0);
    gtk_drag_dest_set(gd_viewport, GTK_DEST_DEFAULT_ALL, ltab_targets,
        n_ltab_targets, GDK_ACTION_COPY);
    g_signal_connect_after(G_OBJECT(gd_viewport), "drag-data-received",
        G_CALLBACK(ltab_drag_data_received), this);
    g_signal_connect(G_OBJECT(gd_viewport), "style-set",
        G_CALLBACK(ltab_font_change_hdlr), this);
    g_signal_connect(G_OBJECT(gd_viewport), "scroll-event",
        G_CALLBACK(ltab_scroll_event_hdlr), this);

    GTKfont::setupFont(gd_viewport, FNT_SCREEN, true);

    // The table and scrollbar are returned separately, which seems to
    // be required, at least in GTK-1, for the hidden scrollbar to
    // expand the main window instead of leaving a gray area or
    // similar.

    GtkWidget *frame = gtk_frame_new(0);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), gd_viewport);
    ltab_container = frame;

    ltab_scrollbar = gtk_vscrollbar_new(0);
    gtk_widget_show(ltab_scrollbar);

    GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(ltab_scrollbar));
    gtk_range_set_adjustment(GTK_RANGE(ltab_scrollbar), adj);
    g_signal_connect(G_OBJECT(adj), "value-changed",
        G_CALLBACK(ltab_scroll_change_proc), this);

    int ew;
    entry_size(&ew, 0);
    gtk_widget_set_size_request(gd_viewport, ew, 400);

    // The search container contains a text entry and "search" button. 
    // The user can type a few chars into the text area, and cycle
    // through matches by pressing the search button.  For each match,
    // the current layer is set, and the entry will contain the full
    // layer name.

    GtkWidget *hbox = gtk_hbox_new(false, 0);
    gtk_widget_show(hbox);
    ltab_search_container = hbox;

    GtkWidget *button = gtk_NewPixmapButton(lsearch_xpm, 0, false);
    gtk_widget_show(button);
    g_signal_connect(G_OBJECT(button), "clicked",
        G_CALLBACK(ltab_search_hdlr), this);
    gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
    ltab_sbtn = button;
    ltab_lsearch = gtk_bin_get_child(GTK_BIN(button));

    GtkWidget *entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 0);
    gtk_widget_set_size_request(entry, 80, -1);
    g_signal_connect(G_OBJECT(entry), "activate",
        G_CALLBACK(ltab_activate_proc), this);
    ltab_entry = entry;
}


void
GTKltab::setup_drawable()
{
    // Make sure window is set.
#if GTK_CHECK_VERSION(3,0,0)
    if (!GetDrawable()->get_window()) {
        GdkWindow *window = gtk_widget_get_window(gd_viewport);
        GetDrawable()->set_window(window);
    }
#else
    if (!gd_window)
        gd_window = gtk_widget_get_window(gd_viewport);
#endif
}


#define DIMPIXVAL 5

#ifdef WIN32

// Since the gtk implementation doesn't work as of 2.24.10, do it
// in Win32.  Some pretty ugly stuff here.

namespace {
    GdkGCValuesMask Win32GCvalues =
        (GdkGCValuesMask)(GDK_GC_FOREGROUND | GDK_GC_BACKGROUND);

    struct blinker
    {
        blinker(win_bag*, const CDl*);

        ~blinker()
            {
                if (b_dcMemNorm)
                    DeleteDC(b_dcMemNorm);
                if (b_bmapNorm)
                    DeleteBitmap(b_bmapNorm);
                if (b_dcMemDim)
                    DeleteDC(b_dcMemDim);
                if (b_bmapDim)
                    DeleteBitmap(b_bmapDim);
            }

        bool is_ok()
            {
                return (b_wid > 0 && b_hei > 0 && b_dcMemNorm && b_bmapNorm &&
                    b_dcMemDim && b_bmapDim);
            }

        void show_dim()
            {
                if (!b_dcMemDim || !b_bmapDim)
                    return;
                HDC dc = gdk_win32_hdc_get(b_wbag->Window(), b_wbag->GC(),
                    Win32GCvalues);
                BitBlt(dc, b_xoff, b_yoff, b_wid + 1, b_hei + 1, b_dcMemDim,
                    0, 0, SRCCOPY);
                gdk_win32_hdc_release(b_wbag->Window(), b_wbag->GC(),
                    Win32GCvalues);
            }

        void show_norm()
            {
                if (!b_dcMemNorm || !b_bmapNorm)
                    return;
                HDC dc = gdk_win32_hdc_get(b_wbag->Window(), b_wbag->GC(),
                    Win32GCvalues);
                BitBlt(dc, b_xoff, b_yoff, b_wid + 1, b_hei + 1, b_dcMemNorm,
                    0, 0, SRCCOPY);
                gdk_win32_hdc_release(b_wbag->Window(), b_wbag->GC(),
                    Win32GCvalues);
            }

    private:
        win_bag *b_wbag;
        HDC b_dcMemDim;
        HDC b_dcMemNorm;
        HBITMAP b_bmapDim;
        HBITMAP b_bmapNorm;
        int b_wid;
        int b_hei;
        int b_xoff;
        int b_yoff;
    };


    blinker::blinker(win_bag *wb, const CDl *ld)
    {
        b_wbag = wb;
        b_dcMemDim = 0;
        b_dcMemNorm = 0;
        b_bmapDim = 0;
        b_bmapNorm = 0;
        b_xoff = 0;
        b_yoff = 0;

        gdk_window_get_size(wb->Window(), &b_wid, &b_hei);
        if (b_wid < 0 || b_hei < 0 || (!b_wid && !b_hei))
            return;

        // Note the ordering of the COLORREFs relative to the GDK pixels.

        // The dim pixel that alternates with the regular color.
        DspLayerParams *lp = dsp_prm(ld);
        COLORREF dimpix = RGB(
            (GetBValue(lp->pixel())*DIMPIXVAL)/10,
            (GetGValue(lp->pixel())*DIMPIXVAL)/10,
            (GetRValue(lp->pixel())*DIMPIXVAL)/10);

        if (GDK_IS_WINDOW(wb->Window())) {
            // Need this correction for windows without implementation.

            gdk_window_get_internal_paint_info(wb->Window(), 0,
                &b_xoff, &b_yoff);
            b_xoff = -b_xoff;
            b_yoff = -b_yoff;
        }

        HDC dc = gdk_win32_hdc_get(wb->Window(), wb->GC(), Win32GCvalues);
        struct dc_releaser
        {
            dc_releaser(win_bag *wbg)  { wbag = wbg; }
            ~dc_releaser()
            {
                gdk_win32_hdc_release(wbag->Window(), wbag->GC(),
                    Win32GCvalues);
            }
            win_bag *wbag;
        } releaser(wb);

        // Create a pixmap backing the entire window.
        b_dcMemNorm = CreateCompatibleDC(dc);
        if (!b_dcMemNorm)
            return;
        b_bmapNorm = CreateCompatibleBitmap(dc, b_wid, b_hei);
        if (!b_bmapNorm)
            return;
        HBITMAP obm = SelectBitmap(b_dcMemNorm, b_bmapNorm);
        BitBlt(b_dcMemNorm, 0, 0, b_wid + 1, b_hei + 1, dc, b_xoff, b_yoff,
            SRCCOPY);
        SelectBitmap(b_dcMemNorm, obm);
        // The bimap must not be selected into a dc when passed to the
        // image map add function (M$ headache #19305).
        DeleteDC(b_dcMemNorm);
        b_dcMemNorm = 0;

        // Create am image list, which gives us the masking function.
        HIMAGELIST h = ImageList_Create(b_wid, b_hei, ILC_COLORDDB | ILC_MASK,
            0, 0);
        if (!h)
            return;

        ImageList_SetBkColor(h, dimpix);
        // in 256 color mode, the mask is always for the background,
        // whatever pixel is passed.

        COLORREF pix = RGB(GetBValue(lp->pixel()),
            GetGValue(lp->pixel()), GetRValue(lp->pixel()));
        if (ImageList_AddMasked(h, b_bmapNorm, pix) < 0) {
            ImageList_Destroy(h);
            return;
        }
        DeleteBitmap(b_bmapNorm);
        b_bmapNorm = 0;

        // Create a bitmap and draw the image,  The dim-color background
        // of the image list replaces ld->pixel.  Destroy the image list
        // since we're done with it.
        b_dcMemDim = CreateCompatibleDC(dc);
        if (!b_dcMemDim) {
            ImageList_Destroy(h);
            return;
        }
        b_bmapDim = CreateCompatibleBitmap(dc, b_wid, b_hei);
        if (!b_bmapDim) {
            ImageList_Destroy(h);
            return;
        }
        SelectBitmap(b_dcMemDim, b_bmapDim);
        ImageList_Draw(h, 0, b_dcMemDim, 0, 0, ILD_NORMAL);
        ImageList_Destroy(h);

        // Recreate the reference bitmap.  It gets trashed by the image
        // list creation function, so it doesn't work to keep the
        // original.
        b_dcMemNorm = CreateCompatibleDC(dc);
        if (!b_dcMemNorm)
            return;
        b_bmapNorm = CreateCompatibleBitmap(dc, b_wid, b_hei);
        if (!b_bmapNorm)
            return;
        SelectBitmap(b_dcMemNorm, b_bmapNorm);
        BitBlt(b_dcMemNorm, 0, 0, b_wid + 1, b_hei + 1, dc, b_xoff, b_yoff,
            SRCCOPY);
    }


    // Return true if button 3 is pressed.
    //
    bool button3_down()
    {
        return (GetAsyncKeyState(VK_RBUTTON) < 0);
    }
}


#else

namespace {
    struct blinker
    {
        blinker(win_bag*, const CDl*);

        ~blinker()
            {
#if GTK_CHECK_VERSION(3,0,0)
                if (b_dim_pm)
                    b_dim_pm->dec_ref();
                if (b_norm_pm)
                    b_norm_pm->dec_ref();
#else
                if (b_dim_pm)
                    gdk_pixmap_unref(b_dim_pm);
                if (b_norm_pm)
                    gdk_pixmap_unref(b_norm_pm);
#endif
            }

        bool is_ok()
            {
                return (b_wid > 0 && b_hei > 0 && b_dim_pm && b_norm_pm);
            }

        void show_dim()
            {
                if (!b_dim_pm)
                    return;
#if GTK_CHECK_VERSION(3,0,0)
                b_dim_pm->copy_to_window(b_wbag->GetDrawable()->get_window(),
                    b_wbag->GC(), 0, 0, 0, 0, b_wid, b_hei);
#else
                gdk_window_copy_area(b_wbag->Window(), b_wbag->GC(), 0, 0,
                    b_dim_pm, 0, 0, b_wid, b_hei);
                gdk_flush();
#endif
            }

        void show_norm()
            {
                if (!b_norm_pm)
                    return;
#if GTK_CHECK_VERSION(3,0,0)
                b_norm_pm->copy_to_window(b_wbag->GetDrawable()->get_window(),
                    b_wbag->GC(), 0, 0, 0, 0, b_wid, b_hei);
#else
                gdk_window_copy_area(b_wbag->Window(), b_wbag->GC(), 0, 0,
                    b_norm_pm, 0, 0, b_wid, b_hei);
#endif
                gdk_flush();
            }

    private:
        win_bag *b_wbag;
#if GTK_CHECK_VERSION(3,0,0)
        ndkPixmap *b_dim_pm;
        ndkPixmap *b_norm_pm;
#else
        GdkPixmap *b_dim_pm;
        GdkPixmap *b_norm_pm;
#endif
        int b_wid;
        int b_hei;
    };


    unsigned int revbytes(unsigned bits, int nb)
    {
        unsigned int tmp = bits;
        bits = 0;
        unsigned char *a = (unsigned char*)&tmp;
        unsigned char *b = (unsigned char*)&bits;
        nb--;
        for (int i = 0; i <= nb; i++)
            b[i] = a[nb-i];
        return (bits);
    }


    blinker::blinker(win_bag *wb, const CDl *ld)
    {
        b_wbag = wb;
        b_dim_pm = 0;
        b_norm_pm = 0;
        b_wid = 0;
        b_hei = 0;

#if GTK_CHECK_VERSION(3,0,0)
        if (!wb)
            return;

        GdkWindow *window = wb->GetDrawable()->get_window();
        ndkPixmap *pm = new ndkPixmap(window, 1, 1);
        if (!pm)
            return;
        wb->GetDrawable()->set_pixmap(pm);
        wb->SetColor(dsp_prm(ld)->pixel());
        wb->Pixel(0, 0);
        wb->GetDrawable()->set_window(window);  // frees pm
        ndkImage *im = new ndkImage(pm, 0, 0, 1, 1);
        if (!im)
            return;

        int bpp = im->get_bytes_per_pixel();

        int im_order = im->get_byte_order();
        int order = GDK_LSB_FIRST;
        unsigned int pix = 1;
        if (!(*(unsigned char*)&pix))
            order = GDK_MSB_FIRST;

        pix = 0;
        memcpy(&pix, im->get_pixels(), bpp);
        if (order != im_order)
            pix = revbytes(pix, bpp);
        if (order == GDK_MSB_FIRST)
            pix >>= 8*(sizeof(int) - bpp);
        delete im;

        // im->visual = 0 from pixmap
        unsigned int red_mask, green_mask, blue_mask;
        gdk_visual_get_red_pixel_details(GRX->Visual(), &red_mask, 0, 0);
        gdk_visual_get_green_pixel_details(GRX->Visual(), &green_mask, 0, 0);
        gdk_visual_get_blue_pixel_details(GRX->Visual(), &blue_mask, 0, 0);

        int r = (pix & red_mask);
        r = (((r * DIMPIXVAL)/10) & red_mask);
        int g = (pix & green_mask);
        g = (((g * DIMPIXVAL)/10) & green_mask);
        int b = (pix & blue_mask);
        b = (((b * DIMPIXVAL)/10) & blue_mask);
        unsigned int dimpix = r | g | b;

        if (order == GDK_MSB_FIRST) {
            pix <<= 8*(sizeof(int) - bpp);
            dimpix <<= 8*(sizeof(int) - bpp);
        }
        if (order != im_order) {
            pix = revbytes(pix, bpp);
            dimpix = revbytes(dimpix, bpp);
        }

        b_wid = wb->GetDrawable()->get_width();
        b_hei = wb->GetDrawable()->get_height();
        if (b_wid < 0 || b_hei < 0 || (!b_wid && !b_hei))
            return;

        pm = new ndkPixmap(window, b_wid, b_hei,
            gdk_visual_get_depth(GRX->Visual()));
        if (!pm)
            return;
        pm->copy_from_window(window, wb->GC(), 0, 0, 0, 0, b_wid, b_hei);
        b_norm_pm = pm;

        // There is a bug in 2.24.10, using the window rather than the
        // pixmap in gdk_image_get doesn't work.  The image has offsets
        // that seem to require compensation with
        // gdk_window_get_internal_paint_info() or similar, but I could
        // never get this to work properly.

        im = new ndkImage(pm, 0, 0, b_wid, b_hei);
        if (!im) {
            delete pm;
            b_norm_pm = 0;
            return;
        }

        // There may be some alpha info that should be ignored.
        unsigned int mask = red_mask | green_mask | blue_mask;

        char *z = (char*)im->get_pixels();
        int i = b_wid*b_hei;
        while (i--) {
            unsigned f = 0;
            unsigned char *a = (unsigned char*)&f;
            for (int j = 0; j < bpp; j++)
                a[j] = *z++;
            if ((f & mask) == (pix & mask)) {
                z -= bpp;
                a = (unsigned char*)&dimpix;
                for (int j = 0; j < bpp; j++)
                    *z++ = a[j];
            }
        }
        pm = new ndkPixmap(window, b_wid, b_hei);
        if (!pm) {
            delete im;
            b_norm_pm->dec_ref();
            b_norm_pm = 0;
            return;
        }
        im->copy_to_pixmap(pm, wb->GC(), 0, 0, 0, 0, b_wid, b_hei);
        delete im;
        b_dim_pm = pm;

#else
        if (!wb)
            return;

        GdkPixmap *pm = gdk_pixmap_new(wb->Window(), 1, 1,
            gdk_visual_get_depth(GRX->Visual()));
        if (!pm)
            return;
        GdkWindow *bk = wb->Window();
        wb->SetWindow(pm);
        wb->SetColor(dsp_prm(ld)->pixel());
        wb->Pixel(0, 0);
        wb->SetWindow(bk);
        GdkImage *im = gdk_image_get(pm, 0, 0, 1, 1);
        gdk_pixmap_unref(pm);
        if (!im)
            return;

        int bpp = im->bpp;

        int im_order = im->byte_order;
        int order = GDK_LSB_FIRST;
        unsigned int pix = 1;
        if (!(*(unsigned char*)&pix))
            order = GDK_MSB_FIRST;

        pix = 0;
        memcpy(&pix, im->mem, bpp);
        if (order != im_order)
            pix = revbytes(pix, bpp);
        if (order == GDK_MSB_FIRST)
            pix >>= 8*(sizeof(int) - bpp);
        gdk_image_destroy(im);

        // im->visual = 0 from pixmap
        unsigned red_mask = GRX->Visual()->red_mask;
        unsigned green_mask = GRX->Visual()->green_mask;
        unsigned blue_mask = GRX->Visual()->blue_mask;
        int r = (pix & red_mask);
        r = (((r * DIMPIXVAL)/10) & red_mask);
        int g = (pix & green_mask);
        g = (((g * DIMPIXVAL)/10) & green_mask);
        int b = (pix & blue_mask);
        b = (((b * DIMPIXVAL)/10) & blue_mask);
        unsigned int dimpix = r | g | b;

        if (order == GDK_MSB_FIRST) {
            pix <<= 8*(sizeof(int) - bpp);
            dimpix <<= 8*(sizeof(int) - bpp);
        }
        if (order != im_order) {
            pix = revbytes(pix, bpp);
            dimpix = revbytes(dimpix, bpp);
        }

        gdk_window_get_size(wb->Window(), &b_wid, &b_hei);
        if (b_wid < 0 || b_hei < 0 || (!b_wid && !b_hei))
            return;

        pm = gdk_pixmap_new(wb->Window(), b_wid, b_hei, GRX->Visual()->depth);
        if (!pm)
            return;
        gdk_window_copy_area(pm, wb->GC(), 0, 0, wb->Window(), 0, 0,
            b_wid, b_hei);
        b_norm_pm = pm;

        // There is a bug in 2.24.10, using the window rather than the
        // pixmap in gdk_image_get doesn't work.  The image has offsets
        // that seem to require compensation with
        // gdk_window_get_internal_paint_info() or similar, but I could
        // never get this to work properly.

        im = gdk_image_get(pm, 0, 0, b_wid, b_hei);
        if (!im) {
            gdk_pixmap_unref(pm);
            b_norm_pm = 0;
            return;
        }

        // There may be some alpha info that should be ignored.
        unsigned int mask = red_mask | green_mask | blue_mask;

        char *z = (char*)im->mem;
        int i = b_wid*b_hei;
        while (i--) {
            unsigned f = 0;
            unsigned char *a = (unsigned char*)&f;
            for (int j = 0; j < bpp; j++)
                a[j] = *z++;
            if ((f & mask) == (pix & mask)) {
                z -= bpp;
                a = (unsigned char*)&dimpix;
                for (int j = 0; j < bpp; j++)
                    *z++ = a[j];
            }
        }
        pm = gdk_pixmap_new(wb->Window(), b_wid, b_hei,
            gdk_visual_get_depth(GRX->Visual()));
        if (!pm) {
            gdk_image_destroy(im);
            gdk_pixmap_unref(b_norm_pm);
            b_norm_pm = 0;
            return;
        }
        gdk_draw_image(pm, wb->GC(), im, 0, 0, 0, 0, b_wid, b_hei);
        gdk_image_destroy(im);
        b_dim_pm = pm;
#endif
    }


    // Return true if button 3 is pressed.
    //
    bool button3_down()
    {
        unsigned state;
        mainBag()->QueryPointer(0, 0, &state);
        return (state & GDK_BUTTON3_MASK);
    }
}

#endif


namespace {
    blinker *blinkers[DSP_NUMWINS];
    int blink_timer_id;
    bool blink_state;


    // Idle proc for blinking layers.
    //
    int blink_idle(void*) {
        int cnt = 0;
        for (int i = 0; i < DSP_NUMWINS; i++) {
            if (!blinkers[i])
                continue;
            cnt++;
            if (!blink_state)
                blinkers[i]->show_dim();
            else
                blinkers[i]->show_norm();
        }
        cTimer::milli_sleep(200);

        if (!button3_down()) {
            for (int i = 0; i < DSP_NUMWINS; i++) {
                if (!blinkers[i])
                    continue;
                blinkers[i]->show_norm();
                delete blinkers[i];
                blinkers[i] = 0;
            }
            cnt = 0;
        }
        if (!cnt) {
            blink_timer_id = 0;
            blink_state = false;
            return (0);
        }
        blink_state = !blink_state;
        return (1);
    }
}


void
GTKltab::blink(CDl *ld)
{
    if (lt_disabled)
        return;

    for (int i = 0; i < DSP_NUMWINS; i++) {
        WindowDesc *wd = DSP()->Window(i);
        if (!wd)
            continue;
        win_bag *w = dynamic_cast<win_bag*>(wd->Wbag());
        if (!w)
            continue;
        blinker *b = new blinker(w, ld);
        if (!b->is_ok()) {
            delete b;
            continue;
        }
        blinker *oldb = blinkers[i];
        blinkers[i] = b;
        delete oldb;
    }
    if (!blink_timer_id)
        blink_timer_id = g_idle_add(blink_idle, 0);
}


void
GTKltab::show(const CDl *ld)
{
    if (lt_disabled)
        return;
#if GTK_CHECK_VERSION(3,0,0)
    GetDrawable()->set_draw_to_pixmap();
#else
    if (!gd_window)
        gd_window = gtk_widget_get_window(gd_viewport);
    if (!ltab_pixmap || ltab_pmap_width != lt_win_width ||
            ltab_pmap_height != lt_win_height) {
        if (ltab_pixmap)
            gdk_pixmap_unref(ltab_pixmap);
        ltab_pmap_width = lt_win_width;
        ltab_pmap_height = lt_win_height;
        ltab_pixmap = gdk_pixmap_new(gd_window, ltab_pmap_width,
            ltab_pmap_height, gdk_visual_get_depth(GRX->Visual()));
    }

    GdkWindow *win = gd_window;
    gd_window = ltab_pixmap;
#endif

    show_direct(ld);

#if GTK_CHECK_VERSION(3,0,0)
    GetDrawable()->set_draw_to_window();
    GetDrawable()->copy_pixmap_to_window(GC(), 0, 0, lt_win_width,
        lt_win_height);
#else
    gdk_window_copy_area(win, GC(), 0, 0, gd_window, 0, 0, lt_win_width,
        lt_win_height);
    gd_window = win;
#endif
    ltab_pmap_dirty = false;
}


// Exposure redraw, avoids flicker.
//
void
GTKltab::refresh(int x, int y, int w, int h)
{
    if (lt_disabled)
        return;
#if GTK_CHECK_VERSION(3,0,0)
    if (!GetDrawable()->get_window())
        GetDrawable()->set_window(gtk_widget_get_window(gd_viewport));
    if (!GetDrawable()->get_window())
        return;
    int lt_win_width = GetDrawable()->get_width();
    int lt_win_height = GetDrawable()->get_height();
    ltab_pmap_dirty = GetDrawable()->set_draw_to_pixmap();
    if (w <= 0)
        w = lt_win_width;
    if (h <= 0)
        h = lt_win_height;
#else
    if (!gd_window)
        gd_window = gtk_widget_get_window(gd_viewport);
    if (!gd_window)
        return;

    int lt_win_width = gdk_window_get_width(gd_window);
    int lt_win_height = gdk_window_get_height(gd_window);
    if (w <= 0)
        w = lt_win_width;
    if (h <= 0)
        h = lt_win_height;

    if (!ltab_pixmap || ltab_pmap_width != lt_win_width ||
            ltab_pmap_height != lt_win_height) {
        if (ltab_pixmap)
            gdk_pixmap_unref(ltab_pixmap);
        ltab_pmap_width = lt_win_width;
        ltab_pmap_height = lt_win_height;
        ltab_pixmap = gdk_pixmap_new(gd_window, ltab_pmap_width,
            ltab_pmap_height, gdk_visual_get_depth(GRX->Visual()));
        ltab_pmap_dirty = true;
    }

    GdkWindow *win = gd_window;
    gd_window = ltab_pixmap;
#endif

    if (ltab_pmap_dirty) {
        show_direct();
        ltab_pmap_dirty = false;
    }
#if GTK_CHECK_VERSION(3,0,0)
    GetDrawable()->set_draw_to_window();
    GetDrawable()->copy_pixmap_to_window(GC(), x, y, w, h);
#else
    gdk_window_copy_area(win, GC(), x, y, gd_window, x, y, w, h);
    gd_window = win;
#endif
}


// Return the drawing area size.
//
void
GTKltab::win_size(int *wid, int *hei)
{
#if GTK_CHECK_VERSION(3,0,0)
    if (wid)
        *wid = GetDrawable()->get_width();
    if (hei)
        *hei = GetDrawable()->get_height();
#else
    *wid = 0;
    *hei = 0;
    if (gd_window) {
        *wid = gdk_window_get_width(gd_window);
        *hei = gdk_window_get_height(gd_window);
    }
#endif
}


// Update the display.
//
void
GTKltab::update()
{
    // Record that we need to update backing store.
    ltab_pmap_dirty = true;
    Update();
}


// Update the scrollbar.
//
void
GTKltab::update_scrollbar()
{
    if (ltab_scrollbar) {
        GtkAdjustment *adj =
            gtk_range_get_adjustment(GTK_RANGE(ltab_scrollbar));

        // This remembers the last scrollbar position across a mode
        // switch.  Setting the current layer will scroll to position,
        // so thie really isn't necessary.

        if (ltab_last_mode >= 0 && ltab_last_mode != DSP()->CurMode()) {
            ltab_sbvals[ltab_last_mode] = gtk_adjustment_get_value(adj);
            gtk_adjustment_set_value(adj, ltab_sbvals[DSP()->CurMode()]);
        }
        ltab_last_mode = DSP()->CurMode();
        ltab_sbvals[ltab_last_mode] = gtk_adjustment_get_value(adj);

        int numents = CDldb()->layersUsed(DSP()->CurMode()) - 1;
        gtk_adjustment_set_upper(adj, numents);
        gtk_adjustment_set_step_increment(adj, 1);
        gtk_adjustment_set_page_increment(adj, lt_vis_entries);
        gtk_adjustment_set_page_size(adj, lt_vis_entries);
        gtk_adjustment_changed(adj);
        gtk_adjustment_value_changed(adj);

        // Below, we compensate for the changing scrollbar presence by
        // moving the paned handle.  This keeps the main drawing
        // window size fixed.  This works nicely with GTK-2, as the
        // visual changes are accumulated, and unacceptably under
        // GTK-1.2, where both operations are performed visibly.

        if (lt_vis_entries <= CDldb()->layersUsed(DSP()->CurMode()) - 1) {
            if (!ltab_hidden && !gtk_widget_get_mapped(ltab_scrollbar)) {
                int w = gtk_paned_get_position(
                    GTK_PANED(gtk_widget_get_parent(ltab_container)));
                GtkRequisition req;
                gtk_widget_get_requisition(ltab_scrollbar, &req);
                int ww = req.width;
                if (getenv("XIC_MENU_RIGHT"))
                    ww = -ww;
                gtk_paned_set_position(
                    GTK_PANED(gtk_widget_get_parent(ltab_container)), w - ww);
                gtk_widget_show(ltab_scrollbar);
            }
        }
        else if (gtk_widget_get_mapped(ltab_scrollbar)) {
            int w = gtk_paned_get_position(
                GTK_PANED(gtk_widget_get_parent(ltab_container)));
            GtkRequisition req;
            gtk_widget_get_requisition(ltab_scrollbar, &req);
            int ww = req.width;
            if (getenv("XIC_MENU_RIGHT"))
                ww = -ww;
            gtk_paned_set_position(
                GTK_PANED(gtk_widget_get_parent(ltab_container)), w + ww);
            gtk_widget_hide(ltab_scrollbar);
        }
    }
}


void
GTKltab::hide_layer_table(bool hide)
{
    // Reparenting does not seem to work, always get fatal errors.
    // We just move the slider to cover the layer table here.

    static int last_wid;
    if (hide) {
        last_wid = gtk_paned_get_position(
            GTK_PANED(gtk_widget_get_parent(ltab_container)));
        if (gtk_widget_get_mapped(ltab_scrollbar)) {
            GtkRequisition req;
            gtk_widget_get_requisition(ltab_scrollbar, &req);
            int ww = req.width;
            if (getenv("XIC_MENU_RIGHT"))
                ww = -ww;
            last_wid += ww;
        }

        gtk_widget_hide(ltab_container);
        gtk_widget_hide(ltab_scrollbar);
        gtk_paned_set_position(
            GTK_PANED(gtk_widget_get_parent(ltab_container)), 0);
        ltab_hidden = true;
    }
    else {
        if (lt_vis_entries <= CDldb()->layersUsed(DSP()->CurMode()) - 1) {
            gtk_widget_show(ltab_scrollbar);
            GtkRequisition req;
            gtk_widget_get_requisition(ltab_scrollbar, &req);
            int ww = req.width;
            if (getenv("XIC_MENU_RIGHT"))
                ww = -ww;
            last_wid -= ww;
        }
        gtk_widget_show(ltab_container);
        gtk_paned_set_position(GTK_PANED(gtk_widget_get_parent(ltab_container)),
            last_wid);
        ltab_hidden = false;
    }
}


void
GTKltab::set_layer()
{
    CDl *ld = LT()->CurLayer();
    if (ld)
        gtk_entry_set_text(GTK_ENTRY(ltab_entry), ld->name());
    else {
        gtk_entry_set_text(GTK_ENTRY(ltab_entry), "");
        return;
    }

    int pos = ld->index(DSP()->CurMode()) - 1;
    if (pos >= first_visible() && pos < first_visible() + vis_entries())
        show();
    else {
        GtkAdjustment *adj =
            gtk_range_get_adjustment(GTK_RANGE(ltab_scrollbar));
        if (adj) {
            int nt = CDldb()->layersUsed(DSP()->CurMode()) - 1;
            nt -= vis_entries();
            pos -= vis_entries()/2;
            if (pos < 0)
                pos = 0;
            else if (pos > nt)
                pos = nt;
            gtk_adjustment_set_value(adj, pos);
            lt_first_visible = pos;
            update_scrollbar();
            show();
        }
    }
}


// Static function.
// Resize the layer table.
//
int
GTKltab::ltab_resize_hdlr(GtkWidget*, GdkEvent*, void *arg)
{
    GTKltab *lt = static_cast<GTKltab*>(arg);
    if (lt)
        lt->init();
    return (true);
}


// Static function.
// Redraw the layer table.
//
#if GTK_CHECK_VERSION(3,0,0)
int
GTKltab::ltab_redraw_hdlr(GtkWidget*, cairo_t *cr, void *arg)
#else
int
GTKltab::ltab_redraw_hdlr(GtkWidget*, GdkEvent *event, void *arg)
#endif
{
#if GTK_CHECK_VERSION(3,0,0)
    GTKltab *lt = static_cast<GTKltab*>(arg);
    if (lt) {
        cairo_rectangle_int_t rect;
        ndkDrawable::redraw_area(cr, &rect);
        lt->refresh(rect.x, rect.y, rect.width, rect.height);
    }
#else
    GdkEventExpose *pev = (GdkEventExpose*)event;
    GTKltab *lt = static_cast<GTKltab*>(arg);
    if (lt) {
        lt->refresh(pev->area.x, pev->area.y, pev->area.width,
            pev->area.height);
    }
#endif
    return (true);
}


// Static function.
// Dispatch button presses in the layer table area.
//
int
GTKltab::ltab_button_down_hdlr(GtkWidget*, GdkEvent *event, void *arg)
{
    if (event->type == GDK_2BUTTON_PRESS ||
            event->type == GDK_3BUTTON_PRESS)
        return (true);
    GdkEventButton *bev = (GdkEventButton*)event;
    int button = Kmap()->ButtonMap(bev->button);

    GTKltab *lt = static_cast<GTKltab*>(arg);

    // Handle mouse wheel events (gtk1 only)
    if (button == 4 || button == 5) {
        if (gtk_widget_get_mapped(lt->ltab_scrollbar)) {
            GtkAdjustment *adj = gtk_range_get_adjustment(
                GTK_RANGE(lt->ltab_scrollbar));
            double nval = gtk_adjustment_get_value(adj) + ((button == 4) ?
                -gtk_adjustment_get_page_increment(adj) / 4 :
                gtk_adjustment_get_page_increment(adj) / 4);
            if (nval < gtk_adjustment_get_lower(adj))
                nval = gtk_adjustment_get_lower(adj);
            else if (nval > gtk_adjustment_get_upper(adj) -
                    gtk_adjustment_get_page_size(adj))
                nval = gtk_adjustment_get_upper(adj) -
                    gtk_adjustment_get_page_size(adj);
            gtk_adjustment_set_value(adj, nval);
        }  
        return (true);
    }


    if (XM()->IsDoingHelp() && !is_shift_down()) {
        DSPmainWbag(PopUpHelp("layertab"))
        return (true);
    }
    if (lt) {
        if (button == 1)
            lt->b1_handler((int)bev->x, (int)bev->y, bev->state, true);
        else if (button == 2)
            lt->b2_handler((int)bev->x, (int)bev->y, bev->state, true);
        else if (button == 3)
            lt->b3_handler((int)bev->x, (int)bev->y, bev->state, true);
    }
    return (true);
}


// Static function.
int
GTKltab::ltab_button_up_hdlr(GtkWidget*, GdkEvent *event, void *arg)
{
    GdkEventButton *bev = (GdkEventButton*)event;
    int button = Kmap()->ButtonMap(bev->button);

    GTKltab *lt = static_cast<GTKltab*>(arg);

    if (XM()->IsDoingHelp() && !is_shift_down())
        return (true);

    if (lt) {
        if (button == 1)
            lt->b1_handler((int)bev->x, (int)bev->y, bev->state, false);
        else if (button == 2)
            lt->b2_handler((int)bev->x, (int)bev->y, bev->state, false);
        else if (button == 3)
            lt->b3_handler((int)bev->x, (int)bev->y, bev->state, false);
    }
    return (true);
}


// Static function.
int
GTKltab::ltab_motion_hdlr(GtkWidget *caller, GdkEvent *event, void *arg)
{
    GTKltab *lt = static_cast<GTKltab*>(arg);
    if (!lt)
        return (false);
    int x = (int)event->motion.x;
    int y = (int)event->motion.y;
    if (lt->drag_check(x, y)) {
        // fillpattern only
        GtkTargetList *targets = gtk_target_list_new(ltab_targets, 1);
        gtk_drag_begin(caller, targets, (GdkDragAction)GDK_ACTION_COPY,
            1, event);
    }

    int entry = lt->entry_of_xy(x, y);
    int last_ent = lt->last_entry();
    if (entry <= last_ent) {
        CDl *ld =
            CDldb()->layer(entry + lt->first_visible() + 1, DSP()->CurMode());
        XM()->PopUpLayerPalette(0, MODE_UPD, true, ld);
    }
    return (true);
}


// Static function.
// Set the pixmap.
//
void
GTKltab::ltab_drag_begin(GtkWidget*, GdkDragContext *context, gpointer)
{
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_xpm_data(fillpattern_xpm);
    gtk_drag_set_icon_pixbuf(context, pixbuf, -2, -2);
    win_bag::HaveDrag = true;
}


// Static function.
void
GTKltab::ltab_drag_end(GtkWidget*, GdkDragContext*, gpointer)
{
    win_bag::HaveDrag = false;
}


// Static function.
void
GTKltab::ltab_drag_data_get(GtkWidget*, GdkDragContext*,
    GtkSelectionData *data, guint, guint, void*)
{
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return;
    LayerFillData dd(ld);
    gtk_selection_data_set(data, gtk_selection_data_get_target(data),
        8, (unsigned char*)&dd, sizeof(LayerFillData));
}


// Static function.
// Drag data received in layer table, from the color selection popup.
// Reset the color of the layer under the drop.
//
void
GTKltab::ltab_drag_data_received(GtkWidget*, GdkDragContext *context,
    gint x, gint y, GtkSelectionData *data, guint, guint time, void *arg)
{
    GTKltab *lt = static_cast<GTKltab*>(arg);
    if (!lt) {
        gtk_drag_finish(context, false, false, time);
        return;
    }

    // datum is a guint16 array of the format:
    //  R G B opacity
    GdkAtom a = gdk_atom_intern("fillpattern", true);
    if (gtk_selection_data_get_target(data) == a) {
        if (mainBag())
            XM()->FillLoadCallback(
                (LayerFillData*)gtk_selection_data_get_data(data),
                LT()->LayerAt(x, y));
    }
    else {
        if (gtk_selection_data_get_length(data) < 0) {
            gtk_drag_finish(context, false, false, time);
            return;
        }
        if (gtk_selection_data_get_format(data) != 16 ||
                gtk_selection_data_get_length(data) != 8) {
            fprintf(stderr, "Received invalid color data\n");
            gtk_drag_finish(context, false, false, time);
            return;
        }
        guint16 *vals = (guint16*)gtk_selection_data_get_data(data);

        int entry = lt->entry_of_xy(x, y);
        if (entry > lt->last_entry()) {
            gtk_drag_finish(context, false, false, time);
            return;
        }
        CDl *layer =
            CDldb()->layer(entry + lt->first_visible() + 1, DSP()->CurMode());

        LT()->SetLayerColor(layer, vals[0] >> 8, vals[1] >> 8, vals[2] >> 8);
        if (dspPkgIf()->IsTrueColor()) {
            // update the colors
            LT()->ShowLayerTable(layer);
            XM()->PopUpFillEditor(0, MODE_UPD);
        }
    }
    gtk_drag_finish(context, true, false, time);
    if (DSP()->CurMode() == Electrical || !LT()->NoPhysRedraw())
        DSP()->RedisplayAll();
}


// Static function.
void
GTKltab::ltab_font_change_hdlr(GtkWidget*, void*, void *arg)
{
    GTKltab *lt = static_cast<GTKltab*>(arg);
    if (lt && lt->gd_viewport) {
        int ew;
        lt->entry_size(&ew, 0);
        gtk_widget_set_size_request(lt->gd_viewport, ew, -1);
    }
}


// Static function.
void
GTKltab::ltab_scroll_change_proc(GtkAdjustment *adj, void *arg)
{
    GTKltab *lt = static_cast<GTKltab*>(arg);
    if (lt) {
        int val = (int)gtk_adjustment_get_value(adj);
        if (val != lt->first_visible()) {
            lt->set_first_visible(val);
            lt->show();
        }
    }
}


// Static function.
// Gtk2 mouse wheel handler, send the event to the vertical scroll   
// bar.
//
int
GTKltab::ltab_scroll_event_hdlr(GtkWidget*, GdkEvent *event, void *arg)
{
    GTKltab *lt = static_cast<GTKltab*>(arg);
    if (lt && gtk_widget_get_mapped(lt->ltab_scrollbar))
        gtk_propagate_event(lt->ltab_scrollbar, event);
    return (true);
}


// Static function.
int
GTKltab::ltab_ent_timer(void *arg)
{
    GTKltab *lt = static_cast<GTKltab*>(arg);
    if (lt) {
        lt->ltab_last_index = 0;
        delete [] lt->ltab_search_str;
        lt->ltab_search_str = 0;
        GList *gl = gtk_container_get_children(GTK_CONTAINER(lt->ltab_sbtn));
        GtkWidget *oldpw = GTK_WIDGET(gl->data);
        g_object_ref(oldpw);
        gtk_container_remove(GTK_CONTAINER(lt->ltab_sbtn), oldpw);
        gtk_container_add(GTK_CONTAINER(lt->ltab_sbtn), lt->ltab_lsearch);
        if (LT()->CurLayer())
            gtk_entry_set_text(GTK_ENTRY(lt->ltab_entry),
                LT()->CurLayer()->name());
        lt->ltab_timer_id = 0;
    }
    return (0);
}


// Static function.
void
GTKltab::ltab_search_hdlr(GtkWidget*, void *arg)
{
    GTKltab *lt = static_cast<GTKltab*>(arg);
    if (!lt)
        return;
    if (!lt->ltab_search_str)
        lt->ltab_search_str =
            lstring::copy(gtk_entry_get_text(GTK_ENTRY(lt->ltab_entry)));
    if (!lt->ltab_search_str)
        return;
    CDl *ld = CDldb()->search(DSP()->CurMode(), lt->ltab_last_index + 1,
        lt->ltab_search_str);
    lt->ltab_last_index = 0;
    if (!ld) {
        delete [] lt->ltab_search_str;
        lt->ltab_search_str = 0;
        return;
    }

    lt->ltab_last_index = ld->index(DSP()->CurMode());
    LT()->SetCurLayer(ld);

    if (!lt->ltab_lsearchn) {
        GdkPixbuf *pb = gdk_pixbuf_new_from_xpm_data(lsearchn_xpm);
        lt->ltab_lsearchn = gtk_image_new_from_pixbuf(pb);
        g_object_unref(pb);
        gtk_widget_show(lt->ltab_lsearchn);
    }
    GList *gl = gtk_container_get_children(GTK_CONTAINER(lt->ltab_sbtn));
    GtkWidget *oldpw = GTK_WIDGET(gl->data);
    g_object_ref(oldpw);
    gtk_container_remove(GTK_CONTAINER(lt->ltab_sbtn), oldpw);
    gtk_container_add(GTK_CONTAINER(lt->ltab_sbtn), lt->ltab_lsearchn);

    if (lt->ltab_timer_id)
        g_source_remove(lt->ltab_timer_id);
    lt->ltab_timer_id = g_timeout_add(3000, ltab_ent_timer, lt);
}


// Static function.
// Handler for pressing Enter in the text entry, will press the search
// button.
//
void
GTKltab::ltab_activate_proc(GtkWidget*, void *arg)
{
    GTKltab *lt = static_cast<GTKltab*>(arg);
    if (!lt)
        return;
    GRX->CallCallback(lt->ltab_sbtn);
}

