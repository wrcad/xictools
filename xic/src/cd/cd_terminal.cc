
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
#include "cd_propnum.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "cd_celldb.h"
#include <ctype.h>


//-----------------------------------------------------------------------------
// CDterm
//
// Used to track physical location assignment of terminals.

namespace {
    // Return false if an instance is found that has a different set
    // of node indices than the master.  Check that the corresponding
    // master/instance properties either both have physical terminals,
    // or neither has terminals.  This will create/destroy terminals
    // as necessary.
    //
    bool checknodes(CDm *mdesc)
    {
        CDs *msd = mdesc->celldesc();
        if (!msd->isElectrical())
            return (true);

        CDp_snode **node_ary;
        unsigned int asz = msd->checkTerminals(&node_ary);
        if (!asz)
            return (true);

        // Subcircuits and devices with a physical part will have
        // physical terminals assigned at this point, however library
        // devices and similar probably do not.  These will be added
        // here,
        
        CDelecCellType tp = msd->elecCellType();
        if (tp == CDelecDev || tp == CDelecMacro || tp == CDelecSubc) {
            if (msd->prpty(P_NOPHYS)) {
                // Should not have physical terminals.
                for (unsigned int i = 0; i < asz; i++) {
                    CDp_snode *ps = node_ary[i];
                    if (!ps)
                        continue;
                    if (ps->cell_terminal()) {
                        delete ps->cell_terminal();
                        ps->set_terminal(0);  // redundant
                    }
                }
            }
            else {
                for (unsigned int i = 0; i < asz; i++) {
                    CDp_snode *ps = node_ary[i];
                    if (!ps)
                        continue;
                    if (ps->flags() & TE_NOPHYS) {
                        // Shouldn't have a phys terminal, if one is
                        // found get rid of it.

                        if (ps->cell_terminal()) {
                            delete ps->cell_terminal();
                            ps->set_terminal(0);  // redundant
                        }
                    }
                    else {
                        // Create a physical terminal if none exists. 
                        // This will allow instantiation of the terminal
                        // in the physical layout.

                        if (!ps->cell_terminal()) {
                            CDs *psd = CDcdb()->findCell(msd->cellname(),
                                Physical);
                            CDsterm *st;
                            if (psd)
                                st = psd->findPinTerm(ps->term_name(), true);
                            else {
                                st = new CDsterm(0, ps->term_name());
                                ps->set_flag(TE_OWNTERM);
                            }
                            ps->set_terminal(st);
                            st->set_uninit(true);
                            st->set_node_prpty(ps);
                        }
                    }
                }
            }
        }
        else {
            // A schematic pin or something strange, should not have
            // physical terminals.

            for (unsigned int i = 0; i < asz; i++) {
                CDp_snode *ps = node_ary[i];
                if (!ps)
                    continue;
                if (ps->cell_terminal()) {
                    delete ps->cell_terminal();
                    ps->set_terminal(0);  // redundant
                }
            }
        }

        CDp_cnode **cnode_ary = new CDp_cnode*[asz];
        GCarray<CDp_snode**> gc1(node_ary);
        GCarray<CDp_cnode**> gc2(cnode_ary);

        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
            CDgenRange rgen(pr);
            int vec_ix = 0;

            // If the index is vectored, only the 0'th index is
            // significant.  However, if node property copies exist,
            // we will enforce consistency of physical terminal
            // existence.

            while (rgen.next(0)) {
                memset(cnode_ary, 0, asz*sizeof(CDp_cnode*));
                if (vec_ix == 0) {
                    CDp_cnode *pc = (CDp_cnode*)cdesc->prpty(P_NODE);
                    for ( ; pc; pc = pc->next()) {
                        if (pc->index() >= asz) {
                            // Index not in range!
                            return (false);
                        }
                        if (!cnode_ary[pc->index()])
                            cnode_ary[pc->index()] = pc;
                        else {
                            // Duplicate index!
                            return (false);
                        }
                    }
                }
                else {
                    CDp_cnode *pc = (CDp_cnode*)cdesc->prpty(P_NODE);
                    for ( ; pc; pc = pc->next()) {
                        unsigned int ix = pc->index();
                        CDp_cnode *pc1 = pr->node(0, vec_ix, ix);
                        if (!pc1) {
                            // The range property has not been set up
                            // yet, which is ok.

                            return (true);
                        }
                        cnode_ary[ix] = pc1;
                    }
                }
                for (unsigned int i = 0; i < asz; i++) {
                    CDp_snode *ps = node_ary[i];
                    CDp_cnode *pc = cnode_ary[i];
                    if (ps && pc) {
                        if (ps->cell_terminal() && !pc->inst_terminal()) {
                            pc->set_terminal(new CDcterm(pc));
                            pc->inst_terminal()->set_instance(cdesc, vec_ix);
                        }
                        else if (!ps->cell_terminal() && pc->inst_terminal()) {
                            // Destructor unlinks from pc.
                            delete pc->inst_terminal();
                        }
                        continue;
                    }
                    if (ps) {
                        // Instance is missing a node.
                        return (false);
                    }
                    if (pc) {
                        // Instance has extra node.
                        return (false);
                    }
                }
                vec_ix++;
            }
        }
        return (true);
    }
}


// Check consistency and create/update as necessary the physical
// terminal structs.
//
// The 'this' can be physical or electrical.
//
// Cell terminals are created explicitly when reading input or in the
// subct command, but only when a physical part of the cell is
// present.  Terminals will be added here if missing and there is no
// electrical part.  These are dummies, but trigger the creation of
// instance terminals, which are actually placed in the design if the
// device is used.
//
// Instance terminals are created here, exclusively.
//
// On error, continue but return false, the error system will have
// a message.
//
bool
CDs::checkPhysTerminals(bool cterms_only)
{
    CDs *sd = this;
    if (sd->isElectrical()) {
        if (sd->owner())
            sd = sd->owner();
    }
    else {
        sd = CDcdb()->findCell(sd->cellname(), Electrical);
        if (!sd)
            return (true);
    }

    if (sd->elecCellType() != CDelecSubc)
        return (true);

    sd->checkTerminals();

    if (cterms_only)
        return (true);

    bool ret = true;
    CDm_gen mgen(sd, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {

        CDs *msd = mdesc->celldesc();
        if (!msd)
            continue;
        switch (msd->elecCellType()) {
        case CDelecDev:
        case CDelecMacro:
        case CDelecSubc:
            break;
        default:
            continue;
        }

        if (!checknodes(mdesc)) {
            // Must be a node property inconsistency, calling
            // reflectTerminals should fix it.
            msd->reflectTerminals();
            if (!checknodes(mdesc)) {
                // Failed again?  Something is screwed up seriously.
                Errs()->add_error(
                    "Internal error: checknodes %s failed twice in %s",
                    Tstring(msd->cellname()), Tstring(cellname()));
                ret = false;
            }
        }
    }
    return (ret);
}
// End of CDs functions.


// Set the terminal to reference od, and maintain a back pointer as an
// internal property in od.
//
void
CDterm::set_ref(CDo *od)
{
    if (t_oset) {
        // remove property from oset
        for (CDp *pd = t_oset->prpty_list(); pd; pd = pd->next_prp()) {
            if (pd->value() == XICP_TERM_OSET &&
                    ((CDp_oset*)pd)->term() == this) {
                t_oset->prptyUnlink(pd);
                delete pd;
                break;
            }
        }
    }
    t_oset = od;
    t_vgroup = -1;
    // don't change ldesc if od == 0
    if (od) {
        t_ldesc = od->ldesc();
        t_flags &= ~TE_UNINIT;
        od->link_prpty_list(new CDp_oset(this));
    }
    else
        t_flags |= TE_UNINIT;
}


// Set as a "virtual" terminal, meaning that there is no suitable
// geometry to reference.  The terminal may indeed reference nothing,
// or it may reference a group by virtue of being placed over a
// connected group in a subcell.  In the latter case, the t_ldesc is
// important.
//
void
CDterm::set_v_group(int g)
{
    t_vgroup = g;
    if (t_oset) {
        // remove property from oset
        for (CDp *pd = t_oset->prpty_list(); pd; pd = pd->next_prp()) {
            if (pd->value() == XICP_TERM_OSET &&
                    ((CDp_oset*)pd)->term() == this) {
                t_oset->prptyUnlink(pd);
                delete pd;
                break;
            }
        }
    }
    t_oset = 0;
    if (g < 0)
        t_flags |= TE_UNINIT;
    else
        t_flags &= ~TE_UNINIT;
}
// End of CDterm functions.


// Given a device or subcircuit terminal, return the name of the
// corresponding terminal in the cell desc.
//
CDnetName
CDcterm::master_name() const
{
    if (t_cdesc && t_node) {
        CDs *sd = t_cdesc->masterCell(true);
        if (sd) {
            unsigned int ix = t_node->index();
            CDp_snode *ps = (CDp_snode*)sd->prpty(P_NODE);
            for ( ; ps; ps = ps->next()) {
                if (ps->index() == ix)
                    return (ps->term_name());
            }
        }
    }
    return (undef_name());
}


// Return the corresponding master cell terminal of this, which is an
// instance terminal.
//
CDsterm *
CDcterm::master_term() const
{
    if (!t_cdesc || !t_node)
        return (0);
    CDs *sdp = t_cdesc->masterCell();
    if (sdp) {
        unsigned int ix = t_node->index();
        CDp_snode *pn = (CDp_snode*)sdp->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            if (pn->index() == ix)
                return (pn->cell_terminal());
        }
    }
    return (0);
}


// Return the corresponding master cell group number of this, which is
// an instance terminal.
//
int
CDcterm::master_group() const
{
    if (!t_cdesc || !t_node)
        return (-1);
    CDs *sdp = t_cdesc->masterCell();
    if (sdp) {
        unsigned int ix = t_node->index();
        CDp_snode *pn = (CDp_snode*)sdp->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            if (pn->index() == ix) {
                CDsterm *t = pn->cell_terminal();
                return (t ? t->group() : -1);
            }
        }
    }
    return (-1);
}


// Static function.
CDnetName
CDcterm::undef_name()
{
    return (CDnetex::undef_name());
}
// End CDcterm functions

