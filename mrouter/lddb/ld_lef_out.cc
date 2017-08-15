
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
 *                                                                        *
 * LEF/DEF Database and Maze Router.                                      *
 *                                                                        *
 * Stephen R. Whiteley (stevew@wrcad.com)                                 *
 * Whiteley Research Inc. (wrcad.com)                                     *
 *                                                                        *
 * Portions adapted from Qrouter by Tim Edwards,                          *
 * (www.opencircuitdesign.com) which used code by Steve Beccue.           *
 * See original headers where applicable.                                 *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "lddb_prv.h"
#include "lefwWriter.hpp"
#include "miscutil/tvals.h"
#include <errno.h>


//
// LEF/DEF Database.
//
// Dump a LEF file from the content of the database.
//


bool
cLDDB::lefWrite(const char *fname, LEF_OUT out)
{
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        emitErrMesg("Cannot open output file: %s\n", strerror(errno));
        return (LD_BAD);
    }
    long time0 = Tvals::millisec();
    lefwInit(fp);

    // Write file header.

    // VERSION 5.4 ;
    lefwVersion(5, 4);
    lefwCaseSensitive(db_lef_case_sens ? "ON" : "OFF");

    // BUSBITCHARS "[]" ;
    lefwBusBitChars("[]");

    // DIVIDERCHAR "/" ;  
    lefwDividerChar("/");

    // UNITS
    //   DATABASE MICRONS 1000 ;
    // END UNITS
    lefwStartUnits();
    lefwUnits(0, 0, 0, 0, 0, 0, 1000);
    lefwEndUnits();

    // USEMINSPACING OBS ON ;
    // USEMINSPACING PIN OFF ;
    lefwUseMinSpacing("OBS", "ON");
    lefwUseMinSpacing("PIN", "OFF");

    // CLEARANCEMEASURE EUCLIDEAN ;
    lefwClearanceMeasure("EUCLIDEAN");
  
    lefwNewLine();

    // MANUFACTURINGGRID 0.1 ;
    lefwManufacturingGrid(lefToMic(db_mfg_grid));
    lefwNewLine();

    if (out == LEF_OUT_ALL || out == LEF_OUT_TECH) {
        // LAYER, VIA, VIARULE.
        for (u_int i = 0; i < db_lef_objcnt; i++) {
            switch (db_lef_objects[i]->lefClass) {
            case CLASS_ROUTE:
            case CLASS_CUT:
            case CLASS_MASTER:
            case CLASS_OVERLAP:
                {
                    lefObject *lo = db_lef_objects[i];
                    lefWriteLayer(lo);
                }
                break;
            case CLASS_VIA:
                {
                    lefViaObject *vo = (lefViaObject*)db_lef_objects[i];
                    lefWriteVia(vo);
                }
                break;
            case CLASS_VIARULE:
                {
                    lefViaRuleObject *vr = (lefViaRuleObject*)db_lef_objects[i];
                    lefWriteViaRule(vr);
                }
                break;
            case CLASS_IGNORE:
                break;
            }
        }
    }

    if (out == LEF_OUT_ALL || out == LEF_OUT_MACROS) {
        // SITE

        // MACRO
        for (u_int i = 0; i < db_lef_gatecnt; i++)
            lefWriteMacro(db_lef_gates[i]);
    }

    lefwEnd();
    fclose(fp);
    if (db_verbose > 0) {
        long elapsed = Tvals::millisec() - time0;
        emitMesg("LEF write: Processed %d lines in %ld milliseconds.\n",
            lefwCurrentLineNumber, elapsed);
    }
    return (LD_OK);
}


namespace {
    const char *dir_name(int d)
    {
        return ((d == DIR_VERT) ? "VERTICAL" : "HORIZONTAL");
    }
}


bool
cLDDB::lefWriteLayer(lefObject *lo)
{
    if (!lo)
        return (LD_BAD);
    if (lo->lefClass == CLASS_CUT) {
        lefCutLayer *cl = (lefCutLayer*)lo;
        lefwStartLayer(cl->lefName, "CUT");
        lefwLayerCutSpacing(lefToMic(cl->spacing));
        lefwLayerCutSpacingEnd();
        lefwEndLayer(lo->lefName);
    }
    else if (lo->lefClass == CLASS_IMPLANT) {
        lefImplLayer *il = (lefImplLayer*)lo;
        lefwStartLayer(il->lefName, "IMPLANT");
        lefwLayerWidth(lefToMic(il->width));
        lefwEndLayer(lo->lefName);
    }
    else if (lo->lefClass == CLASS_MASTER) {
        lefwStartLayer(lo->lefName, "MASTERSLICE");
        lefwEndLayer(lo->lefName);
    }
    else if (lo->lefClass == CLASS_OVERLAP) {
        lefwStartLayer(lo->lefName, "OVERLAP");
        lefwEndLayer(lo->lefName);
    }
    else if (lo->lefClass == CLASS_ROUTE) {
        lefRouteLayer *rl = (lefRouteLayer*)lo;
        lefwStartLayerRouting(rl->lefName);
        lefwLayerRouting(dir_name(rl->route.direction),
            lefToMic(rl->route.width));
        if (rl->route.offsetX == rl->route.offsetY &&
                rl->route.pitchX == rl->route.pitchY) {
            lefwLayerRoutingOffset(lefToMic(rl->route.offsetX));
            lefwLayerRoutingPitch(lefToMic(rl->route.pitchX));
        }
        else {
            lefwLayerRoutingOffsetXYDistance(lefToMic(rl->route.offsetX),
                lefToMic(rl->route.offsetY));
            lefwLayerRoutingPitchXYDistance(lefToMic(rl->route.pitchX),
                lefToMic(rl->route.pitchY));
        }
            
        if (rl->route.spacing)
            lefwLayerRoutingSpacing(lefToMic(rl->route.spacing->spacing));

        /**** May want to add this sometime.
        // RESISTANCE    RPERSQ 0.07 ;
        lefwLayerRoutingResistance(const char* res);
        // CAPACITANCE   CPERSQDIST 1.7e-05 ;
        lefwLayerRoutingCapacitance(const char* cap);
        ****/

        lefwEndLayerRouting(rl->lefName);
    }
    else {
        return (LD_BAD);
    }

    return (LD_OK);
}


bool
cLDDB::lefWriteVia(lefViaObject *vo)
{
    if (vo->lefClass != CLASS_VIA)
        return (LD_BAD);
    const char *def = 0;
    if (vo->via.generate)
        def = "GENERATED";
    else if (vo->via.deflt)
        def = "DEFAULT";
    lefwStartVia(vo->lefName, def);

    // For some reason via rectangles are units of 1/2 lambda.

    if (vo->via.bot.layer >= 0 && vo->via.top.layer >= 0) {
        lefwViaLayer(db_lef_objects[vo->via.bot.lefId]->lefName);
        lefwViaLayerRect(
            lefToMic(vo->via.bot.x1/2), lefToMic(vo->via.bot.y1/2),
            lefToMic(vo->via.bot.x2/2), lefToMic(vo->via.bot.y2/2));
        lefwViaLayer(db_lef_objects[vo->via.cut.lefId]->lefName);
        lefwViaLayerRect(
            lefToMic(vo->via.cut.x1/2), lefToMic(vo->via.cut.y1/2),
            lefToMic(vo->via.cut.x2/2), lefToMic(vo->via.cut.y2/2));
        lefwViaLayer(db_lef_objects[vo->via.top.lefId]->lefName);
        lefwViaLayerRect(
            lefToMic(vo->via.top.x1/2), lefToMic(vo->via.top.y1/2),
            lefToMic(vo->via.top.x2/2), lefToMic(vo->via.top.y2/2));
    }
    else {
        lefwViaLayer(db_lef_objects[vo->via.cut.lefId]->lefName);
        lefwViaLayerRect(
            lefToMic(vo->via.cut.x1/2), lefToMic(vo->via.cut.y1/2),
            lefToMic(vo->via.cut.x2/2), lefToMic(vo->via.cut.y2/2));
    }
    lefwEndVia(vo->lefName);
    return (LD_OK);
}


bool
cLDDB::lefWriteViaRule(lefViaRuleObject *vo)
{
    if (vo->lefClass != CLASS_VIARULE)
        return (LD_BAD);
    if (vo->via.generate) {
        lefwStartViaRuleGen(vo->lefName);
        if (vo->via.deflt)
            lefwViaRuleGenDefault();
        if (vo->met1) {
            dbVRLyr *vl = vo->met1;
            if (vl->lefId >= 0) {
                lefwViaRuleGenLayer(db_lef_objects[vl->lefId]->lefName,
                    dir_name(vl->direction),
                    lefToMic(vl->minWidth), lefToMic(vl->maxWidth),
                    lefToMic(vl->overhang), lefToMic(vl->moverhang));
            }
        }
        if (vo->met2) {
            dbVRLyr *vl = vo->met2;
            if (vl->lefId >= 0) {
                lefwViaRuleGenLayer(db_lef_objects[vl->lefId]->lefName,
                    dir_name(vl->direction),
                    lefToMic(vl->minWidth), lefToMic(vl->maxWidth),
                    lefToMic(vl->overhang), lefToMic(vl->moverhang));
            }
        }
        if (vo->via.lefId >= 0) {
            lefwViaRuleGenLayer3(db_lef_objects[vo->via.lefId]->lefName,
                lefToMic(vo->via.cut.x1), lefToMic(vo->via.cut.y1),
                lefToMic(vo->via.cut.x2), lefToMic(vo->via.cut.y2),
                lefToMic(vo->spacingX), lefToMic(vo->spacingY), 0);
        }
        lefwEndViaRuleGen(vo->lefName);
    }
    else {
        lefwStartViaRule(vo->lefName);
        if (vo->via.deflt)
            lefwViaRuleGenDefault();
        if (vo->met1) {
            dbVRLyr *vl = vo->met1;
            if (vl->lefId >= 0) {
                lefwViaRuleLayer(db_lef_objects[vo->lefId]->lefName,
                    dir_name(vl->direction),
                    lefToMic(vl->minWidth), lefToMic(vl->maxWidth),
                    lefToMic(vl->overhang), lefToMic(vl->moverhang));
            }
        }
        if (vo->met2) {
            dbVRLyr *vl = vo->met2;
            if (vl->lefId >= 0) {
                lefwViaRuleLayer(db_lef_objects[vo->lefId]->lefName,
                    dir_name(vl->direction),
                    lefToMic(vl->minWidth), lefToMic(vl->maxWidth),
                    lefToMic(vl->overhang), lefToMic(vl->moverhang));
            }
        }
        if (vo->via.lefId >= 0)
            lefwViaRuleVia(db_lef_objects[vo->via.lefId]->lefName);

        lefwEndViaRule(vo->lefName);
    }
    return (LD_OK);
}


bool
cLDDB::lefWriteMacro(lefMacro *gate)
{
    if (!gate)
        return (LD_BAD);

    // If the INTERNAL flag is set, this was created for internal use
    // only, don't write.
    if (gate->mclass == MACRO_CLASS_INTERNAL)
        return (LD_OK);

    lefwStartMacro(gate->gatename);

    const char *val1 = 0;
    switch (gate->mclass) {
    case MACRO_CLASS_COVER:
        val1 = "COVER";
        break;
    case MACRO_CLASS_BLOCK:
        val1 = "BLOCK";
        break;
    case MACRO_CLASS_PAD:
        val1 = "PAD";
        break;
    case MACRO_CLASS_CORE:
        val1 = "CORE";
        break;
    case MACRO_CLASS_ENDCAP:
        val1 = "ENDCAP";
        break;
    }
    if (val1) {
        const char *val2 = 0;
        switch (gate->subclass) {
        // COVER
        case MACRO_CLASS_BUMP:
            if (gate->mclass == MACRO_CLASS_COVER)
                val2 = "BUMP";
            break;
        // BLOCK
        case MACRO_CLASS_BLACKBOX:
            if (gate->mclass == MACRO_CLASS_BLOCK)
                val2 = "BLACKBOX";
            break;
        case MACRO_CLASS_SOFT:
            if (gate->mclass == MACRO_CLASS_BLOCK)
                val2 = "SOFT";
            break;
        //PAD
        case MACRO_CLASS_INPUT:
            if (gate->mclass == MACRO_CLASS_PAD)
                val2 = "INPUT";
            break;
        case MACRO_CLASS_OUTPUT:
            if (gate->mclass == MACRO_CLASS_PAD)
                val2 = "OUTPUT";
            break;
        case MACRO_CLASS_INOUT:
            if (gate->mclass == MACRO_CLASS_PAD)
                val2 = "INOUT";
            break;
        case MACRO_CLASS_POWER:
            if (gate->mclass == MACRO_CLASS_PAD)
                val2 = "POWER";
            break;
        case MACRO_CLASS_SPACER:
            if (gate->mclass == MACRO_CLASS_PAD ||
                    gate->mclass == MACRO_CLASS_CORE)
                val2 = "SPACER";
            break;
        case MACRO_CLASS_AREAIO:
            if (gate->mclass == MACRO_CLASS_PAD)
                val2 = "AREAIO";
            break;
        // CORE
        case MACRO_CLASS_FEEDTHRU:
            if (gate->mclass == MACRO_CLASS_CORE)
                val2 = "FEEDTHRU";
            break;
        case MACRO_CLASS_TIEHIGH:
            if (gate->mclass == MACRO_CLASS_CORE)
                val2 = "TIEHIGH";
            break;
        case MACRO_CLASS_TIELOW:
            if (gate->mclass == MACRO_CLASS_CORE)
                val2 = "TIELOW";
            break;
        case MACRO_CLASS_ANTENNACELL:
            if (gate->mclass == MACRO_CLASS_CORE)
                val2 = "ANTENNACELL";
            break;
        case MACRO_CLASS_WELLTAP:
            if (gate->mclass == MACRO_CLASS_CORE)
                val2 = "WELLTAP";
            break;
        // ENDCAP
        case MACRO_CLASS_PRE:
            if (gate->mclass == MACRO_CLASS_ENDCAP)
                val2 = "PRE";
            break;
        case MACRO_CLASS_POST:
            if (gate->mclass == MACRO_CLASS_ENDCAP)
                val2 = "POST";
            break;
        case MACRO_CLASS_TOPLEFT:
            if (gate->mclass == MACRO_CLASS_ENDCAP)
                val2 = "TOPLEFT";
            break;
        case MACRO_CLASS_TOPRIGHT:
            if (gate->mclass == MACRO_CLASS_ENDCAP)
                val2 = "TOPRIGHT";
            break;
        case MACRO_CLASS_BOTTOMLEFT:
            if (gate->mclass == MACRO_CLASS_ENDCAP)
                val2 = "BOTTOMLEFT";
            break;
        case MACRO_CLASS_BOTTOMRIGHT:
            if (gate->mclass == MACRO_CLASS_ENDCAP)
                val2 = "BOTTOMRIGHT";
            break;
        }
        lefwMacroClass(val1, val2);
    }

    for (dbForeign *f = gate->foreign; f; f = f->next) {
        lefwMacroForeign(f->name, lefToMic(f->originX), lefToMic(f->originY),
        f->orient);
    }

    lefwMacroOrigin(lefToMic(gate->placedX), lefToMic(gate->placedY));
    lefwMacroSize(lefToMic(gate->width), lefToMic(gate->height));

    char buf[16];
    buf[0] = 0;
    char *s = buf;
    if (gate->symmetry & SYMMETRY_X)
        s = lstring::stpcpy(s, " X");
    if (gate->symmetry & SYMMETRY_Y)
        s = lstring::stpcpy(s, " Y");
    if (gate->symmetry & SYMMETRY_90)
        s = lstring::stpcpy(s, " 90");
    if (buf[0])
        lefwMacroSymmetry(buf+1);

    if (gate->sitename)
        lefwMacroSite(gate->sitename);

    // Pins
    for (lefPin *pin = gate->pins; pin; pin = pin->next) {
        lefwStartMacroPin(pin->name);
        if (pin->direction)
            lefwMacroPinDirection(pin_classes[(int)pin->direction]);
        if (pin->use)
            lefwMacroPinUse(pin_uses[(int)pin->use]);
        if (pin->shape)
            lefwMacroPinUse(pin_shapes[(int)pin->shape]);

        // Support multiple ports?
        const char *classType = 0;  // or "CORE"
        lefwStartMacroPinPort(classType);
        int lastid = -1;
        for (dbDseg *sg = pin->geom; sg; sg = sg->next) {
            if (sg->lefId >= 0) {
                lefObject *lo = db_lef_objects[sg->lefId];
                if (lo->lefId != lastid) {
                    lefwMacroPinPortLayer(lo->lefName, 0.0);
                    lastid = lo->lefId;
                }
                // numx numy spacex spacey
                lefwMacroPinPortLayerRect(
                    lefToMic(sg->x1), lefToMic(sg->y1),
                    lefToMic(sg->x2), lefToMic(sg->y2),
                    1, 1, 0, 0);
            }
        }
        lefwEndMacroPinPort();
        lefwEndMacroPin(pin->name);
    }

    // Obstructions.
    if (gate->obs) {
        lefwStartMacroObs();
        int lastid = -1;
        for (dbDseg *sg = gate->obs; sg; sg = sg->next) {
            if (sg->lefId >= 0) {
                lefObject *lo = db_lef_objects[sg->lefId];
                if (lo->lefId != lastid) {
                    lefwMacroObsLayer(lo->lefName, 0.0);
                    lastid = lo->lefId;
                }
                // numx numy spacex spacey
                lefwMacroObsLayerRect(
                    lefToMic(sg->x1), lefToMic(sg->y1),
                    lefToMic(sg->x2), lefToMic(sg->y2),
                    1, 1, 0, 0);
            }
        }
        lefwEndMacroObs();
    }

    lefwEndMacro(gate->gatename);
    return (LD_OK);
}

