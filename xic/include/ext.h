
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

#ifndef EXT_H
#define EXT_H

//
// Main include for the extraction package.
//

#include "extif.h"

// Variables

// Extraction General
#define VA_ExtractOpaque        "ExtractOpaque"
#define VA_FlattenPrefix        "FlattenPrefix"
#define VA_GlobalExclude        "GlobalExclude"
#define VA_KeepShortedDevs      "KeepShortedDevs"
#define VA_MaxAssocLoops        "MaxAssocLoops"
#define VA_MaxAssocIters        "MaxAssocIters"
#define VA_NoMeasure            "NoMeasure"
#define VA_UseMeasurePrpty      "UseMeasurePrpty"
#define VA_NoReadMeasurePrpty   "NoMReadeasurePrpty"
#define VA_NoMergeParallel      "NoMergeParallel"
#define VA_NoMergeSeries        "NoMergeSeries"
#define VA_NoMergeShorted       "NoMergeShorted"
#define VA_IgnoreNetLabels      "IgnoreNetLabels"
#define VA_UpdateNetLabels      "UpdateNetLabels"
#define VA_FindOldTermLabels    "FindOldTermLabels"
#define VA_MergeMatchingNamed   "MergeMatchingNamed"
#define VA_MergePhysContacts    "MergePhysContacts"
#define VA_NoPermute            "NoPermute"
#define VA_PinLayer             "PinLayer"
#define VA_PinPurpose           "PinPurpose"
#define VA_RLSolverDelta        "RLSolverDelta"
#define VA_RLSolverTryTile      "RLSolverTryTile"
#define VA_RLSolverGridPoints   "RLSolverGridPoints"
#define VA_RLSolverMaxPoints    "RLSolverMaxPoints"
#define VA_SubcPermutationFix   "SubcPermutationFix"
#define VA_VerbosePromptline    "VerbosePromptline"
#define VA_ViaCheckBtwnSubs     "ViaCheckBtwnSubs"
#define VA_ViaSearchDepth       "ViaSearchDepth"
#define VA_ViaConvex            "ViaConvex"

// Undocumented (for now)
#define VA_NetCellThreshold     "NetCellThreshold"

// Extract Menu Commands (see sced.h)
#define VA_QpathGroundPlane     "QpathGroundPlane"
#define VA_QpathUseConductor    "QpathUseConductor"
#define VA_EnetNet              "EnetNet"
#define VA_EnetSpice            "EnetSpice"
#define VA_EnetBottomUp         "EnetBottomUp"
#define VA_PnetNet              "PnetNet"
#define VA_PnetDevs             "PnetDevs"
#define VA_PnetSpice            "PnetSpice"
#define VA_PnetBottomUp         "PnetBottomUp"
#define VA_PnetShowGeometry     "PnetShowGeometry"
#define VA_PnetIncludeWireCap   "PnetIncludeWireCap"
#define VA_PnetListAll          "PnetListAll"
#define VA_PnetNoLabels         "PnetNoLabels"
#define VA_PnetVerbose          "PnetVerbose"
#define VA_SourceAllDevs        "SourceAllDevs"
#define VA_SourceCreate         "SourceCreate"
#define VA_SourceClear          "SourceClear"
// #define VA_SourceTermDevName "SourceTermDevName"
// #define VA_SourceGndDevName  "SourceGndDevName"
#define VA_NoExsetAllDevs       "NoExsetAllDevs"
#define VA_NoExsetCreate        "NoExsetCreate"
#define VA_ExsetClear           "ExsetClear"
#define VA_ExsetIncludeWireCap  "ExsetIncludeWireCap"
#define VA_ExsetNoLabels        "ExsetNoLabels"
#define VA_LvsFailNoConnect     "LvsFailNoConnect"
#define VA_PathFileVias         "PathFileVias"

// Device name prefix for extracted wire net capacitors.
#define WIRECAP_PREFIX "C@NET"

// Place holder for empty token.
#define EXT_NONE_TOK "<none>"

// Default purpose name for LPPs of net labels.  To be active, a net
// name label must 1) have the reference coordinate touching a BPW on
// layer:drawing, and 2) the label must be on layer:$PIN.
#define EXT_DEF_PIN_PURPOSE "pin"

// We have a reserved LPP used for device data.  It contains boxes
// with BBs identical to device body BBs.  The boxes may contain
// properties that apply to the device.  This is all maintained
// internally and is not meant to be directly modified by users.
#define EXT_DEV_LPP "device:xicdata"

// Names of generic device recognition templates.
#define EXT_PMOS_TEMPLATE "PmosTemplate"
#define EXT_NMOS_TEMPLATE "NmosTemplate"
#define EXT_RES_TEMPLATE "ResTemplate"

// When looking for connections between metal layers in a cell, look
// this deep into the hierarchy to find via material.  The value 0
// indicates to look in the containing cell only.
#define EXT_DEF_VIA_SEARCH_DEPTH 0

// Various SPICE output modes from physical data:
//   normal : use the physical name for devices and subckts
//   mixed  : use the electrical (dual) name if it exists, else physical
//   duals  : print only devices with duals, and use electrical name
//
enum PhysSpicePrintMode { PSPM_physical, PSPM_mixed, PSPM_duals };

// QPATH command ground plane use.
enum { QPifavail, QPcreate, QPnone };
typedef unsigned char QPtype;

enum LVSresult { LVSerror = -1, LVSfail = 0, LVStopok, LVSap, LVSclean }; 
// LVSerror:    Error occurred, LVS couldn't run.
// LVSfail:     Differences in circuit topology.
// LVStopok:    Topology matches, parameter differences found.
// LVSap:       Topology and measured params match, ambiguities found.
// LVSclean:    Topology and component values match, no ambiguity.

class cParamCx;
class cGroupDesc;
struct pathfinder;
struct MenuBox;
struct sParamTab;
struct te_info_t;
struct sDevList;
struct sDevInst;
struct sDevInstList;
struct sDevDesc;
struct sSubcDesc;
struct sGdList;
struct sEinstList;


//
// Structs passed to PopUpExtCmd()
//

typedef const void *opt_atom;

struct sExtCmdBtn
{
    sExtCmdBtn(opt_atom n, const char *v, int c, int r, int *s, bool a)
        {
            cb_atom = n;
            cb_var = v;
            cb_col = c;
            cb_row = r;
#define EXT_BSENS 8
            for (int i = 0; i < EXT_BSENS; i++)
                cb_sens[i] = s ? s[i] : 0;
            cb_active = a;
            cb_set = false;
        }

    sExtCmdBtn() { }

    const char *name()      const { return ((const char*)cb_atom); }
    opt_atom atom()         const { return (cb_atom); }
    const char *var()       const { return (cb_var); }
    int col()               const { return (cb_col); }
    int row()               const { return (cb_row); }
    const unsigned char *sens() const { return (cb_sens); }
    bool is_active()        const { return (cb_active); }
    void set_active(bool b)       { cb_active = b; }
    bool is_set()           const { return (cb_set); }
    void set(bool b)              { cb_set = b; }

private:
    opt_atom cb_atom;       // button text
    const char *cb_var;     // equivalent variable name
    unsigned char cb_col;   // positioning
    unsigned char cb_row;
    unsigned char cb_sens[EXT_BSENS];
                            // for each nonzero element, sensitivity follows
                            // OR of (n-1),
    bool cb_active;         // initially active
    bool cb_set;            // currently set
};

// button text - used for comparison
extern opt_atom opt_atom_all;
extern opt_atom opt_atom_btmup;
extern opt_atom opt_atom_net;
extern opt_atom opt_atom_devs;
extern opt_atom opt_atom_spice;
extern opt_atom opt_atom_geom;
extern opt_atom opt_atom_cap;
extern opt_atom opt_atom_labels;
extern opt_atom opt_atom_verbose;

enum ExtCmdType { ExtDumpPhys, ExtDumpElec, ExtLVS, ExtSource, ExtSet };

struct sExtCmd
{
    sExtCmd(ExtCmdType t, const char *m, const char *f, const char *g,
            const char *w, const char *b, bool hd, int nb, sExtCmdBtn *bt)
        {
            ec_type = t;
            ec_message = m;
            ec_filename = f;
            ec_gotext = g;
            ec_wintitle = w;
            ec_btntitle = b;
            ec_has_depth = hd;
            ec_numbtns = nb;
            ec_btns = bt;
        }

    ExtCmdType type()           const { return (ec_type); }
    const char *message()       const { return (ec_message); }
    const char *filename()      const { return (ec_filename); }
    void set_filename(const char *f)  { ec_filename = f; }
    const char *gotext()        const { return (ec_gotext); }
    const char *wintitle()      const { return (ec_wintitle); }
    const char *btntitle()      const { return (ec_btntitle); }
    bool has_depth()            const { return (ec_has_depth); }
    int num_buttons()           const { return (ec_numbtns); }
    sExtCmdBtn *button(int i)
        {
            if (i >= 0 && i < ec_numbtns)
                return (&ec_btns[i]);
            return (0);
        }

    sExtCmdBtn *btns()          const { return (ec_btns); }
    void set_btns(sExtCmdBtn *b, int nb)
        {
            ec_btns = b;
            ec_numbtns = nb;
        }

private:
    ExtCmdType ec_type;         // command code
    const char *ec_message;     // framed text
    const char *ec_filename;    // initial file name
    const char *ec_gotext;      // activate button label
    const char *ec_wintitle;    // window title
    const char *ec_btntitle;    // button frame label
    bool ec_has_depth;          // use depth menu
    unsigned char ec_numbtns;   // number of buttons
    sExtCmdBtn *ec_btns;        // button array
};

// Passed to cExt::DumpCell()
struct sDumpOpts
{
    sDumpOpts(sExtCmdBtn *b, int nb, int ub)
        {
            do_depth = 0;
            do_numbtns = nb;
            do_userbtn = ub;
            do_btns = b;
            do_spmode = PSPM_physical;
            do_intname = false;
        }

    ~sDumpOpts()    { delete [] do_btns; }

    bool isset(opt_atom name) const
        {
            for (int i = 0; i < do_numbtns; i++) {
                if (name == do_btns[i].atom())
                    return (do_btns[i].is_set());
            }
            return (false);
        }

    int depth()                             const { return (do_depth); }
    void set_depth(int d)                         { do_depth = d; }
    int num_buttons()                       const { return (do_numbtns); }
    int user_button()                       const { return (do_userbtn); }

    sExtCmdBtn *button(int i)
        {
            if (i >= 0 && i < do_numbtns)
                return (&do_btns[i]);
            return (0);
        }

    PhysSpicePrintMode spice_print_mode()   const { return (do_spmode); }
    void set_spice_print_mode(PhysSpicePrintMode m) { do_spmode = m; }
    bool spice_intern_instname()            const { return (do_intname); }
    
private:
    int do_depth;                  // Hierarchy depth to process
    short int do_numbtns;          // Number of buttons
    short int do_userbtn;          // Start of "user" buttons
    sExtCmdBtn *do_btns;           // Button array
    PhysSpicePrintMode do_spmode;  // Use dual names in SPICE output?
    bool do_intname;               // In SPICE output, use only internal
                                   //  instance names, otherwise use the name
                                   //  supplied in the XICP_NAME property if
                                   //  one exists for the instance
};


class cExtGhost
{
public:
    cExtGhost();

    void showGhostPhysTerms(int, int, int, int);
    void showGhostMeasure(int, int, int, int, bool);

private:
    static void ghost_phys_terms(int, int, int, int, bool);
    static void ghost_meas_box(int, int, int, int, bool);
};

// Flag for cExt::makeElec, must be outside of EFS_MASK from sced.h.
#define MEL_NOLABELS    0x10

inline class cExt *EX();

class cExt : public cExtIf
{
public:
    friend inline cExt *EX() { return (static_cast<cExt*>(ExtIf())); }

    // pathFinder argument
    enum PFenum { PFget, PFinitQuick, PFinit, PFclear };

    bool hasExtract() { return (true); }
    cExtGhost *XGst() { return (ext_ghost); }

    // ext.cc
    cExt();
    CDs *cellDesc(cGroupDesc*);                                     // export
    void windowShowHighlighting(WindowDesc*);                       // export
    void windowShowBlinking(WindowDesc*);                           // export
    void layerChangeCallback();                                     // export
    void preCurCellChangeCallback();                                // export
    void postCurCellChangeCallback();                               // export
    void deselect();                                                // export
    void postModeSwitchCallback();                                  // export
    siVariable *createFormatVars();                                 // export
    bool rlsolverMsgs();                                            // export
    void setRLsolverMsgs(bool);                                     // export
    bool logRLsolver();                                             // export
    void setLogRLsolver(bool);                                      // export
    bool logGrouping();                                             // export
    void setLogGrouping(bool);                                      // export
    bool logExtracting();                                           // export
    void setLogExtracting(bool);                                    // export
    bool logAssociating();                                          // export
    void setLogAssociating(bool);                                   // export
    bool logVerbose();                                              // export
    void setLogVerbose(bool);                                       // export

    // ext_device.cc
    void addSubcircuit(sSubcDesc*);
    sSubcDesc *findSubcircuit(const CDs*);
    char *addDevice(sDevDesc*, sDevDesc** = 0);
    sDevDesc *removeDevice(const char*, const char*);
    sDevDesc *findDevice(const char*, const char*);
    sDevDesc *findDevices(const char*);
    stringlist *listDevices();
    void initDevs();                                                // export
    XIrt getDevlist(CDcbin*, sDevList**);
    bool saveMeasuresInProp();

    // ext_devsel.cc
    bool selectDevices(GRobject);
    void clearDeviceSelection();
    void queueDevice(sDevInst*);
    void queueDevices(sDevInstList*);
    void showSelectedDevices(WindowDesc*);
    void paintMeasureBox();
    bool measureLayerElectrical(GRobject);
    void showElectrical(int, int);
    void showMeasureBox(WindowDesc*, bool);

    // ext_duality.cc
    bool associate(CDs*);                                           // export
    int groupOfNode(CDs*, int);                                     // export
    int nodeOfGroup(CDs*, int);                                     // export
    void clearFormalTerms(CDs*);                                    // export

    // ext_dump.cc
    void dumpPhysNetExec(CmdDesc*);
    sDumpOpts *pnetNetOpts(const char*, const char*);
    void dumpElecNetExec(CmdDesc*);
    sDumpOpts *enetNewOpts(const char*, const char*);
    void lvsExec(CmdDesc*);

    // ext_extract.cc
    bool extract(CDs*);
    void showExtract(WindowDesc*, bool);
    bool shouldFlatten(const CDc*, CDs*);
    void updateReferenceTable(const CDs*);
    sGdList *referenceList(const CDs*);
    bool skipExtract(CDs*);                                          // export

    // ext_gnsel.cc
    void selectGroupNode(GRobject);
    bool selectShowNode(int node);                                   // export
    void selectShowPath(bool);
    void selectRedrawPath();
    int netSelB1Up();                                                // export
    int netSelB1Up_altw();                                           // export

    // ext_gplane.cc
    bool setInvGroundPlaneImmutable(bool);                          // export
    XIrt setupGroundPlane(CDs*);
    void activateGroundPlane(bool);
    XIrt invertGroundPlane(CDs*, int, const char*, const char*);

    // ext_group.cc
    bool group(CDs*, int);
    void invalidateGroups(bool = false);                            // export
    void destroyGroups(CDs*);                                       // export
    void clearGroups(CDs*);                                         // export

    // ext_menu.cc
    MenuBox *createMenu();
    bool setupCommand(MenuEnt*, bool*, bool*);                      // export

    // ext_netname.cc
    void updateNetLabels();
    CDl *getPinLayer(const CDl*, bool);

    // ext_nets.cc
    void reset(CDcbin*, bool, bool, bool);
    bool makeElec(CDs*, int, int);
    CDpin *pointAtPins(BBox*);
    sEinstList *pointAtLabels(BBox*);
    void arrangeTerms(CDcbin*, bool);                               // export
    void arrangeInstLabels(CDcbin*);
    void placePhysSubcTerminals(const CDc*, int, const CDc*,        // export
        unsigned int, unsigned int);

    // ext_out_elec.cc
    bool dumpElecNetlist(FILE*, CDs*, sDumpOpts*);

    // ext_out_lvs.cc
    LVSresult lvs(FILE*, CDcbin*, int);

    // ext_out_phys.cc
    bool dumpPhysNetlist(FILE*, CDs*, sDumpOpts*);

    // ext_path.cc
    void selectPath(GRobject);
    void selectPathQuick(GRobject);
    void redrawPath();
    void showCurrentPath(WindowDesc*, bool);
    bool saveCurrentPathToFile(const char*, bool);
    bool getAntennaPath();
    pathfinder *pathFinder(PFenum);

    // ext_pathres.cc
    void editTerminals(GRobject);
    void showTerminals(WindowDesc*, bool);
    bool extractNetResistance(double**, int*, const char*);

    // ext_tech.cc
    void parseDeviceTemplates(FILE*);                               // export
    bool parseDeviceTemplate(FILE*, const char*);
    bool parseDevice(FILE*, bool);
    void addMOS(const sMOSdev*);
    void addRES(const sRESdev*);
    void techPrintDeviceTemplates(FILE*, bool);
    void techPrintDevices(FILE*);

    // ext_term.cc
    CDp_snode *findTerminal(const char*, int, const Point*, const Point*);
    CDp_snode *createTerminal(const char*, const Point*, const char*);
    CDsterm *findPhysTerminal(const char*, const Point*);
    bool createPhysTerminal(CDs*, const char*, const Point*, const char*);
    bool destroyTerminal(CDp_snode*);
    bool destroyPhysTerminal(CDsterm*);
    bool setTerminalName(CDsterm*, const char*);
    bool setTerminalType(CDp_snode*, const char*);
    bool setTerminalLayer(CDsterm*, const char*);
    bool setElecTerminalLoc(CDp_snode*, const Point*);
    bool clearElecTerminalLoc(CDp_snode*, const Point*);
    bool setPhysTerminalLoc(CDsterm*, const Point*);
    void showPhysTermsExec(GRobject, bool);
    void editTermsExec(GRobject, GRobject);
    void editTermsPush(CDsterm*);

    // ext_view.cc
    int showCell(WindowDesc*, const CDs*, const CDl*);
    void showGroups(bool);
    void showExtractionView(GRobject);
    CDol *selectItems(const char*, const BBox*, int, int, int);     // export

    // funcs_extract.cc
    void loadScriptFuncs();

    // graphical
    void PopUpExtCmd(GRobject, ShowMode, sExtCmd*,
        bool(*)(const char*, void*, bool, const char*, int, int),
        void*, int = 0);
    void PopUpExtSetup(GRobject, ShowMode);                         // export
    void PopUpSelections(GRobject, ShowMode);
    void PopUpDevices(GRobject, ShowMode);
    void PopUpPhysTermEdit(GRobject, ShowMode, te_info_t*,
        void(*)(te_info_t*, CDsterm*), CDsterm*, int, int);

    // ext_viatest.h
    static XIrt isConnection(CDs*, const sVia*, const CDo*, const CDo*,
        const CDo*, bool*);

    //
    // access to private members
    //

    stringlist *findDeviceTemplate(const char *nm)
        {
            if (!ext_devtmpl_tab || !nm)
                return (0);
            stringlist *sl = (stringlist*)SymTab::get(ext_devtmpl_tab, nm);
            if (sl == (stringlist*)ST_NIL)
                return (0);
            return (sl);
        }

    sDevDesc *findDeviceDesc(const char *nm)
        {
            if (!ext_device_tab || !nm)
                return (0);
            sDevDesc *dd = (sDevDesc*)SymTab::get(ext_device_tab, nm);
            if (dd == (sDevDesc*)ST_NIL)
                return (0);
            return (dd);
        }

    CDl *groundPlaneLayer()             { return (ext_gp_layer); }
    CDl *groundPlaneLayerInv()          { return (ext_gp_layer_inv); }

    CDl *pinLayer()                     { return (ext_pin_layer); }
    void setPinLayer(CDl *ld)           { ext_pin_layer = ld; }

    stringlist *techDevTemplates()      { return (ext_tech_devtmpls); }
    void setTechDevTemplates(stringlist *s) { ext_tech_devtmpls = s; }

    sDevDesc *deadDevices()             { return (ext_dead_devices); }
    void setDeadDevices(sDevDesc *d)    { ext_dead_devices = d; }

    const char *flattenPrefix()         { return (ext_flatten_prefix); }
    void setFlattenPrefix(char *s)      { ext_flatten_prefix = s; }

    cParamCx *paramCx()                 { return (ext_param_cx); }
    void setParamCx(cParamCx *pcx)      { ext_param_cx = pcx; }

    const sLspec *globalExclude()       { return (&ext_global_excl); }

    bool isBlinkSelections()            { return (ext_blink_sels); }
    void setBlinkSelections(bool b)     { ext_blink_sels = b; }
    bool isSubpathEnabled()             { return (ext_subpath_enabled); }
    void setSubpathEnabled(bool b)      { ext_subpath_enabled = b; }
    bool isGNShowPath()                 { return (ext_gn_show_path); }
    void setGNShowPath(bool b)          { ext_gn_show_path = b; }

    bool isExtractionView()             { return (ext_extraction_view); }
                                                                     // export
    bool isExtractionSelect()           { return (ext_extraction_select); }
                                                                     // export
    int pathDepth()                     { return (ext_path_depth); }
    void setPathDepth(int d)            { ext_path_depth = d; }
    int viaSearchDepth()                { return (ext_via_search_depth); }
    void setViaSearchDepth(int d)       { ext_via_search_depth = d; }

    bool isShowingGroups()              { return (ext_showing_groups); }
    void setShowingGroups(bool b)       { ext_showing_groups = b; }
    bool isShowingNodes()               { return (ext_showing_nodes); }
    void setShowingNodes(bool b)        { ext_showing_nodes = b; }
    bool isShowingDevs()                { return (ext_showing_devs); }
    void setShowingDevs(bool b)         { ext_showing_devs = b; }
    bool isNoMergeParallel()            { return (ext_no_merge_parallel); }
    void setNoMergeParallel(bool b)     { ext_no_merge_parallel = b; }
    bool isNoMergeSeries()              { return (ext_no_merge_series); }
    void setNoMergeSeries(bool b)       { ext_no_merge_series = b; }
    bool isNoMergeShorted()             { return (ext_no_merge_shorted); }
    void setNoMergeShorted(bool b)      { ext_no_merge_shorted = b; }
    bool isExtractOpaque()              { return (ext_extract_opaque); }
    void setExtractOpaque(bool b)       { ext_extract_opaque = b; }
    bool isKeepShortedDevs()            { return (ext_keep_shorted_devs); }
    void setKeepShortedDevs(bool b)     { ext_keep_shorted_devs = b; }
    bool isDevselCompute()              { return (ext_devsel_compute); }
    void setDevselCompute(bool b)       { ext_devsel_compute = b; }
    bool isDevselCompare()              { return (ext_devsel_compare); }
    void setDevselCompare(bool b)       { ext_devsel_compare = b; }
    bool isVerbosePromptline()          { return (ext_verbose_prompt); }
    void setVerbosePromptline(bool b)   { ext_verbose_prompt = b; }
    bool isQuickPathUseConductor()      { return (ext_qp_use_conductor); }
    void setQuickPathUseConductor(bool b) { ext_qp_use_conductor = b; }
    bool isIgnoreNetLabels()            { return (ext_ign_net_labels); }
    void setIgnoreNetLabels(bool b)     { ext_ign_net_labels = b; }
    bool isUpdateNetLabels()            { return (ext_upd_net_labels); }
    void setUpdateNetLabels(bool b)     { ext_upd_net_labels = b; }
    bool isFindOldTermLabels()          { return (ext_find_old_term_labels); }
    void setFindOldTermLabels(bool b)   { ext_find_old_term_labels = b; }
    bool isMergeMatchingNamed()         { return (ext_merge_named); }
    void setMergeMatchingNamed(bool b)  { ext_merge_named = b; }
    bool isNoPermute()                  { return (ext_no_permute); }
    void setNoPermute(bool b)           { ext_no_permute = b; }
    bool isNoMeasure()                  { return (ext_no_measure); }
    void setNoMeasure(bool b)           { ext_no_measure = b; }
    bool isUseMeasurePrpty()            { return (ext_use_meas_prop); }
    void setUseMeasurePrpty(bool b)     { ext_use_meas_prop = b; }
    bool isNoReadMeasurePrpty()         { return (ext_no_read_meas_prop); }
    void setNoReadMeasurePrpty(bool b)  { ext_no_read_meas_prop = b; }
    bool isIgnoreGroupNames()           { return (ext_ignore_group_names); }
    void setIgnoreGroupNames(bool b)    { ext_ignore_group_names = b; }
    bool isMergePhysContacts()          { return (ext_merge_phys_conts); }
    void setMergePhysContacts(bool b)   { ext_merge_phys_conts = b; }
    bool isSubcPermutationFix()         { return (ext_subc_permute_fix); }
    void setSubcPermutationFix(bool b)  { ext_subc_permute_fix = b; }
    bool isViaCheckBtwnSubs()           { return (ext_via_check_btwn_subs); }
    void setViaCheckBtwnSubs(bool b)    { ext_via_check_btwn_subs = b; }

    QPtype quickPathMode()              { return (ext_qp_mode); }
    void setQuickPathMode(QPtype t)     { ext_qp_mode = t; }
    Blist *terminals()                  { return (ext_terminals); }
    void setTerminals(Blist *bl)        { ext_terminals = bl; }

    static bool isViaConvex()           { return (ext_via_convex); }
    static void setViaConvex(bool b)    { ext_via_convex = b; }

private:
    // ext_group.cc
    XIrt group_rec(CDs*, int, SymTab*);

    // ext_nets.cc
    void reset_all_terms(CDs*);
    CDpin *find_pins_in_area(CDs*, const BBox*);
    CDpin *find_pins_ungrouped(CDs*);
    CDcont *find_conts_ungrouped(CDs*);
    sEinstList *find_scname_labels_in_area(CDs*, const BBox*);
    sEinstList *find_scname_labels_unplaced(CDs*);

    // ext_out_elec.cc
    bool dump_elec_recurse(FILE*, CDs*, int, sDumpOpts*, SymTab*, SymTab*);
    bool dump_elec(FILE*, CDs*, int, sDumpOpts*, SymTab*, SymTab*);
    bool dump_elec_formatted(FILE*, CDs*, const char*);

    // ext_out_lvs.cc
    LVSresult lvs_recurse(FILE*, CDcbin*, int, SymTab*);

    // ext_out_phys.cc
    bool dump_phys_recurse(FILE*, CDs*, CDs*, int, sDumpOpts*, SymTab*);

    // ext_techif.cc
    void setupTech();

    // ext_txtcmds.cc
    void setupBangCmds();

    // ext_variables.cc
    void setupVariables();

    sDevInstList *ext_selected_devices; // List of highlighted devices
    SymTab *ext_devtmpl_tab;        // DeviceTemplate table, access by name,
                                    //  get stringlist of template.
    SymTab *ext_device_tab;         // Device table, access by name, get
                                    //  list of sDevDesc for prefixes.
    SymTab *ext_subckt_tab;         // Subcircuit table, access by unsigned
                                    //  long CDs, get sSubcDesc.
    SymTab *ext_reference_tab;      // Reference table, access by unsigned
                                    //  long CDs, get sGdList of callers.
    sDevDesc *ext_dead_devices;     // Temp queue for deletion.
    char *ext_flatten_prefix;       // Cell name prefix to flatten.
    CDl *ext_gp_layer;              // Ground plane layer.
    CDl *ext_gp_layer_inv;          // Ground plane layer inverse.
    cParamCx *ext_param_cx;         // Parameter context.
    CDl *ext_pin_layer;             // Common pin layer if assigned.
    stringlist *ext_tech_devtmpls;  // Device templates read from tech file.

    sLspec ext_global_excl;         // Global exclude layer expression.

    bool ext_gp_inv_set;            // Inverted ground plane set up.
    bool ext_blink_sels;            // True if blinking mode in paths/qpath.
    bool ext_subpath_enabled;       // True to enable sub-path selection.
    bool ext_gn_show_path;          // True to show phys path in g/n select.
    bool ext_extraction_view;       // True when showing extraction view.
    bool ext_extraction_select;     // Set true when selecting extraction objs.
    bool ext_showing_groups;        // True if group numbers shown.
    bool ext_showing_nodes;         // True if node numbers rather than groups.
    bool ext_showing_devs;          // True if selected devices highlighted.
    bool ext_no_merge_parallel;     // Suppress merging parallel devices.
    bool ext_no_merge_series;       // Suppress merging series devices.
    bool ext_no_merge_shorted;      // No merging devs with all terms shorted.
    bool ext_extract_opaque;        // Extract/associate opaque cells.
    bool ext_keep_shorted_devs;     // Don't throw out shorted devices.
    bool ext_devsel_compute;        // Compute mode for device selections.
    bool ext_devsel_compare;        // Compare mode for device selections.
    bool ext_verbose_prompt;        // Show lots of text during grp/ext/assoc.
    bool ext_qp_use_conductor;      // Quick Paths uses CONDUCTOR layers
                                    //  instead of ROUTING.
    bool ext_ign_net_labels;        // Ignore physical net labels.
    bool ext_upd_net_labels;        // Create/update physical net labels.
    bool ext_find_old_term_labels;  // Hunt for old-style net labels.
    bool ext_merge_named;           // Merge groups with the same net name.
    bool ext_no_permute;            // Skip device permutations.
    bool ext_no_measure;            // Skip device measurements.
    bool ext_use_meas_prop;         // Disable measure results cache prpty.
    bool ext_no_read_meas_prop;     // Don't read measure results cache prpty.
    bool ext_ignore_group_names;    // Print group number in phys netlist.
    bool ext_merge_phys_conts;      // Do contact merging split-nets fix.
    bool ext_subc_permute_fix;      // Enable subcircuit permutation fix.
    bool ext_via_check_btwn_subs;   // Extra test for connecting subcells.

    QPtype ext_qp_mode;             // Qpath ground plane use.
    int ext_path_depth;             // Path/Qpath search depth.
    int ext_via_search_depth;       // Extraction via layer search depth.
    cExtGhost *ext_ghost;           // Ghost drawing.
    pathfinder *ext_pathfinder;     // Current path (net).
    Blist *ext_terminals;           // Transient terminals for R extraction.

    static bool ext_via_convex;     // Called from static via test export.
};

// rlsolver.cc
//
extern void sline(double*, double*);

#endif

