
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
 $Id: dsp_poly.cc,v 1.32 2012/06/04 06:55:30 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"


//
// Functions to display polygons.
//

namespace {
    PointBuf<Point> L_bf(64);
    GRmultiPt *S_bf;

    void render_path(WindowDesc*, Point*, int, GRmultiPt*);
}


// Display a completed polygon in the window governed by wdesc.  If
// type is FILLED, use the fill pattern given.  If pBB is given, it
// is taken as the bounding box, and used to test whether clipping
// can be skipped.
//
void
WindowDesc::ShowPolygon(const Poly *poly, int type, const GRfillType *fillpat,
    const BBox *pBB)
{
    if (poly->numpts < 3)
        return;
    if (!w_draw)
        return;

    if (!(type & CDL_FILLED) && (type & CDL_OUTLINED) &&
            (type & CDL_OUTLINEDFAT) && poly->is_manhattan()) {
        // Use "fat" segments.
        Otype cw = poly->winding();
        int n = poly->numpts - 1;
        for (int i = 0; i < n; i++)
            fat_seg(&poly->points[i], &poly->points[i+1], cw);
        return;
    }

    // At high magnification, clip in viewport space, otherwise the
    // poly fill is out of sync with the edge vectors.  At ordinary
    // magnification have to clip in window space, in viewport space
    // coordinates can overlap causing incorrect fill polarity.
    bool clip_vport = (Ratio() > 1.0);

    // Find the largest window that maps to the clip rect.
    BBox cBB = w_clip_rect;
    if (!clip_vport) {
        cBB.right++;
        cBB.bottom++;
    }
    PToLbb(cBB, cBB);
    if (!clip_vport) {
        cBB.right--;
        cBB.top--;
    }

    bool noclip = false;
    if (pBB) {
        // if pBB was given, see if clipping can be skipped
        BBox BB = *pBB;
        DSP()->TBB(&BB, 0);
        if (BB <= cBB)
             noclip = true;
    }

    L_bf.init(poly->numpts);
    if (!S_bf)
        S_bf = new GRmultiPt(poly->numpts);
    else
        S_bf->init(poly->numpts);

    DSP()->TPath(poly->numpts, L_bf.points(), poly->points);
    Point *pp = L_bf.points();
    int n = poly->numpts;
    if (pp[0] != pp[n-1]) {
        // Ensure that path is closed (shouldn't happen).
        L_bf.points()[n++] = L_bf.points()[0];
    }

    if (noclip) {
        for (int i = 0; i < n; i++) {
            int x = pp[i].x;
            int y = pp[i].y;
            LToP(x, y, x, y);
            S_bf->assign(i, x, y);
        }
        if (type & CDL_FILLED) {
            w_draw->SetFillpattern(fillpat);
            w_draw->Polygon(S_bf, n);
            if (type & CDL_OUTLINED) {
                w_draw->SetFillpattern(0);
                w_draw->SetLinestyle(0);
                w_draw->PolyLine(S_bf, n);
            }
        }
        else {
            w_draw->SetFillpattern(0);
            if (type & CDL_OUTLINED)
                w_draw->SetLinestyle(DSP()->BoxLinestyle());
            else
                w_draw->SetLinestyle(0);
            w_draw->PolyLine(S_bf, n);
        }
    }
    else if (clip_vport) {
        for (int i = 0; i < n; i++) {
            int x = pp[i].x;
            int y = pp[i].y;
            LToP(x, y, x, y);
            pp[i].set(x, y);
        }
        if (type & CDL_FILLED) {
            cBB = w_clip_rect;
            cBB.fix();
            Poly tmp_poly(n, L_bf.points());
            PolyList *p0 = tmp_poly.clip(&cBB);
            w_draw->SetFillpattern(fillpat);
            for (PolyList *pl = p0; pl; pl = pl->next) {
                S_bf->init(pl->po.numpts);  // size can grow!
                for (int i = 0; i < pl->po.numpts; i++)
                    S_bf->assign(i, pl->po.points[i].x, pl->po.points[i].y);
                w_draw->Polygon(S_bf, pl->po.numpts);
            }
            PolyList::destroy(p0);

            if (type & CDL_OUTLINED) {
                w_draw->SetFillpattern(0);
                w_draw->SetLinestyle(0);
                render_path(this, L_bf.points(), n, S_bf);
            }
        }
        else {
            w_draw->SetFillpattern(0);
            if (type & CDL_OUTLINED)
                w_draw->SetLinestyle(DSP()->BoxLinestyle());
            else
                w_draw->SetLinestyle(0);
            render_path(this, L_bf.points(), n, S_bf);
        }
    }
    else {
        if (type & CDL_FILLED) {
            Poly tmp_poly(n, L_bf.points());
            PolyList *p0 = tmp_poly.clip(&cBB);
            w_draw->SetFillpattern(fillpat);
            for (PolyList *pl = p0; pl; pl = pl->next) {
                S_bf->init(pl->po.numpts);  // size can grow!
                for (int i = 0; i < pl->po.numpts; i++) {
                    int x, y;
                    LToP(pl->po.points[i].x, pl->po.points[i].y, x, y);
                    S_bf->assign(i, x, y);
                }
                w_draw->Polygon(S_bf, pl->po.numpts);
            }
            PolyList::destroy(p0);

            if (type & CDL_OUTLINED) {
                w_draw->SetFillpattern(0);
                w_draw->SetLinestyle(0);
                for (int i = 0; i < n; i++)
                    LToP(L_bf.points()[i].x, L_bf.points()[i].y,
                        L_bf.points()[i].x, L_bf.points()[i].y);
                render_path(this, L_bf.points(), n, S_bf);
            }
        }
        else {
            w_draw->SetFillpattern(0);
            if (type & CDL_OUTLINED)
                w_draw->SetLinestyle(DSP()->BoxLinestyle());
            else
                w_draw->SetLinestyle(0);
            for (int i = 0; i < n; i++)
                LToP(L_bf.points()[i].x, L_bf.points()[i].y,
                    L_bf.points()[i].x, L_bf.points()[i].y);
            render_path(this, L_bf.points(), n, S_bf);
        }
    }
}


// Display a fat line connecting the two vertices.
//
void
WindowDesc::fat_seg(const Point *p1, const Point *p2, Otype cw)
{
    if (!w_draw)
        return;
    int pw = 2;
    BBox BB(p1->x, p1->y, p2->x, p2->y);
    DSP()->TPoint(&BB.left, &BB.bottom);
    DSP()->TPoint(&BB.right, &BB.top);
    LToPbb(BB, BB);
    BBox BB1(mmMin(BB.left, BB.right) - pw, mmMax(BB.bottom, BB.top) + pw,
        mmMax(BB.left, BB.right) + pw, mmMin(BB.bottom, BB.top) - pw);
    if (ViewportIntersect(BB1, w_clip_rect)) {
        if (BB.left == BB.right) {
            if ((BB.top < BB.bottom && cw == Ocw) ||
                    (BB.top > BB.bottom && cw != Ocw))
                BB.right += pw;
            else
                BB.left -= pw;
        }
        else if (BB.bottom == BB.top) {
            if ((BB.right > BB.left && cw == Ocw) ||
                    (BB.right < BB.left && cw != Ocw))
                BB.bottom += pw;
            else
                BB.top -= pw;
        }
        else {
            if (!cGEO::line_clip(&BB.left, &BB.bottom, &BB.right, &BB.top,
                    &w_clip_rect)) {
                w_draw->Line(BB.left, BB.bottom, BB.right, BB.top);
            }
            return;
        }
        ViewportClip(BB, w_clip_rect);
        w_draw->SetFillpattern(0);
        w_draw->Box(BB.left, BB.bottom, BB.right, BB.top);
    }
}


// Display a line connecting the vertices.  If terminate is true,
// ensure that the path is shown closed.
//
void
WindowDesc::ShowPath(const Point *points, int numpts, bool terminate)
{
    if (!w_draw)
        return;
    if (numpts <= 1)
        return;
    L_bf.init(numpts);
    if (!S_bf)
        S_bf = new GRmultiPt(numpts);
    else
        S_bf->init(numpts);

    DSP()->TPath(numpts, L_bf.points(), points);
    points = L_bf.points();
    if (terminate && (points[numpts-1] != points[0]))
        L_bf.points()[numpts++] = points[0];

    int n;
    for (n = 0; n < numpts; n++) {
        int x = points[n].x;
        int y = points[n].y;
        LToP(x, y, x, y);
        L_bf.points()[n].set(x, y);
    }
    w_draw->SetFillpattern(0);
    w_draw->SetLinestyle(0);
    render_path(this, L_bf.points(), n, S_bf);
}


namespace {
    // Function to render the path passed in pi.  Arg tmp contains a
    // suitably large buffer for temporary storage.
    //
    void
    render_path(WindowDesc *wdesc, Point *pi, int num, GRmultiPt *tmp)
    {
        if (!wdesc->Wdraw())
            return;
        int i = 0;
        for (int n = 1; n < num; n++) {
            int x1 = pi[n-1].x;
            int y1 = pi[n-1].y;
            int x2 = pi[n].x;
            int y2 = pi[n].y;
            if (!cGEO::line_clip(&x1, &y1, &x2, &y2, wdesc->ClipRect())) {
                if (i == 0) {
                    tmp->assign(0, x1, y1);
                    tmp->assign(1, x2, y2);
                    i = 2;
                }
                else {
                    if (x1 != tmp->at(i-1).x || y1 != tmp->at(i-1).y) {
                        wdesc->Wdraw()->PolyLine(tmp, i);
                        tmp->assign(0, x1, y1);
                        tmp->assign(1, x2, y2);
                        i = 2;
                    }
                    else {
                        tmp->assign(i, x2, y2);
                        i++;
                    }
                }
            }
        }
        if (i > 1)
            wdesc->Wdraw()->PolyLine(tmp, i);
    }
}

