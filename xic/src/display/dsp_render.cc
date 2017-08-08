
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

#include "config.h"
#include "dsp.h"
#include "dsp_window.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "cd_sdb.h"
#include "cd_lgen.h"
#include "cd_digest.h"
#include "cd_propnum.h"
#include "fio.h"
#include "fio_chd.h"
#include "miscutil/timer.h"


//
// Private rendering functions.  See dsp_image.cc for rendering
// functions specialized for in-memory images.
//

namespace {
    // Return a number giving an indication of the complexity of the
    // hierarchy to render, up to limit.  This is basically the number
    // of objects needed to render the full hierarchy.
    //
    unsigned int
    complexity(CDs *sdesc, unsigned int limit)
    {
        unsigned int ngm = 0;
        CDm_gen mgen(sdesc, GEN_MASTERS);
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            if (DSP()->IsInvisible(mdesc))
                continue;
            CDs *msdesc = mdesc->celldesc();
            if (!msdesc)
                continue;
            int n = 1 + complexity(msdesc, limit);
            CDc_gen cgen(mdesc);
            for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
                CDap ap(cdesc);
                ngm += n*ap.nx*ap.ny;
                if (ngm > limit)
                    return (ngm);
            }
        }
        CDl *ld;
        CDsLgen lgen(sdesc);
        while ((ld = lgen.next()) != 0) {
            if (ld->isInvisible())
                continue;
            CDg gdesc;
            gdesc.init_gen(sdesc, ld);
            while (gdesc.next() != 0) {
                ngm++;
                if (ngm > limit)
                    return (ngm);
            }
        }
        return (ngm);
    }
}


// Redisplay the geometry, core function.  Return the number of objects
// rendered.
//
int
WindowDesc::redisplay_geom(const BBox *AOI, bool no_clear)
{
    if (!w_draw)
        return (0);

    // Use backing pixmap?
    w_using_pixmap =
        !DSP()->NoPixmapStore() && !DSP()->SlowMode() && !w_frozen;
    if (w_using_pixmap)
        SwitchToPixmap();

    w_using_image = !DSP()->NoLocalImage() && w_using_pixmap;

    if (w_using_image) {
        // Estimate the possible complexity of the display.  If below
        // threshold, don't use a local image, as this is faster. 
        // Transferring an image is slower than transferring the
        // commands to compose the image in this case.  For complex
        // images, the reverse is true, dramatically so over a
        // network.

#define GEO_THRESHOLD 1000
        if (!w_top_cellname)
            w_using_image = false;
        else {
            CDs *sdtop = TopCellDesc(w_mode);
            if (!sdtop || complexity(sdtop, GEO_THRESHOLD) < GEO_THRESHOLD)
                w_using_image = false;
        }
    }

    int numgeom = 0;
    if (w_using_image) {
        GRimage *image = CreateImage(AOI, &numgeom);
        if (image) {
            w_draw->DisplayImage(image, w_clip_rect.left, w_clip_rect.top,
                w_clip_rect.width() + 1, abs(w_clip_rect.height()) + 1);
            delete image;
        }
    }
    else {
        if (!no_clear) {
            w_draw->SetFillpattern(0);
            w_draw->SetColor(DSP()->Color(BackgroundColor, w_mode));
            w_draw->Box(w_clip_rect.left, w_clip_rect.top,
                w_clip_rect.right, w_clip_rect.bottom);
            if (DSP()->SlowMode())
                // Make sure that the initial cleared state shows.
                w_draw->Update();
        }
        if (w_dbtype == WDchd) {
            // CHD drawing always uses local image.
            w_using_image = true;
            GRimage *image = CreateImage(AOI, &numgeom);
            if (image) {
                w_draw->DisplayImage(image, w_clip_rect.left, w_clip_rect.top,
                    w_clip_rect.width() + 1, abs(w_clip_rect.height()) + 1);
                delete image;
            }
        }
        else
            numgeom = redisplay_geom_direct(AOI);
    }

    if (w_using_pixmap)
        SwitchFromPixmap(&w_clip_rect);
    return (numgeom);
}


// Directly render all non-highlighted features contained in AOI.
//
int
WindowDesc::redisplay_geom_direct(const BBox *AOI)
{
    if (!w_draw)
        return (0);
    DSP()->SetTransformOverflow(false);

    if (w_mode == Physical)
        DSP()->SetMinCellWidth((int)(DSP()->CellThreshold()/w_ratio));
    else
        DSP()->SetMinCellWidth((int)(1.0/w_ratio));
    DSP()->SetEmptyCellWidth((int)(DSP_DEF_CELL_THRESHOLD/w_ratio));

    int numgeom = 0;
    if (w_dbtype == WDcddb) {
        if (w_using_image && !w_old_image)
            numgeom = redisplay_cddb_zimg(AOI);
        else
            numgeom = redisplay_cddb(AOI);
    }
    else if (w_dbtype == WDchd)
        numgeom = redisplay_chd(AOI);
    else if (w_dbtype == WDblist)
        numgeom = redisplay_blist(AOI);
    else if (w_dbtype == WDsdb)
        numgeom = redisplay_sdb(AOI);

    if (DSP()->TransformOverflow()) {
        if (this == DSP()->MainWdesc())
            DSP()->notify_stack_overflow();
        DSP()->SetTransformOverflow(false);
    }
    return (numgeom);
}


int
WindowDesc::redisplay_cddb(const BBox *AOI)
{
    if (w_mode == Electrical || !w_attributes.grid(w_mode)->show_on_top())
        ShowGrid();

    int numgeom = 0;
    if (!w_frozen || DSP()->DoingHcopy()) {
        DSPattrib *a = &w_attributes;
        w_rdl_state state(AOI, a->expand_level(w_mode));

        if (w_mode == Electrical) {

            CDs *sdcur = CurCellDesc(Electrical);
            CDs *sdtop = TopCellDesc(Electrical);

            // Show the layers other than active layers, active layers
            // are drawn last.
            CDlgen lgen(Electrical);
            CDl *ld;
            while ((ld = lgen.next()) != 0) {
                if (ld->isInvisible())
                    continue;
                if (ld->isWireActive())
                    continue;
                state.layer = ld;
                if (a->show_context(w_mode) && (sdcur != sdtop)) {
                    DSP()->TPush();
                    DSP()->TLoad(CDtfRegI0);
                    state.is_context = true;

                    // The context is drawn with reduced color
                    // intensity, so that the current cell objects are
                    // distinguishable.  Change the color pixel of the
                    // layer temporarily.
                    //
                    int p = DSP()->ContextDarkPcnt();
                    int pix = dsp_prm(ld)->pixel();
                    int dark_pix;
                    GRpkgIf()->AllocateColor(&dark_pix,
                        (p*dsp_prm(ld)->red())/100,
                        (p*dsp_prm(ld)->green())/100,
                        (p*dsp_prm(ld)->blue()/100));

                    dsp_prm(ld)->set_pixel(dark_pix);
                    numgeom += redisplay_layer(sdtop, &state);
                    dsp_prm(ld)->set_pixel(pix);

                    DSP()->TPop();
                    if (DSP()->Interrupt())
                        break;
                }
                state.is_context = false;
                numgeom += redisplay_layer(sdcur, &state);
                if (DSP()->Interrupt())
                    break;
            }

            // Call with ld = 0 to draw cell boundaries, etc.
            if (!DSP()->Interrupt()) {
                state.layer = 0;
                if (a->show_context(w_mode) && (sdcur != sdtop)) {
                    DSP()->TPush();
                    DSP()->TLoad(CDtfRegI0);
                    state.is_context = true;
                    DSP()->ColorTab()->set_dark(true, w_mode);
                    numgeom += redisplay_layer(sdtop, &state);
                    DSP()->ColorTab()->set_dark(false, w_mode);

                    DSP()->TPop();
                }
                if (!DSP()->Interrupt()) {
                    state.is_context = false;
                    numgeom += redisplay_layer(sdcur, &state);
                }
            }

            // The active layer is always visible.
            if (!DSP()->Interrupt()) {
                CDlgen gen(Electrical);
                while ((ld = gen.next()) != 0) {
                    if (!ld->isWireActive())
                        continue;
                    state.layer = ld;
                    if (a->show_context(w_mode) && (sdcur != sdtop)) {
                        DSP()->TPush();
                        DSP()->TLoad(CDtfRegI0);
                        state.is_context = true;

                        // The context is drawn with reduced color
                        // intensity, so that the current cell objects are
                        // distinguishable.  Change the color pixel of the
                        // layer temporarily.
                        //
                        int p = DSP()->ContextDarkPcnt();
                        int pix = dsp_prm(state.layer)->pixel();
                        int dark_pix;
                        GRpkgIf()->AllocateColor(&dark_pix,
                            (p*dsp_prm(state.layer)->red())/100,
                            (p*dsp_prm(state.layer)->green())/100,
                            (p*dsp_prm(state.layer)->blue()/100));

                        DSP()->ColorTab()->set_dark(true, w_mode);
                        dsp_prm(state.layer)->set_pixel(dark_pix);
                        numgeom += redisplay_layer(sdtop, &state);
                        dsp_prm(state.layer)->set_pixel(pix);
                        DSP()->ColorTab()->set_dark(false, w_mode);

                        DSP()->TPop();
                    }
                    if (!DSP()->Interrupt()) {
                        state.is_context = false;
                        numgeom += redisplay_layer(sdcur, &state);
                    }
                }
            }
        }
        else {

            CDs *sdcur = CurCellDesc(Physical);
            CDs *sdtop = w_top_cellname == w_cur_cellname ? sdcur :
                TopCellDesc(Physical);

            if (DSP()->SlowMode()) {
                // Show the initial cleared state.
                cTimer::milli_sleep(DSP()->SleepTimeMs());
                dspPkgIf()->CheckForInterrupt();
                w_draw->Update();
                if (DSP()->Interrupt())
                    goto finished;
            }

            // Show the layers
            CDlgen lgen(Physical);
            CDl *ld;
            sLstr *lstr = 0;
            while ((ld = lgen.next()) != 0) {
                if (ld->isInvisible())
                    continue;

                state.layer = ld;
                state.has_geom = false;
                state.check_geom = DSP()->SlowMode();
                if (a->show_context(w_mode) && (sdcur != sdtop)) {
                    DSP()->TPush();
                    DSP()->TLoad(CDtfRegI0);
                    state.is_context = true;

                    // The context is drawn with reduced color
                    // intensity, so that the current cell objects are
                    // distinguishable.  Change the color pixel of the
                    // layer temporarily.
                    //
                    int p = DSP()->ContextDarkPcnt();
                    int pix = dsp_prm(ld)->pixel();
                    int dark_pix;
                    GRpkgIf()->AllocateColor(&dark_pix,
                        (p*dsp_prm(ld)->red())/100,
                        (p*dsp_prm(ld)->green())/100,
                        (p*dsp_prm(ld)->blue()/100));

                    dsp_prm(ld)->set_pixel(dark_pix);
                    numgeom += redisplay_layer(sdtop, &state);
                    dsp_prm(ld)->set_pixel(pix);

                    DSP()->TPop();
                }
                if (!DSP()->Interrupt()) {
                    state.is_context = false;
                    numgeom += redisplay_layer(sdcur, &state);
                }
                if (DSP()->SlowMode() && state.has_geom) {
                    state.check_geom = false;
                    if (!lstr) {
                        lstr = new sLstr;
                        lstr->add("Layers found:");
                    }
                    lstr->add_c(' ');
                    lstr->add(ld->name());
                    DSP()->show_message(lstr->string());
                    cTimer::milli_sleep(DSP()->SleepTimeMs());
                    dspPkgIf()->CheckForInterrupt();
                    w_draw->Update();
                }
                if (DSP()->Interrupt())
                    break;
            }
            delete lstr;

            // Call with ld = 0 to draw cell boundaries, etc
            if (!DSP()->Interrupt()) {
                state.layer = 0;
                if (a->show_context(w_mode) && (sdcur != sdtop)) {
                    DSP()->TPush();
                    DSP()->TLoad(CDtfRegI0);
                    state.is_context = true;
                    DSP()->ColorTab()->set_dark(true, w_mode);
                    numgeom += redisplay_layer(sdtop, &state);
                    DSP()->ColorTab()->set_dark(false, w_mode);
                    DSP()->TPop();
                }
                if (!DSP()->Interrupt()) {
                    state.is_context = false;
                    numgeom += redisplay_layer(sdcur, &state);
                }
            }
        }
    }

finished:
    if (w_mode == Physical) {
        if (w_attributes.grid(w_mode)->show_on_top())
            ShowGrid();
        else
            ShowAxes(DISPLAY);
    }
    return (numgeom);
}


int
WindowDesc::redisplay_chd(const BBox *AOI)
{
    if (!w_dbname)
        return (0);
    cCHD *chd = CDchd()->chdRecall(w_dbname, false);
    if (!chd)
        return (0);
    return (compose_chd_image(chd, w_dbcellname, AOI,
        w_attributes.expand_level(Physical)));
}


// Box list display, for cross section.
//
int
WindowDesc::redisplay_blist(const BBox *AOI)
{
    if (w_mode == Electrical || !w_attributes.grid(w_mode)->show_on_top())
        ShowGrid();

    int numgeom = 0;
    cSDB *tab = CDsdb()->findDB(w_dbname);
    if (tab && tab->table() && tab->type() == sdbBdb) {

        double ys = YScale();
        CDlgen lgen(w_mode);
        CDl *ld;
        while ((ld = lgen.next()) != 0) {
            if (ld->isInvisible())
                continue;
            bdb_t *db = (bdb_t*)
                SymTab::get(tab->table(), (unsigned long)ld);
            if (db == (bdb_t*)ST_NIL)
                continue;
            w_draw->SetColor(dsp_prm(ld)->pixel());
            EnableCache();

            // Make sure that we get all geometry needed for
            // scaled drawing.
            BBox tAOI = *AOI;
            tAOI.bottom = -CDinfinity;
            tAOI.top = CDinfinity;
            int num = db->find_objects(&tAOI);

            for (int i = 0; i < num; i++) {
                BBox BB = *db->objects()[i];
                BB.bottom = mmRnd(ys*BB.bottom);
                BB.top = mmRnd(ys*BB.top);
                if (!(numgeom & 0xff)) {
                    // check every 256 objects for efficiency
                    if (numgeom) {
                        dspPkgIf()->CheckForInterrupt();
                        if (DSP()->Interrupt()) {
                            DisableCache();
                            goto done;
                        }
                        w_draw->SetColor(dsp_prm(ld)->pixel());
                    }
                }
                ShowBox(&BB, ld->getAttrFlags(), dsp_prm(ld)->fill());
                numgeom++;
            }
            DisableCache();
        }
    }

done:
    if (w_mode == Physical) {
        if (w_attributes.grid(w_mode)->show_on_top())
            ShowGrid();
        else
            ShowAxes(DISPLAY);
    }
    return (numgeom);
}


int
WindowDesc::redisplay_sdb(const BBox *AOI)
{
    if (w_mode == Electrical || !w_attributes.grid(w_mode)->show_on_top())
        ShowGrid();

    int numgeom = 0;
    cSDB *tab = CDsdb()->findDB(w_dbname);
    if (tab && tab->table()) {

        CDl *ld;
        CDlgen lgen(w_mode);
        while ((ld = lgen.next()) != 0) {
            if (ld->isInvisible())
                continue;
            if (tab->type() == sdbBdb) {
                bdb_t *db = (bdb_t*)
                    SymTab::get(tab->table(), (unsigned long)ld);
                if (db == (bdb_t*)ST_NIL)
                    continue;
                w_draw->SetColor(dsp_prm(ld)->pixel());
                EnableCache();
                int num = db->find_objects(AOI);
                for (int i = 0; i < num; i++) {
                    // Test for user interrupt
                    if (!(numgeom & 0xff)) {
                        // check every 256 objects for efficiency
                        if (numgeom) {
                            dspPkgIf()->CheckForInterrupt();
                            if (DSP()->Interrupt()) {
                                DisableCache();
                                goto done;
                            }
                            w_draw->SetColor(dsp_prm(ld)->pixel());
                        }
                    }
                    ShowBox(db->objects()[i], ld->getAttrFlags(),
                        dsp_prm(ld)->fill());
                    numgeom++;
                }
                DisableCache();
            }
            else if (tab->type() == sdbOdb) {
                odb_t *db = (odb_t*)
                    SymTab::get(tab->table(), (unsigned long)ld);
                if (db == (odb_t*)ST_NIL)
                    continue;
                w_draw->SetColor(dsp_prm(ld)->pixel());
                EnableCache();
                int num = db->find_objects(AOI);
                for (int i = 0; i < num; i++) {
                    // Test for user interrupt
                    if (!(numgeom & 0xff)) {
                        // check every 256 objects for efficiency
                        if (numgeom) {
                            dspPkgIf()->CheckForInterrupt();
                            if (DSP()->Interrupt()) {
                                DisableCache();
                                goto done;
                            }
                            w_draw->SetColor(dsp_prm(ld)->pixel());
                        }
                    }
                    Display(db->objects()[i]);
                    numgeom++;
                }
                DisableCache();
            }
            else if (tab->type() == sdbZdb) {
                zdb_t *db = (zdb_t*)
                    SymTab::get(tab->table(), (unsigned long)ld);
                if (db == (zdb_t*)ST_NIL)
                    continue;
                w_draw->SetColor(dsp_prm(ld)->pixel());
                EnableCache();
                int num = db->find_objects(AOI);
                for (int i = 0; i < num; i++) {
                    // Test for user interrupt
                    if (!(numgeom & 0xff)) {
                        // check every 256 objects for efficiency
                        if (numgeom) {
                            dspPkgIf()->CheckForInterrupt();
                            if (DSP()->Interrupt()) {
                                DisableCache();
                                goto done;
                            }
                            w_draw->SetColor(dsp_prm(ld)->pixel());
                        }
                    }
                    Poly po;
                    if (db->objects()[i]->mkpoly(&po.points, &po.numpts)) {
                        ShowPolygon(&po, ld->getAttrFlags(),
                            dsp_prm(ld)->fill(), 0);
                        delete [] po.points;
                    }
                    numgeom++;
                }
                DisableCache();
            }
            else if (tab->type() == sdbZldb) {
            }
            else if (tab->type() == sdbZbdb) {
                zbins_t *db = (zbins_t*)
                    SymTab::get(tab->table(), (unsigned long)ld);
                if (db == (zbins_t*)ST_NIL)
                    continue;
                unsigned int xn, xm, yn, ym;
                if (!db->overlap(AOI, &xn, &xm, &yn, &ym))
                    continue;
                w_draw->SetColor(dsp_prm(ld)->pixel());
                EnableCache();
                for (int i = ym; i >= (int)yn; i--) {
                    for (int j = xn; j <= (int)xm; j++) {
                        Zlist *z0 = db->getZlist(j, i);
                        for (Zlist *z = z0; z; z = z->next) {
                            // Test for user interrupt
                            if (!(numgeom & 0xff)) {
                                // check every 256 objects for efficiency
                                if (numgeom) {
                                    dspPkgIf()->CheckForInterrupt();
                                    if (DSP()->Interrupt()) {
                                        DisableCache();
                                        goto done;
                                    }
                                    w_draw->SetColor(dsp_prm(ld)->pixel());
                                }
                            }
                            Poly po;
                            if (z->Z.mkpoly(&po.points, &po.numpts)) {
                                ShowPolygon(&po, ld->getAttrFlags(),
                                    dsp_prm(ld)->fill(), 0);
                                delete [] po.points;
                            }
                            numgeom++;
                        }
                    }
                }
                DisableCache();
            }
        }
    }

done:
    if (w_mode == Physical) {
        if (w_attributes.grid(w_mode)->show_on_top())
            ShowGrid();
        else
            ShowAxes(DISPLAY);
    }
    return (numgeom);
}


int
WindowDesc::redisplay_layer(CDs *sdesc, w_rdl_state *state)
{
    InitCache();
    int numgeom = redisplay_layer_rc(sdesc, 0, state, false);
    FlushCache();
    return (numgeom);
}


// Paint the objects found on ldesc in AOI, recursively under sdesc.
// If symexp, show symbolic cell as real data inside symbolic BB.
//
// The top_inst overrides symbolic view if not null.
//
int
WindowDesc::redisplay_layer_rc(CDs *sdesc, int hierlev,
    WindowDesc::w_rdl_state *state, bool symexp)
{
    if (!sdesc || !w_draw)
        return (0);

    // If the layer has the no_inst_view flag set, it is not shown in
    // rendered electrical instances, only when the cell is top-level.
    if (state->layer && hierlev > 0 && (state->layer->isNoInstView() &&
            w_mode == Electrical))
        return (0);

    if (DSP()->TFull()) {
        DSP()->SetTransformOverflow(true);
        return (0);
    }
    bool magged = false;
    DSPattrib *a = &w_attributes;
    if (w_mode == Electrical && symexp) {

        // Show expanded symolic view, i.e., a tiny schematic inside
        // of the symbol BB.
        if (state->layer && state->layer->isWireActive() &&
                !state->did_twires) {
            state->did_twires = true;
            CDtf otf, ntf;
            DSP()->TCurrent(&otf);
            magged = syscale(sdesc);
            DSP()->TCurrent(&ntf);
            if (magged) {
                // We're going to show the schematic, so switch.
                sdesc = sdesc->owner();

                // Add wires to terminals.
                w_draw->SetColor(DSP()->Color(HighlightingColor, Electrical));
                CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
                for ( ; pn; pn = pn->next()) {
                    int x1, y1;
                    pn->get_schem_pos(&x1, &y1);
                    DSP()->TPoint(&x1, &y1);
                    LToP(x1, y1, x1, y1);
                    DSP()->TLoadCurrent(&otf);
                    int x1b = x1;
                    int y1b = y1;
                    for (unsigned int ix = 0; ; ix++) {
                        int x2, y2;
                        if (!pn->get_pos(ix, &x2, &y2))
                            break;
                        DSP()->TPoint(&x2, &y2);
                        LToP(x2, y2, x2, y2);
                        if (!cGEO::line_clip(&x1, &y1, &x2, &y2, &w_clip_rect))
                            w_draw->Line(x1, y1, x2, y2);
                        x1 = x1b;
                        y1 = y1b;
                    }
                    DSP()->TLoadCurrent(&ntf);
                }
            }
        }
        else {
            magged = syscale(sdesc);
            if (magged) {
                // We're going to show the schematic, so switch.
                sdesc = sdesc->owner();
            }
        }
    }

    int numgeom = 0;
    if (state->layer) {
        EnableCache();
        if (!DSP()->ext_display_cell(this, sdesc, state->layer, &numgeom)) {
            if (state->map_color) {
                DisableCache();
                w_draw->SetColor(DSP()->Color(MarkerColor, Electrical));
            }
            else
                w_draw->SetColor(dsp_prm(state->layer)->pixel());

            CDg gdesc;
            DSP()->TInitGen(sdesc, state->layer, state->AOI, &gdesc);

            CDo *odtmp;
            while ((odtmp = gdesc.next()) != 0) {
                // Don't display if conditionally deleted
                if (odtmp->state() == CDDeleted)
                    continue;
                numgeom++;
                // Test for user interrupt
                if (!(numgeom & 0xff)) {
                    // check every 256 objects for efficiency
                    if (numgeom) {
                        dspPkgIf()->CheckForInterrupt();
                        if (DSP()->Interrupt()) {
                            if (magged)
                                DSP()->TPop();
                            DisableCache();
                            return (numgeom);
                        }
                        if (state->map_color)
                            w_draw->SetColor(DSP()->Color(MarkerColor,
                                Electrical));
                        else
                            w_draw->SetColor(dsp_prm(state->layer)->pixel());
                    }
                }
                if (hierlev > 0 && odtmp->type() == CDLABEL
                        && ((CDla*)odtmp)->no_inst_view())
                    continue;
                if (state->check_geom && !state->has_geom) {
                    // Peek mode.
                    BBox BBinv = *state->AOI;
                    Point *invpts = 0;
                    DSP()->TInverse();
                    DSP()->TInverseBB(&BBinv, &invpts);
                    if (invpts) {
                        Poly po(5, invpts);
                        state->has_geom = odtmp->intersect(&po, false);
                        delete [] invpts;
                    }
                    else
                        state->has_geom = odtmp->intersect(&BBinv, false);
                }

#ifdef GRP_CMAP_ENABLED
                if (DSP()->UseGrpCmap() && DSP()->GrpCmap()) {
                    gp_rgb *rgb = DSP()->GrpCmap()->find(GROUP(odtmp));
                    int pix;
                    w_draw->DefineColor(&pix, rgb->red, rgb->green, rgb->blue);
                    w_draw->SetColor(pix);
                }
#endif
                Display(odtmp);
            }
            if (state->map_color)
                w_draw->SetColor(dsp_prm(state->layer)->pixel());
        }
        DisableCache();
    }

    int ncells = 0;
    BBox BBinv = *state->AOI;
    Point *invpts = 0;
    DSP()->TInverse();
    DSP()->TInverseBB(&BBinv, &invpts);
    CDm_gen mgen(sdesc, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        if (w_mode == Physical && DSP()->IsInvisible(mdesc))
            continue;
        enum skipType { skipNone, skipMain, skipSymb, skipAll };
        skipType skip = skipNone;
        CDc_gen cgen(mdesc);
        for (CDc *cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
            if (skip == skipAll)
                break;

            // This is for extraction view.
            if (!DSP()->check_display(this, sdesc, cdesc))
                continue;

            const BBox *BBp = &cdesc->oBB();
            if (invpts) {
                Poly po(5, invpts);
                if (!po.intersect(BBp, true))
                    continue;
            }
            else if (!BBinv.intersect(BBp, true))
                continue;

            ncells++;
            if (!(ncells & 0xff))
                // check every 256 objects for efficiency
                dspPkgIf()->CheckForInterrupt();

            // Test for user interrupt
            if (DSP()->Interrupt()) {
                if (magged)
                    DSP()->TPop();
                delete [] invpts;
                return (numgeom);
            }
            if (cdesc->state() == CDDeleted)
                continue;
            if (state->is_context && cdesc == DSP()->context_cell())
                continue;

            CDs *msdesc = cdesc->masterCell();
            bool symbolic = msdesc->isSymbolic() || msdesc->isDevice();
            if (symbolic) {
                if (skip == skipSymb)
                    continue;
            }
            else {
                if (skip == skipMain)
                    continue;
            }

            if (symbolic || state->expand < 0 || state->expand > hierlev ||
                    cdesc->has_flag(w_displflag)) {

                if (!symbolic) {
                    BBox BB = cdesc->oBB();
                    DSP()->TBB(&BB, 0);
                    int dx = BB.width();
                    int dy = BB.height();
                    BB = cdesc->oBB();
                    if (!dx && !dy) {
                        // empty cell
                        if (!state->layer && !a->no_show_unexpand(w_mode)) {
                            BB.bloat(DSP()->EmptyCellWidth()/2);
                            w_draw->SetColor(
                                DSP()->Color(HighlightingColor, w_mode));
                            ShowBox(&BB, 0, 0);
                            numgeom++;
                        }
                        continue;
                    }
                    if (dx < DSP()->MinCellWidth() &&
                            dy < DSP()->MinCellWidth()) {
                        if (!state->layer && !a->no_show_unexpand(w_mode) &&
                                a->show_tiny_bb(w_mode)) {
                            // Outline BB of Instance
                            w_draw->SetColor(
                                DSP()->Color(InstanceBBColor, w_mode));
                            ShowBox(&BB, 0, 0);
                            numgeom++;
                        }
                        continue;
                    }
                }
                else if (!state->layer)
                    continue;

                // Alter the color used to render NOPHYS devs and subckts.
                bool is_nophys = false;
                if (w_mode == Electrical && state->layer &&
                        state->layer->isWireActive() &&
                        cdesc->prpty(P_NOPHYS)) {
                    is_nophys = true;
                    state->map_color++;
                }

                DSP()->TPush();
                unsigned int x1, x2, y1, y2;
                if (DSP()->TOverlapInst(cdesc, state->AOI,
                        &x1, &x2, &y1, &y2)) {
                    CDap ap(cdesc);
                    int tx, ty;
                    DSP()->TGetTrans(&tx, &ty);
                    bool BBonly = false;
                    bool first = true;
                    const BBox *pBB = msdesc->BB();
                    xyg_t xyg(x1, x2, y1, y2);
                    do {
                        DSP()->TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                        BBox BB = *pBB;
                        DSP()->TBB(&BB, 0);
                        if (first) {
                            first = false;
                            int dx = BB.width();
                            int dy = BB.height();
                            if (dx < DSP()->MinCellWidth() ||
                                    dy < DSP()->MinCellWidth()) {
                                if (state->layer)
                                    break;
                                w_draw->SetColor(
                                    DSP()->Color(InstanceBBColor, w_mode));
                                BBonly = true;
                            }
                        }
                        if (BBonly) {
                            if (a->show_tiny_bb(w_mode)) {
                                ShowBox(pBB, 0, 0);
                                numgeom++;
                            }
                        }
                        else {
                            int ngeo = redisplay_layer_rc(msdesc, hierlev + 1,
                                state,
                                (symbolic && cdesc->has_flag(w_displflag)));
                            if (ngeo == 0 &&
                                    ( (x1 == x2 ||
                                        (xyg.x > x1 && xyg.x < x2)) &&
                                    (y1 == y2 ||
                                        (xyg.y > y1 && xyg.y < y2)) )) {
                                // If the subcell is entirely visible
                                // and nothing was drawn, skip drawing
                                // others.
                                if (BB <= *state->AOI) {
                                    if (msdesc->isElectrical()) {
                                        if (msdesc->isDevice())
                                            skip = skipAll;
                                        else if (symbolic) {
                                            if (skip == skipMain)
                                                skip = skipAll;
                                            else
                                                skip = skipSymb;
                                        }
                                        else {
                                            if (skip == skipSymb)
                                                skip = skipAll;
                                            else
                                                skip = skipMain;
                                        }
                                    }
                                    else
                                        skip = skipAll;
                                    break;
                                }
                            }
                            else
                                numgeom += ngeo;
                        }
                        DSP()->TSetTrans(tx, ty);
                    } while (xyg.advance());
                }
                if (is_nophys)
                    state->map_color--;
                DSP()->TPop();
            }
            else if (!state->layer && !a->no_show_unexpand(w_mode)) {
                show_unexpanded_instance(cdesc);
                numgeom++;
            }
        }
    }
    if (magged)
        DSP()->TPop();
    delete [] invpts;
    return (numgeom);
}


// Outline BB of Instance.
// Unexpanded form of an instance array is its transformed symbol BB
// with the name of its master shown in its center.
//
void
WindowDesc::show_unexpanded_instance(const CDc *cdesc)
{
    CDap ap(cdesc);
    unsigned int nx = ap.nx;
    unsigned int ny = ap.ny;
    int dx = ap.dx;
    int dy = ap.dy;

    if (cdesc->oBB().width() == 0 && cdesc->oBB().height() == 0) {
        // empty cell
        BBox BB = cdesc->oBB();
        BB.bloat(DSP()->EmptyCellWidth()/2);
        w_draw->SetColor(
            DSP()->Color(HighlightingColor, w_mode));
        ShowBox(&BB, 0, 0);
        return;
    }

    CDs *msdesc = cdesc->masterCell();
    if (!msdesc)
        return;
    BBox BB = *msdesc->BB();
    if (nx > 1) {
        if (dx > 0)
            BB.right += (nx - 1)*dx;
        else
            BB.left += (nx - 1)*dx;
    }
    if (ny > 1) {
        if (dy > 0)
            BB.top += (ny - 1)*dy;
        else
            BB.bottom += (ny - 1)*dy;
    }

    DSP()->TPush();
    DSP()->TApplyTransform(cdesc);
    DSP()->TPremultiply();
    w_draw->SetColor(DSP()->Color(InstanceBBColor, w_mode));
    if (nx != 1 || ny != 1)
        // use alternate line style for array BB's
        ShowBox(&BB, CDL_OUTLINED, 0);
    else
        ShowBox(&BB, 0, 0);

    // Show terminal locations.
    if (w_mode == Electrical) {
        int delta = (int)(3.0/w_ratio);
        if (delta == 0)
            delta = 1;
        w_draw->SetColor(DSP()->Color(HighlightingColor, w_mode));
        CDp_snode *pn = (CDp_snode*)msdesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            int x, y;
            pn->get_schem_pos(&x, &y);
            ShowLine(x-delta, y, x+delta, y);
            ShowLine(x, y-delta, x, y+delta);
        }
        CDp_bsnode *pb = (CDp_bsnode*)msdesc->prpty(P_BNODE);
        for ( ; pb; pb = pb->next()) {
            int x, y;
            pb->get_schem_pos(&x, &y);
            ShowLine(x-delta, y, x, y+delta);
            ShowLine(x, y+delta, x+delta, y);
            ShowLine(x+delta, y, x, y-delta);
            ShowLine(x, y-delta, x-delta, y);
        }
    }

    if (w_attributes.label_instances(w_mode)) {
        char s1[128], s2[128];
        char *s = s1;
        if (nx != 1 || ny != 1) {
            s = mmUtoA(s, nx);
            *s++ = '/';
            s = mmUtoA(s, ny);
            *s++ = ' ';
        }

        const char *mname = 0;
        if (msdesc->isPCellSubMaster()) {
            // The XICP_PC property contains a string of the form
            // <libmane><cellname><viewname>.  Show the pcell cellname
            // rather than the sub-master cell name.

            CDp *pd = cdesc->prpty(XICP_PC);
            if (!pd)
                pd = msdesc->prpty(XICP_PC);
            if (pd) {
                mname = strchr(pd->string(), '<');
                if (mname)
                    mname = strchr(mname+1, '<');
                if (mname)
                    mname++;
            }
        }
        if (!mname)
            mname = Tstring(cdesc->cellname());
        while (*mname && *mname != '>')
            *s++ = *mname++;
        *s = 0;

        if (w_mode == Physical) {
            const BBox *cBB = &cdesc->oBB();
            s = mmDtoA(s2, MICRONS(cBB->width()), 3, true);
            *s++ = '/';
            mmDtoA(s, MICRONS(cBB->height()), 3, true);
        }
        else
            *s2 = 0;

        CDtx tx(cdesc);
        ShowUnexpInstanceLabel(tx.magn > 0 && tx.magn != 1.0 ? &tx.magn : 0,
            &BB, s1, s2, w_attributes.label_instances(w_mode));
    }
    DSP()->TPop();
}


// Static function.
// Set up a scale so that the cell data are shown inside the BB of the
// symbolic part.  Electrical only, used for redisplay in symbolic-
// expanded mode.
//
bool
WindowDesc::syscale(const CDs *sdesc)
{
    if (!sdesc)
        return (false);
    if (!sdesc->isSymbolic())
        return (false);
    const BBox *BBsym = sdesc->BB();
    const BBox *BB = sdesc->owner()->BB();
    double rl = ((double)BBsym->width())/BB->width();
    double tb = ((double)BBsym->height())/BB->height();
    double mag = mmMin(rl, tb);
    int xx = (int)((BBsym->left + BBsym->right -
        mag*(BB->left + BB->right))/2);
    int yy = (int)((BBsym->bottom + BBsym->top -
        mag*(BB->bottom + BB->top))/2);
    DSP()->TPush();
    DSP()->TTranslate(xx, yy);
    DSP()->TSetMagn(mag);
    DSP()->TPremultiply();
    return (true);
}

