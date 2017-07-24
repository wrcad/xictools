
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: ext_path.cc,v 5.47 2017/03/14 01:26:38 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_fh.h"
#include "ext_antenna.h"
#include "fio.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "events.h"
#include "select.h"
#include "promptline.h"
#include "errorlog.h"
#include "tech.h"
#include "menu.h"


//-----------------------------------------------------------------------------
// The PATHS and QPATH commands.
//
// Highlight conducting paths in the display.

// These commands are similar, this struct is base for both.
//
namespace {
    namespace ext_path {
        struct sPcmd : public CmdState
        {
            sPcmd(const char *nm, const char *hk) : CmdState(nm, hk)
                {
                    Caller = 0;
                    UseGroups = false;
                }

            virtual ~sPcmd() { }

            void message()
                { PL()->ShowPrompt(
                    "Click on conducting object to show path."); }

            void b1down() { cEventHdlr::sel_b1down(); }
            void b1up();
            void esc();
            bool key(int, const char*, int);

        protected:
            void clear_path();
            void new_path(BBox*);

            void find_path(BBox *AOI)
                {
                    pathfinder *pf = EX()->pathFinder(cExt::PFget);
                    if (pf) {
                        pf->clear();
                        pf->set_depth(EX()->pathDepth());
                        pf->find_path(AOI);
                    }
                }

            GRobject Caller;
            BBox LastAOI;
            BBox BB1, BB2;
            bool UseGroups;
        };

        // The Path command.  Clicking on an object on a Conductor layer will
        // cause all objects electrically connected to the pointed-to object
        // to be highlighted.  Only one path is shown at a given time.
        //
        struct PathState : public sPcmd
        {
            friend void cExt::selectPath(GRobject);
            friend void cExt::redrawPath();
            friend void cExt::showCurrentPath(WindowDesc*, bool);
            friend bool cExt::saveCurrentPathToFile(const char*, bool);
            friend bool cExt::getAntennaPath();

            PathState(const char*, const char*);
            virtual ~PathState();

            void setCaller(GRobject c)  { Caller = c; }
        };

        PathState *PathCmd;

        // The Qpath command.  Clicking on an object on a Conductor layer will
        // cause all objects electrically connected to the pointed-to object
        // to be highlighted.  Only one path is shown at a given time.
        //
        struct QpathState : public sPcmd
        {
            friend void cExt::selectPathQuick(GRobject);
            friend void cExt::redrawPath();
            friend void cExt::showCurrentPath(WindowDesc*, bool);
            friend bool cExt::saveCurrentPathToFile(const char*, bool);
            friend bool cExt::getAntennaPath();

            QpathState(const char*, const char*);
            virtual ~QpathState();

            void setCaller(GRobject c)  { Caller = c; }

            static int work_proc(void*);
        };

        QpathState *QpathCmd;
    }
}

using namespace ext_path;


namespace {
    int exec_p_idle(void *arg)
    {
        GRobject caller = (GRobject)arg;
        CDs *psdesc = CurCell(Physical);
        if (!psdesc) {
            Menu()->Deselect(caller);
            return (false);
        }
        if (!EX()->extract(psdesc)) {
            PL()->ShowPrompt("Extraction failed!");
            Menu()->Deselect(caller);
            return (false);
        }
        cGroupDesc *gd = psdesc->groups();
        if (!gd || gd->isempty()) {
            PL()->ShowPrompt("No conductor groups found!");
            Menu()->Deselect(caller);
            return (false);
        }

        Selections.deselectTypes(CurCell(), 0);

        PathCmd = new PathState("PATH", "xic:exsel");
        PathCmd->setCaller(caller);
        if (!EV()->PushCallback(PathCmd)) {
            delete PathCmd;
            return (false);
        }
        PathCmd->message();
        return (false);
    }


    int
    exec_q_idle(void *arg)
    {
        GRobject caller = (GRobject)arg;
        CDs *psdesc = CurCell(Physical);
        if (!psdesc) {
            Menu()->Deselect(caller);
            return (false);
        }

        Selections.deselectTypes(CurCell(), 0);

        QpathCmd = new QpathState("QPATH", "xic:exsel");
        QpathCmd->setCaller(caller);
        if (!EV()->PushCallback(QpathCmd)) {
            delete QpathCmd;
            return (false);
        }
        QpathCmd->message();
        return (false);
    }
}

using namespace ext_path;


void
cExt::selectPath(GRobject caller)
{
    if (!caller)
        return;
    bool state = Menu()->GetStatus(caller);
    if (!state) {
        if (PathCmd)
            PathCmd->esc();
        return;
    }

    if (QpathCmd)
        QpathCmd->esc();

    if (!XM()->CheckCurCell(false, false, Physical)) {
        Menu()->Deselect(caller);
        return;
    }

    EV()->InitCallback();

    // Launch the state in an idle proc, so present command (if any)
    // will exit cleanly.

    dspPkgIf()->RegisterIdleProc(exec_p_idle, caller);
}


void
cExt::selectPathQuick(GRobject caller)
{
    if (!caller)
        return;
    bool state = Menu()->GetStatus(caller);
    if (!state) {
        if (QpathCmd)
            QpathCmd->esc();
        return;
    }
    if (PathCmd)
        PathCmd->esc();

    if (!XM()->CheckCurCell(false, false, Physical)) {
        Menu()->Deselect(caller);
        return;
    }

    EV()->InitCallback();

    // Launch the state in an idle proc, so present command (if any)
    // will exit cleanly.

    dspPkgIf()->RegisterIdleProc(exec_q_idle, caller);
}


// Redraw the path, callback from widget when depth changes.
//
void
cExt::redrawPath()
{
    if (pathFinder(PFget)) {
        if (PathCmd) {
            PathCmd->clear_path();
            PathCmd->new_path(&PathCmd->LastAOI);
        }
        else if (QpathCmd) {
            QpathCmd->clear_path();
            QpathCmd->new_path(&QpathCmd->LastAOI);
        }
    }
}


// Display or erase the highlighting applied to objects in the
// current path.  This handles both the path and qpath versions.
//
void
cExt::showCurrentPath(WindowDesc *wdesc, bool d_or_e)
{
    if (ext_pathfinder)
        ext_pathfinder->show_path(wdesc, d_or_e);
}


// Save the current path to a native cell file.  If include_vias is true,
// the needed vias will be recognized using the current cell hierarchy
// and added to output.  For this to work, the current cell and the
// path database must be in sync.
//
bool
cExt::saveCurrentPathToFile(const char *name, bool include_vias)
{
    if (ext_pathfinder && ext_pathfinder->path_table() &&
            ext_pathfinder->path_table()->allocated()) {

        CDo *od = ext_pathfinder->get_object_list();
        if (!od)
            return (false);
        CDo *odv = 0;
        XIrt xrt = XIok;
        if (include_vias) {
            const char *vstr = CDvdb()->getVariable(VA_PathFileVias);
            odv = ext_pathfinder->get_via_list(od, &xrt, (vstr && *vstr));
        }
        if (xrt == XIok) {
            SymTab *tab = new SymTab(false, false);
            while (od) {
                SymTabEnt *h = SymTab::get_ent(tab,
                    (unsigned long)od->ldesc());
                if (!h) {
                    tab->add((unsigned long)od->ldesc(), 0, false);
                    h = SymTab::get_ent(tab, (unsigned long)od->ldesc());
                }
                CDo *on = od->next_odesc();
                od->set_next_odesc(0);
                h->stData = new CDol(od, (CDol*)h->stData);
                od = on;
            }
            while (odv) {
                SymTabEnt *h = SymTab::get_ent(tab,
                    (unsigned long)odv->ldesc());
                if (!h) {
                    tab->add((unsigned long)odv->ldesc(), 0, false);
                    h = SymTab::get_ent(tab, (unsigned long)odv->ldesc());
                }
                CDo *on = odv->next_odesc();
                odv->set_next_odesc(0);
                h->stData = new CDol(odv, (CDol*)h->stData);
                odv = on;
            }
            if (!FIO()->TabToNative(tab, name))
                xrt = XIbad;

            SymTabGen gen(tab, true);
            SymTabEnt *h;
            while ((h = gen.next()) != 0) {
                CDol *ol = (CDol*)h->stData;
                while (ol) {
                    CDol *ox = ol;
                    ol = ol->next;
                    delete ox->odesc;
                    delete ox;
                }
                delete h;
            }
            delete tab;
        }

        if (xrt != XIok) {
            Log()->ErrorLogV(mh::Processing, "Write cell failed:\n%s",
                Errs()->get_error());
            return (false);
        }
        return (true);
    }
    return (false);
}


// This will prompt for a net number used in an antenna.log file, and
// select that net.
//
bool
cExt::getAntennaPath()
{
    if (PathCmd) {
        int netnum;
        BBox refBB;
        if (!ant_pathfinder::read_file(&netnum, &refBB))
            return (false);

        PathCmd->clear_path();
        PathCmd->new_path(&refBB);
        PathCmd->LastAOI = refBB;
    }
    return (true);
}


// Interface to the path finder.
//
pathfinder *
cExt::pathFinder(PFenum pfmode)
{
    if (pfmode != PFget) {
        if (pfmode == PFinitQuick) {
            delete ext_pathfinder;
            ext_pathfinder = new pathfinder;
        }
        else if (pfmode == PFinit) {
            delete ext_pathfinder;
            ext_pathfinder = new grp_pathfinder;
        }
        else if (pfmode == PFclear) {
            delete ext_pathfinder;
            ext_pathfinder = 0;
        }
    }
    return (ext_pathfinder);
}
// End of cExtfunctions.


void
sPcmd::b1up()
{
    pathfinder *pf = EX()->pathFinder(cExt::PFget);
    if (!pf)
        return;

    BBox AOI;
    if (!cEventHdlr::sel_b1up(&AOI, 0, B1UP_NOSEL))
        return;
    if (AOI.left == AOI.right || AOI.bottom == AOI.top) {
        WindowDesc *wdesc = EV()->ButtonWin();
        if (!wdesc)
            return;
        double delta = 1 + DSP()->PixelDelta()/wdesc->Ratio();
        AOI.bloat((int)delta);
    }

    if (EX()->isSubpathEnabled()) {
        // The user can display a sub-path by clicking on two
        // highlighted objects in the current path.  Only the part of
        // the path that connects the two objects will be highlighted.

        CDo *odesc = pf->find_and_remove(&AOI);
        if (odesc) {
            if (EV()->Cursor().get_downstate() & GR_SHIFT_MASK) {
                pf->show_path(0, ERASE);
                pf->show_path(0, DISPLAY);
                Level = 0;
                return;
            }
            if (pf->insert(odesc) == pathfinder::PFerror)
                return;
            if (Level == 0) {
                BB1 = AOI;
                Level = 1;
                PL()->ShowPrompt("Saved initial sub-path object.");
            }
            else if (Level == 1) {
                PL()->ShowPrompt("Finding sub-path...");
                BB2 = AOI;
                Level = 0;
                dspPkgIf()->SetWorking(true);
                pf->show_path(0, ERASE);
                pf_ordered_path *path = pf->extract_subpath(&BB1, &BB2);
                if (path) {
                    if (pf->load(path))
                        PL()->ShowPrompt("Showing sub-path.");
                    else
                        PL()->ShowPrompt("Internal error!\n");
                    delete path;
                }
                else
                    PL()->ShowPrompt(Errs()->get_error());
                pf->show_path(0, DISPLAY);
                dspPkgIf()->SetWorking(false);
            }
            return;
        }
    }

    Level = 0;
    clear_path();
    new_path(&AOI);
    LastAOI = AOI;
}


void
sPcmd::esc()
{
    cEventHdlr::sel_esc();
    Menu()->Deselect(Caller);
    EV()->PopCallback(this);
    WindowDesc *wd;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wd = wgen.next()) != 0)
        EX()->showCurrentPath(wd, ERASE);
    PL()->ErasePrompt();
    delete this;
}


// Handle expansion depth changes.
//
bool
sPcmd::key(int code, const char *text, int)
{
    pathfinder *pf = EX()->pathFinder(cExt::PFget);
    if (!pf)
        return (false);

    if (text && *text) {
        if (*text == 'h') {
            EX()->setBlinkSelections(!EX()->isBlinkSelections());
            EX()->PopUpSelections(0, MODE_UPD);
            pf->show_path(0, ERASE);
            pf->show_path(0, DISPLAY);
            return (true);
        }
        if (*text == '+') {
            if (code == NUPLUS_KEY)
                return (false);
            if (EX()->pathDepth() < CDMAXCALLDEPTH) {
                EX()->setPathDepth(EX()->pathDepth() + 1);
                EX()->PopUpSelections(0, MODE_UPD);
            }
            return (true);
        }
        if (*text == '-') {
            if (code == NUMINUS_KEY)
                return (false);
            if (EX()->pathDepth() > 0) {
                EX()->setPathDepth(EX()->pathDepth() - 1);
                EX()->PopUpSelections(0, MODE_UPD);
            }
            return (true);
        }
        if (*text == 'a') {
            EX()->setPathDepth(CDMAXCALLDEPTH);
            EX()->PopUpSelections(0, MODE_UPD);
            return (true);
        }
        if (*text == 'n') {
            EX()->setPathDepth(0);
            EX()->PopUpSelections(0, MODE_UPD);
            return (true);
        }
        if (isdigit(*text)) {
            // Ignore numgers in this mode.
            return (true);
        }
        if (*text == 's') {
            char buf[256];
            sprintf(buf, "%s_grp_%s", Tstring(DSP()->CurCellName()),
                pf->pathname());

            char *in = PL()->EditPrompt("Native cell file name? ", buf);
            if (in && *in) {
                // Maybe write out the vias, too.
                const char *vstr = CDvdb()->getVariable(VA_PathFileVias);
                if (EX()->saveCurrentPathToFile(in, (vstr != 0)))
                    PL()->ShowPrompt("Path saved to native cell file.");
                else
                    PL()->ShowPrompt("Operation failed.");
            }
            return (true);
        }
        if (*text == 't') {
            pf->show_path(0, ERASE);
            pf->atomize_path();
            pf->show_path(0, DISPLAY);
            return (true);
        }
        if (*text == 'f') {
            EX()->getAntennaPath();
            return (true);
        }
    }
    return (false);
}


void
sPcmd::clear_path()
{
    pathfinder *pf = EX()->pathFinder(cExt::PFget);
    if (pf) {
        pf->show_path(0, ERASE);
        pf->clear();
    }
}


void
sPcmd::new_path(BBox *AOI)
{
    pathfinder *pf = EX()->pathFinder(cExt::PFget);
    if (!pf)
        return;

    dspPkgIf()->SetWorking(true);
    pf->clear();
    pf->set_depth(EX()->pathDepth());
    pf->find_path(AOI);
    if (pf->is_empty())
        message();
    else {
        pf->show_path(0, DISPLAY);
        int ndgt = CD()->numDigits();
        PL()->ShowPromptV("Showing path intersecting %.*f,%.*f %.*f,%.*f.",
            ndgt, MICRONS(AOI->left), ndgt, MICRONS(AOI->bottom),
            ndgt, MICRONS(AOI->right), ndgt, MICRONS(AOI->top));
    }
    dspPkgIf()->SetWorking(false);
}
// End of sPcmd functions


PathState::PathState(const char *nm, const char *hk) : sPcmd(nm, hk)
{
    UseGroups = true;
    EX()->pathFinder(cExt::PFinit);
}


PathState::~PathState()
{
    PathCmd = 0;
    EX()->pathFinder(cExt::PFclear);
}
// End of PathState functions.


QpathState::QpathState(const char *nm, const char *hk) : sPcmd(nm, hk)
{
    UseGroups = false;
    EX()->pathFinder(cExt::PFinitQuick);
    dspPkgIf()->RegisterIdleProc(work_proc, 0);
}


QpathState::~QpathState()
{
    if (EX()->quickPathMode() != QPnone)
        EX()->activateGroundPlane(false);
    QpathCmd = 0;
    EX()->pathFinder(cExt::PFclear);
}


// Static function.
// Work procedure to do ground plane setup.
//
int
QpathState::work_proc(void*)
{
    if (Tech()->IsInvertGroundPlane() && EX()->quickPathMode() == QPcreate) {
        CDl *ld;
        CDs *cursdp = CurCell(Physical);
        if (!cursdp)
            return (false);
        CDlgen gen(Physical);
        while ((ld = gen.next()) != 0) {
            if (ld->isGroundPlane() && ld->isDarkField()) {
                EX()->setupGroundPlane(cursdp);
                QpathCmd->message();
                break;
            }
        }
    }
    return (false);
}

