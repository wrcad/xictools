
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
 $Id: layertab_setif.cc,v 5.4 2007/11/13 23:26:14 stevew Exp $
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

