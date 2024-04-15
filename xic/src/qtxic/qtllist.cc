
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

#include "qtllist.h"
#include "main.h"
#include "cvrt.h"
#include "dsp.h"
#include "dsp_inlines.h"
#include "qtmain.h"
#include "qtltab.h"

#include <QLayout>
#include <QLabel>
#include <QCheckBox>
#include <QToolButton>
#include <QLineEdit>
#include <QMimeData>
#include <QDropEvent>


//-----------------------------------------------------------------------------
// QTlayerEdit:  Subwidget group for layer list.
// Used in Convert/Format Conversion and elsewhere.

void
QTlayerEdit::dragEnterEvent(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasFormat(QTltab::mime_type())) {
        ev->accept();
        return;
    }
    QLineEdit::dragEnterEvent(ev);
}


void
QTlayerEdit::dropEvent(QDropEvent *ev)
{
    if (ev->mimeData()->hasFormat(QTltab::mime_type())) {
        QByteArray ba = ev->mimeData()->data(QTltab::mime_type());
        LayerFillData lfd;
        void *d = (void*)&lfd;
        memcpy(d, ba.constData(), ba.size());
        if (lfd.d_from_layer) {
            ev->accept();
            int lnum = lfd.d_layernum;
            CDl *ld = CDldb()->layer(lnum, DSP()->CurMode());
            sLstr lstr;
            lstr.add(text().toLatin1().constData());
            if (lstr.string() && *lstr.string())
                lstr.add_c(' ');
            lstr.add(ld->name());
            setText(lstr.string());
            return;
        }
    }
    ev->ignore();
}
// End of QTlayerEdit definitions.


// The exported widget collection.
//
QTlayerList::QTlayerList()
{
    ll_luse = 0;
    ll_lskip = 0;
    ll_laylist = 0;
    ll_aluse = 0;
    ll_aledit = 0;

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QLabel *label = new QLabel(tr("Layer List (physical)  "));
    hbox->addWidget(label);

    ll_luse = new QCheckBox(tr("Layers only"));
    hbox->addWidget(ll_luse);
    connect(ll_luse, SIGNAL(stateChanged(int)),
        this, SLOT(luse_btn_slot(int)));

    ll_lskip = new QCheckBox(tr("Skip layers"));
    hbox->addWidget(ll_lskip);
    connect(ll_lskip, SIGNAL(stateChanged(int)),
        this, SLOT(lskip_btn_slot(int)));

    ll_laylist = new QTlayerEdit();
    vbox->addWidget(ll_laylist);
    ll_laylist->setReadOnly(false);

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    ll_aluse = new QCheckBox(tr("Use Layer Aliases"));
    hbox->addWidget(ll_aluse);
    connect(ll_aluse, SIGNAL(stateChanged(int)),
        this, SLOT(aluse_btn_slot(int)));

    ll_aledit = new QToolButton();
    ll_aledit->setText(tr("Edit Layer Aliases"));
    hbox->addWidget(ll_aledit);
    ll_aledit->setCheckable(true);
    connect(ll_aledit, SIGNAL(toggled(bool)),
        this, SLOT(aledit_btn_slot(bool)));

    update();

    connect(ll_laylist, SIGNAL(textChanged(const QString&)),
        this, SLOT(text_changed_slot(const QString&)));
}


QTlayerList::~QTlayerList()
{
    if (ll_aledit && QTdev::GetStatus(ll_aledit))
        QTdev::CallCallback(ll_aledit);
}


void
QTlayerList::update()
{
    const char *use = CDvdb()->getVariable(VA_UseLayerList);
    if (use) {
        if (*use == 'n' || *use == 'N') {
            QTdev::SetStatus(ll_lskip, true);
            QTdev::SetStatus(ll_luse, false);
        }
        else {
            QTdev::SetStatus(ll_lskip, false);
            QTdev::SetStatus(ll_luse, true);
        }
    }
    else {
        QTdev::SetStatus(ll_lskip, false);
        QTdev::SetStatus(ll_luse, false);
    }

    QString list(CDvdb()->getVariable(VA_LayerList));
    if (list != ll_laylist->text())
        ll_laylist->setText(list);

    if (QTdev::GetStatus(ll_luse) || QTdev::GetStatus(ll_lskip))
        ll_laylist->setEnabled(true);
    else
        ll_laylist->setEnabled(false);

    QTdev::SetStatus(ll_aluse, CDvdb()->getVariable(VA_UseLayerAlias));
}


void
QTlayerList::luse_btn_slot(int state)
{
    if (state) {
        CDvdb()->setVariable(VA_UseLayerList, 0);
        ll_laylist->setFocus();
    }
    else
        CDvdb()->clearVariable(VA_UseLayerList);
}


void
QTlayerList::lskip_btn_slot(int state)
{
    if (state) {
        CDvdb()->setVariable(VA_UseLayerList, "n");
        ll_laylist->setFocus();
    }
    else
        CDvdb()->clearVariable(VA_UseLayerList);
}


void
QTlayerList::aluse_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_UseLayerAlias, 0);
    else
        CDvdb()->clearVariable(VA_UseLayerAlias);
}


void
QTlayerList::aledit_btn_slot(bool state)
{
    if (state)
        XM()->PopUpLayerAliases(ll_aledit, MODE_ON);
    else
        XM()->PopUpLayerAliases(0, MODE_OFF);
}


void
QTlayerList::text_changed_slot(const QString &s)
{
    if (s.isNull() || s.isEmpty()) {
        CDvdb()->clearVariable(VA_LayerList);
        return;
    }
    const char *ss = CDvdb()->getVariable(VA_LayerList);
    if (!ss || (s != QString(ss))) {
        const char *t = lstring::copy(s.toLatin1().constData());
        CDvdb()->setVariable(VA_LayerList, t);
        delete [] t;
    }
}

