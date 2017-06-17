
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
 $Id: sced_ghost.cc,v 5.7 2009/08/01 05:53:54 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "sced.h"
#include "ghost.h"
#include "events.h"


//-----------------------------------------------------------------------------
// Ghost drawing setup


cScedGhost::cScedGhost()
{
    Gst()->RegisterFunc(GFdiskpth, ghost_diskpth);
    Gst()->RegisterFunc(GFarcpth, ghost_arcpth);
    Gst()->RegisterFunc(GFshape, ghost_shape);
    Gst()->RegisterFunc(GFeterms, ghost_elec_terms);
}


void
cScedGhost::ghost_diskpth(int x, int y, int refx, int refy, bool)
{
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    EV()->CurrentWin()->Snap(&x, &y, true);
    SCD()->SGst()->showGhostDiskPath(x, y, refx, refy);
    DSP()->TPop();
}


void
cScedGhost::ghost_arcpth(int x, int y, int refx, int refy, bool)
{
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    EV()->CurrentWin()->Snap(&x, &y, true);
    SCD()->SGst()->showGhostArcPath(x, y, refx, refy);
    DSP()->TPop();
}


void
cScedGhost::ghost_shape(int x, int y, int refx, int refy, bool)
{
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    EV()->CurrentWin()->Snap(&x, &y);
    SCD()->SGst()->showGhostShape(x, y, refx, refy);
    DSP()->TPop();
}


void
cScedGhost::ghost_elec_terms(int x, int y, int refx, int refy, bool)
{
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    EV()->CurrentWin()->Snap(&x, &y);
    SCD()->SGst()->showGhostElecTerms(x, y, refx, refy);
    DSP()->TPop();
}

