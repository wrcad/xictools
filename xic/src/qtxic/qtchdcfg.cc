
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

#include "qtchdcfg.h"
#include "cvrt.h"
#include "errorlog.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_chd.h"
#include "fio_cgd.h"
#include "cd_digest.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>


//-----------------------------------------------------------------------------
// QTchdCfgDlg:  Dialog to configure a cell hierarchy digest (CHD).
// Called from the Cell Hierarchy Digests listing dialog (QTchdListDlg).
//
// The CHD can be configured with
// 1.  a default top cell.
// 2.  an associated geometry database.
//
// When a configured CHD is used for access, it will use only cells in
// the hierarchy under the top cell that are needed to render the area.
// The original file is accessed, unless the in-core geometry database
// is loaded.
//
// Help system keywords used:
//  xic:chdconfig

void
cConvert::PopUpChdConfig(GRobject caller, ShowMode mode,
    const char *chdname, int x, int y)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTchdCfgDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTchdCfgDlg::self())
            QTchdCfgDlg::self()->update(chdname);
        return;
    }
    if (QTchdCfgDlg::self())
        return;

    new QTchdCfgDlg(caller, chdname);

    QTchdCfgDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_XYA, x, y),
        QTchdCfgDlg::self(), QTmainwin::self()->Viewport());
    QTchdCfgDlg::self()->show();
}
// End of cConvert functions.


class QTchdCfgCellEdit : public QLineEdit
{
public:
    QTchdCfgCellEdit(QWidget *prnt = 0) : QLineEdit(prnt) { }

    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);
};


void
QTchdCfgCellEdit::dragEnterEvent(QDragEnterEvent *ev)
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
QTchdCfgCellEdit::dropEvent(QDropEvent *ev)
{
    if (ev->mimeData()->hasUrls()) {
        QByteArray ba = ev->mimeData()->data("text/plain");
        const char *str = ba.constData();
        str = lstring::strip_path(str);
        setText(str);
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/twostring")) {
        // Drops from content lists may be in the form
        // "fname_or_chd\ncellname".  Keep the cellname.
        char *str = lstring::copy(
            ev->mimeData()->data("text/plain").constData());
        const char *t = strchr(str, '\n');
        if (t)
            t = t+1;
        else
            t = str;
        setText(t);
        delete [] str;
        ev->accept();
        return;
    }
    if (ev->mimeData()->hasFormat("text/plain")) {
        // The default action will insert the text at the click location,
        // instead here we replace any existing text.
        QByteArray ba = ev->mimeData()->data("text/plain");
        const char *str = lstring::strip_path(ba.constData());
        setText(str);
        ev->accept();
        return;
    }
    ev->ignore();
}


QTchdCfgDlg *QTchdCfgDlg::instPtr;

QTchdCfgDlg::QTchdCfgDlg(GRobject caller, const char *chdname) : QTbag(this)
{
    instPtr = this;
    cf_caller = caller;
    cf_label = 0;
    cf_dtc_label = 0;
    cf_last = 0;
    cf_text = 0;
    cf_apply_tc = 0;
    cf_newcgd = 0;
    cf_cgdentry = 0;
    cf_cgdlabel = 0;
    cf_apply_cgd = 0;
    cf_chdname = 0;
    cf_lastname = 0;
    cf_cgdname = 0;

    setWindowTitle(tr("Configure Cell Hierarchy Digest"));
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

    // label in frame plus help btn
    //
    QGroupBox *gb = new QGroupBox();
    hbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qm);
    hb->setSpacing(2);
    cf_label = new QLabel("");
    hb->addWidget(cf_label);

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // Frame and name group.
    //
    gb = new QGroupBox();
    vbox->addWidget(gb);

    QVBoxLayout *gvbox = new QVBoxLayout(gb);
    gvbox->setContentsMargins(qmtop);
    gvbox->setSpacing(2);

    QHBoxLayout *ghbox = new QHBoxLayout();
    gvbox->addLayout(ghbox);
    ghbox->setContentsMargins(qm);
    ghbox->setSpacing(2);

    cf_apply_tc = new QPushButton("");
    ghbox->addWidget(cf_apply_tc);
    cf_apply_tc->setAutoDefault(false);
    connect(cf_apply_tc, SIGNAL(clicked()), this, SLOT(apply_tc_btn_slot()));

    QLabel *label = new QLabel(tr("Set Default Cell"));
    ghbox->addWidget(label);

    // Name group controls.
    //
    ghbox = new QHBoxLayout();
    gvbox->addLayout(ghbox);
    ghbox->setContentsMargins(qm);
    ghbox->setSpacing(2);

    cf_dtc_label = new QLabel(tr("Default top cell"));
    ghbox->addWidget(cf_dtc_label);

    cf_last = new QPushButton(tr("Last"));
    ghbox->addWidget(cf_last);
    cf_last->setAutoDefault(false);
    connect(cf_last, SIGNAL(clicked()), this, SLOT(last_btn_slot()));

    cf_text = new QLineEdit();
    cf_text->setReadOnly(false);
    cf_text->setAcceptDrops(true);
    ghbox->addWidget(cf_text);
    // End of name group.

    // Frame and CGD group.
    //
    gb = new QGroupBox();
    vbox->addWidget(gb);

    gvbox = new QVBoxLayout(gb);
    gvbox->setContentsMargins(qmtop);
    gvbox->setSpacing(2);

    ghbox = new QHBoxLayout();
    gvbox->addLayout(ghbox);
    ghbox->setContentsMargins(qm);
    ghbox->setSpacing(2);

    cf_apply_cgd = new QPushButton("");
    ghbox->addWidget(cf_apply_cgd);
    cf_apply_cgd->setAutoDefault(false);
    connect(cf_apply_cgd, SIGNAL(clicked()),
        this, SLOT(apply_cgd_btn_slot()));

    label = new QLabel(tr("Setup Linked Cell Geometry Digest"));
    ghbox->addWidget(label);

    cf_newcgd = new QCheckBox(tr("Open new CGD"));
    gvbox->addWidget(cf_newcgd);
    connect(cf_newcgd, SIGNAL(stateChanged(int)),
        this, SLOT(new_cgd_btn_slot(int)));

    ghbox = new QHBoxLayout();
    gvbox->addLayout(ghbox);
    ghbox->setContentsMargins(qm);
    ghbox->setSpacing(2);

    cf_cgdlabel = new QLabel(tr("CGD name"));
    ghbox->addWidget(label);

    cf_cgdentry = new QLineEdit();
    ghbox->addWidget(cf_cgdentry);
    cf_cgdentry->setReadOnly(false);
    //
    // End of CGD group.

    // Dismiss button
    //
    btn = new QPushButton(tr("Dismiss"));
    vbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    update(chdname);
}


QTchdCfgDlg::~QTchdCfgDlg()
{
    instPtr = 0;
    delete [] cf_lastname;
    delete [] cf_chdname;
    delete [] cf_cgdname;
    if (cf_caller)
        QTdev::Deselect(cf_caller);
}


void
QTchdCfgDlg::update(const char *chdname)
{
    if (!chdname)
        return;
    if (chdname != cf_chdname) {
        delete [] cf_chdname;
        cf_chdname = lstring::copy(chdname);
    }
    cCHD *chd = CDchd()->chdRecall(chdname, false);
    if (chd) {
        cf_text->setText(chd->defaultCell(Physical));
        bool has_name = (chd->getConfigSymref() != 0);

        cf_dtc_label->setEnabled(!has_name);
        cf_last->setEnabled(!has_name);
        cf_text->setEnabled(!has_name);

        const char *cgdname = chd->getCgdName();
        if (cgdname) {
            cf_cgdentry->setText(cgdname);
            cf_cgdentry->setReadOnly(true);
            QTdev::SetStatus(cf_newcgd, false);
        }
        else {
            cf_cgdentry->setText(cf_cgdname ? cf_cgdname : "");
            cf_cgdentry->setReadOnly(false);
        }

        cf_newcgd->setEnabled(!chd->hasCgd());
        cf_cgdentry->setEnabled(!chd->hasCgd());
        cf_cgdlabel->setEnabled(!chd->hasCgd());

        char buf[256];
        if (has_name || chd->hasCgd()) {
            snprintf(buf, sizeof(buf), "CHD %s is configured with ", chdname);
            int xx = 0;
            if (has_name) {
                strcat(buf, "Cell");
                xx++;
            }
            if (chd->hasCgd()) {
                if (xx)
                    strcat(buf, ", Geometry");
                else
                    strcat(buf, "Geometry");
            }
            strcat(buf, ".");
        }
        else
            snprintf(buf, sizeof(buf), "CHD %s is not configured.", chdname);
        cf_label->setText(buf);

        cf_apply_tc->setText(has_name ? "Clear" : "Apply");
        cf_apply_cgd->setText(chd->hasCgd() ? "Clear" : "Apply");
    }
    cf_apply_tc->setEnabled(chd != 0);
    cf_apply_cgd->setEnabled(chd != 0);
}


// Static function.
// Callback for the Open CGD panel.
//
bool
QTchdCfgDlg::cf_new_cgd_cb(const char *idname, const char *string, int mode,
    void *arg)
{
    if (!idname || !*idname)
        return (false);
    if (!string || !*string)
        return (false);
    CgdType tp = CGDremote;
    if (mode == 0)
        tp = CGDmemory;
    else if (mode == 1)
        tp = CGDfile;
    cCGD *cgd = FIO()->NewCGD(idname, string, tp);
    if (!cgd) {
        const char *fmt = "Failed to create new Geometry Digest:\n%s";
        if (QTchdCfgDlg::self()) {
            const char *s = Errs()->get_error();
            int len = strlen(fmt) + (s ? strlen(s) : 0) + 10;
            char *t = new char[len];
            snprintf(t, len, fmt, s);
            QTchdCfgDlg::self()->PopUpMessage(t, true);
            delete [] t;
        }
        else
            Log()->ErrorLogV(mh::Processing,
                "Failed to create new Geometry Digest:\n%s",
                Errs()->get_error());
        return (false);
    }
    // Link the new CHD, and set the flag to delete the CGD when
    // unlinked.
    cCHD *chd = (cCHD*)arg;
    if (chd) {
        cgd->set_free_on_unlink(true);
        chd->setCgd(cgd);
        if (QTchdCfgDlg::self())
            QTchdCfgDlg::self()->update(QTchdCfgDlg::self()->cf_chdname);
    }
    return (true);
}


void
QTchdCfgDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:chdconfig"))
}


void
QTchdCfgDlg::apply_tc_btn_slot()
{
    // NOTE:  cCHD::setDefaultCellname calls back to the update
    // function, so we don't call it here.  Have to be careful to
    // not revert cf_text before we get the new name.

    if (!cf_chdname)
        return;
    cCHD *chd = CDchd()->chdRecall(cf_chdname, false);
    if (!chd) {
        PopUpMessage("Error: can't find named CHD.", true);
        return;
    }

    bool iscfg = chd->getConfigSymref() != 0;
    if (iscfg) {
        // Save the current configuration cellname, for the Last button.
        delete [] cf_lastname;
        cf_lastname = lstring::copy(cf_text->text().toLatin1().constData());
        chd->setDefaultCellname(0, 0);
        update(cf_chdname);
        return;
    }
    const char *ent = lstring::copy(cf_text->text().toLatin1().constData());
    if (ent && *ent) {
        if (!chd->findSymref(ent, Physical)) {
            PopUpMessage("Error: can't find named cell in CHD.", true);
            delete [] ent;
            return;
        }
    }
    const char *dfl = chd->defaultCell(Physical);
    if (ent && dfl && strcmp(ent, dfl)) {
        if (!chd->setDefaultCellname(ent, 0)) {
            Errs()->add_error("Call to configure failed.");
            PopUpMessage(Errs()->get_error(), true);
            delete [] ent;
            return;
        }
    }
    delete [] ent;
}


void
QTchdCfgDlg::last_btn_slot()
{
    if (!cf_chdname)
        return;
    cCHD *chd = CDchd()->chdRecall(cf_chdname, false);
    if (!chd) {
        PopUpMessage("Error: can't find named CHD.", true);
        return;
    }

    char *ent = lstring::copy(cf_text->text().toLatin1().constData());
    cf_text->setText(cf_lastname ? cf_lastname : chd->defaultCell(Physical));
    delete [] cf_lastname;
    cf_lastname = ent;
}


void
QTchdCfgDlg::apply_cgd_btn_slot()
{
    if (!cf_chdname)
        return;
    cCHD *chd = CDchd()->chdRecall(cf_chdname, false);
    if (!chd) {
        PopUpMessage("Error: can't find named CHD.", true);
        return;
    }

    bool iscfg = chd->hasCgd();
    chd->setCgd(0);
    if (iscfg) {
        // clearing only
        update(cf_chdname);
        return;
    }
    delete [] cf_cgdname;
    cf_cgdname = 
        lstring::copy(cf_cgdentry->text().toLatin1().constData());
    cCGD *cgd = CDcgd()->cgdRecall(cf_cgdname, false);
    if (!cgd) {
        if (QTdev::GetStatus(cf_newcgd)) {
            QPoint pg = mapToGlobal(QPoint(0, 0));
            char *cn;
            if (cf_cgdname && *cf_cgdname)
                cn = lstring::copy(cf_cgdname);
            else
                cn = CDcgd()->newCgdName();
            // Pop down first, panel used elsewhere.
            Cvt()->PopUpCgdOpen(0, MODE_OFF, 0, 0, 0, 0, 0, 0);
            Cvt()->PopUpCgdOpen(0, MODE_ON, cn, chd->filename(),
                pg.x(), pg.y(), cf_new_cgd_cb, chd);
            delete [] cn;
        }
        else {
            char buf[256];
            if (!cf_cgdname || !*cf_cgdname)
                strcpy(buf, "No CGD access name given.");
            else {
                snprintf(buf, sizeof(buf), "No CGD with access name %s "
                    "currently exists.", cf_cgdname);
            }
            PopUpMessage(buf, false);
        }
    }
    else
        chd->setCgd(cgd);
    update(cf_chdname);
}


void
QTchdCfgDlg::new_cgd_btn_slot(int)
{
}


void
QTchdCfgDlg::dismiss_btn_slot()
{
    Cvt()->PopUpChdConfig(0, MODE_OFF, 0, 0, 0);
}

