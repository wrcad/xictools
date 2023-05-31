
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

#include "config.h"
#include "dsp.h"
#include "dsp_window.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "fio.h"
#include "fio_chd.h"
#include "cd_digest.h"
#include "miscutil/timedbg.h"


//
// Display queue management.
//

// Generally, a backing pixmap (X-server side) is used, as well as a
// (client-side) image.  "Updates" refresh an area from the backing
// pixmap.  "Redisplays" actually redraw a region.  Use of a backing
// pixmap and/or in-core image can be turned off if necessary.
//
// Usually, all updates and redisplays are handled by an idle function
// and region queues, though there is provision for synchronous
// redisplay.  The display queue management is performed by the
// functions in this file.


// Redisplay an area of the current mode/cell.
//
void
cDisplay::RedisplayArea(const BBox *BB, int mode)
{
    if (mode < 0) {
        if (!MainWdesc())
            return;
        mode = MainWdesc()->Mode();
    }
    for (int i = 0; i < DSP_NUMWINS; i++) {
        WindowDesc *wd = d_windows[i];
        if (wd && wd->IsSimilar((DisplayMode)mode, MainWdesc()))
            wd->Redisplay(BB);
    }
}


// Redisplay all windows showing mode.
//
void
cDisplay::RedisplayAll(int mode)
{
    if (mode < 0) {
        if (!MainWdesc())
            return;
        mode = MainWdesc()->Mode();
    }
    for (int i = 0; i < DSP_NUMWINS; i++) {
        WindowDesc *wd = d_windows[i];
        if (wd && wd->Mode() == mode)
            wd->Redisplay(wd->Window());
    }
}


// Interrupt handler, stop all drawing.
//
void
cDisplay::RedisplayAfterInterrupt()
{
    if (Interrupt() == DSPinterUser) {
        notify_interrupted();
        if (DoingHcopy())
            DSPpkg::self()->HCabort("User abort");
    }
    else if (Interrupt() == DSPinterSys) {
        if (DoingHcopy())
            DSPpkg::self()->HCabort("System abort");
    }
    SetInterrupt(DSPinterNone);

    for (int i = 0; i < DSP_NUMWINS; i++) {
        WindowDesc *wd = d_windows[i];
        if (wd)
            wd->ClearPending();
    }
}


// Private function to start the redisplay idle procedure.
//
void
cDisplay::QueueRedisplay()
{
    if (d_redisplay_queued)
        return;
    DSPpkg::self()->RegisterIdleProc(RedisplayIdleProc, this);
    d_redisplay_queued = true;
}


// Static idle procedure that actually performs redisplays.
//
int
cDisplay::RedisplayIdleProc(void*)
{
    for (int i = 0; i < DSP_NUMWINS; i++) {
        WindowDesc *wd = DSP()->d_windows[i];
        if (wd)
            wd->RunPending();
    }
    DSP()->d_redisplay_queued = false;
    for (int i = 0; i < DSP_NUMWINS; i++) {
        WindowDesc *wd = DSP()->d_windows[i];
        if (wd) {

            // NeedRedisplay is set when the viewport changes (on
            // window resize) when this occurs while drawing.  In this
            // case, the interupt flag was set so we have quit drawing
            // prematurely, and have to redraw everything.  The
            // Redisplay call must come after d_redisplay_queued is
            // set false.

            if (wd->NeedRedisplay()) {
                wd->SetNeedRedisplay(false);
                wd->Redisplay(0);
            }
        }
    }
    return (false);
}
// End of cDisplay functions


// Register an area of the viewport to update, i.e., restore from the
// backing pixmap.  This is used by the expose handler.  The AOI uses
// pixel coordinates.
//
void
WindowDesc::Update(const BBox *AOI, bool flush)
{
    if (DSP()->NoGraphics())
        return;
    if (!AOI)
        AOI = &Viewport();
    add_update_region(AOI);
    DSP()->QueueRedisplay();
    if (flush)
        cDisplay::RedisplayIdleProc(0);
}


// This is for ghost drawing, when the sprite contains text which must
// actually be cleared rather than drawn over in XOR mode.  The AOI
// uses Viewport coords.
//
// Note that this will not work unless backing store is enabled.
//
void
WindowDesc::GhostUpdate(const BBox *AOI)
{
    // These redraw the area from backing store.
    SwitchToPixmap();
    SwitchFromPixmap(AOI);

    // Controls updating to fix highlighting.  We defer this to avoid
    // rather horrible flashing effects.

    if (w_accum_mode == WDaccumStart) {
        // User just started ghost drawing, this is the first update.
        w_accum_rect = *AOI;
        w_accum_mode = WDaccumAccum;
    }
    else if (w_accum_mode == WDaccumAccum) {
        // User continues to ghost draw, keep track of the extent.
        // Remember!  This is pixel space.
        if (w_accum_rect.left > AOI->left)
            w_accum_rect.left = AOI->left;
        if (w_accum_rect.bottom < AOI->bottom)
            w_accum_rect.bottom = AOI->bottom;
        if (w_accum_rect.right < AOI->right)
            w_accum_rect.right = AOI->right;
        if (w_accum_rect.top > AOI->top)
            w_accum_rect.top = AOI->top;
    }
}


// Update the specified area (in window coordinates) from the backing
// pixmap, if available.  If no backing pixmap, redraw.
//
void
WindowDesc::Refresh(const BBox *AOI)
{
    if (!AOI)
        AOI = &w_window;
    BBox BB = *AOI;
    LToPbb(BB, BB);
    if (ViewportIntersect(BB, Viewport())) {
        ViewportClip(BB, Viewport());
        Update(&BB);
    }
}


// Update the list of regions, BBs in window coordinates.
//
void
WindowDesc::RefreshList(const Blist *list)
{
    if (!list)
        return;
    if (DSP()->NoGraphics() || DSP()->NoRedisplay())
        return;
    BBox BB;
    while (list) {
        LToPbb(list->BB, BB);
        if (ViewportIntersect(BB, Viewport())) {
            ViewportClip(BB, Viewport());
            Update(&BB);
        }
        list = list->next;
    }
}


// Register a region in the window to redisplay (window coordinates),
// and queue a redraw.
//
void
WindowDesc::Redisplay(const BBox *AOI)
{
    if (DSP()->NoGraphics() || DSP()->NoRedisplay())
        return;
    if (DSP()->SlowMode()) {
        RedisplayDirect(AOI);
        return;
    }
    if (!AOI)
        AOI = &w_window;
    add_redisp_region(AOI);
    DSP()->QueueRedisplay();
}


// Redisplay the list of regions.
//
void
WindowDesc::RedisplayList(const Blist *list)
{
    if (!list)
        return;
    if (DSP()->NoGraphics() || DSP()->NoRedisplay() || w_dbtype != WDcddb) 
        return;
    while (list) {
        add_redisp_region(&list->BB);
        list = list->next;
    }
    DSP()->QueueRedisplay();
}


// Directly redisplay the region given, or the entire window if AOI is
// 0.  Do not use caching or backing store.  This is used for
// hardcopy, screen dumps, etc.  If no_clear is true, the rendering
// area is not cleared before drawing, for hardcopy.
//
// The clip_ht, if nonzero, is an area to clip around at the bottom of
// the viewport.  This is a hack for the legend in hard-copy support.
//
void
WindowDesc::RedisplayDirect(const BBox *AOI, bool no_clear, int clip_ht)
{
    if (DSP()->NoGraphics() && !DSP()->DoingHcopy())
        return;
    if (!w_draw)
        return;
    DSP()->SetInterrupt(DSPinterNone);
    if (!AOI)
        AOI = &w_window;
    BBox BB = *AOI;

    bool nodrawold = false;
    AOItype at = set_current_aoi(&BB);
    if (at == AOInone)
        return;
    if (at == AOIfull && !no_clear)
        nodrawold = true;
    if (clip_ht > 0 && clip_ht < w_clip_rect.bottom)
        w_clip_rect.bottom -= clip_ht;

    PushRedisplay();

    // turn these off for the redisplay
    bool no_pixmap = DSP()->NoPixmapStore();
    bool no_cache = DSP()->NoDisplayCache();
    DSP()->SetNoPixmapStore(true);
    DSP()->SetNoDisplayCache(true);

    w_draw->ShowGhost(ERASE);
    bool working_set = DSP()->SlowMode() ||
        w_clip_rect.width() >= 100 || abs(w_clip_rect.height()) >= 100;
    if (working_set)
        DSPpkg::self()->SetWorking(true);

    Tdbg()->start_timing("RedisplayDirect");

    // Erase edit cell BB, the nodrawold suppresses redisplay of the
    // previous cell BB edges.
    bool bbflag = show_cellbb(ERASE, &BB, nodrawold);

    int numgeom = 0;
    if (w_frozen && !DSP()->DoingHcopy()) {
        ShowGrid();
        w_draw->SetFillpattern(0);
        w_draw->SetColor(DSP()->Color(HighlightingColor, w_mode));
        w_draw->Text("FROZEN", 10, 20, 0);
    }
    else
        numgeom = redisplay_geom(&BB, no_clear);

    RedisplayHighlighting(&BB, bbflag);

    w_draw->ShowGhost(DISPLAY);
    if (working_set)
        DSPpkg::self()->SetWorking(false);

    Tdbg()->stop_timing("RedisplayDirect", numgeom);

    DSP()->SetNoPixmapStore(no_pixmap);
    DSP()->SetNoDisplayCache(no_cache);
    PopRedisplay();
}


// Add the edge regions of the BB to the list passed.
//
Blist *
WindowDesc::AddEdges(Blist *bl, const BBox *BB)
{
    int delta = (int)(1.0/w_ratio);
    if (delta == 0)
        delta = 1;

    // Unless the cell is large, redisplay the usual way
    if (BB->width() < 100*delta || BB->height() < 100*delta)
        return (new Blist(BB, bl));

    // Left edge
    BBox eBB;
    eBB.left   = BB->left - delta;
    eBB.right  = BB->left + delta;
    eBB.top    = BB->top + delta;
    eBB.bottom = BB->bottom - delta;
    bl = new Blist(&eBB, bl);

    // Right edge
    eBB.right  = BB->right + delta;
    eBB.left   = BB->right - delta;
    eBB.top    = BB->top + delta;
    eBB.bottom = BB->bottom - delta;
    bl = new Blist(&eBB, bl);

    // Bottom edge
    eBB.bottom = BB->bottom - delta;
    eBB.top    = BB->bottom + delta;
    eBB.right  = BB->right;
    eBB.left   = BB->left;
    bl = new Blist(&eBB, bl);

    // Top edge
    eBB.bottom = BB->top - delta;
    eBB.top    = BB->top + delta;
    eBB.right  = BB->right;
    eBB.left   = BB->left;
    bl = new Blist(&eBB, bl);

    return (bl);
}


// Redraw the highlighting and other "transient" marks.  These are not
// drawn into the backing pixmap, so erasing is simple and
// inexpensive.
//
void
WindowDesc::RedisplayHighlighting(const BBox *AOI, bool bbflag)
{
    if (!w_draw)
        return;
    if (!AOI)
        AOI = &w_window;
    BBox BB = *AOI;
    if (set_current_aoi(&BB) == AOInone)
        return;
    if (!w_frozen || DSP()->DoingHcopy()) {
        // Show incomplete object
        if (DSP()->IncompleteObject() &&
                DSP()->IncompleteObject()->state() == CDobjIncomplete &&
                IsSimilar(DSP()->MainWdesc())) {
            CDl *ld = DSP()->IncompleteObject()->ldesc();
            w_draw->SetColor(dsp_prm(ld)->pixel());
            DisplayIncmplt(DSP()->IncompleteObject());
        }
        if (DSP()->Interrupt())
            DSP()->RedisplayAfterInterrupt();
        else if (!w_attributes.show_no_highlighting()) {
            // show highlighting, etc above grid
            if (w_dbtype == WDcddb) {
                ShowWindowMarks();
                ShowPhysProperties(&BB, DISPLAY);
                DSP()->window_show_highlighting(this);
                ShowHighlighting();  // blinking
            }
            else if (w_dbtype == WDchd) {
                DSP()->window_show_highlighting(this);
                ShowHighlighting();  // blinking
            }
        }
    }
    show_cellbb(DISPLAY, &BB, bbflag);
    w_clip_rect = Viewport();

    ShowRulers();
    // show subwindow locations
    if (this == DSP()->MainWdesc() && w_mode == Physical) {
        for (int i = 1; i < DSP_NUMWINS; i++) {
            WindowDesc *wd = DSP()->Window(i);
            if (wd && wd->w_mode == Physical && wd->w_show_loc &&
                    wd->CurCellName() == CurCellName()) {
                w_draw->SetColor(DSP()->Color(MarkerColor, w_mode));
                ShowBox(&wd->w_window, CDL_OUTLINED, 0);
            }
        }
    }

    // show main window location in first frozen subwindow
    if (w_frozen && this != DSP()->MainWdesc() && w_mode == Physical) {
        WindowDesc *wd = DSP()->MainWdesc();
        if (wd && wd->w_mode == Physical &&
                wd->CurCellName() == CurCellName()) {
            w_draw->SetColor(DSP()->Color(MarkerColor, w_mode));
            ShowBox(&wd->w_window, CDL_OUTLINED, 0);
        }
    }
    /* XXX
    w_draw->Update();
        for (int i = 1; i < DSP_NUMWINS; i++) {
            WindowDesc *wd = DSP()->Window(i);
                wd->Update();
        }
        */
}


// Run the pending lists.
//
void
WindowDesc::RunPending()
{
    if (DSP()->NoGraphics() || DSP()->NoRedisplay()) {
        ClearPending();
        return;
    }
    if (!w_pending_R && !w_pending_U)
        return;
    if (!w_draw)
        return;

    // Zero the queues now in case we reenter.
    Blist *pendR = w_pending_R;
    w_pending_R = 0;
    Blist *pendU = w_pending_U;
    w_pending_U = 0;

    PushRedisplay();

    w_draw->ShowGhost(ERASE);
    DSPpkg::self()->SetWorking(true);

    if (DSP()->NoPixmapStore())
        DestroyPixmap();

    Tdbg()->start_timing("RedisplayPending");

    int numgeom = 0;
    // With w_frozen set, no structure is shown, only the grid and
    // the cell bounding box
    if (w_frozen && !DSP()->DoingHcopy()) {
        SwitchToPixmap();
        if (pendR) {
            w_draw->SetFillpattern(0);
            w_draw->SetColor(DSP()->Color(BackgroundColor, w_mode));
            w_draw->Box(w_clip_rect.left, w_clip_rect.top,
                w_clip_rect.right, w_clip_rect.bottom);
            ShowGrid();
            w_draw->SetColor(DSP()->Color(HighlightingColor, w_mode));
            w_draw->Text("FROZEN", 10, 20, 0);
        }
        SwitchFromPixmap(&Viewport());
        ClearPending();
        RedisplayHighlighting(&w_window, true);
    }
    else {
        BBox tBB(CDnullBB);
        for (const Blist *bl = pendR; bl; bl = bl->next)
            tBB.add(&bl->BB);
        for (const Blist *bl = pendU; bl; bl = bl->next) {
            BBox uBB = bl->BB;
            ViewportBloat(uBB, uBB, 1);
            PToLbb(uBB, uBB);
            if (Ratio() < 1.0)
                uBB.bloat(-1);
            tBB.add(&uBB);
        }

        Blist *erbb;
        bool bbflag = show_cellbb(ERASE, &tBB, false, &erbb);
        if (erbb) {
            // This is the old bounding box that needs to be refreshed,
            // add it to the list.

            Blist *bn;
            for (Blist *b = erbb; b; b = bn) {
                bn = b->next;
                LToPbb(b->BB, b->BB);
                b->next = pendU;
                pendU = b;
            }
        }

        if (pendU) {
            if (!PixmapOk()) {
                // This might be the initial display of a window,
                // before the pixmap is created.  All is well except
                // if the window extends off-screen, in which case
                // pendU may contain only the visible part, causing
                // trouble.  In this case, we should force an update
                // of the entire window.

                if (!DSP()->NoPixmapStore() && !DSP()->SlowMode() &&
                        !w_frozen) {
                    // Will use pixmap, expand to full window.
                    Blist::destroy(pendU->next);
                    pendU->next = 0;
                    pendU->BB = w_window;
                    Blist::destroy(pendR);
                    pendR = pendU;
                    tBB = pendR->BB;
                }
                else {
                    // Can't use pixmap, add updates to redisplay list.
                    for (Blist *bl = pendU; bl; bl = bl->next) {
                        ViewportBloat(bl->BB, bl->BB, 1);
                        PToLbb(bl->BB, bl->BB);
                        bl->BB.bloat(-1);
                    }
                    if (!pendR)
                        pendR = pendU;
                    else {
                        Blist *b = pendR;
                        while (b->next)
                            b = b->next;
                        b->next = pendU;
                        pendR = Blist::merge(pendR);
                    }
                }
                pendU = 0;
            }
        }

        while (pendR) {
            Blist *bl = pendR;
            pendR = pendR->next;
            AOItype at = set_current_aoi(&bl->BB);
            if (at == AOInone) {
                delete bl;
                continue;
            }
            numgeom += redisplay_geom(&bl->BB);
            delete bl;
            if (DSP()->Interrupt()) {
                DSP()->RedisplayAfterInterrupt();
                // pending lists are now clear
            }
        }
        while (pendU) {
            Blist *bl = pendU;
            pendU = pendU->next;
            if (ViewportIntersect(bl->BB, Viewport())) {
                ViewportClip(bl->BB, Viewport());
                SwitchToPixmap();
                SwitchFromPixmap(&bl->BB);
            }
            delete bl;
        }
        RedisplayHighlighting(&tBB, bbflag);
    }

    w_draw->ShowGhost(DISPLAY);
    DSPpkg::self()->SetWorking(false);
    Tdbg()->stop_timing("RedisplayPending", numgeom);
    PopRedisplay();
    DSP()->notify_display_done();
}


// Clear the pending lists.
//
void
WindowDesc::ClearPending()
{
    Blist::destroy(w_pending_R);
    w_pending_R = 0;
    Blist::destroy(w_pending_U);
    w_pending_U = 0;
}


//
// The following functions operate on the backing pixmap through the
// toolkit interface.
//

void
WindowDesc::SwitchToPixmap()
{
    if (w_wbag)
         w_wbag->SwitchToPixmap();
}


void
WindowDesc::SwitchFromPixmap(const BBox *BB)
{
    if (w_wbag)
         w_wbag->SwitchFromPixmap(BB);
}


GRobject
WindowDesc::DrawableReset()
{
    if (w_wbag)
         return (w_wbag->DrawableReset());
    return (0);
}


void
WindowDesc::CopyPixmap(const BBox *BB)
{
    if (w_wbag)
         w_wbag->CopyPixmap(BB);
}


void
WindowDesc::DestroyPixmap()
{
    if (w_wbag)
         w_wbag->DestroyPixmap();
}


bool
WindowDesc::DumpWindow(const char *fname, const BBox *BB)
{
    if (w_wbag)
         return (w_wbag->DumpWindow(fname, BB));
    return (false);
}


bool
WindowDesc::PixmapOk()
{
    if (w_wbag)
         return (w_wbag->PixmapOk());
    return (false);
}



//
// Private functions
//

// Add BB to the redisplay list (BB uses window coordinates).
//
void
WindowDesc::add_redisp_region(const BBox *BB)
{
    if (!BB)
        BB = &w_window;
    else if (BB->left > BB->right || BB->bottom > BB->top)
        return;
    if (BB->right < w_window.left || BB->left > w_window.right ||
            BB->bottom > w_window.top || BB->top < w_window.bottom)
        return;
    if (*BB == w_window) {
        Blist::destroy(w_pending_R);
        w_pending_R = 0;
    }

    BBox tBB = *BB;
    if (tBB.left == tBB.right) {
        tBB.left -= 5;
        tBB.right += 5;
    }
    if (tBB.bottom == tBB.top) {
        tBB.bottom -= 5;
        tBB.top += 5;
    }
    if (!w_pending_R)
        w_pending_R = new Blist(&tBB, 0);
    else
        w_pending_R = Blist::insert_merge(w_pending_R, &tBB);
}


// Add BB to the update list (BB uses pixel coordinates).
//
void
WindowDesc::add_update_region(const BBox *BB)
{
    if (!BB)
        BB = &Viewport();
    else if (BB->left > BB->right || BB->top > BB->bottom)
        return;
    if (BB->right < 0 || BB->left >= w_width ||
            BB->bottom < 0 || BB->top >= w_height)
        return;
    if (*BB == Viewport()) {
        Blist::destroy(w_pending_U);
        w_pending_U = 0;
    }

    BBox tBB = *BB;
    if (tBB.left == tBB.right) {
        tBB.left -= 1;
        tBB.right += 1;
    }
    if (tBB.bottom == tBB.top) {
        tBB.bottom += 1;
        tBB.top -= 1;
    }
    if (!w_pending_U)
        w_pending_U = new Blist(&tBB, 0);
    else {
        Blist::wtov(w_pending_U);
        int t = tBB.top;
        tBB.top = tBB.bottom;
        tBB.bottom = t;
        w_pending_U = Blist::insert_merge(w_pending_U, &tBB);
        Blist::wtov(w_pending_U);
    }
}


// Erase or show the current cell's bounding box.  Return true if the
// old bounding box is erased (display == ERASE only).  In ERASE mode,
// the flag will prevent erasure of the old bounding box, which will
// otherwise happen if the cell's bounding box has changed.  In
// DISPLAY mode, the flag will force redrawing of the entire bounding
// box, i.e., the clip_rect is temporarily set to the viewport.  In
// this case, the flag passed is the return from the ERASE call.
//
bool
WindowDesc::show_cellbb(bool display, const BBox *AOI, bool flag, Blist **bret)
{
    if (bret)
        *bret = 0;
    if (!w_draw || DSP()->DoingHcopy() || w_attributes.show_no_highlighting())
        return (false);

    const BBox *sBB = 0;
    if (w_dbtype == WDcddb) {
        if (w_mode != Physical)
            return (false);
        CDs *sdesc = CurCellDesc(Physical);
        if (!sdesc)
            return (false);
        sBB = sdesc->BB();
    }
    else if (w_dbtype == WDchd) {
        cCHD *chd = CDchd()->chdRecall(w_dbname, false);
        if (chd) {
            symref_t *p = chd->findSymref(w_dbcellname, w_mode, true);
            if (p)
                sBB = p->get_bb();
        }
    }
    else
        return (false);

    if (!sBB)
        return (false);
    if (sBB->left == CDinfinity ||
            (sBB->left == sBB->right && sBB->bottom == sBB->top))
        return (false);

    BBox aBB;
    LToPbb(*AOI, aBB);
    bool bb_unchanged = false;
    if (w_last_cellbb == *sBB) {
        // Return if AOI doesn't touch boundary
        BBox cBB;
        LToPbb(w_last_cellbb, cBB);
        if (aBB.left > cBB.left && aBB.top > cBB.top &&
                aBB.right < cBB.right && aBB.bottom < cBB.bottom)
            return (false);
        if (aBB.left > cBB.right || aBB.right < cBB.left ||
                aBB.bottom < cBB.top || aBB.top > cBB.bottom)
            return (false);
        bb_unchanged = true;
    }
    else if (display)
        w_last_cellbb = *sBB;

    if (!display) {
        if (!bb_unchanged && !flag) {
            Blist *b0 = AddEdges(0, &w_last_cellbb);
            if (b0->BB.bottom == b0->BB.top || b0->BB.left == b0->BB.right)
                return (false);
            if (bret)
                *bret = b0;
            else {
                BBox tBB = w_clip_rect;
                w_clip_rect = Viewport();

                for (Blist *bl = b0; bl; bl = bl->next)
                    Refresh(&bl->BB);

                Blist::destroy(b0);
                w_clip_rect = tBB;
            }
            return (true);
        }
        return (false);
    }
    w_draw->SetColor(DSP()->Color(HighlightingColor, w_mode));

    // Draw the whole boundary, otherwise we have to deal with matching
    // the line pattern
    BBox tBB = w_clip_rect;
    w_clip_rect = Viewport();
    ShowBox(display ? sBB : &w_last_cellbb, CDL_OUTLINED, 0);
    w_clip_rect = tBB;

    return (false);
}


// Set the clip rectangle to the intersection of the pixel space
// corresponding to the window and the viewport.  Reset the AOI
// to the clip rectangle in window space.  This limits the search
// area when the viewport is set to an area smaller than the
// window pixel space, such as during hardcopy generation in
// segments.
//
AOItype
WindowDesc::set_current_aoi(BBox *AOI)
{
    if (AOI->left <= w_window.left &&
            AOI->bottom <= w_window.bottom &&
            AOI->right >= w_window.right &&
            AOI->top >= w_window.top) {
        w_clip_rect = Viewport();
        PToLbb(w_clip_rect, *AOI);
        return (AOIfull);
    }

    BBox BB(*AOI);
    if (Ratio() <= 1.0) {
        // Most common situation, where the window units are
        // finer-grained that screen pixels.  We're going to expand
        // the redisplay area slightly to make sure that all features
        // are properly updated, without leaving artifacts.

        // Bump up the window AOI by one unit, and transform to
        // pixels for the clip_rect.
        BB.bloat(1);
        LToPbb(BB, w_clip_rect);
        if (ViewportIntersect(w_clip_rect, Viewport())) {
            // Clip the clip_rect to the viewport.
            ViewportClip(w_clip_rect, Viewport());

            // Now, transform the AOI to correspond to a pixel area
            // one unit larger than the clip rect.  However, we want
            // the window coordinates to be on the outside of the band
            // corresponding to the transformed pixel coordinates.  We
            // get this by bloating the clip_rect by 2, transforming,
            // and shrinking the transformed box by one.

            ViewportBloat(BB, w_clip_rect, 2);
            PToLbb(BB, *AOI);
            AOI->bloat(-1);
            return (AOIpart);
        }
    }
    else {
        // The window is zoomed-in to the point where the pixel
        // resolution is finer than the window coordinates.  Similar
        // to above, we set the clip_rect to the outside of the band
        // for the transformed AOI.  The AOI is not reset.

        BB.bloat(1);
        LToPbb(BB, w_clip_rect);
        w_clip_rect.bloat(-1);
        if (ViewportIntersect(w_clip_rect, Viewport())) {
            // Clip the clip_rect to the viewport.
            ViewportClip(w_clip_rect, Viewport());
            return (AOIpart);
        }
    }

    // No overlap with viewport, nothing to show.
    w_clip_rect = Viewport();
    return (AOInone);
}

