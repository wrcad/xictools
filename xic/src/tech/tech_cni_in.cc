
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
 $Id: tech_cni_in.cc,v 1.15 2017/03/19 01:58:48 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "si_parsenode.h"
#include "si_lisp.h"
#include "tech.h"
#include "tech_cni_in.h"
#include "tech_drf_in.h"
#include "errorlog.h"
#include "main_variables.h"


// Compatibility function for Ciranova tech files.
//

cTechCniIn *cTechCniIn::instancePtr = 0;

cTechCniIn::cTechCniIn()
{
    if (instancePtr) {
        fprintf(stderr,
            "Singleton class cTechCniIn is already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    // Santana display
    register_node("dispLayerPurposePairs",  dispLayerPurposePairs);

    // Santana tech
    register_node("techId",                 techId);
    register_node("viewTypeUnits",          viewTypeUnits);
    register_node("mfgGridResolution",      mfgGridResolution);
    register_node("layerMapping",           layerMapping);
    register_node("maskNumbers",            maskNumbers);
    register_node("layerMaterials",         layerMaterials);
    register_node("purposeMapping",         purposeMapping);
    register_node("derivedLayers",          derivedLayers);
    register_node("connectivity",           connectivity);
    register_node("viaLayers",              viaLayers);
    register_node("physicalRules",          physicalRules);
    register_node("deviceContext",          deviceContext);
    register_node("characterizationRules",  characterizationRules);
    register_node("oxideDefinitions",       oxideDefinitions);
    register_node("mosfetDefinitions",      mosfetDefinitions);
}


cTechCniIn::~cTechCniIn()
{
    instancePtr = 0;
}


// Main entry to read a Ciranova technology file.
//
bool
cTechCniIn::read(const char *fname, char **err)
{
    return (readEvalLisp(fname, CDvdb()->getVariable(VA_LibPath), true, err));
}


namespace {
    const char *compat = "compat";

    void
    err_rpt(const char *name, lispnode *p)
    {
        sLstr lstr;
        lispnode::print(p, &lstr);
        if (name)
            Log()->WarningLogV(compat, "Ignored bad node in %s:\n\t%s\n",
                name, lstr.string());
        else
            Log()->WarningLogV(compat, "Ignored bad node:\n\t%s\n",
                lstr.string());
    }

    // Defaults for presentation attributes.
#define DEF_STIPPLE "none"
#define DEF_LSTYLE  "solid"
#define DEF_COLOR   "white"
#define DEF_PACKET  "defaultPacket"

    sDrfColor defaultColor(DEF_COLOR, 255, 255, 255, false);

    sDrfPacket defaultPacket(DEF_PACKET, DEF_STIPPLE, DEF_LSTYLE,
        DEF_COLOR, DEF_COLOR);
}


bool
cTechCniIn::dispLayerPurposePairs(lispnode *p0, lispnode*, char **err)
{
    // Fill outlines that we support.
    enum outl_type { outl_none, outl_thin, outl_thick, outl_dashed };

    if (!DrfIn()) {
        Log()->WarningLog(compat,
            "No display resource records found for dispLayerPurposePairs.");
        return (true);
    }

    stringlist *badpkts = 0;
    stringlist *badstips = 0;
    stringlist *badclrs = 0;

    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[5];
        int cnt = lispnode::eval_list(p->args, n, 5, err);
        if (cnt < 5) {
            err_rpt("dispLayerPurposePairs", p);
            continue;
        }
        // n[0]: layer name
        // n[1]: purpose
        // n[2]: packet
        // n[3]: visible        (t or nil)
        // n[4]: selectable     (t or nil)

        if (n[0].type != LN_STRING || n[1].type != LN_STRING ||
                n[2].type != LN_STRING) {
            err_rpt("dispLayerPurposePairs", p);
            continue;
        }

        unsigned int lnum = CDldb()->getOAlayerNum(n[0].string);
        if (lnum == CDL_NO_LAYER)
            continue;
        bool unknown;
        unsigned int pnum =
            CDldb()->getOApurposeNum(n[1].string, &unknown);
        if (pnum == CDL_PRP_DRAWING_NUM && unknown)
            continue;

        CDl *ld = CDldb()->newLayer(lnum, pnum, Physical);
        if (!ld) {
            Log()->WarningLog(compat, Errs()->get_error());
            continue;
        }
        ld->setInvisible(n[3].is_nil());
        ld->setRstInvisible(n[3].is_nil());
        ld->setNoSelect(n[4].is_nil());
        ld->setRstNoSelect(n[4].is_nil());

        const sDrfPacket *pk = DrfIn()->find_packet(n[2].string);
        if (!pk) {
            badpkts = new stringlist(lstring::copy(n[2].string), badpkts);
            pk = DrfIn()->find_packet(DEF_PACKET);
        }
        if (!pk)
            pk = &defaultPacket;

        const sDrfStipple *st = 0;
        bool solid = false;
        bool cut = false;
        if (!strcasecmp(pk->stipple_name(), "solid"))
            solid = true;
        else if (!strcasecmp(pk->stipple_name(), "none"))
            ;
        else if (!strcasecmp(pk->stipple_name(), "X"))
            // Should draw 'X' over boxes.
            cut = true;
        else {
            st = DrfIn()->find_stipple(pk->stipple_name());
            if (!st) {
                badstips = new stringlist(lstring::copy(pk->stipple_name()),
                    badstips);
                st = DrfIn()->find_stipple(DEF_STIPPLE);
            }
            // else none
        }

        const sDrfColor *fc = DrfIn()->find_color(pk->fill_color());
        if (!fc) {
            badclrs = new stringlist(lstring::copy(pk->fill_color()),
                badclrs);
            fc = DrfIn()->find_color(DEF_COLOR);
        }
        if (!fc)
            fc = &defaultColor;

        outl_type outl = outl_none;
        if (!strcasecmp(pk->linestyle_name(), "none"))
            ;
        else if (!strcasecmp(pk->linestyle_name(), "solid"))
            outl = outl_thin;
        else {
            const sDrfLine *ls = DrfIn()->find_line(pk->linestyle_name());
            if (ls) {
                if (ls->size() > 1)
                    outl = outl_thick;
                else
                    outl = outl_dashed;
            }
            else {
                if (lstring::ciprefix("thick", pk->linestyle_name()))
                    outl = outl_thick;
                else
                    outl = outl_dashed;
            }
        }

        // We don't presently support a stipple with a textured
        // outline.

        if (solid)
            outl = outl_none;
        if (outl == outl_dashed && st)
            outl = outl_thin;

        dsp_prm(ld)->set_red(fc->red());
        dsp_prm(ld)->set_green(fc->green());
        dsp_prm(ld)->set_blue(fc->blue());
        int pix = 0;
        DSPmainDraw(DefineColor(&pix, fc->red(), fc->green(), fc->blue()))
        dsp_prm(ld)->set_pixel(pix);

        ld->setFilled(false);
        ld->setOutlined(false);
        ld->setOutlinedFat(false);
        ld->setCut(false);

        if (solid || st)
            ld->setFilled(true);
        if (st) {
            DSPmainDraw(defineFillpattern(dsp_prm(ld)->fill(),
                st->nx(), st->ny(), st->map()))
        }
        if (!solid) {
            if (outl == outl_thick) {
                ld->setOutlined(true);
                ld->setOutlinedFat(true);
            }
            else if (outl == outl_dashed)
                ld->setOutlined(true);
            else if (outl == outl_thin) {
                if (st)
                    ld->setOutlined(true);
            }
            if (cut)
                ld->setCut(true);
        }
    }

    sLstr lstr;
    if (badpkts) {
        lstr.add(
            "Packet names referenced in technology but not defined in drf:\n");
        SymTab tab(false, false);
        for (stringlist *sl = badpkts; sl; sl = sl->next) {
            if (tab.get(sl->string) == ST_NIL)
                tab.add(sl->string, 0, false);
        }
        stringlist *names = tab.names();
        stringlist::sort(names);
        for (stringlist *sl = names; sl; sl = sl->next) {
            lstr.add("    ");
            lstr.add(sl->string);
            lstr.add_c('\n');
        }
        stringlist::destroy(names);
        stringlist::destroy(badpkts);
    }
    if (badstips) {
        lstr.add(
            "Stipple names referenced in technology but not defined in drf:\n");
        SymTab tab(false, false);
        for (stringlist *sl = badstips; sl; sl = sl->next) {
            if (tab.get(sl->string) == ST_NIL)
                tab.add(sl->string, 0, false);
        }
        stringlist *names = tab.names();
        stringlist::sort(names);
        for (stringlist *sl = names; sl; sl = sl->next) {
            lstr.add("    ");
            lstr.add(sl->string);
            lstr.add_c('\n');
        }
        stringlist::destroy(names);
        stringlist::destroy(badstips);
    }
    if (badclrs) {
        lstr.add(
            "Color names referenced in technology but not defined in drf:\n");
        SymTab tab(false, false);
        for (stringlist *sl = badclrs; sl; sl = sl->next) {
            if (tab.get(sl->string) == ST_NIL)
                tab.add(sl->string, 0, false);
        }
        stringlist *names = tab.names();
        stringlist::sort(names);
        for (stringlist *sl = names; sl; sl = sl->next) {
            lstr.add("    ");
            lstr.add(sl->string);
            lstr.add_c('\n');
        }
        stringlist::destroy(names);
        stringlist::destroy(badclrs);
    }
    if (lstr.string() && CDvdb()->getVariable(VA_DrfDebug))
        Log()->WarningLog(compat, lstr.string());

    return (true);
}


bool
cTechCniIn::techId(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCniIn::viewTypeUnits(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[3];
        int cnt = lispnode::eval_list(p->args, n, 3, err);
        if (cnt < 3) {
            err_rpt("viewTypeUnits", p);
            continue;
        }
        // n[0]: view-type
        // n[1]: user-unit
        // n[2]: DBU-per-UU

        if (n[0].type != LN_STRING || n[1].type != LN_STRING ||
                n[2].type != LN_NUMERIC) {
            err_rpt("viewTypeUnits", p);
            continue;
        }
        if (!strcmp(n[0].string, "maskLayout")) {
            if (strcasecmp(n[1].string, "micron")) {
                Log()->WarningLogV(compat,
                    "Unsupported db unit type %s.\n", n[1].string);
                continue;
            }
            int res = mmRnd(n[2].value);
            if (res == 2000)
                CDvdb()->setVariable(VA_DatabaseResolution, "2000");
            else if (res == 5000)
                CDvdb()->setVariable(VA_DatabaseResolution, "5000");
            else if (res == 10000)
                CDvdb()->setVariable(VA_DatabaseResolution, "10000");
            else if (res != 1000) {
                Log()->WarningLogV(compat,
                    "Unsupported db unit scale %d.\n", res);
                continue;
            }
        }
    }
    return (true);
}


bool
cTechCniIn::mfgGridResolution(lispnode *p0, lispnode*, char **err)
{
    lispnode n[1];
    lispnode *p = p0->args;
    int cnt = lispnode::eval_list(p->args, n, 1, err);
    if (cnt < 1) {
        err_rpt("mfgGridResolution", p);
        return (true);
    }
    // n[0]: mfg grid
    if (n[0].type != LN_NUMERIC) {
        err_rpt("mfgGridResolution", p);
        return (true);
    }
    Tech()->SetMfgGrid(n[0].value);
    return (true);
}


bool
cTechCniIn::layerMapping(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[3];
        int cnt = lispnode::eval_list(p->args, n, 2, err);
        if (cnt < 2) {
            err_rpt("layerMapping", p);
            continue;
        }
        // n[0]: layer name
        // n[1]: layer number

        if (n[0].type != LN_STRING || n[1].type != LN_NUMERIC) {
            err_rpt("layerMapping", p);
            continue;
        }
        CDldb()->saveOAlayer(n[0].string, mmRnd(n[1].value), 0);
    }
    return (true);
}


bool
cTechCniIn::maskNumbers(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCniIn::layerMaterials(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCniIn::purposeMapping(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[3];
        int cnt = lispnode::eval_list(p->args, n, 3, err);
        if (cnt < 2) {
            err_rpt("purposeMapping", p);
            continue;
        }
        // n[0]: purpose name
        // n[1]: purpose number

        if (n[0].type != LN_STRING || n[1].type != LN_NUMERIC) {
            err_rpt("purposeMapping", p);
            continue;
        }
        CDldb()->saveOApurpose(n[0].string, mmRnd(n[1].value), 0);
    }
    return (true);
}


bool
cTechCniIn::derivedLayers(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCniIn::connectivity(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCniIn::viaLayers(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCniIn::physicalRules(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCniIn::deviceContext(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCniIn::characterizationRules(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCniIn::oxideDefinitions(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCniIn::mosfetDefinitions(lispnode*, lispnode*, char**)
{
    return (true);
}

