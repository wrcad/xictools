
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
 $Id: dsp_box.cc,v 1.26 2014/11/17 05:17:36 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"


//
// Functions to display boxes, also includes the object cache.
//

// Show a box in the wdesc window, using type attributes and fillpatt.
//
void
WindowDesc::ShowBox(const BBox *boxBB, int type, const GRfillType *fillpat)
{
    if (!w_draw)
        return;
    BBox *AOI = &w_clip_rect;
    int PixWidth = 2;

    BBox BB = *boxBB;
    Poly poly;
    DSP()->TBB(&BB, &poly.points);
    if (poly.points) {
        // transformed to off-orthogonal, show as polygon.
        // The transform is already included in the points.
        //
        DSP()->TPush();
        poly.numpts = 5;
        ShowPolygon(&poly, type, fillpat, &BB);
        delete [] poly.points;
        DSP()->TPop();
        return;
    }

    LToPbb(BB, BB);
    w_fill = (type & CDL_FILLED) ? fillpat : 0;
    w_outline = (type & CDL_OUTLINED) && !(type & CDL_OUTLINEDFAT);

    if (!ViewportIntersect(BB, *AOI))
        return;

    bool ShowLeft   = true;
    bool ShowBottom = true;
    bool ShowRight  = true;
    bool ShowTop    = true;

    // for CDL_CUT, draw diagonals relative to original corners.
    BBox cutBB(BB);

    if (ViewportClipLeft(BB.left, AOI)) {
        BB.left = AOI->left;
        ShowLeft = false;
    }
    if (ViewportClipBottom(BB.bottom, AOI)) {
        BB.bottom = AOI->bottom;
        ShowBottom = false;
    }
    if (ViewportClipRight(BB.right, AOI)) {
        BB.right = AOI->right;
        ShowRight = false;
    }
    if (ViewportClipTop(BB.top, AOI)) {
        BB.top = AOI->top;
        ShowTop = false;
    }
    if (type & CDL_FILLED) {
        if (!w_usecache)
            w_draw->SetFillpattern(fillpat);
        if (!fillpat) {
            cache_solid_box(BB.left, BB.bottom, BB.right, BB.top);
            return;
        }
        cache_box(BB.left, BB.bottom, BB.right, BB.top);
    }

    if ((type & CDL_OUTLINED) && (type & CDL_OUTLINEDFAT)) {
        BBox BB1;
        ViewportBloat(BB1, cutBB, -PixWidth);

        if (!ShowLeft && !ViewportClipLeft(BB1.left, AOI))
            ShowLeft = true;
        if (!ShowBottom && !ViewportClipBottom(BB1.bottom, AOI))
            ShowBottom = true;
        if (!ShowRight && !ViewportClipRight(BB1.right, AOI))
            ShowRight = true;
        if (!ShowTop && !ViewportClipTop(BB1.top, AOI))
            ShowTop = true;

        if (!w_usecache)
            w_draw->SetFillpattern(0);

        if (ShowLeft) {
            if (ViewportClipRight(BB1.left, AOI))
                BB1.left = AOI->right;
            cache_solid_box(BB.left, BB.bottom, BB1.left, BB.top);
        }
        if (ShowBottom)    {
            if (ViewportClipTop(BB1.bottom, AOI))
                BB1.bottom = AOI->top;
            cache_solid_box(BB.left, BB.bottom, BB.right, BB1.bottom);
        }
        if (ShowRight) {
            if (ViewportClipLeft(BB1.right, AOI))
                BB1.right = AOI->left;
            cache_solid_box(BB1.right, BB.bottom, BB.right, BB.top);
        }
        if (ShowTop) {
            if (ViewportClipBottom(BB1.top, AOI))
                BB1.top = AOI->bottom;
            cache_solid_box(BB.left, BB1.top, BB.right, BB.top);
        }
        if (type & CDL_CUT) {
            if (!w_usecache) {
                w_draw->SetFillpattern(0);
                w_draw->SetLinestyle(0);
            }
            BBox tBB(cutBB);
            if (!cGEO::line_clip(&tBB.left, &tBB.bottom,
                    &tBB.right, &tBB.top, ClipRect()))
                cache_line(tBB.left, tBB.bottom, tBB.right, tBB.top);
            tBB = cutBB;
            if (!cGEO::line_clip(&tBB.left, &tBB.top,
                    &tBB.right, &tBB.bottom, ClipRect()))
                cache_line(tBB.left, tBB.top, tBB.right, tBB.bottom);
        }
        return;
    }
    if (type & CDL_FILLED) {
        if (fillpat) {
            if (!w_usecache && (type & (CDL_OUTLINED | CDL_CUT))) {
                w_draw->SetFillpattern(0);
                w_draw->SetLinestyle(0);
            }
            if (type & CDL_OUTLINED) {
                if (ShowBottom)
                    cache_line(BB.right, BB.bottom, BB.left, BB.bottom);
                if (ShowLeft)
                    cache_line(BB.left, BB.bottom, BB.left, BB.top);
                if (ShowRight)
                    cache_line(BB.right, BB.top, BB.right, BB.bottom);
                if (ShowTop)
                    cache_line(BB.left, BB.top, BB.right, BB.top);
            }
            if (type & CDL_CUT) {
                BBox tBB(cutBB);
                if (!cGEO::line_clip(&tBB.left, &tBB.bottom,
                        &tBB.right, &tBB.top, ClipRect()))
                    cache_line(tBB.left, tBB.bottom, tBB.right, tBB.top);
                tBB = cutBB;
                if (!cGEO::line_clip(&tBB.left, &tBB.top,
                        &tBB.right, &tBB.bottom, ClipRect()))
                    cache_line(tBB.left, tBB.top, tBB.right, tBB.bottom);
            }
        }
    }
    else {
        if (!w_usecache) {
            w_draw->SetFillpattern(0);
            if (type & CDL_OUTLINED)
                w_draw->SetLinestyle(DSP()->BoxLinestyle());
            else
                w_draw->SetLinestyle(0);
        }
        if (ShowBottom)
            cache_line(BB.right, BB.bottom, BB.left, BB.bottom);
        if (ShowLeft)
            cache_line(BB.left, BB.bottom, BB.left, BB.top);
        if (ShowRight)
            cache_line(BB.right, BB.top, BB.right, BB.bottom);
        if (ShowTop)
            cache_line(BB.left, BB.top, BB.right, BB.top);
        if (type & CDL_CUT) {
            BBox tBB(cutBB);
            if (!cGEO::line_clip(&tBB.left, &tBB.bottom,
                    &tBB.right, &tBB.top, ClipRect()))
                cache_line(tBB.left, tBB.bottom, tBB.right, tBB.top);
            tBB = cutBB;
            if (!cGEO::line_clip(&tBB.left, &tBB.top,
                    &tBB.right, &tBB.bottom, ClipRect()))
                cache_line(tBB.left, tBB.top, tBB.right, tBB.bottom);
        }
        if (!w_usecache && (type & CDL_OUTLINED))
            w_draw->SetLinestyle(0);
    }
}


// Show an open box.  Used for ghost-drawing and highlighting. 
// Warning:  this doesn't work if the transform is 45/135/225/315
// degrees.
//
void
WindowDesc::ShowLineBox(int x, int y, int refx, int refy)
{
    if (!w_draw)
        return;
    DSP()->TPoint(&x, &y);
    DSP()->TPoint(&refx, &refy);
    LToP(x, y, x, y);
    LToP(refx, refy, refx, refy);
    ShowLineV(refx, refy, x, refy);
    ShowLineV(x, refy, x, y);
    ShowLineV(x, y, refx, y);
    ShowLineV(refx, y, refx, refy);
}


// Show an open box.  Used for ghost-drawing and highlighting.
//
void
WindowDesc::ShowLineBox(const BBox *BB)
{
    if (!w_draw)
        return;
    Point *pts;
    BBox tBB(*BB);
    DSP()->TBB(&tBB, &pts);
    if (pts) {
        for (int i = 0; i < 5; i++)
            LToP(pts[i].x, pts[i].y, pts[i].x, pts[i].y);
        ShowLineV(pts[0].x, pts[0].y, pts[1].x, pts[1].y);
        ShowLineV(pts[1].x, pts[1].y, pts[2].x, pts[2].y);
        ShowLineV(pts[2].x, pts[2].y, pts[3].x, pts[3].y);
        ShowLineV(pts[3].x, pts[3].y, pts[4].x, pts[4].y);
    }
    else {
        LToPbb(tBB, tBB);
        ShowLineV(tBB.left, tBB.bottom, tBB.left, tBB.top);
        ShowLineV(tBB.left, tBB.top, tBB.right, tBB.top);
        ShowLineV(tBB.right, tBB.top, tBB.right, tBB.bottom);
        ShowLineV(tBB.left, tBB.bottom, tBB.right, tBB.bottom);
    }
}


// The following functions implement caching for boxes and box outlines
// (lines) in the hope that this will speed redraws.  Only boxes on the
// same layer with the same fill can be cached.  To use, call InitCache(),
// call EnableCache() / DisableCache() around boxes to be cached,
// then call FlushCache().

// Allocate storage.
//
void
WindowDesc::InitCache()
{
    w_usecache = false;  // call EnableCache() to activate
    if (DSP()->NoDisplayCache())
        return;
    if (!w_cache)
        w_cache = new sDSPcache;
}


// Flush the cache and turn off caching.
//
void
WindowDesc::FlushCache()
{
    if (DSP()->NoDisplayCache())
        return;
    if (!w_cache)
        return;
    if (w_cache->c_numboxes && w_draw) {
        if (w_fill)
            w_draw->SetFillpattern(w_fill);
        else
            w_draw->SetFillpattern(0);
        w_draw->Boxes(w_cache->c_boxes, w_cache->c_numboxes);
        w_draw->Update();
    }
    w_cache->c_numboxes = 0;

    if (w_cache->c_numsboxes && w_draw) {
        w_draw->SetFillpattern(0);
        w_draw->Boxes(w_cache->c_sboxes, w_cache->c_numsboxes);
        w_draw->Update();
    }
    w_cache->c_numsboxes = 0;

    if (w_cache->c_numlines && w_draw) {
        w_draw->SetFillpattern(0);
        if (w_outline && !w_fill)
            w_draw->SetLinestyle(DSP()->BoxLinestyle());
        else
            w_draw->SetLinestyle(0);
        w_draw->Lines(w_cache->c_lines, w_cache->c_numlines);
        w_draw->Update();
        if (w_outline && !w_fill)
            w_draw->SetLinestyle(0);
    }
    w_cache->c_numlines = 0;

    if (w_cache->c_numpixels && w_draw) {
        w_draw->SetFillpattern(0);
        w_draw->Pixels(w_cache->c_pixels, w_cache->c_numpixels);
        w_draw->Update();
    }
    w_cache->c_numpixels = 0;

    w_fill = 0;
    w_outline = false;
    w_usecache = false;
}


// Add the box element to the cache, dump output on overflow.
//
void
WindowDesc::cache_box(int l, int b, int r, int t)
{
    if (!w_usecache) {
        w_draw->Box(l, b, r, t);
        return;
    }
    if (l > r) {
        int tmp = l;
        l = r;
        r = tmp;
    }
    if (t > b) {
        int tmp = t;
        t = b;
        b = tmp;
    }

    if (w_cache->c_numboxes == DSP_CACHE_SIZE) {
        if (!w_draw)
            return;
        if (w_fill)
            w_draw->SetFillpattern(w_fill);
        else
            w_draw->SetFillpattern(0);
        w_draw->Boxes(w_cache->c_boxes, w_cache->c_numboxes);
        w_draw->Update();
        w_cache->c_numboxes = 0;
    }
    if (r == l)
        r++;
    if (b == t)
       b++;
    w_cache->add_box(l, b, r, t);
}


// As above, but these always use a solid fill, for fat edges of
// stippled layers.
//
void
WindowDesc::cache_solid_box(int l, int b, int r, int t)
{
    if (!w_usecache) {
        w_draw->Box(l, b, r, t);
        return;
    }
    if (l > r) {
        int tmp = l;
        l = r;
        r = tmp;
    }
    if (t > b) {
        int tmp = t;
        t = b;
        b = tmp;
    }

    if (w_cache->c_numsboxes == DSP_CACHE_SIZE) {
        if (!w_draw)
            return;
        w_draw->SetFillpattern(0);
        w_draw->Boxes(w_cache->c_sboxes, w_cache->c_numsboxes);
        w_draw->Update();
        w_cache->c_numsboxes = 0;
    }
    if (r != l && t != b) {
        w_cache->add_sbox(l, b, r, t);
        return;
    }

    // Render using a line. or a single pixel.
    if (r == l && t == b) {
        if (w_cache->c_numpixels == DSP_CACHE_SIZE) {
            if (!w_draw)
                return;
            w_draw->Pixels(w_cache->c_pixels, w_cache->c_numpixels);
            w_cache->c_numpixels = 0;
        }
        w_cache->add_pixel(l, t);
        return;
    }
    if (w_cache->c_numlines == DSP_CACHE_SIZE) {
        if (!w_draw)
            return;
        w_draw->SetFillpattern(0);
        w_draw->SetLinestyle(0);
        w_draw->Lines(w_cache->c_lines, w_cache->c_numlines);
        w_draw->Update();
        w_cache->c_numlines = 0;
    }
    w_cache->add_line(l, b, r, t);
}


// Add the line element to the cache, dump output on overflow
//
void
WindowDesc::cache_line(int x1, int y1, int x2, int y2)
{
    if (!w_usecache) {
        w_draw->Line(x1, y1, x2, y2);
        return;
    }
    if (w_cache->c_numlines == DSP_CACHE_SIZE) {
        if (!w_draw)
            return;
        w_draw->SetFillpattern(0);
        if (w_outline && !w_fill)
            w_draw->SetLinestyle(DSP()->BoxLinestyle());
        else
            w_draw->SetLinestyle(0);
        w_draw->Lines(w_cache->c_lines, w_cache->c_numlines);
        w_draw->Update();
        if (w_outline && !w_fill)
            w_draw->SetLinestyle(0);
        w_cache->c_numlines = 0;
    }
    if (x1 == x2 && y1 == y2) {
        if (w_cache->c_numpixels == DSP_CACHE_SIZE) {
            if (!w_draw)
                return;
            w_draw->Pixels(w_cache->c_pixels, w_cache->c_numpixels);
            w_draw->Update();
            w_cache->c_numpixels = 0;
        }
        w_cache->add_pixel(x1, y1);
        return;
    }
    w_cache->add_line(x1, y1, x2, y2);
}
// End of WindowDesc functions.


void
sDSPcache::add_box(int l, int b, int r, int t)
{
    c_boxes->assign(2*c_numboxes, l, t);
    c_boxes->assign(2*c_numboxes + 1, r - l + 1, b - t + 1);
    c_numboxes++;
}


void
sDSPcache::add_sbox(int l, int b, int r, int t)
{
    c_sboxes->assign(2*c_numsboxes, l, t);
    c_sboxes->assign(2*c_numsboxes + 1, r - l + 1, b - t + 1);
    c_numsboxes++;
}


void
sDSPcache::add_line(int l, int b, int r, int t)
{
    c_lines->assign(2*c_numlines, l, b);
    c_lines->assign(2*c_numlines + 1, r, t);
    c_numlines++;
}


void
sDSPcache::add_pixel(int x, int y)
{
    c_pixels->assign(c_numpixels, x, y);
    c_numpixels++;
}

