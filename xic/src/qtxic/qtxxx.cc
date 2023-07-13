
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
#include "edit.h"
#include "sced.h"
#include "drc.h"
#include "ext.h"
#include "oa_if.h"


// qtasm.cc

// Exported function to pop up/down the tool.
//
void
cConvert::PopUpAssemble(GRobject, ShowMode)
{
}



void
cMain::PopUpLayerParamEditor(GRobject, ShowMode, const char*, const char*)
{
}


void
cMain::SetNoToTop(bool)
{
}

void
cMain::SetLowerWinOffset(int)
{
}


// qtdebug.cc

void
cMain::PopUpDebug(GRobject, ShowMode)
{
}

bool
cMain::DbgLoad(MenuEnt*)
{
    return (false);
}


// qtextcmd.cc

void
cExt::PopUpExtCmd(GRobject, ShowMode, sExtCmd*,
    bool(*)(const char*, void*, bool, const char*, int, int),
    void*, int)
{
}

void
cExt::PopUpExtSetup(GRobject, ShowMode)
{
}

void
cExt::PopUpSelections(GRobject, ShowMode)
{
}

void
cExt::PopUpDevices(GRobject, ShowMode)
{
}

void
cExt::PopUpPhysTermEdit(GRobject, ShowMode, TermEditInfo*,
    void(*)(TermEditInfo*, CDsterm*), CDsterm*, int, int)
{
}


// qtlpal.cc

void
cMain::PopUpLayerPalette(GRobject, ShowMode, bool, CDl*)
{
}


void
cEdit::PopUpStdVia(GRobject, ShowMode, CDc*)
{
}


void
cDRC::PopUpRules(GRobject, ShowMode)
{
}

void
cDRC::PopUpDrcRun(GRobject, ShowMode)
{
}

void
cDRC::PopUpRuleEdit(GRobject, ShowMode, DRCtype, const char*,
    bool(*)(const char*, void*), void*, const DRCtestDesc*)
{
}

bool
cSced::PopUpNodeMap(GRobject, ShowMode, int)
{
    return (false);
}


struct PCellParam;
bool
cEdit::PopUpPCellParams(GRobject, ShowMode, PCellParam*,
    const char*, pcpMode)
{
    return (false);
}
