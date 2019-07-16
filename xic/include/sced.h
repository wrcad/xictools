
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
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef SCED_H
#define SCED_H

#include "scedif.h"


//
// Variables
//

// Side Menu Commands
#define VA_DevMenuStyle         "DevMenuStyle"

// Attributes Menu
#define VA_ShowDots             "ShowDots"

// Spice Interface (see scedif.h)
#define VA_SpiceListAll         "SpiceListAll"
#define VA_SpiceAlias           "SpiceAlias"
#define VA_SpiceHost            "SpiceHost"
// #define VA_SpiceHostDisplay     "SpiceHostDisplay"
#define VA_SpiceInclude         "SpiceInclude"
#define VA_SpiceProg            "SpiceProg"
#define VA_SpiceExecDir         "SpiceExecDir"
#define VA_SpiceExecName        "SpiceExecName"
#define VA_SpiceSubcCatchar     "SpiceSubcCatchar"
#define VA_SpiceSubcCatmode     "SpiceSubcCatmode"
#define VA_CheckSolitary        "CheckSolitary"
#define VA_NoSpiceTools         "NoSpiceTools"

// Extract Menu Commands
#define VA_SourceTermDevName    "SourceTermDevName"
#define VA_SourceGndDevName     "SourceGndDevName"

// Properties for the device library.  These can be given in the
// device.lib file keyed by either the number or the text token, e.g.,
//   Property 20 ...
//   Property SpiceDotSave ...

// Add .saves, text is  'device_prefix param_name' (e.g., I c)
// to add ".save @Ixxx[c]" lines.  Added to all spice decks created.
#define LpSpiceDotSaveVal 20
#define LpSpiceDotSaveStr "SpiceDotSave"

// Add a default node, text is 'device_name num_nodes string'
// (e.g., nmos 4 NSUB) to add a 4'th node "NSUB" to nmos device with
// fewer than 4 nodes.
#define LpDefaultNodeVal  21
#define LpDefaultNodeStr  "DefaultNode"

// OBSOLETE
// Add a prefix/device mapping, text is 'prefix opt val nnodes nname pname'
//  prefix  short device id prefix, first char must be alpha.  This must
//          match a device in the library.
//  opt     0,no,off or 1,yes,on (binary). If true, last node presence is
//          optional (such as for bjt).
//  val     0,no,off or 1,yes,on (binary). If true, all text goes into
//          value string (such as for sources).
//  nnodes  number of device nodes, including optional.
//  nname   device name for device, or n-type device, in library.
//  pname   if not 0 or missing, the device name for the p-type library
//          device.
// (e.g., m 0 0 4 nmos pmos)
#define LpDeviceKeyVal    22
#define LpDeviceKeyStr    "DeviceKey"

// Add a prefix/device mapping, text is 'prefix min max devs val nname pname'
//  prefix  short device id prefix, first char must be alpha.  This must
//          match a device in the library.
//  min     min number of nodes.
//  max     max number of nodes.
//  devs    current-controlled device references.
//  val     0,no,off or 1,yes,on (binary). If true, all text goes into
//          value string, device has no model,
//  nname   device name for device, or n-type device, in library.
//  pname   if not 0 or missing, the device name for the p-type library
//          device.
// (e.g., m 4 4 0 0 nmos pmos)
#define LpDeviceKeyV2Val  23
#define LpDeviceKeyV2Str  "DeviceKeyV2"

// Arg to PopUpTermEdit
struct TermEditInfo
{
    TermEditInfo(const CDp_snode*, int);
    TermEditInfo(const CDsterm*);
    TermEditInfo(const CDp_bsnode*);

    TermEditInfo(const char *n, const char *lnm, unsigned int ix, unsigned int f,
        bool p)
        {
            ti_name = n;
            ti_netex = 0;
            ti_layer_name = lnm;
            ti_flags = f;
            ti_index = ix;
            ti_beg = 0;
            ti_end = 0;
            ti_bterm = false;
            ti_has_phys = p;
        }

    TermEditInfo(const char *n, unsigned int ix, unsigned int f, const char *nx)
        {
            ti_name = n;
            ti_netex = lstring::copy(nx);
            ti_layer_name = 0;
            ti_flags = f;
            ti_index = ix;
            ti_beg = 0;
            ti_end = 0;
            ti_bterm = true;
            ti_has_phys = false;
        }

    ~TermEditInfo()
        {
            delete [] ti_netex;
        }

    const char *name()          const { return (ti_name); }
    const char *netex()         const { return (ti_netex); }
    const char *layer_name()    const { return (ti_layer_name); }
    unsigned int flags()        const { return (ti_flags); }
    bool has_flag(unsigned f)   const { return (ti_flags & f); }
    unsigned int index()        const { return (ti_index); }
    unsigned int beg_range()    const { return (ti_beg); }
    unsigned int end_range()    const { return (ti_end); }
    bool has_bterm()            const { return (ti_bterm); }
    bool has_phys()             const { return (ti_has_phys); }

private:
    const char *ti_name;        // node/terminal name
    char *ti_netex;             // bus net expression
    const char *ti_layer_name;  // associated layer name
    unsigned int ti_flags;      // node and terminal flags
    unsigned int ti_index;      // terminal index
    unsigned int ti_beg;        // bterm range begin
    unsigned int ti_end;        // bterm range end
    bool ti_bterm;              // true if info for bterm
    bool ti_has_phys;           // has physical terminal
};

struct hyList;
struct hyEnt;
struct sParamTab;
class cModLib;
class cSpiceIPC;

class cScedGhost
{
public:
    cScedGhost();

    // sced_arcs.cc
    void showGhostDiskPath(int, int, int, int);
    void showGhostArcPath(int, int, int, int);

    // sced_shape.cc
    void showGhostShape(int, int, int, int);

    // sced_subckt.cc
    void showGhostElecTerms(int, int, int, int);

private:
    // sced_ghost.cc
    static void ghost_diskpth(int, int, int, int, bool);
    static void ghost_arcpth(int, int, int, int, bool);
    static void ghost_shape(int, int, int, int, bool);
    static void ghost_elec_terms(int, int, int, int, bool);
};


// Store a line of spice text.  This can look like the similar struct
// in WRspice, but we don't use these features here.
//
struct SpiceLine
{
// #define SP_LINE_FULL
    SpiceLine(const char *s = 0)
        {
            li_next = 0;
            li_line = lstring::copy(s);
#ifdef SP_LINE_FULL
            li_linenum = 0;
            li_error = 0;
            li_actual = 0;
#endif
        }

    ~SpiceLine()
        {
            delete [] li_line;
#ifdef SP_LINE_FULL
            delete [] li_error;
            SpiceLine::destroy(li_actual);
#endif
        }

    static void destroy(SpiceLine *t)
        {
            while (t) {
                SpiceLine *tx = t;
                t = t->li_next;
                delete tx;
            }
        }

    SpiceLine *li_next;
    char *li_line;
#ifdef SP_LINE_FULL
    char *li_error;
    SpiceLine *li_actual;
    int li_linenum;
#endif
};


inline class cSced *SCD();

class cSced : public cScedIf
{
public:
    friend inline cSced *SCD() { return (static_cast<cSced*>(ScedIf())); }

    bool hasSced() { return (true); }

    cScedGhost *SGst() { return (sc_ghost); }

    // ebtn_menu.cc
    MenuBox *createEbtnMenu();
    bool setupCommand(MenuEnt*, bool*, bool*);                      // export

    // funcs_sced.cc
    void loadScriptFuncs();

    // sced.cc
    cSced();
    void modelLibraryOpen(const char*);                             // export
    void modelLibraryClose();                                       // export
    SpiceLine *modelText(const char*);
    bool isModel(const char*);
    void closeSpice();                                              // export
    bool simulationActive();                                        // export
    bool logConnect();                                              // export
    void setLogConnect(bool);                                       // export

    static char *sp_gettok(const char**, bool=false);

    // sced_arcs.cc
    void makeArcPathExec(CmdDesc*);

    // sced_check.cc
    void checkElectrical(CDcbin*);                                  // export
    bool prptyCheck(CDs*, FILE*, bool);
    bool prptyCheckCell(CDs*, char **);
    bool prptyCheckMutual(CDs*, CDp_nmut*, char**);
    bool prptyCheckLabel(CDs*, CDla*, char**);
    bool prptyCheckInst(CDs*, CDc*, char**);
    bool prptyRegenCell();

    // sced_connect.cc
    bool connectAll(bool, CDs* = 0);                                // export
    void unconnectAll();
    bool connect(CDs*);                                             // export
    void updateHlabels(CDs*);
    void updateNames(CDs*);                                         // export
    void renumberInstances(CDs*);

    // sced_dev.cc
    bool saveAsDev(const char*, bool);

    // sced_dots.cc
    void recomputeDots();                                           // export
    void updateDots(CDs*, CDo*);                                    // export
    void dotsSetDirty(const CDs*);                                  // export
    void dotsUpdateDirty();                                         // export
    void clearDots();                                               // export
    void updateDotsCellName(CDcellName, CDcellName);                // export

    // sced_expr.cc
    double *evalExpr(const char**);
    char *findPlotExpressions(const char*);

    // sced_fixup.cc
    void addParentConnection(CDs*, int, int);                       // export
    void addParentConnections(CDs*);
    int addConnection(CDs*, int, int, CDw* = 0);
    bool checkAddConnection(CDs*, int, int, bool);
    void fixPaths(CDs*);
    void install(CDo*, CDs*, bool);                                 // export
    void uninstall(CDo*, CDs*);                                     // export
    void fixVertices(CDo*, CDs*);
    int addConnection(CDo*, int, int, bool);

    // sced_mutual.cc
    void showMutualExec(CmdDesc*);
    bool setMutParam(const char*, int, const char*);
    char *getMutParam(const char*, int);
    bool setMutLabel(CDs*, CDp_nmut*, CDp_nmut*);                   // export
    void mutToNewMut(CDs*);                                         // export
    void mutParseName(const char*, char*, char*);                   // export

    // sced_netlist.cc
    bool isCheckingSolitary();
    CDpl *getElecNodeProps(CDs*, int);
    stringlist *getElecNodeContactNames(CDs*, int);
    stringlist **getElecContactNames(CDs*, int*);

    // sced_nodemap.cc
    void setModified(cNodeMap*);                                    // export
    void destroyNodes(CDs*);                                        // export
    int findNode(const CDs*, const char*);
    const char *nodeName(const CDs*, int, bool* = 0);
    void updateNodes(const CDs*);                                   // export
    void registerGlobalNetName(const char*);                        // export
    bool isGlobalNetName(const char*);
    SymTab *tabGlobals(CDs*);

    // sced_plot.cc
    void showOutputExec(CmdDesc*);
    void setDoIplotExec(CmdDesc*);
    int  deletePlotRef(hyEnt*);
    void setPlotMarkColors();                                       // export
    void clearPlots();                                              // export
    char *getPlotCmd(bool);                                         // export
    void setPlotCmd(const char*);                                   // export
    char *getIplotCmd(bool);                                        // export
    void setIplotCmd(const char*);                                  // export
    hyList *getPlotList();                                          // export
    hyList *getIplotList();                                         // export

    // sced_prplabel.cc
    void genDeviceLabels(CDc*, CDc*, bool);                         // export
    void updateNameLabel(CDc*, CDp_cname*);                         // export
    bool updateLabelText(CDla*, CDs*, hyList*, BBox*);              // export
    CDla *changeLabel(CDla*, CDs*, hyList*);                        // export
    void addDeviceLabel(CDc*, CDp*, CDp*, hyList*, bool, bool);
    int checkRepositionLabels(const CDc*);                          // export
    void labelPlacement(int, CDc*, Label*);

    // sced_prpty.cc
    bool setDevicePrpty(const char*, const char*);                  // export
    char *getDevicePrpty(const char*);                              // export
    CDp *prptyModify(CDc*, CDp*, int, const char*, hyList*);        // export
    CDl *defaultLayer(CDp*);                                        // export

    // sced_shape.cc
    const char *const *shapesList();                                // export
    void addShape(int);                                             // export

    // sced_spicein.cc
    void dumpDevKeys();
#define EFS_ALLDEVS 0x1
#define EFS_CREATE  0x2
#define EFS_CLEAR   0x4
#define EFS_WIRECAP 0x8
#define EFS_MASK    0xf
    void extractFromSpice(CDs*, FILE*, int);

    // sced_spiceout.cc
    char *getAnalysis(bool);                                        // export
    void setAnalysis(const char*);                                  // export
    hyList *getAnalysisList();                                      // export
    hyList *setAnalysisList(hyList*);
    bool dumpSpiceFile(const char*);
    void dumpSpiceDeck(FILE*);                                      // export
    SpiceLine *makeSpiceDeck(CDs*, SymTab** = 0);                   // export
    stringlist *makeSpiceListing(CDs*);

    // sced_subckt.cc
    void showTermsExec(CmdDesc*);
    void symbolicExec(CmdDesc*);
    bool setCurSymbolic(bool);
    bool setCurSymbolicFast(bool);
    void assertSymbolic(bool);                                      // export
    bool makeSymbolic();
    void subcircuitExec(CmdDesc*);
    void subcircuitShowConnectPts(bool);
    bool subcircuitEditTerm(int*, bool*);
    bool subcircuitSetEditTerm(int, bool);
    bool subcircuitDeleteTerm();
    bool subcircuitBits(bool);
    bool subcircuitBitsVisible(int);
    bool subcircuitBitsInvisible(int);

    // graphics system
    void PopUpSpiceIf(GRobject, ShowMode);                          // export
    void PopUpDevs(GRobject, ShowMode);                             // export
    void PopUpDots(GRobject, ShowMode);                             // export
    void DevsEscCallback();                                         // export
    void PopUpDevEdit(GRobject, ShowMode);                          // export
    bool PopUpNodeMap(GRobject, ShowMode, int = -1);                // export
    void PopUpSim(SpType);                                          // export
    void PopUpTermEdit(GRobject, ShowMode, TermEditInfo*,
        void(*)(TermEditInfo*, CDp*), CDp*, int, int);

    cSpiceIPC *spif()                   { return (sc_spice_interface); }
    cModLib *modlib()                   { return (sc_model_library); }

    void setShowDots(DotsType d)        { sc_show_dots = d; }
    DotsType showingDots()              { return (sc_show_dots); }  // export

    // all export
    void setDoingPlot(bool b)           { sc_doing_plot = b; }
    bool doingPlot()                    { return (sc_doing_plot); }
    void setDoingIplot(bool b)          { sc_doing_iplot = b; }
    bool doingIplot()                   { return (sc_doing_iplot); }
    void setIplotStatusChanged(bool b)  { sc_iplot_status_changed = b; }
    bool iplotStatusChanged()           { return (sc_iplot_status_changed); }

    void setIncludeNoPhys(bool b)       { sc_include_nophys = b; }
    bool includeNoPhys()                { return (sc_include_nophys); }
    void setShowDevs(bool b)            { sc_show_devs = b; }
    bool showDevs()                     { return (sc_show_devs); }

private:
    // sced_setif.cc
    void setupInterface();

    // sced_txtcmds.cc
    void setupBangCmds();

    // sced_variables.cc
    void setupVariables();

    cModLib *sc_model_library;      // Device model library container.
    cSpiceIPC *sc_spice_interface;  // Spice communications.

    SymTab *sc_global_tab;          // Global node names.

    hyList *sc_analysis_cmd;        // Spice analysis command.
    hyList *sc_plot_hpr_list;       // Selected trace hypertext for plot;
    hyList *sc_iplot_hpr_list;      // Selected trace hypertext for iplot;

    DotsType sc_show_dots;          // Electrical connection indication.
    bool sc_doing_plot;             // In plot mode.
    bool sc_doing_iplot;            // Doing interactive plot.
    bool sc_iplot_status_changed;   // Iplot status changed.
    bool sc_show_devs;              // Device menu is visible.
    bool sc_include_nophys;         // connect() will include nophys devs.

    cScedGhost *sc_ghost;           // Ghost drawing
};

#endif

