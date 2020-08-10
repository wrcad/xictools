
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

#include "dsp.h"
#include "tech.h"
#include "tech_layer.h"
#include "tech_via.h"
#include "cd_lgen.h"
#include "si_parsenode.h"
#include "si_lspec.h"
#include "drcif.h"


// Layer sequencer debugging.
// #ifdef SEQDEBUG

namespace {
    // A doubly-linked list for sequencing layers, used in
    // cTech::sequenceLayers.
    //
    struct CDllp
    {
        CDllp(CDl *ld, CDllp *n)
            {
                next = n;
                prev = 0;
                ldesc = ld;
            }

        static void destroy(const CDllp *p)
            {
                while (p) {
                    const CDllp *px = p;
                    p = p->next;
                    delete px;
                }
            }

        CDl *bottom_layer();
        CDl *top_layer();

        static bool insert_layer(CDllp **, CDllp*);

        CDllp *next;
        CDllp *prev;
        CDl *ldesc;
    };


    // If this is a VIA, return the bottom layer implied by the sVia
    // list.  On error, return 0.
    //
    CDl *
    CDllp::bottom_layer()
    {
        if (ldesc->isVia()) {
            sVia *vl0 = tech_prm(ldesc)->via_list();
            if (!vl0)
                return (0);
            CDl *lbot = vl0->bottom_layer();
            if (!lbot)
                return (0);
            for (sVia *vl = vl0->next(); vl; vl = vl->next()) {
                CDl *tb = vl->bottom_layer();
                if (!tb)
                    return (0);
                if (tb->physIndex() > lbot->physIndex())
                    lbot = tb;
            }
            return (lbot);
        }
        return (0);
    }


    // If this is a VIA, return the top layer implied by the sVia
    // list.  On error, return 0.
    //
    CDl *
    CDllp::top_layer()
    {
        if (ldesc->isVia()) {
            sVia *vl0 = tech_prm(ldesc)->via_list();
            if (!vl0)
                return (0);
            CDl *ltop = vl0->top_layer();
            if (!ltop)
                return (0);
            for (sVia *vl = vl0->next(); vl; vl = vl->next()) {
                CDl *tt = vl->top_layer();
                if (!tt)
                    return (0);
                if (tt->physIndex() < ltop->physIndex())
                    ltop = tt;
            }
            return (ltop);
        }
        return (0);
    }


    // Static function.
    // Insert the passed VIA layer to the expected position, per the
    // present reordering mode.  We know that lx is not already in the
    // list.
    //
    bool
    CDllp::insert_layer(CDllp **pstack, CDllp *lx)
    {
        if (!lx || !lx->ldesc->isVia())
            return (true);
        if (Tech()->ReorderMode() == tReorderNone) {
            // Insert in layer table order.
            int ix = lx->ldesc->physIndex();
            for (CDllp *l = *pstack; l; l = l->next) {
                if (l->ldesc->physIndex() < ix && (!l->next ||
                        l->next->ldesc->physIndex() > ix)) {
                    lx->next = l->next;
                    if (lx->next)
                        lx->next->prev = lx;
                    l->next = lx;
                    lx->prev = l;
                    return (true);
                }
            }
            if (!*pstack || ix < (*pstack)->ldesc->physIndex()) {
                // Must be true, but makes no sense for Via layer to be
                // on the bottom.
                lx->next = *pstack;
                if (lx->next)
                    lx->next->prev = lx;
                *pstack = lx;
                lx->prev = 0;
            }
            return (true);
        }
        if (Tech()->ReorderMode() == tReorderVabove) {
            CDl *lbot = lx->bottom_layer();
            if (!lbot) {
                // Makes no sense for a Via layer to be on the bottom,
                // but that is the interpretation.
                lx->next = *pstack;
                if (lx->next)
                    lx->next->prev = lx;
                *pstack = lx;
                lx->prev = 0;
                return (true);
            }
            for (CDllp *l = *pstack; l; l = l->next) {
                if (l->ldesc == lbot) {
                    lx->next = l->next;
                    if (lx->next)
                        lx->next->prev = lx;
                    l->next = lx;
                    lx->prev = l;
                    return (true);
                }
            }
            return (false);
        }
        if (Tech()->ReorderMode() == tReorderVbelow) {
            CDl *ltop = lx->top_layer();
            if (!ltop) {
                // Makes no sense for a Via layer to be on the top, but
                // that is the interpretation.
                if (!*pstack) {
                    *pstack = lx;
                    lx->next = 0;
                    lx->prev = 0;
                    return (true);
                }
                CDllp *l = *pstack;
                while (l->next)
                    l = l->next;
                l->next = lx;
                lx->next = 0;
                lx->prev = l;
                return (true);
            }
            for (CDllp *l = *pstack; l; l = l->next) {
                if (l->ldesc == ltop) {
                    lx->next = l;
                    if (l->prev) {
                        lx->prev = l->prev;
                        l->prev->next = lx;
                    }
                    else {
                        *pstack = lx;
                        lx->prev = 0;
                    }
                    l->prev = lx;
                    return (true);
                }
            }
        }
        return (false);
    }
}


// For each layer in the layer table, call the user's filter function. 
// If the function returns true, the layer will be retained in an
// ordered list.  The ordering is performed accordint to the current
// ordering mode This will put layers referenced by vias in correct
// physical order, from substrate and up.
//
// On error, null is returned, with a message in the Errs system.
//
CDll *
cTech::sequenceLayers(bool(*filt_func)(const CDl*))
{
    Errs()->init_error();
    if (!filt_func) {
        Errs()->add_error("sequenceLayers: null filter function.");
        return (0);
    }

    // Create a list of layers, in layer-table order, that pass the
    // user's criteria.

    CDllp *stack = 0, *end = 0;
    CDl *ld;
    CDlgen lgen(Physical);
    while ((ld = lgen.next()) != 0) {
        if ((*filt_func)(ld)) {
#ifdef SEQDEBUG
            printf("saved %s\n", ld->name());
#endif
            if (!stack)
                stack = end = new CDllp(ld, 0);
            else {
                end->next = new CDllp(ld, 0);
                end->next->prev = end;
                end = end->next;
            }
        }
    }

    if (Tech()->ReorderMode() != tReorderNone) {

        // The sequence of conductors and DIELECTRIC layers is in
        // layer table order, starting at the substrate and moving up. 
        // This is the present list order.  If db3reorderNone, VIA
        // layers are also taken in layer table order.
        //
        // If tReorderVabove, VIA layers will be placed just above
        // the lower reference with the largest layer table index.  We
        // allow these layers to be out of deposition order in the
        // layer table to facilitate visibility in the drawing
        // windows.
        //
        // If tReorderVbelow, VIA layers will be placed just below
        // the upper reference with the smallest layer table index.

        CDllp *lend = 0;
        CDllp *lp = 0;
        CDllp *ln;
        for (CDllp *l = stack; l; l = ln) {
            ln = l->next;
            if (l->ldesc->isVia()) {
                bool badord = false;
                if (Tech()->ReorderMode() == tReorderVabove) {
                    badord = (!l->prev ||
                        l->bottom_layer() != l->prev->ldesc);
                }
                else if (Tech()->ReorderMode() == tReorderVbelow) {
                    badord = (!l->next ||
                        l->top_layer() != l->next->ldesc);
                }

                if (badord) {
                    // Out of order, clip it out of the list.
                    if (ln)
                        ln->prev = lp;
                    if (lp)
                        lp->next = ln;
                    else
                        stack = ln;
                    l->next = lend;
                    l->prev = 0;
                    lend = l;
                    continue;
                }
            }
            lp = l;
        }
        while (lend) {
            // Put any via layers back, in our physical order.

            CDllp *lx = lend;
            lend = lend->next;
            CDllp::insert_layer(&stack, lx);
        }
    }
    if (!stack) {
        Errs()->add_error("No suitable layers found to sequence.");
        return (0);
    }

    CDll *l0 = 0, *le = 0;
    while (stack) {
#ifdef SEQDEBUG
        printf("in order %s\n", stack->ldesc->name());
#endif
        if (!l0)
            l0 = le = new CDll(stack->ldesc, 0);
        else {
            le->next = new CDll(stack->ldesc, 0);
            le = le->next;
        }
        end = stack;
        stack = stack->next;
        delete end;
    }
    return (l0);
}
// End of cTech functions.


TechLayerParams::TechLayerParams(CDl*)
{
    lp_rules            = 0;

    lp_pinld            = 0;
    lp_exclude          = 0;
    lp_vialist          = 0;

    lp_rho              = 0.0;
    lp_ohms_per_sq      = 0.0;

    lp_epsrel           = 0.0;
    lp_cap_per_area     = 0.0;
    lp_cap_per_perim    = 0.0;

    lp_lambda           = 0.0;
    lp_gp_lname         = 0;
    lp_diel_thick       = 0.0;
    lp_diel_const       = 0.0;

    lp_fh_nhinc         = 0;
    lp_fh_rh            = 0.0;

    lp_ant_ratio        = 0.0;

    lp_route_dir        = tDirNone;
    lp_route_h_pitch    = 0;
    lp_route_v_pitch    = 0;
    lp_route_h_offset   = 0;
    lp_route_v_offset   = 0;
    lp_route_width      = 0;
    lp_route_max_dist   = 0;
    lp_spacing          = 0;
}


TechLayerParams::~TechLayerParams()
{
    DrcIf()->deleteRules(&lp_rules);
    delete lp_exclude;
    sVia::destroy(lp_vialist);
    delete [] lp_gp_lname;
}
// End of TechLayerParams functions.

