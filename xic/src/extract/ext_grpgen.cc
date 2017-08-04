
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

#include "main.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_grpgen.h"
#include "ext_errlog.h"
#include "edit.h"
#include "sced.h"
#include "cd_terminal.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "cd_propnum.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "geo_zlist.h"
#include "tech_layer.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_lexpr.h"
#include "events.h"
#include "menu.h"
#include "ext_menu.h"
#include "layertab.h"
#include "promptline.h"
#include "select.h"
#include "timer.h"
#include <algorithm>


namespace ext_group {

    // Special generator functions that return only objects used in the
    // group.  This relies on setting the group number to a negative
    // value in objects which are excluded from grouping.
    //
    void
    sGrpGen::init_gen(const cGroupDesc *gd, const CDl *ld, const BBox *BB,
        cTfmStack *tstk)
    {
        if (tstk)
            tstk->TInitGen(gd->cell_desc(), ld, BB, &gg_gdesc);
        else
            gg_gdesc.init_gen(gd->cell_desc(), ld, BB);

        gg_g_sdesc = gd->g_phony();
        gg_did_g = !gg_g_sdesc;
        if (gg_g_sdesc) {
            if (tstk)
                tstk->TInitGen(gg_g_sdesc, ld, BB, &gg_g_gdesc);
            else
                gg_g_gdesc.init_gen(gg_g_sdesc, ld, BB);
        }

        gg_e_sdesc = gd->e_phony();
        gg_did_e = !gg_e_sdesc;
        if (gg_e_sdesc) {
            if (tstk)
                tstk->TInitGen(gg_e_sdesc, ld, BB, &gg_e_gdesc);
            else
                gg_e_gdesc.init_gen(gg_e_sdesc, ld, BB);
        }
    }


    // The retrieval function, cycle through the phonies, then the grouped
    // normal objects.
    //
    CDo *
    sGrpGen::next()
    {
        CDo *odesc;
        if (!gg_did_g) {
            while ((odesc = gg_g_gdesc.next()) != 0) {
                if (odesc->group() >= 0)
                    return (odesc);
            }
            gg_did_g = true;
        }
        if (!gg_did_e) {
            while ((odesc = gg_e_gdesc.next()) != 0) {
                if (odesc->group() >= 0)
                    return (odesc);
            }
            gg_did_e = true;
        }
        while ((odesc = gg_gdesc.next()) != 0) {
            if (odesc->type() == CDLABEL)
                continue;
            if (!odesc->is_normal())
                continue;
            if (odesc->group() >= 0)
                return (odesc);
        }
        return (0);
    }
    // End of sGrpGen functions.


    // Create a new generator to start at the point of the one passed.
    //
    sSubcGen::sSubcGen(const sSubcGen &cg)
    {
        cg_cur = cg.cg_cur;
        cg_stack = 0;
        sTF *es = 0;
        for (sTF *t = cg.cg_stack; t; t = t->next) {
            if (cg_stack == 0)
                cg_stack = es = new sTF(*t);
            else {
                es->next = new sTF(*t);
                es = es->next;
            }
            es->next = 0;
        }
        cg_list = 0;
        sLL *el = 0;
        for (sLL *l = cg.cg_list; l; l = l->next) {
            if (cg_list == 0)
                cg_list = el = new sLL(*l);
            else {
                el->next = new sLL(*l);
                el = el->next;
            }
            el->next = 0;
        }
        cg_depth = cg.cg_depth;
    }


    // Advance the generator to the next subcircuit, if skipsubs is true,
    // don't delve into the subcircuits.
    //
    void
    sSubcGen::advance(bool skipsubs)
    {
        if (cg_cur) {
            if (cg_cur->subs && !skipsubs) {
                if (cg_cur->next)
                    cg_list = new sLL(cg_cur->next, cg_depth, cg_list);
                cg_cur = cg_cur->subs;
                pushxf();
            }
            else if (cg_cur->next) {
                popxf();
                cg_cur = cg_cur->next;
                pushxf();
            }
            else {
                if (cg_list) {
                    while (cg_depth >= cg_list->depth)
                        popxf();
                    cg_cur = cg_list->cl;
                    pushxf();
                    sLL *s = cg_list->next;
                    delete cg_list;
                    cg_list = s;
                }
                else {
                    while (cg_depth > 0)
                        popxf();
                    cg_cur = 0;
                }
            }
        }
    }


    // Advance to the first element whose topmost ancestor is different.
    //
    void
    sSubcGen::advance_top()
    {
        const sSubcLink *ct = toplink();
        do {
            advance(true);
        }
        while (toplink() == ct);
    }


    // Get the lev'th element of the stack, the 0'th element is the current
    // sSubcLink.
    //
    const sSubcLink *
    sSubcGen::getlink(int lev) const
    {
        sTF *s = cg_stack;
        while (lev-- && s->next)
            s = s->next;
        return (s->cl);
    }


    // Get the top sSubcLink in the stack.
    //
    const sSubcLink *
    sSubcGen::toplink() const
    {
        sTF *s = cg_stack;
        while (s->next && s->next->cl)
            s = s->next;
        return (s->cl);
    }


    // Print the current sSubcLink (debugging).
    //
    void
    sSubcGen::print() const
    {
        for (sTF *t = cg_stack; t; t = t->next) {
            if (t->cl)
                printf("%s(%d,%d)\\", Tstring(t->cl->cdesc->cellname()),
                    t->cl->ix, t->cl->iy);
        }
        printf("\n");
    }


    // Push the transform stack.
    //
    void
    sSubcGen::pushxf()
    {
        if (cg_cur) {
            cTfmStack stk;
            stk.TPush();
            stk.TLoadCurrent(&cg_stack->tf);
            stk.TPush();
            stk.TApplyTransform(cg_cur->cdesc);
            stk.TPremultiply();
            CDap ap(cg_cur->cdesc);
            stk.TTransMult(cg_cur->ix*ap.dx, cg_cur->iy*ap.dy);
            cg_stack = new sTF(cg_cur, cg_stack);
            stk.TCurrent(&cg_stack->tf);
            stk.TPop();
            stk.TPop();
            cg_depth++;
        }
    }


    // Pop the transform stack.
    //
    void
    sSubcGen::popxf()
    {
        if (cg_depth) {
            sTF *t = cg_stack->next;
            delete cg_stack;
            cg_stack = t;
            cg_depth--;
        }
    }
    // End of sSubcGen functions.
}


sSubcLink *
cGroupDesc::build_links(cTfmStack *tstk, const BBox *AOI)
{
    if (tstk->TFull())
        return (0);

    sSubcLink *c0 = 0, *ce = 0;
    CDm_gen mgen(gd_celldesc, GEN_MASTERS);
    for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
        CDs *msdesc = m->celldesc();
        if (!msdesc)
            continue;
        // Ignore connectors, these have been accounted for in combine().
        if (msdesc->isConnector())
            continue;
        cGroupDesc *gd = msdesc->groups();
        if (!gd)
            continue;

        CDc_gen cgen(m);
        for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
            if (!c->is_normal())
                continue;
            if (in_flatten_list(c))
                continue;
            if (in_ignore_list(c))
                continue;

            tstk->TPush();
            unsigned int x1, x2, y1, y2;
            if (tstk->TOverlapInst(c, AOI, &x1, &x2, &y1, &y2)) {
                CDap ap(c);
                int tx, ty;
                tstk->TGetTrans(&tx, &ty);
                xyg_t xyg(x1, x2, y1, y2);
                do {
                    if (!c0)
                        c0 = ce = new sSubcLink(c, xyg.x, xyg.y);
                    else {
                        ce->next = new sSubcLink(c, xyg.x, xyg.y);
                        ce = ce->next;
                    }

                    tstk->TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                    ce->subs = gd->build_links(tstk, AOI);
                    tstk->TSetTrans(tx, ty);
                } while (xyg.advance());
            }
            tstk->TPop();
        }
    }

    // include copies of flattened cells
    for (sSubcList *s = gd_subckts; s; s = s->next()) {
        for (sSubcInst *su = s->subs(); su; su = su->next()) {
            if (!su->iscopy())
                continue;
            CDc *cdesc = su->cdesc();

            CDs *msdesc = cdesc->masterCell(true);
            if (!msdesc)
                continue;

            // ignore connectors, these have been accounted for in
            // combine()
            if (msdesc->isConnector())
                continue;
            cGroupDesc *gd = msdesc->groups();
            if (!gd)
                continue;

            tstk->TPush();
            tstk->TApplyTransform(su->cdesc());
            CDap ap(su->cdesc());
            tstk->TTransMult(su->ix()*ap.dx, su->iy()*ap.dy);
            BBox tBB(*msdesc->BB());
            tstk->TBB(&tBB, 0);
            if (tBB.intersect(AOI, true)) {
                if (!c0)
                    c0 = ce = new sSubcLink(cdesc, su->ix(), su->iy());
                else {
                    ce->next = new sSubcLink(cdesc, su->ix(), su->iy());
                    ce = ce->next;
                }
                ce->subs = gd->build_links(tstk, AOI);
            }
            tstk->TPop();
        }
    }
    return (c0);
}

