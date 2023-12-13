
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

#include "qtcolor.h"
#include "scedif.h"
#include "cd_lgen.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "layertab.h"
#include "select.h"
#include "qtparam.h"
#include "qtcoord.h"
#include "qthtext.h"
#include "qtinterf/qtfont.h"
#include "miscutil/pathlist.h"

#include <QLayout>
#include <QColorDialog>
#include <QComboBox>
#include <QPushButton>


#define CLR_CURLYR  -1

// Menu function to display, update, or destroy the color popup.
//
void
cMain::PopUpColor(GRobject caller, ShowMode mode)
{
    (void)caller;
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        delete QTcolorDlg::self();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTcolorDlg::self())
            QTcolorDlg::self()->update();
        return;
    }
    if (QTcolorDlg::self())
        return;

    new QTcolorDlg(caller);

    QTcolorDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_LL), QTcolorDlg::self(),
        QTmainwin::self()->Viewport());
    QTcolorDlg::self()->show();
}


//
// The menus for attribute colors.
//

// Attributes, electrical and physical mode.
//
QTcolorDlg::clritem QTcolorDlg::Menu1[] =
{
    { "Current Layer", CLR_CURLYR },
    { "Background", BackgroundColor },
    { "Coarse Grid", CoarseGridColor },
    { "Fine Grid", FineGridColor },
    { "Ghosting", GhostColor },
    { "Highlighting", HighlightingColor },
    { "Selection Color 1", SelectColor1 },
    { "Selection Color 2", SelectColor2 },
    { "Terminals", MarkerColor },
    { "Instance Boundary", InstanceBBColor },
    { "Instance Name Text", InstanceNameColor },
    { "Instance Size Text", InstanceSizeColor },
    { 0, 0}
};

// Prompt line, electrical and physical mode.
//
QTcolorDlg::clritem QTcolorDlg::Menu2[] =
{
    { "Text", PromptEditTextColor },
    { "Prompt Text", PromptTextColor },
    { "Highlight Text", PromptHighlightColor },
    { "Cursor", PromptCursorColor },
    { "Background", PromptBackgroundColor },
    { "Edit Background", PromptEditBackgColor },
    { "Focussed Background", PromptEditFocusBackgColor },
    { 0, 0 }
};

// Plot mark colors, electrical only.
//
QTcolorDlg::clritem QTcolorDlg::Menu3[] =
{
    { "Plot Mark 1", Color2 },
    { "Plot Mark 2", Color3 },
    { "Plot Mark 3", Color4 },
    { "Plot Mark 4", Color5 },
    { "Plot Mark 5", Color6 },
    { "Plot Mark 6", Color7 },
    { "Plot Mark 7", Color8 },
    { "Plot Mark 8", Color9 },
    { "Plot Mark 9", Color10 },
    { "Plot Mark 10", Color11 },
    { "Plot Mark 11", Color12 },
    { "Plot Mark 12", Color13 },
    { "Plot Mark 13", Color14 },
    { "Plot Mark 14", Color15 },
    { "Plot Mark 15", Color16 },
    { "Plot Mark 17", Color17 },
    { "Plot Mark 17", Color18 },
    { "Plot Mark 18", Color19 },
    { 0, 0 }
};

QTcolorDlg *QTcolorDlg::instPtr;

QTcolorDlg::QTcolorDlg(GRobject c)
{
    instPtr = this;
    c_caller = c;
    c_modemenu = 0;
    c_categmenu = 0;
    c_attrmenu = 0;
    c_clrd = 0;
    c_listbtn = 0;
    c_listpop = 0;
    c_display_mode = DSP()->CurMode();
    c_ref_mode = DSP()->CurMode();
    c_mode = CLR_CURLYR;
    c_red = 0;
    c_green = 0;
    c_blue = 0;

    setWindowTitle(tr("Color Selection"));
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

    // Physical/Electrical mode selector
    //
    c_modemenu = new QComboBox();
    c_modemenu->addItem(tr("Physical"));
    c_modemenu->addItem(tr("Electrical"));
    c_modemenu->setCurrentIndex(c_display_mode == Physical ? 0 : 1);
    hbox->addWidget(c_modemenu);
    connect(c_modemenu, SIGNAL(currentIndexChanged(int)),
        this, SLOT(mode_menu_change_slot(int)));
    if (ScedIf()->hasSced())
        c_modemenu->show();
    else
        c_modemenu->hide();

    // Categories menu
    //
    c_categmenu = new QComboBox();
    fill_categ_menu();
    hbox->addWidget(c_categmenu);
    connect(c_categmenu, SIGNAL(currentIndexChanged(int)),
        this, SLOT(categ_menu_change_slot(int)));

    // Attribute selection menu
    //
    c_attrmenu = new QComboBox();
    fill_attr_menu(CATEG_ATTR);
    hbox->addWidget(c_attrmenu);
    connect(c_attrmenu, SIGNAL(currentIndexChanged(int)),
        this, SLOT(attr_menu_change_slot(int)));

    c_clrd = new QColorDialog();
    c_clrd->setWindowFlags(Qt::Widget);
    c_clrd->setOptions(QColorDialog::DontUseNativeDialog |
        QColorDialog::NoButtons);
    vbox->addWidget(c_clrd);
    connect(c_clrd, SIGNAL(currentColorChanged(const QColor&)),
        this, SLOT(color_selected_slot(const QColor&)));

    hbox = new QHBoxLayout(0);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    // Help, Colors, Apply, and Dismiss buttons.
    //
    QPushButton *btn = new QPushButton(tr("Help"));
    hbox->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    c_listbtn = new QPushButton(tr("Named Colors"));
    hbox->addWidget(c_listbtn);
    c_listbtn->setCheckable(true);
    c_listbtn->setAutoDefault(false);
    connect(c_listbtn, SIGNAL(toggled(bool)),
        this, SLOT(colors_btn_slot(bool)));

    btn = new QPushButton(tr("Apply"));
    connect(btn, SIGNAL(clicked()), this, SLOT(apply_btn_slot()));
    hbox->addWidget(btn);

    btn = new QPushButton(tr("Dismiss"));
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));
    hbox->addWidget(btn);

    update_color();
}


QTcolorDlg::~QTcolorDlg()
{
    instPtr = 0;
    if (c_caller)
        QTdev::Deselect(c_caller);
    if (c_listpop)
        c_listpop->popdown();
}


void
QTcolorDlg::update()
{
    if (c_ref_mode != DSP()->CurMode()) {
        // User changed between Physical and Electrical modes. 
        // Rebuild the ATTR menu if current, since the visibility of
        // the Current Layer item will have changed.
        //
        if (c_categmenu->currentIndex() == CATEG_ATTR)
            fill_attr_menu(CATEG_ATTR);
        c_ref_mode = DSP()->CurMode();
    }
    else {
        // The current layer changed.  The second term in the
        // conditional is redundant, for sanity.
        //
        if (c_mode == CLR_CURLYR && c_display_mode == DSP()->CurMode())
            update_color();
    }
}


// Update the color shown.
//
void
QTcolorDlg::update_color()
{
    if (!c_clrd)
        return;
    int r, g, b;
    c_get_rgb(c_mode, &r, &g, &b);
    QColor rgb(r, g, b);
    c_clrd->setCurrentColor(rgb);
//    set_sample_bg();
}


void
QTcolorDlg::fill_categ_menu()
{
    c_categmenu->clear();
    c_categmenu->addItem(tr("Attributes"));
    c_categmenu->addItem(tr("Prompt Line"));
    if (c_display_mode == Electrical)
        c_categmenu->addItem(tr("Plot Marks"));
    c_categmenu->setCurrentIndex(0);
}


void
QTcolorDlg::fill_attr_menu(int categ)
{
    if (categ == CATEG_ATTR) {
        c_attrmenu->clear();
        for (clritem *c = Menu1; c->descr; c++) {
            // We want the Current Layer entry only when the mode
            // matches the display mode, since the current layer
            // of the "opposite" mode is not known here.
            //
            if (c == Menu1 && c_display_mode != DSP()->CurMode())
                continue;
            c_attrmenu->addItem(tr(c->descr), c->tab_indx);
        }
        if (c_display_mode == DSP()->CurMode())
            c_mode = Menu1[0].tab_indx;
        else
            c_mode = Menu1[1].tab_indx;
    }
    else if (categ == CATEG_PROMPT) {
        c_attrmenu->clear();
        for (clritem *c = Menu2; c->descr; c++) {
            c_attrmenu->addItem(tr(c->descr), c->tab_indx);
        }
        c_mode = Menu2[0].tab_indx;
    }
    else if (categ == CATEG_PLOT) {
        c_attrmenu->clear();
        for (clritem *c = Menu3; c->descr; c++) {
            c_attrmenu->addItem(tr(c->descr), c->tab_indx);
        }
        c_mode = Menu3[0].tab_indx;
    }
    else
        return;
    c_attrmenu->setCurrentIndex(0);
    update_color();
}


// Static function.
// Return the RGB for the current mode.
//
void
QTcolorDlg::c_get_rgb(int mode, int *red, int *green, int *blue)
{
    if (mode == CLR_CURLYR) {
        if (LT()->CurLayer())
            LT()->GetLayerColor(LT()->CurLayer(), red, green, blue);
        else {
            *red = 0;
            *green = 0;
            *blue = 0;
        }
    }
    else {
        if (QTcolorDlg::self()) {
            DSP()->ColorTab()->get_rgb(mode, QTcolorDlg::self()->c_display_mode,
                red, green, blue);
        }
        else {
            *red = 0;
            *green = 0;
            *blue = 0;
        }
    }
}


// Static function.
// This used to actually effect color change, now we defer to the
// Apply button.
//
void
QTcolorDlg::c_set_rgb(int red, int green, int blue)
{
    if (!QTcolorDlg::self())
        return;
    QTcolorDlg::self()->c_red = red;
    QTcolorDlg::self()->c_green = green;
    QTcolorDlg::self()->c_blue = blue;
}


// Insert the selected rgb.txt entry into the color selector.
//
void
QTcolorDlg::c_list_callback(const char *string, void*)
{
    if (string) {
        while (isspace(*string))
            string++;
        int r, g, b;
        if (sscanf(string, "%d %d %d", &r, &g, &b) != 3)
            return;
        if (QTcolorDlg::self()) {
            QColor rgb(r, g, b);
            /*
            gtk_color_selection_set_current_color(
                GTK_COLOR_SELECTION(Clr->c_sel), &rgb);
            */
            QTcolorDlg::c_set_rgb(r, g, b);
        }
    }
    else if (QTcolorDlg::self()) {
        QTdev::SetStatus(QTcolorDlg::self()->c_listbtn, false);
        QTcolorDlg::self()->c_listpop = 0;
    }
}


void
QTcolorDlg::mode_menu_change_slot(int ix)
{
    c_display_mode = ix ? Electrical : Physical;
    fill_categ_menu();
    fill_attr_menu(CATEG_ATTR);
}


void
QTcolorDlg::categ_menu_change_slot(int ix)
{
    fill_attr_menu(ix);
}


void
QTcolorDlg::attr_menu_change_slot(int)
{
    int ix = c_attrmenu->currentData().toInt();
    if (ix == Physical || ix == Electrical) {
        c_mode = ix;
        update_color();
    }
}


void
QTcolorDlg::color_selected_slot(const QColor &clr)
{
    int r = clr.red();
    int g = clr.green();
    int b = clr.blue();
    c_set_rgb(r, g, b);
}


void
QTcolorDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("xic:color"))
}


void
QTcolorDlg::colors_btn_slot(bool state)
{
    if (!c_listpop && state) {
        stringlist *list = GRcolorList::listColors();
        if (!list) {
            QTdev::SetStatus(c_listbtn, false);
            return;
        }
        c_listpop = DSPmainWbagRet(PopUpList(list, "Colors",
            "click to select", c_list_callback, 0, false, false));
        stringlist::destroy(list);
        if (c_listpop)
            c_listpop->register_usrptr((void**)&c_listpop);
    }
    else if (c_listpop && !state)
        c_listpop->popdown();
}


void
QTcolorDlg::apply_btn_slot()
{
    // Actually perform the color change.
    int mode = c_mode;
    int red = c_red;
    int green = c_green;
    int blue = c_blue;
    DisplayMode dm = c_display_mode;

    if (mode == CLR_CURLYR) {
        if (LT()->CurLayer() && dm == DSP()->CurMode()) {
            LT()->SetLayerColor(LT()->CurLayer(), red, green, blue);
            LT()->ShowLayerTable(LT()->CurLayer());
            XM()->PopUpFillEditor(0, MODE_UPD);
            XM()->PopUpLayerPalette(0, MODE_UPD, false, 0);

            if (DSP()->CurMode() == Electrical || !LT()->NoPhysRedraw()) {
                DSP()->RedisplayAll();
            }
        }
        return;
    }
    if (mode == BackgroundColor) {
        DSP()->ColorTab()->set_rgb(mode, dm, red, green, blue);
        if (dm == DSP()->CurMode()) {
            LT()->ShowLayerTable(LT()->CurLayer());
            XM()->PopUpFillEditor(0, MODE_UPD);
            XM()->PopUpLayerPalette(0, MODE_UPD, false, 0);
        }
        XM()->UpdateCursor(0, XM()->GetCursor());
        if (dm == Electrical || !LT()->NoPhysRedraw()) {
            DSP()->RedisplayAll(dm);
        }
        return;
    }

    DSP()->ColorTab()->set_rgb(mode, dm, red, green, blue);
    switch (mode) {
    case PromptEditTextColor:
    case PromptTextColor:
    case PromptHighlightColor:
    case PromptCursorColor:
    case PromptBackgroundColor:
    case PromptEditBackgColor:
        QTparam::self()->redraw();
        QTcoord::self()->redraw();
        DSP()->MainWbag()->ShowKeys();
        QTedit::self()->redraw();
        break;

    case CoarseGridColor:
    case FineGridColor:
        if (dm == Electrical || !LT()->NoPhysRedraw()) {
            DSP()->RedisplayAll(dm);
        }
        break;

    case GhostColor:
        XM()->UpdateCursor(0, XM()->GetCursor());
        break;

    case HighlightingColor:
        if (dm == Electrical || !LT()->NoPhysRedraw()) {
            DSP()->RedisplayAll(dm);
        }
        break;

    case SelectColor1:
    case SelectColor2:
        break;

    case MarkerColor:
    case InstanceBBColor:
    case InstanceNameColor:
    case InstanceSizeColor:
        if (dm == Electrical || !LT()->NoPhysRedraw()) {
            DSP()->RedisplayAll(dm);
        }
        break;

    default:
        break;
    }
}


void
QTcolorDlg::dismiss_btn_slot()
{
    XM()->PopUpColor(0, MODE_OFF);
}

