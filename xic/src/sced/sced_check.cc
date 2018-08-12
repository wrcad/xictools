
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
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "cd_lgen.h"
#include "cd_terminal.h"
#include "fio.h"
#include "fio_cvt_base.h"
#include "errorlog.h"
#include "cvrt.h"
#include "select.h"
#include "promptline.h"
#include "oa_if.h"


// Check the electrical properties of the cells in the hierarchy.
// Any errors are appended to the log file, which is popped up
// or refreshed.  This must be called after the logfile is closed.
//
void
cSced::checkElectrical(CDcbin *cbin)
{
    if (!cbin || !cbin->elec())
        return;

    Cvt()->SetShowLog(false);
    const char *fn;
    FileType ft = cbin->fileType();
    if (ft == Fgds)
        fn = READGDS_FN;
    else if (ft == Fcgx)
        fn = READCGX_FN;
    else if (ft == Foas)
        fn = READOAS_FN;
    else if (ft == Fcif)
        fn = READCIF_FN;
    else if (ft == Foa)
        fn = READ_OA_FN;
    else
        fn = READXIC_FN;
    // if this fails, stderr will be used
    Cvt()->SetLogFp(Log()->OpenLog(fn, "a"));

    CDgenHierDn_s gen(cbin->elec());
    CDs *sd;
    bool err;
    while ((sd = gen.next(&err)) != 0) {
        if (sd->isDevice())
            continue;
        if (sd->isEmpty())
            continue;
        sd->unsetConnected();

        // Add vertices at cell connection points.
        CDp_snode *pn = (CDp_snode*)sd->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            if (!pn->has_flag(TE_BYNAME)) {
                int x, y;
                pn->get_schem_pos(&x, &y);
                SCD()->addConnection(sd, x, y, 0);
            }
        }

        int chgcnt = 0;
        CDm_gen mgen(sd, GEN_MASTERS);
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            if (!mdesc->parent())
                continue;
            CDc_gen cgen(mdesc);
            for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {

                // Initialize instance terminal locations.
                cdesc->updateTerminals(0);

                // Check associated label positioning, move labels
                // that are too far from the device.
                chgcnt += checkRepositionLabels(cdesc);
            }
        }
        if (chgcnt) {
            sd->setBBvalid(false);
            sd->computeBB();
        }

        if (!prptyCheck(sd,
                Cvt()->LogFp() ? Cvt()->LogFp() : stderr, false) &&
                Cvt()->LogFp())
            Cvt()->SetShowLog(true);

        // If the cell is a terminal, make sure that it has a label
        // location property so that the label can be modified to
        // change the terminal name internally.
        //
        CDp_sname *pna = (CDp_sname*)sd->prpty(P_NAME);
        if (pna && pna->name_string() &&
                *Tstring(pna->name_string()) == P_NAME_TERM) {
            CDp_labloc *pl = (CDp_labloc*)sd->prpty(P_LABLOC);
            if (!pl)
                sd->prptyAdd(P_LABLOC, "name 2");
        }
    }

    FILE *fp = Cvt()->LogFp();
    Cvt()->SetLogFp(0);
    if (!fp) {
        Cvt()->SetShowLog(false);
        return;
    }
    fclose(fp);

    if (!Cvt()->ShowLog())
        return;
    Cvt()->SetShowLog(false);
    if (XM()->PanicFp())
        return;
    if (fn && !CDvdb()->getVariable(VA_NoPopUpLog)) {
        char buf[256];
        if (Log()->LogDirectory() && *Log()->LogDirectory()) {
            sprintf(buf, "%s/%s", Log()->LogDirectory(), fn);
            fn = buf;
        }
        DSPmainWbag(PopUpFileBrowser(fn))
    }
}


namespace {
    // Add the property string to lstr.
    //
    bool
    printprop(CDp *pd, sLstr &lstr)
    {
        char *s;
        if (pd->string(&s)) {
            lstr.add(s);
            delete [] s;
        }
        else
            lstr.add("Invalid property");
        lstr.add_c('\n');
        return (true);
    }
}


// Look through all of the devices and check the pointer linkages.
// Any unlinked labels are selected on exit if select_labels is true.
// Messages are printed to fp.  false is returned if something wrong
// was found.
//
bool
cSced::prptyCheck(CDs *sdesc, FILE* fp, bool select_labels)
{
    if (!sdesc)
        return (true);
    CDs *tsd = sdesc->owner();
    if (tsd)
        sdesc = tsd;

    if (sdesc != CurCell())
        select_labels = false;
    const bool nofix = sdesc->isImmutable();
    bool ret = true;

    // Try to minimize lines of text for log file
    bool neednl = false;
    if (select_labels)
        Selections.deselectTypes(sdesc, 0);
    if (!nofix)
        Ulist()->ListCheckPush("check", sdesc, false, true);
    fprintf(fp, "Checking electrical properties in cell %s...",
        Tstring(sdesc->cellname()));
    neednl = true;

    char *str;
    if (!prptyCheckCell(sdesc, &str)) {
        fputc('\n', fp);
        fputs(str, fp);
        delete [] str;
        neednl = false;
        ret = false;
    }
    bool headpr = false;
    CDp *pd, *pnext;
    for (pd = sdesc->prptyList(); pd; pd = pnext) {
        pnext = pd->next_prp();
        if (pd->value() == P_NEWMUT) {
            if (!prptyCheckMutual(sdesc, (CDp_nmut*)pd, &str))
                ret = false;
            if (str) {
                if (!headpr) {
                    if (neednl) {
                        putc('\n', fp);
                        neednl = false;
                    }
                    fprintf(fp, "Mutual Inductors:\n");
                    headpr = true;
                }
                fputs(str, fp);
                delete [] str;
            }
        }
    }

    CDg gdesc;
    CDo *pointer;
    headpr = false;

    CDsLgen lgen(sdesc);
    CDl *ld;
    while ((ld = lgen.next()) != 0) {
        gdesc.init_gen(sdesc, ld);
        while ((pointer = gdesc.next()) != 0) {
            if (pointer->type() != CDLABEL)
                continue;
            if (!pointer->is_normal())
                continue;

            if (!prptyCheckLabel(sdesc, OLABEL(pointer), &str))
                ret = false;
            if (str) {
                if (!headpr) {
                    if (neednl) {
                        putc('\n', fp);
                        neednl = false;
                    }
                    fprintf(fp, "Labels:\n");
                    headpr = true;
                }
                fputs(str, fp);
                delete [] str;
            }
            if (DSP()->CurMode() == Electrical && select_labels) {
                CDp_lref *prf = (CDp_lref*)pointer->prpty(P_LABRF);
                if (!prf || !prf->devref()) {
                    pointer->set_state(CDobjVanilla);
                    Selections.insertObject(sdesc, pointer);
                }
            }
        }
    }

    headpr = false;
    bool cache_state = CD()->EnableNameCache(true);
    CDm_gen mgen(sdesc, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            if (!prptyCheckInst(sdesc, cdesc, &str))
                ret = false;
            if (str) {
                if (!headpr) {
                    if (neednl) {
                        putc('\n', fp);
                        neednl = false;
                    }
                    fprintf(fp, "Devices:\n");
                    headpr = true;
                }
                fputs(str, fp);
                delete [] str;
            }
        }
    }
    CD()->EnableNameCache(cache_state);

    if (!sdesc->isDevice()) {
        CDp_snode *ps = (CDp_snode*)sdesc->prpty(P_NODE);
        if (ps) {
            // make sure subckt has a name
            if (!sdesc->prpty(P_NAME)) {
                if (!nofix) {
                    sdesc->prptyAdd(P_NAME, P_NAME_SUBC_STR);
                }
            }
        }
    }

    if (!nofix)
        Ulist()->ListPop();

    if (select_labels) {
        if (DSP()->CurMode() == Electrical) {
            PL()->ShowPrompt("Done.  Unlinked labels are selected.");
            XM()->ShowParameters();
        }
        else
            PL()->ShowPrompt("Done.");
    }
    fprintf(fp, "Done\n");
    return (ret);
}


namespace {
    void print_err(sLstr &lstr, const char *fmt, ...)
    {
        va_list args;
        char buf[256];

        if (!fmt)
            fmt = "";
        va_start(args, fmt);
        vsnprintf(buf, 256, fmt, args);
        va_end(args);
        lstr.add(buf);
    }

    void af_cb(bool yn, void*)
    {
        if (yn)
            FIO()->SetWriteMacroProps(true);
    }
}


bool
cSced::prptyCheckCell(CDs *sdesc, char **str)
{
    sLstr lstr;
    const char *msg1 = "Cell %s contains inapplicable %s property.\n";
    const char *msg2 = "Cell %s has more than one %s property.\n";
    const char *im_msg = "Cell %s is IMMUTABLE, can't fix.\n";
    if (str)
        *str = 0;
    if (!sdesc)
        return (true);
    const char *cname = Tstring(sdesc->cellname());
    const bool nofix = sdesc->isImmutable();
    CDelecCellType tp = sdesc->elecCellType();

    const int prpmax = P_MAX_PRP_NUM + 1;

    // Remove obsolete P_MACRO properties, set flag in name property.
    // This is redundant, also done in CDs::prptyAdd.
    CDp_sname *pns = (CDp_sname*)sdesc->prpty(P_NAME);
    if (pns) {
        if (sdesc->prpty(P_MACRO)) {
            pns->set_macro(true);
            sdesc->prptyRemove(P_MACRO);
        }
    }

    // Check property counts.
    int pcnts[prpmax];
    for (int i = 0; i < prpmax; i++)
        pcnts[i] = 0;
    for (CDp *pd = sdesc->prptyList(); pd; pd = pd->next_prp()) {
        if (pd->value() >= 0 && pd->value() < prpmax)
            pcnts[pd->value()]++;
    }

    for (int i = 1; i < prpmax; i++) {
        switch (i) {
        case P_MODEL:
        case P_VALUE:
            if (pcnts[i] > 0 && tp != CDelecDev)
                print_err(lstr, msg1, cname, CDp::elec_prp_name(i));
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, CDp::elec_prp_name(i));
            break;
        case P_PARAM:
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, CDp::elec_prp_name(i));
            break;
        case P_OTHER:
            break;
        case P_NOPHYS:
        case P_VIRTUAL:
        case P_FLATTEN:
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, CDp::elec_prp_name(i));
            break;
        case P_RANGE:
            if (pcnts[i] > 0)
                print_err(lstr, msg1, cname, CDp::elec_prp_name(i));
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, CDp::elec_prp_name(i));
            break;
        case P_BNODE:
        case P_NODE:
            break;
        case P_NAME:
        case P_LABLOC:
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, CDp::elec_prp_name(i));
            break;
        case P_MUT:
        case P_NEWMUT:
            break;
        case P_BRANCH:
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, CDp::elec_prp_name(i));
            break;
        case P_LABRF:
        case P_MUTLRF:
            if (pcnts[i] > 0)
                print_err(lstr, msg1, cname, CDp::elec_prp_name(i));
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, CDp::elec_prp_name(i));
            break;
        case P_SYMBLC:
        case P_NODMAP:
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, CDp::elec_prp_name(i));
            break;
        case P_MACRO:
            if (pcnts[i] > 0)
                print_err(lstr, msg1, cname, CDp::elec_prp_name(i));
            break;
        case P_DEVREF:
            if (pcnts[i] > 0)
                print_err(lstr, msg1, cname, CDp::elec_prp_name(i));
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, CDp::elec_prp_name(i));
            break;
        }
    }
    if (lstr.string()) {
        if (nofix)
            print_err(lstr, im_msg, cname);
        else {
            for (int i = 0; i < prpmax; i++)
                pcnts[i] = 0;
            CDp *pp = 0, *pn;
            for (CDp *pd = sdesc->prptyList(); pd; pd = pn) {
                int cnt = 0;
                if (pd->value() >= 0 && pd->value() < prpmax)
                    cnt = ++pcnts[pd->value()];
                pn = pd->next_prp();

                bool ok = true;

                switch (pd->value()) {
                case P_MODEL:
                case P_VALUE:
                    if (cnt > 0 && tp != CDelecDev)
                        ok = false;
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_PARAM:
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_OTHER:
                    break;
                case P_NOPHYS:
                case P_VIRTUAL:
                case P_FLATTEN:
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_RANGE:
                    if (cnt > 0)
                        ok = false;
                    break;
                case P_BNODE:
                case P_NODE:
                    break;
                case P_NAME:
                case P_LABLOC:
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_MUT:
                case P_NEWMUT:
                    break;
                case P_BRANCH:
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_LABRF:
                case P_MUTLRF:
                    if (cnt > 0)
                        ok = false;
                    break;
                case P_SYMBLC:
                case P_NODMAP:
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_MACRO:
                    // Shouldn't get here.
                    if (cnt > 0)
                        ok = false;
                    break;
                case P_DEVREF:
                    if (cnt > 0)
                        ok = false;
                    break;
                }
                if (!ok) {
                    if (pp)
                        pp->set_next_prp(pn);
                    else
                        sdesc->setPrptyList(pn);
                    delete pd;
                    continue;
                }
                pp = pd;
            }
            lstr.add("Fixed.\n");
        }
    }
    if (lstr.string()) {
        if (str)
            *str = lstr.string_trim();
        return (false);
    }
    return (true);
}


bool
cSced::prptyCheckMutual(CDs *sdesc, CDp_nmut *pm, char **str)
{
    bool printed = false;
    bool bogus = false;
    sLstr lstr;

    const bool nofix = sdesc->isImmutable();
    const char *im_msg = "Cell %s is IMMUTABLE, can't fix.\n";

    if (str)
        *str = 0;
    CDla *olabel = pm->bound();
    if (olabel) {
        if (olabel->type() != CDLABEL) {
            printed = printprop(pm, lstr);
            lstr.add("  Label pointer not a label.\n");
            bogus = true;
        }
        else {
            CDp_lref *prf = (CDp_lref*)olabel->prpty(P_LABRF);
            if (prf && (prf->propref() || prf->cellref() != sdesc)) {
                printed = printprop(pm, lstr);
                lstr.add("  Label links bogus.\n");
                bogus = true;
            }
            if (olabel->state() == CDobjDeleted) {
                if (!printed)
                    printed = printprop(pm, lstr);
                lstr.add("  Label in delete list.\n");
                bogus = true;
            }
        }
    }
    if (pm->l1_dev()) {
        CDo *odesc = pm->l1_dev();
        if (odesc->type() != CDINSTANCE) {
            if (!printed)
                printed = printprop(pm, lstr);
            lstr.add("  Object field1 not a cell.\n");
            bogus = true;
        }
        else {
            if (odesc->state() == CDobjDeleted) {
                if (!printed)
                    printed = printprop(pm, lstr);
                lstr.add("  Object field1 in delete list.\n");
                bogus = true;
            }
            bool mutrf = false;
            const char *name = 0;
            for (CDp *pd = odesc->prpty_list(); pd; pd = pd->next_prp()) {
                if (pd->value() == P_MUTLRF)
                    mutrf = true;
                else if (pd->value() == P_NAME)
                    name = Tstring(((CDp_cname*)pd)->name_string());
                if (mutrf && name)
                    break;
            }
            if (!mutrf) {
                if (!printed)
                    printed = printprop(pm, lstr);
                lstr.add("  Object field1 has no mut ref, fixing.\n");
                if (nofix)
                    print_err(lstr, im_msg, Tstring(sdesc->cellname()));
                else
                    odesc->prptyAdd(P_MUTLRF, "mutual", Electrical);
            }
            if (!name || (*name != 'L' && *name != 'l')) {
                if (!printed)
                    printed = printprop(pm, lstr);
                lstr.add("  Object field1 not inductor.\n");
                bogus = true;
            }
        }
    }
    else {
        if (!printed)
            printed = printprop(pm, lstr);
        lstr.add("  Object field1 empty.\n");
        bogus = true;
    }
    if (pm->l2_dev()) {
        CDo *odesc = pm->l2_dev();
        if (odesc->type() != CDINSTANCE) {
            if (!printed)
                printed = printprop(pm, lstr);
            lstr.add("  Object field2 not a cell.\n");
            bogus = true;
        }
        else {
            if (odesc->state() == CDobjDeleted) {
                if (!printed)
                    printprop(pm, lstr);
                lstr.add("  Object field1 in delete list.\n");
                bogus = true;
            }
            bool mutrf = false;
            const char *name = 0;
            for (CDp *pd = odesc->prpty_list(); pd; pd = pd->next_prp()) {
                if (pd->value() == P_MUTLRF)
                    mutrf = true;
                else if (pd->value() == P_NAME)
                    name = Tstring(((CDp_cname*)pd)->name_string());
                if (mutrf && name)
                    break;
            }
            if (!mutrf) {
                if (!printed)
                    printed = printprop(pm, lstr);
                lstr.add("  Object field2 has no mut ref, fixing.\n");
                if (nofix)
                    print_err(lstr, im_msg, Tstring(sdesc->cellname()));
                else
                    odesc->prptyAdd(P_MUTLRF, "mutual", Electrical);
            }
            if (!name || (*name != 'L' && *name != 'l')) {
                if (!printed)
                    printed = printprop(pm, lstr);
                lstr.add("  Object field2 not inductor.\n");
                bogus = true;
            }
        }
    }
    else {
        if (!printed)
            printed = printprop(pm, lstr);
        lstr.add("  Object field2 empty.\n");
        bogus = true;
    }
    if (bogus) {
        const char *mname;
        char tbuf[64];
        if (pm->assigned_name())
            mname = pm->assigned_name();
        else {
            sprintf(tbuf, "%s%d", MUT_CODE, pm->index());
            mname = tbuf;
        }
        lstr.add("Deleting mutual inductor ");
        lstr.add(mname);
        lstr.add(" property due to errors.\n");
        if (nofix)
            print_err(lstr, im_msg, Tstring(sdesc->cellname()));
        else {
            if (pm->bound())
                // get rid of label, too
                Ulist()->RecordObjectChange(sdesc, pm->bound(), 0);
            sdesc->prptyUnlink(pm);
            delete(pm);
            sdesc->incModified();
        }
    }
    if (lstr.string()) {
        if (str)
            *str = lstr.string_trim();
        return (false);
    }
    return (true);
}


bool
cSced::prptyCheckLabel(CDs *sdesc, CDla *olabel, char **str)
{
    if (str)
        *str = 0;
    const char *err = 0;
    CDp_lref *prf = (CDp_lref*)olabel->prpty(P_LABRF);
    if (prf) {
        if (!prf->devref())
            err = "Has LABRF property, no link";
        else if (prf->devref()->type() == CDWIRE) {
            if (prf->name())
                err = "Wire label reference property name not null";
            if (prf->propref())
                err = "Wire label reference property pointer not null";
        }
        else {
            if (!prf->propref()) {
                if (prf->cellref() != sdesc)
                    err = "Bad cell pointer";
            }
            else {
                CDc *cdesc = prf->devref();
                if (!cdesc->in_db())
                    err = "Linked device or subckt not in database";
                else if (cdesc->parent() == 0)
                    err = "Linked device or subckt has null parent";
                else if (cdesc->parent() != sdesc)
                    err = "Linked device or subckt has wrong parent";
                else if (((CDo*)prf->devref())->state() == CDobjDeleted)
                    err = "Linked device or subckt in delete list";
            }
        }
    }
    if (!err)
        return (true);

    if (str) {
        char *s = hyList::string(olabel->label(), HYcvPlain, false);
        sLstr lstr;
        lstr.add("Label: ");
        lstr.add(s);
        lstr.add("  ");
        lstr.add(err);
        lstr.add(".\n");
        delete [] s;
        *str = lstr.string_trim();
    }
    return (false);
}


bool
cSced::prptyCheckInst(CDs *sdesc, CDc *cdesc, char **str)
{
    sLstr lstr;
    const char *msg1 =
        "Instance of %s at LL=%.3f,%.3f contains inapplicable %s property.\n";
    const char *msg2 =
        "Instance of %s at LL=%.3f,%.3f has more than one %s property.\n";

    const bool nofix = sdesc->isImmutable();
    const char *im_msg = "Cell %s is IMMUTABLE, can't fix.\n";

    CDs *msdesc = cdesc->masterCell();
    bool isdev = (msdesc && msdesc->elecCellType() == CDelecDev);
    bool ismac = (msdesc && msdesc->elecCellType() == CDelecMacro);
    const char *cname = msdesc ? Tstring(msdesc->cellname()) : "<unknown>";
    double xll = ELEC_MICRONS(cdesc->oBB().left);
    double yll = ELEC_MICRONS(cdesc->oBB().bottom);

    if (str)
        *str = 0;
    // First check property counts
    const int prpmax = P_MAX_PRP_NUM + 1;
    int pcnts[prpmax];
    for (int i = 0; i < prpmax; i++)
        pcnts[i] = 0;
    for (CDp *pd = cdesc->prpty_list(); pd; pd = pd->next_prp()) {
        if (pd->value() >= 0 && pd->value() < prpmax)
            pcnts[pd->value()]++;
    }

    for (int i = 1; i < prpmax; i++) {
        switch (i) {
        case P_MODEL:
            if (pcnts[i] > 0 && !ismac && !isdev)
                print_err(lstr, msg1, cname, xll, yll, CDp::elec_prp_name(i));
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        case P_VALUE:
            if (pcnts[i] > 0 && !isdev)
                print_err(lstr, msg1, cname, xll, yll, CDp::elec_prp_name(i));
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        case P_PARAM:
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        case P_OTHER:
            break;
        case P_NOPHYS:
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        case P_VIRTUAL:
            if (pcnts[i] > 0)
                print_err(lstr, msg1, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        case P_FLATTEN:
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        case P_RANGE:
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        case P_BNODE:
        case P_NODE:
            break;
        case P_NAME:
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        case P_LABLOC:
            if (pcnts[i] > 0)
                print_err(lstr, msg1, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        case P_MUT:
        case P_NEWMUT:
            break;
        case P_BRANCH:
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        case P_LABRF:
            if (pcnts[i] > 0)
                print_err(lstr, msg1, cname, xll, yll, CDp::elec_prp_name(i));
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        case P_MUTLRF:
            break;
        case P_SYMBLC:
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        case P_NODMAP:
            if (pcnts[i] > 0)
                print_err(lstr, msg1, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        case P_MACRO:
            if (pcnts[i] > 0)
                print_err(lstr, msg1, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        case P_DEVREF:
            if (pcnts[i] > 1)
                print_err(lstr, msg2, cname, xll, yll, CDp::elec_prp_name(i));
            break;
        }
    }
    if (lstr.string()) {
        if (nofix)
            print_err(lstr, im_msg, Tstring(sdesc->cellname()));
        else {
            for (int i = 0; i < prpmax; i++)
                pcnts[i] = 0;
            for (CDp *pd = cdesc->prpty_list(); pd; pd = pd->next_prp()) {
                int cnt = 0;
                if (pd->value() >= 0 && pd->value() < prpmax)
                    cnt = ++pcnts[pd->value()];

                bool ok = true;
                switch (pd->value()) {
                case P_MODEL:
                    if (cnt > 0 && !ismac && !isdev)
                        ok = false;
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_VALUE:
                    if (cnt > 0 && !isdev)
                        ok = false;
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_PARAM:
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_OTHER:
                    break;
                case P_NOPHYS:
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_VIRTUAL:
                    if (cnt > 0)
                        ok = false;
                    break;
                case P_FLATTEN:
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_RANGE:
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_BNODE:
                case P_NODE:
                    break;
                case P_NAME:
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_LABLOC:
                    if (cnt > 0)
                        ok = false;
                    break;
                case P_MUT:
                case P_NEWMUT:
                    if (cnt > 0)
                        ok = false;
                    break;
                case P_BRANCH:
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_LABRF:
                    if (cnt > 0)
                        ok = false;
                    break;
                case P_MUTLRF:
                    break;
                case P_SYMBLC:
                    if (cnt > 1)
                        ok = false;
                    break;
                case P_NODMAP:
                    if (cnt > 0)
                        ok = false;
                    break;
                case P_MACRO:
                    if (cnt > 0)
                        ok = false;
                    break;
                case P_DEVREF:
                    if (cnt > 1)
                        ok = false;
                    break;
                }
                if (!ok) {
                    Ulist()->RecordPrptyChange(sdesc, cdesc, pd, 0);
                    lstr.add("Fixed.\n");
                }
            }
        }
    }

    // Next check name consistency
    CDp_cname *pn = (CDp_cname*)cdesc->prpty(P_NAME);
    const char *name = 0;
    CDpfxName realname = 0;
    if (!pn) {
        // A cell without a name property and with exactly one node
        // property is a "gnd" device.

        CDp_node *pnd = (CDp_node*)cdesc->prpty(P_NODE);
        if (!pnd || pnd->next()) {
            // not a gnd device
            print_err(lstr,
                "Instance of %s at LL=%.3f,%.3f with no name property.\n",
                Tstring(cdesc->cellname()),
                ELEC_MICRONS(cdesc->oBB().left),
                ELEC_MICRONS(cdesc->oBB().bottom));
        }
    }
    else {
        name = cdesc->getElecInstBaseName(pn);

        // Check that the instance default name is that of the
        // corresponding device.  If not, make it so and report.  Skip
        // this for terminals, which can change type.

        if (msdesc) {

            CDp_sname *pn1 = (CDp_sname*)msdesc->prpty(P_NAME);
            if (!pn1) {
                print_err(lstr, "%s %s has no name property!\n",
                    msdesc->isDevice() ? "Device" : "Cell",
                    Tstring(msdesc->cellname()));
            }
            else if (msdesc->elecCellType() != CDelecTerm) {

                realname = pn1->name_string();
                if (pn1->name_string() != pn->name_string()) {
                    print_err(lstr,
                        "Name prefix for %s (%s) doesn't match "
                        "cell entry (%s), updating.\n",
                        name, TstringNN(pn->name_string()),
                        TstringNN(pn1->name_string()));
                    if (nofix)
                        print_err(lstr, im_msg, Tstring(sdesc->cellname()));
                    else {
                        CDp *op = pn;
                        pn = (CDp_cname*)pn->dup();
                        pn->set_name_string(pn1->name_string());
                        Ulist()->RecordPrptyChange(sdesc, cdesc, op, pn);
                        CDla *olabel = pn->bound();
                        if (olabel) {
                            Label ltmp(olabel->la_label());
                            CDla *nlabel = sdesc->newLabel(olabel, &ltmp,
                                olabel->ldesc(), olabel->prpty_list(),
                                true);
                            pn->bind(nlabel);
                            nlabel->link(sdesc, cdesc, pn);
                            CDp_lref *pl =
                                (CDp_lref*)nlabel->prpty(P_LABRF);
                            if (pl)
                                pl->set_name(pn1->name_string());
                        }
                    }
                }
                else
                    realname = 0;
            }
        }
    }
    if (msdesc) {
        CDelecCellType mtp = msdesc->elecCellType();
        CDelecCellType itp = cdesc->elecCellType();
        if (mtp != itp) {
            print_err(lstr,
        "Instance of %s at LL=%.3f,%.3f, master/instance types differ %d/%d.\n",
                Tstring(cdesc->cellname()),
                ELEC_MICRONS(cdesc->oBB().left),
                ELEC_MICRONS(cdesc->oBB().bottom), mtp, itp);
        }
    }

    if (!name)
        name = "unnamed";
    for (CDp *pd = cdesc->prpty_list(); pd; pd = pd->next_prp()) {
        CDla *olabel = pd->bound();
        if (olabel) {
            if (olabel->type() != CDLABEL) {
                printprop(pd, lstr);
                print_err(lstr, "  %s: Label pointer not a label.\n", name);
                continue;
            }
            CDp_lref *prf = (CDp_lref*)olabel->prpty(P_LABRF);
            if (!prf || prf->propref() != pd || prf->devref() != cdesc) {
                printprop(pd, lstr);
                print_err(lstr, "  %s: Label links bogus.\n", name);
            }
            if (olabel->state() == CDobjDeleted) {
                printprop(pd, lstr);
                print_err(lstr, "  %s: Label in delete list.\n", name);
            }
            if (realname && pd->value() != P_NAME) {
                // update the link name
                if (prf && (prf->name() != realname)) {
                    // An im_msg here would be redundant.
                    if (!nofix) {
                        CDp *op = prf;
                        prf = (CDp_lref*)prf->dup();
                        prf->set_name(realname);
                        Ulist()->RecordPrptyChange(sdesc, olabel, op, prf);
                    }
                }
            }
        }
    }
    // Last, if a mutual inductor, check if OK
    if (pcnts[P_MUTLRF]) {
        int count;
        CDp_nmut *pm = (CDp_nmut*)sdesc->prpty(P_NEWMUT);
        for (count = 0; pm; pm = pm->next()) {
            if (pm->l1_dev() == cdesc || pm->l2_dev() == cdesc)
                count++;
        }
        if (count > pcnts[P_MUTLRF]) {
            // Missing some mutlrf properties in device, add them.
            count -= pcnts[P_MUTLRF];
            print_err(lstr,
                "Adding %d mutref propert%s to inductor at %.3f,%.3f.\n",
                count, count == 1 ? "y" : "ies",
                ELEC_MICRONS(cdesc->oBB().left),
                ELEC_MICRONS(cdesc->oBB().bottom));
            if (nofix)
                print_err(lstr, im_msg, Tstring(sdesc->cellname()));
            else {
                while (count--)
                    cdesc->prptyAdd(P_MUTLRF, "mutual", Electrical);
            }
        }
        else if (count < pcnts[P_MUTLRF]) {
            // Too many mutlrf properties in device, delete extras.
            count = pcnts[P_MUTLRF] - count;
            print_err(lstr,
                "Deleting %d mutref propert%s from inductor at %.3f,%.3f.\n",
                count, count == 1 ? "y" : "ies",
                ELEC_MICRONS(cdesc->oBB().left),
                ELEC_MICRONS(cdesc->oBB().bottom));
            if (nofix)
                print_err(lstr, im_msg, Tstring(sdesc->cellname()));
            else {
                while (count--) {
                    CDp_mutlrf *px = (CDp_mutlrf*)cdesc->prpty(P_MUTLRF);
                    Ulist()->RecordPrptyChange(sdesc, cdesc, px, 0);
                }
            }
        }
    }

    // Make sure all instance terminals have a vertex in crossing
    // wires.
    //
    CDp_cnode *pcn = (CDp_cnode*)cdesc->prpty(P_NODE);
    for ( ; pcn; pcn = pcn->next()) {
        for (unsigned int ix = 0; ; ix++) {
            int x, y;
            if (!pcn->get_pos(ix, &x, &y))
                break;
            if (SCD()->checkAddConnection(sdesc, x, y, !nofix)) {
                print_err(lstr,
                    "Cell %s, added missing connection at %.3f,%.3f.\n",
                    Tstring(sdesc->cellname()),
                    ELEC_MICRONS(x), ELEC_MICRONS(y));
                if (nofix)
                    print_err(lstr, im_msg, Tstring(sdesc->cellname()));
                else
                    sdesc->incModified();
            }
        }
    }
    CDp_bcnode *pb = (CDp_bcnode*)cdesc->prpty(P_BNODE);
    for ( ; pb; pb = pb->next()) {
        for (unsigned int ix = 0; ; ix++) {
            int x, y;
            if (!pb->get_pos(ix, &x, &y))
                break;
            if (SCD()->checkAddConnection(sdesc, x, y, !nofix)) {
                print_err(lstr,
                    "Cell %s, added missing connection at %.3f,%.3f.\n",
                    Tstring(sdesc->cellname()),
                    ELEC_MICRONS(x), ELEC_MICRONS(y));
                if (nofix)
                    print_err(lstr, im_msg, Tstring(sdesc->cellname()));
                else
                    sdesc->incModified();
            }
        }
    }

    if (lstr.string()) {
        if (str)
            *str = lstr.string_trim();
        return (false);
    }
    return (true);
}


// For each device in the database of XM()->curop.cell_desc, regenerate
// the labels and link pointers for properties without labels.  Return
// true if something was changed.
//
bool
cSced::prptyRegenCell()
{
    CDs *sdesc = Ulist()->CurOp().celldesc();
    Ochg *al = Ulist()->CurOp().obj_list();

    // first the device labels
    CDg gdesc;
    gdesc.init_gen(sdesc, CellLayer());
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0)
        if (cdesc->is_normal())
            genDeviceLabels(cdesc, 0, true);

    // now mutual inductor labels
    CDp_nmut *pm = (CDp_nmut*)sdesc->prpty(P_NEWMUT);
    for ( ; pm; pm = pm->next())
        if (!pm->bound())
            setMutLabel(sdesc, pm, 0);

    if (al != Ulist()->CurOp().obj_list())
        return (true);
    return (false);
}

