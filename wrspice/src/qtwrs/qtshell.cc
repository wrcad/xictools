
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
#include "qtshell.h"
#include "graph.h"
#include "simulator.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "qttoolb.h"

#include <QLayout>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QLineEdit>
#include <QAction>


/**************************************************************************
 * Shell parameter setting dialog.
 **************************************************************************/

// The plot defaults popup, initiated from the toolbar.
//
void
QTtoolbar::PopUpShellDefs(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        delete QTshellParamDlg::self();
        return;
    }
    if (QTshellParamDlg::self())
        return;

    new QTshellParamDlg(x, y);
    QTshellParamDlg::self()->show();
}

/*
// Remove the plot defaults popup.  Called from the toolbar.
//
void
GTKtoolbar::PopDownShellDefs()
{
    if (!sh_shell)
        return;
    TB()->PopDownTBhelp(TBH_SH);
    SetLoc(ntb_shell, sh_shell);

    GTKdev::Deselect(tb_shell);
    g_signal_handlers_disconnect_by_func(G_OBJECT(sh_shell),
        (gpointer)sh_cancel_proc, sh_shell);

    for (int i = 0; KW.shell(i)->word; i++) {
        xKWent *ent = static_cast<xKWent*>(KW.shell(i));
        if (ent->ent) {
            if (ent->ent->entry) {
                const char *str =
                    gtk_entry_get_text(GTK_ENTRY(ent->ent->entry));
                delete [] ent->lastv1;
                ent->lastv1 = lstring::copy(str);
            }
            if (ent->ent->entry2) {
                const char *str =
                    gtk_entry_get_text(GTK_ENTRY(ent->ent->entry2));
                delete [] ent->lastv2;
                ent->lastv2 = lstring::copy(str);
            }
            delete ent->ent;
            ent->ent = 0;
        }
    }
    gtk_widget_destroy(sh_shell);
    sh_shell = 0;
    SetActive(ntb_shell, false);
}
*/

// End of QTtoolbar functions.


#define KWGET(string) (xKWent*)sHtab::get(Sp.Options(), string)

QTshellParamDlg *QTshellParamDlg::instPtr;

QTshellParamDlg::QTshellParamDlg(int xx, int yy)
{
    instPtr = this;

    setWindowTitle(tr("Shell Options"));
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
    QLabel *label = new QLabel(tr("Shell Options"));
    hb->addWidget(label);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    tbtn->setCheckable(true);
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(toggled(bool)), this, SLOT(help_btn_slot(bool)));

    QGridLayout *grid = new QGridLayout();
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);
    vbox->addLayout(grid);

    xKWent *entry = KWGET(kw_history);
    if (entry) {
        char tbuf[64];
        snprintf(tbuf, sizeof(tbuf), "%d", CP_DefHistLen);
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 0, 0, 1, 2);
        entry->qtent()->setup(CP_DefHistLen, 1.0, 0, 0, 0);
    }

    entry = KWGET(kw_prompt);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            "-> ");
        grid->addWidget(entry->qtent(), 0, 2, 1, 2);
    }

    entry = KWGET(kw_width);
    if (entry) {
        char tbuf[64];
        snprintf(tbuf, sizeof(tbuf), "%d", DEF_WIDTH);
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 1, 0, 1, 2);
        entry->qtent()->setup(DEF_WIDTH, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(kw_height);
    if (entry) {
        char tbuf[64];
        snprintf(tbuf, sizeof(tbuf), "%d", DEF_HEIGHT);
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 1, 2, 1, 2);
        entry->qtent()->setup(DEF_HEIGHT, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(kw_cktvars);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 0);
    }

    entry = KWGET(kw_ignoreeof);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 1);
    }

    entry = KWGET(kw_noaskquit);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 2);
    }

    entry = KWGET(kw_nocc);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 3);
    }

    entry = KWGET(kw_noclobber);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 4, 0);
    }

    entry = KWGET(kw_noedit);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 4, 1);
    }

    entry = KWGET(kw_noerrwin);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 4, 2);
    }

    entry = KWGET(kw_noglob);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 4, 3);
    }

    entry = KWGET(kw_nomoremode);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 5, 0);
    }

    entry = KWGET(kw_nonomatch);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 5, 1);
    }

    entry = KWGET(kw_nosort);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 5, 2);
    }

    entry = KWGET(kw_unixcom);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 5, 3);
    }

    entry = KWGET(kw_sourcepath);
    if (entry) {
        char *s = get_sourcepath();
        entry->ent = new QTkwent(KW_NO_SPIN, sourcepath_func, entry, s);
        grid->addWidget(entry->qtent(), 6, 0, 1, 4);
        delete [] s;
    }

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    if (xx || yy) {
        TB()->FixLoc(&xx, &yy);
        move(xx, yy);
    }
    TB()->SetActiveDlg(tid_shell, this);
}


QTshellParamDlg::~QTshellParamDlg()
{
    TB()->PopUpTBhelp(MODE_OFF, 0, 0, TBH_SH);
    instPtr = 0;
    TB()->SetLoc(tid_shell, this);
    TB()->SetActiveDlg(tid_shell, 0);
    QTtoolbar::entries(tid_shell)->action()->setChecked(false);
}


// Static function.
void
QTshellParamDlg::sourcepath_func(bool isset, variable*, void *entp)
{
    QTkwent *ent = static_cast<QTkwent*>(entp);
    if (ent->active()) {
        char *s;
        QTdev::SetStatus(ent->active(), isset);
        if (isset) {
            s = get_sourcepath();
            ent->entry()->setText(s ? s : "");
            ent->entry()->setReadOnly(true);
            ent->entry()->setEnabled(false);
            delete [] s;
        }
        else {
            ent->entry()->setReadOnly(false);
            ent->entry()->setEnabled(true);
        }
    }
}


// Static function.
char *
QTshellParamDlg::get_sourcepath()
{
    VTvalue vv;
    if (Sp.GetVar(kw_sourcepath, VTYP_STRING, &vv))
        return (lstring::copy(vv.get_string()));
    return (0);
}


void
QTshellParamDlg::dismiss_btn_slot()
{
    TB()->PopUpShellDefs(MODE_OFF, 0, 0);
}


void
QTshellParamDlg::help_btn_slot(bool state)
{
    if (state)
        TB()->PopUpTBhelp(MODE_ON, this, sender(), TBH_SH);
    else
        TB()->PopUpTBhelp(MODE_OFF, 0, 0, TBH_SH);
}

