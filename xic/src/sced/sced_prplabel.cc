
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
#include "sced.h"
#include "edit.h"
#include "undolist.h"
#include "scedif.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "cd_hypertext.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "events.h"
#include "layertab.h"
#include "errorlog.h"
#include "miscutil/texttf.h"


// If regen is false, fix or set the labels when replacing old.  If
// old is 0, this is a newly placed subcell.  The properties have
// already been copied or created.  If regen is true, regenerate any
// missing property labels.
//
void
cSced::genDeviceLabels(CDc *cdesc, CDc *old, bool regen)
{
    if (!cdesc || cdesc->type() != CDINSTANCE)
        return;
    // Have to do the name property last, otherwise when the name property
    // is unlinked, linking in new properties will fail.
    //
    CDp *pd, *pnxt;
    for (pd = cdesc->prpty_list(); pd; pd = pnxt) {
        pnxt = pd->next_prp();
        if (pd->value() != P_NAME && !(regen && pd->bound())) {
            CDp *opd = (old ? old->prpty(pd->value()) : 0);
            addDeviceLabel(cdesc, pd, opd, 0, regen, false);
        }
    }
    pd = cdesc->prpty(P_NAME);
    if (pd && !(regen && pd->bound())) {
        CDp *opd = (old ? old->prpty(pd->value()) : 0);
        addDeviceLabel(cdesc, pd, opd, 0, regen, false);
    }
}


// Update the name label according to pna.
//
void
#ifdef NEWNMP
cSced::updateNameLabel(CDc *cdesc, CDp_cname *pna)
#else
cSced::updateNameLabel(CDc *cdesc, CDp_name *pna)
#endif
{
    if (!cdesc || cdesc->type() != CDINSTANCE)
        return;
    if (!pna || !pna->name_string() || !pna->bound())
        return;
    if (!isalpha(pna->key()))
        return;
    bool copied;  // always returned true
    hyList *htxt = pna->label_text(&copied, cdesc);
    BBox BB;
    if (!updateLabelText(pna->bound(), cdesc->parent(), htxt, &BB))
        hyList::destroy(htxt);
    else
        DSP()->RedisplayArea(&BB, Electrical);
}


// Update the label text, no undo.  Return true if the label is
// updated, in which case the htxt is used NOT COPIED, and the second
// arg if non-nil is the redisplay area.
//
bool
cSced::updateLabelText(CDla *ladesc, CDs *sdesc, hyList *htxt, BBox *BB)
{
    if (!ladesc || ladesc->type() != CDLABEL)
        return (false);
    if (hyList::hy_strcmp(htxt, ladesc->label())) {
        // text has changed, update
        int tw = ladesc->width();
        int th = ladesc->height();
        DSP()->LabelResize(htxt, ladesc->label(), &tw, &th);
        ladesc->set_width(tw);
        ladesc->set_height(th);
        hyList::destroy(ladesc->label());
        ladesc->set_label(htxt);
        if (BB)
            *BB = ladesc->oBB();
        BBox tBB(ladesc->oBB());
        tBB.left = ladesc->xpos();
        tBB.bottom = ladesc->ypos();
        tBB.right = ladesc->xpos() + ladesc->width();
        tBB.top = ladesc->ypos() + ladesc->height();
        Label::TransformLabelBB(ladesc->xform(), &tBB, 0);
        if (sdesc)
            sdesc->reinsert(ladesc, &tBB);
        if (BB)
            BB->add(&ladesc->oBB());
        return (true);
    }
    return (false);
}


// Change the label text, resetting associated properties.  Takes care of
// redisplay and undo list.
//
CDla *
cSced::changeLabel(CDla *ladesc, CDs *sdesc, hyList *lastr)
{
    if (!sdesc || !sdesc->isElectrical())
        return (0);
    if (!ladesc || ladesc->type() != CDLABEL)
        return (0);

    const char *msg = "Bad value for mutual inductor K factor.";
    const char *label_change = "label change";
    Errs()->init_error();
    bool wire_label = false;
    CDp_nmut *pm = 0;
    CDp_lref *prf = (CDp_lref*)ladesc->prpty(P_LABRF);
    if (prf && prf->devref()) {
        if (lastr)
            lastr->trim_white_space();
        if (prf->propref() && prf->devref()->type() == CDINSTANCE) {
            // device property
            CDp *pd = 0;
            if (((CDo*)prf->devref())->type() == CDINSTANCE) {
                // shouldn't be other than a call
                // This takes care of creating new label, display, etc.
                pd = prptyModify(prf->devref(), 0, prf->propref()->value(), 0,
                    lastr);
            }
            else
                return (0);
            if (!pd)
                return (0);
            CDla *nlabel = pd->bound();
            if (!nlabel)
                return (0);
            return (nlabel);
        }
        if (prf->devref()->type() == CDWIRE) {
            // Wire node/bnode name label.
            wire_label = true;
        }
        else {
            // mutual inductor
            // The label has the form "name=value" or just "value", where
            // name, if given and not the previous name, becomes an alias
            // for the mutual inductor.
            //
            double val = 0;
            char devn[128], buf[128];
            char *string = hyList::string(lastr, HYcvPlain, false);
            ScedIf()->mutParseName(string, devn, buf);
            delete [] string;

            // take any symbol, but balk at out of range numeric
            if (sscanf(buf, "%lf", &val) == 1 && (val < -1 || val > 1)) {
                Log()->ErrorLog(label_change, msg);
                return (0);
            }
            pm = (CDp_nmut*)sdesc->prpty(P_NEWMUT);
            while (pm && pm->bound() != ladesc)
                pm = pm->next();
            if (pm) {
                pm->set_coeff_str(buf);
                if (!pm->rename(sdesc, devn))
                    Log()->ErrorLog(label_change, Errs()->get_error());
            }
            else
                Log()->ErrorLog(label_change,
                    "Internal Error: mutual not found.");
        }
    }
    Label label(ladesc->la_label());
    if (!ED()->labelOverride(&label))
        DSP()->LabelResize(lastr, label.label, &label.width, &label.height);
    label.label = lastr;
    CDla *nlabel = sdesc->newLabel(ladesc, &label, ladesc->ldesc(),
        ladesc->prpty_list(), true);
    if (!nlabel) {
        Errs()->add_error("newLabel failed");
        Log()->ErrorLog(label_change, Errs()->get_error());
        return (0);
    }
    if (wire_label) {
        // bind for wire above
        sdesc->prptyLabelUpdate(nlabel, ladesc);
    }
    else if (pm) {
        // bind for mutual above
        pm->bind(nlabel);
        nlabel->link(sdesc, 0, 0);
    }
    BBox BB = ladesc->oBB();
    BB.add(&nlabel->oBB());
    DSP()->RedisplayArea(&BB);
    return (nlabel);
}


// Add a label next to the device showing the property.  If pdesc is
// 0, the oldp is simply being deleted.  If oldp is 0, pdesc is
// a new property.
// CDp *pdesc;  property
// CDp *oldp;   old property, use this position if not null
// hyList *hstring;  overrides the label text
//
void
cSced::addDeviceLabel(CDc *cdesc, CDp *pdesc, CDp *oldp, hyList *hstring,
    bool regen, bool redraw)
{
    if (!cdesc || cdesc->type() != CDINSTANCE)
        return;
    CDla *olabel = 0;
    CDs *sdesc = cdesc->parent();

    if (oldp && (olabel = oldp->bound()) != 0) {
        Ulist()->RecordObjectChange(sdesc, olabel, 0);
        if (redraw)
            // erase old label
            DSP()->RedisplayArea(&olabel->oBB(), Electrical);
    }
    if (!pdesc)
        return;
    if (pdesc->value() == P_NAME) {
        // The "null" devices don't have a name label.
#ifdef NEWNMP
        CDp_cname *pa = (CDp_cname*)pdesc;
#else
        CDp_name *pa = (CDp_name*)pdesc;
#endif
        int key = pa->key();
        if (key != P_NAME_TERM && !isalpha(key))
            return;
    }
    bool copied = false;
    Label label;
    label.label = hstring ? hstring : pdesc->label_text(&copied, cdesc);
    if (!label.label)
        return;
    if (olabel) {
        label.x = olabel->xpos();
        label.y = olabel->ypos();
        if (!ED()->labelOverride(&label)) {
            label.width = olabel->width();
            label.height = olabel->height();
            label.xform = olabel->xform();
            DSP()->LabelResize(label.label, olabel->label(),
                &label.width, &label.height);
        }
    }
    else {
        if (pdesc->value() == P_VALUE || pdesc->value() == P_PARAM)
            label.xform |= TXTF_LIML;

        if (!ED()->labelOverride(&label))
            DSP()->DefaultLabelSize(label.label, Electrical,
                &label.width, &label.height);
        labelPlacement(pdesc->value(), cdesc, &label);
    }
    if (sdesc) {
        Errs()->init_error();
        CDla *nlabel = sdesc->newLabel(0, &label,
            olabel ? olabel->ldesc() : defaultLayer(pdesc),
            olabel ? olabel->prpty_list() : 0, true);
        if (copied)
            hyList::destroy(label.label);
        if (!nlabel) {
            Errs()->add_error("newLabel failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            return;
        }
        if (redraw)
            DSP()->RedisplayArea(&nlabel->oBB(), Electrical);
        if (regen || oldp == pdesc) {
            // replace pd, as pOdesc field changes
            CDp *op = pdesc;
            pdesc = pdesc->dup();
            Ulist()->RecordPrptyChange(sdesc, cdesc, op, pdesc);
        }
        pdesc->bind(nlabel);
        if (!nlabel->link(sdesc, cdesc, pdesc)) {
            Errs()->add_error("link failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        }
    }
}


// Reposition any bound labels that are too far outside of the
// instance BB.  This is not undoable.
//
int
cSced::checkRepositionLabels(const CDc *cdesc)
{
    CDs *sdesc = cdesc->parent();
    if (!sdesc)
        return (0);
    if (!sdesc->isElectrical())  
        return (0);

    int chgcnt = 0;
    BBox BB(cdesc->oBB());
    BB.bloat(5000);
    for (CDp *pd = cdesc->prpty_list(); pd; pd = pd->next_prp()) {
        CDla *la = pd->bound();
        if (!la)
            continue;

        if (!BB.intersect(&la->oBB(), true)) {

            sdesc->db_remove(la);

            int dx = 0, dy = 0;
            if (la->oBB().right + 1000 < cdesc->oBB().left)
                dx = cdesc->oBB().left - la->oBB().right - 1000;
            else if (la->oBB().left - 1000 > cdesc->oBB().right)
                dx = cdesc->oBB().right - la->oBB().left + 1000;
            if (la->oBB().top + 1000 < cdesc->oBB().bottom)
                dy = cdesc->oBB().bottom - la->oBB().top - 1000;
            else if (la->oBB().bottom - 1000 > cdesc->oBB().top)
                dy = cdesc->oBB().top - la->oBB().bottom + 1000;

            la->set_xpos(la->xpos() + dx);
            la->set_ypos(la->ypos() + dy);
            la->computeBB();

            sdesc->db_insert(la);
            chgcnt++;
        }
    }
    return (chgcnt);
}


namespace {

    // Set the location and justification of the label according to
    // code.  The '.' position implies the horizontal justification. 
    // All are default vertical justification except 17,18 which are
    // VJC, and 4,5,8,9, and 12,13,14,15,19 which are VJT.
    //
    //     .0     1.
    //     .2 1.6 3.
    //  4. --------- .8
    //  5. |.20 21.| .9
    // 17. |       | .18
    //  6. |.22 23.| .10
    //  7. --------- .11
    //    .12 1.9 13.
    //    .14     15.
    //

    // position mapping
    char my[24] =
        {14, 15, 12, 13, 7, 6, 5, 4, 11, 10, 9, 8, 2, 3, 0, 1, 19, 17, 18, 16,
        22, 23, 20, 21};

    char r90[24] =
        {4, 7, 5, 6, 14, 12, 13, 15, 0, 2, 3, 1, 9, 10, 8, 11, 17, 19, 16, 18,
        22, 20, 23, 21};

    char r180[24] =
        {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 16, 18, 17, 19,
        21, 20, 23, 22};

    char r270[24] =
        {8, 11, 9, 10, 0, 2, 3, 1, 14, 12, 13, 15, 5, 6, 4, 7, 18, 16, 19, 17,
        21, 23, 20, 22};


    // Set location and justification for the cell property label
    // according to the code.
    //
    void setloc(const BBox *BB, int thei, Label *label, unsigned int code)
    {
        int spa = thei/4;

#ifdef HAVE_COMPUTED_GOTO
        static void *array[] = {
            &&L_0,  &&L_1,  &&L_2,  &&L_3,  &&L_4,  &&L_5,  &&L_6,  &&l_7,
            &&L_8,  &&L_9,  &&L_10, &&L_11, &&L_12, &&L_13, &&L_14, &&l_15,
            &&L_16, &&L_17, &&L_18, &&L_19, &&L_20, &&L_21, &&L_22, &&l_23
        };
        if (code > 23)
            code = 0;
        goto array[code];
#else
        switch (code) {
        case 0: goto L__0;
        case 1: goto L__1;
        case 2: goto L__2;
        case 3: goto L__3;
        case 4: goto L__4;
        case 5: goto L__5;
        case 6: goto L__6;
        case 7: goto L__7;
        case 8: goto L__8;
        case 9: goto L__9;
        case 10: goto L__10;
        case 11: goto L__11;
        case 12: goto L__12;
        case 13: goto L__13;
        case 14: goto L__14;
        case 15: goto L__15;
        case 16: goto L__16;
        case 17: goto L__17;
        case 18: goto L__18;
        case 19: goto L__19;
        case 20: goto L__20;
        case 21: goto L__21;
        case 22: goto L__22;
        case 23: goto L__23;
        }
#endif

    L__0:
        label->x = BB->left + spa;
        label->y = BB->top + spa + thei;
        return;
    L__1:
        label->x = BB->right - spa;
        label->y = BB->top + spa + thei;
        label->xform |= TXTF_HJR;
        return;
    L__2:
        label->x = BB->left + spa;
        label->y = BB->top + spa;
        return;
    L__3:
        label->x = BB->right - spa;
        label->y = BB->top + spa;
        label->xform |= TXTF_HJR;
        return;
    L__4:
        label->x = BB->left - spa;
        label->y = BB->top - spa;
        label->xform |= (TXTF_HJR | TXTF_VJT);
        return;
    L__5:
        label->x = BB->left - spa;
        label->y = BB->top - spa - thei;
        label->xform |= (TXTF_HJR | TXTF_VJT);
        return;
    L__6:
        label->x = BB->left - spa;
        label->y = BB->bottom + spa + thei;
        label->xform |= TXTF_HJR;
        return;
    L__7:
        label->x = BB->left - spa;
        label->y = BB->bottom + spa;
        label->xform |= TXTF_HJR;
        return;
    L__8:
        label->x = BB->right + spa;
        label->y = BB->top - spa;
        label->xform |= TXTF_VJT;
        return;
    L__9:
        label->x = BB->right + spa;
        label->y = BB->top - spa - thei;
        label->xform |= TXTF_VJT;
        return;
    L__10:
        label->x = BB->right + spa;
        label->y = BB->bottom + spa + thei;
        return;
    L__11:
        label->x = BB->right + spa;
        label->y = BB->bottom + spa;
        return;
    L__12:
        label->x = BB->left + spa;
        label->y = BB->bottom - spa;
        label->xform |= TXTF_VJT;
        return;
    L__13:
        label->x = BB->right - spa;
        label->y = BB->bottom - spa;
        label->xform |= (TXTF_HJR | TXTF_VJT);
        return;
    L__14:
        label->x = BB->left + spa;
        label->y = BB->bottom - spa - thei;
        label->xform |= TXTF_VJT;
        return;
    L__15:
        label->x = BB->right - spa;
        label->y = BB->bottom - spa - thei;
        label->xform |= (TXTF_HJR | TXTF_VJT);
        return;
    L__16:
        label->x = (BB->left + BB->right)/2;
        label->y = BB->top + spa;
        label->xform |= TXTF_HJC;
        return;
    L__17:
        label->x = BB->left - spa;
        label->y = (BB->bottom + BB->top)/2;
        label->xform |= (TXTF_HJR | TXTF_VJC);
        return;
    L__18:
        label->x = BB->right + spa;
        label->y = (BB->bottom + BB->top)/2;
        label->xform |= TXTF_VJC;
        return;
    L__19:
        label->x = (BB->left + BB->right)/2;
        label->y = BB->bottom - spa;
        label->xform |= (TXTF_HJC | TXTF_VJT);
        return;
    L__20:
        label->x = BB->left + spa;
        label->y = BB->top - spa - thei;
        return;
    L__21:
        label->x = BB->right - spa;
        label->y = BB->top - spa - thei;
        label->xform |= TXTF_HJR;
        return;
    L__22:
        label->x = BB->left + spa;
        label->y = BB->bottom + spa;
        return;
    L__23:
        label->x = BB->right - spa;
        label->y = BB->bottom + spa;
        label->xform |= TXTF_HJR;
        return;
    }


    // Set default location and justification for the cell property label.
    //
    void
    setdefloc(const BBox *BB, int thei, Label *label, int type)
    {
        if (BB->height() > BB->width()) {
            switch (type) {
            case P_NAME:
                setloc(BB, thei, label, 5);
                break;
            case P_MODEL:
            case P_VALUE:
                setloc(BB, thei, label, 8);
                break;
            case P_PARAM:
                setloc(BB, thei, label, 11);
                break;
            case P_DEVREF:
                setloc(BB, thei, label, 18);
                break;
            }
        }
        else {
            switch (type) {
            case P_NAME:
                setloc(BB, thei, label, 2);
                break;
            case P_MODEL:
            case P_VALUE:
                setloc(BB, thei, label, 13);
                break;
            case P_PARAM:
                setloc(BB, thei, label, 14);
                break;
            case P_DEVREF:
                setloc(BB, thei, label, 12);
                break;
            }
        }
    }
}


// This function sets the locations of labels associated with an
// instance.
//
void
cSced::labelPlacement(int type, CDc *cdesc, Label *label)
{
    CDs *sdesc = cdesc->masterCell();
    int code = -1;
    if (sdesc) {
        CDp_labloc *pl = (CDp_labloc*)sdesc->prpty(P_LABLOC);
        if (pl) {
            switch (type) {
            case P_NAME:
                code = pl->name_code();
                break;
            case P_MODEL:
                code = pl->model_code();
                break;
            case P_VALUE:
                code = pl->value_code();
                break;
            case P_PARAM:
                code = pl->param_code();
                break;
            case P_DEVREF:
                code = pl->devref_code();
                break;
            }
        }
        if (code >= 0) {
            CDtx tx(cdesc);
            if (tx.refly)
                code = my[code];
            if (tx.ax != 1 || tx.ay != 0) {
                if (tx.ax == 0) {
                    if (tx.ay == 1)
                        code = r90[code];
                    else if (tx.ay == -1)
                        code = r270[code];
                }
                else if (tx.ax == -1 && tx.ay == 0)
                    code = r180[code];
            }
        }
    }
    label->xform &= ~(TXTF_HJC | TXTF_HJR | TXTF_VJC | TXTF_VJT);
    int wid, hei;
    DSP()->DefaultLabelSize((const char*)0, Electrical, &wid, &hei);
    if (code < 0)
        setdefloc(&cdesc->oBB(), hei, label, type);
    else
        setloc(&cdesc->oBB(), hei, label, code);
}

