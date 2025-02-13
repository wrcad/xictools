
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
#include "ginterf/grvecfont.h"
#include "miscutil/filestat.h"
#include "qtinterf/qtdblsb.h"

#include <QApplication>
#include <QLayout>
#include <QLabel>
#include <QRadioButton>
#include <QCheckBox>
#include <QComboBox>
#include <QToolButton>
#include <QPushButton>


//-----------------------------------------------------------------------------
// QTlogoDlg:  Panel to control generation of physical text in layouts.
// Called from the logo button in the Physical side menu.
//
// Help keywords used in this file:
//   xic:logo

void
cEdit::PopUpLogo(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTlogoDlg::self();
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
// Associated text extent function, returns the size of the text field and
// the number of lines.
//
void
cEdit::polytextExtent(const char *string, int *widp, int *heip, int *numlines)
{
    if (XM()->RunMode() != ModeNormal || !QTmainwin::self()) {
        if (widp)
            *widp = 0;
        if (heip)
            *heip = 0;
        if (numlines)
            *numlines = 1;
        return;
    }
    const char *fname = QTlogoDlg::font(true);
    QFont *font = QTfont::self()->new_font(fname, false);
    QFontMetrics fm(*font);
    int lineht = fm.height();

    if (!string)
        string = "X";
    int nl = 1;
    int txtwid = 0;
    char *buf = lstring::copy(string);
    const char *curstr = buf;
    for (char *s = buf; ; s++) {
        if (*s == '\n' || *s == 0) {
            bool done = !*s;
            if (!done) {
                if (*(s+1))
                    nl++;
                *s = 0;
            }
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
            int lw = fm.horizontalAdvance(curstr);
#else
            int lw = fm.width(curstr);
#endif
            if (lw > txtwid)
                txtwid = lw;
            if (!done)
                curstr = s+1;
            else
                break;
        }
    }
    delete [] buf;
    if (widp)
        *widp = txtwid;
    if (heip)
        *heip = nl*lineht;
    if (numlines)
        *numlines = nl;
    delete font;
}


// Static function.
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

    int wid, hei, nl;
    polytextExtent(string, &wid, &hei, &nl);

    const char *fname = QTlogoDlg::font(true);
    QFont *font = QTfont::self()->new_font(fname, false);
    QFontMetrics fm(*font);

    QImage im(wid + 4, hei + 4, QImage::Format_RGB32);
    QPainter p(&im);
    p.setFont(*font);
    QBrush brush(QColor("white"));
    p.setBackground(brush);
    QPen pen(QColor("black"));
    p.setPen(pen);
    p.setBrush(QBrush("white"));
    p.drawRect(-1, -1, wid + 2, hei + 2);

    char *buf = lstring::copy(string);
    const char *curstr = buf;
    int lhei = hei/nl;
    int lc = 1;
    for (char *s = buf; ; s++) {
        if (*s == '\n' || *s == 0) {
            bool done = !*s;
            if (!done)
                *s = 0;
            int tx = 2;
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
            int lw = fm.horizontalAdvance(curstr);
#else
            int lw = fm.width(curstr);
#endif
            switch (ED()->horzJustify()) {
            case 1:
                tx += (wid - lw)/2;
                break;
            case 2:
                tx += wid - lw;
                break;
            }
            int ty = -2 + lc*lhei;
            p.drawText(tx, ty, curstr);

            if (!done) {
                lc++;
                curstr = s+1;
                if (!*curstr)
                    break;
            }
            else
                break;
        }
    }
    delete [] buf;
    p.end();

    Zlist *z0 = 0;
    for (int i = 0; i < hei - 1; i++) {
        for (int j = 0; j < wid - 1;  j++) {
            unsigned int px = im.pixel(j, i);
            // Many fonts are anti-aliased, the code below does a
            // semi-reasonable job of filtering the pixels.
            int r, g, b;
            QTdev::self()->RGBofPixel(px, &r, &g, &b);
            if (r + g + b <= 512) {
                Zoid Z(x + j*psz, x + (j+1)*psz, y + (lhei - i - 1)*psz,
                    x + j*psz, x + (j+1)*psz, y + (lhei - i)*psz);
                z0 = new Zlist(&Z, z0);
            }
        }
    }
    PolyList *po = Zlist::to_poly_list(z0);
    return (po);
}
// End of cEdit functions.


#define FB_SET          "Set Pretty Font"

#ifdef Q_OS_MACOS
#define DEF_FONTNAME    "Menlo 18"
#else
#ifdef Q_OS_WIN
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

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QGridLayout *grid = new QGridLayout(this);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    // Font selection radio buttons
    //
    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    grid->addLayout(hbox, 0, 0, 1, 2);

    QLabel *label = new QLabel(tr("Font:  "));
    hbox->addWidget(label);

    lgo_vector = new QRadioButton(tr("Vector"));
    hbox->addWidget(lgo_vector);
    connect(lgo_vector, &QRadioButton::toggled,
        this, &QTlogoDlg::vector_btn_slot);

    lgo_manh = new QRadioButton(tr("Manhattan"));
    hbox->addWidget(lgo_manh);
    connect(lgo_manh, &QRadioButton::toggled,
        this, &QTlogoDlg::manh_btn_slot);

    lgo_pretty = new QRadioButton(tr("Pretty"));
    hbox->addWidget(lgo_pretty);
    connect(lgo_pretty, &QRadioButton::toggled,
        this, &QTlogoDlg::pretty_btn_slot);

    hbox->addStretch(1);
    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, &QAbstractButton::clicked,
        this, &QTlogoDlg::help_btn_slot);

#if QT_VERSION >= QT_VERSION_CHECK(6,8,0)
#define CHECK_BOX_STATE_CHANGED &QCheckBox::checkStateChanged
#else
#define CHECK_BOX_STATE_CHANGED &QCheckBox::stateChanged
#endif

    // second row
    lgo_setpix = new QCheckBox(tr("Define \"pixel\" size"));
    grid->addWidget(lgo_setpix, 1, 0);
    connect(lgo_setpix, CHECK_BOX_STATE_CHANGED,
        this, &QTlogoDlg::pixel_btn_slot);

    lgo_sb_pix = new QTdoubleSpinBox();
    grid->addWidget(lgo_sb_pix, 1, 1);
    lgo_sb_pix->setDecimals(CD()->numDigits());
    lgo_sb_pix->setMinimum(MICRONS(1));
    lgo_sb_pix->setMaximum(100.0);
    lgo_sb_pix->setValue(lgo_defpixsz);
    connect(lgo_sb_pix, QOverload<double>::of(&QTdoubleSpinBox::valueChanged),
        this, &QTlogoDlg::value_changed_slot);

    // third row
    label = new QLabel(tr("     Vector end style"));
    grid->addWidget(label, 2, 0);

    lgo_endstyle = new QComboBox();
    grid->addWidget(lgo_endstyle, 2, 1);
    for (int i = 0; lgo_endstyles[i]; i++)
        lgo_endstyle->addItem(tr(lgo_endstyles[i]));
    connect(lgo_endstyle, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &QTlogoDlg::endstyle_change_slot);

    // fourth row
    label = new QLabel(tr("     Vector path width"));
    grid->addWidget(label, 3, 0);

    lgo_pwidth = new QComboBox();
    grid->addWidget(lgo_pwidth, 3, 1);
    for (int i = 0; lgo_pathwidth[i]; i++)
        lgo_pwidth->addItem(tr(lgo_pathwidth[i]));
    connect(lgo_pwidth, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &QTlogoDlg::pwidth_change_slot);

    // fifth row
    lgo_create = new QCheckBox(tr("Create cell for text"));
    grid->addWidget(lgo_create, 4, 0);
    connect(lgo_create, CHECK_BOX_STATE_CHANGED,
        this, &QTlogoDlg::create_btn_slot);

    // bottom row
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(0, 8, 0, 0);
    hbox->setSpacing(2);
    grid->addLayout(hbox, 5, 0, 6, 2);

    lgo_dump = new QToolButton();
    lgo_dump->setText(tr("Dump Vector Font "));
    hbox->addWidget(lgo_dump);
    lgo_dump->setCheckable(true);
    connect(lgo_dump, &QAbstractButton::toggled,
        this, &QTlogoDlg::dump_btn_slot);

    lgo_sel = new QToolButton();
    lgo_sel->setText(tr("Select Pretty Font"));
    hbox->addWidget(lgo_sel);
    lgo_sel->setCheckable(true);
    connect(lgo_sel, &QAbstractButton::toggled,
        this, &QTlogoDlg::sel_btn_slot);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Default");
    hbox->addWidget(btn);
    connect(btn, &QAbstractButton::clicked,
        this, &QTlogoDlg::dismiss_btn_slot);

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


#ifdef Q_OS_MACOS
#define DLGTYPE QTlogoDlg
#include "qtinterf/qtmacos_event.h"
#endif


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
        if (fp) {
            ED()->logoFont()->dumpFont(fp);
            fclose(fp);
            QTlogoDlg::self()->PopUpMessage("Logo vector font saved in file.",
                false);
        }
        else
            QTpkg::self()->Perror(lstring::strip_path(tok));
        delete [] tok;
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
QTlogoDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:logo"))
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

