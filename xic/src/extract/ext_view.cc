
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
#include "ext.h"
#include "ext_extract.h"
#include "ext_grpgen.h"
#include "dsp_inlines.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_tkif.h"
#include "cd_lgen.h"
#include "menu.h"
#include "promptline.h"
#include "select.h"
#include "layertab.h"
#include "events.h"


//-----------------------------------------------------------------------------
//
// Extraction View, show current cell with extraction-specific objects.
//
//-----------------------------------------------------------------------------


// Display the objects on ldesc (only) which compose the groups.
//
int
cExt::showCell(WindowDesc *wdesc, const CDs *sdesc, const CDl *ldesc)
{
    cGroupDesc *gd = sdesc->groups();
    if (!gd)
        return (0);
    return (gd->show_objects(wdesc, ldesc));
}


// If groups are being shown, display or erase the groups in all
// windows.
//
void
cExt::showGroups(bool d_or_e)
{
    if (ext_showing_groups) {
        CDs *cursdp = CurCell(Physical);
        if (!cursdp)
            return;
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            WindowDesc *wd;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wd = wgen.next()) != 0)
                gd->show_groups(wd, d_or_e);
        }
    }
}


namespace {

    // Callback returns true if extraction views is enabled and the
    // window should use it.
    bool
    check_ext_view(WindowDesc *wdesc)
    {
        return (EX()->isExtractionView() &&
            wdesc->IsSimilar(Physical, DSP()->MainWdesc()));
    }


    // Callback for physical display rendering.
    //
    bool
    ext_display_cell(WindowDesc *wdesc, const CDs *sdesc, const CDl *ldesc,
        int *numgeom)
    {
        if (wdesc->IsSimilar(Physical, DSP()->MainWdesc())) {
            *numgeom += EX()->showCell(wdesc, sdesc, ldesc);
            return (true);
        }
        return (false);
    }


    // Callback for subcell display enable/disable.  Flattened subcells
    // should return false, since they are part of the parent cell.
    //
    bool
    check_should_display(WindowDesc *wdesc, const CDs *sdesc, const CDc *cdesc)
    {
        if (wdesc->IsSimilar(Physical, DSP()->MainWdesc())) {
            cGroupDesc *gd = sdesc->groups();
            if (!gd)
                return (true);
            return (!gd->in_flatten_list(cdesc));
        }
        return (true);
    }
}


// The VIEXT Extraction View command.  Only the objects used by the
// extraction system are visible in physical mode.  When enabled:
// 1. Use special selection filter.
// 2. Inhibit all physical editing operations.
//
void
cExt::showExtractionView(GRobject caller)
{
    if (!caller)
        return;
    if (Menu()->GetStatus(caller)) {
        if (!ext_extraction_view) {
            CDs *psdesc = CurCell(Physical);
            if (!psdesc) {
                PL()->ShowPrompt("No current physical cell!");
                Menu()->Deselect(caller);
                return;
            }
            // Exit command state.  Editing is forbiddne in Extraction
            // View.
            EV()->InitCallback();

            if (!EX()->associate(psdesc)) {
                PL()->ShowPrompt("Association failed!");
                Menu()->Deselect(caller);
                return;
            }

            DSP()->Register_check_ext_display(check_ext_view);
            DSP()->Register_ext_display_cell(ext_display_cell);
            DSP()->Register_check_display(check_should_display);
            ext_extraction_view = true;
        }
    }
    else {
        if (ext_extraction_view) {
            DSP()->Register_check_ext_display(0);
            DSP()->Register_ext_display_cell(0);
            DSP()->Register_check_display(0);
            ext_extraction_view = false;
            Selections.deselectTypes(CurCell(), 0);
        }
    }
    EX()->PopUpExtSetup(0, MODE_UPD);

    WindowDesc *wd;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wd = wgen.next()) != 0) {
        if (wd->IsSimilar(Physical, DSP()->MainWdesc()))
            wd->Redisplay(0);
    }
}


// Exported selection core for use while Extraction View is active.
//
CDol *
cExt::selectItems(const char *types, const BBox *BB, int psel,
    int asel, int delta)
{
    if (!ext_extraction_view && !ext_extraction_select)
        return (0);

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (0);
    cGroupDesc *gd = cursdp->groups();
    if (!gd)
        return (0);

    // Always ignore labels and instances.
    char stypes[8];
    char *s = stypes;
    if (types) {
        if (strchr(types, CDBOX))
            *s++ = CDBOX;
        if (strchr(types, CDPOLYGON))
            *s++ = CDPOLYGON;
        if (strchr(types, CDWIRE))
            *s++ = CDWIRE;
    }
    *s = 0;
    if (!stypes[0]) {
        *s++ = CDBOX;
        *s++ = CDPOLYGON;
        *s++ = CDWIRE;
        *s = 0;
    }


    CDol *c0 = 0, *ce = 0;
    CDextLgen gen(CDL_CONDUCTOR, Selections.layerSearchUp() ?
        CDextLgen::BotToTop: CDextLgen::TopToBot);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isConductor())
            continue;
        if (ld->isGroundPlane() && ld->isDarkField())
            continue;
        if (ld->isNoSelect())
            continue;
        if (ld->isInvisible())
            continue;

        sGrpGen gdesc;
        gdesc.init_gen(gd, ld, BB);
        if (psel == PSELstrict_area_nt)
            gdesc.setflags(GEN_RET_NOTOUCH);

        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (!odesc->is_normal())
                continue;
            if (!strchr(stypes, odesc->type()))
                continue;
            if (cSelections::processSelect(odesc, BB, (PSELmode)psel,
                    (ASELmode)asel, delta)) {
                if (c0 == 0)
                    c0 = ce = new CDol(odesc, 0);
                else {
                    ce->next = new CDol(odesc, 0);
                    ce = ce->next;
                }
            }
        }
    }
    return (c0);
}
// End of cExt functions.


// Display the objects that compose the groups, on layer ldesc only.
// The number of objects rendered is returned.
//
int
cGroupDesc::show_objects(WindowDesc *wdesc, const CDl *ldesc)
{
    if (!ldesc)
        return (0);
    
    int numgeom = 0;
    if (ldesc->isConductor()) {
        wdesc->Wdraw()->SetColor(dsp_prm(ldesc)->pixel());
        for (int i = 0; i < gd_asize; i++) {
            sGroup &g = gd_groups[i];
            if (g.net()) {
                for (CDol *ol = g.net()->objlist(); ol; ol = ol->next) {
                    if (ldesc && ol->odesc->ldesc() != ldesc)
                        continue;
                    numgeom++;
                    // Test for user interrupt
                    if (!(numgeom & 0xff)) {
                        // check every 256 objects for efficiency
                        if (numgeom) {
                            dspPkgIf()->CheckForInterrupt();
                            if (DSP()->Interrupt())
                                return (numgeom);
                        }
                        wdesc->Wdraw()->SetColor(dsp_prm(ldesc)->pixel());
                    }
                    wdesc->Display(ol->odesc);
                }
            }
        }
    }
    else if (ldesc->isVia()) {
        // Display the vias.
        // There aren't any vias in the extraction data struct, only
        // conductors.  Here, we'll whow the vias from the normal
        // cell, then cycle through the flattened instances and show
        // the vias in those.

        CDg gdesc;
        gdesc.init_gen(gd_celldesc, ldesc);

        CDo *odtmp;
        while ((odtmp = gdesc.next()) != 0) {
            // Don't display if conditionally deleted.
            if (odtmp->state() == CDDeleted)
                continue;
            numgeom++;
            // Test for user interrupt
            if (!(numgeom & 0xff)) {
                // Check every 256 objects for efficiency.
                dspPkgIf()->CheckForInterrupt();
                if (DSP()->Interrupt())
                    return (numgeom);
                wdesc->Wdraw()->SetColor(dsp_prm(ldesc)->pixel());
            }
            wdesc->Display(odtmp);
        }

        SymTabGen gen(gd_flatten_tab);
        SymTabEnt *ent;
        while ((ent = gen.next()) != 0) {
            CDc *cd = (CDc*)ent->stTag;
            CDs *msd = cd->masterCell();
            if (!msd)
                continue;
            DSP()->TPush();
            DSP()->TApplyTransform(cd);
            DSP()->TPremultiply();
            CDap ap(cd);

            int tx, ty;
            DSP()->TGetTrans(&tx, &ty);
            xyg_t xyg(0, ap.nx-1, 0, ap.ny-1);
            do {
                DSP()->TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);

                gdesc.init_gen(msd, ldesc);

                while ((odtmp = gdesc.next()) != 0) {
                    numgeom++;
                    // Test for user interrupt
                    if (!(numgeom & 0xff)) {
                        // Check every 256 objects for efficiency.
                        dspPkgIf()->CheckForInterrupt();
                        if (DSP()->Interrupt()) {
                            DSP()->TPop();
                            return (numgeom);
                        }
                        wdesc->Wdraw()->SetColor(dsp_prm(ldesc)->pixel());
                    }
                    wdesc->Display(odtmp);
                }

                DSP()->TSetTrans(tx, ty);

            } while (xyg.advance());
            DSP()->TPop();
        }
    }
    return (numgeom);
}


// Show/erase the group numbers for all groups with the display flag set.
//
void
cGroupDesc::show_groups(WindowDesc *wdesc, bool d_or_e)
{
    static bool skipit;  // prevent reentrancy
    if (skipit)
        return;
    if (!wdesc->Wdraw())
        return;
    if (!wdesc->IsSimilar(Physical, DSP()->MainWdesc()))
        return;

    if (dspPkgIf()->IsDualPlane())
        wdesc->Wdraw()->SetXOR(d_or_e == DISPLAY ? GRxHlite : GRxUnhlite);
    else {
        if (d_or_e == DISPLAY)
            wdesc->Wdraw()->SetColor(
                DSP()->Color(HighlightingColor, Physical));
        else {
            BBox BB(CDnullBB);
            for (int i = 0; i < gd_asize; i++) {
                if (gd_groups[i].net() && gd_groups[i].displayed())
                    gd_groups[i].show(wdesc, &BB);
            }
            skipit = true;
            wdesc->Redisplay(&BB);
            skipit = false;
            return;
        }
    }
    for (int i = 0; i < gd_asize; i++) {
        if (gd_groups[i].net() && gd_groups[i].displayed())
            gd_groups[i].show(wdesc);
    }
    if (dspPkgIf()->IsDualPlane())
        wdesc->Wdraw()->SetXOR(GRxNone);
    else if (LT()->CurLayer())
        wdesc->Wdraw()->SetColor(dsp_prm(LT()->CurLayer())->pixel());
}


// Set or clear the display flag for all groups containing a net.
//
void
cGroupDesc::set_group_display(bool state)
{
    for (int i = 0; i < gd_asize; i++) {
        if (gd_groups[i].net())
            gd_groups[i].set_displayed(state);
    }
}

