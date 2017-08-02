
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "cd_hypertext.h"
#include "cd_lgen.h"


namespace {
    bool PPgoaway;  // reentrancy control

    void find_loc(CDo*, int*, int*, int);


    // We don't show internal properties.
    //
    inline bool
    is_prop_showable(CDp *pdesc, bool showprp)
    {
        if (!prpty_internal(pdesc->value()))
            return (showprp);
        if (pdesc->value() == XICP_CNDR && DSP()->ShowCndrNumbers())
            return (true);
        return (false);
    }


    // In physical mode properties, long text properties have an
    // identifying prefix.  This returns the symbolic form if the
    // prefix is seen.
    //
    inline const char *
    get_prop_text(CDp *pdesc)
    {
        const char *lttok = HYtokPre HYtokLT HYtokSuf;
        if (pdesc->string() && lstring::prefix(lttok, pdesc->string()))
            return ("[text]");
        return (pdesc->string());
    }
}


// Show or erase the properties (physical mode) of odesc.
//
void
cDisplay::ShowOdescPhysProperties(CDo *odesc, int display)
{
    if (CurMode() != Physical)
        return;
    if (!odesc || !odesc->prpty_list())
        return;

    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wdesc = wgen.next()) != 0) {
        if (!wdesc->Wdraw())
            continue;
        if (wdesc->Mode() != Physical)
            continue;

        bool showprp = wdesc->Attrib()->show_phys_props();
        if (!showprp && !d_show_cnums)
            continue;

        int delta = wdesc->LogScale(d_phys_prop_size);
        if (dspPkgIf()->IsDualPlane())
            wdesc->Wdraw()->SetXOR(display ? GRxHlite : GRxUnhlite);
        else {
            if (display)
                wdesc->Wdraw()->SetColor(Color(HighlightingColor, Physical));
            else {
                BBox BB;
                bool locfound = false;
                for (CDp *pdesc = odesc->prpty_list(); pdesc;
                        pdesc = pdesc->next_prp()) {
                    if (is_prop_showable(pdesc, showprp)) {
                        if (!locfound) {
                            find_loc(odesc, &BB.left, &BB.bottom, delta);
                            BB.right = BB.left;
                            BB.top = BB.bottom;
                            locfound = true;
                        }
                        else
                            BB.bottom -= delta;
                        sLstr lstr;
                        lstr.add_i(pdesc->value());
                        lstr.add_c(' ');
                        lstr.add(get_prop_text(pdesc));
                        int w, h;
                        int nl = DefaultLabelSize(lstr.string(),
                            wdesc->Mode(), &w, &h);
                        double hd = delta*nl;
                        w = mmRnd(w*hd/h);
                        h = mmRnd(hd);
                        BB.bottom -= h - delta;
                        if (BB.right < BB.left + w)
                            BB.right = BB.left + w;
                        if (BB.top < BB.bottom + h)
                            BB.top = BB.bottom + h;
                    }
                }
                PPgoaway = true;
                wdesc->Redisplay(&BB);
                PPgoaway = false;
                return;
            }
        }

        bool erase = d_erase_behind_props;
        bool locfound = false;
        BBox tBB = *wdesc->ClipRect();
        *wdesc->ClipRect() = *wdesc->ClipRect();
        int x, y, w, h;
        for (CDp *pdesc = odesc->prpty_list(); pdesc;
                pdesc = pdesc->next_prp()) {
            if (is_prop_showable(pdesc, showprp)) {
                if (!locfound) {
                    find_loc(odesc, &x, &y, delta);
                    locfound = true;
                }
                sLstr lstr;
                lstr.add_i(pdesc->value());
                lstr.add_c(' ');
                lstr.add(get_prop_text(pdesc));
                int nl = DefaultLabelSize(lstr.string(), wdesc->Mode(),
                    &w, &h);
                double hd = delta*nl;
                w = mmRnd(w*hd/h);
                h = mmRnd(hd);
                y -= h - delta;
                if (erase && display) {
                    if (dspPkgIf()->IsDualPlane())
                        wdesc->Wdraw()->SetXOR(GRxNone);
                    wdesc->Wdraw()->SetColor(DSP()->Color(BackgroundColor,
                        Physical));
                    BBox BB(x, y, x + w, y + h);
                    wdesc->ShowBox(&BB, CDL_FILLED, 0);
                    if (dspPkgIf()->IsDualPlane())
                        wdesc->Wdraw()->SetXOR(GRxHlite);
                    wdesc->Wdraw()->SetColor(Color(HighlightingColor,
                        Physical));
                }
                wdesc->ShowLabel(lstr.string(), x, y, w, h, 0);
                if (erase && !display) {
                    if (dspPkgIf()->IsDualPlane())
                        wdesc->Wdraw()->SetXOR(GRxNone);
                    BBox BB(x, y, x + w, y + h);
                    wdesc->Redisplay(&BB);
                    if (dspPkgIf()->IsDualPlane())
                        wdesc->Wdraw()->SetXOR(GRxUnhlite);
                    wdesc->Wdraw()->SetColor(Color(HighlightingColor,
                        Physical));
                }
                y -= delta;
            }
        }
        *wdesc->ClipRect() = tBB;
        if (dspPkgIf()->IsDualPlane())
            wdesc->Wdraw()->SetXOR(GRxNone);
    }
}
// End of cDisplay functions.


// Function to show the properties in the current cell on-screen in
// physical mode.  The properties of all objects found in AOI are
// shown or erased.
//
void
WindowDesc::ShowPhysProperties(const BBox *AOI, int display)
{
    if (!w_draw)
        return;
    if (!DSP()->CurCellName())
        return;
    if (PPgoaway)
        // don't want te reenter from Redisplay()
        return;
    if (!IsSimilar(Physical, DSP()->MainWdesc()))
        return;
    bool showprp = w_attributes.show_phys_props();
    if (!showprp && !DSP()->ShowCndrNumbers())
        return;
    if (!AOI)
        AOI = &w_window;

    int delta = LogScale(DSP()->PhysPropSize());
    CDs *sdesc = CurCellDesc(Physical);
    if (!sdesc)
        return;

    if (dspPkgIf()->IsDualPlane())
        w_draw->SetXOR(display ? GRxHlite : GRxUnhlite);
    else {
        if (display)
            w_draw->SetColor(DSP()->Color(HighlightingColor, Physical));
        else {
            CDg gdesc;
            DSP()->TInitGen(sdesc, CellLayer(), AOI, &gdesc);
            CDo *pointer;
            bool locfound;
            BBox BB(CDnullBB);
            while ((pointer = gdesc.next()) != 0) {
                if (!pointer->is_normal())
                    continue;
                CDp *pdesc = pointer->prpty_list();
                if (!pdesc)
                    continue;
                locfound = false;
                BBox tBB;
                for ( ; pdesc; pdesc = pdesc->next_prp()) {
                    if (is_prop_showable(pdesc, showprp)) {
                        if (!locfound) {
                            find_loc(pointer, &tBB.left, &tBB.bottom, delta);
                            locfound = true;
                        }
                        sLstr lstr;
                        lstr.add_i(pdesc->value());
                        lstr.add_c(' ');
                        lstr.add(get_prop_text(pdesc));
                        int w, h;
                        int nl = DSP()->DefaultLabelSize(lstr.string(),
                            w_mode, &w, &h);
                        double hd = delta*nl;
                        w = mmRnd(w*hd/h);
                        h = mmRnd(hd);
                        tBB.bottom -= h - delta;
                        tBB.right = tBB.left + w;
                        tBB.top = tBB.bottom + h;
                        BB.add(&tBB);
                        tBB.bottom -= delta;
                    }
                }
            }

            CDl *ld;
            CDsLgen lgen(sdesc);
            lgen.sort();
            while ((ld = lgen.next()) != 0) {
                DSP()->TInitGen(sdesc, ld, AOI, &gdesc);
                while ((pointer = gdesc.next()) != 0) {
                    if (!pointer->is_normal())
                        continue;
                    CDp *pdesc = pointer->prpty_list();
                    if (!pdesc)
                        continue;
                    locfound = false;
                    BBox tBB;
                    for ( ; pdesc; pdesc = pdesc->next_prp()) {
                        if (is_prop_showable(pdesc, showprp)) {
                            if (!locfound) {
                                find_loc(pointer, &tBB.left, &tBB.bottom,
                                    delta);
                                locfound = true;
                            }
                            sLstr lstr;
                            if (pdesc->value() == XICP_CNDR &&
                                    DSP()->ShowCndrNumbers()) {
                                const char *t = get_prop_text(pdesc);
                                lstring::advtok(&t);
                                lstr.add(t);
                            }
                            else {
                                lstr.add_i(pdesc->value());
                                lstr.add_c(' ');
                                lstr.add(get_prop_text(pdesc));
                            }
                            int w, h;
                            int nl = DSP()->DefaultLabelSize(lstr.string(),
                                w_mode, &w, &h);
                            double hd = delta*nl;
                            w = mmRnd(w*hd/h);
                            h = mmRnd(hd);
                            tBB.bottom -= h - delta;
                            tBB.right = tBB.left + w;
                            tBB.top = tBB.bottom + h;
                            BB.add(&tBB);
                            tBB.bottom -= delta;
                        }
                    }
                }
            }
            PPgoaway = true;
            Redisplay(&BB);
            PPgoaway = false;
            return;
        }
    }

    bool erase = DSP()->EraseBehindProps();
    CDg gdesc;
    DSP()->TInitGen(sdesc, CellLayer(), AOI, &gdesc);
    CDo *pointer;
    int x, y, w, h;
    bool locfound;
    BBox tBB;
    while ((pointer = gdesc.next()) != 0) {
        if (!pointer->is_normal())
            continue;
        CDp *pdesc = pointer->prpty_list();
        if (!pdesc)
            continue;
        locfound = false;
        tBB = w_clip_rect;
        w_clip_rect = Viewport();
        for ( ; pdesc; pdesc = pdesc->next_prp()) {
            if (is_prop_showable(pdesc, showprp)) {
                if (!locfound) {
                    find_loc(pointer, &x, &y, delta);
                    locfound = true;
                }
                sLstr lstr;
                lstr.add_i(pdesc->value());
                lstr.add_c(' ');
                lstr.add(get_prop_text(pdesc));
                int nl = DSP()->DefaultLabelSize(lstr.string(), w_mode,
                    &w, &h);
                double hd = delta*nl;
                w = mmRnd(w*hd/h);
                h = mmRnd(hd);
                y -= h - delta;
                if (erase && display) {
                    if (dspPkgIf()->IsDualPlane())
                        w_draw->SetXOR(GRxNone);
                    w_draw->SetColor(DSP()->Color(BackgroundColor, Physical));
                    BBox BB(x, y, x + w, y + h);
                    ShowBox(&BB, CDL_FILLED, 0);
                    if (dspPkgIf()->IsDualPlane())
                        w_draw->SetXOR(GRxHlite);
                    w_draw->SetColor(DSP()->Color(HighlightingColor,
                        Physical));
                }
                ShowLabel(lstr.string(), x, y, w, h, 0);
                if (erase && !display) {
                    if (dspPkgIf()->IsDualPlane())
                        w_draw->SetXOR(GRxNone);
                    BBox BB(x, y, x + w, y + h);
                    PPgoaway = true;
                    Redisplay(&BB);
                    PPgoaway = false;
                    if (dspPkgIf()->IsDualPlane())
                        w_draw->SetXOR(GRxUnhlite);
                    w_draw->SetColor(DSP()->Color(HighlightingColor,
                        Physical));
                }
                y -= delta;
            }
        }
        w_clip_rect = tBB;
    }

    CDl *ld;
    CDsLgen lgen(sdesc);
    lgen.sort();
    while ((ld = lgen.next()) != 0) {
        DSP()->TInitGen(sdesc, ld, AOI, &gdesc);
        while ((pointer = gdesc.next()) != 0) {
            if (!pointer->is_normal())
                continue;
            CDp *pdesc = pointer->prpty_list();
            if (!pdesc)
                continue;
            locfound = false;
            tBB = w_clip_rect;
            w_clip_rect = Viewport();
            for ( ; pdesc; pdesc = pdesc->next_prp()) {
                if (is_prop_showable(pdesc, showprp)) {
                    if (!locfound) {
                        find_loc(pointer, &x, &y, delta);
                        locfound = true;
                    }
                    sLstr lstr;
                    if (pdesc->value() == XICP_CNDR &&
                            DSP()->ShowCndrNumbers()) {
                        const char *t = get_prop_text(pdesc);
                        lstring::advtok(&t);
                        lstr.add(t);
                    }
                    else {
                        lstr.add_i(pdesc->value());
                        lstr.add_c(' ');
                        lstr.add(get_prop_text(pdesc));
                    }
                    int nl = DSP()->DefaultLabelSize(lstr.string(), w_mode,
                        &w, &h);
                    double hd = delta*nl;
                    w = mmRnd(w*hd/h);
                    h = mmRnd(hd);
                    y -= h - delta;
                    if (erase && display) {
                        if (dspPkgIf()->IsDualPlane())
                            w_draw->SetXOR(GRxNone);
                        w_draw->SetColor(DSP()->Color(BackgroundColor,
                            Physical));
                        BBox BB(x, y, x + w, y + h);
                        ShowBox(&BB, CDL_FILLED, 0);
                        if (dspPkgIf()->IsDualPlane())
                            w_draw->SetXOR(GRxHlite);
                        w_draw->SetColor(DSP()->Color(HighlightingColor,
                            Physical));
                    }
                    ShowLabel(lstr.string(), x, y, w, h, 0);
                    if (erase && !display) {
                        if (dspPkgIf()->IsDualPlane())
                            w_draw->SetXOR(GRxNone);
                        BBox BB(x, y, x + w, y + h);
                        PPgoaway = true;
                        Redisplay(&BB);
                        PPgoaway = false;
                        if (dspPkgIf()->IsDualPlane())
                            w_draw->SetXOR(GRxUnhlite);
                        w_draw->SetColor(DSP()->Color(HighlightingColor,
                            Physical));
                    }
                    y -= delta;
                }
            }
            w_clip_rect = tBB;
        }
    }
    if (dspPkgIf()->IsDualPlane())
        w_draw->SetXOR(GRxNone);
}
// End of WindowDesc functions.


namespace {
    // Return screen coordinates for physical properties of odesc.
    // Arg delta is approx. text height.
    //
    void
    find_loc(CDo *odesc, int *x, int *y, int delta)
    {
        if (odesc->type() == CDWIRE) {
            // leftmost end
            const Point *pts = ((CDw*)odesc)->points();
            int num = ((CDw*)odesc)->numpts();
            int wid = ((CDw*)odesc)->wire_width()/2;
            if (pts[0].x < pts[num-1].x || (pts[0].x == pts[num-1].x &&
                    pts[0].y > pts[num-1].y)) {
                *x = pts[0].x - wid;
                *y = pts[0].y + wid - delta;
            }
            else {
                *x = pts[num-1].x - wid;
                *y = pts[num-1].y + wid - delta;
            }
        }
        else if (odesc->type() == CDPOLYGON) {
            // leftmost vertex with largest y
            const Point *pts = ((CDpo*)odesc)->points();
            int num = ((CDpo*)odesc)->numpts();
            int minx = CDinfinity;
            int maxy = -CDinfinity;
            int iref = 0;
            for (int i = 0; i < num; i++) {
                if (pts[i].x < minx || (pts[i].x == minx && pts[i].y > maxy)) {
                    minx = pts[i].x;
                    maxy = pts[i].y;
                    iref = i;
                }
            }
            *x = pts[iref].x;
            *y = pts[iref].y - delta;
        }
        else {
            // upper left corner
            *x = odesc->oBB().left;
            *y = odesc->oBB().top - delta;
        }
    }
}

