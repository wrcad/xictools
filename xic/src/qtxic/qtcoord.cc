
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

coord_w::coord_w(mainwin *prnt) : QWidget(prnt)
{
    co_x = co_y = 0;
    co_rel = false;
    co_snap = true;

    viewport = draw_if::new_draw_interface(DrawNative, false, this);
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(viewport->widget());

    int wid = 34*char_width() + 4;
    int hei = 3*line_height() + line_height()/2;
    setFixedSize(wid, hei);
    connect(viewport->widget(), SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(redraw_slot()), Qt::QueuedConnection);

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_SCREEN))
        viewport->set_font(fnt);
}


void
coord_w::print(int xc, int yc, int upd)
{
    const char *fmt = "%12.3f";
    static int lxc, lyc;
    char buf[128];
    unsigned c1 = DSP()->Color(PromptTextColor);
    unsigned c2 = DSP()->Color(PromptEditTextColor);

    int fwid, dy;
    TextExtent(0, &fwid, &dy);
    int yy = dy + 2;
    int xx = 2;

    if (co_snap)
        EV()->CurrentWin()->Snap(&xc, &yc);
    if (upd == COOR_BEGIN) {
        SetWindowBackground(DSP()->Color(PromptBackgroundColor));
        Clear();
    }
    else if (upd == COOR_MOTION) {
        if (xc == lxc && yc == lyc)
            return;
        lxc = xc;
        lyc = yc;
        viewport->clear_area(8*fwid+2, 4, 0, dy);
        viewport->clear_area(8*fwid+2, 4 + 2*dy, 0, dy);
    }
    else if (upd == COOR_REL) {
        viewport->clear_area(2, 4+dy, 0, dy);
        viewport->clear_area(8*fwid+2, 4 + 2*dy, 0, dy);
        xc = lxc;
        yc = lyc;
    }
    else
        return;

    SetFillpattern(0);
    SetColor(c1);
//    if (upd == COOR_BEGIN || upd == COOR_REL)
    if (upd == COOR_BEGIN)
        Text("point", xx, yy, 0);
    if (upd != COOR_REL) {
        xx += 8*fwid;
        SetColor(c2);
        sprintf(buf, fmt, MICRONS(xc));
        Text(buf, xx, yy, 0);
        xx += (strlen(buf))*fwid;
        SetColor(c1);
        Text(",", xx, yy, 0);
        xx += 2*fwid;
        SetColor(c2);
        sprintf(buf, fmt, MICRONS(yc));
        Text(buf, xx, yy, 0);
    }

    xx = 2;
    yy += dy;
    int xr, yr;
    if (co_rel) {
        xr = co_x;
        yr = co_y;
    }
    else
        EV()->Cursor().get_reference(&xr, &yr);
    SetColor(c1);
    if (upd == COOR_BEGIN || upd == COOR_REL) {
        Text(co_rel ? "anchor" : "last", xx, yy, 0);
        xx += 8*fwid;
        SetColor(c2);
        sprintf(buf, fmt, MICRONS(xr));
        Text(buf, xx, yy, 0);
        xx += (strlen(buf))*fwid;
        SetColor(c1);
        Text(",", xx, yy, 0);
        xx += 2*fwid;
        SetColor(c2);
        sprintf(buf, fmt, MICRONS(yr));
        Text(buf, xx, yy, 0);
    }

    xx = 2;
    yy += dy;
    SetColor(c1);
    if (upd == COOR_BEGIN)
        Text("dx, dy", xx, yy, 0);
    xx += 8*fwid;
    SetColor(c2);
    sprintf(buf, fmt, MICRONS(xc - xr));
    Text(buf, xx, yy, 0);
    xx += (strlen(buf))*fwid;
    SetColor(c1);
    Text(",", xx, yy, 0);
    xx += 2*fwid;
    SetColor(c2);
    sprintf(buf, fmt, MICRONS(yc - yr));
    Text(buf, xx, yy, 0);

    Update();
}


void
coord_w::redraw_slot()
{
    int xx, yy;
    EV()->Cursor().get_xy(&xx, &yy);
    print(xx, yy, COOR_BEGIN);
}

