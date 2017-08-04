
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

#include "cd.h"
#include "cd_types.h"
#include "geo_ylist.h"
#include "geo_zgroup.h"
#include "geo_zdb.h"


// Define to implement Zlist::merge for testing.
//#define MERGE_TEST

// Define for general noisy debugging.
//#define DEBUG

// This database is not currently used.  Performance gains may be seen
// for extremely large collections, but smaller collections suffer. 
// Here's plug-in implementation of Zlist::merge that can be used for
// evaluation.

#ifdef NOTDEF
// Process a zlist so that no two zoids overlap, and any that share
// two adjacent corners are merged.  Watch for interrupts if the
// argument is true.  On exception:  'this' is freed.
//
Zlist *
Zlist::merge(bool ignore_intr) throw (XIrt)
{
    (void)ignore_intr;
    Zlist *zt = this;
    if (!zt || !zt->next)
        return (zt);

    Zdb *zdb = new Zdb(zt);
    zdb->mutualClipAndMerge();
    Zlist *z0 = zdb->toZlist();
    return (z0);
}
#endif


#ifdef MERGE_TEST
#include "tvals.h"

namespace {
    double zz_t1, zz_t2, zz_t3, zz_t4, zz_t5, zz_t6;
    double tot_len;
    int call_cnt;
}

Zlist *
Zlist::merge(bool ignore_intr) throw (XIrt)
{
    if (!this || !next)
        return (this);

    Zlist *zlbk = copy();
    tot_len += length();
    call_cnt++;

    Tvals tv;
    tv.start();
    Ylist *yl = new Ylist(this);
    try {
        yl = yl->merge(ignore_intr);
    }
    catch (XIrt) {
        if (ignore_intr)
            // shouldn't happen
            return (0);
        throw;
    }
    Zlist *z1 = yl->to_zlist();
    tv.stop();
    zz_t1 += tv.realTime();

    tv.start();
    Tvals tv1;
    tv1.start();
    Zdb *zdb = new Zdb(zlbk);
    BBox tBB(zdb->treeBB());
    tv1.stop();
    zz_t3 += tv1.realTime();
    tv1.start();
    zdb->mutualClipAndMerge();
    tv1.stop();
    zz_t4 += tv1.realTime();

    tv1.start();
    Zlist *z0 = zdb->toZlist();
    tv1.stop();
    zz_t5 += tv1.realTime();
//    return (z0);
    tv.stop();
    zz_t2 += tv.realTime();

    printf("calls %u avg_len %u ylist %g zdb %g\n", call_cnt,
        (unsigned int)(tot_len/call_cnt), zz_t1, zz_t2);
    printf("mkzdb %g clipmrg %g dstr %g lens y/z %d,%d\n", zz_t3, zz_t4, zz_t5,
        z1->length(), z0->length());

    while (z0 && z1) {
        if (z0->Z == z1->Z) {
            Zlist *z0x = z0;
            Zlist *z1x = z1;
            z0 = z0->next;
            z1 = z1->next;
            delete z0x;
            delete z1x;
            continue;
        }
        z0->Z.print();
        z1->Z.print();
        break;
    }
    printf("%lx %lx %d %d\n", z0, z1, z0->length(), z1->length());

    // Zlist::zl_andnot2(&z0, &z1);
    if (!z0 && !z1)
        return (0);

/*
    printf("(%.3f,%.3f -- %.3f,%.3f)\n",
        MICRONS(tBB.left), MICRONS(tBB.bottom),
        MICRONS(tBB.right), MICRONS(tBB.top));

    printf("<<\n");
    for (Zlist *z = z0; z; z = z->next)
        z->Z.print();
    printf(">>\n");
    for (Zlist *z = z1; z; z = z->next)
        z->Z.print();

*/
//    Zlist::zl_andnot2(&z0, &z1);

    GEO()->ifSaveZlist(z0, "z0");
    GEO()->ifSaveZlist(z1, "z1");
    GEO()->ifShowAndPause("foo? ");
    GEO()->ifClearLayer("z0");
    GEO()->ifClearLayer("z1");
//    GEO()->ifClearLayer("z01");
//    GEO()->ifClearLayer("z11");

    return (0);
}
#endif


#ifdef DEBUG
namespace {
    bool checkYorder(const Zdb *zdb, const char *str)
    {
        bool ret = true;
        ZdbGenY gen(zdb, -CDinfinity, CDinfinity);
        const GEOblock *row;
        int last = CDinfinity;
        while ((row = gen.next()) != 0) {
            if (row->bBB().top >= last) {
                printf("BAD Y ORDER! %s %d > %d\n", str, row->bBB().top, last);
                ret = false;
            }
            last = row->bBB().top;
        }
        return (ret);
    }

    bool checkXorder(GEOblock *b, const char *str)
    {
        bool ret = true;
        RowGenZ zgen(b, -CDinfinity, CDinfinity, CDinfinity);
        const Zoid *Z, *Zl = 0;
        while ((Z = zgen.next()) != 0) {
            if (Zl && Zl->zcmp(Z) > 0) {
                printf("BAD X ORDER! %s\n", str);
                printf("Z= ");
                Z->print();
                printf("Zlast= ");
                Zl->print();
                ret = false;
            }
            Zl = Z;
        }
        return (ret);
    }

    void check_blk(const GEOblock *b, const GEOblock *p)
    {
        if (!b->parent() && p)
            printf("unparented node\n");
        if (b->parent() != p)
            printf("wrongly parented node\n");
        if (!b->list())
            printf("empty node\n");
        if (b->type() != GBLKzlist) {
            for (const GEOblock *b1 = (GEOblock*)b->list(); b1; b1->next()) {
                check_blk(b1, b);
            }
        }
    }

    void check_zdb(Zdb *zdb)
    {
        const GEOblock *b = zdb->tree();
        if (!b) {
            printf("The tree root is null.\n");
            return;
        }
        if (!b->list()) {
            printf("The tree root list is null.\n");
            return;
        }
        check_blk(b, 0);
    }
}
#endif


Zdb *GEOblock::b_context = 0;

// Static function.
// Create a list/tree of the base type provided, from the (assumed
// sorted) list given.  This will build the tree upwards, so that no
// individual block will contain more than GBLK_SIZE elements.
//
// This consumes the list.
//
GEOblock *
GEOblock::build(GBLKtype tp, void *list)
{
    GEOblock *g0 = 0, *ge = 0;

    if (tp == GBLKgblk) {
        GEOblock *glist = (GEOblock*)list;
        if (!glist)
            return (0);
        int nblks = 0;
        while (glist) {
            GEOblock *gnext = 0;
            int cnt = 0;
            for (GEOblock *g = glist; g; g = g->next()) {
                cnt++;
                if (cnt == GBLK_SIZE) {
                    gnext = g->next();
                    g->set_next(0);
                    break;
                }
            }
            GEOblock *nblk = new GEOblock(GBLKgblk, glist);
            if (!g0)
                g0 = ge = nblk;
            else {
                ge->set_next(nblk);
                ge = nblk;
            }
            nblks++;
            glist = gnext;
        }
        if (nblks <= GBLK_SIZE)
            return (g0);
        return (build(GBLKgblk, g0));
    }
    if (tp == GBLKzlist) {
        Zlist *zlist = (Zlist*)list;
        if (!zlist)
            return (0);
        int nblks = 0;
        while (zlist) {
            Zlist *znext = 0;
            int cnt = 0;
            for (Zlist *z = zlist; z; z = z->next) {
                cnt++;
                if (cnt == GBLK_SIZE) {
                    znext = z->next;
                    z->next = 0;
                    break;
                }
            }
            GEOblock *nblk = new GEOblock(GBLKzlist, zlist);
            if (!g0)
                g0 = ge = nblk;
            else {
                ge->set_next(nblk);
                ge = nblk;
            }
            nblks++;
            zlist = znext;
        }
        if (nblks <= GBLK_SIZE)
            return (g0);
        return (build(GBLKgblk, g0));
    }
    if (tp == GBLKrow) {
        GEOblock *rowlist = (GEOblock*)list;
        if (!rowlist)
            return (0);
        int nblks = 0;
        while (rowlist) {
            GEOblock *rownext = 0;
            int cnt = 0;
            for (GEOblock *row = rowlist; row; row = row->next()) {
                cnt++;
                if (cnt == GBLK_SIZE) {
                    rownext = row->next();
                    row->set_next(0);
                    break;
                }
            }
            GEOblock *nblk = new GEOblock(GBLKgblk, rowlist);
            if (!g0)
                g0 = ge = nblk;
            else {
                ge->set_next(nblk);
                ge = nblk;
            }
            nblks++;
            rowlist = rownext;
        }
        if (nblks <= GBLK_SIZE)
            return (g0);
        return (build(GBLKgblk, g0));
    }
    return (0);
}


// Static function.
// Build a row head list from the given Zlist, which is consumed.
//
GEOblock *
GEOblock::build(Zlist *z0, bool nosort)
{
    if (!nosort)
        z0 = Zlist::sort(z0, 1);  // Same order as Ylist in Gen3.

    GEOblock *row0 = 0, *rowe = 0;
    while (z0) {
        int yu = z0->Z.yu;
        Zlist *zn = z0;
        while (zn->next && zn->next->Z.yu == yu)
            zn = zn->next;
        Zlist *ze = zn;
        zn = ze->next;
        ze->next = 0;
        
        GEOblock *rownew = new GEOblock(z0);
        if (!row0)
            row0 = rowe = rownew;
        else {
            rowe->set_next(rownew);
            rowe = rownew;
        }
        z0 = zn;
    }
    return (row0);
}


// Constructor.
//
GEOblock::GEOblock(GBLKtype tp, void *lst)
{
    b_next = 0;
    b_parent = 0;
    b_list = lst;
    b_type = tp;
    reparent();
    computeBB(true);  // Don't purge empty lists here.
    if (b_context)
        b_context->onCreate(this);
}


// Constructor, for a row head.  We assume that the passed Zlist has
// been sorted and all Z.yu values are the same.  The list will be
// consumed.  We can also create an empty list head by passing a null
// Zlist, and the y value in the optional second arument.
//
GEOblock::GEOblock(Zlist *zl, int yval)
{
    b_type = GBLKrow;
    b_next = 0;
    b_parent = 0;
    if (zl) {
        b_list = build(GBLKzlist, zl);
        reparent();
        computeBB();
    }
    else {
        b_list = 0;
        b_BB.left = CDinfinity;
        b_BB.bottom = yval;
        b_BB.right = -CDinfinity;
        b_BB.top = yval;
    }
    if (b_context)
        b_context->onCreate(this);
}


// Destructor.
//
GEOblock::~GEOblock()
{
    if (b_list) {
        if (b_type == GBLKzlist)
            Zlist::destroy(((Zlist*)b_list));
        else
            destroy((GEOblock*)b_list);
    }
    if (b_context)
        b_context->onDestroy(this);
}


// This applies to row list heads (type GBLKrow) only.  Remove and
// return the first zoid from the list, taking care of fixing up BBs
// and removing empty blocks.  The caller should check if this is now
// empty.
//
// 'This' may be freed by calling this function!
//
Zlist *
GEOblock::removeFirstZoid()
{
    if (b_type != GBLKrow) {
#ifdef DEBUG
        printf("removeFirstZoid: this pointer has wrong type.\n");
#endif
        return (0);
    }
    GEOblock *b = (GEOblock*)b_list;
    if (!b)
        return (0);
    while (b->type() == GBLKgblk)
        b = (GEOblock*)b->list();

    Zlist *z = (Zlist*)b->b_list;
    b->b_list = z->next;
    z->next = 0;
    b->propagateBBrm(z->Z);
    return (z);
}


// This applies to zoid list blocks (type GBLKzlist).  Search this
// block only, and remove the element pointed to by Z.  This takes
// care of BB updates and removing empty blocks.
//
Zlist *
GEOblock::removeZoid(const Zoid *Z)
{
    if (b_type != GBLKzlist) {
#ifdef DEBUG
        printf("removeZoid: this pointer has wrong type.\n");
#endif
        return (0);
    }
    Zlist *zp = 0;
    Zlist *zret = 0;
    for (Zlist *z = (Zlist*)b_list; z; z = z->next) {
        if (Z == &z->Z) {
            if (zp)
                zp->next = z->next;
            else
                b_list = z->next;
            z->next = 0;
            zret = z;
            break;
        }
        zp = z;
    }
    if (!zret)
        return (0);

#ifdef xDEBUG
    if (!b_list) {
        printf("removeZoid: now empty %lx.\n", (unsigned long)this);
        if (!b_parent)
            printf("... which has no parent.\n");
    }
#endif
    propagateBBrm(zret->Z);
    return (zret);
}


// This applies to row head blocks.  Link zl into the proper location
// within the row (it is consumed).  The zl contains a single element
// (zl->next = 0).
//
// Presently, blocks are expanded without limit.
//
bool
GEOblock::insertZoid(Zlist *zl)
{
    zl->next = 0;  // should already be true.
    if (b_type != GBLKrow) {
#ifdef DEBUG
        printf("insertZoid: incorrect argument block type.\n");
#endif
        return (false);
    }
    if (b_BB.top != zl->Z.yu) {
#ifdef DEBUG
        printf("insertZoid: incorrect argument y value.\n");
#endif
        return (false);
    }
    if (!b_list) {
        // The block is empty, must have just been created in getRow.
        b_list = new GEOblock(GBLKzlist, zl);
        reparent();
        propagateBBadd(zl->Z);
        return (true);
    }

    const Zoid &Z = zl->Z;
    int minleft = Z.minleft();
    GEOblock *b = (GEOblock*)b_list;
    while (b) {
        if (b->b_BB.left > minleft) {
            // End of search.
            break;
        }
        if (b->b_BB.right <= minleft) {
            while (!b->next()) {
                b = b->parent();
                if (b->type() == GBLKrow)
                    break;
            }
            if (b->type() == GBLKrow) {
                b = 0;
                break;
            }
            b = b->next();
            continue;
        }
        // Descend to the leaf.
        if (b->type() == GBLKgblk) {
            b = (GEOblock*)b->list();
            continue;
        }
        break;
    }
    if (!b) {
        // Add to end.
        b = (GEOblock*)b_list;
        while (b->next())
            b = b->next();
        while (b->type() == GBLKgblk) {
            b = (GEOblock*)b->list();
            while (b->next())
                b = b->next();
        }
        Zlist *z = (Zlist*)b->b_list;
        while (z->next)
            z = z->next;
        z->next = zl;
        b->propagateBBadd(zl->Z);
#ifdef DEBUG
        checkXorder(this, "insertZoid_1");
#endif
        return (true);
    }
    while (b->type() == GBLKgblk)
        b = (GEOblock*)b->list();

    // We've found, in b, the first leaf block that spans the
    // insertion point.

    GEOblock *bprv = 0;
    while (b) {
        Zlist *zp = 0;
        for (Zlist *z = (Zlist*)b->b_list; z; z = z->next) {
            int cmp = Z.zcmp(&z->Z);
            if (cmp < 0) {
                zl->next = z;
                if (zp)
                    zp->next = zl;
                else
                    b->b_list = zl;
                b->propagateBBadd(zl->Z);
#ifdef DEBUG
                checkXorder(this, "insertZoid_2");
#endif
                return (true);
            }
            if (cmp == 0) {
                // Exact duplicate exists.
                delete zl;
                return (true);
            }
            zp = z;
        }
        bprv = b;
        b = b->nextBlock();
    }

    // The last block in the line contains the insertion point, and all
    // existing zoids are to the left of this point.

    // Add zoid to end.
    Zlist *z = (Zlist*)bprv->list();
    while (z->next)
        z = z->next;
    z->next = zl;
    bprv->propagateBBadd(zl->Z);
#ifdef DEBUG
    checkXorder(this, "insertZoid_3");
#endif

    return (true);
}


// Return the first Zlist element with xul exactly as given.  The
// 'this' block must be a row head.  This is used in Zdb::mergeCols.
//
Zlist *
GEOblock::findZoid(int xul, GEOblock **bp)
{
    if (b_type != GBLKrow)
        return (0);
    GEOblock *b = (GEOblock*)b_list;
    while (b) {
        if (b->b_BB.left > xul)
            return (0);
        if (b->b_BB.right <= xul) {
            b = b->next();
            continue;
        }
        // Descend to the leaf.
        if (b->type() == GBLKgblk) {
            b = (GEOblock*)b->list();
            continue;
        }
        break;
    }
    // Here, b is a leaf block, need to find the first good zoid.

    while (b) {
        for (Zlist *z = (Zlist*)b->list(); z; z = z->next) {
            if (z->Z.minleft() > xul) {
                // None of the zoids here are any good, and we know
                // that no other block contains suitable zoids.
                return (0);
            }
            if (z->Z.xul == xul) {
                if (bp)
                    *bp = b;
                return (z);
            }
        }
        // We've traversed this block without success, test the next
        // block.

        b = b->nextBlock();
    }
    return (0);
}
// End of GEOblock functions.


// Contructor.  The passed Zlist is consumed.
//
Zdb::Zdb(Zlist *z0)
{
    db_rowtab = new SymTab(false, false);;
    GEOblock::setContext(this);
    GEOblock *rowlist = GEOblock::build(z0);
    db_tree = new GEOblock(GBLKgblk, GEOblock::build(GBLKrow, rowlist));
    GEOblock::setContext(0);
}


Zdb::~Zdb()
{
    GEOblock::destroy(db_tree);
    delete db_rowtab;
}


// Remove and return the sorted zoids from the Zdb, which is freed.
//
Zlist *
Zdb::toZlist()
{
    Zlist *z0 = 0, *ze = 0;
    GEOblock *row = firstRow();
    while (row) {
        GEOblock *b = (GEOblock*)row->list();
        while (b->type() == GBLKgblk)
            b = (GEOblock*)b->list();
        while (b) {
            Zlist *zt = (Zlist*)b->list();
            b->set_list(0);
            if (!z0)
                z0 = ze = zt;
            else {
                while (ze->next)
                    ze = ze->next;
                ze->next = zt;
            }
            b = b->nextBlock();
        }
        row = row->nextBlock();
    }
    delete this;
    return (z0);
}


// Mutually clip and merge the trapezoids.  The result is a mimimal
// count of non-overlapping trapezoids.  This is the basic
// conditioning operation.
//
void
Zdb::mutualClipAndMerge()
{
    GEOblock::setContext(this);
    Zlist *z0 = removeFirstZoid();
    if (!z0)
        return;
    clipAroundZoid(z0->Z);
    Zlist *ze = z0, *zt;
    while ((zt = removeFirstZoid()) != 0) {
        ze->next = zt;
        ze = zt;
        clipAroundZoid(zt->Z);
    }
    GEOblock::destroy(db_tree);
    delete db_rowtab;
    db_rowtab = new SymTab(false, false);;
    GEOblock *rowlist = GEOblock::build(z0, true);
    db_tree = new GEOblock(GBLKgblk, GEOblock::build(GBLKrow, rowlist));

    GEOblock *row = firstRow();
    while (row) {
        rowMergeRow(row);
        row = row->nextBlock();
    }
    row = firstRow();
    while (row) {
        for (;;) {
            if (!rowMergeCols(row))
                break;
            if (!rowMergeRow(row))
                break;
            // We did a row merge in this row, back up to the first
            // row that touches this row, to catch any new column
            // merges.
            row = getFirstRowTouching(row);
        }
        row = row->nextBlock();
    }
    GEOblock::setContext(0);
}


Zgroup *
Zdb::group(int max_in_grp)
{
    struct gtmp_t { gtmp_t *next; Zlist *lst[256]; };
    gtmp_t *g0 = 0, *ge = 0;

    int gcnt = 0;
    for (;;) {
        Zlist *ze = removeFirstZoid();
        if (!ze)
            break;

        int gc = gcnt & 0xff;
        if (!gc) {
            if (!g0)
                g0 = ge = new gtmp_t;
            else {
                ge->next = new gtmp_t;
                ge = ge->next;
            }
            ge->next = 0;
            ge->lst[0] = ze;
        }
        else
            ge->lst[gc] = ze;

        gcnt++;

        int cnt = 1;
        for (Zlist *z = ze; z; z = z->next) {
            ze->next = removeTouching(z->Z);
            while (ze->next) {
                ze = ze->next;
                cnt++;
            }
            if (max_in_grp > 0 && cnt > max_in_grp)
                break;
        }
    }
    Zgroup *g = new Zgroup;
    g->num = gcnt;
    g->list = new Zlist*[gcnt+1];

    Zlist **p = g->list;
    for (gtmp_t *gt = g0; gt; gt = ge) {
        ge = gt->next;
        int n = mmMin(gcnt, 256);
        for (int i = 0; i < n; i++)
            *p++ = gt->lst[i];
        gcnt -= n;
        delete gt;
    }
    g->list[g->num] = 0;
    return (g);
}


//
// Remaining methods are private.
//

// Return the row list head for yval.  If create is true, insert a new
// head if a head for yval does not exist.  If a new row head is
// created, the caller should immediately add a zoid - the database is
// unstable with empty blocks.
//
GEOblock *
Zdb::getRow(int yval, bool create)
{
    if (db_rowtab) {
        GEOblock *b = (GEOblock*)SymTab::get(db_rowtab, (unsigned long)yval);
        if (b != (GEOblock*)ST_NIL)
            return (b);
        if (!create)
            return (0);
    }

    GEOblock *b = db_tree;
    if (!b->list()) {
        // The entire tree is empty.  We know that the top node never
        // has the next or parent pointer set.

        if (!create)
            return (0);

        GEOblock *row = new GEOblock((Zlist*)0, yval);
        b->set_list(row);
        row->set_parent(b);
        return (row);
    }
    while (b) {
        if (b->bBB().top < yval) {
            if (!create)
                return (0);
            break;
        }
        if (b->bBB().bottom >= yval) {
            while (!b->next()) {
                b = b->parent();
                if (!b)
                    break;
            }
            if (!b)
                break;
            b = b->next();
            continue;
        }
        // Descend to the leaf.
        if (b->type() == GBLKgblk) {
            b = (GEOblock*)b->list();
            continue;
        }
        if (b->bBB().top > yval) {
            while (!b->next()) {
                b = b->parent();
                if (!b)
                    break;
            }
            if (!b)
                break;
            b = b->next();
            continue;
        }
        break;
    }
    if (b) {
        while (b->type() == GBLKgblk)
            b = (GEOblock*)b->list();
        if (b->bBB().top == yval)
            return (b);
        if (!create)
            return (0);

        GEOblock *p = b->parent();
        GEOblock *gp = 0;
        for (GEOblock *g = (GEOblock*)p->list(); g; g = g->next()) {
            if (g == b) {
                GEOblock *bnew = new GEOblock((Zlist*)0, yval);
                bnew->set_parent(p);
                bnew->set_next(g);
                if (gp)
                    gp->set_next(bnew);
                else
                    p->set_list(bnew);
#ifdef DEBUG
                checkYorder(this, "getRow_1");
#endif
                return (bnew);
            }
            gp = g;
        }
    }
    if (!create)
        return (0);

    // Add to end.
    b = db_tree;
    while (b->type() == GBLKgblk) {
        b = (GEOblock*)b->list();
        while (b->next())
            b = b->next();
    }
    GEOblock *bnew = new GEOblock((Zlist*)0, yval);
    bnew->set_parent(b->parent());
    b->set_next(bnew);
#ifdef DEBUG
    checkYorder(this, "getRow_2");
#endif
    return (bnew);
}


// Return the first row that touches row.  If none, return the
// argument.
//
GEOblock *
Zdb::getFirstRowTouching(GEOblock *row)
{
    int yval = row->bBB().top;
    GEOblock *b = db_tree;
    while (b) {
        if (b->bBB().top < yval)
            return (row);
        if (b->bBB().bottom > yval) {
            while (!b->next()) {
                b = b->parent();
                if (!b)
                    return (0);
            }
            b = b->next();
            continue;
        }
        // Descend to the leaf.
        if (b->type() == GBLKgblk) {
            b = (GEOblock*)b->list();
            continue;
        }
        return (b);
    }
    return (row);
}


// Insert the Zlist into the database, the Zlist is consumed.
//
void
Zdb::insertZlist(Zlist *zl)
{
    GEOblock *row = 0;
    while (zl) {
        Zlist *z = zl;
        zl = zl->next;
        z->next = 0;
        if (!row || row->bBB().top != z->Z.yu)
            row = getRow(z->Z.yu, true);
        row->insertZoid(z);
    }
}


// Clip all zoids in the database around Z.
//
void
Zdb::clipAroundZoid(const Zoid &Z)
{
    BBox BB(Z.minleft(), Z.yl, Z.maxright(), Z.yu);

    ZdbGenY rowgen(this, BB.bottom, BB.top);
    const GEOblock *row;
    while ((row = rowgen.next()) != 0) {
        Zlist *z0 = 0, *ze = 0;
        RowGenZ zgen(row, BB.left, BB.right, BB.top);
        const Zoid *Zg;
        GEOblock *blk;
        while ((Zg = zgen.next(&blk)) != 0) {
            bool no_ovl;
            Zlist *zt = Zg->clip_out(&Z, &no_ovl);
            if (no_ovl)
                continue;
            if (zt) {
                if (ze) {
                    while (ze->next)
                        ze = ze->next;
                    ze->next = zt;
                }
                else
                    z0 = ze = zt;
            }
            Zlist *zx = blk->removeZoid(Zg);
#ifdef DEBUG
            if (!zx)
                printf("clipAroundZoid: removeZoid failed.\n");
#endif
            delete zx;
        }
        insertZlist(z0);
    }
}


// Merge trapezoids along the row when possible.  A count of the
// number of merges is returned.
//
int
Zdb::rowMergeRow(GEOblock *row)
{
    if (row->type() != GBLKrow) {
#ifdef DEBUG
        printf("rowMergeRow: wrong type.\n");
#endif
        return (0);
    }
    int mcnt = 0;
    GEOblock *b = (GEOblock*)row->list();
    while (b->type() == GBLKgblk)
        b = (GEOblock*)b->list();
    RowGenAZ gen1(b);
    for (;;) {
        GEOblock *b1;
        Zlist *zl1 = gen1.next(&b1);
        if (!zl1)
            break;
        RowGenAZ gen2(gen1);
        for (;;) {
            GEOblock *b2;
            Zlist *zl2 = gen2.next(&b2);
            if (!zl2)
                break;
            if (zl2->Z.minleft() > zl1->Z.maxright())
                break;

            if (zl1->Z.join_right(zl2->Z)) {
                mcnt++;
                if (b2->removeZoid(&zl2->Z))
                    delete zl2;
#ifdef DEBUG
                else
                    printf("rowMergeRows: removeZoid failed.\n");
#endif
                if (b1 != b2)
                    b1->propagateBBadd(zl1->Z);
                // The gen2 is corrupt after zl2 removal, reset it.
                gen2 = gen1;
            }
        }
    }
    return (mcnt);
}


// For each trapezoid in the row, attempt to merge in trapezoids from
// lower rows.  A count of the number of merges is returned.
//
int
Zdb::rowMergeCols(GEOblock *row)
{
    if (row->type() != GBLKrow) {
#ifdef DEBUG
        printf("rowMergeCols: wrong type.\n");
#endif
        return (0);
    }
    int mcnt = 0;
    GEOblock *b = (GEOblock*)row->list();
    while (b->type() == GBLKgblk)
        b = (GEOblock*)b->list();
    RowGenAZ gen(b);
    GEOblock *b1;
    Zlist *zl1 = gen.next(&b1);
    while (zl1) {
        // Find the row head for the bottom, if any.
        GEOblock *zy2 = getRow(zl1->Z.yl, false);
        if (!zy2) {
            zl1 = gen.next(&b1);
            continue;
        }

        // Look at the zoids with matching left coordinate, link
        // to one if possible.
        int tmcnt = mcnt;
        GEOblock *b2;

        Zlist *zl2 = zy2->findZoid(zl1->Z.xll, &b2);
        while (zl2) {
            if (zl2->Z.minleft() > zl1->Z.xll)
                break;
            if (zl1->Z.join_below(zl2->Z)) {
                mcnt++;
                if (b2->removeZoid(&zl2->Z))
                    delete zl2;
#ifdef DEBUG
                else
                    printf("rowMergeCols: removeZoid failed.\n");
#endif
                b1->propagateBBadd(zl1->Z);
                break;
            }
            zl2 = b2->nextZoid(zl2, &b2);
        }

        // If something was linked, loop again with the same
        // (expanded) zoid, otherwise go to the next one.
        if (tmcnt == mcnt)
            zl1 = gen.next(&b1);
    }
    return (mcnt);
}


// Remove and return the trapezoids that touch or overlap Z.
//
Zlist *
Zdb::removeTouching(const Zoid &Z)
{
    Zlist *z0 = 0;
    BBox BB(Z.minleft(), Z.yl, Z.maxright(), Z.yu);
    BB.bloat(1);
    ZdbGenZ gen(this, BB);
    GEOblock *b = 0;
    const Zoid *Zg;
    while ((Zg = gen.next(&b)) != 0) {
        if (Z.touching(Zg)) {
            Zlist *zt = b->removeZoid(Zg);
            zt->next = z0;
            z0 = zt;
        }
    }
    return (z0);
}
// End of Zdb functions.

