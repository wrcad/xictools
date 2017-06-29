
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2015 Whiteley Research Inc, all rights reserved.        *
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
 $Id: si_lspec.cc,v 5.2 2017/03/19 01:58:46 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "si_parsenode.h"
#include "si_lexpr.h"
#include "si_lspec.h"
#include "si_parser.h"


//
// The sLspec is the structure use to carry layer expressions in DRC
// and extraction systems, and elsewhere.  The methods defined are a
// bit specific to those applications.
//

sLspec::~sLspec()
{
    ls_tree->free();
    delete [] ls_lname;
}


void
sLspec::reset()
{
    ls_tree->free();
    delete [] ls_lname;
    ls_tree = 0;
    ls_ldesc = 0;
    ls_lname = 0;
}


// Parse line as far as possible.  If the line consists of a single layer
// token, return the layer name in lname.  Otherwise, return the tree.
// Return false if error.  This supports the layer_name[.stname][.cellname]
// form.
//
// If noexp is false, derived layer expressions will be parsed
// recursively, providing a tree that has regular layers only. 
// Otherwise, derived layers will appear (by name) the same as regular
// layers.  This option may apply when the component derived layers
// have been pre-evaluated, so that evaluation requires only grabbing
// geometry from the cell database, as for normal layers.
//
bool
sLspec::parseExpr(const char **line, bool noexp)
{
    reset();
    ParseNode *p = SIparse()->getLexprTree(line, noexp);
    if (!p) {
        char *er = SIparse()->errMessage();
        if (er) {
            Errs()->add_error(er);
            delete [] er;
        }
        else
            Errs()->add_error("Parse error: no layer expression found");
        return (false);
    }
    while (isspace(**line) || **line == ')')
        (*line)++;
    if (p->type == PT_VAR) {
        if (p->data.v->type == TYP_LDORIG) {
            LDorig *ldo = p->data.v->content.ldorig;
            if (!ldo->cellname() && !ldo->stab_name() && ldo->ldesc() &&
                    ldo->ldesc() != CellLayer()) {
                set_lname(ldo->ldesc()->name());
                p->free();
                return (true);
            }
        }
        else if (p->data.v->type == TYP_STRING ||
                p->data.v->type == TYP_NOTYPE) {
            // The string is the layer name.
            // The variables are stored out of the parse tree, so freeing
            // p will not clobber this string.
            //
            // If the name is "$$", it is the cell layer, so handle it
            // through the parse tree.

            if (p->data.v->name[0] != '$' || p->data.v->name[1] != '$' ||
                    p->data.v->name[2]) {
                set_lname(p->data.v->name);
                p->free();
                return (true);
            }
        }
    }
    ls_tree = p;
    return (true);
}


// This should be called before evaluation.
//
bool
sLspec::setup()
{
    if (ls_tree) {
        char *s = ls_tree->checkLayersInTree();
        if (s) {
            Errs()->add_error("unknown layer %s", s);
            delete [] s;
            return (false);
        }
    }
    else if (!ls_ldesc && ls_lname) {
        ls_ldesc = CDldb()->findLayer(ls_lname, Physical);
        if (!ls_ldesc)
            ls_ldesc = CDldb()->findDerivedLayer(ls_lname);
        if (!ls_ldesc) {
            Errs()->add_error("unknown layer %s", ls_lname);
            return (false);
        }
    }
    return (true);
}


char *
sLspec::string(bool nullret) const
{
    char *s = 0;
    if (ls_lname)
        s = lstring::copy(lname());
    else if (ls_tree) {
        sLstr lstr;
        ls_tree->string(lstr);
        s = lstr.string_trim();
    }
    else if (ls_ldesc)
        s = lstring::copy(ls_ldesc->name());
    if (!s) {
        if (!nullret)
            s = lstring::copy("???");
    }
    else if (!*s) {
        delete [] s;
        if (!nullret)
            s = lstring::copy("???");
        else
            s = 0;
    }
    return (s);
}


void
sLspec::print(FILE *fp)
{
    if (ls_lname)
        fputs(ls_lname, fp);
    else if (ls_tree)
        ls_tree->print(fp);
    else if (ls_ldesc)
        fputs(ls_ldesc->name(), fp);
}


CDll *
sLspec::findLayers()
{
    {
        sLspec *lst = this;
        if (!lst)
            return (0);
    }
    if (ls_tree)
        return (ls_tree->findLayersInTree());
    if (ls_ldesc)
        return (new CDll(ls_ldesc, 0));
    if (ls_lname) {
        ls_ldesc = CDldb()->findLayer(ls_lname, Physical);
        if (!ls_ldesc)
            ls_ldesc = CDldb()->findDerivedLayer(ls_lname);
        if (ls_ldesc)
            return (new CDll(ls_ldesc, 0));
    }
    return (0);
}


// Test the background zoids for coverage, return immediately on a
// zoid where coverage is not full.  Possible return values are
//   CovNone    Only one background zoid and no intersection.
//   CovPartial A partially covered zoid, or a non-intersecting
//              zoid with more in the list that are NOT TESTED.
//   CovFull    Background zoids are fully covered.
//
XIrt
sLspec::testZlistCovFull(SIlexprCx *cx, CovType *cov, int minsz)
{
    if (!cx)
        return (XIbad);
    *cov = CovNone;
    bool found_full = false;

    if (!ls_tree) {
        // Note:  named database/symtab access is through tree, only
        // simple cell access here.

        if (ls_lname && !ls_ldesc) {
            ls_ldesc = CDldb()->findLayer(ls_lname, Physical);
            if (!ls_ldesc)
                ls_ldesc = CDldb()->findDerivedLayer(ls_lname);
        }
        if (!ls_ldesc)
            return (XIbad);

        const CDs *sdesc = cx->celldesc();
        if (!sdesc)
            sdesc = SIparse()->ifGetCurPhysCell();
        if (!sdesc)
            return (XIbad);

        for (const Zlist *zl = cx->getZref(); zl; zl = zl->next) {
            bool cc;
            if (!zl->Z.test_coverage(sdesc, ls_ldesc, &cc, minsz)) {
                *cov = CovPartial;
                return (XIok);
            }
            if (cc) {
                found_full = true;
                *cov = CovFull;
            }
            else {
                if (found_full|| zl->next)
                    *cov = CovPartial;
                return (XIok);
            }
        }
        return (XIok);
    }

    for (const Zlist *zl = cx->getZref(); zl; zl = zl->next) {
        try {
            siVariable v;
            const Zlist *zbak = cx->getZref();
            Zlist zref(&zl->Z);
            cx->setZref(&zref);
            cx->enableExceptions(true);
            bool ok = (*ls_tree->evfunc)(ls_tree, &v, cx);
            cx->enableExceptions(false);
            cx->setZref(zbak);
            if (ok != OK || v.type != TYP_ZLIST)
                return (XIbad);

            Zlist *z = v.content.zlist;
            z = Zlist::filter_drc_slivers(z, minsz);
            if (z) {
                Zlist *za = Zlist::copy(&zref);
                XIrt ret = Zlist::zl_andnot(&za, z);
                if (ret != XIok)
                    return (ret);
                za = Zlist::filter_drc_slivers(za, minsz);
                if (!za) {
                    found_full = true;
                    *cov = CovFull;
                }
                else {
                    Zlist::free(za);
                    *cov = CovPartial;
                    return (XIok);
                }
            }
            else {
                if (found_full || zl->next)
                    *cov = CovPartial;
                return (XIok);
            }

        }
        catch (XIrt ret) {
            return (XIintr);
        }
    }
    return (XIok);
}


// Test the background zoids for coverage.  Return immeditely for a
// zoid with partialoverage, or after once full-coverage and one
// no-coverage zoids are seen.  Possible return values are
//   CovNone    No background zoids intersect.
//   CovPartial A partially covered zoid, or a non-intersecting
//              zoid and fully-covered zoid seen.
//   CovFull    Background zoids are fully covered.
//
XIrt
sLspec::testZlistCovPartial(SIlexprCx *cx, CovType *cov, int minsz)
{
    if (!cx)
        return (XIbad);
    *cov = CovNone;
    bool found_full = false;
    bool found_none = false;

    if (!ls_tree) {
        // Note:  named database/symtab access is through tree, only
        // simple cell access here.

        if (ls_lname && !ls_ldesc) {
            ls_ldesc = CDldb()->findLayer(ls_lname, Physical);
            if (!ls_ldesc)
                ls_ldesc = CDldb()->findDerivedLayer(ls_lname);
        }
        if (!ls_ldesc)
            return (XIbad);

        const CDs *sdesc = cx->celldesc();
        if (!sdesc)
            sdesc = SIparse()->ifGetCurPhysCell();
        if (!sdesc)
            return (XIbad);

        for (const Zlist *zl = cx->getZref(); zl; zl = zl->next) {
            bool cc;
            if (!zl->Z.test_coverage(sdesc, ls_ldesc, &cc, minsz)) {
                *cov = CovPartial;
                return (XIok);
            }
            if (cc) {
                if (found_none) {
                    *cov = CovPartial;
                    return (XIok);
                }
                found_full = true;
                *cov = CovFull;
            }
            else {
                if (found_full) {
                    *cov = CovPartial;
                    return (XIok);
                }
                found_none = true;
            }
        }
        return (XIok);
    }

    for (const Zlist *zl = cx->getZref(); zl; zl = zl->next) {
        try {
            siVariable v;
            const Zlist *zbak = cx->getZref();
            Zlist zref(&zl->Z);
            cx->setZref(&zref);
            cx->enableExceptions(true);
            bool ok = (*ls_tree->evfunc)(ls_tree, &v, cx);
            cx->enableExceptions(false);
            cx->setZref(zbak);
            if (ok != OK || v.type != TYP_ZLIST)
                return (XIbad);

            Zlist *z = v.content.zlist;
            z = Zlist::filter_drc_slivers(z, minsz);
            if (z) {
                Zlist *za = Zlist::copy(&zref);
                XIrt ret = Zlist::zl_andnot(&za, z);
                if (ret != XIok)
                    return (ret);
                za = Zlist::filter_drc_slivers(za, minsz);
                if (!za) {
                    if (found_none) {
                        *cov = CovPartial;
                        return (XIok);
                    }
                    found_full = true;
                    *cov = CovFull;
                }
                else {
                    Zlist::free(za);
                    *cov = CovPartial;
                    return (XIok);
                }
            }
            else {
                if (found_full) {
                    *cov = CovPartial;
                    return (XIok);
                }
                found_none = true;
            }
        }
        catch (XIrt ret) {
            return (XIintr);
        }
    }
    return (XIok);
}


// Test the background zoids for coverage, return immediately on a
// zoid where coverage is not empty.  Return values are CovNone
// and CovPartial.
//
XIrt
sLspec::testZlistCovNone(SIlexprCx *cx, CovType *cov, int minsz)
{
    if (!cx)
        return (XIbad);
    *cov = CovNone;

    if (!ls_tree) {
        // Note:  named database/symtab access is through tree, only
        // simple cell access here.

        if (ls_lname && !ls_ldesc) {
            ls_ldesc = CDldb()->findLayer(ls_lname, Physical);
            if (!ls_ldesc)
                ls_ldesc = CDldb()->findDerivedLayer(ls_lname);
        }
        if (!ls_ldesc)
            return (XIbad);

        const CDs *sdesc = cx->celldesc();
        if (!sdesc)
            sdesc = SIparse()->ifGetCurPhysCell();
        if (!sdesc)
            return (XIbad);

        for (const Zlist *zl = cx->getZref(); zl; zl = zl->next) {
            if (zl->Z.test_existence(sdesc, ls_ldesc, minsz)) {
                *cov = CovPartial;
                return (XIok);
            }
        }
        return (XIok);
    }

    for (const Zlist *zl = cx->getZref(); zl; zl = zl->next) {
        try {
            siVariable v;
            const Zlist *zbak = cx->getZref();
            Zlist zref(&zl->Z);
            cx->setZref(&zref);
            cx->enableExceptions(true);
            bool ok = (*ls_tree->evfunc)(ls_tree, &v, cx);
            cx->enableExceptions(false);
            cx->setZref(zbak);
            if (ok != OK || v.type != TYP_ZLIST)
                return (XIbad);

            Zlist *z = v.content.zlist;
            z = Zlist::filter_drc_slivers(z, minsz);
            if (z) {
                *cov = CovPartial;
                Zlist::free(z);
                return (XIok);
            }
        }
        catch (XIrt ret) {
            return (XIintr);
        }
    }
    return (XIok);
}


// Return a list of zoids in zret that intersect elements of zl.  The
// sdesc is the source for the geometry.
//
XIrt
sLspec::getZlist(SIlexprCx *cx, Zlist **zret, bool isclear)
{
    *zret = 0;
    if (!cx)
        return (XIbad);

    if (ls_tree) {
        return (ls_tree->evalTree(cx, zret,
            isclear ? PolarityClear : PolarityDark));
    }

    if (ls_lname && !ls_ldesc) {
            ls_ldesc = CDldb()->findLayer(ls_lname, Physical);
        if (!ls_ldesc)
            ls_ldesc = CDldb()->findDerivedLayer(ls_lname);
    }
    if (ls_ldesc) {
        Zlist *z0;
        XIrt ret = cx->getZlist(ls_ldesc, &z0);

        if (ret != XIok)
            return (ret);
        if (isclear) {
            *zret = Zlist::copy(cx->getZref());
            return (Zlist::zl_andnot(zret, z0));
        }
        *zret = z0;
    }
    return (XIok);
}


// Return true if there is no tree, or if there is at least partial
// coverage within the odesc area.
//
XIrt
sLspec::testContact(const CDs *sdesc, int maxdepth, const CDo *odesc,
    bool *istrue)
{
    if (!odesc)
        return (XIbad);
    if (!ls_tree) {
        *istrue = true;
        return (XIok);
    }

    Zlist *zl = odesc->toZlist();
    XIrt ret = testContact(sdesc, maxdepth, zl, istrue);
    Zlist::free(zl);
    return (ret);
}


// Return true if there is no tree, or if there is at least partial
// coverage within the Zlist area.
//
XIrt
sLspec::testContact(const CDs *sdesc, int maxdepth, const Zlist *zl0,
    bool *istrue)
{
    if (!zl0)
        return (XIbad);
    if (!ls_tree) {
        *istrue = true;
        return (XIok);
    }
    *istrue = false;
    if (!sdesc)
        return (XIok);

#define MINVSIZE 10

    try {
        siVariable v;
        SIlexprCx cx(sdesc, maxdepth, zl0);
        cx.enableExceptions(true);
        if ((*ls_tree->evfunc)(ls_tree, &v, &cx) != OK)
            return (XIbad);
        cx.enableExceptions(false);
        if (v.type == TYP_ZLIST && v.content.zlist != 0) {
            // something left, evaluate true if a zoid is large enough
            for (Zlist *z = v.content.zlist; z; z = z->next) {
                if (z->Z.yu - z->Z.yl > MINVSIZE &&
                        (z->Z.xlr - z->Z.xll > MINVSIZE ||
                        z->Z.xur - z->Z.xul > MINVSIZE)) {
                    *istrue = true;
                    break;
                }
            }
            Zlist::free(v.content.zlist);
        }
        return (XIok);
    }
    catch (XIrt ret) {
        return (XIintr);
    }
}

