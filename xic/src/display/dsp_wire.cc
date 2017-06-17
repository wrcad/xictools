
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: dsp_wire.cc,v 1.15 2012/05/10 05:21:11 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_layer.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"


//
// Function to display wires.
//

// Display a wire in the window, with type attributes and fill
// pattern fillpat.
//
void
WindowDesc::ShowWire(const Wire *wire, int type, const GRfillType *fillpat)
{
    if (!w_draw)
        return;
    if (wire->numpts <= 0)
        return;
    if (wire->wire_width()*w_ratio <= 2) {
        ShowPath(wire->points, wire->numpts, false);
        return;
    }

    Poly poly;
    if (wire->toPoly(&poly.points, &poly.numpts)) {
        ShowPolygon(&poly, type, fillpat, 0);
        delete [] poly.points;
    }
}


// Function to show a line path, for highlighting and ghosting.
//
void
WindowDesc::ShowLinePath(const Point *points, int numpts)
{
    if (!w_draw)
        return;
    if (numpts <= 1)
        return;
    ShowPath(points, numpts, false);
}

