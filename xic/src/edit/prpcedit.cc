
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
#include "edit.h"
#include "undolist.h"
#include "pcell.h"
#include "pcell_params.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "promptline.h"
#include "errorlog.h"


//----------------------------------------------------------------------
// The cprop command - modify properties of the current cell using
// PopUpCellProperties()
//----------------------------------------------------------------------

namespace {
    // Return true if the property is not alterable through the pop-up.
    //
    bool
    is_reserved(int num)
    {
        if (DSP()->CurMode() == Electrical) {
            switch (num) {
            case P_PARAM:
            case P_OTHER:
            case P_VIRTUAL:
            case P_FLATTEN:
            case P_MACRO:
                return (false);
            default:
                break;
            }
            return (true);
        }
        if (prpty_gdsii(num))
            return (true);
        if (prpty_global(num))
            return (true);
        if (prpty_reserved(num))
            return (true);
        if (num == XICP_PC)
            return (true);
        return (false);
    }


    // Return true if property number pnum is valid for editing.
    bool prpty_editable(int pnum)
    {
        if (is_reserved(pnum))
            return (false);
        if (DSP()->CurMode() == Electrical) {
            switch (pnum) {
            case P_VIRTUAL:     // boolean, not editable
            case P_FLATTEN:     // boolean, not editable
            case P_MACRO:       // boolean, not editable
                break;
            default:
                return (true);
            }
            return (false);
        }
        switch (pnum) {
        case XICP_EXT_FLATTEN:  // booleam, not editable
            break;
        default:
            return (true);
        }
        return (false);
    }


    // Return true if property number pnum is valid for removable.
    bool prpty_removable(int pnum)
    {
        if (is_reserved(pnum))
            return (false);
        switch (pnum) {
        case XICP_PC_PARAMS:
        case XICP_PC_SCRIPT:
            break;
        default:
            return (true);
        }
        return (false);
    }


    // Return true if the property number should remain fixed.
    //
    bool
    is_fixed(int num)
    {
        switch (num) {
        case P_PARAM:
        case P_OTHER:
        case P_VIRTUAL:
        case P_FLATTEN:
        case P_MACRO:
        case XICP_FLAGS:
        case XICP_EXT_FLATTEN:
        case XICP_PC_SCRIPT:
        case XICP_PC_PARAMS:
            return (true);
        default:
            break;
        }
        return (false);
    }


    void
    c_prompt(char *buf, int which)
    {
        sprintf(buf, "Property %d string? ", which);
        switch (which) {
        case P_PARAM:
            if (DSP()->CurMode() != Physical)
                strcpy(buf, "Parameters? ");
            break;
        case P_OTHER:
            if (DSP()->CurMode() != Physical)
                strcpy(buf, "String? ");
            break;
        case P_VIRTUAL:
        case P_FLATTEN:
        case P_MACRO:
            break;
        case XICP_PC_SCRIPT:
            if (DSP()->CurMode() == Physical)
                strcpy(buf, "PCell Script? ");
            break;
        case XICP_PC_PARAMS:
            if (DSP()->CurMode() == Physical)
                strcpy(buf, "PCell Params? ");
            break;
        default:
            break;
        }
    }


    CDp *
    prpmatch(CDs *sdesc, CDp *pdesc, int val)
    {
        if (!sdesc)
            return (0);
        if (DSP()->CurMode() == Electrical) {
            if (!pdesc) {
                if (val == P_PARAM)
                    return (sdesc->prpty(val));
            }
            else {
                if (pdesc->value() == P_PARAM)
                    return (sdesc->prpty(pdesc->value()));
                if (pdesc->value() == P_OTHER) {
                    // Return a P_OTHER property with matching text
                    char *s1 = hyList::string(((CDp_user*)pdesc)->data(),
                        HYcvPlain, false);
                    for (CDp *pd = sdesc->prptyList(); pd;
                            pd = pd->next_prp()) {
                        if (pd->value() == P_OTHER) {
                            char *s2 = hyList::string(((CDp_user*)pd)->data(),
                                HYcvPlain, false);
                            int j = strcmp(s1, s2);
                            delete [] s2;
                            if (!j) {
                                delete [] s1;
                                return (pd);
                            }
                        }
                    }
                    delete [] s1;
                }
            }
        }
        else {
            if (!pdesc) {
                if (is_fixed(val))
                    return (sdesc->prpty(val));
            }
            else {
                if (is_fixed(pdesc->value()))
                    return (sdesc->prpty(pdesc->value()));
                if (pdesc->string()) {
                    for (CDp *pd = sdesc->prptyList(); pd;
                            pd = pd->next_prp()) {
                        if (pd->value() == pdesc->value() && pd->string() &&
                                !strcmp(pdesc->string(), pd->string()))
                            return (pd);
                    }
                }
            }
        }
        return (0);
    }


    struct c_ltobj
    {
        c_ltobj(CDs *s, CDp *p, int v)
            { sdesc = s; pdesc = p ? p->dup() : 0, value = v; }
        ~c_ltobj() { delete pdesc; }

        CDs *sdesc;
        CDp *pdesc;
        int value;
    };

    // Callback from the "long text" editor.
    //
    void
    c_ltcb(hyList *h, void *arg)
    {
        c_ltobj *lt = (c_ltobj*)arg;
        if (!lt)
            return;
        CDp *newp = 0;
        if (lt->value == P_PARAM || lt->value == P_OTHER) {
            CDp_user *pu = new CDp_user(lt->value);
            pu->set_data(h);
            newp = pu;
        }
        else {
            char *string;
            if (h->ref_type() == HLrefLongText)
                string = hyList::string(h, HYcvAscii, true);
            else
                string = hyList::string(h, HYcvPlain, true);
            if (string && *string)
                newp = new CDp(string, lt->value);
            delete [] string;
            hyList::destroy(h);
        }
        if (newp) {
            Ulist()->ListCheckPush("edcprp", lt->sdesc, false, false);
            CDp *pdesc = prpmatch(lt->sdesc, lt->pdesc, lt->value);
            // RecordPrptyChange deletes pdesc
            Ulist()->RecordPrptyChange(lt->sdesc, 0, pdesc, newp);
            Ulist()->ListPop();
            ED()->PopUpCellProperties(MODE_UPD);
        }
        delete lt;
    }


}


// Main function to add a cell property.
//
void
cEdit::cellPrptyAdd(int which)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    char tbuf[64];
    *tbuf = '\0';
    if (!is_fixed(which)) {
        if (which >= 0)
            sprintf(tbuf, "%d", which);
        which = -1;
        char *in = PL()->EditPrompt("Property number? ", tbuf);
        PL()->ErasePrompt();
        if (!in)
            return;
        int d;
        if (sscanf(in, "%d", &d) == 1 && d >= 0)
            which = d;
    }
    if (which < 0) {
        PL()->ShowPrompt("Negative property number not allowed.");
        return;
    }
    if (is_reserved(which)) {
        PL()->ShowPrompt("Can't add property, number reserved.");
        return;
    }
    if (which == XICP_PC_PARAMS) {
        // Use a modal version of the Parameters pop-up instead of the
        // prompt line, if editing.

        CDp *pdesc = cursd->prpty(XICP_PC_PARAMS);
        if (pdesc) {
            char *newstr = 0;
            CDp *pn = cursd->prpty(XICP_PC);
            if (pn) {
                // a sub-master.

                char *dbname = PCellDesc::canon(pn->string());
                PCellParam *pm;
                if (PC()->getDefaultParams(dbname, &pm))
                    pm->update(pdesc->string());
                else {
                    Log()->ErrorLogV(mh::PCells,
                        "Can't find default parameters for %s.", dbname);
                    delete [] dbname;
                    return;
                }
                delete [] dbname;

                if (PopUpPCellParams(0, MODE_ON, pm, pn->string(), pcpEdit)) {
                    newstr = pm->string(true);
                    PCellParam::destroy(pm);
                }
                else {
                    PCellParam::destroy(pm);
                    return;
                }
            }
            else if (cursd->isPCellSuperMaster()) {
                PCellParam *pm;
                if (!PCellParam::parseParams(pdesc->string(), &pm)) {
                    Log()->ErrorLogV(mh::PCells,
                        "Parse error in property string:\n%s",
                        Errs()->get_error());
                    return;
                }
                if (PopUpPCellParams(0, MODE_ON, pm,
                        Tstring(cursd->cellname()), pcpEdit)) {
                    newstr = pm->string(false);
                    PCellParam::destroy(pm);
                }
                else {
                    PCellParam::destroy(pm);
                    return;
                }
            }
            if (newstr) {
                hyList *hnew = new hyList(cursd, newstr, HYcvPlain);
                c_ltobj *lt = new c_ltobj(cursd, pdesc, pdesc->value());
                c_ltcb(hnew, lt);
            }
            return;
        }
    }

    CDp *pdesc = 0;
    hyList *hp = 0;
    if (DSP()->CurMode() != Physical) {
        if (which == P_PARAM) {
            pdesc = cursd->prpty(which);
            if (pdesc)
                hp = pdesc->hpstring(cursd);
        }
        if (which == P_VIRTUAL) {
            if (cursd->prpty(P_VIRTUAL))
                return;
            Ulist()->ListCheckPush("edcprp", cursd, false, false);
            CDp *newp = new CDp("virtual", P_VIRTUAL);
            Ulist()->RecordPrptyChange(cursd, 0, 0, newp);
            Ulist()->ListPop();
            ED()->PopUpCellProperties(MODE_UPD);
            return;
        }
        if (which == P_FLATTEN) {
            if (cursd->prpty(P_FLATTEN))
                return;
            Ulist()->ListCheckPush("edcprp", cursd, false, false);
            CDp *newp = new CDp("flatten", P_FLATTEN);
            Ulist()->RecordPrptyChange(cursd, 0, 0, newp);
            Ulist()->ListPop();
            ED()->PopUpCellProperties(MODE_UPD);
            return;
        }
    }
    else {
        if (which == XICP_EXT_FLATTEN) {
            if (cursd->prpty(XICP_EXT_FLATTEN))
                return;
            Ulist()->ListCheckPush("edcprp", cursd, false, false);
            CDp *newp = new CDp("flatten", XICP_EXT_FLATTEN);
            Ulist()->RecordPrptyChange(cursd, 0, 0, newp);
            Ulist()->ListPop();
            ED()->PopUpCellProperties(MODE_UPD);
            return;
        }
    }
    c_prompt(tbuf, which);

    c_ltobj *lt = new c_ltobj(cursd, pdesc, which);
    hyList *hnew = PL()->EditHypertextPrompt(tbuf, hp, true, PLedStart,
        PLedNormal, c_ltcb, lt);
    hyList::destroy(hp);
    PL()->ErasePrompt();
    if (!hnew) {
        delete lt;
        return;
    }
    if (hnew->ref_type() == HLrefLongText)
        // text editor popped, calls c_ltcb when done
        return;
    c_ltcb(hnew, lt);
}


// Main function to edit a cell property.
//
void
cEdit::cellPrptyEdit(Ptxt *line)
{
    if (!line)
        return;
    CDp *pdesc = line->prpty();
    if (!pdesc)
        return;
    if (!prpty_editable(pdesc->value())) {
        PL()->ShowPrompt("This property can not be edited.");
        return;
    }
    CDs *cursd = CurCell();
    if (!cursd)
        return;

    if (pdesc->value() == XICP_PC_PARAMS) {
        // Use a modal version of the Parameters pop-up instead of the
        // prompt line.

        char *newstr = 0;
        CDp *pn = cursd->prpty(XICP_PC);
        if (pn) {
            // a sub-master.

            char *dbname = PCellDesc::canon(pn->string());
            PCellParam *pm;
            if (PC()->getDefaultParams(dbname, &pm))
                pm->update(pdesc->string());
            else {
                Log()->ErrorLogV(mh::PCells,
                    "Can't find default parameters for %s.", dbname);
                delete [] dbname;
                return;
            }
            delete [] dbname;

            if (PopUpPCellParams(0, MODE_ON, pm, pn->string(), pcpEdit)) {
                newstr = pm->string(true);
                PCellParam::destroy(pm);
            }
            else {
                PCellParam::destroy(pm);
                return;
            }
        }
        else if (cursd->isPCellSuperMaster()) {
            PCellParam *pm;
            if (!PCellParam::parseParams(pdesc->string(), &pm)) {
                Log()->ErrorLogV(mh::PCells,
                    "Parse error in property string:\n%s",
                    Errs()->get_error());
                return;
            }
            if (PopUpPCellParams(0, MODE_ON, pm,
                    Tstring(cursd->cellname()), pcpEdit)) {
                newstr = pm->string(false);
                PCellParam::destroy(pm);
            }
            else {
                PCellParam::destroy(pm);
                return;
            }
        }
        if (newstr) {
            hyList *hnew = new hyList(cursd, newstr, HYcvPlain);
            c_ltobj *lt = new c_ltobj(cursd, pdesc, pdesc->value());
            c_ltcb(hnew, lt);
        }
        return;
    }

    hyList *hp = pdesc->hpstring(cursd);
    char tbuf[64];
    c_prompt(tbuf, pdesc->value());
    c_ltobj *lt = new c_ltobj(cursd, pdesc, pdesc->value());
    hyList *hnew = PL()->EditHypertextPrompt(tbuf, hp, true, PLedStart,
        PLedNormal, c_ltcb, lt);
    hyList::destroy(hp);
    PL()->ErasePrompt();
    if (!hnew) {
        delete lt;
        return;
    }
    if (hnew->ref_type() == HLrefLongText)
        // text editor popped, calls c_ltcb when done
        return;
    c_ltcb(hnew, lt);
}


// Main function to remove a cell property.
//
void
cEdit::cellPrptyRemove(Ptxt *line)
{
    if (!line)
        return;
    CDp *pdesc = line->prpty();
    if (!pdesc)
        return;
    if (!prpty_removable(pdesc->value())) {
        PL()->ShowPrompt("This property can not be removed.");
        return;
    }
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    Ulist()->ListCheckPush("rmcprp", cursd, false, false);
    // RecordPrptyChange deletes pdesc
    Ulist()->RecordPrptyChange(cursd, 0, pdesc, 0);
    Ulist()->ListPop();
    PopUpCellProperties(MODE_UPD);
}

