
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

#ifndef CVRT_VARIABLES_H
#define CVRT_VARIABLES_H


//
// Convert Module Variables
//

// Convert Menu - General
#define VA_ChdFailOnUnresolved      "ChdFailOnUnresolved"
#define VA_ChdCmpThreshold          "ChdCmpThreshold"
#define VA_MultiMapOk               "MultiMapOk"
#define VA_NoPopUpLog               "NoPopUpLog"
#define VA_UnknownGdsLayerBase      "UnknowGdsLayerBase"
#define VA_UnknownGdsDatatype       "UnknownGdsDatatype"
#define VA_NoStrictCellnames        "NoStrictCellnames"

// Convert Menu - Input and ASCII Output
#define VA_ChdLoadTopOnly           "ChdLoadTopOnly"
#define VA_ChdRandomGzip            "ChdRandomGzip"
#define VA_AutoRename               "AutoRename"
#define VA_NoCreateLayer            "NoCreateLayer"
#define VA_NoAskOverwrite           "NoAskOverwrite"
#define VA_NoOverwritePhys          "NoOverwritePhys"
#define VA_NoOverwriteElec          "NoOverwriteElec"
#define VA_NoOverwriteLibCells      "NoOverwriteLibCells"
#define VA_NoCheckEmpties           "NoCheckEmpties"
#define VA_NoReadLabels             "NoReadLabels"
#define VA_MergeInput               "MergeInput"
#define VA_NoPolyCheck              "NoPolyCheck"
#define VA_DupCheckMode             "DupCheckMode"
#define VA_EvalOaPCells             "EvalOaPCells"
#define VA_NoEvalNativePCells       "NoEvalNativePCells"
#define VA_LayerList                "LayerList"
#define VA_UseLayerList             "UseLayerList"
#define VA_LayerAlias               "LayerAlias"
#define VA_UseLayerAlias            "UseLayerAlias"
#define VA_InToLower                "InToLower"
#define VA_InToUpper                "InToUpper"
#define VA_InUseAlias               "InUseAlias"
#define VA_InCellNamePrefix         "InCellNamePrefix"
#define VA_InCellNameSuffix         "InCellNameSuffix"
#define VA_NoMapDatatypes           "NoMapDatatypes"
#define VA_CifLayerMode             "CifLayerMode"
#define VA_OasReadNoChecksum        "OasReadNoChecksum"
#define VA_OasPrintNoWrap           "OasPrintNoWrap"
#define VA_OasPrintOffset           "OasPrintOffset"

// Convert Menu - Output
#define VA_StripForExport           "StripForExport"
#define VA_WriteAllCells            "WriteAllCells"
#define VA_SkipInvisible            "SkipInvisible"
#define VA_KeepBadArchive           "KeepBadArchive"
#define VA_NoCompressContext        "NoCompressContext"
#define VA_RefCellAutoRename        "RefCellAutoRename"
#define VA_UseCellTab               "UseCellTab"
#define VA_SkipOverrideCells        "SkipOverrideCells"
#define VA_Out32nodes               "Out32nodes"
#define VA_OutToLower               "OutToLower"
#define VA_OutToUpper               "OutToUpper"
#define VA_OutUseAlias              "OutUseAlias"
#define VA_OutCellNamePrefix        "OutCellNamePrefix"
#define VA_OutCellNameSuffix        "OutCellNameSuffix"
#define VA_CifOutStyle              "CifOutStyle"
#define VA_CifOutExtensions         "CifOutExtensions"
#define VA_CifAddBBox               "CifAddBBox"
#define VA_GdsOutLevel              "GdsOutLevel"
#define VA_GdsMunit                 "GdsMunit"
#define VA_GdsTruncateLongStrings   "GdsTruncateLongStrings"
#define VA_NoGdsMapOk               "NoGdsMapOk"
#define VA_OasWriteCompressed       "OasWriteCompressed"
#define VA_OasWriteNameTab          "OasWriteNameTab"
#define VA_OasWriteRep              "OasWriteRep"
#define VA_OasWriteChecksum         "OasWriteChecksum"
#define VA_OasWriteNoTrapezoids     "OasWriteNoTrapezoids"
#define VA_OasWriteWireToBox        "OasWriteWireToBox"
#define VA_OasWriteRndWireToPoly    "OasWriteRndWireToPoly"
#define VA_OasWriteNoGCDcheck       "OasWriteNoGCDcheck"
#define VA_OasWriteUseFastSort      "OasWriteUseFastSort"
#define VA_OasWritePrptyMask        "OasWritePrptyMask"

#endif

