
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2012 Whiteley Research Inc, all rights reserved.        *
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
 $Id: abut.cc,v 1.23 2015/03/13 06:37:07 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "edit.h"
#include "pcell_params.h"
#include "cd_propnum.h"
#include "cd_lgen.h"
#include "dsp_inlines.h"
#include "errorlog.h"
#include "abut.h"

#include <sys/time.h>
#include <time.h>


// Instantiate abutment control class.
namespace {
    cAbutCtrl AC;
}


// This should be called when a new PCell instance is added to the
// database.  Simply add it to a list.
//
void
cEdit::checkAbutment(CDc *cdesc)
{
    AC.addInstance(cdesc);
}


// Call this at the end of an operation, after all new instances have
// been recorded.  This takes care of actual reverting and abutting.
//
void
cEdit::handleAbutment()
{
    AC.handleAbutment();
}
// End of cEdit functions.


void
cAbutCtrl::addInstance(CDc *cdesc)
{
    if (ac_no_check)
        return;
    CDs *msd = cdesc->masterCell();
    if (!msd || !msd->isPCell())
        return;

    ac_abut_instances = new CDol(cdesc, ac_abut_instances);
}


void
cAbutCtrl::handleAbutment()
{
    // Use a table to avoid changing id twice.
    SymTab id_tab(false, false);

    // First pass, update properties, maybe revert partners.
    for (CDol *ol = ac_abut_instances; ol; ol = ol->next) {
        CDc *cdesc = (CDc*)ol->odesc;
        if (cdesc->state() == CDDeleted)
            continue;

        // If the XICP_AB_COPY property is found, then cdesc is a copy
        // and we don't want to revert the partners.
        bool no_ptnr = cdesc->prpty(XICP_AB_COPY);
        if (no_ptnr)
            cdesc->prptyRemove(XICP_AB_COPY);

        CDp *p0 = cdesc->prpty(XICP_AB_PRIOR);
        for (CDp *p = p0; p; p = p->next_n()) {
            sAbutPrior ap(cdesc);
            if (!ap.parse(p->string())) {
                Log()->WarningLog(mh::Initialization, Errs()->get_error());
                continue;
            }
            CDp *p2;
            CDc *cd2 = ap.findNewPartnerInList(ac_abut_instances, &p2);
            if (cd2) {
                // Update properties, provide a new id, and update pin
                // locations.

                sAbutPrior ap2(cd2);
                ap2.parse(p2->string());  // This won't fail.
                if (SymTab::get(&id_tab, ap2.id_number()) != ST_NIL) {
                    // We have already updated this pair.
                    continue;
                }

                unsigned int id = sAbutPrior::newId(cdesc, cd2);
                id_tab.add(id, 0, false);
                ap.set_id_number(id);
                ap2.set_id_number(id);
                ap.updatePin();
                ap2.updatePin();
                char *pstr = ap.string();
                p->set_string(pstr);
                delete [] pstr;
                pstr = ap2.string();
                p2->set_string(pstr);
                delete [] pstr;

                // We're done with this, both partners were moved/copied.
                continue;
            }
            if (!no_ptnr && !ap.revertPartnerAbutment()) {
                Errs()->add_error("handleAbutment: partner revert failed.");
                Log()->WarningLog(mh::Initialization, Errs()->get_error());
            }
        }
    }

    // Second pass, check and do abutments.
    for (CDol *ol = ac_abut_instances; ol; ol = ol->next) {
        CDc *cdesc = (CDc*)ol->odesc;
        if (cdesc->state() == CDDeleted)
            continue;

        CDp *p_prms = cdesc->prpty(XICP_PC_PARAMS);
        if (!p_prms) {
            Errs()->add_error("handleAbutment: missing parameters property.");
            Log()->WarningLog(mh::Initialization, Errs()->get_error());
            continue;
        }

        PCellParam *prms = 0;
        CDp *p0 = cdesc->prpty_list();
        CDp *pp = 0, *pn;
        for (CDp *p = p0; p; p = pn) {
            pn = p->next_prp();
            if (p->value() != XICP_AB_PRIOR) {
                pp = p;
                continue;
            }

            sAbutPrior ap(cdesc);
            if (!ap.parse(p->string())) {
                pp = p;
                continue;
            }
            if (ap.findNewPartnerInList(ac_abut_instances, 0)) {
                pp = p;
                continue;
            }

            if (!prms) {
                if (!PCellParam::parseParams(p_prms->string(), &prms)) {
                    Errs()->add_error(
                        "handleAbutment: parameters parse failed.");
                    pp = p;
                    continue;
                }
            }
            for (const sAbutKeyVal *k = ap.params(); k; k = k->nextc())
                prms->setValue(k->key(), k->value());
            if (pp)
                pp->set_next_prp(pn);
            else
                cdesc->set_prpty_list(pn);
            delete p;
        }
        cAbutHandler a;
        a.set_params(prms);

        // Next we test for potential abutments involving cdesc.  We look
        // for other suitably overlapping instances of the same PCell.

        if (!a.checkAbutment(cdesc)) {
            Log()->ErrorLog(mh::Initialization, Errs()->get_error());
            continue;
        }

        // This will handle the abutment or un-abutment of cdesc and any
        // new abutment partners.

        if (!a.handleAbutment()) {
            Log()->ErrorLog(mh::Initialization, Errs()->get_error());
            continue;
        }
    }
    CDol::destroy(ac_abut_instances);
    ac_abut_instances = 0;
}
// End of cAbutCtrl functions.


// Look for possible abutments for cdesc.  These are listed in the
// ah_list field when this function returns.
//
bool
cAbutHandler::checkAbutment(CDc *cdesc)
{
    // Make sure that master is a sub-master.
    CDs *msdesc = cdesc->masterCell();
    if (!msdesc || !msdesc->isPCellSubMaster())
        return (false);

    ah_cdesc1 = cdesc;

    // Find the instance BB.  The instance can't be arrayed.
    BBox BB(*msdesc->BB());
    cTfmStack stk;
    stk.TPush();
    stk.TApplyTransform(cdesc);
    stk.TBB(&BB, 0);
    stk.TPop();

    // Look for other overlapping pcell instances.
    CDg gdesc;
    stk.TInitGen(CurCell(), CellLayer(), &BB, &gdesc);
    CDc *cd2;
    while ((cd2 = (CDc*)gdesc.next()) != 0) {
        if (cd2 == cdesc)
            continue;
        if (!cd2->is_normal())
            continue;
        CDs *msdesc2 = cd2->masterCell();
        if (!msdesc2 || !msdesc2->isPCellSubMaster())
            continue;
        checkAbut(cdesc, cd2);
    }
    return (true);
}


bool
cAbutHandler::handleAbutment()
{
    if (!ah_cdesc1) {
        Errs()->add_error("handleAbutment: no instance given.");
        return (false);
    }
    if (!ah_list) {
        // No abutting.  If there is an ah_prms1 list, then we need to
        // reinstantiate to revert.  Otherwise, we're done.
        if (ah_prms1 && !setNewParams(ah_cdesc1, ah_prms1)) {
            Errs()->add_error("handleAbutment: failed to revert cell.");
            return (false);
        }
        return (true);
    }

    for (sAbutItem *ai = ah_list; ai; ai = ai->next()) {
        int npins = ED()->pcellAbutMode();

        CDp *pn = ai->odesc1()->prpty(XICP_AB_RULES);
        if (!pn || !pn->string()) {
            Errs()->add_error(
                "handleAbutment: instance 1, missing abutment rules property.");
            return (false);
        }

        sAbutRule *rules1;
        if (!sAbutRule::parseRules(pn->string(), &rules1)) {
            Errs()->add_error("handleAbutment: instance 1, rules parse error.");
            return (false);
        }
#ifdef AB_DEBUG
        rules1->print_all(stdout);
#endif

        pn = ai->odesc2()->prpty(XICP_AB_RULES);
        if (!pn || !pn->string()) {
            Errs()->add_error(
                "handleAbutment: instance 2, missing abutment rules property.");
            return (false);
        }

        sAbutRule *rules2;
        if (!sAbutRule::parseRules(pn->string(), &rules2)) {
            Errs()->add_error("handleAbutment: instance 2, rules parse error.");
            return (false);
        }
#ifdef AB_DEBUG
        rules2->print_all(stdout);
#endif

        // Set up the abutment.
        aRuleType rule1 = bogusValue, rule2 = bogusValue;
        if (npins == 0) {
            if (ai->info1()->pin_size() < ai->info2()->pin_size()) {
                rule1 = adjSmaller;
                rule2 = adjBigger;
            }
            else if (ai->info1()->pin_size() == ai->info2()->pin_size()) {
                rule1 = adjEqual;
                rule2 = adjEqual;
            }
            else {
                rule1 = adjBigger;
                rule2 = adjSmaller;
            }
        }
        else if (npins == 1) {
            if (ai->info1()->pin_size() < ai->info2()->pin_size()) {
                rule1 = abut2PinSmaller;
                rule2 = abut2PinBigger;
            }
            else if (ai->info1()->pin_size() == ai->info2()->pin_size()) {
                rule1 = abut2PinEqual;
                rule2 = abut2PinEqual;
            }
            else {
                rule1 = abut2PinBigger;
                rule2 = abut2PinSmaller;
            }
        }
        else if (npins == 2) {
            if (ai->info1()->pin_size() < ai->info2()->pin_size()) {
                rule1 = abut3PinSmaller;
                rule2 = abut3PinBigger;
            }
            else if (ai->info1()->pin_size() == ai->info2()->pin_size()) {
                rule1 = abut3PinEqual;
                rule2 = abut3PinEqual;
            }
            else {
                rule1 = abut3PinBigger;
                rule2 = abut3PinSmaller;
            }
        }

        sAbutRule *r1 = sAbutRule::find(rules1, rule1);
        if (!r1) {
            Errs()->add_error("handleAbutment: shape 1, missing %s rule.",
                sAbutRule::rule_name(rule1));
            return (false);
        }
#ifdef AB_DEBUG
        r1->print(stdout);
#endif

        sAbutRule *r2 = sAbutRule::find(rules2, rule2);
        if (!r2) {
            Errs()->add_error("handleAbutment: shape 2, missing %s rule.",
                sAbutRule::rule_name(rule2));
            return (false);
        }
#ifdef AB_DEBUG
        r2->print(stdout);
#endif

        // Set parameters for abutment.
        if (!ah_prms1) {
            CDp *p_prms = ah_cdesc1->prpty(XICP_PC_PARAMS);
            if (!p_prms) {
                Errs()->add_error(
                    "handleAbutment:  missing pc_params property.");
                return (false);
            }
            if (!PCellParam::parseParams(p_prms->string(), &ah_prms1))
                return (false);
        }
        sAbutKeyVal *kv1 = 0, *ke = 0;
        for (const sAbutKeyVal *p = r1->moving_keys(); p; p = p->nextc()) {
            char *v = ah_prms1->getValueByName(p->key());
            if (v) {
                sAbutKeyVal *kv = new sAbutKeyVal(lstring::copy(p->key()), v);
                if (!kv1)
                    kv1 = ke = kv;
                else {
                    ke->set_next(kv);
                    ke = kv;
                }
            }
            ah_prms1->setValue(p->key(), p->value());
        }

        CDp *p_prms = ai->cdesc2()->prpty(XICP_PC_PARAMS);
        if (!p_prms) {
            Errs()->add_error(
                "handleAbutment:  missing pc_params property.");
            return (false);
        }

        PCellParam *prms2;
        if (!PCellParam::parseParams(p_prms->string(), &prms2))
            return (false);
        sAbutKeyVal *kv2 = 0;
        ke = 0;
        for (const sAbutKeyVal *p = r2->fixed_keys(); p; p = p->nextc()) {
            char *v = prms2->getValueByName(p->key());
            if (v) {
                sAbutKeyVal *kv = new sAbutKeyVal(lstring::copy(p->key()), v);
                if (!kv2)
                    kv2 = ke = kv;
                else {
                    ke->set_next(kv);
                    ke = kv;
                }
            }
            prms2->setValue(p->key(), p->value());
        }

        // Create a unique id number for the prior property pair.
        unsigned int id = sAbutPrior::newId(ah_cdesc1, ai->cdesc2());

        sAbutPrior ap1(ah_cdesc1, id, ai, kv1);
        if (!ap1.class_name() || !ap1.self_shape_name() ||
                !ap1.ptnr_shape_name()) {
            Errs()->add_error(
                "handleAbutment: error creating prior property1.\n");
            sAbutKeyVal::destroy(kv2);
            return (false);
        }
        char *pstr = ap1.string();
        CDp *pprior1 = new CDp(pstr, XICP_AB_PRIOR);
        delete [] pstr;

        sAbutPrior ap2(0, id, ai, kv2);
        if (!ap2.class_name() || !ap2.self_shape_name() ||
                !ap2.ptnr_shape_name()) {
            Errs()->add_error(
                "handleAbutment: error creating prior property1.\n");
            delete pprior1;
            return (false);
        }
        pstr = ap2.string();
        CDp *pprior2 = new CDp(pstr, XICP_AB_PRIOR);
        delete [] pstr;

        CDc *c1new;
        if (!setNewParams(ah_cdesc1, ah_prms1, &c1new))
            return (false);
        if (c1new) {
            pprior1->set_next_prp(c1new->prpty_list());
            c1new->set_prpty_list(pprior1);
            ah_cdesc1 = c1new;
        }

        CDc *c2new;
        if (!setNewParams(ai->cdesc2(), prms2, &c2new)) {
            PCellParam::destroy(prms2);
            return (false);
        }
        PCellParam::destroy(prms2);
        if (c2new) {
            pprior2->set_next_prp(c2new->prpty_list());
            c2new->set_prpty_list(pprior2);
            ai->set_cdesc2(c2new);
        }

        // Move cdesc1 into position.
        if (!fixPosition(ai, r1, r2, c1new, c2new)) {
            Errs()->add_error("Failed to reposition abutted instance.");
            Log()->WarningLog(mh::Initialization, Errs()->get_error());
        }
    }
    return (true);
}


// static function.
// A wrapper to reparameterize the instance.  The new CDc is returned
// in cnew, it this argument is not null.  The existing instance
// pointer should not be used again.
//
bool
cAbutHandler::setNewParams(CDc *cdesc, const PCellParam *prms, CDc **pnew)
{
    char *nstr = prms->string(true);
    CDp *nprp = new CDp(nstr, XICP_PC_PARAMS);
    delete [] nstr;

    AC.setNoCheck(true);
    bool ret = ED()->reparamInstance(CurCell(), cdesc, nprp, pnew);
    delete nprp;
    if (!ret)
        Errs()->add_error("setNewParams: reparamInstance failed.");
    AC.setNoCheck(false);
    return (ret);
}


namespace {
    CDo *findPin(CDs *sd, const char *cls, const char *shp, const CDl *ld)
    {
        CDg gdesc;
        gdesc.init_gen(sd, ld);
        CDo *od;
        while ((od = gdesc.next()) != 0) {
            sAbutPinInfo info;
            if (!info.setup(od))
                continue;
            if (!strcmp(cls, info.class_name()) &&
                    !strcmp(shp, info.shape_name()))
                return (od);

        }
        return (0);
    }

    enum DirType { DirNone, DirLeft, DirBottom, DirRight, DirTop };

    DirType findDirection(const char *dir)
    {
        if (dir) {
            if (!strcasecmp(dir, "left"))
                return (DirLeft);
            if (!strcasecmp(dir, "bottom"))
                return (DirBottom);
            if (!strcasecmp(dir, "right"))
                return (DirRight);
            if (!strcasecmp(dir, "top"))
                return (DirTop);
        }
        return (DirNone);
    }
}


// Private function.
// This will move c1new into place, following an abutment trigger.
//
bool
cAbutHandler::fixPosition(const sAbutItem *ai, const sAbutRule *r1,
    const sAbutRule *r2, CDc *c1new, const CDc *c2new)
{
    if (!ai || !r1 || !r2) {
        Errs()->add_error("unexpected null argument");
        return (false);
    }
    if (!c1new) {
        Errs()->add_error("null instance pointer 1");
        return (false);
    }
    CDs *msd1 = c1new->masterCell();
    if (!msd1) {
        Errs()->add_error("null master pointer 1");
        return (false);
    }
    const char *cls1 = ai->info1()->class_name();
    if (!cls1) {
        Errs()->add_error("null class name 1");
        return (false);
    }
    const char *shp1 = ai->info1()->shape_name();
    if (!shp1) {
        Errs()->add_error("null shape name 1");
        return (false);
    }

    if (!c2new) {
        Errs()->add_error("null instance pointer 2");
        return (false);
    }
    CDs *msd2 = c2new->masterCell();
    if (!msd2) {
        Errs()->add_error("null master pointer 2");
        return (false);
    }
    const char *cls2 = ai->info2()->class_name();
    if (!cls2) {
        Errs()->add_error("null class name 2");
        return (false);
    }
    const char *shp2 = ai->info2()->shape_name();
    if (!shp2) {
        Errs()->add_error("null shape name 2");
        return (false);
    }

    CDl *ld = ai->odesc1()->ldesc();
    if (!ld) {
        Errs()->add_error("null layer descriptor");
        return (false);
    }

    // Find the two pin objects, we align with these.
    CDo *od1 = findPin(msd1, cls1, shp1, ld);
    if (!od1) {
        Errs()->add_error("failed to find pin 1");
        return (false);
    }
    CDo *od2 = findPin(msd2, cls2, shp2, ld);
    if (!od2) {
        Errs()->add_error("failed to find pin 2");
        return (false);
    }
    BBox BB1(od1->oBB());
    BBox BB2(od2->oBB());

    int xo = c1new->posX();
    int yo = c1new->posY();

    // Transform BB1 and xo,yo into the msd2 space.
    cTfmStack stk;
    stk.TPush();
    stk.TApplyTransform(c2new);
    stk.TInverse();
    stk.TLoadInverse();
    stk.TPoint(&xo, &yo);
    stk.TPush();
    stk.TApplyTransform(c1new);
    stk.TPremultiply();
    stk.TBB(&BB1, 0);
    stk.TPop();
    stk.TPop();

    // Find the direction to use as a reference.  We know that the
    // c1new direction is compatible.
    DirType d2 = findDirection(ai->info2()->directions());

    // The two spacing values should be the same.
    double spacing = mmMax(r1->spacing(), r2->spacing());
    int sp = INTERNAL_UNITS(spacing);

    // Find the offset that needs to be applied to BB1 to move it
    // into the correct location.
    int dx = 0, dy = 0;
    if (d2 == DirLeft) {
        dx = BB2.left - BB1.right + sp;
        dy = BB2.bottom - BB1.bottom;
    }
    else if (d2 == DirRight) {
        dx = BB2.right - BB1.left + sp;
        dy = BB2.bottom - BB1.bottom;
    }
    else if (d2 == DirBottom) {
        dx = BB2.left - BB1.left;
        dy = BB2.bottom - BB1.top + sp;
    }
    else if (d2 == DirTop) {
        dx = BB2.left - BB1.left;
        dy = BB2.top - BB1.bottom + sp;
    }

    // Add this offset to the transformed c1new placement location,
    // and transform back to parent cell coordinates.
    xo += dx;
    yo += dy;
    stk.TPush();
    stk.TApplyTransform(c2new);
    stk.TPoint(&xo, &yo);
    stk.TPop();

    // Move c1new.
    CDs *sd = CurCell(Physical);
    sd->db_remove(c1new);
    if (!(*sd->BB() > c1new->oBB()))
        sd->setBBvalid(false);
    c1new->setPosX(xo);
    c1new->setPosY(yo);
    c1new->computeBB();
    CurCell()->db_insert(c1new);
    if (!(*sd->BB() > c1new->oBB()))
        sd->setBBvalid(false);

    return (true);
}


// Private function.
// Return true if cd1 and cd2 abut.
//
bool
cAbutHandler::checkAbut(CDc *cd1, CDc *cd2)
{
    CDs *sd1 = cd1->masterCell();
    CDs *sd2 = cd2->masterCell();
    BBox BB1(*sd1->BB());
    BBox BB2(*sd2->BB());

    cTfmStack stk;
    stk.TPush();
    stk.TApplyTransform(cd1);
    stk.TBB(&BB1, 0);
    stk.TPop();
    stk.TPush();
    stk.TApplyTransform(cd2);
    stk.TBB(&BB2, 0);
    stk.TPop();
    BBox BBov(mmMax(BB1.left, BB2.left), mmMax(BB1.bottom, BB2.bottom),
        mmMin(BB1.right, BB2.right), mmMin(BB1.top, BB2.top));
    if (BBov.top <= BBov.bottom || BBov.right <= BBov.left)
        return (false);
    // BBov is the overlap area of the two instances.

    // Look for material with the XICP_AB_CLASS property set.
    stk.TPush();
    stk.TApplyTransform(cd1);

    CDsLgen lgen(sd1);
    CDl *ld;
    while ((ld = lgen.next()) != 0) {
        CDg gdesc;
        stk.TInitGen(sd1, ld, &BBov, &gdesc);
        CDo *od1;
        while ((od1 = gdesc.next()) != 0) {
            sAbutPinInfo info1;
            if (!info1.setup(od1))
                continue;

            BBox shpBB(od1->oBB());
            stk.TBB(&shpBB, 0);

            cTfmStack stk2;
            stk2.TPush();
            stk2.TApplyTransform(cd2);
            CDg gdesc2;
            stk2.TInitGen(sd2, ld, &shpBB, &gdesc2);
            CDo *od2;
            while ((od2 = gdesc2.next()) != 0) {
                sAbutPinInfo info2;
                if (!info2.setup(od2))
                    continue;
                if (strcmp(info1.class_name(), info2.class_name()))
                    continue;
                if (!sAbutPinInfo::checkDirections(stk, stk2,
                        info1.directions(), info2.directions()))
                    continue;

                sAbutItem *ai = new sAbutItem(cd2, od1, od2, info1, info2);
                ai->set_next(ah_list);
                ah_list = ai;
                stk2.TPop();
                return (true);
            }
            stk2.TPop();
        }
    }
    stk.TPop();
    return (false);
}
// End of cAbutHandler functions.


// Ciranova rule names, order must be the same as enum aRuleType.
//
const char *sAbutRule::ar_rule_names[] =
{
    "noAbut",
    "adjSmaller",
    "adjEqual",
    "adjBigger",
    "abut2PinSmaller",
    "abut2PinEqual",
    "abut2PinBigger",
    "abut3PinSmaller",
    "abut3PinEqual",
    "abut3PinBigger",
    0
};


sAbutRule::sAbutRule()
{
    ar_name = bogusValue;
    ar_spacing = 0.0;
    ar_moving_keys = 0;
    ar_fixed_keys = 0;
    ar_next = 0;
}


sAbutRule::~sAbutRule()
{
    sAbutKeyVal::destroy(ar_moving_keys);
    sAbutKeyVal::destroy(ar_fixed_keys);
}


// Static function.
// Parse the rules string, return an sAbutRule list.
//
bool
sAbutRule::parseRules(const char *str, sAbutRule **pret)
{
    *pret = 0;
    sAbutRule *a0 = 0, *ae = 0;
    const char *s = str;
    char *tok;
    while ((tok = lstring::gettok(&s, "^")) != 0) {
        const char *t = tok;
        char *tok1 = lstring::gettok(&t, ";");
        char *tok2 = lstring::gettok(&t, ";");
        char *tok3 = lstring::gettok(&t, ";");
        delete [] tok;
        sAbutRule *ar = new sAbutRule;
        bool ret = ar->setup(tok1, tok2, tok3);
        delete [] tok2;
        delete [] tok3;

        if (!ret) {
            delete ar;
            sAbutRule::destroy(a0);
            Errs()->add_error("Rule parse failed: token1=%s\n",
                tok1 ? tok1 : "null");
            delete [] tok1;
            return (false);
        }
        delete [] tok1;

        if (!a0)
            a0 = ae = ar;
        else {
            ae->set_next(ar);
            ae = ar;
        }
    }
    *pret = a0;
    return (true);
}


// Parse the three tokens from a rule property.  On error, return false
// with a message in the Errs system.
//
bool
sAbutRule::setup(const char *tok1, const char *tok2, const char *tok3)
{
    // The tok1 contains the _name and _spacing.

    char *tok;
    const char *tchars = ":,";
    const char *t = tok1;

    tok = lstring::gettok(&t, tchars);
    if (!tok || strcmp(tok, "_name")) {
        delete [] tok;
        Errs()->add_error("Rule parse: _name keyword missing.");
        return (false);
    }
    delete [] tok;

    tok = lstring::gettok(&t, tchars);
    ar_name = rule_type(tok);
    if (ar_name == bogusValue) {
        Errs()->add_error("Rule parse: invalid rule name %s.", tok);
        delete [] tok;
        return (false);
    }
    delete [] tok;

    tok = lstring::gettok(&t, tchars);
    if (!tok || strcmp(tok, "_spacing")) {
        delete [] tok;
        Errs()->add_error("Rule parse: _spacing keyword missing.");
        return (false);
    }
    delete [] tok;

    tok = lstring::gettok(&t, tchars);
    if (!tok || sscanf(tok, "%lf", &ar_spacing) != 1) {
        delete [] tok;
        Errs()->add_error("Rule parse: invalid spacing value.");
        return (false);
    }
    delete [] tok;

    // The tok2 contains the moving keyword/values.
    char *key, *val;
    t = tok2;
    sAbutKeyVal *ve = 0;
    for (;;) {
        tok = lstring::gettok(&t, ",");
        if (!tok)
            break;
        const char *tt = tok;
        key = lstring::gettok(&tt, ":");
        val = lstring::gettok(&tt, ":");
        delete [] tok;
        sAbutKeyVal *kv = new sAbutKeyVal(key, val);
        if (!ar_moving_keys)
            ar_moving_keys = ve = kv;
        else {
            ve->set_next(kv);
            ve = kv;
        }
    }

    // The tok3 contains the fixed keyword/values.
    t = tok3;
    ve = 0;
    for (;;) {
        tok = lstring::gettok(&t, ",");
        if (!tok)
            break;
        const char *tt = tok;
        key = lstring::gettok(&tt, ":");
        val = lstring::gettok(&tt, ":");
        delete [] tok;
        sAbutKeyVal *kv = new sAbutKeyVal(key, val);
        if (!ar_fixed_keys)
            ar_fixed_keys = ve = kv;
        else {
            ve->set_next(kv);
            ve = kv;
        }
    }
    return (true);
}


void
sAbutRule::print(FILE *fp) const
{
    const char *nm = rule_name(ar_name);
    fprintf(fp, "Name: %s  Spacing: %g\n", nm ? nm : "null", ar_spacing);
    fprintf(fp, "Moving:\n");
    for (sAbutKeyVal *kv = ar_moving_keys; kv; kv = kv->next())
        fprintf(fp, "  %s: %s\n", kv->key(), kv->value());
    fprintf(fp, "Fixed:\n");
    for (sAbutKeyVal *kv = ar_fixed_keys; kv; kv = kv->next())
        fprintf(fp, "  %s: %s\n", kv->key(), kv->value());
    fprintf(fp, "\n");
}
// End of sAbutRule functions.


sAbutPrior::sAbutPrior(CDc *cd, unsigned int id, sAbutItem *ai,
    sAbutKeyVal *pars)
{
    CDc *cd1;
    CDo *od1, *od2;
    if (cd) {
        cd1 = cd;
        od1 = ai->odesc1();
        od2 = ai->odesc2();
    }
    else {
        cd1 = ai->cdesc2();
        od1 = ai->odesc2();
        od2 = ai->odesc1();
    }
    ap_self = cd1;
    ap_class = 0;
    ap_self_shape = 0;
    ap_ptnr_shape = 0;
    ap_ldesc = 0;
    ap_params = pars;
    ap_id = id;

    CDp *p = od1->prpty(XICP_AB_CLASS);
    if (!p || !p->string())
        Errs()->add_error("abutPrior: no class property.");
    else
        ap_class = lstring::copy(p->string());

    p = od1->prpty(XICP_AB_SHAPENAME);
    if (!p || !p->string())
        Errs()->add_error("abutPrior: no shapename 1 property.");
    else
        ap_self_shape = lstring::copy(p->string());

    p = od2->prpty(XICP_AB_SHAPENAME);
    if (!p || !p->string())
        Errs()->add_error("abutPrior: no shapename 2 property.");
    else
        ap_ptnr_shape = lstring::copy(p->string());

    ap_ldesc = od1->ldesc();

    ap_pinBB = od1->oBB();
    cTfmStack stk;
    stk.TPush();
    stk.TApplyTransform(cd1);
    stk.TBB(&ap_pinBB, 0);
    stk.TPop();
}


bool
sAbutPrior::parse(const char *str)
{
    // id class shape1 shape2 layername left,bottom,right,top name value ...

    const char *tchars = ",;";
    const char *s = str;

    char *tok = lstring::gettok(&s, tchars);
    if (!tok || sscanf(tok, "%u", &ap_id) != 1) {
        delete [] tok;
        Errs()->add_error("Parse reversion:  bad or missing id value.");
        return (false);
    }
    delete [] tok;

    ap_class = lstring::getqtok(&s, tchars);
    if (!ap_class) {
        Errs()->add_error("Parse reversion:  no class name.");
        return (false);
    }

    ap_self_shape = lstring::getqtok(&s, tchars);
    if (!ap_self_shape) {
        Errs()->add_error("Parse reversion:  no shape1 name.");
        return (false);
    }

    ap_ptnr_shape = lstring::getqtok(&s, tchars);
    if (!ap_ptnr_shape) {
        Errs()->add_error("Parse reversion:  no shape2 name.");
        return (false);
    }

    char *lname = lstring::getqtok(&s, tchars);
    if (!lname) {
        Errs()->add_error("Parse reversion:  no layer name.");
        return (false);
    }
    ap_ldesc = CDldb()->findLayer(lname, Physical);
    if (!ap_ldesc) {
        Errs()->add_error("Parse reversion:  bad layer name %s.", lname);
        delete [] lname;
        return (false);
    }
    delete [] lname;

    const char *errmsg = "Parse reversion:  error parsing bounding box.";
    double d;
    tok = lstring::gettok(&s, tchars);
    if (!tok) {
        Errs()->add_error(errmsg);
        return (false);
    }
    if (sscanf(tok, "%lf", &d) != 1) {
        Errs()->add_error(errmsg);
        delete [] tok;
        return (false);
    }
    delete [] tok;
    ap_pinBB.left = INTERNAL_UNITS(d);

    tok = lstring::gettok(&s, tchars);
    if (!tok) {
        Errs()->add_error(errmsg);
        return (false);
    }
    if (sscanf(tok, "%lf", &d) != 1) {
        Errs()->add_error(errmsg);
        delete [] tok;
        return (false);
    }
    delete [] tok;
    ap_pinBB.bottom = INTERNAL_UNITS(d);

    tok = lstring::gettok(&s, tchars);
    if (!tok) {
        Errs()->add_error(errmsg);
        return (false);
    }
    if (sscanf(tok, "%lf", &d) != 1) {
        Errs()->add_error(errmsg);
        delete [] tok;
        return (false);
    }
    delete [] tok;
    ap_pinBB.right = INTERNAL_UNITS(d);

    tok = lstring::gettok(&s, tchars);
    if (!tok) {
        Errs()->add_error(errmsg);
        return (false);
    }
    if (sscanf(tok, "%lf", &d) != 1) {
        Errs()->add_error(errmsg);
        delete [] tok;
        return (false);
    }
    delete [] tok;
    ap_pinBB.top = INTERNAL_UNITS(d);

    sAbutKeyVal *ke = 0;
    while (*s) {
        char typ, *nam, *val;
        if (!PCellParam::getPair(&s, &typ, &nam, &val, 0)) {
            Errs()->add_error(
                "Parse reversion:  property string syntax error.");
            return (false);
        }
        if (nam) {
            sAbutKeyVal *kv = new sAbutKeyVal(nam, val);
            if (!ap_params)
                ap_params = ke = kv;
            else {
                ke->set_next(kv);
                ke = kv;
            }
        }
    }
    return (true);
}


char *
sAbutPrior::string()
{
    sLstr lstr;
    lstr.add_u(ap_id);
    lstr.add_c(',');
    lstr.add(ap_class);
    lstr.add_c(',');
    lstr.add(ap_self_shape);
    lstr.add_c(',');
    lstr.add(ap_ptnr_shape);
    lstr.add_c(',');
    lstr.add(ap_ldesc ? ap_ldesc->name() : "none");
    lstr.add_c(',');
    lstr.add_d(MICRONS(ap_pinBB.left), 4, true);
    lstr.add_c(',');
    lstr.add_d(MICRONS(ap_pinBB.bottom), 4, true);
    lstr.add_c(',');
    lstr.add_d(MICRONS(ap_pinBB.right), 4, true);
    lstr.add_c(',');
    lstr.add_d(MICRONS(ap_pinBB.top), 4, true);

    for (sAbutKeyVal *kv = ap_params; kv; kv = kv->next()) {
        lstr.add_c(',');
        lstr.add(kv->key());
        lstr.add_c('=');
        lstr.add(kv->value());
    }
    return (lstr.string_trim());
}


// Look through the list for the partner cdesc, return it if found,
// along with the matching property if the argument is not null.
//
CDc *
sAbutPrior::findNewPartnerInList(CDol *ol0, CDp **pprior) const
{
    if (pprior)
        *pprior = 0;
    if (!ap_self)
        return (0);
    CDs *msd = ap_self->masterCell();
    if (!msd || !msd->isPCellSubMaster())
        return (0);

    for (CDol *ol = ol0; ol; ol = ol->next) {
        CDc *cd2 = (CDc*)ol->odesc;
        if (cd2->state() == CDDeleted)
            continue;
        CDs *msd2 = cd2->masterCell();
        if (!msd2 || !msd2->isPCellSubMaster())
            continue;
        if (cd2 == ap_self)
            continue;

        for (CDp *p = cd2->prpty(XICP_AB_PRIOR); p; p = p->next_n()) {
            sAbutPrior ap(cd2);
            if (!ap.parse(p->string()))
                continue;
            if (id_number() == ap.id_number() &&
                    !strcmp(class_name(), ap.class_name()) && 
                    !strcmp(ptnr_shape_name(), ap.self_shape_name())) {
                if (pprior)
                    *pprior = p;
                return (cd2);
            }
        }
    }
    return (0);
}


// Find and return the partner cdesc.  The corresponding XICP_AB_PRIOR
// property will returned separately in the second argument.
//
CDc *
sAbutPrior::findPartner(CDp **pprior) const
{
    if (pprior)
        *pprior = 0;
    if (!ap_self)
        return (0);
    CDs *msd = ap_self->masterCell();
    if (!msd || !msd->isPCellSubMaster())
        return (0);

    // We will search the area near the pin for the partner.  We know
    // that it is nearby.
    //
    BBox srchBB(*pinBB());
    {
        int w = msd->BB()->width();
        int h = msd->BB()->height();
        int dim = mmMax(w, h);
        srchBB.bloat(dim);
    }

    cTfmStack stk;
    CDg gdesc;
    stk.TInitGen(CurCell(), CellLayer(), &srchBB, &gdesc);
    CDc *cd2;
    while ((cd2 = (CDc*)gdesc.next()) != 0) {
        if (cd2->state() == CDDeleted)
            continue;
        CDs *msd2 = cd2->masterCell();
        if (!msd2 || !msd2->isPCellSubMaster())
            continue;
        if (cd2 == ap_self)
            continue;

        for (CDp *p = cd2->prpty(XICP_AB_PRIOR); p; p = p->next_n()) {
            sAbutPrior ap(cd2);
            if (!ap.parse(p->string()))
                continue;
            if (id_number() == ap.id_number() &&
                    !strcmp(class_name(), ap.class_name()) && 
                    !strcmp(ptnr_shape_name(), ap.self_shape_name())) {
                if (pprior)
                    *pprior = p;
                return (cd2);
            }
        }
    }
    return (0);
}


// Find the shape in the master cell, and save the transformed BB in
// this.  Return true if the update was successful.
//
bool
sAbutPrior::updatePin()
{
    if (!ap_self || !ap_self_shape)
        return (false);
    CDs *msd = ap_self->masterCell();
    if (!msd)
        return (false);

    CDg gdesc;
    gdesc.init_gen(msd, ap_ldesc);
    CDo *od;
    while ((od = gdesc.next()) != 0) {
        for (CDp *p = od->prpty(XICP_AB_SHAPENAME); p; p = p->next_n()) {
            if (p && p->string() && !strcmp(p->string(), ap_self_shape)) {
                BBox BB(od->oBB());
                cTfmStack stk;
                stk.TPush();
                stk.TApplyTransform(ap_self);
                stk.TBB(&BB, 0);
                stk.TPop();
                ap_pinBB = BB;
                return (true);
            }
        }
    }
    return (false);
}


// Find the matching cdesc and revert the corresponding abutment.
//
bool
sAbutPrior::revertPartnerAbutment()
{
    CDp *pprior;
    CDc *cptnr = findPartner(&pprior);
    if (!cptnr) {
        Errs()->add_error(
            "revertPartnerAbutment: failed to find abutment partner.");
        return (false);
    }
    if (!pprior) {
        Errs()->add_error(
        "revertPartnerAbutment: failed to find abutment reversion property.");
        return (false);
    }

    CDp *p_prms = cptnr->prpty(XICP_PC_PARAMS);
    if (!p_prms) {
        Errs()->add_error(
            "revertPartnerAbutment: missing parameters property.");
        return (false);
    }

    PCellParam *prms;
    if (!PCellParam::parseParams(p_prms->string(), &prms)) {
        Errs()->add_error("revertPartnerAbutment: parameters parse failed.");
        return (false);
    }

    sAbutPrior prtap(cptnr);
    if (!prtap.parse(pprior->string())) {
        Errs()->add_error(
            "revertPartnerAbutment: revert property string parse failed.");
        PCellParam::destroy(prms);
        return (false);
    }
    for (const sAbutKeyVal *p = prtap.params(); p; p = p->nextc())
        prms->setValue(p->key(), p->value());
    CDc *cnew;
    if (!cAbutHandler::setNewParams(cptnr, prms, &cnew)) {
        Errs()->add_error(
            "revertPartnerAbutment: failed to revert cell.");
        PCellParam::destroy(prms);
        return (false);
    }
    PCellParam::destroy(prms);

    // Find and rid the PRIOR property in the new instance.
    CDp *pp = 0, *pn;
    for (CDp *p = cnew->prpty_list(); p; p = pn) {
        pn = p->next_prp();
        if (p->value() == XICP_AB_PRIOR) {
            sAbutPrior ap(cnew);
            if (!ap.parse(p->string()))
                continue;
            if (id_number() == ap.id_number() &&
                    !strcmp(class_name(), ap.class_name()) && 
                    !strcmp(ptnr_shape_name(), ap.self_shape_name())) {
                if (pp)
                    pp->set_next_prp(pn);
                else
                    cnew->set_prpty_list(pn);
                delete p;
                return (true);
            }
            pp = p;
        }
    }
    Errs()->add_error(
        "revertPartnerAbutment: failed to clear property.");
    return (false);
}


// Static function.
// Return a (hopefully) unique id for the id field.
//
unsigned int
sAbutPrior::newId(const CDc *cd1, const CDc *cd2)
{
    unsigned int id = time(0);
    id *= ((unsigned long)cd1) >> 8;
    id *= ((unsigned long)cd2) >> 8;
    return (id);
}
// End of sAbutPrior functions.


bool
sAbutPinInfo::setup(const CDo *odesc)
{
    if (!odesc)
        return (false);

    CDp *p = odesc->prpty(XICP_AB_CLASS);
    if (!p || !p->string())
        return (false);
    pi_class_name = lstring::copy(p->string());

    p = odesc->prpty(XICP_AB_SHAPENAME);
    if (!p || !p->string())
        return (false);
    pi_shape_name = lstring::copy(p->string());

    p = odesc->prpty(XICP_AB_DIRECTS);
    if (!p || !p->string())
        return (false);
    pi_directions = lstring::copy(p->string());

    p = odesc->prpty(XICP_AB_PINSIZE);
    if (!p || !p->string())
        return (false);
    if (sscanf(p->string(), "%lf", &pi_pin_size) != 1)
        return (false);
    return (true);
}


// Static function.
// Return true if the directions are compatible.
//
bool
sAbutPinInfo::checkDirections(const cTfmStack &t1, const cTfmStack &t2,
    const char *dir1, const char *dir2)
{
    const char *s = dir1;
    char *tok;
    while ((tok = lstring::gettok(&s, ",")) != 0) {
        Point_c p1(0, 0);
        Point_c p2(0, 0);
        if (!strcasecmp(tok, "left"))
            p2.x = -1;
        else if (!strcasecmp(tok, "bottom"))
            p2.y = -1;
        else if (!strcasecmp(tok, "right"))
            p2.x = 1;
        else if (!strcasecmp(tok, "top"))
            p2.y = 1;
        t1.TPoint(&p1.x, &p1.y);
        t1.TPoint(&p2.x, &p2.y);
        p2.x -= p1.x;
        p2.y -= p1.y;

        delete [] tok;

        const char *t = dir2;
        while ((tok = lstring::gettok(&t, ",")) != 0) {
            Point_c p3(0, 0);
            Point_c p4(0, 0);
            if (!strcasecmp(tok, "left"))
                p4.x = -1;
            else if (!strcasecmp(tok, "bottom"))
                p4.y = -1;
            else if (!strcasecmp(tok, "right"))
                p4.x = 1;
            else if (!strcasecmp(tok, "top"))
                p4.y = 1;
            t2.TPoint(&p3.x, &p3.y);
            t2.TPoint(&p4.x, &p4.y);
            p4.x -= p3.x;
            p4.y -= p3.y;
            delete [] tok;

            if (p2.x*p4.x + p2.y*p4.y == -1)
                return (true);
        }
    }
    return (false);
}
// End of sAbutPinInfo functions.

