
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


cCoord *cCoord::instancePtr = 0;

// Application interface to set abs/rel and redraw after color change.
// The optional rx,ry are the reference in relative mode.
//
void
cMain::SetCoordMode(COmode mode, int rx, int ry)
{
    if (!Coord())
        return;
    if (mode == CO_ABSOLUTE)
        Coord()->set_mode(0, 0, false, true);
    else if (mode == CO_RELATIVE)
        Coord()->set_mode(rx, ry, true, true);
    else if (mode == CO_REDRAW)
        Coord()->redraw();
}


cCoord::cCoord(QTmainwin *prnt) : QWidget(prnt), QTdraw(XW_TEXT)
{
    instancePtr = this;

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

    int wid = 600;
    int hei = line_height() + 2;
    setFixedSize(wid, hei);
    co_width = wid;
    co_height = hei;
    connect(gd_viewport->widget(), SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(redraw_slot()), Qt::QueuedConnection);

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_SCREEN))
        gd_viewport->set_font(fnt);
}


cCoord::~cCoord()
{
    instancePtr = 0;
}


void
cCoord::print(int xc, int yc, int upd)
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
    int x = 2;
//    int y = (co_height + fhei)/2;  // center justify
// XXX The fhei is 16, co_height is 50, obviously something isn't consistent.
int y = fhei-3;

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
    Text(str, x, y, 0);
    x += (strlen(str) + 2)*fwid;

    int xs = x;
    snprintf(buf, sizeof(buf), fmt,
        mode == Physical ? MICRONS(xc) : ELEC_MICRONS(xc));
    SetColor(c2);
    Text(buf, x, y, 0);
    x += strlen(buf)*fwid;
    SetColor(c1);
    Text(",", x, y, 0);
    x += 2*fwid;
    snprintf(buf, sizeof(buf), fmt,
        mode == Physical ? MICRONS(yc) : ELEC_MICRONS(yc));
    SetColor(c2);
    Text(buf, x, y, 0);

    xs += 24*fwid;
    x += (strlen(buf) + 4)*fwid;
    if (x < xs)
        x = xs;
    str = "dx,dy";
    SetColor(c1);
    Text(str, x, y, 0);
    x += (strlen(str) + 2)*fwid;

    int xr, yr;
    if (co_rel) {
        xr = co_x;
        yr = co_y;
    }
    else
        EV()->GetReference(&xr, &yr);

    xs = x;
    snprintf(buf, sizeof(buf), fmt,
        mode == Physical ? MICRONS(xc - xr) : ELEC_MICRONS(xc - xr));
    SetColor(c2);
    Text(buf, x, y, 0);
    x += (strlen(buf))*fwid;
    SetColor(c1);
    Text(",", x, y, 0);
    x += 2*fwid;
    snprintf(buf, sizeof(buf), fmt,
        mode == Physical ? MICRONS(yc - yr) : ELEC_MICRONS(yc - yr));
    SetColor(c2);
    Text(buf, x, y, 0);

    xs += 24*fwid;
    x += (strlen(buf) + 4)*fwid;
    if (x < xs)
        x = xs;
    str = co_rel ? "anchor" : "last";
    SetColor(c1);
    Text(str, x, y, 0);
    x += (strlen(str) + 2)*fwid;

    snprintf(buf, sizeof(buf),
        fmt, mode == Physical ? MICRONS(xr) : ELEC_MICRONS(xr));
    SetColor(c2);
    Text(buf, x, y, 0);
    x += strlen(buf)*fwid;
    SetColor(c1);
    Text(",", x, y, 0);
    x += 2*fwid;
    snprintf(buf, sizeof(buf),
        fmt, mode == Physical ? MICRONS(yr) : ELEC_MICRONS(yr));
    SetColor(c2);
    Text(buf, x, y, 0);

    Update();
}


void
cCoord::redraw_slot()
{
    int xx, yy;
    EV()->Cursor().get_xy(&xx, &yy);
    print(xx, yy, COOR_BEGIN);
}

