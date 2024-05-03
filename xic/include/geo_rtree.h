
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

#ifndef GEO_RTREE_H
#define GEO_RTREE_H

#include "cd_memmgr_cfg.h"
#include "geo_box.h"


//-------------------------------------------------------------------------
// The R-Tree Implementation

typedef void(*RTfunc)(struct RTelem*);

// The root is given a nonzero up pointer, so that in the case where
// the root is also a data element (the tree contains one object) the
// object will have a nonzero up pointer, so we know it is in the tree.
//
#define RT_ROOT_UP (RTelem*)1

enum RTwhere_t { RT_FIRST, RT_MID, RT_LAST };

// Tree link element, and superclass for database elements.
//
struct RTelem
{
    friend struct RTree;
    friend struct RTgen;

#ifdef CD_USE_MANAGER
    void *operator new(size_t);
    void operator delete(void*, size_t);
#endif

    RTelem()
        {
            e_up = 0;               // parent
            e_right = 0;            // sibling
            e_children = 0;         // children, non-leaf nodes only
            e_type = 0;             // 0 => non-leaf node
            e_nchildren = 0;        // number of children in non-leaf nodes
            e_flags = 0;            // not used
        }

    virtual ~RTelem();

    // Free the element and its descendents.
    static void destroy(RTelem *rte)
        {
            if (rte) {
                RTelem *rn;
                for (RTelem *r = rte->children(); r; r = rn) {
                    rn = r->sibling();
                    destroy(r);
                }
                delete rte;
            }
        }

    // Ordering
    bool op_gt(const RTelem *r) const
        {
            return (e_BB.top < r->e_BB.top ||
                (e_BB.top == r->e_BB.top && xfirst() > r->xfirst()));
        }
    bool op_ge(const RTelem *r) const
        {
            return (e_BB.top < r->e_BB.top ||
                (e_BB.top == r->e_BB.top && xfirst() >= r->xfirst()));
        }
    bool op_lt(const RTelem *r) const
        {
            return (e_BB.top > r->e_BB.top ||
                (e_BB.top == r->e_BB.top && xfirst() < r->xfirst()));
        }
    bool op_le(const RTelem *r) const
        {
            return (e_BB.top > r->e_BB.top ||
                (e_BB.top == r->e_BB.top && xfirst() <= r->xfirst()));
        }

    // Return true if this is in the tree, which is true if parented.
    //
    bool in_db() const
        {
            return (e_up != 0);
        }

    // Return the prior element on the same level as this.
    //
    RTelem *prev() const
        {
            if (parent() != RT_ROOT_UP) {
                int sp = 0;
                for (const RTelem *r = this; r->parent() != RT_ROOT_UP;
                        r = r->parent(), sp++) {
                    RTelem *rx = r->parent()->children();
                    if (rx != r) {
                        while (rx->sibling() != r)
                            rx = rx->sibling();
                        while (sp--) {
                            rx = rx->children();
                            while (rx->sibling())
                                rx = rx->sibling();
                        }
                        return (rx);
                    }
                }
            }
            return (0);
        }

    // Return the next element on the same level as this.
    //
    RTelem *next()
        {
            int sp = 0;
            for (RTelem *r = this; r != RT_ROOT_UP;
                    r = r->parent(), sp++) {
                if (r->sibling()) {
                    r = r->sibling();
                    while (sp--)
                        r = r->children();
                    return (r);
                }
            }
            return (0);
        }

    // For working with RTelem lists returned from RTree::to_list().
    //
    static RTelem *list_next(RTelem *thisel, RTelem **nx)
        {
            RTelem *r = thisel;
            if (r) {
                *nx = r->sibling();
                r->set_sibling(0);
             }
             return (r);
        }

    const BBox &oBB() const
        {
            return (e_BB);
        }

private:

    // Return the next element in a list of elements (children).
    //
    RTelem *sibling() const
        {
            return (e_right);
        }

    // Set the next element pointer.
    //
    void set_sibling(RTelem *r)
        {
            e_right = r;
        }

    // Return the element containing this.
    //
    RTelem *parent() const
        {
            return (e_up);
        }

    // Set the parent pointer.
    //
    void set_parent(RTelem *p)
        {
            e_up = p;
        }

    // Clear the sibling pointer and set the parent pointer, called
    // when unlinking from tree.
    //
    void unhook(RTelem *p)
        {
            e_up = p;
            e_right = 0;
        }

    // Set parent pointer if necessary.
    //
    void rehook(RTelem *p)
        {
            set_parent(p);
        }

    void clear_parent()
        {
            e_up = 0;
        }

    // Advance to end of list, clear parent pointer.
    //
    RTelem *adv_clear_parent()
        {
            RTelem *r = this;
            r->e_up = 0;
            while (r->sibling()) {
                r = r->sibling();
                r->e_up = 0;
            }
            return (r);
        }

    // The data elements are ordered descending in e_BB.top and
    // ascending in e_BB.left.  The e_BB.top on any level corresponds
    // to the e_BB.top of the first data element underneath, however
    // this is not true for e_BB.left.  This function descends down to
    // return the e_BB.left of the first element.  We should probably
    // add a cache for this in the RTelem definition.
    //
    int xfirst() const
        {
            const RTelem *r = this, *rn;
            while ((rn = r->children()))
                r = rn;
            return (r->e_BB.left);
        }

    RTelem *first()
        {
            RTelem *r = this, *rn;
            while ((rn = r->children()))
                r = rn;
            return (r);
        }

    // Link ch into this, returning a position code.  This takes care
    // of the BB, pointers, and occupation count.
    //
    RTwhere_t link(RTelem *ch)
        {
            RTwhere_t wh = RT_MID;
            RTelem *r = children();
            if (!r || ch->op_le(r)) {
                ch->set_sibling(r);
                set_children(ch);
                wh = RT_FIRST;
            }
            else {
                for ( ; ; r = r->sibling()) {
                    if (!r->sibling()) {
                        wh = RT_LAST;
                        break;
                    }
                    if (ch->op_le(r->sibling()))
                        break;
                }
                ch->set_sibling(r->sibling());
                r->set_sibling(ch);
            }
            ch->rehook(this);
            if (!children()->sibling())
                e_BB = ch->e_BB;
            else
                e_BB.add(&ch->e_BB);
            inc_count();
            return (wh);
        }

    // Unlink ch from this, returning false if not found (error).
    // This keeps the up pointer needed for reinsertion.  The BB is
    // *not* adjusted, but the occupation count is decremented.
    //
    bool unlink(RTelem *ch)
        {
            RTelem *rp = 0;
            for (RTelem *r = children(); r; rp = r, r = r->sibling()) {
                if (r == ch) {
                    if (!rp)
                        set_children(r->sibling());
                    else
                        rp->set_sibling(r->sibling());
                    r->unhook(this);
                    dec_count();
                    return (true);
                }
            }
            return (false);
        }

    // If this BB has been expanded (by adding an element), this
    // function propagates the BB change up the tree.
    //
    void expand_bb_to_root()
        {
            RTelem *r = this, *p = parent();
            while (p != RT_ROOT_UP) {
                if (!p->e_BB.addc(&r->e_BB))
                    break;
                r = p;
                p = p->parent();
            }
        }

    // If the BB has potentially shrunk (by removing an element), this
    // function will fix the BB and propagate the change up the tree.
    //
    void shrink_bb_to_root(RTelem *ch)
        {
            for (RTelem *r = this; r != RT_ROOT_UP; r = r->parent()) {
                if (r->e_BB > ch->e_BB)
                    break;
                r->compute_bb();
            }
        }

    // This function returns the appropriate branch in which to insert
    // rx.
    //
    RTelem *find_branch(RTelem *rx) const
        {
            RTelem *rc = children();
            if (rx->op_le(rc))
                return (rc);
            RTelem *r;
            for (r = rc; r->sibling(); r = r->sibling()) {
                if (rx->op_le(r->sibling()))
                    break;
            }
            return (r);
        }

    // This function computes the BB from the sub-node BBs.
    //
    void compute_bb()
        {
            RTelem *rc = children();
            if (rc) {
                e_BB = rc->e_BB;
                for (RTelem *r = rc->sibling(); r; r = r->sibling())
                    e_BB.add(&r->e_BB);
            }
        }

    // Return true is this is a leaf (data) node.
    //
    bool is_leaf() const
        {
            return (e_type);
        }

    // Return list head of children.
    //
    RTelem *children() const
        {
            return (is_leaf() ? 0 : e_children);
        }

    // Set pointer to children list.
    //
    void set_children(RTelem *c)
        {
            if (!is_leaf())
                e_children = c;
        }

    // Return number of children.
    int get_count() const
        {
            return (is_leaf() ? 0 : e_nchildren);
        }

    // Set number of children cache.
    //
    void set_count(int c)
        {
            if (!is_leaf())
                e_nchildren = c;
        }

    // Increment number of children count cache.
    //
    void inc_count()
        {
            e_nchildren++;
        }

    // Decrement number of children count cache.
    //
    void dec_count()
        {
            e_nchildren--;
        }

    bool count_ge(int n) const
        {
            return (e_nchildren >= n);
        }

    bool count_eq(int n) const
        {
            return (e_nchildren == n);
        }

public:
    void test_nch(bool, const char*);
    int test();
    void show(int, int = 1);
    void list_test();

protected:
    RTelem *e_up;       // parent
    RTelem *e_right;    // sibling
    RTelem *e_children; // e_type == 0:  pointer to children
                        // e_type != 0:  used by application
    BBox e_BB;          // bounding box
    char e_type;        // object type code: If e_type != 0,
                        // this is a leaf node, otherwise not
    char e_nchildren;   // e_type == 0:  number of children
                        // e_type != 0:  used by application
    unsigned short e_flags; // used by application

    static int e_minlinks;      // Minimum link count.
    static int e_maxlinks;      // Maximum link count.

    // Derived objects.
    // ----------------
    // We want to use be able to flexibly derive objects from RTelem
    // for use as leaf nodes in the application.  We are also choosing
    // to not use C++ virtual fuctions, to reduce structure sizes and
    // possibly improve speed.  Finally, the derived objects may or
    // may not use block allocation or some other allocation
    // technique.
    //
    // Derived objects follow these rules:
    // 1.  Derived objects have no destructor.
    // 2.  Derived objects must be given a type of 'a' through 'z',
    //     i.e., the e_type field is set to a lower-case letter.  This
    //     provides 26 types, which is sufficient for now.
    // 3.  The application is responsible for allocating and freeing
    //     the derived objects.  This includes defining operator new/
    //     delete as appropriate.
    // 4.  The application must register the type by calling
    //     registerType (see below).

public:
    // Register a class derived from RTelem.
    // typecode:      The object's type field, must be 'a' thru 'z'.
    // destroy_proc:  The object's class destructor.
    // delete_proc:   The object's deallocation function.
    // The function args can be null.
    //
    static bool registerType(char typecode, RTfunc destroy_proc,
            RTfunc delete_proc)
        {
            if (typecode < 'a' || typecode > 'z')
                return (false);
            int i = typecode - 'a';
            e_destroy_funcs[i] = destroy_proc;
            e_delete_funcs[i] = delete_proc;
            return (true);
        }

private:
    static RTfunc e_destroy_funcs[26];
    static RTfunc e_delete_funcs[26];
};


// General class to hold the r-tree.  Note that the destructor does not
// free the nodes.
//
struct RTree
{
    friend struct RTgen;

    RTree()
        {
            rt_root = 0;
            rt_allocated = 0;
            rt_deferred = false;
        }

    virtual ~RTree() { };

    void clear()
        {
            RTelem::destroy(rt_root);
            rt_root = 0;
            rt_allocated = 0;
            rt_deferred = false;
        }

    unsigned int num_elements() const { return (rt_allocated); }
    const BBox *root_bb()   const { return (rt_root ? &rt_root->e_BB : 0); }
    RTelem *first_element() const { return (rt_root ? rt_root->first() : 0); }
    bool is_deferred()      const { return (rt_deferred); }

    bool insert(RTelem *rd)
        {
            if (rd->e_up || rd->e_right || !rd->is_leaf())
                return (false);
            if (rt_deferred) {
                // Set the up pointer nonzero to mark as used,
                // otherwise the one at the end of the e_right list
                // can be "inserted" twice, with unpleasant results.

                rd->e_up = RT_ROOT_UP;
                rd->e_right = rt_root;
                rt_root = rd;
                rt_allocated++;
                return (true);
            }
            return (insert_prv(rd));
        }

    bool set_deferred();
    bool unset_deferred();
    bool remove(RTelem*);
    RTelem *to_list();
    void test(RTelem* = 0) const;
    void show(int) const;

private:
    bool insert_prv(RTelem*);

    RTelem *rt_root;            // Tree root element.
    unsigned int rt_allocated;  // Element count.
    bool rt_deferred;           // Don't insert new elements, just link.
};

// Generator class for r-tree range search.
//
struct RTgen
{
    RTgen()
        {
            st[0] = 0;
            sp = 0;
        }

    void init(const RTree *tree, const BBox *xBB)
        {
            BB = *xBB;
            st[0] = 0;
            st[1] = tree ? tree->rt_root : 0;
            sp = 1;
        }

    // Skip the element, if any, currently ready for return.
    //
    void skip_element(RTelem *r) { if (sp && st[sp] == r) advance(); }

    // Clear the generator.
    //
    void clear_elements() { sp = 0; }

    RTelem *next_element();
    RTelem *next_element_notouch();
    RTelem *next_element_exact();
    RTelem *next_element_nchk();

protected:
    // Advance past the current element to begin next search.
    //
    void advance()
        {
            while (st[sp]) {
                if ((st[sp] = st[sp]->sibling()) != 0)
                    break;
                sp--;
            }
        }

    BBox BB;        // comparison region
    RTelem *st[20]; // stack
    int sp;         // stack pointer
};

// End of R-Tree implementation
//-------------------------------------------------------------------------

#endif
