
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

#include "fio.h"
#include "fio_info.h"
#include "fio_chd.h"
#include "fio_chd_ecf.h"
#include "timedbg.h"


//
// CVecFilt: class to mark cells that are empty due to layer filtering.
//

// Only one CVecFile can be active.
//
bool CVecFilt::ec_lock;


// If layer filtering is enabled, and the passed CHD was built with
// cvINFOplpc, use the info counts to mark cells that are empty due to
// the layer filtering.  This is recursive, i.e., parents containing
// only empty cells are also empty.
//
// On successful exit, the CVemty flag is set in all empty symrefs in
// the hierarchy, which is traversed across different CHDs if there
// are symrefs without "defseen" set.
//
// The flags are reset by the destructor, or with the unsetup method.
//
bool
CVecFilt::setup(cCHD *chd, symref_t *p)
{
    TimeDbg tdbg("empty cell prefiltering");

    if (!chd || !p) {
        Errs()->add_error("setup: null symref or CHD pointer.\n");
        return (false);
    }
    if (ec_lock)
        return (true);

    // Check layer filtering, make sure that the passed CHD has
    // per-cell and per-layer object counts.
    //
    bool skip_layers = false;
    stringlist *sl = 0;
    cv_info *info = chd->pcInfo(p->mode());
    if (cv_info::savemode(info) == cvINFOplpc) {
        ULLtype ull = FIO()->UseLayerList();
        if (ull == ULLnoList) {
            ec_lock = true;
            return (true);
        }
        skip_layers = (ull == ULLskipList);
        const char *ll = FIO()->LayerList();
        if (ll) {
            char *tok;
            while ((tok = lstring::gettok(&ll)) != 0)
                sl = new stringlist(tok, sl);
        }
    }
    if (!sl) {
        ec_lock = true;
        return (true);
    }

    // Obtain an info_lnames_t for the layer name list.  This replaces
    // the layer names with string table entries.
    //
    ec_ifln = info->get_info_lnames(sl);
    if (!ec_ifln) {
        ec_lock = true;
        return (true);
    }
    ec_skip_layers = skip_layers;

    // Traverse the hierarchy and load cells into the hash table. 
    // During the traverse, identify the empty cells and set the
    // CVemty flags in the symrefs.
    //
    ec_table = new ptrtab_t;
    bool ret = load_table_rc(chd, p);
    delete ec_ifln;
    ec_ifln = 0;
    ec_skip_layers = false;
    if (!ret) {
        if (ec_num_empty) {
            // Clear the flags.
            ptrgen_t gen(ec_table);
            symref_t *px;
            while ((px = (symref_t*)gen.next()) != 0)
                px->set_emty(false);
            ec_num_empty = 0;
        }
        delete ec_table;
        ec_table = 0;
        return (false);
    }

    ec_lock = true;

    // If empty cells were found, save these symrefs in an array so we
    // can easily reset the flags when done.
    //
    if (ec_num_empty) {
        Tdbg()->save_message("marked %d empty cells", ec_num_empty);
        ec_symrefs = new symref_t*[ec_num_empty];
        ptrgen_t gen(ec_table);
        unsigned int cnt = 0;
        symref_t *px;
        while ((px = (symref_t*)gen.next()) != 0) {
            if (px->get_emty())
                ec_symrefs[cnt++] = px;
        }
    }

    // Delete the hash table, we're done with it.
    //
    delete ec_table;
    ec_table = 0;
    return (true);
}


// If CVemty flags have been set in the symref hierarchy passed to
// setup, unset the flags, also clear the lock The setup method can be
// called again if desired.
//
void
CVecFilt::unsetup()
{
    if (ec_num_empty && ec_symrefs) {
        for (unsigned int i = 0; i <  ec_num_empty; i++)
            ec_symrefs[i]->set_emty(false);
        delete [] ec_symrefs;
    }
    ec_symrefs = 0;
    ec_num_empty = 0;
    ec_lock = false;
}


// Private function to recursively load the symrefs used in the
// hierarchy into the hash table.  Since the ordering in intrinsically
// bottom-up, we can do empty cell testing on the same pass.  For
// empty cells, we set the CVemty flag in the symref.
//
bool
CVecFilt::load_table_rc(cCHD *chd, symref_t *p)
{
    nametab_t *ntab = chd->nameTab(p->mode());
    if (!ntab)
        return (true);
    bool has_sc = false;
    ticket_t last_srfptr = 0;
    crgen_t gen(ntab, p);
    const cref_o_t *c;
    while ((c = gen.next()) != 0) {
        if (c->srfptr == last_srfptr)
            continue;
        last_srfptr = c->srfptr;
        symref_t *cp = ntab->find_symref(c->srfptr);
        if (!cp) {
            Errs()->add_error(
                "load_table_rc: unresolved instance symref back-pointer.");
            return (false);
        }

        cCHD *tchd = chd;
        if (!cp->get_defseen()) {
            RESOLVtype rsv = FIO()->ResolveUnseen(&tchd, &cp);
            if (rsv == RESOLVerror)
                return (false);
            if (rsv == RESOLVnone)
                continue;
        }
        if (cp->should_skip())
            continue;

        if (ec_table->find(cp)) {
            has_sc = true;
            continue;
        }
        if (!load_table_rc(tchd, cp))
            return (false);

        if (!cp->get_emty())
            has_sc = true;
    }

    ec_table->add(p);
    if (!has_sc) {
        cv_info *info = chd->pcInfo(p->mode());
        if (info && !info->has_geom(p, ec_ifln, ec_skip_layers)) {
            p->set_emty(true);
            ec_num_empty++;
        }
    }
    return (true);
}

