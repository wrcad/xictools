
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

#include "main.h"
#include "ext.h"
#include "edit.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "events.h"
#include "layertab.h"
#include "promptline.h"
#include "tech.h"


//------ Ground Plane -----------------------------------------------------

// Internal layer name for inverted ground plane layer.
#define GP_INV_LNAME "$GPI"

// The following functions are for handling the ground plane layer in
// extraction.  The layer attributes can set the following.
//
//  1. GROUNDPLANE and not DARKFIELD and not inverting
//      Handle like a Conductor,_inv assign 0 to largest area group.
//
//  2. GROUNDPLANE and DARKFIELD and not inverting
//      Assume that the inverse of the layer is group 0.
//
//  3. GROUNDPLANE and DARKFIELD and inverting
//      Create an actual inverse layer, and use this in grouping.


bool
cExt::setInvGroundPlaneImmutable(bool b)
{
    if (ext_gp_layer_inv) {
        bool ret = ext_gp_layer_inv->isImmutable();
        ext_gp_layer_inv->setImmutable(b);
        return (ret);
    }
    return (false);
}


// Create the inverse ground plane in the hierarchy.
//
XIrt
cExt::setupGroundPlane(CDs *sdesc)
{
    GPItype mode = Tech()->GroundPlaneMode();
    if (Tech()->IsInvertGroundPlane()) {
        ext_gp_layer = 0;
        ext_gp_layer_inv = 0;
        CDl *ld;
        CDlgen lgen(Physical);
        while ((ld = lgen.next()) != 0) {
            if (ld->isGroundPlane() && ld->isDarkField()) {
                ext_gp_layer = ld;
                ext_gp_layer_inv = CDldb()->findLayer(GP_INV_LNAME, Physical);
                if (ext_gp_layer_inv)
                    ext_gp_layer_inv->setImmutable(false);
                XIrt ret = invertGroundPlane(sdesc, mode,
                    GP_INV_LNAME, ext_gp_layer->name());
                if (ret != XIok) {
                    ext_gp_layer = 0;
                    ext_gp_layer_inv = 0;
                    return (ret);
                }
                ext_gp_layer_inv = CDldb()->findLayer(GP_INV_LNAME, Physical);
                if (ext_gp_layer_inv)
                    ext_gp_layer_inv->setImmutable(true);
                break;
            }
        }
    }
    return (XIok);
}


// Set/unset the use of the temporary inverse ground plane layer.
//
void
cExt::activateGroundPlane(bool set)
{
    if (!ext_gp_layer_inv || !ext_gp_layer)
        return;
    if (set && !ext_gp_inv_set) {
        // ext_gp_layer becomes a non-conductor (and non-via),
        // ext_gp_layer_inv becomes a normal (dark active) ground plane

        ext_gp_layer_inv->setConductor(true);
        ext_gp_layer_inv->setRouting(false);
        ext_gp_layer_inv->setVia(false);
        ext_gp_layer_inv->setInContact(false);
        ext_gp_layer_inv->setGroundPlane(true);
        ext_gp_layer_inv->setDarkField(false);
        ext_gp_layer_inv->setInvisible(true);

        ext_gp_layer->setConductor(false);
        ext_gp_layer->setRouting(false);
        ext_gp_layer->setVia(false);
        ext_gp_layer->setInContact(false);
        ext_gp_layer->setGroundPlane(false);
        ext_gp_layer->setDarkField(false);
        ext_gp_inv_set = true;
    }
    else if (!set && ext_gp_inv_set) {

        ext_gp_layer->setConductor(true);
        ext_gp_layer->setRouting(false);
        ext_gp_layer->setVia(false);
        ext_gp_layer->setInContact(false);
        ext_gp_layer->setGroundPlane(true);
        ext_gp_layer->setDarkField(true);

        ext_gp_layer_inv->setConductor(false);
        ext_gp_layer_inv->setRouting(false);
        ext_gp_layer_inv->setVia(false);
        ext_gp_layer_inv->setInContact(false);
        ext_gp_layer_inv->setGroundPlane(false);
        ext_gp_layer_inv->setDarkField(false);
        ext_gp_inv_set = false;
    }
}


// This creates the $GPI layer for a dark-field ground plane,  The mode
// is one of:
//   GPI_PLACE    The $GPI layer is created in in each cell, by computing
//                $GPI = !GPD & !$$, i.e., the local ground plane is
//                inverted, and the area over subcells is removed.
//   GPI_TOP      The $GPI layer is computed by reflecting all ground
//                plane structure to the top cell and inverting.  Only
//                the top cell contains $GPI.
//   GPI_ALL      The $GPI layer is created in each cell, by reflecting
//                all geometry lower in the hierarchy to the present
//                cell and inverting.
//
// All cells in the hierarchy are processed, and the operation is not
// undoable.
//
// The first method should work for a hierarchy since virtual contacts
// will be added to provide grounds in the subcells.  The second
// method is more efficient, but may not work if sibling subcells
// overlap.  The third method should work in all cases, though most
// likely a lot of redundant geometry is created.
//
XIrt
cExt::invertGroundPlane(CDs *sdesc, int mode, const char *gpiname,
    const char *gpname)
{
    if (!sdesc)
        return (XIok);
    // For library gates, forget about the ground plane.
    if (EX()->skipExtract(sdesc))
        return (XIok);
    dspPkgIf()->SetWorking(true);
    if (EX()->isVerbosePromptline())
        PL()->PushPromptV("Inverting ground plane in %s: ",
            Tstring(sdesc->cellname()));
    else
        PL()->ShowPromptV("Inverting ground plane in %s ...",
            Tstring(sdesc->cellname()));
    char buf[128];
    XIrt ret = XIok;
    if (mode == GPI_PLACE) {
        sprintf(buf, "%s = !%s&!$$", gpiname, gpname);
        CDgenHierDn_s gen(sdesc, CDMAXCALLDEPTH);
        bool err;
        CDs *sd;
        while ((sd = gen.next(&err)) != 0) {
            if (EX()->skipExtract(sd))
                continue;
            if (!sd->isGPinv()) {
                ret = ED()->createLayer(sd, buf, 0, CLdefault | CLnoUndo);
                if (ret == XIok)
                    sd->setGPinv(true);
            }
            if (ret != XIok)
                break;
        }
        if (err)
            ret = XIbad;
    }
    else if (mode == GPI_TOP) {
        if (!sdesc->isGPinv()) {
            sprintf(buf, "%s = !%s", gpiname, gpname);
            ret = ED()->createLayer(sdesc, buf, CDMAXCALLDEPTH,
                CLdefault | CLnoUndo);
            if (ret == XIok)
                sdesc->setGPinv(true);
        }
    }
    else if (mode == GPI_ALL) {
        sprintf(buf, "%s = !%s", gpiname, gpname);
        CDgenHierDn_s gen(sdesc, CDMAXCALLDEPTH);
        bool err;
        CDs *sd;
        while ((sd = gen.next(&err)) != 0) {
            if (EX()->skipExtract(sd))
                continue;
            if (!sd->isGPinv()) {
                ret = ED()->createLayer(sd, buf, CDMAXCALLDEPTH,
                    CLdefault | CLnoUndo);
                if (ret == XIok)
                    sd->setGPinv(true);
            }
            if (ret != XIok)
                break;
        }
        if (err)
            ret = XIbad;

    }
    if (EX()->isVerbosePromptline())
        PL()->PopPrompt();
    dspPkgIf()->SetWorking(false);
    return (ret);
}
// End ground plane functions.

