
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

#include "qtltedit.h"
#include "dsp_inlines.h"
#include "cd_strmdata.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>


//--------------------------------------------------------------------
// Layer Editor pop-up
//

// Pop up the Layer editor.  The editor has buttons to add a layer, remove
// layers, and a combo box for layer name entry.  The combo contains a
// list of previously removed layers.
//
sLcb *
cMain::PopUpLayerEditor(GRobject c)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return (0);
    QTltabEditDlg *cbs = new QTltabEditDlg(c);

    QTdev::self()->SetPopupLocation(GRloc(), cbs,
        QTmainwin::self()->Viewport());
    cbs->show();
    return (cbs);
}
// End of cMain functions.


const char *QTltabEditDlg::initmsg = "Layer Editor -- add or remove layers.";

QTltabEditDlg::QTltabEditDlg(GRobject c)
{
    le_caller = c;
    le_add = 0;
    le_rem = 0;
    le_opmenu = 0;
    le_label = 0;

    setWindowTitle(tr("Layer Editor"));
    setWindowFlags(Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_ShowWithoutActivating);
//    gtk_window_set_resizable(GTK_WINDOW(le_shell), false);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    // label in frame
    //
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    le_label = new QLabel(tr(initmsg));
    hb->addWidget(le_label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    vbox->addLayout(hbox);

    // combo box input area
    //
    le_opmenu = new QComboBox();
    le_opmenu->setEditable(true);
    le_opmenu->setInsertPolicy(QComboBox::NoInsert);
    hbox->addWidget(le_opmenu);

    // buttons
    //
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    btn = new QPushButton(tr("Add Layer"));
    hbox->addWidget(btn);
    btn->setCheckable(true);
    connect(btn, SIGNAL(toggled(bool)), this, SLOT(add_layer_slot(bool)));
    le_add = btn;

    btn = new QPushButton(tr("Remove Layer"));
    hbox->addWidget(btn);
    btn->setCheckable(true);
    connect(btn, SIGNAL(toggled(bool)), this, SLOT(rem_layer_slot(bool)));
    le_rem = btn;

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_slot()));

//    gtk_window_set_focus(GTK_WINDOW(le_shell), text);
}


QTltabEditDlg::~QTltabEditDlg()
{
    if (le_caller)
        QTdev::Deselect(le_caller);
    quit_cb();
}


// Update the list of removed layers
//
void
QTltabEditDlg::update(CDll *list)
{
    le_opmenu->clear();
    for (CDll *l = list; l; l = l->next) {
        le_opmenu->addItem(l->ldesc->name());
    }
    le_opmenu->setCurrentIndex(0);
    QLineEdit *text = le_opmenu->lineEdit();
    if (!list)
        text->setText("");
}


// Return the current name in the text box.  The return value should be
// freed.  If the value isn't good, 0 is returned.  This is called when
// adding layers.
//
char *
QTltabEditDlg::layername()
{
    QByteArray qba = le_opmenu->currentText().toLatin1();
    const char *text = (const char*)qba.constData();
    if (!text)
        return (0);
    char *string = le_get_lname();

    if (!string) {
        QTdev::Deselect(le_add);
        add_cb(false);
    }
    return (string);
}


// Deselect the Remove button.
//
void
QTltabEditDlg::desel_rem()
{
    QTdev::Deselect(le_rem);
}


// Pop down the widget (from the caller).
//
void
QTltabEditDlg::popdown()
{
    deleteLater();
}


// Return the text input, or 0 if no good.
//
char *
QTltabEditDlg::le_get_lname()
{
    QByteArray qba = le_opmenu->lineEdit()->text().toLatin1();;
    const char *string = (const char*)qba.constData();
    if (!string || !*string) {
        le_label->setText(tr("No name entered.  Enter a layer name:"));
        return (0);
    }
    char *lname = lstring::copy(string);
    if (CDldb()->findLayer(lname, DSP()->CurMode())) {
        char buf[256];
        snprintf(buf, sizeof(buf),
            "A layer %s already exists.  Enter a new name:", lname);
        le_label->setText(tr(buf));
        delete [] lname;
        return (0);
    }
    return (lname);
}


void
QTltabEditDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:edlyr"))
}


void
QTltabEditDlg::add_layer_slot(bool state)
{
    if (state) {
        le_label->setText(tr(initmsg));
        add_cb(false);
        return;
    }
    QTdev::Deselect(le_rem);
    char *string = le_get_lname();
    if (!string) {
        QTdev::Deselect(le_add);
        return;
    }
    delete [] string;
    le_label->setText(tr("Click where new layer is to be added."));
    add_cb(true);
}


void
QTltabEditDlg::rem_layer_slot(bool state)
{
    if (!state) {
        le_label->setText(tr(initmsg));
        rem_cb(false);
        return;
    }
    bool rmstate;
    if (DSP()->CurMode() == Electrical)
        rmstate = (CDldb()->layer(2, Electrical) != 0);
    else
        rmstate = (CDldb()->layer(1, Physical) != 0);
    if (!rmstate) {
        le_label->setText(tr("No removable layers left"));
        QTdev::Deselect(le_rem);
        return;
    }
    QTdev::Deselect(le_add);
    le_label->setText(tr("Click on layers to remove"));
    rem_cb(true);
}


void
QTltabEditDlg::dismiss_slot()
{
    popdown();
}

