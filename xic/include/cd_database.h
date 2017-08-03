
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

#ifndef CD_DATABASE_H
#define CD_DATABASE_H

#include "cd_memmgr_cfg.h"
#include "geo_rtree.h"


struct CDc;
struct CDcxy;
struct CDg;
struct CDl;
struct CDo;
struct CDol;
struct CDs;
struct sPF;

struct CDtree : public RTree
{
    CDtree()                    { t_ldesc = 0; }

    CDl *ldesc()          const { return (t_ldesc); }
    void set_ldesc(CDl *ld)     { t_ldesc = ld; }

private:
    CDl *t_ldesc;
};


// Superclass for cells, contains database hook.  Instantiated only as
// base class of CDs.
//
struct CDdb
{
#ifdef CD_USE_MANAGER
    void *operator new(size_t);
    void operator delete(void*, size_t);
#endif
    CDdb()
        {
            db_layer_heads = 0;
            db_layers_used = 0;
            db_mod_count = 0;
        }

    // Explicitly call subclass destroy function, subclass has no
    // destructor.
    ~CDdb();

    // The list heads are sorted in order of the ldesc pointer, so we
    // can use a binary search to find the list head given a layer.
    //
    CDtree *db_find_layer_head(const CDl *ldesc) const
        {
            if (!db_layer_heads || !db_layers_used)
                return (0);
            unsigned int min = 0;
            unsigned int max = db_layers_used-1;
            for (;;) {
                int mid = (min + max)/2;
                CDl *t = db_layer_heads[mid].ldesc();
                if (ldesc < t) {
                    if (!mid || min > (max = mid - 1))
                        return (0);
                    continue;
                }
                if (ldesc > t) {
                    if ((min = mid + 1) > max)
                        return (0);
                    continue;
                }
                return (&db_layer_heads[mid]);
            }
        }

    // Return the box enclosing all objects on passed layer, or 0 if
    // no objects.
    //
    const BBox *db_layer_bb(const CDl *ldesc) const
        {
            CDtree *rt = db_find_layer_head(ldesc);
            if (!rt)
                return (0);
            return (rt->root_bb());
        }

    const CDtree *layer_heads()     const { return (db_layer_heads); }
    int num_layers_used()           const { return (db_layers_used); }

    bool db_insert_deferred_instances();
    bool db_insert(CDo*);
    bool db_remove(CDo*);
    bool db_is_empty(const CDl*) const;
    void db_clear_layer(const CDl*);
    void db_clear_layers(bool = false);
    void db_clear();
    bool db_check_coinc(CDo*);
    CDol *db_list_coinc(CDo*);
    void db_rebuild(bool(*process)(CDo*, CDdb*, void*), void*);
    void db_merge(CDdb*, bool(*process)(CDo*, CDdb*, CDdb*, void*), void*);
    void db_bb(BBox*) const;
    unsigned int db_objcnt() const;
    void db_bincnt(const CDl*, int) const;

protected:
    CDtree *db_layer_heads;         // array of tree heads for layers
    unsigned short db_layers_used;  // length of array
    unsigned short db_mod_count;    // used in derived class for
                                    //  modification count
};


//-----------------------------------------------------------------------------
// Object Retrieval Generator
//-----------------------------------------------------------------------------

// CDg::flags: clipping code
#define GEN_CLIP_LEFT   0x1
#define GEN_CLIP_BOTTOM 0x2
#define GEN_CLIP_RIGHT  0x4
#define GEN_CLIP_TOP    0x8
#define GEN_CLIP_MASK   0xf

// CDg::flags: search mode (set at most 1)
#define GEN_RET_ALL     0x10
#define GEN_RET_NOTOUCH 0x20
#define GEN_RET_EXACT   0x40
#define GEN_RET_MASK    0x70

// CDg::flags: set for electrical mode
#define GEN_ELEC_MODE   0x80

struct SymTab;

// Generator object, for database traversal.  These are crated by
// cTfmStack::InitGen.
//
struct CDg : public RTgen
{
    CDg();
    ~CDg();

    void check_clear_elements(const CDdb *cntr, bool all, const CDl *ld,
        const CDl *ldnot)
        {
            if (container == cntr) {
                if (all || ldesc == ld || (ldnot && ldesc != ldnot))
                    clear_elements();
            }
        }

    bool is_active()        { return (container != 0); }
    bool is_elec()          { return (flags & GEN_ELEC_MODE); }
    void setflags(int f)    { flags |= (f & (GEN_CLIP_MASK | GEN_RET_MASK)); }

    void init_gen(const CDs*, const CDl*, const BBox* = 0);
    CDo *next();
    void setup(const CDdb*, const CDl*, const BBox*, int);
    void clear();

    static void print_gen_allocation();

protected:
    int flags;              // return and clipping mode
    const CDdb *container;  // cell containing database
    const CDl *ldesc;       // layer to search
};


//-----------------------------------------------------------------------------
// Pseudo-Flat Generator
//-----------------------------------------------------------------------------

// Pseudo-flat generator context element.
//
struct sPFel
{
    friend struct sPF;

    sPFel(const CDs *s, const BBox *bb, const CDl *ld, int d)
        {
            el_next = el_prev = 0;
            el_sdesc = s;
            el_cdesc = 0;
            el_x = el_y = 0;
            el_x1 = el_x2 = 0;
            el_y1 = el_y2 = 0;
            el_AOI = *bb;
            el_ldesc = ld;
            el_depth = d;
        }

    ~sPFel();

    bool bb_check(bool touchok, const BBox &BB)
        {
            return (!touchok &&
                (BB.left == el_invAOI.right || BB.right == el_invAOI.left ||
                BB.bottom == el_invAOI.top || BB.top == el_invAOI.bottom));
        }

    static sPFel *dup(const sPFel*);
    bool init(sPF*);
    bool advance(sPF*);
  
private:
    sPFel *el_next;             // subcell context
    sPFel *el_prev;             // parent context
    CDg el_lgdesc;              // generator desc for layer
    CDg el_sgdesc;              // generator desc for subcells;
    const CDs *el_sdesc;        // present cell desc
    const CDc *el_cdesc;        // instance
    unsigned int el_x, el_y;    // instance array indices
    unsigned int el_x1, el_x2;  // instance range x
    unsigned int el_y1, el_y2;  // instance range y
    CDtf el_tf;                 // present transform
    BBox el_AOI;                // area to search
    BBox el_invAOI;             // inverse transformed area
    const CDl *el_ldesc;        // layer being searched
    int el_depth;               // depth in hierarchy
};

// Pseudo-flat generator
//
struct sPF : public cTfmStack
{
    friend struct sPFel;

    sPF(const CDs*, const BBox*, const CDl*, int);
    sPF(const CDc*, const BBox*, const CDl*, int);
    sPF(sPF&);
    ~sPF();

    void set_info_mode(int);
    void set_returned(const char*);
    CDo *next(bool, bool);
    void purge(const CDc*);
    void purge(const CDs*);
    sPF *dup();
    int info_stack(CDcxy*) const;
    int drc_stack(const CDc**, int) const;

    bool is_elec()          const { return (pf_dmode == Electrical); }
    const CDs *cur_sdesc()  const { return (pf_gen ? pf_gen->el_sdesc : 0); }
    bool has_error()        const { return (pf_error); }

    void clear()
        {
            sPFel *tmp = pf_gen;
            pf_gen = 0;
            delete tmp;
        }

    bool reinit(const CDs *sdesc, const BBox *BB, const CDl *ld, int maxdp)
        {
            pf_maxdepth = maxdp;
            return (init_gen(sdesc, BB, ld));
        }

    static void set_skip(CDs *sd)       { pf_skip_sdesc = sd; }
    static CDs *get_skip()              { return (pf_skip_sdesc); }
    static void set_skip_drc(bool s)    { pf_skip_drc = s; }
    static bool get_skip_drc()          { return (pf_skip_drc); }

    static void print_gen_allocation();

private:
    bool init_gen(const CDs*, const BBox*, const CDl*);
    bool init_gen(sPFel*, const CDs*);
    bool init_gen(sPFel**, const CDc*, const BBox*, const CDl*);

    sPFel *pf_gen;
    int pf_maxdepth;

    // When this value is given, "info mode" (labels and unexpanded
    // subcells returned) is set.  The value is a window expansion
    // flag.
    int pf_info_mode_flag;

    // What to return.
    bool pf_boxes;
    bool pf_polys;
    bool pf_wires;
    bool pf_labels;

    DisplayMode pf_dmode;
    bool pf_error;

    static CDs *pf_skip_sdesc;
    static bool pf_skip_drc;
};


//-----------------------------------------------------------------------------
// Cell Array Traversal
//-----------------------------------------------------------------------------

// Generator for iterating through instance arrays.  The constructor
// parameters are the indices, inclusive, with 2 >= 1.
// Usage:
// xyg_t xyg(...)
// do {
//    ... (xyg.x, xyg.y);
// } while ( xyg.advance() );
//
struct xyg_t
{
    xyg_t(unsigned int x1, unsigned int x2, unsigned int y1, unsigned int y2)
        {
            xy_x1 = x1;
            xy_x2 = x2;
            xy_y1 = y1;
            xy_y2 = y2;
            x = x1;
            y = y2;
        }

    bool advance()
        {
            x++;
            if (x > xy_x2) {
                x = xy_x1;
                if (y == xy_y1)
                    return (false);
                y--;
            }
            return (true);
        }

    unsigned int x, y;
private:
    unsigned int xy_x1, xy_x2, xy_y1, xy_y2;
};

#endif

