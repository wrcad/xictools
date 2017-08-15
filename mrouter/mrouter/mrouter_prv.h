
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
/* qrouter.h -- general purpose autorouter                      */
/*--------------------------------------------------------------*/
/* Written by Steve Beccue, 2003                                */
/* Modified by Tim Edwards 2011-2013                            */
/*--------------------------------------------------------------*/

#ifndef MROUTER_H
#define MROUTER_H

#include "mrouter.h"
#include "mr_stack.h"

//
// Maze Router.
//
// Private include file.
//

// The following values are added to the obs structure for
// unobstructed route positions close to a terminal, but not close
// enough to connect directly.  They describe which direction to go to
// reach the terminal.  The stub vector indicates the distance needed
// to reach the terminal.  The OFFSET_TAP flag marks a position that
// is inside a terminal but which needs to be adjusted in one
// direction to avoid a close obstruction.  The Stub[] vector
// indicates the distance needed to avoid the obstruction.

#define OFFSET_TAP      0x80000000  // Tap position needs to be offset.
#define STUBROUTE       0x40000000  // Route stub to reach terminal.
#define PINOBSTRUCTMASK 0xc0000000  // Either offset tap or stub route.
#define NO_NET          0x20000000  // Indicates a non-routable obstruction.
#define ROUTED_NET      0x10000000  // Indicates posn occupied by a routed net.

#define BLOCKED_N       0x08000000  // Grid point cannot be routed from the N.
#define BLOCKED_S       0x04000000  // Grid point cannot be routed from the S.
#define BLOCKED_E       0x02000000  // Grid point cannot be routed from the E.
#define BLOCKED_W       0x01000000  // Grid point cannot be routed from the W.
#define BLOCKED_U       0x00800000  // Grid point cannot be routed from top.
#define BLOCKED_D       0x00400000  // Grid point cannot be routed from bottom.
#define BLOCKED_MASK    0x0fc00000
#define OBSTRUCT_MASK   0x0000000f  // with NO_NET, directional obstruction.
#define OBSTRUCT_N      0x00000008  // Tells where the obstruction is
#define OBSTRUCT_S      0x00000004  // relative to the grid point.  Nodeinfo
#define OBSTRUCT_E      0x00000002  // offset contains distance to grid point.
#define OBSTRUCT_W      0x00000001

#define MAX_NETNUM      0x003fffff  // Maximum net number.
#define NETNUM_MASK     0x203fffff  // Mask for the net number field
                                    // (includes NO_NET)
#define ROUTED_NET_MASK 0x303fffff  // Mask for the net number field
                                    // (includes NO_NET and ROUTED_NET)
#define DRC_BLOCKAGE    (NO_NET | ROUTED_NET) // Special case

// Defaults
#define MR_PASSES       10
#define MR_SEGCOST      1
#define MR_VIACOST      5
#define MR_JOGCOST      10
#define MR_XVERCOST     4
#define MR_BLOCKCOST    25
#define MR_OFFSETCOST   50
#define MR_CONFLICTCOST 50


// Structure containing x, y, and layer.
//
struct mrGridP
{
    mrGridP()
        {
            x           = 0;
            y           = 0;
            lay         = 0;
            cost        = 0;
        }

    int     x;
    int     y;
    int     lay;
    u_int   cost;
};

// mrPoint is an integer point in three dimensions (layer giving the
// vertical dimension).
//
struct mrPoint
{
    mrPoint()
        {
            next        = 0;
            x1          = 0;
            y1          = 0;
            layer       = -1;
            lefId       = -1;
#ifdef LD_MEMDBG
            mrpoint_cnt++;
            if (mrpoint_cnt > mrpoint_hw)
                mrpoint_hw = mrpoint_cnt;
#endif
        }

    mrPoint(int x, int y, int l, mrPoint *n)
        {
            next        = n;
            x1          = x;
            y1          = y;
            layer       = l;
            lefId       = -1;
#ifdef LD_MEMDBG
            mrpoint_cnt++;
            if (mrpoint_cnt > mrpoint_hw)
                mrpoint_hw = mrpoint_cnt;
#endif
        }

    mrPoint(mrGridP &g, mrPoint *n)
        {
            next        = n;
            x1          = g.x;
            y1          = g.y;
            layer       = g.lay;
            lefId       = -1;
#ifdef LD_MEMDBG
            mrpoint_cnt++;
            if (mrpoint_cnt > mrpoint_hw)
                mrpoint_hw = mrpoint_cnt;
#endif
        }

#ifdef LD_MEMDBG
    ~mrPoint()
        {
            mrpoint_cnt--;
        }
#endif

    void free()
        {
            mrPoint *p = this;
            while (p) {
                mrPoint *px = p;
                p = p->next;
                delete px;
            }
        }

    mrPoint *next; 
    int     x1, y1;
    int     layer;
    int     lefId;      // unused
#ifdef LD_MEMDBG
    static int mrpoint_cnt;
    static int mrpoint_hw;
#endif
};

// Bit values for mrProute::flags.
#define PR_PRED_DMASK   0x07    // Mask for directional bits.

#define PR_PRED_NONE    0x00    // This node does not have a predecessor.
#define PR_PRED_N       0x01    // Predecessor is north.
#define PR_PRED_S       0x02    // Predecessor is south.
#define PR_PRED_E       0x03    // Predecessor is east.
#define PR_PRED_W       0x04    // Predecessor is west.
#define PR_PRED_U       0x05    // Predecessor is up.
#define PR_PRED_D       0x06    // Predecessor is down.

#define PR_PROCESSED    0x08    // Tag to avoid visiting more than once.
#define PR_NO_EVAL      0x08    // Used only for making calls to eval_pt().
#define PR_CONFLICT     0x10    // Two nets collide here during stage 2.
#define PR_SOURCE       0x20    // This is a source node.
#define PR_TARGET       0x40    // This is a target node.
#define PR_COST         0x80    // If set, use prdata.cost, not prdata.net.
// Partial route.
//
struct mrProute
{
    mrProute()
        {
            flags       = 0;
            prdata.cost = 0;
        }

    u_int   flags;      // Values PR_PROCESSED and PR_CONFLICT, and others.
    union {
        u_int cost;     // Cost of route coming from predecessor.
        u_int net;      // Net number at route point.
    } prdata;
};


// A structure to hold information about source and target nets for a
// route, to be passed between the route setup and execution stages.
//
struct mrRouteInfo
{
    mrRouteInfo(dbNet *n)
        {
            net         = n;
            rt          = 0;
            stack       = 0;
            nsrc        = 0;
            nsrctap     = 0;
            maxcost     = MAXRT;
            do_pwrbus   = false;
            pwrbus_src  = 0;
        }

    dbNet   *net;
    dbRoute *rt;
    mrStack *stack;
    dbNode  *nsrc;
    dbDpoint *nsrctap;
    u_int   maxcost;
    bool    do_pwrbus;
    int     pwrbus_src;
    dbSeg   bbox;
};

// Return value of misc. routing functions in cMRouter.
//
enum mrRval
{
    mrUnroutable    = -2,   // Fatal error.
    mrError         = -1,   // Fatal error.
    mrProvisional   = 0,    // OK, but something off.
    mrSuccess       = 1,    // OK, all is well.
};

// Definitions for flags in mrNodeInfo.
#define NI_STUB_NS      0x01   // Stub route north(+)/south(-)
#define NI_STUB_EW      0x02   // Stub route east(+)/west(-)
#define NI_STUB_MASK    0x03   // Stub route mask (N/S + E/W)
#define NI_OFFSET_NS    0x04   // Tap offset north(+)/south(-)
#define NI_OFFSET_EW    0x08   // Tap offset east(+)/west(-)
#define NI_OFFSET_MASK  0x0c   // Tap offset mask (N/S + E/W)

// Store node info.  We save memory by using unused pointer bits as
// flags, avoiding the +8 bytes needed for a separate flags field.
//
// This is bulk-allocated, using cMRouter::newNodeInfo.
//
struct mrNodeInfo
{
    mrNodeInfo()
        {
            ni_nsav = 0;
            ni_nloc = 0;
            ni_stub = 0;
            ni_offs = 0;
        }

    dbNode  *nodeSav()              { return ((dbNode*)(ni_nsav & ~3UL)); }
    void    setNodeSav(dbNode *n)   { ni_nsav =
                                      (unsigned long)n | (ni_nsav & 3); }

    dbNode  *nodeLoc()              { return ((dbNode*)(ni_nloc & ~3UL)); }
    void    setNodeLoc(dbNode *n)   { ni_nloc =
                                      (unsigned long)n | (ni_nloc & 3); }

    lefu_t  stub()                  { return (ni_stub); }
    void    setStub(lefu_t d)       { ni_stub = d; }
    lefu_t  offset()                { return (ni_offs); }
    void    setOffset(lefu_t d)     { ni_offs = d; }

    u_int   flags()                 { return ((ni_nsav & 3) |
                                      ((ni_nloc & 3) << 2)); }
    void    setFlags(u_int f)
        {
            ni_nsav = (ni_nsav & ~3UL) | (f & 3);
            ni_nloc = (ni_nloc & ~3UL) | ((f >> 2) & 3);
        }

private:
    unsigned long ni_nsav;
    unsigned long ni_nloc;
    lefu_t  ni_stub;
    lefu_t  ni_offs;
};

// Per-layer data items used during routing.
//
struct mrLayer
{
    mrLayer()
        {
            obs         = 0;
            obs2        = 0;
            obsinfo     = 0;
            listed      = 0;
            nodeinfo    = 0;
        }

    ~mrLayer()
        {
            delete [] obs;
            delete [] obs2;
            delete [] obsinfo;
            delete [] listed;
            delete [] nodeinfo;
        }

    // Each of these is an nchX*nchY array when allocated.
    u_int   *obs;
    mrProute *obs2;         // Used for pt->pt routes on layer.
    lefu_t  *obsinfo;       // Temp array used for detailed obstruction info.
    bool    *listed;        // Uniqueness flag used while finding routes.
    mrNodeInfo **nodeinfo;  // Stub/offset information.
};

// Point in the wire-channel space.
struct mrGridCell
{
    int     gridx;
    int     gridy;
    u_int   layer;
    u_int   index;      // Offset into nchX*nchY array.
};

// Opaque physical path generator context.
struct sPhysRouteGenCx;

// Main router class.
//
class cMRouter: public cMRif, public cLDDBref
{
public:
    // mrouter.cc
    cMRouter(cLDDBif*);
    ~cMRouter();

    bool    initRouter();
    void    resetRouter();
    bool    doRoute(dbNet*, mrStage, bool);
    int     routeNetRipup(dbNet*, bool);
    int     doFirstStage(bool, int);
    int     doSecondStage(bool, bool);
    int     doThirdStage(bool, int);
    void    printClearFailed();

    // mr_cmds.cc
    bool    readScript(const char*);
    bool    readScript(FILE*);
    bool    doCmd(const char*);
    bool    reset(bool);
    bool    cmdSet(const char*);
    bool    cmdSetcost(const char*);
    bool    cmdUnset(const char*);
    bool    cmdReadCfg(const char*);
    bool    cmdStage1(const char*);
    bool    cmdStage2(const char*);
    bool    cmdStage3(const char*);
    bool    cmdRipUp(const char*);
    bool    cmdFailed(const char*);
    bool    cmdCongested(const char*);

    // mr_config.cc
    int     readConfig(const char*, bool, bool = false);

    // mr_maze.cc
    bool    ripupNet(dbNet*, bool);

    // mr_route.cc
    bool    initPhysRouteGen();
    void    setupRoutePaths();
    bool    setupRoutePath(dbNet*, bool);
    void    clearPhysRouteGen();

    cLDDBif *lddb()                     { return (db); }

    // The following methods are public for use in diagnostics, in
    // particular the graphics system.

    int     ogrid(int x, int y, u_int l) { return (x + y*numChannelsX(l)); }
    void    initGridCell(mrGridCell &c, int x, int y, u_int l)
                {
                    c.gridx = x;
                    c.gridy = y;
                    c.layer = l;
                    c.index = x + y*numChannelsX(l);
                }

    u_int   *obsAry(u_int l)
                { return (mr_layers ? mr_layers[l].obs : 0); }
    u_int   *obsAry(const mrGridCell &c)
                { return (mr_layers ? mr_layers[c.layer].obs : 0); }
    void    setObsAry(u_int i, u_int *v)
                { if (mr_layers) mr_layers[i].obs = v; }
    u_int   obsVal(int x, int y, u_int l)
                { return (obsAry(l) ? obsAry(l)[ogrid(x,y,l)] : 0); }
    void    setObsVal(int x, int y, u_int l, u_int v)
                { if (obsAry(l)) obsAry(l)[ogrid(x,y,l)] = v; }
    u_int   obsVal(const mrGridCell &c)
                { return (obsAry(c) ? obsAry(c)[c.index] : 0); }
    void    setObsVal(const mrGridCell &c, u_int v)
                { if (obsAry(c)) obsAry(c)[c.index] = v; }

    mrProute *obs2Ary(u_int i)
                { return (mr_layers ? mr_layers[i].obs2 : 0); }
    mrProute *obs2Ary(const mrGridCell &c)
                { return (mr_layers ? mr_layers[c.layer].obs2 : 0); }
    void    setObs2Ary(u_int i, mrProute *pr)
                { if (mr_layers) mr_layers[i].obs2 = pr; }
    mrProute *obs2Val(int x, int y, int l)
                { return (obs2Ary(l) ? &obs2Ary(l)[ogrid(x,y,l)] : 0); }
    mrProute *obs2Val(const mrGridCell &c)
                { return (obs2Ary(c) ? &obs2Ary(c)[c.index] : 0); }

    lefu_t  *obsInfoAry(u_int l)
                { return (mr_layers ? mr_layers[l].obsinfo : 0); }
    lefu_t  *obsInfoAry(const mrGridCell &c)
                { return (mr_layers ? mr_layers[c.layer].obsinfo : 0); }
    void    setObsInfoAry(u_int l, lefu_t *v)
                { if (mr_layers) mr_layers[l].obsinfo = v; }
    lefu_t  obsInfoVal(int x, int y, u_int l)
                { return (obsInfoAry(l) ? obsInfoAry(l)[ogrid(x,y,l)] : 0); }
    void    setObsInfoVal(int x, int y, int l, lefu_t v)
                { if (obsInfoAry(l)) obsInfoAry(l)[ogrid(x,y,l)] = v; }
    lefu_t  obsInfoVal(const mrGridCell &c)
                { return (obsInfoAry(c) ? obsInfoAry(c)[c.index] : 0); }
    void    setObsInfoVal(const mrGridCell &c, lefu_t v)
                { if (obsInfoAry(c)) obsInfoAry(c)[c.index] = v; }

    bool    *listedAry(u_int l)
                { return (mr_layers ? mr_layers[l].listed : 0); }
    void    setListedAry(u_int l, bool *v)
                { if (mr_layers) mr_layers[l].listed = v; }
    bool    listed(int x, int y, u_int l)
                { return (listedAry(l) ? listedAry(l)[ogrid(x,y,l)] : false); }
    void    setListed(int x, int y, int l, bool v)
                { if (listedAry(l)) listedAry(l)[ogrid(x,y,l)] = v; }

    mrNodeInfo **nodeInfoAry(u_int l)
                { return (mr_layers ? mr_layers[l].nodeinfo : 0); }
    mrNodeInfo **nodeInfoAry(const mrGridCell &c)
                { return (mr_layers ? mr_layers[c.layer].nodeinfo : 0); }
    void    setNodeInfoAry(u_int l, mrNodeInfo **ni)
                { if (mr_layers) mr_layers[l].nodeinfo = ni; }
    mrNodeInfo *testNodeInfo(int x, int y, int l)
                {
                    if (!nodeInfoAry(l))
                        return (0);
                    mrNodeInfo **p = nodeInfoAry(l) + ogrid(x, y, l);
                    if (!*p)
                        *p = new_nodeInfo();
                    return (*p);
                }
    mrNodeInfo *testNodeInfo(const mrGridCell &c)
                {
                    if (!nodeInfoAry(c))
                        return (0);
                    mrNodeInfo **p = nodeInfoAry(c) + c.index;
                    if (!*p)
                        *p = new_nodeInfo();
                    return (*p);
                }

    dbNode  *nodeSav(int x, int y, int l)
                { return (nodeInfoAry(l) && nodeInfoAry(l)[ogrid(x,y,l)] ?
                  nodeInfoAry(l)[ogrid(x,y,l)]->nodeSav() : 0); }
    void    setNodeSav(int x, int y, int l, dbNode *n)
                { mrNodeInfo *ni = testNodeInfo(x, y, l);
                  if (ni) ni->setNodeSav(n); }
    dbNode  *nodeSav(const mrGridCell &c)
                { return (nodeInfoAry(c) && nodeInfoAry(c)[c.index] ?
                  nodeInfoAry(c)[c.index]->nodeSav() : 0); }
    void    setNodeSav(const mrGridCell &c, dbNode *n)
                { mrNodeInfo *ni = testNodeInfo(c);
                  if (ni) ni->setNodeSav(n); }

    dbNode  *nodeLoc(int x, int y, int l)
                { return (nodeInfoAry(l) && nodeInfoAry(l)[ogrid(x,y,l)] ?
                  nodeInfoAry(l)[ogrid(x,y,l)]->nodeLoc() : 0); }
    void setNodeLoc(int x, int y, int l, dbNode *n)
                { mrNodeInfo *ni = testNodeInfo(x, y, l);
                  if (ni) ni->setNodeLoc(n); }
    dbNode  *nodeLoc(const mrGridCell &c)
                { return (nodeInfoAry(c) && nodeInfoAry(c)[c.index] ?
                  nodeInfoAry(c)[c.index]->nodeLoc() : 0); }
    void    setNodeLoc(const mrGridCell &c, dbNode *n)
                { mrNodeInfo *ni = testNodeInfo(c);
                  if (ni) ni->setNodeLoc(n); }

    lefu_t  stubVal(int x, int y, int l)
                { return (nodeInfoAry(l) && nodeInfoAry(l)[ogrid(x,y,l)] ?
                  nodeInfoAry(l)[ogrid(x,y,l)]->stub() : 0); }
    void    setStubVal(int x, int y, int l, lefu_t n)
                { mrNodeInfo *ni = testNodeInfo(x, y, l);
                  if (ni) ni->setStub(n); }
    lefu_t  stubVal(const mrGridCell &c)
                { return (nodeInfoAry(c) && nodeInfoAry(c)[c.index] ?
                  nodeInfoAry(c)[c.index]->stub() : 0); }
    void    setStubVal(const mrGridCell &c, lefu_t n)
                { mrNodeInfo *ni = testNodeInfo(c);
                  if (ni) ni->setStub(n); }

    lefu_t  offsetVal(int x, int y, int l)
                { return (nodeInfoAry(l) && nodeInfoAry(l)[ogrid(x,y,l)] ?
                  nodeInfoAry(l)[ogrid(x,y,l)]->offset() : 0); }
    void    setOffsetVal(int x, int y, int l, lefu_t n)
                { mrNodeInfo *ni = testNodeInfo(x, y, l);
                  if (ni) ni->setOffset(n); }
    lefu_t  offsetVal(const mrGridCell &c)
                { return (nodeInfoAry(c) && nodeInfoAry(c)[c.index] ?
                  nodeInfoAry(c)[c.index]->offset() : 0); }
    void    setOffsetVal(const mrGridCell &c, lefu_t n)
                { mrNodeInfo *ni = testNodeInfo(c);
                  if (ni) ni->setOffset(n); }

    u_int  flagsVal(int x, int y, int l)
                { return (nodeInfoAry(l) && nodeInfoAry(l)[ogrid(x,y,l)] ?
                  nodeInfoAry(l)[ogrid(x,y,l)]->flags() : 0); }
    void    setFlagsVal(int x, int y, int l, u_int n)
                { mrNodeInfo *ni = testNodeInfo(x, y, l);
                  if (ni) ni->setFlags(n); }
    u_int  flagsVal(const mrGridCell &c)
                { return (nodeInfoAry(c) && nodeInfoAry(c)[c.index] ?
                  nodeInfoAry(c)[c.index]->flags() : 0); }
    void    setFlagsVal(const mrGridCell &c, u_int n)
                { mrNodeInfo *ni = testNodeInfo(c);
                  if (ni) ni->setFlags(n); }

    bool    hasRmask()                  { return (mr_rmask != 0); }
    u_char  rmask(int x, int y)         { return (mr_rmask[ogrid(x,y,0)]); }
    void    setRmask(int x, int y, u_int v) { mr_rmask[ogrid(x,y,0)] = v; }
    u_char   *rmaskIncs()               { return (mr_rmaskIncs); }
    u_int   rmaskIncsSz()               { return (mr_rmaskIncsSz); }
    void    setRmaskIncs(u_char *a, u_int sz)
        {
            delete [] mr_rmaskIncs;
            mr_rmaskIncs = a;
            mr_rmaskIncsSz = sz;
        }
    dbNetList *failedNets()             { return (mr_failedNets.list()); }

    u_int   totalRoutes()               { return (mr_totalRoutes); }
    u_int   maskVal()                   { return (mr_mask); }
    void    setMaskVal(u_int m)         { mr_mask = m; }
    u_int   pinLayers()                 { return (mr_pinLayers); }
    void    setPinLayers(u_int n)       { mr_pinLayers = n; }
    mrNetOrder netOrder()               { return (mr_net_order); }
    void    setNetOrder(mrNetOrder n)   { mr_net_order = n; }


    int     segCost()                   { return (mr_segCost); }
    void    setSegCost(int c)           { mr_segCost = c; }
    int     viaCost()                   { return (mr_viaCost); }
    void    setViaCost(int c)           { mr_viaCost = c; }
    int     jogCost()                   { return (mr_jogCost); }
    void    setJogCost(int c)           { mr_jogCost = c; }
    int     xverCost()                  { return (mr_xverCost); }
    void    setXverCost(int c)          { mr_xverCost = c; }
    int     blockCost()                 { return (mr_blockCost); }
    void    setBlockCost(int c)         { mr_blockCost = c; }
    int     offsetCost()                { return (mr_offsetCost); }
    void    setOffsetCost(int c)        { mr_offsetCost = c; }
    int     conflictCost()              { return (mr_conflictCost); }
    void    setConflictCost(int c)      { mr_conflictCost = c; }

    u_int   numPasses()                 { return (mr_numPasses); }
    void    setNumPasses(u_int n)       { mr_numPasses = n; }
    int     stackedVias()               { return (mr_stackedVias); }
    void    setStackedVias(int n)       { mr_stackedVias = n; }
    VIA_PATTERN viaPattern()            { return ((VIA_PATTERN)mr_viaPattern); }
    void    setViaPattern(VIA_PATTERN v) { mr_viaPattern = v; }

    bool    forceRoutable()             { return (mr_forceRoutable); }
    void    setForceRoutable(bool b)    { mr_forceRoutable = b; }
    u_int   keepTrying()                { return (mr_keepTrying); }
    void    setKeepTrying(u_int i)      { mr_keepTrying = i; }
    u_int   mapType()                   { return (mr_mapType); }
    void    setMapType(u_int t)         { mr_mapType = t; }
    u_int   ripLimit()                  { return (mr_ripLimit); }
    void    setRipLimit(u_int i)        { mr_ripLimit = i; }

    void    registerGraphics(mrGraphics *g) { mr_graphics = g; }

private:
    // mrouter.cc
    void    create_net_order();
    dbNet   *get_net_to_route(int);
    int     ripup_colliding(dbNet*);
    void    create_vbranch_mask(int, int, int, int, int);
    void    create_hbranch_mask(int, int, int, int, int);
    void    create_bbox_mask(dbNet*, int);
    int     analyze_congestion(int, int, int, int, int);
    void    create_mask(dbNet*, int, int);
    void    fill_mask(int);
    void    unable_to_route(const char*, dbNode*, bool);
    mrRval  next_route_setup(mrRouteInfo*, mrStage);
    mrRval  route_setup(mrRouteInfo*, mrStage, u_int*);
    mrRval  route_segs(mrRouteInfo*, mrStage, bool);
    void    printFlags(const char*);
    mrNodeInfo *new_nodeInfo();
    void    clear_nodeInfo();

    // mr_maze.cc
    dbNode  *find_unrouted_node(dbNet*);
    mrRval  set_powerbus_to_net(int);
    void    clear_non_source_targets(dbNet*, mrStack*);
    void    clear_target_node(dbNode*);
    int     count_targets(dbNet*);
    mrRval  set_node_to_net(dbNode*, int, mrStack*, dbSeg*, mrStage);
    bool    disable_node_nets(dbNode*);
    mrRval  set_route_to_net(dbNet*, dbRoute*, int, mrStack*, dbSeg*, mrStage);
    mrRval  set_routes_to_net(dbNet*, int, mrStack*, dbSeg*, mrStage);
    bool    add_colliding_net(dbNetList**, u_int);
    dbNetList *find_colliding(dbNet*, int*);
    bool    eval_pt(mrGridP*, u_char, mrStage);
    void    writeback_segment(dbSeg*, int);
    mrRval  commit_proute(dbRoute*, mrGridP*, mrStage);
    bool    writeback_route(dbRoute*);
    bool    writeback_all_routes(dbNet*);

    // mr_node.cc
    void    expand_tap_geometry();
    void    expand_tap_geometry(dbGate*, int);
    void    clip_gate_taps();
    void    create_obstructions_from_gates();
    void    create_obstructions_from_gates(dbGate*, dbDseg*);
    void    create_obstructions_from_gates(dbGate*, int);
    void    create_obstructions_from_list(dbDseg*);
    void    create_obstructions_inside_nodes();
    void    create_obstructions_inside_nodes(dbGate*, int);
    void    create_obstructions_outside_nodes();
    void    create_obstructions_outside_nodes(dbGate*, int, lefu_t*, lefu_t*);
    void    tap_to_tap_interactions();
    void    tap_to_tap_interactions(dbGate*, int);
    void    create_obstructions_from_variable_pitch();
    void    adjust_stub_lengths();
    void    adjust_stub_lengths(dbGate*, int);
    void    find_route_blocks();
    void    find_route_blocks(dbGate*, int);
    void    count_reachable_taps();
    void    count_reachable_taps(dbGate*, int);
    void    count_pinlayers();
    void    make_routable(dbNode*);
    void    make_routable(dbGate*, int);
    void    block_route(int, int, int, u_char);
    void    find_bounding_box(dbNet*);
    void    define_route_tree(dbNet*);
    void    disable_gridpos(int, int, int);
    void    check_obstruct(int, int, dbDseg*, lefu_t, lefu_t);
    lefu_t  get_via_clear(int, ROUTE_DIR, dbDseg*);
    lefu_t  get_route_clear(int, dbDseg*);

    // mr_route.cc
    void    check_first_offset(bool);
    void    check_offset_terminals(bool);
    void    check_last_offset(bool);
    void    pathstart(int, lefu_t, lefu_t);
    void    pathto(lefu_t, lefu_t, ROUTE_DIR, lefu_t, lefu_t);
    void    pathvia(int, lefu_t, lefu_t, lefu_t, lefu_t, int, int);
    void    pathstub(int, lefu_t, lefu_t, lefu_t, lefu_t, ROUTE_DIR);
    void    cleanup_net(dbNet*);

    // numNets + MIN_NET_NUMBER is guaranteed to be greater than
    // the highest number assigned to a net.
    u_int maxNetNum()               { return (numNets() + LD_MIN_NETNUM); }

    mrLayer *mr_layers;
    dbNet   **mr_nets;              // Array of nets to route, ordered.
    u_char  *mr_rmask;              // The mask, constrains possible routes.
    u_char  *mr_rmaskIncs;          // How much mask bloats with pass.
    dbNet   *mr_curNet;             // Current net to route, used by 2nd stage.
    mrNetList mr_failedNets;        // List of nets that failed to route.
    sPhysRouteGenCx *mr_route_gen;  // Physical route generator context.

    u_int   mr_totalRoutes;
    u_int   mr_mask;                // MASK_TYPE or small integer.
    u_int   mr_pinLayers;           // Number of layers containing pin info.
    mrNetOrder mr_net_order;        // Net ordering algorithm to use.

    struct niBlk *mr_ni_blks;       // mrNodeInfo memory management.
    int     mr_ni_cnt;              // mrNodeInfo memory management.

    short   mr_segCost;             // Route cost of a segment.                 
    short   mr_viaCost;             // Cost of via between adjacent layers.
    short   mr_jogCost;             // Cost of 1 grid off-direction jog.
    short   mr_xverCost;            // Cost of a crossover.
    short   mr_blockCost;           // Cost of a crossover when node has
                                    //  only one tap point.
    short   mr_offsetCost;          // Cost per micron of a node offset.
    short   mr_conflictCost;        // Cost of shorting another route

    u_short mr_numPasses;           // Number of times to iterate route.
    u_short mr_stackedVias;         // Number of vias that can be stacked.
    short   mr_viaPattern;          // Type of via patterning to use.

    int     mr_stepnet;             // Single-stepping counter.
    bool    mr_initialized;
    bool    mr_forceRoutable;
    u_char  mr_keepTrying;
    u_char  mr_mapType;
    u_char  mr_ripLimit;            // Fail net rather than rip up more than
                                    // this number of other nets.
    u_char  mr_rmaskIncsSz;         // Size of mask increments list.
    mrGraphics *mr_graphics;        // optional graphics object.
};

#endif

