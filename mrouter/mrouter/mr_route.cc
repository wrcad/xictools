
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
 $Id:$
 *========================================================================*/

/*--------------------------------------------------------------*/
/*  qrouter.c -- general purpose autorouter                     */
/*  Reads LEF libraries and DEF netlists, and generates an      */
/*  annotated DEF netlist as output.                            */
/*--------------------------------------------------------------*/
/* Written by Tim Edwards, June 2011, based on code by Steve    */
/* Beccue, 2003                                                 */
/*--------------------------------------------------------------*/

#include "mrouter_prv.h"


//
// Maze Router.
//
// Misc. routing functions.
//

// Below, getRoutedNet was originally a single very long and complex
// function.  Since I need to use and understand this logic to get the
// physical structure of the routes, I'm going to break it up into
// smaller, more digestible pieces.  Below is a locally-used structure
// to hold state variables, so we don't have to pass a lot of
// arguments.

enum PathMode { PathNONE = -1, PathDONE = 0, PathSTART = 1 };

struct sPhysRouteGenCx
{
    sPhysRouteGenCx(u_int nl)
        {
            lastx       = -1;
            lasty       = -1;
            lastlay     = -1;
            pathOn      = PathNONE;
            direction   = DIR_VERT;
            seg         = 0;;
            lastseg     = 0;;
            prevseg     = 0;;
            net         = 0;;
            rt          = 0;;
            endp        = 0;
            endsp       = 0;

            viaCheckX   = new u_char[nl];
            viaCheckY   = new u_char[nl];
            viaOffsetX  = new int[nl][3];
            viaOffsetY  = new int[nl][3];
        }

    ~sPhysRouteGenCx()
        {
            delete [] viaCheckX;
            delete [] viaCheckY;
            delete [] viaOffsetX;
            delete [] viaOffsetY;
        }

    lefu_t  lastx;
    lefu_t  lasty;
    int     lastlay;
    PathMode  pathOn;
    ROUTE_DIR direction;
    dbSeg   *seg;
    dbSeg   *lastseg;
    dbSeg   *prevseg;
    dbNet   *net;
    dbRoute *rt;
    dbPath  *endp;
    dbPath  *endsp;

    u_char  *viaCheckX;
    u_char  *viaCheckY;
    int (*viaOffsetX)[3];
    int (*viaOffsetY)[3];
};


//#define DEBUG_RT

// Create a context struct for generating physical routes.  This is
// not seen by the caller, but needs to be created before routes can
// be generated.
//
bool
cMRouter::initPhysRouteGen()
{
    clearPhysRouteGen();
    if (!numLayers())
        return (LD_BAD);
    sPhysRouteGenCx *cx = new sPhysRouteGenCx(numLayers());
    mr_route_gen = cx;

    // Compute via offsets, if needed for adjacent vias.

    // A well-designed standard cell set should not have DRC errors
    // between vias spaced on adjacent tracks.  But not every standard
    // cell set is well-designed. . .

    // Example of offset measurements:
    // viaOffsetX[layer][n]:  layer is the base layer of the via, n is
    // 0 for the via one layer below, 1 for the same via, and 2 for the
    // via one layer above.  Note that the n = 1 has interactions on two
    // different metal layers.  The maximum distance is used.

    // viaCheckX[1] is 0 if all of viaOffsetX[1][0-2] is zero.  This
    //    allows a quick determination if a check for neighboring vias
    //    is required.
    // viaOffsetX[1][0] is the additional spacing above the grid width
    //    for via2-to-via1 (on metal2 only).
    // viaOffsetX[1][1] is the additional spacing above the grid width
    //    for via2-to-via2 (maximum for metal2 and metal3)
    // viaOffsetX[1][2] is the additional spacing above the grid width
    //    for via2-to-via3 (on metal3 only).

    cx->viaOffsetX[0][0] = 0;                // nothing below the 1st via
    cx->viaOffsetY[0][0] = 0;
    cx->viaOffsetX[numLayers() - 1][2] = 0;  // nothing above the last via
    cx->viaOffsetY[numLayers() - 1][2] = 0;

    for (u_int layer = 0; layer < numLayers() - 1; layer++) {
        lefu_t s1  = getRouteSpacing(layer);
        lefu_t s2  = getRouteSpacing(layer + 1);
        lefu_t p1x = pitchX(layer);
        lefu_t p2x = pitchX(layer + 1);
        lefu_t p1y = pitchY(layer);
        lefu_t p2y = pitchY(layer + 1);
        lefu_t w1x = getViaWidth(layer, layer, DIR_VERT);
        lefu_t w1y = getViaWidth(layer, layer, DIR_HORIZ);
        lefu_t w2x = getViaWidth(layer, layer + 1, DIR_VERT);
        lefu_t w2y = getViaWidth(layer, layer + 1, DIR_HORIZ);

        cx->viaCheckX[layer] = 0;
        cx->viaCheckY[layer] = 0;

        if (layer > 0) {

            // Space from via to (via - 1).

            lefu_t w0x = getViaWidth(layer - 1, layer, DIR_VERT);
            lefu_t w0y = getViaWidth(layer - 1, layer, DIR_HORIZ);

            lefu_t dc = s1 + (w1x + w0x) / 2 - p1x;
            cx->viaOffsetX[layer][0] = (dc > 0) ? dc : 0;

            dc = s1 + (w1y + w0y) / 2 - p1y;
            cx->viaOffsetY[layer][0] = (dc > 0) ? dc : 0;
        }

        // Space from via to via (check both lower and upper metal layers).

        lefu_t dc = s1 + w1x - p1x;
        cx->viaOffsetX[layer][1] = (dc > 0) ? dc : 0;

        dc = s2 + w2x - p2x;
        if (dc < 0)
            dc = 0;
        if (dc > cx->viaOffsetX[layer][1])
            cx->viaOffsetX[layer][1] = dc;

        dc = s1 + w1y - p1y;
        cx->viaOffsetY[layer][1] = (dc > 0) ? dc : 0;

        dc = s2 + w2y - p2y;
        if (dc < 0)
            dc = 0;
        if (dc > cx->viaOffsetY[layer][1])
            cx->viaOffsetY[layer][1] = dc;

        if (layer < numLayers() - 1) {

            // Space from via to (via + 1).

            lefu_t w3x = getViaWidth(layer + 1, layer, DIR_VERT);
            lefu_t w3y = getViaWidth(layer + 1, layer, DIR_HORIZ);

            dc = s2 + (w2x + w3x) / 2 - p2x;
            cx->viaOffsetX[layer][2] = (dc > 0) ? dc : 0;

            dc = s2 + (w2y + w3y) / 2 - p2y;
            cx->viaOffsetY[layer][2] = (dc > 0) ? dc : 0;
        }

        if (cx->viaOffsetX[layer][0] > 0 || cx->viaOffsetX[layer][1] > 0 ||
                cx->viaOffsetX[layer][2] > 0)
            cx->viaCheckX[layer] = 1;
        if (cx->viaOffsetY[layer][0] > 0 || cx->viaOffsetY[layer][1] > 0 ||
                cx->viaOffsetY[layer][2] > 0)
            cx->viaCheckY[layer] = 1;
    }
    return (LD_OK);
}


// Set up the physical route paths for all routes in the design.
//
void
cMRouter::setupRoutePaths()
{
    // This can be called before mr_nets is created, in which case
    // there is no routing info so we're done.
    if (!mr_nets)
        return;

    if (mr_route_gen) {
        for (u_int i = 0; i < numNets(); i++) {
            setupRoutePath(nlNet(i), false);
            setupRoutePath(nlNet(i), true);
        }
    }
    else {
        initPhysRouteGen();
        for (u_int i = 0; i < numNets(); i++) {
            setupRoutePath(nlNet(i), false);
            setupRoutePath(nlNet(i), true);
        }
        clearPhysRouteGen();
    }
}


// Use the physical net context struct to generate a linked list
// representing the physical implementation of a routed net.
//
bool
cMRouter::setupRoutePath(dbNet *net, bool special)
{
    sPhysRouteGenCx *cx = mr_route_gen;
    if (!cx || !net)
        return (LD_BAD);

    // Set up the output flags.  If a flag is set, that route will
    // not be returned.

    // If the path pointer is non-zero, just return.  Caller must
    // clear this to compute a new path.

    if (special) {
        if (net->spath)
            return (LD_OK);

        for (dbRoute *rt = net->routes; rt; rt = rt->next) {
            // Do stubs only.
            if (rt->flags & RT_STUB)
                rt->flags &= ~RT_OUTPUT;
            else
                rt->flags |= RT_OUTPUT;
        }
    }
    else {
        if (net->path)
            return (LD_OK);

        for (dbRoute *rt = net->routes; rt; rt = rt->next) {
            // Skip stubs.
            if (rt->flags & RT_STUB)
                rt->flags |= RT_OUTPUT;
            else
                rt->flags &= ~RT_OUTPUT;
        }

        // Quick check to see if cleanup_net can be avoided.
        if (!(net->flags & NET_CLEANUP)) {
            net->flags |= NET_CLEANUP;
            bool need_cleanup = false;
            for (u_int i = 0; i < numLayers(); i++) {
                if (needBlock(i) & (VIABLOCKX | VIABLOCKY)) {
                    need_cleanup = true;
                    break;
                }
            }
            if (need_cleanup)
                cleanup_net(net);
        }
    }

    cx->pathOn = PathNONE;

#ifdef DEBUG_RT
    db->emitMesg("Net: %s num %d nodes %d\n", net->netname, net->netnum,
        net->numnodes);
#endif

    for (dbRoute *rt = net->routes; rt; rt = rt->next) {
        if (!rt->segments || (rt->flags & RT_OUTPUT))
            continue;

#ifdef DEBUG_RT
        db->emitMesg(" New route\n");
#endif
        cx->net = net;
        cx->rt = rt;

        // The router is not necessarily initialized, as we can be
        // called from lddb when writing DEF.  This doesn't really
        // matter since things like obsVal will benignly return 0.
        // We can skip the endpoint check entirely in this case.

        check_first_offset(special);

        // Loop thru the segments and process.
        check_offset_terminals(special);

        // For stub routes, reset the path between terminals,
        // since the stubs are not connected.
        if (special && cx->pathOn != PathNONE)
            cx->pathOn = PathDONE;

        // Check last position for terminal offsets.
        check_last_offset(special);

        if (cx->pathOn != PathNONE)
            cx->pathOn = PathDONE;

    }
    return (0);
}


// Free the physical net context struct, call when done generating
// physical routes.
//
void
cMRouter::clearPhysRouteGen()
{
    delete mr_route_gen;
    mr_route_gen = 0;
}


// It is rare but possible to have a stub route off of an endpoint
// via, so check this case, and use the layer type of the via top if
// needed.
//
void
cMRouter::check_first_offset(bool special)
{
    sPhysRouteGenCx *cx = mr_route_gen;
    if (!cx)
        return;
    dbSeg *seg = cx->rt->segments;
    if (!cx || !seg)
        return;

    int layer = seg->layer;
    if ((seg->segtype & ST_VIA) && seg->next &&
            (seg->next->layer <= seg->layer))
        layer++;

    if (!(obsVal(seg->x1, seg->y1, layer) & STUBROUTE))
        return;

    lefu_t stub = stubVal(seg->x1, seg->y1, layer);
    if (!special && (verbose() > 2)) {
        db->emitMesg("Stub route distance %g to terminal at %d %d (%d)\n",
            db->lefToMic(stub), seg->x1, seg->y1, layer);
    }

    lefu_t dc = xLower() + seg->x1 * pitchX(layer);
    lefu_t x = dc;
    if (flagsVal(seg->x1, seg->y1, layer) & NI_STUB_EW)
        dc += stub;
    lefu_t x2 = dc;
    dc = yLower() + seg->y1 * pitchY(layer);
    lefu_t y = dc;
    if (flagsVal(seg->x1, seg->y1, layer) & NI_STUB_NS)
        dc += stub;
    lefu_t y2 = dc;

    bool cancel = false;
    if (flagsVal(seg->x1, seg->y1, layer) & NI_STUB_EW) {
        cx->direction = DIR_HORIZ;

        // If the gridpoint ahead of the stub has a route on the same
        // net, and the stub is long enough to come within a DRC
        // spacing distance of the other route, then lengthen it to
        // close up the distance and resolve the error.  (NOTE:  This
        // unnecessarily stretches routes to cover taps that have not
        // been routed to.  At least on the test standard cell set,
        // these rules remove a handful of DRC errors and don't create
        // any new ones.  If necessary, a flag can be added to
        // distinguish routes from taps.

        if ((x < x2) && (seg->x1 < (numChannelsX(layer) - 1))) {
            u_int tdir = obsVal(seg->x1 + 1, seg->y1, layer);
            if ((tdir & ROUTED_NET_MASK) ==
                    (u_int)(cx->net->netnum | ROUTED_NET)) {
                if (stub + getRouteKeepout(layer) >= pitchX(layer)) {
                    dc = xLower() + (seg->x1 + 1) * pitchX(layer);
                    x2 = dc;
                }
            }
        }
        else if ((x > x2) && (seg->x1 > 0)) {
            u_int tdir = obsVal(seg->x1 - 1, seg->y1, layer);
            if ((tdir & ROUTED_NET_MASK) ==
                    (u_int)(cx->net->netnum | ROUTED_NET)) {
                if (-stub + getRouteKeepout(layer) >= pitchX(layer)) {
                    dc = xLower() + (seg->x1 - 1) * pitchX(layer);
                    x2 = dc;
                }
            }
        }

        dc = getRouteWidth(layer)/2;
        if (!special) {
            // Regular nets include 1/2 route width at the ends, so
            // subtract from the stub terminus.

            if (x < x2) {
                x2 -= dc;
                if (x >= x2)
                    cancel = true;
            }
            else {
                x2 += dc;
                if (x <= x2)
                    cancel = true;
            }
        }
        else {
            // Special nets don't include 1/2 route width at the ends,
            // so add to the route at the grid.

            if (x < x2)
                x -= dc;
            else
                x += dc;

            // Routes that extend for more than one track without a
            // bend do not need a wide stub,

            if (seg->x1 != seg->x2)
                cancel = true;
        }
    }
    else {
        cx->direction = DIR_VERT;

        // If the gridpoint ahead of the stub has a route on the same
        // net, and the stub is long enough to come within a DRC
        // spacing distance of the other route, then lengthen it to
        // close up the distance and resolve the error.

        if ((y < y2) && (seg->y1 < (numChannelsY(layer) - 1))) {
            u_int tdir = obsVal(seg->x1, seg->y1 + 1, layer);
            if ((tdir & ROUTED_NET_MASK) ==
                    (u_int)(cx->net->netnum | ROUTED_NET)) {
                if (stub + getRouteKeepout(layer) >= pitchY(layer)) {
                    dc = yLower() + (seg->y1 + 1) * pitchY(layer);
                    y2 = dc;
                }
            }
        }
        else if ((y > y2) && (seg->y1 > 0)) {
            u_int tdir = obsVal(seg->x1, seg->y1 - 1, layer);
            if ((tdir & ROUTED_NET_MASK) ==
                    (u_int)(cx->net->netnum | ROUTED_NET)) {
                if (-stub + getRouteKeepout(layer) >= pitchY(layer)) {
                    dc = yLower() + (seg->y1 - 1) * pitchY(layer);
                    y2 = dc;
                }
            }
        }

        dc = getRouteWidth(layer)/2;
        if (!special) {
            // Regular nets include 1/2 route width at the ends, so
            // subtract from the stub terminus.

            if (y < y2) {
                y2 -= dc;
                if (y >= y2)
                    cancel = true;
            }
            else {
                y2 += dc;
                if (y <= y2)
                    cancel = true;
            }
        }
        else {
            // Special nets don't include 1/2 route width at the ends,
            // so add to the route at the grid.

            if (y < y2)
                y -= dc;
            else
                y += dc;

            // Routes that extend for more than one track without a
            // bend do not need a wide stub.

            if (seg->y1 != seg->y2)
                cancel = true;
        }
    }

    if (!cancel) {
        cx->net->flags |= NET_STUB;
        cx->rt->flags |= RT_STUB;
        if (special) {
            pathstub(layer, x2, y2, x, y, cx->direction);
        }
        else {
            pathstart(layer, x2, y2);
            pathto(x, y, cx->direction, x2, y2);
        }
    }
    cx->lastx = x;
    cx->lasty = y;
    cx->lastlay = layer;
}


// Loop thru the segments and process.
//
void
cMRouter::check_offset_terminals(bool special)
{
    sPhysRouteGenCx *cx = mr_route_gen;
    if (!cx || !cx->rt)
        return;
    dbSeg *lastseg = 0;
    cx->lastseg = 0;
    cx->prevseg = 0;
    for (dbSeg *seg = cx->rt->segments; seg; seg = seg->next) {
        cx->seg = seg;  // Save this!
        u_int layer = seg->layer;

#ifdef DEBUG_RT
        db->emitMesg("  seg %d %d %d %d   %d %d\n", seg->x1, seg->y1,
            seg->x2, seg->y2, seg->segtype, seg->layer);
#endif

        // Check for offset terminals at either point.
        lefu_t offset1 = 0;
        lefu_t offset2 = 0;
        u_int dir1 = 0;
        u_int dir2 = 0;
        u_int flags1 = 0;
        u_int flags2 = 0;
        mrGridCell c;

        if (seg->segtype & ST_OFFSET_START) {
            initGridCell(c, seg->x1, seg->y1, seg->layer);
            dir1 = obsVal(c) & OFFSET_TAP;
            if ((dir1 == 0) && lastseg) {
                initGridCell(c, lastseg->x2, lastseg->y2, lastseg->layer);
                dir1 = obsVal(c) & OFFSET_TAP;
                offset1 = offsetVal(c);
                flags1 = flagsVal(c);
            }
            else {
                offset1 = offsetVal(c);
                flags1 = flagsVal(c);
            }

            // Offset was calculated for vias; plain metal routes
            // typically will need less offset distance, so subtract
            // off the difference.

            if (!(seg->segtype & ST_VIA)) {
                if (offset1 < 0) {
                    offset1 += (getViaWidth(seg->layer, seg->layer,
                        cx->direction) - getRouteWidth(seg->layer))/2;
                    if (offset1 > 0)
                        offset1 = 0;
                }
                else if (offset1 > 0) {
                    offset1 -= (getViaWidth(seg->layer, seg->layer,
                        cx->direction) - getRouteWidth(seg->layer))/2;
                    if (offset1 < 0)
                        offset1 = 0;
                }
            }

            if (!special) {
                if ((seg->segtype & ST_VIA) && (verbose() > 2)) {
                    db->emitMesg(
                        "Offset terminal distance %g to grid at %d %d (%d)\n",
                        db->lefToMic(offset1), seg->x1, seg->y1, layer);
                }
            }
        }
        if (seg->segtype & ST_OFFSET_END) {
            initGridCell(c, seg->x2, seg->y2, seg->layer);
            dir2 = obsVal(c) & OFFSET_TAP;
            if ((dir2 == 0) && seg->next) {
                initGridCell(c, seg->next->x1, seg->next->y1, seg->next->layer);
                dir2 = obsVal(c) & OFFSET_TAP;
                offset2 = offsetVal(c);
                flags2 = flagsVal(c);
            }
            else {
                offset2 = offsetVal(c);
                flags2 = flagsVal(c);
            }

            // Offset was calculated for vias; plain metal routes
            // typically will need less offset distance, so subtract
            // off the difference.

            if (!(seg->segtype & ST_VIA)) {
                if (offset2 < 0) {
                    offset2 += (getViaWidth(seg->layer, seg->layer,
                        cx->direction) - getRouteWidth(seg->layer))/2;
                    if (offset2 > 0)
                        offset2 = 0;
                }
                else if (offset2 > 0) {
                    offset2 -= (getViaWidth(seg->layer, seg->layer,
                        cx->direction) - getRouteWidth(seg->layer))/2;
                    if (offset2 < 0)
                        offset2 = 0;
                }
            }

            if (!special) {
                if ((seg->segtype & ST_VIA)
                        && !(seg->segtype & ST_OFFSET_START)) {
                    if (verbose() > 2) {
                        db->emitMesg(
                        "Offset terminal distance %g to grid at %d %d (%d)\n",
                            db->lefToMic(offset2), seg->x2, seg->y2, layer);
                    }
                }
            }
        }

        // To do:  pick up route layer name from lefInfo.  At the
        // moment, technology names don't even match, and are
        // redundant between CIFLayer[] from the config file and
        // lefInfo.

        lefu_t dc = xLower() + seg->x1 * pitchX(layer);
        if ((dir1 & OFFSET_TAP) && (flags1 & NI_OFFSET_EW))
            dc += offset1;
        lefu_t x = dc;
        dc = yLower() + seg->y1 * pitchY(layer);
        if ((dir1 & OFFSET_TAP) && (flags1 & NI_OFFSET_NS))
            dc += offset1;
        lefu_t y = dc;
        dc = xLower() + seg->x2 * pitchX(layer);
        if ((dir2 & OFFSET_TAP) && (flags2 & NI_OFFSET_EW))
            dc += offset2;
        lefu_t x2 = dc;
        dc = yLower() + seg->y2 * pitchY(layer);
        if ((dir2 & OFFSET_TAP) && (flags2 & NI_OFFSET_NS))
            dc += offset2;
        lefu_t y2 = dc;

        if (seg->segtype & ST_WIRE) {

            // Normally layers change only at a via.  However, if a
            // via has been removed and replaced by a 1-track segment
            // to a neighboring via to avoid DRC errors (see
            // cleanup_net()), then a layer change may happen between
            // two ST_WIRE segments, and a new path should be started.

            if ((cx->pathOn != PathNONE) && (cx->lastlay != -1) &&
                    (cx->lastlay != seg->layer))
                cx->pathOn = PathDONE;

            if (cx->pathOn != PathSTART) { // 1st point of route seg
                if (x == x2)
                    cx->direction = DIR_VERT;
                else if (y == y2)
                    cx->direction = DIR_HORIZ;
                else if (verbose() > 3) {
                    // NOTE:  This is a development diagnostic.  The
                    // occasional non-Manhanhattan route is due to a
                    // tap offset and is corrected automatically by
                    // making an L-bend in the wire.

                    db->flushMesg();
                    db->emitErrMesg(
                        "Warning:  non-Manhattan wire in route"
                        " at (%d %d) to (%d %d)\n", x, y, x2, y2);
                }
                if (!special) {
                    pathstart(seg->layer, x, y);
                    cx->lastx = x;
                    cx->lasty = y;
                    cx->lastlay = seg->layer;
                }
            }
            cx->rt->flags |= RT_OUTPUT;
            if (cx->direction == DIR_HORIZ && x == x2)
                cx->direction = DIR_VERT;
            if (cx->direction == DIR_VERT && y == y2)
                cx->direction = DIR_HORIZ;
            if (!(x == x2) && !(y == y2))
                cx->direction = DIR_VERT;
            if (!special) {
                pathto(x2, y2, cx->direction, cx->lastx, cx->lasty);
                cx->lastx = x2;
                cx->lasty = y2;
            }

            // If a segment is 1 track long, there is a via on either
            // end, and the needblock flag is set for the layer, then
            // draw a stub route along the length of the track.

            if (cx->direction == DIR_HORIZ &&
                    needBlock(seg->layer) & VIABLOCKX) {
                if (LD_ABSDIFF(seg->x2, seg->x1) == 1) {
                    if ((lastseg && lastseg->segtype == ST_VIA) ||
                            (seg->next && seg->next->segtype == ST_VIA)) {
                        if (!special) {
                            cx->net->flags |= NET_STUB;
                            cx->rt->flags |= RT_STUB;
                        }
                        else {
                            if (cx->pathOn != PathNONE)
                                cx->pathOn = PathDONE;
                            pathstub(layer, x, y, x2, y2, cx->direction);
                            cx->lastlay = layer;
                        }
                    }
                }
            }
            else if (cx->direction == DIR_VERT &&
                    needBlock(seg->layer) & VIABLOCKY) {
                if (LD_ABSDIFF(seg->y2, seg->y1) == 1)  {
                    if ((lastseg && lastseg->segtype == ST_VIA) ||
                            (seg->next && seg->next->segtype == ST_VIA)) {
                        if (!special) {
                            cx->net->flags |= NET_STUB;
                            cx->rt->flags |= RT_STUB;
                        }
                        else {
                            if (cx->pathOn != PathNONE)
                                cx->pathOn = PathDONE;
                            pathstub(layer, x, y, x2, y2, cx->direction);
                            cx->lastlay = layer;
                        }
                    }
                }
            }
        }
        else if (seg->segtype & ST_VIA) {
            cx->rt->flags |= RT_OUTPUT;
            if (!special) {
                lefu_t vx = 0;
                lefu_t vy = 0;
                u_char viaNL, viaNM, viaNU;
                u_char viaSL, viaSM, viaSU;
                u_char viaEL, viaEM, viaEU;
                u_char viaWL, viaWM, viaWU;

                if (!lastseg) {
                    // Make sure last position is valid.
                    cx->lastx = x;
                    cx->lasty = y;
                }

                // Check for vias between adjacent but different nets
                // that need position offsets to avoid a DRC spacing
                // error.

                // viaCheckX[layer] indicates whether a check for vias
                // is needed.  If so, record what vias are to east and
                // west.

                if (cx->viaCheckX[layer] > 0) {

                    viaEL = viaEM = viaEU = 0;
                    viaWL = viaWM = viaWU = 0;

                    // Check for via to west.
                    if (seg->x1 > 0) {
                        u_int tdir = obsVal(seg->x1 - 1, seg->y1, layer)
                            & ROUTED_NET_MASK;

                        if (((tdir & NO_NET) == 0) && (tdir != 0) &&
                                (tdir != (cx->net->netnum | ROUTED_NET))) {

                            if (layer < numLayers() - 1) {
                                u_int tdirp = obsVal(seg->x1 - 1, seg->y1,
                                    layer + 1) & ROUTED_NET_MASK;
                                if (((tdirp & NO_NET) == 0) && (tdirp != 0) &&
                                        (tdirp !=
                                        (cx->net->netnum | ROUTED_NET))) {

                                    if (layer < numLayers() - 2) {
                                        u_int tdirpp = obsVal(seg->x1 - 1,
                                            seg->y1, layer + 2)
                                            & ROUTED_NET_MASK;
                                        if (tdirp == tdirpp)
                                            viaWU = 1;
                                    }
                                }
                                if (tdir == tdirp)
                                    viaWM = 1;
                            }
            
                            if (layer > 0) {
                                u_int tdirn = obsVal(seg->x1 - 1, seg->y1,
                                    layer - 1) & ROUTED_NET_MASK;
                                if (tdir == tdirn)
                                    viaWL = 1;
                            }
                        }
                    }

                    // Check for via to east.
                    if (seg->x1 < numChannelsX(layer) - 1) {
                        u_int tdir = obsVal(seg->x1 + 1, seg->y1, layer)
                            & ROUTED_NET_MASK;

                        if (((tdir & NO_NET) == 0) && (tdir != 0) &&
                                (tdir != (cx->net->netnum | ROUTED_NET))) {

                            if (layer < numLayers() - 1) {
                                u_int tdirp = obsVal(seg->x1 + 1, seg->y1,
                                    layer + 1) & ROUTED_NET_MASK;
                                if (((tdirp & NO_NET) == 0) && (tdirp != 0) &&
                                        (tdirp !=
                                        (cx->net->netnum | ROUTED_NET))) {

                                    if (layer < numLayers() - 2) {
                                        u_int tdirpp = obsVal(seg->x1 + 1,
                                            seg->y1, layer + 2)
                                            & ROUTED_NET_MASK;
                                        if (tdirp == tdirpp)
                                            viaEU = 1;
                                    }
                                }
                                if (tdir == tdirp)
                                    viaEM = 1;
                            }
            
                            if (layer > 0) {
                                u_int tdirn = obsVal(seg->x1 + 1, seg->y1,
                                    layer - 1) & ROUTED_NET_MASK;
                                if (tdir == tdirn)
                                    viaEL = 1;
                            }
                        }
                    }

                    // Compute X offset
                    vx = 0;
                    if (viaWL)
                        vx = cx->viaOffsetX[layer][0];
                    else if (viaEL)
                        vx = -cx->viaOffsetX[layer][0];

                    if (viaWM && cx->viaOffsetX[layer][1] > vx)
                        vx = cx->viaOffsetX[layer][1];
                    else if (viaEM && -cx->viaOffsetX[layer][1] < vx)
                        vx = -cx->viaOffsetX[layer][1];

                    if (viaWU && cx->viaOffsetX[layer][2] > vx)
                        vx = cx->viaOffsetX[layer][2];
                    else if (viaEU && -cx->viaOffsetX[layer][2] < vx)
                        vx = -cx->viaOffsetX[layer][2];
                }

                // viaCheckY[layer] indicates whether a check for vias
                // is needed.  If so, record what vias are to north
                // and south.

                if (cx->viaCheckY[layer] > 0) {

                    viaNL = viaNM = viaNU = 0;
                    viaSL = viaSM = viaSU = 0;

                    // Check for via to south.
                    if (seg->y1 > 0) {
                        u_int tdir = obsVal(seg->x1, seg->y1 - 1, layer)
                            & ROUTED_NET_MASK;

                        if (((tdir & NO_NET) == 0) && (tdir != 0) &&
                                (tdir != (cx->net->netnum | ROUTED_NET))) {

                            if (layer < numLayers() - 1) {
                                u_int tdirp = obsVal(seg->x1, seg->y1 - 1,
                                    layer + 1) & ROUTED_NET_MASK;
                                if (((tdirp & NO_NET) == 0) && (tdirp != 0) &&
                                        (tdirp !=
                                        (cx->net->netnum | ROUTED_NET))) {

                                    if (layer < numLayers() - 2) {
                                        u_int tdirpp = obsVal(seg->x1,
                                            seg->y1 - 1, layer + 2)
                                            & ROUTED_NET_MASK;
                                        if (tdirp == tdirpp)
                                            viaSU = 1;
                                    }
                                }
                                if (tdir == tdirp)
                                    viaSM = 1;
                            }
            
                            if (layer > 0) {
                                u_int tdirn = obsVal(seg->x1, seg->y1 - 1,
                                    layer - 1) & ROUTED_NET_MASK;
                                if (tdir == tdirn)
                                    viaSL = 1;
                            }
                        }
                    }

                    // Check for via to north
                    if (seg->y1 < numChannelsY(layer) - 1) {
                        u_int tdir = obsVal(seg->x1, seg->y1 + 1, layer)
                            & ROUTED_NET_MASK;

                        if (((tdir & NO_NET) == 0) && (tdir != 0) &&
                                (tdir != (cx->net->netnum | ROUTED_NET))) {

                            if (layer < numLayers() - 1) {
                                u_int tdirp = obsVal(seg->x1, seg->y1 + 1,
                                    layer + 1) & ROUTED_NET_MASK;
                                if (((tdirp & NO_NET) == 0) && (tdirp != 0) &&
                                        (tdirp !=
                                        (cx->net->netnum | ROUTED_NET))) {

                                    if (layer < numLayers() - 2) {
                                        u_int tdirpp = obsVal(seg->x1,
                                            seg->y1 + 1, layer + 2)
                                            & ROUTED_NET_MASK;
                                        if (tdirp == tdirpp)
                                            viaNU = 1;
                                    }
                                }
                                if (tdir == tdirp)
                                    viaNM = 1;
                            }
            
                            if (layer > 0) {
                                u_int tdirn = obsVal(seg->x1, seg->y1 + 1,
                                    layer - 1) & ROUTED_NET_MASK;
                                if (tdir == tdirn)
                                    viaNL = 1;
                            }
                        }
                    }

                    // Compute Y offset
                    vy = 0;
                    if (viaSL)
                        vy = cx->viaOffsetY[layer][0];
                    else if (viaNL)
                        vy = -cx->viaOffsetY[layer][0];

                    if (viaSM && cx->viaOffsetY[layer][1] > vy)
                        vy = cx->viaOffsetY[layer][1];
                    else if (viaNM && -cx->viaOffsetY[layer][1] < vy)
                        vy = -cx->viaOffsetY[layer][1];

                    if (viaSU && cx->viaOffsetY[layer][2] > vy)
                        vy = cx->viaOffsetY[layer][2];
                    else if (viaNU && -cx->viaOffsetY[layer][2] < vy)
                        vy = -cx->viaOffsetY[layer][2];
                }

                // via-to-via interactions are symmetric, so move each
                // via half the distance (?)

                pathvia(layer, x + vx, y + vy, cx->lastx, cx->lasty,
                    seg->x1, seg->y1);

                cx->lastx = x;
                cx->lasty = y;
                cx->lastlay = -1;
            }
        }

        // Break here on last segment so that seg and
        // lastseg are valid in the following section of
        // code.

        if (!seg->next)
            break;

        cx->prevseg = lastseg;
        lastseg = seg;
        cx->lastseg = seg;
    }
}


// Check last position for terminal offsets.
//
void
cMRouter::check_last_offset(bool special)
{
    sPhysRouteGenCx *cx = mr_route_gen;
    if (!cx)
        return;
    dbSeg *seg = cx->seg;
    if (!seg)
        return;
    if (seg == cx->rt->segments && !(seg->segtype & ST_WIRE))
        return;

    u_int layer = seg->layer;
    bool cancel = false;

    // Look for stub routes and offset taps
    u_int dir2 = obsVal(seg->x2, seg->y2, layer) & (STUBROUTE | OFFSET_TAP);

    if ((dir2 & OFFSET_TAP) && (seg->segtype & ST_VIA) && cx->prevseg) {

        // Additional handling for offset taps.  When a tap position
        // is a via and is offset in the direction of the last route
        // segment, then a DRC violation can be created if (1) the via
        // is wider than the route width, and (2) the adjacent track
        // position is another via or a bend in the route, and (3) the
        // tap offset is large enough to create a spacing violation
        // between the via and the adjacent via or perpendicular
        // route.  If these three conditions are satisfied, then
        // generate a stub route the width of the via and one track
        // pitch in length back toward the last track position.

        // Problems only arise when the via width is larger than the
        // width of the metal route leaving the via.

        lefu_t offset = layer < pinLayers() ?
            offsetVal(seg->x2, seg->y2, layer) : 0;
        if (getViaWidth(seg->layer, cx->lastseg->layer,
            cx->direction == DIR_HORIZ ? DIR_VERT : DIR_HORIZ) >
                getRouteWidth(cx->lastseg->layer)) {

            // Problems only arise when the last segment is exactly
            // one track long.

            if ((LD_ABSDIFF(cx->lastseg->x2, cx->lastseg->x1) == 1) ||
                    (LD_ABSDIFF(cx->lastseg->y2, cx->lastseg->y1) == 1)) {

                // This block was moved to be outside of the
                // conditional below, Qrouter bug?
                //
                lefu_t dc = xLower() + seg->x1 * pitchX(layer);
                lefu_t x = dc;
                dc = yLower() + seg->y1 * pitchY(layer);
                lefu_t y = dc;

                dc = xLower() + cx->prevseg->x1 * pitchX(layer);
                lefu_t x2 = dc;
                dc = yLower() + cx->prevseg->y1 * pitchY(layer);
                lefu_t y2 = dc;

                if (cx->prevseg->segtype & ST_VIA) {
                    // Setup is (via, 1 track route, via with offset)

                    if (cx->prevseg->x1 != seg->x1) {
                        if ((pitchX(cx->lastseg->layer) -
                                getViaWidth(seg->layer, cx->lastseg->layer,
                                    DIR_HORIZ)/2 -
                                getViaWidth(cx->prevseg->layer,
                                    cx->lastseg->layer, DIR_HORIZ)/2 -
                                (cx->prevseg->x1 - seg->x1) * offset)
                                < getRouteSpacing(cx->lastseg->layer)) {
                            if (!special) {
                                cx->rt->flags |= RT_STUB;
                                cx->net->flags |= NET_STUB;
                            }
                            else {
                                pathstub(cx->lastseg->layer, x, y, x2, y2,
                                    DIR_HORIZ);
                                cx->lastx = x2;
                                cx->lasty = y2;
                            }
                        }
                    }
                    else if (cx->prevseg->y1 != seg->y1) {
                        if ((pitchY(cx->lastseg->layer) -
                                getViaWidth(seg->layer, cx->lastseg->layer,
                                    DIR_VERT)/2 -
                                getViaWidth(cx->prevseg->layer,
                                    cx->lastseg->layer, DIR_VERT)/2 -
                                (cx->prevseg->y1 - seg->y1) * offset)
                                < getRouteSpacing(cx->lastseg->layer)) {
                            if (!special) {
                                cx->rt->flags |= RT_STUB;
                                cx->net->flags |= NET_STUB;
                            }
                            else {
                                pathstub(cx->lastseg->layer, x, y, x2, y2,
                                    DIR_VERT);
                                cx->lastx = x2;
                                cx->lasty = y2;
                            }
                        }
                    }
                }
                else {  // Metal route bends at next track
                    // The x,y,x2,y2 define above are now in scope here.

                    if (cx->prevseg->x1 != seg->x1) {
                        if ((pitchX(cx->lastseg->layer) -
                                getViaWidth(seg->layer, cx->lastseg->layer,
                                    DIR_HORIZ)/2 -
                                getRouteWidth(cx->prevseg->layer)/2 -
                                    (cx->prevseg->x1 - seg->x1) * offset)
                                < getRouteSpacing(cx->lastseg->layer)) {
                            if (!special) {
                                cx->rt->flags |= RT_STUB;
                                cx->net->flags |= NET_STUB;
                            }
                            else {
                                pathstub(cx->lastseg->layer, x, y, x2, y2,
                                    DIR_HORIZ);
                                cx->lastx = x2;
                                cx->lasty = y2;
                            }
                        }
                    }
                    else if (cx->prevseg->y1 != seg->y1) {
                        if ((pitchY(cx->lastseg->layer) -
                                getViaWidth(seg->layer, cx->lastseg->layer,
                                    DIR_VERT)/2 -
                                getRouteWidth(cx->prevseg->layer)/2 -
                                    (cx->prevseg->y1 - seg->y1) * offset)
                                < getRouteSpacing(cx->lastseg->layer)) {
                            if (!special) {
                                cx->rt->flags |= RT_STUB;
                                cx->net->flags |= NET_STUB;
                            }
                            else {
                                pathstub(cx->lastseg->layer, x, y, x2, y2,
                                    DIR_VERT);
                                cx->lastx = x2;
                                cx->lasty = y2;
                            }
                        }
                    }
                }
            }
        }
    }

    // For stub routes, reset the path between terminals, since
    // the stubs are not connected.
    if (special && cx->pathOn != PathNONE)
        cx->pathOn = PathDONE;

    // Handling of stub routes
    if (dir2 & STUBROUTE) {
        mrGridCell c;
        initGridCell(c, seg->x2, seg->y2, layer);
        lefu_t stub = stubVal(c);
        u_int flags = flagsVal(c);
        lefu_t offset = offsetVal(c);

        if (!special && (verbose() > 2)) {
            db->emitMesg(
                "Stub route distance %g to terminal at %d %d (%d)\n",
                db->lefToMic(stub), seg->x2, seg->y2, layer);
        }

        lefu_t dc = xLower() + seg->x2 * pitchX(layer);
        if (flags & NI_OFFSET_EW)
            dc += offset;
        lefu_t x = dc;
        if (flags & NI_STUB_EW)
            dc += stub;
        lefu_t x2 = dc;
        dc = yLower() + seg->y2 * pitchY(layer);
        if (flags & NI_OFFSET_NS)
            dc += offset;
        lefu_t y = dc;
        if (flags & NI_STUB_NS)
            dc += stub;
        lefu_t y2 = dc;

        if (flags & NI_STUB_EW) {
            cx->direction = DIR_HORIZ;

            // If the gridpoint ahead of the stub has a route on the
            // same net, and the stub is long enough to come within a
            // DRC spacing distance of the other route, then lengthen
            // it to close up the distance and resolve the error.

            if ((x < x2) && (seg->x2 < (numChannelsX(layer) - 1))) {
                u_int tdir = obsVal(seg->x2 + 1, seg->y2, layer);
                if ((tdir & ROUTED_NET_MASK) ==
                        (cx->net->netnum | ROUTED_NET)) {
                    if (stub + getRouteKeepout(layer) >= pitchX(layer)) {
                        dc = xLower() + (seg->x2 + 1) * pitchX(layer);
                        x2 = dc;
                    }
                }
            }
            else if ((x > x2) && (seg->x2 > 0)) {
                u_int tdir = obsVal(seg->x2 - 1, seg->y2, layer);
                if ((tdir & ROUTED_NET_MASK) ==
                        (cx->net->netnum | ROUTED_NET)) {
                    if (-stub + getRouteKeepout(layer) >= pitchX(layer)) {
                        dc = xLower() + (seg->x2 - 1) * pitchX(layer);
                        x2 = dc;
                    }
                }
            }

            dc = getRouteWidth(layer)/2;
            if (!special) {
                // Regular nets include 1/2 route width at the ends,
                // so subtract from the stub terminus.

                if (x < x2) {
                    x2 -= dc;
                    if (x >= x2)
                        cancel = true;
                }
                else {
                    x2 += dc;
                    if (x <= x2)
                        cancel = true;
                }
            }
            else {
                // Special nets don't include 1/2 route width at the
                // ends, so add to the route at the grid.

                if (x < x2)
                    x -= dc;
                else
                    x += dc;

                // Routes that extend for more than one track without
                // a bend do not need a wide stub.

                if (seg->x1 != seg->x2)
                    cancel = true;
            }
        }
        else {
            cx->direction = DIR_VERT;

            // If the gridpoint ahead of the stub has a route on the
            // same net, and the stub is long enough to come within a
            // DRC spacing distance of the other route, then lengthen
            // it to close up the distance and resolve the error.

            if ((y < y2) && (seg->y2 < (numChannelsY(layer) - 1))) {
                u_int tdir = obsVal(seg->x2, seg->y2 + 1, layer);
                if ((tdir & ROUTED_NET_MASK) ==
                        (cx->net->netnum | ROUTED_NET)) {
                    if (stub + getRouteKeepout(layer) >= pitchY(layer)) {
                        dc = yLower() + (seg->y2 + 1) * pitchY(layer);
                        y2 = dc;
                    }
                }
            }
            else if ((y > y2) && (seg->y2 > 0)) {
                u_int tdir = obsVal(seg->x2, seg->y2 - 1, layer);
                if ((tdir & ROUTED_NET_MASK) ==
                        (cx->net->netnum | ROUTED_NET)) {
                    if (-stub + getRouteKeepout(layer) >= pitchY(layer)) {
                        dc = yLower() + (seg->y2 - 1) * pitchY(layer);
                        y2 = dc;
                    }
                }
            }

            dc = getRouteWidth(layer)/2;
            if (!special) {
                // Regular nets include 1/2 route width at the ends,
                // so subtract from the stub terminus.

                if (y < y2) {
                    y2 -= dc;
                    if (y >= y2)
                        cancel = true;
                }
                else {
                    y2 += dc;
                    if (y <= y2)
                        cancel = true;
                }
            }
            else {
                // Special nets don't include 1/2 route width at the
                // ends, so add to the route at the grid.

                if (y < y2)
                    y -= dc;
                else
                    y += dc;

                // Routes that extend for more than one
                // track without a bend do not need a wide stub.

                if (seg->y1 != seg->y2)
                    cancel = true;
            }
        }
        if (!cancel) {
            cx->net->flags |= NET_STUB;
            cx->rt->flags |= RT_STUB;
            if (special)
                pathstub(layer, x, y, x2, y2, cx->direction);
            else {
                if (cx->pathOn != PathSTART) {
                    pathstart(layer, x, y);
                    cx->lastx = x;
                    cx->lasty = y;
                }
                pathto(x2, y2, cx->direction, cx->lastx, cx->lasty);
            }
            cx->lastx = x2;
            cx->lasty = y2;
        }
    }
}


// The following functions were originally used to output the DEF
// text.  Now, these build up a list of dbPath structures that
// describe physical objects, which is subsequently used to generate
// the text.

// pathstart
//
// Begin a route path, normal (not special) nets only.
//
void
cMRouter::pathstart(int layer, lefu_t x, lefu_t y)
{
    sPhysRouteGenCx *cx = mr_route_gen;
    if (!cx)
        return;
    if (cx->pathOn == PathSTART) {
        db->emitErrMesg(
            "pathstart:  Started a new path while one is in progress!\n"
            "Doing it anyway.\n");
    }
    if (layer < 0) {
        db->emitErrMesg(
            "pathstart:  unknown layer (negative index), point %d,%d.\n",
            x, y);
    }
    if (!cx->net->path)
        cx->net->path = cx->endp = new dbPath(x, y);
    else {
        cx->endp->next = new dbPath(x, y);
        cx->endp = cx->endp->next;
    }
    cx->endp->layer = layer;
#ifdef DEBUG_RT
    db->emitMesg("      start %d %d\n", db->lefToDef(x), db->lefToDef(y));
#endif
   cx->pathOn = PathSTART;
}


// pathto
//
// Continue a path to the next point, normal (not special) nets only.
//
void
cMRouter::pathto(lefu_t x, lefu_t y, ROUTE_DIR direction, lefu_t lastx,
    lefu_t lasty)
{
    sPhysRouteGenCx *cx = mr_route_gen;
    if (!cx)
        return;
    if (cx->pathOn != PathSTART) {
        db->emitErrMesg(
            "pathto:  Added to a non-existent path!\n"
            "Doing it anyway.\n");
    }

    // If the route is not manhattan, then it's because an offset was
    // added to the last point, and we need to add a small jog to the
    // route.

    if ((x != lastx) && (y != lasty)) {
        if (direction == DIR_VERT)
            pathto(lastx, y, DIR_VERT, lastx, lasty);
        else
            pathto(x, lasty, DIR_HORIZ, lastx, lasty);
    }
    if (cx->endp) {
        cx->endp->next = new dbPath(x, y);
        cx->endp = cx->endp->next;
#ifdef DEBUG_RT
        db->emitMesg("      to %d %d\n", db->lefToDef(x), db->lefToDef(y));
#endif
    }
}


// pathvia
//
// Add a via to a path.  Called for normal (not special) nets only.
//
void
cMRouter::pathvia(int layer, lefu_t x, lefu_t y, lefu_t lastx, lefu_t lasty,
    int gridx, int gridy)
{
    sPhysRouteGenCx *cx = mr_route_gen;
    if (!cx)
        return;
    if (layer < 0) {
        db->emitErrMesg(
            "pathvia:  unknown layer (negative index), point %d,%d.\n", x, y);
    }
    char checkersign = (gridx + gridy + layer) & 0x01;

    int vid = -1;
    if ((viaPattern() == VIA_PATTERN_NONE) || (viaYid(layer) < 0))
        vid = viaXid(layer);
    else if (viaPattern() == VIA_PATTERN_NORMAL)
        vid = (checkersign == 0) ?  viaXid(layer) : viaYid(layer);
    else
        vid = (checkersign == 0) ?  viaYid(layer) : viaXid(layer);
    lefObject *lefo = db->getLefObject(vid);

    if (!lefo) {
        db->emitErrMesg("pathVia:  can't find LEF object id %d.\n", vid);
        return;
    }

    if (cx->pathOn == PathSTART) {
        // Normally the path will be manhattan and only one of these
        // will be true.  But if the via gets an offset to avoid a DRC
        // spacing violation with an adjacent via, then we may need to
        // apply both paths to make a dog-leg route to the via.

        if (x != lastx)
            pathto(x, lasty, DIR_HORIZ, lastx, lasty);
        if (y != lasty)
            pathto(x, y, DIR_VERT, x, lasty);
    }

#ifdef DEBUG_RT
    db->emitMesg("      via %d %d %s\n", db->lefToDef(x), db->lefToDef(y), s);
#endif
    if (!cx->net->path) {
        cx->net->path = new dbPath(x, y);
        cx->endp = cx->net->path;
        cx->endp->layer = layer;
    }
    else if (cx->pathOn != PathSTART) {
        cx->endp->next = new dbPath(x, y);
        cx->endp = cx->endp->next;
        cx->endp->layer = layer;
    }
    else if (cx->endp->x != x || cx->endp->y != y) {
        cx->endp->next = new dbPath(x, y);
        cx->endp = cx->endp->next;
    }
    cx->endp->vid = lefo->lefId;

    cx->pathOn = PathDONE;
}


// pathstub
//
// Add a stub to the special routes list.
//
void
cMRouter::pathstub(int layer, lefu_t x1, lefu_t y1, lefu_t x2, lefu_t y2,
    ROUTE_DIR direction)
{
    sPhysRouteGenCx *cx = mr_route_gen;
    if (!cx)
        return;
    if (layer >= 0) {
        if (!cx->net->spath)
            cx->net->spath = cx->endsp = new dbPath(x1, y1);
        else {
            cx->endsp->next = new dbPath(x1, y1);
            cx->endsp = cx->endsp->next;
        }
        cx->endsp->layer = layer;
        lefu_t wvia = getViaWidth(layer, layer, direction);
        if (layer > 0) {
            lefu_t wvia2 = getViaWidth(layer - 1, layer, direction);
            if (wvia2 > wvia)
                wvia = wvia2;
        }
        cx->endsp->width = wvia;
        cx->endsp->next = new dbPath(x2, y2);
        cx->endsp = cx->endsp->next;
#ifdef DEBUG_RT
        db->emitMesg("      stub %d %d  to %d %d  width %d\n",
            db->lefToDef(x1), db->lefToDef(y1),
            db->lefToDef(x2), db->lefToDef(y2), db->lefToDef(wvia));
#endif
    }
}


// cleanup_net
//
// Special handling for layers where needBlock() is non-zero, and
// shows that two vias cannot be placed on adjacent routes. 
// writeDefNetRoutes() will add specialnets to merge two adjacent vias
// on the same route.  However, this cannot be used for adjacent vias
// that are each in a different route record.  It is easier just to
// find any such instances and remove them by eliminating one of the
// vias and adding a segment to connect the route to the neighboring
// via.
//
void
cMRouter::cleanup_net(dbNet *net)
{
    int lf = -1;
    int ll = -1;
    int lf2 = -1;
    int ll2 = -1;

    for (dbRoute *rt = net->routes; rt; rt = rt->next) {
        bool fcheck = false;
        bool lcheck = false;

        // This problem will only show up on route endpoints.  segf is
        // the first segment of the route.  segl is the last segment
        // of the route.  lf is the layer at the route start (layer
        // first) lf2 is the layer of the second segment.  ll is the
        // layer at the route end (layer last) ll2 is the layer of the
        // next-to-last segment

        dbSeg *segf = rt->segments;
        if (!segf)
            continue;
        if (segf->next && (segf->segtype == ST_VIA)) {
            if (segf->next->layer > segf->layer) {
                lf = segf->layer;
                lf2 = segf->layer + 1;
            }
            else {
                lf = segf->layer + 1;
                lf2 = segf->layer;
            }
            // Set flag fcheck indicating that segf needs checking
            fcheck = true;

            // We're going to remove the contact so it can't be a tap.
            if ((lf < (int)pinLayers()) && nodeSav(segf->x1, segf->y1, lf))
                fcheck = false;
        }
        bool xcheckf = needBlock(lf) & VIABLOCKX;
        bool ycheckf = needBlock(lf) & VIABLOCKY;
        if (!xcheckf && !ycheckf)
            fcheck = false;

        // Move to the next-to-last segment.
        dbSeg *segl = segf->next;
        if (segl) {
            while (segl->next && segl->next->next)
                segl = segl->next;
        }

        if (segl && segl->next && (segl->next->segtype == ST_VIA)) {
            if (segl->next->layer < segl->layer) {
                ll = segl->next->layer;
                ll2 = segl->next->layer + 1;
            }
            else {
                ll = segl->next->layer + 1;
                ll2 = segl->next->layer;
            }
            // Move segl to the last segment.
            segl = segl->next;
            // Set flag lcheck indicating that segl needs checking.
            lcheck = true;

            // We're going to remove the contact so it can't be a tap.
            if ((ll < (int)pinLayers()) && nodeSav(segl->x1, segl->y1, ll))
                lcheck = false;
        }
        bool xcheckl = needBlock(ll) & VIABLOCKX;
        bool ycheckl = needBlock(ll) & VIABLOCKY;
        if (!xcheckl && !ycheckl)
            lcheck = false;

        // For each route rt2 that is not rt, look at every via and
        // see if it is adjacent to segf or segl.

        for (dbRoute *rt2 = net->routes; rt2; rt2 = rt2->next) {

            if (!fcheck && !lcheck)
                break;
            if (rt2 == rt)
                continue;

            for (dbSeg *seg = rt2->segments; seg; seg = seg->next) {
                if (seg->segtype & ST_VIA) {
                    if (fcheck) {
                        if ((seg->layer == lf) || ((seg->layer + 1) == lf)) {
                            if (xcheckf && (seg->y1 == segf->y1) &&
                                    (LD_ABSDIFF(seg->x1, segf->x1) == 1)) {
                                if (seg->layer != segf->layer) {

                                    // Adjacent vias are different types. 
                                    // Deal with it by creating a route
                                    // between the vias on their shared
                                    // layer.  This will later be made
                                    // into a special net to avoid notch
                                    // DRC errors.

                                    rt->segments = new dbSeg(lf, -1, ST_WIRE,
                                        segf->x1, segf->y1, seg->x1, seg->y1,
                                        segf);
                                }
                                else {
                                    // Change via to wire route, connect
                                    // it to seg, and make sure it has the
                                    // same layer type as the following
                                    // route.

                                    segf->segtype = ST_WIRE;
                                    segf->x1 = seg->x1;
                                    segf->layer = lf2;
                                }
                            }
                            else if (ycheckf && (seg->x1 == segf->x1) &&
                                    (LD_ABSDIFF(seg->y1, segf->y1) == 1)) {
                                if (seg->layer != segf->layer) {
                                    // Adjacent vias are different types. 
                                    // Deal with it by creating a route
                                    // between the vias on their shared
                                    // layer.  This will later be made
                                    // into a special net to avoid notch
                                    // DRC errors.

                                    rt->segments = new dbSeg(lf, -1, ST_WIRE,
                                        segf->x1, segf->y1, seg->x1, seg->y1,
                                        segf);
                                }
                                else {
                                    // Change via to wire route, connect
                                    // it to seg, and make sure it has the
                                    // same layer type as the following
                                    // route.

                                    segf->segtype = ST_WIRE;
                                    segf->y1 = seg->y1;
                                    segf->layer = lf2;
                                }
                            }
                        }
                    }

                    if (lcheck) {
                        if ((seg->layer == ll) || ((seg->layer + 1) == ll)) {
                            if (xcheckl && (seg->y1 == segl->y1) &&
                                    (LD_ABSDIFF(seg->x1, segl->x1) == 1)) {
                                if (seg->layer != segl->layer) {

                                    // Adjacent vias are different types. 
                                    // Deal with it by creating a route
                                    // between the vias on their shared
                                    // layer.  This will later be made
                                    // into a special net to avoid notch
                                    // DRC errors.

                                    segl->next = new dbSeg(ll, -1, ST_WIRE,
                                        segl->x1, segl->y1, seg->x1, seg->y1,
                                        0);
                                }
                                else {
                                    // Change via to wire route, connect
                                    // it to seg, and make sure it has the
                                    // same layer type as the previous
                                    // route.

                                    segl->segtype = ST_WIRE;
                                    segl->x2 = seg->x2;
                                    segl->layer = ll2;
                                }
                            }
                            else if (ycheckl && (seg->x1 == segl->x1) &&
                                    (LD_ABSDIFF(seg->y1, segl->y1) == 1)) {
                                if (seg->layer != segl->layer) {

                                    // Adjacent vias are different types. 
                                    // Deal with it by creating a route
                                    // between the vias on their shared
                                    // layer.  This will later be made
                                    // into a special net to avoid notch
                                    // DRC errors.

                                    segl->next = new dbSeg(ll, -1, ST_WIRE,
                                        segl->x1, segl->y1, seg->x1, seg->y1,
                                        0);
                                }
                                else {
                                    // Change via to wire route, connect
                                    // it to seg, and make sure it has the
                                    // same layer type as the previous
                                    // route.

                                    segl->segtype = ST_WIRE;
                                    segl->y2 = seg->y2;
                                    segl->layer = ll2;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

