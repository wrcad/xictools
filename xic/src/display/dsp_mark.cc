
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
 $Id: dsp_mark.cc,v 1.154 2016/06/02 03:53:46 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "cd_hypertext.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "cd_lgen.h"
#include "fio.h"
#include "fio_chd.h"
#include "grfont.h"
#include "filestat.h"


/*************************************************************************
 * Functions for drawing various types of marks on-screen.
 *************************************************************************/

// The mColor field of sMark takes an index into the color table, or one
// of the special values below.
#define MultiColor  -1
#define SelectColor -2
//  MultiColor : Use a special color mapping, for Plot marks.
//  SelectColor: Use the selection (blinking) colors.

// These are the mark types
// #define MARK_CROSS   0
// #define MARK_BOX     1
// #define MARK_ARROW   2
// #define MARK_ETERM   3
// #define MARK_STERM   4
// #define MARK_PTERM   5
// #define MARK_ILAB    6
// #define MARK_BSC     7
// #define MARK_SYBSC   8
// #define MARK_FENCE   9
// #define MARK_OBASE   10

// MARK_OBASE must be top.
#define MARK_LIST_TOP   (MARK_OBASE + 1)

namespace dsp_mark {
    // Base type for marks.
    //
    struct sMark
    {
        enum mkDspType { mkDspAll, mkDspMain, mkDspSymb };
        // mdDspAll      Show mark in all mode/cell matching windows.
        // mdDspMain     Show mark in schematic cell of name-matching windows.
        // mdDspSymb     Show mark in symbolic cell of name-matching windows.

        enum gp_style { gp_box, gp_barrel, gp_ullr, gp_diam, gp_oct };

        sMark();
        virtual ~sMark() { }

        static void destroy(const sMark *m)
            {
                while (m) {
                    const sMark *mx  = m;
                    m = m->mNext;
                    delete mx;
                }
            }

        int pixel(WindowDesc*);
        void gp_mark(WindowDesc*, int, bool, gp_style);
        static void destroy();
        virtual void show(WindowDesc*, bool) = 0;
        virtual void addBB(WindowDesc*, BBox*) = 0;

        static void erase_box(WindowDesc*, BBox*);
        static void show_line(WindowDesc*, int, int, int, int);
        static HYorType trans_mark(HYorType);
        static sMark *new_phys_term_marks(CDc*, int);
        static sMark *new_elec_term_marks(CDc*, bool);
        static sMark *new_term_marks(DisplayMode, CDc*, int, bool);
        static sMark *new_elec_bterm_marks(CDc*, bool);
        static sMark *new_bterm_marks(int, CDc*, bool);
        static sMark *new_inst_label(const CDc*, int);

        // Queue a restoration of the BB area from backing store.
        //
        static void
        refresh(WindowDesc *wdesc, BBox *BB)
        {
            BBox tBB;
            wdesc->LToPbb(*BB, tBB);
            wdesc->Update(&tBB);
        }

        sMark *mNext;
        short mType;         // mark type code
        short mMode;         // Physical or Electrical
        short mDelta;        // pixel size of mark
        short mOrient;       // orientation code
        int mX, mY;          // mark position
        short mColor;        // rendering color index
        short mAltColor;     // alternate rendering color index
        int mSubType;        // user data

        static bool mInvisOverride; // show "invisible" terminals
    };

    bool sMark::mInvisOverride;

    // Simple '+' mark, no orientation.
    //
    struct sMark_Cross : public sMark
    {
        sMark_Cross(int, int, int, int, int, int, const char*, const char*,
            sMark*);
        ~sMark_Cross()
            {
                delete [] mStrU;
                delete [] mStrL;
            }
        void show(WindowDesc*, bool);
        void addBB(WindowDesc*, BBox*);
        void cross_mark(WindowDesc*, bool);

        char *mStrU;
        char *mStrL;
    };

    // Small outline box, no orientation.
    //
    struct sMark_Box : public sMark
    {
        sMark_Box(int, int, int, int, int, sMark*, int = 0,
            mkDspType = mkDspAll);
        void show(WindowDesc*, bool);
        void addBB(WindowDesc*, BBox*);

        mkDspType mShowNode;
    };

    // Arrow, oriented.
    //
    struct sMark_Arrow : public sMark
    {
        sMark_Arrow(int, int, int, int, int, int, sMark*);
        void show(WindowDesc*, bool);
        void addBB(WindowDesc*, BBox*);
    };

    // Electrical mode terminal.
    //
    struct sMark_Eterm : public sMark
    {
        sMark_Eterm(int, int, int, int, unsigned int, const CDc*, const char*,
            unsigned int, bool, bool, sMark*);
        ~sMark_Eterm()
            {
                delete [] mTname;
            }
        void show(WindowDesc*, bool);
        void addBB(WindowDesc*, BBox*);
        void setQuad();
        void eterm_mark(WindowDesc*, bool);

        const CDc *mCdesc;
        char *mTname;
        unsigned int mTermnum;
        unsigned int mVflags;
        char mQuad;
        bool mSymbolic;
        bool mUnboxed;
    };

    // Physical mode terminal.
    //
    struct sMark_Pterm : public sMark
    {
        sMark_Pterm(const CDterm*, int, int, sMark*);
        void show(WindowDesc*, bool);
        void addBB(WindowDesc*, BBox*);
        void pterm_mark(WindowDesc*, bool);

        const CDc *mCdesc;
        const CDterm *mTerm;
    };

    // Physical mode instance name label.
    //
    struct sMark_Ilab : public sMark
    {
        sMark_Ilab(const CDc*, int, int, int, sMark*);
        void show(WindowDesc*, bool);
        void addBB(WindowDesc*, BBox*);
        void ilab_mark(WindowDesc*, bool);

        const CDc *mCdesc;
        int mVecIx;
    };

    // Mark for Bus Subcircuit Connector location, electrical mode.
    //
    struct sMark_Bsc : public sMark
    {
        sMark_Bsc(int, int, int, int, int, int, int, CDc*, const char*,
            unsigned int, bool, sMark*);
        ~sMark_Bsc()
            {
                delete [] mTname;
            }
        void show(WindowDesc*, bool);
        void addBB(WindowDesc*, BBox*);
        void setQuad();
        void bsc_mark(WindowDesc*, bool);

        const CDc *mCdesc;
        char *mTname;
        int mBeg;
        int mEnd;
        int mId;
        unsigned int mVflags;
        int mQuad;
        bool mSymbolic;
    };

    // Mark to highlight an edge.  Used for pcell grip handles.
    //
    struct sMark_Fence : public sMark
    {
        sMark_Fence(const CDc*, int, int, int, int, int, int, sMark*);
        void show(WindowDesc*, bool);
        void addBB(WindowDesc*, BBox*);

        const CDc *mCdesc;
        int mEndX;
        int mEndY;
        int mId;
    };

    // Family of marks for SPICE plotting nodes.  These may be oriented,
    // and the type field is <= MARK_OBASE.  These will be visible in
    // windows showing a schematic representation of mSdesc;
    //
    struct sMark_Plot : public sMark
    {
        sMark_Plot(const CDs*, const hyParent*, int, int, int, int, int, int,
            sMark*);
        ~sMark_Plot()
            {
                hyParent::destroy(mProxy);
            }
        void show(WindowDesc*, bool);
        void addBB(WindowDesc*, BBox*);

        const CDs *mSdesc;
        hyParent *mProxy;
    };

    // List for 'current object', each can have own color.
    //
    struct sMobj
    {
        sMobj(CDo *o, int c, sMobj *n) { odesc = o; color = c; next = n; }
        void show(WindowDesc*, bool);

        CDo *odesc;
        int color;
        sMobj *next;
    };

    // List for highlighted wires, we can't trust the wire node property
    // so keep a separate node field.
    //
    struct sMwire
    {
        sMwire(CDw *w, int nd, sMwire *nx)
            {
                wire = w;
                node = nd;
                next = nx;
            }

        CDw *wire;
        int node;
        sMwire *next;
    };

    struct hlite_t;

    // Struct to hold the various mark lists.
    //
    struct sMK
    {
        // Mark support
        int listnum(int type)
            {
                return (type > MARK_OBASE ? MARK_OBASE : type);
            }
        void clear(int);
        void add_front(int, sMark*);
        void add_back(int, sMark*);
        void install_terminal_marks(CDc*);
        sMark *remove_marks(int);
        sMark *remove_terminal_marks(int, CDc*, int = 0);
        void purge_terminal(CDterm*);
        void display_terminal_marks(WindowDesc*, bool);
        void erase_behind(WindowDesc*);
        void show_hlite(WindowDesc*);

        // Cell BB highlight
        void show_BBs(WindowDesc*);
        void clear_BBs();

        // User-specified marks
        void show_user_marks(WindowDesc*, bool);
        void add_user_marksBB(WindowDesc*, BBox*);
        bool remove_user_mark(const CDs*, const BBox*);
        bool remove_user_mark(const CDs*, int);
        void clear_user_marks(const CDs*);
        bool add_user_mark(const CDs*, hlite_t*);
        hlite_t *new_user_mark(const CDs*);
        int dump_user_marks(const char*, const CDs*);

        // Mark support
        sMark *mark_heads[MARK_LIST_TOP]; // list heads for mark types
        sMark *hlite_list;      // list of blinking marks
        sMark *contact_list;    // list of contact marks
        sMobj *object_list;     // "current object" list
        sMwire *wire_list;      // list of wires, for contacts
        Blist *erbh_list;       // "erase behind" regions
        bool save_bound;        // compute/save "erase behind" regions

        // Cell BB highlight
        Blist *phys_show;       // highlight cell BBs (phys)
        Blist *elec_show;       // highlight cell BBs (elec)

        // User-specified marks
        SymTab *user_marks_tab; // table for user mark lists
    };

    sMK MK;


    // Display or erase a "current object".
    //
    void
    sMobj::show(WindowDesc *wdesc, bool display)
    {
        if (!wdesc->Wdraw())
            return;
        if (!wdesc->IsSimilar(DSP()->MainWdesc()))
            return;
        if (!display) {
            wdesc->Redisplay(&odesc->oBB());
            return;
        }
        const BBox *BB = &odesc->oBB();
        wdesc->Wdraw()->SetColor(DSP()->Color(color, wdesc->Mode()));

        if (odesc->type() == CDWIRE) {
            const Wire w(((const CDw*)odesc)->w_wire());
            wdesc->ShowWire(&w, CDL_OUTLINED, 0);
        }
        else if (odesc->type() == CDPOLYGON) {
            const Poly po(((const CDpo*)odesc)->po_poly());
            wdesc->ShowPolygon(&po, CDL_OUTLINED, 0, &odesc->oBB());
        }
        else {
            if (wdesc->Mode() == Electrical) {
                // Dotted box.  Pattern outline is shifted so that the
                // highlighting won't be obscured.

                if (color != HighlightingColor)
                    DSP()->BoxLinestyle()->offset = 3;
                wdesc->Wdraw()->SetLinestyle(DSP()->BoxLinestyle());
                wdesc->ShowLine(BB->left, BB->bottom, BB->right, BB->bottom);
                wdesc->ShowLine(BB->right, BB->bottom, BB->right, BB->top);
                wdesc->ShowLine(BB->right, BB->top, BB->left, BB->top);
                wdesc->ShowLine(BB->left, BB->top, BB->left, BB->bottom);
                DSP()->BoxLinestyle()->offset = 0;
                wdesc->Wdraw()->SetLinestyle(0);
            }
            else {
                // Show the object with an X through the bounding box
                wdesc->ShowLine(BB->left, BB->bottom, BB->right, BB->top);
                wdesc->ShowLine(BB->left, BB->top, BB->right, BB->bottom);
                wdesc->ShowBox(BB, 0, 0);
            }
        }
    }
}

using namespace dsp_mark;

// Highlight the object in odesc, usually with an X over the bounding
// box.
//
void
cDisplay::ShowCurrentObject(bool display, CDo *odesc, int color)
{
    if (color < 0 || color >= ColorTableEnd)
        color = MarkerColor;

    WindowDesc *wdesc;
    if (odesc == 0) {
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            for (sMobj *q = MK.object_list; q; q = q->next) {
                if (q->color == color)
                    q->show(wdesc, display);
            }
        }
        if (!display) {
            sMobj *qp = 0, *qn;
            for (sMobj *q = MK.object_list; q; q = qn) {
                qn = q->next;
                if (q->color == color) {
                    if (!qp)
                        MK.object_list = qn;
                    else
                        qp->next = qn;
                    delete q;
                    continue;
                }
                qp = q;
            }
        }
    }
    else if (display) {
        for (sMobj *q = MK.object_list; q; q = q->next) {
            if (q->odesc == odesc && q->color == color)
                // already there
                return;
        }
        MK.object_list = new sMobj(odesc, color, MK.object_list);
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0)
            MK.object_list->show(wdesc, DISPLAY);
    }
    else {
        sMobj *qp = 0, *qn;
        for (sMobj *q = MK.object_list; q; q = qn) {
            qn = q->next;
            if (q->odesc == odesc && q->color == color) {
                if (!qp)
                    MK.object_list = qn;
                else
                    qp->next = qn;

                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                while ((wdesc = wgen.next()) != 0)
                    q->show(wdesc, ERASE);
                delete q;
                break;
            }
            qp = q;
        }
    }
}


namespace {
    // Recursive instance boundary finder.
    //
    struct lcb_t : public cTfmStack
    {
        lcb_t(CDs *s)
            {
                lcb_sdref = s;
                lcb_blist = 0;
            }

        bool list_subcells_rc(CDs*);

        CDs *lcb_sdref;
        Blist *lcb_blist;
        ptrtab_t lcb_check_tab;
    };


    bool
    lcb_t::list_subcells_rc(CDs *sdesc)
    {
        if (TFull())
            return (false);
        Blist *btmp = lcb_blist;
        CDm_gen mgen(sdesc, GEN_MASTERS);
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            if (!mdesc->celldesc())
                continue;
            if (lcb_check_tab.find(mdesc->celldesc()))
                continue;
            Blist *btmp1 = lcb_blist;
            CDc_gen cgen(mdesc);
            for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
                if (mdesc->celldesc() == lcb_sdref) {
                    lcb_blist = new Blist(&c->oBB(), lcb_blist);
                    TBB(&lcb_blist->BB, 0);
                }
                else {
                    TPush();
                    TApplyTransform(c);
                    TPremultiply();

                    int tx, ty;
                    TGetTrans(&tx, &ty);
                    CDap ap(c);
                    xyg_t xyg(0, ap.nx - 1, 0, ap.ny - 1);

                    do {
                        TTransMult(xyg.x*ap.dx, xyg.y*ap.dy);
                        if (!list_subcells_rc(mdesc->celldesc())) {
                            TPop();
                            return (false);
                        }
                        TSetTrans(tx, ty);
                    } while (xyg.advance());

                    TPop();
                }
                if (btmp1 == lcb_blist)
                    break;
            }
        }
        if (btmp == lcb_blist)
            lcb_check_tab.add(sdesc);
        return (true);
    }


    // Return a list of the BBs of instances of sdesc in the hierarchy
    // rooted in top, in the coordinates of top.
    //
    bool
    list_subcells(CDs *top, CDs *sdesc, Blist **bl)
    {
        *bl = 0;
        if (!top || !sdesc)
            return (true);
        if (top == sdesc) {
            *bl = new Blist(sdesc->BB(), 0);
            return (true);
        }
        lcb_t lcb(sdesc);
        if (!lcb.list_subcells_rc(top))
            return (false);
        *bl = lcb.lcb_blist;
        lcb.lcb_blist = 0;
        return (true);
    }
}


// Highlight the bounding boxes of instances of the given cell.
//
void
cDisplay::ShowCells(const char *name)
{
    MK.clear_BBs();
    if (!name)
        return;
    char buf[256];
    if (MainWdesc()->DbType() == WDcddb) {
        if (!CurCellName())
            return;
        CDcbin cbin;
        CDcdb()->findSymbol(name, &cbin);
        CDcbin ccbin;
        CDcdb()->findSymbol(CurCellName(), &ccbin);
        if (!list_subcells(ccbin.phys(), cbin.phys(), &MK.phys_show)) {
            Errs()->get_error();
            return;
        }
        if (!list_subcells(ccbin.elec(), cbin.elec(), &MK.elec_show)) {
            Errs()->get_error();
            return;
        }
        sprintf(buf, "Marked %d instances of %s.",
            CurMode() == Electrical ?
                Blist::length(MK.elec_show) : Blist::length(MK.phys_show),
            name);
        show_message(buf);
    }
    else if (MainWdesc()->DbType() == WDchd) {
        cCHD *chd = CDchd()->chdRecall(MainWdesc()->DbName(), false);
        if (!chd)
            return;
        symref_t *ptop = chd->findSymref(MainWdesc()->DbCellName(),
            Physical, true);
        if (!ptop)
            return;
        symref_t *pref = chd->findSymref(name, Physical, true);
        if (!pref)
            return;
        Blist *b0;
        if (!chd->instanceBoundaries(ptop, pref->get_name(), &b0)) {
            Errs()->get_error();
            return;
        }
        MK.phys_show = b0;
        sprintf(buf, "Marked %d instances of %s.", Blist::length(b0),
            pref->get_name()->string());
        show_message(buf);
    }
    // actual display is in WindowDesc::ShowHighlighting()
}


// Show a small box around each cell contact corresponding to node.
//
void
cDisplay::ShowNode(bool d_or_e, int node)
{
    if (node < 1)
        return;

    if (d_or_e == ERASE) {
        d_showing_node = -1;
        sMark *mprev = 0, *mnext;
        for (sMark *mm = MK.contact_list; mm; mm = mnext) {
            mnext = mm->mNext;
            sMark_Box *mb = dynamic_cast<sMark_Box*>(mm);
            if (mb && mb->mSubType == node) {
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                WindowDesc *wdesc;
                while ((wdesc = wgen.next()) != 0)
                    mb->show(wdesc, ERASE);
                if (mprev)
                    mprev->mNext = mnext;
                else
                    MK.contact_list = mnext;
                delete mm;
                continue;
            }
            mprev = mm;
        }

        sMwire *wlprev = 0, *wlnext;
        for (sMwire *wl = MK.wire_list; wl; wl = wlnext) {
            wlnext = wl->next;
            if (wl->node == node) {

                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                WindowDesc *wdesc;
                while ((wdesc = wgen.next()) != 0) {
                    if (!wdesc->IsSimilarNonSymbolic(MainWdesc()))
                        continue;
                    BBox BB;
                    wdesc->LToPbb(wl->wire->oBB(), BB);
                    wdesc->Update(&BB);
                }
                if (wlprev)
                    wlprev->next = wlnext;
                else
                    MK.wire_list = wlnext;
                delete wl;
                continue;
            }
            wlprev = wl;
        }
        return;
    }

    CDs *sdesc = CurCell(Electrical, true);
    if (!sdesc)
        return;

    CDg gdesc;
    gdesc.init_gen(sdesc, CellLayer());
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        if (!cdesc->is_normal())
            continue;

        // skip terminals
        if (!isDevOrSubc(cdesc->elecCellType()))
            continue;

        CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
        CDp_cnode *pc = (CDp_cnode*)cdesc->prpty(P_NODE);
        for ( ; pc; pc = pc->next()) {
            int cnt = 0;
            CDgenRange rgen(pr);
            while (rgen.next(0)) {
                CDp_cnode *px;
                if (!cnt)
                    px = pc;
                else
                    px = pr->node(0, cnt, pc->index());
                cnt++;
                if (px && px->enode() == node) {
                    for (unsigned int ix = 0; ; ix++) {
                        int x, y;
                        if (!px->get_pos(ix, &x, &y))
                            break;
                        MK.contact_list = new sMark_Box(Electrical, x, y, 10,
                            SelectColor, MK.contact_list, node,
                            sMark::mkDspMain);
                    }
                }
            }
        }
    }

    CDsLgen gen(sdesc);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (!ld->isWireActive())
            continue;
        gdesc.init_gen(sdesc, ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (!odesc->is_normal())
                continue;
            if (odesc->type() != CDWIRE)
                continue;
            CDp_node *pn = (CDp_node*)odesc->prpty(P_NODE);
            if (pn && pn->enode() == node)
                MK.wire_list = new sMwire((CDw*)odesc, node, MK.wire_list);
        }
    }

    // Add boxes for matching cell terminals, both schematic and
    // symbolic.
    bool found_term = false;
    CDp_snode *ps = (CDp_snode*)sdesc->prpty(P_NODE);
    for ( ; ps; ps = ps->next()) {
        if (ps->enode() == node) {
            int x, y;
            ps->get_schem_pos(&x, &y);
            MK.contact_list = new sMark_Box(Electrical, x, y, 10,
                SelectColor, MK.contact_list, node, sMark::mkDspMain);
            found_term = true;
        }
    }
    if (found_term) {
        sdesc = sdesc->symbolicRep(0);
        if (sdesc) {
            ps = (CDp_snode*)sdesc->prpty(P_NODE);
            for ( ; ps; ps = ps->next()) {
                if (ps->enode() == node) {
                    for (unsigned int ix = 0; ; ix++) {
                        int x, y;
                        if (!ps->get_pos(ix, &x, &y))
                            break;
                        MK.contact_list = new sMark_Box(Electrical, x, y, 10,
                            SelectColor, MK.contact_list, node,
                            sMark::mkDspSymb);
                        found_term = true;
                    }
                }
            }
        }
    }

    d_showing_node = node;
}


// Turn on/off the display of terminals, electrical/physical mode
// specific.
//
void
cDisplay::ShowTerminals(bool display)
{
    if (display) {
        CDs *sdesc = CurCell(Electrical, true);
        if (!sdesc)
            return;
        CDg gdesc;
        gdesc.init_gen(sdesc, CellLayer());
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0)
            MK.install_terminal_marks(cdesc);
        if (d_terminals_visible || d_contacts_visible ||
                CurMode() == Electrical)
            MK.install_terminal_marks(0);
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0)
            MK.display_terminal_marks(wdesc, DISPLAY);
        d_show_terminals = true;
    }
    else {
        d_show_terminals = false;
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0)
            MK.display_terminal_marks(wdesc, ERASE);
        MK.clear(MARK_ETERM);
        MK.clear(MARK_STERM);
        MK.clear(MARK_PTERM);
        MK.clear(MARK_ILAB);
        MK.clear(MARK_BSC);
        MK.clear(MARK_SYBSC);
    }
}


// Erase or display the current cell's terminal marks.  This function
// works in physical and electrical mode.
//
void
cDisplay::ShowCellTerminalMarks(bool display)
{
    if (!CurCellName())
        return;

    if (display) {
        sMark *mp = sMark::new_phys_term_marks(0, 0);
        sMark *me = sMark::new_elec_term_marks(0, false);
        sMark *ms = sMark::new_elec_term_marks(0, true);
        sMark *mb = sMark::new_elec_bterm_marks(0, false);
        sMark *my = sMark::new_elec_bterm_marks(0, true);

        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (wdesc->Mode() == Physical) {
                if (d_erase_behind_terms != ErbhNone) {
                    MK.save_bound = true;
                    for (sMark *mm = mp; mm; mm = mm->mNext)
                        mm->show(wdesc, display);
                    MK.save_bound = false;
                    MK.erase_behind(wdesc);
                }
                for (sMark *mm = mp; mm; mm = mm->mNext)
                    mm->show(wdesc, display);
            }
            else {
                for (sMark *mm = me; mm; mm = mm->mNext)
                    mm->show(wdesc, display);
                for (sMark *mm = ms; mm; mm = mm->mNext)
                    mm->show(wdesc, display);
                for (sMark *mm = mb; mm; mm = mm->mNext)
                    mm->show(wdesc, display);
                for (sMark *mm = my; mm; mm = mm->mNext)
                    mm->show(wdesc, display);
            }
        }
        MK.add_front(MARK_PTERM, mp);
        MK.add_front(MARK_ETERM, me);
        MK.add_front(MARK_STERM, ms);
        MK.add_front(MARK_BSC, mb);
        MK.add_front(MARK_SYBSC, my);
    }
    else {
        sMark *mp = MK.remove_terminal_marks(MARK_PTERM, 0);
        sMark *me = MK.remove_terminal_marks(MARK_ETERM, 0);
        sMark *ms = MK.remove_terminal_marks(MARK_STERM, 0);
        sMark *mb = MK.remove_terminal_marks(MARK_BSC, 0);
        sMark *my = MK.remove_terminal_marks(MARK_SYBSC, 0);
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (wdesc->Mode() == Physical) {
                for (sMark *mm = mp; mm; mm = mm->mNext)
                    mm->show(wdesc, ERASE);
            }
            else {
                for (sMark *mm = me; mm; mm = mm->mNext)
                    mm->show(wdesc, ERASE);
                for (sMark *mm = ms; mm; mm = mm->mNext)
                    mm->show(wdesc, ERASE);
                for (sMark *mm = mb; mm; mm = mm->mNext)
                    mm->show(wdesc, ERASE);
                for (sMark *mm = my; mm; mm = mm->mNext)
                    mm->show(wdesc, ERASE);
            }
        }
        sMark::destroy(mp);
        sMark::destroy(me);
        sMark::destroy(ms);
        sMark::destroy(mb);
        sMark::destroy(my);
    }
}


// Erase or display cdesc's terminal marks.  This function works in
// physical and electrical mode, but in either case cdesc must be
// from the electrical database.  The vecix indicates which component
// if cdesc is vectorized.
//
void
cDisplay::ShowInstTerminalMarks(bool display, CDc *cdesc, int vecix)
{
    if (!CurCellName() || !cdesc)
        return;

    if (display) {
        sMark *mp = sMark::new_phys_term_marks(cdesc, vecix);
        sMark *me = sMark::new_elec_term_marks(cdesc, false);
        sMark *ms = sMark::new_elec_term_marks(cdesc, true);
        sMark *mb = sMark::new_elec_bterm_marks(cdesc, false);
        sMark *my = sMark::new_elec_bterm_marks(cdesc, true);
        MK.add_back(MARK_PTERM, mp);
        MK.add_back(MARK_ETERM, me);
        MK.add_back(MARK_STERM, ms);
        MK.add_back(MARK_BSC, mb);
        MK.add_back(MARK_SYBSC, my);
        // These must be linked before showing for the quad
        // feature to work (ETERM/STERM only)

        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (wdesc->Mode() == Physical) {
                if (d_erase_behind_terms != ErbhNone) {
                    MK.save_bound = true;
                    for (sMark *mm = mp; mm; mm = mm->mNext)
                        mm->show(wdesc, display);
                    MK.save_bound = false;
                    MK.erase_behind(wdesc);
                }
                for (sMark *mm = mp; mm; mm = mm->mNext)
                    mm->show(wdesc, display);
            }
            else {
                for (sMark *mm = me; mm; mm = mm->mNext)
                    mm->show(wdesc, display);
                for (sMark *mm = ms; mm; mm = mm->mNext)
                    mm->show(wdesc, display);
                for (sMark *mm = mb; mm; mm = mm->mNext)
                    mm->show(wdesc, display);
                for (sMark *mm = my; mm; mm = mm->mNext)
                    mm->show(wdesc, display);
            }
        }
    }
    else {
        sMark *mp = MK.remove_terminal_marks(MARK_PTERM, cdesc, vecix);
        sMark *me = MK.remove_terminal_marks(MARK_ETERM, cdesc);
        sMark *ms = MK.remove_terminal_marks(MARK_STERM, cdesc);
        sMark *mb = MK.remove_terminal_marks(MARK_BSC, cdesc);
        sMark *my = MK.remove_terminal_marks(MARK_SYBSC, cdesc);
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (wdesc->Mode() == Physical) {
                for (sMark *mm = mp; mm; mm = mm->mNext)
                    mm->show(wdesc, ERASE);
            }
            else {
                for (sMark *mm = me; mm; mm = mm->mNext)
                    mm->show(wdesc, ERASE);
                for (sMark *mm = ms; mm; mm = mm->mNext)
                    mm->show(wdesc, ERASE);
                for (sMark *mm = mb; mm; mm = mm->mNext)
                    mm->show(wdesc, ERASE);
                for (sMark *mm = my; mm; mm = mm->mNext)
                    mm->show(wdesc, ERASE);
            }
        }
        sMark::destroy(mp);
        sMark::destroy(me);
        sMark::destroy(ms);
        sMark::destroy(mb);
        sMark::destroy(my);
    }
}


// Show or erase a list of terminals, called in physical mode.
//
void
cDisplay::ShowPhysTermList(bool display, CDpin *tlist)
{
    if (display) {
        sMark *m0 = 0;
        for (CDpin *t = tlist; t; t = t->next()) {
            m0 = new sMark_Pterm(t->term(), HighlightingColor,
                d_term_mark_size, m0);
        }

        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (wdesc->Mode() == Physical) {
                if (d_erase_behind_terms != ErbhNone) {
                    MK.save_bound = true;
                    for (sMark *mm = m0; mm; mm = mm->mNext)
                        mm->show(wdesc, DISPLAY);
                    MK.save_bound = false;
                    MK.erase_behind(wdesc);
                }
                for (sMark *mm = m0; mm; mm = mm->mNext)
                    mm->show(wdesc, DISPLAY);
            }
        }
        MK.add_front(MARK_PTERM, m0);
    }
    else {
        sMark *m0 = 0;
        for (CDpin *t = tlist; t; t = t->next()) {
            sMark *mp = 0, *mn;
            for (sMark *mm = MK.mark_heads[MARK_PTERM]; mm; mm = mn) {
                mn = mm->mNext;
                if (((sMark_Pterm*)mm)->mTerm == t->term()) {
                    if (!mp)
                        MK.mark_heads[MARK_PTERM] = mn;
                    else
                        mp->mNext = mn;
                    mm->mNext = m0;
                    m0 = mm;
                    break;
                }
                mp = mm;
            }
        }
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            for (sMark *mm = m0; mm; mm = mm->mNext)
                mm->show(wdesc, ERASE);
        }
        sMark::destroy(m0);
    }
}


// Show or erase a list of terminals, called in physical mode.
//
void
cDisplay::ShowPhysTermList(bool display, CDcont *tlist)
{
    if (display) {
        sMark *m0 = 0;
        for (CDcont *t = tlist; t; t = t->next()) {
            m0 = new sMark_Pterm(t->term(), HighlightingColor,
                d_term_mark_size, m0);
        }

        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (wdesc->Mode() == Physical) {
                if (d_erase_behind_terms != ErbhNone) {
                    MK.save_bound = true;
                    for (sMark *mm = m0; mm; mm = mm->mNext)
                        mm->show(wdesc, DISPLAY);
                    MK.save_bound = false;
                    MK.erase_behind(wdesc);
                }
                for (sMark *mm = m0; mm; mm = mm->mNext)
                    mm->show(wdesc, DISPLAY);
            }
        }
        MK.add_front(MARK_PTERM, m0);
    }
    else {
        sMark *m0 = 0;
        for (CDcont *t = tlist; t; t = t->next()) {
            sMark *mp = 0, *mn;
            for (sMark *mm = MK.mark_heads[MARK_PTERM]; mm; mm = mn) {
                mn = mm->mNext;
                if (((sMark_Pterm*)mm)->mTerm == t->term()) {
                    if (!mp)
                        MK.mark_heads[MARK_PTERM] = mn;
                    else
                        mp->mNext = mn;
                    mm->mNext = m0;
                    m0 = mm;
                    break;
                }
                mp = mm;
            }
        }
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            for (sMark *mm = m0; mm; mm = mm->mNext)
                mm->show(wdesc, ERASE);
        }
        sMark::destroy(m0);
    }
}


void
cDisplay::HlitePhysTermList(bool display, CDpin *tlist)
{
    if (display) {
        for (CDpin *t = tlist; t; t = t->next())
            MK.hlite_list = new sMark_Pterm(t->term(), SelectColor,
                d_term_mark_size, MK.hlite_list);
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0)
            MK.show_hlite(wdesc);
    }
    else {
        sMark *m0 = 0;
        for (CDpin *t = tlist; t; t = t->next()) {
            sMark *mp = 0, *mn;
            for (sMark *mm = MK.hlite_list; mm; mm = mn) {
                mn = mm->mNext;
                if (mm->mType == MARK_PTERM &&
                        ((sMark_Pterm*)mm)->mTerm == t->term()) {
                    if (!mp)
                        MK.hlite_list = mn;
                    else
                        mp->mNext = mn;
                    mm->mNext = m0;
                    m0 = mm;
                    break;
                }
                mp = mm;
            }
        }
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            for (sMark *mm = m0; mm; mm = mm->mNext)
                mm->show(wdesc, ERASE);
        }
    }
}


void
cDisplay::HlitePhysTermList(bool display, CDcont *tlist)
{
    if (display) {
        for (CDcont *t = tlist; t; t = t->next())
            MK.hlite_list = new sMark_Pterm(t->term(), SelectColor,
                d_term_mark_size, MK.hlite_list);
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0)
            MK.show_hlite(wdesc);
    }
    else {
        sMark *m0 = 0;
        for (CDcont *t = tlist; t; t = t->next()) {
            sMark *mp = 0, *mn;
            for (sMark *mm = MK.hlite_list; mm; mm = mn) {
                mn = mm->mNext;
                if (mm->mType == MARK_PTERM &&
                        ((sMark_Pterm*)mm)->mTerm == t->term()) {
                    if (!mp)
                        MK.hlite_list = mn;
                    else
                        mp->mNext = mn;
                    mm->mNext = m0;
                    m0 = mm;
                    break;
                }
                mp = mm;
            }
        }
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            for (sMark *mm = m0; mm; mm = mm->mNext)
                mm->show(wdesc, ERASE);
        }
    }
}


void
cDisplay::HlitePhysTerm(bool display, const CDterm *term)
{
    if (!term)
        return;
    if (display) {
        MK.hlite_list = new sMark_Pterm(term, SelectColor,
            d_term_mark_size, MK.hlite_list);
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0)
            MK.show_hlite(wdesc);
    }
    else {
        sMark *m0 = 0;
        sMark *mp = 0, *mn;
        for (sMark *mm = MK.hlite_list; mm; mm = mn) {
            mn = mm->mNext;
            if (mm->mType == MARK_PTERM && ((sMark_Pterm*)mm)->mTerm == term) {
                if (!mp)
                    MK.hlite_list = mn;
                else
                    mp->mNext = mn;
                mm->mNext = m0;
                m0 = mm;
                break;
            }
            mp = mm;
        }
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            for (sMark *mm = m0; mm; mm = mm->mNext)
                mm->show(wdesc, ERASE);
        }
    }
}


void
cDisplay::HliteElecTerm(bool display, const CDp_node *pn, const CDc *cdesc,
    int termnum)
{
    if (!pn)
        return;
    CDs *esdesc = CurCell(Electrical);
    if (!esdesc)
        return;
    bool symb = esdesc->isSymbolic();
    if (symb && cdesc)
        return;

    for (unsigned int ix = 0; ; ix++) {
        int x, y;
        if (cdesc) {
            CDp_cnode *pcn = (CDp_cnode*)pn;
            if (!pcn->get_pos(ix, &x, &y))
                break;
        }
        else {
            CDp_snode *psn = (CDp_snode*)pn;
            if (symb) {
                if (!psn->get_pos(ix, &x, &y))
                    break;
            }
            else {
                if (ix)
                    break;
                psn->get_schem_pos(&x, &y);
            }
        }

        if (display) {
            if (cdesc) {
                CDp_cnode *pcn = (CDp_cnode*)pn;
                MK.hlite_list = new sMark_Eterm(x, y, 40, SelectColor,
                    pcn->index(), cdesc, pcn->term_name()->string(),
                    pcn->term_flags(), symb, false, MK.hlite_list);
            }
            else {
                CDp_snode *psn = (CDp_snode*)pn;
                MK.hlite_list = new sMark_Eterm(x, y, 40, SelectColor,
                    termnum < 0 ? psn->index() : termnum, 0,
                    psn->term_name()->string(), psn->term_flags(),
                    symb, (termnum < 0), MK.hlite_list);
            }

            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0)
                MK.show_hlite(wdesc);
        }
        else {
            sMark *m0 = 0;
            sMark *mp = 0, *mn;
            for (sMark *mm = MK.hlite_list; mm; mm = mn) {
                mn = mm->mNext;
                if (mm->mType == symb ? MARK_STERM : MARK_ETERM &&
                        mm->mX == x && mm->mY == y &&
                        mm->mColor == SelectColor) {
                    if (!mp)
                        MK.hlite_list = mn;
                    else
                        mp->mNext = mn;
                    mm->mNext = m0;
                    m0 = mm;
                    break;
                }
                mp = mm;
            }
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0) {
                for (sMark *mm = m0; mm; mm = mm->mNext)
                    mm->show(wdesc, ERASE);
            }
        }
    }
}


void
cDisplay::HliteElecBsc(bool display, const CDp_bsnode *pn)
{
    if (!pn)
        return;

    CDs *esdesc = CurCell(Electrical);
    if (!esdesc)
        return;
    bool symb = esdesc->isSymbolic();

    for (unsigned int ix = 0; ; ix++) {
        int x, y;
        if (symb) {
            if (!pn->get_pos(ix, &x, &y))
                break;
        }
        else {
            if (ix)
                break;
            pn->get_schem_pos(&x, &y);
        }

        if (display) {
            sLstr lstr;
            pn->add_label_text(&lstr);
            MK.hlite_list = new sMark_Bsc(x, y, 40, SelectColor,
                pn->beg_range(), pn->end_range(), pn->index(),
                0, lstr.string(), pn->flags(), symb, MK.hlite_list);

            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0)
                MK.show_hlite(wdesc);
        }
        else {
            sMark *m0 = 0;
            sMark *mp = 0, *mn;
            for (sMark *mm = MK.hlite_list; mm; mm = mn) {
                mn = mm->mNext;
                if (mm->mType == symb ? MARK_STERM : MARK_ETERM &&
                        mm->mX == x && mm->mY == y &&
                        mm->mColor == SelectColor) {
                    if (!mp)
                        MK.hlite_list = mn;
                    else
                        mp->mNext = mn;
                    mm->mNext = m0;
                    m0 = mm;
                    break;
                }
                mp = mm;
            }
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0) {
                for (sMark *mm = m0; mm; mm = mm->mNext)
                    mm->show(wdesc, ERASE);
            }
        }
    }
}


// Shouldn't need this, since the terminal mark lists are presumably
// cleared while changes are being made.  This will delete any marks
// that reference the terminal.
//
void
cDisplay::ClearTerminal(CDterm *t)
{
    MK.purge_terminal(t);
}


// Show or erase a subcircuit name label for a physical instance.
//
void
cDisplay::ShowInstanceLabel(bool display, CDc *cdesc, int vecix)
{
    if (!CurCellName())
        return;

    if (display) {
        sMark *mm = sMark::new_inst_label(cdesc, vecix);
        if (mm) {
            MK.add_front(MARK_ILAB, mm);
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0) {
                if (wdesc->Mode() == Physical) {
                    if (d_erase_behind_terms != ErbhNone) {
                        MK.save_bound = true;
                        MK.mark_heads[MARK_ILAB]->show(wdesc, DISPLAY);
                        MK.save_bound = false;
                        MK.erase_behind(wdesc);
                    }
                    MK.mark_heads[MARK_ILAB]->show(wdesc, DISPLAY);
                }
            }
        }
    }
    else {
        sMark *m0 = 0, *mp = 0, *mn;
        for (sMark *mm = MK.mark_heads[MARK_ILAB]; mm; mm = mn) {
            mn = mm->mNext;
            if (((sMark_Ilab*)mm)->mCdesc == cdesc &&
                    ((sMark_Ilab*)mm)->mVecIx == vecix) {
                if (!mp)
                    MK.mark_heads[MARK_ILAB] = mn;
                else
                    mp->mNext = mn;
                mm->mNext = m0;
                m0 = mm;
                break;
            }
            mp = mm;
        }
        if (m0) {
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wdesc = wgen.next()) != 0)
                m0->show(wdesc, ERASE);
            delete m0;
        }
    }

}


// Show a mark at the node or branch point being plotted.  Argument
// ent is the hypertext entry referencing the circuit variable being
// plotted, and indx is the trace number.  If prompt is true, also
// print the node/branch reference.
//
void
cDisplay::ShowPlotMark(bool display, hyEnt *ent, int indx, bool prompt)
{
    int x = ent->pos_x();
    int y = ent->pos_y();
    indx++;  // start at 1
    hyParent *p;
    for (p = ent->parent(); p; p = p->next()) {
        DSP()->TPush();
        DSP()->TApplyTransform(p->cdesc());
        DSP()->TPremultiply();
    }
    DSP()->TPoint(&x, &y);
    HYorType orient = sMark::trans_mark(ent->orient());
    for (p = ent->parent(); p; p = p->next())
        DSP()->TPop();

    if (display) {
        ShowObaseMark(DISPLAY, ent->container(), ent->proxy(), x, y, indx, 40,
            orient);
        if (prompt) {
            char *str = ent->stringUpdate(DSP());
            if (str) {
                char buf[256];
                char *s = lstring::stpcpy(buf, "Trace ");
                s = mmItoA(s, indx + 1);
                *s++ = ':';
                *s++ = ' ';
                strcpy(s, str);
                delete [] str;
                show_message(buf);
            }
        }
    }
    else {
        if (prompt)
            show_message("Trace deleted.");
        ShowObaseMark(ERASE, ent->container(), ent->proxy(), x, y, indx, 40,
            orient);
    }
}


// Plot marks default to the highlighting color, this will reset the
// color of the indx mark to the clr_index in the plot colors table.
//
void
cDisplay::SetPlotMarkColor(int indx, int clr_indx)
{
    for (sMark *m = MK.mark_heads[MARK_OBASE]; m; m = m->mNext) {
        if (m->mType == indx + MARK_OBASE) {
            m->mAltColor = clr_indx;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            WindowDesc *wdesc;
            while ((wdesc = wgen.next()) != 0)
                m->show(wdesc, DISPLAY);
            break;
        }
    }
}


// Create or erase a MARK_CROSS mark.  To erase, x, y, the display
// mode, and type must match.  The last three arguments are optional,
// providing a user-defined mark type (default 0), and upper and lower
// strings to be printed by the mark.
//
void
cDisplay::ShowCrossMark(bool display, int x, int y, int color, int pixsz,
    DisplayMode mode, int type, const char *str_u, const char *str_l)
{
    if (display) {
        sMark *mm = MK.mark_heads[MARK_CROSS] =
            new sMark_Cross(mode, x, y, pixsz, color, type, str_u, str_l,
                MK.mark_heads[MARK_CROSS]);
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        WindowDesc *wdesc;
        while ((wdesc = wgen.next()) != 0)
            mm->show(wdesc, DISPLAY);
    }
    else {
        sMark *mp = 0, *mn;
        for (sMark *mm = MK.mark_heads[MARK_CROSS]; mm; mm = mn) {
            mn = mm->mNext;
            if (mm->mX == x && mm->mY == y && mm->mMode == mode &&
                    mm->mSubType == type) {
                if (!mp)
                    MK.mark_heads[MARK_CROSS] = mn;
                else
                    mp->mNext = mn;
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                WindowDesc *wdesc;
                while ((wdesc = wgen.next()) != 0)
                    mm->show(wdesc, ERASE);
                delete mm;
                break;
            }
            mp = mm;
        }
    }
}


// Erase all cross marks matchint mode ant type, type < 0 is a wild
// card.  The type is a user-defined class, this allows all marks of
// the same class to be erased expediently.
//
void
cDisplay::EraseCrossMarks(DisplayMode mode, int type)
{
    sMark *mp = 0, *mn;
    for (sMark *mm = MK.mark_heads[MARK_CROSS]; mm; mm = mn) {
        mn = mm->mNext;
        if (mm->mMode == mode && (type < 0 || mm->mSubType == type)) {
            if (!mp)
                MK.mark_heads[MARK_CROSS] = mn;
            else
                mp->mNext = mn;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            WindowDesc *wdesc;
            while ((wdesc = wgen.next()) != 0)
                mm->show(wdesc, ERASE);
            delete mm;
            continue;
        }
        mp = mm;
    }
}


// Create or erase a MARK_BOX mark.  To erase, x, y, and the display
// mode must match.
//
void
cDisplay::ShowBoxMark(bool display, int x, int y, int color, int pixsz,
    DisplayMode mode)
{
    if (display) {
        sMark *mm = MK.mark_heads[MARK_BOX] =
            new sMark_Box(mode, x, y, pixsz, color,
                MK.mark_heads[MARK_BOX]);
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        WindowDesc *wdesc;
        while ((wdesc = wgen.next()) != 0)
            mm->show(wdesc, DISPLAY);
    }
    else {
        sMark *mp = 0, *mn;
        for (sMark *mm = MK.mark_heads[MARK_BOX]; mm; mm = mn) {
            mn = mm->mNext;
            if (mm->mX == x && mm->mY == y && mm->mMode == mode) {
                if (!mp)
                    MK.mark_heads[MARK_BOX] = mn;
                else
                    mp->mNext = mn;
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                WindowDesc *wdesc;
                while ((wdesc = wgen.next()) != 0)
                    mm->show(wdesc, ERASE);
                delete mm;
                break;
            }
            mp = mm;
        }
    }
}


// Create or erase a MARK_ETERM (electrical non-symbolic terminal)
// mark.  To erase, x, y, and the color must match.
//
void
cDisplay::ShowEtermMark(bool display, int x, int y, int color, int pixsz,
    const CDp_nodeEx *pn, int termnum)
{
    if (!pn)
        return;
    // note mCdesc = 0, i.e., term is formal

    if (display) {
        sMark *mm = MK.mark_heads[MARK_ETERM] =
            new sMark_Eterm(x, y, pixsz, color, termnum, 0,
                pn->term_name()->string(), pn->term_flags(),
                false, false, MK.mark_heads[MARK_ETERM]);
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        WindowDesc *wdesc;
        while ((wdesc = wgen.next()) != 0)
            mm->show(wdesc, DISPLAY);
    }
    else {
        sMark *mp = 0, *mn;
        for (sMark *mm = MK.mark_heads[MARK_ETERM]; mm; mm = mn) {
            mn = mm->mNext;
            if (mm->mX == x && mm->mY == y && mm->mColor == color) {
                if (!mp)
                    MK.mark_heads[MARK_ETERM] = mn;
                else
                    mp->mNext = mn;
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                WindowDesc *wdesc;
                while ((wdesc = wgen.next()) != 0)
                    mm->show(wdesc, ERASE);
                delete mm;
                break;
            }
            mp = mm;
        }
    }
}


// Create or erase a MARK_STERM (electrical symbolic terminal) mark. 
// To erase, x, y, and the color must match.
//
void
cDisplay::ShowStermMark(bool display, int x, int y, int color, int pixsz,
    const CDp_nodeEx *pn, int termnum)
{
    if (!pn)
        return;
    // note mCdesc = 0, i.e., term is formal

    if (display) {
        sMark *mm = MK.mark_heads[MARK_STERM] =
            new sMark_Eterm(x, y, pixsz, color, termnum, 0,
                pn->term_name()->string(), pn->term_flags(),
                true, false, MK.mark_heads[MARK_STERM]);
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        WindowDesc *wdesc;
        while ((wdesc = wgen.next()) != 0)
            mm->show(wdesc, DISPLAY);
    }
    else {
        sMark *mp = 0, *mn;
        for (sMark *mm = MK.mark_heads[MARK_STERM]; mm; mm = mn) {
            mn = mm->mNext;
            if (mm->mX == x && mm->mY == y && mm->mColor == color) {
                if (!mp)
                    MK.mark_heads[MARK_STERM] = mn;
                else
                    mp->mNext = mn;
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                WindowDesc *wdesc;
                while ((wdesc = wgen.next()) != 0)
                    mm->show(wdesc, ERASE);
                delete mm;
                break;
            }
            mp = mm;
        }
    }
}


// Create or erase a MARK_BSC (electrical non-symbolic bus connector)
// mark.  To erase, x, y, and the color must match.
//
void
cDisplay::ShowBscMark(bool display, int x, int y, int color, int pixsz,
    const CDp_bcnode *pb, int indx)
{
    if (display) {
        sLstr lstr;
        pb->add_label_text(&lstr);
        sMark *mm = MK.mark_heads[MARK_BSC] =
            new sMark_Bsc(x, y, pixsz, color,
                pb->beg_range(), pb->end_range(), indx, 0,
                lstr.string(), pb->flags(), false, MK.mark_heads[MARK_BSC]);
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        WindowDesc *wdesc;
        while ((wdesc = wgen.next()) != 0)
            mm->show(wdesc, DISPLAY);
    }
    else {
        sMark *mp = 0, *mn;
        for (sMark *mm = MK.mark_heads[MARK_BSC]; mm; mm = mn) {
            mn = mm->mNext;
            if (mm->mX == x && mm->mY == y && mm->mColor == color) {
                if (!mp)
                    MK.mark_heads[MARK_BSC] = mn;
                else
                    mp->mNext = mn;
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                WindowDesc *wdesc;
                while ((wdesc = wgen.next()) != 0)
                    mm->show(wdesc, ERASE);
                delete mm;
                break;
            }
            mp = mm;
        }
    }
}


// Create or erase a MARK_SYBSC (electrical symbolic bus connector)
// mark.  To erase, x, y, and the color must match.
//
void
cDisplay::ShowSyBscMark(bool display, int x, int y, int color, int pixsz,
    const CDp_bcnode *pb, int indx)
{
    if (display) {
        sLstr lstr;
        pb->add_label_text(&lstr);
        sMark *mm = MK.mark_heads[MARK_SYBSC] =
            new sMark_Bsc(x, y, pixsz, color, pb->beg_range(),
                pb->end_range(), indx, 0,
                lstr.string(), pb->flags(), true, MK.mark_heads[MARK_SYBSC]);
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        WindowDesc *wdesc;
        while ((wdesc = wgen.next()) != 0)
            mm->show(wdesc, DISPLAY);
    }
    else {
        sMark *mp = 0, *mn;
        for (sMark *mm = MK.mark_heads[MARK_SYBSC]; mm; mm = mn) {
            mn = mm->mNext;
            if (mm->mX == x && mm->mY == y && mm->mColor == color) {
                if (!mp)
                    MK.mark_heads[MARK_SYBSC] = mn;
                else
                    mp->mNext = mn;
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                WindowDesc *wdesc;
                while ((wdesc = wgen.next()) != 0)
                    mm->show(wdesc, ERASE);
                delete mm;
                break;
            }
            mp = mm;
        }
    }
}


// Create or erase a MARK_FENCE (physical pcell stretch handle)
// mark.  To erase, the id must match.
//
void
cDisplay::ShowFenceMark(bool display, const CDc *cdesc, int id, int x, int y,
    int xe, int ye, int color)
{
    if (display) {
        sMark *mm = MK.mark_heads[MARK_FENCE] =
            new sMark_Fence(cdesc, id, x, y, xe, ye, color,
                MK.mark_heads[MARK_FENCE]);
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        WindowDesc *wdesc;
        while ((wdesc = wgen.next()) != 0)
            mm->show(wdesc, DISPLAY);
    }
    else {
        sMark *mp = 0, *mn;
        for (sMark *mm = MK.mark_heads[MARK_FENCE]; mm; mm = mn) {
            mn = mm->mNext;
            if (((sMark_Fence*)mm)->mId == id) {
                if (!mp)
                    MK.mark_heads[MARK_FENCE] = mn;
                else
                    mp->mNext = mn;
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                WindowDesc *wdesc;
                while ((wdesc = wgen.next()) != 0)
                    mm->show(wdesc, ERASE);
                delete mm;
                break;
            }
            mp = mm;
        }
    }
}


// Create or erase a MARK_OBASE (electrical plot) mark.  To erase, the
// x, y, and indx must match.
//
void
cDisplay::ShowObaseMark(bool display, const CDs *sd, const hyParent *p,
    int x, int y, int indx, int pixsz, int orient) {
    if (display) {
        sMark *mm = MK.mark_heads[MARK_OBASE] =
            new sMark_Plot(sd, p, MARK_OBASE + indx, x, y, pixsz, orient,
                MultiColor, MK.mark_heads[MARK_OBASE]);
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        WindowDesc *wdesc;
        while ((wdesc = wgen.next()) != 0)
            mm->show(wdesc, DISPLAY);
    }
    else {
        sMark *mp = 0, *mn;
        for (sMark *mm = MK.mark_heads[MARK_OBASE]; mm; mm = mn) {
            mn = mm->mNext;
            if (mm->mX == x && mm->mY == y && mm->mType == MARK_OBASE + indx) {
                if (!mp)
                    MK.mark_heads[MARK_OBASE] = mn;
                else
                    mp->mNext = mn;
                WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                WindowDesc *wdesc;
                while ((wdesc = wgen.next()) != 0)
                    mm->show(wdesc, ERASE);
                delete mm;
                break;
            }
            mp = mm;
        }
    }
}


// Erase all marks of the given type.
//
void
cDisplay::EraseMarks(int type)
{
    sMark *m0 = MK.remove_marks(type);
    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wdesc = wgen.next()) != 0) {
        for (sMark *mm = m0; mm; mm = mm->mNext)
            mm->show(wdesc, ERASE);
    }
    sMark::destroy(m0);
}


// Clear the mark list (no redisplay).
//
void
cDisplay::ClearWindowMarks()
{
    for (int i = 0; i < MARK_LIST_TOP; i++)
        MK.clear(i);
}


// Scale dltp logarithmically depending on the magnification in the
// window.  The return value is the window-coordinate scaled width of
// dltp, which is in pixels.  This is used when we want a mark to
// increase in size as we zoom in, but not linearly.
//
int
WindowDesc::LogScale(int dltp)
{
    double r = w_ratio;
    if (w_mode == Physical)
        r *= (1000.0/INTERNAL_UNITS(CDphysDefTextHeight));
    double delta = dltp/r;
    if (r < .7788)
        delta *= -5.0/log(r);
    else
        delta *= 20.0;

    int idel = mmRnd(delta);
    if (idel <= 0)
        idel = 1;
    return (idel);
}


// This is similar to LogScale(), but the return value is in pixels.
//
int
WindowDesc::LogScaleToPix(int dltp)
{
    double r = w_ratio;
    if (w_mode == Physical)
        r *= (1000.0/INTERNAL_UNITS(CDphysDefTextHeight));
    double delta = dltp/r;
    if (r < .7788)
        delta *= -5.0/log(r);
    else
        delta *= 20.0;

    int deltap = mmRnd(delta*w_ratio);
    dltp <<= 3;
    if (deltap > dltp)
        deltap = dltp;
    return (deltap);
}


// Display highlighted marks and other features (blinking).
//
void
WindowDesc::ShowHighlighting()
{
    if (w_dbtype == WDcddb) {
        DSP()->window_show_blinking(this);
        MK.show_BBs(this);
        MK.show_hlite(this);
        MK.show_user_marks(this, true);
    }
    else if (w_dbtype == WDchd)
        MK.show_BBs(this);
}


// Display all of the marks in the window. Called after a redraw.
//
void
WindowDesc::ShowWindowMarks()
{
    // display the active marks
    for (sMark *mm = MK.mark_heads[MARK_CROSS]; mm; mm = mm->mNext)
        mm->show(this, DISPLAY);
    for (sMark *mm = MK.mark_heads[MARK_BOX]; mm; mm = mm->mNext)
        mm->show(this, DISPLAY);
    for (sMark *mm = MK.mark_heads[MARK_ARROW]; mm; mm = mm->mNext)
        mm->show(this, DISPLAY);
    if (w_mode == Electrical) {
        for (sMark *mm = MK.mark_heads[MARK_ETERM]; mm; mm = mm->mNext)
            if (mm->mColor == HighlightingColor || DSP()->ShowTerminals())
                mm->show(this, DISPLAY);
        for (sMark *mm = MK.mark_heads[MARK_STERM]; mm; mm = mm->mNext)
            if (mm->mColor == HighlightingColor || DSP()->ShowTerminals())
                mm->show(this, DISPLAY);
        for (sMark *mm = MK.mark_heads[MARK_BSC]; mm; mm = mm->mNext)
            if (mm->mColor == HighlightingColor || DSP()->ShowTerminals())
                mm->show(this, DISPLAY);
        for (sMark *mm = MK.mark_heads[MARK_SYBSC]; mm; mm = mm->mNext)
            if (mm->mColor == HighlightingColor || DSP()->ShowTerminals())
                mm->show(this, DISPLAY);
    }
    if (DSP()->ShowTerminals() && w_mode == Physical) {
        if (DSP()->EraseBehindTerms() != ErbhNone) {
            MK.save_bound = true;
            for (sMark *mm = MK.mark_heads[MARK_PTERM]; mm; mm = mm->mNext)
                mm->show(this, DISPLAY);
            for (sMark *mm = MK.mark_heads[MARK_ILAB]; mm; mm = mm->mNext)
                mm->show(this, DISPLAY);
            MK.save_bound = false;
            MK.erase_behind(this);
        }
        for (sMark *mm = MK.mark_heads[MARK_PTERM]; mm; mm = mm->mNext)
            mm->show(this, DISPLAY);
        for (sMark *mm = MK.mark_heads[MARK_ILAB]; mm; mm = mm->mNext)
            mm->show(this, DISPLAY);
    }
    if (w_mode == Physical) {
        for (sMark *mm = MK.mark_heads[MARK_FENCE]; mm; mm = mm->mNext)
            mm->show(this, DISPLAY);
    }
    for (sMark *mm = MK.mark_heads[MARK_OBASE]; mm; mm = mm->mNext)
        mm->show(this, DISPLAY);

    if (w_mode == DSP()->CurMode()) {
        // Don't clip, mungs line pattern
        BBox tBB = w_clip_rect;
        w_clip_rect = Viewport();
        for (sMobj *q = MK.object_list; q; q = q->next)
            q->show(this, DISPLAY);
        w_clip_rect = tBB;
    }
    MK.show_user_marks(this, false);
}


// Add the bounding boxes of all marks to the BB.  The BB is
// initialized with the current cell BB, and this computes the "real"
// BB including marks.
//
void
WindowDesc::AddWindowMarksBB(BBox *BB)
{
    // display the active marks
    for (sMark *mm = MK.mark_heads[MARK_CROSS]; mm; mm = mm->mNext)
        mm->addBB(this, BB);
    for (sMark *mm = MK.mark_heads[MARK_BOX]; mm; mm = mm->mNext)
        mm->addBB(this, BB);
    for (sMark *mm = MK.mark_heads[MARK_ARROW]; mm; mm = mm->mNext)
        mm->addBB(this, BB);
    if (w_mode == Electrical) {
        for (sMark *mm = MK.mark_heads[MARK_ETERM]; mm; mm = mm->mNext)
            if (mm->mColor == HighlightingColor || DSP()->ShowTerminals())
                mm->addBB(this, BB);
        for (sMark *mm = MK.mark_heads[MARK_STERM]; mm; mm = mm->mNext)
            if (mm->mColor == HighlightingColor || DSP()->ShowTerminals())
                mm->addBB(this, BB);
        for (sMark *mm = MK.mark_heads[MARK_BSC]; mm; mm = mm->mNext)
            if (mm->mColor == HighlightingColor || DSP()->ShowTerminals())
                mm->addBB(this, BB);
        for (sMark *mm = MK.mark_heads[MARK_SYBSC]; mm; mm = mm->mNext)
            if (mm->mColor == HighlightingColor || DSP()->ShowTerminals())
                mm->addBB(this, BB);
    }
    if (DSP()->ShowTerminals() && w_mode == Physical) {
        for (sMark *mm = MK.mark_heads[MARK_PTERM]; mm; mm = mm->mNext)
            mm->addBB(this, BB);
        for (sMark *mm = MK.mark_heads[MARK_ILAB]; mm; mm = mm->mNext)
            mm->addBB(this, BB);
    }
    for (sMark *mm = MK.mark_heads[MARK_OBASE]; mm; mm = mm->mNext)
        mm->addBB(this, BB);

    MK.add_user_marksBB(this, BB);
}


// Show a cross at the origin of the subcell.  This is called when
// rendering a selected subcell.  The color is set elsewhere.
//
void
WindowDesc::ShowInstanceOriginMark(bool display, const CDc *cdesc)
{
    if (!DSP()->ShowInstanceOriginMark())
        return;
    int x = 0;
    int y = 0;
    CDs *msdesc = cdesc->masterCell();
    if (msdesc && msdesc->isElectrical()) {
        CDp_snode *pn = (CDp_snode*)msdesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            if (pn->index() == 0) {
                if (msdesc->symbolicRep(cdesc))
                    pn->get_pos(0, &x, &y);
                else
                    pn->get_schem_pos(&x, &y);
                break;
            }
        }
    }
    DSP()->TPush();
    DSP()->TApplyTransform(cdesc);
    DSP()->TPoint(&x, &y);
    DSP()->TPop();

    int delta = LogScaleToPix(4);
    LToP(x, y, x, y);

    if (display) {
        CDtx tx(cdesc);
        if (tx.ax && tx.ay) {
            // 45 rotation.
            sMark::show_line(this, x-delta, y, x+delta, y);
            sMark::show_line(this, x, y-delta, x, y+delta);
        }
        else {
            sMark::show_line(this, x-delta, y-delta, x+delta, y+delta);
            sMark::show_line(this, x-delta, y+delta, x+delta, y-delta);
        }
    }
    else {
        BBox BB(x - delta, y + delta, x + delta, y - delta);
        Update(&BB);
    }
}


void
WindowDesc::ShowObjectCentroidMark(bool display, const CDo *odesc)
{
    if (!DSP()->ShowObjectCentroidMark() || w_mode != Physical)
        return;
    double dx, dy;
    odesc->centroid(&dx, &dy);
    int x = INTERNAL_UNITS(dx);
    int y = INTERNAL_UNITS(dy);

    int delta = LogScaleToPix(8);
    LToP(x, y, x, y);

    if (display) {
        sMark::show_line(this, x-delta, y, x+delta, y);
        sMark::show_line(this, x, y-delta, x, y+delta);
    }
    else {
        BBox BB(x - delta, y + delta, x + delta, y - delta);
        Update(&BB);
    }
}


// Return the BB if the origin mark.
//
bool
WindowDesc::InstanceOriginMarkBB(const CDc *cdesc, BBox *BB)
{
    if (!DSP()->ShowInstanceOriginMark())
        return (false);

    int x = 0;
    int y = 0;
    CDs *msdesc = cdesc->masterCell();
    if (msdesc && msdesc->isElectrical()) {
        CDp_snode *pn = (CDp_snode*)msdesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            if (pn->index() == 0) {
                if (msdesc->symbolicRep(cdesc))
                    pn->get_pos(0, &x, &y);
                else
                    pn->get_schem_pos(&x, &y);
                break;
            }
        }
    }
    DSP()->TPush();
    DSP()->TApplyTransform(cdesc);
    DSP()->TPoint(&x, &y);
    DSP()->TPop();

    int delta = (int)(LogScaleToPix(8) * w_ratio * 1.01);

    BB->left = x;
    BB->bottom = y;
    BB->right = x;
    BB->top = y;
    BB->bloat(delta);
    return (true);
}


void
WindowDesc::ShowRulers()
{
    if (IsSimilar(DSP()->MainWdesc())) {
        int wnum = WinNumber();
        if (wnum >= 0 && wnum < DSP_NUMWINS) {
            for (sRuler *ruler = DSP()->Rulers(); ruler; ruler = ruler->next)
                if (ruler->win_num == wnum)
                    ruler->show(DISPLAY);
        }
    }
}


// Set the sub-window to act as a hypertext proxy for another window. 
// This enable hypertext links to ge established relative to the other
// window by clicking is this one.  Likely, this is showing a
// schematic view of an instance shown as a symbol in the other view.
//
// The ent, if not null, should be a reference to a subcircuit.  It
// can have a proxy list, but not a parent list.
//
bool
WindowDesc::SetProxy(const hyEnt *ent)
{
    // Only applies to sub-windows.
    if (this == DSP()->Window(0))
        return (false);

    // If null passed, destroy any existing proxy.
    if (!ent) {
        if (w_proxy) {
            delete w_proxy;
            w_proxy = 0;
            UpdateProxy();
        }
        return (true);
    }

    // Proxy applies to CDDB sub-windows only.
    if (w_dbtype != WDcddb) {
        if (w_proxy) {
            // Shouldn't have a proxy, rid it.
            delete w_proxy;
            w_proxy = 0;
        }
        return (false);
    }

    // The link must refer to an instance directly (no parent).
    if (!ent->odesc() || ent->odesc()->type() != CDINSTANCE)
        return (false);
    if (ent->parent())
        return (false);

    const CDc *cd = (CDc*)ent->odesc();
    CDs *msd = cd->masterCell(true);
    if (!msd)
        return (false);

    // Instance must be a subcircuit.
    if (msd->elecCellType() != CDelecSubc)
        return (false);

    // If the window has a cell, its schematic part must match. 
    // Otherwise set the window cellname.
    CDs *sd = CurCellDesc(Electrical, true);
    if (sd && sd != msd)
        return (false);
    if (!sd) {
        SetCurCellName(msd->cellname());
        SetTopCellName(msd->cellname());
    }

    // Replace any existing proxy and set new.
    delete w_proxy;
    w_proxy = ent->dup();
    w_proxy->add();
    UpdateProxy();
    return (true);
}


// Return true if the window is an active proxy.
//
bool
WindowDesc::HasProxy() const
{
    if (!w_proxy || !w_proxy->odesc() || w_proxy->odesc()->type() != CDINSTANCE)
        return (false);
    if (Mode() != Electrical)
        return (false);
    CDc *cd = (CDc*)w_proxy->odesc();
    CDs *msd = cd->masterCell(true);
    if (!msd)
        return (false);
    if (msd != CurCellDesc(Electrical))
        return (false);
    hyParent *p = w_proxy->proxy();
    if (p) {
        while (p->next())
            p = p->next();
        cd = p->cdesc();
    }
    if (cd->parent() != CurCell())
        return (false);

    return (true);
}


// Return a parent list, if active.  This is a copy which includes the
// reference cdesc, user should free.
//
hyParent *
WindowDesc::ProxyList() const
{
    if (!HasProxy())
        return (0);
    return (new hyParent((CDc*)w_proxy->odesc(), w_proxy->pos_x(),
        w_proxy->pos_y(), hyParent::dup(w_proxy->proxy())));
}


// Update the proxy label for the current window state.  The label is
// shown when the proxy is active.  The proxy structure is kept even
// if inapplicable, it simply becomes inactive.  It becomes active
// again if the user returns to the previous status, e.g., switching
// back to electrical mode.
//
void
WindowDesc::UpdateProxy()
{
    if (!Wbag())
        return;
    if (this == DSP()->Window(0)) {
        // This is the main window, which never itself is a proxy,
        // but is the target of any active proxy.  Check all sub-windows.
        for (int i = 1; i < DSP_NUMWINS; i++) {
            if (DSP()->Window(i))
                DSP()->Window(i)->UpdateProxy();
        }
        return;
    }

    hyParent *pl = ProxyList();
    if (!pl) {
        Wbag()->SetLabelText(0);
        return;
    }
    sLstr lstr;
    lstr.add("    Proxy ");
    for (const hyParent *p = pl; p; p = p->next()) {
        CDp_name *pn = (CDp_name*)p->cdesc()->prpty(P_NAME);
        if (pn) {
            if (p != pl)
                lstr.add_c('.');
            bool copied;
            hyList *hp = pn->label_text(&copied, p->cdesc());
            char *s = hyList::string(hp, HYcvPlain, false);
            lstr.add(s);
            delete [] s;
            if (copied)
                hyList::destroy(hp);
        }
        else {
            lstr.free();
            break;
        }
    }
    hyParent::destroy(pl);
    Wbag()->SetLabelText(lstr.string());
}


// Clear the proxy link, update label.
//
void
WindowDesc::ClearProxy()
{
    if (this == DSP()->Window(0)) {
        // This is the main window, which never itself is a proxy,
        // but is the target of any active proxy.  Check all sub-windows.
        for (int i = 1; i < DSP_NUMWINS; i++) {
            if (DSP()->Window(i))
                DSP()->Window(i)->ClearProxy();
        }
        return;
    }
    delete w_proxy;
    w_proxy = 0;
    UpdateProxy();
}
// End WindowDesc functions.


// Clear the list for type.
//
void
sMK::clear(int type)
{
    if (type < 0)
        return;
    sMark::destroy(mark_heads[listnum(type)]);
    mark_heads[listnum(type)] = 0;
}


// Add list to the front of the existing list for type.
//
void
sMK::add_front(int type, sMark *list)
{
    if (list) {
        sMark *m = list;
        while (m->mNext)
            m = m->mNext;
        m->mNext = mark_heads[listnum(type)];
        mark_heads[listnum(type)] = list;
    }
}


// Add list to the back of the existing list for type.
//
void
sMK::add_back(int type, sMark *list)
{
    if (list) {
        if (!mark_heads[listnum(type)])
            mark_heads[listnum(type)] = list;
        else {
            sMark *m = mark_heads[listnum(type)];
            while (m->mNext)
                m = m->mNext;
            m->mNext = list;
        }
    }
}


void
sMK::install_terminal_marks(CDc *cdesc)
{
    if (cdesc) {
        CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
        CDgenRange rgen(pr);
        int vec_ix = 0;
        if (DSP()->TerminalsVisible() || DSP()->CurMode() == Electrical) {
            add_back(MARK_ETERM, sMark::new_elec_term_marks(cdesc, false));
            add_back(MARK_STERM, sMark::new_elec_term_marks(cdesc, true));
            add_back(MARK_BSC, sMark::new_elec_bterm_marks(cdesc, false));
            add_back(MARK_SYBSC, sMark::new_elec_bterm_marks(cdesc, true));
        }
        while (rgen.next(0)) {
            if (DSP()->TerminalsVisible() || DSP()->CurMode() == Electrical)
                add_back(MARK_PTERM, sMark::new_phys_term_marks(cdesc, vec_ix));
            CDs *msdesc = cdesc->masterCell();
            if (msdesc && !msdesc->isDevice())
                add_front(MARK_ILAB, sMark::new_inst_label(cdesc, vec_ix));
            vec_ix++;
        }
    }
    else {
        if (DSP()->TerminalsVisible() || DSP()->ContactsVisible() ||
                DSP()->CurMode() == Electrical) {
            add_front(MARK_PTERM, sMark::new_phys_term_marks(0, 0));
            add_front(MARK_ETERM, sMark::new_elec_term_marks(0, false));
            add_front(MARK_STERM, sMark::new_elec_term_marks(0, true));
            add_front(MARK_BSC, sMark::new_elec_bterm_marks(0, false));
            add_front(MARK_SYBSC, sMark::new_elec_bterm_marks(0, true));
        }
    }
}


// Remove and return marks of the given type.
//
sMark *
sMK::remove_marks(int type)
{
    if (type < 0)
        return (0);
    if (type < MARK_OBASE) {
        sMark *mm = mark_heads[type];
        mark_heads[type] = 0;
        return (mm);
    }
    sMark *m0 = 0, *mp = 0, *mn;
    int mtype = listnum(type);
    for (sMark *mm = mark_heads[mtype]; mm; mm = mn) {
        mn = mm->mNext;
        if (mm->mType == type) {
            if (!mp)
                mark_heads[mtype] = mn;
            else
                mp->mNext = mn;
            mm->mNext = m0;
            m0 = mm;
            continue;
        }
        mp = mm;
    }
    return (m0);
}


// Remove from the lists and return the terminal marks associated with
// cdesc.
//
sMark *
sMK::remove_terminal_marks(int type, CDc *cdesc, int vecix)
{
    sMark *m0 = 0;
    if (type == MARK_PTERM) {
        sMark *mp = 0, *mn;
        for (sMark *mm = MK.mark_heads[MARK_PTERM]; mm; mm = mn) {
            mn = mm->mNext;
            if (((sMark_Pterm*)mm)->mCdesc == cdesc) {
                if (!cdesc) {
                    if (!mp)
                        MK.mark_heads[MARK_PTERM] = mn;
                    else
                        mp->mNext = mn;
                    mm->mNext = m0;
                    m0 = mm;
                    continue;
                }
                if (((sMark_Pterm*)mm)->mTerm  &&
                        ((sMark_Pterm*)mm)->mTerm->inst_index() == vecix) {
                    if (!mp)
                        MK.mark_heads[MARK_PTERM] = mn;
                    else
                        mp->mNext = mn;
                    mm->mNext = m0;
                    m0 = mm;
                    continue;
                }
            }
            mp = mm;
        }
    }
    else if (type == MARK_ETERM) {
        // The highlighting eterm entries are handled
        // separately
        sMark *mp = 0, *mn;
        for (sMark *mm = MK.mark_heads[MARK_ETERM]; mm; mm = mn) {
            mn = mm->mNext;
            if (((sMark_Eterm*)mm)->mCdesc == cdesc &&
                    mm->mColor != HighlightingColor) {
                if (!mp)
                    MK.mark_heads[MARK_ETERM] = mn;
                else
                    mp->mNext = mn;
                mm->mNext = m0;
                m0 = mm;
                continue;
            }
            mp = mm;
        }
    }
    else if (type == MARK_STERM) {
        // The highlighting eterm entries are handled
        // separately
        sMark *mp = 0, *mn;
        for (sMark *mm = MK.mark_heads[MARK_STERM]; mm; mm = mn) {
            mn = mm->mNext;
            if (((sMark_Eterm*)mm)->mCdesc == cdesc &&
                    mm->mColor != HighlightingColor) {
                if (!mp)
                    MK.mark_heads[MARK_STERM] = mn;
                else
                    mp->mNext = mn;
                mm->mNext = m0;
                m0 = mm;
                continue;
            }
            mp = mm;
        }
    }
    else if (type == MARK_BSC) {
        // The highlighting bterm entries are handled
        // separately
        sMark *mp = 0, *mn;
        for (sMark *mm = MK.mark_heads[MARK_BSC]; mm; mm = mn) {
            mn = mm->mNext;
            if (((sMark_Bsc*)mm)->mCdesc == cdesc &&
                    mm->mColor != HighlightingColor) {
                if (!mp)
                    MK.mark_heads[MARK_BSC] = mn;
                else
                    mp->mNext = mn;
                mm->mNext = m0;
                m0 = mm;
                continue;
            }
            mp = mm;
        }
    }
    else if (type == MARK_SYBSC) {
        // The highlighting eterm entries are handled
        // separately
        sMark *mp = 0, *mn;
        for (sMark *mm = MK.mark_heads[MARK_SYBSC]; mm; mm = mn) {
            mn = mm->mNext;
            if (((sMark_Bsc*)mm)->mCdesc == cdesc &&
                    mm->mColor != HighlightingColor) {
                if (!mp)
                    MK.mark_heads[MARK_SYBSC] = mn;
                else
                    mp->mNext = mn;
                mm->mNext = m0;
                m0 = mm;
                continue;
            }
            mp = mm;
        }
    }
    return (m0);
}


// Purge any marks pointing at t, t is about to be deleted.
//
void
sMK::purge_terminal(CDterm *t)
{
    sMark_Pterm *mpp = 0, *mpnext;
    for (sMark_Pterm *m = (sMark_Pterm*)mark_heads[MARK_PTERM]; m;
            m = mpnext) {
        mpnext = (sMark_Pterm*)m->mNext;
        if (m->mTerm == t) {
            if (mpp)
                mpp->mNext = mpnext;
            else
                mark_heads[MARK_PTERM] = mpnext;
            continue;
        }
        mpp = m;
    }
}


// Display or erase all terminal marks.
//
void
sMK::display_terminal_marks(WindowDesc *wdesc, bool display)
{
    if (wdesc->Mode() == Physical) {
        if (DSP()->EraseBehindTerms() != ErbhNone) {
            MK.save_bound = true;
            for (sMark *mm = mark_heads[MARK_PTERM]; mm; mm = mm->mNext)
                mm->show(wdesc, display);
            for (sMark *mm = mark_heads[MARK_ILAB]; mm; mm = mm->mNext)
                mm->show(wdesc, display);
            MK.save_bound = false;
            MK.erase_behind(wdesc);
        }
        for (sMark *mm = mark_heads[MARK_PTERM]; mm; mm = mm->mNext)
            mm->show(wdesc, display);
        for (sMark *mm = mark_heads[MARK_ILAB]; mm; mm = mm->mNext)
            mm->show(wdesc, display);
    }
    else {
        for (sMark *mm = mark_heads[MARK_ETERM]; mm; mm = mm->mNext)
            mm->show(wdesc, display);
        for (sMark *mm = mark_heads[MARK_STERM]; mm; mm = mm->mNext)
            mm->show(wdesc, display);
        for (sMark *mm = mark_heads[MARK_BSC]; mm; mm = mm->mNext)
            mm->show(wdesc, display);
        for (sMark *mm = mark_heads[MARK_SYBSC]; mm; mm = mm->mNext)
            mm->show(wdesc, display);
    }
}


// Erase the area in the erase-behind list.
//
void
sMK::erase_behind(WindowDesc *wdesc)
{
    if (!wdesc->Wdraw())
        return;
    if (erbh_list) {
        erbh_list = Blist::merge(erbh_list);
        wdesc->Wdraw()->SetColor(DSP()->Color(BackgroundColor, Physical));
        for (Blist *bl = erbh_list; bl; bl = bl->next)
            wdesc->ShowBox(&bl->BB, CDL_FILLED, 0);
        Blist::destroy(erbh_list);
        erbh_list = 0;
    }
}


void
sMK::show_hlite(WindowDesc *wdesc)
{
    for (sMark *mm = hlite_list; mm; mm = mm->mNext) {
        switch (mm->mType) {
        default:
            mm->show(wdesc, DISPLAY);
            break;
        case MARK_ETERM:
        case MARK_STERM:
        case MARK_PTERM:
        case MARK_ILAB:
        case MARK_BSC:
        case MARK_SYBSC:
            if (mm->mColor == SelectColor || DSP()->ShowTerminals())
                mm->show(wdesc, DISPLAY);
            break;
        }
    }
    for (sMark *mm = contact_list; mm; mm = mm->mNext)
        mm->show(wdesc, DISPLAY);
    if (wdesc->IsSimilarNonSymbolic(DSP()->MainWdesc())) {
        for (sMwire *wl = wire_list; wl; wl = wl->next)
            wdesc->DisplaySelected(wl->wire);
    }
}


// Show or erase the instance box outlines.
//
void
sMK::show_BBs(WindowDesc *wdesc)
{
    if (!wdesc->Wdraw())
        return;
    if (wdesc->IsSimilar(DSP()->MainWdesc(), WDsimXmode)) {
        wdesc->Wdraw()->SetColor(DSP()->SelectPixel());
        Blist *b0 = (wdesc->Mode() == Physical ? phys_show : elec_show);
        for (Blist *b = b0; b; b = b->next)
            wdesc->ShowBox(&b->BB, 0, 0);
    }
}


// Erase and clear the instance box outlines.
//
void
sMK::clear_BBs()
{
    Blist *pl = phys_show;
    phys_show = 0;
    Blist *el = elec_show;
    elec_show = 0;
    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::ALL);
    while ((wdesc = wgen.next()) != 0) {
        if (wdesc->IsSimilar(DSP()->MainWdesc(), WDsimXmode)) {
            if (wdesc->Mode() == Physical) {
                for (Blist *b = pl; b; b = b->next)
                    sMark::refresh(wdesc, &b->BB);
            }
            else {
                for (Blist *b = el; b; b = b->next)
                    sMark::refresh(wdesc, &b->BB);
            }
        }
    }
    Blist::destroy(pl);
    Blist::destroy(el);
}
// End of sMK functions.


sMark::sMark()
{
    mType = 0;
    mMode = 0;
    mDelta = 0;
    mOrient = 0;
    mNext = 0;
    mX = mY = 0;
    mColor = 0;
    mAltColor = -1;
}


// Return the color pixel to use in rendering.
//
int
sMark::pixel(WindowDesc *wdesc)
{
    if (mInvisOverride && mColor == HighlightingColor)
        return (DSP()->Color(MarkerColor, wdesc->Mode()));

    if (mColor == SelectColor)
        return (DSP()->SelectPixel());
    if (mColor == MultiColor) {
        int ctab;
        if (mType < MARK_OBASE || mAltColor < 0)
            ctab = HighlightingColor;
        else
            ctab = Color2 + (mAltColor % 18);  // 18 plotting colors
        return (DSP()->Color(ctab, wdesc->Mode()));
    }
    return (DSP()->Color(mColor, wdesc->Mode()));
}


// Show a number enclosed in an outline figure, for plot and terminal
// marks.  If decimal, show the number, expanding outline as
// necessary.  Otherwise, show a single character, with fixed outline
// size.  The outline style is utilized only when there is no
// orientation given, otherwise an "arrow" shape is used.
//
void
sMark::gp_mark(WindowDesc *wdesc, int deltap, bool decimal,
    sMark::gp_style style)
{
    if (!wdesc->Wdraw())
        return;
    int delta = deltap/2;
    int x0 = mX;
    int y0 = mY;
    DSP()->TPoint(&x0, &y0);
    wdesc->LToP(x0, y0, x0, y0);
    int x1 = x0 + delta;
    x0 -= delta;
    int y1 = y0;
    int y2 = y0 + delta;
    y0 -= delta;

    // compensate for rotation in hardcopies
    bool up = false;
    HYorType orient = trans_mark((HYorType)mOrient);
    if (mOrient == HYorNone) {
        orient = trans_mark(HYorUp);
        if (orient != HYorUp)
            up = true;
        orient = HYorNone;
    }
    else if (orient != mOrient)
        up = true;

    double scale = delta/10.0;
    int xos = 0;
    int yos = (int)(2*scale);
    int marknum = mType - MARK_OBASE;
    char nbuf[64];
    if (decimal)
        mmItoA(nbuf, marknum);
    else {
        const char *mchars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ#";
        int tlen = strlen(mchars);
        if (marknum > tlen - 1)
            nbuf[0] = mchars[tlen-1];
        else
            nbuf[0] = mchars[marknum];
        nbuf[1] = '\0';
    }

    int wid, hei, numl;
    FT.textExtent(nbuf, &wid, &hei, &numl);
    wid = (int)(wid*scale);
    int d2 = delta + delta - 4;
    if (wid <= d2)
        xos = (d2 - wid)/2 + 2;
    else {
        wid >>= 1;
        if (wid < delta)
            wid = delta;
        if (!up) {
            x0 += delta - wid - 2;
            x1 -= delta - wid - 2;
        }
        else {
            y0 += delta - wid - 2;
            y1 -= delta - wid - 2;
        }
        xos = 2;
    }

    BBox AOI(x0, y2, x1, y0);
    int clr = pixel(wdesc);

    switch (orient) {
    case HYorNone:
        erase_box(wdesc, &AOI);
        wdesc->Wdraw()->SetColor(clr);
        if (style == gp_box) {
            show_line(wdesc, x0, y2, x1, y2);
            show_line(wdesc, x1, y2, x1, y0);
            show_line(wdesc, x1, y0, x0, y0);
            show_line(wdesc, x0, y0, x0, y2);
        }
        else if (style == gp_barrel) {
            int yc = (y0 + y2)/2;
            int d = delta/2;
            show_line(wdesc, x0+d, y2, x1-d, y2);
            show_line(wdesc, x1-d, y2, x1, yc);
            show_line(wdesc, x1, yc, x1-d, y0);
            show_line(wdesc, x0+d, y0, x1-d, y0);
            show_line(wdesc, x0+d, y0, x0, yc);
            show_line(wdesc, x0, yc, x0+d, y2);
        }
        else if (style == gp_ullr) {
            int yc = (y0 + y2)/2;
            int d = delta/2;
            show_line(wdesc, x0, y2, x1-d, y2);
            show_line(wdesc, x1-d, y2, x1, yc);
            show_line(wdesc, x1, yc, x1, y0);
            show_line(wdesc, x0+d, y0, x1, y0);
            show_line(wdesc, x0+d, y0, x0, yc);
            show_line(wdesc, x0, yc, x0, y2);
        }
        else if (style == gp_diam) {
            int xm = (x0 + x1)/2;
            int ym = (y0 + y2)/2;
            show_line(wdesc, x0, ym, xm, y0);
            show_line(wdesc, xm, y0, x1, ym);
            show_line(wdesc, x1, ym, xm, y2);
            show_line(wdesc, xm, y2, x0, ym);
        }
        else if (style == gp_oct) {
            int xm = (x0 + x1)/2;
            int dx = (x1 - x0)/4;
            int dy = (y2 - y0)/4;
            show_line(wdesc, x0, y1-dy, x0, y1+dy);
            show_line(wdesc, x0, y1+dy, xm-dx, y2);
            show_line(wdesc, xm-dx, y2, xm+dx, y2);
            show_line(wdesc, xm+dx, y2, x1, y1+dy);
            show_line(wdesc, x1, y1+dy, x1, y1-dy);
            show_line(wdesc, x1, y1-dy, xm+dx, y0);
            show_line(wdesc, xm+dx, y0, xm-dx, y0);
            show_line(wdesc, xm-dx, y0, x0, y1-dy);
        }
        break;
    case HYorUp: // up
        AOI.top -= delta;
        erase_box(wdesc, &AOI);
        wdesc->Wdraw()->SetColor(clr);
        show_line(wdesc, x0, y2, x1, y2);
        show_line(wdesc, x1, y2, x1, y0);
        show_line(wdesc, x1, y0, (x0+x1)/2, y0-delta);
        show_line(wdesc, (x0+x1)/2, y0-delta, x0, y0);
        show_line(wdesc, x0, y0, x0, y2);
        break;
    case HYorRt: // right
        AOI.right += delta;
        erase_box(wdesc, &AOI);
        wdesc->Wdraw()->SetColor(clr);
        show_line(wdesc, x0, y2, x1, y2);
        show_line(wdesc, x1, y2, x1+delta, y1);
        show_line(wdesc, x1+delta, y1, x1, y0);
        show_line(wdesc, x1, y0, x0, y0);
        show_line(wdesc, x0, y0, x0, y2);
        break;
    case HYorDn: // down
        AOI.bottom += delta;
        erase_box(wdesc, &AOI);
        wdesc->Wdraw()->SetColor(clr);
        show_line(wdesc, x0, y2, (x0+x1)/2, y2+delta);
        show_line(wdesc, (x0+x1)/2, y2+delta, x1, y2);
        show_line(wdesc, x1, y2, x1, y0);
        show_line(wdesc, x1, y0, x0, y0);
        show_line(wdesc, x0, y0, x0, y2);
        break;
    case HYorLt: // left
        AOI.left -= delta;
        erase_box(wdesc, &AOI);
        wdesc->Wdraw()->SetColor(clr);
        show_line(wdesc, x0, y2, x1, y2);
        show_line(wdesc, x1, y2, x1, y0);
        show_line(wdesc, x1, y0, x0, y0);
        show_line(wdesc, x0, y0, x0-delta, y1);
        show_line(wdesc, x0-delta, y1, x0, y2);
        break;
    }
    // Some hard-coded assumptions: pixel width of box is 20,
    // character cell is 8x14.
    //
    wdesc->Wdraw()->SetColor(DSP()->Color(HighlightingColor,
        wdesc->Mode()));
    if (!up)
        wdesc->ViewportText(nbuf, x0 + xos, y2 - yos, scale, false);
    else
        wdesc->ViewportText(nbuf, x0 + yos, y2 - xos, scale, true);
}


// Static function.
// Erase AOI in the viewport.
//
void
sMark::erase_box(WindowDesc *wdesc, BBox *AOI)
{
    if (!wdesc->Wdraw())
        return;
    BBox BB = *AOI;
    if (ViewportIntersect(BB, *wdesc->ClipRect())) {
        ViewportClip(BB, *wdesc->ClipRect());
        wdesc->Wdraw()->SetFillpattern(0);
        wdesc->Wdraw()->SetColor(DSP()->Color(BackgroundColor, wdesc->Mode()));
        wdesc->Wdraw()->Box(BB.left, BB.bottom, BB.right, BB.top);
    }
}


// Static function.
// Draw a line in viewport coords, clipped.
//
void
sMark::show_line(WindowDesc *wdesc, int x1, int y1, int x2, int y2)
{
    if (!wdesc->Wdraw())
        return;
    if (!cGEO::line_clip(&x1, &y1, &x2, &y2, wdesc->ClipRect()))
        wdesc->Wdraw()->Line(x1, y1, x2, y2);
}


// Static function.
// Transform the orientation of the mark according to the current
// active transformation.
//
HYorType
sMark::trans_mark(HYorType orient)
{
    int x, y;
    switch (orient) {
        default:
        case HYorNone:
            return (HYorNone);
        case HYorDn:
            x = 0;
            y = -1;
            break;
        case HYorRt:
            x = 1;
            y = 0;
            break;
        case HYorUp:
            x = 0;
            y = 1;
            break;
        case HYorLt:
            x = -1;
            y = 0;
            break;
    }
    int x1 = 0;
    int y1 = 0;
    DSP()->TPoint(&x, &y);
    DSP()->TPoint(&x1, &y1);
    x -= x1;
    y -= y1;

    if (x == 0) {
        if (y == 1)
            orient = HYorUp;
        else
            orient = HYorDn;
    }
    else {
        if (x == 1)
            orient = HYorRt;
        else
            orient = HYorLt;
    }
    return (orient);
}


// Static function.
sMark *
sMark::new_phys_term_marks(CDc *cdesc, int vecix)
{
    sMark *mp0 = 0;
    if (!cdesc)
        mp0 = new_term_marks(Physical, 0, 0, false);
    else if (cdesc->type() == CDINSTANCE) {
        CDs *msdesc = cdesc->masterCell();
        if (!msdesc)
            return (0);
        if (msdesc->isDevice() || cdesc->findPhysDualOfElec(vecix, 0, 0))
            mp0 = new_term_marks(Physical, cdesc, vecix, false);
    }
    return (mp0);
}


// Static function.
sMark *
sMark::new_elec_term_marks(CDc *cdesc, bool symbolic)
{
    sMark *m0 = 0;
    if (!cdesc) {
        // Don't add electrical top level connections if the
        // "subct" command is active.
        if (!DSP()->is_subct_cmd_active())
            m0 = new_term_marks(Electrical, 0, 0, symbolic);
    }
    else if (!symbolic && cdesc->type() == CDINSTANCE) {
        CDs *sdesc = CurCell(Electrical, true);
        if (sdesc)
            m0 = new_term_marks(Electrical, cdesc, 0, false);
    }
    return (m0);
}


// Static function.
//
sMark *
sMark::new_term_marks(DisplayMode mode, CDc *cdesc, int vecix, bool symbolic)
{
    sMark *m0 = 0;
    if (!cdesc) {
        CDs *sdesc = CurCell(Electrical, true);
        if (!sdesc)
            return (0);
        if (symbolic && !sdesc->symbolicRep(0))
            return (0);
        CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            if (mode == Electrical) {
                if (symbolic) {
                    for (unsigned int ix = 0; ; ix++) {
                        int x, y;
                        if (!pn->get_pos(ix, &x, &y))
                            break;
                        m0 = new sMark_Eterm(x, y, 40, MarkerColor,
                            pn->index(), 0, pn->term_name()->string(),
                            pn->term_flags(), true, false, m0);
                    }
                }
                else {
                    int x, y;
                    pn->get_schem_pos(&x, &y);
                    m0 = new sMark_Eterm(x, y, 40, MarkerColor,
                        pn->index(), 0, pn->term_name()->string(),
                        pn->term_flags(), false, false, m0);
                }
            }
            else {
                CDterm *term = pn->cell_terminal();
                if (!term)
                    continue;
                m0 = new sMark_Pterm(term, HighlightingColor,
                    DSP()->TermMarkSize(), m0);
            }
        }
    }
    else {
        CDs *sdesc = cdesc->masterCell();
        if (!sdesc)
            return (0);
        if (mode == Electrical && sdesc->isDevice())
            return (0);
        CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
        CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            CDp_cnode *pn1;
            if (vecix > 0 && pr)
                pn1 = pr->node(0, vecix, pn->index());
            else
                pn1 = pn;
            if (!pn1)
                continue;
            if (mode == Electrical) {
                for (unsigned int ix = 0; ; ix++) {
                    int x, y;
                    if (!pn1->get_pos(ix, &x, &y))
                        break;
                    m0 = new sMark_Eterm(x, y, 40, MarkerColor,
                        pn1->index(), cdesc, pn1->term_name()->string(),
                        pn1->term_flags(), false, false, m0);
                }
            }
            else {
                CDterm *term = pn1->inst_terminal();
                if (!term)
                    continue;
                m0 = new sMark_Pterm(term, MarkerColor,
                    DSP()->TermMarkSize(), m0);
            }
        }
    }
    return (m0);
}


// Static function.
sMark *
sMark::new_elec_bterm_marks(CDc *cdesc, bool symbolic)
{
    sMark *m0 = 0;
    if (!cdesc) {
        // Don't add electrical top level connections if the
        // "subct" command is active.
        if (!DSP()->is_subct_cmd_active())
            m0 = new_bterm_marks(Electrical, 0, symbolic);
    }
    else if (!symbolic && cdesc->type() == CDINSTANCE) {
        CDs *sdesc = CurCell(Electrical, true);
        if (sdesc)
            m0 = new_bterm_marks(Electrical, cdesc, false);
    }
    return (m0);
}


// Static function.
sMark *
sMark::new_bterm_marks(int mode, CDc *cdesc, bool symbolic)
{
    if (mode != Electrical)
        return (0);
    sMark *m0 = 0;
    if (!cdesc) {
        CDs *sdesc = CurCell(Electrical, true);
        if (!sdesc)
            return (0);
        if (symbolic && !sdesc->symbolicRep(0))
            return (0);
        CDp_bsnode *pn = (CDp_bsnode*)sdesc->prpty(P_BNODE);

        for ( ; pn; pn = pn->next()) {
            sLstr lstr;
            pn->add_label_text(&lstr);
            if (symbolic) {
                for (unsigned int ix = 0; ; ix++) {
                    int x, y;
                    if (!pn->get_pos(ix, &x, &y))
                        break;
                    m0 = new sMark_Bsc(x, y, 40, MarkerColor,
                        pn->beg_range(), pn->end_range(), pn->index(),
                        cdesc, lstr.string(), pn->flags(), true, m0);
                }
            }
            else {
                int x, y;
                pn->get_schem_pos(&x, &y);
                m0 = new sMark_Bsc(x, y, 40, MarkerColor,
                    pn->beg_range(), pn->end_range(), pn->index(),
                    cdesc, lstr.string(), pn->flags(), false, m0);
            }
        }
    }
    else {
        CDs *sdesc = cdesc->masterCell();
        if (!sdesc)
            return (0);
        if (mode == Electrical && sdesc->isDevice())
            return (0);
        CDp_bcnode *pn = (CDp_bcnode*)cdesc->prpty(P_BNODE);

        for ( ; pn; pn = pn->next()) {
            sLstr lstr;
            pn->add_label_text(&lstr);
            for (unsigned int ix = 0; ; ix++) {
                int x, y;
                if (!pn->get_pos(ix, &x, &y))
                    break;
                m0 = new sMark_Bsc(x, y, 40, MarkerColor,
                    pn->beg_range(), pn->end_range(), pn->index(),
                    cdesc, lstr.string(), pn->flags(), false, m0);
            }
        }
    }
    return (m0);
}


// Static function.
sMark *
sMark::new_inst_label(const CDc *cdesc, int vecix)
{
    if (!cdesc || cdesc->type() != CDINSTANCE)
        return (0);
    CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
    CDp_name *pn;
    if (vecix > 0 && pr)
        pn = pr->name_prp(0, vecix);
    else
        pn = (CDp_name*)cdesc->prpty(P_NAME);
    if (!pn)
        return (0);
    sMark *m0 = new sMark_Ilab(cdesc, vecix, HighlightingColor,
        DSP()->TermMarkSize(), 0);
    return (m0);
}
// End of sMark functions.


sMark_Cross::sMark_Cross(int m, int x, int y, int del, int clr, int type,
    const char *str_u, const char *str_l, sMark *nx)
{
    mType = MARK_CROSS;
    mMode = m;
    mDelta = del;
    mOrient = 0;
    mNext = nx;
    mX = x;
    mY = y;
    mColor = clr;
    mSubType = type;
    mStrU = lstring::copy(str_u);
    mStrL = lstring::copy(str_l);
}


void
sMark_Cross::show(WindowDesc *wdesc, bool display)
{
    if (!wdesc->Wdraw())
        return;
    if (!wdesc->IsSimilar((DisplayMode)mMode, DSP()->MainWdesc()))
        return;
    cross_mark(wdesc, display ? DISPLAY : ERASE);
}


void
sMark_Cross::addBB(WindowDesc *wdesc, BBox *BB)
{
    if (!wdesc->IsSimilar((DisplayMode)mMode, DSP()->MainWdesc()))
        return;

    int x = mX;
    int y = mY;
    DSP()->TPoint(&x, &y);

    int delta, phei;
    if (wdesc->Ratio() < 0.1) {
        delta = mmRnd(mDelta/wdesc->Ratio());
        phei = mmRnd(DSP()->TermTextSize()/wdesc->Ratio());
    }
    else {
        delta = wdesc->LogScale(mDelta);
        phei = wdesc->LogScale(DSP()->TermTextSize());
        if (phei < FT.cellHeight())
            phei = FT.cellHeight();
    }

    BBox tBB(x-delta, y-delta, x+delta, y+delta);
    int wn, wl, h;
    int d4 = delta/4;
    if (mStrU) {
        DSP()->DefaultLabelSize(mStrU, wdesc->Mode(), &wn, &h);
        wn = mmRnd((wn*(double)phei)/h);
        h = phei;
        if (y + d4 + h > tBB.top)
            tBB.top = y + d4 + h;
        if (x + d4 + wn > tBB.right)
            tBB.right = x + d4 + wn;
    }
    if (mStrL) {
        DSP()->DefaultLabelSize(mStrL, wdesc->Mode(), &wl, &h);
        wl = mmRnd((wl*(double)phei)/h);
        h = phei;
        if (y - d4 - h < tBB.bottom)
            tBB.bottom = y - d4 - h;
        if (x + d4 + wl > tBB.right)
            tBB.right = x + d4 + wl;
    }
    BB->add(&tBB);
}


void
sMark_Cross::cross_mark(WindowDesc *wdesc, bool display)
{
    int x = mX;
    int y = mY;
    DSP()->TPoint(&x, &y);

    int delta, phei;
    if (wdesc->Ratio() < 0.1) {
        delta = mmRnd(mDelta/wdesc->Ratio());
        phei = mmRnd(DSP()->TermTextSize()/wdesc->Ratio());
    }
    else {
        delta = wdesc->LogScale(mDelta);
        int dmax = 2*mmRnd(mDelta/wdesc->Ratio());
        if (delta >dmax)
            delta = dmax;

        phei = wdesc->LogScale(DSP()->TermTextSize());
        if (phei < FT.cellHeight())
            phei = FT.cellHeight();
    }

    BBox BB(x-delta, y-delta, x+delta, y+delta);
    int wn, wl, h;
    int d4 = delta/4;
    if (mStrU) {
        DSP()->DefaultLabelSize(mStrU, wdesc->Mode(), &wn, &h);
        wn = mmRnd((wn*(double)phei)/h);
        h = phei;
        if (y + d4 + h > BB.top)
            BB.top = y + d4 + h;
        if (x + d4 + wn > BB.right)
            BB.right = x + d4 + wn;
    }
    if (mStrL) {
        DSP()->DefaultLabelSize(mStrL, wdesc->Mode(), &wl, &h);
        wl = mmRnd((wl*(double)phei)/h);
        h = phei;
        if (y - d4 - h < BB.bottom)
            BB.bottom = y - d4 - h;
        if (x + d4 + wl > BB.right)
            BB.right = x + d4 + wl;
    }
    if (display) {
        wdesc->Wdraw()->SetColor(pixel(wdesc));
        wdesc->ShowLine(x-delta, y, x+delta, y);
        wdesc->ShowLine(x, y-delta, x, y+delta);
        if (mStrU)
            wdesc->ShowLabel(mStrU, x + d4, y + d4, wn, h, 0);
        if (mStrL)
            wdesc->ShowLabel(mStrL, x + d4, y - d4 - h, wl, h, 0);
    }
    else
        refresh(wdesc, &BB);
}
// End of sMark_Cross functions.


sMark_Box::sMark_Box(int m, int x, int y, int del, int clr, sMark *nx,
    int st, sMark::mkDspType dt)
{
    mType = MARK_BOX;
    mMode = m;
    mDelta = del;
    mOrient = 0;
    mNext = nx;
    mX = x;
    mY = y;
    mColor = clr;
    mSubType = st;
    mShowNode = dt;
}


void
sMark_Box::show(WindowDesc *wdesc, bool display)
{
    if (!wdesc->Wdraw())
        return;
    if (mShowNode != sMark::mkDspAll) {
        if (!wdesc->IsSimilar(Electrical, DSP()->MainWdesc(), WDsimXsymb))
            return;
        CDs *sd = wdesc->CurCellDesc(Electrical);
        if (!sd)
            return;
        if (sd->isSymbolic()) {
            if (mShowNode == sMark::mkDspMain)
                return;
        }
        else {
            if (mShowNode == sMark::mkDspSymb)
                return;
        }
    }
    else {
        if (!wdesc->IsSimilar((DisplayMode)mMode, DSP()->MainWdesc()))
            return;
    }

    int x = mX;
    int y = mY;
    DSP()->TPoint(&x, &y);
    wdesc->LToP(x, y, x, y);
    int delta = wdesc->LogScaleToPix(mDelta/2);

    if (display) {
        wdesc->Wdraw()->SetColor(pixel(wdesc));
        show_line(wdesc, x-delta, y+delta, x+delta, y+delta);
        show_line(wdesc, x+delta, y+delta, x+delta, y-delta);
        show_line(wdesc, x+delta, y-delta, x-delta, y-delta);
        show_line(wdesc, x-delta, y-delta, x-delta, y+delta);
    }
    else {
        BBox BB(x - delta, y + delta, x + delta, y - delta);
        wdesc->Update(&BB);
    }
}


void
sMark_Box::addBB(WindowDesc *wdesc, BBox *BB)
{
    if (!wdesc->Wdraw())
        return;
    if (mShowNode != sMark::mkDspAll) {
        if (!wdesc->IsSimilar(Electrical, DSP()->MainWdesc(), WDsimXsymb))
            return;
        CDs *sd = wdesc->CurCellDesc(Electrical);
        if (!sd)
            return;
        if (sd->isSymbolic()) {
            if (mShowNode == sMark::mkDspMain)
                return;
        }
        else {
            if (mShowNode == sMark::mkDspSymb)
                return;
        }
    }
    else {
        if (!wdesc->IsSimilar((DisplayMode)mMode, DSP()->MainWdesc()))
            return;
    }

    int x = mX;
    int y = mY;
    DSP()->TPoint(&x, &y);
    wdesc->LToP(x, y, x, y);
    int delta = wdesc->LogScaleToPix(mDelta/2);
    delta = mmRnd(delta/wdesc->Ratio());
    BBox tBB(x-delta, y-delta, x+delta, y+delta);
    BB->add(&tBB);
}
// End of sMark_Box functions.


sMark_Arrow::sMark_Arrow(int m, int x, int y, int del, int ori, int clr,
    sMark *nx)
{
    mType = MARK_ARROW;
    mMode = m;
    mDelta = del;
    mOrient = ori;
    mNext = nx;
    mX = x;
    mY = y;
    mColor = clr;
}


void
sMark_Arrow::show(WindowDesc *wdesc, bool display)
{
    if (!wdesc->Wdraw())
        return;
    if (!wdesc->IsSimilar((DisplayMode)mMode, DSP()->MainWdesc()))
        return;

    int x = mX;
    int y = mY;
    DSP()->TPoint(&x, &y);
    wdesc->LToP(x, y, x, y);
    int delta = wdesc->LogScaleToPix(mDelta/2);

    if (display) {
        wdesc->Wdraw()->SetColor(pixel(wdesc));
        switch (mOrient) {
        default:
        case HYorUp:
            show_line(wdesc, x, y-delta, x, y+delta);
            show_line(wdesc, x-delta, y, x, y+delta);
            show_line(wdesc, x+delta, y, x, y+delta);
            break;
        case HYorRt:
            show_line(wdesc, x-delta, y, x+delta, y);
            show_line(wdesc, x, y+delta, x+delta, y);
            show_line(wdesc, x, y-delta, x+delta, y);
            break;
        case HYorDn:
            show_line(wdesc, x, y-delta, x, y+delta);
            show_line(wdesc, x-delta, y, x, y-delta);
            show_line(wdesc, x+delta, y, x, y-delta);
            break;
        case HYorLt:
            show_line(wdesc, x-delta, y, x+delta, y);
            show_line(wdesc, x, y+delta, x-delta, y);
            show_line(wdesc, x, y-delta, x-delta, y);
            break;
        }
    }
    else {
        BBox BB(x - delta, y + delta, x + delta, y - delta);
        wdesc->Update(&BB);
    }
}


void
sMark_Arrow::addBB(WindowDesc *wdesc, BBox *BB)
{
    if (!wdesc->Wdraw())
        return;
    if (!wdesc->IsSimilar((DisplayMode)mMode, DSP()->MainWdesc()))
        return;

    int x = mX;
    int y = mY;
    DSP()->TPoint(&x, &y);
    wdesc->LToP(x, y, x, y);
    int delta = wdesc->LogScaleToPix(mDelta/2);
    delta = mmRnd(delta/wdesc->Ratio());
    BBox tBB(x-delta, y-delta, x+delta, y+delta);
    BB->add(&tBB);
}
// End of sMark_Arrow functions.


sMark_Eterm::sMark_Eterm(int x, int y, int del, int clr, unsigned int trm,
    const CDc *cd, const char *tname, unsigned int vflags, bool symb,
    bool nobox, sMark *nx)
{
    mType = symb ? MARK_STERM : MARK_ETERM;
    mMode = Electrical;
    mDelta = del;
    mOrient = 0;
    mNext = nx;
    mX = x;
    mY = y;
    mColor = clr;

    mCdesc = cd;
    mTname = lstring::copy(tname);
    mTermnum = trm;
    mVflags = vflags;
    mQuad = 1;
    mSymbolic = symb;
    mUnboxed = nobox;
}


void
sMark_Eterm::show(WindowDesc *wdesc, bool display)
{
    if (!wdesc->Wdraw())
        return;
    if (!wdesc->IsSimilar((DisplayMode)mMode, DSP()->MainWdesc(), WDsimXsymb))
        return;
    if (wdesc->Mode() != Electrical)
        return;

    // Make sure symbolic mode matches window.
    CDs *sd = wdesc->CurCellDesc(Electrical);
    if (!sd)
        return;
    if (sd->isSymbolic() != mSymbolic)
        return;

    if (display) {
        if (!mCdesc) {
            // The cell's terminals, allow overriding of invisibility
            // for editing.

            if ((!mSymbolic && (mVflags & TE_SCINVIS)) ||
                    (mSymbolic && (mVflags & TE_SYINVIS))) {
                if (!DSP()->ShowInvisMarks())
                    return;
                mInvisOverride = true;
            }
            wdesc->Wdraw()->SetColor(pixel(wdesc));
            eterm_mark(wdesc, DISPLAY);
            mInvisOverride = false;
        }
        else {
            // Instance terminals.

            CDs *msdesc = mCdesc->masterCell();
            if (!msdesc)
                return;
            if (msdesc->isSymbolic()) {
                if (mVflags & TE_SYINVIS)
                    return;
            }
            else {
                if (mVflags & TE_SCINVIS)
                    return;
            }
            wdesc->Wdraw()->SetColor(pixel(wdesc));
            eterm_mark(wdesc, DISPLAY);
        }
    }
    else
        eterm_mark(wdesc, ERASE);
}


void
sMark_Eterm::addBB(WindowDesc *wdesc, BBox *BB)
{
    if (!wdesc->Wdraw())
        return;
    if (!wdesc->IsSimilar((DisplayMode)mMode, DSP()->MainWdesc(), WDsimXsymb))
        return;
    if (wdesc->Mode() != Electrical)
        return;

    // Make sure symbolic mode matches window.
    CDs *sd = wdesc->CurCellDesc(Electrical);
    if (!sd)
        return;
    if (sd->isSymbolic() != mSymbolic)
        return;

    if (!mCdesc) {
        // The cell's terminals, allow overriding of invisibility
        // for editing.

        if ((!mSymbolic && (mVflags & TE_SCINVIS)) ||
                (mSymbolic && (mVflags & TE_SYINVIS))) {
            if (!DSP()->ShowInvisMarks())
                return;
        }
    }
    else {
        // Instance terminals.

        CDs *msdesc = mCdesc->masterCell();
        if (!msdesc)
            return;
        if (msdesc->isSymbolic()) {
            if (mVflags & TE_SYINVIS)
                return;
        }
        else {
            if (mVflags & TE_SCINVIS)
                return;
        }
    }

    int delta = wdesc->LogScale(DSP()->TermMarkSize());
    int phei = wdesc->LogScale(DSP()->TermTextSize());
    if (phei < FT.cellHeight())
        phei = FT.cellHeight();

    // be clever about coincident marks
    if (mCdesc || mUnboxed)
        setQuad();

    BBox tBB(mX-delta, mY-delta, mX+delta, mY+delta);
    int w, h;
    int d4 = delta/4;
    if (mTname && !CDnetex::is_default_name(mTname)) {
        DSP()->DefaultLabelSize(mTname, wdesc->Mode(), &w, &h);
        w = mmRnd((w*(double)phei)/h);
        h = phei;
        if (!mCdesc && !mUnboxed) {
            // Show the cell top level terminals as boxed number.
            if (mX + delta + d4 + w > tBB.right)
                tBB.right = mX + delta + d4 + w;
            if (mY - d4 - h < tBB.bottom)
                tBB.bottom = mY - d4 - h;
        }
        else {
            if (mQuad == 0) {
                if (mX - d4 - w < tBB.left)
                    tBB.left = mX - d4 - w;
                if (mY + d4 + h > tBB.top)
                    tBB.top = mY + d4 + h;
            }
            else if (mQuad == 2) {
                if (mX + d4 + w > tBB.right)
                    tBB.right = mX + d4 + w;
                if (mY - d4 - h < tBB.bottom)
                    tBB.bottom = mY - d4 - h;
            }
            else if (mQuad == 3) {
                if (mX - d4 - w < tBB.left)
                    tBB.left = mX - d4 - w;
                if (mY - d4 - h < tBB.bottom)
                    tBB.bottom = mY - d4 - h;
            }
            else {
                if (mX + d4 + w > tBB.right)
                    tBB.right = mX + d4 + w;
                if (mY + d4 + h > tBB.top)
                    tBB.top = mY + d4 + h;
            }
        }
    }
    if (!mCdesc && !mUnboxed) {
        int i = 0;
        int j = 10;
        while (mTermnum/j) {
            j *= 10;
            i++;
        }
        if (i) {
            tBB.left -= i*delta;
            tBB.right += i*delta;
        }
    }
    BB->add(&tBB);
}


// Look for coincident marks, and set the mQuad field.
//
void
sMark_Eterm::setQuad()
{
    int count = 0;
    for (sMark *m = MK.mark_heads[mType]; m && m != this; m = m->mNext) {
        if (m->mX == mX && m->mY == mY)
            count++;
    }
    switch (count) {
    case 1:
        mQuad = 3;
        break;
    case 2:
        mQuad = 2;
        break;
    case 3:
        mQuad = 0;
        break;
    }
}


// Render a terminal for electrical mode.
//
void
sMark_Eterm::eterm_mark(WindowDesc *wdesc, bool display)
{
    int delta = wdesc->LogScale(DSP()->TermMarkSize());
    int phei = wdesc->LogScale(DSP()->TermTextSize());
    if (phei < FT.cellHeight())
        phei = FT.cellHeight();

    // be clever about coincident marks
    if (display == DISPLAY && (mCdesc || mUnboxed))
        setQuad();

    BBox BB(mX-delta, mY-delta, mX+delta, mY+delta);
    int w, h;
    int d4 = delta/4;
    bool show_name = false;
    if (mTname && !CDnetex::is_default_name(mTname)) {
        DSP()->DefaultLabelSize(mTname, wdesc->Mode(), &w, &h);
        w = mmRnd((w*(double)phei)/h);
        h = phei;
        if (!mCdesc && !mUnboxed) {
            // Show the cell top level terminals as boxed number.
            if (mX + delta + d4 + w > BB.right)
                BB.right = mX + delta + d4 + w;
            if (mY - d4 - h < BB.bottom)
                BB.bottom = mY - d4 - h;
        }
        else {
            if (mQuad == 0) {
                if (mX - d4 - w < BB.left)
                    BB.left = mX - d4 - w;
                if (mY + d4 + h > BB.top)
                    BB.top = mY + d4 + h;
            }
            else if (mQuad == 2) {
                if (mX + d4 + w > BB.right)
                    BB.right = mX + d4 + w;
                if (mY - d4 - h < BB.bottom)
                    BB.bottom = mY - d4 - h;
            }
            else if (mQuad == 3) {
                if (mX - d4 - w < BB.left)
                    BB.left = mX - d4 - w;
                if (mY - d4 - h < BB.bottom)
                    BB.bottom = mY - d4 - h;
            }
            else {
                if (mX + d4 + w > BB.right)
                    BB.right = mX + d4 + w;
                if (mY + d4 + h > BB.top)
                    BB.top = mY + d4 + h;
            }
        }
        show_name = true;
    }
    if (display == DISPLAY) {
        if (!mCdesc && !mUnboxed) {
            // Show the cell top level terminals as boxed number.
            int type = mType;
            mType = MARK_OBASE + mTermnum;
            sMark::gp_style style = sMark::gp_box;
            if (mVflags & TE_BYNAME)
                style = sMark::gp_barrel;
            gp_mark(wdesc, (int)(2*delta*wdesc->Ratio()), true, style);
            mType = type;
            if (show_name)
                wdesc->ShowLabel(mTname, mX + delta + d4, mY - d4 - h,
                    w, h, 0);
        }
        else {
            wdesc->ShowLine(mX-delta, mY, mX+delta, mY);
            wdesc->ShowLine(mX, mY-delta, mX, mY+delta);
            if (show_name) {
                int x, y;
                if (mQuad == 0) {
                    x = mX - d4 - w;
                    y = mY + d4;
                }
                else if (mQuad == 2) {
                    x = mX + d4;
                    y = mY - d4 - h;
                }
                else if (mQuad == 3) {
                    x = mX - d4 - w;
                    y = mY - d4 - h;
                }
                else {
                    x = mX + d4;
                    y = mY + d4;
                }
                wdesc->ShowLabel(mTname, x, y, w, h, 0);
            }
        }
    }
    else {
        if (!mCdesc && !mUnboxed) {
            int i = 0;
            int j = 10;
            while (mTermnum/j) {
                j *= 10;
                i++;
            }
            if (i) {
                BB.left -= i*delta;
                BB.right += i*delta;
            }
        }
        refresh(wdesc, &BB);
    }
}
// End of sMark_Eterm functions.


sMark_Pterm::sMark_Pterm(const CDterm *t, int clr, int size, sMark *nx)
{
    mType = MARK_PTERM;
    mMode = Physical;
    mDelta = size;
    mOrient = 0;
    mNext = nx;
    mX = t ? t->lx() : 0;
    mY = t ? t->ly() : 0;
    mColor = clr;

    mCdesc = t ? t->instance() : 0;
    mTerm = t;
}


void
sMark_Pterm::show(WindowDesc *wdesc, bool display)
{
    if (!wdesc->IsSimilar((DisplayMode)mMode, DSP()->MainWdesc()))
        return;

    if (display) {
        if (wdesc->Mode() == Physical)
            pterm_mark(wdesc, DISPLAY);
    }
    else {
        if (wdesc->Mode() == Physical)
            pterm_mark(wdesc, ERASE);
    }
}


void
sMark_Pterm::addBB(WindowDesc *wdesc, BBox *BB)
{
    if (!wdesc->IsSimilar((DisplayMode)mMode, DSP()->MainWdesc()))
        return;
    if (wdesc->Mode() != Physical)
        return;
    if (!mTerm)
        return;

    int x = mTerm->lx();
    int y = mTerm->ly();
    int delta = wdesc->LogScale(mTerm->instance() ? (4*mDelta)/5 : mDelta);
    int phei = wdesc->LogScale(DSP()->TermTextSize());
    if (phei < FT.cellHeight())
        phei = FT.cellHeight();

    BBox tBB(x-delta, y-delta, x+delta, y+delta);
    int wn, wl, h;
    int d4 = delta/4;
    if (mTerm->name()) {
        DSP()->DefaultLabelSize(mTerm->name()->string(), wdesc->Mode(),
            &wn, &h);
        wn = mmRnd((wn*(double)phei)/h);
        h = phei;
        if (y + d4 + h > tBB.top)
            tBB.top = y + d4 + h;
        if (x + d4 + wn > tBB.right)
            tBB.right = x + d4 + wn;
    }
    CDl *ld = mTerm->layer();
    if (ld) {
        DSP()->DefaultLabelSize(ld->name(), wdesc->Mode(), &wl, &h);
        wl = mmRnd((wl*(double)phei)/h);
        h = phei;
        if (y - d4 - h < tBB.bottom)
            tBB.bottom = y - d4 - h;
        if (x + d4 + wl > tBB.right)
            tBB.right = x + d4 + wl;
    }
    BB->add(&tBB);
}


void
sMark_Pterm::pterm_mark(WindowDesc *wdesc, bool display)
{
    if (wdesc->Mode() != Physical)
        return;
    if (!mTerm)
        return;

    int x = mTerm->lx();
    int y = mTerm->ly();
    int delta = wdesc->LogScale(mTerm->instance() ? (4*mDelta)/5 : mDelta);
    int phei = wdesc->LogScale(DSP()->TermTextSize());
    if (phei < FT.cellHeight())
        phei = FT.cellHeight();

    BBox BB(x-delta, y-delta, x+delta, y+delta);
    int wn, wl, h;
    int d4 = delta/4;
    bool show_name = false;
    if (mTerm->name()) {
        DSP()->DefaultLabelSize(mTerm->name()->string(), wdesc->Mode(),
            &wn, &h);
        wn = mmRnd((wn*(double)phei)/h);
        h = phei;
        if (y + d4 + h > BB.top)
            BB.top = y + d4 + h;
        if (x + d4 + wn > BB.right)
            BB.right = x + d4 + wn;
        show_name = true;
    }
    CDl *ld = mTerm->layer();
    if (ld) {
        DSP()->DefaultLabelSize(ld->name(), wdesc->Mode(), &wl, &h);
        wl = mmRnd((wl*(double)phei)/h);
        h = phei;
        if (y - d4 - h < BB.bottom)
            BB.bottom = y - d4 - h;
        if (x + d4 + wl > BB.right)
            BB.right = x + d4 + wl;
    }

    if (MK.save_bound) {
        if (DSP()->EraseBehindTerms() == ErbhAll ||
                (DSP()->EraseBehindTerms() == ErbhSome &&
                mColor != MarkerColor))
            MK.erbh_list = new Blist(&BB, MK.erbh_list);
        return;
    }

    if (display) {
        wdesc->Wdraw()->SetColor(pixel(wdesc));
        wdesc->ShowLine(x-delta, y, x+delta, y);
        wdesc->ShowLine(x, y-delta, x, y+delta);
        if (show_name)
            wdesc->ShowLabel(mTerm->name()->string(),
                x + d4, y + d4, wn, h, 0);
        if (ld)
            wdesc->ShowLabel(ld->name(), x + d4, y - d4 - h, wl, h, 0);
    }
    else
        refresh(wdesc, &BB);
}
// End of sMark_Pterm functions.


sMark_Ilab::sMark_Ilab(const CDc *cd, int vecix, int clr, int size, sMark *nx)
{
    mType = MARK_ILAB;
    mMode = Physical;
    mDelta = size;
    mOrient = 0;
    mNext = nx;
    mX = 0;
    mY = 0;
    mColor = clr;

    mCdesc = cd;
    mVecIx = vecix;
}


void
sMark_Ilab::show(WindowDesc *wdesc, bool display)
{
    if (!wdesc->IsSimilar((DisplayMode)mMode, DSP()->MainWdesc()))
        return;

    if (display) {
        if (wdesc->Mode() == Physical)
            ilab_mark(wdesc, DISPLAY);
    }
    else {
        if (wdesc->Mode() == Physical)
            ilab_mark(wdesc, ERASE);
    }
}


void
sMark_Ilab::addBB(WindowDesc *wdesc, BBox *BB)
{
    if (!wdesc->IsSimilar((DisplayMode)mMode, DSP()->MainWdesc()))
        return;

    if (!wdesc->Wdraw())
        return;
    if (wdesc->Mode() != Physical)
        return;
    if (!mCdesc)
        return;

    int x = -1;
    int y = -1;
    CDp_range *pr = (CDp_range*)mCdesc->prpty(P_RANGE);
    if (mVecIx > 0 && pr) {
        CDp_name *pn = pr->name_prp(0, mVecIx);
        if (pn) {
            x = pn->pos_x();
            y = pn->pos_y();
        }
    }
    else {
        CDp_name *pn = (CDp_name*)mCdesc->prpty(P_NAME);
        if (pn) {
            x = pn->pos_x();
            y = pn->pos_y();
        }
    }
    if (x == -1 && y == -1)
        return;

    int delta = wdesc->LogScale(4);
    int phei = wdesc->LogScale(DSP()->TermTextSize());
    if (phei < FT.cellHeight())
        phei = FT.cellHeight();

    BBox tBB(x-delta, y-delta, x+delta, y+delta);
    char *instname = mCdesc->getInstName(mVecIx);
    int w, h;
    DSP()->DefaultLabelSize(instname, wdesc->Mode(), &w, &h);
    delete [] instname;
    w = mmRnd((w*(double)phei)/h);
    h = phei;
    if (tBB.top < y + delta/2 + h)
        tBB.top = y + delta/2 + h;
    if (tBB.right < x + delta/2 + w)
        tBB.right = x + delta/2 + w;
    BB->add(&tBB);
}


void
sMark_Ilab::ilab_mark(WindowDesc *wdesc, bool display)
{
    if (!wdesc->Wdraw())
        return;
    if (wdesc->Mode() != Physical)
        return;
    if (!mCdesc)
        return;

    int x = -1;
    int y = -1;
    CDp_range *pr = (CDp_range*)mCdesc->prpty(P_RANGE);
    if (mVecIx > 0 && pr) {
        CDp_name *pn = pr->name_prp(0, mVecIx);
        if (pn) {
            x = pn->pos_x();
            y = pn->pos_y();
        }
    }
    else {
        CDp_name *pn = (CDp_name*)mCdesc->prpty(P_NAME);
        if (pn) {
            x = pn->pos_x();
            y = pn->pos_y();
        }
    }
    if (x == -1 && y == -1)
        return;

    int delta = wdesc->LogScale(4);
    int phei = wdesc->LogScale(DSP()->TermTextSize());
    if (phei < FT.cellHeight())
        phei = FT.cellHeight();

    BBox BB(x-delta, y-delta, x+delta, y+delta);
    char *instname = mCdesc->getInstName(mVecIx);
    int w, h;
    DSP()->DefaultLabelSize(instname, wdesc->Mode(), &w, &h);
    w = mmRnd((w*(double)phei)/h);
    h = phei;
    if (BB.top < y + delta/2 + h)
        BB.top = y + delta/2 + h;
    if (BB.right < x + delta/2 + w)
        BB.right = x + delta/2 + w;

    if (MK.save_bound) {
        if (DSP()->EraseBehindTerms() != ErbhNone)
            MK.erbh_list = new Blist(&BB, MK.erbh_list);
        delete [] instname;
        return;
    }

    if (display) {
        wdesc->Wdraw()->SetColor(pixel(wdesc));
        wdesc->ShowLine(x-delta, y, x+delta, y);
        wdesc->ShowLine(x, y-delta, x, y+delta);
        wdesc->ShowLabel(instname, x+delta/2, y+delta/2, w, h, 0);
    }
    else
        refresh(wdesc, &BB);
    delete [] instname;
}
// End of sMark_Ilab functions.


sMark_Bsc::sMark_Bsc(int x, int y, int del, int clr, int beg, int end,
    int tnum, CDc *cd, const char *tname, unsigned int vflags, bool symb,
    sMark *nx)
{
    mType = symb ? MARK_SYBSC : MARK_BSC;
    mMode = Electrical;
    mDelta = del;
    mOrient = 0;
    mNext = nx;
    mX = x;
    mY = y;
    mColor = clr;

    mCdesc = cd;
    mTname = lstring::copy(tname);
    mBeg = beg;
    mEnd = end;
    mId = tnum;
    mVflags = vflags;
    mQuad = 1;
    mSymbolic = symb;
}


void
sMark_Bsc::show(WindowDesc *wdesc, bool display)
{
    if (!wdesc->Wdraw())
        return;
    if (!wdesc->IsSimilar((DisplayMode)mMode, DSP()->MainWdesc(), WDsimXsymb))
        return;
    if (wdesc->Mode() != Electrical)
        return;

    // Make sure symbolic mode matches window.
    CDs *sd = wdesc->CurCellDesc(Electrical);
    if (!sd)
        return;
    if (sd->isSymbolic() != mSymbolic)
        return;

    if (display) {
        if (!mCdesc) {
            // The cell's terminals, allow overriding of invisibility
            // for editing.

            if ((!mSymbolic && (mVflags & TE_SCINVIS)) ||
                    (mSymbolic && (mVflags & TE_SYINVIS))) {
                if (!DSP()->ShowInvisMarks())
                    return;
                mInvisOverride = true;
            }
            wdesc->Wdraw()->SetColor(pixel(wdesc));
            bsc_mark(wdesc, DISPLAY);
            mInvisOverride = false;
        }
        else {
            // Instance terminals.

            CDs *msdesc = mCdesc->masterCell();
            if (!msdesc)
                return;
            if (msdesc->isSymbolic()) {
                if (mVflags & TE_SYINVIS)
                    return;
            }
            else {
                if (mVflags & TE_SCINVIS)
                    return;
            }
            wdesc->Wdraw()->SetColor(pixel(wdesc));
            bsc_mark(wdesc, DISPLAY);
        }
    }
    else
        bsc_mark(wdesc, ERASE);
}


void
sMark_Bsc::addBB(WindowDesc *wdesc, BBox *BB)
{
    if (!wdesc->Wdraw())
        return;
    if (!wdesc->IsSimilar((DisplayMode)mMode, DSP()->MainWdesc(), WDsimXsymb))
        return;
    if (wdesc->Mode() != Electrical)
        return;

    // Make sure symbolic mode matches window.
    CDs *sd = wdesc->CurCellDesc(Electrical);
    if (!sd)
        return;
    if (sd->isSymbolic() != mSymbolic)
        return;

    if (!mCdesc) {
        // The cell's terminals, allow overriding of invisibility
        // for editing.

        if ((!mSymbolic && (mVflags & TE_SCINVIS)) ||
                (mSymbolic && (mVflags & TE_SYINVIS))) {
            if (!DSP()->ShowInvisMarks())
                return;
        }
    }
    else {
        // Instance terminals.

        CDs *msdesc = mCdesc->masterCell();
        if (!msdesc)
            return;
        if (msdesc->isSymbolic()) {
            if (mVflags & TE_SYINVIS)
                return;
        }
        else {
            if (mVflags & TE_SCINVIS)
                return;
        }
    }

    int delta = wdesc->LogScale(DSP()->TermMarkSize());
    int phei = wdesc->LogScale(DSP()->TermTextSize());
    if (phei < FT.cellHeight())
        phei = FT.cellHeight();

    if (mCdesc)
        setQuad();

    BBox tBB(mX-delta, mY-delta, mX+delta, mY+delta);
    int w, h;
    int d4 = delta/4;
    sLstr lstr;
    if (mTname)
        lstr.add(mTname);
    lstr.add_c(cTnameTab::subscr_open());
    lstr.add_i(mBeg);
    lstr.add_c(':');
    lstr.add_i(mEnd);
    lstr.add_c(cTnameTab::subscr_close());
    DSP()->DefaultLabelSize(lstr.string(), wdesc->Mode(), &w, &h);
    w = mmRnd((w*(double)phei)/h);
    h = phei;

    if (!mCdesc) {
        if (mX + delta + d4 + w > tBB.right)
            tBB.right = mX + delta + d4 + w;
        if (mY - d4 - h < tBB.bottom)
            tBB.bottom = mY - d4 - h;
    }
    else {
        if (mQuad == 0) {
            if (mX - d4 - w < tBB.left)
                tBB.left = mX - d4 - w;
            if (mY + d4 + h > tBB.top)
                tBB.top = mY + d4 + h;
        }
        else if (mQuad == 2) {
            if (mX + d4 + w > tBB.right)
                tBB.right = mX + d4 + w;
            if (mY - d4 - h < tBB.bottom)
                tBB.bottom = mY - d4 - h;
        }
        else if (mQuad == 3) {
            if (mX - d4 - w < tBB.left)
                tBB.left = mX - d4 - w;
            if (mY - d4 - h < tBB.bottom)
                tBB.bottom = mY - d4 - h;
        }
        else {
            if (mX + d4 + w > tBB.right)
                tBB.right = mX + d4 + w;
            if (mY + d4 + h > tBB.top)
                tBB.top = mY + d4 + h;
        }
    }
    if (!mCdesc) {
        int i = 0;
        int j = 10;
        while (mId/j) {
            j *= 10;
            i++;
        }
        if (i) {
            tBB.left -= i*delta;
            tBB.right += i*delta;
        }
    }
    BB->add(&tBB);
}


// Look for coincident marks, and set the mQuad field.
//
void
sMark_Bsc::setQuad()
{
    int count = 0;
    for (sMark *m = MK.mark_heads[mType]; m && m != this; m = m->mNext) {
        if (m->mX == mX && m->mY == mY)
            count++;
    }
    switch (count) {
    case 1:
        mQuad = 3;
        break;
    case 2:
        mQuad = 2;
        break;
    case 3:
        mQuad = 0;
        break;
    }
}


void
sMark_Bsc::bsc_mark(WindowDesc *wdesc, bool display)
{
    int delta = wdesc->LogScale(DSP()->TermMarkSize());
    int phei = wdesc->LogScale(DSP()->TermTextSize());
    if (phei < FT.cellHeight())
        phei = FT.cellHeight();

    // be clever about coincident marks
    if (display == DISPLAY && mCdesc)
        setQuad();

    BBox BB(mX-delta, mY-delta, mX+delta, mY+delta);
    int w, h;
    int d4 = delta/4;
    sLstr lstr;
    if (mTname)
        lstr.add(mTname);
    else {
        lstr.add_c(cTnameTab::subscr_open());
        lstr.add_i(mBeg);
        lstr.add_c(':');
        lstr.add_i(mEnd);
        lstr.add_c(cTnameTab::subscr_close());
    }
    DSP()->DefaultLabelSize(lstr.string(), wdesc->Mode(), &w, &h);
    w = mmRnd((w*(double)phei)/h);
    h = phei;

    if (!mCdesc) {
        // Show the cell top level terminals as an outlined number.
        if (mX + delta + d4 + w > BB.right)
            BB.right = mX + delta + d4 + w;
        if (mY - d4 - h < BB.bottom)
            BB.bottom = mY - d4 - h;
    }
    else {
        if (mQuad == 0) {
            if (mX - d4 - w < BB.left)
                BB.left = mX - d4 - w;
            if (mY + d4 + h > BB.top)
                BB.top = mY + d4 + h;
        }
        else if (mQuad == 2) {
            if (mX + d4 + w > BB.right)
                BB.right = mX + d4 + w;
            if (mY - d4 - h < BB.bottom)
                BB.bottom = mY - d4 - h;
        }
        else if (mQuad == 3) {
            if (mX - d4 - w < BB.left)
                BB.left = mX - d4 - w;
            if (mY - d4 - h < BB.bottom)
                BB.bottom = mY - d4 - h;
        }
        else {
            if (mX + d4 + w > BB.right)
                BB.right = mX + d4 + w;
            if (mY + d4 + h > BB.top)
                BB.top = mY + d4 + h;
        }
    }

    if (display == DISPLAY) {
        if (!mCdesc) {
            // Show the cell top level terminals as an outlined number.
            int type = mType;
            mType = MARK_OBASE + mId;
            gp_mark(wdesc, (int)(2*delta*wdesc->Ratio()), true,
                sMark::gp_oct);
            mType = type;
            wdesc->ShowLabel(lstr.string(), mX + delta + d4, mY - d4 - h,
                w, h, 0);
        }
        else {
            int d2 = delta/2;
            wdesc->ShowLine(mX-d2, mY, mX, mY+d2);
            wdesc->ShowLine(mX, mY+d2, mX+d2, mY);
            wdesc->ShowLine(mX+d2, mY, mX, mY-d2);
            wdesc->ShowLine(mX, mY-d2, mX-d2, mY);
            wdesc->ShowLine(mX-delta, mY, mX+delta, mY);
            wdesc->ShowLine(mX, mY-delta, mX, mY+delta);
            int x, y;
            if (mQuad == 0) {
                x = mX - d4 - w;
                y = mY + d4;
            }
            else if (mQuad == 2) {
                x = mX + d4;
                y = mY - d4 - h;
            }
            else if (mQuad == 3) {
                x = mX - d4 - w;
                y = mY - d4 - h;
            }
            else {
                x = mX + d4;
                y = mY + d4;
            }
            wdesc->ShowLabel(lstr.string(), x, y, w, h, 0);
        }
    }
    else {
        if (!mCdesc) {
            int i = 0;
            int j = 10;
            while (mId/j) {
                j *= 10;
                i++;
            }
            if (i) {
                BB.left -= i*delta;
                BB.right += i*delta;
            }
        }
        refresh(wdesc, &BB);
    }
}
// End of sMark_Bsc functions.


sMark_Fence::sMark_Fence(const CDc *cdesc, int id, int x, int y,
    int xe, int ye, int clr, sMark *nx)
{
    mNext = nx;
    mType = MARK_FENCE;
    mMode = Physical;
    mX = x;
    mY = y;
    mColor = clr;
    mCdesc = cdesc;
    mEndX = xe;
    mEndY = ye;
    mId = id;
}


void
sMark_Fence::show(WindowDesc *wdesc, bool display)
{
    if (!wdesc->Wdraw())
        return;
    if (wdesc->Mode() != Physical)
        return;
    if (!wdesc->IsSimilar(Physical, DSP()->MainWdesc()))
        return;

    if (display) {
        if (mCdesc) {
            // Show this mark type in expanded instance only, if
            // associated with an instance.

            int xlev = wdesc->Attrib()->expand_level(Physical);
            if (xlev == 0 && !mCdesc->has_flag(wdesc->DisplFlags()))
                return;

            // Show marks only of the instance display on screen is
            // suitably large.
            int len = mCdesc->oBB().width();
            int tmp = mCdesc->oBB().height();
            if (tmp < len)
                len = tmp;
            len = (int)(len * wdesc->Ratio());
            if (len < DSP()->FenceInstPixSize())
                return;
        }
        wdesc->Wdraw()->SetColor(pixel(wdesc));
        int x1, y1, x2, y2;
        wdesc->LToP(mX, mY, x1, y1);
        wdesc->LToP(mEndX, mEndY, x2, y2);
        if (x1 == x2) {
            show_line(wdesc, x1-1, y1, x1-1, y2);
            show_line(wdesc, x1+1, y1, x1+1, y2);
        }
        else if (y1 == y2) {
            show_line(wdesc, x1, y1-1, x2, y1-1);
            show_line(wdesc, x1, y1+1, x2, y1+1);
        }
        else {
            show_line(wdesc, x1, y1, x2, y2);
        }
    }
    else {
        int delta = (int)(2.0/wdesc->Ratio());
        BBox BB(mX, mY, mEndX, mEndY);
        BB.fix();
        BB.bloat(delta);
        refresh(wdesc, &BB);
    }
}


void
sMark_Fence::addBB(WindowDesc *wdesc, BBox *BB)
{
    if (!wdesc->Wdraw())
        return;
    if (wdesc->Mode() != Physical)
        return;
    if (!wdesc->IsSimilar(Physical, DSP()->MainWdesc()))
        return;

    int delta = (int)(1.1/wdesc->Ratio());
    BBox tBB(mX, mY, mEndX, mEndY);
    tBB.fix();
    tBB.bloat(delta);
    BB->add(&tBB);
}
// End of sMark_Fence functions.


sMark_Plot::sMark_Plot(const CDs *sd, const hyParent *p, int t, int x, int y,
    int del, int ori, int clr, sMark *nx)
{
    mType = t;
    mMode = Electrical;
    mDelta = del;
    mOrient = ori;
    mNext = nx;
    mX = x;
    mY = y;
    mColor = clr;
    mSdesc = sd;
    mProxy = hyParent::dup(p);
}


void
sMark_Plot::show(WindowDesc *wdesc, bool display)
{
    const CDs *sd = wdesc->CurCellDesc(wdesc->Mode());
    if (sd != mSdesc)
        return;
    hyParent *p = wdesc->ProxyList();
    int c = hyParent::cmp(p, mProxy);
    hyParent::destroy(p);
    if (!c)
        return;

    int x = mX;
    int y = mY;
    int delta = wdesc->LogScaleToPix(mDelta/2);

    if (display)
        // plot marks
        gp_mark(wdesc, delta, false, sMark::gp_box);
    else {
        DSP()->TPoint(&x, &y);
        wdesc->LToP(x, y, x, y);
        BBox BB(x - delta, y + delta, x + delta, y - delta);
        wdesc->Update(&BB);
    }
}


void
sMark_Plot::addBB(WindowDesc *wdesc, BBox *BB)
{
    const CDs *sd = wdesc->CurCellDesc(wdesc->Mode());
    if (sd != mSdesc)
        return;
    hyParent *p = wdesc->ProxyList();
    int c = hyParent::cmp(p, mProxy);
    hyParent::destroy(p);
    if (!c)
        return;

    int x = mX;
    int y = mY;
    int delta = wdesc->LogScaleToPix(mDelta/2);
    delta = mmRnd(delta/wdesc->Ratio());
    BBox tBB(x-delta, y-delta, x+delta, y+delta);
    BB->add(&tBB);
}
// End of sMark_Plot functions.



//-------------------------------------------------------------------------
// A general-purpose mark list for user-supplied annotation and graphics.
//

#define hlaTextured 0x1
#define hlaBlink    0x2
#define hlaColor    0x4

namespace { int id_cnt = 1; }

namespace dsp_mark {
    struct hlite_t
    {
        hlite_t(hlite_t *n)
        {
            hl_type = hlNone;
            hl_id = id_cnt++;
            hl_attr = 0;
            next = n;
        }
        ~hlite_t();

        void line_init(int, int, int, int);
        void box_init(int, int, int, int);
        void vtriang_init(int, int, int, int);
        void htriang_init(int, int, int, int);
        void circle_init(int, int, int);
        void ellipse_init(int, int, int, int);
        void poly_init(Point*, int);
        void text_init(const char*, int, int, int, int, int);
        void display(bool);
        void display(WindowDesc*, bool, bool);
        void addBB(WindowDesc*, BBox*);
        bool intersect(const BBox*);
        char *print();

        static hlite_t *parse(const char*);

        hlType hl_type;         // mark type
        int hl_id;              // mark id number
        unsigned int hl_attr;   // attributes flags
        union {
            struct { int x1, y1, x2, y2; } line;
            struct { int l, b, r, t; } box;
            struct { int xl, xr, yb, yt; } vtriang;
            struct { int xb, xt, yl, yu; } htriang;
            struct { int xc, yc, rad; } circle;
            struct { int xc, yc, rx, ry; } ellipse;
            struct { Point *points; int numpts; } poly;
            struct { char *label; int x, y, wid, hei, xform; } text;
        } hl_u;
        hlite_t *next;
    };
}


hlite_t::~hlite_t()
{
    if (hl_type == hlPoly)
        delete [] hl_u.poly.points;
    else if (hl_type == hlText)
        delete [] (char*)hl_u.text.label;
}


void
hlite_t::line_init(int x1, int y1, int x2, int y2)
{
    hl_type = hlLine;
    hl_u.line.x1 = x1;
    hl_u.line.y1 = y1;
    hl_u.line.x2 = x2;
    hl_u.line.y2 = y2;
}


void
hlite_t::box_init(int l, int b, int r, int t)
{
    hl_type = hlBox;
    hl_u.box.l = l;
    hl_u.box.b = b;
    hl_u.box.r = r;
    hl_u.box.t = t;
}


void
hlite_t::vtriang_init(int xl, int xr, int yb, int yt)
{
    hl_type = hlVtriang;
    hl_u.vtriang.xl = xl;
    hl_u.vtriang.xr = xr;
    hl_u.vtriang.yb = yb;
    hl_u.vtriang.yt = yt;
}


void
hlite_t::htriang_init(int xb, int xt, int yl, int yu)
{
    hl_type = hlHtriang;
    hl_u.htriang.xb = xb;
    hl_u.htriang.xt = xt;
    hl_u.htriang.yl = yl;
    hl_u.htriang.yu = yu;
}


void
hlite_t::circle_init(int xc, int yc, int rad)
{
    hl_type = hlCircle;
    hl_u.circle.xc = xc;
    hl_u.circle.yc = yc;
    hl_u.circle.rad = rad;
}


void
hlite_t::ellipse_init(int xc, int yc, int rx, int ry)
{
    hl_type = hlEllipse;
    hl_u.ellipse.xc = xc;
    hl_u.ellipse.yc = yc;
    hl_u.ellipse.rx = rx;
    hl_u.ellipse.ry = ry;
}


void
hlite_t::poly_init(Point *pts, int numpts)
{
    hl_type = hlPoly;
    hl_u.poly.points = new Point[numpts];
    hl_u.poly.numpts = numpts;
    memcpy(hl_u.poly.points, pts, numpts*sizeof(Point));
}


void
hlite_t::text_init(const char *text, int x, int y, int wid, int hei, int xf)
{
    hl_type = hlText;
    hl_u.text.x = x;
    hl_u.text.y = y;
    hl_u.text.wid = wid;
    hl_u.text.hei = hei;
    hl_u.text.xform = xf;
    hl_u.text.label = lstring::copy(text);
}


void
hlite_t::display(bool displ)
{
    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wdesc = wgen.next()) != 0)
        display(wdesc, displ, false);
}


void
hlite_t::display(WindowDesc *wdesc, bool disp_or_erase, bool blink)
{
    if (!wdesc->Wdraw())
        return;
    if (disp_or_erase == DISPLAY) {
        if (hl_attr & hlaColor) {
            if (blink)
                return;
            wdesc->Wdraw()->SetColor(DSP()->Color(MarkerColor,
                wdesc->Mode()));
        }
        else if (hl_attr & hlaBlink) {
            if (!blink)
                return;
            wdesc->Wdraw()->SetColor(DSP()->SelectPixel());
        }
        else {
            if (blink)
                return;
            wdesc->Wdraw()->SetColor(DSP()->Color(HighlightingColor,
                wdesc->Mode()));
        }
        if (hl_attr & hlaTextured)
            wdesc->Wdraw()->SetLinestyle(DSP()->BoxLinestyle());
        else
            wdesc->Wdraw()->SetLinestyle(0);

        if (hl_type == hlLine) {
            wdesc->ShowLine(hl_u.line.x1, hl_u.line.y1,
                hl_u.line.x2, hl_u.line.y2);
        }
        else if (hl_type == hlBox) {
            wdesc->ShowLine(hl_u.box.l, hl_u.box.b, hl_u.box.l, hl_u.box.t);
            wdesc->ShowLine(hl_u.box.l, hl_u.box.t, hl_u.box.r, hl_u.box.t);
            wdesc->ShowLine(hl_u.box.r, hl_u.box.t, hl_u.box.r, hl_u.box.b);
            wdesc->ShowLine(hl_u.box.r, hl_u.box.b, hl_u.box.l, hl_u.box.b);
        }
        else if (hl_type == hlVtriang) {
            wdesc->ShowLine(hl_u.vtriang.xl, hl_u.vtriang.yb,
                (hl_u.vtriang.xl + hl_u.vtriang.xr)/2, hl_u.vtriang.yt);
            wdesc->ShowLine((hl_u.vtriang.xl + hl_u.vtriang.xr)/2,
                hl_u.vtriang.yt, hl_u.vtriang.xr, hl_u.vtriang.yb);
            wdesc->ShowLine(hl_u.vtriang.xr, hl_u.vtriang.yb, hl_u.vtriang.xl,
                hl_u.vtriang.yb);
        }
        else if (hl_type == hlHtriang) {
            wdesc->ShowLine(hl_u.htriang.xb, hl_u.htriang.yl,
                hl_u.htriang.xt, (hl_u.htriang.yl + hl_u.htriang.yu)/2);
            wdesc->ShowLine(
                hl_u.htriang.xt, (hl_u.htriang.yl + hl_u.htriang.yu)/2,
                hl_u.htriang.xb, hl_u.htriang.yu);
            wdesc->ShowLine(hl_u.htriang.xb, hl_u.htriang.yu, hl_u.htriang.xb,
                hl_u.htriang.yl);
        }
        else if (hl_type == hlCircle) {
            int xc = hl_u.circle.xc;
            int yc = hl_u.circle.yc;
            DSP()->TPoint(&xc, &yc);
            wdesc->LToP(xc, yc, xc, yc);
            int r = (int)(wdesc->Ratio()*hl_u.circle.rad);
            wdesc->Wdraw()->Arc(xc, yc, r, r, 0.0, 0.0);
        }
        else if (hl_type == hlEllipse) {
            int xc = hl_u.circle.xc;
            int yc = hl_u.circle.yc;
            DSP()->TPoint(&xc, &yc);
            wdesc->LToP(xc, yc, xc, yc);
            int rx = (int)(wdesc->Ratio()*hl_u.ellipse.rx);
            int ry = (int)(wdesc->Ratio()*hl_u.ellipse.ry);
            int ax = 0;
            int ay = 1;
            DSP()->TPoint(&ax, &ay);
            if (ax) {
                int t = rx;
                rx = ry;
                ry = t;
            }
            wdesc->Wdraw()->Arc(xc, yc, rx, ry, 0.0, 0.0);
        }
        else if (hl_type == hlPoly) {
            Point *p = hl_u.poly.points;
            for (int i = 1; i < hl_u.poly.numpts; i++)
                wdesc->ShowLine(p[i-1].x, p[i-1].y, p[i].x, p[i].y);
        }
        else if (hl_type == hlText) {
            wdesc->ShowLabel(hl_u.text.label, hl_u.text.x, hl_u.text.y,
                hl_u.text.wid, hl_u.text.hei, hl_u.text.xform);
        }
    }
    else {
        // Assumes that 'this' is no longer in display list.
        BBox BB = CDnullBB;
        addBB(wdesc, &BB);
        wdesc->Redisplay(&BB);
    }
}


void
hlite_t::addBB(WindowDesc *wdesc, BBox *BB)
{
    BBox tBB = CDnullBB;
    if (hl_type == hlLine) {
        tBB.add(hl_u.line.x1, hl_u.line.y1);
        tBB.add(hl_u.line.x2, hl_u.line.y2);
    }
    else if (hl_type == hlBox) {
        tBB.add(hl_u.box.l, hl_u.box.b);
        tBB.add(hl_u.box.r, hl_u.box.t);
    }
    else if (hl_type == hlVtriang) {
        tBB.add(hl_u.vtriang.xl, hl_u.vtriang.yb);
        tBB.add(hl_u.vtriang.xr, hl_u.vtriang.yt);
    }
    else if (hl_type == hlHtriang) {
        tBB.add(hl_u.htriang.xb, hl_u.htriang.yl);
        tBB.add(hl_u.htriang.xt, hl_u.htriang.yu);
    }
    else if (hl_type == hlCircle) {
        tBB.add(hl_u.circle.xc - hl_u.circle.rad,
            hl_u.circle.yc - hl_u.circle.rad);
        tBB.add(hl_u.circle.xc + hl_u.circle.rad,
            hl_u.circle.yc + hl_u.circle.rad);
    }
    else if (hl_type == hlEllipse) {
        tBB.add(hl_u.ellipse.xc - hl_u.ellipse.rx,
            hl_u.ellipse.yc - hl_u.ellipse.ry);
        tBB.add(hl_u.ellipse.xc + hl_u.ellipse.rx,
            hl_u.ellipse.yc + hl_u.ellipse.ry);
    }
    else if (hl_type == hlPoly) {
        Point *p = hl_u.poly.points;
        for (int i = 1; i < hl_u.poly.numpts; i++)
            tBB.add(p[i].x, p[i].y);
    }
    else if (hl_type == hlText) {
        tBB.left = 0;
        tBB.bottom = 0;
        tBB.right = hl_u.text.wid;
        tBB.top = hl_u.text.hei;
        DSP()->TSetTransformFromXform(hl_u.text.xform,
            hl_u.text.wid, hl_u.text.hei);
        DSP()->TTranslate(hl_u.text.x, hl_u.text.y);
        DSP()->TBB(&tBB, 0);
        DSP()->TPop();
    }
    int bl = (int)(2.0/wdesc->Ratio());
    tBB.bloat(bl);
    BB->add(&tBB);
}


bool
hlite_t::intersect(const BBox *AOI)
{
    if (hl_type == hlLine) {
        Point_c p1(hl_u.line.x1, hl_u.line.y1);
        Point_c p2(hl_u.line.x2, hl_u.line.y2);
        return (!GEO()->line_clip(&p1.x, &p1.y, &p2.x, &p2.y, AOI));
    }
    if (hl_type == hlBox) {
        BBox BB(hl_u.box.l, hl_u.box.b, hl_u.box.r, hl_u.box.t);
        BB.fix();
        return (BB.intersect(AOI, true));
    }
    if (hl_type == hlVtriang) {
        Point pts[4];
        pts[0].set(hl_u.vtriang.xl, hl_u.vtriang.yb);
        pts[1].set((hl_u.vtriang.xl + hl_u.vtriang.xr)/2, hl_u.vtriang.yt);
        pts[2].set(hl_u.vtriang.xr, hl_u.vtriang.yb);
        pts[3] = pts[0];
        Poly po(4, pts);
        return (po.intersect(AOI, true));
    }
    if (hl_type == hlHtriang) {
        Point pts[4];
        pts[0].set(hl_u.htriang.xb, hl_u.htriang.yl);
        pts[1].set(hl_u.htriang.xt, (hl_u.htriang.yl + hl_u.htriang.yu)/2);
        pts[2].set(hl_u.htriang.xb, hl_u.htriang.yu);
        pts[3] = pts[0];
        Poly po(4, pts);
        return (po.intersect(AOI, true));
    }
    if (hl_type == hlCircle) {
        Poly po;
        po.points = GEO()->makeArcPath(&po.numpts, true,
            hl_u.circle.xc, hl_u.circle.yc, hl_u.circle.rad, hl_u.circle.rad);
        bool ret = po.intersect(AOI, true);
        delete [] po.points;
        return (ret);
    }
    if (hl_type == hlEllipse) {
        Poly po;
        po.points = GEO()->makeArcPath(&po.numpts, true, hl_u.ellipse.xc,
            hl_u.ellipse.yc, hl_u.ellipse.rx, hl_u.ellipse.ry);
        bool ret = po.intersect(AOI, true);
        delete [] po.points;
        return (ret);
    }
    if (hl_type == hlPoly) {
        Poly po(hl_u.poly.numpts, hl_u.poly.points);
        return (po.intersect(AOI, true));
    }
    if (hl_type == hlText) {
        BBox BB(0, 0, hl_u.text.wid, hl_u.text.hei);
        DSP()->TSetTransformFromXform(hl_u.text.xform,
            hl_u.text.wid, hl_u.text.hei);
        DSP()->TTranslate(hl_u.text.x, hl_u.text.y);
        DSP()->TBB(&BB, 0);
        DSP()->TPop();
        return (BB.intersect(AOI, true));
    }
    return (false);
}


// Return a string describing the user mark.
//
char *
hlite_t::print()
{
    char buf[256];
    if (hl_type == hlLine) {
        sprintf(buf, "l %.4f %.4f %.4f %.4f %d",
            MICRONS(hl_u.line.x1), MICRONS(hl_u.line.y1),
            MICRONS(hl_u.line.x2), MICRONS(hl_u.line.y2), hl_attr);
        return (lstring::copy(buf));
    }
    if (hl_type == hlBox) {
        sprintf(buf, "b %.4f %.4f %.4f %.4f %d",
            MICRONS(hl_u.box.l), MICRONS(hl_u.box.b),
            MICRONS(hl_u.box.r), MICRONS(hl_u.box.t), hl_attr);
        return (lstring::copy(buf));
    }
    if (hl_type == hlVtriang) {
        sprintf(buf, "u %.4f %.4f %.4f %.4f %d",
            MICRONS(hl_u.vtriang.xl), MICRONS(hl_u.vtriang.xr),
            MICRONS(hl_u.vtriang.yb), MICRONS(hl_u.vtriang.yt), hl_attr);
        return (lstring::copy(buf));
    }
    if (hl_type == hlHtriang) {
        sprintf(buf, "t %.4f %.4f %.4f %.4f %d",
            MICRONS(hl_u.htriang.yl), MICRONS(hl_u.htriang.yu),
            MICRONS(hl_u.htriang.xb), MICRONS(hl_u.htriang.xt), hl_attr);
        return (lstring::copy(buf));
    }
    if (hl_type == hlCircle) {
        sprintf(buf, "c %.4f %.4f %.4f %d",
            MICRONS(hl_u.circle.xc), MICRONS(hl_u.circle.yc),
            MICRONS(hl_u.circle.rad), hl_attr);
        return (lstring::copy(buf));
    }
    if (hl_type == hlEllipse) {
        sprintf(buf, "e %.4f %.4f %.4f %.4f %d",
            MICRONS(hl_u.ellipse.xc), MICRONS(hl_u.ellipse.yc),
            MICRONS(hl_u.ellipse.rx), MICRONS(hl_u.ellipse.ry), hl_attr);
        return (lstring::copy(buf));
    }
    if (hl_type == hlPoly) {
        sprintf(buf, "p %d", hl_u.poly.numpts);
        sLstr lstr;
        lstr.add(buf);
        for (int i = 0; i < hl_u.poly.numpts; i++) {
            sprintf(buf, " %.4f %.4f",
                MICRONS(hl_u.poly.points[i].x),
                MICRONS(hl_u.poly.points[i].y));
            lstr.add(buf);
        }
        sprintf(buf, " %d", hl_attr);
        lstr.add(buf);
        return (lstr.string_trim());
    }
    if (hl_type == hlText) {
        sLstr lstr;
        lstr.add("s \"");
        lstr.add(hl_u.text.label);
        lstr.add("\" ");
        sprintf(buf, "%.4f %.4f %.4f %.4f %d %d",
            MICRONS(hl_u.text.x), MICRONS(hl_u.text.y),
            MICRONS(hl_u.text.wid), MICRONS(hl_u.text.hei),
            hl_u.text.xform, hl_attr);
        lstr.add(buf);
        return (lstr.string_trim());
    }
    return (0);
}


// Static function
// Parse a line, in the format used by the print method, returning a
// new hlite_t if successful.
//
hlite_t *
hlite_t::parse(const char *str)
{
    if (!str)
        return (0);
    while (isspace(*str))
        str++;
    if (*str == 'l') {
        double x1, y1, x2, y2;
        int at;
        lstring::advtok(&str);
        if (sscanf(str, "%lf %lf %lf %lf %d", &x1, &y1, &x2, &y2, &at) == 5) {
            hlite_t *m = new hlite_t(0);
            m->line_init(INTERNAL_UNITS(x1), INTERNAL_UNITS(y1),
                INTERNAL_UNITS(x2), INTERNAL_UNITS(y2));
            m->hl_attr = at;
            return (m);
        }
        return (0);
    }
    if (*str == 'b') {
        double l, b, r, t;
        int at;
        lstring::advtok(&str);
        if (sscanf(str, "%lf %lf %lf %lf %d", &l, &b, &r, &t, &at) == 5) {
            hlite_t *m = new hlite_t(0);
            m->box_init(INTERNAL_UNITS(l), INTERNAL_UNITS(b),
                INTERNAL_UNITS(r), INTERNAL_UNITS(t));
            m->hl_attr = at;
            return (m);
        }
        return (0);
    }
    if (*str == 'u') {
        double xl, xr, yb, yt;
        int at;
        lstring::advtok(&str);
        if (sscanf(str, "%lf %lf %lf %lf %d", &xl, &xr, &yb, &yt, &at) == 5) {
            hlite_t *m = new hlite_t(0);
            m->vtriang_init(INTERNAL_UNITS(xl), INTERNAL_UNITS(xr),
                INTERNAL_UNITS(yb), INTERNAL_UNITS(yt));
            m->hl_attr = at;
            return (m);
        }
        return (0);
    }
    if (*str == 't') {
        double yl, yu, xb, xt;
        int at;
        lstring::advtok(&str);
        if (sscanf(str, "%lf %lf %lf %lf %d", &yl, &yu, &xb, &xt, &at) == 5) {
            hlite_t *m = new hlite_t(0);
            m->htriang_init(INTERNAL_UNITS(xb), INTERNAL_UNITS(xt),
                INTERNAL_UNITS(yl), INTERNAL_UNITS(yu));
            m->hl_attr = at;
            return (m);
        }
        return (0);
    }
    if (*str == 'c') {
        double xc, yc, rad;
        int at;
        lstring::advtok(&str);
        if (sscanf(str, "%lf %lf %lf %d", &xc, &yc, &rad, &at) == 4) {
            hlite_t *m = new hlite_t(0);
            m->circle_init(INTERNAL_UNITS(xc), INTERNAL_UNITS(yc),
                INTERNAL_UNITS(rad));
            m->hl_attr = at;
            return (m);
        }
        return (0);
    }
    if (*str == 'e') {
        double xc, yc, rx, ry;
        int at;
        lstring::advtok(&str);
        if (sscanf(str, "%lf %lf %lf %lf %d", &xc, &yc, &rx, &ry, &at) == 5) {
            hlite_t *m = new hlite_t(0);
            m->ellipse_init(INTERNAL_UNITS(xc), INTERNAL_UNITS(yc),
                INTERNAL_UNITS(rx), INTERNAL_UNITS(ry));
            m->hl_attr = at;
            return (m);
        }
        return (0);
    }
    if (*str == 'p') {
        lstring::advtok(&str);
        char *tok = lstring::gettok(&str);
        int npts;
        if (!tok || (npts = atoi(tok)) < 4) {
            delete [] tok;
            return (0);
        }
        delete [] tok;
        Point *pts = new Point[npts];
        for (int i = 0; i < npts; i++) {
            char *tok1 = lstring::gettok(&str);
            char *tok2 = lstring::gettok(&str);
            if (!tok2) {
                delete [] tok2;
                delete [] pts;
                return (0);
                double x = atof(tok1);
                double y = atof(tok2);
                delete [] tok1;
                delete [] tok2;
                pts[i].set(INTERNAL_UNITS(x), INTERNAL_UNITS(y));
            }
        }
        tok = lstring::gettok(&str);
        if (!tok) {
            delete [] pts;
            return (0);
        }
        int at = atoi(tok);
        delete [] tok;

        hlite_t *m = new hlite_t(0);
        m->poly_init(pts, npts);
        m->hl_attr = at;
        delete [] pts;
        return (m);
    }
    if (*str == 's') {
        const char *s = strchr(str, '"');
        const char *e = strrchr(str, '"');
        if (!s || !e || e == s)
            return (0);
        char *label = new char[e - s];
        strncpy(label, s+1, e - s - 1);
        label[e - s - 1] = 0;

        e++;
        while (isspace(*e))
            e++;

        double x, y, w, h;
        int xform, at;
        if (sscanf(e, "%lf %lf %lf %lf %d %d", &x, &y, &w, &h,
                &xform, &at) == 6) {
            hlite_t *m = new hlite_t(0);
            m->text_init(label, INTERNAL_UNITS(x), INTERNAL_UNITS(y),
                INTERNAL_UNITS(w), INTERNAL_UNITS(h), xform);
            delete [] label;
            m->hl_attr = at;
            return (m);
        }
        delete [] label;
        return (0);
    }
    return (0);
}
// End of hlite_t functions.


// Display the user mark list.
//
void
sMK::show_user_marks(WindowDesc *wdesc, bool blink)
{
    if (!user_marks_tab)
        return;
    if (wdesc->DbType() != WDcddb)
        return;
    CDs *sd = wdesc->CurCellDesc(wdesc->Mode(), true);
    if (!sd)
        return;
    hlite_t *m = (hlite_t*)user_marks_tab->get((unsigned long)sd);
    if (m == (hlite_t*)ST_NIL)
        return;
    for ( ; m; m = m->next)
        m->display(wdesc, DISPLAY, blink);
}


void
sMK::add_user_marksBB(WindowDesc *wdesc, BBox *BB)
{
    if (!user_marks_tab)
        return;
    if (wdesc->DbType() != WDcddb)
        return;
    CDs *sd = wdesc->CurCellDesc(wdesc->Mode(), true);
    if (!sd)
        return;
    hlite_t *m = (hlite_t*)user_marks_tab->get((unsigned long)sd);
    if (m == (hlite_t*)ST_NIL)
        return;
    for ( ; m; m = m->next)
        m->addBB(wdesc, BB);
}


// Remove and un-display the first mark found that overlaps AOI in
// the current cell.
//
bool
sMK::remove_user_mark(const CDs *sd, const BBox *AOI)
{
    if (!user_marks_tab || !sd)
        return (false);
    SymTabEnt *h = user_marks_tab->get_ent((unsigned long)sd);
    if (!h)
        return (false);
    hlite_t *mp = 0;
    for (hlite_t *m = (hlite_t*)h->stData; m; m = m->next) {
        if (m->intersect(AOI)) {
            if (mp)
                mp->next = m->next;
            else
                h->stData = m->next;
            m->display(ERASE);
            delete m;
            return (true);
        }
        mp = m;
    }
    return (false);
}


// Remove and un-display a mark, passed by id, in the current cell. 
// If id <= 0, remove/undisplay all marks in current cell.
//
bool
sMK::remove_user_mark(const CDs *sd, int id)
{
    if (!user_marks_tab || !sd)
        return (false);
    SymTabEnt *h = user_marks_tab->get_ent((unsigned long)sd);
    if (!h)
        return (false);
    if (id <= 0) {
        hlite_t *m = (hlite_t*)h->stData;
        while (m) {
            hlite_t *mx = m;
            m = m->next;
            mx->display(ERASE);
            delete mx;
        }
        user_marks_tab->remove((unsigned long)sd);
    }
    else {
        hlite_t *mp = 0;
        for (hlite_t *m = (hlite_t*)h->stData; m; m = m->next) {
            if (m->hl_id == id) {
                if (mp)
                    mp->next = m->next;
                else
                    h->stData = m->next;
                m->display(ERASE);
                delete m;
                return (true);
            }
            mp = m;
        }
    }
    return (false);
}


// Clear/free the user mark list (no redisplay).  If passed 0, destroy
// the entire table, otherwise destroy the corresponding marks.
//
void
sMK::clear_user_marks(const CDs *sd)
{
    if (!user_marks_tab)
        return;
    if (!sd) {
        SymTabGen gen(user_marks_tab, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            hlite_t *m = (hlite_t*)h->stData;
            while (m) {
                hlite_t *mx = m;
                m = m->next;
                delete mx;
            }
            delete h;
        }
        delete user_marks_tab;
        user_marks_tab = 0;
        return;
    }
    hlite_t *m = (hlite_t*)user_marks_tab->get((unsigned long)sd);
    if (m == (hlite_t*)ST_NIL)
        return;
    while (m) {
        hlite_t *mx = m;
        m = m->next;
        delete mx;
    }
    user_marks_tab->remove((unsigned long)sd);
}


bool
sMK::add_user_mark(const CDs *sd, hlite_t *m)
{
    if (!sd)
        return (false);
    if (!m)
        return (true);
    if (!user_marks_tab)
        user_marks_tab = new SymTab(false, false);
    SymTabEnt *h = user_marks_tab->get_ent((unsigned long)sd);
    if (!h) {
        user_marks_tab->add((unsigned long)sd, 0, false);
        h = user_marks_tab->get_ent((unsigned long)sd);
    }
    m->next = (hlite_t*)h->stData;
    h->stData = m;
    return (true);
}


hlite_t *
sMK::new_user_mark(const CDs *sd)
{
    if (!sd)
        return (0);
    if (!user_marks_tab)
        user_marks_tab = new SymTab(false, false);
    SymTabEnt *h = user_marks_tab->get_ent((unsigned long)sd);
    if (!h) {
        user_marks_tab->add((unsigned long)sd, 0, false);
        h = user_marks_tab->get_ent((unsigned long)sd);
    }
    hlite_t *m = new hlite_t((hlite_t*)h->stData);
    h->stData = m;
    return (m);
}


// Dump the mark list of sd to fname.  If fname is null or empty, a
// default name is used.  Return the number of marks dumped, -r -1 if
// error.  If there are no marks, no file is generated.
//
int
sMK::dump_user_marks(const char *fname, const CDs *sd)
{
    if (!sd) {
        Errs()->add_error("dump_error_marks: null cell pointer");
        return (-1);
    }
    if (!user_marks_tab)
        return (0);
    SymTabEnt *h = user_marks_tab->get_ent((unsigned long)sd);
    if (!h || !h->stData)
        return (0);

    char buf[256];
    if (!fname || !*fname) {
        sprintf(buf, "%s.%s.marks", sd->cellname()->string(),
            sd->isElectrical() ? "elec" : "phys");
        fname = buf;
    }
    if (!filestat::create_bak(fname)) {
        Errs()->add_error(filestat::error_msg());
        return (-1);  
    }
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        Errs()->add_error("Unable to open %s.", fname);
        return (-1);
    }
    fprintf(fp, "%s %s\n", sd->cellname()->string(),
        sd->isElectrical() ? "elec" : "phys");
    int mcnt = 0;
    for (hlite_t *m = (hlite_t*)h->stData; m; m = m->next) {
        char *s = m->print();
        if (s) {
            mcnt++;
            fprintf(fp, "%s\n", s);
            delete [] s;
        }
    }
    fclose (fp);
    return (mcnt);
}
// End of sMK functions.


// Add and display a new mark.  A unique integer id is returned, or 0 if
// error.
//
int
cDisplay::AddUserMark(hlType type, ...)
{
    va_list args;
    va_start(args, type);

    if (type == hlLine) {
        int x1 = va_arg(args, int);
        int y1 = va_arg(args, int);
        int x2 = va_arg(args, int);
        int y2 = va_arg(args, int);
        int at = va_arg(args, int);

        hlite_t *m = MK.new_user_mark(CurCell());
        if (!m) {
            va_end(args);
            return (0);
        }
        m->line_init(x1, y1, x2, y2);
        m->hl_attr = at;
        m->display(DISPLAY);
        va_end(args);
        return (m->hl_id);
    }
    if (type == hlBox) {
        int l = va_arg(args, int);
        int b = va_arg(args, int);
        int r = va_arg(args, int);
        int t = va_arg(args, int);
        int at = va_arg(args, int);

        hlite_t *m = MK.new_user_mark(CurCell());
        if (!m) {
            va_end(args);
            return (0);
        }
        m->box_init(l, b, r, t);
        m->hl_attr = at;
        m->display(DISPLAY);
        va_end(args);
        return (m->hl_id);
    }
    if (type == hlVtriang) {
        int xl = va_arg(args, int);
        int xr = va_arg(args, int);
        int yb = va_arg(args, int);
        int yt = va_arg(args, int);
        int at = va_arg(args, int);

        hlite_t *m = MK.new_user_mark(CurCell());
        if (!m) {
            va_end(args);
            return (0);
        }
        m->vtriang_init(xl, xr, yb, yt);
        m->hl_attr = at;
        m->display(DISPLAY);
        va_end(args);
        return (m->hl_id);
    }
    if (type == hlHtriang) {
        int yl = va_arg(args, int);
        int yu = va_arg(args, int);
        int xb = va_arg(args, int);
        int xt = va_arg(args, int);
        int at = va_arg(args, int);

        hlite_t *m = MK.new_user_mark(CurCell());
        if (!m) {
            va_end(args);
            return (0);
        }
        m->htriang_init(xb, xt, yl, yu);
        m->hl_attr = at;
        m->display(DISPLAY);
        va_end(args);
        return (m->hl_id);
    }
    if (type == hlCircle) {
        int xc = va_arg(args, int);
        int yc = va_arg(args, int);
        int rad = va_arg(args, int);
        int at = va_arg(args, int);
        if (rad <= 0) {
            va_end(args);
            return (0);
        }

        hlite_t *m = MK.new_user_mark(CurCell());
        if (!m) {
            va_end(args);
            return (0);
        }
        m->circle_init(xc, yc, rad);
        m->hl_attr = at;
        m->display(DISPLAY);
        va_end(args);
        return (m->hl_id);
    }
    if (type == hlEllipse) {
        int xc = va_arg(args, int);
        int yc = va_arg(args, int);
        int rx = va_arg(args, int);
        int ry = va_arg(args, int);
        int at = va_arg(args, int);
        if (rx <= 0 || ry <= 0) {
            va_end(args);
            return (0);
        }

        hlite_t *m = MK.new_user_mark(CurCell());
        if (!m) {
            va_end(args);
            return (0);
        }
        m->ellipse_init(xc, yc, rx, ry);
        m->hl_attr = at;
        m->display(DISPLAY);
        va_end(args);
        return (m->hl_id);
    }
    if (type == hlPoly) {
        Point *pts = va_arg(args, Point*);
        int numpts = va_arg(args, int);
        int at = va_arg(args, int);
        if (numpts < 2) {
            va_end(args);
            return (0);
        }

        hlite_t *m = MK.new_user_mark(CurCell());
        if (!m) {
            va_end(args);
            return (0);
        }
        m->poly_init(pts, numpts);
        m->hl_attr = at;
        m->display(DISPLAY);
        va_end(args);
        return (m->hl_id);
    }
    if (type == hlText) {
        char *str = va_arg(args, char*);
        int x = va_arg(args, int);
        int y = va_arg(args, int);
        int wid = va_arg(args, int);
        int hei = va_arg(args, int);
        int xf = va_arg(args, int);
        int at = va_arg(args, int);
        at &= ~hlaTextured;
        if (!str) {
            va_end(args);
            return (0);
        }
        if (wid <= 0 || hei <= 0) {
            int w, h;
            DefaultLabelSize(str, Physical, &w, &h);
            if (wid > 0)
                hei = (wid*h)/w;
            else if (hei > 0)
                wid = (hei*w)/h;
            else {
                wid = w;
                hei = h;
            }
        }

        hlite_t *m = MK.new_user_mark(CurCell());
        if (!m) {
            va_end(args);
            return (0);
        }
        m->text_init(str, x, y, wid, hei, xf);
        m->hl_attr = at;
        m->display(DISPLAY);
        va_end(args);
        return (m->hl_id);
    }
    va_end(args);
    return (0);
}


bool
cDisplay::RemoveUserMark(int id)
{
    return (MK.remove_user_mark(CurCell(), id));
}


bool
cDisplay::RemoveUserMarkAt(const BBox *AOI)
{
    return (MK.remove_user_mark(CurCell(), AOI));
}


void
cDisplay::ClearUserMarks(const CDs *sd)
{
    MK.clear_user_marks(sd);
}


int
cDisplay::DumpUserMarks(const char *fname, const CDs *sd)
{
    return (MK.dump_user_marks(fname, sd));
}


namespace {
    char *rdline(FILE *fp)
    {
        sLstr lstr;
        int c;
        while ((c = getc(fp)) != EOF) {
            lstr.add_c(c);
            if (c == '\n')
                return (lstr.string_trim());
        }
        return (lstr.string_trim());
    }
}


int
cDisplay::ReadUserMarks(const char *fname)
{
    char buf[256];
    if (!fname || !*fname) {
        CDs *sd = CurCell();
        if (!sd) {
            Errs()->add_error("ReadUserMarks: no current cell!");
            return (-1);
        }
        sprintf(buf, "%s.%s.marks", sd->cellname()->string(),
            sd->isElectrical() ? "elec" : "phys");
        fname = buf;
    }
    FILE *fp = fopen(fname, "r");
    if (!fp) {
        Errs()->add_error("ReadUserMarks: can't open %s.", fname);
        Errs()->sys_error("fopen");
        return(-1);
    }
    char *s = rdline(fp);
    if (!s) {
        Errs()->add_error("ReadUserMarks: no data in file.");
        fclose(fp);
        return (-1);
    }
    // The first line is in the form <cellname> <elec|phys>
    char *t = s;
    char *cn = lstring::getqtok(&t);
    char *md = lstring::gettok(&t);
    delete [] s;
    if (!md) {
        Errs()->add_error("ReadUserMarks: data format error in file line 1.");
        delete [] cn;
        fclose(fp);
        return (-1);
    }

    DisplayMode mode;
    if (*md == 'e' || *md == 'E')
        mode = Electrical;
    else if (*md == 'p' || *md == 'P')
        mode = Physical;
    else {
        Errs()->add_error(
            "ReadUserMarks: unknown display mode in file line 1.");
        delete [] cn;
        delete [] md;
        fclose(fp);
        return (-1);
    }
    delete [] md;

    CDs *sd = CDcdb()->findCell(cn, mode);
    if (!sd) {
        Errs()->add_error("ReadUserMarks: %s cell not found.",
            mode == Physical ? "Physical" : "Electrical");
        fclose(fp);
        return (-1);
    }

    int mcnt = 0;
    for (;;) {
        s = rdline(fp);
        if (!s)
            break;
        hlite_t *mx = hlite_t::parse(s);
        delete [] s;
        if (mx) {
            mcnt++;
            MK.add_user_mark(sd, mx);
            if (sd == CurCell())
                mx->display(DISPLAY);
        }
    }
    fclose(fp);
    return (mcnt);
}

