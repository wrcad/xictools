
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
#include "ext.h"
#include "ext_extract.h"
#include "ext_nets.h"
#include "ext_pathfinder.h"
#include "sced.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "events.h"
#include "select.h"
#include "promptline.h"
#include "menu.h"
#include "ebtn_menu.h"


//-----------------------------------------------------------------------------

// Command state for group/node selection.
//
namespace {
    namespace ext_gnsel {
        struct SelState : public CmdState
        {
            friend void cExt::selectGroupNode(GRobject);
            friend bool cExt::selectShowNode(int);
            friend void cExt::selectShowPath(bool);
            friend void cExt::selectRedrawPath();

            SelState(const char*, const char*);
            virtual ~SelState();

            void setCaller(GRobject c)  { Caller = c; }

            void message()
                { PL()->ShowPrompt(
        "Click on wires/connections or conductors to show node or group."); }

            void b1down() { cEventHdlr::sel_b1down(); }
            void b1up();
            void b1up_altw();
            void esc();
            bool key(int, const char*, int);

        private:
            void do_it(int, bool);
            void show_group(int, bool);

            GRobject Caller;
            int GroupShown;
            int NodeShown;
        };

        SelState *SelCmd;
    }
}

using namespace ext_gnsel;


namespace {
    int
    exec_idle(void *arg)
    {
        GRobject caller = (GRobject)arg;

        CDs *psdesc = CurCell(Physical);
        if (!psdesc) {
            Menu()->Deselect(caller);
            return (false);
        }

        if (!EX()->associate(psdesc)) {
            PL()->ShowPrompt("Association failed!");
            Menu()->Deselect(caller);
            return (false);
        }
        cGroupDesc *gd = psdesc->groups();
        if (!gd || gd->isempty()) {
            PL()->ShowPrompt("No conductor groups found!");
            Menu()->Deselect(caller);
            return (false);
        }

        Selections.deselectTypes(CurCell(), 0);

        SelCmd = new SelState("GNSEL", "xic:exsel");
        SelCmd->setCaller(caller);
        if (!EV()->PushCallback(SelCmd)) {
            delete SelCmd;
            return (false);
        }

        if (DSP()->ShowingNode() > 0) {
            // A wire net is current highlighted, start with the
            // corresponding group.

            EX()->selectShowNode(DSP()->ShowingNode());
        }
        SelCmd->message();
        return (false);
    }
}


// Enter the group/node selection command mode.
//
void
cExt::selectGroupNode(GRobject caller)
{
    if (!caller)
        return;
    bool state = Menu()->GetStatus(caller);
    if (!state) {
        if (SelCmd)
            SelCmd->esc();
        return;
    }
    if (SelCmd)
        return;

    if (!XM()->CheckCurCell(false, false, Physical)) {
        Menu()->Deselect(caller);
        return;
    }

    EV()->InitCallback();

    // Launch the state in an idle proc, so present command (if any)
    // will exit cleanly.

    dspPkgIf()->RegisterIdleProc(exec_idle, caller);
}


// This is called from the Node Name Mapping editor, so that the
// node/group displayed will track a selection made there.
//
// If the passed node is negative, return false/true if the selection
// mode is inactive/active.
//
bool
cExt::selectShowNode(int node)
{
    if (!SelCmd)
        return (false);
    if (node < 0)
        return (true);

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (false);
    cGroupDesc *gd = cursdp->groups();
    if (gd) {
        int grp = gd->group_of_node(node);
        bool selected = (grp >= 0 && grp == SelCmd->GroupShown);
        if (!selected)
            SelCmd->do_it(grp, selected);
        if (grp < 0) {
            CDcbin cbin(DSP()->CurCellName());
            if (cbin.elec()) {
                const char *nn = SCD()->nodeName(cbin.elec(), node);
                PL()->ShowPromptV("Selected %s %d, electrical node %s.",
                    gd->net_of_group(grp) ? "group" : "virtual group",
                    grp, nn);
            }
            else
                PL()->ShowPromptV("Selected %s %d.",
                    gd->net_of_group(grp) ? "group" : "virtual group",
                    grp);
            return (false);
        }
        return (true);
    }
    return (false);
}


// Set or unset the mode where a recursive path is shown in the physical
// windows when a group is selected.
//
void
cExt::selectShowPath(bool show)
{
    if (!SelCmd || show == isGNShowPath())
        return;

    if (SelCmd->GroupShown >= 0) {
        int gp = SelCmd->GroupShown;
        SelCmd->do_it(gp, true);
        setGNShowPath(show);
        SelCmd->do_it(gp, false);
    }
    else
        setGNShowPath(show);
    EX()->PopUpSelections(0, MODE_UPD);
}


// Redraw the path, if a path is being shown.  This is a callback from
// the widget when the depth changes.
//
void
cExt::selectRedrawPath()
{
    if (!SelCmd || !isGNShowPath())
        return;

    int gp = SelCmd->GroupShown;
    if (gp >= 0) {
        SelCmd->do_it(gp, true);
        SelCmd->do_it(gp, false);
    }
}


namespace {
    // Logic for group selection.  This allows clicking at the same
    // location to cycle through groups that are selectable at the
    // location.

    struct sGrpSel
    {
        struct grp_list
        {
            grp_list(int g, grp_list *n)
                {
                    next = n;\
                    group = g;
                }

            static void destroy(grp_list *gl)
                {
                    while (gl) {
                        grp_list *x = gl;
                        gl = gl->next;
                        delete x;
                    }
                }

            grp_list *next;
            int group;
        };

        sGrpSel()
            {
                gs_groups = 0;
                gs_sdesc = 0;
            }

        int get_group(int, int);

    private:
        BBox gs_BB;             // last click vacinity
        grp_list *gs_groups;    // list of groups in vacinity
        const CDs *gs_sdesc;    // current cell
    };

    sGrpSel GrpSel;


    int
    sGrpSel::get_group(int x, int y)
    {
        CDs *cursdp = CurCell(Physical);
        if (!cursdp)
            return (-1);
        cGroupDesc *gd = cursdp->groups();
        if (!gd)
            return (-1);
        if (cursdp != gs_sdesc || !gs_BB.intersect(x, y, false)) {
            gs_sdesc = cursdp;
            grp_list::destroy(gs_groups);
            gs_groups = 0;
            WindowDesc *wd = EV()->CurrentWin();
            if (!wd)
                wd = DSP()->MainWdesc();
            int delta = mmRnd(2.0/wd->Ratio());
            gs_BB.left = gs_BB.right = x;
            gs_BB.bottom = gs_BB.top = y;
            gs_BB.bloat(delta);

            // Make a list of the groups found at the location.
            CDextLgen lgen(CDL_CONDUCTOR, Selections.layerSearchUp() ?
                CDextLgen::BotToTop: CDextLgen::TopToBot);
            CDl *ld;
            while ((ld = lgen.next()) != 0) {
                int grp = gd->find_group_at_location(ld, x, y, false);
                if (grp >= 0) {
                    grp_list *l = gs_groups;
                    for ( ; l && l->group != grp; l = l->next) ;
                    if (l)
                        continue;
                    gs_groups = new grp_list(grp, gs_groups);
                }
            }
        }
        else if (gs_groups && gs_groups->next) {
            grp_list *l = gs_groups;
            gs_groups = l->next;
            l->next = 0;
            grp_list *n = gs_groups;
            while (n->next)
                n = n->next;
            n->next = l;
        }
        if (gs_groups)
            return (gs_groups->group);
        return (-1);
    }
}


// Exported button-up handler for electrical and physical net selection.
//
int
cExt::netSelB1Up()
{
    BBox AOI;
    if (DSP()->CurMode() == Electrical) {

        CDol *selections;
        bool btmp = ext_extraction_select;
        ext_extraction_select = true;
        bool bret = cEventHdlr::sel_b1up(&AOI, 0, &selections);
        ext_extraction_select = btmp;
        if (!bret)
            return (-1);
        CDol::destroy(selections);

        // hyPoint won't detect an object unless the AOI intersects
        // it, which for a point select won't occur unless it happens
        // to be on grid.  Recall that wires generally have zero width
        // and points have zero width and height.  We can force the
        // locations to the grid, or better yet just expand the point
        // area which will work if the grid is set to something wacky
        // for some reason.
        //
        // EV()->CurrentWin()->Snap(&AOI.left, &AOI.bottom);
        // EV()->CurrentWin()->Snap(&AOI.right, &AOI.top);
        //
        if (AOI.left == AOI.right && AOI.bottom == AOI.top) {
            int delta = 1 +
                (int)(DSP()->PixelDelta()/EV()->CurrentWin()->Ratio());
            AOI.bloat(delta);
        }

        CDs *cursde = CurCell(Electrical);
        if (cursde) {
            cTfmStack stk;
            hyEnt *ent = cursde->hyPoint(&stk, &AOI, HY_NODE);
            if (ent) {
                int n = ent->nodenum();
                delete ent;
                return (n);
            }
        }
        return (-1);
    }

    cEventHdlr::sel_b1up(&AOI, 0, B1UP_NOSEL);
    int x, y;
    EV()->Cursor().get_raw(&x, &y);
    return (GrpSel.get_group(x, y));
}


// Exported alternate window button-up handler for electrical and
// physical net selection.
//
int
cExt::netSelB1Up_altw()
{
    BBox AOI;
    EV()->Cursor().get_alt_down(&AOI.left, &AOI.bottom);
    EV()->Cursor().get_alt_up(&AOI.right, &AOI.top);
    AOI.fix();

    if (EV()->CurrentWin()->Mode() == Electrical) {
        CDs *cursde = EV()->CurrentWin()->CurCellDesc(Electrical);

        // hyPoint won't detect an object unless the AOI intersects
        // it, which for a point select won't occur unless it happens
        // to be on grid.  Recall that wires generally have zero width
        // and points have zero width and height.  We can force the
        // locations to the grid, or better yet just expand the point
        // area which will work if the grid is set to something wacky
        // for some reason.
        //
        // EV()->CurrentWin()->Snap(&AOI.left, &AOI.bottom);
        // EV()->CurrentWin()->Snap(&AOI.right, &AOI.top);
        //
        if (AOI.left == AOI.right && AOI.bottom == AOI.top) {
            int delta = 1 +
                (int)(DSP()->PixelDelta()/EV()->CurrentWin()->Ratio());
            AOI.bloat(delta);
        }
        if (cursde) {
            cTfmStack stk;
            hyEnt *ent = cursde->hyPoint(&stk, &AOI, HY_NODE);
            if (ent) {
                int n = ent->nodenum();
                delete ent;
                return (n);
            }
        }
    }
    else {
        CDs *cursdp = CurCell(Physical);
        if (!cursdp)
            return (-1);
        cGroupDesc *gd = cursdp->groups();

        // First check the phonies.
        CDo *odesc = gd ? gd->intersect_phony(&AOI) : 0;
        if (odesc) {
            int grp = odesc->group();
            if (grp >= 0)
                return (grp);
        }
        const char *types = "bpw";
        CDol *list = Selections.selectItems(cursdp, types, &AOI, PSELpoint);
        list = Selections.filter(cursdp, list, &AOI, false);
        for (CDol *s = list; s; s = s->next) {
            odesc = s->odesc;
            if (odesc->type() == CDLABEL)
                continue;
            if (!odesc->is_normal())
                continue;
            if (!odesc->ldesc()->isConductor())
                continue;
            if (odesc->ldesc()->isGroundPlane() &&
                    odesc->ldesc()->isDarkField())
                continue;
            int grp = odesc->group();
            if (grp >= 0) {
                CDol::destroy(list);
                return (grp);
            }
        }
        CDol::destroy(list);
    }
    return (-1);
}
// End of cExtfunctions.


SelState::SelState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    GroupShown = -1;
    NodeShown = -1;
    EX()->setGNShowPath(false);
}


SelState::~SelState()
{
    SelCmd = 0;
}


void
SelState::b1up()
{
    int group = -1;
    if (DSP()->CurMode() == Electrical) {
        int node = EX()->netSelB1Up();
        if (node >= 0) {
            CDs *cursdp = CurCell(Physical);
            if (!cursdp)
                return;
            cGroupDesc *gd = cursdp->groups();
            if (gd)
                group = gd->group_of_node(node);
        }
    }
    else {
        int grp = EX()->netSelB1Up();
        if (grp >= 0)
            group = grp;
    }
    bool selected = (group >= 0 && group == GroupShown);
    do_it(group, selected);
}


void
SelState::b1up_altw()
{
    if (EV()->CurrentWin()->CurCellName() != DSP()->MainWdesc()->CurCellName())
        return;

    int group = -1;
    if (DSP()->CurMode() == Physical) {
        int node = EX()->netSelB1Up_altw();
        if (node >= 0) {
            CDs *cursdp = CurCell(Physical);
            if (!cursdp)
                return;
            cGroupDesc *gd = cursdp->groups();
            if (gd)
                group = gd->group_of_node(node);
        }
    }
    else {
        int grp = EX()->netSelB1Up_altw();
        if (grp >= 0)
            group = grp;
    }
    bool selected = (group >= 0 && group == GroupShown);
    do_it(group, selected);
}


void
SelState::esc()
{
    // Keep the node displayed if the Node Mapping Editor is present.
    if (!SCD()->PopUpNodeMap(0, MODE_UPD, NodeShown))
        DSP()->ShowNode(ERASE, NodeShown);

    cEventHdlr::sel_esc();
    pathfinder *pf = EX()->pathFinder(cExt::PFget);
    if (pf)
        pf->show_path(0, ERASE);
    EX()->pathFinder(cExt::PFclear);
    EX()->setGNShowPath(false);
    EX()->PopUpSelections(0, MODE_UPD);
    Menu()->Deselect(Caller);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    delete this;
}


namespace {
    bool isquote(int c) { return (c == '"' || c == '\''); }
}

// Handle entering of a group number from the keyboard.
//
bool
SelState::key(int code, const char *text, int)
{
    char tbuf[32];
    if (code == RETURN_KEY) {
        PL()->GetTextBuf(EV()->CurrentWin(), tbuf);

        // Node names are entered following a quote char, to avoid
        // accelerators.  A trailing quote char is optional.
        char *buf = tbuf;
        if (isquote(*buf)) {
            buf++;
            char *t = buf + strlen(buf) - 1;
            if (t > buf && isquote(*t))
                *t-- = 0;
        }

        CDs *cursdp = CurCell(Physical);
        if (!cursdp)
            return (false);
        cGroupDesc *gd = cursdp->groups();
        if (!gd)
            return (false);

        if (EV()->CurrentWin()->Mode() == Physical) {
            // In a physical window, the entered value is a group
            // number.

            int g;
            if (sscanf(buf, "%d", &g) != 1) {
                PL()->ShowPrompt("Text input is not a group number.");
                PL()->SetTextBuf(EV()->CurrentWin(), "");
                return (true);
            }
            if (g < 0 || g >= gd->nextindex()) {
                PL()->ShowPromptV("%d is not a valid group number.", g);
                PL()->SetTextBuf(EV()->CurrentWin(), "");
                return (true);
            }
            bool selected = (g >= 0 && g == GroupShown);
            do_it(g, selected);
        }
        else {
            // In an electrical window, the entered value is a node
            // name or number.

            int n = SCD()->findNode(CurCell(Electrical, true), buf);
            if (n < 0) {
                PL()->ShowPrompt("Text input is not a node name or number.");
                PL()->SetTextBuf(EV()->CurrentWin(), "");
                return (true);
            }

            int grp = gd->group_of_node(n);
            if (grp < 0) {
                PL()->ShowPromptV("Node %s has no corresponding group.", buf);
                PL()->SetTextBuf(EV()->CurrentWin(), "");
                return (true);
            }
            bool selected = (grp >= 0 && grp == GroupShown);
            do_it(grp, selected);
        }
        PL()->SetTextBuf(EV()->CurrentWin(), "");
        return (true);
    }
    else if (text && *text) {

        // If the first char in the keypress buffer is a quote char,
        // lock out accelerators.
        PL()->GetTextBuf(EV()->CurrentWin(), tbuf);
        if (isquote(tbuf[0]))
            return (false);

        if (*text == 'p') {
            EX()->selectShowPath(!EX()->isGNShowPath());
            EX()->PopUpSelections(0, MODE_UPD);
            return (true);
        }
        if (*text == 'h') {
            EX()->setBlinkSelections(!EX()->isBlinkSelections());
            EX()->PopUpSelections(0, MODE_UPD);
            pathfinder *pf = EX()->pathFinder(cExt::PFget);
            if (pf) {
                pf->show_path(0, ERASE);
                pf->show_path(0, DISPLAY);
            }
            return (true);
        }
        if (*text == '+') {
            if (code == NUPLUS_KEY)
                return (false);
            if (EX()->pathDepth() < CDMAXCALLDEPTH) {
                EX()->setPathDepth(EX()->pathDepth() + 1);
                EX()->PopUpSelections(0, MODE_UPD);
            }
            return (true);
        }
        if (*text == '-') {
            if (code == NUMINUS_KEY)
                return (false);
            if (EX()->pathDepth() > 0) {
                EX()->setPathDepth(EX()->pathDepth() - 1);
                EX()->PopUpSelections(0, MODE_UPD);
            }
            return (true);
        }
        if (*text == 'a') {
            EX()->setPathDepth(CDMAXCALLDEPTH);
            EX()->PopUpSelections(0, MODE_UPD);
            return (true);
        }
        if (*text == 'n') {
            EX()->setPathDepth(0);
            EX()->PopUpSelections(0, MODE_UPD);
            return (true);
        }
        if (*text == 's') {
            pathfinder *pf = EX()->pathFinder(cExt::PFget);
            if (pf) {
                char buf[256];
                sprintf(buf, "%s_grp_%s", Tstring(DSP()->CurCellName()),
                    pf->pathname());

                char *in = PL()->EditPrompt("Native cell file name? ", buf);
                if (in && *in) {
                    // Maybe write out the vias, too.
                    const char *vstr = CDvdb()->getVariable(VA_PathFileVias);
                    if (EX()->saveCurrentPathToFile(in, (vstr != 0)))
                        PL()->ShowPrompt("Path saved to native cell file.");
                    else
                        PL()->ShowPrompt("Operation failed.");
                }
            }
            else
                PL()->ShowPrompt("No current path!");
            return (true);
        }
        if (*text == 't') {
            pathfinder *pf = EX()->pathFinder(cExt::PFget);
            if (pf) {
                pf->show_path(0, ERASE);
                pf->atomize_path();
                pf->show_path(0, DISPLAY);
            }
        }
    }
    return (false);
}


// Process the request
//
void
SelState::do_it(int group, bool selected)
{
    if (group >= 0) {
        if (!selected) {
            CDcbin cbin(DSP()->CurCellName());
            cGroupDesc *gd = cbin.groups();
            if (gd) {
                sGroup *g = gd->group_for(group);
                if (!g)
                    return;
                sLstr lstr;
                if (!g->net())
                    lstr.add("virtual");
                if (g->global()) {
                    if (lstr.string())
                        lstr.add_c(' ');
                    lstr.add("global");
                }
                if (lstr.string())
                    lstr.add_c(' ');
                lstr.add("group");
                lstr.add_c(' ');
                lstr.add_i(group);
                if (g->netname()) {
                    lstr.add(" (");
                    lstr.add(Tstring(g->netname()));
                    lstr.add_c(')');
                }
                if (cbin.elec()) {
                    const char *nn = SCD()->nodeName(cbin.elec(),
                        gd->node_of_group(group));
                    if (nn) {
                        lstr.add(", electrical node ");
                        lstr.add(nn);
                    }
                }
                PL()->ShowPrompt(lstr.string());
                if (GroupShown >= 0)
                    show_group(GroupShown, true);
                GroupShown = group;
            }
        }
        else {
            PL()->ErasePrompt();
            GroupShown = -1;
            NodeShown = -1;
        }
        show_group(group, selected);
    }
    else {
        PL()->ErasePrompt();
        show_group(group, (GroupShown >= 0));
        GroupShown = -1;
        NodeShown = -1;
    }
}


// Select or deselect all objects in the group.
//
void
SelState::show_group(int group, bool isselected)
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    cGroupDesc *gd = cursdp->groups();
    if (!gd)
        return;
    // highlight terminals in electrical windows
    if (isselected) {
        if (!SCD()->PopUpNodeMap(0, MODE_UPD)) {
            if (group < 0 && NodeShown > 0)
                DSP()->ShowNode(ERASE, NodeShown);
            else if (group > 0)
                DSP()->ShowNode(ERASE, gd->node_of_group(group));
        }
    }
    else {
        if (group < 0)
            return;
        NodeShown = gd->node_of_group(group);
        if (!SCD()->PopUpNodeMap(0, MODE_UPD, NodeShown))
            DSP()->ShowNode(DISPLAY, NodeShown);
    }

    int depth = 0;
    if (EX()->isGNShowPath())
        depth = EX()->pathDepth();

    if (!isselected) {
        sGroupObjs *go = gd->net_of_group(group);
        if (go && go->objlist()) {
            pathfinder *pf = EX()->pathFinder(cExt::PFinit);
            pf->set_depth(depth);
            pf->find_path(go->objlist()->odesc);
            pf->show_path(0, DISPLAY);
        }
    }
    else {
        pathfinder *pf = EX()->pathFinder(cExt::PFget);
        if (pf)
            pf->show_path(0, ERASE);
        EX()->pathFinder(cExt::PFclear);
    }
}

