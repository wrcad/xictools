
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
#include "qtcanvas.h"
#ifdef WITH_X11
#include "qtcanvas_x.h"
#endif

#include <QApplication>

using namespace qtinterf;


draw_if *
draw_if::new_draw_interface(DrawType type, bool use_common, QWidget *parent)
{
    (void)use_common; //XXX
    if (type == DrawGL) {
        fprintf(stderr,
            "GL is not currently supported, using native QT graphics.\n");
        type = DrawNative;
    }
    if (type == DrawX) {
#ifdef WITH_X11
        return (new QTcanvas_x(use_common, parent));
#else
        fprintf(stderr,
            "X11 is not currently supported, using native QT graphics.\n");
        type = DrawNative;
#endif
    }
    return (new QTcanvas(parent));
}


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
// sGdraw functions, ghost rendering

// Set up ghost drawing.  Whenever the pointer moves, the callback is
// called with the current position and the x, y reference.
//
void
sGdraw::set_ghost(GhostDrawFunc callback, int x, int y)
{
    if (callback) {
        gd_draw_ghost = callback;
        gd_ref_x = x;
        gd_ref_y = y;
        gd_last_x = 0;
        gd_last_y = 0;
        gd_ghost_cx_cnt = 0;
        gd_first_ghost = true;
        gd_show_ghost = true;
        gd_undraw = false;
        return;
    }
    if (gd_draw_ghost) {
        if (!gd_first_ghost) {
            // undraw last
            gd_linedb = new GRlineDb;
            (*gd_draw_ghost)(gd_last_x, gd_last_y, gd_ref_x, gd_ref_y,
                gd_undraw);
            delete gd_linedb;
            gd_linedb = 0;
            gd_undraw ^= true;
        }
        gd_draw_ghost = 0;
    }
}


/*XXX
// Below, show_ghost is called after every display refresh.  If the
// warp pointer call is made directly, when dragging a window across
// the main drawing area, this causes ugly things to happen to the
// display, particularly when running over a network.  Therefor we put
// the warp pointer call in a timeout which will cut the call
// frequency way down.
//
namespace {
    int ghost_timer_id;

    int ghost_timeout(void*)
    {
        ghost_timer_id = 0;

        // This redraws ghost objects.
        int x0, y0;
        GdkScreen *screen;
        GdkDisplay *display = gdk_display_get_default();
        gdk_display_get_pointer(display, &screen, &x0, &y0, 0);
        gdk_display_warp_pointer(display, screen, x0, y0);
        return (0);
    }
}
*/


// Turn on/off display of ghosting.  Keep track of calls in ghostcxcnt.
//
void
sGdraw::show_ghost(bool show)
{
    if (!show) {
        if (!gd_ghost_cx_cnt) {
            undraw_ghost(false);
            gd_show_ghost = false;
            gd_first_ghost = true;
        }
        gd_ghost_cx_cnt++;
    }
    else {
        if (gd_ghost_cx_cnt)
            gd_ghost_cx_cnt--;
        if (!gd_ghost_cx_cnt) {
            gd_show_ghost = true;

            // The warp_pointer call mungs things if called too
            // frequently, so we put it in a timeout.

/*XXX
            if (ghost_timer_id)
                g_source_remove(ghost_timer_id);
            ghost_timer_id = g_timeout_add(100, ghost_timeout, 0);
*/
        }
    }
}


// Erase the last ghost.
//
void
sGdraw::undraw_ghost(bool reset)
{
    if (gd_draw_ghost && gd_show_ghost && gd_undraw && !gd_first_ghost) {
        gd_linedb = new GRlineDb;
        (*gd_draw_ghost)(gd_last_x, gd_last_y, gd_ref_x, gd_ref_y, gd_undraw);
        delete gd_linedb;
        gd_linedb = 0;
        gd_undraw ^= true;
        if (reset)
            gd_first_ghost = true;
    }
}


// Draw a ghost at x, y.
//
void
sGdraw::draw_ghost(int x, int y)
{
    if (gd_draw_ghost && gd_show_ghost && !gd_undraw) {
        gd_last_x = x;
        gd_last_y = y;
        gd_linedb = new GRlineDb;
        (*gd_draw_ghost)(x, y, gd_ref_x, gd_ref_y, gd_undraw);
        delete gd_linedb;
        gd_linedb = 0;
        gd_undraw ^= true;
        gd_first_ghost = false;
    }
}
// End of sGbag functions


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


// Move the pointer by x, y relative to current position, if absolute
// is false.  If true, move to given location.
//
void
QTdraw::MovePointer(int x, int y, bool absolute)
{
    // Called with 0,0 this redraws ghost objects
    if (absolute)
        QCursor::setPos(x, y);
    else {
        QPoint qp = QCursor::pos();
        QCursor::setPos(qp.x() + x, qp.y() + y);
    }
}


void
QTdraw::QueryPointer(int *x, int *y, unsigned *state)
{
    QPoint ptg(QCursor::pos());
    QPoint ptw = Viewport()->mapFromGlobal(ptg);
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
QTdraw::SetGhostColor(int pixel)
{
    gd_viewport->set_ghost_color(pixel);
}


void
QTdraw::Input(int*, int*, int*, int*)
{
}


void
QTdraw::SetXOR(int val)
{
    switch (val) {
    case GRxNone:
        gd_viewport->set_xor_mode(false);
        break;
    case GRxXor:
        gd_viewport->set_xor_mode(true);
        break;
    case GRxHlite:
    case GRxUnhlite:
        break;
    }
}


void
QTdraw::ShowGlyph(int, int, int)
{
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

