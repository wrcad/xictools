
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
#include "dsp_layer.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "geo_zlist.h"
#include "geo_grid.h"
#include "si_parsenode.h"
#include "si_lexpr.h"
#include "si_lspec.h"
#include "si_parser.h"
#include "promptline.h"
#include "errorlog.h"
#include "miscutil/timer.h"
#include "miscutil/threadpool.h"
#include "miscutil/timedbg.h"


// Core command procedure for layer expression evaluator.
//
void
cEdit::createLayerCmd(const char *expr, int depth, int flags)
{
    bool undoable = !(flags & CLnoUndo);

    if (depth < 0)
        depth = 0;
    CDs *cursd = CurCell();
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    Tdbg()->start_timing("create_layer");
    DSPpkg::self()->SetWorking(true);
    PL()->ShowPrompt("Working...");
    if (undoable)
        Ulist()->ListCheck("layer", cursd, false);
    int ret = ED()->createLayerRecurse(expr, depth, flags);
    if (undoable)
        Ulist()->CommitChanges();
    if (ret == XIok) {
        DSP()->RedisplayAll();
        PL()->ShowPrompt("Operation succeeded.");
    }
    else if (ret == XIbad)
        PL()->ShowPrompt("Operation FAILED.");
    DSPpkg::self()->SetWorking(false);
    Tdbg()->print_accum(TDB_ALL_ACCUM);
    Tdbg()->stop_timing("create_layer");
}


// Create a new layer and fill it from the expression in string, which
// has the form "new_layer_name  expression".  The hierarchy is processed
// to depth, 0 means current cell only.  If flat is true, reflect underlying
// structure to top level, otherwise create layer in subcells recursively.
//
XIrt
cEdit::createLayerRecurse(const char *string, int depth, int flags)
{
    bool undoable = !(flags & CLnoUndo);
    bool flat = !(flags & CLrecurse);

    if (depth < 0)
        depth = 0;
    CDs *cursd = CurCell();
    if (!cursd)
        return (XIbad);
    if (flat || depth == 0)
        return (ED()->createLayer(cursd, string, depth, flags));

    CDgenHierDn_s gen(cursd, depth);
    bool err;
    CDs *sd;
    while ((sd = gen.next(&err)) != 0) {
        if (undoable)
            Ulist()->ListChangeCell(sd);
        XIrt ret = ED()->createLayer(sd, string, 0, flags);
        if (ret != XIok)
            return (ret);
    }
    return (XIok);
}


namespace { const char *layer_creation = "layer creation"; }

// str format: layer_name [[=] expression]
// Create a new layer or use existing "layer_name" and add the results
// of the expression to this layer.
//
// If no expression:
// Create a new layer if the target doesn't eist.  If the layer exists,
// and mode is not CLdefault, apply the join/split to the existing layer,
// otherwise just return.  In this case with join/split, the merge flag
// is ignored.
//
XIrt
cEdit::createLayer(CDs *sdesc, const char *str, int depth, int flags)
{
    int mode = flags & CLmode;

    if (!sdesc)
        return (XIok);
    if (!str)
        return (XIbad);
    while (isspace(*str) || *str == '"')
        str++;
    const char *t = str;
    while (*t && !isspace(*t) && *t != '=' && *t != '"')
        t++;
    if (t == str) {
        Log()->ErrorLog(layer_creation, "createLayer: invalid layer name.");
        return (XIbad);
    }
    char *lbuf = new char[t - str + 1];
    GCarray<char*> gc_lbuf(lbuf);
    strncpy(lbuf, str, t-str);
    lbuf[t-str] = 0;
    str = t;

    // Find or create target layer.
    bool target_exists = false;
    DisplayMode dspmode = sdesc->isElectrical() ? Electrical : Physical;
    CDl *ld = CDldb()->findLayer(lbuf, dspmode);
    if (ld)
        target_exists = true;
    else {
        ld = CDldb()->newLayer(lbuf, dspmode);
        if (!ld) {
            Log()->ErrorLog(layer_creation, Errs()->get_error());
            Log()->ErrorLog(layer_creation,
                "createLsyer: couldn't create new layer.");
            return (XIbad);
        }
        CDl *tld = CDldb()->findDerivedLayer(lbuf);
        if (tld) {
            // A derived layer of the same name exists.  Transfer
            // attributes to the new layer,
            ld->setBlink(tld->isBlink());
            ld->setCut(tld->isCut());
            ld->setOutlined(tld->isOutlined());
            ld->setOutlinedFat(tld->isOutlinedFat());
            ld->setFilled(tld->isFilled());

            ld->setDescription(tld->description());

            DspLayerParams *p = dsp_prm(ld);
            DspLayerParams *q = dsp_prm(tld);
            p->set_red(q->red());
            p->set_green(q->green());
            p->set_blue(q->blue());
            p->set_pixel(q->pixel());
            unsigned char *map = q->fill()->newBitmap();
            p->fill()->newMap(q->fill()->nX(), q->fill()->nY(), map);
            delete [] map;
        }
    }

    while (isspace(*str) || *str == '=')
        str++;
    if (!*str) {
        if (mode == CLdefault || !target_exists)
            // Just quit, having added new layer.
            return (XIok);
        else
            // Set up so that the join/split is applied to the target
            // layer.
            str = lbuf;
    }
    return (createLayer(sdesc, 0, ld, str, depth, flags));
}


namespace {
    // Stuff for multi-threading support.  When gridding, we can
    // assign grid cell evaluation to different threads.

    // Container for various global run parameters.
    //
    struct th_gvars_t
    {
        th_gvars_t(CDs *sd, CDl *ld, sLspec *s, int d, int m, int b,
            bool u, bool t)
            {
                sdesc = sd;
                ldesc = ld;
                lspec = s;
                depth = d;
                mode  = m;
                blval = b;
                ud    = u;
                tmp_merge = t;
            }

        CDs *sdesc;
        CDl *ldesc;
        sLspec *lspec;
        int depth;
        int mode;
        int blval;
        bool ud;
        bool tmp_merge;

    };

    // Container for local run parameters and a pointer to global
    // parameters, to be passed to the thread work function.
    //
    struct th_lvars_t
    {
        th_lvars_t(th_gvars_t *g, const BBox *bb) : gvars(g), AOI(*bb) { }

        th_gvars_t *gvars;
        BBox AOI;
    };

    // The thread work function.
    //
    int thread_proc(sTPthreadData*, void *arg)
    {
        th_lvars_t *lv = (th_lvars_t*)arg;
        th_gvars_t *gv = lv->gvars;

#ifdef TH_DEBUG
        printf("%lx  ", arg);
        lv->AOI.print();
#endif
        BBox xBB(lv->AOI);
        if (gv->blval > 0)
            xBB.bloat(gv->blval);
        SIlexprCx cx(gv->sdesc, gv->depth, &xBB);

        Zlist *zret;
        XIrt ret = gv->lspec->tree()->evalTree(&cx, &zret, PolarityDark);
        if (ret != XIok) {
            delete lv;
            // The XIok enum value is 0 unless somebody changed it.
            if (XIok == 0)
                return (ret);
            return (1);
        }
        if (gv->blval > 0)
            Zlist::zl_and(&zret, new Zlist(&lv->AOI, 0));

        if (gv->mode == CLsplitH) {
            Zlist::add(zret, gv->sdesc, gv->ldesc, gv->ud, gv->tmp_merge);
            Zlist::destroy(zret);
        }
        else if (gv->mode == CLsplitV) {
            zret = Zlist::to_r(zret);
            Zlist::add_r(zret, gv->sdesc, gv->ldesc, gv->ud, gv->tmp_merge);
            Zlist::destroy(zret);
        }
        else {
            // default
            ret = Zlist::to_poly_add(zret, gv->sdesc, gv->ldesc, gv->ud, 0,
                gv->tmp_merge);
        }
        delete lv;  // Clean up done here!
        return (0);
    }
}


XIrt
cEdit::createLayer(CDs *sdesc, const BBox *pAOI, CDl *ld, const char *str,
    int depth, int flags)
{
    if (!sdesc)
        return (XIok);

    sLspec lspec;
    Errs()->init_error();
    if (!lspec.parseExpr(&str)) {
        Log()->ErrorLogV(layer_creation,
            "createLayer: Expression parse failed:\n%s", Errs()->get_error());
        return (XIbad);
    }

    if (lspec.lname()) {
        return (createLayer_notree(sdesc, pAOI, ld, lspec.lname(), depth,
            flags));
    }
    if (!lspec.tree())
        return (XIok);

    DisplayMode dspmode = sdesc->displayMode();
    if (dspmode == Electrical) {
        Log()->ErrorLog(layer_creation,
            "Non-trivial layer expressions are not available\n"
            "in electrical mode.");
        return (XIbad);
    }
    char *bad = lspec.tree()->checkLayersInTree();
    if (bad) {
        Log()->ErrorLogV(layer_creation,
            "createLayer: unknown layer %s.", bad);
        delete [] bad;
        return (XIbad);
    }

    // If cells are found (references in the layer name), the BB
    // is expanded to include the cells' boundaries.  We need to
    // expand AOI if creating the full layer, which is assumed if
    // pAOI is null.  If non-null pAOI is passed, we keep the AOI
    // fixed.

    BBox AOI(pAOI ? *pAOI : *sdesc->BB());
    {
        BBox BB(AOI);
        bad = lspec.tree()->checkCellsInTree(&BB);
        if (bad) {
            Log()->ErrorLogV(layer_creation,
                "createLayer: unknown cell %s.", bad);
            delete [] bad;
            return (XIbad);
        }
        if (!pAOI)
            AOI = BB;
    }

    // If the target layer is also used in the expression, write the
    // new data to a temporary layer, maybe clear the new layer, then
    // copy the temp data to the new layer.  If not, the use of a temp
    // layer can be skipped.
    //
    bool use_tmp = lspec.tree()->isLayerInTree(ld);
    int mode = flags & CLmode;
    bool undoable = !(flags & CLnoUndo);
    bool use_merge = flags & CLmerge;
    bool clear = !(flags & CLnoClear);

    CDl *tmpld = 0;
    if (use_tmp) {
        tmpld = ld;
        // find or create "temporary" layer
        const char *tmpl = "$tmp";
        ld = CDldb()->newLayer(tmpl, dspmode);
        if (!ld) {
            Log()->ErrorLog(layer_creation, Errs()->get_error());
            Log()->ErrorLog(layer_creation,
                "createLayer: couldn't create temporary layer.");
            return (XIbad);
        }
        sdesc->clearLayer(ld);
    }
    else {
        if (clear)
            sdesc->clearLayer(ld, undoable);
    }
    bool ud = (undoable && !tmpld);
    bool tmp_merge = use_merge & !tmpld;

    grd_t grd(&AOI, grd_t::def_gridsize());
    const BBox *gBB;

    int numgrd = grd.numgrid() - 1;
    int nth = DSP()->NumThreads();
    if (nth > numgrd)
        nth = numgrd;
    numgrd++;

    int bloatval = 0;
    lspec.tree()->getBloat(&bloatval);

    if (nth > 0) {
        cThreadPool pool(nth);
        th_gvars_t gvars(sdesc, ld, &lspec, depth, mode, bloatval,
            ud, tmp_merge);
        while ((gBB = grd.advance()) != 0) {
            th_lvars_t *data = new th_lvars_t(&gvars, gBB);
            pool.submit(thread_proc, data);
        }
        pool.run(0);
    }
    else {
        int cnt = 0;
        uint64_t check_time = 0;
        cTimer::self()->check_interval(check_time);
        while ((gBB = grd.advance()) != 0) {
            cnt++;
            if (cTimer::self()->check_interval(check_time))
                PL()->ShowPromptV("Evaluating region %d/%d.", cnt, numgrd);

            // If we're doing a bloat, expand the grid cell size, and
            // clip off the extra later.
            BBox xBB(*gBB);
            if (bloatval > 0)
                xBB.bloat(bloatval);
            SIlexprCx cx(sdesc, depth, &xBB);
            Zlist *zret;
            XIrt ret = lspec.tree()->evalTree(&cx, &zret, PolarityDark);
            if (ret == XIbad) {
                Log()->ErrorLog(layer_creation,
                    "createLayer: Evaluation failed.");
                return (XIbad);
            }
            if (ret == XIintr)
                return (XIintr);
            if (!zret)
                continue;
            if (bloatval > 0)
                Zlist::zl_and(&zret, new Zlist(gBB, 0));

            if (mode == CLsplitH) {
                Zlist::add(zret, sdesc, ld, ud, tmp_merge);
                Zlist::destroy(zret);
            }
            else if (mode == CLsplitV) {
                zret = Zlist::to_r(zret);
                Zlist::add_r(zret, sdesc, ld, ud, tmp_merge);
                Zlist::destroy(zret);
            }
            else {
                // default
                ret = Zlist::to_poly_add(zret, sdesc, ld, ud, 0, tmp_merge);
                if (ret != XIok)
                    return (ret);
            }
        }
    }

    if (tmpld) {
        if (clear)
            sdesc->clearLayer(tmpld, undoable);
        CDg gdesc;
        gdesc.init_gen(sdesc, ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (!change_layer(odesc, sdesc, tmpld, false, undoable,
                    use_merge)) {
                Errs()->add_error("change_layer failed");
                Log()->ErrorLog(layer_creation, Errs()->get_error());
            }
        }
        sdesc->clearLayer(ld, false);
    }

    if (!undoable && ld->layerType() != CDLinternal &&
            ld->layerType() != CDLderived)
        sdesc->incModified();
    return (XIok);
}


// "Copy" operation, sourcelname is an extended layer name.  This can
// have the form layer_name[.stname][.cellname].  The AOI is ignored
// here, unless pAOI is non-null, in which case we clip.
//
XIrt
cEdit::createLayer_notree(CDs *sdesc, const BBox *pAOI, CDl *ld,
    const char *sourcelname, int depth, int flags)
{
    if (!sdesc || !sourcelname)
        return (XIbad);

    char *cellname, *stname;
    char *lname = SIparse()->parseLayer(sourcelname, &cellname, &stname);
    DisplayMode dspmode = sdesc->displayMode();
    CDl *sld = CDldb()->findLayer(lname, dspmode);
    if (!sld)
        sld = CDldb()->findDerivedLayer(lname);
    if (!sld) {
        Log()->ErrorLogV(layer_creation,
            "createLayer: unknown layer %s.", lname);
        delete [] lname;
        delete [] cellname;
        delete [] stname;
        return (XIbad);
    }
    delete [] lname;

    BBox AOI(pAOI ? *pAOI : *sdesc->BB());
    bool import = false;
    CDs *targ = sdesc;
    Zlist *zimport = 0;
    if (cellname || stname) {
        if (cellname && *cellname == SI_DBCHAR && (!stname || !*stname)) {
            // This triggers use of a named database.

            Zlist ztmp(&AOI);
            SIlexprCx cx(cellname+1, &ztmp);
            XIrt ret = cx.getDbZlist(sld, sld->name(), &zimport);
            if (ret != XIok) {
                delete [] cellname;
                delete [] stname;
                return (ret);
            }
        }
        else {
            targ = SIparse()->openReference(cellname, stname);
            if (!targ) {
                if (!stname)
                    Log()->ErrorLogV(layer_creation,
                        "createLayer: unknown cellname \"%s\".",
                        cellname);
                else if (cellname)
                    Log()->ErrorLogV(layer_creation,
                        "createLayer: unknown cellname \"%s.%s\".",
                        stname, cellname);
                else
                    Log()->ErrorLogV(layer_creation,
                        "createLayer: unknown cellname \"%s.NULL\".",
                        stname);
                delete [] cellname;
                delete [] stname;
                return (XIbad);
            }
        }
        delete [] cellname;
        delete [] stname;
        import = true;
    }

    int mode = flags & CLmode;
    bool undoable = !(flags & CLnoUndo);
    bool use_merge = flags & CLmerge;
    bool clear = !(flags & CLnoClear);

    if (sld == ld && !import && mode == CLdefault) {
        CDl *tld = CDldb()->findDerivedLayer(ld->name());

        // The user is assigning to a layer with the same name as
        // a derived layer.  In this case, copy from the derived
        // layer.  The existence of a non-derived layer of the
        // same name will mostly prevent access to the derived
        // layer, until the non-derived layer is renamed or
        // deleted.

        sld = tld;
    }

    if (sld != ld || import || mode != CLdefault) {
        PL()->ShowPrompt("Copying ...");
        CDl *tmpld = 0;
        if (ld == sld && !import) {
            // Destination is same as source, we're doing a join
            // or split.  New objects will be created on a
            // temporary layer.

            clear = true;
            use_merge = false;

            tmpld = ld;
            // find or create "temporary" layer
            const char *tmpl = "$tmp";
            ld = CDldb()->newLayer(tmpl, dspmode);
            if (!ld) {
                Log()->ErrorLog(layer_creation, Errs()->get_error());
                Log()->ErrorLog(layer_creation,
                    "createLayer: couldn't create temporary layer.");
                return (XIbad);
            }
            sdesc->clearLayer(ld);
        }
        else {
            // Source and target are different, no temporary layer
            // is needed.
            if (clear)
                sdesc->clearLayer(ld, undoable);
        }

        bool ud = (undoable && !tmpld);
        if (zimport) {
            if (mode == CLsplitV) {
                zimport = Zlist::to_r(zimport);
                Zlist::add_r(zimport, sdesc, ld, ud, use_merge);
                Zlist::destroy(zimport);
            }
            else if (mode == CLjoin) {
                XIrt ret = Zlist::to_poly_add(zimport, sdesc, ld, ud, 0,
                    use_merge);
                if (ret != XIok)
                    return (ret);
            }
            else {
                Zlist::add(zimport, sdesc, ld, ud, use_merge);
                Zlist::destroy(zimport);
            }
        }
        else {
            sPF gen(targ, &CDinfiniteBB, sld, depth);
            CDo *odesc;

            Zlist *zl0 = 0, *ze = 0;
            int zcnt = 0;
            while ((odesc = gen.next(false, false)) != 0) {
                if (mode == CLsplitH) {
                    Zlist *z = odesc->toZlist();
                    if (pAOI) {
                        Zoid Z(&AOI);
                        Zlist::zl_and(&z, &Z);
                    }
                    if (z) {
                        Zlist::add(z, sdesc, ld, ud, use_merge);
                        Zlist::destroy(z);
                    }
                }
                else if (mode == CLsplitV) {
                    Zlist *z = odesc->toZlist();
                    if (pAOI) {
                        Zoid Z(&AOI);
                        Zlist::zl_and(&z, &Z);
                    }
                    if (z) {
                        z = Zlist::to_r(z);
                        Zlist::add_r(z, sdesc, ld, ud, use_merge);
                        Zlist::destroy(z);
                    }
                }
                else if (mode == CLjoin) {
                    const int maxq = Zlist::JoinMaxQueue;
                    Zlist *z = odesc->toZlist();
                    if (pAOI) {
                        Zoid Z(&AOI);
                        Zlist::zl_and(&z, &Z);
                    }
                    if (!z)
                        continue;
                    int n = Zlist::length(z);
                    if (maxq > 0 && n >= maxq) {
                        if (pAOI) {
                            XIrt ret = Zlist::to_poly_add(z, sdesc, ld, ud, 0,
                                use_merge);
                            if (ret != XIok) {
                                Zlist::destroy(zl0);
                                delete odesc;
                                return (ret);
                            }
                        }
                        else {
                            Zlist::destroy(z);
                            if (!change_layer(odesc, sdesc, ld, false, ud,
                                    false)) {
                                Errs()->add_error("change_layer failed");
                                Log()->ErrorLog(layer_creation,
                                    Errs()->get_error());
                                Zlist::destroy(zl0);
                                delete odesc;
                                return (XIbad);
                            }
                        }
                        continue;
                    }
                    if (maxq <= 0 || zcnt + n < maxq) {
                        if (!zl0)
                            zl0 = ze = z;
                        else {
                            while (ze->next)
                                ze = ze->next;
                            ze->next = z;
                        }
                        zcnt += n;
                        continue;
                    }
                    XIrt ret = Zlist::to_poly_add(zl0, sdesc, ld, ud, 0,
                        use_merge);
                    if (ret != XIok) {
                        delete odesc;
                        return (ret);
                    }
                    zl0 = ze = z;
                    zcnt = n;
                }
                else {
                    if (!change_layer(odesc, sdesc, ld, false, ud,
                            use_merge)) {
                        Errs()->add_error("change_layer failed");
                        Log()->ErrorLog(layer_creation,
                            Errs()->get_error());
                        delete odesc;
                        return (XIbad);
                    }
                }
                delete odesc;
            }
            if (mode == CLjoin && zcnt) {
                XIrt ret = Zlist::to_poly_add(zl0, sdesc, ld, ud, 0, use_merge);
                if (ret != XIok)
                    return (ret);
            }
        }

        if (tmpld) {
            if (clear)
                sdesc->clearLayer(tmpld, undoable);
            CDg gdesc;
            gdesc.init_gen(sdesc, ld);
            CDo *od;
            while ((od = gdesc.next()) != 0) {
                if (!change_layer(od, sdesc, tmpld, false, undoable,
                        use_merge)) {
                    Errs()->add_error("change_layer failed");
                    Log()->ErrorLog(layer_creation, Errs()->get_error());
                }
            }
            sdesc->clearLayer(ld, false);
        }
    }

    if (!undoable && ld->layerType() != CDLinternal &&
            ld->layerType() != CDLderived)
        sdesc->incModified();
    return (XIok);
}


namespace {
    // Recursively add the referenced derived layers to tab.
    //
    bool addrefs(CDl *ld, SymTab *tab)
    {
        if (ld->layerType() != CDLderived)
            return (true);
        if (SymTab::get(tab, (uintptr_t)ld) != ST_NIL)
            return (true);
        tab->add((uintptr_t)ld, 0, false);
        sLspec lspec;
        const char *ex = ld->drvExpr();
        if (!lspec.parseExpr(&ex)) {
            Log()->ErrorLogV(layer_creation,
                "Parse failed for %s layer expression:\n%s",
                ld->name(), ld->drvExpr(), Errs()->get_error());
            return (false);
        }
        CDll *l0 = lspec.findLayers();
        for (CDll *l = l0; l; l = l->next) {
            if (!addrefs(l->ldesc, tab)) {
                CDll::destroy(l0);
                return (false);
            }
        }
        CDll::destroy(l0);
        return (true);
    }
}

// Derived Layers
//
// There are two ways to handle derived layers.  By default, layer
// expression parse trees are expanded, meaning that when a derived
// layer is encountered, the parser recursively descends into the
// layer's expression.  The resulting tree references only normal
// layers, and evaluation is straightforward.
//
// A second approach might be faster, and is used in DRC.  The parse
// trees are not expanded, and a parse node to a derived layer
// contains a layer desc, just as for normal layers.  Before any
// computation, evalDerivedLayers must be called, which actually
// creates database objects in a database for the derived layer. 
// Evaluation involves only finding the geometry in the search area,
// as for a normal layer.


// Evaluate the listed derived layers.  This must be done in an order
// so that layers are evaluated before being referenced by another
// derived layer, so this is a bit tricky.  On successful return, the
// plist points to all of the descs evaluated, i.e., it includes any
// references not found in the original list.  The updated list can be
// used to clear when done.  The list is updated even if an error
// occurs.
//
XIrt
cEdit::evalDerivedLayers(CDll **plist, CDs *sdesc, const BBox *AOI)
{
    if (!plist)
        return (XIbad);
    CDll *list = *plist;
    if (!list)
        return (XIok);

    if (!sdesc || sdesc->isElectrical()) {
        CDll::destroy(list);
        *plist = 0;
        return (XIbad);
    }
    if (!AOI)
        AOI = sdesc->BB();

    SymTab *dltab = new SymTab(false, false);
    for (CDll *l = list; l; l = l->next) {
        if (!addrefs(l->ldesc, dltab)) {
            delete dltab;
            CDll::destroy(list);
            *plist = 0;
            return (XIbad);
        }
    }

    // The table now contains all of the derived layers we need.  Put
    // these into an array.

    int asize = dltab->allocated();
    if (!asize) {
        delete dltab;
        CDll::destroy(list);
        *plist = 0;
        return (XIok);
    }
    CDl **ary = new CDl*[asize];
    int cnt = 0;
    SymTabGen gen(dltab);
    SymTabEnt *ent;
    while ((ent = gen.next()) != 0)
        ary[cnt++] = (CDl*)ent->stTag;
    delete dltab;

    // Loop through the list, evaluating ldescs that have all
    // references already evaluated.  Break when no evaluations are
    // done.  The dltab will contain the layer descs evaluated,
    // anything left in the array is bad news.

    XIrt ret = XIok;
    dltab = new SymTab(false, false);
    for (;;) {
        int added = 0;
        for (int i = 0; i < asize; i++) {
            if (!ary[i])
                continue;
            sLspec lspec;

            const char *str = ary[i]->drvExpr();
            const char *ex = str;
            if (!lspec.parseExpr(&str)) {
                Log()->ErrorLogV(layer_creation,
                    "Parse failed for %s layer expression:\n%s",
                    ary[i]->name(), ex, Errs()->get_error());
                delete [] ary;
                ret = XIbad;
                goto err;
            }
            bool defer = false;
            CDll *l0 = lspec.findLayers();
            for (CDll *l = l0; l; l = l->next) {
                if (l->ldesc->layerType() == CDLderived) {
                    if (SymTab::get(dltab, (uintptr_t)l->ldesc) == ST_NIL) {
                        defer = true;
                        break;
                    }
                }
            }
            CDll::destroy(l0);
            if (!defer) {
                // For now at least, depth is always maximum, and
                // non-mode flags are ignored except to impose
                // no-undo.

                ret = createLayer(sdesc, AOI, ary[i], ex,
                    CDMAXCALLDEPTH, ary[i]->drvMode() | CLnoUndo);
                if (ret != XIok) {
                    delete [] ary;
                    goto err;
                }
                dltab->add((uintptr_t)ary[i], 0, false);
                ary[i] = 0;
                added++;
            }
        }
        if (!added)
            break;
    }

    {
        sLstr lstr;
        for (int i = 0; i < asize; i++) {
            if (ary[i]) {
                if (!lstr.string()) {
                    lstr.add(
                        "Unable to evaluate the following layers, mutual "
                        "cross dependency?\n");
                }
                lstr.add_c(' ');
                lstr.add(ary[i]->name());
            }
        }
        delete [] ary;
        if (lstr.string()) {
            lstr.add_c('\n');
            Log()->ErrorLog(layer_creation, lstr.string());
            ret = XIbad;
        }
    }

err:
    // Finally, update the list.
    CDll *l0 = 0;
    cnt = 0;
    gen = SymTabGen(dltab);
    while ((ent = gen.next()) != 0)
        l0 = new CDll((CDl*)ent->stTag, l0);
    delete dltab;
    CDll::destroy(list);
    *plist = l0;
    return (ret);
}


// Parse the arguments and advance the string pointer for the syntax
//   [split|splitv|join] -s[h] -sv -j [-d depth] -da [-r] [-c] [-m] [-f]
//     layername [=] expr
// The flags can be grouped, e.g. -cdfrm depth .
// This is used in the !layer command.
//
// False is returned on error here.  The return string should be tested to
// ensure validity of the assignment and expression.
//
bool
cEdit::parseNewLayerSpec(const char **str, int *pdepth, int *pflags)
{
    *pdepth = 0;        // Hierarchy depth to process.
    *pflags = 0;        // Flags to pass to createLayer.

    if (!str)
        return (true);

    int mode = CLdefault;
    int depth = 0;
    bool recurse = false;
    bool noclear = false;
    bool use_merge = false;
    bool fast_mode = false;

    const char *s = *str;
    for (;;) {
        const char *sbeg = s;
        char *tok = lstring::gettok(&s);
        if (!tok) {
            // Shouldn't happen, this is really an error but let the
            // caller deal with it.
            s = sbeg;
            break;
        }

        if (lstring::cieq(tok, "join"))
            mode = CLjoin;
        else if (lstring::cieq(tok, "split") || lstring::cieq(tok, "splith"))
            mode = CLsplitH;
        else if (lstring::cieq(tok, "splitv"))
            mode = CLsplitV;
        else if (*tok == '-') {
            bool had_d = false;
            for (const char *t = tok+1; *t; t++) {
                switch (*t) {
                case 'c':
                case 'C':
                    noclear = true;
                    break;
                case 'd':
                case 'D':
                    if (t[1] == 'a' || t[1] == 'A') {
                        depth = CDMAXCALLDEPTH;
                        t++;
                        break;
                    }
                    had_d = true;
                    break;
                case 'f':
                case 'F':
                    fast_mode = true;
                    break;
                case 'j':
                case 'J':
                    mode = CLjoin;
                    break;
                case 'm':
                case 'M':
                    use_merge = true;
                    break;
                case 'r':
                case 'R':
                    recurse = true;
                    break;
                case 's':
                case 'S':
                    if (t[1] == 'v' || t[1] == 'V') {
                        mode = CLsplitV;
                        t++;
                        break;
                    }
                    if (t[1] == 'h' || t[1] == 'H')
                        t++;
                    mode = CLsplitH;
                    break;
                default:
                    if (*tok == '-') {
                        Errs()->add_error("unrecognized option in \"%s\".",
                            tok);
                        delete [] tok;
                        return (false);
                    }
                }
            }
            if (had_d) {
                delete [] tok;
                tok = lstring::gettok(&s);
                if (!tok) {
                    Errs()->add_error("Missing -d depth argument.");
                    return (false);
                }
                if (*tok == 'a' || *tok == 'A')
                    depth = CDMAXCALLDEPTH;
                else if (!isdigit(*tok)) {
                    Errs()->add_error("Incorrect -d depth argument.");
                    delete [] tok;
                    return (false);
                }
                else {
                    depth = atoi(tok);
                    if (depth < 0)
                        depth = 0;
                    else if (depth > CDMAXCALLDEPTH)
                        depth = CDMAXCALLDEPTH;
                }
            }
        }
        else {
            delete [] tok;
            s = sbeg;
            break;
        }
        delete [] tok;
    }

    int flags = mode;
    if (recurse && depth > 0)
        flags |= CLrecurse;
    if (noclear)
        flags |= CLnoClear;
    if (use_merge)
        flags |= CLmerge;
    if (fast_mode)
        flags |= CLnoUndo;
    *str = s;
    *pdepth = depth;
    *pflags = flags;
    return (true);
}

