
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
 $Id: lddb.h,v 1.19 2017/02/17 05:44:36 stevew Exp $
 *========================================================================*/

#ifndef LDDB_H
#define LDDB_H

#include "ld_vers.h"
#include "miscutil/lstring.h"

//
// LEF/DEF Database.
//
// The LEF/DEF database class cLDDB holds data from Layout Exchange
// Format (LEF) and Design Exchange Format (DEF) files, as used by the
// router.  This provides needed definitions and the interface, for
// export.
//

#ifdef WIN32
typedef unsigned int    u_int;
typedef unsigned short  u_short;
typedef unsigned char   u_char;
#endif

// We save coordinates and lengths as integers in LEF units.  We give
// such values a special type, to avoid confusion with coordinates in
// DEF or channel space.
typedef int             lefu_t;

// This is used for channel grid coordinates.  Since coordinates are
// non-negative, and a maze router is not likely to support more than
// 16536 channels, we use shorts, which saves memory.
typedef u_short         grdu_t;
#define LD_MAX_CHANNELS 65536
// We can use unsigned, I think.  Define below if reverting to signed.
//#define LD_SIGNED_GRID

#define LD_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define LD_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define LD_ABSDIFF(x, y) (((x) > (y)) ? ((x) - (y)) : ((y) - (x)))

#define LD_OK   false
#define LD_BAD  true

// Turn on allocation counters for debugging.
// WARNING: this breaks the plug-in capability, as the static fields
// don't resolve across the interface.
#define LD_MEMDBG

// Path segment information, for dbSeg::segtype.
#define ST_WIRE         0x01
#define ST_VIA          0x02
#define ST_OFFSET_START 0x04    // (x1, y1) is offset from grid
#define ST_OFFSET_END   0x08    // (x2, y2) is offset from grid

// A route segment, coordinates are route grid indices.
//
struct dbSeg
{
    dbSeg()
        {
            next        = 0;
            x1          = 0;
            y1          = 0;
            x2          = 0;
            y2          = 0;
            segtype     = 0;
            layer       = -1;
            lefId       = -1;
#ifdef LD_MEMDBG
            seg_cnt++;
            if (seg_cnt > seg_hw)
                seg_hw = seg_cnt;
#endif
        }

    dbSeg(int l, int id, u_int st, int tx1, int ty1, int tx2, int ty2,
        dbSeg *n)
        {
            next        = n;
            x1          = tx1;
            y1          = ty1;
            x2          = tx2;
            y2          = ty2;
            segtype     = st;
            layer       = l;
            lefId       = id;
#ifdef LD_MEMDBG
            seg_cnt++;
            if (seg_cnt > seg_hw)
                seg_hw = seg_cnt;
#endif
        }

#ifdef LD_MEMDBG
    ~dbSeg()
        {
            seg_cnt--;
        }
#endif

    static void destroy(dbSeg *s)
        {
            while (s) {
                dbSeg *sx = s;
                s = s->next;
                delete sx;
            }
        }

    dbSeg   *next;
    grdu_t  x1, y1, x2, y2;
    u_short segtype;
    short   layer;
    short   lefId;
#ifdef LD_MEMDBG
    static int seg_cnt;
    static int seg_hw;
#endif
};

// dbDseg is like a dbSeg, but coordinates are in LEF units (therefore
// type lefu_t).
//
struct dbDseg
{
    dbDseg()
        {
            next        = 0;
            x1          = 0;
            y1          = 0;
            x2          = 0;
            y2          = 0;
            segtype     = 0;
            layer       = -1;
            lefId       = -1;
#ifdef LD_MEMDBG
            dseg_cnt++;
            if (dseg_cnt > dseg_hw)
                dseg_hw = dseg_cnt;
#endif
        }

    dbDseg(lefu_t tx1, lefu_t ty1, lefu_t tx2, lefu_t ty2, int l, int id,
        dbDseg *n)
        {
            next        = n;
            x1          = tx1;
            y1          = ty1;
            x2          = tx2;
            y2          = ty2;
            segtype     = 0;
            layer       = l;
            lefId       = id;
#ifdef LD_MEMDBG
            dseg_cnt++;
            if (dseg_cnt > dseg_hw)
                dseg_hw = dseg_cnt;
#endif
        }

#ifdef LD_MEMDBG
    ~dbDseg()
        {
            dseg_cnt--;
        }
#endif

    static void destroy(dbDseg *ds)
        {
            while (ds) {
                dbDseg *dx = ds;
                ds = ds->next;
                delete dx;
            }
        }

    dbDseg  *next;
    lefu_t  x1, y1, x2, y2;
    u_short segtype;    // not used
    short   layer;
    int     lefId;
#ifdef LD_MEMDBG
    static int dseg_cnt;
    static int dseg_hw;
#endif
};

// Like a dbDseg but without the next pointer or segtype.
// 
//
struct dbDsegB
{
    dbDsegB()
        {
            x1          = 0;
            y1          = 0;
            x2          = 0;
            y2          = 0;
            layer       = -1;
            lefId       = -1;
        }

    dbDsegB(lefu_t tx1, lefu_t ty1, lefu_t tx2, lefu_t ty2, int l, int id)
        {
            x1          = tx1;
            y1          = ty1;
            x2          = tx2;
            y2          = ty2;
            layer       = l;
            lefId       = id;
        }

    lefu_t  x1, y1, x2, y2;
    int     layer;
    int     lefId;
};

// dbDpoint is a point location with coordinates given *both* as an
// integer (for the grid-based routing) and as LEF units.
//
struct dbDpoint
{
    dbDpoint(lefu_t xx, lefu_t yy, int l, int id, dbDpoint *n)
        {
            next        = n;
            x           = xx;
            y           = yy;
            gridx       = 0;
            gridy       = 0;
            layer       = l;
            lefId       = id;
#ifdef LD_MEMDBG
            dpoint_cnt++;
            if (dpoint_cnt > dpoint_hw)
                dpoint_hw = dpoint_cnt;
#endif
        }

#ifdef LD_MEMDBG
    ~dbDpoint()
        {
            dpoint_cnt--;
        }
#endif

    static void destroy(dbDpoint *dp)
        {
            while (dp) {
                dbDpoint *dx = dp;
                dp = dp->next;
                delete dx;
            }
        }

    dbDpoint *next;
    lefu_t  x, y;
    grdu_t  gridx, gridy;
    short   layer;
    short   lefId;
#ifdef LD_MEMDBG
    static int dpoint_cnt;
    static int dpoint_hw;
#endif
};

// Path element for physical route, can be a via placement or net wire
// vertex.  These are created by the router for routed nets, or read
// in from a DEF file with physical nets included.  The DEF writer uses
// this when creating output.
//
struct dbPath
{
    dbPath(lefu_t xx, lefu_t yy)
        {
            next    = 0;
            x       = xx;
            y       = yy;
            width   = 0;
            layer   = -1;
            vid     = -1;
#ifdef LD_MEMDBG
            path_cnt++;
            if (path_cnt > path_hw)
                path_hw = path_cnt;
#endif
        }

#ifdef LD_MEMDBG
    ~dbPath()
        {
            path_cnt--;
        }
#endif

    static void destroy(dbPath *e)
        {
            while (e) {
                dbPath *ex = e;
                e = e->next;
                delete ex;
            }
        }

    dbPath  *next;
    lefu_t  x;
    lefu_t  y;
    lefu_t  width;      // Width if stub.
    short   layer;      // Layer index or -1.
    short   vid;        // Via lefId.
#ifdef LD_MEMDBG
    static int path_cnt;
    static int path_hw;
#endif
};

// Bit values for dbRoute::flags.
#define RT_OUTPUT       0x1     // Route has been output.
#define RT_STUB         0x2     // Route has at least one stub route.

struct dbRoute
{
    dbRoute()
        {
            next        = 0;
            segments    = 0;
            netnum      = 0;
            flags       = 0;
#ifdef LD_MEMDBG
            route_cnt++;
            if (route_cnt > route_hw)
                route_hw = route_cnt;
#endif
        }

    dbRoute(dbSeg *s, u_int nn, u_int f, dbRoute *n)
        {
            next        = n;
            segments    = s;
            netnum      = nn;
            flags       = f;
#ifdef LD_MEMDBG
            route_cnt++;
            if (route_cnt > route_hw)
                route_hw = route_cnt;
#endif
        }

    ~dbRoute()
        {
            dbSeg::destroy(segments);
#ifdef LD_MEMDBG
            route_cnt--;
#endif
        }

    static void destroy(dbRoute *rt)
        {
            while (rt) {
                dbRoute *rx = rt;
                rt = rt->next;
                delete rx;
            }
        }

    dbRoute *next;
    dbSeg   *segments;      // Computed routes.
    u_int   netnum;         // Unique id of net.
    u_int   flags;          // See below for flags.
#ifdef LD_MEMDBG
    static int route_cnt;
    static int route_hw;
#endif
};


struct dbNode
{
    dbNode()
        {
            next        = 0;
            taps        = 0;
            extend      = 0;
            netnum      = 0;
            numnodes    = 0;
            nodenum     = 0;
            gorpnum     = -1;
            pinindx     = 0;
            numtaps     = 0;
            branchx     = 0;
            branchy     = 0;
#ifdef LD_MEMDBG
            node_cnt++;
            if (node_cnt > node_hw)
                node_hw = node_cnt;
#endif
        }

    ~dbNode()
        {
            dbDpoint::destroy(taps);
            dbDpoint::destroy(extend);
#ifdef LD_MEMDBG
            node_cnt--;
#endif
        }

    static void destroy(dbNode *nd)
        {
            while (nd) {
                dbNode *nx = nd;
                nd = nd->next;
                delete nx;
            }
        }

    // The "gorpnum" is the index into the gates or pins array left
    // shifted by one, with the LSB set to one for pins, zero for
    // gates.

    bool is_pin()           { return (gorpnum & 1); }

    dbNode  *next;
    dbDpoint *taps;         // Point position for node taps.
    dbDpoint *extend;       // Point position within halo of the tap.
    u_int   netnum;         // Number of net this node belongs to.
    u_int   numnodes;       // Number of nodes on this net.
    int     nodenum;        // Node ordering within its net.
    int     gorpnum;        // Gate or pin identifier, see note above.
    u_short pinindx;        // One-based index of node in master pins list.
    u_short numtaps;        // Number of actual reachable taps.
    grdu_t  branchx;        // Position of the node branch in x.
    grdu_t  branchy;        // Position of the node branch in y.
#ifdef LD_MEMDBG
    static int node_cnt;
    static int node_hw;
#endif
};

enum PORT_CLASS
{
    PORT_CLASS_UNSET,
    PORT_CLASS_INPUT,
    PORT_CLASS_TRISTATE,
    PORT_CLASS_OUTPUT,
    PORT_CLASS_BIDIRECTIONAL,
    PORT_CLASS_FEEDTHROUGH
};

enum PORT_USE
{
    PORT_USE_UNSET,
    PORT_USE_SIGNAL,
    PORT_USE_ANALOG,
    PORT_USE_POWER,
    PORT_USE_GROUND,
    PORT_USE_CLOCK
};

enum PORT_SHAPE
{
    PORT_SHAPE_UNSET,
    PORT_SHAPE_ABUTMENT,
    PORT_SHAPE_RING,
    PORT_SHAPE_FEEDTHRU
};

// Pin list, saved in lefMacro.
//
struct lefPin
{
    lefPin(char *s, dbDseg *g, int d, int u, int h, lefPin *n)
        {
            next        = n;
            name        = s;
            geom        = g;
            direction   = d;
            use         = u;
            shape       = h;
        }

    ~lefPin()
        {
            delete [] name;
            dbDseg::destroy(geom);
        }

    static void destroy(lefPin *p)
        {
            while (p) {
                lefPin *px = p;
                p = p->next;
                delete px;
            }
        }

    lefPin  *next;
    char    *name;
    dbDseg  *geom;
    char    direction;      // PORT_CLASS
    char    use;            // PORT_USE
    char    shape;          // PORT_SHAPE
};

// Values for lefMacro::mclass and lefMacro::subclass.
enum MACRO_CLASS
{
    MACRO_CLASS_UNSET,
    MACRO_CLASS_COVER,
    MACRO_CLASS_BLOCK,
    MACRO_CLASS_PAD,
    MACRO_CLASS_CORE,
    MACRO_CLASS_ENDCAP,
    MACRO_CLASS_BUMP,
    MACRO_CLASS_BLACKBOX,
    MACRO_CLASS_SOFT,
    MACRO_CLASS_INPUT,
    MACRO_CLASS_OUTPUT,
    MACRO_CLASS_INOUT,
    MACRO_CLASS_POWER,
    MACRO_CLASS_SPACER,
    MACRO_CLASS_AREAIO,
    MACRO_CLASS_FEEDTHRU,
    MACRO_CLASS_TIEHIGH,
    MACRO_CLASS_TIELOW,
    MACRO_CLASS_ANTENNACELL,
    MACRO_CLASS_WELLTAP,
    MACRO_CLASS_PRE,
    MACRO_CLASS_POST,
    MACRO_CLASS_TOPLEFT,
    MACRO_CLASS_TOPRIGHT,
    MACRO_CLASS_BOTTOMLEFT,
    MACRO_CLASS_BOTTOMRIGHT,
    MACRO_CLASS_INTERNAL
};

// Macro symmetry bits.
#define SYMMETRY_X  0x1
#define SYMMETRY_Y  0x2
#define SYMMETRY_90 0x4

// Orientation code, same as DEF.
enum ORIENT_CODE
{
    ORIENT_NORTH,
    ORIENT_WEST,
    ORIENT_SOUTH,
    ORIENT_EAST,
    ORIENT_FLIPPED_NORTH,
    ORIENT_FLIPPED_WEST,
    ORIENT_FLIPPED_SOUTH,
    ORIENT_FLIPPED_EAST
};

// Contents of a FOREIGN macro record.
struct dbForeign
{
    dbForeign(char *n, lefu_t x, lefu_t y, ORIENT_CODE o)
        {
            next        = 0;
            name        = n;
            originX     = x;
            originY     = y;
            orient      = o;
        }

    ~dbForeign()
        {
            delete [] name;
        }

    static void destroy(dbForeign *f)
        {
            while (f) {
                dbForeign *fx = f;
                f = f->next;
                delete fx;
            }
        }

    dbForeign   *next;
    char        *name;
    lefu_t      originX;
    lefu_t      originY;
    ORIENT_CODE orient;
};

// Standard cell macro definition.
//
struct lefMacro
{
    lefMacro(char *nm, lefu_t w, lefu_t h)
        {
            gatename    = nm;
            nodes       = 0;
            mclass      = MACRO_CLASS_UNSET;
            subclass    = MACRO_CLASS_UNSET;
            symmetry    = 0;
            width       = w;
            height      = h;
            placedX     = 0;
            placedY     = 0;
            foreign     = 0;
            sitename    = 0;
            pins        = 0;
            obs         = 0;
        }

    ~lefMacro()
        {
            delete [] gatename;
            delete foreign;
            delete [] sitename;
            lefPin::destroy(pins);
            dbDseg::destroy(obs);
        }

    char    *gatename;                  // Name of instance.
    int     nodes;                      // Number of nodes on this gate.
    u_char  mclass;                     // Macro class and sub-class.
    u_char  subclass;
    u_char  symmetry;                   // Symmetry flags.
    lefu_t  width, height;              // Size.
    lefu_t  placedX, placedY;           // Origin.           
    dbForeign *foreign;                 // FOREIGN record.
    char    *sitename;                  // SITE name;
    lefPin  *pins;                      // List of pins.
    dbDseg  *obs;                       // List of obstructions in gate.
};

// The "status" field of pins and gates.
enum LD_STATUS { LD_NOLOC, LD_COVER, LD_FIXED, LD_PLACED, LD_UNPLACED }; 

// These are instances of lefMacro in the netlist.
//
struct dbGate
{
    dbGate(char *nm, lefMacro *c)
        {
            next        = 0;
            gatename    = nm;
            gatetype    = c;
            nodes       = 0;
            orient      = 0;
            placed      = LD_NOLOC;
            obs         = 0;
            width       = 0;
            height      = 0;
            placedX     = 0;
            placedY     = 0;
            node        = 0;
            netnum      = 0;
            noderec     = 0;
            taps        = 0;
            if (c) {
                width       = c->width;
                height      = c->height;
                nodes       = c->nodes;
                node        = new const char*[nodes];
                netnum      = new u_int[nodes];
                noderec     = new dbNode*[nodes];
                taps        = new dbDseg*[nodes];
                memset(node,    0, nodes*sizeof(const char*));
                memset(netnum,  0, nodes*sizeof(u_int));
                memset(noderec, 0, nodes*sizeof(dbNode*));
                memset(taps,    0, nodes*sizeof(dbDseg*));
            }
        }

    ~dbGate()
        {
            delete [] gatename;
            dbDseg::destroy(obs);
            delete [] node;
            delete [] netnum;
            delete [] noderec;
            for (int i = 0; i < nodes; i++)
                dbDseg::destroy(taps[i]);
            delete [] taps;
        }

    static void destroy(dbGate *g)
        {
            while (g) {
                dbGate *gx = g;
                g = g->next;
                delete gx;
            }
        }

    // Orientation code utilities.
    static ORIENT_CODE orientation(const char*);
    static const char *orientationStr(int);

    dbGate  *next;
    char    *gatename;                  // Name of instance.
    lefMacro *gatetype;                 // Pointer to macro record.
    int     nodes;                      // Number of nodes on this gate.
    short   orient;                     // ORIENT_CODE
    short   placed;                     // LD_STATUS,
    dbDseg  *obs;                       // List of obstructions in gate.
    lefu_t  width, height;
    lefu_t  placedX;                 
    lefu_t  placedY;
    const char **node;                  // Names of the pins on this gate.
    u_int   *netnum;                    // Net number connected to each pin.
    dbNode  **noderec;                  // Node record for each pin.
    dbDseg  **taps;                     // List of gate node locs and levels.
};

struct dbNet;

// List of nets, used to maintain a list of failed routes.
struct dbNetList
{
    dbNetList(dbNet *nt, dbNetList *nx)
        {
            next        = nx;
            net         = nt;
#ifdef LD_MEMDBG
            netlist_cnt++;
            if (netlist_cnt > netlist_hw)
                netlist_hw = netlist_cnt;
#endif
        }

#ifdef LD_MEMDBG
    ~dbNetList()
        {
            netlist_cnt--;
        }
#endif

    static void destroy(dbNetList *nptr)
        {
            while (nptr) {
                dbNetList *nx = nptr;
                nptr = nptr->next;
                delete nx;
            }
        }

    // Count the number of entries in a simple linked list.
    //
    static u_int countlist(const dbNetList *nptr)
        {
            u_int count = 0;
            while (nptr) {
                count++;
                nptr = nptr->next;
            }
            return (count);
        }

    dbNetList *next;
    dbNet   *net;
#ifdef LD_MEMDBG
    static int netlist_cnt;
    static int netlist_hw;
#endif
};

// The maximum number of global (power/ground) nets.
#define LD_MAX_GLOBALS  6

// This is the smallest net number allowed for a non-global net. 
// Smaller nonzero values are reserved for global nets that are not
// found in the DEF NETS list.  A zero net number is not valid.
//
#define LD_MIN_NETNUM   (LD_MAX_GLOBALS + 1)

// Bit values for dbNet::flags.
#define NET_PENDING         0x1     // Pending being placed on "abandoned" list.
#define NET_CRITICAL        0x2     // Net is in CriticalNet list.
#define NET_IGNORED         0x4     // Net is ignored by router.
#define NET_STUB            0x8     // Net has at least one stub.
#define NET_VERTICAL_TRUNK  0x10    // Trunk line is (preferred) vertical.
#define NET_CLEANUP         0x20    // Cleanup operation was called.
#define NET_GLOBAL          0x40    // Vdd or Gnd net.
#define NET_BULK_ROUTED     0x80    // Routes allocated in bundle.

// Structure for a network to be routed.
struct dbNet
{
    dbNet()
        {
            netname     = 0;
            netnodes    = 0;
            netnum      = 0;
            numnodes    = 0;
            flags       = 0;
            netorder    = 0;
            xmin = ymin = 0;
            xmax = ymax = 0;
            trunkx      = 0;
            trunky      = 0;
            noripup     = 0;
            routes      = 0;
            path        = 0;
            spath       = 0;
        }

    ~dbNet()
        {
            delete [] netname;
            dbNode::destroy(netnodes);
            dbNetList::destroy(noripup);
            clear_routes();
        }

    void clear_routes()
        {
            if (flags & NET_BULK_ROUTED) {
                delete [] (char*)routes;
                routes = 0;
                flags &= ~NET_BULK_ROUTED;
            }
            else {
                dbRoute::destroy(routes);
                routes = 0;
            }
            dbPath::destroy(path);
            path = 0;
            dbPath::destroy(spath);
            spath = 0;
        }

    // Copy the route list into a new one, where all components are
    // allocated in one memory block.  This should be deleted by
    // casting the pointer do char*, i.e.,
    //   delete [] (char*)net->routes;
    // This should have memory overhead and cache locality advantages.
    // The router can call this once the routes have been identified.
    //
    void realloc_routes()
        {
            if (routes) {
                int nroutes = 0;
                int nsegs = 0;
                for (dbRoute *rt = routes; rt; rt = rt->next) {
                    nroutes++;
                    for (dbSeg *s = rt->segments; s; s = s->next)
                        nsegs++;
                }
                size_t szroute = sizeof(dbRoute);
                if (szroute % sizeof(void*))
                    szroute += 4;
                size_t szseg = sizeof(dbSeg);
                if (szseg % sizeof(void*))
                    szseg += 4;
                size_t sz = nroutes * szroute + nsegs * szseg;
                char *base = (char*)malloc(sz);
                char *b = base;

                dbRoute *prt = 0;
                for (dbRoute *rt = routes; rt; rt = rt->next) {
                    dbRoute *rnew = (dbRoute*)b;
                    b += szroute;
                    *rnew = *rt;
                    rnew->next = 0;
                    dbSeg *psg = 0;
                    for (dbSeg *s = rt->segments; s; s = s->next) {
                        dbSeg *snew = (dbSeg*)b;
                        b += szseg;
                        *snew = *s;
                        snew->next = 0;
                        if (psg)
                            psg->next = snew;
                        else
                            rnew->segments = snew;
                        psg = snew;
                    }
                    if (prt)
                        prt->next = rnew;
                    prt = rnew;
                }
                dbRoute::destroy(routes);
                routes = (dbRoute*)base;
                flags |= NET_BULK_ROUTED;
            }
        }

    char    *netname;
    dbNode  *netnodes;      // List of nodes connected to the net.
    u_int   netnum;         // A unique number for this net.
    int     numnodes;       // Number of nodes connected to the net.
    u_int   flags;          // Flags for this net.
    int     netorder;       // To be assigned by route strategy (critical
                            //  nets first, then order by number of nodes).
    grdu_t  xmin, ymin;     // Bounding box lower left corner.
    grdu_t  xmax, ymax;     // Bounding box upper right corner.
    grdu_t  trunkx;         // X position of the net's trunk line (see flags).
    grdu_t  trunky;         // Y position of the net's trunk line (see flags).
    dbNetList *noripup;     // List of nets that have been ripped up to
                            //  route this net.  This will not be allowed
                            //  a second time, to avoid looping.
    // Warning: use clear_routes to free the routes field.
    dbRoute *routes;        // Routes for this net.
    dbPath  *path;          // Physical net description of routes.
    dbPath  *spath;         // Physical special net description of routes.
};

// Layer identification, by name and/or layer/purpose or layer/datatype
// numbers.
//
struct dbLayerId
{
    dbLayerId()
        {
            lname       = 0;
            layer       = 0;
            dtype       = 0;
            lefId       = -1;
        }

    ~dbLayerId()
        {
            delete [] lname;
        }

    void set_layername(const char *nm)
        {
            char *t = lstring::copy(nm);
            delete [] lname;
            lname = t;
        }

    char    *lname;     // Layer name.
    u_short layer;      // Layer number, user supplied (e.g., GDSII layer);
    u_short dtype;      // Datatype number, user supplied.
    int     lefId;      // Corresponding LEF object index.
};

// Definitions of bits in dbLayer::needBlock.
#define ROUTEBLOCKX     1           // Block adjacent routes in X.
#define ROUTEBLOCKY     2           // Block adjacent routes in Y.
#define VIABLOCKX       4           // Block adjacent vias in X.
#define VIABLOCKY       8           // Block adjacent vias in Y.

// Properties that apply to routing levels.
//
struct dbLayer
{
    dbLayer()
        {
            pathWidth   = 0;
            startX      = 0;
            startY      = 0;
            pitchX      = 0;
            pitchY      = 0;
            numChanX    = 0;
            numChanY    = 0;
            viaXid      = -1;
            viaYid      = -1;
            haloX       = 0;
            haloY       = 0;
            vert        = false;
            trackXset   = false;
            trackYset   = false;
            needBlock   = 0;
        }

    lefu_t  pathWidth;      // Routing wire width.
    lefu_t  startX;         // Origin value.
    lefu_t  startY;         // Origin value.
    lefu_t  pitchX;         // Routing pitch X.
    lefu_t  pitchY;         // Routing pitch Y.
    u_int   numChanX;       // Channel count X.
    u_int   numChanY;       // Channel count Y.
    int     viaXid;         // LEF ID of via this layer is lower metal for.
    int     viaYid;         // Only one of these is set, depends on via shape.
    lefu_t  haloX;          // Exclusion area.
    lefu_t  haloY;          // Exclusion area.
    bool    vert;           // True if vertical orientation preferred.
    bool    trackXset;      // Have seen a DEF TRACK X line for layer.
    bool    trackYset;      // Have seen a DEF TRACK Y line for layer.
    u_char  needBlock;
    dbLayerId lid;          // Layer identification.
};

// Defined types for lefClass in the lefObject structure.
//
enum lefCLASS
{
    CLASS_ROUTE     = 0,    // ROUTING layer
    CLASS_CUT,              // CUT layer
    CLASS_IMPLANT,          // IMPLANT layer
    CLASS_MASTER,           // MASTERSLICE layer
    CLASS_OVERLAP,          // OVERLAP layer
    CLASS_VIA,              // VIA object
    CLASS_VIARULE,          // VIARULE object
    CLASS_IGNORE            // inactive object
};

// Structure to hold information about spacing rules.
//
struct lefSpacingRule
{
    lefSpacingRule(lefu_t w, lefu_t s, lefSpacingRule *n)
        {
            next        = n;
            width       = w;
            spacing     = s;
        }

    static void destroy(lefSpacingRule *sr)
        {
            while (sr) {
                lefSpacingRule *sx = sr;
                sr = sr->next;
                delete sx;
            }
        }

    lefSpacingRule *next;
    lefu_t  width;      // Width, in microns.
    lefu_t  spacing;    // Minimum spacing rule, in microns.
};

// Routing direction.
enum ROUTE_DIR { DIR_VERT, DIR_HORIZ };

// Structure used to maintain default routing information for each
// routable layer type.
//
struct lefRoute
{
    lefRoute()
        {
            spacing     = 0;
            width       = 0;
            pitchX      = 0;
            pitchY      = 0;
            offsetX     = 0;
            offsetY     = 0;
            direction   = DIR_VERT;
            areacap     = 0.0;
            edgecap     = 0.0;
            respersq    = 0.0;
        }

    ~lefRoute()
        {
            clear();
        }

    void clear()
        {
            lefSpacingRule::destroy(spacing);
            spacing     = 0;
            width       = 0;
            pitchX      = 0;
            pitchY      = 0;
            offsetX     = 0;
            offsetY     = 0;
            direction   = DIR_VERT;
            areacap     = 0.0;
            edgecap     = 0.0;
            respersq    = 0.0;
        }

    lefSpacingRule *spacing; // Spacing rules, ordered by width.
    lefu_t  width;      // Nominal route width, in microns.
    lefu_t  pitchX;     // Route pitch, in microns.
    lefu_t  pitchY;     // Route pitch, in microns.
    lefu_t  offsetX;    // Route track offset from origin, in microns.
    lefu_t  offsetY;    // Route track offset from origin, in microns.
    ROUTE_DIR direction; // Preferred routing direction.
    double  areacap;    // Capacitance per area.
    double  edgecap;    // Capacitance per edge length.
    double  respersq;   // Resistance per square.
};


// Structure used to maintain default generation information for each
// via or viarule (contact) type.
//
struct lefVia
{
    lefVia()
        {
            deflt       = false;
            generate    = false;
            layer       = -1;
            lefId       = -1;
        }

    dbDsegB cut;        // The cut layer geometry, or cell bounding box.
    dbDsegB bot;        // Bottom metal layer geometry.
    dbDsegB top;        // Top metal layer geometry.
    bool    deflt;      // DEFAULT given.
    bool    generate;   // GENERATE/GENERATED given.
    short   layer;      // Lower routing level.
    int     lefId;      // LEF element.
    double  respervia;  // Resistance per via.
};

// There are two arrays of objects:  The cLDDB::db_lef_objects lists
// the objects in sequence as defined in the LEF file.  Each object
// (LAYER, VIA, VIARULE) is given a sequence number ("lefId"), which
// is simply its array index.  The second array is the
// cLDDB::db_layers, which lists only routing (metal) layers.  These
// are in physical sequence bottom to top, which is the same sequence
// as found in the objects array.  The router uses this array
// extensively.

// Structure defining a route or via layer referenced in LEF input,
// This structure is saved in the cLDDB::db_lef_objects array.
//
struct lefObject
{
    lefObject(char *a, lefCLASS t)
        {
            lefName     = a;
            lefClass    = t;
            layer       = -1;
            lefId       = -1;
        }

    ~lefObject()
        {
            delete [] lefName;
        }

    const char *lefName;    // CIF name of this layer.
    u_short lefClass;       // Is this a via, route, or masterslice layer.
    short   layer;          // Routing layer, or -1.
    int     lefId;          // LEF record sequence number.
};

// Routing layer.
//
struct lefRouteLayer : public lefObject
{
    lefRouteLayer(char *a) : lefObject(a, CLASS_ROUTE) { }

    lefRoute route;
};

struct lefCutLayer : public lefObject
{
    lefCutLayer(char *a) : lefObject(a, CLASS_CUT), spacing(0) { }

    lefu_t  spacing;
};

struct lefImplLayer : public lefObject
{
    lefImplLayer(char *a) : lefObject(a, CLASS_IMPLANT), width(0) { }

    lefu_t  width;
};

// Via or ViaRule object.
//
struct lefViaObject : public lefObject
{
    lefViaObject(char *a) : lefObject(a, CLASS_VIA) { }

    lefVia  via;
};

// Structure to hold info about routing layers in a via rule.
//
struct dbVRLyr
{
    dbVRLyr(int l, int id, ROUTE_DIR d, lefu_t d1, lefu_t d2, lefu_t d3,
        lefu_t d4)
        {
            direction   = d;
            layer       = l;
            lefId       = id;
            minWidth    = d1;
            maxWidth    = d2;
            overhang    = d3;
            moverhang   = d4;
        }

    short   direction;  // ROUTE_DIR
    short   layer;
    int     lefId;
    lefu_t  minWidth;
    lefu_t  maxWidth;
    lefu_t  overhang;
    lefu_t  moverhang;
};

struct lefViaRuleObject : public lefObject
{
    lefViaRuleObject(char *a) : lefObject(a, CLASS_VIARULE)
        {
            spacingX = spacingY = 0;
            met1 = met2 = 0;
        }

    ~lefViaRuleObject()
        {
            delete met1;
            delete met2;
        }

    lefVia  via;
    lefu_t  spacingX;
    lefu_t  spacingY;
    dbVRLyr *met1;
    dbVRLyr *met2;
};

namespace LefDefParser {
    // Defines for the Cadence LEF/DEF parser.
    class   lefiGeometries;
    class   lefiVia;
    class   lefiViaRule;
    class   lefiLayer;
    class   lefiSite;
    class   lefiMacro;
    class   lefiPin;
    class   lefiObstruction;
    class   lefiUnits;

    class   defiTrack;
    class   defiBox;
    class   defiComponent;
    class   defiBlockage;
    class   defiVia;
    class   defiPin;
    class   defiNet;
    class   defiWire;
}

// Interface class for error/warning/message reporting.  The user
// can implement this for their own channels.  A default to stdio
// is provided.
//
class cLDio
{
public:
    virtual void emitErrMesg(const char*) = 0;
    virtual void flushErrMesg() = 0;
    virtual void emitMesg(const char*) = 0;
    virtual void flushMesg() = 0;
    virtual void destroy() = 0;
};

// cLDDB:lefWrite argument.
enum LEF_OUT { LEF_OUT_ALL, LEF_OUT_TECH, LEF_OUT_MACROS };

// Debugging flags (for cLDDBif::debug).
// These flags control issuing of debugging messages, mostly for
// testing consistency with Qrouter (the Qrouter source must be
// modified to issue the same messages).

#define LD_DBG_ORDR     0x1
    // When set, pins and gates, etc., are saved internally in the
    // same order used by Qrouter, which is generally opposite file
    // order.  We prefer to keep file order when possible.
#define LD_DBG_NETS     0x2
    // In mrouter/mrouter.cc (cMRouter::create_net_order) dump a
    // listing of the sorted nets to a file named "nets".
#define LD_DBG_NODES    0x4
    // In mrouter/mrouter.cc (cMRouter::initRouter) call the
    // cLDDB::printNodes function to dump a listing of the "nodes"
    // (grid cells of contact points) associated with each net, to a
    // file named "nodes".
#define LD_DBG_FLGS     0x8
    // In mrouter/mrouter.cc (cMRouter::initRouter) dump a list of the
    // flags set at each routing grid location to a file named "flags".

// End of debugging flags.

// This is the exported interface of the cLDDB class.  By deriving
// cLDDB from this class, one can pass a pointer through dlsym, and
// have access to all class methods, greatly facilitating run-time
// loading of the LDDB feature set into another application.
//
class cLDDBif
{
public:
    // lddb.cc
    static cLDDBif *newLDDB();

    virtual ~cLDDBif() { }

    virtual bool    reset() = 0;
    virtual bool    setupChannels(bool) = 0;
    virtual void    emitError(const char*, ...) = 0;
    virtual void    emitErrMesg(const char*, ...) = 0;
    virtual void    flushErrMesg() = 0;
    virtual void    emitMesg(const char*, ...) = 0;
    virtual void    flushMesg() = 0;
    virtual int     getLayer(const char*) = 0;
    virtual lefu_t  getRouteKeepout(int) = 0;
    virtual lefu_t  getRouteWidth(int) = 0;
    virtual lefu_t  getRouteOffset(int, ROUTE_DIR) = 0;
    virtual lefu_t  getViaWidth(int, int, ROUTE_DIR) = 0;
    virtual lefu_t  getXYViaWidth(int, int, ROUTE_DIR, int) = 0;
    virtual lefu_t  getRouteSpacing(int) = 0;
    virtual lefu_t  getRouteWideSpacing(int, lefu_t) = 0;
    virtual lefu_t  getRoutePitch(int, ROUTE_DIR) = 0;
    virtual const char *getRouteName(int) = 0;
    virtual ROUTE_DIR getRouteOrientation(int) = 0;
    virtual dbGate  *getGate(const char*) = 0;
    virtual int     getGateNum(const char*) = 0;
    virtual dbGate  *getPin(const char*) = 0;
    virtual int     getPinNum(const char*) = 0;
    virtual dbGate  *getGateOrPinByNum(int) = 0;
    virtual dbNet   *getNet(const char*) = 0;
    virtual dbNet   *getNetByNum(u_int) = 0;
    virtual lefMacro *getLefGate(const char*) = 0;
    virtual lefObject *getLefObject(const char*) = 0;
    virtual lefRouteLayer *getLefRouteLayer(const char*) = 0;
    virtual lefRouteLayer *getLefRouteLayer(int) = 0;
    virtual void    printInfo(FILE*) = 0;
    virtual const char *printNodeName(dbNode*) = 0;
    virtual void    printNets(const char*) = 0;
    virtual void    printNodes(const char*) = 0;
    virtual void    printRoutes(const char*) = 0;
    virtual void    printNlgates(const char*) = 0;
    virtual void    printNlnets(const char*) = 0;
    virtual void    printNet(dbNet*) = 0;
    virtual void    printGate(dbGate*) = 0;
    virtual void    checkVariablePitch(int, int*, int*) = 0;
    virtual void    addObstruction(lefu_t, lefu_t, lefu_t, lefu_t, u_int) = 0;
    virtual dbDseg  *findObstruction(lefu_t, lefu_t, lefu_t, lefu_t, u_int) = 0;

    virtual bool    readScript(const char*) = 0;
    virtual bool    readScript(FILE*) = 0;
    virtual bool    doCmd(const char*) = 0;
    virtual bool    cmdSet(const char*) = 0;
    virtual bool    cmdUnset(const char*) = 0;
    virtual bool    cmdIgnore(const char*) = 0;
    virtual bool    cmdCritical(const char*) = 0;
    virtual bool    cmdObstruction(const char*) = 0;
    virtual bool    cmdLayer(const char*) = 0;
    virtual bool    cmdNewLayer(const char*) = 0;
    virtual bool    cmdBoundary(const char*) = 0;
    virtual bool    cmdReadLef(const char*) = 0;
    virtual bool    cmdReadDef(const char*) = 0;
    virtual bool    cmdWriteLef(const char*) = 0;
    virtual bool    cmdWriteDef(const char*) = 0;
    virtual bool    cmdUpdateDef(const char*, const char*) = 0;

    virtual bool    defRead(const char*) = 0;
    virtual void    defReset() = 0;

    virtual bool    defWrite(const char*) = 0;
    virtual bool    writeDefRoutes(const char*, const char*) = 0;
    virtual bool    writeDefNetRoutes(FILE*, dbNet*, bool) = 0;
    virtual bool    writeDefStubs(FILE*) = 0;

    virtual bool    lefRead(const char*, bool = false) = 0;
    virtual void    lefReset() = 0;
    virtual void    lefAddObject(lefObject*) = 0;
    virtual void    lefAddGate(lefMacro*) = 0;

    virtual bool    lefWrite(const char*, LEF_OUT = LEF_OUT_ALL) = 0;

    virtual cLDio   *ioHandler() = 0;
    virtual void    setIOhandler(cLDio*) = 0;

    virtual const char *global(u_int) = 0;
    virtual bool    addGlobal(const char*) = 0;
    virtual void    clearGlobal(int = 0) = 0;

    virtual stringlist *noRouteList() = 0;
    virtual void    setNoRouteList(stringlist*) = 0;
    virtual void    dontRoute(const char*) = 0;

    virtual stringlist *criticalNetList() = 0;
    virtual void    setCriticalNetList(stringlist*) = 0;
    virtual void    criticalNet(const char*) = 0;

    virtual const char *commentLayerName() = 0;
    virtual void    setCommentLayerName(const char*) = 0;
    virtual int     commentLayerNumber() = 0;
    virtual void    setCommentLayerNumber(int) = 0;
    virtual int     commentLayerPurpose() = 0;
    virtual void    setCommentLayerPurpose(int) = 0;

    virtual u_int   verbose() = 0;
    virtual void    setVerbose(u_int) = 0;

    virtual u_int   debug() = 0;
    virtual void    setDebug(u_int) = 0;

    virtual u_int   numLayers() = 0;
    virtual void    setNumLayers(u_int) = 0;
    virtual u_int   allocLayers() = 0;

    virtual const char *layerName(u_int) = 0;
    virtual void    setLayerName(u_int, const char*) = 0;

    virtual lefu_t  pathWidth(u_int) = 0;
    virtual void    setPathWidth(u_int, lefu_t) = 0;

    virtual lefu_t  startX(u_int) = 0;
    virtual void    setStartX(u_int, lefu_t) = 0;

    virtual lefu_t  startY(u_int) = 0;
    virtual void    setStartY(u_int, lefu_t) = 0;

    virtual lefu_t  pitchX(u_int) = 0;
    virtual void    setPitchX(u_int, lefu_t) = 0;

    virtual lefu_t  pitchY(u_int) = 0;
    virtual void    setPitchY(u_int, lefu_t) = 0;

    virtual int     numChannelsX(u_int) = 0;
    virtual void    setNumChannelsX(u_int, u_int) = 0;

    virtual int     numChannelsY(u_int) = 0;
    virtual void    setNumChannelsY(u_int, u_int) = 0;

    virtual int     viaXid(u_int) = 0;
    virtual void    setViaXid(u_int, int) = 0;

    virtual int     viaYid(u_int) = 0;
    virtual void    setViaYid(u_int, int) = 0;

    virtual lefu_t  haloX(u_int) = 0;
    virtual void    setHaloX(u_int, lefu_t) = 0;

    virtual lefu_t  haloY(u_int) = 0;
    virtual void    setHaloY(u_int, lefu_t) = 0;

    virtual int     layerNumber(u_int) = 0;
    virtual void    setLayerNumber(u_int, int) = 0;

    virtual int     purposeNumber(u_int) = 0;
    virtual void    setPurposeNumber(u_int, int) = 0;

    virtual bool    vert(u_int) = 0;
    virtual void    setVert(u_int, bool) = 0;

    virtual u_int   needBlock(u_int) = 0;
    virtual void    setNeedBlock(u_int, u_int) = 0;

    virtual u_int   lefResol() = 0;
    virtual bool    setLefResol(u_int) = 0;
    virtual u_int   defInResol() = 0;
    virtual bool    setDefInResol(u_int) = 0;
    virtual u_int   defOutResol() = 0;
    virtual bool    setDefOutResol(u_int) = 0;
    virtual lefu_t  micToLef(double) = 0;
    virtual lefu_t  micToLefGrid(double) = 0;
    virtual lefu_t  defToLefGrid(double) = 0;
    virtual double  lefToMic(lefu_t) = 0;
    virtual int     lefToDef(lefu_t) = 0;
    virtual lefu_t  manufacturingGrid() = 0;
    virtual bool    setManufacturingGrid(lefu_t) = 0;

    virtual lefObject *getLefObject(u_int) = 0;
    virtual lefMacro *pinMacro() = 0;

    virtual const char *technology() = 0;
    virtual void    setTechnology(const char*) = 0;

    virtual const char *design() = 0;
    virtual void    setDesign(const char*) = 0;

    virtual dbGate  *nlGate(u_int) = 0;
    virtual dbGate  *nlPin(u_int) = 0;
    virtual dbNet   *nlNet(u_int) = 0;

    virtual dbDseg  *userObs() = 0;
    virtual void    setUserObs(dbDseg*) = 0;
    virtual dbDseg  *intObs() = 0;
    virtual void    setIntObs(dbDseg*) = 0;

    virtual u_int   numGates() = 0;
    virtual u_int   numPins() = 0;
    virtual u_int   numNets() = 0;
    virtual u_int   maxNets() = 0;
    virtual void    setMaxNets(u_int) = 0;
    virtual u_int   dfMaxNets() = 0;
    virtual void    setDfMaxNets(u_int) = 0;

    virtual lefu_t  xLower() = 0;
    virtual void    setXlower(lefu_t) = 0;
    virtual lefu_t  yLower() = 0;
    virtual void    setYlower(lefu_t) = 0;
    virtual lefu_t  xUpper() = 0;
    virtual void    setXupper(lefu_t) = 0;
    virtual lefu_t  yUpper() = 0;
    virtual void    setYupper(lefu_t) = 0;

    virtual int     currentLine() = 0;
    virtual void    setCurrentLine(int) = 0;

    virtual const char *doneMsg() = 0;
    virtual void setDoneMsg(char*) = 0;
    virtual const char *warnMsg() = 0;
    virtual void setWarnMsg(char*) = 0;
    virtual const char *errMsg() = 0;
    virtual void setErrMsg(char*) = 0;
    virtual void clearMsgs() = 0;
};

// This is a wrapper class that the router class can derive from,
// which will provide the most-called cLDDB functions indirected
// through a pointer.  Using this instead of deriving direcely from
// cLDDB (which we used to do) provides modularity, the database and
// router are separate modules.  It avoids for the most part having to
// add the explicit pointer into long, complicated expressions found
// in router code.
//
class cLDDBref
{
public:
    cLDDBref(cLDDBif *d)            { db = d; }

    int     getLayer(const char *n) { return (db->getLayer(n)); }
    lefu_t  getRouteKeepout(int l)  { return (db->getRouteKeepout(l)); }
    lefu_t  getRouteWidth(int l)    { return (db->getRouteWidth(l)); }
    lefu_t  getRouteOffset(int l, ROUTE_DIR d)
                                    { return (db->getRouteOffset(l, d)); }
    lefu_t  getViaWidth(int b, int l, ROUTE_DIR d)
                                    { return (db->getViaWidth(b, l, d)); }
    lefu_t  getXYViaWidth(int b, int l, ROUTE_DIR d, int o)
                                    { return (db->getXYViaWidth(b, l, d, o)); }
    lefu_t  getRouteSpacing(int l)  { return (db->getRouteSpacing(l)); }
    lefu_t  getRouteWideSpacing(int l, lefu_t w)
                                    { return (db->getRouteWideSpacing(l, w)); }
    lefu_t  getRoutePitch(int l, ROUTE_DIR d)
                                    { return (db->getRoutePitch(l, d)); }
    const char *getRouteName(int l) { return (db->getRouteName(l)); }
    ROUTE_DIR getRouteOrientation(int l)
                                    { return (db->getRouteOrientation(l)); }
    dbGate  *getGate(const char *nm) { return (db->getGate(nm)); }
    dbGate  *getPin(const char *nm) { return (db->getPin(nm)); }
    dbNet   *getNet(const char *nm) { return (db->getNet(nm)); }
    dbNet   *getNetByNum(u_int n)   { return (db->getNetByNum(n)); }
    lefMacro *getLefGate(const char *nm)
                                    { return (db->getLefGate(nm)); }
    lefObject *getLefObject(const char *nm)
                                    { return (db->getLefObject(nm)); }
    lefRouteLayer *getLefRouteLayer(const char *nm)
                                    { return (db->getLefRouteLayer(nm)); }
    lefRouteLayer *getLefRouteLayer(int l)
                                    { return (db->getLefRouteLayer(l)); }

    u_int   verbose()               { return (db->verbose()); }
    u_int   debug()                 { return (db->debug()); }
    u_int   numLayers()             { return (db->numLayers()); }
    const char *layerName(u_int l)  { return (db->layerName(l)); }
    lefu_t  pathWidth(u_int l)      { return (db->pathWidth(l)); }
    lefu_t  pitchX(u_int l)         { return (db->pitchX(l)); }
    lefu_t  pitchY(u_int l)         { return (db->pitchY(l)); }
    int     numChannelsX(u_int l)   { return (db->numChannelsX(l)); }
    int     numChannelsY(u_int l)   { return (db->numChannelsY(l)); }
    int     viaXid(u_int l)         { return (db->viaXid(l)); }
    int     viaYid(u_int l)         { return (db->viaYid(l)); }
    lefu_t  haloX(u_int l)          { return (db->haloX(l)); }
    lefu_t  haloY(u_int l)          { return (db->haloY(l)); }
    int     layerNumber(u_int l)    { return (db->layerNumber(l)); }
    int     purposeNumber(u_int l)  { return (db->purposeNumber(l)); }
    bool    vert(u_int l)           { return (db->vert(l)); }
    u_int   needBlock(u_int l)      { return (db->needBlock(l)); }

    lefObject *getLefObject(u_int l) { return (db->getLefObject(l)); }
    lefMacro *pinMacro()            { return (db->pinMacro()); }
    const char *technology()        { return (db->technology()); }
    const char *design()            { return (db->design()); }

    dbGate  *nlGate(u_int n)        { return (db->nlGate(n)); }
    dbGate  *nlPin(u_int n)         { return (db->nlPin(n)); }
    dbNet   *nlNet(u_int n)         { return (db->nlNet(n)); }
    dbDseg  *userObs()              { return (db->userObs()); }
    dbDseg  *intObs()               { return (db->intObs()); }
    u_int   numGates()              { return (db->numGates()); }
    u_int   numPins()               { return (db->numPins()); }
    u_int   numNets()               { return (db->numNets()); }
    u_int   maxNets()               { return (db->maxNets()); }
    lefu_t  xLower()                { return (db->xLower()); }
    lefu_t  yLower()                { return (db->yLower()); }
    lefu_t  xUpper()                { return (db->xUpper()); }
    lefu_t  yUpper()                { return (db->yUpper()); }

    const char *doneMsg()           { return (db->doneMsg()); }
    void setDoneMsg(char *s)        { db->setDoneMsg(s); }
    const char *warnMsg()           { return (db->warnMsg()); }
    void setWarnMsg(char *s)        { db->setWarnMsg(s); }
    const char *errMsg()            { return (db->errMsg()); }
    void setErrMsg(char *s)         { db->setErrMsg(s); }
    void clearMsgs()                { db->clearMsgs(); }

protected:
    cLDDBif *db;
};

#endif

