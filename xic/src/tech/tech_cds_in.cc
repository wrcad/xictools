
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
 $Id: tech_cds_in.cc,v 1.65 2017/03/17 04:35:02 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "si_parsenode.h"
#include "si_lisp.h"
#include "tech.h"
#include "tech_cds_in.h"
#include "tech_drf_in.h"
#include "tech_layer.h"
#include "tech_spacetab.h"
#include "tech_via.h"
#include "errorlog.h"
#include "main_variables.h"
#include "extif.h"
#include "drcif.h"


//#define DEBUG

//-----------------------------------------------------------------------------
// cTechCdsIn  Cadence Virtuoso ascii tech file reader

// From Cadence documentation: SKILL Reference Manual 4.4.6, chpt 7.
// The following classes are defined in the reference and are handled
// explicitly.
//
// Nodes added for Virtuoso 6.1.4 for tsmcN65 marked with '*'.
//
//    include
//    comment
//    controls
//        techParams
//        techPermissions
//        viewTypeUnits *
//        mfgGridResolution *  (also in physicalRules)
//    layerDefinitions
//        techLayers
//        techPurposes
//        techLayerPurposePriorities
//        techDisplays
//        techLayerProperties
//        techDerivedLayers *
//    layerRules
//        functions *
//        routingDirections *
//        stampLabelLayers *
//        currentDensityTables *
//        viaLayers
//        equivalentLayers
//        streamLayers
//    viaDefs *
//        standardViaDefs *
//        customViaDefs *
//    constraintGroups *
//        foundry *
//            spacings *
//            viaStackLimits *
//            spacingTables *
//            orderedSpacings *
//            antennaModels *
//            electrical *
//            memberConstraintGroups *
//        LEFDefaultRouteSpec
//            interconnect
//            routingGrids
//    devices
//        tcCreateCDSDeviceClass
//        multipartPathTemplates *
//        extractMOS *
//        extractRES *
//        symContactDevice
//        ruleContactDevice
//        symEnhancementDevice
//        symDepletionDevice
//        symPinDevice
//        symRectPinDevice
//        tcCreateDeviceClass
//        tcDeclareDevice
//        tfcDefineDeviceClassProp
//    viaSpecs *
//----  below is all V5 stuff ----
//    physicalRules
//        orderedSpacingRules
//        spacingRules
//        mfgGridResolution
//    electricalRules
//        characterizationRules
//        orderedCharacterizationRules
//    leRules
//        leLswLayers
//    lxRules
//        lxExtractLayers
//        lxNoOverlapLayers
//        lxMPPTemplates
//    compactorRules
//        compactorLayers
//        symWires
//        symRules
//    lasRules
//        lasLayers
//        lasDevices
//        lasWires
//        lasProperties
//    prRules
//        prRoutingLayers
//        prViaTypes
//        prStackVias
//        prMastersliceLayers
//        prViaRules
//        prGenViaRules
//        prTurnViaRules
//        prNonDefaultRules
//        prRoutingPitch
//        prRoutingOffset
//        prOverlapLayer

cTechCdsIn *cTechCdsIn::instancePtr = 0;

cTechCdsIn::cTechCdsIn()
{
    if (instancePtr) {
        fprintf(stderr,
            "Singleton class cTechCdsIn is already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    // Technology File support
    register_node("comment",                comment);
    register_node("include",                include_node);

    register_node("controls",               class_dispatch);
    register_node("techParams",             techParams);
    register_node("techPermissions",        techPermissions);
    register_node("viewTypeUnits",          viewTypeUnits);
    register_node("mfgGridResolution",      mfgGridResolution);

    register_node("layerDefinitions",       class_dispatch);
    register_node("techLayers",             techLayers);
    register_node("techPurposes",           techPurposes);
    register_node("techLayerPurposePriorities", techLayerPurposePriorities);
    register_node("techDisplays",           techDisplays);
    register_node("techLayerProperties",    techLayerProperties);
    register_node("techDerivedLayers",      techDerivedLayers);

    register_node("layerRules",             class_dispatch);
    register_node("functions",              functions);
    register_node("routingDirections",      routingDirections);
    register_node("stampLabelLayers",       stampLabelLayers);
    register_node("currentDensityTables",   stampLabelLayers);
    register_node("viaLayers",              viaLayers);
    register_node("equivalentLayers",       equivalentLayers);
    register_node("streamLayers",           streamLayers);

    register_node("viaDefs",                class_dispatch);
    register_node("standardViaDefs",        standardViaDefs);
    register_node("customViaDefs",          customViaDefs);

    register_node("constraintGroups",       constraintGroups);
    register_node("spacings",               spacings);
    register_node("viaStackingLimits",      viaStackingLimits);
    register_node("spacingTables",          spacingTables);
    register_node("orderedSpacings",        orderedSpacings);
    register_node("antennaModels",          antennaModels);
    register_node("electrical",             electrical);
    register_node("memberConstraintGroups", memberConstraintGroups);
    register_node("interconnect",           interconnect);
    register_node("routingGrids",           routingGrids);

    register_node("devices",                class_dispatch);
    register_node("tcCreateCDSDeviceClass", tcCreateCDSDeviceClass);
    register_node("multipartPathTemplates", multipartPathTemplates);
    register_node("extractMOS",             extractMOS);
    register_node("extractRES",             extractRES);
    register_node("symContactDevice",       symContactDevice);
    register_node("ruleContactDevice",      ruleContactDevice);
    register_node("symEnhancementDevice",   symEnhancementDevice);
    register_node("symDepletionDevice",     symDepletionDevice);
    register_node("symPinDevice",           symPinDevice);
    register_node("symRectPinDevice",       symRectPinDevice);
    register_node("tcCreateDeviceClass",    tcCreateDeviceClass);
    register_node("tcDeclareDevice",        tcDeclareDevice);
    register_node("tfcDefineDeviceClassProp", tfcDefineDeviceClassProp);

    register_node("viaSpecs",               viaSpecs);

    //
    // Below are V5 nodes, virtually all of these are empty stubs.
    //

    register_node("physicalRules",          class_dispatch);
    register_node("orderedSpacingRules",    orderedSpacingRules);
    register_node("spacingRules",           spacingRules);

    register_node("electricalRules",        class_dispatch);
    register_node("characterizationRules",  characterizationRules);
    register_node("orderedCharacterizationRules", orderedCharacterizationRules);

    register_node("leRules",                class_dispatch);
    register_node("leLswLayers",            leLswLayers);

    register_node("lxRules",                class_dispatch);
    register_node("lxExtractLayers",        lxExtractLayers);
    register_node("lxNoOverlapLayers",      lxNoOverlapLayers);
    register_node("lxMPPTemplates",         lxMPPTemplates);

    // deprecated, mapped to lxRules
    register_node("dleRules",               class_dispatch);
    register_node("dleExtractLayers",       lxExtractLayers);
    register_node("dleNoOverlapLayers",     lxNoOverlapLayers);

    register_node("compactorRules",         class_dispatch);
    register_node("compactorLayers",        compactorLayers);
    register_node("symWires",               symWires);
    register_node("symRules",               symRules);

    register_node("lasRules",               class_dispatch);
    register_node("lasLayers",              lasLayers);
    register_node("lasDevices",             lasDevices);
    register_node("lasWires",               lasWires);
    register_node("lasProperties",          lasProperties);

    register_node("prRules",                class_dispatch);
    register_node("prRoutingLayers",        prRoutingLayers);
    register_node("prViaTypes",             prViaTypes);
    register_node("prStackVias",            prStackVias);
    register_node("prMastersliceLayers",    prMastersliceLayers);
    register_node("prViaRules",             prViaRules);
    register_node("prGenViaRules",          prGenViaRules);
    register_node("prTurnViaRules",         prTurnViaRules);
    register_node("prNonDefaultRules",      prNonDefaultRules);
    register_node("prRoutingPitch",         prRoutingPitch);
    register_node("prRoutingOffset",        prRoutingOffset);
    register_node("prOverlapLayer",         prOverlapLayer);
}


cTechCdsIn::~cTechCdsIn()
{
    instancePtr = 0;
}


// Main entry to read a Virtuoso ascii technology file.
//
bool
cTechCdsIn::read(const char *fname, char **err)
{
    static bool did_setup;
    if (!did_setup) {
        setupCdsReserved();
        did_setup = true;
    }
    bool ok = readEvalLisp(fname, CDvdb()->getVariable(VA_LibPath), true, err);
    if (ok) {
        // Give the wire:drawing LPP the WireActive flag in electrical
        // mode, so it will be treated as a wiring layer (like the
        // SCED layer) as it is in Virtuoso.
        //
        CDl *ld = CDldb()->findLayer("wire");
        if (ld)
            ld->setWireActive(true);

        // This is the instance:drawing layer, this behavior seems to
        // be built into Virtuoso.  What other interesting properties
        // lurk?  I sure can't find any documentation.
        //
        ld = CDldb()->findLayer(236, CDL_PRP_DRAWING_NUM);
        if (ld)
            ld->setNoInstView(true);
    }
    return (ok);
}


// Static function.
// Parse a Virtuoso layermap file, and set the GDSII layer mapping
// in the corresponding Xic layers.
//
bool
cTechCdsIn::readLayerMap(const char *fname)
{
    FILE *fp = fopen(fname, "r");
    if (!fp) {
        Errs()->sys_error("fopen");
        return (false);
    }

    char *s, buf[256];
    while ((s = fgets(buf, 256, fp)) != 0) {
        while (isspace(*s))
            s++;
        if (!*s || *s == '#')
            continue;

        char *lname = lstring::getqtok(&s);
        char *pname = lstring::getqtok(&s);
        int gdslnum, gdsdtype;
        if (sscanf(s, "%d %d", &gdslnum, &gdsdtype) == 2) {
            unsigned int lnum = CDldb()->getOAlayerNum(lname);
            if (lnum != CDL_NO_LAYER) {
                bool unknown;
                unsigned int pnum = CDldb()->getOApurposeNum(pname, &unknown);
                if (pnum != CDL_PRP_DRAWING_NUM || !unknown) {
                    CDl *ld = CDldb()->findLayer(lnum, pnum);
                    if (ld) {
                        ld->setStrmIn(gdslnum, gdsdtype);
                        ld->addStrmOut(gdslnum, gdsdtype);
                    }
                }
            }
        }
        delete [] lname;
        delete [] pname;
    }
    fclose(fp);
    return (true);
}


// Static function.
// Set up the Virtuoso system reserved layers and purposes.  This came
// from very old (4.4.6) docs.  I haven't found similar info in the
// 6.1.x docs.
//
void
cTechCdsIn::setupCdsReserved()
{
    // reserved layers
    CDldb()->saveOAlayer("Unrouted",            200, 0);
    CDldb()->saveOAlayer("Row",                 201, 0);
    CDldb()->saveOAlayer("Group",               202, 0);
    CDldb()->saveOAlayer("Cannotoccupy",        203, 0);
    CDldb()->saveOAlayer("Canplace",            204, 0);
    CDldb()->saveOAlayer("hardFence",           205, 0);
    CDldb()->saveOAlayer("softFence",           206, 0);
    CDldb()->saveOAlayer("y0",                  207, 0);
    CDldb()->saveOAlayer("y1",                  208, 0);
    CDldb()->saveOAlayer("y2",                  209, 0);
    CDldb()->saveOAlayer("y3",                  210, 0);
    CDldb()->saveOAlayer("y4",                  211, 0);
    CDldb()->saveOAlayer("y5",                  212, 0);
    CDldb()->saveOAlayer("y6",                  213, 0);
    CDldb()->saveOAlayer("y7",                  214, 0);
    CDldb()->saveOAlayer("y8",                  215, 0);
    CDldb()->saveOAlayer("y9",                  216, 0);
    CDldb()->saveOAlayer("designFlow",          217, 0);
    CDldb()->saveOAlayer("stretch",             218, 0);
    CDldb()->saveOAlayer("edgeLayer",           219, 0);
    CDldb()->saveOAlayer("changedLayer",        220, 0);
    CDldb()->saveOAlayer("unset",               221, 0);
    CDldb()->saveOAlayer("unknown",             222, 0);
    CDldb()->saveOAlayer("spike",               223, 0);
    CDldb()->saveOAlayer("hiz",                 224, 0);
    CDldb()->saveOAlayer("resist",              225, 0);
    CDldb()->saveOAlayer("drive",               226, 0);
    CDldb()->saveOAlayer("supply",              227, 0);
    CDldb()->saveOAlayer("wire",                228, 0);
    CDldb()->saveOAlayer("pin",                 229, 0);
    CDldb()->saveOAlayer("text",                230, 0);
    CDldb()->saveOAlayer("device",              231, 0);
    CDldb()->saveOAlayer("border",              232, 0);
    CDldb()->saveOAlayer("snap",                233, 0);
    CDldb()->saveOAlayer("align",               234, 0);
    CDldb()->saveOAlayer("prBoundary",          235, 0);
    CDldb()->saveOAlayer("instance",            236, 0);
    CDldb()->saveOAlayer("annotate",            237, 0);
    CDldb()->saveOAlayer("marker",              238, 0);
    CDldb()->saveOAlayer("select",              239, 0);
    CDldb()->saveOAlayer("winActiveBanner",     240, 0);
    CDldb()->saveOAlayer("winAttentionText",    241, 0);
    CDldb()->saveOAlayer("winBackground",       242, 0);
    CDldb()->saveOAlayer("winBorder",           243, 0);
    CDldb()->saveOAlayer("winBottomShadow",     244, 0);
    CDldb()->saveOAlayer("winButton",           245, 0);
    CDldb()->saveOAlayer("winError",            246, 0);
    CDldb()->saveOAlayer("winForeground",       247, 0);
    CDldb()->saveOAlayer("winInactiveBanner",   248, 0);
    CDldb()->saveOAlayer("winText",             249, 0);
    CDldb()->saveOAlayer("winTopShadow",        250, 0);
    CDldb()->saveOAlayer("grid",                251, 0);
    CDldb()->saveOAlayer("axis",                252, 0);
    CDldb()->saveOAlayer("hilite",              253, 0);
    CDldb()->saveOAlayer("background",          254, 0);

    // reserved purposes
    // OpenAccess reserved
    // fillOPC      −11
    // oaNo         -10
    // oaAny        -9
    // redundant    −8
    // gapFill      −7
    // annotation   −6
    // OPCAntiSerif −5
    // OPCSerif     −4
    // slot         −3
    // fill         −2
    // drawing      −1

    // New Virtuoso
    CDldb()->saveOApurpose("fatal",             223, 0);
    CDldb()->saveOApurpose("critical",          224, 0);
    CDldb()->saveOApurpose("soCritical",        225, 0);
    CDldb()->saveOApurpose("soError",           226, 0);
    CDldb()->saveOApurpose("ackWarn",           227, 0);
    CDldb()->saveOApurpose("info",              228, 0);
    CDldb()->saveOApurpose("track",             229, 0);
    CDldb()->saveOApurpose("blockage",          230, 0);
    CDldb()->saveOApurpose("grid",              231, 0);

    // Legacy Virtuoso
    CDldb()->saveOApurpose("warning",           234, 0);
    CDldb()->saveOApurpose("tool1",             235, 0);
    CDldb()->saveOApurpose("tool0",             236, 0);
    CDldb()->saveOApurpose("label",             237, 0);
    CDldb()->saveOApurpose("flight",            238, 0);
    CDldb()->saveOApurpose("error",             239, 0);
    CDldb()->saveOApurpose("annotate",          240, 0);
    CDldb()->saveOApurpose("drawing1",          241, 0);
    CDldb()->saveOApurpose("drawing2",          242, 0);
    CDldb()->saveOApurpose("drawing3",          243, 0);
    CDldb()->saveOApurpose("drawing4",          244, 0);
    CDldb()->saveOApurpose("drawing5",          245, 0);
    CDldb()->saveOApurpose("drawing6",          246, 0);
    CDldb()->saveOApurpose("drawing7",          247, 0);
    CDldb()->saveOApurpose("drawing8",          248, 0);
    CDldb()->saveOApurpose("drawing9",          249, 0);
    CDldb()->saveOApurpose("boundary",          250, 0);
    CDldb()->saveOApurpose("pin",               251, 0);
    CDldb()->saveOApurpose("drawing",           252, 0);
    CDldb()->saveOApurpose("net",               253, 0);
    CDldb()->saveOApurpose("cell",              254, 0);
    CDldb()->saveOApurpose("all",               255, 0);
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

    void
    err_no_layer(lispnode &n, const char *what)
    {
        const char *lname, *pname = 0;
        if (n.type == LN_STRING)
            lname = n.string;
        else if (n.type == LN_NODE) {
            lispnode v[2];
            char *err = 0;
            int sc = lispnode::eval_list(n.args, v, 2, &err);
            delete [] err;
            if (sc != 2 || v[0].type != LN_STRING || v[1].type != LN_STRING)
                return;
            lname = v[0].string;
            pname = v[1].string;
        }
        else
            return;
        if (!lname || !*lname)
            return;
        if (!pname || !*pname)
            pname = "drawing";
        Log()->WarningLogV(compat,
            "Unresolved layer/purpose: %s/%s, %s setting ignored.",
            lname, pname, what);
    }

    // Utility to find a layer given a "layer" node.
    //
    CDl *get_layer(lispnode &n, char **err, bool *badnode)
    {
        *badnode = false;
        const char *lname, *pname = 0;
        if (n.type == LN_STRING)
            lname = n.string;
        else if (n.type == LN_NODE) {
            lispnode v[2];
            int sc = lispnode::eval_list(n.args, v, 2, err);
            if (sc != 2 || v[0].type != LN_STRING || v[1].type != LN_STRING) {
                *badnode = true;
                return (0);
            }
            lname = v[0].string;
            pname = v[1].string;
        }
        else {
            *badnode = true;
            return (0);
        }

        unsigned int lnum = CDldb()->getOAlayerNum(lname);
        if (lnum == CDL_NO_LAYER)
            return (0);
        bool unknown;
        unsigned int pnum = CDldb()->getOApurposeNum(pname, &unknown);
        if (pnum == CDL_PRP_DRAWING_NUM && unknown)
            return (0);
        return (CDldb()->findLayer(lnum, pnum));
    }
}


//
// Below are node evaluation callbacks, all static.
//

bool
cTechCdsIn::comment(lispnode*, lispnode*, char**)
{
    // comment. no action
    return (true);
}


bool
cTechCdsIn::include_node(lispnode *p0, lispnode*, char **err)
{
    lispnode n;
    int cnt = lispnode::eval_list(p0->args, &n, 1, err);
    if (cnt == 1 && n.type == LN_STRING) {
        if (!CdsIn())
            new cTechCdsIn;
        char *erret;
        if (!CdsIn()->readEvalLisp(n.string,
                CDvdb()->getVariable(VA_LibPath), true, &erret))
            Log()->WarningLogV(compat, "Failed: %s.",
                erret ? erret : "unknown error");
        return (true);
    }
    *err = lstring::copy("include: invalid arg");
    return (false);
}


bool
cTechCdsIn::class_dispatch(lispnode *p0, lispnode *v, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        if (p->type != LN_NODE || !p->string)
            continue;
        nodefunc func = CdsIn()->find_func(p->string);
        if (!func) {
            if (CDvdb()->getVariable(VA_DrfDebug)) {
                Log()->WarningLogV(compat, "Unknown node name %s in %s.\n",
                    p->string, p0->string);
            }
            continue;
        }
        (*func)(p, v, err);
    }
    return (true);
}


//-----------------------------------------------------------------------
// controls

bool
cTechCdsIn::techParams(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[2];
        int cnt = lispnode::eval_list(p->args, n, 3, err);
        if (cnt < 2) {
            err_rpt("techParams", p);
            continue;
        }
        // n[0] paramName
        // n[1] value

        if (n[0].type != LN_STRING) {
            err_rpt("techParams", p);
            continue;
        }

        // Look for string properties with a name suffix "_layer". 
        // We take these as an alias definition for the value, where
        // the value is assumed to be a layer name.  Note that we
        // can't verify that the value is really a layer name, since
        // it probably hasn't been created yet.

        if (n[1].type == LN_QSTRING || n[1].type == LN_STRING) {
            const char *name = n[0].string;
            const char *us = strrchr(name, '_');
            if (us && !strcasecmp(us+1, "layer")) {
                const char *val = n[1].string;
                CDldb()->saveAlias(name, val);
            }
        }

        // Other properties are ignored for now.
    }
    return (true);
}


bool
cTechCdsIn::techPermissions(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCdsIn::viewTypeUnits(lispnode *p0, lispnode*, char **err)
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
cTechCdsIn::mfgGridResolution(lispnode *p0, lispnode*, char **err)
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


//-----------------------------------------------------------------------
// layerDefinitions

bool
cTechCdsIn::techLayers(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[3];
        int cnt = lispnode::eval_list(p->args, n, 3, err);
        if (cnt < 2) {
            err_rpt("techLayers", p);
            continue;
        }
        // n[0]: layer name
        // n[1]: layer number
        // n[2]: abbrev (up to 7 chars) optional
        // Cadence allows layer numbers 0 through 194, 256 - 2^32-1026.

        if (n[0].type != LN_STRING || n[1].type != LN_NUMERIC ||
                (cnt == 3 && n[2].type != LN_STRING)) {
            err_rpt("techLayers", p);
            continue;
        }
        const char *abbrev = (cnt == 3 ? n[2].string : 0);
        CDldb()->saveOAlayer(n[0].string, mmRnd(n[1].value), abbrev);
    }
    return (true);
}


bool
cTechCdsIn::techPurposes(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[3];
        int cnt = lispnode::eval_list(p->args, n, 3, err);
        if (cnt < 2) {
            err_rpt("techPurposes", p);
            continue;
        }
        // n[0]: purpose name
        // n[1]: purpose number 0-195 user, 195-255 sys
        // n[2]: abbrev (up to 7 chars) optional
        // Cadence allows purpose numbers 0 through 194, 256 - 2^32-1026.

        if (n[0].type != LN_STRING || n[1].type != LN_NUMERIC ||
                (cnt == 3 && n[2].type != LN_STRING)) {
            err_rpt("techPurposes", p);
            continue;
        }
        const char *abbrev = (cnt == 3 ? n[2].string : 0);
        CDldb()->saveOApurpose(n[0].string, mmRnd(n[1].value), abbrev);
    }
    return (true);
}


bool
cTechCdsIn::techLayerPurposePriorities(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[2];
        int cnt = lispnode::eval_list(p->args, n, 2, err);
        if (cnt < 2) {
            err_rpt("techLayerPurposePriorities", p);
            continue;
        }
        // n[0]: layer name
        // n[1]: purpose name

        if (n[0].type != LN_STRING || n[1].type != LN_STRING) {
            err_rpt("techLayerPurposePriorities", p);
            continue;
        }

        unsigned int lnum = CDldb()->getOAlayerNum(n[0].string);
        if (lnum == CDL_NO_LAYER) {
            // Layer wasn't in the layers techLayers table, add it.
            lnum = CDldb()->newLayerNum();
            CDldb()->saveOAlayer(n[0].string, lnum);
        }
        bool unknown;
        unsigned int pnum =
        CDldb()->getOApurposeNum(n[1].string, &unknown);
        if (pnum == CDL_PRP_DRAWING_NUM && unknown) {
            // Purpose is not "drawing" and is not listed in the
            // techPurposes table, add it.
            pnum = CDldb()->newPurposeNum();
            CDldb()->saveOApurpose(n[1].string, pnum);
        }

        CDl *ld = CDldb()->newLayer(lnum, pnum, Physical);
        if (!ld) {
            Log()->WarningLog(compat, Errs()->get_error());
            continue;
        }

        // For Cadence compatibility, the following layer numbers will
        // also go into the Electrical menu.
        //
        //   228  wire
        //   229  pin
        //   230  text
        //   231  device
        //   236  instance
        //   237  annotate

        if ((lnum >= 228 && lnum <= 231) || lnum == 236 || lnum == 237)
            CDldb()->newLayer(lnum, pnum, Electrical);
    }
    return (true);
}


bool
cTechCdsIn::techDisplays(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[8];
        int cnt = lispnode::eval_list(p->args, n, 8, err);
        if (cnt < 8) {
            err_rpt("techDisplays", p);
            continue;
        }
        // n[0]: layer name
        // n[1]: purpose
        // n[2]: packet
        // n[3]: visible        (t or nil)
        //       Will be shown in display device.
        // n[4]: selectable     (t or nil)
        //       Shapes can be selected.
        // n[5]: Con2ChgLy      (t or nil)
        //       Changes on layer trigger Diva layer change.
        // n[6]: DrgEnbl        (t or nil)
        //       Shapes can be dragged.
        // n[7]: valid          (t or nil)
        //       LPP is listed in the layer selection window.

        if (n[0].type != LN_STRING || n[1].type != LN_STRING ||
                n[2].type != LN_STRING) {
            err_rpt("techDisplays", p);
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

        CDl *ld = CDldb()->findLayer(lnum, pnum);
        if (!ld)
            continue;
        ld->setInvisible(n[3].is_nil());
        ld->setRstInvisible(n[3].is_nil());
        ld->setNoSelect(n[4].is_nil());
        ld->setRstNoSelect(n[4].is_nil());
        
        if (n[7].is_nil()) {
            // In Virtuoso, "Invalid" layers don't appear in the layer
            // selection window, but otherwise display and act
            // normally.
            //
            // Presently, Invalid layers in Xic simply make the
            // objects invisible, with no messages or clues.  This is
            // not good.  However, the layers will appear in the tech
            // file, so can be reinstated by hand if the user figures
            // this out.
            //
            // In the present reference PDK, all of the schematic
            // layers are now Invalid, which completely breaks
            // electrical mode.

            // This could be skipped entirely, but instead we will set
            // to Invalid layers that are also not visible and
            // selectable.

            if (ld->isInvisible() && ld->isNoSelect())
                CDldb()->setInvalid(ld, true);
        }

        if (!DrfIn()) {
            // The drf file hasn't been read yet, so save a list of the
            // layers and their packet names and flags.  We'll update when
            // the drf is read.

            cTechDrfIn::save(ld, n[2].string);
        }
        else
            DrfIn()->apply_packet(ld, n[2].string);

    }

    if (DrfIn()) {
        sLstr lstr;
        DrfIn()->report_unresolved(lstr);
        if (lstr.string() && CDvdb()->getVariable(VA_DrfDebug))
            Log()->WarningLog(compat, lstr.string());
    }

    return (true);
}


bool
cTechCdsIn::techLayerProperties(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[4];
        int cnt = lispnode::eval_list(p->args, n, 4, err);
        if (cnt < 3) {
            err_rpt("techLayerProperties", p);
            continue;
        }
        // n[0]: prop name
        // n[1]: layer1  (name or LPP)
        // n[2]: [layer2]
        // n[3]: value

        if (cnt == 4)
            continue;  // layer2 absent in properties of interest.

        if (n[0].type != LN_STRING) {
            err_rpt("techLayerProperties", p);
            continue;
        }
        const char *prpname = n[0].string;  // property name
        if (!prpname)
            continue;

        bool bad;
        CDl *ld = get_layer(n[1], err, &bad);
        if (bad) {
            err_rpt("techLayerProperties", p);
            continue;
        }
        if (!ld) {
            err_no_layer(n[1], "techLayerProperties");
            continue;
        }

        if (lstring::cieq(prpname, "sheetResistance")) {
            if (n[2].type != LN_NUMERIC) {
                err_rpt("techLayerProperties", p);
                continue;
            }
            tech_prm(ld)->set_ohms_per_sq(n[2].value);
        }
        else if (lstring::cieq(prpname, "areaCapacitance")) {
            if (n[2].type != LN_NUMERIC) {
                err_rpt("techLayerProperties", p);
                continue;
            }
            tech_prm(ld)->set_cap_per_area(n[2].value);
        }
        else if (lstring::cieq(prpname, "edgeCapacitance")) {
            if (n[2].type != LN_NUMERIC) {
                err_rpt("techLayerProperties", p);
                continue;
            }
            tech_prm(ld)->set_cap_per_perim(n[2].value);
        }
        else if (lstring::cieq(prpname, "thickness")) {
            // tsmcN65 specifies this in angstroms
            if (n[2].type != LN_NUMERIC) {
                err_rpt("techLayerProperties", p);
                continue;
            }
            dsp_prm(ld)->set_thickness(n[2].value * 1e-4);
        }
    }
    return (true);
}


bool
cTechCdsIn::techDerivedLayers(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[3];
        int cnt = lispnode::eval_list(p->args, n, 3, err);
        if (cnt < 3) {
            err_rpt("techDerivedLayers", p);
            continue;
        }
        // n[0]: assigned name
        // n[1]: assigned number
        // n[2]: node (name 'op name)

        if (n[0].type != LN_STRING) {
            err_rpt("techDerivedLayers", p);
            continue;
        }
        const char *lname = n[0].string;  // layer name
        if (!lname)
            continue;

        if (n[1].type != LN_NUMERIC) {
            err_rpt("techDerivedLayers", p);
            continue;
        }
        int lnum = mmRnd(n[1].value);

        if (n[2].type != LN_NODE) {
            err_rpt("techDerivedLayers", p);
            continue;
        }
        lispnode a[3];
        int acnt = lispnode::eval_list(n[2].args, a, 3, err);
        if (acnt < 3) {
            err_rpt("techDerivedLayers", p);
            continue;
        }

        if (a[0].type != LN_STRING) {
            err_rpt("techDerivedLayers", p);
            continue;
        }
        const char *lrf1 = a[0].string;  // ref 1 layer name
        if (!lrf1)
            continue;

        if (a[1].type != LN_STRING) {
            err_rpt("techDerivedLayers", p);
            continue;
        }
        const char *op = a[1].string;  // operator
        if (!op)
            continue;

        if (a[2].type != LN_STRING) {
            err_rpt("techDerivedLayers", p);
            continue;
        }
        const char *lrf2 = a[2].string;  // ref 2 layer name
        if (!lrf2)
            continue;

        // Create a layer expression
        sLstr lstr;
        lstr.add(lrf1);
        if (lstring::cieq(op, "'and"))
            lstr.add_c('&');
        else if (lstring::cieq(op, "'or"))
            lstr.add_c('|');
        else if (lstring::cieq(op, "'not"))
            lstr.add_c('-');
        else if (lstring::cieq(op, "'xor"))
            lstr.add_c('^');
        /*
        elseif (lstring::cieq(op, "'butting"))
        elseif (lstring::cieq(op, "'buttOnly"))
        elseif (lstring::cieq(op, "'coincident"))
        elseif (lstring::cieq(op, "'coincidentOnly"))
        elseif (lstring::cieq(op, "'buttingOrCoincident"))
        elseif (lstring::cieq(op, "'overlapping"))
        elseif (lstring::cieq(op, "'buttingOrOverlapping"))
        elseif (lstring::cieq(op, "'touching"))
        elseif (lstring::cieq(op, "'inside"))
        elseif (lstring::cieq(op, "'outside"))
        elseif (lstring::cieq(op, "'avoiding"))
        elseif (lstring::cieq(op, "'straddling"))
        elseif (lstring::cieq(op, "'enclosing"))
        */
        else {
            err_rpt("techDerivedLayers", p);
            continue;
        }
        lstr.add(lrf2);

        CDldb()->addDerivedLayer(lname, lnum, CLdefault, lstr.string());
    }
    return (true);
}


//-----------------------------------------------------------------------
// layerRules

bool
cTechCdsIn::functions(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCdsIn::routingDirections(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[2];
        int cnt = lispnode::eval_list(p->args, n, 2, err);
        if (cnt < 2) {
            err_rpt("routingDirections", p);
            continue;
        }
        // n[0]: layername (or node)
        // n[1]: "horizontal", "vertical", or "none"

        bool bad;
        CDl *ld = get_layer(n[0], err, &bad);
        if (bad) {
            err_rpt("routingDirections", p);
            continue;
        }
        if (!ld) {
            err_no_layer(n[0], "routingDirections");
            continue;
        }

        bool is_v = false;
        bool is_h = false;
        if (n[1].type == LN_STRING) {
            const char *dir = n[1].string;
            if (dir) {
                if (!strcmp(dir, "vertical"))
                    is_v = true;
                else if (!strcmp(dir, "horizontal"))
                    is_h = true;
            }
        }
        ld->setConductor(true);
        if (is_v) {
            tech_prm(ld)->set_route_dir(tDirVert);
            ld->setRouting(true);
        }
        else if (is_h) {
            tech_prm(ld)->set_route_dir(tDirHoriz);
            ld->setRouting(true);
        }
    }
    return (true);
}


bool
cTechCdsIn::stampLabelLayers(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCdsIn::currentDensityTables(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCdsIn::viaLayers(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[3];
        int cnt = lispnode::eval_list(p->args, n, 3, err);
        if (cnt < 3) {
            err_rpt("viaLayers", p);
            continue;
        }
        // n[0]: layer 1
        // n[1]: via layer
        // n[2]: layer 2

        bool bad;
        CDl *ld1 = get_layer(n[0], err, &bad);
        if (bad) {
            err_rpt("viaLayers", p);
            continue;
        }
        if (!ld1) {
            err_no_layer(n[0], "viaLayers");
            continue;
        }

        ld1->setConductor(true);

        CDl *ld2 = get_layer(n[2], err, &bad);
        if (bad) {
            err_rpt("viaLayers", p);
            continue;
        }
        if (!ld2) {
            err_no_layer(n[2], "viaLayers");
            continue;
        }

        ld2->setConductor(true);

        CDl *ld = get_layer(n[1], err, &bad);
        if (bad) {
            err_rpt("viaLayers", p);
            continue;
        }
        if (!ld) {
            err_no_layer(n[1], "viaLayers");
            continue;
        }

        cTech::AddVia(ld, ld1->name(), ld2->name(), 0);
    }
    return (true);
}


bool
cTechCdsIn::equivalentLayers(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCdsIn::streamLayers(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[3];
        int cnt = lispnode::eval_list(p->args, n, 3, err);
        if (cnt < 3) {
            err_rpt("streamLayers", p);
            continue;
        }
        // n[0]: layer name (or node)
        // n[1]: stream number
        // n[2]: data type

        if (n[1].type != LN_NUMERIC || n[2].type != LN_NUMERIC) {
            err_rpt("streamLayers", p);
            continue;
        }

        bool bad;
        CDl *ld = get_layer(n[0], err, &bad);
        if (bad) {
            err_rpt("streamLayers", p);
            continue;
        }
        if (!ld) {
            err_no_layer(n[0], "streamLayers");
            continue;
        }

        int gdslnum = mmRnd(n[1].value);
        int gdsdtyp = mmRnd(n[2].value);
        if (gdslnum <= 0 || gdsdtyp < 0)
            continue;

        ld->setStrmIn(gdslnum, gdsdtyp);
        ld->addStrmOut(gdslnum, gdsdtyp);
    }
    return (true);
}


//-----------------------------------------------------------------------
// viaDefs

bool
cTechCdsIn::standardViaDefs(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[14];
        int cnt = lispnode::eval_list(p->args, n, 14, err);
        if (cnt < 10) {
            err_rpt("standardViaDefs", p);
            continue;
        }
        // n[0]: viaDefName
        // n[1]: layer1
        // n[2]: layer2
        // n[3]: (cutLayer cutWidth cutHeight [resistancePerCut])
        // n[4]: (cutRows   cutCol  (cutSpace))
        // n[5]: (layer1Enc)
        // n[6]: (layer2Enc)
        // n[7]: (layer1Offset)
        // n[8]: (layer2Offset)
        // n[9]: (origOffset)
        // ...   [implant1 (implant1Enc)  [implant2 (implant2Enc)
        //       [well/substrate]]]

        const char *viaDefName = n[0].string;

        bool bad;
        CDl *ld1 = get_layer(n[1], err, &bad);
        if (bad) {
            err_rpt("standardViaDefs", p);
            continue;
        }
        if (!ld1) {
            err_no_layer(n[1], "standardViaDefs");
            continue;
        }

        ld1->setConductor(true);

        CDl *ld2 = get_layer(n[2], err, &bad);
        if (bad) {
            err_rpt("standardViaDefs", p);
            continue;
        }
        if (!ld2) {
            err_no_layer(n[2], "standardViaDefs");
            continue;
        }

        ld2->setConductor(true);

        lispnode v[4];
        int sc = lispnode::eval_list(n[3].args, v, 4, err);
        if (sc < 3) {
            err_rpt("standardViaDefs", p);
            continue;
        }

        CDl *lv = get_layer(v[0], err, &bad);
        if (bad) {
            err_rpt("standardViaDefs", p);
            continue;
        }
        if (!lv) {
            err_no_layer(v[0], "standardViaDefs");
            continue;
        }

        sVia *via = cTech::AddVia(lv, ld1->name(), ld2->name(), 0);
        if (!via)
            continue;

        sStdVia svia(viaDefName, lv, ld1, ld2);
        if (v[1].type == LN_NUMERIC && v[2].type == LN_NUMERIC) {
            // Cut size
            svia.set_via_wid(INTERNAL_UNITS(v[1].value));
            svia.set_via_hei(INTERNAL_UNITS(v[2].value));
        }
        if (v[3].type == LN_NUMERIC) {
            // Resistance per cut
            svia.set_res_per_cut(v[3].value);
        }

        // (rows cols ( space_w space_h ) )
        sc = lispnode::eval_list(n[4].args, v, 3, err);
        if (sc != 3) {
            err_rpt("standardViaDefs", p);
            continue;
        }
        if (v[0].type == LN_NUMERIC && v[1].type == LN_NUMERIC) {
            // Rows and cols
            svia.set_via_rows(mmRnd(v[0].value));
            svia.set_via_cols(mmRnd(v[1].value));
        }
        if (v[2].type == LN_NODE) {
            lispnode vx[2];
            sc = lispnode::eval_list(v[2].args, vx, 2, err);
            if (sc != 2) {
                err_rpt("standardViaDefs", p);
                continue;
            }
            if (vx[0].type == LN_NUMERIC && vx[1].type == LN_NUMERIC) {
                // Cut spacing
                svia.set_via_spa_x(INTERNAL_UNITS(vx[0].value));
                svia.set_via_spa_y(INTERNAL_UNITS(vx[1].value));
            }
        }

        // ( l1_enc_w l1_enc_h )
        sc = lispnode::eval_list(n[5].args, v, 2, err);
        if (sc != 2) {
            err_rpt("standardViaDefs", p);
            continue;
        }
        if (v[0].type == LN_NUMERIC && v[1].type == LN_NUMERIC) {
            // Layer 1 enclosure
            svia.set_bot_enc_x(INTERNAL_UNITS(v[0].value));
            svia.set_bot_enc_y(INTERNAL_UNITS(v[1].value));
        }

        // ( l2_enc_w l2_enc_h )
        sc = lispnode::eval_list(n[6].args, v, 2, err);
        if (sc != 2) {
            err_rpt("standardViaDefs", p);
            continue;
        }
        if (v[0].type == LN_NUMERIC && v[1].type == LN_NUMERIC) {
            // Layer 2 enclosure
            svia.set_top_enc_x(INTERNAL_UNITS(v[0].value));
            svia.set_top_enc_y(INTERNAL_UNITS(v[1].value));
        }

        // ( l1_offs_w l1_offs_h )
        sc = lispnode::eval_list(n[7].args, v, 2, err);
        if (sc != 2) {
            err_rpt("standardViaDefs", p);
            continue;
        }
        if (v[0].type == LN_NUMERIC && v[1].type == LN_NUMERIC) {
            // Layer 1 offset
            svia.set_bot_off_x(INTERNAL_UNITS(v[0].value));
            svia.set_bot_off_y(INTERNAL_UNITS(v[1].value));
        }

        // ( l2_offs_w l2_offs_h )
        sc = lispnode::eval_list(n[8].args, v, 2, err);
        if (sc != 2) {
            err_rpt("standardViaDefs", p);
            continue;
        }
        if (v[0].type == LN_NUMERIC && v[1].type == LN_NUMERIC) {
            // Layer 2 offset
            svia.set_top_off_x(INTERNAL_UNITS(v[0].value));
            svia.set_top_off_y(INTERNAL_UNITS(v[1].value));
        }

        // ( org_offs_w org_offs_h )
        sc = lispnode::eval_list(n[9].args, v, 2, err);
        if (sc != 2) {
            err_rpt("standardViaDefs", p);
            continue;
        }
        if (v[0].type == LN_NUMERIC && v[1].type == LN_NUMERIC) {
            // Origin offset
            svia.set_org_off_x(INTERNAL_UNITS(v[0].value));
            svia.set_org_off_y(INTERNAL_UNITS(v[1].value));
        }

        // [implant1 (implant1Enc)  [implant2 (implant2Enc)
        //       [well/substrate]]]

        char *terr = 0;
        bool tbad = false;
        CDl *lim1 = get_layer(n[10], &terr, &tbad);
        delete [] terr;
        if (lim1) {
            // Implant 1
            svia.set_implant1(lim1);
            if (n[11].type == LN_NODE) {
                sc = lispnode::eval_list(n[11].args, v, 2, err);
                if (sc != 2) {
                    err_rpt("standardViaDefs", p);
                    continue;
                }
                if (v[0].type == LN_NUMERIC && v[1].type == LN_NUMERIC) {
                    // Implant 1 enclosure
                    svia.set_imp1_enc_x(INTERNAL_UNITS(v[0].value));
                    svia.set_imp1_enc_y(INTERNAL_UNITS(v[1].value));
                }
                CDl *lim2 = get_layer(n[12], &terr, &tbad);
                delete [] terr;
                if (lim2) {
                    // Implant 2
                    svia.set_implant2(lim2);
                    if (n[13].type == LN_NODE) {
                        sc = lispnode::eval_list(n[13].args, v, 2, err);
                        if (sc != 2) {
                            err_rpt("standardViaDefs", p);
                            continue;
                        }
                        if (v[0].type == LN_NUMERIC &&
                                v[1].type == LN_NUMERIC) {
                            // Implant 2 enclosure
                            svia.set_imp2_enc_x(INTERNAL_UNITS(v[0].value));
                            svia.set_imp2_enc_y(INTERNAL_UNITS(v[1].value));
                        }
                    }
                }
            }
        }
        Tech()->AddStdVia(svia);
    }

    return (true);
}


bool
cTechCdsIn::customViaDefs(lispnode*, lispnode*, char**)
{
    return (true);
}


//-----------------------------------------------------------------------
// constraintGroups

bool
cTechCdsIn::constraintGroups(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[2];
        int cnt = lispnode::eval_list(p->args, n, 2, err);
        if (cnt < 2) {
            err_rpt("constraintGroups", p);
            continue;
        }
        if (n[0].type == LN_STRING) {
            if (!strcmp(n[0].string, "foundry")) {
                if (!foundry(p, 0, err))
                    return (false);
            }
            else if (!strcmp(n[0].string, "LEFDefaultRouteSpec")) {
                if (!LEFDefaultRouteSpec(p, 0, err))
                    return (false);
            }
        }
    }
    return (true);
}


bool
cTechCdsIn::foundry(lispnode *p0, lispnode *v, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        if (p->type != LN_NODE || !p->string)
            continue;
        nodefunc func = CdsIn()->find_func(p->string);
        if (!func) {
            if (CDvdb()->getVariable(VA_DrfDebug)) {
                Log()->WarningLogV(compat, "Unknown node name %s in %s.\n",
                    p->string, p0->args->string);
            }
            continue;
        }
        (*func)(p, v, err);
    }
    return (true);
}


bool
cTechCdsIn::spacings(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[6];
        int cnt = lispnode::eval_list(p->args, n, 6, err);
        if (cnt < 3) {
            err_rpt("spacings", p);
            continue;
        }
        if (n[0].type != LN_STRING || !n[0].string) {
            err_rpt("spacings", p);
            continue;
        }

        const char *rname = n[0].string;
        if (lstring::cieq(rname, "maxWidth")) {
            if (n[2].type != LN_NUMERIC) {
                err_rpt(rname, p);
                continue;
            }
            const char *cmt = 0;
            if (n[3].type == LN_STRING && !strcmp(n[3].string, "\'ref"))
                cmt = n[4].string;

            bool bad;
            CDl *ld = get_layer(n[1], err, &bad);
            if (bad) {
                err_rpt(rname, p);
                continue;
            }
            if (ld)
                DrcIf()->addMaxWidth(ld, n[2].value, cmt);
            else
                err_no_layer(n[1], rname);
        }
        else if (lstring::cieq(rname, "minWidth")) {
            if (n[2].type != LN_NUMERIC) {
                err_rpt(rname, p);
                continue;
            }
            const char *cmt = 0;
            if (n[3].type == LN_STRING && !strcmp(n[3].string, "\'ref"))
                cmt = n[4].string;

            bool bad;
            CDl *ld = get_layer(n[1], err, &bad);
            if (bad) {
                err_rpt(rname, p);
                continue;
            }
            if (ld) {
                DrcIf()->addMinWidth(ld, n[2].value, cmt);
                tech_prm(ld)->set_route_width(INTERNAL_UNITS(n[2].value));
            }
            else
                err_no_layer(n[1], rname);
        }
        else if (lstring::cieq(rname, "minDiagonalWidth")) {
            if (n[2].type != LN_NUMERIC) {
                err_rpt(rname, p);
                continue;
            }
            const char *cmt = 0;
            if (n[3].type == LN_STRING && !strcmp(n[3].string, "\'ref"))
                cmt = n[4].string;

            bool bad;
            CDl *ld = get_layer(n[1], err, &bad);
            if (bad) {
                err_rpt(rname, p);
                continue;
            }
            if (ld)
                DrcIf()->addMinDiagonalWidth(ld, n[2].value, cmt);
            else
                err_no_layer(n[1], rname);
        }
        else if (lstring::cieq(rname, "minSpacing")) {
            if (n[2].type == LN_NUMERIC) {
                const char *cmt = 0;
                if (n[3].type == LN_STRING && !strcmp(n[3].string, "\'ref"))
                    cmt = n[4].string;

                bool bad;
                CDl *ld = get_layer(n[1], err, &bad);
                if (bad) {
                    err_rpt(rname, p);
                    continue;
                }
                if (ld) {
                    DrcIf()->addMinSpacing(ld, 0, n[2].value, cmt);
                    tech_prm(ld)->set_spacing(INTERNAL_UNITS(n[2].value));
                }
                else
                    err_no_layer(n[1], rname);
            }
            else {
                if (n[3].type != LN_NUMERIC) {
                    err_rpt(rname, p);
                    continue;
                }
                const char *cmt = 0;
                if (n[4].type == LN_STRING && !strcmp(n[4].string, "\'ref"))
                    cmt = n[5].string;

                bool bad;
                CDl *ld1 = get_layer(n[1], err, &bad);
                if (bad) {
                    err_rpt(rname, p);
                    continue;
                }
                if (!ld1) {
                    err_no_layer(n[1], rname);
                    continue;
                }

                CDl *ld2 = get_layer(n[2], err, &bad);
                if (bad) {
                    err_rpt(rname, p);
                    continue;
                }
                if (!ld2) {
                    err_no_layer(n[2], rname);
                    continue;
                }

                DrcIf()->addMinSpacing(ld1, ld2, n[3].value, cmt);
            }
        }
        else if (lstring::cieq(rname, "minSameNetSpacing")) {
            if (n[2].type == LN_NUMERIC) {
                const char *cmt = 0;
                if (n[3].type == LN_STRING && !strcmp(n[3].string, "\'ref"))
                    cmt = n[4].string;

                bool bad;
                CDl *ld = get_layer(n[1], err, &bad);
                if (bad) {
                    err_rpt(rname, p);
                    continue;
                }
                if (ld)
                    DrcIf()->addMinSameNetSpacing(ld, 0, n[2].value, cmt);
                else
                    err_no_layer(n[1], rname);
            }
            else {
                if (n[3].type != LN_NUMERIC) {
                    err_rpt(rname, p);
                    continue;
                }
                const char *cmt = 0;
                if (n[4].type == LN_STRING && !strcmp(n[4].string, "\'ref"))
                    cmt = n[5].string;

                bool bad;
                CDl *ld1 = get_layer(n[1], err, &bad);
                if (bad) {
                    err_rpt(rname, p);
                    continue;
                }
                if (!ld1) {
                    err_no_layer(n[1], rname);
                    continue;
                }

                CDl *ld2 = get_layer(n[2], err, &bad);
                if (bad) {
                    err_rpt(rname, p);
                    continue;
                }
                if (!ld2) {
                    err_no_layer(n[2], rname);
                    continue;
                }

                DrcIf()->addMinSameNetSpacing(ld1, ld2, n[3].value, cmt);
            }
        }
        else if (lstring::cieq(rname, "minDiagonalSpacing")) {
            if (n[2].type == LN_NUMERIC) {
                const char *cmt = 0;
                if (n[3].type == LN_STRING && !strcmp(n[3].string, "\'ref"))
                    cmt = n[4].string;

                bool bad;
                CDl *ld = get_layer(n[1], err, &bad);
                if (bad) {
                    err_rpt(rname, p);
                    continue;
                }
                if (ld)
                    DrcIf()->addMinDiagonalSpacing(ld, 0, n[2].value, cmt);
                else
                    err_no_layer(n[1], rname);
            }
            else {
                if (n[3].type != LN_NUMERIC) {
                    err_rpt(rname, p);
                    continue;
                }
                const char *cmt = 0;
                if (n[4].type == LN_STRING && !strcmp(n[4].string, "\'ref"))
                    cmt = n[5].string;

                bool bad;
                CDl *ld1 = get_layer(n[1], err, &bad);
                if (bad) {
                    err_rpt(rname, p);
                    continue;
                }
                if (!ld1) {
                    err_no_layer(n[1], rname);
                    continue;
                }

                CDl *ld2 = get_layer(n[2], err, &bad);
                if (bad) {
                    err_rpt(rname, p);
                    continue;
                }
                if (!ld2) {
                    err_no_layer(n[2], rname);
                    continue;
                }

                DrcIf()->addMinDiagonalSpacing(ld1, ld2, n[3].value, cmt);
            }
        }
        else if (lstring::cieq(rname, "minArea")) {
            if (n[2].type != LN_NUMERIC) {
                err_rpt(rname, p);
                continue;
            }
            const char *cmt = 0;
            if (n[3].type == LN_STRING && !strcmp(n[3].string, "\'ref"))
                cmt = n[4].string;

            bool bad;
            CDl *ld = get_layer(n[1], err, &bad);
            if (bad) {
                err_rpt(rname, p);
                continue;
            }
            if (ld)
                DrcIf()->addMinArea(ld, n[2].value, cmt);
            else
                err_no_layer(n[1], rname);
        }
        else if (lstring::cieq(rname, "minHoleArea")) {
            if (n[2].type != LN_NUMERIC) {
                err_rpt(rname, p);
                continue;
            }
            const char *cmt = 0;
            if (n[3].type == LN_STRING && !strcmp(n[3].string, "\'ref"))
                cmt = n[4].string;

            bool bad;
            CDl *ld = get_layer(n[1], err, &bad);
            if (bad) {
                err_rpt(rname, p);
                continue;
            }
            if (ld)
                DrcIf()->addMinHoleArea(ld, n[2].value, cmt);
            else
                err_no_layer(n[1], rname);
        }
        else if (lstring::cieq(rname, "minHoleWidth")) {
            if (n[2].type != LN_NUMERIC) {
                err_rpt(rname, p);
                continue;
            }
            const char *cmt = 0;
            if (n[3].type == LN_STRING && !strcmp(n[3].string, "\'ref"))
                cmt = n[4].string;

            bool bad;
            CDl *ld = get_layer(n[1], err, &bad);
            if (bad) {
                err_rpt(rname, p);
                continue;
            }
            if (ld)
                DrcIf()->addMinHoleWidth(ld, n[2].value, cmt);
            else
                err_no_layer(n[1], rname);
        }
        else if (lstring::cieq(rname, "minOverlap")) {
        }
        else if (lstring::cieq(rname, "viaSpacing")) {
        }
        else if (lstring::cieq(rname, "minViaSpacing")) {
        }
        else if (lstring::cieq(rname, "minDensity")) {
        }
        else if (lstring::cieq(rname, "maxDensity")) {
        }
        else if (lstring::cieq(rname, "minInsideCornerEdgeLength")) {
        }
        else if (lstring::cieq(rname, "minProtrusionNumCut")) {
        }
        else if (lstring::cieq(rname, "minPRBoundaryInteriorHalo")) {
        }
        else if (lstring::cieq(rname, "keepPRBoundarySharedEdges")) {
        }
        else if (lstring::cieq(rname, "minPRBoundaryExtension")) {
        }
        else if (lstring::cieq(rname, "stackable")) {
        }
        else if (CDvdb()->getVariable(VA_DrfDebug)) {
            Log()->WarningLogV(compat, "Unknown node %s in spacings.\n", rname);
        }
    }

    return (true);
}


bool
cTechCdsIn::viaStackingLimits(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[3];
        int cnt = lispnode::eval_list(p->args, n, 3, err);
        if (cnt < 3) {
            err_rpt("viaStackingLimits", p);
            continue;
        }
        // e.g., ( 4 "M1" "M7")
    }
    return (true);
}


bool
cTechCdsIn::spacingTables(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[4];
        int cnt = lispnode::eval_list(p->args, n, 4, err);
        if (cnt < 4) {
            err_rpt("spacingTables", p);
            continue;
        }
        if (n[0].type != LN_STRING || !n[0].string) {
            err_rpt("spacingTables", p);
            continue;
        }
        const char *rname = n[0].string;
        if (lstring::cieq(rname, "minSpacing")) {
            lispnode *q = p->args->next;
            lispnode v[1];
            if (lispnode::eval_list(q, v, 1, err) != 1) {
                err_rpt(rname, q);
                continue;
            }

            bool bad;
            CDl *ld = get_layer(v[0], err, &bad);
            if (bad) {
                err_rpt(rname, p);
                continue;
            }
            if (!ld) {
                err_no_layer(v[0], rname);
                continue;
            }

            q = q->next;
            if (!q)
                continue;

            CDl *ld2 = 0;
            if (lispnode::eval_list(q, v, 1, err) != 1) {
                err_rpt(rname, q);
                continue;
            }
            if (v->type == LN_STRING) {
                ld2 = get_layer(v[0], err, &bad);
                if (bad) {
                    err_rpt(rname, p);
                    continue;
                }
                if (!ld2) {
                    err_no_layer(v[0], rname); 
                    continue;
                }

                q = q->next;
                if (!q)
                    continue;
            }

            if (q->type != LN_NODE) {
                err_rpt(rname, q);
                continue;
            }

            // The current record looks like
            // (( "width" nil nil ["width" | "length" nil nil] )
            //   ['inLayerDir "layer2"]['horizontal | 'vertical | 'any]
            //   [default_value] )
            // The first "width" can be "twoWidth" in which case the
            // second "length" is required.

            // Look at the first argument to see if the table is one
            // dim or two, and if two whether it is "length" or
            // "width".
            int dims = 1;
            unsigned int flags = 0;
            lispnode *a = q->args->args;
            if (a) {
                if (a->type == LN_QSTRING || a->type == LN_STRING) {
                    if (!strcmp(a->string, "twoWidth"))
                        flags |= STF_TWOWID;
                }
                a = a->next;
            }
            if (a)
                a = a->next;
            if (a)
                a = a->next;
            if (a && (a->type == LN_QSTRING || a->type == LN_STRING)) {
                if (!strcmp(a->string, "width"))
                    flags |= STF_WIDWID;
                dims = 2;
            }

            int defspa = 0;
            for (lispnode *e = q->args->next; e; e = e->next) {
                if (e->type == LN_STRING) {
                    if (!strcmp(e->string, "`inLayerDir")) {
                        e = e->next;
                        // This is not handled.
                    }
                    else if (!strcmp(e->string, "`horizontal"))
                        flags |= STF_HORIZ;
                    else if (!strcmp(e->string, "`vertical"))
                        flags |= STF_VERT;
                    else if (!strcmp(e->string, "`any"))
                        flags |= STF_HORIZ | STF_VERT;
                    else if (!strcmp(e->string, "`sameNet"))
                        flags |= STF_SAMENET;
                    else if (!strcmp(e->string, "`PGNet"))
                        flags |= STF_PGNET;
                    else if (!strcmp(e->string, "'sameMetal"))
                        flags |= STF_SAMEMTL;
                    continue;
                }
                if (e->type == LN_NUMERIC) {
                    defspa = INTERNAL_UNITS(e->value);
                    break;
                }
            }
            // OK if no default given.

            q = q->next;
            if (!q)
                continue;

            // Count the number of records, each row has two records.
            cnt = 0;
            for (lispnode *e = q->args; e; e = e->next)
                cnt++;
            cnt /= 2;
            cnt++;

            // Write the table, the first record is the default
            // spacing, etc.
            sTspaceTable *st = new sTspaceTable[cnt];
            bool xerr = false;
            st[0].entries = cnt--;
            st[0].width = dims;
            st[0].length = flags;
            st[0].dimen = defspa;

            int i = 1;
            for (lispnode *e = q->args; e; e = e->next) {
                st[i].entries = cnt--;
                if (e->type == LN_NODE) {
                    lispnode *x = e->args;
                    if (x->type == LN_NUMERIC)
                        st[i].width = INTERNAL_UNITS(x->value);
                    else {
                        err_rpt(rname, q);
                        xerr = true;
                        break;
                    }
                    x = x->next;
                    if (x && x->type == LN_NUMERIC)
                        st[i].length = INTERNAL_UNITS(x->value);
                    else
                        st[i].length = 0;
                }
                else if (e->type == LN_NUMERIC) {
                    st[i].width = INTERNAL_UNITS(e->value);
                    st[i].length = 0;
                }
                else {
                    err_rpt(rname, q);
                    xerr = true;
                    break;
                }

                e = e->next;
                if (e && e->type == LN_NUMERIC)
                    st[i].dimen = INTERNAL_UNITS(e->value);
                else {
                    err_rpt(rname, q);
                    xerr = true;
                    break;
                }
                i++;
            }
            if (!xerr) {

                // Set the IGNORE flag for any but the plain vanilla
                // tables that we can handle at the moment.

                if (st->length)
                    st->length |= STF_IGNORE;

                if (!sTspaceTable::check_sort(st)) {
                    sLstr lstr;
                    lispnode::print(q, &lstr);
                    Log()->WarningLogV(compat,
                        "Ignored bad spacing table:\n\t%s\n",
                        lstr.string());
                }
                else
                    DrcIf()->addMinSpaceTable(ld, ld2, st);
            }
            else
                delete [] st;
        }
        else if (lstring::cieq(rname, "minDensity")) {
        }
        else if (lstring::cieq(rname, "maxDensity")) {
        }
        else if (lstring::cieq(rname, "minNumCut")) {
        }
        else if (CDvdb()->getVariable(VA_DrfDebug)) {
            Log()->WarningLogV(compat,
                "Unknown node %s in spacingTables.\n", rname);
        }
    }
    return (true);
}


bool
cTechCdsIn::orderedSpacings(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[6];
        int cnt = lispnode::eval_list(p->args, n, 6, err);
        if (cnt < 4) {
            err_rpt("orderedSpacings", p);
            continue;
        }
        if (n[0].type != LN_STRING || !n[0].string) {
            err_rpt("orderedSpacings", p);
            continue;
        }

        const char *rname = n[0].string;
        if (lstring::cieq(rname, "minExtension") ||
                lstring::cieq(rname, "minOverlapDistance")) {
            if (n[3].type != LN_NUMERIC) {
                err_rpt(rname, p);
                continue;
            }
            const char *cmt = 0;
            if (n[4].type == LN_STRING && !strcmp(n[4].string, "\'ref"))
                cmt = n[5].string;

            bool bad;
            CDl *ld1 = get_layer(n[1], err, &bad);
            if (bad) {
                err_rpt(rname, p);
                continue;
            }
            if (!ld1) {
                err_no_layer(n[1], rname);
                continue;
            }

            CDl *ld2 = get_layer(n[2], err, &bad);
            if (bad) {
                err_rpt(rname, p);
                continue;
            }
            if (!ld2) {
                err_no_layer(n[2], rname);
                continue;
            }

            DrcIf()->addMinExtension(ld1, ld2, n[3].value, cmt);
        }
        else if (lstring::cieq(rname, "minEnclosure") ||
                lstring::cieq(rname, "minEnclosureDistance")) {
            if (n[3].type != LN_NUMERIC) {
                err_rpt(rname, p);
                continue;
            }
            const char *cmt = 0;
            if (n[4].type == LN_STRING && !strcmp(n[4].string, "\'ref"))
                cmt = n[5].string;

            bool bad;
            CDl *ld1 = get_layer(n[1], err, &bad);
            if (bad) {
                err_rpt(rname, p);
                continue;
            }
            if (!ld1) {
                err_no_layer(n[1], rname);
                continue;
            }

            CDl *ld2 = get_layer(n[2], err, &bad);
            if (bad) {
                err_rpt(rname, p);
                continue;
            }
            if (!ld2) {
                err_no_layer(n[2], rname);
                continue;
            }

            DrcIf()->addMinEnclosure(ld1, ld2, n[3].value, cmt);
        }
        else if (lstring::cieq(rname, "minOppExtension")) {
            if (n[3].type != LN_NODE) {
                err_rpt(rname, p);
                continue;
            }
            lispnode v[2];
            cnt = lispnode::eval_list(n[3].args, v, 2, err);
            if (cnt < 2 || v[0].type != LN_NUMERIC ||
                    v[1].type != LN_NUMERIC) {
                err_rpt(rname, &n[3]);
                continue;
            }
            const char *cmt = 0;
            if (n[4].type == LN_STRING && !strcmp(n[4].string, "\'ref"))
                cmt = n[5].string;

            bool bad;
            CDl *ld1 = get_layer(n[1], err, &bad);
            if (bad) {
                err_rpt(rname, p);
                continue;
            }
            if (!ld1) {
                err_no_layer(n[1], rname);
                continue;
            }

            CDl *ld2 = get_layer(n[2], err, &bad);
            if (bad) {
                err_rpt(rname, p);
                continue;
            }
            if (!ld2) {
                err_no_layer(n[2], rname);
                continue;
            }

            DrcIf()->addMinOppExtension(ld1, ld2, v[0].value, v[1].value,
                cmt);
        }
        else if (CDvdb()->getVariable(VA_DrfDebug)) {
            Log()->WarningLogV(compat,
                "Unknown node %s in orderedSpacings.\n", rname);
        }
    }

    return (true);
}


bool
cTechCdsIn::antennaModels(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[1];
        int cnt = lispnode::eval_list(p->args, n, 1, err);
        if (cnt < 1) {
            err_rpt("antennaModels", p);
            continue;
        }
        if (n[0].type != LN_STRING) {
            err_rpt("antennaModels", p);
            continue;
        }
        if (lstring::cieq(n[0].string, "default")) {
        }
    }
    return (true);
}


bool
cTechCdsIn::electrical(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[1];
        int cnt = lispnode::eval_list(p->args, n, 1, err);
        if (cnt < 1) {
            err_rpt("electrical", p);
            continue;
        }
        if (n[0].type != LN_STRING) {
            err_rpt("electrical", p);
            continue;
        }
        if (lstring::cieq(n[0].string, "contactRes")) {
            //  ( contactRes layer1 [layer2] value )
        }
        else if (CDvdb()->getVariable(VA_DrfDebug)) {
            Log()->WarningLogV(compat, 
                "Unknown node %s in electrical.\n", n[0].string);
        }
    }

    return (true);
}


bool
cTechCdsIn::memberConstraintGroups(lispnode*, lispnode*, char**)
{
    return (true);
}


bool
cTechCdsIn::LEFDefaultRouteSpec(lispnode *p0, lispnode *v, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        if (p->type != LN_NODE || !p->string)
            continue;
        nodefunc func = CdsIn()->find_func(p->string);
        if (!func) {
            if (CDvdb()->getVariable(VA_DrfDebug)) {
                Log()->WarningLogV(compat, "Unknown node name %s in %s.\n",
                    p->string, p0->args->string);
            }
            continue;
        }
        (*func)(p, v, err);
    }
    return (true);
}


bool
cTechCdsIn::interconnect(lispnode *p0, lispnode*, char**)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        if (p->type == LN_NODE) {
            lispnode *pp = p->args;
            if (pp->type == LN_STRING) {
                if (lstring::cieq(pp->string, "validLayers")) {
                    //  ( validLayers (L1 L2 ...) )
                    /* unused
                    pp = pp->next;
                    if (pp->type == LN_NODE) {
                        for (lispnode *q = pp->args; q; q = q->next) {
                            if (q->type == LN_STRING)
                        }
                    }
                    */
                }
                else if (lstring::cieq(pp->string, "validVias")) {
                    //  ( validVias (V1 V2 ...) )
                    /* unused
                    pp = pp->next;
                    if (pp->type == LN_NODE) {
                        for (lispnode *q = pp->args; q; q = q->next) {
                            if (q->type == LN_STRING)
                        }
                    }
                    */
                }
                else if (lstring::cieq(pp->string, "maxRoutingDistance")) {
                    //  ( maxRoutingDistance Lname distance )
                    pp = pp->next;
                    if (pp->type == LN_STRING) {
                        const char *lname = pp->string;
                        if (lname) {
                            CDl *ld = CDldb()->findLayer(lname);
                            if (ld) {
                                pp = pp->next;
                                if (pp->type == LN_NUMERIC) {
                                    tech_prm(ld)->set_route_max_dist(
                                        INTERNAL_UNITS(pp->value));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return (true);
}


bool
cTechCdsIn::routingGrids(lispnode *p0, lispnode*, char **err)
{
    for (lispnode *p = p0->args; p; p = p->next) {
        lispnode n[3];
        int cnt = lispnode::eval_list(p->args, n, 3, err);
        if (cnt < 1) {
            err_rpt("routingGrids", p);
            continue;
        }
        if (n[0].type == LN_STRING) {
            if (lstring::cieq(n[0].string, "verticalPitch")) {
                CDl *ld = CDldb()->findLayer(n[1].string);
                if (ld) {
                    tech_prm(ld)->set_route_v_pitch(
                        INTERNAL_UNITS(n[2].value));
                }
            }
            else if (lstring::cieq(n[0].string, "horizontalPitch")) {
                CDl *ld = CDldb()->findLayer(n[1].string);
                if (ld) {
                    tech_prm(ld)->set_route_h_pitch(
                        INTERNAL_UNITS(n[2].value));
                }
            }
            else if (lstring::cieq(n[0].string, "verticalOffset")) {
                CDl *ld = CDldb()->findLayer(n[1].string);
                if (ld) {
                    tech_prm(ld)->set_route_v_offset(
                        INTERNAL_UNITS(n[2].value));
                }
            }
            else if (lstring::cieq(n[0].string, "horizontalOffset")) {
                CDl *ld = CDldb()->findLayer(n[1].string);
                if (ld) {
                    tech_prm(ld)->set_route_h_offset(
                        INTERNAL_UNITS(n[2].value));
                }
            }
        }
    }
    return (true);
}


//-----------------------------------------------------------------------
// devices
//
bool cTechCdsIn::tcCreateCDSDeviceClass(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::multipartPathTemplates(lispnode*, lispnode*, char**)
    { return (true); }

bool
cTechCdsIn::extractMOS(lispnode *p0, lispnode*, char**)
{
    sMOSdev m;
    int cnt = 0;
    for (lispnode *p = p0->args; p; p = p->next) {
        if (p->type != LN_QSTRING && p->type != LN_STRING) {
            err_rpt("extractMOS", p);
            continue;
        }
        switch (cnt) {
        case 0:
            m.name = lstring::copy(p->string);
            break;
        case 1:
            m.base = lstring::copy(p->string);
            break;
        case 2:
            m.poly = lstring::copy(p->string);
            break;
        case 3:
            m.actv = lstring::copy(p->string);
            break;
        case 4:
            m.well = lstring::copy(p->string);
            break;
        default:
            break;
        }
        cnt++;
    }
    ExtIf()->addMOS(&m);
    return (true);
}

bool
cTechCdsIn::extractRES(lispnode *p0, lispnode*, char**)
{
    sRESdev r;
    int cnt = 0;
    for (lispnode *p = p0->args; p; p = p->next) {
        if (p->type != LN_QSTRING && p->type != LN_STRING) {
            err_rpt("extractRES", p);
            continue;
        }
        switch (cnt) {
        case 0:
            r.name = lstring::copy(p->string);
            break;
        case 1:
            r.base = lstring::copy(p->string);
            break;
        case 2:
            r.matl = lstring::copy(p->string);
            break;
        default:
            break;
        }
        cnt++;
    }
    ExtIf()->addRES(&r);
    return (true);
}

bool cTechCdsIn::symContactDevice(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::ruleContactDevice(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::symEnhancementDevice(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::symDepletionDevice(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::symPinDevice(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::symRectPinDevice(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::tcCreateDeviceClass(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::tcDeclareDevice(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::tfcDefineDeviceClassProp(lispnode*, lispnode*, char**)
    { return (true); }


//-----------------------------------------------------------------------
// viaSpecs
//
bool cTechCdsIn::viaSpecs(lispnode*, lispnode*, char**)
    { return (true); }


//
// Below are V5 nodes, mostly not supported.
//

// physicalRules
bool cTechCdsIn::orderedSpacingRules(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::spacingRules(lispnode*, lispnode*, char**)
    { return (true); }
// electricalRules
bool cTechCdsIn::characterizationRules(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::orderedCharacterizationRules(lispnode*, lispnode*, char**)
    { return (true); }
// leRules
bool cTechCdsIn::leLswLayers(lispnode*, lispnode*, char**)
    { return (true); }
// lxRules
bool cTechCdsIn::lxExtractLayers(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::lxNoOverlapLayers(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::lxMPPTemplates(lispnode*, lispnode*, char**)
    { return (true); }
// compactorRules
bool cTechCdsIn::compactorLayers(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::symWires(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::symRules(lispnode*, lispnode*, char**)
    { return (true); }
// lasRules
bool cTechCdsIn::lasLayers(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::lasDevices(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::lasWires(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::lasProperties(lispnode*, lispnode*, char**)
    { return (true); }
// prRules
bool cTechCdsIn::prRoutingLayers(lispnode *p0, lispnode*, char **err)
    { return (routingDirections(p0, 0, err)); }
bool cTechCdsIn::prViaTypes(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::prStackVias(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::prMastersliceLayers(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::prViaRules(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::prGenViaRules(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::prTurnViaRules(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::prNonDefaultRules(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::prRoutingPitch(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::prRoutingOffset(lispnode*, lispnode*, char**)
    { return (true); }
bool cTechCdsIn::prOverlapLayer(lispnode*, lispnode*, char**)
    { return (true); }

