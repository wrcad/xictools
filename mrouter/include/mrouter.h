
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
 $Id: mrouter.h,v 1.9 2017/02/17 05:44:36 stevew Exp $
 *========================================================================*/

#ifndef MR_IF_H
#define MR_IF_H

#include "lddb.h"
#include "mr_vers.h"

//
// Maze Router.
//
// Implementation of a maze router, for use with the LEF/DEF database.
//

// Default configuration filename.
#define MR_CONFIG_FILENAME      "route.cfg" 

// Maximum pass count.
#define MR_MAX_PASSES           100

// Define types of via checkerboard patterns.
enum VIA_PATTERN
{
    VIA_PATTERN_NONE    = -1,
    VIA_PATTERN_NORMAL  = 0,
    VIA_PATTERN_INVERT  = 1
};

// Define search directions.
enum MR_DIR
{
    NORTH   = 1,
    SOUTH,
    EAST,
    WEST,
    UP,
    DOWN
};

// Choose a sorting algorithm for nets.  NoSort keeps nets in the
// database order, other algorithms presently match those of Qrouter.
enum mrNetOrder { mrNetDefault, mrNetAlt1, mrNetNoSort };

// Mask types (values other than 255 are interpreted as "slack" value).
enum MASK_TYPE
{
    MASK_MINIMUM        = 0,        // No slack.
    MASK_SMALL          = 1,        // Slack of +/-1.
    MASK_MEDIUM         = 2,        // Slack of +/-2.
    MASK_LARGE          = 4,        // Slack of +/-4.
    MASK_AUTO           = 253,      // Choose best mask type:
                                    //  stage1 SMALL, otherwise LARGE.
    MASK_BBOX           = 254,      // Mask is simple bounding box.
    MASK_NONE           = 255       // No mask used.
};

#define MAXRT   10000000        // "Infinite" route cost.

// Map and draw modes.
#define MAP_NONE        0x0         // No map (blank background).
#define MAP_OBSTRUCT    0x1         // Make a map of obstructions and pins.
#define MAP_CONGEST     0x2         // Make a map of congestion.
#define MAP_ESTIMATE    0x3         // Make a map of estimated congestion.
#define MAP_MASK        0x3

#define DRAW_NONE       0x0         // Draw only the background map.
#define DRAW_ROUTES     0x4         // Draw routes on top of background map.
#define DRAW_UNROUTED   0x8         // Draw unrouted nets on top of backgrnd map.
#define DRAW_MASK       0xc

struct dbNet;
struct mrStack;

// Interface class for graphics support.  User can derive a class from
// this and register it with cMRouter for graphics support.
//
struct mrGraphics
{
    virtual void    highlight_source()                  = 0;
    virtual void    highlight_dest()                    = 0; 
    virtual void    highlight_starts(const mrStack*)    = 0;
    virtual void    highlight_mask()                    = 0;
    virtual void    highlight(int, int)                 = 0;
    virtual void    draw_net(const dbNet*, u_int, int*) = 0;
    virtual void    draw_layout()                       = 0;
    virtual bool    recalc_spacing()                    = 0;
};

// Routing pass: stage 1 normal route, stage2 route and ripup.
enum mrStage { mrStage1, mrStage2, mrStage2p2, mrStage2p3 };

// This is the exported interface of the cMRouter class.  By deriving
// cMRouter from this class, one can pass a pointer through dlsym, and
// have access to all class methods, greatly facilitating run-time
// loading of the router feature set into another application.
//
class cMRif
{
public:
    // mrouter.cc
    static cMRif    *newMR(cLDDBif*);

    virtual ~cMRif() { }

    virtual bool    initRouter() = 0;
    virtual void    resetRouter() = 0;
    virtual bool    doRoute(dbNet*, mrStage, bool) = 0;
    virtual int     routeNetRipup(dbNet*, bool) = 0;
    virtual int     doFirstStage(bool, int) = 0;
    virtual int     doSecondStage(bool, bool) = 0;
    virtual int     doThirdStage(bool, int) = 0;
    virtual void    printClearFailed() = 0;

    virtual bool    readScript(const char*) = 0;
    virtual bool    readScript(FILE*) = 0;
    virtual bool    doCmd(const char*) = 0;
    virtual bool    reset(bool) = 0;
    virtual bool    cmdSet(const char*) = 0;
    virtual bool    cmdSetcost(const char*) = 0;
    virtual bool    cmdUnset(const char*) = 0;
    virtual bool    cmdReadCfg(const char*) = 0;
    virtual bool    cmdStage1(const char*) = 0;
    virtual bool    cmdStage2(const char*) = 0;
    virtual bool    cmdStage3(const char*) = 0;
    virtual bool    cmdRipUp(const char*) = 0;
    virtual bool    cmdFailed(const char*) = 0;

    virtual int     readConfig(const char*, bool, bool = false) = 0;

    virtual bool    ripupNet(dbNet*, bool) = 0;

    virtual bool    initPhysRouteGen() = 0;
    virtual void    setupRoutePaths() = 0;
    virtual bool    setupRoutePath(dbNet*, bool) = 0;
    virtual void    clearPhysRouteGen() = 0;

    virtual cLDDBif *lddb() = 0;

    virtual bool    hasRmask() = 0;
    virtual u_char  rmask(int, int) = 0;
    virtual void    setRmask(int, int, u_int) = 0;
    virtual u_char  *rmaskIncs() = 0;
    virtual u_int   rmaskIncsSz() = 0;
    virtual void    setRmaskIncs(u_char*, u_int) = 0;
    virtual dbNetList *failedNets() = 0;

    virtual u_int   totalRoutes() = 0;
    virtual u_int   maskVal() = 0;
    virtual void    setMaskVal(u_int) = 0;
    virtual u_int   pinLayers() = 0;
    virtual void    setPinLayers(u_int) = 0;
    virtual mrNetOrder netOrder() = 0;
    virtual void    setNetOrder(mrNetOrder) = 0;

    virtual int     segCost() = 0;
    virtual void    setSegCost(int) = 0;
    virtual int     viaCost() = 0;
    virtual void    setViaCost(int) = 0;
    virtual int     jogCost() = 0;
    virtual void    setJogCost(int) = 0;
    virtual int     xverCost() = 0;
    virtual void    setXverCost(int) = 0;
    virtual int     blockCost() = 0;
    virtual void    setBlockCost(int) = 0;
    virtual int     offsetCost() = 0;
    virtual void    setOffsetCost(int) = 0;
    virtual int     conflictCost() = 0;
    virtual void    setConflictCost(int) = 0;

    virtual u_int   numPasses() = 0;
    virtual void    setNumPasses(u_int) = 0;
    virtual int     stackedVias() = 0;
    virtual void    setStackedVias(int) = 0;
    virtual VIA_PATTERN viaPattern() = 0;
    virtual void    setViaPattern(VIA_PATTERN) = 0;

    virtual bool    forceRoutable() = 0;
    virtual void    setForceRoutable(bool) = 0;
    virtual u_int   keepTrying() = 0;
    virtual void    setKeepTrying(u_int) = 0;
    virtual u_int   mapType() = 0;
    virtual void    setMapType(u_int) = 0;
    virtual u_int   ripLimit() = 0;
    virtual void    setRipLimit(u_int) = 0;

    virtual void    registerGraphics(mrGraphics*) = 0;
};

#endif

