
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#include "main.h"
#include "edit.h"
#include "tech.h"
#include "dsp_inlines.h"
#include "events.h"
#include "ghost.h"


// The following functions implement the "constrain 45" directive for
// creating wires and polygons.  Unless in simple mode, an additional
// Manhattan segment is generated which connects the 45 point to the
// actual point.  The mode is set by the ed_override_45 and
// ed_simple_45 variables, in conjunction with the Tech()->Constrain45()
// setting.  The Override variable is intended to be set with the
// Shift key, and ed_simple_45 with the Ctrl key.  Since simultaneous
// Shift-Ctrl is not allowed (simulates button4) the logic is a little
// complicated:
//
//
// !Tech()->IsConstrain45():
//             !Shift      Shift
//     !Ctrl   no45        reg45
//     Ctrl    simp45      simp45
//
// Tech()->IsConstrain45():
//             !Shift      Shift
//     !Ctrl   reg45       no45
//     Ctrl    simp45      no45


// Switch between "simple" path (only generate one point) and
// standard operation.
//
void
cEdit::pthSetSimple(bool simp, bool override)
{
    ed_45.simple_45 = simp;
    ed_45.override_45 = override;
}


// Set the initial intermediate point.
//
void
cEdit::pthSet(int x, int y)
{
    ed_45.int_x = x;
    ed_45.int_y = y;
}


// Get the intermediate point.
//
void
cEdit::pthGet(int *x, int *y)
{
    *x = ed_45.int_x;
    *y = ed_45.int_y;
}
// End of cEdit functions


//----------------
// Ghost Rendering

// Functions to show a path extension.  If the 45's constraint is on,
// the path consists of two "rubber band" segments.
//
void
cEditGhost::showGhostPathSeg(int x, int y, int refx, int refy)
{
    WindowDesc *wdesc = EV()->CurrentWin();
    if (!wdesc)
        return;
    sEdit45 &e45 = ED()->state45();

    if ((!Tech()->IsConstrain45() && !e45.override_45 && !e45.simple_45) ||
            (Tech()->IsConstrain45() && e45.override_45)) {
        // no constraint, set ed_int_x/y so PthGet() won't return crap
        EV()->Cursor().get_xy(&e45.int_x, &e45.int_y);

        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (!wdesc->IsSimilar(DSP()->MainWdesc()))
                continue;
            wdesc->ShowLineW(x, y, refx, refy);
        }
        return;
    }
    int delta = (int)(20.0/wdesc->Ratio());
    int snp;
    if (wdesc->Mode() == Physical)
        snp = INTERNAL_UNITS(
            wdesc->Attrib()->grid(Physical)->spacing(Physical));
    else
        snp = ELEC_INTERNAL_UNITS(
            wdesc->Attrib()->grid(Electrical)->spacing(Electrical));
    snp += snp/2;
    if (delta < snp)
        delta = snp;

    if (e45.simple_45 || (abs(x - refx) < delta && abs(y - refy) < delta) ||
            x == refx || y == refy ||
            (e45.int_x == refx && e45.int_y == refy)) {
        // set the slope of the initial segment
        e45.int_x = x;
        e45.int_y = y;
        XM()->To45(refx, refy, &e45.int_x, &e45.int_y);
        if (e45.simple_45) {
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0) {
                if (!wdesc->IsSimilar(DSP()->MainWdesc()))
                    continue;
                wdesc->ShowLineW(refx, refy, e45.int_x, e45.int_y);
            }
            return;
        }
    }
    for (;;) {
        if (e45.int_x == refx)
            e45.int_y = y;
        else if (e45.int_y == refy)
            e45.int_x = x;
        else {
            if (abs(x-refx) < abs(y-refy)) {
                e45.int_y = refy +
                    (x - refx)*((e45.int_y - refy)/(e45.int_x - refx));
                e45.int_x = x;
            }
            else {
                e45.int_x = refx +
                    (y - refy)*((e45.int_x - refx)/(e45.int_y - refy));
                e45.int_y = y;
            }
        }
        // If quadrants change without landing on a boundary point, the
        // angle can be wrong.  Check this here.
        //
        if (e45.int_x - refx == e45.int_y - refy &&
                ((x - refx > 0 && y - refy < 0) ||
                (x - refx < 0 && y - refy > 0))) {
            e45.int_x = x;
            e45.int_y = y;
            XM()->To45(refx, refy, &e45.int_x, &e45.int_y);
            continue;
        }
        if (e45.int_x - refx == -(e45.int_y - refy) &&
                ((x - refx > 0 && y - refy > 0) ||
                (x - refx < 0 && y - refy < 0))) {
            e45.int_x = x;
            e45.int_y = y;
            XM()->To45(refx, refy, &e45.int_x, &e45.int_y);
            continue;
        }
        break;
    }

    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wdesc = wgen.next()) != 0) {
        if (!wdesc->IsSimilar(DSP()->MainWdesc()))
            continue;
        if (e45.int_x != refx || e45.int_y != refy)
            wdesc->ShowLineW(refx, refy, e45.int_x, e45.int_y);
        if (e45.int_x != x || e45.int_y != y)
            wdesc->ShowLineW(e45.int_x, e45.int_y, x, y);
    }
}

