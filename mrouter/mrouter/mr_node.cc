
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA 2016, http://wrcad.com       *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY OR WHITELEY     *
 *   RESEARCH INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,   *
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 *   DEALINGS IN THE SOFTWARE.                                            *
 *                                                                        *
 *   Licensed under the Apache License, Version 2.0 (the "License");      *
 *   you may not use this file except in compliance with the License.     *
 *   You may obtain a copy of the License at                              *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *   Unless required by applicable law or agreed to in writing, software  *
 *   distributed under the License is distributed on an "AS IS" BASIS,    *
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or      *
 *   implied. See the License for the specific language governing         *
 *   permissions and limitations under the License.                       *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * LEF/DEF Database and Maze Router.                                      *
 *                                                                        *
 * Stephen R. Whiteley (stevew@wrcad.com)                                 *
 * Whiteley Research Inc. (wrcad.com)                                     *
 *                                                                        *
 * Portions adapted from Qrouter by Tim Edwards,                          *
 * (www.opencircuitdesign.com) which used code by Steve Beccue.           *
 * See original headers where applicable.                                 *
 *                                                                        *
 *========================================================================*
 $Id: mr_node.cc,v 1.27 2017/02/15 01:39:39 stevew Exp $
 *========================================================================*/

/*--------------------------------------------------------------*/
/* node.c -- Generation of detailed network and obstruction     */
/* information on the routing grid based on the geometry of the */
/* layout of the standard cell macros.                          */
/*                                                              */
/*--------------------------------------------------------------*/
/* Written by Tim Edwards, June, 2011, based on work by Steve   */
/* Beccue.                                                      */
/*--------------------------------------------------------------*/

#include "mrouter_prv.h"


//
// Maze Router.
//
// Misc. operations, pretty much verbatim from Qrouter.
//


// expand_tap_geometry
//
// For each rectangle defining part of a gate terminal, search the
// surrounding terminal geometry.  If the rectangle can expand in any
// direction, then allow it to grow to the maximum size.  This
// generates overlapping geometry for each terminal, but avoids bad
// results for determining how to route to a terminal point if the
// terminal is broken up into numerous nonoverlapping rectangles.
//
// Note that this is not foolproof.  It also needs a number of
// enhancements.  For example, to expand east, other geometry should
// be looked at in order of increasing left edge X value, and so
// forth.
//
void
cMRouter::expand_tap_geometry()
{
    for (u_int k = 0; k < numPins(); k++) {
        dbGate *g = nlPin(k);
        if (!g->taps)
            continue;
        for (int i = 0; i < g->nodes; i++) {
            if (g->netnum[i])
                expand_tap_geometry(g, i);
        }
    }
    for (u_int k = 0; k < numGates(); k++) {
        dbGate *g = nlGate(k);
        if (!g->taps)
            continue;
        for (int i = 0; i < g->nodes; i++) {
            if (g->netnum[i])
                expand_tap_geometry(g, i);
        }
    }
}


void
cMRouter::expand_tap_geometry(dbGate *g, int i)
{
    for (dbDseg *ds = g->taps[i]; ds; ds = ds->next) {
        bool expanded = true;
        while (expanded == true) {
            expanded = false;

            for (dbDseg *ds2 = g->taps[i]; ds2; ds2 = ds2->next) {
                if (ds == ds2)
                    continue;
                if (ds->layer != ds2->layer)
                    continue;
            
                if ((ds2->y1 <= ds->y1) && (ds2->y2 >= ds->y2)) {
                    // Expand east.
                    if ((ds2->x1 > ds->x1) && (ds2->x1 <= ds->x2)) {
                        if (ds->x2 < ds2->x2) {
                            ds->x2 = ds2->x2;
                            expanded = true;
                        }
                    }
            
                    // Expand west.
                    if ((ds2->x2 < ds->x2) && (ds2->x2 >= ds->x1)) {
                        if (ds->x1 > ds2->x1) {
                            ds->x1 = ds2->x1;
                            expanded = true;
                        }
                    }
                }
            
                if ((ds2->x1 <= ds->x1) && (ds2->x2 >= ds->x2)) {
                    // Expand north.
                    if ((ds2->y1 > ds->y1) && (ds2->y1 <= ds->y2)) {
                        if (ds->y2 < ds2->y2) {
                            ds->y2 = ds2->y2;
                            expanded = true;
                        }
                    }
            
                    // Expand south.
                    if ((ds2->y2 < ds->y2) && (ds2->y2 >= ds->y1)) {
                        if (ds->y1 > ds2->y1) {
                            ds->y1 = ds2->y1;
                            expanded = true;
                        }
                    }
                }
            }
        }
    }
}


// Truncate gates to the set of tracks.  Warn about any gates with
// nodes that are clipped entirely outside the routing area.
//
void
cMRouter::clip_gate_taps()
{
    for (u_int i = 0; i < numNets(); i++) {
        dbNet *net = mr_nets[i];
        for (dbNode *node = net->netnodes; node; node = node->next) {
            dbDpoint *dpl = 0;
            for (dbDpoint *dp = node->taps; dp; ) {

                int lay = dp->layer;

#ifdef LD_SIGNED_GRID
                if (dp->gridx < 0 || dp->gridy < 0 ||
                        dp->gridx >= numChannelsX(lay) ||
                        dp->gridy >= numChannelsY(lay)) {
#else
                if (dp->gridx >= numChannelsX(lay) ||
                        dp->gridy >= numChannelsY(lay)) {
#endif
                    db->emitErrMesg(
                        "Tap of port of node %d of net %s is outside of "
                        "route area\n", node->nodenum, net->netname);

                    if (!dpl)
                        node->taps = dp->next;
                    else
                        dpl->next = dp->next;

                    delete dp;
                    dp = (dpl == 0) ? node->taps : dpl->next;
                }
                else {
                    dpl = dp;
                    dp = dp->next;
                }
            }
        }
    }
}


//#define DEBUG_CO

// create_obstructions_from_gates
//
// Fills in the Obs[][] grid from obstructions that were defined for
// each macro in the technology LEF file and translated into a list of
// grid coordinates in each instance.
//
// Also, fills in the Obs[][] grid with obstructions that are defined
// by nodes of the gate that are unconnected in this netlist.
//
void
cMRouter::create_obstructions_from_gates()
{
    // Give a single net number to all obstructions, over the range of
    // the number of known nets, so these positions cannot be routed
    // through.  If a grid position is not wholly inside an
    // obstruction, then we maintain the direction of the nearest
    // obstruction in Obs and the distance to it in Obsinfo.  This
    // indicates that a route can avoid the obstruction by moving away
    // from it by the amount in Obsinfo plus spacing clearance.  If
    // another obstruction is found that prevents such a move, then
    // all direction flags will be set, indicating that the position
    // is not routable under any condition. 

    for (u_int k = 0; k < numPins(); k++) {
        dbGate *g = nlPin(k);
        for (dbDseg *ds = g->obs; ds; ds = ds->next)
            create_obstructions_from_gates(g, ds);
        for (int i = 0; i < g->nodes; i++)
            create_obstructions_from_gates(g, i);
    }
    for (u_int k = 0; k < numGates(); k++) {
        dbGate *g = nlGate(k);
        for (dbDseg *ds = g->obs; ds; ds = ds->next)
            create_obstructions_from_gates(g, ds);
        for (int i = 0; i < g->nodes; i++)
            create_obstructions_from_gates(g, i);
    }
}


void
cMRouter::create_obstructions_from_gates(dbGate*, dbDseg *ds)
{
    lefu_t deltax = get_via_clear(ds->layer, DIR_HORIZ, ds);
    int gridx = (ds->x1 - xLower() - deltax) / pitchX(ds->layer) - 1;
    for (;;) {
        lefu_t dx = (gridx * pitchX(ds->layer)) + xLower();
        if (dx >= (ds->x2 + deltax) || gridx >= numChannelsX(ds->layer))
            break;
        else if (dx > (ds->x1 - deltax) && gridx >= 0) {
            lefu_t deltay = get_via_clear(ds->layer, DIR_VERT, ds);
            int gridy = (ds->y1 - yLower() - deltay) / pitchY(ds->layer) - 1;
            for (;;) {
                lefu_t dy = (gridy * pitchY(ds->layer)) + yLower();
                if (dy >= (ds->y2 + deltay) ||
                        gridy >= numChannelsY(ds->layer))
                    break;
                if (dy > (ds->y1 - deltay) && gridy >= 0) {

                    // Check Euclidean distance measure.
                    lefu_t s = getRouteSpacing(ds->layer);

                    lefu_t edist, xp, yp;
                    if (dx < (ds->x1 + s - deltax)) {
                        xp = dx + deltax - s;
                        edist = (ds->x1 - xp) * (ds->x1 - xp);
                    }
                    else if (dx > (ds->x2 - s + deltax)) {
                        xp = dx - deltax + s;
                        edist = (xp - ds->x2) * (xp - ds->x2);
                    }
                    else
                        edist = 0;
                    if ((edist > 0) && (dy < (ds->y1 + s - deltay))) {
                        yp = dy + deltay - s;
                        edist += (ds->y1 - yp) * (ds->y1 - yp);
                    }
                    else if ((edist > 0) &&
                            (dy > (ds->y2 - s + deltay))) {
                        yp = dy - deltay + s;
                        edist += (yp - ds->y2) * (yp - ds->y2);
                    }
                    else
                        edist = 0;

                    if (edist < (s * s)) {

                        // If it clears distance for a route
                        // layer but not vias, then block vias
                        // only.

                        lefu_t deltaxy = get_route_clear(ds->layer, ds);
                        if ((dx <= (ds->x1 - deltaxy)) ||
                                (dx >= (ds->x2 + deltaxy)) ||
                                (dy <= (ds->y1 - deltaxy)) ||
                                (dy >= (ds->y2 + deltaxy))) {
                            block_route(gridx, gridy, ds->layer, UP);
                            block_route(gridx, gridy, ds->layer, DOWN);
                        }
                        else {
#ifdef DEBUG_CO
                            printf("CO2 %d %d %d %g %g\n", gridx, gridy,
                                ds->layer, db->lefToMic(dx), db->lefToMic(dy));
#endif
                            check_obstruct(gridx, gridy, ds, dx, dy);
                        }
                    }
                    else {
                        edist = 0;     // diagnostic break
                    }
                }
                gridy++;
            }
        }
        gridx++;
    }
}


void
cMRouter::create_obstructions_from_gates(dbGate *g, int i)
{
    if (g->netnum[i])
        return;

    // Unconnected node.
    // Diagnostic, and power bus handling.
    if (g->node[i]) {
        // Should we flag a warning if we see something
        // that looks like a power or ground net here?

        if (verbose() > 1) {
            db->emitMesg(
                "Gate instance %s unconnected node %s\n",
                g->gatename, g->node[i]);
        }
    }
    else {
        if (verbose() > 1) {
            db->emitMesg(
                "Gate instance %s unconnected node (%d)\n",
                g->gatename, i);
        }
    }
    for (dbDseg *ds = g->taps[i]; ds; ds = ds->next) {

        lefu_t deltax = get_via_clear(ds->layer, DIR_HORIZ, ds);
        int gridx = (ds->x1 - xLower() - deltax) / pitchX(ds->layer) - 1;
        for (;;) {
            lefu_t dx = (gridx * pitchX(ds->layer)) + xLower();
            if (dx > (ds->x2 + deltax) || gridx >= numChannelsX(ds->layer))
                break;
            else if (dx >= (ds->x1 - deltax) && gridx >= 0) {
                lefu_t deltay = get_via_clear(ds->layer, DIR_VERT, ds);
                int gridy = (ds->y1 - yLower() - deltay) /
                    pitchY(ds->layer) - 1;
                for (;;) {
                    lefu_t dy =
                        (gridy * pitchY(ds->layer)) + yLower();
                    if (dy >= (ds->y2 + deltay) ||
                            gridy >= numChannelsY(ds->layer))
                        break;
                    if (dy > (ds->y1 - deltay) && gridy >= 0) {

                        // Check Euclidean distance measure.
                        lefu_t s = getRouteSpacing(ds->layer);

                        lefu_t edist = 0, xp, yp;
                        if (dx < (ds->x1 + s - deltax)) {
                            xp = dx + deltax - s;
                            edist += (ds->x1 - xp) * (ds->x1 - xp);
                        }
                        else if (dx > (ds->x2 - s + deltax)) {
                            xp = dx - deltax + s;
                            edist += (xp - ds->x2) * (xp - ds->x2);
                        }
                        else
                            edist = 0;
                        if ((edist > 0) && (dy <
                                (ds->y1 + s - deltay))) {
                            yp = dy + deltay - s;
                            edist += (ds->y1 - yp) * (ds->y1 - yp);
                        }
                        else if ((edist > 0) && (dy >
                                (ds->y2 - s + deltay))) {
                            yp = dy - deltay + s;
                            edist += (yp - ds->y2) * (yp - ds->y2);
                        }
                        else
                            edist = 0;

                        if (edist < (s * s)) {

                            // If it clears distance for a
                            // route layer but not vias,
                            // then block vias only.

                            lefu_t deltaxy = get_route_clear(ds->layer, ds);
                            if ((dx <= (ds->x1 - deltaxy)) ||
                                    (dx >= (ds->x2 + deltaxy)) ||
                                    (dy <= (ds->y1 - deltaxy)) ||
                                    (dy >= (ds->y2 + deltaxy))) {
                                block_route(gridx, gridy, ds->layer, UP);
                                block_route(gridx, gridy, ds->layer, DOWN);
                            }
                            else {
#ifdef DEBUG_CO
                                printf("CO3 %d %d %d %g %g\n", gridx, gridy,
                                    ds->layer, db->lefToMic(dx),
                                    db->lefToMic(dy));
#endif
                                check_obstruct(gridx, gridy, ds, dx, dy);
                            }
                        }
                    }
                    gridy++;
                }
            }
            gridx++;
        }
    }
}


// create_obstructions_from_list
//
// Adds obstructions from the passed list of obstructions.
//
void
cMRouter::create_obstructions_from_list(dbDseg *obslist)
{
    // Create additional obstructions from the list.  These
    // obstructions are not considered to be metal layers, so we don't
    // compute a distance measure.  However, we need to compute a
    // boundary of 1/2 route width to avoid having the route
    // overlapping the obstruction area.

    lefu_t *delta = new lefu_t[numLayers()];
    for (u_int i = 0; i < numLayers(); i++)
        delta[i] = getRouteWidth(i)/2;

    for (dbDseg *ds = obslist; ds; ds = ds->next) {
#ifdef DEBUG_CO
        printf("CO %g %g %g %g %d\n", db->lefToMic(ds->x1),
            db->lefToMic(ds->y1), db->lefToMic(ds->x2),
            db->lefToMic(ds->y2), ds->layer);
#endif
        int gridx = (ds->x1 - xLower() - delta[ds->layer]) /
            pitchX(ds->layer) - 1;
        for (;;) {
            lefu_t dx = (gridx * pitchX(ds->layer)) + xLower();
            if (dx > (ds->x2 + delta[ds->layer]) ||
                    gridx >= numChannelsX(ds->layer))
                break;
            if (dx >= (ds->x1 - delta[ds->layer]) && gridx >= 0) {
                int gridy = (ds->y1 - yLower() - delta[ds->layer]) /
                    pitchY(ds->layer) - 1;
                for (;;) {
                    lefu_t dy = (gridy * pitchY(ds->layer)) + yLower();
                    if (dy > (ds->y2 + delta[ds->layer]) ||
                            gridy >= numChannelsY(ds->layer))
                        break;
                    if (dy >= (ds->y1 - delta[ds->layer]) && gridy >= 0) {
#ifdef DEBUG_CO
                        printf("CO1 %d %d %d %g %g\n", gridx, gridy,
                            ds->layer, db->lefToMic(dx), db->lefToMic(dy));
#endif
                        check_obstruct(gridx, gridy, ds, dx, dy);
                    }
                    gridy++;
                }
            }
            gridx++;
        }
    }
    delete [] delta;
}


// create_obstructions_inside_nodes
//
// Fills in the Obs[][] grid from the position of each node (net
// terminal), which may have multiple unconnected positions.
//
// Also fills in the Nodeinfo.nodeloc[] grid with the node number,
// which causes the router to put a premium on routing other nets over
// or under this position, to discourage boxing in a pin position and
// making it unroutable.
//
// This function is split into two passes.  This pass adds information
// for points inside node regions.
//
// AUTHOR:  Tim Edwards, June 2011, based on code by Steve Beccue.
//
void
cMRouter::create_obstructions_inside_nodes()
{
    // For each node terminal (gate pin), mark each grid position with
    // the net number.  This overrides any obstruction that may be
    // placed at that point.

    // For each pin position, we also find the "keepout" area around
    // the pin where we may not place an unrelated route.  For this,
    // we use a flag bit, so that the position can be ignored when
    // routing the net associated with the pin.  Normal obstructions
    // take precedence.

    for (u_int k = 0; k < numPins(); k++) {
        dbGate *g = nlPin(k);
        for (int i = 0; i < g->nodes; i++) {
            if (g->netnum[i])
                create_obstructions_inside_nodes(g, i);
        }
    }
    for (u_int k = 0; k < numGates(); k++) {
        dbGate *g = nlGate(k);
        for (int i = 0; i < g->nodes; i++) {
            if (g->netnum[i])
                create_obstructions_inside_nodes(g, i);
        }
    }
}


//#define DEBUG_IN
//#define DEBUG_DA

void
cMRouter::create_obstructions_inside_nodes(dbGate *g, int i)
{
    // Get the node record associated with this pin.
    dbNode *node = g->noderec[i];
    if (!node)
        return;

    // First mark all areas inside node geometry boundary.

    for (dbDseg *ds = g->taps[i]; ds; ds = ds->next) {

#ifdef DEBUG_IN
printf("IN %s/%s %g %g %g %g %d\n", g->gatename, g->node[i],
    db->lefToMic(ds->x1), db->lefToMic(ds->y1),
    db->lefToMic(ds->x2), db->lefToMic(ds->y2), ds->layer);
#endif

        int gridx = (ds->x1 - xLower()) / pitchX(ds->layer) - 1;
        for (;;) {
            lefu_t dx = (gridx * pitchX(ds->layer)) + xLower();
#ifdef DEBUG_IN
printf("IN xtop %g %d\n", db->lefToMic(dx), gridx);
#endif
            if (dx >= ds->x2 || gridx >= numChannelsX(ds->layer)) {
#ifdef DEBUG_IN
printf("IN xbrk\n");
#endif
                break;
            }
            if (dx > ds->x1 && gridx >= 0) {
                int gridy = (ds->y1 - yLower()) / pitchY(ds->layer) - 1;
                for (;;) {
                    lefu_t dy = (gridy * pitchY(ds->layer)) + yLower();
#ifdef DEBUG_IN
printf("IN ytop %g %d\n", db->lefToMic(dy), gridy);
#endif
                    if (dy >= ds->y2 ||
                            gridy >= numChannelsY(ds->layer)) {
#ifdef DEBUG_IN
printf("IN ybrk\n");
#endif
                        break;
                    }

                    // Area inside defined pin geometry.

                    if (dy > ds->y1 && gridy >= 0) {
#ifdef DEBUG_IN
printf("IN x1\n");
#endif
                        mrGridCell c;
                        initGridCell(c, gridx, gridy, ds->layer);
                        int orignet = obsVal(c);

                        if ((orignet & ROUTED_NET_MASK &
                                ~ROUTED_NET) == node->netnum) {

                            // Duplicate tap point, or pre-existing
                            // route.  Don't re-process it if it is a
                            // duplicate.

                            if (nodeLoc(c) != 0) {
                                gridy++;
                                continue;
                            }
                        }
                        else if (!(orignet & NO_NET) &&
                                (orignet & ROUTED_NET_MASK)) {

                            // Net was assigned to other net, but is
                            // inside this pin's geometry.  Declare
                            // point to be unroutable, as it is too
                            // close to both pins.  NOTE:  This is a
                            // conservative rule and could potentially
                            // make a pin unroutable.  Another note: 
                            // By setting Obs[] to OBSTRUCT_MASK as
                            // well as NO_NET, we ensure that it falls
                            // through on all subsequent processing.

#ifdef DEBUG_DA
                            db->emitMesg("disable a %d %d %d\n",
                                gridx, gridy, ds->layer);
#endif

                            disable_gridpos(gridx, gridy, ds->layer);
                            gridy++;
                            continue;
                        }

                        if (!(orignet & NO_NET)) {

                            // A grid point that is within 1/2
                            // route width of a tap rectangle
                            // corner can violate metal width
                            // rules, and so should declare a
                            // stub.
                
                            u_int mask = 0;
                            u_int dir = 0;
                            lefu_t dist = 0;
                            lefu_t xdist = getRouteWidth(ds->layer)/2;

                            if (dx >= ds->x2 - xdist) {
                                if (dy > ds->y2 - xdist) {
                                    // Check northeast corner.

                                    if ((ds->x2 - dx) >= (ds->y2 - dy)) {
                                        // West-pointing stub.
                                        mask = STUBROUTE;
                                        dir = NI_STUB_EW;
                                        dist = ds->x2 - dx - 2*xdist;
                                    }
                                    else {
                                        // South-pointing stub.
                                        mask = STUBROUTE;
                                        dir = NI_STUB_NS;
                                        dist = ds->y2 - dy - 2*xdist;
                                    }
                                }
                                else if (dy < ds->y1 + xdist) {
                                    // Check southeast corner.

                                    if ((ds->x2 - dx) >= (dy - ds->y1)) {
                                        // West-pointing stub.
                                        mask = STUBROUTE;
                                        dir = NI_STUB_EW;
                                        dist = ds->x2 - dx - 2*xdist;
                                    }
                                    else {
                                        // North-pointing stub.
                                        mask = STUBROUTE;
                                        dir = NI_STUB_NS;
                                        dist = ds->y1 - dy + 2*xdist;
                                    }
                                }
                            }
                            else if (dx <= ds->x1 + xdist) {
                                if (dy > ds->y2 - xdist) {
                                    // Check northwest corner.

                                    if ((dx - ds->x1) >= (ds->y2 - dy)) {
                                        // East-pointing stub.
                                        mask = STUBROUTE;
                                        dir = NI_STUB_EW;
                                        dist = ds->x1 - dx + 2*xdist;
                                    }
                                    else {
                                        // South-pointing stub.
                                        mask = STUBROUTE;
                                        dir = NI_STUB_NS;
                                        dist = ds->y2 - dy - 2*xdist;
                                    }
                                }
                                else if (dy < ds->y1 + xdist) {
                                    // Check southwest corner.

                                    if ((dx - ds->x2) >= (dy - ds->y1)) {
                                        // East-pointing stub.
                                        mask = STUBROUTE;
                                        dir = NI_STUB_EW;
                                        dist = ds->x1 - dx + 2*xdist;
                                    }
                                    else {
                                        // North-pointing stub.
                                        mask = STUBROUTE;
                                        dir = NI_STUB_NS;
                                        dist = ds->y1 - dy + 2*xdist;
                                    }
                                }
                            }

                            setObsVal(c, ((obsVal(c) &
                                BLOCKED_MASK) | node->netnum | mask));
                            setNodeLoc(c, node);
                            setNodeSav(c, node);
                            setStubVal(c, dist);
                            setFlagsVal(c, flagsVal(c) | dir);
#ifdef DEBUG_IN
printf("IN a %d %d %d %x\n", c.gridx, c.gridy, c.layer, obsVal(c));
#endif
                        }
                        else if ((orignet & NO_NET) &&
                                ((orignet & OBSTRUCT_MASK)
                                != OBSTRUCT_MASK)) {
                            // Handled on next pass.
                        }

                        // Check that we have not created a PINOBSTRUCT
                        // route directly over this point.
                        if (ds->layer < (int)numLayers() - 1) {
                            initGridCell(c, gridx, gridy, ds->layer + 1);
                            u_int k = obsVal(c);
                            if (k & PINOBSTRUCTMASK) {
                                if ((k & ROUTED_NET_MASK) != node->netnum) {
                                    setObsVal(c, NO_NET);
#ifdef DEBUG_IN
printf("IN b %d %d %d %x\n", c.gridx, c.gridy, c.layer, obsVal(c));
#endif
                                    mrNodeInfo **p = nodeInfoAry(c);
                                    if (p) {
                                        p += c.index;
                                        // Bulk allocated, don't delete!
                                        *p = 0;
                                    }
                                }
                            }
                        }
                    }
                    gridy++;
                }
            }
            gridx++;
        }
    }
}


//#define DEBUG_OT

// create_obstructions_outside_nodes
//
// Fills in the Obs[][] grid from the position of each node (net
// terminal), which may have multiple unconnected positions.
//
// Also fills in the Nodeinfo.nodeloc[] grid with the node number,
// which causes the router to put a premium on routing other nets over
// or under this position, to discourage boxing in a pin position and
// making it unroutable.
//
// This function is split into two passes.  This pass adds information
// for points outside node regions but close enough to interact with
// the node.
//
//  AUTHOR:  Tim Edwards, June 2011, based on code by Steve Beccue
//
void
cMRouter::create_obstructions_outside_nodes()
{
    // Use a more conservative definition of keepout, to include via
    // widths, which may be bigger than route widths.

    lefu_t *offmaxx = new lefu_t[numLayers()];
    lefu_t *offmaxy = new lefu_t[numLayers()];
    for (u_int i = 0; i < numLayers(); i++) {
        lefu_t w = getRouteWidth(i)/2;
        offmaxx[i] = pitchX(i) - (haloX(i) + w);
        offmaxy[i] = pitchY(i) - (haloY(i) + w);
#ifdef DEBUG_OT
        db->emitMesg("offmax %g %g %g %g %g %g\n",
            db->lefToMic(offmaxx[i]), db->lefToMic(offmaxy[i]),
            db->lefToMic(haloX(i)),
            db->lefToMic(w), db->lefToMic(getViaWidth(i, i, DIR_VERT)),
            db->lefToMic(getRouteWidth(i)));
#endif
    }

    // When we place vias at an offset, they have to satisfy the
    // spacing requirements for the via's top layer, as well.  So take
    // the least maximum offset of each layer and the layer above it.

    for (u_int i = 0; i < numLayers() - 1; i++) {
        offmaxx[i] = LD_MIN(offmaxx[i], offmaxx[i + 1]);
        offmaxy[i] = LD_MIN(offmaxy[i], offmaxy[i + 1]);
    }

    // For each node terminal (gate pin), mark each grid position with
    // the net number.  This overrides any obstruction that may be
    // placed at that point.

    // For each pin position, we also find the "keepout" area around
    // the pin where we may not place an unrelated route.  For this,
    // we use a flag bit, so that the position can be ignored when
    // routing the net associated with the pin.  Normal obstructions
    // take precedence.

    for (u_int k = 0; k < numPins(); k++) {
        dbGate *g = nlPin(k);
        for (int i = 0; i < g->nodes; i++) {
            if (g->netnum[i]) {
#ifdef DEBUG_DA
                db->emitMesg(" %s/%s\n", g->gatename, g->node[i]);
#endif
                create_obstructions_outside_nodes(g, i, offmaxx, offmaxy);
            }
        }
    }
    for (u_int k = 0; k < numGates(); k++) {
        dbGate *g = nlGate(k);
        for (int i = 0; i < g->nodes; i++) {
            if (g->netnum[i]) {
#ifdef DEBUG_DA
                db->emitMesg(" %s/%s\n", g->gatename, g->node[i]);
#endif
                create_obstructions_outside_nodes(g, i, offmaxx, offmaxy);
            }
        }
    }
    delete [] offmaxx;
    delete [] offmaxy;
}


void
cMRouter::create_obstructions_outside_nodes(dbGate *g, int i,
    lefu_t *offmaxx, lefu_t *offmaxy)
{
    // Get the node record associated with this pin.
    dbNode *node = g->noderec[i];
    if (!node)
        return;

    // Repeat this whole exercise for areas in the halo
    // outside the node geometry.  We have to do this
    // after enumerating all inside areas because the tap
    // rectangles often overlap, and one rectangle's halo
    // may be inside another tap.

    for (dbDseg *ds = g->taps[i]; ds; ds = ds->next) {

        // Note:  Should be handling get_route_clear as a
        // less restrictive case, as was done above.

        lefu_t deltax = get_via_clear(ds->layer, DIR_HORIZ, ds);
        int gridx = (ds->x1 - xLower() - deltax) / pitchX(ds->layer) - 1;
#ifdef DEBUG_OT
printf("OT %g %d   %g %g %g %g %d\n", db->lefToMic(deltax), gridx,
    db->lefToMic(ds->x1), db->lefToMic(ds->y1),
    db->lefToMic(ds->x2), db->lefToMic(ds->y2), ds->layer);
#endif

        for (;;) {
            lefu_t dx = (gridx * pitchX(ds->layer)) + xLower();
            if (dx >= (ds->x2 + deltax) ||
                    gridx >= numChannelsX(ds->layer)) {
#ifdef DEBUG_OT
printf("OT xbrk\n");
#endif
                break;
            }
#ifdef DEBUG_OT
printf("OT xtop %g\n", db->lefToMic(dx));
#endif

            if (dx > (ds->x1 - deltax) && gridx >= 0) {
                lefu_t deltay = get_via_clear(ds->layer, DIR_VERT, ds);
                int gridy = (ds->y1 - yLower() - deltay) /
                    pitchY(ds->layer) - 1;

                for (;;) {
                    lefu_t dy = (gridy * pitchY(ds->layer)) + yLower();
                    if (dy >= (ds->y2 + deltay) ||
                            gridy >= numChannelsY(ds->layer)) {
#ifdef DEBUG_OT
printf("OT ybrk %g %g %d %d\n", db->lefToMic(dy),
    db->lefToMic(ds->y2 + deltay), gridy, numChannelsY(ds->layer));
#endif
                        break;
                    }
#ifdef DEBUG_OT
printf("OT ytop %g %g %d %d %x\n", db->lefToMic(dy),
    db->lefToMic(ds->y2 + deltay), gridy,
    numChannelsY(ds->layer), obsVal(gridx, gridy, ds->layer));
#endif

                    // 2nd pass on area inside defined pin
                    // geometry, allowing terminal
                    // connections to be made using an
                    // offset tap, where possible.

                    if ((dy >= ds->y1 && gridy >= 0) && (dx >= ds->x1)
                            && (dy <= ds->y2) && (dx <= ds->x2)) {
                        mrGridCell c;
                        initGridCell(c, gridx, gridy, ds->layer);
                        int orignet = obsVal(c);

                        if ((orignet & ROUTED_NET_MASK) == node->netnum) {

                            // Duplicate tap point.   Don't re-process it.
                            gridy++;
#ifdef DEBUG_OT
printf("continue\n");
#endif
                            continue;
                        }

                        if (!(orignet & NO_NET) &&
                                (orignet & ROUTED_NET_MASK)) {
                            // Do nothing;  previously handled.
                        }

                        else if ((orignet & NO_NET) &&
                                ((orignet & OBSTRUCT_MASK) != OBSTRUCT_MASK)) {
                            lefu_t sdistx =
                                getViaWidth(ds->layer, ds->layer, DIR_VERT)/2
                                + getRouteSpacing(ds->layer);
                            lefu_t sdisty =
                                getViaWidth(ds->layer, ds->layer, DIR_HORIZ)/2
                                + getRouteSpacing(ds->layer);
                            lefu_t offd;

                            // Define a maximum offset we can have in
                            // X or Y above which the placement of a
                            // via will cause a DRC violation with a
                            // wire in the adjacent route track in the
                            // direction of the offset.

                            int maxerr = 0;

                            // If a cell is positioned off-grid, then
                            // a grid point may be inside a pin and
                            // still be unroutable.  The Obsinfo[]
                            // array tells where an obstruction is, if
                            // there was only one obstruction in one
                            // direction blocking the grid point.  If
                            // so, then we set the Nodeinfo.stub[]
                            // distance to move the tap away from the
                            // obstruction to resolve the DRC error.

                            // Make sure we have marked this as a node.
                            setNodeLoc(c, node);
                            setNodeSav(c, node);
                            setObsVal(c, ((obsVal(c) & BLOCKED_MASK) |
                                node->netnum));

                            if (orignet & OBSTRUCT_N) {
                                offd = -(sdisty - obsInfoVal(c));
                                if (offd >= -offmaxy[ds->layer]) {
                                    setObsVal(c, obsVal(c) | OFFSET_TAP);
                                    setOffsetVal(c, offd);
                                    setFlagsVal(c,
                                        (flagsVal(c) | NI_OFFSET_NS));

                                    // If position above has obstruction, then
                                    // add up/down block to prevent vias.

                                    if ((ds->layer < (int)numLayers() - 1) &&
                                            (gridy > 0) &&
                                            (obsVal(gridx, gridy - 1,
                                                ds->layer + 1)
                                            & OBSTRUCT_MASK))
                                        block_route(gridx, gridy, ds->layer,
                                            UP);
                                }
                                else
                                    maxerr = 1;
                            }
                            else if (orignet & OBSTRUCT_S) {
                                offd = sdisty - obsInfoVal(c);
                                if (offd <= offmaxy[ds->layer]) {
                                    setObsVal(c, obsVal(c) | OFFSET_TAP);
                                    setOffsetVal(c, offd);
                                    setFlagsVal(c,
                                        (flagsVal(c) | NI_OFFSET_NS));

                                    // If position above has obstruction, then
                                    // add up/down block to prevent vias.

                                    if ((ds->layer < (int)numLayers() - 1) &&
                                            (gridy <
                                            numChannelsY(ds->layer + 1) - 1) &&
                                            (obsVal(gridx, gridy + 1,
                                                ds->layer + 1)
                                            & OBSTRUCT_MASK))
                                        block_route(gridx, gridy, ds->layer,
                                            UP);
                                }
                                else
                                    maxerr = 1;
                            }
                            else if (orignet & OBSTRUCT_E) {
                                offd = -(sdistx - obsInfoVal(c));
                                if (offd >= -offmaxx[ds->layer]) {
                                    setObsVal(c, obsVal(c) | OFFSET_TAP);
                                    setOffsetVal(c, offd);
                                    setFlagsVal(c,
                                        (flagsVal(c) | NI_OFFSET_EW));

                                    // If position above has obstruction, then
                                    // add up/down block to prevent vias.

                                    if ((ds->layer < (int)numLayers() - 1) &&
                                            (gridx > 0) &&
                                            (obsVal(gridx - 1, gridy,
                                                ds->layer + 1)
                                            & OBSTRUCT_MASK))
                                        block_route(gridx, gridy, ds->layer,
                                            UP);
                                }
                                else
                                    maxerr = 1;
                            }
                            else if (orignet & OBSTRUCT_W) {
                                offd = sdistx - obsInfoVal(c);
                                if (offd <= offmaxx[ds->layer]) {
                                    setObsVal(c, obsVal(c) | OFFSET_TAP);
                                    setOffsetVal(c, offd);
                                    setFlagsVal(c,
                                        (flagsVal(c) | NI_OFFSET_EW));

                                    // If position above has obstruction, then
                                    // add up/down block to prevent vias.

                                    if ((ds->layer < (int)numLayers() - 1) &&
                                            (gridx <
                                                numChannelsX(ds->layer) - 1) &&
                                            (obsVal(gridx + 1, gridy,
                                                ds->layer + 1)
                                            & OBSTRUCT_MASK))
                                        block_route(gridx, gridy, ds->layer,
                                            UP);
                                }
                                else
                                    maxerr = 1;
                            }

                            if (maxerr == 1) {
#ifdef DEBUG_DA
                                db->emitMesg("disable b %d %d %d\n",
                                    gridx, gridy, ds->layer);
#endif
                                disable_gridpos(gridx, gridy, ds->layer);
                            }

                            // Diagnostic
                            else if (verbose() > 3) {
                                db->emitErrMesg("Port overlaps obstruction"
                                    " at grid %d %d, position %g %g\n",
                                    gridx, gridy,
                                    db->lefToMic(dx), db->lefToMic(dy));
                            }
                        }
                    }

                    if (dy > (ds->y1 - deltay) && gridy >= 0) {

                        // Area inside halo around defined pin geometry.
                        // Exclude areas already processed (areas inside
                        // some pin geometry have been marked with netnum)

                        // Also check that we are not about to define a
                        // route position for a pin on a layer above 0 that
                        // blocks a pin underneath it.

                        // Flag positions that pass a Euclidean distance check.
                        // epass = 1 indicates that position clears a
                        // Euclidean distance measurement.

                        bool epass = false;
                        lefu_t s = getRouteSpacing(ds->layer);

                        lefu_t edist, xp, yp;
                        if (dx < (ds->x1 + s - deltax)) {
                            xp = dx + deltax - s;
                            edist = (ds->x1 - xp) * (ds->x1 - xp);
                        }
                        else if (dx > (ds->x2 - s + deltax)) {
                            xp = dx - deltax + s;
                            edist = (xp - ds->x2) * (xp - ds->x2);
                        }
                        else
                            edist = 0;
                        if ((edist > 0) && (dy < (ds->y1 + s - deltay))) {
                            yp = dy + deltay - s;
                            edist += (ds->y1 - yp) * (ds->y1 - yp);
                        }
                        else if ((edist > 0) && (dy > (ds->y2 - s + deltay))) {
                            yp = dy - deltay + s;
                            edist += (yp - ds->y2) * (yp - ds->y2);
                        }
                        else
                            edist = 0;
                        if (edist >= (s * s))
                            epass = true;

                        lefu_t xdist = getRouteWidth(ds->layer)/2;

                        dbNode *n2 = 0;
                        if (ds->layer > 0)
                            n2 = nodeLoc(gridx, gridy, ds->layer - 1);
                        if (!n2)
                            n2 = nodeLoc(gridx, gridy, ds->layer);

                        else {
                            // Watch out for the case where a tap crosses
                            // over a different tap.  Don't treat the tap
                            // on top as if it is not there!

                            dbNode *n3;
                            n3 = nodeLoc(gridx, gridy, ds->layer);
                            if (n3 && n3 != node)
                                n2 = n3;
                        }

                        // Ignore my own node.
                        if (n2 == node)
                            n2 = 0;

                        u_int k = obsVal(gridx, gridy, ds->layer);

                        // In case of a port that is inaccessible from a grid
                        // point, or not completely overlapping it, the
                        // stub information will show how to adjust the
                        // route position to cleanly attach to the port.

                        u_int mask = STUBROUTE;
                        u_int dir = NI_STUB_NS | NI_STUB_EW;
                        lefu_t dist = 0;

#ifdef DEBUG_OT
printf("OT x0 %x %x %d\n", (k & ROUTED_NET_MASK), node->netnum, (n2 != 0));
#endif
                        if (((k & ROUTED_NET_MASK) != node->netnum)
                                && (n2 == 0)) {
#ifdef DEBUG_OT
printf("OT x1\n");
#endif

                            if (k & OBSTRUCT_MASK) {
                                lefu_t sdist = obsInfoVal(gridx, gridy,
                                    ds->layer);

                                // If the point is marked as close to an
                                // obstruction, we can declare this an
                                // offset tap if we are not on a corner.
                                // Because we cannot define both an offset
                                // and a stub simultaneously, if the distance
                                // to clear the obstruction does not make the
                                // route reach the tap, then we mark the grid
                                // position as unroutable.

                                if (dy >= (ds->y1 - xdist) &&
                                        dy <= (ds->y2 + xdist)) {
                                    if ((dx >= ds->x2) &&
                                            ((k & OBSTRUCT_MASK) ==
                                            OBSTRUCT_E)) {
                                        dist = sdist -
                                            getRouteKeepout(ds->layer);
                                        if ((dx - ds->x2 + dist) < xdist) {
                                            mask = OFFSET_TAP;
                                            dir = NI_OFFSET_EW;

                                            if ((ds->layer <
                                                    (int)numLayers() - 1) &&
                                                    (gridx > 0) &&
                                                    (obsVal(gridx - 1, gridy,
                                                    ds->layer + 1)
                                                    & OBSTRUCT_MASK))
                                                block_route(gridx, gridy,
                                                    ds->layer, UP);
                                        }
                                    }
                                    else if ((dx <= ds->x1) &&
                                            ((k & OBSTRUCT_MASK) ==
                                            OBSTRUCT_W)) {
                                        dist = getRouteKeepout(ds->layer) -
                                            sdist;
                                        if ((ds->x1 - dx - dist) < xdist) {
                                            mask = OFFSET_TAP;
                                            dir = NI_OFFSET_EW;

                                            if ((ds->layer <
                                                    (int)numLayers() - 1) &&
                                                    gridx <
                                                  (numChannelsX(ds->layer) - 1)
                                                    && (obsVal(gridx + 1, gridy,
                                                    ds->layer + 1)
                                                    & OBSTRUCT_MASK))
                                                block_route(gridx, gridy,
                                                    ds->layer, UP);
                                        }
                                    }
                                }    
                                if (dx >= (ds->x1 - xdist) &&
                                        dx <= (ds->x2 + xdist)) {
                                    if ((dy >= ds->y2) &&
                                            ((k & OBSTRUCT_MASK) ==
                                            OBSTRUCT_N)) {
                                        dist = sdist -
                                            getRouteKeepout(ds->layer);
                                        if ((dy - ds->y2 + dist) < xdist) {
                                            mask = OFFSET_TAP;
                                            dir = NI_OFFSET_NS;

                                            if ((ds->layer <
                                                    (int)numLayers() - 1) &&
                                                    gridy < 
                                                  (numChannelsY(ds->layer) - 1)
                                                    && (obsVal(gridx, gridy - 1,
                                                    ds->layer + 1)
                                                    & OBSTRUCT_MASK))
                                                block_route(gridx, gridy,
                                                    ds->layer, UP);
                                        }
                                    }
                                    else if ((dy <= ds->y1) &&
                                            ((k & OBSTRUCT_MASK) ==
                                            OBSTRUCT_S)) {
                                        dist = getRouteKeepout(ds->layer) -
                                            sdist;
                                        if ((ds->y1 - dy - dist) < xdist) {
                                            mask = OFFSET_TAP;
                                            dir = NI_OFFSET_NS;

                                            if ((ds->layer <
                                                    (int)numLayers() - 1) &&
                                                    (gridy > 0) &&
                                                    (obsVal(gridx, gridy + 1,
                                                    ds->layer + 1)
                                                    & OBSTRUCT_MASK))
                                                block_route(gridx, gridy,
                                                    ds->layer, UP);
                                        }
                                    }
                                }
                                // Otherwise, dir is left as NI_STUB_MASK.
                            }
                            else {

                                // Cleanly unobstructed area.  Define stub
                                // route from point to tap, with a route width
                                // overlap if necessary to avoid a DRC width
                                // violation.

                                if ((dx >= ds->x2) &&
                                        ((dx - ds->x2) > (dy - ds->y2)) &&
                                        ((dx - ds->x2) > (ds->y1 - dy))) {
                                    // West-pointing stub
                                    if ((dy - ds->y2) <= xdist &&
                                            (ds->y1 - dy) <= xdist) {
                                        // Within reach of tap rectangle
                                        mask = STUBROUTE;
                                        dir = NI_STUB_EW;
                                        dist = ds->x2 - dx;
                                        if (dy < (ds->y2 - xdist) &&
                                                dy > (ds->y1 + xdist)) {
                                            if (dx < ds->x2 + xdist)
                                                dist = 0;
                                        }
                                        else {
                                            dist -= 2*xdist;
                                        }
                                    }
                                }
                                else if ((dx <= ds->x1) &&
                                        ((ds->x1 - dx) > (dy - ds->y2)) &&
                                        ((ds->x1 - dx) > (ds->y1 - dy))) {
                                    // East-pointing stub
                                    if ((dy - ds->y2) <= xdist &&
                                            (ds->y1 - dy) <= xdist) {
                                        // Within reach of tap rectangle
                                        mask = STUBROUTE;
                                        dir = NI_STUB_EW;
                                        dist = ds->x1 - dx;
                                        if (dy < (ds->y2 - xdist) &&
                                                dy > (ds->y1 + xdist)) {
                                            if (dx > ds->x1 - xdist)
                                                dist = 0;
                                        }
                                        else {
                                            dist += 2*xdist;
                                        }
                                    }
                                }
                                else if ((dy >= ds->y2) &&
                                        ((dy - ds->y2) > (dx - ds->x2)) &&
                                        ((dy - ds->y2) > (ds->x1 - dx))) {
                                    // South-pointing stub
                                    if ((dx - ds->x2) <= xdist &&
                                            (ds->x1 - dx) <= xdist) {
                                        // Within reach of tap rectangle
                                        mask = STUBROUTE;
                                        dir = NI_STUB_NS;
                                        dist = ds->y2 - dy;
                                        if (dx < (ds->x2 - xdist) &&
                                                dx > (ds->x1 + xdist)) {
                                            if (dy < ds->y2 + xdist)
                                                dist = 0;
                                        }
                                        else {
                                            dist -= 2*xdist;
                                        }
                                    }
                                }
                                else if ((dy <= ds->y1) &&
                                        ((ds->y1 - dy) > (dx - ds->x2)) &&
                                        ((ds->y1 - dy) > (ds->x1 - dx))) {
                                    // North-pointing stub
                                    if ((dx - ds->x2) <= xdist &&
                                            (ds->x1 - dx) <= xdist) {
                                        // Within reach of tap rectangle
                                        mask = STUBROUTE;
                                        dir = NI_STUB_NS;
                                        dist = ds->y1 - dy;
                                        if (dx < (ds->x2 - xdist) &&
                                                dx > (ds->x1 + xdist)) {
                                            if (dy > ds->y1 - xdist)
                                                dist = 0;
                                        }
                                        else {
                                            dist += 2*xdist;
                                        }
                                    }
                                }

                                if ((mask == STUBROUTE) &&
                                        (dir == NI_STUB_MASK)) {

                                    // Outside of pin at a corner. 
                                    // First, if one direction is too
                                    // far away to connect to a pin,
                                    // then we must route the other
                                    // direction.

                                    if (dx < ds->x1 - xdist ||
                                            dx > ds->x2 + xdist) {
                                        if (dy >= ds->y1 - xdist &&
                                                dy <= ds->y2 + xdist) {
                                            mask = STUBROUTE;
                                            dir = NI_STUB_EW;
                                            dist = (ds->x1 + ds->x2)/2 - dx;
                                        }
                                    }
                                    else if (dy < ds->y1 - xdist ||
                                            dy > ds->y2 + xdist) {
                                        mask = STUBROUTE;
                                        dir = NI_STUB_NS;
                                        dist = (ds->y1 + ds->y2)/2 - dy;
                                    }

                                    // Otherwise we are too far away
                                    // at a diagonal to reach the pin
                                    // by moving in any single
                                    // direction.  To be pedantic, we
                                    // could define some jogged stub,
                                    // but for now, we just call the
                                    // point unroutable (leave dir =
                                    // NI_STUB_MASK).

                                    // To do:  Apply offset + stub
                                }
                            }

                            // Additional checks on stub routes.

                            // Stub distances of <= 1/2 route width are
                            // unnecessary, so don't create them.

#ifdef DEBUG_OT
printf("OT x2 %x\n", dir);
#endif
                            if (mask == STUBROUTE && (dir == NI_STUB_NS ||
                                    dir == NI_STUB_EW) &&
                                    (abs(dist) <= xdist)) {
                                mask = 0;
                                dir = 0;
                                dist = 0;
                            }
                            else if (mask == STUBROUTE && (dir == NI_STUB_NS ||
                                    dir == NI_STUB_EW)) {

                                // Additional check:  Sometimes the
                                // above checks put stub routes where
                                // they are not needed because the
                                // stub is completely covered by other
                                // tap geometry.  Take the stub area
                                // and remove parts covered by other
                                // tap rectangles.  If the entire stub
                                // is gone, then don't put a stub
                                // here.

                                dbDseg de;
                                if (dir == NI_STUB_NS) {
                                    de.x1 = dx - xdist;
                                    de.x2 = dx + xdist;
                                    if (dist > 0) {
                                        de.y1 = dy + xdist;
                                        de.y2 = dy + dist;
                                    }
                                    else {
                                        de.y1 = dy + dist;
                                        de.y2 = dy - xdist;
                                    }
                                }
                                if (dir == NI_STUB_EW) {
                                    de.y1 = dy - xdist;
                                    de.y2 = dy + xdist;
                                    if (dist > 0) {
                                        de.x1 = dx + xdist;
                                        de.x2 = dx + dist;
                                    }
                                    else {
                                        de.x1 = dx + dist;
                                        de.x2 = dx - xdist;
                                    }
                                }
#ifdef DEBUG_OT
printf("OT de %g %g %g %g\n", db->lefToMic(de.x1), db->lefToMic(de.y1),
    db->lefToMic(de.x2), db->lefToMic(de.y2));
#endif
                                // For any tap that overlaps the stub
                                // extension box, remove that part of
                                // the box.

                                bool errbox = true;
                                for (dbDseg *ds2 = g->taps[i]; ds2;
                                        ds2 = ds2->next) {
                                    if (ds2->layer != ds->layer)
                                        continue;

                                    if (ds2->x1 <= de.x1 && ds2->x2 >= de.x2 &&
                                        ds2->y1 <= de.y1 && ds2->y2 >= de.y2) {
                                        errbox = false; // Completely covered
#ifdef DEBUG_OT
printf("OT eb0\n");
#endif
                                        break;
                                    }

                                    // Look for partial coverage. 
                                    // Note that any change can cause
                                    // a change in the original two
                                    // conditionals, so we have to
                                    // keep evaluating those
                                    // conditionals.

                                    if (ds2->x1 < de.x2 && ds2->x2 > de.x1) {
                                        if (ds2->y1 < de.y2 &&
                                                ds2->y2 > de.y1) {
                                            if (ds2->x1 <= de.x1 &&
                                                    ds2->x2 < de.x2) {
                                                de.x1 = ds2->x2;
#ifdef DEBUG_OT
printf("OT eb1a %g\n", db->lefToMic(de.x1));
#endif
                                                if (ds2->x2 >= ds->x2) {
#ifdef DEBUG_OT
printf("OT eb1\n");
#endif
                                                    errbox = false;
                                                }
                                            }
                                        }
                                    }

                                    if (ds2->x1 < de.x2 && ds2->x2 > de.x1) {
                                        if (ds2->y1 < de.y2 &&
                                                ds2->y2 > de.y1) {
                                            if (ds2->x2 >= de.x2 &&
                                                    ds2->x1 > de.x1) {
                                                de.x2 = ds2->x1;
#ifdef DEBUG_OT
printf("OT eb2a %g\n", db->lefToMic(de.x2));
#endif
                                                if (ds2->x1 <= ds->x1) {
#ifdef DEBUG_OT
printf("OT eb2\n");
#endif
                                                    errbox = false;
                                                }
                                            }
                                        }
                                    }

                                    if (ds2->x1 < de.x2 && ds2->x2 > de.x1) {
                                        if (ds2->y1 < de.y2 &&
                                                ds2->y2 > de.y1) {
                                            if (ds2->y1 <= de.y1 &&
                                                    ds2->y2 < de.y2) {
                                                de.y1 = ds2->y2;
#ifdef DEBUG_OT
printf("OT eb3a %g\n", db->lefToMic(de.y1));
#endif
                                                if (ds2->y2 >= ds->y2) {
#ifdef DEBUG_OT
printf("OT eb3\n");
#endif
                                                    errbox = false;
                                                }
                                            }
                                        }
                                    }

                                    if (ds2->x1 < de.x2 && ds2->x2 > de.x1) {
                                        if (ds2->y1 < de.y2 &&
                                                ds2->y2 > de.y1) {
                                            if (ds2->y2 >= de.y2 &&
                                                    ds2->y1 > de.y1) {
                                                de.y2 = ds2->y1;
#ifdef DEBUG_OT
printf("OT eb4a %g\n", db->lefToMic(de.y2));
#endif
                                                if (ds2->y1 <= ds->y1) {
#ifdef DEBUG_OT
printf("OT eb4\n");
#endif
                                                    errbox = false;
                                                }
                                            }
                                        }
                                    }
                                }

                                // If nothing is left of the stub box,
                                // then remove the stub.

                                if (errbox == false) {
                                    mask = 0;
                                    dir = 0;
                                    dist = 0;
                                }
                            }
#ifdef DEBUG_OT
printf("OT x3 %x\n", dir);
#endif

                            mrGridCell c;
                            initGridCell(c, gridx, gridy, ds->layer);

                            if ((k < (u_int)numNets()) &&
                                    (dir != NI_STUB_MASK)) {
                                setObsVal(c, ((obsVal(c) & BLOCKED_MASK) |
                                      g->netnum[i] | mask));
                                setNodeLoc(c, node);
                                setNodeSav(c, node);
                                setFlagsVal(c, (flagsVal(c) | dir));
                            }
                            else if (obsVal(c) & NO_NET) {
                                // Keep showing an obstruction, but add the
                                // direction info and log the stub distance.
                                setObsVal(c, (obsVal(c) | mask));
                                setFlagsVal(c, (flagsVal(c) | dir));
                            }
                            else {
                                setObsVal(c, (obsVal(c) |
                                    (mask | (g->netnum[i] & ROUTED_NET_MASK))));
                                setFlagsVal(c, (flagsVal(c) | dir));
                            }
                            if (mask & STUBROUTE)
                                setStubVal(c, dist);
                            else if ((mask & OFFSET_TAP) || (dist != 0))
                                setOffsetVal(c, dist);

                            // Remove entries with
                            // NI_STUB_MASK---these are blocked-in
                            // taps that are not routable without
                            // causing DRC violations (formerly called
                            // STUBROUTE_X).

                            if (dir == NI_STUB_MASK) {
#ifdef DEBUG_DA
                                db->emitMesg("disable c %d %d %d\n",
                                    gridx, gridy, ds->layer);
#endif
                                disable_gridpos(gridx, gridy, ds->layer);
                            }
                        }
                        else if (!epass) {

                            // Position fails euclidean distance check.

                            u_int othernet = (k & ROUTED_NET_MASK);

                            if (othernet != 0 && othernet != node->netnum) {

                                // This location is too close to two different
                                // node terminals and should not be used

                                // If there is a stub, then we can't specify
                                // an offset tap, so just disable it.  If
                                // there is already an offset, then just
                                // disable it.  Otherwise, check if othernet
                                // could be routed using a tap offset.

                                // To avoid having to check all nearby
                                // geometry, place a restriction that the
                                // next grid point in the direction of the
                                // offset must be free (not a tap point of
                                // any net, including this one).  That is
                                // still "more restrictive than necessary",
                                // but since the alternative is an efficient
                                // area search for interacting geometry, this
                                // restriction will stand until an example
                                // comes along that requires the detailed
                                // search.

                                // Such an example has come along, leading to
                                // an additional relaxation allowing an offset
                                // if the neighboring channel does not have a
                                // node record.  This will probably need
                                // revisiting.
                            
                                if (k & PINOBSTRUCTMASK) {
#ifdef DEBUG_DA
                                    db->emitMesg("disable d %d %d %d\n",
                                        gridx, gridy, ds->layer);
#endif
                                    disable_gridpos(gridx, gridy, ds->layer);
                                }
                                else if (nodeSav(gridx, gridy, ds->layer)) {
                                    mrGridCell c;
                                    initGridCell(c, gridx, gridy, ds->layer);

                                    bool no_offsets = true;
                                    u_int offset_net;

                                    // By how much would a tap need to
                                    // be moved to clear the
                                    // obstructing geometry?

                                    // Check tap to right.

                                    if ((dx > ds->x2) && (gridx <
                                            numChannelsX(ds->layer) - 1)) {
                                        offset_net = obsVal(gridx + 1, gridy,
                                            ds->layer);
                                        if (offset_net == 0 ||
                                                offset_net == othernet) {
                                            xdist = getViaWidth(ds->layer,
                                                ds->layer, DIR_VERT)/2;
                                            dist = ds->x2 - dx + xdist +
                                                getRouteSpacing(ds->layer);
                                            mask = OFFSET_TAP;
                                            dir = NI_OFFSET_EW;
                                            setObsVal(c, (obsVal(c) | mask));
                                            setOffsetVal(c, dist);
                                            setFlagsVal(c, (flagsVal(c) | dir));
                                            no_offsets = false;

                                            if ((ds->layer <
                                                    (int)numLayers() - 1) &&
                                                    (gridx > 0) &&
                                                    (obsVal(gridx + 1, gridy,
                                                    ds->layer + 1)
                                                    & OBSTRUCT_MASK))
                                                block_route(gridx, gridy,
                                                    ds->layer, UP);
                                        }
                                    }

                                    // Check tap to left.

                                    if ((dx < ds->x1) && (gridx > 0)) {
                                        offset_net =
                                            obsVal(gridx - 1, gridy, ds->layer);
                                        if (offset_net == 0 ||
                                                offset_net == othernet) {
                                            xdist = getViaWidth(ds->layer,
                                                ds->layer, DIR_VERT)/2;
                                            dist = ds->x1 - dx - xdist -
                                                getRouteSpacing(ds->layer);
                                            mask = OFFSET_TAP;
                                            dir = NI_OFFSET_EW;
                                            setObsVal(c, (obsVal(c) | mask));
                                            setOffsetVal(c, dist);
                                            setFlagsVal(c, (flagsVal(c) | dir));
                                            no_offsets = false;

                                            if ((ds->layer <
                                                    (int)numLayers() - 1) &&
                                                    gridx <
                                                (numChannelsX(ds->layer) - 1) &&
                                                    (obsVal(gridx - 1, gridy,
                                                    ds->layer + 1)
                                                    & OBSTRUCT_MASK))
                                                block_route(gridx, gridy,
                                                    ds->layer, UP);
                                        }
                                    }

                                    // Check tap up.

                                    if ((dy > ds->y2) && (gridy <
                                            numChannelsY(ds->layer) - 1)) {
                                        offset_net =
                                            obsVal(gridx, gridy + 1, ds->layer);
                                        if (offset_net == 0 ||
                                                offset_net == othernet) {
                                            xdist = getViaWidth(ds->layer,
                                                ds->layer, DIR_HORIZ)/2;
                                            dist = ds->y2 - dy + xdist +
                                                getRouteSpacing(ds->layer);
                                            mask = OFFSET_TAP;
                                            dir = NI_OFFSET_NS;
                                            setObsVal(c, (obsVal(c) | mask));
                                            setOffsetVal(c, dist);
                                            setFlagsVal(c, (flagsVal(c) | dir));
                                            no_offsets = false;

                                            if ((ds->layer <
                                                    (int)numLayers() - 1) &&
                                                    (gridy > 0) &&
                                                    (obsVal(gridx, gridy + 1,
                                                    ds->layer + 1)
                                                    & OBSTRUCT_MASK))
                                                block_route(gridx, gridy,
                                                    ds->layer, UP);
                                        }
                                    }

                                    // Check tap down.

                                    if ((dy < ds->y1) && (gridy > 0)) {
                                        offset_net =
                                            obsVal(gridx, gridy - 1, ds->layer);
                                        if (offset_net == 0 ||
                                                offset_net == othernet) {
                                            xdist = getViaWidth(ds->layer,
                                                ds->layer, DIR_HORIZ)/2;
                                            dist = ds->y1 - dy - xdist -
                                                getRouteSpacing(ds->layer);
                                            mask = OFFSET_TAP;
                                            dir = NI_OFFSET_NS;
                                            setObsVal(c,(obsVal(c) | mask));
                                            setOffsetVal(c, dist);
                                            setFlagsVal(c, (flagsVal(c) | dir));
                                            no_offsets = false;

                                            if ((ds->layer <
                                                    (int)numLayers() - 1) &&
                                                    gridx <
                                                (numChannelsX(ds->layer) - 1) &&
                                                    (obsVal(gridx, gridy - 1,
                                                    ds->layer + 1)
                                                    & OBSTRUCT_MASK))
                                                block_route(gridx, gridy,
                                                    ds->layer, UP);
                                        }
                                    }

                                    // No offsets were possible, so disable the
                                    // position

                                    if (no_offsets) {
#ifdef DEBUG_DA
                                        db->emitMesg("disable e %d %d %d\n",
                                            gridx, gridy, ds->layer);
#endif
                                        disable_gridpos(gridx, gridy,
                                            ds->layer);
                                    }
                                }
                                else {
#ifdef DEBUG_DA
                                    db->emitMesg("disable f %d %d %d\n",
                                        gridx, gridy, ds->layer);
#endif
                                    disable_gridpos(gridx, gridy, ds->layer);
                                }
                            }

                            // If we are on a layer > 0, then this geometry
                            // may block or partially block a pin on layer
                            // zero.  Mark this point as belonging to the
                            // net with a stub route to it.
                            // NOTE:  This is possibly too restrictive
                            // May want to force a tap offset for vias on
                            // layer zero. . .

                            if ((ds->layer > 0) && n2 && (n2->netnum
                                    != node->netnum) && ((othernet == 0) ||
                                    (othernet == node->netnum))) {

                                mrGridCell c;
                                initGridCell(c, gridx, gridy, ds->layer);

                                xdist = getViaWidth(ds->layer, ds->layer,
                                    DIR_VERT)/2;
                                if ((dy + xdist + getRouteSpacing(ds->layer) >
                                        ds->y1) && (dy + xdist < ds->y1)) {
                                    if ((dx - xdist < ds->x2) &&
                                            (dx + xdist > ds->x1) &&
                                            (stubVal(c) == 0)) {
                                        setObsVal(c,
                                            ((obsVal(c) & BLOCKED_MASK) |
                                            node->netnum | STUBROUTE));
                                        setNodeLoc(c, node);
                                        setNodeSav(c, node);
                                        setStubVal(c, ds->y1 - dy);
                                        setFlagsVal(c,
                                            (flagsVal(c) | NI_STUB_NS));
                                    }
                                }
                                if ((dy - xdist - getRouteSpacing(ds->layer) <
                                        ds->y2) && (dy - xdist > ds->y2)) {
                                    if ((dx - xdist < ds->x2) &&
                                            (dx + xdist > ds->x1) &&
                                            (stubVal(c) == 0)) {
                                        setObsVal(c,
                                            ((obsVal(c) & BLOCKED_MASK) |
                                            node->netnum | STUBROUTE));
                                        setNodeLoc(c, node);
                                        setNodeSav(c, node);
                                        setStubVal(c, ds->y2 - dy);
                                        setFlagsVal(c,
                                            (flagsVal(c) | NI_STUB_NS));
                                    }
                                }

                                xdist = getViaWidth(ds->layer, ds->layer,
                                    DIR_HORIZ)/2;
                                if ((dx + xdist + getRouteSpacing(ds->layer) >
                                        ds->x1) && (dx + xdist < ds->x1)) {
                                    if ((dy - xdist < ds->y2) &&
                                            (dy + xdist > ds->y1) &&
                                            (stubVal(c) == 0)) {
                                        setObsVal(c,
                                            ((obsVal(c) & BLOCKED_MASK) |
                                            node->netnum | STUBROUTE));
                                        setNodeLoc(c, node);
                                        setNodeSav(c, node);
                                        setStubVal(c, ds->x1 - dx);
                                        setFlagsVal(c,
                                            (flagsVal(c) | NI_STUB_EW));
                                    }
                                }
                                if ((dx - xdist - getRouteSpacing(ds->layer) <
                                        ds->x2) && (dx - xdist > ds->x2)) {
                                    if ((dy - xdist < ds->y2) &&
                                            (dy + xdist > ds->y1) &&
                                            (stubVal(c) == 0)) {
                                        setObsVal(c,
                                            ((obsVal(c) & BLOCKED_MASK) |
                                            node->netnum | STUBROUTE));
                                        setNodeLoc(c, node);
                                        setNodeSav(c, node);
                                        setStubVal(c, ds->x2 - dx);
                                        setFlagsVal(c,
                                            (flagsVal(c) | NI_STUB_EW));
                                    }
                                }
                            }
                        }
                    }
                    gridy++;
                }
            }
            gridx++;
        }
    }
}


// tap_to_tap_interactions
//
// Similar to create_obstructions_from_nodes(), but looks at each
// node's tap geometry, looks at every grid point in a wider area
// surrounding the tap.  If any other node has an offset that would
// place it too close to this node's tap geometry, then we mark the
// other node as unroutable at that grid point.
//
void
cMRouter::tap_to_tap_interactions()
{
    for (u_int k = 0; k < numPins(); k++) {
        dbGate *g = nlPin(k);
        for (int i = 0; i < g->nodes; i++) {
            if (g->netnum[i])
                tap_to_tap_interactions(g, i);
        }
    }
    for (u_int k = 0; k < numGates(); k++) {
        dbGate *g = nlGate(k);
        for (int i = 0; i < g->nodes; i++) {
            if (g->netnum[i])
                tap_to_tap_interactions(g, i);
        }
    }
}


//#define DEBUG_TT

void
cMRouter::tap_to_tap_interactions(dbGate *g, int i)
{
    int net = g->netnum[i];
    if (!net)
        return;
    for (dbDseg *ds = g->taps[i]; ds; ds = ds->next) {

        int mingridx = (ds->x1 - xLower()) / pitchX(ds->layer) - 1;
        if (mingridx < 0)
            mingridx = 0;
        int maxgridx = (ds->x2 - xLower()) / pitchX(ds->layer) + 2;
        if (maxgridx >= numChannelsX(ds->layer))
            maxgridx = numChannelsX(ds->layer) - 1;
        int mingridy = (ds->y1 - yLower()) / pitchY(ds->layer) - 1;
        if (mingridy < 0)
            mingridy = 0;
        int maxgridy = (ds->y2 - yLower()) / pitchY(ds->layer) + 2;
        if (maxgridy >= numChannelsY(ds->layer))
            maxgridy = numChannelsY(ds->layer) - 1;
#ifdef DEBUG_TT
printf("TT %s/%s %g %g %g %g %d - %d %d %d %d %g %g\n", g->gatename, g->node[i],
    db->lefToMic(ds->x1), db->lefToMic(ds->y1),
    db->lefToMic(ds->x2), db->lefToMic(ds->y2), ds->layer,
    mingridx, maxgridx, mingridy, maxgridy,
    db->lefToMic(yLower()), db->lefToMic(pitchY(ds->layer)));
#endif

        dbDseg de;
        for (int gridx = mingridx; gridx <= maxgridx; gridx++) {
            for (int gridy = mingridy; gridy <= maxgridy; gridy++) {

                // Is there an offset tap at this position, and
                // does it belong to a net that is != net?

                mrGridCell c;
                initGridCell(c, gridx, gridy, ds->layer);
                int orignet = obsVal(c);
                if (orignet & OFFSET_TAP) {
                    orignet &= ROUTED_NET_MASK;
                    if (orignet != net) {

                        lefu_t dx = (gridx * pitchX(ds->layer)) + xLower();
                        lefu_t dy = (gridy * pitchY(ds->layer)) + yLower();

                        lefu_t dist = offsetVal(c);

                        // "de" is the bounding box of a via placed
                        // at (gridx, gridy) and offset as specified.
                        // Expanded by metal spacing requirement.

                        de.x1 = dx - haloX(ds->layer);
                        de.x2 = dx + haloX(ds->layer);
                        de.y1 = dy - haloY(ds->layer);
                        de.y2 = dy + haloY(ds->layer);

                        u_int fl = flagsVal(c);
                        if (fl & NI_OFFSET_NS) {
                            de.y1 += dist;
                            de.y2 += dist;
                        }
                        else if (fl & NI_OFFSET_EW) {
                            de.x1 += dist;
                            de.x2 += dist;
                        }

#ifdef DEBUG_TT
printf("TTx %d %d  %g %g %g %g\n", gridx, gridy,
    db->lefToMic(de.x1), db->lefToMic(de.y1),
    db->lefToMic(de.x2), db->lefToMic(de.y2));
#endif

                        // Does the via bounding box interact with
                        // the tap geometry?

                        if ((de.x1 < ds->x2) && (ds->x1 < de.x2) &&
                                (de.y1 < ds->y2) && (ds->y1 < de.y2)) {
#ifdef DEBUG_DA
                            db->emitMesg("disable g %d %d %d\n",
                                gridx, gridy, ds->layer);
#endif
                            disable_gridpos(gridx, gridy, ds->layer);
                        }
                    }
                }
            }
        }
    }
}


// create_obstructions_from_variable_pitch
//
// Although it would be nice to have an algorithm that would work with
// any arbitrary pitch, qrouter will work around having larger pitches
// on upper metal layers by selecting 1 out of every N tracks for
// routing, and placing obstructions in the interstices.  This makes
// the possibly unwarranted assumption that the contact down to the
// layer below does not cause spacing violations to neighboring
// tracks.  If that assumption fails, this function will have to be
// revisited.
//
void
cMRouter::create_obstructions_from_variable_pitch()
{
    for (u_int l = 0; l < numLayers(); l++) {

        int hnum, vnum;
        db->checkVariablePitch(l, &hnum, &vnum);

        // This could be better handled by restricting access from
        // specific directions rather than marking a position as
        // NO_NET.  Since the function below will mark no positions
        // restricted if either hnum is 1 or vnum is 1, regardless of
        // the other value, then we force both values to be at least
        // 2.

        if (vnum > 1 && hnum == 1)
            hnum++;
        if (hnum > 1 && vnum == 1)
            vnum++;

        if (vnum > 1 || hnum > 1) {
            for (int x = 0; x < numChannelsX(l); x++) {
                if (x % hnum == 0)
                    continue;
                for (int y = 0; y < numChannelsY(l); y++) {
                    if (y % vnum == 0)
                        continue;

                    mrGridCell c;
                    initGridCell(c, x, y, l);

                    // If the grid position itself is a node, don't
                    // restrict routing based on variable pitch.
                    if (nodeLoc(c) != 0)
                        continue;

                    // If there is a node in an adjacent grid then
                    // allow routing from that direction.

                    if ((x > 0) && nodeLoc(x - 1, y, l) != 0)
                        setObsVal(c, BLOCKED_MASK & ~BLOCKED_W);
                    else if ((y > 0) && nodeLoc(x , y - 1, l) != 0)
                        setObsVal(c, BLOCKED_MASK & ~BLOCKED_S);
                    else if ((x < numChannelsX(l) - 1)
                            && nodeLoc(x + 1, y, l) != 0)
                        setObsVal(x, y, l, BLOCKED_MASK & ~BLOCKED_E);
                    else if ((y < numChannelsY(l) - 1)
                            && nodeLoc(x, y + 1, l) != 0)
                        setObsVal(c, BLOCKED_MASK & ~BLOCKED_N);
                    else
                        setObsVal(c, NO_NET);
                }
            }
        }
    }
}


// adjust_stub_lengths
//
// Makes an additional pass through the tap and obstruction databases,
// checking geometry against the potential stub routes for DRC spacing
// violations.  Adjust stub routes as necessary to resolve the DRC
// error(s).
//
// AUTHOR:  Tim Edwards, April 2013
//
void
cMRouter::adjust_stub_lengths()
{
    // For each node terminal (gate pin), look at the surrounding grid
    // points.  If any define a stub route or an offset, check if the
    // stub geometry or offset geometry would create a DRC spacing
    // violation.  If so, adjust the stub route to resolve the error. 
    // If the error cannot be resolved, mark the position as
    // unroutable.  If it is the ONLY grid point accessible to the
    // pin, keep it as-is and flag a warning.

    // Unlike blockage-finding function, which look in an area of a
    // size equal to the DRC interaction distance around a tap
    // rectangle, this function looks out one grid pitch in each
    // direction, to catch information about stubs that may terminate
    // within a DRC interaction distance of the tap rectangle.

    for (u_int k = 0; k < numPins(); k++) {
        dbGate *g = nlPin(k);
        for (int i = 0; i < g->nodes; i++) {
            if (g->netnum[i])
                adjust_stub_lengths(g, i);
        }
    }
    for (u_int k = 0; k < numGates(); k++) {
        dbGate *g = nlGate(k);
        for (int i = 0; i < g->nodes; i++) {
            if (g->netnum[i])
                adjust_stub_lengths(g, i);
        }
    }
}


//#define DEBUG_AS

void
cMRouter::adjust_stub_lengths(dbGate *g, int i)
{
    dbDseg dt, de;

    // Get the node record associated with this pin.
    dbNode *node = g->noderec[i];
    if (!node)
        return;

    // Work through each rectangle in the tap geometry

    for (dbDseg *ds = g->taps[i]; ds; ds = ds->next) {
        lefu_t wx = getViaWidth(ds->layer, ds->layer, DIR_VERT)/2;
        lefu_t wy = getViaWidth(ds->layer, ds->layer, DIR_HORIZ)/2;
        lefu_t s = getRouteSpacing(ds->layer);
        int gridx = (ds->x1 - xLower()) / pitchX(ds->layer) - 2;
        for (;;) {
            lefu_t dx = (gridx * pitchX(ds->layer)) + xLower();
            if (dx > (ds->x2 + pitchX(ds->layer)) ||
                    gridx >= numChannelsX(ds->layer))
                break;
            else if (dx >= (ds->x1 - pitchX(ds->layer)) && gridx >= 0) {
                int gridy = (ds->y1 - yLower()) / pitchY(ds->layer) - 2;
                for (;;) {
                    lefu_t dy = (gridy * pitchY(ds->layer)) + yLower();
                    if (dy > (ds->y2 + pitchY(ds->layer)) ||
                            gridy >= numChannelsY(ds->layer))
                        break;
                    if (dy >= (ds->y1 - pitchY(ds->layer)) && gridy >= 0) {

                        mrGridCell c;
                        initGridCell(c, gridx, gridy, ds->layer);
                        int orignet = obsVal(c);

                        // Ignore this location if it is assigned to another
                        // net, or is assigned to NO_NET.

                        if ((orignet & ROUTED_NET_MASK) != node->netnum) {
                            gridy++;
                            continue;
                        }

                        // Even if it's on the same net, we need to check
                        // if the stub is to this node, otherwise it is not
                        // an issue.
                        if (nodeSav(c) != node) {
                            gridy++;
                            continue;
                        }

                        // NI_STUB_MASK are unroutable;  leave them alone
                        if (orignet & STUBROUTE) {
                            if ((flagsVal(c) &
                                    NI_OFFSET_MASK) == NI_OFFSET_MASK) {
                                gridy++;
                                continue;
                            }
                        }

                        // Define a route box around the grid point.

                        dt.x1 = dx - wx;
                        dt.x2 = dx + wx;
                        dt.y1 = dy - wy;
                        dt.y2 = dy + wy;

                        // Adjust the route box according to the stub
                        // or offset geometry, provided that the stub
                        // is longer than the route box.

                        if (orignet & OFFSET_TAP) {
                            lefu_t dist = offsetVal(c);
                            if (flagsVal(c) & NI_OFFSET_EW) {
                                dt.x1 += dist;
                                dt.x2 += dist;
                            }
                            else if (flagsVal(c) & NI_OFFSET_NS) {
                                dt.y1 += dist;
                                dt.y2 += dist;
                            }
                        }
                        else if (orignet & STUBROUTE) {
                            lefu_t dist = stubVal(c);
                            if (flagsVal(c) & NI_STUB_EW) {
                                if (dist > 0) {
                                    if (dx + dist > dt.x2)
                                        dt.x2 = dx + dist;
                                }
                                else {
                                    if (dx + dist < dt.x1)
                                        dt.x1 = dx + dist;
                                }
                            }
                            else if (flagsVal(c) & NI_STUB_NS) {
                                if (dist > 0) {
                                    if (dy + dist > dt.y2)
                                        dt.y2 = dy + dist;
                                }
                                else {
                                    if (dy + dist < dt.y1)
                                        dt.y1 = dy + dist;
                                }
                            }
                        }

                        de = dt;

                        // Check for DRC spacing interactions between
                        // the tap box and the route box.

                        bool errbox = false;
                        if ((dt.y1 - ds->y2) > 0 && (dt.y1 - ds->y2) < s) {
                            if (ds->x2 > (dt.x1 - s) && ds->x1 < (dt.x2 + s)) {
                                de.y2 = dt.y1;
                                de.y1 = ds->y2;
                                if (ds->x2 + s < dt.x2)
                                    de.x2 = ds->x2 + s;
                                if (ds->x1 - s > dt.x1)
                                    de.x1 = ds->x1 - s;
                                errbox = true;
                            }
                        }
                        else if ((ds->y1 - dt.y2) > 0 &&
                                (ds->y1 - dt.y2) < s) {
                            if (ds->x2 > (dt.x1 - s) && ds->x1 < (dt.x2 + s)) {
                                de.y1 = dt.y2;
                                de.y2 = ds->y1;
                                if (ds->x2 + s < dt.x2)
                                    de.x2 = ds->x2 + s;
                                if (ds->x1 - s > dt.x1)
                                    de.x1 = ds->x1 - s;
                                errbox = true;
                            }
                        }

                        if ((dt.x1 - ds->x2) > 0 && (dt.x1 - ds->x2) < s) {
                            if (ds->y2 > (dt.y1 - s) && ds->y1 < (dt.y2 + s)) {
                                de.x2 = dt.x1;
                                de.x1 = ds->x2;
                                if (ds->y2 + s < dt.y2)
                                    de.y2 = ds->y2 + s;
                                if (ds->y1 - s > dt.y1)
                                    de.y1 = ds->y1 - s;
                                errbox = true;
                            }
                        }
                        else if ((ds->x1 - dt.x2) > 0 &&
                                (ds->x1 - dt.x2) < s) {
                            if (ds->y2 > (dt.y1 - s) && ds->y1 < (dt.y2 + s)) {
                                de.x1 = dt.x2;
                                de.x2 = ds->x1;
                                if (ds->y2 + s < dt.y2)
                                    de.y2 = ds->y2 + s;
                                if (ds->y1 - s > dt.y1)
                                    de.y1 = ds->y1 - s;
                                errbox = true;
                            }
                        }

                        if (errbox) {

                            // Chop areas off the error box that are
                            // covered by other taps of the same port.

                            for (dbDseg *ds2 = g->taps[i]; ds2;
                                    ds2 = ds2->next) {
                                if (ds2 == ds)
                                    continue;
                                if (ds2->layer != ds->layer)
                                    continue;

                                if (ds2->x1 <= de.x1 && ds2->x2 >= de.x2 &&
                                        ds2->y1 <= de.y1 && ds2->y2 >= de.y2) {
                                    errbox = false;   // Completely covered
                                    break;
                                }

                                // Look for partial coverage.  Note that any
                                // change can cause a change in the original
                                // two conditionals, so we have to keep
                                // evaluating those conditionals.

                                if (ds2->x1 < de.x2 && ds2->x2 > de.x1) {
                                    if (ds2->y1 < de.y2 && ds2->y2 > de.y1) {
                                        if (ds2->x1 <= de.x1 &&
                                                ds2->x2 < de.x2) {
                                            de.x1 = ds2->x2;
                                            if (ds2->x2 >= ds->x2)
                                                errbox = false;
                                        }
                                    }
                                }

                                if (ds2->x1 < de.x2 && ds2->x2 > de.x1) {
                                    if (ds2->y1 < de.y2 && ds2->y2 > de.y1) {
                                        if (ds2->x2 >= de.x2 &&
                                                ds2->x1 > de.x1) {
                                            de.x2 = ds2->x1;
                                            if (ds2->x1 <= ds->x1)
                                                errbox = false;
                                        }
                                    }
                                }

                                if (ds2->x1 < de.x2 && ds2->x2 > de.x1) {
                                    if (ds2->y1 < de.y2 && ds2->y2 > de.y1) {
                                        if (ds2->y1 <= de.y1 &&
                                                ds2->y2 < de.y2) {
                                            de.y1 = ds2->y2;
                                            if (ds2->y2 >= ds->y2)
                                                errbox = false;
                                        }
                                    }
                                }

                                if (ds2->x1 < de.x2 && ds2->x2 > de.x1) {
                                    if (ds2->y1 < de.y2 && ds2->y2 > de.y1) {
                                        if (ds2->y2 >= de.y2 &&
                                                ds2->y1 > de.y1) {
                                            de.y2 = ds2->y1;
                                            if (ds2->y1 <= ds->y1)
                                                errbox = false;
                                        }
                                    }
                                }
                            }
                        }

                        // Any area left over is a potential DRC error.

                        if ((de.x2 <= de.x1) || (de.y2 <= de.y1))
                            errbox = false;
    
                        if (errbox) {

                            // Create stub route to cover error box,
                            // or if possible, stretch existing stub
                            // route to cover error box.

                            // Allow EW stubs to be changed to NS
                            // stubs and vice versa if the original
                            // stub length was less than a route
                            // width.  This means the grid position
                            // makes contact without the stub.  Moving
                            // the stub to another side should not
                            // create an error.

                            // NOTE:  error box must touch ds
                            // geometry, and by more than just a
                            // point.
                                  
                            // 8/31/2016:
                            // If DRC violations are created on two
                            // adjacent sides, then create both a stub
                            // route and a tap offset.  Put the stub
                            // route in the preferred metal direction
                            // of the layer, and set the tap offset to
                            // prevent the DRC error in the other
                            // direction.
                            //
                            // 10/3/2016:
                            // The tap offset can be set either by
                            // moving toward the obstructing edge to
                            // remove the gap, or moving away from it
                            // to avoid the DRC spacing error.  Choose
                            // the one that offsets by the smaller
                            // distance.

                            if ((de.x2 > dt.x2) && (de.y1 < ds->y2) &&
                                    (de.y2 > ds->y1)) {
#ifdef DEBUG_AS
printf("ASxa %x %x\n", orignet, flagsVal(c));
#endif
                                if ((orignet & STUBROUTE) == 0) {
                                    setObsVal(c, (obsVal(c) | STUBROUTE));
#ifdef DEBUG_AS
printf("ASa %x %x\n", orignet, obsVal(c));
#endif
                                    setStubVal(c, de.x2 - dx);
                                    setFlagsVal(c,
                                        (flagsVal(c) | NI_STUB_EW));
                                    errbox = false;
                                }
                                else if ((orignet & STUBROUTE) &&
                                        (flagsVal(c) & NI_STUB_EW)) {
                                    // Beware, if dist > 0 then this reverses
                                    // the stub.  For U-shaped ports may need
                                    // to have separate E and W stubs.
                                    setStubVal(c, de.x2 - dx);
                                    errbox = false;
                                }
                                else if ((orignet & STUBROUTE) &&
                                        (flagsVal(c) & NI_STUB_NS)) {

                                    // If preferred route direction is
                                    // horizontal, then change the stub.

                                    setObsVal(c, (obsVal(c) | OFFSET_TAP));
#ifdef DEBUG_AS
printf("ASb %x %x\n", orignet, obsVal(c));
#endif
                                    if (getRouteOrientation(ds->layer) ==
                                            DIR_HORIZ) {
                                        setFlagsVal(c,
                                            (NI_OFFSET_NS | NI_STUB_EW));
                                        // lnode->offset = lnode->stub;  // ?
                                        if (stubVal(c) > 0) {
                                            lefu_t ofst = de.y2 - dy - wy;
                                            if (ofst > s - ofst)
                                                ofst -= s;
                                            setOffsetVal(c, ofst);
                                        }
                                        else {
                                            lefu_t ofst = de.y1 - dy + wy;
                                            if (-ofst > s + ofst)
                                                ofst += s;
                                            setOffsetVal(c, ofst);
                                        }
                                        setStubVal(c, de.x2 - dx);
                                        errbox = false;
                                    }
                                    else {
                                        // Add the offset.
                                        lefu_t ofst = de.x2 - dx - wx;
                                        if (ofst > s - ofst)
                                            ofst -= s;
                                        setOffsetVal(c, ofst);
                                        setFlagsVal(c,
                                            (flagsVal(c) | NI_OFFSET_EW));
                                        errbox = false;
                                    }
                                }
                            }
                            else if ((de.x1 < dt.x1) && (de.y1 < ds->y2) &&
                                    (de.y2 > ds->y1)) {
#ifdef DEBUG_AS
printf("ASxb %x %x\n", orignet, flagsVal(c));
#endif
                                if ((orignet & STUBROUTE) == 0) {
                                    setObsVal(c, (obsVal(c) | STUBROUTE));
#ifdef DEBUG_AS
printf("ASc %x %x\n", orignet, obsVal(c));
#endif
                                    setStubVal(c, de.x1 - dx);
                                    setFlagsVal(c,
                                        (flagsVal(c) | NI_STUB_EW));
                                    errbox = false;
                                }
                                else if ((orignet & STUBROUTE) &&
                                        (flagsVal(c) & NI_STUB_EW)) {
                                    // Beware, if dist > 0 then this reverses
                                    // the stub.  For U-shaped ports may need
                                    // to have separate E and W stubs.
                                    setStubVal(c, de.x1 - dx);
                                    errbox = false;
                                }
                                else if ((orignet & STUBROUTE) &&
                                        (flagsVal(c) & NI_STUB_NS)) {

                                    // If preferred route direction is
                                    // horizontal, then change the stub.

                                    setObsVal(c, (obsVal(c) | OFFSET_TAP));
#ifdef DEBUG_AS
printf("ASd %x %x\n", orignet, obsVal(c));
#endif
                                    if (getRouteOrientation(ds->layer) ==
                                            DIR_HORIZ) {
                                        setFlagsVal(c,
                                            (NI_OFFSET_NS | NI_STUB_EW));
                                        // lnode->offset = lnode->stub;  // ?
                                        if (stubVal(c) > 0) {
                                            lefu_t ofst = de.y2 - dy - wy;
                                            if (ofst > s - ofst)
                                                ofst -= s;
                                            setOffsetVal(c, ofst);
                                        }
                                        else {
                                            lefu_t ofst = de.y1 - dy + wy;
                                            if (-ofst > s + ofst)
                                                ofst += s;
                                            setOffsetVal(c, ofst);
                                        }
                                        setStubVal(c, de.x1 - dx);
                                        errbox = false;
                                    }
                                    else {
                                        // Add the offset
                                        lefu_t ofst = de.x1 - dx + wx;
                                        if (-ofst > s + ofst)
                                            ofst += s;
                                        setOffsetVal(c, ofst);
                                        setFlagsVal(c,
                                            (flagsVal(c) | NI_OFFSET_EW));
                                        errbox = false;
                                    }
                                }
                            }
                            else if ((de.y2 > dt.y2) && (de.x1 < ds->x2) &&
                                    (de.x2 > ds->x1)) {
                                if ((orignet & STUBROUTE) == 0) {
                                    setObsVal(c, (obsVal(c) | STUBROUTE));
#ifdef DEBUG_AS
printf("ASe %x %x\n", orignet, obsVal(c));
#endif
                                    setStubVal(c, de.y2 - dy);
                                    setFlagsVal(c,
                                        (flagsVal(c) | NI_STUB_NS));
                                    errbox = false;
                                }
                                else if ((orignet & STUBROUTE) &&
                                        (flagsVal(c) & NI_STUB_NS)) {
                                    // Beware, if dist > 0 then this reverses
                                    // the stub.  For C-shaped ports may need
                                    // to have separate N and S stubs.
                                    setStubVal(c, de.y2 - dy);
                                    errbox = false;
                                }
                                else if ((orignet & STUBROUTE) &&
                                        (flagsVal(c) & NI_STUB_EW)) {
                                    // If preferred route direction is
                                    // vertical, then change the stub

                                    setObsVal(c, (obsVal(c) | OFFSET_TAP));
#ifdef DEBUG_AS
printf("ASf %x %x\n", orignet, obsVal(c));
#endif
                                    if (getRouteOrientation(ds->layer) ==
                                            DIR_VERT) {
                                        setFlagsVal(c,
                                            (NI_OFFSET_EW | NI_STUB_NS));
                                        // lnode->offset = lnode->stub;  // ?
                                        if (stubVal(c) > 0) {
                                            lefu_t ofst = de.x2 - dx - wx;
                                            if (ofst > s - ofst)
                                                ofst -= s;
                                            setOffsetVal(c, ofst);
                                        }
                                        else {
                                            lefu_t ofst = de.x1 - dx + wx;
                                            if (-ofst > s + ofst)
                                                ofst += s;
                                            setOffsetVal(c, ofst);
                                        }
                                        setStubVal(c, de.y2 - dy);
                                        errbox = false;
                                    }
                                    else {
                                        // Add the offset
                                        lefu_t ofst = de.y2 - dy - wy;
                                        if (ofst > s - ofst)
                                            ofst -= s;
                                        setOffsetVal(c, ofst);
                                        setFlagsVal(c,
                                            (flagsVal(c) | NI_OFFSET_NS));
                                        errbox = false;
                                    }
                                }
                            }
                            else if ((de.y1 < dt.y1) && (de.x1 < ds->x2) &&
                                    (de.x2 > ds->x1)) {
                                if ((orignet & STUBROUTE) == 0) {
                                    setObsVal(c, (obsVal(c) | STUBROUTE));
#ifdef DEBUG_AS
printf("ASg %x %x\n", orignet, obsVal(c));
#endif
                                    setStubVal(c, de.y1 - dy);
                                    setFlagsVal(c,
                                        (flagsVal(c) | NI_STUB_NS));
                                    errbox = false;
                                }
                                else if ((orignet & STUBROUTE) &&
                                        (flagsVal(c) & NI_STUB_NS)) {
                                    // Beware, if dist > 0 then this reverses
                                    // the stub.  For C-shaped ports may need
                                    // to have separate N and S stubs.
                                    setStubVal(c, de.y1 - dy);
                                    errbox = false;
                                }
                                else if ((orignet & STUBROUTE) &&
                                        (flagsVal(c) & NI_STUB_EW)) {
                                    // If preferred route direction is
                                    // vertical, then change the stub

                                    setObsVal(c, (obsVal(c) | OFFSET_TAP));
#ifdef DEBUG_AS
printf("ASh %x %x\n", orignet, obsVal(c));
#endif
                                    if (getRouteOrientation(ds->layer) ==
                                            DIR_VERT) {
                                        setFlagsVal(c,
                                            (NI_OFFSET_EW | NI_STUB_NS));
                                        // lnode->offset = lnode->stub;  // ?
                                        if (stubVal(c) > 0) {
                                            lefu_t ofst = de.x2 - dx - wx;
                                            if (ofst > s - ofst)
                                                ofst -= s;
                                            setOffsetVal(c, ofst);
                                        }
                                        else {
                                            lefu_t ofst = de.x1 - dx + wx;
                                            if (-ofst > s + ofst)
                                                ofst += s;
                                            setOffsetVal(c, ofst);
                                        }
                                        setStubVal(c, de.y1 - dy + wy);
                                        errbox = false;
                                    }
                                    else {
                                        // Add the offset
                                        lefu_t ofst = de.y1 - dy + wy;
                                        if (-ofst > s + ofst)
                                            ofst += s;
                                        setOffsetVal(c, ofst);
                                        setFlagsVal(c,
                                            (flagsVal(c) | NI_OFFSET_NS));
                                        errbox = false;
                                    }
                                }
                            }

                            // Where the error box did not touch the stub
                            // route, there is assumed to be no error.

                            if (errbox) {
                                if ((de.x2 > dt.x2) || (de.x1 < dt.x1) ||
                                        (de.y2 > dt.y2) || (de.y1 < dt.y1))
                                    errbox = false;
                            }

                            if (errbox) {
                                // Unroutable position, so mark it unroutable.
                                setObsVal(c, (obsVal(c) | STUBROUTE));
#ifdef DEBUG_AS
printf("ASi %x %x\n", orignet, obsVal(c));
#endif
                                setFlagsVal(c, NI_STUB_MASK);
                            }
                        }
                    }
                    gridy++;
                }
            }
            gridx++;
        }
    }
}


// find_route_blocks
//
// Search tap geometry for edges that cause DRC spacing errors with
// route edges.  This specifically checks edges of the route tracks,
// not the intersection points.  If a tap would cause an error with a
// route segment, the grid points on either end of the segment are
// flagged to prevent generating a route along that specific segment.
//
void
cMRouter::find_route_blocks()
{
    for (u_int k = 0; k < numPins(); k++) {
        dbGate *g = nlPin(k);
        for (int i = 0; i < g->nodes; i++) {
            if (g->netnum[i])
                find_route_blocks(g, i);
        }
    }
    for (u_int k = 0; k < numGates(); k++) {
        dbGate *g = nlGate(k);
        for (int i = 0; i < g->nodes; i++) {
            if (g->netnum[i])
                find_route_blocks(g, i);
        }
    }
}


void
cMRouter::find_route_blocks(dbGate *g, int i)
{
    dbDseg dt;
    // Work through each rectangle in the tap geometry.

    for (dbDseg *ds = g->taps[i]; ds; ds = ds->next) {
        lefu_t w = getRouteWidth(ds->layer)/2;
        lefu_t v = getViaWidth(ds->layer, ds->layer, DIR_VERT)/2;
        lefu_t s = getRouteSpacing(ds->layer);

        // Look west.

        int gridx = (ds->x1 - xLower()) / pitchX(ds->layer);
        lefu_t dx = (gridx * pitchX(ds->layer)) + xLower();
        lefu_t dist = ds->x1 - dx - w;
        if (dist > 0 && dist < s && gridx >= 0) {
            dt.x1 = dt.x2 = dx;
            dt.y1 = ds->y1;
            dt.y2 = ds->y2;

            // Check for other taps covering this edge
            // (to do).

            // Find all grid points affected.
            int gridy = (ds->y1 - yLower() - pitchY(ds->layer)) /
                pitchY(ds->layer);
            lefu_t dy = (gridy * pitchY(ds->layer)) + yLower();
            while (dy < ds->y1 - s) {
                dy += pitchY(ds->layer);
                gridy++;
            }
            while (dy < ds->y2 + s) {
                mrGridCell c;
                initGridCell(c, gridx, gridy, ds->layer);

                lefu_t u = ((obsVal(c) & STUBROUTE) &&
                    (flagsVal(c) & NI_STUB_EW)) ? v : w;
                if (dy < ds->y2 - u)
                    block_route(gridx, gridy, ds->layer, NORTH);
                if (dy > ds->y1 + u)
                    block_route(gridx, gridy, ds->layer, SOUTH);
                dy += pitchY(ds->layer);
                gridy++;
            }
        }

        // Look east.

        gridx = 1 + (ds->x2 - xLower()) / pitchX(ds->layer);
        dx = (gridx * pitchX(ds->layer)) + xLower();
        dist = dx - ds->x2 - w;
        if (dist > 0 && dist < s && gridx < numChannelsX(ds->layer)) {
            dt.x1 = dt.x2 = dx;
            dt.y1 = ds->y1;
            dt.y2 = ds->y2;

            // Check for other taps covering this edge
            // (to do).

            // Find all grid points affected.
            int gridy = (ds->y1 - yLower() - pitchY(ds->layer)) /
                pitchY(ds->layer);
            lefu_t dy = (gridy * pitchY(ds->layer)) + yLower();
            while (dy < ds->y1 - s) {
                dy += pitchY(ds->layer);
                gridy++;
            }
            while (dy < ds->y2 + s) {
                mrGridCell c;
                initGridCell(c, gridx, gridy, ds->layer);

                lefu_t u = ((obsVal(c) & STUBROUTE) &&
                    (flagsVal(c) & NI_STUB_EW)) ? v : w;
                if (dy < ds->y2 - u)
                    block_route(gridx, gridy, ds->layer, NORTH);
                if (dy > ds->y1 + u)
                    block_route(gridx, gridy, ds->layer, SOUTH);
                dy += pitchY(ds->layer);
                gridy++;
            }
        }

        // Look south.

        int gridy = (ds->y1 - yLower()) / pitchY(ds->layer);
        lefu_t dy = (gridy * pitchY(ds->layer)) + yLower();
        dist = ds->y1 - dy - w;
        if (dist > 0 && dist < s && gridy >= 0) {
            dt.x1 = ds->x1;
            dt.x2 = ds->x2;
            dt.y1 = dt.y2 = dy;

            // Check for other taps covering this edge
            // (to do).

            // Find all grid points affected.
            gridx = (ds->x1 - xLower() - pitchX(ds->layer)) /
                pitchX(ds->layer);
            dx = (gridx * pitchX(ds->layer)) + xLower();
            while (dx < ds->x1 - s) {
                dx += pitchX(ds->layer);
                gridx++;
            }
            while (dx < ds->x2 + s) {
                mrGridCell c;
                initGridCell(c, gridx, gridy, ds->layer);

                lefu_t u = ((obsVal(c) & STUBROUTE) &&
                    (flagsVal(c) & NI_STUB_NS)) ? v : w;
                if (dx < ds->x2 - u)
                    block_route(gridx, gridy, ds->layer, EAST);
                if (dx > ds->x1 + u)
                    block_route(gridx, gridy, ds->layer, WEST);
                dx += pitchX(ds->layer);
                gridx++;
            }
        }

        // Look north.

        gridy = 1 + (ds->y2 - yLower()) / pitchY(ds->layer);
        dy = (gridy * pitchY(ds->layer)) + yLower();
        dist = dy - ds->y2 - w;
        if (dist > 0 && dist < s && gridy < numChannelsY(ds->layer)) {
            dt.x1 = ds->x1;
            dt.x2 = ds->x2;
            dt.y1 = dt.y2 = dy;

            // Check for other taps covering this edge
            // (to do).

            // Find all grid points affected.
            gridx = (ds->x1 - xLower() - pitchX(ds->layer)) /
                pitchX(ds->layer);
            dx = (gridx * pitchX(ds->layer)) + xLower();
            while (dx < ds->x1 - s) {
                dx += pitchX(ds->layer);
                gridx++;
            }
            while (dx < ds->x2 + s) {
                mrGridCell c;
                initGridCell(c, gridx, gridy, ds->layer);

                lefu_t u = ((obsVal(c) & STUBROUTE) &&
                    (flagsVal(c) & NI_STUB_NS)) ? v : w;
                if (dx < ds->x2 - u)
                    block_route(gridx, gridy, ds->layer, EAST);
                if (dx > ds->x1 + u)
                    block_route(gridx, gridy, ds->layer, WEST);
                dx += pitchX(ds->layer);
                gridx++;
            }
        }
    }
}


// count_reachable_taps
//
// For each grid point in the layout, find if it corresponds to a node
// and is unobstructed.  If so, increment the node's count of
// reachable taps.  Then work through the list of nodes and determine
// if any are completely unreachable.  If so, then unobstruct any
// position that is inside tap geometry that can contain a via.
//
// NOTE:  This function should check for tap rectangles that may
// combine to form an area large enough to place a via; also, it
// should check for tap points that are routable by a wire and not a
// tap.  However, those conditions are rare and are left unhandled for
// now.
//
void
cMRouter::count_reachable_taps()
{
    for (u_int l = 0; l < numLayers(); l++) {
        u_int sz = numChannelsX(l) * numChannelsY(l);
        for (u_int j = 0; j < sz; j++) {
            mrGridCell c;
            c.layer = l;
            c.index = j;
            dbNode *node = nodeLoc(c);
            if (node) {

                // Redundant check; if Obs has NO_NET set, then
                // nodeloc for that position should already be 0.

                if (!(obsAry(l)[j] & NO_NET))
                    node->numtaps++;
            }
        }
    }

    for (u_int k = 0; k < numPins(); k++) {
        dbGate *g = nlPin(k);
        for (int i = 0; i < g->nodes; i++)
            count_reachable_taps(g, i);
    }
    for (u_int k = 0; k < numGates(); k++) {
        dbGate *g = nlGate(k);
        for (int i = 0; i < g->nodes; i++)
            count_reachable_taps(g, i);
    }
}


void
cMRouter::count_reachable_taps(dbGate *g, int i)
{
    dbNode *node = g->noderec[i];
    if (!node)
        return;
    if (node->numnodes == 0)
        return;     // e.g., vdd or gnd bus
    if (node->numtaps == 0) {
        dbNet *nnet = getNetByNum(node->netnum);
        db->emitErrMesg(
            "Error: Node %s of net \"%s\" has no taps!\n",
            db->printNodeName(node), nnet ? nnet->netname : "unknown_net");

        for (dbDseg *ds = g->taps[i]; ds; ds = ds->next) {
            lefu_t deltax = getViaWidth(ds->layer, ds->layer, DIR_VERT)/2;
            lefu_t deltay = getViaWidth(ds->layer, ds->layer, DIR_HORIZ)/2;

            int gridx = (ds->x1 - xLower()) / pitchX(ds->layer) - 1;
            for (;;) {
                lefu_t dx = (gridx * pitchX(ds->layer)) + xLower();
                if (dx > ds->x2 || gridx >= numChannelsX(ds->layer))
                    break;

                if (((dx - ds->x1) >= deltax) && ((ds->x2 - dx) >= deltax)) {
                    int gridy = (ds->y1 - yLower()) / pitchY(ds->layer) - 1;
                    for (;;) {
                        lefu_t dy = (gridy * pitchY(ds->layer)) + yLower();
                        if (dy > ds->y2 || gridy >= numChannelsY(ds->layer))
                            break;

                        if (((dy - ds->y1) >= deltay) &&
                                ((ds->y2 - dy) >= deltay)) {

                            if ((ds->layer == (int)numLayers() - 1) ||
                                    !(obsVal(gridx, gridy,
                                    ds->layer + 1) & NO_NET)) {

                                // Grid position is clear for
                                // placing a via.

                                db->emitErrMesg(
                                    "Tap position (%g, %g) appears"
                                    " to be technically routable, so "
                                    "it is being forced routable.\n",
                                    db->lefToMic(dx), db->lefToMic(dy));

                                mrGridCell c;
                                initGridCell(c, gridx, gridy, ds->layer);
                                setObsVal(c, ((obsVal(c) & BLOCKED_MASK)
                                    | node->netnum));
                                setNodeLoc(c, node);
                                setNodeSav(c, node);
                                node->numtaps++;
                            }
                        }
                        gridy++;
                    }
                }
                gridx++;
            }
        }
    }
    if (node->numtaps == 0) {
        // Node wasn't cleanly within tap geometry when
        // centered on a grid point.  But if the via can be
        // offset and is cleanly within the tap geometry, then
        // allow it.

        int tapx = 0, tapy = 0, tapl = 0;
        int mask = 0;
        lefu_t dist = 0;

        // Initialize mindist to a large value.
        lefu_t mindist = pitchX(numLayers() - 1) + pitchY(numLayers() - 1);
        int dir = 0;        // Indicates no solution found.

        for (dbDseg *ds = g->taps[i]; ds; ds = ds->next) {
            lefu_t deltax = getViaWidth(ds->layer, ds->layer, DIR_VERT)/2;
            lefu_t deltay = getViaWidth(ds->layer, ds->layer, DIR_HORIZ)/2;

            int gridx = (ds->x1 - xLower()) / pitchX(ds->layer) - 1;
            for (;;) {
                lefu_t dx = (gridx * pitchX(ds->layer)) + xLower();
                if (dx > ds->x2 || gridx >= numChannelsX(ds->layer))
                    break;

                if (((dx - ds->x1) >= -deltax) && ((ds->x2 - dx) >= -deltax)) {
                    int gridy = (ds->y1 - yLower()) / pitchY(ds->layer) - 1;

                    for (;;) {
                        lefu_t dy = (gridy * pitchY(ds->layer)) + yLower();
                        if (dy > ds->y2 || gridy >= numChannelsY(ds->layer))
                            break;

                        // Check that the grid position is
                        // inside the tap rectangle.  However,
                        // if the point above the grid is
                        // blocked, then a via cannot be
                        // placed here, so skip it.

                        if (((ds->layer == (int)numLayers() - 1) ||
                                !(obsVal(gridx, gridy, ds->layer + 1)
                                & NO_NET)) &&
                                ((dy - ds->y1) >= -deltay) &&
                                ((ds->y2 - dy) >= -deltay)) {

                            // Grid point is inside tap
                            // geometry.  Since it did not
                            // pass the simple insideness test
                            // previously, it can be assumed
                            // that one of the edges is closer
                            // to the grid point than 1/2 via
                            // width.  Find that edge and use
                            // it to determine the offset.

                            // Check right edge.
                            if ((ds->x2 - dx) < deltax) {
                                dist = deltax - ds->x2 + dx;
                                // Confirm other edges.
                                if ((dx - dist - deltax >= ds->x1) &&
                                        (dy - deltay >= ds->y1) &&
                                        (dy + deltay <= ds->y2)) {
                                    if (dist < abs(mindist)) {
                                        mindist = dist;
                                        mask = STUBROUTE;
                                        dir = NI_STUB_EW;
                                        tapx = gridx;
                                        tapy = gridy;
                                        tapl = ds->layer;
                                    }
                                }
                            }

                            // Check left edge.
                            if ((dx - ds->x1) < deltax) {
                                dist = deltax - dx + ds->x1;
                                // Confirm other edges.
                                if ((dx + dist + deltax <= ds->x2) &&
                                        (dy - deltay >= ds->y1) &&
                                        (dy + deltay <= ds->y2)) {
                                    if (dist < abs(mindist)) {
                                        mindist = -dist;
                                        mask = STUBROUTE;
                                        dir = NI_STUB_EW;
                                        tapx = gridx;
                                        tapy = gridy;
                                        tapl = ds->layer;
                                    }
                                }
                            }

                            // Check top edge.
                            if ((ds->y2 - dy) < deltay) {
                                dist = deltay - ds->y2 + dy;
                                // Confirm other edges.
                                if ((dx - deltax >= ds->x1) &&
                                        (dx + deltax <= ds->x2) &&
                                        (dy - dist - deltay >=
                                        ds->y1)) {
                                    if (dist < abs(mindist)) {
                                        mindist = -dist;
                                        mask = STUBROUTE;
                                        dir = NI_STUB_NS;
                                        tapx = gridx;
                                        tapy = gridy;
                                        tapl = ds->layer;
                                    }
                                }
                            }

                            // Check bottom edge.
                            if ((dy - ds->y1) < deltay) {
                                dist = deltay - dy + ds->y1;
                                // Confirm other edges.
                                if ((dx - deltax >= ds->x1) &&
                                        (dx + deltax <= ds->x2) &&
                                        (dy + dist + deltay <=
                                        ds->y2)) {
                                    if (dist < abs(mindist)) {
                                        mindist = dist;
                                        mask = STUBROUTE;
                                        dir = NI_STUB_NS;
                                        tapx = gridx;
                                        tapy = gridy;
                                        tapl = ds->layer;
                                    }
                                }
                            }
                        }
                        gridy++;
                    }
                }
                gridx++;
            }
        }

        // Was a solution found?
        if (mask != 0) {
            // Grid position is clear for placing a via.

            db->emitErrMesg(
                "Tap position (%d, %d) appears to be"
                " technically routable with an offset, so"
                " it is being forced routable.\n",
                tapx, tapy);

            mrGridCell c;
            initGridCell(c, tapx, tapy, tapl);
            setObsVal(c, ((obsVal(c) & BLOCKED_MASK) | mask | node->netnum));
            setNodeLoc(c, node);
            setNodeSav(c, node);
            setStubVal(c, dist);
            setFlagsVal(c, (flagsVal(c) | dir));
            node->numtaps++;
        }
    }
    if (node->numtaps == 0) {
        db->emitErrMesg(
            "Router will not be able to completely route this net.\n");
    }
}


// count_pinlayers()
//
// Check which layers have non-NULL Nodeinfo.nodeloc,
// Nodeinfo.nodesav, and Nodeinfo.stub entries.  Then set pinLayers
// and free all the unused layers.  This saves a lot of memory,
// especially when the number of routing layers becomes large.
//
void
cMRouter::count_pinlayers()
{
    setPinLayers(0);
    for (u_int l = 0; l < numLayers(); l++) {
        u_int sz = numChannelsX(l) * numChannelsY(l);
        for (u_int j = 0; j < sz; j++) {
            // any NodeInfo element nonzero
            mrGridCell c;
            c.layer = l;
            c.index = j;
            if (nodeSav(c) || nodeLoc(c) || stubVal(c) || offsetVal(c) ||
                    flagsVal(c)) {
                setPinLayers(l + 1);
                break;
            }
        }
    }

    for (u_int l = pinLayers(); l < numLayers(); l++) {
        delete [] nodeInfoAry(l);
        setNodeInfoAry(l, 0);
    }
}


// make_routable
//
// In the case that a node can't be routed because it has no available
// tap points, but there is tap geometry recorded for the node, then
// take the first available grid location near the tap.  This, of
// course, bypasses all of qrouter's DRC checks.  But it is only meant
// to be a stop-gap measure to get qrouter to complete all routes, and
// may work in cases where, say, the tap passes euclidean rules but
// not manhattan rules.
//
void
cMRouter::make_routable(dbNode *node)
{
    // The database is not organized to find tap points from nodes, so
    // we have to search for the node.  Fortunately this function isn't
    // normally called.

    for (u_int k = 0; k < numPins(); k++) {
        dbGate *g = nlPin(k);
        for (int i = 0; i < g->nodes; i++) {
            if (g->noderec[i] == node)
                make_routable(g, i);
        }
    }
    for (u_int k = 0; k < numGates(); k++) {
        dbGate *g = nlGate(k);
        for (int i = 0; i < g->nodes; i++) {
            if (g->noderec[i] == node)
                make_routable(g, i);
        }
    }
}


void
cMRouter::make_routable(dbGate *g, int i)
{
    dbNode *node = g->noderec[i];
    for (dbDseg *ds = g->taps[i]; ds; ds = ds->next) {
        int gridx = (ds->x1 - xLower()) / pitchX(ds->layer) - 1;
        for (;;) {
            lefu_t dx = (gridx * pitchX(ds->layer)) + xLower();
            if (dx > ds->x2 || gridx >= numChannelsX(ds->layer))
                break;
            else if (dx >= ds->x1 && gridx >= 0) {
                int gridy = (ds->y1 - yLower()) / pitchY(ds->layer) - 1;
                for (;;) {
                    lefu_t dy = (gridy * pitchY(ds->layer)) + yLower();
                    if (dy > ds->y2 || gridy >= numChannelsY(ds->layer))
                        break;

                    // Area inside defined pin geometry.

                    if (dy > ds->y1 && gridy >= 0) {
                        mrGridCell c;
                        initGridCell(c, gridx, gridy, ds->layer);
                        int orignet = obsVal(c);

                        if (orignet & NO_NET) {
                            setObsVal(c, g->netnum[i]);
                            setNodeLoc(c, node);
                            setNodeSav(c, node);
                            return;
                        }
                    }
                    gridy++;
                }
            }
            gridx++;
        }
    }
}


// block_route
//
// Mark a specific length along the route tracks as unroutable by
// finding the grid point in the direction indicated, and setting the
// appropriate block bit in the Obs[] array for that position.  The
// original grid point is marked as unroutable in the opposite
// direction, for symmetry.
//
void
cMRouter::block_route(int x, int y, int lay, u_char dir)
{
    int bx = x;
    int by = y;
    int bl = lay;

    switch (dir) {
    case NORTH:
        if (y == numChannelsY(lay) - 1)
            return;
        by = y + 1;
        break;
    case SOUTH:
        if (y == 0)
            return;
        by = y - 1;
        break;
    case EAST:
        if (x == numChannelsX(lay) - 1)
            return;
        bx = x + 1;
        break;
    case WEST:
        if (x == 0)
            return;
        bx = x - 1;
        break;
    case UP:
        if (lay == (int)numLayers() - 1)
            return;
        bl = lay + 1;
        break;
    case DOWN:
        if (lay == 0)
            return;
        bl = lay - 1;
        break;
    }
   
    mrGridCell c, cb;
    initGridCell(c, x, y, lay);
    initGridCell(cb, bx, by, bl);

    if (obsVal(cb) & NO_NET)
        return;

    switch (dir) {
    case NORTH:
        setObsVal(cb, (obsVal(cb) | BLOCKED_S));
        setObsVal(c, (obsVal(c) | BLOCKED_N));
        break;
    case SOUTH:
        setObsVal(cb, (obsVal(cb) | BLOCKED_N));
        setObsVal(c, (obsVal(c) | BLOCKED_S));
        break;
    case EAST:
        setObsVal(cb, (obsVal(cb) | BLOCKED_W));
        setObsVal(c, (obsVal(c) | BLOCKED_E));
        break;
    case WEST:
        setObsVal(cb, (obsVal(cb) | BLOCKED_E));
        setObsVal(c, (obsVal(c) | BLOCKED_W));
        break;
    case UP:
        setObsVal(cb, (obsVal(cb) | BLOCKED_D));
        setObsVal(c, (obsVal(c) | BLOCKED_U));
        break;
    case DOWN:
        setObsVal(cb, (obsVal(cb) | BLOCKED_U));
        setObsVal(c, (obsVal(c) | BLOCKED_D));
        break;
    }
}


// find_bounding_box
//
// Measure and record the bounding box of a net.  This is preparatory
// to generating a mask for the net.  Find the bounding box of each
// node, and record that information, at the same time computing the
// whole net's bounding box as the area bounding all of the nodes. 
// Determine if the bounding box is more horizontal or vertical, and
// specify a direction for the net's trunk line.  Initialize the trunk
// line as the midpoint between all of the nodes, extending the width
// (or height) of the bounding box.  Initialize the node branch
// position as the line extending from the middle of the node's
// bounding box to the trunk line.  These positions (trunk and
// branches) will be sorted and readjusted by "create_nodeorder()".
//
void
cMRouter::find_bounding_box(dbNet *net)
{
    if (net->numnodes == 2) {

        dbNode *n1 = net->netnodes;
        dbNode *n2 = net->netnodes->next;

        // Simple 2-pass---pick up first tap on n1, find closest tap
        // on n2, then find closest tap on n1.

        dbDpoint *d1tap = (n1->taps == 0) ? n1->extend : n1->taps;
        if (!d1tap)
            return;
        dbDpoint *d2tap = (n2->taps == 0) ? n2->extend : n2->taps;
        if (!d2tap)
            return;
        int dx = d2tap->gridx - d1tap->gridx;
        int dy = d2tap->gridy - d1tap->gridy;
        int mindist = dx * dx + dy * dy;
        dbDpoint *mintap = d2tap;
        for (d2tap = d2tap->next; d2tap; d2tap = d2tap->next) {
            dx = d2tap->gridx - d1tap->gridx;
            dy = d2tap->gridy - d1tap->gridy;
            int dist = dx * dx + dy * dy;
            if (dist < mindist) {
                mindist = dist;
                mintap = d2tap;
            }
        }
        d2tap = mintap;
        d1tap = (n1->taps == 0) ? n1->extend : n1->taps;
        dx = d2tap->gridx - d1tap->gridx;
        dy = d2tap->gridy - d1tap->gridy;
        mindist = dx * dx + dy * dy;
        mintap = d1tap;
        for (d1tap = d1tap->next; d1tap; d1tap = d1tap->next) {
            dx = d2tap->gridx - d1tap->gridx;
            dy = d2tap->gridy - d1tap->gridy;
            int dist = dx * dx + dy * dy;
            if (dist < mindist) {
                mindist = dist;
                mintap = d1tap;
            }
        }
        d1tap = mintap;

        net->xmin = (d1tap->gridx < d2tap->gridx) ? d1tap->gridx : d2tap->gridx;
        net->xmax = (d1tap->gridx < d2tap->gridx) ? d2tap->gridx : d1tap->gridx;
        net->ymin = (d1tap->gridy < d2tap->gridy) ? d1tap->gridy : d2tap->gridy;
        net->ymax = (d1tap->gridy < d2tap->gridy) ? d2tap->gridy : d1tap->gridy;
    }
    else {          // Net with more than 2 nodes.

        // Use the first tap point for each node to get a rough bounding
        // box and centroid of all taps.

#ifdef LD_SIGNED_GRID
        net->xmax = net->ymax = -(MAXRT);
        net->xmin = net->ymin = MAXRT;
#else
        net->xmax = net->ymax = 0;
        net->xmin = net->ymin = (LD_MAX_CHANNELS - 1);
#endif
        for (dbNode *n1 = net->netnodes; n1; n1 = n1->next) {
            dbDpoint *dtap = (n1->taps == 0) ? n1->extend : n1->taps;
            if (dtap) {
                if (dtap->gridx > net->xmax)
                    net->xmax = dtap->gridx;
                if (dtap->gridx < net->xmin)
                    net->xmin = dtap->gridx;
                if (dtap->gridy > net->ymax)
                    net->ymax = dtap->gridy;
                if (dtap->gridy < net->ymin)
                    net->ymin = dtap->gridy;
            }
        }
    }
}


// define_route_tree
//
// Define a trunk-and-branches potential best route for a net.
//
// The net is analyzed for aspect ratio, and is determined if it will
// have a horizontal or vertical trunk.  Then, each node will define a
// branch line extending from the node position to the trunk.  Trunk
// position is recorded in the net record, and branch positions are
// recorded in the node records.
//
// To do:
// Trunk and branch lines will be analyzed for immediate collisions
// and sorted to help ensure a free track exists for each net's trunk
// line.
//
void
cMRouter::define_route_tree(dbNet *net)
{
    // This is called after create_bounding_box(), so bounds have been
    // calculated.

    int xmin = net->xmin;
    int xmax = net->xmax;
    int ymin = net->ymin;
    int ymax = net->ymax;

    if (net->numnodes == 2) {

        // For 2-node nets, record the initial position as one
        // horizontal trunk + one branch for one "L" of the bounding
        // box, and one vertical trunk + one branch for the other "L"
        // of the bounding box.

        net->trunkx = xmin;
        net->trunky = ymin;
    }
    else if (net->numnodes > 0) {

        // Use the first tap point for each node to get a rough
        // centroid of all taps

        int xcent = 0;
        int ycent = 0;
        for (dbNode *n1 = net->netnodes; n1; n1 = n1->next) {
            dbDpoint *dtap = (n1->taps == 0) ? n1->extend : n1->taps;
            if (!dtap)
                continue;
            xcent += dtap->gridx;
            ycent += dtap->gridy;
        }
        xcent /= net->numnodes;
        ycent /= net->numnodes;

        // Record the trunk line in the net record.

        net->trunkx = xcent;
        net->trunky = ycent;
    }

    if (xmax - xmin > ymax - ymin) {
        // Horizontal trunk preferred.
        net->flags &= ~NET_VERTICAL_TRUNK;
    }
    else {
        // Vertical trunk preferred.
        net->flags |= NET_VERTICAL_TRUNK;
    }

    // Set the branch line positions to the node tap points.

    for (dbNode *n1 = net->netnodes; n1; n1 = n1->next) {
        dbDpoint *dtap = (n1->taps == 0) ? n1->extend : n1->taps;
        if (!dtap)
            continue;
        n1->branchx = dtap->gridx;
        n1->branchy = dtap->gridy;
    }
}


// disable_gridpos()
//
// Render the position at (x, y, lay) unroutable by setting its Obs[]
// entry to NO_NET and removing it from the nodeloc and nodesav
// records.
//
void
cMRouter::disable_gridpos(int x, int y, int lay)
{
    mrGridCell c;
    initGridCell(c, x, y, lay);

    setObsVal(c, (NO_NET | OBSTRUCT_MASK));
    setNodeSav(c, 0);
    setNodeLoc(c, 0);
    setStubVal(c, 0);
    setOffsetVal(c, 0);
    setFlagsVal(c, 0);
}


// The logic changes here between qrouter 1.3.57 and 1.3.62.  This seems
// to have a strong effect on the routes chosen.  Define this to use
// qrouter-1.3.57 logic.
//#define QR1357

// check_obstruct()
//
// Called from create_obstructions_from_gates(), this function takes a
// grid point at (gridx, gridy) (physical position (dx, dy)) and an
// obstruction defined by the rectangle "ds", and sets flags and fills
// the Obsinfo array to reflect how the obstruction affects routing to
// the grid position.
//
void
cMRouter::check_obstruct(int gridx, int gridy, dbDseg *ds,
    lefu_t dx, lefu_t dy)
{
    mrGridCell c;
    initGridCell(c, gridx, gridy, ds->layer);
    u_int obsval = obsVal(c);
    lefu_t dist = obsInfoVal(c);

    // Grid point is inside obstruction + halo.
    obsval |= NO_NET;

    // Completely inside obstruction?
#ifdef QR1357
    if (dy >= ds->y1 && dy <= ds->y2 && dx >= ds->x1 && dx <= ds->x2)
#else
    if (dy > ds->y1 && dy < ds->y2 && dx > ds->x1 && dx < ds->x2)
#endif
        obsval |= OBSTRUCT_MASK;

    else {

        // Make more detailed checks in each direction.

#ifdef QR1357
        if (dy < ds->y1) {
#else
        if (dy <= ds->y1) {
#endif
            if ((obsval & (OBSTRUCT_MASK & ~OBSTRUCT_N)) == 0) {
                if ((dist == 0) || ((ds->y1 - dy) < dist))
                    setObsInfoVal(c, ds->y1 - dy);
                obsval |= OBSTRUCT_N;
            }
            else
                obsval |= OBSTRUCT_MASK;
        }
#ifdef QR1357
        else if (dy > ds->y2) {
#else
        else if (dy >= ds->y2) {
#endif
            if ((obsval & (OBSTRUCT_MASK & ~OBSTRUCT_S)) == 0) {
                if ((dist == 0) || ((dy - ds->y2) < dist))
                    setObsInfoVal(c, dy - ds->y2);
                obsval |= OBSTRUCT_S;
            }
            else
                obsval |= OBSTRUCT_MASK;
        }
#ifdef QR1357
        if (dx < ds->x1) {
#else
        if (dx <= ds->x1) {
#endif
            if ((obsval & (OBSTRUCT_MASK & ~OBSTRUCT_E)) == 0) {
                if ((dist == 0) || ((ds->x1 - dx) < dist))
                    setObsInfoVal(c, ds->x1 - dx);
                obsval |= OBSTRUCT_E;
            }
            else
                obsval |= OBSTRUCT_MASK;
        }
#ifdef QR1357
        else if (dx > ds->x2) {
#else
        else if (dx >= ds->x2) {
#endif
            if ((obsval & (OBSTRUCT_MASK & ~OBSTRUCT_W)) == 0) {
                if ((dist == 0) || ((dx - ds->x2) < dist))
                    setObsInfoVal(c, dx - ds->x2);
                obsval |= OBSTRUCT_W;
            }
            else
                obsval |= OBSTRUCT_MASK;
        }
    }
    setObsVal(c, obsval);
}


// get_via_clear
//
// Find the amount of clearance needed between an obstruction and a
// route track position.  This takes into consideration whether the
// obstruction is wide or narrow metal, if the spacing rules are
// graded according to metal width, and if a via placed at the
// position is or is not symmetric in X and Y.
//
lefu_t
cMRouter::get_via_clear(int lay, ROUTE_DIR dir, dbDseg *rect)
{
    ROUTE_DIR ndir = (dir == DIR_VERT) ? DIR_HORIZ : DIR_VERT;
    lefu_t vdelta = getViaWidth(lay, lay, ndir);
    if (lay > 0) {
        lefu_t v2delta = getViaWidth(lay - 1, lay, ndir);
        if (v2delta > vdelta)
            vdelta = v2delta;
   }
   vdelta /= 2;

   // Spacing rule is determined by the minimum metal width, either in
   // X or Y, regardless of the position of the metal being checked.

   lefu_t mwidth = LD_MIN(rect->x2 - rect->x1, rect->y2 - rect->y1);
   lefu_t mdelta = getRouteWideSpacing(lay, mwidth);

   return (vdelta + mdelta);
}


// get_route_clear
//
// Find the distance from an obstruction to a grid point, considering
// only routes which are placed at the position, not vias.
//
lefu_t
cMRouter::get_route_clear(int lay, dbDseg *rect)
{
   lefu_t rdelta = getRouteWidth(lay)/2;

   // Spacing rule is determined by the minimum metal width, either in
   // X or Y, regardless of the position of the metal being checked.

   lefu_t mwidth = LD_MIN(rect->x2 - rect->x1, rect->y2 - rect->y1);
   lefu_t mdelta = getRouteWideSpacing(lay, mwidth);

   return (rdelta + mdelta);
}

