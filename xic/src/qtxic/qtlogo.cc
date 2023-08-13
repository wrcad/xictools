
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

#include "qtlogo.h"
#include "edit.h"
#include "dsp_inlines.h"
#include "ginterf/grfont.h"
#include "miscutil/filestat.h"

#include <QLayout>
#include <QLabel>
#include <QRadioButton>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QDoubleSpinBox>


void
cEdit::PopUpLogo(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTlogoDlg::self())
            QTlogoDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTlogoDlg::self())
            QTlogoDlg::self()->update();
        return;
    }
    if (QTlogoDlg::self())
        return;

    new QTlogoDlg(caller);

    QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTlogoDlg::self(),
        QTmainwin::self()->Viewport());
    QTlogoDlg::self()->show();
}


// XXX this need not be global, part of logo panel only.
// Pop up the font selector to enable the user to select a font for
// use with the logo command.
//
void
cEdit::PopUpPolytextFont(GRobject caller, ShowMode mode)
{
    /*
    if (!QTdev::exists() || !QTmainwin::exists())
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
        GTK_WINDOW(GTKmainwin::self()->Shell()));

    GTKdev::self()->SetPopupLocation(GRloc(), Pf->shell(),
        GTKmainwin::self()->Viewport());
    gtk_widget_show(Pf->shell());
    */
}


// Static function.
// Associated text extent function, returns the size of the text field and
// the number of lines.
//
void
cEdit::polytextExtent(const char *string, int *width, int *height,
    int *numlines)
{
    if (XM()->RunMode() != ModeNormal || !QTmainwin::self()) {
        *width = 0;
        *height = 0;
        *numlines = 1;
        return;
    }
/*
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
    PangoLayout *lout = gtk_widget_create_pango_layout(
        GTKmainwin::self()->Viewport(), string);
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
*/
}


// Static function.
// Return a polygon list for rendering the string.  Argument psz is
// the "pixel" size, x and y the position, and lwid the line width.
//
PolyList *
cEdit::polytext(const char *string, int psz, int x, int y)
{
    /*
    if (XM()->RunMode() != ModeNormal)
        return (0);
    if (psz <= 0)
        return (0);
    if (!GTKmainwin::self())
        return (0);
    if (!sPf::font())
        sPf::init_font();
    if (!sPf::font())
        return (0);

    int wid, hei, numlines;
    polytextExtent(string, &wid, &hei, &numlines);

#if GTK_CHECK_VERSION(3,0,0)
    GdkWindow *window = GTKmainwin::self()->GetDrawable()->get_window();
    ndkPixmap *pixmap = new ndkPixmap(window, wid, hei);
    ndkGC *gc = new ndkGC(window);
    GdkColor c;
    c.pixel = 0xffffff;  // white
    gc->set_background(&c);
    gc->set_foreground(&c);
    gc->clear(pixmap);
    c.pixel = 0;  // black
    gc->set_foreground(&c);
#else
    GdkPixmap *pixmap = gdk_pixmap_new(GTKmainwin::self()->Window(), wid, hei,
        GTKdev::self()->Visual()->depth);
    GdkGC *gc = gdk_gc_new(GTKmainwin::self()->Window());
    GdkColor c;
    c.pixel = 0xffffff;  // white
    gdk_gc_set_background(gc, &c);
    gdk_gc_set_foreground(gc, &c);
    gdk_draw_rectangle(pixmap, gc, true, 0, 0, wid, hei);
    c.pixel = 0;  // black
    gdk_gc_set_foreground(gc, &c);
#endif

    PangoFontDescription *pfd =
        pango_font_description_from_string(sPf::font());

    int fw, fh;
    PangoLayout *lout =
        gtk_widget_create_pango_layout(GTKmainwin::self()->Viewport(), "X");
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
        lout = gtk_widget_create_pango_layout(GTKmainwin::self()->Viewport(),
            ctmp);
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
#if GTK_CHECK_VERSION(3,0,0)
        pixmap->copy_from_pango_layout(gc, tx, ty, lout);
#else
        gdk_draw_layout(pixmap, gc, tx, ty, lout);
#endif
        g_object_unref(lout);
        t = s+1;
        s = strchr(t, '\n');
        ty += fh;
    }
    int tx = 0;
    lout = gtk_widget_create_pango_layout(GTKmainwin::self()->Viewport(), t);
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
#if GTK_CHECK_VERSION(3,0,0)
    pixmap->copy_from_pango_layout(gc, tx, ty, lout);
#else
    gdk_draw_layout(pixmap, gc, tx, ty, lout);
#endif

    g_object_unref(lout);
    pango_font_description_free(pfd);
    y -= psz*(hei - fh);

#if GTK_CHECK_VERSION(3,0,0)
    ndkImage *im = new ndkImage(pixmap, 0, 0, wid, hei);
    pixmap->dec_ref();
    delete gc;
#else
    GdkImage *im = gdk_image_get(pixmap, 0, 0, wid, hei);
    gdk_pixmap_unref(pixmap);
    gdk_gc_unref(gc);
#endif

    Zlist *z0 = 0;
    for (int i = 0; i < hei; i++) {
        for (int j = 0; j < wid;  j++) {
#if GTK_CHECK_VERSION(3,0,0)
            int px = im->get_pixel(j, i);
#else
            int px = gdk_image_get_pixel(im, j, i);
#endif
            // Many fonts are anti-aliased, the code below does a
            // semi-reasonable job of filtering the pixels.
            int r, g, b;
            GTKdev::self()->RGBofPixel(px, &r, &g, &b);
            if (r + g + b <= 512) {
                Zoid Z(x + j*psz, x + (j+1)*psz, y + (hei-i-1)*psz,
                    x + j*psz, x + (j+1)*psz, y + (hei-i)*psz);
                z0 = new Zlist(&Z, z0);
            }
        }
    }
#if GTK_CHECK_VERSION(3,0,0)
    delete im;
#else
    gdk_image_destroy(im);
#endif
    PolyList *po = Zlist::to_poly_list(z0);
    return (po);
    */
return (0);
}

// End of cEdit functions.


const char *QTlogoDlg::lgo_endstyles[] =
{
    "flush",
    "rounded",
    "extended",
    0
};

const char *QTlogoDlg::lgo_pathwidth[] =
{
    "thinner",
    "thin",
    "medium",
    "thick",
    "thicker",
    0
};

double QTlogoDlg::lgo_defpixsz = 1.0;
QTlogoDlg *QTlogoDlg::instPtr;

QTlogoDlg::QTlogoDlg(GRobject c)
{
    instPtr = this;
    lgo_caller = c;
    lgo_vector = 0;
    lgo_manh = 0;
    lgo_pretty = 0;
    lgo_setpix = 0;
    lgo_endstyle = 0;
    lgo_pwidth = 0;
    lgo_create = 0;
    lgo_dump = 0;
    lgo_sel = 0;
    lgo_sav_pop = 0;

    setWindowTitle(tr("Logo Font Setup"));
    setWindowFlags(Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);

//    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    // Font selection radio buttons
    //
    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QLabel *label = new QLabel(tr("Font:  "));
    hbox->addWidget(label);

    lgo_vector = new QRadioButton(tr("Vector"));
    hbox->addWidget(lgo_vector);
    connect(lgo_vector, SIGNAL(toggled(bool)),
        this, SLOT(vector_btn_slot(bool)));

    lgo_manh = new QRadioButton(tr("Manhattan"));
    hbox->addWidget(lgo_manh);
    connect(lgo_manh, SIGNAL(toggled(bool)),
        this, SLOT(manh_btn_slot(bool)));

    lgo_pretty = new QRadioButton(tr("Pretty"));
    hbox->addWidget(lgo_pretty);
    connect(lgo_pretty, SIGNAL(toggled(bool)),
        this, SLOT(pretty_btn_slot(bool)));

    // second row
    hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QVBoxLayout *col1 = new QVBoxLayout();
    col1->setMargin(0);
    col1->setSpacing(2);
    hbox->addLayout(col1);

    QVBoxLayout *col2 = new QVBoxLayout();
    col2->setMargin(0);
    col2->setSpacing(2);
    hbox->addLayout(col2);

    lgo_setpix = new QCheckBox(tr("Define \"pixel\" size"));
    col1->addWidget(lgo_setpix);
    connect(lgo_setpix, SIGNAL(stateChanged(int)),
        this, SLOT(pixel_btn_slot(int)));

    lgo_sb_pix = new QDoubleSpinBox();
    col2->addWidget(lgo_sb_pix);
    lgo_sb_pix->setDecimals(CD()->numDigits());
    lgo_sb_pix->setMinimum(MICRONS(1));
    lgo_sb_pix->setMaximum(100.0);
    lgo_sb_pix->setValue(lgo_defpixsz);
    connect(lgo_sb_pix, SIGNAL(valueChanged(double)),
        this, SLOT(value_changed_slot(double)));

    // third row
    label = new QLabel(tr("Vector end style"));
    col1->addWidget(label);

    lgo_endstyle = new QComboBox();
    col2->addWidget(lgo_endstyle);
    for (int i = 0; lgo_endstyles[i]; i++)
        lgo_endstyle->addItem(tr(lgo_endstyles[i]));
    connect(lgo_endstyle, SIGNAL(currentIndexChanged(int)),
        this, SLOT(endstyle_change_slot(int)));

    // fourth row
    label = new QLabel(tr("Vector path width"));
    col1->addWidget(label);

    lgo_pwidth = new QComboBox();
    col2->addWidget(lgo_pwidth);
    for (int i = 0; lgo_pathwidth[i]; i++)
        lgo_pwidth->addItem(tr(lgo_pathwidth[i]));
    connect(lgo_pwidth, SIGNAL(currentIndexChanged(int)),
        this, SLOT(pwidth_change_slot(int)));

    // fifth row
    lgo_create = new QCheckBox(tr("Create cell for text"));
    col1->addWidget(lgo_create);
    connect(lgo_create, SIGNAL(stateChanged(int)),
        this, SLOT(create_btn_slot(int)));

    lgo_dump = new QPushButton(tr("Dump Vector Font "));
    col2->addWidget(lgo_dump);
    lgo_dump->setCheckable(true);
    connect(lgo_dump, SIGNAL(toggled(bool)),
        this, SLOT(dump_btn_slot(bool)));

    // bottom row
    lgo_sel = new QPushButton(tr("Select Pretty Font"));
    col1->addWidget(lgo_sel);
    lgo_sel->setCheckable(true);
    connect(lgo_sel, SIGNAL(toggled(bool)),
        this, SLOT(sel_btn_slot(bool)));

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    col2->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QTlogoDlg::~QTlogoDlg()
{
    instPtr = 0;
    ED()->PopUpPolytextFont(0, MODE_OFF);
    if (lgo_caller)
        QTdev::Deselect(lgo_caller);
    if (lgo_sav_pop)
        lgo_sav_pop->popdown();
}


namespace {
    inline bool str_to_int(int *iret, const char *s)
    {
        if (!s)
            return (false);
        return (sscanf(s, "%d", iret) == 1);
    }
}


void
QTlogoDlg::update()
{
    int dd;
    if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoAltFont)) &&
            dd >= 0 && dd <= 1) {
        if (dd == 0)
            QTdev::SetStatus(lgo_manh, true);
        else
            QTdev::SetStatus(lgo_pretty, true);
    }
    else
        QTdev::SetStatus(lgo_vector, true);

    const char *pix = CDvdb()->getVariable(VA_LogoPixelSize);
    if (pix) {
        char *nstr;
        double d = strtod(pix, &nstr);
        if (nstr != pix)
            lgo_sb_pix->setValue(d);
        QTdev::SetStatus(lgo_setpix, true);
        lgo_sb_pix->setEnabled(true);
    }
    else {
        QTdev::SetStatus(lgo_setpix, false);
        lgo_sb_pix->setEnabled(false);
    }
    ED()->assert_logo_pixel_size();

    if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoPathWidth)) &&
            dd >= 1 && dd <= 5)
        lgo_pwidth->setCurrentIndex(dd-1);
    else
        lgo_pwidth->setCurrentIndex(DEF_LOGO_PATH_WIDTH-1);

    if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoEndStyle)) &&
            dd >= 0 && dd <= 2)
        lgo_endstyle->setCurrentIndex(dd);
    else
        lgo_endstyle->setCurrentIndex(DEF_LOGO_END_STYLE);

    if (CDvdb()->getVariable(VA_LogoToFile))
        QTdev::SetStatus(lgo_create, true);
    else
        QTdev::SetStatus(lgo_create, false);
}


// Static function.
// Callback for the Save dialog.
//
ESret
QTlogoDlg::lgo_sav_cb(const char *fname, void*)
{
    if (!fname)
        return (ESTR_IGN);
    char *tok = lstring::getqtok(&fname);
    if (!tok)
        return (ESTR_IGN);
    if (filestat::create_bak(tok)) {
        FILE *fp = fopen(tok, "w");
        delete [] tok;
        if (fp) {
            ED()->logoFont()->dumpFont(fp);
            fclose(fp);
            QTlogoDlg::self()->PopUpMessage("Logo vector font saved in file.",
                false);
        }
        else
            QTpkg::self()->Perror(lstring::strip_path(tok));
    }
    else
        QTpkg::self()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
    delete [] tok;
    return (ESTR_DN);
}


void
QTlogoDlg::vector_btn_slot(bool state)
{
    if (state)
        CDvdb()->clearVariable(VA_LogoAltFont);
}


void
QTlogoDlg::manh_btn_slot(bool state)
{
    if (state)
        CDvdb()->setVariable(VA_LogoAltFont, "0");
}


void
QTlogoDlg::pretty_btn_slot(bool state)
{
    if (state)
        CDvdb()->setVariable(VA_LogoAltFont, "1");
}


void
QTlogoDlg::pixel_btn_slot(int state)
{
    if (state) {
        const char *s =
            lstring::copy(lgo_sb_pix->text().toLatin1().constData());
        CDvdb()->setVariable(VA_LogoPixelSize, s);
        delete [] s;
    }
    else
        CDvdb()->clearVariable(VA_LogoPixelSize);
}


void
QTlogoDlg::value_changed_slot(double d)
{
    lgo_defpixsz = d;
    if (QTdev::GetStatus(lgo_setpix)) {
        const char *s =
            lstring::copy(lgo_sb_pix->text().toLatin1().constData());
        CDvdb()->setVariable(VA_LogoPixelSize, s);
        delete [] s;
    }
}


void
QTlogoDlg::endstyle_change_slot(int es)
{
    if (es != DEF_LOGO_END_STYLE) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", es);
        CDvdb()->setVariable(VA_LogoEndStyle, buf);
    }
    else
        CDvdb()->clearVariable(VA_LogoEndStyle);
}


void
QTlogoDlg::pwidth_change_slot(int pw)
{
    pw++;
    if (pw != DEF_LOGO_PATH_WIDTH) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", pw);
        CDvdb()->setVariable(VA_LogoPathWidth, buf);
    }
    else
        CDvdb()->clearVariable(VA_LogoPathWidth);
}


void
QTlogoDlg::create_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_LogoToFile, "");
    else
        CDvdb()->clearVariable(VA_LogoToFile);
}


void
QTlogoDlg::dump_btn_slot(bool state)
{
    if (lgo_sav_pop)
        lgo_sav_pop->popdown();
    if (state) {
        lgo_sav_pop = PopUpEditString((GRobject)lgo_dump,
            GRloc(), "Enter pathname for font file: ",
            XM()->LogoFontFileName(), lgo_sav_cb, 0, 250, 0, false, 0);
        if (lgo_sav_pop)
            lgo_sav_pop->register_usrptr((void**)&lgo_sav_pop);
    }
}


void
QTlogoDlg::sel_btn_slot(bool state)
{
    if (state)
        ED()->PopUpPolytextFont(lgo_sel, MODE_ON);
    else
        ED()->PopUpPolytextFont(0, MODE_OFF);
}


void
QTlogoDlg::dismiss_btn_slot()
{
    ED()->PopUpLogo(0, MODE_OFF);
}


//XXX merge with logo panel
#ifdef notdef
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

    pf_fontsel = new GTKfontPopup(GTKmainwin::self(), -1, 0, pf_btns, "");
    pf_fontsel->register_caller(caller);
    pf_fontsel->register_callback(pf_cb);
    pf_fontsel->register_usrptr((void**)&pf_fontsel);

    // The GTK-2 font widget needs to be initialized with an idle
    // proc, fix this sometime.
    GTKpkg::self()->RegisterIdleProc(pf_upd_idle, 0);
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
        GTKpkg::self()->AddTimer(1000, pf_updlabel, 0);
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

#endif

