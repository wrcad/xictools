
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
        if (cLogo::self())
            cLogo::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (cLogo::self())
            cLogo::self()->update();
        return;
    }
    if (cLogo::self())
        return;

    new cLogo(caller);

    QTdev::self()->SetPopupLocation(GRloc(LW_LL), cLogo::self(),
        QTmainwin::self()->Viewport());
    cLogo::self()->show();
}
// End of cEdit functions.


const char *cLogo::lgo_endstyles[] =
{
    "flush",
    "rounded",
    "extended",
    0
};

const char *cLogo::lgo_pathwidth[] =
{
    "thinner",
    "thin",
    "medium",
    "thick",
    "thicker",
    0
};

double cLogo::lgo_defpixsz = 1.0;
cLogo *cLogo::instPtr;

cLogo::cLogo(GRobject c)
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
        this, SLOT(dumb_btn_slot(bool)));

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


cLogo::~cLogo()
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
cLogo::update()
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
cLogo::lgo_sav_cb(const char *fname, void*)
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
            cLogo::self()->PopUpMessage("Logo vector font saved in file.",
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
cLogo::vector_btn_slot(bool state)
{
    if (state)
        CDvdb()->clearVariable(VA_LogoAltFont);
}


void
cLogo::manh_btn_slot(bool state)
{
    if (state)
        CDvdb()->setVariable(VA_LogoAltFont, "0");
}


void
cLogo::pretty_btn_slot(bool state)
{
    if (state)
        CDvdb()->setVariable(VA_LogoAltFont, "1");
}


void
cLogo::pixel_btn_slot(int state)
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
cLogo::value_changed_slot(double d)
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
cLogo::endstyle_change_slot(int es)
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
cLogo::pwidth_change_slot(int pw)
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
cLogo::create_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_LogoToFile, "");
    else
        CDvdb()->clearVariable(VA_LogoToFile);
}


void
cLogo::dump_btn_slot(bool state)
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
cLogo::sel_btn_slot(bool state)
{
    if (state)
        ED()->PopUpPolytextFont(lgo_sel, MODE_ON);
    else
        ED()->PopUpPolytextFont(0, MODE_OFF);
}


void
cLogo::dismiss_btn_slot()
{
    ED()->PopUpLogo(0, MODE_OFF);
}

