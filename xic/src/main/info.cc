
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
 $Id: info.cc,v 5.176 2016/05/31 06:23:25 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "cd_lgen.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_layer.h"
#include "cd_terminal.h"
#include "cd_celldb.h"
#include "cd_netname.h"
#include "fio.h"
#include "events.h"
#include "menu.h"
#include "view_menu.h"
#include "promptline.h"
#include "select.h"
#include "layertab.h"
#include "pathlist.h"
#include "texttf.h"


//
// The Info command, show information about objects.
//

namespace {
    // A list for object *copies*, used for potential selections.  For
    // each object, a "stack" of parent instance descriptors is kept.
    //
    struct tlst_t
    {
        tlst_t(CDo *od, CDclxy *cl, tlst_t *nx)
            {
                next = nx;
                odesc = od;
                cdescs = cl;
            }
        ~tlst_t()
            {
                delete odesc;
                CDclxy::destroy(cdescs);
            }

        static void destroy(tlst_t *tl0)
            {
                while (tl0) {
                    tlst_t *t = tl0;
                    tl0 = tl0->next;
                    delete t;
                }
            }

        tlst_t *next;
        CDo *odesc;         // object
        CDclxy *cdescs;     // containing cell instances, walking up hierarchy
    };

    namespace main_info {
        struct InfoState : public CmdState
        {
            friend void cMain::InfoExec(CmdDesc*);
            friend void cMain::ShowCellInfo(const char*, bool, DisplayMode);

            InfoState(const char*, const char*);
            virtual ~InfoState();

            CDclxy *StackList();

            void b1down() { cEventHdlr::sel_b1down(); }
            void b1up();
            void b1up_altw();
            bool key(int, const char*, int);
            void esc();
            void undo() { cEventHdlr::sel_undo(); }
            void redo() { cEventHdlr::sel_redo(); }

            void refresh()
                {
                    if (TmpObj)
                        show_obj_info(TmpObj);
                    else if (TmpCell)
                        show_cell_info(TmpCell);
                }

        private:
            tlst_t *find_obj(WindowDesc*, const BBox*);
            void show_cell_expand(bool, bool);

            static void info_to_file(FILE*);
            static void show_cell_info(CDs*);
            static void show_obj_info(tlst_t*);
            static bool info_cb(bool, void*);

            CDol *SelBack;
            tlst_t *ObjList;
            tlst_t *TmpObj;
            CDs *TmpCell;
            CDol *ExpObjs;
            bool ExpMode;

            static const char *msg;
        };

        InfoState *InfoCmd;
    }
}

using namespace main_info;

const char *InfoState::msg =
    "Click outside of current cell boundary or hold\n"
    "Shift for current cell info.  Click on objects to\n"
    "see information about the object.  With an object\n"
    "selected, press Enter to toggle selection of all\n"
    "objects in containing subcell.\n\n";


void
cMain::InfoExec(CmdDesc *cmd)
{
    if (!InfoCmd) {

        // If there is something in the selection queue, give the user the
        // option of dumping all the info to a file
        unsigned int nsel;
        Selections.countQueue(CurCell(), &nsel, 0);
        if (nsel > 0) {
            char *in = PL()->EditPrompt(
                "Dump info on selected objects to file? ", "n");
            in = lstring::strip_space(in);
            if (!in) {
                PL()->ErasePrompt();
                if (cmd && cmd->caller)
                    Menu()->Deselect(cmd->caller);
                return;
            }
            if (*in == 'y' || *in == 'Y') {
                if (cmd && cmd->caller)
                    Menu()->Deselect(cmd->caller);
                in = XM()->SaveFileDlg("File name for info? ", "infolist.txt");
                if (!in) {
                    PL()->ErasePrompt();
                    return;
                }
                char *fname = pathlist::expand_path(in, false, true);
                FILE *fp = fopen(fname, "w");
                if (!fp) {
                    PL()->ShowPromptV("Error: can't open file %s.", fname);
                    delete [] fname;
                    return;
                }
                InfoState::info_to_file(fp);
                fclose(fp);
                in = PL()->EditPrompt("Done.  View file? ", "y");
                in = lstring::strip_space(in);
                PL()->ErasePrompt();
                if (!in) {
                    delete [] fname;
                    return;
                }
                if (*in == 'y' || *in == 'Y')
                    DSPmainWbag(PopUpFileBrowser(fname))
                delete [] fname;
                return;
            }
        }

        InfoCmd = new InfoState("INFO", "xic:info");

        sSelGen sg(Selections, CurCell());
        CDo *od;
        while ((od = sg.next()) != 0)
            InfoCmd->SelBack = new CDol(od, InfoCmd->SelBack);

        Selections.deselectTypes(CurCell(), 0);
        if (!EV()->PushCallback(InfoCmd)) {
            delete InfoCmd;
            return;
        }
    }
    DSPmainWbag(PopUpInfo2(MODE_ON, InfoState::msg, InfoState::info_cb, 0,
        STY_FIXED))
    GRtextPopup *info2 = DSPmainWbagRet(ActiveInfo2());
    if (info2)
        info2->set_btn2_state(true);
}


// Update displayed info after a change (undo/redo).
//
void
cMain::InfoRefresh()
{
    if (InfoCmd)
        InfoCmd->refresh();
}


// Hook for the info command in the cells popup.
//
void
cMain::ShowCellInfo(const char *name, bool init, DisplayMode mode)
{
    if (!InfoCmd) {
        if (!init)
            return;
        Selections.deselectTypes(CurCell(), 0);
        InfoCmd = new InfoState("INFO", "xic:cells#info");
        if (!EV()->PushCallback(InfoCmd)) {
            delete InfoCmd;
            InfoCmd = 0;
            return;
        }
    }
    if (name && *name) {
        CDs *sdesc = 0;
        CDcbin cbin;
        if (CDcdb()->findSymbol(name, &cbin))
            sdesc = cbin.celldesc(mode);
        if (!sdesc)
            return;
        char *str = Info(sdesc, 100);
        DSPmainWbag(PopUpInfo2(MODE_ON, str, InfoState::info_cb, 0, STY_FIXED))
        delete [] str;
    }
    else
        DSPmainWbag(PopUpInfo2(MODE_ON, InfoState::msg, InfoState::info_cb, 0,
            STY_FIXED))
    GRtextPopup *info2 = DSPmainWbagRet(ActiveInfo2());
    if (info2)
        info2->set_btn2_state(true);
}


// This export is used by the Push command.  The Push command will
// push the editing context to the cell containing the currently
// selected object.  The returned CDclxl is in top-down order and
// should be freed by the caller.
//
CDclxy *
cMain::GetPushList()
{
    if (InfoCmd)
        return (InfoCmd->StackList());
    return (0);
}


// Return a string containing formatted infor about the cell.  If
// level > 0 be more verbose.
//
char *
cMain::Info(CDs *sdesc, int level)
{
    if (!sdesc)
        return (0);
    sLstr lstr;
    char buf[512];
    sprintf(buf, "%s Cell: %s\n", DisplayModeName(sdesc->displayMode()),
        Tstring(sdesc->cellname()));
    lstr.add(buf);
    const BBox *tBB = sdesc->BB();
    bool printint = (CDvdb()->getVariable(VA_InfoInternal) != 0);
    int ndgt = CD()->numDigits();
    if (printint) {
        sprintf(buf, "Bounding box: %d,%d %d,%d\n", tBB->left,
            tBB->bottom, tBB->right, tBB->top);
    }
    else if (sdesc->isElectrical()) {
        sprintf(buf, "Bounding box: %.3f,%.3f %.3f,%.3f\n",
            ELEC_MICRONS(tBB->left), ELEC_MICRONS(tBB->bottom),
            ELEC_MICRONS(tBB->right), ELEC_MICRONS(tBB->top));
    }
    else {
        sprintf(buf, "Bounding box: %.*f,%.*f %.*f,%.*f\n",
            ndgt, MICRONS(tBB->left), ndgt, MICRONS(tBB->bottom),
            ndgt, MICRONS(tBB->right), ndgt, MICRONS(tBB->top));
    }
    lstr.add(buf);

    if (level > 0) {
        if (sdesc->isModified())
            lstr.add("Cell is modified\n");

        lstr.add("Source file type: ");
        const char *tn = FIO()->TypeName(sdesc->fileType());
        if (!tn)
            tn = "none";
        lstr.add(tn);
        lstr.add("\n");

        lstr.add("Source file name: ");
        const char *fn = sdesc->fileName();
        if (!fn)
            fn = "unknown";
        lstr.add(fn);
        lstr.add("\n");

        sprintf(buf, "Flags:  (%x hex)", sdesc->getFlags());
        if (!sdesc->getFlags()) {
            strcat(buf, "none\n");
            lstr.add(buf);
        }
        else {
            lstr.add(buf);
            lstr.add("\n");
            for (FlagDef *f = SdescFlags; f->name; f++) {
                if (sdesc->getFlags() & f->value) {
                    sprintf(buf, "  %s (%s)\n", f->name, f->desc);
                    lstr.add(buf);
                }
            }
        }
    }

    if (level > 0) {
        stringnumlist *list = 0;
        int count = sdesc->listParents(&list, false);
        int counte = 0;
        if (list) {
            stringnumlist *liste = 0;
            counte = sdesc->listParents(&liste, true);
            stringnumlist::sort_by_string(list);
            stringnumlist::sort_by_string(liste);
            lstr.add("Parent cells:\n");
            while (list) {
                if (!liste || strcmp(list->string, liste->string)) {
                    // can't happen;
                    stringnumlist::destroy(list);
                    stringnumlist::destroy(liste);
                    break;
                }
                sprintf(buf, "  %-24s %6d %6d\n", list->string, list->num,
                    liste->num);
                lstr.add(buf);
                stringnumlist *x = list;
                list = list->next;
                delete [] x->string;
                delete x;
                x = liste;
                liste = liste->next;
                delete [] x->string;
                delete x;
            }
        }
        sprintf(buf, "Total instantiations: %d  elements: %d\n", count,
            counte);
    }
    else {
        int count = sdesc->listParents(0, false);
        int counte = sdesc->listParents(0, true);
        sprintf(buf, "Instantiation count: %d\n", count);
        sprintf(buf, "Total instantiations: %d  elements: %d\n", count,
            counte);
    }
    lstr.add(buf);
    if (level > 0) {
        stringnumlist *list = 0;
        int count = sdesc->listSubcells(&list, false);

        int counte = 0;
        if (list) {
            stringnumlist *liste = 0;
            counte = sdesc->listSubcells(&liste, true);
            stringnumlist::sort_by_string(list);
            stringnumlist::sort_by_string(liste);
            lstr.add("Subcells:\n");
            while (list) {
                if (!liste || strcmp(list->string, liste->string)) {
                    // can't happen;
                    stringnumlist::destroy(list);
                    stringnumlist::destroy(liste);
                    break;
                }
                sprintf(buf, "  %-24s %6d %6d\n", list->string, list->num,
                    liste->num);
                lstr.add(buf);
                stringnumlist *x = list;
                list = list->next;
                delete [] x->string;
                delete x;
                x = liste;
                liste = liste->next;
                delete [] x->string;
                delete x;
            }
        }
        sprintf(buf, "Instances: %d  elements: %d\n", count, counte);
    }
    else {
        int count = sdesc->listSubcells(0, false);
        int counte = sdesc->listSubcells(0, true);
        sprintf(buf, "Instances: %d  elements: %d\n", count, counte);
    }
    lstr.add(buf);

    int boxcnt = 0;
    int wirecnt = 0;
    int polycnt = 0;
    int labelcnt = 0;
    CDl *ldesc;
    CDsLgen lgen(sdesc);
    while ((ldesc = lgen.next()) != 0) {
        CDg gdesc;
        gdesc.init_gen(sdesc, ldesc);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            switch (odesc->type()) {
            case CDBOX:
                boxcnt++;
                break;
            case CDWIRE:
                wirecnt++;
                break;
            case CDPOLYGON:
                polycnt++;
                break;
            case CDLABEL:
                labelcnt++;
                break;
            }
        }
    }

    sprintf(buf, "Boxes: %d\n", boxcnt);
    lstr.add(buf);
    sprintf(buf, "Wires: %d\n", wirecnt);
    lstr.add(buf);
    sprintf(buf, "Polygons: %d\n", polycnt);
    lstr.add(buf);
    sprintf(buf, "Labels: %d\n", labelcnt);
    lstr.add(buf);

    CDcbin cbin(sdesc);
    CDp_snode *ps = (CDp_snode*)(cbin.elec() ?
            cbin.elec()->prpty(P_NODE) : 0);
    int tcnt = 0;
    for ( ; ps; ps = ps->next())
        tcnt++;
    if (tcnt) {
        sprintf(buf, "Cell Terminals: %d\n", tcnt);
        lstr.add(buf);
        if (level > 0) {
            ps = (CDp_snode*)cbin.elec()->prpty(P_NODE);
            for ( ; ps; ps = ps->next()) {
                sprintf(buf, "  Name: %s\n", Tstring(ps->term_name()));
                lstr.add(buf);
                FlagDef *f;
                for (f = TermTypes; f->name; f++) {
                    if (f->value == (unsigned int)ps->termtype())
                        break;
                }
                sprintf(buf, "    Type: %s\n", f->name ? f->name : "bogus!");
                lstr.add(buf);
                bool fnd = false;
                unsigned int flgs = ps->term_flags();
                for (f = TermFlags; f->name; f++) {
                    if (flgs & f->value) {
                        sprintf(buf, "      %s (%s)\n", f->name, f->desc);
                        if (!fnd) {
                            fnd = true;
                            lstr.add("    Flags:\n");
                        }
                        lstr.add(buf);
                    }
                }
                CDsterm *t = sdesc->isElectrical() ? 0 : ps->cell_terminal();
                if (t) {
                    if (printint)
                        sprintf(buf, "    Location: %d,%d\n", t->lx(), t->ly());
                    else {
                        sprintf(buf, "    Location: %.*f,%.*f\n",
                            ndgt, MICRONS(t->lx()), ndgt, MICRONS(t->ly()));
                    }
                    lstr.add(buf);
                    sprintf(buf, "    Group: %d\n", t->group());
                    lstr.add(buf);
                    CDl *tld = t->layer();
                    const char *lname = tld ? tld->name() : "none";
                    sprintf(buf, "    Layer: %s\n", lname);
                    lstr.add(buf);
                }
            }
        }
    }

    tcnt = 0;
    CDp_bsnode *pbs = (CDp_bsnode*)(cbin.elec() ?
            cbin.elec()->prpty(P_BNODE) : 0);
    for ( ; pbs; pbs = pbs->next())
        tcnt++;
    if (tcnt) {
        sprintf(buf, "Cell Bus Connections: %d\n", tcnt);
        lstr.add(buf);
        if (level > 0) {
            pbs = (CDp_bsnode*)cbin.elec()->prpty(P_BNODE);
            for ( ; pbs; pbs = pbs->next()) {
                sLstr tstr;
                pbs->add_label_text(&tstr);
                if (tstr.string()) {
                    lstr.add("  Name ");
                    lstr.add(tstr.string());
                    lstr.add_c(',');
                }
                lstr.add("  Index ");
                lstr.add_i(pbs->index());
                lstr.add(",  Range ");
                lstr.add_i(pbs->beg_range());
                lstr.add_c('-');
                lstr.add_i(pbs->end_range());
                lstr.add_c('\n');

                bool fnd = false;
                unsigned int flgs = pbs->flags();
                for (FlagDef *f = TermFlags; f->name; f++) {
                    if (flgs & f->value) {
                        sprintf(buf, "      %s (%s)\n", f->name, f->desc);
                        if (!fnd) {
                            fnd = true;
                            lstr.add("    Flags:\n");
                        }
                        lstr.add(buf);
                    }
                }
            }
        }
    }

    if (sdesc->prptyList()) {
        int cnt = 0;
        for (CDp *pdesc = sdesc->prptyList(); pdesc; pdesc = pdesc->next_prp())
            cnt++;
        sprintf(buf, "Properties: %d\n", cnt);
        lstr.add(buf);
        if (level > 0) {
            for (CDp *pdesc = sdesc->prptyList(); pdesc;
                    pdesc = pdesc->next_prp()) {
                char *s;
                if (pdesc->string(&s)) {
                    sprintf(buf, "  %d ", pdesc->value());
                    lstr.add(buf);
                    lstr.add(s);
                    lstr.add_c('\n');
                    delete [] s;
                }
                else {
                    sprintf(buf, "  %d (printing error!)\n", pdesc->value());
                    lstr.add(buf);
                }
            }
        }
    }
    return (lstr.string_trim());
}


namespace {
    void print_inst_terms(const CDc *ecdesc, int vecix, sLstr &lstr, char *buf,
        bool printint, int ndgt)
    {
        if (!ecdesc)
            return;
        CDp_range *pr = (CDp_range*)ecdesc->prpty(P_RANGE);
        CDp_cnode *pc = (CDp_cnode*)ecdesc->prpty(P_NODE);
        int tcnt = 0;
        for ( ; pc; pc = pc->next())
            tcnt++;
        if (tcnt) {
            sprintf(buf, "Instance Terminals: %d\n", tcnt);
            lstr.add(buf);
            CDp_cnode *pct = (CDp_cnode*)ecdesc->prpty(P_NODE);
            for ( ; pct; pct = pct->next()) {
                if (vecix > 0 && pr)
                    pc = pr->node(0, vecix, pct->index());
                else
                    pc = pct;

                sprintf(buf, "  Name %s\n", Tstring(pc->term_name()));
                lstr.add(buf);
                sprintf(buf, "  Node %d\n", pc->enode());
                lstr.add(buf);
                FlagDef *f;
                for (f = TermTypes; f->name; f++) {
                    if (f->value == (unsigned int)pc->termtype())
                        break;
                }
                sprintf(buf, "    Type %s\n", f->name ? f->name : "bogus!");
                lstr.add(buf);
                unsigned int flgs = pc->term_flags();
                bool fnd = false;
                for (f = TermFlags; f->name; f++) {
                    if (flgs & f->value) {
                        sprintf(buf, "      %s (%s)\n", f->name, f->desc);
                        if (!fnd) {
                            fnd = true;
                            lstr.add("    Flags:\n");
                        }
                        lstr.add(buf);
                    }
                }
                CDcterm *t = pc->inst_terminal();
                if (t) {
                    if (printint)
                        sprintf(buf, "    Location %d,%d\n", t->lx(),
                            t->ly());
                    else
                        sprintf(buf, "    Location %.*f,%.*f\n",
                            ndgt, MICRONS(t->lx()),
                            ndgt, MICRONS(t->ly()));
                    lstr.add(buf);
                    sprintf(buf, "    Group %d\n", t->group());
                    lstr.add(buf);
                    CDl *tld = t->layer();
                    const char *lname = tld ? tld->name() : "none";
                    sprintf(buf, "    Layer %s\n", lname);
                    lstr.add(buf);
                }
            }
        }

        tcnt = 0;
        CDp_bcnode *pbc = (CDp_bcnode*)ecdesc->prpty(P_BNODE);
        for ( ; pbc; pbc = pbc->next())
            tcnt++;
        if (tcnt) {
            sprintf(buf, "Instance Bus Connections: %d\n", tcnt);
            lstr.add(buf);
            pbc = (CDp_bcnode*)ecdesc->prpty(P_BNODE);
            for ( ; pbc; pbc = pbc->next()) {
                sprintf(buf, "  Index %d,  Range %d-%d\n", pbc->index(),
                    pbc->beg_range(), pbc->end_range());
                lstr.add(buf);
            }
        }
    }
}


char *
cMain::Info(const CDo *odesc)
{
    if (!odesc)
        return (0);
    const CDo *ocpy = 0;
    if (odesc->is_copy() && odesc->const_next_odesc() &&
            !odesc->const_next_odesc()->is_copy()) {
        // This is a copy, and the next_odesc() points to a non-copy. 
        // assume that this is the "real" object from the database. 
        // The copy is a representation transformed to top-level
        // coords.  This might not be the same object type!  E.g.,
        // 45-rotated boxes have a polygon representation.

        ocpy = odesc;
        odesc = odesc->const_next_odesc();
    }
    DisplayMode dmode = DSP()->CurMode();

    sLstr lstr;
    lstr.add("-----------------\n");
    switch (odesc->type()) {
    case CDBOX:
        lstr.add("**     Box     **\n");
        break;
    case CDWIRE:
        lstr.add("**    Wire     **\n");
        break;
    case CDPOLYGON:
        lstr.add("**   Polygon   **\n");
        break;
    case CDLABEL:
        lstr.add("**    Label    **\n");
        break;
    case CDINSTANCE:
        lstr.add("**   Instance  **\n");
        break;
    }
    lstr.add("-----------------\n");

    char buf[256];
    int ndgt = CD()->numDigits();
    if (odesc->type() == CDWIRE) {
        // The ocpy is guaranteed to also be a wire.
        if (ocpy && ocpy->type() != odesc->type())
            ocpy = 0;
        if (CDvdb()->getVariable(VA_InfoInternal)) {
            if (ocpy)
                sprintf(buf, "Width: %d  (at top level: %d)\n",
                    ((CDw*)odesc)->wire_width(), ((CDw*)ocpy)->wire_width());
            else
                sprintf(buf, "Width: %d\n", ((CDw*)odesc)->wire_width());
        }
        else {
            if (dmode == Physical) {
                if (ocpy)
                    sprintf(buf, "Width: %.*f  (at top level: %.*f) \n",
                        ndgt, MICRONS(((CDw*)odesc)->wire_width()),
                        ndgt, MICRONS(((CDw*)ocpy)->wire_width()));
                else
                    sprintf(buf, "Width: %.*f\n",
                        ndgt, MICRONS(((CDw*)odesc)->wire_width()));
            }
            else {
                if (ocpy)
                    sprintf(buf, "Width: %.*f  (at top level: %.*f) \n",
                        3, ELEC_MICRONS(((CDw*)odesc)->wire_width()),
                        3, ELEC_MICRONS(((CDw*)ocpy)->wire_width()));
                else
                    sprintf(buf, "Width: %.*f\n",
                        3, ELEC_MICRONS(((CDw*)odesc)->wire_width()));
            }
        }
        lstr.add(buf);

        lstr.add("Style: ");
        switch (((CDw*)odesc)->wire_style()) {
        case 0:
            lstr.add("0 (flush-end)\n");
            break;
        case 1:
            lstr.add("1 (rounded-end)\n");
            break;
        case 2:
            lstr.add("2 (extended-end)\n");
            break;
        default:
            lstr.add("unknown\n");
        }
    }
    else if (odesc->type() == CDLABEL) {
        // The ocpy is guaranteed to also be a label.
        if (ocpy && ocpy->type() != odesc->type())
            ocpy = 0;
        lstr.add("Text: ");
        char *s = hyList::string(((CDla*)odesc)->label(), HYcvPlain, false);
        lstr.add(s);
        lstr.add_c('\n');
        delete [] s;
        if (CDvdb()->getVariable(VA_InfoInternal)) {
            if (ocpy)
                sprintf(buf, "Width: %d  (at top level: %d)\n",
                    ((CDla*)odesc)->width(), ((CDla*)ocpy)->width());
            else
                sprintf(buf, "Width: %d\n", ((CDla*)odesc)->width());
            lstr.add(buf);
            if (ocpy)
                sprintf(buf, "Height: %d  (at top level: %d)\n",
                    ((CDla*)odesc)->height(), ((CDla*)ocpy)->height());
            else
                sprintf(buf, "Height: %d\n", ((CDla*)odesc)->height());
            lstr.add(buf);
            if (ocpy)
                sprintf(buf, "Coord: %d, %d  (at top level: %d, %d)\n",
                    ((CDla*)odesc)->xpos(), ((CDla*)odesc)->ypos(),
                    ((CDla*)ocpy)->xpos(), ((CDla*)ocpy)->ypos());
            else
                sprintf(buf, "Coord:  %d, %d\n", ((CDla*)odesc)->xpos(),
                    ((CDla*)odesc)->ypos());
            lstr.add(buf);
        }
        else {
            if (dmode == Physical) {
                if (ocpy)
                    sprintf(buf, "Width: %.*f  (at top level: %.*f)\n",
                        ndgt, MICRONS(((CDla*)odesc)->width()),
                        ndgt, MICRONS(((CDla*)ocpy)->width()));
                else
                    sprintf(buf, "Width: %.*f\n",
                        ndgt, MICRONS(((CDla*)odesc)->width()));
                lstr.add(buf);
                if (ocpy)
                    sprintf(buf, "Height: %.*f  (at top level: %.*f)\n",
                        ndgt, MICRONS(((CDla*)odesc)->height()),
                        ndgt, MICRONS(((CDla*)ocpy)->height()));
                else
                    sprintf(buf, "Height: %.*f\n",
                        ndgt, MICRONS(((CDla*)odesc)->height()));
                lstr.add(buf);
                if (ocpy)
                    sprintf(buf, "Coord: %.*f, %.*f  (at top level: %.*f, %.*f)\n",
                        ndgt, MICRONS(((CDla*)odesc)->xpos()),
                        ndgt, MICRONS(((CDla*)odesc)->ypos()),
                        ndgt, MICRONS(((CDla*)ocpy)->xpos()),
                        ndgt, MICRONS(((CDla*)ocpy)->ypos()));
                else
                    sprintf(buf, "Coord:  %.*f, %.*f\n",
                        ndgt, MICRONS(((CDla*)odesc)->xpos()),
                        ndgt, MICRONS(((CDla*)odesc)->ypos()));
                lstr.add(buf);
            }
            else {
                if (ocpy)
                    sprintf(buf, "Width: %.*f  (at top level: %.*f)\n",
                        3, ELEC_MICRONS(((CDla*)odesc)->width()),
                        3, ELEC_MICRONS(((CDla*)ocpy)->width()));
                else
                    sprintf(buf, "Width: %.*f\n",
                        3, ELEC_MICRONS(((CDla*)odesc)->width()));
                lstr.add(buf);
                if (ocpy)
                    sprintf(buf, "Height: %.*f  (at top level: %.*f)\n",
                        3, ELEC_MICRONS(((CDla*)odesc)->height()),
                        3, ELEC_MICRONS(((CDla*)ocpy)->height()));
                else
                    sprintf(buf, "Height: %.*f\n",
                        3, ELEC_MICRONS(((CDla*)odesc)->height()));
                lstr.add(buf);
                if (ocpy)
                    sprintf(buf,
                        "Coord: %.*f, %.*f  (at top level: %.*f, %.*f)\n",
                        3, ELEC_MICRONS(((CDla*)odesc)->xpos()),
                        3, ELEC_MICRONS(((CDla*)odesc)->ypos()),
                        3, ELEC_MICRONS(((CDla*)ocpy)->xpos()),
                        3, ELEC_MICRONS(((CDla*)ocpy)->ypos()));
                else
                    sprintf(buf, "Coord:  %.*f, %.*f\n",
                        3, ELEC_MICRONS(((CDla*)odesc)->xpos()),
                        3, ELEC_MICRONS(((CDla*)odesc)->ypos()));
                lstr.add(buf);
            }
        }

        int n = ((CDla*)odesc)->xform();
        sprintf(buf, "Transform:\n Code: %d\n", n);
        lstr.add(buf);
        buf[0] = 0;
        int r = (n & TXTF_ROT)*90;
        if (n & TXTF_45)
            r += 45;
        if (r)
            sprintf(buf, " Rot%d", r);
        if (n & TXTF_MY)
            strcat(buf, " MirY");
        if (n & TXTF_MX)
            strcat(buf, " MirX");
        if (n & TXTF_HJR)
            strcat(buf, " HJustRight");
        else if (n & TXTF_HJC)
            strcat(buf, " HJustCenter");
        else
            strcat(buf, " HJustLeft");
        if (n & TXTF_VJT)
            strcat(buf, " VJustTop");
        else if (n & TXTF_VJC)
            strcat(buf, " VJustCenter");
        else
            strcat(buf, " VJustBottom");
        strcat(buf, "\n");
        lstr.add(buf);
        if (ocpy) {
            n = ((CDla*)ocpy)->xform();
            sprintf(buf, "at top level:\n Code: %d\n", n);
            lstr.add(buf);
            buf[0] = 0;
            r = (n & TXTF_ROT)*90;
            if (n & TXTF_45)
                r += 45;
            if (r)
                sprintf(buf, " Rot%d", r);
            if (n & TXTF_MY)
                strcat(buf, " MirY");
            if (n & TXTF_MX)
                strcat(buf, " MirX");
            if (n & TXTF_HJR)
                strcat(buf, " HJustRight");
            else if (n & TXTF_HJC)
                strcat(buf, " HJustCenter");
            else
                strcat(buf, " HJustLeft");
            if (n & TXTF_VJT)
                strcat(buf, " VJustTop");
            else if (n & TXTF_VJC)
                strcat(buf, " VJustCenter");
            else
                strcat(buf, " VJustBottom");
            strcat(buf, "\n");
            lstr.add(buf);
        }
    }
    if (odesc->type() != CDINSTANCE) {
        sprintf(buf, "Layer: %s\n", odesc->ldesc()->name());
        lstr.add(buf);
    }
    else {
        sprintf(buf, "Master: %s\n", Tstring(((CDc*)odesc)->cellname()));
        lstr.add(buf);
    }
    if (CDvdb()->getVariable(VA_InfoInternal)) {
        sprintf(buf, "Bounding Box: %d,%d %d,%d\n", odesc->oBB().left,
            odesc->oBB().bottom, odesc->oBB().right, odesc->oBB().top);
        lstr.add(buf);
        if (ocpy) {
            sprintf(buf, "at top level:: %d,%d %d,%d\n", ocpy->oBB().left,
                ocpy->oBB().bottom, ocpy->oBB().right, ocpy->oBB().top);
            lstr.add(buf);
        }
    }
    else {
        if (dmode == Physical) {
            sprintf(buf, "Bounding Box: %.*f,%.*f %.*f,%.*f\n",
                ndgt, MICRONS(odesc->oBB().left),
                ndgt, MICRONS(odesc->oBB().bottom),
                ndgt, MICRONS(odesc->oBB().right),
                ndgt, MICRONS(odesc->oBB().top));
            lstr.add(buf);
            if (ocpy) {
                sprintf(buf, "at top level: %.*f,%.*f %.*f,%.*f\n",
                    ndgt, MICRONS(ocpy->oBB().left),
                    ndgt, MICRONS(ocpy->oBB().bottom),
                    ndgt, MICRONS(ocpy->oBB().right),
                    ndgt, MICRONS(ocpy->oBB().top));
                lstr.add(buf);
            }
        }
        else {
            sprintf(buf, "Bounding Box: %.*f,%.*f %.*f,%.*f\n",
                3, ELEC_MICRONS(odesc->oBB().left),
                3, ELEC_MICRONS(odesc->oBB().bottom),
                3, ELEC_MICRONS(odesc->oBB().right),
                3, ELEC_MICRONS(odesc->oBB().top));
            lstr.add(buf);
            if (ocpy) {
                sprintf(buf, "at top level: %.*f,%.*f %.*f,%.*f\n",
                    3, ELEC_MICRONS(ocpy->oBB().left),
                    3, ELEC_MICRONS(ocpy->oBB().bottom),
                    3, ELEC_MICRONS(ocpy->oBB().right),
                    3, ELEC_MICRONS(ocpy->oBB().top));
                lstr.add(buf);
            }
        }
    }
    strcpy(buf, "Flags: ");
    if (!odesc->flags()) {
        strcat(buf, "none\n");
        lstr.add(buf);
    }
    else {
        lstr.add(buf);
        lstr.add("\n");
        int expf = (CDexpand | (CDexpand<<1) | (CDexpand<<2) |
            (CDexpand<<3) | (CDexpand<<4));
        if (odesc->has_flag(expf)) {
            int mask = CDexpand;
            char tbuf[8];
            char *s = tbuf;
            for (int i = 0; i < 5; i++) {
                if (odesc->has_flag(mask))
                    *s++ = '0' + i;
            }
            *s = 0;
            sprintf(buf, "  CDexpand (expansion in %s)\n", tbuf);
            lstr.add(buf);
        }

        for (FlagDef *f = OdescFlags; f->name; f++) {
            if (odesc->has_flag(f->value) && f->value != CDexpand) {
                sprintf(buf, "  %s (%s)\n", f->name, f->desc);
                lstr.add(buf);
            }
        }
    }

    strcpy(buf, "State:");
    if (odesc->state() == CDVanilla)
        strcat(buf, " Normal");
    else if (odesc->state() == CDSelected)
        strcat(buf, " Selected");
    else if (odesc->state() == CDDeleted)
        strcat(buf, " Deleted");
    else if (odesc->state() == CDIncomplete)
        strcat(buf, " Incomplete");
    else if (odesc->state() == CDInternal)
        strcat(buf, " Internal");
    else
        strcat(buf, " Bad State!");
    strcat(buf, "\n");
    lstr.add(buf);
    sprintf(buf, "Group: %d\n", odesc->group());
    lstr.add(buf);
    if (odesc->prpty_list()) {
        lstr.add("Properties:\n");
        for (CDp *pdesc = odesc->prpty_list(); pdesc;
                pdesc = pdesc->next_prp()) {
            char *s;
            if (pdesc->string(&s)) {
                sprintf(buf, "  %d ", pdesc->value());
                lstr.add(buf);
                lstr.add(s);
                if (pdesc->bound())
                    lstr.add(" (label reference)");
                lstr.add_c('\n');
                delete [] s;
            }
            else if (dmode == Electrical && pdesc->value() == P_RANGE) {
                // This is a phony range property added to accommodate
                // a non-unit m or nf parameter.

                sprintf(buf, "  %d (dummy range for M or NF param)\n",
                    pdesc->value());
                lstr.add(buf);
            }
            else {
                sprintf(buf, "  %d (printing error!)\n", pdesc->value());
                lstr.add(buf);
            }
        }
    }
#ifdef HAVE_COMPUTED_GOTO
    COMPUTED_JUMP(odesc)
#else
    CONDITIONAL_JUMP(odesc)
#endif
box:
    return (lstr.string_trim());
poly:
    {
        Otype o = ((const CDpo*)odesc)->po_winding();
        sprintf(buf, "Winding: %s\n",
            o == Ocw ? "cw" : (o == Occw ? "ccw" : "none"));
        lstr.add(buf);

        // This can change the vertex list (removes duplicates).
        int ret = ((CDpo*)odesc)->po_check_poly(PCHK_ALL, true);

        if (ret & PCHK_NVERTS)
            lstr.add("TOO FEW VERTICES, POLYGON BAD!\n");
        else if (ret & PCHK_OPEN)
            lstr.add("VERTEX LIST NOT CLOSED, POLYGON BAD!\n");
        else {
            if (ret & PCHK_REENT)
                lstr.add("WARNING, POLYGON IS BADLY FORMED OR REENTRANT.\n");
            if (ret & PCHK_ZERANG)
                lstr.add("WARNING, POLYGON CONTAINS ZERO DEGREE ANGLE.\n");
            else if (ret & PCHK_ACUTE)
                lstr.add("Polygon contains acute angle(s).\n");
            if (ret & PCHK_NON45)
                lstr.add("Polygon contains non-45 angle(s).\n");
        }
        int num = ((const CDpo*)odesc)->numpts();
        const Point *pts = ((const CDpo*)odesc)->points();
        sprintf(buf, "Coords (%d):\n", num);
        lstr.add(buf);
        int indgt = CDvdb()->getVariable(VA_InfoInternal) ? 0 : ndgt;
        char *s = cGEO::path_string(pts, num, indgt);
        lstr.add(s);
        lstr.add_c('\n');
        delete [] s;
        if (ocpy) {
            pts = ((const CDpo*)ocpy)->points();
            s = cGEO::path_string(pts, num, indgt);
            lstr.add("at top level:\n");
            lstr.add(s);
            lstr.add_c('\n');
            delete [] s;
        }

        lstr.add("Diffs:\n");
        s = cGEO::path_diff_string(pts, num, indgt);
        lstr.add(s);
        lstr.add_c('\n');
        delete [] s;

        return (lstr.string_trim());
    }
wire:
    {
        Point *p;
        int n;
        unsigned int flags;
        const Wire w = ((const CDw*)odesc)->w_wire();
        bool rt = w.toPoly(&p, &n, &flags);
        delete [] p;
        if (!rt) {
            lstr.add("WIRE CAN NOT BE RENDERED!\n");
            if (flags & CDWIRE_NOPTS)
                lstr.add("WARNING: Wire contains no vertex list.\n");
            if (flags & CDWIRE_BADWIDTH)
                lstr.add(
                    "WARNING: Wire has zero width and only one vertex.\n");
            if (flags & CDWIRE_BADSTYLE)
                lstr.add(
                    "WARNING: Wire has flush ends and only one vertex.\n");
        }
        else if (flags) {
            if (flags & CDWIRE_ONEVRT)
                lstr.add("WARNING: Wire contains only one vertex.\n");
            if (DSP()->CurMode() == Physical) {
                if (flags & CDWIRE_ZEROWIDTH)
                    lstr.add("WARNING: Wire width is zero.\n");
            }
            if (flags & CDWIRE_CLOSEVRTS)
                lstr.add(
                    "WARNING: Wire contains vertices spaced less than half "
                    "width.\n");
            if (flags & CDWIRE_CLIPFIX)
                lstr.add(
                    "WARNING: Wire requires special procesing to render.\n");
            if (flags & CDWIRE_BIGPOLY)
                lstr.add(
                    "WARNING: Wire requires very complex polygon to render.\n");
        }

        int num = ((const CDw*)odesc)->numpts();
        const Point *pts = ((const CDw*)odesc)->points();
        sprintf(buf, "Coords (%d):\n", num);
        lstr.add(buf);
        int indgt = CDvdb()->getVariable(VA_InfoInternal) ? 0 : ndgt;
        char *s = cGEO::path_string(pts, num, indgt);
        lstr.add(s);
        lstr.add_c('\n');
        delete [] s;
        if (ocpy) {
            pts = ((const CDw*)ocpy)->points();
            s = cGEO::path_string(pts, num, indgt);
            lstr.add("at top level:\n");
            lstr.add(s);
            lstr.add_c('\n');
            delete [] s;
        }

        lstr.add("Diffs:\n");
        s = cGEO::path_diff_string(pts, num, indgt);
        lstr.add(s);
        lstr.add_c('\n');
        delete [] s;

        return (lstr.string_trim());
    }
label:
    return (lstr.string_trim());
inst:
    {
        bool printint = (CDvdb()->getVariable(VA_InfoInternal) != 0);
        CDap ap((CDc*)odesc);
        if (ap.nx > 1 || ap.ny > 1) {
            lstr.add("Array: ");
            sprintf(buf, "nx=%d, ny=%d, ",
                ap.nx ? ap.nx : 1, ap.ny ? ap.ny : 1);
            lstr.add(buf);
            if (printint)
                sprintf(buf, "dx=%d, dy=%d\n", ap.dx, ap.dy);
            else {
                sprintf(buf, "dx=%.*f, dy=%.*f\n",
                    ndgt, MICRONS(ap.dx), ndgt, MICRONS(ap.dy));
            }
            lstr.add(buf);
        }
        lstr.add("Transform:\n");
        CDtx tx((CDc*)odesc);
        if (tx.refly)
            lstr.add("  Mirror Y\n");
        if (tx.ax != 1 || tx.ay != 0) {
            lstr.add("  Rotate ");
            const char *str = "??";
            if (tx.ay == 1) {
                if (tx.ax == 1)
                    str = "45";
                else if (tx.ax == 0)
                    str = "90";
                else if (tx.ax == -1)
                    str = "135";
            }
            else if (tx.ay == 0) {
                if (tx.ax == -1)
                    str = "180";
            }
            else if (tx.ay == -1) {
                if (tx.ax == -1)
                    str = "225";
                else if (tx.ax == 0)
                    str = "270";
                else if (tx.ax == 1)
                    str = "315";
            }
            lstr.add(str);
            lstr.add_c('\n');
        }
        if (tx.tx != 0 || tx.ty != 0) {
            if (printint)
                sprintf(buf, "  Translate %d %d\n", tx.tx, tx.ty);
            else
                sprintf(buf, "  Translate %.*f %.*f\n",
                    ndgt, MICRONS(tx.tx), ndgt, MICRONS(tx.ty));
            lstr.add(buf);
        }
        if (tx.magn > 0 && tx.magn != 1.0) {
            sprintf(buf, "  Magnify %g\n", tx.magn);
            lstr.add(buf);
        }

        CDs *msd = ((CDc*)odesc)->masterCell();
        if (msd) {
            if (msd->isElectrical()) {
                CDc *ecdesc = (CDc*)odesc;
                CDp_range *pr = (CDp_range*)ecdesc->prpty(P_RANGE);
                CDgenRange gen(pr);
                int vecix = 0;
                if (pr) {
                    sprintf(buf, "Instance is vectored:  %s%c%d:%d%c\n",
                        ecdesc->getBaseName(),
                        cTnameTab::subscr_open(), pr->beg_range(),
                        pr->end_range(), cTnameTab::subscr_close());
                    lstr.add(buf);
                }
                while (gen.next(0)) {
                    if (pr) {
                        char *s = ecdesc->getInstName(vecix);
                        sprintf(buf, "Component: %s\n", s);
                        delete [] s;
                        lstr.add(buf);
                    }
                    print_inst_terms(ecdesc, vecix, lstr, buf, printint, ndgt);
                    vecix++;
                }
            }
            else {
                bool isary = (ap.nx > 1 || ap.ny > 1);
                for (unsigned int iy = 0; iy < ap.ny; iy++) {
                    for (unsigned int ix = 0; ix < ap.nx; ix++) {
                        int vecix;
                        CDc *ecdesc = ((CDc*)odesc)->findElecDualOfPhys(
                            &vecix, ix, iy);
                        if (!ecdesc)
                            continue;
                        if (isary) {
                            sprintf(buf, "Component x=%-4d y=%-4d\n", ix, iy);
                            lstr.add(buf);
                            char *s = ecdesc->getInstName(vecix);
                            sprintf(buf, "Dual %s\n", s);
                            delete [] s;
                            lstr.add(buf);
                        }
                        print_inst_terms(ecdesc, vecix, lstr, buf, printint,
                            ndgt);
                    }
                }
            }
        }

        lstr.add("-----------------\n");
        if (((CDc*)odesc)->masterCell()) {
            char *s = XM()->Info(((CDc*)odesc)->masterCell(), 100);
            lstr.add(s);
            delete [] s;
        }
        return (lstr.string_trim());
    }
}
// End of sMain functions.


InfoState::InfoState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    SelBack = 0;
    ObjList = 0;
    TmpObj = 0;
    TmpCell = 0;
    ExpObjs = 0;
    ExpMode = false;
    Menu()->MenuButtonSet("view", MenuINFO, true);
}


InfoState::~InfoState()
{
    InfoCmd = 0;
    CDol::destroy(SelBack);
    tlst_t::destroy(ObjList);
    delete TmpObj;
    while (ExpObjs) {
        CDol *o = ExpObjs;
        ExpObjs = ExpObjs->next;
        delete o->odesc;
        delete o;
    }
    Menu()->MenuButtonSet("view", MenuINFO, false);
}


// Return a top-down list of the instance parents.
//
CDclxy *
InfoState::StackList()
{
    if (!TmpObj || !TmpObj->odesc)
        return (0);
    // The returned list is top-down, have to reverse order while
    // copying.
    CDclxy *c0 = 0;
    for (CDclxy *c = TmpObj->cdescs; c; c = c->next)
        c0 = new CDclxy(c->cdesc, c0, c->xind, c->yind);
    return (c0);
}


void
InfoState::b1up()
{
    BBox AOI;
    if (EV()->Cursor().get_downstate() & GR_SHIFT_MASK) {
        CDs *sdesc = 0;
        if (cEventHdlr::sel_b1up(&AOI, 0, B1UP_NOSEL)) {
            // Shift pressed, show cell info.
            if (EV()->CurrentWin())
                sdesc = EV()->CurrentWin()->CurCellDesc(
                    EV()->CurrentWin()->Mode());
        }
        show_cell_expand(false, true);
        show_cell_info(sdesc);
        Selections.deselectTypes(CurCell(), 0);
        return;
    }

    if (cEventHdlr::sel_b1up(&AOI, 0, B1UP_NOSEL)) {
        WindowDesc *wdesc = EV()->CurrentWin();
        if (wdesc) {
            int del = 1 + (int)(DSP()->PixelDelta()/wdesc->Ratio());
            int b = 2*del - AOI.width();
            if (b > 0) {
                AOI.left -= b;
                AOI.right += b;
            }
            b = del - AOI.height();
            if (b > 0) {
                AOI.bottom -= b;
                AOI.top += b;
            }
            CDs *sdesc = wdesc->CurCellDesc(wdesc->Mode());
            if (sdesc) {
                tlst_t *ol = find_obj(wdesc, &AOI);
                if (ol) {
                    show_cell_expand(false, true);
                    show_obj_info(ol);
                    Selections.deselectTypes(CurCell(), 0);
                    ol->odesc->set_state(CDVanilla);
                    Selections.insertObject(CurCell(), ol->odesc);
                    delete TmpObj;
                    TmpObj = ol;
                    TmpCell = 0;
                }
                else {
                    if (!AOI.intersect(sdesc->BB(), false)) {
                        // Outside of cell BB, show cell info.
                        show_cell_info(sdesc);
                        Selections.deselectTypes(CurCell(), 0);
                        TmpCell = sdesc;
                    }
                }
            }
        }
    }
}


// This receives button-up events from subwindows showing a different
// cell than the main window.  Show info about that cell, if any.
//
void
InfoState::b1up_altw()
{
    if (EV()->CurrentWin()) {
        CDs *sdesc = EV()->CurrentWin()->CurCellDesc(
            EV()->CurrentWin()->Mode());
        if (sdesc) {
            Selections.deselectTypes(CurCell(), 0);
            show_cell_info(sdesc);
        }
    }
}


bool
InfoState::key(int code, const char*, int)
{
    if (code == RETURN_KEY) {
        show_cell_expand(!ExpMode, false);
        return (true);
    }
    return (false);
}


// Esc entered, clean up and exit.
//
void
InfoState::esc()
{
    cEventHdlr::sel_esc();
    EV()->PopCallback(this);
    GRtextPopup *info2 = DSPmainWbagRet(ActiveInfo2());
    if (info2)
        info2->set_btn2_state(false);
    Selections.deselectTypes(CurCell(), 0);
    for (CDol *s = SelBack; s; s = s->next) {
        if (s->odesc) {
            s->odesc->set_state(CDVanilla);
            Selections.insertObject(CurCell(), s->odesc);
        }
    }
    delete this;
}

namespace {
    // The ocpy is a return from the sPF generator.  Instances are not
    // copied, the return is a box or poly representing the outline. 
    // The next_odesc() returns a pointer to the "real" object, which
    // will be of instance type for instances.
    //
    bool type_ok(const CDo *ocpy)
    {
        if (ocpy->next_odesc())
            ocpy = ocpy->next_odesc();
        return (Selections.typeSelected(ocpy));
    }
}


tlst_t *
InfoState::find_obj(WindowDesc *wdesc, const BBox *BB)
{
    // If the new location is on the present object, look in the list
    // of previously saved objects for a new object to return.  The
    // location must also be over the new object, and it must pass the
    // selection criteria.  Objects that don't match are thrown away.
    //
    if (TmpObj && TmpObj->odesc && ObjList &&
            TmpObj->odesc->intersect(BB, true)) {
        while (ObjList) {
            tlst_t *o = ObjList;
            ObjList = ObjList->next;
            if (!type_ok(o->odesc) || !o->odesc->ldesc()->isSelectable() ||
                    !o->odesc->intersect(BB, true)) {
                delete o;
                continue;
            }
            o->next = 0;
            return (o);
        }
    }

    // Clear list.
    tlst_t::destroy(ObjList);
    ObjList = 0;

    if (!wdesc)
        return (0);
    int expand_level = wdesc->Attrib()->expand_level(DSP()->CurMode());
    if (expand_level < 0)
        expand_level = CDMAXCALLDEPTH;
    CDs *sdesc = wdesc->CurCellDesc(wdesc->Mode());
    if (!sdesc)
        return (0);

    tlst_t *o0 = 0, *oe = 0;
    CDcxy arg[CDMAXCALLDEPTH];
    CDl *ld;
    CDlgen gen(DSP()->CurMode(), CDlgen::TopToBotWithCells);
    while ((ld = gen.next()) != 0) {
        if (!ld->isSelectable())
            continue;
        sPF pfg(sdesc, BB, ld, expand_level);
        pfg.set_info_mode(wdesc->DisplFlags());
        CDo *ocpy;
        while ((ocpy = pfg.next(false, true)) != 0) {
            if (!type_ok(ocpy)) {
                delete ocpy;
                continue;
            }
            int n = pfg.info_stack(arg);
            CDclxy *c0 = 0, *ce = 0;
            CDcxy *a = arg;
            for (int i = 0; i < n; i++) {
                // listing order: bottom -> top
                if (!c0)
                    c0 = ce = new CDclxy(a->cdesc, 0, a->xind, a->yind);
                else {
                    ce->next = new CDclxy(a->cdesc, 0, a->xind, a->yind);
                    ce = ce->next;
                }
                a++;
            }
            if (!o0)
                o0 = oe = new tlst_t(ocpy, c0, 0);
            else {
                oe->next = new tlst_t(ocpy, c0, 0);
                oe = oe->next;
            }
        }
    }
    if (o0) {
        ObjList = o0->next;
        o0->next = 0;
    }
    return (o0);
}


// In expanded mode:
// 1.  All objects in the parent subcell of the clicked-on
//     object are shown selected.
// 2.  The info window displays info about the parent subcell.
//
void
InfoState::show_cell_expand(bool exp, bool clear)
{
    if (exp && !ExpMode) {
        if (!TmpObj || !TmpObj->odesc)
            return;

        CDs *sd;
        if (TmpObj->cdescs)
            sd = TmpObj->cdescs->cdesc->masterCell();
        else {
            if (!EV()->CurrentWin())
                return;
            sd = EV()->CurrentWin()->CurCellDesc(EV()->CurrentWin()->Mode());
        }
        if (!sd)
            // "can't happen"
            return;
        ExpMode = true;

        if (!TmpObj->cdescs) {
            // Target object is in current cell.  Select all objects in
            // current cell, show current cell info.

            CDol *ol0 = 0;
            CDlgen lgen(DSP()->CurMode(), CDlgen::BotToTopWithCells);
            CDl *ld;
            while ((ld = lgen.next()) != 0) {
                if (!ld->isSelectable())
                    continue;
                CDg gdesc;
                gdesc.init_gen(sd, ld);
                const CDo *odref = TmpObj->odesc;
                if (odref->is_copy())
                    odref = odref->const_next_odesc();
                CDo *od;
                while ((od = gdesc.next()) != 0) {
                    if (od == odref)
                        // Already in selections list.
                        continue;
                    if (!Selections.typeSelected(od))
                        continue;
                    CDo *newod = od->copyObject(true);
                    ol0 = new CDol(newod, ol0);
                    newod->set_state(CDVanilla);
                    Selections.insertObject(CurCell(), newod);
                }
            }
            ExpObjs = ol0;
            show_cell_info(sd);
        }
        else {
            // Target object is in a subcell.  Select all objects in this
            // subcell instance, show instance info.

            // Need a reverse-order stack trace.
            CDclxy *c0 = 0;
            for (CDclxy *cl = TmpObj->cdescs; cl; cl = cl->next)
                c0 = new CDclxy(cl->cdesc, c0, cl->xind, cl->yind);

            // Push the transformations.
            cTfmStack stk;
            stk.TPush();
            for (CDclxy *cl = c0; cl; cl = cl->next) {
                stk.TPush();
                stk.TApplyTransform(cl->cdesc);
                stk.TPremultiply();
                if (cl->xind || cl->yind) {
                    CDap ap(cl->cdesc);
                    stk.TTransMult(cl->xind*ap.dx, cl->yind*ap.dy);
                }
            }

            // Grab the objects, insert transformed copies into the
            // selection queue.

            CDol *ol0 = 0;
            CDlgen lgen(DSP()->CurMode(), CDlgen::BotToTopWithCells);
            CDl *ld;
            while ((ld = lgen.next()) != 0) {
                if (!ld->isSelectable())
                    continue;
                CDg gdesc;
                gdesc.init_gen(sd, ld);
                const CDo *odref = TmpObj->odesc;
                if (odref->is_copy())
                    odref = odref->const_next_odesc();
                CDo *od;
                while ((od = gdesc.next()) != 0) {
                    if (od == odref)
                        // Already in selections list.
                        continue;
                    if (!Selections.typeSelected(od))
                        continue;

                    CDo *newod = od->copyObjectWithXform(&stk, true);
                    ol0 = new CDol(newod, ol0);
                    newod->set_state(CDVanilla);
                    Selections.insertObject(CurCell(), newod);
                }
            }
            ExpObjs = ol0;

            // Build a temporary tlst_t for showing instance info.
            tlst_t tlst(0, 0, 0);
            while (c0) {
                CDclxy *cl = c0;
                c0 = c0->next;
                cl->next = tlst.cdescs;
                tlst.cdescs = cl;
            }
            c0 = tlst.cdescs;
            tlst.cdescs = tlst.cdescs->next;
            tlst.odesc = (CDo*)c0->cdesc;
            delete c0;

            show_obj_info(&tlst);
            CDclxy::destroy(tlst.cdescs);
            tlst.cdescs = 0;
            tlst.odesc = 0;
        }
    }
    else if (!exp && ExpMode) {
        ExpMode = false;
        Selections.deselectTypes(CurCell(), 0);
        while (ExpObjs) {
            CDol *o = ExpObjs;
            ExpObjs = ExpObjs->next;
            delete o->odesc;
            delete o;
        }
        if (!clear) {
            TmpObj->odesc->set_state(CDVanilla);
            Selections.insertObject(CurCell(), TmpObj->odesc);
            show_obj_info(TmpObj);
        }
    }
}


// Static function.
// Dump info for all objects in the selection queue to the file.
//
void
InfoState::info_to_file(FILE *fp)
{
    sSelGen sg(Selections, CurCell());
    CDo *od;
    while ((od = sg.next()) != 0) {
        char *t = XM()->Info(od);
        fputs(t, fp);
        fputs("\n", fp);
        delete [] t;
    }
}


// Static function.
// Pop up a window displaying the info.
//
void
InfoState::show_cell_info(CDs *sdesc)
{
    char buf[64];
    sprintf(buf, "Type: ");
    if (sdesc == CurCell())
        sprintf(buf + strlen(buf), "Current Cell\n");
    else
        sprintf(buf + strlen(buf), "Cell\n");
    char *str = lstring::copy(buf);
    if (sdesc) {
        char *s = XM()->Info(sdesc, 100);
        str = lstring::build_str(str, s);
        delete [] s;
    }
    else
        str = lstring::build_str(str, "undefined\n");

    DSPmainWbag(PopUpInfo2(MODE_ON, str, info_cb, 0, STY_FIXED))
    delete [] str;
}


void
InfoState::show_obj_info(tlst_t *ol)
{
    sLstr lstr;
    if (!ol->cdescs)
        lstr.add("In current cell:\n");
    else if (!ol->cdescs->next) {
        lstr.add("In subcell ");
        lstr.add(Tstring(ol->cdescs->cdesc->cellname()));
        lstr.add(":\n");
    }
    else {
        // The printing order is top down.
        stringlist *s0 = 0;
        for (CDclxy *cl = ol->cdescs; cl; cl = cl->next) {
            s0 = new stringlist(
                lstring::copy(Tstring(cl->cdesc->cellname())), s0);
        }
        lstr.add("Instance hierarchy (top-down):\n");
        for (stringlist *s = s0; s; s = s->next) {
            lstr.add("  ");
            lstr.add(s->string);
            lstr.add("\n");
        }
        stringlist::destroy(s0);
    }

    if (!ol->cdescs) {
        // Don't pass a copy in this case.
        char *s;
        if (ol->odesc->is_copy())
            s = XM()->Info(ol->odesc->const_next_odesc());
        else
            s = XM()->Info(ol->odesc);
        lstr.add(s);
        delete [] s;
    }
    else {
        char *s = XM()->Info(ol->odesc);
        lstr.add(s);
        delete [] s;
    }
    DSPmainWbag(PopUpInfo2(MODE_ON, lstr.string(), info_cb, 0, STY_FIXED))
}


// Static function.
bool
InfoState::info_cb(bool state, void *arg)
{
    if (!state) {
        if (arg == GRtextPopupHelp) {
            DSPmainWbag(PopUpHelp("xic:info"))
        }
        else if (InfoCmd)
            InfoCmd->esc();
    }
    else {
        if (!InfoCmd) {
            InfoCmd = new InfoState("INFO", "xic:info");

            sSelGen sg(Selections, CurCell());
            CDo *od;
            while ((od = sg.next()) != 0)
                InfoCmd->SelBack = new CDol(od, InfoCmd->SelBack);

            Selections.deselectTypes(CurCell(), 0);
            if (!EV()->PushCallback(InfoCmd)) {
                delete InfoCmd;
                InfoCmd = 0;
                return (false);
            }
        }
        DSPmainWbag(PopUpInfo2(MODE_ON, msg, info_cb, 0, STY_FIXED))
    }
    return (false);
}
// End of InfoState functions.

