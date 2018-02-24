
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
#include "cd_database.h"
#include "cd_cbin.h"
#include "cd_objects.h"
#include "cd_instance.h"
#include <pthread.h>


// Keep track of the existence of generators, so we can purge them of
// bad objects when necessary.
//
namespace {
    template <class T>
    struct sGenTab
    {
        sGenTab() : gt_tab(false, false)
            {
                pthread_mutex_init(&gt_mtx, 0);
            }

        void add(T *a)
            {
                if (!a)
                    return;
                pthread_mutex_lock(&gt_mtx);
                gt_tab.add((unsigned long)a, 0, true);
                pthread_mutex_unlock(&gt_mtx);
            }

        void remove(T *a)
            {
                if (!a)
                    return;
                pthread_mutex_lock(&gt_mtx);
                gt_tab.remove((unsigned long)a);
                pthread_mutex_unlock(&gt_mtx);
            }

        SymTab *table()
            {
                return (&gt_tab);
            }

    private:
        SymTab gt_tab;
        pthread_mutex_t gt_mtx;
    };

    sGenTab<CDg> CDg_genTab;
    sGenTab<sPF> sPF_genTab;
}


// Destructor.  This handles all CDdb/CDs cleanup.  In the
// application, CDdb is only instantiated as a CDs.  We don't use
// virtual functions or destructor, simply to save a bit of space by
// avoiding the function table pointer that would otherwise be
// included in the structure.
//
CDdb::~CDdb()
{
    ((CDs*)this)->clear(false);
}


// In CDs::fixBBs, use is made of "deferred mode" for instances. 
// This is initiated by calling CD()->SetDeferInst(true).  Note that
// this is a second use of this flag; it is also set during file
// reading to prevent instances from being added to the database. 
// Here, it causes new cell layer databases to be created with the
// deferred flag set, so inserted instances are saved in a linked
// list, and not merged into the database. 
//
// The instances are actually added in fixBBs, by calling
// db_insert_deferred instances.  This will create the database after
// sorting the linked list.  In sorted order, database creation is
// much faster.

// Unset deferred mode and create the database for ld.  See
// description of RTree::unset_deferred.  This should be called after
// calling CD()->SetDeferInst(false).
//
bool
CDdb::db_insert_deferred_instances()
{
    for (unsigned int i = 0; i < db_layers_used; i++) {
        CDtree *l = db_layer_heads + i;

        // If either index is 0, this is a cell layer.  In that case
        // if an index is not 0, is must be -1.  If -1, it is not in
        // use and we wouldn't see it here, so if we see a 0 it must
        // be for our cell mode.

        if (l->ldesc()->index(Physical) == 0 ||
                l->ldesc()->index(Electrical) == 0)
            return (l->unset_deferred());
    }
    return (true);
}


namespace {
    // Lock the insert/remove functions, as they are used in layer
    // expression evaluation which is where multi-threading is applied
    // at present.  A lot of work is needed to efficiently protect the
    // databases for general access.
    //
    // Given that we are likely writing to one cell (the current cell)
    // and one layer (the layer expression target), the single mutex
    // is reasonable.

    pthread_mutex_t db_mtx = PTHREAD_MUTEX_INITIALIZER;
}


// Public function to insert odesc into the database, true is returned
// on success.
//
bool
CDdb::db_insert(CDo *odesc)
{
    if (!odesc)
        return (false);
    pthread_mutex_lock(&db_mtx);
    CDtree *l = db_find_layer_head(odesc->ldesc());
    if (!l) {
        if (!db_layer_heads) {
            db_layer_heads = new CDtree[1];
            db_layer_heads->set_ldesc(odesc->ldesc());
            db_layers_used = 1;
            l = db_layer_heads;
        }
        else {
            CDtree *lt = new CDtree[db_layers_used + 1];
            unsigned int cnt = 0;
            for ( ; cnt < db_layers_used; cnt++) {
                if (db_layer_heads[cnt].ldesc() < odesc->ldesc())
                    lt[cnt] = db_layer_heads[cnt];
                else
                    break;
            }
            lt[cnt].set_ldesc(odesc->ldesc());
            l = lt + cnt;
            for ( ; cnt < db_layers_used; cnt++)
                lt[cnt+1] = db_layer_heads[cnt];
            db_layers_used++;

            delete [] db_layer_heads;
            db_layer_heads = lt;
        }

        // If CD()->SetDeferInst(true) is in effect, set the deferred
        // flag in the new database created for the instance layer. 
        // When all instances have been added, call
        // CD()->SetDeferInst(false) and db_insert_deferred_instances
        // to actually create the database.  This is generally faster
        // than adding the instances directly, due to the sorting in
        // db_insert_deferred_instances.

        if (CD()->IsDeferInst() && odesc->type() == CDINSTANCE)
            l->set_deferred();
    }
    bool ret = l->insert(odesc);
    pthread_mutex_unlock(&db_mtx);
    return (ret);
}


// Public function to remove odesc from the database.  True is returned
// on success.
//
bool
CDdb::db_remove(CDo *odesc)
{
    if (!odesc)
        return (true);

    pthread_mutex_lock(&db_mtx);
    {
        SymTabGen gen(CDg_genTab.table());
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            CDg *gd = (CDg*)ent->stTag;
            if (((CDs*)this)->isElectrical() == gd->is_elec())
                gd->skip_element(odesc);
        }
    }
    if (odesc->type() == CDINSTANCE) {
        SymTabGen gen(sPF_genTab.table());
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            sPF *pf = (sPF*)ent->stTag;
            if (((CDs*)this)->isElectrical() == pf->is_elec())
                pf->purge((CDc*)odesc);
        }
    }

    CDtree *l = db_find_layer_head(odesc->ldesc());
    if (!l)
        return (false);
    bool ret = l->remove(odesc);
    pthread_mutex_unlock(&db_mtx);
    return (ret);
}


// Return true if there is nothing in the database, for the layer if
// given.
//
bool
CDdb::db_is_empty(const CDl *ld) const
{

    for (unsigned int i = 0; i < db_layers_used; i++) {
        if (ld && ld != db_layer_heads[i].ldesc())
            continue;
        if (db_layer_heads[i].num_elements() > 0)
            return (false);
        if (ld)
            break;
    }
    return (true);
}


// Clear all objects on ld from the database.  The objects are deleted.
//
void
CDdb::db_clear_layer(const CDl *ld)
{
    RTree *l = db_find_layer_head(ld);
    if (!l)
        return;

    {
        SymTabGen gen(CDg_genTab.table());
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            CDg *gd = (CDg*)ent->stTag;
            if (((CDs*)this)->isElectrical() == gd->is_elec())
                gd->check_clear_elements(this, false, ld, 0);
        }
    }
    if (ld == CellLayer()) {
        SymTabGen gen(sPF_genTab.table());
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            sPF *pf = (sPF*)ent->stTag;
            if (((CDs*)this)->isElectrical() == pf->is_elec())
                pf->purge((CDs*)this);
        }
    }

    l->clear();
}


// Free the objects, but retain the list heads.  By default, retain the
// cell layer.
//
void
CDdb::db_clear_layers(bool all)
{
    if (!db_layer_heads)
        return;

    CDl *cell_layer = CellLayer();
    {
        SymTabGen gen(CDg_genTab.table());
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            CDg *gd = (CDg*)ent->stTag;
            if (((CDs*)this)->isElectrical() == gd->is_elec())
                gd->check_clear_elements(this, all, 0, cell_layer);
        }
    }
    if (all) {
        SymTabGen gen(sPF_genTab.table());
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            sPF *pf = (sPF*)ent->stTag;
            if (((CDs*)this)->isElectrical() == pf->is_elec())
                pf->purge((CDs*)this);
        }
    }

    for (unsigned int i = 0; i < db_layers_used; i++) {
        if (db_layer_heads[i].ldesc() == cell_layer && !all)
            continue;
        db_layer_heads[i].clear();
    }
}


// Free the bin data structure *and* the objects.
//
void
CDdb::db_clear()
{
    if (!db_layer_heads)
        return;

    {
        SymTabGen gen(CDg_genTab.table());
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            CDg *gd = (CDg*)ent->stTag;
            if (((CDs*)this)->isElectrical() == gd->is_elec())
                gd->check_clear_elements(this, true, 0, 0);
        }
    }
    {
        SymTabGen gen(sPF_genTab.table());
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            sPF *pf = (sPF*)ent->stTag;
            if (((CDs*)this)->isElectrical() == pf->is_elec())
                pf->purge((CDs*)this);
        }
    }

    for (unsigned int i = 0; i < db_layers_used; i++)
        db_layer_heads[i].clear();
    delete [] db_layer_heads;
    db_layer_heads = 0;
    db_layers_used = 0;
}


// Return true if there is an object identical to od already in the
// database.  This assumes that od was just added to the database, and
// makes use of the ordering which causes od to be added ahead of any
// existing duplicate.  This is NOT a general purpose function!
//
bool
CDdb::db_check_coinc(CDo *od)
{
    for (CDo *o = od->db_next(); o; o = o->db_next()) {
        if (od->oBB().top != o->oBB().top || od->oBB().left != o->oBB().left)
            break;
        if (*o == *od)
            return (true);
    }
    return (false);
}


// Return a list of objects identical to od that are already in the
// database.  This will NOT find duplicates that happen to be ahead of
// od in the database order.  This is NOT a general purpose function!
//
CDol *
CDdb::db_list_coinc(CDo *od)
{
    CDol *l0 = 0;
    for (CDo *o = od->db_next(); o; o = o->db_next()) {
        if (od->oBB().top != o->oBB().top || od->oBB().left != o->oBB().left)
            break;
        if (*o == *od)
            l0 = new CDol(o, l0);
    }
    return (l0);
}


// This will rebuild the database, calling process on every object.  If
// process returns false, it is not added to the new database (it is up
// to the user to free the object, it has been unlinked when process is
// called).
//
void
CDdb::db_rebuild(bool(*process)(CDo*, CDdb*, void*), void *arg)
{
    if (!process)
        return;

    unsigned int ntmp = db_layers_used;
    db_layers_used = 0;
    CDtree *ttmp = db_layer_heads;
    db_layer_heads = 0;

    for (unsigned int i = 0; i < ntmp; i++) {
        RTelem *list = ttmp[i].to_list();
        while (list) {
            RTelem *ln = 0;
            list = RTelem::list_next(list, &ln);
            if ((*process)((CDo*)list, this, arg))
                ttmp[i].insert(list);
            list = ln;
        }
    }
    db_layer_heads = ttmp;
    db_layers_used = ntmp;
}


// This function removes each object from sdb, calls process, and if
// process returns true inserts the object into this.
//
void
CDdb::db_merge(CDdb *sdb, bool(*process)(CDo*, CDdb*, CDdb*, void*),
    void *arg)
{
    if (!process || !sdb)
        return;

    unsigned int ntmp = sdb->db_layers_used;
    sdb->db_layers_used = 0;
    RTree *ttmp = sdb->db_layer_heads;
    sdb->db_layer_heads = 0;

    for (unsigned int i = 0; i < ntmp; i++) {
        RTelem *list = ttmp[i].to_list();
        while (list) {
            RTelem *ln = 0;
            list = RTelem::list_next(list, &ln);
            if ((*process)((CDo*)list, this, sdb, arg))
                db_insert((CDo*)list);
            list = ln;
        }
    }
    delete [] ttmp;
}


// Return the bounding box of all geometry and subcells, will be the
// cell's bounding box if subcell bounding boxes are correct.
//
void
CDdb::db_bb(BBox *BB) const
{
    *BB = CDnullBB;
    if (db_layer_heads) {
        for (unsigned int i = 0; i < db_layers_used; i++) {
            RTree *l = &db_layer_heads[i];
            if (l->num_elements() > 0 && l->root_bb())
                BB->add(l->root_bb());
        }
    }
}


// Return the number of objects in the database.
//
unsigned int
CDdb::db_objcnt() const
{
    unsigned int cnt = 0;
    if (db_layer_heads) {
        for (unsigned int i = 0; i < db_layers_used; i++)
            cnt += db_layer_heads[i].num_elements();
    }
    return (cnt);
}


// Print the bin counts (debugging).
//
void
CDdb::db_bincnt(const CDl *ld, int lev) const
{
    RTree *l = db_find_layer_head(ld);
    if (l) {
        l->test();
        if (lev >= 0)
            l->show(lev);
    }
}
// End of CDdb functions


//-----------------------------------------------------------------------------
// Object Retrieval Generator
//-----------------------------------------------------------------------------

CDg::CDg()
{
    container = 0;
    ldesc = 0;
    flags = 0;
}


CDg::~CDg()
{
    if (container)
        CDg_genTab.remove(this);
}


// Initialize the generator descriptor.  This *does not* account for
// the transformation stack, as does cTfmStack::TInitGen.
//
void
CDg::init_gen(const CDs *sdesc, const CDl *ld, const BBox *pBB)
{
    // If the passed BB covers the infinite BB, assume that the user
    // wants to cycle through all objects.  Setting this flag makes
    // this traversal more efficient.

    if (!pBB || (*pBB >= CDinfiniteBB))
        setup(sdesc, ld, &CDinfiniteBB, GEN_RET_ALL);
    else
        setup(sdesc, ld, pBB, 0);
    if (sdesc && sdesc->isElectrical())
        flags |= GEN_ELEC_MODE;
    else
        flags &= ~GEN_ELEC_MODE;
}


// Generator "next" function, returns pointer to next object.
//
CDo *
CDg::next()
{
    for (;;) {
        RTelem *rt;
        if (!(flags & GEN_RET_MASK))
            rt = next_element();
        else if (flags & GEN_RET_ALL)
            rt = next_element_nchk();
        else if (flags & GEN_RET_NOTOUCH)
            rt = next_element_notouch();
        else
            rt = next_element_exact();

        if (!rt) {
            clear();
            return (0);
        }
        if (flags & GEN_CLIP_MASK) {
            const BBox *xBB = &rt->oBB();
            if ((flags & GEN_CLIP_RIGHT) && xBB->left == BB.right)
                continue;
            if ((flags & GEN_CLIP_TOP) && xBB->bottom == BB.top)
                continue;
            if ((flags & GEN_CLIP_LEFT) && xBB->right == BB.left)
                continue;
            if ((flags & GEN_CLIP_BOTTOM) && xBB->top == BB.bottom)
                continue;
        }
        return ((CDo*)rt);
    }
}


void
CDg::setup(const CDdb *c, const CDl *ld, const BBox *xBB, int fg)
{
    if (c && ld) {
        RTree *rt = c->db_find_layer_head(ld);
        if (rt) {
            container = c;
            ldesc = ld;
            flags = fg & (GEN_CLIP_MASK | GEN_RET_MASK);
            init(rt, xBB);
            CDg_genTab.add(this);
        }
    }
}


void
CDg::clear()
{
    if (container)
        CDg_genTab.remove(this);
    container = 0;
    ldesc = 0;
    flags = 0;
    sp = 0;
}


// Static function;
void
CDg::print_gen_allocation()
{
    int pcnt = 0, ecnt = 0;
    SymTabGen gen(CDg_genTab.table());
    SymTabEnt *ent;
    while ((ent = gen.next()) != 0) {
        CDg *g = (CDg*)ent->stData;
        if (g->is_elec())
            ecnt++;
        else
            pcnt++;
    }
    fprintf(stderr,
        "Generators: alloc count Phys %d, Elec %d\n", pcnt, ecnt);
}
// End CDg functions


//-----------------------------------------------------------------------------
// Pseudo-Flat Generator
//-----------------------------------------------------------------------------

CDs *sPF::pf_skip_sdesc;
bool sPF::pf_skip_drc;

#define PFG_DONE (unsigned int)(-1)


// This should handle symbolic mode.  It will return the objects shown
// in a symbolic subcell, which makes sense for the Info command.

sPF::sPF(const CDs *sdesc, const BBox *AOI, const CDl *ldesc, int maxdepth)
{
    pf_gen = 0;
    pf_maxdepth = maxdepth;
    pf_info_mode_flag = 0;
    pf_boxes = true;
    pf_polys = true;
    pf_wires = true;
    pf_labels = false;
    pf_error = false;
    pf_dmode = sdesc ? sdesc->displayMode() : Physical;

    if (!init_gen(sdesc, AOI, ldesc))
        pf_error = true;
    sPF_genTab.add(this);
}


sPF::sPF(const CDc *cdesc, const BBox *AOI, const CDl *ldesc, int maxdepth)
{
    CDs *sdesc = cdesc ? cdesc->masterCell() : 0;

    pf_gen = 0;
    pf_maxdepth = maxdepth;
    pf_info_mode_flag = 0;
    pf_boxes = true;
    pf_polys = true;
    pf_wires = true;
    pf_labels = false;
    pf_error = false;
    pf_dmode = sdesc ? sdesc->displayMode() : Physical;

    if (!init_gen(&pf_gen, cdesc, AOI, ldesc))
        pf_error = true;
    sPF_genTab.add(this);
}


sPF::sPF(sPF &pf) : cTfmStack(pf)
{
    pf_maxdepth = pf.pf_maxdepth;
    pf_info_mode_flag = pf.pf_info_mode_flag;
    pf_boxes = pf.pf_boxes;
    pf_polys = pf.pf_polys;
    pf_wires = pf.pf_wires;
    pf_labels = pf.pf_labels;
    pf_error = pf.pf_error;
    pf_gen = pf_error ? 0 : sPFel::dup(pf.pf_gen);
    pf_dmode = pf.pf_dmode;
    sPF_genTab.add(this);
}


sPF::~sPF()
{
    sPF_genTab.remove(this);
    clear();
}


// If info_mode_flag is set to the initiating window expansion flag:
// 1)  label objects will be returned.
// 2)  unexpanded instances will be returned if ldesc is a cell layer,
//     will follow Peek Mode.
//
void
sPF::set_info_mode(int info_mode)
{
    pf_info_mode_flag = info_mode;
}


// Set the flags which allow objects of a given type to be returned. 
// Passing a null or empty string resets to defaults.  Otherwise, the
// string consists of characters b,p,w,l to enable boxes, polys,
// wires, labels, respectively.
//
void
sPF::set_returned(const char *str)
{
    if (str && isalpha(*str)) {
        pf_boxes = false;
        pf_polys = false;
        pf_wires = false;
        pf_labels = false;
        for (const char *s = str; *s; s++) {
            if (*s == 'b' || *s == 'B')
                pf_boxes = true;
            else if (*s == 'p' || *s == 'P')
                pf_polys = true;
            else if (*s == 'w' || *s == 'W')
                pf_wires = true;
            else if (*s == 'l' || *s == 'L')
                pf_labels = true;
        }
    }
    else {
        pf_boxes = true;
        pf_polys = true;
        pf_wires = true;
        pf_labels = false;
    }
}


namespace {
    // I haven't a clue why this is needed, but without it there is
    // geometric corruption.  For some reason it is necessary to lock
    // sPFel::advance and sPFel::init at the top level.

    pthread_mutex_t pf_mtx = PTHREAD_MUTEX_INITIALIZER;
}


// Generator function for pseudo-flat search.  If nocopy is false,
// this returns transformed copied odescs on each call, or 0 when
// done.  The next_odesc() returns a pointer to the actual odesc.  If
// nocopy is true, the raw odesc is returned, or 0 when done.  If
// touchok is true, objects that touch but do not overlap the AOI are
// returned.
//
CDo *
sPF::next(bool nocopy, bool touchok)
{
    if (!pf_gen)
        return (0);
    CDo *pointer = 0;
    for (;;) {
        if (pf_gen->el_sdesc == pf_skip_sdesc)
            pf_gen->el_lgdesc.clear();  // Don't return objects from this cell.
        if (pf_gen->el_lgdesc.is_active()) {
            TPush();
            TLoadCurrent(&pf_gen->el_tf);
            while ((pointer = pf_gen->el_lgdesc.next()) != 0) {
                if (!pointer->is_normal() ||
                        (pf_skip_drc && pointer->has_flag(CDnoDRC)))
                    continue;
                if (pointer->type() == CDBOX) {
                    if (pf_boxes && !pf_gen->bb_check(touchok, pointer->oBB()))
                        break;
                }
                else if (pointer->type() == CDPOLYGON) {
                    if (pf_polys && ((const CDpo*)pointer)->
                            po_intersect(&pf_gen->el_invAOI, touchok))
                        break;
                }
                else if (pointer->type() == CDWIRE) {
                    if (pf_wires && ((const CDw*)pointer)->
                            w_intersect(&pf_gen->el_invAOI, touchok))
                        break;
                }
                else if (pointer->type() == CDLABEL) {
                    if ((pf_labels || pf_info_mode_flag) &&
                            !pf_gen->bb_check(touchok, pointer->oBB()))
                        break;
                }
                else if (pointer->type() == CDINSTANCE) {
                    if (pf_info_mode_flag) {
                        if (pf_gen->bb_check(touchok, pointer->oBB()))
                            continue;
                        if (((CDc*)pointer)->masterCell(true)->isDevice())
                            break;
                        if (pf_gen->el_depth < pf_maxdepth)
                            // cell is expanded
                            continue;
                        if (pointer->has_flag(pf_info_mode_flag))
                            // cell is expanded
                            continue;
                        break;
                    }
                }
            }
            if (pointer) {
                if (!nocopy) {

                    // WARNING!
                    // This returns a BOX given a LABEL.  You'll have to
                    // check the original object to see the label.

                    pointer = pointer->copyObjectWithXform(this,
                        (pf_labels || pf_info_mode_flag));
                }
                TPop();
                return (pointer);
            }
            TPop();
        }

        if (pf_gen->el_next) {
            pf_gen = pf_gen->el_next;
            continue;
        }

        sPFel *genp = pf_gen->el_prev;
        if (genp)
            genp->el_next = 0;
        pf_gen->el_prev = 0;
        clear();
        pf_gen = genp;
        if (!pf_gen)
            return (0);
        pthread_mutex_lock(&pf_mtx);
        bool ret = pf_gen->advance(this);
        pthread_mutex_unlock(&pf_mtx);
        if (!ret) {
            clear();
            pf_error = true;
            return (0);
        }
        if (pf_gen->el_next)
            pf_gen = pf_gen->el_next;
    }
}


// The cdesc is about to be destroyed, purge its references, if any,
// from the generator.
//
void
sPF::purge(const CDc *cdesc)
{
    for (sPFel *g = pf_gen; g; g = g->el_prev) {
        if (g->el_cdesc == cdesc) {
            g->el_cdesc = 0;
            sPFel *tmp = g->el_next;
            g->el_next = 0;
            if (tmp)
                tmp->el_prev = 0;
            delete tmp;
            pf_gen = g;
            return;
        }
    }
    if (pf_gen) {
        for (sPFel *g = pf_gen->el_next; g; g = g->el_next) {
            if (g->el_cdesc == cdesc) {
                g->el_cdesc = 0;
                sPFel *tmp = g->el_next;
                g->el_next = 0;
                if (tmp)
                    tmp->el_prev = 0;
                delete tmp;
                return;
            }
        }
    }
}


// The sdesc is about to be destroyed, purge its references, if any,
// from the generator.
//
void
sPF::purge(const CDs *sdesc)
{
    for (sPFel *g = pf_gen; g; g = g->el_prev) {
        if (g->el_sdesc == sdesc) {
            g->el_cdesc = 0;
            sPFel *tmp = g->el_next;
            g->el_next = 0;
            if (tmp)
                tmp->el_prev = 0;
            delete tmp;
            g->el_sgdesc.clear();
            g->el_lgdesc.clear();
            pf_gen = g;
            return;
        }
    }
    if (pf_gen) {
        for (sPFel *g = pf_gen->el_next; g; g = g->el_next) {
            if (g->el_sdesc == sdesc) {
                g->el_cdesc = 0;
                sPFel *tmp = g->el_next;
                g->el_next = 0;
                if (tmp)
                    tmp->el_prev = 0;
                delete tmp;
                g->el_sgdesc.clear();
                g->el_lgdesc.clear();
                return;
            }
        }
    }
}


sPF*
sPF::dup()
{
    return (new sPF(*this));
}


// Special export for Info.
//
int
sPF::info_stack(CDcxy *st) const
{
    int sp = 0;
    for (sPFel *g = pf_gen->el_prev; g; g = g->el_prev) {
        if (!g->el_cdesc)
            // "can't happen"
            break;
        st[sp].cdesc = g->el_cdesc;
        st[sp].xind = g->el_x;
        st[sp].yind = g->el_y;
        sp++;
    }
    return (sp);
}


// Special export for DRC.
//
int
sPF::drc_stack(const CDc **st, int sp) const
{
    sPFel *g = pf_gen;
    if (g) {
        while (g->el_prev)
            g = g->el_prev;
        for ( ; g && g->el_cdesc; g = g->el_next) {
            st[sp++] = g->el_cdesc;
            if (g->el_lgdesc.is_active())
                break;
        }
    }
    return (sp);
}


// Initializer for pseudo-flat generator.  sdesc is the top level
// cell.  The AOI is the area to search.  ldesc is the layer to
// search.  The depth arg should be considered private.
//
bool
sPF::init_gen(const CDs *sdesc, const BBox *AOI, const CDl *ldesc)
{
    bool continue_recurse;
    if (pf_info_mode_flag)
        continue_recurse = (0 <= pf_maxdepth);
    else
        continue_recurse = (0 < pf_maxdepth);
    if (!sdesc)
        return (false);
    if (!ldesc)
        return (false);
    if (!AOI)
        AOI = sdesc->BB();
    if (TFull())
        return (false);

    pf_gen = new sPFel(sdesc, AOI, ldesc, 0);
    if (continue_recurse) {
        if (!TInitGen(sdesc, CellLayer(), AOI, &pf_gen->el_sgdesc)) {
            clear();
            return (false);
        }
    }
    else
        TInverse();

    TCurrent(&pf_gen->el_tf);
    pf_gen->el_invAOI = *AOI;
    TInverseBB(&pf_gen->el_invAOI, 0);

    if (!TInitGen(sdesc, ldesc, AOI, &pf_gen->el_lgdesc)) {
        clear();
        return (false);
    }

    if (continue_recurse) {
        pthread_mutex_lock(&pf_mtx);
        bool ret = pf_gen->init(this);
        pthread_mutex_unlock(&pf_mtx);
        if (!ret) {
            clear();
            return (false);
        }
    }
    return (true);
}


// As above, but called while traversing.
//
bool
sPF::init_gen(sPFel *el, const CDs *sdesc)
{
    int depth = el->el_depth + 1;
    bool continue_recurse;
    if (pf_info_mode_flag) {
        continue_recurse = (depth <= pf_maxdepth);
        if (!continue_recurse && el->el_cdesc &&
                el->el_cdesc->has_flag(pf_info_mode_flag))
            continue_recurse = true;
        else {
            if (depth > pf_maxdepth)
                return (true);
        }
    }
    else {
        continue_recurse = (depth < pf_maxdepth);
        if (depth > pf_maxdepth)
            return (true);
    }

    if (!sdesc)
        return (false);
    if (!el->el_ldesc)
        return (false);
    if (TFull())
        return (false);

    el->el_next = new sPFel(sdesc, &el->el_AOI, el->el_ldesc, depth);
    if (continue_recurse) {
        if (!TInitGen(sdesc, CellLayer(), &el->el_AOI,
                &el->el_next->el_sgdesc)) {
            delete el->el_next;
            el->el_next = 0;
            return (false);
        }
    }
    else
        TInverse();

    TCurrent(&el->el_next->el_tf);
    el->el_next->el_invAOI = el->el_AOI;
    TInverseBB(&el->el_next->el_invAOI, 0);

    if (!TInitGen(sdesc, el->el_ldesc, &el->el_AOI, &el->el_next->el_lgdesc)) {
        delete el->el_next;
        el->el_next = 0;
        return (false);
    }

    if (continue_recurse) {
        if (!el->el_next->init(this)) {
            delete el->el_next;
            el->el_next = 0;
            return (false);
        }
    }
    return (true);
}


// This function initializes the generator to return only objects in
// the hierarchy of the cell instanced by cdesc (and only those
// touching AOI).
//
bool
sPF::init_gen(sPFel **pfgp, const CDc *cdesc, const BBox *AOI,
    const CDl *ldesc)
{
    *pfgp = 0;
    if (!cdesc || !ldesc)
        return (false);
    if (!cdesc->parent())
        return (false);
    if (!AOI)
        AOI = cdesc->parent()->BB();
    sPFel *pfg = new sPFel(cdesc->parent(), AOI, ldesc, 0);
    TInverse();
    TCurrent(&pfg->el_tf);
    pfg->el_invAOI = *AOI;
    TInverseBB(&pfg->el_invAOI, 0);

    pfg->el_cdesc = cdesc;
    if (!pfg->init(this)) {
        delete pfg;
        return (false);
    }

    *pfgp = pfg;
    return (true);
}


// Static function;
void
sPF::print_gen_allocation()
{
    int pcnt = 0, ecnt = 0;
    SymTabGen gen(sPF_genTab.table());
    SymTabEnt *ent;
    while ((ent = gen.next()) != 0) {
        sPF *pf = (sPF*)ent->stData;
        if (pf->is_elec())
            ecnt++;
        else
            pcnt++;
    }
    fprintf(stderr,
        "PFgenerators: alloc count Phys %d, Elec %d\n", pcnt, ecnt);
}
// End of sPF functions.


sPFel::~sPFel()
{
    sPFel *g = this;
    if (g) {
        while (g->el_prev)
            g = g->el_prev;
    }
    while (g) {
        sPFel *gn = g->el_next;
        g->el_next = 0;
        g->el_prev = 0;
        if (g != this)
            delete g;
        g = gn;
    }
}


// Static function.
sPFel *
sPFel::dup(const sPFel *thispe)
{
    if (!thispe)
        return (0);
    const sPFel *t = thispe;
    while (t->el_prev)
        t = t->el_prev;
    sPFel *n0 = 0, *ne = 0;
    sPFel *newg = 0;
    while (t) {
        sPFel *n = new sPFel(*t);
        n->el_prev = n->el_next = 0;

        if (!n0)
            n0 = ne = n;
        else {
            n->el_prev = ne;
            ne->el_next = n;
            ne = n;
        }
        if (t == thispe)
            newg = n;
        t = t->el_next;
    }
    return (newg);
}


bool
sPFel::init(sPF *owner)
{
    el_next = 0;
    if (!owner)
        return (false);
    if (el_cdesc) {
        CDs *msdesc = el_cdesc->masterCell();
        owner->TPush();
        CDap ap(el_cdesc);
        int tx, ty;
        owner->TGetTrans(&tx, &ty);
        if (owner->TOverlapInstForLayer(el_cdesc, el_ldesc, &el_AOI,
                &el_x1, &el_x2, &el_y1, &el_y2, true)) {
            el_y = el_y2;
            el_x = el_x1;
            for (;;) {
                owner->TTransMult(el_x*ap.dx, el_y*ap.dy);
                if (!owner->init_gen(this, msdesc)) {
                    owner->TPop();
                    return (false);
                }
                owner->TSetTrans(tx, ty);
                if (el_next)
                    break;

                el_x++;
                if (el_x > el_x2) {
                    el_x = el_x1;
                    if (el_y == el_y1) {
                        el_y = PFG_DONE;
                        break;
                    }
                    el_y--;
                }
            }
        }
        owner->TPop();
    }
    else if (el_sgdesc.is_active()) {
        while ((el_cdesc = (CDc*)el_sgdesc.next()) != 0) {
            if (!el_cdesc->is_normal())
                continue;
            if (owner->get_skip_drc() && el_cdesc->has_flag(CDnoDRC))
                continue;
            CDs *msdesc = el_cdesc->masterCell();
            if (!msdesc || msdesc->isDevice())
                continue;

            owner->TPush();
            CDap ap(el_cdesc);
            int tx, ty;
            owner->TGetTrans(&tx, &ty);
            if (owner->TOverlapInstForLayer(el_cdesc, el_ldesc,
                    &el_AOI, &el_x1, &el_x2, &el_y1, &el_y2, true)) {
                el_y = el_y2;
                el_x = el_x1;
                for (;;) {
                    owner->TTransMult(el_x*ap.dx, el_y*ap.dy);
                    if (!owner->init_gen(this, msdesc)) {
                        owner->TPop();
                        return (false);
                    }
                    owner->TSetTrans(tx, ty);
                    if (el_next)
                        break;

                    el_x++;
                    if (el_x > el_x2) {
                        el_x = el_x1;
                        if (el_y == el_y1) {
                            el_y = PFG_DONE;
                            break;
                        }
                        el_y--;
                    }
                }
            }
            owner->TPop();
            if (el_next)
                break;
        }
    }

    if (!el_next) {
        el_sgdesc.clear();
        el_cdesc = 0;
    }
    else
        el_next->el_prev = this;
    return (true);
}


bool
sPFel::advance(sPF *owner)
{
    if (!owner)
        return (false);
    owner->TPush();
    owner->TLoadCurrent(&el_tf);
    if (el_cdesc) {
        if (el_y != PFG_DONE) {
            el_x++;
            if (el_x > el_x2) {
                el_x = el_x1;
                if (el_y == el_y1)
                    el_y = PFG_DONE;
                else
                    el_y--;
            }
            if (el_y != PFG_DONE) {
                CDs *msdesc = el_cdesc->masterCell();
                owner->TPush();
                owner->TApplyTransform(el_cdesc);
                owner->TPremultiply();
                CDap ap(el_cdesc);
                int tx, ty;
                owner->TGetTrans(&tx, &ty);
                for (;;) {
                    owner->TTransMult(el_x*ap.dx, el_y*ap.dy);
                    if (!owner->init_gen(this, msdesc)) {
                        owner->TPop();
                        owner->TPop();
                        return (false);
                    }
                    owner->TSetTrans(tx, ty);
                    if (el_next)
                        break;
                    el_x++;
                    if (el_x > el_x2) {
                        el_x = el_x1;
                        if (el_y == el_y1) {
                            el_y = PFG_DONE;
                            break;
                        }
                        el_y--;
                    }
                }
                owner->TPop();
            }
        }
        if (el_next) {
            el_next->el_prev = this;
            owner->TPop();
            return (true);
        }
        el_cdesc = 0;
    }
    bool ret = init(owner);
    owner->TPop();
    return (ret);
}
// End of Generators

