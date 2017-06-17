
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
 $Id: mrouter.cc,v 1.34 2017/02/17 21:37:44 stevew Exp $
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
#include <algorithm>


//
// Maze Router.
//
// Constructor and some basic functions for export.
//

extern "C" {
    // C function which can be found with dlsym, allows
    // load-on-demand of the router package.  Note that a database
    // pointer is required, which can be obtained from the new_lddb
    // function defined in the lddb code.

    cMRif *new_router(cLDDBif *db)
    {
        return (cMRif::newMR(db));
    }
}


#ifdef LD_MEMDBG
int mrPoint::mrpoint_cnt;
int mrPoint::mrpoint_hw;
int mrStack::mrstack_cnt;
int mrStack::mrstack_hw;
#endif


// Static function.
// Export a "constructor" for direct C++ usage.
//
cMRif *
cMRif::newMR(cLDDBif *db)
{
    return (new cMRouter(db));
}


cMRouter::cMRouter(cLDDBif *d) : cLDDBref(d)
{
    mr_layers           = 0;
    mr_nets             = 0;
    mr_rmask            = 0;
    mr_rmaskIncs        = 0;
    mr_curNet           = 0;
    mr_route_gen        = 0;

    mr_totalRoutes      = 0;
    mr_mask             = MASK_AUTO;
    mr_pinLayers        = 0;

    mr_ni_blks          = 0;
    mr_ni_cnt           = 0;

    mr_segCost          = MR_SEGCOST;
    mr_viaCost          = MR_VIACOST;
    mr_jogCost          = MR_JOGCOST;
    mr_xverCost         = MR_XVERCOST;
    mr_blockCost        = MR_BLOCKCOST;
    mr_offsetCost       = MR_OFFSETCOST;
    mr_conflictCost     = MR_CONFLICTCOST;

    mr_numPasses        = MR_PASSES;
    mr_stackedVias      = -1;
    mr_viaPattern       = VIA_PATTERN_NONE;

    mr_stepnet          = -1;
    mr_initialized      = false;
    mr_forceRoutable    = false;
    mr_keepTrying       = 0;
    mr_mapType          = MAP_OBSTRUCT | DRAW_ROUTES;
    mr_ripLimit         = 10;
    mr_rmaskIncsSz      = 0;
    mr_graphics         = 0;

    if (d) {
        // Initialize the maximum nets variable in the database, and
        // set this value as the default.

        d->setMaxNets(MAX_NETNUM);
        d->setDfMaxNets(MAX_NETNUM);
    }
}


cMRouter::~cMRouter()
{
    delete [] mr_layers;
    delete [] mr_nets;
    delete [] mr_rmask;
    delete [] mr_rmaskIncs;
    clear_nodeInfo();
}


// initRouter()
//
// Things to do after a DEF file has been read in, and the size of the
// layout, components, and nets are known, before attempting to route. 
// This is now called from stage1, so no real need to call this
// explicitly.
//
bool
cMRouter::initRouter()
{
    if (!numNets()) {
        db->emitErrMesg("No nets defined, nothing to set up.\n");
        return (LD_BAD);
    }
    if (!numLayers()) {
        db->emitErrMesg("No routing layers defined, nothing to do.\n");
        return (LD_BAD);
    }
    if (numNets() > MAX_NETNUM) {
        db->emitError(
            "Number of nets in design (%d) exceeds maximum (%d).\n",
            numNets(), MAX_NETNUM);
        return (LD_BAD);
    }
    if (mr_initialized)
        return (LD_OK);

    long time0 = db->millisec();

    // Already done when reading DEF, do it again here in case layers
    // were added by other means.
    if (db->setupChannels(true) != LD_OK)
        return (LD_BAD);

    if (!mr_layers)
        mr_layers = new mrLayer[numLayers()];

    // Set the actual value if negative and otherwise fix.
    if (stackedVias() < 0)
        setStackedVias(numLayers());
    else if (stackedVias() == 0)
        setStackedVias(1);
    else if (stackedVias() > (int)numLayers())
        setStackedVias(numLayers());

    create_net_order();  // This allocates mr_nets.

    for (u_int i = 0; i < numNets(); i++) {
        dbNet *net = mr_nets[i];
        find_bounding_box(net);
        define_route_tree(net);
    }

    // If debugging, print a list of node coordinates in a file named
    // "nodes".
    if (debug() & LD_DBG_NODES)
        db->printNodes("nodes");

    if (mr_graphics) {
        if (mr_graphics->recalc_spacing())
            mr_graphics->draw_layout();
    }

    for (u_int i = 0; i < numLayers(); i++) {
        u_int sz = numChannelsX(i) * numChannelsY(i);

        delete [] obsAry(i);
        u_int *obs = new u_int[sz];
        memset(obs, 0, sz*sizeof(u_int));
        setObsAry(i, obs);

        delete [] obs2Ary(i);
        setObs2Ary(i, 0);

        delete [] obsInfoAry(i);
        lefu_t *obsInfo = new lefu_t[sz];
        memset(obsInfo, 0, sz*sizeof(lefu_t));
        setObsInfoAry(i, obsInfo);

        delete [] listedAry(i);
        setListedAry(i, 0);

        delete [] nodeInfoAry(i);
        mrNodeInfo **nodeInfo = new mrNodeInfo*[sz];
        memset(nodeInfo, 0, sz*sizeof(mrNodeInfo*));
        setNodeInfoAry(i, nodeInfo);
    }
    if (!mr_rmask) {
        u_int sz = numChannelsX(0) * numChannelsY(0);
        mr_rmask = new u_char[sz];
        memset(mr_rmask, 0, sz*sizeof(u_char));
    }
    if (!mr_rmaskIncs) {
        // For passes larger than the size, the largest value applies,
        // so setting size and value to 1 sets all increments to 1,
        // as the default.

        mr_rmaskIncsSz = 1;
        mr_rmaskIncs = new u_char[1];
        mr_rmaskIncs[0] = 1;
    }

    db->flushMesg();

    if (verbose() > 1)
        db->emitErrMesg("Diagnostic: memory block is %d bytes\n",
            (int)sizeof(u_int) * numChannelsX(0) * numChannelsY(0));

    // Be sure to create obstructions from gates first, since we don't
    // want improperly defined or positioned obstruction layers to
    // overwrite our node list.

    expand_tap_geometry();
    clip_gate_taps();
    create_obstructions_from_gates();
    create_obstructions_from_list(userObs());
    create_obstructions_from_list(intObs());
    create_obstructions_inside_nodes();
    create_obstructions_outside_nodes();
    tap_to_tap_interactions();
    create_obstructions_from_variable_pitch();
    adjust_stub_lengths();
    find_route_blocks();
    count_reachable_taps();
    count_pinlayers();
   
    if (debug() & LD_DBG_FLGS)
        printFlags("flags1");

    // If any nets are pre-routed, place those routes.

    for (u_int i = 0; i < numNets(); i++) {
        dbNet *net = mr_nets[i];
        writeback_all_routes(net);
    }

    // Remove the Obsinfo array, which is no longer needed, and
    // allocate the Obs2 array for costing information, etc.

    for (u_int i = 0; i < numLayers(); i++) {
        u_int sz = numChannelsX(i) * numChannelsY(i);

        delete obsInfoAry(i);
        setObsInfoAry(i, 0);

        mrProute *obs2 = new mrProute[sz];
        memset(obs2, 0, sz*sizeof(mrProute));
        setObs2Ary(i, obs2);

        bool *lstd = new bool[sz];
        memset(lstd, 0, sz*sizeof(bool));
        setListedAry(i, lstd);
    }

    // Fill in needBlock bit fields, which are used by
    // commit_proute when route layers are too large for the grid
    // size, and grid points around a route need to be marked as
    // blocked whenever something is routed on those layers.

    // "ROUTEBLOCK" is set if the spacing is violated between a normal
    // route and an adjacent via.  "VIABLOCK" is set if the spacing is
    // violated between two adjacent vias.  It may be helpful to define
    // a third category which is route-to-route spacing violation.

    for (u_int i = 0; i < numLayers(); i++) {
        db->setNeedBlock(i, 0);
        lefu_t sreq1 = getRouteSpacing(i);

        lefu_t sreq2 = getViaWidth(i, i, DIR_VERT) + sreq1;
        if (sreq2 > pitchX(i))
            db->setNeedBlock(i, needBlock(i) | VIABLOCKX);
        if (i != 0) {
            sreq2 = getViaWidth(i - 1, i, DIR_VERT) + sreq1;
            if (sreq2 > pitchX(i))
                db->setNeedBlock(i, needBlock(i) | VIABLOCKX);
        }

        sreq2 = getViaWidth(i, i, DIR_HORIZ) + sreq1;
        if (sreq2 > pitchY(i))
            db->setNeedBlock(i, needBlock(i) | VIABLOCKY);
        if (i != 0) {
            sreq2 = getViaWidth(i - 1, i, DIR_HORIZ) + sreq1;
            if (sreq2 > pitchY(i))
                db->setNeedBlock(i, needBlock(i) | VIABLOCKY);
        }

        sreq1 += getRouteWidth(i)/2;

        sreq2 = sreq1 + getViaWidth(i, i, DIR_VERT)/2;
        if (sreq2 > pitchX(i))
            db->setNeedBlock(i, needBlock(i) | ROUTEBLOCKX);
        if (i != 0) {
            sreq2 = sreq1 + getViaWidth(i - 1, i, DIR_VERT)/2;
            if (sreq2 > pitchX(i))
                db->setNeedBlock(i, needBlock(i) | ROUTEBLOCKX);
        }

        sreq2 = sreq1 + getViaWidth(i, i, DIR_HORIZ)/2;
        if (sreq2 > pitchY(i))
            db->setNeedBlock(i, needBlock(i) | ROUTEBLOCKY);
        if (i != 0) {
            sreq2 = sreq1 + getViaWidth(i - 1, i, DIR_HORIZ)/2;
            if (sreq2 > pitchY(i))
                db->setNeedBlock(i, needBlock(i) | ROUTEBLOCKY);
        }
    }

    // Now we have netlist data, and can use it to get a list of nets.

    mr_failedNets.clear();
    db->flushMesg();
    if (verbose() > 0) {
        long elapsed = db->millisec() - time0;
        db->emitMesg("Initialization complete (%g sec).\n"
            "There are %d nets in this design.\n", 1e-3*elapsed, numNets());
    }

    mr_initialized = true;
    return (LD_OK);
}


// resetRouter
//
// Free up memory in preparation for reading another DEF description.
//
void
cMRouter::resetRouter()
{
    db->defReset();
    for (u_int i = 0; i < numLayers(); i++) {
        delete [] obsAry(i);
        setObsAry(i, 0);
        delete [] obs2Ary(i);
        setObs2Ary(i, 0);
        delete [] obsInfoAry(i);
        setObsInfoAry(i, 0);
        delete [] listedAry(i);
        setListedAry(i, 0);
        delete [] nodeInfoAry(i);
        setNodeInfoAry(i, 0);
    }
    clear_nodeInfo();
    delete [] mr_nets;
    mr_nets             = 0;
    delete [] mr_rmask;
    mr_rmask            = 0;
    mr_failedNets.clear();
    mr_segCost          = 1;
    mr_viaCost          = 5;
    mr_jogCost          = 10;
    mr_xverCost         = 4;
    mr_blockCost        = 25;
    mr_offsetCost       = 50;
    mr_conflictCost     = 50;
    mr_initialized      = false;
}


// doRoute
//
// Basic route call.
//
// stage = mrStage1 is normal routing
// stage = mrStage2 is the rip-up and reroute stage
//
// AUTHOR: steve beccue
//
bool
cMRouter::doRoute(dbNet *net, mrStage stage, bool graphdebug)
{
    if (initRouter() == LD_BAD) {
        db->emitErrMesg("doRoute: Error, router initialization failed.\n");
        return (LD_BAD);
    }

    if (!net) {
        db->emitErrMesg("doRoute: Warning, null net, ignored.\n");
        return (LD_OK);
    }

    mr_curNet = net;                // Global, used by 2nd stage.

    // Fill out route information record.
    mrRouteInfo iroute(net);
    mrStack stack;
    iroute.stack = &stack; 

    int lastlayer = -1;

#ifdef LD_MEMDBG
    printf("A %g %6d/%6d %6d/%6d %6d/%6d %6d/%6d\n", db->coresize(),
        mrStack::mrstack_cnt, mrStack::mrstack_hw,
        dbSeg::seg_cnt, dbSeg::seg_hw,
        mrPoint::mrpoint_cnt, mrPoint::mrpoint_hw,
        dbNetList::netlist_cnt, dbNetList::netlist_hw);
#endif

    // Set up Obs2[] matrix for first route.

    u_int unroutable;
    mrRval result = route_setup(&iroute, stage, &unroutable);
    if (graphdebug && mr_graphics)
        mr_graphics->highlight_mask();

    // Keep going until we are unable to route to a terminal.

    // Slightly different logic than Qrouter here, see note below. 
    // Also, might want to try different values for MAXFAILURES, value
    // 1 corresponds to Qrouter.

#define MAXFAILURES 1
    int failures = 0;

    while (net && (result == mrSuccess)) {

        if (graphdebug && mr_graphics) {
            mr_graphics->highlight_source();
            mr_graphics->highlight_dest();
            mr_graphics->highlight_starts(&stack);
        }

        dbRoute *rt1 = new dbRoute;
        rt1->netnum = net->netnum;
        iroute.rt = rt1;

        if (verbose() > 3) {
            if (debug() & LD_DBG_ORDR) {
                // Emit "doroute" as does Qrouter to avoid diffs.
                db->emitMesg("doroute(): added net %d path start %d\n", 
                    net->netnum, net->netnodes->nodenum);
            }
            else {
                db->emitMesg("doRoute: added net %d path start %d\n", 
                    net->netnum, net->netnodes->nodenum);
            }
        }

        result = route_segs(&iroute, stage, graphdebug);

        if (result == mrError || result == mrUnroutable) {
            // Route failure.
            // Break out if we have hit the limit.

            delete rt1;
            if (++failures > MAXFAILURES)
                break;
        }
        else {
            mr_totalRoutes++;

            if (net->routes) {
                dbRoute *lrt = net->routes;
                while (lrt->next)
                    lrt = lrt->next;
                lrt->next = rt1;
            }
            else {
                net->routes = rt1;
            }
            if (mr_graphics)
                mr_graphics->draw_net(net, true, &lastlayer);
        }

        // For power routing, clear the list of existing pending route
        // solutions---they will not be relevant.

        if (iroute.do_pwrbus)
            stack.clear();

        // Set up for next route and check if routing is done.
        result = next_route_setup(&iroute, stage);
    }

    // Finished routing (or error occurred).

    stack.clear();

    // If we have failure to route completely, put the net in the
    // failed list and return BAD.  This is a little different from
    // Qrouter, where it is possible to return OK but with one failed
    // route (and the net in the failed list.

    bool ret;
    if ((result == mrError) || (result == mrUnroutable) ||
            (unroutable > 0) || (failures > 0)) {

        mr_failedNets.push(net);
        ret = LD_BAD;
    }
    else {
        net->realloc_routes();
        ret = LD_OK;
    }

#ifdef LD_MEMDBG
    printf("B %g %6d/%6d %6d/%6d %6d/%6d %6d/%6d\n", db->coresize(),
        mrStack::mrstack_cnt, mrStack::mrstack_hw,
        dbSeg::seg_cnt, dbSeg::seg_hw,
        mrPoint::mrpoint_cnt, mrPoint::mrpoint_hw,
        dbNetList::netlist_cnt, dbNetList::netlist_hw);
#endif

    return (ret);
}


// routeNetRipup
//
// Do a second-stage route (rip-up and re-route) of a single net
// "net".
//
// Return value is 0 for a successful route, or the number of routes
// ripped up otherwise, or -1 if the ripup count would exceed the
// limit.
//
int
cMRouter::routeNetRipup(dbNet *net, bool graphdebug)
{
    // Find the net in the Failed list and remove it.
    mr_failedNets.remove(net);

    bool result = doRoute(net, mrStage2, graphdebug);
    if (result != LD_OK) {
        if (net->noripup) {
            if ((net->flags & NET_PENDING) == 0) {
                // Clear this net's "noripup" list and try again.

                net->noripup->free();
                net->noripup = 0;
                result = doRoute(net, mrStage2, graphdebug);
                net->flags |= NET_PENDING;      // Next time we abandon it.
            }
        }
    }
    if (result == LD_OK)
        return (ripup_colliding(net));

    return (0);
}


// Do first-stage routing, return value is the number of remaining
// unrouted nets.
//
int
cMRouter::doFirstStage(bool graphdebug, int debug_netnum)
{
    if (initRouter() == LD_BAD)
        db->emitErrMesg("Warning, router initialization failed.\n");

    // Clear the lists of failed routes, in case first stage is being
    // called more than once.

    if (debug_netnum <= 0)
        mr_failedNets.clear();

    // Now find and route all the nets.

    long time0 = db->millisec();
    int remaining = numNets();
 
    if (debug() & LD_DBG_FLGS)
        printFlags("flags2");

    for (u_int i = (debug_netnum >= 0) ? debug_netnum : 0; i < numNets();
            i++) {

        dbNet *net = get_net_to_route(i);
        if (net && net->netnodes) {
            if (doRoute(net, mrStage1, graphdebug) == LD_OK) {
                remaining--;
                if (verbose() > 0)
                    db->emitMesg("Finished routing net %s\n", net->netname);
                db->emitMesg("Nets remaining: %d\n", remaining);
            }
            else {
                if (verbose() > 0)
                    db->emitMesg("Failed to route net %s\n", net->netname);
            }
        }
        else {
            if (net && (verbose() > 0)) {
                db->emitMesg("Nothing to do for net %s\n", net->netname);
            }
            remaining--;
        }
        if (debug_netnum >= 0)
            break;
    }
    int failcount = mr_failedNets.num_elements();
    if (debug_netnum >= 0)
        return (failcount);

    if (verbose() > 0) {
        long elapsed = db->millisec() - time0;
        db->flushMesg();
        db->emitMesg("\n----------------------------------------------\n");
        db->emitMesg("Progress: ");
        db->emitMesg("Stage 1 done, %g sec., %d routes completed.\n",
            1e-3*elapsed, mr_totalRoutes);
    }
    if (failcount == 0)
        db->emitMesg("No failed routes!\n");
    else
        db->emitMesg("Failed net routes: %d\n", failcount);
    if (verbose() > 0)
        db->emitMesg("----------------------------------------------\n");

    return (failcount);
}


// doSecondStage
//
// Second stage:  Rip-up and reroute failing nets.
// Method:
// 1) Route a failing net with stage = 1 (other nets become costs, not
//    blockages, no copying to Obs)
// 2) If net continues to fail, flag it as unroutable and remove it
//    from the list.
// 3) Otherwise, determine the nets with which it collided.
// 4) Remove all of the colliding nets, and add them to the mr_failedNets
//    list
// 5) Route the original failing net.
// 6) Continue until all failed nets have been processed.
//
// Return value:  The number of failing nets.
//
int
cMRouter::doSecondStage(bool graphdebug, bool singlestep)
{
    int origcount = mr_failedNets.num_elements();
    long time0 = db->millisec();
    u_int maxtries;
    if (origcount)
        maxtries = mr_totalRoutes + ((origcount < 20) ? 20 : origcount) * 8;
    else
        maxtries = 0;

    fill_mask(0);
    // Abandoned routes---not even trying any more.
    dbNetList *Abandoned = 0;

    int failcount = 0;
    while (!mr_failedNets.is_empty()) {

        // Diagnostic:  how are we doing?
        failcount = mr_failedNets.num_elements();
        if (verbose() > 1)
            db->emitMesg("------------------------------\n");
        db->emitMesg("Nets remaining: %d\n", failcount);
        if (verbose() > 1)
            db->emitMesg("------------------------------\n");

        dbNet *net = mr_failedNets.pop();

        // Keep track of which routes existed before the call to
        // doRoute().
        dbRoute *rt = net->routes;
        if (rt) {
            while (rt->next)
                rt = rt->next;
        }

        if (rt && (net->flags & NET_BULK_ROUTED)) {
            // This is a sanity check, should never get here since the
            // failed net list should never have a net with this flag
            // set.
            db->emitErrMesg(
                "Warning: net %s is in failed list but has bulk allocation.\n",
                net->netname);
            ripupNet(net, true);   // Remove routing information from net.
            rt = 0;
        }

        if (verbose() > 2)
            db->emitMesg("Routing net %s with collisions\n", net->netname);
        db->flushMesg();

        bool result = doRoute(net, mrStage2, graphdebug);

        if (result != LD_OK) {
            if (net->noripup) {
                if ((net->flags & NET_PENDING) == 0) {
                    // Clear this net's "noripup" list and try again.

                    net->noripup->free();
                    net->noripup = 0;
                    result = doRoute(net, mrStage2, graphdebug);
                    net->flags |= NET_PENDING;      // Next time we abandon it.
                }
            }
        }

        if (result == LD_OK) {

            // Find nets that collide with "net" and remove them,
            // adding them to the end of the mr_failedNets list.

            // If the number of nets to be ripped up exceeds
            // "ripLimit", then treat this as a route failure, and
            // don't rip up any of the colliding nets.

            result = (ripup_colliding(net) < 0 ? LD_BAD : LD_OK);
        }

        if (result != LD_OK) {

            // Complete failure to route, even allowing collisions. 
            // Abandon routing this net.

            if (verbose() > 0) {
                db->flushMesg();
                db->emitErrMesg(
                    "----------------------------------------------\n");
                db->emitErrMesg("Complete failure on net %s:  Abandoning.\n",
                    net->netname);
                db->emitErrMesg(
                    "----------------------------------------------\n");
            }

            // Add the net to the "abandoned" list.
            Abandoned = new dbNetList(net, Abandoned);
            mr_failedNets.remove(net);

            // Remove routing information for all new routes that have
            // not been copied back into Obs[].

            if (!rt) {
                rt = net->routes;
                net->routes = 0;
            }
            else {
                dbRoute *rt2 = rt->next;
                rt->next = 0;
                rt = rt2;
            }
            rt->free();

            // Remove both routing information and remove the route
            // from Obs[] for all parts of the net that were
            // previously routed.

            ripupNet(net, true);   // Remove routing information from net.
            continue;
        }

        // Write back the original route to the grid array.
        writeback_all_routes(net);

        // Failsafe---if we have been looping enough times to exceed
        // maxtries (which is set to 8 route attempts per original
        // failed net), then we check progress.  If we have reduced
        // the number of failed nets by half or more, then we have an
        // indication of real progress, and will continue.  If not, we
        // give up.  Qrouter is almost certainly hopelessly stuck at
        // this point.

        if (mr_totalRoutes >= maxtries) {
            if (failcount <= (origcount / 2)) {
                maxtries = mr_totalRoutes + failcount * 8;
                origcount = failcount;
            }
            else if (mr_keepTrying == 0) {
                db->emitErrMesg(
                    "\nRouter is stuck, abandoning remaining routes.\n");
                break;
            }
            else {
                mr_keepTrying--;
                db->emitErrMesg(
                    "\nQrouter is stuck, but I was told to keep trying.\n");
                maxtries = mr_totalRoutes + failcount * 8;
                origcount = failcount;
            }
        }
        if (singlestep && !mr_failedNets.is_empty())
            return (mr_failedNets.num_elements());
    }

    // If the list of abandoned nets is non-null, attach it to the end
    // of the failed nets list.

    if (Abandoned) {
        mr_failedNets.append(Abandoned);
        Abandoned = 0;
    }

    // Blow away the noripup lists, we're done with these, presumably.
    for (u_int i = 0; i < numNets(); i++) {
        dbNet *net = nlNet(i);
        net->noripup->free();
        net->noripup = 0;
        net->flags &= ~NET_PENDING;
    }

    if (verbose() > 0) {
        long elapsed = db->millisec() - time0;
        db->flushMesg();
        db->emitMesg("\n----------------------------------------------\n");
        db->emitMesg("Progress: ");
        db->emitMesg("Stage 2 done, %g sec., %d routes completed.\n",
            1e-3*elapsed, mr_totalRoutes);
    }
    if (mr_failedNets.is_empty()) {
        failcount = 0;
        db->emitMesg("No failed routes!\n");
    }
    else {
        failcount = mr_failedNets.num_elements();
        db->emitMesg("Failed net routes: %d\n", failcount);
    }
    if (verbose() > 0)
        db->emitMesg("----------------------------------------------\n");

    return (failcount);
}


// 3rd stage routing (cleanup).  Rip up each net in turn and reroute
// it.  With all of the crossover costs gone, routes should be much
// better than the 1st stage.  Any route that existed before it got
// ripped up should by definition be routable.
//
// Return the number of unrouted nets remaining.
//
int
cMRouter::doThirdStage(bool graphdebug, int debug_netnum)
{
    // Clear the lists of failed routes, in case first stage is being
    // called more than once.

    long time0 = db->millisec();
    if (debug_netnum <= 0)
        mr_failedNets.clear();

    // Now find and route all the nets.

    int remaining = numNets();
 
    for (u_int i = (debug_netnum >= 0) ? debug_netnum : 0; i < numNets();
            i++) {

        dbNet *net = get_net_to_route(i);
        if (net && net->netnodes) {
            ripupNet(net, false);
            if (doRoute(net, mrStage1, graphdebug) == LD_OK) {
                remaining--;
                if (verbose() > 0)
                    db->emitMesg("Finished routing net %s\n", net->netname);
                db->emitMesg("Nets remaining: %d\n", remaining);
            }
            else {
                if (verbose() > 0)
                    db->emitMesg("Failed to route net %s\n", net->netname);
            }
        }
        else {
            if (net && (verbose() > 0)) {
                db->emitMesg("Nothing to do for net %s\n", net->netname);
            }
            remaining--;
        }
        if (debug_netnum >= 0)
            break;
    }
    int failcount = mr_failedNets.num_elements();
    if (debug_netnum >= 0)
        return (failcount);

    if (verbose() > 0) {
        long elapsed = db->millisec() - time0;
        db->flushMesg();
        db->emitMesg("\n----------------------------------------------\n");
        db->emitMesg("Progress: ");
        db->emitMesg("Stage 3 done, %g sec., %d routes completed.\n",
            1e-3*elapsed, mr_totalRoutes);
    }
    if (!failcount)
        db->emitMesg("No failed routes!\n");
    else
        db->emitMesg("Failed net routes: %d\n", failcount);
    if (verbose() > 0)
        db->emitMesg("----------------------------------------------\n");

    return (failcount);
}


// Print the failed routes on stdout, and clear the failed routes list.
//
void
cMRouter::printClearFailed()
{
    db->emitMesg("----------------------------------------------\n");
    db->emitMesg("Final: ");
    if (mr_failedNets.is_empty())
        db->emitMesg("No failed routes!\n");
    else {
        db->emitMesg("Failed net routes: %d\n", mr_failedNets.num_elements());
        db->emitMesg("List of failed nets follows:\n");

        // Make sure mr_failedNets is cleaned up as we output the
        // failed nets.

        for (dbNetList *nl = mr_failedNets.list(); nl; nl = nl->next) {
            dbNet *net = nl->net;
            db->emitMesg(" %s\n", net->netname);
        }
        db->emitMesg("\n");
        mr_failedNets.clear();
    }
    db->emitMesg("----------------------------------------------\n");
}


namespace {
    // Comparison function for nets.
    //
    bool compNets(const dbNet *p, const dbNet *q)
    {
        // Null nets get shoved up front.
        if (!p)
            return (q != 0);
        if (!q)
            return (false);

        // Sort critical nets at the front by assigned order.

        if (p->flags & NET_CRITICAL) {
            if (q->flags & NET_CRITICAL)
                return (p->netorder < q->netorder);
            return (true);
        }

        // Preserve unsorted order if the node counts are the same.
        if (p->numnodes == q->numnodes)
            return (p->netorder < q->netorder);

        // Otherwise sort inversely by number of nodes.
        return (p->numnodes > q->numnodes);
    }


    // Alternative net comparison.  Sort nets by minimum dimension of
    // the bounding box, and if equal, by the number of nodes in the
    // net.  Bounding box dimensions are ordered smallest to largest,
    // and number of nodes are ordered largest to smallest.
    //
    bool compNetsAlt(const dbNet *p, const dbNet *q)
    {
        // Any null nets get shoved up front.
        if (!p)
            return (q != 0);
        if (!q)
            return (false);

        // Sort critical nets at the front by assigned order.

        if (p->flags & NET_CRITICAL) {
            if (q->flags & NET_CRITICAL)
                return (p->netorder < q->netorder);
            return (true);
        }

        // Otherwise sort as described above.

        int pwidth = p->xmax - p->xmin;
        int pheight = p->ymax - p->ymin;
        int pdim = (pwidth > pheight) ? pheight : pwidth;

        int qwidth = q->xmax - q->xmin;
        int qheight = q->ymax - q->ymin;
        int qdim = (qwidth > qheight) ? qheight : qwidth;

        if (pdim < qdim)
            return (true);
        if (pdim > qdim)
            return (false);

        // Preserve unsorted order if the node counts are the same.
        if (p->numnodes == q->numnodes)
            return (p->netorder < q->netorder);

        return (p->numnodes > q->numnodes);
    }


    // "NoSort" comparison function for nets.  We still do the
    // critical nets in order first, otherwise nets are ordered as
    // they are given.
    //
    bool compNetsNS(const dbNet *p, const dbNet *q)
    {
        // Null nets get shoved up front.
        if (!p)
            return (q != 0);
        if (!q)
            return (false);

        // Sort critical nets at the front by assigned order.

        if (p->flags & NET_CRITICAL) {
            if (q->flags & NET_CRITICAL)
                return (p->netorder < q->netorder);
            return (true);
        }

        // Preserve unsorted order.
        return (p->netorder < q->netorder);
    }
}


// create_net_order
//
// Create an array for the nets, and order them in routing priority.
// Assign indexes to net->netorder.
//
// method = 0
// Nets are ordered simply from those with the most nodes to those
// with the fewest.  However, any nets marked critical in the
// configuration or critical net files will be given precedence.
//
// method = 1
// Nets are ordered by minimum bounding box dimension.  This is based
// on the principle that small or narrow nets have little room to be
// moved around without greatly increasing the net length.  If these
// are put down first, then remaining nets can route around them.
//
void
cMRouter::create_net_order()
{
    // Make a copy of the nets in the database.  The database order is
    // preserved for efficiency in finding a net by number.

    delete [] mr_nets;
    mr_nets = new dbNet*[numNets()];
    for (u_int i = 0; i < numNets(); i++)
        mr_nets[i] = nlNet(i);

    int i = 1;
    for (dbStringList *cn = db->criticalNetList(); cn; cn = cn->next) {
        if (verbose() > 1)
            db->emitMesg("critical net %s\n", cn->string);

        dbNet *net = getNet(cn->string);
        if (net) {
            net->netorder = i++;
            net->flags |= NET_CRITICAL;
        }
    }

    // Set the order number of non-critical nets.  This is used as a
    // tie-breaker when sorting.
    for (u_int j = 0; j < numNets(); j++) {
        dbNet *net = mr_nets[j];
        if (!(net->flags & NET_CRITICAL))
            mr_nets[j]->netorder = j;
    }

    switch (mr_net_order) {

    case mrNetDefault:
        std::sort(mr_nets, mr_nets + numNets(), compNets);
        break;
    case mrNetAlt1:
        std::sort(mr_nets, mr_nets + numNets(), compNetsAlt);
        break;
    case mrNetNoSort:
        std::sort(mr_nets, mr_nets + numNets(), compNetsNS);
        break;
    }

    FILE *fp = 0;
    if (debug() & LD_DBG_NETS)
        fp = fopen("nets", "w");

    // Renumber all nets in the present order.
    for (u_int j = 0; j < numNets(); j++) {
        dbNet *net = mr_nets[j];
        if (fp) {
            fprintf(fp, "%-4d %-4d %-4d %-4d %s\n", j, net->netorder,
                (net->flags & NET_CRITICAL) != 0, net->numnodes, net->netname);
        }
        net->netorder = j;
    }
    if (fp)
        fclose(fp);
}


// get_net_to_route
//
// Get a net to route.
//
// AUTHOR: steve beccue
//
dbNet *
cMRouter::get_net_to_route(int order)
{
    dbNet *net = mr_nets ? mr_nets[order] : 0;
    if (!net)
        return (0);
  
    if (net->flags & NET_IGNORED)
        return (0);
    if (net->numnodes >= 2)
        return (net);

    // The router will route power and ground nets even if the
    // standard cell power and ground pins are not listed in the nets
    // section.  Because of this, it is okay to have only one node.

    if ((net->numnodes == 1) && (net->flags & NET_GLOBAL))
        return (net);

    if (verbose() > 3) {
        db->flushMesg();
        db->emitErrMesg("get_net_to_route():  Fell through\n");
    }
    return (0);

}


// ripup_colliding
//
// Find all routes that collide with net "net", remove them from the
// Obs[] matrix, append them to the mr_failedNets list, and then write
// the net "net" back to the Obs[] matrix.
//
// Return the number of nets ripped up, or -1 it limit exceeded.
//
int
cMRouter::ripup_colliding(dbNet *net)
{
    // Analyze route for nets with which it collides.

    int ripped;
    dbNetList *nl = find_colliding(net, &ripped);

    // "ripLimit" limits the number of collisions so that the router
    // avoids ripping up huge numbers of nets, which can cause the
    // number of failed nets to keep increasing.

    if (ripped > mr_ripLimit) {
        nl->free();
        return (-1);
    }

    // Remove the colliding nets from the route grid and append them
    // to mr_failedNets.

    ripped = 0;
    while (nl) {
        ripped++;
        dbNetList *nlnext = nl->next;
        nl->next = 0;
        if (verbose() > 0)
            db->emitMesg("Ripping up blocking net %s\n", nl->net->netname);
        if (ripupNet(nl->net, true) == LD_OK) { 
            mr_failedNets.append(nl);

            // Add nl->net to "noripup" list for this net, so it won't
            // be routed over again by the net.  Avoids infinite
            // looping in the second stage.

            net->noripup = new dbNetList(nl->net, net->noripup);
        }
        else
            delete nl;

        nl = nlnext;
     }
     return (ripped);
}


// Fill mask around the area of a vertical line.
//
void
cMRouter::create_vbranch_mask(int x, int y1, int y2, int slack, int halo)
{
    int gx1 = x - slack;
    int gx2 = x + slack;
    int gy1, gy2;
    if (y1 > y2) {
        gy1 = y2 - slack;
        gy2 = y1 + slack;
    }
    else {
        gy1 = y1 - slack;
        gy2 = y2 + slack;
    }
    if (gx1 < 0)
        gx1 = 0;
    if (gx2 >= numChannelsX(0))
        gx2 = numChannelsX(0) - 1;
    if (gy1 < 0)
        gy1 = 0;
    if (gy2 >= numChannelsY(0))
        gy2 = numChannelsY(0) - 1;

    for (int i = gx1; i <= gx2; i++) {
        for (int j = gy1; j <= gy2; j++)
            setRmask(i, j, 0);
    }

    for (int k = 1; k < halo; k++) {
        int h = mr_rmaskIncs ? ((k <= mr_rmaskIncsSz) ? mr_rmaskIncs[k-1] :
            mr_rmaskIncs[mr_rmaskIncsSz - 1]) : 1;
        for (int d = 0; d < h; d++) {
            if (gx1 > 0)
                gx1--;
            if (gx2 < numChannelsX(0) - 1)
                gx2++;
            if (y1 > y2) {
                if (gy1 < numChannelsY(0) - 1)
                    gy1++;
                if (gy2 < numChannelsY(0) - 1)
                    gy2++;
            }
            else {
                if (gy1 > 0)
                    gy1--;
                if (gy2 > 0)
                    gy2--;
            }
            for (int i = gx1; i <= gx2; i++) {
                for (int j = gy1; j <= gy2; j++) {
                    int m = rmask(i, j);
                    if (m > k)
                        setRmask(i, j, k);
                }
            }
        }
    }
}


// Fill mask around the area of a horizontal line.
//
void
cMRouter::create_hbranch_mask(int y, int x1, int x2, int slack, int halo)
{
    int gy1 = y - slack;
    int gy2 = y + slack;
    int gx1, gx2;
    if (x1 > x2) {
        gx1 = x2 - slack;
        gx2 = x1 + slack;
    }
    else {
        gx1 = x1 - slack;
        gx2 = x2 + slack;
    }
    if (gx1 < 0)
        gx1 = 0;
    if (gx2 >= numChannelsX(0))
        gx2 = numChannelsX(0) - 1;
    if (gy1 < 0)
        gy1 = 0;
    if (gy2 >= numChannelsY(0))
        gy2 = numChannelsY(0) - 1;

    for (int i = gx1; i <= gx2; i++) {
        for (int j = gy1; j <= gy2; j++)
            setRmask(i, j, 0);
    }

    for (int k = 1; k < halo; k++) {
        int h = mr_rmaskIncs ? ((k <= mr_rmaskIncsSz) ? mr_rmaskIncs[k-1] :
            mr_rmaskIncs[mr_rmaskIncsSz - 1]) : 1;
        for (int d = 0; d < h; d++) {
            if (gy1 > 0)
                gy1--;
            if (gy2 < numChannelsY(0) - 1)
                gy2++;
            if (x1 > x2) {
                if (gx1 < numChannelsX(0) - 1)
                    gx1++;
                if (gx2 < numChannelsX(0) - 1)
                    gx2++;
            }
            else {
                if (gx1 > 0)
                    gx1--;
                if (gx2 > 0)
                    gx2--;
            }
            for (int i = gx1; i <= gx2; i++) {
                for (int j = gy1; j <= gy2; j++) {
                    int m = rmask(i, j);
                    if (m > k)
                        setRmask(i, j, k);
                }
            }
        }
    }
}


// create_bbox_mask
//
// Create mask limiting the area to search for routing.
//
// The bounding box mask generates an area including the bounding box
// as defined in the net record, includes all pin positions in the
// mask, and increases the mask area by one route track for each pass,
// up to "halo".
//
void
cMRouter::create_bbox_mask(dbNet *net, int halo)
{
    fill_mask(halo);

    int xmin = net->xmin;
    int xmax = net->xmax;
    int ymin = net->ymin;
    int ymax = net->ymax;
  
    for (int gx1 = xmin; gx1 <= xmax; gx1++) {
        for (int gy1 = ymin; gy1 <= ymax; gy1++)
            setRmask(gx1, gy1, 0);
    }

    int i = 0;
    for (int k = 1; k < halo; k++) {
        int h = mr_rmaskIncs ? ((k <= mr_rmaskIncsSz) ? mr_rmaskIncs[k-1] :
            mr_rmaskIncs[mr_rmaskIncsSz - 1]) : 1;
        for (int d = 0; d < h; d++) {
            i++;
            int gx1 = xmin - i;
            if (gx1 >= 0 && gx1 < numChannelsX(0)) {
                for (int j = ymin - i; j <= ymax + i; j++) {
                    if (j >= 0 && j < numChannelsY(0))
                        setRmask(gx1, j, k);
                }
            }

            int gx2 = xmax + i;
            if (gx2 >= 0 && gx2 < numChannelsX(0)) {
                for (int j = ymin - i; j <= ymax + i; j++) {
                    if (j >= 0 && j < numChannelsY(0))
                        setRmask(gx2, j, k);
                }
            }

            int gy1 = ymin - i;
            if (gy1 >= 0 && gy1 < numChannelsY(0)) {
                for (int j = xmin - i; j <= xmax + i; j++) {
                    if (j >= 0 && j < numChannelsX(0))
                        setRmask(j, gy1, k);
                }
            }

            int gy2 = ymax + i;
            if (gy2 >= 0 && gy2 < numChannelsY(0)) {
                for (int j = xmin - i; j <= xmax + i; j++) {
                    if (j >= 0 && j < numChannelsX(0))
                        setRmask(j, gy2, k);
                }
            }
        }
    }
}


// analyze_congestion
//
// Given a trunk route at ycent, between ymin and ymax, score the
// neighboring positions as a function of congestion and offset from
// the ideal location.  Return the position of the best location for
// the trunk route.
//
int
cMRouter::analyze_congestion(int ycent, int ymin, int ymax, int xmin, int xmax)
{
    int *score = new int[ymax - ymin + 1];

    for (int y = ymin; y <= ymax; y++) {
        int sidx = y - ymin;
        score[sidx] = LD_ABSDIFF(ycent, y) * numLayers();
        for (int x = xmin; x <= xmax; x++) {
            for (u_int i = 0; i < numLayers(); i++) {
                int n = obsVal(x, y, i);
                if (n & ROUTED_NET)
                    score[sidx]++;
                if (n & NO_NET)
                    score[sidx]++;
                if (n & PINOBSTRUCTMASK)
                    score[sidx]++;
            }
        }
    }
    int minidx = -1;
    int minscore = MAXRT;
    for (int i = 0; i < (ymax - ymin + 1); i++) {
        if (score[i] < minscore) {
            minscore = score[i];
            minidx = i + ymin;
        }
    }

    delete [] score;
    return (minidx);
}


// create_mask
//
// Create mask limiting the area to search for routing.
//
// For 2-node routes, find the two L-shaped routes between the two
// closest points of the nodes.  For multi-node (>2) routes, find the
// best trunk line that passes close to all nodes, and generate stems
// to the closest point on each node.
//
// Optimizations:  (1) multi-node routes that are in a small enough
// area, just mask the bounding box.  (2) Where nodes at the end of
// two branches are closer to each other than to the trunk, mask an
// additional cross-connection between the two branches.
//
// Values are "halo" where there is no mask, 0 on the closest "slack"
// routes to the ideal (typically 1), and values increasing out to a
// distance of "halo" tracks away from the ideal.  This allows a
// greater search area as the number of passes of the search algorithm
// increases.
//
// To do:  Choose the position of trunk line based on congestion
// analysis.
//
void
cMRouter::create_mask(dbNet *net, int slack, int halo)
{
    fill_mask(halo);

    int xmin = net->xmin;
    int xmax = net->xmax;
    int ymin = net->ymin;
    int ymax = net->ymax;

    int xcent = net->trunkx;
    int ycent = net->trunky;

#define HORZ    1
#define VERT    2
    int orient = 0;

    // Construct the trunk line mask.

    if (!(net->flags & NET_VERTICAL_TRUNK) || (net->numnodes == 2)) {
        // Horizontal trunk.
        orient |= HORZ;

        ycent = analyze_congestion(net->trunky, ymin, ymax, xmin, xmax);
        ymin = ymax = ycent;

        for (int i = xmin - slack; i <= xmax + slack; i++) {
            if (i < 0 || i >= numChannelsX(0))
                continue;
            for (int j = ycent - slack; j <= ycent + slack; j++) {
                if (j < 0 || j >= numChannelsY(0)) continue;
                    setRmask(i, j, 0);
            }
        }

        int i = 0;
        for (int k = 1; k < halo; k++) {
            int h = mr_rmaskIncs ? ((k <= mr_rmaskIncsSz) ? mr_rmaskIncs[k-1] :
                mr_rmaskIncs[mr_rmaskIncsSz - 1]) : 1;
            for (int d = 0; d < h; d++) {
                i++;
                int gy1 = ycent - slack - i;
                int gy2 = ycent + slack + i;
                for (int j = xmin - slack - i; j <= xmax + slack + i; j++) {
                    if (j < 0 || j >= numChannelsX(0))
                        continue;
                    if (gy1 >= 0)
                        setRmask(j, gy1, k);
                    if (gy2 < numChannelsY(0))
                        setRmask(j, gy2, k);
                }
                int gx1 = xmin - slack - i;
                int gx2 = xmax + slack + i;
                for (int j = ycent - slack - i; j <= ycent + slack + i; j++) {
                    if (j < 0 || j >= numChannelsY(0))
                        continue;
                    if (gx1 >= 0)
                        setRmask(gx1, j, k);
                    if (gx2 < numChannelsX(0))
                        setRmask(gx2, j, k);
                }
            }
        }
    }
    if ((net->flags & NET_VERTICAL_TRUNK) || (net->numnodes == 2)) {
        // Vertical trunk.
        orient |= VERT;
        xmin = xmax = xcent;

        for (int i = xcent - slack; i <= xcent + slack; i++) {
            if (i < 0 || i >= numChannelsX(0))
                continue;
            for (int j = ymin - slack; j <= ymax + slack; j++) {
                if (j < 0 || j >= numChannelsY(0))
                    continue;
                setRmask(i, j, 0);
            }
        }

        int i = 0;
        for (int k = 1; k < halo; k++) {
            int h = mr_rmaskIncs ? ((k <= mr_rmaskIncsSz) ? mr_rmaskIncs[k-1] :
                mr_rmaskIncs[mr_rmaskIncsSz - 1]) : 1;
            for (int d = 0; d < h; d++) {
                i++;
                int gx1 = xcent - slack - i;
                int gx2 = xcent + slack + i;
                for (int j = ymin - slack - i; j <= ymax + slack + i; j++) {
                    if (j < 0 || j >= numChannelsY(0))
                        continue;
                    if (gx1 >= 0)
                        setRmask(gx1, j, k);
                    if (gx2 < numChannelsX(0))
                        setRmask(gx2, j, k);
                }
                int gy1 = ymin - slack - i;
                int gy2 = ymax + slack + i;
                for (int j = xcent - slack - i; j <= xcent + slack + i; j++) {
                    if (j < 0 || j >= numChannelsX(0))
                        continue;
                    if (gy1 >= 0)
                        setRmask(j, gy1, k);
                    if (gy2 < numChannelsY(0))
                        setRmask(j, gy2, k);
                }
            }
        }
    }
     
    // Construct the branch line masks.

    for (dbNode *n1 = net->netnodes; n1; n1 = n1->next) {
        dbDpoint *dtap = (n1->taps == 0) ? n1->extend : n1->taps;
        if (!dtap)
            continue;

        if (orient | HORZ)      // Horizontal trunk, vertical branches.
            create_vbranch_mask(n1->branchx, n1->branchy, ycent, slack, halo);
        if (orient | VERT)      // Vertical trunk, horizontal branches.
            create_hbranch_mask(n1->branchy, n1->branchx, xcent, slack, halo);
    }

    // Look for branches that are closer to each other than to the
    // trunk line.  If any are found, make a cross-connection between
    // the branch end that is closer to the trunk and the branch that
    // is its nearest neighbor.

    if (orient | HORZ) {        // Horizontal trunk, vertical branches.
        for (dbNode *n1 = net->netnodes; n1; n1 = n1->next) {
            for (dbNode *n2 = net->netnodes->next; n2; n2 = n2->next) {

                // Check if both ends are on the same side of the trunk.
                if ((n2->branchy > ycent && n1->branchy > ycent) ||
                        (n2->branchy < ycent && n1->branchy < ycent)) {

                    // Check if branches are closer to each other than
                    // the shortest branch is away from the trunk.
                    int dx = LD_ABSDIFF(n2->branchx, n1->branchx);
                    int gy1 = LD_ABSDIFF(n1->branchy, ycent);
                    int gy2 = LD_ABSDIFF(n2->branchy, ycent);
                    if ((dx < gy1) && (dx < gy2)) {
                        if (gy1 < gy2)
                            create_hbranch_mask(n1->branchy, n2->branchx,
                                n1->branchx, slack, halo);
                        else
                            create_hbranch_mask(n2->branchy, n2->branchx,
                                n1->branchx, slack, halo);
                    }
                }
            }
        }
    }
    if (orient | VERT) {        // Vertical trunk, horizontal branches.
        for (dbNode *n1 = net->netnodes; n1; n1 = n1->next) {
            for (dbNode *n2 = net->netnodes->next; n2; n2 = n2->next) {

                // Check if both ends are on the same side of the trunk.
                if ((n2->branchx > xcent && n1->branchx > xcent) ||
                        (n2->branchx < xcent && n1->branchx < xcent)) {

                    // Check if branches are closer to each other than
                    // the shortest branch is away from the trunk.
                    int dy = LD_ABSDIFF(n2->branchy, n1->branchy);
                    int gx1 = LD_ABSDIFF(n1->branchx, xcent);
                    int gx2 = LD_ABSDIFF(n2->branchx, xcent);
                    if ((dy < gx1) && (dy < gx2)) {
                        if (gx1 < gx2)
                            create_vbranch_mask(n1->branchx, n2->branchy,
                                n1->branchy, slack, halo);
                        else
                            create_vbranch_mask(n2->branchx, n2->branchy,
                                n1->branchy, slack, halo);
                    }
                }
            }
        }
    }

    // Allow routes at all tap and extension points.
    for (dbNode *n1 = net->netnodes; n1; n1 = n1->next) {
        for (dbDpoint *dtap = n1->taps; dtap; dtap = dtap->next)
            setRmask(dtap->gridx, dtap->gridy, 0);
        for (dbDpoint *dtap = n1->extend; dtap; dtap = dtap->next)
            setRmask(dtap->gridx, dtap->gridy, 0);
    }

    if (verbose() > 2) {
        if (net->numnodes == 2) {
            db->emitMesg(
                "Two-port mask has bounding box (%d %d) to (%d %d)\n",
                xmin, ymin, xmax, ymax);
        }
        else {
            db->emitMesg(
                "multi-port mask has trunk line (%d %d) to (%d %d)\n",
                xmin, ymin, xmax, ymax);
        }
    }
}


// fill_mask
//
// Fills the mr_rmask array with all 1s as a last resort, ensuring
// that no valid routes are missed due to a bad guess about the
// optimal route positions.
//
void
cMRouter::fill_mask(int value)
{
    size_t sz = numChannelsX(0) * numChannelsY(0);
    memset(mr_rmask, value, sz);
}


// unable_to_route
//
// Catch-all function when no tap points are found.  This is a common
// problem when the technology is not set up correctly and it's
// helpful to have all these error conditions pass to a single
// entry point.
//
void
cMRouter::unable_to_route(const char *netname, dbNode *node, bool forced)
{
    if (node)
        db->emitErrMesg("Node %s of net %s has no tap points---",
            db->printNodeName(node), netname);
    else
        db->emitErrMesg("Node of net %s has no tap points---", netname);

    if (forced)
        db->emitErrMesg("forcing a tap point.\n");
    else
        db->emitErrMesg("unable to route!\n");
}


// next_route_setup
//
mrRval
cMRouter::next_route_setup(mrRouteInfo *iroute, mrStage stage)
{
    mrRval result = mrProvisional;
    if (iroute->do_pwrbus) {

        iroute->pwrbus_src++;
        iroute->nsrc = iroute->nsrc->next;
        mrRval rval = mrUnroutable;
        while (rval == mrUnroutable) {
            if ((iroute->pwrbus_src > iroute->net->numnodes) ||
                    (iroute->nsrc == 0)) {
                result = mrProvisional;
                break;
            }
            else {
                result = set_powerbus_to_net(iroute->nsrc->netnum);
                clear_target_node(iroute->nsrc);
                rval = set_node_to_net(iroute->nsrc, PR_SOURCE, iroute->stack,
                    &iroute->bbox, stage);
                if (rval == mrUnroutable) {
                    if (mr_forceRoutable) {
                        make_routable(iroute->nsrc);
                    }
                    else {
                        iroute->pwrbus_src++;
                        iroute->nsrc = iroute->nsrc->next;
                    }
                    unable_to_route(iroute->net->netname, iroute->nsrc,
                        mr_forceRoutable);
                }
                else if (rval == mrError)
                    return (mrError);
            }
        }
    }
    else {

        dbRoute *rt = iroute->net->routes;
        if (rt) {
            while (rt->next)
                rt = rt->next;
        }

        // Set positions on last route to PR_SOURCE.
        if (rt) {
            result = set_route_to_net(iroute->net, rt, PR_SOURCE,
                iroute->stack, &iroute->bbox, stage);

            if (result == mrUnroutable) {
                unable_to_route(iroute->net->netname, NULL, 0);
                return (mrError);
            }
        }
        else
            return (mrError);

        result = (count_targets(iroute->net) == 0) ? mrProvisional : mrSuccess;
    }

    // Check for the possibility that there is already a route to the
    // target.

    if (result == mrProvisional) {

        // Remove nodes of the net from Nodeinfo.nodeloc so that they
        // will not be used for crossover costing of future routes.

        for (u_int i = 0; i < pinLayers(); i++) {
            u_int sz = numChannelsX(i) * numChannelsY(i);
            for (u_int j = 0; j < sz; j++) {
                mrGridCell c;
                c.layer = i;
                c.index = j;
                dbNode *node = nodeLoc(c);
                if (node && node->netnum == (u_int)iroute->net->netnum)
                    setNodeLoc(c, 0);
            }
        }

        iroute->stack->clear();
        return (mrProvisional);
    }

    if (!iroute->do_pwrbus) {

        // If any target is found during the search, but is not the
        // target that is chosen for the minimum-cost path, then it
        // will be left marked "processed" and never visited again. 
        // Make sure this doesn't happen my clearing the "processed"
        // flag from all such target nodes, and placing the positions
        // on the stack for processing again.

        clear_non_source_targets(iroute->net, iroute->stack);
    }

    if (verbose() > 1) {
        db->emitMesg("netname = %s, route number %d\n",
            iroute->net->netname, mr_totalRoutes );
        db->flushMesg();
    }

    if (iroute->maxcost > 2)
        iroute->maxcost >>= 1;  // Halve the maximum cost from the last run.

    return (mrSuccess);         // Successful setup.
}


// route_setup
//
mrRval
cMRouter::route_setup(mrRouteInfo *iroute, mrStage stage, u_int *unrt)
{
    // Make Obs2[][] a copy of Obs[][].  Convert pin obstructions to
    // terminal positions for the net being routed.

    if (unrt)
        *unrt = 0;
    for (u_int i = 0; i < numLayers(); i++) {
        u_int sz = numChannelsX(i) * numChannelsY(i);
        for (u_int j = 0; j < sz; j++) {
            u_int netnum = obsAry(i)[j] & (~BLOCKED_MASK);
            mrProute *Pr = &obs2Ary(i)[j];
            if (netnum != 0) {
                Pr->flags = 0;            // Clear all flags
                if (netnum == DRC_BLOCKAGE)
                    Pr->prdata.net = netnum;
                else
                    Pr->prdata.net = netnum & NETNUM_MASK;
            }
            else {
                Pr->flags = PR_COST;        // This location is routable.
                Pr->prdata.cost = MAXRT;
            }
        }
    }

    int result;
    if (iroute->net->flags & NET_GLOBAL) {
        // The normal method of selecting source and target is not
        // amenable to power bus routes.  Instead, we use the global
        // standard cell power rails as the target, and each net in
        // sequence becomes the sole source node.
     
        iroute->do_pwrbus = true;
        iroute->nsrc = find_unrouted_node(iroute->net);
        result = (iroute->nsrc != 0);
    }
    else {
        iroute->do_pwrbus = false;
        if (iroute->net->netnodes)
            iroute->nsrc = iroute->net->netnodes;
        else {
            db->emitErrMesg("Net %s has no nodes, unable to route!\n",
                iroute->net->netname);
            return (mrError);
        }
        result = 1;
    }

    // We start at the node referenced by the route structure, and
    // flag all of its taps as PR_SOURCE, as well as all connected
    // routes.

    u_int unroutable = 0;

    if (result) {
        iroute->bbox.x2 = iroute->bbox.y2 = 0;
        iroute->bbox.x1 = numChannelsX(0);
        iroute->bbox.y1 = numChannelsY(0);

        mrRval rval;
        for (;;) {
            rval = set_node_to_net(iroute->nsrc, PR_SOURCE, iroute->stack,
                &iroute->bbox, stage);
            if (rval == mrUnroutable) {
                iroute->nsrc = iroute->nsrc->next;
                if (!iroute->nsrc)
                    break;
            }
            else
                break;
        }
        if (rval == mrUnroutable) {
            if (mr_forceRoutable)
                    make_routable(iroute->net->netnodes);
            unable_to_route(iroute->net->netname, iroute->nsrc,
                mr_forceRoutable);
            return (mrError);
        }

        if (iroute->do_pwrbus == false) {

            // Set associated routes to PR_SOURCE.
            rval = set_routes_to_net(iroute->net, PR_SOURCE,
                iroute->stack, &iroute->bbox, stage);

            if (rval == mrUnroutable) {
                unable_to_route(iroute->net->netname, NULL, 0);
                return (mrError);
            }

            // Now search for all other nodes on the same net that
            // have not yet been routed, and flag all of their taps as
            // PR_TARGET.

            result = 0;
            for (dbNode *node = iroute->net->netnodes; node;
                    node = node->next) {
                if (node == iroute->nsrc)
                    continue;
                rval = set_node_to_net(node, PR_TARGET, NULL, &iroute->bbox,
                    stage);
                if (rval == mrSuccess) {
                    result = 1;
                }
                else if (rval == mrUnroutable) {
                    if (mr_forceRoutable)
                        make_routable(node);
                    unable_to_route(iroute->net->netname, node,
                        mr_forceRoutable);
                    if (result == 0)
                        result = -1;
                    unroutable++;
                }
            }

            // If there's only one node and it's not routable, then fail.
            if (result == -1)
                return (mrError);
        }
        else {     // Do this for power bus connections.

            // Set all nodes that are NOT nsrc to an unused net number.
            for (dbNode *node = iroute->net->netnodes; node;
                    node = node->next) {
                if (node != iroute->nsrc) {
                    disable_node_nets(node);
                }
            }
            set_powerbus_to_net(iroute->nsrc->netnum);
        }
    }

    // Check for the possibility that there is already a route to the
    // target.

    if (!result) {
        // Remove nodes of the net from Nodeinfo.nodeloc so that they
        // will not be used for crossover costing of future routes.

        for (u_int i = 0; i < pinLayers(); i++) {
            u_int sz = numChannelsX(i) * numChannelsY(i);
            for (u_int j = 0; j < sz; j++) {
                mrGridCell c;
                c.layer = i;
                c.index = j;
                iroute->nsrc = nodeLoc(c);
                if (iroute->nsrc &&
                        iroute->nsrc->netnum == (u_int)iroute->net->netnum)
                    setNodeLoc(c, 0);
            }
        }

        iroute->stack->clear();
        return (mrProvisional);
    }

    // Generate a search area mask representing the "likely best route".
    if (!iroute->do_pwrbus && (mr_mask == MASK_AUTO)) {
        if (stage == mrStage1)
            create_mask(iroute->net, MASK_SMALL, numPasses());
        else
            create_mask(iroute->net, MASK_LARGE, numPasses());
    }
    else if (iroute->do_pwrbus || (mr_mask == MASK_NONE))
        fill_mask(0);
    else if (mr_mask == MASK_BBOX)
        create_bbox_mask(iroute->net, numPasses());
    else
        create_mask(iroute->net, mr_mask, numPasses());

    // Heuristic:  Set the initial cost beyond which we stop
    // searching.  This value is twice the cost of a direct route
    // across the maximum extent of the source to target, divided by
    // the square root of the number of nodes in the net.  We
    // purposely set this value low.  It has a severe impact on the
    // total run time of the algorithm.  If the initial max cost is so
    // low that no route can be found, it will be doubled on each
    // pass.

    if (iroute->do_pwrbus)
        iroute->maxcost = 20;      // Maybe make this SegCost * row height?
    else {
        iroute->maxcost = 1 + 2 * LD_MAX((iroute->bbox.x2 - iroute->bbox.x1),
            (iroute->bbox.y2 - iroute->bbox.y1))
            * segCost() + (stage == mrStage2) * conflictCost();
        iroute->maxcost /= (iroute->nsrc->numnodes - 1);
    }

    iroute->nsrctap = iroute->nsrc->taps;
    if (!iroute->nsrctap)
        iroute->nsrctap = iroute->nsrc->extend;
    if (!iroute->nsrctap) {
        unable_to_route(iroute->net->netname, iroute->nsrc, 0);
        return (mrError);
    }

    if (verbose() > 2) {
        db->emitMesg("Source node @ %gum %gum layer=%d grid=(%d %d)\n",
            db->lefToMic(iroute->nsrctap->x), db->lefToMic(iroute->nsrctap->y),
            iroute->nsrctap->layer,
            iroute->nsrctap->gridx, iroute->nsrctap->gridy);
    }

    if (verbose() > 1) {
        db->emitMesg("netname = %s, route number %d\n",
            iroute->net->netname, mr_totalRoutes );
        db->flushMesg();
    }

    // Successful setup, although if nodes were marked unroutable,
    // this information is passed back; routing will proceed for all
    // routable nodes and the net will be then marked as abandoned.

    if (unrt)
        *unrt = unroutable;
    return (mrSuccess);
}


//#define DEBUG_RS

// route_segs
//
// Detailed route from node to node using onestep method.
//
// Return mrError general error, mrProvisional via stacking error,
// mrSuccess success.
//
// AUTHOR: steve beccue
//
mrRval
cMRouter::route_segs(mrRouteInfo *iroute, mrStage stage, bool graphdebug)
{
    mrGridP best;
    best.cost = MAXRT;
    u_int maskpass = 0;
    bool first = true;

#ifdef DEBUG_RS
    for (mrPoint *m = iroute->glist; m; m = m->next)
        db->emitMesg("xyl %d %d %d\n", m->x1, m->y1, m->layer);
#endif
  
    mrRval rval = mrProvisional;
    u_int pass = 0;
    for ( ; pass < numPasses(); pass++) {

        bool max_reached = false;
        if (!first && (verbose() > 2)) {
            db->emitMesg("\n");
            first = true;
        }
        if (verbose() > 2) {
            db->emitMesg("Pass %d", pass + 1);
            db->emitMesg(" (maxcost is %d)\n", iroute->maxcost);
        }

        // Clear the 'listed' flags.  These are used when saving stack
        // elements to prevent duplicate entries.

        for (u_int l = 0; l < numLayers(); l++) {
            int lsz = numChannelsX(0) * numChannelsY(0);
            memset(listedAry(l), 0, lsz*sizeof(bool));
        }

        mrGridP curpt;
        while (iroute->stack->pop(&curpt.x, &curpt.y, &curpt.lay)) {

            if (graphdebug && mr_graphics)
                mr_graphics->highlight(curpt.x, curpt.y);
        
            mrProute *Pr = obs2Val(curpt.x, curpt.y, curpt.lay);

            // Ignore grid positions that have already been processed.
            if (Pr->flags & PR_PROCESSED)
                continue;

            if (Pr->flags & PR_COST)
                curpt.cost = Pr->prdata.cost; // Route points, including target
            else
                curpt.cost = 0;                 // For source tap points

            // If the grid position is the destination, save the
            // position and cost if minimum.

            if (Pr->flags & PR_TARGET) {

                if (curpt.cost < best.cost) {
                    if (first) {
                        if (verbose() > 2)
                            db->emitMesg("Found a route of cost ");
                        first = false;
                    }
                    else if (verbose() > 2) {
                        db->emitMesg("|");
                        db->emitMesg("%d", curpt.cost);
                        db->flushMesg();
                    }

                    // This position may be on a route, not at a
                    // terminal, so record it.

                    best.x = curpt.x;
                    best.y = curpt.y;
                    best.lay = curpt.lay;
                    best.cost = curpt.cost;

                    // If a complete route has been found, then
                    // there's no point in searching paths with a
                    // greater cost than this one.

                    if (best.cost < iroute->maxcost)
                        iroute->maxcost = best.cost;
                }

                // Don't continue processing from the target.
                Pr->flags |= PR_PROCESSED;
                continue;
            }

            if (curpt.cost < MAXRT) {

                // Severely limit the search space by not
                // processing anything that is not under the
                // current route mask, which identifies a narrow
                // "best route" solution.

                if (rmask(curpt.x, curpt.y) > maskpass) {
                    if (!listed(curpt.x, curpt.y, curpt.lay)) {
                        setListed(curpt.x, curpt.y, curpt.lay, true);
                        iroute->stack->push_save(curpt.x, curpt.y, curpt.lay);
                    }
                    continue;
                }

                // Quick check:  Limit maximum cost to limit
                // search space Move the point onto the
                // "unprocessed" stack and we'll pick up from this
                // point on the next pass, if needed.

                if (curpt.cost > iroute->maxcost) {
                    max_reached = true;
                    if (!listed(curpt.x, curpt.y, curpt.lay)) {
                        setListed(curpt.x, curpt.y, curpt.lay, true);
                        iroute->stack->push_save(curpt.x, curpt.y, curpt.lay);
                    }
                    continue;
                }
            }

            // Check east/west/north/south, and bottom to top.

            // 1st optimization:  Direction of route on current
            // layer is preferred.

            ROUTE_DIR o = getRouteOrientation(curpt.lay);
            u_int forbid =
                obsVal(curpt.x, curpt.y, curpt.lay) & BLOCKED_MASK;

            // To reach otherwise unreachable taps, allow
            // searching on blocked paths but with a high cost.

            u_char conflict = mr_forceRoutable ? PR_CONFLICT : PR_NO_EVAL;

            u_char check_order[6];
            if (o == DIR_HORIZ) {
                // Horizontal routes---check EAST and WEST first.
                check_order[0] = EAST  | ((forbid & BLOCKED_E) ? conflict : 0);
                check_order[1] = WEST  | ((forbid & BLOCKED_W) ? conflict : 0);
                check_order[2] = UP    | ((forbid & BLOCKED_U) ? conflict : 0);
                check_order[3] = DOWN  | ((forbid & BLOCKED_D) ? conflict : 0);
                check_order[4] = NORTH | ((forbid & BLOCKED_N) ? conflict : 0);
                check_order[5] = SOUTH | ((forbid & BLOCKED_S) ? conflict : 0);
            }
            else {
                // Vertical routes---check NORTH and SOUTH first.
                check_order[0] = NORTH | ((forbid & BLOCKED_N) ? conflict : 0);
                check_order[1] = SOUTH | ((forbid & BLOCKED_S) ? conflict : 0);
                check_order[2] = UP    | ((forbid & BLOCKED_U) ? conflict : 0);
                check_order[3] = DOWN  | ((forbid & BLOCKED_D) ? conflict : 0);
                check_order[4] = EAST  | ((forbid & BLOCKED_E) ? conflict : 0);
                check_order[5] = WEST  | ((forbid & BLOCKED_W) ? conflict : 0);
            }

            // Check order is from 0 (1st priority) to 5 (last
            // priority).  However, this is a stack system, so the
            // last one placed on the stack is the first to be
            // pulled and processed.  Therefore we evaluate and
            // drop positions to check on the stack in reverse
            // order (5 to 0).

            for (int i = 5; i >= 0; i--) {
                u_int predecessor = 0;
                switch (check_order[i]) {
                case EAST | PR_CONFLICT:
                    predecessor = PR_CONFLICT;
                case EAST:
                    predecessor |= PR_PRED_W;
                    if ((curpt.x + 1) < numChannelsX(curpt.lay)) {
                        if (eval_pt(&curpt, predecessor, stage)) {
                            iroute->stack->push(curpt.x + 1, curpt.y,
                                curpt.lay);
#ifdef DEBUG_RS
printf("RSa %d %d %d\n", curpt.x+1, curpt.y, curpt.lay);
#endif
                        }
                    }
                    break;

                case WEST | PR_CONFLICT:
                    predecessor = PR_CONFLICT;
                case WEST:
                    predecessor |= PR_PRED_E;
                    if ((curpt.x - 1) >= 0) {
                        if (eval_pt(&curpt, predecessor, stage)) {
                            iroute->stack->push(curpt.x - 1, curpt.y,
                                curpt.lay);
#ifdef DEBUG_RS
printf("RSb %d %d %d\n", curpt.x-1, curpt.y, curpt.lay);
#endif
                        }
                    }
                    break;
     
                case SOUTH | PR_CONFLICT:
                    predecessor = PR_CONFLICT;
                case SOUTH:
                    predecessor |= PR_PRED_N;
                    if ((curpt.y - 1) >= 0) {
                        if (eval_pt(&curpt, predecessor, stage)) {
                            iroute->stack->push(curpt.x, curpt.y - 1,
                                curpt.lay);
#ifdef DEBUG_RS
printf("RSc %d %d %d\n", curpt.x, curpt.y-1, curpt.lay);
#endif
                        }
                    }
                    break;

                case NORTH | PR_CONFLICT:
                    predecessor = PR_CONFLICT;
                case NORTH:
                    predecessor |= PR_PRED_S;
                    if ((curpt.y + 1) < numChannelsY(curpt.lay)) {
                        if (eval_pt(&curpt, predecessor, stage)) {
                            iroute->stack->push(curpt.x, curpt.y + 1,
                                curpt.lay);
#ifdef DEBUG_RS
printf("RSd %d %d %d\n", curpt.x, curpt.y+1, curpt.lay);
#endif
                        }
                    }
                    break;
  
                case DOWN | PR_CONFLICT:
                    predecessor = PR_CONFLICT;
                case DOWN:
                    predecessor |= PR_PRED_U;
                    if (curpt.lay > 0) {
                        if (eval_pt(&curpt, predecessor, stage)) {
                            iroute->stack->push(curpt.x, curpt.y,
                                curpt.lay - 1);
#ifdef DEBUG_RS
printf("RSe %d %d %d\n", curpt.x, curpt.y, curpt.lay-1);
#endif
                        }
                    }
                    break;
     
                case UP | PR_CONFLICT:
                    predecessor = PR_CONFLICT;
                case UP:
                    predecessor |= PR_PRED_D;
                    if (curpt.lay < ((int)numLayers() - 1)) {
                        if (eval_pt(&curpt, predecessor, stage)) {
                            iroute->stack->push(curpt.x, curpt.y,
                                curpt.lay + 1);
#ifdef DEBUG_RS
printf("RSf %d %d %d\n", curpt.x, curpt.y, curpt.lay+1);
#endif
                        }
                    }
                    break;
                }
            }

            // Mark this node as processed.
            Pr->flags |= PR_PROCESSED;

        } // while stack is not empty

        iroute->stack->clear();

        // If we found a route, save it and return.

        if (best.cost <= iroute->maxcost) {
            curpt.x = best.x;
            curpt.y = best.y;
            curpt.lay = best.lay;
            rval = commit_proute(iroute->rt, &curpt, stage);
            if (rval != mrSuccess)
                break;
            if (verbose() > 2) {
                db->emitMesg(
                    "\nCommit to a route of cost %d\n", best.cost);
                db->emitMesg("Between positions (%d %d) and (%d %d)\n",
                    best.x, best.y, curpt.x, curpt.y);
            }

            iroute->stack->clear_to_saved();
            return (mrSuccess);
        }

        // Continue loop to next pass if any positions were
        // ignored due to masking or due to exceeding maxcost.

        // If the cost of the route exceeded maxcost at one or
        // more locations, then increase maximum cost for next
        // pass.

        if (max_reached) {
            iroute->maxcost <<= 1;
            // Cost overflow; we're probably completely hosed long
            // before this.

            if (iroute->maxcost > MAXRT)
                break;
        }
        else
            maskpass++;     // Increase the mask size.

        if (!iroute->stack->has_saved())
            break;          // Route failure not due to limiting
                            // search to maxcost or to masking.

        // Regenerate the stack of unprocessed nodes.
        iroute->stack->clear_to_saved();

    } // pass

    if (iroute->stack->has_saved())
        iroute->stack->clear_to_saved();

    if (!first && (verbose() > 2)) {
        db->emitMesg("\n");
        db->flushMesg();
    }
    if (verbose() > 1) {
        db->emitErrMesg("Fell through %d passes\n", pass);
    }
    if (!iroute->do_pwrbus && (verbose() > 2)) {
        db->emitErrMesg("(%g,%g) net=%s\n",
            db->lefToMic(iroute->nsrctap->x), db->lefToMic(iroute->nsrctap->y),
            iroute->net->netname);
    }

    return (mrError);
}


// printFlags
//
// Print a list of the obsVal, flagsVal, and obs2val for each grid
// location, useful for debugging.
//
void
cMRouter::printFlags(const char *filename)
{
    FILE *fp;
    if (!filename || !strcmp(filename, "stdout"))
        fp = stdout;
    else {
        fp = fopen(filename, "w");
        if (!fp) {
            db->emitErrMesg("printFlags.  Couldn't open output file\n");
            return;
        }
    }

    for (u_int i = 0; i < numLayers(); i++) {
        for (int x = 0; x < numChannelsX(i); x++) {
            for (int y = 0; y < numChannelsY(i); y++) {
                fprintf(fp, "%d %d %d %x %x %x\n", x, y, i, obsVal(x, y, i),
                    flagsVal(x, y, i), obs2Val(x, y, i) ?
                    obs2Val(x, y, i)->flags : 0);
            }
        }
    }
    if (fp && (fp != stdout))
        fclose(fp);
}


//
// Memory Management.
//

// The size of the struct below is just less that 4096 bytes,
// assuming 64-bit addresses.
#define NI_NUM  170

struct niBlk
{
    niBlk(niBlk *n)     { next = n; }

    niBlk *next;
    mrNodeInfo blocks[NI_NUM];
};

// Bulk allocator for mrNodeInfo.
//
mrNodeInfo *
cMRouter::new_nodeInfo()
{
    if (!mr_ni_blks || mr_ni_cnt == NI_NUM) {
        mr_ni_blks = new niBlk(mr_ni_blks);
        mr_ni_cnt = 0;
    }
    mrNodeInfo *ni = mr_ni_blks->blocks + mr_ni_cnt++;
    return (ni);
}


// Free all blocks.
//
void
cMRouter::clear_nodeInfo()
{
    while (mr_ni_blks) {
        niBlk *x = mr_ni_blks;
        mr_ni_blks = mr_ni_blks->next;
        delete x;
    }
    mr_ni_blks = 0;
    mr_ni_cnt = 0;
}

