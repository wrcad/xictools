
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

#ifndef MAIN_VARIABLES_H
#define MAIN_VARIABLES_H


//
// Main Module Variables
//

// Setup
#define VA_DatabaseResolution   "DatabaseResolution"
#define VA_NetNamesCaseSens     "NetNamesCaseSens"
#define VA_Subscripting         "Subscripting"
#define VA_DrfDebug             "DrfDebug"

// Paths and Directories
#define VA_Path                 "Path"
#define VA_LibPath              "LibPath"
#define VA_HelpPath             "HelpPath"
#define VA_ScriptPath           "ScriptPath"
#define VA_NoReadExclusive      "NoReadExclusive"
#define VA_AddToBack            "AddToBack"
#define VA_DocsDir              "DocsDir"
#define VA_ProgramRoot          "ProgramRoot"
#define VA_TeePrompt            "TeePrompt"

// General Visual
#define VA_MouseWheel           "MouseWheel"
#define VA_NoCheckUpdate        "NoCheckUpdate"
#define VA_ListPageEntries      "ListPageEntries"
#define VA_NoLocalImage         "NoLocalImage"
#define VA_NoPixmapStore        "NoPixmapStore"
#define VA_NoDisplayCache       "NoDisplayCache"
#define VA_LowerWinOffset       "LowerWinOffset"
#define VA_PhysGridOrigin       "PhysGridOrigin"
#define VA_ScreenCoords         "ScreenCoords"
#define VA_PixelDelta           "PixelDelta"
#define VA_NoPhysRedraw         "NoPhysRedraw"
#define VA_NoToTop              "NoToTop"

// `!' Commands
#define VA_InstallCmdFormat     "InstallCmdFormat"
#define VA_Shell                "Shell"

// Parameterized Cells (see also edit_variables.h)
// #define VA_PCellAbutMode            "PCellAbutMode"
// #define VA_PCellHideGrips           "PCellHideGrips"
// #define VA_PCellGripInstSize        "PCellGripInstSize"
#define VA_PCellKeepSubMasters      "PCellKeepSubMasters"
#define VA_PCellListSubMasters      "PCellListSubMasters"
#define VA_PCellScriptPath          "PCellScriptPath"
#define VA_PCellShowAllWarnings     "PCellShowAllWarnings"

// Standard Vias
#define VA_ViaKeepSubMasters        "ViaKeepSubMasters"
#define VA_ViaListSubMasters        "ViaListSubMasters"

// Scripts (see si_parsenode.h)
// #define VA_LogIsLog10           "LogIsLog10"

// Selections
#define VA_MarkInstanceOrigin   "MarkInstanceOrigin"
#define VA_MarkObjectCentroid   "MarkObjectCentroid"
#define VA_SelectTime           "SelectTime"
#define VA_NoAltSelection       "NoAltSelection"
#define VA_MaxBlinkingObjects   "MaxBlinkingObjects"

// File Menu - Printing
#define VA_NoAskFileAction      "NoAskFileAction"
#define VA_DefaultPrintCmd      "DefaultPrintCmd"
#define VA_NoDriverLabels       "NoDriverLabels"
#define VA_PSlineWidth          "PSlineWidth"
#define VA_RmTmpFileMinutes     "RmTmpFileMinutes"

// Cell Menu
#define VA_ContextDarkPcnt      "ContextDarkPcnt"

// Editing General (see edit_variables.h)
#define VA_AskSaveNative        "AskSaveNative"
// #define VA_Constrain45          "Constrain45"
// #define VA_NoMergeObjects       "NoMergeObjects"
// #define VA_NoMergePolys         "NoMergePolys"
#define VA_NoFixRot45           "NoFixRot45"

// Side Menu Commands (see edit_variables.h)
// #define VA_MasterMenuLength     "MasterMenuLength"
// #define VA_DevMenuStyle         "DevMenuStyle"
#define VA_LabelDefHeight       "LabelDefHeight"
#define VA_LabelMaxLen          "LabelMaxLen"
#define VA_LabelMaxLines        "LabelMaxLines"
#define VA_LabelHiddenMode      "LabelHiddenMode"
// #define VA_LogoEndStyle         "LogoEndStyle"
// #define VA_LogoPathWidth        "LogoPathWidth"
// #define VA_LogoAltFont          "LogoAltFont"
// #define VA_LogoPixelSize        "LogoPixelSize"
// #define VA_LogoPrettyFont       "LogoPrettyFont"
// #define VA_LogoToFile           "LogoToFile"
// #define VA_NoConstrainRound     "NoConstrainRound"
// #define VA_RoundFlashSides      "RoundFlashSides"
// #define VA_ElecRoundFlashSides  "ElecRoundFlashSides"
// #define VA_SpotSize             "SpotSize"

// View Menu (see dsp.h)
#define VA_InfoInternal         "InfoInternal"
#define VA_PeekSleepMsec        "PeekSleepMsec"
#define VA_LockMode             "LockMode"
#define VA_XSectNoAutoY         "XSectNoAutoY"
#define VA_XSectYScale          "XSectYScale"

// Attributes Menu (see sced.h)
#define VA_TechNoPrintPatMap    "TechNoPrintPatMap"
#define VA_TechPrintDefaults    "TechPrintDefaults"
// #define VA_BoxLineStyle         "BoxLineStyle"
#define VA_EraseBehindProps     "EraseBehindProps"
#define VA_PhysPropTextSize     "PhysPropTextSize"
#define VA_EraseBehindTerms     "EraseBehindTerms"
#define VA_TermTextSize         "TermTextSize"
#define VA_TermMarkSize         "TermMarkSize"
// #define VA_ShowDots             "ShowDots"
#define VA_FullWinCursor        "FullWinCursor"
#define VA_CellThreshold        "CellThreshold"
// #define VA_GridNoCoarseOnly     "GridNoCoarseOnly"
// #define VA_GridThreshold        "GridThreshold"

// Help System
#define VA_HelpMultiWin         "HelpMultiWin"
#define VA_HelpDefaultTopic     "HelpDefaultTopic"

#endif

