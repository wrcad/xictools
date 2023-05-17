
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
#include "dsp_layer.h"
#include "cd_lgen.h"
#include "cd_strmdata.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_lisp.h"
#include "tech.h"
#include "tech_layer.h"
#include "tech_via.h"
#include "tech_cds_out.h"
#include "tech_drf_in.h"
#include "drcif.h"
#include "miscutil/filestat.h"


//
// Functions for cTechCdsOut Cadence tech/drf file writer.
//

#define DEFAULT_BASENAME "xic_tech_cds"

// Public function for writing a Cadence Virtuoso compatible ascii
// technology file.  The file will be named <basename>.txt The
// basename can have a path prepended.  If basename is null, a default
// "xic_tech_cds" is used.


//
bool
cTechCdsOut::write_tech(const char *basename)
{
    if (!basename || !*basename)
        basename = DEFAULT_BASENAME;
    char *filename = new char[strlen(basename) + 8];
    char *end = lstring::stpcpy(filename, basename);

    strcpy(end, ".txt");

    filestat::clear_error();
    if (!filestat::create_bak(filename)) {
        const char *erm = filestat::error_msg();
        if (erm) {
            Errs()->add_error(erm);
            filestat::clear_error();
        }
        else {
            Errs()->add_error("failed to back up %s.",
                lstring::strip_path(filename));
        }
        delete [] filename;
        return (false);
    }

    tco_fp = fopen(filename, "w");
    if (!tco_fp) {
        Errs()->sys_error(lstring::strip_path(filename));
        Errs()->add_error("failed to open %s for writing.",
            lstring::strip_path(filename));
        delete [] filename;
        return (false);
    }
    if (!dump_tech()) {
        Errs()->add_error("failed writing %s file.",
            lstring::strip_path(filename));
        fclose(tco_fp);
        tco_fp = 0;
        delete [] filename;
        return (false);
    }

    delete [] filename;
    if (tco_fp) {
        fclose(tco_fp);
        tco_fp = 0;
    }
    return (true);
}


// Public function for writing a Cadence Virtuoso compatible display
// resource file.  The file will be named <basename>.drf.  The
// basename can have a path prepended.  If basename is null, a default
// "xic_tech_cds" is used.
//
bool
cTechCdsOut::write_drf(const char *basename)
{
    if (!basename || !*basename)
        basename = DEFAULT_BASENAME;
    char *filename = new char[strlen(basename) + 8];
    char *end = lstring::stpcpy(filename, basename);

    strcpy(end, ".drf");

    if (!filestat::create_bak(filename)) {
        const char *erm = filestat::error_msg();
        if (erm) {
            Errs()->add_error(erm);
            filestat::clear_error();
        }
        else {
            Errs()->add_error("failed to back up %s.",
                lstring::strip_path(filename));
        }
        delete [] filename;
        return (false);
    }

    tco_fp = fopen(filename, "w");
    if (!tco_fp) {
        Errs()->sys_error(lstring::strip_path(filename));
        Errs()->add_error("failed to open %s for writing.",
            lstring::strip_path(filename));
        delete [] filename;
        return (false);
    }
    if (!dump_drf()) {
        Errs()->add_error("failed writing %s file.",
            lstring::strip_path(filename));
        fclose(tco_fp);
        tco_fp = 0;
        delete [] filename;
        return (false);
    }

    delete [] filename;
    if (tco_fp) {
        fclose(tco_fp);
        tco_fp = 0;
    }
    return (true);
}


// Public function for writing a Cadence Virtuoso compatible GDSII
// layer map file.  The file will be named <basename>.gdsmap.  The
// basename can have a path prepended.  If basename is null, a default
// "xic_tech_cds" is used.
//
bool
cTechCdsOut::write_lmap(const char *basename)
{
    if (!basename || !*basename)
        basename = DEFAULT_BASENAME;
    char *filename = new char[strlen(basename) + 8];
    char *end = lstring::stpcpy(filename, basename);
    strcpy(end, ".gdsmap");

    filestat::clear_error();
    if (!filestat::create_bak(filename)) {
        const char *erm = filestat::error_msg();
        if (erm) {
            Errs()->add_error(erm);
            filestat::clear_error();
        }
        else {
            Errs()->add_error("failed to back up %s.",
                lstring::strip_path(filename));
        }
        delete [] filename;
        return (false);
    }

    tco_fp = fopen(filename, "w");
    if (!tco_fp) {
        Errs()->sys_error(lstring::strip_path(filename));
        Errs()->add_error("failed to open %s for writing.",
            lstring::strip_path(filename));
        delete [] filename;
        return (false);
    }
    fprintf(tco_fp, "#  %s\n", CD()->ifIdString());
    fprintf(tco_fp, "#  GDSII mapping, technology %s\n",
        Tech()->TechnologyName() ? Tech()->TechnologyName() : "unnamed");

    CDlgen pgen(Physical);
    CDl *ld;
    while ((ld = pgen.next()) != 0) {
        if (ld->strmOut()) {
            int l = ld->oaLayerNum();
            const char *lname = CDldb()->getOAlayerName(l);
            if (lname) {
                int p = ld->oaPurposeNum();
                const char *pname = CDldb()->getOApurposeName(p);
                if (!pname)
                    pname = "drawing";
                fprintf(tco_fp, "%-20s %-20s %-6d %d\n", lname,
                    pname, ld->strmOut()->layer(), ld->strmOut()->dtype());
            }
        }
    }

    CDlgen egen(Electrical);
    while ((ld = egen.next()) != 0) {
        if (ld->strmOut()) {
            int l = ld->oaLayerNum();
            const char *lname = CDldb()->getOAlayerName(l);
            if (lname) {
                int p = ld->oaPurposeNum();
                const char *pname = CDldb()->getOApurposeName(p);
                if (!pname)
                    pname = "drawing";
                fprintf(tco_fp, "%-20s %-20s %-6d %d\n", lname,
                    pname, ld->strmOut()->layer(), ld->strmOut()->dtype());
            }
        }
    }

    delete [] filename;
    fclose (tco_fp);
    tco_fp = 0;
    return (true);
}


//
// Remaining functions are private.
//

bool
cTechCdsOut::dump_tech()
{
    const char *errmsg = "failed writing %s block.";
    fprintf(tco_fp, ";  %s\n", CD()->ifIdString());
    if (Tech()->TechnologyName())
        fprintf(tco_fp, ";  Technology: %s\n", Tech()->TechnologyName());
    if (Tech()->TechFilename())
        fprintf(tco_fp, ";  File: %s\n", Tech()->TechFilename());
    fprintf(tco_fp, "\n");

    if (!dump_controls()) {
        Errs()->add_error(errmsg, "controls");
        return (false);
    }
    if (!dump_layerDefinitions()) {
        Errs()->add_error(errmsg, "layerDefinitions");
        return (false);
    }
    if (!dump_layerRules()) {
        Errs()->add_error(errmsg, "layerRules");
        return (false);
    }
    if (!dump_viaDefs()) {
        Errs()->add_error(errmsg, "viaDefs");
        return (false);
    }
    if (!dump_constraintGroups()) {
        Errs()->add_error(errmsg, "constraintGroups");
        return (false);
    }
    if (!dump_devices()) {
        Errs()->add_error(errmsg, "devices");
        return (false);
    }
    if (!dump_viaSpecs()) {
        Errs()->add_error(errmsg, "viaSpecs");
        return (false);
    }
    return (true);
}


bool
cTechCdsOut::dump_controls()
{
    // controls
    //   techParams
    //   techPermissions
    //   viewTypeUnits
    //   mfgGridResolution

    if (!tco_fp)
        return (false);
    fputs("controls(\n", tco_fp);

    string2list *s2 = CDldb()->listAliases();
    if (s2) {
        // Presently, we only know about layer name aliases.
        fputs("  techParams(\n", tco_fp);
        for (string2list *s = s2; s; s = s->next)
            fprintf(tco_fp, "    ( %-19s \"%s\" )\n", s->string, s->value);
        fputs("  )\n\n", tco_fp);
        string2list::destroy(s2);
    }

    fputs("  viewTypeUnits(\n", tco_fp);
    fprintf(tco_fp, "    ( maskLayout       \"micron\" %d )\n",
        INTERNAL_UNITS(1.0));
    fprintf(tco_fp, "    ( schematic        \"micron\" %d )\n",
        ELEC_INTERNAL_UNITS(1.0));
    fprintf(tco_fp, "    ( schematicSymbol  \"micron\" %d )\n",
        ELEC_INTERNAL_UNITS(1.0));
    fputs("  )\n\n", tco_fp);

    fputs("  mfgGridResolution(\n", tco_fp);
    fprintf(tco_fp, "    ( %.6f )\n", Tech()->MfgGrid());
    fputs("  )\n", tco_fp);

    fputs(")\n\n", tco_fp);
    return (!ferror(tco_fp));
}


bool
cTechCdsOut::dump_layerDefinitions()
{
    // layerDefinitions
    //   techPurposes
    //   techLayers
    //   techLayerPurposePriorities
    //   techDisplays
    //   techLayerProperties
    //   techDerivedLayers

    if (!tco_fp)
        return (false);
    fputs("layerDefinitions(\n", tco_fp);
    if (!dump_techPurposes())
        return (false);
    if (!dump_techLayers())
        return (false);
    if (!dump_techLayerPurposePriorities())
        return (false);
    if (!dump_techDisplays())
        return (false);
    if (!dump_techLayerProperties())
        return (false);
    if (!dump_techDerivedLayers())
        return (false);
    fputs(")\n\n", tco_fp);
    return (true);
}


bool
cTechCdsOut::dump_techPurposes()
{
    fputs("  techPurposes(\n", tco_fp);
    stringnumlist *s0 = CDldb()->listOApurposeTab();
    stringnumlist::sort_by_num(s0);
    for (stringnumlist *s = s0; s; s = s->next) {
        stringnumlist *sn = s->next;
        if (sn && sn->num == s->num) {
            const char *nm, *ab;
            if (strlen(s->string) >= strlen(sn->string)) {
                nm = s->string;
                ab = sn->string;
            }
            else {
                nm = sn->string;
                ab = s->string;
            }
            fprintf(tco_fp, "    ( %-24s %6d %s )\n", nm, s->num, ab);
            s = sn;
        }
        else
            fprintf(tco_fp, "    ( %-24s %6d )\n", s->string, s->num);
    }
    stringnumlist::destroy(s0);
    fputs("  )\n\n", tco_fp);
    return (!ferror(tco_fp));
}


bool
cTechCdsOut::dump_techLayers()
{
    fputs("  techLayers(\n", tco_fp);
    stringnumlist *s0 = CDldb()->listOAlayerTab();
    stringnumlist::sort_by_num(s0);
    for (stringnumlist *s = s0; s; s = s->next) {
        stringnumlist *sn = s->next;
        if (sn && sn->num == s->num) {
            const char *nm, *ab;
            if (strlen(s->string) >= strlen(sn->string)) {
                nm = s->string;
                ab = sn->string;
            }
            else {
                nm = sn->string;
                ab = s->string;
            }
            fprintf(tco_fp, "    ( %-24s %6d %s )\n", nm, s->num, ab);
            s = sn;
        }
        else
            fprintf(tco_fp, "    ( %-24s %6d )\n", s->string, s->num);
    }
    stringnumlist::destroy(s0);
    fputs("  )\n\n", tco_fp);
    return (!ferror(tco_fp));
}


bool
cTechCdsOut::dump_techLayerPurposePriorities()
{
    fputs("  techLayerPurposePriorities(\n", tco_fp);
    CDl *ld;
    CDlgen pgen(Physical);
    while ((ld = pgen.next()) != 0) {
        const char *lname = CDldb()->getOAlayerName(ld->oaLayerNum());
        if (!lname)
            continue;
        const char *pname = CDldb()->getOApurposeName(ld->oaPurposeNum());
        if (!pname)
            pname = CDL_PRP_DRAWING;
        fprintf(tco_fp, "    ( %-24s %s )\n", lname, pname);
    }
    CDlgen egen(Electrical);
    while ((ld = egen.next()) != 0) {
        // At least for now, do not write out the standard electrical
        // layers.  These can be considered "internal" layers.
        if (ld->isElecStd())
            continue;
        const char *lname = CDldb()->getOAlayerName(ld->oaLayerNum());
        if (!lname)
            continue;
        const char *pname = CDldb()->getOApurposeName(ld->oaPurposeNum());
        if (!pname)
            pname = CDL_PRP_DRAWING;
        fprintf(tco_fp, "    ( %-24s %s )\n", lname, pname);
    }
    fputs("  )\n\n", tco_fp);
    return (!ferror(tco_fp));
}


bool
cTechCdsOut::dump_techDisplays()
{
    fputs("  techDisplays(\n", tco_fp);
    CDlgen pgen(Physical);
    CDl *ld;
    const char *fmtstr = "    ( %-12s %-12s %-20s %-3s %-3s t t t )\n";
    while ((ld = pgen.next()) != 0) {
        char *colorname = mk_colorname(ld);
        char *stipplename;
        const char *line, *fill;
        dspstrings(ld, &line, &fill, &stipplename);
        const char *lname = CDldb()->getOAlayerName(ld->oaLayerNum());
        if (!lname) {
            delete [] stipplename;
            delete [] colorname;
            continue;
        }
        const char *pname = CDldb()->getOApurposeName(ld->oaPurposeNum());
        if (!pname)
            pname = CDL_PRP_DRAWING;
        char *packetname = mk_packetname(colorname, fill, line);

        // (layer purpose packet is_visible is_selectable is_chglayer
        //  is_drag is_valid)
        fprintf(tco_fp, fmtstr, lname, pname, packetname,
            ld->isInvisible() ? "nil" : "t", ld->isNoSelect() ? "nil" : "t");
        delete [] packetname;
        delete [] stipplename;
        delete [] colorname;
    }
    CDlgen egen(Electrical);
    while ((ld = egen.next()) != 0) {
        char *colorname = mk_colorname(ld);
        char *stipplename;
        const char *line, *fill;
        dspstrings(ld, &line, &fill, &stipplename);
        const char *lname = CDldb()->getOAlayerName(ld->oaLayerNum());
        if (!lname) {
            delete [] stipplename;
            delete [] colorname;
            continue;
        }
        const char *pname = CDldb()->getOApurposeName(ld->oaPurposeNum());
        if (!pname)
            pname = CDL_PRP_DRAWING;
        char *packetname = mk_packetname(colorname, fill, line);

        // (layer purpose packet is_visible is_selectable is_chglayer
        //  is_drag is_valid)
        fprintf(tco_fp, fmtstr, lname, pname, packetname,
            ld->isInvisible() ? "nil" : "t", ld->isNoSelect() ? "nil" : "t");
        delete [] packetname;
        delete [] stipplename;
        delete [] colorname;
    }
    for (CDll *ll = CDldb()->invalidLayers(); ll; ll = ll->next) {
        ld = ll->ldesc;
        char *colorname = mk_colorname(ld);
        char *stipplename;
        const char *line, *fill;
        dspstrings(ld, &line, &fill, &stipplename);
        const char *lname = CDldb()->getOAlayerName(ld->oaLayerNum());
        if (!lname) {
            delete [] stipplename;
            delete [] colorname;
            continue;
        }
        const char *pname = CDldb()->getOApurposeName(ld->oaPurposeNum());
        if (!pname)
            pname = CDL_PRP_DRAWING;
        char *packetname = mk_packetname(colorname, fill, line);

        // (layer purpose packet is_visible is_selectable is_chglayer
        //  is_drag is_valid)
        fprintf(tco_fp, "    ( %-12s %-10s %-16s %-3s %-3s t t nil )\n",
            lname, pname, packetname, ld->isInvisible() ? "nil" : "t",
            ld->isNoSelect() ? "nil" : "t");
        delete [] packetname;
        delete [] stipplename;
        delete [] colorname;
    }
    fputs("  )\n\n", tco_fp);
    return (!ferror(tco_fp));
}


bool
cTechCdsOut::dump_techLayerProperties()
{
    // areaCapacitance   pF/um2
    // areaCapacitance   pF/um2 (to second layer)
    // edgeCapacitance   pF/um
    // edgeCapacitance   pf/Um (to second layer)
    // sheetResistance   ohms/square
    // resistancePerCut  ohms/number of cuts for via
    // height            distance ground plane top to layer bottom
    // thickness         layer thickness in angstroms
    // shrinkage         etch bias
    // capMultiplier     The multiplier for interconnect capacitance to
    //                   account for increases in capacitance caused by
    //                   nearby wires.

    fputs("  techLayerProperties(\n", tco_fp);
    char buf[256];
    CDlgen gen(Physical);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        const char *lname = CDldb()->getOAlayerName(ld->oaLayerNum());
        if (!lname)
            continue;
        const char *pname = CDldb()->getOApurposeName(ld->oaPurposeNum());
        if (pname)
            snprintf(buf, sizeof(buf), "( %s %s )", lname, pname);
        else
            strcpy(buf, lname);

        const char *fmt = "    ( %-20s %-24s %g )\n";
        TechLayerParams *lp = tech_prm(ld);
        if (lp->cap_per_area() > 0.0)
            fprintf(tco_fp, fmt,
                "areaCapacitance", buf, lp->cap_per_area());
        if (lp->cap_per_perim() > 0.0)
            fprintf(tco_fp, fmt,
                "edgeCapacitance", buf, lp->cap_per_perim());
        if (lp->ohms_per_sq() > 0.0)
            fprintf(tco_fp, fmt,
                "sheetResistance", buf, lp->ohms_per_sq());
        DspLayerParams *dp = dsp_prm(ld);
        if (dp->thickness() > 0.0) {
            // in angstroms!
            fprintf(tco_fp, fmt,
                "thickness", buf, 1e4*dp->thickness());
        }
    }
    fputs("  )\n\n", tco_fp);
    return (!ferror(tco_fp));
}


namespace {
    // Convert the layer expression into a node for techDerivedLayers,
    // if possible.  This must be in the form name op name.
    //
    char *parse_drv(const char *expr)
    {
        ParseNode *p = SIparse()->getLexprTree(&expr);
        if (p && p->type == PT_BINOP && p->left && p->left->type == PT_VAR &&
                p->right && p->right->type == PT_VAR) {
            // Name op name form, can't use anything else apparently.
            sLstr lstr;
            lstr.add_c('(');
            lstr.add(p->left->data.v->content.string);
            lstr.add_c(' ');
            if (p->optype == TOK_AND || p->optype == TOK_TIMES)
                lstr.add("'and");
            else if (p->optype == TOK_OR || p->optype == TOK_PLUS)
                lstr.add("'or");
            else if (p->optype == TOK_MINUS)
                lstr.add("'not");
            else if (p->optype == TOK_POWER)
                lstr.add("'xor");
            else {
                ParseNode::destroy(p);
                return (0);
            }
            lstr.add_c(' ');
            lstr.add(p->right->data.v->content.string);
            lstr.add_c(')');
            ParseNode::destroy(p);
            return (lstr.string_trim());
        }
        if (p && p->type == PT_BINOP && p->optype == TOK_AND && p->left &&
                p->left->type == PT_VAR && p->right &&
                p->right->type == PT_UNOP && p->right->optype == TOK_NOT &&
                p->right->left && p->right->left->type == PT_VAR) {
            // Name &! name form, same as name - name.
            sLstr lstr;
            lstr.add_c('(');
            lstr.add(p->left->data.v->content.string);
            lstr.add(" 'and ");
            lstr.add(p->right->left->data.v->content.string);
            lstr.add_c(')');
            ParseNode::destroy(p);
            return (lstr.string_trim());
        }
        ParseNode::destroy(p);
        return (0);
    }
}


bool
cTechCdsOut::dump_techDerivedLayers()
{
    CDl **ary = CDldb()->listDerivedLayers();
    if (ary) {
        fputs("  techDerivedLayers(\n", tco_fp);
        for (int i = 0; ary[i]; i++) {
            char *expr = parse_drv(ary[i]->drvExpr());
            if (!expr)
                continue;
            fprintf(tco_fp, "    ( %-12s %-5d %s )\n", ary[i]->name(),
                ary[i]->drvIndex(), expr);
            delete [] expr;
        }
        fputs("  )\n", tco_fp);
        delete [] ary;
    }
    return (!ferror(tco_fp));
}


bool
cTechCdsOut::dump_layerRules()
{
    // functions
    // routingDirections
    // stampLabelLayers
    // currentDensityTables

    fputs("layerRules(\n", tco_fp);
    fputs("  routingDirections(\n", tco_fp);
    CDlgen pgen(Physical);
    CDl *ld;
    while ((ld = pgen.next()) != 0) {
        if (ld->isRouting()) {
            int rdir = tech_prm(ld)->route_dir();
            const char *dstr = "none";
            if (rdir == tDirVert)
                dstr = "vertical";
            else if (rdir == tDirHoriz)
                dstr = "horizontal";

            fprintf(tco_fp, "    ( %-24s \"%s\" )\n", ld->name(), dstr);
        }
    }
    fputs("  )\n", tco_fp);
    fputs(")\n\n", tco_fp);
    return (!ferror(tco_fp));
}


bool
cTechCdsOut::dump_viaDefs()
{
    // standardViaDefs
    // customViaDefs

    fputs("viaDefs(\n", tco_fp);
    fputs("  standardViaDefs(\n", tco_fp);

    char buf[256];
    tgen_t<sStdVia> gen(Tech()->StdViaTab());
    const sStdVia *sv;
    while ((sv = gen.next()) != 0) {
        if (!sv->via() || !sv->bottom() || !sv->top())
            continue;
        fprintf(tco_fp, "    ( %-16s %-10s %-10s (\"%s\" %g %g)\n",
            sv->tab_name(), sv->bottom()->name(), sv->top()->name(),
            sv->via()->name(),
            MICRONS(sv->via_wid()), MICRONS(sv->via_hei()));
        fprintf(tco_fp, "      (%d %d (%g %g))\n",
            sv->via_rows(), sv->via_cols(),
            MICRONS(sv->via_spa_x()), MICRONS(sv->via_spa_y()));
        snprintf(buf, sizeof(buf), "(%g %g)",
            MICRONS(sv->bot_enc_x()), MICRONS(sv->bot_enc_y()));
        fprintf(tco_fp, "      %-12s", buf);
        snprintf(buf, sizeof(buf), "(%g %g)",
            MICRONS(sv->top_enc_x()), MICRONS(sv->top_enc_y()));
        fprintf(tco_fp, " %-12s", buf);
        snprintf(buf, sizeof(buf), "(%g %g)",
            MICRONS(sv->bot_off_x()), MICRONS(sv->bot_off_y()));
        fprintf(tco_fp, " %-12s", buf);
        snprintf(buf, sizeof(buf), "(%g %g)",
            MICRONS(sv->top_off_x()), MICRONS(sv->top_off_y()));
        fprintf(tco_fp, " %-12s", buf);
        snprintf(buf, sizeof(buf), "(%g %g)",
            MICRONS(sv->org_off_x()), MICRONS(sv->org_off_y()));
        fprintf(tco_fp, " %-12s", buf);
        if (sv->implant1()) {
            snprintf(buf, sizeof(buf), "(%g %g)",
                MICRONS(sv->imp1_enc_x()), MICRONS(sv->imp1_enc_y()));
            fprintf(tco_fp, "\n      %-16s %12s",
                sv->implant1()->name(), buf);
            if (sv->implant2()) {
                snprintf(buf, sizeof(buf), "(%g %g)",
                    MICRONS(sv->imp2_enc_x()),
                    MICRONS(sv->imp2_enc_y()));
                fprintf(tco_fp, "\n      %-16s %12s",
                    sv->implant2()->name(), buf);
            }
        }
        fprintf(tco_fp, "\n    )\n");
    }
    fputs("  )\n", tco_fp);
    fputs(")\n\n", tco_fp);
    return (!ferror(tco_fp));
}


bool
cTechCdsOut::dump_constraintGroups()
{
    fputs("constraintGroups(\n", tco_fp);

    fputs("  ( \"virtuosoDefaultSetup\"  nil\n", tco_fp);
    fputs("    interconnect(\n", tco_fp);
    fputs("     ( validLayers  ", tco_fp);
    {
        sLstr lstr;
        CDextLgen lgen(CDL_CONDUCTOR, CDextLgen::TopToBot);
        CDl *ld;
        while ((ld = lgen.next()) != 0) {
            lstr.add_c(' ');
            lstr.add(ld->name());
        }
        fprintf(tco_fp, "(%s ))\n", lstr.string());
    }
    fputs("     ( validVias    ", tco_fp);
    {
        sLstr lstr;
        CDextLgen lgen(CDL_VIA, CDextLgen::TopToBot);
        CDl *ld;
        while ((ld = lgen.next()) != 0) {
            lstr.add_c(' ');
            lstr.add(ld->name());
        }
        fprintf(tco_fp, "(%s ))\n", lstr.string());
    }
    fputs("    )\n", tco_fp);
    fputs("  )\n", tco_fp);

    fputs("  ( \"LEFDefaultRoutingSpec\"  nil\n", tco_fp);
    fputs("    interconnect(\n", tco_fp);
    fputs("     ( validLayers  ", tco_fp);
    {
        sLstr lstr;
        CDextLgen lgen(CDL_ROUTING, CDextLgen::TopToBot);
        CDl *ld;
        while ((ld = lgen.next()) != 0) {
            lstr.add_c(' ');
            lstr.add(ld->name());
        }
        fprintf(tco_fp, "(%s ))\n", lstr.string());
    }
    fputs("     ( validVias    ", tco_fp);
    {
        sLstr lstr;
        CDextLgen lgen(CDL_VIA, CDextLgen::TopToBot);
        CDl *ld;
        while ((ld = lgen.next()) != 0) {
            // List only vias between routing layers.
            bool goodone = false;
            for (sVia *via = tech_prm(ld)->via_list(); via;
                    via = via->next()) {
                CDl *ld1 = via->layer1();
                CDl *ld2 = via->layer2();
                if (!ld1 || !ld2 || (ld1 == ld2))
                    continue;
                if (!ld1->isRouting() || !ld2->isRouting())
                    continue;
                goodone = true;
                break;
            }
            if (goodone) {
                lstr.add_c(' ');
                lstr.add(ld->name());
            }
        }
        fprintf(tco_fp, "(%s ))\n", lstr.string());
    }
    fputs("    )\n", tco_fp);
    fputs("    routingGrids(\n", tco_fp);
    {
        CDextLgen lgen(CDL_ROUTING);
        CDl *ld;
        while ((ld = lgen.next()) != 0) {
            const char *fmt = "     ( %-27s \"%s\" %*c%g )\n";
            const char *lnm = ld->name();
            int nsp = 7 - (strlen(lnm) + 2);
            if (nsp < 0)
                nsp = 0;
            if (tech_prm(ld)->route_v_pitch() > 0) {
                fprintf(tco_fp, fmt, "verticalPitch", lnm, nsp, ' ',
                    MICRONS(tech_prm(ld)->route_v_pitch()));
            }
            if (tech_prm(ld)->route_h_pitch() > 0) {
                fprintf(tco_fp, fmt, "horizontalPitch", lnm, nsp, ' ',
                    MICRONS(tech_prm(ld)->route_h_pitch()));
            }
            if (tech_prm(ld)->route_v_offset() > 0) {
                fprintf(tco_fp, fmt, "verticalOffset", lnm, nsp, ' ',
                    MICRONS(tech_prm(ld)->route_v_offset()));
            }
            if (tech_prm(ld)->route_h_offset() > 0) {
                fprintf(tco_fp, fmt, "horizontalOffset", lnm, nsp, ' ',
                    MICRONS(tech_prm(ld)->route_h_offset()));
            }
        }
    }
    fputs("    )\n", tco_fp);
//    fputs("    placementGrids(\n", tco_fp);
//    fputs("    )\n", tco_fp);
    fputs("  )\n", tco_fp);

    fputs("  ( \"foundry\"  nil\n", tco_fp);
    char *sp = DrcIf()->minSpaceTables("      ");
    if (sp) {
        fputs("    spacingTables(\n", tco_fp);
        fprintf(tco_fp, "%s", sp);
        fputs("    )\n", tco_fp);
        delete [] sp;
    }
    sp = DrcIf()->spacings("      ");
    if (sp) {
        fputs("    spacings(\n", tco_fp);
        fprintf(tco_fp, "%s", sp);
        fputs("    )\n", tco_fp);
        delete [] sp;
    }
    sp = DrcIf()->orderedSpacings("      ");
    if (sp) {
        fputs("    orderedSpacings(\n", tco_fp);
        fprintf(tco_fp, "%s", sp);
        fputs("    )\n", tco_fp);
        delete [] sp;
    }
    fputs("  )\n", tco_fp);
    fputs(")\n\n", tco_fp);
    return (!ferror(tco_fp));
}


bool
cTechCdsOut::dump_devices()
{
    return (true);
}


bool
cTechCdsOut::dump_viaSpecs()
{
    return (true);
}


namespace {
    char *stipstr(GRfillType *fill, const char *pref)
    {
        sLstr lstr;
        unsigned char *map = fill->newBitmap();
        unsigned char *a = map;
        int bpl = (fill->nX() + 7)/8;
        for (unsigned int i = 0; i < fill->nY(); i++) {
            unsigned int d = *a++;
            for (int j = 1; j < bpl; j++)
                d |= *a++ <<  8*j;

            lstr.add(pref);
            lstr.add("(");
            unsigned int mask = 1;
            for (unsigned int j = 0; j < fill->nX(); j++) {
                lstr.add_c((d & mask) ? '1' : '0');
                mask <<= 1;
                if (j == fill->nX() - 1)
                    lstr.add(")\n");
                else
                    lstr.add_c(' ');
            }
        }
        delete [] map;
        return (lstr.string_trim());
    }
}


bool
cTechCdsOut::dump_drf()
{
    fprintf(tco_fp, ";  %s\n\n", CD()->ifIdString());

    // drDefineDisplay
    fputs("drDefineDisplay(\n", tco_fp);
    fputs("  ( display )\n", tco_fp);
    fputs(")\n\n", tco_fp);

    if (!dump_drDefineColor())
        return (false);
    if (!dump_drDefineStipple())
        return (false);
    if (!dump_drDefineLineStyle())
        return (false);
    if (!dump_drDefinePacket())
        return (false);
    return (true);
}


bool
cTechCdsOut::dump_drDefineColor()
{
    fputs("drDefineColor(\n", tco_fp);
    SymTab ctab(true, false);
    CDlgen pgen(Physical);
    CDl *ld;
    while ((ld = pgen.next()) != 0) {
        DspLayerParams *lp = dsp_prm(ld);
        char *colorname = mk_colorname(ld);
        if (SymTab::get(&ctab, colorname) != ST_NIL) {
            delete [] colorname;
            continue;
        }
        fprintf(tco_fp, "  ( %s %-10s %-3d %-3d %-3d )\n", "display",
            colorname, lp->red(), lp->green(), lp->blue());
        ctab.add(colorname, 0, false);
    }
    CDlgen egen(Electrical);
    while ((ld = egen.next()) != 0) {
        DspLayerParams *lp = dsp_prm(ld);
        char *colorname = mk_colorname(ld);
        if (SymTab::get(&ctab, colorname) != ST_NIL) {
            delete [] colorname;
            continue;
        }
        fprintf(tco_fp, "  ( %s %-10s %-3d %-3d %-3d )\n", "display",
            colorname, lp->red(), lp->green(), lp->blue());
        ctab.add(colorname, 0, false);
    }
    fputs(")\n\n", tco_fp);
    return (!ferror(tco_fp));
}


bool
cTechCdsOut::dump_drDefineStipple()
{
    fputs("drDefineStipple(\n", tco_fp);
    fprintf(tco_fp, "  (%s %s (\n", "display", "solid");
    for (int i = 0; i < 16; i++)
        fputs("    (1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1)\n", tco_fp);
    fputs("  ))\n", tco_fp);
    SymTab tab(true, true);
    CDlgen pgen(Physical);
    CDl *ld;
    while ((ld = pgen.next()) != 0) {
        DspLayerParams *lp = dsp_prm(ld);
        if (ld->isFilled() && lp->fill()->hasMap()) {

            GRfillType *fill = lp->fill();
            char *ss = stipstr(fill, "    ");
            if (!ss)
                continue;
            if (SymTab::get(&tab, ss) != ST_NIL) {
                delete [] ss;
                continue;
            }
            char *stipplename = mk_stipplename(ld);
            fprintf(tco_fp, "  ( %s %s (\n", "display", stipplename);
            fprintf(tco_fp, "%s", ss);
            fputs("  ))\n", tco_fp);
            tab.add(ss, stipplename, false);
        }
    }
    CDlgen egen(Electrical);
    while ((ld = egen.next()) != 0) {
        DspLayerParams *lp = dsp_prm(ld);
        if (ld->isFilled() && lp->fill()->hasMap()) {

            GRfillType *fill = lp->fill();
            char *ss = stipstr(fill, "    ");
            if (!ss)
                continue;
            if (SymTab::get(&tab, ss) != ST_NIL) {
                delete [] ss;
                continue;
            }
            char *stipplename = mk_stipplename(ld);
            fprintf(tco_fp, "  ( %s %s (\n", "display", stipplename);
            fprintf(tco_fp, "%s", ss);
            fputs("  ))\n", tco_fp);
            tab.add(ss, stipplename, false);
        }
    }
    fputs(")\n\n", tco_fp);
    return (!ferror(tco_fp));
}


bool
cTechCdsOut::dump_drDefineLineStyle()
{
    fputs("drDefineLineStyle(\n", tco_fp);
    fputs("  ( display solid 1 (1 1 1) )\n", tco_fp);
    fputs("  ( display dashed 1 (1 1 1 0 0 0) )\n", tco_fp);
    fputs("  ( display thick 3 (1 1 1) )\n", tco_fp);
    fputs(")\n\n", tco_fp);
    return (!ferror(tco_fp));
}


bool
cTechCdsOut::dump_drDefinePacket()
{
    fputs("drDefinePacket(\n", tco_fp);
    SymTab *tab = new SymTab(true, false);
    char buf[256];

    CDlgen pgen(Physical);
    CDl *ld;
    while ((ld = pgen.next()) != 0) {
        char *colorname = mk_colorname(ld);
        char *stipplename;
        const char *line, *fill;
        dspstrings(ld, &line, &fill, &stipplename);
        char *packetname = mk_packetname(colorname, fill, line);
        if (SymTab::get(tab, packetname) == ST_NIL) {
            snprintf(buf, sizeof(buf), "  ( display %s %s %s %s %s )\n",
                packetname, fill, line, colorname, colorname);
            tab->add(packetname, lstring::copy(buf), false);
        }
        else
            delete [] packetname;
        delete [] stipplename;
        delete [] colorname;
    }
    CDlgen egen(Electrical);
    while ((ld = egen.next()) != 0) {
        char *colorname = mk_colorname(ld);
        char *stipplename;
        const char *line, *fill;
        dspstrings(ld, &line, &fill, &stipplename);
        char *packetname = mk_packetname(colorname, fill, line);
        if (SymTab::get(tab, packetname) == ST_NIL) {
            snprintf(buf, sizeof(buf), "  ( display %s %s %s %s %s )\n",
                packetname, fill, line, colorname, colorname);
            tab->add(packetname, lstring::copy(buf), false);
        }
        else
            delete [] packetname;
        delete [] stipplename;
        delete [] colorname;
    }
    SymTabGen gen(tab);
    SymTabEnt *ent;
    while ((ent = gen.next()) != 0)
        fputs((const char*)ent->stData, tco_fp);
    delete tab;
    fputs(")\n", tco_fp);
    return (!ferror(tco_fp));
}


// Find and return the line, fill, and stipple names.
//
void
cTechCdsOut::dspstrings(const CDl *ld, const char **pline, const char **pfill,
    char **pstip)
{
    char *stipplename = 0;
    const char *line = "solid";
    const char *fill = "none";
    if (ld->isFilled()) {
        DspLayerParams *lp = dsp_prm(ld);
        if (!lp->fill()->hasMap())
            fill = "solid";
            // As in TSMC tech file, keep line solid, too.
        else {
            stipplename = mk_stipplename(ld);
            fill = stipplename;
            if (ld->isOutlined()) {
                if (ld->isOutlinedFat())
                    line = "thick";
            }
            else
                line = "none";
            if (ld->isCut())
                fill = "X";
        }
    }
    else {
        if (ld->isOutlined()) {
            if (ld->isOutlinedFat())
                line = "thick";
            else
                line = "dashed";
        }
        if (ld->isCut())
            fill = "X";
    }
    *pline = line;
    *pfill = fill;
    *pstip = stipplename;
}


// Create a color name.
//
char *
cTechCdsOut::mk_colorname(const CDl *ld)
{
    DspLayerParams *lp = dsp_prm(ld);
    const sDrfColor *c = DrfIn()->find_color(lp->red(), lp->green(), lp->blue());
    if (c)
        return (lstring::copy(c->name()));

    char buf[32];
    snprintf(buf, sizeof(buf),  "c%02x%02x%02x", lp->red(), lp->green(),
        lp->blue());
    return (lstring::copy(buf));
}


// Create a stipple name.
//
char *
cTechCdsOut::mk_stipplename(const CDl *ld)
{
    GRfillType *fill = dsp_prm(ld)->fill();
    const sDrfStipple *st = DrfIn()->find_stipple(fill);
    if (st)
        return (lstring::copy(st->name()));

    // We'll use the drf input reader to keep track of stipple
    // patterns that were not read from an external drf file.

    char buf[32];
    snprintf(buf, sizeof(buf), "stp_%d", tco_stip_cnt);

    unsigned char *map = fill->newBitmap();
    DrfIn()->add_stipple(new sDrfStipple(buf, fill->nX(), fill->nY(), map));

    tco_stip_cnt++;
    return (lstring::copy(buf));
}


// Construct a packet name.
//
char *
cTechCdsOut::mk_packetname(const char *color, const char *stipple,
    const char *line)
{
    const sDrfPacket *p = DrfIn()->find_packet(color, stipple, line, color);
    if (p)
        return (lstring::copy(p->name()));

    return (cTechDrfIn::mk_packet_name(color, stipple, line, color));
}

