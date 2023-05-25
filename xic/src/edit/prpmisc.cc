
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
#include "edit.h"
#include "undolist.h"
#include "scedif.h"
#include "dsp_color.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "promptline.h"
#include "tech.h"
#include "tech_attr_cx.h"


// Add/replace the property specified, taking care of pointers and
// redisplay.  For electrical, set hystr if not 0, otherwise string.
// Arg pdesc is the property in odesc to replace, or 0 if add only.
// Return the new property if one is created.
//
CDp *
cEdit::prptyModify(CDc *cdesc, CDp *oldp, int value, const char *string,
    hyList *hystr)
{
    if (!cdesc)
        return (0);
    CDs *sdesc = cdesc->parent();
    if (!sdesc)
        return (0);
    if (sdesc->isElectrical())
        return (ScedIf()->prptyModify(cdesc, oldp, value, string, hystr));
    if (!string)
        return (0);
    CDp *newp = new CDp(string, value);
    Ulist()->RecordPrptyChange(sdesc, cdesc, oldp, newp);
    return (newp);
}


//----------------------------------------------------------------------
// Internal Global Properties Maintenance
//----------------------------------------------------------------------

// Assign the global properties to sdesc.  Called just before a cell is
// saved or updated.
//
void
cEdit::assignGlobalProperties(CDcbin *cbin)
{
    if (!cbin)
        return;
    for (int pnum = INTERNAL_PROPERTY_BASE; prpty_global(pnum); pnum++) {
        if (pnum == XICP_GRID) {
            // physical only
            CDs *sdesc = cbin->phys();
            if (sdesc) {
                CDp *pdesc = sdesc->prpty(pnum);
                char buf[256];

                int res, snap;
                if (DSP()->DoingHcopy()) {
                    sAttrContext *cxbak = Tech()->GetHCbakAttrContext();
                    res = INTERNAL_UNITS(
                        cxbak->attr()->grid(Physical)->spacing(Physical));
                    snap = cxbak->attr()->grid(Physical)->snap();
                }
                else {
                    res = INTERNAL_UNITS(DSP()->MainWdesc()->Attrib()->
                        grid(Physical)->spacing(Physical));
                    snap = DSP()->MainWdesc()->Attrib()->
                        grid(Physical)->snap();
                }
                if (res == Tech()->DefaultPhysResol() &&
                        snap == Tech()->DefaultPhysSnap()) {
                    // These are the initial values after the techfile is
                    // read.  No property unless current values are
                    // different.
                    if (pdesc) {
                        sdesc->prptyUnlink(pdesc);
                        delete pdesc;
                    }
                }
                else {
                    snprintf(buf, sizeof(buf), "grid %d %d", res, snap);
                    if (pdesc)
                        pdesc->set_string(buf);
                    else
                        sdesc->prptyAdd(pnum, buf);
                }
            }
        }
        else if (pnum == XICP_RUN) {
            // electrical only
            CDs *sdesc = cbin->elec();
            if (sdesc) {

                CDp *pdesc = sdesc->prpty(pnum);
                if (pdesc) {
                    sdesc->prptyUnlink(pdesc);
                    delete pdesc;
                }
                char *s = ScedIf()->getAnalysis(true); // copy returned
                if (s && *s) {
                    CDp_glob *np = new CDp_glob(pnum);
                    np->set_string(s);
                    np->set_data(hyList::dup(ScedIf()->getAnalysisList()));
                    np->set_next_prp(sdesc->prptyList());
                    sdesc->setPrptyList(np);
                }
                delete [] s;
            }
        }
        else if (pnum == XICP_PLOT) {
            // electrical only
            CDs *sdesc = cbin->elec();
            if (sdesc) {

                CDp *pdesc = sdesc->prpty(pnum);
                if (pdesc) {
                    sdesc->prptyUnlink(pdesc);
                    delete pdesc;
                }
                char *s = ScedIf()->getPlotCmd(true); // copy returned
                if (s && *s) {
                    CDp_glob *np = new CDp_glob(pnum);
                    np->set_string(s);
                    np->set_data(hyList::dup(ScedIf()->getPlotList()));
                    np->set_next_prp(sdesc->prptyList());
                    sdesc->setPrptyList(np);
                }
                delete [] s;
            }
        }
        else if (pnum == XICP_IPLOT) {
            // electrical only
            CDs *sdesc = cbin->elec();
            if (sdesc) {
                CDp *pdesc = sdesc->prpty(pnum);
                if (pdesc) {
                    sdesc->prptyUnlink(pdesc);
                    delete pdesc;
                }
                char *s = ScedIf()->getIplotCmd(true);  // copy returned
                if (s && *s) {
                    CDp_glob *np = new CDp_glob(pnum);
                    np->set_string(s);
                    np->set_data(hyList::dup(ScedIf()->getIplotList()));
                    np->set_next_prp(sdesc->prptyList());
                    sdesc->setPrptyList(np);
                }
                delete [] s;
            }
        }
    }
}


// Assert the XIC properties stored with sdesc.  Called whenever a new
// cell is opened for editing.  The properties are removed from the
// cell, they are added back when the cell is saved.
//
void
cEdit::assertGlobalProperties(CDcbin *cbin)
{
    if (!cbin)
        return;
    char buf[512];
    // physical
    CDp *pprev = 0, *pdesc, *pnext;
    CDs *sdesc = cbin->phys();
    if (sdesc) {
        for (pdesc = sdesc->prptyList(); pdesc; pdesc = pnext) {
            pnext = pdesc->next_prp();
            if (prpty_global(pdesc->value())) {
                if (pdesc->value() == XICP_GRID) {
                    int i1, i2;
                    if (pdesc->string() && sscanf(pdesc->string(), "%s %d %d",
                            buf, &i1, &i2) == 3) {
                        if (DSP()->DoingHcopy()) {
                            sAttrContext *cxbak =
                                Tech()->GetHCbakAttrContext();
                            cxbak->attr()->grid(Physical)->set_spacing(
                                MICRONS(i1));
                            cxbak->attr()->grid(Physical)->set_snap(i2);
                        }
                        else {
                            DSP()->MainWdesc()->Attrib()->
                                grid(Physical)->set_spacing(MICRONS(i1));
                            DSP()->MainWdesc()->Attrib()->
                                grid(Physical)->set_snap(i2);
                        }
                    }
                    if (!pprev)
                        sdesc->setPrptyList(pnext);
                    else
                        pprev->set_next_prp(pnext);
                    delete pdesc;
                    continue;
                }
            }
            pprev = pdesc;
        }
    }
    // electrical
    pprev = 0;
    sdesc = cbin->elec();
    if (sdesc) {
        for (pdesc = sdesc->prptyList(); pdesc; pdesc = pnext) {
            pnext = pdesc->next_prp();
            if (prpty_global(pdesc->value())) {

                if (pdesc->value() == XICP_RUN) {
                    if (pdesc->string() &&
                            cbin->cellname() == DSP()->TopCellName()) {
                        sLstr lstr;
                        pdesc->print(&lstr, 0, 0);
                        ScedIf()->setAnalysis(lstr.string());
                    }
                    if (!pprev)
                        sdesc->setPrptyList(pnext);
                    else
                        pprev->set_next_prp(pnext);
                    delete pdesc;
                    continue;
                }
                if (pdesc->value() == XICP_PLOT) {
                    if (pdesc->string() &&
                            cbin->cellname() == DSP()->TopCellName()) {
                        sLstr lstr;
                        pdesc->print(&lstr, 0, 0);
                        ScedIf()->setPlotCmd(lstr.string());
                    }
                    if (!pprev)
                        sdesc->setPrptyList(pnext);
                    else
                        pprev->set_next_prp(pnext);
                    delete pdesc;
                    continue;
                }
                if (pdesc->value() == XICP_IPLOT) {
                    if (cbin->cellname() == DSP()->TopCellName()) {
                        sLstr lstr;
                        pdesc->print(&lstr, 0, 0);
                        ScedIf()->setIplotCmd(lstr.string());
                    }
                    if (!pprev)
                        sdesc->setPrptyList(pnext);
                    else
                        pprev->set_next_prp(pnext);
                    delete pdesc;
                    continue;
                }
            }
            pprev = pdesc;
        }
    }
}


// Strip out any global properties that apply to top level cells only.
//
void
cEdit::stripGlobalProperties(CDs *sdesc)
{
    if (!sdesc)
        return;
    for (int pnum = INTERNAL_PROPERTY_BASE; prpty_global(pnum); pnum++)
        sdesc->prptyRemove(pnum);
}

