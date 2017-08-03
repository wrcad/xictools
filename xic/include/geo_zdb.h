
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

#ifndef GEO_ZDB_H
#define GEO_ZDB_H

#include "geo_memmgr_cfg.h"
#include "cd_symtab.h"


struct Zlist;
struct Zdb;

#define GBLK_SIZE 8

// GEOblock:  holds a list of up to GBLK_SIZE objects, of particular
// types.  Possible types are Zlist, and flavors of GEOblock.  This
// forms a tree structure, with Zlist leaves.

// Block types.
//
// GBLKgblk
//   The block contains a list of blocks, forming an upper node
//   of an ordered tree.
//
// GBLKzlist
//   The block contains a list of trapezoids, all with the same
//   upper coordinate.  The zoids are sorted in ascending order
//   in accord with Ylist sorting.
//
// GBLKrow
//   These contain the trapezoid trees for a row.  The trapezoid
//   tree is parented by this, which should be considered the
//   root for row-only searches.
//
enum GBLKtype { GBLKgblk, GBLKzlist, GBLKrow };

struct GEOblock
{
#ifdef GEO_USE_MANAGER
    void *operator new(size_t);
    void operator delete(void*, size_t);
#endif
    static GEOblock *build(GBLKtype, void*);
    static GEOblock *build(Zlist*, bool = false);

    GEOblock(GBLKtype, void*);
    GEOblock(Zlist*, int = 0);
    ~GEOblock();

    // Destroy the entire list.
    //
    static void destroy(GEOblock *g)
        {
            while (g) {
                GEOblock *gx = g;
                g = g->next();
                delete gx;
            }
        }

    // Return the leaf block that logically follows this.  This works
    // for GBLKzlist and GBLKrow types.
    //
    GEOblock *nextBlock()
        {
            GEOblock *b = this;
            while (!b->next()) {
                b = b->parent();
                if (!b || b->type() == GBLKrow)
                    return (0);
            }
            b = b->next();
            while (b->type() == GBLKgblk)
                b = (GEOblock*)b->b_list;
            return (b);
        }

    // This applies to type GBLKzlist blocks.  The 'this' block
    // contains z.  The function returns the next ordered Zlist
    // element and its containing block.
    //
    Zlist *nextZoid(Zlist *z, GEOblock **bp)
        {
            if (b_type != GBLKzlist)
                return (0);
            if (z->next) {
                *bp = this;
                return (z->next);
            }
            GEOblock *b = nextBlock();
            if (b) {
                *bp = b;
                return ((Zlist*)b->list());
            }
            return (0);
        }

    void propagateBBadd(const BBox *BB)
        {
            if (b_BB.addc(BB) && b_parent)
                b_parent->propagateBBadd(&b_BB);
        }

    void propagateBBadd(const Zoid &Z)
        {
            BBox BB(Z.minleft(), Z.yl, Z.maxright(), Z.yu);
            if (b_BB.addc(&BB) && b_parent)
                b_parent->propagateBBadd(&b_BB);
        }

    void propagateBBrm(const BBox *BB)
        {
            if (b_list && safeBBrm(BB))
                return;
            if (!b_list) {
                if (b_parent) {
                    BBox tBB(b_BB);
                    b_parent->unlink(this);
                    b_parent->propagateBBrm(&tBB);
                }
                return;
            }

            BBox tBB(b_BB);
            computeBB();
            if (b_parent && tBB != b_BB)
                b_parent->propagateBBrm(&tBB);
        }

    void propagateBBrm(const Zoid &Z)
        {
            BBox BB(Z.minleft(), Z.yl, Z.maxright(), Z.yu);
            propagateBBrm(&BB);
        }

    GEOblock *next()            const { return (b_next); }
    void set_next(GEOblock *n)  { b_next = n; }

    GEOblock *parent()          const { return (b_parent); }
    void set_parent(GEOblock *p) { b_parent = p; }

    const void *list()          const { return (b_list); }
    void set_list(void *l)      { b_list = l; }

    const BBox &bBB()           const { return (b_BB); }
    GBLKtype type()             const { return (b_type); }

    static void setContext(Zdb *zdb) { b_context = zdb; }

    Zlist *removeFirstZoid();
    Zlist *removeZoid(const Zoid*);
    bool insertZoid(Zlist*);
    Zlist *findZoid(int, GEOblock** = 0);

protected:
    // Set the parent pointer of the list elements.
    //
    void reparent()
        {
            if (b_type != GBLKzlist) {
                for (GEOblock *g = (GEOblock*)b_list; g; g = g->next())
                    g->set_parent(this);
            }
        }

    bool safeBBrm(const BBox *BB)
        {
            if (BB->left <= b_BB.left || BB->bottom <= b_BB.bottom ||
                    BB->right >= b_BB.right)
                return (false);
            if (b_type != GBLKrow && BB->top >= b_BB.top)
                return (false);
            return (true);
        }

    void unlink(const GEOblock *b)
        {
            if (b_type == GBLKzlist)
                return;
            GEOblock *gp = 0;
            for (GEOblock *g = (GEOblock*)b_list; g; g = g->next()) {
                if (g == b) {
                    if (gp)
                        gp->set_next(g->next());
                    else
                        b_list = g->next();
                    delete b;
                    return;
                }
                gp = g;
            }
        }

    // Compute the bounding box of elements in the list.  This will
    // also remove and free any empty non-Zlist list elements unless
    // nodel is set.
    //
    void computeBB(bool nodel = false)
        {
            if (b_type == GBLKrow && !b_list) {
                // Retain the row value!
                b_BB.left = CDinfinity;
                b_BB.right = -CDinfinity;
                b_BB.bottom = b_BB.top;
                return;
            }
            if (b_type == GBLKzlist) {
                Zlist *z = (Zlist*)b_list;
                b_BB.left = z->Z.minleft();
                b_BB.bottom = z->Z.yl;
                b_BB.right = z->Z.maxright();
                b_BB.top = z->Z.yu;
                for (z = z->next; z; z = z->next) {
                    int l = z->Z.minleft();
                    if (l < b_BB.left)
                        b_BB.left = l;
                    int r = z->Z.maxright();
                    if (r > b_BB.right)
                        b_BB.right = r;
                    if (z->Z.yl < b_BB.bottom)
                        b_BB.bottom = z->Z.yl;
                }
                return;
            }
            b_BB = CDnullBB;
            GEOblock *gp = 0, *gn;
            for (GEOblock *g = (GEOblock*)b_list; g; g = gn) {
                gn = g->b_next;
                if (!g->b_list) {
                    if (nodel)
                        continue;
                    if (gp)
                        gp->b_next = gn;
                    else
                        b_list = gn;
                    delete g;
                    continue;
                }
                gp = g;
                b_BB.add(&g->bBB());
            }
        }

    GEOblock *b_next;
    GEOblock *b_parent;
    void *b_list;
    BBox b_BB;
    GBLKtype b_type;
    
    static Zdb *b_context;
};


// A container for a GEOblock tree of row list heads.  This top-level
// structure implements a spatial database for trapezoids.
//
struct Zdb
{
    Zdb(Zlist*);
    ~Zdb();

    Zlist *toZlist();
    void mutualClipAndMerge();
    Zgroup *group(int = 0);

    void onCreate(GEOblock *b)
        {
            if (db_rowtab && b->type() == GBLKrow)
                db_rowtab->add((unsigned long)b->bBB().top, b, false);
        }

    void onDestroy(GEOblock *b)
        {
            if (db_rowtab && b->type() == GBLKrow)
                db_rowtab->remove((unsigned long)b->bBB().top);
        }

    const GEOblock *tree()      const { return (db_tree); }

    const BBox &treeBB()
        const { return (db_tree ? db_tree->bBB() : CDnullBB); }

private:
    GEOblock *getRow(int, bool);
    GEOblock *getFirstRowTouching(GEOblock*);
    void insertZlist(Zlist*);
    void clipAroundZoid(const Zoid&);
    int rowMergeRow(GEOblock*);
    int rowMergeCols(GEOblock*);
    Zlist *removeTouching(const Zoid&);

    GEOblock *firstRow()
        {
            GEOblock *b = db_tree;
            while (b->type() == GBLKgblk)
                b = (GEOblock*)b->list();
            return (b);
        }

    // Remove the first (top left) zoid from the database.
    //
    Zlist *removeFirstZoid()
        {
            GEOblock *b = db_tree;
            if (!b || !b->list())
                return (0);
            while (b->type() == GBLKgblk)
                b = (GEOblock*)b->list();
            return (b->removeFirstZoid());
        }

    GEOblock *db_tree;      // Tree root.
    SymTab *db_rowtab;      // Optional hash table for row heads.
};


// -----------------------------------------------------------------
// Generators.
// It is possible to remove any or all zoids up to and including the
// currently returned zoid (or row head) while the generator is
// active.
// -----------------------------------------------------------------

// RowGenAZ 
//
// This generator returns all zoids in order, with no spatial
// filtering.  Further, the logic is different from the other
// generators, in that there is no look-ahead to the next state, so it
// is safe to delete the next zoid or block (but it is bad news to
// delete the current zoid or block).  This is used in Zdb::mergeRows.
//
struct RowGenAZ
{
    // Constructor, initialize with a type GBLKzlist block pointer.
    //
    RowGenAZ(GEOblock *b)
        {
            g_init_block = 0;
            g_cur_block = 0;
            g_cur_zlist = 0;
            if (b && b->type() == GBLKzlist)
                g_init_block = b;
        }

    Zlist *next(GEOblock **bp = 0)
        {
            for (;;) {
                if (!g_cur_zlist) {
                    if (!g_cur_block) {
                        if (!g_init_block) {
                            if (bp)
                                *bp = 0;
                            return (0);
                        }
                        g_cur_block = g_init_block;
                        g_init_block = 0;
                    }
                    else
                        g_cur_block = g_cur_block->nextBlock();
                    if (!g_cur_block) {
                        if (bp)
                            *bp = 0;
                        return (0);
                    }
                    g_cur_zlist = (Zlist*)g_cur_block->list();
                    if (bp)
                        *bp = g_cur_block;
                    return (g_cur_zlist);
                }
                g_cur_zlist = g_cur_zlist->next;
                if (g_cur_zlist)
                    break;
            }
            if (bp)
                *bp = g_cur_block;
            return (g_cur_zlist);
        }

private:
    GEOblock *g_init_block;
    GEOblock *g_cur_block;
    Zlist *g_cur_zlist;
};


// RowGenZ 
//
// Generator for zoids whose bounding box overlaps the area given, for
// a single GEOblock row.  The returned zoids will overlap with nonzero
// area, not merely touch (bloat the search area by 1 to get the zoids
// that touch).
//
struct RowGenZ
{
    RowGenZ(const GEOblock *row, int xmin, int xmax, int yu)
        {
            g_next_block = 0;
            g_cur_block = 0;
            g_cur_zlist = 0;
            g_xmin = xmin;
            g_xmax = xmax;
            g_yu = yu;
            find_first(row);
        }
        
    // Restart the generator, same test area but (presumably)
    // different row.
    //
    void restart(const GEOblock *row)
        {
            g_next_block = 0;
            g_cur_block = 0;
            g_cur_zlist = 0;
            find_first(row);
        }

    // Return the next zoid, also its list container if bp is not
    // null.  Note that if zoids are removed after the call, the bp
    // return can point to freed memory if the block is emptied.
    //
    const Zoid *next(GEOblock **bp = 0)
        {
            if (!g_cur_zlist)
                return (0);
            const Zlist *zret = g_cur_zlist;
            if (bp)
                *bp = (GEOblock*)g_cur_block;
            find_next();
            return (&zret->Z);
        }

private:
    bool check_continue(const Zoid &Z)  { return (Z.maxright() <= g_xmin ||
                                                  Z.yl >= g_yu); }
    bool check_continue(const BBox &BB) { return (BB.right <= g_xmin ||
                                                  BB.bottom >= g_yu); }
    bool check_break(const Zoid &Z)     { return (Z.minleft() >= g_xmax); }
    bool check_break(const BBox &BB)    { return (BB.left >= g_xmax); }

    const GEOblock *find_leaf(const GEOblock *b)
        {
            while (b) {
                if (check_break(b->bBB()))
                    return (0);
                if (check_continue(b->bBB())) {
                    while (!b->next()) {
                        b = b->parent();
                        if (!b || b->type() == GBLKrow)
                            return (0);
                    }
                    b = b->next();
                    continue;
                }
                // Descend to the leaf.
                if (b->type() == GBLKgblk) {
                    b = (const GEOblock*)b->list();
                    continue;
                }
                return (b);
            }
            return (0);
        }

    const GEOblock *find_next_block(const GEOblock *b)
        {
            if (!b)
                return (0);
            while (!b->next()) {
                b = b->parent();
                // If we advance to the list head, there are no more blocks
                // in this row.
                if (!b || b->type() == GBLKrow)
                    return (0);
            }
            b = b->next();
            return (find_leaf(b));
        }

    void find_first(const GEOblock *row)
        {
            if (!row || row->type() != GBLKrow)
                return;
            const GEOblock *b = (const GEOblock*)row->list();
            b = find_leaf(b);

            // Here, b is a leaf block, need to find the first good zoid.
            while (b) {
                for (const Zlist *z = (const Zlist*)b->list(); z; z = z->next) {
                    if (check_break(z->Z)) {
                        // None of the zoids here are any good, and we know
                        // that no other block contains suitable zoids.
                        return;
                    }
                    if (check_continue(z->Z))
                        continue;
                    // OK, we have a reasonable zoid.
                    g_cur_block = b;
                    g_next_block = find_next_block(b);
                    g_cur_zlist = z;
                    return;
                }
                // We've traversed this block without success, test the next
                // block.

                b = find_next_block(b);
            }
        }

    void find_next()
        {
            if (!g_cur_zlist)
                return;
            for (Zlist *z = g_cur_zlist->next; z; z = z->next) {
                if (check_break(z->Z)) {
                    // None of the zoids here are any good, and we know
                    // that no other block contains suitable zoids.
                    g_next_block = 0;
                    g_cur_block = 0;
                    g_cur_zlist = 0;
                    return;
                }
                if (check_continue(z->Z))
                    continue;
                // OK, we have a reasonable zoid.
                g_cur_zlist = z;
                return;
            }
            g_cur_zlist = 0;
            g_cur_block = 0;
            if (g_next_block) {
                g_cur_block = g_next_block;
                g_cur_zlist = (const Zlist*)g_next_block->list();
                g_next_block = find_next_block(g_next_block);
            }
        }

    const GEOblock *g_next_block;
    const GEOblock *g_cur_block;
    const Zlist *g_cur_zlist;
    int g_xmin;
    int g_xmax;
    int g_yu;
};


// ZdbGenY 
//
// Generator that returns GEOblock row head elements from a Zdb, for
// a given Y range.
//
struct ZdbGenY
{
    ZdbGenY(const Zdb *zdb, int ymin, int ymax)
        {
            g_ymin = ymin;
            g_ymax = ymax;
            g_row = zdb ? find_leaf(zdb->tree()) : 0;
        }

    const GEOblock *next()
        {
            const GEOblock *row = g_row;
            g_row = find_next(g_row);
            return (row);
        }

private:
    bool check_continue(const BBox &BB)     { return (BB.bottom >= g_ymax); }
    bool check_break(const BBox &BB)        { return (BB.top <= g_ymin); }

    const GEOblock *find_leaf(const GEOblock *b)
        {
            while (b) {
                if (check_break(b->bBB()))
                    return (0);
                if (check_continue(b->bBB())) {
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
                    b = (const GEOblock*)b->list();
                    continue;
                }
                return (b);
            }
            return (0);
        }

    const GEOblock *find_next(const GEOblock *b)
        {
            if (!b)
                return (0);
            while (!b->next()) {
                b = b->parent();
                if (!b)
                    return (0);
            }
            b = b->next();
            return (find_leaf(b));
        }

    const GEOblock *g_row;
    int g_ymin;
    int g_ymax;
};


// ZdbGenZ 
//
// Generator that returns Zoid elements from a Zdb, for a given BB.
//
struct ZdbGenZ
{
    ZdbGenZ(const Zdb *zdb, const BBox &BB) :
        g_rowgen(zdb, BB.bottom, BB.top), g_zgen(0, BB.left, BB.right, BB.top)
    {
        const GEOblock *row = g_rowgen.next();
        g_zgen.restart(row);
    }

    // Return the next zoid, also its list container if bp is not null. 
    // Note that if zoids are removed after the call, the bp return can
    // point to freed memory if the block is emptied.
    //
    const Zoid *next(GEOblock **bp = 0)
    {
        const Zoid *Zret = g_zgen.next(bp);
        while (!Zret) {
            const GEOblock *row = g_rowgen.next();
            if (!row)
                break;
            g_zgen.restart(row);
            Zret = g_zgen.next(bp);
        }
        return (Zret);
    }

private:
    ZdbGenY g_rowgen;
    RowGenZ g_zgen;
};

#endif

