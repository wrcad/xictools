
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
#include "gtkmain.h"
#include "gtkfont.h"
#include "gtkinlines.h"


//-----------------------------------------------------------------------------
// The following exports support the use of X fonts in the "logo" command.
//

#define FB_SET    "Set Pretty Font"

#define DEF_FONTNAME "monospace 16"

namespace {
    struct sPf
    {
        sPf(GRobject);
        ~sPf();

        GtkWidget *shell()
            {
                return (pf_fontsel ? pf_fontsel->Shell() : 0);
            }

        GTKfontPopup *fontsel()     { return (pf_fontsel); }

        static char *font()         { return (pf_font); }

        static void update();
        static void init_font();

    private:
        static void pf_cb(const char*, const char*, void*);
        static int pf_updlabel(void*);
        static int pf_upd_idle(void*);

        const char      *pf_btns[2];
        GTKfontPopup    *pf_fontsel;
        static char     *pf_font;
    };

    sPf *Pf;
}

char *sPf::pf_font = 0;


sPf::sPf(GRobject caller)
{
    Pf = this;
    pf_btns[0] = FB_SET;
    pf_btns[1] = 0;
    pf_fontsel = 0;
    pf_font = 0;

    pf_fontsel = new GTKfontPopup(mainBag(), -1, 0, pf_btns, "");
    pf_fontsel->register_caller(caller);
    pf_fontsel->register_callback(pf_cb);
    pf_fontsel->register_usrptr((void**)&pf_fontsel);

    // The GTK-2 font widget needs to be initialized with an idle
    // proc, fix this sometime.
    dspPkgIf()->RegisterIdleProc(pf_upd_idle, 0);
}


sPf::~sPf()
{
    Pf = 0;
    if (pf_fontsel)
        pf_fontsel->popdown();
}


// Static function.
void
sPf::update()
{
    const char *fn = CDvdb()->getVariable(VA_LogoPrettyFont);
    if (!fn)
        fn = DEF_FONTNAME;
    delete [] pf_font;
    pf_font = lstring::copy(fn);
    if (Pf && Pf->pf_fontsel) {
        Pf->pf_fontsel->set_font_name(fn);
        Pf->pf_fontsel->update_label("Pretty font updated.");
        GRpkgIf()->AddTimer(1000, pf_updlabel, 0);
    }
}


// Static function.
void
sPf::init_font()
{
    pf_font = lstring::copy(DEF_FONTNAME);
}


// Static function.
// Callback passed to the font selector.  The variable
// LogoPrettyFont is set to the name of the selected font.
//
void
sPf::pf_cb(const char *name, const char *fname, void*)
{
    if (Pf && name && !strcmp(name, FB_SET) && *fname)
        CDvdb()->setVariable(VA_LogoPrettyFont, fname);
    else if (!name && !fname) {
        // Dismiss button pressed.
        if (Pf) {
            // Get here if called from fontsel destructor.
            Pf->pf_fontsel = 0;
            delete Pf;
        }
    }
}


// Static function.
int
sPf::pf_updlabel(void*)
{
    if (Pf && Pf->pf_fontsel)
        Pf->pf_fontsel->update_label("");
    return (false);
}


// Static function.
int
sPf::pf_upd_idle(void*)
{
    Pf->update();
    return (0);
}
// End of sPf functions.


// Pop up the font selector to enable the user to select a font for
// use with the logo command.
//
void
cEdit::PopUpPolytextFont(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete Pf;
        return;
    }
    if (mode == MODE_UPD) {
        sPf::update();
        return;
    }
    if (Pf)
        return;

    new sPf(caller);
    if (!Pf->shell()) {
        delete Pf;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(Pf->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), Pf->shell(), mainBag()->Viewport());
    gtk_widget_show(Pf->shell());
}


// Static function.
// Return a polygon list for rendering the string.  Argument psz is
// the "pixel" size, x and y the position, and lwid the line width.
//
PolyList *
cEdit::polytext(const char *string, int psz, int x, int y)
{
    if (XM()->RunMode() != ModeNormal)
        return (0);
    if (psz <= 0)
        return (0);
    if (!mainBag())
        return (0);
    if (!sPf::font())
        sPf::init_font();
    if (!sPf::font())
        return (0);

    int wid, hei, numlines;
    polytextExtent(string, &wid, &hei, &numlines);
    GdkPixmap *pixmap = gdk_pixmap_new(mainBag()->Window(), wid, hei,
        GRX->Visual()->depth);

    GdkGC *gc = gdk_gc_new(mainBag()->Window());
    GdkColor c;
    gdk_color_white(GRX->Colormap(), &c);
    gdk_gc_set_background(gc, &c);
    gdk_gc_set_foreground(gc, &c);
    gdk_draw_rectangle(pixmap, gc, true, 0, 0, wid, hei);
    gdk_color_black(GRX->Colormap(), &c);
    gdk_gc_set_foreground(gc, &c);

    PangoFontDescription *pfd =
        pango_font_description_from_string(sPf::font());

    int fw, fh;
    PangoLayout *lout =
        gtk_widget_create_pango_layout(mainBag()->Viewport(), "X");
    pango_layout_set_font_description(lout, pfd);
    pango_layout_get_pixel_size(lout, &fw, &fh);
    g_object_unref(lout);

    int ty = 0;
    const char *t = string;
    const char *s = strchr(t, '\n');
    while (s) {
        int tx = 0;
        char *ctmp = new char[s-t + 1];
        strncpy(ctmp, t, s-t);
        ctmp[s-t] = 0;
        lout = gtk_widget_create_pango_layout(mainBag()->Viewport(), ctmp);
        delete [] ctmp;
        pango_layout_set_font_description(lout, pfd);
        int len, xx;
        pango_layout_get_pixel_size(lout, &len, &xx);
        switch (ED()->horzJustify()) {
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
    lout = gtk_widget_create_pango_layout(mainBag()->Viewport(), t);
    pango_layout_set_font_description(lout, pfd);
    int len, xx;
    pango_layout_get_pixel_size(lout, &len, &xx);
    switch (ED()->horzJustify()) {
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

    GdkImage *im = gdk_image_get(pixmap, 0, 0, wid, hei);
    gdk_pixmap_unref(pixmap);
    gdk_gc_unref(gc);

    Zlist *z0 = 0;
    for (int i = 0; i < hei; i++) {
        for (int j = 0; j < wid;  j++) {
            int px = gdk_image_get_pixel(im, j, i);
            // Many fonts are anti-aliased, the code below does a
            // semi-reasonable job of filtering the pixels.
            int r, g, b;
            GRX->RGBofPixel(px, &r, &g, &b);
            if (r + g + b <= 512) {
                Zoid Z(x + j*psz, x + (j+1)*psz, y + (hei-i-1)*psz,
                    x + j*psz, x + (j+1)*psz, y + (hei-i)*psz);
                z0 = new Zlist(&Z, z0);
            }
        }
    }
    gdk_image_destroy(im);
    PolyList *po = Zlist::to_poly_list(z0);
    return (po);
}


// Static function.
// Associated text extent function, returns the size of the text field and
// the number of lines.
//
void
cEdit::polytextExtent(const char *string, int *width, int *height,
    int *numlines)
{
    if (XM()->RunMode() != ModeNormal || !mainBag()) {
        *width = 0;
        *height = 0;
        *numlines = 1;
        return;
    }
    if (!sPf::font())
        sPf::init_font();
    if (!sPf::font()) {
        *width = 0;
        *height = 0;
        *numlines = 1;
        return;
    }
    PangoFontDescription *pfd =
        pango_font_description_from_string(sPf::font());
    PangoLayout *lout = gtk_widget_create_pango_layout(mainBag()->Viewport(),
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
}

