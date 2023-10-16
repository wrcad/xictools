
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

#include "qtplace.h"
#include "dsp_inlines.h"
#include "promptline.h"
#include "select.h"
#include "pbtn_menu.h"
#include "qtmenu.h"

#include <QLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>


//-----------------------------------------------------------------------------
// Popup Form for managing the placing of subcells.
//
// Help system keywords used:
//  placepanel

// From GDSII spec.
#define MAX_ARRAY 32767

// Default window size, assumes 6X13 chars, 48 cols
#define DEF_WIDTH 292

#define PL_NEW_CODE "_new$$entry_"



// Main function to bring up the Placement Control pop-up.  If
// noprompt it true, don't prompt for a master if none is defined
//
void
cEdit::PopUpPlace(ShowMode mode, bool noprompt)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTplaceDlg::self())
            QTplaceDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTplaceDlg::self())
            QTplaceDlg::self()->update();
        else
            QTplaceDlg::update_params();
        return;
    }
    if (QTplaceDlg::self())
        return;

    new QTplaceDlg(noprompt);
    QTdev::self()->SetPopupLocation(GRloc(LW_UL), QTplaceDlg::self(),
        QTmainwin::self()->Viewport());
    QTplaceDlg::self()->show();

    // Give focus to main window.
//    QTdev::SetFocus(QTmainwin::self()->Shell());
}
// End of cEdit functions.


iap_t QTplaceDlg::pl_iap;
QTplaceDlg *QTplaceDlg::instPtr;

QTplaceDlg::QTplaceDlg(bool noprompt)
{
    instPtr = this;
    pl_arraybtn = 0;
    pl_replbtn = 0;
    pl_smashbtn = 0;
    pl_refmenu = 0;

    pl_label_nx = 0;
    pl_label_ny = 0;
    pl_label_dx = 0;
    pl_label_dy = 0;

    pl_masterbtn = 0;
    pl_placebtn = 0;
    pl_menu_placebtn = 0;
    pl_str_editor = 0;
    pl_dropfile = 0;

    ED()->plInitMenuLen();

    setWindowTitle(tr("Cell Placement Control"));
    setAttribute(Qt::WA_DeleteOnClose);
//    gtk_window_set_resizable(GTK_WINDOW(pl_popup), false);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // First row buttons.
    //
    pl_arraybtn = new QPushButton(tr("Use Array"));
    pl_arraybtn->setCheckable(true);
    hbox->addWidget(pl_arraybtn);
    connect(pl_arraybtn, SIGNAL(toggled(bool)),
        this, SLOT(array_btn_slot(bool)));

    pl_replbtn = new QPushButton(tr("Replace"));
    pl_replbtn->setCheckable(true);
    hbox->addWidget(pl_replbtn);
    connect(pl_replbtn, SIGNAL(clicked(bool)),
        this, SLOT(replace_btn_slot(bool)));

    pl_smashbtn = new QPushButton(tr("Smash"));
    pl_smashbtn->setCheckable(true);
    hbox->addWidget(pl_smashbtn);

    pl_refmenu = new QComboBox();
    pl_refmenu->addItem(tr("Origin"));
    pl_refmenu->addItem(tr("Lower Left"));
    pl_refmenu->addItem(tr("Upper Left"));
    pl_refmenu->addItem(tr("Upper Right"));
    pl_refmenu->addItem(tr("Lower Right"));
    pl_refmenu->setCurrentIndex(ED()->instanceRef());
    hbox->addWidget(pl_refmenu);
    connect(pl_refmenu, SIGNAL(currentIndexChanged(int)),
        this, SLOT(refmenu_slot(int)));

    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // Array set labels and entries.
    //
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QVBoxLayout *vb = new QVBoxLayout();
    hbox->addLayout(vb);

    pl_label_nx = new QLabel(tr("Nx"));
    vb->addWidget(pl_label_nx);
    pl_nx = new QSpinBox();
    vb->addWidget(pl_nx);
    pl_nx->setValue(1);
    pl_nx->setMaximum(MAX_ARRAY);
    pl_nx->setMinimum(1);
    connect(pl_nx, SIGNAL(valueChanged(int)),
        this, SLOT(nx_change_slot(int)));

    pl_label_ny = new QLabel(tr("Ny"));
    vb->addWidget(pl_label_ny);
    pl_ny = new QSpinBox();
    vb->addWidget(pl_ny);
    pl_ny->setValue(1);
    pl_ny->setMaximum(MAX_ARRAY);
    pl_ny->setMinimum(1);
    connect(pl_ny, SIGNAL(valueChanged(int)),
        this, SLOT(ny_change_slot(int)));

    vb = new QVBoxLayout();
    hbox->addLayout(vb);
    int ndgt = CD()->numDigits();

    pl_label_dx = new QLabel(tr("Dx"));
    vb->addWidget(pl_label_dx);;
    pl_dx = new QDoubleSpinBox();
    vb->addWidget(pl_dx);;
    pl_dx->setMaximum(1e6);
    pl_dx->setMinimum(-1e6);
    pl_dx->setValue(0.0);
    pl_dx->setDecimals(ndgt);
    connect(pl_dx, SIGNAL(valueChanged(double)),
        this, SLOT(dx_change_slot(double)));

    pl_label_dy = new QLabel(tr("Dy"));
    vb->addWidget(pl_label_dy);;
    pl_dy = new QDoubleSpinBox();
    vb->addWidget(pl_dy);
    pl_dy->setMaximum(1e6);
    pl_dy->setMinimum(-1e6);
    pl_dy->setValue(0.0);
    pl_dy->setDecimals(ndgt);
    connect(pl_dy, SIGNAL(valueChanged(double)),
        this, SLOT(dy_change_slot(double)));

    // Master selection option menu.
    //
    pl_masterbtn = new QComboBox();
    vbox->addWidget(pl_masterbtn);
    rebuild_menu();

    // Label and entry area for max menu length.
    //
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QLabel *label = new QLabel(tr("Maximum menu length"));
    hbox->addWidget(label);
    pl_mmlen = new QSpinBox();
    hbox->addWidget(pl_mmlen);
    pl_mmlen->setValue(ED()->plMenuLen());
    pl_mmlen->setMinimum(1);
    pl_mmlen->setMaximum(75);

    // Place and dismiss buttons.
    //
    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    MenuEnt *m = MainMenu()->FindEntry(MMside, MenuPLACE);
    if (m)
        pl_menu_placebtn = (QPushButton*)m->cmd.caller;
    if (pl_menu_placebtn) {
        // pl_menu_placebtn is the menu "place" button, which
        // will be "connected" to the Place button in the widget.

        pl_placebtn = new QPushButton(tr("Place"));;
        pl_placebtn->setCheckable(true);
        bool status = QTdev::GetStatus(pl_menu_placebtn);
        pl_placebtn->setChecked(status);
        hbox->addWidget(pl_placebtn);
        connect(pl_placebtn, SIGNAL(toggled(bool)),
            this, SLOT(place_btn_slot(bool)));
        connect(pl_menu_placebtn, SIGNAL(toggled(bool)),
            this, SLOT(menu_placebtn_slot(bool)));
    }

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    if (DSP()->CurMode() == Electrical)
        pl_arraybtn->setEnabled(false);
    set_sens(false);
    pl_refmenu->setCurrentIndex(ED()->instanceRef());

    // drop site
    setAcceptDrops(true);

    // Delay popping the string editor until we know where to put it!
    if (!noprompt)
        QTdev::self()->AddTimer(100, pl_timeout, 0);
}


QTplaceDlg::~QTplaceDlg()
{
    instPtr = 0;
    if (QTdev::GetStatus(pl_placebtn)) {
        // exit Place mode
        QTdev::CallCallback(pl_menu_placebtn);
    }
    // Turn off the replace and array functions
    ED()->setReplacing(false);
    ED()->setUseArray(false);
    int state = QTdev::GetStatus(pl_arraybtn);
    if (state)
        ED()->setArrayParams(iap_t());
    if (pl_str_editor)
        pl_str_editor->popdown();
}


void
QTplaceDlg::update()
{
    pl_refmenu->setCurrentIndex(ED()->instanceRef());

    ED()->plInitMenuLen();
    if (pl_mmlen->value() != ED()->plMenuLen())
        pl_mmlen->setValue(ED()->plMenuLen());

    if (DSP()->CurMode() == Electrical) {
        iap_t iaptmp = pl_iap;
        pl_nx->setValue(1);
        pl_ny->setValue(1);
        pl_dx->setValue(0.0);
        pl_dy->setValue(0.0);
        pl_iap = iaptmp;
        ED()->setArrayParams(iap_t());
        QTdev::SetStatus(pl_arraybtn, false);
        pl_arraybtn->setEnabled(false);
        set_sens(false);
    }
    else {
        pl_iap = ED()->arrayParams();
        pl_arraybtn->setEnabled(true);
        if (QTdev::GetStatus(pl_arraybtn)) {
            pl_nx->setValue(pl_iap.nx());
            pl_ny->setValue(pl_iap.ny());
            pl_dx->setValue(MICRONS(pl_iap.spx()));
            pl_dy->setValue(MICRONS(pl_iap.spy()));
        }
    }
}


// Static function.
ESret
QTplaceDlg::pl_new_cb(const char *string, void*)
{
    if (string && *string) {
        // If two tokens, the first is a library or archive file name,
        // the second is the cell name.

        char *aname = lstring::getqtok(&string);
        char *cname = lstring::getqtok(&string);
        ED()->addMaster(aname, cname);
        delete [] aname;
        delete [] cname;
    }
    return (ESTR_DN);
}


void
QTplaceDlg::desel_placebtn()
{
    QTdev::Deselect(pl_placebtn);
}


bool
QTplaceDlg::smash_mode()
{
    return (QTdev::GetStatus(pl_smashbtn));
}


// Set the order of names in the menu, and rebuild.
//
void
QTplaceDlg::rebuild_menu()
{
    if (!pl_masterbtn)
        return;
    pl_masterbtn->clear();
    pl_masterbtn->addItem(tr("New"));
    for (stringlist *p = ED()->plMenu(); p; p = p->next) {
        pl_masterbtn->addItem(tr(p->string));
    }
    if (ED()->plMenu())
        pl_masterbtn->setCurrentIndex(1);
    else
        pl_masterbtn->setCurrentIndex(0);
    connect(pl_masterbtn, SIGNAL(currentIndexChanged(int)),
        this, SLOT(master_menu_slot(int)));
}


// Override QWidget virtual function.
void
QTplaceDlg::dragEnterEvent(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasFormat("text/twostring") ||
            ev->mimeData()->hasFormat("text/cellname") ||
            ev->mimeData()->hasFormat("text/string") ||
            ev->mimeData()->hasFormat("text/plain")) {
        ev->acceptProposedAction();
    }
}


// Override QWidget virtual function.
void
QTplaceDlg::dropEvent(QDropEvent *ev)
{
    const char *fmt = 0;
    if (ev->mimeData()->hasFormat("text/twostring"))
        fmt = "text/twostring";
    else if (ev->mimeData()->hasFormat("text/cellname"))
        fmt = "text/cellname";
    else if (ev->mimeData()->hasFormat("text/string"))
        fmt = "text/string";
    else if (ev->mimeData()->hasFormat("text/plain"))
        fmt = "text/plain";
    if (fmt) {
        QByteArray bary = ev->mimeData()->data(fmt);
        char *src = lstring::copy((const char*)bary.data());
        char *t = 0;
        if (!strcmp(fmt, "text/twostring")) {
            // Drops from content lists may be in the form
            // "fname_or_chd\ncellname".
            t = strchr(src, '\n');
            if (t)
                *t++ = 0;
        }
        delete [] pl_dropfile;
        pl_dropfile = 0;
        if (pl_str_editor) {
            pl_str_editor->update(0, src);
            delete [] src;
        }
        else {
            pl_dropfile = src;
            master_menu_slot(0);
        }
        ev->acceptProposedAction();
    }
}


// Set the sensitivity of the array parameter entry widgets
//
void
QTplaceDlg::set_sens(bool set)
{
    pl_label_nx->setEnabled(set);
    pl_nx->setEnabled(set);
    pl_label_ny->setEnabled(set);
    pl_ny->setEnabled(set);
    pl_label_dx->setEnabled(set);
    pl_dx->setEnabled(set);
    pl_label_dy->setEnabled(set);
    pl_dy->setEnabled(set);
}


// Static function
int
QTplaceDlg::pl_timeout(void*)
{
    if (instPtr)
        instPtr->master_menu_slot(0);
    return (0);
}


void
QTplaceDlg::array_btn_slot(bool state)
{
    // When on, cells placed can be arrayed.  Otherwise only a single
    // instance is created.

    if (state) {
        pl_nx->setValue(pl_iap.nx());
        pl_ny->setValue(pl_iap.ny());
        pl_dx->setValue(MICRONS(pl_iap.spx()));
        pl_dy->setValue(MICRONS(pl_iap.spy()));
        set_sens(true);
        ED()->setUseArray(true);
    }
    else {
        iap_t iaptmp = pl_iap;
        pl_nx->setValue(1);
        pl_ny->setValue(1);
        pl_dx->setValue(0.0);
        pl_dy->setValue(0.0);
        pl_iap = iaptmp;
        set_sens(false);
        ED()->setUseArray(false);
    }
}


void
QTplaceDlg::replace_btn_slot(bool state)
{
    // When the replace mode is active, clicking on existing instances
    // will cause them to be replaced by the current master.  The
    // current transform is added to the existing transform.
    //
    if (state) {
        ED()->setReplacing(true);
        PL()->ShowPrompt("Select subcells to replace, press Esc to quit.");
        QTdev::SetStatus(pl_smashbtn, false);
        pl_smashbtn->setEnabled(false);
    }
    else {
        ED()->setReplacing(false);
        PL()->ShowPrompt(
            "Click on locations to place cell, press Esc to quit.");
        pl_smashbtn->setEnabled(true);
        if (DSP()->CurMode() == Physical)
            pl_arraybtn->setEnabled(true);
    }
}


void
QTplaceDlg::refmenu_slot(int ix)
{
    // The reference point of the cell, i.e., the point that is
    // located at the cursor, is switchable between the corners of the
    // untransformed cell, or the origin of the cell, in physical
    // mode.

    if (ix < 0)
        return;
    if (ix == 0)
        ED()->setInstanceRef(PL_ORIGIN);
    else if (ix == 1)
        ED()->setInstanceRef(PL_LL);
    else if (ix == 2)
        ED()->setInstanceRef(PL_UL);
    else if (ix == 3)
        ED()->setInstanceRef(PL_UR);
    else if (ix == 4)
        ED()->setInstanceRef(PL_LR);
}


void
QTplaceDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("placepanel"))
}


void
QTplaceDlg::dismiss_btn_slot()
{
    ED()->PopUpPlace(MODE_OFF, false);
}


void
QTplaceDlg::master_menu_slot(int ix)
{
    if (ix == 0) {
        if (QTdev::GetStatus(pl_placebtn))
            QTdev::CallCallback(pl_menu_placebtn);
        char *dfile = pl_dropfile;
        pl_dropfile = 0;
        GCarray<char*> gc_dfile(dfile);
        const char *defname = dfile;
        if (!defname || !*defname) {
            defname = XM()->GetCurFileSelection();
            if (!defname) {
                CDc *cd = (CDc*)Selections.firstObject(CurCell(), "c");
                if (cd)
                    defname = Tstring(cd->cellname());
            }
        }
        if (pl_str_editor)
            pl_str_editor->popdown();
        int xx, yy;
        QTdev::self()->Location(pl_masterbtn, &xx, &yy);
        pl_str_editor = DSPmainWbagRet(PopUpEditString(0,
            GRloc(LW_XYA, xx - 200, yy), "File or cell name of cell to place?",
            defname, pl_new_cb, 0, 200, 0));
        if (pl_str_editor)
            pl_str_editor->register_usrptr((void**)&pl_str_editor);
    }
    else {
        const char *tok = (const char*)pl_masterbtn->currentText().toLatin1();
        const char *string = tok;
        const char *aname = lstring::getqtok(&string);
        const char *cname = lstring::getqtok(&string);
        ED()->addMaster(aname, cname);
        delete [] aname;
        delete [] cname;
    }
}


void
QTplaceDlg::place_btn_slot(bool state)
{
    bool mbstate = QTdev::GetStatus(pl_menu_placebtn);
    if (state != mbstate)
        QTdev::CallCallback(pl_menu_placebtn);
}


void
QTplaceDlg::menu_placebtn_slot(bool status)
{
    if (status)
        QTdev::Select(pl_placebtn);
    else
        QTdev::Deselect(pl_placebtn);
}


void
QTplaceDlg::mmlen_change_slot(int val)
{
    if (val == DEF_PLACE_MENU_LEN)
        CDvdb()->clearVariable(VA_MasterMenuLength);
    else {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", val);
        CDvdb()->setVariable(VA_MasterMenuLength, buf);
    }
}


void
QTplaceDlg::nx_change_slot(int num)
{
    pl_iap.set_nx(num);
    if (QTmainwin::self()) {
        QTmainwin::self()->ShowGhost(ERASE);
        ED()->setArrayParams(pl_iap);
        QTmainwin::self()->ShowGhost(DISPLAY);
    }
}


void
QTplaceDlg::ny_change_slot(int num)
{
    pl_iap.set_ny(num);
    if (QTmainwin::self()) {
        QTmainwin::self()->ShowGhost(ERASE);
        ED()->setArrayParams(pl_iap);
        QTmainwin::self()->ShowGhost(DISPLAY);
    }
}


void
QTplaceDlg::dx_change_slot(double val)
{
    pl_iap.set_spx(INTERNAL_UNITS(val));
    if (QTmainwin::self()) {
        QTmainwin::self()->ShowGhost(ERASE);
        ED()->setArrayParams(pl_iap);
        QTmainwin::self()->ShowGhost(DISPLAY);
    }
}


void
QTplaceDlg::dy_change_slot(double val)
{
    pl_iap.set_spy(INTERNAL_UNITS(val));
    if (QTmainwin::self()) {
        QTmainwin::self()->ShowGhost(ERASE);
        ED()->setArrayParams(pl_iap);
        QTmainwin::self()->ShowGhost(DISPLAY);
    }
}

