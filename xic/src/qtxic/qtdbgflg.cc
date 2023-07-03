
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

#include "qtdbgflg.h"
#include "tech_ldb3d.h"
#include "extif.h"
#include "scedif.h"
#include "oa_if.h"
#include "si_lisp.h"
#include "dsp_inlines.h"
#include "errorlog.h"
#include "miscutil/filestat.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>


//--------------------------------------------------------------------
// Pop-up to control program logging and debugging flags.
//
// Help system keywords used:
// xic:dblog

void
cMain::PopUpDebugFlags(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTdbgFlagsDlg::self())
            QTdbgFlagsDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTdbgFlagsDlg::self())
            QTdbgFlagsDlg::self()->update();
        return;
    }
    if (QTdbgFlagsDlg::self())
        return;

    new QTdbgFlagsDlg(caller);

    QTdev::self()->SetPopupLocation(GRloc(), QTdbgFlagsDlg::self(),
        QTmainwin::self()->Viewport());
    QTdbgFlagsDlg::self()->show();
}


QTdbgFlagsDlg *QTdbgFlagsDlg::instPtr;

QTdbgFlagsDlg::QTdbgFlagsDlg(GRobject c)
{
    instPtr = this;
    df_caller = c;
    df_sel = 0;
    df_undo = 0;
    df_ldb3d = 0;
    df_rlsolv = 0;
    df_fname = 0;
    df_lisp = 0;
    df_connect = 0;
    df_rlsolvlog = 0;
    df_group = 0;
    df_extract = 0;
    df_assoc = 0;
    df_verbose = 0;
    df_load = 0;
    df_net = 0;
    df_pcell = 0;

    setWindowTitle("Logging Options");
    setAttribute(Qt::WA_DeleteOnClose);
//    gtk_window_set_resizable(GTK_WINDOW(df_popup), false);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // label in frame plus help btn
    //
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setMargin(0);
    hb->setSpacing(2);
    QLabel *label = new QLabel(tr("Enable debugging messages"));
    hb->addWidget(label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // check boxes
    //
    df_sel = new QCheckBox(tr("Selection list consistency"));
    vbox->addWidget(df_sel);
    connect(df_sel, SIGNAL(stateChanged(int)),
        this, SLOT(sel_btn_slot(int)));

    df_undo = new QCheckBox(tr("Undo/redo list processing"));
    vbox->addWidget(df_undo);
    connect(df_undo, SIGNAL(stateChanged(int)),
        this, SLOT(undo_btn_slot(int)));

    if (ExtIf()->hasExtract()) {
        df_ldb3d = new QCheckBox(tr(
            "3-D processing (cross sections, C and LR extract)"));
    }
    else {
        df_ldb3d = new QCheckBox(tr(
            "3-D processing for cross sections"));
    }
    vbox->addWidget(df_ldb3d);
    connect(df_ldb3d, SIGNAL(stateChanged(int)),
        this, SLOT(ldb3d_btn_slot(int)));

    if (ExtIf()->hasExtract()) {
        df_rlsolv = new QCheckBox(tr("Net resistance solver"));
        vbox->addWidget(df_rlsolv);
        connect(df_rlsolv, SIGNAL(stateChanged(int)),
            this, SLOT(rlsolv_btn_slot(int)));
    }

    // file name entry
    //
    hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    label = new QLabel(tr("Message file:"));
    hbox->addWidget(label);

    df_fname = new QLineEdit();
    hbox->addWidget(df_fname);
    connect(df_fname, SIGNAL(editingFinished()),
        this, SLOT(editing_finished_slot()));


    // Log file creation
    //
    label = new QLabel(tr("Enable optional log files"));
    label->setAlignment(Qt::AlignCenter);
    vbox->addWidget(label);

    df_lisp = new QCheckBox(tr("Lisp parser (lisp.log)"));
    vbox->addWidget(df_lisp);
    connect(df_lisp, SIGNAL(stateChanged(int)),
        this, SLOT(lisp_btn_slot(int)));

    if (ScedIf()->hasSced()) {
        df_connect = new QCheckBox(tr("Schematic connectivity (connect.log)"));
        vbox->addWidget(df_connect);
        connect(df_connect, SIGNAL(stateChanged(int)),
            this, SLOT(connect_btn_slot(int)));
    }
    if (ExtIf()->hasExtract()) {
        df_rlsolvlog = new QCheckBox(tr(
            "Resistance/inductance extraction (rlsolver.log)"));
        vbox->addWidget(df_rlsolvlog);
        connect(df_rlsolvlog, SIGNAL(stateChanged(int)),
            this, SLOT(rlsolvlog_btn_slot(int)));

        gb = new QGroupBox(tr("Grouping/Extraction/Association"));
        vbox->addWidget(gb);
        hb = new QHBoxLayout(gb);
        hb->setMargin(0);
        hb->setSpacing(2);

        df_group = new QCheckBox(tr("Group"));
        hb->addWidget(df_group);
        connect(df_group, SIGNAL(stateChanged(int)),
            this, SLOT(group_btn_slot(int)));

        df_extract = new QCheckBox(tr("Extract"));;
        hb->addWidget(df_extract);
        connect(df_extract, SIGNAL(stateChanged(int)),
            this, SLOT(extract_btn_slot(int)));

        df_assoc = new QCheckBox(tr("Assoc"));;
        hb->addWidget(df_assoc);
        connect(df_assoc, SIGNAL(stateChanged(int)),
            this, SLOT(assoc_btn_slot(int)));

        df_verbose = new QCheckBox(tr("Verbose"));;
        hb->addWidget(df_verbose);
        connect(df_verbose, SIGNAL(stateChanged(int)),
            this, SLOT(verbose_btn_slot(int)));
    }

    if (OAif()->hasOA()) {
        gb = new QGroupBox(tr("OpenAccess (oa_debug.log)"));
        vbox->addWidget(gb);
        hb = new QHBoxLayout(gb);
        hb->setMargin(0);
        hb->setSpacing(2);

        df_load = new QCheckBox(tr("Load"));;
        hb->addWidget(df_load);
        connect(df_load, SIGNAL(stateChanged(int)),
            this, SLOT(load_btn_slot(int)));

        df_net = new QCheckBox(tr("Net"));
        hb->addWidget(df_net);
        connect(df_net, SIGNAL(stateChanged(int)),
            this, SLOT(net_btn_slot(int)));

        df_pcell = new QCheckBox(tr("PCell"));
        hb->addWidget(df_pcell);
        connect(df_pcell, SIGNAL(stateChanged(int)),
            this, SLOT(pcell_btn_slot(int)));
    }

    // dismiss button
    //
    btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()),
        this, SLOT(dismiss_btn_slot()));

    update();
}


QTdbgFlagsDlg::~QTdbgFlagsDlg()
{
    instPtr = 0;
    if (df_caller)
        QTdev::Deselect(df_caller);
}


void
QTdbgFlagsDlg::update()
{
    unsigned int flags = XM()->DebugFlags();
    QTdev::SetStatus(df_sel, flags & DBG_SELECT);
    QTdev::SetStatus(df_undo, flags & DBG_UNDOLIST);
    QTdev::SetStatus(df_ldb3d, Ldb3d::logging());
    if (ExtIf()->hasExtract()) {
        QTdev::SetStatus(df_rlsolv, ExtIf()->rlsolverMsgs());
    }

    const char *fn = XM()->DebugFile();
    if (!fn)
        fn = "";
    df_fname->setText(fn);
    QTdev::SetStatus(df_lisp, cLispEnv::is_logging());
    QTdev::SetStatus(df_connect, ScedIf()->logConnect());
    if (ExtIf()->hasExtract()) {
        QTdev::SetStatus(df_rlsolvlog, ExtIf()->logRLsolver());
        QTdev::SetStatus(df_group, ExtIf()->logGrouping());
        QTdev::SetStatus(df_extract, ExtIf()->logExtracting());
        QTdev::SetStatus(df_assoc, ExtIf()->logAssociating());
        QTdev::SetStatus(df_verbose, ExtIf()->logVerbose());
    }
    if (OAif()->hasOA()) {
        const char *str = OAif()->set_debug_flags(0, 0);
        const char *s = strstr(str, "load=");
        if (s) {
            s += 5;
            QTdev::SetStatus(df_load, *s != '0');
        }
        s = strstr(str, "net=");
        if (s) {
            s += 4;
            QTdev::SetStatus(df_net, *s != '0');
        }
        s = strstr(str, "pcell=");
        if (s) {
            s += 6;
            QTdev::SetStatus(df_pcell, *s != '0');
        }
    }
}


void
QTdbgFlagsDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:dblog"))
}


void
QTdbgFlagsDlg::sel_btn_slot(int state)
{
    unsigned int f = XM()->DebugFlags();
    if (state)
        f |= DBG_SELECT;
    else
        f &= ~DBG_SELECT;
    XM()->SetDebugFlags(f);
}


void
QTdbgFlagsDlg::undo_btn_slot(int state)
{
    unsigned int f = XM()->DebugFlags();
    if (state)
        f |= DBG_UNDOLIST;
    else
        f &= ~DBG_UNDOLIST;
    XM()->SetDebugFlags(f);
}


void
QTdbgFlagsDlg::ldb3d_btn_slot(int state)
{
    Ldb3d::set_logging(state);
}


void
QTdbgFlagsDlg::rlsolv_btn_slot(int state)
{
    ExtIf()->setRLsolverMsgs(state);
}


void
QTdbgFlagsDlg::editing_finished_slot()
{
    const char *text = lstring::copy(df_fname->text().toLatin1().constData());
    const char *t = text;
    char *fn = lstring::getqtok(&t);
    delete [] text;
    if (fn) {
        if (!strcmp(fn, "stdout")) {
            if (XM()->DebugFp()) {
                fclose(XM()->DebugFp());
                XM()->SetDebugFp(0);
            }
            delete [] XM()->DebugFile();
            XM()->SetDebugFile(fn);
        }
        else if (!strcmp(fn, "stderr")) {
            if (XM()->DebugFp()) {
                fclose(XM()->DebugFp());
                XM()->SetDebugFp(0);
            }
            delete [] XM()->DebugFile();
            XM()->SetDebugFile(0);
            delete [] fn;
        }
        else {
            if (XM()->DebugFile() && !strcmp(XM()->DebugFile(), fn)) {
                delete [] fn;
                return;
            }
            FILE *fp = filestat::open_file(fn, "w");
            if (!fp) {
                Log()->ErrorLog(mh::Initialization, filestat::error_msg());
                delete [] fn;
                return;
            }
            if (XM()->DebugFp()) {
                fclose(XM()->DebugFp());
                XM()->SetDebugFp(fp);
            }
            delete [] XM()->DebugFile();
            XM()->SetDebugFile(fn);
        }
    }
    else {
        if (XM()->DebugFp()) {
            fclose(XM()->DebugFp());
            XM()->SetDebugFp(0);
        }
        delete [] XM()->DebugFile();
        XM()->SetDebugFile(0);
    }
}


void
QTdbgFlagsDlg::lisp_btn_slot(int state)
{
    cLispEnv::set_logging(state);
}


void
QTdbgFlagsDlg::connect_btn_slot(int state)
{
    ScedIf()->setLogConnect(state);
}


void
QTdbgFlagsDlg::rlsolvlog_btn_slot(int state)
{
    ExtIf()->setLogRLsolver(state);
}


void
QTdbgFlagsDlg::group_btn_slot(int state)
{
    ExtIf()->setLogGrouping(state);
}


void
QTdbgFlagsDlg::extract_btn_slot(int state)
{
    ExtIf()->setLogExtracting(state);
}


void
QTdbgFlagsDlg::assoc_btn_slot(int state)
{
    ExtIf()->setLogAssociating(state);
}


void
QTdbgFlagsDlg::verbose_btn_slot(int state)
{
    ExtIf()->setLogVerbose(state);
}


void
QTdbgFlagsDlg::load_btn_slot(int state)
{
    if (state)
        OAif()->set_debug_flags("l", 0);
    else
        OAif()->set_debug_flags(0, "l");
}


void
QTdbgFlagsDlg::net_btn_slot(int state)
{
    if (state)
        OAif()->set_debug_flags("n", 0);
    else
        OAif()->set_debug_flags(0, "n");
}


void
QTdbgFlagsDlg::pcell_btn_slot(int state)
{
    if (state)
        OAif()->set_debug_flags("p", 0);
    else
        OAif()->set_debug_flags(0, "p");
}


void
QTdbgFlagsDlg::dismiss_btn_slot()
{
    XM()->PopUpDebugFlags(0, MODE_OFF);
}

