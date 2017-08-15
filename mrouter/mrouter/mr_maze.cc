
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

//--------------------------------------------------------------
// maze.c -- general purpose maze router functions.              
//                                                              
// This file contains the main cost evaluation function,         
// the route segment generator, and a function to search         
// the database for all parts of a routed network.              
//--------------------------------------------------------------
// Written by Tim Edwards, June 2011, based on code by Steve    
// Beccue                                                       
//--------------------------------------------------------------

#include "mrouter_prv.h"


//
// Maze Router.
//
// Misc. routing operations, pretty much verbatim from Qrouter.
//

//#define DEBUG_RU

// ripupNet
//
// Rip up the entire network.
//
// If argument "restore" is TRUE, then at each node, restore the
// crossover cost by attaching the node back to the NODELOC array.
//
bool
cMRouter::ripupNet(dbNet *net, bool restore)
{
#ifdef DEBUG_RU
printf("RU %s %d\n", net->netname, net->numnodes);
#endif
    u_int thisnet = net->netnum;
    for (dbRoute *rt = net->routes; rt; rt = rt->next) {
        for (dbSeg *seg = rt->segments; seg; seg = seg->next) {
            u_int lay = seg->layer;
            int x = seg->x1;
            int y = seg->y1;
            for (;;) {
                mrGridCell c;
                initGridCell(c, x, y, lay);

                u_int oldnet = obsVal(c) & NETNUM_MASK;
                if ((oldnet > 0) && (oldnet < maxNetNum())) {
                    if (oldnet != thisnet) {
                        db->emitErrMesg(
                            "Error: position %d %d layer %d has net "
                            "%d not %d!\n", x, y, lay, oldnet, thisnet);
                            return (LD_BAD);     // Something went wrong
                    }

                    // Reset the net number to zero along this
                    // route for every point that is not a node
                    // tap.  Points that were routed over
                    // obstructions to reach off-grid taps are
                    // returned to obstructions.

                    if ((lay >= pinLayers()) || (nodeSav(c) == 0)) {
                        int dir = obsVal(c) & PINOBSTRUCTMASK;
                        if (dir == 0)
                            setObsVal(c, (obsVal(c) & BLOCKED_MASK));
                        else
                            setObsVal(c, (NO_NET | dir));
                    }
                    else {
                        // Clear routed mask bit.

                        setObsVal(c, (obsVal(c) & ~ROUTED_NET));
                    }
#ifdef DEBUG_RU
printf("RUa %d %d %d %x\n", x, y, lay, obsVal(c));
#endif

                    // Routes which had blockages added on the
                    // sides due to spacing constraints have
                    // DRC_BLOCKAGE set; these flags should be
                    // removed.

                    if (needBlock(lay) & (ROUTEBLOCKX | VIABLOCKX)) {
                        mrGridCell c1, c2;
                        initGridCell(c1, x - 1, y, lay);
                        initGridCell(c2, x + 1, y, lay);

                        if ((x > 0) && ((obsVal(c1) & DRC_BLOCKAGE) ==
                                DRC_BLOCKAGE)) {
                            setObsVal(c1, (obsVal(c1) & ~DRC_BLOCKAGE));
#ifdef DEBUG_RU
printf("RUb %d %d %d %x\n", x-1, y, lay, obsVal(c1));
#endif
                        }
                        else if ((x < numChannelsX(lay) - 1) &&
                                ((obsVal(c2) & DRC_BLOCKAGE) ==
                                DRC_BLOCKAGE)) {
                            setObsVal(c2, (obsVal(c2) & ~DRC_BLOCKAGE));
#ifdef DEBUG_RU
printf("RUc %d %d %d %x\n", x+1, y, lay, obsVal(c2));
#endif
                        }
                    }
                    if (needBlock(lay) & (ROUTEBLOCKY | VIABLOCKY)) {
                        mrGridCell c1, c2;
                        initGridCell(c1, x, y - 1, lay);
                        initGridCell(c2, x, y + 1, lay);

                        if ((y > 0) && ((obsVal(c1) & DRC_BLOCKAGE) ==
                                DRC_BLOCKAGE)) {
                            setObsVal(c1, (obsVal(c1) & ~DRC_BLOCKAGE));
#ifdef DEBUG_RU
printf("RUd %d %d %d %x\n", x, y-1, lay, obsVal(c1));
#endif
                        }
                        else if ((y < numChannelsY(lay) - 1) &&
                                ((obsVal(c2) & DRC_BLOCKAGE) ==
                                DRC_BLOCKAGE)) {
                            setObsVal(c2, (obsVal(c2) & ~DRC_BLOCKAGE));
#ifdef DEBUG_RU
printf("RUe %d %d %d %x\n", x, y+1, lay, obsVal(c2));
#endif
                        }
                    }
                }

                // This break condition misses via ends, but those
                // are terminals and don't get ripped out.

                if ((x == seg->x2) && (y == seg->y2))
                    break;

                if (x < seg->x2)
                    x++;
                else if (x > seg->x2)
                    x--;
                if (y < seg->y2)
                    y++;
                else if (y > seg->y2)
                    y--;
            }
        }
    }

    // For each net node tap, restore the node pointer on NODELOC so
    // that crossover costs are again applied to routes over this node
    // tap.

    if (restore) {
        for (dbNode *node = net->netnodes; node; node = node->next) {
            for (dbDpoint *ntap = node->taps; ntap; ntap = ntap->next) {
                u_int lay = ntap->layer;
                int x = ntap->gridx;
                int y = ntap->gridy;
                if (lay < pinLayers()) {
                    mrGridCell c;
                    initGridCell(c, x, y, lay);

                    setNodeLoc(c, nodeSav(c));
#ifdef DEBUG_RU
printf("RUf %d %d %d\n", x, y, lay);
#endif
                }
            }
        }
    }

    // Remove all routing information from this net.

    net->clear_routes();

    // If this was a specialnet (numnodes set to 0), then routes are
    // considered fixed obstructions and cannot be removed.

    return ((net->numnodes == 0) ? LD_BAD : LD_OK);
}


// find_unrouted_node
//
// On a power bus, the nodes are routed individually, using the entire
// power bus as the destination.  So to find out if a node is already
// routed or not, the only way is to check the routes recorded for the
// net and determine if any net endpoint is on a node. 
//
// Return the first node found that is not connected to any route
// endpoint.  If all nodes are routed, then return 0.
//
dbNode *
cMRouter::find_unrouted_node(dbNet *net)
{
    // Quick check:  If the number of routes == number of nodes,
    // then return NULL and we're done.

    int numroutes = 0;
    for (dbRoute *rt = net->routes; rt; rt = rt->next)
        numroutes++;
    if (numroutes == net->numnodes)
        return (0);

    u_char *routednode = new u_char[net->numnodes];
    for (int i = 0; i < net->numnodes; i++)
        routednode[i] = 0;

    // Otherwise, we don't know which nodes have been routed,
    // so check each one individually.

    for (dbRoute *rt = net->routes; rt; rt = rt->next) {
        dbSeg *seg1 = rt->segments;
        if (!seg1)
            continue;
        dbSeg *seg2 = seg1;
        while (seg2->next)
            seg2 = seg2->next;

        for (dbNode *node = net->netnodes; node; node = node->next) {
            if (routednode[node->nodenum] == 1)
                continue;

            dbDpoint *tap = node->taps;
            for ( ; tap; tap = tap->next) {
                if (seg1->x1 == tap->gridx && seg1->y1 == tap->gridy &&
                        seg1->layer == tap->layer) {
                    routednode[node->nodenum] = 1;
                    break;
                }
                else if (seg1->x2 == tap->gridx && seg1->y2 == tap->gridy &&
                        seg1->layer == tap->layer) {
                    routednode[node->nodenum] = 1;
                    break;
                }
                else if (seg2->x1 == tap->gridx && seg2->y1 == tap->gridy &&
                        seg2->layer == tap->layer) {
                    routednode[node->nodenum] = 1;
                    break;
                }
                else if (seg2->x2 == tap->gridx && seg2->y2 == tap->gridy &&
                        seg2->layer == tap->layer) {
                    routednode[node->nodenum] = 1;
                    break;
                }
            }
            if (tap)
                continue;

            tap = node->extend;
            for ( ; tap; tap = tap->next) {

                seg1 = rt->segments;
                seg2 = seg1;
                while (seg2->next)
                    seg2 = seg2->next;

                if (seg1->x1 == tap->gridx && seg1->y1 == tap->gridy &&
                        seg1->layer == tap->layer) {
                    routednode[node->nodenum] = 1;
                    break;
                }
                else if (seg1->x2 == tap->gridx && seg1->y2 == tap->gridy &&
                        seg1->layer == tap->layer) {
                    routednode[node->nodenum] = 1;
                    break;
                }
                else if (seg2->x1 == tap->gridx && seg2->y1 == tap->gridy &&
                        seg2->layer == tap->layer) {
                    routednode[node->nodenum] = 1;
                    break;
                }
                else if (seg2->x2 == tap->gridx && seg2->y2 == tap->gridy &&
                        seg2->layer == tap->layer) {
                    routednode[node->nodenum] = 1;
                    break;
                }
            }
        }
    }

    for (dbNode *node = net->netnodes; node; node = node->next) {
        if (routednode[node->nodenum] == 0) {
            delete [] routednode;
            return (node);
        }
    }

    delete [] routednode;
    return (0);        // Statement should never be reached.
}


// set_powerbus_to_net
//
// If we have a power or ground net, go through the entire Obs array
// and mark all points matching the net as TARGET in Obs2.
//
// We do this after the call to PR_SOURCE, before the calls to set
// PR_TARGET.
//
// If any grid position was marked as TARGET, return mrSuccess, else
// return mrProvisional (meaning the net has been routed already).
//
mrRval
cMRouter::set_powerbus_to_net(int netnum)
{
    mrRval rval = mrProvisional;
    dbNet *net = getNetByNum(netnum);
    if (net && (net->flags & NET_GLOBAL)) {
        for (u_int lay = 0; lay < numLayers(); lay++) {
            for (int x = 0; x < numChannelsX(lay); x++) {
                for (int y = 0; y < numChannelsY(lay); y++) {
                    mrGridCell c;
                    initGridCell(c, x, y, lay);

                    if ((obsVal(c) & NETNUM_MASK) == (u_int)netnum) {
                        // Skip locations that have been purposefully
                        // disabled.

                        mrProute *Pr = obs2Val(c);
                        if (!(Pr->flags & PR_COST) &&
                                (Pr->prdata.net == maxNetNum()))
                            continue;
                        if (!(Pr->flags & PR_SOURCE)) {
                            Pr->flags |= (PR_TARGET | PR_COST);
                            Pr->prdata.cost = MAXRT;
                            rval = mrSuccess;
                        }
                    }
                }
            }
        }
    }
    return (rval);
}


//#define DEBUG_CT

// clear_non_source_targets
//
// Look at all target nodes of a net.  For any that are not marked as
// SOURCE, but have terminal points marked as PROCESSED, remove the
// PROCESSED flag and put the position back on the stack for visiting
// on the next round.
//
void
cMRouter::clear_non_source_targets(dbNet *net, mrStack *stack)
{
    for (dbNode *node = net->netnodes; node; node = node->next) {
        for (dbDpoint *ntap = node->taps; ntap; ntap = ntap->next) {
            int lay = ntap->layer;
            int x = ntap->gridx;
            int y = ntap->gridy;
            mrProute *Pr = obs2Val(x, y, lay);
            if (Pr->flags & PR_TARGET) {
                if (Pr->flags & PR_PROCESSED) {
                    Pr->flags &= ~PR_PROCESSED;
#ifdef DEBUG_CT
printf("CT1 %d %d %d\n", x, y, lay);
#endif
                    stack->push(x, y, lay);
                }
            }
        }
        for (dbDpoint *ntap = node->extend; ntap; ntap = ntap->next) {
            int lay = ntap->layer;
            int x = ntap->gridx;
            int y = ntap->gridy;
            mrProute *Pr = obs2Val(x, y, lay);
            if (Pr->flags & PR_TARGET) {
                if (Pr->flags & PR_PROCESSED) {
                    Pr->flags &= ~PR_PROCESSED;
#ifdef DEBUG_CT
printf("CT2 %d %d %d\n", x, y, lay);
#endif
                    stack->push(x, y, lay);
                }
            }
        }
    }
}


// clear_target_node
//
// Remove PR_TARGET flags from all points belonging to a node.
//
void
cMRouter::clear_target_node(dbNode *node)
{
    // Process tap points of the node.
    mrGridCell c;
    for (dbDpoint *ntap = node->taps; ntap; ntap = ntap->next) {
        initGridCell(c, ntap->gridx, ntap->gridy, ntap->layer);
        if ((ntap->layer < (int)pinLayers()) && (nodeLoc(c) == 0))
            continue;
        mrProute *Pr = obs2Val(c);
        Pr->flags = 0;
        Pr->prdata.net = node->netnum;
    }

    for (dbDpoint *ntap = node->extend; ntap; ntap = ntap->next) {
        initGridCell(c, ntap->gridx, ntap->gridy, ntap->layer);
        if (ntap->layer < (int)pinLayers() && nodeSav(c) != node)
            continue;
        mrProute *Pr = obs2Val(c);
        Pr->flags = 0;
        Pr->prdata.net = node->netnum;
    }
}


// count_targets
//
// Count the number of nodes of a net that are still marked as TARGET.
//
int
cMRouter::count_targets(dbNet *net)
{
    int count = 0;
    mrGridCell c;
    for (dbNode *node = net->netnodes; node; node = node->next) {
        dbDpoint *ntap = node->taps;
        for ( ; ntap; ntap = ntap->next) {
            initGridCell(c, ntap->gridx, ntap->gridy, ntap->layer);
            mrProute *Pr = obs2Val(c);
            if (Pr->flags & PR_TARGET) {
                count++;
                break;
            }
        }
        if (!ntap) {
            // Try extended tap areaa.s
            for (ntap = node->extend; ntap; ntap = ntap->next) {
                initGridCell(c, ntap->gridx, ntap->gridy, ntap->layer);
                mrProute *Pr = obs2Val(c);
                if (Pr->flags & PR_TARGET) {
                    count++;
                    break;
                }
            }
        }
    }
    return (count);
}


//#define DEBUG_NN

// set_node_to_net
//
// Change the Obs2[][] flag values to "newflags" for all tap positions
// of route terminal "node".  Then follow all routes connected to
// "node", updating their positions.  Where those routes connect to
// other nodes, repeat recursively.
//
// Return value is 1 if at least one terminal of the node is already
// marked as PR_SOURCE, indicating that the node has already been
// routed.  Otherwise, the return value is zero if no error occured,
// and -1 if any point was found to be unoccupied by any net, which
// should not happen.
// 
// If "bbox" is non-null, record the grid extents of the node in the
// x1, x2, y1, y2 values.
//
// If "stage" is 1 (rip-up and reroute), then don't let an existing
// route prevent us from adding terminals.  However, the area will be
// first checked for any part of the terminal that is routable, only
// resorting to overwriting colliding nets if there are no other
// available taps.  Defcon stage 3 indicates desperation due to a
// complete lack of routable taps.  This happens if, for example, a
// port is offset from the routing grid and tightly boxed in by
// obstructions.  In such case, we allow routing on an obstruction,
// but flag the point.  In the output stage, the stub route
// information will be used to reposition the contact on the port and
// away from the obstruction.
//
// If we completely fail to find a tap point under any condition, then
// return mrUnroutable.  This is a fatal error; there will be no way
// to route the net.
//
mrRval
cMRouter::set_node_to_net(dbNode *node, int newflags, mrStack *stack,
    dbSeg *bbox, mrStage stage)
{
    int obsnet = 0;
    bool found_one = false;

    // If called from set_routes_to_net, the node has no taps, and the
    // net is a power bus, just return.

    mrRval result = mrSuccess;
    if (!node->taps && !node->extend) {
        dbNet *net = getNetByNum(node->netnum);
        if (net && (net->flags & NET_GLOBAL))
            return (result);
    }

    // Process tap points of the node.

    for (dbDpoint *ntap = node->taps; ntap; ntap = ntap->next) {
        int lay = ntap->layer;
        int x = ntap->gridx;
        int y = ntap->gridy;
        mrProute *Pr = obs2Val(x, y, lay);
        if ((Pr->flags & (newflags | PR_COST)) == PR_COST) {
            db->emitErrMesg(
                "Error:  Tap position %d, %d layer %d not marked as source!\n",
                x, y, lay);
            return (mrError);   // This should not happen.
        }
#ifdef DEBUG_NN
printf("NN %d %d %d %x %d %x %x\n", x, y, lay, Pr->prdata.net, node->netnum,
  Pr->flags, newflags);
#endif

        if (Pr->flags & PR_SOURCE) {
            result = mrProvisional;     // Node is already connected!
#ifdef DEBUG_NN
printf("NN x1\n");
#endif
        }
        else if (((Pr->prdata.net == node->netnum) || (stage == mrStage2p2)) &&
                !(Pr->flags & newflags)) {
#ifdef DEBUG_NN
printf("NN x1.2\n");
#endif

            // If we got here, we're on the rip-up stage, and there is
            // an existing route completely blocking the terminal.  So
            // we will route over it and flag it as a collision.

            if (Pr->prdata.net != node->netnum) {
                if ((Pr->prdata.net == (NO_NET | OBSTRUCT_MASK)) ||
                        (Pr->prdata.net == NO_NET))
{
#ifdef DEBUG_NN
printf("NN x2\n");
#endif
                    continue;
}
                Pr->flags |= PR_CONFLICT;
            }

            // Do the source and dest nodes need to be marked routable?
            Pr->flags |= (newflags == PR_SOURCE) ?
                newflags : (newflags | PR_COST);

            Pr->prdata.cost = (newflags == PR_SOURCE) ? 0 : MAXRT;

            // Push this point on the stack to process.
            if (stack)
{
                stack->push(x, y, lay);
#ifdef DEBUG_NN
printf("NN npl1 %d %d %d\n", x, y, lay);
#endif
}
            found_one = true;

            // record extents
            if (bbox) {
                if (x < bbox->x1)
                    bbox->x1 = x;
                if (x > bbox->x2)
                    bbox->x2 = x;
                if (y < bbox->y1)
                    bbox->y1 = y;
                if (y > bbox->y2)
                    bbox->y2 = y;
            }
        }
        else if ((Pr->prdata.net < maxNetNum()) && (Pr->prdata.net > 0))
{
#ifdef DEBUG_NN
printf("NN x3\n");
#endif
            obsnet++;
}
#ifdef DEBUG_NN
else
printf("NN x4\n");
#endif
    }

    // Do the same for point in the halo around the tap, but only if
    // they have been attached to the net during a past routing run.

    for (dbDpoint *ntap = node->extend; ntap; ntap = ntap->next) {
        u_int lay = ntap->layer;
        int x = ntap->gridx;
        int y = ntap->gridy;
        mrGridCell c;
        initGridCell(c, x, y, lay);

        // Don't process extended areas if they coincide with other
        // nodes, or those that are out-of-bounds

        if (lay < pinLayers() && nodeSav(c) != node)
            continue;

        mrProute *Pr = obs2Val(c);
        if (Pr->flags & PR_SOURCE) {
            result = mrProvisional;     // Node is already connected!
        }
        else if ( !(Pr->flags & newflags) &&
                ((Pr->prdata.net == node->netnum) ||
                (stage == mrStage2p2 && Pr->prdata.net < maxNetNum()) ||
                (stage == mrStage2p3))) {

            if (Pr->prdata.net != node->netnum)
                Pr->flags |= PR_CONFLICT;
            Pr->flags |= (newflags == PR_SOURCE) ?
                newflags : (newflags | PR_COST);
            Pr->prdata.cost = (newflags == PR_SOURCE) ? 0 : MAXRT;

            // Push this point on the stack to process.
            if (stack)
{
#ifdef DEBUG_NN
printf("NN npl2 %d %d %d\n", x, y, lay);
#endif
                stack->push(x, y, lay);
}
            found_one = true;

            // record extents
            if (bbox) {
                if (x < bbox->x1)
                    bbox->x1 = x;
                if (x > bbox->x2)
                    bbox->x2 = x;
                if (y < bbox->y1)
                    bbox->y1 = y;
                if (y > bbox->y2)
                    bbox->y2 = y;
            }
        }
        else if ((Pr->prdata.net < maxNetNum()) && (Pr->prdata.net > 0))
            obsnet++;
    }

    // In the case that no valid tap points were found, if we're on
    // the rip-up and reroute section, try again, ignoring existing
    // routes that are in the way of the tap point.  If that fails,
    // then we will route over obstructions and shift the contact when
    // committing the route solution.  And if that fails, we're
    // basically hosed.
    //
    // Make sure to check for the case that the tap point is simply
    // not reachable from any grid point, in the first stage, so we
    // don't wait until the rip-up and reroute stage to route them.

    if ((result == mrSuccess) && !found_one) {
        if (stage == mrStage2) {
            return (set_node_to_net(node, newflags, stack, bbox,
                mrStage2p2));
        }
        if (stage == mrStage2p2) {
            return (set_node_to_net(node, newflags, stack, bbox,
                mrStage2p3));
        }
        if ((stage == mrStage1) && (obsnet == 0)) {
            return (set_node_to_net(node, newflags, stack, bbox,
                mrStage2p3));
        }
        return (mrUnroutable);
    }
    return (result);
}


// disable_node_nets
//
// Set all taps of node "node" to maxNetNum(), so that it will not be
// routed to.
//
bool
cMRouter::disable_node_nets(dbNode *node)
{
    bool result = false;

    // Process tap points of the node.

    for (dbDpoint *ntap = node->taps; ntap; ntap = ntap->next) {
        int lay = ntap->layer;
        int x = ntap->gridx;
        int y = ntap->gridy;
        mrProute *Pr = obs2Val(x, y, lay);
        if (Pr->flags & PR_SOURCE || Pr->flags & PR_TARGET ||
               Pr->flags & PR_COST) {
            result = true;
        }
        else if (Pr->prdata.net == node->netnum) {
            Pr->prdata.net = maxNetNum();
        }
    }

    // Do the same for point in the halo around the tap, but only if
    // they have been attached to the net during a past routing run.

    for (dbDpoint *ntap = node->extend; ntap; ntap = ntap->next) {
        int lay = ntap->layer;
        int x = ntap->gridx;
        int y = ntap->gridy;
        mrProute *Pr = obs2Val(x, y, lay);
        if (Pr->flags & PR_SOURCE || Pr->flags & PR_TARGET ||
                Pr->flags & PR_COST) {
            result = true;
        }
        else if (Pr->prdata.net == node->netnum) {
            Pr->prdata.net = maxNetNum();
        }
    }
    return (result);
}


//#define DEBUG_RN

// set_route_to_net
//
// That which is already routed (routes should be attached to source
// nodes) is routable by definition.
//
mrRval
cMRouter::set_route_to_net(dbNet *net, dbRoute *rt, int newflags,
    mrStack *stack, dbSeg *bbox, mrStage stage)
{
    mrRval result = mrSuccess;
    if (rt && rt->segments) {
        for (dbSeg *seg = rt->segments; seg; seg = seg->next) {
            u_int lay = seg->layer;
            int x = seg->x1;
            int y = seg->y1;
            for (;;) {
                mrProute *Pr = obs2Val(x, y, lay);
                Pr->flags = (newflags == PR_SOURCE) ?
                    newflags : (newflags | PR_COST);
                // Conflicts should not happen (check for this?)
                // if (Pr->prdata.net != node->netnum) Pr->flags |= PR_CONFLICT;
                Pr->prdata.cost = (newflags == PR_SOURCE) ? 0 : MAXRT;

                // Push this point on the stack to process.
                if (stack)
{
#ifdef DEBUG_RN
printf("RN %d %d %d\n", x, y, lay);
#endif
                    stack->push(x, y, lay);
}

                // record extents
                if (bbox) {
                    if (x < bbox->x1)
                        bbox->x1 = x;
                    if (x > bbox->x2)
                        bbox->x2 = x;
                    if (y < bbox->y1)
                        bbox->y1 = y;
                    if (y > bbox->y2)
                        bbox->y2 = y;
                }

                // If we found another node connected to the route,
                // then process it, too.

                dbNode *n2 = (lay >= pinLayers()) ? 0 : nodeLoc(x, y, lay);
                if (n2 && (n2 != net->netnodes)) {
                    if (newflags == PR_SOURCE)
                        clear_target_node(n2);
                    result = set_node_to_net(n2, newflags, stack, bbox, stage);
                    // On error, continue processing.
#ifdef DEBUG_RN
printf("RN x1\n");
#endif
                }

                // Process top part of via.
                if (seg->segtype & ST_VIA) {
                    if (lay != (u_int)seg->layer)
                        break;
                    lay++;
                    continue;
                }

                // Move to next grid position in segment.
                if (x == seg->x2 && y == seg->y2)
                    break;
                if (seg->x2 > seg->x1)
                    x++;
                else if (seg->x2 < seg->x1)
                    x--;
                if (seg->y2 > seg->y1)
                    y++;
                else if (seg->y2 < seg->y1)
                    y--;
            }
        }
    }
    return (result);
}


// set_routes_to_net
//
// Process all routes of a net, and set their routed positions
// to SOURCE in Obs2[].
//
mrRval
cMRouter::set_routes_to_net(dbNet *net, int newflags, mrStack *stack,
    dbSeg *bbox, mrStage stage)
{
    mrRval result = mrSuccess;
    for (dbRoute *rt = net->routes; rt; rt = rt->next) {
        mrRval r = set_route_to_net(net, rt, newflags, stack, bbox, stage);
        if (r == mrUnroutable || r == mrError)
            result = r;
    }
    return (result);
}


// add_colliding_net
//
// Used by find_colliding() (see below).  Save net "netnum" to the
// list of colliding nets if it is not already in the list.  Return
// true if the list got longer, false otherwise.
//
bool
cMRouter::add_colliding_net(dbNetList **nlptr, u_int netnum)
{
    for (dbNetList *cnl = *nlptr; cnl; cnl = cnl->next) {
        if (cnl->net->netnum == netnum)
            return (false);
    }

    dbNet *fnet = getNetByNum(netnum);
    if (fnet) {
        *nlptr = new dbNetList(fnet, *nlptr);
        return (true);
    }
    return (false);
}


// find_colliding
//
// Find nets that are colliding with the given net "net", and create
// and return a list of them.
//
dbNetList *
cMRouter::find_colliding(dbNet *net, int *ripnum)
{
    dbNetList *nl = 0;
    int rnum = 0;

    // Scan the routed points for recorded collisions.

    for (dbRoute *rt = net->routes; rt; rt = rt->next) {
        for (dbSeg *seg = rt->segments; seg; seg = seg->next) {
            int lay = seg->layer;
            int x = seg->x1;
            int y = seg->y1;

            // The following skips over vias, which is okay, since
            // those positions are covered by segments on both
            // layers or are terminal positions that by definition
            // can't belong to a different net.

            for (;;) {
                u_int orignet = obsVal(x, y, lay) & ROUTED_NET_MASK;

                if (orignet == DRC_BLOCKAGE) {

                    // If original position was a DRC-related
                    // blockage, find out which net or nets would
                    // be in conflict.

                    if (needBlock(lay) & (ROUTEBLOCKX | VIABLOCKX)) {
                        if (x < numChannelsX(lay) - 1) {
                            orignet = obsVal(x + 1, y, lay) &
                                ROUTED_NET_MASK;
                            if (!(orignet & NO_NET)) {
                                orignet &= NETNUM_MASK;
                                if (orignet && (orignet != net->netnum))
                                    rnum += add_colliding_net(&nl, orignet);
                            }
                        }
                        if (x > 0) {
                            orignet = obsVal(x - 1, y, lay) &
                                ROUTED_NET_MASK;
                            if (!(orignet & NO_NET)) {
                                orignet &= NETNUM_MASK;
                                if (orignet && (orignet != net->netnum))
                                    rnum += add_colliding_net(&nl, orignet);
                            }
                        }
                    }
                    if (needBlock(lay) & (ROUTEBLOCKY | VIABLOCKY)) {
                        if (y < numChannelsY(lay) - 1) {
                            orignet = obsVal(x, y + 1, lay) &
                                ROUTED_NET_MASK;
                            if (!(orignet & NO_NET)) {
                                orignet &= NETNUM_MASK;
                                if (orignet && (orignet != net->netnum))
                                    rnum += add_colliding_net(&nl, orignet);
                            }
                        }
                        if (y > 0) {
                            orignet = obsVal(x, y - 1, lay) &
                                ROUTED_NET_MASK;
                            if (!(orignet & NO_NET)) {
                                orignet &= NETNUM_MASK;
                                if (orignet && (orignet != net->netnum))
                                    rnum += add_colliding_net(&nl, orignet);
                            }
                        }
                    }
                }
                else if ((orignet & NETNUM_MASK) != net->netnum)
                    rnum += add_colliding_net(&nl, (orignet & NETNUM_MASK));

                if ((x == seg->x2) && (y == seg->y2))
                    break;

                if (x < seg->x2)
                    x++;
                else if (x > seg->x2)
                    x--;
                if (y < seg->y2)
                    y++;
                else if (y > seg->y2)
                    y--;
            }
        }
    }

    // Diagnostic

    if (nl && (verbose() > 0)) {
        db->emitMesg("Best route of %s collides with nets: ", net->netname);
        for (dbNetList *cnl = nl; cnl; cnl = cnl->next) {
            db->emitMesg("%s ", cnl->net->netname);
        }
        db->emitMesg("\n");
    }

    if (ripnum)
        *ripnum = rnum;
    return (nl);
}


//#define DEBUG_PT

// eval_pt
//
// Evaluate cost to get from given point to current point.  Current
// point is passed in "ept", and the direction from the new point to
// the current point is indicated by "flags".
//
// ONLY consider the cost of the single step itself.
//
// If "stage" is not mrStage1, then this is a second stage routing,
// where we should consider other nets to be a high cost to short to,
// rather than a blockage.  This will allow us to finish the route,
// but with a minimum number of collisions with other nets.  Then, we
// rip up those nets, add them to the "failed" stack, and reroute this
// one.
//
//  Returns true if node needs to be (re)processed, false if not.
//
bool
cMRouter::eval_pt(mrGridP *ept, u_char flags, mrStage stage)
{
    // conflictCost is passed in flags if "force" option is set and
    // this route crosses a prohibited boundary.  This allows the
    // prohibited move but gives it a high cost.

    mrGridP newpt(*ept);
    int thiscost = 0;
    if (flags & PR_CONFLICT) {
        thiscost = conflictCost() * 10;
        flags &= ~PR_CONFLICT;
    }

    switch (flags) {
    case PR_PRED_N:
        newpt.y--;
        break;
    case PR_PRED_S:
        newpt.y++;
        break;
    case PR_PRED_E:
        newpt.x--;
        break;
    case PR_PRED_W:
        newpt.x++;
        break;
    case PR_PRED_U:
        newpt.lay--;
        break;
    case PR_PRED_D:
        newpt.lay++;
        break;
    }

    mrProute *Pr = obs2Val(newpt.x, newpt.y, newpt.lay);
    bool hasnodeinfo = (newpt.lay < (int)pinLayers());
    dbNode *nodesav = hasnodeinfo ? nodeSav(newpt.x, newpt.y, newpt.lay) : 0;
    lefu_t stub = hasnodeinfo ? stubVal(newpt.x, newpt.y, newpt.lay) : 0;

    if (!(Pr->flags & (PR_COST | PR_SOURCE))) {
        // 2nd stage allows routes to cross existing routes
        u_int netnum = Pr->prdata.net;
        if ((stage != mrStage1) && (netnum < maxNetNum())) {
            if (hasnodeinfo && nodesav)
{
#ifdef DEBUG_PT
printf("PT 0 0\n");
#endif
                return (false);         // But cannot route over terminals!
}

            // Is net k in the "noripup" list?  If so, don't route it.

            for (dbNetList *nl = mr_curNet->noripup; nl; nl = nl->next) {
                if (nl->net->netnum == netnum)
{
#ifdef DEBUG_PT
printf("PT 0 1\n");
#endif
                    return (false);
}
            }

            // In case of a collision, we change the grid point to be
            // routable but flag it as a point of collision so we can
            // later see what were the net numbers of the interfering
            // routes by cross-referencing the Obs[][] array.

            Pr->flags |= (PR_CONFLICT | PR_COST);
            Pr->prdata.cost = MAXRT;
            thiscost += conflictCost();
        }
        else if ((stage != mrStage1) && (netnum == DRC_BLOCKAGE)) {
            if (hasnodeinfo && nodesav)
{
#ifdef DEBUG_PT
printf("PT 0 2\n");
#endif
                return (false);         // But cannot route over terminals!
}

            // Position does not contain the net number, so we have to
            // go looking for it.  Fortunately this is a fairly rare
            // occurrance.  But it is necessary to find all
            // neighboring nets that might have created the blockage,
            // and refuse to route here if any of them are on the
            // noripup list.

            if (needBlock(newpt.lay) & (ROUTEBLOCKX | VIABLOCKX)) {
                if (newpt.x < numChannelsX(newpt.lay) - 1) {
                    netnum = obsVal(newpt.x + 1, newpt.y, newpt.lay) &
                        ROUTED_NET_MASK;
                    if (!(netnum & NO_NET)) {
                        netnum &= NETNUM_MASK;
                        if ((netnum != 0) && (netnum != mr_curNet->netnum)) {
                            // Is net k in the "noripup" list?  If so,
                            // don't route it.

                            for (dbNetList *nl = mr_curNet->noripup; nl;
                                    nl = nl->next) {
                                if (nl->net->netnum == netnum)
{
#ifdef DEBUG_PT
printf("PT 0 3\n");
#endif
                                    return (false);
}
                            }
                        }
                    }
                }

                if (newpt.x > 0) {
                    netnum = obsVal(newpt.x - 1, newpt.y, newpt.lay) &
                        ROUTED_NET_MASK;
                    if (!(netnum & NO_NET)) {
                        netnum &= NETNUM_MASK;
                        if ((netnum != 0) && (netnum != mr_curNet->netnum)) {
                            // Is net k in the "noripup" list?  If so,
                            // don't route it.

                            for (dbNetList *nl = mr_curNet->noripup; nl;
                                    nl = nl->next) {
                                if (nl->net->netnum == netnum)
{
#ifdef DEBUG_PT
printf("PT 0 4\n");
#endif
                                    return (false);
}
                            }
                        }
                    }
                }
            }  
            if (needBlock(newpt.lay) & (ROUTEBLOCKY | VIABLOCKY)) {
                if (newpt.y < numChannelsY(newpt.lay) - 1) {
                    netnum = obsVal(newpt.x, newpt.y + 1, newpt.lay) &
                        ROUTED_NET_MASK;
                    if (!(netnum & NO_NET)) {
                        netnum &= NETNUM_MASK;
                        if ((netnum != 0) && (netnum != mr_curNet->netnum)) {
                            // Is net k in the "noripup" list?  If so,
                            // don't route it.

                            for (dbNetList *nl = mr_curNet->noripup; nl;
                                    nl = nl->next) {
                                if (nl->net->netnum == netnum)
{
#ifdef DEBUG_PT
printf("PT 0 5\n");
#endif
                                    return (false);
}
                            }
                        }
                    }
                }

                if (newpt.y > 0) {
                    netnum = obsVal(newpt.x, newpt.y - 1, newpt.lay) &
                        ROUTED_NET_MASK;
                    if (!(netnum & NO_NET)) {
                        netnum &= NETNUM_MASK;
                        if ((netnum != 0) && (netnum != mr_curNet->netnum)) {
                            // Is net k in the "noripup" list?  If so,
                            // don't route it.

                            for (dbNetList *nl = mr_curNet->noripup; nl;
                                    nl = nl->next) {
                                if (nl->net->netnum == netnum)
{
#ifdef DEBUG_PT
printf("PT 0 6\n");
#endif
                                    return (false);
}
                            }
                        }
                    }
                }
            }

            // In case of a collision, we change the grid point to be
            // routable but flag it as a point of collision so we can
            // later see what were the net numbers of the interfering
            // routes by cross-referencing the Obs[][] array.

            Pr->flags |= (PR_CONFLICT | PR_COST);
            Pr->prdata.cost = MAXRT;
            thiscost += conflictCost();
        }
        else
{
#ifdef DEBUG_PT
printf("PT 0 7\n");
#endif
            return (false);         // Position is not routeable.
}
    }

    // Compute the cost to step from the current point to the new
    // point.  blockCost is used if the node has only one point to
    // connect to, so that routing over it could block it entirely.

    if ((newpt.lay > 0) && hasnodeinfo) {
        dbNode *node = nodeLoc(newpt.x, newpt.y, newpt.lay - 1);
        if (node) {
            mrProute *Pt = obs2Val(newpt.x, newpt.y, newpt.lay - 1);
            if (!(Pt->flags & PR_TARGET) && !(Pt->flags & PR_SOURCE)) {
                if (node->taps && !node->taps->next)
                    thiscost += blockCost(); // Cost to block out a tap.
                else if (!node->taps) {
                    if (node->extend) {
                        if (!node->extend->next) {
                            // Node has only one extended access
                            // point:  Try very hard to avoid routing
                            // over it

                            thiscost += 10 * blockCost();
                        }
                        else
                            thiscost += blockCost();
                    }
                    // If both node->taps and node->extend are NULL,
                    // then the node has no access and will never be
                    // routed, so don't bother costing it.

                }
                else
                    thiscost += xverCost();    // Cross-under cost.
            }
        }
    }
    if (((newpt.lay + 1) < (int)pinLayers()) &&
            (newpt.lay < (int)numLayers() - 1)) {
        dbNode *node = nodeLoc(newpt.x, newpt.y, newpt.lay + 1);
        if (node) {
            mrProute *Pt = obs2Val(newpt.x, newpt.y, newpt.lay + 1);
            if (!(Pt->flags & PR_TARGET) && !(Pt->flags & PR_SOURCE)) {
                if (node->taps && !node->taps->next)
                    thiscost += blockCost(); // Cost to block out a tap.
                else
                    thiscost += xverCost();    // Cross-over cost.
            }
        }
    }
    if (ept->lay != newpt.lay)
        thiscost += viaCost();
    if (ept->x != newpt.x)
        thiscost += (vert(newpt.lay)*jogCost() +
            (1 - vert(newpt.lay))*segCost());
    if (ept->y != newpt.y)
        thiscost += (vert(newpt.lay)*segCost() +
            (1 - vert(newpt.lay))*jogCost());

    // Add the cost to the cost of the original position.
    thiscost += ept->cost;

    // Routes that reach nodes are given a cost based on the "quality"
    // of the node location:  higher cost given to stub routes and
    // offset positions.

    if (hasnodeinfo)
        thiscost += abs(stub) * offsetCost();
   
    // Replace node information if cost is minimum.

    if (Pr->flags & PR_CONFLICT)
        thiscost += conflictCost();      // For 2nd stage routes.

    if (thiscost < (int)Pr->prdata.cost) {
#ifdef DEBUG_PT
printf("PT 1 %d %d\n", thiscost, Pr->prdata.cost);
#endif
        Pr->flags &= ~PR_PRED_DMASK;
        Pr->flags |= flags;
        Pr->prdata.cost = thiscost;
        Pr->flags &= ~PR_PROCESSED;      // Need to reprocess this node.

        if (verbose() > 3) {
            db->emitMesg("New cost %d at (%d %d %d)\n", thiscost,
            newpt.x, newpt.y, newpt.lay);
        }
        return (true);
    }
#ifdef DEBUG_PT
printf("PT 0 8 %d %d\n", thiscost, Pr->prdata.cost);
#endif
    return (false);     // New position did not get a lower cost.
}


//#define DEBUG_WB

// writeback_segment
//
// Copy information from a single segment back the Obs[] array.
//
// NOTE:  needBlock is used to handle cases where the existence of a
// route prohibits any routing on adjacent tracks on the same plane
// due to DRC restrictions (i.e., metal is too wide for single track
// spacing).  Be sure to check the value of adjacent positions in Obs
// against the mask NETNUM_MASK, because NETNUM_MASK includes NO_NET. 
// By replacing only empty and routable positions with the unique flag
// combination DRC_BLOCKAGE, it is possible to detect and remove the
// same if that net is ripped up, without touching any position
// originally marked NO_NET.
//
// Another NOTE:  Tap offsets can cause the position in front of the
// offset to be unroutable.  So if the segment is on a tap offset,
// mark the position in front as unroutable.  If the segment neighbors
// an offset tap, then mark the tap unroutable.
//
void
cMRouter::writeback_segment(dbSeg *seg, int netnum)
{
    mrGridCell c;
    if (seg->segtype == ST_VIA) {
        // Preserve blocking information.
        initGridCell(c, seg->x1, seg->y1, seg->layer + 1);
        int dir = obsVal(c) & BLOCKED_MASK;
        setObsVal(c, (netnum | dir));
#ifdef DEBUG_WB
printf("WBa %d %d %d %x\n", seg->x1, seg->y1, seg->layer+1, (netnum | dir));
#endif
        if (needBlock(seg->layer + 1) & VIABLOCKX) {
            initGridCell(c, seg->x1 + 1, seg->y1, seg->layer + 1);
            if ((seg->x1 < (numChannelsX(seg->layer + 1) - 1)) &&
                    (obsVal(c) & NETNUM_MASK) == 0)
{
#ifdef DEBUG_WB
printf("WBb\n");
#endif
                setObsVal(c, DRC_BLOCKAGE);
}
            initGridCell(c, seg->x1 - 1, seg->y1, seg->layer + 1);
            if ((seg->x1 > 0) && (obsVal(c) & NETNUM_MASK) == 0)
{
#ifdef DEBUG_WB
printf("WBc\n");
#endif
                setObsVal(c, DRC_BLOCKAGE);
}
        }
        if (needBlock(seg->layer + 1) & VIABLOCKY) {
            initGridCell(c, seg->x1, seg->y1 + 1, seg->layer + 1);
            if ((seg->y1 < (numChannelsY(seg->layer + 1) - 1)) &&
                    (obsVal(c) & NETNUM_MASK) == 0)
{
#ifdef DEBUG_WB
printf("WBd\n");
#endif
                setObsVal(c, DRC_BLOCKAGE);
}
            initGridCell(c, seg->x1, seg->y1 - 1, seg->layer + 1);
            if ((seg->y1 > 0) && (obsVal(c) & NETNUM_MASK) == 0)
{
#ifdef DEBUG_WB
printf("WBe\n");
#endif
                setObsVal(c, DRC_BLOCKAGE);
}
        }

        // If position itself is an offset route, then make the route
        // position on the forward side of the offset unroutable, on
        // both via layers.  (Like the above code, there is no check
        // for whether the offset distance is enough to cause a DRC
        // violation.)

        initGridCell(c, seg->x1, seg->y1, seg->layer);
        u_int sobs = obsVal(c);
        if (sobs & OFFSET_TAP) {
            lefu_t dist = offsetVal(c);
            u_int flg = flagsVal(c);
            if (flg & NI_OFFSET_EW) {
                if ((dist > 0) && (seg->x1 < (numChannelsX(seg->layer) - 1))) {
                    initGridCell(c, seg->x1 + 1, seg->y1, seg->layer);
                    setObsVal(c, (obsVal(c) | DRC_BLOCKAGE));
#ifdef DEBUG_WB
printf("WB1 %x\n", obsVal(c));
#endif
                    initGridCell(c, seg->x1 + 1, seg->y1, seg->layer + 1);
                    setObsVal(c, (obsVal(c) | DRC_BLOCKAGE));
#ifdef DEBUG_WB
printf("WB2 %x\n", obsVal(c));
#endif
                }
                if ((dist < 0) && (seg->x1 > 0)) {
                    initGridCell(c, seg->x1 - 1, seg->y1, seg->layer);
                    setObsVal(c, (obsVal(c) | DRC_BLOCKAGE));
#ifdef DEBUG_WB
printf("WB3 %x\n", obsVal(c));
#endif
                    initGridCell(c, seg->x1 - 1, seg->y1, seg->layer + 1);
                    setObsVal(c, (obsVal(c) | DRC_BLOCKAGE));
#ifdef DEBUG_WB
printf("WB4 %x\n", obsVal(c));
#endif
                }
            }
            else if (flg & NI_OFFSET_NS) {
                if ((dist > 0) && (seg->y1 < (numChannelsY(seg->layer) - 1))) {
                    initGridCell(c, seg->x1, seg->y1 + 1, seg->layer);
                    setObsVal(c, (obsVal(c) | DRC_BLOCKAGE));
#ifdef DEBUG_WB
printf("WB5 %x\n", obsVal(c));
#endif
                    initGridCell(c, seg->x1, seg->y1 + 1, seg->layer + 1);
                    setObsVal(c, (obsVal(c) | DRC_BLOCKAGE));
#ifdef DEBUG_WB
printf("WB6 %x\n", obsVal(c));
#endif
                }
                if ((dist < 0) && (seg->y1 > 0)) {
                    initGridCell(c, seg->x1, seg->y1 - 1, seg->layer);
                    setObsVal(c, (obsVal(c) | DRC_BLOCKAGE));
#ifdef DEBUG_WB
printf("WB7 %x\n", obsVal(c));
#endif
                    initGridCell(c, seg->x1, seg->y1 - 1, seg->layer + 1);
                    setObsVal(c, (obsVal(c) | DRC_BLOCKAGE));
#ifdef DEBUG_WB
printf("WB8 %x\n", obsVal(c));
#endif
                }
            }
        }
    }

    for (int i = seg->x1; ; i += (seg->x2 > seg->x1) ? 1 : -1) {
        initGridCell(c, i, seg->y1, seg->layer);
        int dir = obsVal(c) & BLOCKED_MASK;
        setObsVal(c, netnum | dir);
#ifdef DEBUG_WB
printf("WBi %d %d %d %x\n", i, seg->y1, seg->layer, (netnum | dir));
#endif
        if (needBlock(seg->layer) & ROUTEBLOCKY) {
            initGridCell(c, i, seg->y1 + 1, seg->layer);
            if ((seg->y1 < (numChannelsY(seg->layer) - 1)) &&
                    (obsVal(c) & NETNUM_MASK) == 0)
{
#ifdef DEBUG_WB
printf("WBi1\n");
#endif
                setObsVal(c, DRC_BLOCKAGE);
}
            initGridCell(c, i, seg->y1 - 1, seg->layer);
            if ((seg->y1 > 0) && (obsVal(c) & NETNUM_MASK) == 0)
{
#ifdef DEBUG_WB
printf("WBi2\n");
#endif
                setObsVal(c, DRC_BLOCKAGE);
}
        }

        // Check position on each side for an offset tap on a
        // different net, and mark the position unroutable.
        //
        // NOTE:  This is a bit conservative, as it will block
        // positions next to an offset tap without checking if the
        // offset distance is enough to cause a DRC error.  Could be
        // refined.  .  .

        int layer = (seg->layer == 0) ? 0 : seg->layer - 1;
        if (seg->y1 < (numChannelsY(layer) - 1)) {
            initGridCell(c, i, seg->y1 + 1, layer);
            u_int sobs = obsVal(c);
            if ((sobs & OFFSET_TAP) && !(sobs & ROUTED_NET)) {
                if (flagsVal(c) & NI_OFFSET_NS) {
                    lefu_t dist = offsetVal(c);
                    if (dist < 0) {
#ifdef DEBUG_WB
printf("WBi3\n");
#endif
                        setObsVal(c, (obsVal(c) | DRC_BLOCKAGE));
                    }
                }
            }
        }
        if (seg->y1 > 0) {
            initGridCell(c, i, seg->y1 - 1, layer);
            u_int sobs = obsVal(c);
            if ((sobs & OFFSET_TAP) && !(sobs & ROUTED_NET)) {
                if (flagsVal(c) & NI_OFFSET_NS) {
                    lefu_t dist = offsetVal(c);
                    if (dist > 0) {
#ifdef DEBUG_WB
printf("WBi4\n");
#endif
                        setObsVal(c, (obsVal(c) | DRC_BLOCKAGE));
                    }
                }
            }
        }
        if (i == seg->x2)
            break;
    }
    for (int i = seg->y1; ; i += (seg->y2 > seg->y1) ? 1 : -1) {
        initGridCell(c, seg->x1, i, seg->layer);
        int dir = obsVal(c) & BLOCKED_MASK;
        setObsVal(c, netnum | dir);
#ifdef DEBUG_WB
printf("WBj %d %d %d %x\n", seg->x1, i, seg->layer, (netnum | dir));
#endif
        if (needBlock(seg->layer) & ROUTEBLOCKX) {
            initGridCell(c, seg->x1 + 1, i, seg->layer);
            if ((seg->x1 < (numChannelsX(seg->layer) - 1)) &&
                    (obsVal(c) & NETNUM_MASK) == 0)
{
#ifdef DEBUG_WB
printf("WBj1\n");
#endif
                setObsVal(c, DRC_BLOCKAGE);
}
            initGridCell(c, seg->x1 - 1, i, seg->layer);
            if ((seg->x1 > 0) && (obsVal(c) & NETNUM_MASK) == 0)
{
#ifdef DEBUG_WB
printf("WBj2\n");
#endif
                setObsVal(c, DRC_BLOCKAGE);
}
        }

        // Check position on each side for an offset tap on a
        // different net, and mark the position unroutable (see
        // above).

        int layer = (seg->layer == 0) ? 0 : seg->layer - 1;
        if (seg->x1 < (numChannelsX(layer) - 1)) {
            initGridCell(c, seg->x1 + 1, i, layer);
            u_int sobs = obsVal(c);
            if ((sobs & OFFSET_TAP) && !(sobs & ROUTED_NET)) {
                if (flagsVal(c) & NI_OFFSET_EW) {
                    lefu_t dist = offsetVal(c);
                    if (dist < 0) {
#ifdef DEBUG_WB
printf("WBj3\n");
#endif
                        setObsVal(c, (obsVal(c) | DRC_BLOCKAGE));
                    }
                }
            }
        }
        if (seg->x1 > 0) {
            initGridCell(c, seg->x1 - 1, i, layer);
            u_int sobs = obsVal(c);
            if ((sobs & OFFSET_TAP) && !(sobs & ROUTED_NET)) {
                if (flagsVal(c) & NI_OFFSET_EW) {
                    lefu_t dist = offsetVal(c);
                    if (dist > 0) {
#ifdef DEBUG_WB
printf("WBj4\n");
#endif
                        setObsVal(c, (obsVal(c) | DRC_BLOCKAGE));
                    }
                }
            }
        }
        if (i == seg->y2)
            break;
    }
}


//#define DEBUG_PR

// commit_proute
//
// Turn the potential route into an actual route by generating the
// route segments.
//
// ARGS:
// route structure to fill; "stage" is not mrStage1 if we're on second
// stage routing, in which case we fill the route structure but don't
// modify the Obs array.
//
// RETURNS:
// mrSuccess on success, mrProvisional on stacked via failure, and
// mrError on failure to find a terminal.  On a stacked via failure,
// the route is committed anyway.
//
// SIDE EFFECTS:
// Obs update, RT llseg added.
//
mrRval
cMRouter::commit_proute(dbRoute *rt, mrGridP *ept, mrStage stage)
{
    bool first = true;
    int dx = -1, dy = -1;

    if (verbose() > 1) {
        db->flushMesg();
        db->emitMesg("\nCommit: TotalRoutes = %d\n", mr_totalRoutes);
    }

    u_int netnum = rt->netnum;

    mrProute *Pr = obs2Val(ept->x, ept->y, ept->lay);
    if (!(Pr->flags & PR_COST)) {
        db->emitErrMesg(
            "commit_proute(): impossible - terminal is not routable!\n");
        return (mrError);
    }

    // Generate an indexed route, recording the series of predecessors
    // and their positions.

    mrPoint *lrtop = new mrPoint(ept->x, ept->y, ept->lay, 0);
    mrPoint *lrend = lrtop;

#ifdef DEBUG_PR
    db->emitMesg("  (%d %d %d)\n", lrtop->x1, lrtop->y1, lrtop->layer);
#endif

    for (;;) {

        Pr = obs2Val(lrend->x1, lrend->y1, lrend->layer);
        u_int dmask = Pr->flags & PR_PRED_DMASK;
        if (dmask == PR_PRED_NONE)
            break;

        mrPoint *newlr = new mrPoint(lrend->x1, lrend->y1, lrend->layer, 0);
        lrend->next = newlr;

        switch (dmask) {
        case PR_PRED_N:
            (newlr->y1)++;
            break;
        case PR_PRED_S:
            (newlr->y1)--;
            break;
        case PR_PRED_E:
            (newlr->x1)++;
            break;
        case PR_PRED_W:
            (newlr->x1)--;
            break;
        case PR_PRED_U:
            (newlr->layer)++;
            break;
        case PR_PRED_D:
            (newlr->layer)--;
            break;
        }
        lrend = newlr;

#ifdef DEBUG_PR
        db->emitMesg("  (%d %d %d) %x\n", newlr->x1, newlr->y1,
            newlr->layer, dmask);
#endif
    }
    lrend = lrtop;

    // TEST:  Walk through the solution, and look for stacked vias. 
    // When found, look for an alternative path that avoids the stack.

    if (stackedVias() < ((int)numLayers() - 1)) {
        int minx = -1, miny = -1;

        int stacks = 1;
        while (stacks != 0) {
            // Keep doing until all illegal stacks are gone.
            stacks = 0;
            mrPoint *lrcur = lrend;
            mrPoint *lrprev = lrend->next;

            while (lrprev) {
                mrPoint *lrppre = lrprev->next;
                if (!lrppre)
                    break;
                int stackheight = 0;
                mrPoint *a = lrcur;
                mrPoint *b = lrprev;
                while (a->layer != b->layer) {
                    stackheight++;
                    a = b;
                    b = a->next;
                    if (!b)
                        break;
                }
                bool collide = false;
                while (stackheight > (int)stackedVias()) {
                    // Illegal stack found.
                    stacks++;

                    // Try to move the second contact in the path.
                    int cx = lrprev->x1;
                    int cy = lrprev->y1;
                    int cl = lrprev->layer;
                    int mincost = MAXRT;
                    int dl = lrppre->layer;

                    // Check all four positions around the contact for
                    // the lowest cost, and make sure the position
                    // below that is available.

                    if (cx < numChannelsX(cl) - 1) {
                        dx = cx + 1;     // Check to the right.
                        mrProute *pri = obs2Val(dx, cy, cl);
                        u_int pflags = pri->flags;
                        int cost = pri->prdata.cost;
                        if (collide && !(pflags & (PR_COST | PR_SOURCE)) &&
                                (pri->prdata.net < maxNetNum())) {
                            pflags = 0;
                            cost = conflictCost();
                        }
                        if (pflags & PR_COST) {
                            pflags &= ~PR_COST;
                            if ((pflags & PR_PRED_DMASK) != PR_PRED_NONE &&
                                    cost < mincost) {
                                mrProute *pri2 = obs2Val(dx, cy, dl);
                                u_int p2flags = pri2->flags;
                                if (p2flags & PR_COST) {
                                    p2flags &= ~PR_COST;
                                    if ((p2flags & PR_PRED_DMASK) !=
                                            PR_PRED_NONE &&
                                            pri2->prdata.cost < MAXRT) {
                                        mincost = cost;
                                        minx = dx;
                                        miny = cy;
                                    }
                                }
                                else if (collide &&
                                        !(p2flags & (PR_COST | PR_SOURCE)) &&
                                        (pri2->prdata.net < maxNetNum()) &&
                                        ((cost + conflictCost()) < mincost)) {
                                    mincost = cost + conflictCost();
                                    minx = dx;
                                    miny = dy;
                                }
                            }
                        }
                    }

                    if (cx > 0) {
                        dx = cx - 1;     // Check to the left.
                        mrProute *pri = obs2Val(dx, cy, cl);
                        u_int pflags = pri->flags;
                        int cost = pri->prdata.cost;
                        if (collide && !(pflags & (PR_COST | PR_SOURCE)) &&
                                (pri->prdata.net < maxNetNum())) {
                            pflags = 0;
                            cost = conflictCost();
                        }
                        if (pflags & PR_COST) {
                            pflags &= ~PR_COST;
                            if ((pflags & PR_PRED_DMASK) != PR_PRED_NONE &&
                                    cost < mincost) {
                                mrProute *pri2 = obs2Val(dx, cy, dl);
                                u_int p2flags = pri2->flags;
                                if (p2flags & PR_COST) {
                                    p2flags &= ~PR_COST;
                                    if ((p2flags & PR_PRED_DMASK) !=
                                            PR_PRED_NONE &&
                                            pri2->prdata.cost < MAXRT) {
                                        mincost = cost;
                                        minx = dx;
                                        miny = cy;
                                    }
                                }
                                else if (collide &&
                                        !(p2flags & (PR_COST | PR_SOURCE)) &&
                                        (pri2->prdata.net < maxNetNum()) &&
                                        ((cost + conflictCost()) < mincost)) {
                                    mincost = cost + conflictCost();
                                    minx = dx;
                                    miny = dy;
                                }
                            }
                        }
                    }

                    if (cy < numChannelsY(cl) - 1) {
                        dy = cy + 1;     // Check north.
                        mrProute *pri = obs2Val(cx, dy, cl);
                        u_int pflags = pri->flags;
                        int cost = pri->prdata.cost;
                        if (collide && !(pflags & (PR_COST | PR_SOURCE)) &&
                                (pri->prdata.net < maxNetNum())) {
                            pflags = 0;
                            cost = conflictCost();
                        }
                        if (pflags & PR_COST) {
                            pflags &= ~PR_COST;
                            if ((pflags & PR_PRED_DMASK) != PR_PRED_NONE &&
                                    cost < mincost) {
                                mrProute *pri2 = obs2Val(cx, dy, dl);
                                u_int p2flags = pri2->flags;
                                if (p2flags & PR_COST) {
                                    p2flags &= ~PR_COST;
                                    if ((p2flags & PR_PRED_DMASK) !=
                                            PR_PRED_NONE &&
                                            pri2->prdata.cost < MAXRT) {
                                        mincost = cost;
                                        minx = cx;
                                        miny = dy;
                                    }
                                }
                                else if (collide &&
                                        !(p2flags & (PR_COST | PR_SOURCE)) &&
                                        (pri2->prdata.net < maxNetNum()) &&
                                        ((cost + conflictCost()) < mincost)) {
                                    mincost = cost + conflictCost();
                                    minx = dx;
                                    miny = dy;
                                }
                            }
                        }
                    }

                    if (cy > 0) {
                        dy = cy - 1;     // Check south.
                        mrProute *pri = obs2Val(cx, dy, cl);
                        u_int pflags = pri->flags;
                        int cost = pri->prdata.cost;
                        if (collide && !(pflags & (PR_COST | PR_SOURCE)) &&
                                (pri->prdata.net < maxNetNum())) {
                            pflags = 0;
                            cost = conflictCost();
                        }
                        if (pflags & PR_COST) {
                            pflags &= ~PR_COST;
                            if ((pflags & PR_PRED_DMASK) != PR_PRED_NONE &&
                                    cost < mincost) {
                                mrProute *pri2 = obs2Val(cx, dy, dl);
                                u_int p2flags = pri2->flags;
                                if (p2flags & PR_COST) {
                                    p2flags &= ~PR_COST;
                                    if ((p2flags & PR_PRED_DMASK) !=
                                            PR_PRED_NONE &&
                                            pri2->prdata.cost < MAXRT) {
                                        mincost = cost;
                                        minx = cx;
                                        miny = dy;
                                    }
                                }
                                else if (collide &&
                                        !(p2flags & (PR_COST | PR_SOURCE)) &&
                                        (pri2->prdata.net < maxNetNum()) &&
                                        ((cost + conflictCost()) < mincost)) {
                                    mincost = cost + conflictCost();
                                    minx = dx;
                                    miny = dy;
                                }
                            }
                        }
                    }

                    // Was there an available route?  If so, modify
                    // records to route through this alternate path. 
                    // If not, then try to move the first contact
                    // instead.

                    if (mincost < MAXRT) {
                        mrPoint *newlr = new mrPoint(minx, miny, cl, 0);
                        mrPoint *newlr2 = new mrPoint(minx, miny, dl, 0);
                        lrprev->next = newlr;
                        newlr->next = newlr2;

                        // Check if point at pri2 is equal to position
                        // of lrppre->next.  If so, bypass lrppre.

                        mrPoint *lrnext = lrppre->next;
                        if (lrnext) {
                            if (lrnext->x1 == minx && lrnext->y1 == miny &&
                                    lrnext->layer == dl) {
                                newlr->next = lrnext;
                                delete lrppre;
                                delete newlr2;
                                lrppre = lrnext;        // ?
                            }
                            else
                                newlr2->next = lrppre;
                        }
                        else
                            newlr2->next = lrppre;

                        break;        // Found a solution;  we're done.
                    }
                    else {

                        // If we couldn't offset lrprev position, then
                        // try offsetting lrcur.

                        cx = lrcur->x1;
                        cy = lrcur->y1;
                        cl = lrcur->layer;
                        mincost = MAXRT;
                        dl = lrprev->layer;

                        if (cx < numChannelsX(cl) - 1) {
                            dx = cx + 1;  // Check to the right.
                            mrProute *pri = obs2Val(dx, cy, cl);
                            u_int pflags = pri->flags;
                            if (pflags & PR_COST) {
                                pflags &= ~PR_COST;
                                if ((pflags & PR_PRED_DMASK) != PR_PRED_NONE &&
                                        pri->prdata.cost < (u_int)mincost) {
                                    mrProute *pri2 = obs2Val(dx, cy, dl);
                                    u_int p2flags = pri2->flags;
                                    if (p2flags & PR_COST) {
                                        p2flags &= ~PR_COST;
                                        if ((p2flags & PR_PRED_DMASK) !=
                                                PR_PRED_NONE &&
                                                pri2->prdata.cost < MAXRT) {
                                            mincost = pri->prdata.cost;
                                            minx = dx;
                                            miny = cy;
                                        }
                                    }
                                }
                            }
                        }

                        if (cx > 0) {
                            dx = cx - 1;  // Check to the left.
                            mrProute *pri = obs2Val(dx, cy, cl);
                            u_int pflags = pri->flags;
                            if (pflags & PR_COST) {
                                pflags &= ~PR_COST;
                                if ((pflags & PR_PRED_DMASK) != PR_PRED_NONE &&
                                        pri->prdata.cost < (u_int)mincost) {
                                    mrProute *pri2 = obs2Val(dx, cy, dl);
                                    u_int p2flags = pri2->flags;
                                    if (p2flags & PR_COST) {
                                        p2flags &= ~PR_COST;
                                        if ((p2flags & PR_PRED_DMASK) !=
                                                PR_PRED_NONE &&
                                                pri2->prdata.cost < MAXRT) {
                                            mincost = pri->prdata.cost;
                                            minx = dx;
                                            miny = cy;
                                        }
                                    }
                                }
                            }
                        }

                        if (cy < numChannelsY(cl) - 1) {
                            dy = cy + 1;  // Check north.
                            mrProute *pri = obs2Val(cx, dy, cl);
                            u_int pflags = pri->flags;
                            if (pflags & PR_COST) {
                                pflags &= ~PR_COST;
                                if ((pflags & PR_PRED_DMASK) != PR_PRED_NONE &&
                                        pri->prdata.cost < (u_int)mincost) {
                                    mrProute *pri2 = obs2Val(cx, dy, dl);
                                    u_int p2flags = pri2->flags;
                                    if (p2flags & PR_COST) {
                                        p2flags &= ~PR_COST;
                                        if ((p2flags & PR_PRED_DMASK) !=
                                                PR_PRED_NONE &&
                                                pri2->prdata.cost < MAXRT) {
                                            mincost = pri->prdata.cost;
                                            minx = cx;
                                            miny = dy;
                                        }
                                    }
                                }
                            }
                        }

                        if (cy > 0) {
                            dy = cy - 1;  // Check south.
                            mrProute *pri = obs2Val(cx, dy, cl);
                            u_int pflags = pri->flags;
                            if (pflags & PR_COST) {
                                pflags &= ~PR_COST;
                                if ((pflags & PR_PRED_DMASK) != PR_PRED_NONE &&
                                        pri->prdata.cost < (u_int)mincost) {
                                    mrProute *pri2 = obs2Val(cx, dy, dl);
                                    u_int p2flags = pri2->flags;
                                    if (p2flags & PR_COST) {
                                        p2flags &= ~PR_COST;
                                        if ((p2flags & PR_PRED_DMASK) !=
                                                PR_PRED_NONE &&
                                                pri2->prdata.cost < MAXRT) {
                                            mincost = pri->prdata.cost;
                                            minx = cx;
                                            miny = dy;
                                        }
                                    }
                                }
                            }
                        }

                        if (mincost < MAXRT) {
                            mrPoint *newlr = new mrPoint(minx, miny, cl, 0);
                            mrPoint *newlr2 = new mrPoint(minx, miny, dl, 0);

                            // If newlr is a source or target, then
                            // make it the endpoint, because we have
                            // just moved the endpoint along the
                            // source or target, and the original
                            // endpoint position is not needed.

                            mrProute *pri = obs2Val(minx, miny, cl);
                            mrProute *pri2 = obs2Val(lrcur->x1, lrcur->y1,
                                lrcur->layer);
                            if ((((pri->flags & PR_SOURCE) &&
                                    (pri2->flags & PR_SOURCE)) ||
                                     ((pri->flags & PR_TARGET) &&
                                     (pri2->flags & PR_TARGET))) &&
                                     (lrcur == lrtop)) {
                                lrtop = newlr;
                                lrend = newlr;
                                delete lrcur;
                                lrcur = newlr;
                            }
                            else
                                lrcur->next = newlr;

                            newlr->next = newlr2;

                            // Check if point at pri2 is equal to
                            // position of lrprev->next.  If so,
                            // bypass lrprev.

                            if (lrppre->x1 == minx && lrppre->y1 == miny &&
                                    lrppre->layer == dl) {
                                newlr->next = lrppre;
                                delete lrprev;
                                delete newlr2;
                                lrprev = lrcur;
                            }
                            else
                                newlr2->next = lrprev;
                
                            break;     // Found a solution;  we're done.
                        }
                        else if (stage == mrStage1) {
                            // On the first stage, we call it an error
                            // and move on to the next net.  This is a
                            // bit conservative, but it works because
                            // failing to remove a stacked via is a
                            // rare occurrance.

                            if (verbose() > 0) {
                                db->emitErrMesg(
                                    "Failed to remove stacked via "
                                    "at grid point %d %d.\n",
                                    lrcur->x1, lrcur->y1);
                            }
                            stacks = 0;
                            lrtop->free();
                            return (mrProvisional);
                        }
                        else {
                            if (collide) {
                                db->emitErrMesg(
                                    "Failed to remove stacked via "
                                    "at grid point %d %d;  position may "
                                    "not be routable.\n",
                                    lrcur->x1, lrcur->y1);
                                stacks = 0;
                                lrtop->free();
                                return (mrProvisional);
                            }

                            // On the second stage, we will run
                            // through the search again, but allow
                            // overwriting other nets, which will be
                            // treated like other colliding nets in
                            // the regular route path search.

                            collide = true;
                        }
                    }
                }
                lrcur = lrprev;
                lrprev = lrppre;
            }
        }
    }
 
    lrend = lrtop;
    mrPoint *lrcur = lrtop;
    mrPoint *lrprev = lrcur->next;
    dbSeg *lseg = 0;

#ifdef DEBUG_PR
    db->emitMesg("new\n");
#endif

    for (;;) {
        dbSeg *seg = new dbSeg;
        seg->next = 0;

        seg->segtype = (lrcur->layer == lrprev->layer) ? ST_WIRE : ST_VIA;

        seg->x1 = lrcur->x1;
        seg->y1 = lrcur->y1;

        seg->layer = LD_MIN(lrcur->layer, lrprev->layer);

        seg->x2 = lrprev->x1;
        seg->y2 = lrprev->y1;

        dx = seg->x2 - seg->x1;
        dy = seg->y2 - seg->y1;

        // segments are in order---place final segment at end of list.
        if (!rt->segments)
            rt->segments = seg;
        else
            lseg->next = seg;

#ifdef DEBUG_PR
        db->emitMesg("  new seg %d %d %d %d   %d %d\n", seg->x1, seg->y1,
            seg->x2, seg->y2, seg->segtype, seg->layer);
#endif

        // Continue processing predecessors as long as the direction
        // is the same, so we get a single long wire segment.  This
        // minimizes the number of segments produced.  Vias have to be
        // handled one at a time, as we make no assumptions about
        // stacked vias.

        if (seg->segtype == ST_WIRE) {
            mrPoint *lrnext;
            while ((lrnext = lrprev->next) != 0) {
                lrnext = lrprev->next;
                if (((lrnext->x1 - lrprev->x1) == dx) &&
                        ((lrnext->y1 - lrprev->y1) == dy) &&
                        (lrnext->layer == lrprev->layer)) {
                    lrcur = lrprev;
                    lrprev = lrnext;
                    seg->x2 = lrprev->x1;
                    seg->y2 = lrprev->y1;
                }
                else
                    break;
            }
        }

        if (verbose() > 3) {
            db->emitMesg("commit: index = %d, net = %d\n",
                Pr->prdata.net, netnum);

            if (seg->segtype == ST_WIRE) {
                db->emitMesg("commit: wire layer %d, (%d,%d) to (%d,%d)\n",
                    seg->layer, seg->x1, seg->y1, seg->x2, seg->y2);
            }
            else {
                db->emitMesg("commit: via %d to %d\n", seg->layer,
                    seg->layer + 1);
            }
            db->flushMesg();
        }

        // Now fill in the Obs structure with this route.

        int lay2 = (seg->segtype & ST_VIA) ? seg->layer + 1 : seg->layer;

        u_int netobs1 = obsVal(seg->x1, seg->y1, seg->layer);
        u_int netobs2 = obsVal(seg->x2, seg->y2, lay2);

        u_int dir1 = netobs1 & PINOBSTRUCTMASK;
        u_int dir2 = netobs2 & PINOBSTRUCTMASK;

        netobs1 &= NETNUM_MASK;
        netobs2 &= NETNUM_MASK;

        netnum |= ROUTED_NET;

        // Write back segment, but not on stage 2 or else the
        // collision information will be lost.  Stage 2 uses
        // writeback_route to call writeback_segment after the
        // colliding nets have been ripped up.

        if (stage == mrStage1)
            writeback_segment(seg, netnum);

        // If Obs shows this position as an obstruction, then this was
        // a port with no taps in reach of a grid point.  This will be
        // dealt with by moving the via off-grid and onto the port
        // position in emit_routes().

        mrGridCell c;
        if (stage == mrStage1) {
            if (first && dir1) {
                first = false;
            }
            else if (first && dir2 && (seg->segtype & ST_VIA) && lrprev &&
                    (lrprev->layer != lay2)) {
                // This also applies to vias at the beginning of a
                // route if the path goes down instead of up (can
                // happen on pins, in particular)

                initGridCell(c, seg->x1, seg->y1, lay2);
                setObsVal(c, (obsVal(c) | dir2));
            }
        }

        // Keep stub information on obstructions that have been routed
        // over, so that in the rip-up stage, we can return them to
        // obstructions.

        initGridCell(c, seg->x1, seg->y1, seg->layer);
        setObsVal(c, (obsVal(c) | dir1));
        initGridCell(c, seg->x2, seg->y2, lay2);
        setObsVal(c, (obsVal(c) | dir2));

        // An offset route end on the previous segment, if it is a
        // via, needs to carry over to this one, if it is a wire
        // route.

        if (lseg && ((lseg->segtype & (ST_VIA | ST_OFFSET_END)) ==
                (ST_VIA | ST_OFFSET_END))) {
            if (seg->segtype != ST_VIA) {
                u_int f1 = seg->layer < (int)pinLayers() ?
                    flagsVal(seg->x1, seg->y1, seg->layer) : 0;
                if (((seg->x1 == seg->x2) && (f1 & NI_OFFSET_NS)) ||
                        ((seg->y1 == seg->y2) && (f1 & NI_OFFSET_EW)))
                    seg->segtype |= ST_OFFSET_START;
            }
        }

        // Check if the route ends are offset.  If so, add flags.  The
        // segment entries are integer grid points, so offsets need to
        // be made when the location is output.  This is only for
        // offsets that are in the same direction as the route, to
        // make sure that the route reaches the offset via, and does
        // not extend past it.

        if (dir1 & OFFSET_TAP) {
            u_int f1 = seg->layer < (int)pinLayers() ?
                flagsVal(seg->x1, seg->y1, seg->layer) : 0;
            if (((seg->x1 == seg->x2) && (f1 & NI_OFFSET_NS)) ||
                    ((seg->y1 == seg->y2) && (f1 & NI_OFFSET_EW)))
                seg->segtype |= ST_OFFSET_START;

            // An offset on a via needs to be applied to the previous
            // route segment as well, if that route is a wire, and the
            // offset is in the same direction as the wire.

            u_int f2 = lay2 < (int)pinLayers() ?
                flagsVal(seg->x2, seg->y2, lay2) : 0;
            if (lseg && (seg->segtype & ST_VIA) && !(lseg->segtype & ST_VIA)) {
                if (((lseg->x1 == lseg->x2) && (f2 & NI_OFFSET_NS)) ||
                        ((lseg->y1 == lseg->y2) && (f2 & NI_OFFSET_EW)))
                    lseg->segtype |= ST_OFFSET_END;
            }
        }

        if (dir2 & OFFSET_TAP)
            seg->segtype |= ST_OFFSET_END;

        lrend = lrcur;            // Save the last route position
        lrend->x1 = lrcur->x1;
        lrend->y1 = lrcur->y1;
        lrend->layer = lrcur->layer;

        lrcur = lrprev;           // Move to the next route position
        lrcur->x1 = seg->x2;
        lrcur->y1 = seg->y2;
        lrprev = lrcur->next;

        if (!lrprev) {

            if (dir2 && (stage == mrStage1)) {
                initGridCell(c, seg->x2, seg->y2, lay2);
                setObsVal(c, (obsVal(c) | dir2));
            }
            else if (dir1 && (seg->segtype & ST_VIA)) {
                // This also applies to vias at the end of a route.
                initGridCell(c, seg->x1, seg->y1, seg->layer);
                setObsVal(c, (obsVal(c) | dir1));
            }

            // Before returning, set *ept to the endpoint position. 
            // This is for diagnostic purposes only.

            ept->x = lrend->x1;
            ept->y = lrend->y1;
            ept->lay = lrend->layer;

            // Clean up allocated memory for the route. . .
            lrtop->free();

            return (mrSuccess);     // Success
        }
        lseg = seg;     // Move to next segment position.
    }

    lrtop->free();
    return (mrProvisional);
}


// writeback_route()
//
// This function is the last part of the function above.  It copies the
// net defined by the segments in the route structure "rt" into the
// Obs array.  This is used only for stage 2, when the writeback is
// not done by commit_proute because we want to rip up nets first, and
// also done prior to routing for any pre-defined net routes.
//
bool
cMRouter::writeback_route(dbRoute *rt)
{
    bool first = true;
    u_int netnum = rt->netnum | ROUTED_NET;
    for (dbSeg *seg = rt->segments; seg; seg = seg->next) {

        // Save stub route information at segment ends.  NOTE:  Where
        // segment end is a via, make sure we are placing the segment
        // end on the right metal layer!

        int lay2 = (seg->segtype & ST_VIA) ? seg->layer + 1 : seg->layer;

        mrGridCell c1, c2;
        initGridCell(c1, seg->x1, seg->y1, seg->layer);
        initGridCell(c2, seg->x2, seg->y2, lay2);
        u_int dir1 = obsVal(c1) & PINOBSTRUCTMASK;
        u_int dir2 = obsVal(c2) & PINOBSTRUCTMASK;

        writeback_segment(seg, netnum);

        if (first) {
            first = false;
            if (dir1)
                setObsVal(c1, (obsVal(c1) | dir1));
            else if (dir2)
                setObsVal(c2, (obsVal(c2) | dir2));
        }
        else if (!seg->next) {
            if (dir1)
                setObsVal(c1, (obsVal(c1) | dir1));
            else if (dir2)
                setObsVal(c2, (obsVal(c2) | dir2));
        }
    }
    return (LD_OK);
}


// writeback_all_routes
//
// Writeback all routes belonging to a net.
//
bool
cMRouter::writeback_all_routes(dbNet *net)
{
    bool result = LD_OK;
    for (dbRoute *rt = net->routes; rt; rt = rt->next) {
        if (writeback_route(rt) != LD_OK)
            result = LD_BAD;
    }
    return (result);
}

