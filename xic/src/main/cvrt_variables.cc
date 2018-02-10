
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
#include "cvrt.h"
#include "fio.h"
#include "fio_cxfact.h"
#include "cd_compare.h"
#include "dsp_inlines.h"
#include "errorlog.h"
#include "tech.h"


//-----------------------------------------------------------------------------
// Variables

namespace {
    // A smarter atoi()
    //
    inline bool
    str_to_int(int *iret, const char *s)
    {
        if (!s)
            return (false);
        char *e;
        *iret = strtol(s, &e, 0);
        return (e != s);
    }

    // A smarter atof()
    //
    inline bool
    str_to_dbl(double *dret, const char *s)
    {
        if (!s)
            return (false);
        return (sscanf(s, "%lf", dret) == 1);
    }

    void
    postset(const char*)
    {
        Cvt()->UpdatePopUps();
    }

    void
    postfilt(const char*)
    {
        Cvt()->PopUpPropertyFilter(0, MODE_UPD);
    }

    void
    postpc(const char*)
    {
        Cvt()->PopUpExport(0, MODE_UPD, 0, 0);
    }

    void
    postfl(const char*)
    {
        Cvt()->PopUpImport(0, MODE_UPD, 0, 0);
        Cvt()->PopUpExport(0, MODE_UPD, 0, 0);
        Cvt()->PopUpConvert(0, MODE_UPD, 0, 0, 0);
    }
}


//
// Symbol Path
//

namespace {
    bool
    evPath(const char *vstring, bool set)
    {
        if (set) {
            FIO()->PSetPath(vstring);
            Cvt()->PopUpFiles(0, MODE_UPD);
        }

        // This variables can't be unset.
        return (set);
    }

    bool
    evNoReadExclusive(const char*, bool set)
    {
        FIO()->SetNoReadExclusive(set);
        return (true);
    }

    bool
    evAddToBack(const char*, bool set)
    {
        FIO()->SetAddToBack(set);
        return (true);
    }
}


//
// PCells
//

namespace {
    bool
    evPCellKeepSubMasters(const char*, bool set)
    {
        FIO()->SetKeepPCellSubMasters(set);
        CDvdb()->registerPostFunc(postpc);
        return (true);
    }

    // These two functions are overridden in edit_variables.cc.

    bool
    evPCellListSubMasters(const char*, bool set)
    {
        FIO()->SetListPCellSubMasters(set);
        return (true);
    }

    bool
    evPCellShowAllWarnings(const char*, bool set)
    {
        FIO()->SetShowAllPCellWarnings(set);
        return (true);
    }
}


//
// Standard Vias
//

namespace {
    bool
    evViaKeepSubMasters(const char*, bool set)
    {
        FIO()->SetKeepViaSubMasters(set);
        CDvdb()->registerPostFunc(postpc);
        return (true);
    }

    // These two functions are overridden in edit_variables.cc.

    bool
    evViaListSubMasters(const char*, bool set)
    {
        FIO()->SetListViaSubMasters(set);
        return (true);
    }
}


//
// Property Filtering for Cell Comparison
//

namespace {
    bool
    evPrpFilt(const char*, bool)
    {
        CDvdb()->registerPostFunc(postfilt);
        return (true);
    }
}


//
// Conversion - General
//

namespace {
    bool
    evChdFailOnUnresolved(const char*, bool set)
    {
        FIO()->SetChdFailOnUnresolved(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evChdCmpThreshold(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0)
                cmp_bytefact_t::set_cmp_threshold(i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect ChdCmpThreshold: requires integer >= 0.");
                return (false);
            }
        }
        else
            cmp_bytefact_t::set_cmp_threshold(CMP_DEF_THRESHOLD);
        return (true);
    }

    bool
    evMultiMapOk(const char*, bool set)
    {
        FIO()->SetMultiLayerMapOk(set);
        return (true);
    }

    bool
    evUnknownGdsLayerBase(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 65535)
                FIO()->SetUnknownGdsLayerBase(i);
            else {
                Log()->ErrorLog(mh::Variables,
                "Incorrect UnknownGdsLayerBase: requires integer 0-65535.");
                return (false);
            }
        }
        else
            FIO()->SetUnknownGdsLayerBase(FIO_UNKNOWN_LAYER_BASE);
        return (true);
    }

    bool
    evUnknownGdsDatatype(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 65535)
                FIO()->SetUnknownGdsDatatype(i);
            else {
                Log()->ErrorLog(mh::Variables,
                "Incorrect UnknownGdsDatatype: requires integer 0-65535.");
                return (false);
            }
        }
        else
            FIO()->SetUnknownGdsDatatype(FIO_UNKNOWN_DATATYPE);
        return (true);
    }

    bool
    evNoStrictCellNames(const char*, bool set)
    {
        FIO()->SetNoStrictCellnames(set);
        return (true);
    }

    bool
    evNoFlattenStdVias(const char*, bool set)
    {
        FIO()->SetNoFlattenStdVias(set);
        CDvdb()->registerPostFunc(postfl);
        return (true);
    }

    bool
    evNoFlattenPCells(const char*, bool set)
    {
        FIO()->SetNoFlattenPCells(set);
        CDvdb()->registerPostFunc(postfl);
        return (true);
    }

    bool
    evNoFlattenLabels(const char*, bool set)
    {
        FIO()->SetNoFlattenLabels(set);
        CDvdb()->registerPostFunc(postfl);
        return (true);
    }

    bool
    evNoReadLabels(const char*, bool set)
    {
        FIO()->SetNoReadLabels(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evKeepBadArchive(const char*, bool set)
    {
        FIO()->SetKeepBadArchive(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }
}


//
// Conversion - Import and Conversion Commands
//

namespace {
    bool
    ev_update(const char*, bool)
    {
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evChdRandomGzip(const char *vstring, bool set)
    {
        if (set) {
            if (!vstring || !*vstring)
                FIO()->SetChdRandomGzip(1);
            else {
                int i;
                if (str_to_int(&i, vstring) && i >= 0 && i <= 255)
                    FIO()->SetChdRandomGzip(i);
                else {
                    Log()->ErrorLog(mh::Variables,
                        "Incorrect ChdRandomGzip: requires integer 0-255.");
                    return (false);
                }
            }
        }
        else
            FIO()->SetChdRandomGzip(0);
        return (true);
    }

    bool
    evAutoRename(const char*, bool set)
    {
        FIO()->SetAutoRename(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evNoCreateLayer(const char*, bool set)
    {
        FIO()->SetNoCreateLayer(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evNoOverwritePhys(const char*, bool set)
    {
        FIO()->SetNoOverwritePhys(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evNoOverwriteElec(const char*, bool set)
    {
        FIO()->SetNoOverwriteElec(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evNoOverwriteLibCells(const char*, bool set)
    {
        FIO()->SetNoOverwriteLibCells(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evNoCheckEmpties(const char*, bool set)
    {
        FIO()->SetNoCheckEmpties(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evMergeInput(const char*, bool set)
    {
        FIO()->SetMergeInput(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evNoPolyCheck(const char*, bool set)
    {
        CD()->SetNoPolyCheck(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evDupCheckMode(const char *vset, bool set)
    {
        if (set) {
            if (vset && (*vset == 'r' || *vset == 'R'))
                CD()->SetDupCheckMode(cCD::DupRemove);
            else if (vset && (*vset == 'w' || *vset == 'W'))
                CD()->SetDupCheckMode(cCD::DupWarn);
            else
                CD()->SetDupCheckMode(cCD::DupNoTest);
        }
        else
            CD()->SetDupCheckMode(cCD::DupWarn);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evEvalOaPCells(const char*, bool set)
    {
        FIO()->SetEvalOaPCells(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evNoEvalNativePCells(const char*, bool set)
    {
        FIO()->SetNoEvalNativePCells(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evLayerList(const char *vstring, bool set)
    {
        FIO()->SetLayerList(set ? vstring : 0);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evUseLayerList(const char *vstring, bool set)
    {
        if (set) {
            if (*vstring == 'n' || *vstring == 'N')
                FIO()->SetUseLayerList(ULLskipList);
            else
                FIO()->SetUseLayerList(ULLonlyList);
        }
        else
            FIO()->SetUseLayerList(ULLnoList);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evLayerAlias(const char *vstring, bool set)
    {
        FIO()->SetLayerAlias(set ? vstring : 0);
        XM()->PopUpLayerAliases(0, MODE_UPD);
        return (true);
    }

    bool
    evUseLayerAlias(const char*, bool set)
    {
        FIO()->SetUseLayerAlias(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evInToLower(const char*, bool set)
    {
        FIO()->SetInToLower(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evInToUpper(const char*, bool set)
    {
        FIO()->SetInToUpper(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evInUseAlias(const char *vstring, bool set)
    {
        if (set) {
            if (*vstring == 'r' || *vstring == 'R')
                FIO()->SetInUseAlias(UAread);
            else if (*vstring == 'w' || *vstring == 'W' ||
                    *vstring == 's' || *vstring == 'S')
                FIO()->SetInUseAlias(UAwrite);
            else
                FIO()->SetInUseAlias(UAupdate);
        }
        else
            FIO()->SetInUseAlias(UAnone);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evInCellNamePrefix(const char *vstring, bool set)
    {
        FIO()->SetInCellNamePrefix(set ? vstring : 0);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evInCellNameSuffix(const char *vstring, bool set)
    {
        FIO()->SetInCellNameSuffix(set ? vstring : 0);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evNoMapDatatypes(const char*, bool set)
    {
        FIO()->SetNoMapDatatypes(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evCifLayerMode(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 2)
                FIO()->CifStyle().set_lread_type((EXTlreadType)i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect CifLayerMode: requires integer 0-2.");
                return (false);
            }
        }
        else
            FIO()->CifStyle().set_lread_type(EXTlreadDef);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOasReadNoChecksum(const char*, bool set)
    {
        FIO()->SetOasReadNoChecksum(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOasPrintNoWrap(const char*, bool set)
    {
        FIO()->SetOasPrintNoWrap(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOasPrintOffset(const char*, bool set)
    {
        FIO()->SetOasPrintOffset(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }
}


//
// Conversion - Export Commands
//

namespace {
    bool
    evStripForExport(const char*, bool set)
    {
        FIO()->SetStripForExport(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evWriteAllCells(const char*, bool set)
    {
        FIO()->SetWriteAllCells(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evSkipInvisible(const char *vstring, bool set)
    {
        FIO()->SetSkipInvisiblePhys(false);
        FIO()->SetSkipInvisibleElec(false);
        if (set) {
            if (*vstring != 'p' && *vstring != 'P')
                FIO()->SetSkipInvisibleElec(true);
            if (*vstring != 'e' && *vstring != 'E')
                FIO()->SetSkipInvisiblePhys(true);
        }
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evNoCompressContext(const char*, bool set)
    {
        FIO()->SetNoCompressContext(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evRefCellAutoRename(const char*, bool set)
    {
        FIO()->SetRefCellAutoRename(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evUseCellTab(const char*, bool set)
    {
        FIO()->SetUseCellTab(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evSkipOverrideCells(const char*, bool set)
    {
        FIO()->SetSkipOverrideCells(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOut32nodes(const char*, bool set)
    {
        CD()->SetOut32nodes(set);
        return (true);
    }

    bool
    evOutToLower(const char*, bool set)
    {
        FIO()->SetOutToLower(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOutToUpper(const char*, bool set)
    {
        FIO()->SetOutToUpper(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOutUseAlias(const char *vstring, bool set)
    {
        if (set) {
            if (*vstring == 'r' || *vstring == 'R')
                FIO()->SetOutUseAlias(UAread);
            else if (*vstring == 'w' || *vstring == 'W' ||
                    *vstring == 's' || *vstring == 'S')
                FIO()->SetOutUseAlias(UAwrite);
            else
                FIO()->SetOutUseAlias(UAupdate);
        }
        else
            FIO()->SetOutUseAlias(UAnone);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOutCellNamePrefix(const char *vstring, bool set)
    {
        FIO()->SetOutCellNamePrefix(set ? vstring : 0);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOutCellNameSuffix(const char *vstring, bool set)
    {
        FIO()->SetOutCellNameSuffix(set ? vstring : 0);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evCifOutStyle(const char *vstring, bool set)
    {
        if (set) {
            Errs()->init_error();
            if (!FIO()->CifStyle().set(vstring)) {
                Log()->ErrorLogV(mh::Variables, "Incorrect CifOutStyle: %s",
                    Errs()->get_error());
                return (false);
            }
        }
        else
            FIO()->CifStyle().set_def();
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evCifOutExtensions(const char *vstring, bool set)
    {
        if (set) {
            int i1, i2;
            if (sscanf(vstring, "%d %d", &i1, &i2) != 2) {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect CitOutExtensions: requires two integers.");
                return (false);
            }
            FIO()->CifStyle().set_flags(i1 & 0xfff);
            FIO()->CifStyle().set_flags_export(i2 & 0xfff);
        }
        else {
            FIO()->CifStyle().set_flags(0xfff);
            FIO()->CifStyle().set_flags_export(0);
        }
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evCifAddBBox(const char*, bool set)
    {
        FIO()->CifStyle().set_add_obj_bb(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evGdsOutLevel(const char *vstring, bool set)
    {
        if (set) {
            int i;
            if (str_to_int(&i, vstring) && i >= 0 && i <= 2)
                FIO()->SetGdsOutLevel(i);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect GdsOutLevel: requires integer 0-2.");
                return (false);
            }
        }
        else
            FIO()->SetGdsOutLevel(0);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evGdsMunit(const char *vstring, bool set)
    {
        if (set) {
            double d;
            if (str_to_dbl(&d, vstring) && d >= 0.01 && d <= 100.0)
                FIO()->SetGdsMunit(d);
            else {
                Log()->ErrorLog(mh::Variables,
                    "Incorrect GdsMunit: range is 0.01 - 100.0.");
                return (false);
            }
        }
        else
            FIO()->SetGdsMunit(1.0);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evGdsTruncateLongStrings(const char*, bool set)
    {
        FIO()->SetGdsTruncateLongStrings(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evNoGdsMapOk(const char*, bool set)
    {
        FIO()->SetNoGdsMapOk(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOasWriteCompressed(const char *vstring, bool set)
    {
        if (set) {
            if (*vstring == 'f' || *vstring == 'F')
                FIO()->SetOasWriteCompressed(OAScompForce);
            else
                FIO()->SetOasWriteCompressed(OAScompSmart);
        }
        else
            FIO()->SetOasWriteCompressed(OAScompNone);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOasWriteNameTab(const char*, bool set)
    {
        FIO()->SetOasWriteNameTab(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOasWriteRep(const char *vstring, bool set)
    {
        FIO()->SetOasWriteRep(set ? vstring : 0);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOasWriteChecksum(const char *vstring, bool set)
    {
        if (set) {
            if (*vstring == 2 || lstring::ciprefix("ch", vstring))
                FIO()->SetOasWriteChecksum(OASchksumBsum);
            else
                FIO()->SetOasWriteChecksum(OASchksumCRC);
        }
        else
            FIO()->SetOasWriteChecksum(OASchksumNone);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOasWriteNoTrapezoids(const char*, bool set)
    {
        FIO()->SetOasWriteNoTrapezoids(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOasWriteWireToBox(const char*, bool set)
    {
        FIO()->SetOasWriteWireToBox(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOasWriteRndWireToPoly(const char*, bool set)
    {
        FIO()->SetOasWriteRndWireToPoly(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOasWriteNoGCDcheck(const char*, bool set)
    {
        FIO()->SetOasWriteNoGCDcheck(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOasWriteUseFastSort(const char*, bool set)
    {
        FIO()->SetOasWriteUseFastSort(set);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }

    bool
    evOasWritePrptyMask(const char *vstring, bool set)
    {
        if (set) {
            int d;
            int msk = (OAS_PRPMSK_GDS_LBL | OAS_PRPMSK_XIC_LBL);
            if (!*vstring)
                FIO()->SetOasWritePrptyMask(msk);
            else if (sscanf(vstring, "%d", &d) == 1)
                FIO()->SetOasWritePrptyMask(msk & d);
            else
                FIO()->SetOasWritePrptyMask(OAS_PRPMSK_ALL);
        }
        else
            FIO()->SetOasWritePrptyMask(0);
        CDvdb()->registerPostFunc(postset);
        return (true);
    }
}


#define B 'b'
#define S 's'

namespace {
    void vsetup(const char *vname, char c, bool(*fn)(const char*, bool))
    {
        CDvdb()->registerInternal(vname,  fn);
        if (c == B)
            Tech()->RegisterBooleanAttribute(vname);
        else if (c == S)
            Tech()->RegisterStringAttribute(vname);
    }
}


void
cConvert::setupVariables()
{
    // Symbol Path
    vsetup(VA_Path,                     0,  evPath);
    vsetup(VA_NoReadExclusive,          B,  evNoReadExclusive);
    vsetup(VA_AddToBack,                B,  evAddToBack);

    // PCells
    vsetup(VA_PCellKeepSubMasters,      B,  evPCellKeepSubMasters);
    vsetup(VA_PCellListSubMasters,      B,  evPCellListSubMasters);
    vsetup(VA_PCellShowAllWarnings,     B,  evPCellShowAllWarnings);

    // Standard Vias
    vsetup(VA_ViaKeepSubMasters,        B,  evViaKeepSubMasters);
    vsetup(VA_ViaListSubMasters,        B,  evViaListSubMasters);

    // Property Filtering for Cell Comparison
    vsetup(VA_PhysPrpFltCell,           S,  evPrpFilt);
    vsetup(VA_PhysPrpFltInst,           S,  evPrpFilt);
    vsetup(VA_PhysPrpFltObj,            S,  evPrpFilt);
    vsetup(VA_ElecPrpFltCell,           S,  evPrpFilt);
    vsetup(VA_ElecPrpFltInst,           S,  evPrpFilt);
    vsetup(VA_ElecPrpFltObj,            S,  evPrpFilt);

    // Conversion - General
    vsetup(VA_ChdFailOnUnresolved,      B,  evChdFailOnUnresolved);
    vsetup(VA_ChdCmpThreshold,          S,  evChdCmpThreshold);
    vsetup(VA_MultiMapOk,               B,  evMultiMapOk);
    vsetup(VA_NoPopUpLog,               B,  0);
    vsetup(VA_UnknownGdsLayerBase,      S,  evUnknownGdsLayerBase);
    vsetup(VA_UnknownGdsDatatype,       S,  evUnknownGdsDatatype);
    vsetup(VA_NoStrictCellnames,        B,  evNoStrictCellNames);
    vsetup(VA_NoFlattenStdVias,         B,  evNoFlattenStdVias);
    vsetup(VA_NoFlattenPCells,          B,  evNoFlattenPCells);
    vsetup(VA_NoFlattenLabels,          B,  evNoFlattenLabels);
    vsetup(VA_NoReadLabels,             B,  evNoReadLabels);
    vsetup(VA_KeepBadArchive,           B,  evKeepBadArchive);

    // Conversion - Import and Conversion Commands
    vsetup(VA_ChdLoadTopOnly,           B,  ev_update);
    vsetup(VA_ChdRandomGzip,            S,  evChdRandomGzip);
    vsetup(VA_AutoRename,               B,  evAutoRename);
    vsetup(VA_NoCreateLayer,            B,  evNoCreateLayer);
    vsetup(VA_NoAskOverwrite,           B,  ev_update);
    vsetup(VA_NoOverwritePhys,          B,  evNoOverwritePhys);
    vsetup(VA_NoOverwriteElec,          B,  evNoOverwriteElec);
    vsetup(VA_NoOverwriteLibCells,      B,  evNoOverwriteLibCells);
    vsetup(VA_NoCheckEmpties,           B,  evNoCheckEmpties);
    vsetup(VA_MergeInput,               B,  evMergeInput);
    vsetup(VA_NoPolyCheck,              B,  evNoPolyCheck);
    vsetup(VA_DupCheckMode,             S,  evDupCheckMode);
    vsetup(VA_EvalOaPCells,             B,  evEvalOaPCells);
    vsetup(VA_NoEvalNativePCells,       B,  evNoEvalNativePCells);
    vsetup(VA_LayerList,                S,  evLayerList);
    vsetup(VA_UseLayerList,             S,  evUseLayerList);
    vsetup(VA_LayerAlias,               S,  evLayerAlias);
    vsetup(VA_UseLayerAlias,            B,  evUseLayerAlias);
    vsetup(VA_InToLower,                B,  evInToLower);
    vsetup(VA_InToUpper,                B,  evInToUpper);
    vsetup(VA_InUseAlias,               S,  evInUseAlias);
    vsetup(VA_InCellNamePrefix,         S,  evInCellNamePrefix);
    vsetup(VA_InCellNameSuffix,         S,  evInCellNameSuffix);
    vsetup(VA_NoMapDatatypes,           B,  evNoMapDatatypes);
    vsetup(VA_CifLayerMode,             S,  evCifLayerMode);
    vsetup(VA_OasReadNoChecksum,        B,  evOasReadNoChecksum);
    vsetup(VA_OasPrintNoWrap,           B,  evOasPrintNoWrap);
    vsetup(VA_OasPrintOffset,           B,  evOasPrintOffset);

    // Conversion - Export Commands
    vsetup(VA_StripForExport,           0,  evStripForExport);
    vsetup(VA_WriteAllCells,            B,  evWriteAllCells);
    vsetup(VA_SkipInvisible,            S,  evSkipInvisible);
    vsetup(VA_NoCompressContext,        B,  evNoCompressContext);
    vsetup(VA_RefCellAutoRename,        B,  evRefCellAutoRename);
    vsetup(VA_UseCellTab,               B,  evUseCellTab);
    vsetup(VA_SkipOverrideCells,        B,  evSkipOverrideCells);
    vsetup(VA_Out32nodes,               0,  evOut32nodes);
    vsetup(VA_OutToLower,               B,  evOutToLower);
    vsetup(VA_OutToUpper,               B,  evOutToUpper);
    vsetup(VA_OutUseAlias,              S,  evOutUseAlias);
    vsetup(VA_OutCellNamePrefix,        S,  evOutCellNamePrefix);
    vsetup(VA_OutCellNameSuffix,        S,  evOutCellNameSuffix);
    vsetup(VA_CifOutStyle,              S,  evCifOutStyle);
    vsetup(VA_CifOutExtensions,         S,  evCifOutExtensions);
    vsetup(VA_CifAddBBox,               B,  evCifAddBBox);
    vsetup(VA_GdsOutLevel,              S,  evGdsOutLevel);
    vsetup(VA_GdsMunit,                 S,  evGdsMunit);
    vsetup(VA_GdsTruncateLongStrings,   B,  evGdsTruncateLongStrings);
    vsetup(VA_NoGdsMapOk,               B,  evNoGdsMapOk);
    vsetup(VA_OasWriteCompressed,       S,  evOasWriteCompressed);
    vsetup(VA_OasWriteNameTab,          B,  evOasWriteNameTab);
    vsetup(VA_OasWriteRep,              S,  evOasWriteRep);
    vsetup(VA_OasWriteChecksum,         S,  evOasWriteChecksum);
    vsetup(VA_OasWriteNoTrapezoids,     B,  evOasWriteNoTrapezoids);
    vsetup(VA_OasWriteWireToBox,        B,  evOasWriteWireToBox);
    vsetup(VA_OasWriteRndWireToPoly,    B,  evOasWriteRndWireToPoly);
    vsetup(VA_OasWriteNoGCDcheck,       B,  evOasWriteNoGCDcheck);
    vsetup(VA_OasWriteUseFastSort,      B,  evOasWriteUseFastSort);
    vsetup(VA_OasWritePrptyMask,        S,  evOasWritePrptyMask);
}

