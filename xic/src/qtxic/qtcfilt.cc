
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

#include "qtcfilt.h"
#include "dsp_inlines.h"
#include "errorlog.h"
#include "cfilter.h"

#include <QApplication>
#include <QLayout>
#include <QToolButton>
#include <QPushButton>
#include <QMenu>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>


//-----------------------------------------------------------------------------
// QTcfiltDlg:  Panel for setting up cell name filtering for the Cells
// Listing panel (QTcellsDlg).
//
// Help keyword:
// xic:cfilt

void
cMain::PopUpCellFilt(GRobject caller, ShowMode mode, DisplayMode dm,
    void(*cb)(cfilter_t*, void*), void *arg)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTcfiltDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTcfiltDlg::self())
            QTcfiltDlg::self()->update(dm);
        return;
    }
    if (QTcfiltDlg::self())
        return;

    new QTcfiltDlg(caller, dm, cb, arg);

    QTcfiltDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTcfiltDlg::self(),
        QTmainwin::self()->Viewport());
    QTcfiltDlg::self()->show();
}
// End of cMain functions.


char *QTcfiltDlg::cf_phys_regs[NUMREGS];
char *QTcfiltDlg::cf_elec_regs[NUMREGS];
QTcfiltDlg *QTcfiltDlg::instPtr;

QTcfiltDlg::QTcfiltDlg(GRobject c, DisplayMode dm, void(*cb)(cfilter_t*, void*),
    void *arg)
{
    instPtr = this;
    cf_caller = c;
    cf_mode = dm;
    cf_cb = cb;
    cf_arg = arg;

    cf_nimm = 0;
    cf_imm = 0;
    cf_nvsm = 0;
    cf_vsm = 0;
    cf_nlib = 0;
    cf_lib = 0;
    cf_npsm = 0;
    cf_psm = 0;
    cf_ndev = 0;
    cf_dev = 0;
    cf_nspr = 0;
    cf_spr = 0;
    cf_ntop = 0;
    cf_top = 0;
    cf_nmod = 0;
    cf_mod = 0;
    cf_nalt = 0;
    cf_alt = 0;
    cf_nref = 0;
    cf_ref = 0;
    cf_npcl = 0;
    cf_pcl = 0;
    cf_pclent = 0;
    cf_nscl = 0;
    cf_scl = 0;
    cf_sclent = 0;
    cf_nlyr = 0;
    cf_lyr = 0;
    cf_lyrent = 0;
    cf_nflg = 0;
    cf_flg = 0;
    cf_flgent = 0;
    cf_nftp = 0;
    cf_ftp = 0;
    cf_ftpent = 0;
    cf_apply = 0;

    setWindowTitle(tr("Cell List Filter"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // store/recall menus, label in frame plus help btn
    //
    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Store"));
    hbox->addWidget(tbtn);
    {
        char buf[16];
        QMenu *menu = new QMenu;
        for (int i = 1; i < NUMREGS; i++) {
            snprintf(buf, sizeof(buf), "reg%d", i);
            QAction *a = menu->addAction(buf);
            a->setData(i);
        }
        tbtn->setMenu(menu);
        tbtn->setPopupMode(QToolButton::InstantPopup);
        connect(menu, SIGNAL(triggered(QAction*)),
            this, SLOT(store_menu_slot(QAction*)));
    }
    tbtn = new QToolButton();
    tbtn->setText(tr("Recall"));
    hbox->addWidget(tbtn);
    {
        char buf[16];
        QMenu *menu = new QMenu;
        for (int i = 1; i < NUMREGS; i++) {
            snprintf(buf, sizeof(buf), "reg%d", i);
            QAction *a = menu->addAction(buf);
            a->setData(i);
        }
        tbtn->setMenu(menu);
        tbtn->setPopupMode(QToolButton::InstantPopup);
        connect(menu, SIGNAL(triggered(QAction*)),
            this, SLOT(recall_menu_slot(QAction*)));
    }

    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    QLabel *label = new QLabel(tr("Set filtering for cells list"));
    hb->addWidget(label);

    tbtn = new QToolButton();
    tbtn->setText(tr("Help"));
    hbox->addWidget(tbtn);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // Two colums
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QVBoxLayout *col1 = new QVBoxLayout();
    col1->setContentsMargins(qm);
    col1->setSpacing(2);
    hbox->addLayout(col1);

    QVBoxLayout *col2 = new QVBoxLayout();
    col2->setContentsMargins(qm);
    col2->setSpacing(2);
    hbox->addLayout(col2);

    QVBoxLayout *col3 = new QVBoxLayout();
    col3->setContentsMargins(qm);
    col3->setSpacing(2);
    hbox->addLayout(col3);

    hb = new QHBoxLayout();
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    col3->addLayout(hb);

    QVBoxLayout *col3a = new QVBoxLayout();
    col3->setContentsMargins(qm);
    col3->setSpacing(2);
    hb->addLayout(col3a);

    QVBoxLayout *col3b = new QVBoxLayout();
    col3->setContentsMargins(qm);
    col3->setSpacing(2);
    hb->addLayout(col3b);

    // Immutable
    cf_nimm = new QCheckBox(tr("not"));
    col1->addWidget(cf_nimm);
    connect(cf_nimm, SIGNAL(stateChanged(int)),
        this, SLOT(nimm_btn_slot(int)));
    cf_imm = new QCheckBox(tr("Immutable"));
    col2->addWidget(cf_imm);
    connect(cf_imm, SIGNAL(stateChanged(int)),
        this, SLOT(imm_btn_slot(int)));

    // Via sub-master
    cf_nvsm = new QCheckBox(tr("not"));
    col3a->addWidget(cf_nvsm);
    connect(cf_nvsm, SIGNAL(stateChanged(int)),
        this, SLOT(nvsm_btn_slot(int)));
    cf_vsm = new QCheckBox(tr("Via sub-master"));
    col3b->addWidget(cf_vsm);
    connect(cf_vsm, SIGNAL(stateChanged(int)),
        this, SLOT(vsm_btn_slot(int)));

    // Library
    cf_nlib = new QCheckBox(tr("not"));
    col1->addWidget(cf_nlib);
    connect(cf_nlib, SIGNAL(stateChanged(int)),
        this, SLOT(nlib_btn_slot(int)));
    cf_lib = new QCheckBox(tr("Library"));
    col2->addWidget(cf_lib);
    connect(cf_lib, SIGNAL(stateChanged(int)),
        this, SLOT(lib_btn_slot(int)));

    // PCell sub-master
    cf_npsm = new QCheckBox(tr("not"));
    col3a->addWidget(cf_npsm);
    connect(cf_npsm, SIGNAL(stateChanged(int)),
        this, SLOT(npsm_btn_slot(int)));
    cf_psm = new QCheckBox(tr("PCell sub-master"));
    col3b->addWidget(cf_psm);
    connect(cf_psm, SIGNAL(stateChanged(int)),
        this, SLOT(psm_btn_slot(int)));

    // Device
    cf_ndev = new QCheckBox(tr("not"));
    col1->addWidget(cf_ndev);
    connect(cf_ndev, SIGNAL(stateChanged(int)),
        this, SLOT(ndev_btn_slot(int)));
    cf_dev = new QCheckBox(tr("Device"));
    col2->addWidget(cf_dev);
    connect(cf_dev, SIGNAL(stateChanged(int)),
        this, SLOT(dev_btn_slot(int)));

    // PCell super-master
    cf_nspr = new QCheckBox(tr("not"));
    col3a->addWidget(cf_nspr);
    connect(cf_nspr, SIGNAL(stateChanged(int)),
        this, SLOT(nspr_btn_slot(int)));
    cf_spr = new QCheckBox(tr("PCell super"));
    col3b->addWidget(cf_spr);
    connect(cf_spr, SIGNAL(stateChanged(int)),
        this, SLOT(spr_btn_slot(int)));

    // Top level
    cf_ntop = new QCheckBox(tr("not"));
    col1->addWidget(cf_ntop);
    connect(cf_ntop, SIGNAL(stateChanged(int)),
        this, SLOT(ntop_btn_slot(int)));
    cf_top = new QCheckBox(tr("Top level"));
    col2->addWidget(cf_top);
    connect(cf_top, SIGNAL(stateChanged(int)),
        this, SLOT(top_btn_slot(int)));

    // Modified
    cf_nmod = new QCheckBox(tr("not"));
    col3a->addWidget(cf_nmod);
    connect(cf_nmod, SIGNAL(stateChanged(int)),
        this, SLOT(nmod_btn_slot(int)));
    cf_mod = new QCheckBox(tr("Modified"));
    col3b->addWidget(cf_mod);
    connect(cf_mod, SIGNAL(stateChanged(int)),
        this, SLOT(mod_btn_slot(int)));

    // With alt
    cf_nalt = new QCheckBox(tr("not"));
    col1->addWidget(cf_nalt);
    connect(cf_nalt, SIGNAL(stateChanged(int)),
        this, SLOT(nalt_btn_slot(int)));
    cf_alt = new QCheckBox(tr("With alt"));
    col2->addWidget(cf_alt);
    connect(cf_alt, SIGNAL(stateChanged(int)),
        this, SLOT(alt_btn_slot(int)));

    cf_nref = new QCheckBox(tr("not"));
    col3a->addWidget(cf_nref);
    connect(cf_nref, SIGNAL(stateChanged(int)),
        this, SLOT(nref_btn_slot(int)));
    cf_ref = new QCheckBox(tr("Reference"));
    col3b->addWidget(cf_ref);
    connect(cf_ref, SIGNAL(stateChanged(int)),
        this, SLOT(ref_btn_slot(int)));

    // Parent cells
    cf_npcl = new QCheckBox(tr("not"));
    col1->addWidget(cf_npcl);
    connect(cf_npcl, SIGNAL(stateChanged(int)),
        this, SLOT(npcl_btn_slot(int)));
    cf_pcl = new QCheckBox(tr("Parent cells"));
    col2->addWidget(cf_pcl);
    connect(cf_pcl, SIGNAL(stateChanged(int)),
        this, SLOT(pcl_btn_slot(int)));

    cf_pclent = new QLineEdit();
    col3->addWidget(cf_pclent);

    // Subcell cells
    cf_nscl = new QCheckBox(tr("not"));
    col1->addWidget(cf_nscl);
    connect(cf_nscl, SIGNAL(stateChanged(int)),
        this, SLOT(nscl_btn_slot(int)));
    cf_scl = new QCheckBox(tr("Subcell cells"));
    col2->addWidget(cf_scl);
    connect(cf_scl, SIGNAL(stateChanged(int)),
        this, SLOT(scl_btn_slot(int)));

    cf_sclent = new QLineEdit();
    col3->addWidget(cf_sclent);

    // With layers
    cf_nlyr = new QCheckBox(tr("not"));
    col1->addWidget(cf_nlyr);
    connect(cf_nlyr, SIGNAL(stateChanged(int)),
        this, SLOT(nlyr_btn_slot(int)));
    cf_lyr = new QCheckBox(tr("With layers"));
    col2->addWidget(cf_lyr);
    connect(cf_lyr, SIGNAL(stateChanged(int)),
        this, SLOT(lyr_btn_slot(int)));

    cf_lyrent = new QLineEdit();
    col3->addWidget(cf_lyrent);

    // With flags
    cf_nflg = new QCheckBox(tr("not"));
    col1->addWidget(cf_nflg);
    connect(cf_nflg, SIGNAL(stateChanged(int)),
        this, SLOT(nflg_btn_slot(int)));
    cf_flg = new QCheckBox(tr("With flags"));
    col2->addWidget(cf_flg);
    connect(cf_flg, SIGNAL(stateChanged(int)),
        this, SLOT(flg_btn_slot(int)));

    cf_flgent = new QLineEdit();
    col3->addWidget(cf_flgent);

    // From filetypes
    cf_nftp = new QCheckBox(tr("not"));
    col1->addWidget(cf_nftp);
    connect(cf_nftp, SIGNAL(stateChanged(int)),
        this, SLOT(nftp_btn_slot(int)));
    cf_ftp = new QCheckBox(tr("From filetypes"));
    col2->addWidget(cf_ftp);
    connect(cf_ftp, SIGNAL(stateChanged(int)),
        this, SLOT(ftp_btn_slot(int)));

    cf_ftpent = new QLineEdit();
    col3->addWidget(cf_ftpent);

    // Apply and Dismiss buttons
    //
    hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    cf_apply = new QToolButton();
    cf_apply->setText(tr("Apply"));
    hbox->addWidget(cf_apply);
    connect(cf_apply, SIGNAL(clicked()), this, SLOT(apply_btn_slot()));

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    btn->setObjectName("Dismiss");
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update(cf_mode);
}


QTcfiltDlg::~QTcfiltDlg()
{
    instPtr = 0;
    if (cf_caller)
        QTdev::Deselect(cf_caller);
}


#ifdef Q_OS_MACOS
#define DLGTYPE QTcfiltDlg
#include "qtinterf/qtmacos_event.h"
#endif


// This is called when the listing mode changes in the Cells Listing
// panel, before the actual list is generated.
//
void
QTcfiltDlg::update(DisplayMode mode)
{
    cf_mode = mode;
    cfilter_t *cf;
    if (mode == Physical)
        cf = cfilter_t::parse(cf_phys_regs[0], Physical, 0);
    else
        cf = cfilter_t::parse(cf_elec_regs[0], Electrical, 0);
    setup(cf);
    if (cf_cb)
        (*cf_cb)(cf, cf_arg);  // Receiver takes ownership of cf.
}


void
QTcfiltDlg::setup(const cfilter_t *cf)
{
    if (!cf)
        return;
    // We know that for the flags, at most one of NOTX, X is set. 
    // This is not the case for the list forms, however this simple
    // panel can handle only one of each polarity.  Check these and
    // throw out any where both bits are set.

    unsigned int f = cf->flags();
    if ((f & CF_NOTPARENT) && (f & CF_PARENT))
        f &= ~(CF_NOTPARENT | CF_PARENT);
    if ((f & CF_NOTSUBCELL) && (f & CF_SUBCELL))
        f &= ~(CF_NOTSUBCELL | CF_SUBCELL);
    if ((f & CF_NOTLAYER) && (f & CF_LAYER))
        f &= ~(CF_NOTLAYER | CF_LAYER);
    if ((f & CF_NOTFLAG) && (f & CF_FLAG))
        f &= ~(CF_NOTFLAG | CF_FLAG);
    if ((f & CF_NOTFTYPE) && (f & CF_FTYPE))
        f &= ~(CF_NOTFTYPE | CF_FTYPE);

    QTdev::SetStatus(cf_nimm, f & CF_NOTIMMUTABLE);
    QTdev::SetStatus(cf_imm,  f & CF_IMMUTABLE);
    QTdev::SetStatus(cf_nvsm, f & CF_NOTVIASUBM);
    QTdev::SetStatus(cf_vsm,  f & CF_VIASUBM);
    QTdev::SetStatus(cf_nlib, f & CF_NOTLIBRARY);
    QTdev::SetStatus(cf_lib,  f & CF_LIBRARY);
    QTdev::SetStatus(cf_npsm, f & CF_NOTPCELLSUBM);
    QTdev::SetStatus(cf_psm,  f & CF_PCELLSUBM);
    QTdev::SetStatus(cf_ndev, f & CF_NOTDEVICE);
    QTdev::SetStatus(cf_dev,  f & CF_DEVICE);
    QTdev::SetStatus(cf_nspr, f & CF_NOTPCELLSUPR);
    QTdev::SetStatus(cf_spr,  f & CF_PCELLSUPR);
    QTdev::SetStatus(cf_ntop, f & CF_NOTTOPLEV);
    QTdev::SetStatus(cf_top,  f & CF_TOPLEV);
    QTdev::SetStatus(cf_nmod, f & CF_NOTMODIFIED);
    QTdev::SetStatus(cf_mod,  f & CF_MODIFIED);
    QTdev::SetStatus(cf_nalt, f & CF_NOTWITHALT);
    QTdev::SetStatus(cf_alt,  f & CF_WITHALT);
    QTdev::SetStatus(cf_nref, f & CF_NOTREFERENCE);
    QTdev::SetStatus(cf_ref,  f & CF_REFERENCE);
    QTdev::SetStatus(cf_npcl, f & CF_NOTPARENT);
    QTdev::SetStatus(cf_pcl,  f & CF_PARENT);
    QTdev::SetStatus(cf_nscl, f & CF_NOTSUBCELL);
    QTdev::SetStatus(cf_scl,  f & CF_SUBCELL);
    QTdev::SetStatus(cf_nlyr, f & CF_NOTLAYER);
    QTdev::SetStatus(cf_lyr,  f & CF_LAYER);
    QTdev::SetStatus(cf_nflg, f & CF_NOTFLAG);
    QTdev::SetStatus(cf_flg,  f & CF_FLAG);
    QTdev::SetStatus(cf_nftp, f & CF_NOTFTYPE);
    QTdev::SetStatus(cf_ftp,  f & CF_FTYPE);

    char *s = 0;
    if (f & CF_NOTPARENT)
        s = cf->not_prnt_list();
    else if (f & CF_PARENT)
        s = cf->prnt_list();
    if (s) {
        cf_pclent->setText(s);
        delete [] s;
    }
    else
        cf_pclent->setText("");

    s = 0;
    if (f & CF_NOTSUBCELL)
        s = cf->not_subc_list();
    else if (f & CF_SUBCELL)
        s = cf->subc_list();
    if (s) {
        cf_sclent->setText(s);
        delete [] s;
    }
    else
        cf_sclent->setText("");

    s = 0;
    if (f & CF_NOTLAYER)
        s = cf->not_layer_list();
    else if (f & CF_LAYER)
        s = cf->layer_list();
    if (s) {
        cf_lyrent->setText(s);
        delete [] s;
    }
    else
        cf_lyrent->setText("");

    s = 0;
    if (f & CF_NOTFLAG)
        s = cf->not_flag_list();
    else if (f & CF_FLAG)
        s = cf->flag_list();
    if (s) {
        cf_flgent->setText(s);
        delete [] s;
    }
    else
        cf_flgent->setText("");

    s = 0;
    if (f & CF_NOTFTYPE)
        s = cf->not_ftype_list();
    else if (f & CF_FTYPE)
        s = cf->ftype_list();
    if (s) {
        cf_ftpent->setText(s);
        delete [] s;
    }
    else
        cf_ftpent->setText("");

    if (cf_mode == Physical) {
        cf_ndev->setEnabled(false);
        cf_dev->setEnabled(false);
        cf_nvsm->setEnabled(true);
        cf_vsm->setEnabled(true);
        cf_npsm->setEnabled(true);
        cf_psm->setEnabled(true);
        cf_nspr->setEnabled(true);
        cf_spr->setEnabled(true);
        cf_nref->setEnabled(true);
        cf_ref->setEnabled(true);
    }
    else {
        cf_nvsm->setEnabled(false);
        cf_vsm->setEnabled(false);
        cf_npsm->setEnabled(false);
        cf_psm->setEnabled(false);
        cf_nspr->setEnabled(false);
        cf_spr->setEnabled(false);
        cf_nref->setEnabled(false);
        cf_ref->setEnabled(false);
        cf_ndev->setEnabled(true);
        cf_dev->setEnabled(true);
    }
}


// Create a new filter based on the present panel content.
//
cfilter_t *
QTcfiltDlg::new_filter()
{
    cfilter_t *cf = new cfilter_t(cf_mode);
    unsigned int f = 0;
    if (QTdev::GetStatus(cf_imm))
        f |= CF_IMMUTABLE;
    else if (QTdev::GetStatus(cf_nimm))
        f |= CF_NOTIMMUTABLE;
    if (QTdev::GetStatus(cf_vsm))
        f |= CF_VIASUBM;
    else if (QTdev::GetStatus(cf_nvsm))
        f |= CF_NOTVIASUBM;
    if (QTdev::GetStatus(cf_lib))
        f |= CF_LIBRARY;
    else if (QTdev::GetStatus(cf_nlib))
        f |= CF_NOTLIBRARY;
    if (QTdev::GetStatus(cf_psm))
        f |= CF_PCELLSUBM;
    else if (QTdev::GetStatus(cf_npsm))
        f |= CF_NOTPCELLSUBM;
    if (QTdev::GetStatus(cf_dev))
        f |= CF_DEVICE;
    else if (QTdev::GetStatus(cf_ndev))
        f |= CF_NOTDEVICE;
    if (QTdev::GetStatus(cf_spr))
        f |= CF_PCELLSUPR;
    else if (QTdev::GetStatus(cf_nspr))
        f |= CF_NOTPCELLSUPR;
    if (QTdev::GetStatus(cf_top))
        f |= CF_TOPLEV;
    else if (QTdev::GetStatus(cf_ntop))
        f |= CF_NOTTOPLEV;
    if (QTdev::GetStatus(cf_mod))
        f |= CF_MODIFIED;
    else if (QTdev::GetStatus(cf_nmod))
        f |= CF_NOTMODIFIED;
    if (QTdev::GetStatus(cf_alt))
        f |= CF_WITHALT;
    else if (QTdev::GetStatus(cf_nalt))
        f |= CF_NOTWITHALT;
    if (QTdev::GetStatus(cf_ref))
        f |= CF_REFERENCE;
    else if (QTdev::GetStatus(cf_nref))
        f |= CF_NOTREFERENCE;

    if (QTdev::GetStatus(cf_pcl)) {
        const char *s = lstring::copy(
            cf_pclent->text().toLatin1().constData());
        if (QTdev::GetStatus(cf_npcl)) {
            f |= CF_NOTPARENT;
            cf->parse_parent(true, s, 0);
        }
        else {
            f |= CF_PARENT;
            cf->parse_parent(false, s, 0);
        }
        delete [] s;
    }
    if (QTdev::GetStatus(cf_scl)) {
        const char *s = lstring::copy(
           cf_sclent->text().toLatin1().constData());
        if (QTdev::GetStatus(cf_nscl)) {
            f |= CF_NOTSUBCELL;
            cf->parse_subcell(true, s, 0);
        }
        else {
            f |= CF_SUBCELL;
            cf->parse_subcell(false, s, 0);
        }
        delete [] s;
    }
    if (QTdev::GetStatus(cf_lyr)) {
        const char *s = lstring::copy(
            cf_lyrent->text().toLatin1().constData());
        if (QTdev::GetStatus(cf_nlyr)) {
            f |= CF_NOTLAYER;
            cf->parse_layers(true, s, 0);
        }
        else {
            f |= CF_LAYER;
            cf->parse_layers(false, s, 0);
        }
        delete [] s;
    }
    if (QTdev::GetStatus(cf_flg)) {
        const char *s = lstring::copy(
            cf_flgent->text().toLatin1().constData());
        if (QTdev::GetStatus(cf_nflg)) {
            f |= CF_NOTFLAG;
            cf->parse_flags(true, s, 0);
        }
        else {
            f |= CF_FLAG;
            cf->parse_flags(false, s, 0);
        }
        delete [] s;
    }
    if (QTdev::GetStatus(cf_ftp)) {
        const char *s = lstring::copy(
            cf_ftpent->text().toLatin1().constData());
        if (QTdev::GetStatus(cf_nftp)) {
            f |= CF_NOTFTYPE;
            cf->parse_ftypes(true, s, 0);
        }
        else {
            f |= CF_FTYPE;
            cf->parse_ftypes(false, s, 0);
        }
        delete [] s;
    }
    cf->set_flags(f);
    return (cf);
}


void
QTcfiltDlg::store_menu_slot(QAction *a)
{
    int ix = a->data().toInt();
    cfilter_t *cf = new_filter();
    if (!cf) {
        Log()->PopUpErrV("Failed: %s", Errs()->get_error());
        return;
    }
    if (cf_mode == Physical) {
        delete [] cf_phys_regs[ix];
        cf_phys_regs[ix] = cf->string();
    }
    else {
        delete [] cf_elec_regs[ix];
        cf_elec_regs[ix] = cf->string();
    }
    delete cf;
}


void
QTcfiltDlg::recall_menu_slot(QAction *a)
{
    int ix = a->data().toInt();
    cfilter_t *cf;
    if (cf_mode == Physical)
        cf = cfilter_t::parse(cf_phys_regs[ix], Physical, 0);
    else
        cf = cfilter_t::parse(cf_elec_regs[ix], Electrical, 0);
    setup(cf);
    delete cf;
}


void
QTcfiltDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:cfilt"))
}


void
QTcfiltDlg::nimm_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_imm, false);
}


void
QTcfiltDlg::imm_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_nimm, false);
}


void
QTcfiltDlg::nvsm_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_vsm, false);
}


void
QTcfiltDlg::vsm_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_nvsm, false);
}


void
QTcfiltDlg::nlib_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_lib, false);
}


void
QTcfiltDlg::lib_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_nlib, false);
}


void
QTcfiltDlg::npsm_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_psm, false);
}


void
QTcfiltDlg::psm_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_npsm, false);
}


void
QTcfiltDlg::ndev_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_dev, false);
}


void
QTcfiltDlg::dev_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_ndev, false);
}


void
QTcfiltDlg::nspr_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_spr, false);
}


void
QTcfiltDlg::spr_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_nspr, false);
}


void
QTcfiltDlg::ntop_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_top, false);
}


void
QTcfiltDlg::top_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_ntop, false);
}


void
QTcfiltDlg::nmod_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_mod, false);
}


void
QTcfiltDlg::mod_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_nmod, false);
}


void
QTcfiltDlg::nalt_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_alt, false);
}


void
QTcfiltDlg::alt_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_nalt, false);
}


void
QTcfiltDlg::nref_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_ref, false);
}


void
QTcfiltDlg::ref_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_nref, false);
}


void
QTcfiltDlg::npcl_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_pcl, false);
}


void
QTcfiltDlg::pcl_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_npcl, false);
}


void
QTcfiltDlg::nscl_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_scl, false);
}


void
QTcfiltDlg::scl_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_nscl, false);
}


void
QTcfiltDlg::nlyr_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_lyr, false);
}


void
QTcfiltDlg::lyr_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_nlyr, false);
}


void
QTcfiltDlg::nflg_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_flg, false);
}


void
QTcfiltDlg::flg_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_nflg, false);
}


void
QTcfiltDlg::nftp_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_ftp, false);
}


void
QTcfiltDlg::ftp_btn_slot(int state)
{
    if (state)
        QTdev::SetStatus(cf_nftp, false);
}


void
QTcfiltDlg::apply_btn_slot()
{
    if (!cf_cb)
        return;
    cfilter_t *cf = new_filter();
    if (!cf)
        return;
    (*cf_cb)(cf, cf_arg);  // Receiver takes ownership of cf.
}


void
QTcfiltDlg::dismiss_btn_slot()
{
    XM()->PopUpCellFilt(0, MODE_OFF, Physical, 0, 0);
}

