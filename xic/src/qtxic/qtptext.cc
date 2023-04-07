
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
#include "edit.h"
#include "geo_zlist.h"
#include "qtmain.h"
#include "qtfont.h"
//#include "qtinlines.h"


//-----------------------------------------------------------------------------
// The following exports support the use of X fonts in the "logo" command.


// Update the label in the pop-up.
//
void
polytext_update_label()
{
/*
    if (fsel_label) {
        char *str = "internal vector";
        if (GetVariable(VA_LogoPretty))
            str = "pretty";
        else if (GetVariable(VA_LogoManhattan))
            str = "Manhattan";
        char buf[256];
        sprintf(buf, "Logo command will use %s font.", str);
        gtk_label_set(GTK_LABEL(fsel_label), buf);
    }
*/
}


// Pop up the font selector to enable the user to select a font for use with
// the logo command.
//
void
polytext_setfont()
{
/*
    if (XM()->RunMode != ModeNormal)
        return;
    // Use a dummy widget_bag since the main bag allows only one font
    // selector, to allow this and the one from the Attributes menu to
    // coexist.
    static widget_bag wb;
    wb.shell = GTKmainWin->shell;
    wb.PopUpFontSel(0, GRloc(), MODE_ON, fontcb, 0, -1, fontbtns, "");
    wb.shell = 0;
    if (wb.fontsel) {
        fsel_label = (GtkWidget*)gtk_object_get_data(GTK_OBJECT(wb.fontsel),
            "label");
        polytext_update_label();
    }
    const char *ft = GetVariable(VA_LogoPrettyFont);
    if (wb.fontsel && ft && *ft)
        gtk_font_selection_set_font_name(GTK_FONT_SELECTION(wb.fontsel), ft);
*/
}


// Return a polygon list for rendering the string.  Argument psz is
// the "pixel" size, x and y the position, and lwid the line width.
//
PolyList *
polytext(const char *string, int psz, int x, int y)
{
/*
    if (XM()->RunMode != ModeNormal)
        return (0);
    if (psz <= 0)
        return (0);
    if (!polytext_font)
        fontcb(FB_SET, polytext_def_fontname, 0);
    if (!polytext_font)
        return (0);

    int wid, hei, numlines;
    polytext_extent(string, &wid, &hei, &numlines);
    GdkPixmap *pixmap = gdk_pixmap_new(GTKmainWin->window, wid, hei,
        GTKdev::visual->depth); 

    GdkGC *gc = gdk_gc_new(GTKmainWin->window);
    GdkColor c;
    c.pixel = BlackPixel(gr_x_display(), 0);
    gdk_gc_set_background(gc, &c);
    gdk_gc_set_foreground(gc, &c);
    gdk_draw_rectangle(pixmap, gc, true, 0, 0, wid, hei);
    c.pixel = WhitePixel(gr_x_display(), 0);
    gdk_gc_set_foreground(gc, &c);

#if GTK_CHECK_VERSION(1,3,15)
    PangoFontDescription *pfd =
        pango_font_description_from_string(polytext_font);

    int fw, fh;
    PangoLayout *lout =
        gtk_widget_create_pango_layout(GTKmainWin->viewport, "X");
    pango_layout_set_font_description(lout, pfd);
    pango_layout_get_pixel_size(lout, &fw, &fh);
    g_object_unref(lout);

    int ty = 0;
    const char *t = string;
    char *s = strchr(t, '\n');
    while (s) {
        int tx = 0;
        char *ctmp = new char[s-t + 1];
        strncpy(ctmp, t, s-t);
        ctmp[s-t] = 0;
        lout = gtk_widget_create_pango_layout(GTKmainWin->viewport, ctmp);
        delete [] ctmp;
        pango_layout_set_font_description(lout, pfd);
        int len, xx;
        pango_layout_get_pixel_size(lout, &len, &xx);
        switch (XM()->h_justify) {
        case 1:
            tx += (wid - len)/2;
            break;
        case 2:
            tx += wid - len;
            break;
        }
        gdk_draw_layout(pixmap, gc, tx, ty, lout);
        g_object_unref(lout);
        t = s+1;
        s = strchr(t, '\n');
        ty += fh;
    }
    int tx = 0;
    lout = gtk_widget_create_pango_layout(GTKmainWin->viewport, t);
    pango_layout_set_font_description(lout, pfd);
    int len, xx;
    pango_layout_get_pixel_size(lout, &len, &xx);
    switch (XM()->h_justify) {
    case 1:
        tx += (wid - len)/2;
        break;
    case 2:
        tx += wid - len;
        break;
    }
    gdk_draw_layout(pixmap, gc, tx, ty, lout);
    g_object_unref(lout);
    pango_font_description_free(pfd);
    y -= psz*(hei - fh);

#else

    int ty = polytext_font->ascent;
    const char *t = string;
    char *s = strchr(t, '\n');
    while (s) {
        int tx = 0;
        int len = gdk_text_width(polytext_font, t, s-t);
        switch (XM()->h_justify) {
        case 1:
            tx += (wid - len)/2;
            break;
        case 2:
            tx += wid - len;
            break;
        }
        gdk_draw_text(pixmap, polytext_font, gc, tx, ty, t, s - t);
        ty += polytext_font->ascent + polytext_font->descent;
        t = s+1;
        s = strchr(t, '\n');
    }
    int tx = 0;
    int len = gdk_string_width(polytext_font, t);
    switch (XM()->h_justify) {
    case 1:
        tx += (wid - len)/2;
        break;
    case 2:
        tx += wid - len;
        break;
    }
    gdk_draw_string(pixmap, polytext_font, gc, tx, ty, t);
    y -= psz*(hei - polytext_font->ascent - polytext_font->descent);
#endif

    GdkImage *im = gdk_image_get(pixmap, 0, 0, wid, hei);
    gdk_pixmap_unref(pixmap);
    gdk_gc_unref(gc);

    int bg = BlackPixel(gr_x_display(), 0);
    Zlist *z0 = 0;
    for (int i = 0; i < hei; i++) {
        for (int j = 0; j < wid;  j++) {
            int px = gdk_image_get_pixel(im, j, i);
            if (px != bg) {
                Zoid Z(x + j*psz, x + (j+1)*psz, y + (hei-i-1)*psz,
                    x + j*psz, x + (j+1)*psz, y + (hei-i)*psz);
                z0 = new Zlist(&Z, z0);
            }
        }
    }
    gdk_image_destroy(im);
    PolyList *po = z0->toPolyList();
    return (po);
*/
(void)string;
(void)psz;
(void)x;
(void)y;
return (0);
}


// Associated text extent function, returns the size of the text field and
// the number of lines
//
void
polytext_extent(const char *string, int *width, int *height, int *numlines)
{
/*
    if (XM()->RunMode != ModeNormal) {
        *width = 0;
        *height = 0;
        *numlines = 1;
        return;
    }
    if (!polytext_font)
        fontcb(FB_SET, polytext_def_fontname, 0);
    if (!polytext_font) {
        *width = 0;
        *height = 0;
        *numlines = 1;
        return;
    }
#if GTK_CHECK_VERSION(1,3,15)
    PangoFontDescription *pfd =
        pango_font_description_from_string(polytext_font);
    PangoLayout *lout = gtk_widget_create_pango_layout(GTKmainWin->viewport,
        string);
    pango_layout_set_font_description(lout, pfd);
    pango_layout_get_pixel_size(lout, width, height);
    g_object_unref(lout);
    pango_font_description_free(pfd);
    int nl = 1;
    for (const char *s = string; *s; s++) {
        if (*s == '\n' && *(s+1))
            nl++;
    }
    *numlines = nl;
#else
    int mlen = 0;
    int nl = 1;
    const char *t = string;
    char *s = strchr(t, '\n');
    while (s) {
        nl++;
        int len = gdk_text_width(polytext_font, t, s-t);
        if (mlen < len)
            mlen = len;
        t = s+1;
        s = strchr(t, '\n');
    }
    int len = gdk_string_width(polytext_font, t);
    if (mlen < len)
        mlen = len;
    t = string + strlen(string) - 1;
    if (*t == '\n')
        nl--;
    *width = mlen;
    *height = nl*(polytext_font->ascent + polytext_font->descent);
    *numlines = nl;
#endif
*/
(void)string;
(void)width;
(void)height;
(void)numlines;
}

