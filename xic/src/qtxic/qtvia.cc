
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

#include "qtvia.h"
#include "edit.h"
#include "tech.h"
#include "tech_via.h"
#include "fio.h"
#include "dsp_inlines.h"
#include "cd_lgen.h"
#include "cd_propnum.h"
#include "errorlog.h"
#include "menu.h"
#include "undolist.h"
#include "promptline.h"
#include "qtinterf/qtdblsb.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>


//-----------------------------------------------------------------------------
// QTstdViaDlg:  Dialog to create a standard via.
// Called from the main menu: Edit/Create Via.
//
// Help system keywords used:
//  xic:stvia

// If cdvia is null, pop up the Standard Via panel, and upon Apply,
// standard vias will be instantiated where the user clicks.  If cdvia
// is not null, it must point to a standard via instance.  If so, the
// Standard Via panel will appear.  On Apply, this instance will be
// updated to use modified parameters set from the panel.
//
void
cEdit::PopUpStdVia(GRobject caller, ShowMode mode, CDc *cdvia)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTstdViaDlg::self();
        return;
    }
    if (cdvia && !cdvia->prpty(XICP_STDVIA)) {
        Log()->PopUpErr("Internal error:  PopUpStdVia, missing property.");
        return;
    }
    if (cdvia && caller) {
        // If cdvia, caller is null, since were aren't called from
        // a menu.  We assume this below, so enforce it here.
        Log()->PopUpErr("Internal error:  caller not null.");
        return;
    }

    if (mode == MODE_UPD) {
        if (QTstdViaDlg::self())
            QTstdViaDlg::self()->update(caller, cdvia);
        return;
    }
    if (QTstdViaDlg::self()) {
        QTstdViaDlg::self()->update(caller, cdvia);
        return;
    }

    new QTstdViaDlg(caller, cdvia);

    QTstdViaDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTstdViaDlg::self(),
        QTmainwin::self()->Viewport());
    QTstdViaDlg::self()->show();
}
// End of cEdit functions.


#define WID_DEF     0.1
#define WID_MIN     0.01
#define WID_MAX     10.0
#define HEI_DEF     0.1
#define HEI_MIN     0.01
#define HEI_MAX     10.0
#define ROWS_DEF    1
#define ROWS_MIN    1
#define ROWS_MAX    100
#define COLS_DEF    1
#define COLS_MIN    1
#define COLS_MAX    100
#define SPA_X_DEF   0.0
#define SPA_X_MIN   0.0
#define SPA_X_MAX   10.0
#define SPA_Y_DEF   0.0
#define SPA_Y_MIN   0.0
#define SPA_Y_MAX   10.0
#define ENC1_X_DEF  0.0
#define ENC1_X_MIN  0.0
#define ENC1_X_MAX  10.0
#define ENC1_Y_DEF  0.0
#define ENC1_Y_MIN  0.0
#define ENC1_Y_MAX  10.0
#define OFF1_X_DEF  0.0
#define OFF1_X_MIN  0.0
#define OFF1_X_MAX  10.0
#define OFF1_Y_DEF  0.0
#define OFF1_Y_MIN  0.0
#define OFF1_Y_MAX  10.0
#define ENC2_X_DEF  0.0
#define ENC2_X_MIN  0.0
#define ENC2_X_MAX  10.0
#define ENC2_Y_DEF  0.0
#define ENC2_Y_MIN  0.0
#define ENC2_Y_MAX  10.0
#define OFF2_X_DEF  0.0
#define OFF2_X_MIN  0.0
#define OFF2_X_MAX  10.0
#define OFF2_Y_DEF  0.0
#define OFF2_Y_MIN  0.0
#define OFF2_Y_MAX  10.0
#define ORG_X_DEF   0.0
#define ORG_X_MIN   0.0
#define ORG_X_MAX   10.0
#define ORG_Y_DEF   0.0
#define ORG_Y_MIN   0.0
#define ORG_Y_MAX   10.0
#define IMP1_X_DEF  0.0
#define IMP1_X_MIN  0.0
#define IMP1_X_MAX  10.0
#define IMP1_Y_DEF  0.0
#define IMP1_Y_MIN  0.0
#define IMP1_Y_MAX  10.0
#define IMP2_X_DEF  0.0
#define IMP2_X_MIN  0.0
#define IMP2_X_MAX  10.0
#define IMP2_Y_DEF  0.0
#define IMP2_Y_MIN  0.0
#define IMP2_Y_MAX  10.0


QTstdViaDlg *QTstdViaDlg::instPtr;

QTstdViaDlg::QTstdViaDlg(GRobject caller, CDc *cdesc)
{
    instPtr = this;
    stv_caller = 0;  // set in update()
    stv_name = 0;
    stv_layer1 = 0;
    stv_layer2 = 0;
    stv_layerv = 0;
    stv_imp1 = 0;
    stv_imp2 = 0;
    stv_apply = 0;
    stv_cdesc = 0;

    stv_sb_wid = 0;
    stv_sb_hei = 0;
    stv_sb_rows = 0;
    stv_sb_cols = 0;
    stv_sb_spa_x = 0;
    stv_sb_spa_y = 0;
    stv_sb_enc1_x = 0;
    stv_sb_enc1_y = 0;
    stv_sb_off1_x = 0;
    stv_sb_off1_y = 0;
    stv_sb_enc2_x = 0;
    stv_sb_enc2_y = 0;
    stv_sb_off2_x = 0;
    stv_sb_off2_y = 0;
    stv_sb_org_x = 0;
    stv_sb_org_y = 0;
    stv_sb_imp1_x = 0;
    stv_sb_imp1_y = 0;
    stv_sb_imp2_x = 0;
    stv_sb_imp2_y = 0;

    setWindowTitle(tr("Standard Via Parameters"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);
    vbox->setSizeConstraint(QLayout::SetFixedSize);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // label in frame plus help btn
    //
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    QLabel *label = new QLabel(tr("Set parameters for new standard via"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    QGridLayout *grid = new QGridLayout();
    vbox->addLayout(grid);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    label = new QLabel(tr("via name, cut layer"));
    grid->addWidget(label, 0, 0);
    stv_name = new QComboBox();
    grid->addWidget(stv_name, 0, 1);
    connect(stv_name, SIGNAL(currentTextChanged(const QString&)),
        this, SLOT(name_menu_slot(const QString&)));
    stv_layerv = new QComboBox();
    grid->addWidget(stv_layerv, 0, 2);
    connect(stv_layerv, SIGNAL(currentTextChanged(const QString&)),
        this, SLOT(layerv_menu_slot(const QString&)));

    label = new QLabel(tr("layer 1, layer 2"));
    grid->addWidget(label, 1, 0);
    stv_layer1 = new QLineEdit();
    stv_layer1->setReadOnly(true);
    grid->addWidget(stv_layer1, 1, 1);
    stv_layer2 = new QLineEdit();
    stv_layer2->setReadOnly(true);
    grid->addWidget(stv_layer2, 1, 2);

    int ndgt = CD()->numDigits();
    label = new QLabel(tr("cut width, height"));
    grid->addWidget(label, 2, 0);
    stv_sb_wid = new QTdoubleSpinBox();
    stv_sb_wid->setRange(WID_MIN, WID_MAX);
    stv_sb_wid->setDecimals(ndgt);
    stv_sb_wid->setValue(WID_DEF);
    grid->addWidget(stv_sb_wid, 2, 1);
    stv_sb_hei = new QTdoubleSpinBox();
    stv_sb_hei->setRange(HEI_MIN, HEI_MAX);
    stv_sb_hei->setDecimals(ndgt);
    stv_sb_hei->setValue(HEI_DEF);
    grid->addWidget(stv_sb_hei, 2, 2);

    label = new QLabel(tr("cut rows, columns"));
    grid->addWidget(label, 3, 0);
    stv_sb_rows = new QSpinBox();
    stv_sb_rows->setRange(ROWS_MIN, ROWS_MAX);
    stv_sb_rows->setValue(ROWS_DEF);
    grid->addWidget(stv_sb_rows, 3, 1);
    stv_sb_cols = new QSpinBox();
    stv_sb_cols->setRange(COLS_MIN, COLS_MAX);
    stv_sb_cols->setValue(COLS_DEF);
    grid->addWidget(stv_sb_cols, 3, 2);

    label = new QLabel(tr("cut spacing X,Y"));
    grid->addWidget(label, 4, 0);
    stv_sb_spa_x = new QTdoubleSpinBox();
    stv_sb_spa_x->setRange(SPA_X_MIN, SPA_X_MAX);
    stv_sb_spa_x->setDecimals(ndgt);
    stv_sb_spa_x->setValue(SPA_X_DEF);
    grid->addWidget(stv_sb_spa_x, 4, 1);
    stv_sb_spa_y = new QTdoubleSpinBox();
    stv_sb_spa_y->setRange(SPA_Y_MIN, SPA_Y_MAX);
    stv_sb_spa_y->setDecimals(ndgt);
    stv_sb_spa_y->setValue(SPA_Y_DEF);
    grid->addWidget(stv_sb_spa_y, 4, 2);

    label = new QLabel(tr("enclosure 1 X,Y"));
    grid->addWidget(label, 5, 0);
    stv_sb_enc1_x = new QTdoubleSpinBox();
    stv_sb_enc1_x->setRange(ENC1_X_MIN, ENC1_X_MAX);
    stv_sb_enc1_x->setDecimals(ndgt);
    stv_sb_enc1_x->setValue(ENC1_X_DEF);
    grid->addWidget(stv_sb_enc1_x, 5, 1);
    stv_sb_enc1_y = new QTdoubleSpinBox();
    stv_sb_enc1_y->setRange(ENC1_Y_MIN, ENC1_Y_MAX);
    stv_sb_enc1_y->setDecimals(ndgt);
    stv_sb_enc1_y->setValue(ENC1_Y_DEF);
    grid->addWidget(stv_sb_enc1_y, 5, 2);

    label = new QLabel(tr("offset 1 X,Y"));
    grid->addWidget(label, 6, 0);
    stv_sb_off1_x = new QTdoubleSpinBox();
    stv_sb_off1_x->setRange(OFF1_X_MIN, OFF1_X_MAX);
    stv_sb_off1_x->setDecimals(ndgt);
    stv_sb_off1_x->setValue(OFF1_X_DEF);
    grid->addWidget(stv_sb_off1_x, 6, 1);
    stv_sb_off1_y = new QTdoubleSpinBox();
    stv_sb_off1_y->setRange(OFF1_Y_MIN, OFF1_Y_MAX);
    stv_sb_off1_y->setDecimals(ndgt);
    stv_sb_off1_y->setValue(OFF1_Y_DEF);
    grid->addWidget(stv_sb_off1_y, 6, 2);

    label = new QLabel(tr("enclosure 2 X,Y"));
    grid->addWidget(label, 7, 0);
    stv_sb_enc2_x = new QTdoubleSpinBox();
    stv_sb_enc2_x->setRange(ENC2_X_MIN, ENC2_X_MAX);
    stv_sb_enc2_x->setDecimals(ndgt);
    stv_sb_enc2_x->setValue(ENC2_X_DEF);
    grid->addWidget(stv_sb_enc2_x, 7, 1);
    stv_sb_enc2_y = new QTdoubleSpinBox();
    stv_sb_enc2_y->setRange(ENC2_Y_MIN, ENC2_Y_MAX);
    stv_sb_enc2_y->setDecimals(ndgt);
    stv_sb_enc2_y->setValue(ENC2_Y_DEF);
    grid->addWidget(stv_sb_enc2_y, 7, 2);

    label = new QLabel(tr("offset 2 X,Y"));
    grid->addWidget(label, 8, 0);
    stv_sb_off2_x = new QTdoubleSpinBox();
    stv_sb_off2_x->setRange(OFF2_X_MIN, OFF2_X_MAX);
    stv_sb_off2_x->setDecimals(ndgt);
    stv_sb_off2_x->setValue(OFF2_X_DEF);
    grid->addWidget(stv_sb_off2_x, 8, 1);
    stv_sb_off2_y = new QTdoubleSpinBox();
    stv_sb_off2_y->setRange(OFF2_Y_MIN, OFF2_Y_MAX);
    stv_sb_off2_y->setDecimals(ndgt);
    stv_sb_off2_y->setValue(OFF2_Y_DEF);
    grid->addWidget(stv_sb_off2_y, 8, 2);

    label = new QLabel(tr("origin offset X,Y"));
    grid->addWidget(label, 9, 0);
    stv_sb_org_x = new QTdoubleSpinBox();
    stv_sb_org_x->setRange(ORG_X_MIN, ORG_X_MAX);
    stv_sb_org_x->setDecimals(ndgt);
    stv_sb_org_x->setValue(ORG_X_DEF);
    grid->addWidget(stv_sb_org_x, 9, 1);
    stv_sb_org_y = new QTdoubleSpinBox();
    stv_sb_org_y->setRange(ORG_Y_MIN, ORG_Y_MAX);
    stv_sb_org_y->setDecimals(ORG_Y_DEF);
    stv_sb_org_y->setValue(ORG_Y_DEF);
    grid->addWidget(stv_sb_org_y, 9, 2);
    
    label = new QLabel(tr("implant 1, implant 2"));
    grid->addWidget(label, 10, 0);
    stv_imp1 = new QLineEdit();
    stv_imp1->setReadOnly(true);
    grid->addWidget(stv_imp1, 10, 1);
    stv_imp2 = new QLineEdit();
    stv_imp2->setReadOnly(true);
    grid->addWidget(stv_imp2, 10, 2);

    label = new QLabel(tr("implant 1 enc X,Y"));
    grid->addWidget(label, 11, 0);
    stv_sb_imp1_x = new QTdoubleSpinBox();
    stv_sb_imp1_x->setRange(IMP1_X_MIN, IMP1_X_MAX);
    stv_sb_imp1_x->setDecimals(ndgt);
    stv_sb_imp1_x->setValue(IMP1_X_DEF);
    grid->addWidget(stv_sb_imp1_x, 11, 1);
    stv_sb_imp1_y = new QTdoubleSpinBox();
    stv_sb_imp1_y->setRange(IMP1_Y_MIN, IMP1_Y_MAX);
    stv_sb_imp1_y->setDecimals(ndgt);
    stv_sb_imp1_y->setValue(IMP1_Y_DEF);
    grid->addWidget(stv_sb_imp1_y, 11, 2);

    label = new QLabel(tr("implant 2 enc X,Y"));
    grid->addWidget(label, 12, 0);
    stv_sb_imp2_x = new QTdoubleSpinBox();
    stv_sb_imp2_x->setRange(IMP2_X_MIN, IMP2_X_MAX);
    stv_sb_imp2_x->setDecimals(ndgt);
    stv_sb_imp2_x->setValue(IMP2_X_DEF);
    grid->addWidget(stv_sb_imp2_x, 12, 1);
    stv_sb_imp2_y = new QTdoubleSpinBox();
    stv_sb_imp2_y->setRange(IMP2_Y_MIN, IMP2_Y_MAX);
    stv_sb_imp2_y->setDecimals(ndgt);
    stv_sb_imp2_y->setValue(IMP2_Y_DEF);
    grid->addWidget(stv_sb_imp2_y, 12, 2);

    // Apply and Dismiss buttons
    //
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    stv_apply = new QPushButton(tr("Apply"));
    hbox->addWidget(stv_apply);
    connect(stv_apply, SIGNAL(clicked()), this, SLOT(apply_btn_slot()));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update(caller, cdesc);
}


QTstdViaDlg::~QTstdViaDlg()
{
    instPtr = 0;
    if (stv_caller)
        QTdev::Deselect(stv_caller);
}


void
QTstdViaDlg::update(GRobject caller, CDc *cd)
{
    if (cd) {
        // The user has selected a new std via instance, update
        // everything.

        if (stv_caller) {
            // Panel was started from the menu (via creation mode),
            // detach from menu.

            MainMenu()->SetStatus(caller, false);
            stv_caller = 0;
        }
        stv_apply->setText(tr("Update Via"));

        CDp *psv = cd->prpty(XICP_STDVIA);
        if (!psv) {
            Log()->PopUpErr("Missing StdVia instance property.");
            return;
        }
        const char *str = psv->string();
        char *vname = lstring::gettok(&str);
        if (!vname) {
            Log()->PopUpErr("StdVia instance property has no via name.");
            return;
        }

        // The first token is the standard via name.
        const sStdVia *sv = Tech()->FindStdVia(vname);
        delete [] vname;
        if (!sv) {
            Log()->PopUpErr("StdVia instance property has unknown via name.");
            return;
        }
        if (!sv->bottom() || !sv->top()) {
            Log()->PopUpErr("StdVia has unknown top or bottom layer.");
            return;
        }

        sStdVia rv(*sv);
        rv.parse(str);

        stv_name->clear();

        tgen_t<sStdVia> gen(Tech()->StdViaTab());
        const sStdVia *tsv;
        int cnt = 0;
        int ix = 0;
        while ((tsv = gen.next()) != 0) {
            if (tsv->via() == sv->via()) {
                stv_name->addItem(sv->tab_name());
                if (!strcmp(tsv->tab_name(), sv->tab_name()))
                    ix = cnt;
                cnt++;
            }
        }
        stv_name->setCurrentIndex(ix);

        stv_layer1->setText(sv->bottom()->name());
        stv_layer2->setText(sv->top()->name());
        stv_sb_wid->setValue(MICRONS(rv.via_wid()));
        stv_sb_hei->setValue(MICRONS(rv.via_hei()));
        stv_sb_rows->setValue(rv.via_rows());
        stv_sb_cols->setValue(rv.via_cols());
        stv_sb_spa_x->setValue(MICRONS(rv.via_spa_x()));
        stv_sb_spa_y->setValue(MICRONS(rv.via_spa_y()));
        stv_sb_enc1_x->setValue(MICRONS(rv.bot_enc_x()));
        stv_sb_enc1_y->setValue(MICRONS(rv.bot_enc_y()));
        stv_sb_off1_x->setValue(MICRONS(rv.bot_off_x()));
        stv_sb_off1_y->setValue(MICRONS(rv.bot_off_y()));
        stv_sb_enc2_x->setValue(MICRONS(rv.top_enc_x()));
        stv_sb_enc2_y->setValue(MICRONS(rv.top_enc_y()));
        stv_sb_off2_x->setValue(MICRONS(rv.top_off_x()));
        stv_sb_off2_y->setValue(MICRONS(rv.top_off_y()));
        stv_sb_org_x->setValue(MICRONS(rv.org_off_x()));
        stv_sb_org_y->setValue(MICRONS(rv.org_off_y()));

        CDl *ldim1 = sv->implant1();
        if (ldim1) {
            stv_imp1->setText(ldim1->name());
            stv_sb_imp1_x->setValue(MICRONS(rv.imp1_enc_x()));
            stv_sb_imp1_y->setValue(MICRONS(rv.imp1_enc_y()));
            stv_imp1->setEnabled(true);
            stv_sb_imp1_x->setEnabled(true);
            stv_sb_imp1_y->setEnabled(true);
        }
        else {
            stv_imp1->setText("");
            stv_sb_imp1_x->setValue(0);
            stv_sb_imp1_y->setValue(0);
            stv_imp1->setEnabled(false);
            stv_sb_imp1_x->setEnabled(false);
            stv_sb_imp1_y->setEnabled(false);
        }
        CDl *ldim2 = sv->implant2();
        if (ldim1 && ldim2) {
            stv_imp2->setText(ldim2->name());
            stv_sb_imp2_x->setValue(MICRONS(rv.imp2_enc_x()));
            stv_sb_imp2_y->setValue(MICRONS(rv.imp2_enc_y()));
            stv_imp2->setEnabled(true);
            stv_sb_imp2_x->setEnabled(true);
            stv_sb_imp2_y->setEnabled(true);
        }
        else {
            stv_imp2->setText("");
            stv_sb_imp2_x->setValue(0);
            stv_sb_imp2_y->setValue(0);
            stv_imp2->setEnabled(false);
            stv_sb_imp2_x->setEnabled(false);
            stv_sb_imp2_y->setEnabled(false);
        }
        stv_cdesc = cd;
    }
    else {
        // Via creation mode.

        stv_apply->setText(tr("Create Via"));
        stv_cdesc = 0;
        stv_caller = caller;

        QByteArray layerv_ba = stv_layerv->currentText().toLatin1();
        if (layerv_ba.isNull()) {
            // No cut layer, need to fill these in.  Only add those
            // that are used by an existing standard via.

            CDlgen lgen(Physical);
            CDl *ld;
            while ((ld = lgen.next()) != 0) {
                if (!ld->isVia())
                    continue;

                tgen_t<sStdVia> gen(Tech()->StdViaTab());
                const sStdVia *sv;
                while ((sv = gen.next()) != 0) {
                    if (sv->via() == ld) {
                        stv_layerv->addItem(ld->name());
                        break;
                    }
                }
            }
            // The signal handler will finish the job.
            stv_layerv->setCurrentIndex(0);
            return;
        }

        // Rebuild the name menu, in case it has changed (hence the
        // reason for the update).

        CDl *ldv = CDldb()->findLayer(layerv_ba.constData(), Physical);
        if (!ldv)
            return;

        stv_name->clear();
        tgen_t<sStdVia> gen(Tech()->StdViaTab());
        const sStdVia *sv;
        while ((sv = gen.next()) != 0) {
            if (sv->via() == ldv)
                stv_name->addItem(sv->tab_name());
        }
    }
}


void
QTstdViaDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:crvia"))
}


void
QTstdViaDlg::name_menu_slot(const QString &qs)
{
    if (!Tech()->StdViaTab())
        return;
    QByteArray nm_ba = qs.toLatin1();
    const char *nm = nm_ba.constData();
    if (!nm)
        return;
    const sStdVia *sv = Tech()->FindStdVia(nm);
    if (!sv)
        return;
    if (!sv->bottom() || !sv->top())
        return;
    stv_layer1->setText(sv->bottom()->name());
    stv_layer2->setText(sv->top()->name());
    stv_sb_wid->setValue(MICRONS(sv->via_wid()));
    stv_sb_hei->setValue(MICRONS(sv->via_hei()));
    stv_sb_rows->setValue(sv->via_rows());
    stv_sb_cols->setValue(sv->via_cols());
    stv_sb_spa_x->setValue(MICRONS(sv->via_spa_x()));
    stv_sb_spa_y->setValue(MICRONS(sv->via_spa_y()));
    stv_sb_enc1_x->setValue(MICRONS(sv->bot_enc_x()));
    stv_sb_enc1_y->setValue(MICRONS(sv->bot_enc_y()));
    stv_sb_off1_x->setValue(MICRONS(sv->bot_off_x()));
    stv_sb_off1_y->setValue(MICRONS(sv->bot_off_y()));
    stv_sb_enc2_x->setValue(MICRONS(sv->top_enc_x()));
    stv_sb_enc2_y->setValue(MICRONS(sv->top_enc_y()));
    stv_sb_off2_x->setValue(MICRONS(sv->top_off_x()));
    stv_sb_off2_y->setValue(MICRONS(sv->top_off_y()));
    stv_sb_org_x->setValue(MICRONS(sv->org_off_x()));
    stv_sb_org_y->setValue(MICRONS(sv->org_off_y()));
    CDl *ldim1 = sv->implant1();
    if (ldim1) {
        stv_imp1->setText(ldim1->name());
        stv_sb_imp1_x->setValue(MICRONS(sv->imp1_enc_x()));
        stv_sb_imp1_y->setValue(MICRONS(sv->imp1_enc_y()));
        stv_imp1->setEnabled(true);
        stv_sb_imp1_x->setEnabled(true);
        stv_sb_imp1_y->setEnabled(true);
    }
    else {
        stv_imp1->setText("");
        stv_sb_imp1_x->setValue(0);
        stv_sb_imp1_y->setValue(0);
        stv_imp1->setEnabled(false);
        stv_sb_imp1_x->setEnabled(false);
        stv_sb_imp1_y->setEnabled(false);
    }
    CDl *ldim2 = sv->implant2();
    if (ldim1 && ldim2) {
        stv_imp2->setText(ldim2->name());
        stv_sb_imp2_x->setValue(MICRONS(sv->imp2_enc_x()));
        stv_sb_imp2_y->setValue(MICRONS(sv->imp2_enc_y()));
        stv_imp2->setEnabled(true);
        stv_sb_imp2_x->setEnabled(true);
        stv_sb_imp2_y->setEnabled(true);
    }
    else {
        stv_imp2->setText("");
        stv_sb_imp2_x->setValue(0);
        stv_sb_imp2_y->setValue(0);
        stv_imp2->setEnabled(false);
        stv_sb_imp2_x->setEnabled(false);
        stv_sb_imp2_y->setEnabled(false);
    }
}


void
QTstdViaDlg::layerv_menu_slot(const QString &qs)
{
    QByteArray vlyr_ba = qs.toLatin1();
    const char *vlyr = vlyr_ba.constData();
    if (!vlyr)
        return;
    CDl *ldv = CDldb()->findLayer(vlyr, Physical);
    if (!ldv)
        return;

    stv_name->clear();

    tgen_t<sStdVia> gen(Tech()->StdViaTab());
    const sStdVia *sv;
    while ((sv = gen.next()) != 0) {
        if (sv->via() == ldv)
            stv_name->addItem(sv->tab_name());
    }
    stv_name->setCurrentIndex(0);
}


void
QTstdViaDlg::apply_btn_slot()
{
    if (!Tech()->StdViaTab())
        return;
    QByteArray name_ba = stv_name->currentText().toLatin1();
    const char *nm = name_ba.constData();
    if (!nm)
        return;
    sStdVia *sv = Tech()->FindStdVia(nm);
    if (!sv)
        return;
    if (!sv->bottom() || !sv->top())
        return;
    sStdVia rv(0, sv->via(), sv->bottom(), sv->top());
    rv.set_via_wid(INTERNAL_UNITS(stv_sb_wid->value()));
    rv.set_via_hei(INTERNAL_UNITS(stv_sb_hei->value()));
    rv.set_via_rows(mmRnd(stv_sb_rows->value()));
    rv.set_via_cols(mmRnd(stv_sb_cols->value()));
    rv.set_via_spa_x(INTERNAL_UNITS(stv_sb_spa_x->value()));
    rv.set_via_spa_y(INTERNAL_UNITS(stv_sb_spa_y->value()));
    rv.set_bot_enc_x(INTERNAL_UNITS(stv_sb_enc1_x->value()));
    rv.set_bot_enc_y(INTERNAL_UNITS(stv_sb_enc1_y->value()));
    rv.set_bot_off_x(INTERNAL_UNITS(stv_sb_off1_x->value()));
    rv.set_bot_off_y(INTERNAL_UNITS(stv_sb_off1_y->value()));
    rv.set_top_enc_x(INTERNAL_UNITS(stv_sb_enc2_x->value()));
    rv.set_top_enc_y(INTERNAL_UNITS(stv_sb_enc2_y->value()));
    rv.set_top_off_x(INTERNAL_UNITS(stv_sb_off2_x->value()));
    rv.set_top_off_y(INTERNAL_UNITS(stv_sb_off2_y->value()));
    rv.set_org_off_x(INTERNAL_UNITS(stv_sb_org_x->value()));
    rv.set_org_off_y(INTERNAL_UNITS(stv_sb_org_y->value()));
    rv.set_implant1(sv->implant1());
    rv.set_imp1_enc_x(INTERNAL_UNITS(stv_sb_imp1_x->value()));
    rv.set_imp1_enc_y(INTERNAL_UNITS(stv_sb_imp1_y->value()));
    rv.set_implant2(sv->implant2());
    rv.set_imp2_enc_x(INTERNAL_UNITS(stv_sb_imp2_x->value()));
    rv.set_imp2_enc_y(INTERNAL_UNITS(stv_sb_imp2_y->value()));

    CDs *sd = 0;
    for (sStdVia *v = sv; v; v = v->variations()) {
        if (*v == rv) {
            sd = v->open();
            break;
        }
    }
    if (!sd) {
        sStdVia *nv = new sStdVia(rv);
        sv->add_variation(nv);
        sd = nv->open();
    }
    if (!sd) {
        // This really can't happen.
        Log()->PopUpErr("Failed to create standard via sub-master.");
        return;
    }

    if (!stv_cdesc) {
        PL()->ShowPrompt("Click on locations where to instantiate vias.");
        ED()->placeDev(stv_apply, Tstring(sd->cellname()), false);
    }
    else {
        // We require that the cdesc still be in good shape here. 
        // Nothing prevents the application from messing with it
        // between the pop-up origination and Apply button press,
        // maybe should make this modal.

        CDc *cdesc = stv_cdesc;
        if (cdesc->state() == CDobjDeleted) {
            Log()->PopUpErr("Target via instance was deleted.");
            return;
        }
        CDs *cursd = cdesc->parent();
        if (!cursd) {
            Log()->PopUpErr("Target via has no parent!");
            return;
        }

        CDcbin cbin;
        cbin.setPhys(sd);

        Ulist()->ListCheck("UPDVIA", cursd, true);
        if (!ED()->replaceInstance(cdesc, &cbin, false, false))
            Log()->PopUpErr(Errs()->get_error());
        Ulist()->CommitChanges(true);
    }
}


void
QTstdViaDlg::dismiss_btn_slot()
{
    ED()->PopUpStdVia(0, MODE_OFF);
}

