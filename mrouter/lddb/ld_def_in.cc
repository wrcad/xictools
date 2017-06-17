
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
 $Id: ld_def_in.cc,v 1.13 2017/02/15 01:39:36 stevew Exp $
 *========================================================================*/

#include "lddb_prv.h"
#include "ld_hash.h"
#include "defrReader.hpp"
#include <errno.h>
#include <math.h>


//
// LEF/DEF Database.
//
// DEF reader, using the Cadence DEF parser.  The DEF file should have
// information on die are, track placement, pins, components, and
// nets.  Should be called after LEF.  Currently, routed nets are
// parsed and existing routes are ignored.
//
// To-do:  Routed nets should have their routes dropped into track
// obstructions, and the nets should be ignored.
//


//#define DEBUG

namespace {
    void lineNumberCB(int n)
    {
        cLDDB *db = (cLDDB*)defrGetUserData();
        db->setCurrentLine(n);
    }
  
    void errorCB(const char* msg)
    {
        cLDDB *db = (cLDDB*)defrGetUserData();
        db->emitErrMesg("%s\n", msg);
    }
  
    void warningCB(const char* msg)
    {
        cLDDB *db = (cLDDB*)defrGetUserData();
        db->emitMesg("%s\n", msg);
    }

    int versionCB(defrCallbackType_e, double v, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defSetCaseSens(v > 5.55));
    }

    int caseSensCB(defrCallbackType_e, int v, void *data)
    {
        // This won't be called for version 5.6 and later, the
        // NAMECASESENSITIVE statement should not appear.  The parser
        // will do the right thing:  emit a warning and ignore the
        // setting.  Case sensitivity is on for 5.6 and later,
        // defaulting off for earlier.

        cLDDB *db = (cLDDB*)data;
        return (db->defSetCaseSens(v));
    }

    int technologyCB(defrCallbackType_e, const char *str, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defTechnologySet(str));
    }

    int designCB(defrCallbackType_e, const char *str, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defDesignSet(str));
    }

    int unitsCB(defrCallbackType_e, double d, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defUnitsSet(d));
    }

    int tracksCB(defrCallbackType_e, defiTrack *trk, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defTracksSet(trk));
    }

    int dieAreaCB(defrCallbackType_e, defiBox *box, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defDieAreaSet(box));
    }

    int componentsBeginCB(defrCallbackType_e, int num, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defComponentsBegin(num));
    }

    int componentsCB(defrCallbackType_e, defiComponent *cmp, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defComponentsSet(cmp));
    }

    int componentsEndCB(defrCallbackType_e, void*, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defComponentsEnd());
    }

    int blockagesBeginCB(defrCallbackType_e, int num, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defBlockagesBegin(num));
    }

    int blockagesCB(defrCallbackType_e, defiBlockage *blk, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defBlockagesSet(blk));
    }

    int blockagesEndCB(defrCallbackType_e, void*, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defBlockagesEnd());
    }

    int viasBeginCB(defrCallbackType_e, int num, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defViasBegin(num));
    }

    int viasCB(defrCallbackType_e, defiVia *via, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defViasSet(via));
    }

    int viasEndCB(defrCallbackType_e, void*, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defViasEnd());
    }

    int pinsBeginCB(defrCallbackType_e, int num, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defPinsBegin(num));
    }

    int pinsCB(defrCallbackType_e, defiPin *pin, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defPinsSet(pin));
    }

    int pinsEndCB(defrCallbackType_e, void*, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defPinsEnd());
    }

    int netsBeginCB(defrCallbackType_e, int num, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defNetsBegin(num));
    }

    int netsCB(defrCallbackType_e, defiNet *net, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defNetsSet(net));
    }

    int netsEndCB(defrCallbackType_e, void*, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defNetsEnd());
    }

    int specialNetsBeginCB(defrCallbackType_e, int num, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defSpecialNetsBegin(num));
    }

    int specialNetsCB(defrCallbackType_e, defiNet *net, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defSpecialNetsSet(net));
    }

    int specialNetsEndCB(defrCallbackType_e, void*, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->defSpecialNetsEnd());
    }
}


// defRead
//
// Read a .def file and parse die area, track positions, components,
// pins, and nets.
//
// Results:
// Sets the units scale, so the routed output can be scaled to
// match the DEF file header.
//
bool
cLDDB::defRead(const char *filename)
{
    if (!filename) {
        emitErrMesg("defRead: Error, null filename.\n");
        return (LD_BAD);
    }
    long time0 = millisec();

    defrInit();
    defrSetLineNumberFunction(lineNumberCB);
    defrSetDeltaNumberLines(1);
    defrSetLogFunction(errorCB);
    defrSetWarningLogFunction(warningCB);

    defrSetVersionCbk(versionCB);
    defrSetCaseSensitiveCbk(caseSensCB);
    defrSetTechnologyCbk(technologyCB);
    defrSetDesignCbk(designCB);
    defrSetUnitsCbk(unitsCB);
    defrSetTrackCbk(tracksCB);
    defrSetDieAreaCbk(dieAreaCB);
    defrSetComponentStartCbk(componentsBeginCB);
    defrSetComponentCbk(componentsCB);
    defrSetComponentEndCbk(componentsEndCB);
    defrSetBlockageStartCbk(blockagesBeginCB);
    defrSetBlockageCbk(blockagesCB);
    defrSetBlockageEndCbk(blockagesEndCB);
    defrSetViaStartCbk(viasBeginCB);
    defrSetViaCbk(viasCB);
    defrSetViaEndCbk(viasEndCB);
    defrSetStartPinsCbk(pinsBeginCB);
    defrSetPinCbk(pinsCB);
    defrSetPinEndCbk(pinsEndCB);
    defrSetSNetStartCbk(specialNetsBeginCB);
    defrSetSNetCbk(specialNetsCB);
    defrSetSNetEndCbk(specialNetsEndCB);
    defrSetNetStartCbk(netsBeginCB);
    defrSetNetCbk(netsCB);
    defrSetNetEndCbk(netsEndCB);

    defrSetAddPathToNet();

    FILE *f = fopen(filename, "r");
    if (!f) {
        emitErrMesg("defRead: Error, cannot open input file: %s.\n",
            strerror(errno));
        return (LD_BAD);
    }

    // Initialize state.
    db_def_total        = 0;
    db_def_processed    = 0;
    db_def_corient      = '.';
    db_errors           = 0;
    db_currentLine      = 0;

    // Finalize LEF stuff.
    lefPostSetup();

    if (db_verbose > 0) {
        emitMesg("Reading DEF data from file %s.\n",
            lddb::strip_path(filename));
        flushMesg();
    }
    int res = defrRead(f, filename, this, 1);
    if (res)
        emitErrMesg("defRead: Warning, DEF reader returned bad status.\n");

    defrReleaseNResetMemory();
    defrUnsetCallbacks();

    defrUnsetTechnologyCbk();
    defrUnsetDesignCbk();
    defrUnsetUnitsCbk();
    defrUnsetTrackCbk();
    defrUnsetDieAreaCbk();
    defrUnsetComponentStartCbk();
    defrUnsetComponentCbk();
    defrUnsetComponentEndCbk();
    defrUnsetBlockageStartCbk();
    defrUnsetBlockageCbk();
    defrUnsetBlockageEndCbk();
    defrUnsetViaStartCbk();
    defrUnsetViaCbk();
    defrUnsetViaEndCbk();
    defrUnsetStartPinsCbk();
    defrUnsetPinCbk();
    defrUnsetPinEndCbk();
    defrUnsetSNetStartCbk();
    defrUnsetSNetCbk();
    defrUnsetSNetEndCbk();
    defrUnsetNetStartCbk();
    defrUnsetNetCbk();
    defrUnsetNetEndCbk();

    // Release allocated singleton data.
    defrClear();

    if (db_verbose > 0) {
        long elapsed = millisec() - time0;
        emitMesg("DEF read: Processed %d lines in %ld milliseconds.\n",
            db_currentLine, elapsed);
    }
    emitError(0);     // Print statement of errors, if any, and reset.

    // Cleanup

    if (f)
        fclose(f);

    flushErrMesg();
    flushMesg();

    return (res);
}


void
cLDDB::defReset()
{
    // DEF info.
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
    db_userObs->free();
    db_userObs          = 0;
    db_intObs->free();
    db_intObs           = 0;
    db_numGates         = 0;
    db_numPins          = 0;
    db_numNets          = 0;
    db_xLower           = 0;
    db_xUpper           = 0;
    db_yLower           = 0;
    db_yUpper           = 0;

    // DEF parser state.
    db_def_resol        = LEFDEF_DEFAULT_RESOL;
    db_def_total        = 0;
    db_def_processed    = 0;
    db_def_netidx       = LD_MIN_NETNUM;
    db_def_corient      = '.';
    db_def_case_sens    = true;
    db_def_resol_set    = false;
    db_def_tracks_set   = false;
}


bool
cLDDB::defSetCaseSens(bool b)
{
    db_def_case_sens = b;
    return (LD_OK);
}


bool
cLDDB::defTechnologySet(const char *str)
{
    setTechnology(str);
    return (LD_OK);
}


bool
cLDDB::defDesignSet(const char *str)
{
    setDesign(str);
    return (LD_OK);
}


// DEF scaling:
//  db_def_resol is the DEF DATABASE UNITS value.
//  DEF length values are stored as microns, snapped to the
//    db_def_resol grid.
//  The db_def_resol can only take certain values relative to
//  the db_lef_resol, checked when set.
//
bool
cLDDB::defUnitsSet(double d)
{
    u_int resol = (u_int)(fabs(d) + 0.5);
    return (defResolSet(resol));
}


bool
cLDDB::defResolSet(u_int resol)
{
    if (db_def_resol_set) {
        if (resol != (u_int)db_def_resol) {
            emitErrMesg(
                "DEF database resolution already set, new value ignored.\n");
        }
        return (LD_OK);
    }

    // DEF only allows certain values, check this.
    switch (resol) {
    case 100:
    case 200:
    case 400:
    case 800:
    case 1000:
    case 2000:
    case 4000:
    case 8000:
    case 10000:
    case 20000:
        break;
    default:
        emitErrMesg(
            "Error: DEF dbu/micron %d is not an accepted value.\n", resol);
        return (LD_BAD);
    }

    if (db_lef_resol % resol != 0) {
        emitErrMesg(
            "Error: DEF dbu/micron %d is numerically incompatible "
            "with LEF\ndbu/micron %d.\n",
            resol, db_lef_resol);
        return (LD_BAD);
    }
    db_def_resol = resol;
    db_def_resol_set = true;

    return (LD_OK);
}


bool
cLDDB::defTracksSet(defiTrack *trk)
{
    const char *token = trk->macro();  // null return possible?
    if (!token || strlen(token) != 1) {
        emitError(
        "defRead: Missing or unknown track orientation (requires X or Y).\n");
        if (!token)
            token = "";
    }
    else {
        switch (*token) {
        case 'X':
        case 'x':
            db_def_corient = 'x';
            break;
        case 'Y':
        case 'y':
            db_def_corient = 'y';
            break;
        default:
            emitError(
                "defRead: Unknown track orientation (requires X or Y).\n");
        }
    }
    int channels = trk->xNum();
    double start = trk->x();
    double step = trk->xStep();

    for (int i = 0; i < trk->numLayers(); i++) {
        token = trk->layer(i);
        lefRouteLayer *rl = getLefRouteLayer(token);
        if (!rl) {
            emitError(
                "defRead: Warning, TRACKS: unknown routing layer \"%s\", "
                "ignored.\n",
                token);
            continue;
        }
        int curlayer = rl->layer;

        if (db_def_corient == 'x') {
            // If X, routing channels are North/South, and pitch, etc. 
            // take the X suffix.   vert(layer) returns true;

            db_layers[curlayer].trackXset = true;
            setVert(curlayer, true);
            setStartX(curlayer, defToLefGrid(start));
            setPitchX(curlayer, defToLefGrid(step));
            setNumChannelsX(curlayer, channels);
        }
        else {
            // If Y, routing channels are East/West, and pitch, etc. 
            // take the Y suffix.   vert(layer) returns false;

            db_layers[curlayer].trackYset = true;
            setVert(curlayer, false);
            setStartY(curlayer, defToLefGrid(start));
            setPitchY(curlayer, defToLefGrid(step));
            setNumChannelsY(curlayer, channels);
        }
    }
    return (LD_OK);
}


bool
cLDDB::defDieAreaSet(defiBox *box)
{
    db_xLower = defToLefGrid(box->xl());
    db_yLower = defToLefGrid(box->yl());
    db_xUpper = defToLefGrid(box->xh());
    db_yUpper = defToLefGrid(box->yh());
    return (LD_OK);
}


bool
cLDDB::defComponentsBegin(int num)
{
    bool ret = defFinishTracks();
    db_def_total = num;
    db_def_processed = 0;

    if (db_numGates == 0) {
        db_nlGates = new dbGate*[db_def_total];
        for (u_int i = 0; i < db_def_total; i++)
            db_nlGates[i] = 0;
    }
    else {
        // Support incremental...
        dbGate **ogates = db_nlGates;
        db_nlGates = new dbGate*[db_numGates + db_def_total];
        for (u_int i = 0; i < db_numGates; i++)
            db_nlGates[i] = ogates[i];
        delete [] ogates;
        for (u_int i = db_numGates; i < (db_numGates + db_def_total); i++)
            db_nlGates[i] = 0;
    }

    // Create or expand hash table for gate names.
    if (!db_gate_hash) {
        if (db_numGates + db_def_total > 16) {
            db_gate_hash = new dbHtab(!db_def_case_sens,
                db_numGates + db_def_total);
            for (u_int i = 0; i < db_numGates; i++) {
                dbGate *g = db_nlGates[i];
                if (g && g->gatename)
                    db_gate_hash->add(g->gatename, i);
            }
        }
    }
    else {
        db_gate_hash->incsize(db_def_total);
    }
    return (ret);
}


bool
cLDDB::defComponentsSet(defiComponent *cmp)
{
    db_def_processed++;

    const char *usename = cmp->id();
    // Find the corresponding macro.
    lefMacro *gateginfo = getLefGate(cmp->name());
    dbGate *gate;
    if (!gateginfo) {
        emitError(
            "defRead: Error, could not find a macro definition for \"%s\".\n",
            cmp->name());
        gate = 0;
        return (LD_BAD);
    }
    else {
        gate = new dbGate(lddb::copy(usename), gateginfo);
#ifdef DEBUG
        printf("new gate %s\n", usename);
#endif
    }
    if (cmp->isPlaced())
        gate->placed = LD_PLACED;
    else if (cmp->isUnplaced())
        gate->placed = LD_UNPLACED;
    else if (cmp->isFixed())
        gate->placed = LD_FIXED;
    else if ( cmp->isCover())
        gate->placed = LD_COVER;
    if (gate->placed) {
        gate->placedX = defToLefGrid(cmp->placementX());
        gate->placedY = defToLefGrid(cmp->placementY());
        gate->orient  = cmp->placementOrient();
        switch (gate->orient) {
        case ORIENT_EAST:
        case ORIENT_WEST:
        case ORIENT_FLIPPED_EAST:
        case ORIENT_FLIPPED_WEST:
            emitError(
                "defRead: Warning, %s, cannot handle 90-degree rotated "
                "components!\n", gate->gatename);
        default:
            break;
        }
#ifdef DEBUG
        printf("  x= %g  y= %g o= %d r=%d\n", lefToMic(gate->placedX),
            lefToMic(gate->placedY), gate->orient, db_def_resol);
#endif
    }
    // Process the gate.
#ifdef DEBUG
    printf("  w= %g h= %g n= %d\n", lefToMic(gate->width),
        lefToMic(gate->height), gate->nodes);
#endif

    int i = 0;
    for (lefPin *pin = gateginfo->pins; pin; pin = pin->next, i++) {
        // Let the node names point to the master cell;
        // this is just diagnostic;  allows us, for
        // instance, to identify vdd and gnd nodes, so
        // we don't complain about them being
        // disconnected.

        gate->node[i] = pin->name;  // copy pointer
        gate->taps[i] = 0;

        gate->netnum[i] = 0;        // Until we read NETS.
        gate->noderec[i] = 0;

        // Make a copy of the gate nodes and adjust for
        // instance position.

        for (dbDseg *drect = pin->geom; drect; drect = drect->next) {
            dbDseg *newrect = new dbDseg(*drect);
            newrect->next = gate->taps[i];
            gate->taps[i] = newrect;
        }

        for (dbDseg *drect = gate->taps[i]; drect; drect = drect->next) {
            // Handle offset from gate origin.
            drect->x1 -= gateginfo->placedX;
            drect->x2 -= gateginfo->placedX;
            drect->y1 -= gateginfo->placedY;
            drect->y2 -= gateginfo->placedY;

            // Handle rotations and orientations here.
            switch (gate->orient) {
            case ORIENT_NORTH:
                drect->x1 += gate->placedX;
                drect->x2 += gate->placedX;
                drect->y1 += gate->placedY;
                drect->y2 += gate->placedY;
                break;
            case ORIENT_WEST:
                // Not handled.
                break;
            case ORIENT_SOUTH:
                {
                    lefu_t tmp = drect->x1;
                    drect->x1 = -drect->x2;
                    drect->x1 += gate->placedX + gateginfo->width;
                    drect->x2 = -tmp;
                    drect->x2 += gate->placedX + gateginfo->width;
                }
                {
                    lefu_t tmp = drect->y1;
                    drect->y1 = -drect->y2;
                    drect->y1 += gate->placedY + gateginfo->height;
                    drect->y2 = -tmp;
                    drect->y2 += gate->placedY + gateginfo->height;
                }
                break;
            case ORIENT_EAST:
                // Not handled.
                break;
            case ORIENT_FLIPPED_NORTH:
                {
                    lefu_t tmp = drect->x1;
                    drect->x1 = -drect->x2;
                    drect->x1 += gate->placedX + gateginfo->width;
                    drect->x2 = -tmp;
                    drect->x2 += gate->placedX + gateginfo->width;
                }
                drect->y1 += gate->placedY;
                drect->y2 += gate->placedY;
                break;
            case ORIENT_FLIPPED_WEST:
                // Not handled.
                break;
            case ORIENT_FLIPPED_SOUTH:
                drect->x1 += gate->placedX;
                drect->x2 += gate->placedX;
                {
                    lefu_t tmp = drect->y1;
                    drect->y1 = -drect->y2;
                    drect->y1 += gate->placedY + gateginfo->height;
                    drect->y2 = -tmp;
                    drect->y2 += gate->placedY + gateginfo->height;
                }
                break;
            case ORIENT_FLIPPED_EAST:
                // Not handled.
                break;
            }
        }
    }
#ifdef DEBUG
    for (int j = 0; j < gate->nodes; j++) {
        for (dbDseg *drect = gate->taps[j]; drect; drect = drect->next) {
            printf("    %g %g %g %g %d\n",
                lefToMic(drect->x1), lefToMic(drect->y1),
                lefToMic(drect->x2), lefToMic(drect->y2), drect->layer);
        }
    }
#endif

    // Make a copy of the gate obstructions and adjust for
    // instance position.

    for (dbDseg *drect = gateginfo->obs; drect; drect = drect->next) {
        dbDseg *newrect = new dbDseg(*drect);
        newrect->next = gate->obs;
        gate->obs = newrect;
    }

    for (dbDseg *drect = gate->obs; drect; drect = drect->next) {
        drect->x1 -= gateginfo->placedX;
        drect->x2 -= gateginfo->placedX;
        drect->y1 -= gateginfo->placedY;
        drect->y2 -= gateginfo->placedY;

        // Handle rotations and orientations here.
        switch (gate->orient) {
        case ORIENT_NORTH:
            drect->x1 += gate->placedX;
            drect->x2 += gate->placedX;
            drect->y1 += gate->placedY;
            drect->y2 += gate->placedY;
            break;
        case ORIENT_WEST:
            // Not handled.
            break;
        case ORIENT_SOUTH:
            {
                lefu_t tmp = drect->x1;
                drect->x1 = -drect->x2;
                drect->x1 += gate->placedX + gateginfo->width;
                drect->x2 = -tmp;
                drect->x2 += gate->placedX + gateginfo->width;
            }
            {
                lefu_t tmp = drect->y1;
                drect->y1 = -drect->y2;
                drect->y1 += gate->placedY + gateginfo->height;
                drect->y2 = -tmp;
                drect->y2 += gate->placedY + gateginfo->height;
            }
            break;
        case ORIENT_EAST:
            // Not handled.
            break;
        case ORIENT_FLIPPED_NORTH:
            {
                lefu_t tmp = drect->x1;
                drect->x1 = -drect->x2;
                drect->x1 += gate->placedX + gateginfo->width;
                drect->x2 = -tmp;
                drect->x2 += gate->placedX + gateginfo->width;
            }
            drect->y1 += gate->placedY;
            drect->y2 += gate->placedY;
            break;
        case ORIENT_FLIPPED_WEST:
            // Not handled.
            break;
        case ORIENT_FLIPPED_SOUTH:
            drect->x1 += gate->placedX;
            drect->x2 += gate->placedX;
            {
                lefu_t tmp = drect->y1;
                drect->y1 = -drect->y2;
                drect->y1 += gate->placedY + gateginfo->height;
                drect->y2 = -tmp;
                drect->y2 += gate->placedY + gateginfo->height;
            }
            break;
        case ORIENT_FLIPPED_EAST:
            // Not handled.
            break;
        }
    }
#ifdef DEBUG
    printf("    obs\n");
    for (dbDseg *drect = gate->obs; drect; drect = drect->next) {
        printf("    %g %g %g %g %d\n",
            lefToMic(drect->x1), lefToMic(drect->y1),
            lefToMic(drect->x2), lefToMic(drect->y2), drect->layer);
    }
#endif
    db_nlGates[db_numGates] = gate;
    db_numGates++;

    return (LD_OK);
}


bool
cLDDB::defComponentsEnd()
{
    // For Qrouter emulation, reverse list order.
    if (debug() & LD_DBG_ORDR) {
        u_int n2 = db_numGates/2;
        for (u_int i = 0; i < n2; i++) {
            dbGate *g = db_nlGates[i];
            db_nlGates[i] = db_nlGates[db_numGates - i - 1];
            db_nlGates[db_numGates - i - 1] = g;
        }
    }

    // Hash the array index of the gates.
    if (db_gate_hash) {
        for (u_int i = db_numGates - db_def_processed; i < db_numGates; i++) {
            dbGate *g = db_nlGates[i];
            db_gate_hash->add(g->gatename, i);
        }
    }

    if (db_def_processed == db_def_total) {
        if (db_verbose > 0) {
            emitMesg("  Processed %d subcell instances total.\n",
                db_def_processed);
        }
    }
    else {
        emitError(
            "defRead: Warning, number of subcells read (%d) does not match "
            "the number declared (%d).\n", db_def_processed, db_def_total);
    }
    return (LD_OK);
}


bool
cLDDB::defBlockagesBegin(int num)
{
    bool ret = defFinishTracks();
    db_def_total = num;
    db_def_processed = 0;
    return (ret);
}


bool
cLDDB::defBlockagesSet(defiBlockage *blk)
{
    db_def_processed++;

    // Update the record of the number of components processed and
    // spit out a message for every 5% done.

    if (blk->hasLayer()) {
        lefRouteLayer *lr = getLefRouteLayer(blk->layerName());
        if (lr) {
            dbDseg *dend = db_userObs;

            for (int i = 0; i < blk->numRectangles(); i++) {
                dbDseg *dnew = new dbDseg(
                    defToLefGrid(blk->xl(i)), defToLefGrid(blk->yl(i)),
                    defToLefGrid(blk->xh(i)), defToLefGrid(blk->yh(i)),
                    lr->layer, lr->lefId, 0);
                if (!dend)
                    dend = db_userObs = dnew;
                else {
                    while (dend->next)
                        dend = dend->next;
                    dend->next = dnew;
                }
            }
            for (int i = 0; i < blk->numPolygons(); i++) {
                defiPoints points = blk->getPolygon(i);
                dbDpoint *plist = 0;
                for (int j = 0; j < points.numPoints; j++) {
                    plist = new dbDpoint(
                        defToLefGrid(points.x[j]), defToLefGrid(points.y[j]),
                        lr->layer, lr->lefId, plist);
                }
                dbDseg *rlist;
                polygonToRects(&rlist, plist);
                if (!dend)
                    dend = db_userObs = rlist;
                else {
                    while (dend->next)
                        dend = dend->next;
                    dend->next = rlist;
                }
            }
        }
    }

    return (LD_OK);
}


bool
cLDDB::defBlockagesEnd()
{
    if (db_def_processed == db_def_total) {
        if (db_verbose > 0)
            emitMesg("  Processed %d blockages total.\n", db_def_processed);
    }
    else {
        emitError(
            "defRead: Warning, number of blockages read (%d) does not match "
            "the number declared (%d).\n", db_def_processed, db_def_total);
    }
    return (LD_OK);
}


bool
cLDDB::defViasBegin(int num)
{
    bool ret = defFinishTracks();
    db_def_total = num;
    db_def_processed = 0;
    return (ret);
}


bool
cLDDB::defViasSet(defiVia *via)
{
    db_def_processed++;
    lefObject *lefo = getLefObject(via->name());
    if (lefo) {
        emitError("defRead: Error, composite via \"%s\" redefined.\n",
            via->name());
        return (LD_BAD);
    }
    lefViaObject *lefv = new lefViaObject(lddb::copy(via->name()));
    lefAddObject(lefv);

    if (via->defiVia::numLayers() == 1 || via->defiVia::numLayers() == 3) {
        dbDsegB dsegs[3];
        for (int i = 0; i < via->defiVia::numLayers(); i++) {
            char *lname;
            int xl, yl, xh, yh;
            via->defiVia::layer(i, &lname, &xl, &yl, &xh, &yh);
            lefObject *lo = getLefObject(lname);
            if (!lo) {
                emitError(
                    "defRead: Error, composite via \"%s\" unknown "
                    "layer \"%s\".\n", via->defiVia::name(), lname);
                return (LD_BAD);
            }
            // Rectangles for vias are read in units of 1/2 lambda.
            dsegs[i] = dbDsegB(
                defToLefGrid(2*xl), defToLefGrid(2*yl),
                defToLefGrid(2*xh), defToLefGrid(2*yh),
                lo->layer, lo->lefId);
        }
        if (via->defiVia::numLayers() == 1)
            lefv->via.cut = dsegs[0];
        else {
            int cnt = 0;
            for (int i = 0; i < 3; i++) {
                if (dsegs[i].layer < 0)
                    cnt++;
            }
            if (cnt != 1) {
                emitError(
                    "defRead: Error, composite via \"%s\" not one cut, two "
                    "metal.\n", via->defiVia::name());
                return (LD_BAD);
            }
            for (int i = 0; i < 3; i++) {
                if (dsegs[i].layer < 0) {
                    lefv->via.cut = dsegs[i];
                    break;
                }
            }
            for (int i = 0; i < 3; i++) {
                if (dsegs[i].layer >= 0) {
                    for (int j = i+1; j < 3; j++) {
                        if (dsegs[j].layer >= 0) {
                            if (dsegs[j].layer > dsegs[i].layer) {
                                lefv->via.bot = dsegs[i];
                                lefv->via.top = dsegs[j];
                            }
                            else if (dsegs[j].layer < dsegs[i].layer) {
                                lefv->via.bot = dsegs[j];
                                lefv->via.top = dsegs[i];
                            }
                            else {
                                emitError(
                                    "defRead: Error, composite via \"%s\" "
                                    "duplicate layer.\n", via->defiVia::name());
                                return (LD_BAD);
                            }
                        }
                    }
                }
            }
        }
    }
    else if (via->numLayers() > 0) {
        emitError(
            "defRead: Error, composite via \"%s\" layer count not 1 or 3.\n",
            via->defiVia::name());
        return (LD_BAD);
    }
    return (LD_OK);
}


bool
cLDDB::defViasEnd()
{
    if (db_def_processed == db_def_total) {
        if (db_verbose > 0)
            emitMesg("  Processed %d vias total.\n", db_def_processed);
    }
    else {
        emitError(
            "defRead: Warning, number of vias read (%d) does not match "
            "the number declared (%d).\n", db_def_processed, db_def_total);
    }
    return (LD_OK);
}


bool
cLDDB::defPinsBegin(int num)
{
    bool ret = defFinishTracks();
    db_def_total = num;
    db_def_processed = 0;

    if (db_numPins == 0) {
        db_nlPins = new dbGate*[db_def_total];
        for (u_int i = 0; i < db_def_total; i++)
            db_nlPins[i] = 0;
    }
    else {
        // Support incremental...
        dbGate **opins = db_nlPins;
        db_nlPins = new dbGate*[db_numPins + db_def_total];
        for (u_int i = 0; i < db_numPins; i++)
            db_nlPins[i] = opins[i];
        delete [] opins;
        for (u_int i = db_numPins; i < (db_numPins + db_def_total); i++)
            db_nlPins[i] = 0;
    }

    // Create or expand hash table for pin names.
    if (!db_pin_hash) {
        if (db_numPins + db_def_total > 16) {
            db_pin_hash = new dbHtab(!db_def_case_sens,
                db_numPins + db_def_total);
            for (u_int i = 0; i < db_numPins; i++) {
                dbGate *g = db_nlPins[i];
                if (g && g->gatename)
                    db_pin_hash->add(g->gatename, i);
            }
        }
    }
    else {
        db_pin_hash->incsize(db_def_total);
    }
    return (ret);
}


bool
cLDDB::defPinsSet(defiPin *pin)
{
    db_def_processed++;

    // Create the pin record.
    dbGate *gate = new dbGate(lddb::copy(pin->netName()), db_pinMacro);
    int routelayer = -1;
    int rt_id = -1;

    gate->node[0] = lddb::copy(pin->netName());

    if (pin->hasLayer()) {
        // Here we set the gate width/height to the maximum values if
        // there are multiple layers.  Original code would define
        // width/height for each layer, overwriting any previous.
        // to-do: Keep geometry for multiple layers?

        for (int i = 0; i < pin->numLayer(); i++) {
            lefObject *lo = getLefObject(pin->layer(i));
            if (lo) {
                if (routelayer < 0) {
                    int rl = lo->layer;
                    if (rl >= 0) {
                        routelayer = rl;
                        rt_id = lo->lefId;
                    }
                }
                int xl, yl, xh, yh;
                pin->bounds(i, &xl, &yl, &xh, &yh);
                lefu_t wid = defToLefGrid(xh - xl);
                lefu_t hei = defToLefGrid(yh - yl);
                if (wid > gate->width)
                    gate->width = wid;
                if (hei > gate->height)
                    gate->height = hei;
#ifdef DEBUG
                printf("new pin %s %g %g\n", gate->gatename,
                    lefToMic(gate->width), lefToMic(gate->height));
#endif
            }
            else {
                emitErrMesg("defRead: Warning, unknown layer %s.\n",
                    pin->layer(i));
            }
        }
    }

    if (pin->isPlaced())
        gate->placed = LD_PLACED;
    else if (pin->isFixed())
        gate->placed = LD_FIXED;
    else if (pin->isCover())
        gate->placed = LD_COVER;
    if (gate->placed) {
        gate->placedX = defToLefGrid(pin->placementX());
        gate->placedY = defToLefGrid(pin->placementY());
        gate->orient  = pin->orient();
        switch (gate->orient) {
        case ORIENT_EAST:
        case ORIENT_WEST:
        case ORIENT_FLIPPED_EAST:
        case ORIENT_FLIPPED_WEST:
            emitError(
                "defRead: Warning, %s, cannot handle 90-degree rotated "
                "components!\n", gate->gatename);
        default:
            break;
        }
#ifdef DEBUG
        printf("  %g %g %d\n",
            lefToMic(gate->placedX), lefToMic(gate->placedY),
            gate->orient);
#endif
    }
    if (routelayer >= 0 && routelayer < (int)db_numLayers) {

        // If no NET was declared for pin, use pinname.
        if (!gate->gatename)
            gate->gatename = lddb::copy(pin->pinName());

        // Make sure pin is at least the size of the route layer.

        lefu_t hwidth = getRouteWidth(routelayer);
        if (gate->width < hwidth)
            gate->width = hwidth;
        if (gate->height < hwidth)
            gate->height = hwidth;
        hwidth /= 2;

        gate->taps[0] = new dbDseg(
            gate->placedX - hwidth, gate->placedY - hwidth,
            gate->placedX + hwidth, gate->placedY + hwidth,
            routelayer, rt_id, 0);

        gate->obs = 0;
        gate->nodes = 1;

        db_nlPins[db_numPins] = gate;
        db_numPins++;
#ifdef DEBUG
        printf("  %g %g %g %g %d\n",
            lefToMic(gate->taps[0]->x1), lefToMic(gate->taps[0]->y1),
            lefToMic(gate->taps[0]->x2), lefToMic(gate->taps[0]->y2),
            gate->taps[0]->layer);
#endif
    }
    else {
        emitError(
        "readDef: Warning, pin %s is defined outside of route layer area!\n",
            pin->pinName());
        delete gate;
    }

    return (LD_OK);
}


bool
cLDDB::defPinsEnd()
{
    // For Qrouter emulation, reverse list order.

    if (debug() & LD_DBG_ORDR) {
        u_int n2 = db_numPins/2;
        for (u_int i = 0; i < n2; i++) {
            dbGate *g = db_nlPins[i];
            db_nlPins[i] = db_nlPins[db_numPins - i - 1];
            db_nlPins[db_numPins - i - 1] = g;
        }
    }

    // Hash the array index of the pins.
    if (db_pin_hash) {
        for (u_int i = db_numPins - db_def_processed; i < db_numPins; i++) {
            dbGate *g = db_nlPins[i];
            db_pin_hash->add(g->gatename, i);
        }
    }

    if (db_def_processed == db_def_total) {
        if (db_verbose > 0)
            emitMesg("  Processed %d pins total.\n", db_def_processed);
    }
    else {
        emitError(
            "readDef: Warning, number of pins read (%d) does not match "
            "the number declared (%d).\n", db_def_processed, db_def_total);
    }
    return (LD_OK);
}


bool
cLDDB::defNetsBegin(int num)
{
    bool ret = defFinishTracks();
    db_def_total = num;
    db_def_processed = 0;
    if (ret == LD_OK) {
        if (db_maxNets > 0 && db_def_total + db_numNets > db_maxNets) {
            emitError(
                "defRead: Error, number of nets in design (%d) exceeds "
                "maximum (%d)\n", db_def_total + db_numNets, db_maxNets);
            return (LD_BAD);
        }
        if (db_numNets == 0) {
            // Initialize net and node records.
            db_def_netidx = LD_MIN_NETNUM;
            db_nlNets = new dbNet*[db_def_total];
            for (u_int i = 0; i < db_def_total; i++)
                db_nlNets[i] = 0;
        }
        else {
            // Support incremental...
            dbNet **onets = db_nlNets;
            db_nlNets = new dbNet*[db_numNets + db_def_total];
            for (u_int i = 0; i < db_numNets; i++)
                db_nlNets[i] = onets[i];
            delete [] onets;
            for (u_int i = db_numNets; i < (db_numNets + db_def_total); i++)
                db_nlNets[i] = 0;
        }

        // Create or expand hash table for net names.
        if (!db_net_hash) {
            if (db_numNets + db_def_total > 16) {
                db_net_hash = new dbHtab(!db_def_case_sens,
                    db_numNets + db_def_total);
                for (u_int i = 0; i < db_numNets; i++) {
                    dbNet *net = db_nlNets[i];
                    if (net && net->netname)
                        db_net_hash->add(net->netname, i);
                }
            }
        }
        else {
            db_net_hash->incsize(db_def_total);
        }
    }
    return (ret);
}


bool
cLDDB::defNetsSet(defiNet *net)
{
    defReadNet(net, false);
    return (LD_OK);
}


bool
cLDDB::defNetsEnd()
{
    // Set the number of nodes per net for each node on the net.

    // Fill in the netnodes list for each net, needed for checking
    // for isolated routed groups within a net.

    for (u_int i = 0; i < db_numNets; i++) {
        dbNet *net = db_nlNets[i];
        for (dbNode *node = net->netnodes; node; node = node->next)
            net->numnodes++;
        for (dbNode *node = net->netnodes; node; node = node->next)
            node->numnodes = net->numnodes;
    }

    // Hash the array index of the nets.
    if (db_net_hash) {
        for (u_int i = db_numNets - db_def_processed; i < db_numNets; i++) {
            dbNet *net = db_nlNets[i];
            db_net_hash->add(net->netname, i);
        }
    }

    if (db_def_processed == db_def_total) {
        if (db_verbose > 0)
            emitMesg("  Processed %d nets total.\n", db_def_processed);
    }
    else {
        emitError(
            "defRead: Warning, number of nets read (%d) does not match "
            "the number declared (%d).\n", db_def_processed, db_def_total);
    }

    // Look through all gate instances for unconnected global pins. 
    // Give these a dummy node.
    //
    for (u_int j = 0; j < db_numGates; j++) {
        dbGate *gate = db_nlGates[j];
        int i = 0;
        for (lefPin *pin = gate->gatetype->pins; pin; pin = pin->next, i++) {
            if (gate->netnum[i] != 0)
                continue;
            // Pin is not connected.

            for (u_int k = 0; k < db_numGlobals; k++) {
                const char *s = db_global_names[k];
                if (s && !defStrcmp(gate->node[i], s)) {
                    gate->netnum[i] = db_global_nnums[k];
                    gate->noderec[i] = new dbNode;
                    gate->noderec[i]->netnum = db_global_nnums[k];
                }
            }
        }
    }

    // Same for pins, not sure this is needed.
    for (u_int j = 0; j < db_numPins; j++) {
        dbGate *gate = db_nlPins[j];
        int i = 0;
        for (lefPin *pin = gate->gatetype->pins; pin; pin = pin->next, i++) {
            if (gate->netnum[i] != 0)
                continue;
            // Pin is not connected.

            for (u_int k = 0; k < db_numGlobals; k++) {
                const char *s = db_global_names[k];
                if (s && !defStrcmp(gate->node[i], s)) {
                    gate->netnum[i] = db_global_nnums[k];
                    gate->noderec[i] = new dbNode;
                    gate->noderec[i]->netnum = db_global_nnums[k];
                }
            }
        }
    }

    return (LD_OK);
}


bool
cLDDB::defSpecialNetsBegin(int num)
{
    bool ret = defFinishTracks();
    db_def_total = num;
    db_def_processed = 0;
    return (ret);
}


bool
cLDDB::defSpecialNetsSet(defiNet *net)
{
    defReadNet(net, true);
    return (LD_OK);
}


bool
cLDDB::defSpecialNetsEnd()
{
    if (db_def_processed == db_def_total) {
        if (db_verbose > 0)
            emitMesg("  Processed %d special nets total.\n", db_def_processed);
    }
    else {
        emitError(
            "defRead: Warning, number of special nets read (%d) does not match "
            "the number declared (%d).\n", db_def_processed, db_def_total);
    }
    return (LD_OK);
}

//
// End of callbacks for Cadence DEF parser.
//


bool
cLDDB::defFinishTracks()
{
    // After the TRACKS have been read in, corient is 'x' or 'y'.
    // On the next keyword, finish filling in track information.

    if (db_def_corient == '.')
        return (LD_OK);

    bool ret = setupChannels(false);
    db_def_corient = '.';  // So we don't run this code again.

#ifdef DEBUG
    printf("Tracks\n");
    for (u_int i = 0; i < db_numLayers; i++) {
        printf("%g %g %d\n", lefToMic(pitchX(i)), lefToMic(pitchY(i)),
            vert(i));
    }
    printf("%g %g %g %g\n", lefToMic(db_xLower), lefToMic(db_yLower),
        lefToMic(db_xUpper), lefToMic(db_yUpper));
#endif
    return (ret);
}


// defReadNet
//
void
cLDDB::defReadNet(defiNet *dnet, bool special)
{
    db_def_processed++;
    dbNet *net;
    if (special) {
        net = getNet(dnet->name());
        if (!net) {
            emitError(
                "defRead: Warning, SPECIALNET %s not found in NETS, ignored.\n",
                dnet->name());
            return;
        }
    }
    else {
        net = new dbNet;
        net->netname = lddb::copy(dnet->name());
        db_nlNets[db_numNets++] = net;

        net->netnum = db_def_netidx++;
        for (u_int i = 0; i < db_numGlobals; i++) {
            const char *s = db_global_names[i];
            if (s && !defStrcmp(dnet->name(), s)) {
                net->flags |= NET_GLOBAL;
                db_global_nnums[i] = net->netnum;
            }
        }

        dbNode *last = 0;
        for (int i = 0; i < dnet->numConnections(); i++) {
            const char *instname = dnet->instance(i);
            const char *pinname = dnet->pin(i);

            if (!strcasecmp(instname, "pin")) {
                instname = pinname;
                pinname = "pin";
            }
            dbNode *node = new dbNode;
            node->nodenum = i;
            defReadGatePin(net, node, instname, pinname);
            if (!node->pinindx)
                delete node;
            else {
                if (debug() & LD_DBG_ORDR) {
                    // Order matches Qrouter, backwards in numbering.
                    node->next = net->netnodes;
                    net->netnodes = node;
                }
                else {
                    if (last) {
                        last->next = node;
                        last = node;
                    }
                    else
                        last = net->netnodes = node;
                }
            }
        }
    }

    for (int i = 0; i < dnet->numWires(); i++) {
        defiWire *wire = dnet->wire(i);
        if (!strcmp(wire->wireType(), "ROUTED")) {
            // Read in the route; qrouter now takes responsibility for
            // this route.

            defAddRoutes(wire, net, special);
        }
        else if (!strcmp(wire->wireType(), "FIXED") ||
                !strcmp(wire->wireType(), "COVER")) {
            // Treat fixed nets like specialnets.
            
            defAddRoutes(wire, net, true);
        }
    }
}


//#define DEBUG_TP

// defReadGatePin
//
// Given a gate name and a pin name in a net from the DEF file NETS
// section, find the position of the gate, then the position of the
// pin within the gate, and add pin and obstruction information to the
// grid network.
//
void
cLDDB::defReadGatePin(dbNet *net, dbNode *node, const char *instname,
    const char *pinname)
{
    int gorpnum = getGateNum(instname);
    if (gorpnum < 0)
        gorpnum = getPinNum(instname);
    dbGate *g = getGateOrPinByNum(gorpnum);
    if (!g) {
        emitError("defRead: Warning, instance %s for net %s not found.\n",
            instname, net->netname);
        return;
    }
    node->gorpnum = gorpnum;

    lefMacro *gateginfo = g->gatetype;

    if (!gateginfo) {
        emitError("defRead: Warning, endpoint %s/%s of net %s not found.\n",
            instname, pinname, net->netname);
        return;
    }

#ifdef DEBUG_TP
    emitMesg("Net %s\n", net->netname);
#endif
    int i = 0;
    for (lefPin *pin = gateginfo->pins; pin; pin = pin->next, i++) {
        if (!defStrcmp(pin->name, pinname)) {
            node->pinindx = i+1;
            node->taps = 0;
            node->extend = 0;

            for (dbDseg *drect = g->taps[i]; drect; drect = drect->next) {
#ifdef DEBUG_TP
                emitMesg("  tap %d %g %g %g %g\n", drect->layer,
                    lefToMic(drect->x1), lefToMic(drect->y1),
                    lefToMic(drect->x2), lefToMic(drect->y2));
#endif

                // Add all routing gridpoints that fall inside
                // the rectangle.  Much to do here:
                // (1) routable area should extend 1/2 route width
                // to each side, as spacing to obstructions allows.
                // (2) terminals that are wide enough to route to
                // but not centered on gridpoints should be marked
                // in some way, and handled appropriately.

                int gridx = (drect->x1 - db_xLower) / pitchX(drect->layer) - 1;
                if (gridx < 0)
                    gridx = 0;
                for (;;) {
                    lefu_t dx = (gridx * pitchX(drect->layer)) + db_xLower;
                    if (dx > drect->x2 + db_layers[drect->layer].haloX)
                        break;
                    if (dx < drect->x1 - db_layers[drect->layer].haloX) {
                        gridx++;
                        continue;
                    }
                    int gridy = ((drect->y1 - db_yLower) /
                        pitchY(drect->layer)) - 1;
                    if (gridy < 0)
                        gridy = 0;
                    for (;;) {
                        lefu_t dy = (gridy * pitchY(drect->layer)) + db_yLower;
                        if (dy > drect->y2 + db_layers[drect->layer].haloY)
                            break;
                        if (dy < drect->y1 - db_layers[drect->layer].haloY){
                            gridy++;
                            continue;
                        }

                        // Routing grid point is an interior point
                        // of a gate port.  Record the position.

                        dbDpoint *dp = new dbDpoint(dx, dy, drect->layer,
                            drect->lefId, 0);
                        dp->gridx = gridx;
                        dp->gridy = gridy;

                        if (dy >= drect->y1 && dx >= drect->x1 &&
                                dy <= drect->y2 && dx <= drect->x2) {
                            dp->next = node->taps;
                            node->taps = dp;
#ifdef DEBUG_TP
                            emitMesg("    t %g %g %d %d\n",
                                lefToMic(dx), lefToMic(dy), gridx, gridy);
#endif
                        }
                        else {
                            dp->next = node->extend;
                            node->extend = dp;
#ifdef DEBUG_TP
                            emitMesg("    x %g %g %d %d\n",
                                lefToMic(dx), lefToMic(dy), gridx, gridy);
#endif
                        }
                        gridy++;
                    }
                    gridx++;
                }
            }
            node->netnum = net->netnum;
            g->netnum[i] = net->netnum;
            g->noderec[i] = node;

            return;
        }
    }
    emitError("defRead: Warning, pin %s/%s of net %s not found.\n",
        instname, pinname, net->netname);
}


// defAddRoutes
//
// Parse a network route statement from the DEF file.  If "special" is
// true, add the geometry to the list of internal obstructions.  These
// are used when routing, but not written to a DEF file.  Read the
// geometry into a route structure for the net. 
//
void
cLDDB::defAddRoutes(defiWire *wire, dbNet *net, bool special)
{
    dbRoute *routednet = 0;
    bool valid = false;         // Is there a valid reference point?
    int routeLayer = -1;
    int paintLayer = -1;
    dbSeg grdpt;
    int refx1 = 0;
    int refy1 = 0;
    dbSeg *endseg = 0;
    dbPath *endpath = 0;

    // There may already be routes present, since we allow
    // incremental, and regular/special routes are added separately.

    dbRoute *endroute = net->routes;
    if (endroute) {
        while (endroute->next)
            endroute = endroute->next;
    }

    if (special) {
        net->spath->free();
        net->spath = 0;
    }
    else {
        net->path->free();
        net->path = 0;
    }

    for (int i = 0; i < wire->numPaths(); i++) {
        defiPath *p = wire->path(i);
        p->initTraverse();
        lefu_t x = 0;
        lefu_t y = 0;
        lefu_t w = 0;
        int path;
        while ((path = (int)p->next()) != DEFIPATH_DONE) {
            if (path == DEFIPATH_LAYER) {
                // Initial or following NEW.

                // Invalidate reference point.
                valid = false;
                routednet = 0;
                routeLayer = -1;
                paintLayer = -1;

                lefRouteLayer *rl = getLefRouteLayer(p->getLayer());
                if (!rl) {
                    emitError(
                        "defRead: Warning, unknown layer type \"%s\" for "
                        "NEW route.\n", p->getLayer()); 
                    continue;
                }
                routeLayer = rl->layer;
                paintLayer = routeLayer;
                w = getRouteWidth(paintLayer); 

                // Create a new route record, add to the 1st node.

                routednet = new dbRoute(0, net->netnum,
                    special ? RT_STUB : 0, 0);
                if (endroute)
                    endroute->next = routednet;
                else
                    net->routes = routednet;
                endroute = routednet;
                endseg = 0;

                dbPath *npath = 0;
                if (special) {
                    if (!net->spath)
                        net->spath = npath = new dbPath(0, 0);
                }
                else {
                    if (!net->path)
                        net->path = npath = new dbPath(0, 0);
                }
                if (npath)
                    endpath = npath;
                else {
                    endpath->next = new dbPath(0, 0);
                    endpath = endpath->next;
                }
                endpath->layer = routeLayer;
            }
            else if (path == DEFIPATH_WIDTH) {
                // SPECIALNETS has the additional width.
                if (special && routednet) {
                    int ww = p->getWidth();
                    if (ww != 0)
                        w = defToLefGrid(ww);
                    endpath->width = w;
                }
            }
            else if (path == DEFIPATH_VIA) {
                if (!routednet)
                    continue;

                if (!valid) {
                    emitError(
                        "defRead: Warning, route has via name \"%s\" but "
                        "no points!\n", p->getVia());
                    continue;
                }
                lefObject *lefo = getLefObject(p->getVia());
                if (lefo) {
                    // The area to paint is derived from the via definitions.

                    if (lefo->lefClass == CLASS_VIA) {
                        lefViaObject *lefv = (lefViaObject*)lefo;
                        paintLayer = db_numLayers - 1;
                        routeLayer = -1;

                        routeLayer = lefv->via.bot.layer;
                        if (routeLayer < paintLayer)
                            paintLayer = routeLayer;
                        if ((routeLayer >= 0) && special && valid) {
                            db_intObs = new dbDseg(
                                x + lefv->via.bot.x1, y + lefv->via.bot.y1,
                                x + lefv->via.bot.x2, y + lefv->via.bot.y2,
                                lefv->via.bot.layer, lefv->via.bot.lefId,
                                db_intObs);
                        }
                        routeLayer = lefv->via.top.layer;
                        if (routeLayer < paintLayer)
                            paintLayer = routeLayer;
                        if ((routeLayer >= 0) && special && valid) {
                            db_intObs = new dbDseg(
                                x + lefv->via.top.x1, y + lefv->via.top.y1,
                                x + lefv->via.top.x2, y + lefv->via.top.y2,
                                lefv->via.bot.layer, lefv->via.bot.lefId,
                                db_intObs);
                        }
                        if (routeLayer == -1)
                            paintLayer = lefo->layer;
                    }
                    else
                        paintLayer = lefo->layer;

                    if (paintLayer >= 0) {

                        dbSeg *newSeg = new dbSeg(paintLayer, -1, ST_VIA,
                            refx1, refy1, refx1, refy1, 0);
                        if (endseg)
                            endseg->next = newSeg;
                        else
                            routednet->segments = newSeg;
                        endseg = newSeg;

                        endpath->vid = lefo->lefId;
                        endpath->x = x;
                        endpath->y = y;
                    }
                    else {
                        emitError(
                            "defRead: Warning, via \"%s\" does not define a "
                            "metal layer!\n", p->getVia());
                    }
                }
                else {
                    emitError(
                        "defRead: Warning, via name \"%s\" unknown in route.\n",
                        p->getVia());
                }
            }
            else if (path == DEFIPATH_POINT) {
                if (!routednet)
                    continue;

                // Revert to the routing layer type, in case we painted a via.
                paintLayer = routeLayer;

                // Record current reference point.
                grdpt.x1 = refx1;
                grdpt.y1 = refy1;
                lefu_t lx = x;
                lefu_t ly = y;

                int xx, yy;
                p->getPoint(&xx, &yy);

                x = defToLefGrid(xx);
                refx1 = (x - db_xLower) / pitchX(paintLayer);
                y = defToLefGrid(yy);
                refy1 = (y - db_yLower) / pitchY(paintLayer);

                // Indicate that we have a valid reference point.

                if (!valid) {
                    if (endpath) {
                        endpath->x = x;
                        endpath->y = y;
                    }
                    valid = true;
                }
                else if (lx != x && ly != y) {
                    // Skip over nonmanhattan segments, reset the
                    // reference point, and output a warning.

                    emitError(
                        "defRead: Warning, can't handle non-Manhattan "
                        "geometry in route.\n");
                    grdpt.x1 = refx1;
                    grdpt.y1 = refy1;
                    lx = x;
                    ly = y;
                }
                else {
                    grdpt.x2 = refx1;
                    grdpt.y2 = refy1;

                    if (special) {
                        if (valid) {
                            lefRouteLayer *rl = getLefRouteLayer(routeLayer);
                            int id = rl ? rl->lefId : -1;
                            dbDseg *drect = new dbDseg(0, 0, 0, 0,
                                routeLayer, id, db_intObs);
                            if (lx > x) {
                                drect->x1 = x - w;
                                drect->x2 = lx + w;
                            }
                            else {
                                drect->x1 = x + w;
                                drect->x2 = lx - w;
                            }
                            if (ly > y) {
                                drect->y1 = y - w;
                                drect->y2 = ly + w;
                            }
                            else {
                                drect->y1 = y + w;
                                drect->y2 = ly - w;
                            }
                            db_intObs = drect;
                        }
                    }
                    if (paintLayer >= 0) {

                        dbSeg *newSeg = new dbSeg(paintLayer, -1, ST_WIRE,
                            grdpt.x1, grdpt.y1, grdpt.x2, grdpt.y2, 0);
                        if (endseg)
                            endseg->next = newSeg;
                        else
                            routednet->segments = newSeg;
                        endseg = newSeg;

                        endpath->next = new dbPath(x, y);
                        endpath = endpath->next;
                    }
                    lx = x;
                    ly = y;
                }
            }
        }
    }
}

