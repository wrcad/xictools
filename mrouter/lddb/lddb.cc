
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
 $Id: lddb.cc,v 1.20 2017/02/17 21:37:44 stevew Exp $
 *========================================================================*/

#include <stdarg.h>
#include <errno.h>
#include <math.h>
#include <algorithm>
#include "lddb_prv.h"
#include "ld_hash.h"


//
// LEF/DEF Database.
//
// Constructor and miscellaneous functions.
//


extern "C" {
    // C function which can be found with dlsym, allows
    // load-on-demand of the lddb package.
    //
    cLDDBif *new_lddb()
    {
        return (cLDDBif::newLDDB());
    }

    // C function returns the version string defined in lddb.h, for
    // library compatibility test.  At least for now, an application
    // shouldn't try and load a version different from the one used
    // when the application was built.
    //
    const char *ld_version_string()
    {
        return (LD_VERSION);
    }
}


#ifdef LD_MEMDBG
int dbSeg::seg_cnt;
int dbSeg::seg_hw;
int dbDseg::dseg_cnt;
int dbDseg::dseg_hw;
int dbDpoint::dpoint_cnt;
int dbDpoint::dpoint_hw;
int dbPath::path_cnt;
int dbPath::path_hw;
int dbRoute::route_cnt;
int dbRoute::route_hw;
int dbNode::node_cnt;
int dbNode::node_hw;
int dbNetList::netlist_cnt;
int dbNetList::netlist_hw;
#endif


// Static function.
// Export a "constructor" for direct C++ usage.
//
cLDDBif *
cLDDBif::newLDDB()
{
    return (new cLDDB);
}


// Same order as PORT_CLASS enum.
const char *cLDDB::pin_classes[] = {
    "unset",
    "INPUT",
    "OUTPUT TRISTATE",
    "OUTPUT",
    "INOUT",
    "FEEDTHRU",
    0
};

// Same order as PORT_USE enum.
const char *cLDDB::pin_uses[] = {
    "unset",
    "SIGNAL",
    "ANALOG",
    "POWER",
    "GROUND",
    "CLOCK",
    0
};

// Same order as PORT_SHAPE enum.
const char *cLDDB::pin_shapes[] = {
    "unset",
    "ABUTMENT",
    "RING",
    "FEEDTHRU",
    0
};


cLDDB::cLDDB()
{
    // Config info.
    db_io               = new cLDstdio;
    for (int i = 0; i < LD_MAX_GLOBALS; i++) {
        db_global_names[i] = 0;
        db_global_nnums[i] = i+1;
    }
    db_dontRoute        = 0;
    db_criticalNet      = 0;
    db_layers           = 0;
    db_numGlobals       = 0;
    db_numLayers        = 0;
    db_allocLyrs        = 0;
    db_verbose          = 0;
    db_debug            = 0;

    // LEF info.
    db_lef_objects      = 0;
    db_lef_obj_hash     = 0;
    db_lef_objsz        = 0;
    db_lef_objcnt       = 0;
    db_lef_gates        = 0;
    db_lef_gate_hash    = 0;
    db_lef_gatesz       = 0;
    db_lef_gatecnt      = 0;
    db_pinMacro         = 0;
    db_mfg_grid         = 0;
    db_lef_precis       = 1;
    db_lef_resol        = LEFDEF_DEFAULT_RESOL;

    // DEF info.
    db_technology       = 0;
    db_design           = 0;
    db_nlGates          = 0;
    db_gate_hash        = 0;
    db_nlPins           = 0;
    db_pin_hash         = 0;
    db_nlNets           = 0;
    db_net_hash         = 0;
    db_userObs          = 0;
    db_intObs           = 0;
    db_numGates         = 0;
    db_numPins          = 0;
    db_numNets          = 0;
    db_maxNets          = 0;
    db_dfMaxNets        = 0;
    db_xLower           = 0;
    db_xUpper           = 0;
    db_yLower           = 0;
    db_yUpper           = 0;
    db_def_resol        = LEFDEF_DEFAULT_RESOL;
    db_def_out_resol    = 0;

    // Parser.
    db_currentLine      = 0;
    db_errors           = 0;

    // DEF parser state.
    db_def_total        = 0;
    db_def_processed    = 0;
    db_def_netidx       = LD_MIN_NETNUM;
    db_def_corient      = '.';
    db_def_case_sens    = true;
    db_def_resol_set    = false;
    db_def_tracks_set   = false;

    // LEF parser state.
    db_lef_resol_set    = false;
    db_lef_precis_set   = false;
    db_lef_case_sens    = true;

    db_donemsg          = 0;
    db_warnmsg          = 0;
    db_errmsg           = 0;
}


cLDDB::~cLDDB()
{
    if (db_io)
        db_io->destroy();
    for (int i = 0; i < LD_MAX_GLOBALS; i++)
        delete [] db_global_names[i];
    reset();
}


// Clear and reset the database.
//
bool
cLDDB::reset()
{
    // Config info.
    // Don't reset globals, verbose.  These are externally set
    // (e.g., command line).

    stringlist::destroy(db_dontRoute);
    db_dontRoute        = 0;
    stringlist::destroy(db_criticalNet);
    db_criticalNet      = 0;

    delete [] db_layers;
    db_layers           = 0;
    db_numLayers        = 0;
    db_allocLyrs        = 0;

    // LEF info.
    for (u_int i = 0; i < db_lef_objcnt; i++)
        delete db_lef_objects[i];
    delete [] db_lef_objects;
    db_lef_objects      = 0;
    delete db_lef_obj_hash;
    db_lef_obj_hash     = 0;
    db_lef_objsz        = 0;
    db_lef_objcnt       = 0;
    for (u_int i = 0; i < db_lef_gatecnt; i++)
        delete db_lef_gates[i];
    delete [] db_lef_gates;
    db_lef_gates        = 0;
    delete db_lef_gate_hash;
    db_lef_gate_hash    = 0;
    db_lef_gatesz       = 0;
    db_lef_gatecnt      = 0;
    db_pinMacro         = 0;
    db_mfg_grid         = 0;
    db_lef_precis       = 1;
    db_lef_resol        = LEFDEF_DEFAULT_RESOL;

    // DEF info.
    delete [] db_technology;
    db_technology       = 0;
    delete [] db_design;
    db_design           = 0;
    for (u_int i = 0; i < db_numGates; i++)
        delete db_nlGates[i];
    delete [] db_nlGates;
    db_nlGates          = 0;
    delete db_gate_hash;
    db_gate_hash        = 0;
    for (u_int i = 0; i < db_numPins; i++)
        delete db_nlPins[i];
    delete [] db_nlPins;
    db_nlPins           = 0;
    delete db_pin_hash;
    db_pin_hash         = 0;
    for (u_int i = 0; i < db_numNets; i++)
        delete db_nlNets[i];
    delete [] db_nlNets;
    db_nlNets           = 0;
    delete db_net_hash;
    db_net_hash         = 0;
    dbDseg::destroy(db_userObs);
    db_userObs          = 0;
    dbDseg::destroy(db_intObs);
    db_intObs           = 0;
    db_numNets          = 0;
    db_xLower           = 0;
    db_xUpper           = 0;
    db_yLower           = 0;
    db_yUpper           = 0;
    db_def_resol        = LEFDEF_DEFAULT_RESOL;
    db_def_out_resol    = 0;

    // Parser.
    db_currentLine      = 0;
    db_errors           = 0;

    // DEF parser state.
    db_def_total        = 0;
    db_def_processed    = 0;
    db_def_netidx       = LD_MIN_NETNUM;
    db_def_corient      = '.';
    db_def_case_sens    = true;
    db_def_resol_set    = false;
    db_def_tracks_set   = false;

    // LEF parser state.
    db_lef_resol_set    = false;
    db_lef_precis_set   = false;
    db_lef_case_sens    = true;

    return (true);
}


// setupChannels
//
// Check track pitch and set the number of channels.  Currently we are
// constrained to a single routing pitch for each direction, which
// implies a single channel count for each direction.  The channel
// counts will have been set when reading TRACKS from DEF.  The values
// we use are defined on the lowest layers.
//
bool
cLDDB::setupChannels(bool checknodes)
{
    if (debug() & LD_DBG_ORDR) {
        // Use the same track count as Qrouter, for consistency of
        // comparisons.  I don't like this, as it can give different
        // values than specified in the DEF file, e.g., for map9v3,
        // one gets 159/96 instead of the given 157/95.
        
        // Another problem is that this function can be called more
        // than once, and each call will change the track numbers. 
        // The following prevents this.
        if (db_def_tracks_set) {
            checkNodes();
            return (LD_OK);
        }

        lefu_t xl = 0, xu = 0, yl = 0, yu = 0;
        for (u_int i = 0; i < numLayers(); i++) {
            if (vert(i)) {
                xl = startX(i);
                xu = xl + numChannelsX(i)*pitchX(i);
            }
            else {
                yl = startY(i);
                yu = yl + numChannelsY(i)*pitchY(i);
            }
        }
        // Just set the area here, this will be used to get track
        // counts below.

        db_xLower = xl;
        db_yLower = yl;
        db_xUpper = xu;
        db_yUpper = yu;
        db_def_tracks_set = true;
    }

    // If a TRACK wasn't given for the layer, use LEF defaults.
    for (u_int i = 0; i < numLayers(); i++) {
        lefRouteLayer *lefr = getLefRouteLayer(i);

        dbLayer *layer = &db_layers[i];

        // This doesn't seem to be set anywhere else?
        if (!layer->pathWidth)
            layer->pathWidth = lefr->route.width;

        if (!layer->trackXset && !layer->trackYset) {
            layer->vert = (lefr->route.direction == DIR_VERT);
            layer->pitchX = lefr->route.pitchX;
            layer->pitchY = lefr->route.pitchY;
        }
    }

    int h = -1;
    int v = -1;
    for (u_int i = 0; i < numLayers(); i++) {
        if (!vert(i)) {
            h = i;
            // In the config file and the layer command we only set
            // pitchX.  Fix this here according to the routing
            // direction.

            if (pitchY(i) == 0) {
                setPitchY(i, pitchX(i));
                setPitchX(i, 0);
            }
        }
        else
            v = i;
    }

    // In case all layers are listed as horizontal or all as vertical,
    // we should still handle it gracefully.

    if (h == -1)
        h = v;
    else if (v == -1)
        v = h;

    // The following code ensures that the layer grids align.  For
    // now, all PitchX[i] and PitchY[i] should be the same for all
    // layers.  Hopefully this restriction can be lifted sometime, but
    // it will necessarily be a royal pain.

    for (u_int i = 0; i < numLayers(); i++) {
        if (vert(i)) {
            if (pitchX(i) != 0 && pitchX(i) != pitchX(v)) {
                emitErrMesg(
                    "Multiple vertical route layers at different pitches.  "
                    "Using smaller\npitch %g, will route on 1-of-N tracks "
                    "if necessary.\n", lefToMic(pitchX(i)));
                setPitchX(v, pitchX(i));
            }
            setPitchX(i, pitchX(v));
        }
        else {
            if (pitchY(i) != 0 && pitchY(i) != pitchY(h)) {
                emitErrMesg(
                    "Multiple horizontal route layers at different pitches.  "
                    "Using smaller\npitch %g, will route on 1-of-N tracks "
                    "if necessary.\n", lefToMic(pitchY(i)));
                setPitchY(h, pitchY(i));
            }
            setPitchY(i, pitchY(h));
        }
    }

    // Make sure each layer has X and Y pitch defined.
    for (u_int i = 0; i < numLayers(); i++) {
        if (pitchX(i) == 0)
            setPitchX(i, pitchX(v));
        if (pitchY(i) == 0)
            setPitchY(i, pitchY(h));
    }

    int nchX = 0;
    int nchY = 0;
    if (!(debug() & LD_DBG_ORDR)) {
        // Normal case, get the counts from the lowest tracks.

        for (u_int i = 0; i < numLayers(); i++) {
            if (!nchX && numChannelsX(i) > 0)
                nchX = numChannelsX(i);
            if (!nchY && numChannelsY(i) > 0)
                nchY = numChannelsY(i);
            if (nchX && nchY)
                break;
        }
    }

    // If we didn't get a channel count set by TRACK records, use the
    // DIEAREA.

    if (!nchX) {
        for (u_int i = 0; i < numLayers(); i++) {
            if (pitchX(i) > 0) {
                if (debug() & LD_DBG_ORDR)
                    nchX = 1 + (xUpper() - xLower() + pitchX(i)/2)/pitchX(i);
                else
                    nchX = (xUpper() - xLower())/pitchX(i);
                break;
            }
        }
    }
    if (!nchY) {
        for (u_int i = 0; i < numLayers(); i++) {
            if (pitchY(i) > 0) {
                if (debug() & LD_DBG_ORDR)
                    nchY = 1 + (yUpper() - yLower() + pitchY(i)/2)/pitchY(i);
                else
                    nchY = (yUpper() - yLower())/pitchY(i);
                break;
            }
        }
    }
    if (nchX <= 0 || nchY <= 0) {
        emitErrMesg("Error:  Can't determine channel counts.\n");
        return (LD_BAD);
    }

    // Now set all the channel counts to the appropriate values.
    for (u_int i = 0; i < numLayers(); i++) {
        if (pitchX(i) == 0 || pitchY(i) == 0) {
            emitErrMesg("Have a 0 pitch for layer %d (of %d).  Exit.\n",
                i + 1, numLayers());
            return (LD_BAD);
        }
        setNumChannelsX(i, nchX);
        setNumChannelsY(i, nchY);
        if (verbose() > 1) {
            emitMesg("  Number of x,y channels for layer %d is %d,%d.\n",
                i, nchX, nchY);
        }
    }

    // Set the bounds to be consistent with the tracks.  This means,
    // given that all pitches are equal, that the start values and
    // lower limits must be equal.  The upper limits will be expanded
    // if necessary to enclose all tracks.
    //
    // Maybe the track numbers should be adjusted instead to fit
    // within the given diearea?

    bool setx = false;
    bool sety = false;
    for (u_int i = 0; i < numLayers(); i++) {
        if (vert(i)) {
            if (!setx) {
                db_xLower = startX(i);
                setx = true;
            }
            else {
                if (startX(i) != db_xLower) {
                    emitError(
                        "Warning, origin X change in layer %d routing grid \
                        not handled.\n", i+1);
                }
                setStartX(i, db_xLower);
            }
        }
        else {
            if (!sety) {
                db_yLower = startY(i);
                sety = true;
            }
            else {
                if (startY(i) != db_yLower) {
                    emitError(
                        "Warning, origin Y change in layer %d routing grid \
                        not handled.\n", i+1);
                }
                setStartY(i, db_yLower);
            }
        }
    }

    setx = false;
    sety = false;
    for (u_int i = 0; i < numLayers(); i++) {
        if (vert(i)) {
            if (!setx) {
                db_xUpper = db_xLower + numChannelsX(i)*pitchX(i);
                setx = true;
            }
        }
        else {
            if (!sety) {
                db_yUpper = db_yLower + numChannelsY(i)*pitchY(i);
                sety = true;
            }
        }
        if (setx && sety)
            break;
    }
    if (verbose() > 2) {
        for (u_int i = 0; i < numLayers(); i++) {
            emitMesg("  Layer %-2d %7g %7g %7g %7g %d\n", i,
                lefToMic(startX(i)), lefToMic(startY(i)),
                lefToMic(pitchX(i)), lefToMic(pitchY(i)), vert(i));
        }
        emitMesg("  Area %g,%g  %g,%g\n",
            lefToMic(xLower()), lefToMic(yLower()),
            lefToMic(xUpper()), lefToMic(yUpper()));
    }

    // Compute distance for keepout halo for each route layer.
    for (u_int i = 0; i < numLayers(); i++) {
        setHaloX(i, getViaWidth(i, i, DIR_VERT)/2 + getRouteSpacing(i));
        setHaloY(i, getViaWidth(i, i, DIR_HORIZ)/2 + getRouteSpacing(i));
#ifdef DEBUG
        printf("haloX,Y %g %g\n", lefToMic(db_layers[i].haloX),
            lefToMic(db_layers[i].haloY));
#endif
    }

    if (checknodes)
        checkNodes();

    return (LD_OK);
}


// emitError 
//
// Print an error message giving the line number of the input file on
// which the error occurred.
//
void
cLDDB::emitError(const char *fmt, ...)
{  
    va_list args;

    if (!db_io)
        return;

    if (!fmt) {
        // Special case:  report any errors and reset.
        if (db_errors) {
            emitMesg("Read: encountered %d error%s total.\n",
                db_errors, (db_errors == 1) ? "" : "s");
            db_errors = 0;
        }
        return;
    }

    if (db_errors < LD_MAX_ERRORS) {
        char buf[2048];
        sprintf(buf, "Read, line %d: ", db_currentLine);
        int n = strlen(buf);
        char *e = buf + n;
        va_start(args, fmt);
        vsnprintf(e, 2048 - n, fmt, args);
        va_end(args);
        db_io->emitErrMesg(buf);
    }
    else if (db_errors == LD_MAX_ERRORS)
        db_io->emitErrMesg("Read:  Further errors will not be reported.\n");

    db_errors++;
}


// Print a message to the error channel.
//
void
cLDDB::emitErrMesg(const char *fmt, ...)
{
    if (!db_io)
        return;
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, 2048, fmt, args);
    va_end(args);
    db_io->emitErrMesg(buf);
    db_io->flushErrMesg();
}


// Flush the error message channel.
//
void
cLDDB::flushErrMesg()
{
    if (db_io)
        db_io->flushErrMesg();
}


// Print a message.
//
void
cLDDB::emitMesg(const char *fmt, ...)
{
    if (!db_io)
        return;
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, 2048, fmt, args);
    va_end(args);
    db_io->emitMesg(buf);
}


// Flush the message channel.
//
void
cLDDB::flushMesg()
{
    if (db_io)
        db_io->flushMesg();
}


// Return the layer index from the passed name, which is either a
// layer name, or the 1-based index from user-land.  Return -1 if name
// can't be resolved.
//
int
cLDDB::getLayer(const char *s)
{
    int lnum = -1;
    lefObject *lo = getLefObject(s);
    if (!lo) {
        // The layer name must be an unsigned integer, not, for
        // example, a floating point number.

        for (const char *t = s; *t; t++) {
            if (!isdigit(*t))
                return (-1);
        }
        u_int n;
        if (sscanf(s, "%u", &n) == 1 && n >= 1 && n <= db_numLayers)
            lnum = n - 1;
    }
    else
        lnum = lo->layer;
    return (lnum);
}


// getRouteKeepout
//
// Return the route keepout area, defined as the route space plus 1/2
// the route width.  This is the distance outward from an obstruction
// edge within which one cannot place a route.
//
// If no route layer is defined, then we pick up the value from
// information in the route.cfg file (if any).  Here we define it as
// the route pitch less 1/2 the route width, which is the same as
// above if the route pitch has been chosen for minimum spacing.
//
// If all else fails, return zero.
//
lefu_t
cLDDB::getRouteKeepout(int layer)
{
    lefRouteLayer *rl = getLefRouteLayer(layer);
    if (rl) {
        lefu_t ko = rl->route.width / 2;
        if (rl->route.spacing)
            ko += rl->route.spacing->spacing;
        return (ko);
    }
    return (minPitch(layer) - pathWidth(layer)/2);
}


// getRouteWidth
//
// Similar function to the above.  Return the route width for a route
// layer.  Return value in microns.  If there is no LEF file
// information about the route width, then return half of the minimum
// route pitch.
//
lefu_t
cLDDB::getRouteWidth(int layer)
{
    lefRouteLayer *rl = getLefRouteLayer(layer);
    if (rl)
        return (rl->route.width);
    return (minPitch(layer)/2);
}


// getRouteOffset
//
// Similar function to the above.  Return the route offset for a route
// layer.  Return value in microns.  If there is no LEF file
// information about the route offset, then return half of the minimum
// route pitch.
//
lefu_t
cLDDB::getRouteOffset(int layer, ROUTE_DIR dir)
{
    lefRouteLayer *rl = getLefRouteLayer(layer);
    if (rl)
        return ((dir == DIR_VERT) ? rl->route.offsetX : rl->route.offsetY);
    return (minPitch(layer)/2);
}


// getViaWidth
//
// Determine and return the width of a via.  The first layer is the
// base (lower) layer of the via (e.g., layer 0, or metal1, for
// via12).  The second layer is the layer for which we want the width
// rule (e.g., 0 or 1, for metal1 or metal2).  If dir = 0, return the
// side-to-side width, otherwise, return the top-to-bottom width. 
// This accounts for non-square vias.
//
// Note that Via rectangles are stored with x2 dimensions because the
// center can be on a half-grid position; so, return half the value
// obtained.
//
// This function always uses a horizontally oriented via if available. 
// See the specific getXYViaWidth() function for differentiation
// between via orientations.
//
lefu_t
cLDDB::getViaWidth(int base, int layer, ROUTE_DIR dir)
{
   return (getXYViaWidth(base, layer, dir, 0));
}


// getXYViaWidth
//
// The base routing used by getViaWidth(), with an additional
// argument that specifies which via orientation to use, if an
// alternative orientation is available.  This is necessary for doing
// checkerboard via patterning and for certain standard cells with
// ports that do not always fit one orientation of via.
//
lefu_t
cLDDB::getXYViaWidth(int base, int layer, ROUTE_DIR dir, int orient)
{
    int vid = (orient == 1) ? viaYid(base) : viaXid(base);
    lefObject *lefo = getLefObject(vid);
    if (!lefo) {
        vid = (orient == 1) ? viaXid(base) : viaYid(base);
        lefo = getLefObject(vid);
    }
    if (!lefo) {
        if (base == (int)db_numLayers - 1) {
            vid = (orient == 1) ? viaYid(base-1) : viaXid(base-1);
            lefo = getLefObject(vid);
        }
    }
    if (lefo && (lefo->lefClass == CLASS_VIA ||
            lefo->lefClass == CLASS_VIARULE)) {
        lefViaObject *lefv = (lefViaObject*)lefo;
        if (lefv->via.bot.layer == layer) {
            lefu_t width;
            if (dir == DIR_VERT)
                width = lefv->via.bot.x2 - lefv->via.bot.x1;
            else
                width = lefv->via.bot.y2 - lefv->via.bot.y1;
            return (width/2);
        }
        if (lefv->via.top.layer == layer) {
            lefu_t width;
            if (dir == DIR_VERT)
                width = lefv->via.top.x2 - lefv->via.top.x1;
            else
                width = lefv->via.top.y2 - lefv->via.top.y1;
            return (width/2);
        }
    }
    return (minPitch(layer)/2); // Best guess.
}


// getRouteSpacing
//
// And another such function, for route spacing (minimum width).
//
lefu_t
cLDDB::getRouteSpacing(int layer)
{
    lefRouteLayer *rl = getLefRouteLayer(layer);
    if (rl) {
        if (rl->route.spacing)
            return (rl->route.spacing->spacing);
        return (0);
    }
    return (minPitch(layer)/2);
}


// getRouteWideSpacing
//
// Find route spacing to a routing layer of specific width.
//
lefu_t
cLDDB::getRouteWideSpacing(int layer, lefu_t width)
{
    lefRouteLayer *rl = getLefRouteLayer(layer);
    if (rl) {
        // Prepare a default in case of bad values.

        lefu_t spacing = rl->route.spacing->spacing;
        for (lefSpacingRule *srule = rl->route.spacing; srule;
                srule = srule->next) {
            if (srule->width > width)
                break;
            spacing = srule->spacing;
        }
        return (spacing);
    }
    return (minPitch(layer)/2);
}


// getRoutePitch
//
// Get the route pitch for a given routing layer.
//
lefu_t
cLDDB::getRoutePitch(int layer, ROUTE_DIR dir)
{
    lefRouteLayer *rl = getLefRouteLayer(layer);
    if (rl)
        return ((dir == DIR_VERT) ? rl->route.pitchX : rl->route.pitchY);
    return (minPitch(layer));
}


// getRouteName
//
// Get the route name for a given layer.
//
const char *
cLDDB::getRouteName(int layer)
{
    lefRouteLayer *rl = getLefRouteLayer(layer);
    if (rl)
        return (rl->lefName);
    return (0);
}


// getRouteOrientation
//
// Get the route orientation for the routing layer.
//
ROUTE_DIR
cLDDB::getRouteOrientation(int layer)
{
    lefRouteLayer *rl = getLefRouteLayer(layer);
    if (rl)
        return (rl->route.direction);
    return (DIR_VERT);
}


// Find the gate instance named gatename and return a pointer to it. 
//
dbGate *
cLDDB::getGate(const char *gatename)
{
    if (gatename) {
        if (db_gate_hash) {
            unsigned long ix = db_gate_hash->get(gatename);
            return ((ix == ST_NIL) ? 0 : db_nlGates[ix]);
        }
        if (db_def_case_sens) {
            for (u_int i = 0; i < db_numGates; i++) {
                dbGate *gate = db_nlGates[i];
                if (!strcmp(gate->gatename, gatename))
                    return (gate);
            }
        }
        else {
            for (u_int i = 0; i < db_numGates; i++) {
                dbGate *gate = db_nlGates[i];
                if (!strcasecmp(gate->gatename, gatename))
                    return (gate);
            }
        }
    }
    return (0);
}


// Return the gate array offset.
//
int
cLDDB::getGateNum(const char *gatename)
{
    if (gatename) {
        if (db_gate_hash) {
            unsigned long ix = db_gate_hash->get(gatename);
            return ((ix == ST_NIL) ? -1 : (ix << 1));
        }
        if (db_def_case_sens) {
            for (u_int i = 0; i < db_numGates; i++) {
                dbGate *gate = db_nlGates[i];
                if (!strcmp(gate->gatename, gatename))
                    return (i << 1);
            }
        }
        else {
            for (u_int i = 0; i < db_numGates; i++) {
                dbGate *gate = db_nlGates[i];
                if (!strcasecmp(gate->gatename, gatename))
                    return (i << 1);
            }
        }
    }
    return (-1);
}


// Find the pin instance named pinname and return a pointer to it. 
//
dbGate *
cLDDB::getPin(const char *pinname)
{
    if (pinname) {
        if (db_pin_hash) {
            unsigned long ix = db_pin_hash->get(pinname);
            return ((ix == ST_NIL) ? 0 : db_nlPins[ix]);
        }
        if (db_def_case_sens) {
            for (u_int i = 0; i < db_numPins; i++) {
                dbGate *pin = db_nlPins[i];
                if (!strcmp(pin->gatename, pinname))
                    return (pin);
            }
        }
        else {
            for (u_int i = 0; i < db_numPins; i++) {
                dbGate *pin = db_nlPins[i];
                if (!strcasecmp(pin->gatename, pinname))
                    return (pin);
            }
        }
    }
    return (0);
}


// Get the pin array offset.
//
int
cLDDB::getPinNum(const char *pinname)
{
    if (pinname) {
        if (db_pin_hash) {
            unsigned long ix = db_pin_hash->get(pinname);
            return ((ix == ST_NIL) ? -1 : (ix << 1) | 1);
        }
        if (db_def_case_sens) {
            for (u_int i = 0; i < db_numPins; i++) {
                dbGate *pin = db_nlPins[i];
                if (!strcmp(pin->gatename, pinname))
                    return ((i << 1) | 1);
            }
        }
        else {
            for (u_int i = 0; i < db_numPins; i++) {
                dbGate *pin = db_nlPins[i];
                if (!strcasecmp(pin->gatename, pinname))
                    return ((i << 1) | 1);
            }
        }
    }
    return (-1);
}


dbGate *
cLDDB::getGateOrPinByNum(int num)
{
    if (num < 0)
        return (0);
    if (num & 1) {
        // A pin.

        num >>= 1;
        if (num < (int)db_numPins)
            return (db_nlPins[num]);
    }
    else {
        // A gate.

        num >>= 1;
        if (num < (int)db_numGates)
            return (db_nlGates[num]);
    }
    return (0);
}


// Find the net named "netname" in the list of nets and return a
// pointer to it. 
//
dbNet *
cLDDB::getNet(const char *netname)
{
    if (netname) {
        if (db_net_hash) {
            unsigned long ix = db_net_hash->get(netname);
            return ((ix == ST_NIL) ? 0 : db_nlNets[ix]);
        }
        if (db_def_case_sens) {
            for (u_int i = 0; i < db_numNets; i++) {
                dbNet *net = db_nlNets[i];
                if (!strcmp(net->netname, netname))
                    return (net);
            }
        }
        else {
            for (u_int i = 0; i < db_numNets; i++) {
                dbNet *net = db_nlNets[i];
                if (!strcasecmp(net->netname, netname))
                    return (net);
            }
        }
    }
    return (0);
}


// Find the net with number "num" in the list of nets and return a
// pointer to it.
//
dbNet *
cLDDB::getNetByNum(u_int num)
{
    if (num >= LD_MIN_NETNUM) {
        num -= LD_MIN_NETNUM;
        if (num < db_numNets)
            return (db_nlNets[num]);
    }
    return (0);
}


// getLefGate
//
// Returns the dbGate entry for the cell from the db_lef_gates array.
//
lefMacro *
cLDDB::getLefGate(const char *name)
{
    if (name) {
        if (db_lef_gate_hash) {
            unsigned long ix = db_lef_gate_hash->get(name);
            return ((ix == ST_NIL) ? 0 : db_lef_gates[ix]);
        }
        if (db_lef_case_sens) {
            for (u_int i = 0; i < db_lef_gatecnt; i++) {
                if (!strcmp(db_lef_gates[i]->gatename, name))
                    return (db_lef_gates[i]);
            }
        }
        else {
            for (u_int i = 0; i < db_lef_gatecnt; i++) {
                if (!strcasecmp(db_lef_gates[i]->gatename, name))
                    return (db_lef_gates[i]);
            }
        }
    }
    return (0);
}


// getLefObject
//
// Find a layer record in the list of layers.
//
lefObject *
cLDDB::getLefObject(const char *token)
{

    if (token) {
        if (db_lef_obj_hash) {
            unsigned long ix = db_lef_obj_hash->get(token);
            return ((ix == ST_NIL) ? 0 : db_lef_objects[ix]);
        }
        if (db_lef_case_sens) {
            for (u_int i = 0; i < db_lef_objcnt; i++) {
                if (!strcmp(db_lef_objects[i]->lefName, token))
                    return (db_lef_objects[i]);
            }
        }
        else {
            for (u_int i = 0; i < db_lef_objcnt; i++) {
                if (!strcasecmp(db_lef_objects[i]->lefName, token))
                    return (db_lef_objects[i]);
            }
        }
    }
    return (0);
}


// getLefRouteLayer
//
// Get a routing layer by name.  In addition to the literal layer
// name, we support names like "m1" or "metal2" suggesting a 1-based
// offset into the laters list.
//
lefRouteLayer *
cLDDB::getLefRouteLayer(const char *name)
{
    lefObject *lefo = getLefObject(name);
    if (lefo) {
        if (lefo->lefClass == CLASS_ROUTE)
            return ((lefRouteLayer*)lefo);
        return (0);
    }

    const char *e = name + strlen(name) - 1;
    if (isdigit(*e)) {
        while (e > name && isdigit(*(e-1)))
            e--;
        int num = atoi(e);
        if (num > 0 && num <= (int)numLayers())
            return (getLefRouteLayer(num-1));
    }
    return (0);
}


// getLefRouteLayer
//
// Find a routing layer record in the list of layers, by route layer
// number.
//
lefRouteLayer *
cLDDB::getLefRouteLayer(int layer)
{
    if (layer >= 0 && layer < (int)db_numLayers) {
        int lid = db_layers[layer].lid.lefId;
        if (lid >= 0 && lid < (int)db_lef_objcnt) {
            lefObject *lo = db_lef_objects[lid];
            if (lo->lefClass == CLASS_ROUTE && lo->layer == layer)
                return ((lefRouteLayer*)lo);
        }

        // Above should never fail, but here's some back-up.
        for (u_int i = 0; i < db_lef_objcnt; i++) {
            if (db_lef_objects[i]->lefClass == CLASS_ROUTE) {
                lefRouteLayer *rl = (lefRouteLayer*)db_lef_objects[i];
                if (rl->layer == layer) {
                    db_layers[layer].lid.lefId = rl->lefId;
                    return (rl);
                }
            }
        }
    }
    return (0);
}


void
cLDDB::printInfo(FILE *infoFILEptr)
{
    if (!infoFILEptr)
        return;

    // Print library name and version number at the top.
    fprintf(infoFILEptr, "LDDB-%s\n", LD_VERSION);

    // Resolve pitches.  This is normally done after reading the
    // DEF file, but the info file is usually generated from LEF
    // layer information only, in order to get the values needed
    // to write the DEF file tracks.

    for (u_int i = 0; i < db_numLayers; i++) {
        ROUTE_DIR o = getRouteOrientation(i);

        // Set PitchX and PitchY from route info as
        // checkVariablePitch needs the values

        if (o == DIR_VERT)
            setPitchX(i, getRoutePitch(i, o));
        else
            setPitchY(i, getRoutePitch(i, o));
    }

    // Resolve pitch information similarly to post_config().

    for (u_int i = 1; i < db_numLayers; i++) {
        ROUTE_DIR o = getRouteOrientation(i);

        if ((o == DIR_HORIZ) && (pitchY(i - 1) == 0))
            setPitchY(i - 1, pitchY(i));
        else if ((o == DIR_VERT) && (pitchX(i - 1) == 0))
            setPitchX(i - 1, pitchX(i));
    }

    // Print information about route layers, and exit.
    for (u_int i = 0; i < db_numLayers; i++) {
        lefu_t pitch, width;
        ROUTE_DIR o = getRouteOrientation(i);
        const char *layername = getRouteName(i);

        int vnum, hnum;
        checkVariablePitch(i, &hnum, &vnum);
        if (vnum > 1 && hnum == 1)
            hnum++;     // see note in node.cc
        if (hnum > 1 && vnum == 1)
            vnum++;
            
        if (layername) {
            pitch = (o == DIR_HORIZ) ? pitchY(i) : pitchX(i),
            width = getRouteWidth(i);
            if (pitch == 0 || width == 0)
                continue;
            fprintf(infoFILEptr, "%s %g %g %g %s", layername,
                lefToMic(pitch), lefToMic(getRouteOffset(i, o)),
                lefToMic(width),
                (o == DIR_HORIZ) ? "horizontal" : "vertical");
            if (o == DIR_HORIZ && vnum > 1)
                fprintf(infoFILEptr, " %d", vnum);
            else if (o == DIR_VERT && hnum > 1)
                fprintf(infoFILEptr, " %d", hnum);
            fprintf(infoFILEptr, "\n");
        }
    }
}


// Nodes aren't saved in a way that makes it easy to recall the name
// of the cell and pin to which they belong.  But that information
// doesn't need to be looked up except as a diagnostic output.  This
// function does that lookup.
//
const char *
cLDDB::printNodeName(dbNode *node)
{
    static char *nodestr = 0;

    for (u_int k = 0; k < db_numGates; k++) {
        dbGate *g = db_nlGates[k];
        for (int i = 0; i < g->nodes; i++) {
            if (g->noderec[i] == node) {
               delete [] nodestr;
                nodestr = new char[strlen(g->gatename) +
                    strlen(g->node[i]) + 2];
                sprintf(nodestr, "%s/%s", g->gatename, g->node[i]);
                return (nodestr);
            }
        }
    }
    for (u_int k = 0; k < db_numPins; k++) {
        dbGate *g = db_nlPins[k];
        for (int i = 0; i < g->nodes; i++) {
            if (g->noderec[i] == node) {
               delete [] nodestr;
                nodestr = new char[strlen(g->gatename) +
                    strlen(g->node[i]) + 2];
                sprintf(nodestr, "%s/%s", g->gatename, g->node[i]);
                return (nodestr);
            }
        }
    }
    delete [] nodestr;
    nodestr = new char(22);
    sprintf(nodestr, "(error: no such node)");
    return (nodestr);
}


// printNets
//
// Print the nets list - created from Nlgates list.
//
void
cLDDB::printNets(const char *filename)
{
    FILE *fp;
    if (!filename || !strcmp(filename, "stdout"))
        fp = stdout;
    else {
        fp = fopen(filename, "w");
        if (!fp) {
            emitErrMesg("printNets:  Couldn't open output file\n");
            return;
        }
    }

    for (u_int k = 0; k < db_numPins; k++) {
        dbGate *g = db_nlPins[k];
        fprintf(fp, "%s: %s: nodes->", g->gatename, g->gatetype->gatename);
        for (int i = 0; i < g->nodes; i++) {
            // This prints the first tap position only.
            dbDseg *drect = g->taps[i];
            fprintf(fp, "%s(%g,%g) ", g->node[i], lefToMic(drect->x1),
                lefToMic(drect->y1));
        }
    }
    for (u_int k = 0; k < db_numGates; k++) {
        dbGate *g = db_nlGates[k];
        fprintf(fp, "%s: %s: nodes->", g->gatename, g->gatetype->gatename);
        for (int i = 0; i < g->nodes; i++) {
            // This prints the first tap position only.
            dbDseg *drect = g->taps[i];
            fprintf(fp, "%s(%g,%g) ", g->node[i], lefToMic(drect->x1),
                lefToMic(drect->y1));
        }
    }
    fprintf(fp, "\n");
    if (fp && (fp != stdout))
        fclose(fp);
}


// printNodes
//
// Print the nodes list.
//
void
cLDDB::printNodes(const char *filename)
{
    FILE *fp;
    if (!filename || !strcmp(filename, "stdout"))
        fp = stdout;
    else {
        fp = fopen(filename, "w");
        if (!fp) {
            emitErrMesg("printNodes:  Couldn't open output file\n" );
            return;
        }
    }

    for (u_int i = 0; i < db_numNets; i++) {
        dbNet *net = db_nlNets[i];
        for (dbNode *node = net->netnodes; node; node = node->next) {
            dbNet *nnet = getNetByNum(node->netnum);
            if (nnet != net) {
                fprintf(fp,
                    "Warning: bad net back reference in node of net %s.\n",
                    net->netname);
            }
            /*
            // legacy:  print only the first point
            dbDpoint *dp = node->taps;
            fprintf(fp, "%d\t%s\t(%g,%g)(%d,%d) :%d:num=%d netnum=%d\n",
                node->nodenum, nnet ? nnet->netname : "BAD",
                lefToMic(dp->x), lefToMic(dp->y), dp->gridx, dp->gridy,
                node->netnum, node->numnodes, node->netnum );
            */
            for (dbDpoint *dp = node->taps; dp; dp = dp->next) {
                fprintf(fp, "%d\t%s\t(%g,%g)(%d,%d) :%d:num=%d netnum=%d\n",
                    node->nodenum, nnet ? nnet->netname : "BAD",
                    lefToMic(dp->x), lefToMic(dp->y), dp->gridx, dp->gridy,
                    node->netnum, node->numnodes, node->netnum );
            }
            for (dbDpoint *dp = node->extend; dp; dp = dp->next) {
                fprintf(fp, "%d\t%s\t(%g,%g)(%d,%d) :%d:num=%d netnum=%d\n",
                    node->nodenum, "x",
                    lefToMic(dp->x), lefToMic(dp->y), dp->gridx, dp->gridy,
                    node->netnum, node->numnodes, node->netnum );
            }
        }
    }
    if (fp && (fp != stdout))
        fclose(fp);
}


// printRoutes
//
// Print the routes list.
//
void
cLDDB::printRoutes(const char *filename )
{
    FILE *fp;
    if (!filename || !strcmp(filename, "stdout"))
        fp = stdout;
    else {
        fp = fopen( filename, "w" );
        if (!fp) {
            emitErrMesg("printRoutes:  Couldn't open output file\n" );
            return;
        }
    }

    for (u_int k = 0; k < db_numPins; k++) {
        dbGate *g = db_nlPins[k];
        fprintf(fp, "%s: %s: nodes->", g->gatename, g->gatetype->gatename);
        for (int i = 0; i < g->nodes; i++)
            fprintf(fp, "%s ", g->node[i]);
        fprintf(fp, "\n");
    }
    for (u_int k = 0; k < db_numGates; k++) {
        dbGate *g = db_nlGates[k];
        fprintf(fp, "%s: %s: nodes->", g->gatename, g->gatetype->gatename);
        for (int i = 0; i < g->nodes; i++)
            fprintf(fp, "%s ", g->node[i]);
        fprintf(fp, "\n");
    }
    if (fp && (fp != stdout))
        fclose(fp);
}


// printNlgates
//
// Print the nlgate list.
//
void
cLDDB::printNlgates(const char *filename )
{
    FILE *fp;
    if (!filename || !strcmp(filename, "stdout"))
        fp = stdout;
    else {
        fp = fopen( filename, "w" );
        if (!fp) {
            emitErrMesg("printNlgates.  Couldn't open output file\n" );
            return;
        }
    }

    for (u_int k = 0; k < db_numPins; k++) {
        dbGate *g = db_nlPins[k];
        fprintf(fp, "%s: %s: nodes->", g->gatename, g->gatetype->gatename);
        for (int i = 0; i < g->nodes; i++) {
            // This prints the first tap position only.
            dbDseg *drect = g->taps[i];
            fprintf(fp, "%s(%g,%g)", g->node[i], lefToMic(drect->x1),
                lefToMic(drect->y1));
        }
        fprintf(fp, "\n");
    }
    for (u_int k = 0; k < db_numGates; k++) {
        dbGate *g = db_nlGates[k];
        fprintf(fp, "%s: %s: nodes->", g->gatename, g->gatetype->gatename);
        for (int i = 0; i < g->nodes; i++) {
            // This prints the first tap position only.
            dbDseg *drect = g->taps[i];
            fprintf(fp, "%s(%g,%g)", g->node[i], lefToMic(drect->x1),
                lefToMic(drect->y1));
        }
        fprintf(fp, "\n");
    }
    if (fp && (fp != stdout))
        fclose(fp);
}


// printNlnets
//
// Print the nets.
//
void
cLDDB::printNlnets(const char *filename)
{
    FILE *fp;
    if (!filename || !strcmp(filename, "stdout"))
        fp = stdout;
    else {
        fp = fopen(filename, "w");
        if (!fp) {
            emitErrMesg("printNlnets.  Couldn't open output file\n");
            return;
        }
    }

    for (u_int i = 0; i < db_numNets; i++) {
        dbNet *net = db_nlNets[i];
        fprintf(fp, "%d\t#=%d\t%s   \t\n", net->netnum, net->numnodes,
            net->netname);

        for (dbNode *nd = net->netnodes; nd; nd = nd->next)
            fprintf(fp, "%d ", nd->nodenum);
    }
    fprintf(fp, "%d nets\n", db_numNets);

    if (fp && (fp != stdout))
        fclose(fp);
}


// printNet
//
// Print info about the net to the message channel.
//
void
cLDDB::printNet(dbNet *net)
{
    if (!net) {
        emitErrMesg("printNet:  Null pointer received!\n");
        return;
    }
    emitMesg("Net %d: %s", net->netnum, net->netname);
    for (dbNode *node = net->netnodes; node != NULL; node = node->next) {
        emitMesg("\n  Node %d: \n    Taps: ", node->nodenum);
        int i = 0;
        bool first = true;
        for (dbDpoint *tap = node->taps; tap; tap = tap->next) {
            emitMesg("%sL%d:(%.2lf,%.2lf)",
                (i == 0 ? (first ? "" : "\n        ") : " "),
                tap->layer, tap->x, tap->y);
            i = (i + 1) % 4;
            first = false;
        }
        emitMesg("\n    Tap extends: ");
        i = 0;
        first = true;
        for (dbDpoint *tap = node->extend; tap; tap = tap->next) {
            emitMesg("%sL%d:(%.2lf,%.2lf)",
                (i == 0 ? (first ? "" : "\n        ") : " "),
                tap->layer, tap->x, tap->y);
            i = (i + 1) % 4;
            first = false;
        }
    }
    emitMesg("\n  bbox: (%d,%d)-(%d,%d)\n",
        net->xmin, net->ymin, net->xmax, net->ymax);
}


// printGate
//
// Print info about the net to the message channel.
//
void
cLDDB::printGate(dbGate *gate)
{
    if (!gate) {
        emitErrMesg("printGate:  Null pointer received!\n");
        return;
    }
    emitMesg("Gate %s\n", gate->gatename);
    emitMesg("  Loc: (%.2lf, %.2lf), WxH: %.2lfx%.2lf\n",
        gate->placedX, gate->placedY, gate->width, gate->height);
    emitMesg("  Pins");
    for (int i = 0; i < gate->nodes; i++) {
        emitMesg("\n    Pin %s, net %d\n", gate->node[i], gate->netnum[i]);
        emitMesg("      Segs: ");

        int j = 0;
        bool first = true;
        for (dbDseg *seg = gate->taps[i]; seg; seg = seg->next) {
            emitMesg("%sL%d:(%.2lf,%.2lf)-(%.2lf,%.2lf)",
                (j == 0 ? (first ? "" : "\n        ") : " "),
                seg->layer, seg->x1, seg->y1, seg->x2, seg->y2);
            j = (j + 1) % 3;
            first = false;
        }
        dbNode *node;
        if ((node = gate->noderec[i]) != 0) {
            emitMesg("\n      Taps: ");
            j = 0;
            first = true;
            for (dbDpoint *tap = node->taps; tap; tap = tap->next) {
                emitMesg("%sL%d:(%.2lf,%.2lf)",
                    (j == 0 ? (first ? "" : "\n        ") : " "),
                    tap->layer, tap->x, tap->y);
                j = (j + 1) % 4;
                first = false;
            }
            emitMesg("\n      Tap extends: ");
            j = 0;
            first = true;
            for (dbDpoint *tap = node->extend; tap; tap = tap->next) {
                emitMesg("%sL%d:(%.2lf,%.2lf)",
                    (j == 0 ? (first ? "" : "\n        ") : " "),
                    tap->layer, tap->x, tap->y);
                j = (j + 1) % 4;
                first = false;
            }
        }
    }
    emitMesg("\n  Obstructions: ");
    int j = 0;
    bool first = true;
    for (dbDseg *seg = gate->obs; seg; seg = seg->next) {
        emitMesg("%sL%d:(%.2lf,%.2lf)-(%.2lf,%.2lf)",
            (j == 0 ? (first ? "" : "\n    ") : " "),
            seg->layer, seg->x1, seg->y1, seg->x2, seg->y2);
        j = (j + 1) % 3;
        first = false;
    }
    emitMesg("\n");
}


// checkVariablePitch
//
// This function is used by the function below it to generate
// obstructions that force routes to be placed in 1-of-N tracks. 
// However, it is also used to determine the same information for the
// .info file, so that the effective pitch is output, not the pitch
// copied from the LEF file.  Output is the vertical and horizontal
// pitch multipliers, passed back through pointers.
//
void
cLDDB::checkVariablePitch(int l, int *hptr, int *vptr)
{
    ROUTE_DIR o = getRouteOrientation(l);

    // Pick the best via size for the layer.  Usually this means the
    // via whose base is at layer - 1, because the top metal layer will
    // either have the same width or a larger width. 

    // Note that when "horizontal" (o = 1) is passed to getViaWidth,
    // it returns the via width side-to-side; but for horizontal
    // routing the dimension of interest is the height of the via. 
    // Therefore the direction argument passed to getViaWidth is
    // (1 - o).

    ROUTE_DIR dopp = (o == DIR_VERT) ? DIR_HORIZ : DIR_VERT;
    lefu_t wvia;
    if (l == 0)
        wvia = getViaWidth(l, l, dopp);
    else
        wvia = getViaWidth(l - 1, l, dopp);

    lefu_t vpitch = 0, hpitch = 0;
    if (o == DIR_HORIZ) {       // Horizontal route.
        vpitch = getRoutePitch(l, o);
        // Changed:  routes must be able to accomodate the placement
        // of a via in the track next to it.

        // hpitch = getRouteWidth(l) + getRouteSpacing(l);
        hpitch = (getRouteWidth(l) + wvia)/2 + getRouteSpacing(l);
    }
    else if (o == DIR_VERT) {   // Vertical route
        hpitch = getRoutePitch(l, o);
        // vpitch = getRouteWidth(l) + getRouteSpacing(l);
        vpitch = (getRouteWidth(l) + wvia)/2 + getRouteSpacing(l);
    }

    int vnum = 1;
    while (vpitch > pitchY(l)) {
        vpitch /= 2;
        vnum++;
    }
    int hnum = 1;
    while (hpitch > pitchX(l)) {
        hpitch /= 2;
        hnum++;
    }

    *vptr = vnum;
    *hptr = hnum;
}


// Add an obstruction to the list, if it is not there already. 
// Presently, only obstructions on conductors are allowed, some day
// add via layer support.
//
void
cLDDB::addObstruction(lefu_t xl, lefu_t yl, lefu_t xu, lefu_t yu, u_int l)
{
    if (l < db_numLayers && !findObstruction(xl, yl, xu, yu, l)) {
        lefRouteLayer *rl = getLefRouteLayer(l);
        int id = rl ? rl->lefId : -1;
        db_userObs = new dbDseg(xl, yl, xu, yu, l, id, db_userObs);
    }
}


// Hunt for a matching obstruction, return if found.
//
dbDseg *
cLDDB::findObstruction(lefu_t xl, lefu_t yl, lefu_t xu, lefu_t yu, u_int l)
{
    if (l < db_numLayers) {
        for (dbDseg *s = db_userObs; s; s = s->next) {
            if (s->layer != (int)l)
                continue;
            if (s->x1 != xl)
                continue;
            if (s->y1 != yl)
                continue;
            if (s->x2 != xu)
                continue;
            if (s->y2 != yu)
                continue;
            return (s);
        }
    }
    return (0);
}

// The remaining functions are protected.


//
// Support functions for polygon conversion to rectangles.
//

#define HEDGE 0         // Horizontal edge
#define REDGE 1         // Rising edge
#define FEDGE -1        // Falling edge

namespace {
    // lowX
    //
    // Sort function, ascending in x.
    //
    inline bool lowX(const dbDpoint *p, const dbDpoint *q)
    {
        return (p->x < q->x);
    }


    // lowY
    //
    // Sort function, ascending in y.
    //
    inline bool lowY(const dbDpoint *p, const dbDpoint *q)
    {
        return (p->y < q->y);
    }


    // orient
    //
    // Assign a direction to each of the edges in a polygon.
    //
    // Note that edges have been sorted, but retain the original
    // linked list pointers, from which we can determine the path
    // orientation.
    //
    bool orient(dbDpoint **edges, int nedges, int *dir)
    {
        for (int n = 0; n < nedges; n++) {
            dbDpoint *p = edges[n];
            dbDpoint *q = edges[n]->next;

            if (p->y == q->y) {
                dir[n] = HEDGE;
                continue;
            }
            if (p->x == q->x) {
                if (p->y < q->y) {
                    dir[n] = REDGE;
                    continue;
                }
                if (p->y > q->y) {
                    dir[n] = FEDGE;
                    continue;
                }
                // Point connects to itself.
                dir[n] = HEDGE;
                continue;
            }
            // It's not Manhattan, folks.
            return (false);
        }
        return (true);
    }


    // cross
    //
    // See if an edge crosses a particular area.  Return true if edge
    // if vertical and if it crosses the y-range defined by ybot and
    // ytop.  Otherwise return false.
    //
    bool cross(dbDpoint *edge, int dir, lefu_t ybot, lefu_t ytop)
    {
        lefu_t ebot, etop;
        switch (dir) {
        case REDGE:
            ebot = edge->y;
            etop = edge->next->y;
            return (ebot <= ybot && etop >= ytop);

        case FEDGE:
            ebot = edge->next->y;
            etop = edge->y;
            return (ebot <= ybot && etop >= ytop);
        }
        return (false);
    }
}

 
// polygonToRects
//
// Convert Geometry information from a POLYGON statement into
// rectangles.  NOTE:  For now, this function assumes that all points
// are Manhattan.  It will flag non-Manhattan geometry.
//
// the DSEG pointed to by rectListPtr is updated by having the list of
// rectangles appended to it.
//
void
cLDDB::polygonToRects(dbDseg **rectListPtr, dbDpoint *pointlist)
{
    if (!pointlist)
        return;

    // Close the path by duplicating 1st point if necessary.

    dbDpoint *ptail = pointlist;
    while (ptail->next)
        ptail = ptail->next;

    if ((ptail->x != pointlist->x) || (ptail->y != pointlist->y)) {
        dbDpoint *p = new dbDpoint(*pointlist);
        p->next = 0;
        ptail->next = p;
    }

    // To do:  Break out non-manhattan parts here.
    // See CIFMakeManhattanPath in magic-8.0

    dbDseg *rex = 0;
    int npts = 0;
    for (dbDpoint *p = pointlist; p->next; p = p->next, npts++);
    dbDpoint **pts = new dbDpoint*[npts];
    dbDpoint **edges = new dbDpoint*[npts];
    int *dir = new int[npts];

    npts = 0;
    for (dbDpoint *p = pointlist; p->next; p = p->next, npts++) {
        // pts and edges are two lists of pointlist entries
        // that are NOT linked lists and can be shuffled
        // around by qsort().  The linked list "next" pointers
        // *must* be retained.

        pts[npts] = p;
        edges[npts] = p;
    }

    if (npts < 4) {
        emitError("Polygon with fewer than 4 points.\n");
        goto done;
    }

    // Sort points by low y, edges by low x.
    std::sort(pts, pts + npts, lowY);
    std::sort(edges, edges + npts, lowX);

    // Find out which direction each edge points.

    if (!orient(edges, npts, dir)) {
        emitError("I can't handle non-manhattan polygons!\n");
        goto done;
    }

    // Scan the polygon from bottom to top.  At each step, process a
    // minimum-sized y-range of the polygon (i.e., a range such that
    // there are no vertices inside the range).  Use wrap numbers
    // based on the edge orientations to determine how much of the
    // x-range for this y-range should contain material.

    for (int curr = 1; curr < npts; curr++) {
        // Find the next minimum-sized y-range.

        lefu_t ybot = pts[curr - 1]->y;
        while (ybot == pts[curr]->y) {
            if (++curr >= npts)
                goto done;
        }
        lefu_t ytop = pts[curr]->y;

        // Process all the edges that cross the y-range, from left to
        // right.

        lefu_t xbot = 0;
        for (int wrapno = 0, n = 0; n < npts; n++) {
            if (wrapno == 0)
                xbot = edges[n]->x;
            if (!cross(edges[n], dir[n], ybot, ytop))
                continue;
            wrapno += (dir[n] == REDGE) ? 1 : -1;
            if (wrapno == 0) {
                lefu_t xtop = edges[n]->x;
                if (xbot == xtop)
                    continue;
                rex = new dbDseg(xbot, ybot, xtop, ytop,
                    edges[n]->layer, edges[n]->lefId, rex);
            }
        }
    }

done:
    delete [] edges;
    delete [] dir;
    delete [] pts;

    if (*rectListPtr == 0)
        *rectListPtr = rex;
    else {
        dbDseg *rtail = *rectListPtr;
        while (rtail->next)
            rtail = rtail->next;
        rtail->next = rex;
    }
}


// lookup 
//
// Searches a table of strings to find one that matches a given
// string.  It's useful mostly for command lookup.
//
// Only the portion of a string in the table up to the first blank
// character is considered significant for matching.
//
// Results:
// If str is the same as or an unambiguous abbreviation for one of the
// entries in table, then the index of the matching entry is returned. 
// If str is not the same as any entry in the table, but an
// abbreviation for more than one entry, then -1 is returned.  If str
// doesn't match any entry, then -2 is returned.  Case differences are
// ignored.
//
// NOTE:  
// Table entries need no longer be in alphabetical order and they need
// not be lower case.  The irouter command parsing depends on these
// features.
//
// char *str;
// Pointer to a string to be looked up
//
// char *(table[]);
// Pointer to an array of string pointers which are the valid
// commands.  The end of the table is indicated by a null string.
//
int
cLDDB::lookup(const char *str, const char **table)
{
    if (!str)
        return (-1);
    int match = -2;     // result, initialized to -2 = no match
    int ststart = 0;

    // search for match
    for (int pos=0; table[pos]; pos++) {
        const char *tabc = table[pos];
        const char *strc = &(str[ststart]);
        while (*strc!='\0' && *tabc!=' ' &&
            ((*tabc==*strc) ||
             (isupper(*tabc) && islower(*strc) && (tolower(*tabc)== *strc))||
             (islower(*tabc) && isupper(*strc) && (toupper(*tabc)== *strc)) )) {
            strc++;
            tabc++;
        }

        if (*strc=='\0') {
            // entry matches
            if (*tabc==' ' || *tabc=='\0') {
                // Exact match - record it and terminate search.
                match = pos;
                break;
            }    
            else if (match == -2) {
                // Inexact match and no previous match - record this one 
                // and continue search.
                match = pos;
            }   
            else {
                // previous match, so string is ambiguous unless exact
                // match exists.  Mark ambiguous for now, and continue
                // search.
                match = -1;
            }
        }
    }
    return (match);
}


// Go through all nodes and remove any tap or extend entries that are
// out of bounds.
//
void
cLDDB::checkNodes()
{
    for (u_int i = 0; i < numNets(); i++) {
        dbNet *net = nlNet(i);
        for (dbNode *node = net->netnodes; node; node = node->next) {

            dbDpoint *ltap = 0;
            for (dbDpoint *ctap = node->taps; ctap; ) {
                dbDpoint *ntap = ctap->next;
                int glimitx = numChannelsX(ctap->layer);
                int glimity = numChannelsY(ctap->layer);
#ifdef LD_SIGNED_GRID
                if (ctap->gridx < 0 || ctap->gridx >= glimitx ||
                        ctap->gridy < 0 || ctap->gridy >= glimity) {
#else
                if (ctap->gridx >= glimitx || ctap->gridy >= glimity) {
#endif
                    // Remove ctap.
                    if (!ltap)
                        node->taps = ntap;
                    else
                        ltap->next = ntap;
                }
                else
                    ltap = ctap;
                ctap = ntap;
            }

            ltap = 0;
            for (dbDpoint *ctap = node->extend; ctap; ) {
                dbDpoint *ntap = ctap->next;
                int glimitx = numChannelsX(ctap->layer);
                int glimity = numChannelsY(ctap->layer);

#ifdef LD_SIGNED_GRID
                if (ctap->gridx < 0 || ctap->gridx >= glimitx ||
                        ctap->gridy < 0 || ctap->gridy >= glimity) {
#else
                if (ctap->gridx >= glimitx || ctap->gridy >= glimity) {
#endif
                    // Remove ctap.
                    if (!ltap)
                        node->taps = ntap;
                    else
                        ltap->next = ntap;
                }
                else
                    ltap = ctap;
                ctap = ntap;
            }
        }
    }
}
// End of cLDDB functions.


// Print a message to the error channel.
//
void
cLDstdio::emitErrMesg(const char *str)
{
    fputs(str, stderr);
    fflush(stderr);
}


// Flush the message channel.
//
void
cLDstdio::flushErrMesg()
{
    fflush(stderr);
}


// Print a message.
//
void
cLDstdio::emitMesg(const char *str)
{
    fputs(str, stdout);
}


// Flush the message channel.
//
void
cLDstdio::flushMesg()
{
    fflush(stdout);
}


// Destroy function.  This should be called and not the destructor
// directly.
//
void
cLDstdio::destroy()
{
    delete this;
}
// End of cLDstdio functions.


//
// Utilities for parsing/generating the orientation code.  The
// numerical value is the same as used in the Cadence LEF/DEF kit.
//

// Static function.
// Return the ORIENT_CODE for the string, or ORIENT_NORTH if not
// recognized.
//
ORIENT_CODE
dbGate::orientation(const char *str)
{
    if (str) {
        if (!str[1]) {
            if (str[0] == 'N' || str[0] == 'n')
                return (ORIENT_NORTH);
            if (str[0] == 'W' || str[0] == 'w')
                return (ORIENT_WEST);
            if (str[0] == 'S' || str[0] == 's')
                return (ORIENT_SOUTH);
            if (str[0] == 'E' || str[0] == 'e')
                return (ORIENT_EAST);
        }
        else if ((str[0] == 'F' || str[0] == 'f') && !str[2]) {
            if (str[1] == 'N' || str[1] == 'n')
                return (ORIENT_FLIPPED_NORTH);
            if (str[1] == 'W' || str[1] == 'w')
                return (ORIENT_FLIPPED_WEST);
            if (str[1] == 'S' || str[1] == 's')
                return (ORIENT_FLIPPED_SOUTH);
            if (str[1] == 'E' || str[1] == 'e')
                return (ORIENT_FLIPPED_EAST);
        }
    }
    return (ORIENT_NORTH);
}


// Static function.
// Return the LEF/DEF orientation code token for the value, "N" if not
// recognized.
//
const char *
dbGate::orientationStr(int code)
{
    switch (code) {
    case ORIENT_NORTH:
        return ("N");
    case ORIENT_WEST:
        return ("W");
    case ORIENT_SOUTH:
        return ("S");
    case ORIENT_EAST:
        return ("E");
    case ORIENT_FLIPPED_NORTH:
        return ("FN");
    case ORIENT_FLIPPED_WEST:
        return ("FW");
    case ORIENT_FLIPPED_SOUTH:
        return ("FS");
    case ORIENT_FLIPPED_EAST:
        return ("FE");
    }
    return ("N");
}

