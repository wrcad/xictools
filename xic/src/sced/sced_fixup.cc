
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: sced_fixup.cc,v 5.34 2014/10/04 22:18:37 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "sced.h"
#include "dsp_inlines.h"
#include "cd_terminal.h"
#include "cd_lgen.h"
#include "errorlog.h"


// For each cell that contains an instance of sdesc, add a connection
// vertex at the reflected x,y coordinate if there is an overlying
// wire, and the wire has no vertex at the location.
//
void
cSced::addParentConnection(CDs *sdesc, int x, int y)
{
    cTfmStack stk;
    CDm_gen gen(sdesc, GEN_MASTER_REFS);
    for (CDm *mdesc = gen.m_first(); mdesc; mdesc = gen.m_next()) {
        CDs *sd = mdesc->parent();
        if (!sd)
            continue;
        bool upd = false;
        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            stk.TPush();
            stk.TApplyTransform(cdesc);
            int xo = x;
            int yo = y;
            stk.TPoint(&xo, &yo);
            if (addConnection(sd, xo, yo, 0) == 2)
                upd = true;
            stk.TPop();
        }
        if (upd)
            dotsSetDirty(sd);
    }
}


// For each instance os sdesc, add wire vertices at connection points
// in the parent if necessary.
//
void
cSced::addParentConnections(CDs *sdesc)
{
    CDm_gen gen(sdesc, GEN_MASTER_REFS);
    for (CDm *mdesc = gen.m_first(); mdesc; mdesc = gen.m_next()) {
        CDs *sd = mdesc->parent();
        if (!sd)
            continue;
        bool upd = false;
        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                for (unsigned int ix = 0; ; ix++) {
                    int x, y;
                    if (!pn->get_pos(ix, &x, &y))
                        break;
                    if (addConnection(sd, x, y) == 2)
                        upd = true;
                }
            }
            CDp_bcnode *pb = (CDp_bcnode*)cdesc->prpty(P_BNODE);
            for ( ; pb; pb = pb->next()) {
                for (unsigned int ix = 0; ; ix++) {
                    int x, y;
                    if (!pb->get_pos(ix, &x, &y))
                        break;
                    if (addConnection(sd, x, y) == 2)
                        upd = true;
                }
            }
        }
        if (upd)
            dotsSetDirty(sd);
    }
}


// If a wire crosses x, y add a vertex to the wire, if it is not a
// vertex already.  Don't check wrdesc if given.  If any vertex was
// added, return 2.  Return 1 if a vertex was found.  Return 0
// otherwise.
//
int
cSced::addConnection(CDs *sd, int x, int y, CDw *wrdesc)
{
    if (!sd)
        return (false);
    BBox BB;
    BB.left = x - 10;
    BB.bottom = y - 10;
    BB.right = x + 10;
    BB.top = y + 10;
    int conn = 0;
    CDg gdesc;
    CDsLgen gen(sd);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        gdesc.init_gen(sd, ld, &BB);
        CDw *wd;
        while ((wd = (CDw*)gdesc.next()) != 0) {
            if (wd->type() != CDWIRE || wd == wrdesc)
                continue;
            if (!wd->is_normal())
                continue;
            int c = addConnection(wd, x, y, true);
            if (c > conn)
                conn = c;
        }
    }
    return (conn);
}


// Return true if a connection is missing at x,y and add it if fix is
// true.
//
bool
cSced::checkAddConnection(CDs *sd, int x, int y, bool fix)
{
    if (!sd)
        return (false);
    BBox BB;
    BB.left = x - 10;
    BB.bottom = y - 10;
    BB.right = x + 10;
    BB.top = y + 10;
    bool conn = false;
    CDg gdesc;
    CDsLgen gen(sd);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        gdesc.init_gen(sd, ld, &BB);
        CDw *wd;
        while ((wd = (CDw*)gdesc.next()) != 0) {
            if (wd->type() != CDWIRE)
                continue;
            if (!wd->is_normal())
                continue;
            if (addConnection(wd, x, y, fix) == 2)
                conn = true;
        }
    }
    return (conn);
}


// Simplify all wire paths by removing unused colinear vertices.
// Can be called it physical or electrical mode.
//
void
cSced::fixPaths(CDs *sd)
{
    if (!sd)
        return;
    CDg gdesc;
    CDsLgen gen(sd);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        gdesc.init_gen(sd, ld);
        CDw *wd;
        while ((wd = (CDw*)gdesc.next()) != 0) {
            if (wd->type() != CDWIRE)
                continue;
            if (!wd->is_normal())
                continue;
            fixVertices(wd, sd);
        }
    }
}


namespace {
    // If the wire in wrdesc crosses a device terminal or the end
    // vertex of another wire without making connection, add a vertex
    // at that point establishing the connection.
    //
    void
    add_wire_connections(CDs *sdesc, CDw *wrdesc)
    {
        BBox BB = wrdesc->oBB();
        BB.bloat(10);
        CDg gdesc;
        gdesc.init_gen(sdesc, CellLayer(), &BB);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (!odesc->is_normal())
                continue;
            CDp_cnode *pn = (CDp_cnode*)odesc->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                for (unsigned int ix = 0; ; ix++) {
                    int x, y;
                    if (!pn->get_pos(ix, &x, &y))
                        break;
                    SCD()->addConnection(wrdesc, x, y, true);
                }
            }
            CDp_bcnode *pb = (CDp_bcnode*)odesc->prpty(P_BNODE);
            for ( ; pb; pb = pb->next()) {
                for (unsigned int ix = 0; ; ix++) {
                    int x, y;
                    if (!pb->get_pos(ix, &x, &y))
                        break;
                    SCD()->addConnection(wrdesc, x, y, true);
                }
            }
        }
        CDsLgen gen(sdesc);
        CDl *ld;
        while ((ld = gen.next()) != 0) {
            if (!ld->isWireActive())
                continue;
            gdesc.init_gen(sdesc, ld, &BB);
            CDw *wd;
            while ((wd = (CDw*)gdesc.next()) != 0) {
                if (wd->type() != CDWIRE || wd == wrdesc)
                    continue;
                if (!wd->is_normal())
                    continue;
                const Point *pts = wd->points();
                int num = wd->numpts();
                SCD()->addConnection(wrdesc, pts[0].x, pts[0].y, true);
                SCD()->addConnection(wrdesc, pts[num-1].x, pts[num-1].y, true);
            }
        }

        // Add vertices at non-byname cell connection points.
        //
        CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            if (!pn->has_flag(TE_BYNAME)) {
                int x, y;
                pn->get_schem_pos(&x, &y);
                SCD()->addConnection(wrdesc, x, y, true);
            }
        }
    }


    // Return false if another wire exists with precisely the same
    // cordinates in either direction.
    //
    bool
    check_if_overlay(CDs *sdesc, CDw *wrdesc)
    {
        int num = wrdesc->numpts();
        const Point *pts = wrdesc->points();
        CDg gdesc;
        CDsLgen gen(sdesc);
        CDl *ld;
        while ((ld = gen.next()) != 0) {
            if (!ld->isWireActive())
                continue;
            gdesc.init_gen(sdesc, ld, &wrdesc->oBB());
            CDw *wd;
            while ((wd = (CDw*)gdesc.next()) != 0) {
                if (wd->type() != CDWIRE || wd == wrdesc)
                    continue;
                if (!wd->is_normal())
                    continue;
                if (wd->numpts() != num)
                    continue;
                const Point *opts = wd->points();
                int i;
                if (pts[0].x == opts[0].x && pts[0].y == opts[0].y) {
                    for (i = 1; i < num; i++) {
                        if (pts[i].x != opts[i].x || pts[i].y != opts[i].y)
                            break;
                    }
                    if (i == num)
                        return (false);
                }
                else if (pts[0].x == opts[num-1].x && pts[0].y == opts[num-1].y) {
                    for (i = 1; i < num; i++) {
                        if (pts[i].x != opts[num-1-i].x ||
                                pts[i].y != opts[num-1-i].y)
                            break;
                    }
                    if (i == num)
                        return (false);
                }
            }
        }
        return (true);
    }
}


void
cSced::install(CDo *odesc, CDs *sdesc, bool full_check)
{
    if (!odesc)
        return;
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
poly:
label:
    return;
wire:
    {
        // Function to examine and fix various problems with a newly
        // created wire.  Called in electrical mode.
        //
        // Only check wires on the active layers, and not in symbolic cells.
        if (!((CDw*)odesc)->ldesc()->isWireActive())
            return;
        if (sdesc->isSymbolic())
            return;

        // Pathologies (no pun intended)
        // 1) exactly overlays another wire.
        // 2) internal co-linear vertex without connection.
        // 3) wire crosses connection point without connection.

        // Establish a connection to any wire lying under a vertex
        const Point *pts = ((CDw*)odesc)->points();
        int num = ((CDw*)odesc)->numpts();
        for (int i = 0; i < num; i++)
            SCD()->addConnection(sdesc, pts[i].x, pts[i].y, (CDw*)odesc);

        // take care of (2)
        fixVertices(odesc, sdesc);

        // establish connection to underlying terminal (3)
        add_wire_connections(sdesc, (CDw*)odesc);

        if (!full_check)
            return;

        // take care of (1)
        if (!check_if_overlay(sdesc, (CDw*)odesc)) {
            Log()->WarningLog(mh::ObjectCreation,
                "Exact overlap of another wire.");
        }
        return;
    }
inst:
    {
        // Function to add connection points to existing wires which pass
        // directly across the terminals.  Called in electrical mode.
        //
        CDp_cnode *pn = (CDp_cnode*)odesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int x, y;
                if (!pn->get_pos(ix, &x, &y))
                    break;
                SCD()->addConnection(sdesc, x, y);
            }
        }
        CDp_bcnode *pb = (CDp_bcnode*)odesc->prpty(P_BNODE);
        for ( ; pb; pb = pb->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int x, y;
                if (!pb->get_pos(ix, &x, &y))
                    break;
                SCD()->addConnection(sdesc, x, y);
            }
        }
        return;
    }
}


void
cSced::uninstall(CDo *odesc, CDs *sdesc)
{
    if (!odesc)
        return;
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
poly:
label:
    return;
wire:
    {
        // Only check wires on the active layers, and not in symbolic cells.
        if (!odesc->ldesc()->isWireActive())
            return;
        if (sdesc->isSymbolic())
            return;

        const Point *pts = ((CDw*)odesc)->points();
        int num = ((CDw*)odesc)->numpts();
        for (int i = 0; i < num; i++) {
            BBox BB;
            BB.left = pts[i].x - 10;
            BB.bottom = pts[i].y - 10;
            BB.right = pts[i].x + 10;
            BB.top = pts[i].y + 10;
            CDg gdesc;
            CDsLgen gen(sdesc);
            CDl *ld;
            while ((ld = gen.next()) != 0) {
                if (!ld->isWireActive())
                    continue;
                gdesc.init_gen(sdesc, ld);
                CDw *wd;
                while ((wd = (CDw*)gdesc.next()) != 0) {
                    if (wd->type() != CDWIRE)
                        continue;
                    if (!wd->is_normal())
                        continue;
                    fixVertices(wd, sdesc);
                }
            }
        }
        return;
    }
inst:
    {
        CDp_cnode *pn = (CDp_cnode*)odesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int x, y;
                if (!pn->get_pos(ix, &x, &y))
                    break;
                BBox BB(x - 10, y - 10, x + 10, y + 10);
                CDg gdesc;
                CDsLgen gen(sdesc);
                CDl *ld;
                while ((ld = gen.next()) != 0) {
                    if (!ld->isWireActive())
                        continue;
                    gdesc.init_gen(sdesc, ld);
                    CDw *wd;
                    while ((wd = (CDw*)gdesc.next()) != 0) {
                        if (wd->type() != CDWIRE)
                            continue;
                        if (!wd->is_normal())
                            continue;
                        fixVertices(wd, sdesc);
                    }
                }
            }
        }
        CDp_bcnode *pb = (CDp_bcnode*)odesc->prpty(P_BNODE);
        for ( ; pb; pb = pb->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int x, y;
                if (!pb->get_pos(ix, &x, &y))
                    break;
                BBox BB(x - 10, y - 10, x + 10, y + 10);
                CDg gdesc;
                CDsLgen gen(sdesc);
                CDl *ld;
                while ((ld = gen.next()) != 0) {
                    if (!ld->isWireActive())
                        continue;
                    gdesc.init_gen(sdesc, ld);
                    CDw *wd;
                    while ((wd = (CDw*)gdesc.next()) != 0) {
                        if (wd->type() != CDWIRE)
                            continue;
                        if (!wd->is_normal())
                            continue;
                        fixVertices(wd, sdesc);
                    }
                }
            }
        }
        return;
    }
}


// Remove vertices in manhattan segments which have no connection.
//
void
cSced::fixVertices(CDo *odesc, CDs *sdesc)
{
    if (!odesc || odesc->type() != CDWIRE)
        return;
    const Point *pts = ((CDw*)odesc)->points();
    int num = ((CDw*)odesc)->numpts();
    for (int i = 1; i < num-1; i++) {
        if ((pts[i].x == pts[i-1].x && pts[i].x == pts[i+1].x) ||
                (pts[i].y == pts[i-1].y && pts[i].y == pts[i+1].y)) {
            if (!sdesc->checkVertex(pts[i].x, pts[i].y, (CDw*)odesc)) {
                num--;
                ((CDw*)odesc)->delete_vertex(i);
                i--;
            }
        }
    }
}


// If x,y lies in a Manhattan segment of the wire, and fix is true,
// introduce a new vertex at x,y allowing a connection point.  Return
// 2 if a vertex was added, or would have been added if fix was true. 
// Return 1 if the vertex already exists.  Return 0 otherwise.
//
int
cSced::addConnection(CDo *odesc, int x, int y, bool fix)
{
    if (!odesc || odesc->type() != CDWIRE)
        return (0);
    const Point *pts = ((CDw*)odesc)->points();
    int num = ((CDw*)odesc)->numpts();
    int i;
    for (i = 0; i < num - 1; i++) {
        if (pts[i].x == pts[i+1].x) {
            if (x != pts[i].x)
                continue;
            if (pts[i+1].y > pts[i].y) {
                if (y > pts[i+1].y || y < pts[i].y)
                    continue;
            }
            else {
                if (y > pts[i].y || y < pts[i+1].y)
                    continue;
            }
            if (y == pts[i].y || y == pts[i+1].y)
                return (1);
            // add new vertex
            break;
        }
        else if (pts[i].y == pts[i+1].y) {
            if (y != pts[i].y)
                continue;
            if (pts[i+1].x > pts[i].x) {
                if (x > pts[i+1].x || x < pts[i].x)
                    continue;
            }
            else {
                if (x > pts[i].x || x < pts[i+1].x)
                    continue;
            }
            if (x == pts[i].x || x == pts[i+1].x)
                return (1);
            // add new vertex
            break;
        }
    }
    if (i != num - 1) {
        if (fix) {
            Point *p = new Point[num+1];
            int j;
            for (j = 0; j <= i; j++)
                p[j] = pts[j];
            p[j].set(x, y);
            for (j++; j <= num; j++)
                p[j] = pts[j-1];
            delete [] pts;
            ((CDw*)odesc)->set_points(p);
            ((CDw*)odesc)->set_numpts(num + 1);
        }
        return (2);
    }
    return (0);
}

