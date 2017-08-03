
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include "dsp.h"
#include "dsp_window.h"
#include "main_variables.h"


//
// Top level include file for Xic application.
//

// Enable testing for use violations.
#define SECURITY_TEST

// Product Codes
#define XIC_PRODUCT_CODE    1
#define XICII_PRODUCT_CODE  2
#define XIV_PRODUCT_CODE    3

// Look ahead
struct CmdDesc;
struct FIOreadPrms;
struct LayerFillData;
struct line;
struct Ptxt;
struct sAuthChk;
struct SIfile;
class SIinterp;
struct umenu;
struct siVariable;
struct WindowDesc;
struct MenuBox;
struct MenuEnt;
struct sLcb;
struct updif_t;
struct cfilter_t;

namespace main_txtcmds {
    struct bcel_t;
}
template <class T> struct table_t;

namespace ginterf
{
    struct HCcb;
}
namespace ed_edit {
    struct sEditState;
}

// Storage of undo/redo lists, etc., for mode switch.
//
struct sModeSave
{
    sModeSave();
    void saveCurrent();
    void assertSaved();
    void clearHist();
    CDl *currentLd();

    ed_edit::sEditState *editState() { return (EditState); }
    void setEditState(ed_edit::sEditState *s) { EditState = s; }

    BBox PhysMainWin;
    BBox ElecMainWin;

private:
    double PhysMagn;
    CDcellName PhysCurCellName;
    CDcellName PhysTopCellName;
    CDcellName ElecCurCellName;
    CDcellName ElecTopCellName;
    double PhysCharWidth;
    double PhysCharHeight;
    double ElecCharWidth;
    double ElecCharHeight;
    CDtf PhysTf;
    CDtf ElecTf;
    CDl *PhysLd;
    CDl *ElecLd;
    ed_edit::sEditState *EditState;
};


// Return from cMain::CheckModified().
//
enum CmodType
{
    CmodAborted     = -1,       // User abort
    CmodFailed      = 0,        // Write failed
    CmodOK          = 1,        // Success
    CmodNoChange    = 2         // No write required
};

// Program run mode
enum ModeType { ModeNormal, ModeBatch, ModeServer, ModeBackground };

// Flags for cMain::DebugsFlags, these turn on various messages.
//
#define DBG_SELECT      0x1
#define DBG_UNDOLIST    0x2

#define DBG_FP (XM()->DebugFp() ? XM()->DebugFp() : \
  XM()->DebugFile() && !strcmp(XM()->DebugFile(), "stdout") ? stdout : stderr)

// SetWireAttribute() argument
enum WsType { WsWidth, WsFlush, WsRound, WsExtnd };

// Argument to cMain::Exit.
enum ExitType { ExitCheckMod, ExitNormal, ExitPanic, ExitDebugger };

// Return from cMain::EditCell() and cMain::Load().
//
enum EditType
{
    EditReentered   = -2,       // Attempt to reenter function call
    EditAborted     = -1,       // User abort
    EditFailed      = 0,        // Open failed
    EditOK          = 1,        // Success
    EditAmbiguous   = 2,        // Read, but current cell not set
    EditText        = 3         // Special file, opened for text editing
};

// Argument to cMain::SetCoordMode.
//
enum COmode
{
    CO_ABSOLUTE,                // Set "absolute" mode.
    CO_RELATIVE,                // Set "relative" mode.
    CO_REDRAW                   // Redraw coord box, for color change.
};

// Supported cursor styles.
//
enum CursorType {
    CursorDefault,      // toolkit default
    CursorCross,        // legacy cross cursor
    CursorLeftArrow,    // arrow cursor
    CursorRightArrow,   // arrow cursor
    CursorBusy          // busy cursor
};

// This list type is used to pass lists of property values for display.
//
struct Ptxt
{
    Ptxt(char *h, char *s, CDp *p, Ptxt *n = 0)
        {
            pt_head = h;
            pt_string = s;
            pt_start = pt_end = 0;
            pt_pdesc = p;
            pt_next = n;
        }

    ~Ptxt()
        {
            delete [] pt_head;
            delete [] pt_string;
        }

    static void destroy(Ptxt *p)
        {
            while(p) {
                Ptxt *px = p;
                p = p->pt_next;
                delete px;
            }
        }

    // Convert the Ptxt list into a long string, and fill in the
    // offsets in the Ptxt elements.
    //
    static char *tostring(Ptxt *thisp)
        {
            int cnt = 0;
            sLstr lstr;
            for (Ptxt *p = thisp; p; p = p->pt_next) {
                p->pt_start = cnt;
                lstr.add(p->pt_head);
                cnt += strlen(p->pt_head);
                lstr.add(p->pt_string);
                cnt += strlen(p->pt_string);
                p->pt_end = cnt;
                lstr.add_c('\n');
                cnt++;
            }
            return (lstr.string_trim());
        }

    Ptxt *next()            const { return (pt_next); }
    void set_next(Ptxt *p)        { pt_next = p; }

    const char *head()      const { return (pt_head); }
    const char *string()    const { return (pt_string); }

    int start()             const { return (pt_start); }
    void set_start(int i)         { pt_start = i; }

    int end()               const { return (pt_end); }
    void set_end(int i)           { pt_end = i; }

    CDp *prpty()            const { return (pt_pdesc); }

private:
    char *pt_head;
    char *pt_string;
    int pt_start;
    int pt_end;
    CDp *pt_pdesc;
    Ptxt *pt_next;
};

// Prototype for registered 'bang' commands.  True is returned if a
// command was executed, false otherwise.
//
typedef void(BangCmdFunc)(const char*);

// This is a dummy hash name for the command that executes text as script
// functions.
#define BANG_BANG_NAME "!!name"

// Passed to the PopUpTree method.
enum TreeUpdMode { TU_CUR = -1, TU_PHYS = Physical, TU_ELEC = Electrical };

inline class cMain *XM();

// This is the main class for the application.
//
class cMain : public GRappCalls
{
    static cMain *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cMain *XM() { return (cMain::ptr()); }

    // main.cc
    cMain();
    const char *IdString();

    // xic.cc
    void InitializeStrings();
    void InitializeVariables();
    void InitializeMenus();
    const char *NextArg();
    void SetPrefix(const char*);
    char *ReleaseNotePath();

    // attr_menu.cc
    MenuBox *createAttrMenu();
    MenuBox *createSubwAttrMenu();
    MenuBox *createObjSubMenu();

    // bangcmds.cc
    void RegisterBangCmd(const char*, BangCmdFunc*);
    stringlist *ListBangCmds();
    bool TextCmd(const char*, bool);

    // cell_menu.cc
    MenuBox *createCellMenu();
    bool setupCommand(MenuEnt*, bool*, bool*);

    // cvrt_menu.cc
    MenuBox *createCvrtMenu();

    // daemon.cc
    int Daemon(int);

    // expand.cc
    void ExpandExec(CmdDesc*);

    // file_menu.cc
    MenuBox *createFileMenu();
    const char *const *OpenCellMenuList();
    void PushOpenCellName(const char*);
    void ReplaceOpenCellName(const char*, const char*);
    void HandleOpenCellMenu(const char*, bool);

    // grcalls.cc
    // These are the GRappCalls virtual overrides.  A pointer to this
    // interface is also available through GRappIf().
    const char *GetPrintCmd();
    char *ExpandHelpInput(char*);
    bool ApplyHelpInput(const char *s);
    pix_list *ListPixels();
    int PixelIndex(int);
    int BackgroundPixel();
    int LineTypeIndex(const GRlineType*);
    int FillTypeIndex(const GRfillType*);
    int FillStyle(int, int, int* = 0, int* = 0);
    // These are defined in the graphics package.
    bool DrawCallback(void*, GRdraw*, int, int, int, int, int, int);
    void *SetupLayers(void*, GRdraw*, void*);
    bool MenuItemLocation(int, int*, int*);

    // hardcopy.cc
    void HCswitchMode(bool, bool, int);
    void HCdrawFrame(int);
    void HCkillFrame();

    // help_menu.cc
    MenuBox *createHelpMenu();
    MenuBox *createSubwHelpMenu();
    void QuitHelp();

    // info.cc
    void InfoExec(CmdDesc*);
    void InfoRefresh();
    void ShowCellInfo(const char*, bool, DisplayMode);
    CDclxy *GetPushList();
    char *Info(CDs*, int);
    char *Info(const CDo*);

    // init.cc
    void AppInit();
    void ExecStartupScript(const char*);
    bool WhereisPointer(int*, int*);
    bool CheckCurMode(DisplayMode);
    bool CheckCurLayer();
    bool CheckCurCell(bool, bool, DisplayMode);
    void Rehash();
    void LegalMsg();
    static int System(const char*);

    // keymap.h
    void KeyMapExec(CmdDesc*);

    // layers.cc
    stringlist *ListLayersInArea(CDs*, BBox*, int);
    void PeekExec(CmdDesc*);
    void ProfileExec(CmdDesc*);

    // layertab.cc
    void EditLtabExec(CmdDesc*);
    void SetLayerSpecificSelections(bool);
    void SetLayerSearchUpSelections(bool);

    // line45.cc
    void To45(int, int, int*, int*);
    bool To45snap(int*, int*, int, int);
    bool IsManhattan(int, int, int, int);

    // main_txtcmds.cc
    static void ShellWindowCmd(const char*);

    // measure.cc
    void RulerExec(CmdDesc*);
    void SetRulers(CDs*, CDs*);
    void EraseRulers(CDs*, WindowDesc*, int);

    // misc_menu.cc
    MenuBox *createMiscMenu();

    // modeswitch.cc
    void SetMode(DisplayMode, int = 0);
    void ClearAltModeUndo();
    void SetHierDisplayMode(const char*, const char*, const BBox*);
    void SetSymbolTable(const char*);
    void ClearSymbolTable();
    void ClearReferences(bool, bool = false);
    void Clear(const char*);
    void ClearAll(bool);

    // onexit.cc
    bool ConfirmAbort(const char* = 0);
    void Exit(ExitType);

    // open.cc
    EditType EditCell(const char*, bool, const FIOreadPrms* = 0,
        const char* = 0, cCHD* = 0);
    EditType Load(WindowDesc*, const char*, const FIOreadPrms* = 0,
        const char* = 0, cCHD* = 0);
    OItype TouchCell(const char*, bool);
    void SetNewContext(CDcbin*, bool);
    char *NewCellName();

    // prpty.cc
    Ptxt *PrptyStrings(CDs*);
    Ptxt *PrptyStrings(CDo*, CDs*);
    char *GetPseudoProp(CDo*, int);
    bool IsBoundaryVisible(const CDs*, const CDo*);

    // pushpop.cc
    void pushSetup();
    void pushExec(CmdDesc*);

    // save.cc
    void Save();
    CmodType CheckModified(bool);
    bool SaveCellAs(const char*, bool = false);
    void CommitCell(bool = false);

    // scriptif.cc
    void LoadScriptFuncs();
    void SaveScript(FILE*);
    void DumpScripts(FILE*);
    SymTab *GetFormatFuncTab(int);
    siVariable *GetFormatVars(int);
    void RegisterScript(const char*, const char*);
    umenu *GetFunctionList();
    void OpenScript(const char*, SIfile**, stringlist**, bool = false);
    char *FindScript(const char*);
    void OpenFormatLib(int);
    void RunTechScripts();

    // select.cc
    void RegisterDeselOverride(void(*)(CmdDesc*));
    void DeselectExec(CmdDesc*);

    // signals.cc
    void InitSignals(bool);
    static void InterruptHandler();

    // subwin.cc
    void SubWindowExec(CmdDesc*);

    // user_menu.cc
    MenuBox *createUserMenu();

    // variable.cc
    void SetupVariables();
    void PopUpVariables(bool);

    // view_menu.cc
    const char *const *ViewList();
    MenuBox *createViewMenu();
    MenuBox *createSubwViewMenu();

    // graphics system
    //
    void ShowParameters(const char* = 0);
    void SetCoordMode(COmode, int = 0, int = 0);

    void PopUpDebugFlags(GRobject, ShowMode);
    void PopUpMemory(ShowMode);
    char *SaveFileDlg(const char*, const char*);
    char *OpenFileDlg(const char*, const char*);
    void PopUpFileSel(const char*, void(*)(const char*, void*), void*);

    void PopUpColor(GRobject, ShowMode);
    void ColorTimerInit();

    void FixupColors(void*);

    void PopUpDebug(GRobject, ShowMode);
    bool DbgLoad(MenuEnt*);

    char *GetCurFileSelection();
    void DisableDialogs();

    void PopUpCells(GRobject, ShowMode);
    void PopUpCellFilt(GRobject, ShowMode, DisplayMode,
        void(*)(cfilter_t*, void*), void*);
    void PopUpCellFlags(GRobject, ShowMode, const stringlist*, int);

    void PopUpFillEditor(GRobject, ShowMode);
    void FillLoadCallback(LayerFillData*, CDl*);

    CursorType GetCursor();
    void UpdateCursor(WindowDesc*, CursorType, bool = false);
    void PopUpAttributes(GRobject, ShowMode);
    void PopUpSelectControl(GRobject, ShowMode);
    void PopUpSymTabs(GRobject, ShowMode);
    void PopUpTree(GRobject, ShowMode, const char*, TreeUpdMode,
        const char* = 0);

    sLcb *PopUpLayerEditor(GRobject);
    void PopUpLayerParamEditor(GRobject, ShowMode, const char*, const char*);
    void PopUpLayerAliases(GRobject, ShowMode);
    void PopUpLayerPalette(GRobject, ShowMode, bool, CDl*);

    void PopUpTechWrite(GRobject, ShowMode);

    bool SendButtonEvent(const char*, int, int, int, int, bool);
    bool SendKeyEvent(const char*, int, int, bool);

    void SetNoToTop(bool);
    void SetLowerWinOffset(int);

    // access to private members
    //
    DisplayMode InitialMode()           { return (xm_initial_mode); }
    void SetInitialMode(DisplayMode m)  { xm_initial_mode = m; }

    sModeSave &hist()                   { return (xm_mode_save); }

    bool IsDoingHelp()                  { return (xm_doing_help); }
    void SetDoingHelp(bool b)           { xm_doing_help = b; }

    bool IsNoConfirmAbort()             { return (xm_no_confirm_abort); }
    void SetNoConfirmAbort(bool b)      { xm_no_confirm_abort = b; }

    bool IsFullWinCursor()              { return (xm_full_win_cursor); }
    void SetFullWinCursor(bool b)       { xm_full_win_cursor = b; }

    bool IsAppInitDone()                { return (xm_app_init_done); }
    void SetAppInitDone(bool b)         { xm_app_init_done = b; }

    bool IsAppReady()                   { return (xm_app_ready); }
    void SetAppReady(bool b)            { xm_app_ready = b; }

    bool IsExitCleanupDone()            { return (xm_exit_cleanup_done); }
    void SetExitCleanupDone(bool b)     { xm_exit_cleanup_done = b; }

    const char *Product()               { return (xm_product); }
    const char *Description()           { return (xm_description); }
    const char *Program()               { return (xm_program); }
    void SetProgram(char *s)            { xm_program = s; }

    const char *ProgramRoot()           { return (xm_program_root); }
    const char *Prefix()                { return (xm_prefix); }
    const char *ToolsRoot()             { return (xm_tools_root); }
    const char *AppRoot()               { return (xm_app_root); }
    const char *HomeDir()               { return (xm_homedir); }

    const char *VersionString()         { return (xm_version_string); }
    const char *BuildDate()             { return (xm_build_date); }
    const char *TagString()             { return (xm_tag_string); }
    const char *AboutFile()             { return (xm_about_file); }
    const char *OSname()                { return (xm_os_name); }
    const char *Arch()                  { return (xm_arch); }
    const char *DistSuffix()            { return (xm_dist_suffix); }
    const char *Geometry()              { return (xm_geometry); }
    void SetGeometry(char *s)           { xm_geometry = s; }
    const char *TechFileBase()          { return (xm_tech_file_base); }
    const char *DefaultEditName()       { return (xm_default_edit_name); }
    const char *DeviceLibName()         { return (xm_device_lib_name); }
    void SetDeviceLibName(char *s)      { xm_device_lib_name = s; }
    const char *ModelLibName()          { return (xm_model_lib_name); }
    void SetModelLibName(char *s)       { xm_model_lib_name = s; }
    const char *ModelSubdirName()       { return (xm_model_subdir_name); }
    void SetModelSubdirName(char *s)    { xm_model_subdir_name = s; }
    const char *InitScript()            { return (xm_init_script); }
    const char *StartupScript()         { return (xm_startup_script); }
    const char *MacroFileName()         { return (xm_macro_file_name); }
    const char *ExecDirectory()         { return (xm_exec_directory); }
    const char *SpiceExecName()         { return (xm_spice_exec_name); }
    const char *FontFileName()          { return (xm_font_file_name); }
    const char *LogoFontFileName()      { return (xm_logo_font_file_name); }
    const char *LogName()               { return (xm_log_name); }

    sAuthChk *Auth()                    { return (xm_auth); }
    int ProductCode()                   { return (xm_product_code); }
    int BuildYear()                     { return (xm_build_year); }
    ModeType RunMode()                  { return (xm_run_mode); }
    void SetRunMode(ModeType m)         { xm_run_mode = m; }

    unsigned int DebugFlags()           { return (xm_debug_flags); }
    void SetDebugFlags(unsigned int f)  { xm_debug_flags = f; }
    const char *DebugFile()             { return (xm_debug_file); }
    void SetDebugFile(char *f)          { xm_debug_file = f; }
    FILE *DebugFp()                     { return (xm_debug_fp); }
    void SetDebugFp(FILE *fp)           { xm_debug_fp = fp; }
    FILE *PanicFp()                     { return (xm_panic_fp); }
    void SetPanicDir(const char *d)     { xm_panic_dir = d; }
    const char *PanicDir()              { return (xm_panic_dir); }

    bool HtextCnamesOnly()              { return (xm_htext_cnames_only); }

    void SetMemError(bool b)            { xm_mem_error = b; }
    bool MemError()                     { return (xm_mem_error); }

    void SetSavingDev(bool b)           { xm_saving_dev = b; }
    bool SavingDev()                    { return (xm_saving_dev); }

    void SetTreeCaptive(bool b)         { xm_tree_captive = b; }
    bool TreeCaptive()                  { return (xm_tree_captive); }

    static const updif_t *UpdateIf()    { return (xm_updif); }

private:
    // funcs_misc*.cc
    void load_funcs_misc1();
    void load_funcs_misc2();
    void load_funcs_misc3();

    // hardcopy.cc
    void setupHcopy();

    // main_setif.cc
    void setupInterface();

    // main_techif.cc
    void setupTech();

    // main_txtcmds.cc
    void setupBangCmds();

    // memory.cc
    void setupMemoryBangCmds();

    // Misc. name strings
    const char *xm_product;           // The product name, e.g., "Xic"
    const char *xm_description;       // Short description of this app
    const char *xm_program;           // Program name, may include path

    const char *xm_program_root;      // Installation location
    const char *xm_prefix;            // Installation location prefix
    const char *xm_tools_root;        // Installation location common
    const char *xm_app_root;          // Installation location dir
    const char *xm_homedir;           // Home directory

    const char *xm_version_string;    // The program version number
    const char *xm_build_date;        // Program build date string
    const char *xm_tag_string;        // The CVS tag string
    const char *xm_about_file;        // About message file name
    const char *xm_os_name;           // Operating sysyem name
    const char *xm_arch;              // CPU architecture name
    const char *xm_dist_suffix;       // Distrib file suffix
    const char *xm_geometry;          // X-style geometry string
    const char *xm_tech_file_base;    // Base name of tech file to read.
    const char *xm_default_edit_name; // Default edit cell ("noname")
    const char *xm_device_lib_name;   // Device lib file (def. "device.lib")
    const char *xm_model_lib_name;    // Model lib file (def. "model.lib")
    const char *xm_model_subdir_name; // Subdir name for models (def. "models")
    const char *xm_init_script;       // Name of init script (".xicinit")
    const char *xm_startup_script;    // Name of startup script (".xicstart")
    const char *xm_macro_file_name;   // Name of macro file (".xicmacros")
    const char *xm_exec_directory;    // Dir containing executables
    const char *xm_spice_exec_name;   // WRspice name ("wrspice")
    const char *xm_font_file_name;    // Name of font file ("xic_font")
    const char *xm_logo_font_file_name; // Logo font file ("xic_logofont")
    const char *xm_log_name;          // Name of log file ("xic_run.log")

    sAuthChk *xm_auth;          // Authentication class.
    ModeType xm_run_mode;       // Normal/Batch/Server mode.
    int xm_product_code;        // A product indicator index.
    int xm_build_year;          // Year of build date.

    table_t<main_txtcmds::bcel_t> *xm_bang_cmd_tab;
                                 // Table of 'bang' commands.
    CDvarTab *xm_var_bak;        // Initial variables.
    DisplayMode xm_initial_mode; // Startup display mode.

    sModeSave xm_mode_save;     // Context save for mode switch.

    CDclxy *xm_push_data;       // Data for the Push command.

    const char *xm_debug_file;  // File for debugging output.
    FILE *xm_debug_fp;          // File pointer for debugging output.

    // When nonzero, all messages during file/cell write are directed
    // to this log, during crash dump.
    FILE *xm_panic_fp;
    const char *xm_panic_dir;   // Panic subdirectory (save location)

    // Misc flags
    unsigned int xm_debug_flags; // Enable misc. debugging features
    bool xm_doing_help;         // Help is active.
    bool xm_no_confirm_abort;   // Suppress abort dialog after interrupt.
    bool xm_full_win_cursor;    // True to use full screen cursor.
    bool xm_app_init_done;      // True after AppInit() called.
    bool xm_app_ready;          // True after first expose event.
    bool xm_exit_cleanup_done;  // Exiting
    bool xm_htext_cnames_only;  // Restrict hypertext returns to cellnames.
    bool xm_mem_error;          // Memory error occurred.
    bool xm_saving_dev;         // True when saving a device to a file.
    bool xm_tree_captive;       // Tree is showing cell from Cells List.

    // Update interface callbacks
    static const updif_t *xm_updif;

    static cMain *instancePtr;
};


//
// Macros
//

// subclass coersion for objects
#define OPOLY(zz)  ((CDpo*)zz)
#define OWIRE(zz)  ((CDw*)zz)
#define OLABEL(zz) ((CDla*)zz)
#define OCALL(zz)  ((CDc*)zz)

// subclass coersion for properties
#define PNOD(zz)   ((CDp_node*)zz)
#define PNAM(zz)   ((CDp_name*)zz)
#define PMUT(zz)   ((CDp_mut*)zz)
#define PNMU(zz)   ((CDp_nmut*)zz)
#define PBRN(zz)   ((CDp_branch*)zz)
#define PLRF(zz)   ((CDp_lref*)zz)
#define PMRF(zz)   ((CDp_mutlrf*)zz)
#define PUSR(zz)   ((CDp_user*)zz)

// Integer swap
#define SwapInts(a, b) { int c = a; a = b; b = c; }

#endif

