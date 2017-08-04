
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
#include "layertab.h"
#include "dsp_inlines.h"


//-----------------------------------------------------------------------------
// CD configuration

namespace {
    // The layer sequence for the passed mode was just changed.
    //
    void
    ifLayerTableChange(DisplayMode mode)
    {
        // Repaint layer table for mode if visible.
        if (DSP()->MainWdesc() && mode == DSP()->CurMode()) {
            LT()->InitLayerTable();
            LT()->ShowLayerTable();
        }
    }

    // The list of unused layers, i.e., those that have been removed from
    // the layer table, has changed.  The new list for the display mode is
    // passed.
    //
    void
    ifUnusedLayerListChange(CDll *list, DisplayMode mode)
    {
        // Update layer editor if unused layer list changes.
        sLcb::check_update(list, mode);
    }
}


void
cLayerTab::setupInterface()
{
    CD()->RegisterIfLayerTableChange(ifLayerTableChange);
    CD()->RegisterIfUnusedLayerListChange(ifUnusedLayerListChange);
}

