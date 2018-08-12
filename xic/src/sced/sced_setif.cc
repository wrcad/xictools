
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
#include "fio.h"
#include "sced.h"
#include "menu.h"
#include "ebtn_menu.h"


//-----------------------------------------------------------------------------
// CD configuration

namespace {
    // The text name for the node is copied into nbuf.  If nbuf[0] is 0, a
    // default will be used.  Applies to electrical mode.
    //
    static const char*
    ifNodeName(const CDs *sdesc, int node, bool *glob)
    {
        return (SCD()->nodeName(sdesc, node, glob));
    }

    // Update the P_NODMAP property which contains a mapping between
    // internal node numbers and assigned node names.  Applies to
    // electrical mode.
    //
    static void
    ifUpdateNodes(const CDs *sdesc)
    {
        if (sdesc->isElectrical())
            SCD()->updateNodes(sdesc);
    }

    // Update the name label for cdesc from name property pna.  Applies to
    // electrical mode.
    //
    void
    ifUpdateNameLabel(CDc *cdesc, CDp_cname *pna)
    {
        SCD()->updateNameLabel(cdesc, pna);
    }

    // Create or update a mutual inductor label from mutual property pm.
    // Applies to electrical mode.  Return true on success.
    //
    static bool
    ifUpdateMutLabel(CDs *sdesc, CDp_nmut *pm)
    {
        return (SCD()->setMutLabel(sdesc, pm, 0));
    }

    // Create an instance label for cdesc.  Applies to electrical mode.
    //
    void
    ifLabelInstance(CDs *sdesc, CDc *cdesc)
    {
        if (sdesc->isElectrical())
            SCD()->genDeviceLabels(cdesc, 0, false);
    }

    // Update the property label.  Applies to electrical mode.
    //
    bool
    ifUpdatePrptyLabel(int prpnum, CDo *odesc, Label *label)
    {
        if (odesc->type() == CDINSTANCE && (GEO()->curTx()->angle() != 0 ||
                GEO()->curTx()->reflectY() || GEO()->curTx()->reflectX())) {
            // Instance was rotated, use default locations.
            SCD()->labelPlacement(prpnum, (CDc*)odesc, label);
            return (true);
        }
        return (false);
    }

    // The cell was just modified.
    //
    static void
    ifCellModified(const CDs *sdesc)
    {
        if (sdesc->isElectrical())
            SCD()->setModified(sdesc->nodes());
    }
}


//-----------------------------------------------------------------------------
// FIO configuration

// reuse ifUpdateNodes from above


//-----------------------------------------------------------------------------
// DSP configuration

namespace {
    // Hack for Xic, return true if the subct command mode is active.
    //
    static bool
    is_subct_cmd_active()
    {
        return (Menu()->MenuButtonStatus("main", MenuSUBCT) == 1);
    }
}


void
cSced::setupInterface()
{
    CD()->RegisterIfNodeName(ifNodeName);
    CD()->RegisterIfUpdateNodes(ifUpdateNodes);
    CD()->RegisterIfUpdateNameLabel(ifUpdateNameLabel);
    CD()->RegisterIfUpdateMutLabel(ifUpdateMutLabel);
    CD()->RegisterIfLabelInstance(ifLabelInstance);
    CD()->RegisterIfUpdatePrptyLabel(ifUpdatePrptyLabel);
    CD()->RegisterIfCellModified(ifCellModified);

    FIO()->RegisterIfUpdateNodes(ifUpdateNodes);

    DSP()->Register_is_subct_cmd_active(is_subct_cmd_active);
}

