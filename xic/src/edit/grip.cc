
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
#include "grip.h"
#include "pcell.h"
#include "pcell_params.h"
#include "cd_propnum.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "events.h"
#include "ghost.h"
#include "undolist.h"
#include "errorlog.h"
#include "spnumber/spnumber.h"


//
// Stretch handle (grip) implementation.  This is mostly for pcells.
//
// Each physical cell can have a table of grips.  Each grip is
// associated with a box in a pcell instance.  Dragging the grip will
// cause re-evaluation of the pcell with the modified parameter.  We
// provide a scripting interface, and support Ciranova pcells.
//
// Objects are associated with a grip by a property.  When a pcell is
// selected, the geometry is searched for these properties.  For each
// property, an sGrip is created and linked into a database.
//
// Grips are shown in selected expanded instances only.
//
// In the button press/motion event handling:  If the press is over an
// expanded pcell instance, we check if the press is over (or close
// to) a grip.  If so, the grip is "selected", and attached to the
// mouse pointer.  The motion is constrained to be perpendicular to
// the grip edge.  On button-up, the change is processed and the pcell
// instance may be reevaluated.
//


namespace {
    void accum_init()
    {
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CHD);
        while ((wdesc = wgen.next()) != 0) {
            if (Gst()->ShowingGhostInWindow(wdesc))
                wdesc->SetAccumMode(WDaccumStart);
        }
    }

    void accum_final()
    {
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CHD);
        while ((wdesc = wgen.next()) != 0) {
            if (Gst()->ShowingGhostInWindow(wdesc))
                wdesc->GhostFinalUpdate();
        }
    }
}


// Go through the pcell sub-master's geometry and look for XICP_GRIP
// properties.  For each found, set up the associated grip.  If cdesc
// is null and the current cell is a sub-master, add grips for the
// current cell geometry.
//
bool
cEdit::registerGrips(CDc *cdesc)
{
    if (ED()->hideGrips())
        return (true);
    CDs *msdesc = cdesc ? cdesc->masterCell() : CurCell();
    if (!msdesc || !msdesc->isPCellSubMaster() || msdesc->isElectrical())
        return (true);
    if (ed_gripdb && ed_gripdb->hasCdesc(cdesc)) {
        if (cdesc)
            return (true);
        unregisterGrips(0);
    }

    CDsLgen lgen(msdesc);
    CDl *ld;
    while ((ld = lgen.next()) != 0) {
        CDg gdesc;
        gdesc.init_gen(msdesc, ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {

            // Grips can use only box edges (for now).
            if (odesc->type() != CDBOX)
                continue;

            for (CDp *pd = odesc->prpty(XICP_GRIP); pd; pd = pd->next_prp()) {
                if (pd->value() != XICP_GRIP)
                    continue;
                if (!ed_gripdb)
                    ed_gripdb = new cGripDb;

                const char *s = pd->string();
                sCniGripDesc cg;
                while (*s) {
                    if (!cg.parse(&s)) {
                        Log()->ErrorLog(mh::Initialization,
                            Errs()->get_error());
                        break;
                    }
                    sGrip *grip = new sGrip(cdesc);
                    if (grip->setup(cg, odesc->oBB())) 
                        ed_gripdb->saveGrip(grip);
                    else {
                        delete grip;
                        Log()->ErrorLog(mh::Initialization,
                            Errs()->get_error());
                    }
                    while (isspace(*s) || *s == ';' || *s == ',')
                        s++;
                }
            }
        }
    }
    return (true);
}


// Delete the grips associated with cdesc, if any.  Note that cdesc=0
// may represent a class of grips not associated with a cell instance.
//
void
cEdit::unregisterGrips(const CDc *cdesc)
{
    if (ed_gripdb)
        ed_gripdb->deleteGrip(cdesc, -1);
}


// This is called from the main state button down handler.  If button
// 1 is pressed over or near a grip (generally in an expended pcell
// instance), initiate a grip drag.
//
bool
cEdit::checkGrips()
{
    if (hideGrips())
        return (false);
    if (DSP()->CurMode() != Physical)
        return (false);
    if (!ed_gripdb)
        return (false);

    int x, y;
    EV()->Cursor().get_raw(&x, &y);
    BBox BB(x, y, x, y);
    WindowDesc *wdesc = EV()->CurrentWin() ?
        EV()->CurrentWin() : DSP()->MainWdesc();
    int delta = 1 + (int)(DSP()->PixelDelta()/wdesc->Ratio());
    BB.bloat(delta);

    int xlev = wdesc->Attrib()->expand_level(Physical);

    CDg gdesc;
    gdesc.init_gen(CurCell(), CellLayer(), &BB);
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {

        // Only if showing expanded.
        if (xlev == 0 && !cdesc->has_flag(wdesc->DisplFlags()))
            continue;

        // Only if the displayed instance on-screen size is
        // "reasonable".  We want to limit the occurrence where the
        // user tries to move the subcell and drags a grip instead.
        int len = cdesc->oBB().width();
        int tmp = cdesc->oBB().height();
        if (tmp < len)
            len = tmp;
        len = (int)(len * wdesc->Ratio());
        if (len < DSP()->FenceInstPixSize())
            continue;

        sGripGen gen(ed_gripdb, cdesc);
        sGrip *grip;
        while ((grip = gen.next()) != 0) {
            int x1 = grip->end1x();
            int y1 = grip->end1y();
            int x2 = grip->end2x();
            int y2 = grip->end2y();
            if (!GEO()->line_clip(&x1, &y1, &x2, &y2, &BB)) {
                ed_cur_grip = grip;
                Gst()->SetGhost(GFgrip);
                accum_init();
                return (true);
            }
        }
    }
    {
        sGripGen gen(ed_gripdb, 0);
        sGrip *grip;
        while ((grip = gen.next()) != 0) {
            int x1 = grip->end1x();
            int y1 = grip->end1y();
            int x2 = grip->end2x();
            int y2 = grip->end2y();
            if (!GEO()->line_clip(&x1, &y1, &x2, &y2, &BB)) {
                ed_cur_grip = grip;
                Gst()->SetGhost(GFgrip);
                accum_init();
                return (true);
            }
        }
    }
    return (false);
}


// This is called from the main state button up handler.  Terminate
// the grip drag, if any.  Handle changes.  Called from mainstate esc
// handler with bail true, to abort.
//
bool
cEdit::resetGrips(bool bail)
{
    if (!ed_cur_grip)
        return (false);

    Gst()->SetGhost(GFnone);
    accum_final();

    sGrip *grip = ed_cur_grip;
    ed_cur_grip = 0;

    if (bail)
        return (true);

    int ox, oy;
    EV()->Cursor().get_raw(&ox, &oy);
    int nx, ny;
    EV()->Cursor().get_release(&nx, &ny);
    EV()->CurrentWin()->Snap(&nx, &ny);

    double prm_val;
    if (grip->param_value(ox, oy, nx, ny, &prm_val)) {
        if (!resetInstance(grip->cdesc(), grip->param_name(), prm_val)) {
            Errs()->add_error("resetInstance failed:\n%s",
                Errs()->get_error());
            Log()->ErrorLog(mh::Initialization, Errs()->get_error());
            return (false);
        }
    }
    return (true);
}
// End of cEdit functions.


// This draws a movable grip for use as stretch handle in pcells.
//
void
cEditGhost::showGhostGrip(int map_x, int map_y, int, int, bool erase)
{
    sGrip *grip = ED()->getCurGrip();
    if (grip)
        grip->show_ghost(map_x, map_y, erase);
}
// End of cEditGhost functions.


sCniGripDesc::sCniGripDesc()
{
    gd_name = 0;
    gd_param = 0;
    gd_key = 0;
    gd_minval = 0.0;
    gd_maxval = 0.0;
    gd_scale = 1.0;
    gd_snap = 0.0;
    gd_loc = CN_LL;
    gd_absolute = false;
    gd_vert = false;
}


sCniGripDesc::~sCniGripDesc()
{
    delete [] gd_name;
    delete [] gd_param;
    delete [] gd_key;
}


// Parse a Ciranova-style stretch parameter string, which has the form
//   name:val; stretchType:val, direction:val, parameter:val, minVal:val,
//     maxVal:val, location:val, userScale:val, userSnap:val, key:val
//
// Xic differences:
// Space can follow ; and , separators in Xic, no space in Ciranova. 
// The ; and , are in fact optional, tokens can be space-separated. 
// Any duplicate keyword terminates the specification.  Ciranova
// apparently assumes one big token, no space, but space can separate
// specifications in the property string.  Ciranova also expects all
// fields to be defined in each specification, except perhaps name. 
// The name can be omitted if the key changes value.
//
// A parse error returns false, with a message in the Errs system.
//
bool
sCniGripDesc::parse(const char **pstr)
{
    const char *tchrs = ":;,";
    const char *s = *pstr;
    const char *last = s;
    bool have_name = false;
    bool have_stretchtype = false;
    bool have_direction = false;
    bool have_parameter = false;
    bool have_minval = false;
    bool have_maxval = false;
    bool have_location = false;
    bool have_userscale = false;
    bool have_usersnap = false;
    bool have_key = false;
    bool have_haveunits = false;

    char *tok;
    while ((tok = lstring::gettok(&s, tchrs)) != 0) {
        if (!strcasecmp(tok, "name")) {
            delete [] tok;
            if (have_name) {
                // Already have this, we're done.
                s = last;
                break;
            }
            have_name = true;
            tok = lstring::gettok(&s, tchrs);
            if (!tok) {
                Errs()->add_error("Missing value for \"name\".");
                return (false);
            }
            delete [] gd_name;
            gd_name = tok;
            tok = 0;
        }
        else if (!strcasecmp(tok, "stretchtype")) {
            delete [] tok;
            if (have_stretchtype) {
                // Already have this, we're done.
                s = last;
                break;
            }
            have_stretchtype = true;
            tok = lstring::gettok(&s, tchrs);
            if (!tok) {
                Errs()->add_error("Missing value for \"stretchType\".");
                return (false);
            }
            // "absolute" or "relative"
            gd_absolute = (*tok == 'a' || *tok == 'A');
        }
        else if (!strcasecmp(tok, "direction")) {
            delete [] tok;
            if (have_direction) {
                // Already have this, we're done.
                s = last;
                break;
            }
            have_direction = true;
            tok = lstring::gettok(&s, tchrs);
            if (!tok) {
                Errs()->add_error("Missing value for \"direction\".");
                return (false);
            }
            // "north_south" or "east_west"
            gd_vert =  (*tok == 'n' || *tok == 'N');
        }
        else if (!strcasecmp(tok, "parameter")) {
            delete [] tok;
            if (have_parameter) {
                // Already have this, we're done.
                s = last;
                break;
            }
            have_parameter = true;
            tok = lstring::gettok(&s, tchrs);
            if (!tok) {
                Errs()->add_error("Missing value for \"parameter\".");
                return (false);
            }
            delete [] gd_param;
            gd_param = tok;
            tok = 0;
        }
        else if (!strcasecmp(tok, "minval")) {
            delete [] tok;
            if (have_minval) {
                // Already have this, we're done.
                s = last;
                break;
            }
            have_minval = true;
            tok = lstring::gettok(&s, tchrs);
            if (!tok) {
                Errs()->add_error("Missing value for \"minVal\".");
                return (false);
            }
            const char *t = tok;
            double *d = SPnum.parse(&t, true, false, 0);
            if (!d) {
                Errs()->add_error("Number parse failed for \"minVal\".");
                delete [] tok;
                return (false);
            }
            gd_minval = *d;
        }
        else if (!strcasecmp(tok, "maxval")) {
            delete [] tok;
            if (have_maxval) {
                // Already have this, we're done.
                s = last;
                break;
            }
            have_maxval = true;
            tok = lstring::gettok(&s, tchrs);
            if (!tok) {
                Errs()->add_error("Missing value for \"maxVal\".");
                return (false);
            }
            const char *t = tok;
            double *d = SPnum.parse(&t, true, false, 0);
            if (!d) {
                Errs()->add_error("Number parse failed for \"maxVal\".");
                delete [] tok;
                return (false);
            }
            gd_maxval = *d;
        }
        else if (!strcasecmp(tok, "location")) {
            delete [] tok;
            if (have_location) {
                // Already have this, we're done.
                s = last;
                break;
            }
            have_location = true;
            tok = lstring::gettok(&s, tchrs);
            if (!tok) {
                Errs()->add_error("Missing value for \"location\".");
                return (false);
            }
            char *t = tok;
            if (lstring::ciprefix("location:", t))
                t += strlen("location:");
            if (!strcasecmp(t, "lower_left"))
                gd_loc = CN_LL;
            else if (!strcasecmp(t, "center_left"))
                gd_loc = CN_CL;  // L
            else if (!strcasecmp(t, "upper_left"))
                gd_loc = CN_UL;
            else if (!strcasecmp(t, "lower_center"))
                gd_loc = CN_LC;  // B
            else if (!strcasecmp(t, "center_center"))
                gd_loc = CN_CC;
            else if (!strcasecmp(t, "upper_center"))
                gd_loc = CN_UC;  // T
            else if (!strcasecmp(t, "lower_right"))
                gd_loc = CN_LR;
            else if (!strcasecmp(t, "center_right"))
                gd_loc = CN_CR;  // R
            else if (!strcasecmp(t, "upper_right"))
                gd_loc = CN_UR;
            else {
                Errs()->add_error("Unsupported location \"%s\".", t);
                delete [] tok;
                return (false);
            }
        }
        else if (!strcasecmp(tok, "userscale")) {
            delete [] tok;
            if (have_userscale) {
                // Already have this, we're done.
                s = last;
                break;
            }
            have_userscale = true;
            tok = lstring::gettok(&s, tchrs);
            if (!tok) {
                Errs()->add_error("Missing value for \"userScale\".");
                return (false);
            }
            const char *t = tok;
            double *d = SPnum.parse(&t, true, false, 0);
            if (!d) {
                Errs()->add_error("Number parse failed for \"userScale\".");
                delete [] tok;
                return (false);
            }
            gd_scale = *d;
        }
        else if (!strcasecmp(tok, "usersnap")) {
            delete [] tok;
            if (have_usersnap) {
                // Already have this, we're done.
                s = last;
                break;
            }
            have_usersnap = true;
            tok = lstring::gettok(&s, tchrs);
            if (!tok) {
                Errs()->add_error("Missing value for \"userSnap\".");
                return (false);
            }
            const char *t = tok;
            double *d = SPnum.parse(&t, true, false, 0);
            if (!d) {
                Errs()->add_error("Number parse failed for \"userSnap\".");
                delete [] tok;
                return (false);
            }
            gd_snap = *d;
        }
        else if (!strcasecmp(tok, "key")) {
            delete [] tok;
            if (have_key) {
                // Already have this, we're done.
                s = last;
                break;
            }
            have_key = true;
            tok = lstring::gettok(&s, tchrs);
            if (!tok) {
                Errs()->add_error("Missing value for \"key\".");
                return (false);
            }
            delete [] gd_key;
            gd_key = tok;
            tok = 0;
            break;
        }
        else if (!strcasecmp(tok, "hasUnits")) {
            //  Hmmmm, undocumented?
            delete [] tok;
            if (have_haveunits) {
                // Already have this, we're done.
                s = last;
                break;
            }
            have_haveunits = true;
            tok = lstring::gettok(&s, tchrs);
        }
        else {
            Errs()->add_error("Unknown keyword \"%s\".", tok);
            delete [] tok;
            return (false);
        }
        delete [] tok;
        last = s;
    }
    *pstr = s;
    return (true);
}
// End of sCniGripDesc functions.


sGrip::sGrip(CDc *cd)
{
    g_value = 0.0;
    g_cdesc = cd;
    g_constr = 0;
    g_id = 0;
    g_x1 = 0;
    g_y1 = 0;
    g_x2 = 0;
    g_y2 = 0;
    g_ux = 0;
    g_uy = 0;
    g_active = false;
}


sGrip::~sGrip()
{
    if (ED()->getCurGrip() == this)
        ED()->resetGrips(true);
    set_active(false);
}


namespace {
    // Obtain the value for prm_name from the property string.
    //
    bool get_value(const CDc *cdesc, const char *prm_name, double *prm_value)
    {
        CDp *prp;
        if (cdesc)
            prp = cdesc->prpty(XICP_PC_PARAMS);
        else
            prp = CurCell()->prpty(XICP_PC_PARAMS);
        if (!prp)
            return (false);

        PCellParam *pcp;
        if (!PCellParam::parseParams(prp->string(), &pcp))
            return (false);

        bool found = false;
        for (PCellParam *p = pcp; p; p = p->next()) {
            if (!strcmp(prm_name, p->name())) {
                if (p->type() == PCPstring) {
                    found = (sscanf(p->stringVal(), "%lf", prm_value) == 1);
                    break;
                }
                if (p->type() == PCPbool) {
                    *prm_value = p->boolVal();
                    found = true;
                    break;
                }
                if (p->type() == PCPint) {
                    *prm_value = p->intVal();
                    found = true;
                    break;
                }
                if (p->type() == PCPtime) {
                    *prm_value = p->timeVal();
                    found = true;
                    break;
                }
                if (p->type() == PCPfloat) {
                    *prm_value = p->floatVal();
                    found = true;
                    break;
                }
                if (p->type() == PCPdouble) {
                    *prm_value = p->doubleVal();
                    found = true;
                    break;
                }
            }
        }
        PCellParam::destroy(pcp);
        return (found);
    }
}


// Set up the grip fields.  The BB is the box of the reference object
// that contains the property (in master-cell coords).
//
bool
sGrip::setup(const sCniGripDesc &gd, const BBox &BB)
{
    setgd(gd);
    // Just fake these if not set.
    if (!gd_name)
        gd_name = lstring::copy("unnamed");
    if (!gd_key)
        gd_key = lstring::copy("None");

    if (!gd_param || !*gd_param) {
        Errs()->add_error("Can't determine grip parameter name.");
        return (false);
    }
    if (!get_value(g_cdesc, gd_param, &g_value)) {
        Errs()->add_error("Can't determine grip parameter value.");
        return (false);
    }

    CDp *pd = g_cdesc->prpty(XICP_PC);
    if (pd) {
        char *dbname = PCellDesc::canon(pd->string());
        PCellParam *pcp;
        if (PC()->getDefaultParams(dbname, &pcp)) {
            pcp = PCellParam::find(pcp, gd_param);
            if (pcp)
                g_constr = pcp->constraint();
        }
        delete [] dbname;
    }

    // The vector x1,y1 -> x2,y2 points counter-clockwise relative to
    // the box center.

    int x1, y1, x2, y2;
    switch (gd_loc) {
    case CN_LL:
        x1 = BB.left;
        y1 = BB.bottom;
        x2 = BB.left;
        y2 = BB.bottom;
        break;
    case CN_CL: // L
        x1 = BB.left;
        y1 = BB.top;
        x2 = BB.left;
        y2 = BB.bottom;
        break;
    case CN_UL:
        x1 = BB.left;
        y1 = BB.top;
        x2 = BB.left;
        y2 = BB.top;
        break;
    case CN_LC: // B
        x1 = BB.left;
        y1 = BB.bottom;
        x2 = BB.right;
        y2 = BB.bottom;
        break;
    default:
    case CN_CC:
        x1 = (BB.left + BB.right)/2;
        y1 = (BB.bottom + BB.top)/2;
        x2 = (BB.left + BB.right)/2;
        y2 = (BB.bottom + BB.top)/2;
        break;
    case CN_UC: // T
        x1 = BB.right;
        y1 = BB.top;
        x2 = BB.left;
        y2 = BB.top;
        break;
    case CN_LR:
        x1 = BB.right;
        y1 = BB.bottom;
        x2 = BB.right;
        y2 = BB.bottom;
        break;
    case CN_CR: // R
        x1 = BB.right;
        y1 = BB.bottom;
        x2 = BB.right;
        y2 = BB.top;
        break;
    case CN_UR:
        x1 = BB.right;
        y1 = BB.top;
        x2 = BB.right;
        y2 = BB.top;
        break;
    }

    if (gd_vert) {
        g_ux = 0;
        g_uy = 1;
    }
    else {
        g_ux = 1;
        g_uy = 0;
    }

    if (g_cdesc) {
        // Convert to parent cell coordinates.
        cTfmStack stk;
        stk.TPush();
        stk.TApplyTransform(g_cdesc);

        int xm = (x1 + x2)/2;
        int ym = (y1 + y2)/2;
        int xmp = xm;
        int ymp = ym;
        if (gd_vert)
            ymp += 10;
        else
            xmp += 10;
        stk.TPoint(&xm, &ym);
        stk.TPoint(&xmp, &ymp);
        xmp -= xm;
        ymp -= ym;

        if (abs(ymp) <= 1) {
            g_uy = 0;
            if (xmp > 0)
                g_ux = 1;
            else
                g_ux = -1;
        }
        else if (abs(xmp) <= 1) {
            g_ux = 0;
            if (ymp > 0)
                g_uy = 1;
            else
                g_uy = -1;
        }
        else if (xmp > 0) {
            g_ux = 1;
            if (ymp > 0)
                g_uy = 1;
            else
                g_uy = -1;
        }
        else {
            g_ux = -1;
            if (ymp > 0)
                g_uy = 1;
            else
                g_uy = -1;
        }

        stk.TPoint(&x1, &y1);
        stk.TPoint(&x2, &y2);
        stk.TPop();
    }

    g_x1 = x1;
    g_y1 = y1;
    g_x2 = x2;
    g_y2 = y2;
    return (true);
}


// Set the activity status of the grip.  When inactive, the grip is
// invisible, and can't be moved.
//
void
sGrip::set_active(bool actv)
{
    if (actv == g_active)
        return;
    if (actv) {
        DSP()->ShowFenceMark(DISPLAY, g_cdesc, g_id, g_x1, g_y1, g_x2, g_y2,
            HighlightingColor);
        g_active = true;
    }
    else {
        DSP()->ShowFenceMark(ERASE, g_cdesc, g_id, g_x1, g_y1, g_x2, g_y2,
            HighlightingColor);
        g_active = false;
    }
}


namespace {
    inline double distance(int x, int y)
    {
        return (sqrt(x*(double)x + y*(double)y));
    }
}


// Compute the parameter value after pointer motion from x1,y1 to x2,y2.
// Returns true if the parameter has changed value, false otherwise.
//
bool
sGrip::param_value(int x1, int y1, int x2, int y2, double *pval) const
{
    WindowDesc *wdesc =
        EV()->CurrentWin() ? EV()->CurrentWin() : DSP()->MainWdesc();

    // Ignore if motion is two pixels or less.
    double d = distance(x2-x1, y2-y1)*wdesc->Ratio();
    if (d <= 2.0)
        return (false);

#if 1
    Point_c p1(end1x(), end1y());
    Point_c p2(end2x(), end2y());
    Point_c pmid((p1.x + p2.x)/2, (p1.y + p2.y)/2);

    // We don't care about x1,y1.  The "up" location x2,y2 defines
    // the new parameter value.
    int dx = x2 - pmid.x;
    int dy = y2 - pmid.y;
    double a = (g_ux*dx + g_uy*dy)/CDphysResolution;
    if (g_ux && g_uy)
        a /= M_SQRT2;

    double scale = gd_scale;
    double snap = gd_snap;
    // Note gd_absolute is not handled.

    double dnew = a*scale;
    if (snap != 0.0)
        dnew = snap*(int)(dnew/snap);
    dnew += g_value;

    double amax = (gd_maxval - g_value)/scale;
    double amin = (gd_minval - g_value)/scale;
    bool rv = false;
    if (amax < amin) {
        double t = amin;
        amin = amax;
        amax = t;
        rv = true;
    }
    if (a > amax) {
        a = amax;
        dnew = rv ? gd_minval : gd_maxval;
    }
    else if (a < amin) {
        a = amin;
        dnew = rv ? gd_maxval : gd_minval;
    }

    // Check against the constraint, if any.
    if (g_constr && !g_constr->checkConstraint(dnew))
        return (false);

#else
    Point_c p1(end1x(), end1y());
    Point_c p2(end2x(), end2y());
    double dnew;
    if (p1 != p2) {
        Point_c pmid((p1.x + p2.x)/2, (p1.y + p2.y)/2);
        Point_c p2m(p2.x - pmid.x, p2.y - pmid.y);
        d = distance(p2m.x, p2m.y);
        double ux = p2m.y/d;
        double uy = -p2m.x/d;

        // We don't care about x1,y1.  The "up" location x2,y2 defines
        // the new parameter value.
        int dx = x2 - pmid.x;
        int dy = y2 - pmid.y;
        double a = (ux*dx + uy*dy)/CDphysResolution;

        double scale = gd_scale;
        double snap = gd_snap;
        // Note gd_absolute is not handled, wtf does it do?

        dnew = a*scale;
        if (snap != 0.0)
            dnew = snap*(int)(dnew/snap);
        dnew += g_value;

        double amax = (gd_maxval - g_value)/scale;
        double amin = (gd_minval - g_value)/scale;
        bool rv = false;
        if (amax < amin) {
            double t = amin;
            amin = amax;
            amax = t;
            rv = true;
        }
        if (a > amax) {
            a = amax;
            dnew = rv ? gd_minval : gd_maxval;
        }
        else if (a < amin) {
            a = amin;
            dnew = rv ? gd_maxval : gd_minval;
        }

        // Check against the constraint, if any.
        if (g_constr && !g_constr->checkConstraint(dnew))
            return (false);
    }
    else {
        // We don't care about x1,y1.  The "up" location x2,y2 defines the
        // new parameter value.
        int dx = x2 - p1.x;
        int dy = y2 - p1.y;
        int ux = gd_vert ? 0 : 1;
        int uy = gd_vert ? 1 : 0;
        
//XXX
/*
    cTfmStack stk;
    stk.TPush();
    stk.TApplyTransform(g_cdesc);
    stk.TPoint(&ux, &uy);
    stk.TPop();
*/
        double a = (ux*dx + uy*dy)/CDphysResolution;

        double scale = gd_scale;
        double snap = gd_snap;

        dnew = a*scale;
        if (snap != 0.0)
            dnew = snap*(int)(dnew/snap);
        dnew += g_value;

        double amax = (gd_maxval - g_value)/scale;
        double amin = (gd_minval - g_value)/scale;
        bool rv = false;
        if (amax < amin) {
            double t = amin;
            amin = amax;
            amax = t;
            rv = true;
        }
        if (a > amax) {
            a = amax;
            dnew = rv ? gd_minval : gd_maxval;
        }
        else if (a < amin) {
            a = amin;
            dnew = rv ? gd_maxval : gd_minval;
        }

        // Check against the constraint, if any.
        if (g_constr && !g_constr->checkConstraint(dnew))
            return (false);
    }
#endif

    if (pval)
        *pval = dnew;
    return (dnew != g_value);
}


void
sGrip::show_ghost(int map_x, int map_y, bool erase)
{
    Point_c p1(end1x(), end1y());
    Point_c p2(end2x(), end2y());
    Point_c pmid((p1.x + p2.x)/2, (p1.y + p2.y)/2);

    int dx = map_x - pmid.x;
    int dy = map_y - pmid.y;
    double a = (g_ux*dx + g_uy*dy)/CDphysResolution;
    if (g_ux && g_uy)
        a /= M_SQRT2;

    double scale = gd_scale;
    double snap = gd_snap;
    // Note gd_absolute is not handled.

    double dnew = a*scale;
    if (snap != 0.0)
        dnew = snap*(int)(dnew/snap);
    dnew += g_value;

    double amax = (gd_maxval - g_value)/scale;
    double amin = (gd_minval - g_value)/scale;
    bool rv = false;
    if (amax < amin) {
        double t = amin;
        amin = amax;
        amax = t;
        rv = true;
    }
    if (a > amax) {
        a = amax;
        dnew = rv ? gd_minval : gd_maxval;
    }
    else if (a < amin) {
        a = amin;
        dnew = rv ? gd_maxval : gd_minval;
    }

    // Check against the constraint, if any.
    if (g_constr && !g_constr->checkConstraint(dnew))
        return;

    if (g_ux && g_uy) {
        dx = mmRnd(a*g_ux*CDphysResolution/M_SQRT2);
        dy = mmRnd(a*g_uy*CDphysResolution/M_SQRT2);
    }
    else {
        dx = mmRnd(a*g_ux*CDphysResolution);
        dy = mmRnd(a*g_uy*CDphysResolution);
    }
    p1.x += dx;
    p1.y += dy;
    p2.x += dx;
    p2.y += dy;

    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::CHD);
    while ((wdesc = wgen.next()) != 0) {
        if (Gst()->ShowingGhostInWindow(wdesc)) {
            if (g_cdesc) {
                int xlev = wdesc->Attrib()->expand_level(Physical);
                if (xlev == 0 && !g_cdesc->has_flag(wdesc->DisplFlags()))
                    continue;
            }
            if (p1 == p2) {
                int delta = (int)((DSP_GRIP_MARK_PIXELS+1)/wdesc->Ratio());
                wdesc->ShowLineW(p1.x-delta, p1.y, p1.x, p1.y+delta);
                wdesc->ShowLineW(p1.x, p1.y+delta, p1.x+delta, p1.y);
                wdesc->ShowLineW(p1.x+delta, p1.y, p1.x, p1.y-delta);
                wdesc->ShowLineW(p1.x, p1.y-delta, p1.x-delta, p1.y);
            }
            else
                wdesc->ShowLineW(p1.x, p1.y, p2.x, p2.y);

            char buf[128];
            sprintf(buf, "%s %.5f", gd_param, dnew);
            int x = 4;
            int y = wdesc->ViewportHeight() - 5;
            if (erase) {
                int w = 0, h = 0;
                wdesc->Wdraw()->TextExtent(buf, &w, &h);
                BBox BB(x, y, x+w, y-h);
                wdesc->GhostUpdate(&BB);
            }
            else
                wdesc->Wdraw()->Text(buf, x, y, 0, 1, 1);
        }
    }
}
// End of sGrip functions.


cGripDb::cGripDb()
{
    gdb_grip_tab = 0;
    gdb_unused = 0;
    gdb_idcnt = 0;
}


cGripDb::~cGripDb()
{
    tgen_t<cdelt_t> gen(gdb_grip_tab);
    cdelt_t *cdelt;
    while ((cdelt = gen.next()) != 0) {
        tgen_t<idelt_t> igen(cdelt->table());
        idelt_t *idelt;
        while ((idelt = igen.next()) != 0)
            delete idelt->grip();
        delete cdelt->table();
    }
    delete gdb_grip_tab;
}


// Add the grip to the database.  A unique id number for the grip is
// returned.  The database takes ownership, the passed sGrip should
// not be freed or changed.
//
int
cGripDb::saveGrip(sGrip *grip)
{
    if (!grip)
        return (true);
    if (grip->id() > 0)
        // Grip already saved.
        return (true);
    grip->set_id(++gdb_idcnt);

    if (!gdb_grip_tab)
        gdb_grip_tab = new itable_t<cdelt_t>;
    cdelt_t *cdelt = gdb_grip_tab->find(grip->cdesc());
    if (!cdelt) {
        if (gdb_unused) {
            cdelt = gdb_unused;
            gdb_unused = gdb_unused->tab_next();
        }
        else
            cdelt = (cdelt_t*)gdb_elt_allocator.new_element();
        cdelt->set_key(grip->cdesc());
        cdelt->set_tab_next(0);
        cdelt->set_table(0);
        gdb_grip_tab->link(cdelt);
        gdb_grip_tab = gdb_grip_tab->check_rehash();
    }
    itable_t<idelt_t> *idtab = cdelt->table();
    if (!idtab) {
        idtab = new itable_t<idelt_t>;
        cdelt->set_table(idtab);
    }
    idelt_t *idelt;
    if (gdb_unused) {
        idelt = (idelt_t*)gdb_unused;
        gdb_unused = gdb_unused->tab_next();
    }
    else
        idelt = (idelt_t*)gdb_elt_allocator.new_element();
    idelt->set_key(gdb_idcnt);
    idelt->set_tab_next(0);
    idelt->set_grip(grip);
    idtab->link(idelt);
    idtab = idtab->check_rehash();
    cdelt->set_table(idtab);

    // We don't use the active flag, except that the sGrip destructor
    // calls set_active(false), thus taking care of erasing the
    // markers.
    // 
    grip->set_active(true);

    return (gdb_idcnt);
}


// Delete grips.  If grip_id is less than 1, all of the grips for the
// instance, or the no-instance class if cdesc is null, will be
// deleted.  Otherwise, only the grip with the given id will be
// deleted.
//
bool
cGripDb::deleteGrip(const CDc *cdesc, int grip_id)
{
    if (!gdb_grip_tab)
        return (true);
    if (grip_id <= 0) {
        cdelt_t *cdelt = gdb_grip_tab->remove(cdesc);
        if (cdelt) {
            tgen_t<idelt_t> gen(cdelt->table());
            idelt_t *idelt;
            while ((idelt = gen.next()) != 0)
                delete idelt->grip();
            delete cdelt->table();
            recycle(cdelt);
        }
    }
    else {
        cdelt_t *cdelt = gdb_grip_tab->find(cdesc);
        if (cdelt) {
            idelt_t *idelt = cdelt->table()->remove(grip_id);
            if (idelt) {
                delete idelt->grip();
                recycle(idelt);
            }
        }
    }
    return (true);
}


// Activate or unactivate grips.  If grip_id is less than 1, all of
// the grips for the instance, or the no-instance class if cdesc is
// null, will have activation set.  Otherwise, only the grip with the
// given id will have activation set.  Non-active grips are invisible
// and can't be selected.
//
bool
cGripDb::activateGrip(const CDc *cdesc, int grip_id, bool actv)
{
    if (!gdb_grip_tab)
        return (true);
    if (grip_id <= 0) {
        cdelt_t *cdelt = gdb_grip_tab->find(cdesc);
        if (cdelt) {
            tgen_t<idelt_t> gen(cdelt->table());
            idelt_t *idelt;
            while ((idelt = gen.next()) != 0)
                idelt->grip()->set_active(actv);
        }
    }
    else {
        cdelt_t *cdelt = gdb_grip_tab->find(cdesc);
        if (cdelt) {
            idelt_t *idelt = cdelt->table()->find(grip_id);
            if (idelt)
                idelt->grip()->set_active(actv);
        }
    }
    return (true);
}


// Find and return the sGrip, given the id, and the cdesc.  Note that
// cdesc = 0 is valid.
//
sGrip *
cGripDb::findGrip(const CDc *cdesc, int grip_id)
{
    if (!gdb_grip_tab)
        return (0);
    if (grip_id <= 0)
        return (0);
    cdelt_t *cdelt = gdb_grip_tab->find(cdesc);
    if (!cdelt)
        return (0);
    idelt_t *idelt = cdelt->table()->find(grip_id);
    return (idelt ? idelt->grip() : 0);
}


// Return true if cdesc is in the table.
//
bool
cGripDb::hasCdesc(const CDc *cdesc)
{
    return (gdb_grip_tab && gdb_grip_tab->find(cdesc) != 0);
}


// Save unused elements on a list for reuse.
//
void
cGripDb::recycle(void *elt)
{
    cdelt_t *cdelt = (cdelt_t*)elt;
    cdelt->set_tab_next(gdb_unused);
    gdb_unused = cdelt;
}
// End of cGripDb functions.


sGripGen::sGripGen(const cGripDb *gdb, const CDc *cdesc) :
    gg_cdgen(0), gg_idgen(0)
{
    if (cdesc == GG_ALL_GRIPS)
        gg_cdgen.tinit(gdb->table());
    else if (gdb->table()) {
        gg_celt = gdb->table()->find(cdesc);
        if (gg_celt)
            gg_idgen.tinit(gg_celt->table());
    }
}


sGrip *
sGripGen::next()
{
    idelt_t *idelt = gg_idgen.next();
    if (!idelt) {
        gg_celt = gg_cdgen.next();
        if (gg_celt) {
            gg_idgen.tinit(gg_celt->table());
            idelt = gg_idgen.next();
        }
    }
    if (idelt)
        return (idelt->grip());
    return (0);
}
// End of sGripGen functions.

