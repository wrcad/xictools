
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

#include "qtrule.h"
#include "drc_kwords.h"
#include "dsp_inlines.h"
#include "qtinterf/qtdblsb.h"

#include <QApplication>
#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>


//-----------------------------------------------------------------------------
// QTruleDlg:  Dialog to edit design rule input parameters.
// Called from the main menu: DRC/Edit Rules.
//
// Help system keywords used:
//  xic:ruleedit

// Pop up or update a design rule parameter input panel.  The username
// is the name of a user-defined rule, when type is drUserDefinedRule,
// otherwise it should be null.  The optional rule argument provides
// initial values, its type must match or it is ignored.
//
void
cDRC::PopUpRuleEdit(GRobject caller, ShowMode mode, DRCtype type,
    const char *username, bool (*callback)(const char*, void*),
    void *arg, const DRCtestDesc *rule)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTruleDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTruleDlg::self())
            QTruleDlg::self()->update(type, username, rule);
        return;
    }
    if (QTruleDlg::self()) {
        QTruleDlg::self()->update(type, username, rule);
        return;
    }

    new QTruleDlg(caller, type, username, callback, arg, rule);

    QTruleDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTruleDlg::self(),
        QTmainwin::self()->Viewport());
    QTruleDlg::self()->show();
}
// End of cDRC functions.


QTruleDlg *QTruleDlg::instPtr;

QTruleDlg::QTruleDlg(GRobject c, DRCtype type, const char *username,
    bool (*callback)(const char*, void*), void *arg, const DRCtestDesc *rule)
    : QTbag(this)
{
    instPtr = this;
    ru_caller = c;
    ru_callback = callback;
    ru_arg = arg;
    ru_username = 0;
    ru_stabstr = 0;
    ru_rule = drNoRule;

    ru_label = 0;
    ru_region_la = 0;
    ru_region_ent = 0;
    ru_inside_la = 0;
    ru_inside_ent = 0;
    ru_outside_la = 0;
    ru_outside_ent = 0;
    ru_target_la = 0;
    ru_target_ent = 0;
    ru_dimen_la = 0;
    ru_dimen_sb = 0;
    ru_area_sb = 0;
    ru_diag_la = 0;
    ru_diag_sb = 0;
    ru_net_la = 0;
    ru_net_sb = 0;
    ru_use_st = 0;
    ru_edit_st = 0;
    ru_enc_la = 0;
    ru_enc_sb = 0;
    ru_opp_la = 0;
    ru_opp_sb1 = 0;
    ru_opp_sb2 = 0;
    ru_user_la = 0;
    ru_user_ent = 0;
    ru_descr_la = 0;
    ru_descr_ent = 0;

    setWindowTitle(tr("Design Rule Parameters"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

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
    ru_label = new QLabel(tr("Set Design Rule parameters"));
    hb->addWidget(ru_label);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    QGridLayout *grid = new QGridLayout();
    vbox->addLayout(grid);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    // Region.
    //
    ru_region_la = new QLabel(tr("Layer expression to AND with source\n"
        "figures on current layer (optional)"));
    grid->addWidget(ru_region_la, 0, 0);

    ru_region_ent = new QLineEdit();;
    grid->addWidget(ru_region_ent, 0, 1);

    // Inside edge layer or expression.
    //
    ru_inside_la = new QLabel(tr(
        "Layer expression to AND at inside edges\n"
        "when forming test areas (optional)"));
    grid->addWidget(ru_inside_la, 1, 0);

    ru_inside_ent = new QLineEdit;
    grid->addWidget(ru_inside_ent, 1, 1);

    // Outside edge layer or expression.
    //
    ru_outside_la = new QLabel(tr(
        "Layer expression to AND at outside edges\n"
        "when forming test areas (optional)"));
    grid->addWidget(ru_outside_la, 2, 0);

    ru_outside_ent = new QLineEdit();;
    grid->addWidget(ru_outside_ent, 2, 1);

    // Target object layer or expression.
    //
    ru_target_la = new QLabel(tr("Target layer name or expression"));
    grid->addWidget(ru_target_la, 3, 0);

    ru_target_ent = new QLineEdit();
    grid->addWidget(ru_target_ent, 3, 1);

    // Dimension.
    //
    ru_dimen_la = new QLabel(tr("Dimension (microns)"));
    grid->addWidget(ru_dimen_la, 4, 0);

    hb = new QHBoxLayout();
    grid->addLayout(hb, 4, 1);
    hb->setContentsMargins(qm);
    hb->setSpacing(0);

    int ndgt = CD()->numDigits();
    ru_dimen_sb = new QTdoubleSpinBox();
    ru_dimen_sb->setRange(0.0, 1e6);
    ru_dimen_sb->setDecimals(ndgt);
    ru_dimen_sb->setValue(0.0);
    hb->addWidget(ru_dimen_sb);

    ru_area_sb = new QTdoubleSpinBox();
    ru_area_sb->setRange(0.0, 1e6);
    ru_area_sb->setDecimals(6);
    ru_area_sb->setValue(0.0);
    hb->addWidget(ru_area_sb);

    // Non-Manhattan "Diagional" dimension.
    //
    ru_diag_la = new QLabel(tr("Non-Manhattan \"diagonal\" dimension"));
    grid->addWidget(ru_diag_la, 5, 0);

    ru_diag_sb = new QTdoubleSpinBox();
    ru_diag_sb->setRange(0.0, 1e6);
    ru_diag_sb->setDecimals(ndgt);
    ru_diag_sb->setValue(0.0);
    grid->addWidget(ru_diag_sb, 5, 1);

    // Same-Net dimension.
    //
    ru_net_la = new QLabel(tr("Same-Net spacing"));
    grid->addWidget(ru_net_la, 6, 0);

    ru_net_sb = new QTdoubleSpinBox();
    ru_net_sb->setRange(0.0, 1e6);
    ru_net_sb->setDecimals(ndgt);
    ru_net_sb->setValue(0.0);
    grid->addWidget(ru_net_sb, 6, 1);

    ru_use_st = new QCheckBox(tr("Use spacing table"));
    grid->addWidget(ru_use_st, 7, 0);

    ru_edit_st = new QToolButton();
    ru_edit_st->setText(tr("Edit Table"));
    grid->addWidget(ru_edit_st, 7, 1);
    connect(ru_edit_st, SIGNAL(clicked()), this, SLOT(edit_table_slot()));

    // Dimension when target objects are fully enclosed.
    //
    ru_enc_la = new QLabel(tr("Dimension when target objects\n"
        "are fully enclosed"));
    grid->addWidget(ru_enc_la, 8, 0);

    ru_enc_sb = new QTdoubleSpinBox();
    ru_enc_sb->setRange(0.0, 1e6);
    ru_enc_sb->setDecimals(ndgt);
    ru_enc_sb->setValue(0.0);
    grid->addWidget(ru_enc_sb, 8, 1);

    // Opposite-side dimensions.
    //
    ru_opp_la = new QLabel(tr("Opposite side dimensions"));
    grid->addWidget(ru_opp_la, 9, 0);

    hb = new QHBoxLayout();
    grid->addLayout(hb, 9, 1);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);

    ru_opp_sb1 = new QTdoubleSpinBox();
    ru_opp_sb1->setRange(0.0, 1e6);
    ru_opp_sb1->setDecimals(ndgt);
    ru_opp_sb1->setValue(0.0);
    hb->addWidget(ru_opp_sb1);

    ru_opp_sb2 = new QTdoubleSpinBox();
    ru_opp_sb2->setRange(0.0, 1e6);
    ru_opp_sb2->setDecimals(ndgt);
    ru_opp_sb2->setValue(0.0);
    hb->addWidget(ru_opp_sb2);

    // User-defined rule arguments.
    //
    ru_user_la = new QLabel(tr("User-defined rule arguments"));
    grid->addWidget(ru_user_la, 10, 0);

    ru_user_ent = new QLineEdit();
    grid->addWidget(ru_user_ent, 10, 1);

    // Description string.
    //
    ru_descr_la = new QLabel(tr("Description string"));
    grid->addWidget(ru_descr_la, 11, 0, 1, 2);

    ru_descr_ent = new QLineEdit();
    grid->addWidget(ru_descr_ent, 12, 0, 1, 2);

    // Apply and Dismiss buttons.
    //
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    tbtn = new QToolButton();
    tbtn->setText(tr("Apply"));
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(apply_btn_slot()));

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Dismiss");
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update(type, username, rule);
}


QTruleDlg::~QTruleDlg()
{
    instPtr = 0;
    delete [] ru_username;
    delete [] ru_stabstr;
    if (ru_caller)
        QTdev::Deselect(ru_caller);
}


#ifdef Q_OS_MACOS

bool
QTruleDlg::event(QEvent *ev)
{
    // Fix for QT BUG 116674, text becomes invisible on autodefault
    // button when the main window has focus.

    if (ev->type() == QEvent::ActivationChange) {
        QPushButton *dsm = findChild<QPushButton*>("Dismiss",
            Qt::FindDirectChildrenOnly);
        if (dsm) {
            QWidget *top = this;
            while (top->parentWidget())
                top = top->parentWidget();
            if (QApplication::activeWindow() == top)
                dsm->setDefault(false);
            else if (QApplication::activeWindow() == this)
                dsm->setDefault(true);
        }
    }
    return (QDialog::event(ev));
}

#endif


void
QTruleDlg::update(DRCtype type, const char *username, const DRCtestDesc *rule)
{
    if (rule && rule->type() != type)
        rule = 0;
    delete [] ru_username;
    ru_username = (type == drUserDefinedRule ? lstring::copy(username) : 0);
    if (type == drUserDefinedRule && !ru_username) {
        PopUpMessage("Internal error, no rule name given.", true);
        return;
    }
    delete [] ru_stabstr;
    ru_stabstr = 0;
    if (rule && rule->spaceTab()) {
        sLstr lstr;
        sTspaceTable::tech_print(rule->spaceTab(), 0, &lstr);
        ru_stabstr = lstr.string_trim();
    }

    // Passing type == drNoRule when updating keeps existing rule.
    if (type != drNoRule)
        ru_rule = type;
    type = ru_rule;

    char buf[256];
    snprintf(buf, sizeof(buf), "Set Design Rule parameters for %s",
        type == drUserDefinedRule ? ru_username : DRCtestDesc::ruleName(type));
    ru_label->setText(buf);

    ru_region_ent->clear();
    ru_descr_ent->clear();
    ru_dimen_sb->setValue(0.0);
    ru_area_sb->setValue(0.0);
    if (rule) {
        char *s = rule->regionString();
        if (s) {
            ru_region_ent->setText(s);
            delete [] s;
        }
        sLstr lstr;
        rule->printComment(0, &lstr);
        if (lstr.length() > 0)
            ru_descr_ent->setText(lstr.string());
        ru_dimen_sb->setValue(MICRONS(rule->dimen()));
        ru_area_sb->setValue(rule->area());
    }

    alloff();
    switch (type) {
    case drNoRule:
        // Should not see this.
        break;

    case drExist:
        ru_descr_la->setVisible(true);
        ru_descr_ent->setVisible(true);
        break;

    case drConnected:
        ru_region_la->setVisible(true);
        ru_region_ent->setVisible(true);
        ru_descr_la->setVisible(true);
        ru_descr_ent->setVisible(true);
        break;

    case drNoHoles:
        ru_dimen_la->setText(tr("Minimum area (square microns)"));
        ru_diag_la->setText(tr("Minimum width (microns)"));
        if (rule && rule->value(0) > 0)
            ru_diag_sb->setValue(MICRONS(rule->value(0)));
        ru_region_la->setVisible(true);
        ru_region_ent->setVisible(true);
        ru_dimen_la->setVisible(true);
        ru_area_sb->setVisible(true);
        ru_diag_la->setVisible(true);
        ru_diag_sb->setVisible(true);
        ru_descr_la->setVisible(true);
        ru_descr_ent->setVisible(true);
        break;

    case drOverlap:
    case drIfOverlap:
    case drNoOverlap:
    case drAnyOverlap:
    case drPartOverlap:
    case drAnyNoOverlap:
        ru_target_ent->clear();
        if (rule) {
            char *tstr = rule->targetString(true);
            if (tstr) {
                ru_target_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_region_la->setVisible(true);
        ru_region_ent->setVisible(true);
        ru_target_la->setVisible(true);
        ru_target_ent->setVisible(true);
        ru_descr_la->setVisible(true);
        ru_descr_ent->setVisible(true);
        break;

    case drMinArea:
    case drMaxArea:
        ru_dimen_la->setText(type == drMinArea ?
                tr("Minimum area (square microns)") :
                tr("Maximum area (square microns)"));
        ru_region_la->setVisible(true);
        ru_region_ent->setVisible(true);
        ru_dimen_la->setVisible(true);
        ru_area_sb->setVisible(true);
        ru_descr_la->setVisible(true);
        ru_descr_ent->setVisible(true);
        break;

    case drMinEdgeLength:
        ru_inside_ent->clear();
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                ru_inside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_outside_ent->clear();
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                ru_outside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_dimen_la->setText(tr("Minimum edge length (microns)"));
        ru_target_ent->clear();
        if (rule) {
            char *tstr = rule->targetString(true);
            if (tstr) {
                ru_target_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_region_la->setVisible(true);
        ru_region_ent->setVisible(true);
        ru_inside_la->setVisible(true);
        ru_inside_ent->setVisible(true);
        ru_outside_la->setVisible(true);
        ru_outside_ent->setVisible(true);
        ru_target_la->setVisible(true);
        ru_target_ent->setVisible(true);
        ru_dimen_la->setVisible(true);
        ru_dimen_sb->setVisible(true);
        ru_descr_la->setVisible(true);
        ru_descr_ent->setVisible(true);
        break;

    case drMaxWidth:
        ru_inside_ent->clear();
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                ru_inside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_outside_ent->clear();
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                ru_outside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_dimen_la->setText(tr("Maximum width (microns)"));
        ru_region_la->setVisible(true);
        ru_region_ent->setVisible(true);
        ru_inside_la->setVisible(true);
        ru_inside_ent->setVisible(true);
        ru_outside_la->setVisible(true);
        ru_outside_ent->setVisible(true);
        ru_dimen_la->setVisible(true);
        ru_dimen_sb->setVisible(true);
        ru_descr_la->setVisible(true);
        ru_descr_ent->setVisible(true);
        break;

    case drMinWidth:
        ru_inside_ent->clear();
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                ru_inside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_outside_ent->clear();
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                ru_outside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_dimen_la->setText(tr("Minimum width (microns)"));
        ru_diag_la->setText(tr("Non-Manhattan \"diagonal\" width"));
        ru_diag_sb->setValue(0.0);
        if (rule) {
            if (rule->value(0) > 0)
                ru_diag_sb->setValue(MICRONS(rule->value(0)));
        }
        ru_region_la->setVisible(true);
        ru_region_ent->setVisible(true);
        ru_inside_la->setVisible(true);
        ru_inside_ent->setVisible(true);
        ru_outside_la->setVisible(true);
        ru_outside_ent->setVisible(true);
        ru_dimen_la->setVisible(true);
        ru_dimen_sb->setVisible(true);
        ru_diag_la->setVisible(true);
        ru_diag_sb->setVisible(true);
        ru_descr_la->setVisible(true);
        ru_descr_ent->setVisible(true);
        break;

    case drMinSpace:
        ru_inside_ent->clear();
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                ru_inside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_outside_ent->clear();
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                ru_outside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_dimen_la->setText(tr("Default minimum spacing (microns)"));
        ru_diag_la->setText(tr("Non-Manhattan \"diagonal\" spacing"));
        ru_diag_sb->setValue(0.0);
        ru_net_sb->setValue(0.0);
        if (rule) {
            if (rule->value(0) > 0)
                ru_diag_sb->setValue(MICRONS(rule->value(0)));
            if (rule->value(1) > 0)
                ru_net_sb->setValue(MICRONS(rule->value(1)));
        }
        QTdev::SetStatus(ru_use_st, (rule && rule->spaceTab() &&
            !(rule->spaceTab()->length & STF_IGNORE)));
        ru_region_la->setVisible(true);
        ru_region_ent->setVisible(true);
        ru_inside_la->setVisible(true);
        ru_inside_ent->setVisible(true);
        ru_outside_la->setVisible(true);
        ru_outside_ent->setVisible(true);
        ru_dimen_la->setVisible(true);
        ru_dimen_sb->setVisible(true);
        ru_diag_la->setVisible(true);
        ru_diag_sb->setVisible(true);
        ru_net_la->setVisible(true);
        ru_net_sb->setVisible(true);
        ru_use_st->setVisible(true);
        ru_edit_st->setVisible(true);
        ru_descr_la->setVisible(true);
        ru_descr_ent->setVisible(true);
        break;

    case drMinSpaceTo:
        ru_inside_ent->clear();
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                ru_inside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_outside_ent->clear();
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                ru_outside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_dimen_la->setText(tr("Default minimum spacing (microns)"));
        ru_diag_la->setText(tr("Non-Manhattan \"diagonal\" spacing"));
        ru_target_ent->clear();
        ru_diag_sb->setValue(0.0);
        ru_net_sb->setValue(0.0);
        if (rule) {
            char *tstr = rule->targetString(true);
            if (tstr) {
                ru_target_ent->setText(tstr);
                delete [] tstr;
            }
            if (rule->value(0) > 0)
                ru_diag_sb->setValue(MICRONS(rule->value(0)));
            if (rule->value(1) > 0)
                ru_net_sb->setValue(MICRONS(rule->value(1)));
        }
        QTdev::SetStatus(ru_use_st, (rule && rule->spaceTab() &&
            !(rule->spaceTab()->length & STF_IGNORE)));
        ru_region_la->setVisible(true);
        ru_region_ent->setVisible(true);
        ru_inside_la->setVisible(true);
        ru_inside_ent->setVisible(true);
        ru_outside_la->setVisible(true);
        ru_outside_ent->setVisible(true);
        ru_target_la->setVisible(true);
        ru_target_ent->setVisible(true);
        ru_dimen_la->setVisible(true);
        ru_dimen_sb->setVisible(true);
        ru_diag_la->setVisible(true);
        ru_diag_sb->setVisible(true);
        ru_net_la->setVisible(true);
        ru_net_sb->setVisible(true);
        ru_use_st->setVisible(true);
        ru_edit_st->setVisible(true);
        ru_descr_la->setVisible(true);
        ru_descr_ent->setVisible(true);
        break;

    case drMinSpaceFrom:
        ru_inside_ent->clear();
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                ru_inside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_outside_ent->clear();
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                ru_outside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_dimen_la->setText(tr("Minimum dimension (microns)"));
        ru_target_ent->clear();
        ru_enc_sb->setValue(0.0);
        ru_opp_sb1->setValue(0.0);
        ru_opp_sb2->setValue(0.0);
        if (rule) {
            char *tstr = rule->targetString(true);
            if (tstr) {
                ru_target_ent->setText(tstr);
                delete [] tstr;
            }
            if (rule->value(0) > 0)
                ru_enc_sb->setValue(MICRONS(rule->value(0)));
            if (rule->value(1) > 0 || rule->value(2) > 0) {
                ru_opp_sb1->setValue(MICRONS(rule->value(1)));
                ru_opp_sb2->setValue(MICRONS(rule->value(2)));
            }
        }
        ru_region_la->setVisible(true);
        ru_region_ent->setVisible(true);
        ru_inside_la->setVisible(true);
        ru_inside_ent->setVisible(true);
        ru_outside_la->setVisible(true);
        ru_outside_ent->setVisible(true);
        ru_target_la->setVisible(true);
        ru_target_ent->setVisible(true);
        ru_dimen_la->setVisible(true);
        ru_dimen_sb->setVisible(true);
        ru_enc_la->setVisible(true);
        ru_enc_sb->setVisible(true);
        ru_opp_la->setVisible(true);
        ru_opp_sb1->setVisible(true);
        ru_opp_sb2->setVisible(true);
        ru_descr_la->setVisible(true);
        ru_descr_ent->setVisible(true);
        break;

    case drMinOverlap:
    case drMinNoOverlap:
        ru_inside_ent->clear();
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                ru_inside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_outside_ent->clear();
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                ru_outside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_dimen_la->setText(tr("Minimum dimension (microns)"));
        ru_target_ent->clear();
        if (rule) {
            char *tstr = rule->targetString(true);
            if (tstr) {
                ru_target_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_region_la->setVisible(true);
        ru_region_ent->setVisible(true);
        ru_inside_la->setVisible(true);
        ru_inside_ent->setVisible(true);
        ru_outside_la->setVisible(true);
        ru_outside_ent->setVisible(true);
        ru_target_la->setVisible(true);
        ru_target_ent->setVisible(true);
        ru_dimen_la->setVisible(true);
        ru_dimen_sb->setVisible(true);
        ru_descr_la->setVisible(true);
        ru_descr_ent->setVisible(true);
        break;

    case drUserDefinedRule:
        ru_inside_ent->clear();
        if (rule) {
            char *tstr = rule->insideString(true);
            if (tstr) {
                ru_inside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_outside_ent->clear();
        if (rule) {
            char *tstr = rule->outsideString(true);
            if (tstr) {
                ru_outside_ent->setText(tstr);
                delete [] tstr;
            }
        }
        ru_user_ent->clear();
        if (rule) {
            const DRCtest *ur = rule->userRule();
            char *astr = ur ? ur->argString() : 0;
            if (astr) {
                ru_user_ent->setText(astr);
                delete [] astr;
            }
        }
        DRCtest *tst = DRC()->userTests();
        for ( ; tst; tst = tst->next()) {
            if (lstring::cieq(tst->name(), ru_username))
                break;
        }
        if (tst)
            snprintf(buf, sizeof(buf),
                "User-defined rule arguments (%d required)", tst->argc());
        else
            snprintf(buf, sizeof(buf),
                "User-defined rule arguments (warning: unknown rule)");
        ru_user_la->setText(buf);
        ru_region_la->setVisible(true);
        ru_region_ent->setVisible(true);
        ru_inside_la->setVisible(true);
        ru_inside_ent->setVisible(true);
        ru_outside_la->setVisible(true);
        ru_outside_ent->setVisible(true);
        ru_user_la->setVisible(true);
        ru_user_ent->setVisible(true);
        ru_descr_la->setVisible(true);
        ru_descr_ent->setVisible(true);
        break;
    }
}


void
QTruleDlg::alloff()
{
    ru_region_la->setVisible(false);
    ru_region_ent->setVisible(false);
    ru_target_la->setVisible(false);
    ru_target_ent->setVisible(false);
    ru_inside_la->setVisible(false);
    ru_inside_ent->setVisible(false);
    ru_outside_la->setVisible(false);
    ru_outside_ent->setVisible(false);
    ru_dimen_la->setVisible(false);
    ru_dimen_sb->setVisible(false);
    ru_area_sb->setVisible(false);
    ru_diag_la->setVisible(false);
    ru_diag_sb->setVisible(false);
    ru_net_la->setVisible(false);
    ru_net_sb->setVisible(false);
    ru_use_st->setVisible(false);
    ru_edit_st->setVisible(false);
    ru_enc_la->setVisible(false);
    ru_enc_sb->setVisible(false);
    ru_opp_la->setVisible(false);
    ru_opp_sb1->setVisible(false);
    ru_opp_sb2->setVisible(false);
    ru_user_la->setVisible(false);
    ru_user_ent->setVisible(false);
    ru_descr_la->setVisible(false);
    ru_descr_ent->setVisible(false);
}


namespace {
    char *get_string(QLineEdit *w)
    {
        QByteArray ba = w->text().toLatin1();
        const char *s = ba.constData();
        if (s) {
            while (isspace(*s))
                s++;
            if (*s) {
                char *str = lstring::copy(s);
                char *e = str + strlen(str) - 1;
                while (e >= str && isspace(*e))
                    *e-- = 0;
                if (!*str) {
                    delete [] str;
                    return (0);
                }
                return (str);
            }
        }
        return (0);
    }


    // Add the SpaceTable record to lstr, but fix the IGNORE flag.  We
    // know the the string syntax is ok.
    //
    void add_stab_fix(const char *stabstr, bool ignore, sLstr &lstr)
    {
        const char *s = stabstr;
        char *tok = lstring::gettok(&s); // "SpacingTable"
        lstr.add_c(' ');
        lstr.add(tok);
        delete [] tok;
        tok = lstring::gettok(&s); // default space
        lstr.add_c(' ');
        lstr.add(tok);
        delete [] tok;
        tok = lstring::gettok(&s); // table dimensions
        lstr.add_c(' ');
        lstr.add(tok);
        delete [] tok;
        tok = lstring::gettok(&s); // flags
        unsigned long f = strtoul(tok, 0, 0);
        delete [] tok;
        if (ignore)
            f |= STF_IGNORE;
        else
            f &= ~STF_IGNORE;
        lstr.add_c(' ');
        lstr.add_h(f, true);
        lstr.add_c(' ');
        lstr.add(s);
    }
}


void
QTruleDlg::apply()
{
    const char *msg_target = "Error: the target layer must be provided.";
    const char *msg_dimen = "Error: the Dimension must be nonzero.";

    sLstr lstr;
    if (ru_rule == drUserDefinedRule)
        lstr.add(ru_username);
    else
        lstr.add(DRCtestDesc::ruleName(ru_rule));

    if (ru_rule != drExist) {
        char *rstr = get_string(ru_region_ent);
        if (rstr) {
            lstr.add_c(' ');
            lstr.add("Region ");
            lstr.add(rstr);
            delete [] rstr;
        }
    }

    switch (ru_rule) {
    case drNoRule:
        // Should not see this.
        return;

    case drExist:
    case drConnected:
        break;

    case drNoHoles:
        {
            if (ru_area_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(Dkw.MinArea());
                lstr.add_c(' ');
                lstr.add(ru_area_sb->cleanText().toLatin1().constData());
            }
            if (ru_diag_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(Dkw.MinWidth());
                lstr.add_c(' ');
                lstr.add(ru_diag_sb->cleanText().toLatin1().constData());
            }
        }
        break;

    case drOverlap:
    case drIfOverlap:
    case drNoOverlap:
    case drAnyOverlap:
    case drPartOverlap:
    case drAnyNoOverlap:
        {
            char *tstr = get_string(ru_target_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            else {
                PopUpMessage(msg_target, true);
                return;
            }
        }
        break;

    case drMinArea:
    case drMaxArea:
        {
            if (ru_area_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(ru_area_sb->cleanText().toLatin1().constData());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
        }
        break;

    case drMinEdgeLength:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_target_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            else {
                PopUpMessage(msg_target, true);
                return;
            }
            if (ru_dimen_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(ru_dimen_sb->cleanText().toLatin1().constData());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
        }
        break;

    case drMaxWidth:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            if (ru_dimen_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(ru_dimen_sb->cleanText().toLatin1().constData());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
        }
        break;

    case drMinWidth:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            if (ru_dimen_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(ru_dimen_sb->cleanText().toLatin1().constData());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
            if (ru_diag_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(Dkw.Diagonal());
                lstr.add_c(' ');
                lstr.add(ru_diag_sb->cleanText().toLatin1().constData());
            }
        }
        break;

    case drMinSpace:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            if (ru_stabstr) {
                bool ignore = !QTdev::GetStatus(ru_use_st);
                add_stab_fix(ru_stabstr, ignore, lstr);
            }
            else if (ru_dimen_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(ru_dimen_sb->cleanText().toLatin1().constData());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
            if (ru_diag_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(Dkw.Diagonal());
                lstr.add_c(' ');
                lstr.add(ru_diag_sb->cleanText().toLatin1().constData());
            }
            if (ru_net_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(Dkw.SameNet());
                lstr.add_c(' ');
                lstr.add(ru_net_sb->cleanText().toLatin1().constData());
            }
        }
        break;

    case drMinSpaceTo:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_target_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            else {
                PopUpMessage(msg_target, true);
                return;
            }
            if (ru_stabstr) {
                bool ignore = !QTdev::GetStatus(ru_use_st);
                add_stab_fix(ru_stabstr, ignore, lstr);
            }
            else if (ru_dimen_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(ru_dimen_sb->cleanText().toLatin1().constData());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
            if (ru_diag_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(Dkw.Diagonal());
                lstr.add_c(' ');
                lstr.add(ru_diag_sb->cleanText().toLatin1().constData());
            }
        }
        break;

    case drMinSpaceFrom:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_target_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            else {
                PopUpMessage(msg_target, true);
                return;
            }
            if (ru_dimen_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(ru_dimen_sb->cleanText().toLatin1().constData());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
            if (ru_enc_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(Dkw.Enclosed());
                lstr.add_c(' ');
                lstr.add(ru_enc_sb->cleanText().toLatin1().constData());
            }
            if (ru_opp_sb1->value() > 0.0 || ru_opp_sb2->value() > 0.0) {
                lstr.add_c(' ');
#ifdef Q_OS_X11
#undef Opposite
// Stupid thing in X.h.
#endif
                lstr.add(Dkw.Opposite());
                lstr.add_c(' ');
                lstr.add(ru_opp_sb1->cleanText().toLatin1().constData());
                lstr.add_c(' ');
                lstr.add(ru_opp_sb2->cleanText().toLatin1().constData());
            }
        }
        break;

    case drMinOverlap:
    case drMinNoOverlap:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_target_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            else {
                PopUpMessage(msg_target, true);
                return;
            }
            if (ru_dimen_sb->value() > 0.0) {
                lstr.add_c(' ');
                lstr.add(ru_dimen_sb->cleanText().toLatin1().constData());
            }
            else {
                PopUpMessage(msg_dimen, true);
                return;
            }
        }
        break;

    case drUserDefinedRule:
        {
            char *tstr = get_string(ru_inside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Inside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            tstr = get_string(ru_outside_ent);
            if (tstr) {
                lstr.add_c(' ');
                lstr.add(Dkw.Outside());
                lstr.add_c(' ');
                lstr.add(tstr);
                delete [] tstr;
            }
            DRCtest *tst = DRC()->userTests();
            for ( ; tst; tst = tst->next()) {
                if (lstring::cieq(tst->name(), ru_username))
                    break;
            }
            if (!tst) {
                sLstr tlstr;
                tlstr.add("Internal error:  ");
                tlstr.add(ru_username);
                tlstr.add("is not found.");
                PopUpMessage(tlstr.string(), true);
                return;
            }
            char *astr = get_string(ru_user_ent);
            if (astr) {
                int ac = 0;
                char *t = astr;
                while ((lstring::advtok(&t)) != false) {
                    if (ac == tst->argc()) {
                        // Truncate any extra args.
                        *t = 0;
                        break;
                    }
                    ac++;
                }
                if (ac != tst->argc()) {
                    sLstr tlstr;
                    tlstr.add("Error:  too few arguments given for ");
                    tlstr.add(ru_username);
                    tlstr.add_c('.');
                    PopUpMessage(tlstr.string(), true);
                    delete [] astr;
                    return;
                }
                lstr.add_c(' ');
                lstr.add(astr);
                delete [] astr;
            }
        }
        break;
    }

    char *dstr = get_string(ru_descr_ent);
    if (dstr) {
        lstr.add_c(' ');
        lstr.add(dstr);
        delete [] dstr;
    }

    if (ru_callback) {
        if (!(*ru_callback)(lstr.string(), ru_arg))
            return;
    }
    DRC()->PopUpRuleEdit(0, MODE_OFF, drNoRule, 0, 0, 0, 0);
}


// Static function.
bool
QTruleDlg::ru_edit_cb(const char *string, void*, XEtype xet)
{
    if (!instPtr)
        return (false);
    if (xet == XE_SAVE) {
        if (string) {
            const char *s = string;
            const char *err;
            sTspaceTable *t = sTspaceTable::tech_parse(&s, &err);
            if (!t) {
                if (err) {
                    instPtr->PopUpErr(MODE_ON, err);
                    return (false);
                }
                // Empty return is ok.
                delete [] string;
                string = 0;
            }
            delete [] t;
        }
        delete [] instPtr->ru_stabstr;
        instPtr->ru_stabstr = (char*)string;  // Yes, this is a copy.
        return (true);
    }
    return (false);
}


void
QTruleDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:ruleedit"))
}


void
QTruleDlg::edit_table_slot()
{
    PopUpStringEditor(ru_stabstr, ru_edit_cb, 0);
}


void
QTruleDlg::apply_btn_slot()
{
    apply();
}


void
QTruleDlg::dismiss_btn_slot()
{
    DRC()->PopUpRuleEdit(0, MODE_OFF, drNoRule, 0, 0, 0, 0);
}

