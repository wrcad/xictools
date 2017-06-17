
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
 $Id: bloat.cc,v 1.17 2016/04/03 21:30:10 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "edit.h"
#include "drcif.h"
#include "undolist.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "geo_zlist.h"
#include "layertab.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"


//
// Commands to bloat or Manhattanize objects.
//

namespace {
    // List element for an object and its bloated polygon representation,
    // for bloating.
    //
    struct OPlist
    {
        OPlist(CDo *o, PolyList *p, OPlist *n)
            { odesc = o; plist = p; next = n; }
        ~OPlist() { plist->free(); }

        void free()
            {
                OPlist *tn;
                for (OPlist *t = this; t; t = tn) {
                    tn = t->next;
                    delete t;
                }
            }

        void add(CDs*, bool);

        CDo *odesc;
        PolyList *plist;
        OPlist *next;
    };


    // Add the new objects to sdesc, and remove the old objects.  This
    // frees the list.
    //
    void
    OPlist::add(CDs *sdesc, bool undoable)
    {
        Errs()->init_error();
        OPlist *onext;
        for (OPlist *o = this; o; o = onext) {
            onext = o->next;
            CDl *ld = o->odesc->ldesc();
            for (PolyList *pl = o->plist; pl; pl = pl->next) {
                if (!pl->po.numpts)
                    continue;
                if (pl->po.is_rect()) {
                    BBox BB(pl->po.points);
                    delete [] pl->po.points;
                    pl->po.points = 0;
                    CDo *newo;
                    if (sdesc->makeBox(ld, &BB, &newo) != CDok) {
                        Errs()->add_error("makeBox failed");
                        Log()->ErrorLog(mh::ObjectCreation,
                            Errs()->get_error());
                    }
                    else if (undoable) {
                        Ulist()->RecordObjectChange(sdesc, o->odesc, newo);
                        o->odesc = 0;
                    }
                }
                else {
                    CDpo *newo;
                    if (sdesc->makePolygon(ld, &pl->po, &newo) != CDok) {
                        Errs()->add_error("makePolygon failed");
                        Log()->ErrorLog(mh::ObjectCreation,
                            Errs()->get_error());
                    }
                    else if (undoable) {
                        Ulist()->RecordObjectChange(sdesc, o->odesc, newo);
                        o->odesc = 0;
                    }
                }
            }
            if (o->odesc) {
                Ulist()->RecordObjectChange(sdesc, o->odesc, 0);
                o->odesc = 0;
            }
            delete o;
        }
    }
    // End of OPlist functions.
}


// Bloat and replace selected objects.  Giving mode == 0 specifies the
// default bloating algorithm, nonzero specifies a zoidlist algorithm.
//
// If mode == DRC_BLOAT, call the drc bloating function directly.  If
// anything else, pass to the Zlist::bloat, which may itself call the
// DRC bloating function (but with a flag).
//
XIrt
cEdit::bloatQueue(int width, int mode)
{
    if (DSP()->CurMode() != Physical)
        return (XIbad);
    CDs *cursd = CurCell(Physical);
    if (!cursd)
        return (XIbad);
    dspPkgIf()->SetWorking(true);
    PL()->ShowPrompt("Working...");
    int ocnt = 0;
    sSelGen sg(Selections, cursd, "bpw");
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (!od->is_normal())
            continue;
        if (od->state() == CDSelected && od->ldesc()->isSelectable())
            ocnt++;
    }
    if (!ocnt) {
        PL()->ErasePrompt();
        dspPkgIf()->SetWorking(false);
        return (XIok);
    }

    int nused = CDldb()->layersUsed(Physical);
    Zlist **zheads = 0;
    if (mode != DRC_BLOAT) {
        zheads = new Zlist*[nused];
        memset(zheads, 0, nused*sizeof(Zlist*));
    }

    int cnt = 0;
    OPlist *l0 = 0;
    sg = sSelGen(Selections, cursd, "bpw");
    while ((od = sg.next()) != 0) {
        if (!od->is_normal())
            continue;
        if (od->state() == CDSelected && od->ldesc()->isSelectable()) {
            if (!(cnt % 50))
                PL()->ShowPromptV("Working... %6d/%d", cnt, ocnt);
            cnt++;

            if (mode != DRC_BLOAT) {
                Zlist *zx = od->toZlist();
                if (zx) {
                    int ix = od->ldesc()->index(Physical);
                    if (!zheads[ix])
                        zheads[ix] = zx;
                    else {
                        Zlist *ze = zx;
                        while (ze->next)
                            ze = ze->next;
                        ze->next = zheads[ix];
                        zheads[ix] = zx;
                    }
                }
                l0 = new OPlist(od, 0, l0);
            }
            else {
                XIrt ret;
                Zlist *zlist = GEO()->ifBloatObj(od, width, &ret);
                if (ret != XIok) {
                    l0->free();
                    PL()->ErasePrompt();
                    XM()->ShowParameters();
                    dspPkgIf()->SetWorking(false);
                    return (ret);
                }
                l0 = new OPlist(od, zlist->to_poly_list(), l0);
            }
        }
    }
    if (l0) {
        PL()->ShowPrompt("Working... adding objects");
        if (mode != DRC_BLOAT) {
            for (int i = 0; i < nused; i++) {
                if (zheads[i]) {
                    Zlist *zlist = zheads[i];
                    zheads[i] = 0;

                    XIrt ret = Zlist::zl_bloat(&zlist, width, mode);
                    if (ret != XIok) {
                        l0->free();
                        for (i++; i < nused; i++)
                            zheads[i]->free();
                        delete [] zheads;
                        PL()->ErasePrompt();
                        XM()->ShowParameters();
                        dspPkgIf()->SetWorking(false);
                        return (ret);
                    }

                    CDl *ld = CDldb()->layer(i, Physical);
                    for (OPlist *o = l0; o; o = o->next) {
                        if (o->odesc->ldesc() == ld) {
                            if (mode & BL_NO_TO_POLY) {
                                for (Zlist *z = zlist; z; z = z->next) {
                                    Poly po;
                                    if (z->Z.mkpoly(&po.points, &po.numpts,
                                            false))
                                        o->plist = new PolyList(po, o->plist);
                                }
                                zlist->free();
                            }
                            else
                                o->plist = zlist->to_poly_list();
                            break;
                        }
                    }
                }
            }
            delete [] zheads;
        }
        l0->add(cursd, true);
    }
    PL()->ErasePrompt();
    XM()->ShowParameters();
    dspPkgIf()->SetWorking(false);
    return (XIok);
}


// Change selected non-Manhattan polygons and wires to Manhattan
// polygons.
//
XIrt
cEdit::manhattanizeQueue(int minside, int mode)
{
    if (DSP()->CurMode() != Physical)
        return (XIbad);
    CDs *cursd = CurCell(Physical);
    if (!cursd)
        return (XIbad);
    XIrt ret = XIok;
    sSelGen sg(Selections, cursd, "pw");
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (!od->is_normal())
            continue;
        if (od->state() == CDSelected && od->ldesc()->isSelectable()) {
            if (od->type() == CDPOLYGON) {
                if (((const CDpo*)od)->po_is_manhattan())
                    continue;
            }
            else if (od->type() == CDWIRE) {
                if (((const CDw*)od)->w_is_manhattan())
                    continue;
            }
            else
                continue;

            Zlist *zl = od->toZlist();
            zl = zl->manhattanize(minside, mode);
            ret = zl->to_poly_add(cursd, od->ldesc(), true);
            if (ret != XIok)
                break;
            Ulist()->RecordObjectChange(cursd, od, 0);
        }
    }
    return (ret);
}

