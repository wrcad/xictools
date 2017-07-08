
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
 $Id: cd_compare.cc,v 5.89 2014/11/17 05:17:36 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "cd.h"
#include "cd_types.h"
#include "cd_compare.h"
#include "cd_structdb.h"
#include "cd_lgen.h"
#include "cd_hypertext.h"
#include "geo_zlist.h"
#include "geo_grid.h"
#include "hashfunc.h"


//
// Functions to implement cell-to-cell comparison.
//

namespace {

    // If the object is a wire or poly that looks like a rectangle,
    // convert it to a rectangle (using otmp) for comparison.
    //
    CDo *tobox(CDo *od, CDo *otmp)
    {
        if (od->type() == CDWIRE) {
            CDw *wd = (CDw*)od;
            if (wd->numpts() == 2 &&
                    (wd->wire_style() == CDWIRE_FLUSH ||
                    wd->wire_style() == CDWIRE_EXTEND) &&
                    (wd->points()[0].x == wd->points()[1].x ||
                    wd->points()[0].y == wd->points()[1].y)) {
                otmp->set_ldesc(od->ldesc());
                otmp->set_oBB(od->oBB());
                return (otmp);
            }
        }
        else if (od->type() == CDPOLYGON) {
            CDpo *pd = (CDpo*)od;
            if (pd->numpts() == 5) {
                const Point *pts = pd->points();
                if ((pts[0].x == pts[1].x && pts[1].y == pts[2].y &&
                        pts[2].x == pts[3].x && pts[3].y == pts[0].y) ||
                        (pts[0].y == pts[1].y && pts[1].x == pts[2].x &&
                        pts[2].y == pts[3].y && pts[3].x == pts[0].x)) {
                    otmp->set_ldesc(od->ldesc());
                    otmp->set_oBB(od->oBB());
                    return (otmp);
                }
            }
        }
        return (od);
    }
}


// Set up property filtering.
//
// Properties are only considered if the corresponding object is being
// compared, for otherwise identical objects, and if the DiffPrpxxx
// flag is set.
//
// If properties are being compared, a null filter indicates no filtering.
//
// The PrpFltMode controls use of filtering:
//   PrpFltNone
//       No filtering, all properties found are considered.
//   PrpFltDflt
//       Default filtering for mode, as per this table:
//
//               cell         inst         obj
//       phys    no filt      no filt      no filt
//       elec    def1         def2         no check
//
//       def1 and def2 are internal defaults.
//   PrpFltCstm
//       Use custom filtering.  There are six variables which can
//       contain strings defining filtering strings:
//
//       VA_PhysPrpFltCell
//       VA_PhysPrpFltInst
//       VA_PhysPrpFltObj
//       VA_ElecPrpFltCell
//       VA_ElecPrpFltInst
//       VA_ElecPrpFltObj
//
//       Strings are space or comma-separated lists of property numbers
//       or electrical names.  If the first token of the string is "s"
//       (but not "symblc") then the listed properties are skipped rather
//       than being tested exclusively.  If a variable is not set, then
//       the default action (as above) will be done.  Strings are not
//       case-sensitive.
//

// Default filtering for electrical instances;
// MODEL, VALUE, PARAM, NOPHYS
#define DEF_ELEC_INST_FLT "1,2,3,5"

// Default filtering for electrical cells.
// PARAM, VIRTUAL, NEWMUT, SYMBLC, NODMAP
#define DEF_ELEC_CELL_FLT "3,6,14,18,19"

// Default filtering for electrical objects.
// Don't check any properties.
#define DEF_ELEC_OBJ_FLT ""


// Set up filtering.  The display mode passed here should correspond
// to the actual display mode of the cell descs passed to
// CDdiff::diff.
//
void
CDdiff::setup_filtering(DisplayMode m, PrpFltMode f)
{
    if (f == PrpFltNone) {
        // Null filters -> no filtering (compare all properties).
        set_cell_prp_filter(0);
        set_inst_prp_filter(0);
        set_obj_prp_filter(0);
    }
    else if (f == PrpFltDflt) {
        if (m == Physical) {
            set_cell_prp_filter(0);
            set_inst_prp_filter(0);
            set_obj_prp_filter(0);
        }
        else {
            set_cell_prp_filter(DEF_ELEC_CELL_FLT);
            set_inst_prp_filter(DEF_ELEC_INST_FLT);
            set_obj_prp_filter(DEF_ELEC_OBJ_FLT);
        }
    }
    else if (f == PrpFltCstm) {
        if (m == Physical) {
            set_cell_prp_filter(CDvdb()->getVariable(VA_PhysPrpFltCell));
            set_inst_prp_filter(CDvdb()->getVariable(VA_PhysPrpFltInst));
            set_obj_prp_filter(CDvdb()->getVariable(VA_PhysPrpFltObj));
        }
        else {
            const char *s = CDvdb()->getVariable(VA_ElecPrpFltCell);
            set_cell_prp_filter(s ? s : DEF_ELEC_CELL_FLT);
            s = CDvdb()->getVariable(VA_ElecPrpFltInst);
            set_inst_prp_filter(s ? s : DEF_ELEC_INST_FLT);
            s = CDvdb()->getVariable(VA_ElecPrpFltObj);
            set_obj_prp_filter(s ? s : DEF_ELEC_OBJ_FLT);
        }
    }
}


// Function to diff the cell content to that of another cell, returns
// differences in *pret.
//
DFtype
CDdiff::diff(const CDs *s1, const CDs *s2, Sdiff **pret)
{
    if (!pret) {
        Errs()->add_error("diff: return pointer is null");
        return (DFerror);
    }
    *pret = 0;

    if (s1 == s2)
        return (DFsame);
    if (!s1) {
        if (!s2)
            return (DFnoLR);
        return (DFnoL);
    }
    if (!s2)
        return (DFnoR);

    if (s1->displayMode() != s2->displayMode()) {
        Errs()->add_error(
            "diff: comparison cells are different phys/elec mode");
        return (DFerror);
    }
    DisplayMode dmode = s1->displayMode();
    cdf_electrical = (dmode == Electrical);

    if (!cdf_obj_types)
        set_obj_types((cdf_flags & DiffGeometric) ? "bpw" : "cbpw");

    Ldiff *lf0 = 0, *lfe = 0;
    if (cdf_layer_list && !(cdf_flags & DiffSkipLayers)) {
        char *ltok;
        const char *t = cdf_layer_list;
        while ((ltok = lstring::gettok(&t)) != 0) {
            CDl *ldesc = CDldb()->findLayer(ltok, dmode);
            delete [] ltok;
            if (!ldesc)
                continue;
            Ldiff *lf;
            switch (diff_layer(ldesc, s1, s2, &lf)) {
            case DFerror:
                Errs()->add_error("diff: diff_layer failed");
                Ldiff::destroy(lf0);
                return (DFerror);
            case DFabort:
                Errs()->add_error("diff: diff_layer aborted");
                Ldiff::destroy(lf0);
                return (DFabort);
            default:
                break;
            }
            if (lf) {
                if (lf0) {
                    lfe->set_next_ldiff(lf);
                    lfe = lf;
                }
                else
                    lf0 = lfe = lf;
            }
            if (max_diffs())
                break;
        }
    }
    else {
        SymTab *tab = new SymTab(true, false);
        if (cdf_layer_list) {
            char *ltok;
            const char *t = cdf_layer_list;
            while ((ltok = lstring::gettok(&t)) != 0)
                tab->add(ltok, 0, true);
        }

        CDlgen gen(dmode, CDlgen::BotToTopWithCells);
        CDl *ldesc;
        while ((ldesc = gen.next()) != 0) {
            if (SymTab::get(tab, ldesc->name()) != ST_NIL)
                continue;
            Ldiff *lf;
            switch (diff_layer(ldesc, s1, s2, &lf)) {
            case DFerror:
                Errs()->add_error("diff: diff_layer failed");
                Ldiff::destroy(lf0);
                return (DFerror);
            case DFabort:
                Errs()->add_error("diff: diff_layer aborted");
                Ldiff::destroy(lf0);
                return (DFabort);
            default:
                break;
            }
            if (lf) {
                if (lf0) {
                    lfe->set_next_ldiff(lf);
                    lfe = lf;
                }
                else
                    lf0 = lfe = lf;
            }
            if (max_diffs())
                break;
        }
        delete tab;
    }

    Pdiff *pd0 = 0;
    if ((cdf_flags & DiffPrpCell) && !max_diffs() &&
            !prpfilt_t::filter_all(cdf_filt_cell)) {
        pd0 = diff_cell_props(s1, s2);
        if (pd0)
            cdf_diffcnt++;
    }

    if (lf0 || pd0) {
        *pret = new Sdiff(lf0, pd0);
        return (DFdiffer);
    }
    return (DFsame);
}


// Function to diff the cell content to that of another cell, returns
// only whether or not cells differ.
//
DFtype
CDdiff::diff(const CDs *s1, const CDs *s2)
{
    if (s1 == s2)
        return (DFsame);
    if (!s1 || !s2)
        return (DFdiffer);
    if (s1->displayMode() != s2->displayMode())
        return (DFdiffer);
    DisplayMode dmode = s1->displayMode();
    cdf_electrical = (dmode == Electrical);

    if (!cdf_obj_types)
        set_obj_types((cdf_flags & DiffGeometric) ? "bpw" : "cbpw");

    if (cdf_layer_list && !(cdf_flags & DiffSkipLayers)) {
        char *ltok;
        const char *t = cdf_layer_list;
        while ((ltok = lstring::gettok(&t)) != 0) {
            CDl *ldesc = CDldb()->findLayer(ltok, dmode);
            delete [] ltok;
            if (!ldesc)
                continue;
            DFtype dft = diff_layer(ldesc, s1, s2);
            if (dft != DFsame)
                return (dft);
        }
    }
    else {
        SymTab *tab = new SymTab(true, false);
        if (cdf_layer_list) {
            char *ltok;
            const char *t = cdf_layer_list;
            while ((ltok = lstring::gettok(&t)) != 0)
                tab->add(ltok, 0, true);
        }

        CDlgen gen(dmode, CDlgen::BotToTopWithCells);
        CDl *ldesc;
        while ((ldesc = gen.next()) != 0) {
            if (SymTab::get(tab, ldesc->name()) != ST_NIL)
                continue;
            DFtype dft = diff_layer(ldesc, s1, s2);
            if (dft != DFsame) {
                delete tab;
                return (dft);
            }
        }
        delete tab;
    }
    return (DFsame);
}


namespace {
    inline CDo *
    next_elem(RTgen &gen, const char *obj_types)
    {
        CDo *od;
        while ((od = (CDo*)gen.next_element_nchk()) != 0) {
            if (!obj_types || strchr(obj_types, od->type()))
                return (od);
        }
        return (0);
    }

    inline const char *
    morediffs()
    {
        return ("    more differences found");
    }

    // Return true if checking properties of object of this type.
    //
    inline bool
    obj_pcheck(unsigned int flags, int type)
    {
        if (type == CDBOX)
            return (flags & DiffPrpBox);
        if (type == CDPOLYGON)
            return (flags & DiffPrpPoly);
        if (type == CDWIRE)
            return (flags & DiffPrpWire);
        if (type == CDLABEL)
            return (flags & DiffPrpLabl);
        return (false);
    }
}


// Function to diff the s1, s2 cell content on ldesc, returns
// differences in *lret.
//
DFtype
CDdiff::diff_layer(CDl *ldesc, const CDs *s1, const CDs *s2, Ldiff **lret)
{
    if (!lret) {
        Errs()->add_error("diff_layer: null return pointer");
        return (DFerror);
    }
    *lret = 0;

    if (!ldesc) {
        Errs()->add_error("diff_layer: null layer descriptor");
        return (DFerror);
    }
    if (ldesc == CellLayer()) {
        // Compare the instance layer specially.  Unless the entire
        // hierarchy has been read into the database, the instances
        // will not be in the right bin, and won't have the BB set.

        if (!cdf_obj_types || strchr(cdf_obj_types, 'c'))
            return (diff_instances(s1, s2, lret));
        return (DFsame);
    }

    const char *otypes = (cdf_flags & DiffGeometric) ? "bpw" : cdf_obj_types;

    RTree *l1 = s1 ? s1->db_find_layer_head(ldesc) : 0;
    RTree *l2 = s2 ? s2->db_find_layer_head(ldesc) : 0;
    Ldiff *ld0 = 0;
    Pdiff *pd0 = 0, *pde = 0;
    const unsigned int obj_pflags =
        DiffPrpBox | DiffPrpPoly | DiffPrpWire | DiffPrpLabl;

    // The code below assumes that the objects returned from the
    // generators are in predictable order.

    if (!l2 && l1) {
        RTgen gen;
        gen.init(l1, &CDinfiniteBB);
        CDo *od;
        while ((od = next_elem(gen, otypes)) != 0) {
            if (max_diffs()) {
                ld0 = Ldiff::save(ld0, ldesc, morediffs(), 0);
                break;
            }
            char *str = od->cif_string(0, 0, true);
            ld0 = Ldiff::save(ld0, ldesc, str, 0);
            delete [] str;
            cdf_diffcnt++;
        }
    }
    else if (!l1 && l2) {
        RTgen gen;
        gen.init(l2, &CDinfiniteBB);
        CDo *od;
        while ((od = next_elem(gen, otypes)) != 0) {
            if (max_diffs()) {
                ld0 = Ldiff::save(ld0, ldesc, 0, morediffs());
                break;
            }
            char *str = od->cif_string(0, 0, true);
            ld0 = Ldiff::save(ld0, ldesc, 0, str);
            delete [] str;
            cdf_diffcnt++;
        }
    }
    else if (l1 && l2) {
        if (cdf_flags & DiffGeometric) {

            BBox cBB = *s1->BB();
            cBB.add(s2->BB());

            grd_t grd(&cBB, grd_t::def_gridsize());
            const BBox *gBB;
            int gcnt = 0;
            while ((gBB = grd.advance()) != 0) {

                if (gcnt)
                    Sdiff::ufb_check();
                gcnt++;

                Zlist zr(gBB);

                XIrt ret;
                Zlist *z1 = s1->getZlist(0, ldesc, &zr, &ret);
                if (ret == XIintr) {
                    Errs()->add_error("diffLayer: user interrupt");
                    delete ld0;
                    return (DFabort);
                }
                else if (ret == XIbad) {
                    Errs()->add_error("diffLayer: getZlist failed");
                    delete ld0;
                    return (DFerror);
                }
                Zlist *z2 = s2->getZlist(0, ldesc, &zr, &ret);
                if (ret == XIintr) {
                    Errs()->add_error("diffLayer: user interrupt");
                    delete ld0;
                    Zlist::destroy(z1);
                    return (DFabort);
                }
                else if (ret == XIbad) {
                    Errs()->add_error("diffLayer: getZlist failed");
                    delete ld0;
                    Zlist::destroy(z1);
                    return (DFerror);
                }

                ret = Zlist::zl_andnot2(&z1, &z2);
                if (ret == XIintr) {
                    Errs()->add_error("diffLayer: user interrupt");
                    delete ld0;
                    return (DFabort);
                }
                else if (ret == XIbad) {
                    Errs()->add_error("diffLayer: zl_andnot2 failed");
                    delete ld0;
                    return (DFerror);
                }

                bool skip = false;
                CDo *o1 = Zlist::to_obj_list(z1, ldesc), *on;
                for (CDo *o = o1; o; o = on) {
                    on = o->next_odesc();
                    if (!skip) {
                        if (max_diffs()) {
                            ld0 = Ldiff::save(ld0, ldesc, morediffs(), 0);
                            skip = true;
                        }
                        else {
                            char *str = o->cif_string(0, 0);
                            ld0 = Ldiff::save(ld0, ldesc, str, 0);
                            delete [] str;
                        }
                        cdf_diffcnt++;
                    }
                    delete o;
                }
                if (skip)
                    break;

                CDo *o2 = Zlist::to_obj_list(z2, ldesc);
                for (CDo *o = o2; o; o = on) {
                    on = o->next_odesc();
                    if (!skip) {
                        if (max_diffs()) {
                            ld0 = Ldiff::save(ld0, ldesc, 0, morediffs());
                            skip = true;
                        }
                        else {
                            char *str = o->cif_string(0, 0);
                            ld0 = Ldiff::save(ld0, ldesc, 0, str);
                            delete [] str;
                        }
                        cdf_diffcnt++;
                    }
                    delete o;
                }
                if (skip)
                    break;
            }
        }
        else {
            RTgen gen1, gen2;
            gen1.init(l1, &CDinfiniteBB);
            gen2.init(l2, &CDinfiniteBB);

            CDo *od1 = next_elem(gen1, otypes);
            CDo *od2 = next_elem(gen2, otypes);

            bool sloppy_boxes = (cdf_flags & DiffSloppyBoxes);
            bool ignore_dups = (cdf_flags & DiffIgnoreDups);
            CDo otmp1(0, 0);
            CDo otmp2(0, 0);
            while (od1 && od2) {
                if (od1->op_lt(od2)) {
                    while (od1 && od1->op_lt(od2))
                        od1 = next_elem(gen1, otypes);
                    continue;
                }
                if (od2->op_lt(od1)) {
                    while (od2 && od2->op_lt(od1))
                        od2 = next_elem(gen2, otypes);
                    continue;
                }
                for (CDo *od = od2; od; od = od->db_next()) {
                    if (!ignore_dups && od->has_flag(CDoMark1))
                        continue;
                    if (od->op_gt(od1))
                        break;
                    if (sloppy_boxes) {
                        if (*tobox(od, &otmp1) != *tobox(od1, &otmp2))
                            continue;
                    }
                    else {
                        if (*od != *od1)
                            continue;
                    }
                    od->set_flag(CDoMark1);
                    od1->set_flag(CDoMark1);
                    if (!ignore_dups)
                        break;
                }
                od1 = next_elem(gen1, otypes);
            }

            bool skip = false;
            gen1.init(l1, &CDinfiniteBB);
            while ((od1 = next_elem(gen1, otypes)) != 0) {
                if (od1->has_flag(CDoMark1)) {
                    od1->unset_flag(CDoMark1);
                    continue;
                }
                if (!skip) {
                    if (max_diffs()) {
                        ld0 = Ldiff::save(ld0, ldesc, morediffs(), 0);
                        skip = true;
                    }
                    else {
                        char *str = od1->cif_string(0, 0, true);
                        ld0 = Ldiff::save(ld0, ldesc, str, 0);
                        delete [] str;
                    }
                    cdf_diffcnt++;
                }
            }
            gen2.init(l2, &CDinfiniteBB);
            while ((od2 = next_elem(gen2, otypes)) != 0) {
                if (od2->has_flag(CDoMark1)) {
                    od2->unset_flag(CDoMark1);
                    continue;
                }
                if (!skip) {
                    if (max_diffs()) {
                        ld0 = Ldiff::save(ld0, ldesc, 0, morediffs());
                        skip = true;
                    }
                    else {
                        char *str = od2->cif_string(0, 0, true);
                        ld0 = Ldiff::save(ld0, ldesc, 0, str);
                        delete [] str;
                    }
                    cdf_diffcnt++;
                }
            }
            // CDoMark1 flags are now cleared.

            if ((cdf_flags & obj_pflags) && !max_diffs() &&
                    !prpfilt_t::filter_all(cdf_filt_obj)) {
                gen1.init(l1, &CDinfiniteBB);
                gen2.init(l2, &CDinfiniteBB);
                od1 = next_elem(gen1, otypes);
                od2 = next_elem(gen2, otypes);
                while (od1 && od2 && !skip) {
                    if (od1->op_lt(od2)) {
                        while (od1 && od1->op_lt(od2))
                            od1 = next_elem(gen1, otypes);
                        continue;
                    }
                    if (od2->op_lt(od1)) {
                        while (od2 && od2->op_lt(od1))
                            od2 = next_elem(gen2, otypes);
                        continue;
                    }
                    for (CDo *od = od2; od; od = od->db_next()) {
                        if (od->has_flag(CDoMark1))
                            continue;
                        if (od->op_gt(od1))
                            break;
                        if (*od != *od1)
                            continue;
                        od->set_flag(CDoMark1);
                        od1->set_flag(CDoMark1);
                        if (obj_pcheck(cdf_flags, od1->type())) {
                            Pdiff *p = diff_obj_props(od1, od);
                            if (p) {
                                if (pde) {
                                    pde->set_next_pdiff(p);
                                    pde = pde->next_pdiff();
                                }
                                else
                                    pd0 = pde = p;
                                if (max_diffs()) {
                                    p->reset(morediffs());
                                    skip = true;
                                    break;
                                }
                                cdf_diffcnt++;
                            }
                        }
                        break;
                    }
                    od1 = next_elem(gen1, otypes);
                }
                gen1.init(l1, &CDinfiniteBB);
                while ((od1 = next_elem(gen1, otypes)) != 0) {
                    if (od1->has_flag(CDoMark1))
                        od1->unset_flag(CDoMark1);
                }
                gen2.init(l2, &CDinfiniteBB);
                while ((od2 = next_elem(gen2, otypes)) != 0) {
                    if (od2->has_flag(CDoMark1))
                        od2->unset_flag(CDoMark1);
                }
                // CDoMark1 flags are now cleared.
            }
        }
    }

    if (pd0) {
        if (!ld0)
            ld0 = new Ldiff(ldesc);
        ld0->set_pdiffs(pd0);
    }
    *lret = ld0;
    return (ld0 ? DFdiffer : DFsame);
}


// Function to diff the s1, s2 cell content on ldesc, returns only
// whether or not cells differ.
//
DFtype
CDdiff::diff_layer(CDl *ldesc, const CDs *s1, const CDs *s2)
{
    if (!ldesc) {
        Errs()->add_error("diffLayer: null layer descriptor");
        return (DFerror);
    }
    if (ldesc == CellLayer()) {
        // Compare the instance layer specially.  Unless the entire
        // hierarchy has been read into the database, the instances
        // will not be in the right bin, and won't have the BB set.

        if (!cdf_obj_types || strchr(cdf_obj_types, 'c'))
            return (diff_instances(s1, s2));
        return (DFsame);
    }

    const char *otypes = (cdf_flags & DiffGeometric) ? "bpw" : cdf_obj_types;

    RTree *l1 = s1 ? s1->db_find_layer_head(ldesc) : 0;
    RTree *l2 = s2 ? s2->db_find_layer_head(ldesc) : 0;

    if (!l1 && l2) {
        RTgen gen;
        gen.init(l2, &CDinfiniteBB);
        if (next_elem(gen, otypes) != 0)
            return (DFdiffer);
    }
    else if (!l2 && l1) {
        RTgen gen;
        gen.init(l1, &CDinfiniteBB);
        if (next_elem(gen, otypes) != 0)
            return (DFdiffer);
    }
    else if (l1 && l2) {
        if (cdf_flags & DiffGeometric) {
            RTgen gen1, gen2;
            gen1.init(l1, &CDinfiniteBB);
            gen2.init(l2, &CDinfiniteBB);

            BBox cBB = *s1->BB();
            cBB.add(s2->BB());

            grd_t grd(&cBB, grd_t::def_gridsize());
            const BBox *gBB;
            while ((gBB = grd.advance()) != 0) {
                Zlist zr(gBB);

                XIrt ret;
                Zlist *z1 = s1->getZlist(0, ldesc, &zr, &ret);
                if (ret != XIok)
                    return (DFerror);
                Zlist *z2 = s2->getZlist(0, ldesc, &zr, &ret);
                if (ret != XIok) {
                    Zlist::destroy(z1);
                    return (DFerror);
                }
                ret = Zlist::zl_andnot2(&z1, &z2);
                if (ret != XIok)
                    return (DFerror);

                bool df = (z1 || z2);
                Zlist::destroy(z1);
                Zlist::destroy(z2);
                if (df)
                    return (DFdiffer);
            }
        }
        else {
            if (l1->num_elements() != l2->num_elements())
                return (DFdiffer);

            RTgen gen1, gen2;
            gen1.init(l1, &CDinfiniteBB);
            gen2.init(l2, &CDinfiniteBB);

            for (;;) {
                CDo *od1 = (CDo*)gen1.next_element_nchk();
                CDo *od2 = (CDo*)gen2.next_element_nchk();
                if (!od1 || !od2) {
                    if (od1 || od2)
                        return (DFdiffer);
                    break;
                }
                if (*od1 != *od2)
                    return (DFdiffer);
            }
        }
    }
    return (DFsame);
}


//
// Helper classes for comparing instance placements.
//

namespace {
    struct cinst_t
    {
        cinst_t(const CDc *c = 0) { c_cdesc = c; }

        bool operator==(cinst_t&);
        unsigned int hash();
        char *string(const char *name);

        const CDc *cdesc()      { return (c_cdesc); }

    private:
        const CDc *c_cdesc;
    };


    // This variation does not include array parameters.  This will be
    // instantiated once for each array element and singleton.
    //
    struct celt_t : public CDtx
    {
        bool operator==(celt_t&);
        unsigned int hash();
        char *string(const char *name);
    };


    // Container for table and add function.
    //
    template <class T>
    struct ccmp_t
    {
        ccmp_t() { cc_table = 0; }
        ~ccmp_t() { delete cc_table; }

        unsigned int allocated()
            {
                if (cc_table)
                    return (cc_table->end_ticket());
                return (0);
            }

        T *find(ticket_t i)
            {
                if (cc_table)
                    return (cc_table->find(i));
                return (0);
            }

        ticket_t check(T *c)
            {
                if (cc_table)
                    return (cc_table->check(c));
                return (NULL_TICKET);
            }

        void add(CDc*);

    private:
        sDBase<T> *cc_table;
    };


    // Needed specialization.
    //
    template <>
    void
    ccmp_t<cinst_t>::add(CDc *c)
    {
        cinst_t cinst(c);
        if (!cc_table)
            cc_table = new sDBase<cinst_t>;
        cc_table->record(&cinst);
    }


    // Needed specialization.
    //
    template <>
    void
    ccmp_t<celt_t>::add(CDc *c)
    {
        CDap ap;
        c->get_ap(&ap);
        CDtx tx;
        c->get_tx(&tx);

        cTfmStack stk;
        stk.TPush();
        stk.TApplyTransform(c);
        for (unsigned int i = 0; i < ap.ny; i++) {
            for (unsigned int j = 0; j < ap.nx; j++) {
                int x, y;
                stk.TGetTrans(&x, &y);
                stk.TTransMult(j*ap.dx, i*ap.dy);

                celt_t celt;
                stk.TCurrent(&celt);
                stk.TSetTrans(x, y);

                if (!cc_table)
                    cc_table = new sDBase<celt_t>;
                cc_table->record(&celt);
            }
        }
        stk.TPop();
    }
}
// End of ccmp_t functions.


// Needed specialization.
//
template<>
unsigned int
sDBase<cinst_t>::hash(cinst_t *c)
{
    return (c->hash() & DBhashmask);
}


// Needed specialization.
//
template<>
unsigned int
sDBase<celt_t>::hash(celt_t *c)
{
    return (c->hash() & DBhashmask);
}


// Function to diff the instance list of another cell, returns
// differences in *lret.
//
DFtype
CDdiff::diff_instances(const CDs *s1, const CDs *s2, Ldiff **lret)
{
    if (!lret) {
        Errs()->add_error("diff_instances: null return pointer");
        return (DFerror);
    }
    *lret = 0;

    if ((!s1 || !s1->masters()) && (!s2 || !s2->masters()))
        return (DFsame);

    SymTab *st = new SymTab(false, false);
    if (s1 && s1->masters()) {
        CDm_gen mgen(s1, GEN_MASTERS);
        for (CDm *md = mgen.m_first(); md; md = mgen.m_next())
            st->add(md->cellname()->string(), 0, true);
    }
    if (s2 && s2->masters()) {
        CDm_gen mgen(s2, GEN_MASTERS);
        for (CDm *md = mgen.m_first(); md; md = mgen.m_next())
            st->add(md->cellname()->string(), 0, true);
    }
    CDl *ldesc = CellLayer();

    Ldiff *ld0 = 0;
    Pdiff *pd0 = 0, *pde = 0;

    SymTabGen stgen(st);
    SymTabEnt *hent;
    while ((hent = stgen.next()) != 0) {
        if (max_diffs())
            break;

        // The entry tags are string hashed.
        CDcellName sname = (CDcellName)hent->stTag;
        CDm *m1 = s1 ? s1->findMaster(sname) : 0;
        CDm *m2 = s2 ? s2->findMaster(sname) : 0;

        if (cdf_flags & DiffExpArrays) {
            ccmp_t<celt_t> cc1;
            CDc_gen cgen1(m1);
            for (CDc *c = cgen1.c_first(); c; c = cgen1.c_next())
                cc1.add(c);

            ccmp_t<celt_t> cc2;
            CDc_gen cgen2(m2);
            for (CDc *c = cgen2.c_first(); c; c = cgen2.c_next())
                cc2.add(c);

            char *e1 = 0, *e2 = 0;
            unsigned int n1 = cc1.allocated();
            if (n1) {
                e1 = new char[n1];
                memset(e1, 0, n1);
            }

            unsigned int n2 = cc2.allocated();
            if (n2) {
                e2 = new char[n2];
                memset(e2, 0, n2);
            }

            for (unsigned int i = 0; i < n1; i++) {
                celt_t *c = cc1.find(i);
                if (!c)
                    continue;  // can't happen
                ticket_t t2 = cc2.check(c);
                if (t2 != NULL_TICKET) {
                    e1[i] = 1;
                    e2[t2] = 1;
                }
            }
            for (unsigned int i = 0; i < n1; i++) {
                if (e1[i])
                    continue;
                celt_t *c1 = cc1.find(i);
                if (!c1)
                     continue;  // can't happen

                if (max_diffs()) {
                    ld0 = Ldiff::save(ld0, ldesc, morediffs(), 0);
                    break;
                }
                char *str = c1->string(sname->string());
                ld0 = Ldiff::save(ld0, ldesc, str, 0);
                delete [] str;
                cdf_diffcnt++;
            }
            if (!max_diffs()) {
                for (unsigned int i = 0; i < n2; i++) {
                    if (e2[i])
                        continue;
                    celt_t *c2 = cc2.find(i);
                    if (!c2)
                        continue;  // can't happen

                    if (max_diffs()) {
                        ld0 = Ldiff::save(ld0, ldesc, 0, morediffs());
                        break;
                    }
                    char *str = c2->string(sname->string());
                    ld0 = Ldiff::save(ld0, ldesc, 0, str);
                    delete [] str;
                    cdf_diffcnt++;
                }
            }
            delete [] e1;
            delete [] e2;
        }
        else {
            ccmp_t<cinst_t> cc1;
            CDc_gen cgen1(m1);
            for (CDc *c = cgen1.c_first(); c; c = cgen1.c_next())
                cc1.add(c);

            ccmp_t<cinst_t> cc2;
            CDc_gen cgen2(m2);
            for (CDc *c = cgen2.c_first(); c; c = cgen2.c_next())
                cc2.add(c);

            char *e1 = 0, *e2 = 0;
            unsigned int n1 = cc1.allocated();
            if (n1) {
                e1 = new char[n1];
                memset(e1, 0, n1);
            }

            unsigned int n2 = cc2.allocated();
            if (n2) {
                e2 = new char[n2];
                memset(e2, 0, n2);
            }

            for (unsigned int i = 0; i < n1; i++) {
                cinst_t *c = cc1.find(i);
                if (!c)
                    continue;  // can't happen
                ticket_t t2 = cc2.check(c);
                if (t2 != NULL_TICKET) {
                    e1[i] = 1;
                    e2[t2] = 1;
                }
            }
            for (unsigned int i = 0; i < n1; i++) {
                if (e1[i])
                    continue;
                cinst_t *c1 = cc1.find(i);
                if (!c1)
                     continue;  // can't happen

                if (max_diffs()) {
                    ld0 = Ldiff::save(ld0, ldesc, morediffs(), 0);
                    break;
                }
                char *str = c1->string(sname->string());
                ld0 = Ldiff::save(ld0, ldesc, str, 0);
                delete [] str;
                cdf_diffcnt++;
            }
            if (!max_diffs()) {
                for (unsigned int i = 0; i < n2; i++) {
                    if (e2[i])
                        continue;
                    cinst_t *c2 = cc2.find(i);
                    if (!c2)
                        continue;  // can't happen

                    if (max_diffs()) {
                        ld0 = Ldiff::save(ld0, ldesc, 0, morediffs());
                        break;
                    }
                    char *str = c2->string(sname->string());
                    ld0 = Ldiff::save(ld0, ldesc, 0, str);
                    delete [] str;
                    cdf_diffcnt++;
                }
            }
            if ((cdf_flags & DiffPrpInst) && !max_diffs() &&
                    !prpfilt_t::filter_all(cdf_filt_inst)) {
                for (unsigned int i = 0; i < n1; i++) {
                    if (!e1[i])
                        continue;
                    cinst_t *c1 = cc1.find(i);
                    if (!c1)
                        continue;  // can't happen
                    ticket_t t2 = cc2.check(c1);
                    if (t2 == NULL_TICKET)
                        continue;
                    cinst_t *c2 = cc2.find(t2);
                    if (!c2)
                        continue;  // can't happen

                    Pdiff *p = diff_obj_props(c1->cdesc(), c2->cdesc());
                    if (p) {
                        if (pde) {
                            pde->set_next_pdiff(p);
                            pde = pde->next_pdiff();
                        }
                        else
                            pd0 = pde = p;
                        if (max_diffs()) {
                            p->reset(morediffs());
                            break;
                        }
                        cdf_diffcnt++;
                    }
                }
            }
            delete [] e1;
            delete [] e2;
        }
    }
    delete st;

    if (pd0) {
        if (!ld0)
            ld0 = new Ldiff(ldesc);
        ld0->set_pdiffs(pd0);
    }
    *lret = ld0;
    return (ld0 ? DFdiffer : DFsame);
}


// Function to diff the instance list of another cell, returns only
// whether or not the lists differ.
//
DFtype
CDdiff::diff_instances(const CDs *s1, const CDs *s2)
{
    if ((!s1 || !s1->masters()) && (!s2 || !s2->masters())) {
        // No instance lists found in either cell, treat this as
        // "no difference".

        return (DFsame);
    }

    // Make a table of unique master cell names, from both cells.
    SymTab *st = new SymTab(false, false);
    if (s1 && s1->masters()) {
        CDm_gen mgen(s1, GEN_MASTERS);
        for (CDm *md = mgen.m_first(); md; md = mgen.m_next())
            st->add(md->cellname()->string(), 0, true);
    }
    if (s2 && s2->masters()) {
        CDm_gen mgen(s2, GEN_MASTERS);
        for (CDm *md = mgen.m_first(); md; md = mgen.m_next())
            st->add(md->cellname()->string(), 0, true);
    }

    SymTabGen stgen(st);
    SymTabEnt *hent;
    while ((hent = stgen.next()) != 0) {
        // The entry tags are string hashed.
        CDcellName sname = (CDcellName)hent->stTag;
        CDm *m1 = s1 ? s1->findMaster(sname) : 0;
        if (!m1) {
            delete st;
            return (DFdiffer);
        }
        CDm *m2 = s2 ? s2->findMaster(sname) : 0;
        if (!m2) {
            delete st;
            return (DFdiffer);
        }

        if (cdf_flags & DiffExpArrays) {
            ccmp_t<celt_t> cc1;
            CDc_gen cgen1(m1);
            for (CDc *c = cgen1.c_first(); c; c = cgen1.c_next())
                cc1.add(c);

            ccmp_t<celt_t> cc2;
            CDc_gen cgen2(m2);
            for (CDc *c = cgen2.c_first(); c; c = cgen2.c_next())
                cc2.add(c);

            char *e1 = 0, *e2 = 0;
            unsigned int n1 = cc1.allocated();
            if (n1) {
                e1 = new char[n1];
                memset(e1, 0, n1);
            }

            unsigned int n2 = cc2.allocated();
            if (n2) {
                e2 = new char[n2];
                memset(e2, 0, n2);
            }

            for (unsigned int i = 0; i < n1; i++) {
                celt_t *c = cc1.find(i);
                if (!c)
                    continue;  // can't happen
                ticket_t t2 = cc2.check(c);
                if (t2 != NULL_TICKET) {
                    e1[i] = 1;
                    e2[t2] = 1;
                }
            }

            for (unsigned int i = 0; i < n1; i++) {
                if (e1[i])
                    continue;
                delete [] e1;
                delete [] e2;
                delete st;
                return (DFdiffer);
            }

            for (unsigned int i = 0; i < n2; i++) {
                if (e2[i])
                    continue;
                delete [] e1;
                delete [] e2;
                delete st;
                return (DFdiffer);
            }
            delete [] e1;
            delete [] e2;
        }
        else {
            ccmp_t<cinst_t> cc1;
            CDc_gen cgen1(m1);
            for (CDc *c = cgen1.c_first(); c; c = cgen1.c_next())
                cc1.add(c);

            ccmp_t<cinst_t> cc2;
            CDc_gen cgen2(m2);
            for (CDc *c = cgen2.c_first(); c; c = cgen2.c_next())
                cc2.add(c);

            char *e1 = 0, *e2 = 0;
            unsigned int n1 = cc1.allocated();
            if (n1) {
                e1 = new char[n1];
                memset(e1, 0, n1);
            }

            unsigned int n2 = cc2.allocated();
            if (n2) {
                e2 = new char[n2];
                memset(e2, 0, n2);
            }

            for (unsigned int i = 0; i < n1; i++) {
                cinst_t *c = cc1.find(i);
                if (!c)
                    continue;  // can't happen
                ticket_t t2 = cc2.check(c);
                if (t2 != NULL_TICKET) {
                    e1[i] = 1;
                    e2[t2] = 1;
                }
            }

            for (unsigned int i = 0; i < n1; i++) {
                if (e1[i])
                    continue;
                delete [] e1;
                delete [] e2;
                delete st;
                return (DFdiffer);
            }

            for (unsigned int i = 0; i < n2; i++) {
                if (e2[i])
                    continue;
                delete [] e1;
                delete [] e2;
                delete st;
                return (DFdiffer);
            }
            delete [] e1;
            delete [] e2;
        }
    }
    delete st;

    // Instance lists are identical.
    return (DFsame);
}


// Report diffs in cell property lists, works in electrical and
// physical mode.
//
Pdiff *
CDdiff::diff_cell_props(const CDs *sd1, const CDs *sd2)
{
    if (!sd1 || !sd2)
        return (0);
    CDp *pl1 = sd1->prptyList();
    CDp *pl2 = sd2->prptyList();
    return (diff_plist("Cell Properties", pl1, pl2, cdf_filt_cell));
}


// Report diffs in object property lists, this works in physical
// and electrical modes, but reports too much spurious junk for
// general use in electrical mode.
//
Pdiff *
CDdiff::diff_obj_props(const CDo *od1, const CDo *od2)
{
    if (!od1 || !od2)
        return (0);
    CDp *pl1 = od1->prpty_list();
    CDp *pl2 = od2->prpty_list();
    char *nstring = od1->cif_string(0, 0, true);
    Pdiff *pd = diff_plist(nstring, pl1, pl2,
        od1->type() == CDINSTANCE ? cdf_filt_inst : cdf_filt_obj);
    delete [] nstring;
    return (pd);
}


Pdiff *
CDdiff::diff_plist(const char *nstring, CDp *pl1, CDp *pl2, prpfilt_t *filt)
{
    Pdiff *pd = 0;

    int n1 = 0;
    for (CDp *p = pl1; p; p = p->next_prp())
        n1++;
    int n2 = 0;
    for (CDp *p = pl2; p; p = p->next_prp())
        n2++;
    if (!n1) {
        if (!n2)
            return (0);
        for (CDp *p = pl2; p; p = p->next_prp()) {
            if (!filt || filt->check(p->value())) {
                sLstr lstr;
                if (cdf_electrical)
                    lstr.add(CDp::elec_prp_name(p->value()));
                else
                    lstr.add_i(p->value());
                lstr.add(":  ");
                p->print(&lstr, 0, 0);
                pd = Pdiff::save(pd, nstring, 0, lstr.string());
            }
        }
    }
    else if (!n2) {
        for (CDp *p = pl1; p; p = p->next_prp()) {
            if (!filt || filt->check(p->value())) {
                sLstr lstr;
                if (cdf_electrical)
                    lstr.add(CDp::elec_prp_name(p->value()));
                else
                    lstr.add_i(p->value());
                lstr.add(":  ");
                p->print(&lstr, 0, 0);
                pd = Pdiff::save(pd, nstring, lstr.string(), 0);
            }
        }
    }
    else {
        char *e1 = new char[n1];
        char *e2 = new char[n2];
        memset(e1, 0, n1);
        memset(e2, 0, n2);
        if (filt) {
            int i = 0;
            for (CDp *p1 = pl1; p1; p1 = p1->next_prp()) {
                if (!filt->check(p1->value()))
                    e1[i] = 1;
                i++;
            }
            i = 0;
            for (CDp *p2 = pl2; p2; p2 = p2->next_prp()) {
                if (!filt->check(p2->value()))
                    e2[i] = 1;
                i++;
            }
        }
        int i1 = 0;
        for (CDp *p1 = pl1; p1; p1 = p1->next_prp()) {
            if (!e1[i1]) {
                sLstr l1;
                l1.add(" ");
                p1->print(&l1, 0, 0);
                int i2 = 0;
                for (CDp *p2 = pl2; p2; p2 = p2->next_prp()) {
                    if (!e2[i2] && p1->value() == p2->value()) {
                        sLstr l2;
                        l2.add(" ");
                        p2->print(&l2, 0, 0);
                        if (!strcmp(l1.string(), l2.string())) {
                            e1[i1] = 1;
                            e2[i2] = 1;
                            break;
                        }
                    }
                    i2++;
                }
            }
            i1++;
        }

        i1 = 0;
        for (CDp *p1 = pl1; p1; p1 = p1->next_prp()) {
            if (!e1[i1]) {
                sLstr lstr;
                if (cdf_electrical)
                    lstr.add(CDp::elec_prp_name(p1->value()));
                else
                    lstr.add_i(p1->value());
                lstr.add(":  ");
                p1->print(&lstr, 0, 0);
                pd = Pdiff::save(pd, nstring, lstr.string(), 0);
            }
            i1++;
        }

        i1 = 0;
        for (CDp *p2 = pl2; p2; p2 = p2->next_prp()) {
            if (!e2[i1]) {
                sLstr lstr;
                if (cdf_electrical)
                    lstr.add(CDp::elec_prp_name(p2->value()));
                else
                    lstr.add_i(p2->value());
                lstr.add(":  ");
                p2->print(&lstr, 0, 0);
                pd = Pdiff::save(pd, nstring, 0, lstr.string());
            }
            i1++;
        }
        delete [] e1;
        delete [] e2;
    }
    return (pd);
}
// End of CDdiff functions.


// Parse a list of comma or space-separated property numbers or names,
// adding to the table.  Unrecognized tokens are ignored.
//
void
prpfilt_t::parse(const char *str)
{
    if (!str)
        return;
    while (isspace(*str) || *str == ',')
        str++;

    // If the first non-delimiter is 's' and the following char is not 'y'
    // (to avoid "symblc"), the skip mode is indicated.  In this case, strip
    // remaining alpha chars to delimiter.
    if ((str[0] == 's' || str[0] == 'S') && str[1] != 'y' && str[1] != 'Y') {
        set_skip(true);
        while (isalpha(*str))
            str++;
    }
    else
        set_skip(false);

    const char *sepc = ",";
    char *tok;
    while ((tok = lstring::gettok(&str, sepc)) != 0) {
        if (isalpha(*tok)) {
            int d;
            if (CDp::elec_prp_num(tok, &d))
                add(d);
        }
        else {
            int d;
            if (sscanf(tok, "%d", &d) == 1)
               add(d);
        }
        delete [] tok;
    }
}
// End of prpfilt_t functions.


Pdiff::~Pdiff()
{
    delete [] pd_name;
    stringlist::destroy(pd_list12);
    stringlist::destroy(pd_list21);
}


void
Pdiff::add(const char *s1, const char *s2)
{
    if (s1)
        pd_list12 = new stringlist(lstring::copy(s1), pd_list12);
    if (s2)
        pd_list21 = new stringlist(lstring::copy(s2), pd_list21);
}


void
Pdiff::reset(const char *n)
{
    delete [] pd_name;
    pd_name = lstring::copy(n);
    stringlist::destroy(pd_list12);
    pd_list12 = 0;
    stringlist::destroy(pd_list21);
    pd_list21 = 0;
}


// Property diff printing, all output is commented to avoid confusing
// geometry parsing of result file.
//
void
Pdiff::print(FILE *fp)
{
    if (!pd_name)
        return;
    fprintf(fp, "# %s\n", pd_name);

    // The lists are in reverse order, reverse before printing.
    
    stringlist *l0 = 0;
    while (pd_list12) {
        stringlist *sx = pd_list12;
        pd_list12 = pd_list12->next;
        sx->next = l0;
        l0 = sx;
    }
    while (l0) {
        fprintf(fp, "# %s %s\n", DIFF_LTOK, l0->string);
        stringlist *sx = l0;
        l0 = l0->next;
        sx->next = pd_list12;
        pd_list12 = sx;
    }
    while (pd_list21) {
        stringlist *sx = pd_list21;
        pd_list21 = pd_list21->next;
        sx->next = l0;
        l0 = sx;
    }
    while (l0) {
        fprintf(fp, "# %s %s\n", DIFF_RTOK, l0->string);
        stringlist *sx = l0;
        l0 = l0->next;
        sx->next = pd_list21;
        pd_list21 = sx;
    }
}


// Static function
Pdiff *
Pdiff::save(Pdiff *pd, const char *n, const char *s1, const char *s2)
{
    if (!pd) {
        if (!n)
            return (0);
        pd = new Pdiff(lstring::copy(n));
    }
    pd->add(s1, s2);
    return (pd);
}
// End of Pdiff functions.


Ldiff::~Ldiff()
{
    stringlist::destroy(ld_list12);
    stringlist::destroy(ld_list21);
    Pdiff::destroy(ld_pdiffs);
}


void
Ldiff::add(const char *s1, const char *s2)
{
    if (s1)
        ld_list12 = new stringlist(lstring::copy(s1), ld_list12);
    if (s2)
        ld_list21 = new stringlist(lstring::copy(s2), ld_list21);
}


void
Ldiff::print(FILE *fp, bool nohdr)
{
    if (!ld_ldesc)
        return;
    const char *lname = ld_ldesc->name();
    if (!lname)
        return;
    bool inst = (ld_ldesc == CellLayer());

    // The lists are in reverse order, reverse before printing.
    
    stringlist *l0 = 0;
    while (ld_list12) {
        stringlist *sx = ld_list12;
        ld_list12 = ld_list12->next;
        sx->next = l0;
        l0 = sx;
    }
    bool phead = nohdr;
    while (l0) {
        if (!phead) {
            if (inst)
                fprintf(fp, "%s %s\n", DIFF_LTOK, DIFF_INSTANCES);
            else
                fprintf(fp, "%s %s %s\n", DIFF_LTOK, DIFF_LAYER, lname);
            phead = true;
        }
        fprintf(fp, "%s;\n", l0->string);
        stringlist *sx = l0;
        l0 = l0->next;
        sx->next = ld_list12;
        ld_list12 = sx;
    }

    while (ld_list21) {
        stringlist *sx = ld_list21;
        ld_list21 = ld_list21->next;
        sx->next = l0;
        l0 = sx;
    }
    phead = nohdr;
    while (l0) {
        if (!phead) {
            if (inst)
                fprintf(fp, "%s %s\n", DIFF_RTOK, DIFF_INSTANCES);
            else
                fprintf(fp, "%s %s %s\n", DIFF_RTOK, DIFF_LAYER, lname);
            phead = true;
        }
        fprintf(fp, "%s;\n", l0->string);
        stringlist *sx = l0;
        l0 = l0->next;
        sx->next = ld_list21;
        ld_list21 = sx;
    }

    if (ld_pdiffs) {
        if (!inst)
            fprintf(fp, "# Layer %s\n", lname);
        for (Pdiff *p = ld_pdiffs; p; p = p->next_pdiff())
            p->print(fp);
    }
}


// Static function.
Ldiff *
Ldiff::save(Ldiff *ldiff, CDl *ldesc, const char *s1, const char *s2)
{
    if (!ldiff) {
        if (!ldesc)
            return (0);
        ldiff = new Ldiff(ldesc);
    }
    ldiff->add(s1, s2);
    return (ldiff);
}
// End of Ldiff functions.


// This can be set by the caller to implement crude user progress
// feedback for geometric mode checking.
//
void (*Sdiff::ufb_callback)() = 0;

void
Sdiff::print(FILE *fp)
{
    if (sd_pdiffs)
        sd_pdiffs->print(fp);
    for (Ldiff *l = sd_ldiffs; l; l = l->next_ldiff())
        l->print(fp);
}
// End of Sdiff functions.


bool
cinst_t::operator==(cinst_t &c)
{
    if (!c_cdesc || !c.c_cdesc)
        return (false);
    if (c_cdesc->attr() != c.c_cdesc->attr())
        return (false);
    if (c_cdesc->posX() != c.c_cdesc->posX())
        return (false);
    if (c_cdesc->posY() != c.c_cdesc->posY())
        return (false);
    return (true);
}


unsigned int
cinst_t::hash()
{
    if (!c_cdesc)
        return (0);
    unsigned int k = INCR_HASH_INIT;
    int tmp = c_cdesc->posX();
    k = incr_hash(k, &tmp);
    tmp = c_cdesc->posY();
    k = incr_hash(k, &tmp);
    tmp = c_cdesc->attr();
    k = incr_hash(k, &tmp);
    return (k);
}


char *
cinst_t::string(const char *name)
{
    if (!c_cdesc)
        return (lstring::copy("internal error: null cdesc!"));

    sLstr lstr;
    lstr.add("C (");
    lstr.add(name);
    CDap ap(c_cdesc);
    CDtx tx(c_cdesc);
    if (tx.magn != 1.0) {
        lstr.add(" Mag ");
        lstr.add_d(tx.magn, 6);
    }
    lstr.add_c(')');
    if (tx.refly)
        lstr.add(" MY");
    if (tx.ax != 1 || tx.ay != 0) {
        lstr.add(" R ");
        lstr.add_i(tx.ax);
        lstr.add_c(' ');
        lstr.add_i(tx.ay);
    }
    if (c_cdesc->posX() || c_cdesc->posY()) {
        lstr.add(" T ");
        lstr.add_i(c_cdesc->posX());
        lstr.add_c(' ');
        lstr.add_i(c_cdesc->posY());
    }
    if (ap.nx > 1 || ap.ny > 1) {
        lstr.add(" (nx=");
        lstr.add_i(ap.nx);
        lstr.add(" ny=");
        lstr.add_i(ap.ny);
        lstr.add(" dx=");
        lstr.add_i(ap.dx);
        lstr.add(" dy=");
        lstr.add_i(ap.dy);
        lstr.add_c(')');
    }
    return (lstr.string_trim());
}
// End of cinst_t functions.


bool
celt_t::operator==(celt_t &c)
{
    if (fabs(magn - c.magn) > 1e-12)
        return (false);
    if (abs(tx - c.tx) > 1 || abs(ty - c.ty) > 1)
        return (false);
    if (ax != c.ax || ay != c.ay || refly != c.refly)
        return (false);
    return (true);
}


unsigned int
celt_t::hash()
{
    unsigned int k = INCR_HASH_INIT;
    k = incr_hash(k, &tx);
    k = incr_hash(k, &ty);
    if (magn != 1.0)
        k = incr_hash(k, &magn);
    k = incr_hash(k, &ax);
    k = incr_hash(k, &ay);
    k = incr_hash(k, &refly);
    return (k);
}


char *
celt_t::string(const char *name)
{
    sLstr lstr;
    lstr.add("C (");
    lstr.add(name);
    if (magn != 1.0) {
        lstr.add(" Mag ");
        lstr.add_d(magn, 6);
    }
    lstr.add_c(')');
    if (refly)
        lstr.add(" MY");
    if (ax != 1 || ay != 0) {
        lstr.add(" R ");
        lstr.add_i(ax);
        lstr.add_c(' ');
        lstr.add_i(ay);
    }
    if (tx || ty) {
        lstr.add(" T ");
        lstr.add_i(tx);
        lstr.add_c(' ');
        lstr.add_i(ty);
    }
    return (lstr.string_trim());
}
// End of celt_t functions.

