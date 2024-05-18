
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

#include "qtlpedit.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "tech.h"
#include "tech_extract.h"
#include "tech_layer.h"
#include "tech_ldb3d.h"
#include "layertab.h"
#include "promptline.h"
#include "events.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qttextw.h"

#include <QLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QTabWidget>
#include <QMouseEvent>
#include <QGroupBox>
#include <QLabel>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>


//-----------------------------------------------------------------------------
// QTlayerParamDlg:  Layer and tech parameter editor dialog.
// Called from main menu: Attributes/Edit Tech Params.
//
// Help system keywords used:
//  xic:lpedt

void
cMain::PopUpLayerParamEditor(GRobject caller, ShowMode mode, const char *msg,
    const char *string)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTlayerParamDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        // update the text
        if (QTlayerParamDlg::self())
            QTlayerParamDlg::self()->update(msg, string);
        return;
    }
    if (QTlayerParamDlg::self())
        return;

    new QTlayerParamDlg(caller, msg, string);

    QTlayerParamDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTlayerParamDlg::self(),
        QTmainwin::self()->Viewport());
    QTlayerParamDlg::self()->show();
}
// End of cMain functions.

#ifdef Q_OS_MACOS
#define USE_QTOOLBAR
#endif

// Default window size, assumes 6X13 chars, 80 cols, 12 rows
// with a 2-pixel margin.
#define DEF_WIDTH 364
#define DEF_HEIGHT 80

namespace {

        enum {lpBoxLineStyle, lpLayerReorderMode, lpNoPlanarize,
            lpAntennaTotal, lpSubstrateEps, lpSubstrateThickness };
}

QTlayerParamDlg *QTlayerParamDlg::instPtr;

QTlayerParamDlg::QTlayerParamDlg(GRobject c, const char *msg,
    const char *string)
{
    instPtr = this;
    lp_caller = c;
    lp_text = 0;

    lp_lyr_menu = 0;
    lp_ext_menu = 0;
    lp_phy_menu = 0;
    lp_cvt_menu = 0;

    lp_label = 0;
    lp_cancel = 0;
    lp_edit = 0;
    lp_del = 0;
    lp_undo = 0;
    lp_ivis = 0;
    lp_nmrg = 0;
    lp_xthk = 0;
    lp_wira = 0;
    lp_noiv = 0;
    lp_darkfield = 0;
    lp_rho = 0;
    lp_sigma = 0;
    lp_rsh = 0;
    lp_tau = 0;
    lp_epsrel = 0;
    lp_cap = 0;
    lp_lambda = 0;
    lp_tline = 0;
    lp_antenna = 0;
    lp_nddt = 0;

    lp_ldesc = 0;
    lp_line_selected = -1;
    lp_in_callback = false;
    lp_start = 0;
    lp_end = 0;
    lp_page = 0;

    setWindowTitle(tr("Tech Parameter Editor"));
    setAttribute(Qt::WA_DeleteOnClose);

    // menu bar
    //
#ifdef USE_QTOOLBAR
    QToolBar *menubar = new QToolBar(this);
#else
    QMenuBar *menubar = new QMenuBar(this);
#endif

    // Edit menu.
    QAction *a;
#ifdef USE_QTOOLBAR
    a = menubar->addAction(tr("&Edit"));
    QMenu *menu = new QMenu();
    a->setMenu(menu);
    QToolButton *tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    QMenu *menu = menubar->addMenu(tr("&Edit"));
#endif
    connect(menu, &QMenu::triggered,
        this, &QTlayerParamDlg::edit_menu_slot);
    // _Edit, edEdit, Ctrl-E
    a = menu->addAction(tr("Edit"));
    a->setData(edEdit);
    a->setShortcut(QKeySequence("Ctrl+E"));
    lp_edit = a;
    // _Delete, edDelect, Ctrl-D
    a = menu->addAction(tr("&Delete"));
    a->setData(edDelete);
    a->setShortcut(QKeySequence("Ctrl+D"));
    lp_del = a;
    // _Undo, edUndo, Ctrl-U
    a = menu->addAction(tr("&Undo"));
    a->setData(edUndo);
    a->setShortcut(QKeySequence("Ctrl+U"));
    lp_undo = a;
    // Separator
    menu->addSeparator();
    // _Quit, edQuit, Ctrl-Q
    a = menu->addAction(tr("&Quit"));
    a->setData(edQuit);
    a->setShortcut(QKeySequence("Ctrl+Q"));

    // Layer Keywords menu.
#ifdef USE_QTOOLBAR
    a = menubar->addAction("Layer Keywords");
    lp_lyr_menu = new QMenu();
    a->setMenu(lp_lyr_menu);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    lp_lyr_menu = menubar->addMenu(tr("Layer &Keywords"));
#endif
    connect(lp_lyr_menu, &QMenu::triggered,
        this, &QTlayerParamDlg::layer_menu_slot);
    // LppName, lpLppName, 0
    a = lp_lyr_menu->addAction("LppName");
    a->setData(lpLppName);
    // Description, lpDescription, 0);
    a = lp_lyr_menu->addAction("Description");
    a->setData(lpDescription);
    // NoSelect, lpNoSelect, 0);
    a = lp_lyr_menu->addAction("NoSelect");
    a->setData(lpNoSelect);
    // NoMerge, lpNoMerge, 0);
    a = lp_lyr_menu->addAction("NoMerge");
    a->setData(lpNoMerge);
    lp_nmrg = a;
    // WireActive, lpWireActive, 0);
    a = lp_lyr_menu->addAction("WireActive");
    a->setData(lpWireActive);
    lp_wira = a;
    // Symbolic, lpSymbolic, 0
    a = lp_lyr_menu->addAction("Symbolic");
    a->setData(lpSymbolic);
    // The Invalid keyword is not really supported.  At best
    // it can be provided in the tech file only.
    // Invisible, lpInvisible, 0
    a = lp_lyr_menu->addAction("Invisible");
    a->setData(lpInvisible);
    lp_ivis = a;
    // NoInstView, lpNoInstView, 0
    a = lp_lyr_menu->addAction("NoInstView");
    a->setData(lpNoInstView);
    lp_noiv = a;
    // WireWidth, lpWireWidth, 0
    a = lp_lyr_menu->addAction("WireWidth");
    a->setData(lpWireWidth);
    // CrossThick, lpCrossThick, 0
    a = lp_lyr_menu->addAction("CrossThick");
    a->setData(lpCrossThick);
    lp_xthk = a;

    // Extract Keywords menu.
#ifdef USE_QTOOLBAR
    a = menubar->addAction("Extract Keywords");
    lp_ext_menu = new QMenu();
    a->setMenu(lp_ext_menu);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    lp_ext_menu = menubar->addMenu(tr("Extract &Keywords"));
#endif
    connect(lp_ext_menu, &QMenu::triggered,
        this, &QTlayerParamDlg::extract_menu_slot);
    lp_ext_menu->menuAction()->setVisible(false);
    // Conductor, exConductor, 0
    a = lp_ext_menu->addAction("Conductor");
    a->setData(exConductor);
    // Routing, exRouting, 0);
    a = lp_ext_menu->addAction("Routing");
    a->setData(exRouting);
    // GroundPlane, exGroundPlane, 0);
    a = lp_ext_menu->addAction("GroundPlane");
    a->setData(exGroundPlane);
    // GroundPlaneClear, exGroundPlaneClear, 0
    a = lp_ext_menu->addAction("GroundPlaneClear");
    a->setData(exGroundPlaneClear);
    // Contact, exContact, 0);
    a = lp_ext_menu->addAction("Contact");
    a->setData(exContact);
    // Via, exVia, 0);
    a = lp_ext_menu->addAction("Via");
    a->setData(exVia);
    // ViaCut, exViaCut, 0
    a = lp_ext_menu->addAction("ViaCut");
    a->setData(exViaCut);
    // Dielectric, exDielectric, 0
    a = lp_ext_menu->addAction("Dielectric");
    a->setData(exDielectric);
    // DarkField, exDarkField, 0
    a = lp_ext_menu->addAction("DarkField");
    a->setData(exDarkField);
    lp_darkfield = a;

    // Physical Keywords menu.
#ifdef USE_QTOOLBAR
    a = menubar->addAction("Physical Keywords");
    lp_phy_menu = new QMenu();
    a->setMenu(lp_phy_menu);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    lp_phy_menu = menubar->addMenu(tr("Physical &Keywords"));
#endif
    connect(lp_phy_menu, &QMenu::triggered,
        this, &QTlayerParamDlg::physical_menu_slot);
    lp_phy_menu->menuAction()->setVisible(false);
    // Planarize, 0, lp_kw_proc, phPlanarize, 0
    a = lp_phy_menu->addAction("Planarize");
    a->setData(phPlanarize);
    // Thickness, 0, lp_kw_proc, phThickness, 0
    a = lp_phy_menu->addAction("Thickness");
    a->setData(phThickness);
    // FH_nhinc, 0, lp_kw_proc, phFH_nhinc, 0
    a = lp_phy_menu->addAction("FH_nhinc");
    a->setData(phFH_nhinc);
    // FH_rh, 0, lp_kw_proc, phFH_rh, 0
    a = lp_phy_menu->addAction("FH_rh");
    a->setData(phFH_rh);
    // Rho, 0, lp_kw_proc, phRho, 0
    a = lp_phy_menu->addAction("Rho");
    a->setData(phRho);
    lp_rho = a;
    // Sigma, 0, lp_kw_proc, phSigma, 0
    a = lp_phy_menu->addAction("Sigma");
    a->setData(phSigma);
    lp_sigma = a;
    // Rsh, 0, lp_kw_proc, phRsh, 0
    a = lp_phy_menu->addAction("Rsh");
    a->setData(phRsh);
    lp_rsh = a;
    // Tau, 0, lp_kw_proc, phTau, 0
    a = lp_phy_menu->addAction("Tau");
    a->setData(phTau);
    lp_tau = a;
    // EpsRel, 0, lp_kw_proc, phEpsRel, 0
    a = lp_phy_menu->addAction("EpsRel");
    a->setData(phEpsRel);
    lp_epsrel = a;
    // Capacitance, 0, lp_kw_proc, phCapacitance, 0
    a = lp_phy_menu->addAction("Capacitance");
    a->setData(phCapacitance);
    lp_cap = a;
    // Lambda, 0, lp_kw_proc, phLambda, 0);
    a = lp_phy_menu->addAction("Lambda");
    a->setData(phLambda);
    lp_lambda = a;
    // Tline, 0, lp_kw_proc, phTline, 0
    a = lp_phy_menu->addAction("Tline");
    a->setData(phTline);
    lp_tline = a;
    // Antenna, 0, lp_kw_proc, phAntenna, 0
    a = lp_phy_menu->addAction("Antenna");
    a->setData(phAntenna);
    lp_antenna = a;

    // Convert Keywords menu.
#ifdef USE_QTOOLBAR
    a = menubar->addAction("Convert Keywords");
    lp_cvt_menu = new QMenu();
    a->setMenu(lp_cvt_menu);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    lp_cvt_menu = menubar->addMenu(tr("Convert &Keywords"));
#endif
    connect(lp_cvt_menu, &QMenu::triggered,
        this, &QTlayerParamDlg::convert_menu_slot);
    lp_cvt_menu->menuAction()->setVisible(false);
    // StreamIn, cvStreamIn, 0
    a = lp_cvt_menu->addAction("StreamIn");
    a->setData(cvStreamIn);
    // StreamOut, cvStreamOut, 0
    a = lp_cvt_menu->addAction("StreamOut");
    a->setData(cvStreamOut);
    // NoDrcDataType, cvNoDrcDataType, 0
    a = lp_cvt_menu->addAction("NoDrcDataType");
    a->setData(cvNoDrcDataType);
    lp_nddt = a;

    // Global Attributes menu.
#ifdef USE_QTOOLBAR
    a = menubar->addAction("Global Attributes");
    menu = new QMenu();
    a->setMenu(menu);
    tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
    if (tb)
        tb->setPopupMode(QToolButton::InstantPopup);
#else
    menu = menubar->addMenu(tr("Global &Attributes"));
#endif
    connect(menu, &QMenu::triggered,
        this, &QTlayerParamDlg::global_menu_slot);
    // BoxLineStyle, lpBoxLineStyle, 0
    a = menu->addAction("BoxLineStyle");
    a->setData(lpBoxLineStyle);
    // LayerReorderMode, lpLayerReorderMode, 0
    a = menu->addAction("LayerReorderMode");
    a->setData(lpLayerReorderMode);
    // NoPlanarize, lpNoPlanarize, 0
    a = menu->addAction("NoPlanarize");
    a->setData(lpNoPlanarize);
    // AntennaTotal, lpAntennaTotal, 0
    a = menu->addAction("AntennaTotal");
    a->setData(lpAntennaTotal);
    // SubstrateEps, lpSubstrateEps, 0
    a = menu->addAction("SubstrateEps");
    a->setData(lpSubstrateEps);
    // SubstrateThickness, 0, lpSubstrateThickness, 0
    a = menu->addAction("SubstrateThickness");
    a->setData(lpSubstrateThickness);

    // Help Menu.
#ifdef USE_QTOOLBAR
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    menubar->addAction(tr("&Help"), Qt::CTRL|Qt::Key_H, this,
        &QTlayerParamDlg::help_slot()));
#else
    a = menubar->addAction(tr("&Help"), this, &QTlayerParamDlg::help_slot);
    a->setShortcut(QKeySequence("Ctrl+H"));
#endif
#else
    menu = menubar->addMenu(tr("&Help"));
    a = menu->addAction(tr("&Help"), this, &QTlayerParamDlg::help_slot);
    a->setShortcut(QKeySequence("Ctrl+H"));
#endif

    // End of menus.

    QMargins qmtop(2, 2, 2, 2);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);
    vbox->addWidget(menubar);

    QTabWidget *nbook = new QTabWidget();
    vbox->addWidget(nbook);
    nbook->setMaximumHeight(50);
    connect(nbook, &QTabWidget::currentChanged,
        this, &QTlayerParamDlg::page_changed_slot);

    QWidget *page = new QWidget();
    nbook->addTab(page, tr("Layer"));
    QVBoxLayout *vb = new QVBoxLayout(page);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    QLabel *label = new QLabel(tr("Edit basic parameters for layer"));
    vb->addWidget(label);

    page = new QWidget();
    nbook->addTab(page, tr("Extract"));
    vb = new QVBoxLayout(page);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    label = new QLabel(tr("Edit extraction parameters for layer"));
    vb->addWidget(label);

    page = new QWidget();
    nbook->addTab(page, tr("Physical"));
    vb = new QVBoxLayout(page);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    label = new QLabel(tr("Edit physical attributes of layer"));
    vb->addWidget(label);

    page = new QWidget();
    nbook->addTab(page, tr("Convert"));
    vb = new QVBoxLayout(page);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);

    label = new QLabel(tr("Edit conversion parameters of layer"));
    vb->addWidget(label);

    lp_text = new QTtextEdit();
    vbox->addWidget(lp_text);
    lp_text->setReadOnly(true);
    lp_text->setMouseTracking(true);
    connect(lp_text, &QTtextEdit::press_event,
        this, &QTlayerParamDlg::mouse_press_slot);

    QGroupBox *gb = new QGroupBox();
    vbox->addWidget(gb);
    QHBoxLayout *hbox = new QHBoxLayout(gb);
    hbox->setContentsMargins(qmtop);
    hbox->setSpacing(2);

    lp_label = new QLabel("");
    hbox->addWidget(lp_label);

    if (lp_undo)
        lp_undo->setEnabled(false);

    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_FIXED))
        lp_text->setFont(*fnt);
    connect(QTfont::self(), &QTfont::fontChanged,
        this, &QTlayerParamDlg::font_changed_slot, Qt::QueuedConnection);

    update(msg, string);
}


QTlayerParamDlg::~QTlayerParamDlg()
{
    instPtr = 0;

    if (lp_page == LayerPage) {
        if (lp_lyr_kw.is_editing())
            PL()->AbortEdit();
    }
    else if (lp_page == ExtractPage) {
        if (lp_ext_kw.is_editing())
            PL()->AbortEdit();
    }
    else if (lp_page == PhysicalPage) {
        if (lp_phy_kw.is_editing())
            PL()->AbortEdit();
    }
    else if (lp_page == ConvertPage) {
        if (lp_cvt_kw.is_editing())
            PL()->AbortEdit();
    }
    if (lp_caller)
        QTdev::Deselect(lp_caller);
}


QSize
QTlayerParamDlg::sizeHint() const
{
    int fw, fh;
    QTfont::stringBounds(0, FNT_FIXED, &fw, &fh);
    return (QSize(80*fw + 4, 16*fh));
}


void
QTlayerParamDlg::update(const char *msg, const char *string)
{
    if (lp_ldesc != LT()->CurLayer() || !lp_in_callback) {
        load(string);
        lp_ldesc = LT()->CurLayer();
    }

    if (lp_page == LayerPage) {
        if (DSP()->CurMode() == Physical) {
            lp_nmrg->setVisible(true);
            lp_xthk->setVisible(true);
            lp_wira->setVisible(false);
            lp_noiv->setVisible(false);
            lp_ivis->setEnabled(true);
        }
        else {
            lp_nmrg->setVisible(false);
            lp_xthk->setVisible(false);
            lp_wira->setVisible(true);
            lp_noiv->setVisible(true);
            lp_ivis->setEnabled(!LT()->CurLayer()->isWireActive());
        }
    }
    else if (lp_page == ExtractPage) {
        if (DSP()->CurMode() == Physical) {
            lp_ext_menu->menuAction()->setEnabled(true);
            if (lp_ldesc) {
                char *s = Tech()->ExtCheckLayerKeywords(lp_ldesc);
                if (s) {
                    DSPmainWbag(PopUpInfo(MODE_ON, s))
                    delete [] s;
                }
            } 
            lp_darkfield->setEnabled(true);

            if (lp_ldesc) {
                if (lp_ldesc->isConductor()) {
                    if (lp_ldesc->isGroundPlane()) {
                        if (lp_ldesc->isDarkField())
                            lp_darkfield->setEnabled(false);
                    }
                }
            }
        }
        else {
            lp_ext_menu->menuAction()->setEnabled(false);
        }
    }
    else if (lp_page == PhysicalPage) {
        if (DSP()->CurMode() == Physical) {
            lp_phy_menu->menuAction()->setEnabled(true);
            lp_rho->setEnabled(true);
            lp_sigma->setEnabled(true);
            lp_rsh->setEnabled(true);
            lp_tau->setEnabled(true);
            lp_epsrel->setEnabled(true);
            lp_cap->setEnabled(true);
            lp_lambda->setEnabled(true);
            lp_tline->setEnabled(true);
            lp_antenna->setEnabled(true);

            if (lp_ldesc) {
                if (lp_ldesc->isConductor()) {
                    lp_epsrel->setEnabled(false);
                    if (lp_ldesc->isGroundPlane()) {
                        lp_cap->setEnabled(false);
                        lp_tline->setEnabled(false);
                    }
                    if (!lp_ldesc->isRouting())
                        lp_antenna->setEnabled(false);
                }
                else if (lp_ldesc->isVia() || lp_ldesc->isViaCut()) {
                    lp_rho->setEnabled(false);
                    lp_sigma->setEnabled(false);
                    lp_rsh->setEnabled(false);
                    lp_tau->setEnabled(false);
                    lp_cap->setEnabled(false);
                    lp_lambda->setEnabled(false);
                    lp_tline->setEnabled(false);
                    lp_antenna->setEnabled(false);
                }
                else {
                    TechLayerParams *lp = tech_prm(lp_ldesc);
                    if (lp->epsrel() > 0.0) {
                        lp_rho->setEnabled(false);
                        lp_sigma->setEnabled(false);
                        lp_rsh->setEnabled(false);
                        lp_tau->setEnabled(false);
                        lp_cap->setEnabled(false);
                        lp_lambda->setEnabled(false);
                        lp_tline->setEnabled(false);
                        lp_antenna->setEnabled(false);
                    }
                    else if (lp->rho() > 0.0 || lp->ohms_per_sq() > 0.0 ||
                            lp->tau() > 0.0 || lp->cap_per_area() > 0.0 ||
                            lp->cap_per_perim() > 0.0 ||
                            lp->lambda() > 0.0 || lp->gp_lname() ||
                            lp->ant_ratio() > 0.0)
                        lp_epsrel->setEnabled(false);
                }
            }
        }
        else {
            lp_phy_menu->menuAction()->setEnabled(false);
        }
    }
    else if (lp_page == ConvertPage) {
        lp_nddt->setEnabled((DSP()->CurMode() == Physical));
    }
    lp_label->setText(msg);
}


void
QTlayerParamDlg::insert(const char *str, const char *l1, const char *l2)
{
    if (lp_undo)
        lp_undo->setEnabled(false);

    char *status = 0;
    if (lp_page == LayerPage)
        status = lp_lyr_kw.insert_keyword_text(str, 0, 0);
    else if (lp_page == ExtractPage)
        status = lp_ext_kw.insert_keyword_text(str, l1, l2);
    else if (lp_page == PhysicalPage)
        status = lp_phy_kw.insert_keyword_text(str, 0, 0);
    else if (lp_page == ConvertPage)
        status = lp_cvt_kw.insert_keyword_text(str, l1, 0);

    update();
    if (status) {
        PL()->ShowPrompt(status);
        delete [] status;
    }
    else
        PL()->ErasePrompt();
}


// Undo the last operation.
//
void
QTlayerParamDlg::undoop()
{
    if (lp_page == LayerPage)
        lp_lyr_kw.undo_keyword_change();
    else if (lp_page == ExtractPage)
        lp_ext_kw.undo_keyword_change();
    else if (lp_page == PhysicalPage)
        lp_phy_kw.undo_keyword_change();
    else if (lp_page == ConvertPage)
        lp_cvt_kw.undo_keyword_change();

    if (lp_undo)
        lp_undo->setEnabled(false);
    update();
}


void
QTlayerParamDlg::load(const char *string)
{
    if (lp_undo)
        lp_undo->setEnabled(false);

    if (lp_page == LayerPage)
        lp_lyr_kw.load_keywords(LT()->CurLayer(), string);
    else if (lp_page == ExtractPage)
        lp_ext_kw.load_keywords(LT()->CurLayer(), string);
    else if (lp_page == PhysicalPage)
        lp_phy_kw.load_keywords(LT()->CurLayer(), string);
    else if (lp_page == ConvertPage)
        lp_cvt_kw.load_keywords(LT()->CurLayer(), string);

    update();
}


void
QTlayerParamDlg::clear_undo()
{
    if (lp_page == LayerPage)
        lp_lyr_kw.clear_undo_list();
    else if (lp_page == ExtractPage)
        lp_ext_kw.clear_undo_list();
    else if (lp_page == PhysicalPage)
        lp_phy_kw.clear_undo_list();
    else if (lp_page == ConvertPage)
        lp_cvt_kw.clear_undo_list();

    if (lp_undo)
        lp_undo->setEnabled(false);
}


void
QTlayerParamDlg::update()
{
    select_range(0, 0);
    stringlist *list = 0;

    if (lp_page == LayerPage) {
        list = lp_lyr_kw.list();
        lp_lyr_kw.sort();
    }
    else if (lp_page == ExtractPage) {
        list = lp_ext_kw.list();
        lp_ext_kw.sort();
    }
    else if (lp_page == PhysicalPage) {
        list = lp_phy_kw.list();
        lp_phy_kw.sort();
    }
    else if (lp_page == ConvertPage) {
        list = lp_cvt_kw.list();
        lp_cvt_kw.sort();
    }

    QColor c = QTbag::PopupColor(GRattrColorHl4);
    double val = lp_text->get_scroll_value();
    lp_text->clear();
    for (stringlist *l = list; l; l = l->next) {
        char *tmp = lstring::copy(l->string);
        char *t = tmp;
        while (*t && !isspace(*t))
            t++;
        char ch = *t;
        *t = 0;
        lp_text->setTextColor(c);
        lp_text->insertPlainText(tmp);
        *t = ch;
        if (*t) {
            lp_text->setTextColor(QColor("black"));
            lp_text->insertPlainText(t);
        }
        lp_text->insertPlainText("\n");
        delete [] tmp;
    }
    lp_text->set_scroll_value(val);

    if (lp_page == LayerPage) {
        if (lp_undo && (lp_lyr_kw.undo_list() || lp_lyr_kw.new_string()))
            lp_undo->setEnabled(true);
    }
    else if (lp_page == ExtractPage) {
        if (lp_undo && (lp_ext_kw.undo_list() || lp_ext_kw.new_string()))
            lp_undo->setEnabled(true);
    }
    else if (lp_page == PhysicalPage) {
        if (lp_undo && (lp_phy_kw.undo_list() || lp_phy_kw.new_string()))
            lp_undo->setEnabled(true);
    }
    else if (lp_page == ConvertPage) {
        if (lp_undo && (lp_cvt_kw.undo_list() || lp_cvt_kw.new_string()))
            lp_undo->setEnabled(true);
    }
    check_sens();
}


void
QTlayerParamDlg::call_callback(const char *before)
{
    char *after = lp_text->get_chars();
    if (strcmp(before, after)) {
        lp_in_callback = true;
        char *err = 0;
        if (lp_page == LayerPage)
            err = lyrKWstruct::set_settings(LT()->CurLayer(), after);
        else if (lp_page == ExtractPage)
            err = extKWstruct::set_settings(LT()->CurLayer(), after);
        else if (lp_page == PhysicalPage)
            err = phyKWstruct::set_settings(LT()->CurLayer(), after);
        else if (lp_page == ConvertPage)
            err = cvtKWstruct::set_settings(LT()->CurLayer(), after);
        if (!instPtr)
            return;
        if (!err) {
            err = lstring::copy(
                "Succeeded - keywords attached to current layer");
        }
        update(err, 0);
        delete [] err;
        lp_in_callback = false;
    }
    delete [] after;
}


// Select the chars in the range, start=end deselects existing.  In
// GTK-1, selecting gives blue inverse, which turns gray if
// unselected, retaining an indication for the buttons.  GTK-2
// doesn't do this automatically so we provide something similar here.
//
void
QTlayerParamDlg::select_range(int start, int end)
{
    lp_text->select_range(start, end);
    lp_start = start;
    lp_end = end;
    check_sens();
}


void
QTlayerParamDlg::check_sens()
{
    char *s = lp_text->get_selection();
    if (lp_edit) {
        if (lp_page == LayerPage) {
            if (s) {
                // Don't turn on editing for booleans.
                if (lstring::prefix(Tkw.Symbolic(), s) ||
                        lstring::prefix(Tkw.NoMerge(), s) ||
                        lstring::prefix(Tkw.Invisible(), s) ||
                        lstring::prefix(Tkw.NoSelect(), s) ||
                        lstring::prefix(Tkw.WireActive(), s) ||
                        lstring::prefix(Tkw.NoInstView(), s))
                    lp_edit->setEnabled(false);
                else
                    lp_edit->setEnabled(true);
            }
            else
                lp_edit->setEnabled(false);
        }
        else if (lp_page == ExtractPage) {
            if (s) {
                if (lstring::prefix(Ekw.GroundPlane(), s) ||
                        lstring::prefix(Ekw.DarkField(), s))
                    lp_edit->setEnabled(false);
                else
                    lp_edit->setEnabled(true);
            }
            else
                lp_edit->setEnabled(false);
        }       
        else if (lp_page == PhysicalPage) {
            if (s)
                lp_edit->setEnabled(true);
            else
                lp_edit->setEnabled(false);
        }       
        else if (lp_page == ConvertPage) {
            if (s)
                lp_edit->setEnabled(false);
            else
                lp_edit->setEnabled(true);
        }
    }

    if (lp_del) {
        if (s)
            lp_del->setEnabled(true);
        else
            lp_del->setEnabled(false);
    }
    if (!s)
        lp_line_selected = -1;
    delete [] s;
}


void
QTlayerParamDlg::edit_menu_slot(QAction *a)
{
    int atype = a->data().toInt();
    if (atype == edEdit) {
        // Edit the selected entry.
        int start, end;
        start = lp_start;
        end = lp_end;
        if (start == end)
            return;
        char *string = lp_text->get_chars(start, end);
        char *nl = string + strlen(string) - 1;
        if (*nl == '\n')
            *nl = 0;

        int ktype = -1;
        char *nstring = 0;
        if (lp_page == LayerPage) {
            ktype = lyrKWstruct::kwtype(string);
            nstring = lp_lyr_kw.get_string_for(ktype, string);
            if (nstring) {
                char *before = lp_text->get_chars();
                insert(nstring, 0, 0);
                delete [] nstring;
                call_callback(before);
                delete [] before;
            }
        }
        else if (lp_page == ExtractPage) {
            ktype = extKWstruct::kwtype(string);
            nstring = lp_ext_kw.get_string_for(ktype, string);
            if (nstring) {
                char *before = lp_text->get_chars();
     
                // The Via keyword is the only type which can appear more
                // than once.  The layer reference names are passed to
                // distinguish the entry.

                if (ktype == exVia) {
                    char *s = string;
                    lstring::advtok(&s);
                    char *ln1 = lstring::gettok(&s);
                    char *ln2 = lstring::gettok(&s);
                    insert(nstring, ln1, ln2);
                    delete [] ln1;
                    delete [] ln2;
                }
                else
                    insert(nstring, 0, 0);
                delete [] nstring;
                call_callback(before);
                delete [] before;
            }
        }
        else if (lp_page == PhysicalPage) {
            ktype = phyKWstruct::kwtype(string);
            nstring = lp_phy_kw.get_string_for(ktype, string);
            if (nstring) {
                char *before = lp_text->get_chars();
                insert(nstring, 0, 0);
                delete [] nstring;
                call_callback(before);
                delete [] before;
            }
        }
        else if (lp_page == ConvertPage) {
            ktype = cvtKWstruct::kwtype(string);
            nstring = lp_cvt_kw.get_string_for(ktype, string);
            if (nstring) {
                char *before = lp_text->get_chars();
                insert(nstring, string, 0);
                delete [] nstring;
                call_callback(before);
                delete [] before;
            }
        }
        delete [] string;
    }
    else if (atype == edDelete) {
        // Delete the selected entry.
        int start, end;
        start = lp_start;
        end = lp_end;
        if (start == end)
            return;
        char *string = lp_text->get_chars(start, end);
        char *nl = string + strlen(string) - 1;
        if (*nl == '\n')
            *nl = 0;

        if (lp_page == LayerPage) {
            int ktype = lyrKWstruct::kwtype(string);
            if (ktype != lpNil) {
                char *before = lp_text->get_chars();
                clear_undo();
                lp_lyr_kw.remove_keyword_text(ktype);
                update();
                call_callback(before);
                delete [] before;
            }
        }
        else if (lp_page == ExtractPage) {
            int ktype = extKWstruct::kwtype(string);
            if (ktype != exNil) {
                char *before = lp_text->get_chars();
                clear_undo();

                // The Via keyword is the only type which can appear
                // more than once.  The layer reference names are
                // passed to distinguish the entry.

                if (ktype == exVia) {
                    char *s = string;
                    lstring::advtok(&s);
                    char *ln1 = lstring::gettok(&s);
                    char *ln2 = lstring::gettok(&s);
                    lp_ext_kw.remove_keyword_text(ktype, false, ln1, ln2);
                    delete [] ln1;
                    delete [] ln2;
                }
                else
                    lp_ext_kw.remove_keyword_text(ktype);
                update();
                call_callback(before);
                delete [] before;
            }
        }
        else if (lp_page == PhysicalPage) {
            int ktype = phyKWstruct::kwtype(string);
            if (ktype != phNil) {
                char *before = lp_text->get_chars();
                clear_undo();
                lp_phy_kw.remove_keyword_text(ktype);
                update();
                call_callback(before);
                delete [] before;
            }
        }
        else if (lp_page == ConvertPage) {
            int ktype = cvtKWstruct::kwtype(string);
            if (ktype != cvNil) {
                char *before = lp_text->get_chars();
                clear_undo();
                lp_cvt_kw.remove_keyword_text(ktype, false, string);
                update();
                call_callback(before);
                delete [] before;
            }
        }
        delete [] string;
    }
    else if (atype == edUndo) {
        // Undo the last operation.
        char *before = lp_text->get_chars();
        undoop();
        call_callback(before);
        delete [] before;
    }
    else if (atype == edQuit) {
        XM()->PopUpLayerParamEditor(0, MODE_OFF, 0, 0);
    }
}


void
QTlayerParamDlg::layer_menu_slot(QAction *a)
{
    int ktype = a->data().toInt();
    char *string = lp_lyr_kw.get_string_for(ktype, 0);

    if (string) {
        char *before = lp_text->get_chars();
        insert(string, 0, 0);
        call_callback(before);
        delete [] before;
    }
}


void
QTlayerParamDlg::extract_menu_slot(QAction *a)
{
    int ktype = a->data().toInt();
    char *string = lp_ext_kw.get_string_for(ktype, 0);

    if (string) {
        char *before = lp_text->get_chars();
        if (lp_page == ExtractPage && ktype == exVia) {
            // Pass two dummy via strings so that existing Via
            // keywords are not deleted.

            insert(string, "", "");
        }
        else
            insert(string, 0, 0);
        call_callback(before);
        delete [] before;
    }
}


void
QTlayerParamDlg::physical_menu_slot(QAction *a)
{
    int ktype = a->data().toInt();
    char *string = lp_phy_kw.get_string_for(ktype, 0);

    if (string) {
        char *before = lp_text->get_chars();
        insert(string, 0, 0);
        call_callback(before);
        delete [] before;
    }
}


void
QTlayerParamDlg::convert_menu_slot(QAction *a)
{
    int ktype = a->data().toInt();
    char *string = lp_cvt_kw.get_string_for(ktype, 0);

    if (string) {
        char *before = lp_text->get_chars();
        insert(string, 0, 0);
        call_callback(before);
        delete [] before;
    }
}


void
QTlayerParamDlg::global_menu_slot(QAction *a)
{
    int ktype = a->data().toInt();
    char tbuf[64];
    if (ktype == lpBoxLineStyle) {
        snprintf(tbuf, sizeof(tbuf), "0x%x", DSP()->BoxLinestyle()->mask);
        char *in = lp_ext_kw.prompt(
            "Enter highlighting box linestyle mask: ", tbuf);
        if (in) {
            if (*in)
                CDvdb()->setVariable(VA_BoxLineStyle, in);
            else
                CDvdb()->clearVariable(VA_BoxLineStyle);
            PL()->ErasePrompt();
        }
    }
    else if (ktype == lpLayerReorderMode) {
        snprintf(tbuf, sizeof(tbuf), "%d", Tech()->ReorderMode());
        char *in = lp_ext_kw.prompt(
            "Enter Via layer reordering mode (integer 0-2): ", tbuf);
        unsigned int d;
        if (in) {
            if (sscanf(in, "%u", &d) == 1 &&  d <= 2) {
                if (d > 0)
                    CDvdb()->setVariable(VA_LayerReorderMode, in);
                else
                    CDvdb()->clearVariable(VA_LayerReorderMode);
                PL()->ErasePrompt();
            }
            else
                PL()->ShowPrompt("Bad value, must be integer 0-2 ");
        }
    }
    else if (ktype == lpNoPlanarize) {
        char *in;
        if (Tech()->IsNoPlanarize()) {
            in = lp_ext_kw.prompt(
                "NoPlanarize is currently set, press Enter to unset, "
                "Esc to abort. ", "");
            if (in)
                CDvdb()->clearVariable(VA_NoPlanarize);
        }
        else {
            in = lp_ext_kw.prompt(
                "NoPlanarize is currently unset, press Enter to set. "
                "Esc to abort. ", "");
            if (in)
                CDvdb()->setVariable(VA_NoPlanarize, "");
        }
    }
    else if (ktype == lpAntennaTotal) {
        snprintf(tbuf, sizeof(tbuf), "%g",
            Tech()->AntennaTotal());
        char *in = lp_ext_kw.prompt(
            "Enter total net antenna ratio limit: ", tbuf);
        double d;
        if (in) {
            if (sscanf(in, "%lf", &d) == 1 &&  d >= 0.0) {
                if (d > 0.0)
                    CDvdb()->setVariable(VA_AntennaTotal, in);
                else
                    CDvdb()->clearVariable(VA_AntennaTotal);
                PL()->ErasePrompt();
            }
            else
                PL()->ShowPrompt("Bad value, must be >= 0.0");
        }
    }
    else if (ktype == lpSubstrateEps) {
        snprintf(tbuf, sizeof(tbuf), "%g", Tech()->SubstrateEps());
        char *in = lp_ext_kw.prompt(
            "Enter substrate relative dielectric constant: ", tbuf);
        double d;
        if (in) {
            if (sscanf(in, "%lf", &d) == 1 && d >= SUBSTRATE_EPS_MIN &&
                    d <= SUBSTRATE_EPS_MAX) {
                if (d != SUBSTRATE_EPS)
                    CDvdb()->setVariable(VA_SubstrateEps, in);
                else
                    CDvdb()->clearVariable(VA_SubstrateEps);
                PL()->ErasePrompt();
            }
            else
                PL()->ShowPromptV("Bad value, must be %.3f - %.3f",
                    SUBSTRATE_EPS_MIN, SUBSTRATE_EPS_MAX);
        }
    }
    else if (ktype == lpSubstrateThickness) {
        snprintf(tbuf, sizeof(tbuf), "%g", Tech()->SubstrateThickness());
        char *in = lp_ext_kw.prompt(
            "Enter substrate thickness in microns: ", tbuf);
        double d;
        if (in) {
            if (sscanf(in, "%lf", &d) == 1 && d >= SUBSTRATE_THICKNESS_MIN &&
                    d <= SUBSTRATE_THICKNESS_MAX) {
                if (d != SUBSTRATE_THICKNESS)
                    CDvdb()->setVariable(VA_SubstrateThickness, in);
                else
                    CDvdb()->clearVariable(VA_SubstrateThickness);
                PL()->ErasePrompt();
            }
            else
                PL()->ShowPromptV("Bad value, must be %.3f - %.3f",
                    SUBSTRATE_THICKNESS_MIN, SUBSTRATE_THICKNESS_MAX);
        }
    }
}


void
QTlayerParamDlg::help_slot()
{
    DSPmainWbag(PopUpHelp("xic:lpedt"))
}


void
QTlayerParamDlg::page_changed_slot(int page)
{
    switch (lp_page) {
        case LayerPage:
            lp_lyr_menu->menuAction()->setVisible(false);
            break;
        case ExtractPage:
            lp_ext_menu->menuAction()->setVisible(false);
            break;
        case PhysicalPage:
            lp_phy_menu->menuAction()->setVisible(false);
            break;
        case ConvertPage:
            lp_cvt_menu->menuAction()->setVisible(false);
            break;
    }
    lp_page = page;
    switch (lp_page) {
        case LayerPage:
            lp_lyr_menu->menuAction()->setVisible(true);
            break;
        case ExtractPage:
            lp_ext_menu->menuAction()->setVisible(true);
            break;
        case PhysicalPage:
            lp_phy_menu->menuAction()->setVisible(true);
            break;
        case ConvertPage:
            lp_cvt_menu->menuAction()->setVisible(true);
            break;
    }
    if (lp_text)
        update(0, 0);
}


void
QTlayerParamDlg::mouse_press_slot(QMouseEvent *ev)
{
    // Handle button presses in the text area.  If neither edit mode or
    // delete mode is active, highlight the line pointed to.  Otherwise,
    // perform the operation on the pointed-to line.
    //
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    int vsv = lp_text->verticalScrollBar()->value();
    int hsv = lp_text->horizontalScrollBar()->value();
    select_range(0, 0);

    char *str = lp_text->get_chars();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    int posn = lp_text->document()->documentLayout()->hitTest(
        QPointF(xx + hsv, yy + vsv), Qt::ExactHit);
    
    if (isspace(str[posn])) {
        // Clicked on white space.
        delete [] str;
        return;
    }

    const char *lineptr = str;
    int linecnt = 0;
    for (int i = 0; i <= posn; i++) {
        if (str[i] == '\n') {
            if (i == posn) {
                // Clicked to right of line.
                delete [] str;
                return;
            }
            lineptr = str + i+1;
            linecnt++;
        }
    }
    if (lp_line_selected >= 0 && lp_line_selected == linecnt) {
        delete [] str;
        select_range(0, 0);
        return;
    }
    lp_line_selected = linecnt;
    int start = (lineptr - str);

    // find the end of line
    const char *s = lineptr;
    for ( ; *s && *s != '\n'; s++) ;

    select_range(start, start + (s - lineptr));
    // Don't let the scroll position change.
    lp_text->verticalScrollBar()->setValue(vsv);
    lp_text->horizontalScrollBar()->setValue(hsv);
    delete [] str;
}


void
QTlayerParamDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_FIXED))
            lp_text->setFont(*fnt);
        update();
    }
}

