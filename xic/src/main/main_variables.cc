
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

#include "main.h"
#include "scedif.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "menu.h"
#include "view_menu.h"
#include "help_menu.h"
#include "promptline.h"
#include "layertab.h"
#include "errorlog.h"
#include "select.h"
#include "tech.h"
#include "help/help_defs.h"
#include "filestat.h"


//
// What follows are a number of static functions which are executed when
// certain variables are set/unset.  Pointers to these are kept in a
// symbol table for efficiency
//

namespace {
    // A smarter atoi().
    //
    inline bool
    str_to_int(int *iret, const char *s)
    {
        if (!s)
            return (false);
        char *e;
        *iret = strtol(s, &e, 0);
        return (e != s);
    }


    // A smarter atof().
    //
    inline bool
    str_to_dbl(double *dret, const char *s)
    {
        if (!s)
            return (false);
        return (sscanf(s, "%lf", dret) == 1);
    }

    void
    postAttr(const char*)
    {
        XM()->PopUpAttributes(0, MODE_UPD);
    }

    void
    postTech(const char*)
    {
        XM()->PopUpTechWrite(0, MODE_UPD);
    }
}


//--------------------------------------------------------------------------
// Database Setup

namespace {
    bool
    evDatabaseResolution(const char *vstring, bool set)
    {
        if (XM()->IsAppInitDone() || Tech()->HaveTechfile()) {
            Log()->ErrorLog(mh::Variables,
                "Resolution can not be changed after startup.");
            return (false);
        }
        if (set) {
            int res = atoi(vstring);
            if (res == 1000 || res == 2000 || res == 5000 || res == 10000)
                CDphysResolution = res;
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect DatabaseResolution: must be 1000, 2000, "
                    "5000, or 10000.");
                return (false);
            }
        }
        else
            CDphysResolution = 1000;
        SIparse()->updateInfinity();
        return (true);
    }
}


//--------------------------------------------------------------------------
// Paths and Directories

namespace {
    bool
    evLibPath(const char*, bool set)
    {
        // This variables can't be unset.
        return (set);
    }

    bool
    evHelpPath(const char *vstring, bool set)
    {
        if (set) {
            HLP()->set_path(vstring, false);
            HLP()->rehash();
        }

        // This variables can't be unset.
        return (set);
    }

    void
    postScriptPath(const char*)
    {
        XM()->Rehash();
    }

    bool
    evScriptPath(const char*, bool set)
    {
        // In startup, don't want to call Rehash twice:  first when
        // ScriptPath is processed in the tech file (we get here), again
        // when Rehash is called in Initialize().  XM()->app_init_done is
        // set when XM()->AppInit returns, so won't be set when reading tech
        // file.

        if (set && XM()->IsAppInitDone())
            CDvdb()->registerPostFunc(postScriptPath);

        // This variables can't be unset.
        return (set);
    }

    bool
    evTeePrompt(const char *vstring, bool set)
    {
        PL()->TeePromptUser(set ? vstring : 0);
        return (true);
    }
}


//--------------------------------------------------------------------------
// General Visual

namespace {
    bool
    evMouseWheel(const char *str, bool set)
    {
        if (set) {
            double pfct, zfct;
            if (sscanf(str, "%lf %lf", &pfct, &zfct) != 2 ||
                    pfct < 0.0 || pfct > 0.4 || zfct < 0.0 || zfct > 0.5) {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect MouseWheel: must be two numbers both 0.0-0.5.");
                return (false);
            }
            DSP()->SetMouseWheelPanFactor(pfct);
            DSP()->SetMouseWheelZoomFactor(zfct);
        }
        else {
            DSP()->SetMouseWheelPanFactor(DEF_MW_PAN_FCT);
            DSP()->SetMouseWheelZoomFactor(DEF_MW_ZOOM_FCT);
        }
        return (true);
    }

    bool
    evListPageEntries(const char *vstring, bool set)
    {
        if (set) {
            int d;
            if (str_to_int(&d, vstring) && d >= 10 && d <= 50000)
                ;
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect ListPageEntries: range 100-50000.");
                return (false);
            }
        }
        return (true);
    }

    bool
    evNoLocalImage(const char*, bool set)
    {
        DSP()->SetNoLocalImage(set);
        return (true);
    }

    bool
    evNoPixmapStore(const char*, bool set)
    {
        DSP()->SetNoPixmapStore(set);
        return (true);
    }

    bool
    evNoDisplayCache(const char*, bool set)
    {
        DSP()->SetNoDisplayCache(set);
        return (true);
    }

    bool
    evLowerWinOffset(const char *vstring, bool set)
    {
        if (set) {
            int d;
            if (!str_to_int(&d, vstring) || abs(d) > 16) {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect LowerWinOffset: must be -16 to 16.");
                return (false);
            }
            XM()->SetLowerWinOffset(d);
        }
        else
            XM()->SetLowerWinOffset(0);
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }

    bool
    evPhysGridOrigin(const char *vstring, bool set)
    {
        if (set) {
            double x, y;
            if (sscanf(vstring, "%lf%*c%lf", &x, &y) < 2) {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect PhysGridOrigin: must be 2 numbers.");
                return (false);
            }
            DSP()->PhysVisGridOrigin()->x = INTERNAL_UNITS(x);
            DSP()->PhysVisGridOrigin()->y = INTERNAL_UNITS(y);
        }
        else {
            DSP()->PhysVisGridOrigin()->x = 0;
            DSP()->PhysVisGridOrigin()->y = 0;
        }
        DSP()->window_view_change(DSP()->MainWdesc());
        DSP()->RedisplayAll(Physical);
        return (true);
    }

    bool
    evPixelDelta(const char *vstr, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstr) && d >= 1.0 && d <= 10.0)
                DSP()->SetPixelDelta(d);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect PixelDelta: range 1.0-10.0.");
                return (false);
            }
        }
        else
            DSP()->SetPixelDelta(DEF_PixelDelta);
        return (true);
    }


    bool
    evNoPhysRedraw(const char*, bool set)
    {
        LT()->SetNoPhysRedraw(set);
        return (true);
    }

    bool
    evNoToTop(const char*, bool set)
    {
        XM()->SetNoToTop(set);
        return (true);
    }
}


//--------------------------------------------------------------------------
// Scripts

namespace {
    bool
    evLogIsLog10(const char*, bool set)
    {
        SI()->SetLogIsLog10(set);
        return (true);
    }
}


//--------------------------------------------------------------------------
// Selections

namespace {
    bool
    evMarkInstanceOrigin(const char*, bool set)
    {
        DSP()->SetShowInstanceOriginMark(set);
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }

    bool
    evMarkObjectCentroid(const char*, bool set)
    {
        DSP()->SetShowObjectCentroidMark(set);
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }

    bool
    evMaxBlinkingObjects(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 500 && i <= 250000)
                Selections.setMaxBlinkingObjects(i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect MaxBlinkingObjects: must be 500-250000.");
                return (false);
            }
        }
        else
            Selections.setMaxBlinkingObjects(DEF_MAX_BLINKING_OBJECTS);
        return (true);
    }
}


//--------------------------------------------------------------------------
// File Menu - Printing

namespace {
    bool
    evNoAskFileAction(const char*, bool set)
    {
        GRwbag::SetNoAskFileAction(set);
        return (true);
    }

    bool
    evDefaultPrintCmd(const char *vstring, bool set)
    {
        if (set)
            HCdefaults::default_print_cmd = lstring::copy(vstring);
        else
#ifdef WIN32
            HCdefaults::default_print_cmd = "default";
#else
            HCdefaults::default_print_cmd = "lpr";
#endif
        return (true);
    }

    bool
    evPSlineWidth(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) && d >= 0.0 && d <= 25.0)
                HCpsLineWidth = d;
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect PSLineWidth: range 0-25.");
                return (false);
            }
        }
        else
            HCpsLineWidth = 0.0;
        return (true);
    }

    bool
    evRmTmpFileMinutes(const char *vstring, bool set)
    {
        if (set) {
            int d;
            if (str_to_int(&d, vstring) && d >= 0 && d <= 4320)
               filestat::set_rm_minutes(d);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect RmTmpFileMinutes: range 0-4320.");
                return (false);
            }
#ifdef WIN32
            DSPmainWbag(PopUpMessage(
                "Temp. file deletion scheduling is not available in Windows.",
                false))
#endif
        }
        else
            filestat::set_rm_minutes(0);
        return (true);
    }
}


//--------------------------------------------------------------------------
// Cell Menu Commands

namespace {
    bool
    evContextDarkPcnt(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= DSP_MIN_CX_DARK_PCNT &&
                    i <= 100)
                DSP()->SetContextDarkPcnt(i);
            else { 
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect ContextDarkPcnt: must be %d-100.",
                    DSP_MIN_CX_DARK_PCNT);
                return (false);
            }
        }
        else
            DSP()->SetContextDarkPcnt(DSP_DEF_CX_DARK_PCNT);
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }
}


//--------------------------------------------------------------------------
// Side Menu Commands

namespace {
    bool
    evLabelDefHeight(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) && d >= CD_MIN_TEXT_HEI &&
                    d <= CD_MAX_TEXT_HEI) {
                CDphysDefTextHeight = d;
                CDphysDefTextWidth = CD_TEXT_WID_PER_HEI*d;
            }
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect LabelDefHeight: must be %.2f - %.1f.",
                    CD_MIN_TEXT_HEI, CD_MAX_TEXT_HEI);
                return (false);
            }
        }
        else {
            CDphysDefTextHeight = CD_DEF_TEXT_HEI;
            CDphysDefTextWidth = CD_TEXT_WID_PER_HEI * CD_DEF_TEXT_HEI;
        }
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }

    bool
    evLabelMaxLen(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= DSP_MIN_MAX_LABEL_LEN &&
                    i <= DSP_MAX_MAX_LABEL_LEN)
                DSP()->SetMaxLabelLen(i);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect LabelMaxLen: must be %d - %d.",
                    DSP_MIN_MAX_LABEL_LEN, DSP_MAX_MAX_LABEL_LEN);
                return (false);
            }
        }
        else
            DSP()->SetMaxLabelLen(DSP_DEF_MAX_LABEL_LEN);
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }

    bool
    evLabelMaxLines(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= DSP_MIN_MAX_LABEL_LINES &&
                    i <= DSP_MAX_MAX_LABEL_LINES)
                DSP()->SetMaxLabelLines(i);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect LabelMaxLines: must be %d-%d.",
                    DSP_MIN_MAX_LABEL_LINES, DSP_MAX_MAX_LABEL_LINES);
                return (false);
            }
        }
        else
            DSP()->SetMaxLabelLines(DSP_DEF_MAX_LABEL_LINES);
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }

    bool
    evLabelHiddenMode(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= HLall && i <= HLnone)
                DSP()->SetHiddenLabelMode((HLmode)i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect LabelHiddenMode: must be integer 0-3.");
                return (false);
            }
        }
        else
            DSP()->SetHiddenLabelMode(HLall);
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }
}


//--------------------------------------------------------------------------
// View Menu Commands

namespace {
    bool
    evPeekSleepMsec(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0)
                DSP()->SetSleepTimeMs(i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect PeekSleepMsec: must be >= 0.");
                return (false);
            }
        }
        else
            DSP()->SetSleepTimeMs(DSP_SLEEP_TIME_MS);
        return (true);
    }

    bool
    evLockMode(const char*, bool set)
    {
        CD()->SetNoElectrical(false);
        if (set) {
            if (DSP()->CurMode() == Physical) {
                MenuEnt *ent = Menu()->FindEntry("view", MenuSCED);
                if (ent)
                    Menu()->SetSensitive(ent->cmd.caller, false);
                CD()->SetNoElectrical(true);
            }
            else {
                MenuEnt *ent = Menu()->FindEntry("view", MenuPHYS);
                if (ent)
                    Menu()->SetSensitive(ent->cmd.caller, false);
            }
        }
        else {
            if (DSP()->CurMode() == Physical) {
                MenuEnt *ent = Menu()->FindEntry("view", MenuSCED);
                if (ent)
                    Menu()->SetSensitive(ent->cmd.caller, true);
            }
            else {
                MenuEnt *ent = Menu()->FindEntry("view", MenuPHYS);
                if (ent)
                    Menu()->SetSensitive(ent->cmd.caller, true);
            }
        }
        return (true);
    }

    bool
    evXSectYScale(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) && d >= CDSCALEMIN && d <= CDSCALEMAX)
                ;
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect XSectYScale: range %g - %g.",
                    CDSCALEMIN, CDSCALEMAX);
                return (false);
            }
        }
        return (true);
    }
}


//--------------------------------------------------------------------------
// Attributes Menu Commands

namespace {
    bool
    evTechPrintDefaults(const char*, bool)
    {
        CDvdb()->registerPostFunc(postTech);
        return (true);
    }

    bool
    evEraseBehindProps(const char*, bool set)
    {
        DSP()->SetEraseBehindProps(set);
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }

    bool
    evPhysPropTextSize(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= DSP_MIN_PTRM_TXTHT &&
                    i <= DSP_MAX_PTRM_TXTHT)
                DSP()->SetPhysPropSize(i);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect PhysPropTextSize: range %d-%d.",
                    DSP_MIN_PTRM_TXTHT, DSP_MAX_PTRM_TXTHT);
                return (false);
            }
        }
        else
            DSP()->SetPhysPropSize(DSP_DEF_PTRM_TXTHT);
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }

    bool
    evEraseBehindTerms(const char *vstring, bool set)
    {
        if (set) {
            if (vstring && !strcmp(vstring, "all"))
                DSP()->SetEraseBehindTerms(ErbhAll);
            else
                DSP()->SetEraseBehindTerms(ErbhSome);
        }
        else
            DSP()->SetEraseBehindTerms(ErbhNone);
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }

    bool
    evTermTextSize(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= DSP_MIN_PTRM_TXTHT &&
                    i <= DSP_MAX_PTRM_TXTHT)
                DSP()->SetTermTextSize(i);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect TermTextSize: range %d-%d.",
                    DSP_MIN_PTRM_TXTHT, DSP_MAX_PTRM_TXTHT);
                return (false);
            }
        }
        else
            DSP()->SetTermTextSize(DSP_DEF_PTRM_TXTHT);
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }

    bool
    evTermMarkSize(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= DSP_MIN_PTRM_DELTA &&
                    i <= DSP_MAX_PTRM_DELTA)
                DSP()->SetTermMarkSize(i);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect TermMarkSize: range %d-%d.",
                    DSP_MIN_PTRM_DELTA, DSP_MAX_PTRM_DELTA);
                return (false);
            }
        }
        else
            DSP()->SetTermMarkSize(DSP_DEF_PTRM_DELTA);
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }

    bool
    evFullWinCursor(const char*, bool set)
    {
        DSPmainDraw(ShowGhost(ERASE))
        XM()->SetFullWinCursor(set);
        DSPmainDraw(ShowGhost(DISPLAY))
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }

    bool
    evCellThreshold(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= DSP_MIN_CELL_THRESHOLD &&
                    i <= DSP_MAX_CELL_THRESHOLD)
                DSP()->SetCellThreshold(i);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect CellThreshold: must be %d-%d.",
                    DSP_MIN_CELL_THRESHOLD, DSP_MAX_CELL_THRESHOLD);
                return (false);
            }
        }
        else
            DSP()->SetCellThreshold(DSP_DEF_CELL_THRESHOLD);
        CDvdb()->registerPostFunc(postAttr);
        return (true);
    }
}


//--------------------------------------------------------------------------
// Help System

namespace {
    bool
    evHelpMultiWin(const char*, bool set)
    {
        HLP()->set_multi_win(set);
        Menu()->MenuButtonSet("help", MenuMULTW, set);
        return (true);
    }
}

// End of variable actions
//--------------------------------------------------------------------------

namespace {
    // Global hook procedure to register.  This intercepts the "set"
    // operation, aborting if true is returned.
    //
    bool
    set_hook_proc(const char *name, const char *string)
    {
        if (*name == '@') {
            ScedIf()->setDevicePrpty(name+1, string);
            return (true);
        }
        return (false);
    }


    // Global hook procedure to register.  This intercepts the "get"
    // operation, which will return a non-null return from this
    // function instead of the saved string.
    //
    const char *
    get_hook_proc(const char *name)
    {
        static char *tstr;
        if (*name == '@') {
            delete [] tstr;
            tstr = ScedIf()->getDevicePrpty(name+1);
            return (tstr);
        }
        return (0);
    }
}


#define B 'b'
#define S 's'

namespace {
    void vsetup(const char *vname, char c, bool(*fn)(const char*, bool))
    {
        CDvdb()->registerInternal(vname,  fn);
        if (c == B)
            Tech()->RegisterBooleanAttribute(vname);
        else if (c == S)
            Tech()->RegisterStringAttribute(vname);
    }
}


// Register the internal variables and callbacks.  Called from
// constructor.
//
void
cMain::SetupVariables()
{
    CDvdb()->registerGlobalSetHook(set_hook_proc);
    CDvdb()->registerGlobalGetHook(get_hook_proc);

    // Database Setup
    vsetup(VA_DatabaseResolution,  0,   evDatabaseResolution);
    vsetup(VA_NetNamesCaseSens,    B,   0);
    vsetup(VA_Subscripting,        S,   0);
    vsetup(VA_DrfDebug,            B,   0);

    // Paths and Directories
    vsetup(VA_Path,                0,   0);
    vsetup(VA_LibPath,             0,   evLibPath);
    vsetup(VA_HelpPath,            0,   evHelpPath);
    vsetup(VA_ScriptPath,          0,   evScriptPath);
    vsetup(VA_NoReadExclusive,     B,   0);
    vsetup(VA_AddToBack,           B,   0);
    vsetup(VA_DocsDir,             S,   0);
    vsetup(VA_ProgramRoot,         S,   0);
    vsetup(VA_TeePrompt,           S,   evTeePrompt);

    // General Visual
    vsetup(VA_MouseWheel,          S,   evMouseWheel);
    vsetup(VA_NoCheckUpdate,       B,   0);
    vsetup(VA_ListPageEntries,     S,   evListPageEntries);
    vsetup(VA_NoLocalImage,        B,   evNoLocalImage);
    vsetup(VA_NoPixmapStore,       B,   evNoPixmapStore);
    vsetup(VA_NoDisplayCache,      B,   evNoDisplayCache);
    vsetup(VA_LowerWinOffset,      S,   evLowerWinOffset);
    vsetup(VA_PhysGridOrigin,      S,   evPhysGridOrigin);
    vsetup(VA_ScreenCoords,        B,   0);
    vsetup(VA_PixelDelta,          S,   evPixelDelta);
    vsetup(VA_NoPhysRedraw,        B,   evNoPhysRedraw);
    vsetup(VA_NoToTop,             B,   evNoToTop);

    // '!' Commands
    vsetup(VA_Shell,               S,   0);
    vsetup(VA_InstallCmdFormat,    S,   0);

    // Parameterized Cells
    vsetup(VA_PCellKeepSubMasters, B,   0);
    vsetup(VA_PCellListSubMasters, B,   0);
    vsetup(VA_PCellScriptPath,     0,   0);
    vsetup(VA_PCellShowAllWarnings,B,   0);

    // Standard Vias
    vsetup(VA_ViaKeepSubMasters,   B,   0);
    vsetup(VA_ViaListSubMasters,   B,   0);

    // Scripts
    vsetup(VA_LogIsLog10,          B,   evLogIsLog10);

    // Selections
    vsetup(VA_MarkInstanceOrigin,  B,   evMarkInstanceOrigin);
    vsetup(VA_MarkObjectCentroid,  B,   evMarkObjectCentroid);
    vsetup(VA_SelectTime,          S,   0);
    vsetup(VA_NoAltSelection,      B,   0);
    vsetup(VA_MaxBlinkingObjects,  S,   evMaxBlinkingObjects);

    // File Menu - Printing
    vsetup(VA_NoAskFileAction,     B,   evNoAskFileAction);
    vsetup(VA_DefaultPrintCmd,     S,   evDefaultPrintCmd);
    vsetup(VA_NoDriverLabels,      B,   0);
    vsetup(VA_PSlineWidth,         S,   evPSlineWidth);
    vsetup(VA_RmTmpFileMinutes,    S,   evRmTmpFileMinutes);

    // Cell Menu Commands
    vsetup(VA_ContextDarkPcnt,     S,   evContextDarkPcnt);

    // Editing General
    vsetup(VA_AskSaveNative,       B,   0);
    vsetup(VA_NoFixRot45,          B,   0);

    // Side Menu Commands
    vsetup(VA_LabelDefHeight,      S,   evLabelDefHeight);
    vsetup(VA_LabelMaxLen,         S,   evLabelMaxLen);
    vsetup(VA_LabelMaxLines,       S,   evLabelMaxLines);
    vsetup(VA_LabelHiddenMode,     S,   evLabelHiddenMode);
    // Rid these (temp. back compat.)
    vsetup("DefLabelHeight",       S,   evLabelDefHeight);
    vsetup("MaxLabelLen",          S,   evLabelMaxLen);
    vsetup("MaxLabelLines",        S,   evLabelMaxLines);
    vsetup("HiddenLabelMode",      S,   evLabelHiddenMode);

    // View Menu Commands
    vsetup(VA_InfoInternal,        B,   0);
    vsetup(VA_PeekSleepMsec,       S,   evPeekSleepMsec);
    vsetup(VA_LockMode,            B,   evLockMode);
    vsetup(VA_XSectNoAutoY,        B,   0);
    vsetup(VA_XSectYScale,         S,   evXSectYScale);

    // Attributes Menu Commands
    vsetup(VA_TechNoPrintPatMap,   B,   0);
    vsetup(VA_TechPrintDefaults,   S,   evTechPrintDefaults);
    vsetup(VA_EraseBehindProps,    B,   evEraseBehindProps);
    vsetup(VA_PhysPropTextSize,    S,   evPhysPropTextSize);
    vsetup(VA_EraseBehindTerms,    S,   evEraseBehindTerms);
    vsetup(VA_TermTextSize,        S,   evTermTextSize);
    vsetup(VA_TermMarkSize,        S,   evTermMarkSize);
    vsetup(VA_FullWinCursor,       B,   evFullWinCursor);
    vsetup(VA_CellThreshold,       S,   evCellThreshold);

    // Help System
    vsetup(VA_HelpMultiWin,        B,   evHelpMultiWin);
    vsetup(VA_HelpDefaultTopic,    S,   0);
}


// If force_show is true, pop up a list of the variables currently
// set.  Otherwise, the listing will be updated if it is already
// displayed.
//
void
cMain::PopUpVariables(bool force_show)
{
    static int info_id = -1;
    if (force_show || DSPmainWbagRet(PopUpHTMLinfo(MODE_UPD, 0)) == info_id) {

        stringlist *list = CDvdb()->listVariables();
        // List is sorted, each element is name[ value].

        sLstr lstr;
        int numvars = stringlist::length(list);
        if (numvars == 0)
            lstr.add("<h2>There are no variables currently set.</h2>");
        else {
            if (numvars > 1) {
                lstr.add("<h2>There are ");
                lstr.add_i(numvars);
                lstr.add(" variables currently set:</h2>\n");
            }
            else
                lstr.add("<h2>There is 1 variable currently set:</h2>\n");
            lstr.add("<table border=0><tr><th>Name</th><th>Value</th></tr>\n");
            for (stringlist *s = list; s; s = s->next) {
                char *t = s->string;
                char *name = lstring::gettok(&t);
                char *value = t;

                if (HLP()->is_keyword(name)) {
                    lstr.add("<tr><td><a href=\"");
                    lstr.add(name);
                    lstr.add("\"><tt>");
                    lstr.add(name);
                    lstr.add("</tt></a></td><td>");
                }
                else {
                    lstr.add("<tr><td><tt>");
                    lstr.add(name);
                    lstr.add("</tt></td><td>");
                }
                delete [] name;

                if (value && *value ) {
                    lstr.add("<tt>");
                    for (const char *p = value; *p; p++) {
                        if (*p == '<')
                            lstr.add("&#60;");
                        else
                            lstr.add_c(*p);
                    }
                    lstr.add("</tt></td></tr>\n");
                }
                else
                    lstr.add("</td></tr>\n");
            }
            lstr.add("</table>");
        }
        info_id = DSPmainWbagRet(PopUpHTMLinfo(MODE_ON, lstr.string()));
        stringlist::destroy(list);
    }
}

