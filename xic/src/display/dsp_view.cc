
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

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "cd_sdb.h"
#include "cd_digest.h"
#include "fio.h"
#include "fio_chd.h"


//
// This code maintains the lists of views associated with windows.
//

// Clear all of the view lists.  Called when a new cell is edited, and
// when changing modes.
//
void
cDisplay::ClearViews()
{
    for (int i = 0; i < DSP_NUMWINS; i++) {
        if (Window(i))
            Window(i)->ClearViews();
    }
}
// End of cDisplay functions.


// Set the view according to vname, return true if view was changed.
//
bool
WindowDesc::SetView(const char *vname)
{
    if (!vname)
        return (false);
    while (isspace(*vname))
        vname++;
    if (!*vname)
        return (false);
    if (!strncmp(vname, "full", 4)) {
        BBox bb = w_window;
        CenterFullView();
        if (bb != w_window) {
            Redisplay(0);
            DSP()->window_view_change(this);
        }
        return (true);
    }
    else if (!strncmp(vname, "prev", 4)) {
        w_views.rot_hist_l();
        BBox *b = w_views.view_hist();
        if (b) {
            if (*b != w_window) {
                w_window = *b;
                w_ratio = ((double)w_width)/w_window.width();
                Redisplay(0);
                DSP()->window_view_change(this);
            }
            return (true);
        }
    }
    else if (!strncmp(vname, "next", 4)) {
        w_views.rot_hist_r();
        BBox *b = w_views.view_hist();
        if (b) {
            if (*b != w_window) {
                w_window = *b;
                w_ratio = ((double)w_width)/w_window.width();
                Redisplay(0);
                DSP()->window_view_change(this);
            }
            return (true);
        }
    }
    else {
        BBox *b = w_views.view_view(vname);
        if (b) {
            if (*b != w_window) {
                w_window = *b;
                w_ratio = ((double)w_width)/w_window.width();
                w_views.add_hist(&w_window);
                Redisplay(0);
                DSP()->window_view_change(this);
            }
            return (true);
        }
    }
    return (false);
}


// Assign the present view a name, and save it in the list.
//
void
WindowDesc::SaveViewOnStack()
{
    int indx = w_views.add_view(&w_window);
    DSP()->window_view_saved(this, indx);
}


// Set the default window size for wdesc.
//
void
WindowDesc::DefaultWindow()
{
    if (w_mode == Physical)
        InitWindow(0, 0, 100.0*CDphysResolution);
    else
        InitWindow(0, 0, 100.0*CDelecResolution);
}


// Set the wdesc window so that the current cell is centered and fully
// visible.  If BB is passed, instead make sure that it is fully
// visible.
//
void
WindowDesc::CenterFullView(const BBox *BB)
{
    bool awm = false;
    if (!BB) {
        BB = ContentBB();
        if (DbType() == WDcddb)
            awm = true;
    }
    if (!BB || (BB->left == CDinfinity && BB->bottom == CDinfinity)) {
        DefaultWindow();
        return;
    }

    // We don't want to add window marks unless we are showing the
    // entire cell.
    //
    if (!awm && DbType() == WDcddb) {
        const BBox *cBB = ContentBB();
        if (cBB && *BB >= *cBB)
            awm = true;
    }
    if (IsXSect()) {
        // Showing cross-section.
        int extent = BB->right - BB->left;
        int marg = extent/20;
        int yy = (int)(extent/Aspect());
        InitWindow((BB->left + BB->right)/2, yy/2, extent + 2*marg);
        return;
    }

    BBox tBB(*BB);
    if (awm) {
        AddWindowMarksBB(&tBB);
        if (DSP()->CFVbloat() && IsSimilar(DSP()->MainWdesc()))
            tBB.bloat(DSP()->CFVbloat());
    }

    int width = tBB.width();
    int height = tBB.height();
    int x = tBB.left + width/2;
    int y = tBB.bottom + height/2;
    if (width < 0) width = -width;
    if (height < 0) height = -height;
    double Vratio = Aspect();
    if (width == 0 || height == 0) {
        int w = width > height ? width : height;
        if (w == 0) {
            if (w_mode == Physical)
                InitWindow(x, y, 100.0*CDphysResolution);
            else
                InitWindow(x, y, 100.0*CDelecResolution);
            return;
        }
        if (w == height)
            w = (int)(w*Vratio);
        InitWindow(x, y, w*1.1);
        return;
    }
    double Cratio = (double) width/height;

    if (Cratio > Vratio)
        InitWindow(x, y, width * 1.1);
    else
        InitWindow(x, y, 1.1 * height * Vratio);
}


// Return a pointer the the bounding box of the contents (of whatever
// type), or null if no content.
//
const BBox *
WindowDesc::ContentBB()
{
    const BBox *BBp = 0;
    if (w_dbtype == WDcddb) {
        CDs *sdesc = CurCellDesc(w_mode);
        if (sdesc)
            BBp = sdesc->BB();
    }
    else if (w_dbtype == WDchd) {
        cCHD *chd = CDchd()->chdRecall(w_dbname, false);
        if (chd) {
            symref_t *p = chd->findSymref(w_dbcellname, w_mode, true);
            if (p) {
                chd->setBoundaries(p);
                BBp = p->get_bb();
            }
        }
    }
    else {
        cSDB *tab = CDsdb()->findDB(w_dbname);
        if (tab && tab->BB()->right > tab->BB()->left)
            BBp = tab->BB();
    }
    return (BBp);
}


// Initialize the viewport.  The width and height are the size of the
// display device bit field.  The DSP internal clipping retains the
// boundary points, so we subtract 1 so that the right and bottom
// values are in the display.
//
// Note: this should be the only place where the viewport values are set.
//
void
WindowDesc::InitViewport(int width, int height)
{
    if (InRedisplay()) {
        // We're drawing, not good since we get garbage when the
        // viewport changes.  Set the interrupt flag so drawing will
        // cease asap, and set a flag to indicate that a redisplay is
        // needed thereafter.

        DSP()->SetInterrupt(DSPinterSys);
        w_need_redisplay = true;
    }
    w_width = width;
    w_height = height;
    w_viewport.left = 0;
    w_viewport.bottom = height - 1;
    w_viewport.right = width - 1;
    w_viewport.top = 0;
}


// Set a new window center and width, and save it.
//
void
WindowDesc::InitWindow(int x, int y, double width)
{
    DSPmainDraw(ShowGhost(ERASE))

    if (width < 0)
        width = -width;
    if (width < MinWidth())
        width = MinWidth();
    if (width == 0.0)
        width = 1.0;

    double wid2 = width/2;
    double hei2 = wid2/Aspect();
    double Imax = CDinfinity;
    if (wid2 > hei2) {
        if (wid2 > Imax) {
            wid2 = Imax;
            hei2 = Imax/Aspect();
        }
    }
    else {
        if (hei2 > Imax) {
            hei2 = Imax;
            wid2 = Imax*Aspect();
        }
    }
    width = 2*wid2;

    if (x - wid2 < -Imax)
        x = (int)(-Imax + wid2);
    else if (x + wid2 > Imax)
        x = (int)(Imax - wid2);
    if (y - hei2 < -Imax)
        y = (int)(-Imax + hei2);
    else if (y + hei2 > Imax)
        y = (int)(Imax - hei2);

    BBox oldwin = w_window;
    int w = mmRnd(wid2);
    int h = mmRnd(hei2);
    w_window.left = x - w;
    w_window.right = x + w;
    w_window.bottom = y - h;
    w_window.top = y + h;

    w_ratio = ((double)w_width)/width;
    w_views.add_hist(&w_window);
    DSP()->window_view_change(this);

    // Update the previous subwindow location in main window.
    if (w_mode == Physical && w_show_loc && DSP()->MainWdesc() != this &&
            DSP()->MainWdesc())
        DSP()->MainWdesc()->Refresh(&oldwin);

    // Update the previous main window location in frozen subwindow.
    if (w_mode == Physical && DSP()->MainWdesc() == this) {
        for (int i = 1; i < DSP_NUMWINS; i++) {
            WindowDesc *wd = DSP()->Window(i);
            if (wd && wd->w_mode == Physical && wd->w_frozen &&
                    wd->CurCellName() == CurCellName())
                wd->Redisplay(&oldwin);
        }
    }

    // Update the new subwindow location in main window.
    if (w_mode == Physical && w_show_loc && DSP()->MainWdesc() != this &&
            DSP()->MainWdesc())
        DSP()->MainWdesc()->Refresh(&w_window);

    DSPmainDraw(ShowGhost(DISPLAY))
}


void
WindowDesc::InitWindow(const BBox *BB)
{
    if (!BB) {
        CenterFullView();
        return;
    }

    int x = (BB->left + BB->right)/2;
    int y = (BB->bottom + BB->top)/2;
    InitWindow(x, y, BB->width());
}


// Move the center of the view to x,y.
//
void
WindowDesc::Center(int x, int y)
{
    InitWindow(x, y, w_window.width());
    Redisplay(0);
}


// Pan by factor in the direction dir.  Unit factor is window
// width/height.
//
void
WindowDesc::Pan(DirectionType dir, double factor)
{
    if (dspPkgIf()->IsBusy())
        return;
    int x = (w_window.left + w_window.right)/2;
    int y = (w_window.bottom + w_window.top)/2;
    int dx = w_window.width();
    int dy = w_window.height();

    switch (dir) {
    case DirNone:
        return;
    case DirWest:
        InitWindow(x - (int)(factor*dx), y, dx);
        break;
    case DirNorthWest:
        factor *= .707;
        InitWindow(x - (int)(factor*dx), y + (int)(factor*dy), dx);
        break;
    case DirNorth:
        InitWindow(x, y + (int)(factor*dy), dx);
        break;
    case DirNorthEast:
        factor *= .707;
        InitWindow(x + (int)(factor*dx), y + (int)(factor*dy), dx);
        break;
    case DirEast:
        InitWindow(x + (int)(factor*dx), y, dx);
        break;
    case DirSouthEast:
        factor *= .707;
        InitWindow(x + (int)(factor*dx), y - (int)(factor*dy), dx);
        break;
    case DirSouth:
        InitWindow(x, y - (int)(factor*dy), dx);
        break;
    case DirSouthWest:
        factor *= .707;
        InitWindow(x - (int)(factor*dx), y - (int)(factor*dy), dx);
        break;
    }
    Redisplay(0);
}


void
WindowDesc::Zoom(double factor)
{
    if (dspPkgIf()->IsBusy())
        return;
    int x = (w_window.left + w_window.right)/2;
    int y = (w_window.bottom + w_window.top)/2;
    int dx = w_window.width();
    double w = factor * dx;
    if (w >= 10.0) {
        InitWindow(x, y, w);
        Redisplay(0);
    }
}


void
WindowDesc::ClearViews()
{
    w_last_cellbb = BBox(0, 0, 0, 0);
    DSP()->window_clear_views(this);
    w_views.clear();
}
// End WindowDesc functions


// Constructor
//
wStackElt::wStackElt(BBox *b, const char *nm, wStackElt *nx)
{
    BB = *b;
    if (!nm)
        nm = "";
    strncpy(name, nm, 8);
    next = nx;
}
// End wStackElt functions


// Clear and zero element lists.
//
void
wStack::clear()
{
    wStackElt::destroy(w_views);
    w_views = 0;
    wStackElt::destroy(w_viewhist);
    w_viewhist = 0;
}


// Add BB to history list, if different from previous.
//
void
wStack::add_hist(BBox *BB)
{
    if (w_viewhist && *BB == w_viewhist->BB)
        return;
    int i = 0;
    for (wStackElt *w = w_viewhist; w; w = w->next, i++) {
        if (i == WSTACK_HIST - 1 && w->next) {
            wStackElt *wn = w->next;
            w->next = 0;
            wn->BB = *BB;
            wn->next = w_viewhist;
            w_viewhist = wn;
            return;
        }
    }
    w_viewhist = new wStackElt(BB, 0, w_viewhist);
}


// Add BB to named view list, generating name.  If a new entry is added,
// return the index of that entry, otherwise return -1.
//
int
wStack::add_view(BBox *BB)
{
    int i = 0;
    for (wStackElt *w = w_views; w; w = w->next, i++) {
        if (i == WSTACK_VIEWS - 2 && w->next) {
            wStackElt *wn = w->next;
            w->next = 0;
            wn->BB = *BB;
            wn->next = w_views;
            w_views = wn;
            return (-1);
        }
    }
    char buf[8];
    buf[0] = 'A' + i;
    buf[1] = 0;
    w_views = new wStackElt(BB, buf, w_views);
    return (i);
}


// Rotate history list left, prev becomes current.
//
void
wStack::rot_hist_l()
{
    if (!w_viewhist || !w_viewhist->next)
        return;
    wStackElt *wt = w_viewhist;
    w_viewhist = w_viewhist->next;
    wt->next = 0;
    wStackElt *w = w_viewhist;
    while (w->next)
        w = w->next;
    w->next = wt;
}


// Rotate history list right, current becomes prev.
//
void
wStack::rot_hist_r()
{
    if (!w_viewhist || !w_viewhist->next)
        return;
    wStackElt *w = w_viewhist;
    while (w->next->next)
        w = w->next;
    wStackElt *wt = w->next;
    w->next = 0;
    wt->next = w_viewhist;
    w_viewhist = wt;
}


// Return current BBox from history list.
//
BBox *
wStack::view_hist()
{
    if (!w_viewhist)
        return (0);
    return (&w_viewhist->BB);
}


// Return matching BBox from named list, trailing space is ignored
// in name.
//
BBox*
wStack::view_view(const char *name)
{
    if (!name)
        return (0);
    const char *s = name;
    while (*s && !isspace(*s))
        s++;
    int len = s - name;
    for (wStackElt *w = w_views; w; w = w->next) {
        if (!strncmp(name, w->name, len))
            return (&w->BB);
    }
    return (0);
}


// Return the lengths of the two lists.
//
void
wStack::numsaved(int *hsaved, int *vsaved)
{
    wStackElt *w;
    int i;
    if (hsaved) {
        for (i = 0, w = w_viewhist; w; i++, w = w->next) ;
        *hsaved = i;
    }
    if (vsaved) {
        for (i = 0, w = w_views; w; i++, w = w->next) ;
        *vsaved = i;
    }
}


// Zero the list heads.
//
void
wStack::zero()
{
    w_viewhist = w_views = 0;
}
// End of wStack functions

