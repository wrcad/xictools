
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
 $Id: ghost.cc,v 5.104 2016/05/28 06:24:30 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ghost.h"
#include "dsp_inlines.h"
#include "events.h"


/*==================================================================*
 *
 *  The ghost interface: function for XOR highlighting.
 *
 *==================================================================*/

// NOTE:
// We can't count on exclusive-OR drawing working with text.  It works
// fine for simple bitmap drawing, as in X-Windows, but fails for Pango
// rendering which may include anti-aliasing.  If the ghost rendering
// includes text, we have to pass the "erase" boolean and actually
// refresh drawing area.


#define DEFAULT_FUNC ghost_snap_point

cGhost *cGhost::instancePtr = 0;


cGhost::cGhost()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cGhost already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    g_level = 0;
    g_started = false;
    g_ghost_line_vert = false;

    RegisterFunc(GFline, ghost_line_snap);
    RegisterFunc(GFline_ns, ghost_line);
    RegisterFunc(GFvector, ghost_vector_snap);
    RegisterFunc(GFvector_ns, ghost_vector);
    RegisterFunc(GFbox, ghost_box_snap);
    RegisterFunc(GFbox_ns, ghost_box);
    RegisterFunc(GFzoom, ghost_zoom);
    RegisterFunc(GFruler, ghost_ruler);
    RegisterFunc(GFscript, ghost_script);
};


// Private static error exit.
//
void
cGhost::on_null_ptr()
{
    fprintf(stderr, "Singleton class cGhost used before instantiated.\n");
    exit(1);
}


void
cGhost::RegisterFunc(GFtype type, GhostDrawFunc dfunc, GhostSetupFunc sfunc)
{
    g_functions[(int)type].draw_func = dfunc;
    g_functions[(int)type].setup_func = sfunc;
}


// Set the drawing function according to the mode passed.
//
void
cGhost::SetGhost(GFtype gmode, GhostMode mode)
{
    if (!g_started)
        return;

    if (gmode == GFzoom)
        mode = GhostZoom;
    else if (mode == GhostZoom)
        mode = GhostVanilla;

    int x, y;
    switch (gmode) {
    case GFnone:
        pop();
        break;
    case GFline_ns:
    case GFvector_ns:
    case GFbox_ns:
    case GFruler:
    case GFzoom:
        if ((mode == GhostAltOnly || mode == GhostAltIncluded) &&
                EV()->Cursor().get_press_alt())
            EV()->Cursor().get_alt_down(&x, &y);
        else
            EV()->Cursor().get_raw(&x, &y);
        push(gmode, x, y, mode);
        break;
    default:
        if ((mode == GhostAltOnly || mode == GhostAltIncluded) &&
                EV()->Cursor().get_press_alt()) {
            EV()->Cursor().get_alt_down(&x, &y);
            EV()->ButtonWin()->Snap(&x, &y);
        }
        else
            EV()->Cursor().get_xy(&x, &y);
        push(gmode, x, y, mode);
        break;
    }
}


// Start ghosting, as if the last button press was at x, y.
//
void
cGhost::SetGhostAt(GFtype gmode, int x, int y, GhostMode mode)
{
    if (gmode == GFzoom)
        mode = GhostZoom;
    else if (mode == GhostZoom)
        mode = GhostVanilla;

    switch (gmode) {
    case GFnone:
        pop();
        break;
    case GFline_ns:
    case GFvector_ns:
    case GFbox_ns:
    case GFruler:
    case GFzoom:
        push(gmode, x, y, mode);
        break;
    default:
        EV()->ButtonWin()->Snap(&x, &y);
        push(gmode, x, y, mode);
        break;
    }

    // Set the reference location to x,y so that the ^E coordinate
    // entry with the relative option ('+') will be relative to
    // x,y.

    EV()->SetReference(x, y);
}


// Save the current drawing function context.
//
void
cGhost::SaveGhost()
{
    if (g_level)
        g_saved_context = g_context[g_level - 1];
    else
        g_saved_context.set(GFnone, DEFAULT_FUNC, 0, 0, GhostVanilla);
}


// Restore the saved drawing function context.
//
void
cGhost::RestoreGhost()
{
    if (g_saved_context.func == 0)
        g_saved_context.set(GFnone, DEFAULT_FUNC, 0, 0, GhostVanilla);
    pop();
    if (g_level) {
        if (!(g_saved_context == g_context[g_level-1]))
            push();
    }
    else {
        if (g_saved_context.func != DEFAULT_FUNC)
            push();
    }
}


void
cGhost::RepaintGhost()
{
    DSPmainDraw(MovePointer(0, 0, false))
}


void
cGhost::BumpGhostPointer(int gmode)
{
    switch (gmode) {
    case GFdisk:
    case GFdonut:
    case GFarc:
    case GFdiskpth:
    case GFarcpth:
        DSPmainDraw(MovePointer(10, 10, false))
        break;
    case GFbox_ns:
    case GFzoom:
    case GFpterms:
        DSPmainDraw(MovePointer(5, 5, false))
        break;
    default:
        break;
    }
}


// Return true if ghosting is allowed in the passed window.
//
bool
cGhost::ShowingGhostInWindow(WindowDesc *wd)
{
    GhostMode mode =
        g_level > 0 ? g_context[g_level - 1].mode : GhostSnapPoint;
    if (mode == GhostVanilla)
        return (wd->IsSimilar(DSP()->MainWdesc()));
    if (mode == GhostZoom) {
        WindowDesc *wz = EV()->ZoomWin();
        if (wz)
            return (wd->IsSimilar(wz));
    }
    else if (mode == GhostAltOnly) {
        WindowDesc *wa = EV()->ButtonWin(true);
        if (!wa)
            wa = DSP()->MainWdesc();
        if (wd->IsSimilar(wa))
            return (true);
    }
    else if (mode == GhostAltIncluded) {
        WindowDesc *wc = EV()->CurrentWin();
        if (wc && wd->IsSimilar(wc)) {
            WindowDesc *wm =  DSP()->MainWdesc();
            if (wd->IsSimilar(wm))
                return (true);
            WindowDesc *wa = EV()->ButtonWin(true);
            if (wa && wa != wm && wd->IsSimilar(wa))
                return (true);
        }
    }
    else if (mode == GhostSnapPoint) {
        // The mark should always appear, for the coordinates display.
        return (true);
    }
    return (false);
}


void
cGhost::ShowGhostBox(int x, int y, int refx, int refy)
{
    WindowDesc *wd;
    WDgen wgen(WDgen::MAIN, WDgen::CHD);
    while ((wd = wgen.next()) != 0) {
        if (ShowingGhostInWindow(wd))
            wd->ShowLineBox(x, y, refx, refy);
    }
}


void
cGhost::ShowGhostPath(const Point *points, int numpts)
{
    if (numpts <= 1)
        return;
    WindowDesc *wd;
    WDgen wgen(WDgen::MAIN, WDgen::CHD);
    while ((wd = wgen.next()) != 0) {
        if (ShowingGhostInWindow(wd))
            wd->ShowLinePath(points, numpts);
    }
}


void
cGhost::ShowGhostCross(int x, int y, int pixels, bool do45)
{
    WindowDesc *wd;
    WDgen wgen(WDgen::MAIN, WDgen::CHD);
    while ((wd = wgen.next()) != 0) {
        if (ShowingGhostInWindow(wd))
            wd->ShowCross(x, y, pixels, do45);
    }
}


// Private functions.
//

// Set a new drawing function.
//
void
cGhost::push(GFtype type, int x, int y, GhostMode m)
{
    if (g_level < GHOST_LEVELS-1) {
        DSPmainDraw(SetGhost(0, 0, 0))
        gf_t &gf = g_functions[(int)type]; 
        g_context[g_level].set(type, gf.draw_func, x, y, m);
        g_level++;
        if (gf.setup_func)
            (*gf.setup_func)(true);
        setghost();
    }
}


// Push back the saved drawing function.
//
void
cGhost::push()
{
    if (g_level < GHOST_LEVELS-1) {
        DSPmainDraw(SetGhost(0, 0, 0))
        g_context[g_level] = g_saved_context;
        g_level++;
        setghost();
    }
}


// Revert to the previous, or the default drawing function.
//
void
cGhost::pop()
{
    DSPmainDraw(SetGhost(0, 0, 0))
    if (g_level) {
        GFtype type = g_context[g_level-1].type;
        gf_t &gf = g_functions[(int)type]; 
        if (gf.setup_func)
            (*gf.setup_func)(false);
        g_level--;
    }
    setghost();
}


// Actually install the current drawing function, or the default if a true
// argument is passed.
//
void
cGhost::setghost()
{
    if (g_level == 0)
        DSPmainDraw(SetGhost(DEFAULT_FUNC, 0, 0))
    else {
        ghparm_t *g = &g_context[g_level-1];
        if (g->func) {
            DSPmainDraw(SetGhost(g->func, g->x, g->y))
            if (    g->type == GFstretch ||
                    g->type == GFrotate ||
                    g->type == GFplace ||
                    g->type == GFlabel ||
                    g->type == GFput ||
                    g->type == GFscript)
                DSPmainDraw(MovePointer(0, 0, false))
        }
    }
}


// The (static) drawing functions.  The functions passed to
// RegisterFunction should be similar to these.

// Static function.
void
cGhost::ghost_snap_point(int x, int y, int, int, bool)
{
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    WindowDesc *wdesc = EV()->CurrentWin();
    wdesc->PToL(x, y, x, y);
    wdesc->Snap(&x, &y, true);

    if (XM()->IsFullWinCursor()) {
        if (wdesc->IsXSect()) {
            int x1, y1;
            wdesc->LToP(x, y, x1, y1);
            int yb = wdesc->Viewport().bottom;
            int yt = wdesc->Viewport().top;
            wdesc->ShowLineV(x1, yb, x1, yt);
            int xl = wdesc->Viewport().left;
            int xr = wdesc->Viewport().right;
            wdesc->ShowLineV(xl, y1, xr, y1);
        }
        else {
            WDgen wgen(WDgen::MAIN, WDgen::CHD);
            while ((wdesc = wgen.next()) != 0) {
                if (wdesc->IsSimilar(DSP()->MainWdesc())) {

                    int x1, y1;
                    wdesc->LToP(x, y, x1, y1);
                    int yb = wdesc->Viewport().bottom;
                    int yt = wdesc->Viewport().top;
                    wdesc->ShowLineV(x1, yb, x1, yt);
                    int xl = wdesc->Viewport().left;
                    int xr = wdesc->Viewport().right;
                    wdesc->ShowLineV(xl, y1, xr, y1);
                }
            }
        }
    }
    else {
        // Done in viewport coords so mark doesn't disappear at high
        // magnification.
        //
        wdesc->LToP(x, y, x, y);
        int delta = 2;
        wdesc->ShowLineV(x-delta, y-delta, x-delta, y+delta);
        wdesc->ShowLineV(x-delta+1, y+delta, x+delta-1, y+delta);
        wdesc->ShowLineV(x+delta, y+delta, x+delta, y-delta);
        wdesc->ShowLineV(x+delta-1, y-delta, x-delta+1, y-delta);
    }
    DSP()->TPop();
}


// Static function.
void
cGhost::ghost_line(int x, int y, int, int, bool)
{
    cGhost *ghost = Gst();
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::CHD);
    while ((wdesc = wgen.next()) != 0) {
        if (ghost->ShowingGhostInWindow(wdesc)) {
            if (ghost->GhostLineVert())
                wdesc->ShowLineW(x, wdesc->Window()->bottom,
                    x, wdesc->Window()->top);
            else
                wdesc->ShowLineW(wdesc->Window()->left, y,
                    wdesc->Window()->right, y);
        }
    }
    DSP()->TPop();
}


// Static function.
void
cGhost::ghost_line_snap(int x, int y, int, int, bool)
{
    cGhost *ghost = Gst();
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    EV()->CurrentWin()->Snap(&x, &y, true);
    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::CHD);
    while ((wdesc = wgen.next()) != 0) {
        if (ghost->ShowingGhostInWindow(wdesc)) {
            if (ghost->GhostLineVert())
                wdesc->ShowLineW(x, wdesc->Window()->bottom,
                    x, wdesc->Window()->top);
            else
                wdesc->ShowLineW(wdesc->Window()->left, y,
                    wdesc->Window()->right, y);
        }
    }
    DSP()->TPop();
}


// Static function.
void
cGhost::ghost_vector(int x, int y, int refx, int refy, bool)
{
    cGhost *ghost = Gst();
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    if (XM()->To45snap(&x, &y, refx, refy)) {
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CHD);
        while ((wdesc = wgen.next()) != 0) {
            if (ghost->ShowingGhostInWindow(wdesc))
                wdesc->ShowLineW(x, y, refx, refy);
        }
    }
    DSP()->TPop();
}


// Static function.
void
cGhost::ghost_vector_snap(int x, int y, int refx, int refy, bool)
{
    cGhost *ghost = Gst();
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    EV()->CurrentWin()->Snap(&x, &y);
    if (XM()->To45snap(&x, &y, refx, refy)) {
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CHD);
        while ((wdesc = wgen.next()) != 0) {
            if (ghost->ShowingGhostInWindow(wdesc))
                wdesc->ShowLineW(x, y, refx, refy);
        }
    }
    DSP()->TPop();
}


// Static function.
void
cGhost::ghost_box(int x, int y, int refx, int refy, bool)
{
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    Gst()->ShowGhostBox(x, y, refx, refy);
    DSP()->TPop();
}


// Static function.
void
cGhost::ghost_box_snap(int x, int y, int refx, int refy, bool)
{
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    EV()->CurrentWin()->Snap(&x, &y, true);
    Gst()->ShowGhostBox(x, y, refx, refy);
    DSP()->TPop();
}


// Static function.
void
cGhost::ghost_zoom(int x, int y, int refx, int refy, bool)
{
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    Gst()->ShowGhostZoom(x, y, refx, refy);
    DSP()->TPop();
}


// Static function.
void
cGhost::ghost_ruler(int x, int y, int refx, int refy, bool erase)
{
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    Gst()->ShowGhostRuler(x, y, refx, refy, erase);
    DSP()->TPop();
}


// Static function.
void
cGhost::ghost_script(int x, int y, int refx, int refy, bool)
{
    if (!EV()->CurrentWin())
        return;
    DSP()->TPush();

    EV()->CurrentWin()->PToL(x, y, x, y);
    EV()->CurrentWin()->Snap(&x, &y);
    Gst()->ShowGhostScript(x, y, refx, refy);
    DSP()->TPop();
}

