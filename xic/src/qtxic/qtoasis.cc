
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

#include "qtoasis.h"
#include "fio.h"
#include "fio_oasis.h"
#include "fio_oas_sort.h"
#include "fio_oas_reps.h"
#include "cvrt.h"
#include "dsp_inlines.h"
#include "qtcvofmt.h"
#include "qtinterf/qtfont.h"

#include <QApplication>
#include <QLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QToolButton>
#include <QPushButton>
#include <QSpinBox>
#include <QGroupBox>
#include <QLabel>


//-----------------------------------------------------------------------------
// QToasisDlg:  Dialog for advanced/obscure OASIS writing features.
// Called from the Advanced button on the OASIS tab of the output
// format widget (QTconvOutFmt).
//
// Help system keywords used:
//  xic:oasadv

void
cConvert::PopUpOasAdv(GRobject caller, ShowMode mode, int x, int y)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QToasisDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QToasisDlg::self())
            QToasisDlg::self()->update();
        return;
    }
    if (QToasisDlg::self()) {
        if (caller && caller != QToasisDlg::self()->call_btn())
            QTdev::Deselect(caller);
        return;
    }

    new QToasisDlg(caller);

    QToasisDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_XYA, x, y),
        QToasisDlg::self(), QTmainwin::self()->Viewport());
    QToasisDlg::self()->show();
}
// End of cConvert functions.


const char *QToasisDlg::pmaskvals[] =
{
    "mask none",
    "mask XIC_PROPERTIES 7012",
    "mask XIC_LABEL",
    "mask 7012 and XIC_LABEL",
    "mask all properties",
    0
};

QToasisDlg *QToasisDlg::instPtr;

QToasisDlg::QToasisDlg(GRobject c)
{
    instPtr = this;
    oas_caller = c;
    oas_notrap = 0;
    oas_wtob = 0;
    oas_rwtop = 0;
    oas_nogcd = 0;
    oas_oldsort = 0;
    oas_pmask = 0;
    oas_objc = 0;
    oas_objb = 0;
    oas_objp = 0;
    oas_objw = 0;
    oas_objl = 0;
    oas_def = 0;
    oas_noruns = 0;
    oas_noarrs = 0;
    oas_nosim = 0;
    oas_sb_entm = 0;
    oas_sb_enta = 0;
    oas_sb_entx = 0;
    oas_sb_entt = 0;

    oas_lastm = REP_RUN_MIN;
    oas_lasta = REP_ARRAY_MIN;
    oas_lastt = REP_MAX_REPS;

    setWindowTitle(tr("Advanced OASIS Export Parameters"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);
    vbox->setSizeConstraint(QLayout::SetFixedSize);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    oas_notrap = new QCheckBox(tr("Don't write trapezoid records"));
    hbox->addWidget(oas_notrap);
    connect(oas_notrap, SIGNAL(stateChanged(int)),
        this, SLOT(nozoid_btn_slot(int)));

    hbox->addSpacing(60);
    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    oas_wtob = new QCheckBox(tr(
        "Convert Wire to Box records when possible"));
    vbox->addWidget(oas_wtob);
    connect(oas_wtob, SIGNAL(stateChanged(int)),
        this, SLOT(wtob_btn_slot(int)));

    oas_rwtop = new QCheckBox(tr(
        "Convert rounded-end Wire records to Poly records"));
    vbox->addWidget(oas_rwtop);
    connect(oas_rwtop, SIGNAL(stateChanged(int)),
        this, SLOT(rwtop_btn_slot(int)));

    oas_nogcd = new QCheckBox(tr("Skip GCD check"));
    vbox->addWidget(oas_nogcd);
    connect(oas_nogcd, SIGNAL(stateChanged(int)),
        this, SLOT(nogcd_btn_slot(int)));

    oas_oldsort = new QCheckBox(tr("Use alternate modal sort algorithm"));
    vbox->addWidget(oas_oldsort);
    connect(oas_oldsort, SIGNAL(stateChanged(int)),
        this, SLOT(oldsort_btn_slot(int)));

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QLabel *label = new QLabel(tr("Property masking"));
    hbox->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    oas_pmask = new QComboBox();
    hbox->addWidget(oas_pmask);
    for (int i = 0; pmaskvals[i]; i++)
        oas_pmask->addItem(pmaskvals[i], i);
    connect(oas_pmask, SIGNAL(currentIndexChanged(int)),
        this, SLOT(pmask_menu_slot(int)));

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    hbox->addSpacing(20);
    label = new QLabel(tr("Repetition Finder Configuration"));
    hbox->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    hbox->addSpacing(40);
    oas_def = new QToolButton();
    oas_def->setText(tr("Restore Defaults"));
    hbox->addWidget(oas_def);
    connect(oas_def, SIGNAL(clicked()), this, SLOT(def_btn_slot()));

    // Repetition Finder Configuration
    // four columns
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QGridLayout *grid = new QGridLayout();
    vbox->addLayout(grid);

    QGroupBox *gb = new QGroupBox(tr("Objects"));
    grid->addWidget(gb, 0, 0, 4, 1);
    QVBoxLayout *vb = new QVBoxLayout(gb);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    oas_objc = new QCheckBox(tr("Cells"));
    vb->addWidget(oas_objc);
    connect(oas_objc, SIGNAL(stateChanged(int)),
        this, SLOT(cells_btn_slot(int)));

    oas_objb = new QCheckBox(tr("Boxes"));;
    vb->addWidget(oas_objb);
    connect(oas_objb, SIGNAL(stateChanged(int)),
        this, SLOT(boxes_btn_slot(int)));

    oas_objp = new QCheckBox(tr("Polys"));;
    vb->addWidget(oas_objp);
    connect(oas_objp, SIGNAL(stateChanged(int)),
        this, SLOT(polys_btn_slot(int)));

    oas_objw = new QCheckBox(tr("Wires"));
    vb->addWidget(oas_objw);
    connect(oas_objw, SIGNAL(stateChanged(int)),
        this, SLOT(wires_btn_slot(int)));

    oas_objl = new QCheckBox(tr("Labels"));
    vb->addWidget(oas_objl);
    connect(oas_objl, SIGNAL(stateChanged(int)),
        this, SLOT(labels_btn_slot(int)));

    // Entry areas
    //
    label = new QLabel(tr("Run minimum"));
    grid->addWidget(label, 0, 1);

    oas_noruns = new QToolButton();
    oas_noruns->setText(tr("None"));
    grid->addWidget(oas_noruns, 0, 2);
    oas_noruns->setCheckable(true);
    connect(oas_noruns, SIGNAL(toggled(bool)),
        this, SLOT(noruns_btn_slot(bool)));

    oas_sb_entm = new QSpinBox();
    grid->addWidget(oas_sb_entm, 0, 3);
    oas_sb_entm->setMinimum(REP_RUN_MIN);
    oas_sb_entm->setMaximum(REP_RUN_MAX);
    oas_sb_entm->setValue(REP_RUN_MIN);
    connect(oas_sb_entm, SIGNAL(valueChanged(int)),
        this, SLOT(entm_changed_slot(int)));

    label = new QLabel(tr("Array minimum"));
    grid->addWidget(label, 1, 1);

    oas_noarrs = new QToolButton();
    oas_noarrs->setText(tr("None"));
    grid->addWidget(oas_noarrs, 1, 2);
    oas_noarrs->setCheckable(true);
    connect(oas_noarrs, SIGNAL(toggled(bool)),
        this, SLOT(noarrs_btn_slot(bool)));

    oas_sb_enta = new QSpinBox();
    grid->addWidget(oas_sb_enta, 1, 3);
    oas_sb_enta->setMinimum(REP_ARRAY_MIN);
    oas_sb_enta->setMaximum(REP_ARRAY_MAX);
    oas_sb_enta->setValue(REP_ARRAY_MIN);
    connect(oas_sb_enta, SIGNAL(valueChanged(int)),
        this, SLOT(enta_changed_slot(int)));

    label = new QLabel(tr("Max different objects"));
    grid->addWidget(label, 2, 1);

    oas_sb_entx = new QSpinBox();
    grid->addWidget(oas_sb_entx, 2, 3);
    oas_sb_entx->setMinimum(REP_MAX_ITEMS_MIN);
    oas_sb_entx->setMaximum(REP_MAX_ITEMS_MAX);
    oas_sb_entx->setValue(REP_MAX_ITEMS);
    connect(oas_sb_entx, SIGNAL(valueChanged(int)),
        this, SLOT(entx_changed_slot(int)));

    label = new QLabel(tr("Max identical objects"));
    grid->addWidget(label, 3, 1);

    oas_nosim = new QToolButton();
    oas_nosim->setText(tr("None"));
    grid->addWidget(oas_nosim, 3, 2);
    oas_nosim->setCheckable(true);
    connect(oas_nosim, SIGNAL(toggled(bool)),
        this, SLOT(nosim_btn_slot(bool)));

    oas_sb_entt = new QSpinBox();
    grid->addWidget(oas_sb_entt, 3, 3);
    oas_sb_entt->setMinimum(REP_MAX_REPS_MIN);
    oas_sb_entt->setMaximum(REP_MAX_REPS_MAX);
    oas_sb_entt->setValue(REP_MAX_REPS);
    connect(oas_sb_entt, SIGNAL(valueChanged(int)),
        this, SLOT(entt_changed_slot(int)));

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Dismiss");
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update();
}


QToasisDlg::~QToasisDlg()
{
    instPtr = 0;
    if (oas_caller)
        QTdev::Deselect(oas_caller);
}


#ifdef Q_OS_MACOS
#define DLGTYPE QToasisDlg
#include "qtinterf/qtmacos_event.h"
#endif


namespace {
    // Struct to hold repetition finder state while parsing property
    // string.
    //
    struct rp_state
    {
        rp_state()
        {
            cf = true;
            bf = true;
            pf = true;
            wf = true;
            lf = true;
            mval = REP_RUN_MIN;
            aval = REP_ARRAY_MIN;
            xval = REP_MAX_ITEMS;
            tval = REP_MAX_REPS;
            mnone = false;
            anone = false;
            tnone = false;
            oflgs = 0;
        }

        bool cf, bf, pf, wf, lf;
        int mval, aval, xval, tval;
        bool mnone, anone, tnone;
        int oflgs;
    };
}


void
QToasisDlg::update()
{
    QTdev::SetStatus(oas_notrap,
        CDvdb()->getVariable(VA_OasWriteNoTrapezoids));
    QTdev::SetStatus(oas_wtob,
        CDvdb()->getVariable(VA_OasWriteWireToBox));
    QTdev::SetStatus(oas_rwtop,
        CDvdb()->getVariable(VA_OasWriteRndWireToPoly));
    QTdev::SetStatus(oas_nogcd,
        CDvdb()->getVariable(VA_OasWriteNoGCDcheck));
    QTdev::SetStatus(oas_oldsort,
        CDvdb()->getVariable(VA_OasWriteUseFastSort));

    const char *str = CDvdb()->getVariable(VA_OasWritePrptyMask);
    int nn;
    if (!str)
        nn = 0;
    else if (!*str)
        nn = 3;
    else if (isdigit(*str))
        nn = (atoi(str) & 0x3);
    else
        nn = 4;
    oas_pmask->setCurrentIndex(nn);

    const char *s = CDvdb()->getVariable(VA_OasWriteRep);
    if (s) {
        // The property is set, update the backup string if necessary.
        if (!QTconvOutFmt::rep_string() ||
                strcmp(s, QTconvOutFmt::rep_string()))
            QTconvOutFmt::set_rep_string(lstring::copy(s));
    }
    else
        s = QTconvOutFmt::rep_string();
    if (!s || !*s) {
        restore_defaults();
        return;
    }

    rp_state rp;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (*tok == 'r')
            rp.mnone = true;
        else if (*tok == 'm') {
            char *t = strchr(tok, '=');
            if (t) {
                int n = atoi(t+1);
                if (n >= REP_RUN_MIN && n <= REP_RUN_MAX)
                    rp.mval = n;
            }
        }
        else if (*tok == 'a') {
            char *t = strchr(tok, '=');
            if (t) {
                int n = atoi(t+1);
                if (n == 0) {
                    rp.anone = true;
                    rp.aval = 0;
                }
                else if (n >= REP_ARRAY_MIN && n <= REP_ARRAY_MAX)
                    rp.aval = n;
            }
        }
        else if (*tok == 't') {
            char *t = strchr(tok, '=');
            if (t) {
                int n = atoi(t+1);
                if (n == 0) {
                    rp.tnone = true;
                    rp.tval = 0;
                }
                else if (n >= REP_MAX_REPS_MIN && n <= REP_MAX_REPS_MAX)
                    rp.tval = n;
            }
        }
        else if (*tok == 'd') {
            ;
        }
        else if (*tok == 'x') {
            char *t = strchr(tok, '=');
            if (t) {
                int n = atoi(t+1);
                if (n >= REP_MAX_ITEMS_MIN && n <= REP_MAX_ITEMS_MAX)
                    rp.xval = n;
            }
        }
        else {
            if (strchr(tok, 'b'))
                rp.oflgs |= OAS_CR_BOX;
            if (strchr(tok, 'p'))
                rp.oflgs |= OAS_CR_POLY;
            if (strchr(tok, 'w'))
                rp.oflgs |= OAS_CR_WIRE;
            if (strchr(tok, 'l'))
                rp.oflgs |= OAS_CR_LAB;
            if (strchr(tok, 'c'))
                rp.oflgs |= OAS_CR_CELL;
        }
        delete [] tok;
    }
    if (rp.oflgs) {
        rp.cf = (rp.oflgs & OAS_CR_CELL);
        rp.bf = (rp.oflgs & OAS_CR_BOX);
        rp.pf = (rp.oflgs & OAS_CR_POLY);
        rp.wf = (rp.oflgs & OAS_CR_WIRE);
        rp.lf = (rp.oflgs & OAS_CR_LAB);
    }

    QTdev::SetStatus(oas_objc, rp.cf);
    QTdev::SetStatus(oas_objb, rp.bf);
    QTdev::SetStatus(oas_objp, rp.pf);
    QTdev::SetStatus(oas_objw, rp.wf);
    QTdev::SetStatus(oas_objl, rp.lf);
    if (rp.mnone) {
        QTdev::SetStatus(oas_noruns, true);
        oas_sb_entm->setEnabled(false);
        oas_sb_enta->setEnabled(false);
        QTdev::SetStatus(oas_noarrs, true);
        oas_noarrs->setEnabled(false);
    }
    else {
        QTdev::SetStatus(oas_noruns, false);
        oas_sb_entm->setEnabled(true);
        oas_sb_entm->setValue(rp.mval);
        oas_noarrs->setEnabled(true);
        if (rp.anone) {
            QTdev::SetStatus(oas_noarrs, true);
            oas_sb_enta->setEnabled(false);
        }
        else {
            QTdev::SetStatus(oas_noarrs, false);
            oas_sb_enta->setEnabled(true);
            oas_sb_enta->setValue(rp.aval);
        }
    }
    oas_sb_entx->setValue(rp.xval);
    if (rp.tnone) {
        QTdev::SetStatus(oas_nosim, true);
        oas_sb_entt->setEnabled(false);
    }
    else {
        QTdev::SetStatus(oas_nosim, false);
        oas_sb_entt->setEnabled(true);
        oas_sb_entt->setValue(rp.tval);
    }
}


void
QToasisDlg::restore_defaults()
{
    QTdev::SetStatus(oas_objc, true);
    QTdev::SetStatus(oas_objb, true);
    QTdev::SetStatus(oas_objp, true);
    QTdev::SetStatus(oas_objw, true);
    QTdev::SetStatus(oas_objl, true);

    QTdev::SetStatus(oas_noruns, false);
    QTdev::SetStatus(oas_noarrs, false);
    QTdev::SetStatus(oas_nosim, false);
    oas_noruns->setEnabled(true);
    oas_noarrs->setEnabled(true);
    oas_nosim->setEnabled(true);

    oas_sb_entm->setValue(REP_RUN_MIN);
    oas_sb_entm->setEnabled(true);
    oas_sb_enta->setValue(REP_ARRAY_MIN);
    oas_sb_enta->setEnabled(true);
    oas_sb_entx->setValue(REP_MAX_ITEMS);
    oas_sb_entt->setValue(REP_MAX_REPS);
    oas_sb_entt->setEnabled(true);

    oas_lastm = REP_RUN_MIN;
    oas_lasta = REP_ARRAY_MIN;
    oas_lastt = REP_MAX_REPS;
}


void
QToasisDlg::set_repvar()
{
    sLstr lstr;
    bool cv = QTdev::GetStatus(oas_objc);
    bool bv = QTdev::GetStatus(oas_objb);
    bool pv = QTdev::GetStatus(oas_objp);
    bool wv = QTdev::GetStatus(oas_objw);
    bool lv = QTdev::GetStatus(oas_objl);
    if (cv && bv && pv && wv && lv)
        ;
    else {
        if (cv)
            lstr.add_c('c');
        if (bv)
            lstr.add_c('b');
        if (pv)
            lstr.add_c('p');
        if (wv)
            lstr.add_c('w');
        if (lv)
            lstr.add_c('l');
    }

    char buf[64];
    if (QTdev::GetStatus(oas_noruns)) {
        if (lstr.length())
            lstr.add_c(' ');
        lstr.add("r");
    }
    else {
        const char *str = oas_sb_entm->text().toLatin1().constData();
        int mval;
        if (sscanf(str, "%d", &mval) == 1 &&
                mval > REP_RUN_MIN && mval <= REP_RUN_MAX) {
            snprintf(buf, sizeof(buf), "m=%d", mval);
            if (lstr.length())
                lstr.add_c(' ');
            lstr.add(buf);
        }
        if (QTdev::GetStatus(oas_noarrs)) {
            if (lstr.length())
                lstr.add_c(' ');
            lstr.add("a=0");
        }
        else {
            str = oas_sb_enta->text().toLatin1().constData();
            int aval;
            if (sscanf(str, "%d", &aval) == 1 &&
                    aval > REP_ARRAY_MIN && aval <= REP_ARRAY_MAX) {
                snprintf(buf, sizeof(buf), "a=%d", aval);
                if (lstr.length())
                    lstr.add_c(' ');
                lstr.add(buf);
            }
        }
    }
    if (QTdev::GetStatus(oas_nosim)) {
        if (lstr.length())
            lstr.add_c(' ');
        lstr.add("t=0");
    }
    else {
        const char *str = oas_sb_entt->text().toLatin1().constData();
        int tval;
        if (sscanf(str, "%d", &tval) == 1 &&
                tval >= REP_MAX_REPS_MIN && tval <= REP_MAX_REPS_MAX &&
                tval != REP_MAX_REPS) {
            snprintf(buf, sizeof(buf), "t=%d", tval);
            if (lstr.length())
                lstr.add_c(' ');
            lstr.add(buf);
        }
    }

    const char *str = oas_sb_entx->text().toLatin1().constData();
    int xval;
    if (sscanf(str, "%d", &xval) == 1 &&
            xval >= REP_MAX_ITEMS_MIN && xval <= REP_MAX_ITEMS_MAX &&
            xval != REP_MAX_ITEMS) {
        snprintf(buf, sizeof(buf), "x=%d", xval);
        if (lstr.length())
            lstr.add_c(' ');
        lstr.add(buf);
    }
    QTconvOutFmt::set_rep_string(lstr.string_trim());
    if (!QTconvOutFmt::rep_string())
        QTconvOutFmt::set_rep_string(lstring::copy(""));

    // If the property is already set, update it.
    str = CDvdb()->getVariable(VA_OasWriteRep);
    if (str && strcmp(str, QTconvOutFmt::rep_string()))
        CDvdb()->setVariable(VA_OasWriteRep, QTconvOutFmt::rep_string());
}


void
QToasisDlg::nozoid_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_OasWriteNoTrapezoids, 0);
    else
        CDvdb()->clearVariable(VA_OasWriteNoTrapezoids);
}


void
QToasisDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:oasadv"))
}


void
QToasisDlg::wtob_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_OasWriteWireToBox, 0);
    else
        CDvdb()->clearVariable(VA_OasWriteWireToBox);
}


void
QToasisDlg::rwtop_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_OasWriteRndWireToPoly, 0);
    else
        CDvdb()->clearVariable(VA_OasWriteRndWireToPoly);
}


void
QToasisDlg::nogcd_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_OasWriteNoGCDcheck, 0);
    else
        CDvdb()->clearVariable(VA_OasWriteNoGCDcheck);
}


void
QToasisDlg::oldsort_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_OasWriteUseFastSort, 0);
    else
        CDvdb()->clearVariable(VA_OasWriteUseFastSort);
}


void
QToasisDlg::pmask_menu_slot(int i)
{
    char bf[2];
    bf[1] = 0;
    if (i == 0)
        CDvdb()->clearVariable(VA_OasWritePrptyMask);
    else if (i == 1) {
        bf[0] = '1';
        CDvdb()->setVariable(VA_OasWritePrptyMask, bf);
    }
    else if (i == 2) {
        bf[0] = '2';
        CDvdb()->setVariable(VA_OasWritePrptyMask, bf);
    }
    else if (i == 3) {
        bf[0] = '3';
        CDvdb()->setVariable(VA_OasWritePrptyMask, bf);
    }
    else if (i == 4)
        CDvdb()->setVariable(VA_OasWritePrptyMask, "all");
}


void
QToasisDlg::def_btn_slot()
{
    restore_defaults();
    set_repvar();
}


void
QToasisDlg::cells_btn_slot(int)
{
    set_repvar();
}


void
QToasisDlg::boxes_btn_slot(int)
{
    set_repvar();
}


void
QToasisDlg::polys_btn_slot(int)
{
    set_repvar();
}


void
QToasisDlg::wires_btn_slot(int)
{
    set_repvar();
}


void
QToasisDlg::labels_btn_slot(int)
{
    set_repvar();
}


void
QToasisDlg::noruns_btn_slot(bool state)
{
    if (state) {
        const char *str = oas_sb_entm->text().toLatin1().constData();
        int mval;
        if (sscanf(str, "%d", &mval) == 1 &&
                mval >= REP_RUN_MIN && mval <= REP_RUN_MAX)
            oas_lastm = mval;
        str = oas_sb_enta->text().toLatin1().constData();
        int aval;
        if (sscanf(str, "%d", &aval) == 1 &&
                aval > REP_ARRAY_MIN && aval <= REP_ARRAY_MAX)
            oas_lasta = aval;

        oas_sb_entm->setEnabled(false);
        oas_sb_enta->setEnabled(false);
        QTdev::SetStatus(oas_noarrs, true);
        oas_noarrs->setEnabled(false);
    }
    else {
        oas_sb_entm->setEnabled(true);
        oas_sb_entm->setValue(oas_lastm);
        oas_noarrs->setEnabled(true);
    }
    set_repvar();
}


void
QToasisDlg::entm_changed_slot(int)
{
    set_repvar();
}


void
QToasisDlg::noarrs_btn_slot(bool state)
{
    if (state) {
        const char *str = oas_sb_enta->text().toLatin1().constData();
        int aval;
        if (sscanf(str, "%d", &aval) == 1 &&
                aval > REP_ARRAY_MIN && aval <= REP_ARRAY_MAX)
            oas_lasta = aval;

        oas_sb_enta->setEnabled(false);
    }
    else {
        oas_sb_enta->setValue(oas_lasta);
        oas_sb_enta->setEnabled(true);
    }
    set_repvar();
}


void
QToasisDlg::enta_changed_slot(int)
{
    set_repvar();
}


void
QToasisDlg::entx_changed_slot(int)
{
    set_repvar();
}


void
QToasisDlg::nosim_btn_slot(bool state)
{
    if (state) {
        const char *str = oas_sb_entt->text().toLatin1().constData();
        int tval;
        if (sscanf(str, "%d", &tval) == 1 &&
                tval >= REP_MAX_REPS_MIN && tval <= REP_MAX_REPS_MAX)
            oas_lastt = tval;
        oas_sb_entt->setEnabled(false);
    }
    else {
        oas_sb_entt->setValue(oas_lastt);
        oas_sb_entt->setEnabled(true);
    }
    set_repvar();
}


void
QToasisDlg::entt_changed_slot(int)
{
    set_repvar();
}


void
QToasisDlg::dismiss_btn_slot()
{
    Cvt()->PopUpOasAdv(0, MODE_OFF, 0, 0);
}

