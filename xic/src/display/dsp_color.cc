
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
#include "dsp_inlines.h"
#include "dsp_tkif.h"


// Called from constructor.
//
void
cDisplay::createColorTable()
{
    d_color_table = new sColorTab;
}
// End of cDisplay functions


sColorTab::sColorTab()
{
    use_dark = false;
    sColorTabEnt *c = colors;

    // Display attributes (mode dependent)
    new(c + BackgroundColor) sColorTabEnt(this, "BackgroundColor",
        0, "black",             0, 0, 0, 0, false);
    new(c + ElecBackgroundColor) sColorTabEnt(this, "ElecBackgroundColor",
        0, "black",             0, 0, 0, 0, false);
    new(c + PhysBackgroundColor) sColorTabEnt(this, "PhysBackgroundColor",
        0, "black",             0, 0, 0, 0, false);
    new(c + GhostColor) sColorTabEnt(this, "GhostColor",
        0, "white",             0, 0, 0, 0, false);
    new(c + ElecGhostColor) sColorTabEnt(this, "ElecGhostColor",
        0, "white",             0, 0, 0, 0, false);
    new(c + PhysGhostColor) sColorTabEnt(this, "PhysGhostColor",
        0, "white",             0, 0, 0, 0, false);
    //
    new(c + HighlightingColor) sColorTabEnt(this, "HighlightingColor",
        0, "white",             0, 0, 0, 0, false);
    new(c + ElecHighlightingColor) sColorTabEnt(this, "ElecHighlightingColor",
        0, "white",             0, 0, 0, 0, false);
    new(c + PhysHighlightingColor) sColorTabEnt(this, "PhysHighlightingColor",
        0, "white",             0, 0, 0, 0, false);
    //
    new(c + SelectColor1) sColorTabEnt(this, "SelectColor1",
        0, "white",             0, 0, 0, 0, false);
    new(c + ElecSelectColor1) sColorTabEnt(this, "ElecSelectColor1",
        0, "white",             0, 0, 0, 0, false);
    new(c + PhysSelectColor1) sColorTabEnt(this, "PhysSelectColor1",
        0, "white",             0, 0, 0, 0, false);
    //
    new(c + SelectColor2) sColorTabEnt(this, "SelectColor2",
        0, "pink",              0, 0, 0, 0, false);
    new(c + ElecSelectColor2) sColorTabEnt(this, "ElecSelectColor2",
        0, "pink",              0, 0, 0, 0, false);
    new(c + PhysSelectColor2) sColorTabEnt(this, "PhysSelectColor2",
        0, "pink",              0, 0, 0, 0, false);
    //
    new(c + MarkerColor) sColorTabEnt(this, "MarkerColor",
        0, "yellow",            0, 0, 0, 0, false);
    new(c + ElecMarkerColor) sColorTabEnt(this, "ElecMarkerColor",
        0, "yellow",            0, 0, 0, 0, false);
    new(c + PhysMarkerColor) sColorTabEnt(this, "PhysMarkerColor",
        0, "yellow",            0, 0, 0, 0, false);
    //
    new(c + InstanceBBColor) sColorTabEnt(this, "InstanceBBColor",
        0, "turquoise",         0, 0, 0, 0, false);
    new(c + ElecInstanceBBColor) sColorTabEnt(this, "ElecInstanceBBColor",
        0, "turquoise",         0, 0, 0, 0, false);
    new(c + PhysInstanceBBColor) sColorTabEnt(this, "PhysInstanceBBColor",
        0, "turquoise",         0, 0, 0, 0, false);
    //
    new(c + InstanceNameColor) sColorTabEnt(this, "InstanceNameColor",
        0, "pink",              0, 0, 0, 0, false);
    new(c + ElecInstanceNameColor) sColorTabEnt(this, "ElecInstanceNameColor",
        0, "pink",              0, 0, 0, 0, false);
    new(c + PhysInstanceNameColor) sColorTabEnt(this, "PhysInstanceNameColor",
        0, "pink",              0, 0, 0, 0, false);
    //
    new(c + InstanceSizeColor) sColorTabEnt(this, "InstanceSizeColor",
        0, "salmon",            0, 0, 0, 0, false);
    //
    new(c + CoarseGridColor) sColorTabEnt(this, "CoarseGridColor",
        0, "skyblue",           0, 0, 0, 0, false);
    new(c + ElecCoarseGridColor) sColorTabEnt(this, "ElecCoarseGridColor",
        0, "skyblue",           0, 0, 0, 0, false);
    new(c + PhysCoarseGridColor) sColorTabEnt(this, "PhysCoarseGridColor",
        0, "skyblue",           0, 0, 0, 0, false);
    //
    new(c + FineGridColor) sColorTabEnt(this, "FineGridColor",
        0, "royalblue",         0, 0, 0, 0, false);
    new(c + ElecFineGridColor) sColorTabEnt(this, "ElecFineGridColor",
        0, "royalblue",         0, 0, 0, 0, false);
    new(c + PhysFineGridColor) sColorTabEnt(this, "PhysFineGridColor",
        0, "royalblue",         0, 0, 0, 0, false);

    // Prompt line colors
    new(c + PromptTextColor) sColorTabEnt(this, "PromptTextColor",
        0, "sienna",            0, 0, 0, 0, false);
    new(c + PromptEditTextColor) sColorTabEnt(this, "PromptEditTextColor",
        0, "black",             0, 0, 0, 0, false);
    new(c + PromptHighlightColor) sColorTabEnt(this, "PromptHighlightColor",
        0, "red",               0, 0, 0, 0, false);
    new(c + PromptCursorColor) sColorTabEnt(this, "PromptCursorColor",
        0, "blue",              0, 0, 0, 0, false);
    new(c + PromptBackgroundColor) sColorTabEnt(this, "PromptBackgroundColor",
        0, "gray92",            0, 0, 0, 0, false);
    new(c + PromptEditBackgColor) sColorTabEnt(this, "PromptEditBackgColor",
        0, "gray96",            0, 0, 0, 0, false);
    new(c + PromptEditFocusBackgColor) sColorTabEnt(this, "PromptEditFocusBackgColor",
        0, "gray100",           0, 0, 0, 0, false);
    new(c + PromptSelectionBackgColor) sColorTabEnt(this, "PromptSelectionBackgColor",
        0, "skyblue",           0, 0, 0, 0, false);

    // WRspice plot colors (electrical only)
    new(c + Color2) sColorTabEnt(this, "Color2",
        0, "red",               0, 0, 0, 0, false);
    new(c + Color3) sColorTabEnt(this, "Color3",
        0, "lime green",        0, 0, 0, 0, false);
    new(c + Color4) sColorTabEnt(this, "Color4",
        0, "blue",              0, 0, 0, 0, false);
    new(c + Color5) sColorTabEnt(this, "Color5",
        0, "orange",            0, 0, 0, 0, false);
    new(c + Color6) sColorTabEnt(this, "Color6",
        0, "magenta",           0, 0, 0, 0, false);
    new(c + Color7) sColorTabEnt(this, "Color7",
        0, "turquoise",         0, 0, 0, 0, false);
    new(c + Color8) sColorTabEnt(this, "Color8",
        0, "sienna",            0, 0, 0, 0, false);
    new(c + Color9) sColorTabEnt(this, "Color9",
        0, "gray",              0, 0, 0, 0, false);
    new(c + Color10) sColorTabEnt(this, "Color10",
        0, "hot pink",          0, 0, 0, 0, false);
    new(c + Color11) sColorTabEnt(this, "Color11",
        0, "slate blue",        0, 0, 0, 0, false);
    new(c + Color12) sColorTabEnt(this, "Color12",
        0, "spring green",      0, 0, 0, 0, false);
    new(c + Color13) sColorTabEnt(this, "Color13",
        0, "cadet blue",        0, 0, 0, 0, false);
    new(c + Color14) sColorTabEnt(this, "Color14",
        0, "pink",              0, 0, 0, 0, false);
    new(c + Color15) sColorTabEnt(this, "Color15",
        0, "indian red",        0, 0, 0, 0, false);
    new(c + Color16) sColorTabEnt(this, "Color16",
        0, "chartreuse",        0, 0, 0, 0, false);
    new(c + Color17) sColorTabEnt(this, "Color17",
        0, "khaki",             0, 0, 0, 0, false);
    new(c + Color18) sColorTabEnt(this, "Color18",
        0, "dark salmon",       0, 0, 0, 0, false);
    new(c + Color19) sColorTabEnt(this, "Color19",
        0, "rosy brown",        0, 0, 0, 0, false);

    // Colors used by the GUI.  The names are also kept in the
    // GRattrColor table.

    new(c + GUIcolorSel) sColorTabEnt(this, "GUIcolorSel",
        0, "#e1e1ff",           0, 0, 0, 0, false, GRattrColorLocSel);
    new(c + GUIcolorNo) sColorTabEnt(this, "GUIcolorNo",
        0, "red",               0, 0, 0, 0, false, GRattrColorNo);
    new(c + GUIcolorYes) sColorTabEnt(this, "GUIcolorYes",
        0, "green3",            0, 0, 0, 0, false, GRattrColorYes);
    new(c + GUIcolorHl1) sColorTabEnt(this, "GUIcolorHl1",
        0, "red",               0, 0, 0, 0, false, GRattrColorHl1);
    new(c + GUIcolorHl2) sColorTabEnt(this, "GUIcolorHl2",
        0, "darkblue",          0, 0, 0, 0, false, GRattrColorHl2);
    new(c + GUIcolorHl3) sColorTabEnt(this, "GUIcolorHl3",
        0, "darkviolet",        0, 0, 0, 0, false, GRattrColorHl3);
    new(c + GUIcolorHl4) sColorTabEnt(this, "GUIcolorHl4",
        0, "sienna",            0, 0, 0, 0, false, GRattrColorHl4);
    new(c + GUIcolorDvBg) sColorTabEnt(this, "GUIcolorDvBg",
        0, "gray90",            0, 0, 0, 0, false, GRattrColorDvBg);
    new(c + GUIcolorDvFg) sColorTabEnt(this, "GUIcolorDvFg",
        0, "black",             0, 0, 0, 0, false, GRattrColorDvFg);
    new(c + GUIcolorDvHl) sColorTabEnt(this, "GUIcolorDvHl",
        0, "blue",              0, 0, 0, 0, false, GRattrColorDvHl);
    new(c + GUIcolorDvSl) sColorTabEnt(this, "GUIcolorDvSl",
        0, "gray80",            0, 0, 0, 0, false, GRattrColorDvSl);
};


// Initialize the pixel/rgb values from the default color names, and set
// up aliases.
//
void
sColorTab::init()
{
    for (unsigned i = 0; i < ColorTableEnd; i++)
        set_index_color(i, colors[i].get_defclr());

    // add aliases
    colors[PromptTextColor].add_alias("PromptText");
    colors[PromptEditTextColor].add_alias("PromptEditText");
    colors[PromptHighlightColor].add_alias("PromptHighlight");
    colors[PromptCursorColor].add_alias("PromptCursor");
    colors[PromptBackgroundColor].add_alias("PromptBackground");
    colors[PromptEditBackgColor].add_alias("PromptEditBackg");
    colors[PromptEditBackgColor].add_alias("PromptEditBackground");
    colors[PromptEditFocusBackgColor].add_alias("PromptEditFocusBackg");
    colors[PromptEditFocusBackgColor].add_alias("PromptEditFocusBackground");
    colors[PromptSelectionBackgColor].add_alias("PromptSelectionBackground");
    colors[BackgroundColor].add_alias("Background");
    colors[ElecBackgroundColor].add_alias("ElecBackground");
    colors[PhysBackgroundColor].add_alias("PhysBackground");
    colors[HighlightingColor].add_alias("Highlighting");
    colors[ElecHighlightingColor].add_alias("ElecHighlighting");
    colors[PhysHighlightingColor].add_alias("PhysHighlighting");
    colors[InstanceBBColor].add_alias("InstanceBB");
    colors[InstanceBBColor].add_alias("InstanceBox");
    colors[ElecInstanceBBColor].add_alias("ElecInstanceBB");
    colors[ElecInstanceBBColor].add_alias("ElecInstanceBox");
    colors[PhysInstanceBBColor].add_alias("PhysInstanceBB");
    colors[PhysInstanceBBColor].add_alias("PhysInstanceBox");
    colors[InstanceNameColor].add_alias("InstanceName");
    colors[ElecInstanceNameColor].add_alias("ElecInstanceName");
    colors[PhysInstanceNameColor].add_alias("PhysInstanceName");
    colors[InstanceSizeColor].add_alias("InstanceSize");
    colors[CoarseGridColor].add_alias("CoarseGrid");
    colors[ElecCoarseGridColor].add_alias("ElecCoarseGrid");
    colors[PhysCoarseGridColor].add_alias("PhysCoarseGrid");
    colors[FineGridColor].add_alias("FineGrid");
    colors[ElecFineGridColor].add_alias("ElecFineGrid");
    colors[PhysFineGridColor].add_alias("PhysFineGrid");

    // The special-purpose GUI colors are initialized here.
    for (int i = GUIcolorBase; i < ColorTableEnd; i++) {
        sColorTabEnt *c = colors + i;
        if (c->attr_index() >= 0) {
            DSPpkg::self()->SetAttrColor((GRattrColor)c->attr_index(),
                c->get_defclr());
        }
    }
}


// Allocate private color cells for certain attributes.  Call this
// after reading the tech file.  This is for 256-color support.
//
void
sColorTab::alloc()
{
    colors[PhysFineGridColor].alloc();
    colors[PhysCoarseGridColor].alloc();
    colors[ElecFineGridColor].alloc();
    colors[ElecCoarseGridColor].alloc();
    colors[InstanceBBColor].alloc();
}


// Return true if the name is a known color keyword.
//
bool
sColorTab::is_colorname(const char*name)
{
    if (!name)
        return (false);
    if (find_index(name) >= 0)
        return (true);
    return (false);
}


// Set the value for the color, either in the main color table or the
// GUI color table.  Return true on success.
//
bool
sColorTab::set_color(const char *name, const char *value)
{
    return (set_index_color(find_index(name), value));
}


// Set the attribute color, the text is saved in the defclr field. 
// This will also update the elec/phys parts for those colors that
// have them, if they are the same.
//
bool
sColorTab::set_index_color(int cindex, const char *colorin)
{
    if (cindex < 0 || cindex >= ColorTableEnd)
        return (false);
    if (!colorin)
        return (false);
    while (isspace(*colorin))
        colorin++;
    if (!*colorin)
        return (false);
    char *clr = lstring::copy(colorin);
    char *t = clr + strlen(clr) - 1;
    while (isspace(*t) && t >= clr)
        *t-- = 0;

    int rgb[3];
    if (!DSPpkg::self()->NameToRGB(clr, rgb)) {
        delete [] clr;
        return (false);
    }
    colors[cindex].set(this, rgb[0], rgb[1], rgb[2], clr);
    delete [] clr;

    if (cindex >= GUIcolorBase) {
        sColorTabEnt *c = colors + cindex;
        if (c->attr_index() >= 0) {
            DSPpkg::self()->SetAttrColor((GRattrColor)c->attr_index(),
                c->get_defclr());
        }
        return (true);
    }

    switch (cindex) {
    case BackgroundColor:
    case GhostColor:
    case HighlightingColor:
    case SelectColor1:
    case SelectColor2:
    case MarkerColor:
    case InstanceBBColor:
    case InstanceNameColor:
    case CoarseGridColor:
    case FineGridColor:
        {
            sColorTabEnt *ce = colors + cindex + 1;
            sColorTabEnt *cp = colors + cindex + 2;
            if (ce->rgb_eq(cp)) {
                ce->load(colors + cindex);
                cp->load(colors + cindex);
            }
        }

    default:
        break;
    }

    return (true);
}


// Find an attribute color entry by name.
//
sColorTab::sColorTabEnt *
sColorTab::find(const char *cname)
{
    for (int i = 0; i < ColorTableEnd; i++) {
        sColorTabEnt *c = colors + i;
        if (lstring::cieq(cname, c->keyword()))
            return (c);
        for (stringlist *l = c->aliases(); l; l = l->next) {
            if (lstring::cieq(cname, l->string))
                return (c);
        }
    }
    return (0);
}


// Find an attribute color entry by name, return index or -1 if not
// found.
//
int
sColorTab::find_index(const char *cname)
{
    for (int i = 0; i < ColorTableEnd; i++) {
        sColorTabEnt *c = colors + i;
        if (lstring::cieq(cname, c->keyword()))
            return (i);
        for (stringlist *l = c->aliases(); l; l = l->next) {
            if (lstring::cieq(cname, l->string))
                return (i);
        }
    }
    return (-1);
}


// Return the color resource name for the index.  The index can be
// cast to a GRattrColor if the return is non-null.
//
const char *
sColorTab::gui_color_name(int ix)
{
    for (int i = GUIcolorBase; i < ColorTableEnd; i++) {
        sColorTabEnt *c = colors + i;
        if (c->attr_index() == ix)
            return (c->keyword());
    }
    return (0);
}


// The next two functions return the color pixel value.
//
int
sColorTab::color(int cindex)
{
    return (color(cindex, DSP()->CurMode()));
}


int
sColorTab::color(int cindex, DisplayMode mode)
{
    switch (cindex) {
    case HighlightingColor:
    case MarkerColor:
    case InstanceBBColor:
    case InstanceNameColor:
        if (use_dark) {
            if (mode == Physical)
                return (colors[cindex + 2].dark_pixel());
            return (colors[cindex + 1].dark_pixel());
        }
        if (mode == Physical)
            return (colors[cindex + 2].pixel());
        return (colors[cindex + 1].pixel());

    case InstanceSizeColor:
        if (use_dark)
            return (colors[cindex].dark_pixel());
        return (colors[cindex].pixel());

    case BackgroundColor:
    case GhostColor:
    case SelectColor1:
    case SelectColor2:
    case CoarseGridColor:
    case FineGridColor:
        if (mode == Physical)
            return (colors[cindex + 2].pixel());
        return (colors[cindex + 1].pixel());
    default:
        break;
    }
    if (cindex >= 0 && cindex < ColorTableEnd)
        return (colors[cindex].pixel());
    return (0);
}


// The next two functions return the color rgb values.
//
void
sColorTab::get_rgb(int cindex, int *r, int *g, int *b)
{
    get_rgb(cindex, DSP()->CurMode(), r, g, b);
}


void
sColorTab::get_rgb(int cindex, DisplayMode mode, int *r, int *g, int *b)
{
    switch (cindex) {
    case BackgroundColor:
    case GhostColor:
    case HighlightingColor:
    case SelectColor1:
    case SelectColor2:
    case MarkerColor:
    case InstanceBBColor:
    case InstanceNameColor:
    case CoarseGridColor:
    case FineGridColor:
        if (mode == Physical)
            colors[cindex + 2].get_rgb(r, g, b);
        else
            colors[cindex + 1].get_rgb(r, g, b);
        return;
    default:
        break;
    }
    if (cindex >= 0 && cindex < ColorTableEnd)
        colors[cindex].get_rgb(r, g, b);
}


// The next two functions set the color rgb values.
//
void
sColorTab::set_rgb(int cindex, int r, int g, int b)
{
    set_rgb(cindex, DSP()->CurMode(), r, g, b);
}


void
sColorTab::set_rgb(int cindex, DisplayMode mode, int r, int g, int b)
{
    GRdraw *draw = DSP()->MainDraw();
    if (!draw)
        return;
    switch (cindex) {
    case GhostColor:
        if (mode == Physical) {
            colors[cindex + 2].set_rgb(r, g, b);
            if (draw) {
                draw->DefineColor(colors[cindex + 2].pixel_addr(), r, g, b);
                draw->SetGhostColor(colors[cindex + 2].pixel());
            }
            WindowDesc *wdesc;
            WDgen wgen(WDgen::SUBW, WDgen::ALL);
            while ((wdesc = wgen.next()) != 0) {
                if (!wdesc->Wdraw())
                    continue;
                if (wdesc->IsSimilar(DSP()->MainWdesc()))
                    wdesc->Wdraw()->SetGhostColor(colors[cindex + 2].pixel());
            }
        }
        else {
            colors[cindex + 1].set_rgb(r, g, b);
            if (draw) {
                draw->DefineColor(colors[cindex + 1].pixel_addr(), r, g, b);
                draw->SetGhostColor(colors[cindex + 1].pixel());
            }
            WindowDesc *wdesc;
            WDgen wgen(WDgen::SUBW, WDgen::ALL);
            while ((wdesc = wgen.next()) != 0) {
                if (!wdesc->Wdraw())
                    continue;
                if (wdesc->IsSimilar(DSP()->MainWdesc()))
                    wdesc->Wdraw()->SetGhostColor(colors[cindex + 1].pixel());
            }
        }
        return;
    case BackgroundColor:
    case HighlightingColor:
    case SelectColor1:
    case SelectColor2:
    case MarkerColor:
    case InstanceBBColor:
    case InstanceNameColor:
    case CoarseGridColor:
    case FineGridColor:
        if (mode == Physical) {
            colors[cindex + 2].set_rgb(r, g, b);
            if (draw)
                draw->DefineColor(colors[cindex + 2].pixel_addr(), r, g, b);
        }
        else {
            colors[cindex + 1].set_rgb(r, g, b);
            if (draw)
                draw->DefineColor(colors[cindex + 1].pixel_addr(), r, g, b);
        }
        return;
    default:
        break;
    }
    if (cindex >= 0 && cindex < ColorTableEnd) {
        colors[cindex].set_rgb(r, g, b);
        if (draw)
            draw->DefineColor(colors[cindex].pixel_addr(), r, g, b);
    }
}


// Set or unset the "dark mode".  Used to un-highlight display
// attributes.
//
void
sColorTab::set_dark(bool b, DisplayMode mode)
{
    if (b) {
        if (mode == Physical) {
            colors[HighlightingColor + 2].set_dark();
            colors[MarkerColor + 2].set_dark();
            colors[InstanceBBColor + 2].set_dark();
            colors[InstanceNameColor + 2].set_dark();
        }
        else {
            colors[HighlightingColor + 1].set_dark();
            colors[MarkerColor + 1].set_dark();
            colors[InstanceBBColor + 1].set_dark();
            colors[InstanceNameColor + 1].set_dark();
        }
        colors[InstanceSizeColor].set_dark();
    }
    use_dark = b;
}


// Dump the color information to the tech file.  The second arg sets
// how to treat colors set to default values:  skip these, print
// commented, of print as normal.
//
void
sColorTab::dump(FILE *techfp, CTPmode pmode)
{
    // Dump the attribute colors.
    for (int i = 0; i < ColorTableEnd; i++) {
        sColorTabEnt *c = colors + i;
        switch (i) {
        case BackgroundColor:
        case GhostColor:
        case HighlightingColor:
        case SelectColor1:
        case SelectColor2:
        case MarkerColor:
        case InstanceBBColor:
        case InstanceNameColor:
        case CoarseGridColor:
        case FineGridColor:
            c->check_print(techfp, pmode);
            break;
        case ElecBackgroundColor:
        case PhysBackgroundColor:
        case ElecGhostColor:
        case PhysGhostColor:
        case ElecHighlightingColor:
        case PhysHighlightingColor:
        case ElecSelectColor1:
        case PhysSelectColor1:
        case ElecSelectColor2:
        case PhysSelectColor2:
        case ElecMarkerColor:
        case PhysMarkerColor:
        case ElecInstanceBBColor:
        case PhysInstanceBBColor:
        case ElecInstanceNameColor:
        case PhysInstanceNameColor:
        case ElecCoarseGridColor:
        case PhysCoarseGridColor:
        case PhysFineGridColor:
        case ElecFineGridColor:
            break;
        default:
            c->print(techfp, pmode);
            DSP()->comment_dump(techfp, c->keyword(), c->aliases());
        }
    }
}
// End of sColorTab functions.


void
sColorTab::sColorTabEnt::set_dark()
{
    int p = DSP()->ContextDarkPcnt();
    DSPpkg::self()->AllocateColor(&te_dark_pixel, (p*te_red)/100,
        (p*te_green)/100, (p*te_blue/100));
}


// Add a keyword alias.
//
void
sColorTab::sColorTabEnt::add_alias(const char *alias)
{
    if (lstring::cieq(te_name, alias))
        return;
    for (stringlist *l = te_alii; l; l = l->next) {
        if (lstring::cieq(l->string, alias))
            return;
    }
    te_alii = new stringlist(lstring::copy(alias), te_alii);
}


// Look at the two following values, if different print them separately,
// otherwise print the present index.
// NOTE: This relies on ordering: general, electrical, physical
//
void
sColorTab::sColorTabEnt::check_print(FILE *techfp, CTPmode pmode)
{
    sColorTabEnt *ce = this + 1;
    sColorTabEnt *cp = this + 2;
    if (ce->rgb_eq(cp)) {
        print(techfp, pmode);
        DSP()->comment_dump(techfp, te_name, te_alii);
        DSP()->comment_dump(techfp, ce->te_name, ce->te_alii);
        DSP()->comment_dump(techfp, cp->te_name, cp->te_alii);
    }
    else {
        ce->print(techfp, pmode);
        DSP()->comment_dump(techfp, ce->te_name, ce->te_alii);
        cp->print(techfp, pmode);
        DSP()->comment_dump(techfp, cp->te_name, cp->te_alii);
        DSP()->comment_dump(techfp, te_name, te_alii);
    }
}


void
sColorTab::sColorTabEnt::print(FILE *techfp, CTPmode pmode)
{
    if (te_defclr) {
        if (pmode == CTPall || te_defclr != te_initclr)
            fprintf(techfp, "%s %s\n", te_name, te_defclr);
        else if (pmode == CTPcmtDef)
            fprintf(techfp, "# %s %s\n", te_name, te_defclr);
    }
    else
        fprintf(techfp, "%s %d %d %d\n", te_name, te_red, te_green, te_blue);
}
// End of sColorTabEnt functions

