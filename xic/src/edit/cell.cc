
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
#include "scedif.h"
#include "fio.h"
#include "fio_chd.h"
#include "cd_celldb.h"
#include "cd_lgen.h"
#include "dsp_inlines.h"
#include "select.h"
#include "errorlog.h"
#include "promptline.h"
#include "undolist.h"
#include "events.h"


namespace {
    void find_ll(CDo *odesc, int *xmin, int *ymin)
    {
        if (!odesc)
            return;
#ifdef HAVE_COMPUTED_GOTO
        COMPUTED_JUMP(odesc)
#else
        CONDITIONAL_JUMP(odesc)
#endif
    box:
    poly:
    label:
        return;
    wire:
        {
            const Point *pts = ((CDw*)odesc)->points();
            int num = ((CDw*)odesc)->numpts();
            for (int i = 0; i < num; i++) {
                if (*xmin > pts[i].x)
                    *xmin = pts[i].x;
                if (*ymin > pts[i].y)
                    *ymin = pts[i].y;
            }
            return;
        }
    inst:
        {
            CDp_cnode *pn = (CDp_cnode*)((CDc*)odesc)->prpty(P_NODE);
            for (; pn; pn = pn->next()) {
                for (unsigned int ix = 0; ; ix++) {
                    int x, y;
                    if (!pn->get_pos(ix, &x, &y))
                        break;
                    if (*xmin > x)
                        *xmin = x;
                    if (*ymin > y)
                        *ymin = y;
                }
            }
            return;
        }
    }


    // Return the the point to be used as offset, the lower left
    // projection of device nodes and wire vertices.  One can't just
    // use the lower left of the BB, since it may be off-grid.  In no
    // wires or subcells, false is returned.
    //
    bool find_ll_node(int *x, int *y)
    {
        int xmin = CDinfinity;
        int ymin = CDinfinity;
        sSelGen sg(Selections, CurCell(), "wc");
        CDo *od;
        while ((od = sg.next()) != 0)
            find_ll(od, &xmin, &ymin);
        if (xmin == CDinfinity) {
            *x = *y = 0;
            return (false);
        }
        *x = xmin;
        *y = ymin;
        return (true);
    }
}


// CRCEL menu command.
// Create a new cell from the contents of the selection queue.
//
void
cEdit::createCellExec(CmdDesc*)
{
    CDs *cursd = CurCell();
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    if (!Selections.hasTypes(cursd, 0)) {
        PL()->ShowPrompt("Objects must be selected first.");
        return;
    }
    char *in = PL()->EditPrompt("Name of new cell? ", "newcell");
    in = lstring::strip_space(in);
    if (!in || !*in) {
        PL()->ErasePrompt();
        return;
    }
    char *cellname = lstring::copy(in);
    CDcbin cbin;
    if (!createCell(cellname, &cbin, true, 0)) {
        delete [] cellname;
        return;
    }
    delete [] cellname;

    if (!cursd->isImmutable()) {
        in = PL()->EditPrompt("Replace selected objects with new cell? ", "n");
        in = lstring::strip_space(in);
        if (in && (*in == 'y' || *in == 'Y')) {
            BBox BB;
            Selections.computeBB(cursd, &BB, false);  // always true
            // Find the new cell origin.
            int xo, yo;
            if (DSP()->CurMode() == Physical || !find_ll_node(&xo, &yo)) {
                xo = BB.left;
                yo = BB.bottom;
            }
            Ulist()->ListCheck("crcel", cursd, false);
            deleteQueue();
            if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
                // These might be deleted
                DSP()->ShowCellTerminalMarks(ERASE);

            sCurTx curtf = *GEO()->curTx();
            sCurTx ident;
            GEO()->setCurTx(ident);
            makeInstance(cursd, Tstring(cbin.cellname()), xo, yo, 0, 0);
            GEO()->setCurTx(curtf);

            Ulist()->CommitChanges(true);
            if (DSP()->CurMode() == Electrical && DSP()->ShowTerminals())
                DSP()->ShowCellTerminalMarks(DISPLAY);

            XM()->PopUpCells(0, MODE_UPD);
        }
        PL()->ErasePrompt();
    }
}


// Create a new cell from the contents of the selection queue
// (CDselected or not).  Return true if successful.  If perm, we are
// creating a permanent new cell, otherwise:
//
//  1. the cell may be empty
//  2. we don't check for name in use
//  3. we don't set the save as native flag
//
// If BBorg is non-nil, use left/bottom as the origin point of the new
// cell in physical mode only.  Otherwise, use lower left of bounding
// box or reference node.
//
bool
cEdit::createCell(const char *cellname, CDcbin *cbret, bool perm, BBox *BBorg)
{
    if (cbret)
        cbret->reset();
    CDs *cursd = CurCell();
    if (perm && !Selections.hasTypes(cursd, 0)) {
        PL()->ShowPrompt("Nothing selected!");
        return (false);
    }
    const char *cname = lstring::strip_path(cellname);
    if (perm && !CDvdb()->getVariable(VA_CrCellOverwrite) &&
            CDcdb()->findSymbol(cname, 0)) {
        PL()->ShowPromptV("Cell name %s already in use.", cname);
        return (false);
    }
    if (DSP()->CurMode() == Electrical) {
        // Remove all property labels from the selections.  We can't
        // include orphaned property labels.
        Selections.purgeLabels(cursd, true);

        // Now add back all labels bound to objects in the selections.
        Selections.addLabels(cursd);

        // In order to set up the reference pointers properly in electrical
        // mode, the calls must be added first.
        Selections.instanceToFront(cursd);

        if (perm && !Selections.hasTypes(cursd, 0)) {
            PL()->ShowPrompt(
                "After purging bound property labels, nothing selected!");
            return (false);
        }
    }

    BBox BB;
    Selections.computeBB(cursd, &BB);

    CDs *newdesc = CD()->ReopenCell(cname, DSP()->CurMode());
    if (!newdesc) {
        PL()->ShowPrompt(Errs()->get_error());
        return (false);
    }
    XM()->PopUpCells(0, MODE_UPD);
    XM()->PopUpTree(0, MODE_UPD, 0, TU_CUR);

    CDcbin cbin;
    if (DSP()->CurMode() == Physical)
        cbin.setPhys(newdesc);
    else
        cbin.setElec(newdesc);

    // Find the new cell origin
    int xo, yo;
    if (DSP()->CurMode() == Physical) {
        if (BBorg) {
            xo = BBorg->left;
            yo = BBorg->bottom;
        }
        else {
            xo = BB.left;
            yo = BB.bottom;
        }
    }
    else if (!find_ll_node(&xo, &yo)) {
        xo = BB.left;
        yo = BB.bottom;
    }
    if (DSP()->CurMode() == Electrical && newdesc) {
        CDs *cursde = CurCell(Electrical);
        if (cursde) {
            ScedIf()->connectAll(false);
            // catch mutual inductors if both selected
            for (CDp *pdesc = cursde->prptyList(); pdesc;
                    pdesc = pdesc->next_prp()) {
                if (pdesc->value() == P_NEWMUT && mutSelected(cursde, pdesc)) {
                    CDp_nmut *pm = (CDp_nmut*)newdesc->prptyAddCopy(pdesc);
                    if (pm) {
                        pm->set_l1_dev(0);
                        pm->set_l2_dev(0);
                        pm->bind(0);
                    }
                }
                else if (pdesc->value() == P_MUT &&
                        mutSelected(cursde, pdesc)) {
                    CDp_mut *pm = (CDp_mut*)newdesc->prptyAddCopy(pdesc);
                    if (pm) {
                        pm->set_pos1_x(pm->pos1_x() - xo);
                        pm->set_pos1_y(pm->pos1_y() - yo);
                        pm->set_pos2_x(pm->pos2_x() - xo);
                        pm->set_pos2_y(pm->pos2_y() - yo);
                        pm->bind(0);
                    }
                }
            }
        }
    }

    sSelGen sg(Selections, cursd, "c");
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (!ED()->addToCell(od, newdesc, xo, yo)) {
            Errs()->add_error("Object addition failed.");
            Log()->ErrorLog(mh::Internal, Errs()->get_error());
            CD()->Close(CD()->CellNameTableFind(cname));
            PL()->ErasePrompt();
            return (false);
        }
    }

    CDsLgen lgen(cursd);
    CDl *ld;
    while ((ld = lgen.next()) != 0) {
        sg = sSelGen(Selections, cursd, "bpwl");
        while ((od = sg.next()) != 0) {
            if (od->ldesc() == ld)
                ED()->addToCell(od, newdesc, xo, yo);
        }
    }

    if (perm) {
        cbin.setSaveNative();
        PL()->ShowPromptV("New cell %s created.", cname);
    }
    if (cbret)
        *cbret = cbin;
    return (true);
}


// Function to open the cell in name in the mode given.  The cell desc
// is returned if successful.  If name is an archive file, cname is
// the cell to open.  If chd is given, name is ignored.
//
bool
cEdit::openCell(const char *name, CDcbin *cbret, char *cname, cCHD *chd)
{
    if (cbret)
        cbret->reset();
    if (chd)
        name = chd->filename();

    CDcbin cbin;
    OItype oiret = FIO()->OpenImport(name, FIO()->DefReadPrms(), cname,
        chd, &cbin);

    if (oiret == OIerror) {
        Errs()->add_error("Error opening %s", name);
        return (false);
    }
    if (oiret == OIaborted) {
        Errs()->add_error("Open %s aborted on user request,", name);
        return (false);
    }
    if (oiret == OInew) {
        Errs()->add_error("Cell %s was not found.", name);
        CD()->Close(CD()->CellNameTableFind(name));  // delete empty cell
        return (false);
    }
    if (cbin.elec() && !cbin.isDevice() && oiret != OIold)
        ScedIf()->checkElectrical(&cbin);
    if (cbret)
        *cbret = cbin;
    return (true);
}


// Add a copy of odesc in newdesc, offset by xo,yo.
//
bool
cEdit::addToCell(CDo *odesc, CDs *newdesc, int xo, int yo)
{
    if (!odesc)
        return (true);
    if (!newdesc)
        return (false);
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
    {
        BBox BB = odesc->oBB();
        BB.left -= xo;
        BB.right -= xo;
        BB.bottom -= yo;
        BB.top -= yo;

        CDo *newo;
        if (newdesc->makeBox(odesc->ldesc(), &BB, &newo) != CDok)
            return (false);
        newo->prptyAddCopyList(odesc->prpty_list());
        return (true);
    }
poly:
    {
        int num = ((const CDpo*)odesc)->numpts();
        const Point *pts = ((const CDpo*)odesc)->points();
        Poly poly(num, new Point[num]);
        for (int i = 0; i < num; i++)
            poly.points[i].set(pts[i].x - xo, pts[i].y - yo);

        CDpo *newo;
        if (newdesc->makePolygon(odesc->ldesc(), &poly, &newo) != CDok)
            return (false);
        newo->prptyAddCopyList(odesc->prpty_list());
        return (true);
    }
wire:
    {
        int num = ((const CDw*)odesc)->numpts();
        const Point *pts = ((const CDw*)odesc)->points();
        Wire wire(num, new Point[num], ((const CDw*)odesc)->attributes());
        for (int i = 0; i < wire.numpts; i++)
            wire.points[i].set(pts[i].x - xo, pts[i].y - yo);

        CDw *newo;
        if (newdesc->makeWire(odesc->ldesc(), &wire, &newo) != CDok)
            return (false);
        newo->prptyAddCopyList(odesc->prpty_list());
        return (true);
    }
label:
    {
        Label label(((CDla*)odesc)->la_label());
        label.x -= xo;
        label.y -= yo;
        label.label = hyList::dup(label.label);

        CDla *newo;
        if (newdesc->makeLabel(odesc->ldesc(), &label, &newo) != CDok)
            return (false);
        newo->prptyAddCopyList(odesc->prpty_list());
        if (DSP()->CurMode() == Electrical)
            newdesc->prptyLabelPatch(newo);
        return (true);
    }
inst:
    {
        CallDesc calldesc;
        ((CDc*)odesc)->call(&calldesc);

        cTfmStack stk;
        stk.TPush();
        stk.TApplyTransform((CDc*)odesc);
        stk.TTranslate(-xo, -yo);
        CDtx tx;
        stk.TCurrent(&tx);
        stk.TPop();

        CDap ap((CDc*)odesc);
        CDc *newo;
        if (OIfailed(newdesc->makeCall(&calldesc, &tx, &ap, CDcallNone,
                &newo)))
            return (false);
        if (DSP()->CurMode() == Electrical) {
            // remove any default user properties
            newo->prptyRemove(P_NAME);
            newo->prptyRemove(P_MODEL);
            newo->prptyRemove(P_VALUE);
            newo->prptyRemove(P_PARAM);
            newo->prptyRemove(P_OTHER);
            newo->prptyRemove(P_NOPHYS);
            newo->prptyRemove(P_FLATTEN);
            newo->prptyRemove(P_RANGE);
            newo->prptyRemove(P_DEVREF);
            // add the user and mutlrf properties
            for (CDp *pdesc = odesc->prpty_list(); pdesc;
                    pdesc = pdesc->next_prp()) {
                if (pdesc->value() == P_NAME ||
                    pdesc->value() == P_MODEL ||
                    pdesc->value() == P_VALUE ||
                    pdesc->value() == P_PARAM ||
                    pdesc->value() == P_OTHER ||
                    pdesc->value() == P_NOPHYS ||
                    pdesc->value() == P_FLATTEN ||
                    pdesc->value() == P_RANGE ||
                    pdesc->value() == P_MUTLRF) {
                    CDp *pnew = newo->prptyAddCopy(pdesc);
                    if (pnew)
                        pnew->bind(0);
                }
            }
            newdesc->prptyInstPatch(newo);
        }
        else
            newo->prptyAddCopyList(odesc->prpty_list());
        return (true);
    }
}


// Copy the named cell, which must be in the database, to newname. 
// Returns true on success.  This operates on both physical and
// electrical cells of the given name.
//
bool
cEdit::copySymbol(const char *name, const char *newname)
{
    CDcbin cbin;
    if (CDcdb()->findSymbol(name, &cbin) && !CDcdb()->findSymbol(newname, 0)) {
        CDcbin newcbin;

        if (cbin.phys()) {
            newcbin.setPhys(CDcdb()->insertCell(newname, Physical));
            if (newcbin.phys())
                cbin.phys()->cloneCell(newcbin.phys());
        }
        if (cbin.elec()) {
            newcbin.setElec(CDcdb()->insertCell(newname, Electrical));
            if (newcbin.elec())
                cbin.elec()->cloneCell(newcbin.elec());
        }
            
        XM()->PopUpCells(0, MODE_UPD);
        return (true);
    }
    return (false);
}


// Rename the named cell, which must be in the database, to newname. 
// Returns true on success.  This operates on both physical and
// electrical cells of the given name.
//
bool
cEdit::renameSymbol(const char *name, const char *newname)
{
    if (!name || !newname) {
        Errs()->add_error("renameSymbol: null cell name encountered.");
        return (false);
    }
    CDcbin cbin;
    if (!CDcdb()->findSymbol(name, &cbin)) {
        Errs()->add_error("renameSymbol: no cell named \"%s\" found.", name);
        return (false);
    }
    CDcellName oldname = cbin.cellname();
    if (!cbin.rename(newname))
        return (false);

    CDcellName nname = CD()->CellNameTableAdd(newname);
    ScedIf()->updateDotsCellName(oldname, nname);

    plChangeMenuEnt(newname, Tstring(oldname));
    XM()->ReplaceOpenCellName(newname, Tstring(oldname));
    WDgen gen(WDgen::MAIN, WDgen::CDDB);
    WindowDesc *wd;
    while ((wd = gen.next()) != 0) {
        if (wd->CurCellName() == oldname)
            wd->SetCurCellName(nname);
        if (wd->TopCellName() == oldname)
            wd->SetTopCellName(nname);
    }
    return (true);
}


// Rename all cells in the hierarchy to the concatenation/substitution
// pre, cellname, suf.  The pre/suf can either be a simple string, or
// a substitution form /str/sub/.  The substitution form syntax is
// assumed to be correct, no checking is done here.  Return a list of
// names that can't be changed, except for immutable cells that are
// simply skipped.
//
stringlist *
cEdit::renameAll(CDcbin *tbin, const char *pre, const char *suf)
{
    CDgenHierDn_cbin gen(tbin);
    CDcbin cbin;
    stringlist *s0 = 0;
    while (gen.next(&cbin, 0)) {
        if ((cbin.isLibrary() && cbin.isDevice()) || cbin.isImmutable())
            continue;
        s0 = new stringlist(lstring::copy(Tstring(cbin.cellname())), s0);
    }
    int cnt = 0;
    stringlist *sx = 0;
    while (s0) {
        char *newname = cCD::AlterName(s0->string, pre, suf);
        if (!strcmp(s0->string, newname)) {
            // No name change, original name already has given
            // form.
            delete [] newname;
            stringlist *s = s0;
            s0 = s0->next;
            delete [] s->string;
            delete s;
        }
        else {
            bool ret = renameSymbol(s0->string, newname);
            delete [] newname;
            stringlist *s = s0;
            s0 = s0->next;
            if (!ret) {
                s->next = sx;
                sx = s;
                Errs()->get_error();  // clear
            }
            else {
                cnt++;
                delete [] s->string;
                delete s;
            }
        }
    }
    if (cnt) {
        EV()->InitCallback();
        XM()->ShowParameters();
        XM()->PopUpCells(0, MODE_UPD);
        XM()->PopUpTree(0, MODE_UPD, 0, TU_CUR);
    }
    return (sx);
}

