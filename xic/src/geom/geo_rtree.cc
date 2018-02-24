
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

#include "geo.h"
#include "geo_rtree.h"
#include "geo_zlist.h"
#include <algorithm>


//-------------------------------------------------------------------------
// The Tree Implementation
//
// The tree structure is ordered, and insertion/deletion is similar to
// a B-tree.  However, a hierarchy of enclosing rectangles is
// maintained, similar to an R-tree, for reasonably efficient spatial
// access.

// Use simplified insertion logic.
// #define RT_SIMPLE

// Check ordering after insertion.
// #define RT_DEBUG

int RTelem::e_minlinks = 4;
int RTelem::e_maxlinks = 8;

RTfunc RTelem::e_delete_funcs[26];
RTfunc RTelem::e_destroy_funcs[26];


// Note:  RTelems are deleted only by the function below, and
// explicitly in other functions in this file only.  They are created
// in this file only.

RTelem::~RTelem()
{
    if (e_type >= 'a' && e_type <= 'z') {
        int p = e_type - 'a';
        if (e_destroy_funcs[p])
            (*e_destroy_funcs[p])(this);
    }
}


//---- Diagnostics

// Test the children count.
//
void
RTelem::test_nch(bool recurse, const char *s)
{
    if (!s)
        s = "count error";
    if (recurse) {
        for (RTelem *r = this; r != RT_ROOT_UP; r = r->parent()) {
            int cc = 0;
            for (RTelem *t = r->children(); t; t = t->sibling())
                cc++;
            if (cc != r->get_count())
                printf("%s %d %d\n", s, cc, r->get_count());
        }
        return;
    }
    int cc = 0;
    for (RTelem *t = children(); t; t = t->sibling())
        cc++;
    if (cc != get_count())
        printf("%s %d %d\n", s, cc, get_count());
}


// Test the BB's and pointers (diagnostic).
//
int
RTelem::test()
{
    if (is_leaf())
        return (0);
    BBox tBB = children()->e_BB;
    int cnt = 1;
    RTelem *rp = children();
    for (RTelem *r = children()->sibling(); r; r = r->sibling()) {
        cnt++;
        tBB.add(&r->e_BB);
        if (r->parent() != this)
            printf("test: bad up\n");
        if (rp->op_gt(r))
            printf("test: bad ordering\n");
        rp = r;
    }
    if (tBB != e_BB)
        printf("test: bad BB\n");
    return (cnt);
}


void
RTelem::show(int depth, int d)
{
    if (depth <= 0) {
        Zoid Z(&e_BB);
        Z.show();
        return;
    }
    if (d == depth) {
        for (RTelem *r = children(); r; r = r->sibling()) {
            Zoid Z(&r->e_BB);
            Z.show();
        }
    }
    else {
        for (RTelem *r = children(); r; r = r->sibling())
            r->show(depth, d+1);
    }
}


void
RTelem::list_test()
{
    if (children())
        printf("list_test: down pointer\n");
     if (!parent())
        printf("list_test: no up pointer\n");
    bool fnd = false;
    for (RTelem *r = parent()->children(); r; r = r->sibling()) {
        if (r == this) {
            fnd = true;
            break;
        }
    }
    if (!fnd)
        printf("list_text: not found\n");
}
// End of RTelem functions


// Set "deferred mode".  In this mode, objects are simply linked into
// a list (headed by rt_root) and the count incremented.  Calling
// unset_deferred will actually create the database.
//
// While in deferred mode, NONE of the database functions other than
// insert should be called.  Doing so is likely to produce faults.
//
// The motivation for this feature is when adding instances in
// CDs::fix_bb, instances are added in arbitrary order, not in file
// (or sorted) order.  If the instances are sorted into the database
// order, addition is far faster.
//
bool
RTree::set_deferred()
{
    if (rt_allocated && !rt_deferred)
        return (false);
    rt_deferred = true;
    return (true);
}


namespace {
    // Compare RTelems, these will be sorted descending in e_BB.top and
    // ascending in e_BB.left.
    //
    inline bool rt_cmp(const RTelem *r1, const RTelem *r2)
    {
        return (r2->op_gt(r1));
    }
}


// Unset "deferred mode", sort the objects, and create the actual
// database tree.  Adding object in sorted order can be much faster
// than adding objects in arbitrary order.
//
bool
RTree::unset_deferred()
{
    if (!rt_deferred)
        return (true);
    if (!rt_allocated) {
        // No objects in list.
        rt_root = 0;  // already true
        rt_deferred = false;
        return (true);
    }
    RTelem **ary = new RTelem*[rt_allocated];
    unsigned int cnt = 0;
    for (RTelem *rt = rt_root; rt; rt = rt->e_right) {
        // a sanity test
        if (cnt >= rt_allocated)
            break;
        ary[cnt++] = rt;
    }
    std::sort(ary, ary + cnt, rt_cmp);

    rt_root = 0;
    rt_allocated = 0;
    rt_deferred = false;

    bool ok = true;
    for (unsigned int i = 0; i < cnt; i++) {
        ary[i]->e_right = 0;
        ary[i]->e_up = 0;
        if (!insert_prv(ary[i]))
            ok = false;
    }
    delete [] ary;
    return (ok);
}


// Remove rd from the tree.
//
bool
RTree::remove(RTelem *rd)
{
    if (!rd->is_leaf()) {
        // not a data node
        return (false);
    }
    if (rt_deferred) {
        RTelem *rp = 0, *rm;
        for (RTelem *r = rt_root; r; r = rm) {
            rm = r->e_right;
            if (r == rd) {
                if (rp)
                    rp->e_right = rm;
                else
                    rt_root = rm;
                rt_allocated--;
                r->e_right = 0;
                r->e_up = 0;
                return (true);
            }
            rp = r;
        }
        return (false);
    }
    RTelem *p = rd->parent();
    if (!p) {
        // not in database
        return (false);
    }
    if (p == RT_ROOT_UP) {
        if (rd == rt_root) {
            rt_root = 0;
            rd->clear_parent();
            rt_allocated = 0;
            return (true);
        }
        // not in this tree
        return (false);
    }

    if (!p->unlink(rd)) {
        // not found
        return (false);
    }
    p->shrink_bb_to_root(rd);

    rd->clear_parent();  // all link pointers now nil

    for(;;) {
        if (p->count_ge(RTelem::e_minlinks))
            break;
        if (p->parent() == RT_ROOT_UP) {
            if (p->count_eq(1)) {
                rt_root = p->children();
                rt_root->set_parent(RT_ROOT_UP);
                p->set_children(0);
                delete p;
            }
            break;
        }

        // The parent p now has too few children, try and merge its
        // contents into adjacent elements.  If we can't merge p
        // away, no problem, just keep it.  Otherwise, recurse.

        RTelem *r = p;

        RTelem *rprev = r->prev();
        int nprev = rprev ? RTelem::e_maxlinks - rprev->get_count() : 0;
        RTelem *rnext = r->next();
        int nnext = rnext ? RTelem::e_maxlinks - rnext->get_count() : 0;

        // If we can't find space for all of r's children, just return.
        if (nprev + nnext < r->get_count())
            break;

        if (rprev) {
            int xcnt = rprev->get_count();
            int cnt = nprev;
            if (cnt > r->get_count())
                cnt = r->get_count();

            RTelem *r0 = 0, *rn = 0;
            for ( int c = cnt; c; c--) {
                RTelem *rt = r->children();
                r->set_children(rt->sibling());
                rt->unhook(rprev);
                r->dec_count();
                if (!r0)
                    r0 = rn = rt;
                else {
                    rn->set_sibling(rt);
                    rn = rt;
                }
                rprev->e_BB.add(&rt->e_BB);
            }

            RTelem *rt = rprev->children();
            while (rt->sibling())
                rt = rt->sibling();
            rt->set_sibling(r0);
            rprev->set_count(xcnt + cnt);
        }
        if (rnext && r->children()) {
            int xcnt = rnext->get_count();
            int cnt = nnext;
            if (cnt > r->get_count())
                cnt = r->get_count();

            RTelem *r0 = r->children();
            r->set_children(0);
            r->set_count(0);
            for (RTelem *rt = r0; rt; rt = rt->sibling()) {
                rt->set_parent(rnext);
                rnext->e_BB.add(&rt->e_BB);
            }
            RTelem *rt = rnext->children();
            rnext->set_children(r0);
            while (r0->sibling())
                r0 = r0->sibling();
            r0->set_sibling(rt);
            rnext->set_count(xcnt + cnt);
        }

        p = r->parent();
        p->unlink(r);
        delete r;
        while (!p->children()) {
            RTelem *pp = p->parent();
            if (pp == RT_ROOT_UP) {
                rt_root = 0;
                rt_allocated = 0;
                delete p;
                return (true);
            }
            pp->unlink(p);
            delete p;
            p = pp;
        }
        p->shrink_bb_to_root(p);
    }
    rt_allocated--;
    return (true);
}


// Return all of the data nodes in one long linked list, and clear the
// tree.
//
RTelem *
RTree::to_list()
{
    if (!rt_root)
        return (0);
    if (!rt_root->children()) {
        RTelem *r = rt_root;
        rt_root = 0;
        r->clear_parent();
        rt_allocated = 0;
        return (r);
    }
    RTelem *st[20];
    st[0] = 0;
    st[1] = rt_root;
    int sp = 1;
    RTelem *r0 = 0;

    while (st[sp]) {
        if (!st[sp]->children()->children()) {
            RTelem *re = st[sp]->children();
            re = re->adv_clear_parent();
            re->set_sibling(r0);
            r0 = st[sp]->children();
            st[sp]->set_children(0);

            while (st[sp]) {
                if ((st[sp] = st[sp]->sibling()) != 0)
                    break;
                sp--;
            }
            continue;
        }
        st[sp+1] = st[sp]->children();
        sp++;
    }
    clear();
    return (r0);
}


// Diagnostic
//
void
RTree::test(RTelem *rt) const
{
    static int items;
    static int nodes;
    static int nch;

    if (!rt) {
        rt = rt_root;
        items = 0;
        nodes = 0;
        nch = 0;
        if (!rt)
            return;
    }
    if (rt->children()) {
        nodes++;
        int n = rt->test();
        nch += n;
        if (n > RTelem::e_maxlinks)
            printf("test: too many links\n");
        else if (rt != rt_root && n < RTelem::e_minlinks)
            printf("test: too few links\n");
        for (RTelem *r = rt->children(); r; r = r->sibling())
            test(r);
    }
    else
        items++;
    if (rt == rt_root) {
        int lev = 0;
        while (rt) {
            lev++;
            rt = rt->children();
        }
        printf("levels %d, nodes %d, frac %g, items %d (allocated %d)\n",
            lev, nodes, nch/(double)(nodes*RTelem::e_maxlinks), items,
            rt_allocated);
    }
}


void
RTree::show(int depth) const
{
    rt_root->show(depth);
}


// Private function to insert rnew into the tree.
//
// If inserting a leaf that has a duplicate already in the database,
// we assert (require) that the new object will be inserted ahead of
// the existing object in database order.  This allows efficient
// testing for duplicate objects.
//
bool
RTree::insert_prv(RTelem *rnew)
{
    if (!rnew)
        return (true);
    if (rnew->parent()) {
        // Already parented, this is an error.
        return (false);
    }
    if (!rnew->is_leaf()) {
        // Not a leaf node, this is an error.
        return (false);
    }

    if (!rt_root) {
        rt_root = rnew;
        rt_root->set_parent(RT_ROOT_UP);
        rt_allocated = 1;
        return (true);
    }

    // Choose best leaf
    RTelem *r = rt_root;
    if (!r->children()) {
        RTelem *rn = new RTelem;
        rn->link(rt_root);
        rn->link(rnew);
        rt_allocated = 2;
        rt_root = rn;
        rt_root->set_parent(RT_ROOT_UP);
        return (true);
    }
    while (r->children()->children())
        r = r->find_branch(rnew);

    if (r->get_count() < RTelem::e_maxlinks) {
        r->link(rnew);
        r->expand_bb_to_root();
    }
    else {
        RTelem *rnx = rnew;
        RTwhere_t wh = r->link(rnx);
        for (;;) {

            if (r->parent() != RT_ROOT_UP) {
#ifdef RT_SIMPLE
                // Reinsertion strategy
                // Upon overflow, if the previous node exists and has
                // space, move one sub-node down.  Otherwise, if the
                // next node exists and has space, move a single
                // sub-node up.
                RTelem *rx = r->prev();
                if (rx && rx->get_count() < RTelem::e_maxlinks) {
                    RTelem *rt = r->children();
                    r->set_children(rt->sibling());
                    rt->unhook(r);
                    r->dec_count();

                    rx->link(rt);
                    rx->expand_bb_to_root();
                    while (r != RT_ROOT_UP) {
                        r->compute_bb();
                        r = r->parent();
                    }
                    break;
                }
                rx = r->next();
                if (rx && rx->get_count() < RTelem::e_maxlinks) {
                    RTelem *rt = r->children();
                    while (rt->sibling()->sibling())
                        rt = rt->sibling();
                    RTelem *rp = rt;
                    rt = rt->sibling();
                    rp->unhook(r);
                    r->dec_count();

                    rx->link(rt);
                    rx->expand_bb_to_root();
                    while (r != RT_ROOT_UP) {
                        r->compute_bb();
                        r = r->parent();
                    }
                    break;
                }

#else
                // Reinsertion strategy
                // The hieuristic assumes that objects are added in
                // order, which will be true when building from files
                // where objects are written in the order maintained
                // by this package.  We also accept the possibility of
                // counter-order.
                //
                // If a node overflows and the new sub-node is last,
                // and the previous node is unfilled, it is filled
                // with entries from the present node.  If the
                // sub-node is not last, and the previous node is
                // unfilled, one sub-node is transferred.  In either
                // case, if the previous node does not exist or is
                // filled, and the next node is unfilled, one sub-node
                // is transferred to the next node.
                //
                // Similar logic applies if the new sub-node is first,
                // i.e., if the next node is unfilled, if is filled
                // from the present node.  Otherwise one sub-node is
                // transferred to the next or previous node.  If no
                // sub-nodes can be transferred, the present node is
                // split.

                int xcnt;
                RTelem *rx = r->prev();
                if (rx && (xcnt = rx->get_count()) < RTelem::e_maxlinks) {
                    // First unlink the sub-nodes, then add them to
                    // the previous node.  We take care with the
                    // ordering to ensure that rnx will be linked
                    // ahead of any duplicates.

                    int cnt = wh == RT_LAST ? RTelem::e_maxlinks - xcnt : 1;
                    RTelem *r0 = 0, *rn = 0;
                    for ( int c = cnt; c; c--) {
                        RTelem *rt = r->children();
                        r->set_children(rt->sibling());
                        rt->unhook(rx);
                        r->dec_count();
                        if (!r0)
                            r0 = rn = rt;
                        else {
                            rn->set_sibling(rt);
                            rn = rt;
                        }
                        rx->e_BB.add(&rt->e_BB);
                    }

                    RTelem *rt = rx->children();
                    while (rt->sibling())
                        rt = rt->sibling();
                    rt->set_sibling(r0);
                    rx->set_count(xcnt + cnt);
                    rx->expand_bb_to_root();
                    rx = r;
                    while (rx != RT_ROOT_UP) {
                        rx->compute_bb();
                        rx = rx->parent();
                    }
                    break;
                }
                rx = r->next();
                if (rx && (xcnt = rx->get_count()) < RTelem::e_maxlinks) {
                    int cnt = wh == RT_FIRST ? RTelem::e_maxlinks - xcnt : 1;

                    RTelem *rt = r->children();
                    int rcnt = r->get_count() - cnt;
                    for (int c = rcnt-1; c; c--)
                        rt = rt->sibling();

                    RTelem *r0 = rt->sibling();
                    rt->set_sibling(0);
                    r->set_count(rcnt);
                    r->compute_bb();
                    for (rt = r0; rt; rt = rt->sibling()) {
                        rt->set_parent(rx);
                        rx->e_BB.add(&rt->e_BB);
                    }
                    rt = rx->children();
                    rx->set_children(r0);
                    while (r0->sibling())
                        r0 = r0->sibling();
                    r0->set_sibling(rt);
                    rx->set_count(xcnt + cnt);

                    rx->expand_bb_to_root();
                    rx = r;
                    while (rx != RT_ROOT_UP) {
                        rx->compute_bb();
                        rx = rx->parent();
                    }
                    break;
                }
#endif
            }

            // Split r such that new node rs < r
            RTelem *rs = new RTelem;
            int cnt = RTelem::e_maxlinks/2;
            if (wh == RT_LAST)
                cnt++;

            RTelem *re = 0;
            for (int i = 0; i < cnt; i++) {
                RTelem *rx = r->children();
                r->set_children(rx->sibling());
                rx->unhook(rs);
                if (!rs->children()) {
                    rs->set_children(rx);
                    rs->e_BB = rx->e_BB;
                }
                else {
                    re->set_sibling(rx);
                    rs->e_BB.add(&rx->e_BB);
                }
                re = rx;
            }
            rs->set_count(cnt);
            r->set_count(r->get_count() - cnt);
            r->compute_bb();

            RTelem *p = r->parent();
            if (p == RT_ROOT_UP) {
                // Grow a new root
                RTelem *rn = new RTelem;
                rs->set_sibling(r);
                rn->set_children(rs);
                rt_root = rn;
                rt_root->set_parent(RT_ROOT_UP);
                r->rehook(rn);
                rs->rehook(rn);
                rn->e_BB = r->e_BB;
                rn->e_BB.add(&rs->e_BB);
                rn->set_count(2);
                break;
            }

            // Stitch in rs just ahead of r.
            rs->set_parent(p);
            if (p->children() == r) {
                p->set_children(rs);
                rs->set_sibling(r);
                wh = RT_FIRST;
            }
            else {
                for (RTelem *e = p->children(); e->sibling();
                        e = e->sibling()) {
                    if (e->sibling() == r) {
                        e->set_sibling(rs);
                        rs->set_sibling(r);
                        break;
                    }
                }
                wh = RT_MID;
            }
            p->e_BB.add(&rs->e_BB);
            p->e_BB.add(&r->e_BB);
            p->inc_count();

            if (p->get_count() < RTelem::e_maxlinks) {
                p->expand_bb_to_root();
                break;
            }
            r = p;
        }
    }
    rt_allocated++;

#ifdef RT_DEBUG
    // Check the order, yell loudly if error.

    RTelem *rp = rnew->prev();
    if (rp && rnew->op_le(rp))
        printf("Ordering error 1 after database insertion!\n");
    RTelem *rn = rnew->next();
    if (rn && rn->op_lt(rnew))
        printf("Ordering error 2 after database insertion!\n");

    for (RTelem *rx = rnew; rx != RT_ROOT_UP; rx = rx->parent()) {
        if (!rx) {
            printf("Null parent after database insertion!\n");
            break;
        }
        rx->test_nch(true, "After insert, count mismatch");
    }
#endif
    return (true);
}
// End of RTree functions


//---- Diagnostics
// #define DT_DIAGNOSTICS

#ifdef RT_DIAGNOSTICS

#include "randval.h"

namespace {
    RTree *Rtree;

    struct RTl
    {
        RTl(RTelem *r, RTl *n) { rt = r; next = n; }

        static void destroy(RTl *s)
            {
                while (s) {
                    RTl *sx = s;
                    s = s->next;
                    delete sx;
                }
            }

        RTl *next;
        RTelem *rt;
    };

    RTl *dlist;


    void
    rand_box(BBox *BB)
    {
        int a1 = (int)randval::rand_value() % 100000;
        int a2 = (int)randval::rand_value() % 100000;
        int a3 = (int)randval::rand_value() % 100000;
        int a4 = (int)randval::rand_value() % 100000;
        BB->left = CDMin(a1, a2);
        BB->bottom = CDMin(a3, a4);
        BB->right = CDMax(a1, a2);
        BB->top = CDMax(a3, a4);
    }
}


void
rtree_test(cCD *cd, const char *str)
{
    if (*str == 'c') {
        delete Rtree;
        Rtree = new RTree;
        return;
    }
    if (*str == 'i') {
        if (!Rtree)
            Rtree = new RTree;
        int n = atoi(str + 2);
        if (n <= 0)
            n = 10000;
        int t1 = Tvals::millisec();
        for (int i = 0; i < n; i++) {
            RTelem *r = new(*cd) RTelem;
            dlist = new RTl(r, dlist);
            rand_box(&r->oBB);
            Rtree->insert(r);
        }
        printf("%d insert: %d %ld\n", n, Rtree->num_elements(),
            Tvals::millisec() - t1);
        Rtree->test();
        for (RTl *d = dlist; d; d = d->next)
            d->rt->list_test();;
        return;
    }
    if (*str == 'd') {
        int n = atoi(str + 2);
        int t1 = Tvals::millisec();
        int cnt = 0;
        while (dlist) {
            RTl *dn = dlist->next;
            if (!Rtree->remove(dlist->rt))
                printf("remove failed\n");
            delete dlist;
            dlist = dn;
            cnt++;
            if (n && cnt == n)
                break;
        }
        printf("%d remove: %d %ld\n", n, Rtree->num_elements(),
            Tvals::millisec() - t1);
        Rtree->test();
        for (RTl *d = dlist; d; d = d->next)
            d->rt->list_test();;
        return;
    }
}

#endif

// End of Tree implementation
//-------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Database Traversal Generator
//-----------------------------------------------------------------------------

// Return the data element, and advance to the next one.  Return 0
// when the traversal is complete.  The returned element will touch
// or overlap BB.
//
RTelem *
RTgen::next_element()
{
    while (st[sp]) {
        if (BB.isect_i(&st[sp]->oBB())) {
            if (st[sp]->is_leaf()) {
                RTelem *last = st[sp];
                advance();
                return (last);
            }
            st[sp+1] = st[sp]->children();
            sp++;
            continue;
        }
        advance();
    }
    return (0);
}


// Return the data element, and advance to the next one.  Return 0
// when the traversal is complete.  The returned element will
// overlap BB with nonzero area.
//
RTelem *
RTgen::next_element_notouch()
{
    while (st[sp]) {
        if (BB.isect_x(&st[sp]->oBB())) {
            if (st[sp]->is_leaf()) {
                RTelem *last = st[sp];
                advance();
                return (last);
            }
            st[sp+1] = st[sp]->children();
            sp++;
            continue;
        }
        advance();
    }
    return (0);
}


// Return the data element, and advance to the next one.  Return 0
// when the traversal is complete.  The returned element will have
// the exact BB.
//
RTelem *
RTgen::next_element_exact()
{
    while (st[sp]) {
        if (BB <= st[sp]->oBB()) {
            if (st[sp]->is_leaf() && BB == st[sp]->oBB()) {
                RTelem *last = st[sp];
                advance();
                return (last);
            }
            st[sp+1] = st[sp]->children();
            sp++;
            continue;
        }
        advance();
    }
    return (0);
}


// Return the data element, and advance to the next one.  Return 0
// when the traversal is complete.  All elements are returned, i.e.,
// BB is ignored.
//
RTelem *
RTgen::next_element_nchk()
{
    while (st[sp]) {
        if (st[sp]->is_leaf()) {
            RTelem *last = st[sp];
            advance();
            return (last);
        }
        st[sp+1] = st[sp]->children();
        sp++;
    }
    return (0);
}
// End RTgen functions

