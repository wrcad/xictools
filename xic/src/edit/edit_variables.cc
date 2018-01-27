
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

#include "main.h"
#include "fio.h"
#include "edit.h"
#include "pcell.h"
#include "cvrt.h"
#include "undolist.h"
#include "menu.h"
#include "edit_menu.h"
#include "geo_zlist.h"
#include "geo_grid.h"
#include "dsp_inlines.h"
#include "tech.h"
#include "errorlog.h"
#include "cvrt_variables.h"
#include "select.h"
#include "events.h"


//--------------------------------------------------------------------------
// Variables

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
    postset_es(const char*)
    {
        ED()->PopUpEditSetup(0, MODE_UPD);
    }

    void
    postset_jn(const char*)
    {
        ED()->PopUpJoin(0, MODE_UPD);
    }

    void
    postset_lx(const char*)
    {
        ED()->PopUpLayerExp(0, MODE_UPD);
    }

    void
    postset_pc(const char*)
    {
        ED()->PopUpPCellCtrl(0, MODE_UPD);
    }

    void
    postset_pl(const char*)
    {
        ED()->PopUpPlace(MODE_UPD, true);
    }

    void
    postset_fl(const char*)
    {
        ED()->PopUpFlatten(0, MODE_UPD, 0, 0, 0, false);
    }
}


//--------------------
//  Editing General

namespace {
    bool
    evConstrain45(const char*, bool set)
    {
        Tech()->SetConstrain45(set);
        CDvdb()->registerPostFunc(postset_es);
        return (true);
    }

    bool
    evNoMergeObjects(const char*, bool set)
    {
        CD()->SetNoMergeObjects(set);
        CDvdb()->registerPostFunc(postset_es);
        return (true);
    }

    bool
    evNoMergePolys(const char*, bool set)
    {
        CD()->SetNoMergePolys(set);
        CDvdb()->registerPostFunc(postset_es);
        return (true);
    }

    bool
    evAskSaveNative(const char*, bool)
    {
        CDvdb()->registerPostFunc(postset_es);
        return (true);
    }

    bool
    evSafeClipping(const char*, bool set)
    {
        // This is now for debugging only, recompile geo_zlfuncs.cc.
        GEO()->setUseSclFuncs(set);
        return (true);
    }

    bool
    evNoFixRot45(const char*, bool set)
    {
        sTT::set_nofix45(set);
        return (true);
    }
}


//--------------------
// Side Menu Commands

namespace {
    void
    postLogo(const char*)
    {
        ED()->PopUpLogo(0, MODE_UPD);
        ED()->logoUpdate();
    }

    void
    postLogoPolytext(const char*)
    {
        ED()->PopUpPolytextFont(0, MODE_UPD);
        ED()->PopUpLogo(0, MODE_UPD);
        ED()->logoUpdate();
    }

    bool
    evMasterMenuLength(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 1 && i <= 75)
                ;
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect MasterMenuLength: range 1-75.");
                return (false);
            }
        }
        CDvdb()->registerPostFunc(postset_pl);
        return (true);
    }

    bool
    evLogoEndStyle(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && (i >= 0 && i <= 2))
                ;
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect LogoEndStyle: must be integer 0-2.");
                return (false);
            }
        }
        CDvdb()->registerPostFunc(postLogo);
        return (true);
    }

    bool
    evLogoPathWidth(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && (i >= 1 && i <= 5))
                ;
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect LogoPathWidth: must be integer 1-5.");
                return (false);
            }
        }
        CDvdb()->registerPostFunc(postLogo);
        return (true);
    }

    bool
    evLogoAltFont(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && (i >= 0 && i <= 1))
                ;
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect LogoAltFont: must be integer 0-1.");
                return (false);
            }
        }
        CDvdb()->registerPostFunc(postLogo);
        return (true);
    }

    bool
    evLogoPixelSize(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) && (d > 0 && d <= 100.0))
                ;
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect LogoPixelSize: must be positive <= 100.0.");
                return (false);
            }
        }
        CDvdb()->registerPostFunc(postLogo);
        return (true);
    }

    bool
    evLogoPrettyFont(const char *vstring, bool set)
    {
        if (set) {
            if (!vstring || !*vstring) {
                Log()->ErrorLog(mh::Variables,
                    "LogoPrettyFont: must be a font name string.");
                return (false);
            }
        }
        CDvdb()->registerPostFunc(postLogoPolytext);
        return (true);
    }


    bool
    evLogoToFile(const char*, bool)
    {
        CDvdb()->registerPostFunc(postLogo);
        return (true);
    }


    bool
    evSpotSize(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) && d >= 0.0 && d <= 1.0) {
                if (d < 0.01)
                    d = 0.0;
                GEO()->setSpotSize(INTERNAL_UNITS(d));
            }
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect SpotSize: range 0 - 1.0.");
                return (false);
            }
        }
        else
            GEO()->setSpotSize(-1);  // Defaults to MfgGrid.
        return (true);
    }
}


//--------------------
// Edit/Modify Menu Commands

namespace {
    bool
    evUndoListLength(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 1000)
                Ulist()->SetUndoLength(i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect UndoListLength: must be 0-1000.");
                return (false);
            }
        }
        else
            Ulist()->SetUndoLength(DEF_MAX_UNDO_LEN);
        CDvdb()->registerPostFunc(postset_es);
        return (true);
    }

    bool
    evMaxGhostDepth(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 8) {
                DSPmainDraw(ShowGhost(ERASE))
                EGst()->setMaxGhostDepth(i);
                DSPmainDraw(ShowGhost(DISPLAY))
            }
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect MaxGhostDepth: must be %d-%d.", 0, 8);
                return (false);
            }
        }
        else {
            DSPmainDraw(ShowGhost(ERASE))
            EGst()->setMaxGhostDepth(-1);
            DSPmainDraw(ShowGhost(DISPLAY))
        }
        CDvdb()->registerPostFunc(postset_es);
        return (true);
    }

    bool
    evMaxGhostObjects(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 50 && i <= 50000) {
                DSPmainDraw(ShowGhost(ERASE))
                EGst()->setMaxGhostObjects(i);
                DSPmainDraw(ShowGhost(DISPLAY))
            }
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect MaxGhostObjects: must be 50-50000.");
                return (false);
            }
        }
        else {
            DSPmainDraw(ShowGhost(ERASE))
            EGst()->setMaxGhostObjects(DEF_MAX_GHOST_OBJECTS);
            DSPmainDraw(ShowGhost(DISPLAY))
        }
        CDvdb()->registerPostFunc(postset_es);
        return (true);
    }

    bool
    evNoWireWidthMag(const char*, bool set)
    {
        ED()->setNoWireWidthMag(set);
        CDvdb()->registerPostFunc(postset_es);
        return (true);
    }

    bool
    evCrCellOverwrite(const char*, bool)
    {
        CDvdb()->registerPostFunc(postset_es);
        return (true);
    }

    bool
    evLayerChangeMode(const char *vstring, bool set)
    {
        if (set) {
            if (vstring && lstring::cieq(vstring, "all"))
                ED()->setLchgMode(LCHGall);
            else
                ED()->setLchgMode(LCHGcur);
        }
        else
            ED()->setLchgMode(LCHGnone);
        return (true);
    }

    bool
    evJoinMaxPolyVerts(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && (i == 0 || (i >= 20 && i <= 8000)))
                Zlist::JoinMaxVerts = i;
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect JoinMaxPolyVerts: 0 or range 20-8000.");
                return (false);
            }
        }
        else
            Zlist::JoinMaxVerts = DEF_JoinMaxVerts;
        CDvdb()->registerPostFunc(postset_jn);
        return (true);
    }

    bool
    evJoinMaxPolyGroup(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0)
                Zlist::JoinMaxGroup = i;
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect JoinMaxPolyGroup: must be 0 or larger.");
                return (false);
            }
        }
        else
            Zlist::JoinMaxGroup = DEF_JoinMaxGroup;
        CDvdb()->registerPostFunc(postset_jn);
        return (true);
    }

    bool
    evJoinMaxPolyQueue(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0)
                Zlist::JoinMaxQueue = i;
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect JoinMaxPolyQueue: must be 0 or larger.");
                return (false);
            }
        }
        else
            Zlist::JoinMaxQueue = DEF_JoinMaxQueue;
        CDvdb()->registerPostFunc(postset_jn);
        return (true);
    }

    bool
    evJoinBreakClean(const char*, bool set)
    {
        Zlist::JoinBreakClean = set;
        CDvdb()->registerPostFunc(postset_jn);
        return (true);
    }

    bool
    evJoinSplitWires(const char*, bool set)
    {
        Zlist::JoinSplitWires = set;
        CDvdb()->registerPostFunc(postset_jn);
        return (true);
    }

    bool
    evPartitionSize(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) && (INTERNAL_UNITS(d) == 0 ||
                    (INTERNAL_UNITS(d) >= INTERNAL_UNITS(GRD_PART_MIN) &&
                    INTERNAL_UNITS(d) <= INTERNAL_UNITS(GRD_PART_MAX))))
                grd_t::set_def_gridsize(INTERNAL_UNITS(d));
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect PartitionSize: must be 0 or %.2f-%.2f.",
                    GRD_PART_MIN, GRD_PART_MAX);
                return (false);
            }
        }
        else
            grd_t::set_def_gridsize(INTERNAL_UNITS(DEF_GRD_PART_SIZE));
        CDvdb()->registerPostFunc(postset_lx);
        return (true);
    }

    bool
    evNoFlattenStdVias(const char*, bool set)
    {
        ED()->setNoFlattenStdVias(set);
        FIO()->SetNoFlattenStdVias(set);
        CDvdb()->registerPostFunc(postset_fl);
        return (true);
    }

    bool
    evNoFlattenPCells(const char*, bool set)
    {
        ED()->setNoFlattenPCells(set);
        FIO()->SetNoFlattenPCells(set);
        CDvdb()->registerPostFunc(postset_fl);
        return (true);
    }

    bool
    evThreads(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= DSP_MIN_THREADS &&
                    i <= DSP_MAX_THREADS)
                DSP()->SetNumThreads(i);
            else {
                Log()->ErrorLogV(mh::Variables,
                    "Incorrect Threads: range %d-%d.",
                    DSP_MIN_THREADS, DSP_MAX_THREADS);
                return (false);
            }
        }
        else
            DSP()->SetNumThreads(DSP_DEF_THREADS);
        CDvdb()->registerPostFunc(postset_lx);
        return (true);
    }
}


// Parameterized Cells
//

namespace {
    bool
    evPCellAbutMode(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 2)
                ED()->setPCellAbutMode((AbutMode)i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect PCellAbutMode: must be 0-2.");
                return (false);
            }
        }
        else
            ED()->setPCellAbutMode(AbutMode1);
        CDvdb()->registerPostFunc(postset_pc);
        return (true);
    }

    bool
    evPCellHideGrips(const char*, bool set)
    {
        ED()->setHideGrips(set);
        if (set)
            ED()->unregisterGrips(0);
        else
            ED()->registerGrips(0);
        CDs *psd = CurCell(Physical);
        sSelGen sg(Selections, psd, "c");
        CDc *cd;
        while ((cd = (CDc*)sg.next()) != 0) {
            if (set) {
                ED()->unregisterGrips(cd);
                Selections.showUnselected(psd, cd);
                Selections.showSelected(psd, cd);
            }
            else
                ED()->registerGrips(cd);
        }
        CDvdb()->registerPostFunc(postset_pc);
        return (true);
    }

    bool
    evPCellGripInstSize(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 1000)
                DSP()->SetFenceInstPixSize(i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect PCellGripInstSize: must be 0-1000.");
                return (false);
            }
        }
        else
            DSP()->SetFenceInstPixSize(DSP_MIN_FENCE_INST_PIXELS);
        CDvdb()->registerPostFunc(postset_pc);
        return (true);
    }

    bool
    evPCellListSubMasters(const char*, bool set)
    {
        FIO()->SetListPCellSubMasters(set);
        CDvdb()->registerPostFunc(postset_pc);
        return (true);
    }

    bool
    evPCellShowAllWarnings(const char*, bool set)
    {
        FIO()->SetShowAllPCellWarnings(set);
        CDvdb()->registerPostFunc(postset_pc);
        return (true);
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


void
cEdit::setupVariables()
{
    // Editing General
    vsetup(VA_Constrain45,          B,  evConstrain45);
    vsetup(VA_NoMergeObjects,       B,  evNoMergeObjects);
    vsetup(VA_NoMergePolys,         B,  evNoMergePolys);
    vsetup(VA_AskSaveNative,        B,  evAskSaveNative);
    vsetup("scldebug",              B,  evSafeClipping);  // for debugging
    vsetup(VA_NoFixRot45,           B,  evNoFixRot45);

    // Side Menu Commands
    vsetup(VA_MasterMenuLength,     S,  evMasterMenuLength);
    vsetup(VA_LogoEndStyle,         S,  evLogoEndStyle);
    vsetup(VA_LogoPathWidth,        S,  evLogoPathWidth);
    vsetup(VA_LogoAltFont,          S,  evLogoAltFont);
    vsetup(VA_LogoPixelSize,        S,  evLogoPixelSize);
    vsetup(VA_LogoPrettyFont,       S,  evLogoPrettyFont);
    vsetup(VA_LogoToFile,           B,  evLogoToFile);
    vsetup(VA_NoConstrainRound,     B,  0);
    vsetup(VA_SpotSize,             S,  evSpotSize);

    // Edit/Modify Menu Commands
    vsetup(VA_UndoListLength,       S,  evUndoListLength);
    vsetup(VA_MaxGhostDepth,        S,  evMaxGhostDepth);
    vsetup(VA_MaxGhostObjects,      S,  evMaxGhostObjects);
    vsetup(VA_NoWireWidthMag,       B,  evNoWireWidthMag);
    vsetup(VA_CrCellOverwrite,      B,  evCrCellOverwrite);
    vsetup(VA_LayerChangeMode,      S,  evLayerChangeMode);
    vsetup(VA_JoinMaxPolyVerts,     S,  evJoinMaxPolyVerts);
    vsetup(VA_JoinMaxPolyGroup,     S,  evJoinMaxPolyGroup);
    vsetup(VA_JoinMaxPolyQueue,     S,  evJoinMaxPolyQueue);
    vsetup(VA_JoinBreakClean,       B,  evJoinBreakClean);
    vsetup(VA_JoinSplitWires,       B,  evJoinSplitWires);
    vsetup(VA_PartitionSize,        S,  evPartitionSize);
    vsetup(VA_NoFlattenStdVias,     B,  evNoFlattenStdVias);
    vsetup(VA_NoFlattenPCells,      B,  evNoFlattenPCells);
    vsetup(VA_Threads,              S,  evThreads);

    // Parameterized Cells
    vsetup(VA_PCellAbutMode,        S,  evPCellAbutMode);
    vsetup(VA_PCellHideGrips,       B,  evPCellHideGrips);
    vsetup(VA_PCellGripInstSize,    S,  evPCellGripInstSize);
    vsetup(VA_PCellListSubMasters,  B,  evPCellListSubMasters);
    vsetup(VA_PCellShowAllWarnings, B,  evPCellShowAllWarnings);
}

