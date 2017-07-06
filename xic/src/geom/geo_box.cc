
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
 $Id: geo_box.cc,v 1.17 2015/06/11 05:54:06 stevew Exp $
 *========================================================================*/

#include "geo.h"
#include "geo_box.h"
#include "geo_poly.h"
#include "geo_wire.h"
#include <algorithm>


// Apply some cleverness to get the correct box from the CIF factors.
//
void
BBox::set_cif(int x, int y, int wid, int hei)
{
    // If the wid (x2-x1) is odd, then so is x1+x2.  Thus, as
    // x = (x1+x2)/2, the real value for x1+x2 is 2*x+1 (x > 0) or
    // 2*x-1 (x < 0).  Knowing x1+x2, and x1-x2, one has x1 and x2.
    //
    if (wid & 1) {
        int zz = 2*x + (x > 0 ? 1 : -1);
        int v1 = (zz + wid)/2;
        int v2 = (zz - wid)/2;
        left = mmMin(v1, v2);
        right = mmMax(v1, v2);
    }
    else {
        if (wid < 0)
            wid = -wid;
        wid >>= 1;
        left = x - wid;
        right = x + wid;
    }
    if (hei & 1) {
        int zz = 2*y + (y > 0 ? 1 : -1);
        int v1 = (zz + hei)/2;
        int v2 = (zz - hei)/2;
        bottom = mmMin(v1, v2);
        top = mmMax(v1, v2);
    }
    else {
        if (hei < 0)
            hei = -hei;
        hei >>= 1;
        bottom = y - hei;
        top = y + hei;
    }
}


bool
BBox::intersect(const Poly *p, bool touchok) const
{
    return (p->intersect(this, touchok));
}


bool
BBox::intersect(const Wire *w, bool touchok) const
{
    return (w->intersect(this, touchok));
}


// Return this clipped to BB.
//
Blist *
BBox::clip_to(const BBox *BB) const
{
    int l = mmMax(left, BB->left);
    int r = mmMin(right, BB->right);
    if (r <= l)
        return (0);
    int b = mmMax(bottom, BB->bottom);
    int t = mmMin(top, BB->top);
    if (t <= b)
        return (0);
    return (new Blist(l, b, r, t));
}


// Return a list of the parts of this that do not intersect BB.
//
Blist *
BBox::clip_out(const BBox *BB, bool *no_ovl) const
{
    if (no_ovl) {
        *no_ovl = false;
        if (!BB || BB->right <= left || BB->left >= right ||
                BB->top <= bottom || BB->bottom >= top) {
            *no_ovl = true;
            return (0);
        }
    }
    else if (!BB || BB->right <= left || BB->left >= right ||
            BB->top <= bottom || BB->bottom >= top)
        return (new Blist(this, 0));

    Blist *b0 = 0;
    if (top > BB->top)
        b0 = new Blist(left, BB->top, right, top, b0);
    if (bottom < BB->bottom)
        b0 = new Blist(left, bottom, right, BB->bottom, b0);
    int t = mmMin(top, BB->top);
    int b = mmMax(bottom, BB->bottom);
    if (left < BB->left)
        b0 = new Blist(left, b, BB->left, t, b0);
    if (right > BB->right)
        b0 = new Blist(BB->right, b, right, t, b0);
    return (b0);
}


// Return a list of the parts of this that don't intersect any of the
// elements of bc.
//
Blist *
BBox::clip_out(const Blist *bc) const
{
    if (!bc)
        return (new Blist(this, 0));
    Blist *bl = clip_out(&bc->BB);
    bc = bc->next;
    while (bc && bl) {
        Blist *b0 = 0;
        for (Blist *b = bl; b; b = b->next) {
            Blist *bx = b->BB.clip_out(&bc->BB);
            if (bx) {
                if (!b0)
                    b0 = bx;
                else {
                    Blist *btmp = bx;
                    while (bx->next)
                        bx = bx->next;
                    bx->next = b0;
                    b0 = btmp;
                }
            }
        }
        Blist::destroy(bl);
        bl = b0;
        bc = bc->next;
    }
    return (bl);
}


// Return (this & !yl0).
//
Blist *
BBox::clip_out(const BYlist *yl0) const
{
    BYlist *yt = new BYlist(new Blist(this, 0));
    for (const BYlist *y = yl0; y; y = y->next) {
        if (y->yl >= top)
            continue;
        if (y->yu <= bottom)
            break;
        for (Blist *b = y->blist; b; b = b->next) {
            if (b->BB.right <= left)
                continue;
            if (b->BB.left >= right)
                break;
            if (b->BB.bottom >= top)
                continue;
            yt = BYlist::clip_out(yt, &b->BB);
            if (!yt)
                break;
        }
        if (!yt)
            break;
    }
    return (BYlist::toblist(yt));
}


// Clip to or combine with BB.  If there is overlap or boxes can be
// merged, new boxes are returned.
//
Blist *
BBox::clip_merge(const BBox *BB) const
{
    if (right < BB->left || left > BB->right ||
            top < BB->bottom || bottom > BB->top)
        // no touching or overlap
        return (0);

    if (left == BB->left && right == BB->right) {
        // combine
        Blist *b0 = new Blist;
        b0->BB.left = left;
        b0->BB.bottom = mmMin(bottom, BB->bottom);
        b0->BB.right = right;
        b0->BB.top = mmMax(top, BB->top);
        return (b0);
    }
    if (bottom == BB->bottom && top == BB->top) {
        // combine
        Blist *b0 = new Blist;
        b0->BB.left = mmMin(left, BB->left);
        b0->BB.bottom = bottom;
        b0->BB.right = mmMax(right, BB->right);
        b0->BB.top = top;
        return (b0);
    }
    if (right <= BB->left || left >= BB->right ||
            top <= BB->bottom || bottom >= BB->top)
        // no overlap
        return (0);

    // overlapping boxes
    Blist *BBtop = 0;
    Blist *BBbot = 0;
    if (BB->top > top) {
        BBtop = new Blist;
        BBtop->BB = *BB;
        BBtop->BB.bottom = top;
    }
    else if (top > BB->top) {
        BBtop = new Blist;
        BBtop->BB = *this;
        BBtop->BB.bottom = BB->top;
    }
    if (BB->bottom < bottom) {
        BBbot = new Blist;
        BBbot->BB = *BB;
        BBbot->BB.top = bottom;
    }
    else if (bottom < BB->bottom) {
        BBbot = new Blist;
        BBbot->BB = *this;
        BBbot->BB.top = BB->bottom;
    }
    // look at the overlap region
    BBox nBB;
    nBB.top = mmMin(top, BB->top);
    nBB.bottom = mmMax(bottom, BB->bottom);
    nBB.left = mmMin(left, BB->left);
    nBB.right = mmMax(right, BB->right);
    Blist *b0 = 0, *end = 0;
    if (BBtop) {
        if (nBB.left == BBtop->BB.left && nBB.right == BBtop->BB.right) {
            nBB.top = BBtop->BB.top;
            delete BBtop;
        }
        else
            b0 = end = BBtop;
    }
    if (BBbot) {
        if (nBB.left == BBbot->BB.left && nBB.right == BBbot->BB.right) {
            nBB.bottom = BBbot->BB.bottom;
            delete BBbot;
        }
        else {
            if (!b0)
                b0 = end = BBbot;
            else {
                end->next = BBbot;
                end = end->next;
            }
        }
    }
    if (!b0)
        b0 = new Blist(&nBB, 0);
    else
        end->next = new Blist(&nBB, 0);
    return (b0);
}


// Magnify the box around x, y.
//
void
BBox::scale(double magn, int x, int y)
{
    left = x + mmRnd((left - x)*magn);
    bottom = y + mmRnd((bottom - y)*magn);
    right = x + mmRnd((right - x)*magn);
    top = y + mmRnd((top - y)*magn);
}
// End of BBox functions


namespace {
    // Default sort - ascending in bottom, left
    //
    inline bool
    bcmp0(const Blist *bl1, const Blist *bl2)
    {
        const BBox *b1 = &bl1->BB;
        const BBox *b2 = &bl2->BB;
        if (b1->bottom < b2->bottom)
            return (true);
        if (b1->bottom > b2->bottom)
            return (false);
        return (b1->left < b2->left);
    }


    // Descending in top, ascending in left
    //
    inline bool
    bcmp1(const Blist *bl1, const Blist *bl2)
    {
        const BBox *b1 = &bl1->BB;
        const BBox *b2 = &bl2->BB;
        if (b1->top > b2->top)
            return (true);
        if (b1->top < b2->top)
            return (false);
        return (b1->left < b2->left);
    }
}


// Static function.
//
Blist *
Blist::sort(Blist *thisbl, int mode)
{
    int len = 0;
    Blist *b0 = thisbl;
    for (Blist *b = b0; b; b = b->next, len++) ;

    if (len > 1) {
        Blist **tb = new Blist*[len];
        int i = 0;
        for (Blist *b = b0; b; b = b->next, i++)
            tb[i] = b;
        if (mode)
            std::sort(tb, tb + len, bcmp1);
        else
            std::sort(tb, tb + len, bcmp0);
        len--;
        for (i = 0; i < len; i++)
            tb[i]->next = tb[i+1];
        tb[i]->next = 0;
        b0 = tb[0];
        delete [] tb;
    }
    return (b0);
}


// Static function.
// Insert an element for tBB, clipped to and merged with the existing
// elements.
//
Blist *
Blist::insert_merge(Blist *thisbl, const BBox *tBB)
{
    BYlist *y = 0;
    if (thisbl)
        y = new BYlist(thisbl);
    y = BYlist::insert_merge(y, tBB);
    return (BYlist::toblist(y));
}


// Static function.
// Process the box list making no two boxes overlap, and merge boxes
// when possible.  Boxes with zero area are removed.
//
Blist *
Blist::merge(Blist *thisbl)
{
    Blist *bt = thisbl;
    if (!bt || !bt->next)
        return (bt);
    BYlist *y = new BYlist(bt);
    y = BYlist::merge(y);
    return (BYlist::toblist(y));
}


// Static function.
// Return a list of the parts of this that don't intersect BB, this
// is deleted.
//
Blist *
Blist::clip_out(Blist *thisbl, const BBox *pBB)
{
    Blist *b0 = 0, *be = 0, *bnxt;
    for (Blist *bl = thisbl; bl; bl = bnxt) {
        bnxt = bl->next;
        Blist *bx = bl->BB.clip_out(pBB);
        if (!b0)
            b0 = be = bx;
        else {
            while (be->next)
                be = be->next;
            be->next = bx;
        }
        delete bl;
    }
    return (merge(b0));
}


// Static function.
// Return a list of the parts of this that don't intersect any of the
// elements of bc, this is deleted.
//
Blist *
Blist::clip_out(Blist *thisbl, const Blist *bc)
{
    Blist *b0 = 0, *be = 0, *bnxt;
    for (Blist *bl = thisbl; bl; bl = bnxt) {
        bnxt = bl->next;
        Blist *bx = bl->BB.clip_out(bc);
        if (!b0)
            b0 = be = bx;
        else {
            while (be->next)
                be = be->next;
            be->next = bx;
        }
        delete bl;
    }
    return (merge(b0));
}


// Static function.
// Return the parts of this that overlap BB, this is consumed.
//
Blist *
Blist::clip_to(Blist *thisbl, const BBox *pBB)
{
    Blist *b0 = 0, *be = 0, *bnxt;
    for (Blist *bl = thisbl; bl; bl = bnxt) {
        bnxt = bl->next;
        Blist *bx = bl->BB.clip_to(pBB);
        if (!b0)
            b0 = be = bx;
        else {
            while (be->next)
                be = be->next;
            be->next = bx;
        }
        delete bl;
    }
    return (merge(b0));
}


// Static function.
// Return the parts of this that overlap any of the parts of bc, this
// is consumed.
//
Blist *
Blist::clip_to(Blist *thisbl, const Blist *bc)
{
    Blist *b0 = 0, *be = 0, *bnxt;
    for (Blist *bl = thisbl; bl; bl = bnxt) {
        bnxt = bl->next;
        for (const Blist *b = bc; b; b = b->next) {
            Blist *bx = bl->BB.clip_to(&b->BB);
            if (!b0)
                b0 = be = bx;
            else {
                while (be->next)
                    be = be->next;
                be->next = bx;
            }
        }
        delete bl;
    }
    return (merge(b0));
}


// Static function.
// Return true if BB overlaps or touches one of the elements
// of bl, false otherwise.
//
bool
Blist::intersect(const Blist *thisbl, const BBox *pBB, bool touchok)
{
    if (!touchok) {
        for (const Blist *bl = thisbl; bl; bl = bl->next) {
            if (pBB->left >= bl->BB.right || pBB->right <= bl->BB.left ||
                    pBB->bottom >= bl->BB.top || pBB->top <= bl->BB.bottom)
                continue;
            return (true);
        }
    }
    else {
        for (const Blist *bl = thisbl; bl; bl = bl->next) {
            if (pBB->left > bl->BB.right || pBB->right < bl->BB.left ||
                    pBB->bottom > bl->BB.top || pBB->top < bl->BB.bottom)
                continue;
            return (true);
        }
    }
    return (false);
}
// End of Blist functions


BYlist::BYlist(Blist *b0, bool sub)
{
    blist = 0;
    yu = yl = 0;
    next = 0;

    if (b0) {
        if (sub) {
            blist = b0;
            yu = b0->BB.top;
            yl = b0->BB.bottom;
        }
        else {
            b0 = Blist::sort(b0, 1);
            blist = b0;
            yu = b0->BB.top;
            yl = b0->BB.bottom;
            BYlist *ylast = this;
            while (b0) {
                if (ylast->yl > b0->BB.bottom)
                    ylast->yl = b0->BB.bottom;
                if (b0->next && b0->next->BB.top != ylast->yu) {
                    ylast->next = new BYlist(b0->next, true);
                    ylast = ylast->next;
                    Blist *bt = b0->next;
                    b0->next = 0;
                    b0 = bt;
                    continue;
                }
                b0 = b0->next;
            }
        }
    }
}


// Static function.
// Add BB to the list, clipping and merging.
//
BYlist *
BYlist::insert_merge(BYlist *thisby, const BBox *BB)
{
    Blist *bl = BB->clip_out(thisby);
    if (!bl) {
        // BB is already fully covered.
        return (thisby);
    }

    BYlist *y = thisby;
    Blist *bn;
    for (Blist *b = bl; b; b = bn) {
        bn = b->next;
        b->next = 0;

        while (y) {
            Blist *bx;
            y = y->trymerge(&b->BB, &bx);
            if (bx) {
                delete b;
                b = bx;
                continue;
            }
            break;
        }
        y = insert(y, b);
    }
    return (y);
}


// Static function.
// Clip and merge the elements.
//
BYlist *
BYlist::merge(BYlist *thisby)
{
    if (!thisby)
        return (0);
    BYlist *y = thisby->clip();
    if (!y)
        return (0);
    y->merge_rows();
    for (;;) {
        bool chg = y->merge_cols();
        if (chg)
            chg = y->merge_rows();
        if (!chg)
            break;
    }
    return (y);
}


// Static function.
//
BYlist *
BYlist::clip_out(BYlist *thisby, const BBox *BB)
{
    BYlist *yl0 = thisby, *yp = 0, *yn;
    for (BYlist *y = yl0; y; y = yn) {
        yn = y->next;
        // yn is next row not added in insert()
        if (y->yl >= BB->top) {
            yp = y;
            continue;
        }
        if (y->yu <= BB->bottom)
            break;
        Blist *b0 = 0, *be = 0;
        Blist *bn, *bp = 0;
        for (Blist *b = y->blist; b; b = bn) {
            bn = b->next;
            if (b->BB.left >= BB->right)
                break;
            if (b->BB.bottom >= BB->top || b->BB.right <= BB->left) {
                bp = b;
                continue;
            }

            bool no_ovl;
            Blist *bt = (b->BB).clip_out(BB, &no_ovl);
            if (no_ovl) {
                bp = b;
                continue;
            }
            if (!bp)
                y->blist = bn;
            else
                bp->next = bn;
            delete b;
            if (bt) {
                if (be) {
                    while (be->next)
                        be = be->next;
                    be->next = bt;
                }
                else
                    b0 = be = bt;
            }
        }
        insert(y, b0);
        if (!y->blist) {
            if (!yp)
                yl0 = y->next; // may != yn!
            else
                yp->next = y->next;
            delete y;
            continue;
        }
        yp = y;
    }
    return (yl0);
}


// Static function.
//
BYlist *
BYlist::insert(BYlist *thisby, Blist *b0)
{
    BYlist *y0 = thisby;
    if (!y0)
        return (new BYlist(b0));
    Blist *bn;
    for (Blist *b = b0; b; b = bn) {
        bn = b->next;
        if (b->BB.top > y0->yu) {
            BYlist *yy = new BYlist(0);
            yy->next = y0;
            y0 = yy;
            yy->blist = b;
            b->next = 0;
            yy->yu = b->BB.top;
            yy->yl = b->BB.bottom;
            continue;
        }
        for (BYlist *y = y0; y; y = y->next) {
            BYlist *yy;
            if (y->yu == b->BB.top) {
                yy = y;
                if (yy->yl > b->BB.bottom)
                    yy->yl = b->BB.bottom;
            }
            else if (!y->next || y->next->yu < b->BB.top) {
                yy = new BYlist(0);
                yy->next = y->next;
                y->next = yy;
                yy->yu = b->BB.top;
                yy->yl = b->BB.bottom;
            }
            else
                continue;

            Blist *bb = yy->blist;
            if (!bb || b->BB.left <= bb->BB.left) {
                b->next = bb;
                yy->blist = b;
            }
            else {
                for ( ; bb->next; bb = bb->next) {
                    if (b->BB.left <= bb->next->BB.left)
                        break;
                }
                b->next = bb->next;
                bb->next = b;
            }
            break;
        }
    }
    return (y0);
}


// Static function.
//
Blist *
BYlist::toblist(BYlist *thisby)
{
    Blist *b0 = 0, *be = 0;
    BYlist *yn;
    for (BYlist *y = thisby; y; y = yn) {
        yn = y->next;
        if (y->blist) {
            if (!b0)
                b0 = be = y->blist;
            else
                be->next = y->blist;
            while (be->next)
                be = be->next;
            y->blist = 0;
        }
        delete y;
    }
    return (b0);
}


// Private support function for insert_merge().
// If an element in the list is adjacent to BB such that it can be
// combined with BB, pull it out of the list, do the combining, and
// return it.
//
BYlist *
BYlist::trymerge(const BBox *BB, Blist **bret)
{
    *bret = 0;
    BYlist *y0 = this, *yp = 0, *yn;
    for (BYlist *y = y0; y; yp = y, y = yn) {
        yn = y->next;
        if (y->yl > BB->top)
            continue;
        if (y->yu < BB->bottom)
            break;
        Blist *bp = 0, *bn;
        for (Blist *b = y->blist; b; bp = b, b = bn) {
            bn = b->next;
            if (b->BB.left > BB->right)
                break;
            if (b->BB.right < BB->left)
                continue;
            if (b->BB.bottom > BB->top)
                continue;

            if (b->BB.left == BB->left && b->BB.right == BB->right) {
                if (b->BB.bottom == BB->top)
                    b->BB.bottom = BB->bottom;
                else if (b->BB.top == BB->bottom)
                    b->BB.top = BB->top;
                else
                    continue;
            }
            else if (b->BB.bottom == BB->bottom && b->BB.top == BB->top) {
                if (b->BB.right == BB->left)
                    b->BB.right = BB->right;
                else if (b->BB.left == BB->right)
                    b->BB.left = BB->left;
                else
                    continue;
            }
            else
                continue;

            if (!bp) {
                y->blist = bn;
                if (!y->blist) {
                    if (!yp)
                        y0 = yn;
                    else
                        yp->next = yn;
                    delete y;
                }
            }
            else
                bp->next = bn;
            b->next = 0;
            *bret = b;
            return (y0);
        }
    }
    return (y0);
}


namespace {
    inline bool
    match_X(BBox &BL, BBox &BR)
    {
        if (BL.bottom == BR.bottom && BL.right == BR.left) {
            BL.right = BR.right;
            return (true);
        }
        return (false);
    }
}


// Private support function for merge().
// For each row, join adjacent elements.
//
bool
BYlist::merge_rows()
{
    bool change = false;
    for (BYlist *y = this; y; y = y->next) {
        for (Blist *bl1 = y->blist; bl1; bl1 = bl1->next) {
            Blist *bn;
            for (Blist *bl2 = bl1->next; bl2; bl2 = bn) {
                bn = bl2->next;
                if (match_X(bl1->BB, bl2->BB)) {
                    bl1->next = bn;
                    delete bl2;
                    change = true;
                    continue;
                }
                break;
            }
        }
    }
    return (change);
}


namespace {
    inline bool
    match_Y(BBox &BT, BBox &BB)
    {
        if (BT.bottom == BB.top &&
                BT.left == BB.left && BT.right == BB.right) {
            BT.bottom = BB.bottom;
            return (true);
        }
        return (false);
    }
}


// Private support function for merge().
// Join elements in the vertical sense.
//
bool
BYlist::merge_cols()
{
    bool change = true;
    for (BYlist *y = this; y; y = y->next) {
        for (Blist *bl1 = y->blist; bl1; bl1 = bl1->next) {
            BYlist *yp = y, *yn;
            for (BYlist *yy = y->next; yy; yy = yn) {
                yn = yy->next;
                if (yy->yu > bl1->BB.bottom) {
                    yp = yy;
                    continue;
                }
                if (yy->yu < bl1->BB.bottom)
                    break;
                Blist *bp = 0, *bn;
                for (Blist *bl2 = yy->blist; bl2; bl2 = bn) {
                    bn = bl2->next;
                    if (bl2->BB.left < bl1->BB.left) {
                        bp = bl2;
                        continue;
                    }
                    if (bl2->BB.left > bl1->BB.left)
                        break;
                    if (match_Y(bl1->BB, bl2->BB)) {
                        if (y->yl > bl1->BB.bottom)
                            y->yl = bl1->BB.bottom;
                        if (!bp)
                            yy->blist = bn;
                        else
                            bp->next = bn;
                        change = true;
                        delete bl2;
                    }
                    break;
                }
                if (!yy->blist) {
                    yp->next = yn;
                    delete yy;
                    continue;
                }
                yp = yy;
            }
        }
    }
    return (change);
}


// Private support function for merge().
// Clip the zoids against each other so that no two overlap.
//
BYlist *
BYlist::clip()
{
    Blist *b0 = 0, *be = 0, *bl;
    BYlist *yl0 = first(&bl);
    if (bl) {
        for (;;) {
            if (!yl0) {
                if (!b0)
                    b0 = be = bl;
                else
                    be->next = bl;
                break;
            }
            Blist *bret = bl->BB.clip_out(yl0);
            delete bl;
            if (bret) {
                if (!b0)
                    b0 = be = bret;
                else
                    be->next = bret;
                while (be->next)
                    be = be->next;
            }
            yl0 = yl0->first(&bl);
        }
    }
    if (b0)
        yl0 = new BYlist(b0);
    return (yl0);
}


// Private support function for merge(), and others.
// Remove and return the top left element.  Empty rows are deleted.
//
BYlist *
BYlist::first(Blist **bp)
{
    *bp = 0;
    BYlist *yl0 = this, *yn;
    for (BYlist *y = yl0; y; y = yn) {
        yn = y->next;
        if (y->blist) {
            *bp = y->blist;
            y->blist = y->blist->next;
            (*bp)->next = 0;
        }
        if (!y->blist) {
            yl0 = yn;
            delete y;
        }
        if (*bp)
            break;
    }
    return (yl0);
}

