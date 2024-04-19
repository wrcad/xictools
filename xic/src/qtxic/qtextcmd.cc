
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

#include "qtextcmd.h"
#include "ext.h"
#include "dsp_inlines.h"
#include "qtinterf/qtfont.h"

#include <QApplication>
#include <QLayout>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QToolButton>
#include <QPushButton>
#include <QLineEdit>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>


//-----------------------------------------------------------------------------
// QTextCmdDlg:  Configurable dialog interface for the following
// extraction commands: PNET, ENET, SOURC, EXSET.
// These are called from the main menu: Extract/Dump Phys Netlist,
// Extract/Dump Elec Netlist, Extract/Source SPICE, Extract/Source Physical.

// Pop up/down the interface.  Only one pop-up can exist at a given time.
// Args are:
//  caller          invoking button
//  mode            MODE_ON/MODE_OFF/MODE_UPD
//  cmd             buttons and other details
//  x, y            position of pop-up
// action_cb        callback function
//   const char*      button label
//   void*            action_arg
//   bool             button state
//   const char*      entry or depth
//   int, int         widget location
// action_arg       arg to callback function
// depth            initial depth
//
void
cExt::PopUpExtCmd(GRobject caller, ShowMode mode, sExtCmd *cmd,
    bool (*action_cb)(const char*, void*, bool, const char*, int, int),
    void *action_arg, int depth)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTextCmdDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTextCmdDlg::self())
            QTextCmdDlg::self()->update();
        return;
    }
    if (QTextCmdDlg::self()) {
        sExtCmd *oldmode = QTextCmdDlg::self()->excmd();
        PopUpExtCmd(0, MODE_OFF, 0, 0, 0);
        if (oldmode == cmd)
            return;
    }
    if (!cmd)
        return;

    new QTextCmdDlg(caller, cmd, action_cb, action_arg, depth);

    QTextCmdDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTextCmdDlg::self(),
        QTmainwin::self()->Viewport());
    QTextCmdDlg::self()->show();
}
// End of cExt functions.


class QTextCmdPathEdit : public QLineEdit
{
public:
    QTextCmdPathEdit(QWidget *prnt = 0) : QLineEdit(prnt) { }

    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
};


void
QTextCmdPathEdit::dragEnterEvent(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasUrls()) {
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/plain")) {
        ev->accept();
        return;
    }
    ev->ignore();
}


void
QTextCmdPathEdit::dropEvent(QDropEvent *ev)
{
    if (ev->mimeData()->hasUrls()) {
        QByteArray ba = ev->mimeData()->data("text/plain");
        setText(ba.constData());
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/twostring")) {
        // Drops from content lists may be in the form
        // "fname_or_chd\ncellname".  Keep the cellname.
        char *str = lstring::copy(ev->mimeData()->data("text/plain").constData());
        char *t = strchr(str, '\n');
        if (t)
            *t = 0;
        setText(str);
        delete [] str;
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/plain")) {
        // The default action will insert the text at the click location,
        // instead here we replace any existing text.
        QByteArray ba = ev->mimeData()->data("text/plain");
        setText(ba.constData());
        ev->accept();
        return;
    }
    ev->ignore();
}


// Depth choices are 0 -- DMAX-1, "all"
#define DMAX 6

QTextCmdDlg *QTextCmdDlg::instPtr;

QTextCmdDlg::QTextCmdDlg(GRobject c, sExtCmd *cmd,
    bool (*action_cb)(const char*, void*, bool, const char*, int, int),
    void *action_arg, int dep)
{
    instPtr = this;
    cmd_caller = c;
    cmd_depth = 0;
    cmd_label = 0;
    cmd_text = 0;
    cmd_go = 0;

    cmd_excmd = cmd;
    cmd_bx = new QCheckBox*[cmd->num_buttons()];
    for (int i = 0; i < cmd->num_buttons(); i++)
        cmd_bx[i] = 0;

    cmd_action = action_cb;
    cmd_arg = action_arg;
    cmd_helpkw = 0;

    setWindowTitle(cmd_excmd->wintitle());
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
    const char *titlemsg = 0;
    switch (cmd_excmd->type()) {
    case ExtDumpPhys:
        titlemsg = "Dump Physical Netlist";
        cmd_helpkw = "xic:pnet";
        break;
    case ExtDumpElec:
        titlemsg = "Dump Schematic Netlist";
        cmd_helpkw = "xic:enet";
        break;
    case ExtLVS:
        titlemsg = "Compare Layout Vs. Schematic";
        cmd_helpkw = "xic:lvs";
        break;
    case ExtSource:
        titlemsg = "Schematic from SPICE File";
        cmd_helpkw = "xic:sourc";
        break;
    case ExtSet:
        titlemsg = "Schematic from Layout";
        cmd_helpkw = "xic:exset";
        break;
    };
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    QLabel *label = new QLabel(tr(titlemsg));
    hb->addWidget(label);

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    gb = new QGroupBox(tr(cmd_excmd->btntitle()));
    vbox->addWidget(gb);
    QGridLayout *grid = new QGridLayout(gb);
    grid->setContentsMargins(qm);
    grid->setSpacing(2);

    // setup check buttons
    for (int i = 0; i < cmd_excmd->num_buttons(); i++) {
        if (!cmd_excmd->button(i)->name())
            continue;
        cmd_bx[i] = new QCheckBox(tr(cmd_excmd->button(i)->name()));

        int col = cmd_excmd->button(i)->col();
        int row = cmd_excmd->button(i)->row();
        grid->addWidget(cmd_bx[i], row, col);
        if (cmd_excmd->button(i)->is_active())
            QTdev::SetStatus(cmd_bx[i], true);
        connect(cmd_bx[i], SIGNAL(stateChanged(int)),
            this, SLOT(check_state_changed_slot(int)));
    }

    // set sensitivity
    for (int i = 0; i < cmd_excmd->num_buttons(); i++) {
        bool sens = false;
        bool set = false;
        for (int j = 0; j < EXT_BSENS; j++) {
            unsigned int ix = cmd_excmd->button(i)->sens()[j];
            if (ix) {
                ix--;
                if ((int)ix < cmd_excmd->num_buttons()) {
                    set = true;
                    sens = QTdev::GetStatus(cmd_bx[ix]);
                    if (sens)
                        break;
                }
            }
        }
        cmd_bx[i]->setEnabled(!set || sens);
    }

    if (cmd_excmd->has_depth()) {
        //
        // depth option menu
        //
        hbox = new QHBoxLayout();
        hbox->setContentsMargins(qm);
        hbox->setSpacing(2);
        vbox->addLayout(hbox);

        label = new QLabel(tr("Depth to process"));
        hbox->addWidget(label);

        cmd_depth = new QComboBox();
        hbox->addWidget(cmd_depth);
        if (dep < 0)
            dep = 0;
        if (dep > DMAX)
            dep = DMAX;
        for (int i = 0; i <= DMAX; i++) {
            char buf[16];
            if (i == DMAX)
                strcpy(buf, "all");
            else
                snprintf(buf, sizeof(buf), "%d", i);
            cmd_depth->addItem(buf);
        }
        cmd_depth->setCurrentIndex(dep);
        connect(cmd_depth, SIGNAL(currentIndexChanged(int)),
            this, SLOT(depth_changed_slot(int)));
    }

    if (cmd_excmd->message()) {
        //
        // label in frame
        //
        gb = new QGroupBox();
        vbox->addWidget(gb);
        hb = new QHBoxLayout(gb);
        hb->setContentsMargins(qm);
        hb->setSpacing(2);

        cmd_label = new QLabel(tr(cmd_excmd->message()));
        cmd_label->setAlignment(Qt::AlignCenter);
        hb->addWidget(cmd_label);

        cmd_text = new QLineEdit();
        vbox->addWidget(cmd_text);

        if (cmd_excmd->filename())
            cmd_text->setText(cmd_excmd->filename());
        cmd_text->setReadOnly(false);
        cmd_text->setAcceptDrops(true);
    }

    // activate button
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    cmd_go = new QToolButton();
    cmd_go->setText(tr(cmd_excmd->gotext()));
    hbox->addWidget(cmd_go);
    cmd_go->setCheckable(true);
    connect(cmd_go, SIGNAL(toggled(bool)), this, SLOT(go_btn_slot(bool)));

    QPushButton *btn = new QPushButton(tr("Cancel"));
    btn->setObjectName("Dismiss");
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(cancel_btn_slot()));
}


QTextCmdDlg::~QTextCmdDlg()
{
    instPtr = 0;
    if (cmd_caller)
        QTdev::Deselect(cmd_caller);
    // call action, passing 0 on popdown
    if (cmd_action)
        (*cmd_action)(0, cmd_arg, false, 0, 0, 0);
    delete [] cmd_bx;
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTextCmdDlg
#include "qtinterf/qtmacos_event.h"
#endif


void
QTextCmdDlg::update()
{
    for (int i = 0; i < cmd_excmd->num_buttons(); i++) {
        if (cmd_excmd->button(i)->var()) {
            bool bstate = cmd_bx[i]->isChecked();
            bool vstate = CDvdb()->getVariable(cmd_excmd->button(i)->var());
            if (lstring::ciprefix("no", cmd_excmd->button(i)->var())) {
                if (bstate == vstate)
                    cmd_bx[i]->setChecked(!vstate);;
            }
            else if (bstate != vstate)
                cmd_bx[i]->setChecked(vstate);;
        }
    }
}


void
QTextCmdDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp(cmd_helpkw))
}


void
QTextCmdDlg::go_btn_slot(bool)
{
}


void
QTextCmdDlg::check_state_changed_slot(int)
{
    if (sender() == cmd_go && !QTdev::GetStatus(sender()))
        return;
    bool down = false;
    if (!cmd_action)
        down = true;
    else {
        for (int i = 0; i < cmd_excmd->num_buttons(); i++) {
            bool sens = false;
            bool set = false;
            for (int j = 0; j < EXT_BSENS; j++) {
                unsigned int ix = cmd_excmd->button(i)->sens()[j];
                if (ix) {
                    ix--;
                    if ((int)ix < cmd_excmd->num_buttons()) {
                        set = true;
                        sens = cmd_bx[ix]->isChecked();
                        if (sens)
                            break;
                    }
                }
            }
            cmd_bx[i]->setEnabled(!set || sens);
        }
        QByteArray ba;
        int xx = 0, yy = 0;
        if (cmd_text && sender() == cmd_go) {
            ba = cmd_text->text().toLatin1();
            QPoint pn = pos();
            xx = pn.x();
            yy = pn.y();

        }

        // Hide the widget during computation.  If the action returns true,
        // we don't pop down, so make the widget visible again if it still
        // exists.  The widget may have been destroyed.

        if (sender() == cmd_go)
            hide();
        if (!cmd_action ||
                !(*cmd_action)(QTdev::GetLabel(sender()),
                cmd_arg, QTdev::GetStatus(sender()), ba.constData(), xx, yy))
            down = true;
        else
            show();
        if (sender() == cmd_go)
            QTdev::Deselect(sender());
    }
    if (down)
        EX()->PopUpExtCmd(0, MODE_OFF, 0, 0, 0);
}


void
QTextCmdDlg::depth_changed_slot(int)
{
    if (cmd_action) {
        QByteArray dp_ba = cmd_depth->currentText().toLatin1();
        const char *t = dp_ba.constData();
        (*cmd_action)("depth", cmd_arg, true, t, 0, 0);
    }
}


void
QTextCmdDlg::cancel_btn_slot()
{
    EX()->PopUpExtCmd(0, MODE_OFF, 0, 0, 0);
}

