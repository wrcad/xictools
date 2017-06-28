
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
 $Id: flatten.cc,v 1.79 2016/03/02 00:39:36 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "edit.h"
#include "undolist.h"
#include "scedif.h"
#include "extif.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_layer.h"
#include "cd_lgen.h"
#include "cd_propnum.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "events.h"
#include "menu.h"
#include "select.h"
#include "promptline.h"
#include "errorlog.h"
#include "timer.h"
#include "texttf.h"


namespace {
    // Instance count and progress monitor.
    //
    struct sFfb
    {
        void init(int cnt)
            {
                Fcn = 0;
                Fcnt = 0;
                Fmax = cnt;
                Ftime = Timer()->elapsed_msec();
            }

        void check()
            {
                Fcnt++;
                if (Ftime != Timer()->elapsed_msec()) {
                    if (Fcn)
                        PL()->ShowPromptV("Flattening %s %d/%d",
                            Fcn->string(), Fcnt, Fmax);
                    Ftime = Timer()->elapsed_msec();
                }
            }

        void instance(CDcellName cn)
            {
                Fcn = cn;
            }

        static int count_instances(CDs*, int, int = 0);

    private:
        CDcellName Fcn;
        int Fcnt;
        int Fmax;
        unsigned int Ftime;
    };


    // This counts the number on instances that will be flattened, for use
    // in progress monitoring.
    //
    int
    sFfb::count_instances(CDs *sdesc, int maxdepth, int depth)
    {
        int cnt = 0;
        if (maxdepth > depth) {
            CDm_gen mgen(sdesc, GEN_MASTERS);
            for (CDm *md = mgen.m_first(); md; md = mgen.m_next()) {
                int mcnt = 0;
                CDc_gen cgen(md);
                for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
                    CDap ap(c);
                    int n = ap.nx * ap.ny;
                    if (n <= 0)
                        n = 1;
                    mcnt += n;
                }
                if (!mcnt)
                    continue;
                cnt += mcnt * count_instances(md->celldesc(), maxdepth,
                    depth + 1);
            }
        }
        cnt++;
        return (cnt);
    }


    // Data struct for pop-up.
    //
    struct sFdata
    {
        sFdata() { depth = 0; merge = false; mode = false; }

        int depth;
        bool merge;
        bool mode;
    };


    sFfb FB;

    // Idle proc to actually perform the flattening operation
    //
    int
    f_idle(void *arg)
    {
        sFdata *fd = (sFdata*)arg;
        if (!fd)
            return (false);
        cTfmStack stk;
        ED()->flattenSelected(&stk, fd->depth, fd->merge, fd->mode);
        return (false);
    }


    // Callback for the pop-up
    //
    bool
    f_cb(const char *name, bool state, const char *string, void *arg)
    {
        if (name) {
            sFdata *fd = (sFdata*)arg;
            if (!fd)
                return (false);
            if (!strcmp(name, "depth")) {
                if (string) {
                    if (isdigit(*string))
                        fd->depth = *string - '0';
                    else if (*string == 'a')
                        fd->depth = CDMAXCALLDEPTH;
                }
                return (true);
            }
            if (!strcmp(name, "mode")) {
                fd->mode = state;
                return (true);
            }
            if (!strcmp(name, "merge")) {
                fd->merge = state;
                return (true);
            }
            if (!strcmp(name, "flatten")) {
                dspPkgIf()->RegisterIdleProc(f_idle, arg);
                return (false);
            }
        }
        return (true);
    }


    // Only non-symbolic non-library cells can be flattened in
    // electrical mode, all subcells are flattenable in physical mode.
    //
    inline bool
    is_flattenable(CDc *cdesc)
    {
        CDs *sd = cdesc->masterCell();
        if (!sd)
            return (false);
        if (sd->isElectrical()) {
            if (sd->isDevice())
                return (false);
            if (sd->isSymbolic())
                return (false);
        }
        return (true);
    }
}


// Pop up the flatten control panel.
//
void
cEdit::flattenExec(CmdDesc *cmd)
{
    static sFdata *Fdata;
    if (noEditing())
        return;
    if (!CurCell() || CurCell()->isSymbolic())
        return;
    if (!XM()->CheckCurCell(true, false, DSP()->CurMode())) {
        if (cmd && cmd->caller)
            Menu()->Deselect(cmd->caller);
        return;
    }
    if (cmd && Menu()->GetStatus(cmd->caller)) {
        if (!Fdata)
            Fdata = new sFdata;
        PopUpFlatten(cmd->caller, MODE_ON, f_cb, Fdata, Fdata->depth,
            Fdata->mode);
    }
    else
        PopUpFlatten(0, MODE_OFF, 0, 0);
}


// Flatten selected cells to depth, no undo if fastmode.
//
void
cEdit::flattenSelected(cTfmStack *tstk, int depth, bool use_merge,
    bool fastmode)
{
    CDs *cursd = CurCell();
    if (!cursd || cursd->isSymbolic())
        return;
    dspPkgIf()->SetWorking(true);
    CDtf mtf;
    tstk->TCurrent(&mtf);
    if (!fastmode)
        Ulist()->ListCheck("flatten", cursd, true);
    EV()->PushCallback(0);  // reset main state, avoid undo strangeness

    int icnt = 0;
    sSelGen sg(Selections, cursd, "c");
    CDo *od;
    while ((od = sg.next()) != 0) {
        CDc *cd = OCALL(od);
        if (is_flattenable(cd)) {
            icnt += FB.count_instances(cd->masterCell(), depth);
            CDap ap(cd);
            int n = ap.nx * ap.ny;
            if (n <= 0)
                n = 1;
            icnt += n;
        }
    }
    if (icnt == 0) {
        PL()->ShowPrompt("No flattenable instances are selected.");
        dspPkgIf()->SetWorking(false);
        return;
    }

    FB.init(icnt);

    // Allow flattening of cells containing the inverted ground plane
    // internal layer from extraction.
    bool tmp_exgp = ExtIf()->setInvGroundPlaneImmutable(false);

    // For each call, flatten it, remove it from the select queue,
    // and add it to the delete list.  The add list contains new
    // objects.  Note that we are not in symbolic mode.
    //
    BBox selBB;
    Selections.computeBB(cursd, &selBB, false);
    sg = sSelGen(Selections, cursd, "c");
    while ((od = sg.next()) != 0) {
        CDc *cdesc = (CDc*)od;
        if (is_flattenable(cdesc)) {
            CallDesc calldesc;
            cdesc->call(&calldesc);
            FB.instance(cdesc->cellname());

            // Delete associated property labels, if any.
            // celldesc() is ok, from is_flattenable
            if (cdesc->masterCell()->isElectrical()) {
                // Delete name property.
                CDp_name *pn = (CDp_name*)cdesc->prpty(P_NAME);
                if (pn && pn->bound()) {
                    if (fastmode) {
                        SI()->UpdateObject(pn->bound(), 0);
                        pn->bound()->set_state(CDDeleted);
                        if (!cursd->isElectrical())
                            DSP()->ShowOdescPhysProperties(
                                pn->bound(), ERASE);
                        cursd->unlink(pn->bound(), false);
                    }
                    else
                        Ulist()->RecordObjectChange(cursd, pn->bound(), 0);
                }

                // Delete param property.
                CDp_user *pp = (CDp_user*)cdesc->prpty(P_PARAM);
                if (pp && pp->bound()) {
                    if (fastmode) {
                        SI()->UpdateObject(pp->bound(), 0);
                        pp->bound()->set_state(CDDeleted);
                        if (!cursd->isElectrical())
                            DSP()->ShowOdescPhysProperties(
                                pp->bound(), ERASE);
                        cursd->unlink(pp->bound(), false);
                    }
                    else
                        Ulist()->RecordObjectChange(cursd, pp->bound(), 0);
                }
            }

            tstk->TPush();
            tstk->TApplyTransform(cdesc);
            tstk->TPremultiply();
            CDap ap(cdesc);
            int tx, ty;
            tstk->TGetTrans(&tx, &ty);
            xyg_t xyg(0, ap.nx - 1, 0, ap.ny - 1);
            do {
                tstk->TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                if (!flatten_cell_recurse(tstk, calldesc.celldesc(), depth, 1,
                        &mtf, use_merge, fastmode)) {
                    tstk->TPop();
                    goto quit;
                }
                tstk->TSetTrans(tx, ty);
            } while(xyg.advance());
            tstk->TPop();
            sg.remove();
            if (fastmode) {
                // update script handles
                SI()->UpdateObject(cdesc, 0);
                cdesc->set_state(CDDeleted);
                if (cdesc->type() == CDINSTANCE) {
                    WindowDesc *wdesc;
                    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                    while ((wdesc = wgen.next()) != 0) {
                        if (wdesc->IsSimilar(DSP()->MainWdesc()))
                            wdesc->ShowInstanceOriginMark(ERASE, cdesc);
                    }
                }
                if (!cursd->isElectrical())
                    DSP()->ShowOdescPhysProperties(cdesc, ERASE);
                cursd->unlink(cdesc, false);
            }
            else
                Ulist()->RecordObjectChange(cursd, cdesc, 0);
            continue;
        }
    }
quit:
    if (!fastmode) {
        PL()->ShowPrompt("Updating undo list...");
        // Skip interactive DRC.
        Ulist()->CommitChanges(false, true);
    }
    if (DSP()->CurMode() == Electrical)
        // Since we don't really know how much to redraw if subcells were
        // symbolic, redraw everything
        DSP()->RedisplayAll(Electrical);
    else
        DSP()->RedisplayArea(&selBB);
    PL()->ErasePrompt();
    XM()->ShowParameters();
    ExtIf()->setInvGroundPlaneImmutable(tmp_exgp);
    dspPkgIf()->SetWorking(false);
}


// Flatten the cell desc provided, full undo mode and zero depth.
// No merging.
//
bool
cEdit::flattenCell(cTfmStack *tstk, CDc *cdesc)
{
    CDs *cursd = CurCell(true);
    if (!cursd)
        return (false);

    CDtf mtf;
    tstk->TCurrent(&mtf);
    if (is_flattenable(cdesc)) {
        CallDesc calldesc;
        cdesc->call(&calldesc);

        // delete the name label, if any
        // celldesc() is ok, from is_flattenable
        if (cdesc->masterCell()->isElectrical()) {
            CDp_name *pn = (CDp_name*)cdesc->prpty(P_NAME);
            if (pn && pn->bound())
                Ulist()->RecordObjectChange(cursd, pn->bound(), 0);
        }

        tstk->TPush();
        tstk->TApplyTransform(cdesc);
        tstk->TPremultiply();
        CDap ap(cdesc);
        int tx, ty;
        tstk->TGetTrans(&tx, &ty);
        xyg_t xyg(0, ap.nx - 1, 0, ap.ny - 1);
        do {
            tstk->TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
            if (!flatten_cell_recurse(tstk, calldesc.celldesc(), 0, 1,
                    &mtf, false, false)) {
                tstk->TPop();
                return (false);
            }
            tstk->TSetTrans(tx, ty);
        } while (xyg.advance());
        tstk->TPop();
        Ulist()->RecordObjectChange(cursd, cdesc, 0);
    }
    return (true);
}


// Transform the object and add it to the top level.
//
bool
cEdit::promote_object(cTfmStack *tstk, CDo *odesc, CDs *sdesc, CDtf *tfold,
    CDtf *tfnew, bool use_merge, bool FastMode)
{
    if (!odesc)
        return (false);
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
    {
        // So that the length and widths are rotated, we have to
        // convert to a BBox and transform the corner points.
        //
        BBox BB = odesc->oBB();
        Poly poly;
        tstk->TBB(&BB, &poly.points);
        if (FastMode) {
            CDo *newo;
            if (poly.points) {
                poly.numpts = 5;
                if (sdesc->makePolygon(odesc->ldesc(), &poly,
                        (CDpo**)&newo) != CDok) {
                    Errs()->add_error("makePolygon failed");
                    return (false);
                }
            }
            else {
                if (sdesc->makeBox(odesc->ldesc(), &BB, &newo) != CDok) {
                    Errs()->add_error("makeBox failed");
                    return (false);
                }
            }
            newo->prptyAddCopyList(odesc->prpty_list());
        }
        else {
            if (poly.points) {
                poly.numpts = 5;
                CDo *newo = sdesc->newPoly(0, &poly, odesc->ldesc(),
                    odesc->prpty_list(), false);
                if (!newo) {
                    Errs()->add_error("newPoly failed");
                    return (false);
                }
                if (use_merge) {
                    tstk->TLoadCurrent(tfold);
                    bool ret = sdesc->mergeBoxOrPoly(newo, true);
                    tstk->TLoadCurrent(tfnew);
                    if (!ret) {
                        Errs()->add_error("mergeBoxOrPoly failed");
                        return (false);
                    }
                }
            }
            else {
                CDo *newo = sdesc->newBox(0, &BB, odesc->ldesc(),
                    odesc->prpty_list());
                if (!newo) {
                    Errs()->add_error("newBox failed");
                    return (false);
                }
                if (use_merge) {
                    tstk->TLoadCurrent(tfold);
                    bool ret = sdesc->mergeBoxOrPoly(newo, true);
                    tstk->TLoadCurrent(tfnew);
                    if (!ret) {
                        Errs()->add_error("mergeBoxOrPoly failed");
                        return (false);
                    }
                }
            }
        }
        return (true);
    }
poly:
    {
        int num = ((const CDpo*)odesc)->numpts();
        Poly po(num, Point::dup_with_xform(((const CDpo*)odesc)->points(),
            tstk, num));
        CDpo *newo;
        if (FastMode) {
            if (sdesc->makePolygon(odesc->ldesc(), &po, &newo) != CDok) {
                Errs()->add_error("makePolygon failed");
                return (false);
            }
        }
        else {
            newo = sdesc->newPoly(0, &po, odesc->ldesc(), odesc->prpty_list(),
                false);
            if (!newo) {
                Errs()->add_error("newPoly failed");
                return (false);
            }
            if (use_merge) {
                tstk->TLoadCurrent(tfold);
                bool ret = sdesc->mergeBoxOrPoly(newo, true);
                tstk->TLoadCurrent(tfnew);
                if (!ret) {
                    Errs()->add_error("mergeBoxOrPoly failed");
                    return (false);
                }
            }
        }
        return (true);
    }
wire:
    {
        Wire wire(((const CDw*)odesc)->numpts(), 0,
            ((const CDw*)odesc)->attributes());
        wire.points = Point::dup_with_xform(((const CDw*)odesc)->points(),
            tstk, wire.numpts);
        if (!ED()->noWireWidthMag())
            wire.set_wire_width(mmRnd(wire.wire_width()*tfnew->mag()));
        CDw *newo;
        if (FastMode) {
            if (sdesc->makeWire(odesc->ldesc(), &wire, &newo) != CDok) {
                Errs()->add_error("makeWire failed");
                return (false);
            }
            newo->prptyAddCopyList(odesc->prpty_list());
        }
        else {
            newo = sdesc->newWire(0, &wire, odesc->ldesc(),
                odesc->prpty_list(), false);
            if (!newo) {
                Errs()->add_error("newWire failed");
                return (false);
            }
            if (use_merge) {
                tstk->TLoadCurrent(tfold);
                bool ret = sdesc->mergeWire(newo, true);
                tstk->TLoadCurrent(tfnew);
                if (!ret) {
                    Errs()->add_error("mergeWire failed");
                    return (false);
                }
            }
        }
        return (true);
    }
label:
    {
        CDp_lref *prf = (CDp_lref*)odesc->prpty(P_LABRF);
        if (prf && prf->devref())
            // should have already been added
            return (true);
        Label label(((CDla*)odesc)->la_label());
        if (tfnew->mag() >= 0 && tfnew->mag() != 1.0) {
            label.width = mmRnd(label.width * tfnew->mag());
            label.height = mmRnd(label.height * tfnew->mag());
        }
        tstk->TPoint(&label.x, &label.y);

        // have to fix transform
        tstk->TSetTransformFromXform(label.xform, 0, 0);
        tstk->TPremultiply();
        CDtf tf;
        tstk->TCurrent(&tf);
        label.xform &= ~TXTF_XF;
        label.xform |= (tf.get_xform() & TXTF_XF);
        tstk->TPop();

        CDla *newo;
        if (FastMode) {
            label.label = label.label->dup();
            if (sdesc->makeLabel(odesc->ldesc(), &label, &newo) != CDok) {
                Errs()->add_error("makeLabel failed");
                return (false);
            }
            newo->prptyAddCopyList(odesc->prpty_list());
        }
        else {
            newo = sdesc->newLabel(0, &label, odesc->ldesc(),
                odesc->prpty_list(), true);
            if (!newo) {
                Errs()->add_error("newLabel failed");
                return (false);
            }
        }
        return (true);
    }
inst:
    {
        CallDesc calldesc;
        ((CDc*)odesc)->call(&calldesc);
        tstk->TPush();
        tstk->TApplyTransform((CDc*)odesc);
        tstk->TPremultiply();
        CDtx tx;
        tstk->TCurrent(&tx);
        tstk->TPop();
        CDap ap((CDc*)odesc);
        CDc *cpointer;
        if (OIfailed(sdesc->makeCall(&calldesc, &tx, &ap, CDcallNone,
                &cpointer))) {
            Errs()->add_error("makeCall failed");
            return (false);
        }
        if (!FastMode)
            Ulist()->RecordObjectChange(sdesc, 0, cpointer);

        // free any default properties and copy the old ones
        cpointer->prptyFreeList();
        cpointer->prptyAddCopyList(odesc->prpty_list());
        for (CDp *pd = cpointer->prpty_list(); pd; pd = pd->next_prp())
            pd->transform(tstk);

        if (DSP()->CurMode() == Electrical) {
            bool mutref = false;
            for (CDp *pdesc = cpointer->prpty_list(); pdesc;
                    pdesc = pdesc->next_prp()) {
                CDla *olabel = pdesc->bound();
                if (pdesc->value() == P_MUTLRF)
                    mutref = true;
                else if (olabel) {
                    // create the label
                    Label label(olabel->la_label());
                    tstk->TPoint(&label.x, &label.y);
                    // have to fix transform
                    tstk->TSetTransformFromXform(label.xform, 0, 0);
                    tstk->TPremultiply();
                    CDtf tf;
                    tstk->TCurrent(&tf);
                    label.xform &= ~TXTF_XF;
                    label.xform |= (tf.get_xform() & TXTF_XF);
                    tstk->TPop();
                    Errs()->init_error();
                    CDla *nlabel = sdesc->newLabel(0, &label, olabel->ldesc(),
                        olabel->prpty_list(), true);
                    if (!nlabel) {
                        Errs()->add_error("newLabel failed");
                        Log()->ErrorLog(mh::ObjectCreation,
                            Errs()->get_error());
                        continue;
                    }
                    // update pointers
                    pdesc->bind(nlabel);
                    nlabel->link(sdesc, cpointer, pdesc);
                }
            }
            if (mutref) {
                // Patch the pointer in the P_NEWMUT property, odesc
                // can have any number of references.
                for (CDp *pdesc = sdesc->prptyList(); pdesc;
                        pdesc = pdesc->next_prp()) {
                    if (pdesc->value() == P_NEWMUT) {
                        CDp_nmut *pm = (CDp_nmut*)pdesc;
                        if (pm->l1_dev() == odesc)
                            pm->set_l1_dev(cpointer);
                        if (pm->l2_dev() == odesc)
                            pm->set_l2_dev(cpointer);
                    }
                }
            }
        }
        return (true);
    }
}


bool
cEdit::flatten_cell_recurse(cTfmStack *tstk, CDs *sdesc, int depth,
    int hierlev, CDtf *mtf, bool use_merge, bool fastmode)
{
    Errs()->init_error();
    if (tstk->TFull()) {
        Errs()->add_error(
            "transform stack overflow, cell hierarchy recursive?");
        return (false);
    }
    CDs *cursd = CurCell();
    if (!cursd || cursd->isSymbolic())
        return (false);
    if (DSP()->CurMode() == Electrical) {
        // mutual inductors
        for (CDp *pdesc = sdesc->prptyList(); pdesc;
                pdesc = pdesc->next_prp()) {
            if (pdesc->value() == P_MUT) {
                CDp_mut *pdesc1 = (CDp_mut*)cursd->prptyAddCopy(pdesc);
                if (pdesc1)
                    pdesc1->transform(tstk);
            }
            else if (pdesc->value() == P_NEWMUT) {
                CDp_nmut *pdesc1 = (CDp_nmut*)cursd->prptyAddCopy(pdesc);
                if (pdesc1) {
                    // create the label
                    CDla *olabel = ((CDp_nmut*)pdesc)->bound();
                    if (olabel) {
                        Label label(OLABEL(olabel)->la_label());
                        tstk->TPoint(&label.x, &label.y);

                        // have to fix transform
                        tstk->TSetTransformFromXform(label.xform, 0, 0);
                        tstk->TPremultiply();
                        CDtf tf;
                        tstk->TCurrent(&tf);
                        label.xform &= ~TXTF_XF;
                        label.xform |= (tf.get_xform() & TXTF_XF);
                        tstk->TPop();

                        CDla *nlabel = cursd->newLabel(0, &label,
                            olabel->ldesc(), olabel->prpty_list(), true);
                        if (!nlabel) {
                            Errs()->add_error("newLabel failed");
                            Log()->ErrorLog(mh::ObjectCreation,
                                Errs()->get_error());
                            continue;
                        }
                        // update pointers
                        pdesc1->bind(nlabel);
                        nlabel->link(cursd, 0, 0);
                    }
                }
            }
        }
    }

    // Traverse calls
    CDg gdesc;
    gdesc.init_gen(sdesc, CellLayer());
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        CallDesc calldesc;
        cdesc->call(&calldesc);
        if (hierlev > depth || !is_flattenable(cdesc)) {
            // add call to top level, also takes care of labels
            if (!promote_object(tstk, cdesc, cursd, 0, 0, use_merge,
                    fastmode)) {
                Errs()->add_error("flatten failed");
                Log()->ErrorLog(mh::Flatten, Errs()->get_error());
                continue;
            }
        }
        else {
            tstk->TPush();
            tstk->TApplyTransform(cdesc);
            tstk->TPremultiply();
            CDap ap(cdesc);
            int tx, ty;
            tstk->TGetTrans(&tx, &ty);
            xyg_t xyg(0, ap.nx - 1, 0, ap.ny - 1);
            do {
                tstk->TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                if (!flatten_cell_recurse(tstk, calldesc.celldesc(), depth,
                        hierlev + 1, mtf, use_merge, fastmode)) {
                    tstk->TPop();
                    return (false);
                }
                tstk->TSetTrans(tx, ty);
            } while (xyg.advance());
            tstk->TPop();
        }
    }

    FB.check();

    CDtf tfcur;
    tstk->TCurrent(&tfcur);
    const char *msg =
        "Interrupted!  Do you want to abort (your cell may be a mess!) ? ";
    // Quitting in the middle of a flatten is not a good thing to do.

    unsigned long check_time = 0;
    CDsLgen lgen(sdesc);
    CDl *ld;
    while ((ld = lgen.next()) != 0) {
        gdesc.init_gen(sdesc, ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (!odesc->is_normal())
                continue;
            if (Timer()->check_interval(check_time)) {
                dspPkgIf()->CheckForInterrupt();
                if (DSP()->Interrupt()) {
                    tstk->TPush();
                    tstk->TLoadCurrent(mtf);
                    bool ret = XM()->ConfirmAbort(msg);
                    tstk->TPop();
                    if (ret)
                        return (false);
                    dspPkgIf()->CheckForInterrupt();
                }
            }
            if (!promote_object(tstk, odesc, cursd, mtf, &tfcur, use_merge,
                    fastmode)) {
                Errs()->add_error("flatten failed");
                Log()->ErrorLog(mh::Flatten, Errs()->get_error());
            }
        }
    }
    return (true);
}

