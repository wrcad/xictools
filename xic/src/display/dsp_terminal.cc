
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
#include "dsp_window.h"
#include "dsp_inlines.h"
#include "cd_terminal.h"
#include "cd_netname.h"


// Locate the terminal tname, return coordinate and cell desc, according
// to (physical or electrical) mode.
//
bool
cDisplay::FindTerminal(const char *tname, CDc **cdp, int *pvix, CDp_node **ppn)
{
    if (pvix)
        *pvix = 0;
    CDs *cursde = CurCell(Electrical, true);
    if (!cursde)
        return (false);

    CDnetName name = CDnetex::name_tab_find(tname);
    if (!name)
        return (false);

    CDm_gen mgen(cursde, GEN_MASTERS);
    for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
        CDc_gen cgen(m);
        for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
            CDp_range *pr = (CDp_range*)c->prpty(P_RANGE);
            CDgenRange rgen(pr);
            int vec_ix = 0;
            while (rgen.next(0)) {
                CDp_cnode *pn = (CDp_cnode*)c->prpty(P_NODE);
                for ( ; pn; pn = pn->next()) {
                    CDp_cnode *pn1;
                    if (vec_ix == 0)
                        pn1 = pn;
                    else
                        pn1 = pr->node(0, vec_ix, pn->index());
                    if (!pn1)
                        continue;
                    if (name == pn1->term_name()) {
                        *cdp = c;
                        *ppn = pn1;
                        if (pvix)
                            *pvix = vec_ix;
                        return (true);
                    }
                }
                vec_ix++;
            }
        }
    }
    CDp_snode *pn = (CDp_snode*)cursde->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {
        if (name == (CDnetName)pn->term_name()) {
            *cdp = 0;
            *ppn = pn;
            return (true);
        }
    }
    return (false);
}


// Show the selected terminal in a pop-up window.
//
void
cDisplay::ShowTerminal(CDc *ecdesc, int vecix, CDp_node *pn)
{
    static int wnum = -1;
    if (!pn)
        return;
    CDc *pcdesc = 0;
    unsigned int ix = 0, iy = 0;
    if (ecdesc)
        pcdesc = ecdesc->findPhysDualOfElec(vecix, &ix, &iy);
    int mode;
    if (wnum < 0 || !DSP()->Window(wnum))
        mode = DSP()->CurMode();
    else
        mode = DSP()->Window(wnum)->Mode();

    Point epx;
    Point_c ppx;
    int dp = 10*CDphysResolution;
    int de = 10*CDelecResolution;
    if (!ecdesc) {
        // top level terminals
        CDp_snode *pns = (CDp_snode*)pn;
        CDsterm *term = pns->cell_terminal();
        if (term) {
            ppx.x = term->lx();
            ppx.y = term->ly();
        }
        CDs *cursde = CurCell(Electrical);
        if (cursde && cursde->isSymbolic()) {
            int x, y;
            if (!pns->get_pos(0, &x, &y))
                return;
            epx.set(x, y);
        }
        else {
            int x, y;
            pns->get_schem_pos(&x, &y);
            epx.set(x, y);
        }
    }
    else {
        CDp_cnode *pnc = (CDp_cnode*)pn;
        CDterm *term = pnc->inst_terminal();
        if (term) {
            ppx.x = term->lx();
            ppx.y = term->ly();
        }
        if (pcdesc) {
            CDap ap(pcdesc);
            cTfmStack stk;
            stk.TPush();
            stk.TApplyTransform(pcdesc);
            stk.TTransMult(ix*ap.dx, iy*ap.dy);
            BBox BB(*pcdesc->masterCell()->BB());
            stk.TBB(&BB, 0);
            stk.TPop();

            if (BB.intersect(&ppx, true)) {
                dp = ppx.x - BB.left;
                if (BB.right - ppx.x > dp)
                    dp = BB.right - ppx.x;
                int d = ppx.y - BB.bottom;
                if (BB.top - ppx.y > d)
                    d = BB.top - ppx.y;
                if (d > dp)
                    dp = d;
            }
        }
        int x, y;
        if (!pnc->get_pos(0, &x, &y))
            return;
        epx.set(x, y);
        if (ecdesc->oBB().intersect(&epx, true)) {
            de = epx.x - ecdesc->oBB().left;
            if (ecdesc->oBB().right - epx.x > de)
                de = ecdesc->oBB().right - epx.x;
            int d = epx.y - ecdesc->oBB().bottom;
            if (ecdesc->oBB().top - epx.y > d)
                d = ecdesc->oBB().top - epx.y;
            if (d > de)
                de = d;
        }
    }

    if (mode == Electrical) {
        BBox AOI(epx.x - de, epx.y - de, epx.x + de, epx.y + de);
        if (wnum < 0 || !DSP()->Window(wnum))
            wnum = DSP()->OpenSubwin(&AOI);
        else {
            DSP()->Window(wnum)->InitWindow(epx.x, epx.y, 2*de);
            DSP()->Window(wnum)->Redisplay(0);
        }
    }
    else {
        BBox AOI(ppx.x - dp, ppx.y - dp, ppx.x + dp, ppx.y + dp);
        if (wnum < 0 || !DSP()->Window(wnum))
            wnum = DSP()->OpenSubwin(&AOI);
        else {
            DSP()->Window(wnum)->InitWindow(ppx.x, ppx.y, 2*dp);
            DSP()->Window(wnum)->Redisplay(0);
        }
    }
    if (wnum >= 0) {
        DSP()->Window(wnum)->WinStr()->ex = epx.x;
        DSP()->Window(wnum)->WinStr()->ey = epx.y;
        DSP()->Window(wnum)->WinStr()->ewid = 2*de;
        DSP()->Window(wnum)->WinStr()->eset = true;
        DSP()->Window(wnum)->WinStr()->px = ppx.x;
        DSP()->Window(wnum)->WinStr()->py = ppx.y;
        DSP()->Window(wnum)->WinStr()->pwid = 2*dp;
        DSP()->Window(wnum)->WinStr()->pset = true;
    }
}

