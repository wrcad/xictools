
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
#include "ext.h"
#include "events.h"
#include "ghost.h"


//-----------------------------------------------------------------------------
// Ghost drawing setup

cExtGhost::cExtGhost()
{
    Gst()->RegisterFunc(GFpterms, ghost_phys_terms);
    Gst()->RegisterFunc(GFmeasbox, ghost_meas_box);
}


void
cExtGhost::ghost_phys_terms(int x, int y, int refx, int refy, bool)
{
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    EV()->CurrentWin()->Snap(&x, &y);
    EX()->XGst()->showGhostPhysTerms(x, y, refx, refy);
    DSP()->TPop();
}


void
cExtGhost::ghost_meas_box(int x, int y, int refx, int refy, bool erase)
{
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    EV()->CurrentWin()->Snap(&x, &y);
    Gst()->ShowGhostBox(x, y, refx, refy);
    EX()->XGst()->showGhostMeasure(x, y, refx, refy, erase);
    DSP()->TPop();
}

