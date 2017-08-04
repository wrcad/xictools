
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

#include "config.h"
#include "dsp.h"
#include "dsp_window.h"
#include "dsp_snap.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_sdb.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "cd_hypertext.h"
#include "fio.h"
#include "fio_chd.h"
#include "rgbzimg.h"
#include "texttf.h"


double GridDesc::g_phys_mfg_grid = 0.0;
double GridDesc::g_elec_mfg_grid = 0.0;

DSPattrib::DSPattrib()
{
    a_phys_grid.set_axes            (AxesMark);
    a_phys_grid.set_displayed       (true);
    a_phys_grid.set_show_on_top     (true);
    a_phys_grid.set_spacing         (1.0);
    a_phys_grid.set_snap            (1);
    a_phys_grid.linestyle().mask    = 0xcc;

    a_elec_grid.set_axes            (AxesNone);
    a_elec_grid.set_displayed       (true);
    a_elec_grid.set_show_on_top     (true);
    a_elec_grid.set_spacing         (1.0);
    a_elec_grid.set_snap            (1);
    a_elec_grid.linestyle().mask    = 0x0;

    a_phys_expand_level         = 0;
    a_elec_expand_level         = 0;
    a_phys_display_labels       = SLupright;
    a_elec_display_labels       = SLupright;
    a_phys_label_instances      = SLupright;
    a_elec_label_instances      = SLupright;
    a_phys_show_context         = true;
    a_elec_show_context         = true;
    a_phys_no_show_unexpand     = false;
    a_elec_no_show_unexpand     = false;
    a_phys_show_tiny_bb         = true;
    a_elec_show_tiny_bb         = true;
    a_phys_props                = false;
    a_no_highlighting           = false;
    a_no_elec_symbolic          = false;

    a_show_boxes                = true;
    a_show_polys                = true;
    a_show_wires                = true;

    a_edge_snapping             = EdgeSnapSome;
    a_edge_off_grid             = false;
    a_edge_non_manh             = false;
    a_edge_wire_edge            = true;
    a_edge_wire_path            = false;
}
// End of DSPattrib functions.


WindowDesc::WindowDesc()
{
    w_width             = 0;
    w_height            = 0;
    w_ratio             = 0.0;
    w_wbag              = 0;
    w_draw              = 0;
    w_cur_cellname      = 0;
    w_top_cellname      = 0;

    w_proxy             = 0;
    w_pending_R         = 0;
    w_pending_U         = 0;

    w_cache             = 0;
    w_fill              = 0;

    w_xsect_yscale      = 1.0;
    w_is_xsect          = false;
    w_xsect_auto_y      = false;

    w_outline           = false;

    w_dbfree            = false;
    w_dbtype            = WDcddb;
    w_dbname            = 0;
    w_dbcellname        = 0;

    w_rgbimg            = 0;

    w_mode              = Physical;
    w_displflag         = 0;

    w_redisplay         = 0;

    w_usecache          = false;
    w_frozen            = false;
    w_show_loc          = false;
    w_main_loc          = false;

    w_need_redisplay    = false;
    w_using_pixmap      = false;
    w_using_image       = false;
    w_old_image         = false;

    w_windowid          = 0;
    w_accum_mode        = WDaccumDone;
}


WindowDesc::~WindowDesc()
{
    int wnum = -1;
    for (int i = 0; i < DSP_NUMWINS; i++) {
        if (DSP()->Window(i) == this) {
            wnum = i;
            break;
        }
    }
    if (wnum > 0 && w_mode == Physical && w_show_loc) {
        if (DSP()->MainWdesc())
            DSP()->MainWdesc()->Refresh(&w_window);
    }
    ClearSpecial();

    if (wnum > 0)
        dspPkgIf()->SubwinDestroy(wnum);

    FlushCache();
    DestroyPixmap();

    ClearViews();
    DSP()->window_destroy(this);

    if (wnum >= 0)
        DSP()->SetWindow(wnum, 0);

    delete w_cache;
    delete w_rgbimg;
    delete w_proxy;
}


void
WindowDesc::ShowTitleDirect()
{
    if (!Wbag())
        return;
    int win = WinNumber();
    if (win < 0)
        return;

    sLstr lstr;
    const char *strlong, *strshort;
    DSP()->window_get_title_strings(&strlong, &strshort);
    if (win == 0)
        lstr.add(strlong);
    else {
        lstr.add(strshort);
        lstr.add_c(' ');
        lstr.add_u(win);
    }
    lstr.add("  ");

    char buf[256];
    buf[0] = 0;
    if (DbType() == WDcddb)
        sprintf(buf, "[cell: %s]",
            CurCellName() ? Tstring(CurCellName()) : "<none>");
    else if (DbType() == WDchd)
        sprintf(buf, "[chd: %s cell: %s]",
            DbName() ? DbName() : "<none>",
            DbCellName() ? DbCellName() : "<default>");
    else if (DbType() == WDblist)
        sprintf(buf, "[cross section]");
    else if (DbType() == WDsdb)
        sprintf(buf, "[db: %s]", DbName() ? DbName() : "<none>");
    lstr.add(buf);

    Wbag()->Title(lstr.string(), strshort);
}


namespace {
    int  id;
    bool which_wins[DSP_NUMWINS];

    int title_idle(void*)
    {
        for (int i = 0; i < DSP_NUMWINS; i++) {
            if (which_wins[i]) {
                if (DSP()->Window(i))
                    DSP()->Window(i)->ShowTitleDirect();
                which_wins[i] = false;
            }
        }
        id = 0;
        return (0);
    }
}


// Set the title bar and icon name strings.  As this is a bit time
// consuming, do it in an idle proc.
//
void
WindowDesc::ShowTitle()
{
    if (!Wbag())
        return;
    int win = WinNumber();
    if (win < 0)
        return;
    which_wins[win] = true;
    if (!id)
        id = dspPkgIf()->RegisterIdleProc(title_idle, 0);
}


// Switch the display mode of a subwindow.
//
void
WindowDesc::SetSubwinMode(DisplayMode mode)
{
    if (this == DSP()->Window(0))
        return;
    if (Mode() == mode)
        return;

    SetMode(mode);
    DSP()->window_mode_change(this);
    ClearViews();

    if (mode == Physical) {
        BBox *wBB = Window();
        WinStr()->ex = (wBB->left + wBB->right)/2;
        WinStr()->ey = (wBB->bottom + wBB->top)/2;
        WinStr()->ewid = wBB->width();
        WinStr()->eset = true;
        if (Wdraw()) {
            Wdraw()->SetBackground(DSP()->Color(BackgroundColor, mode));
            Wdraw()->SetWindowBackground(DSP()->Color(BackgroundColor, mode));
            Wdraw()->SetGhostColor(DSP()->Color(PhysGhostColor));
        }
        if (WinStr()->pset)
            InitWindow(WinStr()->px, WinStr()->py, (double)WinStr()->pwid);
        else
            CenterFullView();
    }
    else {
        BBox *wBB = Window();
        WinStr()->px = (wBB->left + wBB->right)/2;
        WinStr()->py = (wBB->bottom + wBB->top)/2;
        WinStr()->pwid = wBB->width();
        WinStr()->pset = true;
        if (Wdraw()) {
            Wdraw()->SetBackground(DSP()->Color(BackgroundColor, mode));
            Wdraw()->SetWindowBackground(DSP()->Color(BackgroundColor, mode));
            Wdraw()->SetGhostColor(DSP()->Color(ElecGhostColor));
        }
        if (WinStr()->eset)
            InitWindow(WinStr()->ex, WinStr()->ey, (double)WinStr()->ewid);
        else
            CenterFullView();
    }
    UpdateProxy();
    Redisplay(0);
    if (Wbag())
        Wbag()->PopUpGrid(0, MODE_UPD);
}


// Set up the window to display a saved cell hierarchy digest.
//
void
WindowDesc::SetHierDisplayMode(const char *hname, const char *cname,
    const BBox *BB)
{
    if (hname && *hname) {
        DSP()->SetNoRedisplay(true);
        // Suppress redisplay, otherwise get spurrious displays during
        // chd boundary computation.
        SetSpecial(WDchd, hname, false, cname);
        if (BB)
            InitWindow((BB->left + BB->right)/2, (BB->bottom + BB->top)/2,
                (double)BB->width());
        else
            CenterFullView();
        DSP()->SetNoRedisplay(false);
    }
    else
        ClearSpecial();
    Redisplay(0);
    ShowTitle();
    UpdateProxy();
}


// Set up the window to a non-standard ("special") display mode, where
// the contents of a named database is rendered rather than the CD
// cell hierarchy.
//
void
WindowDesc::SetSpecial(WDdbType type, const char *dbname, bool dbfree,
    const char *cname)
{
    w_dbtype = type;
    w_dbname = lstring::copy(dbname);
    w_dbfree = dbfree;
    w_dbcellname = lstring::copy(cname);
    ShowTitle();
    UpdateProxy();
}


// Revert the window to the standard display mode.
//
void
WindowDesc::ClearSpecial()
{
    if (w_dbtype == WDcddb)
        return;

    char *name = w_dbname;
    w_dbname = 0;
    bool free = w_dbfree;
    w_dbfree = false;

    if (w_dbtype == WDchd) {
        delete [] w_dbcellname;
        w_dbcellname = 0;
        if (free && name) {
            cCHD *chd = CDchd()->chdRecall(name, true);
            delete chd;
        }
    }
    else if (w_dbtype == WDblist || w_dbtype == WDsdb) {
        if (free && name) {
            cSDB *sdb = CDsdb()->findDB(name);
            if (sdb) {
                sdb->set_owner(0);
                CDsdb()->destroyDB(name);
            }
        }
    }
    delete [] name;
    w_dbtype = WDcddb;
    ShowTitle();
    UpdateProxy();
}


// Set the viewing context for the window, and redisplay.
//
void
WindowDesc::SetSymbol(const CDcbin *cbin)
{
    if (w_dbtype != WDcddb)
        return;
    ClearViews();
    if (cbin) {
        w_cur_cellname = cbin->cellname();
        w_top_cellname = cbin->cellname();
    }
    else {
        w_cur_cellname = 0;
        w_top_cellname = 0;
    }
    CDs *sdesc = CurCellDesc(w_mode);
    if (!sdesc || sdesc->db_is_empty(0))
        DefaultWindow();
    else
        CenterFullView();
    Redisplay(0);
    ShowTitle();
    UpdateProxy();
}


// Return true if both windows are displaying the same database.
// The flags apply to WDcddb windows only and can be:
//  WDsimXmode (0x1)  Ignore phys/elec mode.
//  WDsimXsymb (0x2)  Ignore symbolic/schematic electrical mode.
//  WDsimXcell (0x4)  Ignore cell differences in CDDB windows
//
bool
WindowDesc::IsSimilar(const WindowDesc *w, unsigned int flags)
{
    if (!w)
        return (false);

    // Override callback.
    if (DSP()->check_similar(this, w))
        return (true);

    if (w_dbtype != w->w_dbtype)
        return (false);
    if (!(flags & WDsimXmode) && w_mode != w->w_mode)
        return (false);
    if (w_dbtype == WDcddb) {
        if (!(flags & WDsimXcell) && w_cur_cellname != w->w_cur_cellname)
            return (false);
        if (!(flags & WDsimXsymb) && !(flags & WDsimXmode) &&
                w_mode == Electrical && w->w_mode == Electrical) {
            CDs *s1 = CurCellDesc(Electrical);
            CDs *s2 = w->CurCellDesc(Electrical);
            bool issm1 = !s1 || !s1->isSymbolic();
            bool issm2 = !s2 || !s2->isSymbolic();
            if (issm1 != issm2)
                return (false);
        }
    }
    else {
        if (!w_dbname || !w->w_dbname) {
            if (w_dbname != w->w_dbname)
               return (false);
        }
        else if (strcmp(w_dbname, w->w_dbname))
            return (false);
    }
    return (true);
}


// Return true if 'this' matches display mode m, and the windows are
// otherwise similar independent of display mode.
// The flags apply to WDcddb windows only and can be:
//  WDsimXsymb (0x2)  Ignore symbolic/schematic electrical mode.
//
bool
WindowDesc::IsSimilar(DisplayMode m, const WindowDesc *w, unsigned int flags)
{
    {
        WindowDesc *wdt = this;
        if (!wdt)
            return (false);
    }
    if (!w)
        return (false);

    // Override callback.
    if (DSP()->check_similar(this, w))
        return (true);

    if (w_dbtype != w->w_dbtype || w_mode != m)
        return (false);
    if (w_dbtype == WDcddb) {
        if (w_cur_cellname != w->w_cur_cellname)
            return (false);
        if (!(flags & WDsimXsymb) && w_mode == Electrical &&
                w->w_mode == Electrical) {
            CDs *s1 = CurCellDesc(Electrical);
            CDs *s2 = w->CurCellDesc(Electrical);
            bool issm1 = !s1 || !s1->isSymbolic();
            bool issm2 = !s2 || !s2->isSymbolic();
            if (issm1 != issm2)
                return (false);
        }
    }
    else {
        if (!w_dbname || !w->w_dbname) {
            if (w_dbname != w->w_dbname)
               return (false);
        }
        else if (strcmp(w_dbname, w->w_dbname))
            return (false);
    }
    return (true);
}


// Return true if this is a non-symbolic electrical view of the same
// cell in w.
//
bool
WindowDesc::IsSimilarNonSymbolic(const WindowDesc *w)
{
    if (!w || w_dbtype != w->w_dbtype || w_mode != Electrical)
        return (false);
    if (w_dbtype == WDcddb) {
        if (w_cur_cellname != w->w_cur_cellname)
            return (false);
        CDs *s1 = CurCellDesc(Electrical);
        if (s1 && s1->isSymbolic())
            return (false);
    }
    else {
        if (!w_dbname || !w->w_dbname) {
            if (w_dbname != w->w_dbname)
               return (false);
        }
        else if (strcmp(w_dbname, w->w_dbname))
            return (false);
    }
    return (true);
}


// Return true if the window is displaying sd as CDDB.
//
bool
WindowDesc::IsShowing(const CDs *sd)
{
    return (sd && CurCellDesc(w_mode) == sd);
}


// Return the cell pointer of the current cell being displayed as
// CDDB.  This returns the symbolic cell if displaying in symbolic
// mode, unless no_symb is true.
//
CDs *
WindowDesc::CurCellDesc(DisplayMode m, bool no_symb) const
{
    if (DbType() != WDcddb)
        return (0);
    if (!CurCellName())
        return (0);
    CDs *sd = CDcdb()->findCell(CurCellName(), m);
    if (!sd)
        return (0);
    if (m == Electrical && !no_symb) {
        const CDc *top_cdesc = 0;
        if (AttribC()->no_elec_symbolic() && CurCellName() == TopCellName())
            top_cdesc = CD_NO_SYMBOLIC;
        else if (CurCellName() != TopCellName() &&
                DSP()->MainWdesc()->Mode() == Electrical &&
                DSP()->MainWdesc()->CurCellName() == CurCellName())
            top_cdesc = DSP()->context_cell();
        CDs *srep = sd->symbolicRep(top_cdesc);
        if (srep)
            return (srep);
    }
    return (sd);
}


// Return the cell pointer of the top cell being displayed as CDDB. 
// This returns the symbolic cell if displaying in symbolic mode,
// unless no_symb is true.
//
CDs *
WindowDesc::TopCellDesc(DisplayMode m, bool no_symb) const
{
    if (DbType() != WDcddb)
        return (0);
    if (!TopCellName())
        return (0);
    CDs *sd = CDcdb()->findCell(TopCellName(), m);
    if (!sd)
        return (0);
    if (m == Electrical && !no_symb) {
        const CDc *top_cdesc = 0;
        if (AttribC()->no_elec_symbolic())
            top_cdesc = CD_NO_SYMBOLIC;
        CDs *srep = sd->symbolicRep(top_cdesc);
        if (srep)
            return (srep);
    }
    return (sd);
}


// Set the expansion state, return false if error
//
bool
WindowDesc::Expand(const char *string)
{
    DSPattrib *a = &w_attributes;
    while (isspace(*string))
        string++;
    int n;
    if (*string == '+') {
        for (n = 0; *string == '+'; n++, string++) ;
        if (isdigit(*string))
            n += atoi(string) - 1;
        if (a->expand_level(w_mode) == -1)
            a->set_expand_level(w_mode, n);
        else {
            int lev = a->expand_level(w_mode);
            a->set_expand_level(w_mode, lev + n);
        }
    }
    else if (*string == '-') {
        for (n = 0; *string == '-'; n++, string++) ;
        if (isdigit(*string))
            n += atoi(string) - 1;
        if (a->expand_level(w_mode)  <= 0)
            a->set_expand_level(w_mode, n);
        else {
            int lev = a->expand_level(w_mode);
            lev -= n;
            if (lev < 0)
                lev = 0;
            a->set_expand_level(w_mode, lev);
        }
    }
    else if (isdigit(*string)) {
        int lev = atoi(string);
        a->set_expand_level(w_mode, lev);
        if (lev == 0)
            ClearExpand();
    }
    else if (*string == 'y' || *string == 'a' || *string == 'Y' ||
            *string == 'A')
        a->set_expand_level(w_mode, -1);
    else if (*string == 'n' || *string == 'N') {
        a->set_expand_level(w_mode, 0);
        ClearExpand();
    }
    else
        return (false);
    return (true);
}


// Unset the expand flag of all cell instances in the hierarchy.
//
void
WindowDesc::ClearExpand()
{
    CDs *sdesc = CurCellDesc(w_mode, true);
    if (!sdesc)
        return;
    CDgenHierDn_s gen(sdesc);
    CDs *sd;
    bool err;
    while ((sd = gen.next(&err)) != 0) {
        CDm_gen mgen(sd, GEN_MASTERS);
        for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
            CDc_gen cgen(m);
            for (CDc *c = cgen.c_first(); c; c = cgen.c_next())
                c->unset_flag(w_displflag);
        }
    }
}


// Set the "windowid" which is some unique value for the display
// window.
//
void
WindowDesc::SetID()
{
    w_windowid = w_draw ? w_draw->WindowID() : 0;
}


// Return the window number.
//
int
WindowDesc::WinNumber() const
{
    for (int i = 0; i < DSP_NUMWINS; i++) {
        if (DSP()->Window(i) == this)
            return (i);
    }
    return (-1);
}


// If an electrical mode connection can be made in the vicinity of x,y
// return true, and set cx,cy to the coordinates of the connection
// point.  Arg oskip is ignored in search.  If skipSel is true,
// selected objects are ignored.  The cflags are retruned to indicate
// whether the target is a line (FC_CX horiz or FC_CY vert), or a
// point.
//
bool
WindowDesc::FindContact(int x, int y, int *cx, int *cy, int *cflags, int delta,
    CDo *oskip, bool skipSel)
{
    if (Mode() != Electrical)
        return (false);
    CDs *cursde = CurCellDesc(Electrical);
    if (!cursde || cursde->isSymbolic())
        return (false);

    if (!delta)
        delta = 2;
    BBox BB;
    BB.left = x - delta;
    BB.bottom = y - delta;
    BB.right = x + delta;
    BB.top = y + delta;

    // first look for device/subcircuit contacts
    CDg gdesc;
    gdesc.init_gen(cursde, CellLayer(), &BB);
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        if (!cdesc->is_normal() || cdesc == oskip ||
                (skipSel && cdesc->state() == CDSelected))
            continue;
        CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int px, py;
                if (!pn->get_pos(ix, &px, &py))
                    break;
                if (BB.intersect(px, py, false)) {
                    *cx = px;
                    *cy = py;
                    *cflags = (FC_CX | FC_CY);
                    return (true);
                }
            }
        }
    }

    // Look for manhattan active wire segments.
    CDsLgen gen(cursde);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        gdesc.init_gen(cursde, ld, &BB);
        CDw *wd;
        while ((wd = (CDw*)gdesc.next()) != 0) {
            if (wd->type() != CDWIRE)
                continue;
            if (!wd->is_normal() || wd == oskip ||
                    (skipSel && wd->state() == CDSelected))
                continue;
            const Point *pts = wd->points();
            int num = wd->numpts();
            for (int i = 1; i < num; i++) {
                if (pts[i-1].x == pts[i].x) {
                    if (abs(x - pts[i].x) > delta)
                        continue;
                    int mx = pts[i-1].y;
                    int mn = pts[i].y;
                    if (mx < mn) {
                        int tmp = mx;
                        mx = mn;
                        mn = tmp;
                    }
                    if (mx + delta >= y && mn - delta <= y) {
                        *cflags = 0;
                        *cx = pts[i].x;
                        if (y > mx) {
                            y = mx;
                            *cflags |= FC_CY;
                        }
                        else if (y < mn) {
                            y = mn;
                            *cflags |= FC_CY;
                        }
                        *cy = y;
                        *cflags |= FC_CX;
                        return (true);
                    }
                }
                else if (pts[i-1].y == pts[i].y) {
                    if (abs(y - pts[i].y) > delta)
                        continue;
                    int mx = pts[i-1].x;
                    int mn = pts[i].x;
                    if (mx < mn) {
                        int tmp = mx;
                        mx = mn;
                        mn = tmp;
                    }
                    if (mx + delta >= x && mn - delta <= x) {
                        *cflags = 0;
                        if (x > mx) {
                            x = mx;
                            *cflags |= FC_CX;
                        }
                        else if (x < mn) {
                            x = mn;
                            *cflags |= FC_CX;
                        }
                        *cx = x;
                        *cy = pts[i].y;
                        *cflags |= FC_CY;
                        return (true);
                    }
                }
            }
        }
    }
    return (false);
}


// As above, but sensitive to bus terminals.
//
bool
WindowDesc::FindBterm(int x, int y, int *cx, int *cy, int *cflags, int delta,
    CDo *oskip, bool skipSel)
{
    if (Mode() != Electrical)
        return (false);
    CDs *cursde = CurCellDesc(Electrical);
    if (!cursde || cursde->isSymbolic())
        return (false);

    if (!delta)
        delta = 2;
    BBox BB;
    BB.left = x - delta;
    BB.bottom = y - delta;
    BB.right = x + delta;
    BB.top = y + delta;

    // first look for device/subcircuit contacts
    CDg gdesc;
    gdesc.init_gen(cursde, CellLayer(), &BB);
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        if (!cdesc->is_normal() || cdesc == oskip ||
                (skipSel && cdesc->state() == CDSelected))
            continue;
        CDp_bcnode *pn = (CDp_bcnode*)cdesc->prpty(P_BNODE);
        for ( ; pn; pn = pn->next()) {
            for (unsigned int ix = 0; ; ix++) {
                int xx, yy;
                if (!pn->get_pos(ix, &xx, &yy))
                    break;
                if (BB.intersect(xx, yy, false)) {
                    *cx = xx;
                    *cy = yy;
                    *cflags = (FC_CX | FC_CY);
                    return (true);
                }
            }
        }
    }

    // Look for manhattan active wire segments.
    CDsLgen gen(cursde);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        gdesc.init_gen(cursde, ld, &BB);
        CDw *wd;
        while ((wd = (CDw*)gdesc.next()) != 0) {
            if (wd->type() != CDWIRE)
                continue;
            if (!wd->is_normal() || wd == oskip ||
                    (skipSel && wd->state() == CDSelected))
                continue;
            const Point *pts = wd->points();
            int num = wd->numpts();
            for (int i = 1; i < num; i++) {
                if (pts[i-1].x == pts[i].x) {
                    if (abs(x - pts[i].x) > delta)
                        continue;
                    int mx = pts[i-1].y;
                    int mn = pts[i].y;
                    if (mx < mn) {
                        int tmp = mx;
                        mx = mn;
                        mn = tmp;
                    }
                    if (mx + delta >= y && mn - delta <= y) {
                        *cflags = 0;
                        *cx = pts[i].x;
                        if (y > mx) {
                            y = mx;
                            *cflags |= FC_CY;
                        }
                        else if (y < mn) {
                            y = mn;
                            *cflags |= FC_CY;
                        }
                        *cy = y;
                        *cflags |= FC_CX;
                        return (true);
                    }
                }
                else if (pts[i-1].y == pts[i].y) {
                    if (abs(y - pts[i].y) > delta)
                        continue;
                    int mx = pts[i-1].x;
                    int mn = pts[i].x;
                    if (mx < mn) {
                        int tmp = mx;
                        mx = mn;
                        mn = tmp;
                    }
                    if (mx + delta >= x && mn - delta <= x) {
                        *cflags = 0;
                        if (x > mx) {
                            x = mx;
                            *cflags |= FC_CX;
                        }
                        else if (x < mn) {
                            x = mn;
                            *cflags |= FC_CX;
                        }
                        *cx = x;
                        *cy = pts[i].y;
                        *cflags |= FC_CY;
                        return (true);
                    }
                }
            }
        }
    }
    return (false);
}


void
WindowDesc::Snap(int *x, int *y, bool indicate)
{
    bool edg = false;
    bool off_gr = false;
    switch (Attrib()->edge_snapping()) {
    case EdgeSnapNone:
        break;
    case EdgeSnapSome:
        if (!DSP()->InEdgeSnappingCmd())
            break;
        edg = true;
        off_gr = Attrib()->edge_off_grid();
        break;
    case EdgeSnapAll:
        edg = true;
        off_gr = Attrib()->edge_off_grid();
        break;
    }

    DSPsnapper sn;
    sn.set_indicate(indicate);
    sn.set_use_edges(edg);
    sn.set_allow_off_grid(off_gr);
    sn.set_do_non_manh(Attrib()->edge_non_manh());
    sn.set_do_wire_edges(Attrib()->edge_wire_edge());
    sn.set_do_wire_path(Attrib()->edge_wire_path());
    sn.snap(this, x, y);
}


// Return a scale factor for Y values (The X scale is always unity). 
// Presently, this is used only for cross-section (profile) displays.
//
double
WindowDesc::YScale()
{
    double ys = 1.0;
    if (IsXSect()) {
        // Showing cross-section.
        if (IsXSectAutoY()) {
            // Scaled so that displayed height is constant with
            // zoom.

            const BBox *cBB = ContentBB();
            if (cBB) {
                int tot_thick = cBB->height();
                ys = w_height*XSectYScale()/(1.1*tot_thick*Ratio());
            }
        }
        else
            ys = XSectYScale();
    }
    return (ys);
}


void
WindowDesc::Display(const CDo *odesc)
{
    if (!odesc || !w_draw)
        return;
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
    {
        if (w_attributes.showing_boxes())
            ShowBox(&odesc->oBB(), odesc->ldesc()->getAttrFlags(),
                dsp_prm(odesc->ldesc())->fill());
        return;
    }
poly:
    {
        if (odesc->state() == CDIncomplete)
            return;
        if (w_attributes.showing_polys()) {
            const Poly po(((const CDpo*)odesc)->po_poly());
            ShowPolygon(&po, odesc->ldesc()->getAttrFlags(),
                dsp_prm(odesc->ldesc())->fill(), &odesc->oBB());
        }
        return;
    }
wire:
    {
        if (odesc->state() == CDIncomplete)
            return;
        if (w_attributes.showing_wires()) {
            const Wire w(((const CDw*)odesc)->w_wire());
            ShowWire(&w, odesc->ldesc()->getAttrFlags(),
                dsp_prm(odesc->ldesc())->fill());
        }
        return;
    }
label:
    {
        if (w_attributes.display_labels(w_mode)) {
            int xform = ((CDla*)odesc)->xform();
            if (xform & (TXTF_SHOW | TXTF_HIDE)) {
                if (xform & TXTF_SHOW) {
                    const Label la(((const CDla*)odesc)->la_label());
                    ShowLabel(&la);
                    return;
                }
                if (xform & TXTF_HIDE) {
                    BBox BB;
                    LabelHideBB((CDla*)odesc, &BB);
                    ShowBox(&BB, 0, 0);
                    return;
                }
            }
            if (LabelHideTest((CDla*)odesc)) {
                BBox BB;
                LabelHideBB((CDla*)odesc, &BB);
                ShowBox(&BB, 0, 0);
                return;
            }
            const Label la(((const CDla*)odesc)->la_label());
            ShowLabel(&la);
        }
        return;
    }
inst:
    return;
}


void
WindowDesc::DisplayIncmplt(const CDo *odesc)
{
    if (!odesc)
        return;
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
label:
inst:
    return;
poly:
    ShowPath(((CDpo*)odesc)->points(), ((CDpo*)odesc)->numpts() - 1, false);
    return;
wire:
    const Wire w = ((const CDw*)odesc)->w_wire();
    ShowWire(&w, odesc->ldesc()->getAttrFlags(),
        dsp_prm(odesc->ldesc())->fill());
    return;
}


void
WindowDesc::DisplaySelected(const CDo *odesc)
{
    if (!odesc || !w_draw)
        return;
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
    ShowBox(&odesc->oBB(), 0, 0);
    ShowObjectCentroidMark(DISPLAY, odesc);
    return;
poly:
    {
        const Poly po(((const CDpo*)odesc)->po_poly());
        ShowPath(po.points, po.numpts, true);
        if (DSP()->NumberVertices() && w_mode == Physical) {
            char buf[32];
            for (int i = 0; i < po.numpts-1; i++) {
                mmItoA(buf, i);
                int x, y;
                LToP(po.points[i].x, po.points[i].y, x, y);
                int dx, dy;
                if (i == 0) {
                    dx = po.points[0].x - po.points[po.numpts-2].x;
                    dy = po.points[0].y - po.points[po.numpts-2].y;
                }
                else {
                    dx = po.points[i].x - po.points[i-1].x;
                    dy = po.points[i].y - po.points[i-1].y;
                }
                int d = (int)sqrt(dx*(double)dx + dy*(double)dy);
                if (!d)
                    d = 1;
                dx = (10*dx)/d;
                dy = (10*dy)/d;
                x += dy - 4;
                y += dx + 6;
                w_draw->Text(buf, x, y, 0);
            }
        }
        ShowObjectCentroidMark(DISPLAY, odesc);
        return;
    }
wire:
    {
        const Wire w(((const CDw*)odesc)->w_wire());
        ShowWire(&w, 0, 0);
        ShowObjectCentroidMark(DISPLAY, odesc);
        return;
    }
label:
    {
        int xform = ((CDla*)odesc)->xform();
        if (xform & (TXTF_SHOW | TXTF_HIDE)) {
            if (xform & TXTF_HIDE) {
                BBox BB;
                LabelHideBB((CDla*)odesc, &BB);
                ShowBox(&BB, 0, 0);
                return;
            }
        }
        else if (LabelHideTest((CDla*)odesc)) {
            BBox BB;
            LabelHideBB((CDla*)odesc, &BB);
            ShowBox(&BB, 0, 0);
            return;
        }
        BBox BB;
        Point *pts;
        odesc->boundary(&BB, &pts);
        if (pts) {
            ShowPath(pts, 5, true);
            delete [] pts;
        }
        else
            ShowBox(&BB, 0, 0);

        // Mark the reference point.
        int delta = LogScaleToPix(3)/Ratio();
        int x = ((CDla*)odesc)->xpos();
        int y = ((CDla*)odesc)->ypos();
        if (xform & TXTF_45) {
            ShowLine(x-delta, y, x+delta, y);
            ShowLine(x, y-delta, x, y+delta);
        }
        else {
            ShowLine(x-delta, y-delta, x+delta, y+delta);
            ShowLine(x-delta, y+delta, x+delta, y-delta);
        }
        return;
    }
inst:
    {
        CDc *cd = (CDc*)odesc;
        CDs *sdesc = cd->masterCell();
        if (!sdesc)
            return;
        if (sdesc->isElectrical() &&
                (sdesc->isSymbolic() || sdesc->isDevice())) {
            // Show cell structure.
            DSP()->TPush();
            DSP()->TApplyTransform(cd);
            DSP()->TPremultiply();
            CDsLgen gen(sdesc);
            CDl *ld;
            while ((ld = gen.next()) != 0) {
                CDg gdesc;
                gdesc.init_gen(sdesc, ld);
                CDo *odtmp;
                while ((odtmp = gdesc.next()) != 0) {
                    if (odtmp->type() != CDLABEL)
                        DisplaySelected(odtmp);
                }
            }
            DSP()->TPop();
        }
        else {
            BBox tBB;
            Point *pts;
            bool mt = false;
            odesc->boundary(&tBB, &pts);
            if (pts) {
                ShowPath(pts, 5, true);
                delete [] pts;
            }
            else {
                if (tBB.width() == 0 && tBB.height() == 0) {
                    // empty cell
                    int mcw = (int)(4.0/w_ratio);
                    tBB.bloat(mcw/2);
                    mt = true;
                }
                ShowBox(&tBB, 0, 0);
            }
            if (!mt)
                ShowInstanceOriginMark(DISPLAY, cd);
        }
        return;
    }
}


// This is only called when not using a backing pixmap, when we have
// to redraw everything.
//
void
WindowDesc::DisplayUnselected(const CDo *odesc)
{
    if (!odesc || !w_draw)
        return;
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
    {
        Blist *bl = AddEdges(0, &odesc->oBB());
        RedisplayList(bl);
        Blist::destroy(bl);
        return;
    }
poly:
wire:
    Redisplay(&odesc->oBB());
    return;
label:
    {
        // Need to bloat this to include the origin mark.
        BBox BB(odesc->oBB());
        int delta = 4.0/Ratio();
        BB.bloat(delta);
        Redisplay(&BB);
    }
    return;
inst:
    {
        CDs *sdesc = ((CDc*)odesc)->masterCell();
        if (!sdesc)
            return;
        BBox BB;
        if (sdesc->isElectrical() && sdesc->isDevice()) {
            odesc->boundary(&BB, 0);
            Redisplay(&BB);
        }
        else {
            Point *pts;
            odesc->boundary(&BB, &pts);
            ShowInstanceOriginMark(ERASE, (CDc*)odesc);
            if (pts) {
                Redisplay(&BB);
                delete [] pts;
            }
            else {
                if (BB.width() == 0 && BB.height() == 0) {
                    // empty cell
                    BB = odesc->oBB();
                    int mcw = (int)(4.0/w_ratio);
                    BB.bloat(mcw/2);
                    Redisplay(&BB);
                }
                else {
                    Blist *bl = AddEdges(0, &odesc->oBB());
                    RedisplayList(bl);
                    Blist::destroy(bl);
                }
            }
        }
        return;
    }
}

