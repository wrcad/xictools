
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
 $Id: tech_attrib.cc,v 1.59 2017/04/16 20:28:21 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "tech.h"
#include "tech_kwords.h"
#include "tech_extract.h"
#include "tech_ldb3d.h"
#include "tech_attr_cx.h"
#include "fontutil.h"


// Parse attribute keywords.  When there are multiple keywords to set
// an attribute, the last one specified "wins", if there is ambiguity.
//
// This is used for parsing main attributes as well as attributes in
// driver blocks.  The final two args are used in the latter case.
//
TCret
cTech::ParseAttributes(sAttrContext *ac, bool noact)
{
    if (ac && noact)
        noact = false;

    DSPattrib *a = (ac ? ac->attr() : DSP()->MainWdesc()->Attrib());
    const char *snapmsg = "%s: invalid snap number, set to 1.";
    const char *spmesg = "Obsolete keyword \"%s\" ignored,\n"
        "use SnapGridSpacing, SnapPerGrid/GridPerSnap.";
    const char *valmsg = "%s: bad value, (ignored).";

    //
    // Grid Presentation
    //

    if (Matching(Tkw.Axes())) {
        if (noact)
            return (TCmatch);
        if (lstring::cieq(tc_inbuf, "plain"))
            a->grid(Physical)->set_axes(AxesPlain);
        else if (lstring::cieq(tc_inbuf, "mark"))
            a->grid(Physical)->set_axes(AxesMark);
        else if (lstring::cieq(tc_inbuf, "none"))
            a->grid(Physical)->set_axes(AxesNone);
        else
            return (SaveError("%s: unknown keyword %s, ignored.", tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.ShowGrid())) {
        // Set display of grid in both physical and electrical modes.
        //
        if (noact)
            return (TCmatch);
        a->grid(Physical)->set_displayed(GetBoolean(tc_inbuf));
        a->grid(Electrical)->set_displayed(GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.PhysShowGrid())) {
        // Set display of grid in physical mode.
        //
        if (noact)
            return (TCmatch);
        a->grid(Physical)->set_displayed(GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.ElecShowGrid())) {
        // Set display of grid in electrical mode.
        //
        if (noact)
            return (TCmatch);
        a->grid(Electrical)->set_displayed(GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.GridOnBottom())) {
        // Show the grid below geometry in both physical and electrical modes.
        //
        if (noact)
            return (TCmatch);
        a->grid(Physical)->set_show_on_top(!GetBoolean(tc_inbuf));
        a->grid(Electrical)->set_show_on_top(!GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.PhysGridOnBottom())) {
        // Show the grid below geometry in physical mode.
        //
        if (noact)
            return (TCmatch);
        a->grid(Physical)->set_show_on_top(!GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.ElecGridOnBottom())) {
        // Show the grid below geometry in electrical mode.
        //
        if (noact)
            return (TCmatch);
        a->grid(Electrical)->set_show_on_top(!GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.GridStyle())) {
        // Set the grid linestyle used in both physical and
        // electrical modes.
        //
        if (noact)
            return (TCmatch);
        char *c = tc_inbuf;
        a->grid(Physical)->linestyle().mask = GetInt(c);
        a->grid(Electrical)->linestyle().mask = GetInt(c);
        a->grid(Physical)->set_dotsize(0);
        a->grid(Electrical)->set_dotsize(0);
        if (a->grid(Physical)->linestyle().mask == 0) {
            lstring::advtok(&c);
            a->grid(Physical)->set_dotsize(GetInt(c));
            a->grid(Electrical)->set_dotsize(GetInt(c));
        }
        return (TCmatch);
    }
    if (Matching(Tkw.PhysGridStyle())) {
        // Set the grid linestyle used in physical mode.
        //
        if (noact)
            return (TCmatch);
        char *c = tc_inbuf;
        a->grid(Physical)->linestyle().mask = GetInt(c);
        a->grid(Physical)->set_dotsize(0);
        if (a->grid(Physical)->linestyle().mask == 0) {
            lstring::advtok(&c);
            a->grid(Physical)->set_dotsize(GetInt(c));
        }
        return (TCmatch);
    }
    if (Matching(Tkw.ElecGridStyle())) {
        // Set the grid linestyle used in electrical mode.
        //
        if (noact)
            return (TCmatch);
        char *c = tc_inbuf;
        a->grid(Electrical)->linestyle().mask = GetInt(c);
        a->grid(Electrical)->set_dotsize(0);
        if (a->grid(Electrical)->linestyle().mask == 0) {
            lstring::advtok(&c);
            a->grid(Electrical)->set_dotsize(GetInt(c));
        }
        return (TCmatch);
    }
    if (Matching(Tkw.GridCoarseMult())) {
        // Set the grid coarse multiple for both physical and
        // electrical mode.
        //
        if (noact)
            return (TCmatch);
        char *c = tc_inbuf;
        int mult = GetInt(c);
        if (mult < 1 || mult > 50)
            return (SaveError(valmsg, Tkw.GridCoarseMult()));
        a->grid(Physical)->set_coarse_mult(mult);
        a->grid(Electrical)->set_coarse_mult(mult);
        return (TCmatch);
    }
    if (Matching(Tkw.PhysGridCoarseMult())) {
        // Set the grid coarse multiple for physical mode.
        //
        if (noact)
            return (TCmatch);
        char *c = tc_inbuf;
        int mult = GetInt(c);
        if (mult < 1 || mult > 50)
            return (SaveError(valmsg, Tkw.PhysGridCoarseMult()));
        a->grid(Physical)->set_coarse_mult(mult);
        return (TCmatch);
    }
    if (Matching(Tkw.ElecGridCoarseMult())) {
        // Set the grid coarse multiple for electrical mode.
        //
        if (noact)
            return (TCmatch);
        char *c = tc_inbuf;
        int mult = GetInt(c);
        if (mult < 1 || mult > 50)
            return (SaveError(valmsg, Tkw.ElecGridCoarseMult()));
        a->grid(Electrical)->set_coarse_mult(mult);
        return (TCmatch);
    }

    //
    // Misc. Presentation
    //

    if (Matching(Tkw.Expand())) {
        // Set to expand subcells, both physical and electrical modes.
        //
        if (noact)
            return (TCmatch);
        int i;
        if (sscanf(tc_inbuf, "%d", &i) == 1 && i >= -1 && i <= 10) {
            a->set_expand_level(Physical, i);
            a->set_expand_level(Electrical, i);
        }
        else {
            a->set_expand_level(Physical, 0);
            a->set_expand_level(Electrical, 0);
        }
        return (TCmatch);
    }
    if (Matching(Tkw.PhysExpand())) {
        if (noact)
            return (TCmatch);
        int i;
        if (sscanf(tc_inbuf, "%d", &i) == 1 && i >= -1 && i <= 10)
            a->set_expand_level(Physical, i);
        else
            a->set_expand_level(Physical, 0);
        return (TCmatch);
    }
    if (Matching(Tkw.ElecExpand())) {
        if (noact)
            return (TCmatch);
        int i;
        if (sscanf(tc_inbuf, "%d", &i) == 1 && i >= -1 && i <= 10)
            a->set_expand_level(Electrical, i);
        else
            a->set_expand_level(Electrical, 0);
        return (TCmatch);
    }
    if (Matching(Tkw.DisplayAllText())) {
        // Set to show labels, both physical and electrical modes.  If
        // 1, labels will not be transformed, if 2, label will be
        // shown in true orientation.
        if (noact)
            return (TCmatch);
        int i = GetInt(tc_inbuf);
        if (i == 0) {
            a->set_display_labels(Physical, SLnone);
            a->set_display_labels(Electrical, SLnone);
        }
        else if (i == 2) {
            a->set_display_labels(Physical, SLtrueOrient);
            a->set_display_labels(Electrical, SLtrueOrient);
        }
        else {
            a->set_display_labels(Physical, SLupright);
            a->set_display_labels(Electrical, SLupright);
        }
        return (TCmatch);
    }
    if (Matching(Tkw.PhysDisplayAllText())) {
        if (noact)
            return (TCmatch);
        int i = GetInt(tc_inbuf);
        if (i == 0)
            a->set_display_labels(Physical, SLnone);
        else if (i == 2)
            a->set_display_labels(Physical, SLtrueOrient);
        else
            a->set_display_labels(Physical, SLupright);
        return (TCmatch);
    }
    if (Matching(Tkw.ElecDisplayAllText())) {
        if (noact)
            return (TCmatch);
        int i = GetInt(tc_inbuf);
        if (i == 0)
            a->set_display_labels(Electrical, SLnone);
        else if (i == 2)
            a->set_display_labels(Electrical, SLtrueOrient);
        else
            a->set_display_labels(Electrical, SLupright);
        return (TCmatch);
    }
    if (Matching(Tkw.ShowPhysProps())) {
        // If set, physical properties will be shown in
        // physical mode.
        //
        if (noact)
            return (TCmatch);
        a->set_show_phys_props(GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.LabelAllInstances())) {
        // Set to label unexpanded instances, both physical and
        // electrical modes.  If 1, labels will not be transformed,
        // if 2, label will be transformed as instance.
        //
        if (noact)
            return (TCmatch);
        int i = GetInt(tc_inbuf);
        if (i == 0) {
            a->set_label_instances(Physical, SLnone);
            a->set_label_instances(Electrical, SLnone);
        }
        else if (i == 2) {
            a->set_label_instances(Physical, SLtrueOrient);
            a->set_label_instances(Electrical, SLtrueOrient);
        }
        else {
            a->set_label_instances(Physical, SLupright);
            a->set_label_instances(Electrical, SLupright);
        }
        return (TCmatch);
    }
    if (Matching(Tkw.PhysLabelAllInstances())) {
        if (noact)
            return (TCmatch);
        int i = GetInt(tc_inbuf);
        if (i == 0)
            a->set_label_instances(Physical, SLnone);
        else if (i == 2)
            a->set_label_instances(Physical, SLtrueOrient);
        else
            a->set_label_instances(Physical, SLupright);
        return (TCmatch);
    }
    if (Matching(Tkw.ElecLabelAllInstances())) {
        if (noact)
            return (TCmatch);
        int i = GetInt(tc_inbuf);
        if (i == 0)
            a->set_label_instances(Electrical, SLnone);
        else if (i == 2)
            a->set_label_instances(Electrical, SLtrueOrient);
        else
            a->set_label_instances(Electrical, SLupright);
        return (TCmatch);
    }
    if (Matching(Tkw.ShowContext())) {
        // If set, the context sill be shown in subedits
        if (noact)
            return (TCmatch);
        a->set_show_context(Physical, GetBoolean(tc_inbuf));
        a->set_show_context(Electrical, GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.PhysShowContext())) {
        if (noact)
            return (TCmatch);
        a->set_show_context(Physical, GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.ElecShowContext())) {
        if (noact)
            return (TCmatch);
        a->set_show_context(Electrical, GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.ShowTinyBB())) {
        // If set, tiny cells will not be expanded in physical
        // mode, the BB will be shown instead.
        //
        if (noact)
            return (TCmatch);
        a->set_show_tiny_bb(Physical, GetBoolean(tc_inbuf));
        a->set_show_tiny_bb(Electrical, GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.PhysShowTinyBB())) {
        if (noact)
            return (TCmatch);
        a->set_show_tiny_bb(Physical, GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.ElecShowTinyBB())) {
        if (noact)
            return (TCmatch);
        a->set_show_tiny_bb(Electrical, GetBoolean(tc_inbuf));
        return (TCmatch);
    }

    //
    // Attribute Colors
    //

    if (DSP()->ColorTab()->is_colorname(tc_kwbuf)) {
        const char *cmsg = "Bad specified color for %s.";
        if (ac) {
            int cindex = DSP()->ColorTab()->find_index(tc_kwbuf);
            if (cindex >= 0 && cindex < AttrColorSize) {
                // The way the color table works requires that three
                // entries be saved/restored.
                sColorTab::sColorTabEnt c[3];
                c[0] = *DSP()->ColorTab()->color_ent(cindex);
                c[1] = *DSP()->ColorTab()->color_ent(cindex+1);
                c[2] = *DSP()->ColorTab()->color_ent(cindex+2);
                if (!DSP()->ColorTab()->set_index_color(cindex, tc_inbuf))
                    return (SaveError(cmsg, tc_kwbuf));
                *ac->color(cindex) = *DSP()->ColorTab()->color_ent(cindex);
                if (cindex+1 < AttrColorSize) {
                    *ac->color(cindex+1) =
                        *DSP()->ColorTab()->color_ent(cindex+1);
                }
                if (cindex+2 < AttrColorSize) {
                    *ac->color(cindex+2) =
                        *DSP()->ColorTab()->color_ent(cindex+2);
                }
                *DSP()->ColorTab()->color_ent(cindex) = c[0];
                *DSP()->ColorTab()->color_ent(cindex+1) = c[1];
                *DSP()->ColorTab()->color_ent(cindex+2) = c[2];
            }
        }
        else if (!DSP()->ColorTab()->set_color(tc_kwbuf, tc_inbuf))
            return (SaveError(cmsg, tc_kwbuf));
        return (TCmatch);
    }
    // End of color keywords

    // ---- END of attributes that can be set in print blocks ----
    if (ac || noact)
        return (TCnone);

    //
    // Grid and Edge Snapping
    //

    // Electrical grid now initially set to 1.0/1.
    // Grid spacing not accepted in driver blocks.

    if (Matching(Tkw.MfgGrid())) {
        double d;
        if (sscanf(tc_inbuf, "%lf", &d) != 1)
            return (SaveError("%s: failed to parse value.", Tkw.MfgGrid()));
        if (!SetMfgGrid(d))
            return (SaveError("%s: unacceptable value.", Tkw.MfgGrid()));
        return (TCmatch);
    }
    if (Matching(Tkw.SnapGridSpacing())) {
        double d;
        if (sscanf(tc_inbuf, "%lf", &d) != 1) {
            return (SaveError("%s: failed to parse value.",
                Tkw.SnapGridSpacing()));
        }
        if (d <= 0.0) {
            return (SaveError("%s: not a positive value.",
                Tkw.SnapGridSpacing()));
        }
        a->grid(Physical)->set_spacing(d);
        return (TCmatch);
    }
    if (Matching(Tkw.SnapPerGrid())) {
        int n;
        if (sscanf(tc_inbuf, "%d", &n) == 1 && n >= 1 && n <= 10) {
            a->grid(Physical)->set_snap(n);
            return (TCmatch);
        }
        return (SaveError(snapmsg, Tkw.SnapPerGrid()));
    }
    if (Matching(Tkw.GridPerSnap())) {
        int n;
        if (sscanf(tc_inbuf, "%d", &n) == 1 && n >= 1 && n <= 10) {
            a->grid(Physical)->set_snap(-n);
            return (TCmatch);
        }
        return (SaveError(snapmsg, Tkw.GridPerSnap()));
    }
    if (Matching(Tkw.EdgeSnapping())) {
        const char *s = tc_inbuf;
        char *tok;
        while ((tok = lstring::gettok(&s)) != 0) {
            char c = isupper(*tok) ? tolower(*tok) : *tok;
            if (c == 'n')
                a->set_edge_snapping(EdgeSnapNone);
            else if (c == 's')
                a->set_edge_snapping(EdgeSnapSome);
            else if (c == 'a')
                a->set_edge_snapping(EdgeSnapAll);
            else if (c == '-') {
                c = isupper(tok[1]) ? tolower(tok[1]) : tok[1];
                if (c == 'o')
                    a->set_edge_off_grid(false);
                else if (c == 'n')
                    a->set_edge_non_manh(false);
                else if (c == 'e')
                    a->set_edge_wire_edge(false);
                else if (c == 'p')
                    a->set_edge_wire_path(false);
            }
            else if (c == '+') {
                c = isupper(tok[1]) ? tolower(tok[1]) : tok[1];
                if (c == 'o')
                    a->set_edge_off_grid(true);
                else if (c == 'm')
                    a->set_edge_non_manh(true);
                else if (c == 'e')
                    a->set_edge_wire_edge(true);
                else if (c == 'p')
                    a->set_edge_wire_path(true);
            }
            delete [] tok;
        };
        return (TCmatch);
    }
    if (Matching(Tkw.RulerEdgeSnapping())) {
        DSPattrib ar;
        DSP()->RulerGetSnapDefaults(&ar, 0, true);
        const char *s = tc_inbuf;
        char *tok;
        while ((tok = lstring::gettok(&s)) != 0) {
            char c = isupper(*tok) ? tolower(*tok) : *tok;
            if (c == 'n')
                ar.set_edge_snapping(EdgeSnapNone);
            else if (c == 's')
                ar.set_edge_snapping(EdgeSnapSome);
            else if (c == 'a')
                ar.set_edge_snapping(EdgeSnapAll);
            else if (c == '-') {
                c = isupper(tok[1]) ? tolower(tok[1]) : tok[1];
                if (c == 'o')
                    ar.set_edge_off_grid(false);
                else if (c == 'n')
                    ar.set_edge_non_manh(false);
                else if (c == 'e')
                    ar.set_edge_wire_edge(false);
                else if (c == 'p')
                    ar.set_edge_wire_path(false);
            }
            else if (c == '+') {
                c = isupper(tok[1]) ? tolower(tok[1]) : tok[1];
                if (c == 'o')
                    ar.set_edge_off_grid(true);
                else if (c == 'm')
                    ar.set_edge_non_manh(true);
                else if (c == 'e')
                    ar.set_edge_wire_edge(true);
                else if (c == 'p')
                    ar.set_edge_wire_path(true);
            }
            delete [] tok;
        };
        DSP()->RulerSetSnapDefaults(&ar, 0);
        return (TCmatch);
    }
    if (Matching(Tkw.RulerSnapToGrid())) {
        bool snap = GetBoolean(tc_inbuf);
        DSP()->RulerSetSnapDefaults(0, &snap);
        return (TCmatch);
    }

    // These are deprecated but still accepted.
    if (Matching(Tkw.Snapping()) || Matching(Tkw.PhysSnapping())) {
        int n;
        if (sscanf(tc_inbuf, "%d", &n) == 1 && abs(n) >= 1 && abs(n) <= 10) {
            a->grid(Physical)->set_snap(n);
            return (TCmatch);
        }
        return (SaveError(snapmsg, tc_kwbuf));
    }

    // These are no longer accepted.
    if (Matching(Tkw.ElecSnapping()))
        return (SaveError(spmesg, Tkw.ElecSnapping()));
    if (Matching(Tkw.GridSpacing()))
        return (SaveError(spmesg, Tkw.GridSpacing()));
    if (Matching(Tkw.PhysGridSpacing()))
        return (SaveError(spmesg, Tkw.PhysGridSpacing()));
    if (Matching(Tkw.ElecGridSpacing()))
        return (SaveError(spmesg, Tkw.ElecGridSpacing()));

    //
    // Function Key Assignments
    //

    if (*tc_kwbuf == 'F' || *tc_kwbuf == 'f') {
        int fkey = -1;
        for (int i = 1; i <= TECH_NUM_FKEYS; i++) {
            if (Matching(Tkw.FKey(i))) {
                fkey = i - 1;
                break;
            }
        }
        if (fkey >= 0) {
            tc_fkey_strs[fkey] = lstring::copy(tc_inbuf);
            return (TCmatch);
        }
    }

    //
    // Grid Registers
    //

    for (int i = 0; i < TECH_NUM_GRIDS; i++) {
        // Index 0 just returns TCmatch, i.e. silently igfnore.

        if (Matching(Tkw.PhysGridReg(i)))
            return (parse_gridreg(tc_inbuf, i, Physical));
        if (Matching(Tkw.ElecGridReg(i)))
            return (parse_gridreg(tc_inbuf, i, Electrical));

        // deprecated
        if (Matching(Tkw.GridReg(i)))
            return (parse_gridreg(tc_inbuf, i, Physical));
    }

    //
    // Layer Palette Registers
    //

    for (int i = 1; i < TECH_NUM_PALETTES; i++) {
        // Index 0 reserved for internal use.
        if (Matching(Tkw.PhysLayerPalette(i))) {
            Tech()->SetLayerPaletteReg(i, Physical, tc_inbuf);
            return (TCmatch);
        }
        if (Matching(Tkw.ElecLayerPalette(i))) {
            Tech()->SetLayerPaletteReg(i, Electrical, tc_inbuf);
            return (TCmatch);
        }
    }

    //
    // Font Assignments
    //

    // Fixed font for pop-up windows other than text editor/file browser.
    if (Matching(Tkw.Font1())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_FIXED);
        return (TCmatch);
    }
    if (Matching(Tkw.Font1P())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_FIXED, FNT_FMT_P);
        return (TCmatch);
    }
    if (Matching(Tkw.Font1Q())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_FIXED, FNT_FMT_Q);
        return (TCmatch);
    }
    if (Matching(Tkw.Font1W())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_FIXED, FNT_FMT_W);
        return (TCmatch);
    }
    if (Matching(Tkw.Font1X())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_FIXED, FNT_FMT_X);
        return (TCmatch);
    }

    // Prop. font for pop-up windows other than text editor/file browser.
    if (Matching(Tkw.Font2())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_PROP);
        return (TCmatch);
    }
    if (Matching(Tkw.Font2P())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_PROP, FNT_FMT_P);
        return (TCmatch);
    }
    if (Matching(Tkw.Font2Q())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_PROP, FNT_FMT_Q);
        return (TCmatch);
    }
    if (Matching(Tkw.Font2W())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_PROP, FNT_FMT_W);
        return (TCmatch);
    }
    if (Matching(Tkw.Font2X())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_PROP, FNT_FMT_X);
        return (TCmatch);
    }

    // Fixed font for screen.
    if (Matching(Tkw.Font3())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_SCREEN);
        return (TCmatch);
    }
    if (Matching(Tkw.Font3P())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_SCREEN, FNT_FMT_P);
        return (TCmatch);
    }
    if (Matching(Tkw.Font3Q())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_SCREEN, FNT_FMT_Q);
        return (TCmatch);
    }
    if (Matching(Tkw.Font3W())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_SCREEN, FNT_FMT_W);
        return (TCmatch);
    }
    if (Matching(Tkw.Font3X())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_SCREEN, FNT_FMT_X);
        return (TCmatch);
    }

    // Font for text editor/file browser windows.
    if (Matching(Tkw.Font4())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_EDITOR);
        return (TCmatch);
    }
    if (Matching(Tkw.Font4P())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_EDITOR, FNT_FMT_P);
        return (TCmatch);
    }
    if (Matching(Tkw.Font4Q())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_EDITOR, FNT_FMT_Q);
        return (TCmatch);
    }
    if (Matching(Tkw.Font4W())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_EDITOR, FNT_FMT_W);
        return (TCmatch);
    }
    if (Matching(Tkw.Font4X())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_EDITOR, FNT_FMT_X);
        return (TCmatch);
    }

    // Proportional base font for HTML viewer (not used in Windows).
    if (Matching(Tkw.Font5())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_MOZY);
        return (TCmatch);
    }
    if (Matching(Tkw.Font5P())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_MOZY, FNT_FMT_P);
        return (TCmatch);
    }
    if (Matching(Tkw.Font5Q())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_MOZY, FNT_FMT_Q);
        return (TCmatch);
    }
    if (Matching(Tkw.Font5W())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_MOZY, FNT_FMT_W);
        return (TCmatch);
    }
    if (Matching(Tkw.Font5X())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_MOZY, FNT_FMT_X);
        return (TCmatch);
    }

    // Fixed base font for HTML viewer (not used in Windows).
    if (Matching(Tkw.Font6())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_MOZY_FIXED);
        return (TCmatch);
    }
    if (Matching(Tkw.Font6P())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_MOZY_FIXED, FNT_FMT_P);
        return (TCmatch);
    }
    if (Matching(Tkw.Font6Q())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_MOZY_FIXED, FNT_FMT_Q);
        return (TCmatch);
    }
    if (Matching(Tkw.Font6W())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_MOZY_FIXED, FNT_FMT_W);
        return (TCmatch);
    }
    if (Matching(Tkw.Font6X())) {
        dspPkgIf()->SetFont(tc_inbuf, FNT_MOZY_FIXED, FNT_FMT_X);
        return (TCmatch);
    }

    // Caller's attribute keyword processing.
    if (tc_parse_attribute) {
        TCret tcret = (*tc_parse_attribute)();
        if (tcret != TCnone)
            return (tcret);
    }

    //
    // Attribute Variables
    //

    const char *attr;
    if (FindBooleanAttribute(tc_kwbuf, &attr)) {
        if (GetBoolean(tc_inbuf))
            CDvdb()->setVariable(attr, "");
        else
            CDvdb()->clearVariable(attr);
        return (TCmatch);
    }
    if (FindStringAttribute(tc_kwbuf, &attr)) {
        const char *t = tc_inbuf;
        while (isspace(*t))
            t++;
        char *e = tc_inbuf + strlen(t) - 1;
        while (e >= t && isspace(*e))
            *e-- = 0;
        bool ret = CDvdb()->setVariable(attr, t);
        if (ret)
            return (TCmatch);
        return (SaveError("Failed to parse %s value.", attr));
    }

    return (TCnone);
}


void
cTech::RegisterBooleanAttribute(const char *name)
{
    if (!name || !*name)
        return;
    if (!tc_battr_tab)
        tc_battr_tab = new ctable_t<tc_nm_t>;
    if (!tc_battr_tab->find(name)) {
        tc_nm_t *e = new tc_nm_t(name);
        tc_battr_tab->link(e);
        tc_battr_tab = tc_battr_tab->check_rehash();
    }
}


bool
cTech::FindBooleanAttribute(const char *name, const char **pattr)
{
    if (!tc_battr_tab || !name || !*name)
        return (false);
    tc_nm_t *n = tc_battr_tab->find(name);
    if (n && pattr)
        *pattr = n->tab_name();
    return (n != 0);
}


void
cTech::RegisterStringAttribute(const char *name)
{
    if (!name || !*name)
        return;
    if (!tc_sattr_tab)
        tc_sattr_tab = new ctable_t<tc_nm_t>;
    if (!tc_sattr_tab->find(name)) {
        tc_nm_t *e = new tc_nm_t(name);
        tc_sattr_tab->link(e);
        tc_sattr_tab = tc_sattr_tab->check_rehash();
    }
}


bool
cTech::FindStringAttribute(const char *name, const char **pattr)
{
    if (!tc_sattr_tab || !name || !*name)
        return (false);
    tc_nm_t *n = tc_sattr_tab->find(name);
    if (n && pattr)
        *pattr = n->tab_name();
    return (n != 0);
}


void
cTech::PrintAttributes(FILE *techfp, sAttrContext *ac, const char *drvrkw)
{
    // If in hard-copy mode, be careful about which attributes to print.
    if (DSP()->DoingHcopy()) {
        if (!ac) {
            // We're printing the main attributes, get them from the
            // backup.
            ac = GetHCbakAttrContext();
        }
        else if (ac == GetAttrContext(HcopyDriver(), false)) {
            // We're printing attributes for the current driver, these
            // are currently the "main" attributes.
            ac = 0;
        }
    }

    tBlkType blktype = drvrkw ? tBlkHcpy : tBlkNone;

    DSPattrib a_ref;

    DSPattrib *a = (ac ? ac->attr() : DSP()->MainWdesc()->Attrib());
    if (!a)
        return;

    // Grid parameters
    // Axes
    if (pcheck(techfp, (a->grid(Physical)->axes() !=
            a_ref.grid(Physical)->axes()))) {
        if (a->grid(Physical)->axes() == AxesNone)
            fprintf(techfp, "%s None\n", Tkw.Axes());
        else if (a->grid(Physical)->axes() == AxesPlain)
            fprintf(techfp, "%s Plain\n", Tkw.Axes());
        else
            fprintf(techfp, "%s Mark\n", Tkw.Axes());
    }
    CommentDump(techfp, 0, blktype, drvrkw, Tkw.Axes());

    // ShowGrid
    if (a->grid(Physical)->displayed() == a->grid(Electrical)->displayed()) {
        if (pcheck(techfp, (a->grid(Physical)->displayed() !=
                a_ref.grid(Physical)->displayed())))
            fprintf(techfp, "%s %c\n", Tkw.ShowGrid(),
                a->grid(Physical)->displayed() ? 'y' : 'n');
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ShowGrid());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecShowGrid());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysShowGrid());
    }
    else {
        if (pcheck(techfp, (a->grid(Electrical)->displayed() !=
                a_ref.grid(Electrical)->displayed())))
            fprintf(techfp, "%s %c\n", Tkw.ElecShowGrid(),
                a->grid(Electrical)->displayed() ? 'y' : 'n');
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ShowGrid());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecShowGrid());
        if (pcheck(techfp, (a->grid(Physical)->displayed() !=
                a_ref.grid(Physical)->displayed())))
            fprintf(techfp, "%s %c\n", Tkw.PhysShowGrid(),
                a->grid(Physical)->displayed() ? 'y' : 'n');
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysShowGrid());
    }

    // GridOnBottom
    if (a->grid(Physical)->show_on_top() ==
            a->grid(Electrical)->show_on_top()) {
        if (pcheck(techfp, (a->grid(Physical)->show_on_top() !=
                a_ref.grid(Physical)->show_on_top())))
            fprintf(techfp, "%s %c\n", Tkw.GridOnBottom(),
                a->grid(Physical)->show_on_top() ? 'n' : 'y');
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.GridOnBottom());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecGridOnBottom());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysGridOnBottom());
    }
    else {
        if (pcheck(techfp, (a->grid(Electrical)->show_on_top() !=
                a_ref.grid(Electrical)->show_on_top())))
            fprintf(techfp, "%s %c\n", Tkw.ElecGridOnBottom(),
                a->grid(Electrical)->show_on_top() ? 'n' : 'y');
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.GridOnBottom());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecGridOnBottom());
        if (pcheck(techfp, (a->grid(Physical)->show_on_top() !=
                a_ref.grid(Physical)->show_on_top())))
            fprintf(techfp, "%s %c\n", Tkw.PhysGridOnBottom(),
                a->grid(Physical)->show_on_top() ? 'n' : 'y');
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysGridOnBottom());
    }

    // GridStyle
    if (a->grid(Physical)->linestyle().mask ==
            a->grid(Electrical)->linestyle().mask &&
            (a->grid(Physical)->linestyle().mask != 0 ||
            (a->grid(Physical)->dotsize() ==
                a->grid(Electrical)->dotsize()))) {
        if (pcheck(techfp, (a->grid(Physical)->linestyle().mask !=
                a_ref.grid(Physical)->linestyle().mask ||
                a->grid(Physical)->dotsize() !=
                a_ref.grid(Physical)->dotsize()))) {
            int mask = a->grid(Physical)->linestyle().mask;
            if (mask == 0 && a->grid(Physical)->dotsize() != 0)
                fprintf(techfp, "%s 0 %d\n", Tkw.GridStyle(),
                    a->grid(Physical)->dotsize());
            else if (mask == 0 || mask == -1)
                fprintf(techfp, "%s %d\n", Tkw.GridStyle(), mask);
            else
                fprintf(techfp, "%s 0x%x\n", Tkw.GridStyle(), mask);
        }
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.GridStyle());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecGridStyle());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysGridStyle());
    }
    else {
        if (pcheck(techfp, (a->grid(Electrical)->linestyle().mask !=
                a_ref.grid(Electrical)->linestyle().mask ||
                a->grid(Electrical)->dotsize() !=
                a_ref.grid(Electrical)->dotsize()))) {
            int mask = a->grid(Electrical)->linestyle().mask;
            if (mask == 0 && a->grid(Electrical)->dotsize() != 0)
                fprintf(techfp, "%s 0 %d\n", Tkw.ElecGridStyle(),
                    a->grid(Electrical)->dotsize());
            else if (mask == 0 || mask == -1)
                fprintf(techfp, "%s %d\n", Tkw.ElecGridStyle(), mask);
            else
                fprintf(techfp, "%s 0x%x\n", Tkw.ElecGridStyle(), mask);
        }
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.GridStyle());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecGridStyle());
        if (pcheck(techfp, (a->grid(Physical)->linestyle().mask !=
                a_ref.grid(Physical)->linestyle().mask ||
                a->grid(Physical)->dotsize() !=
                a_ref.grid(Physical)->dotsize()))) {
            int mask = a->grid(Physical)->linestyle().mask;
            if (mask == 0 && a->grid(Physical)->dotsize() != 0)
                fprintf(techfp, "%s 0 %d\n", Tkw.PhysGridStyle(),
                    a->grid(Physical)->dotsize());
            else if (mask == 0 || mask == -1)
                fprintf(techfp, "%s %d\n", Tkw.PhysGridStyle(), mask);
            else
                fprintf(techfp, "%s 0x%x\n", Tkw.PhysGridStyle(), mask);
        }
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysGridStyle());
    }

    // CoarseMult
    if (a->grid(Physical)->coarse_mult() ==
            a->grid(Electrical)->coarse_mult()) {
        if (pcheck(techfp, (a->grid(Physical)->coarse_mult() !=
                a_ref.grid(Physical)->coarse_mult())))
            fprintf(techfp, "%s %d\n", Tkw.GridCoarseMult(),
                a->grid(Physical)->coarse_mult());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.GridCoarseMult());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecGridCoarseMult());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysGridCoarseMult());
    }
    else {
        if (pcheck(techfp, (a->grid(Electrical)->coarse_mult() !=
                a_ref.grid(Electrical)->coarse_mult())))
            fprintf(techfp, "%s %d\n", Tkw.ElecGridCoarseMult(),
                a->grid(Electrical)->coarse_mult());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.GridCoarseMult());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecGridCoarseMult());
        if (pcheck(techfp, (a->grid(Physical)->coarse_mult() !=
                a_ref.grid(Physical)->coarse_mult())))
            fprintf(techfp, "%s %d\n", Tkw.PhysGridCoarseMult(),
                a->grid(Physical)->coarse_mult());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysGridCoarseMult());
    }

    // Expand
    if (a->expand_level(Physical) == a->expand_level(Electrical)) {
        if (pcheck(techfp, (a->expand_level(Physical) !=
                a_ref.expand_level(Physical))))
            fprintf(techfp, "%s %d\n", Tkw.Expand(),
                a->expand_level(Physical));
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.Expand());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecExpand());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysExpand());
    }
    else {
        if (pcheck(techfp, (a->expand_level(Electrical) !=
                a_ref.expand_level(Electrical))))
            fprintf(techfp, "%s %d\n", Tkw.ElecExpand(),
                a->expand_level(Electrical));
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.Expand());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecExpand());
        if (pcheck(techfp, (a->expand_level(Physical) !=
                a_ref.expand_level(Physical))))
            fprintf(techfp, "%s %d\n", Tkw.PhysExpand(),
                a->expand_level(Physical));
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysExpand());
    }

    // DisplayAllText
    if (a->display_labels(Physical) == a->display_labels(Electrical)) {
        if (pcheck(techfp, (a->display_labels(Physical) !=
                a_ref.display_labels(Physical))))
            fprintf(techfp, "%s %d\n", Tkw.DisplayAllText(),
                a->display_labels(Physical));
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.DisplayAllText());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecDisplayAllText());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysDisplayAllText());
    }
    else {
        if (pcheck(techfp, (a->display_labels(Electrical) !=
                a_ref.display_labels(Electrical))))
            fprintf(techfp, "%s %d\n", Tkw.ElecDisplayAllText(),
                a->display_labels(Electrical));
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.DisplayAllText());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecDisplayAllText());
        if (pcheck(techfp, (a->display_labels(Physical) !=
                a_ref.display_labels(Physical))))
            fprintf(techfp, "%s %d\n", Tkw.PhysDisplayAllText(),
                a->display_labels(Physical));
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysDisplayAllText());
    }

    // ShowPhysProps
    if (pcheck(techfp, (a->show_phys_props() != a_ref.show_phys_props())))
        fprintf(techfp, "%s %c\n", Tkw.ShowPhysProps(),
            a->show_phys_props() ? 'y' : 'n');
    CommentDump(techfp, 0, blktype, drvrkw, Tkw.ShowPhysProps());

    // LabelAllInstances
    if (a->label_instances(Physical) == a->label_instances(Electrical)) {
        if (pcheck(techfp, (a->label_instances(Physical) !=
                a_ref.label_instances(Physical))))
            fprintf(techfp, "%s %d\n", Tkw.LabelAllInstances(),
                a->label_instances(Physical));
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.LabelAllInstances());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecLabelAllInstances());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysLabelAllInstances());
    }
    else {
        if (pcheck(techfp, (a->label_instances(Electrical) !=
                a_ref.label_instances(Electrical))))
            fprintf(techfp, "%s %d\n", Tkw.ElecLabelAllInstances(),
                a->label_instances(Electrical));
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.LabelAllInstances());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecLabelAllInstances());
        if (pcheck(techfp, (a->label_instances(Physical) !=
                a_ref.label_instances(Physical))))
            fprintf(techfp, "%s %d\n", Tkw.PhysLabelAllInstances(),
                a->label_instances(Physical));
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysLabelAllInstances());
    }

    // ShowContext
    if (a->show_context(Physical) == a->show_context(Electrical)) {
        if (pcheck(techfp, (a->show_context(Physical) !=
                a_ref.show_context(Physical))))
            fprintf(techfp, "%s %c\n", Tkw.ShowContext(),
                a->show_context(Physical) ? 'y' : 'n');
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ShowContext());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecShowContext());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysShowContext());
    }
    else {
        if (pcheck(techfp, (a->show_context(Electrical) !=
                a_ref.show_context(Electrical))))
            fprintf(techfp, "%s %c\n", Tkw.ElecShowContext(),
                a->show_context(Electrical) ? 'y' : 'n');
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ShowContext());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecShowContext());
        if (pcheck(techfp, (a->show_context(Physical) !=
                a_ref.show_context(Physical))))
            fprintf(techfp, "%s %c\n", Tkw.PhysShowContext(),
                a->show_context(Physical) ? 'y' : 'n');
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysShowContext());
    }

    // ShowTinyBB
    if (a->show_tiny_bb(Physical) == a->show_tiny_bb(Electrical)) {
        if (pcheck(techfp, (a->show_tiny_bb(Physical) !=
                a_ref.show_tiny_bb(Physical))))
            fprintf(techfp, "%s %c\n", Tkw.ShowTinyBB(),
                a->show_tiny_bb(Physical) ? 'y' : 'n');
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ShowTinyBB());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecShowTinyBB());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysShowTinyBB());
    }
    else {
        if (pcheck(techfp, (a->show_tiny_bb(Electrical) !=
                a_ref.show_tiny_bb(Electrical))))
            fprintf(techfp, "%s %c\n", Tkw.ElecShowTinyBB(),
                a->show_tiny_bb(Electrical) ? 'y' : 'n');
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ShowTinyBB());
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecShowTinyBB());
        if (pcheck(techfp, (a->show_tiny_bb(Physical) !=
                a_ref.show_tiny_bb(Physical))))
            fprintf(techfp, "%s %c\n", Tkw.PhysShowTinyBB(),
                a->show_tiny_bb(Physical) ? 'y' : 'n');
        CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysShowTinyBB());
    }

    // Background and other colors
    if (ac) {
        if (Tech()->PrintMode() == TCPRall)
            ac->dump(techfp, CTPall);
        if (Tech()->PrintMode() == TCPRcmt)
            ac->dump(techfp, CTPcmtDef);
        else
            ac->dump(techfp, CTPnoDef);
    }
    else {
        if (Tech()->PrintMode() == TCPRall)
            DSP()->ColorTab()->dump(techfp, CTPall);
        if (Tech()->PrintMode() == TCPRcmt)
            DSP()->ColorTab()->dump(techfp, CTPcmtDef);
        else
            DSP()->ColorTab()->dump(techfp, CTPnoDef);
    }
    // End of color keywords

    // We print the rest of the stuff only when printing the main
    // attributes.
    //
    if (drvrkw)
        return;

    // MfgGrid
    if (MfgGrid() > 0.0)
        fprintf(techfp, "%s %.4f\n", Tkw.MfgGrid(), MfgGrid());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.MfgGrid());

    // SnapGridSpacing
    if (a->grid(Physical)->spacing(Physical) != 1.0) {
        fprintf(techfp, "%s %.4f\n", Tkw.SnapGridSpacing(),
            a->grid(Physical)->spacing(Physical));
    }
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.SnapGridSpacing());

    // SnapPerGrid / GridPerSnap
    int snap = a->grid(Physical)->snap();
    if (pcheck(techfp, (abs(snap) != 1))) {
        if (snap > 0)
            fprintf(techfp, "%s %d\n", Tkw.SnapPerGrid(), snap);
        else
            fprintf(techfp, "%s %d\n", Tkw.GridPerSnap(), -snap);
    }
    CommentDump(techfp, 0, blktype, drvrkw, Tkw.SnapPerGrid());
    CommentDump(techfp, 0, blktype, drvrkw, Tkw.GridPerSnap());
    CommentDump(techfp, 0, blktype, drvrkw, Tkw.GridSpacing());
    CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecGridSpacing());
    CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysGridSpacing());
    CommentDump(techfp, 0, blktype, drvrkw, Tkw.Snapping());
    CommentDump(techfp, 0, blktype, drvrkw, Tkw.ElecSnapping());
    CommentDump(techfp, 0, blktype, drvrkw, Tkw.PhysSnapping());

    // EdgeSnapping
    if (pcheck(techfp,
            a->edge_snapping() != a_ref.edge_snapping() ||
            a->edge_off_grid() != a_ref.edge_off_grid() ||
            a->edge_non_manh() != a_ref.edge_non_manh() ||
            a->edge_wire_edge() != a_ref.edge_wire_edge() ||
            a->edge_wire_path() != a_ref.edge_wire_path())) {
        fprintf(techfp, "%s ", Tkw.EdgeSnapping());
        if (a->edge_snapping() == EdgeSnapNone)
            fprintf(techfp, "none\n");
        else {
            if (a->edge_snapping() == EdgeSnapSome)
                fprintf(techfp, "some");
            else
                fprintf(techfp, "all");
            fprintf(techfp,
                " %coff_grid %cnon_manh %cedge_of_wires %cpath_of_wires\n",
                a->edge_off_grid() ? '+' : '-',
                a->edge_non_manh() ? '+' : '-',
                a->edge_wire_edge() ? '+' : '-',
                a->edge_wire_path() ? '+' : '-');
        }
    }
    CommentDump(techfp, 0, blktype, drvrkw, Tkw.EdgeSnapping());

    // RulerSnapToGrid
    bool issnap, snapdef;
    DSPattrib ra, radef;
    DSP()->RulerGetSnapDefaults(&ra, &issnap, false);
    DSP()->RulerGetSnapDefaults(&radef, &snapdef, true);
    if (pcheck(techfp, issnap != snapdef)) {
        fprintf(techfp, "%s %c\n", Tkw.RulerSnapToGrid(),
            issnap ? 'y' : 'n');
    }
    CommentDump(techfp, 0, blktype, drvrkw, Tkw.RulerSnapToGrid());

    // RulerEdgeSnapping
    if (pcheck(techfp,
            (ra.edge_snapping() != EdgeSnapNone) !=
                (radef.edge_snapping() != EdgeSnapNone) ||
            ra.edge_off_grid() !=  radef.edge_off_grid() ||
            ra.edge_non_manh() !=  radef.edge_non_manh() ||
            ra.edge_wire_edge() != radef.edge_wire_edge() ||
            ra.edge_wire_path() != radef.edge_wire_path() )) {
        fprintf(techfp, "%s ", Tkw.RulerEdgeSnapping());
        if (a->edge_snapping() == EdgeSnapNone)
            fprintf(techfp, "none\n");
        else {
            fprintf(techfp,
                " %coff_grid %cnon_manh %cedge_of_wires %cpath_of_wires\n",
                ra.edge_off_grid() ? '+' : '-',
                ra.edge_non_manh() ? '+' : '-',
                ra.edge_wire_edge() ? '+' : '-',
                ra.edge_wire_path() ? '+' : '-');
        }
    }
    CommentDump(techfp, 0, blktype, drvrkw, Tkw.RulerEdgeSnapping());

    // Print function key specifications.
    print_func_keys(techfp);

    // Print grid register assignments.
    print_grid_registers(techfp);

    // Print layer palette lists.
    print_layer_palettes(techfp);

    // Print fonts
    print_fonts(techfp);

    // Print Caller's attributes.
    if (tc_print_attributes)
        (*tc_print_attributes)(techfp);

    // Print the boolean attributes that pass through to variables.
    print_boolean_attributes(techfp);

    // Print the non-boolean attributes that pass through to variables.
    print_string_attributes(techfp);

    fprintf(techfp, "# End of global attributes.\n");
}


// Print a list of all of the registered attribute variables.
//
void
cTech::PrintAttrVars(FILE *fp)
{
    if (tc_battr_tab) {
        fprintf(fp, "*** Boolean attribute variables\n");
        tgen_t<tc_nm_t> gen(tc_battr_tab);
        stringlist *s0 = 0;
        tc_nm_t *n;
        while ((n = gen.next()) != 0)
            s0 = new stringlist(lstring::copy(n->name), s0);
        stringlist::sort(s0);
        for (stringlist *s = s0; s; s = s->next)
            fprintf(fp, "%s\n", s->string);
        stringlist::destroy(s0);
    }
    if (tc_sattr_tab) {
        fprintf(fp, "\n*** String attribute variables\n");
        tgen_t<tc_nm_t> gen(tc_sattr_tab);
        stringlist *s0 = 0;
        tc_nm_t *n;
        while ((n = gen.next()) != 0)
            s0 = new stringlist(lstring::copy(n->name), s0);
        stringlist::sort(s0);
        for (stringlist *s = s0; s; s = s->next)
            fprintf(fp, "%s\n", s->string);
        stringlist::destroy(s0);
    }
}


void
cTech::print_boolean_attributes(FILE *techfp)
{
    if (!tc_battr_tab)
        return;
    tgen_t<tc_nm_t> gen(tc_battr_tab);
    stringlist *s0 = 0;
    tc_nm_t *n;
    while ((n = gen.next()) != 0)
        s0 = new stringlist(lstring::copy(n->name), s0);
    stringlist::sort(s0);

    // The default state of these is unset.
    for (stringlist *s = s0; s; s = s->next) {
        bool isset = CDvdb()->getVariable(s->string);
        if (isset)
            fprintf(techfp, "%s y\n", s->string);
        CommentDump(techfp, 0, tBlkNone, 0, s->string);
    }
    stringlist::destroy(s0);
}


void
cTech::print_string_attributes(FILE *techfp)
{
    if (!tc_sattr_tab)
        return;
    tgen_t<tc_nm_t> gen(tc_sattr_tab);
    stringlist *s0 = 0;
    tc_nm_t *n;
    while ((n = gen.next()) != 0)
        s0 = new stringlist(lstring::copy(n->name), s0);
    stringlist::sort(s0);

    // The default state of these is unset.
    for (stringlist *s = s0; s; s = s->next) {
        const char *val = CDvdb()->getVariable(s->string);
        if (val)
            fprintf(techfp, "%s %s\n", s->string, val);
        CommentDump(techfp, 0, tBlkNone, 0, s->string);
    }
    stringlist::destroy(s0);
}


TCret
cTech::parse_gridreg(const char *str, int ix, DisplayMode m)
{
    if (ix > 0 && ix < TECH_NUM_GRIDS) {
        GridDesc gd;
        if (!GridDesc::parse(str, gd)) {
            return (SaveError("%sGridReg%d: %s.",
                m == Physical ? "Phys" : "Elec", ix, Errs()->get_error()));
        }
        if (m == Physical)
            tc_phys_grids[ix].set(gd);
        else
            tc_elec_grids[ix].set(gd);
    }
    return (TCmatch);
}


namespace {
    void print_gridreg(FILE *techfp, GridDesc &gr, const char *kw)
    {
        char *str = gr.desc_string();
        fprintf(techfp, "%s %s\n", kw, str);
        delete [] str;
    }
}


void
cTech::print_grid_registers(FILE *techfp)
{
    GridDesc defgrid;
    bool is_grd = false;
    for (int i = 0; i < TECH_NUM_GRIDS; i++) {
        if (tc_phys_grids[i] != defgrid || tc_elec_grids[i] != defgrid) {
            is_grd = true;
            break;
        }
    }
    if (!is_grd)
        return;

    fprintf(techfp, "\n# Grid Register Assignments\n");
    for (int i = 1; i < TECH_NUM_GRIDS; i++) {
        if (tc_phys_grids[i] != defgrid)
            print_gridreg(techfp, tc_phys_grids[i], Tkw.PhysGridReg(i));
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.PhysGridReg(i));
        if (tc_elec_grids[i] != defgrid)
            print_gridreg(techfp, tc_elec_grids[i], Tkw.ElecGridReg(i));
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.ElecGridReg(i));
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.GridReg(i));
    }
}


void
cTech::print_func_keys(FILE *techfp)
{
    bool is_fkey = false;
    for (int i = 0; i < TECH_NUM_FKEYS; i++) {
        if (tc_fkey_strs[i]) {
            is_fkey = true;
            break;
        }
    }
    if (!is_fkey)
        return;

    fprintf(techfp, "\n# Function Key Assignments\n");
    for (int i = 0; i < TECH_NUM_FKEYS; i++) {
        if (tc_fkey_strs[i])
            fprintf(techfp, "%s %s\n", Tkw.FKey(i+1), tc_fkey_strs[i]);
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.FKey(i+1));
    }
}


void
cTech::print_layer_palettes(FILE *techfp)
{
    bool is_lpal = false;
    // Index 0 reserved for internal use.
    for (int i = 1; i < TECH_NUM_PALETTES; i++) {
        if (tc_phys_layer_palettes[i] || tc_elec_layer_palettes[i]) {
            is_lpal = true;
            break;
        }
    }
    if (!is_lpal)
        return;

    fprintf(techfp, "\n# Layer Palette registers\n");
    for (int i = 1; i < TECH_NUM_PALETTES; i++) {
        if (tc_phys_layer_palettes[i])
            fprintf(techfp, "%s %s\n", Tkw.PhysLayerPalette(i),
                tc_phys_layer_palettes[i]);
        if (tc_elec_layer_palettes[i])
            fprintf(techfp, "%s %s\n", Tkw.ElecLayerPalette(i),
                tc_elec_layer_palettes[i]);
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.PhysLayerPalette(i));
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.ElecLayerPalette(i));
    }
}


void
cTech::print_fonts(FILE *techfp)
{
    FNT_FMT fnt_fmt = dspPkgIf()->GetFontFmt();
    bool hdr_printed = false;
    const char *hstr = "\n# Fonts\n";

    const char *fn = dspPkgIf()->GetFont(FNT_FIXED);
    const char *dn = FC.getDefaultName(FNT_FIXED);
    if (fn && *fn) {
        if (pcheck(techfp, (!dn || strcmp(fn, dn)))) {
            if (!hdr_printed) {
                hdr_printed = true;
                fputs(hstr, techfp);
            }
            if (fnt_fmt == FNT_FMT_P) {
                fprintf(techfp, "%s %s\n", Tkw.Font1P(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font1P());
            }
            else if (fnt_fmt == FNT_FMT_Q) {
                fprintf(techfp, "%s %s\n", Tkw.Font1Q(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font1Q());
            }
            else if (fnt_fmt == FNT_FMT_W) {
                fprintf(techfp, "%s %s\n", Tkw.Font1W(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font1W());
            }
            else if (fnt_fmt == FNT_FMT_X) {
                fprintf(techfp, "%s %s\n", Tkw.Font1X(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font1X());
            }
        }
    }
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font1());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font1P());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font1Q());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font1W());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font1X());

    fn = dspPkgIf()->GetFont(FNT_PROP);
    dn = FC.getDefaultName(FNT_PROP);
    if (fn && *fn) {
        if (pcheck(techfp, (!dn || strcmp(fn, dn)))) {
            if (!hdr_printed) {
                hdr_printed = true;
                fputs(hstr, techfp);
            }
            if (fnt_fmt == FNT_FMT_P) {
                fprintf(techfp, "%s %s\n", Tkw.Font2P(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font2P());
            }
            else if (fnt_fmt == FNT_FMT_Q) {
                fprintf(techfp, "%s %s\n", Tkw.Font2Q(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font2Q());
            }
            else if (fnt_fmt == FNT_FMT_W) {
                fprintf(techfp, "%s %s\n", Tkw.Font2W(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font2W());
            }
            else if (fnt_fmt == FNT_FMT_X) {
                fprintf(techfp, "%s %s\n", Tkw.Font2X(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font2X());
            }
        }
    }
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font2());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font2P());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font2Q());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font2W());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font2X());

    fn = dspPkgIf()->GetFont(FNT_SCREEN);
    dn = FC.getDefaultName(FNT_SCREEN);
    if (fn && *fn) {
        if (pcheck(techfp, (!dn || strcmp(fn, dn)))) {
            if (!hdr_printed) {
                hdr_printed = true;
                fputs(hstr, techfp);
            }
            if (fnt_fmt == FNT_FMT_P) {
                fprintf(techfp, "%s %s\n", Tkw.Font3P(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font3P());
            }
            else if (fnt_fmt == FNT_FMT_Q) {
                fprintf(techfp, "%s %s\n", Tkw.Font3Q(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font3Q());
            }
            else if (fnt_fmt == FNT_FMT_W) {
                fprintf(techfp, "%s %s\n", Tkw.Font3W(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font3W());
            }
            else if (fnt_fmt == FNT_FMT_X) {
                fprintf(techfp, "%s %s\n", Tkw.Font3X(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font3X());
            }
        }
    }
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font3());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font3P());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font3Q());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font3W());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font3X());

    fn = dspPkgIf()->GetFont(FNT_EDITOR);
    dn = FC.getDefaultName(FNT_EDITOR);
    if (fn && *fn) {
        if (pcheck(techfp, (!dn || strcmp(fn, dn)))) {
            if (!hdr_printed) {
                hdr_printed = true;
                fputs(hstr, techfp);
            }
            if (fnt_fmt == FNT_FMT_P) {
                fprintf(techfp, "%s %s\n", Tkw.Font4P(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font4P());
            }
            else if (fnt_fmt == FNT_FMT_Q) {
                fprintf(techfp, "%s %s\n", Tkw.Font4Q(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font4Q());
            }
            else if (fnt_fmt == FNT_FMT_W) {
                fprintf(techfp, "%s %s\n", Tkw.Font4W(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font4W());
            }
            else if (fnt_fmt == FNT_FMT_X) {
                fprintf(techfp, "%s %s\n", Tkw.Font4X(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font4X());
            }
        }
    }
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font4());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font4P());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font4Q());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font4W());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font4X());

    fn = dspPkgIf()->GetFont(FNT_MOZY);
    dn = FC.getDefaultName(FNT_MOZY);
    if (fn && *fn) {
        if (pcheck(techfp, (!dn || strcmp(fn, dn)))) {
            if (!hdr_printed) {
                hdr_printed = true;
                fputs(hstr, techfp);
            }
            if (fnt_fmt == FNT_FMT_P) {
                fprintf(techfp, "%s %s\n", Tkw.Font5P(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font5P());
            }
            else if (fnt_fmt == FNT_FMT_Q) {
                fprintf(techfp, "%s %s\n", Tkw.Font5Q(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font5Q());
            }
            else if (fnt_fmt == FNT_FMT_W) {
                fprintf(techfp, "%s %s\n", Tkw.Font5W(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font5W());
            }
            else if (fnt_fmt == FNT_FMT_X) {
                fprintf(techfp, "%s %s\n", Tkw.Font5X(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font5X());
            }
        }
    }
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font5());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font5P());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font5Q());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font5W());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font5X());

    fn = dspPkgIf()->GetFont(FNT_MOZY_FIXED);
    dn = FC.getDefaultName(FNT_MOZY_FIXED);
    if (fn && *fn) {
        if (pcheck(techfp, (!dn || strcmp(fn, dn)))) {
            if (!hdr_printed) {
                hdr_printed = true;
                fputs(hstr, techfp);
            }
            if (fnt_fmt == FNT_FMT_P) {
                fprintf(techfp, "%s %s\n", Tkw.Font6P(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font6P());
            }
            else if (fnt_fmt == FNT_FMT_Q) {
                fprintf(techfp, "%s %s\n", Tkw.Font6Q(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font6Q());
            }
            else if (fnt_fmt == FNT_FMT_W) {
                fprintf(techfp, "%s %s\n", Tkw.Font6W(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font6W());
            }
            else if (fnt_fmt == FNT_FMT_X) {
                fprintf(techfp, "%s %s\n", Tkw.Font6X(), fn);
                CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font6X());
            }
        }
    }
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font6());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font6P());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font6Q());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font6W());
    CommentDump(techfp, 0, tBlkNone, 0, Tkw.Font6X());
}

