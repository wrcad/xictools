
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

#include "cd.h"
#include "cd_types.h"
#include "geo_zlist.h"
#include "si_parsenode.h"
#include "si_lexpr.h"
#include "si_lspec.h"


// Width of stripe used for edge testing
#define TEST_WIDTH 4

namespace { Zlist *invert(Zlist*, const Point*, const Point*); }

// edge and corner codes
#define DRCeL 1
#define DRCeB 2
#define DRCeR 4
#define DRCeT 8
#define DRCcLL 3
#define DRCcLR 6
#define DRCcUL 9
#define DRCcUR 12
#define DRCeCWA  16
#define DRCeCWD  17
#define DRCeCCWA 18
#define DRCeCCWD 19


// Static function.
// Return in *elist a list of zoids in a delta region outside or
// inside of the line (p1, p2), either covering if covered is true, or
// space regions otherwise.  Also return a code describing the line.
// The {L, B, R, T} in the code designates the side which is WITHIN
// the object.  For non- manhattan lines, the return code designates
// CW or CCW winding
//
XIrt
cGEO::edge_zlist(const CDs *sdesc, sLspec *lspec, const Point *p1,
    const Point *p2, bool cw, bool covered, int *code, Zlist **elist)
{
    if (elist)
        *elist = 0;
    int delta = TEST_WIDTH;
    Zlist *el = 0;
    if (p1->y == p2->y) {
        // horizontal
        if (p1->x == p2->x)
            return (XIok);

        if ((cw && p2->x > p1->x) || (!cw && p1->x > p2->x)) {
            // check the bottom
            if (code)
                *code = DRCeB;
            if (!elist)
                return (XIok);

            BBox BB(p1->x, p1->y, p2->x, p1->y + delta);
            BB.fixLR();
            SIlexprCx cx(sdesc, CDMAXCALLDEPTH, &BB);
            XIrt ret = lspec->getZlist(&cx, &el, false);
            if (ret != XIok)
                return (ret);

            for (Zlist *z = el; z; z = z->next) {
                z->Z.yu = z->Z.yl;
                z->Z.xur = z->Z.xlr;
                z->Z.xul = z->Z.xll;
            }
        }
        else {
            // check the top
            if (code)
                *code = DRCeT;
            if (!elist)
                return (XIok);

            BBox BB(p1->x, p1->y - delta, p2->x, p1->y);
            BB.fixLR();
            SIlexprCx cx(sdesc, CDMAXCALLDEPTH, &BB);
            XIrt ret = lspec->getZlist(&cx, &el, false);
            if (ret != XIok)
                return (ret);

            for (Zlist *z = el; z; z = z->next) {
                z->Z.yl = z->Z.yu;
                z->Z.xlr = z->Z.xur;
                z->Z.xll = z->Z.xul;
            }
        }
    }
    else if (p1->x == p2->x) {
        // vertical
        if ((cw && p2->y > p1->y) || (!cw && p1->y > p2->y)) {
            // check right
            if (code)
                *code = DRCeR;
            if (!elist)
                return (XIok);

            BBox BB(p1->x - delta, p1->y, p1->x, p2->y);
            BB.fixBT();
            SIlexprCx cx(sdesc, CDMAXCALLDEPTH, &BB);
            XIrt ret = lspec->getZlist(&cx, &el, false);
            if (ret != XIok)
                return (ret);

            Zlist *zp = 0, *zn;
            for (Zlist *z = el; z; z = zn) {
                zn = z->next;
                if (z->Z.xlr != z->Z.xur) {
                    if (!zp)
                        el = zn;
                    else
                        zp->next = zn;
                    delete z;
                    continue;
                }
                z->Z.xul = z->Z.xur;
                z->Z.xll = z->Z.xlr;
                zp = z;
            }
        }
        else {
            // check left
            if (code)
                *code = DRCeL;
            if (!elist)
                return (XIok);

            BBox BB(p1->x, p1->y, p1->x + delta, p2->y);
            BB.fixBT();
            SIlexprCx cx(sdesc, CDMAXCALLDEPTH, &BB);
            XIrt ret = lspec->getZlist(&cx, &el, false);
            if (ret != XIok)
                return (ret);

            Zlist *zp = 0, *zn;
            for (Zlist *z = el; z; z = zn) {
                zn = z->next;
                if (z->Z.xll != z->Z.xul) {
                    if (!zp)
                        el = zn;
                    else
                        zp->next = zn;
                    delete z;
                    continue;
                }
                z->Z.xur = z->Z.xul;
                z->Z.xlr = z->Z.xll;
                zp = z;
            }
        }
    }
    else {
        // non-manhattan

        int edge_code;
        if (p2->y > p1->y)
            edge_code = cw ? DRCeCWA : DRCeCCWA;
        else
            edge_code = cw ? DRCeCWD : DRCeCCWD;
        if (code)
            *code = edge_code;
        if (!elist)
            return (XIok);

        double d = Point::distance(p1, p2);
        int dx = p2->x - p1->x;
        int dy = p2->y - p1->y;
        double co, si;
        if (edge_code == DRCeCCWA || edge_code == DRCeCCWD) {
            co = dx/d;
            si = dy/d;
        }
        else {
            co = -dx/d;
            si = -dy/d;
        }
        double wt = TEST_WIDTH/(fabs(si) + TEST_WIDTH*0.01);
        double ddx = co*wt;
        double ddy = si*wt;
        Point p1p, p2p;
        p1p.set(mmRnd(p1->x + ddy), mmRnd(p1->y - ddx));
        p2p.set(mmRnd(p2->x + ddy), mmRnd(p2->y - ddx));

        Zoid Zl, Zu;
        if (edge_code == DRCeCWA) {
            if (p2->x > p1->x) {
                Zl = Zoid(p1->x, p2->x, p1->y, p2->x, p2->x, p2->y);
                Zu = Zoid(p1p.x, p1p.x, p1p.y, p1p.x, p2p.x, p2p.y);
            }
            else {
                Zl = Zoid(p2p.x, p1p.x, p1p.y, p2p.x, p2p.x, p2p.y);
                Zu = Zoid(p1->x, p1->x, p1->y, p2->x, p1->x, p2->y);
            }
        }
        else if (edge_code == DRCeCWD) {
            if (p2->x > p1->x) {
                Zl = Zoid(p1->x, p2->x, p2->y, p1->x, p1->x, p1->y);
                Zu = Zoid(p2p.x, p2p.x, p2p.y, p1p.x, p2p.x, p1p.y);
            }
            else {
                Zl = Zoid(p2p.x, p1p.x, p2p.y, p1p.x, p1p.x, p1p.y);
                Zu = Zoid(p2->x, p2->x, p2->y, p2->x, p1->x, p1->y);
            }
        }
        else if (edge_code == DRCeCCWA) {
            if (p2->x > p1->x) {
                Zl = Zoid(p1p.x, p2p.x, p1p.y, p2p.x, p2p.x, p2p.y);
                Zu = Zoid(p1->x, p1->x, p1->y, p1->x, p2->x, p2->y);
            }
            else {
                Zl = Zoid(p2->x, p1->x, p1->y, p2->x, p2->x, p2->y);
                Zu = Zoid(p1p.x, p1p.x, p1p.y, p2p.x, p1p.x, p2p.y);
            }
        }
        else if (edge_code == DRCeCCWD) {
            if (p2->x > p1->x) {
                Zl = Zoid(p1p.x, p2p.x, p2p.y, p1p.x, p1p.x, p1p.y);
                Zu = Zoid(p2->x, p2->x, p2->y, p1->x, p2->x, p1->y);
            }
            else {
                Zl = Zoid(p2->x, p1->x, p2->y, p1->x, p1->x, p1->y);
                Zu = Zoid(p2p.x, p2p.x, p2p.y, p2p.x, p1p.x, p1p.y);
            }
        }

        BBox BB(p1->x, p1->y, p2->x, p2->y);
        BB.fix();
        SIlexprCx cx(sdesc, CDMAXCALLDEPTH, &BB);
        XIrt ret = lspec->getZlist(&cx, &el, false);
        if (ret != XIok)
            return (ret);

        Zlist *zt = new Zlist(&Zu);
        zt->next = new Zlist(&Zl);
        ret = Zlist::zl_andnot(&el, zt);
        if (ret != XIok)
            return (ret);

        // Important!
        el = Zlist::filter_slivers(el, 1);

        if (edge_code == DRCeCWA || edge_code == DRCeCCWD) {
            Zlist *zp = 0, *zn;
            for (Zlist *z = el; z; z = zn) {
                zn = z->next;
                if (!check_colinear(p1, p2, z->Z.xlr, z->Z.yl, 1) ||
                        !check_colinear(p1, p2, z->Z.xur, z->Z.yu, 1)) {
                    if (!zp)
                        el = zn;
                    else
                        zp->next = zn;
                    delete z;
                    continue;
                }
                z->Z.xll = z->Z.xlr;
                z->Z.xul = z->Z.xur;
                zp = z;
            }
        }
        else {
            Zlist *zp = 0, *zn;
            for (Zlist *z = el; z; z = zn) {
                zn = z->next;
                if (!check_colinear(p1, p2, z->Z.xll, z->Z.yl, 1) ||
                        !check_colinear(p1, p2, z->Z.xul, z->Z.yu, 1)) {
                    if (!zp)
                        el = zn;
                    else
                        zp->next = zn;
                    delete z;
                    continue;
                }
                z->Z.xlr = z->Z.xll;
                z->Z.xur = z->Z.xul;
                zp = z;
            }
        }
    }

    // Want to keep list sorted for predictable order in user tests,
    // but need ascending order for some operations below.
    bool reverse = (p2->y < p1->y || (p2->y == p1->y && p2->x < p1->x));
    if (reverse) {
        const Point *pt = p1;
        p1 = p2;
        p2 = pt;
    }

    if (el) {
        if (el->next) {
            el = Zlist::sort(el);  // ascending in yl, left
            // el is closest to p1

            // Combine if possible, allow for overlap.  We know that
            // all zoids are colinear.

            if (p1->y == p2->y) {
                Zlist *zp = el, *zn;
                for (Zlist *z = zp->next; z; z = zn) {
                    zn = z->next;
                    if (z->Z.xll - zp->Z.xul <= 0) {
                        if (z->Z.xlr > zp->Z.xlr) {
                            zp->Z.xlr = z->Z.xlr;
                            zp->Z.xur = z->Z.xur;
                        }
                        zp->next = zn;
                        delete z;
                        continue;
                    }
                    zp = z;
                }
            }
            else if (p1->x == p2->x) {
                Zlist *zp = el, *zn;
                for (Zlist *z = zp->next; z; z = zn) {
                    zn = z->next;
                    if (z->Z.yl - zp->Z.yu <= 0) {
                        if (z->Z.yu > zp->Z.yu)
                            zp->Z.yu = z->Z.yu;
                        zp->next = zn;
                        delete z;
                        continue;
                    }
                    zp = z;
                }
            }
            else if (abs(p1->x - p2->x) > abs(p1->y - p2->y)) {
                if (p2->x > p1->x) {
                    Zlist *zp = el, *zn;
                    for (Zlist *z = zp->next; z; z = zn) {
                        zn = z->next;
                        if (z->Z.xlr - zp->Z.xur <= 0) {
                            if (z->Z.xur > zp->Z.xur) {
                                zp->Z.xul = z->Z.xul;
                                zp->Z.xur = z->Z.xur;
                                zp->Z.yu = z->Z.yu;
                            }
                            zp->next = zn;
                            delete z;
                            continue;
                        }
                        zp = z;
                    }
                }
                else {
                    Zlist *zp = el, *zn;
                    for (Zlist *z = zp->next; z; z = zn) {
                        zn = z->next;
                        if (z->Z.xlr - zp->Z.xur >= 0) {
                            if (z->Z.xur < zp->Z.xur) {
                                zp->Z.xul = z->Z.xul;
                                zp->Z.xur = z->Z.xur;
                                zp->Z.yu = z->Z.yu;
                            }
                            zp->next = zn;
                            delete z;
                            continue;
                        }
                        zp = z;
                    }
                }
            }
            else {
                Zlist *zp = el, *zn;
                for (Zlist *z = zp->next; z; z = zn) {
                    zn = z->next;
                    if (z->Z.yl - zp->Z.yu <= 0) {
                        if (z->Z.yu > zp->Z.yu) {
                            zp->Z.yu = z->Z.yu;
                            zp->Z.xul = z->Z.xul;
                            zp->Z.xur = z->Z.xur;
                        }
                        zp->next = zn;
                        delete z;
                        continue;
                    }
                    zp = z;
                }
            }
        }

        // Set the end points exactly if close to p1 or p2.
        if (abs(el->Z.yl - p1->y) <= 0 && abs(el->Z.xll - p1->x) <= 0) {
            if (el->Z.xlr == el->Z.xll)
                el->Z.xlr = p1->x;
            else if (el->Z.xul == el->Z.xll)
                el->Z.xul = p1->x;
            el->Z.xll = p1->x;
            el->Z.yl = p1->y;
        }

        Zlist *ze = el;
        while (ze->next)
            ze = ze->next;

        if (abs(ze->Z.yu - p2->y) <= 0 && abs(ze->Z.xur - p2->x) <= 0) {
            if (ze->Z.xul == ze->Z.xur)
                ze->Z.xul = p2->x;
            else if (ze->Z.xlr == ze->Z.xur)
                ze->Z.xlr = p2->x;
            ze->Z.xur = p2->x;
            ze->Z.yu = p2->y;
        }

        // Check/remove nonsense.
        Zlist *zp = 0, *zn;
        for (Zlist *z = el; z; z = zn) {
            zn = z->next;
            if (z->Z.xur == z->Z.xll && z->Z.yu == z->Z.yl) {
                if (!zp)
                    el = zn;
                else
                    zp->next = zn;
                delete z;
                continue;
            }
            zp = z;
        }
    }
    if (!covered)
        el = invert(el, p1, p2);

    if (reverse) {
        Zlist *z0 = 0;
        while (el) {
            Zlist *en = el->next;
            el->next = z0;
            z0 = el;
            el = en;
        }
        el = z0;
    }

    *elist = el;
    return (XIok);
}


namespace {
    // Polarity inversion, p1, p2 and list are in ascending order.
    // The passed elist is freed.
    //
    Zlist *
    invert(Zlist *elist, const Point *p1, const Point *p2)
    {
        if (!elist) {
            Zoid Z;
            Z.yl = p1->y;
            Z.yu = p2->y;
            Z.xll = p1->x;
            Z.xur = p2->x;
            if (Z.yl != Z.yu) {
                Z.xul = Z.xur;
                Z.xlr = Z.xll;
            }
            else {
                Z.xul = Z.xll;
                Z.xlr = Z.xur;
            }
            return (new Zlist(&Z));
        }

        Zlist *z0 = 0, *ze = 0;
        if (elist->Z.yl != p1->y || elist->Z.xll != p1->x) {
            Zoid Z;
            Z.yl = p1->y;
            Z.yu = elist->Z.yl;
            Z.xll = p1->x;
            Z.xur = elist->Z.xll;
            if (Z.yl != Z.yu) {
                Z.xul = Z.xur;
                Z.xlr = Z.xll;
            }
            else {
                Z.xul = Z.xll;
                Z.xlr = Z.xur;
            }
            if (!z0)
                z0 = ze = new Zlist(&Z);
            else {
                ze->next = new Zlist(&Z);
                ze = ze->next;
            }
        }
        Zlist *zn;
        for (const Zlist *z = elist; z; z = zn) {
            zn = z->next;
            Zoid Z;
            if (!zn) {
                if (z->Z.yu != p2->y || z->Z.xur != p2->x) {
                    Z.yl = z->Z.yu;
                    Z.yu = p2->y;
                    Z.xll = z->Z.xur;
                    Z.xur = p2->x;
                }
                else
                    break;
            }
            else {
                Z.yl = z->Z.yu;
                Z.yu = zn->Z.yl;
                Z.xll = z->Z.xur;
                Z.xur = zn->Z.xll;
            }
            if (Z.yl != Z.yu) {
                Z.xul = Z.xur;
                Z.xlr = Z.xll;
            }
            else {
                Z.xul = Z.xll;
                Z.xlr = Z.xur;
            }
            if (!z0)
                z0 = ze = new Zlist(&Z);
            else {
                ze->next = new Zlist(&Z);
                ze = ze->next;
            }
        }
        Zlist::destroy(elist);
        return (z0);
    }
}


// Static function.
// Copy and clip the zoids in zl1 so that only the portions that also
// share the edge with a zoid in zl2 are included.  List zl2 is freed
//
Zlist *
cGEO::merge_edge_zlists(const Zlist *zl1, int code1, Zlist *zl2)
{
    if (!zl2)
        return (0);
    if (!zl1) {
        Zlist::destroy(zl2);
        return (0);
    }

    // zl1 is always an 'outside' edge list
    const Zlist *z1;
    Zlist *z2, *zx;
    Zlist *zx0 = 0, *zxe = 0;
    switch (code1) {
    case DRCeB:
    case DRCeT:
        for (z1 = zl1; z1; z1 = z1->next) {
            for (z2 = zl2; z2; z2 = z2->next) {
                if (z1->Z.xur <= z2->Z.xll || z2->Z.xur <= z1->Z.xll)
                    continue;
                zx = new Zlist;
                zx->Z.xul = zx->Z.xll = mmMax(z1->Z.xll, z2->Z.xll);
                zx->Z.xur = zx->Z.xlr = mmMin(z1->Z.xur, z2->Z.xur);
                zx->Z.yl = z1->Z.yl;
                zx->Z.yu = z1->Z.yu;
                if (!zx0)
                    zx0 = zxe = zx;
                else {
                    zxe->next = zx;
                    zxe = zxe->next;
                }
            }
        }
        break;
    case DRCeL:
    case DRCeR:
        for (z1 = zl1; z1; z1 = z1->next) {
            for (z2 = zl2; z2; z2 = z2->next) {
                if (z1->Z.yu <= z2->Z.yl || z2->Z.yu <= z1->Z.yl)
                    continue;
                zx = new Zlist;
                zx->Z.xul = zx->Z.xll = z1->Z.xll;
                zx->Z.xur = zx->Z.xlr = z1->Z.xur;
                zx->Z.yl = mmMax(z1->Z.yl, z2->Z.yl);
                zx->Z.yu = mmMin(z1->Z.yu, z2->Z.yu);
                if (!zx0)
                    zx0 = zxe = zx;
                else {
                    zxe->next = zx;
                    zxe = zxe->next;
                }
            }
        }
        break;
    case DRCeCWA:
    case DRCeCCWA:
    case DRCeCWD:
    case DRCeCCWD:
        if (abs(zl1->Z.xur - zl1->Z.xll) > zl1->Z.yu - zl1->Z.yl) {
            // sort by x
            for (z1 = zl1; z1; z1 = z1->next) {
                for (z2 = zl2; z2; z2 = z2->next) {
                    if (z1->Z.xur > z1->Z.xll) {
                        if (z1->Z.xur < z2->Z.xll || z2->Z.xur < z1->Z.xll)
                            continue;
                    }
                    else {
                        if (z1->Z.xur > z2->Z.xll || z2->Z.xur > z1->Z.xll)
                            continue;
                    }
                    zx = new Zlist;
                    if (!zx0)
                        zx0 = zxe = zx;
                    else {
                        zxe->next = zx;
                        zxe = zxe->next;
                    }
                    if ((z1->Z.xur > z1->Z.xll && z1->Z.xur > z2->Z.xur) ||
                            (z1->Z.xur < z1->Z.xll && z1->Z.xur < z2->Z.xur)) {
                        zx->Z.yu = z2->Z.yu;
                        zx->Z.xur = z2->Z.xur;
                        zx->Z.xul = z2->Z.xul;
                    }
                    else {
                        zx->Z.yu = z1->Z.yu;
                        zx->Z.xur = z1->Z.xur;
                        zx->Z.xul = z1->Z.xul;
                    }
                    if ((z1->Z.xll < z1->Z.xur && z1->Z.xll < z2->Z.xll) ||
                            (z1->Z.xll > z1->Z.xur && z1->Z.xll > z2->Z.xll)) {
                        zx->Z.yl = z2->Z.yl;
                        zx->Z.xlr = z2->Z.xlr;
                        zx->Z.xll = z2->Z.xll;
                    }
                    else {
                        zx->Z.yl = z1->Z.yl;
                        zx->Z.xlr = z1->Z.xlr;
                        zx->Z.xll = z1->Z.xll;
                    }
                }
            }
        }
        else {
            // sort by y
            for (z1 = zl1; z1; z1 = z1->next) {
                for (z2 = zl2; z2; z2 = z2->next) {
                    if (z1->Z.yu < z2->Z.yl || z2->Z.yu < z1->Z.yl)
                        continue;
                    zx = new Zlist;
                    if (!zx0)
                        zx0 = zxe = zx;
                    else {
                        zxe->next = zx;
                        zxe = zxe->next;
                    }
                    if (z1->Z.yu > z2->Z.yu) {
                        zx->Z.yu = z2->Z.yu;
                        zx->Z.xul = z2->Z.xul;
                        zx->Z.xur = z2->Z.xur;
                    }
                    else {
                        zx->Z.yu = z1->Z.yu;
                        zx->Z.xul = z1->Z.xul;
                        zx->Z.xur = z1->Z.xur;
                    }
                    if (z1->Z.yl < z2->Z.yl) {
                        zx->Z.yl = z2->Z.yl;
                        zx->Z.xll = z2->Z.xll;
                        zx->Z.xlr = z2->Z.xlr;
                    }
                    else {
                        zx->Z.yl = z1->Z.yl;
                        zx->Z.xll = z1->Z.xll;
                        zx->Z.xlr = z1->Z.xlr;
                    }
                }
            }
        }
        break;
    }
    Zlist::destroy(zl2);
    return (zx0);
}


// Static function.
// Compute the distances along the edge from the vertices that are covered
// on the 'outside'.  These are used to test for sufficient overlap between
// objects
//
void
cGEO::edge_overlap(const Zlist *elist, int code, const Point *p1,
    const Point *p2, int *cw1, int *cw2)
{
    const Zlist *zl, *min, *max;
    int maxp, minp, mn, mx;
    switch (code) {
    case DRCeT:
    case DRCeB:
        maxp = mn = mmMax(p1->x, p2->x);
        minp = mx = mmMin(p1->x, p2->x);
        for (zl = elist; zl; zl = zl->next) {
            if (mn > zl->Z.xll)
                mn = zl->Z.xll;
            if (mx < zl->Z.xur)
                mx = zl->Z.xur;
        }
        if (p2->x == maxp) {
            *cw2 = maxp - mx;
            *cw1 = mn - minp;
        }
        else {
            *cw1 = maxp - mx;
            *cw2 = mn - minp;
        }
        break;
    case DRCeR:
    case DRCeL:
        maxp = mn = mmMax(p1->y, p2->y);
        minp = mx = mmMin(p1->y, p2->y);
        for (zl = elist; zl; zl = zl->next) {
            if (mn > zl->Z.yl)
                mn = zl->Z.yl;
            if (mx < zl->Z.yu)
                mx = zl->Z.yu;
        }
        if (p2->y  == maxp) {
            *cw2 = maxp - mx;
            *cw1 = mn - minp;
        }
        else {
            *cw1 = maxp - mx;
            *cw2 = mn - minp;
        }
        break;
    case DRCeCWA:
    case DRCeCCWA:
        if (!elist) {
            *cw2 = *cw1 = (int)(sqrt((p2->y - p1->y)*(double)(p2->y - p1->y) +
                (p2->x - p1->x)*(double)(p2->x - p1->x)));
            break;
        }
        max = elist;
        min = elist;
        if (p2->x > p1->x) {
            for (zl = elist->next; zl; zl = zl->next) {
                if (zl->Z.yl < min->Z.yl ||
                        (zl->Z.yl == min->Z.yl && zl->Z.xll < min->Z.xll))
                    min = zl;
                if (zl->Z.yu > max->Z.yu ||
                        (zl->Z.yu == max->Z.yu && zl->Z.xur > max->Z.xur))
                    max = zl;
            }
        }
        else {
            for (zl = elist->next; zl; zl = zl->next) {
                if (zl->Z.yl < min->Z.yl ||
                        (zl->Z.yl == min->Z.yl && zl->Z.xll > min->Z.xll))
                    min = zl;
                if (zl->Z.yu > max->Z.yu ||
                        (zl->Z.yu == max->Z.yu && zl->Z.xur < max->Z.xur))
                    max = zl;
            }
        }
        *cw1 = (int)(sqrt((min->Z.yl - p1->y)*(double)(min->Z.yl - p1->y) +
            (min->Z.xll - p1->x)*(double)(min->Z.xll - p1->x)));
        *cw2 = (int)(sqrt((max->Z.yu - p2->y)*(double)(max->Z.yu - p2->y) +
            (max->Z.xur - p2->x)*(double)(max->Z.xur - p2->x)));
        break;
    case DRCeCWD:
    case DRCeCCWD:
        if (!elist) {
            *cw2 = *cw1 = (int)(sqrt((p2->y - p1->y)*(double)(p2->y - p1->y) +
                (p2->x - p1->x)*(double)(p2->x - p1->x)));
            break;
        }
        max = elist;
        min = elist;
        if (p1->x > p2->x) {
            for (zl = elist->next; zl; zl = zl->next) {
                if (zl->Z.yl < min->Z.yl ||
                        (zl->Z.yl == min->Z.yl && zl->Z.xll < min->Z.xll))
                    min = zl;
                if (zl->Z.yu > max->Z.yu ||
                        (zl->Z.yu == max->Z.yu && zl->Z.xur > max->Z.xur))
                    max = zl;
            }
        }
        else {
            for (zl = elist->next; zl; zl = zl->next) {
                if (zl->Z.yl < min->Z.yl ||
                        (zl->Z.yl == min->Z.yl && zl->Z.xll > min->Z.xll))
                    min = zl;
                if (zl->Z.yu > max->Z.yu ||
                        (zl->Z.yu == max->Z.yu && zl->Z.xur < max->Z.xur))
                    max = zl;
            }
        }
        *cw1 = (int)(sqrt((max->Z.yu - p1->y)*(double)(max->Z.yu - p1->y) +
            (max->Z.xur - p1->x)*(double)(max->Z.xur - p1->x)));
        *cw2 = (int)(sqrt((min->Z.yl - p2->y)*(double)(min->Z.yl - p2->y) +
            (min->Z.xll - p2->x)*(double)(min->Z.xll - p2->x)));
        break;
    }
}

