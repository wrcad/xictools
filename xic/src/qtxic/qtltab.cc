
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
#include "bitmaps/lsearch.xpm"
#include "bitmaps/lsearchn.xpm"

#include <unistd.h>
#include <errno.h>

#include <QWidget>
#include <QMouseEvent>
#include <QLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QLineEdit>

// help keywords used:
//  layertab
//  layerchange


//--------------------------------------------------------------------
//
// The Layer Table
//
//--------------------------------------------------------------------

QTltab *QTltab::instancePtr = 0;

QTltab::QTltab(bool nogr) : QTdraw(XW_LTAB)
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class QTltab already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    ltab_scrollbar = 0;
    ltab_entry = 0;
    ltab_sbtn = 0;
    ltab_lsearch = 0;
    ltab_lsearchn = 0;
    ltab_search_str = 0;
    ltab_last_index = 0;
    ltab_last_mode = -1;
    ltab_timer_id = 0;
    ltab_hidden = false;

    lt_drag_x = 0;
    lt_drag_y = 0;
    lt_dragging = false;
    lt_disabled = nogr;
    if (nogr)
        return;

    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->setMargin(0);
    vb->setSpacing(0);

    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(0);
    hbox->setSpacing(0);

    ltab_scrollbar = new QScrollBar(Qt::Vertical, this);
    hbox->addWidget(ltab_scrollbar);

    gd_viewport = draw_if::new_draw_interface(QTmainwin::draw_type(),
        true, this);
    hbox->addWidget(gd_viewport->widget());

    vb->addLayout(hbox);

    QFont *scfont;
    if (FC.getFont(&scfont, FNT_SCREEN))
        gd_viewport->set_font(scfont);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed(int)), Qt::QueuedConnection);

    connect(gd_viewport->widget(), SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));
    connect(gd_viewport->widget(), SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(button_press_slot(QMouseEvent*)));
    connect(gd_viewport->widget(), SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(button_release_slot(QMouseEvent*)));
    connect(ltab_scrollbar, SIGNAL(valueChanged(int)),
        this, SLOT(ltab_scroll_value_changed_slot(int)));
}


QTltab::~QTltab()
{
    instancePtr = 0;
}


void
QTltab::on_null_ptr()
{
    fprintf(stderr, "Singleton class QTltab used before instantiated.\n");
    exit(1);
}


void
QTltab::setup_drawable()
{
    // no action necessary
}


void
QTltab::blink(CDl *ld)
{
    //XXX FIXME
(void)ld;
}


void
QTltab::show(const CDl *ld)
{
    show_direct(ld);
}


// Exposure redraw.
//
void
QTltab::refresh(int xx, int yy, int w, int h)
{
    gd_viewport->widget()->repaint(xx, yy, w, h);
}


// Return the drawing area size.
//
void
QTltab::win_size(int *w, int *h)
{
    QSize qs = gd_viewport->widget()->size();
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
    if (ltab_scrollbar) {
        int numents = CDldb()->layersUsed(DSP()->CurMode()) - 1;
        int nm = numents - lt_vis_entries;
        if (nm < 0)
            nm = 0;
        ltab_scrollbar->setMaximum(nm);
        ltab_scrollbar->setSingleStep(1);
        ltab_scrollbar->setPageStep(nm);
        emit valueChanged(ltab_scrollbar->value());
    }
}


void
QTltab::hide_layer_table(bool)
{
    /*XXX
    // Reparenting does not seem to work, always get fatal errors.
    // We just move the slider to cover the layer table here.

    static int last_wid;
    if (hide) {
        last_wid = gtk_paned_get_position(
            GTK_PANED(gtk_widget_get_parent(ltab_container)));
        if (gtk_widget_get_mapped(ltab_scrollbar)) {
            GtkRequisition req;
            gtk_widget_get_requisition(ltab_scrollbar, &req);
            int ww = req.width;
            if (getenv("XIC_MENU_RIGHT"))
                ww = -ww;
            last_wid += ww;
        }

        gtk_widget_hide(ltab_container);
        gtk_widget_hide(ltab_scrollbar);
        gtk_paned_set_position(
            GTK_PANED(gtk_widget_get_parent(ltab_container)), 0);
        ltab_hidden = true;
    }
    else {
        if (lt_vis_entries <= CDldb()->layersUsed(DSP()->CurMode()) - 1) {
            gtk_widget_show(ltab_scrollbar);
            GtkRequisition req;
            gtk_widget_get_requisition(ltab_scrollbar, &req);
            int ww = req.width;
            if (getenv("XIC_MENU_RIGHT"))
                ww = -ww;
            last_wid -= ww;
        }
        gtk_widget_show(ltab_container);
        gtk_paned_set_position(GTK_PANED(gtk_widget_get_parent(ltab_container)),
            last_wid);
        ltab_hidden = false;
    }
    */
}


void
QTltab::set_layer()
{
    CDl *ld = LT()->CurLayer();
    if (ld)
        ltab_entry->setText(tr(ld->name()));
    else {
        ltab_entry->setText("");
        return;
    }

    int pos = ld->index(DSP()->CurMode()) - 1;
    if (pos >= first_visible() && pos < first_visible() + vis_entries())
        show();
    else {
        int nt = CDldb()->layersUsed(DSP()->CurMode()) - 1;
        nt -= vis_entries();
        pos -= vis_entries()/2;
        if (pos < 0)
            pos = 0;
        else if (pos > nt)
            pos = nt;
        lt_first_visible = pos;
        update_scrollbar();
        show();
    }
}


void
QTltab::set_search_widgets(QPushButton *p, QLineEdit *e)
{
    if (!p || !e)
        return;
    ltab_sbtn = p;
    ltab_entry = e;

    ltab_sbtn->setToolTip(tr("Search layer table for layer."));
    ltab_lsearch = new QPixmap(lsearch_xpm);
    ltab_sbtn->setIcon(*ltab_lsearch);
    connect(ltab_sbtn, SIGNAL(clicked()), this, SLOT(pressed_slot()));
}


void
QTltab::resize_slot(QResizeEvent*)
{
    init();
    show();
}


// Dispatch button presses in the layer table area.
//
void
QTltab::button_press_slot(QMouseEvent *ev)
{
    int button = 0;
    if (ev->button() == Qt::LeftButton)
        button = 1;
    else if (ev->button() == Qt::MidButton)
        button = 2;
    else if (ev->button() == Qt::RightButton)
        button = 3;

    button = Kmap()->ButtonMap(button);
    int state = ev->modifiers();

    if (XM()->IsDoingHelp() && (state & GR_SHIFT_MASK)) {
        DSPmainWbag(PopUpHelp("layertab"))
        return;
    }

    switch (button) {
    case 1:
        b1_handler(ev->x(), ev->y(), state, true);
        break;
    case 2:
        b2_handler(ev->x(), ev->y(), state, true);
        break;
    case 3:
        b3_handler(ev->x(), ev->y(), state, true);
        break;
    }
    update();
}


void
QTltab::button_release_slot(QMouseEvent *ev)
{
    int button = 0;
    if (ev->button() == Qt::LeftButton)
        button = 1;
    else if (ev->button() == Qt::MidButton)
        button = 2;
    else if (ev->button() == Qt::RightButton)
        button = 3;

    button = Kmap()->ButtonMap(button);
    int state = ev->modifiers();

    if (XM()->IsDoingHelp() && (state & GR_SHIFT_MASK)) {
        DSPmainWbag(PopUpHelp("layertab"))
        return;
    }

    switch (button) {
    case 1:
        b1_handler(ev->x(), ev->y(), state, false);
        break;
    case 2:
        b2_handler(ev->x(), ev->y(), state, false);
        break;
    case 3:
        b3_handler(ev->x(), ev->y(), state, false);
        break;
    }
}


void
QTltab::ltab_scroll_value_changed_slot(int val)
{
    if (val != first_visible()) {
        set_first_visible(val);
        show();
    }
}


void
QTltab::font_changed(int fnum)
{
    if (fnum == FNT_SCREEN) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_SCREEN))
            gd_viewport->set_font(fnt);
        //XXX redraw
    }
}


void
QTltab::pressed_slot()
{
    if (!ltab_search_str)
        ltab_search_str =
            lstring::copy(ltab_entry->text().toLatin1());
    if (!ltab_search_str)
        return;
    CDl *ld = CDldb()->search(DSP()->CurMode(), ltab_last_index + 1,
        ltab_search_str);
    ltab_last_index = 0;
    if (!ld) {
        delete [] ltab_search_str;
        ltab_search_str = 0;
        return;
    }

    ltab_last_index = ld->index(DSP()->CurMode());
    LT()->SetCurLayer(ld);

    if (!ltab_lsearchn)
        ltab_lsearchn = new QPixmap(lsearchn_xpm);
    ltab_sbtn->setIcon(*ltab_lsearchn);

    if (ltab_timer_id)
        QTdev::self()->RemoveTimer(ltab_timer_id);
    ltab_timer_id = QTdev::self()->AddTimer(3000, ltab_ent_timer, this);
}


// Static function.
int
QTltab::ltab_ent_timer(void *arg)
{
    QTltab *lt = static_cast<QTltab*>(arg);
    if (lt) {
        lt->ltab_last_index = 0;
        delete [] lt->ltab_search_str;
        lt->ltab_search_str = 0;
        lt->ltab_sbtn->setIcon(*lt->ltab_lsearch);
        if (LT()->CurLayer())
            lt->ltab_entry->setText(tr(LT()->CurLayer()->name()));
        lt->ltab_timer_id = 0;
    }
    return (0);
}
// End of QTltab functions.


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

