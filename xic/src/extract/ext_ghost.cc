
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
 $Id: ext_ghost.cc,v 5.9 2009/08/01 05:53:44 stevew Exp $
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

