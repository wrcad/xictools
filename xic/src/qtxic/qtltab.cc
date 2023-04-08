
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

#include "config.h"
#include "qtmain.h"
#include "qtltab.h"
#include "qtmenu.h"
#include "qtinterf/qtfont.h"
#include "dsp_inlines.h"
#include "select.h"
#include "events.h"
#include "keymap.h"

#include <unistd.h>
#include <errno.h>

#include <QWidget>
#include <QMouseEvent>
#include <QLayout>
#include <QPushButton>
#include <QScrollBar>

#include "qtinlines.h"

// help keywords used:
//  layertab
//  layerchange


//--------------------------------------------------------------------
//
// The Layer Table
//
//--------------------------------------------------------------------

layertab_w::layertab_w(int ht, QWidget *prnt) : QWidget(prnt)
{
    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->setMargin(0);
    vb->setSpacing(0);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(2);

    lspec_btn = new QPushButton(this);
    lspec_btn->setAutoDefault(false);
    lspec_btn->setCheckable(true);
    lspec_btn->setChecked(Selections.layer_specific);
    lspec_btn->setText(QString("S"));
    QFontMetrics fm(lspec_btn->font());
    int bw = fm.width(lspec_btn->text()) + 6;
    lspec_btn->setFixedSize(bw, ht);
    hbox->addWidget(lspec_btn);

    viewport = draw_if::new_draw_interface(mainwin::draw_type(), true, this);
    viewport->widget()->setMinimumHeight(ht);
    viewport->widget()->setMaximumHeight(ht);
    hbox->addWidget(viewport->widget());

    vb->addLayout(hbox);

    ltab_sb = new QScrollBar(Qt::Horizontal, this);
    vb->addWidget(ltab_sb);

    connect(viewport->widget(), SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));
    connect(viewport->widget(), SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(button_press_slot(QMouseEvent*)));
    connect(viewport->widget(), SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(button_release_slot(QMouseEvent*)));
    connect(lspec_btn, SIGNAL(toggled(bool)),
        this, SLOT(s_btn_slot(bool)));
    connect(ltab_sb, SIGNAL(valueChanged(int)),
        this, SLOT(ltab_scroll_value_changed_slot(int)));
}


void
layertab_w::update_scrollbar()
{
    if (ltab_sb) {
        int numents = CD()->LayersUsed(DSP()->CurMode()) - 1;
        int nm = numents - LT()->Ltab()->columns();
        if (nm < 0)
            nm = 0;
        ltab_sb->setMaximum(nm);
        ltab_sb->setSingleStep(1);
        ltab_sb->setPageStep(LT()->Ltab()->columns());
//        emit valueChanged(ltab_sb->value());
    }
}


void
layertab_w::resize_slot(QResizeEvent*)
{
    LT()->InitLayerTable();
    LT()->ShowLayerTable();
}


// Dispatch button presses in the layer table area.
//
void
layertab_w::button_press_slot(QMouseEvent *ev)
{
    int button = 0;
    if (ev->button() == Qt::LeftButton)
        button = 1;
    else if (ev->button() == Qt::MidButton)
        button = 2;
    else if (ev->button() == Qt::RightButton)
        button = 3;

    button = Kmap.button_map(button);
    int state = ev->modifiers();

    if (XM()->IsDoingHelp() && (state & GR_SHIFT_MASK)) {
        DSPmainWbag(PopUpHelp("layertab"))
        return;
    }

    switch (button) {
    case 1:
        LT()->Ltab()->b1_handler(ev->x(), ev->y(), state, true);
        break;
    case 2:
        LT()->Ltab()->b2_handler(ev->x(), ev->y(), state, true);
        break;
    case 3:
        LT()->Ltab()->b3_handler(ev->x(), ev->y(), state, true);
        break;
    }
    update();
}


void
layertab_w::button_release_slot(QMouseEvent *ev)
{
    int button = 0;
    if (ev->button() == Qt::LeftButton)
        button = 1;
    else if (ev->button() == Qt::MidButton)
        button = 2;
    else if (ev->button() == Qt::RightButton)
        button = 3;

    button = Kmap.button_map(button);
    int state = ev->modifiers();

    if (XM()->IsDoingHelp() && (state & GR_SHIFT_MASK)) {
        DSPmainWbag(PopUpHelp("layertab"))
        return;
    }

    switch (button) {
    case 1:
        LT()->Ltab()->b1_handler(ev->x(), ev->y(), state, false);
        break;
    case 2:
        LT()->Ltab()->b2_handler(ev->x(), ev->y(), state, false);
        break;
    case 3:
        LT()->Ltab()->b3_handler(ev->x(), ev->y(), state, false);
        break;
    }
}


// Callback for 'S' (layer specific) button
//
void
layertab_w::s_btn_slot(bool set)
{
    LT()->SetLayerSpecific(set);
}


void
layertab_w::ltab_scroll_value_changed_slot(int val)
{
    if (val != LT()->Ltab()->first_visible()) {
        LT()->Ltab()->set_first_visible(val);
        LT()->Ltab()->show();
    }
}
// End of layertab_w functions


//-----------------------------------------------------------------------------
// QTltab functions

QTltab::QTltab(bool nogr, QWidget *parent)
{
//XXX
//    disabled = nogr;
    if (nogr)
        return;

    mainwin *w = dynamic_cast<mainwin*>(parent);
    if (w)
        viewport = w->layer_table()->draw_area();

    QFont *font;
    if (FC.getFont(&font, FNT_SCREEN))
        viewport->set_font(font);
}


/*
static unsigned int
revbytes(unsigned bits, int nb)
{
    unsigned int tmp = bits;
    bits = 0;
    unsigned char *a = (unsigned char*)&tmp;
    unsigned char *b = (unsigned char*)&bits;
    nb--;
    for (int i = 0; i <= nb; i++)
        b[i] = a[nb-i];
    return (bits);
}
*/


void
QTltab::setup_drawable()
{
    // no action necessary
}


void
QTltab::blink(CDl *ld)
{
(void)ld;
}


// Exposure redraw.
//
void
QTltab::refresh(int x, int y, int w, int h)
{
    viewport->widget()->repaint(x, y, w, h);
}


// Update the layer-specific buttons.
//
void
QTltab::lspec_callback()
{
//    lt_more();
//    sel_control_set_lspec();
    if (mainBag())
        mainBag()->layer_table()->ls_button()->setChecked(
            Selections.layer_specific);
}


// Return the drawing area size.
//
void
QTltab::win_size(int *w, int *h)
{
    QSize qs = viewport->widget()->size();
    *w = qs.width();
    *h = qs.height();
}


// Update the display.
//
void
QTltab::update()
{
    Update();
}


void
QTltab::update_scrollbar()
{
    if (mainBag())
        mainBag()->layer_table()->update_scrollbar();
}
// End of QTltab functions.

