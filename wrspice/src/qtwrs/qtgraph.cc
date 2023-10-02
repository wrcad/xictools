
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1988 Jeffrey M. Hsu
         1995 Stephen R. Whiteley
****************************************************************************/

#include "qtplot.h"
#include "graph.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "simulator.h"
#include "qttoolb.h"
#include "qtinterf/qtfont.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "spnumber/spnumber.h"

//
// The graphics-package (QT) dependent part of the sGraph class.
//


// Return a new graphics context struct.
//
GRwbag *
sGraph::gr_new_gx(int type)
{
    return (new QTplotDlg(type));
}


// Initialization of graphics, return false on success.
//
int
sGraph::gr_pkg_init()
{
    if (!gr_dev)
        return (true);

    QTplotDlg *w = dynamic_cast<QTplotDlg*>(gr_dev);
    return (w->init(this));
}


// Fill in the current colors in the graph from the DefColors array,
// which is updated.
//
void
sGraph::gr_pkg_init_colors()
{
    SpGrPkg::SetDefaultColors();
    const sGraph *tgraph = this;
    if (!tgraph)
        return;

    QTplotDlg *w = dynamic_cast<QTplotDlg*>(gr_dev);
    if (w && w->Viewport()) {
        w->SetWindowBackground(SpGrPkg::DefColors[0].pixel);
        w->SetBackground(SpGrPkg::DefColors[0].pixel);
        w->Clear();
    }
    for (int i = 0; i < GRpkg::self()->MainDev()->numcolors; i++)
        gr_colors[i] = SpGrPkg::DefColors[i];
}


// This rebuilds the button array after adding/removing a trace.  The
// return is true if the button count changes
//
bool
sGraph::gr_init_btns()
{
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(gr_dev);
    if (w)
        return (w->init_gbuttons());
    return (false);
}


// This is called periodically while a plot is being drawn.  If a button
// press or exposure event is detected for the plot window, return true.
//
bool
sGraph::gr_check_plot_events()
{
    /*XXX can QT do this?
    GdkEvent *ev;
    while ((ev = gdk_event_peek()) != 0) {
        if (QTplotDlg::check_event(ev, this)) {
            gdk_event_free(ev);
            return (true);
        }
        gdk_event_free(ev);
        if ((ev = gdk_event_get()) != 0)
            gtk_main_do_event(ev);
    }
    */
    return (false);
}


void
sGraph::gr_redraw()
{
    if (GRpkg::self()->CurDev()->devtype == GRhardcopy) {
        gr_redraw_direct();
        gr_redraw_keyed();
        return;
    }
    QTplotDlg *wb = dynamic_cast<QTplotDlg*>(gr_dev);

    int width = wb->Viewport()->width();
    int height = wb->Viewport()->height();
    area().set_width(width);
    area().set_height(height);
    wb->SetColor(gr_colors[0].pixel);
    wb->Box(0, 0, width, height);
    gr_redraw_direct();  // This might change area().width/area().height.

    // Save the un-annotated plot pixmap for refreshing behind
    // user-editable annotation.
    wb->DrawIf()->create_overlay_backg();

    gr_redraw_keyed();
    gr_dirty = false;
    wb->Update();
}


void
sGraph::gr_refresh(int left, int bottom, int right, int top, bool notxt)
{
    if (gr_dirty) {
        gr_redraw();
        return;
    }
    QTplotDlg *wb = dynamic_cast<QTplotDlg*>(gr_dev);
    area().set_width(wb->Viewport()->width());
    area().set_height(wb->Viewport()->height());
    if (!notxt) {
        wb->DrawIf()->set_clipping(left, top, right-left + 1, bottom-top + 1);
        gr_redraw_keyed();
        wb->DrawIf()->set_clipping(0, 0, 0, 0);
    }
    wb->Update(left, top, right-left + 1, bottom-top + 1);
}


// Pop down and destroy.
//
void
sGraph::gr_popdown()
{
    QTplotDlg *w = dynamic_cast<QTplotDlg*>(dev());
    delete w;
    set_dev(0);
    GP.DestroyGraph(id());
}
// End of sGraph functions.

