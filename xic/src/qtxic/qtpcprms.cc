
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

#include "qtpcprms.h"
#include "pcell.h"
#include "pcell_params.h"
#include "dsp_inlines.h"
#include "errorlog.h"
#include "qtmenu.h"
#include "spnumber/spnumber.h"
#include "qtinterf/qtdblsb.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>


//-----------------------------------------------------------------------------
// QTpcellParamsDlg:  Dialog to edit a perhaps long list of parameters
// for a PCell.
// Called during PCell placement/instantiation.
//
// Help system keywords used:
//  xic:pcparams

//XXX Probably need to detect the need for and use exponential notation
// spin button here.

namespace {
    void start_modal(QDialog *w)
    {
        QTmenu::self()->SetSensGlobal(false);
        QTmenu::self()->SetModal(w);
        QTpkg::self()->SetOverrideBusy(true);
        DSPmainDraw(ShowGhost(ERASE))
    }


    void end_modal()
    {
        QTmenu::self()->SetModal(0);
        QTmenu::self()->SetSensGlobal(true);
        QTpkg::self()->SetOverrideBusy(false);
        DSPmainDraw(ShowGhost(DISPLAY))
    }

    bool return_flag;
}


// mode == pcpPlace:
// The panel is intended for instance placement.  The Apply button
// when pressed will create a new sub-master and set this as the
// placement master.  This is optional, as this is done anyway before
// a placement operation.  However, it may be useful to see the
// modified bounding box before placement.  Non-modal.
//
// mode == pcpPlaceScr
// For use with the Place script function, pretty much identical to
// pcpEdit.
//
// mode == pcpOpen:
// The panel is intended for opening a pcell as the top level cell. 
// Once the user sets the parameters, pressing the Open button will
// create a sub-master and make it the current cell.  Non-modal.
//
// mode == pcpEdit:
// The panel is modal, for use when updating properties.  It is used
// instead of the prompt line string editor.  Pressing Apply pops down
// with return value true, otherwise the return value is false.
//
// The PCellParam array passed is updated when the user alters
// parameters, it can not be freed while the pop-up is active.  A new
// set of parameters can be provided with MODE_UPD.
//
bool
cEdit::PopUpPCellParams(GRobject caller, ShowMode mode, PCellParam *p,
    const char *dbname, pcpMode pmode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return (false);
    if (mode == MODE_OFF) {
        delete QTpcellParamsDlg::self();
        return (false);
    }
    if (mode == MODE_UPD) {
        if (QTpcellParamsDlg::self())
            QTpcellParamsDlg::self()->update(dbname, p);
        return (false);
    }
    if (QTpcellParamsDlg::self() || pmode == pcpNone)
        return (false);

    new QTpcellParamsDlg(caller, p, dbname, pmode);

    QTpcellParamsDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_UR), QTpcellParamsDlg::self(),
        QTmainwin::self()->Viewport());
    QTpcellParamsDlg::self()->show();

    if (pmode == pcpEdit || pmode == pcpPlaceScr) {
        return_flag = false;
        start_modal(QTpcellParamsDlg::self());
        QTdev::self()->MainLoop();  // wait for user's response
        end_modal();
        if (return_flag) {
            return_flag = false;
            return (true);
        }
    }
    return (false);
}
// End of cEdit functions;


QTpcellParamsDlg *QTpcellParamsDlg::instPtr;

QTpcellParamsDlg::QTpcellParamsDlg(GRobject c, PCellParam *prm,
    const char *dbname, pcpMode mode)
{
    instPtr = this;
    pcp_caller = c;
    pcp_label = 0;
    pcp_swin = 0;

    pcp_params = 0;
    pcp_params_bak = 0;
    pcp_dbname = 0;
    pcp_mode = mode;

    setWindowTitle(tr("Parameters"));
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

    char buf[256];
    char *cname;
    if (PCellDesc::split_dbname(dbname, 0, &cname, 0)) {
        snprintf(buf, 256, "Parameters for %s", cname);
        delete [] cname;
    }
    else {
        snprintf(buf, 256, "Parameters for %s", dbname ? dbname : "cell");
    }
    pcp_label = new QLabel(buf);
    hb->addWidget(pcp_label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    pcp_swin = new QScrollArea();
    vbox->addWidget(pcp_swin);

    // Button row.
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    if (pcp_mode == pcpOpen) {
        btn = new QPushButton(tr("Open"));
        hbox->addWidget(btn);
        btn->setAutoDefault(false);
        connect(btn, SIGNAL(clicked()), this, SLOT(open_btn_slot()));
    }
    else {
        btn = new QPushButton(tr("Apply"));
        hbox->addWidget(btn);
        btn->setAutoDefault(false);
        connect(btn, SIGNAL(clicked()), this, SLOT(apply_btn_slot()));
    }

    btn = new QPushButton(tr("Reset"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(reset_btn_slot()));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update(dbname, prm);
}


QTpcellParamsDlg::~QTpcellParamsDlg()
{
    instPtr = 0;
    if (pcp_caller)
        QTdev::Deselect(pcp_caller);
    PCellParam::destroy(pcp_params_bak);
    if (pcp_mode == pcpEdit || pcp_mode == pcpPlaceScr) {
        if (QTdev::self()->LoopLevel() > 1)
            QTdev::self()->BreakLoop();
    }
}


void
QTpcellParamsDlg::update(const char *dbname, PCellParam *p0)
{
    pcp_params = p0;
    if (pcp_mode == pcpEdit || pcp_mode == pcpPlaceScr) {
        PCellParam::destroy(pcp_params_bak);
        pcp_params_bak = PCellParam::dup(p0);
    }
    if (dbname) {
        delete [] pcp_dbname;
        pcp_dbname = lstring::copy(dbname);
        char buf[256];
        char *cname;
        if (PCellDesc::split_dbname(dbname, 0, &cname, 0)) {
            snprintf(buf, 256, "Parameters for %s", cname);
            delete [] cname;
        }
        else {
            snprintf(buf, 256, "Parameters for %s", dbname);
        }
        pcp_label->setText(buf);
    }

    QWidget *page = new QWidget();
    pcp_swin->setWidget(page);
    QGridLayout *grid = new QGridLayout(page);

    int rcnt = 0;
    sLstr lstr;
    for (PCellParam *p = pcp_params; p; p = p->next()) {
        QLabel *label = new QLabel(p->name());
        grid->addWidget(label, rcnt, 0);

        char *ltext;
        QWidget *entry = setup_entry(p, lstr, &ltext);
        grid->addWidget(entry, rcnt, 1);

        /*XXX
        if (ltext) {
            label = new QLabel(ltext);
            grid->addWidget(rcnt, 2);
            delete [] ltext;
        }
        */
        rcnt++;
    }
    if (lstr.string())
        Log()->WarningLog(mh::PCells, lstr.string());
}


namespace {
    int numdgts(const PConstraint *pc)
    {
        int ndgt = 0;
        if (pc->resol_none() || pc->resol() < 1.0)
            ndgt = CD()->numDigits();
        else if (pc->resol() > 1e5)
            ndgt = 6;
        else if (pc->resol() > 1e4)
            ndgt = 5;
        else if (pc->resol() > 1e3)
            ndgt = 4;
        else if (pc->resol() > 1e2)
            ndgt = 3;
        else if (pc->resol() > 1e1)
            ndgt = 2;
        else if (pc->resol() > 1e0)
            ndgt = 1;
        return (ndgt);
    }

    double *numparse(const char *s)
    {
        while (isspace(*s) || *s == '\'' || *s == '"')
            s++;
        return (SPnum.parse(&s, false));
    }
}


QWidget *
QTpcellParamsDlg::setup_entry(PCellParam *p, sLstr &errlstr, char **ltext)
{
    if (ltext)
        *ltext = 0;
    if (!p)
        return (0);
    if (p->type() == PCPappType)
        return (0);

    QString qsname(p->name());
    pcp_hash[qsname] = p;

    // Booleans are always a check box, any constraint string is
    // ignored.
    if (p->type() == PCPbool) {
        QCheckBox *w = new QCheckBox();
        w->setObjectName(qsname);
        QTdev::SetStatus(w, p->boolVal());
        connect(w, SIGNAL(stateChanged(int)),
            this, SLOT(bool_type_slot(int)));
        return (w);
    }

    // The constraint will set the control style, so we will need to
    // coerce things to the proper type.
    const PConstraint *pc = p->constraint();
    if (pc) {
        if (pc->type() == PCchoice) {
            // An option menu.  The entries should make sense for the
            // parameter type.

            stringlist *slc = pc->choices();
            QComboBox *w = new QComboBox();
            w->setObjectName(qsname);
            int hstv = -1, i = 0;
            for (stringlist *sl = slc; sl; sl = sl->next) {
                w->addItem(sl->string);
                char buf[64];
                if (p->type() == PCPint) {
                    snprintf(buf, sizeof(buf), "%ld", p->intVal());
                    if (!strcmp(buf, sl->string))
                        hstv = i;
                }
                else if (p->type() == PCPtime) {
                    snprintf(buf, sizeof(buf), "%ld", (long)p->timeVal());
                    if (!strcmp(buf, sl->string))
                        hstv = i;
                }
                else if (p->type() == PCPfloat) {
                    double *d = numparse(sl->string);
                    if (d) {
                        float f1 = *d;
                        float f2 = p->floatVal();
                        if (f1 == f2)
                            hstv = i;
                        else if (fabs(f1 - f2) < 1e-9*(fabs(f1) + fabs(f2)))
                            hstv = i;
                    }
                }
                else if (p->type() == PCPdouble) {
                    double *d = numparse(sl->string);
                    if (d) {
                        double d1 = *d;
                        double d2 = p->doubleVal();
                        if (d1 == d2)
                            hstv = i;
                        else if (fabs(d1 - d2) < 1e-9*(fabs(d1) + fabs(d2)))
                            hstv = i;
                    }
                }
                else if (p->type() == PCPstring) {
                    if (p->stringVal() && !strcmp(p->stringVal(), sl->string))
                        hstv = i;
                }
                i++;
            }

            connect(w, SIGNAL(curentTextChanged(const QString&)),
                this, SLOT(choice_type_slot(const QString&)));
            if (hstv >= 0)
                w->setCurrentIndex(hstv);
            else {
                w->setEnabled(false);
                errlstr.add("Parameter ");
                errlstr.add(p->name());
                errlstr.add(": default value not found in choice list.\n");
            }
            return (w);
        }
        if (pc->type() == PCrange) {
            // A spin button.  Will use .4f format for floating point
            // numbers, need a way to specify print format.

            QTdoubleSpinBox *w = new QTdoubleSpinBox;
            w->setObjectName(qsname);

            int ndgt = 0;
            double minv = -1e30;
            double maxv = 1e30;
            if (!pc->low_none())
                minv = pc->low();
            if (!pc->high_none())
                maxv = pc->high();
            if (p->type() == PCPfloat || p->type() == PCPdouble ||
                    p->type() == PCPstring) {
                ndgt = numdgts(pc);
            }

            w->setRange(minv, maxv);
            if (p->type() == PCPint) {
                w->setDecimals(0);
                w->setValue(p->intVal());
            }
            else if (p->type() == PCPtime) {
                w->setDecimals(0);
                w->setValue(p->timeVal());
            }
            else if (p->type() == PCPfloat) {
                w->setDecimals(ndgt);
                w->setValue(p->floatVal());
            }
            else if (p->type() == PCPdouble) {
                w->setDecimals(ndgt);
                w->setValue(p->doubleVal());
            }
            else if (p->type() == PCPstring) {
                double *d = numparse(p->stringVal());
                if (d) {
                    w->setDecimals(ndgt);
                    w->setValue(*d);
                }
                else {
                    w->setDecimals(ndgt);
                    w->setValue(0.0);
                    w->setEnabled(false);
                    errlstr.add("Parameter ");
                    errlstr.add(p->name());
                    errlstr.add(": default value is non-numeric, with range "
                        "constraint.\n");
                }
            }
            else
                return (0);
            connect(w, SIGNAL(valueChanged(double)),
                this, SLOT(num_type_slot(double)));

            if (!pc->checkConstraint(p)) {
                w->setEnabled(false);
                errlstr.add("Parameter ");
                errlstr.add(p->name());
                errlstr.add(": default value out of range.\n");
            }
            return (w);
        }
        if (pc->type() == PCstep) {
            // As for range, but with a step increment.

            QTdoubleSpinBox *w = new QTdoubleSpinBox();;
            w->setObjectName(qsname);

            int ndgt = 0;
            double minv = -1e30;
            double maxv = 1e30;
            double del = 0.0;
            if (!pc->start_none())
                minv = pc->start();
            else
                minv = 0.0;
            if (!pc->step_none())
                del = pc->step();
            if (!pc->limit_none())
                maxv = pc->limit();
            if (p->type() == PCPfloat || p->type() == PCPdouble ||
                    p->type() == PCPstring) {
                ndgt = numdgts(pc);
            }

            w->setRange(minv, maxv);
            if (p->type() == PCPint) {
                w->setDecimals(0);
                w->setValue(p->intVal());
            }
            else if (p->type() == PCPtime) {
                w->setDecimals(0);
                w->setValue(p->timeVal());
            }
            else if (p->type() == PCPfloat) {
                w->setDecimals(ndgt);
                w->setValue(p->floatVal());
            }
            else if (p->type() == PCPdouble) {
                w->setDecimals(ndgt);
                w->setValue(p->doubleVal());
            }
            else if (p->type() == PCPstring) {
                double *d = numparse(p->stringVal());
                if (d) {
                    w->setDecimals(ndgt);
                    w->setValue(*d);
                }
                else {
                    w->setDecimals(ndgt);
                    w->setValue(0.0);
                    w->setEnabled(false);
                    errlstr.add("Parameter ");
                    errlstr.add(p->name());
                    errlstr.add(": default value is non-numeric, with step "
                        "constraint.\n");
                }
            }
            else
                return (0);
            if (del > 0.0)
                w->setSingleStep(del);
            connect(w, SIGNAL(valueChanged(double)),
                this, SLOT(num_type_slot(double)));

            if (!pc->checkConstraint(p)) {
                w->setEnabled(false);
                errlstr.add("Parameter ");
                errlstr.add(p->name());
                errlstr.add(
                    ": default value not allowed by step constraint.\n");
            }
            return (w);
        }
        if (pc->type() == PCnumStep) {
            // As for step, but allow SPICE-type numbers with scale suffixes.
            // This is not really implemented, need to see what Ciranova
            // does with this.

            QTdoubleSpinBox *w = new QTdoubleSpinBox;
            w->setObjectName(qsname);

            int ndgt = 0;
            double minv = -1e30;
            double maxv = 1e30;
            double del = 0.0;
            if (!pc->start_none())
                minv = pc->start();
            else
                minv = 0.0;
            if (!pc->step_none())
                del = pc->step();
            if (!pc->limit_none())
                maxv = pc->limit();
            if (p->type() == PCPfloat || p->type() == PCPdouble ||
                    p->type() == PCPstring) {
                ndgt = numdgts(pc);
            }
            double f = pc->scale_factor();
            if (f != 0.0) {
                minv /= f;
                maxv /= f;
                del /= f;
            }

            w->setRange(minv, maxv);
            if (p->type() == PCPint) {
                w->setDecimals(0);
                w->setValue(p->intVal());
            }
            else if (p->type() == PCPtime) {
                w->setDecimals(0);
                w->setValue(p->timeVal());
            }
            else if (p->type() == PCPfloat) {
                w->setDecimals(ndgt);
                w->setValue(p->floatVal());
            }
            else if (p->type() == PCPdouble) {
                w->setDecimals(ndgt);
                w->setValue(p->doubleVal());
            }
            else if (p->type() == PCPstring) {
                double *d = numparse(p->stringVal());
                if (d) {
                    double dd = *d;
                    if (f != 0.0)
                        dd /= f;
                    w->setDecimals(ndgt);
                    w->setValue(dd);
                }
                else {
                    w->setDecimals(ndgt);
                    w->setValue(0.0);
                    w->setEnabled(false);
                    errlstr.add("Parameter ");
                    errlstr.add(p->name());
                    errlstr.add(": default value is non-numeric, with step "
                        "constraint.\n");
                }
            }
            else
                return (0);
            if (del > 0.0)
                w->setSingleStep(del);
            connect(w, SIGNAL(valueChanged(double)),
                this, SLOT(num_type_slot(double)));

            if (!pc->checkConstraint(p)) {
                w->setEnabled(false);
                errlstr.add("Parameter ");
                errlstr.add(p->name());
                errlstr.add(
                    ": default value not allowed by numstep constraint.\n");
            }

            if (pc->scale_factor() != 0.0) {
                char buf[64];
                snprintf(buf, sizeof(buf), "scale: %s", pc->scale());
                *ltext = lstring::copy(buf);
            }
            return (w);
        }
    }

    // No constraint string, use spin button for numerical
    // entries, a plain entry area for strings.

    if (p->type() == PCPint) {
        QSpinBox *w = new QSpinBox;
        w->setObjectName(qsname);
        w->setRange(0xffffffff, 0xefffffff);
        w->setValue(p->intVal());
        connect(w, SIGNAL(valueChanged(int)),
            this, SLOT(ncint_type_slot(int)));
        return (w);
    }
    else if (p->type() == PCPtime) {
        QSpinBox *w = new QSpinBox;
        w->setObjectName(qsname);
        w->setRange(0xffffffff, 0xefffffff);
        w->setValue(p->timeVal());
        connect(w, SIGNAL(valueChanged(int)),
            this, SLOT(nctime_type_slot(int)));
        return (w);
    }
    else if (p->type() == PCPfloat) {
        QTdoubleSpinBox *w = new QTdoubleSpinBox;
        w->setObjectName(qsname);
        w->setRange(-1e30, 1e30);
        w->setDecimals(4);
        w->setValue(p->floatVal());
        connect(w, SIGNAL(valueChanged(double)),
            this, SLOT(ncfd_type_slot(double)));
        return (w);
    }
    else if (p->type() == PCPdouble) {
        QTdoubleSpinBox *w = new QTdoubleSpinBox;
        w->setObjectName(qsname);
        w->setRange(-1e30, 1e30);
        w->setDecimals(4);
        w->setValue(p->doubleVal());
        connect(w, SIGNAL(valueChanged(double)),
            this, SLOT(ncfd_type_slot(double)));
        return (w);
    }
    else if (p->type() == PCPstring) {
        QLineEdit *w = new QLineEdit();
        w->setObjectName(qsname);
        w->setText(p->stringVal());
        connect(w, SIGNAL(textChanged(const QString&)),
            this, SLOT(string_type_slot(const QString&)));
        return (w);
    }
    return (0);
}


void
QTpcellParamsDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:pcparams"))
}


void
QTpcellParamsDlg::open_btn_slot()
{
    // pcpOpen only
    CDcbin cbin;
    if (!ED()->resolvePCell(&cbin, pcp_dbname, true)) {
        Log()->ErrorLogV(mh::PCells,
            "Open failed.\n%s", Errs()->get_error());
    }
    ED()->stopPlacement();

    // If the open succeeds and the pcell is not native, open the
    // sub-master that we just created or referenced.
    if (DSP()->CurMode() == Physical && cbin.phys()) {
        if (cbin.phys()->pcType() == CDpcOA) {
            EditType et = XM()->EditCell(Tstring(cbin.cellname()), true);
            if (et != EditOK) {
                Log()->ErrorLogV(mh::PCells,
                    "Open failed.\n%s", Errs()->get_error());
            }
        }
    }
}


void
QTpcellParamsDlg::apply_btn_slot()
{
    if (pcp_mode == pcpEdit || pcp_mode == pcpPlaceScr) {
        return_flag = true;
        ED()->PopUpPCellParams(0, MODE_OFF, 0, 0, pcpNone);
    }
    else
        ED()->plUpdateSubMaster();
}


void
QTpcellParamsDlg::reset_btn_slot()
{
    if (pcp_mode == pcpEdit || pcp_mode == pcpPlaceScr) {
        // revert using local parameters.
        pcp_params->reset(pcp_params_bak);
        update(0, pcp_params);
    }
    else {
        // Revert to the super-master defaults.
        if (!ED()->resetPlacement(pcp_dbname)) {
            Log()->ErrorLogV(mh::PCells, "Parameter reset failed:\n%s",
                Errs()->get_error());
        }
    }
}


void
QTpcellParamsDlg::dismiss_btn_slot()
{
    ED()->PopUpPCellParams(0, MODE_OFF, 0, 0, pcpNone);
}


void
QTpcellParamsDlg::bool_type_slot(int state)
{
    QObject *obj = sender();
    QString qsname = obj->objectName();
    PCellParam *p = pcp_hash[qsname];
    if (state != p->boolVal())
        p->setBoolVal(state);
}


void
QTpcellParamsDlg::choice_type_slot(const QString &qstext)
{
    QObject *obj = sender();
    QString qsname = obj->objectName();
    PCellParam *p = pcp_hash[qsname];

    QByteArray text_ba = qstext.trimmed().toLatin1();
    const char *t = text_ba.constData();

    if (p->type() == PCPint) {
        int i;
        if (sscanf(t, "%d", &i) == 1 && i != p->intVal())
            p->setIntVal(i);
    }
    else if (p->type() == PCPtime) {
        long i;
        if (sscanf(t, "%ld", &i) == 1 && i != p->timeVal())
            p->setTimeVal(i);
    }
    else if (p->type() == PCPfloat) {
        double *d = numparse(t);
        if (d) {
            float f = *d;
            if (f != p->floatVal())
                p->setFloatVal(f);
        }
    }
    else if (p->type() == PCPdouble) {
        double *d = numparse(t);
        if (d) {
            if (*d != p->doubleVal())
                p->setDoubleVal(*d);
        }
    }
    else if (p->type() == PCPstring) {
        if (!p->stringVal() || strcmp(t, p->stringVal()))
            p->setStringVal(t);
    }
}


void
QTpcellParamsDlg::num_type_slot(double val)
{
    QObject *obj = sender();
    QString qsname = obj->objectName();
    PCellParam *p = pcp_hash[qsname];
    if (!p)
        return;

    if (p->type() == PCPint) {
        int i = rint(val);
        if (i != p->intVal())
            p->setIntVal(i);
    }
    else if (p->type() == PCPtime) {
        time_t t = (time_t)val;
        if (t != p->timeVal())
            p->setTimeVal(t);
    }
    else if (p->type() == PCPfloat) {
        float f = (float)val;
        if (f != p->floatVal())
            p->setFloatVal(f);
    }
    else if (p->type() == PCPdouble) {
        if (val != p->doubleVal())
            p->setDoubleVal(val);
    }
    else if (p->type() == PCPstring) {

        // Two things here.  First, for the numberStep constraint.  we
        // have to put the scale factor back, following the number. 
        // Second, in any case, if the existing string is single or
        // double quoted, apply the same quoting to the update.  This
        // is necessary for Python and TCL.

        const char *sfx = 0;
        const PConstraint *pc = p->constraint();
        if (pc && pc->type() == PCnumStep)
            sfx = pc->scale_factor() != 0.0 ? pc->scale() : 0;
        sLstr lstr;
        const char *s2 = p->stringVal();
        if (s2 && (*s2 == '\'' || *s2 == '"'))
            lstr.add_c(*s2);
        QTdoubleSpinBox *sb = qobject_cast<QTdoubleSpinBox*>(obj);
        if (!sb)
            return;
        lstr.add(sb->cleanText().toLatin1().constData());
        lstr.add(sfx);
        if (s2 && (*s2 == '\'' || *s2 == '"'))
            lstr.add_c(*s2);

        const char *s1 = lstr.string();
        if (s1 && (!s2 || strcmp(s2, s1)))
            p->setStringVal(s1);
    }
}


void
QTpcellParamsDlg::ncint_type_slot(int i)
{
    QObject *obj = sender();
    QString qsname = obj->objectName();
    PCellParam *p = pcp_hash[qsname];
    if (!p)
        return;
    if (i != p->intVal())
        p->setIntVal(i);
}


void
QTpcellParamsDlg::nctime_type_slot(int t)
{
    QObject *obj = sender();
    QString qsname = obj->objectName();
    PCellParam *p = pcp_hash[qsname];
    if (!p)
        return;
    if (t != p->timeVal())
        p->setTimeVal(t);
}


void
QTpcellParamsDlg::ncfd_type_slot(double v)
{
    QObject *obj = sender();
    QString qsname = obj->objectName();
    PCellParam *p = pcp_hash[qsname];
    if (!p)
        return;
    if (p->type() == PCPfloat) {
        if (v != p->floatVal())
            p->setFloatVal(v);
    }
    else if (p->type() == PCPdouble) {
        if (v != p->doubleVal())
            p->setDoubleVal(v);
    }
}


void
QTpcellParamsDlg::string_type_slot(const QString &qs)
{
    QObject *obj = sender();
    QString qsname = obj->objectName();
    PCellParam *p = pcp_hash[qsname];
    if (!p)
        return;
    QByteArray qs_ba = qs.trimmed().toLatin1();
    const char *t = qs_ba.constData();

    if (p->type() == PCPint) {
        int i;
        if (sscanf(t, "%d", &i) == 1 && i != p->intVal())
            p->setIntVal(i);
    }
    else if (p->type() == PCPtime) {
        long i;
        if (sscanf(t, "%ld", &i) == 1 && i != p->timeVal())
            p->setTimeVal(i);
    }
    else if (p->type() == PCPfloat) {
        double *d = numparse(t);
        if (d) {
            float f = *d;
            if (f != p->floatVal())
                p->setFloatVal(f);
        }
    }
    else if (p->type() == PCPdouble) {
        double *d = numparse(t);
        if (d) {
            if (*d != p->doubleVal())
                p->setDoubleVal(*d);
        }
    }
    else if (p->type() == PCPstring) {
        if (!p->stringVal() || strcmp(t, p->stringVal()))
            p->setStringVal(t);
    }
}

