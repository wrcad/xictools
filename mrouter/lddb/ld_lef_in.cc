
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

#include "lddb_prv.h"
#include "ld_hash.h"
#include "lefrReader.hpp"
#include "lefiDebug.hpp"
#include "lefiEncryptInt.hpp"
#include "lefiUtil.hpp"
#include "miscutil/tvals.h"
#include <errno.h>
#include <math.h>


//
// LEF/DEF Database.
//
// LEF reader, using the Cadence LEF parser.  This includes creation
// of cells from macro statements, handling of pins, ports,
// obstructions, and associated geometry.
//


//#define DEBUG

namespace {
    void lineNumberCB(int n)
    {
        cLDDB *db = (cLDDB*)lefrGetUserData();
        db->setCurrentLine(n);
    }
  
    void errorCB(const char* msg)
    {
        cLDDB *db = (cLDDB*)lefrGetUserData();
        db->emitErrMesg("%s\n", msg);
    }
  
    void warningCB(const char* msg)
    {
        cLDDB *db = (cLDDB*)lefrGetUserData();
        db->emitMesg("%s\n", msg);
    }

    int versionCB(lefrCallbackType_e, double v, void *data)
    {
        cLDDB *db = (cLDDB*)data;

        // For lef file with version 5.5 and earlier, the
        // NAMESCASESENSITIVE statement is a required statement.
        //
        // If it is not specified, the current lef parser will
        // generate a warning, but will continue to parse the lef file
        // and will treat it as NAMESCASESENSITIVE OFF.
        //
        // In version 5.6 and later this statement is an optional
        // statement and the default is ON.

        return (db->lefSetCaseSens(v > 5.55));
    }

    int caseSensCB(lefrCallbackType_e, int v, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->lefSetCaseSens(v));
    }

    int unitsCB(lefrCallbackType_e, lefiUnits *unit, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->lefUnitsSet(unit));
    }

    int manufacturingCB(lefrCallbackType_e, double num, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->lefManufacturingSet(num));
    }

    int viaCB(lefrCallbackType_e, lefiVia *via, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->lefViaSet(via));
    }

    int viaRuleCB(lefrCallbackType_e, lefiViaRule *viar, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->lefViaRuleSet(viar));
    }

    int layerCB(lefrCallbackType_e, lefiLayer *lyr, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->lefLayerSet(lyr));
    }

    int siteCB(lefrCallbackType_e, lefiSite *site, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->lefSiteSet(site));
    }

    int macroBeginCB(lefrCallbackType_e, const char *mname, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->lefMacroBegin(mname));
    }

    int macroCB(lefrCallbackType_e, lefiMacro *macro, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->lefMacroSet(macro));
    }

    int pinCB(lefrCallbackType_e, lefiPin *pin, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->lefPinSet(pin));
    }

    int obstructionCB(lefrCallbackType_e, lefiObstruction *obs, void *data)
    {
        cLDDB *db = (cLDDB*)data;
        return (db->lefObstructionSet(obs));
    }
}


// lefRead
//
// Read a .lef file and generate all routing configuration structures
// and values from the LAYER, VIA, and MACRO sections.  If incr is
// true, incrementally add the file contents to existing data
// structures, otherwise the database will be cleared before the read.
//
bool
cLDDB::lefRead(const char *filename, bool incr)
{
    if (!filename)
        return (LD_BAD);

    long time0 = Tvals::millisec();
    lefrInit();

    lefrSetLineNumberFunction(lineNumberCB);
    lefrSetDeltaNumberLines(1);
    lefrSetLogFunction(errorCB);
    lefrSetWarningLogFunction(warningCB);
    lefrSetCaseSensitiveCbk(caseSensCB);
    lefrSetVersionCbk(versionCB);

    lefrSetUnitsCbk(unitsCB);
    lefrSetManufacturingCbk(manufacturingCB);
    lefrSetLayerCbk(layerCB);
    lefrSetViaCbk(viaCB);
    lefrSetViaRuleCbk(viaRuleCB);
    lefrSetSiteCbk(siteCB);
    lefrSetMacroBeginCbk(macroBeginCB);
    lefrSetMacroCbk(macroCB);
    lefrSetPinCbk(pinCB);
    lefrSetObstructionCbk(obstructionCB);

    lefrReset();
    lefrEnableReadEncrypted();

    // Initialize.
    if (!incr)
        lefReset();
    db_currentLine      = 0;
    db_errors           = 0;

    FILE *f = fopen(filename, "r");
    if (!f) {
        emitErrMesg("lefRead: Error, cannot open LEF data file: %s.\n",
            strerror(errno));
        return (LD_BAD);
    }
    if (db_verbose > 0) {
        emitMesg("Reading LEF data from file %s.\n",
            lstring::strip_path(filename));
        flushMesg();
    }
    int res = lefrRead(f, filename, this);
    if (res)
        emitErrMesg("lefRead: Warning, LEF reader returned bad status.\n");

    lefrUnsetCallbacks();

    // Release allocated singleton data.
    lefrClear();    

    if (f)
        fclose(f);

    if (db_verbose > 0) {
        long elapsed = Tvals::millisec() - time0;
        emitMesg("LEF read: Processed %d lines in %ld milliseconds.\n",
            db_currentLine, elapsed);
    }
    emitError(0); // Print statement of errors, if any.

    flushErrMesg();
    flushMesg();
    return (res);
}


void
cLDDB::lefReset()
{
    // LEF info.
    for (u_int i = 0; i < db_lef_objcnt; i++)
        delete db_lef_objects[i];
    delete [] db_lef_objects;
    db_lef_objects      = 0;
    db_lef_objsz        = 0;
    db_lef_objcnt       = 0;
    for (u_int i = 0; i < db_lef_gatecnt; i++)
        delete db_lef_gates[i];
    delete [] db_lef_gates;
    db_lef_gates        = 0;
    db_lef_gatesz       = 0;
    db_lef_gatecnt      = 0;
    db_pinMacro         = 0;
    db_mfg_grid         = 0;

    // LEF parser state.
    db_lef_resol_set    = false;
    db_lef_precis_set   = false;
    db_lef_case_sens    = true;
    db_lef_precis       = 1;
    db_lef_resol        = LEFDEF_DEFAULT_RESOL;
}


#define LEF_SZ_INCR 256

// Add a LEF object (LAYER, VIA, VIARULE) to the dattabase.
//
void
cLDDB::lefAddObject(lefObject *lefo)
{
    if (!lefo)
        return;
    if (db_lef_objcnt >= db_lef_objsz) {
        lefObject **tmp = new lefObject*[db_lef_objsz + LEF_SZ_INCR];
        for (u_int i = 0; i < db_lef_objsz; i++)
            tmp[i] = db_lef_objects[i];
        memset(tmp + db_lef_objsz, 0, LEF_SZ_INCR*sizeof(lefObject*));
        delete [] db_lef_objects;
        db_lef_objects = tmp;
        db_lef_objsz += LEF_SZ_INCR;
    }
    lefo->lefId = db_lef_objcnt;
    db_lef_objects[db_lef_objcnt++] = lefo;

    if (lefo->lefClass == CLASS_ROUTE) {
        lefo->layer = db_numLayers++;

        // Allocate or enlarge the db_layers array.  Since we allow
        // incremental reads of LEF files, it is possible to see
        // layer definitions in different files, BUT, THEY MUST BE IN
        // ORDER as read.

        if (db_numLayers > db_allocLyrs) {
            dbLayer *lyrs = new dbLayer[db_numLayers];
            for (u_int i = 0; i < db_allocLyrs; i++) {
                lyrs[i] = db_layers[i];
                db_layers[i].lid.lname = 0;
            }
            delete [] db_layers;
            db_layers = lyrs;
        }
        setLayerName(lefo->layer, lefo->lefName);
        db_layers[lefo->layer].lid.lefId = lefo->lefId;
        db_allocLyrs = db_numLayers;
    }
    else if (lefo->lefClass == CLASS_VIA) {
        lefViaObject *lefv = (lefViaObject*)lefo;

        // Check the X vs Y dimension of the base layer.  If X is
        // longer, save as ViaX.  If Y is longer, save as ViaY.

        if (lefv->via.bot.layer >= 0 && lefv->via.top.layer >= 0) {
            int routelayer = lefv->via.bot.layer;
            lefu_t xydiff = (lefv->via.bot.x2 - lefv->via.bot.x1) -
                (lefv->via.bot.y2 - lefv->via.bot.y1);

            if (lefv->via.top.layer < routelayer) {
                // Shouldn't happen.
                xydiff = (lefv->via.top.x2 - lefv->via.top.x1) -
                    (lefv->via.top.y2 - lefv->via.top.y1);
                routelayer = lefv->via.top.layer;
            }
            if (routelayer < (int)db_numLayers) {
                if (xydiff >= 0)
                    setViaXid(routelayer, lefv->lefId);
                else
                    setViaYid(routelayer, lefv->lefId);
            }

            // Note that the top route layer doesn't get the
            // viaX/Y set.
        }
    }
}


// Add a MACRO to the database.
//
void
cLDDB::lefAddGate(lefMacro *gate)
{
    if (db_lef_gatecnt >= db_lef_gatesz) {
        lefMacro **tmp = new lefMacro*[db_lef_gatesz + LEF_SZ_INCR];
        for (u_int i = 0; i < db_lef_gatesz; i++)
            tmp[i] = db_lef_gates[i];
        memset(tmp + db_lef_gatesz, 0, LEF_SZ_INCR*sizeof(lefMacro*));
        delete [] db_lef_gates;
        db_lef_gates = tmp;
        db_lef_gatesz += LEF_SZ_INCR;
    }
    db_lef_gates[db_lef_gatecnt++] = gate;
}


bool
cLDDB::lefSetCaseSens(bool b)
{
    db_lef_case_sens = b;
    return (0);
}


bool
cLDDB::lefUnitsSet(lefiUnits *unit)
{
    bool rval = LD_OK;
    if (unit->lefiUnits::hasDatabase() && unit->lefiUnits::databaseName() &&
            !strcasecmp(unit->lefiUnits::databaseName(), "MICRONS")) {
#ifdef DEBUG
        printf("UNITS DATABASE %s %g ;\n", unit->lefiUnits::databaseName(),
             unit->lefiUnits::databaseNumber());
#endif
        int resol = (int)(fabs(unit->lefiUnits::databaseNumber()) + 0.5);
        rval = lefResolSet(resol);
    }
    return (rval);
}


// Handle setting the LEF resolution.  Once set, this can't be changed
// with a reset.  This will be the dbu per micron used for all
// coordinates and lengths in the database.
//
bool
cLDDB::lefResolSet(u_int resol)
{
    if (db_lef_resol_set) {
        if (resol != (u_int)db_lef_resol) {
            emitErrMesg(
                "LEF database resolution already set, new value ignored.\n");
        }
        return (LD_OK);
    }

    // LEF only allows certain values, check this.
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
            "Error: LEF dbu/micron %d is not an accepted value.\n", resol);
        return (LD_BAD);
        break;
    }
    db_lef_resol = resol;
    db_lef_resol_set = true;

    // The MANUFACTURINGGRID should not be set yet.
    if (!db_lef_precis_set)
        db_lef_precis = 1;
    return (LD_OK);
}


// Handle setting the manufacturing grid value.
//
// Scaling notes.
//  db_lef_resol is the DATABASE UNITS value from LEF file.
//  db_lef_precis is the number of DATABASE UNITS in the
//  MANUFACTURINGGRID.
//
bool
cLDDB::lefManufacturingSet(double num)
{
#ifdef DEBUG
    printf("MANUFACTURINGGRID %g ;\n", num);
#endif
    if (num == 0.0) {
        db_mfg_grid = 0;
        db_lef_precis = 1;
        db_lef_precis_set = false;
        return (LD_OK);
    }
    if (num < 0)
        num = -num;
    int prec = (int)(num*db_lef_resol + 0.5);
    if (prec == 0) {
        emitErrMesg(
            "Manufacturing resolution %g is smaller than the LEF "
            "resolution %g, ignoring.\n", num, 1.0/db_lef_resol);
    }
    else if ((db_def_resol > prec && db_lef_resol % prec != 0) ||
            (prec > db_lef_resol && prec % db_lef_resol != 0)) {
        emitErrMesg(
            "Manufacturing resolution %d is not a multiple of the LEF "
            "resolution %d or vice-versa, ignoring.\n", prec, db_lef_resol);
    }
    else {
        if (db_lef_precis_set) {
            if (prec != db_lef_precis) {
                emitErrMesg(
                    "Manufacturing grid already set, new value ignored.\n");
            }
        }
        else {
            db_mfg_grid = micToLef(num);
            db_lef_precis = prec;
            db_lef_precis_set = true;
        }
    }
    return (LD_OK);
}


bool
cLDDB::lefLayerSet(lefiLayer *lyr)
{
    const char *lname = lyr->lefiLayer::name();
#ifdef DEBUG
    printf("LAYER %s ...\n", lname);
#endif
    lefObject *lefo = getLefObject(lname);     
    if (lefo) {
        emitError("lefRead: Error, layer %s is multiply defined!\n", lname);
        return (LD_BAD);
    }
    if (!lyr->lefiLayer::hasType()) {
        emitError("lefRead: Warning, layer %s has no type, ignoring.\n", lname);
        return (LD_OK);
    }

    // These are defined in the order of lefCLASS in lddb.h.
    static const char *layer_type_keys[] = {
        "ROUTING",
        "CUT",
        "IMPLANT",
        "MASTERSLICE",
        "OVERLAP",
        0
    };

    const char *tp = lyr->lefiLayer::type();
    int typekey = lookup(tp, layer_type_keys);
    if (typekey < 0) {
        emitError(
            "lefRead: Warning, layer %s unknown type \"%s\", ignoring.\n",
            lname, tp);
        return (LD_OK);
    }
    if (typekey != CLASS_ROUTE) {
        if (typekey == CLASS_CUT) {
            lefCutLayer *cl = new lefCutLayer(lstring::copy(lname));
            if (lyr->lefiLayer::hasSpacingNumber()) {
                for (int i = 0; i < lyr->lefiLayer::numSpacing(); i++) {
                    // Just grab the first number.
                    cl->spacing = micToLefGrid(lyr->lefiLayer::spacing(i));
                    break;
                }
            }
            lefAddObject(cl);
        }
        else if (typekey == CLASS_IMPLANT) {
            lefImplLayer *il = new lefImplLayer(lstring::copy(lname));
            if (lyr->lefiLayer::hasWidth())
                il->width = micToLefGrid(lyr->lefiLayer::width());
            lefAddObject(il);
        }
        else {
            lefAddObject(
                new lefObject(lstring::copy(lname), (lefCLASS)typekey));
            // Nothing more to do.
        }
        return (LD_OK);
    }

    lefRouteLayer *lefr = new lefRouteLayer(lstring::copy(lname));

    // Use -1 as an indication that offset has not
    // been specified and needs to be set to default.
    lefr->route.offsetX = -1;
    lefr->route.offsetY = -1;

    if (lyr->lefiLayer::hasWidth()) {
        lefr->route.width = micToLefGrid(lyr->lefiLayer::width());
    }
    if (lyr->lefiLayer::hasSpacingNumber()) {
        lefu_t sp = micToLefGrid(lyr->lefiLayer::spacing(0));
        if (!lyr->lefiLayer::hasSpacingRange(0)) {
            // If no range specified, then the rule goes in front.
            lefr->route.spacing = new lefSpacingRule(0, sp,
                lefr->route.spacing);
        }
        else {
            // Get range minimum, ignore range maximum, and sort
            // the spacing order.

            lefu_t wd = micToLefGrid(lyr->lefiLayer::spacingRangeMin(0));
            lefSpacingRule *testrule = lefr->route.spacing;
            for ( ; testrule; testrule = testrule->next) {
                if (testrule->next == 0 || testrule->next->width > wd)
                    break;
            }
            if (!testrule)
                lefr->route.spacing = new lefSpacingRule(wd, sp, 0);
            else
                testrule->next = new lefSpacingRule(wd, sp, testrule->next);
        }
    }
    if (lyr->lefiLayer::numSpacingTable()) {
        lefiSpacingTable *sptab = lyr->lefiLayer::spacingTable(0);
        // Use the values for the maximum parallel runlength.
        if (sptab->lefiSpacingTable::isParallel()) {
            lefiParallel *para = sptab->lefiSpacingTable::parallel();
            int entries = para->lefiParallel::numLength();

            for (int i = 0; i < para->lefiParallel::numWidth(); i++) {
                lefu_t w = micToLefGrid(para->lefiParallel::width(i));
                lefu_t s = micToLefGrid(para->lefiParallel::widthSpacing(
                    i, entries-1));
                lefSpacingRule *newrule = new lefSpacingRule(w, s, 0);

                lefSpacingRule *testrule = lefr->route.spacing;
                for ( ; testrule; testrule = testrule->next) {
                    if (testrule->next == 0 || testrule->next->width > w)
                        break;
                }

                if (!testrule) {
                    lefr->route.spacing = newrule;
                }
                else {
                    newrule->next = testrule->next;
                    testrule->next = newrule;
                }
            }
        }
    }
    if (lyr->lefiLayer::hasDirection()) {
        const char *token = lyr->lefiLayer::direction();
        if (token[0] == 'h' || token[0] == 'H')
            lefr->route.direction = DIR_HORIZ;
        else
            lefr->route.direction = DIR_VERT;
    }
    if (lyr->lefiLayer::hasPitch()) {
        lefr->route.pitchX = micToLefGrid(lyr->lefiLayer::pitch());
        lefr->route.pitchY = lefr->route.pitchX;

        // Offset default is 1/2 the pitch.  Offset is
        // intialized to -1 to tell whether or not the value
        // has been set by an OFFSET statement.

        if (lefr->route.offsetX < 0) {
            lefr->route.offsetX = lefr->route.pitchX / 2;
            lefr->route.offsetY = lefr->route.offsetX;
        }
    }
    else if (lyr->lefiLayer::hasXYPitch()) {
        lefr->route.pitchX = micToLefGrid(lyr->lefiLayer::pitchX());
        lefr->route.pitchY = micToLefGrid(lyr->lefiLayer::pitchY());
        if (lefr->route.offsetX < 0)
            lefr->route.offsetX = lefr->route.pitchX / 2;
        if (lefr->route.offsetY < 0)
            lefr->route.offsetY = lefr->route.pitchY / 2;
    }
    if (lyr->lefiLayer::hasOffset()) {
        lefr->route.offsetX = micToLefGrid(lyr->lefiLayer::offset());
        lefr->route.offsetY = lefr->route.offsetX;
    }
    else if (lyr->lefiLayer::hasXYOffset()) {
        lefr->route.offsetX = micToLefGrid(lyr->lefiLayer::offsetX());
        lefr->route.offsetY = micToLefGrid(lyr->lefiLayer::offsetY());
    }

    if (lyr->lefiLayer::hasCapacitance()) {
        lefr->route.areacap = lyr->lefiLayer::capacitance();
    }
    if (lyr->lefiLayer::hasEdgeCap()) {
        lefr->route.edgecap = lyr->lefiLayer::edgeCap();
    }
    if (lyr->lefiLayer::hasResistance()) {
        lefr->route.respersq = lyr->lefiLayer::resistance();
    }
    lefAddObject(lefr);

    return (LD_OK);
}


bool
cLDDB::lefViaSet(lefiVia *via)
{
    const char *vname = via->lefiVia::name();
#ifdef DEBUG
    printf("VIA %s ...\n", vname);
#endif
    lefObject *lefo = getLefObject(vname);
    if (lefo) {
        emitError("lefRead: Error, VIA type \"%s\" multiply defined.\n", vname);
        return (LD_BAD);
    }

    lefViaObject *lefv = new lefViaObject(lstring::copy(vname));
    if (via->lefiVia::hasDefault())
        lefv->via.deflt = true;
    else if (via->lefiVia::hasGenerated())
        lefv->via.generate = true;
    if (via->lefiVia::hasResistance()) {
        lefv->via.respervia = via->lefiVia::resistance();
    }
    if (via->lefiVia::numLayers() == 1 || via->lefiVia::numLayers() == 3) {
        dbDsegB dsegs[3];
        for (int i = 0; i < via->lefiVia::numLayers(); i++) {
            const char *lname = via->lefiVia::layerName(i);
            lefObject *lo = getLefObject(lname);     
            if (!lo) {
                emitError("lefRead: Error, unknown layer \"%s\".\n", lname);
                return (LD_BAD);
            }

            for (int j = 0; j < via->lefiVia::numRects(i); j++) {

                // Rectangles for vias are read in units of 1/2 lambda.
                dsegs[i] = dbDsegB(
                    2*micToLefGrid(via->lefiVia::xl(i, j)),
                    2*micToLefGrid(via->lefiVia::yl(i, j)),
                    2*micToLefGrid(via->lefiVia::xh(i, j)),
                    2*micToLefGrid(via->lefiVia::yh(i, j)),
                    lo->layer, lo->lefId);
#ifdef DEBUG
                printf("via %g %g %g %g %d\n",
                    lefToMic(dsegs[i].x1), lefToMic(dsegs[i].y1),
                    lefToMic(dsegs[i].x2), lefToMic(dsegs[i].y2),
                    dsegs[i].layer);
#endif
                // We can only keep one rectangle.
                break;
            }
        }
        if (via->lefiVia::numLayers() == 1)
            lefv->via.cut = dsegs[0];
        else {
            int cnt = 0;
            for (int i = 0; i < 3; i++) {
                if (dsegs[i].layer < 0)
                    cnt++;
            }
            if (cnt != 1) {
                emitError(
                    "lefRead: Error, composite via \"%s\" not one cut, two "
                    "metal.\n", via->lefiVia::name());
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
                                    "lefRead: Error, composite via \"%s\" "
                                    "duplicate layer.\n", via->lefiVia::name());
                                return (LD_BAD);
                            }
                        }
                    }
                }
            }
        }
        lefAddObject(lefv);
    }
    else {
        delete lefv;
        emitError(
            "lefRead: Error, composite via \"%s\" layer count not 1 or 3.\n",
            via->lefiVia::name());
        return (LD_BAD);
    }

    return (LD_OK);
}


bool
cLDDB::lefViaRuleSet(lefiViaRule *viar)
{
    const char *vname = viar->lefiViaRule::name();
#ifdef DEBUG
    printf("VIARULE %s ...\n", vname);
#endif
    lefObject *lefo = getLefObject(vname);
    if (lefo) {
        // If we've already seen this via, don't reprocess.  This
        // deals with VIA followed by VIARULE.
        return (LD_OK);
    }
    lefViaRuleObject *lefvr = new lefViaRuleObject(lstring::copy(vname));
    if (viar->lefiViaRule::hasGenerate())
        lefvr->via.generate = true;
    if (viar->lefiViaRule::hasDefault())
        lefvr->via.deflt = true;

    for (int i = 0; i < viar->lefiViaRule::numLayers(); i++) {
        lefiViaRuleLayer *vrl = viar->lefiViaRule::layer(i);
        const char *lname = vrl->lefiViaRuleLayer::name();
        lefObject *lo = getLefObject(lname);     
        if (!lo) {
            emitError(
                "lefRead: Warning, unknown layer \"%s\" in via rule.\n",
                lname);
            continue;
        }
        if (lo->layer >= 0) {
            lefRouteLayer *lr = (lefRouteLayer*)lo;
            ROUTE_DIR dir = lr->route.direction;
            lefu_t minw = 0;
            lefu_t maxw = 0;
            lefu_t oh = 0;
            lefu_t moh = 0;
            if (vrl->lefiViaRuleLayer::hasDirection()) {
                dir = vrl->lefiViaRuleLayer::isHorizontal() ?
                    DIR_HORIZ : DIR_VERT;
            }
            if (vrl->lefiViaRuleLayer::hasWidth()) {
                minw = micToLefGrid(vrl->lefiViaRuleLayer::widthMin());
                maxw = micToLefGrid(vrl->lefiViaRuleLayer::widthMax());
            }
            if (vrl->lefiViaRuleLayer::hasOverhang())
                oh = micToLefGrid(vrl->lefiViaRuleLayer::overhang());
            if (vrl->lefiViaRuleLayer::hasMetalOverhang())
                moh = micToLefGrid(vrl->lefiViaRuleLayer::metalOverhang());
            if (!lefvr->met1) {
                lefvr->met1 = new dbVRLyr(lr->layer, lr->lefId, dir,
                    minw, maxw, oh, moh);
            }
            else if (!lefvr->met2) {
                lefvr->met2 = new dbVRLyr(lr->layer, lr->lefId, dir,
                    minw, maxw, oh, moh);
            }
            continue;
        }
        if (vrl->lefiViaRuleLayer::hasRect()) {
            // Rectangles for vias are read in units of 1/2 lambda.

            lefvr->via.cut.x1 = 2*micToLefGrid(vrl->lefiViaRuleLayer::xl());
            lefvr->via.cut.y1 = 2*micToLefGrid(vrl->lefiViaRuleLayer::yl());
            lefvr->via.cut.x2 = 2*micToLefGrid(vrl->lefiViaRuleLayer::xh());
            lefvr->via.cut.y2 = 2*micToLefGrid(vrl->lefiViaRuleLayer::yh());
            lefvr->via.cut.layer = lo->layer;
            lefvr->via.cut.lefId = lo->lefId;

            if (vrl->lefiViaRuleLayer::hasSpacing()) {
                lefvr->spacingX =
                    micToLefGrid(vrl->lefiViaRuleLayer::spacingStepX());
                lefvr->spacingY =
                    micToLefGrid(vrl->lefiViaRuleLayer::spacingStepY());
            }
#ifdef DEBUG
            printf("viaRule %g %g %g %g %d\n",
                lefToMic(lefvr->via.cut.x1), lefToMic(lefvr->via.cut.y1),
                lefToMic(lefvr->via.cut.x2), lefToMic(lefvr->via.cut.y2),
                lefvr->via.cut.layer);
#endif
        }
    }
    if (viar->lefiViaRule::numLayers() == 2 &&
            !viar->lefiViaRule::hasGenerate()) {
        // Two metal layers plus a via name.
        for (int i = 0; i < viar->lefiViaRule::numVias(); i++) {
            const char *vn = viar->lefiViaRule::viaName(i);
            lefObject *o = getLefObject(vn);
            if (o) {
                lefvr->layer = o->layer;
                lefvr->lefId = o->lefId;
            }
        }
    }
    lefAddObject(lefvr);
    return (LD_OK);
}


bool
cLDDB::lefSiteSet(lefiSite *site)
{
    if (db_verbose > 0) {
        emitMesg("LEF file:  Defines site %s (ignored)\n", site->name());
    }
    return (LD_OK);
}


// Create a new cell, and place it at the top of the db_lef_gates
// array.  This can't fail, so it is safe to assume that the "curent
// gate" is the db_lef_gates[db_lef_gatecnt-1] element.
//
bool
cLDDB::lefMacroBegin(const char *mname)
{
#ifdef DEBUG
    printf("MACRO %s ...\n", mname);
#endif
    lefMacro *gate = getLefGate(mname);
    while (gate) {
        char *newname = new char[strlen(mname) + 8];
        char *e = lstring::stpcpy(newname, mname);

        lefMacro *g = gate;
        for (int suffix = 1; g; suffix++) {
            snprintf(e, 12, "_%d", suffix);
            g = getLefGate(newname);
        }
        emitError(
            "lefRead: Warning, cell \"%s\" was already defined in this file,\n"
            "renaming original cell \"%s\".\n", mname, newname);

        delete [] gate->gatename;
        gate->gatename = newname;
        gate = getLefGate(mname);
    }

    // Create the new cell.  The gate->node value is here 0, used to
    // count the pins.

    lefAddGate(new lefMacro(lstring::copy(mname), 0, 0));

    return (LD_OK);
}


// This gets called AFTER the pin and obstruction handlers.
//
bool
cLDDB::lefMacroSet(lefiMacro *macro)
{
#ifdef DEBUG
    printf("END %s\n", macro->lefiMacro::name());
#endif

    // These are defined in the order of MACRO_CLASS in lddb.h.
    static const char *macro_class_keys[] = {
        "unset",
        "COVER",
        "BLOCK",
        "PAD",
        "CORE",
        "ENDCAP",
        "BUMP",
        "BLACKBOX",
        "SOFT",
        "INPUT",
        "OUTPUT",
        "INOUT",
        "POWER",
        "SPACER",
        "AREAIO",
        "FEEDTHRU",
        "TIEHIGH",
        "TIELOW",
        "SPACER",
        "ANTENNACELL",
        "WELLTAP",
        "PRE",
        "POST",
        "TOPLEFT",
        "TOPRIGHT",
        "BOTTOMLEFT",
        "BOTTOMRIGHT",
        0
    };

    lefMacro *gate = db_lef_gates[db_lef_gatecnt - 1];  // current gate
    if (gate) {
        if (macro->lefiMacro::hasClass()) {
            const char *str = macro->lefiMacro::macroClass();
            char *t1 = lstring::gettok(&str);
            int typekey = lookup(t1, macro_class_keys);
            if (typekey >= 0) {
                gate->mclass = typekey;
                char *t2 = lstring::gettok(&str);
                typekey = lookup(t2, macro_class_keys);
                if (typekey >= 0)
                    gate->subclass = typekey;
                delete [] t2;
            }
            delete [] t1;
        }

        if (macro->lefiMacro::hasXSymmetry())
            gate->symmetry |= SYMMETRY_X;
        if (macro->lefiMacro::hasYSymmetry())
            gate->symmetry |= SYMMETRY_Y;
        if (macro->lefiMacro::has90Symmetry())
            gate->symmetry |= SYMMETRY_90;

        if (macro->lefiMacro::hasSiteName())
            gate->sitename = lstring::copy(macro->lefiMacro::siteName());

        if (macro->lefiMacro::hasForeign()) {
            for (int i = 0; i < macro->lefiMacro::numForeigns(); i++) {
                dbForeign *f = new dbForeign(
                    lstring::copy(macro->lefiMacro::foreignName(i)),
                    micToLefGrid(macro->lefiMacro::foreignX(i)),
                    micToLefGrid(macro->lefiMacro::foreignY(i)),
                    dbGate::orientation(macro->lefiMacro::foreignOrientStr()));
                if (!gate->foreign)
                    gate->foreign = f;
                else {
                    dbForeign *fx = gate->foreign;
                    while (fx->next)
                        fx = fx->next;
                    fx->next = f;
                }
            }
        }

        if (macro->lefiMacro::hasSize()) {
            gate->width = micToLefGrid(macro->lefiMacro::sizeX());
            gate->height = micToLefGrid(macro->lefiMacro::sizeY());
        }
        else {
            emitError("lefRead: Warning, gate %s has no size information!\n",
                gate->gatename);
        }
        if (macro->lefiMacro::hasOrigin()) {
            // "placed" for macros (not instances) corresponds to the
            // cell origin.

            gate->placedX = micToLefGrid(-macro->lefiMacro::originX());
            gate->placedY = micToLefGrid(-macro->lefiMacro::originY());
        }

        // Reverse the order of the pins list and count the gates.  The
        // list will now be in the order seen.
        gate->nodes = 0;
        lefPin *p0 = 0;
        while (gate->pins) {
            lefPin *p = gate->pins;
            gate->pins = gate->pins->next;
            p->next = p0;
            p0 = p;
            gate->nodes++;
        }
        gate->pins = p0;
    }
    return (LD_OK);
}


bool
cLDDB::lefPinSet(lefiPin *pin)
{
    lefMacro *gate = db_lef_gates[db_lef_gatecnt - 1];  // current gate
    if (!gate)
        return (LD_BAD);

    int pinDir = PORT_CLASS_UNSET;
    int pinUse = PORT_USE_UNSET;
    int pinShape = PORT_SHAPE_UNSET;
    if (pin->lefiPin::hasDirection()) {
        const char *token = pin->lefiPin::direction();
        int subkey = lookup(token, pin_classes);
        if (subkey < 0)
            emitError("lefRead: Warning, improper DIRECTION statement.\n");
        else
            pinDir = subkey;
    }
    if (pin->lefiPin::hasUse()) {
        const char *token = pin->lefiPin::use();
        int subkey = lookup(token, pin_uses);
        if (subkey < 0)
            emitError("lefRead: Warning, improper USE statement.\n");
        else
            pinUse = subkey;
    }
    if (pin->lefiPin::hasShape()) {
        const char *token = pin->lefiPin::shape();
        int subkey = lookup(token, pin_shapes);
        if (subkey < 0)
            emitError("lefRead: Warning, improper SHAPE statement.\n");
        else
            pinShape = subkey;
    }
    dbDseg *rectList = 0;
    int nports = pin->lefiPin::numPorts();
    for (int i = 0; i < nports; i++) {
        lefiGeometries *geom = pin->lefiPin::port(i);

        dbDseg *rl = lefProcessGeometry(geom);
        if (rl) {
            if (!rectList)
                rectList = rl;
            else {
                dbDseg *rn = rl;
                while (rn->next)
                    rn = rn->next;
                rn->next = rectList;
                rectList = rl;
            }
        }
    }
#ifdef DEBUG
    printf("PIN %s\n", pin->lefiPin::name());
#endif
    gate->pins = new lefPin(lstring::copy(pin->lefiPin::name()),
        rectList, pinDir, pinUse, pinShape, gate->pins);
#ifdef DEBUG
    if (!rectList)
        printf("BAD! %s no taps\n", pin->lefiPin::name());
    for (dbDseg *sg = rectList; sg; sg = sg->next) {
        if (sg->layer < 0 || sg->layer >= (int)db_numLayers) {
            printf("BAD! %s non-routing layer\n", pin->lefiPin::name());
        }
    }
#endif

    return (LD_OK);
}


bool
cLDDB::lefObstructionSet(lefiObstruction *obs)
{
    lefiGeometries *geom = obs->lefiObstruction::geometries();
#ifdef DEBUG
    printf("OBS\n");
#endif

    lefMacro *gate = db_lef_gates[db_lef_gatecnt - 1];  // current gate
    if (gate) {
        gate->obs = lefProcessGeometry(geom);
#ifdef DEBUG
        for (dbDseg *sg = gate->obs; sg; sg = sg->next) {
            printf("obs %g %g %g %g %d\n",
                lefToMic(sg->x1), lefToMic(sg->y1),
                lefToMic(sg->x2), lefToMic(sg->y2), sg->layer);
        }
#endif
    }

    return (LD_OK);
}


// Process geometry description of pin shapes and obstructions.  Only
// routing layer geometry is retured.
//
dbDseg *
cLDDB::lefProcessGeometry(lefiGeometries *geom)
{
    dbDseg *rectlist = 0;
    lefObject *lefo = 0;
    int nitems = geom->lefiGeometries::numItems();
    for (int i = 0; i < nitems; i++) {
        switch (geom->lefiGeometries::itemType(i)) {

        case lefiGeomLayerE:
            {
                const char *lname = geom->lefiGeometries::getLayer(i);
                lefo = getLefObject(lname);
                if (!lefo) {
                    emitError(
                        "lefRead, Warning, no layer \"%s\" defined for "
                        "RECT/POLYGON.\n", lname);
                }
            }
            break;

        case lefiGeomRectE:
            if (lefo && lefo->layer >= 0) {
                lefiGeomRect *rect = geom->lefiGeometries::getRect(i);
                rectlist = new dbDseg(
                    micToLefGrid(rect->xl), micToLefGrid(rect->yl),
                    micToLefGrid(rect->xh), micToLefGrid(rect->yh),
                    lefo->layer, lefo->lefId, rectlist);
            }
            break;

        case lefiGeomPolygonE:
            if (lefo && lefo->layer >= 0) {
                lefiGeomPolygon *poly = geom->lefiGeometries::getPolygon(i);
                dbDpoint *plist = 0;
                for (int j = 0; j < poly->lefiGeomPolygon::numPoints; j++) {
                    plist = new dbDpoint(
                        micToLefGrid(poly->x[j]), micToLefGrid(poly->y[j]),
                        lefo->layer, lefo->lefId, plist);
                }
                polygonToRects(&rectlist, plist);
            }
            break;

        default:
            break;
        }
    }
    return (rectlist);
}


// lefPostSetup
//
// This should be called after all LEF files (or equivalent) have been
// read, and before any DEF files have been read.  This will actually
// be called by the DEF reader.
//
void
cLDDB::lefPostSetup()
{
    // Make sure that the gate list has one entry called "pin".
    //
    lefMacro *gateginfo = getLefGate("pin");
    if (!gateginfo) {
        // Add a new db_lef_gates entry for pseudo-gate "pin".

        gateginfo = new lefMacro(lstring::copy("pin"), 0, 0);
        gateginfo->mclass = MACRO_CLASS_INTERNAL;
        gateginfo->nodes = 1;
        gateginfo->pins = new lefPin(lstring::copy("pin"), new dbDseg,
            PORT_CLASS_UNSET, PORT_USE_UNSET, PORT_SHAPE_UNSET, 0);
        lefAddGate(gateginfo);
    }
    db_pinMacro = gateginfo;

    // Create hash table for gates.
    if (!db_lef_gate_hash) {
        if (db_lef_gatecnt > 16) {
            db_lef_gate_hash = new dbHtab(!db_lef_case_sens, db_lef_gatecnt);
            for (u_int i = 0; i < db_lef_gatecnt; i++) {
                lefMacro *g = db_lef_gates[i];
                if (g && g->gatename)
                    db_lef_gate_hash->add(g->gatename, i);
            }
        }
    }

    // Create hash table for LEF objects.
    if (!db_lef_obj_hash) {
        if (db_lef_objcnt > 16) {
            db_lef_obj_hash = new dbHtab(!db_lef_case_sens, db_lef_objcnt);
            for (u_int i = 0; i < db_lef_objcnt; i++) {
                lefObject *o = db_lef_objects[i];
                if (o && o->lefName)
                    db_lef_obj_hash->add(o->lefName, i);
            }
        }
    }
}

