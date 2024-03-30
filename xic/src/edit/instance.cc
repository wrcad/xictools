
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

#include "config.h"
#include "main.h"
#include "edit.h"
#include "oa_if.h"
#include "pcell.h"
#include "pcell_params.h"
#include "undolist.h"
#include "scedif.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_layer.h"
#include "cd_lgen.h"
#include "cd_propnum.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "fio.h"
#include "fio_chd.h"
#include "fio_gencif.h"
#include "fio_library.h"
#include "fio_alias.h"
#include "events.h"
#include "menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "ghost.h"
#include "miscutil/pathlist.h"
#include "miscutil/texttf.h"


/**************************************************************************
 *
 * Functions to place/replace cell instances, and set arraying.
 *
 **************************************************************************/


//-----------------------------------------------------------------------------
// Place command

namespace {
    inline bool is_empty(CDcbin *cbin)
    {
        CDs *sdesc = cbin->celldesc(DSP()->CurMode());
        if (!sdesc)
            return (true);
        if (sdesc->isEmpty() && !sdesc->prpty(XICP_PC_SCRIPT))
            return (true);
        return (false);
    }


    namespace ed_place {
        struct PlaceState : public CmdState
        {
            friend void cEdit::placeExec(CmdDesc*);

            PlaceState(const char*, const char*);
            virtual ~PlaceState();

            void setup(GRobject c, bool fg, bool fsm)
                {
                    Caller = c;
                    FromGUI = fg;
                    ForceSmash = fsm;
                }

            void message()
                {
                    PL()->ShowPrompt(ED()->replacing() ? msgr : msg1);
                }
            bool smash_mode()
                {
                    return (ForceSmash || (FromGUI && ED()->plIsSmashMode()));
                }
            bool no_snap() { return (NoSnapToTerm); }

            static CDcbin Sym;  // Symbol being placed

            void b1down();
            void b1up();
            void esc();
            bool key(int, const char*, int);
            void undo() { cEventHdlr::sel_undo(); }
            void redo() {  cEventHdlr::sel_redo(); }

        private:
            static int doit_idle(void*);

            GRobject Caller;    // initiating button
            bool DoIt;
            bool NoSnapToTerm;  // Don't snap to terminals in electrical mode.
            bool FromGUI;       // Initiated from GUI.
            bool ForceSmash;    // Assert smash mode.
            bool IsPCell;       // Instance of a PCell.

            static const char *msg1;
            static const char *msgr;
        };

        PlaceState *PlaceCmd;
    }
}

using namespace ed_place;

const char *PlaceState::msg1 =
    "Click on locations to place cell, press Esc to quit.";
const char *PlaceState::msgr =
    "Select subcells to replace, press Esc to quit.";

CDcbin PlaceState::Sym;


PlaceState::PlaceState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    DoIt = false;
    NoSnapToTerm = false;
    FromGUI = false;
    ForceSmash = false;
    IsPCell = false;
}


PlaceState::~PlaceState()
{
    PlaceCmd = 0;
    ED()->stopPlacement();
}


// Button 1 down callback for place mode.  If not replacing, create a
// new instance or array at to pointed-to location.  Otherwise begin
// a selection operation for the replace.
//
void
PlaceState::b1down()
{
    DoIt = true;
    if (ED()->replacing())
        cEventHdlr::sel_b1down();
}


namespace {
    int
    timeout(void*)
    {
        DSPmainDraw(ShowGhost(DISPLAY))
        return (false);
    }
}


// Button 1 up callback for place mode.  If replacing, replace the
// instances returned from the selection with the current master.
//
void
PlaceState::b1up()
{
    DSPpkg::self()->RegisterIdleProc(doit_idle, 0);
}


// Exit code, called if Esc is pressed.
//
void
PlaceState::esc()
{
    Gst()->SetGhost(GFnone);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    if (Caller)
        MainMenu()->Deselect(Caller);
    if (ED()->replacing())
        cEventHdlr::sel_esc();
    ScedIf()->DevsEscCallback();
    ED()->plEscCallback();
    delete this;
}


bool
PlaceState::key(int code, const char*, int)
{
    switch (code) {
    case SHIFTDN_KEY:
    case CTRLDN_KEY:
        DSPmainDraw(ShowGhost(ERASE))
        NoSnapToTerm = true;
        DSPmainDraw(ShowGhost(DISPLAY))
        break;
    case SHIFTUP_KEY:
    case CTRLUP_KEY:
        DSPmainDraw(ShowGhost(ERASE))
        NoSnapToTerm = false;
        DSPmainDraw(ShowGhost(DISPLAY))
        break;
    case RETURN_KEY:
        DSPmainDraw(ShowGhost(ERASE))
        switch (ED()->instanceRef()) {
        case PL_ORIGIN:
        default:
            ED()->setInstanceRef(PL_LL);
            ED()->PopUpPlace(MODE_UPD, false);
            break;
        case PL_LL:
            ED()->setInstanceRef(PL_UL);
            ED()->PopUpPlace(MODE_UPD, false);
            break;
        case PL_UL:
            ED()->setInstanceRef(PL_UR);
            ED()->PopUpPlace(MODE_UPD, false);
            break;
        case PL_UR:
            ED()->setInstanceRef(PL_LR);
            ED()->PopUpPlace(MODE_UPD, false);
            break;
        case PL_LR:
            ED()->setInstanceRef(PL_ORIGIN);
            ED()->PopUpPlace(MODE_UPD, false);
            break;
        }
        DSPmainDraw(ShowGhost(DISPLAY))
        break;
    default:
        return (false);
    }
    return (true);
}


namespace {
    // In the master list, a two-token entry is used to indicate a
    // foreign pcell (from OpenAccess).  Unlike native pcells, these
    // super-masters can not be imported into Xic.  This function
    // returns a databae name form if the passed name has two tokens.
    //
    char *db_name_from(const char *name)
    {
        const char *s = name;
        char *lname = lstring::gettok(&s);
        char *cname = lstring::gettok(&s);
        char *dbname = 0;
        if (cname)
            dbname = PCellDesc::mk_dbname(lname, cname, "layout");
        delete [] lname;
        delete [] cname;
        return (dbname);
    }
}


// Static function.
// Idle proc to do the work.
//
int
PlaceState::doit_idle(void*)
{
    if (!PlaceCmd)
        return (0);
    if (!PlaceCmd->DoIt)
        return (0);
    PlaceCmd->DoIt = false;

    if (PlaceCmd->IsPCell) {
        char *dbname = db_name_from(ED()->plGetMasterName());
        DSPmainDraw(ShowGhost(ERASE))
        bool ret = ED()->resolvePCell(&Sym, dbname);
        DSPmainDraw(ShowGhost(DISPLAY))
        delete [] dbname;

        if (!ret) {
            Errs()->add_error("Instance placement failed");
            Log()->ErrorLog(mh::CellPlacement, Errs()->get_error());
            return (0);
        }
    }

    if (!PlaceCmd)
        return (0);
    if (!Sym.celldesc(DSP()->CurMode()))
        return (0);

    ED()->placeAction();
    return (0);
}
// End of PlaceState methods


// Menu command for the Place function.  Pop up the Place entry panel,
// and attach the current master to the pointer.  New cell instances
// will be located where the user points, except in replace mode where
// existing cell instances pointed to will be replaced with the current
// master.
//
void
cEdit::placeExec(CmdDesc *cmd)
{
    if (PlaceCmd)
        PlaceCmd->esc();
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;

    PopUpPlace(MODE_ON, false);
    PlaceCmd = new PlaceState("PLACE", "xic:place");
    PlaceCmd->setup(cmd ? cmd->caller : 0, true, false);
    if (!EV()->PushCallback(PlaceCmd)) {
        delete PlaceCmd;
        return;
    }
    PlaceCmd->message();

    PlaceState::Sym.reset();
    const char *mname = plGetMasterName();
    if (mname) {
        char *dbname = db_name_from(mname);
        if (dbname && PC()->findSuperMaster(dbname)) {
            // OA pcell.
            if (!ED()->startPlacement(0, dbname)) {
                Log()->ErrorLogV(mh::CellPlacement,
                    "Failed to initialize PCell placement for %s:\n%s",
                    mname, Errs()->get_error());
                PlaceCmd->IsPCell = false;
                PlaceCmd->esc();
            }
            else {
                PlaceCmd->IsPCell = true;
                Gst()->SetGhost(GFplace);
            }
        }
        else if (getCurrentMaster(&PlaceState::Sym)) {
            // Warn about an empty cell
            if (is_empty(&PlaceState::Sym)) {
                Log()->WarningLogV(mh::CellPlacement,
                    "%s part of specified master is empty, can't place.",
                    DSP()->CurMode() == Physical ? "physical" : "electrical");
                PlaceCmd->esc();
            }
            else {
                CDs *sd = PlaceState::Sym.celldesc(DSP()->CurMode());
                if (sd->isPCellSuperMaster()) {
                    if (!ED()->startPlacement(&PlaceState::Sym, 0)) {
                        Log()->ErrorLogV(mh::CellPlacement,
                            "Failed to initialize PCell placement for %s:\n%s",
                            mname, Errs()->get_error());
                        PlaceCmd->IsPCell = false;
                        PlaceCmd->esc();
                    }
                    else {
                        PlaceCmd->IsPCell = true;
                        Gst()->SetGhost(GFplace);
                    }
                }
                else {
                    PlaceCmd->IsPCell = false;
                    Gst()->SetGhost(GFplace);
                }
            }
        }
        delete [] dbname;
    }
    ds.clear();
}


// Action procedure for the place command.
//
void
cEdit::placeAction()
{
    CDs *cursd = CurCell();
    if (!cursd)
        return;

    if (replacing()) {
        char types[4];
        types[0] = CDINSTANCE;
        types[1] = '\0';
        BBox AOI;
        CDol *slist;
        if (cEventHdlr::sel_b1up(&AOI, types, &slist) && slist) {
            bool didone = false;
            for (CDol *s = slist; s; s = s->next) {
                CDc *cdesc = (CDc*)s->odesc;
                CDs *msdesc = cdesc->masterCell();
                if (!msdesc)
                    continue;
                if (msdesc->isDevice() != PlaceState::Sym.isDevice())
                    continue;
                Ulist()->ListCheck("replace", cursd, false);
                Errs()->init_error();
                if (replaceInstance(cdesc, &PlaceState::Sym, true, useArray()))
                    didone = true;
                else {
                    Errs()->add_error("ReplaceCell failed");
                    Log()->ErrorLog(mh::CellPlacement, Errs()->get_error());
                }
            }
            if (didone)
                Ulist()->CommitChanges(true);
            CDol::destroy(slist);
        }
        Gst()->SetGhost(GFplace);
    }
    else {
        if (EV()->Cursor().is_release_ok()) {
            Ulist()->ListCheck(PlaceCmd->Name(), cursd, false);
            int x, y;
            EV()->Cursor().get_xy(&x, &y);
            if (!PlaceCmd->no_snap()) {
                CDs *sdesc = PlaceState::Sym.elec();
                find_contact(sdesc, &x, &y, false);
            }
            if (new_instance(cursd, x, y, arrayParams(), instanceRef(),
                    PlaceCmd->smash_mode())) {
                Ulist()->CommitChanges(true);
                DSPmainDraw(ShowGhost(false))
                DSPpkg::self()->RegisterTimeoutProc(500, timeout, 0);
            }
            Gst()->RepaintGhost();
        }
    }
}


namespace {
    // Wrapper to pass to cFIO::OpenImport.
    //
    void
    add_cell_cb(const char *name, OIcbData *cbdata)
    {
        if (!cbdata)
            return;
        if (!name) {
            // Called from pop-up destructor.
            delete cbdata;
            return;
        }
        if (cbdata->lib_filename)
            ED()->addMaster(cbdata->lib_filename, name);
        else
            ED()->addMaster(name, 0);
    }
}


// This adds mname to the master list.  If the Place popup is not
// visible, it will be popped.  If mname is an archive, cname is the
// cell to open.  If chd is given, then mnamein is ignored.
//
void
cEdit::addMaster(const char *mnamein, const char *cname, cCHD *chd)
{
    if (!plIsActive())
        PopUpPlace(MODE_ON, true);
    if (plIsActive()) {
        CDcbin cbin;
        Errs()->init_error();
        if (mnamein && cname && OAif()->hasOA()) {
            // Check if cell is a foreign super-master from OA.
            bool is_open;
            bool is_oalib = OAif()->is_library(mnamein, &is_open);
            if (is_oalib && !is_open) {
                // The library exists but is not open, open it
                // temporarily here.

                OAif()->set_lib_open(mnamein, true);
            }
            bool in_lib = false;
            if (is_oalib && !OAif()->is_cell_in_lib(mnamein, cname, &in_lib)) {
                Errs()->add_error("Error opening %s", mnamein);
                Log()->ErrorLog(mh::CellPlacement, Errs()->get_error());
                OAif()->set_lib_open(mnamein, is_open);
                return;
            }
            if (in_lib) {
                PCellParam *p0 = 0;
                const char *new_cell_name;
                bool ok = OAif()->load_cell(mnamein, cname, 0, CDMAXCALLDEPTH,
                        false, &new_cell_name, &p0);
                OAif()->set_lib_open(mnamein, is_open);
                if (!ok) {
                    Log()->ErrorLog(mh::CellPlacement, Errs()->get_error());
                    return;
                }
                if (p0) {
                    // The master is a foreign pcell.

                    // Make sure super-master is in table.
                    char *dbname = PC()->addSuperMaster(mnamein, cname,
                        "layout", p0);
                    PCellParam::destroy(p0);
                    delete [] dbname;

                    sLstr lstr;
                    lstr.add(mnamein);
                    lstr.add_c(' ');
                    lstr.add(cname);
                    plAddMenuEnt(lstr.string());
                    setCurrentMaster(0);
                    return; 
                }
                // We've opened the cell, avoid doing it again below.
                CDcdb()->findSymbol(new_cell_name, &cbin);
            }
        }

        OItype oiret = OIok;
        char *mname = 0;
        OIcbData *cbdata = 0;
        if (!cbin.phys() && !cbin.elec()) {
            if (chd)
                mnamein = chd->filename();
            else {
                chd = CDchd()->chdRecall(mnamein, false);
                if (chd) {
                    // Source name was a CHD in memory, which is allowed.
                    mnamein = chd->filename();
                }
            }
            if (!mnamein)
                return;
            mname = pathlist::expand_path(mnamein, false, true);

            cbdata = new OIcbData;
            oiret = FIO()->OpenImport(mname, FIO()->DefReadPrms(), cname,
                chd, &cbin, 0, add_cell_cb, cbdata);
        }

        if (oiret == OIerror) {
            Errs()->add_error("Error opening %s", mname);
            Log()->ErrorLog(mh::CellPlacement, Errs()->get_error());
            delete [] mname;
            delete cbdata;
            return;
        }
        if (oiret == OIaborted) {
            PL()->ShowPrompt("Aborted.");
            delete [] mname;
            delete cbdata;
            return;
        }
        if (oiret == OIambiguous) {
            // The user opened an archive file with multiple top level
            // cells.  OpenImport has launched a popup for the user to
            // choose which one to edit.
            delete [] mname;
            // The cbdata is deleted in the callback.
            return;
        }

        delete cbdata;

        if (oiret == OInew) {
            Log()->ErrorLogV(mh::CellPlacement, "Cell %s was not found.",
                cname ? cname : mname);
            // delete empty cell
            CD()->Close(CD()->CellNameTableFind(cname ? cname : mname));
            delete [] mname;
            return;
        }
        delete [] mname;
        if (cbin.isLibrary() && cbin.isDevice())
            // library device, don't add to list
            return;
        if (cbin.elec() && !cbin.isDevice() && oiret != OIold)
            ScedIf()->checkElectrical(&cbin);

        // Note that mname is the file name for gds/cif, we want the
        // actual cell name.
        plAddMenuEnt(Tstring(cbin.cellname()));
        setCurrentMaster(0);
    }
}


// Special simplified version which allows addition of an instance to
// any cell.  Also, this allows an offset to be added to the placement
// before the transform.
//
CDc *
cEdit::makeInstance(CDs *sdesc, const char *name, int x, int y, int offx,
    int offy)
{
    CDcbin cbin;
    if (!CDcdb()->findSymbol(lstring::strip_path(name), &cbin) &&
            OIfailed(FIO()->FromNative(name, &cbin, 1.0)))
        return (0);

    CDcbin tcbin(PlaceState::Sym);
    PlaceState::Sym = cbin;

    CDc *cdesc = new_instance(sdesc, x, y, iap_t(), PL_ORIGIN, false,
        offx, offy);
    PlaceState::Sym = tcbin;
    return (cdesc);
}


// Temporarily enter place mode, and attach name as the master.  This
// provides support for the Place script function.
//
// If smash is set, the placement operation will flatten the cell into
// the parent.
//
CDc *
cEdit::placeInstance(const char *name, int x, int y, int nx, int ny,
    int spx, int spy, PLref ref, bool smash, pcpMode pmode)
{
    CDs *cursd = CurCell();
    if (!cursd || cursd->isSymbolic())
        return (0);
    CDc *cdesc = 0;
    if (nx < 1 || cursd->isElectrical())
        nx = 1;
    if (ny < 1 || cursd->isElectrical())
        ny = 1;

    // Split the name into two tokens.  If there are two, the first
    // token can be several things:  CHD, archive file, OA library,
    // etc.  We want to support all of that here.
    //
    const char *s = name;
    char *lname = lstring::getqtok(&s);
    char *cname = lstring::gettok(&s);
    GCarray<char*> gc_lname(lname);
    GCarray<char*> gc_cname(cname);

    CDcbin cbin;
    if (cname && OAif()->hasOA()) {
        // Check if cell is a foreign super-master from OA.
        bool is_oalib;
        if (!OAif()->is_library(lname, &is_oalib)) {
            Errs()->add_error("Error opening %s", lname);
            return (0);
        }
        bool in_lib = false;
        if (is_oalib && !OAif()->is_cell_in_lib(lname, cname, &in_lib)) {
            Errs()->add_error("Error opening %s", lname);
            return (0);
        }
        if (in_lib && DSP()->CurMode() == Physical) {
            PCellParam *p0 = 0;
            if (!OAif()->load_cell(lname, cname, 0, CDMAXCALLDEPTH,
                    false, 0, &p0)) {
                Errs()->add_error("Error opening %s", lname);
                return (0);
            }
            if (p0) {
                // The master is a foreign pcell.

                // Make sure super-master is in table.
                char *dbname = PC()->addSuperMaster(lname, cname,
                    "layout", p0);
                PCellParam::destroy(p0);
                bool ret = ED()->startPlacement(0, dbname, pmode);
                if (!ret || !ED()->resolvePCell(&cbin, dbname)) {
                    Errs()->add_error("Instance placement failed");
                    delete [] dbname;
                    return (0);
                }
                delete [] dbname;

                CDcbin tcbin(PlaceState::Sym);
                PlaceState::Sym = cbin;
                cdesc = new_instance(cursd, x, y, iap_t(nx, ny, spx, spy),
                    ref, false);
                PlaceState::Sym = tcbin;
                ED()->stopPlacement();
                return (cdesc);
            }
        }
    }
    if (!openCell(lname, &cbin, cname))
        return (0);  // Caller should look for error messages.
    if (cbin.phys() && cbin.phys()->isPCellSuperMaster()) {
        // We've opened a native pcell super-master.
        bool ret = ED()->startPlacement(&cbin, 0, pmode);
        if (!ret || !ED()->resolvePCell(&cbin, 0)) {
            Errs()->add_error("Instance placement failed");
            return (0);
        }

        CDcbin tcbin(PlaceState::Sym);
        PlaceState::Sym = cbin;
        cdesc = new_instance(cursd, x, y, iap_t(nx, ny, spx, spy), ref, false);
        PlaceState::Sym = tcbin;
        ED()->stopPlacement();
        return (cdesc);
    }

    CDcbin tcbin(PlaceState::Sym);
    PlaceState::Sym = cbin;
    cdesc = new_instance(cursd, x, y, iap_t(nx, ny, spx, spy), ref, smash);
    PlaceState::Sym = tcbin;
    return (cdesc);
}


// This function is called when device popup buttons are pressed.  Set
// the current master to the library device and enter place mode, but
// do not pop up the Place panel.
//
// If smash is set, the placement operation will flatten the cell into
// the parent.
//
void
cEdit::placeDev(GRobject caller, const char *name, bool smash)
{
    if (PlaceCmd)
        PlaceCmd->esc();
    if (noEditing())
        return;
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;

    if (!name || !*name)
        return;

    CDcbin cbin;
    if (!openCell(name, &cbin, 0)) {
        Log()->ErrorLog(mh::CellPlacement, Errs()->get_error());
        return;
    }
    PlaceCmd = new PlaceState("PLACE", "xic:devs");
    PlaceCmd->setup(caller, false, smash);
    PlaceState::Sym = cbin;
    if (!EV()->PushCallback(PlaceCmd)) {
        delete PlaceCmd;
        PlaceCmd = 0;
        PlaceState::Sym.reset();
        return;
    }
    PlaceCmd->message();
    Gst()->SetGhost(GFplace);
}


// Replace the existing cdesc with an instance of newcbin (the part
// with mode matching sdesc).  The current transform is added to the
// existing transform for the new instance if add_cur_xform is true
// and sdesc is the current cell.  The master's origin or one of the
// corners is used as the reference point in Physical mode.  In
// Electrical mode, the reference terminal of the replacing cell is
// placed at the reference terminal location of the original cell. 
// true is returned on success, false otherwise, with an error message
// in the status string.
//
bool
cEdit::replaceInstance(CDc *cdesc, CDcbin *newcbin, bool add_cur_xform,
    bool use_array_prms)
{
    Errs()->init_error();
    if (!cdesc->master()) {
        Errs()->add_error("Internal: replacement target not linked.");
        return (false);
    }
    CDs *sdesc = cdesc->parent();
    if (!sdesc) {
        Errs()->add_error("Internal: %s not resolved to parent.",
            cdesc->cellname() ? Tstring(cdesc->cellname()) : "??");
        return (false);
    }
    CDs *msdesc = cdesc->masterCell(true);
    if (!msdesc) {
        Errs()->add_error("Internal: %s not resolved to master.",
            cdesc->cellname() ? Tstring(cdesc->cellname()) : "??");
        return (false);
    }

    CallDesc calldesc(newcbin->cellname(),
        newcbin->celldesc(sdesc->displayMode()));
    if (!calldesc.celldesc()) {
        Errs()->add_error("Internal: master %s is null.", newcbin->cellname());
        return (false);
    }

    int xn = 0;
    int yn = 0;
    int xo = 0;
    int yo = 0;
    get_reference_handle(calldesc.celldesc(), &xn, &yn, instanceRef(),
        arrayParams(), 0);
    CDap ap(cdesc);
    iap_t oldiap;
    if (ap.nx > 1 || ap.ny > 1) {
        oldiap.set_nx(ap.nx);
        oldiap.set_ny(ap.ny);
        const BBox *BB = msdesc->BB();
        oldiap.set_spx(ap.dx - BB->width());
        oldiap.set_spy(ap.dy - BB->height()); 
    }
    get_reference_handle(msdesc, &xo, &yo, instanceRef(), oldiap, cdesc);
    cTfmStack stk;
    stk.TPush();
    stk.TApplyTransform(cdesc);
    stk.TPoint(&xn, &yn);
    stk.TPoint(&xo, &yo);
    if (add_cur_xform && sdesc == CurCell())
        // apply transform in current cell only
        GEO()->applyCurTransform(&stk, xn, yn, xo, yo);
    else
        stk.TTranslate(xo-xn, yo-yn);
    CDtx tx;
    stk.TCurrent(&tx);
    stk.TPop();
    if (use_array_prms) {
        const BBox *BB = calldesc.celldesc()->BB();
        if (BB->right > BB->left && BB->top > BB->bottom) {
            // Don't array unless BB is valid.
            ap = CDap(arrayParams().nx(), arrayParams().ny(),
                BB->width() + arrayParams().spx(),
                BB->height() + arrayParams().spy());
        }
    }
    CDc *newcdesc;
    if (OIfailed(sdesc->makeCall(&calldesc, &tx, &ap, CDcallNone, &newcdesc)))
        return (false);

    Ulist()->RecordObjectChange(sdesc, cdesc, newcdesc);
    if (sdesc->isElectrical()) {
        // keep any existing assigned name
        CDp_cname *pon = (CDp_cname*)cdesc->prpty(P_NAME);
        CDp_cname *pnn = (CDp_cname*)newcdesc->prpty(P_NAME);
        if (pon && pnn && pon->assigned_name())
            pnn->set_assigned_name(pon->assigned_name());
        ScedIf()->genDeviceLabels(newcdesc, cdesc, false);
    }
    return (true);
}


// Set the current master to the cell given.  Called from the Place
// panel.
//
void
cEdit::setCurrentMaster(CDcbin *cbin)
{
    if (PlaceCmd && PlaceState::Sym.cellname() &&
            !is_empty(&PlaceState::Sym)) {
        PL()->AbortEdit();  // Get out of text editor.
        Gst()->SetGhost(GFnone);
    }
    if (cbin)
        PlaceState::Sym = *cbin;
    else
        getCurrentMaster(&PlaceState::Sym);
    if (PlaceCmd && PlaceState::Sym.cellname()) {
        if (!is_empty(&PlaceState::Sym))
            Gst()->SetGhost(GFplace);
        else
            Log()->WarningLogV(mh::CellPlacement,
                "%s part of specified master is empty.",
                DSP()->CurMode() == Physical ? "physical" : "electrical");
    }
}


// Return the cell for the current master, or 0 if not around.
//
bool
cEdit::getCurrentMaster(CDcbin *cbret)
{
    if (cbret)
        cbret->reset();
    const char *mname = plGetMasterName();
    if (mname) {
        char *dbname = db_name_from(mname);
        if (dbname && PC()->findSuperMaster(dbname)) {
            delete [] dbname;
            return (true);
        }
        delete [] dbname;

        CDcbin cbin;
        Errs()->init_error();
        if (OIfailed(CD()->OpenExisting(mname, &cbin))) {
            Errs()->add_error("Error opening %s", mname);
            Log()->ErrorLog(mh::CellPlacement, Errs()->get_error());
            // delete bad name from master list
            plRemoveMenuEnt(mname);
            return (false);
        }
        if (cbret)
            *cbret = cbin;
        return (true);
    }
    return (false);
}


void
cEdit::plSetParams(PLref ref, int nx, int ny, int spx, int spy)
{
    if (DSP()->CurMode() == Physical)
        setArrayParams(iap_t(nx, ny, spx, spy));
    setInstanceRef(ref);
    PopUpPlace(MODE_UPD, false);
}


void
cEdit::plInitMenuLen()
{
    const char *s = CDvdb()->getVariable(VA_MasterMenuLength);
    if (!s)
        ed_menu_len = DEF_PLACE_MENU_LEN;
    else {
        ed_menu_len = atoi(s);
        if (ed_menu_len < 1 || ed_menu_len > 75)
            ed_menu_len = DEF_PLACE_MENU_LEN;
    }

    // Resize list if necessary.
    int i = 0;
    for (stringlist *pd = ed_menu_head; pd; pd = pd->next) {
        i++;
        if (i == ed_menu_len) {
            if (pd->next) {
                stringlist::destroy(pd->next);
                pd->next = 0;
                if (ed_popup)
                    ed_popup->rebuild_menu();
            }
            break;
        }
    }
}


// Create a new menu entry for name.  New names are added to the front
// of the list.  The list length is maintained by dropping names from
// the end.
//
void
cEdit::plAddMenuEnt(const char *name)
{
    stringlist *pd, *lp = 0;
    int i = 0;
    for (pd = ed_menu_head; pd; lp = pd, pd = pd->next) {
        if (!strcmp(pd->string, name))
            break;
        i++;
    }
    if (!pd) {
        // new entry
        if (i == ed_menu_len) {
            i -= 2;
            for (pd = ed_menu_head; i && pd; pd = pd->next, i--) ;
            lp = pd->next;
            pd->next = 0;
            delete [] lp->string;
            lp->string = lstring::copy(name);
            lp->next = ed_menu_head;
            ed_menu_head = lp;
        }
        else
            ed_menu_head = new stringlist(lstring::copy(name), ed_menu_head);
    }
    else if (lp) {
        // move to front of list
        lp->next = pd->next;
        pd->next = ed_menu_head;
        ed_menu_head = pd;
    }
    if (ed_popup)
        ed_popup->rebuild_menu();
}


void
cEdit::plChangeMenuEnt(const char *newname, const char *oldname)
{
    for (stringlist *pd = ed_menu_head; pd; pd = pd->next) {
        if (!strcmp(oldname, pd->string)) {
            delete [] pd->string;
            pd->string = lstring::copy(newname);
            if (ed_menu_head == pd)
                setCurrentMaster(0);
            if (ed_popup)
                ed_popup->rebuild_menu();
            break;
        }
    }
}


// Remove a menu entry by name.
//
void
cEdit::plRemoveMenuEnt(const char *name)
{
    if (!name || !*name)
        return;
    stringlist *p, *lp = 0;
    int i;
    for (p = ed_menu_head, i = 0; p; lp = p, p = p->next) {
        if (!strcmp(name, p->string))
            break;
        i++;
    }
    if (!p || i >= ed_menu_len)
        return;
    if (lp)
        lp->next = p->next;
    else
        ed_menu_head = p->next;
    delete [] p->string;
    delete p;

    if (ed_popup)
        ed_popup->rebuild_menu();
}


const char *
cEdit::plGetMasterName()
{
    return (ed_menu_head ? ed_menu_head->string : 0);
}


// This is called by the Apply button in the Parameters pop-up.  It
// recreates the current sub-master, reflecting any parameter changes.
//
void
cEdit::plUpdateSubMaster()
{
    if (PlaceCmd) {
        DSPmainDraw(ShowGhost(ERASE))
        char *dbname = db_name_from(plGetMasterName());
        bool ret = ED()->resolvePCell(&PlaceState::Sym, dbname);
        delete [] dbname;
        DSPmainDraw(ShowGhost(DISPLAY))
        if (!ret) {
            Log()->ErrorLogV(mh::PCells, "Failed to update sub-master:\n%s",
                Errs()->get_error());
        }
    }
}


// Return the coordinates in the master to be used as the reference
// point for placement.  In electrical cells, the reference terminal
// provides the coordinate, unless there are no terminals in which
// case ref is used.
//
void
cEdit::get_reference_handle(CDs *sdesc, int *x, int *y, PLref ref,
    const iap_t &iap, const CDc *cdinst)
{
    *x = 0;
    *y = 0;

    if (sdesc->isElectrical()) {
        // Index 0 is the reference point.
        CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            if (pn->index() == 0) {
                if (sdesc->symbolicRep(cdinst)) {
                    if (!pn->get_pos(0, x, y))
                        continue;
                }
                else
                    pn->get_schem_pos(x, y);
                return;
            }
        }
    }

    const BBox *sBB = sdesc->BB();
    if (sBB->right < sBB->left || sBB->top < sBB->bottom)
        return;
    BBox BB = *sBB;
    if (iap.nx() > 1)
        BB.right += (iap.nx()-1)*(BB.width() + iap.spx());
    if (iap.ny() > 1)
        BB.top += (iap.ny()-1)*(BB.height() + iap.spy());

    if (ref == PL_LL) {
        *x = mmRnd(BB.left*GEO()->curTx()->magn());
        *y = mmRnd(BB.bottom*GEO()->curTx()->magn());
    }
    else if (ref == PL_UL) {
        *x = mmRnd(BB.left*GEO()->curTx()->magn());
        *y = mmRnd(BB.top*GEO()->curTx()->magn());
    }
    else if (ref == PL_UR) {
        *x = mmRnd(BB.right*GEO()->curTx()->magn());
        *y = mmRnd(BB.top*GEO()->curTx()->magn());
    }
    else if (ref == PL_LR) {
        *x = mmRnd(BB.right*GEO()->curTx()->magn());
        *y = mmRnd(BB.bottom*GEO()->curTx()->magn());
    }
    if (sdesc->isElectrical()) {
        // Move to reference point to the nearest grid point,
        // otherwise device terminals will be off-grid.
        EV()->CurrentWin()->Snap(x, y);
    }
}


// This only works for new instances of sdesc, where the instantiation
// will have a symbolic view solely due to the sdesc.
//
bool
cEdit::find_contact(CDs *sdesc, int *new_x, int *new_y, bool indicate)
{
    if (DSP()->CurMode() != Electrical)
        return (false);
    if (!sdesc)
        return (false);

    bool found = false;
    CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
    cTfmStack stk;
    int xx, yy;
    get_reference_handle(sdesc, &xx, &yy, instanceRef(), iap_t(), 0);
    stk.TPush();
    GEO()->applyCurTransform(&stk, xx, yy, *new_x, *new_y);
    bool issym = (sdesc->symbolicRep(0) != 0);
    WindowDesc *wdesc = EV()->CurrentWin() ?
        EV()->CurrentWin() : DSP()->MainWdesc();
    int delta = 1 + (int)(DSP()->PixelDelta()/wdesc->Ratio());
    int box_delta = 1 + (int)(10.0/wdesc->Ratio());
    bool fndx = false, fndy = false;
    for ( ; pn; pn = pn->next()) {
        for (unsigned int ix = 0; ; ix++) {
            int x, y;
            if (issym) {
                if (!pn->get_pos(ix, &x, &y))
                    break;
            }
            else {
                if (ix)
                    break;
                pn->get_schem_pos(&x, &y);
            }
            stk.TPoint(&x, &y);
            int nx, ny, flg;
            if (wdesc->FindContact(x, y, &nx, &ny, &flg,
                    found ? 0 : delta, 0, false)) {
                if (!fndx && (flg & FC_CX)) {
                    *new_x += nx - x;
                    fndx = true;
                }
                if (!fndy && (flg & FC_CY)) {
                    *new_y += ny - y;
                    fndy = true;
                }
                if (indicate) {
                    BBox BB;
                    BB.left = nx - box_delta;
                    BB.bottom = ny - box_delta;
                    BB.right = nx + box_delta;
                    BB.top = ny + box_delta;
                    DSPmainDraw(SetLinestyle(DSP()->BoxLinestyle()))
                    Gst()->ShowGhostBox(BB.left, BB.bottom,
                        BB.right, BB.top);
                    DSPmainDraw(SetLinestyle(0))
                }
                found = true;
            }
        }
    }

    CDp_bsnode *pb = (CDp_bsnode*)sdesc->prpty(P_BNODE);
    for ( ; pb; pb = pb->next()) {
        for (unsigned int ix = 0; ; ix++) {
            int x, y;
            if (issym) {
                if (!pb->get_pos(ix, &x, &y))
                    break;
            }
            else {
                if (ix)
                    break;
                pb->get_schem_pos(&x, &y);
            }
            stk.TPoint(&x, &y);
            int nx, ny, flg;
            if (wdesc->FindBterm(x, y, &nx, &ny, &flg,
                    found ? 0 : delta, 0, false)) {
                if (!fndx && (flg & FC_CX)) {
                    *new_x += nx - x;
                    fndx = true;
                }
                if (!fndy && (flg & FC_CY)) {
                    *new_y += ny - y;
                    fndy = true;
                }
                if (indicate) {
                    BBox BB;
                    BB.left = nx - box_delta;
                    BB.bottom = ny - box_delta;
                    BB.right = nx + box_delta;
                    BB.top = ny + box_delta;
                    DSPmainDraw(SetLinestyle(DSP()->BoxLinestyle()))
                    Gst()->ShowGhostBox(BB.left, BB.bottom,
                        BB.right, BB.top);
                    DSPmainDraw(SetLinestyle(0))
                }
                found = true;
            }
        }
    }

    stk.TPop();
    return (found);
}


// Create a new instance of the master at x, y taking into account the
// current array parameters, and current transform.
//
CDc*
cEdit::new_instance(CDs *sdesc, int x, int y, const iap_t &iap,
    PLref ref, bool smash, int offx, int offy)
{
    if (!PlaceState::Sym.cellname())
        return (0);
    if (!sdesc)
        return (0);
    if (sdesc->isSymbolic()) {
        PL()->ShowPrompt("Can't add instance to symbolic view.");
        return (0);
    }

    CDs *plsd = (sdesc->isElectrical() ?
        PlaceState::Sym.elec() : PlaceState::Sym.phys());
    if (!plsd)
        return (0);

     // This will add a name property if needed.
     plsd->elecCellType();

    // If we are adding a hierarchy to a native cell, set the Save
    // Native flags of all cells in the hierarchy so that we don't
    // forget to save them as native cells.
    if (sdesc->fileType() == Fnative &&
            (PlaceState::Sym.fileType() == Fcgx ||
            PlaceState::Sym.fileType() == Fcif ||
            PlaceState::Sym.fileType() == Fgds ||
            PlaceState::Sym.fileType() == Foas))
        PlaceState::Sym.setSaveNative();

    // Note that if adding an internall-generated (Fnone) cell, the
    // SaveNative flag should already be set for cells we want to
    // keep.

    Errs()->init_error();
    CallDesc calldesc;
    calldesc.setName(PlaceState::Sym.cellname());
    calldesc.setCelldesc(plsd);
    const BBox *BB = calldesc.celldesc()->BB();
    CDap ap;
    if (BB->right > BB->left && BB->top > BB->bottom) {
        // Don't array unless BB is valid.
        ap = CDap(iap.nx(), iap.ny(),
            BB->width() + iap.spx(), BB->height() + iap.spy());

        // Don't allow arraying to same location.
        if (ap.nx > 1 && ap.dx == 0)
            ap.nx = 1;
        if (ap.ny > 1 && ap.dy == 0)
            ap.ny = 1;
    }
    int xx, yy;
    get_reference_handle(calldesc.celldesc(), &xx, &yy, ref, iap, 0);
    xx += offx;
    yy += offy;
    cTfmStack stk;
    stk.TPush();
    stk.TSetMagn(GEO()->curTx()->magn());
    GEO()->applyCurTransform(&stk, xx, yy, x, y);
    CDtx tx;
    stk.TCurrent(&tx);
    stk.TPop();
    Errs()->init_error();
    CDc *cdesc;
    OItype oitmp = OIok;
    if (plsd->prpty(XICP_PC_SCRIPT) && (iap.nx() > 1 || iap.ny() > 1)) {
        // If this is a pcell array, place first without rotation. 
        // This will be replaced, once we know the instance size.

        CDtx ttx(tx);
        ttx.decode_transform(0);
        oitmp = sdesc->makeCall(&calldesc, &ttx, &ap, CDcallNone, &cdesc);
    }
    else
        oitmp = sdesc->makeCall(&calldesc, &tx, &ap, CDcallNone, &cdesc);
    if (oitmp == OIerror) {
        Errs()->add_error("MakeCall failed");
        Log()->ErrorLog(mh::CellPlacement, Errs()->get_error());
        return (0);
    }
    if (oitmp == OIaborted)
        return (0);

    // check if addition makes hierarchy recursive
    CDgenHierDn_s gen(sdesc);
    bool err;
    while (gen.next(&err)) ;
    if (err) {
        if (!sdesc->unlink(cdesc, false)) {
            Log()->ErrorLog(mh::CellPlacement,
                "Errs()->get_error().");
        }
        else {
            Log()->ErrorLog(mh::CellPlacement,
                "Hierarchy depth exceeded, cell not added.");
        }
        return (0);
    }
    Ulist()->RecordObjectChange(sdesc, 0, cdesc);

    if (plsd->prpty(XICP_PC_SCRIPT) && (iap.nx() > 1 || iap.ny() > 1)) {
        // Handle arrayed pcells.  These have unknown BB until
        // instantiated.  We have already instantiated a single
        // instance for sizing.

        BBox tBB = cdesc->oBB();
        CDap xap(iap.nx(), iap.ny(),
            tBB.width() + iap.spx(), tBB.height() + iap.spy());
        CDc *oldc = cdesc;
        oitmp = sdesc->makeCall(&calldesc, &tx, &xap, CDcallNone, &cdesc);
        if (oitmp == OIerror) {
            Errs()->add_error("makeCall failed");
            Log()->ErrorLog(mh::CellPlacement, Errs()->get_error());
            return (0);
        }
        if (oitmp == OIaborted)
            return (0);
        Ulist()->RecordObjectChange(sdesc, oldc, cdesc);
    }

    if (smash)
        // Return a bogus nonzero pointer to indicate success.
        cdesc = (CDc*)flattenCell(&stk, cdesc);
    else if (sdesc->isElectrical())
        ScedIf()->genDeviceLabels(cdesc, 0, false);
    return (cdesc);
}


//------------------------------------------------------------------------------
// Ghost Rendering

namespace {
    // Helpers for showTransformed.  This is a bit tricky in the
    // recursive case, as we want to limit drawing for response time,
    // but want to show everything important.  We do the drawing in
    // order of hierarchy level, and bail if the max object count is
    // reached.


    unsigned int display_ghost_transformed(CDo*, CDtf*, CDtf*, int, int);

    // Record the max hierarchy used in the cell being ghosted.
    int ghost_max_hlev;


    unsigned int show_transformed(CDs *sdesc, CDtf *tf, int hierlev, int dmode)
    {
        // Devices can be symbolic, or not.
        CDs *srep = sdesc->symbolicRep(0);
        if (srep)
            sdesc = srep;
        unsigned int count = 0;

        if (hierlev > ghost_max_hlev)
            ghost_max_hlev = hierlev;

        if (dmode == 0) {
            // Draw geometry.
            CDsLgen lgen(sdesc, false);
            CDl *ld;
            while ((ld = lgen.next()) != 0) {
                CDg gdesc;
                gdesc.init_gen(sdesc, ld);
                CDtf tfold;
                DSP()->TCurrent(&tfold);
                CDo *odesc;
                while ((odesc = gdesc.next()) != 0) {
                    count += display_ghost_transformed(odesc, &tfold, tf,
                        hierlev, dmode);
                    if (count > EGst()->maxGhostObjects()) {
                        Point pnts[5];
                        sdesc->BB()->to_path(pnts);
                        DSP()->TLoadCurrent(tf);
                        Point::xform(pnts, DSP(), 5);
                        DSP()->TLoadCurrent(&tfold);
                        Gst()->ShowGhostPath(pnts, 5);
                        return (count);
                    }
                }
            }
        }
        else if (dmode > 0) {
            // Recurse subcells.
            CDg gdesc;
            gdesc.init_gen(sdesc, CellLayer());
            CDtf tfold;
            DSP()->TCurrent(&tfold);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                count += display_ghost_transformed(odesc, &tfold, tf,
                    hierlev, dmode);
                if (count > EGst()->maxGhostObjects()) {
                    Point pnts[5];
                    sdesc->BB()->to_path(pnts);
                    DSP()->TLoadCurrent(tf);
                    Point::xform(pnts, DSP(), 5);
                    DSP()->TLoadCurrent(&tfold);
                    Gst()->ShowGhostPath(pnts, 5);
                    return (count);
                }
            }
        }
        return (count);
    }


    // Show the transformed outline, return a number which is an
    // approximation of the work needed to render the object.
    //
    unsigned int display_ghost_transformed(CDo *odesc, CDtf *tfold, CDtf *tfnew,
        int hlev, int dmode)
    {
        if (!odesc)
            return (0);
#ifdef HAVE_COMPUTED_GOTO
        COMPUTED_JUMP(odesc)
#else
        CONDITIONAL_JUMP(odesc)
#endif
    box:
        {
            if (dmode != 0)
                return (0);
            Point pnts[5];
            odesc->oBB().to_path(pnts);
            DSP()->TLoadCurrent(tfnew);
            Point::xform(pnts, DSP(), 5);
            DSP()->TLoadCurrent(tfold);
            Gst()->ShowGhostPath(pnts, 5);
            return (1);
        }
    poly:
        {
            if (dmode != 0)
                return (0);
            int num = ((const CDpo*)odesc)->numpts();
            DSP()->TLoadCurrent(tfnew);
            Point *pts = Point::dup_with_xform(((const CDpo*)odesc)->points(),
                DSP(), num);
            DSP()->TLoadCurrent(tfold);
            Gst()->ShowGhostPath(pts, num);
            delete [] pts;
            return ((num+1)/4);
        }
    wire:
        {
            if (dmode != 0)
                return (0);
            int num = ((const CDw*)odesc)->numpts();
            DSP()->TLoadCurrent(tfnew);
            Wire wire(num,
                Point::dup_with_xform(((const CDw*)odesc)->points(),
                    DSP(), num),
                ((const CDw*)odesc)->attributes());
            DSP()->TLoadCurrent(tfold);
            if (!ED()->noWireWidthMag())
                wire.set_wire_width(mmRnd(wire.wire_width()*tfnew->mag()));
            EGst()->showGhostWire(&wire);
            delete [] wire.points;
            return ((num+1)/2);
        }
    label:
        {
            if (dmode != 0)
                return (0);
            BBox BB;
            Point *pts;
            odesc->boundary(&BB, &pts);
            if (!pts) {
                pts = new Point[5];
                BB.to_path(pts);
            }
            DSP()->TLoadCurrent(tfnew);
            Point::xform(pts, DSP(), 5);
            DSP()->TLoadCurrent(tfold);
            Gst()->ShowGhostPath(pts, 5);
            delete [] pts;
            return (1);
        }
    inst:
        {
            if (dmode < 1)
                return (0);
            CDs *msdesc = ((CDc*)odesc)->masterCell();
            if (!msdesc)
                return (0);
            bool show_detail = false;
            if (DSP()->CurMode() == Electrical) {
                if (msdesc->isSymbolic() || msdesc->isDevice())
                    show_detail = true;
            }
            else {
                if (msdesc->isPCellSubMaster() || msdesc->isViaSubMaster())
                    show_detail = true;
                else {
                    int expand = DSP()->MainWdesc()->Attrib()->
                        expand_level(DSP()->CurMode());
                    if (DSP()->CurMode() == Physical &&
                            EGst()->maxGhostDepth() >= 0) {
                        if (expand < 0 || expand > EGst()->maxGhostDepth())
                            expand = EGst()->maxGhostDepth();
                    }
                    show_detail = expand < 0 || expand > hlev ||
                        odesc->has_flag(DSP()->MainWdesc()->DisplFlags());
                }
            }
            if (show_detail) {
                DSP()->TLoadCurrent(tfold);
                DSP()->TPush();
                DSP()->TApplyTransform((CDc*)odesc);
                DSP()->TPremultiply();

                unsigned int cnt = 0;
                CDap ap((CDc*)odesc);
                int tx, ty;
                DSP()->TGetTrans(&tx, &ty);
                xyg_t xyg(0, ap.nx - 1, 0, ap.ny - 1);
                do {
                    DSP()->TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                    cnt += show_transformed(msdesc, tfnew, hlev+1, dmode-1);
                    DSP()->TSetTrans(tx, ty);
                } while (xyg.advance());
                DSP()->TPop();
                return (cnt);
            }
            if (dmode == 1) {
                DSP()->TLoadCurrent(tfold);
                DSP()->TPush();
                DSP()->TApplyTransform((CDc*)odesc);
                DSP()->TPremultiply();
                Point pnts[5];
                msdesc->BB()->to_path(pnts);
                Point::xform(pnts, DSP(), 5);
                DSP()->TLoadCurrent(tfnew);
                Gst()->ShowGhostPath(pnts, 5);
                DSP()->TPop();
                return (1);
            }
            return (0);
        }
    }
}


// The function to display a ghost image of the device or cell
// attached to the pointer.  Used in schematic mode to show the
// internal structure of the library instance, and in smash mode.
//
unsigned int
cEditGhost::showTransformed(CDs *sdesc, CDtf *tf, bool show_hier)
{
    // Devices can be symbolic, or not.
    CDs *srep = sdesc->symbolicRep(0);
    if (srep)
        sdesc = srep;
    unsigned int count = 0;

    if (!show_hier)
        return (show_transformed(sdesc, tf, 0, 0));

    ghost_max_hlev = 0;  // Initialization.
    for (int dmode = 0; dmode < CDMAXCALLDEPTH; dmode++) {
        count += show_transformed(sdesc, tf, 1, dmode);
        if (dmode == ghost_max_hlev)
            break;  // No more hierarchy, we're done.
        if (count > eg_max_ghost_objects)
            break;  // Limit reached, we're done.
    }
    return (count);
}


// Incremental version of above.  Call with dmode = 0, 1,...  until
// done returns true, or positive return larger than limit.
//
//
unsigned int
cEditGhost::showTransformedLevel(CDs *sdesc, CDtf *tf, int dmode, bool *done)
{
    // Devices can be symbolic, or not.
    CDs *srep = sdesc->symbolicRep(0);
    if (srep)
        sdesc = srep;

    if (done)
        *done = false;
    if (dmode < 0 || dmode > CDMAXCALLDEPTH)
        return (0);
    if (dmode == 0)
        ghost_max_hlev = 0;  // Initialization.

    unsigned int count = show_transformed(sdesc, tf, 1, dmode);
    if (done && dmode == ghost_max_hlev)
        *done = true;  // No more hierarchy, we're done.
    return (count);
}


// Callback to render the master bounding box attached to the
// pointer in place mode.
//
void
cEditGhost::showGhostInstance(int x, int y, int, int)
{
    CDs *sdesc = PlaceState::Sym.celldesc(DSP()->CurMode());

    if (!sdesc || sdesc->isPCellSuperMaster()) {
        // The cell is not set yet, or is a super-master.  We don't
        // know the BB.

        DSP()->TPush();
        DSP()->TSetMagn(GEO()->curTx()->magn());
        GEO()->applyCurTransform(DSP(), 0, 0, x, y);
        DSP()->TPremultiply();

        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (!wdesc->IsSimilar(DSP()->MainWdesc()))
                continue;
            BBox BB;
            int np = (int)(15.0/wdesc->Ratio());
            BB.bloat(np);
            wdesc->ShowLineBox(&BB);
            BB.bloat(np/10);
            wdesc->ShowLineBox(&BB);
        }
        DSP()->TPop();
        return;
    }

    CDtf tf_old;
    DSP()->TCurrent(&tf_old);
    int num_x = ED()->arrayParams().nx();
    int num_y = ED()->arrayParams().ny();
    const BBox *sBB = sdesc->BB();
    int dx = ED()->arrayParams().spx() + sBB->width();
    int dy = ED()->arrayParams().spy() + sBB->height();

    bool smash_mode = false;
    if (PlaceCmd) {
        if (!PlaceCmd->no_snap())
            ED()->find_contact(sdesc, &x, &y, true);
        smash_mode = PlaceCmd && PlaceCmd->smash_mode();
    }
    int xx, yy;
    ED()->get_reference_handle(sdesc, &xx, &yy, ED()->instanceRef(),
        ED()->arrayParams(), 0);
    DSP()->TPush();
    DSP()->TSetMagn(GEO()->curTx()->magn());
    GEO()->applyCurTransform(DSP(), xx, yy, x, y);
    DSP()->TPremultiply();
    CDtf tf_new;
    DSP()->TCurrent(&tf_new);

    if (DSP()->CurMode() == Electrical) {
        if (sdesc->isDevice())
            showTransformed(sdesc, &tf_old);
        else {
            CDs *sd = smash_mode ? 0 : sdesc->symbolicRep(0);
            // Show the terminals, other than the handle
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0) {
                if (!wdesc->IsSimilar(Electrical, DSP()->MainWdesc()))
                    continue;
                int delta = (int)(2.0/wdesc->Ratio());
                CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
                for ( ; pn; pn = pn->next()) {
                    for (unsigned int ix = 0; ; ix++) {
                        if (pn->index() == 0 && ix == 0)
                            continue;
                        if (sd) {
                            if (!pn->get_pos(ix, &xx, &yy))
                                break;
                        }
                        else {
                            if (ix)
                                break;
                            pn->get_schem_pos(&xx, &yy);
                        }
                        DSP()->TPoint(&xx, &yy);
                        DSP()->TLoadCurrent(&tf_old);
                        wdesc->ShowLineW(xx - delta, yy, xx + delta, yy);
                        wdesc->ShowLineW(xx, yy - delta, xx, yy + delta);
                        DSP()->TLoadCurrent(&tf_new);
                    }
                }
            }
            if (sd)
                showTransformed(sd, &tf_old);
            else if (smash_mode)
                showTransformed(sdesc, &tf_old);
            else {
                BBox BB = *sBB;
                Point *pts;
                DSP()->TBB(&BB, &pts);
                DSP()->TLoadCurrent(&tf_old);
                if (pts) {
                    Gst()->ShowGhostPath(pts, 5);
                    delete [] pts;
                }
                else
                    Gst()->ShowGhostBox(BB.left, BB.bottom, BB.right, BB.top);
            }
        }
        DSP()->TPop();
        return;
    }
    if (smash_mode || sdesc->isViaSubMaster() || sdesc->isPCellSubMaster() ||
            DSP()->MainWdesc()->Attrib()->expand_level(DSP()->CurMode())) {
        showTransformed(sdesc, &tf_old, true);
    }
    else {
        BBox BBc = *sBB;
        BBox BBa = BBc;
        if (dx > 0)
            BBa.right += (num_x - 1)*dx;
        else
            BBa.left += (num_x - 1)*dx;
        if (dy > 0)
            BBa.top += (num_y - 1)*dy;
        else
            BBa.bottom += (num_y - 1)*dy;
        Point *ptc = 0, *pta = 0;
        DSP()->TBB(&BBc, &ptc);
        if (num_x > 1 || num_y > 1)
            DSP()->TBB(&BBa, &pta);
        DSP()->TLoadCurrent(&tf_old);

        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (!wdesc->Wdraw())
                continue;
            if (!wdesc->IsSimilar(Physical, DSP()->MainWdesc()))
                continue;
            if (!smash_mode) {
                if (ED()->replacing())
                    wdesc->Wdraw()->SetLinestyle(DSP()->BoxLinestyle());
                if (ptc) {
                    wdesc->ShowLineW(ptc[0].x, ptc[0].y, ptc[1].x, ptc[1].y);
                    wdesc->ShowLineW(ptc[1].x, ptc[1].y, ptc[2].x, ptc[2].y);
                    wdesc->ShowLineW(ptc[2].x, ptc[2].y, ptc[3].x, ptc[3].y);
                    wdesc->ShowLineW(ptc[3].x, ptc[3].y, ptc[0].x, ptc[0].y);
                    wdesc->ShowLineW(ptc[0].x, ptc[0].y, ptc[2].x, ptc[2].y);
                    wdesc->ShowLineW(ptc[1].x, ptc[1].y, ptc[3].x, ptc[3].y);
                }
                else {
                    wdesc->ShowLineW(BBc.left, BBc.bottom, BBc.left, BBc.top);
                    wdesc->ShowLineW(BBc.left, BBc.top, BBc.right, BBc.top);
                    wdesc->ShowLineW(BBc.right, BBc.top, BBc.right, BBc.bottom);
                    wdesc->ShowLineW(BBc.right,BBc.bottom, BBc.left,BBc.bottom);
                    wdesc->ShowLineW(BBc.left, BBc.bottom, BBc.right, BBc.top);
                    wdesc->ShowLineW(BBc.left, BBc.top, BBc.right, BBc.bottom);
                }
                if (ED()->replacing())
                    wdesc->Wdraw()->SetLinestyle(0);
            }

            if (num_x == 1 && num_y == 1)
                continue;

            wdesc->Wdraw()->SetLinestyle(DSP()->BoxLinestyle());
            if (pta) {
                wdesc->ShowLineW(pta[0].x, pta[0].y, pta[1].x, pta[1].y);
                wdesc->ShowLineW(pta[1].x, pta[1].y, pta[2].x, pta[2].y);
                wdesc->ShowLineW(pta[2].x, pta[2].y, pta[3].x, pta[3].y);
                wdesc->ShowLineW(pta[3].x, pta[3].y, pta[0].x, pta[0].y);
            }
            else {
                wdesc->ShowLineW(BBa.left, BBa.bottom, BBa.left, BBa.top);
                wdesc->ShowLineW(BBa.left, BBa.top, BBa.right, BBa.top);
                wdesc->ShowLineW(BBa.right, BBa.top, BBa.right, BBa.bottom);
                wdesc->ShowLineW(BBa.right, BBa.bottom, BBa.left, BBa.bottom);
            }
            wdesc->Wdraw()->SetLinestyle(0);
        }
        delete [] ptc;
        delete [] pta;
    }
    DSP()->TPop();
}
// End of cEditGhost functions.

