
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

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_color.h"
#include "dsp_layer.h"
#include "dsp_tkif.h"
#include "dsp_image.h"
#include "cd_strmdata.h"
#include "cd_lgen.h"
#include "fio_layermap.h"
#include "rgbzimg.h"
#include "timedbg.h"
#include <algorithm>

//#define IMAGE_DBG


// Create and return an image from AOI in the indicated cell, reading
// by use of the chd.  The width and height specify the image size. 
// The hierarchy is read to maxdepth.  Display attributes are obtained
// from the main window.
//
GRimage *
cDisplay::CreateImage(cCHD *chd, const char *cellname, BBox *AOI,
    unsigned int width, unsigned int height, int maxdepth)
{
    if (maxdepth < 0)
        maxdepth = 100;

    WindowDesc wd;
    if (DSP()->MainWdesc())
        *wd.Attrib() = *DSP()->MainWdesc()->Attrib();
    wd.InitViewport(width, height);
    wd.CenterFullView(AOI);
    *wd.ClipRect() = wd.Viewport();

    GRimage *im = wd.CreateChdImage(chd, cellname, AOI, maxdepth);

    // Copy the data in im, which is returned as a pointer to private
    // data in wd that will be freed on function exit!
    im->set_own_data();
    return (im);
}
// End of cDisplay functions.


// Create and return an image containing the rendering within AOI. 
// The drawing is performed in redisplay_geom_direct, so is consistent
// with the window display mode.  In particular, for WDchd, the
// compose_chd_image function is called.
//
// WARNING! The image data are not copied, and become bogus if the
// w_rgbimg changes.  If not used immediately, call set_own_data()
// on the return.
//
GRimage *
WindowDesc::CreateImage(const BBox *AOI, int *numgeom)
{
    Tdbg()->start_timing("CreateImage");
    CD()->ifNoConfirmAbort(true);

    if (!w_rgbimg)
        w_rgbimg = new RGBzimg;

#ifdef IMAGE_DBG
    // The only difference for "oldimage" is that there are separate
    // traversals through the hierarchy for each layer, drawn in
    // sequence.  We still do this in Peek mode.
    w_old_image = CD()->GetVariable("oldimage");
#else
    w_old_image = false;
#endif

    w_rgbimg->Init(w_width, w_height, w_old_image);

    GRdraw *dtmp = w_draw;
    w_draw = w_rgbimg;

    w_draw->SetFillpattern(0);
    w_rgbimg->SetLevel(LV_UNDER);
    w_draw->SetColor(DSP()->Color(BackgroundColor, w_mode));
    w_draw->Box(w_clip_rect.left, w_clip_rect.top,
        w_clip_rect.right, w_clip_rect.bottom);

    // The viewport can be resized during image creation, have to save
    // the current window for the GRimage as the display is munged
    // otherwise.
    int xw = w_width;
    int xh = w_height;
    int ngeom = redisplay_geom_direct(AOI);
    if (numgeom)
        *numgeom = ngeom;

    GRimage *im = new GRimage(xw, xh, w_rgbimg->Map(), true,
        w_rgbimg->shmid());

    w_draw = dtmp;

    CD()->ifNoConfirmAbort(false);
    Tdbg()->stop_timing("CreateImage");

    return (im);
}


// Render the geometry of the hierarchy under cellname in chd within
// AOI to maxdepth.  Return an image struct containing the image.
//
// WARNING! The image data are not copied, and become bogus if the
// w_rgbimg changes.  If not used immediately, call set_own_data()
// on the return.
//
GRimage *
WindowDesc::CreateChdImage(cCHD *chd, const char *cellname, const BBox *AOI,
    int maxdepth)
{
    if (!chd)
        return (0);

    Tdbg()->start_timing("RenderImage");
    CD()->ifNoConfirmAbort(true);

    if (!w_rgbimg)
        w_rgbimg = new RGBzimg;
    w_old_image = false;

    w_rgbimg->Init(w_width, w_height, w_old_image);

    GRdraw *dtmp = w_draw;
    w_draw = w_rgbimg;

    w_draw->SetFillpattern(0);
    w_rgbimg->SetLevel(LV_UNDER);
    w_draw->SetColor(DSP()->Color(BackgroundColor, w_mode));
    w_draw->Box(w_clip_rect.left, w_clip_rect.top,
        w_clip_rect.right, w_clip_rect.bottom);

    // The viewport can be resized during image creation, have to save
    // the current window for the GRimage as the display is munged
    // otherwise.
    int xw = w_width;
    int xh = w_height;
    int ngeom = compose_chd_image(chd, cellname, AOI, maxdepth);
    w_draw = dtmp;

    GRimage *im = ngeom ? new GRimage(xw, xh, w_rgbimg->Map(), true,
        w_rgbimg->shmid()) : 0;

    CD()->ifNoConfirmAbort(false);
    Tdbg()->stop_timing("CreateImage");

    return (im);
}


// Private function to do the work in CHD image creation.
//
int
WindowDesc::compose_chd_image(cCHD *chd, const char *cellname, const BBox *AOI,
    int maxdepth)
{
    if (!chd)
        return (0);
    if (!w_using_image || !w_rgbimg)
        return (0);
    Tdbg()->start_timing("compose_chd_image");

    FIOcvtPrms prms;
    prms.set_use_window(true);
    prms.set_clip(true);

    // In hard-copy mode when the image is being rotated, the CDtfRegI0
    // register contains the rotation, and the AOI has been rotated. 
    // Here, we need the un-rotated AOI to pass to readFlat, since
    // area filtering is done before the transformation.

    if (DSP()->DoingHcopy()) {
        BBox xBB(*AOI);
        DSP()->TPush();
        DSP()->TLoad(CDtfRegI0);
        DSP()->TInverse();
        DSP()->TInverseBB(&xBB, 0);
        DSP()->TPop();
        prms.set_window(&xBB);
    }
    else
        prms.set_window(AOI);

    DSP()->SetMinCellWidth((int)(DSP()->CellThreshold()/w_ratio));

    // Need this to show subthreshold boxes.
    if (maxdepth < 0 && DSP()->MinCellWidth() > 0)
        maxdepth = 100;

    int numgeom = 0;
    zimg_backend ib(this, w_rgbimg);

    Tdbg()->start_timing("read_geometry");
    FIO()->SetFlatReadFeedback(true);
    FIO()->SetCgdSkipInvisibleLayers(true);
    OItype oiret = chd->readFlat(cellname, &prms, &ib, maxdepth,
        DSP()->MinCellWidth(), true);
    FIO()->SetCgdSkipInvisibleLayers(false);
    FIO()->SetFlatReadFeedback(false);
    Tdbg()->stop_timing("read_geometry");

    if (oiret == OIerror) {
        ShowGrid();
        goto error;
    }
    if (oiret == OIaborted)
        DSP()->show_message("Redraw interrupted!");
        // Go ahead and draw whatever has been done.
    numgeom = ib.numgeom(0);
    if (w_mode == Electrical ||
            !w_attributes.grid(w_mode)->show_on_top()) {
        w_rgbimg->SetLevel(LV_UNDER);
        ShowGrid();
    }

    if (!w_attributes.no_show_unexpand(w_mode) && maxdepth >= 0) {
        Tdbg()->start_timing("read_bounds");
        symref_t *p = chd->findSymref(cellname, w_mode, true);
        bnd_draw_t bd(this, chd, AOI);
        w_rgbimg->SetLevel(LV_HLITE);
        if (!show_boundaries(p, &bd, maxdepth)) {
            if (bd.aborted())
                DSP()->show_message("Redraw interrupted!");
            else
                goto error;
        }
        numgeom += bd.numgeom();
        Tdbg()->stop_timing("read_bounds");
    }
    if (w_mode == Physical) {
        if (w_attributes.grid(w_mode)->show_on_top()) {
            w_rgbimg->SetLevel(LV_OVER_LAYERS);
            ShowGrid();
        }
        else {
            w_rgbimg->SetLevel(LV_OVER_ALL);
            ShowAxes(DISPLAY);
        }
    }

    Tdbg()->stop_timing("compose_chd_image", numgeom);
    return (numgeom);

error:
    const char *s = Errs()->get_error();
    // If we've scrolled the cell off-screen, we get here with an
    // error complaining about no intersection area.  Just ignore
    // this.  we've already drawn the grid.

    if (!strstr(s, "intersection area")) {
        int len = 64 + (s ? strlen(s) : 0);
        char *t = new char[len];
        sprintf(t, "Error occurred when reading image data:\n%s", s);
        DSP()->show_message(t, true);
        delete [] t;
    }
    Tdbg()->stop_timing("compose_chd_image");
    return (0);
}


// Draw the unexpanded cells, and "tiny" boxes in the hierarchy of p.
//
bool
WindowDesc::show_boundaries(symref_t *p, bnd_draw_t *bd, int hlev)
{
    // If hlev < 0, means "all levels", can only happen if passed in
    // at the top level.  Unless we're drawing "tiny boxes", there is
    // nothing to do.
    if (hlev < 0 && DSP()->MinCellWidth() == 0)
        return (true);

    crgen_t gen(bd->ntab(), p);
    const cref_o_t *c;
    while ((c = gen.next()) != 0) {
        if (bd->aborted())
            return (false);
        symref_t *px = bd->ntab()->find_symref(c->srfptr);
        const BBox *bbp = px->get_bb();
        if (!bbp)
            return (false);
        CDattr at;
        if (!CD()->FindAttr(c->attr, &at)) {
            Errs()->add_error(
                "show_boundaries: unresolved transform ticket %d", c->attr);
            return (false);
        }
        DSP()->TPush();
        DSP()->TApply(c->tx, c->ty, at.ax, at.ay, at.magn, at.refly);
        DSP()->TPremultiply();

        unsigned int x1, x2, y1, y2;
        if (DSP()->TOverlap(bbp, bd->AOI(), at.nx, at.dx, at.ny, at.dy,
                &x1, &x2, &y1, &y2)) {

            if (hlev == 0) {
                BBox BB = *bbp;
                if (at.nx > 1) {
                    if (at.dx > 0)
                        BB.right += (at.nx - 1)*at.dx;
                    else
                        BB.left += (at.nx - 1)*at.dx;
                }
                if (at.ny > 1) {
                    if (at.dy > 0)
                        BB.top += (at.ny - 1)*at.dy;
                    else
                        BB.bottom += (at.ny - 1)*at.dy;
                }

                w_draw->SetColor(DSP()->Color(InstanceBBColor, w_mode));
                if (at.nx != 1 || at.ny != 1)
                    // Use alternate line style for array BB's
                    ShowBox(&BB, CDL_OUTLINED, 0);
                else
                    ShowBox(&BB, 0, 0);

                if (bd->show_text() != SLnone) {
                    char s1[128], s2[128];
                    const char *mname = Tstring(px->get_name());
                    if (at.nx != 1 || at.ny != 1) {
                        char *s = mmUtoA(s1, at.nx);
                        *s++ = '/';
                        s = mmUtoA(s, at.ny);
                        *s++ = ' ';
                        strcpy(s, mname);
                    }
                    else
                        strcpy(s1, mname);
                    if (w_mode == Physical) {
                        BBox cBB = BB;
                        DSP()->TBB(&cBB, 0);
                        char *s = mmDtoA(s2, MICRONS(cBB.width()), 3, true);
                        *s++ = '/';
                        mmDtoA(s, MICRONS(cBB.height()), 3, true);
                    }
                    else
                        *s2 = 0;
                    ShowUnexpInstanceLabel(
                        at.magn > 0 && at.magn != 1.0 ? &at.magn : 0,
                        &BB, s1, s2, bd->show_text());
                }
                bd->new_geom();
            }
            else {
                int tx, ty;
                DSP()->TGetTrans(&tx, &ty);
                xyg_t xyg(x1, x2, y1, y2);

                do {
                    DSP()->TTransMult(xyg.x*at.dx, xyg.y*at.dy);
                    BBox BB = *bbp;
                    DSP()->TBB(&BB, 0);
                    if (BB.width() < DSP()->MinCellWidth() ||
                            BB.height() < DSP()->MinCellWidth()) {
                        if (bd->show_tiny()) {
                            w_draw->SetColor(DSP()->Color(InstanceBBColor,
                                w_mode));
                            w_rgbimg->SetLevel(LV_UNDER);
                            // Show "tiny boxes"  below layers.
                            ShowBox(bbp, 0, 0);
                            w_rgbimg->SetLevel(LV_HLITE);
                            bd->new_geom();
                        }
                    }
                    else {
                        if (!show_boundaries(px, bd, hlev-1)) {
                            DSP()->TPop();
                            return (false);
                        }
                    }
                    DSP()->TSetTrans(tx, ty);
                } while (xyg.advance());
            }
        }
        DSP()->TPop();
    }
    return (true);
}


// Special rendering functions for use with the RGBzimg in-memory
// image device.  This device prioritizes pixel drawing, equal and
// higher levels will overwrite lower levels, but not the reverse. 
// Thus, as long as we call SetLevel along the way, layers and
// attributes can be drawn in any order.  In particular, we only need
// to traverse the cell hierarchy once.


// Redisplay the CDdb hierarchy using redisplay_rc.  This traverses
// the hierarchy once only, rather than once per layer.  It expects
// the display device to correctly handle layer overlays.
//
int
WindowDesc::redisplay_cddb_zimg(const BBox *AOI)
{
    if (!w_using_image || !w_rgbimg)
        return (0);

    int numgeom = 0;
    if (!w_frozen || DSP()->DoingHcopy()) {
        DSPattrib *a = &w_attributes;
        w_rdl_state state(AOI, a->expand_level(w_mode));

        CDs *sdcur = CurCellDesc(w_mode);
        CDs *sdtop = TopCellDesc(w_mode);
        state.layer = 0;  // not used

        if (a->show_context(w_mode) && (sdcur != sdtop)) {
            DSP()->TPush();
            DSP()->TLoad(CDtfRegI0);
            state.is_context = true;

            // The context is drawn with reduced color intensity,
            // so that the current cell objects are
            // distinguishable.  Update the "dim_pixel" values.
            //
            int p = DSP()->ContextDarkPcnt();
            CDlgen lgen(w_mode);
            CDl *ld;
            while ((ld = lgen.next()) != 0) {
                int dim_pix;
                GRpkgIf()->AllocateColor(&dim_pix,
                    (p*dsp_prm(ld)->red())/100,
                    (p*dsp_prm(ld)->green())/100,
                    (p*dsp_prm(ld)->blue()/100));
                dsp_prm(ld)->set_dim_pixel(dim_pix);
            }

            DSP()->ColorTab()->set_dark(true, w_mode);
            numgeom += redisplay_cddb_zimg_rc(sdtop, 0, &state, false);
            DSP()->TPop();
            DSP()->ColorTab()->set_dark(false, w_mode);
        }
        if (!DSP()->Interrupt()) {
            state.is_context = false;
            numgeom += redisplay_cddb_zimg_rc(sdcur, 0, &state, false);
        }
    }

    if (w_mode == Electrical || !w_attributes.grid(w_mode)->show_on_top()) {
        w_rgbimg->SetLevel(LV_UNDER);
        ShowGrid();
    }
    if (w_mode == Physical) {
        if (w_attributes.grid(w_mode)->show_on_top()) {
            w_rgbimg->SetLevel(LV_OVER_LAYERS);
            ShowGrid();
        }
        else {
            w_rgbimg->SetLevel(LV_OVER_ALL);
            ShowAxes(DISPLAY);
        }
    }
    return (numgeom);
}


namespace {
    inline bool
    set_color(RGBzimg *img, const CDl *ld, DisplayMode mode, bool context,
        bool map_color)
    {
        if (mode == Electrical && ld->isWireActive()) {
            // The electrical active layers are shown above other
            // layers and highlighting.
            img->SetLevel(LV_OVER_HLITE);
            if (map_color) {
                img->SetColor(DSP()->Color(MarkerColor, Electrical));
                return (true);
            }
        }
        else
            img->SetLevel(ld->index(mode));
        if (context)
            img->SetColor(dsp_prm(ld)->dim_pixel());
        else
            img->SetColor(dsp_prm(ld)->pixel());
        return (false);
    }
}


// Paint the objects found, recursively under sdesc.  If symexp, show
// symbolic cell as real data inside symbolic BB.
//
// This loops through the layers in each displayed instance, showing
// all visible objects per-cell.  It is intended for use with the
// RGBzimg to handle ordered layer overlays properly.
//
// The advantage of RGBzimg is that this function recurses through
// the hierarchy once only, other back-ends require a full recurse
// for each layer.
//
int
WindowDesc::redisplay_cddb_zimg_rc(CDs *sdesc, int hierlev,
    WindowDesc::w_rdl_state *state, bool symexp)
{
    if (!sdesc || !w_draw)
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
        CDtf otf, ntf;
        DSP()->TCurrent(&otf);
        magged = syscale(sdesc);
        DSP()->TCurrent(&ntf);
        if (magged) {
            // We're going to show the schematic, so switch.
            sdesc = sdesc->owner();

            // Add wires to terminals, the wires are shown with the
            // highlighting color.
            w_draw->SetColor(DSP()->Color(HighlightingColor, w_mode));
            w_rgbimg->SetLevel(LV_OVER_LAYERS);
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

    int numgeom = 0;
    CDsLgen gen(sdesc);
    CDlgen lgen(w_mode);

    for (;;) {
        CDl *ld;
        if (DSP()->check_ext_display(this)) {

            // Extraction view, the CDsLgen does not know about layers
            // used in the phony cells, call with all conductors and
            // vias.

            ld = lgen.next();
            if (!ld)
                break;
            if (!ld->isConductor() && !ld->isVia())
                continue;
        }
        else {
            ld = gen.next();
            if (!ld)
                break;
        }
        if (ld->isInvisible())
            continue;

        // If the layer has the NoInstView flag set, it is not shown
        // in rendered electrical instances, only when the cell is
        // top-level.
        if (hierlev > 0 && (ld->isNoInstView() && w_mode == Electrical))
            continue;

        InitCache();
        EnableCache();
        if (set_color(w_rgbimg, ld, w_mode, state->is_context,
                state->map_color))
            DisableCache();
        if (!DSP()->ext_display_cell(this, sdesc, ld, &numgeom)) {
            // This is the extraction view diversion, returns true if
            // cell layer geometry was drawn by the extraction system.

            CDg gdesc;
            DSP()->TInitGen(sdesc, ld, state->AOI, &gdesc);

            CDo *odtmp;
            while ((odtmp = gdesc.next()) != 0) {
                // Don't display if conditionally deleted.
                if (odtmp->state() == CDDeleted)
                    continue;
                numgeom++;

                // Test for user interrupt
                if (!(numgeom & 0xff) && numgeom) {
                    // Check every 256 objects for efficiency.
                    dspPkgIf()->CheckForInterrupt();
                    if (DSP()->Interrupt()) {
                        if (magged)
                            DSP()->TPop();
                        DisableCache();
                        return (numgeom);
                    }
                    set_color(w_rgbimg, ld, w_mode, state->is_context,
                        state->map_color);
                }
                if (hierlev > 0 && odtmp->type() == CDLABEL
                        && ((CDla*)odtmp)->no_inst_view())
                    continue;
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
        }
        DisableCache();
        FlushCache();
    }

    w_rgbimg->SetLevel(LV_HLITE);

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
            if (w_mode == Physical &&
                    !DSP()->check_display(this, sdesc, cdesc))
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
                        if (!a->no_show_unexpand(w_mode)) {
                            BB.bloat(DSP()->EmptyCellWidth()/2);
                            w_rgbimg->SetColor(
                                DSP()->Color(HighlightingColor, w_mode));
                            ShowBox(&BB, 0, 0);
                            numgeom++;
                        }
                        continue;
                    }
                    if (dx < DSP()->MinCellWidth() &&
                            dy < DSP()->MinCellWidth()) {
                        if (!a->no_show_unexpand(w_mode) &&
                                a->show_tiny_bb(w_mode)) {
                            // Outline BB of Instance
                            w_rgbimg->SetColor(
                                DSP()->Color(InstanceBBColor, w_mode));
                            w_rgbimg->SetLevel(LV_UNDER);
                            // Show "tiny boxes"  below layers.
                            ShowBox(&BB, 0, 0);
                            w_rgbimg->SetLevel(LV_HLITE);
                            numgeom++;
                        }
                        continue;
                    }
                }

                // Alter the color used to render NOPHYS devs and subckts.
                bool is_nophys;
                if (w_mode == Electrical && cdesc->prpty(P_NOPHYS)) {
                    is_nophys = true;
                    state->map_color++;
                }
                else
                    is_nophys = false;

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
                                w_rgbimg->SetColor(
                                    DSP()->Color(InstanceBBColor, w_mode));
                                BBonly = true;
                            }
                        }
                        if (BBonly) {
                            if (a->show_tiny_bb(w_mode)) {
                                // Show "tiny boxes"  below layers.
                                w_rgbimg->SetLevel(LV_UNDER);
                                ShowBox(pBB, 0, 0);
                                w_rgbimg->SetLevel(LV_HLITE);
                                numgeom++;
                            }
                        }
                        else {
                            int ngeo = redisplay_cddb_zimg_rc(msdesc,
                                hierlev + 1, state,
                                (symbolic && cdesc->has_flag(w_displflag)));
                            w_rgbimg->SetLevel(LV_HLITE);
                            if (ngeo == 0 &&
                                    ( (x1 == x2 ||
                                        (xyg.x > x1 && xyg.x < x2)) &&
                                    (y1 == y2 ||
                                        (xyg.y > y1 && xyg.y < y2)) )) {
                                // If the subcell is entirely visible
                                // and nothing was drawn, skip drawing
                                // any others.
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
            else if (!a->no_show_unexpand(w_mode)) {
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
// End of WindowDesc functions.


//-----------------------------------------------------------------------------
// ximg_backend
//

zimg_backend::zimg_backend(WindowDesc *wdesc, RGBzimg *img)
{
    zb_wdesc = wdesc;
    zb_rgbimg = img;
    zb_layer = 0;
    zb_ltab = 0;
    zb_numgeom = 0;
    zb_allow_layer_mapping = false;
}


zimg_backend::~zimg_backend()
{
    SymTabGen gen(zb_ltab, true);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        delete (zb_layer_t*)h->stData;
        delete h;
    }
    delete zb_ltab;
}


bool
zimg_backend::queue_layer(const Layer *layer, bool*)
{
    if (!layer->name)
        return (false);

    if (!zb_ltab)
        zb_ltab = new SymTab(false, false);

    zb_layer_t *ld = (zb_layer_t*)SymTab::get(zb_ltab, layer->name);
    if (ld != (zb_layer_t*)ST_NIL) {
        zb_layer = ld;
        return (true);
    }

    ld = new zb_layer_t(layer);

    // Presently never set.
    if (zb_allow_layer_mapping) {
        const char *l = FIO()->LayerList();
        ULLtype ull = FIO()->UseLayerList();
        if (l && *l && ull != ULLnoList) {
            Lcheck lcheck((ull == ULLonlyList), l);

            // Throw out skipped layers
            if (!lcheck.layer_ok(ld->lname, ld->layer, ld->datatype)) {
                delete ld;
                ld = 0;
            }
        }

        if (ld && FIO()->IsUseLayerAlias()) {
            const char *la = FIO()->LayerAlias();
            if (la && *la) {
                FIOlayerAliasTab latab;
                latab.parse(la);
                const char *new_lname = latab.alias(layer->name);
                if (new_lname) {
                    int lyr, dt;
                    if (strmdata::hextrn(new_lname, &lyr, &dt) &&
                            lyr >= 0 && dt >= 0) {
                        delete [] ld->lname;
                        ld->lname = lstring::copy(new_lname);
                        ld->layer = lyr;
                        ld->datatype = dt;
                    }
                    else {
                        // Bad alias, ignore the layer.
                        delete ld;
                        ld = 0;
                    }
                }
            }
        }
    }

    CDl *ldesc = 0;
    if (ld->layer >= 0) {
        CDll *layers = FIO()->GetGdsInputLayers(ld->layer, ld->datatype,
            Physical);
        if (layers) {
            ldesc = layers->ldesc;
            CDll::destroy(layers);
        }
        if (!ldesc) {
            bool error;
            ldesc = FIO()->MapGdsLayer(ld->layer, ld->datatype, Physical, 0,
                true, &error);
        }
    }
    if (!ldesc)
        ldesc = CDldb()->newLayer(layer->name, Physical);
    if (ldesc) {
        ld->index = ldesc->index(Physical);
        ld->flags = ldesc->getAttrFlags();
        if (dsp_prm(ldesc)) {
            ld->fill = dsp_prm(ldesc)->fill();
            ld->pixel = dsp_prm(ldesc)->pixel();
        }
        zb_ltab->add(ld->lname, ld, false);
        zb_layer = ld;
        return (true);
    }

    delete ld;
    return (false);
}


bool
zimg_backend::write_box(const BBox *BB)
{
    if (!(zb_numgeom & 0xff)) {
        // check every 256 objects for efficiency
        if (zb_numgeom) {
            dspPkgIf()->CheckForInterrupt();
            if (DSP()->Interrupt()) {
                DSP()->SetInterrupt(DSPinterNone);
                be_abort = true;
                return (false);
            }
        }
    }
    if (zb_layer && !zb_layer->isInvisible() &&
            zb_wdesc->Attrib()->showing_boxes()) {
        zb_rgbimg->SetLevel(zb_layer->index);
        zb_rgbimg->SetColor(zb_layer->pixel);
        zb_wdesc->ShowBox(BB, zb_layer->flags, zb_layer->fill);
        zb_numgeom++;
    }
    return (true);
}


bool
zimg_backend::write_poly(const Poly *poly)
{
    if (!(zb_numgeom & 0xff)) {
        // check every 256 objects for efficiency
        if (zb_numgeom) {
            dspPkgIf()->CheckForInterrupt();
            if (DSP()->Interrupt()) {
                DSP()->SetInterrupt(DSPinterNone);
                be_abort = true;
                return (false);
            }
        }
    }
    if (zb_layer && !zb_layer->isInvisible() &&
            zb_wdesc->Attrib()->showing_polys()) {
        zb_rgbimg->SetLevel(zb_layer->index);
        zb_rgbimg->SetColor(zb_layer->pixel);
        zb_wdesc->ShowPolygon(poly, zb_layer->flags, zb_layer->fill, 0);
        zb_numgeom++;
    }
    return (true);
}


bool
zimg_backend::write_wire(const Wire *wire)
{
    if (!(zb_numgeom & 0xff)) {
        // check every 256 objects for efficiency
        if (zb_numgeom) {
            dspPkgIf()->CheckForInterrupt();
            if (DSP()->Interrupt()) {
                DSP()->SetInterrupt(DSPinterNone);
                be_abort = true;
                return (false);
            }
        }
    }
    if (zb_layer && !zb_layer->isInvisible() &&
            zb_wdesc->Attrib()->showing_wires()) {
        zb_rgbimg->SetLevel(zb_layer->index);
        zb_rgbimg->SetColor(zb_layer->pixel);
        zb_wdesc->ShowWire(wire, zb_layer->flags, zb_layer->fill);
        zb_numgeom++;
    }
    return (true);
}


bool
zimg_backend::write_text(const Text *text)
{
    if (!(zb_numgeom & 0xff)) {
        // check every 256 objects for efficiency
        if (zb_numgeom) {
            dspPkgIf()->CheckForInterrupt();
            if (DSP()->Interrupt()) {
                DSP()->SetInterrupt(DSPinterNone);
                be_abort = true;
                return (false);
            }
        }
    }
    if (zb_layer && !zb_layer->isInvisible() &&
            zb_wdesc->Attrib()->display_labels(zb_wdesc->Mode())) {
        zb_rgbimg->SetLevel(zb_layer->index);
        zb_rgbimg->SetColor(zb_layer->pixel);
        zb_wdesc->ShowLabel(text->text, text->x, text->y,
            text->width, text->height, text->xform, 0);
        zb_numgeom++;
    }
    return (true);
}

