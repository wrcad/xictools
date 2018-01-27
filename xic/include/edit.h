
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

#ifndef EDIT_H
#define EDIT_H

#include "editif.h"
#include "edit_variables.h"


// Default maximum size of master name menu in the Cell Placement panel.
#define DEF_PLACE_MENU_LEN  25 

// Default maximum undo list length.
#define DEF_MAX_UNDO_LEN    25

// Defaults for Logo command vector font:  extended square ends,
// medium width.
//
#define DEF_LOGO_END_STYLE  2
#define DEF_LOGO_PATH_WIDTH 3

class cGripDb;
struct sGrip;
struct PCellParam;
struct CmdState;
struct ParseNode;
struct MenuBox;
struct sObj;
struct yb;
struct Vtex;
namespace ginterf { struct GRvecFont; }

// The default maximum number of outlined objects that can be attached
// to the mouse pointer for ghosting.  If the number of objects exceeds
// this, some object outlines won't be shown.
#define DEF_MAX_GHOST_OBJECTS 4000

inline class cEditGhost *EGst();

class cEditGhost
{
    static cEditGhost *ptr()
        {
            if (!instancePtr) 
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cEditGhost *EGst() { return (cEditGhost::ptr()); }

    // edit_ghost.cc
    cEditGhost();

    // 45s.cc
    void showGhostPathSeg(int, int, int, int);

    // copy.cc
    void showGhostMove(int, int, int, int);
    static void ghost_move_setup(bool);

    // erase.cc
    void showGhostYankBuf(int, int, int, int);
    static void ghost_put_setup(bool);

    // grip.cc
    void showGhostGrip(int, int, int, int, bool);

    // instance.cc
    unsigned int showTransformed(CDs*, CDtf*, bool = false);
    unsigned int showTransformedLevel(CDs*, CDtf*, int, bool*);
    void showGhostInstance(int, int, int, int);

    // labels.cc
    void showGhostLabel(int, int, int, int);

    // polygns.cc
    void showGhostDisk(int, int, int, int, bool);
    void showGhostDonut(int, int, int, int, bool);
    void showGhostArc(int, int, int, int, bool);

    // rotate.cc
    void showGhostRotate(int, int, int, int, bool);
    static void ghost_rotate_setup(bool);

    // stretch.cc
    void showGhostStretch(int, int, int, int);
    static void ghost_stretch_setup(bool);

    // wires.cc
    void showGhostWireSeg(int, int, int, int);
    void showGhostWire(Wire*);

    unsigned int maxGhostObjects()  { return (eg_max_ghost_objects); }
    void setMaxGhostObjects(unsigned int i) { eg_max_ghost_objects = i; }
    int maxGhostDepth()             { return (eg_max_ghost_depth); }
    void setMaxGhostDepth(int i)    { eg_max_ghost_depth = i; }

private:
    // edit_ghost.cc
    static void ghost_path_line(int, int, int, int, bool);
    static void ghost_wire_line(int, int, int, int, bool);
    static void ghost_disk(int, int, int, int, bool);
    static void ghost_donut(int, int, int, int, bool);
    static void ghost_arc(int, int, int, int, bool);
    static void ghost_stretch(int, int, int, int, bool);
    static void ghost_rotate(int, int, int, int, bool);
    static void ghost_move(int, int, int, int, bool);
    static void ghost_place(int, int, int, int, bool);
    static void ghost_label(int, int, int, int, bool);
    static void ghost_put(int, int, int, int, bool);
    static void ghost_grip(int, int, int, int, bool);

    unsigned int eg_max_ghost_objects;
    int eg_max_ghost_depth;

    static cEditGhost *instancePtr;
};

struct sEdit45
{
    sEdit45()
        {
            int_x = int_y = 0;
            simple_45 = false;
            override_45 = false;
        }

    int  int_x, int_y;      // For 45's intermediate point.
    bool simple_45;         // 45's state.
    bool override_45;       // 45's state.
};

// PopUpProperties argument.
enum PRPmode { PRPinactive, PRPactive, PRPnochange };

// If the current layer is changed during a move/copy or similar, an
// optional layer mapping may be performed.
//
// LCHGnone     Ignore layer change.
// LCHGcur      Obects on old current layer will be created on the
//              new current layer.
// LCHGall      All new objects will be created on the the current
//              layer.
//
enum LCHGmode { LCHGnone, LCHGcur, LCHGall };

// Instance placement origin for physical mode.
enum PLref { PL_ORIGIN, PL_LL, PL_UL, PL_UR, PL_LR };

// PCell auto-abutment modes.
enum AbutMode { AbutMode0, AbutMode1, AbutMode2 };

// Arg to PopUpPCellParams.
enum pcpMode { pcpNone, pcpPlace, pcpPlaceScr, pcpOpen, pcpEdit };

#define ED_YANK_DEPTH 5
#define ED_LEXPR_STORES 8

inline class cEdit *ED();

class cEdit : public cEditIf
{
public:
    friend inline cEdit *ED() { return (static_cast<cEdit*>(EditIf())); }
    friend void cEditGhost::showGhostInstance(int, int, int, int);

    // Interface to Placement Control pop-up.
    struct sPCpopup
    {
        sPCpopup()                  { ED()->plSetPopup(this); }
        virtual ~sPCpopup()         { ED()->plSetPopup(0); }

        virtual void rebuild_menu() = 0;
        virtual void desel_placebtn() = 0;
        virtual bool smash_mode() = 0;
    };

    bool hasEdit()                  { return (true); }               // export

    sEdit45 &state45()              { return (ed_45); }
    void setState45(sEdit45 &e)     { ed_45 = e; }

    // Two functions are used by the label command to export label
    // parameters temporarily to far-flung places that need it.
    //
    void setLabelOverride(int x, int y, int w, int h, int xf)
        {
            ed_label_x = x;
            ed_label_y = y;
            ed_label_width = w;
            ed_label_height = h;
            ed_label_xform = xf;
        }

    bool labelOverride(Label *lab)
        {
            if (ed_label_width && ed_label_height) {
                lab->x = ed_label_x;
                lab->y = ed_label_y;
                lab->width = ed_label_width;
                lab->height = ed_label_height;
                lab->xform = ed_label_xform;
                return (true);
            }
            return (false);
        }

    // edit.cc
    cEdit();
    void setEditingMode(bool);                                      // export
    bool noEditing();
    void invalidateLayer(CDs*, CDl*);                               // export
    void invalidateObject(CDs*, CDo*, bool);                        // export
    void clearSaveState();                                          // export
    void popState(DisplayMode);                                     // export
    void pushState(DisplayMode);                                    // export
    void ulUndoOperation();                                         // export
    void ulRedoOperation();                                         // export
    bool ulHasRedo();                                               // export
    bool ulHasChanged();                                            // export
    bool ulCommitChanges(bool = false, bool = false);               // export
    void ulListFinalize(bool);                                      // export
    void ulListBegin(bool, bool);                                   // export
    void ulListCheck(const char*, CDs*, bool);                      // export
    op_change_t *ulFindNext(const op_change_t*);                    // export
    void ulPCevalSet(CDs*, ulPCstate*);                             // export
    void ulPCevalReset(ulPCstate*);                                 // export
    void ulPCreparamSet(ulPCstate*);                                // export
    void ulPCreparamReset(ulPCstate*);                              // export
    pp_state_t *newPPstate();                                       // export
    void *saveState();                                              // export
    void revertState(void*);                                        // export

    // 45s.cc
    void pthSetSimple(bool, bool);
    void pthSet(int, int);
    void pthGet(int*, int*);

    // abut.cc
    void checkAbutment(CDc*);
    void handleAbutment();

    // bloat.cc
    XIrt bloatQueue(int, int);
    XIrt manhattanizeQueue(int, int);

    // boxes.cc
    void makeBoxesExec(CmdDesc*);

    // break.cc
    void breakExec(CmdDesc*);
    bool doBreak(int, int, bool, BrkType);
    bool split(CDo*, CDs*, int, int, bool, int);

    // cell.cc
    void createCellExec(CmdDesc*);
    bool createCell(const char*, CDcbin*, bool, BBox*);
    bool openCell(const char*, CDcbin*, char*, cCHD* = 0);
    bool addToCell(CDo*, CDs*, int, int);
    bool copySymbol(const char*, const char*);                      // export
    bool renameSymbol(const char*, const char*);                    // export
    stringlist *renameAll(CDcbin*, const char*, const char*);

    // contexts.cc
    void pushSetup();
    void pushExec(CmdDesc*);

    // copy.cc
    void copyExec(CmdDesc*);
    void copyObjects(int, int, int, int, CDl*, CDl*, int);
    bool replicateQueue(int, int, int, int, int, CDl*, CDl*, CDs* = 0);
    bool copyQueue(int, int, int, int, CDl*, CDl*, CDs* = 0);
    bool findContact(int*, int*, int, int, bool);
    bool mutSelected(CDs*, CDp*);

    // edit_menu.cc
    MenuBox *createEditMenu();

    // erase.cc
    void deleteExec(CmdDesc*);
    void deleteQueue();
    void eraseUnder();
    void eraseExec(CmdDesc*);
    bool eraseList(CDol*, BBox*);
    bool clipList(CDol*, BBox*);
    bool eraseArea(bool, int, int, int, int);
    void putExec(CmdDesc*);
    void put(int, int, int);
    void yank(CDol*, BBox*, bool);

    // flatten.cc
    void flattenExec(CmdDesc*);
    void flattenSelected(cTfmStack*, int, bool, bool);
    bool flattenCell(cTfmStack*, CDc*);

    // funcs_xxx.cc
    void loadScriptFuncs();

    // grip.cc
    bool registerGrips(CDc*);                                       // export
    void unregisterGrips(const CDc*);                               // export
    bool checkGrips();
    bool resetGrips(bool = false);

    // instance.cc
    void placeExec(CmdDesc*);
    void placeAction();
    void addMaster(const char*, const char*, cCHD* = 0);            // export
    CDc *makeInstance(CDs*, const char*, int, int, int = 0,
        int = 0);                                                   // export
    CDc *placeInstance(const char*, int, int, int, int, int, int,
        PLref, bool, pcpMode);
    void placeDev(GRobject, const char*, bool);
    bool replaceInstance(CDc*, CDcbin*, bool, bool);                // export
    void setCurrentMaster(CDcbin*);
    bool getCurrentMaster(CDcbin*);
    void plSetParams(PLref, int, int, int, int);
    void plInitMenuLen();
    void plAddMenuEnt(const char*);
    void plChangeMenuEnt(const char*, const char*);
    void plRemoveMenuEnt(const char*);
    const char *plGetMasterName();
    void plUpdateSubMaster();

    // join.cc
    bool joinAllCmd();
    bool joinLyrCmd();
    bool joinCmd();
    XIrt joinQueue();
    bool joinWireCmd();
    bool joinWireLyrCmd();
    bool splitCmd(bool);
    XIrt splitQueue(bool);

    // labels.cc
    void makeLabelsExec(CmdDesc*);
    CDla *changeLabel(CDla*, CDs*, hyList*);
    bool execLabelScript();
    void makeLogoExec(CmdDesc*);
    void logoUpdate();
    void createLogo(const char*, int, int, int);
    void assert_logo_pixel_size();

    // layerchg.cc
    void changeLayer();

    // mainstate.cc
    void initMainState();

    // miscgeom.cc
    void createLayerCmd(const char*, int, int);
    XIrt createLayerRecurse(const char*, int, int);
    XIrt createLayer(CDs*, const char*, int, int);
    XIrt createLayer(CDs*, const BBox*, CDl*, const char*, int, int);
    XIrt createLayer_notree(CDs*, const BBox*, CDl*, const char*, int, int);
    XIrt evalDerivedLayers(CDll**, CDs*, const BBox*);
    bool parseNewLayerSpec(const char**, int*, int*);

    // modify_menu.cc
    MenuBox *createModfMenu();

    // move.cc
    void moveExec(CmdDesc*);
    void moveObjects(int, int, int, int, CDl*, CDl*);
    bool moveQueue(int, int, int, int, CDl*, CDl*);

    // pcplace.cc
    bool reparameterize(CDc*, char** = 0);
    bool openPlacement(const CDcbin*, const char*);                 // export
    bool startPlacement(const CDcbin*, const char*, pcpMode = pcpPlace);
    bool resetPlacement(const char*);
    void stopPlacement();
    bool resolvePCell(CDcbin*, const char*, bool = false);
    bool reparamSubMaster(CDs*, const char*);
    bool reparamInstance(CDs*, CDc*, const CDp*, CDc** = 0);
    bool resetInstance(CDc*, const char*, double);

    // polygns.cc
    void sidesExec(CmdDesc*);                                       // export
    void makePolygonsExec(CmdDesc*);
    void makeDisksExec(CmdDesc*);
    void makeDonutsExec(CmdDesc*);
    void makeArcsExec(CmdDesc*);
    void pathBB(Point*, int, BBox*);

    // prpcedit.cc
    void cellPrptyAdd(int);
    void cellPrptyEdit(Ptxt*);
    void cellPrptyRemove(Ptxt*);

    // prpedit.cc
    void propertiesExec(CmdDesc*);
    CmdState *prptyCmd();
    bool prptyCallback(CDo*);                                       // export
    void prptyRelist();                                             // export
    void prptySetGlobal(bool);
    void prptySetInfoMode(bool);
    void prptyUpdateList(CDo*, CDo*);
    void prptyAdd(int);
    void prptyEdit(Ptxt*);
    void prptyRemove(Ptxt*);
    bool editPhysPrpty();
    bool acceptPseudoProp(CDo*, CDs*, int, const char*);

    // prpmisc.cc
    CDp *prptyModify(CDc*, CDp*, int, const char*, hyList*);
    void assignGlobalProperties(CDcbin*);                           // export
    void assertGlobalProperties(CDcbin*);                           // export
    void stripGlobalProperties(CDs*);

    // psdmenu.cc
    const char *const *styleList();
    MenuBox *createPbtnMenu();

    // rotate.cc
    void rotationExec(CmdDesc*);
    bool rotateQueue(int, int, double, int, int, CDl*, CDl*, CDmcType);

    // stretch.cc
    void stretchExec(CmdDesc*);
    void setStretchRef(int*, int*);
    bool doStretchObjList(int, int, int, int, bool);
    bool doStretch(int, int, int*, int*);
    static void stretch_label_vertex(Point*, int, int, int, int);
    static void stretch_box_vertex(BBox*, const BBox*, int, int, int,
        int, int);

    // transfrm.cc
    void setCurTransform(int, bool, bool, double);                  // export
    bool setCurTransform(const char*);
    void incrementRotation(bool);                                   // export
    void flipY();                                                   // export
    void flipX();                                                   // export
    void saveCurTransform(int);                                     // export
    void recallCurTransform(int);                                   // export
    void clearCurTransform();                                       // export
    static bool cur_tf_cb(const char*, bool, const char*, void*);

    // vtxedit.cc
    void clearObjectList();
    void purgeObjectList(CDo*);
    bool get_wire_ref(int*, int*, int*, int*);

    // wires.cc
    void widthCallback();                                           // export
    void setWireAttribute(WsType);                                  // export
    void makeWiresExec(CmdDesc*);
    void execWireStyle();
    void execWireWidth();

    // xorbox.cc
    void makeXORboxExec(CmdDesc*);
    bool xorArea(int, int, int, int);

    // Graphics System

    // gtkedset.cc
    void PopUpEditSetup(GRobject, ShowMode);

    // gtkflatten.cc
    void PopUpFlatten(GRobject, ShowMode,
        bool(*)(const char*, bool, const char*, void*),
        void*, int = 0, bool = false);

    // gtkjoin.cc
    void PopUpJoin(GRobject, ShowMode);

    // gtklexp.cc
    void PopUpLayerExp(GRobject, ShowMode);

    // gtklogo.cc
    void PopUpLogo(GRobject, ShowMode);

    // gtkmclchg.cc
    void PopUpLayerChangeMode(ShowMode);

    // gtkmodif.cc
    PMretType PopUpModified(stringlist*, bool(*)(const char*));     // export

    // gtkpcctrl.cc
    void PopUpPCellCtrl(GRobject, ShowMode);

    // gtkpcprms.cc
    bool PopUpPCellParams(GRobject, ShowMode, PCellParam*, const char*,
        pcpMode);

    // gtkplace.cc
    void PopUpPlace(ShowMode, bool);

    // gtkprpcedit.cc
    void PopUpCellProperties(ShowMode);

    // gtkprpedit.cc
    Ptxt *PropertyResolve(int, int, CDo**);                         // export
    void PopUpProperties(CDo*, ShowMode, PRPmode);
    void PropertyPurge(CDo*, CDo*);
    Ptxt *PropertySelect(int);
    Ptxt *PropertyCycle(CDp*, bool(*)(const CDp*), bool);
    void RegisterPrptyBtnCallback(int(*)(Ptxt*));

    // gtkprpinfo.cc
    void PopUpPropertyInfo(CDo*, ShowMode);
    void PropertyInfoPurge(CDo*, CDo*);

    // gtkptext.cc
    void PopUpPolytextFont(GRobject, ShowMode);
    static PolyList *polytext(const char*, int, int, int);
    static void polytextExtent(const char*, int*, int*, int*);

    // gtkvia.cc
    void PopUpStdVia(GRobject, ShowMode, CDc* = 0);

    // gtkxform.cc
    void PopUpTransform(GRobject, ShowMode,
        bool(*)(const char*, bool, const char*, void*), void*);     // export

    // Access to private members.

    const iap_t &arrayParams()              { return (ed_array_params); }
    void setArrayParams(const iap_t &iap)   { ed_array_params = iap; }
                                                                    // export

    sObj *objectList()                      { return (ed_object_list); }
    void setObjectList(sObj *ol)            { ed_object_list = ol; }

    CDl *pressLayer()                       { return (ed_press_layer); }
    void setPressLayer(CDl *l)              { ed_press_layer = l; }

    sGrip *getCurGrip()                     { return (ed_cur_grip); }

    cGripDb *getGripDb()                    { return (ed_gripdb); }
    void setGripDb(cGripDb *gdb)            { ed_gripdb = gdb; }

    WireStyle getWireStyle()                { return (ed_wire_style); }
                                                                    // export
    void setWireStyle(WireStyle ws)         { ed_wire_style = ws; }

    LCHGmode getLchgMode()                  { return (ed_lchange_mode); }
    void setLchgMode(LCHGmode m)            { ed_lchange_mode = m; }

    CDmcType moveOrCopy()                   { return (ed_move_or_copy); }
    void setMoveOrCopy(CDmcType c)          { ed_move_or_copy = c; }

    PLref instanceRef()                     { return (ed_place_ref); }
    void setInstanceRef(PLref r)            { ed_place_ref = r; }

    int stretchBoxCode()                    { return (ed_stretch_box_code); }
    void setStretchBoxCode(int s)           { ed_stretch_box_code = s; }

    short horzJustify()                     { return (ed_h_justify); }
    void setHorzJustify(int h)              { ed_h_justify = h; }
    void incHorzJustify()       { if (++ed_h_justify > 2) ed_h_justify = 0; }
    void decHorzJustify()       { if (--ed_h_justify < 0) ed_h_justify = 2; }

    short vertJustify()                     { return (ed_v_justify); }
    void setVertJustify(int v)              { ed_v_justify = v; }
    void incVertJustify()       { if (++ed_v_justify > 2) ed_v_justify = 0; }
    void decVertJustify()       { if (--ed_v_justify < 0) ed_v_justify = 2; }

    void setPCellAbutMode(AbutMode m)       { ed_auto_abut_mode = m; }
    AbutMode pcellAbutMode()                { return (ed_auto_abut_mode); }

    void setHideGrips(bool b)               { ed_hide_grips = b; }
    bool hideGrips()                        { return (ed_hide_grips); }

    bool replacing()                        { return (ed_replacing); }
    void setReplacing(bool b)               { ed_replacing = b; }

    bool useArray()                         { return (ed_use_array); }
    void setUseArray(bool b)                { ed_use_array = b; }

    bool noWireWidthMag()                   { return (ed_no_wire_width_mag); }
    void setNoWireWidthMag(bool b)          { ed_no_wire_width_mag = b; }

    bool noFlattenStdVias()                 { return (ed_no_flatten_vias); }
    void setNoFlattenStdVias(bool b)        { ed_no_flatten_vias = b; }

    bool noFlattenPCells()                  { return (ed_no_flatten_pcells); }
    void setNoFlattenPCells(bool b)         { ed_no_flatten_pcells = b; }

    yb **yankBuffer()                       { return (ed_yank_buffer); }

    const char *layerExpString(unsigned int i)
        {
            if (i < ED_LEXPR_STORES)
                return (ed_lexpr_stores[i]);
            return (0);
        }

    void setLayerExpString(const char *s, unsigned int i)
        {
            if (i < ED_LEXPR_STORES) {
                delete [] ed_lexpr_stores[i];
                ed_lexpr_stores[i] = 0;
                if (s) {
                    char *t = new char[strlen(s) + 1];
                    strcpy(t, s);
                    ed_lexpr_stores[i] = t;
                }
            }
        }

    GRvecFont *logoFont()                   { return (ed_logofont); }

    // Interface to Placement Control pop-up and support.

    bool plIsActive()
        { return (ed_popup != 0); }

    void plEscCallback()
        { if (ed_popup) ed_popup->desel_placebtn(); }

    bool plIsSmashMode()
         { return (ed_popup && ed_popup->smash_mode()); }

    void plSetPopup(sPCpopup *p)    { ed_popup = p; }
    stringlist *plMenu()            { return (ed_menu_head); }
    int plMenuLen()                 { return (ed_menu_len); }

private:
    // copy.cc
    bool copy_call(int, int, int, int, CDc*, CDmcType, CDc**);
    bool translate(CDo*, CDs*, CDtf*, CDtf*, int, int, int, int,
        CDl*, CDl*, CDmcType, bool* = 0);
    bool translate_muts(const cTfmStack*, int, int, int, int, CDol**,
        CDs* = 0);

    // edit_txtcmds.cc
    void setupBangCmds();

    // edit_variables.cc
    void setupVariables();

    // erase.cc
    bool clip(CDo*, CDs*, BBox*);
    bool erase(CDo*, CDs*, BBox*);
    yb *add_yank(CDo*, BBox*, yb*, bool);

    // flatten.cc
    bool promote_object(cTfmStack*, CDo*, CDs*, CDtf*, CDtf*, bool, bool);
    bool flatten_cell_recurse(cTfmStack*, CDs*, int, int, CDtf*, bool, bool);

    // funcs_geom?.cc
    void load_script_funcs2();

    // instance.cc
    void get_reference_handle(CDs*, int*, int*, PLref, const iap_t&,
        const CDc*);
    bool find_contact(CDs*, int*, int*, bool);
    CDc *new_instance(CDs*, int, int, const iap_t&, PLref, bool,
        int = 0, int = 0);

    // modify.cc
    bool set_stretch_ref(CDo*, int*, int*, double*, int*, int*, bool);
    bool stretch(CDo*, CDs*, int, int, int, int, int, Vtex*);
    bool change_layer(CDo*, CDs*, CDl*, bool, bool, bool);

    // prpedit.cc
    bool acceptBoxPseudoProp(CDo*, CDs*, int, const char*);
    bool acceptPolyPseudoProp(CDpo*, CDs*, int, const char*);
    bool acceptWirePseudoProp(CDw*, CDs*, int, const char*);
    bool acceptLabelPseudoProp(CDla*, CDs*, int, const char*);
    bool acceptInstPseudoProp(CDc*, CDs*, int, const char*);
    
    // rotate.cc
    bool rotate(const cTfmStack*, CDo*, CDs*, int, int, double, CDl*, CDl*,
        CDmcType);

    // xorbox.cc
    void process_xor(CDo*, CDs*, BBox*);

    // Current instance array parameters.
    iap_t ed_array_params;

    sEdit45 ed_45;              // State for 45-degree constraint.

    sObj *ed_object_list;       // Used to pass a list of objects with
                                // vertices to move.

    CDl *ed_press_layer;        // Current layer at move/copy button down.

    sPCpopup *ed_popup;         // Interface to Placement Control pop-up.

    stringlist *ed_menu_head;   // List of masters.

    CDcl *ed_push_data;         // Data for the Push command.

    CDs *ed_pcsuper;            // Used during cell placement from GUI.
    PCellParam *ed_pcparams;

    sGrip *ed_cur_grip;         // Stretch handle being manipulated.
    cGripDb *ed_gripdb;         // Grip database for current cell.

    WireStyle ed_wire_style;    // End style for wires.

    CDmcType ed_move_or_copy;   // Set in MainState and move/copy commands,
                                // for ghost draw.

    LCHGmode ed_lchange_mode;   // Layer remapping after move/copy if
                                // current layer changes.

    int ed_label_x;             // Exports from the label command.
    int ed_label_y;
    int ed_label_width;
    int ed_label_height;
    int ed_label_xform;

    PLref ed_place_ref;         // Instance placement origin reference.

    int ed_stretch_box_code;    // Used to pass a corner code to global
                                // functions while stretching boxes.

    short ed_h_justify;         // Text justification for labels/logo.
    short ed_v_justify;

    int ed_menu_len;            // Length of ed_menu_head list.

    AbutMode ed_auto_abut_mode; // Auto-Abutment mode for pcells, 0-2.
    bool ed_hide_grips;         // Turn off pcell grips if set.

    bool ed_replacing;          // Instances are being replaced, not added.
    bool ed_use_array;          // Array parameters are specified.

    bool ed_no_wire_width_mag;  // Don't change width of magnified wires.

    bool ed_no_flatten_vias;    // Don't flatten standard via cells.
    bool ed_no_flatten_pcells;  // Don't flatten parameterized cells.

    struct yb *ed_yank_buffer[ED_YANK_DEPTH]; // Storage for yanked geometry.
    char *ed_lexpr_stores[ED_LEXPR_STORES]; // Saved layer expression strings.

    static GRvecFont *ed_logofont;
};

#endif

