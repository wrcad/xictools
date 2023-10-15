
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

#include "qtdevedit.h"
#include "fio.h"
#include "sced.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "fio_library.h"
#include "events.h"
#include "promptline.h"
#include "errorlog.h"
#include "menu.h"
#include "miscutil/filestat.h"

#include <QLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>


//-----------------------------------------------------------------------------
// The Device Parameters pop-up.  This panel allows addition of
// properties to library devices, and initiates saving the devices in
// the device library file or a cell file.
//
// Help system keywords used:
//  devedit


// Pop up the Device Parameters panel.
//
void
cSced::PopUpDevEdit(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTdeviceDlg::self())
            QTdeviceDlg::self()->deleteLater();
        return;
    }
    if (QTdeviceDlg::self())
        return;

    CDs *sd = CurCell(Electrical);
    if (!sd) {
        Log()->PopUpErr("No current cell!");
        if (caller)
            MainMenu()->SetStatus(caller, false);
        return;
    }
    CDm_gen mgen(sd, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        if (mdesc->hasInstances()) {
            Log()->PopUpErr(
                "Current cell contains instantiations, can't be a device.");
            if (caller)
                MainMenu()->SetStatus(caller, false);
            return;
        }
    }
    CDs *cursdp = CurCell(Physical);
    if (cursdp && !cursdp->isEmpty()) {
        Log()->PopUpErr(
            "Current cell contains physical data, can't be a device.");
        if (caller)
            MainMenu()->SetStatus(caller, false);
        return;
    }

    new QTdeviceDlg(caller);

    QTdev::self()->SetPopupLocation(GRloc(), QTdeviceDlg::self(),
        QTmainwin::self()->Viewport());
    QTdeviceDlg::self()->show();
}
// End of cSced functions.


namespace {
    struct sBstate : public CmdState
    {
        sBstate(const char*, const char*);
        virtual ~sBstate();

        void setXY(int x, int y)
            {
                xref = x;
                yref = y;
            }
        int x_ref()         { return (xref); }
        int y_ref()         { return (yref); }

        void b1down();
        void b1up();
        void esc();

    private:
        int xref, yref;
    };

    sBstate *Bcmd;
}


const char *QTdeviceDlg::orient_labels[] =
    { "none", "Left", "Down", "Right", "Up", 0 };
QTdeviceDlg *QTdeviceDlg::instPtr;

QTdeviceDlg::QTdeviceDlg(GRobject caller)
{
    instPtr = this;
    de_caller = caller;
    de_cname = 0;
    de_prefix = 0;
    de_model = 0;
    de_value = 0;
    de_param = 0;
    de_toggle = 0;
    de_branch = 0;
    de_nophys = 0;
    de_menustate = 0;
    de_xref = de_yref = 0;

    CDs *cursde = CurCell(Electrical);

    setWindowTitle(tr("Device Parameters"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QGridLayout *grid = new QGridLayout(this);
    grid->setContentsMargins(qmtop);
    grid->setSpacing(2);

    QLabel *label = new QLabel(tr("Device Name"));
    grid->addWidget(label, 0, 0);

    de_cname = new QLineEdit();
    grid->addWidget(de_cname, 0, 1);
    de_cname->setText(Tstring(DSP()->CurCellName()));

    label = new QLabel(tr("SPICE Prefix"));
    grid->addWidget(label, 1, 0);

    de_prefix = new QLineEdit();
    grid->addWidget(de_prefix, 1, 1);

    CDp_sname *pn = (CDp_sname*)(cursde ? cursde->prpty(P_NAME) : 0);
    if (pn && pn->name_string()) {
        sLstr lstr;
        lstr.add(Tstring(pn->name_string()));
        if (pn->is_macro())
            lstr.add(" macro");
        de_prefix->setText(lstr.string());
    }

    QPushButton *btn = new QPushButton(tr("Help"));
    grid->addWidget(btn, 0, 2, 2, 1);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    label = new QLabel(tr("Default Model"));
    grid->addWidget(label, 2, 0);

    de_model = new QLineEdit();
    grid->addWidget(de_model, 2, 1, 1, 2);

    CDp_user *pu = (CDp_user*)(cursde ? cursde->prpty(P_MODEL) : 0);
    if (pu) {
        char *s = hyList::string(pu->data(), HYcvPlain, false);
        de_model->setText(s);
        delete [] s;
    }

    label = new QLabel(tr("Default Value"));
    grid->addWidget(label, 3, 0);

    de_value = new QLineEdit();
    grid->addWidget(de_value, 3, 1, 1, 2);

    pu = (CDp_user*)(cursde ? cursde->prpty(P_VALUE) : 0);
    if (pu) {
        char *s = hyList::string(pu->data(), HYcvPlain, false);
        de_value->setText(s);
        delete [] s;
    }

    label = new QLabel(tr("Default Parameters"));
    grid->addWidget(label, 4, 0);

    de_param = new QLineEdit();
    grid->addWidget(de_param, 4, 1, 1, 2);

    pu = (CDp_user*)(cursde ? cursde->prpty(P_PARAM) : 0);
    if (pu) {
        char *s = hyList::string(pu->data(), HYcvPlain, false);
        de_param->setText(s);
        delete [] s;
    }

    QHBoxLayout *hbox = new QHBoxLayout();
    grid->addLayout(hbox, 5, 0);

    de_toggle = new QPushButton(tr("Hot Spot"));
    de_toggle->setCheckable(true);
    hbox->addWidget(de_toggle);
    connect(de_toggle, SIGNAL(toggled(bool)),
        this, SLOT(hotspot_btn_slot(bool)));

    QComboBox *entry = new QComboBox();
    hbox->addWidget(entry);
    for (int i = 0; orient_labels[i]; i++)
        entry->addItem(orient_labels[i]);
    connect(entry, SIGNAL(currentIndexChanged(int)),
        this, SLOT(menu_changed_slot(int)));

    de_branch = new QLineEdit();
    grid->addWidget(de_branch, 5, 1, 1, 2);

    CDp_branch *pb = (CDp_branch*)(cursde ? cursde->prpty(P_BRANCH) : 0);
    if (pb) {
        int ix = 0;
        if (!pb->rot_x() && pb->rot_y()) {
            if (pb->rot_y() > 0)
                ix = 4;
            else
                ix = 2;
        }
        else if (pb->rot_x() && !pb->rot_y()) {
            if (pb->rot_x() < 0)
                ix = 1;
            else
                ix = 3;
        }
        de_menustate = ix;
        entry->setCurrentIndex(ix);
        if (pb->br_string())
            de_branch->setText(pb->br_string());

        de_xref = pb->pos_x();
        de_yref = pb->pos_y();

        QTdev::SetStatus(de_toggle, true);
/*XXX
        de_branch_proc(de_toggle, 0);
*/
    }

    de_nophys = new QCheckBox(tr("No Physical Implementation"));
    grid->addWidget(de_nophys, 6, 0, 1, 3);

    CDp *pnp = cursde ? cursde->prpty(P_NOPHYS) : 0;
    if (pnp)
        QTdev::SetStatus(de_nophys, true);

    hbox = new QHBoxLayout;
    grid->addLayout(hbox, 7, 0, 1, 3);
    hbox->setContentsMargins(qmtop);
    hbox->setSpacing(2);

    btn = new QPushButton(tr("Save in Library"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(savlib_btn_slot()));

    btn = new QPushButton(tr("Save as Cell File"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(savfile_btn_slot()));

    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));
}


QTdeviceDlg::~QTdeviceDlg()
{
    instPtr = 0;
    if (de_caller)
        QTdev::SetStatus(de_caller, false);
    if (Bcmd)
        Bcmd->esc();
}


void
QTdeviceDlg::set_ref(int x, int y)
{
    de_xref = x;
    de_yref = y;
    QTdev::SetStatus(de_branch, false);
}


namespace {
    // Copy, and remove leading/trailing white space.  Null is
    // returned for an empty string.
    //
    char *strip(const char *str)
    {
        if (!str)
            return (0);
        while (isspace(*str))
            str++;
        if (!*str)
            return (0);
        char *nstr = lstring::copy(str);
        char *e = (char*)nstr + strlen(nstr) - 1;
        while (e >= nstr && isspace(*e))
            *e-- = 0;
        return (nstr);
    }
}


// Load the entries from the pop-up.
//
void
QTdeviceDlg::load(entries_t *e)
{
    if (!e)
        return;
    QByteArray cname_ba = de_cname->text().toLatin1();
    e->cname = strip(cname_ba.constData());
    char *cn = lstring::strip_path(e->cname);
    if (cn != e->cname) {
        char *ctmp = e->cname;
        e->cname = lstring::copy(cn);
        delete [] ctmp;
    }
    QByteArray prefix_ba = de_prefix->text().toLatin1();
    e->prefix = strip(prefix_ba.constData());

    QByteArray model_ba = de_model->text().toLatin1();
    e->model = strip(model_ba.constData());

    QByteArray value_ba = de_value->text().toLatin1();
    e->value = strip(value_ba.constData());

    QByteArray param_ba = de_param->text().toLatin1();
    e->param = strip(param_ba.constData());

    QByteArray branch_ba = de_branch->text().toLatin1();
    e->branch = strip(branch_ba.constData());

    // Make sure that 'X' is a macro.
    if (*e->prefix == 'X' || *e->prefix == 'x') {
        char *tmp = e->prefix;
        char *tok = lstring::gettok(&tmp);
        int len = strlen(tok) + 7;
        tmp = new char[len];
        snprintf(tmp, len, "%s macro", tok);
        delete [] tok;
        delete [] e->prefix;
        e->prefix = tmp;
    }
}


void
QTdeviceDlg::do_save(bool tofile)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde) {
        Log()->PopUpErr("No curent cell!");
        return;
    }

    entries_t ent;
    load(&ent);
    if (!ent.cname) {
        Log()->PopUpErr("No Cell Name given, this is required.");
        return;
    }

    int nodecnt = 0;
    CDp_node *pnd = (CDp_node*)cursde->prpty(P_NODE);
    while (pnd) {
        nodecnt++;
        pnd = pnd->next();
    }

    // If a device is keyed by 'X', it is really a subcircuit macro
    // call to the model library.  The subname field must be 0 for
    // devices in the device library file.  The name of the subcircuit
    // macro is stored in a model property, so that the text will be
    // added along with the model text.  The subckt macros are saved
    // in the model database.
    bool is_sc = ent.prefix && (*ent.prefix == 'X' || *ent.prefix == 'x');

    if (!is_sc && !nodecnt) {
        Log()->PopUpErr(
            "Device has no nodes, use subct button to define nodes.");
        return;
    }

    // For a subcircuit macro reference, the only valid properties are
    // node, name, param, and model (for the macro name).  Note that
    // the user can reset the macro name in instances.

    if (ent.model) {
        CDp_user *pu = (CDp_user*)cursde->prpty(P_MODEL);
        if (pu)
            hyList::destroy(pu->data());
        else {
            pu = new CDp_user(P_MODEL);
            pu->set_next_prp(cursde->prptyList());
            cursde->setPrptyList(pu);
        }
        pu->set_data(new hyList(cursde, ent.model, HYcvPlain));

        pu = (CDp_user*)cursde->prpty(P_VALUE);
        if (pu) {
            cursde->prptyUnlink(pu);
            delete pu;
        }
    }
    else if (ent.value) {
        if (is_sc) {
            Log()->PopUpErr("No Model given, this is mandatory for macro.");
            return;
        }
        CDp_user *pu = (CDp_user*)cursde->prpty(P_VALUE);
        if (pu)
            hyList::destroy(pu->data());
        else {
            pu = new CDp_user(P_VALUE);
            pu->set_next_prp(cursde->prptyList());
            cursde->setPrptyList(pu);
        }
        pu->set_data(new hyList(cursde, ent.value, HYcvPlain));

        pu = (CDp_user*)cursde->prpty(P_MODEL);
        if (pu) {
            cursde->prptyUnlink(pu);
            delete pu;
        }
    }
    else {
        if (is_sc) {
            Log()->PopUpErr("No Model given, this is mandatory for macro.");
            return;
        }
        CDp_user *pu = (CDp_user*)cursde->prpty(P_VALUE);
        if (pu) {
            cursde->prptyUnlink(pu);
            delete pu;
        }
        pu = (CDp_user*)cursde->prpty(P_MODEL);
        if (pu) {
            cursde->prptyUnlink(pu);
            delete pu;
        }
    }

    if (ent.param) {
        CDp_user *pu = (CDp_user*)cursde->prpty(P_PARAM);
        if (pu)
            hyList::destroy(pu->data());
        else {
            pu = new CDp_user(P_PARAM);
            pu->set_next_prp(cursde->prptyList());
            cursde->setPrptyList(pu);
        }
        pu->set_data(new hyList(cursde, ent.param, HYcvPlain));
    }
    else {
        CDp_user *pu = (CDp_user*)cursde->prpty(P_PARAM);
        if (pu) {
            cursde->prptyUnlink(pu);
            delete pu;
        }
    }

    if (QTdev::GetStatus(de_toggle) && !is_sc) {
        CDp_branch *pb = (CDp_branch*)cursde->prpty(P_BRANCH);
        if (!pb) {
            pb = new CDp_branch;
            pb->set_next_prp(cursde->prptyList());
            cursde->setPrptyList(pb);
        }
        pb->set_br_string(ent.branch);
        switch (de_menustate) {
        default:
        case 0:
            pb->set_rot_x(0);
            pb->set_rot_y(0);
            break;
        case 1:  // Left
            pb->set_rot_x(-1);
            pb->set_rot_y(0);
            break;
        case 2:  // Down
            pb->set_rot_x(0);
            pb->set_rot_y(-1);
            break;
        case 3:  // Right
            pb->set_rot_x(1);
            pb->set_rot_y(0);
            break;
        case 4:  // Up
            pb->set_rot_x(0);
            pb->set_rot_y(1);
            break;
        }
        pb->set_pos_x(Bcmd->x_ref());
        pb->set_pos_y(Bcmd->y_ref());
    }
    else {
        CDp_branch *pb = (CDp_branch*)cursde->prpty(P_BRANCH);
        if (pb) {
            cursde->prptyUnlink(pb);
            delete pb;
        }
    }

    if (QTdev::GetStatus(de_nophys)) {
        CDp *p = cursde->prpty(P_NOPHYS);
        if (!p) {
            p = new CDp(P_NOPHYS);
            p->set_string("nophys");
            p->set_next_prp(cursde->prptyList());
            cursde->setPrptyList(p);
        }
    }
    else {
        CDp *p = cursde->prpty(P_NOPHYS);
        if (p) {
            cursde->prptyUnlink(p);
            delete p;
        }
    }

    if (!ent.prefix) {
        // This is only possible for a ground terminal.
        if (nodecnt == 1 && !ent.model && !ent.value && !ent.param &&
                !QTdev::GetStatus(de_toggle)) {
            CDp_sname *pn = (CDp_sname*)cursde->prpty(P_NAME);
            if (pn) {
                cursde->prptyUnlink(pn);
                delete pn;
            }
        }
        else {
            Log()->PopUpErr(
                "No Prefix given, this is required for all except "
                "ground terminal.");
            return;
        }
    }
    else {
        CDp_sname *pn = (CDp_sname*)cursde->prpty(P_NAME);
        if (!pn) {
            pn = new CDp_sname;
            pn->set_next_prp(cursde->prptyList());
            cursde->setPrptyList(pn);
        }
        pn->set_name_string(ent.prefix);
    }
    cursde->setDevice(true);

    if (!SCD()->saveAsDev(ent.cname, tofile)) {
        Log()->ErrorLogV(mh::Processing,
            "Save as device failed:\n%s.", Errs()->get_error());
        return;
    }

    if (tofile)
        PL()->ShowPrompt("Device cell saved.");
    else
        PL()->ShowPrompt("Device library updated, in current directory.");

    SCD()->PopUpDevEdit(0, MODE_OFF);
}


void
QTdeviceDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("devedit"))
}


void
QTdeviceDlg::hotspot_btn_slot(bool state)
{
    if (state) {
        if (!Bcmd) {
            Bcmd = new sBstate("brloc", "devedit#hspot");
            Bcmd->setXY(de_xref, de_yref);
            if (!EV()->PushCallback(Bcmd))
                delete Bcmd;
            DSP()->ShowCrossMark(DISPLAY, Bcmd->x_ref(), Bcmd->y_ref(),
                HighlightingColor, 20, DSP()->CurMode());
        }
    }
    else {
        if (Bcmd)
            Bcmd->esc();
    }
}


void
QTdeviceDlg::menu_changed_slot(int i)
{
    de_menustate = i;
}


void
QTdeviceDlg::savlib_btn_slot()
{
    do_save(false);
}


void
QTdeviceDlg::savfile_btn_slot()
{
    do_save(true);
}


void
QTdeviceDlg::dismiss_btn_slot()
{
    SCD()->PopUpDevEdit(0, MODE_OFF);
}



#ifdef notdef


// When the Branch button is active, clicking in the drawing moves a
// marker around to set the "hot spot" location for the branch property.

// Static function.
void
QTdeviceDlg::de_branch_proc(GtkWidget *caller, void*)
{
}
#endif
// End of QTdeviceDlg functions.


sBstate::sBstate(const char *nm, const char *hk) : CmdState(nm, hk)
{
    xref = 0;
    yref = 0;
}


sBstate::~sBstate()
{
    Bcmd = 0;
}


void
sBstate::b1down()
{
    DSP()->ShowCrossMark(ERASE, xref, yref, HighlightingColor,
        20, DSP()->CurMode());
    EV()->Cursor().get_xy(&xref, &yref);
    DSP()->ShowCrossMark(DISPLAY, xref, yref, HighlightingColor,
        20, DSP()->CurMode());
}


void
sBstate::b1up()
{
}


void
sBstate::esc()
{
    if (QTdeviceDlg::self())
        QTdeviceDlg::self()->set_ref(xref, yref);
    DSP()->ShowCrossMark(ERASE, xref, yref, HighlightingColor, 20,
        DSP()->CurMode());
    EV()->PopCallback(this);
    delete this;
}

