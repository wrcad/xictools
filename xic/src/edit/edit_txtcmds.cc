
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
#include "fio.h"
#include "edit.h"
#include "undolist.h"
#include "cd_celldb.h"
#include "geo_ylist.h"
#include "geo_intdb.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "promptline.h"
#include "events.h"
#include "select.h"
#include "layertab.h"
#include "array.h"
#include "ghost.h"
#include "menu.h"
#include "tech.h"
#include "errorlog.h"
#include "filestat.h"
#include "grfont.h"


//-----------------------------------------------------------------------------
// 'Bang' commands

namespace {
    namespace edit_bangcmds {

        // !! dummy
        void bangbangcmd(const char*);

        // Layout Editing
        void array(const char*);
        void layer(const char*);
        void mo(const char*);
        void co(const char*);
        void spin(const char*);
        void rename(const char*);
        void svq(const char*);
        void rcq(const char*);
        void box2poly(const char*);
        void path2poly(const char*);
        void poly2path(const char*);
        void bloat(const char*);
        void join(const char*);
        void split(const char*);
        void jw(const char*);
        void manh(const char*);
        void polyfix(const char*);
        void polyrev(const char*);
        void noacute(const char*);
        void togrid(const char*);
        void tospot(const char*);
        void origin(const char*);
        void import(const char*);

        // debugging
        void testfunc(const char*);
    }
}


void
cEdit::setupBangCmds()
{
    // !! dummy
    XM()->RegisterBangCmd(BANG_BANG_NAME, &edit_bangcmds::bangbangcmd);

    // Layout Editing
    XM()->RegisterBangCmd("array", &edit_bangcmds::array);
    XM()->RegisterBangCmd("layer", &edit_bangcmds::layer);
    XM()->RegisterBangCmd("mo", &edit_bangcmds::mo);
    XM()->RegisterBangCmd("co", &edit_bangcmds::co);
    XM()->RegisterBangCmd("spin", &edit_bangcmds::spin);
    XM()->RegisterBangCmd("rename", &edit_bangcmds::rename);
    XM()->RegisterBangCmd("svq", &edit_bangcmds::svq);
    XM()->RegisterBangCmd("rcq", &edit_bangcmds::rcq);
    XM()->RegisterBangCmd("box2poly", &edit_bangcmds::box2poly);
    XM()->RegisterBangCmd("path2poly", &edit_bangcmds::path2poly);
    XM()->RegisterBangCmd("poly2path", &edit_bangcmds::poly2path);
    XM()->RegisterBangCmd("bloat", &edit_bangcmds::bloat);
    XM()->RegisterBangCmd("join", &edit_bangcmds::join);
    XM()->RegisterBangCmd("split", &edit_bangcmds::split);
    XM()->RegisterBangCmd("jw", &edit_bangcmds::jw);
    XM()->RegisterBangCmd("manh", &edit_bangcmds::manh);
    XM()->RegisterBangCmd("polyfix", &edit_bangcmds::polyfix);
    XM()->RegisterBangCmd("polyrev", &edit_bangcmds::polyrev);
    XM()->RegisterBangCmd("noacute", &edit_bangcmds::noacute);
    XM()->RegisterBangCmd("togrid", &edit_bangcmds::togrid);
    XM()->RegisterBangCmd("tospot", &edit_bangcmds::tospot);
    XM()->RegisterBangCmd("origin", &edit_bangcmds::origin);
    XM()->RegisterBangCmd("import", &edit_bangcmds::import);

    // debugging
    XM()->RegisterBangCmd("testfunc", &edit_bangcmds::testfunc);
}

namespace {
    const char *phys_msg = "The %s command is for physical mode only!";
}


//-----------------------------------------------------------------------------
// !! dummy

// Dummy function, for the !! script text execution feature.  This will
// override the default function set in main, this takes care of geometry
// modification cleanup.
//
void
edit_bangcmds::bangbangcmd(const char *s)
{
    // Process the passed text as a script line.
    if (s && *s) {
        PL()->ErasePrompt();
        // execute line as if script
        char *tbuf = lstring::copy(s); // scInterpret takes non-const
        const char *t = tbuf;
        CDs *cursd = CurCell();
        Ulist()->ListCheck("scrcmd", cursd, false);
        SI()->Interpret(0, 0, &t, 0);
        Ulist()->CommitChanges();
        delete [] tbuf;
    }
}


//-----------------------------------------------------------------------------
// Layout Editing


namespace {
    namespace ed_bang {
        // Command desc for !array -d
        //
        struct ArrState : public CmdState
        {
            ArrState(const char*, const char*);
            virtual ~ArrState();

            void setup(CDs *sd, CDc *cd)
                {
                    Sdesc = sd;
                    Cdesc = cd;
                }

            void b1down() { cEventHdlr::sel_b1down(); }
            void b1up();
            void esc();
            void undo() { cEventHdlr::sel_undo(); }
            void redo() { cEventHdlr::sel_redo(); }
            void message() {  PL()->ShowPrompt(msg1); }

        private:
            bool do_arrdel(const BBox*);

            CDs *Sdesc;
            CDc *Cdesc;

            static const char *msg1;
        };

        ArrState *ArrCmd;
    }
}

using namespace ed_bang;

const char *ArrState::msg1 = "Click or drag to select array cells to delete.";


void
edit_bangcmds::array(const char *s)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPromptV(phys_msg, "!array");
        return;
    }
    CDs *sdesc = CurCell(Physical);
    if (!sdesc) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    if (!s || *s != '-' || !s[1] || !strchr("uUdDrR", s[1])) {
        PL()->ShowPrompt("usage: !array -u | -d | -r cf_args");
        return;
    }
    if (s[1] == 'u' || s[1] == 'U') {
        // "unarray" mode, all selected instance arrays become individual
        // placements.
        //
        // !array -u

        Ulist()->ListCheck("unarry", sdesc, true);
        bool found = false;
        sArrayManip amp(sdesc, 0);
        sSelGen sg(Selections, sdesc, "c");
        CDo *od;
        while ((od = sg.next()) != 0) {
            CDap ap(OCALL(od));
            if (ap.nx == 1 && ap.ny == 1)
                continue;

            amp.set_instance(OCALL(od));
            if (!amp.unarray()) {
                PL()->ShowPromptV("Error: %s", Errs()->get_error());
                return;
            }
            found = true;
        }
        if (found) {
            Ulist()->CommitChanges(true);
            PL()->ShowPrompt(
            "Selected instance arrays converted to individual placements.");
        }
        else
            PL()->ShowPrompt("No selected instance arrays found.");
    }
    else if (s[1] == 'd' || s[1] == 'D') {
        // "delete" mode, a rectangular portion of the first selected array
        // is deleted, remaining array elements are re-arrayed or become
        // individual placements.
        //
        // !array -d  [nx1[-nx2],[ny1[-ny2]]

        CDc *cdesc = (CDc*)Selections.firstObject(sdesc, "c", true, true);
        if (!cdesc) {
            PL()->ShowPrompt("No array selected, select one first.");
            return;
        }
        lstring::advtok(&s);  // skip "-d"

        int nx1 = -1;
        int nx2 = -1;
        int ny1 = -1;
        int ny2 = -1;

        const char *tstart = s;
        char *tok = lstring::gettok(&s, "-,");
        if (tok) {
            bool range = false;
            for (const char *tt = tstart; tt != s; tt++) {
                if (*tt == '-') {
                    range = true;
                    break;
                }
            }
            sscanf(tok, "%d", &nx1);
            delete [] tok;
            if (nx1 < 0) {
                PL()->ShowPrompt("Error for NX1, bad value or syntax.");
                return;
            }
            if (range) {
                tok = lstring::gettok(&s, ",");
                if (tok) {
                    sscanf(tok, "%d", &nx2);
                    delete [] tok;
                }
                if (nx2 < 0) {
                    PL()->ShowPrompt("Error for NX2, bad value or syntax.");
                    return;
                }
            }
            else
                nx2 = nx1;

            tstart = s;
            tok = lstring::gettok(&s, "-");
            if (tok) {
                range = false;
                for (const char *tt = tstart; tt != s; tt++) {
                    if (*tt == '-') {
                        range = true;
                        break;
                    }
                }
                sscanf(tok, "%d", &ny1);
                delete [] tok;
            }
            if (ny1 < 0) {
                PL()->ShowPrompt("Error for NY1, bad value or syntax.");
                return;
            }
            if (range) {
                tok = lstring::gettok(&s);
                if (tok) {
                    sscanf(tok, "%d", &ny2);
                    delete [] tok;
                }
                if (ny2 < 0) {
                    PL()->ShowPrompt("Error for NY2, bad value or syntax.");
                    return;
                }
            }
            else
                ny2 = ny1;
        }
        else {
            // get coords from mouse ops.
            if (ArrCmd) {
                ArrCmd->esc();
                return;
            }
            ArrCmd = new ArrState("delarry", "!array");
            ArrCmd->setup(sdesc, cdesc);
            if (!EV()->PushCallback(ArrCmd)) {
                delete ArrCmd;
                return;
            }
            ArrCmd->message();
            return;
        }

        Ulist()->ListCheck("delarry", sdesc, true);
        const char *cname = Tstring(cdesc->cellname());
        sArrayManip amp(sdesc, cdesc);
        if (!amp.delete_elements(nx1, nx2, ny1, ny2)) {
            PL()->ShowPromptV("Error: %s", Errs()->get_error());
            return;
        }
        Ulist()->CommitChanges(true);
        PL()->ShowPromptV("Done removing elements from array of %s.", cname);
    }
    else if (s[1] == 'r' || s[1] == 'R') {
        // "reconfigure" mode, the array parameters of the first selected
        // instance can be modified.  Individual instances can become
        // arrays and vice-versa.
        //
        // !array -r [nx[+]=xxx ny[+]=xxx dx[+]=xxx dy[+]=xxx]

        CDc *cdesc = (CDc*)Selections.firstObject(sdesc, "c", true);
        if (!cdesc) {
            PL()->ShowPrompt(
                "No instance or array selected, select one first.");
            return;
        }
        lstring::advtok(&s);  // skip "-r"

        CDap ap(cdesc);

        // When expanding dimensions from 1, the default spacing is the
        // cell size rather than 0.
        int dxoff = 0, dyoff = 0;
        if (ap.nx == 1 && ap.dx == 0) {
            CDs *sd = cdesc->masterCell();
            if (sd) {
                const BBox *sBB = sd->BB();
                dxoff = sBB->width();
                if (dxoff < 0)
                    dxoff = 0;
            }
        }
        if (ap.ny == 1 && ap.dy == 0) {
            CDs *sd = cdesc->masterCell();
            if (sd) {
                const BBox *sBB = sd->BB();
                dyoff = sBB->height();
                if (dyoff < 0)
                    dyoff = 0;
            }
        }

        bool found_nx = false;
        bool found_ny = false;
        bool found_dx = false;
        bool found_dy = false;
        for (;;) {
            const char *msg = "Error parsing %s, bad or no entry.";
            const char *tstart = s;
            char *tok = lstring::gettok(&s, "=+");
            if (!tok)
                break;

            // Look for a trailing '+=', which means that the new
            // value is added to the present value.
            bool adding = false;
            for (const char *tt = tstart; tt != s; tt++) {
                if (tt[0] == '+' && tt[1] == '=') {
                    adding = true;
                    break;
                }
            }

            if (lstring::cieq(tok, "nx")) {
                delete [] tok;
                tok = lstring::gettok(&s);
                if (tok) {
                    int n;
                    if (sscanf(tok, "%d", &n) == 1) {
                        if (adding) {
                            if ((int)ap.nx + n >= 1) {
                                ap.nx = (int)ap.nx + n;
                                found_nx = true;
                            }
                        }
                        else if (n >= 1) {
                            ap.nx = n;
                            found_nx = true;
                        }
                    }
                    delete [] tok;
                }
                if (!found_nx) {
                    PL()->ShowPromptV(msg, "NX");
                    return;
                }
            }
            else if (lstring::cieq(tok, "ny")) {
                delete [] tok;
                tok = lstring::gettok(&s);
                if (tok) {
                    int n;
                    if (sscanf(tok, "%d", &n) == 1) {
                        if (adding) {
                            if ((int)ap.ny + n >= 1) {
                                ap.ny = (int)ap.ny + n;
                                found_ny = true;
                            }
                        }
                        else if (n >= 1) {
                            ap.ny = n;
                            found_ny = true;
                        }
                    }
                    delete [] tok;
                }
                if (!found_ny) {
                    PL()->ShowPromptV(msg, "NY");
                    return;
                }
            }
            else if (lstring::cieq(tok, "dx")) {
                delete [] tok;
                tok = lstring::gettok(&s);
                if (tok) {
                    double d;
                    if (sscanf(tok, "%lf", &d) == 1) {
                        if (adding)
                            ap.dx += INTERNAL_UNITS(d);
                        else {
                            ap.dx = INTERNAL_UNITS(d);
                            dxoff = 0;
                        }
                        found_dx = true;
                    }
                    delete [] tok;
                }
                if (!found_dx) {
                    PL()->ShowPromptV(msg, "DX");
                    return;
                }
            }
            else if (lstring::cieq(tok, "dy")) {
                delete [] tok;
                tok = lstring::gettok(&s);
                if (tok) {
                    double d;
                    if (sscanf(tok, "%lf", &d) == 1) {
                        if (adding)
                            ap.dy += INTERNAL_UNITS(d);
                        else {
                            ap.dy = INTERNAL_UNITS(d);
                            dyoff = 0;
                        }
                        found_dy = true;
                    }
                    delete [] tok;
                }
                if (!found_dy) {
                    PL()->ShowPromptV(msg, "DY");
                    return;
                }
            }
            else {
                PL()->ShowPromptV("Parse error, unknown keyword \"%s\".",
                    tok);
                delete [] tok;
                return;
            }
        }
        if (!(found_nx || found_ny || found_dx || found_dy)) {
            PL()->ShowPrompt("Done (no change).");
            return;
        }
        if (dxoff != 0 && ap.nx != 1)
            ap.dx += dxoff;
        if (dyoff != 0 && ap.ny != 1)
            ap.dy += dyoff;

        Ulist()->ListCheck("cfgarry", sdesc, true);
        const char *cname = Tstring(cdesc->cellname());
        sArrayManip amp(sdesc, cdesc);
        if (!amp.reconfigure(ap.nx, ap.dx, ap.ny, ap.dy)) {
            PL()->ShowPromptV("Error: %s", Errs()->get_error());
            return;
        }
        Ulist()->CommitChanges(true);
        PL()->ShowPromptV("Done reconfiguring instance of %s.", cname);
    }
}


ArrState::ArrState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Sdesc = 0;
    Cdesc = 0;
}


ArrState::~ArrState()
{
    ArrCmd = 0;
}


// Button 1 release handler.
//
void
ArrState::b1up()
{
    BBox AOI;
    if (cEventHdlr::sel_b1up(&AOI, 0, B1UP_NOSEL))
        do_arrdel(&AOI);
}


// Esc entered, clean up and abort.
//
void
ArrState::esc()
{
    cEventHdlr::sel_esc();
    EV()->PopCallback(this);
    delete this;
}


bool
ArrState::do_arrdel(const BBox *AOI)
{
    BBox BB(*AOI);

    cTfmStack stk;
    stk.TPush();
    stk.TApplyTransform(Cdesc);
    stk.TInverse();
    stk.TInversePoint(&BB.left, &BB.bottom);
    stk.TInversePoint(&BB.right, &BB.top);
    stk.TPop();
    BB.fix();

    CDs *sd = Cdesc->masterCell();
    if (!sd)
        return (false);
    const BBox *sBB = sd->BB();

    CDap ap(Cdesc);

    int nx1 = ap.nx;
    int nx2 = -1;
    int ny1 = ap.ny;
    int ny2 = -1;

    int sdx = sBB->width();
    int sdy = sBB->height();
    if (sdx <= 0 || sdy <= 0)
        return (false);

    for (int i = 0; i < (int)ap.ny; i++) {
        for (int j = 0; j < (int)ap.nx; j++) {
            BBox tBB;
            if (ap.dx >= 0) {
                tBB.left = sBB->left + j*ap.dx;
                tBB.right = tBB.left + sdx;
            }
            else {
                tBB.right = sBB->right + j*ap.dx;
                tBB.left = tBB.right - sdx;
            }
            if (ap.dy >= 0) {
                tBB.bottom = sBB->bottom + i*ap.dy;
                tBB.top = tBB.bottom + sdy;
            }
            else {
                tBB.top = sBB->top + i*ap.dy;
                tBB.bottom = tBB.top - sdy;
            }
            if (tBB.intersect(&BB, false)) {
                if (j < nx1)
                    nx1 = j;
                if (j > nx2)
                    nx2 = j;
                if (i < ny1)
                    ny1 = i;
                if (i > ny2)
                    ny2 = i;
            }
        }
    }

    if (nx1 < (int)ap.nx && ny1 < (int)ap.ny) {
        Ulist()->ListCheck("delarry", Sdesc, true);
        const char *cname = Tstring(Cdesc->cellname());
        sArrayManip amp(Sdesc, Cdesc);
        if (!amp.delete_elements(nx1, nx2, ny1, ny2)) {
            PL()->ShowPromptV("Error: %s", Errs()->get_error());
            return (true);
        }
        Ulist()->CommitChanges(true);
        PL()->ShowPromptV("Done removing elements from array of %s.", cname);

        esc();
        return (true);
    }
    return (false);
}
// End of !array command.


void
edit_bangcmds::layer(const char *s)
{
    if (!DSP()->CurCellName()) {
        PL()->ErasePrompt();
        return;
    }
    CDs *cursd = CurCell();
    if (!cursd) {
        PL()->ErasePrompt();
        return;
    }

    int depth, flags;
    if (!ED()->parseNewLayerSpec(&s, &depth, &flags)) {
        PL()->ShowPrompt(Errs()->get_error());
        return;
    }

    if (!*s || (!isalnum(*s) && *s != '$')) {
        PL()->ShowPrompt(
        "usage:  !layer [split|splitv|join] [-d depth] [-r] [-c] [-m] [-f] "
        "layername [=] [expression]");
        return;
    }

    if (flags & CLnoUndo) {
        char *in = PL()->EditPrompt(
            "Fast mode (-f) is NOT UNDOABLE.  Continue? ", "n");
        in = lstring::strip_space(in);
        if (!in) {
            PL()->ErasePrompt();
            return;
        }
        if (*in != 'y' && *in != 'Y') {
            PL()->ErasePrompt();
            return;
        }
    }

    ED()->createLayerCmd(s, depth, flags);
}


void
edit_bangcmds::mo(const char *s)
{
    if (!Selections.hasTypes(CurCell(), 0)) {
        PL()->ShowPrompt("No objects selected to move.");
        return;
    }
    double dx, dy = 0.0;
    char buf[256];
    int i = sscanf(s, "%lf %lf %s", &dx, &dy, buf);
    if (i < 1) {
        PL()->ShowPrompt(
            "Usage:  !mo dx [dy [layer_name]]  (dx,dy in microns)");
        return;
    }
    int idx = INTERNAL_UNITS(dx);
    int idy = INTERNAL_UNITS(dy);
    EV()->InitCallback();
    PL()->ErasePrompt();
    CDl *ldold = 0, *ldnew = 0;
    if (i == 3) {
        CDl *ld = CDldb()->findLayer(buf, DSP()->CurMode());
        if (!ld) {
            PL()->ShowPromptV("Layer not found: %s", buf);
            return;
        }
        if (ld != LT()->CurLayer()) {
            if (ED()->getLchgMode() == LCHGcur) {
                // Map old current to new.
                ldnew = ld;
                ldold = LT()->CurLayer();
            }
            else if (ED()->getLchgMode() == LCHGall) {
                // Map all layers to new.
                ldnew = ld;
                ldold = 0;
            }
        }
    }
    ED()->moveObjects(0, 0, idx, idy, ldold, ldnew);
}


void
edit_bangcmds::co(const char *s)
{
    if (!Selections.hasTypes(CurCell(), 0)) {
        PL()->ShowPrompt("No objects selected to copy.");
        return;
    }
    double dx, dy = 0.0;

    char *tok = lstring::gettok(&s);
    if (!tok || sscanf(tok, "%lf", &dx) != 1) {
        PL()->ShowPrompt(
    "Usage:  !co dx [dy [[-l] layer_name] [[-r] repcnt]]  (dx,dy in microns)");
        delete [] tok;
        return;
    }
    delete [] tok;

    bool found_dy = false;
    tok = lstring::gettok(&s);
    if (tok) {
        if (sscanf(tok, "%lf", &dy) != 1) {
            PL()->ShowPrompt("Bad value for dy.");
            return;
        }
        delete [] tok;
        found_dy = true;
    }
    int idx = INTERNAL_UNITS(dx);
    int idy = INTERNAL_UNITS(dy);
    EV()->InitCallback();
    PL()->ErasePrompt();
    CDl *ldold = 0, *ldnew = 0;
    int repcnt = 1;
    if (found_dy) {
        char *lname = 0;
        while ((tok = lstring::gettok(&s)) != 0) {
            if (!strcmp(tok, "-l")) {
                delete [] tok;
                delete [] lname;
                lname = lstring::gettok(&s);
                if (!lname) {
                    PL()->ShowPrompt("Missing layer name following \"-l\".");
                    return;
                }
                continue;
            }
            if (!strcmp(tok, "-r")) {
                delete [] tok;
                tok = lstring::gettok(&s);
                if (!tok) {
                    PL()->ShowPrompt(
                        "Missing replication count following \"-r\".");
                    delete [] lname;
                    return;
                }
                repcnt = atoi(tok);
                if (repcnt < 1 || repcnt > 10000) {
                    PL()->ShowPromptV(
                        "Bad replication %s, requires 1-100000.", tok);
                    delete [] tok;
                    delete [] lname;
                    return;
                }
                delete [] tok;
                continue;
            }
            bool digits = true;
            for (char *p = tok; *p; p++) {
                if (!isdigit(*p)) {
                    digits = false;
                    break;
                }
            }
            if (digits && !CDldb()->findLayer(tok, DSP()->CurMode())) {
                repcnt = atoi(tok);
                delete [] tok;
            }
            else {
                delete [] lname;
                lname = tok;
            }
        }
        if (lname) {
            CDl *ld = CDldb()->findLayer(lname, DSP()->CurMode());
            if (!ld) {
                PL()->ShowPromptV("Layer not found: %s", lname);
                delete [] lname;
                return;
            }
            if (ld != LT()->CurLayer()) {
                if (ED()->getLchgMode() == LCHGcur) {
                    // Map old current to new.
                    ldnew = ld;
                    ldold = LT()->CurLayer();
                }
                else if (ED()->getLchgMode() == LCHGall) {
                    // Map all layers to new.
                    ldnew = ld;
                    ldold = 0;
                }
            }
        }
        delete [] lname;
    }
    ED()->copyObjects(0, 0, idx, idy, ldold, ldnew, repcnt);
}


void
edit_bangcmds::spin(const char *s)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPromptV(phys_msg, "!spin");
        return;
    }
    CDs *cursd = CurCell(Physical);
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    if (!Selections.hasTypes(cursd, 0)) {
        PL()->ShowPrompt("No objects selected to rotate.");
        return;
    }
    double x, y, ang;
    char buf[256];
    int i = sscanf(s, "%lf %lf %lf %s", &x, &y, &ang, buf);
    if (i < 3) {
        PL()->ShowPrompt(
    "Usage:  !spin x y ang [layer_name]  (x,y in microns, ang in degrees)");
        return;
    }
    int ix = INTERNAL_UNITS(x);
    int iy = INTERNAL_UNITS(y);
    EV()->InitCallback();
    PL()->ErasePrompt();
    CDl *ldnew = 0, *ldold = 0;
    if (i == 4) {
        CDl *ld = CDldb()->findLayer(buf, DSP()->CurMode());
        if (!ld) {
            PL()->ShowPromptV("Layer not found: %s", buf);
            return;
        }
        if (ld != LT()->CurLayer()) {
            if (ED()->getLchgMode() == LCHGcur) {
                // Map old current to new.
                ldnew = ld;
                ldold = LT()->CurLayer();
            }
            else if (ED()->getLchgMode() == LCHGall) {
                // Map all layers to new.
                ldnew = ld;
                ldold = 0;
            }
        }
    }
    ang *= M_PI/180.0;
    Ulist()->ListCheck("spin", cursd, true);
    if (ED()->rotateQueue(ix, iy, ang, ix, iy, ldold, ldnew, CDcopy))
        Ulist()->CommitChanges(true);
}


void
edit_bangcmds::rename(const char *s)
{
    const char *msg = "Error:  usage:  rename [prefix]|[-s] [suffix]";
    if (!*s) {
        PL()->ShowPrompt(msg);
        return;
    }
    if (!DSP()->CurCellName())
        return;
    char *pre = 0, *suf = 0;
    if (*s == '-' && *(s+1) == 's') {
        s += 2;
        while (isspace(*s))
            s++;
        suf = lstring::gettok(&s);
    }
    else {
        pre = lstring::gettok(&s);
        if (*s == '-' && *(s+1) == 's') {
            s += 2;
            while (isspace(*s))
                s++;
        }
        suf = lstring::gettok(&s);
    }
    if (!pre && !suf) {
        PL()->ShowPrompt(msg);
        return;
    }

    // The prefix and suffix can contain only alphanum, and $_?, but
    // can have a substitution form /str/sub/
    int scnt = 0;
    for (s = pre; s && *s; s++) {
        if (!isalpha(*s) && !isdigit(*s) && *s != '_' && *s != '$' &&
                *s != '?' && *s != '/') {
            PL()->ShowPromptV("Error:  bad char '%c' in prefix.", *s);
            return;
        }
        if (*s == '/')
            scnt++;
    }
    if (scnt && (scnt != 3 ||
            *pre != '/' || *(pre + strlen(pre) - 1) != '/')) {
        PL()->ShowPrompt("Error:  bad substitution form in prefix.");
        return;
    }

    scnt = 0;
    for (s = suf; s && *s; s++) {
        if (!isalpha(*s) && !isdigit(*s) && *s != '_' && *s != '$' &&
                *s != '?' && *s != '/') {
            PL()->ShowPromptV("Error:  bad char '%c' in suffix.", *s);
            return;
        }
        if (*s == '/')
            scnt++;
    }
    if (scnt && (scnt != 3 ||
            *suf != '/' || *(suf + strlen(suf) - 1) != '/')) {
        PL()->ShowPrompt("Error:  bad substitution form in suffix.");
        return;
    }

    CDcbin cbin(DSP()->CurCellName());
    stringlist *er = ED()->renameAll(&cbin, pre, suf);
    delete [] pre;
    delete [] suf;
    DSP()->RedisplayAll(Physical);
    DSP()->RedisplayAll(Electrical);
    PL()->ErasePrompt();
    DSP()->MainWdesc()->ShowTitle();
    if (er) {
        char *list = stringlist::col_format(er, 80);
        Log()->ErrorLogV(mh::EditOperation,
            "The following cells were not renamed, likely due to "
            "conflict:\n%s\n",
            list);
        stringlist::destroy(er);
        delete [] list;
    }
}


void
edit_bangcmds::svq(const char *s)
{
    char *tok = lstring::gettok(&s);
    if (tok && !isdigit(*tok)) {
        PL()->ShowPrompt("Usage:  !svq [reg_number]");
        return;
    }
    int regnum = tok ? *tok - '0' : 0;
    delete [] tok;

    char tname[64];
    sprintf(tname, "$$$$REG%d", regnum);
    CDcbin cbin;
    if (ED()->createCell(tname, &cbin, false, 0) && cbin.phys()) {
        cbin.phys()->setImmutable(true);
        cbin.phys()->clearModified();
        PL()->ShowPrompt("Register save successful.");
    }
    else
        PL()->ShowPrompt("Error: register save was unsuccessful.");
}


void
edit_bangcmds::rcq(const char *s)
{
    char *tok = lstring::gettok(&s);
    if (tok && !isdigit(*tok)) {
        PL()->ShowPrompt("Usage:  !svq [reg_number]");
        return;
    }
    int regnum = tok ? *tok - '0' : 0;
    delete [] tok;

    char tname[64];
    sprintf(tname, "$$$$REG%d", regnum);

    const char *msg = "Nothing saved in register %d.";
    CDcbin cbin;
    if (!CDcdb()->findSymbol(tname, &cbin)) {
        PL()->ShowPromptV(msg, regnum);
        return;
    }
    CDs *sd = cbin.celldesc(DSP()->CurMode());
    if (!sd || sd->isEmpty()) {
        PL()->ShowPromptV(msg, regnum);
        return;
    }
    PL()->ErasePrompt();
    ED()->placeDev(0, tname, true);
}


void
edit_bangcmds::box2poly(const char*)
{
    PL()->ErasePrompt();
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPromptV(phys_msg, "!box2poly");
        return;
    }
    CDs *cursd = CurCell(Physical);
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    sSelGen sg(Selections, cursd, "b");
    CDo *od;
    bool found = false;
    while ((od = sg.next()) != 0) {
        if (!od->is_normal())
            continue;
        if (od->state() == CDSelected && od->ldesc()->isSelectable()) {
            found = true;
            break;
        }
    }
    if (!found) {
        PL()->ShowPrompt("No convertable boxes found.");
        return;
    }
    EV()->InitCallback();
    Ulist()->ListCheck("B2P", cursd, true);
    int cnt = 0;
    sg = sSelGen(Selections, cursd, "b");
    while ((od = sg.next()) != 0) {
        if (od->state() == CDSelected && od->ldesc()->isSelectable()) {
            Poly po(5, new Point[5]);
            od->oBB().to_path(po.points);
            if (cursd->newPoly(od, &po, od->ldesc(), 0, false))
                cnt++;
        }
    }
    Ulist()->CommitChanges(true);
    XM()->ShowParameters();
    if (cnt == 1)
        PL()->ShowPromptV("%d box converted to polygon.", cnt);
    else
        PL()->ShowPromptV("%d boxes converted to polygons.", cnt);
}


void
edit_bangcmds::path2poly(const char*)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPromptV(phys_msg, "!path2poly");
        return;
    }
    CDs *cursd = CurCell(Physical);
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    if (!Selections.hasTypes(cursd, "w")) {
        PL()->ShowPrompt("No wires selected.");
        return;
    }
    int ccnt = 0, ecnt = 0;
    Ulist()->ListCheck("pth2ply", cursd, false);
    sSelGen sg(Selections, cursd, "w");
    CDo *od;
    while ((od = sg.next()) != 0) {
        int num = ((const CDw*)od)->numpts();
        const Point *pts = ((const CDw*)od)->points();
        if (num >= 4 && pts[0] == pts[num - 1]) {
            Poly po(num, Point::dup(pts, num));
            if (!cursd->newPoly(od, &po, od->ldesc(), 0, false)) {
                ecnt++;
            }
            else
                ccnt++;
        }
    }
    Ulist()->CommitChanges(true);
    PL()->ShowPromptV("Polys created: %d   Errors: %d", ccnt, ecnt);
}


void
edit_bangcmds::poly2path(const char*)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPromptV(phys_msg, "!poly2path");
        return;
    }
    CDs *cursd = CurCell(Physical);
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    if (!Selections.hasTypes(cursd, "p")) {
        PL()->ShowPrompt("No polygons selected.");
        return;
    }
    int ccnt = 0, ecnt = 0;
    Ulist()->ListCheck("pth2ply", cursd, false);
    sSelGen sg(Selections, cursd, "p");
    CDo *od;
    while ((od = sg.next()) != 0) {
        int num = ((const CDpo*)od)->numpts();
        const Point *pts = ((const CDpo*)od)->points();
        Wire w(dsp_prm(od->ldesc())->wire_width(), CDWIRE_FLUSH,
            num, Point::dup(pts, num));
        if (!cursd->newWire(od, &w, od->ldesc(), 0, false))
            ecnt++;
        else
            ccnt++;
    }
    Ulist()->CommitChanges(true);
    PL()->ShowPromptV("Wires created: %d   Errors: %d", ccnt, ecnt);
}


void
edit_bangcmds::bloat(const char *s)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPrompt("Switch to Physical mode for !bloat.");
        return;
    }
    CDs *cursd = CurCell(Physical);
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    if (!Selections.hasTypes(cursd, "bpw")) {
        PL()->ShowPrompt("No objects are selected.");
        return;
    }
    char *tok = lstring::gettok(&s);
    if (!tok) {
        PL()->ShowPrompt("Usage:  !bloat dimen [mode]");
        return;
    }
    double f = atof(tok);
    delete [] tok;
    int d = INTERNAL_UNITS(f);
    if (d == 0) {
        PL()->ShowPrompt("Bad bloat dimension given.");
        return;
    }
    tok = lstring::gettok(&s);
    int mode = 0;
    if (tok) {
        if (tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X'))
            sscanf(tok+2, "%x", &mode);
        else
            mode = atoi(tok);
        delete [] tok;
    }

    sSelGen sg(Selections, cursd, "bpw", false);
    CDo *od;
    bool found = false;
    while ((od = sg.next()) != 0) {
        if (!od->is_normal())
            continue;
        if (od->state() == CDSelected && od->ldesc()->isSelectable()) {
            found = true;
            break;
        }
    }
    if (!found) {
        PL()->ShowPrompt("Nothing found to bloat.");
        return;
    }
    EV()->InitCallback();
    PL()->ErasePrompt();
    Ulist()->ListCheck("bloat", cursd, true);
    XIrt ret = ED()->bloatQueue(d, mode);
    if (ret == XIbad) {
        PL()->ShowPrompt("FAILED: internal error.");
        return;
    }
    if (ret == XIintr) {
        PL()->ShowPrompt("Interrupt, aborted.");
        return;
    }
    Ulist()->CommitChanges(true);
}


void
edit_bangcmds::join(const char *s)
{
    PL()->ErasePrompt();
    if (s && *s) {
        if (*s == '-')
            s++;
        if (*s == 'a' || *s == 'A') {
            if (!ED()->joinAllCmd())
                PL()->ShowPrompt(Errs()->get_error());
            return;
        }
        if (*s == 'l' || *s == 'L') {
            if (!ED()->joinLyrCmd())
                PL()->ShowPrompt(Errs()->get_error());
            return;
        }
        PL()->ShowPrompt("Unknown option, command aborted.");
        return;
    }

    if (!ED()->joinCmd())
        PL()->ShowPrompt(Errs()->get_error());
}


void
edit_bangcmds::split(const char *s)
{
    PL()->ErasePrompt();
    bool vert = false;
    if (*s == '1' || *s == 'v' || *s == 'V')
        vert = true;
    if (!ED()->splitCmd(vert))
        PL()->ShowPrompt(Errs()->get_error());
}


void
edit_bangcmds::jw(const char *s)
{
    PL()->ErasePrompt();
    if (s && *s) {
        if (*s == '-')
            s++;
        if (*s == 'l' || *s == 'L') {
            if (!ED()->joinWireLyrCmd())
                PL()->ShowPrompt(Errs()->get_error());
            return;
        }
        PL()->ShowPrompt("Unknown option, command aborted.");
        return;
    }
    if (!ED()->joinWireCmd())
        PL()->ShowPrompt(Errs()->get_error());
}


void
edit_bangcmds::manh(const char *s)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPromptV(phys_msg, "!manh");
        return;
    }
    CDs *cursd = CurCell(Physical);
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    double d;
    int m = 0;;
    if (sscanf(s, "%lf %d", &d, &m) < 1) {
        PL()->ShowPrompt("Usage:  !manh min_box_size [mode]");
        return;
    }
    if (d < 0.01) {
        PL()->ShowPrompt("!manh: bad min_box_size");
        return;
    }
    Ulist()->ListCheck("manh", cursd, true);
    ED()->manhattanizeQueue(INTERNAL_UNITS(d), m);
    Ulist()->CommitChanges(true);
    PL()->ErasePrompt();
}


void
edit_bangcmds::polyfix(const char*)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPromptV(phys_msg, "!polyfix");
        return;
    }
    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    if (!Selections.hasTypes(cursdp, "p")) {
        PL()->ShowPrompt("No polygons selected.");
        return;
    }

    CDol *badlist = 0;
    sSelGen sg(Selections, cursdp, "p");
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (((CDpo*)od)->po_check_poly(0, false) & PCHK_NVERTS)
            badlist = new CDol(od, badlist);
    }
    int nbad = 0;
    if (badlist) {
        // Hopelessly bad polygons are deleted.  It should not be possible
        // for these to get into the database.

        Ulist()->ListCheck("polyfix", cursdp, false);
        for (CDol *o = badlist; o; o = o->next) {
            Ulist()->RecordObjectChange(cursdp, o->odesc, 0);
            nbad++;
        }
        Ulist()->CommitChanges(true);
        CDol::destroy(badlist);
    }
    if (nbad)
        PL()->ShowPromptV(
        "Selected polys had inline vertices removed, deleted %d bad polys.",
            nbad);
    else
        PL()->ShowPrompt("Selected polys had inline vertices removed.");
}


void
edit_bangcmds::polyrev(const char*)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPromptV(phys_msg, "!polyrev");
        return;
    }
    CDs *cursd = CurCell(Physical);
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    if (!Selections.hasTypes(cursd, "p")) {
        PL()->ShowPrompt("No polygons selected.");
        return;
    }
    sSelGen sg(Selections, cursd, "p");
    CDo *od;
    while ((od = sg.next()) != 0)
        ((CDpo*)od)->reverse_list();
    PL()->ShowPrompt("Selected polys had vertex order reversed.");
}


void
edit_bangcmds::noacute(const char*)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPromptV(phys_msg, "!noacute");
        return;
    }
    CDs *cursd = CurCell(Physical);
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }

    if (!Selections.hasTypes(cursd, "p")) {
        PL()->ShowPrompt("No polygons are selected.");
        return;
    }

    EV()->InitCallback();
    Ulist()->ListCheck("!noacute", cursd, false);
    sSelGen sg(Selections, cursd, "p");
    CDo *od;
    while ((od = sg.next()) != 0) {
        Poly *p = ((const CDpo*)od)->po_clip_acute(0);
        if (p) {
            CDpo *newp = cursd->newPoly(od, p, od->ldesc(), od->prpty_list(),
                false);
            delete p;
            sg.replace(newp);
        }
    }
    Ulist()->CommitChanges(true);
}


void
edit_bangcmds::togrid(const char*)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPromptV(phys_msg, "!togrid");
        return;
    }
    CDs *cursd = CurCell(Physical);
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }

    if (!Selections.hasTypes(cursd, "bpw")) {
        PL()->ShowPrompt("No boxes/polygons/wires are selected.");
        return;
    }
    int chcnt = 0;
    Ulist()->ListCheck("!togrid", cursd, false);
    sSelGen sg(Selections, cursd, "bpw");
    CDo *od;
    while ((od = sg.next()) != 0) {
        if (od->type() == CDBOX) {
            BBox BB = od->oBB();
            DSP()->MainWdesc()->Snap(&BB.left, &BB.bottom);
            DSP()->MainWdesc()->Snap(&BB.right, &BB.top);
            if (BB != od->oBB() && BB.right > BB.left && BB.top > BB.bottom) {
                CDo *newp = cursd->newBox(od, &BB, od->ldesc(),
                    od->prpty_list());
                if (newp) {
                    sg.replace(newp);
                    chcnt++;
                }
            }
        }
        else if (od->type() == CDPOLYGON) {
            int num = ((const CDpo*)od)->numpts();
            const Point *pts = ((const CDpo*)od)->points();
            Poly p(num, Point::dup(pts, num));
            bool changed = false;
            for (int i = 0; i < num; i++) {
                DSP()->MainWdesc()->Snap(&p.points[i].x, &p.points[i].y);
                if (p.points[i] != pts[i])
                    changed = true;
            }
            if (changed) {
                if (p.valid()) {
                    CDpo *newp = cursd->newPoly(od, &p, od->ldesc(),
                        od->prpty_list(), false);
                    if (newp) {
                        sg.replace(newp);
                        chcnt++;
                    }
                }
            }
            delete [] p.points;
        }
        else if (od->type() == CDWIRE) {
            int num = ((const CDw*)od)->numpts();
            const Point *pts = ((const CDw*)od)->points();
            Wire w(num, Point::dup(pts, num), ((const CDw*)od)->attributes());
            bool changed = false;
            for (int i = 0; i < num; i++) {
                DSP()->MainWdesc()->Snap(&w.points[i].x, &w.points[i].y);
                if (w.points[i] != pts[i])
                    changed = true;
            }
            if (changed) {
                Point::removeDups(w.points, &w.numpts);
                CDw *newp = cursd->newWire(od, &w, od->ldesc(),
                    od->prpty_list(), false);
                if (newp) {
                    sg.replace(newp);
                    chcnt++;
                }
            }
            delete [] w.points;
        }
    }
    Ulist()->CommitChanges(true);
    PL()->ShowPromptV(
        "Selected objects vertices on grid, %d objects modified.",
        chcnt);
}


void
edit_bangcmds::tospot(const char *s)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPromptV(phys_msg, "!tospot");
        return;
    }
    CDs *cursd = CurCell(Physical);
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    if (!Selections.hasTypes(cursd, "p")) {
        PL()->ShowPrompt("No polygons are selected.");
        return;
    }
    EV()->InitCallback();
    int spot_size = 0;
    if (*s)
        spot_size = INTERNAL_UNITS(atof(s));
    else {
        spot_size = GEO()->spotSize();
        if (spot_size < 0)
            spot_size = INTERNAL_UNITS(Tech()->MfgGrid());
    }
    if (spot_size < 2) {
        PL()->ShowPrompt("Spot size too small or not set.");
        return;
    }
    else if (MICRONS(spot_size) > 1.0) {
        PL()->ShowPrompt("Spot size too large, 1.0 micron limit.");
        return;
    }

    int tss = GEO()->spotSize();
    GEO()->setSpotSize(spot_size);
    Ulist()->ListCheck("!tospot", cursd, false);
    sSelGen sg(Selections, cursd, "p");
    CDo *od;
    while ((od = sg.next()) != 0) {
        int num = ((const CDpo*)od)->numpts();
        Poly po(num, Point::dup(((const CDpo*)od)->points(), num));
        GEO()->setToSpot(po.points, &po.numpts);
        CDpo *newp = cursd->newPoly(od, &po, od->ldesc(), od->prpty_list(),
            false);
        sg.replace(newp);
    }
    GEO()->setSpotSize(tss);
    Ulist()->CommitChanges(true);
    PL()->ErasePrompt();
}


namespace {
    bool
    origin_cb(CDo *od, CDdb *db, void *arg)
    {
        int dx = ((int*)arg)[0];
        int dy = ((int*)arg)[1];
        BBox BB(od->oBB());
        BB.left += dx;
        BB.bottom += dy;
        BB.right += dx;
        BB.top += dy;
        od->set_oBB(BB);

        BB = *((CDs*)db)->BB();
        BB.add(&od->oBB());
        ((CDs*)db)->setBB(&BB);

        if (od->type() == CDPOLYGON)
            ((CDpo*)od)->offset_list(dx, dy);
        else if (od->type() == CDWIRE)
            ((CDw*)od)->offset_list(dx, dy);
        else if (od->type() == CDLABEL) {
            CDla *la = (CDla*)od;
            la->set_xpos(la->xpos() + dx);
            la->set_ypos(la->ypos() + dy);
        }
        else if (od->type() == CDINSTANCE) {
            CDc *c = OCALL(od);
            cTfmStack stk;
            stk.TPush();
            stk.TApplyTransform(c);
            stk.TTranslate(dx, dy);
            CDtf tf;
            stk.TCurrent(&tf);
            c->setTransform(&tf);
            stk.TPop();
        }
        return (true);
    }
}


void
edit_bangcmds::origin(const char *s)
{
    const char *msg = "usage: !origin x y | n|s|e|w|nw|ne|sw|se";
    if (!DSP()->CurCellName()) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPromptV(phys_msg, "!origin");
        return;
    }
    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        PL()->ShowPrompt("Physical part of current cell is empty.");
        return;
    }

    const BBox *BB = cursdp->BB();
    int xy[2];
    xy[0] = 0;  // dx
    xy[1] = 0;  // dy
    char *tok1 = lstring::gettok(&s);
    char *tok2 = lstring::gettok(&s);
    if (!tok2) {
        if (!tok1) {
            PL()->ShowPrompt(msg);
            delete [] tok1;
            delete [] tok2;
            return;
        }
        else if (lstring::cieq(tok1, "n"))
            xy[1] = -BB->top;
        else if (lstring::cieq(tok1, "s"))
            xy[1] = -BB->bottom;
        else if (lstring::cieq(tok1, "e"))
            xy[0] = -BB->right;
        else if (lstring::cieq(tok1, "w"))
            xy[0] = -BB->left;
        else if (lstring::cieq(tok1, "nw")) {
            xy[0] = -BB->left;
            xy[1] = -BB->top;
        }
        else if (lstring::cieq(tok1, "ne")) {
            xy[0] = -BB->right;
            xy[1] = -BB->top;
        }
        else if (lstring::cieq(tok1, "sw")) {
            xy[0] = -BB->left;
            xy[1] = -BB->bottom;
        }
        else if (lstring::cieq(tok1, "se")) {
            xy[0] = -BB->right;
            xy[1] = -BB->bottom;
        }
        else {
            PL()->ShowPrompt(msg);
            delete [] tok1;
            delete [] tok2;
            return;
        }
    }
    else {
        double ddx, ddy;
        if (sscanf(tok1, "%lf", &ddx) != 1 ||
                sscanf(tok2, "%lf", &ddy) != 1) {
            PL()->ShowPrompt(msg);
            delete [] tok1;
            delete [] tok2;
            return;
        }
        xy[0] = -INTERNAL_UNITS(ddx) - BB->left;
        xy[1] = -INTERNAL_UNITS(ddy) - BB->bottom;
    }
    delete [] tok1;
    delete [] tok2;

    cursdp->setBB(&CDnullBB);
    cursdp->db_rebuild(origin_cb, xy);
    cursdp->setBBvalid(true);
    cursdp->incModified();
    PL()->ShowPrompt("Done.");
    DSP()->RedisplayAll();
}


namespace {
    bool
    import_cb(CDo *od, CDdb *dbnew, CDdb*, void*)
    {
        BBox BB = *((CDs*)dbnew)->BB();
        BB.add(&od->oBB());
        ((CDs*)dbnew)->setBB(&BB);

        if (od->type() == CDINSTANCE) {
            CDc *c = OCALL(od);
            c->unlinkFromMaster(false);
            bool linked = false;
            CDm_gen mgen((CDs*)dbnew, GEN_MASTERS);
            for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
                if (m->celldesc() == c->masterCell()) {
                    c->setMaster(m);
                    c->linkIntoMaster();
                    linked = true;
                    break;
                }
            }
            if (!linked) {
                CDs *msdesc = c->masterCell();
                CDm *m = new CDm(c->cellname());
                c->setMaster(m);
                c->linkIntoMaster();
                ((CDs*)dbnew)->linkMaster(m);
                m->linkRef(msdesc);
            }
        }
        return (true);
    }
}


void
edit_bangcmds::import(const char *s)
{
    if (!DSP()->CurCellName()) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPromptV(phys_msg, "!import");
        return;
    }
    if (!s || !*s) {
        PL()->ShowPrompt("usage: !import cellname");
        return;
    }
    CDcbin cbin;
    if (!CDcdb()->findSymbol(s, &cbin)) {
        PL()->ShowPromptV("Symbol not found: %s", s);
        return;
    }
    CDs *sd = cbin.phys();
    if (!sd) {
        PL()->ShowPromptV("Physical part of %s is empty.", s);
        return;
    }
    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        PL()->ShowPrompt("Physical part of current cell is empty.");
        return;
    }
    char buf[256];
    sprintf(buf, "This operation will permanently clear %s.  Continue? ",
        s);
    char *in = PL()->EditPrompt(buf, "n");
    in = lstring::strip_space(in);
    if (in && (*in == 'y' || *in == 'Y')) {
        cursdp->db_merge(sd, import_cb, 0);
        CDm_gen mgen(sd, GEN_MASTERS);
        // The master descs are now all empty, free them
        for (CDm *m = mgen.m_first(); m; m = mgen.m_next())
            m->clear();
        sd->setBB(&CDnullBB);
        cursdp->incModified();
        PL()->ShowPrompt("Done.");
        DSP()->RedisplayAll();
    }
    else
        PL()->ErasePrompt();
}


//
// An undocumented test function, for debugging.
//

namespace {
    XIrt
    test_func(const char *s)
    {
        CDs *cursd = CurCell();
        if (!cursd)
            return (XIbad);

        if (!s)
           s = "";

        bool vert = false;  // set me somehow
        bool incl_wires =
            DSP()->CurMode() == Physical && Zlist::JoinSplitWires;
        if (*s == 'w')
            incl_wires = true;
        Zlist *z0 = 0;
        CDo *oo = 0;
        sSelGen sg(Selections, cursd, "bpw");
        CDo *od;
        while ((od = sg.next()) != 0) {
            if (*s == 'w' && od->type() != CDWIRE)
                continue;
            if (od->type() == CDWIRE) {
                if (!incl_wires)
                    continue;
                if (((CDw*)od)->wire_width() == 0)
                    continue;
            }
            if (!od->is_normal())
                continue;
            Zlist *zl;
            if (*s == 'w') {
                Wire w = ((CDw*)od)->w_wire();
                zl = w.toZlistFancy();
            }
            else
                zl = vert ? od->toZlistR() : od->toZlist();
            if (zl) {
                Zlist *zn = zl;
                while (zn->next)
                    zn = zn->next;
                zn->next = z0;
                z0 = zl;
                if (!oo)
                    oo = od;
                Ulist()->RecordObjectChange(cursd, od, 0);
            }
        }
        if (!z0) {
            printf("null zoid list.\n");
            return (XIbad);
        }
        if (!strcmp(s, "ms")) {
            Zlist::expand_by_2(z0);
            Ylist *y = new Ylist(z0);
            z0 = Ylist::repartition_ni(y);
            z0 = Zlist::shrink_by_2(z0);
        }
        if (!strcmp(s, "c")) {
            Ylist *y = new Ylist(z0);
            intDb idb;
            Ylist::scanlines(y, 0, idb);
            int *scans;
            int nscans;
            idb.list(&scans, &nscans);
            y = Ylist::slice(y, scans, nscans);
            delete [] scans;
            z0 = Ylist::to_zlist(y);
        }
        else if (!strcmp(s, "x")) {
            Zlist::zl_andnot(&z0);
        }
        else {
            Ylist *y = new Ylist(z0);
            if (!strcmp(s, "mc")) {
                CD()->SetIgnoreIntr(true);
                bool chg = Ylist::merge_cols(y);
                CD()->SetIgnoreIntr(false);
                printf("merge_cols returned %s.\n", chg ? "true" : "false");
            }
            else if (!strcmp(s, "mr")) {
                CD()->SetIgnoreIntr(true);
                bool chg = Ylist::merge_rows(y);
                CD()->SetIgnoreIntr(false);
                printf("merge_rows returned %s.\n", chg ? "true" : "false");
            }
            else if (!strcmp(s, "r"))
                y = new Ylist(Ylist::repartition_ni(y));
            else if (!strcmp(s, "wr"))
                y = new Ylist(Ylist::repartition_ni(y));
            else if (!strcmp(s, "wp")) {
                z0 = Ylist::to_zlist(y);
                PolyList *pl0 = Zlist::to_poly_list(z0);
                for (PolyList *p = pl0; p; p = p->next)
                    cursd->newPoly(0, &p->po, oo->ldesc(), 0, false);
                PolyList::destroy(pl0);
                return (XIok);
            }

            z0 = Ylist::to_zlist(y);
        }
        for (Zlist *z = z0; z; z = z->next) {
            if (z->Z.xll == z->Z.xul && z->Z.xlr == z->Z.xur) {
                if (vert)
                    cursd->newBox(0, z->Z.yl, -z->Z.xur,
                        z->Z.yu, -z->Z.xll, oo->ldesc(), 0);
                else
                    cursd->newBox(0, z->Z.xll, z->Z.yl,
                        z->Z.xur, z->Z.yu, oo->ldesc(), 0);
            }
            else {
                Poly po;
                if (z->Z.mkpoly(&po.points, &po.numpts, vert))
                    cursd->newPoly(0, &po, oo->ldesc(), 0, false);
            }
        }
        Zlist::destroy(z0);

        return (XIok);
    }
}


// This applies to selected objects.
// The argument is one of:
//   "c"   clip
//   "ci"  clip incremental
//   "mc"  merge cols
//   "mr"  merge rows
//   "r"   repartition
//   "w"   wire zoids, raw
//   "wr"  wire zoids, repartition
//   "wp"  wire zoids, to poly
//
void
edit_bangcmds::testfunc(const char *s)
{
    CDs *cursd = CurCell();
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    PL()->ErasePrompt();
    EV()->InitCallback();
    Ulist()->ListCheck("test", cursd, true);

    XIrt ret = test_func(s);
    if (ret == XIbad)
        PL()->ShowPrompt(Errs()->get_error());
    else {
        Selections.removeTypes(cursd, "bpw");
        Ulist()->CommitChanges(true);
        PL()->ShowPrompt("Done.");
    }
}

