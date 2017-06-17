
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
 $Id: lddb_prv.h,v 1.18 2017/02/17 05:44:38 stevew Exp $
 *========================================================================*/

#ifndef LDDB_PRV_H
#define LDDB_PRV_H

#include "lddb.h"


//
// LEF/DEF Database.
//
// Private include file for main class definition.
//


// Max # errors to report; limits output if something is really wrong
// with the file.
#define LD_MAX_ERRORS           100

// Default metal width for routes if undefined.
#define LD_DEFAULT_WIDTH        3

// Default spacing between metal if undefined.
#define LD_DEFAULT_SPACING      4

// LEF and DEF default dbu per micron.
#define LEFDEF_DEFAULT_RESOL    100

// Default i/o handling.
//
class cLDstdio : public cLDio
{
public:
    virtual ~cLDstdio() { }

    void emitErrMesg(const char*);
    void flushErrMesg();
    void emitMesg(const char*);
    void flushMesg();
    void destroy();
};


// Hash table defined in ld_hash.h/ld_hash.cc.
struct dbHtab;

// The LDDB class, contains the LEF/DEF database implementation..
//
class cLDDB : public cLDDBif
{
public:
    // lddb.cc
    cLDDB();
    virtual ~cLDDB();

    bool    reset();
    bool    setupChannels(bool);
    void    emitError(const char *fmt, ...);
    void    emitErrMesg(const char *fmt, ...);
    void    flushErrMesg();
    void    emitMesg(const char *fmt, ...);
    void    flushMesg();
    int     getLayer(const char*);
    lefu_t  getRouteKeepout(int);
    lefu_t  getRouteWidth(int);
    lefu_t  getRouteOffset(int, ROUTE_DIR);
    lefu_t  getViaWidth(int, int, ROUTE_DIR);
    lefu_t  getXYViaWidth(int, int, ROUTE_DIR, int);
    lefu_t  getRouteSpacing(int);
    lefu_t  getRouteWideSpacing(int, lefu_t);
    lefu_t  getRoutePitch(int, ROUTE_DIR);
    const char *getRouteName(int);
    ROUTE_DIR getRouteOrientation(int);
    dbGate  *getGate(const char*);
    int     getGateNum(const char*);
    dbGate  *getPin(const char*);
    int     getPinNum(const char*);
    dbGate  *getGateOrPinByNum(int);
    dbNet   *getNet(const char*);
    dbNet   *getNetByNum(u_int);
    lefMacro *getLefGate(const char*);
    lefObject *getLefObject(const char*);
    lefRouteLayer *getLefRouteLayer(const char*);
    lefRouteLayer *getLefRouteLayer(int);
    void    printInfo(FILE*);
    const char *printNodeName(dbNode*);
    void    printNets(const char*);
    void    printNodes(const char*);
    void    printRoutes(const char*);
    void    printNlgates(const char*);
    void    printNlnets(const char*);
    void    printNet(dbNet*);
    void    printGate(dbGate*);
    void    checkVariablePitch(int, int*, int*);
    void    addObstruction(lefu_t, lefu_t, lefu_t, lefu_t, u_int);
    dbDseg  *findObstruction(lefu_t, lefu_t, lefu_t, lefu_t, u_int);

    // ld_cmds.cc
    bool    readScript(const char*);
    bool    readScript(FILE*);
    bool    doCmd(const char*);
    bool    cmdSet(const char*);
    bool    cmdUnset(const char*);
    bool    cmdIgnore(const char*);
    bool    cmdCritical(const char*);
    bool    cmdObstruction(const char*);
    bool    cmdLayer(const char*);
    bool    cmdNewLayer(const char*);
    bool    cmdBoundary(const char*);
    bool    cmdReadLef(const char*);
    bool    cmdReadDef(const char*);
    bool    cmdWriteLef(const char*);
    bool    cmdWriteDef(const char*);
    bool    cmdUpdateDef(const char*, const char*);

    // ld_def_in.cc
    bool    defRead(const char*);
    void    defReset();
    bool    defSetCaseSens(bool);
    bool    defTechnologySet(const char*);
    bool    defDesignSet(const char*);
    bool    defUnitsSet(double);
    bool    defResolSet(u_int);
    bool    defTracksSet(LefDefParser::defiTrack*);
    bool    defDieAreaSet(LefDefParser::defiBox*);
    bool    defComponentsBegin(int);
    bool    defComponentsSet(LefDefParser::defiComponent*);
    bool    defComponentsEnd();
    bool    defBlockagesBegin(int);
    bool    defBlockagesSet(LefDefParser::defiBlockage*);
    bool    defBlockagesEnd();
    bool    defViasBegin(int);
    bool    defViasSet(LefDefParser::defiVia*);
    bool    defViasEnd();
    bool    defPinsBegin(int);
    bool    defPinsSet(LefDefParser::defiPin*);
    bool    defPinsEnd();
    bool    defSpecialNetsBegin(int);
    bool    defSpecialNetsSet(LefDefParser::defiNet*);
    bool    defSpecialNetsEnd();
    bool    defNetsBegin(int);
    bool    defNetsSet(LefDefParser::defiNet*);
    bool    defNetsEnd();

    // ld_def_out.cc
    bool    defOutResolSet(u_int);
    bool    defWrite(const char*);
    bool    writeDefRoutes(const char*, const char*);
    bool    writeDefNetRoutes(FILE*, dbNet*, bool);
    bool    writeDefStubs(FILE*);

    // ld_lef_in.cc
    bool    lefRead(const char*, bool = false);
    void    lefReset();
    void    lefAddObject(lefObject*);
    void    lefAddGate(lefMacro*);
    bool    lefSetCaseSens(bool);
    bool    lefUnitsSet(LefDefParser::lefiUnits*);
    bool    lefResolSet(u_int);
    bool    lefManufacturingSet(double);
    bool    lefLayerSet(LefDefParser::lefiLayer*);
    bool    lefViaSet(LefDefParser::lefiVia*);
    bool    lefViaRuleSet(LefDefParser::lefiViaRule*);
    bool    lefSiteSet(LefDefParser::lefiSite*);
    bool    lefMacroBegin(const char*);
    bool    lefMacroSet(LefDefParser::lefiMacro*);
    bool    lefPinSet(LefDefParser::lefiPin*);
    bool    lefObstructionSet(LefDefParser::lefiObstruction*);

    // ld_lef_out.cc
    bool    lefWrite(const char*, LEF_OUT = LEF_OUT_ALL);

    // ld_util.cc
    long    millisec();
    double  coresize();

    //
    // Config info.
    //

    cLDio   *ioHandler()                { return (db_io); }
    void    setIOhandler(cLDio *io)
        {
            if (db_io)
                db_io->destroy();
            db_io = io;
        }

    const char *global(u_int i)
        {
            return ((i < db_numGlobals) ? db_global_names[i] : 0);
        }
    bool    addGlobal(const char *s)
        {
            if (s && *s && db_numGlobals < LD_MAX_GLOBALS) {
                char *ss = lddb::copy(s);
                delete [] db_global_names[db_numGlobals];
                db_global_names[db_numGlobals] = ss;
                db_global_nnums[db_numGlobals] = db_numGlobals + 1;
                db_numGlobals++;
                return (LD_OK);
            }
            return (LD_BAD);
        }
    void    clearGlobal(int i = 0)
        {
            for ( ; i < LD_MAX_GLOBALS; i++) {
                delete [] db_global_names[i];
                db_global_names[i] = 0;
                db_global_nnums[i] = i+1;
            }
        }

    dbStringList *noRouteList()         { return (db_dontRoute); }
    void    setNoRouteList(dbStringList *s)     { db_dontRoute = s; }
    void    dontRoute(const char *s)
        {
            if (!s)
                return;
            if (db_dontRoute) {
                for (dbStringList *sl = db_dontRoute; sl; sl = sl->next) {
                    if (!strcmp(s, sl->string))
                        break;
                    if (!sl->next) {
                        sl->next = new dbStringList(lddb::copy(s), 0);
                        break;
                    }
                }
            }
            else
                db_dontRoute = new dbStringList(lddb::copy(s), 0);
        }

    dbStringList *criticalNetList()     { return (db_criticalNet); }
    void    setCriticalNetList(dbStringList *s)     { db_criticalNet = s; }
    void    criticalNet(const char *s)
        {
            if (!s)
                return;
            if (db_criticalNet) {
                for (dbStringList *sl = db_criticalNet; sl; sl = sl->next) {
                    if (!strcmp(s, sl->string))
                        break;
                    if (!sl->next) {
                        sl->next = new dbStringList(lddb::copy(s), 0);
                        break;
                    }
                }
            }
            else
                db_criticalNet = new dbStringList(lddb::copy(s), 0);
        }

    const char *commentLayerName()      { return (db_commentLayer.lname); }
    void    setCommentLayerName(const char *s)
                                        { db_commentLayer.set_layername(s); }
    int     commentLayerNumber()        { return (db_commentLayer.layer); }
    void    setCommentLayerNumber(int n)    { db_commentLayer.layer = n; }
    int     commentLayerPurpose()       { return (db_commentLayer.dtype); }
    void    setCommentLayerPurpose(int d)   { db_commentLayer.dtype = d; }

    u_int   verbose()                   { return (db_verbose); }
    void    setVerbose(u_int i)         { db_verbose = (i & 7); }

    u_int   debug()                     { return (db_debug); }
    void    setDebug(u_int i)           { db_debug = i; }

    //
    // Routing layers.
    //

    u_int   numLayers()                 { return (db_numLayers); }
    void    setNumLayers(u_int n)
        {
            if (n > 0 && n <= db_allocLyrs)
                db_numLayers = n;
        }
    u_int   allocLayers()               { return (db_allocLyrs); }

    const char *layerName(u_int i)
        { return (i < db_numLayers ? db_layers[i].lid.lname : 0); }
    void    setLayerName(u_int i, const char *s)
        {
            if (i < db_numLayers)
                db_layers[i].lid.set_layername(s);
        }

    lefu_t  pathWidth(u_int i)
        { return (i < db_numLayers ? db_layers[i].pathWidth : 0); }
    void    setPathWidth(u_int i, lefu_t w)
        { if (i < db_numLayers) db_layers[i].pathWidth = w; }

    lefu_t  startX(u_int i)
        { return (i < db_numLayers ? db_layers[i].startX : 0); }
    void    setStartX(u_int i, lefu_t p)
        { if (i < db_numLayers) db_layers[i].startX = p; }

    lefu_t  startY(u_int i)
        { return (i < db_numLayers ? db_layers[i].startY : 0); }
    void    setStartY(u_int i, lefu_t p)
        { if (i < db_numLayers) db_layers[i].startY = p; }

    lefu_t  pitchX(u_int i)
        { return (i < db_numLayers ? db_layers[i].pitchX : 0); }
    void    setPitchX(u_int i, lefu_t p)
        { if (i < db_numLayers) db_layers[i].pitchX = p; }

    lefu_t  pitchY(u_int i)
        { return (i < db_numLayers ? db_layers[i].pitchY : 0); }
    void    setPitchY(u_int i, lefu_t p)
        { if (i < db_numLayers) db_layers[i].pitchY = p; }

    int     numChannelsX(u_int i)
        { return (i < db_numLayers ? db_layers[i].numChanX : 0); }
    void    setNumChannelsX(u_int i, u_int n)
        { if (i < db_numLayers) db_layers[i].numChanX = n; }

    int     numChannelsY(u_int i)
        { return (i < db_numLayers ? db_layers[i].numChanY : 0); }
    void    setNumChannelsY(u_int i, u_int n)
        { if (i < db_numLayers) db_layers[i].numChanY = n; }

    int     viaXid(u_int i)
        { return (i < db_numLayers ? db_layers[i].viaXid : 0); }
    void    setViaXid(u_int i, int vid)
        {
            if (i < db_numLayers)
                db_layers[i].viaXid = vid;
        }

    int     viaYid(u_int i)
        { return (i < db_numLayers ? db_layers[i].viaYid : 0); }
    void    setViaYid(u_int i, int vid)
        {
            if (i < db_numLayers)
                db_layers[i].viaYid = vid;
        }

    lefu_t  haloX(u_int i)
        { return (i < db_numLayers ? db_layers[i].haloX : 0); }
    void    setHaloX(u_int i, lefu_t d)
        { if (i < db_numLayers) db_layers[i].haloX = d; }

    lefu_t  haloY(u_int i)
        { return (i < db_numLayers ? db_layers[i].haloY : 0); }
    void    setHaloY(u_int i, lefu_t d)
        { if (i < db_numLayers) db_layers[i].haloY = d; }

    int     layerNumber(u_int i)
        { return (i < db_numLayers ? db_layers[i].lid.layer : -1); }
    void    setLayerNumber(u_int i, int l)
        { db_layers[i].lid.layer = l; }

    int     purposeNumber(u_int i)
        { return (i < db_numLayers ? db_layers[i].lid.dtype : -1); }
    void    setPurposeNumber(u_int i, int d)
        { db_layers[i].lid.dtype = d; }

    bool    vert(u_int i)
        { return (i < db_numLayers ? db_layers[i].vert : false); }
    void    setVert(u_int i, bool b)
        { db_layers[i].vert = b; }

    u_int   needBlock(u_int i)
        { return (i < db_numLayers ? db_layers[i].needBlock : 0); }
    void    setNeedBlock(u_int i, u_int b)
        { db_layers[i].needBlock = b; }

    // LEF/DEF scaling.
    // length values are saved internally in LEF (integer) units.  The
    // following functions convert between microns, LEF, and DEF
    // units.
    //

    u_int   lefResol()                  { return (db_lef_resol); }
    bool    setLefResol(u_int r)        { return (lefResolSet(r)); }

    u_int   defInResol()                { return (db_def_resol); }
    bool    setDefInResol(u_int r)      { return (defResolSet(r)); }
    u_int   defOutResol()               { return (db_def_out_resol); }
    bool    setDefOutResol(u_int r)     { return (defOutResolSet(r)); }

    // Given a value in microns, return the equivalent LEF units.
    //
    lefu_t  micToLef(double x)
        {
            lefu_t i;
            if (x >= 0.0)
                i = (int)(x*db_lef_resol + 0.5);
            else
                i = (int)(x*db_lef_resol - 0.5);
            return (i);
        }

    // Convert microns to LEF units, and in addition snap the value to
    // the manufacturing grid.  Internal values are always snapped to
    // the manufacturing grid.
    //
    lefu_t  micToLefGrid(double m)
        {
            lefu_t i = micToLef(m);
            if (i >= 0)
                i = ((i + db_lef_precis/2)/db_lef_precis)*db_lef_precis;
            else
                i = ((i - db_lef_precis/2)/db_lef_precis)*db_lef_precis;
            return (i);
        }

    // Return a LEF value on grid given a DEF value.
    //
    lefu_t  defToLefGrid(double d)
        {
            double m = (d/db_def_resol);  // microns
            return (micToLefGrid(m));
        }

    // Return the value in microns given the LEF value.
    //
    double  lefToMic(lefu_t u)
        {
            return (u/(double)db_lef_resol);
        }

    // Return DEF units given LEF units.
    //
    int     lefToDef(lefu_t u)
        {
            double m = (u/(double)db_lef_resol);
            int d;
            if (m >= 0.0)
                d = (int)(m*db_def_out_resol + 0.5);
            else
                d = (int)(m*db_def_out_resol - 0.5);
            return (d);
        }

    // Set/query the manufacturing grid.
    //
    lefu_t  manufacturingGrid()         { return (db_mfg_grid); }
    bool    setManufacturingGrid(lefu_t m)
        { return (lefManufacturingSet(lefToMic(m))); }

    //
    // LEF info
    //

    lefObject *getLefObject(u_int n)
        {
            if (n < db_lef_objcnt)
                return (db_lef_objects[n]);
            return (0);
        }

    lefMacro *pinMacro()                { return (db_pinMacro); }

    //
    // DEF info.
    //

    const char *technology()            { return (db_technology); }
    void    setTechnology(const char *s)
        {
            char *t = lddb::copy(s);
            delete [] db_technology;
            db_technology = t;
        }

    const char *design()                { return (db_design); }
    void    setDesign(const char *s)
        {
            char *t = lddb::copy(s);
            delete [] db_design;
            db_design = t;
        }

    dbGate  *nlGate(u_int i)
        { return (i < db_numGates ? db_nlGates[i] : 0); }
    dbGate  *nlPin(u_int i)
        { return (i < db_numPins ? db_nlPins[i] : 0); }
    dbNet   *nlNet(u_int i)
        { return (i < db_numNets ? db_nlNets[i] : 0); }

    dbDseg  *userObs()                  { return (db_userObs); }
    void    setUserObs(dbDseg *o)       { db_userObs = o; }
    dbDseg  *intObs()                   { return (db_intObs); }
    void    setIntObs(dbDseg *o)        { db_intObs = o; }

    u_int   numGates()                  { return (db_numGates); }
    u_int   numPins()                   { return (db_numPins); }
    u_int   numNets()                   { return (db_numNets); }
    u_int   maxNets()                   { return (db_maxNets); }
    void    setMaxNets(u_int n)         { db_maxNets = n; }
    u_int   dfMaxNets()                 { return (db_dfMaxNets); }
    void    setDfMaxNets(u_int n)       { db_dfMaxNets = n; }

    lefu_t  xLower()                    { return (db_xLower); }
    void    setXlower(lefu_t x)         { db_xLower = x; }
    lefu_t  yLower()                    { return (db_yLower); }
    void    setYlower(lefu_t y)         { db_yLower = y; }
    lefu_t  xUpper()                    { return (db_xUpper); }
    void    setXupper(lefu_t x)         { db_xUpper = x; }
    lefu_t  yUpper()                    { return (db_yUpper); }
    void    setYupper(lefu_t y)         { db_yUpper = y; }

    //
    // Parser.
    //

    int     currentLine()               { return (db_currentLine); }
    void    setCurrentLine(int n)       { db_currentLine = n; }

    //
    // Command interface.
    //

    const char *doneMsg()               { return (db_donemsg); }
    void    setDoneMsg(char *s)         { db_donemsg = s; }

    const char *warnMsg()               { return (db_warnmsg); }
    void    setWarnMsg(char *s)
        {
            if (!db_warnmsg)
                db_warnmsg = s;
            else {
                int len = strlen(db_warnmsg) + strlen(s) + 2;
                char *t = new char[len];
                sprintf(t, "%s\n%s", db_warnmsg, s);
                delete [] s;
                delete [] db_warnmsg;
                db_warnmsg = t;
            }
        }

    const char *errMsg()                { return (db_errmsg); }
    void    setErrMsg(char *s)          { db_errmsg = s; }

    void    clearMsgs()
        {
            delete [] db_donemsg;
            db_donemsg = 0;
            delete [] db_warnmsg;
            db_warnmsg = 0;
            delete [] db_errmsg;
            db_errmsg = 0;
        }

protected:
    // lddb.cc
    void    polygonToRects(dbDseg**, dbDpoint*);
    int     lookup(const char*, const char**);
    void    checkNodes();

    // ld_def_in.cc
    bool    defFinishTracks();
    void    defReadNet(LefDefParser::defiNet*, bool);
    void    defReadGatePin(dbNet*, dbNode*, const char*, const char*);
    void    defAddRoutes(LefDefParser::defiWire*, dbNet*, bool);

    // ld_lef_in.cc
    dbDseg  *lefProcessGeometry(LefDefParser::lefiGeometries*);
    void    lefPostSetup();

    // ld_lef_out.cc
    bool    lefWriteLayer(lefObject*);
    bool    lefWriteVia(lefViaObject*);
    bool    lefWriteViaRule(lefViaRuleObject*);
    bool    lefWriteMacro(lefMacro*);

    int defStrcmp(const char *s1, const char *s2)
        {
            if (db_def_case_sens)
                return (strcmp(s1, s2));
            return (strcasecmp(s1, s2));
        }

    int lefStrcmp(const char *s1, const char *s2)
        {
            if (db_lef_case_sens)
                return (strcmp(s1, s2));
            return (strcasecmp(s1, s2));
        }

    lefu_t minPitch(u_int i)
        {
            lefu_t px = db_layers[i].pitchX;
            lefu_t py = db_layers[i].pitchY;
            return (px < py ? px : py);
        }

    // Config info.
    cLDio   *db_io;                 // Pointer to i/o handler;
    char    *db_global_names[LD_MAX_GLOBALS];   // Power/ground net names.
    int     db_global_nnums[LD_MAX_GLOBALS];    // Power/ground net numbers.
    dbStringList *db_dontRoute;     // List of nets not to route (e.g., power).
    dbStringList *db_criticalNet;   // List of critical nets to route first.
    dbLayerId db_commentLayer;      // Layer for annotation.
    dbLayer *db_layers;             // Routing layer info.
    u_int   db_numGlobals;          // Number of global nets defined.
    u_int   db_numLayers;           // Number of routing layers defined.
    u_int   db_allocLyrs;           // Number of routing layer structs alloc'd.
    u_short db_verbose;             // Verbosity level.
    u_short db_debug;               // Internal debugging flags.

    // LEF info.
    lefObject **db_lef_objects;     // Layer, Via, and ViaRule objects.
    dbHtab  *db_lef_obj_hash;       // Hash table for LEF objects.
    u_int   db_lef_objsz;           // Size of array.
    u_int   db_lef_objcnt;          // Count of such objects.
    lefMacro **db_lef_gates;        // Standard cell macro information.
    dbHtab  *db_lef_gate_hash;      // Hast table for LEF gates.
    u_int   db_lef_gatesz;          // Size of array.
    u_int   db_lef_gatecnt;         // Count of macros.
    lefMacro *db_pinMacro;          // Macro definition for a pin.
    lefu_t  db_mfg_grid;            // Manufacturing grid.
    int     db_lef_precis;          // LEF dbu per mfg. grid.
    int     db_lef_resol;           // LEF dbu per micron, from UNITS.

    // DEF info.
    char    *db_technology;
    char    *db_design;
    dbGate  **db_nlGates;           // Gate instance information.
    dbHtab  *db_gate_hash;          // Hash table for gates.
    dbGate  **db_nlPins;            // Pin instance information.
    dbHtab  *db_pin_hash;           // Hash table for pins.
    dbNet   **db_nlNets;            // List of nets in the design.
    dbHtab  *db_net_hash;           // Hash table for nets.
    dbDseg  *db_userObs;            // User-defined obstructions.
    dbDseg  *db_intObs;             // Internally generated obstructions.
    u_int   db_numGates;            // Number of elements in db_nlGates.
    u_int   db_numPins;             // Number of elements in dp_nlPins.
    u_int   db_numNets;             // Number of nets defined.
    u_int   db_maxNets;             // Maximum number of nets allowed.
    u_int   db_dfMaxNets;           // Default maximum number of nets.
    lefu_t  db_xLower;              // Bounding box of routes.
    lefu_t  db_xUpper;
    lefu_t  db_yLower;
    lefu_t  db_yUpper;
    int     db_def_resol;           // DEF dbu per micron, from UNITS.
    int     db_def_out_resol;       // DEF dbu per micron for output.

    // Parser.
    int     db_currentLine;         // Line number in file being read.
    int     db_errors;              // LEF/DEF parse error count.

    // DEF parser state.
    u_int   db_def_total;           // Record count.
    u_int   db_def_processed;       // Records completed.
    int     db_def_netidx;          // Current net index.
    char    db_def_corient;         // Track orientation.
    bool    db_def_case_sens;       // DEF case-sensitivity flag.
    bool    db_def_resol_set;       // DEF dbu/micron set.
    bool    db_def_tracks_set;      // Channel counts set.

    // LEF parser state.
    bool    db_lef_resol_set;       // LEF dbu/micron set.
    bool    db_lef_precis_set;      // MANUFACTURINGGRID set.
    bool    db_lef_case_sens;       // LEF case-sensitivity flag.

    // Command interface.
    char    *db_donemsg;    // Optional successful termination message.
    char    *db_warnmsg;    // Optional successful termination warning.
    char    *db_errmsg;     // Error message, always set on error.

    static const char *pin_classes[];
    static const char *pin_uses[];
    static const char *pin_shapes[];
};

#endif

