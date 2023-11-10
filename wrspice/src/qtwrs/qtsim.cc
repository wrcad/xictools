
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
#include "qtsim.h"
#include "graph.h"
#include "circuit.h"
#include "kwords_fte.h"
#include "kwords_analysis.h"
#include "qttoolb.h"

#include <QLayout>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QAction>


/**************************************************************************
 * Simulation parameter setting dialog.
 **************************************************************************/

// The simulator parameter setting dialog, initiated from the Tools menu.
//
void
QTtoolbar::PopUpSimDefs(ShowMode mode, int x, int y)
{
    if (mode == MODE_OFF) {
        if (QTsimParamDlg::self())
            QTsimParamDlg::self()->deleteLater();
        return;
    }
    if (QTsimParamDlg::self())
        return;

    new QTsimParamDlg(x, y);
    QTsimParamDlg::self()->show();
}
// End of QTtoolbar functions.


#define STR(x) #x
#define STRINGIFY(x) STR(x)
#define KWGET(string) (xKWent*)sHtab::get(Sp.Options(), string)

namespace {
    inline void dblpr(char *buf, int n, double d, bool ex)
    {
        snprintf(buf, 32, ex ? "%.*e" : "%.*f", n, d);
    }
}


QTsimParamDlg *QTsimParamDlg::instPtr;

QTsimParamDlg::QTsimParamDlg(int xx, int yy)
{
    instPtr = this;
    si_notebook = 0;

    setWindowTitle(tr("Simulation Options"));
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
    QLabel *label = new QLabel(tr("Set simulation parameters"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    btn = new QPushButton(tr("Help"));
    btn->setCheckable(true);
    btn->setAutoDefault(false);
    hbox->addWidget(btn);
    connect(btn, SIGNAL(toggled(bool)), this, SLOT(help_btn_slot(bool)));

    si_notebook = new QTabWidget();
    vbox->addWidget(si_notebook);

    // General page
    //
    QWidget *page = new QWidget();
    si_notebook->addTab(page, tr("General"));

    QGridLayout *grid = new QGridLayout(page);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 4);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("General Parameters"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    xKWent *entry = KWGET(spkw_maxdata);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_real_func, entry,
            "256000");
        grid->addWidget(entry->qtent(), 1, 0);
        entry->qtent()->setup(256000.0, 1000.0, 0, 0, 0);
    }
    entry = KWGET(spkw_extprec);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 1);

    }
    entry = KWGET(spkw_noklu);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 2);
    }
    entry = KWGET(spkw_nomatsort);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 1, 3);
    }

    entry = KWGET(spkw_fpemode);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, "0");
        grid->addWidget(entry->qtent(), 2, 0);
        entry->qtent()->setup(0, 1.0, 0.0, 0.0, 0);
    }
    entry = KWGET(spkw_savecurrent);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 2, 1);
    }
    entry = KWGET(spkw_dcoddstep);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 2, 2);
    }

    entry = KWGET(spkw_loadthrds);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, "0");
        grid->addWidget(entry->qtent(), 3, 0);
        entry->qtent()->setup(0, 1.0, 0.0, 0.0, 0);
    }
    entry = KWGET(spkw_loopthrds);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, "0");
        grid->addWidget(entry->qtent(), 3, 1);
        entry->qtent()->setup(0, 1.0, 0.0, 0.0, 0);
    }
    grid->setRowStretch(4, 1);

    // Timestep page
    //
    page = new QWidget;
    si_notebook->addTab(page, tr("Timestep"));

    grid = new QGridLayout(page);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 4);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("Timestep and Integration Parameters"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    char tbuf[64];

    entry = KWGET(spkw_steptype);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_string_func, entry,
            KW.step(0)->word, KW.KWstep);
        grid->addWidget(entry->qtent(), 1, 0);
    }

    entry = KWGET(spkw_interplev);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_polydegree));
        grid->addWidget(entry->qtent(), 1, 1);
        entry->qtent()->setup(DEF_polydegree, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(spkw_method);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_string_func, entry,
            KW.method(0)->word, KW.KWmethod);
        grid->addWidget(entry->qtent(), 2, 0);
    }

    entry = KWGET(spkw_maxord);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_maxOrder));
        grid->addWidget(entry->qtent(), 2, 1);
        entry->qtent()->setup(DEF_maxOrder, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(spkw_trapratio);
    if (entry) {
        dblpr(tbuf, 2, DEF_trapRatio, false);
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_real_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 3, 0);
        entry->qtent()->setup(DEF_trapRatio, .1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_trapcheck);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 1);
    }

    entry = KWGET(spkw_xmu);
    if (entry) {
        dblpr(tbuf, 3, DEF_xmu, false);
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_real_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 4, 0);
        entry->qtent()->setup(DEF_xmu, .01, 0.0, 0.0, 3);
    }

    entry = KWGET(spkw_spice3);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 4, 1);
    }

    entry = KWGET(spkw_trtol);
    if (entry) {
        dblpr(tbuf, 2, DEF_trtol, false);
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_real_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 5, 0);
        entry->qtent()->setup(DEF_trtol, .1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_chgtol);
    if (entry) {
        dblpr(tbuf, 2, DEF_chgtol, true);
        entry->ent = new QTkwent(KW_FLOAT, QTkwent::ke_real_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 5, 1);
        entry->qtent()->setup(DEF_chgtol, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_dphimax);
    if (entry) {
        dblpr(tbuf, 3, DEF_dphiMax, false);
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_real_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 6, 0);
        entry->qtent()->setup(DEF_dphiMax, .01, 0.0, 0.0, 3);
    }

    entry = KWGET(spkw_jjaccel);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 6, 1);
    }

    entry = KWGET(spkw_nojjtp);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 6, 2);
    }

    entry = KWGET(spkw_delmin);
    if (entry) {
        dblpr(tbuf, 2, DEF_delMin, false);
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_real_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 7, 0);
        entry->qtent()->setup(DEF_delMin, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_minbreak);
    if (entry) {
        dblpr(tbuf, 2, DEF_minBreak, false);
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_real_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 7, 1);
        entry->qtent()->setup(DEF_minBreak, 0.1, 0.0, 0.0, 2);
    }

    /* NOT CURRENTLY USED
    entry = KWGET(spkw_noiter);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 7, 2);
    }
    */
    grid->setRowStretch(8, 1);

    // Tolerance page
    //
    page = new QWidget;
    si_notebook->addTab(page, tr("Tolerance"));

    grid = new QGridLayout(page);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 4);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("Tolerance Parameters"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(spkw_abstol);
    if (entry) {
        dblpr(tbuf, 2, DEF_abstol, true);
        entry->ent = new QTkwent(KW_FLOAT, QTkwent::ke_real_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 1, 0);
        entry->qtent()->setup(DEF_abstol, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_reltol);
    if (entry) {
        dblpr(tbuf, 2, DEF_reltol, true);
        entry->ent = new QTkwent(KW_FLOAT, QTkwent::ke_real_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 1, 1);
        entry->qtent()->setup(DEF_reltol, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_vntol);
    if (entry) {
        dblpr(tbuf, 2, DEF_voltTol, true);
        entry->ent = new QTkwent(KW_FLOAT, QTkwent::ke_real_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 2, 0);
        entry->qtent()->setup(DEF_voltTol, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_pivrel);
    if (entry) {
        dblpr(tbuf, 2, DEF_pivotRelTol, true);
        entry->ent = new QTkwent(KW_FLOAT, QTkwent::ke_real_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 2, 1);
        entry->qtent()->setup(DEF_pivotRelTol, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_gmin);
    if (entry) {
        dblpr(tbuf, 2, DEF_gmin, true);
        entry->ent = new QTkwent(KW_FLOAT, QTkwent::ke_real_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 3, 0);
        entry->qtent()->setup(DEF_gmin, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_pivtol);
    if (entry) {
        dblpr(tbuf, 2, DEF_pivotAbsTol, true);
        entry->ent = new QTkwent(KW_FLOAT, QTkwent::ke_real_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 3, 1);
        entry->qtent()->setup(DEF_pivotAbsTol, 0.1, 0.0, 0.0, 2);
    }
    grid->setRowStretch(4, 1);

    // Convergence page
    //
    page = new QWidget();
    si_notebook->addTab(page, tr("Convergence"));

    grid = new QGridLayout(page);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 4);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("Convergence Parameters"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(spkw_gminsteps);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_numGminSteps));
        grid->addWidget(entry->qtent(), 1, 0);
        entry->qtent()->setup(DEF_numGminSteps, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(spkw_srcsteps);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_numSrcSteps));
        grid->addWidget(entry->qtent(), 1, 1);
        entry->qtent()->setup(DEF_numSrcSteps, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(spkw_gminfirst);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 2, 0);
    }

    entry = KWGET(spkw_noopiter);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 2, 1);
    }

    entry = KWGET(spkw_dcmu);
    if (entry) {
        dblpr(tbuf, 3, DEF_dcMu, false);
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_real_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 3, 0);
        entry->qtent()->setup(DEF_dcMu, 0.01, 0.0, 0.0, 3);
    }

    entry = KWGET(spkw_itl1);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_dcMaxIter));
        grid->addWidget(entry->qtent(), 3, 1);
        entry->qtent()->setup(DEF_dcMaxIter, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(spkw_itl2);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_dcTrcvMaxIter));
        grid->addWidget(entry->qtent(), 4, 0);
        entry->qtent()->setup(DEF_dcTrcvMaxIter, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(spkw_itl4);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_tranMaxIter));
        grid->addWidget(entry->qtent(), 4, 1);
        entry->qtent()->setup(DEF_tranMaxIter, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(spkw_itl2gmin);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_dcOpGminMaxIter));
        grid->addWidget(entry->qtent(), 5, 0);
        entry->qtent()->setup(DEF_dcOpGminMaxIter, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(spkw_itl2src);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry,
            STRINGIFY(DEF_dcOpSrcMaxIter));
        grid->addWidget(entry->qtent(), 5, 1);
        entry->qtent()->setup(DEF_dcOpSrcMaxIter, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(spkw_gmax);
    if (entry) {
        dblpr(tbuf, 2, DEF_gmax, true);
        entry->ent = new QTkwent(KW_FLOAT, QTkwent::ke_real_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 6, 0);
        entry->qtent()->setup(DEF_gmax, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_forcegmin);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 6, 1);
    }

    entry = KWGET(spkw_rampup);
    if (entry) {
        dblpr(tbuf, 2, 0.0, true);
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_real_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 7, 0);
        entry->qtent()->setup(0.0, 0.1, 0.0, 0.0, 2);
    }
    grid->setRowStretch(8, 1);

    // Devices page
    //
    page = new QWidget;
    si_notebook->addTab(page, tr("Devices"));

    grid = new QGridLayout(page);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 4);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("Device Parameters"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(spkw_defad);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%g", DEF_defaultMosAD);
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_real_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 1, 0);
        entry->qtent()->setup(DEF_defaultMosAD, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_defas);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%g", DEF_defaultMosAS);
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_real_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 1, 1);
        entry->qtent()->setup(DEF_defaultMosAS, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_defl);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%g", DEF_defaultMosL);
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_real_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 2, 0);
        entry->qtent()->setup(DEF_defaultMosL, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_defw);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%g", DEF_defaultMosW);
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_real_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 2, 1);
        entry->qtent()->setup(DEF_defaultMosW, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_bypass);
    if (entry) {
        snprintf(tbuf, sizeof(tbuf), "%d", DEF_bypass);
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_int_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 3, 0);
        entry->qtent()->setup(1.0, 1.0, 0.0, 0.0, 0);
    }

    entry = KWGET(spkw_oldlimit);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 1);
    }

    entry = KWGET(spkw_trytocompact);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 2);
    }

    entry = KWGET(spkw_resmin);
    if (entry) {
        dblpr(tbuf, 2, DEF_resmin, true);
        entry->ent = new QTkwent(KW_FLOAT, QTkwent::ke_real_func, entry, tbuf);
        grid->addWidget(entry->qtent(), 4, 0);
        entry->qtent()->setup(DEF_gmax, 0.1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_useadjoint);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 4, 1);
    }

    grid->setRowStretch(5, 1);

    // Temperature page
    //
    page = new QWidget();
    si_notebook->addTab(page, tr("Temperature"));

    grid = new QGridLayout(page);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 4);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("Temperature Parameters"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(spkw_temp);
    if (entry) {
        dblpr(tbuf, 2, DEF_temp - 273.15, false);
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_real_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 1, 0);
        entry->qtent()->setup(DEF_temp - 273.15, .1, 0.0, 0.0, 2);
    }

    entry = KWGET(spkw_tnom);
    if (entry) {
        dblpr(tbuf, 2, DEF_nomTemp - 273.15, false);
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_real_func, entry,
            tbuf);
        grid->addWidget(entry->qtent(), 1, 1);
        entry->qtent()->setup(DEF_temp - 273.15, .1, 0.0, 0.0, 2);
    }
    grid->setRowStretch(2, 1);

    // Parser page
    //
    page = new QWidget();
    si_notebook->addTab(page, tr("Parser"));

    grid = new QGridLayout(page);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    gb = new QGroupBox();
    grid->addWidget(gb, 0, 0, 1, 4);
    hb = new QHBoxLayout(gb);
    label = new QLabel(tr("Parser Parameters"));
    hb->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    entry = KWGET(kw_modelcard);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            ".model");
        grid->addWidget(entry->qtent(), 1, 0);
    }

    entry = KWGET(kw_subend);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            ".ends");
        grid->addWidget(entry->qtent(), 1, 1);
    }

    entry = KWGET(kw_subinvoke);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            "x");
        grid->addWidget(entry->qtent(), 2, 0);
    }

    entry = KWGET(kw_substart);
    if (entry) {
        entry->ent = new QTkwent(KW_NO_SPIN, QTkwent::ke_string_func, entry,
            ".subckt");
        grid->addWidget(entry->qtent(), 2, 1);
    }

    entry = KWGET(kw_nobjthack);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 0);
    }

    entry = KWGET(spkw_renumber);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 3, 1);
    }

    entry = KWGET(spkw_optmerge);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_string_func, entry,
            KW.optmerge(0)->word, KW.KWoptmerge);
        grid->addWidget(entry->qtent(), 4, 0);
    }

    entry = KWGET(spkw_parhier);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_string_func, entry,
            KW.parhier(0)->word, KW.KWparhier);
        grid->addWidget(entry->qtent(), 4, 1);
    }

    entry = KWGET(spkw_hspice);
    if (entry) {
        entry->ent = new QTkwent(KW_NORMAL, QTkwent::ke_bool_func, entry, 0);
        grid->addWidget(entry->qtent(), 5, 0);
    }
    grid->setRowStretch(6, 1);

    if (xx || yy) {
        TB()->FixLoc(&xx, &yy);
        move(xx, yy);
    }
    TB()->SetActiveDlg(tid_simdefs, this);
}


QTsimParamDlg::~QTsimParamDlg()
{
    TB()->PopUpTBhelp(MODE_OFF, 0, 0, TBH_SD);
    instPtr = 0;
    TB()->SetLoc(tid_simdefs, this);
    TB()->SetActiveDlg(tid_simdefs, 0);
    QTtoolbar::entries(tid_simdefs)->action()->setChecked(false);
}


void
QTsimParamDlg::dismiss_btn_slot()
{
    TB()->PopUpSimDefs(MODE_OFF, 0, 0);
}


void
QTsimParamDlg::help_btn_slot(bool state)
{
    if (state)
        TB()->PopUpTBhelp(MODE_ON, this, sender(), TBH_SD);
    else
        TB()->PopUpTBhelp(MODE_OFF, 0, 0, TBH_SD);
}

