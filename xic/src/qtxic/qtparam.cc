
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

#include "qtmain.h"
#include "qtparam.h"
#include "qtcoord.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "editif.h"
#include "select.h"
#include "tech.h"
#include "events.h"
#include "pushpop.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QClipboard>


//-----------------------------------------------------------------------------
// QTparam:  Parameter/status display area of the main window.

// Display the parameter text in the parameter readout area.
//
void
cMain::ShowParameters(const char*)
{
    if (QTparam::self())
        QTparam::self()->print();
}
// End of cMain functions.


QTparam *QTparam::instPtr = 0;

QTparam::QTparam(QTmainwin *prnt) : QWidget(prnt), QTdraw(XW_TEXT)
{
    instPtr = this;
    p_drag_x = 0;
    p_drag_y = 0;
    p_has_drag = false;
    p_dragged = false;
    p_xval = 0;
    p_yval = 0;
    p_width = 0;
    p_height = 0;

    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    gd_viewport = new QTcanvas();
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(0);
    hbox->addWidget(gd_viewport);

    QFont *fnt;
    if (Fnt()->getFont(&fnt, FNT_SCREEN))
        gd_viewport->set_font(fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    connect(Viewport(), SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));
    connect(Viewport(), SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(button_down_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(button_up_slot(QMouseEvent*)));
    connect(Viewport(), SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(motion_slot(QMouseEvent*)));
}


QTparam::~QTparam()
{
    instPtr = 0;
}


void
QTparam::print()
{
    unsigned long c1 = DSP()->Color(PromptTextColor);
    unsigned long c2 = DSP()->Color(PromptEditTextColor);
    int fwid, fhei;
    TextExtent(0, &fwid, &fhei);

    p_xval = 2;
    p_yval = (height() + fhei)/2 - 2;

    unsigned int selectno;
    Selections.countQueue(CurCell(), &selectno, 0);
    char textbuf[256];

    p_text.reset();

    const char *str = Tech()->TechnologyName();
    if (str && *str) {
        p_text.append_string("Tech: ", c1);
        p_text.append_string(str, c2);
        p_text.append_string("  ", c2);
    }
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        str = DSP()->MainWdesc()->DbName();
        p_text.append_string("Hier: ", c1);
        p_text.append_string(str, c2);

        if (DSP()->MainWdesc()->DbCellName()) {
            str = DSP()->MainWdesc()->DbCellName();
            p_text.append_string(" Cell: ", c1);
            p_text.append_string(str, c2);
        }
        p_text.append_string("  ", c2);
    }
    else {
        str = (DSP()->CurCellName() ? Tstring(DSP()->CurCellName()) : "none");
        p_text.append_string("Cell: ", c1);
        p_text.append_string(str, c2);

        CDs *cursd = CurCell();
        if (cursd) {
            if (cursd->isImmutable())
                p_text.append_string(" RO", DSP()->Color(PromptHighlightColor));
            else if (cursd->countModified())
                p_text.append_string(" Mod",DSP()->Color(PromptHighlightColor));
        }
        p_text.append_string("  ", c2);
    }

    if (DSP()->PhysVisGridOrigin()->x || DSP()->PhysVisGridOrigin()->y) {
        p_text.append_string("PhGridOffs: ",DSP()->Color(PromptHighlightColor));
        p_text.append_string("  ", c2);
    }

    DSPattrib *a = DSP()->MainWdesc()->Attrib();
    if (a) {
        DisplayMode m = DSP()->CurMode();
        GridDesc *g = a->grid(m);
        double spa = g->spacing(m);
        if (g->snap() < 0)
            snprintf(textbuf, 28, "%g/%g", -spa/g->snap(), spa);
        else
            snprintf(textbuf, 28, "%g/%g", spa*g->snap(), spa);
        p_text.append_string("Grid/Snap: ", c1);
        p_text.append_string(textbuf, c2);
        p_text.append_string("  ", c2);
    }
    if (selectno) {
        snprintf(textbuf, 10, "%d", selectno);
        p_text.append_string("Select: ", c1);
        p_text.append_string(textbuf, c2);
        CDc *cd = (CDc*)Selections.firstObject(CurCell(), "c");
        if (cd) {
            snprintf(textbuf, sizeof(textbuf), " (%s)",
                Tstring(cd->cellname()));
            p_text.append_string(textbuf, c2);
        }
        p_text.append_string("  ", c2);
    }
    int clev = PP()->Level();
    if (clev) {
        snprintf(textbuf, 12, "%d", clev);
        p_text.append_string("Push: ", c1);
        p_text.append_string(textbuf, c2);
        p_text.append_string("  ", c2);
    }

    char *tfstring = GEO()->curTx()->tform_string();
    if (tfstring) {
        // Above returns empty string for identity transform.
        if (*tfstring) {
            p_text.append_string("Transf: ", c1);
            p_text.append_string(tfstring, c2);
            p_text.append_string("  ", c2);
        }
        delete [] tfstring;
    }

    str = EV()->CurCmd() ? EV()->CurCmd()->Name() : 0;
    if (str) {
        p_text.append_string("Mode: ", c1);
        p_text.append_string(str, c2);
        p_text.append_string("  ", c2);
    }
    if (DSP()->CurMode() == Physical) {
        double dx = DSP()->MainWdesc()->ViewportWidth() /
            DSP()->MainWdesc()->Ratio()/CDphysResolution;
        double dy = DSP()->MainWdesc()->ViewportHeight() /
            DSP()->MainWdesc()->Ratio()/CDphysResolution;
        snprintf(textbuf, sizeof(textbuf), "%.2f X %.2f", dx, dy);
        p_text.append_string("Window: ", c1);
        p_text.append_string(textbuf, c2);
        p_text.append_string("  ", c2);
    }

    p_text.setup(this);
    display(0, 256);

    if (QTcoord::self())
        QTcoord::self()->print(0, 0, QTcoord::COOR_REL);
}


void
QTparam::display(int start, int end)
{
    if (start == 0 && end == 256) {
        SetWindowBackground(DSP()->Color(PromptBackgroundColor));
        SetBackground(DSP()->Color(PromptBackgroundColor));
        Clear();
    }
    p_text.display(this, start, end);
    Update();
}


// Select the chars that overlap the pixel coord range x1,x2.
//
void
QTparam::select(int x1, int x2)
{
    if (x2 < x1) {
        int t = x2;
        x2 = x1;
        x1 = t;
    }
    unsigned int xo1 = 0, xo2 = 0;
    bool was_sel = p_text.has_sel(&xo1, &xo2);
    if (p_text.select(x1, x2)) {
        unsigned int xstart, xend;
        if (p_text.has_sel(&xstart, &xend)) {
            if (was_sel) {
                if (xo1 < xstart)
                    xstart = xo1;
                if (xo2 < xstart)
                    xstart = xo2;
                if (xo1 > xend)
                    xend = xo1;
                if (xo2 > xend)
                    xend = xo2;
            }
            char *str = p_text.get_sel();
            if (str) {
                display(xstart, xend);
                QApplication::clipboard()->setText(str);
                delete [] str;
            }
        }
    }
    else if (was_sel)
        deselect();
}


// Select the word at pixel coord x.
//
void
QTparam::select_word(int xx)
{
    deselect();
    if (p_text.select_word(xx)) {
        unsigned int xstart, xend;
        if (p_text.has_sel(&xstart, &xend)) {
            char *str = p_text.get_sel();
            if (str) {
                display(xstart, xend);
                QApplication::clipboard()->setText(str);
                delete [] str;
            }
        }
    }
}


// Deselect text.
//
bool
QTparam::deselect()
{
    unsigned int xstart, xend;
    if (p_text.has_sel(&xstart, &xend)) {
        p_text.set_sel(0, 0);
        display(xstart, xend);
        return (true);
    }
    return (false);
}


void
QTparam::font_changed_slot(int fnum)
{
    if (fnum == FNT_SCREEN) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_SCREEN)) {
            gd_viewport->set_font(fnt);
            setFixedHeight(QFontMetrics(*fnt).height());
            print();
        }
    }
}


void
QTparam::resize_slot(QResizeEvent*)
{
    print();
}


namespace {
    bool is_shift_down()
    {
        return (QApplication::keyboardModifiers() & Qt::ShiftModifier);
    }
}


void
QTparam::button_down_slot(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton &&
            ev->type() == QEvent::MouseButtonPress) {
        if (XM()->IsDoingHelp() && !is_shift_down())
            DSPmainWbag(PopUpHelp("statusline"))
        else {
            p_has_drag = true;
            p_dragged = false;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
            p_drag_x = ev->position().x();
            p_drag_y = ev->position().y();
#else
            p_drag_x = ev->x();
            p_drag_y = ev->y();
#endif
        }
        ev->accept();
    }
    else
        ev->ignore();
}


void
QTparam::button_up_slot(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton &&
            ev->type() == QEvent::MouseButtonRelease) {
        if (p_has_drag && !p_dragged)
            select_word(p_drag_x);
        p_has_drag = false;
        ev->accept();
    }
    else
        ev->ignore();
}


void
QTparam::motion_slot(QMouseEvent *ev)
{
    if (p_has_drag) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        int mvx = ev->position().x();
#else
        int mvx = ev->x();
#endif
        select(p_drag_x, mvx);
        p_dragged = true;
    }   
}
// End of QTparam functions.


// Select the chars that overlap the pixel coord range xmin,xmax. 
// Return true if success, false if out of range.
//
bool
ptext_t::select(int xmin, int xmax)
{
    if (!pt_set)
        return (false);

    unsigned int xstart = pt_end - 1;
    for (int i = pt_end - 1; i >= 0; i--) {
        if (xmin >= pt_chars[i].pc_posn + pt_chars[i].pc_width)
            break;
        xstart = i;
    }

    unsigned int xend = 0;
    for (unsigned int i = 0; i < pt_end; i++) {
        if (xmax < pt_chars[i].pc_posn)
            break;
        xend = i;
    }

    if (xend >= xstart) {
        set_sel(xstart, xend + 1);
        return (true);
    }
    return (false);
}


// Select the word at pixel coord x.
//
bool
ptext_t::select_word(int x)
{
    if (!pt_set)
        return (false);

    unsigned int xc = pt_end;
    for (unsigned int i = 0; i < pt_end; i++) {
        if (x < pt_chars[i].pc_posn)
            break;
        xc = i;
    }
    if (xc == pt_end || isspace(pt_chars[xc].pc_char))
        return (false);

    int xb = xc;
    while (xb > 0) {
        if (!isspace(pt_chars[xb-1].pc_char))
            xb--;
        else
            break;
    }

    int xe = xc + 1;
    while (xe < pt_end) {
        if (!isspace(pt_chars[xe].pc_char))
            xe++;
        else
            break;
    }
    set_sel(xb, xe);
    return (true);
}


// Set up the character offsets and widths.
//
void
ptext_t::setup(QTparam *prm)
{
    char bf[2];
    bf[1] = 0;
    for (unsigned int i = 0; i < pt_end; i++) {
        bf[0] = pt_chars[i].pc_char;
        if (!i)
            pt_chars[i].pc_posn = prm->xval();
        else
            pt_chars[i].pc_posn = pt_chars[i-1].pc_posn +
                pt_chars[i-1].pc_width;
        int w;
        prm->TextExtent(bf, &w, 0);
        pt_chars[i].pc_width = w;
    }
    pt_sel_start = 0;
    pt_sel_end = 0;
    pt_set = true;
}


// Display the chars in the index range fc through lc-1.
//
void
ptext_t::display(QTparam *prm, unsigned int fc, unsigned int lc)
{
    if (!pt_set)
        return;
    if (lc < fc) {
        unsigned int t = lc;
        lc = fc;
        fc = t;
    }
    if (fc >= pt_end)
        return;
    if (lc > pt_end)
        lc = pt_end;

    prm->SetFillpattern(0);
    prm->SetLinestyle(0);

    prm->SetWindowBackground(DSP()->Color(PromptBackgroundColor));
    prm->SetBackground(DSP()->Color(PromptBackgroundColor));
    prm->SetColor(DSP()->Color(PromptBackgroundColor));
    prm->Box(pt_chars[fc].pc_posn, prm->height(),
        pt_chars[lc-1].pc_posn + pt_chars[lc-1].pc_width - 1, 0);

    char bf[2];
    bf[1] = 0;
    for (unsigned int i = fc; i < lc; i++) {
        bf[0] = pt_chars[i].pc_char;
        if (pt_sel_end > pt_sel_start && i >= pt_sel_start && i < pt_sel_end) {
            prm->SetColor(DSP()->Color(PromptSelectionBackgColor));
            prm->Box(pt_chars[i].pc_posn, prm->height(),
                pt_chars[i].pc_posn + pt_chars[i].pc_width - 1, 0);
        }
        prm->SetColor(pt_chars[i].pc_color);
        prm->Text(bf, pt_chars[i].pc_posn, prm->yval(), 0);
    }
}


// Display the chars that overlap the pixel range xmin,xmax.
//
void
ptext_t::display_c(QTparam *prm, int xmin, int xmax)
{
    if (!pt_set)
        return;

    unsigned int xstart = pt_end - 1;
    for (int i = pt_end - 1; i >= 0; i--) {
        if (xmin >= pt_chars[i].pc_posn + pt_chars[i].pc_width)
            break;
        xstart = i;
    }

    unsigned int xend = 0;
    for (unsigned int i = 0; i < pt_end; i++) {
        if (xmax < pt_chars[i].pc_posn)
            break;
        xend = i;
    }

    if (xend >= xstart)
        prm->display(xstart, xend + 1);
}

