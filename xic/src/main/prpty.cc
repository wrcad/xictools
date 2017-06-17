
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
 $Id: prpty.cc,v 5.160 2016/02/21 18:49:43 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "cd_hypertext.h"
#include "cd_netname.h"


// Return a sorted list of the properties.
//
Ptxt *
cMain::PrptyStrings(CDs *sdesc)
{
    if (!sdesc)
        return (0);
    Ptxt *list = 0, *lend = 0;
    const char *lttok = HYtokPre HYtokLT HYtokSuf;
    char tbuf[256];

    if (sdesc->isElectrical()) {

        CDpl *pl0 = 0;
        for (CDp *pdesc = sdesc->prptyList(); pdesc;
                pdesc = pdesc->next_prp()) {
            switch (pdesc->value()) {
            case P_MODEL:
            case P_VALUE:
            case P_PARAM:
            case P_OTHER:
            case P_NOPHYS:
            case P_VIRTUAL:
            case P_FLATTEN:
            case P_BNODE:
            case P_NODE:
            case P_NAME:
            case P_LABLOC:
            case P_MUT:
            case P_NEWMUT:
            case P_BRANCH:
            case P_SYMBLC:
            case P_NODMAP:
                continue;
            default:
                pl0 = new CDpl(pdesc, pl0);
                break;
            }
        }
        pl0->sort();
        for (CDpl *pl = pl0; pl; pl = pl->next) {
            CDp *pdesc = pl->pdesc;
            const char *string = pdesc->string() ? pdesc->string() : "";
            if (lstring::prefix(lttok, string))
                // use symbolic form for long text
                string = "[text]";
            switch (pdesc->value()) {
            case XICP_PC:
                // This can appear in electrical part of PCell from
                // OpenAccess.
                sprintf(tbuf, "(%d) pc_name: ", pdesc->value());
                break;
            default:
                if (prpty_internal(pdesc->value()))
                    sprintf(tbuf, "(%d): ", pdesc->value());
                else
                    sprintf(tbuf, "%d: ", pdesc->value());
                break;
            }
            char *hd = lstring::copy(tbuf);
            char *str = lstring::copy(string);
            if (!list)
                list = lend = new Ptxt(hd, str, pdesc);
            else {
                lend->set_next(new Ptxt(hd, str, pdesc));
                lend = lend->next();
            }
        }
        pl0->free();

        CDp_name *pna = (CDp_name*)sdesc->prpty(P_NAME);
        if (pna) {
            sprintf(tbuf, "(%d) name: ", P_NAME);
            char *hd = lstring::copy(tbuf);
            if (pna->name_string())
                strcpy(tbuf, pna->name_string()->string());
            else
                tbuf[0] = 0;
            char *str = lstring::copy(tbuf);
            if (!list)
                list = lend = new Ptxt(hd, str, pna);
            else {
                lend->set_next(new Ptxt(hd, str, pna));
                lend = lend->next();
            }
        }

        CDp_bsnode *pbn = (CDp_bsnode*)sdesc->prpty(P_BNODE);
        for ( ; pbn; pbn = pbn->next()) {
            sprintf(tbuf, "(%d) bnode: ", P_BNODE);
            char *hd = lstring::copy(tbuf);
            sLstr lstr;
            lstr.add_i(pbn->index());
            lstr.add_c(' ');
            lstr.add_i(pbn->beg_range());
            lstr.add_c(' ');
            lstr.add_i(pbn->end_range());
            if (pbn->has_name()) {
                lstr.add_c(' ');
                char *s = pbn->full_name();
                lstr.add(s);
                delete [] s;
            }
            else
                lstr.add("<unnamed>");
            if (pbn->has_flag(TE_SCINVIS)) {
                lstr.add_c(' ');
                lstr.add("SCINVIS");
            }
            if (pbn->has_flag(TE_SYINVIS)) {
                lstr.add_c(' ');
                lstr.add("SYINVIS");
            }
            char *str = lstr.string_trim();
            if (!list)
                list = lend = new Ptxt(hd, str, pbn);
            else {
                lend->set_next(new Ptxt(hd, str, pbn));
                lend = lend->next();
            }
        }

        CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            sprintf(tbuf, "(%d) node: ", P_NODE);
            char *hd = lstring::copy(tbuf);
            sLstr lstr;
            lstr.add_i(pn->index());
            lstr.add_c(' ');
            lstr.add_i(pn->enode());
            lstr.add_c(' ');
            if (pn->get_term_name())
                lstr.add(pn->term_name()->string());
            else
                lstr.add("<unnamed>");
            if (pn->has_flag(TE_BYNAME)) {
                lstr.add_c(' ');
                lstr.add("BYNAME");
            }
            if (pn->has_flag(TE_SCINVIS)) {
                lstr.add_c(' ');
                lstr.add("SCINVIS");
            }
            if (pn->has_flag(TE_SYINVIS)) {
                lstr.add_c(' ');
                lstr.add("SYINVIS");
            }

            char *str = lstr.string_trim();
            if (!list)
                list = lend = new Ptxt(hd, str, pn);
            else {
                lend->set_next(new Ptxt(hd, str, pn));
                lend = lend->next();
            }
        }

        CDp_user *pu = (CDp_user*)sdesc->prpty(P_MODEL);
        if (pu) {
            sprintf(tbuf, "(%d) model: ", P_MODEL);
            char *hd = lstring::copy(tbuf);
            char *str = pu->data()->string(HYcvPlain, false);
            if (!list)
                list = lend = new Ptxt(hd, str, pu);
            else {
                lend->set_next(new Ptxt(hd, str, pu));
                lend = lend->next();
            }
        }

        pu = (CDp_user*)sdesc->prpty(P_VALUE);
        if (pu) {
            sprintf(tbuf, "(%d) value: ", P_VALUE);
            char *hd = lstring::copy(tbuf);
            char *str = pu->data()->string(HYcvPlain, false);
            if (!list)
                list = lend = new Ptxt(hd, str, pu);
            else {
                lend->set_next(new Ptxt(hd, str, pu));
                lend = lend->next();
            }
        }

        pu = (CDp_user*)sdesc->prpty(P_PARAM);
        if (pu) {
            sprintf(tbuf, "(%d) param: ", P_PARAM);
            char *hd = lstring::copy(tbuf);
            char *str = pu->data()->string(HYcvPlain, false);
            if (!list)
                list = lend = new Ptxt(hd, str, pu);
            else {
                lend->set_next(new Ptxt(hd, str, pu));
                lend = lend->next();
            }
        }

        pu = (CDp_user*)sdesc->prpty(P_OTHER);
        for ( ; pu; pu = pu->next()) {
            sprintf(tbuf, "(%d) other: ", P_OTHER);;
            char *hd = lstring::copy(tbuf);
            char *str = pu->data()->string(HYcvPlain, false);
            if (!list)
                list = lend = new Ptxt(hd, str, pu);
            else {
                lend->set_next(new Ptxt(hd, str, pu));
                lend = lend->next();
            }
        }

        CDp *px = sdesc->prpty(P_NOPHYS);
        for ( ; px && px->value() == P_NOPHYS; px = px->next_prp()) {
            sprintf(tbuf, "(%d) nophys: ", P_NOPHYS);
            char *hd = lstring::copy(tbuf);
            const char *s = px->string();
            const char *pstr = "nophys";
            if (s && (*s == 's' || *s == 'S'))
                pstr = "shorted";
            char *str = lstring::copy(pstr);
            if (!list)
                list = lend = new Ptxt(hd, str, px);
            else {
                lend->set_next(new Ptxt(hd, str, px));
                lend = lend->next();
            }
        }

        px = sdesc->prpty(P_VIRTUAL);
        for ( ; px && px->value() == P_VIRTUAL; px = px->next_prp()) {
            sprintf(tbuf, "(%d) virtual: ", P_VIRTUAL);
            char *hd = lstring::copy(tbuf);
            char *str = lstring::copy("virtual");
            if (!list)
                list = lend = new Ptxt(hd, str, px);
            else {
                lend->set_next(new Ptxt(hd, str, px));
                lend = lend->next();
            }
        }

        px = sdesc->prpty(P_FLATTEN);
        for ( ; px && px->value() == P_FLATTEN; px = px->next_prp()) {
            sprintf(tbuf, "(%d) flatten: ", P_FLATTEN);
            char *hd = lstring::copy(tbuf);
            char *str = lstring::copy("flatten");
            if (!list)
                list = lend = new Ptxt(hd, str, px);
            else {
                lend->set_next(new Ptxt(hd, str, px));
                lend = lend->next();
            }
        }

        CDp_branch *pb = (CDp_branch*)sdesc->prpty(P_BRANCH);
        for ( ; pb; pb = pb->next()) {
            sprintf(tbuf, "(%d) branch: ", P_BRANCH);
            char *hd = lstring::copy(tbuf);
            if (pb->br_string())
                strcpy(tbuf, pb->br_string());
            else
                tbuf[0] = 0;
            char *str = lstring::copy(tbuf);
            if (!list)
                list = lend = new Ptxt(hd, str, pb);
            else {
                lend->set_next(new Ptxt(hd, str, pb));
                lend = lend->next();
            }
        }

        CDp_labloc *pl = (CDp_labloc*)sdesc->prpty(P_LABLOC);
        if (pl) {
            sprintf(tbuf, "(%d) labloc: ", P_LABLOC);
            char *hd = lstring::copy(tbuf);
            char *str;
            pl->string(&str);
            if (!list)
                list = lend = new Ptxt(hd, str, pl);
            else {
                lend->set_next(new Ptxt(hd, str, pl));
                lend = lend->next();
            }
        }

        CDp_mut *pm = (CDp_mut*)sdesc->prpty(P_MUT);
        for ( ; pm; pm = (CDp_mut*)pm->next()) {
            sprintf(tbuf, "(%d) oldmut: ", P_MUT);
            char *hd = lstring::copy(tbuf);
            char *str;
            pm->string(&str);
            if (!list)
                list = lend = new Ptxt(hd, str, pm);
            else {
                lend->set_next(new Ptxt(hd, str, pm));
                lend = lend->next();
            }
        }

        CDp_nmut *pnm = (CDp_nmut*)sdesc->prpty(P_NEWMUT);
        for ( ; pnm; pnm = (CDp_nmut*)pnm->next()) {
            sprintf(tbuf, "(%d) mut: ", P_NEWMUT);
            char *hd = lstring::copy(tbuf);
            tbuf[0] = 0;
            if (pnm->l1_dev() && pnm->l2_dev()) {
                CDp_name *p1 = (CDp_name*)pnm->l1_dev()->prpty(P_NAME);
                CDp_name *p2 = (CDp_name*)pnm->l2_dev()->prpty(P_NAME);
                if (p1 && p1->name_string() && p2 && p2->name_string())
                    sprintf(tbuf, "%d %s%d %s%d %s", pnm->index(),
                        p1->name_string()->string(), pnm->l1_index(),
                        p2->name_string()->string(), pnm->l2_index(),
                        pnm->coeff_str());
            }
            char *str = lstring::copy(tbuf);
            if (!list)
                list = lend = new Ptxt(hd, str, pnm);
            else {
                lend->set_next(new Ptxt(hd, str, pnm));
                lend = lend->next();
            }
        }

        CDp_sym *ps = (CDp_sym*)sdesc->prpty(P_SYMBLC);
        for ( ; ps; ps = (CDp_sym*)ps->next()) {
            sprintf(tbuf, "(%d) symbolic: ", P_SYMBLC);
            char *hd = lstring::copy(tbuf);
            char *str;
            ps->string(&str);
            if (!list)
                list = lend = new Ptxt(hd, str, ps);
            else {
                lend->set_next(new Ptxt(hd, str, ps));
                lend = lend->next();
            }
        }

        CDp_nodmp *pno = (CDp_nodmp*)sdesc->prpty(P_NODMAP);
        for ( ; pno; pno = (CDp_nodmp*)pno->next()) {
            sprintf(tbuf, "(%d) nodemap: ", P_NODMAP);
            char *hd = lstring::copy(tbuf);
            char *str;
            pno->string(&str);
            if (!list)
                list = lend = new Ptxt(hd, str, pno);
            else {
                lend->set_next(new Ptxt(hd, str, pno));
                lend = lend->next();
            }
        }
    }
    else {
        CDpl *pl0 = 0;
        for (CDp *pdesc = sdesc->prptyList(); pdesc; pdesc = pdesc->next_prp())
            pl0 = new CDpl(pdesc, pl0);
        pl0->sort();
        for (CDpl *pl = pl0; pl; pl = pl->next) {
            CDp *pdesc = pl->pdesc;
            const char *string = pdesc->string() ? pdesc->string() : "";
            if (lstring::prefix(lttok, string))
                // use symbolic form for long text
                string = "[text]";
            switch (pdesc->value()) {
            case XICP_FLAGS:
                sprintf(tbuf, "(%d) flags: ", pdesc->value());
                break;
            case XICP_PC:
                sprintf(tbuf, "(%d) pc_name: ", pdesc->value());
                break;
            case XICP_PC_PARAMS:
                sprintf(tbuf, "(%d) pc_params: ", pdesc->value());
                break;
            case XICP_PC_SCRIPT:
                sprintf(tbuf, "(%d) pc_script: ", pdesc->value());
                break;
            default:
                if (prpty_internal(pdesc->value()))
                    sprintf(tbuf, "(%d): ", pdesc->value());
                else
                    sprintf(tbuf, "%d: ", pdesc->value());
                break;
            }
            char *hd = lstring::copy(tbuf);
            char *str = lstring::copy(string);
            if (!list)
                list = lend = new Ptxt(hd, str, pdesc);
            else {
                lend->set_next(new Ptxt(hd, str, pdesc));
                lend = lend->next();
            }
        }
        pl0->free();
    }
    return (list);
}


Ptxt *
cMain::PrptyStrings(CDo *odesc, CDs *sdesc)
{
    if (!odesc || !sdesc)
        return (0);
    Ptxt *list = 0;
    char tbuf[256];
    if (sdesc->isElectrical()) {
        if (odesc->type() == CDWIRE) {
            CDp_bnode *pb = (CDp_bnode*)odesc->prpty(P_BNODE);
            if (pb) {
                sprintf(tbuf, "(%d) bnode (internal): ", P_BNODE);
                char *hd = lstring::copy(tbuf);
                sLstr lstr;
                lstr.add_i(pb->beg_range());
                lstr.add_c(' ');
                lstr.add_i(pb->end_range());
                if (pb->bound()) {
                    lstr.add_c(' ');
                    pb->add_label_text(&lstr);
                }
                char *str = lstr.string_trim();
                list = new Ptxt(hd, str, pb, list);
            }
            CDp_node *pn = (CDp_node*)odesc->prpty(P_NODE);
            if (pn) {
                sprintf(tbuf, "(%d) node (internal): ", P_NODE);
                char *hd = lstring::copy(tbuf);
                sLstr lstr;
                if (pn->bound() && pn->get_term_name()) {
                    lstr.add(pn->get_term_name()->string());
                    lstr.add_c(' ');
                }
                CDw *cdw = OWIRE(odesc);
                hyList *hp = new hyList(HLrefNode);
                hyEnt *ent = new hyEnt(sdesc, cdw->points()[0].x,
                    cdw->points()[0].y, odesc, HYrefNode, HYorNone);
                ent->add();
                hp->set_hent(ent);
                char *str = hp->string(HYcvPlain, false);
                hp->free();
                list = new Ptxt(hd, str, pn, list);
            }
        }
        else if (odesc->type() == CDLABEL) {
            CDp_lref *pl = (CDp_lref*)odesc->prpty(P_LABRF);
            if (pl) {
                sprintf(tbuf, "(%d) labref (internal): ", P_LABRF);
                char *hd = lstring::copy(tbuf);
                if (pl->name())
                    sprintf(tbuf, "%s %d %d", pl->name()->string(),
                        pl->number(), pl->propnum());
                else
                    sprintf(tbuf, "%d %d %d", pl->pos_x(), pl->pos_y(),
                        pl->propnum());
                char *str = lstring::copy(tbuf);
                list = new Ptxt(hd, str, pl, list);
            }
        }
        else if (odesc->type() == CDINSTANCE) {
            CDp_name *pna = (CDp_name*)odesc->prpty(P_NAME);
            if (pna) {
                sprintf(tbuf, "(%d) name (user): ", P_NAME);
                char *hd = lstring::copy(tbuf);
                bool copied;
                hyList *hp = pna->label_text(&copied, OCALL(odesc));
                char *str = hp->string(HYcvPlain, false);
                if (copied)
                    hp->free();
                list = new Ptxt(hd, str, pna, list);
            }
            CDp_range *pr = (CDp_range*)odesc->prpty(P_RANGE);
            for ( ; pr; pr = pr->next()) {
                sprintf(tbuf, "(%d) range (user): ", P_RANGE);
                char *hd = lstring::copy(tbuf);
                sprintf(tbuf, "%d %d", pr->beg_range(), pr->end_range());
                char *str = lstring::copy(tbuf);
                list = new Ptxt(hd, str, pr, list);
            }
            CDp_user *pu = (CDp_user*)odesc->prpty(P_MODEL);
            if (pu) {
                sprintf(tbuf, "(%d) model (user): ", P_MODEL);
                char *hd = lstring::copy(tbuf);
                char *str = pu->data()->string(HYcvPlain, false);
                list = new Ptxt(hd, str, pu, list);
            }
            pu = (CDp_user*)odesc->prpty(P_VALUE);
            if (pu) {
                sprintf(tbuf, "(%d) value (user): ", P_VALUE);
                char *hd = lstring::copy(tbuf);
                char *str = pu->data()->string(HYcvPlain, false);
                list = new Ptxt(hd, str, pu, list);
            }
            pu = (CDp_user*)odesc->prpty(P_PARAM);
            if (pu) {
                sprintf(tbuf, "(%d) param (user): ", P_PARAM);
                char *hd = lstring::copy(tbuf);
                char *str = pu->data()->string(HYcvPlain, false);
                list = new Ptxt(hd, str, pu, list);
            }
            pu = (CDp_user*)odesc->prpty(P_OTHER);
            for ( ; pu; pu = pu->next()) {
                sprintf(tbuf, "(%d) other (user): ", P_OTHER);
                char *hd = lstring::copy(tbuf);
                char *str = pu->data()->string(HYcvPlain, false);
                list = new Ptxt(hd, str, pu, list);
            }
            pu = (CDp_user*)odesc->prpty(P_DEVREF);
            for ( ; pu; pu = pu->next()) {
                sprintf(tbuf, "(%d) other (user): ", P_DEVREF);
                char *hd = lstring::copy(tbuf);
                char *str = pu->data()->string(HYcvPlain, false);
                list = new Ptxt(hd, str, pu, list);
            }
            CDp *px = odesc->prpty(P_NOPHYS);
            for ( ; px && px->value() == P_NOPHYS; px = px->next_prp()) {
                sprintf(tbuf, "(%d) nophys (user): ", P_NOPHYS);
                char *hd = lstring::copy(tbuf);
                const char *s = px->string();
                const char *pstr = "nophys";
                if (s && (*s == 's' || *s == 'S'))
                    pstr = "shorted";
                char *str = lstring::copy(pstr);
                list = new Ptxt(hd, str, px, list);
            }
            px = odesc->prpty(P_FLATTEN);
            for ( ; px && px->value() == P_FLATTEN; px = px->next_prp()) {
                sprintf(tbuf, "(%d) flatten (user): ", P_FLATTEN);
                char *hd = lstring::copy(tbuf);
                char *str = lstring::copy("flatten");
                list = new Ptxt(hd, str, px, list);
            }
            CDp_sym *psm = (CDp_sym*)odesc->prpty(P_SYMBLC);
            for ( ; psm && psm->value() == P_SYMBLC; psm = psm->next()) {
                sprintf(tbuf, "(%d) nosymb (user): ", P_SYMBLC);
                char *hd = lstring::copy(tbuf);
                bool active = ((CDp_sym*)psm)->active();
                char *str = lstring::copy(active ? "1" : "0");
                list = new Ptxt(hd, str, psm, list);
            }
            CDp_bcnode *pcn = (CDp_bcnode*)odesc->prpty(P_BNODE);
            for ( ; pcn; pcn = pcn->next()) {
                sprintf(tbuf, "(%d) bnode (internal): ", P_NODE);
                char *hd = lstring::copy(tbuf);
                sLstr lstr;
                lstr.add_i(pcn->index());
                lstr.add_c(' ');
                lstr.add_i(pcn->beg_range());
                lstr.add_c(' ');
                lstr.add_i(pcn->end_range());
                if (pcn->has_name()) {
                    lstr.add_c(' ');
                    char *s = pcn->full_name();
                    lstr.add(s);
                    delete [] s;
                }
                char *str = lstr.string_trim();
                list = new Ptxt(hd, str, pcn, list);
            }
            CDp_cnode *pn = (CDp_cnode*)odesc->prpty(P_NODE);
            for ( ; pn; pn = pn->next()) {
                sprintf(tbuf, "(%d) node (internal): ", P_NODE);
                char *hd = lstring::copy(tbuf);
                sLstr lstr;
                if (pn->get_term_name()) {
                    lstr.add(pn->get_term_name()->string());
                    lstr.add_c(' ');
                }

                // Get the name at the first hot-spot only, if
                // multiple all will be the same.
                int x, y;
                if (pn->get_pos(0, &x, &y)) {
                    hyList *hp = new hyList(HLrefNode);
                    hyEnt *ent = new hyEnt(sdesc, x, y, odesc, HYrefNode,
                        HYorNone);
                    ent->add();
                    hp->set_hent(ent);
                    char *s = hp->string(HYcvPlain, false);
                    hp->free();
                    lstr.add(s);
                    delete [] s;
                }
                char *str = lstr.string_trim();
                list = new Ptxt(hd, str, pn, list);
            }
            CDp_branch *pb = (CDp_branch*)odesc->prpty(P_BRANCH);
            for ( ; pb; pb = pb->next()) {
                sprintf(tbuf, "(%d) branch (internal): ", P_BRANCH);
                char *hd = lstring::copy(tbuf);
                hyList *hp = new hyList(HLrefBranch);
                hyEnt *ent = new hyEnt(sdesc, pb->pos_x(), pb->pos_y(), odesc,
                    HYrefBranch, HYorNone);
                ent->add();
                hp->set_hent(ent);
                char *str = hp->string(HYcvPlain, false);
                hp->free();
                list = new Ptxt(hd, str, pb, list);
            }
            CDp_mutlrf *pm = (CDp_mutlrf*)odesc->prpty(P_MUTLRF);
            for ( ; pm; pm = pm->next()) {
                sprintf(tbuf, "(%d) mut (internal): ", P_MUTLRF);
                char *hd = lstring::copy(tbuf);
                char *str = lstring::copy(pm->string());
                list = new Ptxt(hd, str, pm, list);
            }
            CDp *p = odesc->prpty(XICP_PC);
            if (p) {
                sprintf(tbuf, "(%d) pc_name: ", p->value());
                char *hd = lstring::copy(tbuf);
                char *str = lstring::copy(p->string());
                list = new Ptxt(hd, str, p, list);
            }
            p = odesc->prpty(XICP_PC_PARAMS);
            if (p) {
                sprintf(tbuf, "(%d) pc_params: ", p->value());
                char *hd = lstring::copy(tbuf);
                char *str = lstring::copy(p->string());
                list = new Ptxt(hd, str, p, list);
            }
        }
    }
    else {
        const char *lttok = HYtokPre HYtokLT HYtokSuf;

        CDpl *pl0 = 0;
        for (CDp *pdesc = odesc->prpty_list(); pdesc;
                pdesc = pdesc->next_prp())
            pl0 = new CDpl(pdesc, pl0);
        pl0->sort();
        for (CDpl *pl = pl0; pl; pl = pl->next) {
            CDp *pdesc = pl->pdesc;
            const char *string = pdesc->string() ? pdesc->string() : "";
            if (lstring::prefix(lttok, string))
                // use symbolic form for long text
                string = "[text]";
            if (prpty_internal(pdesc->value())) {
                if (pdesc->value() == XICP_PC)
                    sprintf(tbuf, "(%d) pc_name: ", pdesc->value());
                else if (pdesc->value() == XICP_PC_PARAMS)
                    sprintf(tbuf, "(%d) pc_params: ", pdesc->value());
                else
                    sprintf(tbuf, "(%d): ", pdesc->value());
            }
            else
                sprintf(tbuf, "%d: ", pdesc->value());
            list = new Ptxt(lstring::copy(tbuf), lstring::copy(string),
                pdesc, list);
        }
        pl0->free();

        char *s;
        if (odesc->type() != CDINSTANCE) {
            s = GetPseudoProp(odesc, XprpBB);
            if (s) {
                sprintf(tbuf, "(%d) BB: ", XprpBB);
                list = new Ptxt(lstring::copy(tbuf), s, 0, list);
            }
            s = GetPseudoProp(odesc, XprpLayer);
            if (s) {
                sprintf(tbuf, "(%d) Layer: ", XprpLayer);
                list = new Ptxt(lstring::copy(tbuf), s, 0, list);
            }
        }
        s = GetPseudoProp(odesc, XprpFlags);
        if (s) {
            sprintf(tbuf, "(%d) Flags: ", XprpFlags);
            list = new Ptxt(lstring::copy(tbuf), s, 0, list);
        }
        s = GetPseudoProp(odesc, XprpState);
        if (s) {
            sprintf(tbuf, "(%d) State: ", XprpState);
            list = new Ptxt(lstring::copy(tbuf), s, 0, list);
        }
        s = GetPseudoProp(odesc, XprpGroup);
        if (s) {
            sprintf(tbuf, "(%d) Group: ", XprpGroup);
            list = new Ptxt(lstring::copy(tbuf), s, 0, list);
        }
        if (odesc->type() != CDINSTANCE) {
            s = GetPseudoProp(odesc, XprpCoords);
            if (s) {
                sprintf(tbuf, "(%d) Coords: ", XprpCoords);
                list = new Ptxt(lstring::copy(tbuf), s, 0, list);
            }
            s = GetPseudoProp(odesc, XprpMagn);
            if (s) {
                sprintf(tbuf, "(%d) Magn: ", XprpMagn);
                list = new Ptxt(lstring::copy(tbuf), s, 0, list);
            }
        }
        if (odesc->type() == CDWIRE) {
            s = GetPseudoProp(odesc, XprpWwidth);
            if (s) {
                sprintf(tbuf, "(%d) Wwidth: ", XprpWwidth);
                list = new Ptxt(lstring::copy(tbuf), s, 0, list);
            }
            s = GetPseudoProp(odesc, XprpWstyle);
            if (s) {
                sprintf(tbuf, "(%d) Wstyle: ", XprpWstyle);
                list = new Ptxt(lstring::copy(tbuf), s, 0, list);
            }
        }
        if (odesc->type() == CDLABEL) {
            s = GetPseudoProp(odesc, XprpText);
            if (s) {
                sprintf(tbuf, "(%d) Text: ", XprpText);
                list = new Ptxt(lstring::copy(tbuf), s, 0, list);
            }
            s = GetPseudoProp(odesc, XprpXform);
            if (s) {
                sprintf(tbuf, "(%d) Xform: ", XprpXform);
                list = new Ptxt(lstring::copy(tbuf), s, 0, list);
            }
        }
        if (odesc->type() == CDINSTANCE) {
            s = GetPseudoProp(odesc, XprpArray);
            if (s) {
                sprintf(tbuf, "(%d) Array: ", XprpArray);
                list = new Ptxt(lstring::copy(tbuf), s, 0, list);
            }
            s = GetPseudoProp(odesc, XprpTransf);
            if (s) {
                sprintf(tbuf, "(%d) Transf: ", XprpTransf);
                list = new Ptxt(lstring::copy(tbuf), s, 0, list);
            }
            s = GetPseudoProp(odesc, XprpMagn);
            if (s) {
                sprintf(tbuf, "(%d) Magn: ", XprpMagn);
                list = new Ptxt(lstring::copy(tbuf), s, 0, list);
            }
            s = GetPseudoProp(odesc, XprpName);
            if (s) {
                sprintf(tbuf, "(%d) Name: ", XprpName);
                list = new Ptxt(lstring::copy(tbuf), s, 0, list);
            }
        }
        s = GetPseudoProp(odesc, XprpXY);
        if (s) {
            sprintf(tbuf, "(%d) XY: ", XprpXY);
            list = new Ptxt(lstring::copy(tbuf), s, 0, list);
        }
        if (odesc->type() != CDINSTANCE) {
            s = GetPseudoProp(odesc, XprpWidth);
            if (s) {
                sprintf(tbuf, "(%d) Width: ", XprpWidth);
                list = new Ptxt(lstring::copy(tbuf), s, 0, list);
            }
            s = GetPseudoProp(odesc, XprpHeight);
            if (s) {
                sprintf(tbuf, "(%d) Height: ", XprpHeight);
                list = new Ptxt(lstring::copy(tbuf), s, 0, list);
            }
        }
    }
    if (list) {
        // reverse order
        Ptxt *l0 = list;
        list = 0;
        while (l0) {
            Ptxt *lx = l0->next();
            l0->set_next(list);
            list = l0;
            l0 = lx;
        }
    }
    return (list);
}


// Return the physical value corresponding to the given pseudo-
// property.  If this returns a string, the string should contain
// something printable, otherwise the line in the Properties panel can
// not be selected.
//
char *
cMain::GetPseudoProp(CDo *odesc, int val)
{
    if (!odesc)
        return (0);
    char buf[128];
    if (val == XprpType) {
        sprintf(buf, "%c", odesc->type() ? odesc->type() : '0');
        return (lstring::copy(buf));
    }
    if (val == XprpBB) {
        sprintf(buf, "%d,%d %d,%d", odesc->oBB().left, odesc->oBB().bottom,
            odesc->oBB().right, odesc->oBB().top);
        return (lstring::copy(buf));
    }
    if (val == XprpLayer)
        return (lstring::copy(odesc->ldesc()->name()));
    if (val == XprpFlags) {
        int expf = (CDexpand | (CDexpand<<1) | (CDexpand<<2) |
            (CDexpand<<3) | (CDexpand<<4));
        buf[0] = 0;
        if (odesc->has_flag(expf)) {
            int mask = CDexpand;
            char tbuf[8];
            char *s = tbuf;
            for (int i = 0; i < 5; i++) {
                if (odesc->has_flag(mask))
                    *s++ = '0' + i;
                mask <<= 1;
            }
            *s = 0;
            sprintf(buf, "%s", tbuf);
        }
        int l = strlen(buf);
        for (FlagDef *f = OdescFlags; f->name; f++) {
            if (odesc->has_flag(f->value) && f->value != CDexpand) {
                sprintf(buf + l, l ? " %s" : "%s", f->name);
                l = strlen(buf);
            }
        }
        if (buf[0])
            return (lstring::copy(buf));
        return (lstring::copy("none"));
    }
    if (val == XprpState) {
        if (odesc->state() == CDVanilla)
            return (lstring::copy("Normal"));
        else if (odesc->state() == CDSelected)
            return (lstring::copy("Selected"));
        else if (odesc->state() == CDDeleted)
            return (lstring::copy("Deleted"));
        else if (odesc->state() == CDIncomplete)
            return (lstring::copy("Incomplete"));
        else if (odesc->state() == CDInternal)
            return (lstring::copy("Internal"));
        return (lstring::copy("Bad"));
    }
    if (val == XprpGroup) {
        sprintf(buf, "%d", odesc->group());
        return (lstring::copy(buf));
    }

#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
    {
        if (val == XprpMagn) {
            strcpy(buf, "1.000000");
            return (lstring::copy(buf));
        }
        if (val == XprpCoords) {
            Point p[5];
            p[0].x = odesc->oBB().left;
            p[0].y = odesc->oBB().bottom;
            p[1].x = odesc->oBB().left;
            p[1].y = odesc->oBB().top;
            p[2].x = odesc->oBB().right;
            p[2].y = odesc->oBB().top;
            p[3].x = odesc->oBB().right;
            p[3].y = odesc->oBB().bottom;
            p[4].x = odesc->oBB().left;
            p[4].y = odesc->oBB().bottom;
            return (cGEO::path_string(p, 5, 0));
        }
        if (val ==  XprpXY) {
            Point p;
            p.x = odesc->oBB().left;
            p.y = odesc->oBB().bottom;
            return (cGEO::path_string(&p, 1, 0));
        }
        if (val == XprpWidth) {
            sprintf(buf, "%d", odesc->oBB().width());
            return (lstring::copy(buf));
        }
        if (val == XprpHeight) {
            sprintf(buf, "%d", odesc->oBB().height());
            return (lstring::copy(buf));
        }
        return (0);
    }
poly:
    {
        if (val == XprpMagn) {
            strcpy(buf, "1.000000");
            return (lstring::copy(buf));
        }
        if (val == XprpCoords) {
            return (cGEO::path_string(((CDpo*)odesc)->points(),
                ((CDpo*)odesc)->numpts(), 0));
        }
        if (val ==  XprpXY) {
            Point p;
            p.x = ((CDpo*)odesc)->points()->x;
            p.y = ((CDpo*)odesc)->points()->y;
            return (cGEO::path_string(&p, 1, 0));
        }
        if (val == XprpWidth) {
            sprintf(buf, "%d", odesc->oBB().width());
            return (lstring::copy(buf));
        }
        if (val == XprpHeight) {
            sprintf(buf, "%d", odesc->oBB().height());
            return (lstring::copy(buf));
        }
        return (0);
    }
wire:
    {
        if (val == XprpMagn) {
            strcpy(buf, "1.000000");
            return (lstring::copy(buf));
        }
        if (val == XprpCoords)
            return (cGEO::path_string(((CDw*)odesc)->points(),
                ((CDw*)odesc)->numpts(), 0));
        if (val == XprpWwidth) {
            sprintf(buf, "%d", ((CDw*)odesc)->wire_width());
            return (lstring::copy(buf));
        }
        if (val == XprpWstyle) {
            switch (((CDw*)odesc)->wire_style()) {
            case CDWIRE_FLUSH:
                return (lstring::copy("FLUSH"));
            case CDWIRE_ROUND:
                return (lstring::copy("ROUNDED"));
            default:
                break;
            }
            return (lstring::copy("EXTENDED"));
        }
        if (val ==  XprpXY) {
            Point p;
            p.x = ((CDw*)odesc)->points()->x;
            p.y = ((CDw*)odesc)->points()->y;
            return (cGEO::path_string(&p, 1, 0));
        }
        if (val == XprpWidth) {
            sprintf(buf, "%d", odesc->oBB().width());
            return (lstring::copy(buf));
        }
        if (val == XprpHeight) {
            sprintf(buf, "%d", odesc->oBB().height());
            return (lstring::copy(buf));
        }
        return (0);
    }
label:
    {
        if (val == XprpMagn) {
            strcpy(buf, "1.000000");
            return (lstring::copy(buf));
        }
        if (val == XprpCoords) {
            Point p[5];
            p[0].x = odesc->oBB().left;
            p[0].y = odesc->oBB().bottom;
            p[1].x = odesc->oBB().left;
            p[1].y = odesc->oBB().top;
            p[2].x = odesc->oBB().right;
            p[2].y = odesc->oBB().top;
            p[3].x = odesc->oBB().right;
            p[3].y = odesc->oBB().bottom;
            p[4].x = odesc->oBB().left;
            p[4].y = odesc->oBB().bottom;
            return (cGEO::path_string(p, 5, 0));
        }
        if (val == XprpText) {
            char *s = (((CDla*)odesc)->label()->string(HYcvPlain, true));
            if (!s)
                s = lstring::copy("(null)");
            else if (!*s)
                s = lstring::copy("(empty)");
            return (s);
        }
        if (val ==  XprpXform)
            return (xform_to_string(((CDla*)odesc)->xform()));
        if (val ==  XprpXY) {
            Point p;
            p.x = ((CDla*)odesc)->xpos();
            p.y = ((CDla*)odesc)->ypos();
            return (cGEO::path_string(&p, 1, 0));
        }
        if (val == XprpWidth) {
            sprintf(buf, "%d", odesc->oBB().width());
            return (lstring::copy(buf));
        }
        if (val == XprpHeight) {
            sprintf(buf, "%d", odesc->oBB().height());
            return (lstring::copy(buf));
        }
        return (0);
    }
inst:
    {
        if (val == XprpMagn) {
            CDtx tx((CDc*)odesc);
            sprintf(buf, "%.6f", tx.magn > 0.0 ? tx.magn : 1.0);
            return (lstring::copy(buf));
        }
        if (val == XprpCoords) {
            Point p[5];
            p[0].x = odesc->oBB().left;
            p[0].y = odesc->oBB().bottom;
            p[1].x = odesc->oBB().left;
            p[1].y = odesc->oBB().top;
            p[2].x = odesc->oBB().right;
            p[2].y = odesc->oBB().top;
            p[3].x = odesc->oBB().right;
            p[3].y = odesc->oBB().bottom;
            p[4].x = odesc->oBB().left;
            p[4].y = odesc->oBB().bottom;
            return (cGEO::path_string(p, 5, 0));
        }
        if (val == XprpArray) {
            CDap ap((CDc*)odesc);
            sprintf(buf, "%d,%d %d,%d",
                ap.nx >= 1 ? ap.nx : 1,
                ap.ny >= 1 ? ap.ny : 1,
                ap.dx > 0 ? ap.dx : odesc->oBB().width(),
                ap.dy > 0 ? ap.dy : odesc->oBB().height());
            return (lstring::copy(buf));
        }
        if (val == XprpTransf) {
            buf[0] = 0;
            CDtx tx((CDc*)odesc);
            if (tx.refly)
                strcat(buf, " MY");
            if (tx.ax != 1 || tx.ay != 0) {
                sprintf(buf + strlen(buf), " R ");
                const char *str = "??";
                if (tx.ay == 1) {
                    if (tx.ax == 1)
                        str = "45";
                    else if (tx.ax == 0)
                        str = "90";
                    else if (tx.ax == -1)
                        str = "135";
                }
                else if (tx.ay == 0) {
                    if (tx.ax == -1)
                        str = "180";
                }
                else if (tx.ay == -1) {
                    if (tx.ax == -1)
                        str = "225";
                    else if (tx.ax == 0)
                        str = "270";
                    else if (tx.ax == 1)
                        str = "315";
                }
                strcat(buf, str);
            }
            if (tx.tx != 0 || tx.ty != 0)
                sprintf(buf + strlen(buf), " T %d %d", tx.tx, tx.ty);
            if (*buf)
                return (lstring::copy(buf + 1));
            return (lstring::copy("T 0 0"));
        }
        if (val == XprpName)
            return (lstring::copy(((CDc*)odesc)->cellname()->string()));
        if (val ==  XprpXY) {
            Point p;
            CDtx tx((CDc*)odesc);
            p.x = tx.tx;
            p.y = tx.ty;
            return (cGEO::path_string(&p, 1, 0));
        }
        if (val == XprpWidth) {
            sprintf(buf, "%d", odesc->oBB().width());
            return (lstring::copy(buf));
        }
        if (val == XprpHeight) {
            sprintf(buf, "%d", odesc->oBB().height());
            return (lstring::copy(buf));
        }
        return (0);
    }
}


// If the object boundary is visible in one of the display windows,
// return true.
//
bool
cMain::IsBoundaryVisible(const CDs *sd, const CDo *od)
{
    if (!sd || !od)
        return (false);
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(od)
#else
    CONDITIONAL_JUMP(od)
#endif
box:
label:
inst:
    {
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (wdesc->IsShowing(sd)) {
                if (od->oBB().left   <= wdesc->Window()->left   &&
                    od->oBB().right  >= wdesc->Window()->right  &&
                    od->oBB().bottom <= wdesc->Window()->bottom &&
                    od->oBB().top    >= wdesc->Window()->top)
                    continue;
                return (true);
            }
        }
        return (false);
    }
poly:
    {
        int num = ((const CDpo*)od)->numpts();
        const Point *pts = ((const CDpo*)od)->points();
        for (int i = 0; i < num - 1; i++) {
            Point p1(pts[i]);
            Point p2(pts[i+1]);
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0) {
                if (wdesc->IsShowing(sd)) {
                    if (!cGEO::line_clip(&p1.x, &p1.y, &p2.x, &p2.y,
                            wdesc->Window()))
                        return (true);
                }
            }
        }
        return (false);
    }
wire:
    {
        Poly po;
        if (!((const CDw*)od)->w_toPoly(&po.points, &po.numpts))
            return (false);

        for (int i = 0; i < po.numpts - 1; i++) {
            Point p1(po.points[i]);
            Point p2(po.points[i+1]);
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0) {
                if (wdesc->IsShowing(sd)) {
                    if (!cGEO::line_clip(&p1.x, &p1.y, &p2.x, &p2.y,
                            wdesc->Window())) {
                        delete [] po.points;
                        return (true);
                    }
                }
            }
        }
        delete [] po.points;
        return (false);
    }
}
// End of cMain functions

