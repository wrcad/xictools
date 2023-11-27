
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qtdraw.h"
#include "miscutil/texttf.h"

#include <QApplication>

using namespace qtinterf;


// Graphics context storage.  The 0 element is the default.
//
sGbag *sGbag::app_gbags[NUMGCS];


// Static method to create/return the default graphics context.
//
sGbag *
sGbag::default_gbag(int type)
{
    (void)type; //XXX
    return (new sGbag);
    /* XXX  The common graphical context is an X-Windows thing.
    if (type < 0 || type >= NUMGCS)
        type = 0;
    if (!app_gbags[type])
        app_gbags[type] = new sGbag;
    return (app_gbags[type]);
    */
}


//-----------------------------------------------------------------------------
// QTdraw functions

QTdraw::QTdraw(int type)
{
    gd_gbag = sGbag::default_gbag(type);
    gd_viewport = 0;
}


void
QTdraw::Halt()
{
}


//XXX Need transformations
void
QTdraw::Text(const char *str, int x, int y, int xform, int, int)
{
    if (!gd_viewport || !str || !*str)
        return;

    // Swap out embedded newlines for a special character so string
    // remains on one line.
    sLstr lstr;
    if (strchr(str, '\n')) {
        const char *t = str;
        while (*t) {
            if (*t == '\n') {
                // This is the "section sign" UTF-8 character.
                lstr.add_c(0xc2);
                lstr.add_c(0xa7);
            }
            else
                lstr.add_c(*t);
            t++;
        }
        str = lstr.string();
    }

    // Handle justification.
    int wid, hei;
    TextExtent(str, &wid, &hei);
    if (xform & (TXTF_HJC | TXTF_HJR)) {
        if (xform & TXTF_HJR)
            x -= wid;
        else
            x -= wid/2;
    }
    if (xform & (TXTF_VJC | TXTF_VJT)) {
        if (xform & TXTF_VJT)
            y += hei;
        else
            y += hei/2;
    }

    gd_viewport->draw_text(x, y, str, -1);
}


void
QTdraw::DrawGhost()
{
    // This will show the ghost drawing if it has been paused, such as
    // when the ghost changes.  Previously, we would call MovePointer
    // and let the move events trigger the redraw, but applications
    // moving the pointer is no longer generally supported in QT, at
    // least in an obvious way.  This is the replacement for the
    // MovePointer calls, the only purpose of which was to wake up the
    // ghosting.

    QPoint qp = gd_viewport->mapFromGlobal(QCursor::pos());
    gd_viewport->draw_ghost(qp.x(), qp.y());
}


void
QTdraw::QueryPointer(int *x, int *y, unsigned *state)
{
    QPoint ptg(QCursor::pos());
    QPoint ptw = gd_viewport->mapFromGlobal(ptg);
    if (x)
        *x = ptw.x();
    if (y)
        *y = ptw.y();
    if (state) {
        *state = 0;
        int st = QApplication::keyboardModifiers();
        if (st & Qt::ShiftModifier)
            *state |= GR_SHIFT_MASK;
        if (st & Qt::ControlModifier)
            *state |= GR_CONTROL_MASK;
        if (st & Qt::AltModifier)
            *state |= GR_ALT_MASK;
    }
}


void
QTdraw::DefineColor(int *pixel, int r, int g, int b)
{
    *pixel = 0;
    QColor c(r, g, b);
    if (c.isValid())
        *pixel = c.rgb();
}


void
QTdraw::Input(int*, int*, int*, int*)
{
}


//XXX
void
QTdraw::SetXOR(int val)
{
    switch (val) {
    case GRxNone:
        gd_viewport->set_overlay_mode(false);
        break;
    case GRxXor:
        gd_viewport->set_overlay_mode(true);
        break;
    case GRxHlite:
    case GRxUnhlite:
        break;
    }
}


#define GlyphWidth 7

namespace {
    struct glymap { unsigned char bits[GlyphWidth]; } glyphs[] =
    {
       { {0x00, 0x1c, 0x22, 0x22, 0x22, 0x1c, 0x00} }, // circle
       { {0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41} }, // cross (x)
       { {0x08, 0x14, 0x22, 0x41, 0x22, 0x14, 0x08} }, // diamond
       { {0x08, 0x14, 0x14, 0x22, 0x22, 0x41, 0x7f} }, // triangle
       { {0x7f, 0x41, 0x22, 0x22, 0x14, 0x14, 0x08} }, // inverted triangle
       { {0x7f, 0x41, 0x41, 0x41, 0x41, 0x41, 0x7f} }, // square
       { {0x00, 0x00, 0x1c, 0x14, 0x1c, 0x00, 0x00} }, // dot
       { {0x08, 0x08, 0x08, 0x7f, 0x08, 0x08, 0x08} }, // cross (+)
    };
}


// Show a glyph (from the list) centered at x,y.
//
void
QTdraw::ShowGlyph(int gnum, int x, int y)
{
    gnum = gnum % (sizeof(glyphs)/sizeof(glymap));
    glymap *g = &glyphs[gnum];
    gd_viewport->draw_glyph(x, y, g->bits, GlyphWidth);
}


GRobject
QTdraw::GetRegion(int, int, int, int)
{
    return (0);
}


void
QTdraw::PutRegion(GRobject, int, int, int, int)
{
}


void
QTdraw::FreeRegion(GRobject)
{
}
// End of QTdraw functions

