
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "qtdebug.h"
#include "graph.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "simulator.h"
#include "qttoolb.h"

#include <QLayout>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QAction>
#include <QCheckBox>


/**************************************************************************
 * Debug parameter setting dialog.
 **************************************************************************/


// The debug parameter setting dialog, initiated from the Tools menu.
//
void
QTtoolbar::PopUpDebugDefs(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        if (QTdebugParamDlg::self())
            QTdebugParamDlg::self()->deleteLater();
        return;
    }
    if (QTdebugParamDlg::self())
        return;

    new QTdebugParamDlg(x, y);
    QTdebugParamDlg::self()->show();
}
// End of QTtoolbar functions.


typedef void ParseNode;
#include "spnumber/spparse.h"


#define KWGET(string) (xKWent*)sHtab::get(Sp.Options(), string)

/*
namespace {
    void dbg_cancel_proc(GtkWidget*, void*);
    void dbg_help_proc(GtkWidget*, void*);
    void dbg_debug_proc(GtkWidget*, void*);
    void dbg_upd_proc(GtkWidget*, void*);
}
*/


QTdebugParamDlg *QTdebugParamDlg::instPtr;

QTdebugParamDlg::QTdebugParamDlg(int xx, int yy)
{
    instPtr = this;
    db_dbent = 0;

    setWindowTitle(tr("Debug Options"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);
    QLabel *label = new QLabel(tr("Debug Options"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    btn->setCheckable(true);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(toggled(bool)), this, SLOT(help_btn_slot(bool)));

    QGridLayout *grid = new QGridLayout();
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);
    vbox->addLayout(grid);

    xKWent *entry = KWGET(kw_program);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            CP.Program());
        grid->addWidget(entry->qtent(), 0, 0, 1, 2);
        entry->qtent()->active()->setEnabled(false);
    }

    entry = KWGET(kw_display);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            CP.Display());
        grid->addWidget(entry->qtent(), 0, 2, 1, 2);
        entry->qtent()->active()->setEnabled(false);
    }

    entry = KWGET(kw_debug);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_CB, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 0, 1, 2);
        connect(entry->qtent()->active(), SIGNAL(stateChanged(int)),
            this, SLOT(set_btn_slot(int)));
    }
    db_dbent = entry;

    entry = KWGET(kw_async);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_CB, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 2);
        connect(entry->qtent()->active(), SIGNAL(stateChanged(int)),
            this, SLOT(set_btn_slot(int)));
    }

    entry = KWGET(kw_control);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_CB, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 3);
        connect(entry->qtent()->active(), SIGNAL(stateChanged(int)),
            this, SLOT(set_btn_slot(int)));
    }

    entry = KWGET(kw_cshpar);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_CB, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 2, 0);
        connect(entry->qtent()->active(), SIGNAL(stateChanged(int)),
            this, SLOT(set_btn_slot(int)));
    }

    entry = KWGET(kw_eval);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_CB, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 2, 1);
        connect(entry->qtent()->active(), SIGNAL(stateChanged(int)),
            this, SLOT(set_btn_slot(int)));
    }

    entry = KWGET(kw_ginterface);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_CB, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 2, 2);
        connect(entry->qtent()->active(), SIGNAL(stateChanged(int)),
            this, SLOT(set_btn_slot(int)));
    }

    entry = KWGET(kw_helpsys);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_CB, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 2, 3);
        connect(entry->qtent()->active(), SIGNAL(stateChanged(int)),
            this, SLOT(set_btn_slot(int)));
    }

    entry = KWGET(kw_plot);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_CB, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 0);
        connect(entry->qtent()->active(), SIGNAL(stateChanged(int)),
            this, SLOT(set_btn_slot(int)));
    }

    entry = KWGET(kw_parser);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_CB, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 1);
        connect(entry->qtent()->active(), SIGNAL(stateChanged(int)),
            this, SLOT(set_btn_slot(int)));
    }

    entry = KWGET(kw_siminterface);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_CB, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 2);
        connect(entry->qtent()->active(), SIGNAL(stateChanged(int)),
            this, SLOT(set_btn_slot(int)));
    }

    entry = KWGET(kw_vecdb);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_CB, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 3);
        connect(entry->qtent()->active(), SIGNAL(stateChanged(int)),
            this, SLOT(set_btn_slot(int)));
    }

    entry = KWGET(kw_trantrace);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, "0");
        grid->addWidget(entry->qtent(), 4, 0, 1, 2);
        entry->qtent()->setup(0.0, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(kw_term);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            "");
        grid->addWidget(entry->qtent(), 4, 2, 1, 2);
    }

    entry = KWGET(kw_dontplot);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 5, 0);
    }

    entry = KWGET(kw_nosubckt);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 5, 1);
    }

    entry = KWGET(kw_strictnumparse);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 5, 2, 1, 2);
    }

    if (xx || yy) {
        TB()->FixLoc(&xx, &yy);
        move(xx, yy);
    }
    TB()->SetActiveDlg(tid_debug, this);
}


QTdebugParamDlg::~QTdebugParamDlg()
{
    TB()->PopUpTBhelp(MODE_OFF, 0, 0, TBH_DB);
    instPtr = 0;
    TB()->SetLoc(tid_debug, this);
    TB()->SetActiveDlg(tid_debug, 0);
    QTtoolbar::entries(tid_debug)->action()->setChecked(false);
}


void
QTdebugParamDlg::dismiss_btn_slot()
{
    TB()->PopUpDebugDefs(MODE_OFF, 0, 0);
}


void
QTdebugParamDlg::help_btn_slot(bool state)
{
    if (state)
        TB()->PopUpTBhelp(MODE_ON, this, sender(), TBH_DB);
    else
        TB()->PopUpTBhelp(MODE_OFF, 0, 0, TBH_DB);
}


void
QTdebugParamDlg::set_btn_slot(int state)
{
    QTkwent *ent = static_cast<QTkwent*>(sender()->parent());
    if (ent->kwstruct() != db_dbent) {
        // Not from the "debug" kwyword.
//XXX fix state
        bool state = QTdev::GetStatus(db_dbent->qtent()->active());
        if (!state) {
            state = QTdev::GetStatus(sender());
            if (state)
                QTdev::SetStatus(db_dbent->qtent()->active(), true);
        }
        else {
            bool st = false;
            for (int i = 0; KW.dbargs(i)->word; i++) {
                xKWent *k = static_cast<xKWent*>(KW.dbargs(i));
                st = QTdev::GetStatus(k->qtent()->active());
                if (st)
                   break;
            }
            if (!st) {
                variable v;
                v.set_boolean(false);
                QTdev::SetStatus(db_dbent->qtent()->active(), false);
                db_dbent->callback(false, &v);
                return;
            }
        }
//        dbg_debug_proc(entry->ent->active, entry);
    }
    bool isset = QTdev::GetStatus(sender());
    variable v;
    if (!isset) {
        v.set_boolean(false);
        db_dbent->callback(false, &v);
    }
    else {
        int i, j;
        for (j = 0, i = 0; KW.dbargs(i)->word; i++) {
            xKWent *k = static_cast<xKWent*>(KW.dbargs(i));
            if (QTdev::GetStatus(k->qtent()->active()))
               j++;
        }
        if (j == i || j == 0) {
            v.set_boolean(true);
            db_dbent->callback(true, &v);
        }
        else if (j == 1) {
            for (i = 0; KW.dbargs(i)->word; i++) {
                xKWent *k = static_cast<xKWent*>(KW.dbargs(i));
                if (QTdev::GetStatus(k->qtent()->active())) {
                    v.set_string(k->word);
                    break;
                }
            }
            db_dbent->callback(true, &v);
        }
        else {
            variable *vl = 0, *vl0 = 0;
            for (i = 0; KW.dbargs(i)->word; i++) {
                xKWent *k = static_cast<xKWent*>(KW.dbargs(i));
                if (QTdev::GetStatus(k->qtent()->active())) {
                    if (!vl)
                        vl = vl0 = new variable;
                    else {
                        vl->set_next(new variable);
                        vl = vl->next();
                    }
                    vl->set_string(k->word);
                }
            }
            v.set_list(vl0);
            db_dbent->callback(true, &v);
        }
    }
}

