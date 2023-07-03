
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
#include "qtcoord.h"
#include "dsp.h"
#include "dsp_color.h"
#include "events.h"

#include <QHBoxLayout>


// Application interface to set abs/rel and redraw after color change.
// The optional rx,ry are the reference in relative mode.
//
void
cMain::SetCoordMode(COmode mode, int rx, int ry)
{
    if (!QTcoord::self())
        return;
    if (mode == CO_ABSOLUTE)
        QTcoord::self()->set_mode(0, 0, false, true);
    else if (mode == CO_RELATIVE)
        QTcoord::self()->set_mode(rx, ry, true, true);
    else if (mode == CO_REDRAW)
        QTcoord::self()->redraw();
}


QTcoord *QTcoord::instPtr = 0;

QTcoord::QTcoord(QTmainwin *prnt) : QWidget(prnt), QTdraw(XW_TEXT)
{
    instPtr = this;

    co_lx = co_ly = 0;
    co_width = co_height = 0;
    co_x = co_y = 0;
    co_rel = false;
    co_snap = true;

    gd_viewport = draw_if::new_draw_interface(DrawNative, false, this);
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(gd_viewport->widget());

    int wid = 90*QTfont::stringWidth(0, FNT_SCREEN);
    int hei = QTfont::lineHeight(FNT_SCREEN) + 2;
    setMinimumWidth(wid);
    setMinimumHeight(hei);
    setMaximumHeight(hei);
    co_width = wid;
    co_height = hei;
    connect(gd_viewport->widget(), SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(redraw_slot()), Qt::QueuedConnection);

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_SCREEN))
        gd_viewport->set_font(fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);
}


QTcoord::~QTcoord()
{
    instPtr = 0;
}


void
QTcoord::print(int xc, int yc, int upd)
{
    const char *fmt;
    DisplayMode mode = EV()->CurrentWin()->Mode();
    if (mode == Physical && CDphysResolution != 1000)
        fmt = "%.4f";
    else
        fmt = "%.3f";

    if (CDvdb()->getVariable(VA_ScreenCoords)) {
        EV()->CurrentWin()->LToP(xc, yc, xc, yc);
        if (mode == Physical) {
            xc *= CDphysResolution;
            yc *= CDphysResolution;
        }
        else {
            xc *= CDelecResolution;
            yc *= CDelecResolution;
        }
    }
    else {
        WindowDesc *wd = EV()->CurrentWin();
        if (wd) {
            double ys = wd->YScale();
            yc = mmRnd(yc/ys);
        }
    }

    unsigned int c1 = DSP()->Color(PromptTextColor);
    unsigned int c2 = DSP()->Color(PromptEditTextColor);
    int fwid, fhei;
    TextExtent(0, &fwid, &fhei);
    int xx = 2;
    int yy = fhei-2;

    if (co_snap)
        EV()->CurrentWin()->Snap(&xc, &yc);
    if (upd == COOR_MOTION) {
        if (xc == co_lx && yc == co_ly) {
            return;
        }
        co_lx = xc;
        co_ly = yc;
    }
    else if (upd == COOR_REL) {
        xc = co_lx;
        yc = co_ly;
    }

    QSize qs = gd_viewport->widget()->size();
    co_width = qs.width();
    co_height = qs.height();

    SetColor(DSP()->Color(PromptBackgroundColor));
    SetFillpattern(0);
    Box(0, co_height, co_width, 0);
    SetBackground(DSP()->Color(PromptBackgroundColor));

    char buf[128];
    const char *str = "x,y";
    SetColor(c1);
    Text(str, xx, yy, 0);
    xx += (strlen(str) + 2)*fwid;

    int xs = xx;
    snprintf(buf, sizeof(buf), fmt,
        mode == Physical ? MICRONS(xc) : ELEC_MICRONS(xc));
    SetColor(c2);
    Text(buf, xx, yy, 0);
    xx += strlen(buf)*fwid;
    SetColor(c1);
    Text(",", xx, yy, 0);
    xx += 2*fwid;
    snprintf(buf, sizeof(buf), fmt,
        mode == Physical ? MICRONS(yc) : ELEC_MICRONS(yc));
    SetColor(c2);
    Text(buf, xx, yy, 0);

    xs += 24*fwid;
    xx += (strlen(buf) + 4)*fwid;
    if (xx < xs)
        xx = xs;
    str = "dx,dy";
    SetColor(c1);
    Text(str, xx, yy, 0);
    xx += (strlen(str) + 2)*fwid;

    int xr, yr;
    if (co_rel) {
        xr = co_x;
        yr = co_y;
    }
    else
        EV()->GetReference(&xr, &yr);

    xs = xx;
    snprintf(buf, sizeof(buf), fmt,
        mode == Physical ? MICRONS(xc - xr) : ELEC_MICRONS(xc - xr));
    SetColor(c2);
    Text(buf, xx, yy, 0);
    xx += (strlen(buf))*fwid;
    SetColor(c1);
    Text(",", xx, yy, 0);
    xx += 2*fwid;
    snprintf(buf, sizeof(buf), fmt,
        mode == Physical ? MICRONS(yc - yr) : ELEC_MICRONS(yc - yr));
    SetColor(c2);
    Text(buf, xx, yy, 0);

    xs += 24*fwid;
    xx += (strlen(buf) + 4)*fwid;
    if (xx < xs)
        xx = xs;
    str = co_rel ? "anchor" : "last";
    SetColor(c1);
    Text(str, xx, yy, 0);
    xx += (strlen(str) + 2)*fwid;

    snprintf(buf, sizeof(buf),
        fmt, mode == Physical ? MICRONS(xr) : ELEC_MICRONS(xr));
    SetColor(c2);
    Text(buf, xx, yy, 0);
    xx += strlen(buf)*fwid;
    SetColor(c1);
    Text(",", xx, yy, 0);
    xx += 2*fwid;
    snprintf(buf, sizeof(buf),
        fmt, mode == Physical ? MICRONS(yr) : ELEC_MICRONS(yr));
    SetColor(c2);
    Text(buf, xx, yy, 0);

    Update();
}


void
QTcoord::redraw_slot()
{
    int xx, yy;
    EV()->Cursor().get_xy(&xx, &yy);
    print(xx, yy, COOR_BEGIN);
}


void
QTcoord::font_changed_slot(int fnum)
{
    if (fnum == FNT_SCREEN) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_SCREEN))
            gd_viewport->set_font(fnt);
        int xx, yy;
        EV()->Cursor().get_xy(&xx, &yy);
        print(xx, yy, COOR_BEGIN);
    }
}

