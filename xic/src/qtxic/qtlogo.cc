
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
#include "geo_zlist.h"
#include "dsp_inlines.h"
#include "ginterf/grfont.h"
#include "miscutil/filestat.h"
#include "qtinterf/qtdblsb.h"

#include <QLayout>
#include <QLabel>
#include <QRadioButton>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>


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

    QTlogoDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTlogoDlg::self(),
        QTmainwin::self()->Viewport());
    QTlogoDlg::self()->show();
}


// This is not used to pop up the font selector in the QT releases,
// but still called to update and close the font selector from other
// parts of Xic.
//
void
cEdit::PopUpPolytextFont(GRobject, ShowMode mode)
{
    if (mode == MODE_OFF) {
        if (QTlogoDlg::self() && QTlogoDlg::self()->fontsel())
            QTlogoDlg::self()->fontsel()->popdown();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTlogoDlg::self() && QTlogoDlg::self()->fontsel())
            QTlogoDlg::self()->update_fontsel();
    }
}


// Static function.
// XXX FIXME: handle multi lines, justification
// Associated text extent function, returns the size of the text field and
// the number of lines.
//
void
cEdit::polytextExtent(const char *string, int *widp, int *heip, int *numlines)
{
    if (XM()->RunMode() != ModeNormal || !QTmainwin::self()) {
        *widp = 0;
        *heip = 0;
        *numlines = 1;
        return;
    }
    const char *fname = QTlogoDlg::font(true);
    QFont *font = QTfont::self()->new_font(fname, false);
    QFontMetrics fm(*font);
    if (!string)
        string = "X";
    QRect r = fm.boundingRect(string);
    *widp = r.width();
    *heip = r.height();

    int nl = 1;
    for (const char *s = string; *s; s++) {
        if (*s == '\n' && *(s+1))
            nl++;
    }
    *numlines = nl;
    delete font;
}


// Static function.
// XXX FIXME: handle multi lines, justification
// Return a polygon list for rendering the string.  Argument psz is
// the "pixel" size, x and y the position.
//
PolyList *
cEdit::polytext(const char *string, int psz, int x, int y)
{
    if (XM()->RunMode() != ModeNormal)
        return (0);
    if (!string || !*string || psz <= 0)
        return (0);
    if (!QTmainwin::self())
        return (0);

    const char *fname = QTlogoDlg::font(true);
    QFont *font = QTfont::self()->new_font(fname, false);
    QFontMetrics fm(*font);
    QRect br = fm.boundingRect(string);

    QImage im(br.width()+4, br.height()+4, QImage::Format_RGB32);
    QPainter p(&im);
    p.setFont(*font);
    QBrush brush(QColor("white"));
    p.setBackground(brush);
    QPen pen(QColor("black"));
    p.setPen(pen);
    p.setBrush(QBrush("white"));
    p.drawRect(-1, -1, br.width()+2, br.height()+2);
    p.drawText(0, -br.y(), string);
    p.end();

    Zlist *z0 = 0;
    for (int i = 0; i < br.height()-1; i++) {
        for (int j = 0; j < br.width()-1;  j++) {
            unsigned int px = im.pixel(j, i);
            // Many fonts are anti-aliased, the code below does a
            // semi-reasonable job of filtering the pixels.
            int r, g, b;
            QTdev::self()->RGBofPixel(px, &r, &g, &b);
            if (r + g + b <= 512) {
                Zoid Z(x + j*psz, x + (j+1)*psz, y + (br.height()-i-1)*psz,
                    x + j*psz, x + (j+1)*psz, y + (br.height()-i)*psz);
                z0 = new Zlist(&Z, z0);
            }
        }
    }
    PolyList *po = Zlist::to_poly_list(z0);
    return (po);
}
// End of cEdit functions.


#define FB_SET          "Set Pretty Font"

#ifdef __APPLE__
#define DEF_FONTNAME    "Menlo 18"
#else
#ifdef WIN32
#define DEF_FONTNAME    "Menlo 18"
#else
#define DEF_FONTNAME    "Courier New 18"
#endif
#endif

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
char *QTlogoDlg::lgo_font = 0;
QTlogoDlg *QTlogoDlg::instPtr;

QTlogoDlg::QTlogoDlg(GRobject c) : QTbag(this)
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
    lgo_sb_pix = 0;
    lgo_sav_pop = 0;
    lgo_fontsel = 0;

    setWindowTitle(tr("Logo Font Setup"));
    setAttribute(Qt::WA_DeleteOnClose);
//    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    // Font selection radio buttons
    //
    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
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
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QVBoxLayout *col1 = new QVBoxLayout();
    col1->setContentsMargins(qm);
    col1->setSpacing(2);
    hbox->addLayout(col1);

    QVBoxLayout *col2 = new QVBoxLayout();
    col2->setContentsMargins(qm);
    col2->setSpacing(2);
    hbox->addLayout(col2);

    lgo_setpix = new QCheckBox(tr("Define \"pixel\" size"));
    col1->addWidget(lgo_setpix);
    connect(lgo_setpix, SIGNAL(stateChanged(int)),
        this, SLOT(pixel_btn_slot(int)));

    lgo_sb_pix = new QTdoubleSpinBox();
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
    lgo_dump->setAutoDefault(false);
    connect(lgo_dump, SIGNAL(toggled(bool)),
        this, SLOT(dump_btn_slot(bool)));

    // bottom row
    lgo_sel = new QPushButton(tr("Select Pretty Font"));
    col1->addWidget(lgo_sel);
    lgo_sel->setCheckable(true);
    lgo_sel->setAutoDefault(false);
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
    if (lgo_fontsel)
        lgo_fontsel->popdown();
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


void
QTlogoDlg::update_fontsel()
{
    const char *fn = CDvdb()->getVariable(VA_LogoPrettyFont);
    if (!fn)
        fn = DEF_FONTNAME;
    delete [] lgo_font;
    lgo_font = lstring::copy(fn);
    if (lgo_fontsel) {
        lgo_fontsel->set_font_name(fn);
        lgo_fontsel->update_label("Pretty font updated.");
        QTpkg::self()->AddTimer(2000, lgo_upd_label, 0);
    }
}


// Static function.
const char *
QTlogoDlg::font(bool init)
{
    if (init && !lgo_font)
        lgo_font = lstring::copy(DEF_FONTNAME);
    return (lgo_font);
}


// Static function.
// Timer callback to erase message label in Font Selector.
int
QTlogoDlg::lgo_upd_label(void*)
{
    if (instPtr && instPtr->lgo_fontsel)
        instPtr->lgo_fontsel->update_label(0);
    return (false);
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


// Static function.
// Callback passed to the font selector.  The variable
// LogoPrettyFont is set to the name of the selected font.
//
void
QTlogoDlg::lgo_cb(const char *name, const char *fname, void*)
{
    if (QTlogoDlg::self() && name && !strcmp(name, FB_SET) && *fname)
        CDvdb()->setVariable(VA_LogoPrettyFont, fname);
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
    if (state && !lgo_fontsel) {
        lgo_fontsel = new QTfontDlg(QTmainwin::self(), 0, 0);
        lgo_fontsel->register_caller(lgo_sel);
        lgo_fontsel->register_callback(lgo_cb);
        lgo_fontsel->register_usrptr((void**)&lgo_fontsel);
        lgo_fontsel->set_apply_btn_name(FB_SET);
        lgo_fontsel->set_transient_for(QTmainwin::self());

        QTdev::self()->SetPopupLocation(GRloc(LW_LR),lgo_fontsel,
            QTmainwin::self()->Viewport());
        update_fontsel();
        lgo_fontsel->show();
    }
    else {
        if (lgo_fontsel) {
            lgo_fontsel->popdown();
        }
    }
}


void
QTlogoDlg::dismiss_btn_slot()
{
    ED()->PopUpLogo(0, MODE_OFF);
}

